"""Run against a freshly built binary with an isolated temporary database."""
import concurrent.futures, json, os, pathlib, socket, subprocess, sys, tempfile, time, urllib.request, urllib.error

binary=pathlib.Path(sys.argv[1]).resolve()
root=pathlib.Path(__file__).resolve().parent
checks=0
def check(ok, label):
    global checks
    assert ok, label
    checks+=1
    print('PASS',label,flush=True)

with tempfile.TemporaryDirectory(prefix='ss-test-',dir=root) as directory:
    data=pathlib.Path(directory)/'data'
    env=dict(os.environ, DATA_DIR=str(data), SEED_DEMO='0', CORS_ORIGIN='http://localhost:5173')
    with socket.socket() as s: s.bind(('127.0.0.1',0)); port=s.getsockname()[1]
    base=f'http://127.0.0.1:{port}'
    def call(method,path,body=None,token=None,headers=None,raw=None):
        h={'Content-Type':'application/json',**(headers or {})}
        if token: h['Authorization']='Bearer '+token
        payload=raw if raw is not None else (json.dumps(body).encode() if body is not None else None)
        req=urllib.request.Request(base+path,data=payload,headers=h,method=method)
        try: response=urllib.request.urlopen(req,timeout=10)
        except urllib.error.HTTPError as e: response=e
        with response:
            text=response.read()
            return response.status,json.loads(text) if text else None
    log=open(pathlib.Path(directory)/'server.log','w')
    def start():
        process=subprocess.Popen([str(binary),str(port)],cwd=binary.parent,env=env,stdout=log,stderr=log,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
        for _ in range(100):
            if process.poll() is not None: raise RuntimeError('Server exited: '+str(process.returncode))
            try:
                if call('GET','/api/health')[0]==200: return process
            except OSError: pass
            time.sleep(.1)
        raise RuntimeError('Server did not become healthy')
    process=start()
    try:
        check(call('GET','/api/seniors')[1]==[], 'production starts without demo accounts')
        users=[]
        for i,year in enumerate([1,4,2,1,1,1,1,1]):
            status,user=call('POST','/api/auth/signup',{'name':'Review Student','email':f'review{i}@thapar.edu','rollNumber':str(1000000000+i),'password':'testpassword','yearNumber':year,'branch':'COE'})
            check(status==201, f'signup {i}')
            users.append(user)
        junior,senior,outsider=users[:3]
        token=junior['token']; mentor=senior['token']
        check(call('POST','/api/auth/login',{'email':'review0@thapar.edu','password':'wrong'})[0]==401,'reject wrong password')
        check(call('POST','/api/auth/login',{'email':'review0@thapar.edu','password':'testpassword'})[0]==200,'verify Argon2 password')
        stored=json.loads((data/'db.json').read_text())
        check(all(u['passwordHash'].startswith('$argon2id$') for u in stored['users']),'saved passwords use Argon2id')
        check(call('PUT','/api/me',{'yearNumber':4},token)[0]==403,'block year promotion')
        check(call('PUT','/api/me',{'name':'Changed Name','slotsAvailable':99},token)[0]==400,'reject invalid profile batch')
        check(call('GET','/api/me',token=token)[1]['name']=='Review Student','failed profile batch leaves name unchanged')
        check(call('PUT','/api/me',{'name':'Valid Name'},token)[0]==200,'valid profile update')
        # Simulate inability to replace the temp file; the original data must survive.
        saved=(data/'db.json').read_bytes()
        (data/'db.json.tmp').mkdir()
        check(call('PUT','/api/me',{'name':'Unsaved Name'},token)[0]==500,'report save failure')
        check((data/'db.json').read_bytes()==saved,'save failure preserves database bytes')
        check(call('GET','/api/me',token=token)[1]['name']=='Valid Name','save failure rolls back profile')
        (data/'db.json.tmp').rmdir()
        _,q=call('POST','/api/questions',{'title':'Anonymous doubt','description':'Please explain classes','isAnonymous':True},token)
        check(q['authorName'].startswith('TIETian_') and 'authorId' not in q,'anonymous response hides author')
        check(call('POST','/api/bookings',{'questionId':q['id'],'topic':'Help'},mentor)[0]==403,'anonymous booking cannot reveal author')
        check(call('GET','/api/bookings',token=mentor)[1]==[],'rejected anonymous booking creates no record')
        status,conv=call('POST','/api/conversations',{'userId':senior['user']['id']},token)
        check(status==201,'create private chat')
        check(call('POST',f"/api/conversations/{conv['id']}/messages",{'text':'Private hello'},outsider['token'])[0]==404,'outsider cannot send into chat')
        check(call('GET','/api/conversations',token=outsider['token'])[1]==[],'outsider cannot read chat')
        check(call('POST',f"/api/conversations/{conv['id']}/messages",{'text':'Private hello'},token)[0]==201,'send real message')
        check(call('GET','/api/conversations',token=mentor)[1][0]['unread']==1,'recipient sees unread message')
        check(call('POST',f"/api/conversations/{conv['id']}/read",{},mentor)[0]==200,'mark conversation read')
        check(call('GET','/api/conversations',token=mentor)[1][0]['unread']==0,'unread count clears')
        _,slot=call('POST','/api/slots',{'courseCode':'CS','topic':'OOP','date':'Tomorrow','time':'10 AM','meetingType':'Virtual'},mentor)
        with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:
            results=list(pool.map(lambda u: call('POST',f"/api/slots/{slot['id']}/claim",{},u['token'])[0], [junior,*users[3:]]))
        check(results.count(200)==1 and results.count(409)==5,'six simultaneous claims yield exactly one winner')
        check(call('GET','/api/conversations')[0]==401,'private endpoint requires login')
        check(call('GET','/api/health',headers={'Origin':'https://wrong.example'})[0]==403,'CORS rejects unknown origin')
        check(call('GET','/api/health',headers={'Origin':'http://localhost:5173'})[0]==200,'CORS accepts configured origin')
        check(call('POST','/api/questions',token=token,raw=b'{"title":NaN}')[0]==400,'parser rejects non-JSON number')
        check(call('POST','/api/questions',token=token,raw=b'{"x":'+b'['*70+b'0'+b']'*70+b'}')[0]==400,'parser limits nesting')
        slow=socket.create_connection(('127.0.0.1',port)); slow.sendall(b'GET /api/health HTTP/1.1\r\nHost: localhost\r\n')
        t=time.monotonic(); check(call('GET','/api/health')[0]==200 and time.monotonic()-t<2,'slow socket does not block another request'); slow.close()
        process.terminate(); process.wait(timeout=10); process=start()
        check(call('GET','/api/me',token=token)[0]==200,'restart preserves valid session')
        check(any(item['id']==q['id'] and item['isMine'] for item in call('GET','/api/questions',token=token)[1]),'doubt and ownership persist after restart')
        check(all(item['tokenHash']!=token for item in json.loads((data/'db.json').read_text())['sessions']),'stored sessions do not expose bearer tokens')
        _,session=call('POST','/api/auth/login',{'email':'review0@thapar.edu','password':'testpassword'})
        token=session['token']
        check(session['user']['name']=='Valid Name','profile persists after restart')
        check(call('GET','/api/conversations',token=token)[1][0]['messages'][0]['text']=='Private hello','message persists after restart')
        check(call('POST','/api/auth/logout',{},token)[0]==200 and call('GET','/api/me',token=token)[0]==401,'logout invalidates token')
        process.terminate(); process.wait(timeout=10); process=start()
        check(call('GET','/api/me',token=token)[0]==401,'logout stays invalid after restart')
        print(f'{checks} checks passed',flush=True)
    finally:
        process.terminate(); process.wait(timeout=10); log.close()

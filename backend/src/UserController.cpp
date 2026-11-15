#include "Controllers.h"
#include "Utils.h"

void UserController::registerRoutes(HttpServer& server) {
    server.post("/api/auth/signup", [this](const HttpRequest& req, HttpResponse& res) { signup(req, res); });
    server.post("/api/auth/login", [this](const HttpRequest& req, HttpResponse& res) { login(req, res); });
    server.post("/api/auth/logout", [this](const HttpRequest& req, HttpResponse& res) { logout(req, res); });
    server.get("/api/me", [this](const HttpRequest& req, HttpResponse& res) { getMe(req, res); });
    server.put("/api/me", [this](const HttpRequest& req, HttpResponse& res) { updateMe(req, res); });
    server.get("/api/seniors", [this](const HttpRequest& req, HttpResponse& res) { listSeniors(req, res); });
    server.post("/api/users/:id/connect", [this](const HttpRequest& req, HttpResponse& res) { toggleConnect(req, res); });
}

Json UserController::sessionJson(const Session& session) const {
    Json j = Json::object();
    j["token"] = session.token;
    j["user"] = userJson(*session.user, session.user);
    return j;
}

void UserController::signup(const HttpRequest& req, HttpResponse& res) {
    res.send(sessionJson(auth_.signup(req.json())), 201);
}

void UserController::login(const HttpRequest& req, HttpResponse& res) {
    Json body = req.json();
    Session session = auth_.login(body["email"].asString(), body["password"].asString(), AuthService::clientIp(req));
    res.send(sessionJson(session));
}

void UserController::logout(const HttpRequest& req, HttpResponse& res) {
    auth_.logout(AuthService::tokenFromRequest(req));
    Json j = Json::object();
    j["ok"] = true;
    res.send(j);
}

void UserController::getMe(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    res.send(userJson(me, &me));
}

// Only profile details can be edited. Year, branch, email and roll number are
// fixed at sign-up, so a junior cannot promote themselves to "senior".
void UserController::updateMe(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    Json body = req.json();
    User updated = me;

    const char* const lockedFields[] = {"year", "yearNumber", "batch", "branch", "email", "rollNumber",
                                        "karma", "verified", "deansList", "rating", "reviewsCount"};
    for (std::size_t i = 0; i < sizeof(lockedFields) / sizeof(lockedFields[0]); ++i) {
        if (body.has(lockedFields[i])) {
            throw HttpError(403, std::string(lockedFields[i]) + " cannot be changed after sign-up");
        }
    }

    if (body.has("name")) {
        std::string name = requireText(body, "name", "Name", 60);
        if (!utils::isLettersAndSpaces(name)) throw HttpError(400, "Name can only contain alphabetic letters and spaces");
        updated.name = name;
    }
    if (body.has("hostel")) updated.hostel = optionalText(body, "hostel", "", 60);
    if (body.has("cgpa")) updated.cgpa = optionalText(body, "cgpa", "", 5);
    if (body.has("avatar")) updated.avatar = optionalText(body, "avatar", "", 500);
    if (body.has("bio")) updated.bio = optionalText(body, "bio", "", 600);
    if (body.has("company")) updated.company = optionalText(body, "company", "", 80);
    if (body.has("achievement")) updated.achievement = optionalText(body, "achievement", "", 120);
    if (body.has("expertise")) updated.expertise = body["expertise"].asStringList();
    if (body.has("societies")) updated.societies = body["societies"].asStringList();
    if (body.has("slotsAvailable")) {
        long long slots = body["slotsAvailable"].asInt(-1);
        if (slots < 0 || slots > 20) throw HttpError(400, "slotsAvailable must be between 0 and 20");
        updated.slotsAvailable = static_cast<int>(slots);
    }

    User original = me;
    me = updated;
    try { db_.save(); } catch (...) { me = original; throw; }
    res.send(userJson(me, &me));
}

void UserController::listSeniors(const HttpRequest& req, HttpResponse& res) {
    User* viewer = auth_.userFromRequest(req);
    Json list = Json::array();
    for (std::size_t i = 0; i < db_.users.all().size(); ++i) {
        const User& user = db_.users.all()[i];
        if (viewer && user.id() == viewer->id()) continue;
        list.push(userJson(user, viewer));
    }
    res.send(list);
}

void UserController::toggleConnect(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    User& other = findUser(req.paramId("id"));
    if (other.id() == me.id()) throw HttpError(400, "You cannot connect with yourself");

    bool connected = me.toggleConnection(other.id());
    if (connected) {
        db_.notify(other.id(), "New Connection",
                   me.name + " (" + me.yearLabel() + " \u2022 " + me.branch() + ") connected with you.",
                   "academic", "seniors");
    }
    db_.save();
    res.send(userJson(other, &me));
}

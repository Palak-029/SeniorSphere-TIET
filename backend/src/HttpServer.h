#pragma once
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include "Json.h"
#include <memory>
#include <vector>

class HttpError : public std::runtime_error {
public:
    HttpError(int status, const std::string& message) : std::runtime_error(message), status_(status) {}
    int status() const { return status_; }
private:
    int status_;
};
struct HttpRequest {
    std::string method, path, body, clientIp;
    std::map<std::string, std::string> headers, params;
    std::string header(const std::string& name) const;
    int paramId(const std::string& name) const;
    Json json() const;
};
struct HttpResponse {
    int status = 200;
    std::string body;
    void send(const Json& data, int code = 200) { status = code; body = data.dump(); }
};
// cpp-httplib handles network connections. One lock protects the shared data.
class HttpServer {
public:
    using Handler = std::function<void(const HttpRequest&, HttpResponse&)>;
    HttpServer();
    ~HttpServer();
    void get(const std::string& path, Handler h);
    void post(const std::string& path, Handler h);
    void put(const std::string& path, Handler h);
    void del(const std::string& path, Handler h);
    bool listen(int port);
private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

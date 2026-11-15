#include "HttpServer.h"
#include "../third_party/httplib.h"
#include "Utils.h"
#include <cstdlib>
#include <limits>
#include <sstream>
#include <iostream>

std::string HttpRequest::header(const std::string& name) const {
    auto it = headers.find(utils::toLower(name));
    return it == headers.end() ? "" : it->second;
}
int HttpRequest::paramId(const std::string& name) const {
    auto it = params.find(name);
    if (it == params.end() || !utils::isDigits(it->second)) throw HttpError(404, "Not found");
    try {
        long long id = std::stoll(it->second);
        if (id <= 0 || id > std::numeric_limits<int>::max()) throw std::out_of_range("id");
        return static_cast<int>(id);
    } catch (...) { throw HttpError(404, "Not found"); }
}
Json HttpRequest::json() const {
    try {
        Json data = body.empty() ? Json::object() : Json::parse(body);
        if (!data.isObject()) throw HttpError(400, "Body must be a JSON object");
        return data;
    } catch (const std::exception&) { throw HttpError(400, "Invalid JSON object"); }
}
struct HttpServer::Implementation {
    httplib::Server server_;
    std::mutex dataMutex_;
    httplib::Server::Handler wrap(Handler handler);
};
HttpServer::~HttpServer() = default;
HttpServer::HttpServer() : implementation_(new Implementation) {
    auto& server_ = implementation_->server_;
    server_.new_task_queue = [] { return new httplib::ThreadPool(8, 8, 64); };
    server_.set_payload_max_length(1024 * 1024);
    server_.set_read_timeout(5, 0);
    server_.set_write_timeout(5, 0);
    server_.set_keep_alive_max_count(20);
    server_.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        std::string origin = req.get_header_value("Origin");
        std::string allowed = std::getenv("CORS_ORIGIN") ? std::getenv("CORS_ORIGIN") : "http://localhost:5173,http://127.0.0.1:5173";
        if (!origin.empty()) {
            bool found = false;
            std::istringstream list(allowed);
            std::string item;
            while (std::getline(list, item, ',')) if (utils::trim(item) == origin) found = true;
            if (!found) {
                res.status = 403;
                res.set_content("{\"error\":\"Origin not allowed\"}", "application/json");
                return httplib::Server::HandlerResponse::Handled;
            }
            res.set_header("Access-Control-Allow-Origin", origin);
            res.set_header("Vary", "Origin");
        }
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Cache-Control", "no-store");
        res.set_header("X-Content-Type-Options", "nosniff");
        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });
}
httplib::Server::Handler HttpServer::Implementation::wrap(Handler handler) {
    return [this, handler](const httplib::Request& input, httplib::Response& output) {
        HttpRequest req;
        req.method = input.method; req.path = input.path; req.body = input.body; req.clientIp = input.remote_addr;
        for (const auto& pair : input.headers) req.headers[utils::toLower(pair.first)] = pair.second;
        for (const auto& pair : input.path_params) req.params[pair.first] = pair.second;
        HttpResponse res;
        // Serialize business operations, not socket reads: a slow client does not hold this lock.
        std::lock_guard<std::mutex> guard(dataMutex_);
        try { handler(req, res); }
        catch (const HttpError& e) { Json error = Json::object(); error["error"] = e.what(); res.send(error, e.status()); }
        catch (const std::exception& e) {
            std::cerr << e.what() << '\n';
            Json error = Json::object(); error["error"] = "Internal server error"; res.send(error, 500);
        }
        output.status = res.status;
        output.set_content(res.body, "application/json; charset=utf-8");
    };
}

void HttpServer::get(const std::string& path, Handler h) { implementation_->server_.Get(path, implementation_->wrap(h)); }

void HttpServer::post(const std::string& path, Handler h) { implementation_->server_.Post(path, implementation_->wrap(h)); }

void HttpServer::put(const std::string& path, Handler h) { implementation_->server_.Put(path, implementation_->wrap(h)); }

void HttpServer::del(const std::string& path, Handler h) { implementation_->server_.Delete(path, implementation_->wrap(h)); }

bool HttpServer::listen(int port) { return implementation_->server_.listen("0.0.0.0", port); }

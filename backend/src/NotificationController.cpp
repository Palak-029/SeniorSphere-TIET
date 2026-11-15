#include "Controllers.h"
#include "Utils.h"

void NotificationController::registerRoutes(HttpServer& server) {
    server.get("/api/notifications", [this](const HttpRequest& req, HttpResponse& res) { list(req, res); });
    server.post("/api/notifications/read-all", [this](const HttpRequest& req, HttpResponse& res) { markAllRead(req, res); });
    server.del("/api/notifications", [this](const HttpRequest& req, HttpResponse& res) { clear(req, res); });
}

void NotificationController::list(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    Json list = Json::array();
    for (std::size_t i = db_.notifications.all().size(); i-- > 0 && list.size() < 50;) {
        const Notification& n = db_.notifications.all()[i];
        if (n.userId != me.id()) continue;
        Json j = Json::object();
        j["id"] = n.id();
        j["title"] = n.title;
        j["message"] = n.message;
        j["type"] = n.type;
        j["tab"] = n.tab;
        j["unread"] = n.unread;
        j["time"] = utils::timeAgo(n.createdAt);
        list.push(j);
    }
    res.send(list);
}

void NotificationController::markAllRead(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    for (std::size_t i = 0; i < db_.notifications.all().size(); ++i) {
        Notification& n = db_.notifications.all()[i];
        if (n.userId == me.id()) n.unread = false;
    }
    db_.save();
    Json j = Json::object();
    j["ok"] = true;
    res.send(j);
}

void NotificationController::clear(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    int myId = me.id();
    db_.notifications.removeIf([myId](const Notification& n) { return n.userId == myId; });
    db_.save();
    Json j = Json::object();
    j["ok"] = true;
    res.send(j);
}

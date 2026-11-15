#include <algorithm>

#include "Controllers.h"
#include "Utils.h"

void ChatController::registerRoutes(HttpServer& server) {
    server.get("/api/conversations", [this](const HttpRequest& req, HttpResponse& res) { list(req, res); });
    server.post("/api/conversations", [this](const HttpRequest& req, HttpResponse& res) { start(req, res); });
    server.post("/api/conversations/:id/messages", [this](const HttpRequest& req, HttpResponse& res) { send(req, res); });
    server.post("/api/conversations/:id/read", [this](const HttpRequest& req, HttpResponse& res) { markRead(req, res); });
}

// Students can only open conversations they are part of; anything else looks like "not found".
Conversation& ChatController::findConversationFor(int id, const User& user) {
    Conversation* conversation = db_.conversations.find(id);
    if (!conversation || !conversation->hasParticipant(user.id())) throw HttpError(404, "Conversation not found");
    return *conversation;
}

Json ChatController::conversationJson(const Conversation& c, const User& viewer) {
    int otherId = c.otherParticipant(viewer.id());
    User* other = db_.users.find(otherId);

    Json messages = Json::array();
    int unread = 0;
    std::string lastMessage = "No messages yet. Say hi!";
    long long lastTime = c.updatedAt;
    for (std::size_t i = 0; i < db_.messages.all().size(); ++i) {
        const Message& m = db_.messages.all()[i];
        if (m.conversationId != c.id()) continue;
        Json mj = Json::object();
        mj["id"] = m.id();
        mj["sender"] = m.senderId == viewer.id() ? "me" : "them";
        mj["text"] = m.text;
        mj["timestamp"] = utils::clockTime(m.createdAt);
        messages.push(mj);
        if (m.senderId != viewer.id() && m.id() > c.lastReadBy(viewer.id())) ++unread;
        lastMessage = m.text;
        lastTime = m.createdAt;
    }

    Json j = Json::object();
    j["id"] = c.id();
    j["seniorId"] = otherId;
    j["name"] = other ? other->name : "Former student";
    j["avatar"] = other ? other->avatarUrl() : "";
    j["role"] = other ? other->roleLabel() : "";
    j["status"] = other && utils::now() - other->lastSeen < 60 ? "Online" : "Away";
    j["lastMessage"] = lastMessage;
    j["timestamp"] = utils::clockTime(lastTime);
    j["unread"] = unread;
    j["messages"] = messages;
    return j;
}

void ChatController::list(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    std::vector<const Conversation*> mine;
    for (std::size_t i = 0; i < db_.conversations.all().size(); ++i) {
        if (db_.conversations.all()[i].hasParticipant(me.id())) mine.push_back(&db_.conversations.all()[i]);
    }
    std::stable_sort(mine.begin(), mine.end(),
                     [](const Conversation* a, const Conversation* b) { return a->updatedAt > b->updatedAt; });

    Json list = Json::array();
    for (std::size_t i = 0; i < mine.size(); ++i) list.push(conversationJson(*mine[i], me));
    res.send(list);
}

// Opens the existing conversation with another student, or creates one.
void ChatController::start(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    User& other = findUser(static_cast<int>(req.json()["userId"].asInt()));
    if (other.id() == me.id()) throw HttpError(400, "You cannot message yourself");

    for (std::size_t i = 0; i < db_.conversations.all().size(); ++i) {
        const Conversation& c = db_.conversations.all()[i];
        if (c.hasParticipant(me.id()) && c.hasParticipant(other.id())) {
            res.send(conversationJson(c, me));
            return;
        }
    }
    Conversation& created = db_.conversations.add(Conversation(me.id(), other.id(), utils::now()));
    db_.save();
    res.send(conversationJson(created, me), 201);
}

void ChatController::send(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    Conversation& c = findConversationFor(req.paramId("id"), me);

    Message message;
    message.conversationId = c.id();
    message.senderId = me.id();
    message.text = requireText(req.json(), "text", "Message", 2000);
    message.createdAt = utils::now();
    const Message& saved = db_.messages.add(message);

    c.updatedAt = saved.createdAt;
    c.markRead(me.id(), saved.id());
    db_.save();
    res.send(conversationJson(c, me), 201);
}

void ChatController::markRead(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    Conversation& c = findConversationFor(req.paramId("id"), me);

    int newest = 0;
    for (std::size_t i = 0; i < db_.messages.all().size(); ++i) {
        const Message& m = db_.messages.all()[i];
        if (m.conversationId == c.id() && m.id() > newest) newest = m.id();
    }
    if (newest > c.lastReadBy(me.id())) {
        c.markRead(me.id(), newest);
        db_.save();
    }
    Json j = Json::object();
    j["ok"] = true;
    res.send(j);
}

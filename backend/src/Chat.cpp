#include "Chat.h"

Conversation::Conversation(int firstUserId, int secondUserId, long long now)
    : updatedAt(now), userA_(firstUserId), userB_(secondUserId) {}

void Conversation::markRead(int userId, int messageId) {
    if (userId == userA_ && messageId > lastReadA_) lastReadA_ = messageId;
    if (userId == userB_ && messageId > lastReadB_) lastReadB_ = messageId;
}

Json Conversation::toJson() const {
    Json j = Json::object();
    j["id"] = id_;
    j["userA"] = userA_;
    j["userB"] = userB_;
    j["lastReadA"] = lastReadA_;
    j["lastReadB"] = lastReadB_;
    j["updatedAt"] = updatedAt;
    return j;
}

void Conversation::fromJson(const Json& j) {
    id_ = static_cast<int>(j["id"].asInt());
    userA_ = static_cast<int>(j["userA"].asInt());
    userB_ = static_cast<int>(j["userB"].asInt());
    lastReadA_ = static_cast<int>(j["lastReadA"].asInt());
    lastReadB_ = static_cast<int>(j["lastReadB"].asInt());
    updatedAt = j["updatedAt"].asInt();
}

Json Message::toJson() const {
    Json j = Json::object();
    j["id"] = id_;
    j["conversationId"] = conversationId;
    j["senderId"] = senderId;
    j["text"] = text;
    j["createdAt"] = createdAt;
    return j;
}

void Message::fromJson(const Json& j) {
    id_ = static_cast<int>(j["id"].asInt());
    conversationId = static_cast<int>(j["conversationId"].asInt());
    senderId = static_cast<int>(j["senderId"].asInt());
    text = j["text"].asString();
    createdAt = j["createdAt"].asInt();
}

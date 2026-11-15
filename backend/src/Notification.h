#pragma once

#include <string>

#include "Entity.h"

class Notification : public Entity {
public:
    int userId = 0;
    std::string title;
    std::string message;
    std::string type;  // mentorship | doubt | karma | academic | chat
    std::string tab;   // which page of the site the notification opens
    bool unread = true;
    long long createdAt = 0;

    Json toJson() const override {
        Json j = Json::object();
        j["id"] = id_;
        j["userId"] = userId;
        j["title"] = title;
        j["message"] = message;
        j["type"] = type;
        j["tab"] = tab;
        j["unread"] = unread;
        j["createdAt"] = createdAt;
        return j;
    }

    void fromJson(const Json& j) override {
        id_ = static_cast<int>(j["id"].asInt());
        userId = static_cast<int>(j["userId"].asInt());
        title = j["title"].asString();
        message = j["message"].asString();
        type = j["type"].asString();
        tab = j["tab"].asString();
        unread = j["unread"].asBool(true);
        createdAt = j["createdAt"].asInt();
    }
};

#pragma once

#include <string>

#include "Entity.h"

// A private 1:1 chat between two students.
class Conversation : public Entity {
public:
    Conversation() {}
    Conversation(int firstUserId, int secondUserId, long long now);

    bool hasParticipant(int userId) const { return userId == userA_ || userId == userB_; }
    int otherParticipant(int userId) const { return userId == userA_ ? userB_ : userA_; }

    // Id of the newest message this participant has seen (for unread counts).
    int lastReadBy(int userId) const { return userId == userA_ ? lastReadA_ : lastReadB_; }
    void markRead(int userId, int messageId);

    long long updatedAt = 0;

    Json toJson() const override;
    void fromJson(const Json& data) override;

private:
    int userA_ = 0;
    int userB_ = 0;
    int lastReadA_ = 0;
    int lastReadB_ = 0;
};

class Message : public Entity {
public:
    int conversationId = 0;
    int senderId = 0;
    std::string text;
    long long createdAt = 0;

    Json toJson() const override;
    void fromJson(const Json& data) override;
};

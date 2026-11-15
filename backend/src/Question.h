#pragma once

#include <set>
#include <string>
#include <vector>

#include "Entity.h"

class Answer {
public:
    int id = 0;
    int authorId = 0;
    std::string content;
    int upvotes = 0;
    bool isAccepted = false;
    long long createdAt = 0;

    Json toJson() const;
    static Answer fromJson(const Json& data);
};

// A doubt posted on the forum, together with its answers.
class Question : public Entity {
public:
    int authorId = 0;
    bool isAnonymous = false;
    std::string anonymousName;  // e.g. "TIETian_#412", shown instead of the real name
    std::string title;
    std::string description;
    std::string branch;
    std::string subject;
    std::string category;
    std::vector<std::string> tags;
    int upvotes = 0;
    long long createdAt = 0;
    std::vector<Answer> answers;

    bool hasUpvoted(int userId) const { return upvoterIds_.count(userId) > 0; }
    bool toggleUpvote(int userId);  // returns true if now upvoted
    const Answer& addAnswer(int authorId, const std::string& content, long long now);
    bool hasAcceptedAnswer() const;

    Json toJson() const override;
    void fromJson(const Json& data) override;

private:
    std::set<int> upvoterIds_;
};

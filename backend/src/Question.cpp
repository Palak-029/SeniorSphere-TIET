#include "Question.h"

Json Answer::toJson() const {
    Json j = Json::object();
    j["id"] = id;
    j["authorId"] = authorId;
    j["content"] = content;
    j["upvotes"] = upvotes;
    j["isAccepted"] = isAccepted;
    j["createdAt"] = createdAt;
    return j;
}

Answer Answer::fromJson(const Json& j) {
    Answer a;
    a.id = static_cast<int>(j["id"].asInt());
    a.authorId = static_cast<int>(j["authorId"].asInt());
    a.content = j["content"].asString();
    a.upvotes = static_cast<int>(j["upvotes"].asInt());
    a.isAccepted = j["isAccepted"].asBool();
    a.createdAt = j["createdAt"].asInt();
    return a;
}

bool Question::toggleUpvote(int userId) {
    if (upvoterIds_.erase(userId) > 0) {
        --upvotes;
        return false;
    }
    upvoterIds_.insert(userId);
    ++upvotes;
    return true;
}

const Answer& Question::addAnswer(int answerAuthorId, const std::string& content, long long now) {
    Answer answer;
    answer.id = answers.empty() ? 1 : answers.back().id + 1;
    answer.authorId = answerAuthorId;
    answer.content = content;
    answer.createdAt = now;
    answers.push_back(answer);
    return answers.back();
}

bool Question::hasAcceptedAnswer() const {
    for (std::size_t i = 0; i < answers.size(); ++i) {
        if (answers[i].isAccepted) return true;
    }
    return false;
}

Json Question::toJson() const {
    Json j = Json::object();
    j["id"] = id_;
    j["authorId"] = authorId;
    j["isAnonymous"] = isAnonymous;
    j["anonymousName"] = anonymousName;
    j["title"] = title;
    j["description"] = description;
    j["branch"] = branch;
    j["subject"] = subject;
    j["category"] = category;
    j["tags"] = toJsonList(tags);
    j["upvotes"] = upvotes;
    j["upvoterIds"] = idsToJson(upvoterIds_);
    j["createdAt"] = createdAt;
    Json list = Json::array();
    for (std::size_t i = 0; i < answers.size(); ++i) list.push(answers[i].toJson());
    j["answers"] = list;
    return j;
}

void Question::fromJson(const Json& j) {
    id_ = static_cast<int>(j["id"].asInt());
    authorId = static_cast<int>(j["authorId"].asInt());
    isAnonymous = j["isAnonymous"].asBool();
    anonymousName = j["anonymousName"].asString();
    title = j["title"].asString();
    description = j["description"].asString();
    branch = j["branch"].asString();
    subject = j["subject"].asString();
    category = j["category"].asString("Academics");
    tags = j["tags"].asStringList();
    upvotes = static_cast<int>(j["upvotes"].asInt());
    upvoterIds_ = idsFromJson(j["upvoterIds"]);
    createdAt = j["createdAt"].asInt();
    answers.clear();
    for (std::size_t i = 0; i < j["answers"].items().size(); ++i) {
        answers.push_back(Answer::fromJson(j["answers"].items()[i]));
    }
}

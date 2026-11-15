#include <algorithm>
#include <cstdlib>

#include "Controllers.h"
#include "Utils.h"

void ForumController::registerRoutes(HttpServer& server) {
    server.get("/api/questions", [this](const HttpRequest& req, HttpResponse& res) { list(req, res); });
    server.post("/api/questions", [this](const HttpRequest& req, HttpResponse& res) { create(req, res); });
    server.post("/api/questions/:id/upvote", [this](const HttpRequest& req, HttpResponse& res) { toggleUpvote(req, res); });
    server.post("/api/questions/:id/answers", [this](const HttpRequest& req, HttpResponse& res) { addAnswer(req, res); });
}

Question& ForumController::findQuestion(int id) {
    Question* question = db_.questions.find(id);
    if (!question) throw HttpError(404, "Question not found");
    return *question;
}

Json ForumController::questionJson(const Question& q, const User* viewer) {
    User* author = db_.users.find(q.authorId);
    Json j = Json::object();
    j["id"] = q.id();
    j["title"] = q.title;
    j["description"] = q.description;
    j["authorName"] = q.isAnonymous ? q.anonymousName : userName(q.authorId);
    j["authorRole"] = author ? author->yearLabel() + " \u2022 " + author->branch() : "";
    j["isAnonymous"] = q.isAnonymous;
    j["isMine"] = viewer != nullptr && viewer->id() == q.authorId;
    j["branch"] = q.branch;
    j["subject"] = q.subject;
    j["tags"] = toJsonList(q.tags);
    j["category"] = q.category;
    j["upvotes"] = q.upvotes;
    j["userUpvoted"] = viewer != nullptr && q.hasUpvoted(viewer->id());
    j["hasAcceptedAnswer"] = q.hasAcceptedAnswer();
    j["timestamp"] = utils::timeAgo(q.createdAt);

    Json answers = Json::array();
    for (std::size_t i = 0; i < q.answers.size(); ++i) {
        const Answer& a = q.answers[i];
        User* answerAuthor = db_.users.find(a.authorId);
        Json aj = Json::object();
        aj["id"] = a.id;
        aj["authorName"] = userName(a.authorId);
        aj["authorRole"] = userRole(a.authorId);
        aj["authorAvatar"] = answerAuthor ? answerAuthor->avatarUrl() : "";
        aj["content"] = a.content;
        aj["upvotes"] = a.upvotes;
        aj["isAccepted"] = a.isAccepted;
        aj["timestamp"] = utils::timeAgo(a.createdAt);
        answers.push(aj);
    }
    j["answers"] = answers;
    return j;
}

void ForumController::list(const HttpRequest& req, HttpResponse& res) {
    User* viewer = auth_.userFromRequest(req);
    std::vector<const Question*> sorted;
    for (std::size_t i = 0; i < db_.questions.all().size(); ++i) sorted.push_back(&db_.questions.all()[i]);
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const Question* a, const Question* b) { return a->createdAt > b->createdAt; });

    Json list = Json::array();
    for (std::size_t i = 0; i < sorted.size(); ++i) list.push(questionJson(*sorted[i], viewer));
    res.send(list);
}

void ForumController::create(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    Json body = req.json();

    Question q;
    q.authorId = me.id();
    q.title = requireText(body, "title", "Title", 200);
    q.description = requireText(body, "description", "Description", 5000);
    q.subject = optionalText(body, "subject", "General TIET", 60);
    q.branch = optionalText(body, "branch", me.branch(), 20);
    q.category = "Academics";
    q.isAnonymous = body["isAnonymous"].asBool(false);
    if (q.isAnonymous) {
        long number = std::strtol(utils::randomHex(2).c_str(), nullptr, 16) % 900 + 100;
        q.anonymousName = "TIETian_#" + std::to_string(number);
    }
    q.tags.push_back(q.subject);
    q.tags.push_back(q.branch);
    q.tags.push_back("AcademicDoubt");
    q.createdAt = utils::now();
    q.toggleUpvote(me.id());

    Question& saved = db_.questions.add(q);
    db_.save();
    res.send(questionJson(saved, &me), 201);
}

void ForumController::toggleUpvote(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    Question& q = findQuestion(req.paramId("id"));
    q.toggleUpvote(me.id());
    db_.save();
    res.send(questionJson(q, &me));
}

void ForumController::addAnswer(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    Question& q = findQuestion(req.paramId("id"));
    std::string content = requireText(req.json(), "content", "Answer", 5000);

    q.addAnswer(me.id(), content, utils::now());
    if (q.authorId != me.id()) {
        me.addKarma(10);
        db_.notify(q.authorId, "New Answer On Your Doubt",
                   me.name + " (" + me.roleLabel() + ") replied to \"" + q.title + "\".", "doubt", "ask");
    }
    db_.save();
    res.send(questionJson(q, &me), 201);
}

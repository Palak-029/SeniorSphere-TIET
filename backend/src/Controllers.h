#pragma once

#include "Controller.h"

// Sign-up, login, profile and the senior directory.
class UserController : public Controller {
public:
    using Controller::Controller;
    void registerRoutes(HttpServer& server) override;

private:
    void signup(const HttpRequest& req, HttpResponse& res);
    void login(const HttpRequest& req, HttpResponse& res);
    void logout(const HttpRequest& req, HttpResponse& res);
    void getMe(const HttpRequest& req, HttpResponse& res);
    void updateMe(const HttpRequest& req, HttpResponse& res);
    void listSeniors(const HttpRequest& req, HttpResponse& res);
    void toggleConnect(const HttpRequest& req, HttpResponse& res);

    Json sessionJson(const Session& session) const;
};

// Doubt forum: questions, answers and upvotes.
class ForumController : public Controller {
public:
    using Controller::Controller;
    void registerRoutes(HttpServer& server) override;

private:
    void list(const HttpRequest& req, HttpResponse& res);
    void create(const HttpRequest& req, HttpResponse& res);
    void toggleUpvote(const HttpRequest& req, HttpResponse& res);
    void addAnswer(const HttpRequest& req, HttpResponse& res);

    Question& findQuestion(int id);
    Json questionJson(const Question& question, const User* viewer);
};

// Real-time-ish direct messages between two students (the frontend polls).
class ChatController : public Controller {
public:
    using Controller::Controller;
    void registerRoutes(HttpServer& server) override;

private:
    void list(const HttpRequest& req, HttpResponse& res);
    void start(const HttpRequest& req, HttpResponse& res);
    void send(const HttpRequest& req, HttpResponse& res);
    void markRead(const HttpRequest& req, HttpResponse& res);

    Conversation& findConversationFor(int id, const User& user);
    Json conversationJson(const Conversation& conversation, const User& viewer);
};

// Open slots posted by seniors, help requests from juniors, and booked sessions.
class MentorshipController : public Controller {
public:
    using Controller::Controller;
    void registerRoutes(HttpServer& server) override;

private:
    void listSlots(const HttpRequest& req, HttpResponse& res);
    void createSlot(const HttpRequest& req, HttpResponse& res);
    void claimSlot(const HttpRequest& req, HttpResponse& res);
    void listHelpRequests(const HttpRequest& req, HttpResponse& res);
    void createHelpRequest(const HttpRequest& req, HttpResponse& res);
    void acceptHelpRequest(const HttpRequest& req, HttpResponse& res);
    void listBookings(const HttpRequest& req, HttpResponse& res);
    void createBooking(const HttpRequest& req, HttpResponse& res);

    static void requireSenior(const User& user, const std::string& action);
    static Json cleanVenue(const Json& venue);
    Json slotJson(const OpenSlot& slot, const User* viewer);
    Json helpRequestJson(const HelpRequest& request, const User* viewer);
    Json bookingJson(const Booking& booking, const User& viewer);
};

class NotificationController : public Controller {
public:
    using Controller::Controller;
    void registerRoutes(HttpServer& server) override;

private:
    void list(const HttpRequest& req, HttpResponse& res);
    void markAllRead(const HttpRequest& req, HttpResponse& res);
    void clear(const HttpRequest& req, HttpResponse& res);
};

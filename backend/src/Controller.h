#pragma once

#include <string>

#include "AuthService.h"
#include "Database.h"
#include "HttpServer.h"

// Base class for the API controllers. Each subclass owns one feature area
// and registers its own routes on the server.
class Controller {
public:
    Controller(Database& db, AuthService& auth) : db_(db), auth_(auth) {}
    virtual ~Controller() {}

    virtual void registerRoutes(HttpServer& server) = 0;

protected:
    Database& db_;
    AuthService& auth_;

    User& findUser(int userId);  // throws HttpError(404)
    std::string userName(int userId);
    std::string userRole(int userId);

    // Public profile as the frontend expects it. Email and roll number are
    // only included when viewers look at their own profile.
    Json userJson(const User& user, const User* viewer) const;

    // Reads a required, trimmed text field and enforces a maximum length.
    static std::string requireText(const Json& body, const std::string& key,
                                   const std::string& label, std::size_t maxLength);
    static std::string optionalText(const Json& body, const std::string& key,
                                    const std::string& fallback, std::size_t maxLength);
};

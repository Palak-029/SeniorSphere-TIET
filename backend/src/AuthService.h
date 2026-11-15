#pragma once

#include <map>
#include <string>

#include "Database.h"
#include "HttpServer.h"

// Counts failed logins per (email, IP address) pair. Locking only that pair
// means a stranger guessing passwords cannot lock the real owner out.
class LoginLimiter {
public:
    LoginLimiter(int maxFailures, int lockSeconds);

    int secondsLocked(const std::string& email, const std::string& ip) const;
    void recordFailure(const std::string& email, const std::string& ip);
    void clear(const std::string& email, const std::string& ip);

private:
    struct Record {
        int failures = 0;
        long long lockedUntil = 0;
    };

    int maxFailures_;
    int lockSeconds_;
    std::map<std::string, Record> records_;

    static std::string key(const std::string& email, const std::string& ip) { return email + "|" + ip; }
};

struct Session {
    std::string token;
    User* user;
};

class AuthService {
public:
    explicit AuthService(Database& db);

    Session signup(const Json& body);
    Session login(const std::string& email, const std::string& password, const std::string& ip);
    void logout(const std::string& token);

    User* userFromRequest(const HttpRequest& request);  // nullptr when not signed in
    User& requireUser(const HttpRequest& request);      // throws HttpError(401)

    static std::string tokenFromRequest(const HttpRequest& request);
    static std::string clientIp(const HttpRequest& request);

private:
    Database& db_;
    LoginLimiter limiter_;

    Session createSession(User& user);
};

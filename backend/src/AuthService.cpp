#include "AuthService.h"

#include "Utils.h"

namespace {

const char* const kBranches[] = {"COPC", "ENC", "COE", "ECE", "CSBS", "EEC",
                                 "ELE",  "MEE", "CIE", "CHE", "BT",   "DSAI"};

bool isKnownBranch(const std::string& code) {
    for (std::size_t i = 0; i < sizeof(kBranches) / sizeof(kBranches[0]); ++i) {
        if (code == kBranches[i]) return true;
    }
    return false;
}

}  // namespace

// ---------------------------------------------------------------- LoginLimiter

LoginLimiter::LoginLimiter(int maxFailures, int lockSeconds)
    : maxFailures_(maxFailures), lockSeconds_(lockSeconds) {}

int LoginLimiter::secondsLocked(const std::string& email, const std::string& ip) const {
    std::map<std::string, Record>::const_iterator it = records_.find(key(email, ip));
    if (it == records_.end()) return 0;
    long long remaining = it->second.lockedUntil - utils::now();
    return remaining > 0 ? static_cast<int>(remaining) : 0;
}

void LoginLimiter::recordFailure(const std::string& email, const std::string& ip) {
    Record& record = records_[key(email, ip)];
    if (record.lockedUntil != 0 && record.lockedUntil <= utils::now()) record = Record();
    if (++record.failures >= maxFailures_) {
        record.lockedUntil = utils::now() + lockSeconds_;
        record.failures = 0;
    }
}

void LoginLimiter::clear(const std::string& email, const std::string& ip) {
    records_.erase(key(email, ip));
}

// ---------------------------------------------------------------- AuthService

AuthService::AuthService(Database& db) : db_(db), limiter_(5, 5 * 60) {}

Session AuthService::signup(const Json& body) {
    std::string name = utils::trim(body["name"].asString());
    std::string roll = utils::trim(body["rollNumber"].asString());
    std::string email = utils::toLower(utils::trim(body["email"].asString()));
    std::string password = body["password"].asString();
    std::string branch = body["branch"].asString();

    int year = static_cast<int>(body["yearNumber"].asInt(0));
    std::string yearText = body["year"].asString();
    if (year == 0 && !yearText.empty()) year = yearText[0] - '0';

    if (!utils::isLettersAndSpaces(name)) throw HttpError(400, "Name can only contain alphabetic letters and spaces");
    if (roll.size() != 10 || !utils::isDigits(roll)) throw HttpError(400, "Roll number must be exactly 10 digits long");
    if (!utils::endsWith(email, "@thapar.edu") || email.size() <= 11) {
        throw HttpError(400, "Please use your official Thapar email ID");
    }
    if (password.size() < 6) throw HttpError(400, "Password must be at least 6 characters");
    if (year < 1 || year > 4) throw HttpError(400, "Year must be between 1st and 4th");
    if (!isKnownBranch(branch)) throw HttpError(400, "Unknown branch");
    if (db_.findUserByEmail(email)) throw HttpError(409, "An account with this email already exists");
    if (db_.findUserByRoll(roll)) throw HttpError(409, "An account with this roll number already exists");

    if (password.size() > 128) throw HttpError(400, "Password is too long");
    User user(name, email, roll, year, branch, password);
    user.hostel = (branch == "COE" || branch == "CSBS" || branch == "COPC") ? "Hostel PG" : "Hostel N";
    User& saved = db_.users.add(user);
    db_.notify(saved.id(), "Welcome to SeniorSphere!",
               "Your account is ready. Ask a doubt or message a senior to get started.", "academic", "home");
    db_.save();
    return createSession(saved);
}

Session AuthService::login(const std::string& rawEmail, const std::string& password, const std::string& ip) {
    std::string email = utils::toLower(utils::trim(rawEmail));

    int locked = limiter_.secondsLocked(email, ip);
    if (locked > 0) {
        int minutes = (locked + 59) / 60;
        throw HttpError(429, "Too many failed attempts. Try again in " + std::to_string(minutes) + " minute(s).");
    }

    User* user = db_.findUserByEmail(email);
    if (!user || !user->checkPassword(password)) {
        limiter_.recordFailure(email, ip);
        throw HttpError(401, "Incorrect email or password");
    }
    limiter_.clear(email, ip);
    return createSession(*user);
}

void AuthService::logout(const std::string& token) {
    sessions_.erase(token);
}

Session AuthService::createSession(User& user) {
    Session session;
    session.token = utils::randomHex(32);
    session.user = &user;
    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (it->second.expiresAt <= utils::now()) it = sessions_.erase(it); else ++it;
    }
    sessions_[session.token] = {user.id(), utils::now() + 24 * 60 * 60};
    user.lastSeen = utils::now();
    return session;
}

std::string AuthService::tokenFromRequest(const HttpRequest& request) {
    std::string header = request.header("Authorization");
    const std::string prefix = "Bearer ";
    if (header.compare(0, prefix.size(), prefix) != 0) return "";
    return header.substr(prefix.size());
}

// Behind the local dev proxy every request comes from 127.0.0.1, so the proxy's
// X-Forwarded-For header is used instead. It is only trusted from localhost,
// otherwise anyone could fake a new IP on every attempt.
std::string AuthService::clientIp(const HttpRequest& request) {
    std::string forwarded = request.header("X-Forwarded-For");
    if (request.clientIp == "127.0.0.1" && !forwarded.empty()) {
        return utils::trim(forwarded.substr(0, forwarded.find(',')));
    }
    return request.clientIp;
}

User* AuthService::userFromRequest(const HttpRequest& request) {
    auto it = sessions_.find(tokenFromRequest(request));
    if (it == sessions_.end()) return nullptr;
    if (it->second.expiresAt <= utils::now()) { sessions_.erase(it); return nullptr; }
    User* user = db_.users.find(it->second.userId);
    if (user) user->lastSeen = utils::now();
    return user;
}

User& AuthService::requireUser(const HttpRequest& request) {
    User* user = userFromRequest(request);
    if (!user) throw HttpError(401, "Please sign in first");
    return *user;
}

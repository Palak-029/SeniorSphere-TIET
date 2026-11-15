#include "Database.h"

#include <cstdio>
#include <filesystem>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "Utils.h"

Database::Database(const std::string& dataFile, const std::string& seedFile)
    : dataFile_(dataFile), seedFile_(seedFile) {}

bool Database::readFile(const std::string& path, Json& out) const {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file) {
        if (std::filesystem::exists(path)) throw std::runtime_error("Cannot read existing database");
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    out = Json::parse(buffer.str());
    return true;
}

void Database::loadFrom(const Json& data) {
    sessions.clear();
    for (const auto& item : data["sessions"].items()) {
        if (item["expiresAt"].asInt() > utils::now())
            sessions[item["tokenHash"].asString()] = {static_cast<int>(item["userId"].asInt()), item["expiresAt"].asInt()};
    }
    users.fromJson(data["users"]);
    questions.fromJson(data["questions"]);
    conversations.fromJson(data["conversations"]);
    messages.fromJson(data["messages"]);
    slots.fromJson(data["slots"]);
    helpRequests.fromJson(data["helpRequests"]);
    bookings.fromJson(data["bookings"]);
    notifications.fromJson(data["notifications"]);
}

// In the seed file a createdAt of -120 means "120 minutes before first start".
void Database::convertSeedTimes() {
    long long now = utils::now();
    for (std::size_t i = 0; i < questions.all().size(); ++i) {
        Question& q = questions.all()[i];
        if (q.createdAt <= 0) q.createdAt = now + q.createdAt * 60;
        for (std::size_t k = 0; k < q.answers.size(); ++k) {
            if (q.answers[k].createdAt <= 0) q.answers[k].createdAt = now + q.answers[k].createdAt * 60;
        }
    }
    for (std::size_t i = 0; i < slots.all().size(); ++i) {
        if (slots.all()[i].createdAt <= 0) slots.all()[i].createdAt = now + slots.all()[i].createdAt * 60;
    }
    for (std::size_t i = 0; i < helpRequests.all().size(); ++i) {
        HelpRequest& r = helpRequests.all()[i];
        if (r.createdAt <= 0) r.createdAt = now + r.createdAt * 60;
    }
}

void Database::load() {
    Json data;
    if (readFile(dataFile_, data)) {
        loadFrom(data);
        std::cout << "Loaded " << users.all().size() << " users from " << dataFile_ << '\n';
        return;
    }
    std::filesystem::create_directories(std::filesystem::path(dataFile_).parent_path());
    if (std::getenv("SEED_DEMO") && std::string(std::getenv("SEED_DEMO")) == "1") {
        if (!readFile(seedFile_, data)) throw std::runtime_error("Cannot open demo seed");
    } else { data = Json::object(); }

    loadFrom(data);
    convertSeedTimes();
    save();
    std::cout << "First start: created " << dataFile_ << " from " << seedFile_ << '\n';
}

void Database::save() const {
    Json data = Json::object();
    data["sessions"] = Json::array();
    for (const auto& entry : sessions) {
        if (entry.second.expiresAt <= utils::now()) continue;
        Json item = Json::object();
        item["tokenHash"] = entry.first;
        item["userId"] = entry.second.userId;
        item["expiresAt"] = entry.second.expiresAt;
        data["sessions"].push(item);
    }
    data["users"] = users.toJson();
    data["questions"] = questions.toJson();
    data["conversations"] = conversations.toJson();
    data["messages"] = messages.toJson();
    data["slots"] = slots.toJson();
    data["helpRequests"] = helpRequests.toJson();
    data["bookings"] = bookings.toJson();
    data["notifications"] = notifications.toJson();

    // Write to a temporary file first so a crash never leaves a half-written database.
    std::string tempFile = dataFile_ + ".tmp";
    {
        std::ofstream out(tempFile.c_str(), std::ios::binary | std::ios::trunc);
        if (!out) throw std::runtime_error("Cannot write " + tempFile);
        out << data.dump(true);
        out.flush();
        if (!out) throw std::runtime_error("Database write failed");
        out.close();
        if (!out) throw std::runtime_error("Database close failed");
    }
#ifdef _WIN32
    if (!MoveFileExA(tempFile.c_str(), dataFile_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot replace database; original is preserved");
#else
    int fd = ::open(tempFile.c_str(), O_RDONLY);
    if (fd < 0) throw std::runtime_error("Cannot open temporary database");
    int synced = ::fsync(fd);
    ::close(fd);
    if (synced != 0 || std::rename(tempFile.c_str(), dataFile_.c_str()) != 0)
        throw std::runtime_error("Cannot replace database; original is preserved");
    int directory = ::open(std::filesystem::path(dataFile_).parent_path().c_str(), O_RDONLY | O_DIRECTORY);
    if (directory >= 0) { ::fsync(directory); ::close(directory); }
#endif
}

User* Database::findUserByEmail(const std::string& email) {
    std::string wanted = utils::toLower(email);
    for (std::size_t i = 0; i < users.all().size(); ++i) {
        if (users.all()[i].email() == wanted) return &users.all()[i];
    }
    return nullptr;
}

User* Database::findUserByRoll(const std::string& rollNumber) {
    for (std::size_t i = 0; i < users.all().size(); ++i) {
        if (users.all()[i].rollNumber() == rollNumber) return &users.all()[i];
    }
    return nullptr;
}

void Database::notify(int userId, const std::string& title, const std::string& message,
                      const std::string& type, const std::string& tab) {
    Notification n;
    n.userId = userId;
    n.title = title;
    n.message = message;
    n.type = type;
    n.tab = tab;
    n.createdAt = utils::now();
    notifications.add(n);
}

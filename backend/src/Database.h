#pragma once

#include <string>

#include "Chat.h"
#include "Mentorship.h"
#include "Notification.h"
#include "Question.h"
#include "Repository.h"
#include "User.h"

// Holds every collection in memory and persists them to one JSON file.
// save() is called after every change, so data survives server restarts.
class Database {
public:
    Database(const std::string& dataFile, const std::string& seedFile);

    void load();
    void save() const;

    Repository<User> users;
    Repository<Question> questions;
    Repository<Conversation> conversations;
    Repository<Message> messages;
    Repository<OpenSlot> slots;
    Repository<HelpRequest> helpRequests;
    Repository<Booking> bookings;
    Repository<Notification> notifications;

    User* findUserByEmail(const std::string& email);
    User* findUserByRoll(const std::string& rollNumber);

    void notify(int userId, const std::string& title, const std::string& message,
                const std::string& type, const std::string& tab);

private:
    std::string dataFile_;
    std::string seedFile_;

    bool readFile(const std::string& path, Json& out) const;
    void loadFrom(const Json& data);
    void convertSeedTimes();
};

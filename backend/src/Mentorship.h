#pragma once

#include <string>

#include "Entity.h"

// A time slot a senior publishes; exactly one junior can claim it.
class OpenSlot : public Entity {
public:
    int seniorId = 0;
    std::string courseCode;
    std::string courseName;
    std::string topic;
    std::string date;
    std::string time;
    std::string meetingType;  // "In-Person" or "Virtual"
    std::string meetLink;
    Json venue;               // {location, room} or null for virtual slots
    int claimedById = 0;      // 0 while the slot is still available
    long long createdAt = 0;

    bool isAvailable() const { return claimedById == 0; }

    Json toJson() const override;
    void fromJson(const Json& data) override;
};

// A junior asking for 1:1 help; one senior can accept it.
class HelpRequest : public Entity {
public:
    int juniorId = 0;
    std::string subject;
    std::string title;
    std::string description;
    std::string preferredMode;
    std::string preferredTime;
    std::string urgency;
    int acceptedById = 0;  // 0 while the request is open
    long long createdAt = 0;

    bool isOpen() const { return acceptedById == 0; }

    Json toJson() const override;
    void fromJson(const Json& data) override;
};

// A confirmed 1:1 mentorship session between a junior and a senior.
class Booking : public Entity {
public:
    int juniorId = 0;
    int seniorId = 0;
    std::string juniorAlias;  // set when the junior asked anonymously; the senior sees this instead
    std::string topic;
    std::string mode;
    std::string date;
    std::string time;
    std::string note;
    std::string status = "Confirmed";
    Json venue;  // {location, room, campusZone, landmark, instructions}
    long long createdAt = 0;

    Json toJson() const override;
    void fromJson(const Json& data) override;
};

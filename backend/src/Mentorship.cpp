#include "Mentorship.h"

Json OpenSlot::toJson() const {
    Json j = Json::object();
    j["id"] = id_;
    j["seniorId"] = seniorId;
    j["courseCode"] = courseCode;
    j["courseName"] = courseName;
    j["topic"] = topic;
    j["date"] = date;
    j["time"] = time;
    j["meetingType"] = meetingType;
    j["meetLink"] = meetLink;
    j["venue"] = venue;
    j["claimedById"] = claimedById;
    j["createdAt"] = createdAt;
    return j;
}

void OpenSlot::fromJson(const Json& j) {
    id_ = static_cast<int>(j["id"].asInt());
    seniorId = static_cast<int>(j["seniorId"].asInt());
    courseCode = j["courseCode"].asString();
    courseName = j["courseName"].asString();
    topic = j["topic"].asString();
    date = j["date"].asString();
    time = j["time"].asString();
    meetingType = j["meetingType"].asString("In-Person");
    meetLink = j["meetLink"].asString();
    venue = j["venue"];
    claimedById = static_cast<int>(j["claimedById"].asInt());
    createdAt = j["createdAt"].asInt();
}

Json HelpRequest::toJson() const {
    Json j = Json::object();
    j["id"] = id_;
    j["juniorId"] = juniorId;
    j["subject"] = subject;
    j["title"] = title;
    j["description"] = description;
    j["preferredMode"] = preferredMode;
    j["preferredTime"] = preferredTime;
    j["urgency"] = urgency;
    j["acceptedById"] = acceptedById;
    j["createdAt"] = createdAt;
    return j;
}

void HelpRequest::fromJson(const Json& j) {
    id_ = static_cast<int>(j["id"].asInt());
    juniorId = static_cast<int>(j["juniorId"].asInt());
    subject = j["subject"].asString();
    title = j["title"].asString();
    description = j["description"].asString();
    preferredMode = j["preferredMode"].asString();
    preferredTime = j["preferredTime"].asString();
    urgency = j["urgency"].asString();
    acceptedById = static_cast<int>(j["acceptedById"].asInt());
    createdAt = j["createdAt"].asInt();
}

Json Booking::toJson() const {
    Json j = Json::object();
    j["id"] = id_;
    j["juniorId"] = juniorId;
    j["seniorId"] = seniorId;
    j["juniorAlias"] = juniorAlias;
    j["topic"] = topic;
    j["mode"] = mode;
    j["date"] = date;
    j["time"] = time;
    j["note"] = note;
    j["status"] = status;
    j["venue"] = venue;
    j["createdAt"] = createdAt;
    return j;
}

void Booking::fromJson(const Json& j) {
    id_ = static_cast<int>(j["id"].asInt());
    juniorId = static_cast<int>(j["juniorId"].asInt());
    seniorId = static_cast<int>(j["seniorId"].asInt());
    juniorAlias = j["juniorAlias"].asString();
    topic = j["topic"].asString();
    mode = j["mode"].asString();
    date = j["date"].asString();
    time = j["time"].asString();
    note = j["note"].asString();
    status = j["status"].asString("Confirmed");
    venue = j["venue"];
    createdAt = j["createdAt"].asInt();
}

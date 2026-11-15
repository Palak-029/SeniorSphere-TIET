#include <algorithm>

#include "Controllers.h"
#include "Utils.h"

void MentorshipController::registerRoutes(HttpServer& server) {
    server.get("/api/slots", [this](const HttpRequest& req, HttpResponse& res) { listSlots(req, res); });
    server.post("/api/slots", [this](const HttpRequest& req, HttpResponse& res) { createSlot(req, res); });
    server.post("/api/slots/:id/claim", [this](const HttpRequest& req, HttpResponse& res) { claimSlot(req, res); });
    server.get("/api/help-requests", [this](const HttpRequest& req, HttpResponse& res) { listHelpRequests(req, res); });
    server.post("/api/help-requests", [this](const HttpRequest& req, HttpResponse& res) { createHelpRequest(req, res); });
    server.post("/api/help-requests/:id/accept", [this](const HttpRequest& req, HttpResponse& res) { acceptHelpRequest(req, res); });
    server.get("/api/bookings", [this](const HttpRequest& req, HttpResponse& res) { listBookings(req, res); });
    server.post("/api/bookings", [this](const HttpRequest& req, HttpResponse& res) { createBooking(req, res); });
}

void MentorshipController::requireSenior(const User& user, const std::string& action) {
    if (!user.isSenior()) throw HttpError(403, "Only 3rd and 4th year students can " + action);
}

// Keeps only the known venue text fields so clients cannot store arbitrary data.
Json MentorshipController::cleanVenue(const Json& venue) {
    const char* const fields[] = {"location", "room", "campusZone", "landmark", "instructions"};
    Json clean = Json::object();
    for (std::size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        std::string value = utils::trim(venue[fields[i]].asString());
        if (value.size() > 200) value = value.substr(0, 200);
        clean[fields[i]] = value;
    }
    return clean;
}

// ---------------------------------------------------------------- open slots

Json MentorshipController::slotJson(const OpenSlot& slot, const User* viewer) {
    User* senior = db_.users.find(slot.seniorId);
    bool involved = viewer && (viewer->id() == slot.seniorId || viewer->id() == slot.claimedById);

    Json j = Json::object();
    j["id"] = slot.id();
    j["seniorId"] = slot.seniorId;
    j["seniorName"] = userName(slot.seniorId);
    j["seniorRole"] = userRole(slot.seniorId);
    j["seniorAvatar"] = senior ? senior->avatarUrl() : "";
    j["courseCode"] = slot.courseCode;
    j["courseName"] = slot.courseName;
    j["topic"] = slot.topic;
    j["date"] = slot.date;
    j["time"] = slot.time;
    j["meetingType"] = slot.meetingType;
    j["venue"] = slot.venue;
    j["meetLink"] = involved ? slot.meetLink : "";
    j["status"] = slot.isAvailable() ? "Available" : "Claimed";
    j["isMine"] = viewer != nullptr && viewer->id() == slot.seniorId;
    if (slot.isAvailable()) {
        j["claimedBy"] = Json();
    } else {
        User* claimer = db_.users.find(slot.claimedById);
        Json by = Json::object();
        by["name"] = userName(slot.claimedById);
        by["year"] = claimer ? claimer->yearLabel() : "";
        by["branch"] = claimer ? claimer->branch() : "";
        j["claimedBy"] = by;
    }
    return j;
}

void MentorshipController::listSlots(const HttpRequest& req, HttpResponse& res) {
    User* viewer = auth_.userFromRequest(req);
    Json list = Json::array();
    for (std::size_t i = db_.slots.all().size(); i-- > 0;) list.push(slotJson(db_.slots.all()[i], viewer));
    res.send(list);
}

void MentorshipController::createSlot(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    requireSenior(me, "post mentorship slots");
    Json body = req.json();

    OpenSlot slot;
    slot.seniorId = me.id();
    slot.courseCode = requireText(body, "courseCode", "Course code", 20);
    slot.courseName = optionalText(body, "courseName", "Core Engineering", 100);
    slot.topic = requireText(body, "topic", "Topic", 200);
    slot.date = requireText(body, "date", "Date", 60);
    slot.time = requireText(body, "time", "Time", 60);
    slot.meetingType = body["meetingType"].asString() == "Virtual" ? "Virtual" : "In-Person";
    if (slot.meetingType == "Virtual") {
        slot.meetLink = optionalText(body, "meetLink", "", 200);
    } else {
        Json venue = Json::object();
        venue["location"] = requireText(body["venue"], "location", "Venue", 120);
        venue["room"] = optionalText(body["venue"], "room", "Discussion Area", 120);
        slot.venue = venue;
    }
    slot.createdAt = utils::now();

    OpenSlot& saved = db_.slots.add(slot);
    db_.save();
    res.send(slotJson(saved, &me), 201);
}

void MentorshipController::claimSlot(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    OpenSlot* slot = db_.slots.find(req.paramId("id"));
    if (!slot) throw HttpError(404, "Slot not found");
    if (slot->seniorId == me.id()) throw HttpError(400, "You cannot claim your own slot");
    if (!slot->isAvailable()) throw HttpError(409, "This slot was already claimed by another student");

    slot->claimedById = me.id();

    Booking booking;
    booking.juniorId = me.id();
    booking.seniorId = slot->seniorId;
    booking.topic = slot->courseCode + ": " + slot->topic;
    booking.date = slot->date;
    booking.time = slot->time;
    booking.createdAt = utils::now();
    Json venue = Json::object();
    if (slot->meetingType == "Virtual") {
        std::string link = slot->meetLink.empty() ? "Meet link will be shared in chat" : slot->meetLink;
        booking.mode = "Google Meet (Virtual)";
        venue["location"] = "Google Meet (Virtual Campus Room)";
        venue["room"] = link;
        venue["campusZone"] = "SeniorSphere Video Bridge";
        venue["landmark"] = "Virtual 1:1 Senior Lounge";
        venue["instructions"] = "Join 2 minutes before the slot with your questions and notes ready.";
    } else {
        booking.mode = "In-Person (" + slot->venue["location"].asString() + ")";
        venue = cleanVenue(slot->venue);
        venue["campusZone"] = "TIET Patiala Campus";
        venue["instructions"] = "Arrive 5 minutes early. Silence your phone.";
    }
    booking.venue = venue;
    const Booking& saved = db_.bookings.add(booking);

    db_.notify(me.id(), "1:1 Mentorship Confirmed",
               "Your session on " + slot->courseCode + " with " + userName(slot->seniorId) + " is locked in for " +
                   slot->date + " at " + slot->time + ".",
               "mentorship", "mentorship");
    db_.notify(slot->seniorId, "Your Slot Was Claimed",
               me.name + " (" + me.yearLabel() + " \u2022 " + me.branch() + ") claimed your " + slot->courseCode +
                   " slot on " + slot->date + ".",
               "mentorship", "mentorship");
    db_.save();

    Json j = Json::object();
    j["slot"] = slotJson(*slot, &me);
    j["booking"] = bookingJson(saved, me);
    res.send(j);
}

// ---------------------------------------------------------------- help requests

Json MentorshipController::helpRequestJson(const HelpRequest& r, const User* viewer) {
    User* junior = db_.users.find(r.juniorId);
    Json j = Json::object();
    j["id"] = r.id();
    j["juniorName"] = userName(r.juniorId);
    j["juniorYear"] = junior ? junior->yearLabel() : "";
    j["juniorBranch"] = junior ? junior->branch() : "";
    j["subject"] = r.subject;
    j["title"] = r.title;
    j["description"] = r.description;
    j["preferredMode"] = r.preferredMode;
    j["preferredTime"] = r.preferredTime;
    j["urgency"] = r.urgency;
    j["status"] = r.isOpen() ? "Open" : "Claimed";
    j["timestamp"] = utils::timeAgo(r.createdAt);
    j["isMine"] = viewer != nullptr && viewer->id() == r.juniorId;
    if (r.isOpen()) {
        j["claimedBySenior"] = Json();
    } else {
        User* senior = db_.users.find(r.acceptedById);
        j["claimedBySenior"] = senior ? senior->name + " (" + senior->yearLabel() + " \u2022 " + senior->branch() + ")"
                                      : "Senior Mentor";
    }
    return j;
}

void MentorshipController::listHelpRequests(const HttpRequest& req, HttpResponse& res) {
    User* viewer = auth_.userFromRequest(req);
    Json list = Json::array();
    for (std::size_t i = db_.helpRequests.all().size(); i-- > 0;) {
        list.push(helpRequestJson(db_.helpRequests.all()[i], viewer));
    }
    res.send(list);
}

void MentorshipController::createHelpRequest(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    Json body = req.json();

    HelpRequest r;
    r.juniorId = me.id();
    r.subject = requireText(body, "subject", "Subject", 20);
    r.title = requireText(body, "title", "Title", 200);
    r.description = requireText(body, "description", "Description", 3000);
    r.preferredMode = optionalText(body, "preferredMode", "In-Person", 80);
    r.preferredTime = optionalText(body, "preferredTime", "This Week", 80);
    r.urgency = optionalText(body, "urgency", "Normal", 60);
    r.createdAt = utils::now();

    HelpRequest& saved = db_.helpRequests.add(r);
    db_.save();
    res.send(helpRequestJson(saved, &me), 201);
}

void MentorshipController::acceptHelpRequest(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    requireSenior(me, "accept help requests");
    HelpRequest* r = db_.helpRequests.find(req.paramId("id"));
    if (!r) throw HttpError(404, "Help request not found");
    if (r->juniorId == me.id()) throw HttpError(400, "You cannot accept your own request");
    if (!r->isOpen()) throw HttpError(409, "Another senior already accepted this request");

    r->acceptedById = me.id();
    me.addKarma(20);

    bool isVirtual = utils::toLower(r->preferredMode).find("virtual") != std::string::npos;
    Booking booking;
    booking.juniorId = r->juniorId;
    booking.seniorId = me.id();
    booking.topic = r->subject + ": " + r->title;
    booking.mode = isVirtual ? "Google Meet (Virtual)" : "In-Person (Central Library TAN Wing)";
    booking.date = "To be fixed in chat";
    booking.time = r->preferredTime;
    booking.createdAt = utils::now();
    Json venue = Json::object();
    venue["location"] = isVirtual ? "Google Meet (Virtual Room)" : "Central Library (Nava Nalanda / TAN Wing)";
    venue["room"] = isVirtual ? "Link will be shared in chat" : "Discussion Room 3, 2nd Floor";
    venue["campusZone"] = "TIET Patiala Campus";
    venue["landmark"] = "Mentorship Guild Station";
    venue["instructions"] = "Message each other on SeniorSphere to confirm the exact time.";
    booking.venue = venue;
    const Booking& saved = db_.bookings.add(booking);

    db_.notify(r->juniorId, "Help Request Accepted",
               me.name + " (" + me.roleLabel() + ") accepted your request on " + r->subject +
                   ". Message them to fix the time.",
               "mentorship", "mentorship");
    db_.save();

    Json j = Json::object();
    j["request"] = helpRequestJson(*r, &me);
    j["booking"] = bookingJson(saved, me);
    res.send(j);
}

// ---------------------------------------------------------------- bookings

Json MentorshipController::bookingJson(const Booking& b, const User& viewer) {
    bool viewerIsSenior = viewer.id() == b.seniorId;
    std::string juniorName = viewerIsSenior && !b.juniorAlias.empty() ? b.juniorAlias : userName(b.juniorId);

    Json j = Json::object();
    j["id"] = b.id();
    j["seniorName"] = userName(b.seniorId);
    j["seniorRole"] = userRole(b.seniorId);
    j["juniorName"] = juniorName;
    j["myRole"] = viewerIsSenior ? "mentor" : "mentee";
    j["withUserId"] = viewerIsSenior && !b.juniorAlias.empty() ? Json() : Json(viewerIsSenior ? b.juniorId : b.seniorId);
    j["topic"] = b.topic;
    j["mode"] = b.mode;
    j["date"] = b.date;
    j["time"] = b.time;
    j["note"] = b.note;
    j["status"] = b.status;
    j["venue"] = b.venue;
    return j;
}

void MentorshipController::listBookings(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    Json list = Json::array();
    for (std::size_t i = db_.bookings.all().size(); i-- > 0;) {
        const Booking& b = db_.bookings.all()[i];
        if (b.juniorId == me.id() || b.seniorId == me.id()) list.push(bookingJson(b, me));
    }
    res.send(list);
}

// Two ways to book: a junior books a senior from the directory ("seniorId"),
// or a senior offers a session on someone's forum doubt ("questionId").
void MentorshipController::createBooking(const HttpRequest& req, HttpResponse& res) {
    User& me = auth_.requireUser(req);
    Json body = req.json();

    Booking booking;
    booking.topic = requireText(body, "topic", "Topic", 300);
    booking.mode = optionalText(body, "mode", "In-Person", 150);
    booking.date = optionalText(body, "date", "To be fixed in chat", 100);
    booking.time = optionalText(body, "time", "Mutually agreed time", 100);
    booking.note = optionalText(body, "note", "", 1000);
    booking.venue = cleanVenue(body["venue"]);
    booking.createdAt = utils::now();

    int notifyUserId = 0;
    std::string title;
    std::string message;
    if (body.has("questionId")) {
        requireSenior(me, "offer 1:1 sessions");
        Question* q = db_.questions.find(static_cast<int>(body["questionId"].asInt()));
        if (!q) throw HttpError(404, "Question not found");
        if (q->authorId == me.id()) throw HttpError(400, "You cannot offer a session on your own doubt");
        if (q->isAnonymous) throw HttpError(403, "For anonymous doubts, reply publicly and let the author contact you privately");
        booking.juniorId = q->authorId;
        booking.seniorId = me.id();
        if (q->isAnonymous) booking.juniorAlias = q->anonymousName;
        notifyUserId = q->authorId;
        title = "1:1 Session Offered";
        message = me.name + " (" + me.roleLabel() + ") scheduled a 1:1 session on your doubt \"" + q->title + "\".";
    } else {
        User& senior = findUser(static_cast<int>(body["seniorId"].asInt()));
        if (senior.id() == me.id()) throw HttpError(400, "You cannot book a session with yourself");
        bool higherYear = me.yearNumber() >= 4 ? senior.yearNumber() >= 4 : senior.yearNumber() > me.yearNumber();
        if (!higherYear) throw HttpError(403, "You can only book sessions with students in a higher year");
        booking.juniorId = me.id();
        booking.seniorId = senior.id();
        notifyUserId = senior.id();
        title = "New 1:1 Session Request";
        message = me.name + " (" + me.yearLabel() + " \u2022 " + me.branch() + ") booked a session with you: " +
                  booking.topic + " (" + booking.date + ").";
    }

    const Booking& saved = db_.bookings.add(booking);
    db_.notify(notifyUserId, title, message, "mentorship", "mentorship");
    db_.save();
    res.send(bookingJson(saved, me), 201);
}

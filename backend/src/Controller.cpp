#include "Controller.h"

#include "Utils.h"

User& Controller::findUser(int userId) {
    User* user = db_.users.find(userId);
    if (!user) throw HttpError(404, "Student not found");
    return *user;
}

std::string Controller::userName(int userId) {
    User* user = db_.users.find(userId);
    return user ? user->name : "Former student";
}

std::string Controller::userRole(int userId) {
    User* user = db_.users.find(userId);
    return user ? user->roleLabel() : "";
}

Json Controller::userJson(const User& user, const User* viewer) const {
    bool isSelf = viewer && viewer->id() == user.id();
    Json j = Json::object();
    j["id"] = user.id();
    j["name"] = user.name;
    j["year"] = user.yearLabel();
    j["yearNumber"] = user.yearNumber();
    j["branch"] = user.branch();
    j["batch"] = user.batch();
    j["hostel"] = user.hostel;
    j["cgpa"] = user.cgpa;
    j["karma"] = user.karma();
    j["avatar"] = user.avatarUrl();
    j["bio"] = user.bio;
    j["company"] = user.company;
    j["achievement"] = user.achievement;
    j["expertise"] = toJsonList(user.expertise);
    j["societies"] = toJsonList(user.societies);
    j["deansList"] = user.deansList;
    j["verified"] = user.verified;
    j["rating"] = user.rating;
    j["reviewsCount"] = user.reviewsCount;
    j["slotsAvailable"] = user.slotsAvailable;
    j["status"] = utils::now() - user.lastSeen < 60 ? "Online" : "Away";
    j["connected"] = viewer ? viewer->isConnectedTo(user.id()) : false;
    if (isSelf) {
        j["email"] = user.email();
        j["rollNumber"] = user.rollNumber();
    }
    return j;
}

std::string Controller::requireText(const Json& body, const std::string& key,
                                    const std::string& label, std::size_t maxLength) {
    std::string value = utils::trim(body[key].asString());
    if (value.empty()) throw HttpError(400, label + " is required");
    if (value.size() > maxLength) {
        throw HttpError(400, label + " is too long (max " + std::to_string(maxLength) + " characters)");
    }
    return value;
}

std::string Controller::optionalText(const Json& body, const std::string& key,
                                     const std::string& fallback, std::size_t maxLength) {
    std::string value = utils::trim(body[key].asString());
    if (value.empty()) return fallback;
    if (value.size() > maxLength) throw HttpError(400, key + " is too long");
    return value;
}

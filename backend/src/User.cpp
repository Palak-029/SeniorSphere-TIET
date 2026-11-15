#include "User.h"
#include <sodium.h>
#include <stdexcept>

#include "Utils.h"

User::User()
    : deansList(false), verified(false), rating(0), reviewsCount(0), slotsAvailable(3), lastSeen(0),
      yearNumber_(1), karma_(0) {}

User::User(const std::string& fullName, const std::string& email, const std::string& rollNumber,
           int yearNumber, const std::string& branch, const std::string& password)
    : User() {
    name = fullName;
    email_ = email;
    rollNumber_ = rollNumber;
    yearNumber_ = yearNumber;
    branch_ = branch;
    setPassword(password);
}

std::string User::yearLabel() const {
    return utils::yearLabel(yearNumber_);
}

std::string User::batch() const {
    return "Batch of " + std::to_string(2031 - yearNumber_);
}

std::string User::roleLabel() const {
    std::string label = yearLabel() + " \u2022 " + branch_;
    if (!company.empty()) label += " \u00B7 " + company;
    return label;
}

std::string User::avatarUrl() const {
    if (!avatar.empty()) return avatar;
    std::string encoded;
    for (std::size_t i = 0; i < name.size(); ++i) encoded += name[i] == ' ' ? '+' : name[i];
    return "https://ui-avatars.com/api/?background=7f1d1d&color=fcd34d&bold=true&name=" + encoded;
}

// Salted SHA-256, repeated so that guessing passwords from a stolen data file is slow.
std::string User::hashPassword(const std::string& password) const {
    std::string hash = utils::sha256Hex(passwordSalt_ + password);
    for (int i = 0; i < 5000; ++i) hash = utils::sha256Hex(passwordSalt_ + hash);
    return hash;
}

void User::setPassword(const std::string& password) {
    char encoded[crypto_pwhash_STRBYTES];
    if (crypto_pwhash_str(encoded, password.data(), password.size(),
        crypto_pwhash_OPSLIMIT_INTERACTIVE, crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0)
        throw std::runtime_error("Could not hash password");
    passwordHash_ = encoded;
    passwordSalt_.clear(); // Argon2 stores the salt inside its encoded result.
}
bool User::checkPassword(const std::string& password) const {
    if (passwordHash_.rfind("$argon2", 0) == 0)
        return crypto_pwhash_str_verify(passwordHash_.c_str(), password.data(), password.size()) == 0;
    return !passwordHash_.empty() && hashPassword(password) == passwordHash_; // legacy accounts
}

bool User::toggleConnection(int userId) {
    if (connections_.erase(userId) > 0) return false;
    connections_.insert(userId);
    return true;
}

Json User::toJson() const {
    Json j = Json::object();
    j["id"] = id_;
    j["name"] = name;
    j["email"] = email_;
    j["rollNumber"] = rollNumber_;
    j["branch"] = branch_;
    j["yearNumber"] = yearNumber_;
    j["karma"] = karma_;
    j["passwordSalt"] = passwordSalt_;
    j["passwordHash"] = passwordHash_;
    j["connections"] = idsToJson(connections_);
    j["hostel"] = hostel;
    j["cgpa"] = cgpa;
    j["avatar"] = avatar;
    j["bio"] = bio;
    j["company"] = company;
    j["achievement"] = achievement;
    j["expertise"] = toJsonList(expertise);
    j["societies"] = toJsonList(societies);
    j["deansList"] = deansList;
    j["verified"] = verified;
    j["rating"] = rating;
    j["reviewsCount"] = reviewsCount;
    j["slotsAvailable"] = slotsAvailable;
    return j;
}

void User::fromJson(const Json& j) {
    id_ = static_cast<int>(j["id"].asInt());
    name = j["name"].asString();
    email_ = j["email"].asString();
    rollNumber_ = j["rollNumber"].asString();
    branch_ = j["branch"].asString();
    yearNumber_ = static_cast<int>(j["yearNumber"].asInt(1));
    karma_ = static_cast<int>(j["karma"].asInt());
    connections_ = idsFromJson(j["connections"]);
    hostel = j["hostel"].asString();
    cgpa = j["cgpa"].asString();
    avatar = j["avatar"].asString();
    bio = j["bio"].asString();
    company = j["company"].asString();
    achievement = j["achievement"].asString();
    expertise = j["expertise"].asStringList();
    societies = j["societies"].asStringList();
    deansList = j["deansList"].asBool();
    verified = j["verified"].asBool();
    rating = j["rating"].asDouble();
    reviewsCount = static_cast<int>(j["reviewsCount"].asInt());
    slotsAvailable = static_cast<int>(j["slotsAvailable"].asInt(3));

    // The seed file lists plain demo passwords; they are hashed on first load.
    if (j.has("password")) {
        setPassword(j["password"].asString());
    } else {
        passwordSalt_ = j["passwordSalt"].asString();
        passwordHash_ = j["passwordHash"].asString();
    }
}

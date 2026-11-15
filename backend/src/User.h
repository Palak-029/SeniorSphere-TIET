#pragma once

#include <set>
#include <string>
#include <vector>

#include "Entity.h"

class User : public Entity {
public:
    User();
    User(const std::string& name, const std::string& email, const std::string& rollNumber,
         int yearNumber, const std::string& branch, const std::string& password);

    // Identity fields are fixed at sign-up: there are deliberately no setters,
    // so no API call can change a student's email, roll number, branch or year.
    const std::string& email() const { return email_; }
    const std::string& rollNumber() const { return rollNumber_; }
    const std::string& branch() const { return branch_; }
    int yearNumber() const { return yearNumber_; }
    std::string yearLabel() const;
    std::string batch() const;
    bool isSenior() const { return yearNumber_ >= 3; }
    std::string roleLabel() const;  // "4th Year • COE · Microsoft"
    std::string avatarUrl() const;

    void setPassword(const std::string& password);
    bool checkPassword(const std::string& password) const;

    int karma() const { return karma_; }
    void addKarma(int points) { karma_ += points; }

    bool isConnectedTo(int userId) const { return connections_.count(userId) > 0; }
    bool toggleConnection(int userId);  // returns true if now connected

    // Profile fields the student may edit (PUT /api/me).
    std::string name;
    std::string hostel;
    std::string cgpa;
    std::string avatar;
    std::string bio;
    std::string company;
    std::string achievement;
    std::vector<std::string> expertise;
    std::vector<std::string> societies;

    // Mentor stats shown in the senior directory.
    bool deansList;
    bool verified;
    double rating;
    int reviewsCount;
    int slotsAvailable;

    long long lastSeen;  // in memory only; drives the Online/Away status

    Json toJson() const override;
    void fromJson(const Json& data) override;

private:
    std::string email_;
    std::string rollNumber_;
    std::string branch_;
    int yearNumber_;
    int karma_;
    std::string passwordSalt_;
    std::string passwordHash_;
    std::set<int> connections_;

    std::string hashPassword(const std::string& password) const;
};

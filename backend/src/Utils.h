#pragma once

#include <string>

namespace utils {

long long now();                                   // seconds since epoch
std::string timeAgo(long long timestamp);          // "Just now", "5 mins ago", "Yesterday", ...
std::string clockTime(long long timestamp);        // "14:25" (server local time)

std::string trim(const std::string& s);
std::string toLower(std::string s);
bool endsWith(const std::string& s, const std::string& suffix);
bool isDigits(const std::string& s);
bool isLettersAndSpaces(const std::string& s);
std::string yearLabel(int yearNumber);             // 1 -> "1st Year"

std::string sha256Hex(const std::string& data);
std::string randomHex(int bytes);                  // cryptographically random

}  // namespace utils

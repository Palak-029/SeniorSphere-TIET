#include "Utils.h"

#include <cstdio>
#include <ctime>
#include <fstream>
#include <stdexcept>
#include <sodium.h>

#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

namespace utils {

long long now() {
    return static_cast<long long>(std::time(nullptr));
}

std::string timeAgo(long long timestamp) {
    long long diff = now() - timestamp;
    if (diff < 60) return "Just now";
    if (diff < 3600) {
        long long mins = diff / 60;
        return std::to_string(mins) + (mins == 1 ? " min ago" : " mins ago");
    }
    if (diff < 86400) {
        long long hours = diff / 3600;
        return std::to_string(hours) + (hours == 1 ? " hour ago" : " hours ago");
    }
    long long days = diff / 86400;
    if (days == 1) return "Yesterday";
    return std::to_string(days) + " days ago";
}

std::string clockTime(long long timestamp) {
    std::time_t t = static_cast<std::time_t>(timestamp);
    std::tm* local = std::localtime(&t);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M", local);
    if (now() - timestamp >= 86400) return timeAgo(timestamp);
    return buf;
}

std::string trim(const std::string& s) {
    std::size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    std::size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string toLower(std::string s) {
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] = static_cast<char>(s[i] - 'A' + 'a');
    }
    return s;
}

bool endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool isDigits(const std::string& s) {
    if (s.empty()) return false;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

bool isLettersAndSpaces(const std::string& s) {
    if (s.empty()) return false;
    for (std::size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == ' ';
        if (!ok) return false;
    }
    return true;
}

std::string yearLabel(int yearNumber) {
    switch (yearNumber) {
        case 1: return "1st Year";
        case 2: return "2nd Year";
        case 3: return "3rd Year";
        default: return "4th Year";
    }
}


std::string sha256Hex(const std::string& data) {
    unsigned char digest[crypto_hash_sha256_BYTES];
    crypto_hash_sha256(digest, reinterpret_cast<const unsigned char*>(data.data()), data.size());
    char hex[crypto_hash_sha256_BYTES * 2 + 1];
    sodium_bin2hex(hex, sizeof hex, digest, sizeof digest);
    return hex;
}
std::string randomHex(int bytes) {
    std::string buffer(bytes, '\0');
    randombytes_buf(&buffer[0], buffer.size());
    std::string hex(bytes * 2 + 1, '\0');
    sodium_bin2hex(&hex[0], hex.size(), reinterpret_cast<const unsigned char*>(buffer.data()), buffer.size());
    hex.pop_back();
    return hex;
}
} // namespace utils

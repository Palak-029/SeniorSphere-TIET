#include "Json.h"

#include <cmath>
#include "../third_party/json.hpp"
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

Json::Json() : type_(Type::Null), bool_(false), number_(0) {}
Json::Json(std::nullptr_t) : Json() {}
Json::Json(bool value) : type_(Type::Bool), bool_(value), number_(0) {}
Json::Json(int value) : type_(Type::Number), bool_(false), number_(value) {}
Json::Json(long long value) : type_(Type::Number), bool_(false), number_(static_cast<double>(value)) {}
Json::Json(double value) : type_(Type::Number), bool_(false), number_(value) {}
Json::Json(const char* value) : type_(Type::String), bool_(false), number_(0), string_(value) {}
Json::Json(const std::string& value) : type_(Type::String), bool_(false), number_(0), string_(value) {}

Json Json::array() {
    Json j;
    j.type_ = Type::Array;
    return j;
}

Json Json::object() {
    Json j;
    j.type_ = Type::Object;
    return j;
}

Json& Json::operator[](const std::string& key) {
    if (type_ == Type::Null) type_ = Type::Object;
    if (type_ != Type::Object) throw std::runtime_error("JSON value is not an object");
    return object_[key];
}

const Json& Json::operator[](const std::string& key) const {
    static const Json nullValue;
    if (type_ != Type::Object) return nullValue;
    std::map<std::string, Json>::const_iterator it = object_.find(key);
    return it == object_.end() ? nullValue : it->second;
}

bool Json::has(const std::string& key) const {
    return type_ == Type::Object && object_.count(key) > 0;
}

void Json::push(const Json& value) {
    if (type_ == Type::Null) type_ = Type::Array;
    if (type_ != Type::Array) throw std::runtime_error("JSON value is not an array");
    array_.push_back(value);
}

std::size_t Json::size() const {
    if (type_ == Type::Array) return array_.size();
    if (type_ == Type::Object) return object_.size();
    return 0;
}

std::string Json::asString(const std::string& fallback) const {
    return type_ == Type::String ? string_ : fallback;
}

long long Json::asInt(long long fallback) const {
    if (type_ == Type::Number && std::isfinite(number_) && std::floor(number_) == number_ && number_ >= -9007199254740991.0 && number_ <= 9007199254740991.0) return static_cast<long long>(number_);
    if (type_ == Type::String && !string_.empty()) {
        char* end = nullptr;
        long long value = std::strtoll(string_.c_str(), &end, 10);
        if (*end == '\0') return value;
    }
    return fallback;
}

double Json::asDouble(double fallback) const {
    return type_ == Type::Number ? number_ : fallback;
}

bool Json::asBool(bool fallback) const {
    return type_ == Type::Bool ? bool_ : fallback;
}

std::vector<std::string> Json::asStringList() const {
    std::vector<std::string> result;
    for (std::size_t i = 0; i < array_.size(); ++i) {
        if (array_[i].isString()) result.push_back(array_[i].string_);
    }
    return result;
}

Json toJsonList(const std::vector<std::string>& values) {
    Json list = Json::array();
    for (std::size_t i = 0; i < values.size(); ++i) list.push(values[i]);
    return list;
}


namespace {
Json fromLibrary(const nlohmann::json& value) {
    if (value.is_null()) return Json();
    if (value.is_boolean()) return Json(value.get<bool>());
    if (value.is_number()) return Json(value.get<double>());
    if (value.is_string()) return Json(value.get<std::string>());
    Json result = value.is_array() ? Json::array() : Json::object();
    if (value.is_array()) for (const auto& item : value) result.push(fromLibrary(item));
    else for (auto it = value.begin(); it != value.end(); ++it) result[it.key()] = fromLibrary(it.value());
    return result;
}
nlohmann::json toLibrary(const Json& value) {
    if (value.isNull()) return nullptr;
    if (value.isBool()) return value.asBool();
    if (value.isNumber()) { double n = value.asDouble(); if (std::isfinite(n) && std::floor(n) == n && std::abs(n) <= 9007199254740991.0) return value.asInt(); return n; }
    if (value.isString()) return value.asString();
    auto result = value.isArray() ? nlohmann::json::array() : nlohmann::json::object();
    if (value.isArray()) for (const auto& item : value.items()) result.push_back(toLibrary(item));
    else for (const auto& pair : value.members()) result[pair.first] = toLibrary(pair.second);
    return result;
}
}
Json Json::parse(const std::string& text) {
    try {
        return fromLibrary(nlohmann::json::parse(text, [](int depth, auto, auto&) {
            if (depth > 64) throw std::runtime_error("JSON nesting is too deep");
            return true;
        }));
    } catch (const std::exception&) { throw std::runtime_error("Invalid JSON"); }
}
std::string Json::dump(bool pretty) const { return toLibrary(*this).dump(pretty ? 2 : -1); }

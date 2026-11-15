#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>

// Small application interface backed by nlohmann/json for parsing and writing.
class Json {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Json();
    Json(std::nullptr_t);
    Json(bool value);
    Json(int value);
    Json(long long value);
    Json(double value);
    Json(const char* value);
    Json(const std::string& value);

    static Json array();
    static Json object();
    static Json parse(const std::string& text);  // throws std::runtime_error

    Type type() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isObject() const { return type_ == Type::Object; }
    bool isArray() const { return type_ == Type::Array; }
    bool isString() const { return type_ == Type::String; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isBool() const { return type_ == Type::Bool; }

    // Object access. The non-const version creates the key if it is missing.
    Json& operator[](const std::string& key);
    const Json& operator[](const std::string& key) const;
    bool has(const std::string& key) const;
    const std::map<std::string, Json>& members() const { return object_; }

    // Array access.
    void push(const Json& value);
    std::size_t size() const;
    const std::vector<Json>& items() const { return array_; }

    // Typed reads that fall back to a default when the type does not match.
    std::string asString(const std::string& fallback = "") const;
    long long asInt(long long fallback = 0) const;
    double asDouble(double fallback = 0) const;
    bool asBool(bool fallback = false) const;
    std::vector<std::string> asStringList() const;

    std::string dump(bool pretty = false) const;

private:
    Type type_;
    bool bool_;
    double number_;
    std::string string_;
    std::vector<Json> array_;
    std::map<std::string, Json> object_;


};

Json toJsonList(const std::vector<std::string>& values);

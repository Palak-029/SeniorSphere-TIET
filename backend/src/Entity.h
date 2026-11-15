#pragma once

#include <set>

#include "Json.h"

// Base class of everything stored in the database. Each subclass decides how
// it is written to and read from the JSON data file.
class Entity {
public:
    virtual ~Entity() {}

    int id() const { return id_; }
    void setId(int id) { id_ = id; }

    virtual Json toJson() const = 0;
    virtual void fromJson(const Json& data) = 0;

protected:
    int id_ = 0;

    static Json idsToJson(const std::set<int>& ids) {
        Json list = Json::array();
        for (std::set<int>::const_iterator it = ids.begin(); it != ids.end(); ++it) list.push(*it);
        return list;
    }

    static std::set<int> idsFromJson(const Json& list) {
        std::set<int> ids;
        for (std::size_t i = 0; i < list.items().size(); ++i) {
            ids.insert(static_cast<int>(list.items()[i].asInt()));
        }
        return ids;
    }
};

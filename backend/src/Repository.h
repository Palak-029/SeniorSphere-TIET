#pragma once

#include <algorithm>
#include <deque>
#include <type_traits>

#include "Entity.h"

// Generic in-memory collection of entities with auto-increment ids.
// std::deque is used so references stay valid when new items are added.
template <typename T>
class Repository {
    static_assert(std::is_base_of<Entity, T>::value, "Repository items must derive from Entity");

public:
    T& add(T item) {
        item.setId(nextId_++);
        items_.push_back(item);
        return items_.back();
    }

    T* find(int id) {
        for (typename std::deque<T>::iterator it = items_.begin(); it != items_.end(); ++it) {
            if (it->id() == id) return &*it;
        }
        return nullptr;
    }

    std::deque<T>& all() { return items_; }
    const std::deque<T>& all() const { return items_; }

    template <typename Predicate>
    void removeIf(Predicate predicate) {
        items_.erase(std::remove_if(items_.begin(), items_.end(), predicate), items_.end());
    }

    Json toJson() const {
        Json list = Json::array();
        for (typename std::deque<T>::const_iterator it = items_.begin(); it != items_.end(); ++it) {
            list.push(it->toJson());
        }
        return list;
    }

    void fromJson(const Json& list) {
        items_.clear();
        nextId_ = 1;
        for (std::size_t i = 0; i < list.items().size(); ++i) {
            T item;
            item.fromJson(list.items()[i]);
            if (item.id() <= 0) item.setId(nextId_);
            nextId_ = std::max(nextId_, item.id() + 1);
            items_.push_back(item);
        }
    }

private:
    std::deque<T> items_;
    int nextId_ = 1;
};

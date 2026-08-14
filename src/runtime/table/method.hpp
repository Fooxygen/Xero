
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <unordered_map>

#include "runtime/obj/obj.hpp"

namespace rt {

    class MethodTable {
    private:
        struct Key {
            const sema::Type* type;
            std::string       name;

            bool operator==(const Key& other) const {
                return type == other.type && name == other.name;
            }
        };

        struct KeyHash {
            size_t operator()(const Key& key) const {
                return std::hash<const sema::Type*>{}(key.type) ^
                       std::hash<std::string>{}(key.name);
            }
        };

        static inline std::unordered_map<Key, Fn, KeyHash> table_;

    public:
        static void Set(const sema::Type* type, const std::string& name, Fn fn) {
            table_[{type, name}] = fn;
        }

        static Fn Get(const sema::Type* type, const std::string& name) {
            return table_.contains({type, name}) ? table_.at({type, name}) : nullptr;
        }
    };
}

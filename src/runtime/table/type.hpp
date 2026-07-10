
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <unordered_map>

#include "log.hpp"

namespace rt {
    class Obj;

    struct Type {
        std::string_view name;
        size_t           size;  // byte width

        // Bulit-in Function
        std::string (*to_string)(const Obj&);
    };

    class TypeTable {
    private:
        static inline std::unordered_map<std::string, const Type*> table_;

    public:
        static void Reset() {
            table_ = std::unordered_map<std::string, const Type*>();
        }

        static const void Set(const Type& t) {
            auto it = table_.find(std::string(t.name));
            if (it == table_.end()) {
                table_.emplace(t.name, new Type(t));
            }
            else throw LogErr(LogModule::Runtime, std::format("existing type '{}'", t.name));
        }

        static const Type* Get(std::string_view name) {
            auto type_it = table_.find(std::string(name));
            if (type_it != table_.end()) {
                return type_it->second;
            }

            throw LogErr(LogModule::Runtime, std::format("undefined type '{}'", name));
            return nullptr;
        }
    };
}

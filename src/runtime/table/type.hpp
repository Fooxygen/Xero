
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
        std::string_view name  = "";
        size_t           size  = 0;         // Byte width
        bool             isRef = false;     // Reference Type

        // Methods

        static void        destroy_default(void*) {
            throw LogErr(LogModule::Runtime, "method 'destroy()' not implemented for type");
        }
        static std::string to_string_default(const Obj&) {
            throw LogErr(LogModule::Runtime, "method 'to_string()' not implemented for type");
        }

        void        (*destroy)(void*)           = destroy_default;
        std::string (*to_string)(const Obj&)    = to_string_default;

        // ArithOper

        Obj (*plus)  (const Obj&, const Obj&)   = nullptr;
        Obj (*minus) (const Obj&, const Obj&)   = nullptr;
        Obj (*star)  (const Obj&, const Obj&)   = nullptr;
        Obj (*slash) (const Obj&, const Obj&)   = nullptr;
        Obj (*neg)   (const Obj&)               = nullptr;
        
        // RelationOper

        Obj (*gt)   (const Obj&, const Obj&)    = nullptr;
        Obj (*lt)   (const Obj&, const Obj&)    = nullptr;
        Obj (*ge)   (const Obj&, const Obj&)    = nullptr;
        Obj (*le)   (const Obj&, const Obj&)    = nullptr;
        Obj (*eq)   (const Obj&, const Obj&)    = nullptr;
        Obj (*neq)  (const Obj&, const Obj&)    = nullptr;

        // LogicalOper

        Obj (*and_) (const Obj&, const Obj&)    = nullptr;
        Obj (*or_)  (const Obj&, const Obj&)    = nullptr;
        Obj (*not_) (const Obj&)                = nullptr;
    };

    class TypeTable {
    private:
        static inline std::unordered_map<std::string, const Type*> table_;

    public:
        static void Reset() {
            table_ = std::unordered_map<std::string, const Type*>();
        }

        static void Set(const Type& t) {
            if (!table_.contains(std::string(t.name))) {
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

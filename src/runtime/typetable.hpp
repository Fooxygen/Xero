
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <unordered_map>

#include "log.hpp"

namespace rt {

    struct Type {
        std::string_view name;
        size_t           size;  // bit width
    };

    class TypeTable {
    private:
        static inline const Type t_none_    = { .name = "none",  .size = 0  };
        static inline const Type t_i32_     = { .name = "i32",   .size = 32 };
        static inline const Type t_i64_     = { .name = "i64",   .size = 64 };
        static inline const Type t_f32_     = { .name = "f32",   .size = 32 };
        static inline const Type t_f64_     = { .name = "f64",   .size = 64 };

        static inline std::unordered_map<std::string, const Type*> table_;

    public:
        static void Reset() {
            table_ = std::unordered_map<std::string, const Type*>();
        }

        static const Type* Get(std::string_view name) {
            if (name == t_none_.name)   return &t_none_;
            if (name == t_i32_.name)    return &t_i32_;
            if (name == t_i64_.name)    return &t_i64_;
            if (name == t_f32_.name)    return &t_f32_;
            if (name == t_f64_.name)    return &t_f64_;

            auto type_it = table_.find(std::string(name));
            if (type_it != table_.end()) {
                return type_it->second;
            }

            throw LogErr(LogModule::Runtime, "undefined type");
            return nullptr;
        }
    };
}

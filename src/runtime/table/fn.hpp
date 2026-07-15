
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <unordered_map>

#include "runtime/obj/obj.hpp"

namespace rt {

    class FnTable {
    private:
        static inline std::unordered_map<std::string, Obj::Fn> table_;

    public:
        static void    Reset() {
            table_ = std::unordered_map<std::string, Obj::Fn>();
        }

        static void    Set(const std::string& name, Obj::Fn fn) {
            table_[name] = fn;
        }

        static Obj::Fn Get(const std::string& name) {
            return table_.contains(name) ? table_.at(name) : nullptr;
        }
    };
}

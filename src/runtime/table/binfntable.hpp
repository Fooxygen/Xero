
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <unordered_map>

#include "runtime/obj.hpp"

namespace rt {

    class BinfnTable {
    private:
        static inline std::unordered_map<std::string, Obj::binfn> table_;

    public:
        static void Reset() {
            table_ = std::unordered_map<std::string, Obj::binfn>();
        }

        static void Set(const std::string& name, Obj::binfn func) {
            table_[name] = func;
        }

        static Obj::binfn Get(const std::string& name) {
            auto it = table_.find(name);
            return it != table_.end() ? it->second : nullptr;
        }
    };
}

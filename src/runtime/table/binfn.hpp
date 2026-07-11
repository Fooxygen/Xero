
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <unordered_map>

#include "runtime/obj/obj.hpp"

namespace rt {

    class BinfnTable {
    public:
        using binfn = Obj(*)(std::vector<Obj>&);  // Built-in Function

    private:
        static inline std::unordered_map<std::string, binfn> table_;

    public:
        static void Reset() {
            table_ = std::unordered_map<std::string, binfn>();
        }

        static void Set(const std::string& name, binfn func) {
            table_[name] = func;
        }

        static binfn Get(const std::string& name) {
            auto it = table_.find(name);
            return it != table_.end() ? it->second : nullptr;
        }
    };
}

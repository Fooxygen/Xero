
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>
#include <unordered_map>

#include "common/utils.hpp"
#include "obj/obj.hpp"

namespace rt {

    class Env {
    private:
        std::vector<std::unordered_map<std::string, Obj>> scopes_;

    public:
        void ScopePush() { scopes_.emplace_back(); }
        void ScopePop()  { scopes_.pop_back(); }
        std::unordered_map<std::string, Obj>& ScopeGet() { return scopes_.back(); }

        void Declare(const std::string& name, const Obj& value) {
            ScopeGet()[name] = value;
        }
        void Assign(const std::string& name, const Obj& value) {
            *Get(name) = value;
        }
        Obj* Get(const std::string& name, std::optional<Loc> loc = std::nullopt) {
            for (auto sit = scopes_.rbegin(); sit != scopes_.rend(); sit++) {
                auto oit = sit->find(name);
                if (oit != sit->end()) return &oit->second;
            }

            throw LogErr(LogModule::Runtime, std::format("undefined object '{}'", name), loc);
        }
    };
}

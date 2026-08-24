
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>
#include <unordered_map>

#include "common/utils.hpp"
#include "xengine/obj/obj.hpp"

namespace xengine {

    class Env {
    private:
        std::vector<std::unordered_map<std::string, Obj>> scopes_;

    public:
        void ScopePush() { scopes_.emplace_back(); }
        void ScopePop()  { scopes_.pop_back(); }
        std::unordered_map<std::string, Obj>& ScopeGet() { return scopes_.back(); }

        void Declare(const std::string& name, const Obj& value) {
            if (name.empty()) {
                throw LogErr(LogModule::Xengine, "empty declared name");
            }
            auto& scope = ScopeGet();
            if (!scope.contains(name)) scope[name] = value;
            else {
                throw LogErr(LogModule::Xengine, std::format(
                    "duplicate definition of '{}' in same scope",
                    name
                ));
            }
        }
        void Assign(const std::string& name, const Obj& value) {
            *Lookup(name) = value;
        }
        Obj* Lookup(const std::string& name, std::optional<Loc> loc = std::nullopt) {
            for (auto sit = scopes_.rbegin(); sit != scopes_.rend(); sit++) {
                auto oit = sit->find(name);
                if (oit != sit->end()) return &oit->second;
            }

            throw LogErr(LogModule::Xengine, std::format("undefined obj '{}'", name), loc);
        }
    };
}

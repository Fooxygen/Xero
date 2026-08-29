
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

#include "common/log.hpp"
#include "common/utils.hpp"
#include "sema/defs/type.hpp"

namespace sema {

    class Var {
    public:
        std::string name_ = "";
        Type*       type_ = nullptr;
        Loc         loc_;
    
        Var(std::string name, Type* type, Loc loc)
        :   name_(name), type_(type), loc_(loc) {}
    };

    class VarTable {
    public:
        using Scope = std::unordered_map<std::string, std::unique_ptr<Var>>;

    private:
        std::vector<Scope> scopes_;

    public:
        void   ScopePush() { scopes_.emplace_back(); }
        void   ScopePop()  { scopes_.pop_back(); }
        Scope& ScopeGet()  { return scopes_.back(); }

        void   Declare(std::unique_ptr<Var>&& var) {
            if (var->name_.empty()) {
                throw LogErr(LogModule::Sema, "empty variable declared name");
            }
            
            auto& scope = ScopeGet();
            if (!scope.contains(var->name_)) {
                scope[var->name_] = std::move(var);
            }
            else {
                throw LogErr(LogModule::Sema, std::format(
                    "redefinition of variable '{}' in same scope",
                    var->name_
                ));
            }
        }
        
        const Var* Lookup(const std::string& name, std::optional<Loc> loc = std::nullopt) {
            for (auto sit = scopes_.rbegin(); sit != scopes_.rend(); sit++) {
                auto oit = sit->find(name);
                if (oit != sit->end()) return &(*oit->second);
            }
            throw LogErr(LogModule::Sema, std::format("undefined variable '{}'", name), loc);
        }
        const Var* LookupTry(const std::string& name) {
            for (auto sit = scopes_.rbegin(); sit != scopes_.rend(); sit++) {
                auto oit = sit->find(name);
                if (oit != sit->end()) return &(*oit->second);
            }
            return nullptr;
        }
    };
}


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
#include "common/defs/var.hpp"
#include "common/defs/loc.hpp"
#include "common/defs/scope.hpp"
#include "sema/defs/type.hpp"

namespace sema {

    class Var {
    public:
        std::string  name_     = "";
        Type*        type_     = nullptr;
        Loc          loc_;
        VarModifier  modifier_ = VarModifier::None;

        Var(
            std::string  name,
            Type*        type,
            Loc          loc,
            VarModifier  modifier = VarModifier::None
        )
        :   name_(name), type_(type), loc_(loc), modifier_(modifier) {}
    };

    class VarScope {
    private:
        Scope<std::unique_ptr<Var>> scope_;

    public:
        void Push() { scope_.Push(); }
        void Pop()  { scope_.Pop(); }

        void Declare(std::unique_ptr<Var>&& var) {
            auto name = var->name_;
            auto loc  = var->loc_;

            if (name.empty()) {
                throw LogErr(LogStage::Sema, "empty variable declared name", loc);
            }

            auto declared = scope_.Declare(name, std::move(var));
            if (!declared.second) {
                throw LogErr(LogStage::Sema, std::format(
                    "redefinition of variable '{}' in same scope",
                    name
                ), loc);
            }
        }

        const Var* Lookup(const std::string& name, std::optional<Loc> loc = std::nullopt) {
            auto var = LookupTry(name);
            if (!var) {
                throw LogErr(LogStage::Sema, std::format("undefined variable '{}'", name), loc);
            }
            return var;
        }
        const Var* LookupTry(const std::string& name) {
            auto var = scope_.LookupTry(name);
            return var ? var->get() : nullptr;
        }
    };

    class ModuleConstantScope {
    private:
        Namespace<DeclExpr*> scope_;

    public:
        void Enter(const Path &path) {
            scope_.Enter(path);
        }

        void Declare(const std::string& name, DeclExpr* declexpr) {
            auto declared = scope_.Declare(name, declexpr);
            if (!declared.second) {
                throw LogErr(LogStage::Sema, std::format(
                    "redefinition of constant '{}' in same scope",
                    name
                ), declexpr->loc_);
            }
        }

        DeclExpr* LookupTry(const Path& path) {
            auto declexpr = scope_.LookupTry(path);
            return declexpr ? *declexpr : nullptr;
        }
    };
}

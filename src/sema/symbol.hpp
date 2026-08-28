
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
#include "sema/type.hpp"
#include "sema/sign.hpp"

namespace sema {
    
    enum class SymbolType {
        Undefined, Var, Fn
    };

    class Symbol {
    public:
        std::string name_ = "";
        SymbolType  type_ = SymbolType::Undefined;
        Loc         loc_;
    
        Symbol(std::string name, SymbolType type, Loc loc)
        :   name_(name), type_(type), loc_(loc) {}
    };

    class SymbolTable {
    public:
        using Scope = std::unordered_map<std::string, std::unique_ptr<Symbol>>;

    private:
        std::vector<Scope> scopes_;

    public:
        void   ScopePush() { scopes_.emplace_back(); }
        void   ScopePop()  { scopes_.pop_back(); }
        Scope& ScopeGet()  { return scopes_.back(); }

        void          Declare(std::unique_ptr<Symbol>&& symbol) {
            if (symbol->name_.empty()) {
                throw LogErr(LogModule::Sema, "empty declared name");
            }
            auto& scope = ScopeGet();
            if (!scope.contains(symbol->name_)) scope[symbol->name_] = std::move(symbol);
            else {
                throw LogErr(LogModule::Sema, std::format(
                    "duplicate definition of '{}' in same scope",
                    symbol->name_
                ));
            }
        }
        const Symbol* Lookup(const std::string& name, std::optional<Loc> loc = std::nullopt) {
            for (auto sit = scopes_.rbegin(); sit != scopes_.rend(); sit++) {
                auto oit = sit->find(name);
                if (oit != sit->end()) return &(*oit->second);
            }

            throw LogErr(LogModule::Sema, std::format("undefined symbol '{}'", name), loc);
        }
    };

    class VarSymbol : public Symbol {
    public:
        Type* ret_type_ = nullptr;

        VarSymbol(std::string name, Loc loc, Type* ret_type)
        :   Symbol(name, SymbolType::Var, loc), ret_type_(ret_type) {}
    };
    
    class FnSymbol  : public Symbol {
    public:
        std::unique_ptr<FnSign> sign_ = nullptr;

        FnSymbol(std::string name, Loc loc, const FnSign& fn_sign)
        :   Symbol(name, SymbolType::Fn, loc), sign_(std::make_unique<FnSign>(fn_sign))
        {}
    };
}

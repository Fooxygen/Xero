
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
#include "sema/semantics.hpp"

namespace sema {
    
    enum class SymbolType {
        Undefined,

        Var,
        Fn
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
    private:
        std::vector<std::unordered_map<std::string, std::unique_ptr<Symbol>>> scopes_;

    public:
        void ScopePush() { scopes_.emplace_back(); }
        void ScopePop()  { scopes_.pop_back(); }
        std::unordered_map<std::string, std::unique_ptr<Symbol>>&
             ScopeGet()  { return scopes_.back(); }

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
        const Type* var_type_ = nullptr;

        VarSymbol(
            std::string name, Loc loc,
            const Type* var_type
        )
        :   Symbol(name, SymbolType::Var, loc),
            var_type_(var_type)
        {}
    };
    class FnSymbol  : public Symbol {
    public:
        const Type* ret_type_ = nullptr;

        // Params:
        // | params_fix_ | param_infinite_ ...
        std::vector<const Type*>   params_fix_ = {};                // nullptr: any type
        std::optional<const Type*> param_inf_  = std::nullopt;

        FnSymbol(
            std::string name, Loc loc,
            const Type* ret_type,
            const std::vector<const Type*>& params_fix = {},
            std::optional<const Type*>      param_inf  = std::nullopt
        )
        :   Symbol(name, SymbolType::Fn, loc),
            ret_type_(ret_type),
            params_fix_(params_fix),
            param_inf_(param_inf)
        {}
    };

    class MethodSymbolTable {
    private:
        struct Key {
            const Type* type = nullptr;
            std::string name = "";

            bool operator==(const Key& other) const {
                return type == other.type && name == other.name;
            }
        };

        struct KeyHash {
            size_t operator()(const Key& key) const {
                return std::hash<const Type*>{}(key.type) ^
                       std::hash<std::string>{}(key.name);
            }
        };

        static inline std::unordered_map<Key, FnSymbol, KeyHash> table_;

    public:
        static void Set(const Type* type, FnSymbol fn) {
            table_.insert_or_assign(Key{type, fn.name_}, std::move(fn));
        }

        static FnSymbol* Get(const Type* type, const std::string& name) {
            auto it = table_.find({type, name});
            return it == table_.end() ? nullptr : &it->second;
        }
    };
}

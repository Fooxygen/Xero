
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>

#include "common/log.hpp"

namespace sema {
    class Type;
    
    // Modifier of Fn
    struct FnModifier {
        bool hasCast_ = false;

        bool operator ==(const FnModifier& modifier) {
            return modifier.hasCast_ == hasCast_;
        }
    };

    // Signature of Fn
    class  FnSign {
    private:
        Type*                ret_type_        = nullptr;
        std::vector<Type*>   params_type_fix_ = {};
        std::optional<Type*> params_type_var_ = std::nullopt;
        FnModifier           modifier_        = FnModifier{};

    public:
        FnSign(
            Type*                     ret_type,
            const std::vector<Type*>& params_type_fix = {},
            std::optional<Type*>      params_type_var = std::nullopt,
            FnModifier                modifier        = FnModifier{}
        )
        :   ret_type_(ret_type),
            params_type_fix_(params_type_fix),
            params_type_var_(params_type_var),
            modifier_(modifier)
        {}

        Type*                       ret_type()        const { return ret_type_; }
        const std::vector<Type*>&   params_type_fix() const { return params_type_fix_; }
        const std::optional<Type*>& params_type_var() const { return params_type_var_; }
        const FnModifier&           modifier()        const { return modifier_; }

    public:
        std::string ParamsPrint() const;

        bool isSignEqual(const FnSign& sign);
        bool isSignMatch(const std::vector<Type*>& args_type);      // implicit type cast
    };

    // Definition of Fn
    class  Fn {
    private:
        std::string name_ = "";
        std::vector<std::unique_ptr<FnSign>> signs_;

    public:
        Fn(const std::string& name) : name_(name) {}

        std::vector<std::unique_ptr<FnSign>>& signs() { return signs_; }

    public:
        const FnSign* SignLookup(const FnSign& sign) const;
        const FnSign* SignLookup(const std::vector<Type*>& args_type);
        const FnSign* SignLookupTry(const FnSign& sign) const;
        const FnSign* SignLookupTry(const std::vector<Type*>& args_type);

        const FnSign* SignAdd(const FnSign& fnsign);
    };

    // Global Fn Table
    class  FnTable {
    private:
        std::unordered_map<std::string, Fn> table_;

    public:
        std::unordered_map<std::string, Fn>& table() { return table_; }

    public:
        Fn&  Lookup(const std::string& name) {
            auto it = table_.find(name);
            if (it == table_.end()) {
                throw LogErr(LogModule::Sema, std::format(
                    "undefined function '{}'", name
                ));
            }
            return it->second;
        }
        Fn*  LookupTry(const std::string& name) {
            auto it = table_.find(name);
            return it == table_.end() ? nullptr : &it->second;
        }
        
        const FnSign* Add(const std::string& name, const FnSign& sign) {
            auto& fn = table_.try_emplace(name, name).first->second;
            return fn.SignAdd(sign);
        }
    };
}

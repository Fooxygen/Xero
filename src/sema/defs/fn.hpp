
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
    enum   FnModifier : int {
        None = 0,
        Cast = 1 << 0,
    };

    inline FnModifier operator |(FnModifier a, FnModifier b) {
        return FnModifier((int)a | (int)b);
    }
    inline FnModifier operator &(FnModifier a, FnModifier b) {
        return FnModifier((int)a & (int)b);
    }

    // Signature of Fn
    class  FnSign {
    private:
        Type*                return_type_     = nullptr;
        Type*                caller_type_     = nullptr;
        std::vector<Type*>   params_type_fix_ = {};
        std::optional<Type*> params_type_var_ = std::nullopt;
        FnModifier           modifier_        = FnModifier::None;
        std::string          name_            = "";
        const FnSign*        template_sign_   = nullptr;

    public:
        FnSign(
            Type*                     return_type,
            const std::vector<Type*>& params_type_fix = {},
            std::optional<Type*>      params_type_var = std::nullopt,
            FnModifier                modifier        = FnModifier{},
            const std::string&        name            = ""
        )
        :   return_type_(return_type),
            params_type_fix_(params_type_fix),
            params_type_var_(params_type_var),
            modifier_(modifier),
            name_(name)
        {}

        Type*                       return_type()     const { return return_type_; }
        Type*                       caller_type()     const { return caller_type_; }
        const std::vector<Type*>&   params_type_fix() const { return params_type_fix_; }
        const std::optional<Type*>& params_type_var() const { return params_type_var_; }
        const FnModifier&           modifier()        const { return modifier_; }
        const std::string&          name()            const { return name_; }
        const FnSign*               template_sign()   const { return template_sign_ ? template_sign_ : this; }

    public:
        void NameSet(const std::string& name) { name_ = name; }

        std::string ParamsPrint() const;

        bool IsSignEqual(const FnSign& sign);
        bool IsSignMatch(const std::vector<Type*>& args_type);      // implicit type cast

        // As Method

        bool IsCallerMatch(Type* type) const;
        void CallerSet(Type* type) { caller_type_ = type; }

        void TemplateSignSet(const FnSign* sign) { template_sign_ = sign; }
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
        const FnSign* SignLookup(const FnSign& sign, std::optional<Loc> loc = std::nullopt) const;
        const FnSign* SignLookup(const std::vector<Type*>& args_type, std::optional<Loc> loc = std::nullopt);
        const FnSign* SignLookup(Type* caller_type, const std::vector<Type*>& args_type, std::optional<Loc> loc = std::nullopt);
        const FnSign* SignLookupTry(const FnSign& sign) const;
        const FnSign* SignLookupTry(const std::vector<Type*>& args_type);
        const FnSign* SignLookupTry(Type* caller_type, const std::vector<Type*>& args_type);

        const FnSign* SignAdd(const std::string& name, const FnSign& sign);
    };

    // Global Fn Table
    class  FnTable {
    private:
        std::unordered_map<std::string, Fn> table_;
        Type* owner_ = nullptr;     // owner should be recorded when used in a type table

    public:
        std::unordered_map<std::string, Fn>& table() { return table_; }

    public:
        Fn&  Lookup(const std::string& name, std::optional<Loc> loc = std::nullopt) {
            auto it = table_.find(name);
            if (it == table_.end()) {
                throw LogErr(LogModule::Sema, std::format(
                    "undefined function '{}'", name
                ), loc);
            }
            return it->second;
        }
        Fn*  LookupTry(const std::string& name) {
            auto it = table_.find(name);
            return it == table_.end() ? nullptr : &it->second;
        }
        
        const FnSign* Add(const std::string& name, const FnSign& sign) {
            auto& fn = table_.try_emplace(name, name).first->second;
            auto  sign_add = sign;
            if (owner_) sign_add.CallerSet(owner_);
            return fn.SignAdd(name, sign_add);
        }
    
        void OwnerSet(Type* type) { owner_ = type; }
    };
}

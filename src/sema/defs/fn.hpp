
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
#include "common/utils/format.hpp"

namespace sema {
    class Type;
    
    // Modifier
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

    // Signature
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

        static std::string ParamsPrint(
            const std::vector<Type*>& params_type_fix,
            std::optional<Type*>      params_type_var = std::nullopt
        );
        std::string        ParamsPrint() const;

        bool IsSignEqual(const FnSign& sign);
        bool IsSignMatch(const std::vector<Type*>& args_type);      // implicit type cast

        // As Method

        bool IsCallerMatch(Type* type) const;
        void CallerSet(Type* type) { caller_type_ = type; }

        void TemplateSignSet(const FnSign* sign) { template_sign_ = sign; }
    };

    // Obj
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

    // Table
    class  FnTable {
    public:
        class Namespace {
        public:
            std::string name_   = "";
            Namespace*  parent_ = nullptr;
            std::unordered_map<std::string, std::unique_ptr<Namespace>> children_ = {};
            std::unordered_map<std::string, Fn>                         table_    = {};

        public:
            Namespace() = default;
            Namespace(std::string name, Namespace* parent)
            :   name_(std::move(name)), parent_(parent) {}
        
            std::unordered_map<std::string, Fn>& table() { return table_; }
        };

    private:
        Namespace  ns_root_;
        Namespace* ns_current_ = &ns_root_;
        Type*      owner_ = nullptr;    // owner should be recorded when used in a type table

    public:
        Namespace* namespace_current() { return ns_current_; }
    
    public:
        void NamespaceEnter(const std::vector<std::string>& path) {
            auto* ns = &ns_root_;
            for (auto& seg : path) {
                auto& child = ns->children_[seg];
                if (!child) {
                    child = std::make_unique<Namespace>(seg, ns);
                }
                ns = child.get();
            }
            ns_current_ = ns;
        }
        void NamespaceEnter() { ns_current_ = &ns_root_; }

        // call fn, only callee

        Fn*  Lookup(const std::string& name, std::optional<Loc> loc = std::nullopt) {
            auto fn = LookupTry(name);
            if (!fn) {
                throw LogErr(LogStage::Sema, std::format(
                    "undefined function '{}'", name
                ), loc);
            }
            return fn;
        }
        Fn*  LookupTry(const std::string& name) {
            for (auto* ns = ns_current_; ns; ns = ns->parent_) {
                auto it = ns->table_.find(name);
                if (it != ns->table_.end()) return &it->second;
            }
            return nullptr;
        }
        
        // call a::b::fn, path + callee

        Fn*  Lookup(const std::vector<std::string>& path, std::optional<Loc> loc = std::nullopt) {
            auto fn = LookupTry(path);
            if (!fn) {
                throw LogErr(LogStage::Sema, std::format(
                    "undefined function '{}'",
                    format::JoinWithBoundary(path, [](std::string seg) {
                        return seg;
                    }, "", "", "::")
                ), loc);
            }
            return fn;
        }
        Fn*  LookupTry(const std::vector<std::string>& path) {
            auto* ns = &ns_root_;

            // search a::b
            for (size_t i = 0; i < path.size() - 1; i++) {
                auto ns_it = ns->children_.find(path[i]);
                if (ns_it == ns->children_.end()) return nullptr;
                ns = ns_it->second.get();
            }

            // search fn
            auto it = ns->table_.find(path.back());
            return it == ns->table_.end() ? nullptr : &it->second;
        }

        const FnSign* Add(const std::string& name, const FnSign& sign) {
            auto& fn = ns_current_->table_.try_emplace(name, name).first->second;
            auto  sign_add = sign;
            if (owner_) sign_add.CallerSet(owner_);
            return fn.SignAdd(name, sign_add);
        }

        void OwnerSet(Type* type) { owner_ = type; }
    };
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <utility>

#include "common/log.hpp"
#include "sema/defs/fn.hpp"

namespace sema {

    class Type {
    public:
        enum class Using {
            Basic,          // array
            Parametric,     // array[=i32=]
            Reference,      // array&
            Binding         // T
        };
    
    private:
        std::string name_       = "";
        Using       type_using_ = Using::Basic;

        std::set<Type*>                          casts_;
        std::unordered_map<Type*, const FnSign*> casts_fnsign_;     // signature of cast method

    public:
        Type(std::string name, Using type_using)
        :   name_(name), type_using_(type_using) {}
        
        const std::string& name()       const { return name_; }
        Using              type_using() const { return type_using_; }

        std::set<Type*>& casts() { return casts_; }
        std::unordered_map<Type*, const FnSign*>& casts_fnsign() { return casts_fnsign_; }

    public:
        bool IsNone();
        bool Is(std::string_view name);

        virtual Type* BasicTypeGet() = 0;
        void          BasicTypeCheck() const;

        virtual Type* ReferenceUnwrap() = 0;
    };

    class BindingType    : public Type {
    public:
        BindingType(const std::string& name)
        :   Type(name, Using::Binding) {}

        Type* BasicTypeGet()    override { return this; }
        Type* ReferenceUnwrap() override { return this; }
    };

    class BasicType      : public Type {
    private:
        size_t                    params_cnt_     = 0;      // number of parameters in parametric type
        std::vector<BindingType*> params_binding_ = {};     // binding types
        FnTable                   method_table_;

    public:
        BasicType(std::string name, size_t params_cnt = 0)
        :   Type(name, Using::Basic),
            params_cnt_(params_cnt)
        {
            method_table_.OwnerSet(this);
        }

        size_t                     params_cnt() const { return params_cnt_; }
        std::vector<BindingType*>& params_binding()   { return params_binding_; }
        FnTable&                   method_table()     { return method_table_; }
    
    public:
        Type* BasicTypeGet()    override { return this; }
        Type* ReferenceUnwrap() override { return this; }
    };

    class ParametricType : public Type {
    private:
        Type*              basic_  = nullptr;
        std::vector<Type*> params_ = {};
        FnTable            method_table_;

    public:
        ParametricType(std::string name, Type* basic_type, const std::vector<Type*>& params)
        :   Type(name, Using::Parametric),
            basic_(basic_type),
            params_(params)
        {
            method_table_.OwnerSet(this);
        }
        ParametricType(const ParametricType& other)
        :   Type(other.name(), Using::Parametric),
            basic_(other.basic_),
            params_(other.params_)
        {
            method_table_.OwnerSet(this);
        }

        Type*               basic() const  { return basic_; }
        std::vector<Type*>& params()       { return params_; }
        FnTable&            method_table() { return method_table_; }

    public:
        static std::string ParamsPrint(Type* basic_type, const std::vector<Type*>& params);

        Type* BasicTypeGet()    override { return basic_; }
        Type* ReferenceUnwrap() override { return this; }
    };

    class ReferenceType  : public Type {
    private:
        Type* referred_  = nullptr;

    public:
        ReferenceType(std::string name, Type* referred)
        :   Type(name, Using::Reference),
            referred_(referred)
        {}

        Type* referred() const { return referred_; }

    public:
        Type* BasicTypeGet()    override { return referred_->BasicTypeGet(); }
        Type* ReferenceUnwrap() override { return referred_; }
    };

    class TypeTable {
    private:
        static inline std::unordered_map<std::string, Type*> table_;
        static inline std::multimap<Type*, Type*>            casts_;
        static inline std::map<std::set<Type*>, Type*>       common_cache_;

    public:
        static void  Init();

        static Type*  Set(const BasicType& t);
        static Type*  Set(const ParametricType& t);
        static Type*  Set(const ReferenceType& t);
        static Type*  Lookup(std::string_view name, std::optional<Loc> loc = std::nullopt);
        static Type*  LookupTry(std::string_view name);
        
        static Type*  ParametricTypeGet(Type* type, const std::vector<Type*>& params, std::optional<Loc> loc = std::nullopt);
        static Type*  ReferenceTypeGet(Type* type);

        static bool   IsContainBindingType(Type* type);
        static bool   IsContainBindingType(const FnSign& sign);
        static Type*  BindingTypeReplace(Type* type, BasicType* owner, const std::vector<Type*>& params_replace);
        static FnSign SignInstantiate(const FnSign& sign, BasicType* owner, const std::vector<Type*>& params_replace);
        
        static Fn*    MethodLookup(Type* type, const std::string& name);
        static Fn*    MethodLookupTry(Type* type, const std::string& name);

        static void   CastRecompute();
        static Type*  Common(std::set<Type*> ts) {
            if (ts.size() == 1) return *ts.begin();
            
            // Search Cache
            auto it = common_cache_.find(ts);
            if (it != common_cache_.end()) return it->second;

            // Get Common
            std::set<Type*> common;
            {
                bool is_first_add = false;
                for (auto t : ts) {
                    if (!is_first_add) {
                        is_first_add = true;
                        common = t->casts();
                        continue;
                    }

                    std::set<Type*> tmp;
                    std::set_intersection(
                        common.begin(), common.end(),
                        t->casts().begin(), t->casts().end(),
                        std::inserter(tmp, tmp.begin())
                    );
                    common = std::move(tmp);

                    if (common.empty()) {
                        common_cache_[ts] = nullptr;
                        return nullptr;
                    }
                }
            }

            // Find Minimal
            for (auto& i : common) {
                bool is_find = true;

                for (auto& j : common) {
                    if (i == j) continue;
                    if (j->casts().contains(i)) {
                        is_find = false;
                        break;
                    }
                }

                if (is_find) {
                    common_cache_[ts] = i;
                    return i;
                }
            }

            return nullptr;
        }
    };
}

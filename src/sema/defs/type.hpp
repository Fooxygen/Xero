
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
            Reference       // array&
        };
    
    private:
        std::string     name_       = "";
        Using           type_using_ = Using::Basic;

        std::set<Type*>                          casts_;
        std::unordered_map<Type*, const FnSign*> casts_fnsign_;     // signature of cast method

    public:
        Type(std::string name, Using type_using)
        :   name_(name), type_using_(type_using) {}
        
        std::string      name()       const { return name_; }
        Using            type_using() const { return type_using_; }

        std::set<Type*>& casts() { return casts_; }
        std::unordered_map<Type*, const FnSign*>& casts_fnsign() { return casts_fnsign_; }

    public:
        bool isNone();
        bool is(std::string_view name);

        virtual Type* BasicTypeGet() = 0;
        void          BasicTypeCheck() const;
    };

    class BasicType      : public Type {
    private:
        size_t  params_cnt_ = 0;     // Number of Type Parameters
        FnTable method_table_;

    public:
        BasicType(std::string name, size_t params_cnt = 0)
        :   Type(name, Using::Basic),
            params_cnt_(params_cnt)
        {}

        size_t   params_cnt() const { return params_cnt_; }
        FnTable& method_table()     { return method_table_; }
    
    public:
        Type* BasicTypeGet() override { return this; }
    };

    class ParametricType : public Type {
    private:
        Type*              type_basic_  = nullptr;
        std::vector<Type*> params_type_ = {};

    public:
        ParametricType(std::string name, Type* type_basic, const std::vector<Type*>& params_type)
        :   Type(name, Using::Parametric),
            type_basic_(type_basic),
            params_type_(params_type)
        {}

        Type*               type_basic() const { return type_basic_; }
        std::vector<Type*>& params_type()      { return params_type_; }

    public:
        static std::string ParamsPrint(Type* type_basic, const std::vector<Type*>& params_type);

        Type* BasicTypeGet() override { return type_basic_; }
    };

    class ReferenceType  : public Type {
    private:
        Type* type_referred_  = nullptr;

    public:
        ReferenceType(std::string name, Type* type_referred)
        :   Type(name, Using::Reference),
            type_referred_(type_referred)
        {}

        Type* type_referred() const { return type_referred_; }

    public:
        Type* BasicTypeGet() override { return type_referred_->BasicTypeGet(); }
    };

    class TypeTable {
    private:
        static inline std::unordered_map<std::string, Type*> table_;
        static inline std::multimap<Type*, Type*>            casts_;
        static inline std::map<std::set<Type*>, Type*>       common_cache_;

    public:
        static void  Init();

        static Type* Set(const BasicType& t);
        static Type* Set(const ParametricType& t);
        static Type* Set(const ReferenceType& t);
        static Type* Lookup(std::string_view name, std::optional<Loc> loc = std::nullopt);
        static Type* LookupTry(std::string_view name);
        
        static Type* ParametricTypeGet(Type* type, const std::vector<Type*>& params, std::optional<Loc> loc = std::nullopt);
        static Type* ReferenceTypeGet(Type* type);

        static void  CastRecompute();
        static Type* Common(std::set<Type*> ts) {
            if (ts.size() == 1) return *ts.begin();
            
            // Search Cache
            if (common_cache_.contains(ts)) return common_cache_[ts];

            // Get Common
            std::set<Type*> common;
            {
                bool isFirstAdd = false;
                for (auto t : ts) {
                    if (!isFirstAdd) {
                        isFirstAdd = true;
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
                bool isFind = true;

                for (auto& j : common) {
                    if (i == j) continue;
                    if (j->casts().contains(i)) {
                        isFind = false;
                        break;
                    }
                }

                if (isFind) {
                    common_cache_[ts] = i;
                    return i;
                }
            }

            return nullptr;
        }
    };
}

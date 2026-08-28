
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
#include <variant>
#include <utility>

#include "common/log.hpp"
#include "sema/sign.hpp"

namespace sema {

    class Type {
    public:
        enum class Using {
            Basic,      // -> array
            Parametric  // -> array[=i32=]
        };
    
    private:
        std::string     name_       = "";
        Using           type_using_ = Using::Basic;
        std::set<Type*> casts_;

    public:
        Type(std::string name, Using type_using)
        :   name_(name), type_using_(type_using) {}
        
        std::string      name()       const { return name_; }
        Using            type_using() const { return type_using_; }
        std::set<Type*>& casts()            { return casts_; }

    public:
        bool isNone();
        bool is(std::string_view name);

        virtual Type* BasicTypeGet() = 0;
        void          BasicTypeCheck() const;
    };

    // exp: array
    class BasicType      : public Type {
    private:
        size_t  params_cnt_ = 0;     // Number of Type Parameters
        FnTable methods_;

    public:
        BasicType(std::string name, size_t params_cnt = 0)
        :   Type(name, Using::Basic),
            params_cnt_(params_cnt)
        {}

        size_t   params_cnt() const { return params_cnt_; }
        FnTable& methods()          { return methods_; }
    
        Type* BasicTypeGet() override { return this; }
    };

    // exp: array[=i32=]
    class ParametricType : public Type {
    private:
        Type*              base_type_   = nullptr;
        std::vector<Type*> params_type_ = {};

    public:
        ParametricType(
            std::string               name,
            Type*                     base_type,
            const std::vector<Type*>& params_type
        )
        :   Type(name, Using::Parametric),
            base_type_(base_type),
            params_type_(params_type)
        {}

        Type*               base_type() const { return base_type_; }
        std::vector<Type*>& params_type()     { return params_type_; }

        static std::string ParamsPrint(Type* basic_type, const std::vector<Type*>& params_type);

        Type* BasicTypeGet() override { return base_type_; }
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
        static Type* Lookup(std::string_view name, std::optional<Loc> loc = std::nullopt);
        static Type* LookupTry(std::string_view name);
        
        static Type* ParametricTypeGet(
            Type* type, const std::vector<Type*>& params,
            std::optional<Loc> loc = std::nullopt
        );

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

                    std::set<Type*> temp;
                    std::set_intersection(
                        common.begin(), common.end(),
                        t->casts().begin(), t->casts().end(),
                        std::inserter(temp, temp.begin())
                    );
                    common = std::move(temp);

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

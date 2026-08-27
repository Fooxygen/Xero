
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

namespace xcompiler {
    class TypeGen;
}

namespace sema {

    class Type {
    public:
        enum class Using {
            Base, Param
        };
    
        public:
        std::string           name_       = "";
        Using                 type_using_ = Using::Base;
        std::set<const Type*> casts_;

        Type(std::string name, Using type_using)
        :   name_(name), type_using_(type_using) {}
        
        bool isNone() const;
        bool is(std::string_view name) const;

        virtual const Type* BasicTypeGet() const = 0;
        void BasicTypeCheck() const;
    };

    // exp: array
    class BasicType      : public Type {
    public:
        size_t params_cnt_ = 0;     // Number of Type Parameters
        std::unordered_map<std::string, FnOverloads> methods_;

        BasicType(std::string name, size_t params_cnt = 0)
        :   Type(name, Using::Base),
            params_cnt_(params_cnt)
        {}

        void MethodAdd(const std::string& name, const FnSign& fnsign) {
            methods_[name].fnsigns_.emplace_back(std::make_unique<FnSign>(fnsign));
        }
    
        const Type* BasicTypeGet() const override { return this; }
    };

    // exp: array[=i32=]
    class ParametricType : public Type {
    public:
        const Type*              base_type_   = nullptr;
        std::vector<const Type*> params_type_ = {};

        ParametricType(
            std::string name,
            const Type* base_type,
            const std::vector<const Type*>& params_type
        )
        :   Type(name, Using::Param),
            base_type_(base_type),
            params_type_(params_type)
        {}

        std::vector<const Type*>& params_type() { return params_type_; }

        static std::string ParamsPrint(const Type* basic_type, const std::vector<const Type*>& params_type);

        const Type* BasicTypeGet() const override { return base_type_; }
    };

    class TypeTable {
    private:
        static inline std::unordered_map<std::string, Type*>        table_;
        static inline std::multimap<const Type*, const Type*>       casts_;
        static inline std::map<std::set<const Type*>, const Type*>  common_cache_;

    public:
        static void Init();

        static Type* Set(const BasicType& t);
        static Type* Set(const ParametricType& t);
        static Type* Lookup(std::string_view name, std::optional<Loc> loc = std::nullopt);
        static Type* LookupTry(std::string_view name);
        
        static Type* ParamTypeGet(
            const Type* type, const std::vector<const Type*>& params,
            std::optional<Loc> loc = std::nullopt
        );

        static void        CastRecompute();
        static const Type* Common(std::set<const Type*> ts) {
            if (ts.size() == 1) return *ts.begin();
            
            // Search Cache
            if (common_cache_.contains(ts)) return common_cache_[ts];

            // Get Common
            std::set<const Type*> common;
            {
                bool isFirstAdd = false;
                for (auto t : ts) {
                    if (!isFirstAdd) {
                        isFirstAdd = true;
                        common = t->casts_;
                        continue;
                    }

                    std::set<const Type*> temp;
                    std::set_intersection(
                        common.begin(), common.end(),
                        t->casts_.begin(), t->casts_.end(),
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
                    if (j->casts_.contains(i)) {
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

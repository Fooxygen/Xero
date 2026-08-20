
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

namespace rt {
    class TypeImpl;
}

namespace sema {

    class Type {
    private:
        const rt::TypeImpl* impl_ = nullptr;
    
    public:
        enum class UsingType {
            Base, Param
        };

        struct BaseInfo {
            size_t size_         = 0;           // Byte width
            bool   isHeapStored_ = false;       // Stored Data in Heap
            size_t params_cnt_   = 0;
        };

        struct ParamInfo {
            const Type*              base_   = nullptr;
            std::vector<const Type*> params_ = {};
        };

        UsingType                         usingtype_ = UsingType::Base;
        std::string                       name_      = "";
        std::variant<BaseInfo, ParamInfo> info_{BaseInfo{}};
        std::set<const Type*>             converts_  = {};              // List of convertible types

    public:
        Type(std::string_view name, BaseInfo base)
        :   usingtype_(UsingType::Base), name_(name), info_(std::move(base)) {}
        Type(std::string_view name, ParamInfo param)
        :   usingtype_(UsingType::Param), name_(name), info_(std::move(param)) {}

        const BaseInfo&  baseinfo()  const { return std::get<BaseInfo>(info_); }
        const ParamInfo& paraminfo() const { return std::get<ParamInfo>(info_); }
        
        bool isHeapStored() const {
            return base()->baseinfo().isHeapStored_;
        }
        const Type*                     base() const {
            switch(usingtype_) {
                case UsingType::Base:  return this;
                case UsingType::Param: return paraminfo().base_;
                default: std::unreachable();
            }
        }
        const std::vector<const Type*>* params() const {
            switch(usingtype_) {
                case UsingType::Base:  return nullptr;
                case UsingType::Param: return &paraminfo().params_;
                default: std::unreachable();
            }
        }
        const rt::TypeImpl*             impl() const {
            if (impl_) return impl_;
            return base()->impl();
        }

        bool isNone() const;
        bool is(std::string_view name) const;

        void ImplSet(const rt::TypeImpl* impl) {
            impl_ = impl;
        }

        static void        BaseTypeCheck(const Type* base) {
            if (base->usingtype_ != UsingType::Base) {
                throw LogErr(LogModule::Sema, std::format(
                    "invalid base type '{}'", base->name_
                ));
            }
        }
        // Print type name with params: "array<i32, f64>"
        static std::string ParamsPrint(const Type* base, const std::vector<const Type*>& params) {
            BaseTypeCheck(base);

            std::string res(base->name_);
            res += "[=";
            for (size_t i = 0; i < params.size(); i++) {
                if (i != 0) res += ", ";
                res += params[i]->name_;
            }
            res += "=]";
            return res;
        }
    };

    class TypeTable {
    private:
        static inline std::unordered_map<std::string, Type*>        table_;
        static inline std::multimap<const Type*, const Type*>       converts_;
        static inline std::map<std::set<const Type*>, const Type*>  common_cache_;

    public:
        static void Init();

        static Type*       Set(const Type& t) {
            if (!table_.contains(std::string(t.name_))) {
                auto type = new Type(t);
                table_.emplace(t.name_, type);
                return type;
            }
            else throw LogErr(LogModule::Sema, std::format("existing type '{}'", t.name_));
        }
        static Type*       Lookup(std::string_view name, std::optional<Loc> loc = std::nullopt) {
            auto type_it = table_.find(std::string(name));
            if (type_it != table_.end()) {
                return type_it->second;
            }

            throw LogErr(LogModule::Sema, std::format("undefined type '{}'", name), loc);
        }
        static Type*       SetOrGetTypeParam(
            const Type* base, const std::vector<const Type*>& params,
            std::optional<Loc> loc = std::nullopt)
        {
            Type::BaseTypeCheck(base);
            if (params.empty()) return Lookup(base->name_, loc);
            if (base->baseinfo().params_cnt_ != params.size()) {
                throw LogErr(LogModule::Sema, std::format(
                    "type '{}' expects {} type parameter(s), got {}",
                    base->name_, base->baseinfo().params_cnt_, params.size()
                ), loc);
            }
            
            auto name = Type::ParamsPrint(base, params);
            auto it   = table_.find(name);
            if (it != table_.end()) return it->second;

            auto type = Set(Type(
                name, Type::ParamInfo{
                    .base_   = base,
                    .params_ = params
                }
            ));
            ConvertsRecompute();
            return type;
        }

        static void        ConvertSet(const Type* from, const Type* to);
        static void        ConvertsRecompute();
        
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
                        common = t->converts_;
                        continue;
                    }

                    std::set<const Type*> temp;
                    std::set_intersection(
                        common.begin(), common.end(),
                        t->converts_.begin(), t->converts_.end(),
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
                    if (j->converts_.contains(i)) {
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

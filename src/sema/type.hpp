
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

namespace xcompiler {
    class TypeGen;
}

namespace xengine {
    class TypeImpl;
}

namespace sema {

    class Type {
    private:
        const xengine::TypeImpl*     impl_ = nullptr;
        const xcompiler::TypeGen* gen_  = nullptr;
    
    public:
        enum class UsingType {
            Base, Param
        };

        struct BaseInfo {
            size_t size_         = 0;           // Byte width
            bool   isHeapStored_ = false;       // Stored Data in Heap
            size_t params_cnt_   = 0;           // Number of Type Parameters
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

        // Common

        bool isNone() const;
        bool is(std::string_view name) const;
        bool isHeapStored() const;
        
        const Type*             base() const {
            switch(usingtype_) {
                case UsingType::Base:  return this;
                case UsingType::Param: return paraminfo().base_;
                default: std::unreachable();
            }
        }
        const xengine::TypeImpl*     impl() const {
            if (impl_) return impl_;
            return base()->impl();
        }
        const xcompiler::TypeGen* gen()  const {
            return gen_;
        }

        // Link
        void ImplSet(const xengine::TypeImpl* impl)   { impl_ = impl; }
        void GenSet(const xcompiler::TypeGen* gen) { gen_ = gen; }
    
        // Base
        void BaseTypeCheck() const;

        // Param

        const std::vector<const Type*>* params() const {
            switch(usingtype_) {
                case UsingType::Base:  return nullptr;
                case UsingType::Param: return &paraminfo().params_;
                default: std::unreachable();
            }
        }

        // Print type name with params: "array[=i32, f64=]"
        static std::string ParamsPrint(const Type* base, const std::vector<const Type*>& params);
    };

    class TypeTable {
    private:
        static inline std::unordered_map<std::string, Type*>        table_;
        static inline std::multimap<const Type*, const Type*>       converts_;
        static inline std::map<std::set<const Type*>, const Type*>  common_cache_;

    public:
        static void Init();

        static Type* Set(const Type& t);
        static Type* Lookup(std::string_view name, std::optional<Loc> loc = std::nullopt);
        static Type* SetOrGetTypeParam(
            const Type* base, const std::vector<const Type*>& params,
            std::optional<Loc> loc = std::nullopt);

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

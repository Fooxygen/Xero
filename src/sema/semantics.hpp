
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstddef>
#include <string>
#include <set>
#include <map>
#include <unordered_map>

#include "common/log.hpp"

namespace rt {
    class TypeImpl;
}

namespace sema {

    class Type {
    public:
        std::string_view        name  = "";
        size_t                  size  = 0;            // Byte width
        bool                    isHeapStored = false; // Store Data in Heap
        std::set<const Type*>   converts = {};        // List of convertible types

        const rt::TypeImpl*     impl_ = nullptr;

        bool isNone() const;
        bool is(std::string_view name) const;
    };

    class TypeTable {
    private:
        static inline std::unordered_map<std::string, Type*>        table_;
        static inline std::multimap<const Type*, const Type*>       converts_;
        static inline std::map<std::set<const Type*>, const Type*>  common_cache_;

    public:
        static void Init();

        static void        Set(const Type& t) {
            if (!table_.contains(std::string(t.name))) {
                table_.emplace(t.name, new Type(t));
            }
            else throw LogErr(LogModule::Sema, std::format("existing type '{}'", t.name));
        }
        static Type*       Get(std::string_view name) {
            auto type_it = table_.find(std::string(name));
            if (type_it != table_.end()) {
                return type_it->second;
            }

            throw LogErr(LogModule::Sema, std::format("undefined type '{}'", name));
            return nullptr;
        }
    
        static void        ConvertSet(const Type* from, const Type* to);
        static void        ConvertsRecompute();
        
        static const Type* Common(std::set<const Type*> ts) {
            
            // Search Cache
            if (common_cache_.contains(ts)) return common_cache_[ts];

            // Get Common
            std::set<const Type*> common;
            {
                bool isFirstAdd = false;
                for (auto t : ts) {
                    if (!isFirstAdd) {
                        isFirstAdd = true;
                        common = t->converts;
                        continue;
                    }

                    std::set<const Type*> temp;
                    std::set_intersection(
                        common.begin(), common.end(),
                        t->converts.begin(), t->converts.end(),
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
                    if (j->converts.contains(i)) {
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

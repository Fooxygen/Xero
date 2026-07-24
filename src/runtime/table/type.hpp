
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <algorithm>
#include <string>
#include <set>
#include <unordered_map>
#include <map>
#include <vector>

#include "log.hpp"

namespace rt {
    class Obj;

    struct Type {
        std::string_view      name  = "";
        size_t                size  = 0;        // Byte width
        bool                  isRef = false;    // Reference Type
        std::set<const Type*> converts = {};    // List of convertible types

        // Built-in Method

        static Obj         methdef_clone(const Obj&);
        static void        methdef_destroy(void*);
        static std::string methdef_to_string(const Obj&);
        static void        methdef_assign(Obj*, const Obj&);

        Obj         (*clone_)(const Obj&)        = methdef_clone;
        void        (*destroy_)(void*)           = methdef_destroy;
        std::string (*to_string_)(const Obj&)    = methdef_to_string;
        void        (*assign_)(Obj*, const Obj&) = methdef_assign;

        // Arith Oper

        Obj (*plus_)  (const Obj&, const Obj&)   = nullptr;     // +
        Obj (*minus_) (const Obj&, const Obj&)   = nullptr;     // -
        Obj (*star_)  (const Obj&, const Obj&)   = nullptr;     // *
        Obj (*slash_) (const Obj&, const Obj&)   = nullptr;     // /
        Obj (*neg_)   (const Obj&)               = nullptr;     // -
        
        // Relation Oper

        Obj (*gt_)   (const Obj&, const Obj&)    = nullptr;     // >
        Obj (*lt_)   (const Obj&, const Obj&)    = nullptr;     // <
        Obj (*ge_)   (const Obj&, const Obj&)    = nullptr;     // >=
        Obj (*le_)   (const Obj&, const Obj&)    = nullptr;     // <=
        Obj (*eq_)   (const Obj&, const Obj&)    = nullptr;     // ==
        Obj (*neq_)  (const Obj&, const Obj&)    = nullptr;     // !=

        // Logical Oper

        Obj (*and_) (const Obj&, const Obj&)    = nullptr;      // &&
        Obj (*or_)  (const Obj&, const Obj&)    = nullptr;      // ||
        Obj (*not_) (const Obj&)                = nullptr;      // !

        // Container Oper

        Obj (*pick_clone_)(const Obj&, const Obj&) = nullptr;   // []
    };

    class TypeTable {
    private:
        static inline std::unordered_map<std::string, Type*> table_;
        static inline std::map<
            std::pair<const Type*, const Type*>, Obj(*)(const Obj&)
        > converts_;
        static inline std::map<std::set<const Type*>, const Type*> common_cache_;

    public:
        static void Reset() {
            table_ = std::unordered_map<std::string, Type*>();
            converts_.clear();
            common_cache_.clear();
        }

        static void ConvertSet(const Type* from, const Type* to, Obj(*fn)(const Obj&));
        static Obj  Convert(const Obj& obj, const Type* type);
        static void ConvertsRecompute();

        static void        Set(const Type& t) {
            if (!table_.contains(std::string(t.name))) {
                table_.emplace(t.name, new Type(t));
            }
            else throw LogErr(LogModule::Runtime, std::format("existing type '{}'", t.name));
        }
        static const Type* Get(std::string_view name) {
            auto type_it = table_.find(std::string(name));
            if (type_it != table_.end()) {
                return type_it->second;
            }

            throw LogErr(LogModule::Runtime, std::format("undefined type '{}'", name));
            return nullptr;
        }
    
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

                    if (common.empty()) return nullptr;
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

                if (isFind) return i;
            }

            return nullptr;
        }
    };
}

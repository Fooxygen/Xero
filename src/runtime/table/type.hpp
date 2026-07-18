
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <algorithm>
#include <string>
#include <set>
#include <unordered_map>
#include <map>
#include <ranges>
#include <vector>

#include "log.hpp"

namespace rt {
    class Obj;

    struct Type {
        std::string_view      name  = "";
        size_t                size  = 0;        // Byte width
        bool                  isRef = false;    // Reference Type
        std::set<std::string> reach = {};       // Collection of promoted types

        // Methods

        static void        destroy_default(void*) {
            throw LogErr(LogModule::Runtime, "method 'destroy()' not implemented for type");
        }
        static std::string to_string_default(const Obj&) {
            throw LogErr(LogModule::Runtime, "method 'to_string()' not implemented for type");
        }

        void        (*destroy)(void*)           = destroy_default;
        std::string (*to_string)(const Obj&)    = to_string_default;

        // ArithOper

        Obj (*plus)  (const Obj&, const Obj&)   = nullptr;
        Obj (*minus) (const Obj&, const Obj&)   = nullptr;
        Obj (*star)  (const Obj&, const Obj&)   = nullptr;
        Obj (*slash) (const Obj&, const Obj&)   = nullptr;
        Obj (*neg)   (const Obj&)               = nullptr;
        
        // RelationOper

        Obj (*gt)   (const Obj&, const Obj&)    = nullptr;
        Obj (*lt)   (const Obj&, const Obj&)    = nullptr;
        Obj (*ge)   (const Obj&, const Obj&)    = nullptr;
        Obj (*le)   (const Obj&, const Obj&)    = nullptr;
        Obj (*eq)   (const Obj&, const Obj&)    = nullptr;
        Obj (*neq)  (const Obj&, const Obj&)    = nullptr;

        // LogicalOper

        Obj (*and_) (const Obj&, const Obj&)    = nullptr;
        Obj (*or_)  (const Obj&, const Obj&)    = nullptr;
        Obj (*not_) (const Obj&)                = nullptr;
    };

    class TypeTable {
    private:
        static inline std::unordered_map<std::string, Type*> table_;
        static inline std::map<
            std::pair<const Type*, const Type*>, Obj(*)(const Obj&)
        > converts_;

        static void TypeTable::ReachRecompute() {

            // Clear
            for (auto& [name, type] : table_) {
                type->reach.clear();
            }

            // Recompute
            for (auto& [name, type] : table_) {

                // Self
                type->reach.emplace(std::string(name));

                // Search
                std::vector<const Type*> stack = { type };
                while (!stack.empty()) {
                    auto* cur = stack.back(); stack.pop_back();
                    for (auto& [edge, fn] : converts_) {
                        if (edge.first == cur) {
                            type->reach.emplace(std::string(edge.second->name));
                            stack.push_back(edge.second);
                        }
                    }
                }
            }
        }

    public:
        static void Reset() {
            table_ = std::unordered_map<std::string, Type*>();
            converts_.clear();
        }

        static void ConvertSet(const Type* from, const Type* to, Obj(*fn)(const Obj&)) {
            converts_[{from, to}] = fn;
            ReachRecompute();
        }
        static Obj  Convert(const Obj& obj, const Type* target) {
            if (obj.type() == target) return obj;

            auto it = converts_.find({obj.type(), target});
            if (it != converts_.end())
                return it->second(obj);
            return Obj();
        }

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
    
        static const Type* Common(std::set<const Type*> input) {
            
            // Common
            std::set<std::string> common;
            {
                bool isFirstAdd = false;
                for (auto i : input) {
                    if (!isFirstAdd) {
                        isFirstAdd = true;
                        common = i->reach;
                        common.emplace(i->name);
                        continue;
                    }

                    std::set<std::string> reach = i->reach;
                    reach.emplace(i->name);

                    std::set<std::string> temp;
                    std::set_intersection(
                        common.begin(), common.end(),
                        reach.begin(), reach.end(),
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
                    if (Get(j)->reach.contains(i)) {
                        isFind = false;
                        break;
                    }
                }

                if (isFind) return Get(i);
            }

            return nullptr;
        }
    };
}

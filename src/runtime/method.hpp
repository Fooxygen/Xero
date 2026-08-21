
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <unordered_map>

#include "sema/type.hpp"
#include "runtime/obj/impl/function.hpp"

namespace rt {

    class MethodImplTable {
    private:
        struct Key {
            const sema::Type* type_;
            std::string       name_;

            bool operator==(const Key& other) const {
                return type_ == other.type_ && name_ == other.name_;
            }
        };

        struct KeyHash {
            size_t operator()(const Key& key) const {
                return std::hash<const sema::Type*>{}(key.type_) ^
                       std::hash<std::string>{}(key.name_);
            }
        };

        static inline std::unordered_map<Key, Function, KeyHash> table_;

    public:
        static void BuiltinImplRegister();

        static void      Set(const sema::Type* type, const std::string& name, Function fn) {
            table_.insert_or_assign(Key{type, name}, std::move(fn));
        }
        static Function* Lookup(const sema::Type* type, const std::string& name) {
            auto it = table_.find({type, name});
            return it == table_.end() ? nullptr : &it->second;
        }
    };
}

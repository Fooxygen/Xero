
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>

#include "sema/type.hpp"
#include "sema/symbol.hpp"

namespace sema {

    class MethodTable {
    private:
        struct Key {
            const Type* type = nullptr;
            std::string name = "";

            bool operator==(const Key& other) const {
                return type == other.type && name == other.name;
            }
        };

        struct KeyHash {
            size_t operator()(const Key& key) const {
                return std::hash<const Type*>{}(key.type) ^
                       std::hash<std::string>{}(key.name);
            }
        };

        static inline std::unordered_map<Key, FnSymbol, KeyHash> table_;

    public:
        static void BuiltinRegister();

        static void      Set(const Type* type, FnSymbol fn) {
            table_.insert_or_assign(Key{ type, fn.name_ }, std::move(fn));
        }
        static FnSymbol* Lookup(const Type* type, const std::string& name) {
            auto it = table_.find({type, name});
            return it == table_.end() ? nullptr : &it->second;
        }
    };
}

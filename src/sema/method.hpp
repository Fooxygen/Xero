
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <unordered_map>

#include "sema/type.hpp"
#include "sema/symbol.hpp"

namespace sema {

    class MethodTable {
    private:
        struct Key {
            const Type* type_ = nullptr;
            std::string name_ = "";

            bool operator==(const Key& other) const {
                return type_ == other.type_ && name_ == other.name_;
            }
        };

        struct KeyHash {
            size_t operator()(const Key& key) const {
                return std::hash<const Type*>{}(key.type_) ^
                       std::hash<std::string>{}(key.name_);
            }
        };

        static inline std::unordered_map<Key, FnSymbol, KeyHash> table_;

    public:
        static void Register();

        static void      Set(const Type* type, FnSymbol fn) {
            table_.insert_or_assign(Key{ type, fn.name_ }, std::move(fn));
        }
        static FnSymbol* Lookup(const Type* type, const std::string& name) {
            auto it = table_.find({type, name});
            return it == table_.end() ? nullptr : &it->second;
        }
    };
}

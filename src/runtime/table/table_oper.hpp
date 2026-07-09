
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "common/token.hpp"
#include "runtime/obj.hpp"

namespace rt {

    using OperFunc = Obj(*)(const Obj&, const Obj&);
        
    class OperTable {
    private:
        struct Key {
            const Type* type;
            Token::Type oper;

            bool operator ==(const Key&) const = default;
            struct Hash {
                size_t operator()(const Key& key) const {
                    return
                        std::hash<const Type*>{}(key.type) ^ 
                        std::hash<int>{}((int)key.oper);
                }
            };
        };

        static inline std::unordered_map<Key, OperFunc, Key::Hash> table_;

    public:
        static void Reset() {
            table_ = std::unordered_map<Key, OperFunc, Key::Hash>();
        }

        static void Set(const Type* type, Token::Type oper, OperFunc func) {
            table_[{type, oper}] = func;
        }

        static OperFunc Get(const Type* type, Token::Type oper) {
            auto it = table_.find({type, oper});
            return it != table_.end() ? it->second : nullptr;
        }
    };
}

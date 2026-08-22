
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <map>
#include <unordered_map>

#include "common/log.hpp"
#include "sema/type.hpp"

namespace rt {
    class Obj;

    class TypeImpl {
    public:
        // Basic

        Obj         (*clone_)(const Obj&)        = nullptr;
        void        (*destroy_)(void*)           = nullptr;
        std::string (*to_string_)(const Obj&)    = nullptr;
        void        (*assign_)(Obj*, const Obj&) = nullptr;

        // Arith Oper

        Obj (*plus_)  (const Obj&, const Obj&)   = nullptr;     // +
        Obj (*minus_) (const Obj&, const Obj&)   = nullptr;     // - (binary)
        Obj (*star_)  (const Obj&, const Obj&)   = nullptr;     // *
        Obj (*slash_) (const Obj&, const Obj&)   = nullptr;     // /
        Obj (*neg_)   (const Obj&)               = nullptr;     // - (unary)
        Obj (*modt_)  (const Obj&, const Obj&)   = nullptr;     // %
        Obj (*modf_)  (const Obj&, const Obj&)   = nullptr;     // %%
        
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

        Obj (*pick_) (const Obj&, const Obj&)   = nullptr;      // []
    };

    class TypeImplTable {
    private:
        static inline std::unordered_map<const sema::Type*, TypeImpl*> table_;
        static inline std::map<
            std::pair<const sema::Type*, const sema::Type*>, Obj(*)(const Obj&)
        > converts_;

    public:
        static void Init();

        static Obj  Convert(const Obj& obj, const sema::Type* type);
        static void ConvertSet(const sema::Type* from, const sema::Type* to, Obj(*fn)(const Obj&)) {
            converts_[{ from, to }] = fn;
        }

        static void Set(sema::Type* type, const TypeImpl& type_impl) {
            if (!table_.contains(type)) {
                auto impl = new TypeImpl(type_impl);
                table_.emplace(type, impl);
                type->ImplSet(impl);
            }
            else throw LogErr(LogModule::Runtime, std::format(
                "existing type '{}'", type->name_
            ));
        }
    };
}

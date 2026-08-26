
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "llvm/IR/Value.h"

#include "sema/type.hpp"

namespace xcompiler {
    class IRGen;
    
    class TypeGen {
    public:
        // Arith Oper

        llvm::Value* (*plus_)   (IRGen&, llvm::Value*, llvm::Value*) = nullptr; // +
        llvm::Value* (*minus_)  (IRGen&, llvm::Value*, llvm::Value*) = nullptr; // - (binary)
        llvm::Value* (*star_)   (IRGen&, llvm::Value*, llvm::Value*) = nullptr; // *
        llvm::Value* (*slash_)  (IRGen&, llvm::Value*, llvm::Value*) = nullptr; // /
        llvm::Value* (*neg_)    (IRGen&, llvm::Value*)               = nullptr; // - (unary)
        llvm::Value* (*modt_)   (IRGen&, llvm::Value*, llvm::Value*) = nullptr; // %
        llvm::Value* (*modf_)   (IRGen&, llvm::Value*, llvm::Value*) = nullptr; // %%
        
        // Relation Oper

        llvm::Value* (*gt_)     (IRGen&, llvm::Value*, llvm::Value*)  = nullptr; // >
        llvm::Value* (*lt_)     (IRGen&, llvm::Value*, llvm::Value*)  = nullptr; // <
        llvm::Value* (*ge_)     (IRGen&, llvm::Value*, llvm::Value*)  = nullptr; // >=
        llvm::Value* (*le_)     (IRGen&, llvm::Value*, llvm::Value*)  = nullptr; // <=
        llvm::Value* (*eq_)     (IRGen&, llvm::Value*, llvm::Value*)  = nullptr; // ==
        llvm::Value* (*neq_)    (IRGen&, llvm::Value*, llvm::Value*)  = nullptr; // !=

        // Logical Oper

        llvm::Value* (*and_)    (IRGen&, llvm::Value*, llvm::Value*)  = nullptr; // &&
        llvm::Value* (*or_)     (IRGen&, llvm::Value*, llvm::Value*)  = nullptr; // ||
        llvm::Value* (*not_)    (IRGen&, llvm::Value*)                = nullptr; // !
    };

    class TypeGenTable {
    private:
        static inline std::unordered_map<const sema::Type*, TypeGen*> table_;
        static inline std::map<
            std::pair<const sema::Type*, const sema::Type*>,
            llvm::Value*(*)(IRGen&, llvm::Value*)
        > casts_;

    public:
        static void Init();

        static void Set(sema::Type* type, const TypeGen& type_gen) {
            if (!table_.contains(type)) {
                auto gen = new TypeGen(type_gen);
                table_.emplace(type, gen);
                type->GenSet(gen);
            }
            else throw LogErr(LogModule::Xcompiler, std::format(
                "existing type '{}'", type->name_
            ));
        }
    
        static llvm::Value* Cast(
            IRGen& gen, llvm::Value* value,
            const sema::Type* from, const sema::Type* to
        );
        static void         CastSet(
            const sema::Type* from, const sema::Type* to,
            llvm::Value*(*fn)(IRGen&, llvm::Value*))
        {
            casts_[{ from, to }] = fn;
        }
    };
}

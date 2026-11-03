
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "llvm/IR/Constants.h"

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    void TypeImplTable::Init_struct(sema::StructType* type) {
        using ARGS  = const std::vector<Arg>&;

        auto  none_ = sema::TypeTable::Lookup("none");
        auto  impl  = TypeImplTable::Set(TypeImpl(type));

        // @copy and @release

        impl->MethodAdd("@copy", [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder   = gen.llvm_builder();
            auto* type      = (sema::StructType*)args[0].type()->ReferenceUnwrap();
            auto  llvm_type = gen.StructTypeGet(type);
            auto  addr      = gen.ArgAddr(args[0]);
            auto  value     = (llvm::Value*)llvm::PoisonValue::get(llvm_type);

            for (auto& field : type->fields()) {
                auto field_type = field.type_;
                auto field_addr = builder.CreateStructGEP(llvm_type, addr, field.index_);

                // Reference Type
                if (dynamic_cast<sema::ReferenceType*>(field_type)) {
                    value = builder.CreateInsertValue(
                        value,
                        builder.CreateLoad(gen.LlvmType(field_type), field_addr),
                        field.index_
                    );
                }

                // Non-Reference Type                
                else {
                    auto field_val = TypeImplTable::Lookup(field_type)->MethodCall(gen, "@copy", {
                        Arg(field_addr, sema::TypeTable::ReferenceTypeGet(field_type))
                    });
                    value = builder.CreateInsertValue(value, field_val, field.index_);
                }
            }

            return value;
        }, sema::FnSign(type));
        impl->MethodAdd("@release", [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder   = gen.llvm_builder();
            auto* type      = (sema::StructType*)args[0].type()->ReferenceUnwrap();
            auto  llvm_type = gen.StructTypeGet(type);
            auto  addr      = gen.ArgAddr(args[0]);

            for (auto it = type->fields().rbegin(); it != type->fields().rend(); ++it) {
                if (dynamic_cast<sema::ReferenceType*>(it->type_)) continue;

                auto field_addr = builder.CreateStructGEP(llvm_type, addr, it->index_);
                TypeImplTable::Lookup(it->type_)->MethodCall(gen, "@release", {
                    Arg(field_addr, sema::TypeTable::ReferenceTypeGet(it->type_))
                });
            }

            return nullptr;
        }, sema::FnSign(none_));
    }
}

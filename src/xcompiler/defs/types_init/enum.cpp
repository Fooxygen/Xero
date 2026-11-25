
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <vector>

#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/GlobalVariable.h"

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    void TypeImplTable::Init_enum(sema::EnumType* type) {
        using ARGS  = const std::vector<Arg>&;

        auto  none_ = sema::TypeTable::Lookup("none");
        auto  bool_ = sema::TypeTable::Lookup("bool");
        auto  i32_  = sema::TypeTable::Lookup("i32");
        auto  i64_  = sema::TypeTable::Lookup("i64");

        auto  impl  = TypeImplTable::Set(TypeImpl(type));

        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) {
            return gen.ArgValGet(args[0]);
        }, sema::FnSign(type));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // @cast

        impl->MethodAdd("@cast",    [](IRGen& gen, ARGS& args) {
            return gen.ArgValGet(args[0]);
        }, sema::FnSign(i32_, {}, std::nullopt, sema::FnModifier::CastLossy));
        impl->MethodAdd("@cast",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateZExt(
                gen.ArgValGet(args[0]), gen.llvm_builder().getInt64Ty()
            );
        }, sema::FnSign(i64_, {}, std::nullopt, sema::FnModifier::CastLossy));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder    = gen.llvm_builder();

            auto* enum_type     = (sema::EnumType*)args[0].type()->ReferenceUnwrap();
            auto& enum_variants = enum_type->variants();
            auto* ptr_type   = llvm::PointerType::get(gen.llvm_context(), 0);
            auto* table_type = llvm::ArrayType::get(ptr_type, enum_variants.size());

            std::string table_name = ".enum.names." + enum_type->name();
            auto* table = gen.llvm_module()->getNamedGlobal(table_name);
            if (!table) {
                std::vector<llvm::Constant*> elems = {};
                elems.reserve(enum_variants.size());
                for (const auto& variant : enum_variants) {
                    elems.emplace_back(builder.CreateGlobalString(
                        enum_type->name() + "::" + variant.name_,
                        ".enum.name." + enum_type->name() + "." + variant.name_
                    ));
                }

                auto* init = llvm::ConstantArray::get(table_type, elems);
                table = new llvm::GlobalVariable(
                    *gen.llvm_module(),
                    table_type,
                    true,
                    llvm::GlobalValue::PrivateLinkage,
                    init,
                    table_name
                );
            }

            auto* idx      = builder.CreateZExt(gen.ArgValGet(args[0]), builder.getInt64Ty());
            auto* elem_ptr = builder.CreateInBoundsGEP(table_type, table, { builder.getInt64(0), idx });
            auto* str_ptr  = builder.CreateLoad(ptr_type, elem_ptr);
            auto* str_fmt  = builder.CreateGlobalString("%s", ".fmt.str");
            builder.CreateCall(LibC_printf(gen), { str_fmt, str_ptr });
            return nullptr;
        }, sema::FnSign(none_));

        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpEQ(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { type }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpNE(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { type }));
    }
}

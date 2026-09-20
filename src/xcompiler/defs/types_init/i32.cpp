
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    void TypeImplTable::Init_i32() {
        using ARGS  = const std::vector<Arg>&;

        auto  none_ = sema::TypeTable::Lookup("none");
        auto  bool_ = sema::TypeTable::Lookup("bool");
        auto  i32_  = sema::TypeTable::Lookup("i32");
        auto  i64_  = sema::TypeTable::Lookup("i64");
        auto  f32_  = sema::TypeTable::Lookup("f32");
        auto  f64_  = sema::TypeTable::Lookup("f64");

        auto  impl  = TypeImplTable::Set(TypeImpl(i32_));

        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(i32_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // @cast

        impl->MethodAdd("@cast",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateSExt(gen.ArgLoad(args[0]), gen.llvm_builder().getInt64Ty());
        }, sema::FnSign(i64_, {}, std::nullopt, sema::FnModifier::Cast));
        impl->MethodAdd("@cast",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateSIToFP(gen.ArgLoad(args[0]), gen.llvm_builder().getFloatTy());
        }, sema::FnSign(f32_, {}, std::nullopt, sema::FnModifier::Cast));
        impl->MethodAdd("@cast",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateSIToFP(gen.ArgLoad(args[0]), gen.llvm_builder().getDoubleTy());
        }, sema::FnSign(f64_, {}, std::nullopt, sema::FnModifier::Cast));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str_fmt = builder.CreateGlobalString("%d", ".fmt.i32");
            builder.CreateCall(LibC_printf(gen), { str_fmt, gen.ArgLoad(args[0]) });
            return nullptr;
        }, sema::FnSign(none_));
        
        impl->MethodAdd("@plus",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateAdd(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(i32_,  { i32_ }));
        impl->MethodAdd("@minus",   [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateSub(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(i32_,  { i32_ }));
        impl->MethodAdd("@star",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateMul(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(i32_,  { i32_ }));
        impl->MethodAdd("@slash",   [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateSDiv(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(i32_,  { i32_ }));
        impl->MethodAdd("@neg",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateNeg(gen.ArgLoad(args[0]));
        }, sema::FnSign(i32_));
        impl->MethodAdd("@modt",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateSRem(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(i32_,  { i32_ }));
        impl->MethodAdd("@modf",    [](IRGen& gen, ARGS& args) {
            auto& builder = gen.llvm_builder();
            auto  zero    = builder.getInt32(0);
            auto  lval    = gen.ArgLoad(args[0]);
            auto  rval    = gen.ArgLoad(args[1]);
            auto  modt    = builder.CreateSRem(lval, rval);
            auto  isModTNeqZero = builder.CreateICmpNE(modt, zero);
            auto  isDiffSign    = builder.CreateXor(builder.CreateICmpSLT(modt, zero), builder.CreateICmpSLT(rval, zero));
            auto  hasRevise     = builder.CreateAnd(isModTNeqZero, isDiffSign);
            return builder.CreateAdd(modt, builder.CreateSelect(hasRevise, rval, zero));
        }, sema::FnSign(i32_,  { i32_ }));
        
        impl->MethodAdd("@gt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSGT(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i32_ }));
        impl->MethodAdd("@lt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSLT(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i32_ }));
        impl->MethodAdd("@ge",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSGE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i32_ }));
        impl->MethodAdd("@le",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSLE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i32_ }));
        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpEQ(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i32_ }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpNE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i32_ }));
    }
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    void TypeImplTable::Init_f64() {
        using ARGS  = const std::vector<Arg>&;

        auto  none_ = sema::TypeTable::Lookup("none");
        auto  bool_ = sema::TypeTable::Lookup("bool");
        auto  f64_  = sema::TypeTable::Lookup("f64");

        auto  impl  = TypeImplTable::Set(TypeImpl(f64_));

        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(f64_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str_fmt = builder.CreateGlobalString("%f", ".fmt.f64");
            builder.CreateCall(LibC_printf(gen), { str_fmt, gen.ArgLoad(args[0]) });
            return nullptr;
        }, sema::FnSign(none_));

        impl->MethodAdd("@plus",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFAdd(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(f64_,  { f64_ }));
        impl->MethodAdd("@minus",   [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFSub(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(f64_,  { f64_ }));
        impl->MethodAdd("@star",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFMul(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(f64_,  { f64_ }));
        impl->MethodAdd("@slash",   [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFDiv(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(f64_,  { f64_ }));
        impl->MethodAdd("@neg",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFNeg(gen.ArgLoad(args[0]));
        }, sema::FnSign(f64_));
        impl->MethodAdd("@modt",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFRem(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(f64_,  { f64_ }));
        impl->MethodAdd("@modf",    [](IRGen& gen, ARGS& args) {
            auto& builder = gen.llvm_builder();
            auto  zero    = llvm::ConstantFP::get(builder.getDoubleTy(), 0.0);
            auto  lval    = gen.ArgLoad(args[0]);
            auto  rval    = gen.ArgLoad(args[1]);
            auto  modt    = builder.CreateFRem(lval, rval);
            auto  isModTNeqZero = builder.CreateFCmpONE(modt, zero);
            auto  isDiffSign    = builder.CreateXor(builder.CreateFCmpOLT(modt, zero), builder.CreateFCmpOLT(rval, zero));
            auto  hasRevise     = builder.CreateAnd(isModTNeqZero, isDiffSign);
            return builder.CreateFAdd(modt, builder.CreateSelect(hasRevise, rval, zero));
        }, sema::FnSign(f64_,  { f64_ }));
        
        impl->MethodAdd("@gt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpOGT(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f64_ }));
        impl->MethodAdd("@lt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpOLT(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f64_ }));
        impl->MethodAdd("@ge",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpOGE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f64_ }));
        impl->MethodAdd("@le",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpOLE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f64_ }));
        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpOEQ(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f64_ }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpONE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f64_ }));
    }
}

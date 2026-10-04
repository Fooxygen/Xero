
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    // Utility

    namespace {
        llvm::Value* Modf(IRGen& gen, const std::vector<Arg>& args) {
            auto& builder = gen.llvm_builder();
            auto  lval    = gen.ArgLoad(args[0]);
            auto  rval    = gen.ArgLoad(args[1]);

            // f32, f64
            if (lval->getType()->isFloatingPointTy()) {
                auto zero = llvm::ConstantFP::get(lval->getType(), 0.0);
                auto modt = builder.CreateFRem(lval, rval);
                auto is_modt_neq_zero = builder.CreateFCmpONE(modt, zero);
                auto is_sign_diff     = builder.CreateXor(
                    builder.CreateFCmpOLT(modt, zero), builder.CreateFCmpOLT(rval, zero)
                );
                auto has_revise = builder.CreateAnd(is_modt_neq_zero, is_sign_diff);
                return builder.CreateFAdd(modt, builder.CreateSelect(has_revise, rval, zero));
            }
            
            // i32, i64
            else {
                auto zero = llvm::ConstantInt::get(lval->getType(), 0);
                auto modt = builder.CreateSRem(lval, rval);
                auto is_modt_neq_zero = builder.CreateICmpNE(modt, zero);
                auto is_sign_diff     = builder.CreateXor(
                    builder.CreateICmpSLT(modt, zero), builder.CreateICmpSLT(rval, zero)
                );
                auto has_revise = builder.CreateAnd(is_modt_neq_zero, is_sign_diff);
                return builder.CreateAdd(modt, builder.CreateSelect(has_revise, rval, zero));
            }
        }
    }

    // i32

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

        impl->MethodAdd("@copy",        [](IRGen& gen, ARGS& args) {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(i32_));
        impl->MethodAdd("@release",     [](IRGen&, ARGS&) -> llvm::Value* {
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
            return Modf(gen, args);
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

    // i64

    void TypeImplTable::Init_i64() {
        using ARGS  = const std::vector<Arg>&;

        auto  none_ = sema::TypeTable::Lookup("none");
        auto  bool_ = sema::TypeTable::Lookup("bool");
        auto  i64_  = sema::TypeTable::Lookup("i64");
        auto  f32_  = sema::TypeTable::Lookup("f32");
        auto  f64_  = sema::TypeTable::Lookup("f64");

        auto  impl  = TypeImplTable::Set(TypeImpl(i64_));

        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(i64_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // @cast

        impl->MethodAdd("@cast",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateSIToFP(gen.ArgLoad(args[0]), gen.llvm_builder().getFloatTy());
        }, sema::FnSign(f32_, {}, std::nullopt, sema::FnModifier::Cast));
        impl->MethodAdd("@cast",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateSIToFP(gen.ArgLoad(args[0]), gen.llvm_builder().getDoubleTy());
        }, sema::FnSign(f64_, {}, std::nullopt, sema::FnModifier::Cast));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str_fmt = builder.CreateGlobalString("%lld", ".fmt.i64");
            builder.CreateCall(LibC_printf(gen), { str_fmt, gen.ArgLoad(args[0]) });
            return nullptr;
        }, sema::FnSign(none_));
        
        impl->MethodAdd("@plus",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateAdd(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(i64_,  { i64_ }));
        impl->MethodAdd("@minus",   [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateSub(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(i64_,  { i64_ }));
        impl->MethodAdd("@star",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateMul(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(i64_,  { i64_ }));
        impl->MethodAdd("@slash",   [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateSDiv(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(i64_,  { i64_ }));
        impl->MethodAdd("@neg",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateNeg(gen.ArgLoad(args[0]));
        }, sema::FnSign(i64_));
        impl->MethodAdd("@modt",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateSRem(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(i64_,  { i64_ }));
        impl->MethodAdd("@modf",    [](IRGen& gen, ARGS& args) {
            return Modf(gen, args);
        }, sema::FnSign(i64_,  { i64_ }));
        
        impl->MethodAdd("@gt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSGT(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i64_ }));
        impl->MethodAdd("@lt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSLT(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i64_ }));
        impl->MethodAdd("@ge",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSGE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i64_ }));
        impl->MethodAdd("@le",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSLE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i64_ }));
        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpEQ(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i64_ }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpNE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { i64_ }));
    }

    // f32

    void TypeImplTable::Init_f32() {
        using ARGS  = const std::vector<Arg>&;

        auto  none_ = sema::TypeTable::Lookup("none");
        auto  bool_ = sema::TypeTable::Lookup("bool");
        auto  f32_  = sema::TypeTable::Lookup("f32");
        auto  f64_  = sema::TypeTable::Lookup("f64");

        auto  impl  = TypeImplTable::Set(TypeImpl(f32_));

        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(f32_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // @cast

        impl->MethodAdd("@cast",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFPExt(gen.ArgLoad(args[0]), gen.llvm_builder().getDoubleTy());
        }, sema::FnSign(f64_, {}, std::nullopt, sema::FnModifier::Cast));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str_fmt = builder.CreateGlobalString("%f", ".fmt.f32");
            builder.CreateCall(LibC_printf(gen), {
                str_fmt,
                builder.CreateFPExt(gen.ArgLoad(args[0]), builder.getDoubleTy())
            });
            return nullptr;
        }, sema::FnSign(none_));
    
        impl->MethodAdd("@plus",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFAdd(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(f32_,  { f32_ }));
        impl->MethodAdd("@minus",   [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFSub(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(f32_,  { f32_ }));
        impl->MethodAdd("@star",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFMul(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(f32_,  { f32_ }));
        impl->MethodAdd("@slash",   [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFDiv(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(f32_,  { f32_ }));
        impl->MethodAdd("@neg",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFNeg(gen.ArgLoad(args[0]));
        }, sema::FnSign(f32_));
        impl->MethodAdd("@modt",    [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFRem(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(f32_,  { f32_ }));
        impl->MethodAdd("@modf",    [](IRGen& gen, ARGS& args) {
            return Modf(gen, args);
        }, sema::FnSign(f32_,  { f32_ }));
        
        impl->MethodAdd("@gt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpOGT(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f32_ }));
        impl->MethodAdd("@lt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpOLT(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f32_ }));
        impl->MethodAdd("@ge",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpOGE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f32_ }));
        impl->MethodAdd("@le",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpOLE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f32_ }));
        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpOEQ(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f32_ }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateFCmpONE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { f32_ }));
    }

    // f64

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
            return Modf(gen, args);
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

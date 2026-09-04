
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/ir/ir.hpp"
#include "xcompiler/builtin.hpp"

namespace xcompiler {

    // TypeImplTable

    void TypeImplTable::Init() {
        using ARGS      = const std::vector<llvm::Value*>;
        using ARGS_TYPE = const std::vector<sema::Type*>;

        auto none_  = sema::TypeTable::Lookup("none");
        auto bool_  = sema::TypeTable::Lookup("bool");
        auto i32_   = sema::TypeTable::Lookup("i32");
        auto i64_   = sema::TypeTable::Lookup("i64");
        auto f32_   = sema::TypeTable::Lookup("f32");
        auto f64_   = sema::TypeTable::Lookup("f64");
        auto char_  = sema::TypeTable::Lookup("char");
        auto array_ = sema::TypeTable::Lookup("array");
        auto range_ = sema::TypeTable::Lookup("range");

        // Impl
        // TODO Make the size of the type useful
        {
            // bool
            {
                auto impl = TypeImplTable::Set(TypeImpl(bool_, 1));
                
                impl->MethodAdd("@print", [](IRGen& gen, ARGS& args, auto) -> llvm::Value* {
                    auto& builder    = gen.llvm_builder();
                    auto  str_fmt    = builder.CreateGlobalString("%s",    ".fmt.str");
                    auto  str_true   = builder.CreateGlobalString("true",  ".true");
                    auto  str_false  = builder.CreateGlobalString("false", ".false");
                    auto  str_output = builder.CreateSelect(args[0], str_true, str_false);
                    builder.CreateCall(LibC_printf(gen), { str_fmt, str_output });
                    return nullptr;
                }, sema::FnSign(none_, { bool_ }));

                impl->MethodAdd("@deepcopy", [](auto&, ARGS& args, auto) -> llvm::Value* {
                    return args[0];
                }, sema::FnSign(bool_, { bool_ }));
                
                impl->MethodAdd("@eq",   [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpEQ(args[0], args[1]);
                }, sema::FnSign(bool_, { bool_, bool_ }));
                impl->MethodAdd("@neq",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpNE(args[0], args[1]);
                }, sema::FnSign(bool_, { bool_, bool_ }));
                impl->MethodAdd("@and",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateAnd(args[0], args[1]);
                }, sema::FnSign(bool_, { bool_, bool_ }));
                impl->MethodAdd("@or",   [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateOr(args[0], args[1]);
                }, sema::FnSign(bool_, { bool_, bool_ }));
                impl->MethodAdd("@not",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateNot(args[0]);
                }, sema::FnSign(bool_, { bool_ }));
            }

            // i32
            {
                auto impl = TypeImplTable::Set(TypeImpl(i32_, 4));

                impl->MethodAdd("@print", [](IRGen& gen, ARGS& args, auto) -> llvm::Value* {
                    auto& builder = gen.llvm_builder();
                    auto  str_fmt = builder.CreateGlobalString("%d", ".fmt.i32");
                    builder.CreateCall(LibC_printf(gen), { str_fmt, args[0] });
                    return nullptr;
                }, sema::FnSign(none_, { i32_ }));
                
                impl->MethodAdd("@deepcopy", [](auto&, ARGS& args, auto) -> llvm::Value* {
                    return args[0];
                }, sema::FnSign(i32_, { i32_ }));
                
                impl->MethodAdd("@plus",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateAdd(args[0], args[1]);
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                impl->MethodAdd("@minus", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateSub(args[0], args[1]);
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                impl->MethodAdd("@star",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateMul(args[0], args[1]);
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                impl->MethodAdd("@slash", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateSDiv(args[0], args[1]);
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                impl->MethodAdd("@neg",   [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateNeg(args[0]);
                }, sema::FnSign(i32_,  { i32_ }));
                impl->MethodAdd("@modt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateSRem(args[0], args[1]);
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                impl->MethodAdd("@modf",  [](IRGen& gen, ARGS& args, auto) {
                    auto& builder = gen.llvm_builder();
                    auto  zero    = builder.getInt32(0);
                    auto  modt    = builder.CreateSRem(args[0], args[1]);
                    auto  isModTNeqZero = builder.CreateICmpNE(modt, zero);
                    auto  isDiffSign    = builder.CreateXor(builder.CreateICmpSLT(modt, zero), builder.CreateICmpSLT(args[1], zero));
                    auto  hasRevise     = builder.CreateAnd(isModTNeqZero, isDiffSign);
                    return builder.CreateAdd(modt, builder.CreateSelect(hasRevise, args[1], zero));
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                
                impl->MethodAdd("@gt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSGT(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                impl->MethodAdd("@lt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSLT(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                impl->MethodAdd("@ge",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSGE(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                impl->MethodAdd("@le",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSLE(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                impl->MethodAdd("@eq",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpEQ(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                impl->MethodAdd("@neq", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpNE(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                
                impl->MethodAdd("@cast",[](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateSExt(args[0], gen.llvm_builder().getInt64Ty());
                }, sema::FnSign(i64_, { i32_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
                impl->MethodAdd("@cast", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateSIToFP(args[0], gen.llvm_builder().getFloatTy());
                }, sema::FnSign(f32_, { i32_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
                impl->MethodAdd("@cast", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateSIToFP(args[0], gen.llvm_builder().getDoubleTy());
                }, sema::FnSign(f64_, { i32_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
            }

            // i64
            {
                auto impl = TypeImplTable::Set(TypeImpl(i64_, 8));

                impl->MethodAdd("@print", [](IRGen& gen, ARGS& args, auto) -> llvm::Value* {
                    auto& builder = gen.llvm_builder();
                    auto  str_fmt = builder.CreateGlobalString("%lld", ".fmt.i64");
                    builder.CreateCall(LibC_printf(gen), { str_fmt, args[0] });
                    return nullptr;
                }, sema::FnSign(none_, { i64_ }));
                
                impl->MethodAdd("@deepcopy", [](auto&, ARGS& args, auto) -> llvm::Value* {
                    return args[0];
                }, sema::FnSign(i64_, { i64_ }));
                
                impl->MethodAdd("@plus",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateAdd(args[0], args[1]);
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                impl->MethodAdd("@minus", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateSub(args[0], args[1]);
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                impl->MethodAdd("@star",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateMul(args[0], args[1]);
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                impl->MethodAdd("@slash", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateSDiv(args[0], args[1]);
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                impl->MethodAdd("@neg",   [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateNeg(args[0]);
                }, sema::FnSign(i64_,  { i64_ }));
                impl->MethodAdd("@modt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateSRem(args[0], args[1]);
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                impl->MethodAdd("@modf",  [](IRGen& gen, ARGS& args, auto) {
                    auto& builder = gen.llvm_builder();
                    auto  zero    = builder.getInt64(0);
                    auto  modt    = builder.CreateSRem(args[0], args[1]);
                    auto  isModTNeqZero = builder.CreateICmpNE(modt, zero);
                    auto  isDiffSign    = builder.CreateXor(builder.CreateICmpSLT(modt, zero), builder.CreateICmpSLT(args[1], zero));
                    auto  hasRevise     = builder.CreateAnd(isModTNeqZero, isDiffSign);
                    return builder.CreateAdd(modt, builder.CreateSelect(hasRevise, args[1], zero));
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                
                impl->MethodAdd("@gt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSGT(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                impl->MethodAdd("@lt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSLT(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                impl->MethodAdd("@ge",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSGE(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                impl->MethodAdd("@le",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSLE(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                impl->MethodAdd("@eq",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpEQ(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                impl->MethodAdd("@neq", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpNE(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                
                impl->MethodAdd("@cast", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateSIToFP(args[0], gen.llvm_builder().getFloatTy());
                }, sema::FnSign(f32_, { i64_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
                impl->MethodAdd("@cast", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateSIToFP(args[0], gen.llvm_builder().getDoubleTy());
                }, sema::FnSign(f64_, { i64_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
            }

            // f32
            {
                auto impl = TypeImplTable::Set(TypeImpl(f32_, 4));

                impl->MethodAdd("@print", [](IRGen& gen, ARGS& args, auto) -> llvm::Value* {
                    auto& builder = gen.llvm_builder();
                    auto  str_fmt = builder.CreateGlobalString("%f", ".fmt.f32");
                    builder.CreateCall(LibC_printf(gen), {
                        str_fmt,
                        builder.CreateFPExt(args[0], builder.getDoubleTy())
                    });
                    return nullptr;
                }, sema::FnSign(none_, { f32_ }));
                
                impl->MethodAdd("@deepcopy", [](auto&, ARGS& args, auto) -> llvm::Value* {
                    return args[0];
                }, sema::FnSign(f32_, { f32_ }));

                impl->MethodAdd("@plus",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFAdd(args[0], args[1]);
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                impl->MethodAdd("@minus", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFSub(args[0], args[1]);
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                impl->MethodAdd("@star",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFMul(args[0], args[1]);
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                impl->MethodAdd("@slash", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFDiv(args[0], args[1]);
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                impl->MethodAdd("@neg",   [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFNeg(args[0]);
                }, sema::FnSign(f32_,  { f32_ }));
                impl->MethodAdd("@modt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFRem(args[0], args[1]);
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                impl->MethodAdd("@modf",  [](IRGen& gen, ARGS& args, auto) {
                    auto& builder = gen.llvm_builder();
                    auto  zero    = llvm::ConstantFP::get(builder.getFloatTy(), 0.0);
                    auto  modt    = builder.CreateFRem(args[0], args[1]);
                    auto  isModTNeqZero = builder.CreateFCmpONE(modt, zero);
                    auto  isDiffSign    = builder.CreateXor(builder.CreateFCmpOLT(modt, zero), builder.CreateFCmpOLT(args[1], zero));
                    auto  hasRevise     = builder.CreateAnd(isModTNeqZero, isDiffSign);
                    return builder.CreateFAdd(modt, builder.CreateSelect(hasRevise, args[1], zero));
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                
                impl->MethodAdd("@gt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpOGT(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                impl->MethodAdd("@lt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpOLT(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                impl->MethodAdd("@ge",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpOGE(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                impl->MethodAdd("@le",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpOLE(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                impl->MethodAdd("@eq",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpOEQ(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                impl->MethodAdd("@neq", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpONE(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                
                impl->MethodAdd("@cast", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFPExt(args[0], gen.llvm_builder().getDoubleTy());
                }, sema::FnSign(f64_, { f32_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
            }

            // f64
            {
                auto impl = TypeImplTable::Set(TypeImpl(f64_, 8));

                impl->MethodAdd("@print", [](IRGen& gen, ARGS& args, auto) -> llvm::Value* {
                    auto& builder = gen.llvm_builder();
                    auto  str_fmt = builder.CreateGlobalString("%f", ".fmt.f64");
                    builder.CreateCall(LibC_printf(gen), { str_fmt, args[0] });
                    return nullptr;
                }, sema::FnSign(none_, { f64_ }));
                
                impl->MethodAdd("@deepcopy", [](auto&, ARGS& args, auto) -> llvm::Value* {
                    return args[0];
                }, sema::FnSign(f64_, { f64_ }));

                impl->MethodAdd("@plus",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFAdd(args[0], args[1]);
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                impl->MethodAdd("@minus", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFSub(args[0], args[1]);
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                impl->MethodAdd("@star",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFMul(args[0], args[1]);
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                impl->MethodAdd("@slash", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFDiv(args[0], args[1]);
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                impl->MethodAdd("@neg",   [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFNeg(args[0]);
                }, sema::FnSign(f64_,  { f64_ }));
                impl->MethodAdd("@modt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFRem(args[0], args[1]);
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                impl->MethodAdd("@modf",  [](IRGen& gen, ARGS& args, auto) {
                    auto& builder = gen.llvm_builder();
                    auto  zero    = llvm::ConstantFP::get(builder.getDoubleTy(), 0.0);
                    auto  modt    = builder.CreateFRem(args[0], args[1]);
                    auto  isModTNeqZero = builder.CreateFCmpONE(modt, zero);
                    auto  isDiffSign    = builder.CreateXor(builder.CreateFCmpOLT(modt, zero), builder.CreateFCmpOLT(args[1], zero));
                    auto  hasRevise     = builder.CreateAnd(isModTNeqZero, isDiffSign);
                    return builder.CreateFAdd(modt, builder.CreateSelect(hasRevise, args[1], zero));
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                
                impl->MethodAdd("@gt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpOGT(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
                impl->MethodAdd("@lt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpOLT(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
                impl->MethodAdd("@ge",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpOGE(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
                impl->MethodAdd("@le",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpOLE(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
                impl->MethodAdd("@eq",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpOEQ(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
                impl->MethodAdd("@neq", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateFCmpONE(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
            }
        
            // char
            {
                auto impl = TypeImplTable::Set(TypeImpl(char_, 4));

                impl->MethodAdd("@print", [](IRGen& gen, ARGS& args, auto) -> llvm::Value* {
                    auto& builder   = gen.llvm_builder();
                    auto  codepoint = args[0];

                    // Buffer
                    auto buf = builder.CreateAlloca(
                        llvm::ArrayType::get(builder.getInt8Ty(), 5), nullptr, ".char.buf"
                    );

                    // Store
                    auto store_byte = [&](llvm::Value* val, uint32_t idx) {
                        auto ptr = builder.CreateGEP(builder.getInt8Ty(), buf, { builder.getInt32(idx) });
                        builder.CreateStore(builder.CreateTrunc(val, builder.getInt8Ty()), ptr);
                    };
                    auto store_null = [&](uint32_t idx) {
                        auto ptr = builder.CreateGEP(builder.getInt8Ty(), buf, { builder.getInt32(idx) });
                        builder.CreateStore(builder.getInt8(0), ptr);
                    };

                    // Blocks
                    auto fn = builder.GetInsertBlock()->getParent();
                    auto block_write1    = gen.BlockCreate(".char.write1", fn);
                    auto block_write2    = gen.BlockCreate(".char.write2", fn);
                    auto block_write3    = gen.BlockCreate(".char.write3", fn);
                    auto block_write4    = gen.BlockCreate(".char.write4", fn);
                    auto block_or2or3or4 = gen.BlockCreate(".char.or2or3or4", fn);
                    auto block_or3or4    = gen.BlockCreate(".char.or3or4", fn);
                    auto block_end       = gen.BlockCreate(".char.end", fn);

                    // 1 Byte Block
                    builder.CreateCondBr(builder.CreateICmpULT(codepoint, builder.getInt32(0x80)), block_write1, block_or2or3or4);
                    builder.SetInsertPoint(block_write1);
                    {
                        store_byte(codepoint, 0);
                        store_null(1);
                        builder.CreateBr(block_end);
                    }

                    // 2 Bytes Block
                    builder.SetInsertPoint(block_or2or3or4);
                    builder.CreateCondBr(builder.CreateICmpULT(codepoint, builder.getInt32(0x800)), block_write2, block_or3or4);
                    builder.SetInsertPoint(block_write2);
                    {
                        store_byte(builder.CreateOr(builder.getInt32(0xc0), builder.CreateLShr(codepoint, builder.getInt32(6))), 0);
                        store_byte(builder.CreateOr(builder.getInt32(0x80), builder.CreateAnd(codepoint, builder.getInt32(0x3f))), 1);
                        store_null(2);
                        builder.CreateBr(block_end);
                    }

                    // 3 Bytes Block
                    builder.SetInsertPoint(block_or3or4);
                    builder.CreateCondBr(builder.CreateICmpULT(codepoint, builder.getInt32(0x10000)), block_write3, block_write4);
                    builder.SetInsertPoint(block_write3);
                    {
                        store_byte(builder.CreateOr(builder.getInt32(0xe0), builder.CreateLShr(codepoint, builder.getInt32(12))), 0);
                        store_byte(builder.CreateOr(builder.getInt32(0x80),
                            builder.CreateAnd(builder.CreateLShr(codepoint, builder.getInt32(6)), builder.getInt32(0x3f))
                        ), 1);
                        store_byte(builder.CreateOr(builder.getInt32(0x80), builder.CreateAnd(codepoint, builder.getInt32(0x3f))), 2);
                        store_null(3);
                        builder.CreateBr(block_end);
                    }

                    // 4 Bytes Block
                    builder.SetInsertPoint(block_write4);
                    {
                        store_byte(builder.CreateOr(builder.getInt32(0xf0), builder.CreateLShr(codepoint, builder.getInt32(18))), 0);
                        store_byte(builder.CreateOr(builder.getInt32(0x80),
                            builder.CreateAnd(builder.CreateLShr(codepoint, builder.getInt32(12)), builder.getInt32(0x3f))
                        ), 1);
                        store_byte(builder.CreateOr(builder.getInt32(0x80),
                            builder.CreateAnd(builder.CreateLShr(codepoint, builder.getInt32(6)), builder.getInt32(0x3f))
                        ), 2);
                        store_byte(builder.CreateOr(builder.getInt32(0x80), builder.CreateAnd(codepoint, builder.getInt32(0x3f))), 3);
                        store_null(4);
                        builder.CreateBr(block_end);
                    }

                    // End Block
                    builder.SetInsertPoint(block_end);
                    {
                        auto str_fmt = builder.CreateGlobalString("%s", ".fmt.str");
                        builder.CreateCall(LibC_printf(gen), { str_fmt, buf });
                    }

                    return nullptr;                    
                }, sema::FnSign(none_, { char_ }));
                
                impl->MethodAdd("@deepcopy", [](auto&, ARGS& args, auto) -> llvm::Value* {
                    return args[0];
                }, sema::FnSign(char_, { char_ }));

                impl->MethodAdd("@gt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSGT(args[0], args[1]);
                }, sema::FnSign(bool_, { char_, char_ }));
                impl->MethodAdd("@lt",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSLT(args[0], args[1]);
                }, sema::FnSign(bool_, { char_, char_ }));
                impl->MethodAdd("@ge",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSGE(args[0], args[1]);
                }, sema::FnSign(bool_, { char_, char_ }));
                impl->MethodAdd("@le",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpSLE(args[0], args[1]);
                }, sema::FnSign(bool_, { char_, char_ }));
                impl->MethodAdd("@eq",  [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpEQ(args[0], args[1]);
                }, sema::FnSign(bool_, { char_, char_ }));
                impl->MethodAdd("@neq", [](IRGen& gen, ARGS& args, auto) {
                    return gen.llvm_builder().CreateICmpNE(args[0], args[1]);
                }, sema::FnSign(bool_, { char_, char_ }));
            }

            // array
            {
                auto impl = TypeImplTable::Set(TypeImpl(array_, 0));
            }

            // range
            {
                auto impl = TypeImplTable::Set(TypeImpl(range_, 0));

                impl->MethodAdd("@print", [](IRGen& gen, ARGS& args, ARGS_TYPE& args_type) -> llvm::Value* {
                    auto& builder = gen.llvm_builder();

                    auto  range_val      = args[0];
                    auto  range_type     = (sema::ParametricType*)args_type[0];
                    auto  iter_type      = range_type->params_type()[0];
                    auto  iter_type_impl = TypeImplTable::Lookup(iter_type);
                    auto  left_val       = builder.CreateExtractValue(range_val, 0);
                    auto  right_val      = builder.CreateExtractValue(range_val, 1);
                    auto  step_val       = builder.CreateExtractValue(range_val, 2);
                    auto  isClosed_val   = builder.CreateExtractValue(range_val, 3);

                    builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString("[", ".range.lb") });
                    iter_type_impl->MethodCall(gen, "@print", { left_val }, { iter_type });
                    builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString(" : ", ".range.sep") });
                    iter_type_impl->MethodCall(gen, "@print", { step_val }, { iter_type });
                    builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString(" : ", ".range.sep") });
                    iter_type_impl->MethodCall(gen, "@print", { right_val }, { iter_type });
                    
                    // Right Boundary
                    auto rb = builder.CreateGlobalString("]", ".rb");   // RBrace
                    auto rp = builder.CreateGlobalString(")", ".rp");   // RParen
                    builder.CreateCall(LibC_printf(gen), {
                        builder.CreateGlobalString("%s", ".fmt.str"),
                        builder.CreateSelect(isClosed_val, rb, rp)
                    });

                    return nullptr;
                }, sema::FnSign(none_, { range_ }));
            }
        }
    }
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    void TypeImplTable::Init_char() {
        using ARGS    = const std::vector<Arg>&;

        auto  none_   = sema::TypeTable::Lookup("none");
        auto  bool_   = sema::TypeTable::Lookup("bool");
        auto  char_   = sema::TypeTable::Lookup("char");
        auto  string_ = sema::TypeTable::Lookup("string");

        auto impl = TypeImplTable::Set(TypeImpl(char_));

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder   = gen.llvm_builder();
            auto  codepoint = gen.ArgLoad(args[0]);

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
        }, sema::FnSign(none_));
        
        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(char_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        impl->MethodAdd("@gt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSGT(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { char_ }));
        impl->MethodAdd("@lt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSLT(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { char_ }));
        impl->MethodAdd("@ge",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSGE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { char_ }));
        impl->MethodAdd("@le",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSLE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { char_ }));
        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpEQ(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { char_ }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpNE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { char_ }));

        impl->MethodAdd("@cast",    [string_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder   = gen.llvm_builder();
            auto  codepoint = gen.ArgLoad(args[0]);

            auto  data = builder.CreateCall(LibC_malloc(gen), { builder.getInt64(4) });
            builder.CreateStore(codepoint, data);

            auto gen_val = (llvm::Value*)llvm::UndefValue::get(gen.LLVMType(string_));
            gen_val = builder.CreateInsertValue(gen_val, data, 0);
            gen_val = builder.CreateInsertValue(gen_val, builder.getInt64(1), 1);
            return gen_val;
        }, sema::FnSign(string_, {}, std::nullopt, sema::FnModifier::Cast));
    }
}

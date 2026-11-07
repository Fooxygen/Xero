
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

        auto  impl    = TypeImplTable::Set(TypeImpl(char_));

        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) {
            return gen.ArgValGet(args[0]);
        }, sema::FnSign(char_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // @cast

        impl->MethodAdd("@cast",    [string_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder   = gen.llvm_builder();
            auto  codepoint = gen.ArgValGet(args[0]);

            auto  data = builder.CreateCall(LibC_malloc(gen), { builder.getInt64(4) });
            builder.CreateStore(codepoint, data);

            return gen.StructTypeValCreate(
                gen.LlvmType(string_), { data, builder.getInt64(1) }
            );
        }, sema::FnSign(string_, {}, std::nullopt, sema::FnModifier::Cast));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder   = gen.llvm_builder();
            auto  codepoint = gen.ArgValGet(args[0]);

            // Buffer
            auto buf = gen.SlotCreate(
                llvm::ArrayType::get(builder.getInt8Ty(), 5), ".char.buf"
            );

            // Store
            auto ByteStore = [&](llvm::Value* val, uint32_t idx) {
                auto ptr = builder.CreateGEP(builder.getInt8Ty(), buf, { builder.getInt32(idx) });
                builder.CreateStore(builder.CreateTrunc(val, builder.getInt8Ty()), ptr);
            };
            auto NullStore = [&](uint32_t idx) {
                auto ptr = builder.CreateGEP(builder.getInt8Ty(), buf, { builder.getInt32(idx) });
                builder.CreateStore(builder.getInt8(0), ptr);
            };

            // Blocks
            auto fn              = builder.GetInsertBlock()->getParent();
            auto block_encode_1     = gen.BlockCreate(".char.encode.1", fn);
            auto block_encode_2     = gen.BlockCreate(".char.encode.2", fn);
            auto block_encode_3     = gen.BlockCreate(".char.encode.3", fn);
            auto block_encode_4     = gen.BlockCreate(".char.encode.4", fn);
            auto block_encode_or234 = gen.BlockCreate(".char.encode.or234", fn);
            auto block_encode_or34  = gen.BlockCreate(".char.encode.or34", fn);
            auto block_end          = gen.BlockCreate(".char.print.end", fn);

            // 1 Byte Block
            builder.CreateCondBr(builder.CreateICmpULT(codepoint, builder.getInt32(0x80)), block_encode_1, block_encode_or234);
            builder.SetInsertPoint(block_encode_1);
            {
                ByteStore(codepoint, 0);
                NullStore(1);
                builder.CreateBr(block_end);
            }

            // 2 Bytes Block
            builder.SetInsertPoint(block_encode_or234);
            builder.CreateCondBr(builder.CreateICmpULT(codepoint, builder.getInt32(0x800)), block_encode_2, block_encode_or34);
            builder.SetInsertPoint(block_encode_2);
            {
                ByteStore(builder.CreateOr(builder.getInt32(0xc0), builder.CreateLShr(codepoint, builder.getInt32(6))), 0);
                ByteStore(builder.CreateOr(builder.getInt32(0x80), builder.CreateAnd(codepoint, builder.getInt32(0x3f))), 1);
                NullStore(2);
                builder.CreateBr(block_end);
            }

            // 3 Bytes Block
            builder.SetInsertPoint(block_encode_or34);
            builder.CreateCondBr(builder.CreateICmpULT(codepoint, builder.getInt32(0x10000)), block_encode_3, block_encode_4);
            builder.SetInsertPoint(block_encode_3);
            {
                ByteStore(builder.CreateOr(builder.getInt32(0xe0), builder.CreateLShr(codepoint, builder.getInt32(12))), 0);
                ByteStore(builder.CreateOr(builder.getInt32(0x80),
                    builder.CreateAnd(builder.CreateLShr(codepoint, builder.getInt32(6)), builder.getInt32(0x3f))
                ), 1);
                ByteStore(builder.CreateOr(builder.getInt32(0x80), builder.CreateAnd(codepoint, builder.getInt32(0x3f))), 2);
                NullStore(3);
                builder.CreateBr(block_end);
            }

            // 4 Bytes Block
            builder.SetInsertPoint(block_encode_4);
            {
                ByteStore(builder.CreateOr(builder.getInt32(0xf0), builder.CreateLShr(codepoint, builder.getInt32(18))), 0);
                ByteStore(builder.CreateOr(builder.getInt32(0x80),
                    builder.CreateAnd(builder.CreateLShr(codepoint, builder.getInt32(12)), builder.getInt32(0x3f))
                ), 1);
                ByteStore(builder.CreateOr(builder.getInt32(0x80),
                    builder.CreateAnd(builder.CreateLShr(codepoint, builder.getInt32(6)), builder.getInt32(0x3f))
                ), 2);
                ByteStore(builder.CreateOr(builder.getInt32(0x80), builder.CreateAnd(codepoint, builder.getInt32(0x3f))), 3);
                NullStore(4);
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

        impl->MethodAdd("@gt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSGT(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { char_ }));
        impl->MethodAdd("@lt",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSLT(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { char_ }));
        impl->MethodAdd("@ge",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSGE(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { char_ }));
        impl->MethodAdd("@le",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpSLE(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { char_ }));
        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpEQ(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { char_ }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpNE(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { char_ }));
    }
}

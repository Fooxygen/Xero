
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    void TypeImplTable::Init_string() {
        using ARGS    = const std::vector<Arg>&;

        auto  none_   = sema::TypeTable::Lookup("none");
        auto  char_   = sema::TypeTable::Lookup("char");
        auto  i64_    = sema::TypeTable::Lookup("i64");
        auto  string_ = sema::TypeTable::Lookup("string");

        auto  impl    = TypeImplTable::Set(TypeImpl(string_));

        struct StringInfo {
            llvm::Value* addr      = nullptr;
            sema::Type*  type      = nullptr;
            llvm::Type*  llvm_type = nullptr;
            llvm::Value* data      = nullptr;
            llvm::Value* len       = nullptr;
        };

        auto string_load = [](IRGen& gen, const Arg& arg) -> StringInfo {
            auto& builder   = gen.llvm_builder();
            auto  addr      = gen.ArgAddr(arg);
            auto  type      = arg.ReferenceUnwrap();
            auto  llvm_type = gen.LLVMType(type);
            auto  val       = builder.CreateLoad(llvm_type, addr);

            return {
                addr, type, llvm_type,
                builder.CreateExtractValue(val, 0),
                builder.CreateExtractValue(val, 1)
            };
        };
        auto elem_get    = [](IRGen& gen, llvm::Value* data, llvm::Value* idx) -> llvm::Value* {
            return gen.llvm_builder().CreateInBoundsGEP(
                gen.llvm_builder().getInt32Ty(), data, { idx }
            );
        };

        impl->MethodAdd("@print",   [string_load, elem_get, char_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str     = string_load(gen, args[0]);

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".string.print.cond", fn);
            auto block_body  = gen.BlockCreate(".string.print.body", fn);
            auto block_end   = gen.BlockCreate(".string.print.end",  fn);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, str.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto elem = elem_get(gen, str.data, counter);
                TypeImplTable::Lookup(char_)->MethodCall(gen, "@print", {
                    Arg(elem, sema::TypeTable::ReferenceTypeGet(char_))
                });

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            // End Block
            builder.SetInsertPoint(block_end);
            return nullptr;
        }, sema::FnSign(none_));

        impl->MethodAdd("@copy",    [string_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str     = string_load(gen, args[0]);

            // New Data
            auto size     = builder.CreateMul(str.len, builder.getInt64(4));
            auto new_data = builder.CreateCall(LibC_malloc(gen), { size });
            builder.CreateCall(LibC_memmove(gen), { new_data, str.data, size });

            // Generated Value
            auto gen_val = (llvm::Value*)llvm::UndefValue::get(str.llvm_type);
            gen_val = builder.CreateInsertValue(gen_val, new_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, str.len,  1);
            return gen_val;
        }, sema::FnSign(string_));
        impl->MethodAdd("@release", [string_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto str = string_load(gen, args[0]);
            gen.llvm_builder().CreateCall(LibC_free(gen), { str.data });
            return nullptr;
        }, sema::FnSign(none_));

        impl->MethodAdd("len",      [string_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            return string_load(gen, args[0]).len;
        }, sema::FnSign(i64_));
    }
}

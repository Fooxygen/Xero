
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
        auto  bool_   = sema::TypeTable::Lookup("bool");
        auto  char_   = sema::TypeTable::Lookup("char");
        auto  i64_    = sema::TypeTable::Lookup("i64");
        auto  string_ = sema::TypeTable::Lookup("string");

        auto  impl    = TypeImplTable::Set(TypeImpl(string_));

        // Utility

        struct StringInfo {
            llvm::Value* addr      = nullptr;
            sema::Type*  type      = nullptr;
            llvm::Type*  llvm_type = nullptr;
            llvm::Value* data      = nullptr;
            llvm::Value* len       = nullptr;
        };

        auto char_get     = [](IRGen& gen, llvm::Value* data, llvm::Value* idx) -> llvm::Value* {
            return gen.llvm_builder().CreateInBoundsGEP(
                gen.llvm_builder().getInt32Ty(), data, { idx }
            );
        };
        auto string_load  = [](IRGen& gen, const Arg& arg) -> StringInfo {
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
        auto string_store = [](IRGen& gen, llvm::Value* addr, llvm::Value* data, llvm::Type* llvm_type, llvm::Value* len) {
            auto& builder = gen.llvm_builder();
            auto  gen_val = (llvm::Value*)llvm::UndefValue::get(llvm_type);
            gen_val = builder.CreateInsertValue(gen_val, data, 0);
            gen_val = builder.CreateInsertValue(gen_val, len, 1);
            builder.CreateStore(gen_val, addr);
        };
        auto string_equal = [string_load, char_get](
            IRGen& gen, const Arg& arg_lstr, const Arg& arg_rstr) -> llvm::Value*
        {
            auto& builder = gen.llvm_builder();
            auto  lstr    = string_load(gen, arg_lstr);
            auto  rstr    = string_load(gen, arg_rstr);

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".string.equal.cond", fn);
            auto block_body  = gen.BlockCreate(".string.equal.body", fn);
            auto block_end   = gen.BlockCreate(".string.equal.end",  fn);

            auto len_eq = builder.CreateICmpEQ(lstr.len, rstr.len);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            auto result  = builder.CreatePHI(builder.getInt1Ty(),  2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                result->addIncoming(len_eq, block_entry);

                auto lstr_in = builder.CreateICmpSLT(counter, lstr.len);
                auto rstr_in = builder.CreateICmpSLT(counter, rstr.len);
                auto cont    = builder.CreateAnd(builder.CreateAnd(lstr_in, rstr_in), result);
                builder.CreateCondBr(cont, block_body, block_end);
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto lstr_val = builder.CreateLoad(builder.getInt32Ty(), char_get(gen, lstr.data, counter));
                auto rstr_val = builder.CreateLoad(builder.getInt32Ty(), char_get(gen, rstr.data, counter));
                auto eq       = builder.CreateICmpEQ(lstr_val, rstr_val);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
                result->addIncoming(eq,   builder.GetInsertBlock());
            }

            // End Block
            builder.SetInsertPoint(block_end);
            return result;
        };

        // @copy and @release

        impl->MethodAdd("@copy",    [string_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str     = string_load(gen, args[0]);

            // Result
            auto result_size = builder.CreateMul(str.len, builder.getInt64(4));
            auto result_data = builder.CreateCall(LibC_malloc(gen), { result_size });
            builder.CreateCall(LibC_memmove(gen), { result_data, str.data, result_size });

            // Generated Value
            auto gen_val = (llvm::Value*)llvm::UndefValue::get(str.llvm_type);
            gen_val = builder.CreateInsertValue(gen_val, result_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, str.len,  1);
            return gen_val;
        }, sema::FnSign(string_));
        impl->MethodAdd("@release", [string_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto str = string_load(gen, args[0]);
            gen.llvm_builder().CreateCall(LibC_free(gen), { str.data });
            return nullptr;
        }, sema::FnSign(none_));

        // Other

        impl->MethodAdd("@print",   [string_load, char_get, char_](IRGen& gen, ARGS& args) -> llvm::Value* {
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
                auto elem = char_get(gen, str.data, counter);
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

        impl->MethodAdd("@pick",    [string_load, char_get](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto str     = string_load(gen, args[0]);
            auto idx     = gen.ArgLoad(args[1]);
            return char_get(gen, str.data, idx);
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(char_), { i64_ }));
        impl->MethodAdd("@pick",    [string_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder  = gen.llvm_builder();
            auto  str      = string_load(gen, args[0]);
            auto  range    = gen.ArgLoad(args[1]);

            auto left      = builder.CreateIntCast(builder.CreateExtractValue(range, 0), builder.getInt64Ty(), true);
            auto right     = builder.CreateIntCast(builder.CreateExtractValue(range, 1), builder.getInt64Ty(), true);
            auto is_closed = builder.CreateExtractValue(range, 3);

            auto diff = builder.CreateSub(right, left);
            auto len  = builder.CreateSelect(is_closed, builder.CreateAdd(diff, builder.getInt64(1)), diff);

            auto view_type = gen.LLVMType(sema::TypeTable::Lookup("stringview"));
            auto gen_val   = (llvm::Value*)llvm::UndefValue::get(view_type);
            gen_val = builder.CreateInsertValue(gen_val, str.addr, 0);
            gen_val = builder.CreateInsertValue(gen_val, left,     1);
            gen_val = builder.CreateInsertValue(gen_val, len,      2);
            return gen_val;
        }, sema::FnSign(sema::TypeTable::Lookup("stringview"), { sema::TypeTable::Lookup("range") }));

        impl->MethodAdd("@plus",    [string_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  lstr    = string_load(gen, args[0]);
            auto  rstr    = string_load(gen, args[1]);

            auto result_len  = builder.CreateAdd(lstr.len, rstr.len);
            auto result_size = builder.CreateMul(result_len, builder.getInt64(4));
            auto result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            // Copy lstr
            builder.CreateCall(LibC_memmove(gen), {
                result_data, lstr.data, builder.CreateMul(lstr.len, builder.getInt64(4))
            });

            // Copy rstr
            auto rstr_dst = builder.CreateInBoundsGEP(
                builder.getInt32Ty(), result_data, { lstr.len }
            );
            builder.CreateCall(LibC_memmove(gen), {
                rstr_dst, rstr.data, builder.CreateMul(rstr.len, builder.getInt64(4))
            });

            auto gen_val = (llvm::Value*)llvm::UndefValue::get(lstr.llvm_type);
            gen_val = builder.CreateInsertValue(gen_val, result_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, result_len,  1);
            return gen_val;
        }, sema::FnSign(string_, { string_ }));
        impl->MethodAdd("@neg",     [string_load, char_get](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str     = string_load(gen, args[0]);

            auto  result_size = builder.CreateMul(str.len, builder.getInt64(4));
            auto  result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".string.neg.cond", fn);
            auto block_body  = gen.BlockCreate(".string.neg.body", fn);
            auto block_end   = gen.BlockCreate(".string.neg.end",  fn);

            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(counter, str.len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto src_idx = builder.CreateSub(builder.CreateSub(str.len, builder.getInt64(1)), counter);
                auto src = char_get(gen, str.data, src_idx);
                auto dst = char_get(gen, result_data, counter);
                builder.CreateStore(builder.CreateLoad(builder.getInt32Ty(), src), dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);

            auto gen_val = (llvm::Value*)llvm::UndefValue::get(str.llvm_type);
            gen_val = builder.CreateInsertValue(gen_val, result_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, str.len,  1);
            return gen_val;
        }, sema::FnSign(string_));
        impl->MethodAdd("@eq",      [string_equal](IRGen& gen, ARGS& args) -> llvm::Value* {
            return string_equal(gen, args[0], args[1]);
        }, sema::FnSign(bool_, { string_ }));
        impl->MethodAdd("@neq",     [string_equal](IRGen& gen, ARGS& args) -> llvm::Value* {
            return gen.llvm_builder().CreateNot(string_equal(gen, args[0], args[1]));
        }, sema::FnSign(bool_, { string_ }));

        impl->MethodAdd("len",      [string_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            return string_load(gen, args[0]).len;
        }, sema::FnSign(i64_));
        impl->MethodAdd("clear",    [string_load, string_store](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str     = string_load(gen, args[0]);

            builder.CreateCall(LibC_free(gen), { str.data });

            auto null_data = llvm::ConstantPointerNull::get(
                llvm::PointerType::get(gen.llvm_context(), 0)
            );
            string_store(gen, str.addr, null_data, str.llvm_type, builder.getInt64(0));
            return nullptr;
        }, sema::FnSign(none_));
    }
}

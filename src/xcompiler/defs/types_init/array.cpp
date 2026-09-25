
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    void TypeImplTable::Init_array() {
        using ARGS   = const std::vector<Arg>&;

        auto  none_  = sema::TypeTable::Lookup("none");
        auto  i64_   = sema::TypeTable::Lookup("i64");
        auto  array_ = sema::TypeTable::Lookup("array");

        auto  impl   = TypeImplTable::Set(TypeImpl(array_));
        auto  T      = ((sema::BasicType*)array_)->params_binding()[0];

        // Utility

        struct ArrayInfo {
            llvm::Value* addr           = nullptr;
            sema::Type*  type           = nullptr;
            llvm::Type*  llvm_type      = nullptr;
            
            sema::Type*  elem_type      = nullptr;
            TypeImpl*    elem_type_impl = nullptr;
            size_t       elem_size      = 0;

            llvm::Value* data           = nullptr;
            llvm::Value* len            = nullptr;
        };

        auto array_load    = [](IRGen& gen, const Arg& arg) -> ArrayInfo {
            auto& builder  = gen.llvm_builder();
            auto& module   = *gen.llvm_module();

            auto addr           = gen.ArgAddr(arg);
            auto type           = (sema::ParametricType*)arg.ReferenceUnwrap();
            auto llvm_type      = gen.LLVMType(type->BasicTypeGet());
            auto elem_type      = type->params()[0];
            auto elem_type_impl = TypeImplTable::Lookup(elem_type);
            auto elem_size      = module.getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));
            auto val            = builder.CreateLoad(llvm_type, addr);
            
            return {
                addr, type, llvm_type, elem_type, elem_type_impl, elem_size,
                builder.CreateExtractValue(val, 0),
                builder.CreateExtractValue(val, 1)
            };
        };
        auto array_realloc = [](IRGen& gen, llvm::Value* data, size_t elem_size, llvm::Value* new_len) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  size    = builder.CreateMul(new_len, builder.getInt64(elem_size));
            return builder.CreateCall(LibC_realloc(gen), { data, size });
        };
        auto array_store   = [](IRGen& gen, llvm::Value* addr, llvm::Value* data, llvm::Type* array_llvm_type, llvm::Value* len) {
            auto& builder = gen.llvm_builder();
            auto  gen_val = (llvm::Value*)llvm::UndefValue::get(array_llvm_type);
            gen_val = builder.CreateInsertValue(gen_val, data, 0);
            gen_val = builder.CreateInsertValue(gen_val, len, 1);
            builder.CreateStore(gen_val, addr);
        };
        
        auto elem_get      = [](IRGen& gen, llvm::Value* data, size_t elem_size, llvm::Value* idx) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  offset  = builder.CreateMul(idx, builder.getInt64(elem_size));
            return builder.CreateInBoundsGEP(builder.getInt8Ty(), data, { offset });
        };
        auto elem_move     = [](IRGen& gen, llvm::Value* dst, llvm::Value* src, llvm::Value* cnt, size_t elem_size) {
            auto& builder = gen.llvm_builder();
            auto  bytes   = builder.CreateMul(cnt, builder.getInt64(elem_size));
            builder.CreateCall(LibC_memmove(gen), { dst, src, bytes });
        };

        // @copy and @release

        impl->MethodAdd("@copy",        [array_load, elem_get](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = array_load(gen, args[0]);

            // Result
            auto result_size = builder.CreateMul(arr.len, builder.getInt64(arr.elem_size));
            auto result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".array.copy.cond", fn);
            auto block_body  = gen.BlockCreate(".array.copy.body", fn);
            auto block_end   = gen.BlockCreate(".array.copy.end",  fn);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, arr.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto elem_src    = elem_get(gen, arr.data, arr.elem_size, counter);
                auto elem_dst    = elem_get(gen, result_data, arr.elem_size, counter);
                auto elem_copied = arr.elem_type_impl->MethodCall(gen, "@copy", {
                    Arg(elem_src, sema::TypeTable::ReferenceTypeGet(arr.elem_type))
                });
                builder.CreateStore(elem_copied, elem_dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            // End Block
            builder.SetInsertPoint(block_end);

            // Generated Value
            auto gen_val = (llvm::Value*)llvm::UndefValue::get(arr.llvm_type);
            gen_val = builder.CreateInsertValue(gen_val, result_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, arr.len,  1);
            return gen_val;
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T })));
        impl->MethodAdd("@release",     [array_load, elem_get](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = array_load(gen, args[0]);

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".array.release.cond", fn);
            auto block_body  = gen.BlockCreate(".array.release.body", fn);
            auto block_end   = gen.BlockCreate(".array.release.end",  fn);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, arr.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto elem = elem_get(gen, arr.data, arr.elem_size, counter);

                arr.elem_type_impl->MethodCall(gen, "@release", {
                    Arg(elem, sema::TypeTable::ReferenceTypeGet(arr.elem_type))
                });

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            // End Block
            builder.SetInsertPoint(block_end);
            builder.CreateCall(LibC_free(gen), { arr.data });

            return nullptr;
        }, sema::FnSign(none_));

        // Other

        impl->MethodAdd("@print",       [array_load, elem_get](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = array_load(gen, args[0]);

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".array.print.cond",  fn);
            auto block_cont  = gen.BlockCreate(".array.print.cont",  fn);   // continue
            auto block_sep   = gen.BlockCreate(".array.print.sep",   fn);
            auto block_nosep = gen.BlockCreate(".array.print.nosep", fn);
            auto block_body  = gen.BlockCreate(".array.print.body",  fn);
            auto block_end   = gen.BlockCreate(".array.print.end",   fn);

            builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString("[", ".array.lb") });

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry); // init
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, arr.len), block_cont, block_end
                );
            }

            // Cont Block
            builder.SetInsertPoint(block_cont);
            {
                builder.CreateCondBr(
                    builder.CreateICmpEQ(counter, builder.getInt64(0)), block_nosep, block_sep
                );
            }

            // NoSep Block
            builder.SetInsertPoint(block_nosep);
            {
                builder.CreateBr(block_body);
            }

            // Sep Block
            builder.SetInsertPoint(block_sep);
            {
                builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString(", ", ".array.sep") });
                builder.CreateBr(block_body);
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto elem = elem_get(gen, arr.data, arr.elem_size, counter);
                arr.elem_type_impl->MethodCall(gen, "@print", {
                    Arg(elem, sema::TypeTable::ReferenceTypeGet(arr.elem_type))
                });
                
                auto body_end = builder.GetInsertBlock();
                auto next     = builder.CreateAdd(counter, builder.getInt64(1));
                counter->addIncoming(next, body_end);
                
                builder.CreateBr(block_cond);
            }

            // End Block
            builder.SetInsertPoint(block_end);
            {
                builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString("]", ".array.rb") });
            }
            
            return nullptr;
        }, sema::FnSign(none_));

        impl->MethodAdd("@pick",        [array_load, elem_get](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto arr = array_load(gen, args[0]);
            auto idx = gen.ArgLoad(args[1]);
            return elem_get(gen, arr.data, arr.elem_size, idx);
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(T), { i64_ }));
        impl->MethodAdd("@pick",        [array_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder  = gen.llvm_builder();
            auto  arr      = array_load(gen, args[0]);
            auto  range    = gen.ArgLoad(args[1]);

            auto left      = builder.CreateIntCast(builder.CreateExtractValue(range, 0), builder.getInt64Ty(), true);
            auto right     = builder.CreateIntCast(builder.CreateExtractValue(range, 1), builder.getInt64Ty(), true);
            auto is_closed = builder.CreateExtractValue(range, 3);

            auto diff = builder.CreateSub(right, left);
            auto len  = builder.CreateSelect(is_closed, builder.CreateAdd(diff, builder.getInt64(1)), diff);

            auto view_type = gen.LLVMType(sema::TypeTable::Lookup("arrayview"));
            auto gen_val   = (llvm::Value*)llvm::UndefValue::get(view_type);
            gen_val = builder.CreateInsertValue(gen_val, arr.addr, 0);
            gen_val = builder.CreateInsertValue(gen_val, left,     1);
            gen_val = builder.CreateInsertValue(gen_val, len,      2);
            return gen_val;
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(sema::TypeTable::Lookup("arrayview"), { T }), { sema::TypeTable::Lookup("range") }));

        impl->MethodAdd("@neg",         [array_load, elem_get](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = array_load(gen, args[0]);

            // Result
            auto result_size = builder.CreateMul(arr.len, builder.getInt64(arr.elem_size));
            auto result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".array.neg.cond", fn);
            auto block_body  = gen.BlockCreate(".array.neg.body", fn);
            auto block_end   = gen.BlockCreate(".array.neg.end",  fn);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, arr.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto elem_dst_idx = builder.CreateSub(builder.CreateSub(arr.len, builder.getInt64(1)), counter);
                auto elem_src     = elem_get(gen, arr.data, arr.elem_size, counter);
                auto elem_dst     = elem_get(gen, result_data, arr.elem_size, elem_dst_idx);
                auto elem_copied  = arr.elem_type_impl->MethodCall(gen, "@copy", {
                    Arg(elem_src, sema::TypeTable::ReferenceTypeGet(arr.elem_type))
                });
                builder.CreateStore(elem_copied, elem_dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            // End Block
            builder.SetInsertPoint(block_end);

            // Generated Value
            auto gen_val = (llvm::Value*)llvm::UndefValue::get(arr.llvm_type);
            gen_val = builder.CreateInsertValue(gen_val, result_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, arr.len,  1);
            return gen_val;
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T })));
        impl->MethodAdd("@plus",        [array_load, elem_get](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  larr    = array_load(gen, args[0]);
            auto  rarr    = array_load(gen, args[1]);

            auto  result_len  = builder.CreateAdd(larr.len, rarr.len);
            auto  result_size = builder.CreateMul(result_len, builder.getInt64(larr.elem_size));
            auto  result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();

            auto block_l_cond = gen.BlockCreate(".array.plus.l.cond", fn);
            auto block_l_body = gen.BlockCreate(".array.plus.l.body", fn);
            auto block_l_end  = gen.BlockCreate(".array.plus.l.end",  fn);

            builder.CreateBr(block_l_cond);
            builder.SetInsertPoint(block_l_cond);
            auto l_counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                l_counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(l_counter, larr.len), block_l_body, block_l_end);
            }

            builder.SetInsertPoint(block_l_body);
            {
                auto elem_src = elem_get(gen, larr.data, larr.elem_size, l_counter);
                auto elem_dst = elem_get(gen, result_data, larr.elem_size, l_counter);
                auto copied   = larr.elem_type_impl->MethodCall(gen, "@copy", {
                    Arg(elem_src, sema::TypeTable::ReferenceTypeGet(larr.elem_type))
                });
                builder.CreateStore(copied, elem_dst);

                auto next = builder.CreateAdd(l_counter, builder.getInt64(1));
                builder.CreateBr(block_l_cond);
                l_counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_l_end);

            auto block_r_cond = gen.BlockCreate(".array.plus.r.cond", fn);
            auto block_r_body = gen.BlockCreate(".array.plus.r.body", fn);
            auto block_r_end  = gen.BlockCreate(".array.plus.r.end",  fn);

            builder.CreateBr(block_r_cond);
            builder.SetInsertPoint(block_r_cond);
            auto r_counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                r_counter->addIncoming(builder.getInt64(0), block_l_end);
                builder.CreateCondBr(builder.CreateICmpSLT(r_counter, rarr.len), block_r_body, block_r_end);
            }

            builder.SetInsertPoint(block_r_body);
            {
                auto dst_idx  = builder.CreateAdd(larr.len, r_counter);
                auto elem_src = elem_get(gen, rarr.data, rarr.elem_size, r_counter);
                auto elem_dst = elem_get(gen, result_data, rarr.elem_size, dst_idx);
                auto copied   = rarr.elem_type_impl->MethodCall(gen, "@copy", {
                    Arg(elem_src, sema::TypeTable::ReferenceTypeGet(rarr.elem_type))
                });
                builder.CreateStore(copied, elem_dst);

                auto next = builder.CreateAdd(r_counter, builder.getInt64(1));
                builder.CreateBr(block_r_cond);
                r_counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_r_end);

            auto gen_val = (llvm::Value*)llvm::UndefValue::get(larr.llvm_type);
            gen_val = builder.CreateInsertValue(gen_val, result_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, result_len,  1);
            return gen_val;
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T }), { sema::TypeTable::ParametricTypeGet(array_, { T }) }));
        
        impl->MethodAdd("len",          [array_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            return array_load(gen, args[0]).len;
        }, sema::FnSign(i64_));
        impl->MethodAdd("clear",        [array_load, array_store](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = array_load(gen, args[0]);

            TypeImplTable::Lookup(args[0].ReferenceUnwrap())->MethodCall(
                gen, "@release", {
                    Arg(arr.addr, sema::TypeTable::ReferenceTypeGet(args[0].ReferenceUnwrap()))
                }
            );

            auto null_data = llvm::ConstantPointerNull::get(
                llvm::PointerType::get(gen.llvm_context(), 0)
            );
            array_store(gen, arr.addr, null_data, arr.llvm_type, builder.getInt64(0));
            return nullptr;
        }, sema::FnSign(none_));
        impl->MethodAdd("insert",       [array_load, array_realloc, array_store, elem_get, elem_move](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = array_load(gen, args[0]);
            auto  idx     = args[1].val();
            auto  one     = builder.getInt64(1);

            auto  result_len  = builder.CreateAdd(arr.len, one);
            auto  result_data = array_realloc(gen, arr.data, arr.elem_size, result_len);

            elem_move(
                gen,
                elem_get(gen, result_data, arr.elem_size, builder.CreateAdd(idx, one)),
                elem_get(gen, result_data, arr.elem_size, idx),
                builder.CreateSub(arr.len, idx),
                arr.elem_size
            );
            auto value_ref = gen.ArgRefMake(args[2].val(), arr.elem_type);
            auto copied    = arr.elem_type_impl->MethodCall(gen, "@copy", { value_ref });
            builder.CreateStore(copied, elem_get(gen, result_data, arr.elem_size, idx));
            array_store(gen, arr.addr, result_data, arr.llvm_type, result_len);
            return nullptr;
        }, sema::FnSign(none_, { i64_, nullptr }));
        impl->MethodAdd("remove",       [array_load, elem_get, elem_move, array_store](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = array_load(gen, args[0]);
            auto  idx     = args[1].val();
            auto  one     = builder.getInt64(1);
            
            elem_move(
                gen,
                elem_get(gen, arr.data, arr.elem_size, idx),
                elem_get(gen, arr.data, arr.elem_size, builder.CreateAdd(idx, one)),
                builder.CreateSub(builder.CreateSub(arr.len, one), idx),
                arr.elem_size
            );
            array_store(gen, arr.addr, arr.data, arr.llvm_type, builder.CreateSub(arr.len, one));
            return nullptr;
        }, sema::FnSign(none_, { i64_ }));
        impl->MethodAdd("push_back",    [array_load, array_realloc, array_store, elem_get](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = array_load(gen, args[0]);

            auto  result_len  = builder.CreateAdd(arr.len, builder.getInt64(1));
            auto  result_data = array_realloc(gen, arr.data, arr.elem_size, result_len);

            auto value_ref = gen.ArgRefMake(args[1].val(), arr.elem_type);
            auto copied    = arr.elem_type_impl->MethodCall(gen, "@copy", { value_ref });
            builder.CreateStore(copied, elem_get(
                gen, result_data, arr.elem_size, arr.len
            ));
            array_store(gen, arr.addr, result_data, arr.llvm_type, result_len);
            return nullptr;
        }, sema::FnSign(none_, { nullptr }));
        impl->MethodAdd("push_front",   [array_load, array_realloc, array_store, elem_get, elem_move](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = array_load(gen, args[0]);
            auto  one     = builder.getInt64(1);

            auto  result_len  = builder.CreateAdd(arr.len, one);
            auto  result_data = array_realloc(gen, arr.data, arr.elem_size, result_len);

            elem_move(
                gen,
                elem_get(gen, result_data, arr.elem_size, one),
                result_data, arr.len, arr.elem_size
            );
            auto value_ref = gen.ArgRefMake(args[1].val(), arr.elem_type);
            auto copied    = arr.elem_type_impl->MethodCall(gen, "@copy", { value_ref });
            builder.CreateStore(copied, elem_get(gen, result_data, arr.elem_size, builder.getInt64(0)));
            array_store(gen, arr.addr, result_data, arr.llvm_type, result_len);
            return nullptr;
        }, sema::FnSign(none_, { nullptr }));
        impl->MethodAdd("pop_back",     [array_load, array_store](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = array_load(gen, args[0]);

            array_store(
                gen, arr.addr, arr.data, arr.llvm_type,
                builder.CreateSub(arr.len, builder.getInt64(1))
            );
            return nullptr;
        }, sema::FnSign(none_));
        impl->MethodAdd("pop_front",    [array_load, array_store, elem_get, elem_move](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = array_load(gen, args[0]);
            auto  one     = builder.getInt64(1);

            elem_move(
                gen, arr.data,
                elem_get(gen, arr.data, arr.elem_size, one),
                builder.CreateSub(arr.len, one), arr.elem_size
            );
            array_store(gen, arr.addr, arr.data, arr.llvm_type, builder.CreateSub(arr.len, one));
            return nullptr;
        }, sema::FnSign(none_));
    }
}

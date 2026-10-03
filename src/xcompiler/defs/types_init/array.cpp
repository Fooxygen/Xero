
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

        struct MemoryInfo {
            llvm::Value* data = nullptr;
            llvm::Value* len  = nullptr;
        };

        struct ArrayInfo {
            llvm::Value* addr           = nullptr;
            llvm::Value* org            = nullptr;
            llvm::Value* offset         = nullptr;
            llvm::Type*  llvm_type      = nullptr;
            MemoryInfo   memory         = {};

            sema::Type*  elem_type      = nullptr;
            TypeImpl*    elem_type_impl = nullptr;
            size_t       elem_size      = 0;
        };

        ArrayInfo    ArrayLoad(IRGen& gen, const Arg& arg) {
            auto& builder   = gen.llvm_builder();
            auto& module    = *gen.llvm_module();

            auto  addr      = gen.ArgAddr(arg);
            auto  type      = arg.ReferenceUnwrap();
            auto  llvm_type = gen.LLVMType(type->BasicTypeGet());
            auto  val       = builder.CreateLoad(llvm_type, addr);

            sema::Type* elem_type      = nullptr;
            TypeImpl*   elem_type_impl = nullptr;
            size_t      elem_size      = 0;

            // For ArrayExpr, it belongs to BasicType, not ParametricType.
            // So there is no data about elems
            if (auto parametric_type = dynamic_cast<sema::ParametricType*>(type)) {
                elem_type      = parametric_type->params()[0];
                elem_type_impl = TypeImplTable::Lookup(elem_type);
                elem_size      = module.getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));
            }

            return ArrayInfo{
                .addr      = addr,
                .org       = addr,
                .offset    = builder.getInt64(0),
                .llvm_type = llvm_type,
                .memory    = MemoryInfo{
                    .data = builder.CreateExtractValue(val, 0),
                    .len  = builder.CreateExtractValue(val, 1)
                },

                .elem_type      = elem_type,
                .elem_type_impl = elem_type_impl,
                .elem_size      = elem_size
            };
        }

        ArrayInfo    ArrayViewLoad(IRGen& gen, const Arg& arg) {
            auto& builder   = gen.llvm_builder();
            auto& module    = *gen.llvm_module();

            auto  addr      = gen.ArgAddr(arg);
            auto  type      = arg.ReferenceUnwrap();
            auto  llvm_type = gen.LLVMType(type);
            auto  val       = builder.CreateLoad(llvm_type, addr);

            auto  org       = builder.CreateExtractValue(val, 0);
            auto  offset    = builder.CreateExtractValue(val, 1);
            auto  len       = builder.CreateExtractValue(val, 2);

            auto  org_val   = builder.CreateLoad(gen.LLVMType(sema::TypeTable::Lookup("array")), org);
            auto  data      = builder.CreateExtractValue(org_val, 0);

            sema::Type* elem_type      = nullptr;
            TypeImpl*   elem_type_impl = nullptr;
            size_t      elem_size      = 0;

            if (auto parametric_type = dynamic_cast<sema::ParametricType*>(type)) {
                elem_type      = parametric_type->params()[0];
                elem_type_impl = TypeImplTable::Lookup(elem_type);
                elem_size      = module.getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));
            }

            return ArrayInfo{
                .addr      = addr,
                .org       = org,
                .offset    = offset,
                .llvm_type = llvm_type,
                .memory    = MemoryInfo{ data, len },

                .elem_type      = elem_type,
                .elem_type_impl = elem_type_impl,
                .elem_size      = elem_size
            };
        }

        llvm::Value* ElemGet(IRGen& gen, llvm::Value* data, size_t elem_size, llvm::Value* idx) {
            auto& builder = gen.llvm_builder();
            auto  offset  = builder.CreateMul(idx, builder.getInt64(elem_size));
            return builder.CreateInBoundsGEP(builder.getInt8Ty(), data, { offset });
        }  

        void         Store(
            IRGen& gen,
            llvm::Value* addr, const MemoryInfo& memory,
            llvm::Type* llvm_type
        ) {
            gen.llvm_builder().CreateStore(
                gen.StructTypeValCreate(llvm_type, { memory.data, memory.len }),
                addr
            );
        }

        llvm::Value* Realloc(IRGen& gen, llvm::Value* data, size_t elem_size, llvm::Value* new_len) {
            auto& builder = gen.llvm_builder();
            auto  size    = builder.CreateMul(new_len, builder.getInt64(elem_size));
            return builder.CreateCall(LibC_realloc(gen), { data, size });
        }

        llvm::Value* Reverse(IRGen& gen, const ArrayInfo& info) {
            auto& builder = gen.llvm_builder();

            // Result
            auto result_size = builder.CreateMul(info.memory.len, builder.getInt64(info.elem_size));
            auto result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".array.reverse.cond", fn);
            auto block_body  = gen.BlockCreate(".array.reverse.body", fn);
            auto block_end   = gen.BlockCreate(".array.reverse.end",  fn);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, info.memory.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto src = ElemGet(gen, info.memory.data, info.elem_size, builder.CreateAdd(info.offset, counter));
                auto dst = ElemGet(gen, result_data, info.elem_size, builder.CreateSub(builder.CreateSub(info.memory.len, builder.getInt64(1)), counter));
                
                builder.CreateStore(
                    info.elem_type_impl->MethodCall(gen, "@copy", {
                        Arg(src, sema::TypeTable::ReferenceTypeGet(info.elem_type))
                    }),
                    dst
                );

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            // End Block
            builder.SetInsertPoint(block_end);

            // Package
            return gen.StructTypeValCreate(
                gen.LLVMType(sema::TypeTable::Lookup("array")),
                { result_data, info.memory.len }
            );
        }

        void         Copy(IRGen& gen, llvm::Value* dst, llvm::Value* src, llvm::Value* cnt, size_t elem_size) {
            auto& builder = gen.llvm_builder();
            auto  bytes   = builder.CreateMul(cnt, builder.getInt64(elem_size));
            builder.CreateCall(LibC_memmove(gen), { dst, src, bytes });
        }

        llvm::Value* Print(IRGen& gen, const ArrayInfo& info) {
            auto& builder = gen.llvm_builder();

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".array.print.cond",  fn);
            auto block_cont  = gen.BlockCreate(".array.print.cont",  fn);
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
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, info.memory.len), block_cont, block_end
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
                auto idx  = builder.CreateAdd(info.offset, counter);
                auto elem = ElemGet(gen, info.memory.data, info.elem_size, idx);
                
                info.elem_type_impl->MethodCall(gen, "@print", {
                    Arg(elem, sema::TypeTable::ReferenceTypeGet(info.elem_type))
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
        }

        llvm::Value* PickIndex(IRGen& gen, const ArrayInfo& info, llvm::Value* idx) {
            return ElemGet(
                gen,
                info.memory.data, info.elem_size,
                gen.llvm_builder().CreateAdd(info.offset, idx)
            );
        }

        llvm::Value* PickRange(IRGen& gen, const ArrayInfo& info, llvm::Value* range) {
            auto& builder   = gen.llvm_builder();

            auto  left      = builder.CreateIntCast(builder.CreateExtractValue(range, 0), builder.getInt64Ty(), true);
            auto  right     = builder.CreateIntCast(builder.CreateExtractValue(range, 1), builder.getInt64Ty(), true);
            auto  is_closed = builder.CreateExtractValue(range, 3);

            auto  diff      = builder.CreateSub(right, left);
            auto  len       = builder.CreateSelect(is_closed, builder.CreateAdd(diff, builder.getInt64(1)), diff);
            auto  offset    = builder.CreateAdd(info.offset, left);

            return gen.StructTypeValCreate(
                gen.LLVMType(sema::TypeTable::Lookup("arrayview")),
                { info.org, offset, len }
            );
        }
    
        void         Write(
            IRGen& gen,
            const ArrayInfo& lval,
            const MemoryInfo& rval
        ) {
            auto& builder         = gen.llvm_builder();
            auto  array_llvm_type = gen.LLVMType(sema::TypeTable::Lookup("array"));

            auto  elem_type       = lval.elem_type;
            auto  elem_type_impl  = lval.elem_type_impl;
            auto  elem_size       = lval.elem_size;

            auto  lval_load   = builder.CreateLoad(array_llvm_type, lval.org);
            auto  lval_data   = builder.CreateExtractValue(lval_load, 0);
            auto  lval_len    = builder.CreateExtractValue(lval_load, 1);

            auto  temp_size   = builder.CreateMul(rval.len, builder.getInt64(elem_size));
            auto  temp_data   = builder.CreateCall(LibC_malloc(gen), { temp_size });

            // Block
            auto  fn          = builder.GetInsertBlock()->getParent();
            auto  block_entry = builder.GetInsertBlock();
            auto  block_cond  = gen.BlockCreate(".array.write.cond", fn);
            auto  block_body  = gen.BlockCreate(".array.write.body", fn);
            auto  block_end   = gen.BlockCreate(".array.write.end",  fn);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(counter, rval.len), block_body, block_end);
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto src = ElemGet(gen, rval.data, elem_size, counter);
                auto dst = ElemGet(gen, temp_data, elem_size, counter);

                builder.CreateStore(
                    elem_type_impl->MethodCall(gen, "@copy", {
                        Arg(src, sema::TypeTable::ReferenceTypeGet(elem_type))
                    }),
                    dst
                );

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            // End Block
            builder.SetInsertPoint(block_end);
            auto len_common = builder.CreateSelect(
                builder.CreateICmpSLT(lval.memory.len, rval.len), lval.memory.len, rval.len
            );

            auto block_rep_cond = gen.BlockCreate(".array.write.rep.cond", fn);
            auto block_rep_body = gen.BlockCreate(".array.write.rep.body", fn);
            auto block_rep_end  = gen.BlockCreate(".array.write.rep.end",  fn);

            // Rep Cond Block
            builder.CreateBr(block_rep_cond);
            builder.SetInsertPoint(block_rep_cond);
            auto rep_counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                rep_counter->addIncoming(builder.getInt64(0), block_end);
                builder.CreateCondBr(builder.CreateICmpSLT(rep_counter, len_common), block_rep_body, block_rep_end);
            }

            // Rep Body Block
            builder.SetInsertPoint(block_rep_body);
            {
                auto idx = builder.CreateAdd(lval.offset, rep_counter);
                auto dst = ElemGet(gen, lval_data, elem_size, idx);

                elem_type_impl->MethodCall(gen, "@release", {
                    Arg(dst, sema::TypeTable::ReferenceTypeGet(elem_type))
                });
                builder.CreateStore(
                    elem_type_impl->MethodCall(gen, "@copy", {
                        Arg(ElemGet(gen, temp_data, elem_size, rep_counter), sema::TypeTable::ReferenceTypeGet(elem_type))
                    }),
                    dst
                );

                auto next = builder.CreateAdd(rep_counter, builder.getInt64(1));
                builder.CreateBr(block_rep_cond);
                rep_counter->addIncoming(next, builder.GetInsertBlock());
            }

            // Rep End Block
            builder.SetInsertPoint(block_rep_end);
            auto is_equal  = builder.CreateICmpEQ(rval.len, lval.memory.len);
            auto is_shrink = builder.CreateICmpSLT(rval.len, lval.memory.len);

            auto block_diff   = gen.BlockCreate(".array.write.diff",   fn);
            auto block_shrink = gen.BlockCreate(".array.write.shrink", fn);
            auto block_grow   = gen.BlockCreate(".array.write.grow",   fn);
            auto block_adjust = gen.BlockCreate(".array.write.adjust", fn);

            builder.CreateCondBr(is_equal, block_adjust, block_diff);

            // Diff Block
            builder.SetInsertPoint(block_diff);
            builder.CreateCondBr(is_shrink, block_shrink, block_grow);

            // Shrink Block
            builder.SetInsertPoint(block_shrink);
            {
                auto block_del_cond = gen.BlockCreate(".array.write.del.cond", fn);
                auto block_del_body = gen.BlockCreate(".array.write.del.body", fn);
                auto block_del_end  = gen.BlockCreate(".array.write.del.end",  fn);

                builder.CreateBr(block_del_cond);
                builder.SetInsertPoint(block_del_cond);
                auto del_counter = builder.CreatePHI(builder.getInt64Ty(), 2);
                {
                    del_counter->addIncoming(rval.len, block_shrink);
                    builder.CreateCondBr(builder.CreateICmpSLT(del_counter, lval.memory.len), block_del_body, block_del_end);
                }

                builder.SetInsertPoint(block_del_body);
                {
                    auto idx = builder.CreateAdd(lval.offset, del_counter);
                    
                    elem_type_impl->MethodCall(gen, "@release", {
                        Arg(ElemGet(gen, lval_data, elem_size, idx), sema::TypeTable::ReferenceTypeGet(elem_type))
                    });
                    
                    auto next = builder.CreateAdd(del_counter, builder.getInt64(1));
                    builder.CreateBr(block_del_cond);
                    del_counter->addIncoming(next, builder.GetInsertBlock());
                }

                builder.SetInsertPoint(block_del_end);
                {
                    auto move_cnt = builder.CreateSub(builder.CreateSub(lval_len, lval.offset), lval.memory.len);
                    Copy(
                        gen,
                        ElemGet(gen, lval_data, elem_size, builder.CreateAdd(lval.offset, rval.len)),
                        ElemGet(gen, lval_data, elem_size, builder.CreateAdd(lval.offset, lval.memory.len)),
                        move_cnt, elem_size
                    );

                    auto new_len  = builder.CreateSub(lval_len, builder.CreateSub(lval.memory.len, rval.len));
                    auto new_data = Realloc(gen, lval_data, elem_size, new_len);

                    builder.CreateStore(
                        gen.StructTypeValCreate(array_llvm_type, { new_data, new_len }),
                        lval.org
                    );
                    builder.CreateBr(block_adjust);
                }
            }

            // Grow Block
            builder.SetInsertPoint(block_grow);
            {
                auto new_len  = builder.CreateAdd(lval_len, builder.CreateSub(rval.len, lval.memory.len));
                auto new_data = Realloc(gen, lval_data, elem_size, new_len);

                auto move_cnt = builder.CreateSub(builder.CreateSub(lval_len, lval.offset), lval.memory.len);
                Copy(
                    gen,
                    ElemGet(gen, new_data, elem_size, builder.CreateAdd(lval.offset, rval.len)),
                    ElemGet(gen, new_data, elem_size, builder.CreateAdd(lval.offset, lval.memory.len)),
                    move_cnt, elem_size
                );

                auto block_cp_cond = gen.BlockCreate(".array.write.cp.cond", fn);
                auto block_cp_body = gen.BlockCreate(".array.write.cp.body", fn);
                auto block_cp_end  = gen.BlockCreate(".array.write.cp.end",  fn);

                builder.CreateBr(block_cp_cond);
                builder.SetInsertPoint(block_cp_cond);
                auto cp_counter = builder.CreatePHI(builder.getInt64Ty(), 2);
                {
                    cp_counter->addIncoming(lval.memory.len, block_grow);
                    builder.CreateCondBr(builder.CreateICmpSLT(cp_counter, rval.len), block_cp_body, block_cp_end);
                }

                builder.SetInsertPoint(block_cp_body);
                {
                    auto idx    = builder.CreateAdd(lval.offset, cp_counter);
                    auto dst    = ElemGet(gen, new_data, elem_size, idx);
                    
                    builder.CreateStore(
                        elem_type_impl->MethodCall(gen, "@copy", {
                            Arg(ElemGet(gen, temp_data, elem_size, cp_counter), sema::TypeTable::ReferenceTypeGet(elem_type))
                        }),
                        dst
                    );

                    auto next = builder.CreateAdd(cp_counter, builder.getInt64(1));
                    builder.CreateBr(block_cp_cond);
                    cp_counter->addIncoming(next, builder.GetInsertBlock());
                }

                builder.SetInsertPoint(block_cp_end);
                {
                    builder.CreateStore(
                        gen.StructTypeValCreate(array_llvm_type, { new_data, new_len }),
                        lval.org
                    );
                    builder.CreateBr(block_adjust);
                }
            }

            // Adjust Block
            builder.SetInsertPoint(block_adjust);
            builder.CreateCall(LibC_free(gen), { temp_data });
        }
    }

    // array

    void TypeImplTable::Init_array() {
        using ARGS       = const std::vector<Arg>&;

        auto  none_      = sema::TypeTable::Lookup("none");
        auto  i64_       = sema::TypeTable::Lookup("i64");
        auto  array_     = sema::TypeTable::Lookup("array");
        auto  arrayview_ = sema::TypeTable::Lookup("arrayview");
        auto  range_     = sema::TypeTable::Lookup("range");

        auto  impl       = TypeImplTable::Set(TypeImpl(array_));
        auto  T          = ((sema::BasicType*)array_)->params_binding()[0];

        // @copy and @release

        impl->MethodAdd("@copy",        [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder     = gen.llvm_builder();
            auto  arr         = ArrayLoad(gen, args[0]);

            // Result
            auto  result_size = builder.CreateMul(arr.memory.len, builder.getInt64(arr.elem_size));
            auto  result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            // Blocks
            auto  fn          = builder.GetInsertBlock()->getParent();
            auto  block_entry = builder.GetInsertBlock();
            auto  block_cond  = gen.BlockCreate(".array.copy.cond", fn);
            auto  block_body  = gen.BlockCreate(".array.copy.body", fn);
            auto  block_end   = gen.BlockCreate(".array.copy.end",  fn);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, arr.memory.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto elem_src    = ElemGet(gen, arr.memory.data, arr.elem_size, counter);
                auto elem_dst    = ElemGet(gen, result_data, arr.elem_size, counter);
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

            // Package
            return gen.StructTypeValCreate(
                arr.llvm_type, { result_data, arr.memory.len }
            );
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T })));
        impl->MethodAdd("@release",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder     = gen.llvm_builder();
            auto  arr         = ArrayLoad(gen, args[0]);

            // Blocks
            auto  fn          = builder.GetInsertBlock()->getParent();
            auto  block_entry = builder.GetInsertBlock();
            auto  block_cond  = gen.BlockCreate(".array.release.cond", fn);
            auto  block_body  = gen.BlockCreate(".array.release.body", fn);
            auto  block_end   = gen.BlockCreate(".array.release.end",  fn);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, arr.memory.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto elem = ElemGet(gen, arr.memory.data, arr.elem_size, counter);

                arr.elem_type_impl->MethodCall(gen, "@release", {
                    Arg(elem, sema::TypeTable::ReferenceTypeGet(arr.elem_type))
                });

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            // End Block
            builder.SetInsertPoint(block_end);
            builder.CreateCall(LibC_free(gen), { arr.memory.data });

            return nullptr;
        }, sema::FnSign(none_));

        // Other

        impl->MethodAdd("@print",       [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Print(gen, ArrayLoad(gen, args[0]));
        }, sema::FnSign(none_));
        impl->MethodAdd("@pick",        [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return PickIndex(gen, ArrayLoad(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(T), { i64_ }));
        impl->MethodAdd("@pick",        [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return PickRange(gen, ArrayLoad(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(arrayview_, { T }), { range_ }));

        impl->MethodAdd("@neg",         [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Reverse(gen, ArrayLoad(gen, args[0]));
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T })));
        impl->MethodAdd("@plus",        [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder      = gen.llvm_builder();
            auto  lval         = ArrayLoad(gen, args[0]);
            auto  rval         = ArrayLoad(gen, args[1]);

            auto  result_len   = builder.CreateAdd(lval.memory.len, rval.memory.len);
            auto  result_size  = builder.CreateMul(result_len, builder.getInt64(lval.elem_size));
            auto  result_data  = builder.CreateCall(LibC_malloc(gen), { result_size });

            auto  fn           = builder.GetInsertBlock()->getParent();
            auto  block_entry  = builder.GetInsertBlock();

            auto  block_l_cond = gen.BlockCreate(".array.plus.l.cond", fn);
            auto  block_l_body = gen.BlockCreate(".array.plus.l.body", fn);
            auto  block_l_end  = gen.BlockCreate(".array.plus.l.end",  fn);

            builder.CreateBr(block_l_cond);
            builder.SetInsertPoint(block_l_cond);
            auto l_counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                l_counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(l_counter, lval.memory.len), block_l_body, block_l_end);
            }

            builder.SetInsertPoint(block_l_body);
            {
                auto src = ElemGet(gen, lval.memory.data, lval.elem_size, l_counter);
                auto dst = ElemGet(gen, result_data, lval.elem_size, l_counter);
                
                builder.CreateStore(
                    lval.elem_type_impl->MethodCall(gen, "@copy", {
                        Arg(src, sema::TypeTable::ReferenceTypeGet(lval.elem_type))
                    }),
                    dst
                );

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
                builder.CreateCondBr(builder.CreateICmpSLT(r_counter, rval.memory.len), block_r_body, block_r_end);
            }

            builder.SetInsertPoint(block_r_body);
            {
                auto src = ElemGet(gen, rval.memory.data, rval.elem_size, r_counter);
                auto dst = ElemGet(gen, result_data, rval.elem_size, builder.CreateAdd(lval.memory.len, r_counter));
                builder.CreateStore(
                    rval.elem_type_impl->MethodCall(gen, "@copy", {
                        Arg(src, sema::TypeTable::ReferenceTypeGet(rval.elem_type))
                    }),
                    dst
                );

                auto next = builder.CreateAdd(r_counter, builder.getInt64(1));
                builder.CreateBr(block_r_cond);
                r_counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_r_end);

            return gen.StructTypeValCreate(
                lval.llvm_type, { result_data, result_len }
            );
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T }), { sema::TypeTable::ParametricTypeGet(array_, { T }) }));

        impl->MethodAdd("len",          [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return ArrayLoad(gen, args[0]).memory.len;
        }, sema::FnSign(i64_));
        impl->MethodAdd("clear",        [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = ArrayLoad(gen, args[0]);

            TypeImplTable::Lookup(args[0].ReferenceUnwrap())->MethodCall(
                gen, "@release", {
                    Arg(arr.addr, sema::TypeTable::ReferenceTypeGet(args[0].ReferenceUnwrap()))
                }
            );

            auto null_data = llvm::ConstantPointerNull::get(
                llvm::PointerType::get(gen.llvm_context(), 0)
            );
            Store(gen, arr.addr, { null_data, builder.getInt64(0) }, arr.llvm_type);
            return nullptr;
        }, sema::FnSign(none_));
        impl->MethodAdd("insert",       [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder     = gen.llvm_builder();
            auto  arr         = ArrayLoad(gen, args[0]);
            auto  idx         = args[1].val();
            auto  one         = builder.getInt64(1);

            auto  result_len  = builder.CreateAdd(arr.memory.len, one);
            auto  result_data = Realloc(gen, arr.memory.data, arr.elem_size, result_len);

            Copy(
                gen,
                ElemGet(gen, result_data, arr.elem_size, builder.CreateAdd(idx, one)),
                ElemGet(gen, result_data, arr.elem_size, idx),
                builder.CreateSub(arr.memory.len, idx),
                arr.elem_size
            );
            auto value_ref = gen.ArgRefMake(args[2].val(), arr.elem_type);
            auto copied    = arr.elem_type_impl->MethodCall(gen, "@copy", { value_ref });
            builder.CreateStore(copied, ElemGet(gen, result_data, arr.elem_size, idx));
            Store(gen, arr.addr, { result_data, result_len }, arr.llvm_type);
            return nullptr;
        }, sema::FnSign(none_, { i64_, nullptr }));
        impl->MethodAdd("remove",       [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = ArrayLoad(gen, args[0]);
            auto  idx     = args[1].val();
            auto  one     = builder.getInt64(1);

            Copy(
                gen,
                ElemGet(gen, arr.memory.data, arr.elem_size, idx),
                ElemGet(gen, arr.memory.data, arr.elem_size, builder.CreateAdd(idx, one)),
                builder.CreateSub(builder.CreateSub(arr.memory.len, one), idx),
                arr.elem_size
            );
            Store(gen, arr.addr, { arr.memory.data, builder.CreateSub(arr.memory.len, one) }, arr.llvm_type);
            return nullptr;
        }, sema::FnSign(none_, { i64_ }));
        impl->MethodAdd("push_back",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder     = gen.llvm_builder();
            auto  arr         = ArrayLoad(gen, args[0]);

            auto  result_len  = builder.CreateAdd(arr.memory.len, builder.getInt64(1));
            auto  result_data = Realloc(gen, arr.memory.data, arr.elem_size, result_len);

            auto value_ref    = gen.ArgRefMake(args[1].val(), arr.elem_type);
            auto copied       = arr.elem_type_impl->MethodCall(gen, "@copy", { value_ref });
            builder.CreateStore(copied, ElemGet(
                gen, result_data, arr.elem_size, arr.memory.len
            ));
            Store(gen, arr.addr, { result_data, result_len }, arr.llvm_type);
            return nullptr;
        }, sema::FnSign(none_, { nullptr }));
        impl->MethodAdd("push_front",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder     = gen.llvm_builder();
            auto  arr         = ArrayLoad(gen, args[0]);
            auto  one         = builder.getInt64(1);

            auto  result_len  = builder.CreateAdd(arr.memory.len, one);
            auto  result_data = Realloc(gen, arr.memory.data, arr.elem_size, result_len);

            Copy(
                gen,
                ElemGet(gen, result_data, arr.elem_size, one),
                result_data, arr.memory.len, arr.elem_size
            );
            auto value_ref    = gen.ArgRefMake(args[1].val(), arr.elem_type);
            auto copied       = arr.elem_type_impl->MethodCall(gen, "@copy", { value_ref });
            builder.CreateStore(copied, ElemGet(gen, result_data, arr.elem_size, builder.getInt64(0)));
            Store(gen, arr.addr, { result_data, result_len }, arr.llvm_type);
            return nullptr;
        }, sema::FnSign(none_, { nullptr }));
        impl->MethodAdd("pop_back",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = ArrayLoad(gen, args[0]);

            Store(
                gen, arr.addr,
                { arr.memory.data, builder.CreateSub(arr.memory.len, builder.getInt64(1)) },
                arr.llvm_type
            );
            return nullptr;
        }, sema::FnSign(none_));
        impl->MethodAdd("pop_front",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = ArrayLoad(gen, args[0]);
            auto  one     = builder.getInt64(1);

            Copy(
                gen, arr.memory.data,
                ElemGet(gen, arr.memory.data, arr.elem_size, one),
                builder.CreateSub(arr.memory.len, one), arr.elem_size
            );
            Store(gen, arr.addr, { arr.memory.data, builder.CreateSub(arr.memory.len, one) }, arr.llvm_type);
            return nullptr;
        }, sema::FnSign(none_));
    }

    // arrayview

    void TypeImplTable::Init_arrayview() {
        using ARGS       = const std::vector<Arg>&;

        auto  none_      = sema::TypeTable::Lookup("none");
        auto  i64_       = sema::TypeTable::Lookup("i64");
        auto  array_     = sema::TypeTable::Lookup("array");
        auto  arrayview_ = sema::TypeTable::Lookup("arrayview");
        auto  range_     = sema::TypeTable::Lookup("range");

        auto  impl       = TypeImplTable::Set(TypeImpl(arrayview_));
        auto  T          = ((sema::BasicType*)arrayview_)->params_binding()[0];

        // @cast

        impl->MethodAdd("@cast",    [array_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder     = gen.llvm_builder();
            auto  view        = ArrayViewLoad(gen, args[0]);

            auto  new_size    = builder.CreateMul(view.memory.len, builder.getInt64(view.elem_size));
            auto  new_data    = builder.CreateCall(LibC_malloc(gen), { new_size });

            // Blocks
            auto  fn          = builder.GetInsertBlock()->getParent();
            auto  block_entry = builder.GetInsertBlock();
            auto  block_cond  = gen.BlockCreate(".arrayview.cast.cond", fn);
            auto  block_body  = gen.BlockCreate(".arrayview.cast.body", fn);
            auto  block_end   = gen.BlockCreate(".arrayview.cast.end",  fn);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(counter, view.memory.len), block_body, block_end);
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto src_idx  = builder.CreateAdd(view.offset, counter);
                auto elem_src = ElemGet(gen, view.memory.data, view.elem_size, src_idx);
                auto elem_dst = ElemGet(gen, new_data, view.elem_size, counter);
                auto copied   = view.elem_type_impl->MethodCall(gen, "@copy", {
                    Arg(elem_src, sema::TypeTable::ReferenceTypeGet(view.elem_type))
                });
                builder.CreateStore(copied, elem_dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            // End Block
            builder.SetInsertPoint(block_end);

            return gen.StructTypeValCreate(
                gen.LLVMType(array_), { new_data, view.memory.len }
            );
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T }), {}, std::nullopt, sema::FnModifier::Cast));

        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(arrayview_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // @assign

        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder   = gen.llvm_builder();
            auto  lval      = ArrayViewLoad(gen, args[0]);

            auto  rval      = gen.ArgLoad(args[1]);
            auto  rval_data = builder.CreateExtractValue(rval, 0);
            auto  rval_len  = builder.CreateExtractValue(rval, 1);

            Write(gen, lval, { rval_data, rval_len });
            return nullptr;
        }, sema::FnSign(none_, { sema::TypeTable::ParametricTypeGet(array_, { T }) }));
        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = ArrayViewLoad(gen, args[0]);
            auto rval = ArrayViewLoad(gen, args[1]);

            Write(gen, lval, rval.memory);
            return nullptr;
        }, sema::FnSign(none_, { sema::TypeTable::ParametricTypeGet(arrayview_, { T }) }));
        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder  = gen.llvm_builder();
            auto  lval     = ArrayViewLoad(gen, args[0]);
            auto  rval     = gen.ArgLoad(args[1]);
            auto  rval_ref = gen.ArgRefMake(rval, lval.elem_type);

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".arrayview.fill.cond", fn);
            auto block_body  = gen.BlockCreate(".arrayview.fill.body", fn);
            auto block_end   = gen.BlockCreate(".arrayview.fill.end",  fn);

            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(counter, lval.memory.len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto idx = builder.CreateAdd(lval.offset, counter);
                auto dst = ElemGet(gen, lval.memory.data, lval.elem_size, idx);
                
                lval.elem_type_impl->MethodCall(gen, "@release", {
                    Arg(dst, sema::TypeTable::ReferenceTypeGet(lval.elem_type))
                });
                builder.CreateStore(
                    lval.elem_type_impl->MethodCall(gen, "@copy", { rval_ref }),
                    dst
                );

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);
            return nullptr;
        }, sema::FnSign(none_, { T }));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Print(gen, ArrayViewLoad(gen, args[0]));
        }, sema::FnSign(none_));
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return PickIndex(gen, ArrayViewLoad(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(T), { i64_ }));
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return PickRange(gen, ArrayViewLoad(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(arrayview_, { T }), { range_ }));

        impl->MethodAdd("@neg",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Reverse(gen, ArrayViewLoad(gen, args[0]));
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T })));
        impl->MethodAdd("len",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return ArrayViewLoad(gen, args[0]).memory.len;
        }, sema::FnSign(i64_));
    }
}

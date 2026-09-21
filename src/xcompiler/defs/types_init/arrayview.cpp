
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    void TypeImplTable::Init_arrayview() {
        using ARGS       = const std::vector<Arg>&;

        auto  none_      = sema::TypeTable::Lookup("none");
        auto  i64_       = sema::TypeTable::Lookup("i64");
        auto  array_     = sema::TypeTable::Lookup("array");
        auto  arrayview_ = sema::TypeTable::Lookup("arrayview");
        auto  range_     = sema::TypeTable::Lookup("range");

        auto  impl       = TypeImplTable::Set(TypeImpl(arrayview_));
        auto  T          = ((sema::BasicType*)arrayview_)->params_binding()[0];

        // Utility

        struct ViewInfo {
            llvm::Value* addr      = nullptr;
            llvm::Value* org       = nullptr;
            llvm::Value* offset    = nullptr;
            llvm::Value* len       = nullptr;
            llvm::Type*  llvm_type = nullptr;
        };

        auto view_load      = [](IRGen& gen, const Arg& arg) -> ViewInfo {
            auto& builder   = gen.llvm_builder();
            auto  addr      = gen.ArgAddr(arg);
            auto  llvm_type = gen.LLVMType(arg.ReferenceUnwrap());
            auto  val       = builder.CreateLoad(llvm_type, addr);

            return {
                addr,
                builder.CreateExtractValue(val, 0),
                builder.CreateExtractValue(val, 1),
                builder.CreateExtractValue(val, 2),
                llvm_type
            };
        };
        auto view_realloc   = [](IRGen& gen, llvm::Value* data, size_t elem_size, llvm::Value* new_len) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  size    = builder.CreateMul(new_len, builder.getInt64(elem_size));
            return builder.CreateCall(LibC_realloc(gen), { data, size });
        };
        auto view_elem_get  = [](IRGen& gen, llvm::Value* data, size_t elem_size, llvm::Value* idx) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  offset  = builder.CreateMul(idx, builder.getInt64(elem_size));
            return builder.CreateInBoundsGEP(builder.getInt8Ty(), data, { offset });
        };
        auto view_elem_move = [](IRGen& gen, llvm::Value* dst, llvm::Value* src, llvm::Value* cnt, size_t elem_size) {
            auto& builder = gen.llvm_builder();
            auto  bytes   = builder.CreateMul(cnt, builder.getInt64(elem_size));
            builder.CreateCall(LibC_memmove(gen), { dst, src, bytes });
        };
        auto assign_core    = [view_realloc, view_elem_get, view_elem_move, array_](
            IRGen& gen, const ViewInfo& view,
            llvm::Value* right_data, llvm::Value* right_len,
            sema::Type* elem_type, TypeImpl* elem_type_impl, size_t elem_size) -> void
        {
            auto& builder = gen.llvm_builder();

            auto arr_val  = builder.CreateLoad(gen.LLVMType(array_), view.org);
            auto arr_data = builder.CreateExtractValue(arr_val, 0);
            auto arr_len  = builder.CreateExtractValue(arr_val, 1);

            auto temp_size = builder.CreateMul(right_len, builder.getInt64(elem_size));
            auto temp_data = builder.CreateCall(LibC_malloc(gen), { temp_size });

            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".arrayview.assign.cond", fn);
            auto block_body  = gen.BlockCreate(".arrayview.assign.body", fn);
            auto block_end   = gen.BlockCreate(".arrayview.assign.end",  fn);

            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(counter, right_len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto src    = view_elem_get(gen, right_data, elem_size, counter);
                auto dst    = view_elem_get(gen, temp_data,  elem_size, counter);
                auto copied = elem_type_impl->MethodCall(gen, "@copy", {
                    Arg(src, sema::TypeTable::ReferenceTypeGet(elem_type))
                });
                builder.CreateStore(copied, dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);

            auto common = builder.CreateSelect(
                builder.CreateICmpSLT(view.len, right_len), view.len, right_len
            );

            auto block_rep_cond = gen.BlockCreate(".arrayview.assign.rep.cond", fn);
            auto block_rep_body = gen.BlockCreate(".arrayview.assign.rep.body", fn);
            auto block_rep_end  = gen.BlockCreate(".arrayview.assign.rep.end",  fn);

            builder.CreateBr(block_rep_cond);
            builder.SetInsertPoint(block_rep_cond);
            auto rep_counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                rep_counter->addIncoming(builder.getInt64(0), block_end);
                builder.CreateCondBr(builder.CreateICmpSLT(rep_counter, common), block_rep_body, block_rep_end);
            }

            builder.SetInsertPoint(block_rep_body);
            {
                auto idx = builder.CreateAdd(view.offset, rep_counter);
                auto dst = view_elem_get(gen, arr_data, elem_size, idx);
                elem_type_impl->MethodCall(gen, "@release", {
                    Arg(dst, sema::TypeTable::ReferenceTypeGet(elem_type))
                });
                auto copied = elem_type_impl->MethodCall(gen, "@copy", {
                    Arg(view_elem_get(gen, temp_data, elem_size, rep_counter), sema::TypeTable::ReferenceTypeGet(elem_type))
                });
                builder.CreateStore(copied, dst);

                auto next = builder.CreateAdd(rep_counter, builder.getInt64(1));
                builder.CreateBr(block_rep_cond);
                rep_counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_rep_end);

            auto is_equal  = builder.CreateICmpEQ(right_len, view.len);
            auto is_shrink = builder.CreateICmpSLT(right_len, view.len);

            auto block_diff   = gen.BlockCreate(".arrayview.assign.diff",   fn);
            auto block_shrink = gen.BlockCreate(".arrayview.assign.shrink", fn);
            auto block_grow   = gen.BlockCreate(".arrayview.assign.grow",   fn);
            auto block_adjust = gen.BlockCreate(".arrayview.assign.adjust", fn);

            builder.CreateCondBr(is_equal, block_adjust, block_diff);

            builder.SetInsertPoint(block_diff);
            builder.CreateCondBr(is_shrink, block_shrink, block_grow);

            builder.SetInsertPoint(block_shrink);
            {
                auto block_del_cond = gen.BlockCreate(".arrayview.assign.del.cond", fn);
                auto block_del_body = gen.BlockCreate(".arrayview.assign.del.body", fn);
                auto block_del_end  = gen.BlockCreate(".arrayview.assign.del.end",  fn);

                builder.CreateBr(block_del_cond);
                builder.SetInsertPoint(block_del_cond);
                auto del_counter = builder.CreatePHI(builder.getInt64Ty(), 2);
                {
                    del_counter->addIncoming(right_len, block_shrink);
                    builder.CreateCondBr(builder.CreateICmpSLT(del_counter, view.len), block_del_body, block_del_end);
                }

                builder.SetInsertPoint(block_del_body);
                {
                    auto idx = builder.CreateAdd(view.offset, del_counter);
                    elem_type_impl->MethodCall(gen, "@release", {
                        Arg(view_elem_get(gen, arr_data, elem_size, idx), sema::TypeTable::ReferenceTypeGet(elem_type))
                    });
                    auto next = builder.CreateAdd(del_counter, builder.getInt64(1));
                    builder.CreateBr(block_del_cond);
                    del_counter->addIncoming(next, builder.GetInsertBlock());
                }

                builder.SetInsertPoint(block_del_end);
                {
                    auto move_cnt = builder.CreateSub(builder.CreateSub(arr_len, view.offset), view.len);
                    view_elem_move(
                        gen,
                        view_elem_get(gen, arr_data, elem_size, builder.CreateAdd(view.offset, right_len)),
                        view_elem_get(gen, arr_data, elem_size, builder.CreateAdd(view.offset, view.len)),
                        move_cnt, elem_size
                    );

                    auto new_len  = builder.CreateSub(arr_len, builder.CreateSub(view.len, right_len));
                    auto new_data = view_realloc(gen, arr_data, elem_size, new_len);

                    auto gen_val = (llvm::Value*)llvm::UndefValue::get(gen.LLVMType(array_));
                    gen_val = builder.CreateInsertValue(gen_val, new_data, 0);
                    gen_val = builder.CreateInsertValue(gen_val, new_len,  1);
                    builder.CreateStore(gen_val, view.org);
                    builder.CreateBr(block_adjust);
                }
            }

            builder.SetInsertPoint(block_grow);
            {
                auto new_len  = builder.CreateAdd(arr_len, builder.CreateSub(right_len, view.len));
                auto new_data = view_realloc(gen, arr_data, elem_size, new_len);

                auto move_cnt = builder.CreateSub(builder.CreateSub(arr_len, view.offset), view.len);
                view_elem_move(
                    gen,
                    view_elem_get(gen, new_data, elem_size, builder.CreateAdd(view.offset, right_len)),
                    view_elem_get(gen, new_data, elem_size, builder.CreateAdd(view.offset, view.len)),
                    move_cnt, elem_size
                );

                auto block_cp_cond = gen.BlockCreate(".arrayview.assign.cp.cond", fn);
                auto block_cp_body = gen.BlockCreate(".arrayview.assign.cp.body", fn);
                auto block_cp_end  = gen.BlockCreate(".arrayview.assign.cp.end",  fn);

                builder.CreateBr(block_cp_cond);
                builder.SetInsertPoint(block_cp_cond);
                auto cp_counter = builder.CreatePHI(builder.getInt64Ty(), 2);
                {
                    cp_counter->addIncoming(view.len, block_grow);
                    builder.CreateCondBr(builder.CreateICmpSLT(cp_counter, right_len), block_cp_body, block_cp_end);
                }

                builder.SetInsertPoint(block_cp_body);
                {
                    auto idx    = builder.CreateAdd(view.offset, cp_counter);
                    auto dst    = view_elem_get(gen, new_data, elem_size, idx);
                    auto copied = elem_type_impl->MethodCall(gen, "@copy", {
                        Arg(view_elem_get(gen, temp_data, elem_size, cp_counter), sema::TypeTable::ReferenceTypeGet(elem_type))
                    });
                    builder.CreateStore(copied, dst);

                    auto next = builder.CreateAdd(cp_counter, builder.getInt64(1));
                    builder.CreateBr(block_cp_cond);
                    cp_counter->addIncoming(next, builder.GetInsertBlock());
                }

                builder.SetInsertPoint(block_cp_end);
                {
                    auto gen_val = (llvm::Value*)llvm::UndefValue::get(gen.LLVMType(array_));
                    gen_val = builder.CreateInsertValue(gen_val, new_data, 0);
                    gen_val = builder.CreateInsertValue(gen_val, new_len,  1);
                    builder.CreateStore(gen_val, view.org);
                    builder.CreateBr(block_adjust);
                }
            }

            builder.SetInsertPoint(block_adjust);
            builder.CreateCall(LibC_free(gen), { temp_data });
        };

        // @cast
        
        impl->MethodAdd("@cast",    [view_load, array_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);

            auto  view_type      = (sema::ParametricType*)args[0].ReferenceUnwrap();
            auto  elem_type      = view_type->params_type()[0];
            auto  elem_type_impl = TypeImplTable::Lookup(elem_type);
            auto  elem_size      = gen.llvm_module()->getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));

            auto arr_val = builder.CreateLoad(gen.LLVMType(array_), view.org);
            auto data    = builder.CreateExtractValue(arr_val, 0);

            auto new_size = builder.CreateMul(view.len, builder.getInt64(elem_size));
            auto new_data = builder.CreateCall(LibC_malloc(gen), { new_size });

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".arrayview.cast.cond", fn);
            auto block_body  = gen.BlockCreate(".arrayview.cast.body", fn);
            auto block_end   = gen.BlockCreate(".arrayview.cast.end",  fn);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(counter, view.len), block_body, block_end);
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto src_idx  = builder.CreateAdd(view.offset, counter);
                auto src_off  = builder.CreateMul(src_idx, builder.getInt64(elem_size));
                auto dst_off  = builder.CreateMul(counter, builder.getInt64(elem_size));
                auto elem_src = builder.CreateInBoundsGEP(builder.getInt8Ty(), data, { src_off });
                auto elem_dst = builder.CreateInBoundsGEP(builder.getInt8Ty(), new_data, { dst_off });
                auto copied   = elem_type_impl->MethodCall(gen, "@copy", {
                    Arg(elem_src, sema::TypeTable::ReferenceTypeGet(elem_type))
                });
                builder.CreateStore(copied, elem_dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            // End Block
            builder.SetInsertPoint(block_end);

            auto gen_type = gen.LLVMType(array_);
            auto gen_val  = (llvm::Value*)llvm::UndefValue::get(gen_type);
            gen_val = builder.CreateInsertValue(gen_val, new_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, view.len,  1);
            return gen_val;
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T }), {}, std::nullopt, sema::FnModifier::Cast));

        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(arrayview_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // @assign

        impl->MethodAdd("@assign",  [view_load, assign_core, array_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);

            auto  value      = gen.ArgLoad(args[1]);
            auto  value_data = builder.CreateExtractValue(value, 0);
            auto  value_len  = builder.CreateExtractValue(value, 1);

            auto  view_type      = (sema::ParametricType*)args[0].ReferenceUnwrap();
            auto  elem_type      = view_type->params_type()[0];
            auto  elem_type_impl = TypeImplTable::Lookup(elem_type);
            auto  elem_size      = gen.llvm_module()->getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));

            assign_core(gen, view, value_data, value_len, elem_type, elem_type_impl, elem_size);
            return nullptr;
        }, sema::FnSign(none_, { sema::TypeTable::ParametricTypeGet(array_, { T }) }));
        impl->MethodAdd("@assign",  [view_load, assign_core, array_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);
            auto  right   = view_load(gen, args[1]);

            auto  arr_val    = builder.CreateLoad(gen.LLVMType(array_), right.org);
            auto  right_data = builder.CreateExtractValue(arr_val, 0);
            auto  right_len  = right.len;

            auto  view_type      = (sema::ParametricType*)args[0].ReferenceUnwrap();
            auto  elem_type      = view_type->params_type()[0];
            auto  elem_type_impl = TypeImplTable::Lookup(elem_type);
            auto  elem_size      = gen.llvm_module()->getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));

            assign_core(gen, view, right_data, right_len, elem_type, elem_type_impl, elem_size);
            return nullptr;
        }, sema::FnSign(none_, { sema::TypeTable::ParametricTypeGet(arrayview_, { T }) }));
        impl->MethodAdd("@assign",  [view_load, view_elem_get, array_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);
            auto  value   = gen.ArgLoad(args[1]);

            auto  view_type      = (sema::ParametricType*)args[0].ReferenceUnwrap();
            auto  elem_type      = view_type->params_type()[0];
            auto  elem_type_impl = TypeImplTable::Lookup(elem_type);
            auto  elem_size      = gen.llvm_module()->getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));

            auto  value_ref = gen.ArgRefMake(value, elem_type);

            auto arr_val  = builder.CreateLoad(gen.LLVMType(array_), view.org);
            auto arr_data = builder.CreateExtractValue(arr_val, 0);

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
                builder.CreateCondBr(builder.CreateICmpSLT(counter, view.len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto idx = builder.CreateAdd(view.offset, counter);
                auto dst = view_elem_get(gen, arr_data, elem_size, idx);
                elem_type_impl->MethodCall(gen, "@release", {
                    Arg(dst, sema::TypeTable::ReferenceTypeGet(elem_type))
                });
                auto copied = elem_type_impl->MethodCall(gen, "@copy", { value_ref });
                builder.CreateStore(copied, dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);
            return nullptr;
        }, sema::FnSign(none_, { T }));

        // Other

        impl->MethodAdd("@print",   [view_load, array_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);

            auto  view_type      = (sema::ParametricType*)args[0].ReferenceUnwrap();
            auto  elem_type      = view_type->params_type()[0];
            auto  elem_type_impl = TypeImplTable::Lookup(elem_type);
            auto  elem_size      = gen.llvm_module()->getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));

            auto arr_val = builder.CreateLoad(gen.LLVMType(array_), view.org);
            auto data    = builder.CreateExtractValue(arr_val, 0);

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".arrayview.print.cond",  fn);
            auto block_cont  = gen.BlockCreate(".arrayview.print.cont",  fn);
            auto block_sep   = gen.BlockCreate(".arrayview.print.sep",   fn);
            auto block_nosep = gen.BlockCreate(".arrayview.print.nosep", fn);
            auto block_body  = gen.BlockCreate(".arrayview.print.body",  fn);
            auto block_end   = gen.BlockCreate(".arrayview.print.end",   fn);

            builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString("[", ".arrayview.lb") });

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, view.len), block_cont, block_end
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
                builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString(", ", ".arrayview.sep") });
                builder.CreateBr(block_body);
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto idx    = builder.CreateAdd(view.offset, counter);
                auto offset = builder.CreateMul(idx, builder.getInt64(elem_size));
                auto elem   = builder.CreateInBoundsGEP(builder.getInt8Ty(), data, { offset });
                elem_type_impl->MethodCall(gen, "@print", {
                    Arg(elem, sema::TypeTable::ReferenceTypeGet(elem_type))
                });

                auto body_end = builder.GetInsertBlock();
                auto next     = builder.CreateAdd(counter, builder.getInt64(1));
                counter->addIncoming(next, body_end);

                builder.CreateBr(block_cond);
            }

            // End Block
            builder.SetInsertPoint(block_end);
            {
                builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString("]", ".arrayview.rb") });
            }

            return nullptr;
        }, sema::FnSign(none_));

        impl->MethodAdd("@pick",    [view_load, array_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);
            auto  idx     = gen.ArgLoad(args[1]);

            auto  view_type = (sema::ParametricType*)args[0].ReferenceUnwrap();
            auto  elem_type = view_type->params_type()[0];
            auto  elem_size = gen.llvm_module()->getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));

            auto arr_val = builder.CreateLoad(gen.LLVMType(array_), view.org);
            auto data    = builder.CreateExtractValue(arr_val, 0);

            auto abs_idx  = builder.CreateAdd(view.offset, idx);
            auto byte_off = builder.CreateMul(abs_idx, builder.getInt64(elem_size));
            return builder.CreateInBoundsGEP(builder.getInt8Ty(), data, { byte_off });
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(T), { i64_ }));
        impl->MethodAdd("@pick",    [view_load, arrayview_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder  = gen.llvm_builder();
            auto  view     = view_load(gen, args[0]);
            auto  range    = gen.ArgLoad(args[1]);

            auto left      = builder.CreateIntCast(builder.CreateExtractValue(range, 0), builder.getInt64Ty(), true);
            auto right     = builder.CreateIntCast(builder.CreateExtractValue(range, 1), builder.getInt64Ty(), true);
            auto is_closed = builder.CreateExtractValue(range, 3);

            auto offset = builder.CreateAdd(view.offset, left);
            auto diff   = builder.CreateSub(right, left);
            auto len    = builder.CreateSelect(is_closed, builder.CreateAdd(diff, builder.getInt64(1)), diff);

            auto view_type = gen.LLVMType(arrayview_);
            auto gen_val   = (llvm::Value*)llvm::UndefValue::get(view_type);
            gen_val = builder.CreateInsertValue(gen_val, view.org, 0);
            gen_val = builder.CreateInsertValue(gen_val, offset,   1);
            gen_val = builder.CreateInsertValue(gen_val, len,      2);
            return gen_val;
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(arrayview_, { T }), { range_ }));

        impl->MethodAdd("@neg",     [view_load, view_elem_get, array_](IRGen& gen, ARGS& args)
         -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);

            auto  view_type      = (sema::ParametricType*)args[0].ReferenceUnwrap();
            auto  elem_type      = view_type->params_type()[0];
            auto  elem_type_impl = TypeImplTable::Lookup(elem_type);
            auto  elem_size      = gen.llvm_module()->getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));

            auto arr_val  = builder.CreateLoad(gen.LLVMType(array_), view.org);
            auto arr_data = builder.CreateExtractValue(arr_val, 0);

            auto result_size = builder.CreateMul(view.len, builder.getInt64(elem_size));
            auto result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".arrayview.neg.cond", fn);
            auto block_body  = gen.BlockCreate(".arrayview.neg.body", fn);
            auto block_end   = gen.BlockCreate(".arrayview.neg.end",  fn);

            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(counter, view.len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto src_idx     = builder.CreateAdd(view.offset, builder.CreateSub(builder.CreateSub(view.len, builder.getInt64(1)), counter));
                auto elem_src    = view_elem_get(gen, arr_data, elem_size, src_idx);
                auto elem_dst    = view_elem_get(gen, result_data, elem_size, counter);
                auto elem_copied = elem_type_impl->MethodCall(gen, "@copy", {
                    Arg(elem_src, sema::TypeTable::ReferenceTypeGet(elem_type))
                });
                builder.CreateStore(elem_copied, elem_dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);

            auto gen_type = gen.LLVMType(array_);
            auto gen_val  = (llvm::Value*)llvm::UndefValue::get(gen_type);
            gen_val = builder.CreateInsertValue(gen_val, result_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, view.len,    1);
            return gen_val;
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T })));
    
        impl->MethodAdd("len",      [view_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            return view_load(gen, args[0]).len;
        }, sema::FnSign(i64_));
    }
}

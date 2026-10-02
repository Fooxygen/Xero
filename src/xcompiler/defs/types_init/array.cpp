
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

        struct ArrayInfo {
            llvm::Value* addr           = nullptr;
            llvm::Value* org            = nullptr;
            llvm::Value* data           = nullptr;
            llvm::Value* offset         = nullptr;
            llvm::Value* len            = nullptr;
            llvm::Type*  llvm_type      = nullptr;

            sema::Type*  elem_type      = nullptr;
            TypeImpl*    elem_type_impl = nullptr;
            size_t       elem_size      = 0;
        };

        ArrayInfo Load_array(IRGen& gen, const Arg& arg) {
            auto& builder   = gen.llvm_builder();
            auto& module    = *gen.llvm_module();

            auto  addr      = gen.ArgAddr(arg);
            auto  type      = arg.ReferenceUnwrap();
            auto  llvm_type = gen.LLVMType(type->BasicTypeGet());
            auto  val       = builder.CreateLoad(llvm_type, addr);

            sema::Type* elem_type      = nullptr;
            TypeImpl*   elem_type_impl = nullptr;
            size_t      elem_size      = 0;

            if (auto parametric_type = dynamic_cast<sema::ParametricType*>(type)) {
                elem_type      = parametric_type->params()[0];
                elem_type_impl = TypeImplTable::Lookup(elem_type);
                elem_size      = module.getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));
            }

            return {
                addr, addr,
                builder.CreateExtractValue(val, 0),
                builder.getInt64(0),
                builder.CreateExtractValue(val, 1),
                llvm_type,
                elem_type, elem_type_impl, elem_size
            };
        }

        ArrayInfo Load_arrayview(IRGen& gen, const Arg& arg) {
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

            return {
                addr, org, data, offset, len, llvm_type,
                elem_type, elem_type_impl, elem_size
            };
        }

        void Store_array(
            IRGen& gen, llvm::Value* addr, llvm::Value* data,
            llvm::Type* llvm_type, llvm::Value* len
        ) {
            auto& builder = gen.llvm_builder();
            auto  gen_val = (llvm::Value*)llvm::UndefValue::get(llvm_type);
            gen_val = builder.CreateInsertValue(gen_val, data, 0);
            gen_val = builder.CreateInsertValue(gen_val, len, 1);
            builder.CreateStore(gen_val, addr);
        }

        llvm::Value* Get_elem(IRGen& gen, llvm::Value* data, size_t elem_size, llvm::Value* idx) {
            auto& builder = gen.llvm_builder();
            auto  offset  = builder.CreateMul(idx, builder.getInt64(elem_size));
            return builder.CreateInBoundsGEP(builder.getInt8Ty(), data, { offset });
        }

        llvm::Value* Realloc_array(IRGen& gen, llvm::Value* data, size_t elem_size, llvm::Value* new_len) {
            auto& builder = gen.llvm_builder();
            auto  size    = builder.CreateMul(new_len, builder.getInt64(elem_size));
            return builder.CreateCall(LibC_realloc(gen), { data, size });
        }

        void Move_array(IRGen& gen, llvm::Value* dst, llvm::Value* src, llvm::Value* cnt, size_t elem_size) {
            auto& builder = gen.llvm_builder();
            auto  bytes   = builder.CreateMul(cnt, builder.getInt64(elem_size));
            builder.CreateCall(LibC_memmove(gen), { dst, src, bytes });
        }

        llvm::Value* Print_array(IRGen& gen, const ArrayInfo& info) {
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
                    builder.CreateICmpSLT(counter, info.len), block_cont, block_end
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
                auto elem = Get_elem(gen, info.data, info.elem_size, idx);
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

        llvm::Value* Pick_elem(IRGen& gen, const ArrayInfo& info, llvm::Value* idx) {
            auto& builder = gen.llvm_builder();
            return Get_elem(gen, info.data, info.elem_size, builder.CreateAdd(info.offset, idx));
        }

        llvm::Value* Pick_arrayview(IRGen& gen, const ArrayInfo& info, llvm::Value* range) {
            auto& builder  = gen.llvm_builder();

            auto left      = builder.CreateIntCast(builder.CreateExtractValue(range, 0), builder.getInt64Ty(), true);
            auto right     = builder.CreateIntCast(builder.CreateExtractValue(range, 1), builder.getInt64Ty(), true);
            auto is_closed = builder.CreateExtractValue(range, 3);

            auto diff      = builder.CreateSub(right, left);
            auto len       = builder.CreateSelect(is_closed, builder.CreateAdd(diff, builder.getInt64(1)), diff);
            auto offset    = builder.CreateAdd(info.offset, left);

            auto view_type = gen.LLVMType(sema::TypeTable::Lookup("arrayview"));
            auto gen_val   = (llvm::Value*)llvm::UndefValue::get(view_type);
            gen_val = builder.CreateInsertValue(gen_val, info.org, 0);
            gen_val = builder.CreateInsertValue(gen_val, offset,   1);
            gen_val = builder.CreateInsertValue(gen_val, len,      2);
            return gen_val;
        }

        llvm::Value* Reverse_array(IRGen& gen, const ArrayInfo& info) {
            auto& builder = gen.llvm_builder();

            // Result
            auto result_size = builder.CreateMul(info.len, builder.getInt64(info.elem_size));
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
                    builder.CreateICmpSLT(counter, info.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto dst_idx     = builder.CreateSub(builder.CreateSub(info.len, builder.getInt64(1)), counter);
                auto elem_src    = Get_elem(gen, info.data, info.elem_size, builder.CreateAdd(info.offset, counter));
                auto elem_dst    = Get_elem(gen, result_data, info.elem_size, dst_idx);
                auto elem_copied = info.elem_type_impl->MethodCall(gen, "@copy", {
                    Arg(elem_src, sema::TypeTable::ReferenceTypeGet(info.elem_type))
                });
                builder.CreateStore(elem_copied, elem_dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            // End Block
            builder.SetInsertPoint(block_end);

            // Generated Value
            auto gen_val = (llvm::Value*)llvm::UndefValue::get(
                gen.LLVMType(sema::TypeTable::Lookup("array"))
            );
            gen_val = builder.CreateInsertValue(gen_val, result_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, info.len,    1);
            return gen_val;
        }

        void Write_array(
            IRGen& gen, const ArrayInfo& view,
            llvm::Value* right_data, llvm::Value* right_len
        ) {
            auto& builder     = gen.llvm_builder();
            auto  array_      = sema::TypeTable::Lookup("array");
            auto  elem_type   = view.elem_type;
            auto  elem_impl   = view.elem_type_impl;
            auto  elem_size   = view.elem_size;

            auto  arr_val     = builder.CreateLoad(gen.LLVMType(array_), view.org);
            auto arr_data     = builder.CreateExtractValue(arr_val, 0);
            auto  arr_len     = builder.CreateExtractValue(arr_val, 1);

            auto  temp_size   = builder.CreateMul(right_len, builder.getInt64(elem_size));
            auto  temp_data   = builder.CreateCall(LibC_malloc(gen), { temp_size });

            auto  fn          = builder.GetInsertBlock()->getParent();
            auto  block_entry = builder.GetInsertBlock();
            auto  block_cond  = gen.BlockCreate(".array.write.cond", fn);
            auto  block_body  = gen.BlockCreate(".array.write.body", fn);
            auto  block_end   = gen.BlockCreate(".array.write.end",  fn);

            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(counter, right_len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto src    = Get_elem(gen, right_data, elem_size, counter);
                auto dst    = Get_elem(gen, temp_data,  elem_size, counter);
                auto copied = elem_impl->MethodCall(gen, "@copy", {
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

            auto block_rep_cond = gen.BlockCreate(".array.write.rep.cond", fn);
            auto block_rep_body = gen.BlockCreate(".array.write.rep.body", fn);
            auto block_rep_end  = gen.BlockCreate(".array.write.rep.end",  fn);

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
                auto dst = Get_elem(gen, arr_data, elem_size, idx);
                elem_impl->MethodCall(gen, "@release", {
                    Arg(dst, sema::TypeTable::ReferenceTypeGet(elem_type))
                });
                auto copied = elem_impl->MethodCall(gen, "@copy", {
                    Arg(Get_elem(gen, temp_data, elem_size, rep_counter), sema::TypeTable::ReferenceTypeGet(elem_type))
                });
                builder.CreateStore(copied, dst);

                auto next = builder.CreateAdd(rep_counter, builder.getInt64(1));
                builder.CreateBr(block_rep_cond);
                rep_counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_rep_end);

            auto is_equal  = builder.CreateICmpEQ(right_len, view.len);
            auto is_shrink = builder.CreateICmpSLT(right_len, view.len);

            auto block_diff   = gen.BlockCreate(".array.write.diff",   fn);
            auto block_shrink = gen.BlockCreate(".array.write.shrink", fn);
            auto block_grow   = gen.BlockCreate(".array.write.grow",   fn);
            auto block_adjust = gen.BlockCreate(".array.write.adjust", fn);

            builder.CreateCondBr(is_equal, block_adjust, block_diff);

            builder.SetInsertPoint(block_diff);
            builder.CreateCondBr(is_shrink, block_shrink, block_grow);

            builder.SetInsertPoint(block_shrink);
            {
                auto block_del_cond = gen.BlockCreate(".array.write.del.cond", fn);
                auto block_del_body = gen.BlockCreate(".array.write.del.body", fn);
                auto block_del_end  = gen.BlockCreate(".array.write.del.end",  fn);

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
                    elem_impl->MethodCall(gen, "@release", {
                        Arg(Get_elem(gen, arr_data, elem_size, idx), sema::TypeTable::ReferenceTypeGet(elem_type))
                    });
                    auto next = builder.CreateAdd(del_counter, builder.getInt64(1));
                    builder.CreateBr(block_del_cond);
                    del_counter->addIncoming(next, builder.GetInsertBlock());
                }

                builder.SetInsertPoint(block_del_end);
                {
                    auto move_cnt = builder.CreateSub(builder.CreateSub(arr_len, view.offset), view.len);
                    Move_array(
                        gen,
                        Get_elem(gen, arr_data, elem_size, builder.CreateAdd(view.offset, right_len)),
                        Get_elem(gen, arr_data, elem_size, builder.CreateAdd(view.offset, view.len)),
                        move_cnt, elem_size
                    );

                    auto new_len  = builder.CreateSub(arr_len, builder.CreateSub(view.len, right_len));
                    auto new_data = Realloc_array(gen, arr_data, elem_size, new_len);

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
                auto new_data = Realloc_array(gen, arr_data, elem_size, new_len);

                auto move_cnt = builder.CreateSub(builder.CreateSub(arr_len, view.offset), view.len);
                Move_array(
                    gen,
                    Get_elem(gen, new_data, elem_size, builder.CreateAdd(view.offset, right_len)),
                    Get_elem(gen, new_data, elem_size, builder.CreateAdd(view.offset, view.len)),
                    move_cnt, elem_size
                );

                auto block_cp_cond = gen.BlockCreate(".array.write.cp.cond", fn);
                auto block_cp_body = gen.BlockCreate(".array.write.cp.body", fn);
                auto block_cp_end  = gen.BlockCreate(".array.write.cp.end",  fn);

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
                    auto dst    = Get_elem(gen, new_data, elem_size, idx);
                    auto copied = elem_impl->MethodCall(gen, "@copy", {
                        Arg(Get_elem(gen, temp_data, elem_size, cp_counter), sema::TypeTable::ReferenceTypeGet(elem_type))
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
            auto  arr         = Load_array(gen, args[0]);

            // Result
            auto  result_size = builder.CreateMul(arr.len, builder.getInt64(arr.elem_size));
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
                    builder.CreateICmpSLT(counter, arr.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto elem_src    = Get_elem(gen, arr.data, arr.elem_size, counter);
                auto elem_dst    = Get_elem(gen, result_data, arr.elem_size, counter);
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
        impl->MethodAdd("@release",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder     = gen.llvm_builder();
            auto  arr         = Load_array(gen, args[0]);

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
                    builder.CreateICmpSLT(counter, arr.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto elem = Get_elem(gen, arr.data, arr.elem_size, counter);

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

        impl->MethodAdd("@print",       [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Print_array(gen, Load_array(gen, args[0]));
        }, sema::FnSign(none_));
        impl->MethodAdd("@pick",        [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Pick_elem(gen, Load_array(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(T), { i64_ }));
        impl->MethodAdd("@pick",        [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Pick_arrayview(gen, Load_array(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(arrayview_, { T }), { range_ }));

        impl->MethodAdd("@neg",         [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Reverse_array(gen, Load_array(gen, args[0]));
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T })));
        impl->MethodAdd("@plus",        [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder      = gen.llvm_builder();
            auto  larr         = Load_array(gen, args[0]);
            auto  rarr         = Load_array(gen, args[1]);

            auto  result_len   = builder.CreateAdd(larr.len, rarr.len);
            auto  result_size  = builder.CreateMul(result_len, builder.getInt64(larr.elem_size));
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
                builder.CreateCondBr(builder.CreateICmpSLT(l_counter, larr.len), block_l_body, block_l_end);
            }

            builder.SetInsertPoint(block_l_body);
            {
                auto elem_src = Get_elem(gen, larr.data, larr.elem_size, l_counter);
                auto elem_dst = Get_elem(gen, result_data, larr.elem_size, l_counter);
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
                auto elem_src = Get_elem(gen, rarr.data, rarr.elem_size, r_counter);
                auto elem_dst = Get_elem(gen, result_data, rarr.elem_size, dst_idx);
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

        impl->MethodAdd("len",          [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Load_array(gen, args[0]).len;
        }, sema::FnSign(i64_));
        impl->MethodAdd("clear",        [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = Load_array(gen, args[0]);

            TypeImplTable::Lookup(args[0].ReferenceUnwrap())->MethodCall(
                gen, "@release", {
                    Arg(arr.addr, sema::TypeTable::ReferenceTypeGet(args[0].ReferenceUnwrap()))
                }
            );

            auto null_data = llvm::ConstantPointerNull::get(
                llvm::PointerType::get(gen.llvm_context(), 0)
            );
            Store_array(gen, arr.addr, null_data, arr.llvm_type, builder.getInt64(0));
            return nullptr;
        }, sema::FnSign(none_));
        impl->MethodAdd("insert",       [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder     = gen.llvm_builder();
            auto  arr         = Load_array(gen, args[0]);
            auto  idx         = args[1].val();
            auto  one         = builder.getInt64(1);

            auto  result_len  = builder.CreateAdd(arr.len, one);
            auto  result_data = Realloc_array(gen, arr.data, arr.elem_size, result_len);

            Move_array(
                gen,
                Get_elem(gen, result_data, arr.elem_size, builder.CreateAdd(idx, one)),
                Get_elem(gen, result_data, arr.elem_size, idx),
                builder.CreateSub(arr.len, idx),
                arr.elem_size
            );
            auto value_ref = gen.ArgRefMake(args[2].val(), arr.elem_type);
            auto copied    = arr.elem_type_impl->MethodCall(gen, "@copy", { value_ref });
            builder.CreateStore(copied, Get_elem(gen, result_data, arr.elem_size, idx));
            Store_array(gen, arr.addr, result_data, arr.llvm_type, result_len);
            return nullptr;
        }, sema::FnSign(none_, { i64_, nullptr }));
        impl->MethodAdd("remove",       [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = Load_array(gen, args[0]);
            auto  idx     = args[1].val();
            auto  one     = builder.getInt64(1);

            Move_array(
                gen,
                Get_elem(gen, arr.data, arr.elem_size, idx),
                Get_elem(gen, arr.data, arr.elem_size, builder.CreateAdd(idx, one)),
                builder.CreateSub(builder.CreateSub(arr.len, one), idx),
                arr.elem_size
            );
            Store_array(gen, arr.addr, arr.data, arr.llvm_type, builder.CreateSub(arr.len, one));
            return nullptr;
        }, sema::FnSign(none_, { i64_ }));
        impl->MethodAdd("push_back",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder     = gen.llvm_builder();
            auto  arr         = Load_array(gen, args[0]);

            auto  result_len  = builder.CreateAdd(arr.len, builder.getInt64(1));
            auto  result_data = Realloc_array(gen, arr.data, arr.elem_size, result_len);

            auto value_ref    = gen.ArgRefMake(args[1].val(), arr.elem_type);
            auto copied       = arr.elem_type_impl->MethodCall(gen, "@copy", { value_ref });
            builder.CreateStore(copied, Get_elem(
                gen, result_data, arr.elem_size, arr.len
            ));
            Store_array(gen, arr.addr, result_data, arr.llvm_type, result_len);
            return nullptr;
        }, sema::FnSign(none_, { nullptr }));
        impl->MethodAdd("push_front",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder     = gen.llvm_builder();
            auto  arr         = Load_array(gen, args[0]);
            auto  one         = builder.getInt64(1);

            auto  result_len  = builder.CreateAdd(arr.len, one);
            auto  result_data = Realloc_array(gen, arr.data, arr.elem_size, result_len);

            Move_array(
                gen,
                Get_elem(gen, result_data, arr.elem_size, one),
                result_data, arr.len, arr.elem_size
            );
            auto value_ref    = gen.ArgRefMake(args[1].val(), arr.elem_type);
            auto copied       = arr.elem_type_impl->MethodCall(gen, "@copy", { value_ref });
            builder.CreateStore(copied, Get_elem(gen, result_data, arr.elem_size, builder.getInt64(0)));
            Store_array(gen, arr.addr, result_data, arr.llvm_type, result_len);
            return nullptr;
        }, sema::FnSign(none_, { nullptr }));
        impl->MethodAdd("pop_back",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = Load_array(gen, args[0]);

            Store_array(
                gen, arr.addr, arr.data, arr.llvm_type,
                builder.CreateSub(arr.len, builder.getInt64(1))
            );
            return nullptr;
        }, sema::FnSign(none_));
        impl->MethodAdd("pop_front",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  arr     = Load_array(gen, args[0]);
            auto  one     = builder.getInt64(1);

            Move_array(
                gen, arr.data,
                Get_elem(gen, arr.data, arr.elem_size, one),
                builder.CreateSub(arr.len, one), arr.elem_size
            );
            Store_array(gen, arr.addr, arr.data, arr.llvm_type, builder.CreateSub(arr.len, one));
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
            auto  view        = Load_arrayview(gen, args[0]);

            auto  new_size    = builder.CreateMul(view.len, builder.getInt64(view.elem_size));
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
                builder.CreateCondBr(builder.CreateICmpSLT(counter, view.len), block_body, block_end);
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto src_idx  = builder.CreateAdd(view.offset, counter);
                auto elem_src = Get_elem(gen, view.data, view.elem_size, src_idx);
                auto elem_dst = Get_elem(gen, new_data, view.elem_size, counter);
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

        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder   = gen.llvm_builder();
            auto  lval      = Load_arrayview(gen, args[0]);

            auto  rval      = gen.ArgLoad(args[1]);
            auto  rval_data = builder.CreateExtractValue(rval, 0);
            auto  rval_len  = builder.CreateExtractValue(rval, 1);

            Write_array(gen, lval, rval_data, rval_len);
            return nullptr;
        }, sema::FnSign(none_, { sema::TypeTable::ParametricTypeGet(array_, { T }) }));
        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = Load_arrayview(gen, args[0]);
            auto rval = Load_arrayview(gen, args[1]);

            Write_array(gen, lval, rval.data, rval.len);
            return nullptr;
        }, sema::FnSign(none_, { sema::TypeTable::ParametricTypeGet(arrayview_, { T }) }));
        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder  = gen.llvm_builder();
            auto  lval     = Load_arrayview(gen, args[0]);
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
                builder.CreateCondBr(builder.CreateICmpSLT(counter, lval.len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto idx = builder.CreateAdd(lval.offset, counter);
                auto dst = Get_elem(gen, lval.data, lval.elem_size, idx);
                lval.elem_type_impl->MethodCall(gen, "@release", {
                    Arg(dst, sema::TypeTable::ReferenceTypeGet(lval.elem_type))
                });
                auto copied = lval.elem_type_impl->MethodCall(gen, "@copy", { rval_ref });
                builder.CreateStore(copied, dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);
            return nullptr;
        }, sema::FnSign(none_, { T }));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Print_array(gen, Load_arrayview(gen, args[0]));
        }, sema::FnSign(none_));
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Pick_elem(gen, Load_arrayview(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(T), { i64_ }));
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Pick_arrayview(gen, Load_arrayview(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(arrayview_, { T }), { range_ }));

        impl->MethodAdd("@neg",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Reverse_array(gen, Load_arrayview(gen, args[0]));
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(array_, { T })));
        impl->MethodAdd("len",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Load_arrayview(gen, args[0]).len;
        }, sema::FnSign(i64_));
    }
}

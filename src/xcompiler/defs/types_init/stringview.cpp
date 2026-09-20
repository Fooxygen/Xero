
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    void TypeImplTable::Init_stringview() {
        using ARGS        = const std::vector<Arg>&;

        auto  none_       = sema::TypeTable::Lookup("none");
        auto  char_       = sema::TypeTable::Lookup("char");
        auto  i64_        = sema::TypeTable::Lookup("i64");
        auto  string_     = sema::TypeTable::Lookup("string");
        auto  stringview_ = sema::TypeTable::Lookup("stringview");
        auto  range_      = sema::TypeTable::Lookup("range");
        auto  bool_       = sema::TypeTable::Lookup("bool");

        auto  impl        = TypeImplTable::Set(TypeImpl(stringview_));

        struct ViewInfo {
            llvm::Value* addr      = nullptr;
            llvm::Value* org       = nullptr;
            llvm::Value* offset    = nullptr;
            llvm::Value* len       = nullptr;
            llvm::Type*  llvm_type = nullptr;
        };

        auto view_load = [](IRGen& gen, const Arg& arg) -> ViewInfo {
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
        auto stringview_equal   = [view_load, string_](
            IRGen& gen, const Arg& arg_lview, const Arg& arg_rview) -> llvm::Value*
        {
            auto& builder = gen.llvm_builder();
            auto  lview   = view_load(gen, arg_lview);
            auto  rview   = view_load(gen, arg_rview);

            auto lstr  = builder.CreateLoad(gen.LLVMType(string_), lview.org);
            auto ldata = builder.CreateExtractValue(lstr, 0);
            auto rstr  = builder.CreateLoad(gen.LLVMType(string_), rview.org);
            auto rdata = builder.CreateExtractValue(rstr, 0);

            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".stringview.equal.cond", fn);
            auto block_body  = gen.BlockCreate(".stringview.equal.body", fn);
            auto block_end   = gen.BlockCreate(".stringview.equal.end",  fn);

            auto len_eq = builder.CreateICmpEQ(lview.len, rview.len);

            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            auto result  = builder.CreatePHI(builder.getInt1Ty(),  2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                result->addIncoming(len_eq, block_entry);

                auto l_in = builder.CreateICmpSLT(counter, lview.len);
                auto r_in = builder.CreateICmpSLT(counter, rview.len);
                auto cont = builder.CreateAnd(builder.CreateAnd(l_in, r_in), result);
                builder.CreateCondBr(cont, block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto lidx = builder.CreateAdd(lview.offset, counter);
                auto ridx = builder.CreateAdd(rview.offset, counter);
                auto lval = builder.CreateLoad(builder.getInt32Ty(),
                    builder.CreateInBoundsGEP(builder.getInt32Ty(), ldata, { lidx }));
                auto rval = builder.CreateLoad(builder.getInt32Ty(),
                    builder.CreateInBoundsGEP(builder.getInt32Ty(), rdata, { ridx }));
                auto eq   = builder.CreateICmpEQ(lval, rval);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
                result->addIncoming(eq,   builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);
            return result;
        };
        auto string_realloc     = [](IRGen& gen, llvm::Value* data, size_t elem_size, llvm::Value* new_len) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  size    = builder.CreateMul(new_len, builder.getInt64(elem_size));
            return builder.CreateCall(LibC_realloc(gen), { data, size });
        };
        auto string_elem_get    = [](IRGen& gen, llvm::Value* data, size_t elem_size, llvm::Value* idx) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  offset  = builder.CreateMul(idx, builder.getInt64(elem_size));
            return builder.CreateInBoundsGEP(builder.getInt8Ty(), data, { offset });
        };
        auto string_elem_move   = [](IRGen& gen, llvm::Value* dst, llvm::Value* src, llvm::Value* cnt, size_t elem_size) {
            auto& builder = gen.llvm_builder();
            auto  bytes   = builder.CreateMul(cnt, builder.getInt64(elem_size));
            builder.CreateCall(LibC_memmove(gen), { dst, src, bytes });
        };
        auto string_assign_core = [string_realloc, string_elem_get, string_elem_move, string_](
            IRGen& gen, const ViewInfo& view,
            llvm::Value* right_data, llvm::Value* right_len, size_t elem_size) -> void
        {
            auto& builder = gen.llvm_builder();

            auto arr_val  = builder.CreateLoad(gen.LLVMType(string_), view.org);
            auto arr_data = builder.CreateExtractValue(arr_val, 0);
            auto arr_len  = builder.CreateExtractValue(arr_val, 1);

            auto temp_size = builder.CreateMul(right_len, builder.getInt64(elem_size));
            auto temp_data = builder.CreateCall(LibC_malloc(gen), { temp_size });
            builder.CreateCall(LibC_memmove(gen), { temp_data, right_data, temp_size });

            auto common = builder.CreateSelect(
                builder.CreateICmpSLT(view.len, right_len), view.len, right_len
            );

            string_elem_move(
                gen,
                string_elem_get(gen, arr_data, elem_size, view.offset),
                temp_data, common, elem_size
            );

            auto fn           = builder.GetInsertBlock()->getParent();
            auto is_equal     = builder.CreateICmpEQ(right_len, view.len);
            auto is_shrink    = builder.CreateICmpSLT(right_len, view.len);

            auto block_diff   = gen.BlockCreate(".stringview.assign.diff",   fn);
            auto block_shrink = gen.BlockCreate(".stringview.assign.shrink", fn);
            auto block_grow   = gen.BlockCreate(".stringview.assign.grow",   fn);
            auto block_adjust = gen.BlockCreate(".stringview.assign.adjust", fn);

            builder.CreateCondBr(is_equal, block_adjust, block_diff);

            builder.SetInsertPoint(block_diff);
            builder.CreateCondBr(is_shrink, block_shrink, block_grow);

            builder.SetInsertPoint(block_shrink);
            {
                string_elem_move(
                    gen,
                    string_elem_get(gen, arr_data, elem_size, builder.CreateAdd(view.offset, right_len)),
                    string_elem_get(gen, arr_data, elem_size, builder.CreateAdd(view.offset, view.len)),
                    builder.CreateSub(builder.CreateSub(arr_len, view.offset), view.len),
                    elem_size
                );

                auto new_len  = builder.CreateSub(arr_len, builder.CreateSub(view.len, right_len));
                auto new_data = string_realloc(gen, arr_data, elem_size, new_len);

                auto gen_val = (llvm::Value*)llvm::UndefValue::get(gen.LLVMType(string_));
                gen_val = builder.CreateInsertValue(gen_val, new_data, 0);
                gen_val = builder.CreateInsertValue(gen_val, new_len,  1);
                builder.CreateStore(gen_val, view.org);
                builder.CreateBr(block_adjust);
            }

            builder.SetInsertPoint(block_grow);
            {
                auto new_len  = builder.CreateAdd(arr_len, builder.CreateSub(right_len, view.len));
                auto new_data = string_realloc(gen, arr_data, elem_size, new_len);

                string_elem_move(
                    gen,
                    string_elem_get(gen, new_data, elem_size, builder.CreateAdd(view.offset, right_len)),
                    string_elem_get(gen, new_data, elem_size, builder.CreateAdd(view.offset, view.len)),
                    builder.CreateSub(builder.CreateSub(arr_len, view.offset), view.len),
                    elem_size
                );
                string_elem_move(
                    gen,
                    string_elem_get(gen, new_data, elem_size, builder.CreateAdd(view.offset, common)),
                    string_elem_get(gen, temp_data, elem_size, common),
                    builder.CreateSub(right_len, common),
                    elem_size
                );

                auto gen_val = (llvm::Value*)llvm::UndefValue::get(gen.LLVMType(string_));
                gen_val = builder.CreateInsertValue(gen_val, new_data, 0);
                gen_val = builder.CreateInsertValue(gen_val, new_len,  1);
                builder.CreateStore(gen_val, view.org);
                builder.CreateBr(block_adjust);
            }

            builder.SetInsertPoint(block_adjust);
            builder.CreateCall(LibC_free(gen), { temp_data });
        };

        impl->MethodAdd("@print",   [view_load, string_, char_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);

            auto str_val = builder.CreateLoad(gen.LLVMType(string_), view.org);
            auto data    = builder.CreateExtractValue(str_val, 0);

            // Blocks
            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".stringview.print.cond", fn);
            auto block_body  = gen.BlockCreate(".stringview.print.body", fn);
            auto block_end   = gen.BlockCreate(".stringview.print.end",  fn);

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, view.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto idx  = builder.CreateAdd(view.offset, counter);
                auto elem = builder.CreateInBoundsGEP(builder.getInt32Ty(), data, { idx });
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

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(stringview_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        impl->MethodAdd("len",      [view_load](IRGen& gen, ARGS& args) -> llvm::Value* {
            return view_load(gen, args[0]).len;
        }, sema::FnSign(i64_));

        impl->MethodAdd("@pick",    [view_load, string_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);
            auto  idx     = gen.ArgLoad(args[1]);

            auto str_val = builder.CreateLoad(gen.LLVMType(string_), view.org);
            auto data    = builder.CreateExtractValue(str_val, 0);

            auto abs_idx = builder.CreateAdd(view.offset, idx);
            return builder.CreateInBoundsGEP(builder.getInt32Ty(), data, { abs_idx });
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(char_), { i64_ }));

        impl->MethodAdd("@pick",    [view_load, stringview_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);
            auto  range   = gen.ArgLoad(args[1]);

            auto left     = builder.CreateIntCast(builder.CreateExtractValue(range, 0), builder.getInt64Ty(), true);
            auto right    = builder.CreateIntCast(builder.CreateExtractValue(range, 1), builder.getInt64Ty(), true);
            auto isClosed = builder.CreateExtractValue(range, 3);

            auto offset = builder.CreateAdd(view.offset, left);
            auto diff   = builder.CreateSub(right, left);
            auto len    = builder.CreateSelect(isClosed, builder.CreateAdd(diff, builder.getInt64(1)), diff);

            auto view_type = gen.LLVMType(stringview_);
            auto gen_val   = (llvm::Value*)llvm::UndefValue::get(view_type);
            gen_val = builder.CreateInsertValue(gen_val, view.org, 0);
            gen_val = builder.CreateInsertValue(gen_val, offset,   1);
            gen_val = builder.CreateInsertValue(gen_val, len,      2);
            return gen_val;
        }, sema::FnSign(stringview_, { range_ }));

        impl->MethodAdd("@cast",    [view_load, string_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);

            auto str_val = builder.CreateLoad(gen.LLVMType(string_), view.org);
            auto data    = builder.CreateExtractValue(str_val, 0);

            auto new_size = builder.CreateMul(view.len, builder.getInt64(4));
            auto new_data = builder.CreateCall(LibC_malloc(gen), { new_size });

            auto src = builder.CreateInBoundsGEP(builder.getInt32Ty(), data, { view.offset });
            builder.CreateCall(LibC_memmove(gen), { new_data, src, new_size });

            auto gen_type = gen.LLVMType(string_);
            auto gen_val  = (llvm::Value*)llvm::UndefValue::get(gen_type);
            gen_val = builder.CreateInsertValue(gen_val, new_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, view.len,  1);
            return gen_val;
        }, sema::FnSign(string_, {}, std::nullopt, sema::FnModifier::Cast));

        impl->MethodAdd("@neg",     [view_load, string_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);

            auto str_val = builder.CreateLoad(gen.LLVMType(string_), view.org);
            auto data    = builder.CreateExtractValue(str_val, 0);

            auto result_size = builder.CreateMul(view.len, builder.getInt64(4));
            auto result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".stringview.neg.cond", fn);
            auto block_body  = gen.BlockCreate(".stringview.neg.body", fn);
            auto block_end   = gen.BlockCreate(".stringview.neg.end",  fn);

            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(counter, view.len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto src_idx = builder.CreateAdd(view.offset, builder.CreateSub(builder.CreateSub(view.len, builder.getInt64(1)), counter));
                auto src     = builder.CreateInBoundsGEP(builder.getInt32Ty(), data, { src_idx });
                auto dst     = builder.CreateInBoundsGEP(builder.getInt32Ty(), result_data, { counter });
                builder.CreateStore(builder.CreateLoad(builder.getInt32Ty(), src), dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);

            auto gen_type = gen.LLVMType(string_);
            auto gen_val  = (llvm::Value*)llvm::UndefValue::get(gen_type);
            gen_val = builder.CreateInsertValue(gen_val, result_data, 0);
            gen_val = builder.CreateInsertValue(gen_val, view.len,    1);
            return gen_val;
        }, sema::FnSign(string_));

        impl->MethodAdd("@eq",      [stringview_equal](IRGen& gen, ARGS& args) -> llvm::Value* {
            return stringview_equal(gen, args[0], args[1]);
        }, sema::FnSign(bool_, { stringview_ }));
        impl->MethodAdd("@neq",     [stringview_equal](IRGen& gen, ARGS& args) -> llvm::Value* {
            return gen.llvm_builder().CreateNot(stringview_equal(gen, args[0], args[1]));
        }, sema::FnSign(bool_, { stringview_ }));

        impl->MethodAdd("@assign",  [view_load, string_assign_core, char_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);

            auto  value      = gen.ArgLoad(args[1]);
            auto  value_data = builder.CreateExtractValue(value, 0);
            auto  value_len  = builder.CreateExtractValue(value, 1);

            auto  elem_size = gen.llvm_module()->getDataLayout().getTypeAllocSize(gen.LLVMType(char_));
            string_assign_core(gen, view, value_data, value_len, elem_size);
            return nullptr;
        }, sema::FnSign(none_, { string_ }));

        impl->MethodAdd("@assign",  [view_load, string_assign_core, char_, string_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);
            auto  right   = view_load(gen, args[1]);

            auto  str_val    = builder.CreateLoad(gen.LLVMType(string_), right.org);
            auto  right_data = builder.CreateExtractValue(str_val, 0);
            auto  right_len  = right.len;

            auto  elem_size = gen.llvm_module()->getDataLayout().getTypeAllocSize(gen.LLVMType(char_));
            string_assign_core(gen, view, right_data, right_len, elem_size);
            return nullptr;
        }, sema::FnSign(none_, { stringview_ }));

        impl->MethodAdd("@assign",  [view_load, string_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);
            auto  value   = gen.ArgLoad(args[1]);

            auto str_val = builder.CreateLoad(gen.LLVMType(string_), view.org);
            auto data    = builder.CreateExtractValue(str_val, 0);

            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".stringview.fill.cond", fn);
            auto block_body  = gen.BlockCreate(".stringview.fill.body", fn);
            auto block_end   = gen.BlockCreate(".stringview.fill.end",  fn);

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
                auto dst = builder.CreateInBoundsGEP(builder.getInt32Ty(), data, { idx });
                builder.CreateStore(value, dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);
            return nullptr;
        }, sema::FnSign(none_, { char_ }));
    }
}

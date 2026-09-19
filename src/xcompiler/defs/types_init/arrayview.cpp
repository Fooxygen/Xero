
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
        auto  array_     = sema::TypeTable::Lookup("array");
        auto  arrayview_ = sema::TypeTable::Lookup("arrayview");
        auto  range_     = sema::TypeTable::Lookup("range");

        auto  impl       = TypeImplTable::Set(TypeImpl(arrayview_));

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

        impl->MethodAdd("@print",   [view_load, array_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);

            auto  view_type = (sema::ParametricType*)args[0].ReferenceUnwrap();
            auto  elem_type = view_type->params_type()[0];
            auto  elem_impl = TypeImplTable::Lookup(elem_type);
            auto  elem_size = gen.llvm_module()->getDataLayout().getTypeAllocSize(gen.LLVMType(elem_type));

            auto arr_val = builder.CreateLoad(gen.LLVMType(array_), view.org);
            auto data    = builder.CreateExtractValue(arr_val, 0);

            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".arrayview.print.cond",  fn);
            auto block_cont  = gen.BlockCreate(".arrayview.print.cont",  fn);
            auto block_sep   = gen.BlockCreate(".arrayview.print.sep",   fn);
            auto block_nosep = gen.BlockCreate(".arrayview.print.nosep", fn);
            auto block_body  = gen.BlockCreate(".arrayview.print.body",  fn);
            auto block_end   = gen.BlockCreate(".arrayview.print.end",   fn);

            builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString("[", ".arrayview.lb") });

            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, view.len), block_cont, block_end
                );
            }

            builder.SetInsertPoint(block_cont);
            {
                builder.CreateCondBr(
                    builder.CreateICmpEQ(counter, builder.getInt64(0)), block_nosep, block_sep
                );
            }

            builder.SetInsertPoint(block_nosep);
            {
                builder.CreateBr(block_body);
            }

            builder.SetInsertPoint(block_sep);
            {
                builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString(", ", ".arrayview.sep") });
                builder.CreateBr(block_body);
            }

            builder.SetInsertPoint(block_body);
            {
                auto idx    = builder.CreateAdd(view.offset, counter);
                auto offset = builder.CreateMul(idx, builder.getInt64(elem_size));
                auto elem   = builder.CreateInBoundsGEP(builder.getInt8Ty(), data, { offset });
                elem_impl->MethodCall(gen, "@print", {
                    Arg(elem, sema::TypeTable::ReferenceTypeGet(elem_type))
                });

                auto body_end = builder.GetInsertBlock();
                auto next     = builder.CreateAdd(counter, builder.getInt64(1));
                counter->addIncoming(next, body_end);

                builder.CreateBr(block_cond);
            }

            builder.SetInsertPoint(block_end);
            {
                builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString("]", ".arrayview.rb") });
            }

            return nullptr;
        }, sema::FnSign(none_));

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(arrayview_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        impl->MethodAdd("@pick",    [view_load, arrayview_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = view_load(gen, args[0]);
            auto  range   = gen.ArgLoad(args[1]);

            auto left     = builder.CreateIntCast(builder.CreateExtractValue(range, 0), builder.getInt64Ty(), true);
            auto right    = builder.CreateIntCast(builder.CreateExtractValue(range, 1), builder.getInt64Ty(), true);
            auto isClosed = builder.CreateExtractValue(range, 3);

            auto offset = builder.CreateAdd(view.offset, left);
            auto diff   = builder.CreateSub(right, left);
            auto len    = builder.CreateSelect(isClosed, builder.CreateAdd(diff, builder.getInt64(1)), diff);

            auto view_type = gen.LLVMType(arrayview_);
            auto gen_val   = (llvm::Value*)llvm::UndefValue::get(view_type);
            gen_val = builder.CreateInsertValue(gen_val, view.org, 0);
            gen_val = builder.CreateInsertValue(gen_val, offset,   1);
            gen_val = builder.CreateInsertValue(gen_val, len,      2);
            return gen_val;
        }, sema::FnSign(sema::TypeTable::ParametricTypeGet(arrayview_, { ((sema::BasicType*)arrayview_)->params_binding()[0] }), { range_ }));
    }
}

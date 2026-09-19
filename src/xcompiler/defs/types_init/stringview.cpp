
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
    }
}


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

        struct StringInfo {
            llvm::Value* addr      = nullptr;
            llvm::Value* org       = nullptr;
            llvm::Value* offset    = nullptr;
            llvm::Type*  llvm_type = nullptr;
            MemoryInfo   memory    = {};
        };

        StringInfo   StringLoad(IRGen& gen, const Arg& arg) {
            auto& builder   = gen.llvm_builder();
            auto  addr      = gen.ArgAddr(arg);
            auto  llvm_type = gen.LLVMType(arg.ReferenceUnwrap());
            auto  val       = builder.CreateLoad(llvm_type, addr);

            return StringInfo{
                .addr      = addr,
                .org       = addr,
                .offset    = builder.getInt64(0),
                .llvm_type = llvm_type,
                .memory    = MemoryInfo{
                    .data = builder.CreateExtractValue(val, 0),
                    .len  = builder.CreateExtractValue(val, 1)
                }
            };
        }

        StringInfo   StringViewLoad(IRGen& gen, const Arg& arg) {
            auto& builder   = gen.llvm_builder();
            auto  addr      = gen.ArgAddr(arg);
            auto  llvm_type = gen.LLVMType(arg.ReferenceUnwrap());
            auto  val       = builder.CreateLoad(llvm_type, addr);

            auto  org       = builder.CreateExtractValue(val, 0);
            auto  offset    = builder.CreateExtractValue(val, 1);
            auto  len       = builder.CreateExtractValue(val, 2);

            auto  org_val   = builder.CreateLoad(gen.LLVMType(sema::TypeTable::Lookup("string")), org);
            auto  data      = builder.CreateExtractValue(org_val, 0);

            return StringInfo{
                .addr      = addr,
                .org       = org,
                .offset    = offset,
                .llvm_type = llvm_type,
                .memory    = MemoryInfo{ data, len }
            };
        }

        void         Store(
            IRGen& gen, llvm::Value* addr, const MemoryInfo& memory,
            llvm::Type* llvm_type
        ) {
            gen.llvm_builder().CreateStore(
                gen.StructTypeValCreate(llvm_type, { memory.data, memory.len }),
                addr
            );
        }

        llvm::Value* CharGet(IRGen& gen, llvm::Value* data, llvm::Value* idx) {
            return gen.llvm_builder().CreateInBoundsGEP(
                gen.llvm_builder().getInt32Ty(), data, { idx }
            );
        }

        llvm::Value* Compare(
            IRGen& gen,
            const StringInfo& lval, const StringInfo& rval
        ) {
            auto& builder = gen.llvm_builder();
            auto  fn      = builder.GetInsertBlock()->getParent();

            auto  lval_data = CharGet(gen, lval.memory.data, lval.offset);
            auto  rval_data = CharGet(gen, rval.memory.data, rval.offset);
            auto  llen      = lval.memory.len;
            auto  rlen      = rval.memory.len;

            // Blocks
            auto  block_entry = builder.GetInsertBlock();
            auto  block_cond  = gen.BlockCreate(".string.compare.cond", fn);
            auto  block_body  = gen.BlockCreate(".string.compare.body", fn);
            auto  block_next  = gen.BlockCreate(".string.compare.next", fn);
            auto  block_diff  = gen.BlockCreate(".string.compare.diff", fn);
            auto  block_tail  = gen.BlockCreate(".string.compare.tail", fn);
            auto  block_end   = gen.BlockCreate(".string.compare.end",  fn);

            auto  min_len     = builder.CreateSelect(
                builder.CreateICmpSLT(llen, rlen), llen, rlen
            );

            // Cond Block
            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(
                    builder.CreateICmpSLT(counter, min_len), block_body, block_tail
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            auto lchar = builder.CreateLoad(builder.getInt32Ty(), CharGet(gen, lval_data, counter));
            auto rchar = builder.CreateLoad(builder.getInt32Ty(), CharGet(gen, rval_data, counter));
            builder.CreateCondBr(
                builder.CreateICmpEQ(lchar, rchar), block_next, block_diff
            );

            // Next Block
            builder.SetInsertPoint(block_next);
            {
                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, block_next);
            }

            // Diff Block
            builder.SetInsertPoint(block_diff);
            auto diff = builder.CreateSelect(
                builder.CreateICmpSGT(lchar, rchar),
                builder.getInt32(1), builder.getInt32(-1)
            );
            builder.CreateBr(block_end);

            // Tail Block
            builder.SetInsertPoint(block_tail);
            auto tail = builder.CreateSelect(
                builder.CreateICmpEQ(llen, rlen),
                builder.getInt32(0),
                builder.CreateSelect(
                    builder.CreateICmpSGT(llen, rlen),
                    builder.getInt32(1), builder.getInt32(-1)
                )
            );
            builder.CreateBr(block_end);

            // End Block
            builder.SetInsertPoint(block_end);
            auto result = builder.CreatePHI(builder.getInt32Ty(), 2);
            result->addIncoming(diff, block_diff);
            result->addIncoming(tail, block_tail);
            return result;
        }

        llvm::Value* Print(IRGen& gen, const StringInfo& info) {
            auto& builder = gen.llvm_builder();
            auto  char_   = sema::TypeTable::Lookup("char");

            // Blocks
            auto  fn          = builder.GetInsertBlock()->getParent();
            auto  block_entry = builder.GetInsertBlock();
            auto  block_cond  = gen.BlockCreate(".string.print.cond", fn);
            auto  block_body  = gen.BlockCreate(".string.print.body", fn);
            auto  block_end   = gen.BlockCreate(".string.print.end",  fn);

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
                auto elem = CharGet(gen, info.memory.data, builder.CreateAdd(counter, info.offset));
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
        }

        llvm::Value* PickIndex(IRGen& gen, const StringInfo& info, llvm::Value* idx) {
            auto& builder = gen.llvm_builder();
            return CharGet(gen, info.memory.data, builder.CreateAdd(info.offset, idx));
        }

        llvm::Value* PickRange(IRGen& gen, const StringInfo& info, llvm::Value* range) {
            auto& builder   = gen.llvm_builder();

            auto  left      = builder.CreateIntCast(builder.CreateExtractValue(range, 0), builder.getInt64Ty(), true);
            auto  right     = builder.CreateIntCast(builder.CreateExtractValue(range, 1), builder.getInt64Ty(), true);
            auto  is_closed = builder.CreateExtractValue(range, 3);

            auto  diff      = builder.CreateSub(right, left);
            auto  len       = builder.CreateSelect(is_closed, builder.CreateAdd(diff, builder.getInt64(1)), diff);
            auto  offset    = builder.CreateAdd(info.offset, left);

            return gen.StructTypeValCreate(
                gen.LLVMType(sema::TypeTable::Lookup("stringview")),
                { info.org, offset, len }
            );
        }

        llvm::Value* Reverse(IRGen& gen, const StringInfo& info) {
            auto& builder     = gen.llvm_builder();

            auto  result_size = builder.CreateMul(info.memory.len, builder.getInt64(4));
            auto  result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            auto  fn          = builder.GetInsertBlock()->getParent();
            auto  block_entry = builder.GetInsertBlock();
            auto  block_cond  = gen.BlockCreate(".string.reverse.cond", fn);
            auto  block_body  = gen.BlockCreate(".string.reverse.body", fn);
            auto  block_end   = gen.BlockCreate(".string.reverse.end",  fn);

            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(counter, info.memory.len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto src_idx = builder.CreateSub(builder.CreateSub(info.memory.len, builder.getInt64(1)), counter);
                auto src     = CharGet(gen, info.memory.data, builder.CreateAdd(info.offset, src_idx));
                auto dst     = CharGet(gen, result_data, counter);
                builder.CreateStore(builder.CreateLoad(builder.getInt32Ty(), src), dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);

            return gen.StructTypeValCreate(
                gen.LLVMType(sema::TypeTable::Lookup("string")),
                { result_data, info.memory.len }
            );
        }

        llvm::Value* Realloc(IRGen& gen, llvm::Value* data, llvm::Value* new_len) {
            auto& builder = gen.llvm_builder();
            auto  size    = builder.CreateMul(new_len, builder.getInt64(4));
            return builder.CreateCall(LibC_realloc(gen), { data, size });
        }

        void         Copy(IRGen& gen, llvm::Value* dst, llvm::Value* src, llvm::Value* cnt) {
            auto& builder = gen.llvm_builder();
            auto  bytes   = builder.CreateMul(cnt, builder.getInt64(4));
            builder.CreateCall(LibC_memmove(gen), { dst, src, bytes });
        }

        void         Write(
            IRGen& gen,
            const StringInfo& lval, const MemoryInfo& rval
        ) {
            auto& builder   = gen.llvm_builder();
            auto  string_   = sema::TypeTable::Lookup("string");

            auto  lval_load = builder.CreateLoad(gen.LLVMType(string_), lval.org);
            auto  lval_data = builder.CreateExtractValue(lval_load, 0);
            auto  lval_len  = builder.CreateExtractValue(lval_load, 1);

            auto  temp_size = builder.CreateMul(rval.len, builder.getInt64(4));
            auto  temp_data = builder.CreateCall(LibC_malloc(gen), { temp_size });
            Copy(gen, temp_data, rval.data, rval.len);

            auto len_common = builder.CreateSelect(
                builder.CreateICmpSLT(lval.memory.len, rval.len), lval.memory.len, rval.len
            );

            Copy(
                gen,
                CharGet(gen, lval_data, lval.offset),
                temp_data, len_common
            );

            auto fn           = builder.GetInsertBlock()->getParent();
            auto is_equal     = builder.CreateICmpEQ(rval.len, lval.memory.len);
            auto is_shrink    = builder.CreateICmpSLT(rval.len, lval.memory.len);

            auto block_diff   = gen.BlockCreate(".string.write.diff",   fn);
            auto block_shrink = gen.BlockCreate(".string.write.shrink", fn);
            auto block_grow   = gen.BlockCreate(".string.write.grow",   fn);
            auto block_adjust = gen.BlockCreate(".string.write.adjust", fn);

            builder.CreateCondBr(is_equal, block_adjust, block_diff);

            // Diff Block
            builder.SetInsertPoint(block_diff);
            builder.CreateCondBr(is_shrink, block_shrink, block_grow);

            // Shrink Block
            builder.SetInsertPoint(block_shrink);
            {
                Copy(
                    gen,
                    CharGet(gen, lval_data, builder.CreateAdd(lval.offset, rval.len)),
                    CharGet(gen, lval_data, builder.CreateAdd(lval.offset, lval.memory.len)),
                    builder.CreateSub(builder.CreateSub(lval_len, lval.offset), lval.memory.len)
                );

                auto new_len  = builder.CreateSub(lval_len, builder.CreateSub(lval.memory.len, rval.len));
                auto new_data = Realloc(gen, lval_data, new_len);

                Store(gen, lval.org, { new_data, new_len }, gen.LLVMType(string_));
                builder.CreateBr(block_adjust);
            }

            // Grow Block
            builder.SetInsertPoint(block_grow);
            {
                auto new_len  = builder.CreateAdd(lval_len, builder.CreateSub(rval.len, lval.memory.len));
                auto new_data = Realloc(gen, lval_data, new_len);

                Copy(
                    gen,
                    CharGet(gen, new_data, builder.CreateAdd(lval.offset, rval.len)),
                    CharGet(gen, new_data, builder.CreateAdd(lval.offset, lval.memory.len)),
                    builder.CreateSub(builder.CreateSub(lval_len, lval.offset), lval.memory.len)
                );
                Copy(
                    gen,
                    CharGet(gen, new_data, builder.CreateAdd(lval.offset, len_common)),
                    CharGet(gen, temp_data, len_common),
                    builder.CreateSub(rval.len, len_common)
                );

                Store(gen, lval.org, { new_data, new_len }, gen.LLVMType(string_));
                builder.CreateBr(block_adjust);
            }

            // Adjust Block
            builder.SetInsertPoint(block_adjust);
            builder.CreateCall(LibC_free(gen), { temp_data });
        }
    }

    // string

    void TypeImplTable::Init_string() {
        using ARGS        = const std::vector<Arg>&;

        auto  none_       = sema::TypeTable::Lookup("none");
        auto  bool_       = sema::TypeTable::Lookup("bool");
        auto  char_       = sema::TypeTable::Lookup("char");
        auto  i64_        = sema::TypeTable::Lookup("i64");
        auto  string_     = sema::TypeTable::Lookup("string");
        auto  stringview_ = sema::TypeTable::Lookup("stringview");
        auto  range_      = sema::TypeTable::Lookup("range");

        auto  impl        = TypeImplTable::Set(TypeImpl(string_));

        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str     = StringLoad(gen, args[0]);

            // Result
            auto result_size = builder.CreateMul(str.memory.len, builder.getInt64(4));
            auto result_data = builder.CreateCall(LibC_malloc(gen), { result_size });
            Copy(gen, result_data, str.memory.data, str.memory.len);

            // Package
            return gen.StructTypeValCreate(
                str.llvm_type, { result_data, str.memory.len }
            );
        }, sema::FnSign(string_));
        impl->MethodAdd("@release", [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto str = StringLoad(gen, args[0]);
            gen.llvm_builder().CreateCall(LibC_free(gen), { str.memory.data });
            return nullptr;
        }, sema::FnSign(none_));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Print(gen, StringLoad(gen, args[0]));
        }, sema::FnSign(none_));
        
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return PickIndex(gen, StringLoad(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(char_), { i64_ }));
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return PickRange(gen, StringLoad(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(stringview_, { range_ }));
        impl->MethodAdd("@plus",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  lval    = StringLoad(gen, args[0]);
            auto  rval    = StringLoad(gen, args[1]);

            auto result_len  = builder.CreateAdd(lval.memory.len, rval.memory.len);
            auto result_size = builder.CreateMul(result_len, builder.getInt64(4));
            auto result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            // Copy lval
            Copy(gen, result_data, lval.memory.data, lval.memory.len);

            // Copy rval
            auto rval_dst = CharGet(gen, result_data, lval.memory.len);
            Copy(gen, rval_dst, rval.memory.data, rval.memory.len);

            return gen.StructTypeValCreate(
                lval.llvm_type, { result_data, result_len }
            );
        }, sema::FnSign(string_, { string_ }));
        impl->MethodAdd("@neg",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Reverse(gen, StringLoad(gen, args[0]));
        }, sema::FnSign(string_));
        impl->MethodAdd("@gt",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringLoad(gen, args[0]);
            auto rval = StringLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpSGT(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));
        impl->MethodAdd("@lt",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringLoad(gen, args[0]);
            auto rval = StringLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpSLT(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));
        impl->MethodAdd("@ge",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringLoad(gen, args[0]);
            auto rval = StringLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpSGE(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));
        impl->MethodAdd("@le",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringLoad(gen, args[0]);
            auto rval = StringLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpSLE(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));
        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringLoad(gen, args[0]);
            auto rval = StringLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpEQ(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringLoad(gen, args[0]);
            auto rval = StringLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpNE(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));

        impl->MethodAdd("len",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return StringLoad(gen, args[0]).memory.len;
        }, sema::FnSign(i64_));
        impl->MethodAdd("clear",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str     = StringLoad(gen, args[0]);

            builder.CreateCall(LibC_free(gen), { str.memory.data });

            auto null_data = llvm::ConstantPointerNull::get(
                llvm::PointerType::get(gen.llvm_context(), 0)
            );
            Store(gen, str.addr, { null_data, builder.getInt64(0) }, str.llvm_type);
            return nullptr;
        }, sema::FnSign(none_));
    }

    // stringview

    void TypeImplTable::Init_stringview() {
        using ARGS        = const std::vector<Arg>&;

        auto  none_       = sema::TypeTable::Lookup("none");
        auto  bool_       = sema::TypeTable::Lookup("bool");
        auto  char_       = sema::TypeTable::Lookup("char");
        auto  i64_        = sema::TypeTable::Lookup("i64");
        auto  string_     = sema::TypeTable::Lookup("string");
        auto  stringview_ = sema::TypeTable::Lookup("stringview");
        auto  range_      = sema::TypeTable::Lookup("range");

        auto  impl        = TypeImplTable::Set(TypeImpl(stringview_));

        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(stringview_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // @cast

        impl->MethodAdd("@cast",    [string_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  view    = StringViewLoad(gen, args[0]);

            auto  new_size = builder.CreateMul(view.memory.len, builder.getInt64(4));
            auto  new_data = builder.CreateCall(LibC_malloc(gen), { new_size });

            auto  src = CharGet(gen, view.memory.data, view.offset);
            Copy(gen, new_data, src, view.memory.len);

            return gen.StructTypeValCreate(
                gen.LLVMType(string_), { new_data, view.memory.len }
            );
        }, sema::FnSign(string_, {}, std::nullopt, sema::FnModifier::Cast));

        // @assign

        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder   = gen.llvm_builder();
            auto  lval      = StringViewLoad(gen, args[0]);

            auto  rval      = gen.ArgLoad(args[1]);
            auto  rval_data = builder.CreateExtractValue(rval, 0);
            auto  rval_len  = builder.CreateExtractValue(rval, 1);

            Write(gen, lval, { rval_data, rval_len });
            return nullptr;
        }, sema::FnSign(none_, { string_ }));
        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringViewLoad(gen, args[0]);
            auto rval = StringViewLoad(gen, args[1]);

            Write(gen, lval, rval.memory);
            return nullptr;
        }, sema::FnSign(none_, { stringview_ }));
        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  lval    = StringViewLoad(gen, args[0]);
            auto  rval    = gen.ArgLoad(args[1]);

            // Blocks
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
                builder.CreateCondBr(builder.CreateICmpSLT(counter, lval.memory.len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto idx = builder.CreateAdd(lval.offset, counter);
                auto dst = CharGet(gen, lval.memory.data, idx);
                builder.CreateStore(rval, dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);
            return nullptr;
        }, sema::FnSign(none_, { char_ }));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Print(gen, StringViewLoad(gen, args[0]));
        }, sema::FnSign(none_));
        
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return PickIndex(gen, StringViewLoad(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(char_), { i64_ }));
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return PickRange(gen, StringViewLoad(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(stringview_, { range_ }));
        impl->MethodAdd("@neg",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Reverse(gen, StringViewLoad(gen, args[0]));
        }, sema::FnSign(string_));
        impl->MethodAdd("@gt",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringViewLoad(gen, args[0]);
            auto rval = StringViewLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpSGT(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));
        impl->MethodAdd("@lt",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringViewLoad(gen, args[0]);
            auto rval = StringViewLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpSLT(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));
        impl->MethodAdd("@ge",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringViewLoad(gen, args[0]);
            auto rval = StringViewLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpSGE(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));
        impl->MethodAdd("@le",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringViewLoad(gen, args[0]);
            auto rval = StringViewLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpSLE(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));
        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringViewLoad(gen, args[0]);
            auto rval = StringViewLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpEQ(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = StringViewLoad(gen, args[0]);
            auto rval = StringViewLoad(gen, args[1]);
            return gen.llvm_builder().CreateICmpNE(
                Compare(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));

        impl->MethodAdd("len",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return StringViewLoad(gen, args[0]).memory.len;
        }, sema::FnSign(i64_));
    }
}

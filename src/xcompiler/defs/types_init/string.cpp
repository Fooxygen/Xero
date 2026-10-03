
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

        struct StringInfo {
            llvm::Value* addr      = nullptr;
            llvm::Value* org       = nullptr;
            llvm::Value* data      = nullptr;
            llvm::Value* offset    = nullptr;
            llvm::Value* len       = nullptr;
            llvm::Type*  llvm_type = nullptr;
        };

        StringInfo   Load_string(IRGen& gen, const Arg& arg) {
            auto& builder   = gen.llvm_builder();
            auto  addr      = gen.ArgAddr(arg);
            auto  llvm_type = gen.LLVMType(arg.ReferenceUnwrap());
            auto  val       = builder.CreateLoad(llvm_type, addr);

            return {
                addr, addr,
                builder.CreateExtractValue(val, 0),
                builder.getInt64(0),
                builder.CreateExtractValue(val, 1),
                llvm_type
            };
        }

        StringInfo   Load_stringview(IRGen& gen, const Arg& arg) {
            auto& builder   = gen.llvm_builder();
            auto  addr      = gen.ArgAddr(arg);
            auto  llvm_type = gen.LLVMType(arg.ReferenceUnwrap());
            auto  val       = builder.CreateLoad(llvm_type, addr);

            auto  org       = builder.CreateExtractValue(val, 0);
            auto  offset    = builder.CreateExtractValue(val, 1);
            auto  len       = builder.CreateExtractValue(val, 2);

            auto  org_val   = builder.CreateLoad(gen.LLVMType(sema::TypeTable::Lookup("string")), org);
            auto  data      = builder.CreateExtractValue(org_val, 0);

            return { addr, org, data, offset, len, llvm_type };
        }

        void         Store_string(
            IRGen& gen, llvm::Value* addr, llvm::Value* data,
            llvm::Type* llvm_type, llvm::Value* len
        ) {
            auto& builder = gen.llvm_builder();
            builder.CreateStore(
                gen.ValueStructCreate(llvm_type, { data, len }),
                addr
            );
        }

        llvm::Value* Get_char(IRGen& gen, llvm::Value* data, llvm::Value* idx) {
            return gen.llvm_builder().CreateInBoundsGEP(
                gen.llvm_builder().getInt32Ty(), data, { idx }
            );
        }

        llvm::Value* Compare_string(
            IRGen& gen,
            const StringInfo& lval, const StringInfo& rval
        ) {
            auto& builder = gen.llvm_builder();
            auto  fn      = builder.GetInsertBlock()->getParent();

            auto  ldata   = Get_char(gen, lval.data, lval.offset);
            auto  rdata   = Get_char(gen, rval.data, rval.offset);
            auto  llen    = lval.len;
            auto  rlen    = rval.len;

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
            auto lchar = builder.CreateLoad(builder.getInt32Ty(), Get_char(gen, ldata, counter));
            auto rchar = builder.CreateLoad(builder.getInt32Ty(), Get_char(gen, rdata, counter));
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

        llvm::Value* Print_string(IRGen& gen, const StringInfo& info) {
            auto& builder = gen.llvm_builder();
            auto  char_   = sema::TypeTable::Lookup("char");

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
                    builder.CreateICmpSLT(counter, info.len), block_body, block_end
                );
            }

            // Body Block
            builder.SetInsertPoint(block_body);
            {
                auto elem = Get_char(gen, info.data, builder.CreateAdd(counter, info.offset));
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

        llvm::Value* Pick_char(IRGen& gen, const StringInfo& info, llvm::Value* idx) {
            auto& builder = gen.llvm_builder();
            return Get_char(gen, info.data, builder.CreateAdd(info.offset, idx));
        }

        llvm::Value* Pick_stringview(IRGen& gen, const StringInfo& info, llvm::Value* range) {
            auto& builder  = gen.llvm_builder();

            auto left      = builder.CreateIntCast(builder.CreateExtractValue(range, 0), builder.getInt64Ty(), true);
            auto right     = builder.CreateIntCast(builder.CreateExtractValue(range, 1), builder.getInt64Ty(), true);
            auto is_closed = builder.CreateExtractValue(range, 3);

            auto diff      = builder.CreateSub(right, left);
            auto len       = builder.CreateSelect(is_closed, builder.CreateAdd(diff, builder.getInt64(1)), diff);
            auto offset    = builder.CreateAdd(info.offset, left);

            return gen.ValueStructCreate(
                gen.LLVMType(sema::TypeTable::Lookup("stringview")),
                { info.org, offset, len }
            );
        }

        llvm::Value* Reverse_string(IRGen& gen, const StringInfo& info) {
            auto& builder = gen.llvm_builder();

            auto result_size = builder.CreateMul(info.len, builder.getInt64(4));
            auto result_data = builder.CreateCall(LibC_malloc(gen), { result_size });

            auto fn          = builder.GetInsertBlock()->getParent();
            auto block_entry = builder.GetInsertBlock();
            auto block_cond  = gen.BlockCreate(".string.reverse.cond", fn);
            auto block_body  = gen.BlockCreate(".string.reverse.body", fn);
            auto block_end   = gen.BlockCreate(".string.reverse.end",  fn);

            builder.CreateBr(block_cond);
            builder.SetInsertPoint(block_cond);
            auto counter = builder.CreatePHI(builder.getInt64Ty(), 2);
            {
                counter->addIncoming(builder.getInt64(0), block_entry);
                builder.CreateCondBr(builder.CreateICmpSLT(counter, info.len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto src_idx = builder.CreateSub(builder.CreateSub(info.len, builder.getInt64(1)), counter);
                auto src     = Get_char(gen, info.data, builder.CreateAdd(info.offset, src_idx));
                auto dst     = Get_char(gen, result_data, counter);
                builder.CreateStore(builder.CreateLoad(builder.getInt32Ty(), src), dst);

                auto next = builder.CreateAdd(counter, builder.getInt64(1));
                builder.CreateBr(block_cond);
                counter->addIncoming(next, builder.GetInsertBlock());
            }

            builder.SetInsertPoint(block_end);

            return gen.ValueStructCreate(
                gen.LLVMType(sema::TypeTable::Lookup("string")),
                { result_data, info.len }
            );
        }

        llvm::Value* Realloc_string(IRGen& gen, llvm::Value* data, llvm::Value* new_len) {
            auto& builder = gen.llvm_builder();
            auto  size    = builder.CreateMul(new_len, builder.getInt64(4));
            return builder.CreateCall(LibC_realloc(gen), { data, size });
        }

        void         Move_string(IRGen& gen, llvm::Value* dst, llvm::Value* src, llvm::Value* cnt) {
            auto& builder = gen.llvm_builder();
            auto  bytes   = builder.CreateMul(cnt, builder.getInt64(4));
            builder.CreateCall(LibC_memmove(gen), { dst, src, bytes });
        }

        void         Write_string(
            IRGen& gen,
            const StringInfo& lval, llvm::Value* rval_data, llvm::Value* rval_len
        ) {
            auto& builder   = gen.llvm_builder();
            auto  string_   = sema::TypeTable::Lookup("string");

            auto  arr_val   = builder.CreateLoad(gen.LLVMType(string_), lval.org);
            auto  arr_data  = builder.CreateExtractValue(arr_val, 0);
            auto  arr_len   = builder.CreateExtractValue(arr_val, 1);

            auto  temp_size = builder.CreateMul(rval_len, builder.getInt64(4));
            auto  temp_data = builder.CreateCall(LibC_malloc(gen), { temp_size });
            builder.CreateCall(LibC_memmove(gen), { temp_data, rval_data, temp_size });

            auto common = builder.CreateSelect(
                builder.CreateICmpSLT(lval.len, rval_len), lval.len, rval_len
            );

            Move_string(
                gen,
                Get_char(gen, arr_data, lval.offset),
                temp_data, common
            );

            auto fn           = builder.GetInsertBlock()->getParent();
            auto is_equal     = builder.CreateICmpEQ(rval_len, lval.len);
            auto is_shrink    = builder.CreateICmpSLT(rval_len, lval.len);

            auto block_diff   = gen.BlockCreate(".string.write.diff",   fn);
            auto block_shrink = gen.BlockCreate(".string.write.shrink", fn);
            auto block_grow   = gen.BlockCreate(".string.write.grow",   fn);
            auto block_adjust = gen.BlockCreate(".string.write.adjust", fn);

            builder.CreateCondBr(is_equal, block_adjust, block_diff);

            builder.SetInsertPoint(block_diff);
            builder.CreateCondBr(is_shrink, block_shrink, block_grow);

            builder.SetInsertPoint(block_shrink);
            {
                Move_string(
                    gen,
                    Get_char(gen, arr_data, builder.CreateAdd(lval.offset, rval_len)),
                    Get_char(gen, arr_data, builder.CreateAdd(lval.offset, lval.len)),
                    builder.CreateSub(builder.CreateSub(arr_len, lval.offset), lval.len)
                );

                auto new_len  = builder.CreateSub(arr_len, builder.CreateSub(lval.len, rval_len));
                auto new_data = Realloc_string(gen, arr_data, new_len);

                builder.CreateStore(
                    gen.ValueStructCreate(gen.LLVMType(string_), { new_data, new_len }),
                    lval.org
                );
                builder.CreateBr(block_adjust);
            }

            builder.SetInsertPoint(block_grow);
            {
                auto new_len  = builder.CreateAdd(arr_len, builder.CreateSub(rval_len, lval.len));
                auto new_data = Realloc_string(gen, arr_data, new_len);

                Move_string(
                    gen,
                    Get_char(gen, new_data, builder.CreateAdd(lval.offset, rval_len)),
                    Get_char(gen, new_data, builder.CreateAdd(lval.offset, lval.len)),
                    builder.CreateSub(builder.CreateSub(arr_len, lval.offset), lval.len)
                );
                Move_string(
                    gen,
                    Get_char(gen, new_data, builder.CreateAdd(lval.offset, common)),
                    Get_char(gen, temp_data, common),
                    builder.CreateSub(rval_len, common)
                );

                builder.CreateStore(
                    gen.ValueStructCreate(gen.LLVMType(string_), { new_data, new_len }),
                    lval.org
                );
                builder.CreateBr(block_adjust);
            }

            builder.SetInsertPoint(block_adjust);
            builder.CreateCall(LibC_free(gen), { temp_data });
        }
    }

    // string

    void TypeImplTable::Init_string() {
        using ARGS    = const std::vector<Arg>&;

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
            auto  str     = Load_string(gen, args[0]);

            // Result
            auto result_size = builder.CreateMul(str.len, builder.getInt64(4));
            auto result_data = builder.CreateCall(LibC_malloc(gen), { result_size });
            builder.CreateCall(LibC_memmove(gen), { result_data, str.data, result_size });

            // Generated Value
            return gen.ValueStructCreate(
                str.llvm_type, { result_data, str.len }
            );
        }, sema::FnSign(string_));
        impl->MethodAdd("@release", [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto str = Load_string(gen, args[0]);
            gen.llvm_builder().CreateCall(LibC_free(gen), { str.data });
            return nullptr;
        }, sema::FnSign(none_));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Print_string(gen, Load_string(gen, args[0]));
        }, sema::FnSign(none_));
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Pick_char(gen, Load_string(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(char_), { i64_ }));
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Pick_stringview(gen, Load_string(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(stringview_, { range_ }));
        impl->MethodAdd("@plus",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  lstr    = Load_string(gen, args[0]);
            auto  rstr    = Load_string(gen, args[1]);

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

            return gen.ValueStructCreate(
                lstr.llvm_type, { result_data, result_len }
            );
        }, sema::FnSign(string_, { string_ }));
        impl->MethodAdd("@neg",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Reverse_string(gen, Load_string(gen, args[0]));
        }, sema::FnSign(string_));
        impl->MethodAdd("@gt",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lstr = Load_string(gen, args[0]);
            auto rstr = Load_string(gen, args[1]);
            return gen.llvm_builder().CreateICmpSGT(
                Compare_string(gen, lstr, rstr),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));
        impl->MethodAdd("@lt",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lstr = Load_string(gen, args[0]);
            auto rstr = Load_string(gen, args[1]);
            return gen.llvm_builder().CreateICmpSLT(
                Compare_string(gen, lstr, rstr),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));
        impl->MethodAdd("@ge",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lstr = Load_string(gen, args[0]);
            auto rstr = Load_string(gen, args[1]);
            return gen.llvm_builder().CreateICmpSGE(
                Compare_string(gen, lstr, rstr),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));
        impl->MethodAdd("@le",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lstr = Load_string(gen, args[0]);
            auto rstr = Load_string(gen, args[1]);
            return gen.llvm_builder().CreateICmpSLE(
                Compare_string(gen, lstr, rstr),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));
        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lstr = Load_string(gen, args[0]);
            auto rstr = Load_string(gen, args[1]);
            return gen.llvm_builder().CreateICmpEQ(
                Compare_string(gen, lstr, rstr),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lstr = Load_string(gen, args[0]);
            auto rstr = Load_string(gen, args[1]);
            return gen.llvm_builder().CreateICmpNE(
                Compare_string(gen, lstr, rstr),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { string_ }));

        impl->MethodAdd("len",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Load_string(gen, args[0]).len;
        }, sema::FnSign(i64_));
        impl->MethodAdd("clear",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  str     = Load_string(gen, args[0]);

            builder.CreateCall(LibC_free(gen), { str.data });

            auto null_data = llvm::ConstantPointerNull::get(
                llvm::PointerType::get(gen.llvm_context(), 0)
            );
            Store_string(gen, str.addr, null_data, str.llvm_type, builder.getInt64(0));
            return nullptr;
        }, sema::FnSign(none_));
    }

    // stringview

    void TypeImplTable::Init_stringview() {
        using ARGS    = const std::vector<Arg>&;

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
            auto  view    = Load_stringview(gen, args[0]);

            auto  new_size = builder.CreateMul(view.len, builder.getInt64(4));
            auto  new_data = builder.CreateCall(LibC_malloc(gen), { new_size });

            auto  src = Get_char(gen, view.data, view.offset);
            builder.CreateCall(LibC_memmove(gen), { new_data, src, new_size });

            return gen.ValueStructCreate(
                gen.LLVMType(string_), { new_data, view.len }
            );
        }, sema::FnSign(string_, {}, std::nullopt, sema::FnModifier::Cast));

        // @assign

        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder   = gen.llvm_builder();
            auto  lval      = Load_stringview(gen, args[0]);

            auto  rval      = gen.ArgLoad(args[1]);
            auto  rval_data = builder.CreateExtractValue(rval, 0);
            auto  rval_len  = builder.CreateExtractValue(rval, 1);

            Write_string(gen, lval, rval_data, rval_len);
            return nullptr;
        }, sema::FnSign(none_, { string_ }));
        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = Load_stringview(gen, args[0]);
            auto rval = Load_stringview(gen, args[1]);

            Write_string(gen, lval, rval.data, rval.len);
            return nullptr;
        }, sema::FnSign(none_, { stringview_ }));
        impl->MethodAdd("@assign",  [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  lval    = Load_stringview(gen, args[0]);
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
                builder.CreateCondBr(builder.CreateICmpSLT(counter, lval.len), block_body, block_end);
            }

            builder.SetInsertPoint(block_body);
            {
                auto idx = builder.CreateAdd(lval.offset, counter);
                auto dst = builder.CreateInBoundsGEP(builder.getInt32Ty(), lval.data, { idx });
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
            return Print_string(gen, Load_stringview(gen, args[0]));
        }, sema::FnSign(none_));
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Pick_char(gen, Load_stringview(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(sema::TypeTable::ReferenceTypeGet(char_), { i64_ }));
        impl->MethodAdd("@pick",    [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Pick_stringview(gen, Load_stringview(gen, args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(stringview_, { range_ }));
        impl->MethodAdd("@neg",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Reverse_string(gen, Load_stringview(gen, args[0]));
        }, sema::FnSign(string_));
        impl->MethodAdd("@gt",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = Load_stringview(gen, args[0]);
            auto rval = Load_stringview(gen, args[1]);
            return gen.llvm_builder().CreateICmpSGT(
                Compare_string(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));
        impl->MethodAdd("@lt",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = Load_stringview(gen, args[0]);
            auto rval = Load_stringview(gen, args[1]);
            return gen.llvm_builder().CreateICmpSLT(
                Compare_string(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));
        impl->MethodAdd("@ge",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = Load_stringview(gen, args[0]);
            auto rval = Load_stringview(gen, args[1]);
            return gen.llvm_builder().CreateICmpSGE(
                Compare_string(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));
        impl->MethodAdd("@le",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = Load_stringview(gen, args[0]);
            auto rval = Load_stringview(gen, args[1]);
            return gen.llvm_builder().CreateICmpSLE(
                Compare_string(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));
        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = Load_stringview(gen, args[0]);
            auto rval = Load_stringview(gen, args[1]);
            return gen.llvm_builder().CreateICmpEQ(
                Compare_string(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto lval = Load_stringview(gen, args[0]);
            auto rval = Load_stringview(gen, args[1]);
            return gen.llvm_builder().CreateICmpNE(
                Compare_string(gen, lval, rval),
                gen.llvm_builder().getInt32(0)
            );
        }, sema::FnSign(bool_, { stringview_ }));

        impl->MethodAdd("len",      [](IRGen& gen, ARGS& args) -> llvm::Value* {
            return Load_stringview(gen, args[0]).len;
        }, sema::FnSign(i64_));
    }
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    void TypeImplTable::Init_bool() {
        using ARGS  = const std::vector<Arg>&;

        auto  none_   = sema::TypeTable::Lookup("none");
        auto  bool_   = sema::TypeTable::Lookup("bool");
        auto  string_ = sema::TypeTable::Lookup("string");

        auto  impl    = TypeImplTable::Set(TypeImpl(bool_));
                
        // @copy and @release

        impl->MethodAdd("@copy",        [](IRGen& gen, ARGS& args) {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(bool_));
        impl->MethodAdd("@release",     [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // Other

        impl->MethodAdd("@print",       [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder    = gen.llvm_builder();
            auto  str_fmt    = builder.CreateGlobalString("%s",    ".fmt.str");
            auto  str_true   = builder.CreateGlobalString("true",  ".true");
            auto  str_false  = builder.CreateGlobalString("false", ".false");
            auto  str_output = builder.CreateSelect(gen.ArgLoad(args[0]), str_true, str_false);
            builder.CreateCall(LibC_printf(gen), { str_fmt, str_output });
            return nullptr;
        }, sema::FnSign(none_));
        
        impl->MethodAdd("@eq",          [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpEQ(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { bool_ }));
        impl->MethodAdd("@neq",         [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpNE(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { bool_ }));
        impl->MethodAdd("@and",         [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateAnd(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { bool_ }));
        impl->MethodAdd("@or",          [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateOr(gen.ArgLoad(args[0]), gen.ArgLoad(args[1]));
        }, sema::FnSign(bool_, { bool_ }));
        impl->MethodAdd("@not",         [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateNot(gen.ArgLoad(args[0]));
        }, sema::FnSign(bool_));

        impl->MethodAdd("to_string",    [string_](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();
            auto  cond    = gen.ArgLoad(args[0]);

            auto data  = builder.CreateCall(LibC_malloc(gen), { builder.getInt64(20) });
            auto store = [&](llvm::Value* codepoint, int idx) {
                builder.CreateStore(codepoint, builder.CreateInBoundsGEP(
                    builder.getInt32Ty(), data, { builder.getInt64(idx) }
                ));
            };
            store(builder.CreateSelect(cond, builder.getInt32('t'), builder.getInt32('f')), 0);
            store(builder.CreateSelect(cond, builder.getInt32('r'), builder.getInt32('a')), 1);
            store(builder.CreateSelect(cond, builder.getInt32('u'), builder.getInt32('l')), 2);
            store(builder.CreateSelect(cond, builder.getInt32('e'), builder.getInt32('s')), 3);
            store(builder.CreateSelect(cond, builder.getInt32(0),   builder.getInt32('e')), 4);
            auto len = builder.CreateSelect(cond, builder.getInt64(4), builder.getInt64(5));

            return gen.StructTypeValCreate(
                gen.LLVMType(string_), { data, len }
            );
        }, sema::FnSign(string_));
    }
}

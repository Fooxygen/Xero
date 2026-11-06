
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

        auto  none_ = sema::TypeTable::Lookup("none");
        auto  bool_ = sema::TypeTable::Lookup("bool");

        auto  impl  = TypeImplTable::Set(TypeImpl(bool_));
                
        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) {
            return gen.ArgValGet(args[0]);
        }, sema::FnSign(bool_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder    = gen.llvm_builder();
            auto  str_fmt    = builder.CreateGlobalString("%s",    ".fmt.str");
            auto  str_true   = builder.CreateGlobalString("true",  ".true");
            auto  str_false  = builder.CreateGlobalString("false", ".false");
            auto  str_output = builder.CreateSelect(gen.ArgValGet(args[0]), str_true, str_false);
            builder.CreateCall(LibC_printf(gen), { str_fmt, str_output });
            return nullptr;
        }, sema::FnSign(none_));
        
        impl->MethodAdd("@eq",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpEQ(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { bool_ }));
        impl->MethodAdd("@neq",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateICmpNE(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { bool_ }));
        impl->MethodAdd("@and",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateAnd(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { bool_ }));
        impl->MethodAdd("@or",      [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateOr(gen.ArgValGet(args[0]), gen.ArgValGet(args[1]));
        }, sema::FnSign(bool_, { bool_ }));
        impl->MethodAdd("@not",     [](IRGen& gen, ARGS& args) {
            return gen.llvm_builder().CreateNot(gen.ArgValGet(args[0]));
        }, sema::FnSign(bool_));
    }
}

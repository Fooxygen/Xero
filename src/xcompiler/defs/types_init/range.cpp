
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    void TypeImplTable::Init_range() {
        using ARGS   = const std::vector<Arg>&;

        auto  none_  = sema::TypeTable::Lookup("none");
        auto  range_ = sema::TypeTable::Lookup("range");

        auto  impl   = TypeImplTable::Set(TypeImpl(range_));

        // @copy and @release

        impl->MethodAdd("@copy",    [](IRGen& gen, ARGS& args) {
            return gen.ArgLoad(args[0]);
        }, sema::FnSign(range_));
        impl->MethodAdd("@release", [](IRGen&, ARGS&) -> llvm::Value* {
            return nullptr;
        }, sema::FnSign(none_));

        // Other

        impl->MethodAdd("@print",   [](IRGen& gen, ARGS& args) -> llvm::Value* {
            auto& builder = gen.llvm_builder();

            auto  range_val      = gen.ArgLoad(args[0]);
            auto  range_type     = (sema::ParametricType*)args[0].ReferenceUnwrap();
            auto  iter_type      = range_type->params()[0];
            auto  iter_type_impl = TypeImplTable::Lookup(iter_type);
            auto  left_val       = builder.CreateExtractValue(range_val, 0);
            auto  right_val      = builder.CreateExtractValue(range_val, 1);
            auto  step_val       = builder.CreateExtractValue(range_val, 2);
            auto  is_closed_val  = builder.CreateExtractValue(range_val, 3);

            builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString("[", ".range.lb") });
            iter_type_impl->MethodCall(gen, "@print", { gen.ArgRefMake(left_val, iter_type) });
            builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString(" : ", ".range.sep") });
            iter_type_impl->MethodCall(gen, "@print", { gen.ArgRefMake(step_val, iter_type) });
            builder.CreateCall(LibC_printf(gen), { builder.CreateGlobalString(" : ", ".range.sep") });
            iter_type_impl->MethodCall(gen, "@print", { gen.ArgRefMake(right_val, iter_type) });
            
            // Right Boundary
            auto rb = builder.CreateGlobalString("]", ".rb");   // RBrace
            auto rp = builder.CreateGlobalString(")", ".rp");   // RParen
            builder.CreateCall(LibC_printf(gen), {
                builder.CreateGlobalString("%s", ".fmt.str"),
                builder.CreateSelect(is_closed_val, rb, rp)
            });

            return nullptr;
        }, sema::FnSign(none_));
    }
}

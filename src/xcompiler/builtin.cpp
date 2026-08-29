
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "llvm/IR/Function.h"

#include "sema/defs/fn.hpp"
#include "sema/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/defs//type.hpp"
#include "xcompiler/ir/ir.hpp"

namespace xcompiler {
    
    // LibC
    
    llvm::Function* LibC_printf(IRGen& gen) {
        auto module = gen.module();
        if (auto fn = module->getFunction("printf")) return fn;

        auto context = &module->getContext();
        auto fntype  = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*context),
            { llvm::PointerType::getUnqual(*context) },
            true // variable parameters
        );
        return llvm::Function::Create(
            fntype,
            llvm::Function::ExternalLinkage,
            "printf",
            module
        );
    }

    // Built-in Fn

    void BuiltinFnRegister(sema::FnTable& fn_table) {
        auto none_ = sema::TypeTable::Lookup("none");

        // print and println
        {
            auto impl_print_one = [](IRGen& gen, llvm::Value* val, sema::Type* type) {
                auto& method      = ((sema::BasicType*)type->BasicTypeGet())->method_table().Lookup("print");
                auto  method_sign = method.SignLookup({ type });
                auto  method_impl = TypeImplTable::Lookup(type)->MethodGet(method_sign);
                ((NativeFnImpl*)method_impl)->impl()(gen, { val }, { type });
            };

            auto impl_print = [impl_print_one](
                IRGen& gen,
                const std::vector<llvm::Value*>& vals,
                const std::vector<sema::Type*>&  types) -> llvm::Value*
            {
                auto& builder = gen.builder();
                for (size_t i = 0; i < vals.size(); i++) {
                    if (i != 0) builder.CreateCall(LibC_printf(gen), {
                        builder.CreateGlobalString(" ", ".delim")
                    });
                    impl_print_one(gen, vals[i], types[i]);
                }

                return nullptr;
            };

            auto impl_println = [impl_print_one, impl_print](
                IRGen& gen,
                const std::vector<llvm::Value*>& vals,
                const std::vector<sema::Type*>&  types) -> llvm::Value*
            {
                auto& builder = gen.builder();
                impl_print(gen, vals, types);

                // Line Break
                builder.CreateCall(LibC_printf(gen), {
                    builder.CreateGlobalString("\n", ".nl")
                });

                return nullptr;
            };

            auto sign_print   = fn_table.Lookup("print").SignLookup(sema::FnSign(none_, {}, nullptr, {}, "print"));
            auto sign_println = fn_table.Lookup("println").SignLookup(sema::FnSign(none_, {}, nullptr, {}, "println"));
            FnImplTable::Add("print", sign_print,   std::make_unique<NativeFnImpl>(impl_print));
            FnImplTable::Add("println", sign_println, std::make_unique<NativeFnImpl>(impl_println));
        }
    }
}
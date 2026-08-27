
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "llvm/IR/Function.h"

#include "sema/type.hpp"
#include "xcompiler/builtin/builtin.hpp"
#include "xcompiler/ir/type.hpp"
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
            true                                                // flexible parameters
        );
        return llvm::Function::Create(
            fntype,
            llvm::Function::ExternalLinkage,
            "printf",
            module
        );
    }

    // BuiltinFnTable

    void BuiltinFnTable::Register() {

        // print and println
        {
            static auto impl = [](IRGen& gen, FnCallExpr& node) {
                auto& builder = gen.builder();
                auto& exprs   = node.args_->exprs_;

                for (size_t i = 0; i < exprs.size(); i++) {

                    // Delimiter
                    if (i != 0) {
                        builder.CreateCall(
                            LibC_printf(gen),
                            { builder.CreateGlobalString(" ", ".delim") }
                        );
                    }
                    
                    auto& expr        = exprs[i];
                    auto  val         = gen.Exec(*expr);
                    auto  type        = expr->resolved_type_;
                    auto  type_impl   = TypeImplTable::Lookup(type);
                    auto  type_method = type_impl->MethodGet("print");
                    ((NativeMethodImpl*)(type_method->at(0).get()))->fn_(gen, { val });
                }
            };

            Set("print", [](IRGen& gen, FnCallExpr& node) -> llvm::Value* {
                impl(gen, node);
                return nullptr;
            });
            Set("println", [](IRGen& gen, FnCallExpr& node) -> llvm::Value* {
                impl(gen, node);

                auto& builder = gen.builder();
                builder.CreateCall(
                    LibC_printf(gen),
                    { builder.CreateGlobalString("\n", ".nl") }
                );

                return nullptr;
            });
        }
    }
}
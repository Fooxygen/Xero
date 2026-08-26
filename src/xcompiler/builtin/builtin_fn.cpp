
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "llvm/IR/Function.h"

#include "sema/type.hpp"
#include "xcompiler/builtin/builtin_fn.hpp"
#include "xcompiler/ir/type.hpp"
#include "xcompiler/ir/ir.hpp"

namespace xcompiler {
    
    // LibC
    
    llvm::Function* libc_printf(IRGen& gen) {
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
                auto& exprs  = node.args_->exprs_;
                for (size_t i = 0; i < exprs.size(); i++) {
                    if (i != 0) {
                        builder.CreateCall(
                            libc_printf(gen),
                            { builder.CreateGlobalString(" ", ".sep") }
                        );
                    }
                    
                    auto& expr     = exprs[i];
                    auto  arg_type = expr->resolved_type_;
                    auto  arg      = gen.Exec(*expr);
                    arg_type->gen()->print_(gen, arg);
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
                    libc_printf(gen),
                    { builder.CreateGlobalString("\n", ".nl") }
                );

                return nullptr;
            });
        }
    }
}
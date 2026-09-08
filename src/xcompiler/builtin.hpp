
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "llvm/IR/Function.h"

#include "sema/defs/fn.hpp"

namespace xcompiler {
    class IRGen;

    // LibC

    llvm::Function* LibCCreate(IRGen& gen, const std::string& name, llvm::FunctionType* fntype);
    llvm::Function* LibC_printf(IRGen& gen);
    llvm::Function* LibC_malloc(IRGen& gen);
    llvm::Function* LibC_realloc(IRGen& gen);
    llvm::Function* LibC_free(IRGen& gen);
    llvm::Function* LibC_memmove(IRGen& gen);

    // Built-in Fn

    void BuiltinFnRegister(sema::FnTable& fn_table);
}

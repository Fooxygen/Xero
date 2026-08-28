
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "llvm/IR/Value.h"

#include "sema/defs/fn.hpp"

namespace xcompiler {
    class IRGen;

    // LibC

    llvm::Function* LibC_printf(IRGen& gen);

    // Built-in Fn

    void BuiltinFnRegister(sema::FnTable& fn_table);
}

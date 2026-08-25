
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "llvm/IR/Module.h"
#include "llvm/Passes/OptimizationLevel.h"

namespace compiler {

    class Optimizer {
    public:
        void Run(llvm::Module& module, llvm::OptimizationLevel level);
    };
}

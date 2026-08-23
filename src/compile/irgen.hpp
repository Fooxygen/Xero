
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>
#include <string>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include "common/ast.hpp"

namespace compile {

    class IRGen {
    private:
        llvm::LLVMContext             context_;
        std::unique_ptr<llvm::Module> module_;
        llvm::IRBuilder<>             builder_;

    public:
        IRGen(std::string_view module_name);

        // Exec

        void Exec(AstNode& root, std::string path);
    };
}

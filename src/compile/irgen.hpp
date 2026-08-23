
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
#include "sema/type.hpp"

namespace compile {

    class IRGen {
    private:
        llvm::LLVMContext             context_;
        std::unique_ptr<llvm::Module> module_;
        llvm::IRBuilder<>             builder_;

        // Exec

        llvm::Value* Exec(BlockExpr& node);
        llvm::Value* Exec(FnExpr& node);

        llvm::Value* Exec(NumConst& node);

        llvm::Value* Exec(ReturnSignalStmt& node);

        llvm::Value* Exec(Program& node);

    public:
        IRGen(std::string_view module_name, std::string path, AstNode& root);

        llvm::Type* LlvmType(const sema::Type* type);

        // Exec

        llvm::Value* Exec(AstNode& node) {
            switch (node.type_) {
                case AstType::BlockExpr:        return Exec((BlockExpr&)node);
                case AstType::FnExpr:           return Exec((FnExpr&)node);
                
                case AstType::NumConst:         return Exec((NumConst&)node);
                
                case AstType::ReturnSignalStmt: return Exec((ReturnSignalStmt&)node);

                case AstType::Program:          return Exec((Program&)node);

                default: return nullptr;
            }
        }
    };
}

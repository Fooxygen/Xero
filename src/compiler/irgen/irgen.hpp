
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
#include "compiler/irgen/value.hpp"

namespace compiler {

    class IRGen {
    private:
        llvm::LLVMContext             context_;
        std::unique_ptr<llvm::Module> module_;
        llvm::IRBuilder<>             builder_;
        ValueTable                    value_table_;

        // Cache
        llvm::Function* current_fn_ = nullptr;

    private:
        // Utility

        llvm::Type* LlvmType(const sema::Type* type);

        llvm::AllocaInst* EntryBlockSlotCreate(llvm::Type* type, const std::string& name);

        // Exec

        llvm::Value* Exec(BlockExpr& node, std::function<void()> OnScopeReady = nullptr);
        llvm::Value* Exec(IdExpr& node);
        llvm::Value* Exec(DeclExpr& node);
        llvm::Value* Exec(OperExpr& node);
        llvm::Value* Exec(FnExpr& node);

        llvm::Value* Exec(NumConst& node);

        llvm::Value* Exec(ExprStmt& node);
        llvm::Value* Exec(AssignStmt& node);
        llvm::Value* Exec(ReturnSignalStmt& node);

        llvm::Value* Exec(Program& node);

    public:
        IRGen(std::string_view module_name);

        llvm::Module*      module()  { return module_.get(); }
        llvm::IRBuilder<>& builder() { return builder_; }

        // Workflow

        void IROutput(const std::string& path);
        void ObjectCodeOutput(const std::string& path);

        // Exec

        llvm::Value* Exec(AstNode& node) {
            switch (node.type_) {
                case AstType::BlockExpr:        return Exec((BlockExpr&)node);
                case AstType::IdExpr:           return Exec((IdExpr&)node);
                case AstType::DeclExpr:         return Exec((DeclExpr&)node);
                case AstType::OperExpr:         return Exec((OperExpr&)node);
                case AstType::FnExpr:           return Exec((FnExpr&)node);
                
                case AstType::NumConst:         return Exec((NumConst&)node);
                
                case AstType::ExprStmt:         return Exec((ExprStmt&)node);
                case AstType::AssignStmt:       return Exec((AssignStmt&)node);
                case AstType::ReturnSignalStmt: return Exec((ReturnSignalStmt&)node);

                case AstType::Program:          return Exec((Program&)node);

                default: return nullptr;
            }
        }
    };
}

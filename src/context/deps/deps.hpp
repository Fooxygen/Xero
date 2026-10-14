
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>

#include "common/defs/ast.hpp"
#include "common/defs/path.hpp"
#include "context/module.hpp"

namespace context {

    class Deps {
    private:
        Module& module_;

    private:
        std::vector<Path> paths_ = {};

        // Process

        void Process(Exprs& node);
        void Process(BlockExpr& node);
        void Process(RefExpr& node);
        void Process(TypeExpr& node);
        void Process(DeclExpr& node);
        void Process(OperExpr& node);
        void Process(RangeExpr& node);
        void Process(ArrayExpr& node);
        void Process(FnCallExpr& node);
        void Process(MethodCallExpr& node);
        void Process(FnExpr& node);
        void Process(PathExpr& node);
        void Process(UseExpr& node);

        void Process(ExprStmt& node);
        void Process(AssignStmt& node);
        void Process(CondStmt& node);
        void Process(ReturnSignalStmt& node);
        void Process(ForStmt& node);
        void Process(WhileStmt& node);

        void Process(Program& node);

    public:
        Deps(Module& module)
        :   module_(module) {}

        const std::vector<Path>& paths() const { return paths_; }

    public:
        // Process

        void Process(AstNode& node) {
            switch (node.type_) {
                case AstType::Exprs:            Process((Exprs&)node);            return;
                case AstType::BlockExpr:        Process((BlockExpr&)node);        return;
                case AstType::RefExpr:          Process((RefExpr&)node);          return;
                case AstType::TypeExpr:         Process((TypeExpr&)node);         return;
                case AstType::DeclExpr:         Process((DeclExpr&)node);         return;
                case AstType::OperExpr:         Process((OperExpr&)node);         return;
                case AstType::RangeExpr:        Process((RangeExpr&)node);        return;
                case AstType::ArrayExpr:        Process((ArrayExpr&)node);        return;
                case AstType::FnCallExpr:       Process((FnCallExpr&)node);       return;
                case AstType::MethodCallExpr:   Process((MethodCallExpr&)node);   return;
                case AstType::FnExpr:           Process((FnExpr&)node);           return;
                case AstType::PathExpr:         Process((PathExpr&)node);         return;
                case AstType::UseExpr:          Process((UseExpr&)node);          return;
                
                case AstType::ExprStmt:         Process((ExprStmt&)node);         return;
                case AstType::AssignStmt:       Process((AssignStmt&)node);       return;
                case AstType::CondStmt:         Process((CondStmt&)node);         return;
                case AstType::ReturnSignalStmt: Process((ReturnSignalStmt&)node); return;
                case AstType::ForStmt:          Process((ForStmt&)node);          return;
                case AstType::WhileStmt:        Process((WhileStmt&)node);        return;
                
                case AstType::Program:          Process((Program&)node);          return;

                default: return;
            }
        }
    };
}

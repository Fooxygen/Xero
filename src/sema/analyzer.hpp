
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <functional>

#include "common/defs/ast.hpp"
#include "sema/defs/var.hpp"
#include "sema/defs/fn.hpp"

namespace sema {
    
    class Analyzer {
    private:
        // Table

        VarTable var_table_;                // scope variable table
        FnTable& fn_table_;                 // global function table

        // Declare

        void Declare(FnExpr& node);         // signature of global function

        void Declare(Program& node);        // module level

        // Process

        void Process(BlockExpr& node, const std::function<void()>& on_scope_ready = nullptr);
        void Process(IdExpr& node);
        void Process(RefExpr& node);
        void Process(TypeExpr& node);
        void Process(DeclExpr& node);
        void Process(OperExpr& node);
        void Process(RangeExpr& node);
        void Process(ArrayExpr& node);
        void Process(FnCallExpr& node);
        void Process(MethodCallExpr& node);
        void Process(FnExpr& node);

        void Process(NumConst& node);
        void Process(BoolConst& node);
        void Process(CharConst& node);
        void Process(StringConst& node);

        void Process(ExprStmt& node);
        void Process(AssignStmt& node);
        void Process(CondStmt& node);
        void Process(ReturnSignalStmt& node);
        void Process(ForStmt& node);
        void Process(WhileStmt& node);

        void Process(Program& node);

    public:
        Analyzer(FnTable& fn_table) : fn_table_(fn_table) {}
    
    public:
        // Builtin

        void BuiltinFnRegister();

        // Declare

        void Declare(AstNode& node) {
            switch (node.type_) {
                case AstType::FnExpr:  Declare((FnExpr&)node);  return;

                case AstType::Program: Declare((Program&)node); return;

                default: return;
            }
        }

        // Process

        void Process(AstNode& node) {
            switch (node.type_) {
                case AstType::BlockExpr:        Process((BlockExpr&)node);         return;
                case AstType::IdExpr:           Process((IdExpr&)node);            return;
                case AstType::RefExpr:          Process((RefExpr&)node);           return;
                case AstType::TypeExpr:         Process((TypeExpr&)node);          return;
                case AstType::DeclExpr:         Process((DeclExpr&)node);          return;
                case AstType::OperExpr:         Process((OperExpr&)node);          return;
                case AstType::RangeExpr:        Process((RangeExpr&)node);         return;
                case AstType::ArrayExpr:        Process((ArrayExpr&)node);         return;
                case AstType::FnCallExpr:       Process((FnCallExpr&)node);        return;
                case AstType::MethodCallExpr:   Process((MethodCallExpr&)node);    return;
                case AstType::FnExpr:           Process((FnExpr&)node);            return;

                case AstType::NumConst:         Process((NumConst&)node);          return;
                case AstType::BoolConst:        Process((BoolConst&)node);         return;
                case AstType::CharConst:        Process((CharConst&)node);         return;
                case AstType::StringConst:      Process((StringConst&)node);       return;

                case AstType::ExprStmt:         Process((ExprStmt&)node);          return;
                case AstType::AssignStmt:       Process((AssignStmt&)node);        return;
                case AstType::CondStmt:         Process((CondStmt&)node);          return;
                case AstType::ReturnSignalStmt: Process((ReturnSignalStmt&)node);  return;
                case AstType::ForStmt:          Process((ForStmt&)node);           return;
                case AstType::WhileStmt:        Process((WhileStmt&)node);         return;

                case AstType::Program:          Process((Program&)node);           return;

                default: return;
            }
        }
    };
}

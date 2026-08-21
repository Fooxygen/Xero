
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <functional>

#include "common/ast.hpp"
#include "sema/symbol.hpp"

namespace sema {
    using AT      = AstType;
    using OT      = OperType;

    class Sema {
    private:
        // Symbol
    
        SymbolTable symbol_table_;

        // Exec

        void Exec(BlockExpr& node, std::function<void()> OnScopeReady = nullptr);
        void Exec(IdExpr& node);
        void Exec(TypeExpr& node);
        void Exec(DeclExpr& node);
        void Exec(OperExpr& node);
        void Exec(RangeExpr& node);
        void Exec(ArrayExpr& node);
        void Exec(FnCallExpr& node);
        void Exec(MethodCallExpr& node);
        void Exec(FnExpr& node);

        void Exec(NumConst& node);
        void Exec(BoolConst& node);
        void Exec(CharConst& node);
        void Exec(StringConst& node);

        void Exec(ExprStmt& node);
        void Exec(AssignStmt& node);
        void Exec(CondStmt& node);
        void Exec(ReturnSignalStmt& node);
        void Exec(ForStmt& node);
        void Exec(WhileStmt& node);

        void Exec(Program& node);

    public:

        // Fn

        void BuiltinFnRegister();

        void        FnCallCheck(const FnSymbol& fn, const std::vector<const Type*>& args, Loc loc);
        std::string FnParamsPrint(const std::vector<const Type*>& params);
        std::string FnParamsPrint(const FnSymbol& fn);

        // Exec

        void Exec(AstNode& node) {
            switch (node.type_) {
                case AstType::BlockExpr:        Exec((BlockExpr&)node);         return;
                case AstType::IdExpr:           Exec((IdExpr&)node);            return;
                case AstType::TypeExpr:         Exec((TypeExpr&)node);          return;
                case AstType::DeclExpr:         Exec((DeclExpr&)node);          return;
                case AstType::OperExpr:         Exec((OperExpr&)node);          return;
                case AstType::RangeExpr:        Exec((RangeExpr&)node);         return;
                case AstType::ArrayExpr:        Exec((ArrayExpr&)node);         return;
                case AstType::FnCallExpr:       Exec((FnCallExpr&)node);        return;
                case AstType::MethodCallExpr:   Exec((MethodCallExpr&)node);    return;
                case AstType::FnExpr:           Exec((FnExpr&)node);            return;

                case AstType::NumConst:         Exec((NumConst&)node);          return;
                case AstType::BoolConst:        Exec((BoolConst&)node);         return;
                case AstType::CharConst:        Exec((CharConst&)node);         return;
                case AstType::StringConst:      Exec((StringConst&)node);       return;

                case AstType::ExprStmt:         Exec((ExprStmt&)node);          return;
                case AstType::AssignStmt:       Exec((AssignStmt&)node);        return;
                case AstType::CondStmt:         Exec((CondStmt&)node);          return;
                case AstType::ReturnSignalStmt: Exec((ReturnSignalStmt&)node);  return;
                case AstType::ForStmt:          Exec((ForStmt&)node);           return;
                case AstType::WhileStmt:        Exec((WhileStmt&)node);         return;

                case AstType::Program:          Exec((Program&)node);           return;

                default: return;
            }
        }
    };
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "common/ast.hpp"
#include "table/opertable.hpp"
#include "obj.hpp"
#include "env.hpp"

namespace rt {

    class Xengine {
    private:
        Env env_;

    public:
        Xengine() {
            TypeTable::Reset();
            OperTable::Reset();
            OperTableRegister();
            BinfnRegister();
        }
        
        void OperTableRegister();
        void BinfnRegister();

        Obj Exec(AstNode& node) {
            switch (node.type_) {
                case AstType::IdExpr:       return Exec((IdExpr&)node);
                case AstType::OperExpr:     return Exec((OperExpr&)node);
                case AstType::NegExpr:      return Exec((NegExpr&)node);
                case AstType::CallExpr:     return Exec((CallExpr&)node);

                case AstType::NumConst:     return Exec((NumConst&)node);

                case AstType::DeclStmt:     return Exec((DeclStmt&)node);
                case AstType::AssignStmt:   return Exec((AssignStmt&)node);

                case AstType::Program:      return Exec((Program&)node);

                default: return Obj();
            }
        }

        Obj Exec(IdExpr& node);
        Obj Exec(OperExpr& node);
        Obj Exec(NegExpr& node);
        Obj Exec(CallExpr& node);

        Obj Exec(NumConst& node);

        Obj Exec(DeclStmt& node);
        Obj Exec(AssignStmt& node);

        Obj Exec(Program& node);
    };
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "common/ast.hpp"
#include "obj.hpp"
#include "env.hpp"

namespace rt {

    class Engine {
    private:
        Env env_;

    public:
        Obj Exec(AstNode& node) {
            switch (node.type_) {
                case AstType::IdExpr:   return Exec((IdExpr&)node);
                case AstType::OperExpr: return Exec((OperExpr&)node);

                case AstType::NumConst: return Exec((NumConst&)node);

                case AstType::DeclStmt: return Exec((DeclStmt&)node);

                case AstType::Program:  return Exec((Program&)node);

                default: return Obj();
            }
        }

        Obj Exec(IdExpr& node);
        Obj Exec(OperExpr& node);

        Obj Exec(NumConst& node);

        Obj Exec(DeclStmt& node);

        Obj Exec(Program& node);
    };
}

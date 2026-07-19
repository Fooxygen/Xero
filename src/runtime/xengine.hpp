
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <functional>

#include "common/ast.hpp"
#include "obj/obj.hpp"
#include "env.hpp"

namespace rt {

    class Xengine {
    private:
        Env env_;

    public:
        Xengine() {
            TypeTable::Reset();

            TypeRegister();
            FnRegister();
            MethodRegister();
        }
        
        void TypeRegister();
        void FnRegister();
        void MethodRegister();

        Obj Exec(AstNode& node) {
            switch (node.type_) {
                case AstType::IdExpr:           return Exec((IdExpr&)node);
                case AstType::OperExpr:         return Exec((OperExpr&)node);
                case AstType::NegExpr:          return Exec((NegExpr&)node);
                case AstType::NotExpr:          return Exec((NotExpr&)node);
                case AstType::FnCallExpr:       return Exec((FnCallExpr&)node);
                case AstType::MethodCallExpr:   return Exec((MethodCallExpr&)node);
                case AstType::ArrayExpr:        return Exec((ArrayExpr&)node);

                case AstType::NumConst:         return Exec((NumConst&)node);
                case AstType::BoolConst:        return Exec((BoolConst&)node);
                case AstType::StringConst:      return Exec((StringConst&)node);

                case AstType::BlockStmt:        return Exec((BlockStmt&)node);
                case AstType::DeclStmt:         return Exec((DeclStmt&)node);
                case AstType::AssignStmt:       return Exec((AssignStmt&)node);
                case AstType::CondStmt:         return Exec((CondStmt&)node);
                case AstType::ForStmt:          return Exec((ForStmt&)node);

                case AstType::Program:          return Exec((Program&)node);

                default: return Obj();
            }
        }

        Obj Exec(IdExpr& node);
        Obj Exec(OperExpr& node);
        Obj Exec(NegExpr& node);
        Obj Exec(NotExpr& node);
        Obj Exec(FnCallExpr& node);
        Obj Exec(MethodCallExpr& node);
        Obj Exec(ArrayExpr& node);

        Obj Exec(NumConst& node);
        Obj Exec(BoolConst& node);
        Obj Exec(StringConst& node);

        Obj Exec(BlockStmt& node, std::function<void()> OnScopeReady = nullptr);
        Obj Exec(DeclStmt& node);
        Obj Exec(AssignStmt& node);
        Obj Exec(CondStmt& node);
        Obj Exec(ForStmt& node);

        Obj Exec(Program& node);

    public:
        // Call Wrapper

        // Throw while call equals null
        template<typename Call, typename... Args>
        static decltype(auto) CallThrow(Call call, Args&&... args) {
            if (!call) throw LogErr(LogModule::Runtime, "call not implemented for type");
            return call(std::forward<Args>(args)...);
        }

        // Return None Obj while call equals null
        template<typename Call, typename... Args>
        static decltype(auto) CallTry(Call call, Args&&... args) {
            if (!call) return Obj();
            return call(std::forward<Args>(args)...);
        }
    };
}

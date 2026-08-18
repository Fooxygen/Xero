
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <functional>

#include "common/ast.hpp"
#include "runtime/obj/obj.hpp"
#include "runtime/env.hpp"

namespace rt {

    class Xengine {
    private:
        Env env_;

        // Exec

        Obj Exec(BlockExpr& node, std::function<void()> OnScopeReady = nullptr);
        Obj Exec(IdExpr& node);
        Obj Exec(DeclExpr& node);
        Obj Exec(OperExpr& node);
        Obj Exec(RangeExpr& node);
        Obj Exec(ArrayExpr& node);
        Obj Exec(FnCallExpr& node);
        Obj Exec(MethodCallExpr& node);
        Obj Exec(FnExpr& node);

        Obj Exec(NumConst& node);
        Obj Exec(BoolConst& node);
        Obj Exec(CharConst& node);
        Obj Exec(StringConst& node);

        Obj Exec(ExprStmt& node);
        Obj Exec(AssignStmt& node);
        Obj Exec(CondStmt& node);
        Obj Exec(LoopSignalStmt& node);
        Obj Exec(ReturnSignalStmt& node);
        Obj Exec(ForStmt& node);
        Obj Exec(WhileStmt& node);

        Obj Exec(Program& node);

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
    
    public:
      
        // Fn

        void BuiltinFnImplRegister();

        // Exec
        // Calculate the final obj
        Obj Exec(AstNode& node) {
            switch (node.type_) {
                case AstType::BlockExpr:        return Exec((BlockExpr&)node);
                case AstType::IdExpr:           return Exec((IdExpr&)node);
                case AstType::DeclExpr:         return Exec((DeclExpr&)node);
                case AstType::OperExpr:         return Exec((OperExpr&)node);
                case AstType::RangeExpr:        return Exec((RangeExpr&)node);
                case AstType::ArrayExpr:        return Exec((ArrayExpr&)node);
                case AstType::FnCallExpr:       return Exec((FnCallExpr&)node);
                case AstType::MethodCallExpr:   return Exec((MethodCallExpr&)node);
                case AstType::FnExpr:           return Exec((FnExpr&)node);

                case AstType::NumConst:         return Exec((NumConst&)node);
                case AstType::BoolConst:        return Exec((BoolConst&)node);
                case AstType::CharConst:        return Exec((CharConst&)node);
                case AstType::StringConst:      return Exec((StringConst&)node);

                case AstType::ExprStmt:         return Exec((ExprStmt&)node);
                case AstType::AssignStmt:       return Exec((AssignStmt&)node);
                case AstType::CondStmt:         return Exec((CondStmt&)node);
                case AstType::LoopSignalStmt:   return Exec((LoopSignalStmt&)node);
                case AstType::ReturnSignalStmt: return Exec((ReturnSignalStmt&)node);
                case AstType::ForStmt:          return Exec((ForStmt&)node);
                case AstType::WhileStmt:        return Exec((WhileStmt&)node);

                case AstType::Program:          return Exec((Program&)node);

                default: return Obj();
            }
        }
    };
}

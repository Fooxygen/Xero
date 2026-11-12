
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <functional>

#include "common/defs/ast.hpp"
#include "context/module.hpp"
#include "sema/defs/var.hpp"
#include "sema/defs/fn.hpp"

namespace sema {
    
    class Analyzer {
    private:
        // Table

        VarScope            var_scope_;             // scope variable table
        ModuleConstantScope module_const_scope_;    // module-level constant table
        TypeScope           type_scope_;            // global type scope
        FnScope&            fn_scope_;              // global function scope

        // Module

        const context::Module* module_ = nullptr;

        // Function

        size_t fn_depth_ = 0;

        // Module Constant

        Type* module_constant_i32_  = nullptr;
        Type* module_constant_i64_  = nullptr;
        Type* module_constant_f32_  = nullptr;
        Type* module_constant_f64_  = nullptr;
        Type* module_constant_bool_ = nullptr;
        Type* module_constant_char_ = nullptr;

        bool ModuleConstantStructureCheck(Expr& node);
        void ModuleConstantTypeCheck(Expr& node, Type* expected);
        bool ModuleConstantTypeSupported(Type* type);
        bool ModuleConstantCastSupported(Type* from, Type* to);

        // Type

        bool AssignableCheck(Type* from, Type* to) {
            if (from == to) return true;
            if (dynamic_cast<ReferenceType*>(to)) return false;
            return TypeTable::CommonTypeGet({ from->ReferenceUnwrap(), to }) == to;
        }

        // Declare

        void Declare(DeclExpr& node);                                           // signature of global constant
        void Declare(FnExpr& node, FnScope& fn_scope, bool is_named = true);    // signature of function
        void Declare(StructDeclExpr& node);                                     // signature of global struct

        void Declare(Program& node);                                            // module level

        // Process

        void Process(BlockExpr& node, const std::function<void()>& on_scope_ready = nullptr);
        void Process(TypeExpr& node);
        void Process(DeclExpr& node);
        void Process(IdExpr& node);
        void Process(RefExpr& node);
        void Process(OperExpr& node);
        void Process(RangeExpr& node);
        void Process(ArrayLitExpr& node);
        void Process(FnExpr& node);
        void Process(FnCallExpr& node);
        void Process(MethodCallExpr& node);
        void Process(StructDeclExpr& node);
        void Process(StructLitExpr& node);
        void Process(FieldAccessExpr& node);
        void Process(PathExpr& node);

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
        Analyzer(FnScope& fn_scope) : fn_scope_(fn_scope) {}

        ModuleConstantScope& module_const_scope() { return module_const_scope_; }
        TypeScope&           type_scope()         { return type_scope_; }
        FnScope&             fn_scope()           { return fn_scope_; }

    public:
        // Builtin

        void BuiltinFnRegister();

        // Module Constant

        void ModuleConstantTypeCacheInit();     // init supported type for constant

        // Module

        void ModuleSet(const context::Module& module) { module_ = &module; }

        // Declare

        void Declare(AstNode& node) {
            switch (node.type_) {
                case AstType::DeclExpr:         Declare((DeclExpr&)node);           return;
                case AstType::FnExpr:           Declare((FnExpr&)node, fn_scope_);  return;
                case AstType::StructDeclExpr:   Declare((StructDeclExpr&)node);     return;

                case AstType::Program:          Declare((Program&)node);            return;

                default: return;
            }
        }

        // Process

        void Process(AstNode& node) {
            switch (node.type_) {
                case AstType::BlockExpr:        Process((BlockExpr&)node);         return;
                case AstType::TypeExpr:         Process((TypeExpr&)node);          return;
                case AstType::DeclExpr:         Process((DeclExpr&)node);          return;
                case AstType::IdExpr:           Process((IdExpr&)node);            return;
                case AstType::RefExpr:          Process((RefExpr&)node);           return;
                case AstType::OperExpr:         Process((OperExpr&)node);          return;
                case AstType::RangeExpr:        Process((RangeExpr&)node);         return;
                case AstType::ArrayLitExpr:     Process((ArrayLitExpr&)node);      return;
                case AstType::FnExpr:           Process((FnExpr&)node);            return;
                case AstType::FnCallExpr:       Process((FnCallExpr&)node);        return;
                case AstType::MethodCallExpr:   Process((MethodCallExpr&)node);    return;
                case AstType::StructDeclExpr:   Process((StructDeclExpr&)node);    return;
                case AstType::StructLitExpr:    Process((StructLitExpr&)node);     return;
                case AstType::FieldAccessExpr:  Process((FieldAccessExpr&)node);   return;
                case AstType::PathExpr:         Process((PathExpr&)node);          return;

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

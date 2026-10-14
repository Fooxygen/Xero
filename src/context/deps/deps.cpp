
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "context/deps/deps.hpp"

namespace context {

    // Process

    // └─ Common

    void Deps::Process(Exprs& node) {
        for (auto& e : node.exprs_) Process(*e);
    }

    // └─ Expr

    void Deps::Process(BlockExpr& node) {
        for (auto& child : node.children_) Process(*child);
    }

    void Deps::Process(RefExpr& node) {
        if (node.target_) Process(*node.target_);
    }

    void Deps::Process(TypeExpr& node) {
        if (node.params_) Process(*node.params_);
    }

    void Deps::Process(DeclExpr& node) {
        if (node.bind_type_) Process(*node.bind_type_);
        if (node.value_)     Process(*node.value_);
    }

    void Deps::Process(OperExpr& node) {
        if (node.lexpr_) Process(*node.lexpr_);
        if (node.rexpr_) Process(*node.rexpr_);
    }

    void Deps::Process(RangeExpr& node) {
        if (node.lexpr_) Process(*node.lexpr_);
        if (node.rexpr_) Process(*node.rexpr_);
        if (node.step_)  Process(*node.step_);
    }

    void Deps::Process(ArrayExpr& node) {
        if (node.elems_) Process(*node.elems_);
    }

    void Deps::Process(FnCallExpr& node) {
        if (node.callee_) Process(*node.callee_);
        if (node.args_)   Process(*node.args_);
    }

    void Deps::Process(MethodCallExpr& node) {
        if (node.caller_) Process(*node.caller_);
        if (node.callee_) Process(*node.callee_);
        if (node.args_)   Process(*node.args_);
    }

    void Deps::Process(FnExpr& node) {
        if (node.return_type_) Process(*node.return_type_);
        if (node.params_)      Process(*node.params_);
        if (node.body_)        Process(*node.body_);
    }

    void Deps::Process(PathExpr& node) {
        paths_.emplace_back(&node);
    }

    // └─ Stmt

    void Deps::Process(ExprStmt& node) {
        if (node.expr_) Process(*node.expr_);
    }

    void Deps::Process(AssignStmt& node) {
        if (node.target_) Process(*node.target_);
        if (node.value_)  Process(*node.value_);
    }

    void Deps::Process(CondStmt& node) {
        if (node.cond_) Process(*node.cond_);
        if (node.then_) Process(*node.then_);
        if (node.next_) Process(*node.next_);
    }

    void Deps::Process(ReturnSignalStmt& node) {
        if (node.value_) Process(*node.value_);
    }

    void Deps::Process(ForStmt& node) {
        if (node.iter_) Process(*node.iter_);
        if (node.data_) Process(*node.data_);
        if (node.body_) Process(*node.body_);
    }

    void Deps::Process(WhileStmt& node) {
        if (node.cond_) Process(*node.cond_);
        if (node.body_) Process(*node.body_);
    }

    // └─ Program

    void Deps::Process(Program& node) {
        for (auto& child : node.children_) Process(*child);
    }
}

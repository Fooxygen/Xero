
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema.hpp"
#include "semantics.hpp"

namespace sema {

    // Expr

    void Sema::Exec(BlockExpr& node) {
        for (auto& child : node.children_) Exec(*child);
    }

    void Sema::Exec(DeclExpr& node) {
        if (node.value_) Exec(*node.value_);
    }

    void Sema::Exec(OperExpr& node) {
        Exec(*node.lexpr_);
        if (node.rexpr_) Exec(*node.rexpr_);
    }

    void Sema::Exec(RangeExpr& node) {
        Exec(*node.lexpr_);
        Exec(*node.rexpr_);
        if (node.step_) Exec(*node.step_);
    }

    void Sema::Exec(ArrayExpr& node) {
        for (auto& e : node.elements_->exprs_) Exec(*e);
    }

    void Sema::Exec(FnCallExpr& node) {
        Exec(*node.callee_);
        for (auto& e : node.args_->exprs_) Exec(*e);
    }

    void Sema::Exec(MethodCallExpr& node) {
        Exec(*node.target_);
        Exec(*node.callee_);
        for (auto& e : node.args_->exprs_) Exec(*e);
    }

    void Sema::Exec(FnExpr& node) {
        if (node.params_) {
            for (auto& p : node.params_->exprs_) Exec(*p);
        }
        if (node.block_)  Exec(*node.block_);
    }

    // Const

    void Sema::Exec(NumConst& node) {
        const auto& numstr = node.value_;

        // integer
        if (!numstr.contains(".")) {
            
            // i32
            {
                int32_t x = 0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) {
                    node.resolved_type_ = sema::TypeTable::Get("i32");
                    node.resolved_value_.integer = x;
                    return;
                }
            }

            // i64
            {
                int64_t x = 0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) {
                    node.resolved_type_ = sema::TypeTable::Get("i64");
                    node.resolved_value_.integer = x;
                    return;
                }
            }
        }

        // floating
        else {
            // error: 3.14.15
            if (numstr.substr(numstr.find(".") + 1).contains(".")) {
                throw LogErr(LogModule::Sema, std::format(
                    "invalid float format '{}'", numstr
                ), node.loc_);
            }

            // f32
            {
                float x = 0.0f;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) {
                    node.resolved_type_ = sema::TypeTable::Get("f32");
                    node.resolved_value_.floating = x;
                    return;
                }
            }

            // f64
            {
                double x = 0.0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) {
                    node.resolved_type_ = sema::TypeTable::Get("f64");
                    node.resolved_value_.floating = x;
                    return;
                }
            }
        }

        throw LogErr(LogModule::Sema, std::format(
            "numeric overflow '{}'", numstr
        ), node.loc_);
    }

    // Stmt

    void Sema::Exec(ExprStmt& node) {
        Exec(*node.expr_);
    }

    void Sema::Exec(AssignStmt& node) {
        Exec(*node.target_);
        Exec(*node.value_);
    }

    void Sema::Exec(CondStmt& node) {
        if (node.cond_)  Exec(*node.cond_);
        Exec(*node.block_);
        if (node.sub_)   Exec(*node.sub_);
    }

    void Sema::Exec(ReturnSignalStmt& node) {
        if (node.value_) Exec(*node.value_);
    }

    void Sema::Exec(ForStmt& node) {
        Exec(*node.iter_);
        Exec(*node.data_);
        Exec(*node.block_);
    }

    void Sema::Exec(WhileStmt& node) {
        Exec(*node.cond_);
        Exec(*node.block_);
    }

    // Common

    void Sema::Exec(Program& node) {
        Exec((BlockExpr&)node);
    }
}

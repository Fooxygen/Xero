
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema.hpp"
#include "semantics.hpp"

namespace sema {

    // Expr

    void Sema::Exec(BlockExpr& node, std::function<void()> OnScopeReady) {
        node.resolved_type_ = TypeTable::Get("none");

        symbol_table_.ScopePush();
        if (OnScopeReady) OnScopeReady();

        try {
            for (auto& child : node.children_) Exec(*child);
        }
        catch (...) {
            symbol_table_.ScopePop();
            throw;
        }
        
        symbol_table_.ScopePop();
    }

    void Sema::Exec(IdExpr& node) {

        // Resolved Type
        auto sym = symbol_table_.Lookup(node.value_, node.loc_);
        if (sym->type_ == SymbolType::Var) {
            node.resolved_type_ = ((VarSymbol*)sym)->var_type_;
            return;
        }

        throw LogErr(LogModule::Sema, std::format(
            "'{}' is not a variable", sym->name_
        ), node.loc_);
    }

    void Sema::Exec(DeclExpr& node) {
        node.resolved_type_ = sema::TypeTable::Get(node.bind_type_);

        symbol_table_.Declare(std::make_unique<VarSymbol>(
            node.id_, node.loc_, node.resolved_type_
        ));
        if (node.value_) Exec(*node.value_);
    }

    void Sema::Exec(OperExpr& node) {

        // Unary
        Exec(*node.lexpr_);
        {
            if (node.oper_type_ == OperType::Neg) {
                node.resolved_type_ = node.lexpr_->resolved_type_;
                return;
            }
            if (node.oper_type_ == OperType::Not) {
                node.resolved_type_ = TypeTable::Get("bool");
                return;
            }
        }

        // Binary
        Exec(*node.rexpr_);
        {
            
            if (node.oper_type_ == OperType::Pick) {
                return;
            }

            // Boolean
            // TODO: Consider overloading–boolean constraint conflict
            switch (node.oper_type_) {
                case OperType::Gt:
                case OperType::Lt:
                case OperType::Ge:
                case OperType::Le:
                case OperType::Eq:
                case OperType::Neq:
                case OperType::And:
                case OperType::Or:
                    node.resolved_type_ = TypeTable::Get("bool");
                    return;

                default: break;
            }

            // Arith
            node.resolved_type_ = sema::TypeTable::Common({
                node.lexpr_->resolved_type_,
                node.rexpr_->resolved_type_
            });
            if (!node.resolved_type_) {
                throw LogErr(LogModule::Sema, std::format(
                    "cannot match type '{}' to type '{}'",
                    node.lexpr_->resolved_type_->name,
                    node.rexpr_->resolved_type_->name
                ), node.loc_);
            }

            return;
        }
    }

    void Sema::Exec(RangeExpr& node) {
        node.resolved_type_ = TypeTable::Get("range");

        Exec(*node.lexpr_);
        Exec(*node.rexpr_);
        if (node.step_) Exec(*node.step_);
    }

    void Sema::Exec(ArrayExpr& node) {
        node.resolved_type_ = TypeTable::Get("array");

        for (auto& e : node.elements_->exprs_) Exec(*e);
    }

    void Sema::Exec(FnCallExpr& node) {
        node.resolved_type_ = nullptr;
    }

    void Sema::Exec(MethodCallExpr& node) {
        node.resolved_type_ = nullptr;
    }

    void Sema::Exec(FnExpr& node) {
        node.resolved_type_ = TypeTable::Get("function");

        auto& params_expr = node.params_->exprs_;
        std::vector<const Type*> params_type;
        for (auto& e : params_expr) {
            auto expr = (DeclExpr*)e.get();
            params_type.emplace_back(TypeTable::Get(expr->bind_type_));
        }

        if (!node.name_.empty()) {
            symbol_table_.Declare(std::make_unique<FnSymbol>(
                node.name_, node.loc_,
                TypeTable::Get(node.ret_type_), params_type
            ));
        }
        if (node.block_) {
            Exec(*node.block_, [&]() {
                for (size_t i = 0; i < params_expr.size(); i++) {
                    auto expr = (DeclExpr*)(params_expr[i].get());
                    symbol_table_.Declare(std::make_unique<VarSymbol>(
                        expr->id_, expr->loc_, params_type[i])
                    );
                }
            });
        }
    }

    // Const

    void Sema::Exec(NumConst& node) {
        const auto& numstr = node.value_;

        // Resolved Type and Value

        // Integer
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

        // Floating
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

    void Sema::Exec(BoolConst& node) {
        node.resolved_type_ = TypeTable::Get("bool");
    }

    void Sema::Exec(CharConst& node) {
        node.resolved_type_ = TypeTable::Get("char");
    }

    void Sema::Exec(StringConst& node) {
        node.resolved_type_ = TypeTable::Get("string");
    }

    // Stmt

    void Sema::Exec(ExprStmt& node) {
        node.resolved_type_ = TypeTable::Get("none");

        Exec(*node.expr_);
    }

    void Sema::Exec(AssignStmt& node) {
        node.resolved_type_ = TypeTable::Get("none");

        Exec(*node.target_);
        Exec(*node.value_);
    }

    void Sema::Exec(CondStmt& node) {
        node.resolved_type_ = TypeTable::Get("none");

        if (node.cond_)  Exec(*node.cond_);
        Exec(*node.block_);
        if (node.sub_)   Exec(*node.sub_);
    }

    void Sema::Exec(ReturnSignalStmt& node) {
        node.resolved_type_ = TypeTable::Get("none");

        if (node.value_) Exec(*node.value_);
    }

    void Sema::Exec(ForStmt& node) {
        node.resolved_type_ = TypeTable::Get("none");
        return;
    }

    void Sema::Exec(WhileStmt& node) {
        node.resolved_type_ = TypeTable::Get("none");

        Exec(*node.cond_);
        Exec(*node.block_);
    }

    // Common

    void Sema::Exec(Program& node) {
        node.resolved_type_ = TypeTable::Get("none");

        Exec((BlockExpr&)node);
    }
}

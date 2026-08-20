
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/sema.hpp"
#include "sema/type.hpp"
#include "sema/method.hpp"

namespace sema {

    // Fn

    void Sema::FnCallCheck(const FnSymbol& fn, const std::vector<const Type*>& args, Loc loc) {
        auto& params_fix = fn.params_fix_;
        auto& param_inf  = fn.param_inf_;
        
        try {
            // Fixed
            if (args.size() < params_fix.size()) throw 0;
            for (size_t i = 0; i < params_fix.size(); i++) {
                if (params_fix[i] && !args[i]->converts_.contains(params_fix[i])) throw 0;
            }

            // Infinite
            if (param_inf) {
                for (size_t i = params_fix.size(); i < args.size(); i++) {
                    if (*param_inf && !args[i]->converts_.contains(*param_inf)) throw 0;
                }
            }
            else {
                if (args.size() > params_fix.size()) throw 0;
            }
        }
        catch (...) {
            throw LogErr(LogModule::Sema, std::format(
                "'{}' expects {}, got {}",
                fn.name_, FnParamsPrint(fn), FnParamsPrint(args)
            ), loc);
        }
    }

    std::string Sema::FnParamsPrint(const std::vector<const Type*>& params) {
        std::string res = "(";
        for (size_t i = 0; i < params.size(); i++) {
            if (i != 0) res += ", ";
            res += params[i]->name_;
        }
        return res.empty() ? "empty)" : res + ')';
    }

    std::string Sema::FnParamsPrint(const FnSymbol& fn) {
        std::string res = "(";

        for (size_t i = 0; i < fn.params_fix_.size(); i++) {
            if (i != 0) res += ", ";
            res += fn.params_fix_[i] ? fn.params_fix_[i]->name_ : "any";
        }

        if (!fn.params_fix_.empty() && fn.param_inf_) res += ", ";
        if (fn.param_inf_) {
            res += *fn.param_inf_ ? (*fn.param_inf_)->name_ : "any";
            res += "...";
        }

        return res.empty() ? "empty)" : res + ')';
    }

    // Expr

    void Sema::Exec(BlockExpr& node, std::function<void()> OnScopeReady) {
        node.resolved_type_ = TypeTable::Lookup("none");

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
        if (sym->type_ == SymbolType::Fn) {
            node.resolved_type_ = ((FnSymbol*)sym)->ret_type_;
            return;
        }

        throw LogErr(LogModule::Sema, std::format(
            "'{}' is not a identifier", sym->name_
        ), node.loc_);
    }

    void Sema::Exec(TypeExpr& node) {
        auto base = TypeTable::Lookup(node.base_);

        if (!node.params_) node.resolved_type_ = base;
        else {
            std::vector<const Type*> params = {};
            for (auto& e : node.params_->exprs_) {
                if      (e->type_ == AstType::TypeExpr) {
                    Exec(*e);
                    params.emplace_back(e->resolved_type_);
                }
                else if (e->type_ == AstType::IdExpr) {
                    params.emplace_back(TypeTable::Lookup(((IdExpr&)*e).value_));
                }
                else {
                    throw LogErr(LogModule::Sema, "invalid type parameter", e->loc_);
                }
            }
            
            if (params.empty())
                node.resolved_type_ = base;
            else
                node.resolved_type_ = TypeTable::SetOrGetTypeParam(base, params);
        }
    }

    void Sema::Exec(DeclExpr& node) {
        Exec(*node.bind_type_);
        node.resolved_type_ = node.bind_type_->resolved_type_;

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
                node.resolved_type_ = TypeTable::Lookup("bool");
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
                    node.resolved_type_ = TypeTable::Lookup("bool");
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
                    node.lexpr_->resolved_type_->name_,
                    node.rexpr_->resolved_type_->name_
                ), node.loc_);
            }

            return;
        }
    }

    void Sema::Exec(RangeExpr& node) {
        node.resolved_type_ = TypeTable::Lookup("range");

        Exec(*node.lexpr_);
        Exec(*node.rexpr_);
        if (node.step_) Exec(*node.step_);
    }

    void Sema::Exec(ArrayExpr& node) {
        auto& exprs = node.elements_->exprs_;
        for (auto& e : exprs) Exec(*e);
        if (exprs.empty())
            node.elem_type_ = nullptr;
        else
            node.elem_type_ = exprs[0]->resolved_type_;

        // Empty ArrayExpr
        std::vector<const sema::Type*> params = {};
        if (node.elem_type_) params.emplace_back(node.elem_type_);

        node.resolved_type_ = TypeTable::SetOrGetTypeParam(
            TypeTable::Lookup("array"), params
        );
    }

    void Sema::Exec(FnCallExpr& node) {
        auto sym = symbol_table_.Lookup(node.callee_->value_, node.loc_);
        if (sym->type_ != SymbolType::Fn) {
            throw LogErr(LogModule::Sema, std::format(
                "'{}' is not a function", sym->name_
            ), node.loc_);
        }

        std::vector<const Type*> args_type;
        for (auto& e : node.args_->exprs_) {
            Exec(*e);
            args_type.emplace_back(e->resolved_type_);
        }

        auto fn = (const FnSymbol*)sym;
        FnCallCheck(*fn, args_type, node.loc_);
        node.resolved_type_ = fn->ret_type_;
    }

    void Sema::Exec(MethodCallExpr& node) {
        Exec(*node.target_);
        auto target_type = node.target_->resolved_type_;
        if (!target_type) {
            throw LogErr(LogModule::Sema, std::format(
                "cannot resolve type of method target for '{}'",
                node.callee_->value_
            ), node.loc_);
        }

        std::vector<const Type*> args_type;
        for (auto& e : node.args_->exprs_) {
            Exec(*e);
            args_type.emplace_back(e->resolved_type_);
        }

        auto fn = MethodTable::Lookup(target_type->base(), node.callee_->value_);
        if (!fn) {
            throw LogErr(LogModule::Sema, std::format(
                "no method '{}' on type '{}'",
                node.callee_->value_, target_type->name_
            ), node.loc_);
        }

        FnCallCheck(*fn, args_type, node.loc_);
        node.resolved_type_ = fn->ret_type_;
    }

    void Sema::Exec(FnExpr& node) {
        node.resolved_type_     = TypeTable::Lookup("function");

        // Return Type
        Exec(*node.ret_type_);
        node.ret_resolved_type_ = node.ret_type_->resolved_type_;

        // Params
        auto& params_expr = node.params_->exprs_;
        std::vector<const Type*> params_type;
        for (auto& e : params_expr) {
            auto expr = (DeclExpr*)e.get();
            Exec(*expr->bind_type_);
            expr->resolved_type_ = expr->bind_type_->resolved_type_;
            params_type.emplace_back(expr->resolved_type_);
        }

        // Symbol
        if (!node.name_.empty()) {
            symbol_table_.Declare(std::make_unique<FnSymbol>(
                node.name_, node.loc_,
                node.ret_resolved_type_, params_type
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
                    node.resolved_type_ = sema::TypeTable::Lookup("i32");
                    node.resolved_value_.integer = x;
                    return;
                }
            }

            // i64
            {
                int64_t x = 0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) {
                    node.resolved_type_ = sema::TypeTable::Lookup("i64");
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
                    node.resolved_type_ = sema::TypeTable::Lookup("f32");
                    node.resolved_value_.floating = x;
                    return;
                }
            }

            // f64
            {
                double x = 0.0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) {
                    node.resolved_type_ = sema::TypeTable::Lookup("f64");
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
        node.resolved_type_ = TypeTable::Lookup("bool");
    }

    void Sema::Exec(CharConst& node) {
        node.resolved_type_ = TypeTable::Lookup("char");
    }

    void Sema::Exec(StringConst& node) {
        node.resolved_type_ = TypeTable::Lookup("string");
    }

    // Stmt

    void Sema::Exec(ExprStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        Exec(*node.expr_);
    }

    void Sema::Exec(AssignStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        Exec(*node.target_);
        Exec(*node.value_);
    }

    void Sema::Exec(CondStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        if (node.cond_) {
            Exec(*node.cond_);
            if (node.cond_->resolved_type_ && !node.cond_->resolved_type_->is("bool")) {
                throw LogErr(LogModule::Sema, std::format(
                    "condition must be bool, not {}",
                    node.cond_->resolved_type_->name_
                ), node.loc_);
            }
        }

        Exec(*node.block_);
        if (node.sub_) Exec(*node.sub_);
    }

    void Sema::Exec(ReturnSignalStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        if (node.value_) Exec(*node.value_);
    }

    void Sema::Exec(ForStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");
        return;
    }

    void Sema::Exec(WhileStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        if (node.cond_) {
            Exec(*node.cond_);
            if (node.cond_->resolved_type_ && !node.cond_->resolved_type_->is("bool")) {
                throw LogErr(LogModule::Sema, std::format(
                    "condition must be bool, not {}",
                    node.cond_->resolved_type_->name_
                ), node.loc_);
            }
        }
        Exec(*node.block_);
    }

    // Common

    void Sema::Exec(Program& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        Exec((BlockExpr&)node, [&]() {
            BuiltinFnRegister();        // Global Builtin Functions
        });
    }
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/analyzer.hpp"

namespace sema {

    // Builtin-Fn

    void Analyzer::BuiltinFnRegister() {
        
        auto none_ = TypeTable::Lookup("none");

        // IO
        {
            fn_table_.Add("print",   FnSign(none_, {}, nullptr));
            fn_table_.Add("println", FnSign(none_, {}, nullptr));
        }
    }

    // Expr

    void Analyzer::Exec(BlockExpr& node, std::function<void()> OnScopeReady) {
        node.resolved_type_ = TypeTable::Lookup("none");

        var_table_.ScopePush();
        if (OnScopeReady) OnScopeReady();

        try {
            for (auto& child : node.children_) Exec(*child);
        }
        catch (...) {
            var_table_.ScopePop();
            throw;
        }
        
        var_table_.ScopePop();
    }

    void Analyzer::Exec(IdExpr& node) {
        if (auto var = var_table_.LookupTry(node.name_)) {
            node.resolved_type_ = var->type_;
            return;
        }

        throw LogErr(LogModule::Sema, std::format(
            "undefined identifier '{}'", node.name_
        ), node.loc_);
    }

    void Analyzer::Exec(TypeExpr& node) {
        auto type_basic = TypeTable::Lookup(node.type_basic_);

        // Basic
        if (!node.params_) {
            node.resolved_type_ = type_basic;
        }

        // Parametric
        else {
            std::vector<Type*> params_type = {};
            for (auto& e : node.params_->exprs_) {
                if      (auto typeexpr = dynamic_cast<TypeExpr*>(e.get())) {
                    Exec(*typeexpr);
                    params_type.emplace_back(typeexpr->resolved_type_);
                }
                else if (auto idexpr = dynamic_cast<IdExpr*>(e.get())) {
                    params_type.emplace_back(TypeTable::Lookup(idexpr->name_));
                }
                else {
                    throw LogErr(LogModule::Sema, std::format(
                        "invalid type parameter '{}'", e->TypeName()
                    ), e->loc_);
                }
            }
            
            if (params_type.empty())
                node.resolved_type_ = type_basic;
            else
                node.resolved_type_ = TypeTable::ParametricTypeGet(type_basic, params_type);
        }
    }

    void Analyzer::Exec(DeclExpr& node) {
        Exec(*node.bind_type_);
        node.resolved_type_ = node.bind_type_->resolved_type_;

        var_table_.Declare(std::make_unique<Var>(
            node.id_, node.resolved_type_, node.loc_
        ));
        if (node.value_) Exec(*node.value_);
    }

    void Analyzer::Exec(OperExpr& node) {
        using enum OperType;

        // Unary
        Exec(*node.lexpr_);
        {
            if (node.oper_type_ == Neg) {
                node.resolved_type_ = node.lexpr_->resolved_type_;
                return;
            }
            if (node.oper_type_ == Not) {
                node.resolved_type_ = TypeTable::Lookup("bool");
                return;
            }
        }

        // Binary
        Exec(*node.rexpr_);
        {
            // Pick
            if (node.oper_type_ == Pick) {
                return;
            }

            // Boolean
            switch (node.oper_type_) {
                case Gt:
                case Lt:
                case Ge:
                case Le:
                case Eq:
                case Neq:
                case And:
                case Or:
                    node.resolved_type_ = TypeTable::Lookup("bool");
                    return;

                default: break;
            }

            // Arith
            node.resolved_type_ = TypeTable::Common({
                node.lexpr_->resolved_type_,
                node.rexpr_->resolved_type_
            });

            if (!node.resolved_type_) {
                throw LogErr(LogModule::Sema, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    node.lexpr_->resolved_type_->name(),
                    node.rexpr_->resolved_type_->name()
                ), node.loc_);
            }
        }
    }

    void Analyzer::Exec(RangeExpr& node) {

        // Boundary
        Exec(*node.lexpr_);
        Exec(*node.rexpr_);
        auto boundary_type = TypeTable::Common({
            node.lexpr_->resolved_type_, node.rexpr_->resolved_type_
        });
        if (!boundary_type) {
            throw LogErr(LogModule::Sema, "'left bound type of range' must be compatible with 'right bound type of range'", node.loc_);
        }

        // Step
        auto step_type = boundary_type;
        if (node.step_) {
            Exec(*node.step_);
            step_type = node.step_->resolved_type_;
            if (TypeTable::Common({ step_type, boundary_type }) != boundary_type) {
                throw LogErr(LogModule::Sema, "'step type of range' must be compatible with 'boundary type of range'", node.loc_);
            }
        }

        node.iter_type_ = boundary_type;
        node.resolved_type_ = TypeTable::ParametricTypeGet(
            TypeTable::Lookup("range"), { node.iter_type_ }, node.loc_
        );
    }

    void Analyzer::Exec(ArrayExpr& node) {
        auto& exprs = node.elems_->exprs_;
        for (auto& e : exprs) Exec(*e);
        if (exprs.empty())
            node.elem_type_ = nullptr;
        else
            node.elem_type_ = exprs[0]->resolved_type_;

        // Empty ArrayExpr
        std::vector<Type*> params = {};
        if (node.elem_type_) params.emplace_back(node.elem_type_);

        node.resolved_type_ = TypeTable::ParametricTypeGet(
            TypeTable::Lookup("array"), params
        );
    }

    void Analyzer::Exec(FnCallExpr& node) {

        // Args Type
        std::vector<Type*> args_type = {};
        for (auto& e : node.args_->exprs_) {
            Exec(*e);
            args_type.emplace_back(e->resolved_type_);
        }

        // Callee
        auto callee = node.callee_->name_;
        {
            // Stored in FnTable
            if (auto fn = fn_table_.LookupTry(callee)) {
                auto sign = fn->SignLookup(args_type);
                node.resolved_type_ = sign->ret_type();
                node.callee_fnsign_ = sign;
                return;
            }

            // Stored in VarTable
            /*if (auto var = var_table_.LookupTry(callee)) {
                node.resolved_type_ = TypeTable::Lookup("none");
                return;
            }*/
        }

        throw LogErr(LogModule::Sema, std::format(
            "undefined function '{}'", callee
        ), node.loc_);
    }

    void Analyzer::Exec(MethodCallExpr& node) {
        
        // Target
        Exec(*node.target_);
        auto target_type = node.target_->resolved_type_;

        // Args Type
        std::vector<Type*> args_type = { target_type };     // the first parameter is target
        for (auto& e : node.args_->exprs_) {
            Exec(*e);
            args_type.emplace_back(e->resolved_type_);
        }

        // Callee
        auto  callee = node.callee_->name_;
        auto& method = ((BasicType*)target_type->BasicTypeGet())->method_table().Lookup(callee);
        if (auto sign = method.SignLookup(args_type)) {
            node.resolved_type_ = sign->ret_type();
            node.callee_fnsign_ = sign;
        }
    }

    void Analyzer::Exec(FnExpr& node) {
        node.resolved_type_ = TypeTable::Lookup("function");

        // Return Type
        if (node.return_type_) {
            Exec(*node.return_type_);
            node.ret_resolved_type_ = node.return_type_->resolved_type_;
        }
        else {
            node.ret_resolved_type_ = TypeTable::Lookup("none");
        }

        // Params Type
        auto& params_expr = node.params_->exprs_;
        std::vector<Type*> params_type = {};
        for (auto& e : params_expr) {
            auto expr = (DeclExpr*)e.get();
            Exec(*expr->bind_type_);
            expr->resolved_type_ = expr->bind_type_->resolved_type_;
            params_type.emplace_back(expr->resolved_type_);
        }

        // Stored in FnTable
        if (!node.name_.empty()) {
            node.fnsign_ = fn_table_.Add(
                node.name_, FnSign(node.ret_resolved_type_, params_type)
            );
        }
        
        // Stored in VarTable
        if (node.body_) {
            Exec(*node.body_, [&]() {
                // Args
                for (size_t i = 0; i < params_expr.size(); i++) {
                    auto expr = (DeclExpr*)(params_expr[i].get());
                    var_table_.Declare(std::make_unique<Var>(
                        expr->id_, params_type[i], expr->loc_)
                    );
                }
            });
        }
    }

    // Const

    void Analyzer::Exec(NumConst& node) {
        const auto& numstr = node.value_;

        // Integer
        if (!numstr.contains(".")) {
            
            // i32
            {
                int32_t x = 0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) {
                    node.resolved_type_ = TypeTable::Lookup("i32");
                    node.resolved_value_.integer_ = x;
                    return;
                }
            }

            // i64
            {
                int64_t x = 0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) {
                    node.resolved_type_ = TypeTable::Lookup("i64");
                    node.resolved_value_.integer_ = x;
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
                    node.resolved_type_ = TypeTable::Lookup("f32");
                    node.resolved_value_.floating_ = x;
                    return;
                }
            }

            // f64
            {
                double x = 0.0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) {
                    node.resolved_type_ = TypeTable::Lookup("f64");
                    node.resolved_value_.floating_ = x;
                    return;
                }
            }
        }

        throw LogErr(LogModule::Sema, std::format(
            "numeric overflow '{}'", numstr
        ), node.loc_);
    }

    void Analyzer::Exec(BoolConst& node) {
        node.resolved_type_ = TypeTable::Lookup("bool");
    }

    void Analyzer::Exec(CharConst& node) {
        node.resolved_type_ = TypeTable::Lookup("char");
    }

    void Analyzer::Exec(StringConst& node) {
        node.resolved_type_ = TypeTable::Lookup("string");
    }

    // Stmt

    void Analyzer::Exec(ExprStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        Exec(*node.expr_);
    }

    void Analyzer::Exec(AssignStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        Exec(*node.target_);
        Exec(*node.value_);
    }

    void Analyzer::Exec(CondStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        if (node.cond_) {
            Exec(*node.cond_);
            if (node.cond_->resolved_type_ && !node.cond_->resolved_type_->is("bool")) {
                throw LogErr(LogModule::Sema, std::format(
                    "'condition' must be 'bool', not '{}'",
                    node.cond_->resolved_type_->name()
                ), node.loc_);
            }
        }

        Exec(*node.then_);
        if (node.next_) Exec(*node.next_);
    }

    void Analyzer::Exec(ReturnSignalStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        if (node.value_) Exec(*node.value_);
    }

    void Analyzer::Exec(ForStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");
        
        Exec(*node.data_);
        auto data_type = node.data_->resolved_type_;
        Type* iter_type = nullptr;
        
        if (data_type->is("array")) {
            if (auto parametric_type = dynamic_cast<ParametricType*>(data_type)) {
                auto params_type = parametric_type->params_type();
                if (!params_type.empty()) iter_type = params_type[0];
            }
        }
        if (data_type->is("string") || data_type->is("stringview")) {
            iter_type = TypeTable::Lookup("char");
        }
        if (data_type->is("range")) {
            if (auto parametric_type = dynamic_cast<ParametricType*>(data_type)) {
                auto params_type = parametric_type->params_type();
                if (!params_type.empty()) iter_type = params_type[0];
            }
        }

        Exec(*node.body_, [&]() {
            var_table_.Declare(std::make_unique<Var>(
                node.iter_->name_, iter_type, node.iter_->loc_)
            );
        });
    }

    void Analyzer::Exec(WhileStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        if (node.cond_) {
            Exec(*node.cond_);
            if (node.cond_->resolved_type_ && !node.cond_->resolved_type_->is("bool")) {
                throw LogErr(LogModule::Sema, std::format(
                    "'condition' must be 'bool', not '{}'",
                    node.cond_->resolved_type_->name()
                ), node.loc_);
            }
        }
        Exec(*node.body_);
    }

    // Common

    void Analyzer::Exec(Program& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        Exec((BlockExpr&)node, [&]() {
            BuiltinFnRegister();
        });
    }
}

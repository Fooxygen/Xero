
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/analyzer.hpp"

namespace sema {

    // Expr

    void Analyzer::Exec(BlockExpr& node, const std::function<void()>& on_scope_ready) {
        node.resolved_type_ = TypeTable::Lookup("none");

        var_table_.ScopePush();
        if (on_scope_ready) on_scope_ready();

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

    void Analyzer::Exec(RefExpr& node) {
        if (!dynamic_cast<IdExpr*>(node.target_.get())) {
            throw LogErr(LogModule::Sema, "cannot reference a non-referenceable value", node.loc_);
        }

        Exec(*node.target_);
        node.resolved_type_ = TypeTable::ReferenceTypeGet(node.target_->resolved_type_->ReferenceUnwrap());
    }

    void Analyzer::Exec(TypeExpr& node) {
        auto  basic_type = TypeTable::Lookup(node.basic_type_, node.loc_);
        Type* resolved   = nullptr;

        // BasicType
        if (!node.params_) {
            resolved = basic_type;
        }

        // ParametricType
        else {
            std::vector<Type*> params = {};
            for (auto& e : node.params_->exprs_) {
                if      (auto typeexpr = dynamic_cast<TypeExpr*>(e.get())) {
                    Exec(*typeexpr);
                    params.emplace_back(typeexpr->resolved_type_);
                }
                else if (auto idexpr   = dynamic_cast<IdExpr*>(e.get())) {
                    params.emplace_back(TypeTable::Lookup(idexpr->name_, idexpr->loc_));
                }
                else {
                    throw LogErr(LogModule::Sema, std::format(
                        "invalid type parameter '{}'", e->TypeName()
                    ), e->loc_);
                }
            }
            
            if (params.empty())
                resolved = basic_type;
            else
                resolved = TypeTable::ParametricTypeGet(basic_type, params, node.loc_);
        }

        // ReferenceType
        if (node.is_referred_) {
            resolved = TypeTable::ReferenceTypeGet(resolved);
        }

        node.resolved_type_ = resolved;
    }

    void Analyzer::Exec(DeclExpr& node) {
        Exec(*node.bind_type_);

        // ReferenceType
        if (node.bind_type_->is_referred_) {
            
            // x: i32&;
            if (!node.value_) {
                throw LogErr(LogModule::Sema, "reference type must be initialized with a value", node.loc_);
            }

            // x: i32& = 3;
            if (!dynamic_cast<IdExpr*>(node.value_.get())) {
                throw LogErr(LogModule::Sema, std::format(
                    "cannot assign non-reference value to reference type '{}'",
                    node.bind_type_->resolved_type_->name()
                ), node.value_->loc_);
            }
        }
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
                auto ltype  = node.lexpr_->resolved_type_->ReferenceUnwrap();
                auto method = TypeTable::MethodLookup(ltype, "@neg");
                auto sign   = method->SignLookup(ltype, {});
                node.resolved_type_ = sign->return_type();
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
                auto caller_type = node.lexpr_->resolved_type_;
                auto idx_type    = node.rexpr_->resolved_type_->ReferenceUnwrap();

                auto method = TypeTable::MethodLookup(caller_type, "@pick");
                auto sign   = method->SignLookup(caller_type, { idx_type }, node.loc_);
                node.resolved_type_ = sign->return_type();
                
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
            node.resolved_type_ = TypeTable::CommonTypeGet({
                node.lexpr_->resolved_type_->ReferenceUnwrap(),
                node.rexpr_->resolved_type_->ReferenceUnwrap()
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
        auto boundary_type = TypeTable::CommonTypeGet({
            node.lexpr_->resolved_type_->ReferenceUnwrap(),
            node.rexpr_->resolved_type_->ReferenceUnwrap()
        });
        if (!boundary_type) {
            throw LogErr(LogModule::Sema, "'left bound type of range' must be compatible with 'right bound type of range'", node.loc_);
        }

        // Step
        auto step_type = boundary_type;
        if (node.step_) {
            Exec(*node.step_);
            step_type = node.step_->resolved_type_->ReferenceUnwrap();
            if (TypeTable::CommonTypeGet({ step_type, boundary_type }) != boundary_type) {
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
        
        // Elem Type
        node.elem_type_ = nullptr;
        for (size_t i = 0; i < exprs.size(); i++) {
            Exec(*exprs[i]);

            if (i == 0) {
                node.elem_type_ = exprs[i]->resolved_type_->ReferenceUnwrap();
            }
            else if (exprs[i]->resolved_type_->ReferenceUnwrap() != node.elem_type_) {
                throw LogErr(LogModule::Sema, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    exprs[i]->resolved_type_->name(),
                    node.elem_type_->name()
                ), exprs[i]->loc_);
            }
        }

        // XXX: Empty ArrayExpr
        std::vector<Type*> params = {};
        if (node.elem_type_) params.emplace_back(node.elem_type_);

        node.resolved_type_ = TypeTable::ParametricTypeGet(
            TypeTable::Lookup("array", node.loc_), params
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
                auto sign = fn->SignLookup(args_type, node.callee_->loc_);
                node.resolved_type_ = sign->return_type();
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
        
        // Args Type
        std::vector<Type*> args_type = {};
        for (auto& e : node.args_->exprs_) {
            Exec(*e);
            args_type.emplace_back(e->resolved_type_);
        }

        // Caller
        Exec(*node.caller_);
        auto caller_type = node.caller_->resolved_type_;

        // Callee
        auto  callee = node.callee_->name_;
        auto  method = TypeTable::MethodLookup(caller_type, callee);
        auto  sign   = method->SignLookup(caller_type, args_type, node.callee_->loc_);
        
        node.resolved_type_ = sign->return_type();
        node.callee_fnsign_ = sign;
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
        std::vector<Type*> params_type = {};
        auto& params_expr = node.params_->exprs_;
        for (auto& e : params_expr) {

            if (e->type_ != AstType::DeclExpr) {
                throw LogErr(LogModule::Sema, std::format(
                    "parameter of function must be a declaration, not '{}'",
                    e->TypeName()
                ), e->loc_);
            }

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
            if (node.cond_->resolved_type_ && !node.cond_->resolved_type_->Is("bool")) {
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

        Type* iter_type = nullptr;
        auto  data_type = node.data_->resolved_type_->ReferenceUnwrap();

        if (data_type->Is("array") || data_type->Is("arrayview")) {
            if (auto parametric_type = dynamic_cast<ParametricType*>(data_type)) {
                auto params = parametric_type->params();
                if (!params.empty()) iter_type = params[0];
            }
        }
        if (data_type->Is("string") || data_type->Is("stringview")) {
            iter_type = TypeTable::Lookup("char");
        }
        if (data_type->Is("range")) {
            if (auto parametric_type = dynamic_cast<ParametricType*>(data_type)) {
                auto params = parametric_type->params();
                if (!params.empty()) iter_type = params[0];
            }
        }

        if (!iter_type) {
            throw LogErr(LogModule::Sema, std::format(
                "'iterated value type' must be iterable, not '{}'",
                data_type->name()
            ), node.data_->loc_);
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
            if (node.cond_->resolved_type_ && !node.cond_->resolved_type_->Is("bool")) {
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

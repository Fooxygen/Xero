
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/sema.hpp"
#include "sema/type.hpp"

namespace sema {

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
        auto sym = symbol_table_.Lookup(node.name_, node.loc_);
        if (sym->type_ == SymbolType::Var) {
            node.resolved_type_ = ((VarSymbol*)sym)->ret_type_;
            return;
        }
        if (sym->type_ == SymbolType::Fn) {
            node.resolved_type_ = ((FnSymbol*)sym)->sign_->ret_type_;
            return;
        }

        throw LogErr(LogModule::Sema, std::format(
            "undefined identifier '{}'", sym->name_
        ), node.loc_);
    }

    void Sema::Exec(TypeExpr& node) {
        auto base = TypeTable::Lookup(node.base_type_);

        // base
        if (!node.params_) {
            node.resolved_type_ = base;
        }

        // base[=param0, param1, ...=]
        else {
            std::vector<const Type*> params_type = {};
            for (auto& e : node.params_->exprs_) {
                if      (e->type_ == AstType::TypeExpr) {
                    Exec(*e);
                    params_type.emplace_back(e->resolved_type_);
                }
                else if (e->type_ == AstType::IdExpr) {
                    params_type.emplace_back(TypeTable::Lookup(((IdExpr&)*e).name_));
                }
                else {
                    throw LogErr(LogModule::Sema, std::format(
                        "invalid type parameter '{}'", e->TypeName()
                    ), e->loc_);
                }
            }
            
            if (params_type.empty())
                node.resolved_type_ = base;
            else
                node.resolved_type_ = TypeTable::ParamTypeGet(base, params_type);
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
            // TODO: Consider overloading–boolean constraint conflict
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
            node.resolved_type_ = sema::TypeTable::Common({
                node.lexpr_->resolved_type_,
                node.rexpr_->resolved_type_
            });
            if (!node.resolved_type_) {
                throw LogErr(LogModule::Sema, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    node.lexpr_->resolved_type_->name_,
                    node.rexpr_->resolved_type_->name_
                ), node.loc_);
            }
        }
    }

    void Sema::Exec(RangeExpr& node) {

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
        node.resolved_type_ = TypeTable::ParamTypeGet(
            TypeTable::Lookup("range"), { node.iter_type_ }, node.loc_
        );
    }

    void Sema::Exec(ArrayExpr& node) {
        auto& exprs = node.elems_->exprs_;
        for (auto& e : exprs) Exec(*e);
        if (exprs.empty())
            node.elem_type_ = nullptr;
        else
            node.elem_type_ = exprs[0]->resolved_type_;

        // Empty ArrayExpr
        std::vector<const sema::Type*> params = {};
        if (node.elem_type_) params.emplace_back(node.elem_type_);

        node.resolved_type_ = TypeTable::ParamTypeGet(
            TypeTable::Lookup("array"), params
        );
    }

    void Sema::Exec(FnCallExpr& node) {

        // Args Type
        std::vector<const Type*> args_type = {};
        for (auto& e : node.args_->exprs_) {
            Exec(*e);
            args_type.emplace_back(e->resolved_type_);
        }

        // Callee
        auto callee = node.callee_->name_;
        auto sym    = symbol_table_.Lookup(callee, node.loc_);

        // Stored in Variable
        if (sym->type_ == SymbolType::Var) {
            node.resolved_type_ = TypeTable::Lookup("none");
        }

        else {
            auto& fnsign = ((const FnSymbol*)sym)->sign_;
            if(!fnsign->CallCheck(args_type)) {
                throw LogErr(LogModule::Sema, std::format(
                    "function '{}' expects {} type parameter(s), got {}",
                    callee,
                    fnsign->ParamsPrint(),
                    FnSign(nullptr, args_type, std::nullopt).ParamsPrint()
                ), node.loc_);
            }
            node.resolved_type_ = fnsign->ret_type_;
        }
    }

    void Sema::Exec(MethodCallExpr& node) {
        
        // Target
        Exec(*node.target_);
        auto target_type = node.target_->resolved_type_;

        // Args Type
        std::vector<const Type*> args_type = {};
        for (auto& e : node.args_->exprs_) {
            Exec(*e);
            args_type.emplace_back(e->resolved_type_);
        }

        // Callee
        auto  callee  = node.callee_->name_;
        auto& methods = ((const BasicType*)target_type->BasicTypeGet())->methods_;
        auto  method  = methods.find(callee);
        if (method == methods.end()) {
            throw LogErr(LogModule::Sema, std::format(
                "undefined method '{}' on type '{}'",
                callee, target_type->name_
            ), node.loc_);
        }
        
        // Overload Selection
        bool isOverloadFind = false;
        for (auto& fnsign : method->second.fnsigns_) {
            if (fnsign->CallCheck(args_type)) {
                isOverloadFind = true;
                node.resolved_type_ = fnsign->ret_type_;
            }
        }
        if (!isOverloadFind) {
            throw LogErr(LogModule::Sema, std::format(
                "undefined overload of method '{}' on type '{}'",
                callee, target_type->name_
            ), node.loc_);
        }
    }

    void Sema::Exec(FnExpr& node) {
        node.resolved_type_ = TypeTable::Lookup("function");

        // Return Type
        if (node.ret_type_) {
            Exec(*node.ret_type_);
            node.ret_resolved_type_ = node.ret_type_->resolved_type_;
        }
        else {
            node.ret_resolved_type_ = TypeTable::Lookup("none");
        }

        // Params
        auto& params_expr = node.params_->exprs_;
        std::vector<const Type*> params_type = {};
        for (auto& e : params_expr) {
            auto expr = (DeclExpr*)e.get();
            Exec(*expr->bind_type_);
            expr->resolved_type_ = expr->bind_type_->resolved_type_;
            params_type.emplace_back(expr->resolved_type_);
        }

        // FnSymbol
        if (!node.name_.empty()) {
            symbol_table_.Declare(std::make_unique<FnSymbol>(
                node.name_, node.loc_,
                FnSign(node.ret_resolved_type_, params_type)
            ));
        }
        
        // VarSymbol for Args
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
                    node.resolved_value_.integer_ = x;
                    return;
                }
            }

            // i64
            {
                int64_t x = 0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) {
                    node.resolved_type_ = sema::TypeTable::Lookup("i64");
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
                    node.resolved_type_ = sema::TypeTable::Lookup("f32");
                    node.resolved_value_.floating_ = x;
                    return;
                }
            }

            // f64
            {
                double x = 0.0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) {
                    node.resolved_type_ = sema::TypeTable::Lookup("f64");
                    node.resolved_value_.floating_ = x;
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
                    "'condition' must be 'bool', not '{}'",
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
        
        Exec(*node.data_);
        auto data_type = node.data_->resolved_type_;
        const sema::Type* iter_type = nullptr;
        
        if (data_type->is("array")) {
            if (data_type->type_using_ == Type::Using::Param) {
                auto params_type = ((ParametricType*)data_type)->params_type();
                if (!params_type.empty()) iter_type = params_type[0];
            }
        }
        if (data_type->is("string") || data_type->is("stringview")) {
            iter_type = TypeTable::Lookup("char");
        }
        if (data_type->is("range")) {
            if (data_type->type_using_ == Type::Using::Param) {
                auto params_type = ((ParametricType*)data_type)->params_type();
                if (!params_type.empty()) iter_type = params_type[0];
            }
        }

        Exec(*node.block_, [&]() {
            symbol_table_.Declare(std::make_unique<VarSymbol>(
                node.iter_->name_, node.iter_->loc_, iter_type)
            );
        });
    }

    void Sema::Exec(WhileStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        if (node.cond_) {
            Exec(*node.cond_);
            if (node.cond_->resolved_type_ && !node.cond_->resolved_type_->is("bool")) {
                throw LogErr(LogModule::Sema, std::format(
                    "'condition' must be 'bool', not '{}'",
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

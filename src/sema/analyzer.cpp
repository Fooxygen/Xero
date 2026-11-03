
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/analyzer.hpp"

namespace sema {

    // Declare

    // └─ Expr

    void Analyzer::Declare(FnExpr& node) {
        node.resolved_type_ = TypeTable::Lookup("function");
        
        // Return Type
        if (node.return_type_) {
            Process(*node.return_type_);
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
                throw LogErr(LogStage::Sema, std::format(
                    "parameter of function must be a declaration, not '{}'",
                    e->TypeName()
                ), e->loc_);
            }

            auto expr = (DeclExpr*)e.get();
            Process(*expr->bind_type_);
            expr->resolved_type_ = expr->bind_type_->resolved_type_;
            params_type.emplace_back(expr->resolved_type_);
        }

        // Signature
        node.fnsign_ = fn_scope_.Add(
            node.name_, FnSign(node.ret_resolved_type_, params_type)
        );
    }

    void Analyzer::Declare(StructExpr& node) {

        // Path
        Path full_path = module_->namespaces();
        full_path.Add(node.name_);
        
        if (TypeTable::LookupTry(full_path.Print())) {
            throw LogErr(LogStage::Sema, std::format(
                "redefinition of type '{}'", full_path.Print()
            ), node.loc_);
        }

        // Type
        auto  type      = std::make_unique<StructType>(full_path.Print());
        auto  type_raw  = type.get();
        auto* none_type = TypeTable::Lookup("none");
        {
            type_raw->method_table().Add("@copy",    FnSign(type_raw));
            type_raw->method_table().Add("@release", FnSign(none_type));

            TypeTable::Set(std::move(type));
            type_scope_.Declare(node.name_, type_raw);
        }

        node.resolved_type_ = type_raw;
    }

    // └─ Common

    void Analyzer::Declare(Program& node) {
        auto& node_children = ((BlockExpr&)node).children_;

        // Struct names first, so field types and signatures can refer to each other
        for (auto& child : node_children) {
            if (child->type_ == AstType::StructExpr) Declare((StructExpr&)*child);
        }

        // Other global declarations
        for (auto& child : node_children) {
            if (child->type_ != AstType::StructExpr) Declare(*child);
        }
    }

    // Process

    // └─ Expr

    void Analyzer::Process(BlockExpr& node, const std::function<void()>& on_scope_ready) {
        node.resolved_type_ = TypeTable::Lookup("none");

        var_scope_.Push();
        if (on_scope_ready) on_scope_ready();

        try {
            for (auto& child : node.children_) Process(*child);
        }
        catch (...) {
            var_scope_.Pop();
            throw;
        }
        
        var_scope_.Pop();
    }

    void Analyzer::Process(StructExpr& node) {
        auto* struct_type = (StructType*)node.resolved_type_;
        if (!struct_type) {
            throw LogErr(LogStage::Sema,
                "struct declaration must be at module level",
                node.loc_
            );
        }

        for (auto& child : node.body_->children_) {
            auto* decl = dynamic_cast<DeclExpr*>(child.get());
            if (!decl) {
                if (auto* stmt = dynamic_cast<ExprStmt*>(child.get())) {
                    decl = dynamic_cast<DeclExpr*>(stmt->expr_.get());
                }
            }
            if (!decl || !decl->bind_type_) {
                throw LogErr(LogStage::Sema, std::format(
                    "member of struct must be a field declaration, not '{}'",
                    child->TypeName()
                ), child->loc_);
            }

            Process(*decl->bind_type_);
            auto* field_type  = decl->bind_type_->resolved_type_;
            auto  field_index = struct_type->fields().size();

            if (!struct_type->field_indices().emplace(decl->id_, field_index).second) {
                throw LogErr(LogStage::Sema, std::format(
                    "redefinition of field '{}' in type '{}'",
                    decl->id_, struct_type->name()
                ), decl->loc_);
            }
            struct_type->fields().emplace_back(Field{decl->id_, field_type, field_index});
        }

        // Cycle Definition Check
        std::set<StructType*> visiting = {};
        std::function<void(StructType*)> CycleDefinitionCheck = [&](StructType* type) {
            if (!visiting.insert(type).second) {
                throw LogErr(LogStage::Sema, std::format(
                    "struct '{}' must not contain itself by value", type->name()
                ));
            }

            for (auto& field : type->fields()) {
                if (dynamic_cast<ReferenceType*>(field.type_)) continue;

                if (auto* field_struct_type = dynamic_cast<StructType*>(field.type_->BasicTypeGet())) {
                    CycleDefinitionCheck(field_struct_type);
                }
            }

            visiting.erase(type);
        };
        
        CycleDefinitionCheck(struct_type);
    }

    void Analyzer::Process(IdExpr& node) {
        if (auto var = var_scope_.LookupTry(node.name_)) {
            node.resolved_type_ = var->type_;
            return;
        }

        throw LogErr(LogStage::Sema, std::format(
            "undefined identifier '{}'", node.name_
        ), node.loc_);
    }

    void Analyzer::Process(RefExpr& node) {
        if (!dynamic_cast<IdExpr*>(node.target_.get())) {
            throw LogErr(LogStage::Sema, "cannot reference a non-referenceable value", node.loc_);
        }

        Process(*node.target_);
        node.resolved_type_ = TypeTable::ReferenceTypeGet(node.target_->resolved_type_->ReferenceUnwrap());
    }

    void Analyzer::Process(TypeExpr& node) {
        auto* aliases    = module_ ? &module_->namespaces_alias() : nullptr;
        auto  basic_type = type_scope_.Resolve(node.basic_type_, aliases, node.loc_);
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
                    Process(*typeexpr);
                    params.emplace_back(typeexpr->resolved_type_);
                }
                else if (auto idexpr   = dynamic_cast<IdExpr*>(e.get())) {
                    Path param_path = {};
                    param_path.Add(idexpr->name_);
                    params.emplace_back(type_scope_.Resolve(param_path, aliases, idexpr->loc_));
                }
                else {
                    throw LogErr(LogStage::Sema, std::format(
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

    void Analyzer::Process(DeclExpr& node) {
        Process(*node.bind_type_);

        // ReferenceType
        if (node.bind_type_->is_referred_) {
            
            // x: i32&;
            if (!node.value_) {
                throw LogErr(LogStage::Sema, "reference type must be initialized with a value", node.loc_);
            }

            // x: i32& = 3;
            if (!dynamic_cast<IdExpr*>(node.value_.get())) {
                throw LogErr(LogStage::Sema, std::format(
                    "cannot assign non-reference value to reference type '{}'",
                    node.bind_type_->resolved_type_->name()
                ), node.value_->loc_);
            }
        }
        node.resolved_type_ = node.bind_type_->resolved_type_;

        var_scope_.Add(std::make_unique<Var>(
            node.id_, node.resolved_type_, node.loc_
        ));
        if (node.value_) Process(*node.value_);
    }

    void Analyzer::Process(OperExpr& node) {
        using enum OperType;

        // Unary
        Process(*node.lexpr_);
        {
            if (node.oper_type_ == Neg) {
                auto ltype  = node.lexpr_->resolved_type_->ReferenceUnwrap();
                auto method = TypeTable::MethodLookup(ltype, "@neg", node.loc_);
                auto sign   = method->SignLookup(ltype, {}, node.loc_);
                node.resolved_type_ = sign->return_type();
                return;
            }
            if (node.oper_type_ == Not) {
                node.resolved_type_ = TypeTable::Lookup("bool");
                return;
            }
        }

        // Binary
        Process(*node.rexpr_);
        {
            // Pick
            if (node.oper_type_ == Pick) {
                auto caller_type = node.lexpr_->resolved_type_;
                auto idx_type    = node.rexpr_->resolved_type_->ReferenceUnwrap();

                auto method = TypeTable::MethodLookup(caller_type, "@pick", node.loc_);
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
                throw LogErr(LogStage::Sema, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    node.lexpr_->resolved_type_->name(),
                    node.rexpr_->resolved_type_->name()
                ), node.loc_);
            }
        }
    }

    void Analyzer::Process(RangeExpr& node) {

        // Boundary
        Process(*node.lexpr_);
        Process(*node.rexpr_);
        auto boundary_type = TypeTable::CommonTypeGet({
            node.lexpr_->resolved_type_->ReferenceUnwrap(),
            node.rexpr_->resolved_type_->ReferenceUnwrap()
        });
        if (!boundary_type) {
            throw LogErr(LogStage::Sema, "'left bound type of range' must be compatible with 'right bound type of range'", node.loc_);
        }

        // Step
        auto step_type = boundary_type;
        if (node.step_) {
            Process(*node.step_);
            step_type = node.step_->resolved_type_->ReferenceUnwrap();
            if (TypeTable::CommonTypeGet({ step_type, boundary_type }) != boundary_type) {
                throw LogErr(LogStage::Sema, "'step type of range' must be compatible with 'boundary type of range'", node.loc_);
            }
        }

        node.iter_type_ = boundary_type;
        node.resolved_type_ = TypeTable::ParametricTypeGet(
            TypeTable::Lookup("range"), { node.iter_type_ }, node.loc_
        );
    }

    void Analyzer::Process(ArrayExpr& node) {
        auto& exprs = node.elems_->exprs_;
        
        // Elem Type
        node.elem_type_ = nullptr;
        for (size_t i = 0; i < exprs.size(); i++) {
            Process(*exprs[i]);

            if (i == 0) {
                node.elem_type_ = exprs[i]->resolved_type_->ReferenceUnwrap();
            }
            else if (exprs[i]->resolved_type_->ReferenceUnwrap() != node.elem_type_) {
                throw LogErr(LogStage::Sema, std::format(
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

    void Analyzer::Process(FnCallExpr& node) {

        // Args Type
        std::vector<Type*> args_type = {};
        for (auto& e : node.args_->exprs_) {
            Process(*e);
            args_type.emplace_back(e->resolved_type_);
        }

        // Callee
        Fn* fn = nullptr;
        
        // Call fn
        if      (auto* idexpr   = dynamic_cast<IdExpr*>(node.callee_.get())) {
            fn = fn_scope_.Lookup(idexpr->name_, idexpr->loc_);
        }
        
        // Call a::b::fn
        else if (auto* pathexpr = dynamic_cast<PathExpr*>(node.callee_.get())) {
            Path segments = {};
            for (auto& seg : pathexpr->segments_) {
                segments.Add(seg->name_);
            }

            // Alias: a::b::c::fn -> d::fn
            if (module_) {
                segments = Path::Resolve(segments, module_->namespaces_alias());
            }

            fn = fn_scope_.Lookup(segments, pathexpr->loc_);
        }

        auto sign = fn->SignLookup(args_type, node.callee_->loc_);
        node.resolved_type_ = sign->return_type();
        node.callee_fnsign_ = sign;
    }

    void Analyzer::Process(MethodCallExpr& node) {
        
        // Args Type
        std::vector<Type*> args_type = {};
        for (auto& e : node.args_->exprs_) {
            Process(*e);
            args_type.emplace_back(e->resolved_type_);
        }

        // Caller
        Process(*node.caller_);
        auto caller_type = node.caller_->resolved_type_;

        // Callee
        auto  callee = node.callee_->name_;
        auto  method = TypeTable::MethodLookup(caller_type, callee, node.callee_->loc_);
        auto  sign   = method->SignLookup(caller_type, args_type, node.callee_->loc_);
        
        node.resolved_type_ = sign->return_type();
        node.callee_fnsign_ = sign;
    }

    void Analyzer::Process(FieldAccessExpr& node) {
        Process(*node.base_);
        auto* base_type = node.base_->resolved_type_->ReferenceUnwrap();

        // Struct Type
        if (auto* struct_type = dynamic_cast<StructType*>(base_type)) {
            auto it = struct_type->field_indices().find(node.field_->name_);
            if (it == struct_type->field_indices().end()) {
                throw LogErr(LogStage::Sema, std::format(
                    "undefined field '{}' in type '{}'",
                    node.field_->name_, struct_type->name()
                ), node.field_->loc_);
            }

            node.field_index_   = it->second;
            node.resolved_type_ = TypeTable::ReferenceTypeGet(struct_type->fields()[it->second].type_);
        }

        // Non-Struct Type
        else {
            throw LogErr(LogStage::Sema, std::format(
                "base of field access must be a struct type, not '{}'", base_type->name()
            ), node.loc_);
        }
    }

    void Analyzer::Process(FnExpr& node) {
        if (!node.ret_resolved_type_) Declare(node);
        
        // Parameter Binding
        if (node.body_) {
            auto& params_expr = node.params_->exprs_;

            fn_depth_++;
            try {
                Process(*node.body_, [&]() {
                    for (size_t i = 0; i < params_expr.size(); i++) {
                        auto expr = (DeclExpr*)(params_expr[i].get());
                        var_scope_.Add(std::make_unique<Var>(
                            expr->id_, expr->resolved_type_, expr->loc_)
                        );
                    }
                });
            }
            catch (...) {
                fn_depth_--;
                throw;
            }
            fn_depth_--;
        }
    }

    // └─ Const

    void Analyzer::Process(NumConst& node) {
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
                throw LogErr(LogStage::Sema, std::format(
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

        throw LogErr(LogStage::Sema, std::format(
            "numeric overflow '{}'", numstr
        ), node.loc_);
    }

    void Analyzer::Process(BoolConst& node) {
        node.resolved_type_ = TypeTable::Lookup("bool");
    }

    void Analyzer::Process(CharConst& node) {
        node.resolved_type_ = TypeTable::Lookup("char");
    }

    void Analyzer::Process(StringConst& node) {
        node.resolved_type_ = TypeTable::Lookup("string");
    }

    // └─ Stmt

    void Analyzer::Process(ExprStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        if (node.expr_->type_ == AstType::StructExpr) {
            throw LogErr(LogStage::Sema,
                "unexpected semicolon after struct declaration", node.loc_);
        }

        Process(*node.expr_);
    }

    void Analyzer::Process(AssignStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        Process(*node.target_);
        Process(*node.value_);
    }

    void Analyzer::Process(CondStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        if (node.cond_) {
            Process(*node.cond_);
            if (node.cond_->resolved_type_ && !node.cond_->resolved_type_->Is("bool")) {
                throw LogErr(LogStage::Sema, std::format(
                    "'condition' must be 'bool', not '{}'",
                    node.cond_->resolved_type_->name()
                ), node.loc_);
            }
        }

        Process(*node.then_);
        if (node.next_) Process(*node.next_);
    }

    void Analyzer::Process(ReturnSignalStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        if (node.value_) Process(*node.value_);
    }

    void Analyzer::Process(ForStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");
        Process(*node.data_);

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
            throw LogErr(LogStage::Sema, std::format(
                "'iterated value type' must be iterable, not '{}'",
                data_type->name()
            ), node.data_->loc_);
        }

        Process(*node.body_, [&]() {
            var_scope_.Add(std::make_unique<Var>(
                node.iter_->name_, iter_type, node.iter_->loc_)
            );
        });
    }

    void Analyzer::Process(WhileStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        if (node.cond_) {
            Process(*node.cond_);
            if (node.cond_->resolved_type_ && !node.cond_->resolved_type_->Is("bool")) {
                throw LogErr(LogStage::Sema, std::format(
                    "'condition' must be 'bool', not '{}'",
                    node.cond_->resolved_type_->name()
                ), node.loc_);
            }
        }
        Process(*node.body_);
    }

    // └─ Program

    void Analyzer::Process(Program& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        var_scope_.Push();
        try {
            // Struct
            for (auto& child : node.children_) {
                if (child->type_ == AstType::StructExpr) Process((StructExpr&)*child);
            }

            // Other
            for (auto& child : node.children_) {
                if (child->type_ == AstType::StructExpr) continue;
                Process(*child);
            }
        }
        catch (...) {
            var_scope_.Pop();
            throw;
        }
        var_scope_.Pop();
    }
}

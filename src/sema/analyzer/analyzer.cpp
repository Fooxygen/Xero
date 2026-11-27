
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <limits>

#include "sema/analyzer/analyzer.hpp"

namespace sema {

    // Declare

    // └─ Expr

    void Analyzer::Declare(FnExpr& node, FnScope& fn_scope, bool is_named) {
        node.resolved_type_ = TypeTable::Lookup("function");

        // Name
        if (is_named && node.name_.empty()) {
            throw LogErr(LogStage::Sema, "function must have a name", node.loc_);
        }

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
            auto declexpr = (DeclExpr*)e.get();
            Process(*declexpr->bind_type_);
            declexpr->resolved_type_ = declexpr->bind_type_->resolved_type_;
            params_type.emplace_back(declexpr->resolved_type_);
        }

        // Signature
        node.fnsign_ = fn_scope.Add(
            node.name_, FnSign(node.ret_resolved_type_, params_type)
        );
    }

    void Analyzer::Declare(StructDeclExpr& node) {

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

    void Analyzer::Declare(EnumDeclExpr& node) {

        // Path
        Path full_path = module_->namespaces();
        full_path.Add(node.name_);

        if (TypeTable::LookupTry(full_path.Print())) {
            throw LogErr(LogStage::Sema, std::format(
                "redefinition of type '{}'", full_path.Print()
            ), node.loc_);
        }

        // Type
        auto  type      = std::make_unique<EnumType>(full_path.Print());
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

    void Analyzer::Declare(DeclExpr& node) {

        // Module Constant
        if (node.modifier_ & VarModifier::Const) {
            if (!node.value_) {
                throw LogErr(LogStage::Sema, "constant must have an initializer", node.loc_);
            }

            module_const_scope_.Declare(node.id_, &node);
        }

        // Module Variable
        else {
            throw LogErr(LogStage::Sema, "unsupported module variable", node.loc_);
        }

        Process(*node.bind_type_);
        node.resolved_type_ = node.bind_type_->resolved_type_;
    }

    void Analyzer::Declare(Program& node) {
        auto& node_children = ((BlockExpr&)node).children_;

        // Decl for Struct and Enum
        for (auto& child : node_children) {
            if (child->type_ == AstType::StructDeclExpr) Declare((StructDeclExpr&)*child);
            if (child->type_ == AstType::EnumDeclExpr)   Declare((EnumDeclExpr&)*child);
        }

        // Member Decl for Struct and Enum
        bool has_enum_decl = false;
        for (auto& child : node_children) {
            if      (child->type_ == AstType::StructDeclExpr) {
                auto& struct_decl = (StructDeclExpr&)*child;
                auto* struct_type = (StructType*)struct_decl.resolved_type_;

                // Members
                for (auto& member : struct_decl.body_->children_) {
                    // Method
                    if (member->type_ == AstType::FnExpr) {
                        Declare((FnExpr&)*member, struct_type->method_table());
                    }

                    // Field
                    else {
                        auto& field_stmt = (ExprStmt&)*member;
                        auto& field_decl = (DeclExpr&)*field_stmt.expr_;

                        Process(*field_decl.bind_type_);
                        auto* field_type  = field_decl.bind_type_->resolved_type_;
                        auto  field_index = struct_type->fields().size();

                        if (!struct_type->field_indices().emplace(field_decl.id_, field_index).second) {
                            throw LogErr(LogStage::Sema, std::format(
                                "redefinition of field '{}' in type '{}'",
                                field_decl.id_, struct_type->name()
                            ), field_decl.loc_);
                        }
                        struct_type->fields().emplace_back(StructField{field_decl.id_, field_type, field_index});
                    }
                }
            }
            else if (child->type_ == AstType::EnumDeclExpr) {
                has_enum_decl = true;

                auto& enum_decl = (EnumDeclExpr&)*child;
                auto* enum_type = (EnumType*)enum_decl.resolved_type_;

                auto* none_ = TypeTable::Lookup("none");
                auto* bool_ = TypeTable::Lookup("bool");
                auto* i32_  = TypeTable::Lookup("i32");
                auto* i64_  = TypeTable::Lookup("i64");

                // Members
                for (auto& member : enum_decl.body_->children_) {
                    // Method
                    if (member->type_ == AstType::FnExpr) {
                        auto& fnexpr = (FnExpr&)*member;
                        if (fnexpr.name_ == "@copy" || fnexpr.name_ == "@release" ||
                            fnexpr.name_ == "@eq"   || fnexpr.name_ == "@neq"     ||
                            fnexpr.name_ == "@cast"
                        ) {
                            throw LogErr(LogStage::Sema, std::format(
                                "redefinition of built-in method '{}' in enum '{}'",
                                fnexpr.name_, enum_type->name()
                            ), fnexpr.loc_);
                        }

                        Declare(fnexpr, enum_type->method_table());
                    }

                    // Variant
                    else {
                        auto& variant_stmt   = (ExprStmt&)*member;
                        auto& variant_id    = (IdExpr&)*variant_stmt.expr_;
                        auto  variant_index = enum_type->variants().size();
                        auto  variant_max   = static_cast<size_t>(std::numeric_limits<int32_t>::max());
                        
                        if (variant_index >= variant_max) {
                            throw LogErr(LogStage::Sema, std::format(
                                "enum '{}' expects at most {} variants, got {}",
                                enum_type->name(), variant_max, variant_index + 1
                            ), variant_id.loc_);
                        }
                        if (!enum_type->variant_indices().emplace(variant_id.name_, variant_index).second) {
                            throw LogErr(LogStage::Sema, std::format(
                                "redefinition of variant '{}' in enum '{}'",
                                variant_id.name_, enum_type->name()
                            ), variant_id.loc_);
                        }
                        enum_type->variants().emplace_back(EnumVariant{variant_id.name_, variant_index});
                    }
                }

                // Built-in Methods
                enum_type->method_table().Add("@eq",   FnSign(bool_, { enum_type }));
                enum_type->method_table().Add("@neq",  FnSign(bool_, { enum_type }));
                enum_type->method_table().Add("@cast", FnSign(i32_,  {}, std::nullopt, FnModifier::CastLossy));
                enum_type->method_table().Add("@cast", FnSign(i64_,  {}, std::nullopt, FnModifier::CastLossy));
                enum_type->method_table().Add("@print", FnSign(none_));
            }
        }
        if (has_enum_decl) TypeTable::CastRecompute();

        // Other Global Decl
        for (auto& child : node_children) {
            if (child->type_ != AstType::StructDeclExpr &&
                child->type_ != AstType::EnumDeclExpr)
            {
                Declare(*child);
            }
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

    void Analyzer::Process(TypeExpr& node) {
        auto* aliases    = module_ ? &module_->namespaces_alias() : nullptr;
        auto  basic_type = type_scope_.Resolve(node.basic_type_, aliases, node.loc_);
        Type* resolved   = nullptr;

        // Basic Type
        if (!node.params_) {
            resolved = basic_type;
        }

        // Parametric Type
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

        // Reference Type
        if (node.is_referred_) {
            resolved = TypeTable::ReferenceTypeGet(resolved);
        }

        node.resolved_type_ = resolved;
    }

    void Analyzer::Process(DeclExpr& node) {
        Process(*node.bind_type_);
        node.resolved_type_ = node.bind_type_->resolved_type_;

        // Constant
        if (node.modifier_ & VarModifier::Const) {

            // Module
            if (fn_depth_ == 0) {

                // Reference Type
                if (node.bind_type_->is_referred_) {
                    throw LogErr(LogStage::Sema, "module constant cannot have a reference type", node.loc_);
                }

                Process(*node.value_);

                if (!AssignableCheck(node.value_->resolved_type_, node.bind_type_->resolved_type_)) {
                    throw LogErr(LogStage::Sema, std::format(
                        "cannot make type '{}' compatible with '{}'",
                        node.value_->resolved_type_->name(),
                        node.bind_type_->resolved_type_->name()
                    ), node.value_->loc_);
                }

                if (!ModuleConstantStructureCheck(*node.value_)) {
                    throw LogErr(LogStage::Sema, "initializer of module constant must be a constant expression", node.value_->loc_);
                }

                if (!ModuleConstantTypeSupported(node.bind_type_->resolved_type_)) {
                    throw LogErr(LogStage::Sema, std::format(
                        "unsupported module constant type '{}'",
                        node.bind_type_->resolved_type_->name()
                    ), node.loc_);
                }
                
                return;
            }

            // Local
            else {
                if (!node.value_) {
                    throw LogErr(LogStage::Sema, "constant must have an initializer", node.loc_);
                }
            }
        }

        // Assignable Check
        {
            // Reference Type
            if (node.bind_type_->is_referred_) {
                // x: i32& = y;
                if (node.value_)  {
                    Process(*node.value_);
                
                    // x: i32& = 3;
                    if (!dynamic_cast<IdExpr*>(node.value_.get())) {
                        throw LogErr(LogStage::Sema, std::format(
                            "cannot assign non-reference value to reference type '{}'",
                            node.bind_type_->resolved_type_->name()
                        ), node.value_->loc_);
                    }
                }

                // x: i32&;
                else {
                    throw LogErr(LogStage::Sema, "reference type must be initialized with a value", node.loc_);
                }
            }
        
            // Non-Reference Type
            else {
                if (node.value_) {
                    Process(*node.value_);

                    if (!AssignableCheck(node.value_->resolved_type_, node.bind_type_->resolved_type_)) {
                        throw LogErr(LogStage::Sema, std::format(
                            "cannot make type '{}' compatible with '{}'",
                            node.value_->resolved_type_->name(),
                            node.bind_type_->resolved_type_->name()
                        ), node.value_->loc_);
                    }
                }
            }
        }
        node.resolved_type_ = node.bind_type_->resolved_type_;

        var_scope_.Declare(std::make_unique<Var>(
            node.id_, node.resolved_type_, node.loc_, node.modifier_
        ));
    }

    void Analyzer::Process(IdExpr& node) {
        if (auto variable        = var_scope_.LookupTry(node.name_)) {
            node.resolved_type_ = variable->type_;
            return;
        }
        if (auto module_constant = module_const_scope_.LookupTry(
            Path::Resolve({ node.name_ }, module_->namespaces_alias())
        )) {
            node.module_const_decl_ = module_constant;
            node.resolved_type_     = module_constant->resolved_type_;
            return;
        }

        throw LogErr(LogStage::Sema, std::format(
            "undefined identifier '{}'", node.name_
        ), node.loc_);
    }

    void Analyzer::Process(RefExpr& node) {
        if (auto idexpr = dynamic_cast<IdExpr*>(node.target_.get())) {
            Process(*node.target_);
            auto var = var_scope_.LookupTry(idexpr->name_);

            if (idexpr->module_const_decl_ ||                       // module constant
                (var && (var->modifier_ & VarModifier::Const)))     // local  constant
            {
                throw LogErr(LogStage::Sema, std::format(
                    "cannot make a reference to constant '{}'", idexpr->name_
                ), node.loc_);
            }

            node.resolved_type_ = TypeTable::ReferenceTypeGet(node.target_->resolved_type_->ReferenceUnwrap());
            return;
        }

        throw LogErr(LogStage::Sema, "cannot make a reference to a non-referenceable value", node.loc_);
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

            // Left, Right and Common Type
            auto ltype  = node.lexpr_->resolved_type_->ReferenceUnwrap();
            auto rtype  = node.rexpr_->resolved_type_->ReferenceUnwrap();
            if (auto com_type = TypeTable::CommonTypeGet({ ltype, rtype })) {
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

                    default:
                        node.resolved_type_ = com_type;
                        return;
                }
            }

            // Left Fallback
            const char* method_name = nullptr;
            switch (node.oper_type_) {
                case Plus:  method_name = "@plus";  break;
                case Minus: method_name = "@minus"; break;
                case Star:  method_name = "@star";  break;
                case Slash: method_name = "@slash"; break;
                case ModT:  method_name = "@modt";  break;
                case ModF:  method_name = "@modf";  break;
                case Gt:    method_name = "@gt";    break;
                case Lt:    method_name = "@lt";    break;
                case Ge:    method_name = "@ge";    break;
                case Le:    method_name = "@le";    break;
                case Eq:    method_name = "@eq";    break;
                case Neq:   method_name = "@neq";   break;
                default: break;
            }

            if (!method_name) {
                throw LogErr(LogStage::Sema, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    ltype->name(), rtype->name()
                ), node.loc_);
            }

            auto method = TypeTable::MethodLookupTry(ltype, method_name);
            if (!method) {
                throw LogErr(LogStage::Sema, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    ltype->name(), rtype->name()
                ), node.loc_);
            }

            auto sign = method->SignLookup(ltype, { rtype }, node.loc_);
            node.resolved_type_ = sign->return_type();
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
            throw LogErr(LogStage::Sema, std::format(
                "left bound type '{}' must be compatible with right bound type '{}'",
                node.lexpr_->resolved_type_->ReferenceUnwrap()->name(),
                node.rexpr_->resolved_type_->ReferenceUnwrap()->name()
            ), node.loc_);
        }

        // Step
        auto step_type = boundary_type;
        if (node.step_) {
            Process(*node.step_);
            step_type = node.step_->resolved_type_->ReferenceUnwrap();
            if (TypeTable::CommonTypeGet({ step_type, boundary_type }) != boundary_type) {
                throw LogErr(LogStage::Sema, std::format(
                    "step type '{}' must be compatible with boundary type '{}'",
                    step_type->name(), boundary_type->name()
                ), node.loc_);
            }
        }

        node.iter_type_ = boundary_type;
        node.resolved_type_ = TypeTable::ParametricTypeGet(
            TypeTable::Lookup("range"), { node.iter_type_ }, node.loc_
        );
    }

    void Analyzer::Process(ArrayLitExpr& node) {
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

        // XXX: Empty ArrayLitExpr
        std::vector<Type*> params = {};
        if (node.elem_type_) params.emplace_back(node.elem_type_);

        node.resolved_type_ = TypeTable::ParametricTypeGet(
            TypeTable::Lookup("array", node.loc_), params
        );
    }

    void Analyzer::Process(FnExpr& node) {
        if (!node.ret_resolved_type_) Declare(node, fn_scope_);
        
        // Parameter Binding
        if (node.body_) {
            auto& params_expr = node.params_->exprs_;
            auto* owner       = node.fnsign_->caller_type();

            fn_depth_++;
            try {
                Process(*node.body_, [&]() {
                    // Self
                    if (owner) {
                        var_scope_.Declare(std::make_unique<Var>(
                            "self", TypeTable::ReferenceTypeGet(owner), node.loc_)
                        );
                    }

                    for (size_t i = 0; i < params_expr.size(); i++) {
                        auto expr = (DeclExpr*)(params_expr[i].get());
                        var_scope_.Declare(std::make_unique<Var>(
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

    void Analyzer::Process(FnCallExpr& node) {

        // Args Type
        std::vector<Type*> args_type = {};
        for (auto& e : node.args_->exprs_) {
            Process(*e);
            args_type.emplace_back(e->resolved_type_);
        }

        // Callee Path
        Path callee_path = {};
        if      (auto* idexpr   = dynamic_cast<IdExpr*>(node.callee_.get())) {
            callee_path.Add(idexpr->name_);
        }
        else if (auto* pathexpr = dynamic_cast<PathExpr*>(node.callee_.get())) {
            for (auto& seg : pathexpr->segments_) {
                callee_path.Add(seg->name_);
            }

            // Alias: a::b::c::fn -> d::fn
            if (module_) {
                callee_path = Path::Resolve(callee_path, module_->namespaces_alias());
            }
        }
        else {
            throw LogErr(LogStage::Sema, std::format(
                "invalid callee '{}'", node.callee_->TypeName()
            ), node.loc_);
        }

        // Function Call: callee.fn()
        if (auto* fn = fn_scope_.LookupTry(callee_path)) {
            auto sign = fn->SignLookup(args_type, node.callee_->loc_);
            node.resolved_type_ = sign->return_type();
            node.callee_fnsign_ = sign;
            return;
        }

        // Type Cast: T(expr)
        auto* type = type_scope_.LookupTry(callee_path);
        if (!type) type = TypeTable::LookupTry(callee_path.Print());

        if (type) {
            auto IsNumeric = [](Type* type) {
                return type->Is("i32") || type->Is("i64") ||
                       type->Is("f32") || type->Is("f64");
            };

            if (args_type.size() == 1) {
                auto* from = args_type[0]->ReferenceUnwrap();

                if ((IsNumeric(from) && IsNumeric(type)) ||
                    (dynamic_cast<EnumType*>(from) && (type->Is("i32") || type->Is("i64"))))
                {
                    auto* from_basic = (BasicType*)from->BasicTypeGet();

                    if (from != type &&
                        !from_basic->casts().contains(type) &&
                        !from_basic->casts_lossy().contains(type))
                    {
                        throw LogErr(LogStage::Sema, std::format(
                            "cannot cast type from '{}' to '{}'",
                            from->name(), type->name()
                        ), node.loc_);
                    }

                    node.cast_type_     = type;
                    node.resolved_type_ = type;
                    return;
                }
            }
        }

        throw LogErr(LogStage::Sema, std::format(
            "undefined function '{}'", callee_path.Print()
        ), node.loc_);
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
        auto callee = node.callee_->name_;
        auto method = TypeTable::MethodLookup(caller_type, callee, node.callee_->loc_);
        auto sign   = method->SignLookup(caller_type, args_type, node.callee_->loc_);
        
        node.resolved_type_ = sign->return_type();
        node.callee_fnsign_ = sign;
    }

    void Analyzer::Process(StructDeclExpr& node) {
        auto* struct_type = (StructType*)node.resolved_type_;
        if (!struct_type) {
            throw LogErr(LogStage::Sema,
                "struct declaration must be at module scope",
                node.loc_
            );
        }

        // Cycle Definition Check
        {
            std::set<StructType*> visiting = {};
            std::function<void(StructType*)> CycleDefinitionCheck = [&](StructType* type) {
                if (!visiting.insert(type).second) {
                    throw LogErr(LogStage::Sema, std::format(
                        "cyclic definition of struct '{}' by value", type->name()
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

        // Methods
        {
            for (auto& child : node.body_->children_) {
                if (child->type_ == AstType::FnExpr) Process((FnExpr&)*child);
            }
        }
    }

    void Analyzer::Process(StructLitExpr& node) {
        Process(*node.lit_type_);
        auto* struct_type = dynamic_cast<StructType*>(node.lit_type_->resolved_type_);

        // Non-Struct Type
        if (!struct_type) {
            throw LogErr(LogStage::Sema, std::format(
                "type of struct literal must be a struct type, not '{}'",
                node.lit_type_->resolved_type_->name()
            ), node.loc_);
        }

        // Struct Type
        else {
            std::set<std::string> initialized = {};

            std::function<void(FieldInitExpr*)> FieldInitProcess = [&](FieldInitExpr* init) {
                // Undefined Field
                auto it = struct_type->field_indices().find(init->id_->name_);
                if (it == struct_type->field_indices().end()) {
                    throw LogErr(LogStage::Sema, std::format(
                        "undefined field '{}' in type '{}'",
                        init->id_->name_, struct_type->name()
                    ), init->id_->loc_);
                }

                // Duplicated Field
                if (!initialized.emplace(init->id_->name_).second) {
                    throw LogErr(LogStage::Sema, std::format(
                        "duplicate field '{}' in struct literal of type '{}'",
                        init->id_->name_, struct_type->name()
                    ), init->id_->loc_);
                }

                // Field Value
                Process(*init->value_);
                auto* value_type = init->value_->resolved_type_;
                auto& field      = struct_type->fields()[it->second];

                if (!AssignableCheck(value_type, field.type_)) {
                    throw LogErr(LogStage::Sema, std::format(
                        "cannot make type '{}' compatible with '{}'",
                        value_type->name(), field.type_->name()
                    ), init->value_->loc_);
                }
            };

            for (auto& child : node.body_->children_) {
                FieldInitProcess((FieldInitExpr*)child.get());
            }

            node.resolved_type_ = struct_type;
        }
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

    void Analyzer::Process(EnumDeclExpr& node) {
        auto* enum_type = (EnumType*)node.resolved_type_;
        if (!enum_type) {
            throw LogErr(LogStage::Sema,
                "enum declaration must be at module scope",
                node.loc_
            );
        }

        // Methods
        {
            for (auto& child : node.body_->children_) {
                if (child->type_ == AstType::FnExpr) Process((FnExpr&)*child);
            }
        }
    }

    void Analyzer::Process(PathExpr& node) {
        Path path = {}, path_prefix = {};

        for (auto& segment : node.segments_) {
            path.Add(segment->name_);
        }
        path = Path::Resolve(path, module_->namespaces_alias());
        for (size_t i = 0; i + 1 < path.Length(); i++) {
            path_prefix.Add(path[i]);
        }

        // Resolve

        auto* resolve_module_constant = module_const_scope_.LookupTry(path);
        auto* resolve_type            = type_scope_.LookupTry(path_prefix);

        if (resolve_module_constant && resolve_type) {
            throw LogErr(LogStage::Sema, std::format(
                "ambiguous path '{}'", path.Print()
            ), node.loc_);
        }

        {
            // Function:            a::callee.fn(), reduce as FnCallExpr or MethodCallExpr

            // Module Constant:     a::constant
            if (resolve_module_constant) {
                node.resolved_target_ = resolve_module_constant;
                node.resolved_type_   = resolve_module_constant->resolved_type_;
                return;
            }

            // Type Member:         a::type::...
            if (resolve_type) {
                const auto& member    = path.Back();

                // Enum Variant:    a::enum::variant
                if (auto* enum_type = dynamic_cast<EnumType*>(resolve_type)) {
                    auto it = enum_type->variant_indices().find(member);
                    if (it == enum_type->variant_indices().end()) {
                        throw LogErr(LogStage::Sema, std::format(
                            "undefined variant '{}' in enum '{}'",
                            member, enum_type->name()
                        ), node.loc_);
                    }

                    node.resolved_target_ = &enum_type->variants()[it->second];
                    node.resolved_type_   = enum_type;
                    return;
                }           
            
                throw LogErr(LogStage::Sema, std::format(
                    "undefined member '{}' in type '{}'",
                    member, resolve_type->name()
                ), node.loc_);
            }
        }

        throw LogErr(LogStage::Sema, std::format(
            "undefined identifier '{}'", path.Print()
        ), node.loc_);
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

            // f64
            double x = 0.0;
            auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
            if (ec != std::errc{}) {
                throw LogErr(LogStage::Sema, std::format(
                    "numeric overflow '{}'", numstr
                ), node.loc_);
            }

            node.resolved_type_ = TypeTable::Lookup("f64");
            node.resolved_value_.floating_ = x;
            return;
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
        Process(*node.expr_);
    }

    void Analyzer::Process(AssignStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");

        Process(*node.target_);
        Process(*node.value_);

        if (auto idexpr   = dynamic_cast<IdExpr*>(node.target_.get())) {
            // Module Constant
            if (idexpr->module_const_decl_) {
                throw LogErr(LogStage::Sema, std::format(
                    "cannot assign to constant '{}'", idexpr->module_const_decl_->id_
                ), node.target_->loc_);
            }
            // Variable
            if (auto var = var_scope_.LookupTry(idexpr->name_)) {
                // Local Constant
                if (var->modifier_ & VarModifier::Const) {
                    throw LogErr(LogStage::Sema, std::format(
                        "cannot assign to constant '{}'", idexpr->name_
                    ), node.target_->loc_);
                }
            }
        }
        if (auto pathexpr = dynamic_cast<PathExpr*>(node.target_.get())) {
            // Constant
            if (auto* module_constant = pathexpr->ModuleConstDeclGet()) {
                throw LogErr(LogStage::Sema, std::format(
                    "cannot assign to constant '{}'", module_constant->id_
                ), node.target_->loc_);
            }
            // Enum
            if (auto* enum_variant    = pathexpr->EnumVariantGet()) {
                throw LogErr(LogStage::Sema, std::format(
                    "cannot assign to enum variant '{}'", enum_variant->name_
                ), node.target_->loc_);
            }
        }

        // Assignable Check
        {
            auto* target_type = node.target_->resolved_type_->ReferenceUnwrap();
            auto* value_type  = node.value_->resolved_type_;
            if (!AssignableCheck(value_type, target_type)) {
                throw LogErr(LogStage::Sema, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    value_type->name(), target_type->name()
                ), node.value_->loc_);
            }
        }
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

    void Analyzer::Process(MatchStmt& node) {
        node.resolved_type_ = TypeTable::Lookup("none");
        Process(*node.target_);

        auto* target_type           = node.target_->resolved_type_->ReferenceUnwrap();
        auto* target_enum_type      = dynamic_cast<EnumType*>(target_type);
        bool  is_target_scalar_type =
            target_type->Is("i32")  || target_type->Is("i64") ||
            target_type->Is("bool") || target_type->Is("char");

        // Struct Target
        if (dynamic_cast<StructType*>(target_type)) {
            bool has_catch_all = false;

            // Branches
            for (auto& child : node.body_->children_) {
                auto& branch = (MatchBranchExpr&)*child;

                // Unreachable: an always-match branch exists before this
                if (has_catch_all) {
                    LogWarn(LogStage::Sema, "unreachable pattern", branch.loc_).Print();
                }

                // Pattern
                auto result = PatternCheck(*branch.pattern_, target_type);
                if (result.always_match_) {
                    has_catch_all = true;
                }

                // Body
                Process(*branch.body_);
            }

            // No Always-Match Branch
            if (!has_catch_all) {
                throw LogErr(LogStage::Sema, std::format(
                    "match on type '{}' must have an 'else' pattern",
                    target_type->name()
                ), node.loc_);
            }
            
            return;
        }

        // Const Target
        else if (is_target_scalar_type || target_enum_type) {
            std::set<int64_t> cases         = {};
            bool              has_catch_all = false;

            // Domain: finite domain size
            //     - enum:        all of variant
            //     - bool:        2
            //     - other const: unrestricted
            std::optional<size_t> domain = std::nullopt;
            if      (target_enum_type) {
                domain = target_enum_type->variants().size();
            }
            else if (target_type->Is("bool")) {
                domain = 2;
            }

            // Branches
            for (auto& child : node.body_->children_) {
                auto& branch = (MatchBranchExpr&)*child;

                // Unreachable:
                //     - an always-match branch exists before this
                //     - out of domain
                if (has_catch_all || (domain && cases.size() == *domain)) {
                    LogWarn(LogStage::Sema, "unreachable pattern", branch.loc_).Print();
                }

                // Pattern
                auto result = PatternCheck(*branch.pattern_, target_type);
                if (result.always_match_) {
                    has_catch_all = true;
                }
                if (result.case_value_) {
                    if (!cases.emplace(*result.case_value_).second) {
                        throw LogErr(LogStage::Sema, "duplicate match pattern", branch.loc_);
                    }
                }

                // Body
                Process(*branch.body_);
            }

            // No Always-Match Branch
            if (!has_catch_all) {
                // Open domain
                if (!domain) {
                    throw LogErr(LogStage::Sema, std::format(
                        "match on type '{}' must have an 'else' pattern",
                        target_type->name()
                    ), node.loc_);
                }

                // Incomplete Finite Domain
                if (cases.size() != *domain) {
                    throw LogErr(LogStage::Sema, std::format(
                        "match expects {} patterns, got {}",
                        *domain, cases.size()
                    ), node.loc_);
                }
            }

            return;
        }

        throw LogErr(LogStage::Sema, std::format(
            "match target must be an enum, scalar, or struct type, not '{}'",
            target_type->name()
        ), node.target_->loc_);
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

        if (data_type->Is("array")  || data_type->Is("arrayview")) {
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
                "type '{}' must be iterable", data_type->name()
            ), node.data_->loc_);
        }

        Process(*node.body_, [&]() {
            var_scope_.Declare(std::make_unique<Var>(
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
            // Struct and Enum
            for (auto& child : node.children_) {
                if (child->type_ == AstType::StructDeclExpr) Process((StructDeclExpr&)*child);
                if (child->type_ == AstType::EnumDeclExpr)   Process((EnumDeclExpr&)*child);
            }

            // Other
            for (auto& child : node.children_) {
                if (child->type_ == AstType::StructDeclExpr ||
                    child->type_ == AstType::EnumDeclExpr
                ) {
                    continue;
                }
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

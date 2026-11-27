
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/analyzer/analyzer.hpp"

namespace sema {

    // Module Constant

    void Analyzer::ModuleConstantTypeCacheInit() {
        module_constant_i32_  = TypeTable::Lookup("i32");
        module_constant_i64_  = TypeTable::Lookup("i64");
        module_constant_f32_  = TypeTable::Lookup("f32");
        module_constant_f64_  = TypeTable::Lookup("f64");
        module_constant_bool_ = TypeTable::Lookup("bool");
        module_constant_char_ = TypeTable::Lookup("char");
    }

    bool Analyzer::ModuleConstantStructureCheck(Expr& node) {
        using enum OperType;

        if (dynamic_cast<NumConst*>(&node))  return true;
        if (dynamic_cast<BoolConst*>(&node)) return true;
        if (dynamic_cast<CharConst*>(&node)) return true;
        
        // = expr, expr is constant
        if (auto idexpr     = dynamic_cast<IdExpr*>(&node)) {
            return idexpr->module_const_decl_ != nullptr;
        }
        if (auto pathexpr   = dynamic_cast<PathExpr*>(&node)) {
            return pathexpr->ModuleConstDeclGet() != nullptr ||
                   pathexpr->EnumVariantGet()     != nullptr;
        }
        
        // = T(expr), expr is constant expr
        if (auto fncallexpr = dynamic_cast<FnCallExpr*>(&node); fncallexpr && fncallexpr->cast_type_) {
            return ModuleConstantStructureCheck(*fncallexpr->args_->exprs_[0]);
        }
        
        // = x + y..., simple expr
        if (auto operexpr   = dynamic_cast<OperExpr*>(&node)) {
            switch (operexpr->oper_type_) {
                case Plus:
                case Minus:
                case Star:
                case Slash:
                case ModT:
                case Neg:
                case Eq:
                case Neq:
                    break;
                default:
                    return false;
            }

            if (!ModuleConstantStructureCheck(*operexpr->lexpr_)) return false;
            if (operexpr->rexpr_ && !ModuleConstantStructureCheck(*operexpr->rexpr_)) return false;
            return true;
        }

        return false;
    }

    bool Analyzer::ModuleConstantTypeSupported(Type* type) {
        auto* basic = type->BasicTypeGet();
        return basic == module_constant_i32_  || basic == module_constant_i64_  ||
               basic == module_constant_f32_  || basic == module_constant_f64_  ||
               basic == module_constant_bool_ || basic == module_constant_char_ ||
               dynamic_cast<EnumType*>(basic) != nullptr;
    }

    // Pattern

    Analyzer::PatternResult Analyzer::PatternCheck(Pattern& node, Type* target_type) {
        switch (node.type_) {
            // Wildcard Pattern
            case AstType::WildcardPattern: {
                return PatternResult{ .always_match_ = true };
            }

            // Struct Pattern
            case AstType::StructPattern: {
                auto& pattern = (StructPattern&)node;

                if (auto* target_struct_type = dynamic_cast<StructType*>(target_type)) {
                    Process(*pattern.lit_type_);
                    auto pattern_struct_type = pattern.lit_type_->resolved_type_;

                    if (pattern_struct_type == target_struct_type) {
                        std::set<std::string> field_names  = {};
                        bool                  always_match = true;

                        for (auto& field : pattern.fields_) {

                            // Field
                            auto it = target_struct_type->field_indices().find(field.id_);
                            if (it == target_struct_type->field_indices().end()) {
                                throw LogErr(LogStage::Sema, std::format(
                                    "undefined field '{}' in type '{}'",
                                    field.id_, target_struct_type->name()
                                ), field.loc_);
                            }
                            if (!field_names.emplace(field.id_).second) {
                                throw LogErr(LogStage::Sema, std::format(
                                    "duplicate field '{}' in struct pattern of type '{}'",
                                    field.id_, target_struct_type->name()
                                ), field.loc_);
                            }

                            // Pattern
                            auto result = PatternCheck(
                                *field.pattern_,
                                target_struct_type->fields()[it->second].type_
                            );
                            always_match = always_match && result.always_match_;
                        }

                        return PatternResult{
                            .always_match_ = always_match,
                            .case_value_   = std::nullopt
                        };
                    }
                    else {
                        throw LogErr(LogStage::Sema, std::format(
                            "cannot make type '{}' compatible with '{}'",
                            pattern_struct_type->name(),
                            target_struct_type->name()
                        ), pattern.loc_);
                    }
                }
                else {
                    throw LogErr(LogStage::Sema, std::format(
                        "struct pattern must match a struct type, not '{}'",
                        target_type->name()
                    ), pattern.loc_);
                }
            }

            // Const Pattern
            case AstType::ConstPattern: {
                auto& pattern = (ConstPattern&)node;
                Process(*pattern.value_);

                // Enum
                if (auto* target_enum_type = dynamic_cast<EnumType*>(target_type)) {
                    auto* pathexpr     = dynamic_cast<PathExpr*>(pattern.value_.get());
                    auto* enum_variant = pathexpr ? pathexpr->EnumVariantGet() : nullptr;

                    // Exist as Enum Variant
                    if (!enum_variant) {
                        throw LogErr(LogStage::Sema,
                            "match pattern must be an enum variant",
                            pattern.loc_
                        );
                    }

                    // Belong to Enum
                    if (pathexpr->resolved_type_ != target_enum_type) {
                        throw LogErr(LogStage::Sema, std::format(
                            "undefined variant '{}' in enum '{}'",
                            enum_variant->name_, target_enum_type->name()
                        ), pattern.loc_);
                    }

                    return { false, (int64_t)enum_variant->index_ };
                }

                // Bool
                if (target_type->Is("bool")) {
                    auto* value = dynamic_cast<BoolConst*>(pattern.value_.get());
                    if (!value) {
                        throw LogErr(LogStage::Sema,
                            "match pattern must be a 'bool' literal",
                            pattern.loc_
                        );
                    }

                    return { false, value->value_ ? 1 : 0 };
                }

                // Char
                if (target_type->Is("char")) {
                    auto* value = dynamic_cast<CharConst*>(pattern.value_.get());
                    if (!value) {
                        throw LogErr(LogStage::Sema,
                            "match pattern must be a 'char' literal",
                            pattern.loc_
                        );
                    }

                    return { false, value->codepoint_ };
                }

                // Integer
                if (target_type->Is("i32") || target_type->Is("i64")) {
                    auto* value = dynamic_cast<NumConst*>(pattern.value_.get());
                    if (!value || !AssignableCheck(value->resolved_type_, target_type)) {
                        throw LogErr(LogStage::Sema,
                            "match pattern must be an integer literal",
                            pattern.loc_
                        );
                    }

                    return { false, value->resolved_value_.integer_ };
                }

                // Unsupported
                throw LogErr(LogStage::Sema, std::format(
                    "constant pattern must match an enum or scalar type, not '{}'",
                    target_type->name()
                ), pattern.loc_);
            }

            default: {
                throw LogErr(LogStage::Sema, "invalid match pattern", node.loc_);
            }
        }
    }
}

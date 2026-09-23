
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <vector>

#include "common/utils/format.hpp"
#include "sema/defs/fn.hpp"
#include "sema/defs/type.hpp"

namespace sema {
    
    // Type

    bool Type::IsNone() {
        return Is("none");
    }

    bool Type::Is(std::string_view name) {
        if (this->name_ == name) return true;

        Type* basic_type = BasicTypeGet();
        return basic_type != this && basic_type->Is(name);
    }

    void Type::BasicTypeCheck() const {
        if (type_using_ != Using::Basic) {
            throw LogErr(LogModule::Sema, std::format(
                "invalid basic type '{}'", name_
            ));
        }
    }
    
    // ParametricType

    std::string ParametricType::ParamsPrint(
        Type* basic_type, const std::vector<Type*>& params
    ) {
        basic_type->BasicTypeCheck();
        return basic_type->name() + format::JoinWithBoundary(params, [](Type* type) {
            return type->name();
        }, "[=", "=]");
    }

    // TypeTable

    Type*   TypeTable::Set(const BasicType& type) {
        if (!table_.contains(std::string(type.name()))) {
            auto set = table_.emplace(
                type.name(), 
                new BasicType(type.name(), type.params_cnt())
            );
            return set.first->second;
        }
        else throw LogErr(LogModule::Sema, std::format("redefinition of type '{}'", type.name()));
    }

    Type*   TypeTable::Set(const ParametricType& type) {
        if (!table_.contains(std::string(type.name()))) {
            auto set = table_.emplace(
                type.name(),
                new ParametricType(type)
            );
            return set.first->second;
        }
        else throw LogErr(LogModule::Sema, std::format("redefinition of type '{}'", type.name()));
    }

    Type*   TypeTable::Set(const ReferenceType& type) {
        if (!table_.contains(std::string(type.name()))) {
            auto set = table_.emplace(
                type.name(),
                new ReferenceType(type)
            );
            return set.first->second;
        }
        else throw LogErr(LogModule::Sema, std::format("redefinition of type '{}'", type.name()));
    }
    
    Type*   TypeTable::Lookup(std::string_view name, std::optional<Loc> loc) {
        auto it = table_.find(std::string(name));
        if (it != table_.end()) {
            return it->second;
        }
        throw LogErr(LogModule::Sema, std::format("undefined type '{}'", name), loc);
    }

    Type*   TypeTable::LookupTry(std::string_view name) {
        auto it = table_.find(std::string(name));
        return it == table_.end() ? nullptr : it->second;
    }
    
    Type*   TypeTable::ParametricTypeGet(Type* type, const std::vector<Type*>& params, std::optional<Loc> loc) {
        type->BasicTypeCheck();
        if (params.empty()) return Lookup(type->name(), loc);

        auto basic_type = (BasicType*)type;
        if (basic_type->params_cnt() != params.size()) {
            throw LogErr(LogModule::Sema, std::format(
                "type '{}' expects {} type parameter(s), got {}",
                basic_type->name(), basic_type->params_cnt(), params.size()
            ), loc);
        }
        
        auto name = ParametricType::ParamsPrint(basic_type, params);
        auto it   = table_.find(name);
        if (it != table_.end()) return it->second;

        auto parametric_type = (ParametricType*)Set(ParametricType(
            name, basic_type, params
        ));

        for (auto& [method_name, method] : basic_type->method_table().table()) {
            for (auto& sign : method.signs()) {
                if (!IsContainBindingType(*sign)) continue;
                parametric_type->method_table().Add(
                    method_name,
                    SignInstantiate(*sign, basic_type, params)
                );
            }
        }

        CastRecompute();
        return parametric_type;
    }

    bool    TypeTable::IsContainBindingType(Type* type) {

        // T
        if (dynamic_cast<BindingType*>(type)) return true;

        // T&
        if (auto ref = dynamic_cast<ReferenceType*>(type)) {
            return IsContainBindingType(ref->referred());
        }

        // array[=T, T=]
        if (auto par = dynamic_cast<ParametricType*>(type)) {
            for (auto param : par->params()) {
                if (IsContainBindingType(param)) return true;
            }
        }
        return false;
    }

    bool    TypeTable::IsContainBindingType(const FnSign& sign) {
        if (sign.return_type() && IsContainBindingType(sign.return_type())) return true;
        for (auto param : sign.params_type_fix()) {
            if (param && IsContainBindingType(param)) return true;
        }
        if (sign.params_type_var() && *sign.params_type_var()) {
            if (IsContainBindingType(*sign.params_type_var())) return true;
        }
        return false;
    }

    Type*   TypeTable::BindingTypeReplace(Type* type, BasicType* owner, const std::vector<Type*>& params_replace) {
        
        // T
        if (auto binding_type = dynamic_cast<BindingType*>(type)) {
            auto& params_binding = owner->params_binding();
            for (size_t i = 0; i < params_binding.size(); i++) {
                if (params_binding[i] == binding_type) return params_replace[i];
            }
            return binding_type;
        }

        // T&
        if (auto reference_type = dynamic_cast<ReferenceType*>(type)) {
            return ReferenceTypeGet(BindingTypeReplace(
                reference_type->referred(), owner, params_replace
            ));
        }

        // array[=T, T=]
        if (auto parametric_type = dynamic_cast<ParametricType*>(type)) {
            std::vector<Type*> params = {};
            for (auto param : parametric_type->params()) {
                params.emplace_back(BindingTypeReplace(param, owner, params_replace));
            }
            return ParametricTypeGet(parametric_type->basic(), params);
        }
        
        return type;
    }

    FnSign  TypeTable::SignInstantiate(const FnSign& sign, BasicType* owner, const std::vector<Type*>& params_replace) {
        std::vector<Type*> params_type_fix = {};
        for (auto param : sign.params_type_fix())
            params_type_fix.emplace_back(param ? BindingTypeReplace(param, owner, params_replace) : nullptr);

        std::optional<Type*> params_type_var = std::nullopt;
        if (sign.params_type_var())
            params_type_var = *sign.params_type_var() ? BindingTypeReplace(*sign.params_type_var(), owner, params_replace) : nullptr;

        FnSign res(
            BindingTypeReplace(sign.return_type(), owner, params_replace),
            params_type_fix, params_type_var, sign.modifier(), sign.name()
        );
        res.TemplateSignSet(&sign);
        return res;
    }

    Fn*     TypeTable::MethodLookup(Type* type, const std::string& name) {
        auto type_unwrap = type->ReferenceUnwrap();
        if (auto parametric_type = dynamic_cast<ParametricType*>(type_unwrap)) {
            if (auto fn = parametric_type->method_table().LookupTry(name)) return fn;
        }
        return &((BasicType*)type_unwrap->BasicTypeGet())->method_table().Lookup(name);
    }

    Fn*     TypeTable::MethodLookupTry(Type* type, const std::string& name) {
        auto type_unwrap = type->ReferenceUnwrap();
        if (auto parametric_type = dynamic_cast<ParametricType*>(type_unwrap)) {
            if (auto fn = parametric_type->method_table().LookupTry(name)) return fn;
        }
        return ((BasicType*)type_unwrap->BasicTypeGet())->method_table().LookupTry(name);
    }

    Type*   TypeTable::ReferenceTypeGet(Type* type) {
        auto name = type->name() + '&';
        auto it   = table_.find(name);
        if (it != table_.end()) return it->second;
        return Set(ReferenceType(name, type));
    }

    void    TypeTable::CastRecompute() {

        // Clear
        for (auto& [type_name, type] : table_) {
            type->casts().clear();
            type->casts_fnsign().clear();
            type->casts().emplace(type);
        }

        // Recompute
        for (auto& [type_name, type] : table_) {
            if (auto basic_type = dynamic_cast<BasicType*>(type)) {
                auto& method_table = basic_type->method_table();
                for (auto& [method_name, method] : method_table.table()) {
                    for (auto& sign : method.signs()) {
                        if (sign->modifier() & FnModifier::Cast && sign->return_type()) {
                            type->casts().emplace(sign->return_type());
                            type->casts_fnsign()[sign->return_type()] = sign.get();
                        }
                    }
                }
            }
        }
    }
}

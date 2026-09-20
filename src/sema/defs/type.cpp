
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <vector>

#include "common/utils/format.hpp"
#include "sema/defs/fn.hpp"
#include "sema/defs/type.hpp"

namespace sema {
    
    // Type

    bool Type::isNone() {
        return is("none");
    }

    bool Type::is(std::string_view name) {
        if (this->name_ == name) return true;

        Type* base_type = BasicTypeGet();
        return base_type != this && base_type->is(name);
    }

    void Type::BasicTypeCheck() const {
        if (type_using_ != Using::Basic) {
            throw LogErr(LogModule::Sema, std::format(
                "invalid basic type '{}'", name_
            ));
        }
    }
    
    // ParametricType

    std::string ParametricType::ParamsPrint(Type* base, const std::vector<Type*>& params_type) {
        base->BasicTypeCheck();
        return base->name() + format::JoinWithBoundary(params_type, [](Type* type) {
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
    
    Type*   TypeTable::ParametricTypeGet(Type* type, const std::vector<Type*>& params_type, std::optional<Loc> loc) {
        type->BasicTypeCheck();
        if (params_type.empty()) return Lookup(type->name(), loc);

        auto base_type = (BasicType*)type;
        if (base_type->params_cnt() != params_type.size()) {
            throw LogErr(LogModule::Sema, std::format(
                "type '{}' expects {} type parameter(s), got {}",
                base_type->name(), base_type->params_cnt(), params_type.size()
            ), loc);
        }
        
        auto name = ParametricType::ParamsPrint(base_type, params_type);
        auto it   = table_.find(name);
        if (it != table_.end()) return it->second;

        auto parametric_type = (ParametricType*)Set(ParametricType(
            name, base_type, params_type
        ));

        for (auto& [method_name, method] : base_type->method_table().table()) {
            for (auto& sign : method.signs()) {
                if (!isContainsBinding(*sign)) continue;
                parametric_type->method_table().Add(
                    method_name,
                    InstantiateSign(*sign, base_type, params_type)
                );
            }
        }

        CastRecompute();
        return parametric_type;
    }

    bool    TypeTable::isContainsBinding(Type* type) {
        if (dynamic_cast<BindingType*>(type)) return true;
        if (auto ref = dynamic_cast<ReferenceType*>(type))
            return isContainsBinding(ref->type_referred());
        if (auto par = dynamic_cast<ParametricType*>(type)) {
            for (auto param : par->params_type())
                if (isContainsBinding(param)) return true;
        }
        return false;
    }

    bool    TypeTable::isContainsBinding(const FnSign& sign) {
        if (sign.return_type() && isContainsBinding(sign.return_type())) return true;
        for (auto param : sign.params_type_fix())
            if (param && isContainsBinding(param)) return true;
        if (sign.params_type_var() && *sign.params_type_var())
            if (isContainsBinding(*sign.params_type_var())) return true;
        return false;
    }

    Type*   TypeTable::Substitute(Type* type, BasicType* base, const std::vector<Type*>& args) {
        if (auto binding = dynamic_cast<BindingType*>(type)) {
            auto& decl = base->params_binding();
            for (size_t i = 0; i < decl.size(); i++)
                if (decl[i] == binding) return args[i];
            return binding;
        }
        if (auto ref = dynamic_cast<ReferenceType*>(type))
            return ReferenceTypeGet(Substitute(ref->type_referred(), base, args));
        if (auto par = dynamic_cast<ParametricType*>(type)) {
            std::vector<Type*> params = {};
            for (auto param : par->params_type())
                params.emplace_back(Substitute(param, base, args));
            return ParametricTypeGet(par->type_basic(), params);
        }
        return type;
    }

    FnSign  TypeTable::InstantiateSign(const FnSign& sign, BasicType* base, const std::vector<Type*>& args) {
        std::vector<Type*> params_fix = {};
        for (auto param : sign.params_type_fix())
            params_fix.emplace_back(param ? Substitute(param, base, args) : nullptr);

        std::optional<Type*> params_var = std::nullopt;
        if (sign.params_type_var())
            params_var = *sign.params_type_var() ? Substitute(*sign.params_type_var(), base, args) : nullptr;

        FnSign res(
            Substitute(sign.return_type(), base, args),
            params_fix, params_var, sign.modifier(), sign.name()
        );
        res.TemplateSet(&sign);
        return res;
    }

    Fn*     TypeTable::MethodLookup(Type* type, const std::string& name) {
        auto type_unwrap = type->ReferenceUnwrap();
        if (auto par = dynamic_cast<ParametricType*>(type_unwrap)) {
            if (auto fn = par->method_table().LookupTry(name)) return fn;
        }
        return &((BasicType*)type_unwrap->BasicTypeGet())->method_table().Lookup(name);
    }

    Fn*     TypeTable::MethodLookupTry(Type* type, const std::string& name) {
        auto type_unwrap = type->ReferenceUnwrap();
        if (auto par = dynamic_cast<ParametricType*>(type_unwrap)) {
            if (auto fn = par->method_table().LookupTry(name)) return fn;
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

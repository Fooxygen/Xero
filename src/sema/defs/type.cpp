
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

    Type* TypeTable::Set(const BasicType& type) {
        if (!table_.contains(std::string(type.name()))) {
            auto set = table_.emplace(
                type.name(), 
                new BasicType(type.name(), type.params_cnt())
            );
            return set.first->second;
        }
        else throw LogErr(LogModule::Sema, std::format("redefinition of type '{}'", type.name()));
    }

    Type* TypeTable::Set(const ParametricType& type) {
        if (!table_.contains(std::string(type.name()))) {
            auto set = table_.emplace(
                type.name(),
                new ParametricType(type)
            );
            return set.first->second;
        }
        else throw LogErr(LogModule::Sema, std::format("redefinition of type '{}'", type.name()));
    }
    
    Type* TypeTable::Lookup(std::string_view name, std::optional<Loc> loc) {
        auto it = table_.find(std::string(name));
        if (it != table_.end()) {
            return it->second;
        }
        throw LogErr(LogModule::Sema, std::format("undefined type '{}'", name), loc);
    }

    Type* TypeTable::LookupTry(std::string_view name) {
        auto it = table_.find(std::string(name));
        return it == table_.end() ? nullptr : it->second;
    }
    
    Type* TypeTable::ParametricTypeGet(Type* type, const std::vector<Type*>& params_type, std::optional<Loc> loc) {
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

        auto parametric_type = Set(ParametricType(
            name, base_type, params_type
        ));
        CastRecompute();
        return parametric_type;
    }

    void  TypeTable::CastRecompute() {

        // Clear
        for (auto& [type_name, type] : table_) {
            type->casts().clear();
            type->casts().emplace(type);
        }

        // Recompute
        for (auto& [type_name, type] : table_) {
            if (auto basic_type = dynamic_cast<BasicType*>(type)) {
                auto& method_table = basic_type->method_table();
                for (auto& [method_name, method] : method_table.table()) {
                    for (auto& sign : method.signs()) {
                        if (sign->modifier().hasCast_ && sign->ret_type()) {
                            type->casts().emplace(sign->ret_type());
                            type->casts_fnsign()[sign->ret_type()] = sign.get();
                        }
                    }
                }
            }
        }
    }
}

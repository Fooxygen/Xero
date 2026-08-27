
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <vector>

#include "sema/sign.hpp"
#include "sema/type.hpp"

namespace sema {
    
    // Type

    bool Type::isNone() const {
        return is("none");
    }

    bool Type::is(std::string_view name) const {
        if (this->name_ == name) return true;

        const Type* base_type = BasicTypeGet();
        return base_type != this && base_type->is(name);
    }

    void Type::BasicTypeCheck() const {
        if (type_using_ != Using::Base) {
            throw LogErr(LogModule::Sema, std::format(
                "invalid base type '{}'", name_
            ));
        }
    }
    
    // ParametricType

    std::string ParametricType::ParamsPrint(const Type* base, const std::vector<const Type*>& params_type) {
        base->BasicTypeCheck();
        return base->name_ + JoinWithBoundary(params_type, [](const Type* type) {
            return type->name_;
        }, "[=", "=]");
    }

    // TypeTable

    void  TypeTable::Init() {

        auto none_          = (BasicType*)TypeTable::Set(BasicType("none"));
        auto bool_          = (BasicType*)TypeTable::Set(BasicType("bool"));
        auto i32_           = (BasicType*)TypeTable::Set(BasicType("i32"));
        auto i64_           = (BasicType*)TypeTable::Set(BasicType("i64"));
        auto f32_           = (BasicType*)TypeTable::Set(BasicType("f32"));
        auto f64_           = (BasicType*)TypeTable::Set(BasicType("f64"));
        /*auto char_          = (BasicType*)*/TypeTable::Set(BasicType("char"));
        /*auto string_        = (BasicType*)*/TypeTable::Set(BasicType("string"));
        /*auto stringview_    = (BasicType*)*/TypeTable::Set(BasicType("stringview"));
        /*auto array_         = (BasicType*)*/TypeTable::Set(BasicType("array", 1));
        /*auto arrayview_     = (BasicType*)*/TypeTable::Set(BasicType("arrayview"));
        /*auto range_         = (BasicType*)*/TypeTable::Set(BasicType("range", 1));
        /*auto function_      = (BasicType*)*/TypeTable::Set(BasicType("function"));

        // Info
        {
            // bool
            {
                bool_->MethodAdd("print", FnSign(none_, { bool_ }));
                bool_->MethodAdd("eq",    FnSign(bool_, { bool_, bool_ }));
                bool_->MethodAdd("neq",   FnSign(bool_, { bool_, bool_ }));
                bool_->MethodAdd("and",   FnSign(bool_, { bool_, bool_ }));
                bool_->MethodAdd("or",    FnSign(bool_, { bool_, bool_ }));
                bool_->MethodAdd("not",   FnSign(bool_, { bool_ }));
            }

            // i32
            {
                i32_->MethodAdd("print", FnSign(none_, { i32_ }));
                i32_->MethodAdd("plus",  FnSign(i32_,  { i32_, i32_ }));
                i32_->MethodAdd("minus", FnSign(i32_,  { i32_, i32_ }));
                i32_->MethodAdd("star",  FnSign(i32_,  { i32_, i32_ }));
                i32_->MethodAdd("slash", FnSign(i32_,  { i32_, i32_ }));
                i32_->MethodAdd("neg",   FnSign(i32_,  { i32_ }));
                i32_->MethodAdd("modt",  FnSign(i32_,  { i32_, i32_ }));
                i32_->MethodAdd("modf",  FnSign(i32_,  { i32_, i32_ }));
                i32_->MethodAdd("gt",    FnSign(bool_, { i32_, i32_ }));
                i32_->MethodAdd("lt",    FnSign(bool_, { i32_, i32_ }));
                i32_->MethodAdd("ge",    FnSign(bool_, { i32_, i32_ }));
                i32_->MethodAdd("le",    FnSign(bool_, { i32_, i32_ }));
                i32_->MethodAdd("eq",    FnSign(bool_, { i32_, i32_ }));
                i32_->MethodAdd("neq",   FnSign(bool_, { i32_, i32_ }));
            }

            // i64
            {
                i64_->MethodAdd("print", FnSign(none_, { i64_ }));
                i64_->MethodAdd("plus",  FnSign(i64_,  { i64_, i64_ }));
                i64_->MethodAdd("minus", FnSign(i64_,  { i64_, i64_ }));
                i64_->MethodAdd("star",  FnSign(i64_,  { i64_, i64_ }));
                i64_->MethodAdd("slash", FnSign(i64_,  { i64_, i64_ }));
                i64_->MethodAdd("neg",   FnSign(i64_,  { i64_ }));
                i64_->MethodAdd("modt",  FnSign(i64_,  { i64_, i64_ }));
                i64_->MethodAdd("modf",  FnSign(i64_,  { i64_, i64_ }));
                i64_->MethodAdd("gt",    FnSign(bool_, { i64_, i64_ }));
                i64_->MethodAdd("lt",    FnSign(bool_, { i64_, i64_ }));
                i64_->MethodAdd("ge",    FnSign(bool_, { i64_, i64_ }));
                i64_->MethodAdd("le",    FnSign(bool_, { i64_, i64_ }));
                i64_->MethodAdd("eq",    FnSign(bool_, { i64_, i64_ }));
                i64_->MethodAdd("neq",   FnSign(bool_, { i64_, i64_ }));
            }

            // f32
            {
                f32_->MethodAdd("print", FnSign(none_, { f32_ }));
                f32_->MethodAdd("plus",  FnSign(f32_,  { f32_, f32_ }));
                f32_->MethodAdd("minus", FnSign(f32_,  { f32_, f32_ }));
                f32_->MethodAdd("star",  FnSign(f32_,  { f32_, f32_ }));
                f32_->MethodAdd("slash", FnSign(f32_,  { f32_, f32_ }));
                f32_->MethodAdd("neg",   FnSign(f32_,  { f32_ }));
                f32_->MethodAdd("modt",  FnSign(f32_,  { f32_, f32_ }));
                f32_->MethodAdd("modf",  FnSign(f32_,  { f32_, f32_ }));
                f32_->MethodAdd("gt",    FnSign(bool_, { f32_, f32_ }));
                f32_->MethodAdd("lt",    FnSign(bool_, { f32_, f32_ }));
                f32_->MethodAdd("ge",    FnSign(bool_, { f32_, f32_ }));
                f32_->MethodAdd("le",    FnSign(bool_, { f32_, f32_ }));
                f32_->MethodAdd("eq",    FnSign(bool_, { f32_, f32_ }));
                f32_->MethodAdd("neq",   FnSign(bool_, { f32_, f32_ }));
            }

            // f64
            {
                f64_->MethodAdd("print", FnSign(none_, { f64_ }));
                f64_->MethodAdd("plus",  FnSign(f64_,  { f64_, f64_ }));
                f64_->MethodAdd("minus", FnSign(f64_,  { f64_, f64_ }));
                f64_->MethodAdd("star",  FnSign(f64_,  { f64_, f64_ }));
                f64_->MethodAdd("slash", FnSign(f64_,  { f64_, f64_ }));
                f64_->MethodAdd("neg",   FnSign(f64_,  { f64_ }));
                f64_->MethodAdd("modt",  FnSign(f64_,  { f64_, f64_ }));
                f64_->MethodAdd("modf",  FnSign(f64_,  { f64_, f64_ }));
                f64_->MethodAdd("gt",    FnSign(bool_, { f64_, f64_ }));
                f64_->MethodAdd("lt",    FnSign(bool_, { f64_, f64_ }));
                f64_->MethodAdd("ge",    FnSign(bool_, { f64_, f64_ }));
                f64_->MethodAdd("le",    FnSign(bool_, { f64_, f64_ }));
                f64_->MethodAdd("eq",    FnSign(bool_, { f64_, f64_ }));
                f64_->MethodAdd("neq",   FnSign(bool_, { f64_, f64_ }));
            }

            /*
            // char
            {

            }

            // string
            {
                string_->MethodAdd("len",   FnSign(i32_));
                string_->MethodAdd("clear", FnSign(none_));
            }

            // stringview
            {
                stringview_->MethodAdd("len",       FnSign(i32_));
                stringview_->MethodAdd("to_string", FnSign(string_));
            }

            // array
            {
                array_->MethodAdd("len",        FnSign(i32_));
                array_->MethodAdd("clear",      FnSign(none_));
                array_->MethodAdd("insert",     FnSign(none_, { i32_, nullptr }));
                array_->MethodAdd("remove",     FnSign(none_, { i32_ }));
                array_->MethodAdd("push_front", FnSign(none_, { nullptr }));
                array_->MethodAdd("pop_front",  FnSign(none_));
                array_->MethodAdd("push_back",  FnSign(none_, { nullptr }));
                array_->MethodAdd("pop_back",   FnSign(none_));
            }

            // arrayview
            {
                arrayview_->MethodAdd("len",        FnSign(i32_));
                arrayview_->MethodAdd("to_array",   FnSign(array_));
            }

            // range
            {
                
            }

            // function
            {
                
            }
            */
        }

        // Cast
        {
            i32_->MethodAdd("cast", FnSign(i64_, { i32_ }, std::nullopt, FnModifier{ .hasCast_ = true }));
            i32_->MethodAdd("cast", FnSign(f32_, { i32_ }, std::nullopt, FnModifier{ .hasCast_ = true }));
            i32_->MethodAdd("cast", FnSign(f64_, { i32_ }, std::nullopt, FnModifier{ .hasCast_ = true }));

            i64_->MethodAdd("cast", FnSign(f32_, { i64_ }, std::nullopt, FnModifier{ .hasCast_ = true }));
            i64_->MethodAdd("cast", FnSign(f64_, { i64_ }, std::nullopt, FnModifier{ .hasCast_ = true }));

            f32_->MethodAdd("cast", FnSign(f64_, { f32_ }, std::nullopt, FnModifier{ .hasCast_ = true }));
        
            TypeTable::CastRecompute();
        }
    }

    Type* TypeTable::Set(const BasicType& type) {
        if (!table_.contains(std::string(type.name_))) {
            auto set = table_.emplace(
                type.name_, 
                new BasicType(type.name_, type.params_cnt_)
            );
            return set.first->second;
        }
        else throw LogErr(LogModule::Sema, std::format("existing type '{}'", type.name_));
    }

    Type* TypeTable::Set(const ParametricType& type) {
        if (!table_.contains(std::string(type.name_))) {
            auto set = table_.emplace(
                type.name_,
                new ParametricType(type)
            );
            return set.first->second;
        }
        else throw LogErr(LogModule::Sema, std::format("existing type '{}'", type.name_));
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
    
    Type* TypeTable::ParamTypeGet(const Type* type, const std::vector<const Type*>& params, std::optional<Loc> loc) {
        type->BasicTypeCheck();
        if (params.empty()) return Lookup(type->name_, loc);

        auto base_type = (BasicType*)type;

        if (base_type->params_cnt_ != params.size()) {
            throw LogErr(LogModule::Sema, std::format(
                "type '{}' expects {} type parameter(s), got {}",
                base_type->name_, base_type->params_cnt_, params.size()
            ), loc);
        }
        
        auto name = ParametricType::ParamsPrint(base_type, params);
        auto it   = table_.find(name);
        if (it != table_.end()) return it->second;

        auto parametric_type = Set(ParametricType(
            name, base_type, params
        ));
        CastRecompute();
        return parametric_type;
    }

    void TypeTable::CastRecompute() {
        for (auto& [type_name, type] : table_) {
            type->casts_.clear();
            type->casts_.emplace(type);
        }

        for (auto& [type_name, type] : table_) {
            if (type->type_using_ == Type::Using::Base) {
                auto base_type = (BasicType*)type;
                for (auto& [method_name, overloads] : base_type->methods_) {
                    for (auto& fnsign : overloads.fnsigns_) {
                        if (fnsign->modifier_.hasCast_ && fnsign->ret_type_) {
                            type->casts_.emplace(fnsign->ret_type_);
                        }
                    }
                }
            }
        }
    }
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <vector>

#include "sema/type.hpp"

namespace sema {
    
    // Type

    bool Type::isNone() const {
        return is("none");
    }

    bool Type::is(std::string_view name) const {
        if (this->name_ == name) return true;

        const Type* base_type = base();
        return base_type != this && base_type->is(name);
    }

    bool Type::isHeapStored() const {
        return base()->baseinfo().isHeapStored_;
    }

    void Type::BaseTypeCheck() const {
        if (usingtype_ != UsingType::Base) {
            throw LogErr(LogModule::Sema, std::format(
                "invalid base type '{}'", name_
            ));
        }
    }
    
    std::string Type::ParamsPrint(const Type* base, const std::vector<const Type*>& params) {
        base->BaseTypeCheck();
        return base->name_ + JoinWithBoundary(params, [](const Type* type) {
            return type->name_;
        }, "[=", "=]");
    }

    // TypeTable

    void TypeTable::Init() {

        // Info
        {
            TypeTable::Set(Type("none",         Type::BaseInfo{ .size_ = 0 }));
            TypeTable::Set(Type("bool",         Type::BaseInfo{ .size_ = 1 }));
            TypeTable::Set(Type("i32",          Type::BaseInfo{ .size_ = 4 }));
            TypeTable::Set(Type("i64",          Type::BaseInfo{ .size_ = 8 }));
            TypeTable::Set(Type("f32",          Type::BaseInfo{ .size_ = 4 }));
            TypeTable::Set(Type("f64",          Type::BaseInfo{ .size_ = 8 }));
            TypeTable::Set(Type("char",         Type::BaseInfo{ .size_ = 4 }));
            TypeTable::Set(Type("string",       Type::BaseInfo{ .size_ = 0, .isHeapStored_ = true }));
            TypeTable::Set(Type("stringview",   Type::BaseInfo{ .size_ = 0, .isHeapStored_ = true }));
            TypeTable::Set(Type("array",        Type::BaseInfo{ .size_ = 0, .isHeapStored_ = true, .params_cnt_ = 1 }));
            TypeTable::Set(Type("arrayview",    Type::BaseInfo{ .size_ = 0, .isHeapStored_ = true }));
            TypeTable::Set(Type("range",        Type::BaseInfo{ .size_ = 0, .isHeapStored_ = true, .params_cnt_ = 1 }));
            TypeTable::Set(Type("function",     Type::BaseInfo{ .size_ = 0, .isHeapStored_ = true }));
        }

        // Convert
        {
            auto* i32_ = TypeTable::Lookup("i32");
            auto* i64_ = TypeTable::Lookup("i64");
            auto* f32_ = TypeTable::Lookup("f32");
            auto* f64_ = TypeTable::Lookup("f64");

            TypeTable::ConvertSet(i32_, i64_);
            TypeTable::ConvertSet(i32_, f32_);
            TypeTable::ConvertSet(i32_, f64_);
            TypeTable::ConvertSet(i64_, f32_);
            TypeTable::ConvertSet(i64_, f64_);
            TypeTable::ConvertSet(f32_, f64_);
            
            auto* char_         = TypeTable::Lookup("char");
            auto* string_       = TypeTable::Lookup("string");
            auto* stringview_   = TypeTable::Lookup("stringview");

            TypeTable::ConvertSet(char_, string_);
            TypeTable::ConvertSet(stringview_, string_);

            auto* array_       = TypeTable::Lookup("array");
            auto* arrayview_   = TypeTable::Lookup("arrayview");

            TypeTable::ConvertSet(arrayview_, array_);
        
            TypeTable::ConvertsRecompute();
        }
    }

    Type* TypeTable::Set(const Type& t) {
        if (!table_.contains(std::string(t.name_))) {
            auto type = new Type(t);
            table_.emplace(t.name_, type);
            return type;
        }
        else throw LogErr(LogModule::Sema, std::format("existing type '{}'", t.name_));
    }
    
    Type* TypeTable::Lookup(std::string_view name, std::optional<Loc> loc) {
        auto type_it = table_.find(std::string(name));
        if (type_it != table_.end()) {
            return type_it->second;
        }

        throw LogErr(LogModule::Sema, std::format("undefined type '{}'", name), loc);
    }
    
    Type* TypeTable::SetOrGetTypeParam(const Type* base, const std::vector<const Type*>& params, std::optional<Loc> loc)
    {
        base->BaseTypeCheck();
        if (params.empty()) return Lookup(base->name_, loc);
        if (base->baseinfo().params_cnt_ != params.size()) {
            throw LogErr(LogModule::Sema, std::format(
                "type '{}' expects {} type parameter(s), got {}",
                base->name_, base->baseinfo().params_cnt_, params.size()
            ), loc);
        }
        
        auto name = Type::ParamsPrint(base, params);
        auto it   = table_.find(name);
        if (it != table_.end()) return it->second;

        auto type = Set(Type(
            name, Type::ParamInfo{
                .base_   = base,
                .params_ = params
            }
        ));
        ConvertsRecompute();
        return type;
    }
    
    void TypeTable::ConvertSet(const Type* from, const Type* to) {
        if (from == to) return;
        converts_.emplace(from, to);
    }

    void TypeTable::ConvertsRecompute() {

        // Clear
        for (auto& [name, type] : table_) {
            type->converts_.clear();
        }

        // Recompute
        for (auto& [name, type] : table_) {

            // Emplace itself
            type->converts_.emplace(type);

            // Search all path
            std::vector<const Type*> stack = { type };
            while (!stack.empty()) {
                auto* t = stack.back();
                stack.pop_back();

                for (auto& [from, to] : converts_) {
                    if (from == t) {
                        type->converts_.emplace(to);
                        stack.emplace_back(to);
                    }
                }
            }
        }
    }
}

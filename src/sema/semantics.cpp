
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <vector>

#include "semantics.hpp"

namespace sema {
    
    // Type

    bool Type::isNone() const {
        return this == TypeTable::Lookup("none");
    }

    bool Type::is(std::string_view name) const {
        return this == TypeTable::Lookup(name);
    }

    // TypeTable

    void TypeTable::Init() {

        // Info
        {
            TypeTable::Set(Type{ .name_ = "none",        .size_ = 0 });
            TypeTable::Set(Type{ .name_ = "bool",        .size_ = 1 });
            TypeTable::Set(Type{ .name_ = "i32",         .size_ = 4 });
            TypeTable::Set(Type{ .name_ = "i64",         .size_ = 8 });
            TypeTable::Set(Type{ .name_ = "f32",         .size_ = 4 });
            TypeTable::Set(Type{ .name_ = "f64",         .size_ = 8 });
            TypeTable::Set(Type{ .name_ = "char",        .size_ = 4 });
            TypeTable::Set(Type{ .name_ = "string",      .size_ = 0, .isHeapStored_ = true });
            TypeTable::Set(Type{ .name_ = "stringview",  .size_ = 0, .isHeapStored_ = true });
            TypeTable::Set(Type{ .name_ = "array",       .size_ = 0, .isHeapStored_ = true });
            TypeTable::Set(Type{ .name_ = "arrayview",   .size_ = 0, .isHeapStored_ = true });
            TypeTable::Set(Type{ .name_ = "range",       .size_ = 0, .isHeapStored_ = true });
            TypeTable::Set(Type{ .name_ = "function",    .size_ = 0, .isHeapStored_ = true });
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

    void TypeTable::ConvertSet(const Type* from, const Type* to) {
        if (from == to) {
            LogWarn(LogModule::Sema, std::format(
                "cannot convert type '{}' to itself",
                from->name_
            )).Print();
            return;
        }
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

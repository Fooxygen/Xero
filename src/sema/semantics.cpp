
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <vector>

#include "semantics.hpp"

namespace sema {
    
    // Type

    bool Type::isNone() const {
        return this == TypeTable::Get("none");
    }

    bool Type::is(std::string_view name) const {
        return this == TypeTable::Get(name);
    }

    // TypeTable

    void TypeTable::Init() {

        // Info
        {
            TypeTable::Set(Type{ .name = "none",        .size = 0 });
            TypeTable::Set(Type{ .name = "bool",        .size = 1 });
            TypeTable::Set(Type{ .name = "i32",         .size = 4 });
            TypeTable::Set(Type{ .name = "i64",         .size = 8 });
            TypeTable::Set(Type{ .name = "f32",         .size = 4 });
            TypeTable::Set(Type{ .name = "f64",         .size = 8 });
            TypeTable::Set(Type{ .name = "char",        .size = 4 });
            TypeTable::Set(Type{ .name = "string",      .size = 0, .isHeapStored = true });
            TypeTable::Set(Type{ .name = "stringview",  .size = 0, .isHeapStored = true });
            TypeTable::Set(Type{ .name = "array",       .size = 0, .isHeapStored = true });
            TypeTable::Set(Type{ .name = "arrayview",   .size = 0, .isHeapStored = true });
            TypeTable::Set(Type{ .name = "range",       .size = 0, .isHeapStored = true });
            TypeTable::Set(Type{ .name = "function",    .size = 0, .isHeapStored = true });
        }

        // Convert
        {
            auto* i32_ = TypeTable::Get("i32");
            auto* i64_ = TypeTable::Get("i64");
            auto* f32_ = TypeTable::Get("f32");
            auto* f64_ = TypeTable::Get("f64");

            TypeTable::ConvertSet(i32_, i64_);
            TypeTable::ConvertSet(i32_, f32_);
            TypeTable::ConvertSet(i32_, f64_);
            TypeTable::ConvertSet(i64_, f32_);
            TypeTable::ConvertSet(i64_, f64_);
            TypeTable::ConvertSet(f32_, f64_);
            
            auto* char_         = TypeTable::Get("char");
            auto* string_       = TypeTable::Get("string");
            auto* stringview_   = TypeTable::Get("stringview");

            TypeTable::ConvertSet(char_, string_);
            TypeTable::ConvertSet(stringview_, string_);

            auto* array_       = TypeTable::Get("array");
            auto* arrayview_   = TypeTable::Get("arrayview");

            TypeTable::ConvertSet(arrayview_, array_);
        
            TypeTable::ConvertsRecompute();
        }
    }

    void TypeTable::ConvertSet(const Type* from, const Type* to) {
        if (from == to) {
            LogWarn(LogModule::Sema, std::format(
                "cannot convert type '{}' to itself",
                from->name
            )).Print();
            return;
        }
        converts_.emplace(from, to);
    }

    void TypeTable::ConvertsRecompute() {

        // Clear
        for (auto& [name, type] : table_) {
            type->converts.clear();
        }

        // Recompute
        for (auto& [name, type] : table_) {

            // Emplace itself
            type->converts.emplace(type);

            // Search all path
            std::vector<const Type*> stack = { type };
            while (!stack.empty()) {
                auto* t = stack.back();
                stack.pop_back();

                for (auto& [from, to] : converts_) {
                    if (from == t) {
                        type->converts.emplace(to);
                        stack.emplace_back(to);
                    }
                }
            }
        }
    }
}

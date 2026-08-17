
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema.hpp"

namespace sema {

    void Sema::BuiltinFnRegister() {
        using TS = std::vector<const Type*>;
        
        auto none_   = TypeTable::Get("none");
        auto string_ = TypeTable::Get("string");

        // IO
        {
            symbol_table_.Declare(std::make_unique<FnSymbol>(
                "print", Loc(), none_, TS{}, nullptr
            ));
            symbol_table_.Declare(std::make_unique<FnSymbol>(
                "println", Loc(), none_, TS{}, nullptr
            ));
        }

        // Reflection
        {
            symbol_table_.Declare(std::make_unique<FnSymbol>(
                "type", Loc(), string_, TS{ nullptr }
            ));
        }
    }

    void Sema::BuiltinMethodRegister() {
        using TS = std::vector<const Type*>;

        auto none_        = TypeTable::Get("none");
        auto i32_         = TypeTable::Get("i32");
        auto string_      = TypeTable::Get("string");
        auto stringview_  = TypeTable::Get("stringview");
        auto array_       = TypeTable::Get("array");
        auto arrayview_   = TypeTable::Get("arrayview");

        // array
        {
            MethodSymbolTable::Set(array_, FnSymbol("len", Loc(), i32_));
            MethodSymbolTable::Set(array_, FnSymbol("clear", Loc(), none_));
            MethodSymbolTable::Set(array_, FnSymbol("insert", Loc(), none_, TS{ i32_, nullptr }));
            MethodSymbolTable::Set(array_, FnSymbol("remove", Loc(), none_, TS{ i32_ }));
            MethodSymbolTable::Set(array_, FnSymbol("push_front", Loc(), none_, TS{ nullptr }));
            MethodSymbolTable::Set(array_, FnSymbol("pop_front", Loc(), none_));
            MethodSymbolTable::Set(array_, FnSymbol("push_back", Loc(), none_, TS{ nullptr }));
            MethodSymbolTable::Set(array_, FnSymbol("pop_back", Loc(), none_));
        }

        // arrayview
        {
            MethodSymbolTable::Set(arrayview_, FnSymbol("len", Loc(), i32_));
            MethodSymbolTable::Set(arrayview_, FnSymbol("to_array", Loc(), array_));
        }

        // string
        {
            MethodSymbolTable::Set(string_, FnSymbol("len", Loc(), i32_));
            MethodSymbolTable::Set(string_, FnSymbol("clear", Loc(), none_));
        }

        // stringview
        {
            MethodSymbolTable::Set(stringview_, FnSymbol("len", Loc(), i32_));
            MethodSymbolTable::Set(stringview_, FnSymbol("to_string", Loc(), string_));
        }
    }
}

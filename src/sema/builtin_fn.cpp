
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/sema.hpp"
#include "sema/method.hpp"

namespace sema {

    void Sema::BuiltinFnSymbolRegister() {
        // nullptr: any type
        using TS = std::vector<const Type*>;
        
        auto none_   = TypeTable::Lookup("none");
        auto string_ = TypeTable::Lookup("string");

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

    void MethodTable::Register() {
        using TS = std::vector<const Type*>;

        auto none_        = TypeTable::Lookup("none");
        auto i32_         = TypeTable::Lookup("i32");
        auto string_      = TypeTable::Lookup("string");
        auto stringview_  = TypeTable::Lookup("stringview");
        auto array_       = TypeTable::Lookup("array");
        auto arrayview_   = TypeTable::Lookup("arrayview");

        // array
        {
            Set(array_, FnSymbol("len",         Loc(), i32_));
            Set(array_, FnSymbol("clear",       Loc(), none_));
            Set(array_, FnSymbol("insert",      Loc(), none_, TS{ i32_, nullptr }));
            Set(array_, FnSymbol("remove",      Loc(), none_, TS{ i32_ }));
            Set(array_, FnSymbol("push_front",  Loc(), none_, TS{ nullptr }));
            Set(array_, FnSymbol("pop_front",   Loc(), none_));
            Set(array_, FnSymbol("push_back",   Loc(), none_, TS{ nullptr }));
            Set(array_, FnSymbol("pop_back",    Loc(), none_));
        }

        // arrayview
        {
            Set(arrayview_, FnSymbol("len",         Loc(), i32_));
            Set(arrayview_, FnSymbol("to_array",    Loc(), array_));
        }

        // string
        {
            Set(string_, FnSymbol("len",    Loc(), i32_));
            Set(string_, FnSymbol("clear",  Loc(), none_));
        }

        // stringview
        {
            Set(stringview_, FnSymbol("len",        Loc(), i32_));
            Set(stringview_, FnSymbol("to_string",  Loc(), string_));
        }
    }
}

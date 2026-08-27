
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/sign.hpp"
#include "sema/sema.hpp"

namespace sema {

    // Fn

    void Sema::BuiltinFnRegister() {
        // nullptr: any type
        using TS = std::vector<const Type*>;
        
        auto none_ = TypeTable::Lookup("none");

        // IO
        {
            symbol_table_.Declare(std::make_unique<FnSymbol>(
                "print", Loc(), FnSign(none_, TS{}, nullptr)
            ));
            symbol_table_.Declare(std::make_unique<FnSymbol>(
                "println", Loc(), FnSign(none_, TS{}, nullptr)
            ));
        }
    }
}

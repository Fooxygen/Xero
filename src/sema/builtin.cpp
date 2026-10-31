
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/analyzer.hpp"

namespace sema {

    // Built-in Function

    void Analyzer::BuiltinFnRegister() {
        
        auto none_ = TypeTable::Lookup("none");

        // IO
        {
            fn_scope_.Add("print",   FnSign(none_, {}, nullptr));
            fn_scope_.Add("println", FnSign(none_, {}, nullptr));
        }
    }
}

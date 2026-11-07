
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/analyzer.hpp"

namespace sema {

    // Built-in Function

    void Analyzer::BuiltinFnRegister() {
        
        auto none_ = TypeTable::Lookup("none");
        auto f32_  = TypeTable::Lookup("f32");
        auto f64_  = TypeTable::Lookup("f64");

        // IO
        {
            fn_scope_.Add("print",   FnSign(none_, {}, nullptr));
            fn_scope_.Add("println", FnSign(none_, {}, nullptr));
        }

        // Math
        {
            fn_scope_.Add("@sqrt",  FnSign(f32_, { f32_ }));
            fn_scope_.Add("@sqrt",  FnSign(f64_, { f64_ }));

            fn_scope_.Add("@pow",   FnSign(f32_, { f32_, f32_ }));
            fn_scope_.Add("@pow",   FnSign(f64_, { f64_, f64_ }));

            fn_scope_.Add("@exp",   FnSign(f32_, { f32_ }));
            fn_scope_.Add("@exp",   FnSign(f64_, { f64_ }));

            fn_scope_.Add("@ln",    FnSign(f32_, { f32_ }));
            fn_scope_.Add("@ln",    FnSign(f64_, { f64_ }));

            fn_scope_.Add("@log2",  FnSign(f32_, { f32_ }));
            fn_scope_.Add("@log2",  FnSign(f64_, { f64_ }));

            fn_scope_.Add("@log10", FnSign(f32_, { f32_ }));
            fn_scope_.Add("@log10", FnSign(f64_, { f64_ }));

            fn_scope_.Add("@sin",   FnSign(f32_, { f32_ }));
            fn_scope_.Add("@sin",   FnSign(f64_, { f64_ }));

            fn_scope_.Add("@cos",   FnSign(f32_, { f32_ }));
            fn_scope_.Add("@cos",   FnSign(f64_, { f64_ }));

            fn_scope_.Add("@tan",   FnSign(f32_, { f32_ }));
            fn_scope_.Add("@tan",   FnSign(f64_, { f64_ }));
        }
    }
}


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
            fn_scope_.Add("__abs",   FnSign(f32_, { f32_ }));
            fn_scope_.Add("__abs",   FnSign(f64_, { f64_ }));

            fn_scope_.Add("__min",   FnSign(f32_, { f32_, f32_ }));
            fn_scope_.Add("__min",   FnSign(f64_, { f64_, f64_ }));

            fn_scope_.Add("__max",   FnSign(f32_, { f32_, f32_ }));
            fn_scope_.Add("__max",   FnSign(f64_, { f64_, f64_ }));

            fn_scope_.Add("__trunc", FnSign(f32_, { f32_ }));
            fn_scope_.Add("__trunc", FnSign(f64_, { f64_ }));

            fn_scope_.Add("__floor", FnSign(f32_, { f32_ }));
            fn_scope_.Add("__floor", FnSign(f64_, { f64_ }));

            fn_scope_.Add("__ceil",  FnSign(f32_, { f32_ }));
            fn_scope_.Add("__ceil",  FnSign(f64_, { f64_ }));

            fn_scope_.Add("__round", FnSign(f32_, { f32_ }));
            fn_scope_.Add("__round", FnSign(f64_, { f64_ }));

            fn_scope_.Add("__sqrt",  FnSign(f32_, { f32_ }));
            fn_scope_.Add("__sqrt",  FnSign(f64_, { f64_ }));

            fn_scope_.Add("__pow",   FnSign(f32_, { f32_, f32_ }));
            fn_scope_.Add("__pow",   FnSign(f64_, { f64_, f64_ }));

            fn_scope_.Add("__exp",   FnSign(f32_, { f32_ }));
            fn_scope_.Add("__exp",   FnSign(f64_, { f64_ }));

            fn_scope_.Add("__ln",    FnSign(f32_, { f32_ }));
            fn_scope_.Add("__ln",    FnSign(f64_, { f64_ }));

            fn_scope_.Add("__log2",  FnSign(f32_, { f32_ }));
            fn_scope_.Add("__log2",  FnSign(f64_, { f64_ }));

            fn_scope_.Add("__log10", FnSign(f32_, { f32_ }));
            fn_scope_.Add("__log10", FnSign(f64_, { f64_ }));

            fn_scope_.Add("__sin",   FnSign(f32_, { f32_ }));
            fn_scope_.Add("__sin",   FnSign(f64_, { f64_ }));

            fn_scope_.Add("__cos",   FnSign(f32_, { f32_ }));
            fn_scope_.Add("__cos",   FnSign(f64_, { f64_ }));

            fn_scope_.Add("__tan",   FnSign(f32_, { f32_ }));
            fn_scope_.Add("__tan",   FnSign(f64_, { f64_ }));
        }
    }
}

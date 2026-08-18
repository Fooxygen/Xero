
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <variant>

#include "common/ast.hpp"

namespace rt {
    class Obj;
    using NativeFn = Obj(*)(std::vector<Obj>&);
    using LangFn   = std::shared_ptr<FnExpr>;
    
    class Function {
    public:
        enum class UsingType {
            Native, Lang
        };
    
    private:
        UsingType usingtype_ = UsingType::Lang;
        std::variant<LangFn, NativeFn> impl_{LangFn{}};

    public:
        Function() {}
        Function(NativeFn fn) : usingtype_(UsingType::Native) {
            impl_.emplace<NativeFn>(fn);
        }
        Function(LangFn fn)   : usingtype_(UsingType::Lang) {
            impl_.emplace<LangFn>(std::move(fn));
        }

        UsingType     usingtype() const { return usingtype_; }
        NativeFn      native()    const { return std::get<NativeFn>(impl_); }
        const LangFn& lang()      const { return std::get<LangFn>(impl_); }
    };
}

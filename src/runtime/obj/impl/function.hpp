
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <variant>

#include "common/ast.hpp"

namespace rt {
    class Obj;
    using Fn = Obj(*)(std::vector<Obj>&);
    
    class Function {
    private:
        std::variant<Fn, std::shared_ptr<FnExpr>> impl_;

    public:
        Function() {}
        Function(Fn fn)
        :   impl_(fn) {}
        Function(std::shared_ptr<FnExpr> expr)
        :   impl_(std::move(expr)) {}

        bool isNative() const { return std::holds_alternative<Fn>(impl_); }     // cpp lambda
        Fn   native()   const { return std::get<Fn>(impl_); }
        const std::shared_ptr<FnExpr>& expr() const {                           // xero astnode
            return std::get<std::shared_ptr<FnExpr>>(impl_);
        }
    };
}

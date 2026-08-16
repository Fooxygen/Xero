
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "common/ast.hpp"

namespace rt {

    class Function {
    private:
        std::shared_ptr<FnExpr> expr_;

    public:
        Function() {}
        Function(std::shared_ptr<FnExpr> expr)
        :   expr_(std::move(expr)) {}

        const std::shared_ptr<FnExpr>& expr() const { return expr_; }

        void Check(
            const std::string& name,
            const std::vector<const sema::Type*>& args_type)
        {
            
            // Empty
            if (!expr_) throw LogErr(LogModule::Runtime, "call empty function");

            // Number of Params 
            auto& params = expr_->params_->exprs_;
            if (args_type.size() != params.size()) {
                throw LogErr(LogModule::Runtime, std::format(
                    "cannot call '{}()' with mismatched argument count", name
                ));
            }

            for (size_t i = 0; i < params.size(); i++) {
                
                // Type of Expr
                if (params[i]->type_ != AstType::DeclExpr) {
                    throw LogErr(LogModule::Runtime, std::format(
                        "invalid expression at arguments[{}]", i
                    ));
                }

                auto  param = (DeclExpr*)params[i].get();
                auto  type_param = sema::TypeTable::Get(param->bind_type_);
                auto& type_arg   = args_type[i];
                if (!type_arg->converts.contains(type_param)) {
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with '{}'",
                        type_arg->name, type_param->name
                    ));
                }
            }
        }
    };
}

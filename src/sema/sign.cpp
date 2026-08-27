
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/type.hpp"
#include "sema/sema.hpp"

namespace sema {

    // FnSign

    std::string FnSign::ParamsPrint() {
        std::string res = "";

        for (size_t i = 0; i < params_fix_.size(); i++) {
            if (i != 0) res += ", ";
            res += params_fix_[i] ? params_fix_[i]->name_ : "any";
        }

        if (!params_fix_.empty() && param_inf_) res += ", ";
        if (param_inf_) {
            res += *param_inf_ ? (*param_inf_)->name_ : "any";
            res += "...";
        }

        return res.empty() ? "(empty)" : '(' + res + ')';
    }

    bool FnSign::CallCheck(const std::vector<const Type*>& args_type) {
        try {
            // Fixed
            if (args_type.size() < params_fix_.size()) throw 0;
            for (size_t i = 0; i < params_fix_.size(); i++) {
                if (params_fix_[i] && !args_type[i]->casts_.contains(params_fix_[i])) throw 0;
            }

            // Infinite
            if (param_inf_) {
                for (size_t i = params_fix_.size(); i < args_type.size(); i++) {
                    if (*param_inf_ && !args_type[i]->casts_.contains(*param_inf_)) throw 0;
                }
            }
            else {
                if (args_type.size() > params_fix_.size()) throw 0;
            }
        }
        catch (...) {
            return false;
        }
        
        return true;
    }
}

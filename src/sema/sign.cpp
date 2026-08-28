
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/type.hpp"
#include "sema/sema.hpp"

namespace sema {

    // FnSign

    std::string FnSign::ParamsPrint() const {
        std::string res = "";

        for (size_t i = 0; i < params_type_fix_.size(); i++) {
            if (i != 0) res += ", ";
            res += params_type_fix_[i] ? params_type_fix_[i]->name() : "any";
        }

        if (!params_type_fix_.empty() && params_type_inf_) res += ", ";
        if (params_type_inf_) {
            res += *params_type_inf_ ? (*params_type_inf_)->name() : "any";
            res += "...";
        }

        return res.empty() ? "(empty)" : '(' + res + ')';
    }

    bool FnSign::isSignEqual(const FnSign& sign) {
        if (sign.ret_type_ != ret_type_) return false;

        if (sign.params_type_fix_.size() != params_type_fix_.size()) return false;
        for (size_t i = 0; i < params_type_fix_.size(); i++) {
            if (sign.params_type_fix_[i] != params_type_fix_[i]) return false;
        }

        if (sign.params_type_inf_ != params_type_inf_) return false;

        if (sign.modifier_ != modifier_) return false;

        return true;
    }

    bool FnSign::isCallMatch(const std::vector<Type*>& args_type) {

        try {
            // Fix
            if (args_type.size() < params_type_fix_.size()) throw 0;
            for (size_t i = 0; i < params_type_fix_.size(); i++) {
                if (params_type_fix_[i] && !args_type[i]->casts().contains(params_type_fix_[i])) throw 0;
            }

            // Inf
            if (params_type_inf_) {
                for (size_t i = params_type_fix_.size(); i < args_type.size(); i++) {
                    if (*params_type_inf_ && !args_type[i]->casts().contains(*params_type_inf_)) throw 0;
                }
            }
            else {
                if (args_type.size() > params_type_fix_.size()) throw 0;
            }
        }
        catch (...) {
            return false;
        }
        
        return true;
    }
}

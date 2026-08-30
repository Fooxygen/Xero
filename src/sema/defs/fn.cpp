
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"

namespace sema {

    // FnSign

    std::string FnSign::ParamsPrint() const {
        std::string res = "";

        // Fixed Params
        for (size_t i = 0; i < params_type_fix_.size(); i++) {
            if (i != 0) res += ", ";
            res += params_type_fix_[i] ? params_type_fix_[i]->name() : "any";
        }

        // Variable Params
        if (!params_type_fix_.empty() && params_type_var_) res += ", ";
        if (params_type_var_) {
            res += *params_type_var_ ? (*params_type_var_)->name() : "any";
            res += "...";
        }

        return res.empty() ? "(empty)" : '(' + res + ')';
    }

    bool FnSign::isSignEqual(const FnSign& sign) {

        // Return Type
        if (sign.return_type_ != return_type_) return false;

        // Params Fixed Type
        if (sign.params_type_fix_.size() != params_type_fix_.size()) return false;
        for (size_t i = 0; i < params_type_fix_.size(); i++) {
            if (sign.params_type_fix_[i] != params_type_fix_[i]) return false;
        }

        // Params Variable Type
        if (sign.params_type_var_ != params_type_var_) return false;

        // Modifier
        if (sign.modifier_ != modifier_) return false;

        return true;
    }

    bool FnSign::isSignMatch(const std::vector<Type*>& args_type) {

        try {
            // Fixed Params
            if (args_type.size() < params_type_fix_.size()) throw 0;
            for (size_t i = 0; i < params_type_fix_.size(); i++) {
                if (params_type_fix_[i] && !args_type[i]->casts().contains(params_type_fix_[i])) throw 0;
            }

            // Variable Params
            if (params_type_var_) {
                for (size_t i = params_type_fix_.size(); i < args_type.size(); i++) {
                    if (*params_type_var_ && !args_type[i]->casts().contains(*params_type_var_)) throw 0;
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

    // Fn

    const FnSign* Fn::SignLookup(const FnSign& sign) const {
        for (auto& s : signs_) {
            if (s->isSignEqual(sign)) return s.get();
        }
        throw LogErr(LogModule::Sema, std::format(
            "undefined signature {} for function '{}'",
            sign.ParamsPrint(), name_
        ));
    }
    
    const FnSign* Fn::SignLookup(const std::vector<Type*>& args_type) {
        for (auto& s : signs_) {
            if (s->isSignMatch(args_type)) return s.get();
        }
        throw LogErr(LogModule::Sema, std::format(
            "undefined signature {} for function '{}'",
            FnSign(nullptr, args_type).ParamsPrint(), name_
        ));
    }
    
    const FnSign* Fn::SignLookupTry(const FnSign& sign) const {
        for (auto& s : signs_) {
            if (s->isSignEqual(sign)) return s.get();
        }
        return nullptr;
    }
    
    const FnSign* Fn::SignLookupTry(const std::vector<Type*>& args_type) {
        for (auto& s : signs_) {
            if (s->isSignMatch(args_type)) return s.get();
        }
        return nullptr;
    }

    const FnSign* Fn::SignAdd(const std::string& name, const FnSign& sign) {
        if (auto s = SignLookupTry(sign)) return s;

        auto sign_add = signs_.emplace_back(std::make_unique<FnSign>(sign)).get();
        sign_add->NameSet(name);
        return sign_add;
    }
}

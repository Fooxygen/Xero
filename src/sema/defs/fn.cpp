
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"

namespace sema {

    // FnSign

    std::string FnSign::ParamsPrint(const std::vector<Type*>& params_type_fix, std::optional<Type*> params_type_var) {
        std::string res = "";

        // Fixed Params
        for (size_t i = 0; i < params_type_fix.size(); i++) {
            if (i != 0) res += ", ";
            res += params_type_fix[i] ? params_type_fix[i]->name() : "any";
        }

        // Variable Params
        if (!params_type_fix.empty() && params_type_var) res += ", ";
        if (params_type_var) {
            res += *params_type_var ? (*params_type_var)->name() : "any";
            res += "...";
        }

        return res.empty() ? "(empty)" : '(' + res + ')';
    }

    std::string FnSign::ParamsPrint() const {
        return ParamsPrint(params_type_fix_, params_type_var_);
    }

    bool        FnSign::IsSignEqual(const FnSign& sign) {

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

    std::optional<FnSign::Match>
                FnSign::SignMatch(const std::vector<Type*>& args_type)
    {
        Match match(this);

        auto ParamLevelGet = [](sema::Type* param, sema::Type* arg, Match& match) -> bool {

            // Unconstrained: any type
            if (!param) {
                match.match_params().Add(MatchParams::Level::Unconstrained);
            }

            else {
                // Reference Param
                if (dynamic_cast<ReferenceType*>(param)) {
                    // Exact: i32& -> i32&
                    if (arg != param) return false;
                    match.match_params().Add(MatchParams::Level::Exact);
                }
                
                // Non-Reference Param
                else {
                    auto arg_unwrap = arg->ReferenceUnwrap();

                    // Exact: i32 -> i32
                    if (arg == param) {
                        match.match_params().Add(MatchParams::Exact);
                    }

                    // Deref: i32& -> i32
                    else if (arg_unwrap == param) {
                        match.match_params().Add(MatchParams::Deref);
                    }

                    // EqBase: range[=i32=] -> range
                    else if (arg_unwrap->Is(param->name())) {
                        match.match_params().Add(MatchParams::EqBase);
                    }

                    // Cast
                    else if (arg_unwrap->casts().contains(param)) {
                        // Cast After Deref: i32& -> f64
                        if (dynamic_cast<ReferenceType*>(arg)) {
                            match.match_params().Add(MatchParams::Cast_After_Deref);
                        }

                        // Cast: i32 -> f64
                        else {
                            match.match_params().Add(MatchParams::Cast);
                        }
                    }

                    else return false;
                }
            }

            return true;
        };

        // Fixed Params
        if (args_type.size() < params_type_fix_.size()) return {};
        for (size_t i = 0; i < params_type_fix_.size(); i++) {
            auto param = params_type_fix_[i];
            auto arg   = args_type[i];
            if (!ParamLevelGet(param, arg, match)) return std::nullopt;
        }

        // Variable Params
        if (!params_type_var_ && args_type.size() > params_type_fix_.size()) return std::nullopt;
        for (size_t i = params_type_fix_.size(); i < args_type.size(); i++) {
            auto param = *params_type_var_;
            auto arg   = args_type[i];
            if (!ParamLevelGet(param, arg, match)) return std::nullopt;
        }

        return match;
    }

    bool        FnSign::IsCallerMatch(Type* type) const {
        if (!caller_type_ || !type) return false;
        return caller_type_->BasicTypeGet() == type->BasicTypeGet();
    }

    // Fn

    const FnSign* Fn::SignLookup(const FnSign& sign_reference, std::optional<Loc> loc) const {
        auto sign = SignLookupTry(sign_reference);
        if (!sign) {
            throw LogErr(LogStage::Sema, std::format(
                "undefined signature {} for function '{}'",
                sign_reference.ParamsPrint(), name_
            ), loc);
        }
        return sign;
    }
    
    const FnSign* Fn::SignLookup(const std::vector<Type*>& args_type, std::optional<Loc> loc) {
        
        if (auto sign = SignLookupTry(args_type)) {
            if (*sign) return *sign;
            else {
                throw LogErr(LogStage::Sema, std::format(
                    "ambiguous signature {} for method '{}'",
                    FnSign(nullptr, args_type).ParamsPrint(), name_
                ), loc);
            }
        }

        throw LogErr(LogStage::Sema, std::format(
            "undefined signature {} for method '{}'",
            FnSign(nullptr, args_type).ParamsPrint(), name_
        ), loc);
    }
    
    const FnSign* Fn::SignLookup(Type* caller_type, const std::vector<Type*>& args_type, std::optional<Loc> loc) {

        if (auto sign = SignLookupTry(caller_type, args_type)) {
            if (*sign) return *sign;
            else {
                throw LogErr(LogStage::Sema, std::format(
                    "ambiguous signature {} for method '{}'",
                    FnSign(nullptr, args_type).ParamsPrint(), name_
                ), loc);
            }
        }

        throw LogErr(LogStage::Sema, std::format(
            "undefined signature {} for method '{}'",
            FnSign(nullptr, args_type).ParamsPrint(), name_
        ), loc);
    }

    const FnSign* Fn::SignLookupTry(const FnSign& sign_reference) const {
        for (auto& s : signs_) {
            if (s->IsSignEqual(sign_reference)) return s.get();
        }
        return nullptr;
    }
    
    std::optional<const FnSign*>
                  Fn::SignLookupTry(const std::vector<Type*>& args_type) {
        
        std::vector<FnSign::Match> matches = {};
        for (auto& s : signs_) {
            if (auto match = s->SignMatch(args_type)) {
                matches.emplace_back(std::move(*match));
            }
        }
        if (matches.empty()) return std::nullopt;
        if (matches.size() == 1) return matches[0].sign();

        // Best
        for (size_t i = 0; i < matches.size(); i++) {
            bool is_best = true;

            for (size_t j = 0; j < matches.size(); j++) {
                if (i == j) continue;
                if (!(matches[i].match_params() > matches[j].match_params())) {
                    is_best = false;
                    break;
                }
            }

            if (is_best) return matches[i].sign();
        }

        return nullptr;
    }

    std::optional<const FnSign*>
                  Fn::SignLookupTry(Type* caller_type, const std::vector<Type*>& args_type) {

        std::vector<FnSign::Match> matches = {};
        for (auto& s : signs_) {
            if (!s->IsCallerMatch(caller_type)) continue;
            if (auto match = s->SignMatch(args_type)) {
                matches.emplace_back(std::move(*match));
            }
        }
        if (matches.empty()) return std::nullopt;
        if (matches.size() == 1) return matches[0].sign();

        // Best
        for (size_t i = 0; i < matches.size(); i++) {
            bool is_best = true;
            for (size_t j = 0; j < matches.size(); j++) {
                if (i == j) continue;
                if (!(matches[i].match_params() > matches[j].match_params())) {
                    is_best = false;
                    break;
                }
            }
            if (is_best) return matches[i].sign();
        }

        return nullptr;
    }

    const FnSign* Fn::SignAdd(const std::string& name, const FnSign& sign_reference) {
        if (auto sign = SignLookupTry(sign_reference)) return sign;

        auto sign = signs_.emplace_back(std::make_unique<FnSign>(sign_reference)).get();
        sign->NameSet(name);
        return sign;
    }
}

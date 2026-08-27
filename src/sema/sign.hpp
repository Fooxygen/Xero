
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace sema {
    class Type;
    
    struct FnModifier {
        bool hasCast_ = false;

        bool operator ==(const FnModifier& modifier) {
            return modifier.hasCast_ == hasCast_;
        }
    };

    class  FnSign {
    public:
        const Type*                ret_type_   = nullptr;
        std::vector<const Type*>   params_fix_ = {};                // nullptr: any type
        std::optional<const Type*> param_inf_  = std::nullopt;
        FnModifier                 modifier_   = FnModifier{};

        FnSign(
            const Type* ret_type,
            const std::vector<const Type*>& params_fix = {},
            std::optional<const Type*>      param_inf  = std::nullopt,
            FnModifier modifier = FnModifier{}
        )
        :   ret_type_(ret_type),
            params_fix_(params_fix),
            param_inf_(param_inf),
            modifier_(modifier)
        {}

        std::string ParamsPrint();

        bool CallCheck(const std::vector<const Type*>& args_type);

        bool operator ==(const FnSign& sign) {
            if (sign.ret_type_ != ret_type_) return false;

            if (sign.params_fix_.size() != params_fix_.size()) return false;
            for (size_t i = 0; i < params_fix_.size(); i++) {
                if (sign.params_fix_[i] != params_fix_[i]) return false;
            }

            if (sign.param_inf_ != param_inf_) return false;

            if (sign.modifier_ != modifier_) return false;

            return true;
        }
    };

    class  FnOverloads {
    public:
        std::vector<std::unique_ptr<FnSign>> fnsigns_;

        const FnSign* Match(const FnSign& sign) const {
            for (auto& fnsign : fnsigns_) {
                if (*fnsign == sign) return fnsign.get();
            }
            return nullptr;
        }
    };
}

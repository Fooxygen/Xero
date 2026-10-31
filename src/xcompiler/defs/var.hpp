
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <format>
#include <vector>
#include <unordered_map>

#include "llvm/IR/Value.h"

#include "common/log.hpp"
#include "common/defs/scope.hpp"

namespace xcompiler {

    class SlotScope {
    private:
        Scope<llvm::Value*> scope_;

    public:
        void Push() { scope_.Push(); }
        void Pop()  { scope_.Pop(); }

        void Declare(const std::string& name, llvm::Value* value) {
            scope_.Set(name, value);
        }

        llvm::Value* Lookup(const std::string& name, std::optional<Loc> loc = std::nullopt) {
            auto value = scope_.LookupTry(name);
            if (!value) {
                throw LogErr(LogStage::Xcompiler, std::format(
                    "undefined variable '{}'", name
                ), loc);
            }
            return *value;
        }
    };
}

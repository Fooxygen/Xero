
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <format>
#include <vector>
#include <unordered_map>

#include "llvm/IR/Value.h"

#include "common/log.hpp"

namespace xcompiler {

    class ValueTable {
    private:
        std::vector<std::unordered_map<std::string, llvm::Value*>> scopes_;

    public:
        void ScopePush() { scopes_.emplace_back(); }
        void ScopePop()  { scopes_.pop_back(); }

        void Declare(const std::string& name, llvm::Value* value) {
            scopes_.back()[name] = value;
        }

        llvm::Value* Lookup(const std::string& name, std::optional<Loc> loc = std::nullopt) {
            for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
                auto oit = it->find(name);
                if (oit != it->end()) return oit->second;
            }
            throw LogErr(LogModule::Xcompiler, std::format(
                "undefined variable '{}'", name
            ), loc);
        }
    };
}

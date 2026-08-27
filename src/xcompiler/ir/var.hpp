
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

    class VarTable {
    private:
        std::vector<std::unordered_map<std::string, llvm::Value*>> scopes_;

    public:
        void ScopePush() { scopes_.emplace_back(); }
        void ScopePop()  { scopes_.pop_back(); }

        void Declare(const std::string& name, llvm::Value* value) { scopes_.back()[name] = value; }

        llvm::Value* Lookup(const std::string& name, std::optional<Loc> loc = std::nullopt) {
            for (auto scp_it = scopes_.rbegin(); scp_it != scopes_.rend(); ++scp_it) {
                auto val_it = scp_it->find(name);
                if (val_it != scp_it->end()) return val_it->second;
            }
            throw LogErr(LogModule::Xcompiler, std::format(
                "undefined variable '{}'", name
            ), loc);
        }
    };
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <unordered_map>

#include "llvm/IR/Value.h"

#include "common/ast.hpp"

namespace xcompiler {
    class IRGen;

    // LibC

    llvm::Function* LibC_printf(IRGen& gen);

    // Builtin Fn

    using BuiltinFn = llvm::Value* (*)(IRGen&, FnCallExpr&);

    class BuiltinFnTable {
    private:
        static inline std::unordered_map<std::string, BuiltinFn> table_;

    public:
        static void Register();
        static void Set(const std::string& name, BuiltinFn fn) { table_.emplace(name, fn); }
        
        static BuiltinFn* LookupTry(const std::string& name) {
            auto it = table_.find(name);
            return it == table_.end() ? nullptr : &it->second;
        }
        static BuiltinFn* Lookup(const std::string& name) {
            auto it = table_.find(name);
            if (it == table_.end()) {
                throw LogErr(LogModule::Xcompiler, std::format(
                    "undefined function '{}'", name
                ));
            }
            return &it->second;
        }
    };
}

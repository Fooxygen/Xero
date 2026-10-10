
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>

#include "common/defs/ast.hpp"
#include "context/module.hpp"
#include "sema/defs/type.hpp"
#include "sema/analyzer.hpp"

namespace sema {

    class Sema {
    private:
        Analyzer analyzer_;

    public:
        Sema(FnTable& fn_table) : analyzer_(fn_table) {}

    public:
        void Run(std::vector<context::Module>& modules) {
            
            // TypeTable
            TypeTable::Init();

            // Builtin
            analyzer_.BuiltinFnRegister();

            // AstNode
            for (auto& module : modules) analyzer_.Declare(*module.root());
            for (auto& module : modules) {
                analyzer_.Process(*module.root());
                LogBuild(LogStage::Sema, module.name()).Print();
            }
        }
    };
}

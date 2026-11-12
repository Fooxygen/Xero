
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
        Sema(FnScope& fn_scope) : analyzer_(fn_scope) {}

    public:
        void Run(std::vector<context::Module>& modules) {

            // TypeTable
            TypeTable::Init();

            // Builtin
            analyzer_.BuiltinFnRegister();

            // AstNode
            for (auto& module : modules) {
                analyzer_.ModuleSet(module);
                analyzer_.fn_scope().Enter(module.namespaces());
                analyzer_.type_scope().Enter(module.namespaces());
                analyzer_.module_const_scope().Enter(module.namespaces());
                analyzer_.Declare(*module.root());
            }
            for (auto& module : modules) {
                analyzer_.ModuleSet(module);
                analyzer_.fn_scope().Enter(module.namespaces());
                analyzer_.type_scope().Enter(module.namespaces());
                analyzer_.module_const_scope().Enter(module.namespaces());
                analyzer_.Process(*module.root());
                LogBuild(LogStage::Sema, module.name_full()).Print();
            }
        }
    };
}

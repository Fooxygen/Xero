
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "common/defs/ast.hpp"
#include "sema/defs/type.hpp"
#include "sema/analyzer.hpp"

namespace sema {

    class Sema {
    private:
        Analyzer analyzer_;

    public:
        Analyzer& analyzer() { return analyzer_; }

    public:
        void Run(AstNode& node) {
            
            // TypeTable
            TypeTable::Init();

            // Analyzer
            analyzer_.Exec(node);
        }
    };
}

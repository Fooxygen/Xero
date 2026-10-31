
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>

#include "common/config.hpp"
#include "context/module.hpp"
#include "sema/defs/fn.hpp"

namespace context {

    class Context {
    private:
        Config              config_;
        std::vector<Module> modules_ = {};

        // Sema

        sema::FnScope fn_scope_;

    public:
        Config&              config()  { return config_; }
        std::vector<Module>& modules() { return modules_; }
        
        sema::FnScope&       fn_scope() { return fn_scope_; }
    
    public:
        void ModulesProjectLoad();                  // load project modules from config

        void ModuleDependencyLoad(Module& module);  // search dependency of module and load
    };
}

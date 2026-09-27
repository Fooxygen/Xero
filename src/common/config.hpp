
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <filesystem>

#include "llvm/Passes/OptimizationLevel.h"

struct ProjectConfig {

    struct Profile {
        std::string name_ = "";
        llvm::OptimizationLevel opt_level_ = llvm::OptimizationLevel::O0;
    };

    struct Build {
        std::filesystem::path path_ = "";
        bool emit_ir_  = true;
    };

    struct Diag {
        bool is_print_tokens_ = false;
        bool is_print_ast_    = false;
    };

    std::string name_ = "";
    std::filesystem::path root_;
    std::filesystem::path entry_;

    Profile profile_;
    Build   build_;
    Diag    diag_;
};

struct Config {
    ProjectConfig project_;
};


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <filesystem>

#include "llvm/Support/Program.h"

#include "common/config.hpp"
#include "common/log.hpp"
#include "common/defs/ast.hpp"
#include "sema/defs/fn.hpp"
#include "xcompiler/backend/backend.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/ir/gen.hpp"
#include "xcompiler/optimizer/optimizer.hpp"

namespace xcompiler {

    class Xcompiler {
    public:
        void Run(const Config& config, AstNode& node, sema::FnTable& fn_table) {

            // Directories
            auto module_name = config.project_.name_;
            auto path =
                config.project_.build_.path_ /
                config.project_.profile_.name_;

            // TypeImpl
            TypeImplTable::Init();

            // Builtin
            BuiltinFnRegister(fn_table);

            // Backend
            Backend backend;

            // IR Gen and Output
            auto path_ir = path / "ir";
            IRGen irgen(module_name);
            backend.ModuleSet(*irgen.llvm_module());
            irgen.Exec(node);

            if (config.project_.build_.emit_ir_) {
                std::filesystem::create_directories(path_ir);
                backend.IROutput((path_ir / (module_name + ".ll")).string(), *irgen.llvm_module());
            }

            // IR Optimize
            Optimizer optimizer;
            optimizer.Run(*irgen.llvm_module(), config.project_.profile_.opt_level_);

            // Object Code Gen and Output
            auto path_obj = path / "obj";
            std::filesystem::create_directories(path_obj);
            backend.ObjectCodeOutput((path_obj / (module_name + ".o")).string(), *irgen.llvm_module());

            // Linker
            auto gpp = llvm::sys::findProgramByName("g++");
            if (!gpp) {
                throw LogErr(LogModule::Xcompiler, "failed to find g++");
            }
            
            auto link_status = llvm::sys::ExecuteAndWait(*gpp, {
                *gpp, (path_obj / (module_name + ".o")).string(),
                "-o", (path / (module_name + ".exe")).string(),
            });
            if (link_status != 0) {
                throw LogErr(LogModule::Xcompiler, "failed to link object file");
            }
        }
    };
}

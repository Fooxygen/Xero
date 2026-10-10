
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>
#include <filesystem>

#include "llvm/Support/Program.h"

#include "common/config.hpp"
#include "common/log.hpp"
#include "common/defs/ast.hpp"
#include "context/context.hpp"
#include "sema/defs/fn.hpp"
#include "xcompiler/backend/backend.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/ir/gen.hpp"
#include "xcompiler/optimizer/optimizer.hpp"

namespace xcompiler {

    class Xcompiler {
    public:
        void Run(
            const Config& config,
            std::vector<context::Module>& modules,
            sema::FnTable& fn_table
        ) {

            auto project_name = config.project_.name_;
            auto profile_path =
                config.project_.build_.path_ /
                config.project_.profile_.name_;

            // TypeImpl
            TypeImplTable::Init();

            // Builtin
            BuiltinFnRegister(fn_table);

            // Backend
            Backend backend;

            // IR Gen and Output
            auto ir_path = profile_path / "ir";
            IRGen irgen(project_name);
            backend.ModuleSet(*irgen.llvm_module());
            
            for (auto& module : modules) irgen.Declare(*module.root());
            for (auto& module : modules) irgen.Process(*module.root());

            if (config.project_.build_.emit_ir_) {
                std::filesystem::create_directories(ir_path);
                backend.IROutput((ir_path / (project_name + ".ll")).string(), *irgen.llvm_module());
            }

            // IR Optimize
            Optimizer optimizer;
            optimizer.Run(*irgen.llvm_module(), config.project_.profile_.opt_level_);

            // Object Code Gen and Output
            auto obj_path = profile_path / "obj";
            std::filesystem::create_directories(obj_path);
            backend.ObjectCodeOutput((obj_path / (project_name + ".o")).string(), *irgen.llvm_module());

            // Linker
            auto gpp = llvm::sys::findProgramByName("g++");
            if (!gpp) {
                throw LogErr(LogStage::Xcompiler, "failed to find g++");
            }
            
            auto link_status = llvm::sys::ExecuteAndWait(*gpp, {
                *gpp, (obj_path / (project_name + ".o")).string(),
                "-o", (profile_path / (project_name + ".exe")).string(),
            });
            if (link_status != 0) {
                throw LogErr(LogStage::Xcompiler, "failed to link object file");
            }
        }
    };
}

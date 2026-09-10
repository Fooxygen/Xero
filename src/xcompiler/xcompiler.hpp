
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <filesystem>

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
        void Run(const std::string& module_name, AstNode& node, sema::FnTable& fn_table) {

            // Directories
            auto path = std::filesystem::path(std::format(
                "build/{}", module_name
            ));

            auto path_ir  = path / "ir";
            auto path_obj = path / "obj";
            std::filesystem::create_directories(path_ir);
            std::filesystem::create_directories(path_obj);

            // TypeImpl
            TypeImplTable::Init();

            // Builtin
            BuiltinFnRegister(fn_table);

            // Backend
            Backend backend;

            // IR Gen and Output
            IRGen irgen(module_name);
            backend.ModuleSet(*irgen.llvm_module());
            irgen.Exec(node);
            backend.IROutput((path_ir / (module_name + ".ll")).string(), *irgen.llvm_module());

            // IR Optimize
            Optimizer optimizer;
            optimizer.Run(*irgen.llvm_module(), llvm::OptimizationLevel::O2);

            // Object Code Gen and Output
            backend.ObjectCodeOutput((path_obj / (module_name + ".o")).string(), *irgen.llvm_module());

            // Linker
            auto gpp = llvm::sys::findProgramByName("g++");
            if (!gpp) {
                throw LogErr(LogModule::Xcompiler, "failed to find g++");
            }
            
            auto link_status =  llvm::sys::ExecuteAndWait(*gpp, {
                *gpp, (path_obj / (module_name + ".o")).string(),
                "-o", (path / (module_name + ".exe")).string(),
            });
            if (link_status != 0) {
                throw LogErr(LogModule::Xcompiler, "failed to link object file");
            }
        }
    };
}

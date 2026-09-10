
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <memory>

#include "llvm/IR/Module.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Triple.h"

namespace xcompiler {

    class Backend {
    private:
        llvm::Triple                         target_triple_;
        std::unique_ptr<llvm::TargetMachine> target_machine_;
        llvm::DataLayout                     data_layout_;

    public:
        Backend();

    public:
        void ModuleSet(llvm::Module& module);

        void IROutput(const std::string& path, llvm::Module& module);
        void ObjectCodeOutput(const std::string& path, llvm::Module& module);
    };
}

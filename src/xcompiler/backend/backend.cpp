
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/TargetParser/SubtargetFeature.h"

#include "common/log.hpp"
#include "xcompiler/backend/backend.hpp"

namespace xcompiler {

    Backend::Backend()
    :   target_triple_(llvm::sys::getDefaultTargetTriple())
    {
        // Configure
        
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmPrinters();
        
        // └─ Target
        std::string target_err = "";
        auto target = llvm::TargetRegistry::lookupTarget(target_triple_, target_err);
        if (!target) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "failed to lookup target: {}", target_err
            ));
        }

        // └─ Target Machine
        auto cpu            = llvm::sys::getHostCPUName();
        auto features       = llvm::SubtargetFeatures(); {
            for (const auto& feature : llvm::sys::getHostCPUFeatures()) {
                features.AddFeature(feature.first(), feature.second);
            }
        }
        target_machine_ = std::unique_ptr<llvm::TargetMachine>(
            target->createTargetMachine(
                target_triple_, cpu, features.getString(),
                llvm::TargetOptions{}, llvm::Reloc::PIC_
            )
        );

        // Data Layout
        data_layout_ = target_machine_->createDataLayout();
    }

    void Backend::ModuleSet(llvm::Module& module) {
        module.setDataLayout(data_layout_);
        module.setTargetTriple(target_triple_);
    }

    void Backend::IROutput(const std::string& path, llvm::Module& module) {

        // Open File
        if (path.empty()) {
            throw LogErr(LogModule::Xcompiler, "empty file path");
        }

        std::error_code ec;
        llvm::raw_fd_ostream file(path, ec);
        if (ec) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "failed to open file '{}'", path
            ));
        }

        module.print(file, nullptr);
        file.flush();
    }

    void Backend::ObjectCodeOutput(const std::string& path, llvm::Module& module) {

        // └─ Open File
        if (path.empty()) {
            throw LogErr(LogModule::Xcompiler, "empty file path");
        }

        std::error_code ec;
        llvm::raw_fd_ostream file(path, ec);
        if (ec) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "failed to open file '{}'", path
            ));
        }

        // Execute

        // └─ Pass
        llvm::legacy::PassManager pass;
        if (target_machine_->addPassesToEmitFile(
            pass, file, nullptr, llvm::CodeGenFileType::ObjectFile
        ))
        {
            throw LogErr(LogModule::Xcompiler, "failed to generate object code");
        }

        pass.run(module);
        file.flush();
    }
}

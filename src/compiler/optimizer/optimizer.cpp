
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"

#include "compiler/optimizer/optimizer.hpp"

namespace compiler {
    
    void Optimizer::Run(llvm::Module& module, llvm::OptimizationLevel level) {

        // Analysis Manager
        llvm::LoopAnalysisManager     LAM;
        llvm::FunctionAnalysisManager FAM;
        llvm::CGSCCAnalysisManager    CGAM;
        llvm::ModuleAnalysisManager   MAM;

        // Pass
        llvm::PassBuilder PB;
        PB.registerModuleAnalyses(MAM);
        PB.registerCGSCCAnalyses(CGAM);
        PB.registerFunctionAnalyses(FAM);
        PB.registerLoopAnalyses(LAM);
        PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);   // Analyzer Communication Network

        // Pipeline
        auto MPM = PB.buildPerModuleDefaultPipeline(level);
        MPM.run(module, MAM);
    }
}

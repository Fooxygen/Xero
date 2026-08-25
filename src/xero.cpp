
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>

#include "llvm/Support/Program.h"

#include "build.hpp"
#include "common/log.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "sema/type.hpp"
#include "sema/method.hpp"
#include "sema/sema.hpp"
#include "compiler/irgen/type.hpp"
#include "compiler/irgen/irgen.hpp"
#include "compiler/optimizer/optimizer.hpp"
#include "xengine/type.hpp"
#include "xengine/method.hpp"
#include "xengine/xengine.hpp"

struct Args {
    std::filesystem::path path_;
    bool isPrintToken_ = false;
    bool isPrintAst_   = false;
    bool isCompiler    = false;
    bool isXengine     = true;
};

std::string FileRead(const std::string& path) {
    if (path.empty()) {
        throw LogErr(LogModule::File, "empty file path");
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw LogErr(LogModule::File, std::format("failed to open file '{}'", path));
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int main(int argc, char* argv[]) {

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cerr << BuildInfo::Print() << std::endl;

    try {
        if (argc < 2) {
            throw LogErr(LogModule::File, "usage: xero.exe <file.xe> [--ast] [--tok]");
        }

        // Args
        Args args;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if      (arg == "--ast"  || arg == "-a")  args.isPrintAst_   = true;
            else if (arg == "--tok"  || arg == "-t")  args.isPrintToken_ = true;
            else if (arg == "--comp" || arg == "-cp") args.isCompiler    = true;
            else if (arg == "--xeng" || arg == "-xe") args.isXengine     = true;
            else                                      args.path_ = std::filesystem::path(arg);
        }

        // Code
        std::string code = FileRead(args.path_.string());

        // Lexer
        lexer::Lexer lexer(code);
        lexer.TokensGen(args.isPrintToken_);

        // Parser
        parser::Parser parser(lexer.tokens());
        parser.Execute();
        if (args.isPrintAst_) {
            LogStart(LogModule::Parser, "output ast").Print();
            parser.root()->Print("", "", true);
            LogFinish(LogModule::Parser, "output ast").Print();
        }

        // Sema
        sema::TypeTable::Init();
        sema::MethodTable::BuiltinRegister();

        sema::Sema sema;
        sema.Exec(*parser.root());

        // Compiler
        if (args.isCompiler) {
            
            // Directories
            auto module_name = args.path_.stem().string();
            auto path = std::filesystem::path(std::format(
                "build/{}", module_name
            ));

            auto path_ir  = path / "ir";
            auto path_obj = path / "obj";
            std::filesystem::create_directories(path_ir);
            std::filesystem::create_directories(path_obj);

            // TypeGen
            compiler::TypeGenTable::Init();

            // IR Gen and Output
            compiler::IRGen irgen(module_name);
            irgen.Exec(*parser.root());
            irgen.IROutput((path_ir / (module_name + ".ll")).string());

            // IR Optimize
            compiler::Optimizer optimizer;
            optimizer.Run(*irgen.module(), llvm::OptimizationLevel::O2);

            // Object Code Gen and Output
            irgen.ObjectCodeOutput((path_obj / (module_name + ".o")).string());

            // Linker
            auto gpp = llvm::sys::findProgramByName("g++");
            if (!gpp) {
                throw LogErr(LogModule::Compiler, "failed to find g++");
            }
            
            auto link_status =  llvm::sys::ExecuteAndWait(*gpp, {
                *gpp, (path_obj / (module_name + ".o")).string(),
                "-o", (path / (module_name + ".exe")).string(),
            });
            if (link_status != 0) {
                throw LogErr(LogModule::Compiler, std::format(
                    "failed to link object file: {}", link_status
                ));
            }
        }

        // Xengine
        if (args.isXengine) {
            
            // TypeImpl
            xengine::TypeImplTable::Init();

            // MethodImplTable
            xengine::MethodImplTable::BuiltinImplRegister();

            // Run
            xengine::Xengine xengine;
            xengine.Exec(*parser.root());
            std::cerr << std::endl;
        }
    }
    catch (const LogErr& log) {
        log.Print();
        return 1;
    }
    catch (const Log& log) {
        log.Print();
    }
    catch (const std::exception& e) {
        LogErr(LogModule::File, std::format("unexpected error: {}", e.what())).Print();
        return 1;
    }
    
    return 0;
}

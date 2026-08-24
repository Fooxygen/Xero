
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

#include "build.hpp"
#include "common/log.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "sema/type.hpp"
#include "sema/method.hpp"
#include "sema/sema.hpp"
#include "compile/irgen/irgen.hpp"
#include "compile/irgen/type.hpp"
#include "runtime/type.hpp"
#include "runtime/method.hpp"
#include "runtime/xengine.hpp"

struct Args {
    std::filesystem::path path_;
    bool isPrintToken_ = false;
    bool isPrintAst_   = false;
    bool isCompile     = false;
    bool isRuntime     = true;
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
            else if (arg == "--comp" || arg == "-cp") args.isCompile     = true;
            else if (arg == "--runt" || arg == "-rt") args.isRuntime     = true;
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

        // Compile
        if (args.isCompile) {
            
            // Directories
            auto module_name = args.path_.stem().string();
            auto path_project = std::filesystem::path(std::format(
                "build/{}", module_name
            ));

            auto path_ir = path_project / "ir";
            std::filesystem::create_directories(path_ir);

            // TypeGen
            compile::TypeGenTable::Init();

            // IRGen
            compile::IRGen irgen(
                module_name,
                (path_ir / (module_name + ".ll")).string(),
                *parser.root()
            );
        }

        // Runtime
        if (args.isRuntime) {
            
            // TypeImpl
            rt::TypeImplTable::Init();

            // MethodImplTable
            rt::MethodImplTable::BuiltinImplRegister();

            // Xengine
            rt::Xengine xengine;
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

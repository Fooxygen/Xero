
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

#include "llvm/Support/Program.h"

#include "build.hpp"
#include "common/log.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "sema/sema.hpp"
#include "xcompiler/xcompiler.hpp"

struct Args {
    std::filesystem::path path_;
    bool isPrintToken_ = false;
    bool isPrintAst_   = false;
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
            else                                      args.path_         = std::filesystem::path(arg);
        }

        // Code
        std::string code = FileRead(args.path_.string());

        // Lexer
        lexer::Lexer lexer(code);
        lexer.TokensGen(args.isPrintToken_);
        LogDone(LogModule::Lexer).Print();

        // Parser
        parser::Parser parser(lexer.tokens());
        parser.Execute();
        if (args.isPrintAst_) parser.root()->Print("", "", true);
        LogDone(LogModule::Parser).Print();

        // Sema
        sema::Sema sema;
        sema.Run(*parser.root());
        LogDone(LogModule::Sema).Print();

        // Xcompiler
        xcompiler::Xcompiler xcompiler;
        xcompiler.Run(
            args.path_.stem().string(),
            *parser.root(),
            sema.analyzer().fn_table()
        );
        LogDone(LogModule::Xcompiler).Print();
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

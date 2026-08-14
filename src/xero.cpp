
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include "build.hpp"
#include "common/log.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "sema/semantics.hpp"
#include "runtime/runtime.hpp"
#include "runtime/xengine.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

struct BeginInfo {
    std::string path  = "";
    bool isPrintToken = false;
    bool isPrintAst   = false;
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
        // Begin
        BeginInfo begininfo;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if      (arg == "-a" || arg == "--ast") begininfo.isPrintAst   = true;
            else if (arg == "-t" || arg == "--tok") begininfo.isPrintToken = true;
            else begininfo.path = arg;
        }

        // Code
        std::string code = FileRead(begininfo.path);

        // Lexer
        lexer::Lexer lexer(code);
        lexer.TokensGen(begininfo.isPrintToken);

        // Parser
        parser::Parser parser(lexer.tokens());
        parser.Execute();

        if (begininfo.isPrintAst) {
            LogStart(LogModule::Parser, "output ast").Print();
            parser.root()->Print("", "", true);
            LogFinish(LogModule::Parser, "output ast").Print();
        }

        // Semantics
        sema::TypeTable::Init();

        // Runtime
        rt::TypeImplTable::Init();

        rt::Xengine xengine;
        xengine.Exec(*parser.root());
        std::cerr << std::endl;
    }
    catch (const LogErr& log) {
        log.Print();
        return 1;
    }
    catch (const Log& log) {
        log.Print();
    }
    
    return 0;
}

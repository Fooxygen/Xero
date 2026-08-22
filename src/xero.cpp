
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

#include "build.hpp"
#include "common/log.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "sema/type.hpp"
#include "sema/method.hpp"
#include "sema/sema.hpp"
#include "runtime/type.hpp"
#include "runtime/method.hpp"
#include "runtime/xengine.hpp"

struct BeginInfo {
    std::string path_   = "";
    bool isPrintToken_  = false;
    bool isPrintAst_    = false;
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

        // Begin
        BeginInfo begininfo;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if      (arg == "-a" || arg == "--ast") begininfo.isPrintAst_   = true;
            else if (arg == "-t" || arg == "--tok") begininfo.isPrintToken_ = true;
            else begininfo.path_ = arg;
        }

        // Code
        std::string code = FileRead(begininfo.path_);

        // Lexer
        lexer::Lexer lexer(code);
        lexer.TokensGen(begininfo.isPrintToken_);

        // Parser
        parser::Parser parser(lexer.tokens());
        parser.Execute();

        if (begininfo.isPrintAst_) {
            LogStart(LogModule::Parser, "output ast").Print();
            parser.root()->Print("", "", true);
            LogFinish(LogModule::Parser, "output ast").Print();
        }

        // Sema
        sema::TypeTable::Init();
        sema::MethodTable::BuiltinRegister();

        sema::Sema sema;
        sema.Exec(*parser.root());

        // Runtime
        rt::TypeImplTable::Init();
        rt::MethodImplTable::BuiltinImplRegister();

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
    catch (const std::exception& e) {
        LogErr(LogModule::File, std::format("unexpected error: {}", e.what())).Print();
        return 1;
    }
    
    return 0;
}

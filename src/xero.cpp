
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

#include "build.hpp"
#include "log.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "runtime/xengine.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

std::string FileRead(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw LogErr(LogModule::File, std::format("failed to open file '{}'", path));
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int main() {

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cout << BuildInfo::Print() << std::endl;

    try {
        // Code
        std::string file_path = "";
        std::cin >> file_path;
        std::string code = FileRead(file_path);

        // Lexer
        lexer::Lexer lexer(code);
        lexer.TokensGen(true);

        // Parser
        parser::Parser parser(lexer.tokens());
        parser.Execute();

        LogStart(LogModule::Parser, "output ast").Print();
        parser.root()->Print("", "", true);
        LogFinish(LogModule::Parser, "output ast").Print();

        // Runtime
        rt::Xengine xengine;
        LogStart(LogModule::Runtime, "program").Print();
        xengine.Exec(*parser.root());
        std::cout << std::endl;
        LogFinish(LogModule::Runtime, "program").Print();
    }
    catch (const Log& log) {
        log.Print();
    }
    
    return 0;
}

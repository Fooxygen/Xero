
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

std::string FileRead(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw LogErr(LogModule::File, "failed to open file!");
    }

    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

int main() {
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
        parser.root()->AstPrint();
        std::cout << std::endl;
        LogFinish(LogModule::Parser, "output ast").Print();
    }
    catch(const Log& log) {
        log.Print();
    }
    
    return 0;
}

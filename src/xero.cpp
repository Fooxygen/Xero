
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
#include <format>

#include "build.hpp"
#include "common/log.hpp"
#include "context/context.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "sema/sema.hpp"
#include "xcompiler/xcompiler.hpp"

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
        if (argc != 2) {
            throw LogErr(LogModule::File, "usage: xero.exe <xero.project.toml>");
        }

        // Context
        context::Context context;

        // Configs
        auto& config         = context.config();
        auto& config_project = config.project_;
        config.ProjectConfigLoad(argv[1]);

        // Modules
        auto& modules_path = config.project_.modules_;
        for (auto& module_path : modules_path) {
            context.modules().emplace_back(
                module_path,
                FileRead(module_path.string())
            );
        }

        // Lexer
        for (auto& module : context.modules()) {
            lexer::Lexer lexer(module.code());
            lexer.TokensGen(config_project.diag_.is_print_tokens_);
            module.tokens() = std::move(lexer.tokens());
            LogDone(LogModule::Lexer).Print();
        }

        // Parser
        for (auto& module : context.modules()) {
            parser::Parser parser(module.tokens());
            parser.Execute();
            if (config_project.diag_.is_print_ast_) {
                parser.root()->Print("", "", true);
            }
            module.root() = std::move(parser.root());
            LogDone(LogModule::Parser).Print();
        }

        // Sema
        sema::Sema sema(context.fn_table());
        sema.Run(context.modules());
        LogDone(LogModule::Sema).Print();

        // Xcompiler
        xcompiler::Xcompiler xcompiler;
        xcompiler.Run(
            config,
            context.modules(),
            context.fn_table()
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

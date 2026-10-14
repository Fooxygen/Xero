
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <string>
#include <format>

#include "build.hpp"
#include "common/log.hpp"
#include "common/defs/const.hpp"
#include "context/context.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "sema/sema.hpp"
#include "xcompiler/xcompiler.hpp"

int main(int argc, char* argv[]) {

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cerr << BuildInfo::Print() << std::endl;

    try {
        if (argc != 2) {
            throw LogErr(LogStage::File, std::format(
                "usage: xero.exe <{}>", XERO_FILE_CONFIG_PROJECT
            ));
        }

        // Context
        context::Context context;

        // Configs
        auto& config         = context.config();
        auto& config_project = config.project_;
        config.ProjectConfigLoad(argv[1]);

        // Packages and Modules
        context.ModulesProjectLoad();

        // Lexer, Parser and Dependency
        for (size_t i = 0; i < context.modules().size(); i++) {
            auto& module = context.modules()[i];

            // Lexer
            lexer::Lexer lexer(module);
            lexer.Run(config_project.diag_.is_print_tokens_);
            module.tokens() = std::move(lexer.tokens());

            // Parser
            parser::Parser parser(module);
            parser.Run(config_project.diag_.is_print_ast_);
            module.root() = std::move(parser.root());

            // Dependency
            // Any Module may contain dependencies,
            // so newly imported modules will be processed in this loop
            context.ModuleDependencyLoad(module);
        }

        // Sema
        sema::Sema sema(context.fn_table());
        sema.Run(context.modules());

        // Xcompiler
        xcompiler::Xcompiler xcompiler(config, context.fn_table());
        xcompiler.Run(context.modules());
    }
    catch (const LogErr& log) {
        log.Print();
        return 1;
    }
    catch (const Log& log) {
        log.Print();
    }
    catch (const std::exception& e) {
        LogErr(LogStage::File, std::format("unexpected error: {}", e.what())).Print();
        return 1;
    }
    
    return 0;
}

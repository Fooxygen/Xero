
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

#include "deps/vendor/toml++/toml.hpp"

#include "build.hpp"
#include "common/config.hpp"
#include "common/log.hpp"
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

void ProjectConfigLoad(ProjectConfig& project, const std::string& path) {

    toml::table table;
    try {
        table = toml::parse_file(path);
    } catch (const toml::parse_error& e) {
        throw LogErr(LogModule::Config, std::format(
            "failed to parse {} as config of project: {}", path, e.description()
        ));
    }

    // Basic
    {
        // Name

        auto name = table["name"].value<std::string>();
        if (!name || name->empty()) {
            throw LogErr(LogModule::Config, "project: field 'name' must not be empty");
        }
        project.name_ = *name;

        // Root

        project.root_  = std::filesystem::weakly_canonical(path).parent_path();
        
        // Entry

        auto entry = table["entry"].value<std::string>();
        if (!entry || entry->empty()) {
            throw LogErr(LogModule::Config, "project: field 'entry' must not be empty");
        }
        project.entry_ = project.root_ / *entry;
        if (!std::filesystem::is_regular_file(project.entry_)) {
            throw LogErr(LogModule::Config, "project: field 'entry' must be an existing file");
        }
    }
    
    // Profile
    {
        auto& profile = project.profile_;

        // Name
        auto name = table["profile"].value<std::string>();
        if (!name || name->empty()) {
            throw LogErr(LogModule::Config, "project: field 'profile' must not be empty");
        }
        if (!table["profiles"][*name]) {
            throw LogErr(LogModule::Config, std::format(
                "project.profile: failed to find '{}'", *name
            ));
        }
        profile.name_ = *name;

        // Optimization Level
        {
            std::string opt = table["profiles"][profile.name_]["opt_level"].value_or<std::string>("o0");
            if      (opt == "o0") profile.opt_level_ = llvm::OptimizationLevel::O0;
            else if (opt == "o1") profile.opt_level_ = llvm::OptimizationLevel::O1;
            else if (opt == "o2") profile.opt_level_ = llvm::OptimizationLevel::O2;
            else if (opt == "o3") profile.opt_level_ = llvm::OptimizationLevel::O3;
            else if (opt == "os") profile.opt_level_ = llvm::OptimizationLevel::Os;
            else if (opt == "oz") profile.opt_level_ = llvm::OptimizationLevel::Oz;
        }
    }

    // Build
    {
        auto& build    = project.build_;

        auto path = table["build"]["path"].value<std::string>();
        if (!path || path->empty()) {
            throw LogErr(LogModule::Config, "project.build: field 'path' must not be empty");
        }
        build.path_    = project.root_ / *path;
        
        build.emit_ir_ = table["build"]["emit_ir"].value_or<bool>(true);
    }

    // Diag
    {
        auto& diag            = project.diag_;
        diag.is_print_tokens_ = table["diag"]["print_tokens"].value_or<bool>(false);
        diag.is_print_ast_    = table["diag"]["print_ast"].value_or<bool>(false);
    }
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

        // Configs
        Config config;
        ProjectConfigLoad(config.project_, argv[1]);

        // Code
        std::string code = FileRead(config.project_.entry_.string());

        // Lexer
        lexer::Lexer lexer(code);
        lexer.TokensGen(config.project_.diag_.is_print_tokens_);
        LogDone(LogModule::Lexer).Print();

        // Parser
        parser::Parser parser(lexer.tokens());
        parser.Execute();
        if (config.project_.diag_.is_print_ast_) {
            parser.root()->Print("", "", true);
        }
        LogDone(LogModule::Parser).Print();

        // Sema
        sema::Sema sema;
        sema.Run(*parser.root());
        LogDone(LogModule::Sema).Print();

        // Xcompiler
        xcompiler::Xcompiler xcompiler;
        xcompiler.Run(
            config,
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

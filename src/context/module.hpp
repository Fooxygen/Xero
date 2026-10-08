
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <filesystem>

#include "common/defs/token.hpp"
#include "common/defs/ast.hpp"

namespace context {
    
    class Module {
    private:
        std::string           name_ = "";
        std::filesystem::path path_;
        std::string           code_ = "";

        // Lexer

        std::vector<Token> tokens_ = {};

        // Parser

        std::unique_ptr<AstNode> root_ = nullptr;

    public:
        Module(std::filesystem::path path, std::string code)
        :   name_(path.stem().string()),
            path_(std::move(path)),
            code_(std::move(code))
        {}

        const std::string&           name() const { return name_; }
        const std::filesystem::path& path() const { return path_; }
        const std::string&           code() const { return code_; }
        
        std::vector<Token>&          tokens() { return tokens_; }
        std::unique_ptr<AstNode>&    root()   { return root_; }
    };
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>

#include "common/defs/token.hpp"
#include "common/defs/ast.hpp"
#include "common/defs/path.hpp"

namespace context {
    
    class Module {
    private:
        std::string              name_       = "";
        std::string              name_full_  = "";
        std::filesystem::path    path_;

        // Namespace

        Path namespaces_;
        std::unordered_map<std::string, Path> namespaces_alias_;
        
        // Lexer

        std::string        code_   = "";
        std::vector<Token> tokens_ = {};

        // Parser

        std::unique_ptr<AstNode> root_ = nullptr;

    public:
        Module(
            std::filesystem::path path,
            Path                  namespaces,
            std::string           code
        )
        :   name_(path.stem().string()),
            path_(std::move(path)),
            namespaces_(std::move(namespaces)),
            code_(std::move(code))
        {
            // Full Name
            name_full_ = namespaces_.Print();
        }

        const std::string&              name()       const { return name_; }
        const std::string&              name_full()  const { return name_full_; }
        const std::filesystem::path&    path()       const { return path_; }

        const Path&                                  namespaces()       const { return namespaces_; }
        std::unordered_map<std::string, Path>&       namespaces_alias()       { return namespaces_alias_; }
        const std::unordered_map<std::string, Path>& namespaces_alias() const { return namespaces_alias_; }

        const std::string&        code()   const { return code_; }
        std::vector<Token>&       tokens()       { return tokens_; }
        std::unique_ptr<AstNode>& root()         { return root_; }
    };
}

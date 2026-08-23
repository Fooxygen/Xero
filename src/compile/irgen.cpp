
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <format>

#include "common/log.hpp"
#include "compile/irgen.hpp"

namespace compile {

    IRGen::IRGen(std::string_view module_name)
    :   context_(),
        module_(std::make_unique<llvm::Module>(module_name, context_)),
        builder_(context_)
    {}

    void IRGen::Exec(AstNode& root, std::string path) {

        // Open File
        if (path.empty()) {
            throw LogErr(LogModule::File, "empty file path");
        }

        std::error_code ec;
        llvm::raw_fd_ostream file(path, ec);
        if (ec) {
            throw LogErr(LogModule::Compile, std::format(
                "failed to open file '{}'", path
            ));
        }

        // Print
        module_->print(file, nullptr);
        file.flush();

        root.TypePrint();
    }
}

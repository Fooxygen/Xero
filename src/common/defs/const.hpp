
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string_view>

namespace consts {

    // Extension
    inline constexpr std::string_view EXT_SOURCE            = ".xe";
    inline constexpr std::string_view EXT_IR                = ".ll";
    inline constexpr std::string_view EXT_OBJ               = ".o";
    inline constexpr std::string_view EXT_EXECUTABLE        = ".exe";

    // File
    inline constexpr std::string_view FILE_CONFIG_PROJECT   = "xero.project.toml";
    inline constexpr std::string_view FILE_CONFIG_PACKAGE   = "xero.package.toml";
}

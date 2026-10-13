
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <format>

#include "common/log.hpp"

namespace io {

    inline std::string FileRead(const std::string& path) {
        if (path.empty()) {
            throw LogErr(LogStage::File, "empty file path");
        }

        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw LogErr(LogStage::File, std::format("failed to open file '{}'", path));
        }

        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <filesystem>

#include "common/utils/io.hpp"
#include "common/defs/const.hpp"
#include "context/context.hpp"

namespace context {

    void Context::ModulesLoad() {

        // Project
        for (auto& module_path : config_.project_.modules_) {
            std::vector<std::string> namespaces = { module_path.stem().string() };
            modules().emplace_back(
                module_path,
                std::move(namespaces),
                io::FileRead(module_path.string())
            );
        }

        // Packages
        for (auto& package_path : config_.project_.packages_) {
            auto package_name = package_path.filename().string();

            for (auto& entry : std::filesystem::recursive_directory_iterator(package_path)) {
                if (!entry.is_regular_file()) continue;
                if (entry.path().extension() != consts::EXT_SOURCE) continue;

                auto& path          = entry.path();
                auto  path_relative = path.lexically_relative(package_path);

                // Namespaces
                std::vector<std::string> namespaces = { package_name };
                for (auto& part : path_relative.parent_path()) {
                    namespaces.emplace_back(part.string());
                }
                namespaces.emplace_back(path.stem().string());

                // Modules
                modules().emplace_back(
                    entry.path(),
                    std::move(namespaces),
                    io::FileRead(entry.path().string())
                );
            }
        }
    }
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <filesystem>

#include "common/utils/io.hpp"
#include "common/defs/const.hpp"
#include "context/context.hpp"

namespace context {

    void Context::ModulesProjectLoad() {
        for (auto& module_path : config_.project_.modules_) {
            std::vector<std::string> namespaces = { module_path.stem().string() };
            modules().emplace_back(
                module_path,
                std::move(namespaces),
                io::FileRead(module_path.string())
            );
        }
    }

    void Context::ModulesDependencyLoad(const std::vector<std::string>& path) {
        if (path.empty()) return;

        // Package Root
        std::filesystem::path package_root = {};
        for (auto& package_path : config_.project_.packages_) {
            if (package_path.filename().string() == path[0]) {
                package_root = package_path;
                break;
            }
        }
        if (package_root.empty()) return;

        // Recursion Search
        auto current_dir       = package_root;
        auto current_ns        = std::vector<std::string>{ path[0] };
        auto result_namespaces = std::vector<std::string>{};
        auto result_module     = std::filesystem::path{};

        for (size_t i = 1; i < path.size(); i++) {
            const auto& seg = path[i];

            // Exist as module
            auto seg_module = current_dir / (seg + std::string(consts::EXT_SOURCE));
            if (std::filesystem::is_regular_file(seg_module)) {
                result_module     = seg_module;
                result_namespaces = current_ns;
                result_namespaces.emplace_back(seg);
            }

            // Exist as namespace
            auto seg_namespace = current_dir / seg;
            if (std::filesystem::is_directory(seg_namespace)) {
                current_dir = seg_namespace;
                current_ns.emplace_back(seg);
                continue;
            }

            break;
        }
        if (result_module.empty()) return;

        // Deduplicate
        for (auto& module : modules_) {
            if (std::filesystem::weakly_canonical(module.path()) ==
                std::filesystem::weakly_canonical(result_module))
            {
                return;
            }
        }

        // Add
        modules_.emplace_back(
            result_module,
            std::move(result_namespaces),
            io::FileRead(result_module.string())
        );
    }
}

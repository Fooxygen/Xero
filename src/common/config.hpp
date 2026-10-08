
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <vector>
#include <format>
#include <filesystem>

#include "llvm/Passes/OptimizationLevel.h"
#include "deps/vendor/toml++/toml.hpp"

#include "common/log.hpp"

struct ProjectConfig {

    struct Profile {
        std::string name_ = "";
        llvm::OptimizationLevel opt_level_ = llvm::OptimizationLevel::O0;
    };

    struct Build {
        std::filesystem::path path_ = "";
        bool emit_ir_  = true;
    };

    struct Diag {
        bool is_print_tokens_ = false;
        bool is_print_ast_    = false;
    };

    std::string                        name_ = "";
    std::filesystem::path              root_;
    std::vector<std::filesystem::path> modules_;

    Profile profile_;
    Build   build_;
    Diag    diag_;
};

struct Config {
    ProjectConfig project_;

    void ProjectConfigLoad(const std::string& path) {
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
            {
                auto name = table["name"].value<std::string>();
                if (!name || name->empty()) {
                    throw LogErr(LogModule::Config, "project: field 'name' must not be empty");
                }
                project_.name_ = *name;
            }

            // Root
            {
                project_.root_  = std::filesystem::weakly_canonical(path).parent_path();
            }
            
            // Modules
            {
                auto modules = table["modules"].as_array();
                if (!modules || modules->empty()) {
                    throw LogErr(LogModule::Config, "project: field 'modules' must not be empty");
                }

                for (auto&& module : *modules) {
                    auto val = module.value<std::string>();
                    if (!val || val->empty()) continue;

                    auto path_full = project_.root_ / *val;
                    if (!std::filesystem::is_regular_file(path_full)) {
                        throw LogErr(LogModule::Config, std::format(
                            "project.modules: '{}' is not an existing file", *val));
                    }

                    auto path_canon = std::filesystem::weakly_canonical(project_.root_ / *val);

                    for (auto& module_existed : project_.modules_) {
                        if (std::filesystem::equivalent(module_existed, path_canon)) {
                            throw LogErr(LogModule::Config, std::format(
                                "project.modules: '{}' is already listed", *val));
                        }
                    }

                    project_.modules_.push_back(path_canon);
                }
            }
        }
        
        // Profile
        {
            auto& profile = project_.profile_;

            // Name
            {
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
            }

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
            auto& build = project_.build_;

            // Path
            {
                auto path = table["build"]["path"].value<std::string>();
                if (!path || path->empty()) {
                    throw LogErr(LogModule::Config, "project.build: field 'path' must not be empty");
                }
                build.path_    = project_.root_ / *path;
                
                build.emit_ir_ = table["build"]["emit_ir"].value_or<bool>(true);
            }
        }

        // Diag
        {
            auto& diag            = project_.diag_;
            diag.is_print_tokens_ = table["diag"]["print_tokens"].value_or<bool>(false);
            diag.is_print_ast_    = table["diag"]["print_ast"].value_or<bool>(false);
        }
    }
};

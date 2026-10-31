
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>
#include <vector>
#include <initializer_list>

#include "common/utils/format.hpp"

class Path {
private:
    std::vector<std::string> segments_ = {};

public:
    Path() = default;
    Path(std::initializer_list<std::string> segments) : segments_(segments) {}
    Path(const std::vector<std::string>& segments)    : segments_(segments) {}

    const std::vector<std::string>& segments() const { return segments_; }

public:
    void   Add(const std::string& seg) { segments_.emplace_back(seg); }

    size_t Length()  const { return segments_.size(); }
    bool   IsEmpty() const { return segments_.empty(); }
    auto&  Back()    const { return segments_.back(); }

    std::string Print() const {
        return format::JoinWithBoundary(
            segments_, [](const std::string& seg) { return seg;},
            "", "", "::"
        );
    }

    const std::string& operator [](size_t i)          const { return segments_[i]; }
    bool               operator ==(const Path& other) const = default;

    template <typename Aliases>
    static Path Resolve(const Path& path, const Aliases& aliases) {
        // Namespace aliases only apply to qualified paths.
        if (path.Length() < 2) return path;

        auto it = aliases.find(path[0]);
        if (it == aliases.end()) return path;

        Path resolved = it->second;
        for (size_t i = 1; i < path.Length(); i++) {
            resolved.Add(path[i]);
        }
        return resolved;
    }
};

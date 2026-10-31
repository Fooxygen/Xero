
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/defs/path.hpp"

// Anonymous block scope.
template <typename T>
class Scope {
private:
    std::vector<std::unordered_map<std::string, T>> scopes_;

public:
    Scope() {
        scopes_.emplace_back();
    }

    std::unordered_map<std::string, T>& scopes() { return scopes_.back(); }

public:
    void Push() { scopes_.emplace_back(); }
    void Pop() {
        if (scopes_.size() > 1) scopes_.pop_back();
    }

    T* LookupTry(const std::string& name) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) return &found->second;
        }
        return nullptr;
    }

    T* Set(const std::string& name, T value) {
        auto it = scopes_.back().insert_or_assign(name, std::move(value)).first;
        return &it->second;
    }

    std::pair<T*, bool> Declare(const std::string& name, T value) {
        auto [it, is_inserted] = scopes_.back().try_emplace(name, std::move(value));
        return { &it->second, is_inserted };
    }
};

// Named namespace.
template <typename T>
class Namespace {
private:
    struct Node {
        std::string name_ = "";
        Node*       parent_ = nullptr;

        std::unordered_map<std::string, T>                     table_    = {};
        std::unordered_map<std::string, std::unique_ptr<Node>> children_ = {};
    };

    Node  root_;
    Node* current_ = &root_;

public:
    std::unordered_map<std::string, T>& table() { return current_->table_; }

public:
    void Enter(const Path& path) {
        auto* node = &root_;
        for (size_t i = 0; i < path.Length(); i++) {
            auto& child = node->children_[path[i]];
            if (!child) {
                child          = std::make_unique<Node>();
                child->name_   = path[i];
                child->parent_ = node;
            }
            node = child.get();
        }
        current_ = node;
    }
    void EnterRoot() { current_ = &root_; }

    T*   LookupTry(const std::string& name) {
        for (auto* node = current_; node; node = node->parent_) {
            auto it = node->table_.find(name);
            if (it != node->table_.end()) return &it->second;
        }
        return nullptr;
    }
    T*   LookupTry(const Path& path) {
        if (path.IsEmpty()) return nullptr;

        // A single segment is a short name in the current namespace.
        if (path.Length() == 1) return LookupTry(path[0]);

        auto* node = &root_;
        for (size_t i = 0; i + 1 < path.Length(); i++) {
            auto it = node->children_.find(path[i]);
            if (it == node->children_.end()) return nullptr;
            node = it->second.get();
        }

        auto it = node->table_.find(path.Back());
        return it == node->table_.end() ? nullptr : &it->second;
    }

    T*   GetOrCreate(const std::string& name, T value) {
        auto it = current_->table_.try_emplace(name, std::move(value)).first;
        return &it->second;
    }

    std::pair<T*, bool> Declare(const std::string& name, T value) {
        auto [it, is_inserted] = current_->table_.try_emplace(name, std::move(value));
        return { &it->second, is_inserted };
    }
};

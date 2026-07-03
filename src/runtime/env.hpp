
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <unordered_map>

#include "obj.hpp"

namespace rt {

    class Env {
    private:
        std::unordered_map<std::string, Obj> map_obj;

    public:
        void Declare(const std::string& name, const Obj& obj) {
            map_obj[name] = obj;
        }
        void Assign(const std::string& name, const Obj& value) {
            map_obj[name] = value;
        }

        const Obj* Get(const std::string& name) const {
            auto it = map_obj.find(name);
            return it != map_obj.end() ? &it->second : nullptr;
        }

        bool isExist(const std::string& name) const {
            return map_obj.find(name) != map_obj.end();
        }
    };
}

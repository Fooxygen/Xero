
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <unordered_map>

#include "obj/obj.hpp"

namespace rt {

    class Env {
    private:
        std::unordered_map<std::string, Obj> objs_;

    public:
        void Declare(const std::string& name, const Obj& obj) {
            objs_[name] = obj;
        }
        void Assign(const std::string& name, const Obj& value) {
            objs_[name] = value;
        }

        const Obj* Get(const std::string& name) const {
            if (isExist(name)) return &objs_.at(name);
            throw LogErr(LogModule::Runtime, std::format("undefined object '{}'", name));
        }

        bool isExist(const std::string& name) const {
            return objs_.contains(name);
        }
    };
}

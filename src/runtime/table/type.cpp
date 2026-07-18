
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "runtime/obj/obj.hpp"
#include "type.hpp"

namespace rt {

    void TypeTable::ConvertSet(const Type* from, const Type* to, Obj(*fn)(const Obj&)) {
        converts_[{ from, to }] = fn;
    }

    Obj  TypeTable::Convert(const Obj& obj, const Type* type) {
        if (obj.type() == type) return obj;

        auto it = converts_.find(std::pair{ obj.type(), type });
        if (it != converts_.end())
            return it->second(obj);
        return Obj();
    }

    void TypeTable::ConvertsRecompute() {

        // Clear
        for (auto& [name, type] : table_) {
            type->converts.clear();
        }

        // Recompute
        for (auto& [name, type] : table_) {

            // Emplace itself
            type->converts.emplace(type);

            // Search all path
            std::vector<const Type*> stack = { type };
            while (!stack.empty()) {
                auto* o = stack.back();
                stack.pop_back();

                for (auto& [edge, fn] : converts_) {
                    if (edge.first == o) {
                        type->converts.emplace(edge.second);
                        stack.emplace_back(edge.second);
                    }
                }
            }
        }
    }
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "runtime/obj/obj.hpp"
#include "type.hpp"

namespace rt {

    void TypeTable::ConvertSet(const Type* from, const Type* to, Obj(*fn)(const Obj&)) {
        converts_[{ from, to }] = fn;
        ConvertsRecompute();
    }
    Obj TypeTable::Convert(const Obj& obj, const Type* type) {
        if (obj.type() == type) return obj;

        auto it = converts_.find(std::pair{ obj.type(), type });
        if (it != converts_.end())
            return it->second(obj);
        return Obj();
    }
}

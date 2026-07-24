
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>

#include "log.hpp"

namespace rt {
    class Obj;
    class Type;

    class Range {
    private:
        Obj*  l_ = nullptr;
        Obj*  r_ = nullptr;
        Obj*  s_ = nullptr;
        bool  hasRBoundary_   = false;
        const rt::Type* itertype_ = nullptr;

        void Clear();

    public:
        Range(const Obj& l, const Obj& r, const Obj& s, bool hasRBoundary, const Type* itertype);
        Range(const Range& other);
        ~Range() {
            Clear();
        }

        Obj*  left() const { return l_; }
        Obj*  right() const { return r_; }
        Obj*  step() const { return s_; }
        bool  hasRBoundary()   const { return hasRBoundary_; }
        const Type* itertype() const { return itertype_; }

        Range& operator =(const Range& other);
    };
}

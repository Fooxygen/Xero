
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "range.hpp"
#include "runtime/obj/obj.hpp"

namespace rt {

    void Range::Clear()  {
        delete l_;
        delete r_;
        delete s_;
        l_ = nullptr;
        r_ = nullptr;
        s_ = nullptr;
        hasRBoundary_ = false;
        itertype_ = nullptr;
    }

    Range::Range(const Obj& l, const Obj& r, const Obj& s, bool hasRBoundary, const Type* itertype) {
        l_ = new Obj(l);
        r_ = new Obj(r);
        s_ = new Obj(s);
        hasRBoundary_ = hasRBoundary;
        itertype_ = itertype;
    }

    Range::Range(const Range& other) {
        l_ = new Obj(*other.l_);
        r_ = new Obj(*other.r_);
        s_ = new Obj(*other.s_);
        hasRBoundary_ = other.hasRBoundary_;
        itertype_ = other.itertype_;
    }

    Range& Range::operator =(const Range& other) {
        if (this == &other) return *this;

        Clear();
        l_ = new Obj(*other.l_);
        r_ = new Obj(*other.r_);
        s_ = new Obj(*other.s_);
        hasRBoundary_ = other.hasRBoundary_;
        itertype_ = other.itertype_;

        return *this;
    }
}

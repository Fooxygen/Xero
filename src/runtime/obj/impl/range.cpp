
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "range.hpp"
#include "runtime/obj/obj.hpp"

namespace rt {

    void Range::Clear()  {
        if (l_) delete l_;
        if (r_) delete r_;
        if (s_) delete s_;
        l_ = nullptr;
        r_ = nullptr;
        s_ = nullptr;
        isClosed_ = false;
        itertype_ = nullptr;
    }

    Range::Range(const Obj& l, const Obj& r, const Obj& s, bool isClosed, const sema::Type* itertype) {
        l_ = new Obj(l);
        r_ = new Obj(r);
        s_ = new Obj(s);
        isClosed_ = isClosed;
        itertype_ = itertype;
    }

    Range::Range(const Range& other) {
        l_ = new Obj(*other.l_);
        r_ = new Obj(*other.r_);
        s_ = new Obj(*other.s_);
        isClosed_ = other.isClosed_;
        itertype_ = other.itertype_;
    }

    bool Range::isSingle() const {
        Obj next = itertype_->impl_->plus_(*l_, *s_);

        if (isClosed_)  return itertype_->impl_->gt_(next, *r_).Get_bool();
        else            return itertype_->impl_->ge_(next, *r_).Get_bool();
    }

    Range Range::ToClosed() {
        if (isClosed_) return *this;

        auto r = itertype_->impl_->minus_(*r_, Obj::Make_i32(1));
        return Range(
            *l_, r, *s_, true, itertype_
        );
    }

    Range& Range::operator =(const Range& other) {
        if (this == &other) return *this;

        Clear();
        l_ = new Obj(*other.l_);
        r_ = new Obj(*other.r_);
        s_ = new Obj(*other.s_);
        isClosed_ = other.isClosed_;
        itertype_ = other.itertype_;

        return *this;
    }
}

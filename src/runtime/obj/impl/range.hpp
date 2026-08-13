
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>

#include "common/log.hpp"

namespace rt {
    class Obj;
    class Type;

    class Range {
    private:
        Obj*  l_ = nullptr;
        Obj*  r_ = nullptr;
        Obj*  s_ = nullptr;
        bool  isClosed_ = false;
        const rt::Type* itertype_ = nullptr;

        void Clear();

    public:
        Range() {}
        Range(const Obj& l, const Obj& r, const Obj& s, bool isClosed, const Type* itertype);
        Range(const Range& other);
        ~Range() { Clear(); }

        Obj*  left()            const { return l_; }
        Obj*  right()           const { return r_; }
        Obj*  step()            const { return s_; }
        bool  isClosed()        const { return isClosed_; }
        const Type* itertype()  const { return itertype_; }

        bool  isSingle() const;
        Range ToClosed();
        
        Range& operator =(const Range& other);
    };
}

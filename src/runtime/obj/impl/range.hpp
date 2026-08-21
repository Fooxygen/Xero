
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "sema/type.hpp"

namespace rt {
    class Obj;

    class Range {
    private:
        Obj*  l_ = nullptr;
        Obj*  r_ = nullptr;
        Obj*  s_ = nullptr;
        bool  isClosed_ = false;
        const sema::Type* iter_type_ = nullptr;

        void Clear();

    public:
        Range() {}
        Range(const Obj& l, const Obj& r, const Obj& s, bool isClosed, const sema::Type* iter_type);
        Range(const Range& other);
        ~Range() { Clear(); }

        Obj*  left()                    const { return l_; }
        Obj*  right()                   const { return r_; }
        Obj*  step()                    const { return s_; }
        bool  isClosed()                const { return isClosed_; }
        const sema::Type* iter_type()   const { return iter_type_; }

        bool  isSingle() const;
        Range ToClosed();
        
        Range& operator =(const Range& other);
    };
}

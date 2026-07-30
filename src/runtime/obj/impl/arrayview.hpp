
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>

#include "log.hpp"
#include "array.hpp"

namespace rt {
    class Array;

    class ArrayView {
    private:
        Array* org_    = nullptr;
        size_t len_    = 0;
        size_t offset_ = 0;

        void Clear();

    public:
        ArrayView(Array* org, size_t len, size_t offset);
        ArrayView(const ArrayView& other);
        ~ArrayView() { Clear(); }

        Array* org()    const { return org_; }
        size_t len()    const { return len_ ;}
        size_t offset() const { return offset_ ;}

        ArrayView& operator = (const ArrayView& other);
    };
}

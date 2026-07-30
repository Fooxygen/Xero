
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "arrayview.hpp"
#include "runtime/obj/obj.hpp"
#include "runtime/obj/impl/array.hpp"

namespace rt {

    ArrayView::ArrayView(Array* org, size_t len, size_t offset) {
        org_    = org;
        len_    = len;
        offset_ = offset;
    }

    ArrayView::ArrayView(const ArrayView& other) {
        org_    = other.org();
        len_    = other.len();
        offset_ = other.offset();
    }

    void ArrayView::Clear() {
        org_    = nullptr;
        len_    = 0;
        offset_ = 0;
    }

    ArrayView& ArrayView::operator =(const ArrayView& other) {
        if (this == &other) return *this;

        Clear();
        org_    = other.org();
        len_    = other.len();
        offset_ = other.offset();
        return *this;
    }
}

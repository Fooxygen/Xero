
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "stringview.hpp"
#include "runtime/obj/obj.hpp"
#include "runtime/obj/impl/string.hpp"

namespace rt {

    StringView::StringView(String* org, size_t len, size_t offset) {
        org_    = org;
        len_    = len;
        offset_ = offset;
    }

    StringView::StringView(const StringView& other) {
        org_    = other.org();
        len_    = other.len();
        offset_ = other.offset();
    }

    void StringView::Clear() {
        org_    = nullptr;
        len_    = 0;
        offset_ = 0;
    }

    StringView& StringView::operator =(const StringView& other) {
        if (this == &other) return *this;

        Clear();
        org_    = other.org();
        len_    = other.len();
        offset_ = other.offset();
        return *this;
    }
}

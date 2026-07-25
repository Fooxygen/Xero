
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "stringview.hpp"
#include "runtime/obj/impl/string.hpp"
#include "runtime/obj/impl/range.hpp"

namespace rt {

    StringView::StringView(String* str, Range* range) {
        str_   = str;
        range_ = new Range(*range);
    }

    StringView::StringView(const StringView& other) {
        str_   = other.str_;
        range_ = new Range(*other.range_);
    }

    void StringView::Clear() {
        delete range_;
        str_   = nullptr;
        range_ = nullptr;
    }

    StringView& StringView::operator =(const StringView& other) {
        if (this == &other) return *this;
        Clear();
        str_   = other.str_;
        range_ = new Range(*other.range_);
        return *this;
    }
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>

#include "log.hpp"
#include "string.hpp"
#include "range.hpp"

namespace rt {
    class String;
    class Range;

    class StringView {
    private:
        String* str_   = nullptr;
        Range*  range_ = nullptr;

        void Clear();

    public:
        StringView(String* str, Range* range);
        StringView(const StringView& other);
        ~StringView() {
            Clear();
        }

        String* str()   const { return str_; }
        Range*  range() const { return range_; }

        StringView& operator = (const StringView& other);
    };
}

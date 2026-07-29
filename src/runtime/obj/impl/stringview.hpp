
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>

#include "log.hpp"
#include "string.hpp"

namespace rt {
    class String;

    class StringView {
    private:
        String* org_    = nullptr;
        size_t  len_    = 0;
        size_t  offset_ = 0;

        void Clear();

    public:
        StringView(String* org, size_t len, size_t offset);
        StringView(const StringView& other);
        ~StringView() {
            Clear();
        }

        String* org()    const { return org_; }
        size_t  len()    const { return len_ ;}
        size_t  offset() const { return offset_ ;}

        StringView& operator = (const StringView& other);
    };
}

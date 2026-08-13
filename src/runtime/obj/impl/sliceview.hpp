
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>

#include "common/log.hpp"

namespace rt {

    template <typename T>
    class SliceView {
    private:
        T*     org_    = nullptr;
        size_t len_    = 0;
        size_t offset_ = 0;

    public:
        SliceView(T* org = nullptr, size_t len = 0, size_t offset = 0)
            : org_(org), len_(len), offset_(offset) {}
        SliceView(const SliceView& other)
            : org_(other.org_), len_(other.len_), offset_(other.offset_) {}

        T*     org()    const { return org_; }
        size_t len()    const { return len_ ;}
        size_t offset() const { return offset_ ;}

        SliceView& operator = (const SliceView& other) {
            if (this == &other) return *this;

            org_    = other.org();
            len_    = other.len();
            offset_ = other.offset();
            return *this;
        }
    };
}

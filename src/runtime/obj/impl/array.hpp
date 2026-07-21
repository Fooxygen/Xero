
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace rt {
    class Obj;
    
    class Array {
    private:
        Obj**  data_     = nullptr;
        size_t size_     = 0;
        size_t capacity_ = 0;

        void Clear();

        // isBoundaryEq: Allowed to modify at the [max + 1] idx
        void IndexCheck(size_t idx, bool isBoundaryEq = false) const;
        void Expand(size_t size);

    public:
        Array(size_t size = 1) {
            data_ = new Obj*[size];
            memset(data_, 0, sizeof(Obj*) * size);
            capacity_ = size;
        }
        Array(const Array& other);
        ~Array() {
            Clear();
        }

        size_t size() const {
            return size_;
        }
        size_t capacity() const {
            return capacity_;
        }
        
        Obj* Get(size_t idx) const;
        void Reverse();
        void Insert(size_t idx, Obj* obj);
        void Remove(size_t idx);

        Array* operator + (const Array& other);
        Array& operator = (const Array& other);
        Array& operator +=(const Array& other);
    };
}

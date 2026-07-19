
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

    public:
        Array() {
            data_ = new Obj*[1];
            data_[0] = nullptr;
            capacity_ = 1;
        }
        ~Array();

        size_t size() const {
            return size_;
        }
        size_t capacity() const {
            return capacity_;
        }
    
        // isBoundaryEq: Allowed to modify at the [max + 1] idx
        void IndexCheck(size_t idx, bool isBoundaryEq = false);
        void Expand(size_t size);
        
        void Reverse();
        void Insert(size_t idx, Obj* obj);
        void Remove(size_t idx);
        Obj* Get(size_t idx);
    };
}

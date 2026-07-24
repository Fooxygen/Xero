
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "log.hpp"

namespace rt {
    class Obj;
    
    class Array {
    private:
        Obj**  data_     = nullptr;
        size_t size_     = 0;
        size_t capacity_ = 0;

        void Clear();

        // hasRBoundary: Allowed to modify at the [max + 1] idx
        void IndexCheck(size_t idx, bool hasRBoundary = false) const {
            if (!hasRBoundary) {
                if (idx >= size_) {
                    throw LogErr(LogModule::Runtime, "array index out of bounds");
                }
            }
            else {
                if (idx >= size_ + 1) {
                    throw LogErr(LogModule::Runtime, "array index out of bounds");
                }
            }
        }
        void Expand(size_t size);

    public:
        Array(size_t size = 1);
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

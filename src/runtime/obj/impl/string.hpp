
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>
#include <cstring>
#include <string>

#include "log.hpp"

namespace rt {
    class Obj;

    class String {
    private:
        char*  data_     = nullptr;
        size_t length_   = 0;
        size_t capacity_ = 0;

        void Clear();
        
        // isBoundaryEq: Allowed to modify at the [max + 1] idx
        void IndexCheck(size_t idx, bool isBoundaryEq = false) const {
            if (!isBoundaryEq) {
                if (idx >= length_) {
                    throw LogErr(LogModule::Runtime, "string index out of bounds");
                }
            }
            else {
                if (idx >= length_ + 1) {
                    throw LogErr(LogModule::Runtime, "string index out of bounds");
                }
            }
        }
        void Expand(size_t len);
        void Write(const std::string& s);

    public:
        String();
        String(const std::string& s);
        String(const String& other);
        ~String() {
            Clear();
        }

        size_t length() const {
            return length_;
        }
        size_t capacity() const {
            return capacity_;
        }

        std::string ToCppString() const {
            return std::string(data_, length_);
        }
        char Get(int32_t idx);
        void Reverse();
        void ReplaceChar(int32_t idx, char c);
        
        String* operator + (const String& other) const;
        String& operator = (const String& other);
        String& operator +=(const String& other);
    };
}

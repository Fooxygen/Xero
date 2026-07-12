
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>
#include <cstring>
#include <string>

namespace rt {
    
    class String {
    private:
        char*  data_     = nullptr;
        size_t length_   = 0;
        size_t capacity_ = 0;

    public:
        String() {
            data_ = new char[1];
            data_[0] = '\0';
            capacity_ = 1;
        }
        String(std::string s) : String() {
            Write(s.c_str());
        }
        String(const String& other) {
            data_ = new char[other.capacity_];
            length_ = other.length_;
            capacity_ = other.capacity_;
            memcpy(data_, other.data_, length_ + 1);
        }
        ~String() {
            delete[] data_;
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
        void Expand(size_t len) {
            if (capacity_ <= 0) capacity_ = 1;
            if (len + 1 < capacity_) return;

            size_t new_capacity = std::max((len + 1) * 2, capacity_ * 2);
            auto new_data = new char[new_capacity];
            memset(new_data, 0, sizeof(char) * new_capacity);
            memcpy(new_data, data_, sizeof(char) * capacity_);

            delete[] data_;
            data_ = new_data;
            capacity_ = new_capacity;
        }
        void Write(const char* s) {
            auto len = std::strlen(s);
            Expand(len);
            length_ = len;
            memcpy(data_, s, sizeof(char) * (len + 1));
        }

        String& operator = (const String& other) {
            if (this == &other) return *this;

            delete[] data_;
            data_ = new char[other.capacity_];
            length_ = other.length_;
            capacity_ = other.capacity_;
            memcpy(data_, other.data_, length_ + 1);

            return *this;
        }
        String  operator + (const String& other) const {
            String res;
            res.Expand(length_ + other.length_);
            memcpy(res.data_, data_, length_);
            memcpy(res.data_ + length_, other.data_, other.length_);
            res.length_ = length_ + other.length_;
            res.data_[res.length_] = '\0';
            return res;
        }
        String& operator +=(const String& other) {
            *this = *this + other;
            return *this;
        }
    };
}

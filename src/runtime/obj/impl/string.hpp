
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
        String(const std::string& s) : String() {
            Write(s);
        }
        String(const String& other) {
            data_ = new char[other.capacity_];
            length_ = other.length_;
            capacity_ = other.capacity_;
            memcpy(data_, other.data_, length_ + 1);
        }
        ~String() {
            if (data_) delete[] data_;
        }

        size_t length() const {
            return length_;
        }
        size_t capacity() const {
            return capacity_;
        }
        
        void Expand(size_t len) {
            if (len + 1 < capacity_) return;

            size_t capa   = std::max((size_t)((len + 1) * 1.5f), (size_t)(capacity_ * 1.5f));
            auto   expand = new char[capa];
            memset(expand, 0, sizeof(char) * capa);
            memcpy(expand, data_, sizeof(char) * capacity_);

            delete[] data_;
            data_ = expand;
            capacity_ = capa;
        }
        void Write(const std::string& s) {
            auto len = s.length();
            Expand(len);
            length_ = len;
            memcpy(data_, s.data(), sizeof(char) * (len + 1));
        }

        std::string ToCppString() const {
            return std::string(data_, length_);
        }
        void Reverse() {
            if (length_ <= 1) return;
            for (size_t i = 0, j = length_ - 1; i < j; i++, j--) {
                char t = data_[i];
                data_[i] = data_[j];
                data_[j] = t;
            }
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

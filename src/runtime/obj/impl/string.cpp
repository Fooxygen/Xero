
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "string.hpp"

namespace rt {

    void String::Clear() {
        if (data_) delete[] data_;
        data_ = nullptr;
        length_ = 0;
        capacity_ = 0;
    }

    void String::Expand(size_t len) {
        if (len + 1 < capacity_) return;

        size_t capa   = std::max((size_t)((len + 1) * 1.5f), (size_t)(capacity_ * 1.5f));
        auto   expand = new char[capa];
        memset(expand, 0, sizeof(char) * capa);
        memcpy(expand, data_, sizeof(char) * capacity_);

        Clear();
        data_ = expand;
        capacity_ = capa;
    }

    void String::Write(const std::string& s) {
        auto len = s.length();
        Expand(len);
        length_ = len;
        memcpy(data_, s.data(), sizeof(char) * (len + 1));
    }

    String::String() {
        data_ = new char[1];
        data_[0] = '\0';
        capacity_ = 1;
    }

    String::String(const std::string& s) : String() {
        Write(s);
    }

    String::String(const String& other) {
        data_ = new char[other.capacity_];
        length_ = other.length_;
        capacity_ = other.capacity_;
        memcpy(data_, other.data_, length_ + 1);
    }

    char String::Get(int32_t idx) {
        IndexCheck(idx);
        return data_[idx];
    }

    void String::Reverse() {
        if (length_ <= 1) return;
        for (size_t i = 0, j = length_ - 1; i < j; i++, j--) {
            char t = data_[i];
            data_[i] = data_[j];
            data_[j] = t;
        }
    }

    void String::ReplaceChar(int32_t idx, char c) {
        IndexCheck(idx);
        data_[idx] = c;
    }

    String* String::operator +(const String& other) const {
        auto str = new String();
        str->Expand(length_ + other.length_);
        memcpy(str->data_, data_, length_);
        memcpy(str->data_ + length_, other.data_, other.length_);

        str->length_ = length_ + other.length_;
        str->data_[str->length_] = '\0';
        return str;
    }
    
    String& String::operator =(const String& other) {
        if (this == &other) return *this;

        Clear();
        data_ = new char[other.capacity_];
        length_ = other.length_;
        capacity_ = other.capacity_;
        memcpy(data_, other.data_, length_ + 1);

        return *this;
    }
    
    String& String::operator +=(const String& other) {
        Expand(length_ + other.length_);
        memcpy(data_ + length_, other.data_, sizeof(char) * other.length_);
        length_ += other.length_;
        return *this;
    }
}

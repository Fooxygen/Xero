
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "xengine/obj/obj.hpp"
#include "xengine/obj/impl/string.hpp"

namespace xengine {

    String::String(const std::string& s)
    :   Array(sema::TypeTable::Lookup("char"))
    {
        for (size_t i = 0; i < s.size(); i++) {
            Insert(size(), new Obj(Obj::Make_char(s[i])));
        }
    }

    String::String(const String& other)
    :   Array(other) {}

    std::string String::ToCppString() const {
        std::string s;
        s.reserve(size());
        for (size_t i = 0; i < size(); i++) {
            s += Get(i)->Get_char();
        }
        return s;
    }

    String* String::operator +(const String& other) const {
        auto* str = new String();
        for (size_t i = 0; i < size(); i++) {
            str->Insert(str->size(), new Obj(*Get(i)));
        }
        for (size_t i = 0; i < other.size(); i++) {
            str->Insert(str->size(), new Obj(*other.Get(i)));
        }
        return str;
    }

    String& String::operator =(const String& other) {
        Array::operator=(other);
        return *this;
    }

    String& String::operator +=(const String& other) {
        for (size_t i = 0; i < other.size(); i++) {
            Insert(size(), new Obj(*other.Get(i)));
        }
        return *this;
    }
}

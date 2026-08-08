
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>
#include <cstring>
#include <string>

#include "log.hpp"
#include "array.hpp"

namespace rt {
    class Obj;

    class String : public Array {
    public:
        String() = default;
        String(const std::string& s);
        String(const String& other);

        std::string ToCppString() const;
        
        String* operator + (const String& other) const;
        String& operator = (const String& other);
        String& operator +=(const String& other);
    };
}


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstddef>
#include <string>

// Row and column of Token
class Loc {
private:
    size_t line_ = 1;
    size_t col_  = 1;
    std::string_view module_;

public:
    Loc() {}
    Loc(size_t line, size_t col, std::string_view module)
    :   line_(line), col_(col), module_(module) {}

    size_t line() const { return line_; }
    size_t col()  const { return col_; }
    std::string_view module() const { return module_; }

public:
    void ColNext() { col_++; }
    void LineNext() { line_++; col_ = 1; }
};

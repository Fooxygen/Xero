
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstddef>

// Row and column of Token
class Loc {
public:
    size_t line_ = 1;
    size_t col_  = 1;

    void NextChar() { col_++; }
    void NextLine() { line_++; col_ = 1; }
};

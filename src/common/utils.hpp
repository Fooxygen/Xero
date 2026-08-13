
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstddef>

class Loc {
public:
    size_t line = 1;
    size_t col  = 1;

    void NextChar() { col++; }
    void NextLine() { line++; col = 1; }
};

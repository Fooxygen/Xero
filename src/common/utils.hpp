
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstddef>
#include <string>

// Row and column of Token
class Loc {
public:
    size_t line_ = 1;
    size_t col_  = 1;

    void NextChar() { col_++; }
    void NextLine() { line_++; col_ = 1; }
};

// Visualized Escape Char in String
inline std::string ContainedEscapePrint(const std::string& s) {
    std::string res = "";
    for (char c : s) {
        switch (c) {
            case '\n': res += "\\n";    break;
            case '\r': res += "\\r";    break;
            case '\t': res += "\\t";    break;
            case '\\': res += "\\\\";   break;
            default:   res += c;        break;
        }
    }
    return res;
}

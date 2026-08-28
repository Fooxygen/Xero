
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <ranges>
#include <functional>

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

// Join range elements into a formatted string
template <std::ranges::input_range R, typename Fn>
requires std::invocable<Fn, std::ranges::range_value_t<R>>
inline std::string JoinWithBoundary (
    const R& elements,
    Fn to_string,
    std::string_view left  = "(",
    std::string_view right = ")",
    std::string_view sep   = ", "
) {
    std::ostringstream oss;
    oss << left;

    bool isFirst = true;
    for (const auto& e : elements) {
        if (!isFirst) oss << sep;
        oss << std::invoke(to_string, e);
        isFirst = false;
    }

    oss << right;
    return oss.str();
}

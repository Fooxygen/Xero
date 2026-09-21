
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <sstream>
#include <string>
#include <ranges>
#include <functional>

namespace format {

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

    // Join range elems into a formatted string
    template <std::ranges::input_range R, typename Fn>
    requires std::invocable<Fn, std::ranges::range_value_t<R>>
    inline std::string JoinWithBoundary (
        const R& elems,
        Fn to_string,
        std::string_view left  = "(",
        std::string_view right = ")",
        std::string_view sep   = ", "
    ) {
        std::ostringstream oss;
        oss << left;

        bool is_first_elem = true;
        for (const auto& e : elems) {
            if (!is_first_elem) oss << sep;
            oss << std::invoke(to_string, e);
            is_first_elem = false;
        }

        oss << right;
        return oss.str();
    }
}

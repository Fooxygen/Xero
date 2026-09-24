
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>
#include <format>

#include "common/log.hpp"

class UTF8 {
private:
    struct Invalid {};

public:
    static size_t BytesCntGet(uint8_t bytes_first, LogModule log_module) {
        try {
            if (bytes_first < 0x80) return 1;
            if (bytes_first < 0xc0) throw  Invalid{};
            if (bytes_first < 0xe0) return 2;
            if (bytes_first < 0xf0) return 3;
            if (bytes_first < 0xf8) return 4;
            throw Invalid{};
        } catch (const Invalid&) {
            throw LogErr(log_module, std::format(
                "invalid utf8 leading byte 0x{:02x}", bytes_first
            ));
        }
    }

    static void   Decode(const uint8_t* bytes, size_t bytes_cnt, uint32_t& out_codepoint, LogModule log_module) {
        try {
            if (bytes_cnt == 0) throw Invalid{};

            auto cnt = BytesCntGet(bytes[0], log_module);
            if (cnt > bytes_cnt) throw Invalid{};

            uint8_t bytes_first_mask[] = {
                0x7f, 0x1f, 0x0f, 0x07
            };
            out_codepoint = bytes[0] & bytes_first_mask[cnt - 1];

            for (size_t i = 1; i < cnt; i++) {
                if ((bytes[i] & 0xc0) != 0x80) throw Invalid{};
                out_codepoint <<= 6;
                out_codepoint |= bytes[i] & 0x3f;
            }

        } catch (const Invalid&) {
            throw LogErr(log_module, "invalid utf8 sequence");
        }
    }
};

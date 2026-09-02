
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>
#include <memory>

#include "common/log.hpp"

class UTF8 {
public:
    static size_t CharBytesGet(uint8_t first_byte, LogModule log_module) {
        try {
            if (first_byte < 0x80) return 1;
            if (first_byte < 0xc0) throw  0;
            if (first_byte < 0xe0) return 2;
            if (first_byte < 0xf0) return 3;
            if (first_byte < 0xf8) return 4;
            throw 0;
        } catch (...) {
            LogErr(log_module, "invalid");
            throw;
        }
    }

    static void   Encode(uint32_t codepoint, uint8_t*& out_bytes, size_t& out_bytes_len, LogModule log_module) {
        try {
                 if (codepoint < 0x80) {
                out_bytes = (uint8_t*)malloc(sizeof(uint8_t));
                out_bytes[0] = (uint8_t)codepoint;
                out_bytes_len = 1;
            }
            else if (codepoint < 0x800) {
                out_bytes = (uint8_t*)malloc(sizeof(uint8_t) * 2);
                out_bytes[0] = 0xc0 | (codepoint >> 6);
                out_bytes[1] = 0x80 | (codepoint &  0x3f);
                out_bytes_len = 2;
            }
            else if (codepoint < 0x10000) {
                out_bytes = (uint8_t*)malloc(sizeof(uint8_t) * 3);
                out_bytes[0] = 0xe0 | (codepoint  >> 12);
                out_bytes[1] = 0x80 | ((codepoint >> 6) & 0x3f);
                out_bytes[2] = 0x80 | (codepoint  &  0x3f);
                out_bytes_len = 3;
            }
            else if (codepoint < 0x110000) {
                out_bytes = (uint8_t*)malloc(sizeof(uint8_t) * 4);
                out_bytes[0] = 0xf0 | (codepoint  >> 18);
                out_bytes[1] = 0x80 | ((codepoint >> 12) & 0x3f);
                out_bytes[2] = 0x80 | ((codepoint >> 6)  & 0x3f);
                out_bytes[3] = 0x80 | (codepoint  &  0x3f);
                out_bytes_len = 4;
            }
            throw 0;
        } catch(...) {
            LogErr(log_module, "invalid");
            throw;
        }
    }

    static void   Decode(const uint8_t* bytes, size_t bytes_len, uint32_t& out_codepoint, LogModule log_module) {
        try {
            if (bytes_len == 0) throw 0;
            auto bytes_get = CharBytesGet(bytes[0], log_module);
            if (bytes_get > bytes_len) throw 0;

            uint8_t first_masks[] = {
                0x7f, 0x1f, 0x0f, 0x07
            };
            out_codepoint = bytes[0] & first_masks[bytes_get - 1];

            for (size_t i = 1; i < bytes_get; i++) {
                if ((bytes[i] & 0xc0) != 0x80) throw 0;
                out_codepoint <<= 6;
                out_codepoint |= bytes[i] & 0x3f;
            }

        } catch(...) {
            LogErr(log_module, "invalid utf8 first char");
            throw;
        }
    }
};

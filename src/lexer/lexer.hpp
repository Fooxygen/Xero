
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>

#include "common/utils.hpp"
#include "common/token.hpp"

namespace lexer {
    using TT = Token::Type;

    // Lexical Analyzer
    class Lexer {
    private:
        std::string_view code_;

        Loc    loc_;        // Current Location
        Loc    loc_prev_;   // Previous Location
        Loc    loc_scan_;   // Beginning of Current Location
        size_t pos_ = 0;

        std::vector<Token> tokens_;

        static constexpr bool isAlpha(char c) {
            return
                (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z');
        }
        static constexpr bool isNumber(char c) {
            return c >= '0' && c <= '9';
        }
        static constexpr bool isDot(char c) {
            return c == '.';
        }
        static constexpr bool isIdentifierChar(char c) {
            return isAlpha(c) || isNumber(c) || c == '_';
        }
        
        void  WhitespaceSkip() {
            while (!isScanEnd()) {
                char c = code_[pos_];
                if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
                    CharNext();
                
                else break;
            }
        }

        void  CharNext() {
            if (code_[pos_] == '\n') loc_.NextLine();
            else loc_.NextChar();
            
            pos_++;
        }
        void  CharNext(size_t cnt) {
            for (size_t i = 0; i < cnt; i++) CharNext();
        }
        Token TokenScanWord();
        Token TokenScanNumber();
        Token TokenScanChar();
        Token TokenScanString();
        Token TokenScanSingleComment();
        Token TokenScanMultiComment();

    public:
        Lexer(std::string_view code)
        :   code_(code),
            pos_(0)
        {}

        std::vector<Token>& tokens() { return tokens_; }

        bool  isScanEnd()     { return pos_ >= code_.length(); };
        bool  isNextScanEnd() { return pos_ + 1 >= code_.length(); };
        
        std::optional<Token> TokenNext();
        Token                TokenGen(TT type, const std::string& lexeme);
        void                 TokensGen(bool isPrint = false);
    };
}

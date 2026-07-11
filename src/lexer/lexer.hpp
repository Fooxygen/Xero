
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>

#include "common/token.hpp"

namespace lexer {

    // Lexical Analyzer
    class Lexer {
    private:
        std::string_view code_;

        size_t pos_  = 0;
        size_t line_ = 1;
        size_t col_  = 1;
        size_t prev_token_line_ = line_;
        size_t prev_token_col_  = col_;

        std::vector<Token> tokens_;

        static constexpr bool isAlpha(char c) {
            return
                (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z');
        }
        static constexpr bool isNumber(char c) {
            return c >= '0' && c <= '9';
        }
        static constexpr bool isAlphaOrNumber(char c) {
            return isAlpha(c) || isNumber(c);
        }
        static constexpr bool isNumberOrDot(char c) {
            return isNumber(c) || c == '.';
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
            if (code_[pos_] == '\n') {
                line_++;
                col_ = 1;
            }
            else col_++;
            
            pos_++;
        }
        void  CharNext(size_t cnt) {
            for (size_t i = 0; i < cnt; i++) CharNext();
        }
        Token TokenScanWord();
        Token TokenScanNumber();
        Token TokenScanString();
        Token TokenScanSingleComment();
        Token TokenScanMultiComment();

    public:
        Lexer(std::string_view code)
        :   code_(code),
            pos_(0)
        {}

        bool  isScanEnd() {
            return pos_ >= code_.length();
        };
        bool  isNextScanEnd() {
            return pos_ + 1 >= code_.length();
        };
        
        std::optional<Token> TokenNext();
        Token TokenGen(Token::Type type, const std::string& lexeme) {
            prev_token_line_ = line_;
            prev_token_col_  = col_ - lexeme.length();
            return Token{
                .type   = type,
                .lexeme = lexeme,
                .line   = prev_token_line_,
                .col    = prev_token_col_
            };
        }
        void  TokensGen(bool isPrint = false);
        
        const std::vector<Token>& tokens() const {
            return tokens_;
        }
    };
}

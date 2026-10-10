
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>

#include "common/utils/loc.hpp"
#include "common/defs/token.hpp"
#include "context/module.hpp"

namespace lexer {
    using TT = Token::Type;

    // Lexical Analyzer
    class Lexer {
    private:
        context::Module& module_;

        Loc    loc_;        // current location
        Loc    loc_prev_;   // previous location
        Loc    loc_scan_;   // beginning of current location
        size_t pos_ = 0;

        std::vector<Token> tokens_;

        static constexpr bool IsAlpha(char c) {
            return
                (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z');
        }
        static constexpr bool IsNumber(char c) {
            return c >= '0' && c <= '9';
        }
        static constexpr bool IsDot(char c) {
            return c == '.';
        }
        static constexpr bool IsIdBegin(char c) {
            return IsAlpha(c) || c == '_'   || c == '@';
        }
        static constexpr bool IsIdContinue(char c) {
            return IsAlpha(c) || IsNumber(c) ||
                   c == '_'   || c == '@';
        }
        
        void  WhitespaceSkip() {
            while (!IsScanEnd()) {
                char c = module_.code()[pos_];
                if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
                    CharNext();
                
                else break;
            }
        }

        void  CharNext() {
            if (module_.code()[pos_] == '\n') loc_.LineNext();
            else loc_.ColNext();
            
            pos_++;
        }
        void  CharNext(size_t cnt) {
            for (size_t i = 0; i < cnt; i++) CharNext();
        }
        bool  IsScanEnd()     const { return pos_ >= module_.code().length(); }
        bool  IsNextScanEnd() const { return pos_ + 1 >= module_.code().length(); }
        
        std::optional<Token> TokenNext();
        Token TokenGen(TT type, const std::string& lexeme);
        Token TokenScanWord();
        Token TokenScanNumber();
        Token TokenScanChar();
        Token TokenScanString();
        Token TokenScanSingleComment();
        Token TokenScanMultiComment();

    public:
        Lexer(context::Module& module)
        :   module_(module),
            loc_(1, 1, module.name()),
            loc_prev_(1, 1, module.name()),
            loc_scan_(1, 1, module.name()),
            pos_(0)
        {}

        std::vector<Token>& tokens() { return tokens_; }

    public:
        void Run(bool is_print = false);
    };
}

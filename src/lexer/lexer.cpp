
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <iostream>

#include "log.hpp"
#include "lexer.hpp"

namespace lexer {

    void Lexer::TokensGen(bool isPrint) {
        if (isPrint) {
            LogStart(LogModule::Lexer, "output tokens").Print();
        }
        while (!isScanEnd()) {
            auto next = TokenNext();
            if (next.type != Token::Type::Undefined) {
                auto& token = tokens_.emplace_back(next);

                if (isPrint && token.type != Token::Type::Undefined)
                std::cout << token.MetaPrint() << std::endl;
            }
        }
        if (isPrint) {
            LogFinish(LogModule::Lexer, "output tokens").Print();
        }
    }

    Token Lexer::TokenNext() {
        WhitespaceSkip();
        if (isScanEnd()) return Token{};

        char c = code_[pos_];

        // Literal | DataType
        if (isAlpha(c)) return TokenScanWord();

        // Number
        if (isNumber(c)) return TokenScanNumber();

        // Punctuation
        CharNext();
        switch (c) {
            case ':':   return TokenGen(Token::Type::Colon,     ":");
            case '=':   return TokenGen(Token::Type::Assign,    "=");
            case ';':   return TokenGen(Token::Type::Semicolon, ";");
            case '(':   return TokenGen(Token::Type::LParen,    "(");
            case ')':   return TokenGen(Token::Type::RParen,    ")");
            case '.':   return TokenGen(Token::Type::Dot,       ".");
            case '+':   return TokenGen(Token::Type::Plus,      "+");
            case '-':   return TokenGen(Token::Type::Minus,     "-");
            case '*':   return TokenGen(Token::Type::Star,      "*");
            case '/':   return TokenGen(Token::Type::Slash,     "/");
            default:
                throw LogErr(LogModule::Lexer, "failed to identify token");
        }
    }

    Token Lexer::TokenScanWord() {
        size_t pbeg = pos_;
        while (pos_ + 1 < code_.length() && isAlphaOrNumber(code_[pos_ + 1])) {
            CharNext();
        }
        CharNext();
        std::string lexeme = std::string(code_.substr(pbeg, pos_ - pbeg));

        return TokenGen(Token::Type::Id, lexeme);
    }

    Token Lexer::TokenScanNumber() {
        size_t pbeg = pos_;
        while (pos_ + 1 < code_.length() && isNumberOrDot(code_[pos_ + 1])) {
            CharNext();
        }
        CharNext();

        return TokenGen(
            Token::Type::Number,
            std::string(code_.substr(pbeg, pos_ - pbeg))
        );
    }

}


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
            auto next_opt = TokenNext();
            if (!next_opt.has_value()) break;

            auto next = next_opt.value();
            if (next.type() != Token::Type::Undefined) {
                auto& token = tokens_.emplace_back(next);

                if (isPrint && token.type() != Token::Type::Undefined)
                    std::cout << token.MetaPrint() << std::endl;
            }
        }
        if (isPrint) {
            LogFinish(LogModule::Lexer, "output tokens").Print();
        }
    }

    std::optional<Token> Lexer::TokenNext() {
        WhitespaceSkip();
        if (isScanEnd()) return std::nullopt;

        char c = code_[pos_];

        // Indef Length
        if (isAlpha(c))  return TokenScanWord();            // a...
        if (isNumber(c)) return TokenScanNumber();          // 1...
        if (c == '"')    return TokenScanString();          // "..."
        
        // Fixed Length
        CharNext();

        // └─ Multiple Chars
        char nextc = !isNextScanEnd() ? code_[pos_] : '\0';
        switch (c) {
            case '=': {
                if (nextc == '=') {
                    CharNext();
                    return TokenGen(Token::Type::Eq, "==");
                }
                else
                    return TokenGen(Token::Type::Assign, "=");
            }
            case '!': {
                if (nextc == '=') {
                    CharNext();
                    return TokenGen(Token::Type::Neq, "!=");
                }
                else
                    return TokenGen(Token::Type::Not, "!");
            }
            case '>': {
                if (nextc == '=') {
                    CharNext();
                    return TokenGen(Token::Type::Ge, ">=");
                }
                else
                    return TokenGen(Token::Type::Gt, ">");
            }
            case '<': {
                if (nextc == '=') {
                    CharNext();
                    return TokenGen(Token::Type::Le, "<=");
                }
                else
                    return TokenGen(Token::Type::Lt, "<");
            }
            case '&': {
                if (nextc == '&') {
                    CharNext();
                    return TokenGen(Token::Type::And, "&&");
                }
            }
            case '|': {
                if (nextc == '|') {
                    CharNext();
                    return TokenGen(Token::Type::Or, "||");
                }
            }
        }

        // └─ Single Char
        switch (c) {
            case '#': {
                bool isMultiLineCommon =
                    !isScanEnd() && code_[pos_] == '#'; // next '#' has been received
                if (isMultiLineCommon) {
                    CharNext();
                    return TokenScanMultiComment();
                }
                else return TokenScanSingleComment();
            }
            case ':':   return TokenGen(Token::Type::Colon,     ":");
            case ';':   return TokenGen(Token::Type::Semicolon, ";");
            case '(':   return TokenGen(Token::Type::LParen,    "(");
            case ')':   return TokenGen(Token::Type::RParen,    ")");
            case '{':   return TokenGen(Token::Type::LBrace,    "{");
            case '}':   return TokenGen(Token::Type::RBrace,    "}");
            case '.':   return TokenGen(Token::Type::Dot,       ".");
            case ',':   return TokenGen(Token::Type::Comma,     ",");
            case '+':   return TokenGen(Token::Type::Plus,      "+");
            case '-':   return TokenGen(Token::Type::Minus,     "-");
            case '*':   return TokenGen(Token::Type::Star,      "*");
            case '/':   return TokenGen(Token::Type::Slash,     "/");
        }

        throw LogErr(LogModule::Lexer, "invalid token");
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

    Token Lexer::TokenScanString() {
        std::string lexeme = "";
        CharNext();

        while (!isScanEnd()) {
            char c = code_[pos_];

            if (c == '"') {
                CharNext();
                return TokenGen(Token::Type::String, lexeme);
            }

            if (c == '\n')
                throw LogErr(LogModule::Lexer, "unclosed string");

            if (c == '\\') {
                CharNext();
                
                if (isScanEnd())
                    throw LogErr(LogModule::Lexer, "unclosed string");

                switch (code_[pos_]) {
                    case 'n':  lexeme += '\n'; break;
                    case 't':  lexeme += '\t'; break;
                    case 'r':  lexeme += '\r'; break;
                    case '"':  lexeme += '"';  break;
                    case '\\': lexeme += '\\'; break;
                    default:   throw LogErr(LogModule::Lexer, std::format("unknown escape '{}'", code_[pos_]));
                }
                CharNext();
                continue;
            }

            lexeme += c;
            CharNext();
        }

        throw LogErr(LogModule::Lexer, "unclosed string");
    }

    Token Lexer::TokenScanSingleComment() {
        while (!isScanEnd() && code_[pos_] != '\n') CharNext();
        return Token();
    }

    Token Lexer::TokenScanMultiComment() {
        while (!isScanEnd()) {
            if (code_[pos_] == '#' &&
                pos_ + 1 < code_.size() &&
                code_[pos_ + 1] == '#')
            {
                CharNext(2);
                return Token();
            }
            CharNext();
        }
        return Token();
    }
}

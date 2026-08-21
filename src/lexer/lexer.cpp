
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <format>

#include "common/log.hpp"
#include "lexer/lexer.hpp"

namespace lexer {

    Token Lexer::TokenGen(TT type, const std::string& lexeme) {
        loc_prev_ = {
            .line_ = loc_.line_,
            .col_  = loc_.col_ - lexeme.length()
        };
        return Token(type, lexeme, loc_prev_);
    }

    void  Lexer::TokensGen(bool isPrint) {
        if (isPrint) {
            LogStart(LogModule::Lexer, "output tokens").Print();
        }
        while (!isScanEnd()) {
            auto next_opt = TokenNext();
            if (!next_opt.has_value()) break;

            auto next = next_opt.value();
            if (next.type_ != TT::Undefined) {
                auto& token = tokens_.emplace_back(next);

                if (isPrint && token.type_ != TT::Undefined) token.MetaPrint();
            }
        }
        if (isPrint) {
            LogFinish(LogModule::Lexer, "output tokens").Print();
        }
    }

    std::optional<Token> Lexer::TokenNext() {
        WhitespaceSkip();
        if (isScanEnd()) return std::nullopt;

        loc_scan_ = loc_;
        char c = code_[pos_];

        // Indef Length
        if (isAlpha(c))  return TokenScanWord();            // a...
        if (isNumber(c)) return TokenScanNumber();          // 1...
        if (c == '"')    return TokenScanString();          // "..."
        if (c == '\'')   return TokenScanChar();            // '.'
        
        // Fixed Length
        CharNext();

        // └─ Multiple Chars
        char cn  = !isScanEnd()     ? code_[pos_]     : '\0';
        char cnn = !isNextScanEnd() ? code_[pos_ + 1] : '\0';
        switch (c) {
            case '=': {
                if (cn == '=') {
                    CharNext();
                    return TokenGen(TT::Eq, "==");
                }
                if (cn == ']') {
                    CharNext();
                    return TokenGen(TT::REBkt, "=]");
                }
                return TokenGen(TT::Assign, "=");
            }
            case '+': {
                if (cn == '=') {
                    CharNext();
                    return TokenGen(TT::PlusAssign, "+=");
                }
                return TokenGen(TT::Plus, "+");
            }
            case '-': {
                if (cn == '=') {
                    CharNext();
                    return TokenGen(TT::MinusAssign, "-=");
                }
                if (cn == '>') {
                    CharNext();
                    return TokenGen(TT::Arrow, "->");
                }
                return TokenGen(TT::Minus, "-");
            }
            case '*': {
                if (cn == '=') {
                    CharNext();
                    return TokenGen(TT::StarAssign, "*=");
                }
                return TokenGen(TT::Star, "*");
            }
            case '/': {
                if (cn == '=') {
                    CharNext();
                    return TokenGen(TT::SlashAssign, "/=");
                }
                return TokenGen(TT::Slash, "/");
            }
            case '%': {
                if (cn == '%') {
                    if (cnn == '=') {
                        CharNext(2);
                        return TokenGen(TT::ModFAssign, "%%=");
                    }
                    
                    CharNext();
                    return TokenGen(TT::ModF, "%%");
                }
                if (cn == '=') {
                    CharNext();
                    return TokenGen(TT::ModTAssign, "%=");
                }
                return TokenGen(TT::ModT, "%");
            }
            case '!': {
                if (cn == '=') {
                    CharNext();
                    return TokenGen(TT::Neq, "!=");
                }
                return TokenGen(TT::Not, "!");
            }
            case '>': {
                if (cn == '=') {
                    CharNext();
                    return TokenGen(TT::Ge, ">=");
                }
                return TokenGen(TT::Gt, ">");
            }
            case '<': {
                if (cn == '=') {
                    CharNext();
                    return TokenGen(TT::Le, "<=");
                }
                return TokenGen(TT::Lt, "<");
            }
            case '&': {
                if (cn == '&') {
                    CharNext();
                    return TokenGen(TT::And, "&&");
                }
                break;
            }
            case '|': {
                if (cn == '|') {
                    CharNext();
                    return TokenGen(TT::Or, "||");
                }
                break;
            }
            case '.': {
                if (cn == '.') {
                    if (cnn == '=') {
                        CharNext(2);
                        return TokenGen(TT::DotDotEq, "..=");
                    }
                    
                    CharNext();
                    return TokenGen(TT::DotDot, "..");
                }
                return TokenGen(TT::Dot, ".");
            }
            case '[': {
                if (cn == '=') {
                    CharNext();
                    return TokenGen(TT::LEBkt, "[=");
                }
                return TokenGen(TT::LBkt, "[");
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
            case ':':   return TokenGen(TT::Colon,     ":");
            case ';':   return TokenGen(TT::Semicolon, ";");
            case '(':   return TokenGen(TT::LParen,    "(");
            case ')':   return TokenGen(TT::RParen,    ")");
            case '{':   return TokenGen(TT::LBrace,    "{");
            case '}':   return TokenGen(TT::RBrace,    "}");
            case ']':   return TokenGen(TT::RBkt,      "]");
            case ',':   return TokenGen(TT::Comma,     ",");
        }

        throw LogErr(LogModule::Lexer, "invalid token", loc_scan_);
    }

    Token Lexer::TokenScanWord() {
        size_t pbeg = pos_;
        while (pos_ + 1 < code_.length() && isIdentifierChar(code_[pos_ + 1])) {
            CharNext();
        }
        CharNext();
        std::string lexeme = std::string(code_.substr(pbeg, pos_ - pbeg));

        return TokenGen(TT::Id, lexeme);
    }

    Token Lexer::TokenScanNumber() {
        size_t pbeg   = pos_;
        bool   hasDot = false;

        while (pos_ + 1 < code_.length()) {
            char cn = code_[pos_ + 1];
            
            if (isNumber(cn)) CharNext();

            // 123.
            else if (isDot(cn) && !hasDot) {

                // 123.4
                if (pos_ + 2 < code_.length() && isNumber(code_[pos_ + 2])) {
                    hasDot = true;
                    CharNext();
                }

                // 123.. or 123.a
                else break;
            }
            else break;
        }
        CharNext();

        return TokenGen(
            TT::Number,
            std::string(code_.substr(pbeg, pos_ - pbeg))
        );
    }

    Token Lexer::TokenScanChar() {
        std::string lexeme = "";
        CharNext();

        while (!isScanEnd()) {
            char c = code_[pos_];

            if (c == '\'') {
                CharNext();
                return TokenGen(TT::Char, lexeme);
            }

            if (c == '\n')
                throw LogErr(LogModule::Lexer, "unclosed single quotes of char", loc_scan_);

            if (c == '\\') {
                CharNext();
                
                if (isScanEnd())
                    throw LogErr(LogModule::Lexer, "unclosed single quotes of char", loc_scan_);

                switch (code_[pos_]) {
                    case 'n':  lexeme += '\n'; break;
                    case 't':  lexeme += '\t'; break;
                    case 'r':  lexeme += '\r'; break;
                    case '\'': lexeme += '\'';  break;
                    case '\\': lexeme += '\\'; break;
                    default:
                        throw LogErr(LogModule::Lexer, std::format(
                            "unknown escape '{}'", code_[pos_]
                        ), loc_scan_);
                }
                CharNext();
                continue;
            }

            lexeme += c;
            CharNext();
        }     
        
        throw LogErr(LogModule::Lexer, "unclosed single quotes of char", loc_scan_);
    }

    Token Lexer::TokenScanString() {
        std::string lexeme = "";
        CharNext();

        while (!isScanEnd()) {
            char c = code_[pos_];

            if (c == '"') {
                CharNext();
                return TokenGen(TT::String, lexeme);
            }

            if (c == '\n')
                throw LogErr(LogModule::Lexer, "unclosed double quotes of string", loc_scan_);

            if (c == '\\') {
                CharNext();
                
                if (isScanEnd())
                    throw LogErr(LogModule::Lexer, "unclosed double quotes of string", loc_scan_);

                switch (code_[pos_]) {
                    case 'n':  lexeme += '\n'; break;
                    case 't':  lexeme += '\t'; break;
                    case 'r':  lexeme += '\r'; break;
                    case '"':  lexeme += '"';  break;
                    case '\\': lexeme += '\\'; break;
                    default:
                        throw LogErr(LogModule::Lexer, std::format(
                            "unknown escape '{}'", code_[pos_]
                        ), loc_scan_);
                }
                CharNext();
                continue;
            }

            lexeme += c;
            CharNext();
        }

        throw LogErr(LogModule::Lexer, "unclosed double quotes of string", loc_scan_);
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

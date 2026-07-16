
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <iostream>
#include <string>

#include "log.hpp"

class Token {
public:
    enum class Type {
        Undefined,

        // Unsemantic
        // └─ Punctuation
        Unsemantic,         //  Base
        Colon,              //  :
        Assign,             //  =
        Semicolon,          //  ;
        LParen,             //  (
        RParen,             //  )
        LBrace,             //  {
        RBrace,             //  }
        Dot,                //  .
        DotDot,             //  ..
        DotDotEq,           //  ..=
        Comma,              //  ,
        Quote,              //  "

        // Semantic
        // └─ Literal
        Semantic,           //  Base
        Id,                 //  Identity
        Number,             //  Continuous Integer
        String,             //  String

        // └─ Arith Operator
        ArithOper,          //  Base
        PlusOrMinus,        //  Base
        Plus,               //  +
        Minus,              //  -
        StarOrSlash,        //  Base
        Star,               //  *
        Slash,              //  /
        
        // └─ Relation Operator
        RelationOper,       //  Base
        Gt,                 //  >
        Lt,                 //  <
        Ge,                 //  >=
        Le,                 //  <=
        Eq,                 //  ==
        Neq,                //  !=

        // └─ Logical Operator
        LogicalOper,        //  Base
        And,                //  &&
        Or,                 //  ||
        Not,                //  !

        // └─ Keyword
        Keyword,            // Base
        If,                 // If
        Elif,               // Else if
        Else,               // Else
    };

private:
    Type        type_   = Type::Undefined;
    std::string lexeme_ = "";

    size_t line_ = 0;
    size_t col_  = 0;
    
public:
    Token() {}
    Token(Type type, const std::string& lexeme, size_t line, size_t col) {
        type_   = type;
        lexeme_ = lexeme;
        line_   = line;
        col_    = col;
    }

    const Type&        type()   const { return type_; }
    const std::string& lexeme() const { return lexeme_; }
    size_t line() const { return line_; }
    size_t col() const { return col_; }

    static Type BaseOfType(Type type) {
        switch (type) {
            case Type::Colon:
            case Type::Assign:
            case Type::Semicolon:
            case Type::LParen:
            case Type::RParen:
            case Type::LBrace:
            case Type::RBrace:
            case Type::Dot:
            case Type::DotDot:
            case Type::DotDotEq:
            case Type::Comma:
            case Type::Quote:
                return Type::Unsemantic;

            case Type::Id:
            case Type::Number:
            case Type::String:
                return Type::Semantic;

            case Type::Plus:
            case Type::Minus:
                return Type::PlusOrMinus;

            case Type::Star:
            case Type::Slash:
                return Type::StarOrSlash;

            case Type::PlusOrMinus:
            case Type::StarOrSlash:
                return Type::ArithOper;

            case Type::Gt:
            case Type::Ge:
            case Type::Lt:
            case Type::Le:
            case Type::Eq:
            case Type::Neq:
                return Type::RelationOper;

            case Type::Not:
            case Type::And:
            case Type::Or:
                return Type::LogicalOper;

            case Type::If:
            case Type::Elif:
            case Type::Else:
                return Type::Keyword;

            default:
                return Type::Undefined;
        }
    }

    static bool isTypeCompatible(Type expected, Type actual) {
        if (expected == Type::Undefined ||
            actual   == Type::Undefined) return false;
        if (expected == actual) return true;
        return isTypeCompatible(expected, BaseOfType(actual));
    }

    static std::string TypeName(Type type) {
        switch (type) {
            case Type::Colon:       return "Colon";
            case Type::Assign:      return "Assign";
            case Type::Semicolon:   return "Semicolon";
            case Type::LParen:      return "LParen";
            case Type::RParen:      return "RParen";
            case Type::LBrace:      return "LBrace";
            case Type::RBrace:      return "RBrace";
            case Type::Dot:         return "Dot";
            case Type::DotDot:      return "DotDot";
            case Type::DotDotEq:    return "DotDotEq";
            case Type::Comma:       return "Comma";
            case Type::Quote:       return "Quote";

            case Type::Id:          return "Id";
            case Type::Number:      return "Number";
            case Type::String:      return "String";

            case Type::PlusOrMinus: return "PlusOrMinus";
            case Type::Plus:        return "Plus";
            case Type::Minus:       return "Minus";
            case Type::StarOrSlash: return "StarOrSlash";
            case Type::Star:        return "Star";
            case Type::Slash:       return "Slash";

            case Type::Gt:          return "Gt";
            case Type::Ge:          return "Ge";
            case Type::Lt:          return "Lt";
            case Type::Le:          return "Le";
            case Type::Eq:          return "Eq";
            case Type::Neq:         return "Neq";

            case Type::Not:         return "Not";
            case Type::And:         return "And";
            case Type::Or:          return "Or";

            case Type::If:          return "If";
            case Type::Elif:        return "Elif";
            case Type::Else:        return "Else";

            default: 
                LogWarn(LogModule::Lexer, "undefined print name for TokenType").Print();
                return "Undefined";
        }
    }
    static void        TypePrint(Type type) {
        std::cout << TypeName(type);
    }
    
    std::string MetaPrint() const {
        return std::format("[{}:{}] {}('{}')", line_, col_,
            TypeName(type_), lexeme_);
    }
};

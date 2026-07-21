
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
        LBkt,               //  [
        RBkt,               //  ]
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
        For,                // For
        In,                 // In
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
        using enum Type;
        switch (type) {
            case Colon:
            case Assign:
            case Semicolon:
            case LParen:
            case RParen:
            case LBrace:
            case RBrace:
            case LBkt:
            case RBkt:
            case Dot:
            case DotDot:
            case DotDotEq:
            case Comma:
            case Quote:
                return Unsemantic;

            case Id:
            case Number:
            case String:
                return Semantic;

            case Plus:
            case Minus:
                return PlusOrMinus;

            case Star:
            case Slash:
                return StarOrSlash;

            case PlusOrMinus:
            case StarOrSlash:
                return ArithOper;

            case Gt:
            case Ge:
            case Lt:
            case Le:
            case Eq:
            case Neq:
                return RelationOper;

            case Not:
            case And:
            case Or:
                return LogicalOper;

            case If:
            case Elif:
            case Else:
            case For:
            case In:
                return Keyword;

            default:
                return Undefined;
        }
    }

    static bool isTypeCompatible(Type expected, Type actual) {
        if (expected == Type::Undefined ||
            actual   == Type::Undefined) return false;
        if (expected == actual) return true;
        return isTypeCompatible(expected, BaseOfType(actual));
    }

    static std::string TypeName(Type type) {
        using enum Type;
        switch (type) {
            case Colon:       return "Colon";
            case Assign:      return "Assign";
            case Semicolon:   return "Semicolon";
            case LParen:      return "LParen";
            case RParen:      return "RParen";
            case LBrace:      return "LBrace";
            case RBrace:      return "RBrace";
            case LBkt:        return "LBkt";
            case RBkt:        return "RBkt";
            case Dot:         return "Dot";
            case DotDot:      return "DotDot";
            case DotDotEq:    return "DotDotEq";
            case Comma:       return "Comma";
            case Quote:       return "Quote";

            case Id:          return "Id";
            case Number:      return "Number";
            case String:      return "String";

            case PlusOrMinus: return "PlusOrMinus";
            case Plus:        return "Plus";
            case Minus:       return "Minus";
            case StarOrSlash: return "StarOrSlash";
            case Star:        return "Star";
            case Slash:       return "Slash";

            case Gt:          return "Gt";
            case Ge:          return "Ge";
            case Lt:          return "Lt";
            case Le:          return "Le";
            case Eq:          return "Eq";
            case Neq:         return "Neq";

            case Not:         return "Not";
            case And:         return "And";
            case Or:          return "Or";

            case If:          return "If";
            case Elif:        return "Elif";
            case Else:        return "Else";
            case For:         return "For";
            case In:          return "In";

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

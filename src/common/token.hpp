
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <iostream>
#include <string>

#include "log.hpp"

struct Token {
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
        Dot,                //  .
        Comma,              //  ,
        Quota,              //  "

        // Semantic
        // └─ Literal
        Semantic,           //  Base
        Id,                 //  Identity
        Number,             //  Continuous Integer
        String,             //  String

        // └─ Operator
        Operator,           //  Base
        PlusOrMinus,        //  Base
        Plus,               //  +
        Minus,              //  -
        StarOrSlash,        //  Base
        Star,               //  *
        Slash,              //  /
    };

    Type        type    = Type::Undefined;
    std::string lexeme  = "";

    size_t line = 0;
    size_t col  = 0;
    
    static Type BaseOfType(Type type) {
        switch (type) {
            case Type::Colon:
            case Type::Assign:
            case Type::Semicolon:
            case Type::LParen:
            case Type::RParen:
            case Type::Dot:
            case Type::Comma:
            case Type::Quota:
                return Type::Unsemantic;

            case Type::Id:
            case Type::Number:
            case Type::String:
                return Type::Semantic;

            case Type::PlusOrMinus:
            case Type::StarOrSlash:
                return Type::Operator;

            case Type::Plus:
            case Type::Minus:
                return Type::PlusOrMinus;

            case Type::Star:
            case Type::Slash:
                return Type::StarOrSlash;

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
            case Type::Dot:         return "Dot";
            case Type::Comma:       return "Comma";
            case Type::Quota:       return "Quota";

            case Type::Id:          return "Id";
            case Type::Number:      return "Number";
            case Type::String:      return "String";

            case Type::PlusOrMinus: return "PlusOrMinus";
            case Type::Plus:        return "Plus";
            case Type::Minus:       return "Minus";
            case Type::StarOrSlash: return "StarOrSlash";
            case Type::Star:        return "Star";
            case Type::Slash:       return "Slash";

            default: 
                LogWarn(LogModule::Lexer, "undefined print name for TokenType").Print();
                return "Undefined";
        }
    }
    static void TypePrint(Type type) {
        std::cout << TypeName(type);
    }
    
    std::string MetaPrint() const {
        return std::format("[{}:{}] {}('{}')", line, col,
            TypeName(type), lexeme);
    }
};


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
        QuoteSingle,        //  '
        Arrow,              //  ->

        // Semantic
        // └─ Literal
        Semantic,           //  Base
        Id,                 //  Identity
        Number,             //  Continuous Integer
        Char,               //  Char
        String,             //  String

        // └─ Arith Operator
        ArithOper,          //  Base
        Plus,               //  +
        Minus,              //  -
        Star,               //  *
        Slash,              //  /
        ModT,               //  %
        ModF,               //  %%
        
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

        // └─ Assign Operator
        AssignOper ,        //  Base
        Assign,             //  =
        PlusAssign,         //  +=
        MinusAssign,        //  -=
        StarAssign,         //  *=
        SlashAssign,        //  /=
        ModTAssign,         //  %=
        ModFAssign,         //  %%=

        // └─ Keyword
        Keyword,            // Base
        True,               // true
        False,              // false
        If,                 // if
        Elif,               // else if
        Else,               // else
        In,                 // in
        For,                // for
        While,              // while
        Break,              // break
        Continue,           // continue
        Return,             // return
        Fn,                 // fn
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
            case QuoteSingle:
            case Arrow:
                return Unsemantic;

            case Id:
            case Number:
            case Char:
            case String:
                return Semantic;

            case Plus:
            case Minus:
            case Star:
            case Slash:
            case ModT:
            case ModF:
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

            case Assign:
            case PlusAssign:
            case MinusAssign:
            case StarAssign:
            case SlashAssign:
            case ModTAssign:
            case ModFAssign:
                return AssignOper;

            case True:
            case False:
            case If:
            case Elif:
            case Else:
            case In:
            case For:
            case While:
            case Break:
            case Continue:
            case Return:
            case Fn:
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
            case Colon:         return "Colon";
            case Semicolon:     return "Semicolon";
            case LParen:        return "LParen";
            case RParen:        return "RParen";
            case LBrace:        return "LBrace";
            case RBrace:        return "RBrace";
            case LBkt:          return "LBkt";
            case RBkt:          return "RBkt";
            case Dot:           return "Dot";
            case DotDot:        return "DotDot";
            case DotDotEq:      return "DotDotEq";
            case Comma:         return "Comma";
            case Quote:         return "Quote";
            case QuoteSingle:   return "QuoteSingle";
            case Arrow:         return "Arrow";

            case Id:            return "Id";
            case Number:        return "Number";
            case Char:          return "Char";
            case String:        return "String";

            case Plus:          return "Plus";
            case Minus:         return "Minus";
            case Star:          return "Star";
            case Slash:         return "Slash";
            case ModT:          return "ModT";
            case ModF:          return "ModF";

            case Gt:            return "Gt";
            case Ge:            return "Ge";
            case Lt:            return "Lt";
            case Le:            return "Le";
            case Eq:            return "Eq";
            case Neq:           return "Neq";

            case Not:           return "Not";
            case And:           return "And";
            case Or:            return "Or";

            case Assign:        return "Assign";
            case PlusAssign:    return "PlusAssign";
            case MinusAssign:   return "MinusAssign";
            case StarAssign:    return "StarAssign";
            case SlashAssign:   return "SlashAssign";
            case ModTAssign:    return "ModTAssign";
            case ModFAssign:    return "ModFAssign";

            case True:          return "true";
            case False:         return "false";
            case If:            return "if";
            case Elif:          return "elif";
            case Else:          return "else";
            case In:            return "in";
            case For:           return "for";
            case While:         return "while";
            case Break:         return "break";
            case Continue:      return "continue";
            case Return:        return "return";
            case Fn:            return "fn";

            default: 
                LogWarn(LogModule::Lexer, "undefined print name for TokenType").Print();
                return "Undefined";
        }
    }
    static void        TypePrint(Type type) {
        std::cout << TypeName(type);
    }
    
    std::string MetaPrint() const {
        return std::format("[{}:{}] {}({})", line_, col_, TypeName(type_), lexeme_);
    }
};

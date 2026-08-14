
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <iostream>
#include <string>
#include <optional>

#include "utils.hpp"

#define COLOR_DEFAULT   "\033[0m"
#define COLOR_GRAY      "\033[90m"          // Module
#define COLOR_RED       "\033[91m"          // Err, False
#define COLOR_GREEN     "\033[92m"          // Finish, True, Char, String
#define COLOR_YELLOW    "\033[93m"          // Warn
#define COLOR_BLUE      "\033[94m"          // Start, Id
#define COLOR_MAGENTA   "\033[95m"          // Ast:Type
#define COLOR_CYAN      "\033[96m"          // Loc
#define COLOR_WHITE     "\033[97m"
#define COLOR_ORANGE    "\033[38;5;214m"    // Token, Ast:Node, Number

enum class LogModule {
    Undefined,
    File,
    Lexer,
    Parser,
    Sema,
    Runtime
};

class Log {
private:
    LogModule           module_;
    std::string         msg_;
    std::string         color_;
    std::optional<Loc>  loc_;

public:
    Log(
        LogModule           module,
        std::string_view    msg,
        std::string_view    color = COLOR_DEFAULT,
        std::optional<Loc>  loc   = std::nullopt
    )
    :   module_(module), 
        msg_(msg),
        color_(color),
        loc_(loc)
    {}

    std::string ModulePrint() const {
        using enum LogModule;\
        switch (module_) {
            case File:      return "file";
            case Lexer:     return "lexer";
            case Parser:    return "parser";
            case Sema:      return "sema";
            case Runtime:   return "rt";
            default:        return "Undefined";
        }
    }
    void Print() const {
        std::cerr
        <<  COLOR_GRAY <<  "[" << ModulePrint() << "]";

        if (loc_.has_value()) {
            std::cerr << COLOR_CYAN << std::format("[{}:{}]", loc_->line, loc_->col);
        }

        std::cerr
        <<  color_ << msg_
        <<  COLOR_DEFAULT <<
        std::endl;
    }
};

class LogInfo : public Log {
public:
    LogInfo(LogModule module, std::string_view msg)
    :   Log(module, std::format("[info] {}", msg)) {}
};

class LogWarn : public Log {
public:
    LogWarn(LogModule module, std::string_view msg, std::optional<Loc> loc = std::nullopt)
    :   Log(module, std::format("[warn] {}", msg), COLOR_YELLOW, loc) {}
};

class LogErr : public Log {
public:
    LogErr(LogModule module, std::string_view msg, std::optional<Loc> loc = std::nullopt)
    :   Log(module, std::format("[err] {}", msg), COLOR_RED, loc) {}
};

class LogStart : public Log {
public:
    LogStart(LogModule module, std::string_view msg)
    :   Log(module, std::format("[start] {}", msg), COLOR_BLUE) {}
};

class LogFinish : public Log {
public:
    LogFinish(LogModule module, std::string_view msg)
    :   Log(module, std::format("[finish] {}", msg), COLOR_GREEN) {}
};


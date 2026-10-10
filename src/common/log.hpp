
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <iostream>
#include <string>
#include <optional>

#include "common/utils/loc.hpp"

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

enum class LogStage {
    Undefined,
    File,
    Config,
    Lexer,
    Parser,
    Sema,
    Xcompiler
};

class Log {
private:
    LogStage            stage_;
    std::string         msg_;
    std::string         color_;
    std::optional<Loc>  loc_;

public:
    Log(
        LogStage            stage,
        std::string_view    msg,
        std::string_view    color = COLOR_DEFAULT,
        std::optional<Loc>  loc   = std::nullopt
    )
    :   stage_(stage), 
        msg_(msg),
        color_(color),
        loc_(loc)
    {}

    virtual ~Log() = default;

    std::string StageName() const {
        using enum LogStage;
        switch (stage_) {
            case File:      return "File";
            case Config:    return "Config";
            case Lexer:     return "Lexer";
            case Parser:    return "Parser";
            case Sema:      return "Sema";
            case Xcompiler: return "Xcompiler";
            default:        return "Undefined";
        }
    }
    std::string StageNameFixed() const {
        using enum LogStage;
        switch (stage_) {
            case File:      return "File     ";
            case Config:    return "Config   ";
            case Lexer:     return "Lexer    ";
            case Parser:    return "Parser   ";
            case Sema:      return "Sema     ";
            case Xcompiler: return "Xcompiler";
            default:        return "Undefined";
        }
    }

    void Print() const {

        // Stage
        std::cerr << COLOR_GRAY
        << StageNameFixed() << " | ";

        // Module and Loc
        if (loc_.has_value()) {
            auto loc = loc_.value();
            
            if (!loc.module().empty()) {
                std::cerr << COLOR_CYAN << loc.module();
            }
            
            std::cerr << COLOR_CYAN
            << std::format("[{}:{}]\t", loc.line(), loc.col());
        }

        // Message
        std::cerr
        <<  color_ << msg_
        <<  COLOR_DEFAULT;
        
        std::cerr << std::endl;
    }
};

class LogInfo : public Log {
public:
    LogInfo(LogStage stage, std::string_view msg)
    :   Log(stage, std::format("info: {}", msg)) {}
};

class LogWarn : public Log {
public:
    LogWarn(LogStage stage, std::string_view msg, std::optional<Loc> loc = std::nullopt)
    :   Log(stage, std::format("warn: {}", msg), COLOR_YELLOW, loc) {}
};

class LogErr : public Log {
public:
    LogErr(LogStage stage, std::string_view msg, std::optional<Loc> loc = std::nullopt)
    :   Log(stage, std::format("error: {}", msg), COLOR_RED, loc) {}
};

class LogBuild : public Log {
public:
    LogBuild(LogStage stage, const std::string& module)
    :   Log(stage, module, COLOR_GREEN) {}
};

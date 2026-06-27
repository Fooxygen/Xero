
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <iostream>
#include <string>

#define COLOR_DEFAULT   "\033[0m"
#define COLOR_GRAY      "\033[90m"
#define COLOR_RED       "\033[91m"
#define COLOR_GREEN     "\033[92m"
#define COLOR_YELLOW    "\033[93m"
#define COLOR_BLUE      "\033[94m"
#define COLOR_MAGENTA   "\033[95m"
#define COLOR_CYAN      "\033[96m"
#define COLOR_WHITE     "\033[97m"

enum class LogModule {
    Undefined,
    File,
    Lexer,
    Parser
};

class Log {
private:
    LogModule   module_;
    std::string msg_;
    std::string color_;

public:
    Log(
        LogModule        module,
        std::string_view msg,
        std::string_view color = COLOR_DEFAULT)
    :   module_(module), 
        msg_(msg),
        color_(color) {}

    std::string ModulePrint() const {
        switch (module_) {
            case LogModule::File:   return "file";
            case LogModule::Lexer:  return "lexer";
            case LogModule::Parser: return "parser";
            default: return "Undefined";
        }
    }
    void Print() const {
        std::cout << COLOR_GRAY << "[" << ModulePrint() << "]" << COLOR_DEFAULT;
        std::cout << color_ << msg_ << COLOR_DEFAULT << std::endl;
    }
};

class LogInfo : public Log {
public:
    LogInfo(LogModule module, std::string_view msg)
    :   Log(module, std::format("[info] {}", msg)) {}
};

class LogWarn : public Log {
public:
    LogWarn(LogModule module, std::string_view msg)
    :   Log(module, std::format("[warn] {}", msg), COLOR_YELLOW) {}
};

class LogErr : public Log {
public:
    LogErr(LogModule module, std::string_view msg)
    :   Log(module, std::format("[err] {}", msg), COLOR_RED) {}
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


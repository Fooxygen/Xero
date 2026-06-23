
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <iostream>
#include <string>

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
        std::string_view color = "\033[0m")
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
        std::cout << "\033[90m" << "[" << ModulePrint() << "]" << std::string("\033[0m");
        std::cout << color_ << msg_ << std::string("\033[0m") << std::endl;
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
    :   Log(module, std::format("[warn] {}", msg), "\033[93m") {}
};

class LogErr : public Log {
public:
    LogErr(LogModule module, std::string_view msg)
    :   Log(module, std::format("[err] {}", msg), "\033[91m") {}
};

class LogStart : public Log {
public:
    LogStart(LogModule module, std::string_view msg)
    :   Log(module, std::format("[start] {}", msg), "\033[94m") {}
};

class LogFinish : public Log {
public:
    LogFinish(LogModule module, std::string_view msg)
    :   Log(module, std::format("[finish] {}", msg), "\033[92m") {}
};



//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "token.hpp"

// Type of Abstract Syntax Tree's Node
enum class AstType {
    Undefined,

    // Entry
    Program,            //  Entrance

    // Const
    Const,              //  Base ------
    NumConst,           //  Number Type
    BoolConst,          //  Boolean Type
    StringConst,        //  String Type

    // Expr
    Expr,               //  Base ------
    IdExpr,             //  Identity
    OperExpr,           //  Operation
    RangeExpr,          //  Range
    NegExpr,            //  Negative
    NotExpr,            //  Not
    FnCallExpr,         //  Func Call
    MethodCallExpr,     //  Method Call

    // Stmt
    Stmt,               //  Base ------
    BlockStmt,          //  Grouped Stmts
    DeclStmt,           //  Declaration
    AssignStmt,         //  Assignment
    CondStmt,           //  Condition
    ForStmt,            //  For Loop

    // Common
    ArgList,            // List of Args
};

static AstType BaseOfAstType(AstType type) {
    switch (type) {
        case AstType::NumConst:
        case AstType::BoolConst:
        case AstType::StringConst:
            return AstType::Const;
        case AstType::Const:
            return AstType::Expr;

        case AstType::IdExpr:
        case AstType::OperExpr:
        case AstType::RangeExpr:
        case AstType::NegExpr:
        case AstType::NotExpr:
        case AstType::FnCallExpr:
        case AstType::MethodCallExpr:
            return AstType::Expr;

        case AstType::BlockStmt:
        case AstType::DeclStmt:
        case AstType::AssignStmt:
        case AstType::CondStmt:
        case AstType::ForStmt:
            return AstType::Stmt;

        default:
            return AstType::Undefined;
    }
}

static bool isAstTypeCompatible(AstType expected, AstType actual) {
    if (expected == AstType::Undefined ||
        actual   == AstType::Undefined) return false;
    if (expected == actual) return true;
    return isAstTypeCompatible(expected, BaseOfAstType(actual));
}

// Node of Abstract Syntax Tree
class AstNode {
public:
    AstType type_ = AstType::Undefined;

    const std::string TypeName() const {
        switch (type_) {
            case AstType::Program:          return "Program";

            case AstType::ArgList:          return "ArgList";

            case AstType::Const:            return "Const";
            case AstType::NumConst:         return "NumConst";
            case AstType::BoolConst:        return "BoolConst";
            case AstType::StringConst:      return "StringConst";

            case AstType::Expr:             return "Expr";
            case AstType::IdExpr:           return "IdExpr";
            case AstType::OperExpr:         return "OperExpr";
            case AstType::RangeExpr:        return "RangeExpr";
            case AstType::NegExpr:          return "NegExpr";
            case AstType::NotExpr:          return "NotExpr";
            case AstType::FnCallExpr:       return "FnCallExpr";
            case AstType::MethodCallExpr:   return "MethodCallExpr";

            case AstType::Stmt:             return "Stmt";
            case AstType::BlockStmt:        return "BlockStmt";
            case AstType::DeclStmt:         return "DeclStmt";
            case AstType::AssignStmt:       return "AssignStmt";
            case AstType::CondStmt:         return "CondStmt";
            case AstType::ForStmt:          return "ForStmt";

            default: 
                LogWarn(LogModule::Parser, "undefined print name for AstNode");
                return "Undefined";
        }
    }
    const void        TypePrint() const {
        std::cout << TypeName();
    }

    virtual void PrintImpl(std::string prefix) {}
    void         Print(std::string prefix = "", std::string alias = "", bool isBegin = false) {
        std::cout << prefix;
        if (!isBegin) std::cout << "└── ";

        size_t indent_alias = 0;
        if (alias != "") {
            indent_alias = alias.length() + 3;
            std::cout << COLOR_ORANGE << "[" << alias << "] " << COLOR_DEFAULT;
        }
        std::cout << TypeName() << std::endl;

        PrintImpl(prefix + "    " + std::string(indent_alias, ' '));
    }
    void         PrintLabel(const std::string& name, std::string prefix = "") {
        std::cout << prefix;
        std::cout << "└── " << COLOR_ORANGE << "[" << name << "] " << COLOR_DEFAULT;
    }
};
class Expr              : public AstNode {
};
class Const             : public Expr {
};
class Stmt              : public AstNode {
};

// Common
class ArgList           : public AstNode {
private:
    std::vector<std::unique_ptr<Expr>> args_;

public:
    ArgList(std::vector<std::unique_ptr<Expr>>& args) : args_(std::move(args)) {
        type_ = AstType::ArgList;
    }
    
    std::vector<std::unique_ptr<Expr>>& args() {
        return args_;
    }

    void PrintImpl(std::string prefix) override {
        for (auto& arg : args_) {
            arg->Print(prefix);
        }
    }
};

// Expr
class IdExpr            : public Expr {
public:
    std::string value_ = "";

    IdExpr(const std::string& value) : value_(value) {
        type_ = AstType::IdExpr;
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("value", prefix);
        std::cout << COLOR_BLUE << value_ << COLOR_DEFAULT << std::endl;
    }
};
class OperExpr          : public Expr {
public:
    Token::Type opertype_ = Token::Type::Undefined;
    std::unique_ptr<Expr> lexpr_ = nullptr;
    std::unique_ptr<Expr> rexpr_ = nullptr;

    OperExpr(
        Token::Type opertype,
        std::unique_ptr<Expr> lexpr,
        std::unique_ptr<Expr> rexpr
    )
    :   opertype_(opertype),
        lexpr_(std::move(lexpr)),  
        rexpr_(std::move(rexpr))
    {
        type_ = AstType::OperExpr;
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("type", prefix);
        std::cout << COLOR_MAGENTA;
        Token::TypePrint(opertype_);
        std::cout << COLOR_DEFAULT << std::endl;

        lexpr_->Print(prefix, "lexpr");
        rexpr_->Print(prefix, "rexpr");
    }
};
class RangeExpr         : public Expr {
public:
    Token::Type rangetype_ = Token::Type::Undefined;
    std::unique_ptr<Expr> lexpr_ = nullptr;
    std::unique_ptr<Expr> rexpr_ = nullptr;
    std::unique_ptr<Expr> step_  = nullptr;

    RangeExpr(
        Token::Type rangetype,
        std::unique_ptr<Expr> lexpr,
        std::unique_ptr<Expr> rexpr,
        std::unique_ptr<Expr> step
    )
    :   rangetype_(rangetype),
        lexpr_(std::move(lexpr)),  
        rexpr_(std::move(rexpr)),
        step_(std::move(step))
    {
        type_ = AstType::RangeExpr;
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("type", prefix);
        std::cout << COLOR_MAGENTA;
        Token::TypePrint(rangetype_);
        std::cout << COLOR_DEFAULT << std::endl;

        lexpr_->Print(prefix, "lexpr");
        rexpr_->Print(prefix, "rexpr");
        if (step_) step_->Print(prefix, "step");
    }
};
class NegExpr           : public Expr {
public:
    std::unique_ptr<Expr> expr_ = nullptr;

    NegExpr(std::unique_ptr<Expr> expr)
    :   expr_(std::move(expr))
    {
        type_ = AstType::NegExpr;
    }

    void PrintImpl(std::string prefix) override {
        expr_->Print(prefix, "expr");
    }
};
class NotExpr           : public Expr {
public:
    std::unique_ptr<Expr> expr_ = nullptr;

    NotExpr(std::unique_ptr<Expr> expr)
    :   expr_(std::move(expr))
    {
        type_ = AstType::NotExpr;
    }

    void PrintImpl(std::string prefix) override {
        expr_->Print(prefix, "expr");
    }
};
class FnCallExpr        : public Expr {
private:
    std::unique_ptr<IdExpr>  callee_  = nullptr;
    std::unique_ptr<ArgList> arglist_ = nullptr;

public:
    FnCallExpr(
        std::unique_ptr<IdExpr>  callee,
        std::unique_ptr<ArgList> arglist
    )
    :   callee_(std::move(callee)),
        arglist_(std::move(arglist))
    {
        type_ = AstType::FnCallExpr;
    }

    IdExpr*  callee()  const { return callee_.get(); }
    ArgList* arglist() const { return arglist_.get(); }

    void PrintImpl(std::string prefix) override {
        callee_->Print(prefix, "callee");
        arglist_->Print(prefix, "arglist");
    }
};
class MethodCallExpr    : public Expr {
private:
    std::unique_ptr<Expr>    target_  = nullptr;
    std::unique_ptr<IdExpr>  callee_  = nullptr;
    std::unique_ptr<ArgList> arglist_ = nullptr;

public:
    MethodCallExpr(
        std::unique_ptr<Expr>    target,
        std::unique_ptr<IdExpr>  callee,
        std::unique_ptr<ArgList> arglist
    )
    :   target_(std::move(target)),
        callee_(std::move(callee)),
        arglist_(std::move(arglist))
    {
        type_ = AstType::MethodCallExpr;
    }

    Expr*    target()  const { return target_.get(); }
    IdExpr*  callee()  const { return callee_.get(); }
    ArgList* arglist() const { return arglist_.get(); }

    void PrintImpl(std::string prefix) override {
        target_->Print(prefix, "target");
        callee_->Print(prefix, "callee");
        arglist_->Print(prefix, "arglist");
    }
};

// Const
class NumConst          : public Const {
public:
    std::string value_ = "";

    NumConst(const std::string& value)
    :   value_(value)
    {
        type_ = AstType::NumConst;
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("value", prefix);
        std::cout << COLOR_ORANGE << value_ << COLOR_DEFAULT << std::endl;
    }
};
class BoolConst         : public Const {
public:
    bool value_ = false;

    BoolConst(bool value)
    :   value_(value)
    {
        type_ = AstType::BoolConst;
    }
    
    void PrintImpl(std::string prefix) override {
        PrintLabel("value", prefix);
        if (value_)
            std::cout << COLOR_GREEN << "true" << COLOR_DEFAULT << std::endl;
        else
            std::cout << COLOR_RED << "false" << COLOR_DEFAULT << std::endl;
    }
};
class StringConst       : public Const {
public:
    std::string value_ = "";

    StringConst(const std::string& value)
    :   value_(value)
    {
        type_ = AstType::StringConst;
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("value", prefix);
        std::cout << COLOR_GREEN << "'" << value_ << "'" << COLOR_DEFAULT << std::endl;
    }
};

// Stmt
class BlockStmt         : public Stmt {
private:
    std::vector<std::unique_ptr<AstNode>> children_;

public:
    BlockStmt(std::vector<std::unique_ptr<AstNode>>& children)
    :   children_(std::move(children))
    {
        type_ = AstType::BlockStmt;
    }

    std::vector<std::unique_ptr<AstNode>>& children() {
        return children_;
    }

    void PrintImpl(std::string prefix) override {
        for (auto& child : children_) {
            child->Print(prefix);
        }
    }
};
class DeclStmt          : public Stmt {
public:
    std::unique_ptr<IdExpr> id_ = nullptr;
    std::unique_ptr<Expr>   value_ = nullptr;
    std::unique_ptr<IdExpr> value_type_ = nullptr;

    DeclStmt(
        std::unique_ptr<IdExpr> id,
        std::unique_ptr<Expr>   value,
        std::unique_ptr<IdExpr> value_type
    )
    :   id_(std::move(id)),
        value_(std::move(value)),
        value_type_(std::move(value_type))
    {
        type_ = AstType::DeclStmt;
    }

    void PrintImpl(std::string prefix) override {
        id_->Print(prefix, "id");
        value_->Print(prefix, "value");
        value_type_->Print(prefix, "type");
    }
};
class AssignStmt        : public Stmt {
public:
    std::unique_ptr<IdExpr> id_    = nullptr;
    std::unique_ptr<Expr>   value_ = nullptr;

    AssignStmt(
        std::unique_ptr<IdExpr> id,
        std::unique_ptr<Expr>   value
    )
    :   id_(std::move(id)),
        value_(std::move(value))
    {
        type_ = AstType::AssignStmt;
    }

    void PrintImpl(std::string prefix) override {
        id_->Print(prefix, "id");
        value_->Print(prefix, "value");
    }
};
class CondStmt          : public Stmt {
public:
    std::unique_ptr<Expr>      cond_  = nullptr;
    std::unique_ptr<BlockStmt> block_ = nullptr;
    std::unique_ptr<CondStmt>  sub_   = nullptr;

    CondStmt(
        std::unique_ptr<Expr>      cond,
        std::unique_ptr<BlockStmt> block,
        std::unique_ptr<CondStmt>  sub
    )
    :   cond_(std::move(cond)),
        block_(std::move(block)),
        sub_(std::move(sub))
    {
        type_ = AstType::CondStmt;
    }

    void PrintImpl(std::string prefix) override {
        if (cond_) cond_->Print(prefix, "cond");
        block_->Print(prefix, "block");
        if (sub_) sub_->Print(prefix, "sub");
    }
};
class ForStmt           : public Stmt {
public:
    std::unique_ptr<IdExpr>     iter_;
    std::unique_ptr<Expr>       data_;
    std::unique_ptr<BlockStmt>  block_;

    ForStmt(
        std::unique_ptr<IdExpr>     iter,
        std::unique_ptr<Expr>       data,
        std::unique_ptr<BlockStmt>  block
    )
    :   iter_(std::move(iter)),
        data_(std::move(data)),
        block_(std::move(block))
    {
        type_ = AstType::ForStmt;
    }

    void PrintImpl(std::string prefix) override {
        iter_->Print(prefix, "iter");
        data_->Print(prefix, "data");
        block_->Print(prefix, "block");
    }
};

// Program
class Program           : public BlockStmt {
public:
    Program(std::vector<std::unique_ptr<AstNode>>& children)
    :   BlockStmt(children)
    {
        type_ = AstType::Program;
    }
};


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
        case AstType::NegExpr:
        case AstType::NotExpr:
        case AstType::FnCallExpr:
        case AstType::MethodCallExpr:
            return AstType::Expr;

        case AstType::BlockStmt:
        case AstType::DeclStmt:
        case AstType::AssignStmt:
        case AstType::CondStmt:
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
            case AstType::NegExpr:          return "NegExpr";
            case AstType::NotExpr:          return "NotExpr";
            case AstType::FnCallExpr:       return "FnCallExpr";
            case AstType::MethodCallExpr:   return "MethodCallExpr";

            case AstType::Stmt:             return "Stmt";
            case AstType::BlockStmt:        return "BlockStmt";
            case AstType::DeclStmt:         return "DeclStmt";
            case AstType::AssignStmt:       return "AssignStmt";
            case AstType::CondStmt:         return "CondStmt";

            default: 
                LogWarn(LogModule::Parser, "undefined print name for AstNode");
                return "Undefined";
        }
    }
    const void        TypePrint() const {
        std::cout << TypeName();
    }

    // Structure Print
    void         AstPrint(std::string indent = "", size_t expand = 4) {
        // Expand indent for the next layer
        indent += std::string(expand, ' ');
        TypePrint();
        AstPrintImpl(indent, expand);
    }
    // Structure Print Implement
    virtual void AstPrintImpl(std::string indent = "", size_t expand = 4) {}
    
    // New Line and Insert Indent, Label
    // exp:
    //      [normal indent      Program
    //       4 whitespace]          DeclStmt
    //      [custom indent      [id] IdExpr
    //       label length]           [id] i
    // notice:
    //      indent should be updated before use as sublayer
    void         AstLayerPrint(std::string indent, std::string label) {
        std::cout << '\n' << indent;
        if (!label.empty()) {
            std::cout << COLOR_YELLOW << "[" << label << "] " << COLOR_DEFAULT;
        }
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        AstLayerPrint(indent, "args");
        for (auto& arg : args_) {
            AstLayerPrint(indent, "");
            arg->AstPrint(indent, expand);
        }
    }
    
    std::vector<std::unique_ptr<Expr>>& args() {
        return args_;
    }
};

// Expr
class IdExpr            : public Expr {
public:
    std::string value_ = "";

    IdExpr(const std::string& value) : value_(value) {
        type_ = AstType::IdExpr;
    }

    void AstPrintImpl(std::string indent, size_t expand) override {       
        AstLayerPrint(indent, "id");
        std::cout << COLOR_CYAN << value_ << COLOR_DEFAULT;
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

    void AstPrintImpl(std::string indent, size_t expand) override {      
        AstLayerPrint(indent, "oper");
        Token::TypePrint(opertype_);

        AstLayerPrint(indent, "lexpr");
        lexpr_->AstPrint(indent, 8);

        AstLayerPrint(indent, "rexpr");
        rexpr_->AstPrint(indent, 8);
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        AstLayerPrint(indent, "expr");
        expr_->AstPrint(indent, 7);
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        AstLayerPrint(indent, "expr");
        expr_->AstPrint(indent, 7);
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        AstLayerPrint(indent, "callee");
        callee_->AstPrint(indent, 9);

        AstLayerPrint(indent, "arglist");
        arglist_->AstPrint(indent, 10);
    }

    IdExpr*  callee()  const { return callee_.get(); }
    ArgList* arglist() const { return arglist_.get(); }
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        AstLayerPrint(indent, "target");
        target_->AstPrint(indent, 9);

        AstLayerPrint(indent, "callee");
        callee_->AstPrint(indent, 9);

        AstLayerPrint(indent, "arglist");
        arglist_->AstPrint(indent, 10);
    }

    Expr*    target()  const { return target_.get(); }
    IdExpr*  callee()  const { return callee_.get(); }
    ArgList* arglist() const { return arglist_.get(); }
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        AstLayerPrint(indent, "value");
        std::cout << COLOR_GREEN << value_ << COLOR_DEFAULT;
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        AstLayerPrint(indent, "value");
        if (value_) {
            std::cout << COLOR_GREEN << "true" << COLOR_DEFAULT;
        }
        else {
            std::cout << COLOR_RED << "false" << COLOR_DEFAULT;
        }
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        AstLayerPrint(indent, "value");
        std::cout << COLOR_GREEN << '\"' << value_ << '\"' << COLOR_DEFAULT;
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        for (auto& child : children_) {
            AstLayerPrint(indent, "");
            child->AstPrint(indent, expand);
        }
    }

    std::vector<std::unique_ptr<AstNode>>& children() {
        return children_;
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        AstLayerPrint(indent, "id");
        id_->AstPrint(indent, 5);

        AstLayerPrint(indent, "value");
        value_->AstPrint(indent, 8);

        AstLayerPrint(indent, "value_type");
        value_type_->AstPrint(indent, 13);
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        AstLayerPrint(indent, "id");
        id_->AstPrint(indent, 5);

        AstLayerPrint(indent, "value");
        value_->AstPrint(indent, 8);
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        AstLayerPrint(indent, "cond");
        if (cond_) cond_->AstPrint(indent, 7);

        AstLayerPrint(indent, "block");
        block_->AstPrint(indent, 8);

        AstLayerPrint(indent, "sub");
        if (sub_) sub_->AstPrint(indent, 6);
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

    void AstPrintImpl(std::string indent, size_t expand) override {
        BlockStmt::AstPrintImpl(indent, expand);
        std::cout << std::endl;
    }
};


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "token.hpp"

// Type of Operation
enum class OperType {
    Undefined,

    // Arith
    Plus, Minus, Star, Slash, Neg,

    // Relation
    Gt, Lt, Ge, Le, Eq, Neq,

    // Logical
    And, Or, Not,

    // Container
    Pick,
};

static std::string OperTypeName(OperType type) {
    using enum OperType;
    switch(type) {
        case Plus:  return "Plus";
        case Minus: return "Minus";
        case Star:  return "Star";
        case Slash: return "Slash";
        case Neg:   return "Neg";

        case Gt:    return "Gt" ;
        case Lt:    return "Lt" ;
        case Ge:    return "Ge" ;
        case Le:    return "Le" ;
        case Eq:    return "Eq" ;
        case Neq:   return "Neq";

        case And:   return "And";
        case Or:    return "Or" ;
        case Not:   return "Not";

        case Pick:  return "Pick";

        default:    return "Undefined";
    }
}

// Type of Abstract Syntax Tree's Node
enum class AstType {
    Undefined,

    // Entry
    Program,            //  Entrance

    // Const
    Const,              //  Base ------
    NumConst,           //  Number Type
    BoolConst,          //  Boolean Type
    CharConst,          //  Char Type
    StringConst,        //  String Type

    // Expr
    Expr,               //  Base ------
    IdExpr,             //  Identity
    OperExpr,           //  Operation
    RangeExpr,          //  Range
    FnCallExpr,         //  Func Call
    MethodCallExpr,     //  Method Call
    ArrayExpr,          //  Array

    // Stmt
    Stmt,               //  Base ------
    BlockStmt,          //  Grouped Stmts
    DeclStmt,           //  Declaration
    AssignStmt,         //  Assignment
    CondStmt,           //  Condition
    ForStmt,            //  For Loop

    // Common
    Exprs,             // List of expr
};

static AstType BaseOfAstType(AstType type) {
    using enum AstType;
    switch (type) {
        case NumConst:
        case BoolConst:
        case CharConst:
        case StringConst:
            return Const;
        case Const:
            return Expr;

        case IdExpr:
        case OperExpr:
        case RangeExpr:
        case FnCallExpr:
        case MethodCallExpr:
        case ArrayExpr:
            return Expr;

        case BlockStmt:
        case DeclStmt:
        case AssignStmt:
        case CondStmt:
        case ForStmt:
            return Stmt;

        default:
            return Undefined;
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

    virtual const std::string TypeName() const {
        return "Undefined";
    }
    const void TypePrint() const {
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
class Exprs             : public AstNode {
public:
    std::vector<std::unique_ptr<Expr>> exprs_;

    Exprs(std::vector<std::unique_ptr<Expr>>& exprs) : exprs_(std::move(exprs)) {
        type_ = AstType::Exprs;
    }

    const std::string TypeName() const {
        return "Exprs";
    }

    void PrintImpl(std::string prefix) override {
        for (auto& e : exprs_) e->Print(prefix);
    }
};

// Expr
class IdExpr            : public Expr {
public:
    std::string value_ = "";

    IdExpr(const std::string& value) : value_(value) {
        type_ = AstType::IdExpr;
    }

    const std::string TypeName() const {
        return "IdExpr";
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("value", prefix);
        std::cout << COLOR_BLUE << value_ << COLOR_DEFAULT << std::endl;
    }
};
class OperExpr          : public Expr {
public:
    OperType opertype_ = OperType::Undefined;
    std::unique_ptr<Expr> lexpr_ = nullptr;
    std::unique_ptr<Expr> rexpr_ = nullptr;

    OperExpr(
        OperType opertype,
        std::unique_ptr<Expr> lexpr,
        std::unique_ptr<Expr> rexpr
    )
    :   opertype_(opertype),
        lexpr_(std::move(lexpr)),  
        rexpr_(std::move(rexpr))
    {
        type_ = AstType::OperExpr;
    }

    const std::string TypeName() const {
        return "OperExpr";
    }
    
    void PrintImpl(std::string prefix) override {
        PrintLabel("type", prefix);
        std::cout << COLOR_MAGENTA;
        std::cout << OperTypeName(opertype_);
        std::cout << COLOR_DEFAULT << std::endl;
        lexpr_->Print(prefix, "lexpr");
        if (rexpr_) rexpr_->Print(prefix, "rexpr");
    }
};
class RangeExpr         : public Expr {
public:
    std::unique_ptr<Expr> lexpr_ = nullptr;
    std::unique_ptr<Expr> rexpr_ = nullptr;
    std::unique_ptr<Expr> step_  = nullptr;
    bool isClosed_ = false;

    RangeExpr(
        std::unique_ptr<Expr> lexpr,
        std::unique_ptr<Expr> rexpr,
        std::unique_ptr<Expr> step,
        bool isClosed
    )
    :   lexpr_(std::move(lexpr)),  
        rexpr_(std::move(rexpr)),
        step_(std::move(step)),
        isClosed_(isClosed)
    {
        type_ = AstType::RangeExpr;
    }

    const std::string TypeName() const {
        return "RangeExpr";
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("type", prefix);
        std::cout << COLOR_MAGENTA;
        if (isClosed_)  Token::TypePrint(Token::Type::DotDotEq);
        else            Token::TypePrint(Token::Type::DotDot);
        std::cout << COLOR_DEFAULT << std::endl;

        lexpr_->Print(prefix, "lexpr");
        rexpr_->Print(prefix, "rexpr");
        if (step_) step_->Print(prefix, "step");
    }
};
class ArrayExpr         : public Expr {
public:
    std::unique_ptr<Exprs> elements_;

    ArrayExpr(std::unique_ptr<Exprs> elements)
    :   elements_(std::move(elements))
    {
        type_ = AstType::ArrayExpr;
    }

    const std::string TypeName() const {
        return "ArrayExpr";
    }

    void PrintImpl(std::string prefix) override {
        elements_->Print(prefix, "elements");
    }
};
class FnCallExpr        : public Expr {
public:
    std::unique_ptr<IdExpr> callee_ = nullptr;
    std::unique_ptr<Exprs>  args_   = nullptr;

    FnCallExpr(
        std::unique_ptr<IdExpr> callee,
        std::unique_ptr<Exprs>  args
    )
    :   callee_(std::move(callee)),
        args_(std::move(args))
    {
        type_ = AstType::FnCallExpr;
    }

    const std::string TypeName() const {
        return "FnCallExpr";
    }

    void PrintImpl(std::string prefix) override {
        callee_->Print(prefix, "callee");
        args_->Print(prefix, "args");
    }
};
class MethodCallExpr    : public Expr {
public:
    std::unique_ptr<Expr>   target_ = nullptr;
    std::unique_ptr<IdExpr> callee_ = nullptr;
    std::unique_ptr<Exprs>  args_   = nullptr;

    MethodCallExpr(
        std::unique_ptr<Expr>   target,
        std::unique_ptr<IdExpr> callee,
        std::unique_ptr<Exprs>  args
    )
    :   target_(std::move(target)),
        callee_(std::move(callee)),
        args_(std::move(args))
    {
        type_ = AstType::MethodCallExpr;
    }

    const std::string TypeName() const {
        return "MethodCallExpr";
    }

    void PrintImpl(std::string prefix) override {
        target_->Print(prefix, "target");
        callee_->Print(prefix, "callee");
        args_->Print(prefix, "args");
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

    const std::string TypeName() const {
        return "NumConst";
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
    
    const std::string TypeName() const {
        return "BoolConst";
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("value", prefix);
        if (value_)
            std::cout << COLOR_GREEN << "true" << COLOR_DEFAULT << std::endl;
        else
            std::cout << COLOR_RED << "false" << COLOR_DEFAULT << std::endl;
    }
};
class CharConst         : public Const {
public:
    std::string value_ = "";

    CharConst(const std::string& value)
    :   value_(value)
    {
        type_ = AstType::CharConst;
    }

    const std::string TypeName() const {
        return "CharConst";
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("value", prefix);
        std::cout << COLOR_GREEN << "'" << value_ << "'" << COLOR_DEFAULT << std::endl;
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

    const std::string TypeName() const {
        return "StringConst";
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("value", prefix);
        std::cout << COLOR_GREEN << "\"" << value_ << "\"" << COLOR_DEFAULT << std::endl;
    }
};

// Stmt
class BlockStmt         : public Stmt {
public:
    std::vector<std::unique_ptr<AstNode>> children_;

    BlockStmt(std::vector<std::unique_ptr<AstNode>>& children)
    :   children_(std::move(children))
    {
        type_ = AstType::BlockStmt;
    }

    const std::string TypeName() const {
        return "BlockStmt";
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

    const std::string TypeName() const {
        return "DeclStmt";
    }

    void PrintImpl(std::string prefix) override {
        id_->Print(prefix, "id");
        value_->Print(prefix, "value");
        value_type_->Print(prefix, "type");
    }
};
class AssignStmt        : public Stmt {
public:
    std::unique_ptr<Expr> target_ = nullptr;
    std::unique_ptr<Expr> value_  = nullptr;

    AssignStmt(
        std::unique_ptr<Expr> target,
        std::unique_ptr<Expr> value
    )
    :   target_(std::move(target)),
        value_(std::move(value))
    {
        type_ = AstType::AssignStmt;
    }

    const std::string TypeName() const {
        return "AssignStmt";
    }

    void PrintImpl(std::string prefix) override {
        target_->Print(prefix, "target");
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

    const std::string TypeName() const {
        return "CondStmt";
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

    const std::string TypeName() const {
        return "ForStmt";
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

    const std::string TypeName() const {
        return "Program";
    }
};

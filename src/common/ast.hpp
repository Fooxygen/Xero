
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <iostream>
#include <memory>
#include <vector>

#include "token.hpp"
#include "opertype.hpp"
#include "signal.hpp"

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
    BlockExpr,          //  Packaged Astnode
    IdExpr,             //  Identity
    DeclExpr,           //  Declaration
    OperExpr,           //  Operation
    RangeExpr,          //  Range
    ArrayExpr,          //  Array
    FnCallExpr,         //  Function Call
    MethodCallExpr,     //  Method Call
    FnExpr,             //  Function

    // Stmt
    Stmt,               //  Base ------
    ExprStmt,           //  Expr used as Stmt
    AssignStmt,         //  Assignment
    CondStmt,           //  Condition
    LoopSignalStmt,     //  Loop Signal
    ForStmt,            //  For Loop
    ReturnSignalStmt,   //  Return Signal

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

        case BlockExpr:
        case IdExpr:
        case DeclExpr:
        case OperExpr:
        case RangeExpr:
        case ArrayExpr:
        case FnCallExpr:
        case MethodCallExpr:
        case FnExpr:
            return Expr;

        case ExprStmt:
        case AssignStmt:
        case CondStmt:
        case LoopSignalStmt:
        case ForStmt:
        case ReturnSignalStmt:
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

    virtual std::unique_ptr<AstNode> Clone() const = 0;
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

    std::unique_ptr<AstNode> Clone() const override {
        std::vector<std::unique_ptr<Expr>> exprs;
        for (auto& e : exprs_) {
            exprs.emplace_back((Expr*)(e->Clone().release()));
        }
        return std::make_unique<Exprs>(exprs);
    }
};

// Expr

class BlockExpr         : public Expr {
public:
    std::vector<std::unique_ptr<AstNode>> children_;

    BlockExpr(std::vector<std::unique_ptr<AstNode>>& children)
    :   children_(std::move(children))
    {
        type_ = AstType::BlockExpr;
    }

    const std::string TypeName() const {
        return "BlockExpr";
    }

    void PrintImpl(std::string prefix) override {
        for (auto& child : children_) {
            child->Print(prefix);
        }
    }

    std::unique_ptr<AstNode> Clone() const override {
        std::vector<std::unique_ptr<AstNode>> children;
        for (auto& child : children_) {
            children.emplace_back(child->Clone());
        }
        return std::make_unique<BlockExpr>(children);
    }
};
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<IdExpr>(value_);
    }
};
class DeclExpr          : public Expr {
public:
    std::string id_      = "";
    std::string bindtype_ = "";
    std::unique_ptr<Expr> value_ = nullptr;

    DeclExpr(
        const std::string&    id,
        const std::string&    bindtype,
        std::unique_ptr<Expr> value
    )
    :   id_(id),
        bindtype_(bindtype),
        value_(std::move(value))
    {
        type_ = AstType::DeclExpr;
    }

    const std::string TypeName() const {
        return "DeclExpr";
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("id", prefix);
        std::cout << COLOR_BLUE << id_ << COLOR_DEFAULT << std::endl;
        PrintLabel("bindtype", prefix);
        std::cout << COLOR_MAGENTA << bindtype_ << COLOR_DEFAULT << std::endl;
        if (value_) value_->Print(prefix, "value");
    }

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<DeclExpr>(
            id_,
            bindtype_,
            value_ ? std::unique_ptr<Expr>((Expr*)(value_->Clone().release())) : nullptr
        );
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<OperExpr>(
            opertype_,
            std::unique_ptr<Expr>((Expr*)(lexpr_->Clone().release())),
            rexpr_ ? std::unique_ptr<Expr>((Expr*)(rexpr_->Clone().release())) : nullptr
        );
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<RangeExpr>(
            std::unique_ptr<Expr>((Expr*)(lexpr_->Clone().release())),
            std::unique_ptr<Expr>((Expr*)(rexpr_->Clone().release())),
            step_ ? std::unique_ptr<Expr>((Expr*)(step_->Clone().release())) : nullptr,
            isClosed_
        );
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<ArrayExpr>(
            std::unique_ptr<Exprs>((Exprs*)(elements_->Clone().release()))
        );
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<FnCallExpr>(
            std::make_unique<IdExpr>(callee_->value_),
            std::unique_ptr<Exprs>((Exprs*)(args_->Clone().release()))
        );
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<MethodCallExpr>(
            std::unique_ptr<Expr>((Expr*)(target_->Clone().release())),
            std::make_unique<IdExpr>(callee_->value_),
            std::unique_ptr<Exprs>((Exprs*)(args_->Clone().release()))
        );
    }
};
class FnExpr            : public Expr {
public:
    std::string name_     = "";
    std::string ret_type_ = "";
    std::unique_ptr<Exprs>     params_ = nullptr;
    std::unique_ptr<BlockExpr> block_  = nullptr;

    FnExpr(
        std::string name,
        std::string ret_type,
        std::unique_ptr<Exprs>     params,
        std::unique_ptr<BlockExpr> block
    )
    :   name_(name),
        ret_type_((ret_type.empty() || ret_type == "none") ? "none" : ret_type),
        params_(std::move(params)),
        block_(std::move(block))
    {
        type_ = AstType::FnExpr;
    }

    const std::string TypeName() const {
        return "FnExpr";
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("name", prefix);
        std::cout << COLOR_BLUE << name_ << COLOR_DEFAULT << std::endl;
        PrintLabel("ret_type", prefix);
        std::cout << COLOR_MAGENTA << ret_type_ << COLOR_DEFAULT << std::endl;
        if (params_) params_->Print(prefix, "params");
        if (block_)  block_->Print(prefix, "block");
    }

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<FnExpr>(
            name_,
            ret_type_,
            params_ ? std::unique_ptr<Exprs>((Exprs*)(params_->Clone().release())) : nullptr,
            block_  ? std::unique_ptr<BlockExpr>((BlockExpr*)(block_->Clone().release())) : nullptr
        );
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<NumConst>(value_);
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<BoolConst>(value_);
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<CharConst>(value_);
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<StringConst>(value_);
    }
};

// Stmt

class ExprStmt          : public Stmt {
public:
    std::unique_ptr<Expr> expr_ = nullptr;

    ExprStmt(std::unique_ptr<Expr> expr)
    :   expr_(std::move(expr))
    {
        type_ = AstType::ExprStmt;
    }

    const std::string TypeName() const {
        return "ExprStmt";
    }

    void PrintImpl(std::string prefix) override {
        if (expr_) expr_->Print(prefix, "expr");
    }

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<ExprStmt>(
            std::unique_ptr<Expr>((Expr*)(expr_->Clone().release()))
        );
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<AssignStmt>(
            std::unique_ptr<Expr>((Expr*)(target_->Clone().release())),
            std::unique_ptr<Expr>((Expr*)(value_->Clone().release()))
        );
    }
};
class CondStmt          : public Stmt {
public:
    std::unique_ptr<Expr>      cond_  = nullptr;
    std::unique_ptr<BlockExpr> block_ = nullptr;
    std::unique_ptr<CondStmt>  sub_   = nullptr;

    CondStmt(
        std::unique_ptr<Expr>      cond,
        std::unique_ptr<BlockExpr> block,
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<CondStmt>(
            cond_ ? std::unique_ptr<Expr>((Expr*)(cond_->Clone().release())) : nullptr,
            std::unique_ptr<BlockExpr>((BlockExpr*)(block_->Clone().release())),
            sub_ ? std::unique_ptr<CondStmt>((CondStmt*)(sub_->Clone().release())) : nullptr
        );
    }
};
class LoopSignalStmt    : public Stmt {
public:
    LoopSignal signal_;

    LoopSignalStmt(LoopSignal signal)
    :   signal_(signal)
    {
        type_ = AstType::LoopSignalStmt;
    }

    const std::string TypeName() const {
        return "LoopSignalStmt";
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("signal", prefix);
        std::cout << COLOR_MAGENTA;
        switch (signal_) {
            case LoopSignal::Break:     std::cout << "break"    << std::endl; break;
            case LoopSignal::Continue:  std::cout << "continue" << std::endl; break;
            default: __builtin_unreachable();
        }
        std::cout << COLOR_DEFAULT << std::endl;
    }

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<LoopSignalStmt>(signal_);
    }
};
class ForStmt           : public Stmt {
public:
    std::unique_ptr<IdExpr>     iter_;
    std::unique_ptr<Expr>       data_;
    std::unique_ptr<BlockExpr>  block_;

    ForStmt(
        std::unique_ptr<IdExpr>     iter,
        std::unique_ptr<Expr>       data,
        std::unique_ptr<BlockExpr>  block
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

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<ForStmt>(
            std::make_unique<IdExpr>(iter_->value_),
            std::unique_ptr<Expr>((Expr*)(data_->Clone().release())),
            std::unique_ptr<BlockExpr>((BlockExpr*)(block_->Clone().release()))
        );
    }
};
class ReturnSignalStmt  : public Stmt {
public:
    std::unique_ptr<Expr> value_;

    ReturnSignalStmt(std::unique_ptr<Expr> value)
    :   value_(std::move(value))
    {
        type_ = AstType::ReturnSignalStmt;
    }

    const std::string TypeName() const {
        return "ReturnSignalStmt";
    }

    void PrintImpl(std::string prefix) override {
        if (value_) value_->Print(prefix, "value");
    }

    std::unique_ptr<AstNode> Clone() const override {
        return std::make_unique<ReturnSignalStmt>(
            value_ ? std::unique_ptr<Expr>((Expr*)(value_->Clone().release())) : nullptr
        );
    }
};

// Program

class Program           : public BlockExpr {
public:
    Program(std::vector<std::unique_ptr<AstNode>>& children)
    :   BlockExpr(children)
    {
        type_ = AstType::Program;
    }

    const std::string TypeName() const {
        return "Program";
    }

    std::unique_ptr<AstNode> Clone() const override {
        std::vector<std::unique_ptr<AstNode>> children;
        for (auto& child : children_) {
            children.emplace_back(child->Clone());
        }
        return std::make_unique<Program>(children);
    }
};


//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <utility>

#include "common/utils.hpp"
#include "common/token.hpp"
#include "common/opertype.hpp"
#include "common/signal.hpp"

namespace sema {
    class Type;
    class FnSign;
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
    BlockExpr,          //  Packaged Astnode
    IdExpr,             //  Identity
    TypeExpr,           //  Type
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
    ReturnSignalStmt,   //  Return Signal
    ForStmt,            //  For
    WhileStmt,          //  While

    // Common
    Exprs,             // List of expr
};

inline static AstType BaseOfAstType(AstType type) {
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
        case TypeExpr:
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
        case ReturnSignalStmt:
        case ForStmt:
        case WhileStmt:
            return Stmt;

        default:
            return Undefined;
    }
}

inline static bool    isAstTypeCompatible(AstType expected, AstType actual) {
    if (expected == AstType::Undefined ||
        actual   == AstType::Undefined) return false;
    if (expected == actual) return true;
    return isAstTypeCompatible(expected, BaseOfAstType(actual));
}

// Node of Abstract Syntax Tree
class AstNode {
public:
    AstType     type_          = AstType::Undefined;
    Loc         loc_;
    sema::Type* resolved_type_ = nullptr;

public:
    virtual const std::string TypeName() const {
        return "Undefined";
    }
    void TypePrint() const {
        std::cerr << TypeName();
    }

    virtual void PrintImpl(std::string) {}
    void         Print(std::string prefix = "", std::string alias = "", bool isBegin = false) {
        std::cerr << prefix;
        if (!isBegin) std::cerr << "└── ";

        size_t indent_alias = 0;
        if (alias != "") {
            indent_alias = alias.length() + 3;
            std::cerr << COLOR_ORANGE << "[" << alias << "] " << COLOR_DEFAULT;
        }
        std::cerr << TypeName() << std::endl;

        PrintImpl(prefix + "    " + std::string(indent_alias, ' '));
    }
    void         PrintLabel(const std::string& name, std::string prefix = "") {
        std::cerr << prefix;
        std::cerr << "└── " << COLOR_ORANGE << "[" << name << "] " << COLOR_DEFAULT;
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

public:
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
        
        auto node = std::make_unique<Exprs>(exprs);
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};

// Expr

class BlockExpr         : public Expr {
public:
    std::vector<std::unique_ptr<AstNode>> children_;

public:
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
        
        auto node = std::make_unique<BlockExpr>(children);
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class IdExpr            : public Expr {
public:
    std::string name_ = "";

public:
    IdExpr(const std::string& name) : name_(name) {
        type_ = AstType::IdExpr;
    }

    const std::string TypeName() const {
        return "IdExpr";
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("name", prefix);
        std::cerr << COLOR_BLUE << name_ << COLOR_DEFAULT << std::endl;
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<IdExpr>(name_);
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class TypeExpr          : public Expr {
public:
    std::string            basic_type_ = "";
    std::unique_ptr<Exprs> params_     = nullptr;

public:
    TypeExpr(
        const std::string& type_basic,
        std::unique_ptr<Exprs> params
    )
    :   basic_type_(type_basic),
        params_(std::move(params))
    {
        type_ = AstType::TypeExpr;
    }

    const std::string TypeName() const {
        return "TypeExpr";
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("type_basic", prefix);
        std::cerr << COLOR_BLUE << basic_type_ << COLOR_DEFAULT << std::endl;
        if (params_) params_->Print(prefix, "params");
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<TypeExpr>(
            basic_type_,
            params_ ? std::unique_ptr<Exprs>((Exprs*)(params_->Clone().release())) : nullptr
        );
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class DeclExpr          : public Expr {
public:
    std::string               id_        = "";
    std::unique_ptr<TypeExpr> bind_type_ = nullptr;
    std::unique_ptr<Expr>     value_     = nullptr;

public:
    DeclExpr(
        const std::string&        id,
        std::unique_ptr<TypeExpr> bind_type,
        std::unique_ptr<Expr>     value
    )
    :   id_(id),
        bind_type_(std::move(bind_type)),
        value_(std::move(value))
    {
        type_ = AstType::DeclExpr;
    }

    const std::string TypeName() const {
        return "DeclExpr";
    }

    void PrintImpl(std::string prefix) override {
        PrintLabel("id", prefix);
        std::cerr << COLOR_BLUE << id_ << COLOR_DEFAULT << std::endl;
        if (bind_type_) bind_type_->Print(prefix, "bind_type");
        if (value_) value_->Print(prefix, "value");
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<DeclExpr>(
            id_,
            bind_type_ ? std::unique_ptr<TypeExpr>((TypeExpr*)(bind_type_->Clone().release())) : nullptr,
            value_ ? std::unique_ptr<Expr>((Expr*)(value_->Clone().release())) : nullptr
        );
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class OperExpr          : public Expr {
public:
    OperType oper_type_ = OperType::Undefined;
    std::unique_ptr<Expr> lexpr_ = nullptr;
    std::unique_ptr<Expr> rexpr_ = nullptr;

public:
    OperExpr(
        OperType oper_type,
        std::unique_ptr<Expr> lexpr,
        std::unique_ptr<Expr> rexpr
    )
    :   oper_type_(oper_type),
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
        std::cerr << COLOR_MAGENTA;
        std::cerr << OperTypeName(oper_type_);
        std::cerr << COLOR_DEFAULT << std::endl;
        lexpr_->Print(prefix, "lexpr");
        if (rexpr_) rexpr_->Print(prefix, "rexpr");
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node =  std::make_unique<OperExpr>(
            oper_type_,
            std::unique_ptr<Expr>((Expr*)(lexpr_->Clone().release())),
            rexpr_ ? std::unique_ptr<Expr>((Expr*)(rexpr_->Clone().release())) : nullptr
        );
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class RangeExpr         : public Expr {
public:
    std::unique_ptr<Expr> lexpr_ = nullptr;
    std::unique_ptr<Expr> rexpr_ = nullptr;
    std::unique_ptr<Expr> step_  = nullptr;
    bool isClosed_ = false;

public:
    sema::Type* iter_type_ = nullptr;

public:
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
        std::cerr << COLOR_MAGENTA;
        if (isClosed_)  Token::TypePrint(Token::Type::DotDotEq);
        else            Token::TypePrint(Token::Type::DotDot);
        std::cerr << COLOR_DEFAULT << std::endl;

        lexpr_->Print(prefix, "lexpr");
        rexpr_->Print(prefix, "rexpr");
        if (step_) step_->Print(prefix, "step");
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<RangeExpr>(
            std::unique_ptr<Expr>((Expr*)(lexpr_->Clone().release())),
            std::unique_ptr<Expr>((Expr*)(rexpr_->Clone().release())),
            step_ ? std::unique_ptr<Expr>((Expr*)(step_->Clone().release())) : nullptr,
            isClosed_
        );
        node->iter_type_ = iter_type_;
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class ArrayExpr         : public Expr {
public:
    std::unique_ptr<Exprs> elems_;

public:
    sema::Type* elem_type_ = nullptr;

public:
    ArrayExpr(std::unique_ptr<Exprs> elems)
    :   elems_(std::move(elems))
    {
        type_ = AstType::ArrayExpr;
    }

    const std::string TypeName() const {
        return "ArrayExpr";
    }

    void PrintImpl(std::string prefix) override {
        elems_->Print(prefix, "elems");
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<ArrayExpr>(
            std::unique_ptr<Exprs>((Exprs*)(elems_->Clone().release()))
        );
        node->elem_type_ = elem_type_;
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class FnCallExpr        : public Expr {
public:
    std::unique_ptr<IdExpr> callee_ = nullptr;
    std::unique_ptr<Exprs>  args_   = nullptr;

public:
    const sema::FnSign* callee_fnsign_ = nullptr;

public:
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
        auto node = std::make_unique<FnCallExpr>(
            std::make_unique<IdExpr>(callee_->name_),
            std::unique_ptr<Exprs>((Exprs*)(args_->Clone().release()))
        );
        node->callee_fnsign_ = callee_fnsign_;
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class MethodCallExpr    : public Expr {
public:
    std::unique_ptr<Expr>   target_ = nullptr;
    std::unique_ptr<IdExpr> callee_ = nullptr;
    std::unique_ptr<Exprs>  args_   = nullptr;

public:
    const sema::FnSign* callee_fnsign_ = nullptr;

public:
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
        auto node = std::make_unique<MethodCallExpr>(
            std::unique_ptr<Expr>((Expr*)(target_->Clone().release())),
            std::make_unique<IdExpr>(callee_->name_),
            std::unique_ptr<Exprs>((Exprs*)(args_->Clone().release()))
        );
        node->callee_fnsign_ = callee_fnsign_;
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class FnExpr            : public Expr {
public:
    std::string                name_     = "";
    std::unique_ptr<TypeExpr>  ret_type_ = nullptr;
    std::unique_ptr<Exprs>     params_   = nullptr;
    std::unique_ptr<BlockExpr> block_    = nullptr;

public:
    sema::Type*         ret_resolved_type_ = nullptr;
    const sema::FnSign* fnsign_            = nullptr;

public:
    FnExpr(
        std::string                name,
        std::unique_ptr<TypeExpr>  ret_type,
        std::unique_ptr<Exprs>     params,
        std::unique_ptr<BlockExpr> block
    )
    :   name_(name),
        ret_type_(std::move(ret_type)),
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
        std::cerr << COLOR_BLUE << name_ << COLOR_DEFAULT << std::endl;
        if (ret_type_) ret_type_->Print(prefix, "ret_type");
        if (params_) params_->Print(prefix, "params");
        if (block_)  block_->Print(prefix, "block");
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<FnExpr>(
            name_,
            ret_type_ ? std::unique_ptr<TypeExpr>((TypeExpr*)(ret_type_->Clone().release())) : nullptr,
            params_ ? std::unique_ptr<Exprs>((Exprs*)(params_->Clone().release())) : nullptr,
            block_  ? std::unique_ptr<BlockExpr>((BlockExpr*)(block_->Clone().release())) : nullptr
        );

        node->ret_resolved_type_ = ret_resolved_type_;
        node->fnsign_            = fnsign_;

        node->resolved_type_     = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};

// Const

class NumConst          : public Const {
public:
    std::string value_ = "";
    
public:
    union {
        int64_t integer_;
        double  floating_;
    } resolved_value_ = {};

public:
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
        std::cerr << COLOR_ORANGE << value_ << COLOR_DEFAULT << std::endl;
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<NumConst>(value_);
        node->resolved_value_ = resolved_value_;
        node->resolved_type_  = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class BoolConst         : public Const {
public:
    bool value_ = false;

public:
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
            std::cerr << COLOR_GREEN << "true" << COLOR_DEFAULT << std::endl;
        else
            std::cerr << COLOR_RED << "false" << COLOR_DEFAULT << std::endl;
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<BoolConst>(value_);
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class CharConst         : public Const {
public:
    std::string value_ = "";

public:
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
        std::cerr << COLOR_GREEN << "'" <<
            ContainedEscapePrint(value_)
        << "'" << COLOR_DEFAULT << std::endl;
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<CharConst>(value_);
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class StringConst       : public Const {
public:
    std::string value_ = "";

public:
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
        std::cerr << COLOR_GREEN << "\"" <<
            ContainedEscapePrint(value_)
        << "\"" << COLOR_DEFAULT << std::endl;
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<StringConst>(value_);
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};

// Stmt

class ExprStmt          : public Stmt {
public:
    std::unique_ptr<Expr> expr_ = nullptr;

public:
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
        auto node = std::make_unique<ExprStmt>(
            std::unique_ptr<Expr>((Expr*)(expr_->Clone().release()))
        );
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class AssignStmt        : public Stmt {
public:
    std::unique_ptr<Expr> target_ = nullptr;
    std::unique_ptr<Expr> value_  = nullptr;

public:
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
        auto node = std::make_unique<AssignStmt>(
            std::unique_ptr<Expr>((Expr*)(target_->Clone().release())),
            std::unique_ptr<Expr>((Expr*)(value_->Clone().release()))
        );
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class CondStmt          : public Stmt {
public:
    std::unique_ptr<Expr>      cond_ = nullptr;
    std::unique_ptr<BlockExpr> then_ = nullptr;
    std::unique_ptr<CondStmt>  next_ = nullptr;

public:
    CondStmt(
        std::unique_ptr<Expr>      cond,
        std::unique_ptr<BlockExpr> then,
        std::unique_ptr<CondStmt>  next
    )
    :   cond_(std::move(cond)),
        then_(std::move(then)),
        next_(std::move(next))
    {
        type_ = AstType::CondStmt;
    }

    const std::string TypeName() const {
        return "CondStmt";
    }

    void PrintImpl(std::string prefix) override {
        if (cond_) cond_->Print(prefix, "cond");
        then_->Print(prefix, "then");
        if (next_) next_->Print(prefix, "next");
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<CondStmt>(
            cond_ ? std::unique_ptr<Expr>((Expr*)(cond_->Clone().release())) : nullptr,
            std::unique_ptr<BlockExpr>((BlockExpr*)(then_->Clone().release())),
            next_ ? std::unique_ptr<CondStmt>((CondStmt*)(next_->Clone().release())) : nullptr
        );
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class LoopSignalStmt    : public Stmt {
public:
    LoopSignal signal_;

public:
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
        std::cerr << COLOR_MAGENTA;
        switch (signal_) {
            case LoopSignal::Break:     std::cerr << "break"    << std::endl; break;
            case LoopSignal::Continue:  std::cerr << "continue" << std::endl; break;
            default: std::unreachable();
        }
        std::cerr << COLOR_DEFAULT << std::endl;
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<LoopSignalStmt>(signal_);
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class ReturnSignalStmt  : public Stmt {
public:
    std::unique_ptr<Expr> value_;

public:
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
        auto node = std::make_unique<ReturnSignalStmt>(
            value_ ? std::unique_ptr<Expr>((Expr*)(value_->Clone().release())) : nullptr
        );
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class ForStmt           : public Stmt {
public:
    std::unique_ptr<IdExpr>     iter_;
    std::unique_ptr<Expr>       data_;
    std::unique_ptr<BlockExpr>  block_;

public:
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
        auto node = std::make_unique<ForStmt>(
            std::make_unique<IdExpr>(iter_->name_),
            std::unique_ptr<Expr>((Expr*)(data_->Clone().release())),
            std::unique_ptr<BlockExpr>((BlockExpr*)(block_->Clone().release()))
        );
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};
class WhileStmt         : public Stmt {
public:
    std::unique_ptr<Expr>       cond_;
    std::unique_ptr<BlockExpr>  block_;

public:
    WhileStmt(
        std::unique_ptr<Expr>       cond,
        std::unique_ptr<BlockExpr>  block
    )
    :   cond_(std::move(cond)),
        block_(std::move(block))
    {
        type_ = AstType::WhileStmt;
    }

    const std::string TypeName() const {
        return "WhileStmt";
    }

    void PrintImpl(std::string prefix) override {
        cond_->Print(prefix, "cond");
        block_->Print(prefix, "block");
    }

    std::unique_ptr<AstNode> Clone() const override {
        auto node = std::make_unique<WhileStmt>(
            std::unique_ptr<Expr>((Expr*)(cond_->Clone().release())),
            std::unique_ptr<BlockExpr>((BlockExpr*)(block_->Clone().release()))
        );
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
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

public:
    const std::string TypeName() const {
        return "Program";
    }

    std::unique_ptr<AstNode> Clone() const override {
        std::vector<std::unique_ptr<AstNode>> children;
        for (auto& child : children_) {
            children.emplace_back(child->Clone());
        }
        
        auto node = std::make_unique<Program>(children);
        node->resolved_type_ = resolved_type_;
        node->loc_ = loc_;
        return node;
    }
};

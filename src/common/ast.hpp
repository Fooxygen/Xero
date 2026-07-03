
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
    Program,        //  Entrance

    // Const
    Const,          //  Base ------
    NumConst,       //  Number Type

    // Expr
    Expr,           //  Base ------
    IdExpr,         //  Identity
    OperExpr,       //  Operation
    NegExpr,        //  Negative

    // Stmt
    Stmt,           //  Base ------
    DeclStmt,       //  Declaration
    AssignStmt,     //  Assignment
};

static AstType BaseOfAstType(AstType type) {
    switch (type) {
        case AstType::NumConst:
            return AstType::Const;
        case AstType::Const:
            return AstType::Expr;

        case AstType::IdExpr:
        case AstType::OperExpr:
        case AstType::NegExpr:
            return AstType::Expr;

        case AstType::DeclStmt:
        case AstType::AssignStmt:
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
            case AstType::Program:      return "Program";

            case AstType::Const:        return "Const";
            case AstType::NumConst:     return "NumConst";

            case AstType::Expr:         return "Expr";
            case AstType::IdExpr:       return "IdExpr";
            case AstType::OperExpr:     return "OperExpr";
            case AstType::NegExpr:      return "NegExpr";

            case AstType::Stmt:         return "Stmt";
            case AstType::DeclStmt:     return "DeclStmt";
            case AstType::AssignStmt:   return "AssignStmt";

            default: 
                LogWarn(LogModule::Parser, "undefined print name for AstNode");
                return "Undefined";
        }
    }
    const void TypePrint() const {
        std::cout << TypeName();
    }

    // Structure Print
    void         AstPrint(std::string indent = "", size_t expand = 4) {
        // Expand indent for the next layer
        indent += std::string(expand, ' ');
        TypePrint();
        AstPrintImpl(indent, expand);
    }
    // Structture Print Implement
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

// Expr
class Expr : public AstNode {

};
class IdExpr : public Expr {
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
class OperExpr : public Expr {
public:
    Token::Type opertype_ = Token::Type::Undefined;
    std::unique_ptr<Expr> lexpr_;
    std::unique_ptr<Expr> rexpr_;

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
class NegExpr : public Expr {
public:
    std::unique_ptr<Expr> expr_;

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

// Const
class Const : public Expr {

};
class NumConst : public Const {
public:
    std::string value_;

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

// Stmt
class Stmt : public AstNode {

};
class DeclStmt : public Stmt {
public:
    std::unique_ptr<IdExpr> id_;
    std::unique_ptr<Expr>   value_;
    std::unique_ptr<IdExpr> value_type_;

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
class AssignStmt : public Stmt {
public:
    std::unique_ptr<IdExpr> id_;
    std::unique_ptr<Expr>   value_;

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

// Program
class Program : public AstNode {
private:
    std::vector<std::unique_ptr<AstNode>> children_;

public:
    Program(
        std::vector<std::unique_ptr<AstNode>>& children
    )
    :   children_(std::move(children))
    {
        type_ = AstType::Program;
    }

    void AstPrintImpl(std::string indent, size_t expand) override {
        for (auto& child : children_) {
            AstLayerPrint(indent, "");
            child->AstPrint(indent, expand);
        }
        std::cout << std::endl;
    }

    std::vector<std::unique_ptr<AstNode>>& children() {
        return children_;
    }
};

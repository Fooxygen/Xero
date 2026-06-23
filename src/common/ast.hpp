
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <iostream>
#include <memory>
#include <vector>

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
            case AstType::Program:  return "Program";

            case AstType::Const:    return "Const";
            case AstType::NumConst: return "NumConst";

            case AstType::Expr:     return "Expr";
            case AstType::IdExpr:   return "IdExpr";
            case AstType::OperExpr: return "OperExpr";
            case AstType::NegExpr:  return "NegExpr";

            case AstType::Stmt:     return "Stmt";
            case AstType::DeclStmt: return "DeclStmt";

            default: 
                LogWarn(LogModule::Parser, "undefined print name for AstNode");
                return "Undefined";
        }
    }
    const void TypePrint() const {
        std::cout << TypeName();
    }

    virtual void AstPrint(std::string layer = "") {}
    std::string  AstLayerUpdate(std::string prev) {
        return prev + "    ";
    }
    void         AstLayerPrint(std::string layer, std::string label) {
        std::cout << '\n' << layer;
        if (!label.empty()) std::cout << label << ": ";
    }
};

// Expr
class Expr : public AstNode {

};
class IdExpr : public Expr {
public:
    std::string id_ = "";

    IdExpr(const std::string& id) : id_(id) {
        type_ = AstType::IdExpr;
    }

    void AstPrint(std::string layer) override {
        layer = AstLayerUpdate(layer);
        TypePrint();
        
        AstLayerPrint(layer, "id");
        std::cout << id_;
    }
};
class OperExpr : public Expr {
public:
    Token::Type opertype_ = Token::Type::Undefined;
    std::unique_ptr<Expr> lexpr_;
    std::unique_ptr<Expr> rexpr_;

    OperExpr(
        Token::Type opertype,
        std::unique_ptr<Expr>     lexpr,
        std::unique_ptr<Expr>     rexpr
    )
    :   opertype_(opertype),
        lexpr_(std::move(lexpr)),  
        rexpr_(std::move(rexpr))
    {
        type_ = AstType::OperExpr;
    }

    void AstPrint(std::string layer) override {
        layer = AstLayerUpdate(layer);
        TypePrint();
        
        AstLayerPrint(layer, "oper");
        Token::TypePrint(opertype_);

        AstLayerPrint(layer, "lexpr");
        lexpr_->AstPrint(layer);

        AstLayerPrint(layer, "rexpr");
        rexpr_->AstPrint(layer);
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

    void AstPrint(std::string layer) override {
        layer = AstLayerUpdate(layer);
        TypePrint();
        
        AstLayerPrint(layer, "expr");
        expr_->AstPrint(layer);
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

    void AstPrint(std::string layer) override {
        layer = AstLayerUpdate(layer);
        TypePrint();

        AstLayerPrint(layer, "value");
        std::cout << value_;
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

    void AstPrint(std::string layer) override {
        layer = AstLayerUpdate(layer);
        TypePrint();
        
        AstLayerPrint(layer, "id");
        id_->AstPrint(layer);

        AstLayerPrint(layer, "value");
        value_->AstPrint(layer);

        AstLayerPrint(layer, "value_type");
        value_type_->AstPrint(layer);
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

    void AstPrint(std::string layer) override {
        LogStart(LogModule::Parser, "output ast").Print();

        layer = AstLayerUpdate(layer);
        TypePrint();

        for (auto& child : children_) {
            AstLayerPrint(layer, "");
            child->AstPrint(layer);
        }
        
        std::cout << std::endl;
        LogFinish(LogModule::Parser, "output ast").Print();
    }

    std::vector<std::unique_ptr<AstNode>>& children() {
        return children_;
    }
};

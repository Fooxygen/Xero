
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "parser.hpp"

namespace parser {

    void Parser::RulesInit() {
        using TT      = Token::Type;
        using AT      = AstType;
        using PATS    = std::initializer_list<SymbolPattern>;
        using ASTNODE = std::unique_ptr<AstNode>;

        // Delay Reduction
        static auto isExprsBoundary = [](const Token* next) {
            if (!next) return true;                                 // EOF
            return  next->type() == Token::Type::Comma ||           // ,
                    next->type() == Token::Type::Semicolon ||       // ;
                    next->type() == Token::Type::RParen ||          // )
                    next->type() == Token::Type::RBkt;              // ]
        };

        rules_.clear();
        
        // Declare and Assign
        
        // └─ id: expr = expr; -> declarestmt
        {
            RuleAdd(
                PATS{
                    AT::IdExpr,
                    TT::Colon,
                    AT::IdExpr,
                    TT::Assign,
                    AT::Expr,
                    TT::Semicolon
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    return std::make_unique<DeclStmt>(
                        Rule::Move<IdExpr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 5),
                        Rule::Move<IdExpr>(symbols, 3)
                    );
                }
            );
        }
        // └─ expr = expr; -> assignstmt
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::Assign,
                    AT::Expr,
                    TT::Semicolon
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    return std::make_unique<AssignStmt>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }

        // Exprs

        // └─ expr, expr -> exprs
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::Comma,
                    AT::Expr
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {
                    if (!isExprsBoundary(token_next)) return nullptr;

                    std::vector<std::unique_ptr<Expr>> args;
                    args.emplace_back(Rule::Move<Expr>(symbols, 1));
                    args.emplace_back(Rule::Move<Expr>(symbols, 3));
                    return std::make_unique<Exprs>(args);
                }
            );
        }
        // └─ exprs, expr -> exprs
        {
            RuleAdd(
                PATS{
                    AT::Exprs,
                    TT::Comma,
                    AT::Expr
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {
                    if (!isExprsBoundary(token_next)) return nullptr;
                    
                    auto exprs = Rule::Move<Exprs>(symbols, 1);
                    auto expr  = Rule::Move<Expr>(symbols, 3);
                    exprs->exprs_.emplace_back(std::move(expr));
                    return exprs;
                }
            );
        }
        
        // Fn and Method

        // └─ fncallexpr; -> fncallexpr
        {
            RuleAdd(
                PATS{
                    AT::FnCallExpr,
                    TT::Semicolon
                },
                [](auto& symbols, auto*) {
                    return Rule::Move<FnCallExpr>(symbols, 1);
                }
            );
        }
        
        // └─ methodcallexpr; -> methodcallexpr
        {
            RuleAdd(
                PATS{
                    AT::MethodCallExpr,
                    TT::Semicolon
                },
                [](auto& symbols, auto*) {
                    return Rule::Move<MethodCallExpr>(symbols, 1);
                }
            );
        }
        
        // └─ target.expr(expr) -> methodcallexpr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::Dot,
                    AT::IdExpr,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    std::vector<std::unique_ptr<Expr>> args;
                    args.emplace_back(
                        Rule::Move<Expr>(symbols, 5)
                    );

                    return std::make_unique<MethodCallExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<IdExpr>(symbols, 3),
                        std::make_unique<Exprs>(args)
                    );
                }
            );
        }
        // └─ target.expr(exprs?) -> methodcallexpr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::Dot,
                    AT::IdExpr,
                    TT::LParen,
                    SymbolPattern::Opt(AT::Exprs),
                    TT::RParen
                },
                [](std::vector<Symbol>& symbols, auto*) {

                    // Option Check
                    if (Rule::move_positions_[4] == 0) {
                        std::vector<std::unique_ptr<Expr>> args;
                        return std::make_unique<MethodCallExpr>(
                            Rule::Move<Expr>(symbols, 1),
                            Rule::Move<IdExpr>(symbols, 3),
                            std::make_unique<Exprs>(args)
                        );
                    }
                    else {
                        return std::make_unique<MethodCallExpr>(
                            Rule::Move<Expr>(symbols, 1),
                            Rule::Move<IdExpr>(symbols, 3),
                            Rule::Move<Exprs>(symbols, 5)
                        );
                    }
                }
            );
        }
        
        // └─ expr(expr) -> fncallexpr
        {
            RuleAdd(
                PATS{
                    AT::IdExpr,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    std::vector<std::unique_ptr<Expr>> args;
                    args.emplace_back(
                        Rule::Move<Expr>(symbols, 3)
                    );

                    return std::make_unique<FnCallExpr>(
                        Rule::Move<IdExpr>(symbols, 1),
                        std::make_unique<Exprs>(args)
                    );
                }
            );
        }
        // └─ expr(exprs?) -> fncallexpr
        {
            RuleAdd(
                PATS{
                    AT::IdExpr,
                    TT::LParen,
                    SymbolPattern::Opt(AT::Exprs),
                    TT::RParen
                },
                [](std::vector<Symbol>& symbols, auto*) {

                    // Option Check
                    if (Rule::move_positions_[2] == 0) {
                        std::vector<std::unique_ptr<Expr>> args;
                        return std::make_unique<FnCallExpr>(
                            Rule::Move<IdExpr>(symbols, 1),
                            std::make_unique<Exprs>(args)
                        );
                    }
                    else {
                        return std::make_unique<FnCallExpr>(
                            Rule::Move<IdExpr>(symbols, 1),
                            Rule::Move<Exprs>(symbols, 3)
                        );
                    }
                }
            );
        }

        // Pick

        // └─ expr[expr] -> pickexpr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::LBkt,
                    AT::Expr,
                    TT::RBkt
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    return std::make_unique<PickExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }

        // Array

        // └─ [expr] -> arrayexpr
        {
            RuleAdd(
                PATS{
                    TT::LBkt,
                    AT::Expr,
                    TT::RBkt
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    std::vector<std::unique_ptr<Expr>> elements;
                    elements.emplace_back(
                        Rule::Move<Expr>(symbols, 2)
                    );

                    return std::make_unique<ArrayExpr>(
                        std::make_unique<Exprs>(elements)
                    );
                }
            );
        }
        // └─ [exprs?] -> arrayexpr
        {
            RuleAdd(
                PATS{
                    TT::LBkt,
                    SymbolPattern::Opt(AT::Exprs),
                    TT::RBkt
                },
                [](std::vector<Symbol>& symbols, auto*) {

                    // Option Check
                    if (Rule::move_positions_[1] == 0) {
                        std::vector<std::unique_ptr<Expr>> elements;
                        return std::make_unique<ArrayExpr>(
                            std::make_unique<Exprs>(elements)
                        );
                    }
                    else {
                        return std::make_unique<ArrayExpr>(
                            Rule::Move<Exprs>(symbols, 2)
                        );
                    }
                }
            );
        }

        // Basic Oper

        // └─ expr *|/ expr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::StarOrSlash,
                    AT::Expr,
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) {
                    size_t syms_len = symbols.size();
                    auto   opertype = std::get<Token>(symbols[syms_len - 2].data()).type();
                    return std::make_unique<OperExpr>(
                        opertype,
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }
        // └─ expr +|- expr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::PlusOrMinus,
                    AT::Expr,
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {
                    size_t syms_len = symbols.size();

                    // Oper Priority Check
                    auto opertype = std::get<Token>(symbols[syms_len - 2].data()).type();
                    if (token_next && isOperPriority(token_next->type(), opertype))
                    return nullptr;     // postpone

                    return std::make_unique<OperExpr>(
                        opertype,
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }
        // └─ (expr) -> expr
        {
            RuleAdd(
                PATS{
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {
                    if (token_next && token_next->type() == Token::Type::LBrace)
                        return nullptr;

                    return Rule::Move<Expr>(symbols, 2);
                }
            );
        }
        // └─ -expr -> negexpr
        {
            RuleAdd(
                PATS{
                    TT::Minus,
                    AT::Expr,
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    return std::make_unique<NegExpr>(
                        Rule::Move<Expr>(symbols, 2)
                    );
                }
            );
        }
        // └─ !expr -> notexpr
        {
            RuleAdd(
                PATS{
                    TT::Not,
                    AT::Expr,
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    return std::make_unique<NotExpr>(
                        Rule::Move<Expr>(symbols, 2)
                    );
                }
            );
        }
        // └─ expr relationoper expr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::RelationOper,
                    AT::Expr
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {
                    size_t syms_len = symbols.size();

                    auto op = std::get<Token>(symbols[syms_len - 2].data()).type();
                    if (token_next && isOperPriority(token_next->type(), op))
                        return nullptr;

                    return std::make_unique<OperExpr>(op,
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }
        // └─ expr logicaloper expr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::LogicalOper,
                    AT::Expr
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {
                    size_t syms_len = symbols.size();

                    auto op = std::get<Token>(symbols[syms_len - 2].data()).type();
                    if (token_next && isOperPriority(token_next->type(), op))
                        return nullptr;

                    return std::make_unique<OperExpr>(op,
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }
        
        // Range
 
        // └─ expr .. expr .. expr -> rangeexpr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::DotDot,
                    AT::Expr,
                    TT::DotDot,
                    AT::Expr,
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {

                    // Delay Reduction
                    if (token_next && (
                        token_next->type() == TT::DotDot ||
                        token_next->type() == TT::DotDotEq ||
                        token_next->type() == TT::LBkt ||
                        token_next->type() == TT::LParen ||
                        token_next->type() == TT::Dot
                    )) return nullptr;
                    
                    return std::make_unique<RangeExpr>(
                        TT::DotDot,
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 5),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }
        // └─ expr .. expr ..= expr -> rangeexpr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::DotDot,
                    AT::Expr,
                    TT::DotDotEq,
                    AT::Expr,
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {

                    // Delay Reduction
                    if (token_next && (
                        token_next->type() == TT::DotDot ||
                        token_next->type() == TT::DotDotEq ||
                        token_next->type() == TT::LBkt ||
                        token_next->type() == TT::LParen ||
                        token_next->type() == TT::Dot
                    )) return nullptr;

                    return std::make_unique<RangeExpr>(
                        TT::DotDotEq,
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 5),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }

        // └─ expr .. expr -> rangeexpr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::DotDot,
                    AT::Expr,
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {

                    // Delay Reduction
                    if (token_next && (
                        token_next->type() == TT::DotDot ||
                        token_next->type() == TT::DotDotEq ||
                        token_next->type() == TT::LBkt ||
                        token_next->type() == TT::LParen ||
                        token_next->type() == TT::Dot
                    )) return nullptr;

                    return std::make_unique<RangeExpr>(
                        TT::DotDot,
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3),
                        nullptr
                    );
                }
            );
        }
        // └─ expr ..= expr -> rangeexpr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::DotDotEq,
                    AT::Expr,
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {

                    // Delay Reduction
                    if (token_next && (
                        token_next->type() == TT::DotDot ||
                        token_next->type() == TT::DotDotEq ||
                        token_next->type() == TT::LBkt ||
                        token_next->type() == TT::LParen ||
                        token_next->type() == TT::Dot
                    )) return nullptr;

                    return std::make_unique<RangeExpr>(
                        TT::DotDotEq,
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3),
                        nullptr
                    );
                }
            );
        }

        // Logic Stmt

        // └─ if (cond) { } -> condstmt
        {
            RuleAdd(
                PATS{
                    TT::If,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    AT::BlockStmt
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    return std::make_unique<CondStmt>(
                        Rule::Move<Expr>(symbols, 3),
                        Rule::Move<BlockStmt>(symbols, 5),
                        nullptr
                    );
                }
            );
        }
    
        // └─ condstmt elif (cond) { } -> condstmt
        {
            RuleAdd(
                PATS{
                    AT::CondStmt,
                    TT::Elif,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    AT::BlockStmt
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    auto stmt = Rule::Move<CondStmt>(symbols, 1);

                    CondStmt* tail = stmt.get();
                    while (tail->sub_) tail = tail->sub_.get();

                    tail->sub_ = std::make_unique<CondStmt>(
                        Rule::Move<Expr>(symbols, 4),
                        Rule::Move<BlockStmt>(symbols, 6),
                        nullptr
                    );
                    return stmt;
                }
            );
        }

        // └─ condstmt else { } -> condstmt
        {
            RuleAdd(
                PATS{
                    AT::CondStmt,
                    TT::Else,
                    AT::BlockStmt
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    auto stmt = Rule::Move<CondStmt>(symbols, 1);

                    CondStmt* tail = stmt.get();
                    while (tail->sub_) tail = tail->sub_.get();

                    tail->sub_ = std::make_unique<CondStmt>(
                        nullptr,
                        Rule::Move<BlockStmt>(symbols, 3),
                        nullptr
                    );
                    return stmt;
                }
            );
        }
    
        // ForStmt

        // └─ for ( x in expr ) {} -> forstmt
        {
            RuleAdd(
                PATS{
                    TT::For,
                    TT::LParen,
                    AT::IdExpr,
                    TT::In,
                    AT::Expr,
                    TT::RParen,
                    AT::BlockStmt
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    return std::make_unique<ForStmt>(
                        Rule::Move<IdExpr>(symbols, 3),
                        Rule::Move<Expr>(symbols, 5),
                        Rule::Move<BlockStmt>(symbols, 7)
                    );
                }
            );
        }
    }
}

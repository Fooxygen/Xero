
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "parser.hpp"

namespace parser {

    void Parser::RulesInit() {
        using TT      = Token::Type;
        using AT      = AstType;
        using PATS    = std::initializer_list<SymbolPattern>;
        using TS      = std::initializer_list<Token::Type>;
        using ASTNODE = std::unique_ptr<AstNode>;

        // Oper Auxiliary

        static auto isOperPriority = [](Token::Type a, Token::Type b) {
            using TT = Token::Type;
            
            auto group = [](Token::Type type) {
                switch (type) {
                    case TT::LBkt:
                    case TT::LParen:
                    case TT::Dot:
                        return -1;

                    case TT::Star:
                    case TT::Slash:
                        return -2;

                    case TT::Plus:
                    case TT::Minus:
                        return -3;

                    case TT::Gt:
                    case TT::Ge:
                    case TT::Lt:
                    case TT::Le:
                    case TT::Eq:
                    case TT::Neq:
                    case TT::RelationOper:
                        return -4;

                    case TT::And:
                        return -5;

                    case TT::Or:
                        return -6;

                    default:
                        return -7;
                }
            };
            return group(a) > group(b);
        };

        // └─ Binary: expr <token> expr
        auto BinOperAdd = [&](Token::Type token, OperType op) {
            RuleAdd(
                PATS{
                    AT::Expr,
                    token,
                    AT::Expr
                },
                [op, token](std::vector<Symbol>& symbols, Token::Type token_next) -> ASTNODE {
                    if (isOperPriority(token_next, token))
                        return nullptr;     // postpone

                    return std::make_unique<OperExpr>(
                        op,
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        };
        
        // └─ Unary: <token> expr
        auto UnaryOperAdd = [&](Token::Type token, OperType op) {
            RuleAdd(
                PATS{
                    token,
                    AT::Expr
                },
                [op](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<OperExpr>(
                        op,
                        Rule::Move<Expr>(symbols, 2),
                        nullptr
                    );
                }
            );
        };

        rules_.clear();

        // Expr

        // └─ expr, expr -> exprs
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::Comma,
                    AT::Expr
                },
                [](std::vector<Symbol>& symbols, auto) -> ASTNODE {
                    std::vector<std::unique_ptr<Expr>> args;
                    args.emplace_back(Rule::Move<Expr>(symbols, 1));
                    args.emplace_back(Rule::Move<Expr>(symbols, 3));
                    return std::make_unique<Exprs>(args);
                },
                {},
                TS{
                    TT::Comma,
                    TT::Semicolon,
                    TT::RParen,
                    TT::RBkt
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
                [](std::vector<Symbol>& symbols, auto) -> ASTNODE {
                    auto exprs = Rule::Move<Exprs>(symbols, 1);
                    auto expr  = Rule::Move<Expr>(symbols, 3);
                    exprs->exprs_.emplace_back(std::move(expr));
                    return exprs;
                },
                {}, TS{
                    TT::Comma,
                    TT::Semicolon,
                    TT::RParen,
                    TT::RBkt
                }
            );
        }

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
                [](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<DeclStmt>(
                        Rule::Move<IdExpr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 5),
                        Rule::Move<IdExpr>(symbols, 3)
                    );
                }
            );
        }
        // └─ id: expr; -> declarestmt
        {
            RuleAdd(
                PATS{
                    AT::IdExpr,
                    TT::Colon,
                    AT::IdExpr,
                    TT::Semicolon
                },
                [](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<DeclStmt>(
                        Rule::Move<IdExpr>(symbols, 1),
                        nullptr,
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
                [](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<AssignStmt>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }
        // └─ expr += expr; -> assignstmt
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::PlusAssign,
                    AT::Expr,
                    TT::Semicolon
                },
                [](std::vector<Symbol>& symbols, auto) {
                    auto target = Rule::Move<Expr>(symbols, 1);
                    auto lexpr  = std::unique_ptr<Expr>((Expr*)(target->Clone().release()));
                    auto value  = std::make_unique<OperExpr>(
                        OperType::Plus,
                        std::move(lexpr),
                        Rule::Move<Expr>(symbols, 3)
                    );

                    return std::make_unique<AssignStmt>(
                        std::move(target),
                        std::move(value)
                    );
                }
            );
        }
        // └─ expr -= expr; -> assignstmt
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::MinusAssign,
                    AT::Expr,
                    TT::Semicolon
                },
                [](std::vector<Symbol>& symbols, auto) {
                    auto target = Rule::Move<Expr>(symbols, 1);
                    auto lexpr  = std::unique_ptr<Expr>((Expr*)(target->Clone().release()));
                    auto value  = std::make_unique<OperExpr>(
                        OperType::Minus,
                        std::move(lexpr),
                        Rule::Move<Expr>(symbols, 3)
                    );

                    return std::make_unique<AssignStmt>(
                        std::move(target),
                        std::move(value)
                    );
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
                [](auto& symbols, auto) {
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
                [](auto& symbols, auto) {
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
                [](std::vector<Symbol>& symbols, auto) {
                    std::vector<std::unique_ptr<Expr>> args;
                    args.emplace_back(Rule::Move<Expr>(symbols, 5));

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
                [](std::vector<Symbol>& symbols, auto) {

                    // Option Check
                    if (Rule::isPatternOpt(5)) {
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
                [](std::vector<Symbol>& symbols, auto) {
                    std::vector<std::unique_ptr<Expr>> args;
                    args.emplace_back(Rule::Move<Expr>(symbols, 3));

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
                [](std::vector<Symbol>& symbols, auto) {

                    // Option Check
                    if (Rule::isPatternOpt(3)) {
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
        // └─ (expr) -> expr
        {
            RuleAdd(
                PATS{
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                },
                [](std::vector<Symbol>& symbols, auto) -> ASTNODE {
                    return Rule::Move<Expr>(symbols, 2);
                },
                TS{ TT::LBrace }
            );
        }

        // Oper

        // └─ expr[expr]
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::LBkt,
                    AT::Expr,
                    TT::RBkt
                },
                [](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<OperExpr>(
                        OperType::Pick,
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }
        
        // └─ Binary

        BinOperAdd(TT::Star,  OperType::Star);
        BinOperAdd(TT::Slash, OperType::Slash);
        BinOperAdd(TT::Plus,  OperType::Plus);
        BinOperAdd(TT::Minus, OperType::Minus);
        BinOperAdd(TT::Gt,    OperType::Gt);
        BinOperAdd(TT::Ge,    OperType::Ge);
        BinOperAdd(TT::Lt,    OperType::Lt);
        BinOperAdd(TT::Le,    OperType::Le);
        BinOperAdd(TT::Eq,    OperType::Eq);
        BinOperAdd(TT::Neq,   OperType::Neq);
        BinOperAdd(TT::And,   OperType::And);
        BinOperAdd(TT::Or,    OperType::Or);

        // └─ Unary

        UnaryOperAdd(TT::Minus, OperType::Neg);
        UnaryOperAdd(TT::Not,   OperType::Not);
        
        // Array

        // └─ [expr] -> arrayexpr
        {
            RuleAdd(
                PATS{
                    TT::LBkt,
                    AT::Expr,
                    TT::RBkt
                },
                [](std::vector<Symbol>& symbols, auto) {
                    std::vector<std::unique_ptr<Expr>> elements;
                    elements.emplace_back(Rule::Move<Expr>(symbols, 2));

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
                [](std::vector<Symbol>& symbols, auto) {

                    // Option Check
                    if (Rule::isPatternOpt(2)) {
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
                [](std::vector<Symbol>& symbols, auto) -> ASTNODE {                    
                    return std::make_unique<RangeExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 5),
                        Rule::Move<Expr>(symbols, 3),
                        false
                    );
                },
                TS{
                    TT::DotDot,
                    TT::DotDotEq,
                    TT::LBkt,
                    TT::LParen,
                    TT::Dot
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
                [](std::vector<Symbol>& symbols, auto) -> ASTNODE {
                    return std::make_unique<RangeExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 5),
                        Rule::Move<Expr>(symbols, 3),
                        true
                    );
                },
                TS{
                    TT::DotDot,
                    TT::DotDotEq,
                    TT::LBkt,
                    TT::LParen,
                    TT::Dot
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
                [](std::vector<Symbol>& symbols, auto) -> ASTNODE {
                    return std::make_unique<RangeExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3),
                        nullptr,
                        false
                    );
                },
                TS{
                    TT::DotDot,
                    TT::DotDotEq,
                    TT::LBkt,
                    TT::LParen,
                    TT::Dot
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
                [](std::vector<Symbol>& symbols, auto) -> ASTNODE {
                    return std::make_unique<RangeExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3),
                        nullptr,
                        true
                    );
                },
                TS{
                    TT::DotDot,
                    TT::DotDotEq,
                    TT::LBkt,
                    TT::LParen,
                    TT::Dot
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
                [](std::vector<Symbol>& symbols, auto) {
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
                [](std::vector<Symbol>& symbols, auto) {
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
                [](std::vector<Symbol>& symbols, auto) {
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
    
        // Loop

        // └─ break; -> loopsignalstmt
        {
            RuleAdd(
                PATS{
                    TT::Break,
                    TT::Semicolon
                },
                [](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<LoopSignalStmt>(
                        LoopSignal::Break
                    );
                }
            );
        }
        // └─ continue; -> loopsignalstmt
        {
            RuleAdd(
                PATS{
                    TT::Continue,
                    TT::Semicolon
                },
                [](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<LoopSignalStmt>(
                        LoopSignal::Continue
                    );
                }
            );
        }
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
                [](std::vector<Symbol>& symbols, auto) {
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

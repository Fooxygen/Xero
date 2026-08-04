
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "parser.hpp"

namespace parser {

    void Parser::RulesInit() {
        using TT      = Token::Type;
        using AT      = AstType;
        using PATS    = std::initializer_list<SymbolPattern>;
        using TOKS    = std::initializer_list<Token::Type>;
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
                    case TT::ModF:
                    case TT::ModT:
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

        static auto TokenType2OperType = [](Token::Type token, bool isUnary) {
            using TT = Token::Type;
            using OT = OperType;

            switch (token) {
                case TT::Plus:          return OT::Plus;
                case TT::Minus:         return isUnary ? OT::Neg : OT::Minus;
                case TT::Star:          return OT::Star;
                case TT::Slash:         return OT::Slash;
                case TT::ModT:          return OT::ModT;
                case TT::ModF:          return OT::ModF;

                case TT::Gt:            return OT::Gt;
                case TT::Ge:            return OT::Ge;
                case TT::Lt:            return OT::Lt;
                case TT::Le:            return OT::Le;
                case TT::Eq:            return OT::Eq;
                case TT::Neq:           return OT::Neq;

                case TT::And:           return OT::And;
                case TT::Or:            return OT::Or;
                case TT::Not:           return OT::Not;

                case TT::PlusAssign:    return OT::Plus;
                case TT::MinusAssign:   return OT::Minus;
                case TT::StarAssign:    return OT::Star;
                case TT::SlashAssign:   return OT::Slash;
                case TT::ModTAssign:    return OT::ModT;
                case TT::ModFAssign:    return OT::ModF;

                default: __builtin_unreachable();
            }
        };

        // Stmt Auxiliary

        static auto Pack2BlockStmt = [](std::unique_ptr<AstNode> stmt)
        -> std::unique_ptr<BlockStmt>
        {
            if (stmt->type_ == AstType::BlockStmt) {
                return std::unique_ptr<BlockStmt>((BlockStmt*)stmt.release());
            }

            std::vector<std::unique_ptr<AstNode>> children;
            children.emplace_back(std::move(stmt));
            return std::make_unique<BlockStmt>(children);
        };

        rules_.clear();

        // Exprs

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
                {}, {},
                {}, TOKS{
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
                {}, {},
                {}, TOKS{
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
        // └─ expr assignoper expr; -> assignstmt
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    SymbolPattern({
                        TT::Assign,
                        TT::PlusAssign, TT::MinusAssign, TT::StarAssign, TT::SlashAssign,
                        TT::ModTAssign, TT::ModFAssign
                    }),
                    AT::Expr,
                    TT::Semicolon
                },
                [](std::vector<Symbol>& symbols, auto) {
                    auto token  = Rule::GetTokenType(symbols, 2);
                    auto target = Rule::Move<Expr>(symbols, 1);
                    auto value  = Rule::Move<Expr>(symbols, 3);

                    if (token == TT::Assign)
                        return std::make_unique<AssignStmt>(std::move(target), std::move(value));

                    auto lexpr = std::unique_ptr<Expr>((Expr*)(target->Clone().release()));
                    return std::make_unique<AssignStmt>(
                        std::move(target),
                        std::make_unique<OperExpr>(
                            TokenType2OperType(token, false),
                            std::move(lexpr),
                            std::move(value)
                        )
                    );
                }
            );
        }
        
        // Fn and Method
       
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
        
        // Expr
        
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
                PATS{ TT::If, TT::Elif, TT::For }
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
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    SymbolPattern({
                        TT::Star, TT::Slash, TT::ModT, TT::ModF,
                        TT::Plus, TT::Minus,
                        TT::Gt,   TT::Ge,    TT::Lt,   TT::Le, TT::Eq, TT::Neq,
                        TT::And,  TT::Or
                    }),
                    AT::Expr
                },
                [](std::vector<Symbol>& symbols, Token::Type token_next) -> ASTNODE {
                    auto tokentype = Rule::GetTokenType(symbols, 2);
                    if (isOperPriority(token_next, tokentype)) return nullptr;

                    return std::make_unique<OperExpr>(
                        TokenType2OperType(tokentype, false),
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }

        // └─ Unary
        {
            RuleAdd(
                PATS{
                    SymbolPattern({ TT::Minus, TT::Not }),
                    AT::Expr
                },
                [](std::vector<Symbol>& symbols, Token::Type token_next) {
                    return std::make_unique<OperExpr>(
                        TokenType2OperType(Rule::GetTokenType(symbols, 1), true),
                        Rule::Move<Expr>(symbols, 2),
                        nullptr
                    );
                },
                PATS{ AT::Expr }
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
                {}, {},
                TOKS{
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
                {}, {},
                TOKS{
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
                {}, {},
                TOKS{
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
                {}, {},
                TOKS{
                    TT::DotDot,
                    TT::DotDotEq,
                    TT::LBkt,
                    TT::LParen,
                    TT::Dot
                }
            );
        }

        // Logic Stmt

        // └─ if (cond) stmt -> condstmt
        {
            RuleAdd(
                PATS{
                    TT::If,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    AT::Stmt
                },
                [](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<CondStmt>(
                        Rule::Move<Expr>(symbols, 3),
                        Pack2BlockStmt(Rule::Move<Stmt>(symbols, 5)),
                        nullptr
                    );
                },
                {}, {},
                TOKS{ TT::Elif, TT::Else }
            );
        }
        // └─ if (expr) stmt elif (expr) stmt -> condstmt
        {
            RuleAdd(
                PATS{
                    TT::If,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    AT::Stmt,
                    TT::Elif,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    AT::Stmt
                },
                [](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<CondStmt>(
                        Rule::Move<Expr>(symbols, 3),
                        Pack2BlockStmt(Rule::Move<Stmt>(symbols, 5)),
                        std::make_unique<CondStmt>(
                            Rule::Move<Expr>(symbols, 8),
                            Pack2BlockStmt(Rule::Move<Stmt>(symbols, 10)),
                            nullptr
                        )
                    );
                }
            );
        }
        // └─ if (expr) stmt else stmt -> condstmt
        {
            RuleAdd(
                PATS{
                    TT::If,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    AT::Stmt,
                    TT::Else,
                    AT::Stmt
                },
                [](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<CondStmt>(
                        Rule::Move<Expr>(symbols, 3),
                        Pack2BlockStmt(Rule::Move<Stmt>(symbols, 5)),
                        std::make_unique<CondStmt>(
                            nullptr,
                            Pack2BlockStmt(Rule::Move<Stmt>(symbols, 7)),
                            nullptr
                        )
                    );
                }
            );
        }
        // └─ condstmt elif (cond) stmt -> condstmt
        {
            RuleAdd(
                PATS{
                    AT::CondStmt,
                    TT::Elif,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    AT::Stmt
                },
                [](std::vector<Symbol>& symbols, auto) {
                    auto stmt = Rule::Move<CondStmt>(symbols, 1);

                    CondStmt* tail = stmt.get();
                    while (tail->sub_) tail = tail->sub_.get();

                    tail->sub_ = std::make_unique<CondStmt>(
                        Rule::Move<Expr>(symbols, 4),
                        Pack2BlockStmt(Rule::Move<Stmt>(symbols, 6)),
                        nullptr
                    );
                    return stmt;
                }
            );
        }

        // └─ condstmt else stmt -> condstmt
        {
            RuleAdd(
                PATS{
                    AT::CondStmt,
                    TT::Else,
                    AT::Stmt
                },
                [](std::vector<Symbol>& symbols, auto) {
                    auto stmt = Rule::Move<CondStmt>(symbols, 1);

                    CondStmt* tail = stmt.get();
                    while (tail->sub_) tail = tail->sub_.get();

                    tail->sub_ = std::make_unique<CondStmt>(
                        nullptr,
                        Pack2BlockStmt(Rule::Move<Stmt>(symbols, 3)),
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
        // └─ for ( x in expr ) stmt -> forstmt
        {
            RuleAdd(
                PATS{
                    TT::For,
                    TT::LParen,
                    AT::IdExpr,
                    TT::In,
                    AT::Expr,
                    TT::RParen,
                    AT::Stmt
                },
                [](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<ForStmt>(
                        Rule::Move<IdExpr>(symbols, 3),
                        Rule::Move<Expr>(symbols, 5),
                        Pack2BlockStmt(Rule::Move<Stmt>(symbols, 7))
                    );
                }
            );
        }
    
        // Stmt

        // └─ expr; -> exprstmt
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::Semicolon
                },
                [](std::vector<Symbol>& symbols, auto) {
                    return std::make_unique<ExprStmt>(
                        Rule::Move<Expr>(symbols, 1)
                    );
                }
            );
        }
    }
}

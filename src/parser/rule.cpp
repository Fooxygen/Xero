
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <utility>

#include "parser/parser.hpp"

namespace parser {

    void Parser::RulesInit() {

        // Oper Auxiliary

        static auto isOperPriority = [](TT a, TT b) {
            auto group = [](TT type) {
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

        static auto TokenType2OperType = [](TT token, bool isUnary) {
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

                default: std::unreachable();
            }
        };

        // Pack Auxiliary

        static auto Pack2BlockExpr = [](ASTNODE stmt)
            -> std::unique_ptr<BlockExpr>
        {
            if (stmt->type_ == AT::BlockExpr) {
                return std::unique_ptr<BlockExpr>((BlockExpr*)stmt.release());
            }

            std::vector<ASTNODE> children;
            children.emplace_back(std::move(stmt));
            return std::make_unique<BlockExpr>(children);
        };
        
        static auto Pack2Exprs  = [](SS& symbols, size_t pos)
            -> std::unique_ptr<Exprs>
        {
            if (Rule::isOptPatternEmpty(pos)) {
                std::vector<std::unique_ptr<Expr>> empty;
                return std::make_unique<Exprs>(empty);
            }

            size_t mp = Rule::move_positions_[pos - 1];
            if (symbols[symbols.size() - mp].type_astnode() == AT::Exprs)
                return Rule::Move<Exprs>(symbols, pos);

            std::vector<std::unique_ptr<Expr>> exprs;
            exprs.emplace_back(Rule::Move<Expr>(symbols, pos));
            return std::make_unique<Exprs>(exprs);
        };

        rules_.clear();

        // Exprs

        // └─ expr, expr -> exprs
        {
            RuleAdd(
                PATS_INIT{
                    AT::Expr,
                    TT::Comma,
                    AT::Expr
                },
                [](SS& symbols, auto) -> ASTNODE {
                    std::vector<std::unique_ptr<Expr>> args;
                    args.emplace_back(Rule::Move<Expr>(symbols, 1));
                    args.emplace_back(Rule::Move<Expr>(symbols, 3));
                    return std::make_unique<Exprs>(args);
                },
                {}, {}, {},
                TTS_INIT{
                    TT::Comma,  TT::Semicolon,
                    TT::RParen, TT::RBkt, TT::REBkt
                }
            );
        }
        // └─ exprs, expr -> exprs
        {
            RuleAdd(
                PATS_INIT{
                    AT::Exprs,
                    TT::Comma,
                    AT::Expr
                },
                [](SS& symbols, auto) -> ASTNODE {
                    auto exprs = Rule::Move<Exprs>(symbols, 1);
                    auto expr  = Rule::Move<Expr>(symbols, 3);
                    exprs->exprs_.emplace_back(std::move(expr));
                    return exprs;
                },
                {}, {}, {},
                TTS_INIT{
                    TT::Comma,  TT::Semicolon,
                    TT::RParen, TT::RBkt, TT::REBkt
                }
            );
        }

        // Type, Declare and Assign
        
        // └─ id [= exprs =] -> typeexpr
        {
            RuleAdd(
                PATS_INIT{
                    AT::IdExpr,
                    TT::LEBkt,
                    SymbolPattern::Opt({ AT::Expr, AT::Exprs }),
                    TT::REBkt
                },
                [](SS& symbols, auto) {
                    return std::make_unique<TypeExpr>(
                        Rule::Move<IdExpr>(symbols, 1)->name_,
                        Pack2Exprs(symbols, 3)
                    );
                }
            );
        }
        // └─ id: id -> declareexpr
        {
            RuleAdd(
                PATS_INIT{
                    AT::IdExpr,
                    TT::Colon,
                    AT::IdExpr
                },
                [](SS& symbols, auto) {
                    return std::make_unique<DeclExpr>(
                        Rule::Move<IdExpr>(symbols, 1)->name_,
                        std::make_unique<TypeExpr>(
                            Rule::Move<IdExpr>(symbols, 3)->name_,
                            nullptr
                        ),
                        nullptr
                    );
                },
                {}, {},
                TTS_INIT{ TT::LEBkt }
            );
        }
        // └─ id: typeexpr -> declareexpr
        {
            RuleAdd(
                PATS_INIT{
                    AT::IdExpr,
                    TT::Colon,
                    AT::TypeExpr
                },
                [](SS& symbols, auto) {
                    return std::make_unique<DeclExpr>(
                        Rule::Move<IdExpr>(symbols, 1)->name_,
                        Rule::Move<TypeExpr>(symbols, 3),
                        nullptr
                    );
                }
            );
        }
        // └─ declareexpr = expr -> declareexpr
        {
            RuleAdd(
                PATS_INIT{
                    AT::DeclExpr,
                    TT::Assign,
                    AT::Expr
                },
                [](SS& symbols, auto) {
                    auto decl = Rule::Move<DeclExpr>(symbols, 1);
                    return std::make_unique<DeclExpr>(
                        decl->id_,
                        std::move(decl->bind_type_) ,
                        Rule::Move<Expr>(symbols, 3)
                    );
                },
                {}, {}, {},
                TTS_INIT{
                    TT::Semicolon, TT::Comma
                }
            );
        }
        // └─ expr assignoper expr; -> assignstmt
        {
            RuleAdd(
                PATS_INIT{
                    AT::Expr,
                    SymbolPattern({
                        TT::Assign,
                        TT::PlusAssign, TT::MinusAssign, TT::StarAssign, TT::SlashAssign,
                        TT::ModTAssign, TT::ModFAssign
                    }),
                    AT::Expr,
                    TT::Semicolon
                },
                [](SS& symbols, auto) {
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
       
        // └─ target.expr(exprs?) -> methodcallexpr
        {
            RuleAdd(
                PATS_INIT{
                    AT::Expr,
                    TT::Dot,
                    AT::IdExpr,
                    TT::LParen,
                    SymbolPattern::Opt({ AT::Expr, AT::Exprs}),
                    TT::RParen
                },
                [](SS& symbols, auto) {
                    return std::make_unique<MethodCallExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<IdExpr>(symbols, 3),
                        Pack2Exprs(symbols, 5)
                    );
                }
            );
        }

        // └─ expr(exprs?) -> fncallexpr
        {
            RuleAdd(
                PATS_INIT{
                    AT::IdExpr,
                    TT::LParen,
                    SymbolPattern::Opt({ AT::Expr, AT::Exprs}),
                    TT::RParen
                },
                [](SS& symbols, auto) {
                    return std::make_unique<FnCallExpr>(
                        Rule::Move<IdExpr>(symbols, 1),
                        Pack2Exprs(symbols, 3)
                    );
                },
                PATS_INIT{ TT::Fn }
            );
        }
        
        // └─ fn id?(exprs?) -> <id, typeexpr> blockexpr -> fnexpr
        {
            RuleAdd(
                PATS_INIT{
                    TT::Fn,
                    SymbolPattern::Opt(AT::IdExpr),
                    TT::LParen,
                    SymbolPattern::Opt({ AT::Expr, AT::Exprs }),
                    TT::RParen,
                    TT::Arrow,
                    SymbolPattern({ AT::IdExpr, AT::TypeExpr }),
                    AT::BlockExpr
                },
                [](SS& symbols, auto) -> ASTNODE {
                    std::string name = "";
                    if (!Rule::isOptPatternEmpty(2))
                        name = Rule::Move<IdExpr>(symbols, 2)->name_;

                    if (Rule::is(symbols, 7, AT::IdExpr)) {
                        return std::make_unique<FnExpr>(
                            name,
                            std::make_unique<TypeExpr>(
                                Rule::Move<IdExpr>(symbols, 7)->name_,
                                nullptr
                            ),
                            Pack2Exprs(symbols, 4),
                            Rule::Move<BlockExpr>(symbols, 8)
                        );
                    }
                    if (Rule::is(symbols, 7, AT::TypeExpr)) {
                        return std::make_unique<FnExpr>(
                            name,
                            Rule::Move<TypeExpr>(symbols, 7),
                            Pack2Exprs(symbols, 4),
                            Rule::Move<BlockExpr>(symbols, 8)
                        );
                    }

                    return nullptr;
                }
            );
        }
        // └─ fn id?(exprs?) blockexpr -> fnexpr
        {
            RuleAdd(
                PATS_INIT{
                    TT::Fn,
                    SymbolPattern::Opt(AT::IdExpr),
                    TT::LParen,
                    SymbolPattern::Opt({ AT::Expr, AT::Exprs }),
                    TT::RParen,
                    AT::BlockExpr
                },
                [](SS& symbols, auto) {
                    std::string name = "";
                    if (!Rule::isOptPatternEmpty(2)) name = Rule::Move<IdExpr>(symbols, 2)->name_;

                    return std::make_unique<FnExpr>(
                        name,
                        nullptr,
                        Pack2Exprs(symbols, 4),
                        Rule::Move<BlockExpr>(symbols, 6)
                    );
                }
            );
        }
        // └─ return expr?; -> returnsignalstmt
        {
            RuleAdd(
                PATS_INIT{
                    TT::Return,
                    SymbolPattern::Opt(AT::Expr),
                    TT::Semicolon
                },
                [](SS& symbols, auto) {
                    return std::make_unique<ReturnSignalStmt>(
                        !Rule::isOptPatternEmpty(2) ? Rule::Move<Expr>(symbols, 2) : nullptr
                    );
                }
            );
        }

        // Expr
        
        // └─ (expr) -> expr
        {
            RuleAdd(
                PATS_INIT{
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                },
                [](std::vector<Symbol>& symbols, auto) -> ASTNODE {
                    return Rule::Move<Expr>(symbols, 2);
                },
                PATS_INIT{ TT::If, TT::Elif, TT::For, TT::While },
                {},
                TTS_INIT{ TT::LBrace, TT::Arrow }
            );
        }

        // Oper

        // └─ expr[expr]
        {
            RuleAdd(
                PATS_INIT{
                    AT::Expr,
                    TT::LBkt,
                    AT::Expr,
                    TT::RBkt
                },
                [](SS& symbols, auto) {
                    return std::make_unique<OperExpr>(
                        OT::Pick,
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }
        
        // └─ Binary
        {
            RuleAdd(
                PATS_INIT{
                    AT::Expr,
                    SymbolPattern({
                        TT::Star, TT::Slash, TT::ModT, TT::ModF,
                        TT::Plus, TT::Minus,
                        TT::Gt,   TT::Ge,    TT::Lt,   TT::Le, TT::Eq, TT::Neq,
                        TT::And,  TT::Or
                    }),
                    AT::Expr
                },
                [](SS& symbols, TT token_next) -> ASTNODE {
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
                PATS_INIT{
                    SymbolPattern({ TT::Minus, TT::Not }),
                    AT::Expr
                },
                [](SS& symbols, auto) {
                    return std::make_unique<OperExpr>(
                        TokenType2OperType(Rule::GetTokenType(symbols, 1), true),
                        Rule::Move<Expr>(symbols, 2),
                        nullptr
                    );
                },
                PATS_INIT{ AT::Expr }
            );
        }
        
        // Array

        // └─ [exprs?] -> arrayexpr
        {
            RuleAdd(
                PATS_INIT{
                    TT::LBkt,
                    SymbolPattern::Opt({ AT::Expr, AT::Exprs}),
                    TT::RBkt
                },
                [](SS& symbols, auto) {
                    return std::make_unique<ArrayExpr>(
                        Pack2Exprs(symbols, 2)
                    );
                }
            );
        }

        // Range
 
        // └─ expr..expr..expr -> rangeexpr
        {
            RuleAdd(
                PATS_INIT{
                    AT::Expr,
                    TT::DotDot,
                    AT::Expr,
                    TT::DotDot,
                    AT::Expr,
                },
                [](SS& symbols, auto) -> ASTNODE {                    
                    return std::make_unique<RangeExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 5),
                        Rule::Move<Expr>(symbols, 3),
                        false
                    );
                },
                {}, {},
                TTS_INIT{
                    TT::DotDot,
                    TT::DotDotEq,
                    TT::LBkt,
                    TT::LParen,
                    TT::Dot
                }
            );
        }
        // └─ expr..expr..=expr -> rangeexpr
        {
            RuleAdd(
                PATS_INIT{
                    AT::Expr,
                    TT::DotDot,
                    AT::Expr,
                    TT::DotDotEq,
                    AT::Expr,
                },
                [](SS& symbols, auto) -> ASTNODE {
                    return std::make_unique<RangeExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 5),
                        Rule::Move<Expr>(symbols, 3),
                        true
                    );
                },
                {}, {},
                TTS_INIT{
                    TT::DotDot,
                    TT::DotDotEq,
                    TT::LBkt,
                    TT::LParen,
                    TT::Dot
                }
            );
        }

        // └─ expr..expr -> rangeexpr
        {
            RuleAdd(
                PATS_INIT{
                    AT::Expr,
                    TT::DotDot,
                    AT::Expr,
                },
                [](SS& symbols, auto) -> ASTNODE {
                    return std::make_unique<RangeExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3),
                        nullptr,
                        false
                    );
                },
                {}, {},
                TTS_INIT{
                    TT::DotDot,
                    TT::DotDotEq,
                    TT::LBkt,
                    TT::LParen,
                    TT::Dot
                }
            );
        }
        // └─ expr..=expr -> rangeexpr
        {
            RuleAdd(
                PATS_INIT{
                    AT::Expr,
                    TT::DotDotEq,
                    AT::Expr,
                },
                [](SS& symbols, auto) -> ASTNODE {
                    return std::make_unique<RangeExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3),
                        nullptr,
                        true
                    );
                },
                {}, {},
                TTS_INIT{
                    TT::DotDot,
                    TT::DotDotEq,
                    TT::LBkt,
                    TT::LParen,
                    TT::Dot
                }
            );
        }

        // Logic Stmt

        // └─ if (cond) stmt/blockexpr -> condstmt
        {
            RuleAdd(
                PATS_INIT{
                    TT::If,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    { AT::Stmt, AT::BlockExpr }
                },
                [](SS& symbols, auto) {
                    return std::make_unique<CondStmt>(
                        Rule::Move<Expr>(symbols, 3),
                        Pack2BlockExpr(Rule::Move<AstNode>(symbols, 5)),
                        nullptr
                    );
                },
                {}, {},
                TTS_INIT{ TT::Elif, TT::Else }
            );
        }
        // └─ if (expr) stmt elif (expr) stmt -> condstmt
        {
            RuleAdd(
                PATS_INIT{
                    TT::If,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    { AT::Stmt, AT::BlockExpr },
                    TT::Elif,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    { AT::Stmt, AT::BlockExpr }
                },
                [](SS& symbols, auto) {
                    return std::make_unique<CondStmt>(
                        Rule::Move<Expr>(symbols, 3),
                        Pack2BlockExpr(Rule::Move<AstNode>(symbols, 5)),
                        std::make_unique<CondStmt>(
                            Rule::Move<Expr>(symbols, 8),
                            Pack2BlockExpr(Rule::Move<AstNode>(symbols, 10)),
                            nullptr
                        )
                    );
                }
            );
        }
        // └─ if (expr) stmt else stmt -> condstmt
        {
            RuleAdd(
                PATS_INIT{
                    TT::If,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    { AT::Stmt, AT::BlockExpr },
                    TT::Else,
                    { AT::Stmt, AT::BlockExpr }
                },
                [](SS& symbols, auto) {
                    return std::make_unique<CondStmt>(
                        Rule::Move<Expr>(symbols, 3),
                        Pack2BlockExpr(Rule::Move<AstNode>(symbols, 5)),
                        std::make_unique<CondStmt>(
                            nullptr,
                            Pack2BlockExpr(Rule::Move<AstNode>(symbols, 7)),
                            nullptr
                        )
                    );
                }
            );
        }
        // └─ condstmt elif (cond) stmt -> condstmt
        {
            RuleAdd(
                PATS_INIT{
                    AT::CondStmt,
                    TT::Elif,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    { AT::Stmt, AT::BlockExpr }
                },
                [](SS& symbols, auto) {
                    auto stmt = Rule::Move<CondStmt>(symbols, 1);

                    CondStmt* tail = stmt.get();
                    while (tail->sub_) tail = tail->sub_.get();

                    tail->sub_ = std::make_unique<CondStmt>(
                        Rule::Move<Expr>(symbols, 4),
                        Pack2BlockExpr(Rule::Move<AstNode>(symbols, 6)),
                        nullptr
                    );
                    return stmt;
                }
            );
        }

        // └─ condstmt else stmt -> condstmt
        {
            RuleAdd(
                PATS_INIT{
                    AT::CondStmt,
                    TT::Else,
                    { AT::Stmt, AT::BlockExpr }
                },
                [](SS& symbols, auto) {
                    auto stmt = Rule::Move<CondStmt>(symbols, 1);

                    CondStmt* tail = stmt.get();
                    while (tail->sub_) tail = tail->sub_.get();

                    tail->sub_ = std::make_unique<CondStmt>(
                        nullptr,
                        Pack2BlockExpr(Rule::Move<AstNode>(symbols, 3)),
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
                PATS_INIT{
                    TT::Break,
                    TT::Semicolon
                },
                [](auto&, auto) {
                    return std::make_unique<LoopSignalStmt>(
                        LoopSignal::Break
                    );
                }
            );
        }
        // └─ continue; -> loopsignalstmt
        {
            RuleAdd(
                PATS_INIT{
                    TT::Continue,
                    TT::Semicolon
                },
                [](auto&, auto) {
                    return std::make_unique<LoopSignalStmt>(
                        LoopSignal::Continue
                    );
                }
            );
        }
        // └─ for ( x in expr ) stmt -> forstmt
        {
            RuleAdd(
                PATS_INIT{
                    TT::For,
                    TT::LParen,
                    AT::IdExpr,
                    TT::In,
                    AT::Expr,
                    TT::RParen,
                    { AT::Stmt, AT::BlockExpr }
                },
                [](SS& symbols, auto) {
                    return std::make_unique<ForStmt>(
                        Rule::Move<IdExpr>(symbols, 3),
                        Rule::Move<Expr>(symbols, 5),
                        Pack2BlockExpr(Rule::Move<AstNode>(symbols, 7))
                    );
                }
            );
        }
        // └─ while ( cond ) stmt -> whilestmt
        {
            RuleAdd(
                PATS_INIT{
                    TT::While,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen,
                    { AT::Stmt, AT::BlockExpr }
                },
                [](SS& symbols, auto) {
                    return std::make_unique<WhileStmt>(
                        Rule::Move<Expr>(symbols, 3),
                        Pack2BlockExpr(Rule::Move<AstNode>(symbols, 5))
                    );
                }
            );
        }
    
        // Stmt

        // └─ expr; -> exprstmt
        {
            RuleAdd(
                PATS_INIT{
                    AT::Expr,
                    TT::Semicolon
                },
                [](SS& symbols, auto) {
                    return std::make_unique<ExprStmt>(
                        Rule::Move<Expr>(symbols, 1)
                    );
                }
            );
        }
    }
}

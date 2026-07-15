
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

        // Tools

        static auto isArgTerminator = [](const Token* next) {
            if (!next) return true;                             // EOF
            return  next->type == Token::Type::Comma ||         // ,
                    next->type == Token::Type::Semicolon ||     // ;
                    next->type == Token::Type::RParen;          // )
        };

        rules_.clear();
        
        // Var and Const
        
        // └─ i: int = 7;
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
        // └─ i = 3;
        {
            RuleAdd(
                PATS{
                    AT::IdExpr,
                    TT::Assign,
                    AT::Expr,
                    TT::Semicolon
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    return std::make_unique<AssignStmt>(
                        Rule::Move<IdExpr>(symbols, 1),
                        Rule::Move<Expr>(symbols, 3)
                    );
                }
            );
        }

        // Call

        // └─ func(); -> func()
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
        // └─ expr, expr -> arglist
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::Comma,
                    AT::Expr
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {
                    if (!isArgTerminator(token_next)) return nullptr;

                    std::vector<std::unique_ptr<Expr>> args;
                    args.emplace_back(Rule::Move<Expr>(symbols, 1));
                    args.emplace_back(Rule::Move<Expr>(symbols, 3));
                    return std::make_unique<ArgList>(args);
                }
            );
        }
        // └─ arglist , expr -> arglist
        {
            RuleAdd(
                PATS{
                    AT::ArgList,
                    TT::Comma,
                    AT::Expr
                },
                [](std::vector<Symbol>& symbols, const Token* token_next) -> ASTNODE {
                    if (!isArgTerminator(token_next)) return nullptr;
                    
                    auto arglist = Rule::Move<ArgList>(symbols, 1);
                    auto expr    = Rule::Move<Expr>(symbols, 3);
                    arglist->args().emplace_back(std::move(expr));
                    return arglist;
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
                        std::make_unique<ArgList>(args)
                    );
                }
            );
        }
        // └─ target.expr(arglist?) -> methodcallexpr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::Dot,
                    AT::IdExpr,
                    TT::LParen,
                    SymbolPattern::Opt(AT::ArgList),
                    TT::RParen
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    std::vector<std::unique_ptr<Expr>> args;

                    // Option [ArgList] Check
                    if (Rule::move_positions_[4] != 0) {
                        auto arglist = Rule::Move<ArgList>(symbols, 5);
                        for (auto& arg : arglist->args())
                            args.emplace_back(std::move(arg));
                    }

                    return std::make_unique<MethodCallExpr>(
                        Rule::Move<Expr>(symbols, 1),
                        Rule::Move<IdExpr>(symbols, 3),
                        std::make_unique<ArgList>(args)
                    );
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
                        std::make_unique<ArgList>(args)
                    );
                }
            );
        }
        // └─ expr(arglist?) -> fncallexpr
        {
            RuleAdd(
                PATS{
                    AT::IdExpr,
                    TT::LParen,
                    SymbolPattern::Opt(AT::ArgList),
                    TT::RParen
                },
                [](std::vector<Symbol>& symbols, auto*) {
                    std::vector<std::unique_ptr<Expr>> args;

                    // Option [ArgList] Check
                    if (Rule::move_positions_[2] != 0) {
                        auto arglist = Rule::Move<ArgList>(symbols, 3);
                        for (auto& arg : arglist->args())
                            args.emplace_back(std::move(arg));
                    }

                    return std::make_unique<FnCallExpr>(
                        Rule::Move<IdExpr>(symbols, 1),
                        std::make_unique<ArgList>(args)
                    );
                }
            );
        }

        // Operation

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
                    auto   opertype = std::get<Token>(symbols[syms_len - 2].data()).type;
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
                    auto opertype = std::get<Token>(symbols[syms_len - 2].data()).type;
                    if (token_next && isOperPriority(token_next->type, opertype))
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
                    if (token_next && token_next->type == Token::Type::LBrace)
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
    
        // Logic

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
    }
}

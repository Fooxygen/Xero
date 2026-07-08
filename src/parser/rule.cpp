
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "parser.hpp"

namespace parser {

    void Parser::RulesInit() {
        using TT   = Token::Type;
        using AT   = AstType;
        using PATS = std::initializer_list<SymbolPattern>;

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
                [](std::vector<Symbol>& symbols, auto*)
                    -> std::unique_ptr<AstNode>
                {
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
                [](std::vector<Symbol>& symbols, auto*)
                    -> std::unique_ptr<AstNode>
                {
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
                    AT::CallExpr,
                    TT::Semicolon
                },
                [](auto& symbols, auto*)
                    -> std::unique_ptr<AstNode>
                {
                    return Rule::Move<CallExpr>(symbols, 1);
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
                [](std::vector<Symbol>& symbols, auto*)
                    -> std::unique_ptr<AstNode>
                {
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
                [](std::vector<Symbol>& symbols, auto*)
                    -> std::unique_ptr<AstNode>
                {
                    auto arglist = Rule::Move<ArgList>(symbols, 1);
                    auto expr    = Rule::Move<Expr>(symbols, 3);
                    arglist->args().emplace_back(std::move(expr));
                    return arglist;
                }
            );
        }
        // └─ expr(expr) -> callexpr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::LParen,
                    AT::Expr,
                    TT::RParen
                },
                [](std::vector<Symbol>& symbols, auto*)
                    -> std::unique_ptr<AstNode>
                {
                    std::vector<std::unique_ptr<Expr>> args;
                    args.emplace_back(
                        Rule::Move<Expr>(symbols, 3)
                    );

                    return std::make_unique<CallExpr>(
                        std::make_unique<ArgList>(args)
                    );
                }
            );
        }
        // └─ expr(arglist?) -> callexpr
        {
            RuleAdd(
                PATS{
                    AT::Expr,
                    TT::LParen,
                    SymbolPattern::Opt(AT::ArgList),
                    TT::RParen
                },
                [](std::vector<Symbol>& symbols, auto*)
                    -> std::unique_ptr<AstNode>
                {
                    //auto callee = Rule::Move<Expr>(symbols, 1);
                    std::vector<std::unique_ptr<Expr>> args;

                    // Option [ArgList] Check
                    if (Rule::move_positions_[2] != 0) {
                        auto arglist = Rule::Move<ArgList>(symbols, 3);
                        for (auto& arg : arglist->args())
                            args.push_back(std::move(arg));
                    }

                    return std::make_unique<CallExpr>(
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
                [](std::vector<Symbol>& symbols, const Token* token_next)
                    -> std::unique_ptr<AstNode>
                {
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
                [](std::vector<Symbol>& symbols, const Token* token_next)
                    -> std::unique_ptr<AstNode>
                {
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
                [](std::vector<Symbol>& symbols, auto*)
                    -> std::unique_ptr<AstNode>
                {
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
                [](std::vector<Symbol>& symbols, auto*)
                    -> std::unique_ptr<AstNode>
                {
                    return std::make_unique<NegExpr>(
                        Rule::Move<Expr>(symbols, 2)
                    );
                }
            );
        }
    }
}

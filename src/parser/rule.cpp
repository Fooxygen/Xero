
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
        
        // Definition
        
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
                [](std::vector<Symbol>& symbols, const Token* token_next)
                    -> std::unique_ptr<AstNode>
                {
                    return std::make_unique<DeclStmt>(
                        Rule::Move<IdExpr>(symbols, 6),
                        Rule::Move<Expr>(symbols, 2),
                        Rule::Move<IdExpr>(symbols, 4)
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
                    auto   opertype = symbols[syms_len - 2].token().type;
                    return std::make_unique<OperExpr>(
                        opertype,
                        Rule::Move<Expr>(symbols, 3),
                        Rule::Move<Expr>(symbols, 1)
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
                    auto opertype = symbols[syms_len - 2].token().type;
                    if (token_next && isOperPriority(token_next->type, opertype))
                    return nullptr;     // postpone

                    return std::make_unique<OperExpr>(
                        opertype,
                        Rule::Move<Expr>(symbols, 3),
                        Rule::Move<Expr>(symbols, 1)
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
                [](std::vector<Symbol>& symbols, const Token* token_next)
                    -> std::unique_ptr<AstNode>
                {
                    return Rule::Move<Expr>(symbols, 2);
                }
            );
        }
        // └─ - expr -> negexpr
        {
            RuleAdd(
                PATS{
                    TT::Minus,
                    AT::Expr,
                },
                [](std::vector<Symbol>& symbols, const Token* token_next)
                    -> std::unique_ptr<AstNode>
                {
                    return std::make_unique<NegExpr>(
                        Rule::Move<Expr>(symbols, 1)
                    );
                }
            );
        }
    }
}

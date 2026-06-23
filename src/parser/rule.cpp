
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "parser.hpp"

namespace parser {

    void   Parser::RulesInit() {
        rules_.clear();
        
        // Definition
        
        // └─ i: int = 7;
        {
            auto patterns = std::initializer_list<SymbolPattern>{
                SymbolPattern(AstType::IdExpr),
                SymbolPattern(Token::Type::Colon),
                SymbolPattern(AstType::IdExpr),
                SymbolPattern(Token::Type::Assign),
                SymbolPattern(AstType::Expr),
                SymbolPattern(Token::Type::Semicolon)
            };
            rules_.emplace_back(
                Rule(patterns,
                    [](std::vector<Symbol>& symbols, const Token* token_next)
                    -> std::unique_ptr<AstNode>
                {
                    size_t syms_len = symbols.size();
                    auto id         = symbols[syms_len - 6].astnode().release();
                    auto value_type = symbols[syms_len - 4].astnode().release();
                    auto value      = symbols[syms_len - 2].astnode().release();
                    
                    return std::make_unique<DeclStmt>(
                        std::unique_ptr<IdExpr>(static_cast<IdExpr*>(id)),
                        std::unique_ptr<Expr>(static_cast<Expr*>(value)),
                        std::unique_ptr<IdExpr>(static_cast<IdExpr*>(value_type))
                    );
                })
            );
        }

        // Operation

        // └─ expr *|/ expr
        {
            auto patterns = std::initializer_list<SymbolPattern>{
                SymbolPattern(AstType::Expr),
                SymbolPattern(Token::Type::StarOrSlash),
                SymbolPattern(AstType::Expr),
            };
            rules_.emplace_back(
                Rule(patterns,
                    [](std::vector<Symbol>& symbols, const Token* token_next)
                    -> std::unique_ptr<AstNode>
                {
                    size_t syms_len = symbols.size();
                    auto opertype   = symbols[syms_len - 2].token().type;
                    auto lexpr      = symbols[syms_len - 3].astnode().release();
                    auto rexpr      = symbols[syms_len - 1].astnode().release();
                    
                    return std::make_unique<OperExpr>(
                        opertype,
                        std::unique_ptr<Expr>(static_cast<Expr*>(lexpr)),
                        std::unique_ptr<Expr>(static_cast<Expr*>(rexpr))
                    );
                })
            );
        }
        // └─ expr +|- expr
        {
            auto patterns = std::initializer_list<SymbolPattern>{
                SymbolPattern(AstType::Expr),
                SymbolPattern(Token::Type::PlusOrMinus),
                SymbolPattern(AstType::Expr),
            };
            rules_.emplace_back(
                Rule(patterns,
                    [](std::vector<Symbol>& symbols, const Token* token_next)
                    -> std::unique_ptr<AstNode>
                {
                    size_t syms_len = symbols.size();

                    // Oper Priority Check
                    auto opertype = symbols[syms_len - 2].token().type;
                    if (token_next && isOperPriority(token_next->type, opertype))
                    return nullptr;     // postpone

                    auto lexpr    = symbols[syms_len - 3].astnode().release();
                    auto rexpr    = symbols[syms_len - 1].astnode().release();
                    
                    return std::make_unique<OperExpr>(
                        opertype,
                        std::unique_ptr<Expr>(static_cast<Expr*>(lexpr)),
                        std::unique_ptr<Expr>(static_cast<Expr*>(rexpr))
                    );
                })
            );
        }
        // └─ (expr) -> expr
        {
            auto patterns = std::initializer_list<SymbolPattern>{
                SymbolPattern(Token::Type::LParen),
                SymbolPattern(AstType::Expr),
                SymbolPattern(Token::Type::RParen),
            };
            rules_.emplace_back(
                Rule(patterns,
                    [](std::vector<Symbol>& symbols, const Token* token_next)
                    -> std::unique_ptr<AstNode>
                {
                    size_t syms_len = symbols.size();              
                    return std::move(symbols[syms_len - 2].astnode());
                })
            );
        }
        // └─ - expr -> negexpr
        {
            auto patterns = std::initializer_list<SymbolPattern>{
                SymbolPattern(Token::Type::Minus),
                SymbolPattern(AstType::Expr),
            };
            rules_.emplace_back(
                Rule(patterns,
                    [](std::vector<Symbol>& symbols, const Token* token_next)
                    -> std::unique_ptr<AstNode>
                {
                    size_t syms_len = symbols.size();
                    auto expr = symbols[syms_len - 1].astnode().release();
                    
                    return std::make_unique<NegExpr>(
                        std::unique_ptr<Expr>(static_cast<Expr*>(expr))
                    );
                })
            );
        }
        
    }
}

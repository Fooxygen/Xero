
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>
#include <functional>

#include "common/token.hpp"
#include "common/ast.hpp"

namespace parser {

    static bool isOperPriority(Token::Type a, Token::Type b) {
        auto group = [](Token::Type type) {
            switch (type) {
                case Token::Type::Star:
                case Token::Type::Slash:
                case Token::Type::StarOrSlash:
                    return 2;
                case Token::Type::Plus:
                case Token::Type::Minus:
                case Token::Type::PlusOrMinus:
                    return 1;
                default:
                    return 0;
            }
        };
        return group(a) > group(b);
    }

    class Symbol {
    public:
        enum class Type {
            Undefined, Token, AstNode
        };

    private:
        Type type_ = Type::Undefined;

        Token token_;
        std::unique_ptr<AstNode> astnode_;

    public:
        Symbol(const Token& token) {
            type_    = Type::Token;
            token_   = token;
            astnode_ = nullptr;
        }
        Symbol(std::unique_ptr<AstNode> node) {
            type_    = Type::AstNode;
            astnode_ = std::move(node);
        }

        Type      type() const {
            return type_;
        }
        Token::Type type_token() const {
            return token_.type;
        }
        AstType   type_astnode() const {
            if (!astnode_) return AstType::Undefined;
            return astnode_->type_;
        }
        
        Token& token() {
            return token_;
        }
        std::unique_ptr<AstNode>& astnode() {
            return astnode_;
        }
    };

    class SymbolPattern {
    private:
        Symbol::Type type_ = Symbol::Type::Undefined;

        Token::Type type_token_ = Token::Type::Undefined;
        AstType   type_astnode_ = AstType::Undefined;

    public:
        SymbolPattern(Token::Type type) {
            type_ = Symbol::Type::Token;
            type_token_ = type;
        }
        SymbolPattern(AstType type) {
            type_ = Symbol::Type::AstNode;
            type_astnode_ = type;
        }

        Symbol::Type type() const {
            return type_;
        }
        Token::Type  type_token() const {
            return type_token_;
        }
        AstType      type_astnode() const {
            return type_astnode_;
        }
    };

    // Parsing Rule
    // [TokenType, Token, AstType, ....] -> AstType
    class Rule {
    private:
        using ReduceCallback = std::function<
            std::unique_ptr<AstNode>(
                std::vector<Symbol>& symbols,
                const Token* token_next
            )
        >;

        std::vector<SymbolPattern> patterns_;
        ReduceCallback             reduce_callback_;

    public:
        Rule(
            std::initializer_list<SymbolPattern> patterns,
            ReduceCallback reduce_callback)
        {
            patterns_.insert(patterns_.end(), patterns.begin(), patterns.end());
            reduce_callback_ = std::move(reduce_callback);
        }

        const ReduceCallback& reduce_callback() const {
            return reduce_callback_;
        }
        const std::vector<SymbolPattern>& patterns() const {
            return patterns_;
        }

        bool  PatternMatch(const std::vector<Symbol>& symbols) const {
            size_t len = patterns_.size();
            if (symbols.size() < patterns_.size()) return false;
            
            for (size_t i = 0; i < len; i++) {
                auto& pat = patterns_[i];
                auto& sym = symbols[symbols.size() - len + i];

                // Token
                if (pat.type() == Symbol::Type::Token) {
                    if (sym.type() != Symbol::Type::Token ||
                        !Token::isTypeCompatible(pat.type_token(), sym.type_token())
                    ) return false;
                }

                // AstNode
                else {
                    if (sym.type() != Symbol::Type::AstNode ||
                        !isAstTypeCompatible(pat.type_astnode(), sym.type_astnode())
                    ) return false;
                }
            }

            return true;
        }
    };

    // Syntactic Analyzer
    class Parser {
    private:
        // Predefined
        inline static std::vector<Rule> rules_;     // Reduce Rules
        const std::vector<Token>&       tokens_;    // Lexer's Tokens

        // Cache
        std::vector<Symbol>             symbols_;   // Symbols Stack
        std::unique_ptr<AstNode>        root_;      // Program as AST Root Node.

        void RulesInit();

        void Shift(const Token& token);
        bool TryReduce(const Rule& rule, const Token* token_next);

        Symbol Token2Symbol(const Token& token);

    public:
        Parser(const std::vector<Token>& tokens) : tokens_(tokens) {
            RulesInit();
        }

        void Execute();
        std::unique_ptr<AstNode>& root() {
            return root_;
        }
    };
}

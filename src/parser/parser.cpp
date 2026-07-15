
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "parser.hpp"

namespace parser {

    void   Parser::Execute() {
        symbols_.clear();
        scopes_brace.clear();

        // Symbols
        for (size_t i = 0; i < tokens_.size(); i++) {
            const auto& token = tokens_[i];
            const auto* token_next =
                i + 1 >= tokens_.size() ? nullptr : &(tokens_[i + 1]);

            // Shift
            Shift(token);

            // Try Reduce
            bool isNeedTryAgain = false;  // set true only when at least one match
            do {
                isNeedTryAgain = false;
                for (auto& rule : rules_) {

                    // Match
                    size_t len = 0;
                    if (!rule.PatternsMatch(symbols_, len))
                        continue;
                
                    // Execute
                    if (TryReduce(rule, token_next, len)) {
                        isNeedTryAgain = true;
                        break;
                    }
                }
            } while (isNeedTryAgain);
        }

        // Program (Root Node)
        std::vector<std::unique_ptr<AstNode>> program_children;
        for (auto& sym : symbols_) {
            program_children.emplace_back(std::move(
                std::get<std::unique_ptr<AstNode>>(sym.data())
            ));
        }
        root_ = std::make_unique<Program>(program_children);
    }

    Symbol Parser::Token2Symbol(const Token& token) {
        using TT = Token::Type;

        auto sym = Symbol(token);
        switch (token.type()) {
            case TT::Id: {
                if      (token.lexeme() == "true") {
                    sym = Symbol(std::make_unique<BoolConst>(true));
                }
                else if (token.lexeme() == "false") {
                    sym = Symbol(std::make_unique<BoolConst>(false));
                }
                
                else if (token.lexeme() == "if") {
                    sym = Symbol(Token(Token::Type::If, token.lexeme(), token.line(), token.col()));
                }
                else if (token.lexeme() == "elif") {
                    sym = Symbol(Token(Token::Type::Elif, token.lexeme(), token.line(), token.col()));
                }
                else if (token.lexeme() == "else") {
                    sym = Symbol(Token(Token::Type::Else, token.lexeme(), token.line(), token.col()));
                }

                else {
                    sym = Symbol(std::make_unique<IdExpr>(token.lexeme()));
                }
                break;
            }
            case TT::Number: {
                sym = Symbol(std::make_unique<NumConst>(token.lexeme()));
                break;
            }
            case TT::String: {
                sym = Symbol(std::make_unique<StringConst>(token.lexeme()));
                break;
            }
        }

        return sym;
    }

    void   Parser::Shift(const Token& token) {
        using TT = Token::Type;

        // Brace Scope
        {
            if      (token.type() == TT::LBrace) {
                scopes_brace.emplace_back(symbols_.size() + 1);
            }
            else if (token.type() == TT::RBrace) {
                if (scopes_brace.empty()) {
                    throw LogErr(LogModule::Parser, "unclosed brace");
                }

                size_t pbeg = scopes_brace.back();
                size_t pend = symbols_.size() - 1;
                scopes_brace.pop_back();

                std::vector<std::unique_ptr<AstNode>> children;
                for (size_t i = 0; i < pend - pbeg + 1; i++) {
                    auto& data = symbols_.back().data();
                    children.emplace_back(
                        std::move(std::get<std::unique_ptr<AstNode>>(data))
                    );
                    symbols_.pop_back();
                }

                std::reverse(children.begin(), children.end());
                symbols_.pop_back();                                // erase '{'
                symbols_.emplace_back(
                    std::make_unique<BlockStmt>(children)
                );

                return;
            }
        }

        symbols_.emplace_back(Token2Symbol(token));
    }

    bool   Parser::TryReduce(const Rule& rule, const Token* token_next, size_t reduce_len) {

        // Target
        auto target = rule.reduce_callback()(symbols_, token_next);
        if (!target) return false;

        // Remove
        symbols_.erase(symbols_.end() - reduce_len, symbols_.end());

        // Push
        symbols_.emplace_back(std::move(target));

        return true;
    }
}

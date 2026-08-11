
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "parser.hpp"

namespace parser {

    void   Parser::Execute() {
        symbols_.clear();
        scopes_brace.clear();

        // Token Rewrite
        for (size_t i = 0; i < tokens_.size(); i++) {
            TokenRewrite(tokens_[i]);
        }

        // Symbols
        for (size_t i = 0; i < tokens_.size(); i++) {
            const auto& token = tokens_[i];
            const auto  token_next =
                i + 1 >= tokens_.size()
                ? Token::Type::Undefined
                : (tokens_[i + 1]).type();

            // Shift
            Shift(token);

            // Try Reduce
            bool isNeedTryAgain = false;  // set true only when at least one match
            do {
                isNeedTryAgain = false;
                for (auto& rule : rules_) {

                    // Match
                    size_t len = 0;
                    if (!rule.PatternsMatch(symbols_, token_next, len))
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
        {
            // When there are symbols that cannot be reduced to statements
            size_t independent_sym_cnt = 0;
            std::vector<std::unique_ptr<AstNode>> program_children;

            for (auto& sym : symbols_) {
                if (sym.type() != Symbol::Type::AstNode) {
                    independent_sym_cnt++;
                    continue;
                }
                if (independent_sym_cnt == 0) {
                    program_children.emplace_back(std::move(
                        std::get<std::unique_ptr<AstNode>>(sym.data())
                    ));
                }
            }

            if (independent_sym_cnt > 0) {
                throw LogErr(LogModule::Parser, std::format(
                    "reduce failed with '{}' symbols unprocessed", independent_sym_cnt
                ));
            }

            root_ = std::make_unique<Program>(program_children);
        }
    }

    void   Parser::TokenRewrite(Token& token) {
        using TT = Token::Type;

        switch (token.type()) {
            case TT::Id: {
                if      (token.lexeme() == "true") {
                    token = Token(TT::True, token.lexeme(), token.line(), token.col());
                }
                else if (token.lexeme() == "false") {
                    token = Token(TT::False, token.lexeme(), token.line(), token.col());
                }

                else if (token.lexeme() == "if") {
                    token = Token(TT::If, token.lexeme(), token.line(), token.col());
                }
                else if (token.lexeme() == "elif") {
                    token = Token(TT::Elif, token.lexeme(), token.line(), token.col());
                }
                else if (token.lexeme() == "else") {
                    token = Token(TT::Else, token.lexeme(), token.line(), token.col());
                }
                
                else if (token.lexeme() == "in") {
                    token = Token(TT::In, token.lexeme(), token.line(), token.col());
                }
                else if (token.lexeme() == "for") {
                    token = Token(TT::For, token.lexeme(), token.line(), token.col());
                }
                else if (token.lexeme() == "while") {
                    token = Token(TT::While, token.lexeme(), token.line(), token.col());
                }
                else if (token.lexeme() == "break") {
                    token = Token(TT::Break, token.lexeme(), token.line(), token.col());
                }
                else if (token.lexeme() == "continue") {
                    token = Token(TT::Continue, token.lexeme(), token.line(), token.col());
                }
                else if (token.lexeme() == "return") {
                    token = Token(TT::Return, token.lexeme(), token.line(), token.col());
                }

                else if (token.lexeme() == "fn") {
                    token = Token(TT::Fn, token.lexeme(), token.line(), token.col());
                }

                break;
            }
            default: break;
        }
    }

    Symbol Parser::Token2Symbol(const Token& token) {
        using TT = Token::Type;

        auto sym = Symbol(token);
        switch (token.type()) {
            case TT::Id: {
                sym = Symbol(std::make_unique<IdExpr>(token.lexeme()));
                break;
            }
            case TT::True: {
                sym = Symbol(std::make_unique<BoolConst>(true));
                break;
            }
            case TT::False: {
                sym = Symbol(std::make_unique<BoolConst>(false));
                break;
            }
            case TT::Number: {
                sym = Symbol(std::make_unique<NumConst>(token.lexeme()));
                break;
            }
            case TT::Char: {
                sym = Symbol(std::make_unique<CharConst>(token.lexeme()));
                break;
            }
            case TT::String: {
                sym = Symbol(std::make_unique<StringConst>(token.lexeme()));
                break;
            }
            default: break;
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
                    std::make_unique<BlockExpr>(children)
                );

                return;
            }
        }

        symbols_.emplace_back(Token2Symbol(token));
    }

    bool   Parser::TryReduce(const Rule& rule, Token::Type token_next, size_t reduce_len) {

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

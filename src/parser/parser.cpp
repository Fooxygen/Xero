
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "parser.hpp"

namespace parser {

    void   Parser::Execute() {
        symbols_.clear();
        scopes_brace_.clear();

        // Token Rewrite
        for (size_t i = 0; i < tokens_.size(); i++) {
            TokenRewrite(tokens_[i]);
        }

        // Symbols
        for (size_t i = 0; i < tokens_.size(); i++) {
            const auto& token = tokens_[i];
            const auto  token_next =
                i + 1 >= tokens_.size()
                ? TT::Undefined
                : (tokens_[i + 1]).type_;

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
        using enum Token::Type;

        switch (token.type_) {
            case Id: {
                if      (token.lexeme_ == "true") {
                    token = Token(True, token.lexeme_, token.loc_);
                }
                else if (token.lexeme_ == "false") {
                    token = Token(False, token.lexeme_, token.loc_);
                }

                else if (token.lexeme_ == "if") {
                    token = Token(If, token.lexeme_, token.loc_);
                }
                else if (token.lexeme_ == "elif") {
                    token = Token(Elif, token.lexeme_, token.loc_);
                }
                else if (token.lexeme_ == "else") {
                    token = Token(Else, token.lexeme_, token.loc_);
                }
                
                else if (token.lexeme_ == "in") {
                    token = Token(In, token.lexeme_, token.loc_);
                }
                else if (token.lexeme_ == "for") {
                    token = Token(For, token.lexeme_, token.loc_);
                }
                else if (token.lexeme_ == "while") {
                    token = Token(While, token.lexeme_, token.loc_);
                }
                else if (token.lexeme_ == "break") {
                    token = Token(Break, token.lexeme_, token.loc_);
                }
                else if (token.lexeme_ == "continue") {
                    token = Token(Continue, token.lexeme_, token.loc_);
                }
                else if (token.lexeme_ == "return") {
                    token = Token(Return, token.lexeme_, token.loc_);
                }

                else if (token.lexeme_ == "fn") {
                    token = Token(Fn, token.lexeme_, token.loc_);
                }

                break;
            }
            default: break;
        }
    }

    Symbol Parser::Token2Symbol(const Token& token) {
        using enum Token::Type;

        auto sym = Symbol(token);
        switch (token.type_) {
            case Id: {
                sym = Symbol(std::make_unique<IdExpr>(token.lexeme_), token.loc_);
                break;
            }
            case True: {
                sym = Symbol(std::make_unique<BoolConst>(true), token.loc_);
                break;
            }
            case False: {
                sym = Symbol(std::make_unique<BoolConst>(false), token.loc_);
                break;
            }
            case Number: {
                sym = Symbol(std::make_unique<NumConst>(token.lexeme_), token.loc_);
                break;
            }
            case Char: {
                sym = Symbol(std::make_unique<CharConst>(token.lexeme_), token.loc_);
                break;
            }
            case String: {
                sym = Symbol(std::make_unique<StringConst>(token.lexeme_), token.loc_);
                break;
            }
            default: break;
        }

        return sym;
    }

    void   Parser::Shift(const Token& token) {
        using enum Token::Type;

        // Brace Scope
        {
            if      (token.type_ == LBrace) {
                scopes_brace_.emplace_back(symbols_.size() + 1);
            }
            else if (token.type_ == RBrace) {
                if (scopes_brace_.empty()) {
                    throw LogErr(LogModule::Parser, "unclosed brace", token.loc_);
                }

                size_t pbeg = scopes_brace_.back();
                size_t pend = symbols_.size() - 1;
                scopes_brace_.pop_back();

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
                    std::make_unique<BlockExpr>(children), token.loc_
                );

                return;
            }
        }

        symbols_.emplace_back(Token2Symbol(token));
    }

    bool   Parser::TryReduce(const Rule& rule, TT token_next, size_t reduce_len) {

        // Loc
        auto loc = symbols_[symbols_.size() - reduce_len].loc();

        // Result
        auto target = rule.reduce_callback()(symbols_, token_next);
        if (!target) return false;

        // Remove
        symbols_.erase(symbols_.end() - reduce_len, symbols_.end());

        // Push
        symbols_.emplace_back(std::move(target), loc);

        return true;
    }
}

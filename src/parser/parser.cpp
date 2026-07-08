
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "parser.hpp"

namespace parser {

    void   Parser::Execute() {
        symbols_.clear();

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
            } while(isNeedTryAgain);
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
        auto sym = Symbol(token);

        // conversion of Semantics Tokens
        // do not handle Unsemantics tokens
        switch (token.type) {
            case Token::Type::Id: {
                sym = Symbol(std::make_unique<IdExpr>(token.lexeme));
                break;
            }
            case Token::Type::Number: {
                sym = Symbol(std::make_unique<NumConst>(token.lexeme));
                break;
            }
        }

        return sym;
    }

    void   Parser::Shift(const Token& token) {
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

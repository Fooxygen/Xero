
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
                    if (!rule.PatternMatch(symbols_))
                        continue;
                
                    // Execute
                    if (TryReduce(rule, token_next)) {
                        isNeedTryAgain = true;
                        break;
                    }
                }
            } while(isNeedTryAgain);
        }

        // Program (Root Node)
        std::vector<std::unique_ptr<AstNode>> program_children;
        for (auto& sym : symbols_) {
            program_children.emplace_back(std::move(sym.astnode()));
        }
        root_ = std::make_unique<Program>(program_children);
    }

    Symbol Parser::Token2Symbol(const Token& token) {
        auto sym = Symbol(token);

        // conversion of Semantics Tokens
        // do not handle Unsemantics tokens
        switch (sym.token().type) {
            case Token::Type::Id: {
                sym = Symbol(std::make_unique<IdExpr>(sym.token().lexeme));
                break;
            }
            case Token::Type::Number: {
                sym = Symbol(std::make_unique<NumConst>(sym.token().lexeme));
                break;
            }
        }

        return sym;
    }

    void   Parser::Shift(const Token& token) {
        symbols_.emplace_back(Token2Symbol(token));
    }

    bool   Parser::TryReduce(const Rule& rule, const Token* token_next) {

        // Target
        auto target = rule.reduce_callback()(symbols_, token_next);
        if (!target) return false;

        // Remove
        size_t len = rule.patterns().size();
        symbols_.erase(symbols_.end() - len, symbols_.end());

        // Push
        symbols_.emplace_back(std::move(target));

        return true;
    }
}

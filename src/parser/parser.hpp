
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>
#include <functional>
#include <variant>
#include <algorithm>

#include "common/token.hpp"
#include "common/ast.hpp"

namespace parser {

    static bool isOperPriority(Token::Type a, Token::Type b) {
        using TT = Token::Type;
        
        auto group = [](Token::Type type) {
            switch (type) {
                case TT::Star:
                case TT::Slash:
                case TT::StarOrSlash:
                    return 4;

                case TT::Plus:
                case TT::Minus:
                case TT::PlusOrMinus:
                    return 3;

                case TT::Gt:
                case TT::Ge:
                case TT::Lt:
                case TT::Le:
                case TT::Eq:
                case TT::Neq:
                case TT::RelationOper:
                    return 2;

                case TT::And:
                    return 1;

                case TT::Or:
                    return 0;

                default:
                    return -1;
            }
        };
        return group(a) > group(b);
    }

    class Symbol {
    public:
        using Data = std::variant<Token, std::unique_ptr<AstNode>>;
        enum class Type {
            Undefined, Token, AstNode
        };

    private:
        Type type_ = Type::Undefined;
        Data data_;

    public:
        Symbol(const Token& token) {
            type_ = Type::Token;
            data_ = token;
        }
        Symbol(std::unique_ptr<AstNode> node) {
            type_ = Type::AstNode;
            data_ = std::move(node);
        }

        Type  type() const {
            return type_;
        }
        Data& data() {
            return data_;
        }
    
        Token::Type type_token() const {
            if (auto token = std::get_if<Token>(&data_)) {
                return token->type();
            }
            return Token::Type::Undefined;
        }
        AstType     type_astnode() const {
            if (auto astnode = std::get_if<std::unique_ptr<AstNode>>(&data_)) {
                return astnode->get()->type_;
            }
            return AstType::Undefined;
        }
    };

    class SymbolPattern {
    private:
        Symbol::Type type_         = Symbol::Type::Undefined;
        Token::Type  type_token_   = Token::Type::Undefined;
        AstType      type_astnode_ = AstType::Undefined;

        bool isOptional_ = false;

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
    
        bool isOptional() const {
            return isOptional_;
        }
    
        // Make Optional SP for TT
        static SymbolPattern Opt(Token::Type type) {
            auto sp = SymbolPattern(type);
            sp.isOptional_ = true;
            return sp;
        }
        // Make Optional SP for AT
        static SymbolPattern Opt(AstType type) {
            auto sp = SymbolPattern(type);
            sp.isOptional_ = true;
            return sp;
        }
    };

    // Parsing Rule
    // [TokenType, Token, AstType, ....] -> AstType
    class Rule {
    public:
        using ReduceCallback = std::function<
            std::unique_ptr<AstNode>(
                std::vector<Symbol>& symbols,
                const Token* token_next
            )
        >;

        // for Move():
        // while rule = AB[C]DE
        //     if pats = ABCDE, mps = [5, 4, 3, 2, 1]
        //     if pats = _ABDE, mps = [4, 3, 0, 2, 1]
        static inline std::vector<size_t> move_positions_;

    private:
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

        bool  PatternMatch(const SymbolPattern& pat, const Symbol& sym) {
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

            return true;
        }
        bool  PatternsMatch(const std::vector<Symbol>& symbols, size_t& out_reduce_len) {

            // Match Check
            size_t np = patterns_.size();
            size_t ns = symbols.size();

            size_t start_max = (ns > np) ? (ns - np) : 0;

            for (size_t start = start_max; start <= ns; start++) {
                
                size_t len = ns - start;
                if (len == 0) break;

                std::vector<std::vector<bool>> dp(len + 1, std::vector<bool>(np + 1, false));
                dp[0][0] = true;

                for (size_t j = 0; j < np; j++) {
                    bool isOpt = patterns_[j].isOptional();

                    for (size_t i = 0; i <= len; i++) {
                        if (!dp[i][j]) continue;  // unreachable

                        if (isOpt) {
                            // Optional 1: Mismatch
                            dp[i][j + 1] = true;

                            // Optional 2: Match
                            if (i < len && PatternMatch(patterns_[j], symbols[start + i])) dp[i + 1][j + 1] = true;
                        }
                        
                        else {
                            // Match
                            if (i < len && PatternMatch(patterns_[j], symbols[start + i]))dp[i + 1][j + 1] = true;
                        }
                    }
                }

                // Success
                if (dp[len][np]) {

                    // Fill Move Positions for Move()
                    move_positions_.resize(np);

                    size_t cnt_skip = 0;
                    size_t i = len, j = np;
                    while (j > 0) {

                        // Skiped
                        // exist path: (i, j - 1) -> (i, j) dir: →
                        if (patterns_[j - 1].isOptional() && dp[i][j - 1]) {
                            // mark zero: not used
                            move_positions_[j - 1] = 0;
                            cnt_skip++;
                        }

                        // Not Skiped
                        // exist path: (i - 1, j - 1) -> (i, j) dir: ↘
                        else {
                            move_positions_[j - 1] = (int)(np - j) - (int)cnt_skip + 1;
                            i--;
                        }

                        j--;
                    }

                    out_reduce_len = 0;
                    for (auto& p : move_positions_)
                        if (p != 0) out_reduce_len++;

                    return true;
                }
            }

            return false;
        }
    
        // Move AstNode as type T from symbols
        template<typename T>
        static std::unique_ptr<T> Move(std::vector<Symbol>& syms, int pos) {
            size_t p = move_positions_[pos - 1];
            auto& astnode = std::get<std::unique_ptr<AstNode>>(syms[syms.size() - p].data());
            return std::unique_ptr<T>(static_cast<T*>(astnode.release()));
        }
    };

    // Syntactic Analyzer
    class Parser {
    private:
        // Predefined

        inline static std::vector<Rule> rules_;     // Reduce Rules
        const std::vector<Token>&       tokens_;    // Lexer's Tokens

        // Cache
        
        std::vector<Symbol>         symbols_;       // Symbols Stack
        std::vector<size_t>         scopes_brace;   // Brace Scope
        std::unique_ptr<AstNode>    root_;          // Program as AST Root Node.

        void RulesInit();

        void Shift(const Token& token);
        bool TryReduce(const Rule& rule, const Token* token_next, size_t reduce_len);

        Symbol Token2Symbol(const Token& token);

    public:
        Parser(const std::vector<Token>& tokens) : tokens_(tokens) {
            RulesInit();
        }

        void Execute();
        std::unique_ptr<AstNode>& root() {
            return root_;
        }
    
        void RuleAdd(
            std::initializer_list<SymbolPattern> patterns,
            Rule::ReduceCallback reduce_callback)
        {
            rules_.emplace_back(patterns, reduce_callback);
        }
    };
}

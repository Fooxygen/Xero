
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
        Symbol(std::unique_ptr<AstNode> node, Loc loc) {
            node->loc_ = loc;
            type_ = Type::AstNode;
            data_ = std::move(node);
        }
        
        Type   type() const {
            return type_;
        }
        Data&  data() {
            return data_;
        }

        Token::Type type_token() const {
            if (auto token = std::get_if<Token>(&data_)) {
                return token->type_;
            }
            return Token::Type::Undefined;
        }
        AstType     type_astnode() const {
            if (auto astnode = std::get_if<std::unique_ptr<AstNode>>(&data_)) {
                return astnode->get()->type_;
            }
            return AstType::Undefined;
        }

        Loc loc() const {
            if (auto token = std::get_if<Token>(&data_)) {
                return token->loc_;
            }
            if (auto astnode = std::get_if<std::unique_ptr<AstNode>>(&data_)) {
                return astnode->get()->loc_;
            }
            return Loc{};
        }
    };

    class SymbolPattern {
    private:
        Symbol::Type type_ = Symbol::Type::Undefined;
        std::vector<Token::Type>  type_tokens_;
        std::vector<AstType>      type_astnodes_;

        bool isOptional_ = false;

    public:
        SymbolPattern(Token::Type type)
        :   type_tokens_{type}
        {
            type_ = Symbol::Type::Token;
        }
        SymbolPattern(AstType type)
        :   type_astnodes_{type}
        {
            type_ = Symbol::Type::AstNode;
        }
        SymbolPattern(std::initializer_list<Token::Type> types)
        :   type_tokens_(types)
        {
            type_ = Symbol::Type::Token;
        }
        SymbolPattern(std::initializer_list<AstType> types)
        :   type_astnodes_(types)
        {
            type_ = Symbol::Type::AstNode;
        }

        Symbol::Type type() const { return type_; }
        const std::vector<Token::Type>& type_tokens()   const { return type_tokens_; }
        const std::vector<AstType>&     type_astnodes() const { return type_astnodes_; }
        bool isOptional()   const { return isOptional_; }
    
        // Make Optional SP 

        static SymbolPattern Opt(Token::Type type) {
            auto sp = SymbolPattern(type);
            sp.isOptional_ = true;
            return sp;
        }
        static SymbolPattern Opt(AstType type) {
            auto sp = SymbolPattern(type);
            sp.isOptional_ = true;
            return sp;
        }
        static SymbolPattern Opt(std::initializer_list<Token::Type> types) {
            auto sp = SymbolPattern(types);
            sp.isOptional_ = true;
            return sp;
        }
        static SymbolPattern Opt(std::initializer_list<AstType> types) {
            auto sp = SymbolPattern(types);
            sp.isOptional_ = true;
            return sp;
        }
    };

    // Parsing Rule
    // [TokenType, { TokenType, TokenType }, AstType, ...] -> AstType
    class Rule {
    public:
        using ReduceCallback = std::function<
            std::unique_ptr<AstNode>(
                std::vector<Symbol>& symbols,
                Token::Type token_next
            )
        >;

        // for Move():
        // while rule = AB[C]DE
        //     if pats = ABCDE, mps = [5, 4, 3, 2, 1]
        //     if pats = _ABDE, mps = [4, 3, 0, 2, 1]
        static inline std::vector<size_t> move_positions_;

    private:
        std::vector<SymbolPattern>  patterns_;
        std::vector<SymbolPattern>  prefix_delay_;
        std::vector<SymbolPattern>  prefix_allow_;
        std::vector<Token::Type>    suffix_delay_;      // Delay reduction when hit symmbol
        std::vector<Token::Type>    suffix_allow_;      // Delay reduction when miss symbol
        ReduceCallback              reduce_callback_;

        static void PatternIndexCheck(size_t pos) {
            if (pos < 1) {
                throw LogErr(LogModule::Parser, "invalid rule pattern index");
            }
        }

        bool isNeedDelayPrefix(const std::vector<Symbol>& symbols, size_t reduce_len) {
            if (prefix_delay_.empty() && prefix_allow_.empty()) return false;
            if (symbols.size() <= reduce_len) return false;

            const Symbol& pred = symbols[symbols.size() - reduce_len - 1];

            if (!prefix_delay_.empty()) {
                for (auto& sp : prefix_delay_) {
                    if (PatternMatch(sp, pred)) return true;
                }
            }

            if (!prefix_allow_.empty()) {
                bool isFind = false;
                for (auto& sp : prefix_allow_)
                    if (PatternMatch(sp, pred)) { isFind = true; break; }
                if (!isFind) return true;
            }
            return false;
        }
        bool isNeedDelaySuffix(Token::Type token_next) const {
            if (token_next == Token::Type::Undefined) return false;
            if (!suffix_delay_.empty() && std::ranges::contains(suffix_delay_, token_next))
                return true;
            if (!suffix_allow_.empty() && !std::ranges::contains(suffix_allow_, token_next))
                return true;
            return false;
        }

    public:
        Rule(
            std::initializer_list<SymbolPattern>    patterns,
            ReduceCallback                          reduce_callback,
            std::initializer_list<SymbolPattern>    prefix_delay = {},
            std::initializer_list<SymbolPattern>    prefix_allow = {},
            std::initializer_list<Token::Type>      suffix_delay = {},
            std::initializer_list<Token::Type>      suffix_allow = {}
        )
        :   patterns_(patterns),
            prefix_delay_(prefix_delay),
            prefix_allow_(prefix_allow),
            suffix_delay_(suffix_delay),
            suffix_allow_(suffix_allow),
            reduce_callback_(std::move(reduce_callback))
        {}

        const std::vector<SymbolPattern>& patterns() const { return patterns_; }
        const ReduceCallback& reduce_callback()      const { return reduce_callback_; }

        bool PatternMatch(const SymbolPattern& pat, const Symbol& sym) {

            // Token
            if (sym.type() == Symbol::Type::Token) {
                for (auto t : pat.type_tokens()) {
                    if (Token::isTypeCompatible(t, sym.type_token())) return true;
                }
                return false;
            }

            // AstNode
            for (auto a : pat.type_astnodes()) {
                if (isAstTypeCompatible(a, sym.type_astnode())) return true;
            }
 
            return false;
        }
        bool PatternsMatch(
            const std::vector<Symbol>& symbols, Token::Type token_next, size_t& out_reduce_len)
        {
            if (isNeedDelaySuffix(token_next)) return false;

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
                    for (auto& p : move_positions_) {
                        if (p != 0) out_reduce_len++;
                    }

                    if (isNeedDelayPrefix(symbols, out_reduce_len)) return false;

                    return true;
                }
            }

            return false;
        }
        static bool isOptPatternEmpty(size_t pos) {
            PatternIndexCheck(pos);
            return move_positions_[pos - 1] == 0;
        }

        static bool is(std::vector<Symbol>& syms, size_t pos, Token::Type token_type) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            auto& target = syms[syms.size() - pos];
            
            return  target.type() == Symbol::Type::Token &&
                    target.type_token() == token_type;
        }

        static bool is(std::vector<Symbol>& syms, size_t pos, AstType ast_type) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            auto& target = syms[syms.size() - pos];
            
            return  target.type() == Symbol::Type::AstNode &&
                    target.type_astnode() == ast_type;
        }

        static Token::Type GetTokenType(std::vector<Symbol>& syms, size_t pos) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            return syms[syms.size() - pos].type_token();
        }

        // Move AstNode as type T from symbols
        template<typename T>
        static std::unique_ptr<T> Move(std::vector<Symbol>& syms, size_t pos) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            auto& astnode = std::get<std::unique_ptr<AstNode>>(syms[syms.size() - pos].data());
            return std::unique_ptr<T>(static_cast<T*>(astnode.release()));
        }
    };

    // Syntactic Analyzer
    class Parser {
    private:
        // Predefined

        inline static std::vector<Rule> rules_;     // Reduce Rules
        std::vector<Token>&             tokens_;    // Lexer's Tokens

        // Cache
        
        std::vector<Symbol>      symbols_;          // Symbols Stack
        std::vector<size_t>      scopes_brace;      // Brace Scope
        std::unique_ptr<AstNode> root_;             // Program as AST Root Node.

        void RulesInit();

        void Shift(const Token& token);
        bool TryReduce(const Rule& rule, Token::Type token_next, size_t reduce_len);

        void   TokenRewrite(Token& token);          // Modify TokenType
        Symbol Token2Symbol(const Token& token);

    public:
        Parser(std::vector<Token>& tokens) : tokens_(tokens) {
            RulesInit();
        }
        ~Parser() { rules_.clear(); }

        void Execute();
        std::unique_ptr<AstNode>& root() {
            return root_;
        }
    
        void RuleAdd(
            std::initializer_list<SymbolPattern>    patterns,
            Rule::ReduceCallback                    reduce_callback,
            std::initializer_list<SymbolPattern>    prefix_delay = {},
            std::initializer_list<SymbolPattern>    prefix_allow = {},
            std::initializer_list<Token::Type>      suffix_delay = {},
            std::initializer_list<Token::Type>      suffix_allow = {})
        {
            rules_.emplace_back(
                patterns, reduce_callback,
                prefix_delay, prefix_allow, suffix_delay, suffix_allow
            );
        }
    };
}

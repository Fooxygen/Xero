
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
    using TT        = Token::Type;
    using AT        = AstType;
    using OT        = OperType;
    using TS        = std::vector<Token>;
    using TTS       = std::vector<TT>;
    using ATS       = std::vector<AT>;
    using TTS_INIT  = std::initializer_list<TT>;
    using ATS_INIT  = std::initializer_list<AT>;
    using ASTNODE   = std::unique_ptr<AstNode>;
    
    class Symbol {
    public:
        using Data = std::variant<Token, ASTNODE>;
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
        Symbol(ASTNODE node, Loc loc) {
            node->loc_ = loc;
            type_ = Type::AstNode;
            data_ = std::move(node);
        }
        
        Type   type() const { return type_; }
        Data&  data()       { return data_; }

        TT     type_token()   const {
            if (auto token = std::get_if<Token>(&data_)) {
                return token->type_;
            }
            return TT::Undefined;
        }
        AT     type_astnode() const {
            if (auto astnode = std::get_if<ASTNODE>(&data_)) {
                return astnode->get()->type_;
            }
            return AT::Undefined;
        }
        Loc    loc()          const {
            if (auto token = std::get_if<Token>(&data_)) {
                return token->loc_;
            }
            if (auto astnode = std::get_if<ASTNODE>(&data_)) {
                return astnode->get()->loc_;
            }
            return Loc{};
        }
    };
    using ST = Symbol::Type;
    using SS = std::vector<Symbol>;

    class SymbolPattern {
    private:
        ST  type_          = ST::Undefined;
        TTS type_tokens_   = {};
        ATS type_astnodes_ = {};

        bool isOptional_ = false;

    public:
        SymbolPattern(TT type)
        :   type_tokens_{type} { type_ = ST::Token; }
        SymbolPattern(AT type)
        :   type_astnodes_{type} { type_ = ST::AstNode; }
        SymbolPattern(TTS_INIT types)
        :   type_tokens_(types) { type_ = ST::Token; }
        SymbolPattern(ATS_INIT types)
        :   type_astnodes_(types) { type_ = ST::AstNode; }

        ST         type()          const { return type_; }
        const TTS& type_tokens()   const { return type_tokens_; }
        const ATS& type_astnodes() const { return type_astnodes_; }
        bool       isOptional()    const { return isOptional_; }
    
        // Make Optional SP 

        static SymbolPattern Opt(TT type) {
            auto sp = SymbolPattern(type);
            sp.isOptional_ = true;
            return sp;
        }
        static SymbolPattern Opt(AT type) {
            auto sp = SymbolPattern(type);
            sp.isOptional_ = true;
            return sp;
        }
        static SymbolPattern Opt(TTS_INIT types) {
            auto sp = SymbolPattern(types);
            sp.isOptional_ = true;
            return sp;
        }
        static SymbolPattern Opt(ATS_INIT types) {
            auto sp = SymbolPattern(types);
            sp.isOptional_ = true;
            return sp;
        }
    };
    using PATS      = std::vector<SymbolPattern>;
    using PATS_INIT = std::initializer_list<SymbolPattern>;

    // Parsing Rule
    // [TokenType, { TokenType, TokenType }, AstType, ...] -> AstType
    class Rule {
    public:
        using ReduceCallback = std::function<
            ASTNODE(SS& symbols, TT token_next)
        >;

        // for Move():
        // while rule = AB[C]DE
        //     if pats = ABCDE, mps = [5, 4, 3, 2, 1]
        //     if pats = _ABDE, mps = [4, 3, 0, 2, 1]
        static inline std::vector<size_t> move_positions_;

    private:
        PATS           patterns_;
        PATS           prefix_delay_;
        PATS           prefix_allow_;
        TTS            suffix_delay_;      // Delay reduction when hit symmbol
        TTS            suffix_allow_;      // Delay reduction when miss symbol
        ReduceCallback reduce_callback_;

        static void PatternIndexCheck(size_t pos) {
            if (pos < 1) {
                throw LogErr(LogModule::Parser, "invalid rule pattern index");
            }
        }

        bool isNeedDelayPrefix(const SS& symbols, size_t reduce_len) {
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
        bool isNeedDelaySuffix(TT token_next) const {
            if (token_next == TT::Undefined) return false;
            if (!suffix_delay_.empty() && std::ranges::contains(suffix_delay_, token_next))
                return true;
            if (!suffix_allow_.empty() && !std::ranges::contains(suffix_allow_, token_next))
                return true;
            return false;
        }

    public:
        Rule(
            PATS_INIT      patterns,
            ReduceCallback reduce_callback,
            PATS_INIT      prefix_delay = {},
            PATS_INIT      prefix_allow = {},
            TTS_INIT       suffix_delay = {},
            TTS_INIT       suffix_allow = {}
        )
        :   patterns_(patterns),
            prefix_delay_(prefix_delay),
            prefix_allow_(prefix_allow),
            suffix_delay_(suffix_delay),
            suffix_allow_(suffix_allow),
            reduce_callback_(std::move(reduce_callback))
        {}

        const PATS&           patterns()        const { return patterns_; }
        const ReduceCallback& reduce_callback() const { return reduce_callback_; }

        bool PatternMatch(const SymbolPattern& pat, const Symbol& sym) {

            // Token
            if (sym.type() == ST::Token) {
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
        bool PatternsMatch(const SS& symbols, TT token_next, size_t& out_reduce_len) {
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

        static bool is(SS& syms, size_t pos, TT token_type) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            auto& target = syms[syms.size() - pos];
            
            return  target.type() == ST::Token &&
                    target.type_token() == token_type;
        }
        static bool is(SS& syms, size_t pos, AT ast_type) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            auto& target = syms[syms.size() - pos];
            
            return  target.type() == ST::AstNode &&
                    target.type_astnode() == ast_type;
        }

        static Token::Type GetTokenType(SS& syms, size_t pos) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            return syms[syms.size() - pos].type_token();
        }

        // Move AstNode as type T from symbols
        template<typename T>
        static std::unique_ptr<T> Move(SS& syms, size_t pos) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            auto& astnode = std::get<ASTNODE>(syms[syms.size() - pos].data());
            return std::unique_ptr<T>(static_cast<T*>(astnode.release()));
        }
    };

    // Syntactic Analyzer
    class Parser {
    private:
        // Predefined

        inline static std::vector<Rule> rules_; // Reduce Rules
        TS& tokens_;    // Lexer's Tokens

        // Cache
        
        SS                  symbols_;           // Symbols Stack
        std::vector<size_t> scopes_brace;       // Brace Scope
        ASTNODE             root_;              // Program as AST Root Node.

        void RulesInit();

        void Shift(const Token& token);
        bool TryReduce(const Rule& rule, TT token_next, size_t reduce_len);

        void   TokenRewrite(Token& token);          // Modify TokenType
        Symbol Token2Symbol(const Token& token);

    public:
        Parser(TS& tokens) : tokens_(tokens) {
            RulesInit();
        }
        ~Parser() { rules_.clear(); }

        void Execute();
        ASTNODE& root() {
            return root_;
        }
    
        void RuleAdd(
            PATS_INIT               patterns,
            Rule::ReduceCallback    reduce_callback,
            PATS_INIT               prefix_delay = {},
            PATS_INIT               prefix_allow = {},
            TTS_INIT                suffix_delay = {},
            TTS_INIT                suffix_allow = {})
        {
            rules_.emplace_back(
                patterns, reduce_callback,
                prefix_delay, prefix_allow, suffix_delay, suffix_allow
            );
        }
    };
}

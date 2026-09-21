
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <vector>
#include <functional>
#include <variant>
#include <algorithm>

#include "common/defs/token.hpp"
#include "common/defs/ast.hpp"

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
            if (auto node = std::get_if<ASTNODE>(&data_)) {
                return node->get()->type_;
            }
            return AT::Undefined;
        }
        Loc    loc()          const {
            if (auto token = std::get_if<Token>(&data_)) {
                return token->loc_;
            }
            if (auto node = std::get_if<ASTNODE>(&data_)) {
                return node->get()->loc_;
            }
            return Loc{};
        }
    };
    using ST = Symbol::Type;
    using SS = std::vector<Symbol>;

    class SymbolPattern {
    private:
        ST   type_          = ST::Undefined;
        TTS  type_tokens_   = {};
        ATS  type_astnodes_ = {};

        bool is_optional_    = false;

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
        bool       IsOptional()    const { return is_optional_; }
    
        // Make Optional SP 

        static SymbolPattern Opt(TT type) {
            auto sp = SymbolPattern(type);
            sp.is_optional_ = true;
            return sp;
        }
        static SymbolPattern Opt(AT type) {
            auto sp = SymbolPattern(type);
            sp.is_optional_ = true;
            return sp;
        }
        static SymbolPattern Opt(TTS_INIT types) {
            auto sp = SymbolPattern(types);
            sp.is_optional_ = true;
            return sp;
        }
        static SymbolPattern Opt(ATS_INIT types) {
            auto sp = SymbolPattern(types);
            sp.is_optional_ = true;
            return sp;
        }
    };
    using PATS      = std::vector<SymbolPattern>;
    using PATS_INIT = std::initializer_list<SymbolPattern>;

    // Parsing Rule
    // [TokenType, { TokenType, TokenType }, AstType, ...] -> AstType
    class Rule {
    public:
        using ReduceCallback = std::function<ASTNODE(TT token_next)>;

    private:
        PATS           patterns_;
        PATS           prefix_delay_;
        PATS           prefix_allow_;
        TTS            suffix_delay_;      // delay reduction when hit symmbol
        TTS            suffix_allow_;      // delay reduction when miss symbol
        ReduceCallback reduce_callback_;

    private:
        bool IsDelayByPrefix(const SS& symbols, size_t reduce_len) {
            if (prefix_delay_.empty() && prefix_allow_.empty()) return false;
            if (symbols.size() <= reduce_len) return false;

            const Symbol& pred = symbols[symbols.size() - reduce_len - 1];

            if (!prefix_delay_.empty()) {
                for (auto& sp : prefix_delay_) {
                    if (PatternMatchTry(sp, pred)) return true;
                }
            }

            if (!prefix_allow_.empty()) {
                bool is_find = false;
                for (auto& sp : prefix_allow_)
                    if (PatternMatchTry(sp, pred)) { is_find = true; break; }
                if (!is_find) return true;
            }
            return false;
        }
        bool IsDelayBySuffix(TT token_next) const {
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

    public:
        bool PatternMatchTry(const SymbolPattern& pat, const Symbol& sym) {

            // Token
            if (sym.type() == ST::Token) {
                for (auto t : pat.type_tokens()) {
                    if (Token::IsTypeCompatible(t, sym.type_token())) return true;
                }
                return false;
            }

            // AstNode
            for (auto a : pat.type_astnodes()) {
                if (IsAstTypeCompatible(a, sym.type_astnode())) return true;
            }
 
            return false;
        }
        bool PatternsMatchTry(const SS& syms, TT token_next, std::vector<size_t>& move_positions, size_t& out_reduce_len) {
            if (IsDelayBySuffix(token_next)) return false;

            // Match Check
            size_t np = patterns_.size();
            size_t ns = syms.size();

            size_t start_max = (ns > np) ? (ns - np) : 0;

            for (size_t start = start_max; start <= ns; start++) {
                
                size_t len = ns - start;
                if (len == 0) break;

                std::vector<std::vector<bool>> dp(len + 1, std::vector<bool>(np + 1, false));
                dp[0][0] = true;

                for (size_t j = 0; j < np; j++) {
                    bool is_optional = patterns_[j].IsOptional();

                    for (size_t i = 0; i <= len; i++) {
                        if (!dp[i][j]) continue;  // unreachable

                        if (is_optional) {
                            // Optional 1: Mismatch
                            dp[i][j + 1] = true;

                            // Optional 2: Match
                            if (i < len && PatternMatchTry(patterns_[j], syms[start + i])) dp[i + 1][j + 1] = true;
                        }
                        
                        else {
                            // Match
                            if (i < len && PatternMatchTry(patterns_[j], syms[start + i]))
                                dp[i + 1][j + 1] = true;
                        }
                    }
                }

                // Success
                if (dp[len][np]) {

                    // Fill Move Positions for Move()
                    move_positions.resize(np);

                    size_t cnt_skip = 0;
                    size_t i = len, j = np;
                    while (j > 0) {

                        // Skiped
                        // Exist path: (i, j - 1) -> (i, j) dir: →
                        if (patterns_[j - 1].IsOptional() && dp[i][j - 1]) {
                            // Mark zero: not used
                            move_positions[j - 1] = 0;
                            cnt_skip++;
                        }

                        // Not Skiped
                        // Exist path: (i - 1, j - 1) -> (i, j) dir: ↘
                        else {
                            move_positions[j - 1] = (int)(np - j) - (int)cnt_skip + 1;
                            i--;
                        }

                        j--;
                    }

                    out_reduce_len = 0;
                    for (auto& p : move_positions) {
                        if (p != 0) out_reduce_len++;
                    }

                    if (IsDelayByPrefix(syms, out_reduce_len)) return false;

                    return true;
                }
            }

            return false;
        }
    };

    // Syntactic Analyzer
    class Parser {
    private:
        // Defined

        std::vector<Rule> rules_;
        TS& tokens_;

        // Cache
        
        SS                  symbols_;           // symbols stack
        std::vector<size_t> scopes_brace_;      // brace scope

        // for Move():
        // while rule = AB[C]DE
        //     if pats = ABCDE, mps = [5, 4, 3, 2, 1]
        //     if pats = _ABDE, mps = [4, 3, 0, 2, 1]
        std::vector<size_t> move_positions_;

        // Result

        ASTNODE             root_;              // program

    private:
        // Defined

        void   RulesInit();

        // Token

        void   TokenRewrite(Token& token);
        Symbol Token2Symbol(const Token& token);

        // Parsing

        void   Shift(const Token& token);
        bool   ReduceTry(const Rule& rule, TT token_next, size_t reduce_len);

        void        PatternIndexCheck(size_t pos) {
            if (pos < 1) {
                throw LogErr(LogModule::Parser, "invalid rule pattern index");
            }
        }
        Token::Type PatternTokenTypeGet(size_t pos) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            return symbols_[symbols_.size() - pos].type_token();
        }
        
        bool   IsOptPatternEmpty(size_t pos) {
            PatternIndexCheck(pos);
            return move_positions_[pos - 1] == 0;
        }
        bool   IsPattern(size_t pos, TT token_type) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            auto& target = symbols_[symbols_.size() - pos];
            
            return  target.type() == ST::Token &&
                    target.type_token() == token_type;
        }
        bool   IsPattern(size_t pos, AT ast_type) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            auto& target = symbols_[symbols_.size() - pos];
            
            return  target.type() == ST::AstNode &&
                    target.type_astnode() == ast_type;
        }

        // Move AstNode as type T from symbols
        template<typename T>
        std::unique_ptr<T> Move(size_t pos) {
            PatternIndexCheck(pos);
            pos = move_positions_[pos - 1];
            auto& node = std::get<ASTNODE>(symbols_[symbols_.size() - pos].data());
            return std::unique_ptr<T>(static_cast<T*>(node.release()));
        }

    public:
        Parser(TS& tokens) : tokens_(tokens) { RulesInit(); }
        ~Parser() { rules_.clear(); }

        ASTNODE& root() { return root_; }

    public:
        void Execute();
    
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

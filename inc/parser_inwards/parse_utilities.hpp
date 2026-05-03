#pragma once

#include "../ast.hpp"
#include "../token.hpp"
#include <expected>
#include <format>
#include <string_view>
#include <vector>

namespace parser {

constexpr size_t MAX_AST_DEPTH = 256;

struct ParseState {
    const std::vector<Token>& m_tokens;
    size_t m_pos = 0;
    size_t m_depth = 0;
};

struct ParseError {
    SourceLocation loc;
    std::string message;
};

// RAII Guard to increment/decrement depth automatically
struct DepthGuard {
    ParseState& state;
    inline explicit DepthGuard(ParseState& s) noexcept : state(s) {
        ++state.m_depth;
    }
    inline ~DepthGuard() noexcept {
        --state.m_depth;
    }

    DepthGuard(const DepthGuard&) = delete;
    DepthGuard& operator=(const DepthGuard&) = delete;
};

[[nodiscard]] inline const Token& peek(const ParseState& state) noexcept {
    return state.m_tokens[state.m_pos];
}

template <typename T> using ParseResult = std::expected<T, ParseError>;

inline ParseResult<void> check_depth(const ParseState& state) {
    if (state.m_depth >= MAX_AST_DEPTH) {
        return std::unexpected(
            ParseError{peek(state).m_loc,
                       "Превышена максимальная глубина вложенности AST"
                       "(защита от переполнения стека)"});
    }
    return {};
}

// To prevent deep nesting boilerplate.
#define PARSE_TRY(var, expr)                                                   \
    auto _res_##var = (expr);                                                  \
    if (!_res_##var) return std::unexpected(_res_##var.error());               \
    auto var = std::move(*_res_##var)

// Used when the expression returns Result<void>
#define PARSE_TRY_VOID(expr)                                                   \
    if (auto _res = (expr); !_res) return std::unexpected(_res.error())

// Macro to securely initialize the depth guard at function entry
#define PARSE_DEPTH_GUARD(state)                                               \
    PARSE_TRY_VOID(check_depth(state));                                        \
    DepthGuard ast_depth_guard_ {                                              \
        state                                                                  \
    }

[[nodiscard]] inline bool is_at_end(const ParseState& state) noexcept {
    return state.m_pos >= state.m_tokens.size() ||
           std::holds_alternative<token::Eof>(
               state.m_tokens[state.m_pos].m_value);
}

inline Token advance(ParseState& state) noexcept {
    if (!is_at_end(state)) ++state.m_pos;
    return state.m_tokens[state.m_pos - 1];
}

template <typename T>
[[nodiscard]] inline const T* get_if(const Token& t) noexcept {
    return std::get_if<T>(&t.m_value);
}

inline bool check_keyword(const ParseState& state, token::Keyword k) noexcept {
    auto* kw = get_if<token::Keyword>(peek(state));
    return kw && *kw == k;
}

inline bool check_punct(const ParseState& state, token::Punctuator p) noexcept {
    auto* punct = get_if<token::Punctuator>(peek(state));
    return punct && *punct == p;
}

inline bool check_op(const ParseState& state, token::Operator o) noexcept {
    auto* op = get_if<token::Operator>(peek(state));
    return op && *op == o;
}

inline bool match_keyword(ParseState& state, token::Keyword k) noexcept {
    if (check_keyword(state, k)) {
        advance(state);
        return true;
    }
    return false;
}

inline bool match_punct(ParseState& state, token::Punctuator p) noexcept {
    if (check_punct(state, p)) {
        advance(state);
        return true;
    }
    return false;
}

inline bool match_op(ParseState& state, token::Operator o) noexcept {
    if (check_op(state, o)) {
        advance(state);
        return true;
    }
    return false;
}

inline ParseResult<void> consume_punct(ParseState& state,
                                       token::Punctuator p,
                                       std::string_view err_msg = "") {
    if (!match_punct(state, p))
        return std::unexpected(
            ParseError{peek(state).m_loc, std::string(err_msg)});
    return {};
}

inline ParseResult<void> consume_keyword(ParseState& state,
                                         token::Keyword k,
                                         std::string_view err_msg = "") {
    if (!match_keyword(state, k))
        return std::unexpected(
            ParseError{peek(state).m_loc, std::string(err_msg)});
    return {};
}

inline ParseResult<void> consume_op(ParseState& state,
                                    token::Operator o,
                                    std::string_view err_msg = "") {
    if (!match_op(state, o))
        return std::unexpected(
            ParseError{peek(state).m_loc, std::string(err_msg)});
    return {};
}

inline bool check_assign(const ParseState& state,
                         token::Assignment a) noexcept {
    auto* assign = get_if<token::Assignment>(peek(state));
    return assign && *assign == a;
}

inline bool match_assign(ParseState& state, token::Assignment a) noexcept {
    if (check_assign(state, a)) {
        advance(state);
        return true;
    }
    return false;
}

inline ParseResult<void> consume_assign(ParseState& state,
                                        token::Assignment a,
                                        std::string_view err_msg = "") {
    if (!match_assign(state, a))
        return std::unexpected(
            ParseError{peek(state).m_loc, std::string(err_msg)});
    return {};
}

inline ParseResult<std::string_view>
consume_identifier(ParseState& state, std::string_view err_msg = "") {
    if (auto* id = get_if<token::Identifier>(peek(state))) {
        auto text = id->text;
        advance(state);
        return text;
    }
    return std::unexpected(ParseError{peek(state).m_loc, std::string(err_msg)});
}

}  // namespace parser

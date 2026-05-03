#pragma once

#include "parse_utilities.hpp"

namespace parser {

enum class Precedence {
    None = 0,
    Assignment,  // Level 13
    LogicalOr,   // Level 12 (или)
    LogicalAnd,  // Level 11 (и)
    Equality,    // Level 10 (==, !=)
    Relational,  // Level 9 (<, >, <=, >=)
    BitOr,       // Level 8 (|)
    BitXor,      // Level 7 (^)
    BitAnd,      // Level 6 (&)
    Shift,       // Level 5 (<<, >>)
    Term,        // Level 4 (+, -)
    Factor,      // Level 3 (*, /, %)
    Unary,       // Level 2 (-, ~, не, как)
    Call         // Level 1 ((), [], ., \, ...)
};

Precedence get_precedence(const Token& t) noexcept;

ParseResult<std::unique_ptr<ast::BlockStmt>> parse_block(ParseState& state);

// Parses standard expressions AND explicit spread initializations contextually
[[nodiscard]] ParseResult<ast::ExprPtr> parse_init_value(ParseState& state);

[[nodiscard]] ParseResult<ast::ExprPtr> parse_expression(ParseState& state,
                                                         Precedence precedence);

}  // namespace parser

#pragma once

#include <string_view>
#include <vector>
#include <expected>
#include "../inc/token.hpp"

class Lexer {
public:
    explicit Lexer(std::string_view source) noexcept;
    [[nodiscard]] static std::expected<std::vector<Token>, token::Error> tokenize(std::string_view source);

private:
    std::string_view m_source;
    uint32_t m_pos;
    uint32_t m_line;
    uint32_t m_line_start;

    [[nodiscard]] std::expected<std::vector<Token>, token::Error> tokenize() noexcept;
    [[nodiscard]] char current() const noexcept;
    [[nodiscard]] char peek_ahead(uint32_t offset = 1) const noexcept;
    void advance_char() noexcept;
    bool match_sequence(std::string_view seq) const noexcept;
    static bool is_utf8_continuation(char c) noexcept;
    static bool is_ident_start(char c) noexcept;
    static bool is_ident_char(char c) noexcept;
    Token create_token(Token::ValueVariant value, uint32_t start_pos) const noexcept;
    Token error_token(std::string_view message, uint32_t start_pos) const noexcept;
    void skip_whitespace_and_comments() noexcept;
    Token extract() noexcept;
    Token extract_string(uint32_t start_pos) noexcept;
    Token extract_number(uint32_t start_pos) noexcept;
    Token extract_invalid_float(uint32_t start_pos) noexcept;
    Token extract_hex(uint32_t start_pos) noexcept;
    Token extract_octal(uint32_t start_pos) noexcept;
    Token extract_binary(uint32_t start_pos) noexcept;
    Token extract_float_remainder(uint32_t start_pos, uint64_t int_part) noexcept;
    Token extract_identifier(uint32_t start_pos) noexcept;
    Token extract_operator_or_punct(uint32_t start_pos) noexcept;
};

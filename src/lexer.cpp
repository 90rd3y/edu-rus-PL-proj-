#include "../inc/lexer.hpp"
#include "../inc/token.hpp"

#include <cctype>
#include <cmath>
#include <expected>
#include <iostream>
#include <unordered_map>
#include <vector>

static const std::unordered_map<std::string_view, Token::ValueVariant>
    KeywordMap = {
        // Keywords
        {"пер", token::Keyword::Var},
        {"пост", token::Keyword::Const},
        {"функ", token::Keyword::Func},
        {"структ", token::Keyword::Struct},
        {"стат_структ", token::Keyword::StatStruct},
        {"область", token::Keyword::Namespace},
        {"если", token::Keyword::If},
        {"тогда", token::Keyword::Then},
        {"иначе", token::Keyword::Else},
        {"пока", token::Keyword::While},
        {"вернуть", token::Keyword::Return},
        {"прекратить", token::Keyword::Break},
        {"продолжить", token::Keyword::Continue},
        {"изм", token::Keyword::Mut},
        {"включить", token::Keyword::Include},
        {"из", token::Keyword::From},
        {"употреб", token::Keyword::Using},
        {"как", token::Keyword::As},

        // Types
        {"ц8", token::TypeTok::I8},
        {"ц8б", token::TypeTok::U8},
        {"ц16", token::TypeTok::I16},
        {"ц32", token::TypeTok::I32},
        {"ц64", token::TypeTok::I64},
        {"ц16б", token::TypeTok::U16},
        {"ц32б", token::TypeTok::U32},
        {"ц64б", token::TypeTok::U64},
        {"пт32", token::TypeTok::F32},
        {"пт64", token::TypeTok::F64},
        {"лог", token::TypeTok::Bool},
        {"стр", token::TypeTok::Str},
        {"ряд", token::Keyword::DynArray},
        {"стат_ряд", token::Keyword::StatArray},

        // Literals (Converted to Literal variant immediately)
        {"истина", token::Literal{token::BoolLiteral{true}}},
        {"ложь", token::Literal{token::BoolLiteral{false}}},
        {"ничто", token::Literal{token::NullLiteral{}}},

        // Logical Operators parsed as text natively
        {"и", token::Operator::LogAnd},
        {"или", token::Operator::LogOr},
        {"не", token::Operator::LogNot}};

// Resolution inside the Lexer
Token::ValueVariant resolve_identifier(std::string_view text) {
    if (auto it = KeywordMap.find(text); it != KeywordMap.end()) {
        return it->second;
    }
    return token::Identifier{text};
}

Lexer::Lexer(std::string_view source) noexcept
    : m_source(source), m_pos(0), m_line(1), m_line_start(0) {}

std::expected<std::vector<Token>, token::Error>
Lexer::tokenize(std::string_view source) {
    Lexer lexer(source);
    return lexer.tokenize();
}

std::expected<std::vector<Token>, token::Error> Lexer::tokenize() noexcept {
    std::vector<Token> tokens;
    while (true) {
        Token t = extract();
        if (std::holds_alternative<token::Error>(t.m_value)) {
            return std::unexpected(std::get<token::Error>(t.m_value));
        }
        if (std::holds_alternative<token::Eof>(t.m_value)) {
            tokens.push_back(std::move(t));
            break;
        }
        tokens.push_back(std::move(t));
    }
    return tokens;
}

char Lexer::current() const noexcept {
    return m_pos < m_source.length() ? m_source[m_pos] : '\0';
}

char Lexer::peek_ahead(uint32_t offset) const noexcept {
    return m_pos + offset < m_source.length() ? m_source[m_pos + offset] : '\0';
}

void Lexer::advance_char() noexcept {
    ++m_pos;
}

bool Lexer::match_sequence(std::string_view seq) const noexcept {
    if (m_pos + seq.length() > m_source.length()) return false;
    return m_source.substr(m_pos, seq.length()) == seq;
}

bool Lexer::is_utf8_continuation(char c) noexcept {
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

bool Lexer::is_ident_start(char c) noexcept {
    unsigned char uc = static_cast<unsigned char>(c);
    // Only allow '_' or Cyrillic UTF-8 leading bytes (0xD0, 0xD1)
    return uc == '_' || uc == 0xD0 || uc == 0xD1;
}

bool Lexer::is_ident_char(char c) noexcept {
    return is_ident_start(c) || std::isdigit(static_cast<unsigned char>(c)) ||
           is_utf8_continuation(c);
}

Token Lexer::create_token(Token::ValueVariant value,
                          uint32_t start_pos) const noexcept {
    uint32_t len = m_pos - start_pos;
    SourceLocation loc{m_line, start_pos - m_line_start + 1, start_pos, len};
    return Token{std::move(value), loc, m_source.substr(start_pos, len)};
}

Token Lexer::error_token(std::string_view message,
                         uint32_t start_pos) const noexcept {
    uint32_t len = m_pos > start_pos ? m_pos - start_pos : 1;
    SourceLocation loc{m_line, start_pos - m_line_start + 1, start_pos, len};
    return Token{token::Error{message}, loc, m_source.substr(start_pos, len)};
}

void Lexer::skip_whitespace_and_comments() noexcept {
    while (m_pos < m_source.length()) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance_char();
        } else if (c == '\n') {
            advance_char();
            ++m_line;
            m_line_start = m_pos;
        } else if (c == '@') {
            while (m_pos < m_source.length() && current() != '\n') {
                advance_char();
            }
        } else
            break;
    }
}

Token Lexer::extract() noexcept {
    skip_whitespace_and_comments();

    uint32_t start_pos = m_pos;
    if (m_pos >= m_source.length()) {
        return create_token(token::Eof{}, start_pos);
    }

    char c = current();

    if (c == '"') return extract_string(start_pos);
    if (std::isdigit(static_cast<unsigned char>(c)))
        return extract_number(start_pos);
    if (is_ident_start(c)) return extract_identifier(start_pos);
    return extract_operator_or_punct(start_pos);
}

Token Lexer::extract_string(uint32_t start_pos) noexcept {
    advance_char();  // Skip opening quote
    while (m_pos < m_source.length()) {
        char c = current();
        if (c == '"') {
            advance_char();
            return create_token(
                token::Literal{token::StringLiteral{m_source.substr(
                    start_pos + 1, (m_pos - 1) - (start_pos + 1))}},
                start_pos);
        }
        if (c == '\n') {
            return error_token(
                "Перенос строки внутри строкового литерала недопустим",
                start_pos);
        }
        if (c == '\\') {
            advance_char();
            if (m_pos >= m_source.length()) {
                return error_token(
                    "Экранирующая последовательность не была завершена",
                    start_pos);
            }

            if (char d = current(); d == '"' || d == '\\' || d == '0') {
                advance_char();
            } else if (match_sequence("\xD0\xBD")) {  // 'н'
                m_pos += 2;
            } else if (match_sequence("\xD0\xBE")) {  // 'о'
                m_pos += 2;
            } else {
                return error_token("Неверная экранирующая последовательность",
                                   start_pos);
            }
        } else {
            advance_char();
        }
    }
    return error_token(
        "Строковый литерал не был завершен двойной кавычкой '\"'", start_pos);
}

Token Lexer::extract_number(uint32_t start_pos) noexcept {
    if (current() == '0') {
        if (match_sequence("0\xD1\x88")) {  // 0ш
            m_pos += 3;
            return extract_hex(start_pos);
        }
        if (match_sequence("0\xD0\xB2")) {  // 0в
            m_pos += 3;
            return extract_octal(start_pos);
        }
        if (match_sequence("0\xD0\xB4\x31")) {  // 0д1
            m_pos += 4;
            return extract_binary(start_pos);
        }
        if (match_sequence("0\xD0\xB4")) {  // Invalid binary start
            return error_token("Двоичный литерал должен начинаться с '0д1'\n"
                               "(нуль допустим только без приставки: '0')",
                               start_pos);
        }

        advance_char();  // Consume '0'

        if (current() == '.') {
            char peeked = peek_ahead();
            if (std::isdigit(static_cast<unsigned char>(peeked))) {
                return extract_float_remainder(start_pos, 0);
            } else if (peeked == '.') {
                // Spread operator follows ('0...'), fallback to integer
                return create_token(token::Literal{token::IntLiteral{0}},
                                    start_pos);
            } else {
                return extract_invalid_float(start_pos);
            }
        }

        if (std::isdigit(static_cast<unsigned char>(current()))) {
            return error_token(
                "Ненулевой численный литерал не может начинаться с нуля",
                start_pos);
        }

        return create_token(token::Literal{token::IntLiteral{0}}, start_pos);
    }

    uint64_t int_part = 0;
    while (m_pos < m_source.length() &&
           std::isdigit(static_cast<unsigned char>(current()))) {
        int_part = int_part * 10 + (current() - '0');
        advance_char();
    }

    if (current() == '.') {
        char peeked = peek_ahead();
        if (std::isdigit(static_cast<unsigned char>(peeked))) {
            return extract_float_remainder(start_pos, int_part);
        } else if (peeked == '.') {
            // Spread operator follows ('123...'), fallback to integer
            return create_token(token::Literal{token::IntLiteral{int_part}},
                                start_pos);
        } else {
            return extract_invalid_float(start_pos);
        }
    }

    return create_token(token::Literal{token::IntLiteral{int_part}}, start_pos);
}

Token Lexer::extract_invalid_float(uint32_t start_pos) noexcept {
    advance_char();  // Consume '.'

    // Consume the malformed exponent part if present for a wider, clearer
    // error span
    if (match_sequence("\xD1\x8D")) {  // 'э'
        m_pos += 2;
        if (current() == '-') {
            advance_char();
        }
        while (m_pos < m_source.length() &&
               std::isdigit(static_cast<unsigned char>(current()))) {
            advance_char();
        }
    }

    return error_token(
        "Неверный формат числа с плавающей точкой: ожидалась цифра после точки",
        start_pos);
}

Token Lexer::extract_hex(uint32_t start_pos) noexcept {
    uint64_t val = 0;
    char c = current();
    bool first_valid = false;

    if (c >= '1' && c <= '9') {
        val = c - '0';
        advance_char();
        first_valid = true;
    } else if (match_sequence("\xD0\x90")) {
        val = 10;
        m_pos += 2;
        first_valid = true;
    }  // А
    else if (match_sequence("\xD0\x91")) {
        val = 11;
        m_pos += 2;
        first_valid = true;
    }  // Б
    else if (match_sequence("\xD0\x92")) {
        val = 12;
        m_pos += 2;
        first_valid = true;
    }  // В
    else if (match_sequence("\xD0\x93")) {
        val = 13;
        m_pos += 2;
        first_valid = true;
    }  // Г
    else if (match_sequence("\xD0\x94")) {
        val = 14;
        m_pos += 2;
        first_valid = true;
    }  // Д
    else if (match_sequence("\xD0\x95")) {
        val = 15;
        m_pos += 2;
        first_valid = true;
    }  // Е

    if (!first_valid) {
        if (c == '0') {
            return error_token(
                "Шестнадцатеричный литерал не может начинаться с нуля\n"
                "(нуль допустим только без приставки: '0')",
                start_pos);
        }
        return error_token(
            "Ожидалась ненулевая шестнадцатеричная цифра после '0ш'",
            start_pos);
    }

    while (m_pos < m_source.length()) {
        c = current();
        if (std::isdigit(static_cast<unsigned char>(c))) {
            val = val * 16 + (c - '0');
            advance_char();
        } else if (match_sequence("\xD0\x90")) {
            val = val * 16 + 10;
            m_pos += 2;
        } else if (match_sequence("\xD0\x91")) {
            val = val * 16 + 11;
            m_pos += 2;
        } else if (match_sequence("\xD0\x92")) {
            val = val * 16 + 12;
            m_pos += 2;
        } else if (match_sequence("\xD0\x93")) {
            val = val * 16 + 13;
            m_pos += 2;
        } else if (match_sequence("\xD0\x94")) {
            val = val * 16 + 14;
            m_pos += 2;
        } else if (match_sequence("\xD0\x95")) {
            val = val * 16 + 15;
            m_pos += 2;
        } else
            break;
    }
    return create_token(token::Literal{token::IntLiteral{val}}, start_pos);
}

Token Lexer::extract_octal(uint32_t start_pos) noexcept {
    uint64_t val = 0;
    char c = current();

    if (c >= '1' && c <= '7') {
        val = c - '0';
        advance_char();
    } else {
        if (c == '0') {
            return error_token(
                "Восьмеричный литерал не может начинаться с нуля\n"
                "(нуль допустим только без приставки: '0')",
                start_pos);
        }
        return error_token("Ожидалась восьмеричная цифра после '0в'",
                           start_pos);
    }

    while (m_pos < m_source.length() && current() >= '0' && current() <= '7') {
        val = val * 8 + (current() - '0');
        advance_char();
    }
    return create_token(token::Literal{token::IntLiteral{val}}, start_pos);
}

Token Lexer::extract_binary(uint32_t start_pos) noexcept {
    uint64_t val = 1;  // '0д1' already parsed the minimal '1'
    while (m_pos < m_source.length() &&
           (current() == '0' || current() == '1')) {
        val = (val << 1) | (current() - '0');
        advance_char();
    }
    return create_token(token::Literal{token::IntLiteral{val}}, start_pos);
}

Token Lexer::extract_float_remainder(uint32_t start_pos,
                                     uint64_t int_part) noexcept {
    advance_char();  // Consume '.'

    double fraction = 0.0;
    double divisor = 10.0;
    while (m_pos < m_source.length() &&
           std::isdigit(static_cast<unsigned char>(current()))) {
        fraction += (current() - '0') / divisor;
        divisor *= 10.0;
        advance_char();
    }

    double value = static_cast<double>(int_part) + fraction;

    // 'э' = \xD1\x8D
    if (match_sequence("\xD1\x8D")) {
        m_pos += 2;
        bool negative_exp = false;
        if (current() == '-') {
            negative_exp = true;
            advance_char();
        }

        if (current() == '0') {
            return error_token("Экспонента числа с плавающей точкой не может "
                               "начинаться с нуля",
                               start_pos);
        }
        if (!std::isdigit(static_cast<unsigned char>(current()))) {
            return error_token(
                "Ожидались цифры после экспоненты числа с плавающей "
                "точкой 'э'",
                start_pos);
        }

        int exp_part = 0;
        while (m_pos < m_source.length() &&
               std::isdigit(static_cast<unsigned char>(current()))) {
            exp_part = exp_part * 10 + (current() - '0');
            advance_char();
        }

        value = value * std::pow(10.0, negative_exp ? -exp_part : exp_part);
    }

    return create_token(token::Literal{token::FloatLiteral{value}}, start_pos);
}

Token Lexer::extract_identifier(uint32_t start_pos) noexcept {
    while (m_pos < m_source.length() && is_ident_char(current())) {
        advance_char();
    }
    std::string_view text = m_source.substr(start_pos, m_pos - start_pos);
    return create_token(resolve_identifier(text), start_pos);
}

Token Lexer::extract_operator_or_punct(uint32_t start_pos) noexcept {
    char c = current();
    advance_char();

    switch (c) {
    case '(':
        return create_token(token::Punctuator::LParen, start_pos);
    case ')':
        return create_token(token::Punctuator::RParen, start_pos);
    case '{':
        return create_token(token::Punctuator::LBrace, start_pos);
    case '}':
        return create_token(token::Punctuator::RBrace, start_pos);
    case '[':
        return create_token(token::Punctuator::LBracket, start_pos);
    case ']':
        return create_token(token::Punctuator::RBracket, start_pos);
    case ',':
        return create_token(token::Punctuator::Comma, start_pos);
    case ':':
        return create_token(token::Punctuator::Colon, start_pos);
    case ';':
        return create_token(token::Punctuator::Semicolon, start_pos);
    case '\\':
        return create_token(token::Punctuator::Backslash, start_pos);
    case '?':
        return create_token(token::TypeMorpheme::Optional, start_pos);
    case '~':
        return create_token(token::Operator::BitNot, start_pos);
    case '%':
        return create_token(token::Operator::Mod, start_pos);
    case '^':
        return create_token(token::Operator::BitXor, start_pos);

    case '.':
        if (current() == '.' && peek_ahead() == '.') {
            m_pos += 2;
            return create_token(token::Spread{}, start_pos);
        }
        return create_token(token::Punctuator::Dot, start_pos);

    case '!':
        if (current() == '=') {
            advance_char();
            return create_token(token::Operator::Neq, start_pos);
        }
        return create_token(token::TypeMorpheme::MustUse, start_pos);

    case '=':
        if (current() == '=') {
            advance_char();
            return create_token(token::Operator::Eq, start_pos);
        }
        return create_token(token::Assignment::Assign, start_pos);

    case '+':
        if (current() == '=') {
            advance_char();
            return create_token(token::Assignment::PlusAssign, start_pos);
        }
        return create_token(token::Operator::Plus, start_pos);

    case '-':
        if (current() == '=') {
            advance_char();
            return create_token(token::Assignment::MinusAssign, start_pos);
        }
        if (current() == '>') {
            advance_char();
            return create_token(token::Punctuator::Arrow, start_pos);
        }
        return create_token(token::Operator::Minus, start_pos);

    case '*':
        if (current() == '=') {
            advance_char();
            return create_token(token::Assignment::StarAssign, start_pos);
        }
        return create_token(token::Operator::Star, start_pos);

    case '/':
        if (current() == '=') {
            advance_char();
            return create_token(token::Assignment::SlashAssign, start_pos);
        }
        return create_token(token::Operator::Slash, start_pos);

    case '<':
        if (current() == '<') {
            advance_char();
            return create_token(token::Operator::Shl, start_pos);
        }
        if (current() == '=') {
            advance_char();
            return create_token(token::Operator::Le, start_pos);
        }
        return create_token(token::Operator::Lt, start_pos);

    case '>':
        if (current() == '>') {
            advance_char();
            return create_token(token::Operator::Shr, start_pos);
        }
        if (current() == '=') {
            advance_char();
            return create_token(token::Operator::Ge, start_pos);
        }
        return create_token(token::Operator::Gt, start_pos);

    case '&':
        return create_token(token::Operator::BitAnd, start_pos);
    case '|':
        return create_token(token::Operator::BitOr, start_pos);

    default:
        return error_token("Недопустимый символ", start_pos);
    }
}
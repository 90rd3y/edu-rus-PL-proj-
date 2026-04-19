#include <iostream>
#include <string_view>
#include <vector>
#include <format>
#include <cassert>
#include <cmath>

// Include the lexer header
#include "../inc/lexer.hpp"

// --- Test Framework Helpers ---

struct TestRunner {
    size_t passed = 0;
    size_t failed = 0;

    void run(std::string_view name, void(*test_func)()) {
        try {
            test_func();
            std::cout << std::format("[PASS] {}\n", name);
            passed++;
        } catch (const std::exception& e) {
            std::cout << std::format("[FAIL] {}: {}\n", name, e.what());
            failed++;
        }
    }

    void summary() const {
        std::cout << std::format("\n[SUMMARY] {}/{} tests passed.\n", passed, passed + failed);
        if (failed > 0) std::exit(1);
    }
};

// Exception-based assertion for the test runner
#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) throw std::runtime_error(std::format("Assertion failed: {}:{} - {}", __FILE__, __LINE__, msg)); \
    } while (false)

// --- Token Matching Helpers ---

bool is_kw(const Token& t, token::Keyword k) {
    return std::holds_alternative<token::Keyword>(t.m_value) && std::get<token::Keyword>(t.m_value) == k;
}

bool is_op(const Token& t, token::Operator o) {
    return std::holds_alternative<token::Operator>(t.m_value) && std::get<token::Operator>(t.m_value) == o;
}

bool is_punct(const Token& t, token::Punctuator p) {
    return std::holds_alternative<token::Punctuator>(t.m_value) && std::get<token::Punctuator>(t.m_value) == p;
}

bool is_assign(const Token& t, token::Assignment a) {
    return std::holds_alternative<token::Assignment>(t.m_value) && std::get<token::Assignment>(t.m_value) == a;
}

bool is_ident(const Token& t, std::string_view name) {
    if (auto* id = std::get_if<token::Identifier>(&t.m_value)) {
        return id->text == name;
    }
    return false;
}

bool is_int(const Token& t, uint64_t val) {
    if (auto* lit = std::get_if<token::Literal>(&t.m_value)) {
        if (auto* i = std::get_if<token::IntLiteral>(lit)) return i->value == val;
    }
    return false;
}

bool is_float(const Token& t, double val) {
    if (auto* lit = std::get_if<token::Literal>(&t.m_value)) {
        if (auto* f = std::get_if<token::FloatLiteral>(lit)) return std::abs(f->value - val) < 1e-9;
    }
    return false;
}

bool is_str(const Token& t, std::string_view val) {
    if (auto* lit = std::get_if<token::Literal>(&t.m_value)) {
        if (auto* s = std::get_if<token::StringLiteral>(lit)) return s->raw_text == val;
    }
    return false;
}

bool is_bool(const Token& t, bool val) {
    if (auto* lit = std::get_if<token::Literal>(&t.m_value)) {
        if (auto* b = std::get_if<token::BoolLiteral>(lit)) return b->value == val;
    }
    return false;
}

bool is_null(const Token& t) {
    if (auto* lit = std::get_if<token::Literal>(&t.m_value)) {
        return std::holds_alternative<token::NullLiteral>(*lit);
    }
    return false;
}

bool is_eof(const Token& t) {
    return std::holds_alternative<token::Eof>(t.m_value);
}

// Tokenize helper expecting success
std::vector<Token> lex_ok(std::string_view source) {
    auto res = Lexer::tokenize(source);
    if (!res.has_value()) {
        throw std::runtime_error(std::format("Lexing failed unexpectedly: {}", res.error().message));
    }
    return res.value();
}

// Tokenize helper expecting error
token::Error lex_err(std::string_view source) {
    auto res = Lexer::tokenize(source);
    if (res.has_value()) {
        throw std::runtime_error("Lexing succeeded but was expected to fail.");
    }
    return res.error();
}

// --- Test Scenarios ---

void test_keywords_and_identifiers() {
    auto tokens = lex_ok("пер моя_переменная_1 = 10;");
    ASSERT(tokens.size() == 6, "Expected 6 tokens");
    ASSERT(is_kw(tokens[0], token::Keyword::Var), "Expected 'пер'");
    ASSERT(is_ident(tokens[1], "моя_переменная_1"), "Expected identifier 'моя_переменная_1'");
    ASSERT(is_assign(tokens[2], token::Assignment::Assign), "Expected '='");
    ASSERT(is_int(tokens[3], 10), "Expected integer '10'");
    ASSERT(is_punct(tokens[4], token::Punctuator::Semicolon), "Expected ';'");
    ASSERT(is_eof(tokens[5]), "Expected EOF");
}

void test_logical_operators() {
    auto tokens = lex_ok("истина и ложь или не ничто");
    ASSERT(tokens.size() == 7, "Expected 7 tokens");
    ASSERT(is_bool(tokens[0], true), "Expected true");
    ASSERT(is_op(tokens[1], token::Operator::LogAnd), "Expected LogAnd");
    ASSERT(is_bool(tokens[2], false), "Expected false");
    ASSERT(is_op(tokens[3], token::Operator::LogOr), "Expected LogOr");
    ASSERT(is_op(tokens[4], token::Operator::LogNot), "Expected LogNot");
    ASSERT(is_null(tokens[5]), "Expected null");
    ASSERT(is_eof(tokens[6]), "Expected EOF");
}

void test_numeric_literals() {
    // Dec, Hex, Oct, Bin, Dec 0, Float, Float Exp
    auto tokens = lex_ok("42 0шАБ 0в75 0д101 0 3.14 1.5э-2");
    ASSERT(tokens.size() == 8, "Expected 8 tokens");
    
    ASSERT(is_int(tokens[0], 42), "Decimal 42");
    ASSERT(is_int(tokens[1], 171), "Hex 0шАБ (10*16 + 11)");
    ASSERT(is_int(tokens[2], 61), "Octal 0в75 (7*8 + 5)");
    ASSERT(is_int(tokens[3], 5), "Binary 0д101 (1*4 + 0*2 + 1*1)");
    ASSERT(is_int(tokens[4], 0), "Decimal 0");
    ASSERT(is_float(tokens[5], 3.14), "Float 3.14");
    ASSERT(is_float(tokens[6], 0.015), "Float 1.5e-2");
    ASSERT(is_eof(tokens[7]), "Expected EOF");
}

void test_string_literals() {
    auto tokens = lex_ok("\"Привет, мир! \\н \\о \\\" \\\\\"");
    ASSERT(tokens.size() == 2, "Expected 2 tokens");
    ASSERT(is_str(tokens[0], "Привет, мир! \\н \\о \\\" \\\\"), "String raw text mismatch");
    ASSERT(is_eof(tokens[1]), "Expected EOF");
}

void test_operators_and_punctuators() {
    auto tokens = lex_ok("... -> == != << >> += -= *= /= ! ?");
    ASSERT(tokens.size() == 13, "Expected 13 tokens");
    ASSERT(std::holds_alternative<token::Spread>(tokens[0].m_value), "Expected '...'");
    ASSERT(is_punct(tokens[1], token::Punctuator::Arrow), "Expected '->'");
    ASSERT(is_op(tokens[2], token::Operator::Eq), "Expected '=='");
    ASSERT(is_op(tokens[3], token::Operator::Neq), "Expected '!='");
    ASSERT(is_op(tokens[4], token::Operator::Shl), "Expected '<<'");
    ASSERT(is_op(tokens[5], token::Operator::Shr), "Expected '>>'");
    ASSERT(is_assign(tokens[6], token::Assignment::PlusAssign), "Expected '+='");
    ASSERT(is_assign(tokens[7], token::Assignment::MinusAssign), "Expected '-='");
    ASSERT(is_assign(tokens[8], token::Assignment::StarAssign), "Expected '*='");
    ASSERT(is_assign(tokens[9], token::Assignment::SlashAssign), "Expected '/='");
    ASSERT(std::holds_alternative<token::TypeMorpheme>(tokens[10].m_value), "Expected '!'");
    ASSERT(std::holds_alternative<token::TypeMorpheme>(tokens[11].m_value), "Expected '?'");
    ASSERT(is_eof(tokens[12]), "Expected EOF");
}

void test_comments_and_whitespace() {
    auto tokens = lex_ok("  \n @ Это комментарий \n 42 @ Еще один\n");
    ASSERT(tokens.size() == 2, "Expected exactly 2 tokens (integer 42 and EOF)");
    ASSERT(is_int(tokens[0], 42), "Expected 42");
    ASSERT(tokens[0].m_loc.line == 3, "Expected 42 to be on line 3");
    ASSERT(is_eof(tokens[1]), "Expected EOF");
}

void test_lexical_errors() {
    // String errors
    auto err1 = lex_err("\"Unclosed string");
    ASSERT(err1.message.find("не был завершен") != std::string_view::npos, "Expected unclosed string error");

    auto err2 = lex_err("\"Invalid escape \\q \"");
    ASSERT(err2.message.find("Неверная экранирующая последовательность") != std::string_view::npos, "Expected invalid escape error");

    // Number constraints
    auto err3 = lex_err("05");
    ASSERT(err3.message.find("не может начинаться с нуля") != std::string_view::npos, "Expected leading zero error on int");

    auto err4 = lex_err("0ш0А");
    ASSERT(err4.message.find("не может начинаться с нуля") != std::string_view::npos, "Expected leading zero error on hex");

    auto err5 = lex_err("0д2");
    ASSERT(err5.message.find("должен начинаться с '0д1'") != std::string_view::npos, "Expected bad binary prefix error");

    auto err6 = lex_err("3.э5");
    ASSERT(err6.message.find("ожидалась цифра после точки") != std::string_view::npos , "Expected malformed float error");
}

// --- Main Execution ---

int main() {
    TestRunner runner;

    std::cout << "--- Starting Lexer Tests ---\n\n";

    runner.run("Keywords and Identifiers", test_keywords_and_identifiers);
    runner.run("Logical Operators", test_logical_operators);
    runner.run("Numeric Literals (All Bases & Floats)", test_numeric_literals);
    runner.run("String Literals and Escapes", test_string_literals);
    runner.run("Operators and Punctuators", test_operators_and_punctuators);
    runner.run("Comments and Whitespace", test_comments_and_whitespace);
    runner.run("Lexical Error Handling", test_lexical_errors);

    runner.summary();
    return 0;
}
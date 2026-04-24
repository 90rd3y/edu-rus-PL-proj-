#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <variant>

namespace token {

// Structural and Control Flow Keywords
enum class Keyword {
    Var,
    Const,
    Func,
    Struct,
    StatStruct,
    Namespace,
    If,
    Else,
    While,
    Return,
    Break,
    Continue,
    Mut,
    Include,
    From,
    Using,
    SizeOf,
    StringLen,
    As
};

// Built-in Primitive Types
enum class TypeTok {
    I8,
    I16,
    I32,
    I64,
    U8,
    U16,
    U32,
    U64,
    F32,
    F64,
    Bool,
    Str,
    DynArray,
    StatArray
};

enum class TypeMorpheme {
    MustUse,  // '!'
    Optional  // '?'
};

// Arithmetic, Bitwise, Relational, and Logical Operators
enum class Operator {
    Plus,
    Minus,
    Star,
    Slash,
    Mod,
    BitAnd,
    BitOr,
    BitXor,
    BitNot,
    Shl,
    Shr,
    Eq,
    Neq,
    Lt,
    Gt,
    Le,
    Ge,
    LogAnd,
    LogOr,
    LogNot
};

// Assignment Operations
enum class Assignment {
    Assign,
    PlusAssign,
    MinusAssign,
    StarAssign,
    SlashAssign
};

// Structural Punctuators
enum class Punctuator {
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Comma,
    Colon,
    Semicolon,
    Backslash,
    Dot,
    Arrow
};

// --- Payload Structs ---

struct Identifier {
    std::string_view text;
};

// Literals
// uint64_t accommodates all hex/octal/bin bounds
struct IntLiteral {
    uint64_t value;
};
struct FloatLiteral {
    double value;
};
struct BoolLiteral {
    bool value;
};
struct StringLiteral {
    std::string_view raw_text;  // Unescaping deferred to AST generation
                                // to avoid per-token heap allocation
};
struct NullLiteral {};  // Represents "ничто"

using Literal = std::
    variant<IntLiteral, FloatLiteral, BoolLiteral, StringLiteral, NullLiteral>;

// Standalone Tokens
struct Spread {};  // "..."
struct Eof {};     // End of file
struct Error {
    std::string_view message;
};  // Lexical error representation
}  // namespace token

struct SourceLocation {
    uint32_t line;
    uint32_t column;
    uint32_t offset;
    uint32_t length;
};

struct Token {
    using ValueVariant = std::variant<token::Identifier,
                                      token::Keyword,
                                      token::TypeTok,
                                      token::TypeMorpheme,
                                      token::Operator,
                                      token::Assignment,
                                      token::Punctuator,
                                      token::Literal,
                                      token::Spread,
                                      token::Eof,
                                      token::Error>;
    ValueVariant m_value;
    SourceLocation m_loc;
    std::string_view m_lexeme;
};
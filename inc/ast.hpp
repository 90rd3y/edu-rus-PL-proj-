#pragma once

#include "../inc/token.hpp"
#include <memory>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace ast {

// === Visitor Helper (Modern C++ std::visit idiom) ===
// Usage: std::visit(overloaded { [](const BlockStmt& b) { ... }, ... },
// stmt.variant);
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// === Forward Declarations of Wrappers ===

struct TypeNode;
using TypePtr = std::unique_ptr<TypeNode>;

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

struct Stmt;
using StmtPtr = std::unique_ptr<Stmt>;

struct TopLevelStmt;
using TopLevelStmtPtr = std::unique_ptr<TopLevelStmt>;

// ==========================================
//                 TYPES
// ==========================================

struct PrimitiveType {
    token::TypeTok primitive;
};

struct ArrayType {
    TypePtr base_type;
    bool is_static;
    // For `стат_ряд`. Literal size OR compile-time generic identifier.
    std::optional<std::variant<uint64_t, std::string_view>> static_size;
};

struct CustomType {
    std::vector<std::string_view> qualified_path;
};

using TypeVariant = std::variant<PrimitiveType, ArrayType, CustomType>;

struct TypeNode {
    TypeVariant variant;
    SourceLocation loc;
    bool is_optional = false;  // Captures the `?` morpheme
};

// ==========================================
//               EXPRESSIONS
// ==========================================

struct LiteralExpr {
    token::Literal value;
};

// Replaces IdentExprPtr with direct value semantics for cache efficiency
struct IdentifierExpr {
    std::vector<std::string_view> qualified_path;
};

struct UnaryExpr {
    token::Operator op;
    ExprPtr operand;
};

struct BinaryExpr {
    token::Operator op;
    ExprPtr left;
    ExprPtr right;
};

struct Argument {
    bool is_mut;
    ExprPtr expr;
};

struct CallExpr {
    ExprPtr target_path;  // not "IdentExpr" because of Pratt's parser
    std::vector<Argument> arguments;
};

struct IndexSliceExpr {
    ExprPtr target;
    ExprPtr index;
    ExprPtr slice_end;  // Set if slice `[index : slice_end]`
};

struct MemberAccessExpr {
    ExprPtr target;
    std::string_view member;
};

struct SpreadExpr {
    ExprPtr target;
};

struct StructInitExpr {
    ExprPtr target_path;  // not "IdentExpr" because of Pratt's parser
    std::vector<ExprPtr> fields;
};

struct ArrayInitExpr {
    std::vector<ExprPtr> elements;
};

struct CastExpr {
    TypePtr target_type;
    ExprPtr operand;
};

using ExprVariant = std::variant<LiteralExpr,
                                 IdentifierExpr,
                                 UnaryExpr,
                                 BinaryExpr,
                                 CallExpr,
                                 IndexSliceExpr,
                                 MemberAccessExpr,
                                 SpreadExpr,
                                 StructInitExpr,
                                 ArrayInitExpr,
                                 CastExpr>;

struct Expr {
    ExprVariant variant;
    SourceLocation loc;
};

// ==========================================
//      STATEMENTS & DECLARATIONS
// ==========================================

struct BlockStmt {
    std::vector<StmtPtr> statements;
};

struct IfStmt {
    ExprPtr condition;
    std::unique_ptr<BlockStmt> then_block;
    std::variant<std::monostate,
                 std::unique_ptr<BlockStmt>,
                 std::unique_ptr<IfStmt>>
        else_block;
};

struct WhileStmt {
    ExprPtr condition;
    std::unique_ptr<BlockStmt> body;
};

struct ReturnStmt {
    // nullptr strictly denotes `вернуть;` (empty).
    // `вернуть ничто;` is represented by an ExprPtr containing
    // LiteralExpr{NullLiteral}.
    ExprPtr value;
};

struct LoopControlStmt {
    bool is_break;  // true = прекратить, false = продолжить
};

struct AssignmentStmt {
    // satisfiction to <lvalue> constraints is postponed to semantics analyzer
    ExprPtr lvalue;
    token::Assignment op;
    // Supports `a = b = c;`
    std::variant<ExprPtr, std::unique_ptr<AssignmentStmt>> rvalue;
};

struct CallStmt {
    CallExpr call_expr;
};

struct VarDecl {
    bool is_const;  // true = пост, false = пер
    std::string_view name;
    TypePtr type;
    ExprPtr initializer;
};

struct StructField {
    std::string_view name;
    TypePtr type;
};

struct StructDecl {
    bool is_static;  // true = стат_структ, false = структ
    IdentifierExpr name_path;

    // std::nullopt strictly indicates a prototype (forward declaration).
    // An empty vector is a definition
    // (handled as an error in Semantic Analysis).
    std::optional<std::vector<StructField>> fields;

    [[nodiscard]] inline bool is_definition() const noexcept {
        return fields.has_value();
    }
};

struct Parameter {
    // std::nullopt allowed ONLY for prototypes
    std::optional<std::string_view> name;
    TypePtr type;
    bool is_mut;
};

struct FuncSignature {
    IdentifierExpr name_path;
    std::vector<Parameter> params;
    TypePtr return_type;  // nullptr indicates semantic 'ничто' return type
    bool must_use;        // '!' modifier
};

struct FuncDecl {
    FuncSignature signature;
    // nullptr strictly indicates a prototype
    std::unique_ptr<BlockStmt> body;

    [[nodiscard]] inline bool is_definition() const noexcept {
        return body != nullptr;
    }
};

struct TypeAlias {
    TypePtr target;
    std::string_view alias;
};

struct NamespaceExtract {
    IdentifierExpr m_id;
    bool is_wildcard;
};

struct UsingStmt {
    std::variant<TypeAlias, NamespaceExtract> stmt;
};

using StmtVariant = std::variant<BlockStmt,
                                 IfStmt,
                                 WhileStmt,
                                 ReturnStmt,
                                 LoopControlStmt,
                                 AssignmentStmt,
                                 CallStmt,
                                 VarDecl,
                                 StructDecl,
                                 FuncDecl,
                                 UsingStmt>;

struct Stmt {
    StmtVariant variant;
    SourceLocation loc;
};

// ==========================================
//        MODULES & NAMESPACES
// ==========================================

struct IncludeStmt {
    std::string_view file_path;
    // Empty = include completely
    std::vector<std::string_view> specific_imports;
};

struct NamespaceStmt {
    std::string_view name;
    std::vector<TopLevelStmtPtr> members;
};

using TopLevelVariant = std::variant<VarDecl,
                                     StructDecl,
                                     FuncDecl,
                                     IncludeStmt,
                                     UsingStmt,
                                     NamespaceStmt>;

struct TopLevelStmt {
    TopLevelVariant variant;
    SourceLocation loc;
};

// ==========================================
//                ROOT NODE
// ==========================================

struct Program {
    std::vector<TopLevelStmtPtr> top_level_stmts;
    SourceLocation loc;
};

}  // namespace ast
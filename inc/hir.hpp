#pragma once

#include "token.hpp"  // For SourceLocation and token::Assignment
#include <memory>
#include <vector>

namespace hir {

// ==========================================
//        CORE IDENTIFIERS & METADATA
// ==========================================

// Replaces all string-based paths and aliases.
// Every variable, function, and struct definition is resolved to a unique ID.
using SymbolId = uint32_t;

enum class MemoryBinding : uint8_t {
    Stack,  // Passed by strict value copy (primitives, стат_структ, стат_ряд)
    Heap    // Passed by cloned unique ownership (структ, ряд, стр)
};

// Represents the expression's capability to be mutated or assigned to.
enum class ValueCategory : uint8_t {
    LValue,  // Memory location (variables, struct fields, array elements)
    RValue  // Temporary values (literals, arithmetic results, function returns)
};

// Represents the mutability contract of a symbol or expression.
enum class Mutability : uint8_t {
    Mutable,   // "пер" or "изм"
    Immutable  // "пост" or read-only contexts (e.g., string characters)
};

// ==========================================
//                 TYPES
// ==========================================

struct TypeNode;
using TypePtr =
    std::shared_ptr<TypeNode>;  // Shared because types are heavily referenced

enum class Primitive {
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
    Void  // Represents function return 'ничто'
};

struct PrimitiveType {
    Primitive kind;
};

struct StringType {};  // 'стр' - structurally distinct from array of bytes

struct StaticArrayType {
    TypePtr base_type;
    uint64_t size;  // Evaluated to a strict compile-time constant
};

struct DynamicArrayType {
    TypePtr base_type;
};

struct StructType {
    SymbolId def_id;  // Points to the struct definition in the Symbol Table
    bool is_static;  // true = стат_структ (Stack), false = структ (Heap)
};

struct OptionalType {
    TypePtr base_type;  // Must strictly wrap a Heap-bound type
};

using TypeVariant = std::variant<PrimitiveType,
                                 StringType,
                                 StaticArrayType,
                                 DynamicArrayType,
                                 StructType,
                                 OptionalType>;

struct TypeNode {
    TypeVariant variant;
    MemoryBinding memory_binding;

    [[nodiscard]] inline bool is_optional() const noexcept {
        return std::holds_alternative<OptionalType>(variant);
    }
};

// ==========================================
//               EXPRESSIONS
// ==========================================

struct Expr;
using ExprPtr = std::unique_ptr<Expr>;

// Literals are strictly typed by the Semantic Analyzer (Contextual Typing
// applied).
struct IntLiteralExpr {
    uint64_t value;
};  // Fits all, type node dictates bounds
struct FloatLiteralExpr {
    double value;
};
struct BoolLiteralExpr {
    bool value;
};
struct StringLiteralExpr {
    std::string value;
};  // Unescaped and resolved
struct NullLiteralExpr {
};  // 'ничто' - strictly typed as OptionalType in the Expr node

// Direct reference to a resolved variable or function.
struct SymbolAccessExpr {
    SymbolId symbol;
};

// Resolved operations. The Semantic Analyzer translates generic AST operators
// into strict, type-specific HIR operations to simplify Code Generation.
enum class ResolvedBinaryOp {
    // Signed Integer Math
    IntAdd,
    IntSub,
    IntMul,
    IntDiv,
    IntMod,
    // Unsigned Integer Math
    UIntAdd,
    UIntSub,
    UIntMul,
    UIntDiv,
    UIntMod,

    // Floating Point Math
    FloatAdd,
    FloatSub,
    FloatMul,
    FloatDiv,

    // Bitwise (Identical for signed/unsigned except Shr)
    BitAnd,
    BitOr,
    BitXor,
    BitShl,
    BitShrArith,  // Signed >> (Sign-extending)
    BitShrLogic,  // Unsigned >> (Zero-filling)

    // Signed Relational
    IntEq,
    IntNeq,
    IntLt,
    IntGt,
    IntLe,
    IntGe,
    // Unsigned Relational
    UIntEq,
    UIntNeq,
    UIntLt,
    UIntGt,
    UIntLe,
    UIntGe,

    // Float Relational
    FloatEq,
    FloatNeq,
    FloatLt,
    FloatGt,
    FloatLe,
    FloatGe,

    // Logical
    BoolAnd,
    BoolOr,
    BoolEq,
    BoolNeq,

    // String & Optionals
    StringConcat,
    StringRepeat,
    StringEq,
    StringNeq,
    OptionalNullEq,
    OptionalNullNeq
};

enum class ResolvedUnaryOp { IntNeg, FloatNeg, BitNot, LogNot };

struct UnaryExpr {
    ResolvedUnaryOp op;
    ExprPtr operand;
};

struct BinaryExpr {
    ResolvedBinaryOp op;
    ExprPtr left;
    ExprPtr right;
};

// Explicit type casting (как<T>)
struct TypeCastExpr {
    ExprPtr operand;
};

// Implicit widening: T -> T? (Injected by Semantic Analyzer)
struct WrapOptionalExpr {
    ExprPtr operand;
};

// Explicit narrowing: T? -> T (Injected by Semantic Analyzer for как<T>(opt))
// Triggers runtime panic if the optional is 'ничто'.
struct UnwrapOptionalExpr {
    ExprPtr operand;
};

struct Argument {
    bool is_mut;  // 'изм' contract verified
    ExprPtr expr;
};

struct CallExpr {
    SymbolId target_func;
    std::vector<Argument> arguments;
};

struct IndexExpr {
    ExprPtr target;
    ExprPtr index;  // Verified to be an unsigned integer
};

struct StringSliceExpr {
    ExprPtr target;  // Verified to be 'стр'
    ExprPtr start_index;
    ExprPtr end_index;
};

struct MemberAccessExpr {
    ExprPtr target;
    uint32_t field_index;  // Resolved from string name to O(1) memory offset
};

struct StructInitExpr {
    SymbolId target_struct;
    std::vector<ExprPtr> fields;  // Ordered exactly as defined in the struct
};

struct ArrayInitExpr {
    std::vector<ExprPtr> elements;
};

struct SpreadExpr {
    ExprPtr target;  // Evaluated exactly once, then cloned
};

using ExprVariant = std::variant<IntLiteralExpr,
                                 FloatLiteralExpr,
                                 BoolLiteralExpr,
                                 StringLiteralExpr,
                                 NullLiteralExpr,
                                 SymbolAccessExpr,
                                 UnaryExpr,
                                 BinaryExpr,
                                 TypeCastExpr,
                                 WrapOptionalExpr,
                                 UnwrapOptionalExpr,
                                 CallExpr,
                                 IndexExpr,
                                 StringSliceExpr,
                                 MemberAccessExpr,
                                 StructInitExpr,
                                 ArrayInitExpr,
                                 SpreadExpr>;

struct Expr {
    ExprVariant variant;
    TypePtr type;
    ValueCategory category;
    MemoryBinding memory;
    Mutability mutability;
    SourceLocation loc;
};

// ==========================================
//      STATEMENTS & CONTROL FLOW
// ==========================================

struct Stmt;
using StmtPtr = std::unique_ptr<Stmt>;

struct BlockStmt {
    std::vector<StmtPtr> statements;
};

struct VarDeclStmt {
    SymbolId symbol;
    ExprPtr initializer;  // Guaranteed to exist by grammar
};

struct AssignmentStmt {
    // Verified to be ValueCategory::LValue and Mutability::Mutable
    ExprPtr lvalue;
    token::Assignment op;
    ExprPtr rvalue;
};

struct IfStmt {
    ExprPtr condition;  // Verified to be strictly 'лог'
    StmtPtr then_block;
    StmtPtr else_block;  // nullptr if no 'иначе'
};

struct WhileStmt {
    ExprPtr condition;  // Verified to be strictly 'лог'
    StmtPtr body;
};

struct ReturnStmt {
    // nullptr for 'вернуть;' (void). 'ничто' is a NullLiteralExpr.
    ExprPtr value;
};

struct LoopControlStmt {
    bool is_break;  // true = прекратить, false = продолжить
};

struct CallStmt {
    // '!' must-use verified here.
    CallExpr expr;
};

using StmtVariant = std::variant<BlockStmt,
                                 VarDeclStmt,
                                 AssignmentStmt,
                                 IfStmt,
                                 WhileStmt,
                                 ReturnStmt,
                                 LoopControlStmt,
                                 CallStmt>;

struct Stmt {
    StmtVariant variant;
    SourceLocation loc;

    // Evaluated bottom-up during Semantic Analysis.
    // true if this statement guarantees a terminal control flow
    // (return, авария, выйти).
    bool is_terminal = false;
};

// ==========================================
//        TOP-LEVEL DEFINITIONS
// ==========================================

// Note: Aliases (употреб), Namespaces (область), and Includes (включить)
// are completely dissolved into the Symbol Table during Semantic Analysis.
// They do not exist in the HIR.

struct StructFieldDef {
    TypePtr type;
    SymbolId symbol;
};

struct StructDef {
    SymbolId symbol;
    bool is_static;
    std::vector<StructFieldDef> fields;
};

struct FuncParamDef {
    SymbolId symbol;
    TypePtr type;
    bool is_mut;
};

struct FuncDef {
    SymbolId symbol;
    std::vector<FuncParamDef> params;
    TypePtr return_type;
    bool must_use;  // '!' modifier
    StmtPtr body;   // nullptr if it's an external/builtin prototype
};

// The fully resolved, semantically verified program ready for Code Generation.
struct Source {
    std::vector<StructDef> structs;
    std::vector<FuncDef> functions;
    // std::vector<GlobVar> global_vars; // have to define, assuming, that
    // initializaers can be either literals, or global vars.
    SymbolId entry_point = -1;  // ID of the 'начало' function
};

}  // namespace hir
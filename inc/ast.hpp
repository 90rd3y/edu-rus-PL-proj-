#pragma once

#include "../inc/token.hpp"
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// NOTE: restrict syntax grammar to make parser stricter
// in order to simplify semantic analyzer's work

namespace ast {

// === Forward Declarations for Visitor ===

class Program;
class TypeNode;
class PrimitiveType;
class ArrayType;
class CustomType;

class Expr;
class LiteralExpr;
class IdentifierExpr;
class UnaryExpr;
class BinaryExpr;
class CallExpr;
class IndexSliceExpr;
class MemberAccessExpr;
class StructInitExpr;
class ArrayInitExpr;
class SpreadExpr;
class CastExpr;

class Stmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
class ReturnStmt;
class LoopControlStmt;
class AssignmentStmt;
class CallStmt;

class Decl;
class VarDecl;
class StructDecl;
class FuncDecl;

class ModuleStmt;
class IncludeStmt;
class UsingStmt;
class NamespaceStmt;

// === AST Visitor Interface ===

class Visitor {
  public:
    virtual ~Visitor() = default;

    virtual void visit(const Program& node) = 0;

    // Types
    virtual void visit(const PrimitiveType& node) = 0;
    virtual void visit(const ArrayType& node) = 0;
    virtual void visit(const CustomType& node) = 0;

    // Expressions
    virtual void visit(const LiteralExpr& node) = 0;
    virtual void visit(const IdentifierExpr& node) = 0;
    virtual void visit(const UnaryExpr& node) = 0;
    virtual void visit(const BinaryExpr& node) = 0;
    virtual void visit(const CallExpr& node) = 0;
    virtual void visit(const IndexSliceExpr& node) = 0;
    virtual void visit(const MemberAccessExpr& node) = 0;
    virtual void visit(const StructInitExpr& node) = 0;
    virtual void visit(const ArrayInitExpr& node) = 0;
    virtual void visit(const SpreadExpr& node) = 0;
    virtual void visit(const CastExpr& node) = 0;

    // Statements
    virtual void visit(const BlockStmt& node) = 0;
    virtual void visit(const IfStmt& node) = 0;
    virtual void visit(const WhileStmt& node) = 0;
    virtual void visit(const ReturnStmt& node) = 0;
    virtual void visit(const LoopControlStmt& node) = 0;
    virtual void visit(const AssignmentStmt& node) = 0;
    virtual void visit(const CallStmt& node) = 0;

    // Declarations
    virtual void visit(const VarDecl& node) = 0;
    virtual void visit(const StructDecl& node) = 0;
    virtual void visit(const FuncDecl& node) = 0;

    // Modules & Namespaces
    virtual void visit(const IncludeStmt& node) = 0;
    virtual void visit(const UsingStmt& node) = 0;
    virtual void visit(const NamespaceStmt& node) = 0;
};

// === Base Node ===

class ASTNode {
  public:
    explicit ASTNode(SourceLocation loc) : m_loc(loc) {}
    virtual ~ASTNode() = default;

    virtual void accept(Visitor& visitor) const = 0;

    [[nodiscard]] const SourceLocation& location() const noexcept {
        return m_loc;
    }

  protected:
    SourceLocation m_loc;
};

// Aliases for composition
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;
using DeclPtr = std::unique_ptr<Decl>;
using TypePtr = std::unique_ptr<TypeNode>;

// === Type Nodes ===

class TypeNode : public ASTNode {
  public:
    using ASTNode::ASTNode;
    bool is_optional = false;  // captures '?'
};

class PrimitiveType : public TypeNode {
  public:
    PrimitiveType(SourceLocation loc, token::TypeTok type)
        : TypeNode(loc), primitive(type) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    token::TypeTok primitive;
};

class ArrayType : public TypeNode {
  public:
    ArrayType(SourceLocation loc, TypePtr base, bool is_static)
        : TypeNode(loc), base_type(std::move(base)), is_static(is_static) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }

    TypePtr base_type;
    bool is_static;

    // For `стат_ряд`. Variant captures literal size OR
    // compile-time generic identifier.
    std::optional<std::variant<uint64_t, std::string_view>> static_size;
};

class CustomType : public TypeNode {
  public:
    CustomType(SourceLocation loc, std::vector<std::string_view> path)
        : TypeNode(loc), qualified_path(std::move(path)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    std::vector<std::string_view>
        qualified_path;  // Identifiers for namespace resolution
};

// === Expressions ===

class Expr : public ASTNode {
  public:
    using ASTNode::ASTNode;
};

class LiteralExpr : public Expr {
  public:
    LiteralExpr(SourceLocation loc, token::Literal val)
        : Expr(loc), value(std::move(val)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    token::Literal value;
};

// <qualified_id>
class IdentifierExpr : public Expr {
    IdentifierExpr(SourceLocation loc, std::vector<std::string_view> path)
        : Expr(loc), qualified_path(std::move(path)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    std::vector<std::string_view> qualified_path;
};

class UnaryExpr : public Expr {
  public:
    UnaryExpr(SourceLocation loc, token::Operator op, ExprPtr operand)
        : Expr(loc), op(op), operand(std::move(operand)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    token::Operator op;
    ExprPtr operand;
};

class BinaryExpr : public Expr {
  public:
    BinaryExpr(SourceLocation loc,
               token::Operator op,
               ExprPtr left,
               ExprPtr right)
        : Expr(loc), op(op), left(std::move(left)), right(std::move(right)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    token::Operator op;
    ExprPtr left;
    ExprPtr right;
};

struct Argument {
    bool is_mut;   // "изм" modifier
    ExprPtr expr;  // nullptr indicates "ничто" in this context
};

using IdentExprPtr = std::unique_ptr<IdentifierExpr>;

class CallExpr : public Expr {
  public:
    CallExpr(SourceLocation loc,
             IdentExprPtr target,
             std::vector<Argument> args)
        : Expr(loc), target_path(std::move(target)),
          arguments(std::move(args)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    IdentExprPtr target_path;
    std::vector<Argument> arguments;
};

class IndexSliceExpr : public Expr {
  public:
    IndexSliceExpr(SourceLocation loc,
                   IdentExprPtr target,
                   ExprPtr index,
                   ExprPtr slice_end = nullptr)
        : Expr(loc), target(std::move(target)), index(std::move(index)),
          slice_end(std::move(slice_end)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    IdentExprPtr target;
    ExprPtr index;
    ExprPtr slice_end;  // If set, it's a slice [index : slice_end]
};

class MemberAccessExpr : public Expr {
  public:
    MemberAccessExpr(SourceLocation loc,
                     IdentExprPtr target,
                     std::string_view member)
        : Expr(loc), target(std::move(target)), member(member) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    IdentExprPtr target;
    std::string_view member;
};

class SpreadExpr : public Expr {
  public:
    SpreadExpr(SourceLocation loc, ExprPtr target)
        : Expr(loc), target(std::move(target)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    ExprPtr target;
};

class StructInitExpr : public Expr {
  public:
    StructInitExpr(SourceLocation loc,
                   IdentExprPtr target,
                   std::vector<ExprPtr> fields)
        : Expr(loc), target_path(std::move(target)), fields(std::move(fields)) {
    }

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    IdentExprPtr target_path;
    // nullptr represents "ничто" explicit passing
    std::vector<ExprPtr> fields;
};

class ArrayInitExpr : public Expr {
  public:
    ArrayInitExpr(SourceLocation loc, std::vector<ExprPtr> elements)
        : Expr(loc), elements(std::move(elements)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    std::vector<ExprPtr> elements;
};

class CastExpr : public Expr {
  public:
    CastExpr(SourceLocation loc, TypePtr target_type, ExprPtr operand)
        : Expr(loc), target_type(std::move(target_type)),
          operand(std::move(operand)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    TypePtr target_type;
    ExprPtr operand;
};

// === Statements ===

class Stmt : public ASTNode {
  public:
    using ASTNode::ASTNode;
};

class BlockStmt : public Stmt {
  public:
    BlockStmt(SourceLocation loc, std::vector<StmtPtr> stmts)
        : Stmt(loc), statements(std::move(stmts)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    std::vector<StmtPtr> statements;
};

using BlockStmtPtr = std::unique_ptr<BlockStmt>;

// maybe I should restrict syntax and parser
// in order to simplify semantic analyzer's work
class IfStmt : public Stmt {
  public:
    IfStmt(SourceLocation loc,
           ExprPtr cond,
           BlockStmtPtr then_b,
           StmtPtr else_b = nullptr)
        : Stmt(loc), condition(std::move(cond)), then_block(std::move(then_b)),
          else_block(std::move(else_b)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    ExprPtr condition;
    BlockStmtPtr then_block;
    StmtPtr
        else_block;  // Can be BlockStmt or another IfStmt (for "иначе если")
};

class WhileStmt : public Stmt {
  public:
    WhileStmt(SourceLocation loc, ExprPtr cond, BlockStmtPtr body)
        : Stmt(loc), condition(std::move(cond)), body(std::move(body)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    ExprPtr condition;
    BlockStmtPtr body;
};

class ReturnStmt : public Stmt {
  public:
    ReturnStmt(SourceLocation loc, ExprPtr val = nullptr)
        : Stmt(loc), value(std::move(val)) {
    }  // nullptr denotes `вернуть ничто;`

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    ExprPtr value;
};

class LoopControlStmt : public Stmt {
  public:
    LoopControlStmt(SourceLocation loc, bool is_break)
        : Stmt(loc), is_break(is_break) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    bool is_break;  // true = прекратить, false = продолжить
};

// Assignments strictly parsed as statements, forbidding if(a = b)
class AssignmentStmt : public Stmt {
  public:  // v~~~ postponed to semantic analyzer
    AssignmentStmt(SourceLocation loc,
                   ExprPtr lval,
                   token::Assignment op,
                   std::variant<ExprPtr, std::unique_ptr<AssignmentStmt>> rval)
        : Stmt(loc), lvalue(std::move(lval)), op(op), rvalue(std::move(rval)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    ExprPtr lvalue;  // Must syntactically map to LValue constraints
    token::Assignment op;
    // Support `a = b = c;`
    std::variant<ExprPtr, std::unique_ptr<AssignmentStmt>> rvalue;
};

using CallExprPtr = std::unique_ptr<CallExpr>;

class CallStmt : public Stmt {
  public:
    CallStmt(SourceLocation loc, CallExprPtr expr)
        : Stmt(loc), call_expr(std::move(expr)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    CallExprPtr call_expr;
};

// Declarations (Also Statements contextually)

class Decl : public Stmt {
  public:
    using Stmt::Stmt;
};

class VarDecl : public Decl {
  public:
    VarDecl(SourceLocation loc,
            bool is_const,
            std::string_view name,
            TypePtr type,
            ExprPtr init)
        : Decl(loc), is_const(is_const), name(name), type(std::move(type)),
          initializer(std::move(init)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    bool is_const;  // true = пост, false = пер
    std::string_view name;
    TypePtr type;
    ExprPtr initializer;
};

struct StructField {
    std::string_view name;
    TypePtr type;
};

// maybe <qualified_id> should be allowed
class StructDecl : public Decl {
  public:
    StructDecl(SourceLocation loc,
               bool is_static,
               std::string_view name,
               std::vector<StructField> fields)
        : Decl(loc), is_static(is_static), name(name),
          fields(std::move(fields)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    bool is_static;  // true = стат_структ, false = структ
    std::string_view name;
    std::vector<StructField> fields;
};

struct Parameter {
    bool is_mut;
    std::string_view name;
    TypePtr type;  // Might be `<param_type>`
};

class FuncDecl : public Decl {
  public:
    FuncDecl(SourceLocation loc,
             std::string_view name,
             std::vector<Parameter> params,
             TypePtr ret_type,
             bool must_use,
             BlockStmtPtr body)
        : Decl(loc), name(name), params(std::move(params)),
          return_type(std::move(ret_type)), must_use(must_use),
          body(std::move(body)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    std::string_view name;
    std::vector<Parameter> params;
    TypePtr return_type;  // nullptr indicates semantic 'ничто' return
    bool must_use;        // '!' modifier
    BlockStmtPtr body;    // nullptr for forward declarations
};

// Modules, Namespaces and Aliases

class ModuleStmt : public Stmt {
  public:
    using Stmt::Stmt;
};

class IncludeStmt : public ModuleStmt {
  public:
    IncludeStmt(SourceLocation loc,
                std::string_view file_path,
                std::vector<std::string_view> imports)
        : ModuleStmt(loc), file_path(file_path),
          specific_imports(std::move(imports)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    std::string_view file_path;
    // Empty means `включить "файл.ru"` completely
    std::vector<std::string_view> specific_imports;
};

class UsingStmt : public ModuleStmt {  // maybe have to be refactored
  public:
    UsingStmt(SourceLocation loc, std::string_view alias, TypePtr target_type)
        : ModuleStmt(loc), alias_name(alias),
          target_type(std::move(target_type)), is_namespace_wildcard(false) {}

    UsingStmt(SourceLocation loc, IdentExprPtr ns_path)
        : ModuleStmt(loc), target_namespace(std::move(ns_path)),
          is_namespace_wildcard(true) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    std::string_view alias_name;
    TypePtr target_type;  // Set if alias to Type

    // Set if alias to namespace `употреб Область\*`
    IdentExprPtr target_namespace;
    bool is_namespace_wildcard;
};

class NamespaceStmt
    : public ModuleStmt {  // restrict member type respectively to grammar
  public:
    NamespaceStmt(SourceLocation loc,
                  std::string_view name,
                  std::vector<StmtPtr> members)
        : ModuleStmt(loc), name(name), members(std::move(members)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    std::string_view name;
    std::vector<StmtPtr> members;  // Decls or other ModuleStmts
};

// Root Program Node

class Program : public ASTNode {  // restrict param type respectively to grammar
  public:
    explicit Program(SourceLocation loc, std::vector<StmtPtr> stmts)
        : ASTNode(loc), top_level_stmts(std::move(stmts)) {}

    void accept(Visitor& v) const override {
        v.visit(*this);
    }
    std::vector<StmtPtr> top_level_stmts;
};

}  // namespace ast
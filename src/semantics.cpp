#pragma once

#include "ast.hpp"
#include "hir.hpp"
#include <optional>
#include <stdexcept>

namespace sem {

struct SemanticError {
    std::string err_msg;
    SourceLocation loc;
};

template <typename T> using SemanticRes = std::expected<T, SemanticError>;

// Placeholder for the "Rib Cage" Environment
class SymbolTable {
  public:
    // TODO: [ALGORITHM: Rib Cage Scope Management]
    // - Implement a stack of hash tables (scopes).
    // - Handle push_scope() and pop_scope().
    // - Handle insert_symbol(name, type, mutability, state).
    // - Handle resolve_symbol(name) -> SemanticRes<hir::SymbolId>.
    // - Handle alias resolution (употреб).

    void push_scope() {}
    void pop_scope() {}
    SemanticRes<hir::SymbolId> resolve(std::string_view name) {
        return 0; /* stub */
    }
};

class SemanticAnalyzer {
  public:
    SemanticAnalyzer() = default;

    SemanticRes<hir::Source> analyze(const ast::Source& ast_source) {
        m_env.push_scope();  // Global scope

        // TODO:
        // Inject built-in functions (ввод, вывод, авария, etc.)
        // into the global scope here.

        for (const auto& top_stmt : ast_source.top_level_stmts) {
            auto res = visit_top_level(*top_stmt);
            if (!res) {
                m_env.pop_scope();
                return std::unexpected(res.error());
            }
        }

        // TODO:
        // If 'начало' exists, verify its signature correctness
        // and set m_hir_source.entry_point.

        m_env.pop_scope();
        return std::move(m_hir_source);
    }

  private:
    SymbolTable m_env;
    hir::Source m_hir_source;

    // Context tracking for control flow verification
    hir::TypePtr m_current_return_type;  // nullptr, if "ничто"
    int m_loop_depth = 0;

    // ==========================================
    //        TOP-LEVEL DECLARATIONS
    // ==========================================

    SemanticRes<void> visit_top_level(const ast::TopLevelStmt& stmt) {
        return std::visit(
            ast::overloaded{
                [this](const ast::VarDecl& decl) -> SemanticRes<void> {
                    // TODO: [ALGORITHM: Global Variable Initialization]
                    // 1. If decl.name is already registered in m_env,
                    // return std::unexpected(SemanticError{...}).
                    // 2. Verify decl.initializer is evaluable at compile time.
                    // 3. Resolve declared type.
                    // 4. hir::ExprPtr init = check(*decl.initializer,
                    // declared_type)
                    // 5. Store in HIR program (requires extending hir::Source
                    // to hold global vars).
                    return {};
                },
                [this](const ast::StructDecl& decl) -> SemanticRes<void> {
                    // TODO:
                    // 1. Register struct name in m_env, if there is no already.
                    // 2. If already registered and defined and current decl is
                    // redefinition,
                    //    return std::unexpected(SemanticError{...}).
                    // 3. If definition, resolve field types.
                    // 4. Build hir::StructDef and append to
                    // m_hir_source.structs.
                    return {};
                },
                [this](const ast::FuncDecl& decl) -> SemanticRes<void> {
                    // TODO:
                    // 1. Register function signature in m_env, if there is no
                    // already.
                    // 2. If definition:
                    //    a. verify it's not redefinition, else return
                    //    std::unexpected.
                    //    b. m_env.push_scope()
                    //    c. Register parameters in local scope.
                    //    d. m_current_return_type =
                    //    resolve_type(decl.signature.return_type).
                    //    e. auto body_res = visit_block(*decl.body);
                    //       if (!body_res) return
                    //       std::unexpected(body_res.error());
                    //    f. [ALGORITHM: Return Completeness]
                    //       Check if body_res.value()->is_terminal == true.
                    //       If false and return type != 'ничто', return
                    //       std::unexpected.
                    //    g. m_env.pop_scope()
                    // 3. Build hir::FuncDef and append to
                    // m_hir_source.functions.
                    return {};
                },
                [this](const ast::IncludeStmt& stmt) -> SemanticRes<void> {
                    // TODO: [ALGORITHM: Module Inclusion]
                    // 1. Parse the target file into a temporary AST (only
                    // top-level and
                    //    stmts without parsing func body or global variable
                    //    initializers, just collecting names and signatures).
                    // 2. Convert gotten AST into symbol table entries.
                    // 3. Merge exported symbols into current m_env.
                    return {};
                },
                [this](const ast::UsingStmt& stmt) -> SemanticRes<void> {
                    // TODO:
                    // Register the transparent alias in m_env. Does not produce
                    // HIR nodes.
                    return {};
                },
                [this](const ast::NamespaceStmt& stmt) -> SemanticRes<void> {
                    return visit_namespace(stmt);
                }},
            stmt.variant);
    }

    SemanticRes<void> visit_namespace(const ast::NamespaceStmt& stmt) {
        // TODO: [ALGORITHM: Namespace Scoping & Analysis]
        // 1. Push namespace name to current m_env (register it).

        m_env.push_scope();  // Entering namespace analysis mode

        for (const auto& member : stmt.members) {
            auto res = visit_top_level(*member);
            if (!res) {
                m_env.pop_scope();
                return std::unexpected(res.error());
            }
        }

        m_env.pop_scope();
        return {};
    }

    // ==========================================
    //        STATEMENTS & CONTROL FLOW
    // ==========================================

    SemanticRes<hir::StmtPtr> visit_stmt(const ast::Stmt& stmt) {
        SemanticRes<hir::StmtPtr> result = std::visit(
            ast::overloaded{
                [&](const ast::BlockStmt& s) -> SemanticRes<hir::StmtPtr> {
                    auto res = visit_block(s);
                    if (!res) return std::unexpected(res.error());
                    return std::move(res.value());
                },
                [&](const ast::VarDecl& s) -> SemanticRes<hir::StmtPtr> {
                    auto res = visit_var_decl(s);
                    if (!res) return std::unexpected(res.error());
                    return std::move(res.value());
                },
                [&](const ast::AssignmentStmt& s) -> SemanticRes<hir::StmtPtr> {
                    // TODO: [ALGORITHM: L-Value & Mutability Verification]
                    // 1. auto lval_res = infer(*s.lvalue);
                    // 2. Assert lval->category == ValueCategory::LValue.
                    // 3. Assert lval->mutability == Mutability::Mutable.
                    // 4. auto rval_res = check(*s.rvalue, lval->type);
                    // 5. Build hir::AssignmentStmt.
                    return nullptr;  // stub
                },
                [&](const ast::IfStmt& s) -> SemanticRes<hir::StmtPtr> {
                    // TODO: [ALGORITHM: Control Flow Synthesis]
                    // 1. auto cond_res = check(*s.condition, Primitive::Bool);
                    // 2. Visit then_block.
                    // 3. Visit else_block (if present).
                    // 4. is_terminal = (then_block.is_terminal &&
                    // else_block.is_terminal).
                    // 5. Build hir::IfStmt.
                    return nullptr;  // stub
                },
                [&](const ast::WhileStmt& s) -> SemanticRes<hir::StmtPtr> {
                    // 1. auto cond_res = check(*s.condition, Primitive::Bool);
                    ++m_loop_depth;
                    // 2. Visit body.
                    --m_loop_depth;
                    // 3. Build hir::WhileStmt.
                    return nullptr;  // stub
                },
                [&](const ast::ReturnStmt& s) -> SemanticRes<hir::StmtPtr> {
                    // TODO: [ALGORITHM: Return Type Matching & Dual 'ничто'
                    // Semantics]
                    // 1. If !s.value and m_current_return_type is Void -> OK.
                    // (If only one them is null, return std::unexpected)
                    // 2. If s.value is 'ничто' and m_current_return_type is
                    // Optional -> OK (Wrap).
                    // 3. Else: auto val_res = check(*s.value,
                    // *m_current_return_type).
                    // 4. Build hir::ReturnStmt. Set is_terminal = true.
                    return nullptr;  // stub
                },
                [&](const ast::LoopControlStmt& s)
                    -> SemanticRes<hir::StmtPtr> {
                    if (m_loop_depth == 0) {
                        return std::unexpected(SemanticError{
                            "Оператор управления потоком исполнения недопустим "
                            "вне тела цикла!",
                            stmt.loc});
                    }
                    // Build hir::LoopControlStmt.
                    return nullptr;  // stub
                },
                [&](const ast::CallStmt& s) -> SemanticRes<hir::StmtPtr> {
                    // TODO: [ALGORITHM: Must-Use Verification]
                    // 1. auto call_expr_res = infer(s.call_expr);
                    // 2. If call_expr points to a function with '!' modifier,
                    // return std::unexpected.
                    // 3. Build hir::CallStmt.
                    // Note: If call is 'авария' or 'выйти', set is_terminal =
                    // true.
                    return nullptr;  // stub
                }},
            stmt.variant);

        if (result && result.value()) {
            result.value()->loc = stmt.loc;
        }
        return result;
    }

    SemanticRes<std::unique_ptr<hir::BlockStmt>>
    visit_block(const ast::BlockStmt& block) {
        auto hir_block = std::make_unique<hir::BlockStmt>();
        m_env.push_scope();

        for (const auto& stmt : block.statements) {
            // TODO: [ALGORITHM: Dead Code Detection]
            if (hir_block->is_terminal) {
                m_env.pop_scope();
                return std::unexpected(SemanticError{
                    "Обнаружены недостижимые инструкции", stmt->loc});
            }

            auto stmt_res = visit_stmt(*stmt);
            if (!stmt_res) {
                m_env.pop_scope();
                return std::unexpected(stmt_res.error());
            }

            if (stmt_res.value()->is_terminal) {
                hir_block->is_terminal = true;
                // Paste the same loop, but without this check
                // (do it via macro for cleanness)
            }
            hir_block->statements.push_back(std::move(stmt_res.value()));
        }

        m_env.pop_scope();
        return hir_block;
    }

    SemanticRes<std::unique_ptr<hir::VarDeclStmt>>
    visit_var_decl(const ast::VarDecl& decl) {
        // TODO: [ALGORITHM: Variable Initialization Guarantee]
        // 0. Check, if already registered in m_env, then
        // return std::unexpected().
        // 1. Resolve declared type.
        // 2. auto init_res = check(*decl.initializer, declared_type)
        // 3. Register decl.name in m_env with checked type and mutability
        // (пост/пер).
        // 4. Build and return hir::VarDeclStmt.
        return std::make_unique<hir::VarDeclStmt>();  // stub
    }

    // ==========================================
    //        EXPRESSIONS & TYPE CHECKING
    // ==========================================

    // Top-Down: Evaluates expression enforcing an expected type context.
    SemanticRes<hir::ExprPtr> check(const ast::Expr& expr,
                                    hir::TypePtr expected_type) {
        // TODO: [ALGORITHM: Contextual Typing & Implicit Conversions]
        // 1. If expr is a Numeric Literal:
        //    - Verify literal physically fits in expected_type boundaries.
        //    - Return strictly typed hir::LiteralExpr.
        // 2. If expr is 'ничто':
        //    - Verify expected_type is OptionalType.
        //    - Return hir::NullLiteralExpr typed as expected_type.
        // 3. Otherwise, call infer(expr).
        // 4. If inferred_type == expected_type -> return inferred expr.
        // 5. If inferred_type is T and expected_type is T? -> inject
        // hir::WrapOptionalExpr.
        // 6. Else -> return std::unexpected(SemanticError{"Несоответствие
        // типов", expr.loc}).

        return nullptr;  // stub
    }

    // Bottom-Up: Evaluates expression and deduces its type natively.
    SemanticRes<hir::ExprPtr> infer(const ast::Expr& expr) {
        SemanticRes<hir::ExprPtr> result = std::visit(
            ast::overloaded{
                [&](const ast::LiteralExpr& e) -> SemanticRes<hir::ExprPtr> {
                    // TODO: [ALGORITHM: Unconstrained Defaulting]
                    // Default integers to I8/I16/I32/I64, floats to F32/F64.
                    return nullptr;  // stub
                },
                [&](const ast::IdentifierExpr& e) -> SemanticRes<hir::ExprPtr> {
                    // TODO: [ALGORITHM: Symbol Resolution]
                    // Lookup in m_env. Set ValueCategory::LValue. Set
                    // Mutability based on env.
                    return nullptr;  // stub
                },
                [&](const ast::BinaryExpr& e) -> SemanticRes<hir::ExprPtr> {
                    // TODO: [ALGORITHM: Operator Overload Resolution]
                    // 1. Infer left and right.
                    // 2. Match types to determine ResolvedBinaryOp or return
                    // std::unexpected.
                    // 3. Set ValueCategory::RValue.
                    return nullptr;  // stub
                },
                [&](const ast::CallExpr& e) -> SemanticRes<hir::ExprPtr> {
                    // TODO: [ALGORITHM: Function Call & Borrowing Rules]
                    // 1. Resolve target function signature.
                    // 2. For each argument:
                    //    a. Check against parameter type.
                    //    b. If param is 'изм', assert arg is 'изм', LValue, and
                    //    Mutable.
                    // 3. [ALGORITHM: Mutable Aliasing Rules]
                    //    - Run Structural Path Matching to ensure no two 'изм'
                    //    args alias the same memory.
                    return nullptr;  // stub
                },
                [&](const ast::MemberAccessExpr& e)
                    -> SemanticRes<hir::ExprPtr> {
                    // TODO: [ALGORITHM: Transparent Member Access]
                    // 1. Infer target.
                    // 2. If target is OptionalType -> return std::unexpected
                    // (Must unwrap first).
                    // 3. Lookup field in struct definition.
                    // 4. Inherit LValue/Mutability from target.
                    return nullptr;  // stub
                },
                [&](const ast::IndexSliceExpr& e) -> SemanticRes<hir::ExprPtr> {
                    // TODO: [ALGORITHM: Indexing and Slicing]
                    // 1. Infer target. If Optional -> Error.
                    // 2. Check index is Unsigned Integer.
                    // 3. If slice: assert target is StringType. Return RValue
                    // StringType.
                    // 4. If index: return LValue of base_type (or RValue
                    // StringType for chars).
                    return nullptr;  // stub
                },
                [&](const ast::CastExpr& e) -> SemanticRes<hir::ExprPtr> {
                    // TODO: [ALGORITHM: Explicit Casting]
                    // 1. Infer operand.
                    // 2. Validate cast legality (e.g., numeric <-> numeric, T?
                    // -> T).
                    // 3. If T? -> T, inject hir::UnwrapOptionalExpr.
                    return nullptr;  // stub
                },
                [&](const auto&) -> SemanticRes<hir::ExprPtr> {
                    return std::unexpected(SemanticError{
                        "Неопределенное выведение типа!", expr.loc});
                }},
            expr.variant);

        if (result && result.value()) {
            result.value()->loc = expr.loc;
        }
        return result;
    }
};

}  // namespace sem
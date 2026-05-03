#pragma once

#include "../../../inc/ast.hpp"
#include "../../../inc/lexer.hpp"
#include "../../../inc/parser.hpp"
#include "../../test_runner.hpp"

#include <iostream>
#include <string>
#include <string_view>
#include <variant>

// ============================================================================
//                               UTILITIES
// ============================================================================

// Helper to bridge Lexer and Parser, extracting the root AST Program.
inline std::expected<ast::Program, parser::ParseError>
parse_source(std::string_view source) {
    auto tokens_res = Lexer::tokenize(source);
    if (!tokens_res) {
        throw std::runtime_error(std::format("Lexer fatally failed: {}",
                                             tokens_res.error().message));
    }
    parser::Parser p{tokens_res.value()};
    return p.parse();
}

// ============================================================================
//                               TEST CASES
// ============================================================================

inline void test_include_stmts() {
    std::string_view source = R"(
включить "io.ru";
из "math.ru" включить синус, косинус;
)";
    auto res = parse_source(source);
    ASSERT(res.has_value(), "Failed to parse include statements");
    const auto& stmts = res->top_level_stmts;
    ASSERT(stmts.size() == 2, "Expected 2 statements");

    auto* inc1 = std::get_if<ast::IncludeStmt>(&stmts[0]->variant);
    ASSERT(inc1 != nullptr, "Stmt 0 must be IncludeStmt");
    ASSERT(inc1->file_path == "io.ru", "Stmt 0 file must be 'io.ru'");
    ASSERT(inc1->specific_imports.empty(),
           "Stmt 0 must have no specific imports");

    auto* inc2 = std::get_if<ast::IncludeStmt>(&stmts[1]->variant);
    ASSERT(inc2 != nullptr, "Stmt 1 must be IncludeStmt");
    ASSERT(inc2->file_path == "math.ru", "Stmt 1 file must be 'math.ru'");
    ASSERT(inc2->specific_imports.size() == 2,
           "Stmt 1 must have 2 specific imports");
    ASSERT(inc2->specific_imports[0] == "синус", "Stmt 1 specific import 0");
    ASSERT(inc2->specific_imports[1] == "косинус", "Stmt 1 specific import 1");
}

inline void test_using_stmts() {
    std::string_view source = R"(
употреб ц32 Возраст;
употреб ВнешняяОбласть\*;
)";
    auto res = parse_source(source);
    ASSERT(res.has_value(), "Failed to parse using statements");
    const auto& stmts = res->top_level_stmts;
    ASSERT(stmts.size() == 2, "Expected 2 statements");

    auto* using1 = std::get_if<ast::UsingStmt>(&stmts[0]->variant);
    ASSERT(using1 != nullptr, "Stmt 0 must be UsingStmt");
    auto* alias1 = std::get_if<ast::TypeAlias>(&using1->stmt);
    ASSERT(alias1 != nullptr, "Stmt 0 must be TypeAlias");
    ASSERT(alias1->alias == "Возраст", "Stmt 0 alias must be 'Возраст'");
    auto* prim1 = std::get_if<ast::PrimitiveType>(&alias1->target->variant);
    ASSERT(prim1 != nullptr && prim1->primitive == token::TypeTok::I32,
           "Stmt 0 type must be 'ц32'");

    auto* using2 = std::get_if<ast::UsingStmt>(&stmts[1]->variant);
    ASSERT(using2 != nullptr, "Stmt 1 must be UsingStmt");
    auto* ext1 = std::get_if<ast::NamespaceExtract>(&using2->stmt);
    ASSERT(ext1 != nullptr, "Stmt 1 must be NamespaceExtract");
    ASSERT(ext1->m_id.qualified_path.size() == 1 &&
               ext1->m_id.qualified_path[0] == "ВнешняяОбласть",
           "Stmt 1 path must be 'ВнешняяОбласть'");
    ASSERT(ext1->is_wildcard == true, "Stmt 1 must be wildcard '*'");
}

inline void test_namespace_stmt() {
    std::string_view source = R"(
область Физика {
    пост ГРАВИТАЦИЯ: пт64 = 9.81;
}
)";
    auto res = parse_source(source);
    ASSERT(res.has_value(), "Failed to parse namespace statement");
    const auto& stmts = res->top_level_stmts;
    ASSERT(stmts.size() == 1, "Expected 1 statement");

    auto* ns = std::get_if<ast::NamespaceStmt>(&stmts[0]->variant);
    ASSERT(ns != nullptr, "Stmt 0 must be NamespaceStmt");
    ASSERT(ns->name == "Физика", "Namespace must be 'Физика'");
    ASSERT(ns->members.size() == 1, "Namespace 'Физика' must have 1 member");
    auto* ns_var = std::get_if<ast::VarDecl>(&ns->members[0]->variant);
    ASSERT(ns_var != nullptr && ns_var->is_const == true &&
               ns_var->name == "ГРАВИТАЦИЯ",
           "Namespace member must be const 'ГРАВИТАЦИЯ'");
}

inline void test_struct_decls() {
    std::string_view source = R"(
стат_структ Точка {
    х: пт32;
    у: пт32;
}

структ Пользователь {
    имя: стр;
    возраст: Возраст;
    активен: лог;
}
)";
    auto res = parse_source(source);
    ASSERT(res.has_value(), "Failed to parse struct declarations");
    const auto& stmts = res->top_level_stmts;
    ASSERT(stmts.size() == 2, "Expected 2 statements");

    auto* st1 = std::get_if<ast::StructDecl>(&stmts[0]->variant);
    ASSERT(st1 != nullptr, "Stmt 0 must be StructDecl");
    ASSERT(st1->name_path.qualified_path[0] == "Точка",
           "Struct 1 must be 'Точка'");
    ASSERT(st1->is_static == true, "Struct 1 must be static");
    ASSERT(st1->fields->size() == 2, "Struct 1 must have 2 fields");

    auto* st2 = std::get_if<ast::StructDecl>(&stmts[1]->variant);
    ASSERT(st2 != nullptr, "Stmt 1 must be StructDecl");
    ASSERT(st2->name_path.qualified_path[0] == "Пользователь",
           "Struct 2 must be 'Пользователь'");
    ASSERT(st2->is_static == false, "Struct 2 must not be static");
    ASSERT(st2->fields->size() == 3, "Struct 2 must have 3 fields");
    auto* custom_type =
        std::get_if<ast::CustomType>(&st2->fields.value()[1].type->variant);
    ASSERT(custom_type != nullptr &&
               custom_type->qualified_path[0] == "Возраст",
           "Field 2 type must be 'Возраст'");
}

inline void test_func_decl_signature() {
    std::string_view source = R"(
функ обновить_статус: изм Пользователь п, лог новый_статус -> !лог {
    п.активен = новый_статус;
    вернуть истина;
}
)";
    auto res = parse_source(source);
    ASSERT(res.has_value(), "Failed to parse function declaration");
    const auto& stmts = res->top_level_stmts;
    ASSERT(stmts.size() == 1, "Expected 1 statement");

    auto* func1 = std::get_if<ast::FuncDecl>(&stmts[0]->variant);
    ASSERT(func1 != nullptr, "Stmt 0 must be FuncDecl");
    ASSERT(func1->signature.name_path.qualified_path[0] == "обновить_статус",
           "Func must be 'обновить_статус'");
    ASSERT(func1->signature.must_use == true, "Func must be must_use");
    ASSERT(func1->signature.params.size() == 2, "Func must have 2 params");
    ASSERT(func1->signature.params[0].is_mut == true,
           "Func param 0 must be mut");
}

inline void test_func_body_var_decls() {
    std::string_view source = R"(
функ начало: -> ц32 {
    пер т: Точка = Точка { 0.0, 1.5 };
    пер юзер: Пользователь? = Пользователь { "Иван", 30, ложь };
    вернуть 0;
}
)";
    auto res = parse_source(source);
    ASSERT(res.has_value(), "Failed to parse var decls in function body");
    const auto& stmts = res->top_level_stmts;
    ASSERT(stmts.size() == 1, "Expected 1 statement");

    auto* func2 = std::get_if<ast::FuncDecl>(&stmts[0]->variant);
    ASSERT(func2 != nullptr, "Stmt 0 must be FuncDecl");
    ASSERT(func2->body != nullptr, "Func must have a body");

    const auto& fbody = func2->body->statements;
    ASSERT(
        fbody.size() == 3,
        std::format("Func body must have 3 statements, got {}", fbody.size()));

    auto* var_t = std::get_if<ast::VarDecl>(&fbody[0]->variant);
    ASSERT(var_t != nullptr && var_t->name == "т",
           "Func stmt 0 must be var 'т'");

    auto* var_u = std::get_if<ast::VarDecl>(&fbody[1]->variant);
    ASSERT(var_u != nullptr && var_u->name == "юзер",
           "Func stmt 1 must be var 'юзер'");
    ASSERT(var_u->type->is_optional == true, "var 'юзер' must be optional");
}

inline void test_func_body_control_flow() {
    std::string_view source = R"(
функ начало: -> ц32 {
    если юзер != ничто -> {
        пер усп: лог = обновить_статус(изм юзер, истина);
        если не усп -> {
            вернуть 1;
        }
    } иначе {
        пер счетчик: ц32 = 0;
        пока счетчик < 10 -> {
            счетчик += 1;
            если счетчик == 5 -> {
                продолжить;
            }
            если счетчик == 9 -> {
                прекратить;
            }
        }
    }
    вернуть 0;
}
)";
    auto res = parse_source(source);
    if (!res.has_value()) {
        std::cerr << "Parser error in test_func_body_control_flow: "
                  << res.error().message << " at line " << res.error().loc.line
                  << ", col " << res.error().loc.column << "\n";
    }
    ASSERT(res.has_value(), "Failed to parse control flow in function body");
    const auto& stmts = res->top_level_stmts;

    auto* func2 = std::get_if<ast::FuncDecl>(&stmts[0]->variant);
    const auto& fbody = func2->body->statements;

    auto* if_stmt = std::get_if<ast::IfStmt>(&fbody[0]->variant);
    ASSERT(if_stmt != nullptr, "Func stmt 0 must be IfStmt");
    auto* else_block =
        std::get_if<std::unique_ptr<ast::BlockStmt>>(&if_stmt->else_block);
    ASSERT(else_block != nullptr, "If statement must have an else block");
}

inline void test_func_body_arrays_and_spread() {
    std::string_view source = R"(
функ начало: -> ц32 {
    пер стат_массив: стат_ряд[ц32, 5] = [ 1, 2, 3... ];
    пер дин_массив: ряд[стр] = [ "а", "б", "в" ];
    вернуть 0;
}
)";
    auto res = parse_source(source);
    ASSERT(res.has_value(), "Failed to parse arrays and spread");
    auto* func2 = std::get_if<ast::FuncDecl>(&res->top_level_stmts[0]->variant);
    const auto& fbody = func2->body->statements;

    auto* var_sarr = std::get_if<ast::VarDecl>(&fbody[0]->variant);
    ASSERT(var_sarr != nullptr && var_sarr->name == "стат_массив",
           "Func stmt 0 must be var 'стат_массив'");
    auto* sarr_type = std::get_if<ast::ArrayType>(&var_sarr->type->variant);
    ASSERT(sarr_type != nullptr && sarr_type->is_static == true,
           "var 'стат_массив' must be static ArrayType");

    auto* sarr_init =
        std::get_if<ast::ArrayInitExpr>(&var_sarr->initializer->variant);
    ASSERT(sarr_init != nullptr && sarr_init->elements.size() == 3,
           "var 'стат_массив' init must be array with 3 elements");
    auto* spread =
        std::get_if<ast::SpreadExpr>(&sarr_init->elements[2]->variant);
    ASSERT(spread != nullptr, "var 'стат_массив' element 2 must be spread");

    auto* var_darr = std::get_if<ast::VarDecl>(&fbody[1]->variant);
    ASSERT(var_darr != nullptr && var_darr->name == "дин_массив",
           "Func stmt 1 must be var 'дин_массив'");
    auto* darr_type = std::get_if<ast::ArrayType>(&var_darr->type->variant);
    ASSERT(darr_type != nullptr && darr_type->is_static == false,
           "var 'дин_массив' must be dynamic ArrayType");
}

inline void test_func_body_slice_and_cast() {
    std::string_view source = R"(
функ начало: -> ц32 {
    пер срез: стр = дин_массив[0][0:1];
    пер число: ц64 = как<ц64>(стат_массив[0]);
    вернуть 0;
}
)";
    auto res = parse_source(source);
    ASSERT(res.has_value(), "Failed to parse slice and cast");
    auto* func2 = std::get_if<ast::FuncDecl>(&res->top_level_stmts[0]->variant);
    const auto& fbody = func2->body->statements;

    auto* var_slice = std::get_if<ast::VarDecl>(&fbody[0]->variant);
    ASSERT(var_slice != nullptr && var_slice->name == "срез",
           "Func stmt 0 must be var 'срез'");
    auto* slice_expr =
        std::get_if<ast::IndexSliceExpr>(&var_slice->initializer->variant);
    ASSERT(slice_expr != nullptr, "var 'срез' init must be IndexSliceExpr");
    ASSERT(slice_expr->slice_end != nullptr, "var 'срез' init must be slice");

    auto* var_num = std::get_if<ast::VarDecl>(&fbody[1]->variant);
    ASSERT(var_num != nullptr && var_num->name == "число",
           "Func stmt 1 must be var 'число'");
    auto* cast_expr =
        std::get_if<ast::CastExpr>(&var_num->initializer->variant);
    ASSERT(cast_expr != nullptr, "var 'число' init must be CastExpr");
}

inline void test_syntax_errors() {
    // Missing semicolon
    auto res1 = parse_source("пер х: ц32 = 10");
    ASSERT(!res1.has_value(), "Parser should catch missing semicolon");

    // Empty namespace
    auto res2 = parse_source("область Пусто {}");
    ASSERT(!res2.has_value(), "Parser should reject empty namespace");

    // Invalid type modifier on primitive
    auto res3 = parse_source("пер х: ц32? = ничто;");
    ASSERT(!res3.has_value(),
           "Parser should catch invalid '?' on stack primitives");

    // Invalid struct init
    auto res4 = parse_source("пер узел: Узел = Узел{};");
    ASSERT(!res4.has_value(),
           "Parser should block empty struct initialization");
}

inline void test_deep_unary_recursion() {
    std::string source = "функ начало: -> ц32 {\n    пер переполнение: лог = ";
    for (int i = 0; i < 102; ++i) {
        source += "не ";
    }
    source += "истина;\n    вернуть 0;\n}";

    auto res = parse_source(source);
    ASSERT(res.has_value(),
           "Should parse deep recursion without stack overflow");

    auto* func = std::get_if<ast::FuncDecl>(&res->top_level_stmts[0]->variant);
    auto* var = std::get_if<ast::VarDecl>(&func->body->statements[0]->variant);

    int depth = 0;
    ast::Expr* current = var->initializer.get();
    while (auto* un_expr = std::get_if<ast::UnaryExpr>(&current->variant)) {
        ASSERT(un_expr->op == token::Operator::LogNot, "Operator must be 'не'");
        current = un_expr->operand.get();
        depth++;
    }

    ASSERT(depth == 102, "Depth of unary recursion should be exactly 102");
    auto* lit = std::get_if<ast::LiteralExpr>(&current->variant);
    ASSERT(lit != nullptr, "Deepest node must be a literal");
    auto* b_lit = std::get_if<token::BoolLiteral>(&lit->value);
    ASSERT(b_lit != nullptr && b_lit->value == true,
           "Deepest node must be 'истина'");
}

inline void test_right_associative_assignment() {
    std::string_view source =
        "функ тест_присваиваний: -> ничто {\n"
        "    п0 = п1 = п2 = п3 = п4 = п5 = п6 = п7 = п8 = п9 = п10 = п11 = п12 "
        "= п13 = п14 = п15 = п16 = п17 = п18 = п19 = п20 = п21 = п22 = п23 = "
        "п24 = п25 = 0шА;\n"
        "}";

    auto res = parse_source(source);
    ASSERT(res.has_value(),
           "Failed to parse right associative chain of assignments");

    auto* func = std::get_if<ast::FuncDecl>(&res->top_level_stmts[0]->variant);
    auto* current =
        std::get_if<ast::AssignmentStmt>(&func->body->statements[0]->variant);
    ASSERT(current != nullptr, "Statement must be an assignment");

    for (int i = 0; i <= 25; ++i) {
        auto* lval =
            std::get_if<ast::IdentifierExpr>(&current->lvalue->variant);
        ASSERT(lval != nullptr &&
                   lval->qualified_path[0] == "п" + std::to_string(i),
               "LValue name mismatch");

        if (i < 25) {
            auto* next_assign =
                std::get_if<std::unique_ptr<ast::AssignmentStmt>>(
                    &current->rvalue);
            ASSERT(next_assign != nullptr, "RValue must be an assignment");
            current = next_assign->get();
        } else {
            auto* rval_expr = std::get_if<ast::ExprPtr>(&current->rvalue);
            ASSERT(rval_expr != nullptr, "Last RValue must be an expression");
            auto* lit = std::get_if<ast::LiteralExpr>(&(*rval_expr)->variant);
            ASSERT(lit != nullptr, "Last RValue must be a literal");
        }
    }
}

inline void test_top_level_statement_volume() {
    std::string source;
    for (int i = 0; i < 200; ++i) {
        source += "структ Т { п: ц32; }\n";
    }

    auto res = parse_source(source);
    ASSERT(res.has_value(), "Failed to parse massive top-level structs");

    const auto& stmts = res->top_level_stmts;
    ASSERT(
        stmts.size() == 200,
        std::format("Expected 200 top-level statements, got {}", stmts.size()));

    for (int i = 0; i < 200; ++i) {
        auto* st = std::get_if<ast::StructDecl>(&stmts[i]->variant);
        ASSERT(st != nullptr, "Expected StructDecl");
        ASSERT(st->name_path.qualified_path[0] == "Т",
               "Struct name must be 'Т'");
        ASSERT(st->fields->size() == 1, "Struct must have 1 field");
        ASSERT(st->fields.value()[0].name == "п", "Field must be 'п'");
    }
}

inline void test_precedence_warfare() {
    std::string_view source =
        "функ тест: -> ничто {\n"
        "    пер мега_рез: лог = п0 или п1 и п2 != п3 >= п4 | п5 ^ п6 & п7 >> "
        "п8 + п9 * как<ц64>(п10).узел[42];\n"
        "}";

    auto res = parse_source(source);
    ASSERT(res.has_value(),
           std::format("Failed to parse precedence warfare: {}",
                       res.error().message));

    auto* func = std::get_if<ast::FuncDecl>(&res->top_level_stmts[0]->variant);
    auto* var = std::get_if<ast::VarDecl>(&func->body->statements[0]->variant);

    auto* or_expr = std::get_if<ast::BinaryExpr>(&var->initializer->variant);
    ASSERT(or_expr != nullptr && or_expr->op == token::Operator::LogOr,
           "Root should be LogOr");

    auto* left_or = std::get_if<ast::IdentifierExpr>(&or_expr->left->variant);
    ASSERT(left_or != nullptr && left_or->qualified_path[0] == "п0",
           "Left of LogOr should be 'п0'");

    auto* and_expr = std::get_if<ast::BinaryExpr>(&or_expr->right->variant);
    ASSERT(and_expr != nullptr && and_expr->op == token::Operator::LogAnd,
           "Right of LogOr should be LogAnd");

    auto* left_and = std::get_if<ast::IdentifierExpr>(&and_expr->left->variant);
    ASSERT(left_and != nullptr && left_and->qualified_path[0] == "п1",
           "Left of LogAnd should be 'п1'");

    auto* neq_expr = std::get_if<ast::BinaryExpr>(&and_expr->right->variant);
    ASSERT(neq_expr != nullptr && neq_expr->op == token::Operator::Neq,
           "Right of LogAnd should be Neq");

    auto* left_neq = std::get_if<ast::IdentifierExpr>(&neq_expr->left->variant);
    ASSERT(left_neq != nullptr && left_neq->qualified_path[0] == "п2",
           "Left of Neq should be 'п2'");

    auto* ge_expr = std::get_if<ast::BinaryExpr>(&neq_expr->right->variant);
    ASSERT(ge_expr != nullptr && ge_expr->op == token::Operator::Ge,
           "Right of Neq should be Ge");

    auto* left_ge = std::get_if<ast::IdentifierExpr>(&ge_expr->left->variant);
    ASSERT(left_ge != nullptr && left_ge->qualified_path[0] == "п3",
           "Left of Ge should be 'п3'");

    auto* bitor_expr = std::get_if<ast::BinaryExpr>(&ge_expr->right->variant);
    ASSERT(bitor_expr != nullptr && bitor_expr->op == token::Operator::BitOr,
           "Right of Ge should be BitOr");

    auto* left_bitor =
        std::get_if<ast::IdentifierExpr>(&bitor_expr->left->variant);
    ASSERT(left_bitor != nullptr && left_bitor->qualified_path[0] == "п4",
           "Left of BitOr should be 'п4'");

    auto* bitxor_expr =
        std::get_if<ast::BinaryExpr>(&bitor_expr->right->variant);
    ASSERT(bitxor_expr != nullptr && bitxor_expr->op == token::Operator::BitXor,
           "Right of BitOr should be BitXor");

    auto* left_bitxor =
        std::get_if<ast::IdentifierExpr>(&bitxor_expr->left->variant);
    ASSERT(left_bitxor != nullptr && left_bitxor->qualified_path[0] == "п5",
           "Left of BitXor should be 'п5'");

    auto* bitand_expr =
        std::get_if<ast::BinaryExpr>(&bitxor_expr->right->variant);
    ASSERT(bitand_expr != nullptr && bitand_expr->op == token::Operator::BitAnd,
           "Right of BitXor should be BitAnd");

    auto* left_bitand =
        std::get_if<ast::IdentifierExpr>(&bitand_expr->left->variant);
    ASSERT(left_bitand != nullptr && left_bitand->qualified_path[0] == "п6",
           "Left of BitAnd should be 'п6'");

    auto* shr_expr = std::get_if<ast::BinaryExpr>(&bitand_expr->right->variant);
    ASSERT(shr_expr != nullptr && shr_expr->op == token::Operator::Shr,
           "Right of BitAnd should be Shr");

    auto* left_shr = std::get_if<ast::IdentifierExpr>(&shr_expr->left->variant);
    ASSERT(left_shr != nullptr && left_shr->qualified_path[0] == "п7",
           "Left of Shr should be 'п7'");

    auto* plus_expr = std::get_if<ast::BinaryExpr>(&shr_expr->right->variant);
    ASSERT(plus_expr != nullptr && plus_expr->op == token::Operator::Plus,
           "Right of Shr should be Plus");

    auto* left_plus =
        std::get_if<ast::IdentifierExpr>(&plus_expr->left->variant);
    ASSERT(left_plus != nullptr && left_plus->qualified_path[0] == "п8",
           "Left of Plus should be 'п8'");

    auto* star_expr = std::get_if<ast::BinaryExpr>(&plus_expr->right->variant);
    ASSERT(star_expr != nullptr && star_expr->op == token::Operator::Star,
           "Right of Plus should be Star");

    auto* left_star =
        std::get_if<ast::IdentifierExpr>(&star_expr->left->variant);
    ASSERT(left_star != nullptr && left_star->qualified_path[0] == "п9",
           "Left of Star should be 'п9'");

    auto* index_expr =
        std::get_if<ast::IndexSliceExpr>(&star_expr->right->variant);
    ASSERT(index_expr != nullptr, "Right of Star should be IndexSliceExpr");

    auto* member_acc =
        std::get_if<ast::MemberAccessExpr>(&index_expr->target->variant);
    ASSERT(member_acc != nullptr && member_acc->member == "узел",
           "Target of IndexSliceExpr should be MemberAccessExpr '.узел'");

    auto* cast_expr = std::get_if<ast::CastExpr>(&member_acc->target->variant);
    ASSERT(cast_expr != nullptr,
           "Target of MemberAccessExpr should be CastExpr");

    auto* cast_operand =
        std::get_if<ast::IdentifierExpr>(&cast_expr->operand->variant);
    ASSERT(cast_operand != nullptr && cast_operand->qualified_path[0] == "п10",
           "Operand of CastExpr should be 'п10'");

    auto* index_lit =
        std::get_if<ast::LiteralExpr>(&index_expr->index->variant);
    ASSERT(index_lit != nullptr, "Index should be LiteralExpr");
}

inline void test_qualified_func_name_in_def() {
    auto res =
        parse_source("функ Сеть\\отправить: -> ничто {\n    вернуть;\n}");
    ASSERT(!res.has_value(),
           "Parser should catch qualified function name in definition");
    ASSERT(res.error().message.find(
               "ПутевОе имя функции недопустимо при ее определении") !=
               std::string::npos,
           std::format("Unexpected error message: {}", res.error().message));
}

inline void test_qualified_struct_name_in_def() {
    auto res = parse_source("структ Ядро\\Процесс {\n    ид: ц32;\n}");
    ASSERT(!res.has_value(),
           "Parser should catch qualified structure name in definition");
    ASSERT(res.error().message.find(
               "ПутевОе имя структуры не допустимо при ее определении") !=
               std::string::npos,
           std::format("Unexpected error message: {}", res.error().message));
}

inline void test_namespace_in_local_scope() {
    auto res = parse_source("функ тест: -> ничто {\n    область Внутренняя {\n "
                            "       пер а: ц32 = 0;\n    }\n}");
    ASSERT(!res.has_value(), "Parser should catch namespace in local scope");
    ASSERT(res.error().message.find("Ожидалось выражение") != std::string::npos,
           std::format("Unexpected error message: {}", res.error().message));
}

inline void test_assignment_where_expr_expected() {
    auto res =
        parse_source("функ тест: -> ничто {\n    если (а = 5) -> {\n       "
                     " вернуть;\n    }\n}");
    ASSERT(!res.has_value(),
           "Parser should catch assignment in expression place");
    ASSERT(res.error().message.find("Ожидалась ')' в конце выражения") !=
               std::string::npos,
           std::format("Unexpected error message: {}", res.error().message));
}

inline void test_unnamed_param_in_func_def() {
    auto res = parse_source("функ обработать: ц32 -> ничто {\n}");
    ASSERT(!res.has_value(),
           "Parser should catch unnamed parameter in function definition");
    ASSERT(res.error().message.find(
               "Безымянный параметр недопустим при определении функции") !=
               std::string::npos,
           std::format("Unexpected error message: {}", res.error().message));
}

inline void test_must_use_for_null_type() {
    auto res = parse_source("функ пинг: -> !ничто {\n}");
    ASSERT(!res.has_value(), "Parser should catch '!' applied to 'ничто'");
    ASSERT(res.error().message.find(
               "'!' неприменим к возвращаемому типу 'ничто'") !=
               std::string::npos,
           std::format("Unexpected error message: {}", res.error().message));
}

inline void test_optional_static_array() {
    auto res = parse_source("пер м: стат_ряд[ц32, 5]? = ничто;");
    ASSERT(!res.has_value(), "Parser should catch '?' applied to static array");
    ASSERT(res.error().message.find(
               "Модификатор '?' неприменим к статическим массивам") !=
               std::string::npos,
           std::format("Unexpected error message: {}", res.error().message));
}

inline void test_vla_forbidden() {
    auto res = parse_source("пер м: стат_ряд[ц32, РАЗМЕР] = 0;");
    ASSERT(!res.has_value(), "Parser should catch VLA size in static array");
    ASSERT(res.error().message.find(
               "Ожидался численный литерал в качестве размера") !=
               std::string::npos,
           std::format("Unexpected error message: {}", res.error().message));
}

inline void test_wrong_spread_operator_appliance() {
    auto res =
        parse_source("функ тест: -> ничто {\n    вывести(массив...);\n}");
    ASSERT(!res.has_value(),
           "Parser should catch wrong spread operator appliance");
    ASSERT(res.error().message.find("Ожидалась ')' после аргументов функции") !=
                   std::string::npos ||
               res.error().message.find("Ожидалось") != std::string::npos,
           std::format("Unexpected error message: {}", res.error().message));
}

inline void test_end_of_slice_forgotten() {
    auto res = parse_source("пер часть: стр = текст[1:];");
    ASSERT(!res.has_value(), "Parser should catch forgotten slice end");
    ASSERT(res.error().message.find("Ожидалось выражение") != std::string::npos,
           std::format("Unexpected error message: {}", res.error().message));
}

#include "../../inc/parser.hpp"
#include "../../inc/parser_inwards/stmt_expr_parser.hpp"

namespace parser {

// ==========================================
//           RECURSIVE DESCENT (TOP LEVEL)
// ==========================================

[[nodiscard]] ParseResult<ast::Program> Parser::parse() {
    ast::Program program;
    if (!is_at_end(state)) {
        program.loc = peek(state).m_loc;
    }

    while (!is_at_end(state)) {
        PARSE_TRY(stmt, parse_top_level_stmt(state));
        if (stmt) {  // Ignore nullptr (empty statements)
            program.top_level_stmts.push_back(std::move(stmt));
        }
    }
    return program;
}

// ==========================================
//           IMPLEMENTATIONS
// ==========================================

[[nodiscard]] ParseResult<ast::TopLevelStmtPtr>
parse_top_level_stmt(ParseState& state) {
    if (match_punct(state, token::Punctuator::Semicolon)) {
        return nullptr;
    }

    auto stmt = std::make_unique<ast::TopLevelStmt>();
    stmt->loc = peek(state).m_loc;

    if (check_keyword(state, token::Keyword::Include) ||
        check_keyword(state, token::Keyword::From)) {
        PARSE_TRY(inc, parse_include_stmt(state));
        stmt->variant = std::move(inc);
    } else if (check_keyword(state, token::Keyword::Namespace)) {
        PARSE_TRY(ns, parse_namespace_stmt(state));
        stmt->variant = std::move(ns);
    } else if (check_keyword(state, token::Keyword::Using)) {
        PARSE_TRY(us, parse_using_stmt(state));
        stmt->variant = std::move(us);
    } else if (check_keyword(state, token::Keyword::Var) ||
               check_keyword(state, token::Keyword::Const)) {
        PARSE_TRY(vd, parse_var_decl(state));
        stmt->variant = std::move(vd);
    } else if (check_keyword(state, token::Keyword::Func)) {
        PARSE_TRY(fd, parse_func_decl(state));
        stmt->variant = std::move(fd);
    } else if (check_keyword(state, token::Keyword::Struct) ||
               check_keyword(state, token::Keyword::StatStruct)) {
        PARSE_TRY(sd, parse_struct_decl(state));
        stmt->variant = std::move(sd);
    } else {
        return std::unexpected(
            ParseError{peek(state).m_loc,
                       "Ожидалось объявление или модульная инструкция в "
                       "глобальной области видимости"});
    }

    return stmt;
}

ParseResult<ast::IncludeStmt> parse_include_stmt(ParseState& state) {
    ast::IncludeStmt inc;
    if (match_keyword(state, token::Keyword::Include)) {
        auto* lit = get_if<token::Literal>(peek(state));
        if (!lit || !std::holds_alternative<token::StringLiteral>(*lit)) {
            return std::unexpected(
                ParseError{peek(state).m_loc,
                           "Ожидался строковый литерал после 'включить'"});
        }
        inc.file_path = std::get<token::StringLiteral>(*lit).raw_text;
        advance(state);
    } else if (match_keyword(state, token::Keyword::From)) {
        auto* lit = get_if<token::Literal>(peek(state));
        if (!lit || !std::holds_alternative<token::StringLiteral>(*lit)) {
            return std::unexpected(ParseError{
                peek(state).m_loc, "Ожидался строковый литерал после 'из'"});
        }
        inc.file_path = std::get<token::StringLiteral>(*lit).raw_text;
        advance(state);
        PARSE_TRY_VOID(consume_keyword(
            state,
            token::Keyword::Include,
            "Ожидалось ключевое слово 'включить' после строкового литерала"));

        do {
            PARSE_TRY(
                ident,
                consume_identifier(
                    state, "Ожидался индентификатор включаемого компонента"));
            inc.specific_imports.push_back(ident);
        } while (match_punct(state, token::Punctuator::Comma));
    }
    PARSE_TRY_VOID(consume_punct(state,
                                 token::Punctuator::Semicolon,
                                 "Ожидалась ';' после инструкции включения"));
    return inc;
}

ParseResult<ast::UsingStmt> parse_using_stmt(ParseState& state) {
    advance(state);
    ast::UsingStmt using_stmt;

    bool is_alias = true;
    auto saved_pos = state.m_pos;

    auto type_res = parse_type(state);
    if (!type_res) {
        is_alias = false;
    } else {
        auto alias_res = consume_identifier(state);
        if (!alias_res) {
            is_alias = false;
        } else {
            PARSE_TRY_VOID(
                consume_punct(state,
                              token::Punctuator::Semicolon,
                              "Ожидалась ';' в конце инструкции 'употреб'"));
            using_stmt.stmt = ast::TypeAlias{std::move(*type_res), *alias_res};
        }
    }

    if (!is_alias) {
        state.m_pos = saved_pos;
        ast::NamespaceExtract ext;
        ext.is_wildcard = false;
        ast::IdentifierExpr id;
        PARSE_TRY(ident,
                  consume_identifier(
                      state, "Ожидался индентификатор в инструкции 'употреб'"));
        id.qualified_path.push_back(ident);
        while (match_punct(state, token::Punctuator::Backslash)) {
            if (match_op(state, token::Operator::Star)) {
                ext.is_wildcard = true;
                break;
            }
            PARSE_TRY(id_part,
                      consume_identifier(state,
                                         "Ожидался индентификатор после '\\'"));
            id.qualified_path.push_back(id_part);
        }
        if (!ext.is_wildcard && match_op(state, token::Operator::Star)) {
            ext.is_wildcard = true;
        }
        ext.m_id = std::move(id);
        PARSE_TRY_VOID(
            consume_punct(state,
                          token::Punctuator::Semicolon,
                          "Ожидалась ';' в конце инструкции 'употреб'"));
        using_stmt.stmt = std::move(ext);
    }
    return using_stmt;
}

ParseResult<ast::NamespaceStmt> parse_namespace_stmt(ParseState& state) {
    PARSE_DEPTH_GUARD(state);
    advance(state);
    ast::NamespaceStmt ns;

    PARSE_TRY(name, consume_identifier(state, "Ожидалось имя области"));
    ns.name = name;

    PARSE_TRY_VOID(consume_punct(
        state, token::Punctuator::LBrace, "Ожидалась '{' после имени области"));

    if (check_punct(state, token::Punctuator::RBrace)) {
        return std::unexpected(
            ParseError{peek(state).m_loc, "Пустая область запрещена"});
    }

    while (!check_punct(state, token::Punctuator::RBrace) &&
           !is_at_end(state)) {
        PARSE_TRY(stmt, parse_top_level_stmt(state));
        if (stmt) {  // Skip empty statements
            ns.members.push_back(std::move(stmt));
        }
    }
    PARSE_TRY_VOID(
        consume_punct(state,
                      token::Punctuator::RBrace,
                      "Ожидалась '}' в конце блока определения области"));

    return ns;
}
// ==========================================
//               DECLARATIONS
// ==========================================

ParseResult<ast::VarDecl> parse_var_decl(ParseState& state) {
    ast::VarDecl var;
    if (match_keyword(state, token::Keyword::Var)) {
        var.is_const = false;
    } else if (match_keyword(state, token::Keyword::Const)) {
        var.is_const = true;
    } else {
        return std::unexpected(
            ParseError{peek(state).m_loc, "parse_var_decl: unreachable!"});
    }

    PARSE_TRY(name,
              consume_identifier(state, "Ожидалось имя переменной/постоянной"));
    var.name = name;

    PARSE_TRY_VOID(
        consume_punct(state,
                      token::Punctuator::Colon,
                      "Ожидалось ':' после имени переменной/постоянной"));
    PARSE_TRY(type, parse_type(state));
    var.type = std::move(type);

    PARSE_TRY_VOID(
        consume_assign(state,
                       token::Assignment::Assign,
                       "Ожидалось '=' для инициализации переменной"));

    PARSE_TRY(init, parse_init_value(state));
    var.initializer = std::move(init);

    PARSE_TRY_VOID(
        consume_punct(state,
                      token::Punctuator::Semicolon,
                      "Ожидалось ';' после объявления переменной/постоянной"));
    return var;
}

ParseResult<ast::StructDecl> parse_struct_decl(ParseState& state) {
    ast::StructDecl st;
    if (match_keyword(state, token::Keyword::Struct)) {
        st.is_static = false;
    } else if (match_keyword(state, token::Keyword::StatStruct)) {
        st.is_static = true;
    } else {
        return std::unexpected(
            ParseError{peek(state).m_loc, "parse_struct: unreachable!"});
    }

    auto name_loc = peek(state).m_loc;
    ast::IdentifierExpr name;
    PARSE_TRY(ident, consume_identifier(state, "Ожидалось имя структуры"));
    name.qualified_path.push_back(ident);
    while (match_punct(state, token::Punctuator::Backslash)) {
        PARSE_TRY(
            id_part,
            consume_identifier(state, "Ожидался индентификатор после '\\'"));
        name.qualified_path.push_back(id_part);
    }
    st.name_path = std::move(name);

    if (match_punct(state, token::Punctuator::Semicolon)) {
        st.fields = std::nullopt;  // Prototype
        return st;
    } else if (st.name_path.qualified_path.size() != 1) {
        return std::unexpected(ParseError{
            name_loc, "ПутевОе имя структуры не допустимо при ее определении"});
    }

    PARSE_TRY_VOID(
        consume_punct(state,
                      token::Punctuator::LBrace,
                      "Ожидалась либо '{', либо ';' после имени структуры"));
    if (check_punct(state, token::Punctuator::RBrace)) {
        return std::unexpected(
            ParseError{peek(state).m_loc, "Пустая структура запрещена"});
    }
    std::vector<ast::StructField> fields;
    while (!check_punct(state, token::Punctuator::RBrace) &&
           !is_at_end(state)) {
        ast::StructField field;
        PARSE_TRY(field_name, consume_identifier(state, "Ожидалось имя поля"));
        field.name = field_name;
        PARSE_TRY_VOID(consume_punct(
            state, token::Punctuator::Colon, "Ожидалось ':' после имени поля"));
        PARSE_TRY(field_type, parse_type(state));
        field.type = std::move(field_type);
        PARSE_TRY_VOID(consume_punct(state,
                                     token::Punctuator::Semicolon,
                                     "Ожидалась ';' в конце записи поля"));
        fields.push_back(std::move(field));
    }
    PARSE_TRY_VOID(
        consume_punct(state,
                      token::Punctuator::RBrace,
                      "Ожидалась '}' в конце определения структуры"));
    st.fields = std::move(fields);
    return st;
}

ParseResult<ast::FuncDecl> parse_func_decl(ParseState& state) {
    advance(state);
    ast::FuncDecl func;
    auto name_loc = peek(state).m_loc;
    ast::IdentifierExpr name;
    PARSE_TRY(ident, consume_identifier(state, "Ожидалось имя функции"));
    name.qualified_path.push_back(ident);
    while (match_punct(state, token::Punctuator::Backslash)) {
        PARSE_TRY(
            id_part,
            consume_identifier(state, "Ожидался индентификатор после '\\'"));
        name.qualified_path.push_back(id_part);
    }
    func.signature.name_path = std::move(name);

    PARSE_TRY_VOID(consume_punct(
        state, token::Punctuator::Colon, "Ожидалось ':' после имени функции"));

    // Parse Parameters
    if (!check_punct(state, token::Punctuator::Arrow)) {
        do {
            ast::Parameter param;
            if (match_keyword(state, token::Keyword::Mut)) {
                param.is_mut = true;
            } else {
                param.is_mut = false;
            }

            PARSE_TRY(p_type, parse_type(state, true));
            param.type = std::move(p_type);

            if (auto* id = get_if<token::Identifier>(peek(state))) {
                param.name = id->text;
                advance(state);
            } else {
                param.name = std::nullopt;
            }
            func.signature.params.push_back(std::move(param));

            if (check_punct(state, token::Punctuator::Arrow)) break;
        } while (match_punct(state, token::Punctuator::Comma));
    }

    PARSE_TRY_VOID(consume_punct(state,
                                 token::Punctuator::Arrow,
                                 "Ожидалась '->' перед возвращаемым типом"));

    if (auto* morph = get_if<token::TypeMorpheme>(peek(state));
        morph && *morph == token::TypeMorpheme::MustUse) {
        func.signature.must_use = true;
        advance(state);
    } else {
        func.signature.must_use = false;
    }

    if (auto* lit = get_if<token::Literal>(peek(state));
        lit && std::holds_alternative<token::NullLiteral>(*lit)) {
        if (func.signature.must_use) {
            return std::unexpected(
                ParseError{peek(state).m_loc,
                           "'!' неприменим к возвращаемому типу 'ничто'"});
        }
        func.signature.return_type = nullptr;
        advance(state);
    } else {
        PARSE_TRY(ret_type, parse_type(state));
        func.signature.return_type = std::move(ret_type);
    }

    if (match_punct(state, token::Punctuator::Semicolon)) {
        func.body = nullptr;
    } else {
        if (func.signature.name_path.qualified_path.size() != 1) {
            return std::unexpected(ParseError{
                name_loc,
                "ПутевОе имя функции недопустимо при ее определении"});
        }
        for (const auto& el : func.signature.params) {
            if (!el.name.has_value()) {
                return std::unexpected(ParseError{
                    el.type->loc,
                    "Безымянный параметр недопустим при определении функции"});
            }
        }
        PARSE_TRY(body, parse_block(state));
        func.body = std::move(body);
    }

    return func;
}

// ==========================================
//                  TYPES
// ==========================================

ParseResult<ast::TypePtr> parse_type(ParseState& state, bool is_param) {
    PARSE_DEPTH_GUARD(state);
    auto t_node = std::make_unique<ast::TypeNode>();
    t_node->loc = peek(state).m_loc;

    if (auto* prim = get_if<token::TypeTok>(peek(state))) {
        t_node->variant = ast::PrimitiveType{*prim};
        advance(state);
    } else if (match_keyword(state, token::Keyword::StatArray)) {
        PARSE_TRY_VOID(
            consume_punct(state,
                          token::Punctuator::LBracket,
                          "Ожидалась '[' после ключевого слова 'стат_ряд'"));
        ast::ArrayType arr;
        arr.is_static = true;
        PARSE_TRY(base_t, parse_type(state, is_param));
        arr.base_type = std::move(base_t);
        PARSE_TRY_VOID(
            consume_punct(state,
                          token::Punctuator::Comma,
                          "Ожидалась ',' внутри определения типа 'стат_ряд'"));

        if (auto* lit = get_if<token::Literal>(peek(state));
            lit && std::holds_alternative<token::IntLiteral>(*lit)) {
            arr.static_size = std::get<token::IntLiteral>(*lit).value;
            advance(state);
        } else if (is_param && get_if<token::Identifier>(peek(state))) {
            arr.static_size = get_if<token::Identifier>(peek(state))->text;
            advance(state);
        } else {
            return std::unexpected(
                ParseError{peek(state).m_loc,
                           "Ожидался численный литерал в качестве размера"});
        }
        PARSE_TRY_VOID(
            consume_punct(state,
                          token::Punctuator::RBracket,
                          "Ожидалась ']' в конце написания типа 'стат_ряд'"));
        t_node->variant = std::move(arr);
    } else if (match_keyword(state, token::Keyword::DynArray)) {
        PARSE_TRY_VOID(
            consume_punct(state,
                          token::Punctuator::LBracket,
                          "Ожидалась '[' после ключевого слова 'ряд'"));
        ast::ArrayType arr;
        arr.is_static = false;
        PARSE_TRY(base_t, parse_type(state, is_param));
        arr.base_type = std::move(base_t);
        PARSE_TRY_VOID(
            consume_punct(state,
                          token::Punctuator::RBracket,
                          "Ожидалась ']' в конце написания типа 'ряд'"));
        t_node->variant = std::move(arr);
    } else {
        ast::CustomType c_type;
        PARSE_TRY(ident, consume_identifier(state, "Ожидалось название типа"));
        c_type.qualified_path.push_back(ident);
        while (match_punct(state, token::Punctuator::Backslash)) {
            PARSE_TRY(
                id_part,
                consume_identifier(
                    state, "Ожидался индентификатор после '\\' внутри типа"));
            c_type.qualified_path.push_back(id_part);
        }
        t_node->variant = std::move(c_type);
    }

    if (auto* morph = get_if<token::TypeMorpheme>(peek(state));
        morph && *morph == token::TypeMorpheme::Optional) {
        // Enforce structural requirement that stack primitives can't use '?'
        if (std::holds_alternative<ast::PrimitiveType>(t_node->variant)) {
            auto& prim = std::get<ast::PrimitiveType>(t_node->variant);
            if (prim.primitive != token::TypeTok::Str) {
                return std::unexpected(
                    ParseError{peek(state).m_loc,
                               "Модификатор '?' применим только к строкам "
                               "('стр') и типам в куче"});
            }
        } else if (std::holds_alternative<ast::ArrayType>(t_node->variant)) {
            if (std::get<ast::ArrayType>(t_node->variant).is_static) {
                return std::unexpected(ParseError{
                    peek(state).m_loc,
                    "Модификатор '?' неприменим к статическим массивам"});
            }
        }

        t_node->is_optional = true;
        advance(state);
    }

    return t_node;
}

}  // namespace parser
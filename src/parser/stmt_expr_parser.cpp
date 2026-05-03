#include "../../inc/parser_inwards/stmt_expr_parser.hpp"
#include "../../inc/parser.hpp"

namespace parser {

// ==========================================
//               STATEMENTS
// ==========================================

[[nodiscard]] ParseResult<ast::StmtPtr> parse_statement(ParseState& state);
ParseResult<std::unique_ptr<ast::IfStmt>> parse_if(ParseState& state);
ParseResult<std::unique_ptr<ast::WhileStmt>> parse_while(ParseState& state);
ParseResult<ast::ReturnStmt> parse_return(ParseState& state);
ParseResult<ast::LoopControlStmt> parse_loop_control(ParseState& state);
ParseResult<ast::StmtPtr> parse_assignment_or_call(ParseState& state);
ParseResult<std::variant<ast::ExprPtr, std::unique_ptr<ast::AssignmentStmt>>>
parse_rhs_assignment(ParseState& state);

ParseResult<ast::ExprPtr> parse_prefix(ParseState& state);
ParseResult<ast::ExprPtr>
parse_identifier_or_struct_init(ParseState& state, const Token& first_id);
ParseResult<ast::ExprPtr> parse_infix(ParseState& state, ast::ExprPtr left);

[[nodiscard]] ParseResult<ast::StmtPtr> parse_statement(ParseState& state) {
    PARSE_DEPTH_GUARD(state);
    if (match_punct(state, token::Punctuator::Semicolon)) {
        return nullptr;
    }

    auto stmt = std::make_unique<ast::Stmt>();
    stmt->loc = peek(state).m_loc;

    if (check_punct(state, token::Punctuator::LBrace)) {
        PARSE_TRY(block, parse_block(state));
        stmt->variant = std::move(*block);
    } else if (check_keyword(state, token::Keyword::If)) {
        PARSE_TRY(if_stmt, parse_if(state));
        stmt->variant = std::move(*if_stmt);
    } else if (check_keyword(state, token::Keyword::While)) {
        PARSE_TRY(w_stmt, parse_while(state));
        stmt->variant = std::move(*w_stmt);
    } else if (check_keyword(state, token::Keyword::Return)) {
        PARSE_TRY(ret, parse_return(state));
        stmt->variant = std::move(ret);
    } else if (check_keyword(state, token::Keyword::Break) ||
               check_keyword(state, token::Keyword::Continue)) {
        PARSE_TRY(ctrl, parse_loop_control(state));
        stmt->variant = std::move(ctrl);
    } else if (check_keyword(state, token::Keyword::Using)) {
        PARSE_TRY(using_stmt, parse_using_stmt(state));
        stmt->variant = std::move(using_stmt);
    } else if (check_keyword(state, token::Keyword::Var) ||
               check_keyword(state, token::Keyword::Const)) {
        PARSE_TRY(var_decl, parse_var_decl(state));
        stmt->variant = std::move(var_decl);
    } else if (check_keyword(state, token::Keyword::Func)) {
        PARSE_TRY(func_decl, parse_func_decl(state));
        stmt->variant = std::move(func_decl);
    } else if (check_keyword(state, token::Keyword::Struct) ||
               check_keyword(state, token::Keyword::StatStruct)) {
        PARSE_TRY(struct_decl, parse_struct_decl(state));
        stmt->variant = std::move(struct_decl);
    } else {
        PARSE_TRY(assign_or_call, parse_assignment_or_call(state));
        return assign_or_call;
    }

    return stmt;
}

ParseResult<std::unique_ptr<ast::BlockStmt>> parse_block(ParseState& state) {
    PARSE_TRY_VOID(consume_punct(state, token::Punctuator::LBrace));
    auto block = std::make_unique<ast::BlockStmt>();
    while (!check_punct(state, token::Punctuator::RBrace) &&
           !is_at_end(state)) {
        PARSE_TRY(stmt, parse_statement(state));
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
    }
    PARSE_TRY_VOID(consume_punct(state,
                                 token::Punctuator::RBrace,
                                 "Ожидалась '}' в конце блока инструкций"));
    return block;
}

ParseResult<std::unique_ptr<ast::IfStmt>> parse_if(ParseState& state) {
    advance(state);
    auto if_stmt = std::make_unique<ast::IfStmt>();
    PARSE_TRY(cond, parse_expression(state, Precedence::None));
    if_stmt->condition = std::move(cond);
    PARSE_TRY_VOID(consume_keyword(
        state, token::Keyword::Then, "Ожидалось 'тогда' после условия 'если'"));
    PARSE_TRY(then_block, parse_block(state));
    if_stmt->then_block = std::move(then_block);

    if (match_keyword(state, token::Keyword::Else)) {
        if (check_keyword(state, token::Keyword::If)) {
            PARSE_TRY(else_if, parse_if(state));
            if_stmt->else_block = std::move(else_if);
        } else {
            PARSE_TRY(else_block, parse_block(state));
            if_stmt->else_block = std::move(else_block);
        }
    } else {
        if_stmt->else_block = std::monostate{};
    }
    return if_stmt;
}

ParseResult<std::unique_ptr<ast::WhileStmt>> parse_while(ParseState& state) {
    advance(state);
    auto w_stmt = std::make_unique<ast::WhileStmt>();
    PARSE_TRY(cond, parse_expression(state, Precedence::None));
    w_stmt->condition = std::move(cond);
    PARSE_TRY(body, parse_block(state));
    w_stmt->body = std::move(body);
    return w_stmt;
}

ParseResult<ast::ReturnStmt> parse_return(ParseState& state) {
    advance(state);
    ast::ReturnStmt ret;
    if (match_punct(state, token::Punctuator::Semicolon)) {
        ret.value = nullptr;  // Empty return
    } else {
        PARSE_TRY(expr, parse_expression(state, Precedence::None));
        ret.value = std::move(expr);
        PARSE_TRY_VOID(consume_punct(state,
                                     token::Punctuator::Semicolon,
                                     "Ожидалась ';' после выражения возврата"));
    }
    return ret;
}

ParseResult<ast::LoopControlStmt> parse_loop_control(ParseState& state) {
    ast::LoopControlStmt ctrl;
    if (match_keyword(state, token::Keyword::Break)) {
        ctrl.is_break = true;
    } else if (match_keyword(state, token::Keyword::Continue)) {
        ctrl.is_break = false;
    } else {
        return std::unexpected(
            ParseError{peek(state).m_loc, "parse_loop_control: unreachable"});
    }
    PARSE_TRY_VOID(
        consume_punct(state,
                      token::Punctuator::Semicolon,
                      "Ожидалась ';' после инструкции управления цикла"));
    return ctrl;
}

ParseResult<ast::StmtPtr> parse_assignment_or_call(ParseState& state) {
    PARSE_TRY(expr, parse_expression(state, Precedence::None));

    if (auto* assign_op = get_if<token::Assignment>(peek(state))) {
        auto assign_stmt = std::make_unique<ast::Stmt>();
        assign_stmt->loc = expr->loc;

        ast::AssignmentStmt a_stmt;
        a_stmt.lvalue = std::move(expr);
        a_stmt.op = *assign_op;
        advance(state);  // consume assignment operator

        PARSE_TRY(rhs, parse_rhs_assignment(state));
        a_stmt.rvalue = std::move(rhs);

        PARSE_TRY_VOID(consume_punct(state,
                                     token::Punctuator::Semicolon,
                                     "Ожидалась ';' после присваивания"));
        assign_stmt->variant = std::move(a_stmt);
        return assign_stmt;
    }

    if (std::holds_alternative<ast::CallExpr>(expr->variant)) {
        PARSE_TRY_VOID(consume_punct(state,
                                     token::Punctuator::Semicolon,
                                     "Ожидалась ';' после вызова функции"));
        auto call_stmt = std::make_unique<ast::Stmt>();
        call_stmt->loc = expr->loc;
        call_stmt->variant =
            ast::CallStmt{std::get<ast::CallExpr>(std::move(expr->variant))};
        return call_stmt;
    }

    return std::unexpected(
        ParseError{expr->loc, "Ожидалось присваивание или вызов функции"});
}

ParseResult<std::variant<ast::ExprPtr, std::unique_ptr<ast::AssignmentStmt>>>
parse_rhs_assignment(ParseState& state) {
    PARSE_DEPTH_GUARD(state);
    PARSE_TRY(expr, parse_expression(state, Precedence::None));
    if (auto* assign_op = get_if<token::Assignment>(peek(state))) {
        auto a_stmt = std::make_unique<ast::AssignmentStmt>();
        a_stmt->lvalue = std::move(expr);
        a_stmt->op = *assign_op;
        advance(state);
        PARSE_TRY(rhs, parse_rhs_assignment(state));
        a_stmt->rvalue = std::move(rhs);
        return a_stmt;
    }
    return expr;  // Base case: purely an expression
}

// ==========================================
//           PRATT PARSER (EXPRESSIONS)
// ==========================================

[[nodiscard]] ParseResult<ast::ExprPtr> parse_init_value(ParseState& state) {
    PARSE_TRY(expr, parse_expression(state, Precedence::None));

    // Check if the parsed expression is immediately followed by '...'
    if (!is_at_end(state) &&
        std::holds_alternative<token::Spread>(peek(state).m_value)) {
        advance(state);  // Consume '...'
        auto spread_expr = std::make_unique<ast::Expr>();
        spread_expr->loc = expr->loc;  // Inherit location from target
        spread_expr->variant = ast::SpreadExpr{std::move(expr)};
        return spread_expr;
    }

    return expr;
}

[[nodiscard]] ParseResult<ast::ExprPtr>
parse_expression(ParseState& state, Precedence precedence) {
    PARSE_DEPTH_GUARD(state);
    PARSE_TRY(left, parse_prefix(state));

    while (precedence < get_precedence(peek(state))) {
        PARSE_TRY(new_left, parse_infix(state, std::move(left)));
        left = std::move(new_left);
    }

    return left;
}

Precedence get_precedence(const Token& t) noexcept {
    if (auto* op = get_if<token::Operator>(t)) {
        switch (*op) {
        case token::Operator::LogOr:
            return Precedence::LogicalOr;
        case token::Operator::LogAnd:
            return Precedence::LogicalAnd;
        case token::Operator::Eq:
        case token::Operator::Neq:
            return Precedence::Equality;
        case token::Operator::Lt:
        case token::Operator::Gt:
        case token::Operator::Le:
        case token::Operator::Ge:
            return Precedence::Relational;
        case token::Operator::BitOr:
            return Precedence::BitOr;
        case token::Operator::BitXor:
            return Precedence::BitXor;
        case token::Operator::BitAnd:
            return Precedence::BitAnd;
        case token::Operator::Shl:
        case token::Operator::Shr:
            return Precedence::Shift;
        case token::Operator::Plus:
        case token::Operator::Minus:
            return Precedence::Term;
        case token::Operator::Star:
        case token::Operator::Slash:
        case token::Operator::Mod:
            return Precedence::Factor;
        default:
            return Precedence::None;
        }
    }
    if (auto* p = get_if<token::Punctuator>(t)) {
        if (*p == token::Punctuator::LParen ||
            *p == token::Punctuator::LBracket || *p == token::Punctuator::Dot) {
            return Precedence::Call;
        }
    }
    return Precedence::None;
}

ParseResult<ast::ExprPtr> parse_prefix(ParseState& state) {
    Token token = advance(state);
    auto expr = std::make_unique<ast::Expr>();
    expr->loc = token.m_loc;

    if (auto* lit = get_if<token::Literal>(token)) {
        expr->variant = ast::LiteralExpr{*lit};
        return expr;
    }

    if (std::holds_alternative<token::Identifier>(token.m_value)) {
        return parse_identifier_or_struct_init(state, token);
    }

    if (auto* punct = get_if<token::Punctuator>(token)) {
        if (*punct == token::Punctuator::LParen) {
            PARSE_TRY(inner, parse_expression(state, Precedence::None));
            PARSE_TRY_VOID(consume_punct(state,
                                         token::Punctuator::RParen,
                                         "Ожидалась ')' в конце выражения"));
            return inner;  // Grouping doesn't need its own AST node
        }
        if (*punct == token::Punctuator::LBracket) {
            if (check_punct(state, token::Punctuator::RBracket)) {
                return std::unexpected(
                    ParseError{peek(state).m_loc,
                               "Пустая инициализация массива запрещена"});
            }
            ast::ArrayInitExpr arr_init;
            do {
                PARSE_TRY(elem, parse_init_value(state));
                arr_init.elements.push_back(std::move(elem));
            } while (match_punct(state, token::Punctuator::Comma));
            PARSE_TRY_VOID(
                consume_punct(state,
                              token::Punctuator::RBracket,
                              "Ожидалась ']' в конце инициализации массива"));
            expr->variant = std::move(arr_init);
            return expr;
        }
    }

    if (auto* op = get_if<token::Operator>(token)) {
        if (*op == token::Operator::Minus || *op == token::Operator::BitNot ||
            *op == token::Operator::LogNot) {
            ast::UnaryExpr un;
            un.op = *op;
            PARSE_TRY(operand, parse_expression(state, Precedence::Unary));
            un.operand = std::move(operand);
            expr->variant = std::move(un);
            return expr;
        }
    }

    if (auto* kw = get_if<token::Keyword>(token)) {
        if (*kw == token::Keyword::As) {  // Cast: как <Type> (expr)
            PARSE_TRY_VOID(consume_op(
                state, token::Operator::Lt, "Ожидалась '<' после 'как'"));
            ast::CastExpr cast;
            PARSE_TRY(target_type, parse_type(state));
            cast.target_type = std::move(target_type);
            PARSE_TRY_VOID(
                consume_op(state,
                           token::Operator::Gt,
                           "Ожидалась '>' после типа, к которому приводят"));
            PARSE_TRY_VOID(
                consume_punct(state,
                              token::Punctuator::LParen,
                              "Ожидалась '(' перед приводимым выражением"));
            PARSE_TRY(operand, parse_expression(state, Precedence::None));
            cast.operand = std::move(operand);
            PARSE_TRY_VOID(
                consume_punct(state,
                              token::Punctuator::RParen,
                              "Ожидалась ')' после приводимого выражения"));
            expr->variant = std::move(cast);
            return expr;
        }
    }

    return std::unexpected(ParseError{token.m_loc, "Ожидалось выражение"});
}

ParseResult<ast::ExprPtr>
parse_identifier_or_struct_init(ParseState& state, const Token& first_id) {
    auto expr = std::make_unique<ast::Expr>();
    expr->loc = first_id.m_loc;

    ast::IdentifierExpr ident;
    ident.qualified_path.push_back(
        std::get<token::Identifier>(first_id.m_value).text);

    while (match_punct(state, token::Punctuator::Backslash)) {
        PARSE_TRY(
            id_part,
            consume_identifier(state, "Ожидался индентификатор после '\\'"));
        ident.qualified_path.push_back(id_part);
    }

    if (match_punct(state, token::Punctuator::LBrace)) {
        // Enforce semantic rule: empty struct init is prohibited
        if (check_punct(state, token::Punctuator::RBrace)) {
            return std::unexpected(ParseError{
                expr->loc, "Пустая инициализация структуры запрещена"});
        }

        ast::StructInitExpr s_init;
        auto target = std::make_unique<ast::Expr>();
        target->loc = expr->loc;
        target->variant = std::move(ident);
        s_init.target_path = std::move(target);

        do {
            PARSE_TRY(field_expr, parse_init_value(state));
            s_init.fields.push_back(std::move(field_expr));
        } while (match_punct(state, token::Punctuator::Comma));

        PARSE_TRY_VOID(
            consume_punct(state,
                          token::Punctuator::RBrace,
                          "Ожидалась '}' в конце инициализации структуры"));
        expr->variant = std::move(s_init);
        return expr;
    }

    expr->variant = std::move(ident);
    return expr;
}

ParseResult<ast::ExprPtr> parse_infix(ParseState& state, ast::ExprPtr left) {
    Token token = advance(state);
    auto expr = std::make_unique<ast::Expr>();
    expr->loc = left->loc;

    if (auto* op = get_if<token::Operator>(token)) {
        ast::BinaryExpr bin;
        bin.op = *op;
        bin.left = std::move(left);
        PARSE_TRY(right_expr, parse_expression(state, get_precedence(token)));
        bin.right = std::move(right_expr);
        expr->variant = std::move(bin);
        return expr;
    }

    if (auto* punct = get_if<token::Punctuator>(token)) {
        if (*punct == token::Punctuator::LParen) {  // Call
            ast::CallExpr call;
            call.target_path = std::move(left);
            if (!check_punct(state, token::Punctuator::RParen)) {
                do {
                    ast::Argument arg;
                    if (match_keyword(state, token::Keyword::Mut)) {
                        arg.is_mut = true;
                    } else {
                        arg.is_mut = false;
                    }
                    PARSE_TRY(arg_expr,
                              parse_expression(state, Precedence::None));
                    arg.expr = std::move(arg_expr);
                    call.arguments.push_back(std::move(arg));
                } while (match_punct(state, token::Punctuator::Comma));
            }
            PARSE_TRY_VOID(
                consume_punct(state,
                              token::Punctuator::RParen,
                              "Ожидалась ')' после аргументов функции"));
            expr->variant = std::move(call);
            return expr;
        }

        if (*punct == token::Punctuator::LBracket) {  // Index or Slice
            ast::IndexSliceExpr idx;
            idx.target = std::move(left);
            PARSE_TRY(index_expr, parse_expression(state, Precedence::None));
            idx.index = std::move(index_expr);
            if (match_punct(state, token::Punctuator::Colon)) {
                PARSE_TRY(slice_end_expr,
                          parse_expression(state, Precedence::None));
                idx.slice_end = std::move(slice_end_expr);
            }
            PARSE_TRY_VOID(
                consume_punct(state,
                              token::Punctuator::RBracket,
                              "Ожидалась ']' в конце индекса/среза"));
            expr->variant = std::move(idx);
            return expr;
        }

        if (*punct == token::Punctuator::Dot) {  // Member Access
            ast::MemberAccessExpr acc;
            acc.target = std::move(left);
            PARSE_TRY(
                member_id,
                consume_identifier(state, "Ожидалось имя поля после '.'"));
            acc.member = member_id;
            expr->variant = std::move(acc);
            return expr;
        }
    }

    return std::unexpected(
        ParseError{token.m_loc, "Непредвиденный токен в инфиксном выражении"});
}
}  // namespace parser
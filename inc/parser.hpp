#pragma once

#include "parser_inwards/parse_utilities.hpp"

namespace parser {

[[nodiscard]] ParseResult<ast::TopLevelStmtPtr>
parse_top_level_stmt(ParseState& state);
ParseResult<ast::IncludeStmt> parse_include_stmt(ParseState& state);
ParseResult<ast::UsingStmt> parse_using_stmt(ParseState& state);
ParseResult<ast::NamespaceStmt> parse_namespace_stmt(ParseState& state);
ParseResult<ast::VarDecl> parse_var_decl(ParseState& state);
ParseResult<ast::StructDecl> parse_struct_decl(ParseState& state);
ParseResult<ast::FuncDecl> parse_func_decl(ParseState& state);
ParseResult<ast::TypePtr> parse_type(ParseState& state, bool is_param = false);

class Parser {
    ParseState state;

  public:
    explicit Parser(const std::vector<Token>& tokens) : state{tokens, 0} {}

    [[nodiscard]] ParseResult<ast::Program> parse();
};

}  // namespace parser

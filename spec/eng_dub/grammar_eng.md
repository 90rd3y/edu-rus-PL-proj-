# Language Grammar (EBNF)

This document defines the formal lexical and syntactic grammar of the language.
It uses BNF structural conventions (`<name> ::=`) supplemented with grouping
`( )`, optional `[ ]`, and repetition `{ }` (zero or more) expressions.

## 1. Lexical Grammar

@ (_Being referred by_: `language_specification.md` paragraph 1
@ "Core Syntax & Structure" item "Reserved Keywords")

```
<letter> ::= "А" | ... | "Я" | "а" | ... | "я" | "_"

<binary_digit> ::= "0" | "1"
<octal_digit> ::= "0" | ... | "7"
<digit> ::= "0" | ... | "9"
<hex_digit_except_zero> ::= "1" | ... | "9" | "А" | ... | "Е"
<hex_digit> ::= <digit> | "А" | ... | "Е"
@ Cyrillic hex digit mapping: А=10, Б=11, В=12, Г=13, Д=14, Е=15

<id> ::= <letter> { <letter> | <digit> }
@ Constraint: <id> must not match any <keyword> or globally reserved
@ built-in function name (e.g., ввод, вывод, заполнить). Longest token wins.

<keyword> ::= "пер" | "пост" | "функ" | "структ" | "стат_структ" |
              "область" | "если" | "иначе" | "пока" | "вернуть" |
              "прекратить" | "продолжить" | "изм" |
              "включить" | "из" | "употреб" |
              "истина" | "ложь" | "ничто" |
              "и" | "или" | "не" |
              "ц8" | "ц16" | "ц32" | "ц64" |
              "ц8б" | "ц16б" | "ц32б" | "ц64б" |
              "пт32" | "пт64" | "лог" | "стр" |
              "ряд" | "стат_ряд" | "как"

<comment> ::= "@" { <any_char_except_newline> }

<dec_literal> ::= ("1" | ... | "9") { <digit> }
<hex_literal> ::= "0ш" <hex_digit_except_zero> { <hex_digit> }
<oct_literal> ::= "0в" ("1" | ... | "7") { <octal_digit> }
<bin_literal> ::= "0д1" { <binary_digit> }
<nonzero_int_literal> ::= <dec_literal> | <hex_literal> | <oct_literal> |
<bin_literal>
<int_literal> ::= "0" | <nonzero_int_literal>

<float_literal> ::= ("0" | <dec_literal>) "." <digit> {<digit>}
[ "э" [ "-" ] <dec_literal> ]
<bool_literal> ::= "истина" | "ложь"
<escape_seq> ::= "\" ( '"' | "\" | "н" | "о" | "0" )
<string_literal> ::= '"' { <any_char_except_quote_backslash_newline> |
<escape_seq> } '"'

<literal> ::= <int_literal> | <float_literal> | <bool_literal> |
<string_literal>

<spread_token> ::= "..."
```

@ --- Lexical Constraints & Formatting ---

@ String Escape Sequences:
@ The following are the ONLY recognized
@ escape sequences inside<string_literal>:
@ \" — literal quote character
@ \\ — literal backslash
@ \н — newline
@ \о — tabulation (fixed-width offset whitespace)
@ \0 — null byte
@ Any other character following '\' inside a string literal is a compile error.

@ Numeric Literal Constraints:
@ Binary minimal literal is `0д1` (value 1).
@ Binary, octal or hexadecimal zero has no
@ no dedicated different from `0`.
@ Floating-point scientific notation examples: `1.5e10`, `3.14e-2`.

## 2. Syntactic Grammar

### 2.1 Program Structure

```
<program> ::= { <top_level_stmt> }
<top_level_stmt> ::= <declaration> | <module_stmt> | ";"
<statement> ::= <var_decl> | <using_stmt> | <assignment_stmt> | <control_flow> |
<call_stmt> | <block_stmt> | ";"

<block_stmt> ::= "{" { <statement> } "}"
```

### 2.2 Modules, Namespaces and Aliases

@ (_Being referred by_: `semantics.md` paragraph 7
@ "Name Resolution, Scoping & Modules" items:
@ "Type Aliasing", "Namespaces (`область`)", "Module Inclusion")

```
<module_stmt> ::= <include_stmt> | <namespace_stmt> | <using_stmt>
<include_stmt> ::= "включить" <string_literal> ";" |
"из" <string_literal> "включить" <id> { "," <id> } ";"

<using_stmt> ::= "употреб" ( <type> <id> | <qualified_id> [ "\" "*" ] ) ";"

<namespace_stmt> ::= "область" <id> "{" <namespace_member>
{ <namespace_member> } "}"
<namespace_member> ::= <declaration> | <module_stmt>
```

### 2.3 Types

@ (_Being referred by_: `types.md` paragraph 1 "Type Declaration Syntax",
@ `semantics.md` paragraph 4.B "Void, Nullability and dual essence of `ничто`")

```
<primitive> ::= "ц8" | "ц16" | "ц32" | "ц64" | "ц8б" | "ц16б" | "ц32б" |
"ц64б" | "пт32" | "пт64" | "лог"

<type> ::= <primitive> | "стат_ряд" "[" <type> "," <nonzero_int_literal> "]" |
("стр" | <qualified_id> | "ряд" "[" <type> "]") [ "?" ]

@ <param_type>: variant of <type> for function parameters only.
@ Allows a user-named <id> as the стат_ряд size
@ (compile-time captured constant).
<param_type> ::= <primitive> |
"стат_ряд" "[" <param_type> "," (<nonzero_int_literal> | <id>) "]" |
("стр" | <qualified_id> | "ряд" "[" <param_type> "]") [ "?" ]
```

### 2.4 Declarations

@ (_Being referred by_: `types.md` paragraph 1 "Type Declaration Syntax",
@ `semantics.md` paragraphes:
@ 1 "Execution Model" item "Must-Use Return Values (`!` Modifier)",
@ 2 "Memory & Assignment (The Golden Rules)"
@ item "Variable Initialization Rules" subitem "Initialization Guarantee")

```
<declaration> ::= <var_decl> | <func_decl> | <struct_decl>

<var_decl> ::= ("пер" | "пост") <id> ":" <type> "="
("ничто" | <spread_init> | <expr>) ";"

<struct_decl> ::= ("структ" | "стат_структ")
(<id> "{" <struct_field> { <struct_field> } "}" | <qualified_id> ";")
<struct_field> ::= <id> ":" <type> ";"

<func_decl> ::= "функ" (<id> ":" [ <param_list> ] "->" <ret_type> <block_stmt> |
<qualified_id> ":" [ <param_proto_list> ] "->" <ret_type> ";")
<ret_type> ::= [ "!" ] <type> | "ничто"
<param_proto_list> ::= <param_proto> {"," <param_proto>}
<param_proto> ::= [ "изм" ] <param_type> [ <id> ]
<param_list> ::= <param> { "," <param> }
<param> ::= [ "изм" ] <param_type> <id>
```

### 2.5 Control Flow & Statements

```
<control_flow> ::= <if_stmt> | <while_stmt> | <loop_control> | <return_stmt>

<loop_control> ::= ("прекратить" | "продолжить") ";"

<if_stmt> ::= "если" <expr> "->" <block_stmt>
[ "иначе" ( <block_stmt> | <if_stmt> ) ]
<while_stmt> ::= "пока" <expr> "->" <block_stmt>

<return_stmt> ::= "вернуть" [ "ничто" | <expr> ] ";"

<lvalue> ::= <qualified_id> { "." <id> | "[" <expr> "]" }
<assignment> ::= <lvalue> "=" ("ничто" | <assignment> | <expr>) |
<lvalue> ("+=" | "-=" | "*=" | "/=") (<assignment> | <expr>)

<assignment_stmt> ::= <assignment> ";"
<call_stmt> ::= <qualified_id> "(" [ <arg_list> ] ")" ";"
```

### 2.6 Expressions

@ (_Being referred by_: `semantics.md`:
@ paragraph 2 "Memory & Assignment (The Golden Rules)"
@ item "Spread Operator (`...`) Evaluation",
@ paragraph 4.A "Type Conversions (Casting)" item "Explicit Casts",
@ `types.md` paragraph 6 "Type Conversions (Casting) Syntax and Rules")

```
<expr> ::= <logical_or>

<logical_or> ::= <logical_and> { "или" <logical_and> }
<logical_and> ::= <equality> { "и" <equality> }

<equality> ::= <relational> [ ("==" | "!=") ( <relational> | "ничто" ) ] |
"ничто" ("==" | "!=") <relational>
<relational> ::= <bitwise_or> [ ("<" | ">" | "<=" | ">=") <bitwise_or> ]

<bitwise_or> ::= <bitwise_xor> { "|" <bitwise_xor> }
<bitwise_xor> ::= <bitwise_and> { "^" <bitwise_and> }
<bitwise_and> ::= <bitwise_shift> { "&" <bitwise_shift> }
<bitwise_shift> ::= <term> { ("<<" | ">>") <term> }

<term> ::= <factor> { ("+" | "-") <factor> }
<factor> ::= <unary> { ("*" | "/" | "%") <unary> }

<unary> ::= ("-" | "~" | "не") <unary> | <primary>

<primary> ::= <literal> | <struct_init> | <array_init> |
(["как" "<" <type> ">"] "(" <expr> ")" | <qualified_id>) { <postfix> }

<postfix> ::= "(" [ <arg_list> ] ")" | "." <id> | "[" <expr> [ ":" <expr> ] "]"
<arg_list> ::= <arg> { "," <arg> }
<arg> ::= "ничто" | [ "изм" ] <expr>

<qualified_id> ::= <id> { "\\" <id> }
<struct_init> ::= <qualified_id> "{" ("ничто" | <expr> | <spread_init>)
{ "," ("ничто" | <expr> | <spread_init>) } "}"
<array_init> ::= "[" ("ничто" | <expr> | <spread_init>)
{ "," ("ничто" | <expr> | <spread_init>) } "]"
<spread_init> ::= <expr> <spread_token>
```

### 2.7 Operator Precedence and Associativity

@ (_Being referred by_: `language_specification.md` paragraph 7
@ "Expressions and Operators",
@ `semantics.md` paragraph 5.B "Expression and Operators semantics"
@ item "Operator precedence")

Operators are evaluated from highest precedence (Level 1) to lowest (Level 13).
Operators with the same precedence are evaluated strictly according to
associativity.

- **Level 1** (Left-to-Right): `()`, `[]`, `.`, `\`
  - Grouping, Index / Slice, Member Access, Namespace Resolution.
- **Level 2** (Right-to-Left): `-`, `~`, `не`, `как<T>()`
  - Unary Negation, Bitwise NOT, Logical NOT, Type Cast.
- **Level 3** (Left-to-Right): `*`, `/`, `%`
  - Multiplication (Overloaded for `стр * N`), Division, Modulo.
- **Level 4** (Left-to-Right): `+`, `-`
  - Addition (Overloaded for `стр + стр`), Subtraction.
- **Level 5** (Left-to-Right): `<<`, `>>`
  - Bitwise Left Shift, Bitwise Right Shift.
- **Level 6** (Left-to-Right): `&`
  - Bitwise AND.
- **Level 7** (Left-to-Right): `^`
  - Bitwise XOR.
- **Level 8** (Left-to-Right): `|`
  - Bitwise OR.
- **Level 9** (Non-Associative): `<`, `<=`, `>`, `>=`
  - Relational Comparisons.
- **Level 10** (Non-Associative): `==`, `!=`
  - Equality / Inequality Comparisons.
- **Level 11** (Left-to-Right): `и`
  - Logical AND.
- **Level 12** (Left-to-Right): `или`
  - Logical OR.
- **Level 13** (Right-to-Left): `=`, `+=`, `-=`, `*=`, `/=`
  - Assignments (`+=`, `*=` overloaded for `стр`).

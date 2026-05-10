# Language Semantics

This document outlines the core operational semantics and execution rules of
the MVP language.

## 1. Execution Model

- **Sequential Execution:** The program statements execute sequentially unless
  interrupted by control flow constructs
  (`если`, `пока`, `вернуть`, `выйти`, `авария`).

- **Evaluation Order:** Expressions, function arguments, and operands are
  strictly evaluated **Left-to-Right**.

- **Entry Point:** Execution begins exactly at the `начало` function. The
  compiler strictly recognizes two signatures:
  1. `функ начало: ряд[стр] аргументы -> ц32 { ... }`
  2. `функ начало: -> ц32 { ... }` (No arguments. Silently ignores CLI args).
    _(The compiler maps this to C++ `int main`._
    _The return value dictates the OS exit code)._

- **Panic/Abort:** Any invalid operation (e.g., out-of-bounds, invalid cast, or
  авария) triggers a fatal, uncatchable panic. It strictly performs C++ stack
  unwinding to free memory and flush I/O, then exits.

- **Error Handling Strategy:** The language provides no exception or unwinding
  mechanism. Errors follow two patterns:
  - **Recoverable:** Functions signal failure through return values
    (e.g., `лог` for success/failure, `стр?` for operations that may fail).
    The caller is responsible for checking.
  - **Fatal:** Irrecoverable errors are handled via `авария(сообщение)` or
    automatic runtime panics (out-of-bounds, corrupting casts).
    These terminate immediately.

- **Must-Use Return Values (`!` Modifier):** To enforce robust error checking,
  a function's return type can be prefixed with `!` (e.g., `-> !лог`).
  _(See `grammar.md` paragraph 2.4 "Declarations" item "<ret_type>")_
  If a function with a `!` return type is called, and its result is
  not assigned to a variable and not used directly in a condition/expression
  (i.e., the value is discarded), the compiler's semantic analyzer triggers a
  **hard compile error**.

## 2. Memory & Assignment (The Golden Rules)

(_Being referred by_: `language_specification.md` paragraph 3
"Type System Overview")

To keep the compiler simple and eliminate ambiguity, a type's physical memory
location is permanently tied to its definition keyword. The language is built on
a "Golden Rule" of unique ownership.

- **The Golden Rule of Assignment:** Assignment (`=`) transfers ownership via
  strict deep copy, securing logical independence. The compiler optimizes this
  to a "move" for temporaries (e.g. function return value).

- **Values vs. Unique Ownership:**
  - **Stack-Bound Types (Passed by strict Value copy):**
    These types live directly on the stack.
    _(See `types.md` paragraph 2 "Stack-Bound Types (Passed by Value)")_
  - **Heap-Bound Types (Passed by cloned Unique Ownership):**
    These types live exclusively on the heap. A completely new
    heap allocation is performed on assignment, cloning contents entirely.
    _(See `types.md` paragraph 3_
    _"Heap-Bound Types (Passed by Unique Ownership/Borrow)")_

- **Variable Initialization Rules:**
  - **Initialization Guarantee:**
    Due to `grammar.md` constraints
    _(See `grammar.md` paragraph 2.4 "Declarations" item "<var_decl>")_,
    uninitialized variables cannot exist. Every declaration must include an
    explicit initialization assignment, guaranteeing deterministic memory states.
  - **Global variable initialization expression** must be
    evaluable at compile time.

- **Spread Operator (`...`) Evaluation:** When using the spread syntax
  _(See `grammar.md`:_
  _paragraph 2.4 "Declarations" item "<var_decl>"_,
  _paragraph 2.6 "Expressions" items:_
    _"<spread_init>", "<struct_init>")_
  (e.g., `<expr>...` in static array literal filling),
  the `<expr>` is evaluated **exactly once**.
  The resulting value is then cloned/copied across the entire array.

- **Limitations:**
  - _No Cyclical Graphs:_ Strict unique ownership mathematically
    prohibits cyclical graphs.
  - _Hardware Limitation on Deep Recursion:_ Deeply nested object destruction
    and assignment may trigger OS stack overflow (accepted MVP limitation).

## 3. Borrowing & Mutability

Variables cannot hold or store references. Sharing state without copying is
only possible via function parameters:

- **Pass-by-value** (Primitives without `изм`):
  Cloned upon entering the function (maximum performance).

- **Read-only borrow** (Complex types without `изм`):
  Passed as a constant reference. Mutations by the function are strictly
  prohibited by the compiler.

- **Mutable borrow** (`изм`):
  Passed as a direct reference. The contract to mutate must be explicit
  on both sides: the callee function signature must
  declare the parameter with `изм`, and the caller must
  prefix the passed argument with `изм` during the call.
  The function operates directly on the caller's memory block.

- **Immutability (`пост` Constraint):**
  Declaring a variable with `пост` marks it as strictly **deeply** immutable
  for its entire scope lifetime. It is prohibited to apply the `изм` modifier
  to a `пост` variable, and mutating any nested fields, array elements, or string characters owned by it is a hard compile error.

- **Mutable Aliasing Rules:** Passing the identical variable to an `изм`
  parameter and any other parameter in the same call is a hard compile error.
  The AST checker uses Structural Path Matching to ensure exclusivity:
  - _Disjoint Struct Fields:_ Passing distinct fields
    (e.g., `изм узел.левый`, `изм узел.правый`) is guaranteed disjoint and
    is **allowed**.
  - _Dynamic Subscripts:_ Since runtime indices cannot be verified, doing
    `моя_функ(изм м[i], изм м[j])` is strictly prohibited.
    **Exception:** The built-in `обмен(изм м[i], изм м[j])` performs
    safe dynamic checking and is allowed for safe swapping.

## 4. Dedicated to Type System

### A. Type Conversions (Casting):

(_Being referred by_:
`language_specification.md` paragraph 8 "Type Casting",
`types.md` paragraph 6 "Type Conversions (Casting) Syntax and Rules")

- **Implicit Conversions:** Strictly forbidden to prevent unintentional numeric
  truncations, with one exception for optionals (see below).

- **Explicit Casts:** Performed using the `как<тип>(выражение)` operator.
  _(See `grammar.md` paragraph 2.6 "Expressions" item "<primary>")_
  Limited to inter-conversions between native numeric formats, logical types,
  and the unwrapping of optional types.

- **Panic Rule:** If a narrowing cast of a numeric type evaluates to a value
  exceeding the capacity of the target type
  (e.g., truncating `ц64` 300 into `ц8`), execution halts via a runtime panic.

- **Optional Type Conversions:**
  - **Widening (Safe, Implicit):** A non-optional value of type `T` can be
    implicitly converted and assigned to an optional type `T?`.
    This operation is always safe.
  - **Narrowing (Unwrapping, Explicit):** Converting an optional `T?` to a
    non-optional `T` is a potentially unsafe operation and is strictly forbidden
    implicitly. It must be performed explicitly using the explicit cast.
    This cast performs a runtime check: if the optional contains a value, it is
    unwrapped and returned; if the optional is `ничто`, the cast triggers a
    fatal runtime panic. This "unwrap-or-panic" behavior requires the programmer
    to explicitly handle the `ничто` case before casting.
- Built-in functions `в_стр` and `из_стр` must be used to convert to/from
  strings. _(See `builtins.md` paragraph 3 "String Conversions")_

### B. Void, Nullability and dual essence of `ничто`:

(_Being refered by_: paragraph 6 "Control Flow",
`types.md` paragraph 4 "Void, Nullability and Optionals"
item "Definition and constraints",
`language_specification.md` paragraph 3 "Type System Overview")

- Variables are strictly non-null by default. Only heap types being explicitly
  denoted as optionals (by appending `?` (e.g., `МояCтруктура?`)) can hold
  `ничто` (null state). Stack primitives cannot.
  _(See `grammar.md` paragraph 2.3 "Types" item "<type>")_
  The literal `ничто` is only a
  valid initialization expression for optional heap-bound types. Assigning it
  elsewhere is a compile error.

- `ничто` is used simultaneously as the `void` return type for functions that do
  not return a value, and as the `null` state value for explicitly designated
  optional heap-bound types.

- The literal `ничто` carries no inherent type. It cannot be used directly in
  contexts requiring implicit type deduction (e.g., as the value argument in the
  `заполнить` built-in function). To represent a dynamically sized array of
  empty optionals, an explicit cast must be used to establish the base type:
  `заполнить(как<стр?>(ничто), 10)`.

### C. Literal Type Deduction

Numeric literals (e.g., `10`, `3.14`) do not possess an intrinsic strict type
during initial parsing.

- **Contextual Typing:** When a numeric literal is used in a context with a
  strict expected type (e.g., assigned to a declared variable, or passed into a
  typed function parameter), the literal implicitly absorbs that target type.
  The compiler statically verifies that the literal's value physically fits
  within the target type's boundaries; if it overflows,
  a compile error is thrown.

- **Unconstrained Defaulting:** When a numeric literal is used in an
  unconstrained expression without a clear expected type (e.g., passed to an
  overloaded generic-like builtin like `в_стр(5)`, or used in a standalone
  equality check like `5 == ничто`), integer literals strictly default to the
  smallest standard signed integer type capable of physically representing the
  value, with a minimum floor of `ц8`. That is, it defaults to `ц8`,
  or `ц16` if the value exceeds `ц8` capacity. If the value exceeds `ц16`
  capacity, it defaults to `ц32`. If the value exceeds `ц64` capacity,
  a compile error is thrown. Floating-point literals default to `пт32`,
  or `пт64` if the value exceeds `пт32` capacity. If the value exceeds
  `пт64` capacity, a compile error is thrown.

## 5. Expressions & Operations

### A. Supported Operations:

- **Arithmetic (defined for numeric):**
  `+` (addition), `-` (subtraction/unary negation),
  `*` (multiplication), `/` (division), `%` (modulo).
  _Overloads:_
  `+` concatenates strings. `*` repeats a string by an integer.

- **Bitwise (for integer primitives only):**
  `&` (AND), `|` (OR), `~` (Inversion), `^` (XOR),
  `<<` (Left Shift), `>>` (Right Shift).

- **Relational (defined for primitives):**
(_Being refered by_:
`types.md` paragraph 4 "Void, Nullability and Optionals"
item "Null Comparison",
`language_specification.md` paragraph 8 "Type Casting")
  `==`, `!=`, `<`, `>`, `<=`, `>=`.
  _Overloads:_ `==` and `!=`:
  1) `стр` string equality check (compares underlying UTF-8 contents).
  2) Null state checking: comparing any optional type to the literal `ничто`.
  3) Comparing optional type to the same non-optional is prohibited.
  _Explicit Exception:_ `стр?` may be directly compared to `стр`.
  The comparison is symmetric.
  If the optional is `ничто`, `==` safely evaluates to `ложь`.

- **Logical:** `и` (AND), `или` (OR), `не` (NOT).

- **Accessors:**
  `.` (Member access), `[]` (Index and string slice),
  `\` (Namespace resolution).

- **Assignments:** `=`, `+=`, `-=`, `*=`, `/=`.

- **Grouping:** `()`

### B. Expression and Operators semantics

(_Being referred by_: `language_specification.md` paragraph 7
"Expressions and Operators")

- **Operator precedence:**
  _(See `grammar.md` paragraph 2.7 "Operator Precedence and Associativity")_

- **Short-Circuiting:** Logical `и` (AND) and `или` (OR) short-circuit.
  If the left-hand side satisfies the condition,
  the right-hand side is **not evaluated**. Any function calls or side effects
  on the right-hand side are safely skipped.

- **Transparent Member Access (`.`):**
(_Being refered by_:
`codogen.md` paragraph 3 "Memory & Parameter Mapping Semantics"
item "Member Access Translation")
  The dot operator acts as a universal field accessor.
  The user employs the identical `.` syntax regardless of
  whether the structure is a stack-bound value (`стат_структ`) or
  a heap-bound smart pointer (`структ`).
  
- **Indexing and Slicing (`[i]` and `[start:end]`):**
  - Indices must strictly evaluate to an unsigned integer
    (`ц8б`, `ц16б`, `ц32б`, `ц64б`).
  - **Array Indexing:** Dynamic and static arrays only support single-element
    indexing (`[i]`). Slicing arrays is a compile error.
  - **String Slicing:** Slicing (`[start:end]`) is strictly restricted to `стр`.
    It evaluates to a **brand new, deep-copied string** containing the
    characters from index `start` up to, but not including, index `end`. If
    `start >= end`, it returns an empty string (`""`). If any provided index is
    out-of-bounds, the operation triggers a fatal runtime panic.

- **String UTF-8 Subscript Semantics:**
  String elements accessed via `[i]` use **UTF-8 character indexing**
  (not byte offsets) and return a read-only single-character `стр`.
  Because `стр` maps to a byte array under the hood,
  retrieving the _i-th_ UTF-8 character is an **O(N) operation**.
  *Warning*:
  Sequential iteration over a string using an index loop results in an O(N^2)
  algorithm. For sequential character processing, explicitly convert the string
  to a `ряд[ц32]` using the `в_ряд_символов` built-in function.
  _(See`builtins.md` paragraph 3 "String Conversions"_
  _item "String to UTF-32 Array")_
  Assigning to a string subscript is a compile error.

- **Optional Access Constraints:** Applying the member access (`.`) or
  index/slice (`[]`) operators directly to a variable of an optional type (`?`)
  is a **hard compile error**. To access the underlying value, the programmer
  must first explicitly and safely unwrap the optional. This is typically done
  by using the explicit cast, which performs a "unwrap-or-panic" operation,
  thereby forcing the programmer to consciously acknowledge the potential for a
  runtime failure if the value is `ничто`.
- **Assignments as Statements:** Assignment operators (`=`, `+=`, etc.) are
  explicitly parsed as **statements**, not expressions. This natively prevents
  their execution within conditionals (e.g., `если (а = б)` is a compile error).

## 6. Control Flow

- `если`: Evaluates a `лог` (Boolean) expression conditionally. The compiler's
  frontend strictly enforces that the condition evaluates _exactly_ to `лог`.
  Implicit evaluation of numeric types (like C's `if (1)`) is a hard compile
  error, preventing C++ implicit conversions from leaking through the
  transpiler.

- `пока`: Loops execution block while the condition evaluates to `истина`.

- `прекратить` (break): Immediately forces execution to exit the innermost
  enclosing `пока` loop structure.

- `продолжить` (continue): Immediately stops execution of the current iteration
  block and jumps back to re-evaluate the `пока` condition for the next
  potential iteration.

- **Return Completeness:** Every function with a return type other than `ничто`
  must guarantee a return. The compiler enforces this via a strict, single-pass
  **conservative structural check**. A function structurally passes if and only
  if one of these are implicated:
  1. The final statement of the function's top-level block is exactly a
     `вернуть ...;` statement **OR** a standalone call to `авария(...);` or
     `выйти(...)`
  2. The final statement is an `если ... иначе ...` block, where _both_ the
     `если` and `иначе` branches structurally terminate in `вернуть` or `авария`
     or `выйти`.
     _NOTE: The AST semantic analyzer must explicitly treat a_
     _standalone call to `авария` or `выйти` as a terminal control-flow node_
     _(equivalent to `вернуть`), rather than a standard function call,_
     _to satisfy this check._
     _Any other structural flow, even if mathematically guaranteed_
     _to loop infinitely or return, is treated as incomplete and_
     _triggers a compile error._
- **`ничто` in Return Statements (Dual Semantics):** `вернуть ничто;` behaves
  contextually based on the enclosing function's declared return type.
  _(Follows paragraph 4.B "Nullability and dual essence of `ничто`")_

## 7. Name Resolution, Scoping & Modules

(_Being referred by_: `language_specification.md` paragraph 6
"Modules & Namespaces")

- **Lexical Block Scoping:** Variables defined inside a `{}` block are destroyed
  when the block terminates.

- **Shadowing Constraints:** Inner scopes may shadow identically named variables
  from outer scopes. However, re-declaring a variable with the exact same name
  within the _same_ scope is a compile error.

- **Type Aliasing:**
(_Being refered by_: `types.md` paragraph 7 "Type Aliasing")

- - The `употреб` keyword creates a **transparent alias**
    (equivalent to C++ `using`).
- _Example:_ `употреб ц32 Возраст;`
  _(See `grammar.md` paragraph 2.2 "Modules, Namespaces and Aliases"_
  _item "<using_stmt>")_
- Once declared, `Возраст` and `ц32` are 100% interchangeable in function
  signatures, assignments, and expressions. It does not create a strong,
  isolated new type.

- **Variable Mutability Lifecycle:** Variables declared as `пост` are completely
  immutable for their entire lifetime. Applying `изм` to a `пост` variable is a
  compile error.

- **Single-Pass Declaration:** The compiler parses top-to-bottom. Usage of _any_
  identifier (variable, function, struct) prior to its lexical declaration is
  strictly prohibited. Functions called or structures referenced out of order
  must have forward declarations (prototypes).

- **Global Variables:** Variables declared at the top level of a file have
  program-wide lifetime. They are accessible from all functions defined below
  their declaration.

- **Namespaces (`область`):** Encapsulate scopes. Bypassed via explicit prefix.
Empty namespaces are prohibited.
  - _Imports:_ `употреб МояОбласть\*;`
  (The `\*` wildcard applies exclusively to `область` containers).
   _(For syntax see `grammar.md` paragraph 2.2_
   _"Modules, Namespaces and Aliases" item "<using_stmt>")_

- **Empty Structs:** Declaring a `структ` or `стат_структ` with zero fields is
  a compile error.

- **Module Inclusion:** Resolves in a strict single-pass manner.
  - `включить "файл.ru";` — Exposes all target top-level declarations downwards.
  - `из "файл.ru" включить имя1, имя2;` — Exposes exactly the specified
    identifiers. _(For syntax see `grammar.md` paragraph 2.2_
  _"Modules, Namespaces and Aliases" item "<include_stmt>")_
  - Includes within namespaces attach the included identifiers to the enclosing
    namespace.
  - **Idempotent and Additive File Inclusion:**
    - Included files are parsed into the AST only once. Identifiers are added to
      the scope progressively and idempotently.
    - Circular inclusion dependencies must be broken manually via struct
      prototypes declared before the `включить` statement.

## 8. I/O State Machine

(_Being reffered by_: `builtins.md` paragraph 1 "I/O and File Interactions"
item "Target Control (Redirection)")

- **Active State Machine Paradigm:** The program maintains two global "active
  targets": one for reading and one for writing. By default, these map to
  standard console streams (`stdin`, `stdout`). All I/O commands dynamically
  route to and interact with the current active targets.
- Establishing a new active target automatically and safely flushes and closes
  the previous file target, entirely preventing manual descriptor management
  errors or leaks.
- **Fallback Rule:** If any redirection function fails (e.g., file not found,
  permission denied) and returns `ложь`, the currently active I/O target is
  _not_ closed and remains fully active.

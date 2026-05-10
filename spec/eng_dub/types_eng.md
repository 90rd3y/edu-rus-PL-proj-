# Type System

The language features a static, strictly enforced type system designed for
predictable memory layouts.

## 1. Type Declaration Syntax

_(See `grammar.md` paragraph 2.3 "Types" and paragraph 2.4 "Declarations")_

## 2. Stack-Bound Types (Passed by Value)

(_Being refered by:_ `semantics.md` paragraph 2
"Memory & Assignment (The Golden Rules)" item "Values vs. Unique Ownership"
subitem "Stack-Bound Types (Passed by strict Value copy)",
`language_specification.md` paragraph 3 "Type System Overview")

- **Integer Primitives:**
  - Signed: `ц8`, `ц16`, `ц32`, `ц64`
  - Unsigned: `ц8б`, `ц16б`, `ц32б`, `ц64б`
- **Floating Point:** `пт32`, `пт64`
- **Boolean:** `лог` (Values: `истина`, `ложь`)
- **Static Arrays:** `стат_ряд[<тип>, <размер>]`. When instantiating variables
  or configuring structures, the physical size parameter must be an explicit
  numeric integer literal. Within function signatures, the size position accepts
  a user-named identifier (e.g., `стат_ряд[ц32, н]`), which becomes a read-only
  `ц32б` constant implicitly captured at compile time. Each distinct name
  represents an independently sized array.
- **Static Structures:** `стат_структ`. Represents plain contiguous data blocks.

## 3. Heap-Bound Types (Passed by Unique Ownership/Borrow)

(_Being refered by:_ `semantics.md` paragraph 2
"Memory & Assignment (The Golden Rules)" item "Values vs. Unique Ownership"
subitem "Heap-Bound Types (Passed by cloned Unique Ownership)",
`language_specification.md` paragraph 3 "Type System Overview")

- **Dynamic Arrays:** `ряд[<тип>]`. A resizable collection.
- **Dynamic Structures:** `структ`. Encapsulated as securely owned smart
  pointers in the backend mapping.
- **Strings:** `стр`. Functionally handles a dynamic array of UTF-8 characters
  under the hood, but is structurally treated by the compiler as a completely
  distinct type. Implicit or explicit casting via `как<T>` between `стр` and a
  byte array (`ряд[ц8]`) is strictly prohibited. Use dedicated built-in
  functions for such conversions.

## 4. Void, Nullability and Optionals

(_Being referred by_: `language_specification.md`
paragraph 3 "Type System Overview")

- **Definition and constraints:**
  _(See `semantics.md` paragraph 4.B "Nullability and dual essence of `ничто`")_
- **Null Comparison:**
  _(See `semantics.md` paragraph 5.A "Supported operations"_
  _item "Relational (defined for primitives)_ subitem "Overloads")_

## 5. Type Conversions (Casting) Syntax and Rules

- **Syntax:** `как<тип>(выражение)`
_(See `grammar.md` paragraph 2.6 "Expressions" item "<primary>")_
- **Rules:** _(See `semantics.md` paragraph 4.A "Type Conversions (Casting)")_

## 6. Type Aliasing

_(See `semantics.md` paragraph 7 "Name Resolution, Scoping & Modules"_
_item "Type Aliasing")_

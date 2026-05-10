# Core Built-in Functions

(_Being referred by_: `language_specification.md` paragraph 9
"Core Built-in Functions")

_NOTE_: All built-in functions listed in this document are
implicitly declared in the global scope before any user code.
They are resolved as regular function calls by the parser and type-checked as
compiler intrinsics during semantic analysis.
Their names are reserved globally and cannot be redefined by the user.

## 1. I/O and File Interactions

- **Target Control (Redirection)** _(See `semantics.md` paragraph 8 "I/O State
  Machine")_
  1. `функ нацелить_вывод: стр имя_файла -> !лог` _(Attempts to open the file
     specified in overwrite mode. If successful, closes the current output file
     (if it's not `stdout`) and sets the new file as active output. Returns
     `истина`. If opening fails, returns `ложь` and safely leaves the current
     output active.)_
  2. `функ нацелить_вывод_доп: стр имя_файла -> !лог` _(Attempts to open the file
     specified in append mode. If successful, closes the current output file (if
     it's not `stdout`) and sets the new file as active output. Returns
     `истина`. If opening fails, returns `ложь` and safely leaves the current
     output active.)_
  3. `функ вернуть_вывод: -> ничто` _(If the current output file is not
     `stdout`, closes it and_ _restores active output back to `stdout`.)_
  4. `функ нацелить_ввод: стр имя_файла -> !лог` _(Attempts to open the file
     specified in read mode. If successful, closes the current input file (if
     it's not `stdin`) and sets the new file as active input. Returns `истина`.
     If opening fails, returns `ложь` and safely leaves the current input
     active.)_
  5. `функ вернуть_ввод: -> ничто` _(If the current input file is not `stdin`,
     closes it and_ _restores active input back to `stdin`.)_
  6. `функ нацелить_вывод_изм: стр имя_файла -> !лог` _(Attempts to open an
     **existing** file specified in read-write mode without truncation (C++:
     `std::ios::in | std::ios::out`). If successful, closes the current output
     file (if it's not `stdout`) and sets the new file as active output. Returns
     `ложь` if the file does not exist or fails to open, safely leaving the
     current output active. Returns `истина` on success.)_

- **Reading Operations (From active input target)**
  1. `функ ввод: стр подсказка -> стр` _(Reads a line until newline. If the
     active input target is `stdin`, `подсказка` is written to `stdout`.)_
  2. `функ читать_символы: ц32б кол_во -> !стр?` _(Reads exactly the specified
     number of characters)._
  3. `функ читать_слова: ц32б кол_во -> !ряд[стр]?` _(Reads an array of N words
     separated by whitespace)._
  4. `функ задать_поз_ввода: ц32б байт -> ничто` _(Sets the read cursor to the
     specified exact byte position (absolute))._
  5. `функ текущая_поз_ввода: -> ц32б` _(Returns the current byte position of
     the read cursor)._
  6. `функ размер_ввода: -> ц32б` _(Returns the total size of the active input
     target in bytes)._
  7. `функ конец_ввода: -> лог` _(Returns `истина` if the active input target
     has reached End Of File)._

- **Writing Operations (To active output target)**
  1. `функ вывод: стр текст -> ничто` _(Sends text to the current active output
     target)._
  2. `функ вывод_ошибки: стр текст -> ничто` _(Sends text to `stderr`)._
  3. `функ задать_поз_вывода: ц32б байт -> ничто` _(Sets the write cursor to the
     specified byte position (absolute))._
  4. `функ текущая_поз_вывода: -> ц32б` _(Returns the current byte position of
     the write cursor)._
  5. `функ размер_вывода: -> ц32б` _(Returns the total size of the active output
     target in bytes)._

- **File Cropping (Stateless)**
  1. `функ обрезать_файл: стр имя_файла, ц32б байтов_оставить -> !лог`
     _(Instantly truncates the specified file leaving only_ _the specified
     amount of bytes from the beginning)._

## 2. Execution control and other utilities:

- Exit programm with return code: `функ выйти: ц32 -> ничто`
- Emergency exit with error message: `функ авария: стр -> ничто`
- Getting length of string in UTF-8 characters (compiler intrinsic, statically
  type-checked per usage): `функ стр_длина: стр -> ц32б`
- Getting byte size of string (compiler intrinsic, statically type-checked per
  usage): `функ стр_размер: стр -> ц32б`
- Getting length of dynamic array (compiler intrinsic, statically type-checked
  per usage): `функ ряд_длина: ряд[тип] -> ц32б`
- Getting length of static array: compile time known:
  `ст_ряд_длина(<expr>) -> ц32б` _(Handled by compiler as an operator, evaluates
  to the dimension of the provided static array)_
- Appending to dynamic array (compiler intrinsic, statically type-checked per
  usage): `функ пополн: изм ряд[тип], тип арг -> !лог` _(Appends a single element
  to the end of the array. Returns `истина` on success.)_
  `функ пополн: изм ряд[тип], ряд[тип] арг -> !лог` _(Appends all elements of
  `арг` array to the end of the target array. Returns `истина` on success.)_
- Removing last N elements from dynamic array (compiler intrinsic, statically
  type-checked per usage):
  `функ удалить_посл: изм ряд[тип], ц32б кол_во -> ничто` *(Removes
  `кол*во`elements from the end of the array. If`кол*во` exceeds the array
  length, runtime panic.)*
- _Arbitrary-index insertion and removal are deferred to post-MVP._
- Creating a filled dynamic array (compiler intrinsic, statically type-checked
  per usage): `функ заполнить: тип знач, ц32б размер -> !ряд[тип]?` _(Returns a
  new heap-allocated dynamic array of the given `размер`, with every element
  initialized to `знач`. The compiler statically resolves `тип` from the type of
  `знач`.)_
- Getting size in bytes of compile time known types: `размер(<type>) -> ц32б`
  _(Handled by compiler as an operator, not a standard function)_
- State Swapping (compiler intrinsic, statically type-checked per usage):
  `функ обмен: изм тип а, изм тип б -> ничто` _(Instantly swaps the underlying
  memory of `а` and `б` without performing deep copies or heap allocations.
  `тип` is a meta-placeholder; both arguments must strictly resolve to the exact
  same type. In the C++ backend, this maps directly to native
  `std::swap(a, b)`)._

## 3. String Conversions

(_Being referred by_: `language_specification.md` paragraph 8 "Type Casting",
`semantics.md` paragraph 4.A "Type Conversions (Casting)")

Parsing text data and formatting primitives natively. These built-in functions
are statically overloaded by the compiler for all native stack primitives
(`цN`, `цNб`, `птN`, `лог`).

- **Primitive to String:** `функ в_стр: <primitive> -> !стр?` _(Returns a string
  representation of the value. Returns ничто if the underlying heap allocation fails — consistent with the language's
  fatal-error philosophy)._
- **String to Primitive:** `функ из_стр: стр текст, изм <primitive> итог -> !лог`
  _(Parses the text and stores the resulting converted value directly into the
  explicitly mutated `итог` instance. Returns `истина` on success, or `ложь` if
  the string contains invalid characters, is structurally invalid, or numeric
  overflow occurs. For `лог`, it accepts exactly the strings `"истина"` and
  `"ложь"` (case-sensitive). Any other value returns `ложь`)._

_Note: `<primitive>` in the signatures above is a meta-placeholder. The compiler
internally generates a separate overload for each stack primitive type
(`ц8`..`ц64`, `ц8б`..`ц64б`, `пт32`, `пт64`, `лог`). The user calls these as
regular functions and the compiler resolves the correct overload statically from
argument types._

- **String to UTF-32 Array:** `функ в_ряд_символов: стр текст -> !ряд[ц32]?`
  (_Being referred by_: `semantics.md` paragraph 5.B
  "Expression and Operators semantics" item "String UTF-8 Subscript Semantics")

_(Returns a new dynamic array containing the UTF-32 code points (Runes) of the
string. This guarantees O(1) random access per character, bypassing the O(N)
penalty of standard string subscripting)._

- **String to Byte Array:** `функ в_ряд_байтов: стр текст -> !ряд[ц8]?`

_(Returns a new dynamic array containing the raw underlying UTF-8 bytes of_
_the string without any transcoding or interpretation)._

# Micro-C 6809 Compiler Internals

[← Back to README](README.md)

This document covers how the compiler actually works — its architecture,
what the type system looks like under the hood, what language features
are and aren't supported, and how the 6809 code generator makes decisions.

The compiler lives in three files: `compile.c` (~2500 lines), `io.c` (~260
lines), and `6809cg.c` (~810 lines). Total: around 3570 lines of K&R C,
self-hosting, no external libraries beyond stdio.

---

## Architecture

The compiler is a classic single-pass recursive descent design. There is
no IR, no AST, no intermediate representation of any kind. Source text
goes in, 6809 assembly comes out, one statement at a time.

```
Source text
    |
    v  io.c: read_line(), get_lin()
Line buffer (input_line[])
    |
    v  compile.c: get_token()
Token stream (scanner/lexer)
    |
    v  compile.c: statement() -> get_value() -> evaluate()
Expression evaluation (recursive descent with operator stack)
    |
    v  6809cg.c: accval(), accop(), call(), def_func() ...
6809 assembly output (written directly to stdout)
```

`compile()` is a four-line function. It calls `def_module()` (a no-op
on the 6809), then loops forever calling `statement(get_token())`. Every
statement is compiled and emitted before the next token is read. There is
no look-ahead beyond one token (`unget_token()`), and no backtracking.

The clean separation between frontend and backend is worth noting: `compile.c`
calls a fixed set of code generation functions (`accval`, `accop`, `call`,
`def_func`, `def_label`, `jump`, `jump_if`, etc.), and `6809cg.c` implements
them. This is the interface that makes Micro-C retargetable. To target a
different processor you replace `6809cg.c` with a new implementation of the
same interface.

---

## The type system: one unsigned integer

Every type in Micro-C is represented as a single `unsigned` with bit fields
packed in. On the original DOS platform `unsigned` was 16 bits; on Linux
(LP64) it is 32 bits, and bit 16 is used for the `LONG_TYPE` extension:

```
Bit 16   LONG_TYPE   — 32-bit storage type (long) [Linux port extension]
Bit 15   REFERENCE   — symbol has been referenced (liveness tracking)
Bit 14   INITED      — variable has an initializer
Bit 13   ARGUMENT    — symbol is a function argument
Bit 12   EXTERNAL    — external linkage
Bit 11   STATIC      — static storage class
Bit 10   REGISTER    — register hint
Bit  9   TVOID       — void type
Bit  8   UNSIGNED    — unsigned qualifier
Bit  7   BYTE        — 0 = 16-bit (int), 1 = 8-bit (char)
Bit  6   CONSTANT    — const qualifier
Bits 5-4 SYMTYPE     — VARIABLE(0), MEMBER(1), STRUCTURE(2), FUNCGOTO(3)
Bit  3   ARRAY       — symbol is an array
Bits 2-0 POINTER     — pointer indirection level (0–7)
```

This is the type of a variable declaration, an expression intermediate
result, and a symbol table entry — all the same 16 bits. It travels from
the parser through the expression evaluator into the code generator.

The 3-bit POINTER field gives up to 7 levels of pointer indirection. Every
`*` in a declaration increments the POINTER field. Every dereference via `*expr`
decrements it. If you try to go past level 7, `line_error("Too many pointer
levels")` fires.

There is no float, no double, no enum, no bitfield. `typedef` and `long`
have been added as extensions (see below). The original documentation
listed "Long/Double/Float/Enumerated types, Typedef and Bit fields" as
unsupported. `long` is now a 32-bit storage type (no expression-level
arithmetic; use LONGMATH), and `typedef` works fully including struct
typedefs.

---

## What literals are supported

### Integer constants

Three bases, parsed by `get_token()`:

- **Decimal** — `123`, `0` (starts with a nonzero digit or is literally `0`)
- **Octal** — `0177` (starts with `0` followed by more digits)
- **Hexadecimal** — `0x1F` (starts with `0x`)

All integers are stored as `unsigned` (16-bit values on 6809). There is no `L` or
`U` suffix. A value with the high bit set (e.g. `0x8000`) is treated as
`UNSIGNED` automatically in the expression evaluator:
```c
tt = ((int)v < 0) ? UNSIGNED : 0;
```

### Character constants

Single-quoted character constants are supported, including multi-character
constants up to 16 bits. `'A'` is `0x41`, `'AB'` is `0x4142`. This is
a documented Micro-C extension — it lets you pack two-byte command codes
into a single constant, which is useful for 6809 work.

Escape sequences are supported inside character and string constants
(`\n`, `\t`, `\r`, `\0`, `\\`, `\"`, `\'`, and `\xNN` hex escapes) via
`read_special()` in `compile.c`.

### String literals

String literals are stored in `literal_pool[]` (65,535 bytes by default).
Each string gets an offset into the pool as its value; the code generator
emits them as `FCB` sequences at the end of the output.

Adjacent string literals are automatically concatenated:
```c
"hello" " world"   /* becomes a single "hello world" */
```

With the `-f` (fold) flag, **duplicate string literals are merged** — the
second occurrence pointing to the same pool entry as the first. This saves
ROM space at the cost of a linear scan at compile time.

### No floating point

There is no floating point support of any kind. `float` and `double` are
not in the keyword table and will be silently treated as unknown symbols.
For fractional arithmetic, use fixed-point, or declare a 4-byte `long`
variable and implement software float via library functions.

---

## What language features are supported

### Supported C subset

Everything you would expect from K&R C for embedded use:

- `int`, `unsigned`, `char`, `void`
- `static`, `extern`, `const`, `register`
- Pointers and pointer arithmetic (up to 7 levels of indirection)
- Arrays (multi-dimensional, with `sizeof` aware of all dimensions)
- `struct` and `union` (full member access with `.` and `->`)
- All arithmetic operators: `+`, `-`, `*`, `/`, `%`
- All bitwise operators: `&`, `|`, `^`, `~`, `<<`, `>>`
- All comparison operators, signed and unsigned variants
- All assignment operators: `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`
- Pre/post increment and decrement `++` and `--`
- Ternary operator `?:`
- `sizeof` (type-aware, multi-dimensional array aware)
- Typecasts to `int`, `unsigned`, `char`, `const`, `register`, `void` — see **Cast semantics** below
- `(unsigned char)` and `(signed char)` compound casts are **not** supported; use `& 0xFF` to isolate the low byte as unsigned
- Function pointers and indirect calls
- `if`/`else`, `while`, `do`/`while`, `for`, `switch`/`case`/`default`
- `break`, `continue`, `return`, `goto` with labels
- Initialised global and static variables (scalars, arrays, structs)
- Both ANSI and K&R argument declaration styles
- `asm` keyword for inline assembly (block and expression forms)
- `//` C++ line comments (documented extension)
- Nested `/* */` comments (documented extension)

### Notably absent

- `float`, `double` — no floating point (library path possible via FLOATMATH)
- `long` — 32-bit storage type only; no expression-level arithmetic (use LONGMATH)
- `enum` — not in the keyword table
- Bitfields in structs — not supported
- Initialisation of non-static local variables — compile error
- Nested function definitions — compile error
- Variable-length arrays — not supported (all dimensions must be constants)
- Complex pointer declarators like `int (*fp)(int, char *)` — not supported;
  the documentation notes Micro-C does not support "complex declarations
  which use multiple levels of parentheses"

### Added by this port

- `typedef` — scalar, pointer, named struct, and anonymous struct typedefs
- `long` / `unsigned long` — 32-bit storage type, 4 bytes, `sizeof` = 4; direct assignment is a compile error directing to `longset`/`longcpy`
- `#undef` — undefine a macro
- `#error message` — compile-time fatal diagnostic
- `#warning message` — compile-time non-fatal diagnostic
- `#line N "file"` — line number and filename override for error reporting
- `__FILE__` — current source filename as a string literal (dynamic)
- `__LINE__` — current source line number as an integer constant (dynamic)
- `\a`, `\v` — bell and vertical tab escape sequences
- `&struct_var` — address-of on struct variables (was incorrectly rejected)
- GCC-compatible error format (`file:line: error: message`)
- `-W` flag — suppress unreferenced-variable warnings
- `-eN` flag — override maximum error count at runtime
- `* 4` and `* 8` inline shift expansion (strength reduction)
- Inline `== 0` / `!= 0` via `CMPD #0` / `TSTB` without runtime call

---

## Expression evaluator

`get_value()` is the expression parser. It is recursive descent, returning
values on an explicit operator stack (`expr_token[]`, `expr_value[]`,
`expr_type[]`) of depth 20. Operator precedence is table-driven via
`priority[]` — a 40-element array indexed by token number.

The full precedence table in decreasing priority order:

| Priority | Operators |
|----------|-----------|
| 15 | `++`, `--`, `.`, `->` (postfix) |
| 13 | `*`, `/`, `%` |
| 12 | `+`, `-` |
| 11 | `<<`, `>>` |
| 10 | `<=`, `>=`, `<`, `>` |
| 9 | `==`, `!=` |
| 8 | `&` (bitwise) |
| 7 | `^` |
| 6 | `\|` |
| 5 | `&&` |
| 4 | `\|\|` |
| 3 | `?:` |
| 2 | `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `\|=`, `^=`, `<<=`, `>>=` |

`get_value()` recognises six things at the token level:

- `NUMBER` — push the constant
- `STRING` — push address of literal pool entry
- `SYMBOL` — look up in symbol table; handle function calls, array indexing, member access
- `*` (STAR) — pointer dereference
- `&` (AND) — address-of
- `(` (ORB) — typecast or sub-expression
- `sizeof` — computed at compile time, never at runtime
- `asm` — inline assembly in expression context (with operands pushed on stack)
- Any unary operator — recurse via `do_unary()`

The expression evaluator directly drives code emission. There is no
"evaluate and then emit" phase — the code comes out as `get_value()` runs.

---

## Cast semantics

Casts are handled in the `case ORB:` branch of `get_value()` in `compile.c`.
When a typecast keyword (`int`, `unsigned`, `char`, `const`, `register`, `void`)
follows `(`, the cast target type is assembled via `get_type()`, the operand is
evaluated recursively, and then two decisions are made:

**Does the cast require a code-generation action?** The condition is:

```c
if( ((tt1 & (REGISTER|CONSTANT)) != (tt & (REGISTER|CONSTANT)))
||  (((tt1 & (BYTE|POINTER)) == BYTE) != ((tt & (BYTE|POINTER)) == BYTE)))
```

If neither the register/constant class nor the byte/word width changes,
the cast is a pure type annotation — no code is emitted, the type bits
are just replaced. This handles `(int)int_var`, `(unsigned)unsigned_var`,
and similar no-change casts.

**Narrowing (int → char):** When the cast target is `BYTE` and the source
is not, `narrow_byte()` is called. This sets the `last_byte` flag in `6809cg.c`
without emitting any instruction — B already holds the low 8 bits of D from the
preceding `LDD`. The subsequent `expand(tt)` call then emits `SEX` (signed
narrowing) or `CLRA` (unsigned narrowing, not reachable via supported casts)
to produce the correct 16-bit result.

**Widening (char → int):** When the source is `BYTE` and the target is not,
`expand()` is called. The type passed to `expand()` is the cast target type
augmented with the source's `UNSIGNED` bit:

```c
expand(tt | ((source_is_byte && target_is_word) ? (tt1 & UNSIGNED) : 0));
```

Without this, `(int)unsigned_char_var` would emit `SEX` because the cast
target `int` carries no `UNSIGNED` bit — the source type's qualifier would
be silently discarded. With it, `expand()` sees `UNSIGNED` and emits `CLRA`
instead, correctly zero-extending.

**Supported casts and their effects:**

| Cast | Source type | Action |
|------|-------------|--------|
| `(char)int_expr` | int/unsigned (16-bit) in register | `narrow_byte()` → `expand()` → `SEX`; B already has low 8 bits |
| `(int)char_var` | `char` (signed, 8-bit from memory) | `LDB` sets `last_byte`; `expand()` → `SEX` |
| `(int)unsigned_char_var` | `unsigned char` (8-bit from memory) | `LDB` sets `last_byte`; `expand()` with `UNSIGNED` → `CLRA` |
| `(unsigned)int_expr` | any | Type bits replaced; `UNSIGNED` set; no instruction |
| `(int)int_expr` | any | Type bits replaced; no instruction unless byte→word widening |
| `(unsigned char)x` | any | **Not supported** — compile error |

`(const)`, `(register)`, and `(void)` casts are accepted syntactically and
update the type bits but never emit instructions.

---



Arguments are pushed right-to-left onto the hardware stack (`S`). The
called function sees them at positive offsets from `S`. The function
allocates locals by `LEAS -N,S` in the prologue and `LEAS N,S` in the
epilogue. Return value is in the D register (16-bit) or B register (8-bit).

```asm
* Call: foo(a, b, c)
  LDD  a           * third arg goes first (push right-to-left not applicable
  PSHS A,B         * — actually left-to-right on the stack)
  LDD  b
  PSHS A,B
  LDD  c
  PSHS A,B
  JSR  foo
  LEAS 6,S         * caller cleans stack (clean = n_args * 2)

* Prologue: int foo(int x, int y, int z)
foo  LEAS -N,S     * allocate N bytes of local variables
                   * args are at: z=2,S  y=4,S  x=6,S  (after LEAS)

* Epilogue:
  LEAS N+stack_level,S
  RTS
```

Arguments are always passed as 16-bit words. An `int` or `unsigned` is 2
bytes; a `char` is promoted to 2 bytes on the stack. The callee accesses
arguments via positive `S`-relative addressing. Locals are at negative
(below frame) offsets, accessed also via `S` after the prologue LEAS.

The 6809 registers used by the compiler:

| Register | Role |
|----------|------|
| D (A:B) | Primary accumulator — all arithmetic, return value |
| U | Index register — address computation, pointer load |
| X | Second parameter to runtime library calls |
| S | Hardware stack pointer |
| Y | Not used by the compiler |
| DP | Not used by the compiler |

---

## Code generation strategy

`6809cg.c` provides two core interfaces:

**`accop(operation, rtype)`** — zero-operand operations on the accumulator:
stack/unstack, negate, complement, logical NOT, increment/decrement,
copy between D and U. The `rtype` argument controls byte vs word variants.

**`accval(operation, result_type, token, value, source_type)`** — one-operand
operations: load, store, add, subtract, multiply, divide, modulo, AND, OR,
XOR, shift left, shift right, and all 8 comparison variants. The `|` character
in the template string is a placeholder substituted with the operand.

Most arithmetic is **inline** — `ADDD`, `SUBD`, `ANDB`/`ANDA`, `ORB`/`ORA`,
`EORB`/`EORA`. Operations the 6809 cannot do directly are dispatched to
runtime library routines:

| Operation | Runtime call | Notes |
|-----------|-------------|-------|
| `*` | `?mul` | 16×16 → 16 multiply |
| `/` | `?div` or `?sdiv` | unsigned or signed divide |
| `%` | `?mod` or `?smod` | unsigned or signed modulo |
| `<<` | `?shl` | shift left |
| `>>` | `?shr` | shift right |
| `==` | `?eq` | returns 0 or 1 in D |
| `!=` | `?ne` | |
| `<`, `<=`, `>`, `>=` | `?lt`, `?le`, `?gt`, `?ge` | signed |
| `<`, `<=`, `>`, `>=` | `?ult`, `?ule`, `?ugt`, `?uge` | unsigned |
| `!` | `?not` | logical NOT |

The `runtime()` helper function builds a two-instruction sequence loading
the second operand first, then calling the library function — handling the
distinction between byte and word operands automatically.

One special case worth noting: `* 2` is recognised at code generation time
and replaced with `LSLB` / `ROLA` (arithmetic left shift) rather than a
multiply call.

### The `last_byte` / `expand()` / `narrow_byte()` mechanism

The 6809 accumulator is 16-bit (D = A:B). 8-bit values are loaded into B
with `LDB`, leaving A undefined. Before any 16-bit operation the accumulator
must be widened to a valid 16-bit value. This is tracked via the global
`last_byte` flag in `6809cg.c`:

- `accval(_LOAD, ...)` sets `last_byte = -1` when it emits `LDB` (8-bit load).
- `expand(type)` checks `last_byte`: if set, emits `SEX` (signed, B→D) or
  `CLRA` (unsigned, zero-extend) depending on the `UNSIGNED` bit in `type`,
  then clears `last_byte`.
- Most operations call `expand()` before emitting their instruction, so the
  accumulator is always 16-bit clean before arithmetic.

`narrow_byte()` is the complement: it **sets** `last_byte` without emitting
any instruction. It is used by the cast handler when `(char)` is applied to
an already-in-register 16-bit value. The low 8 bits are already in B (since
D = A:B and any preceding `LDD` put them there); calling `narrow_byte()` then
`expand()` produces the correct sign-extension without a redundant load.

`switch` uses a **table dispatch**. The compiler emits `LDU #?table` followed
by `JMP ?switch`. The table itself (a sequence of `FDB value, FDB label` pairs,
terminated with `FDB 0, FDB default`) is emitted after the switch body. The
`?switch` runtime routine walks the table comparing each value to the expression
result, then does an indirect jump.

---

## The peephole optimizer (`mco09`)

The optimizer (`mco.c` + `6809.mco`) is entirely table-driven. The table
`peep_table[]` in `6809.mco` contains pairs of strings: the pattern to
match and its replacement. Patterns use `\200`–`\207` as wildcard
captures (symbolic operands) and `\240` as a "complement" wildcard for
branch instruction suffixes. The optimizer slides a 15-line window over the
assembler output and applies patterns.

Examples of what the optimizer catches:

```asm
* Store then immediately reload the same location:
  STD  foo         ->   STD  foo
  LDD  foo

* Load D then transfer to X:
  LDD  foo         ->   LDX  foo
  TFR  D,X

* Load U then transfer to D:
  LDU  foo         ->   LDD  foo
  TFR  U,D

* LDB zero:
  LDB  #0          ->   CLRB

* LDD zero:
  LDD  #0          ->   CLRB
                        CLRA

* Clean stack then return:
  LEAS 2,S         ->   PULS X,PC
  RTS

* LEAS chain collapse:
  LEAS N,S         ->   LEAS  N+M,S
  LEAS M,S

* Tail call:
  JSR  foo         ->   JMP  foo
  RTS

* Dead jump over label:
  JMP  ?1          ->   ?1 EQU *
  ?1 EQU *

* Branch inversion for conditional jump followed by unconditional:
  LBNE ?1          ->   ?1 EQU *
  JMP  ?2              LBEQ ?2
  ?1 EQU *
```

The `not_table[]` contains the complement pairs (EQ↔NE, LT↔GE, etc.) used
for branch inversion.

---

## Compiler limits

From `compile.h`:

| Parameter | Value | Notes |
|-----------|-------|-------|
| `LINE_SIZE` | 512 | Maximum source line length |
| `FILE_SIZE` | 256 | Maximum filename length |
| `SYMBOL_SIZE` | 31 | Significant characters in a symbol name |
| `MAX_SYMBOL` | 4000 | Active symbol table entries (globals + locals) |
| `INCL_DEPTH` | 10 | Maximum `#include` nesting |
| `DEF_DEPTH` | 10 | Maximum `#define` macro expansion nesting |
| `EXPR_DEPTH` | 20 | Expression operator stack depth |
| `LOOP_DEPTH` | 20 | Maximum nested loops |
| `MAX_ARGS` | 25 | Arguments per function |
| `MAX_SWITCH` | 50 | Active `switch`/`case` statements |
| `MAX_DIMS` | 2000 | Active array dimension entries |
| `MAX_DEFINE` | 500 | `#define` symbols (built-in preprocessor only) |
| `DEFINE_POOL` | 8192 | `#define` string space (bytes) |
| `LITER_POOL` | 65535 | String literal pool (bytes) |
| `MAX_TYPEDEF` | 128 | `typedef` names |
| `MAX_ERRORS` | 25 | Errors before forced abort |

Symbol names are significant to 31 characters.

Globals grow upward from index 0; locals grow downward from `MAX_SYMBOL`.
They meet in the middle. With `MAX_SYMBOL=4000` you have a combined budget
of 4000 symbol table entries for all globals plus all locals currently in
scope.

---

## Inline assembly

Micro-C provides two forms of inline assembly, both via the `asm` keyword.

**Statement form** — a block of raw assembly lines inside `{}`:
```c
asm {
    LDA  #$FF
    STA  $C000
    RTS
}
```

**Expression form** — `asm(expr, expr, ...)` pushes each expression result
onto the stack before emitting the inline code. The code can then access
the values at `2,S`, `4,S`, etc. The result is whatever is left in D.

This is not just a convenience — it is the intended mechanism for calling
ROM routines and hardware interfaces that C code cannot express. The CoCo
ROM driver in `drivers/coco_rom.asm` uses `JSR [$A002]` precisely because
there is no C-level way to dereference a vector table entry as a function
call.

---

## See Also

- [**README**](README.md) — Toolchain overview and build instructions
- [**Standard Library**](STDLIB.md) — Runtime routines and library documentation
- [**Test Suite Addendum**](TESTS_ADDENDUM.md) — Bug fixes and compiler quirks

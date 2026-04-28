# Micro-C 6809 Language Reference

[← Back to README](README.md)

This document is a complete language reference for the Micro-C 6809 cross-compiler.
It is written for programmers who know modern C (C99/C11) and want to understand
exactly what this compiler supports, what it doesn't, and where the dialect
diverges from what they're used to.

**Port additions** — features added by this Linux port that were not in Dunfield's
original DOS toolchain — are called out with:

> **[port]** Description of what was added.

Everything else is original Dunfield Micro-C behaviour.

---

## Contents

1. [The dialect at a glance](#1-the-dialect-at-a-glance)
2. [Types](#2-types)
3. [Literals](#3-literals)
4. [Operators and expressions](#4-operators-and-expressions)
5. [Declarations and scope](#5-declarations-and-scope)
6. [Statements](#6-statements)
7. [Functions](#7-functions)
8. [Preprocessor](#8-preprocessor)
9. [Inline assembly](#9-inline-assembly)
10. [Calling convention and ABI](#10-calling-convention-and-abi)
11. [Compiler limits](#11-compiler-limits)
12. [Standard headers](#12-standard-headers)
13. [Quirks and pitfalls](#13-quirks-and-pitfalls)

---

## 1. The dialect at a glance

Micro-C is **K&R C for microcontrollers**. Think of it as the subset of K&R C
that fits comfortably in a few kilobytes of ROM on a machine with no OS, no
filesystem, and a 16-bit address space.

If you're coming from C99 or C11, the most important things to know up front:

- **No floats, no doubles.** Not a limitation you can work around with a cast —
  the types simply don't exist.
- **No initialised local variables.** `int x = 5;` inside a function is a
  compile error. Assign after the declaration.
- **No standard library.** `stdio.h`, `stdlib.h`, `string.h` — none of these
  exist in the usual sense. The toolchain ships its own minimal runtime; see
  [STDLIB.md](STDLIB.md).
- **`long` means storage only.** You can declare a 32-bit variable, but you
  can't do `a + b` on two longs in an expression. Use the LONGMATH library
  functions.
- **K&R function syntax is the native style.** ANSI-style prototypes work, but
  the compiler doesn't enforce argument types across translation units.
- **Typedef names don't work in casts.** `(uint8_t)x` gives "Undefined symbol"
  — the cast parser only recognises the built-in keyword types `int`, `unsigned`,
  `char`, `void`, `const`, `register`. This is standard K&R-era behaviour;
  ANSI C89 is what unified typedef names and cast targets into the same grammar.
  `sizeof(uint8_t)` does work. Use `(unsigned)`, `(char)`, or `(int)` in casts
  and save typedefs for declarations.
- **The preprocessor is split.** The compiler (`mcc09`) has a basic built-in
  preprocessor for simple `#define` and `#include`. For parameterised macros,
  `#if` expressions, and `##`, you need the separate `mcp` preprocessor
  (`cc09 -P`). See [§8 Preprocessor](#8-preprocessor).

---

## 2. Types

### 2.1 Scalar types

| Type | Size | Range | Notes |
|------|------|-------|-------|
| `char` | 1 byte | −128 to 127 | Signed by default |
| `unsigned char` | 1 byte | 0 to 255 | |
| `int` | 2 bytes | −32768 to 32767 | Native word size |
| `unsigned` / `unsigned int` | 2 bytes | 0 to 65535 | |
| `void` | — | — | For function return types and `void *` |

There is no `short`, no `long long`, no `float`, no `double`, no `enum`.

> **[port]** `long` and `unsigned long` are now 32-bit storage types (4 bytes).
> `sizeof(long)` returns 4. They can be declared and passed by pointer, but
> expression-level arithmetic on `long` values is not supported — use the
> LONGMATH library. Direct assignment such as `long x = 100000;` is a compile
> error with a message directing you to `longset`/`longcpy`.

Unsigned arithmetic is available on `unsigned` and `unsigned char`. The compiler
tracks the `UNSIGNED` qualifier through expressions and selects the appropriate
comparison routine (`?ult`, `?uge`, etc.) automatically.

### 2.2 Pointers

Pointers are 16-bit (the 6809 has a 16-bit address space). Any pointer type
— `int *`, `char *`, `void *` — is 2 bytes.

Up to **7 levels of pointer indirection** are supported. This is enforced at
compile time; `line_error("Too many pointer levels")` fires if you exceed it.

Pointer arithmetic scales correctly by element size:

```c
int arr[4];
int *p = arr;

p + 1     /* advances by 2 bytes (sizeof int) */
p[2]      /* same as *(p + 2), correct */
*(p + 2)  /* also correct — see note below */
```

> **[port]** The binary `+` and `-` operators now scale the integer operand by
> `sizeof(*ptr)` when one operand is a pointer. Previously only the subscript
> operator `p[n]` and the increment/decrement operators `++p`/`p++` scaled
> correctly; `*(p+n)` added raw bytes. Both constant and variable offsets are
> handled for all non-`char` pointer types.

`char *` arithmetic is always stride-1 and unaffected.

### 2.3 Arrays

Arrays are declared with constant dimensions. Variable-length arrays are not
supported — all dimension sizes must be integer constants at compile time.

Multi-dimensional arrays work:

```c
int matrix[4][4];
matrix[1][2] = 99;
```

`sizeof` is aware of all dimensions. `sizeof(matrix)` is 32.

The maximum number of array dimension entries across all active declarations is
controlled by `MAX_DIMS` (default 2000).

### 2.4 Structs and unions

Full struct and union support including:

- Member access via `.` and `->`
- Nested structs and unions
- Arrays of structs
- Pointers to structs
- `sizeof` on struct types (returns the total byte size)

```c
struct Point {
    int x;
    int y;
};

struct Point p;
struct Point *pp = &p;
pp->x = 10;
p.y = 20;
```

**Struct assignment is not supported.** `s2 = s1` produces "Non-assignable".
Copy field by field, or use `memcpy`.

**Bitfields are not supported.** Use masks and shifts manually — e.g. `(x >> 4) & 0x0F` to extract a 4-bit field at bit 4.

> **[port]** Taking the address of a struct variable (`&struct_var`) previously
> produced a compile error. This has been fixed — `&p` works correctly.

### 2.5 typedef

> **[port]** `typedef` was not in the original Dunfield toolchain. It has been
> added in full, including:

```c
typedef unsigned char  uint8_t;     /* scalar typedef */
typedef unsigned int   uint16_t;
typedef struct Point   PointAlias;  /* named struct typedef */
typedef struct { int x; int y; } Vec2;  /* anonymous struct typedef */
typedef Vec2 *Vec2Ptr;              /* pointer typedef */
```

Typedefs work in declarations, `sizeof`, K&R-style argument declarations, and
as the base type for further typedefs. They do **not** work in cast expressions
— see §1 and §4.4. Up to `MAX_TYPEDEF` (128) names.

The `include/stdint.h` header provides `int8_t`, `uint8_t`, `int16_t`,
`uint16_t`, `int32_t`, `uint32_t` via typedef.

---

## 3. Literals

### 3.1 Integer constants

Three bases are supported:

| Base | Example | Notes |
|------|---------|-------|
| Decimal | `255`, `0` | Starts with a nonzero digit, or is literally `0` |
| Octal | `0377` | Starts with `0` followed by more digits |
| Hexadecimal | `0xFF` | Starts with `0x` or `0X` |

All integer constants are 16-bit values. There is no `L` or `U` suffix.
A constant with the high bit set (e.g. `0x8000`) is automatically treated as
`UNSIGNED` in the expression evaluator.

### 3.2 Character constants

Single-quoted character constants support all the usual escape sequences plus
a Micro-C extension: **two-character constants**.

```c
'A'      /* 0x41 */
'\n'     /* 0x0A */
'\xFF'   /* 255  */
'AB'     /* 0x4142 — Micro-C extension: two bytes packed into one 16-bit value */
```

Multi-character constants are useful for encoding two-byte command codes or
opcodes as a single integer constant. This is intentional and documented.

Supported escape sequences inside character and string literals:

| Sequence | Value | Description |
|----------|-------|-------------|
| `\n` | 0x0A | Newline |
| `\r` | 0x0D | Carriage return |
| `\t` | 0x09 | Tab |
| `\0` | 0x00 | Null |
| `\\` | 0x5C | Backslash |
| `\"` | 0x22 | Double quote |
| `\'` | 0x27 | Single quote |
| `\xNN` | 0xNN | Hex escape |
| `\a` | 0x07 | Bell (**[port]**) |
| `\v` | 0x0B | Vertical tab (**[port]**) |

### 3.3 String literals

String literals are stored in a pool (`LITER_POOL`, 65535 bytes by default).
Each string gets an offset into the pool as its value; the code generator emits
them as `FCB` sequences at the end of the output.

**Adjacent string literals are automatically concatenated:**

```c
"hello" " world"   /* becomes a single "hello world\0" */
```

With the `-f` (fold) flag, duplicate string literals are merged — later
occurrences point to the same pool entry as the first. This saves ROM space at
the cost of a linear scan at compile time.

---

## 4. Operators and expressions

### 4.1 Operator precedence

Higher number = tighter binding. Evaluated left-to-right within the same priority.

| Priority | Operators |
|----------|-----------|
| 15 | `++` `--` `.` `->` (postfix) |
| 13 | `*` `/` `%` |
| 12 | `+` `-` |
| 11 | `<<` `>>` |
| 10 | `<=` `>=` `<` `>` |
| 9 | `==` `!=` |
| 8 | `&` (bitwise AND) |
| 7 | `^` |
| 6 | `\|` |
| 5 | `&&` |
| 4 | `\|\|` |
| 3 | `?:` |
| 2 | `=` `+=` `-=` `*=` `/=` `%=` `&=` `\|=` `^=` `<<=` `>>=` |

Note that unary prefix operators (`*`, `&`, `!`, `~`, unary `-`, prefix `++`/`--`)
are handled at parse time, not in this table. They bind tighter than any binary
operator.

### 4.2 Supported operators

All standard C arithmetic, bitwise, logical, comparison, and assignment operators
are supported. Specifically:

- **Arithmetic:** `+`, `-`, `*`, `/`, `%`, unary `-`
- **Bitwise:** `&`, `|`, `^`, `~`, `<<`, `>>`
- **Logical:** `&&`, `||`, `!`
- **Comparison:** `==`, `!=`, `<`, `<=`, `>`, `>=` (signed and unsigned variants)
- **Assignment:** `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, `^=`, `<<=`, `>>=`
- **Increment/decrement:** `++`, `--` (prefix and postfix)
- **Pointer/member:** `*` (dereference), `&` (address-of), `[]`, `.`, `->`
- **Ternary:** `?:`
- **Comma:** not supported as a sequence operator; do not use

> **[port]** `+` and `-` (including `+=` and `-=`) scale the integer operand
> by `sizeof(*ptr)` when one operand is a non-char pointer. `++` and `--`
> also scale. `[]` has always scaled. See [§13.4](#134-pointer-arithmetic-only---and--scale-in-the-original) for struct pointer caveats.

### 4.3 What the code generator emits

Most arithmetic is **inline**. Some operations dispatch to runtime library
routines in the prefix file:

| Operation | Emitted as |
|-----------|------------|
| `+`, `-` | `ADDD`, `SUBD` inline |
| `& \| ^` | `ANDA`/`ANDB`, `ORA`/`ORB`, `EORA`/`EORB` inline |
| `* 2` | `LSLB / ROLA` inline (strength reduction) |
| `* 4`, `* 8` | Inline shift pairs (**[port]**) |
| `== 0`, `!= 0` | `CMPD #0` or `TSTB` inline (**[port]**) |
| `*` (general) | `JSR ?mul` |
| `/` | `JSR ?div` (unsigned) or `JSR ?sdiv` (signed) |
| `%` | `JSR ?mod` / `JSR ?smod` |
| `<<`, `>>` | `JSR ?shl` / `JSR ?shr` |
| `==`, `!=` | `JSR ?eq` / `JSR ?ne` |
| `<`, `<=`, `>`, `>=` | `JSR ?lt` etc. (signed) or `JSR ?ult` etc. (unsigned) |
| `!` | `JSR ?not` |

### 4.4 Casts

Supported cast targets: `(int)`, `(unsigned)`, `(char)`, `(const)`,
`(register)`, `(void)`. These are the only tokens the cast parser recognises
after `(`. Typedef names such as `(uint8_t)` are not recognised here — they
fall through to subexpression parsing and produce "Undefined symbol". This is
K&R-era behaviour; use the underlying primitive type in casts.

**`(unsigned char)` is not a supported cast.** To isolate the low byte as
unsigned, use `x & 0xFF`.

Cast behaviour:

| Cast | Source | Effect |
|------|--------|--------|
| `(char)expr` | 16-bit in register | Truncates to low 8 bits, then sign-extends to 16 |
| `(int)char_var` | `char` from memory | `LDB` + `SEX` (sign-extend) |
| `(int)unsigned_char_var` | `unsigned char` from memory | `LDB` + `CLRA` (zero-extend) |
| `(unsigned)expr` | any | Sets UNSIGNED flag; no instruction |
| `(int)expr` | any | Clears UNSIGNED flag; may widen if source is byte |
| `(const)` `(register)` `(void)` | any | Type bits only; no instruction |

> **[port]** `(int)(char)expr` — applying `(char)` to an already-in-register
> value now correctly truncates and sign-extends. Previously it was a no-op.
>
> **[port]** `(int)unsigned_char_member` now zero-extends (`CLRA`) instead of
> sign-extending (`SEX`). The `UNSIGNED` qualifier is preserved through the
> cast widening path.

### 4.5 sizeof

`sizeof` is evaluated at compile time and never emits any instructions.
It is aware of all types including multi-dimensional arrays, structs, unions,
and pointer types.

```c
sizeof(int)          /* 2 */
sizeof(char)         /* 1 */
sizeof(long)         /* 4  [port] */
sizeof(int[4][4])    /* 32 */
sizeof(struct Point) /* 4 — two ints */
sizeof(int *)        /* 2 — all pointers are 16-bit */
```

---

## 5. Declarations and scope

### 5.1 Global variables

Global variables are declared at file scope (outside any function) and persist
for the life of the program. They are stored in the BSS/data section of the
linked output.

```c
int counter;
unsigned char flags;
char name[32];
struct Point origin;
```

Initialised globals are supported:

```c
int version = 3;
char greeting[] = "Hello";
int table[] = { 1, 2, 3, 4 };
```

### 5.2 Local variables

Local variables are allocated on the stack in the function prologue.

**Local variables cannot be initialised in their declaration.** This is not an
oversight — it is a fundamental property of the single-pass compiler. The
workaround is to assign after declaration:

```c
/* WRONG — compile error */
int count = 0;

/* CORRECT */
int count;
count = 0;
```

The exception is `static` locals, which are global storage under the hood and
can be initialised.

### 5.3 Storage classes

| Keyword | Effect |
|---------|--------|
| `static` (local) | Global storage, local name scope. Persists across calls. Can be initialised. |
| `static` (global) | Same as global (no linkage concept across translation units here) |
| `extern` | Declares a symbol defined elsewhere; the name is emitted in the assembly as an external reference |
| `register` | Hint to use register passing convention for `printf`/`concat`-style variadics; has special meaning in this toolchain — see [§7.3](#73-register-functions-and-variadics) |
| `const` | Sets the CONSTANT type bit; no read-only enforcement in the compiler |

### 5.4 Symbol table layout

Globals grow upward from index 0; locals grow downward from `MAX_SYMBOL`
(default 4000). They meet in the middle. With `MAX_SYMBOL = 4000` you have
a combined budget of 4000 entries for all globals plus all locals currently
in scope.

Symbol names are significant to **31 characters** (controlled by `SYMBOL_SIZE`).

---

## 6. Statements

### 6.1 if / else

Standard C `if`/`else` with optional `else if` chaining:

```c
if(x > 0)
    do_positive();
else if(x < 0)
    do_negative();
else
    do_zero();
```

### 6.2 while and do/while

```c
while(condition)
    body;

do {
    body;
} while(condition);
```

### 6.3 for

```c
for(init; condition; step)
    body;
```

Note that `init` is a statement, not a declaration — you cannot declare a
variable in the `for` initialiser (`for(int i = 0; ...)` is not supported).
Declare `i` before the loop.

### 6.4 switch

```c
switch(expr) {
    case 1:
        ...
        break;
    case 2:
        /* fall-through to case 3 */
    case 3:
        ...
        break;
    default:
        ...
}
```

`switch` uses table dispatch internally. The compiler emits `LDU #?table /
JMP ?switch`; the `?switch` runtime routine walks a `(value, label)` pair
table and does an indirect jump. There is no limit on the number of cases
other than ROM space.

The expression in `switch` must be an `int` or `char`. Switching on a `long`
is not supported.

### 6.5 break and continue

`break` exits the innermost `switch`, `while`, `do`, or `for`. `continue`
skips to the next iteration of the innermost loop. Both work in nested loops
as expected.

### 6.6 return

`return expr;` returns a value from a function. The expression result ends up
in D (16-bit) or B (8-bit) depending on the return type. `return;` (no
expression) is valid for `void` functions.

### 6.7 goto

`goto label;` and `label:` are supported. Labels must be in the same function.
There is no `computed goto`. The maximum number of active goto labels is bounded
by the expression stack depth; in practice there is no meaningful limit.

---

## 7. Functions

### 7.1 Declaration styles

Both K&R and ANSI styles are accepted. K&R is the native style — it's what
Dunfield used throughout the library and runtime.

**K&R style:**
```c
add(a, b)
    int a; int b;
{
    return a + b;
}
```

**ANSI style:**
```c
int add(int a, int b)
{
    return a + b;
}
```

You can mix styles across translation units. The compiler does not enforce
argument types at call sites — it trusts you, as K&R C does.

Functions can have at most `MAX_ARGS` (25) parameters.

### 7.2 Nested functions

Nested function definitions are not supported. All functions must be at file
scope.

### 7.3 register functions and variadics

Micro-C's mechanism for variable-argument functions is the `register` storage
class combined with the `nargs()` intrinsic. Any function declared `register`
receives the argument count in D before the JSR:

```c
register printf(fmt)
    char *fmt;
{
    /* nargs() returns the number of arguments passed */
}
```

In practice you declare `printf`, `sprintf`, and `concat` as `register` and
use `stdarg.h` macros to walk the argument list. The `mcp` preprocessor is
required for `stdarg.h` (`cc09 -P`).

The `nargs` label in the runtime is just `RTS` — the optimizer strips `JSR nargs`
to nothing. The label exists so the peephole rule has something to match.

### 7.4 Function pointers

Complex declarators like `int (*fp)(int)` are not supported by the parser.
The correct Micro-C idiom is to declare a plain `int *` and call it with
the `(*ptr)()` or `ptr()` syntax — the compiler emits `JSR [ptr]` in
either case:

```c
int *fp;

fp = my_func;      /* assign address */
(*fp)(arg);        /* call — valid */
fp(arg);           /* also valid */
```

See [§13.6](#136-calling-any-pointer-value-as-a-function) for details and
the alternative switch-dispatch pattern when you need to store multiple
function addresses.

---

## 8. Preprocessor

The toolchain provides **two preprocessors**. Understanding which one you have
available is important.

### 8.1 Built-in preprocessor (mcc09)

The compiler has a simple preprocessor built in. It handles:

| Feature | Example |
|---------|---------|
| Object-like `#define` | `#define BUFSIZE 256` |
| `#ifdef` / `#ifndef` / `#endif` | Include guards |
| `#include "file"` | Local includes |
| `#include <file>` | Searches `MCINCLUDE` path |
| `#undef` | `#undef BUFSIZE` (**[port]**) |
| `#error message` | Compile-time fatal (**[port]**) |
| `#warning message` | Compile-time warning, continues (**[port]**) |
| `#line N "file"` | Line number / filename override (**[port]**) |
| `__FILE__` | Current filename as string literal (**[port]**) |
| `__LINE__` | Current line number as integer (**[port]**) |

The built-in preprocessor does **not** support:
- Parameterised (function-like) macros: `#define MAX(a,b) ...`
- `##` token-paste
- `#if` with expressions (`#if VERSION >= 3`)
- `#elif`

If you need any of these, use `mcp`.

### 8.2 Full preprocessor (mcp) — `cc09 -P`

`mcp` is a complete, standalone C preprocessor. Invoke it by passing `-P` to
the `cc09` coordinator, or run it manually before `mcc09`.

Everything the built-in preprocessor supports, plus:

| Feature | Example |
|---------|---------|
| Parameterised macros | `#define MAX(a,b) ((a)>(b)?(a):(b))` |
| Multi-line macros | `\` continuation |
| `##` token-paste | `#define REG(b,o) b##o` |
| `#if` with expressions | `#if VERSION >= 3` |
| `#elif` | `#elif defined(COCO)` |
| `#undef` / `#forget` | Undefine one or a block |
| `#error` / `#message` | Compile-time diagnostics |
| `__LINE__`, `__FILE__`, `__TIME__`, `__DATE__`, `__INDEX__` | Predefined symbols |

`mcp` command-line options:

| Option | Effect |
|--------|--------|
| `-I<path>` | Add include search path (also `l=<path>`) |
| `-c` | Keep comments in output |
| `-d` | Warn on duplicate macro definitions |
| `-l` | Emit `#line` directives for error tracking |
| `-q` | Quiet |
| `NAME=value` | Command-line macro definition |

The `MCINCLUDE` environment variable sets the default include search path for
both `mcc09` and `mcp`.

### 8.3 Headers that require mcp

Several headers use parameterised macros or `#if` and therefore require `cc09 -P`:

| Header | Requires mcp because... |
|--------|------------------------|
| `ctype.h` | Macro versions of `isdigit`, `isalpha`, etc. use `#define foo(x)` |
| `setjmp.h` | `jmp_buf` typedef uses parameterised macros |
| `stdarg.h` | `va_start`, `va_arg`, `va_end` use parameterised macros |
| `assert.h` | `assert(c)` uses parameterised macros |

Using any of these without `-P` will produce cryptic errors from `mcc09`'s
limited built-in preprocessor.

---

## 9. Inline assembly

Micro-C provides two forms of inline assembly via the `asm` keyword. Both
emit raw text directly into the assembly output. There is no operand
constraint system — you manage registers yourself.

### 9.1 Statement form

A block of assembly lines inside `{}`:

```c
asm {
    LDA  #$FF
    STA  $C000
}
```

The block is emitted verbatim into the output at the point of the `asm`
statement. Lines are separated by `\n` within the string. This is the
standard way to call ROM routines, manipulate hardware registers, or
insert timing-critical sequences.

### 9.2 Expression form

`asm(expr, expr, ...)` pushes each expression result onto the hardware stack
before emitting the inline code. The expressions are evaluated in order and
each is pushed as a 16-bit word with `PSHS A,B`. The result of the asm
expression is whatever is left in D when control returns.

```c
result = asm(a, b) {
    LDD  4,S    /* second argument (a) */
    ADDD 2,S    /* add first argument (b) — note reversed order on stack */
};
```

The arguments are accessible at positive offsets from S: `2,S` is the last
argument pushed (rightmost), `4,S` is the second-to-last, and so on.
The compiler does **not** clean the stack after an expression `asm` — you
are responsible for leaving S correct on exit.

### 9.3 Practical notes

- **U register is the frame pointer** for functions with local variables.
  If you trash U in inline asm inside such a function, locals will be read
  incorrectly. Save and restore U if needed.
- **Y and DP are unused** by the compiler. They are safe to use in asm
  blocks without saving, within a single function.
- **X is the second parameter** for runtime library calls. It is clobbered
  by any expression that invokes a runtime routine (`?mul`, `?div`, etc.);
  don't rely on X across expressions.
- **D is always the return value.** Leave the result in D before exiting
  an expression-form `asm`.

---

## 10. Calling convention and ABI

### 10.1 Argument passing

Arguments are pushed **left-to-right** onto the hardware stack (S). This is
the opposite of cdecl (which pushes right-to-left). The consequence is that
the first argument in the source lands at the **highest** stack offset, not
the lowest.

```c
foo(a, b, c);
```

Produces:

```asm
    LDD  a
    PSHS A,B      ; push a first
    LDD  b
    PSHS A,B      ; push b second
    LDD  c
    PSHS A,B      ; push c last (nearest S on entry to foo)
    JSR  foo
    LEAS 6,S      ; caller cleans: n_args * 2 bytes
```

Inside `foo`, after the prologue `LEAS -N,S` that allocates locals:

```
     2,S  →  c  (last arg pushed, nearest to current S)
     4,S  →  b
     6,S  →  a  (first arg, farthest from current S)
```

This left-to-right push convention is documented Micro-C behaviour. It affects
any multi-argument library function — notably `strcmp`, which loads its arguments
in reversed order. See [TESTS_ADDENDUM.md](TESTS_ADDENDUM.md) for the practical
implication.

All arguments are passed as **16-bit words**. A `char` argument is zero/sign-
extended to 16 bits before being pushed.

### 10.2 Return values

| Return type | Location |
|-------------|----------|
| `int`, `unsigned`, pointer | D register (A:B, 16-bit) |
| `char` | B register (8-bit) |
| `void` | (nothing) |

There is no mechanism for returning a struct by value — return a pointer instead.

### 10.3 Register usage

| Register | Role |
|----------|------|
| D (A:B) | Primary accumulator — all arithmetic, return value |
| U | Frame pointer / index register — address computation, pointer loads |
| X | Second parameter for runtime library calls; caller-saved |
| S | Hardware stack pointer |
| Y | **Not used by the compiler** — safe to use freely in asm |
| DP | **Not used by the compiler** — safe to use freely in asm |

### 10.4 Stack frame layout

```
[high addresses]
   arg_1        <- a,  at highest offset from S after prologue
   arg_2        <- b
   ...
   arg_n        <- n,  at lowest arg offset
   return_addr  <- 2 bytes (pushed by JSR)
   local_1      <- first local variable
   local_2
   ...
   local_m      <- last local, nearest to current S
[low addresses, S points here]
```

The prologue emits `LEAS -N,S` (where N = total bytes of locals). The epilogue
emits `LEAS N+stack_level,S` then `RTS`.

---

## 11. Compiler limits

All table sizes are compile-time constants in the compiler source. These are the
values for this port:

| Parameter | Value | What it controls |
|-----------|-------|-----------------|
| `LINE_SIZE` | 512 | Maximum source line length (bytes) |
| `FILE_SIZE` | 256 | Maximum filename length |
| `SYMBOL_SIZE` | 31 | Significant characters in a symbol name |
| `MAX_SYMBOL` | 4000 | Symbol table entries (globals + locals combined) |
| `INCL_DEPTH` | 10 | Maximum `#include` nesting depth |
| `DEF_DEPTH` | 10 | Maximum `#define` macro expansion nesting |
| `EXPR_DEPTH` | 20 | Expression operator stack depth |
| `LOOP_DEPTH` | 20 | Maximum nested loops |
| `MAX_ARGS` | 25 | Parameters per function |
| `MAX_SWITCH` | 50 | Active `switch`/`case` nesting |
| `MAX_DIMS` | 2000 | Array dimension entries |
| `MAX_DEFINE` | 500 | `#define` symbols (built-in preprocessor only) |
| `DEFINE_POOL` | 8192 | `#define` string space (bytes) |
| `LITER_POOL` | 65535 | String literal pool (bytes) |
| `MAX_TYPEDEF` | 128 | `typedef` names |
| `MAX_ERRORS` | 25 | Errors before forced abort (override with `-eN`) |

> **[port]** The original DOS values for several of these were much smaller.
> `MAX_SYMBOL` was ~256, `LITER_POOL` was ~4096, `LINE_SIZE` was 80.
> All were bumped for modern use.

> **[port]** The `-eN` flag overrides `MAX_ERRORS` at runtime. Useful for
> seeing all errors in a large translation unit instead of stopping at 25.

### What's not supported — quick reference

For readers coming from modern C, a condensed list of things that will
catch you out:

| Feature | Status |
|---------|--------|
| `float`, `double` | Not in the language at all |
| `long` arithmetic (`a + b` where both are `long`) | Storage only; use LONGMATH |
| `enum` | Not supported |
| Bitfields | Not supported — use masks and shifts: `x & 0x0F`, `x >> 4` |
| Initialised local variables | Compile error — assign after declaration |
| Variable-length arrays | Not supported — all dimensions must be constants |
| Struct assignment (`s2 = s1`) | "Non-assignable" — copy field by field or use `memcpy` |
| `(unsigned char)` cast | Not supported — use `& 0xFF` |
| Complex declarators (`int (*fp)(int)`) | Not supported |
| Nested function definitions | Not supported |
| `printf` to `stdout` | No stdio — `printf` writes to serial via `SERIO.ASM` |
| File I/O | No filesystem — implement against your target hardware |
| `//` line comments | Supported (documented Micro-C extension) |
| Nested `/* */` comments | Supported (documented Micro-C extension) |

---

## 12. Standard headers

All headers live in `include/` and are found via the `MCINCLUDE` environment
variable. Headers marked **[mcp]** cannot be used without `cc09 -P`.

| Header | Requires mcp | What it provides |
|--------|:---:|-----------------|
| `6809io.h` | — | `_CPU_` = 6809, `NULL`, `EOF`; `extern register` decls for `printf`, `sprintf`, `concat` |
| `stddef.h` | — | `NULL`, `size_t` (= `unsigned`), `ptrdiff_t` (= `int`) |
| `limits.h` | — | `CHAR_MIN/MAX`, `SCHAR_MIN/MAX`, `UCHAR_MAX`, `INT_MIN/MAX`, `UINT_MAX`, `CHAR_BIT` |
| `stdint.h` | — | `int8_t` … `uint32_t`, `intptr_t`, `uintptr_t`, `intmax_t`, `uintmax_t`, all `_MIN`/`_MAX` constants |
| `stdbool.h` | — | `bool` (= `unsigned char`), `true` = 1, `false` = 0 |
| `ctype.h` | ✓ | Macro-based `isdigit`, `isalpha`, `isupper`, `islower`, `isspace`, `iscntrl`, `ispunct`, `isalnum`, `isgraph`, `isxdigit` — backed by a 129-byte lookup table |
| `stdarg.h` | ✓ | `va_func`, `va_list`, `va_start`, `va_arg`, `va_end` |
| `setjmp.h` | ✓ | `jmp_buf` (4 bytes on 6809: U + return address); links against `LONGJMP.ASM` |
| `assert.h` | ✓ | `assert(c)` — no-op unless `DEBUG` is defined; also needs a stdio header for `EOF` |
| `6809int.h` | ✓ | `INTERRUPT(vec)` macro — generates the stub that installs a C function as an interrupt handler |

### 12.1 6809io.h — the usual starting point

Include this first in most programs. It sets `_CPU_` to 6809 (used by
`setjmp.h` and other headers to select the right configuration), and
declares `printf`, `sprintf`, and `concat` as `register extern` so the
compiler knows they are variadic:

```c
#include <6809io.h>

main()
{
    printf("hello %s\n", "world");
}
```

Without `6809io.h`, calls to `printf` will compile but the compiler won't
know to pass the argument count in D, so variadic argument walking will
produce garbage.

### 12.2 Interrupt handlers — 6809int.h

`6809int.h` provides the `INTERRUPT(vec)` macro, which requires mcp. It
generates a short stub at the interrupt vector address that saves the
temporary location, calls your C function, and returns with `RTI`:

```c
#include <6809int.h>
#include <6809io.h>

INTERRUPT(65524)        /* 0xFFF4 — IRQ vector */
serial_irq()
{
    /* Handle the interrupt. Don't use printf here — not reentrant. */
    putchr(inchar());
}
```

The number passed to `INTERRUPT` is the **decimal** address of the vector.
The `##` token-paste in the macro body requires mcp.

### 12.3 ctype.h vs CTYPE.ASM

`ctype.h` provides **macro** versions backed by a 129-byte classification
table. The table covers ASCII 0–127 plus EOF (index 0). The macros are
fast but consume the table space even if you call only one function.

If you need only one or two classification tests and RAM is tight, use the
**function** versions from `CTYPE.ASM` / `ISFUNS.ASM` directly — they
are smaller because they encode the logic in assembly rather than a table.

### 12.4 Variable arguments — stdarg.h

Micro-C's variadic mechanism differs from C99 in argument order and
pointer direction. The key insight: **the named argument in a `va_func`
function is the last positional argument, not the first**. `va_arg`
walks the stack backwards (toward lower addresses) from that anchor:

```c
#include <6809io.h>
#include <stdarg.h>

va_func my_sum(last_int)       /* va_func = register */
    int last_int;
{
    va_list p;
    int total, i, count;

    count = nargs();           /* total argument count */
    va_start(p, last_int);    /* p = nargs()*2 + &last_int */

    total = 0;
    i = count;
    while(i--) {
        total += va_arg(p, int);   /* *--p */
    }
    return total;
}
```

`va_start(p, last_arg)` sets `p` to point just past the last argument
(i.e. to the first argument in source order). `va_arg(p, t)` pre-decrements
then dereferences, so it retrieves from first to last. After all
arguments are consumed, `va_end` is a no-op.

---

## 13. Quirks and pitfalls

This section documents the ways Micro-C deviates from what modern C
programmers expect. Most of these are documented in Dunfield's original
manual and are intentional design decisions, not bugs. Understanding them
up front will save significant debugging time.

### 13.1 Characters are not promoted to int

**This is the single most common source of subtle bugs in Micro-C.**

In standard C, character values are promoted to `int` before being used
in arithmetic. Micro-C does the opposite: it keeps character expressions
as 8-bit operations for efficiency, and only promotes to 16 bits when
necessary.

```c
char c = 100;
return c + 1;       /* adds two BYTES, promotes to int only for return */
return c & 255;     /* byte & byte = byte, then promoted: result -128 to 127 NOT 0 to 255 */
return (int)c & 255;/* force int arithmetic: result 0 to 255 — correct */
return (unsigned)c; /* most efficient: zero-extends and marks unsigned */
```

The practical consequence: code that uses `& 0xFF` to extract an unsigned
byte value from a `char` does not work as expected. Cast to `int` or
`unsigned` first, or declare the variable as `unsigned char`.

### 13.2 Constants are truncated to match char operands

A related quirk: when an untyped integer constant appears in an expression
with a `char` operand, the constant is **truncated to 8 bits** to match.

```c
char c;
0x1234 / c;          /* becomes 0x34 / c — 8-bit arithmetic */
(int)0x1234 / c;     /* becomes 0x1234 / c — 16-bit arithmetic */
```

If you need a full 16-bit constant in an expression involving chars, cast
the constant explicitly.

### 13.3 Adding a signed char to an int assumes the char is positive

The code generator uses unsigned carry propagation when adding a `char` to
an `int`. This is efficient and correct as long as the char value is
non-negative. If the char might be negative, cast it to `int` first:

```c
int i;
char c;
i += c;          /* fast, but c is treated as unsigned — wrong if c < 0 */
i += (int)c;     /* sign-extends first — correct for negative c */
```

### 13.4 Pointer arithmetic: only ++, --, and [] scale

**This applies to the original behaviour.** The `+=` compound assignment
operator does **not** scale by `sizeof(*ptr)`. Only `++`, `--`, and `[]`
(subscript) apply element-size scaling.

```c
int arr[10];
int *p = arr;

++p;             /* advances by 2 — correct */
p[3];            /* addresses arr[3] — correct */
p += 5;          /* advances by 5 BYTES, not 10 — still wrong even in this port */
p = &p[5];       /* advances by 10 bytes — the safe idiom */
```

> **[port]** The binary `+` and `-` operators now scale correctly (e.g.
> `p + 5` and `p += 5` both advance by the right number of bytes). The
> compound assignment operators `+=` and `-=` fall through to the same
> code path and are also fixed. The test suite (`t15_ptrscale.c`) covers
> `*(p+n)`, `*(p-n)`, and variable offsets but does not yet have a
> dedicated `p += n` test case.

Struct pointers still behave as the original: incrementing a `struct Foo *`
with `++` advances only 1 byte, because structs are internally represented
as `char` arrays. Use `ptr = &ptr[1]` or `ptr += sizeof(struct Foo)` to
advance to the next struct instance — `sizeof` scaling via `+=` works
correctly since the right-hand side becomes a constant integer.

### 13.5 Built-in preprocessor: #define stops at the first space

When using the built-in preprocessor (without `-P`), the value of a
`#define` is taken only up to the first whitespace character:

```c
#define APLUSONE A+1       /* value is "A+1" — correct */
#define APLUSONE A +1      /* value is "A" — only the first token! */
```

If your macro value contains spaces, use `mcp` (`cc09 -P`) instead.

As a workaround for inserting syntactic spaces in the built-in preprocessor,
use `/**/` which is treated as an empty comment:

```c
#define BYTE unsigned/**/char    /* expands to "unsigned char" */
```

Comments are also stripped **after** the `#` command line is processed by
the built-in preprocessor. This means a `#define` value that starts with
`/*` will include the comment delimiter as its value:

```c
#define NULL    /* comment */    /* NULL becomes "/*" — WRONG */
#define NULL    0               /* correct */
```

### 13.6 Calling any pointer value as a function

Micro-C allows you to call any value that looks like an address by
following it with `()`. This is the standard idiom for function pointers,
since complex declarators like `int (*fp)(int)` are not supported:

```c
int *fp;          /* declare as plain pointer-to-int */
fp = my_func;     /* assign the address */
(*fp)(arg);       /* call it — valid Micro-C */
fp(arg);          /* also valid */
```

The compiler emits `JSR [fp]` (indirect call through the pointer variable).
You don't need a separate function-pointer typedef or cast.

### 13.7 extern symbols used only in asm must be declared in the asm

The compiler only emits an `EXTERN` assembler directive for symbols
that are actually referenced in the C source. If your inline assembly uses
an external symbol that nothing in the C code references, the assembler
will produce an "undefined symbol" error. Declare it in the assembly block
instead:

```c
/* WRONG: extern only works if referenced in C code */
extern rom_routine();
asm { JSR rom_routine }    /* may not emit EXTERN rom_routine */

/* CORRECT: declare it where you use it */
asm {
    EXTERN rom_routine
    JSR    rom_routine
}
```

### 13.8 K&R argument declarations are mandatory — they're not defaulted

In K&R style, arguments listed in the function header are positional
placeholders only. They must be explicitly re-declared between the header
and the opening brace, or the compiler will not know their types:

```c
/* WRONG — a and b are not declared */
add(a, b)
{
    return a + b;
}

/* CORRECT */
add(a, b)
    int a; int b;
{
    return a + b;
}
```

If you omit the declarations, the compiler will still process the function
but argument access will be wrong.

### 13.9 No complex declarators

Micro-C does not parse declarations that use parentheses for grouping,
such as pointer-to-function or array-of-pointer:

```c
int (*fp)(int);     /* not supported — parse error */
int *arr[10];       /* array of pointers — this IS supported */
```

Use the `int *fp;` / `(*fp)()` idiom described in §13.6 for function
pointers.

### 13.10 Local variable declarations must precede all statements

All local variables must be declared at the top of the function body,
before any executable statement. This is standard K&R C behaviour but
catches programmers used to C99's ability to declare variables anywhere:

```c
main()
{
    int i;
    int j;       /* all declarations here */

    i = 0;       /* statements after */
    j = i + 1;
}

/* WRONG — declaration after a statement */
main()
{
    int i;
    i = 0;
    int j;       /* compile error: "Declaration must precede code" */
}
```

---

## See Also

- [**README**](README.md) — Toolchain overview and build instructions
- [**Compiler Internals**](COMPILER.md) — Architecture, type system bit layout, code generation detail
- [**Standard Library**](STDLIB.md) — Runtime routines, serial I/O, string and math functions
- [**Test Suite Addendum**](TESTS_ADDENDUM.md) — Discovered quirks, bug fixes, and workarounds

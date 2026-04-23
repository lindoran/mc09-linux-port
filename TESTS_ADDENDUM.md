# Micro-C 6809 Linux Port — Test Suite Addendum

[← Back to README](README.md)

This document records findings discovered while building and running the test
suite against the usim09 target. Each finding describes a deviation from
standard C behaviour, a compiler limitation, or a bug in the original or ported
toolchain, along with the workaround used in the tests.

---

## Summary of Core Bug Fixes

The following critical bugs were identified and resolved during the porting and
validation process. These fixes ensure the compiler behaves correctly for standard
C patterns that previously failed.

| Fix | Impact | File(s) |
|-----|--------|---------|
| **`typedef` Side-Channel** | Resolved "Non-assignable" errors on globals following struct pointer typedefs. | `compile.c` |
| **ROM Global Writable** | Updated `mktarget` to place globals in RAM (`$0100`) for usim09/CoCo targets. | `mktarget`, `targets/*.cfg` |
| **`mcp` `#undef`** | Allowed `#undef` of undefined macros to be a no-op (C standard compliant). | `mcp.c` |
| **`&struct_var`** | Fixed compiler rejection of taking the address of a struct variable. | `compile.c` |
| **Comment Scanning** | Fixed infinite loops/crashes with multi-line comments spanning file boundaries. | `compile.c` |
| **`(char)` cast no-op** | `(int)(char)expr` now correctly truncates and sign-extends in-register values. Added `narrow_byte()` to set `last_byte` without re-emitting a load. | `6809cg.c`, `compile.c` |
| **`unsigned char` sign-extends** | `(int)unsigned_char_member` now zero-extends (`CLRA`) instead of sign-extending (`SEX`). Source `UNSIGNED` bit preserved through the cast widening path. | `compile.c` |

---

## Running the tests

```sh
cd tests/
./run_tests.sh          # all 15 suites
./run_tests.sh t06      # one suite by prefix
```

Requires `make` to have been run in the root directory and `usim09` to be on
PATH or present in the root directory. Each test file is compiled with
`cc09 -PIq` (preprocessor + Intel HEX + quiet) then run under usim09.

---

## Bug fix: `mktarget` did not generate `6809RLM.ASM`

**Severity:** Critical — all ROM targets produced by `mktarget` had unwritable globals.

**Root cause:** `mktarget` generated `6809RLP.ASM`, `SERIO.ASM`, and `6809RLS.ASM`
from the config file, but treated `6809RLM.ASM` (the "middle" runtime file that
marks the start of the uninitialised variable region) as an invariant library file
and copied it unchanged from `lib09/`. The base `lib09/6809RLM.ASM` has the RAM
`ORG` commented out, so all global variables landed in the ROM region (`$E000+`)
on both the usim09 and CoCo targets. Writes to ROM are silently discarded by the
simulator and the real hardware.

**Fix:** `mktarget` now accepts a `RAM_ORG` config parameter (defaulting to
`$0100`) and generates `6809RLM.ASM` with an explicit `ORG`. Both
`targets/usim09/usim09.cfg` and `targets/coco/coco.cfg` have been updated with
appropriate `RAM_ORG` values. The base `lib09/6809RLM.ASM` retains its
commented-out ORG with an explanation that it is only correct for the default
RAM-only target.

**Workaround for manual targets:** If creating a target directory by hand (not
via `mktarget`), always include an explicit `ORG <ram_address>` in
`6809RLM.ASM` before the `$FS:` marker.

---

## Bug fix: `mcp` — `#undef` of undefined macro was a fatal error

**Severity:** Moderate — `#undef SYMBOL` where `SYMBOL` was never defined caused
`mcp` to abort with "Undefined macro".

**Standard C behaviour:** `#undef` of an undefined name is explicitly a no-op
(C89 §3.8.3, C99 §6.10.3.5).

**Fix:** Changed `lookup_macro(-1)` to `lookup_macro(0)` in the `#undef`
handler in `mcp.c`. The `-1` flag caused `lookup_macro` to emit an error on
not-found; `0` makes it return silently.

---

## Compiler limitation: `*(p+n)` does not scale for non-char pointers

**Affects:** `int *p; *(p+n)` — adds `n` bytes, not `n * sizeof(int)`.

**What works:** `p[n]` subscript syntax scales correctly. `++p`, `--p`,
`p++`, `p--` also scale correctly.

**Example:**
```c
int arr[4]; int *p; p = arr;
*(p+1)   /* WRONG: reads arr[0] high byte + arr[1] low byte — garbage */
p[1]     /* correct: reads arr[1] */
```

**Fix in tests:** All random-access index operations on `int*` use `p[n]`.
Linear walks use `++` / `--`. Reverse-in-place uses index variables instead
of two pointer variables being decremented toward each other.

---

## Bug fix: `(char)` cast of in-register values was a no-op

**Fixed in:** `6809cg.c` (`narrow_byte()`) and `compile.c` (cast handler).

**Was broken:** `(int)(char)some_int_var` and `(int)(char)0x0141` did not
truncate or sign-extend. The `(char)` typecast set the BYTE flag on the
expression type, but the code generator only emitted `SEX` when loading
a BYTE-typed symbol from *memory*. Casting an already-in-register value
did not re-emit the load, so `last_byte` stayed zero and `expand()` was
a no-op.

**Fix:** Added `narrow_byte()` to `6809cg.c`, which sets `last_byte`
without emitting any instruction (B already holds the low 8 bits of D).
The cast handler in `compile.c` calls `narrow_byte()` when narrowing
int→char, then `expand()` emits `SEX` (or `CLRA` for unsigned) correctly.

**After fix — all these work directly:**
```c
(int)(char)0x0141   /* → 65    (high byte dropped, no sign issue) */
(int)(char)0x00FF   /* → -1    (sign-extended) */
(int)(char)0x0080   /* → -128  (sign-extended) */
```

**Storage bounce workaround** (still valid, now agrees with direct cast):
```c
char c; int n;
c = some_int;   /* stores low 8 bits */
n = c;          /* loads with LDB+SEX */
```

**To get the low byte as unsigned (no sign extension):**
```c
int lo = some_int & 0xFF;
```

---

## Compiler limitation: `(unsigned char)` is not a supported cast

The supported casts are: `(int)`, `(unsigned)`, `(char)`, `(const)`,
`(register)`, `(void)`. Attempting `(unsigned char)x` causes a compile error.

**Workaround:** Use `x & 0xFF` to isolate the low byte as unsigned.

---

## Compiler limitation: struct assignment (`s2 = s1`) not supported

Direct struct-to-struct assignment produces "Non-assignable".

**Workaround:** Copy field by field via pointers, or use `memcpy`.
```c
copy_point(dst, src)
    struct Point *dst; struct Point *src;
{ dst->x = src->x; dst->y = src->y; }
```

---

## Bug fix: `unsigned char` struct members sign-extended on read

**Fixed in:** `compile.c` (cast handler — widening path preserves `UNSIGNED`).

**Was broken:** `unsigned char` struct members sign-extended into int when
read via `(int)member`. The `UNSIGNED` bit was present in the source type
(`s_type[sptr]`), but `get_type(INT, 0)` in the cast target produced a
type with no `UNSIGNED` bit, so `expand()` always emitted `SEX` instead
of `CLRA`.

**Fix:** When the cast handler widens char→int, it now ORs the source
type's `UNSIGNED` bit into the type passed to `expand()`. If the original
member was `unsigned char`, `expand()` emits `CLRA` (zero-extend).

**After fix — works without masking:**
```c
struct Pixel { unsigned char r; unsigned char g; unsigned char b; };
struct Pixel px;
px.r = 255;
(int)px.r   /* → 255, not -1 */
px.g = 128;
(int)px.g   /* → 128, not -128 */
```

**The `& 0xFF` workaround** is no longer needed for `unsigned char` members
read via `(int)`, but remains valid for plain `char` members where unsigned
interpretation is wanted.

---

## Bug fix: pointer typedef followed by uint8_t global caused "Non-assignable"

**Trigger:** `typedef StructType *PtrType;` (or `typedef Vec2 Vec2Copy;` —
any typedef whose base type resolves through a previously-defined struct
typedef) followed by any scalar typedef global declaration.

**Symptom:** The subsequent global was flagged "Non-assignable" at compile
time, making it impossible to assign to it.

**Root cause:** `define_typedef()` has two branches — one for `struct/union`
typedefs and one for scalar typedefs. When the scalar branch calls
`get_type()` and that call resolves through a struct typedef, `get_type()`
sets the global side-channel `typedef_ssize` (the struct's member-size table
entry). Only `declare()` consumes and clears this value. `define_typedef()`
never cleared it, so it remained set as a stale value. When the very next
declaration was processed by `declare()`, the `if(typedef_ssize)` guard
fired immediately, treating the new variable as a struct and corrupting its
type — specifically setting `BYTE` and calling `test_next(SEMI)`, which
consumed the variable name token instead of a semicolon, leaving the type
bits in a state that caused "Non-assignable".

**Fix:** Added `if(typedef_ssize) typedef_ssize = 0;` at the end of the
scalar branch in `define_typedef()` in `compile.c`. One line. The side-channel
is flushed before control returns from `define_typedef()`.

After this fix, `typedef Vec2 *Vec2Ptr;` followed immediately by `uint8_t a;`
compiles and behaves correctly. The `t12_typedef.c` test now uses `Vec2Ptr`
directly.

---

## Library finding: `strcmp` sign inverted due to Micro-C's left-to-right argument push

**What the docs say:** `strcmp(s1, s2)` returns `1` if `s1 > s2`, `-1` if
`s1 < s2`.

**What happens:** `strcmp("abc", "abd")` returns `+1` even though abc < abd.

**Root cause:** This is not a K&R C behaviour — it is specific to Micro-C's
non-standard calling convention. Standard K&R/ANSI C (cdecl) pushes arguments
**right-to-left**, so the first argument lands nearest SP (`2,S`). Micro-C
pushes arguments **left-to-right** (explicitly documented in COMPILER.md), so
the first argument lands furthest from SP (`4,S`) and the second lands nearest
(`2,S`). `STRING2.ASM` loads "string1" from `2,S`, which is the *second* C
argument. Every Dunfield library function that takes multiple same-typed
arguments and compares them in order will share this property.

**Implication:** `strcmp` is reliable for equality (`== 0`) but not for
ordering. Do not rely on the sign for greater-than / less-than comparisons
unless you account for the inversion.

---

## Library finding: `sqrt` returns ceiling, not floor, for non-perfect squares

`sqrt(2)` returns `2`, `sqrt(15)` returns `4`. This is ceiling (round up)
behaviour, not the floor behaviour of standard `sqrt` on integers. Tests
use the actual ceiling values.

---

## Library finding: `sprintf` — negative number with field width: sign before padding

`sprintf(buf, "%5d", -1)` produces `"-   1"` (sign first, then spaces,
then digit), not `"   -1"` (spaces then signed number). Left-justify and
zero-fill work consistently: `"%-5d"` → `"-1   "`, `"%05d"` → `"-0001"`.

---

## ROM size constraint on usim09 target

The usim09 ROM region is `$E000–$FFF0` — approximately 4 KB. Pulling in
multiple large library modules (ISFUNS.ASM in particular, which implements
all `isXXX` character classification functions) alongside test code can
overflow this space, producing assembler "out of range" branch errors.

**Fix:** Split tests that use many library modules into separate files
(`t14_stdlib.c` and `t14b_ctype.c`) so each fits within the ROM budget.

---

## Function pointer storage limitation

Storing a function address in an `int` or `unsigned` variable causes "Type
clash". The compiler's type system marks function symbols with `FUNCGOTO`
in the `SYMTYPE` bits, and the assignment operator rejects assigning a
FUNCGOTO-typed value to a plain integer storage location.

**Supported idioms:**

1. **Switch dispatch** — idiomatic and portable:
   ```c
   dispatch(op, n) int op, n; {
       switch(op) { case 0: return fn_a(n); case 1: return fn_b(n); }
   }
   ```

2. **Inline asm to store and call** — load the function address via `LDD #label`
   and store to a global int, then JSR indirectly:
   ```c
   set_fp() { asm { LDD #my_func \n STD g_fp } }
   call_fp(n) int n; { asm { LDD 2,S \n PSHS A,B \n LDX g_fp \n JSR ,X \n LEAS 2,S \n STD result } return result; }
   ```

---

## Test file quick reference

| File | Coverage | Tests |
|------|----------|-------|
| `t01_controlflow.c` | if/else, while, for, do/while, break, continue, nested loops | 18 |
| `t02_switch.c` | switch/case/default, fall-through, char switch, switch-in-loop | 19 |
| `t03_pointers.c` | indexing, arithmetic, dereference, address-of, 2D arrays, sizeof | 22 |
| `t04_strings.c` | strlen, strcpy, strcat, strcmp (with inversion note), strchr | 26 |
| `t05_structs.c` | dot, arrow, nested, pass-by-value, array-of-structs, union, sizeof | 25 |
| `t06_bitwise.c` | &, \|, ^, ~, <<, >>, compound assigns, bit manip, popcount | 37 |
| `t07_casting.c` | (char) inline narrowing (Fix 1), unsigned char struct members (Fix 2), storage path, unsigned reinterpret, sign extension | 33 |
| `t08_funcptr.c` | switch dispatch, asm-indirect JSR,X, function address in asm | 17 |
| `t09_printf.c` | sprintf %d %u %x %s %c, field width, left-justify, zero-fill | 24 |
| `t10_preproc.c` | #define, parameterised macros, ##, #if, #ifdef, #undef, __LINE__ | 23 |
| `t11_static.c` | static locals (persist), static globals, const, accumulation | 19 |
| `t12_typedef.c` | typedef scalar/struct, uint8_t/int8_t zero/sign-extend (Fix 2), long, stdint.h, stdbool.h, longcmp | 24 |
| `t13_malloc.c` | malloc, free, overlap check, free+realloc, coalescing | 19 |
| `t14_stdlib.c` | abs, max, min, sqrt, memset, memcpy, atoi, toupper, tolower | 39 |
| `t14b_ctype.c` | isalpha, isdigit, isspace, isupper, islower, isalnum, ispunct, iscntrl | 31 |
---

## See Also

- [**README**](README.md) — Toolchain overview and build instructions
- [**Compiler Internals**](COMPILER.md) — Architecture and code generation
- [**Standard Library**](STDLIB.md) — Runtime routines and library documentation

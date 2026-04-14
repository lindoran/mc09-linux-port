# Micro-C 6809 Standard Library

This document covers the runtime library that ships with the 6809 toolchain —
what functions exist, what each one does, how they're organised, and the
important quirks you need to know to use them effectively.

The library lives in `lib09/` as individual `.ASM` source files. The source
linker (`slink`) pulls in only the modules actually referenced by your code,
so you only pay for what you use. Library modules are identified by
`EXTINDEX.LIB`, which maps public symbol names to the `.ASM` file that
defines them.

---

## How the library is structured

There are three special files that are not optional library modules —
they're always present and form the skeleton of every linked program:

| File | Role |
|------|------|
| `6809RLP.ASM` | **Prefix** — `ORG`, startup, runtime arithmetic, switch dispatch |
| `6809RLM.ASM` | **Middle** — RAM section marker (for ROM targets) |
| `6809RLS.ASM` | **Suffix** — heap marker, optional reset vector |

Everything else is an optional module pulled in on demand.

### The prefix (`6809RLP.ASM`)

This is what `slink s=CRT0.ASM` overrides. It contains:

- `?begin` — startup: `LDS` (stack init), `CLR ?heap`, `JSR main`, exit stub
- `?mul` — 16×16 → 16 bit multiply
- `?sdiv` / `?div` — signed and unsigned 16-bit divide
- `?smod` / `?mod` — signed and unsigned 16-bit modulo
- `?shl` / `?shr` — 16-bit shift left and right
- `?gt`, `?ge`, `?lt`, `?le` — signed 16-bit comparisons, return 0 or 1
- `?eq`, `?ne` — equality comparisons
- `?ugt`, `?uge`, `?ult`, `?ule` — unsigned comparisons
- `?not` — logical NOT (D == 0 → 1, else 0)
- `?switch` — switch table dispatch handler
- `?temp` — 2-byte scratch area used by the compiler

All of these are **compiler-generated calls** — you never call them from C
directly. The `?` prefix marks them as slink-managed private symbols that
get uniquified per module.

Also note `nargs` — it's defined here as just `RTS`. That's the variadic
argument count hook used by `register` functions. The compiler emits
`JSR nargs` before calling a `register` function; the optimizer strips it
(`JSR nargs` → nothing). The label exists so the optimizer rule has
something to match.

---

## Serial I/O (`SERIO.ASM`)

The default `SERIO.ASM` targets a 6551 ACIA at `$0000`. Target-specific
versions (in `targets/*/lib09/`) replace this for real hardware.

| Function | Signature | Description |
|----------|-----------|-------------|
| `putstr` | `putstr(char *s)` | Write null-terminated string to console |
| `putch` | `putch(char c)` | Write char, translating `\n` → `\r\n` |
| `putchr` | `putchr(char c)` | Write char raw (no translation) |
| `getch` | `int getch()` | Read char, translating `\r` → `\n`, blocking |
| `getchr` | `int getchr()` | Read char raw, blocking |
| `chkchr` | `int chkchr()` | Non-blocking check: return char or -1 |
| `chkch` | `int chkch()` | Non-blocking check: return 0 or non-zero |
| `getstr` | `int getstr(char *buf, int len)` | Read line with backspace editing, returns char count |

`getstr` handles backspace (`\x08`) and DEL (`\x7F`) with proper console
echo — it overwrites with space then backs up. The returned count does not
include the null terminator.

---

## Formatted output (`PRINTF.ASM`, `SPRINTF.ASM`, `FORMAT.ASM`)

`printf` and `sprintf` share a common formatting engine in `_format_`.

| Function | Signature | Description |
|----------|-----------|-------------|
| `printf` | `register printf(char *fmt, ...)` | Format to console via `putstr` |
| `sprintf` | `register sprintf(char *buf, char *fmt, ...)` | Format to buffer |

Both are declared `register` — meaning the compiler passes the argument
count in D before the call. This is how they access a variable argument
list in Micro-C (see `stdarg.h`).

`_format_` is the internal engine. It walks the format string and a
pointer into the argument list simultaneously. You don't call it directly.

### Format specifiers

| Specifier | Meaning |
|-----------|---------|
| `%d` | Signed decimal integer |
| `%u` | Unsigned decimal integer |
| `%x` | Hexadecimal (uppercase A–F) |
| `%o` | Octal |
| `%b` | Binary |
| `%s` | Null-terminated string |
| `%c` | Single character |

Field width is supported: `%5d` right-justifies in a 5-char field, `%-5d`
left-justifies, `%05d` zero-fills. There is no `%f`, no `%e`, no `%g` —
no floating point support anywhere in the library.

**Important:** `printf` allocates a 100-byte buffer on the stack for the
formatted output. If your format string can produce more than 100 characters
the buffer overflows silently. Plan accordingly.

---

## String functions (`STRING1.ASM`, `STRING2.ASM`, `STRING3.ASM`, `CONCAT.ASM`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `strlen` | `int strlen(char *s)` | Length of string (not counting null terminator) |
| `strcpy` | `strcpy(char *dst, char *src)` | Copy string including null terminator |
| `strcat` | `strcat(char *dst, char *src)` | Append `src` to `dst` |
| `strcmp` | `int strcmp(char *s1, char *s2)` | Compare: returns -1, 0, or 1 |
| `strchr` | `char *strchr(char *s, char c)` | Find first occurrence of `c` in `s`, or NULL |
| `concat` | `register concat(char *dst, char *src1, ...)` | Concatenate multiple strings into `dst` |

`concat` is a Dunfield extension — a `register` variadic that concatenates
any number of source strings into `dst`. The destination is the **first**
argument. Useful for building strings from parts without needing temporary
buffers:

```c
concat(buf, "Hello, ", name, "!\n");
```

There is no `strncpy`, no `strncat`, no `strncmp`, no `strstr`, no `strtok`.
If you need those, write them or add them to the library.

---

## Character classification (`ISFUNS.ASM`, `CTYPE.ASM`, `ctype.h`)

Two implementations are provided. Choose one.

### Function versions (`ISFUNS.ASM`)

Pulled in on demand via `$EX:` references. Each is a separate call.

| Function | Returns non-zero if... |
|----------|------------------------|
| `isalpha(c)` | `c` is A–Z or a–z |
| `isdigit(c)` | `c` is 0–9 |
| `isalnum(c)` | `c` is A–Z, a–z, or 0–9 |
| `isupper(c)` | `c` is A–Z |
| `islower(c)` | `c` is a–z |
| `isspace(c)` | `c` is space, tab, or newline |
| `iscntrl(c)` | `c` is a control character (0–31 or 127) |
| `isprint(c)` | `c` is printable (32–126) |
| `isgraph(c)` | `c` is printable and non-space (33–126) |
| `ispunct(c)` | `c` is printable but not alphanumeric |
| `isxdigit(c)` | `c` is 0–9, A–F, or a–f |
| `isascii(c)` | `c` is in range 0–127 |

### Macro versions (`ctype.h`)

Defined as macros in `include/ctype.h` using a 129-byte lookup table
(`_chartype_` from `CTYPE.ASM`). Faster than the function versions but
require `#include <ctype.h>` and the **extended preprocessor** (`cc09 -P`)
because the macros use parameterised `#define`.

The table classifies every ASCII character (0–127) plus an EOF entry at
index 0. Characters outside that range produce undefined results.

---

## Case conversion (`TOFUNS.ASM`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `toupper` | `int toupper(char c)` | Convert a–z to A–Z, others unchanged |
| `tolower` | `int tolower(char c)` | Convert A–Z to a–z, others unchanged |

---

## Numeric conversion (`ATOI.ASM`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `atoi` | `int atoi(char *s)` | Convert decimal string to integer |

`atoi` skips leading spaces and tabs, handles a leading `-` sign, stops
at the first non-digit. There is no `atol`, no `atof`, no `strtol`.

---

## Math (`MATH.ASM`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `abs` | `int abs(int n)` | Absolute value |
| `max` | `int max(int a, int b)` | Larger of two signed integers |
| `min` | `int min(int a, int b)` | Smaller of two signed integers |
| `sqrt` | `int sqrt(int n)` | Integer square root (linear search, max input 255²) |

`sqrt` is a simple upward-counting loop — it finds the first integer whose
square ≥ the argument. For values over 255² (65025) it returns 256. It's
not fast but it's correct for embedded use where you only need an
approximate result and the value is small.

---

## Memory (`MEMORY.ASM`, `PEEK.ASM`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `memset` | `memset(char *dst, char val, int n)` | Fill `n` bytes at `dst` with `val` |
| `memcpy` | `memcpy(char *dst, char *src, int n)` | Copy `n` bytes from `src` to `dst` |
| `peek` | `int peek(int addr)` | Read byte from absolute address |
| `peekw` | `int peekw(int addr)` | Read word (16-bit) from absolute address |
| `poke` | `poke(int addr, int val)` | Write byte to absolute address |
| `pokew` | `pokew(int addr, int val)` | Write word to absolute address |

`peek`/`poke`/`peekw`/`pokew` take the address as an `int` (16-bit), which
covers the entire 6809 address space. These are the correct way to access
memory-mapped hardware registers from C.

---

## Heap allocation (`MALLOC.ASM`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `malloc` | `char *malloc(int size)` | Allocate `size` bytes, return pointer or NULL |
| `free` | `free(char *ptr)` | Release block, merge adjacent free blocks |

The heap starts at `?heap` (immediately after the program's code and
static data) and grows upward. `malloc` keeps a linked list of blocks,
each with a 3-byte header: one flag byte (0=end, 1=free, 2=allocated)
and a two-byte size.

`malloc` refuses to allocate if the remaining space between the heap top
and the stack pointer is less than 1000 bytes. This 1000-byte margin is
hardcoded as a stack guard.

`free` performs immediate coalescing — after marking a block free it scans
the list forward, merging any adjacent free blocks into one. The list is
also trimmed at the last allocated block so the end marker moves backward
as blocks are freed.

There is no `realloc`, no `calloc`.

---

## Random numbers (`RAND.ASM`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `rand` | `int rand(int limit)` | Return pseudo-random integer in range 0 to limit-1 |

Linear congruential generator: `seed = seed * 13709 + 13849`. The seed is
stored in `RANDSEED`, a 2-byte BSS variable. It is not initialised — the
initial seed is whatever happens to be at that memory address at startup,
which gives a different (if not truly random) sequence each run on hardware.

---

## Interrupts (`ENABLE.ASM`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `enable` | `enable()` | Enable IRQ (clear I bit in CC) |
| `disable` | `disable()` | Disable IRQ (set I bit in CC) |
| `enablef` | `enablef()` | Enable FIRQ (clear F bit in CC) |
| `disablef` | `disablef()` | Disable FIRQ (set F bit in CC) |

---

## Long jump (`LONGJMP.ASM`)

| Function | Signature | Description |
|----------|-----------|-------------|
| `setjmp` | `int setjmp(int env[2])` | Save return address and stack pointer, return 0 |
| `longjmp` | `longjmp(int env[2], int val)` | Restore saved state, return `val` at setjmp site |

`jmp_buf` on the 6809 is a 2-word (4-byte) array: `env[0]` holds the return
address, `env[1]` holds the saved stack pointer. Defined in `setjmp.h`.

`setjmp` returns 0 when called directly. `longjmp` never returns to its
caller — it unwinds the stack to the saved frame and makes `setjmp` return
`val`. If `val` is 0, `longjmp` substitutes 1 (to distinguish from the
initial `setjmp` call).

---

## Long (32-bit) arithmetic (`LONGMATH.ASM`)

The library provides 32-bit integer arithmetic as explicit function calls.
There is no `long` type in the compiler — these functions operate on 4-byte
arrays passed by pointer.

| Function | Signature | Description |
|----------|-----------|-------------|
| `longadd` | `longadd(char *n1, char *n2)` | n1 += n2 in-place |
| `longsub` | `longsub(char *n1, char *n2)` | n1 -= n2 in-place |
| `longmul` | `longmul(char *n1, char *n2)` | n1 *= n2 in-place |
| `longdiv` | `longdiv(char *n1, char *n2)` | n1 /= n2 in-place (result in n1, remainder in `Longreg`) |
| `longshl` | `longshl(char *n)` | n <<= 1 in-place |
| `longshr` | `longshr(char *n)` | n >>= 1 in-place |
| `longcmp` | `int longcmp(char *n1, char *n2)` | Compare: returns -1, 0, or 1 |
| `longtst` | `int longtst(char *n)` | Test for zero: returns 0 if zero, nonzero otherwise |
| `longcpy` | `longcpy(char *dst, char *src)` | Copy 32-bit value |
| `longset` | `longset(char *n, int val)` | Set from 16-bit value (zero-extends) |

The word size is controlled by `?LSIZE EQU 4` at the top of `LONGMATH.ASM`.
To work with 64-bit values, change this to 8. The algorithms (shift-and-add
multiply, restoring division) generalise to any byte count.

Numbers are stored **big-endian**: the most significant byte is at the lowest
address. `Longreg` is a scratch 4-byte BSS variable used as a working
register inside multiply and divide.

Usage example:
```c
char a[4], b[4];
longset(a, 1000);   /* a = 1000 */
longset(b, 999);    /* b = 999  */
longadd(a, b);      /* a = 1999 */
```

---

## Headers

| Header | Contents |
|--------|---------|
| `6809io.h` | `NULL`, `EOF`, `_CPU_ 6809`, `extern` declarations for `printf`, `sprintf`, `concat` |
| `stddef.h` | `NULL`, `ptrdiff_t` (`int`), `size_t` (`unsigned`) |
| `ctype.h` | Macro versions of character classification functions. **Requires `cc09 -P`** |
| `limits.h` | `CHAR_MAX`, `CHAR_MIN`, `INT_MAX`, `INT_MIN`, `UINT_MAX` etc. |
| `setjmp.h` | `jmp_buf` typedef (CPU-specific, 4 bytes on 6809). **Requires `cc09 -P`** |
| `stdarg.h` | `va_list`, `va_start`, `va_arg`, `va_end`, `va_func`. **Requires `cc09 -P`** |
| `6809int.h` | Interrupt vector definitions (hardware-specific) |

Headers marked **Requires `cc09 -P`** use parameterised macros (`#define
foo(x) ...`) and/or `#if` expressions that the built-in preprocessor in
`mcc09` cannot handle. You must use the full `mcp` preprocessor:
```sh
cc09 prog.c -Pq   # run mcp before mcc09
```

---

## What's not in the library

Things you might expect from a standard C library that aren't here:

- **File I/O** — no `fopen`, `fclose`, `fread`, `fwrite`, `fgets`, `fputs`.
  The 6809 targets are typically ROM-based single-task systems with no
  filesystem. If you need file-like I/O you'd implement it against whatever
  storage hardware your target has.
- **`stdio.h`** — there is no stdio. `printf` and `putstr` write directly
  to the serial console via `SERIO.ASM`.
- **`stdlib.h`** — partial. `malloc`/`free` exist, `rand` exists, `abs` exists.
  No `exit` (there's the linker symbol `exit` that jumps to the monitor,
  but it's not a normal callable function). No `atof`, `strtol`, `strtod`.
- **`string.h`** — partial. `strlen`, `strcpy`, `strcat`, `strcmp`, `strchr`
  exist. No `strncpy`, `strncat`, `strncmp`, `strstr`, `strtok`, `memcmp`,
  `memmove`.
- **Floating point** — nothing. Not in the type system, not in the library.
  Use fixed-point arithmetic.
- **`time.h`** — nothing. No RTC abstraction.
- **`assert.h`** — nothing. The `assert.h` file is present but empty.

---

## Adding your own library modules

Any `.ASM` file can be added to the library. The process:

1. Write the assembly source with public entry points as plain labels and
   local labels as `?N` (they'll be uniquified by slink).
2. Use `$EX:symbol` at the end to declare any external symbols your module
   calls from other library files.
3. Use `$DD:name N` to declare any BSS storage your module needs.
4. Run `sindex` to rebuild `EXTINDEX.LIB`:
   ```sh
   cd lib09
   sindex *.ASM
   ```

After that, any C code that calls your new function will have it pulled in
automatically via slink.

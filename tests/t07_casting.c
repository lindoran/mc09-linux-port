/*
 * t07_casting.c — type casting and promotion tests
 *
 * Covers: (char) narrowing inline (no storage bounce), (char) narrowing via
 *         storage for comparison, (unsigned) reinterpret, signed/unsigned
 *         arithmetic, sign extension, unsigned char struct members without
 *         the & 0xFF workaround, and multi-char constants.
 *
 * CAST FIXES VALIDATED HERE:
 *
 *   Fix 1 — (char) narrowing on in-register values:
 *     (int)(char)0x0141 must yield 65, not 321.
 *     (int)(char)0x00FF must yield -1, not 255.
 *     (int)(char)0x0080 must yield -128, not 128.
 *     The old workaround (bounce through char storage) is kept for
 *     comparison; both paths must agree.
 *
 *   Fix 2 — (int) cast of unsigned char struct member:
 *     (int)px.r where r is unsigned char and r==255 must yield 255, not -1.
 *     The UNSIGNED bit must survive the cast so expand() emits CLRA, not SEX.
 *     The old workaround ((int)px.r & 0xFF) is kept for comparison.
 *
 * SUPPORTED CASTS: (int), (unsigned), (char), (const), (register), (void).
 * NOT SUPPORTED: (unsigned char), (signed char) — use & 0xFF to mask.
 */
#include "testutil.h"

/* ------------------------------------------------------------------ */
/* Helpers for Fix 1 — inline narrowing without storage               */
/* ------------------------------------------------------------------ */

/*
 * Returns (int)(char)n directly — no intermediate char variable.
 * Before the fix this returned n unchanged (no truncation, no SEX).
 */
cast_char_direct(n)
    int n;
{
    return (int)(char)n;
}

/*
 * Old workaround: bounce through char storage.
 * Must give identical results to cast_char_direct() after the fix.
 */
char g_c;

cast_char_storage(n)
    int n;
{
    g_c = (char)n;
    return (int)g_c;
}

/* ------------------------------------------------------------------ */
/* Helpers for remaining cast types                                    */
/* ------------------------------------------------------------------ */

as_unsigned(n)
    int n;
{
    return (unsigned)n;
}

umax(a, b)
    unsigned a, b;
{
    return a > b ? a : b;
}

/* ------------------------------------------------------------------ */
/* Struct with unsigned char members — Fix 2 target                   */
/* ------------------------------------------------------------------ */

struct Pixel { unsigned char r; unsigned char g; unsigned char b; };
struct Pixel px;

unsigned wrap_u;
int      wrap_i;
char     cvar;
int      ivar;

main()
{
    unsigned u;

    putstr("t07_casting\n");

    /* ---- Fix 1: inline (char) narrowing — direct cast path -------- */

    /* Positive value: high byte stripped, stays positive */
    EXPECT_EQ("direct-pos",   cast_char_direct(0x0041),   65)
    /* High byte must be discarded */
    EXPECT_EQ("direct-wrap",  cast_char_direct(0x0141),   65)
    /* 0xFF sign-extends to -1 */
    EXPECT_EQ("direct-neg",   cast_char_direct(0x00FF),   -1)
    /* 0x80 sign-extends to -128 */
    EXPECT_EQ("direct-neg2",  cast_char_direct(0x0080), -128)
    /* Zero stays zero */
    EXPECT_EQ("direct-zero",  cast_char_direct(0x0000),    0)
    /* Large positive with high byte — must truncate to low signed byte */
    EXPECT_EQ("direct-big",   cast_char_direct(0x7FFF),   -1)

    /* ---- Fix 1: storage path — must agree with direct path -------- */

    EXPECT_EQ("store-pos",    cast_char_storage(0x0041),   65)
    EXPECT_EQ("store-wrap",   cast_char_storage(0x0141),   65)
    EXPECT_EQ("store-neg",    cast_char_storage(0x00FF),   -1)
    EXPECT_EQ("store-neg2",   cast_char_storage(0x0080), -128)
    EXPECT_EQ("store-zero",   cast_char_storage(0x0000),    0)

    /* ---- Fix 2: (int) unsigned char struct member — no mask -------- */
    /*
     * Before the fix, (int)px.r where px.r==255 returned -1 because
     * the UNSIGNED bit was stripped by get_type(INT,0) in the cast
     * target type, so expand() emitted SEX instead of CLRA.
     */
    px.r = 255; px.g = 128; px.b = 0;

    EXPECT_EQ("uchr-r-direct", (int)px.r, 255)   /* must be 255, not -1  */
    EXPECT_EQ("uchr-g-direct", (int)px.g, 128)   /* must be 128, not -128 */
    EXPECT_EQ("uchr-b-direct", (int)px.b,   0)

    /* Old workaround still works and must agree */
    EXPECT_EQ("uchr-r-mask",  (int)px.r & 0xFF, 255)
    EXPECT_EQ("uchr-g-mask",  (int)px.g & 0xFF, 128)

    /* ---- (unsigned) reinterpret ------------------------------------ */
    u = as_unsigned(-1);     EXPECT_UEQ("uint-neg1", u, 0xFFFF)
    u = as_unsigned(-32768); EXPECT_UEQ("uint-min",  u, 0x8000)
    u = as_unsigned(32767);  EXPECT_UEQ("uint-max",  u, 0x7FFF)

    /* ---- char variable sign extension into int (storage path) ------ */
    cvar = (char)0xFF; ivar = cvar; EXPECT_EQ("sext-ff",  ivar,   -1)
    cvar = (char)0x80; ivar = cvar; EXPECT_EQ("sext-80",  ivar, -128)
    cvar = (char)0x7F; ivar = cvar; EXPECT_EQ("sext-7f",  ivar,  127)
    cvar = (char)0x00; ivar = cvar; EXPECT_EQ("sext-00",  ivar,    0)

    /* ---- unsigned via & 0xFF (no cast) ----------------------------- */
    cvar = (char)0xFF;
    ivar = (int)cvar & 0xFF;  EXPECT_EQ("uchr-ff", ivar, 255)
    cvar = (char)0x80;
    ivar = (int)cvar & 0xFF;  EXPECT_EQ("uchr-80", ivar, 128)

    /* ---- unsigned arithmetic wrap ---------------------------------- */
    wrap_u = 0xFFFF; ++wrap_u;
    EXPECT_UEQ("uint-over",  wrap_u, 0)
    wrap_u = 0; --wrap_u;
    EXPECT_UEQ("uint-under", wrap_u, 0xFFFF)

    /* ---- signed overflow (two's complement) ------------------------ */
    wrap_i = 32767; ++wrap_i;
    EXPECT_EQ("int-over", wrap_i, -32768)

    /* ---- umax ------------------------------------------------------ */
    EXPECT_UEQ("umax-a",  umax(100, 200), 200)
    EXPECT_UEQ("umax-b",  umax(200, 100), 200)
    EXPECT_UEQ("umax-eq", umax(50, 50),   50)

    /* ---- multi-char constant --------------------------------------- */
    EXPECT_EQ("mchar-AB", (int)'AB', 0x4142)
    EXPECT_EQ("mchar-lo", cast_char_direct('AB'), 66)

    SUITE_DONE()
}

/*
 * t07_casting.c — type casting and promotion tests
 *
 * Covers: (char) narrowing via char storage, (unsigned) reinterpret,
 *         signed/unsigned arithmetic, sign extension, multi-char constants.
 *
 * KEY FINDING — (char) cast is a compile-time NO-OP:
 *   (int)(char)some_int  — does NOT truncate or sign-extend.
 *   Sign extension only happens when loading from a char-typed STORAGE
 *   location (the LDB instruction followed by expand() -> SEX).
 *   To narrow an int to signed 8-bit: assign to a char variable first,
 *   then read back into int. e.g.:  char c; c = n; i = c;
 *
 * SUPPORTED CASTS: (int), (unsigned), (char), (const), (register), (void).
 * NOT SUPPORTED: (unsigned char), (signed char) — use & 0xFF to mask.
 */
#include "testutil.h"

/* Narrow n to signed 8-bit via char storage (the only reliable way) */
char g_c;   /* global char used as narrowing scratch */

low_byte(n)
    int n;
{
    g_c = (char)n;    /* stores low 8 bits as byte */
    return (int)g_c;  /* loads with LDB+SEX -> sign-extended int */
}

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

unsigned wrap_u;
int      wrap_i;
char     cvar;
int      ivar;

main()
{
    unsigned u;

    putstr("t07_casting\n");

    /* low_byte: narrow via char storage -> sign-extends */
    EXPECT_EQ("low-pos",  low_byte(0x0041),  65)   /* 0x41 = 'A' */
    EXPECT_EQ("low-wrap", low_byte(0x0141),  65)   /* high byte dropped */
    EXPECT_EQ("low-neg",  low_byte(0x00FF),  -1)   /* 0xFF sign-extends */
    EXPECT_EQ("low-neg2", low_byte(0x0080), -128)  /* 0x80 sign-extends */
    EXPECT_EQ("low-zero", low_byte(0x0000),   0)

    /* (unsigned) reinterpret */
    u = as_unsigned(-1);     EXPECT_UEQ("uint-neg1", u, 0xFFFF)
    u = as_unsigned(-32768); EXPECT_UEQ("uint-min",  u, 0x8000)
    u = as_unsigned(32767);  EXPECT_UEQ("uint-max",  u, 0x7FFF)

    /* char variable sign extension into int */
    cvar = (char)0xFF; ivar = cvar; EXPECT_EQ("sext-ff",  ivar, -1)
    cvar = (char)0x80; ivar = cvar; EXPECT_EQ("sext-80",  ivar, -128)
    cvar = (char)0x7F; ivar = cvar; EXPECT_EQ("sext-7f",  ivar, 127)
    cvar = (char)0x00; ivar = cvar; EXPECT_EQ("sext-00",  ivar, 0)

    /* unsigned char (no sign ext): use & 0xFF after loading */
    cvar = (char)0xFF;
    ivar = (int)cvar & 0xFF;  EXPECT_EQ("uchr-ff", ivar, 255)
    cvar = (char)0x80;
    ivar = (int)cvar & 0xFF;  EXPECT_EQ("uchr-80", ivar, 128)

    /* unsigned arithmetic wrap */
    wrap_u = 0xFFFF; ++wrap_u;
    EXPECT_UEQ("uint-over",  wrap_u, 0)
    wrap_u = 0; --wrap_u;
    EXPECT_UEQ("uint-under", wrap_u, 0xFFFF)

    /* signed overflow (two's complement) */
    wrap_i = 32767; ++wrap_i;
    EXPECT_EQ("int-over", wrap_i, -32768)

    /* umax */
    EXPECT_UEQ("umax-a",  umax(100, 200), 200)
    EXPECT_UEQ("umax-b",  umax(200, 100), 200)
    EXPECT_UEQ("umax-eq", umax(50, 50),   50)

    /* multi-char constant: 'AB' = 0x4142 */
    EXPECT_EQ("mchar-AB", (int)'AB', 0x4142)
    /* low_byte extracts 'B' = 0x42 = 66 */
    EXPECT_EQ("mchar-lo", low_byte('AB'), 66)

    SUITE_DONE()
}

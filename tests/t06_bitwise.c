/*
 * t06_bitwise.c — bitwise operator tests
 *
 * MACRO NOTE: EXPECT_UEQ uses unsigned subtraction. When both operands
 * are complex expressions (e.g. ~(unsigned)x), evaluate into variables
 * first to ensure the subtraction is performed on the final values, not
 * on re-evaluated sub-expressions.
 */
#include "testutil.h"

bitfield(val, lo, hi)
    unsigned val; int lo, hi;
{
    unsigned mask;
    int width;
    width = hi - lo + 1;
    mask = (unsigned)((1 << width) - 1);
    return (val >> lo) & mask;
}

popcount(n)
    unsigned n;
{
    int c;
    c = 0;
    while(n) { c += (int)(n & 1); n >>= 1; }
    return c;
}

unsigned bit_u;

set_bit(b) int b; { bit_u |=  (unsigned)(1 << b); }
clr_bit(b) int b; { bit_u &= ~(unsigned)(1 << b); }
tog_bit(b) int b; { bit_u ^=  (unsigned)(1 << b); }
test_bit(v, b) unsigned v; int b; { return (int)((v >> b) & 1); }

main()
{
    unsigned u, got_u, exp_u;
    int r;

    putstr("t06_bitwise\n");

    EXPECT_EQ("and-basic",  (int)(0xFF   & 0x0F),   0x0F)
    EXPECT_EQ("and-zero",   (int)(0xAA   & 0x55),   0x00)
    EXPECT_EQ("and-ident",  (int)(0x1234 & 0xFFFF), 0x1234)

    EXPECT_EQ("or-basic",   (int)(0xF0 | 0x0F), 0xFF)
    EXPECT_EQ("or-overlap", (int)(0x0A | 0x05), 0x0F)
    EXPECT_EQ("or-same",    (int)(0xAA | 0xAA), 0xAA)

    EXPECT_EQ("xor-diff",   (int)(0xFF ^ 0x0F), 0xF0)
    EXPECT_EQ("xor-same",   (int)(0xAB ^ 0xAB), 0x00)
    EXPECT_EQ("xor-toggle", (int)(0x00 ^ 0xFF), 0xFF)

    r = (int)(~(int)0x00 & 0xFF); EXPECT_EQ("not-zero", r, 0xFF)
    r = (int)(~(int)0xFF & 0xFF); EXPECT_EQ("not-ff",   r, 0x00)
    r = (int)(~(int)0x0F & 0xFF); EXPECT_EQ("not-0f",   r, 0xF0)

    /* 16-bit NOT: store result in variable before EXPECT_UEQ */
    got_u = ~(unsigned)0x5A5A;
    exp_u = (unsigned)0xA5A5;
    EXPECT_UEQ("not-word", got_u, exp_u)

    EXPECT_EQ("shl-1",   (int)((unsigned)1 << 1),  2)
    EXPECT_EQ("shl-4",   (int)((unsigned)1 << 4), 16)
    EXPECT_EQ("shl-8",   (int)((unsigned)1 << 8), 256)
    EXPECT_EQ("shl-mul", (int)((unsigned)3 << 3), 24)

    EXPECT_UEQ("shr-1",   (unsigned)256   >> 1,  128)
    EXPECT_UEQ("shr-4",   (unsigned)256   >> 4,  16)
    EXPECT_UEQ("shr-msb", (unsigned)0x8000 >> 1, 0x4000)

    u = 0xFF;  u &= 0x0F; EXPECT_EQ("and-eq", (int)u, 0x0F)
    u = 0x00;  u |= 0xA5; EXPECT_EQ("or-eq",  (int)u, 0xA5)
    u = 0xFF;  u ^= 0x0F; EXPECT_EQ("xor-eq", (int)u, 0xF0)
    u = 1;     u <<= 3;   EXPECT_EQ("shl-eq", (int)u, 8)
    u = 256;   u >>= 4;   EXPECT_EQ("shr-eq", (int)u, 16)

    bit_u = 0;
    set_bit(0); EXPECT_EQ("set-b0",  (int)bit_u, 1)
    set_bit(7); EXPECT_EQ("set-b7",  (int)bit_u, 0x81)
    clr_bit(0); EXPECT_EQ("clr-b0",  (int)bit_u, 0x80)
    tog_bit(3); EXPECT_EQ("tog-on",  test_bit(bit_u, 3), 1)
    tog_bit(3); EXPECT_EQ("tog-off", test_bit(bit_u, 3), 0)

    EXPECT_EQ("pop-0",    popcount(0),      0)
    EXPECT_EQ("pop-1",    popcount(1),      1)
    EXPECT_EQ("pop-ff",   popcount(0xFF),   8)
    EXPECT_EQ("pop-5555", popcount(0x5555), 8)
    EXPECT_EQ("pop-ffff", popcount(0xFFFF), 16)

    r = bitfield(0xABCD, 4, 7); EXPECT_EQ("bitfld-c", r, 0xC)
    r = bitfield(0x1234, 0, 3); EXPECT_EQ("bitfld-4", r, 4)

    SUITE_DONE()
}

/*
 * t14_stdlib.c — math, memory, string utility library tests
 *
 * Covers: abs, max, min, sqrt (MATH.ASM),
 *         memset, memcpy (MEMORY.ASM),
 *         atoi (ATOI.ASM),
 *         toupper, tolower (TOFUNS.ASM).
 *
 * Character classification (isXXX) is in t14b_ctype.c — it pulls in
 * ISFUNS.ASM which is large enough to overflow ROM space if combined here.
 */
#include "testutil.h"

extern abs(), max(), min(), sqrt();
extern memset(), memcpy();
extern atoi();
extern toupper(), tolower();

char src_buf[16];
char dst_buf[16];

main()
{
    putstr("t14_stdlib\n");

    EXPECT_EQ("abs-pos",  abs(10),   10)
    EXPECT_EQ("abs-neg",  abs(-10),  10)
    EXPECT_EQ("abs-zero", abs(0),    0)

    EXPECT_EQ("max-ab", max(3, 7),  7)
    EXPECT_EQ("max-ba", max(9, 2),  9)
    EXPECT_EQ("max-eq", max(5, 5),  5)
    EXPECT_EQ("min-ab", min(3, 7),  3)
    EXPECT_EQ("min-ba", min(9, 2),  2)
    EXPECT_EQ("min-eq", min(5, 5),  5)

    /* sqrt: integer square root — MATH.ASM returns ceiling, not floor */
    EXPECT_EQ("sqrt-0",   sqrt(0),   0)
    EXPECT_EQ("sqrt-1",   sqrt(1),   1)
    EXPECT_EQ("sqrt-4",   sqrt(4),   2)
    EXPECT_EQ("sqrt-9",   sqrt(9),   3)
    EXPECT_EQ("sqrt-16",  sqrt(16),  4)
    EXPECT_EQ("sqrt-25",  sqrt(25),  5)
    EXPECT_EQ("sqrt-100", sqrt(100), 10)
    /* non-perfect: sqrt rounds UP (ceiling), not floor */
    EXPECT_EQ("sqrt-2",   sqrt(2),   2)
    EXPECT_EQ("sqrt-15",  sqrt(15),  4)

    memset(dst_buf, 0, 16);
    EXPECT_EQ("mset-0",  (int)dst_buf[0],  0)
    EXPECT_EQ("mset-15", (int)dst_buf[15], 0)
    memset(dst_buf, 'X', 8);
    EXPECT_EQ("mset-x0", (int)dst_buf[0], 'X')
    EXPECT_EQ("mset-x7", (int)dst_buf[7], 'X')
    EXPECT_EQ("mset-x8", (int)dst_buf[8], 0)

    src_buf[0]='H'; src_buf[1]='i'; src_buf[2]='!'; src_buf[3]=0;
    memcpy(dst_buf, src_buf, 4);
    EXPECT_EQ("mcpy-0", (int)dst_buf[0], 'H')
    EXPECT_EQ("mcpy-1", (int)dst_buf[1], 'i')
    EXPECT_EQ("mcpy-2", (int)dst_buf[2], '!')
    EXPECT_EQ("mcpy-3", (int)dst_buf[3], 0)

    EXPECT_EQ("atoi-0",    atoi("0"),     0)
    EXPECT_EQ("atoi-42",   atoi("42"),   42)
    EXPECT_EQ("atoi-neg",  atoi("-7"),   -7)
    EXPECT_EQ("atoi-1000", atoi("1000"), 1000)

    EXPECT_EQ("toupper-a", toupper('a'), 'A')
    EXPECT_EQ("toupper-z", toupper('z'), 'Z')
    EXPECT_EQ("toupper-A", toupper('A'), 'A')
    EXPECT_EQ("toupper-1", toupper('1'), '1')

    EXPECT_EQ("tolower-A", tolower('A'), 'a')
    EXPECT_EQ("tolower-Z", tolower('Z'), 'z')
    EXPECT_EQ("tolower-a", tolower('a'), 'a')
    EXPECT_EQ("tolower-1", tolower('1'), '1')

    SUITE_DONE()
}

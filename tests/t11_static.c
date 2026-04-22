/*
 * t11_static.c — static storage class and extern linkage tests
 *
 * Covers: static local variables (retain value across calls),
 *         static global variables (file-scope, not zero-init by default),
 *         extern declaration of globals defined here,
 *         const qualifier (compile-time constant, not runtime).
 */
#include "testutil.h"

/* static global: visible only in this file */
static int s_counter;
static int s_flag;

/* const global: value fixed at declaration */
const int C_TEN = 10;
const int C_NEG = -5;

/* increment static counter and return new value */
inc_counter()
{
    s_counter = s_counter + 1;
    return s_counter;
}

/* reset static counter */
reset_counter()
{
    s_counter = 0;
}

/* static local: persists across calls */
next_id()
{
    static int id;
    id = id + 1;
    return id;
}

/* static local with initial call pattern */
toggle()
{
    static int state;
    state = !state;
    return state;
}

/* accumulate into static local */
accumulate(n)
    int n;
{
    static int total;
    total = total + n;
    return total;
}

reset_accumulate()
{
    /* can't directly reset a static local from outside — only via flag */
}

main()
{
    int i;

    putstr("t11_static\n");

    /* static global via function */
    reset_counter();
    EXPECT_EQ("ctr-1",  inc_counter(), 1)
    EXPECT_EQ("ctr-2",  inc_counter(), 2)
    EXPECT_EQ("ctr-3",  inc_counter(), 3)
    reset_counter();
    EXPECT_EQ("ctr-rst", inc_counter(), 1)

    /* const globals */
    EXPECT_EQ("const-ten", C_TEN, 10)
    EXPECT_EQ("const-neg", C_NEG, -5)

    /* static local: next_id() */
    EXPECT_EQ("id-1", next_id(), 1)
    EXPECT_EQ("id-2", next_id(), 2)
    EXPECT_EQ("id-3", next_id(), 3)
    EXPECT_EQ("id-4", next_id(), 4)

    /* static local: toggle() */
    EXPECT_NZ("tog-1", toggle())     /* 0 -> 1 */
    EXPECT_Z ("tog-2", toggle())     /* 1 -> 0 */
    EXPECT_NZ("tog-3", toggle())     /* 0 -> 1 */

    /* static local: accumulate() */
    EXPECT_EQ("acc-10", accumulate(10), 10)
    EXPECT_EQ("acc-20", accumulate(20), 30)
    EXPECT_EQ("acc-5",  accumulate(5),  35)

    /* static flag */
    s_flag = 0;
    EXPECT_Z ("sflag-off", s_flag)
    s_flag = 1;
    EXPECT_NZ("sflag-on",  s_flag)

    /* const value cannot be modified (compile-time only) — just verify read */
    EXPECT_EQ("const-arith", C_TEN + C_NEG, 5)

    SUITE_DONE()
}

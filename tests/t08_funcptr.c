/*
 * t08_funcptr.c — indirect call / dispatch tests
 *
 * FINDING: Storing a FUNCGOTO-typed symbol in an int/unsigned variable
 * causes "Type clash". The compiler's type system distinguishes function
 * addresses from data pointers and forbids the assignment.
 *
 * SUPPORTED IDIOMS for function dispatch:
 *
 * 1. Switch dispatch (portable, idiomatic Micro-C):
 *      dispatch(op, arg) { switch(op) { case 0: return fn0(arg); ... } }
 *
 * 2. Inline asm to load function address and JSR indirectly:
 *      asm { LDD #fn_name; STD g_fp }   -- store address
 *      asm { LDX g_fp; JSR ,X }         -- indirect call
 *
 * 3. Variable call syntax  var()  where var is declared as extern function:
 *      extern myfunc();  myfunc = other_func;  myfunc();
 *      (Only works for FUNCGOTO-typed names, not plain int.)
 *
 * This test covers idioms 1 and 2.
 */
#include "testutil.h"

int fn_neg(n) int n; { return -n; }
int fn_sq(n)  int n; { return n * n; }
int fn_id(n)  int n; { return n; }
int fn_add(a, b) int a, b; { return a + b; }
int fn_sub(a, b) int a, b; { return a - b; }
int fn_mul(a, b) int a, b; { return a * b; }

/* --- Idiom 1: switch dispatch --- */

dispatch_u(op, n)
    int op, n;
{
    switch(op) {
        case 0: return fn_neg(n);
        case 1: return fn_sq(n);
        case 2: return fn_id(n);
        default: return 0;
    }
}

dispatch_b(op, a, b)
    int op, a, b;
{
    switch(op) {
        case 0: return fn_add(a, b);
        case 1: return fn_sub(a, b);
        case 2: return fn_mul(a, b);
        default: return 0;
    }
}

/* --- Idiom 2: asm indirect call --- */

int g_fp;    /* holds function address loaded by asm */
int g_res;   /* result written back by asm */

/* set_fp_*: load function address into g_fp via inline asm */
set_fp_neg() { asm {
    LDD #fn_neg
    STD g_fp
} }
set_fp_sq()  { asm {
    LDD #fn_sq
    STD g_fp
} }
set_fp_id()  { asm {
    LDD #fn_id
    STD g_fp
} }

/* call_fp(n): invoke g_fp with one argument n */
call_fp(n)
    int n;
{
    asm {
        LDD  2,S
        PSHS A,B
        LDX  g_fp
        JSR  ,X
        LEAS 2,S
        STD  g_res
    }
    return g_res;
}

int  src[4];
int  dst[4];

map_sw(op, len)
    int op, len;
{
    int i;
    for(i = 0; i < len; ++i)
        dst[i] = dispatch_u(op, src[i]);
}

main()
{
    putstr("t08_funcptr\n");

    /* --- switch dispatch --- */
    EXPECT_EQ("sw-neg-5",  dispatch_u(0, 5),  -5)
    EXPECT_EQ("sw-neg-0",  dispatch_u(0, 0),   0)
    EXPECT_EQ("sw-sq-4",   dispatch_u(1, 4),  16)
    EXPECT_EQ("sw-id-7",   dispatch_u(2, 7),   7)
    EXPECT_EQ("sw-def",    dispatch_u(9, 1),   0)

    EXPECT_EQ("sw-add",    dispatch_b(0, 3, 4),  7)
    EXPECT_EQ("sw-sub",    dispatch_b(1, 10, 3), 7)
    EXPECT_EQ("sw-mul",    dispatch_b(2, 6, 7), 42)

    /* dispatch in loop (functional map) */
    src[0]=1; src[1]=2; src[2]=3; src[3]=4;
    map_sw(1, 4);
    EXPECT_EQ("map-sq-0", dst[0],  1)
    EXPECT_EQ("map-sq-1", dst[1],  4)
    EXPECT_EQ("map-sq-2", dst[2],  9)
    EXPECT_EQ("map-sq-3", dst[3], 16)

    /* --- asm indirect call --- */
    set_fp_neg(); EXPECT_EQ("asm-neg-7",  call_fp(7), -7)
    set_fp_neg(); EXPECT_EQ("asm-neg-0",  call_fp(0),  0)
    set_fp_sq();  EXPECT_EQ("asm-sq-5",   call_fp(5), 25)
    set_fp_id();  EXPECT_EQ("asm-id-42",  call_fp(42), 42)

    /* verify g_fp changes between calls */
    set_fp_neg();
    set_fp_sq();
    EXPECT_EQ("asm-repoint", call_fp(4), 16)

    SUITE_DONE()
}

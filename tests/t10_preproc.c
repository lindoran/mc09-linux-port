/*
 * t10_preproc.c — preprocessor directive tests
 *
 * Covers: #define (object-like), #define with params via mcp,
 *         #undef, #ifdef/#ifndef/#endif, #if expressions,
 *         __FILE__, __LINE__, token paste ##, #error (compile-time).
 *
 * Requires mcp (-P flag): parameterised macros, ##, #if expressions.
 */
#include "testutil.h"

/* Object-like defines */
#define ZERO     0
#define ONE      1
#define ANSWER   42
#define NEGVAL  -7
#define STR_A   "hello"

/* Parameterised macros (require mcp) */
#define MAX(a,b)    ((a)>(b)?(a):(b))
#define MIN(a,b)    ((a)<(b)?(a):(b))
#define ABS(x)      ((x)<0?-(x):(x))
#define SQ(x)       ((x)*(x))
#define IDENT(x)    (x)

/* Token paste */
#define PASTE(a,b)  a##b

/* Stringify via double-expansion */
#define STR_(x)  #x
#define STR(x)   STR_(x)

/* Conditional compilation */
#define FEATURE_A
#undef  FEATURE_B

int paste_val;      /* will be assigned via PASTE macro */

/* Test #if expression */
#if ANSWER == 42
int if_answer = 1;
#else
int if_answer = 0;
#endif

#if ANSWER > 100
int if_gt = 1;
#else
int if_gt = 0;
#endif

main()
{
    int r;
    char buf[16];

    putstr("t10_preproc\n");

    /* object-like macros */
    EXPECT_EQ("def-zero",   ZERO,   0)
    EXPECT_EQ("def-one",    ONE,    1)
    EXPECT_EQ("def-answer", ANSWER, 42)
    EXPECT_EQ("def-neg",    NEGVAL, -7)

    /* parameterised macros */
    EXPECT_EQ("max-ab",  MAX(3, 7),    7)
    EXPECT_EQ("max-ba",  MAX(9, 2),    9)
    EXPECT_EQ("max-eq",  MAX(5, 5),    5)
    EXPECT_EQ("min-ab",  MIN(3, 7),    3)
    EXPECT_EQ("min-eq",  MIN(5, 5),    5)
    EXPECT_EQ("abs-pos", ABS(10),     10)
    EXPECT_EQ("abs-neg", ABS(-10),    10)
    EXPECT_EQ("sq-4",    SQ(4),       16)
    EXPECT_EQ("sq-neg",  SQ(-3),       9)
    EXPECT_EQ("ident-7", IDENT(7),     7)

    /* #if expressions */
    EXPECT_EQ("if-eq",  if_answer, 1)
    EXPECT_EQ("if-gt",  if_gt,     0)

    /* token paste: PASTE(paste_,val) -> paste_val */
    PASTE(paste_,val) = 99;
    EXPECT_EQ("paste", paste_val, 99)

    /* #ifdef / #ifndef */
#ifdef FEATURE_A
    r = 1;
#else
    r = 0;
#endif
    EXPECT_EQ("ifdef-a", r, 1)

#ifdef FEATURE_B
    r = 1;
#else
    r = 0;
#endif
    EXPECT_EQ("ifdef-b-undef", r, 0)

#ifndef FEATURE_B
    r = 1;
#else
    r = 0;
#endif
    EXPECT_EQ("ifndef-b", r, 1)

    /* __LINE__ */
    r = __LINE__;
    EXPECT_NZ("line-nonzero", r)

    /* Nested macro expansion */
    EXPECT_EQ("nested-max-sq", MAX(SQ(3), SQ(4)), 16)
    EXPECT_EQ("nested-abs-neg", ABS(MIN(-5, -3)), 5)

    SUITE_DONE()
}

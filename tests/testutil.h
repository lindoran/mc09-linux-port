/*
 * testutil.h — Micro-C 6809 test harness for usim09 target
 *
 * Compile each test file with the full preprocessor (-P):
 *   cd tests/
 *   MCDIR=.. MCINCLUDE=../include MCLIBDIR=../targets/usim09/lib09 \
 *     ../cc09 tNN_name.c -PIq
 *
 * COMPILER QUIRK NOTES:
 *   1. All local variable declarations must appear at the TOP of a
 *      function body, before any statements (K&R requirement).
 *   2. No block-scoped declarations inside inner { } blocks.
 *   3. The inline == 0 optimisation generates an inverted branch.
 *      Avoid `if(x == 0)`. Prefer `if(x != 0)` or `if(!x)`.
 *   4. Global variables are in RAM ($0100) via 6809RLM.ASM ORG fix.
 */

#ifndef _TESTUTIL_H
#define _TESTUTIL_H

#include <6809io.h>

extern putchr(), putstr(), strcmp();

int _pass;
int _fail;

/* Recursive unsigned integer printer — avoids char buffer and == 0 branch bug */
tut_uputs(u)
    unsigned u;
{
    if(u >= 10) tut_uputs(u / 10);
    putchr((char)('0' + (int)(u % 10)));
}

tut_putint(n)
    int n;
{
    unsigned u;
    if(n == -32768) { putstr("-32768"); return; }
    if(n < 0) { putchr('-'); n = -n; }
    u = (unsigned)n;
    if(u != 0) { tut_uputs(u); return; }
    putchr('0');
}

tut_putuint(n)
    unsigned n;
{
    if(n != 0) { tut_uputs(n); return; }
    putchr('0');
}

tut_puthex(n)
    unsigned n;
{
    int d;
    if(n >= 16) tut_puthex(n >> 4);
    d = (int)(n & 0xf);
    if(d < 10) putchr((char)('0' + d));
    else putchr((char)('A' + d - 10));
}

/* Use subtraction to compare: avoids the inline ==0 branch-inversion quirk */
#define EXPECT_EQ(name, got, exp) \
    if((int)(got)-(int)(exp)){putstr(name ":FAIL(got=");tut_putint((int)(got));putstr(",exp=");tut_putint((int)(exp));putstr(")\n");++_fail;} \
    else{putstr(name ":OK\n");++_pass;}

#define EXPECT_UEQ(name, got, exp) \
    if((unsigned)(got)-(unsigned)(exp)){putstr(name ":FAIL(got=");tut_putuint((unsigned)(got));putstr(",exp=");tut_putuint((unsigned)(exp));putstr(")\n");++_fail;} \
    else{putstr(name ":OK\n");++_pass;}

#define EXPECT_NZ(name, got) \
    if(got){putstr(name ":OK\n");++_pass;} \
    else{putstr(name ":FAIL(zero)\n");++_fail;}

#define EXPECT_Z(name, got) \
    if(!(got)){putstr(name ":OK\n");++_pass;} \
    else{putstr(name ":FAIL(nonzero)\n");++_fail;}

#define EXPECT_STR(name, got, exp) \
    if(!strcmp((got),(exp))){putstr(name ":OK\n");++_pass;} \
    else{putstr(name ":FAIL(");putstr(got);putstr("!=");putstr(exp);putstr(")\n");++_fail;}

#define SUITE_DONE() \
    if(_fail!=0){putstr("SUITE:FAIL(");tut_putint(_fail);putstr("/");tut_putint(_pass+_fail);putstr(")\n");return;} \
    putstr("SUITE:PASS(");tut_putint(_pass);putstr(")\n");

#endif /* _TESTUTIL_H */

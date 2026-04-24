/*
 * t15_ptrscale.c — pointer scaling diagnostic for *(p+n) / *(p-n)
 *
 * ROOT CAUSE:
 *   The binary '+' and '-' operators in the expression evaluator (compile.c)
 *   do not scale the integer offset by sizeof(*p) when one operand is a
 *   non-char pointer.  The subscript operator p[n] and the increment/
 *   decrement operators ++p, --p scale correctly.  Only the explicit
 *   *(p+n) / *(p-n) form is broken.
 *
 *   On the 6809 the compiler emits:
 *     ADDD #n          <-- adds n BYTES
 *   It should emit:
 *     ADDD #(n * sizeof(*p))   i.e. ADDD #(n*2) for int*
 *
 * ARRAY LAYOUT (arr[4], sizeof(int)=2, big-endian 6809):
 *   Byte offset: +0    +1    +2    +3    +4    +5    +6    +7
 *   Raw bytes:   0x01  0x00  0x02  0x00  0x04  0x00  0x08  0x00
 *   As ints:     [  256    ] [  512    ] [  1024   ] [  2048   ]
 *
 * CURRENT (WRONG) BEHAVIOUR for int* *(p+n):
 *   *(p+0) =  256  -- trivially correct (offset 0 never needs scaling)
 *   *(p+1) =    2  -- reads bytes[+1..+2] = 0x0002 (expected 512)
 *   *(p+2) =  512  -- reads bytes[+2..+3] = 0x0200 (reads arr[1], expected 1024)
 *   *(p+3) =    4  -- reads bytes[+3..+4] = 0x0004 (expected 2048)
 *   sum    =  774  -- 256+2+512+4           (expected 3840)
 *
 * CURRENT (WRONG) BEHAVIOUR for int* *(p-n) (p = &arr[3] via ++p x3):
 *   *(p-0) = 2048  -- trivially correct
 *   *(p-1) =    8  -- reads bytes[-1..-2] from arr[3] (expected 1024)
 *   *(p-2) = 1024  -- reads arr[2] instead of arr[1]   (expected 512)
 *   *(p-3) =    4  -- reads bytes[-3..-4] from arr[3]  (expected 256)
 *
 * WHAT THIS TEST COVERS:
 *   Section A -- int*      *(p+n) constant offset  (FAILS before fix)
 *   Section B -- int*      p[n]   subscript        (PASSES -- contrast)
 *   Section C -- char*     *(cp+n) offset          (PASSES -- sizeof 1)
 *   Section D -- int*      *(p+vn) variable offset (FAILS before fix)
 *   Section E -- int*      *(p-n) subtraction      (FAILS before fix)
 *   Section F -- unsigned* *(up+n)                 (FAILS before fix)
 */
#include "testutil.h"

int      arr[4];
char     carr[4];
unsigned uarr[4];
int      varsum;
int      vn;

main()
{
    int      *p;
    char     *cp;
    unsigned *up;

    putstr("t15_ptrscale\n");

    arr[0]  =  256; arr[1]  =  512; arr[2]  = 1024; arr[3]  = 2048;
    carr[0] =   10; carr[1] =   20; carr[2] =   30; carr[3] =   40;
    uarr[0] =  256; uarr[1] =  512; uarr[2] = 1024; uarr[3] = 2048;

    /* ---------------------------------------------------------------- */
    /* Section A: int* *(p+n) constant offset                           */
    /* All except ipc-0 FAIL on the unpatched compiler.                 */
    /* Wrong values shown in comments.                                  */
    /* ---------------------------------------------------------------- */
    p = arr;
    EXPECT_EQ("ipc-0", *(p+0),  256)   /* correct -- offset 0 trivial  */
    EXPECT_EQ("ipc-1", *(p+1),  512)   /* wrong=2    (bytes +1..+2)    */
    EXPECT_EQ("ipc-2", *(p+2), 1024)   /* wrong=512  (reads arr[1]!)   */
    EXPECT_EQ("ipc-3", *(p+3), 2048)   /* wrong=4    (bytes +3..+4)    */

    /* ---------------------------------------------------------------- */
    /* Section B: int* p[n] subscript -- PASSES, shown as contrast      */
    /* ---------------------------------------------------------------- */
    EXPECT_EQ("isub-1", p[1],  512)
    EXPECT_EQ("isub-2", p[2], 1024)
    EXPECT_EQ("isub-3", p[3], 2048)

    /* ---------------------------------------------------------------- */
    /* Section C: char* *(cp+n) -- PASSES (sizeof char == 1, no scale)  */
    /* ---------------------------------------------------------------- */
    cp = carr;
    EXPECT_EQ("cpc-1", (int)*(cp+1), 20)
    EXPECT_EQ("cpc-2", (int)*(cp+2), 30)
    EXPECT_EQ("cpc-3", (int)*(cp+3), 40)

    /* ---------------------------------------------------------------- */
    /* Section D: int* *(p+vn) variable offset                          */
    /* Correct sum: 256+512+1024+2048 = 3840                           */
    /* Wrong sum:   256+  2+ 512+   4 =  774                           */
    /* ---------------------------------------------------------------- */
    p = arr;
    varsum = *(p+0);
    vn = 1; varsum = varsum + *(p+vn);
    vn = 2; varsum = varsum + *(p+vn);
    vn = 3; varsum = varsum + *(p+vn);
    EXPECT_EQ("ipv-sum", varsum, 3840)  /* wrong=774 */

    /* ---------------------------------------------------------------- */
    /* Section E: int* *(p-n) subtraction                               */
    /* Use ++p three times to land correctly on arr[3] without          */
    /* triggering the broken +(n) path in the setup.                    */
    /* ---------------------------------------------------------------- */
    p = arr; ++p; ++p; ++p;            /* p now correctly = &arr[3]    */
    EXPECT_EQ("ipm-0", *(p-0), 2048)   /* correct -- offset 0 trivial  */
    EXPECT_EQ("ipm-1", *(p-1), 1024)   /* wrong=8    (bytes -1 from arr[3]) */
    EXPECT_EQ("ipm-2", *(p-2),  512)   /* wrong=1024 (reads arr[2]!)   */
    EXPECT_EQ("ipm-3", *(p-3),  256)   /* wrong=4    (bytes -3 from arr[3]) */

    /* ---------------------------------------------------------------- */
    /* Section F: unsigned* *(up+n) -- same root cause, unsigned type   */
    /* ---------------------------------------------------------------- */
    up = uarr;
    EXPECT_UEQ("upc-1", *(up+1),  512)  /* wrong=2   */
    EXPECT_UEQ("upc-2", *(up+2), 1024)  /* wrong=512 */
    EXPECT_UEQ("upc-3", *(up+3), 2048)  /* wrong=4   */

    SUITE_DONE()
}

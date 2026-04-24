/*
 * t03_pointers.c — pointer and array tests
 *
 * POINTER ARITHMETIC:
 *   p[n]   — subscript: correctly scales by sizeof(*p)
 *   p++    — post-increment: correctly scales by sizeof(*p)
 *   --p    — pre-decrement: correctly scales by sizeof(*p)
 *   p + n  — correctly scales by sizeof(*p) (fixed; see t15_ptrscale.c)
 *   p - n  — correctly scales by sizeof(*p) (fixed; see t15_ptrscale.c)
 *
 * Detailed *(p+n) / *(p-n) coverage is in t15_ptrscale.c.
 */
#include "testutil.h"

sum_ptr(p, len)
    int *p; int len;
{
    int s;
    s = 0;
    while(len--) s += *p++;
    return s;
}

/* Index-based reverse — avoids unscaled pointer arithmetic */
reverse(arr, len)
    int *arr; int len;
{
    int lo, hi, tmp;
    lo = 0;
    hi = len - 1;
    while(lo < hi) {
        tmp      = arr[lo];
        arr[lo]  = arr[hi];
        arr[hi]  = tmp;
        ++lo;
        --hi;
    }
}

my_strlen(s)
    char *s;
{
    char *p;
    p = s;
    while(*p) ++p;
    return (int)(p - s);
}

fill_and_sum(buf, len, val)
    char *buf; int len; char val;
{
    int i, s;
    char *p;
    p = buf;
    for(i = 0; i < len; ++i) *p++ = val;
    s = 0;
    p = buf;
    for(i = 0; i < len; ++i) s += (int)*p++;
    return s;
}

int  grid[3][4];
int  arr[6];
char cbuf[8];

init_grid()
{
    int r, c;
    for(r = 0; r < 3; ++r)
        for(c = 0; c < 4; ++c)
            grid[r][c] = r * 10 + c;
}

main()
{
    int *p;
    int  x, sz, r;

    putstr("t03_pointers\n");

    arr[0]=1; arr[1]=2; arr[2]=3; arr[3]=4; arr[4]=5; arr[5]=6;

    EXPECT_EQ("idx-0",  arr[0], 1)
    EXPECT_EQ("idx-5",  arr[5], 6)

    EXPECT_EQ("sum-ptr", sum_ptr(arr, 6), 21)

    x = 42; p = &x;
    EXPECT_EQ("addr-deref",    *p, 42)
    *p = 99;
    EXPECT_EQ("store-via-ptr", x,  99)

    /* subscript on int* pointer (correctly scaled) */
    p = arr;
    EXPECT_EQ("psub-0", p[0], 1)
    EXPECT_EQ("psub-1", p[1], 2)
    EXPECT_EQ("psub-5", p[5], 6)

    /* ++ and -- scale correctly */
    p = arr;
    EXPECT_EQ("ptr-postinc", *p++, 1)
    EXPECT_EQ("ptr-after",   *p,   2)
    EXPECT_EQ("ptr-predec",  *--p, 1)

    /* index-based reverse */
    reverse(arr, 6);
    EXPECT_EQ("rev-0", arr[0], 6)
    EXPECT_EQ("rev-5", arr[5], 1)

    EXPECT_EQ("my-strlen-0", my_strlen(""),      0)
    EXPECT_EQ("my-strlen-5", my_strlen("hello"), 5)

    r = fill_and_sum(cbuf, 4, (char)3);
    EXPECT_EQ("fill-sum", r, 12)

    init_grid();
    EXPECT_EQ("grid-0-0", grid[0][0],  0)
    EXPECT_EQ("grid-1-2", grid[1][2], 12)
    EXPECT_EQ("grid-2-3", grid[2][3], 23)

    sz = sizeof(int);  EXPECT_EQ("sizeof-int",  sz, 2)
    sz = sizeof(char); EXPECT_EQ("sizeof-char", sz, 1)
    sz = sizeof(arr);  EXPECT_EQ("sizeof-arr",  sz, 12)

    SUITE_DONE()
}

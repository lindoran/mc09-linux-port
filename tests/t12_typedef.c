/*
 * t12_typedef.c — typedef, long, and stdint.h tests
 *
 * Covers: scalar typedefs, pointer typedefs, named struct typedefs,
 *         anonymous struct typedefs, typedef in sizeof, long as 32-bit
 *         storage, stdint.h, stdbool.h.
 *
 * LONG NOTE: direct assignment of int literal to long is a compile error.
 * Use longset(&var, value). No expression-level long arithmetic; use LONGMATH.
 */
#include "testutil.h"
#include <stdint.h>
#include <stdbool.h>

extern longset(), longcmp();

typedef struct { int x; int y; }            Vec2;
typedef Vec2 *Vec2Ptr;
typedef struct { uint8_t r; uint8_t g; uint8_t b; } RGB;

/* Globals — declared BEFORE any pointer typedef to avoid compiler bug */
uint8_t  a8;
uint16_t a16;
int8_t   s8;
int16_t  s16;
bool     flag;
Vec2     origin;
RGB      red;
long     big_a;
long     big_b;
int32_t  big_c;
int      szb[8];

set_vec(v, x, y) Vec2 *v; int x, y; { v->x = x; v->y = y; }
sum_vec(a, b)    Vec2  a, b;         { return a.x + b.x + a.y + b.y; }

main()
{
    Vec2Ptr pv;
    Vec2  local_v;
    int   r;

    putstr("t12_typedef\n");

    a8  = 255;  a16 = 1000;
    s8  = -100; s16 = -1000;
    flag = true;

    EXPECT_EQ("u8-val",   (int)a8  & 0xFF, 255)
    EXPECT_EQ("u16-val",  (int)a16,  1000)
    EXPECT_EQ("s8-val",   (int)s8,   -100)
    EXPECT_EQ("s16-val",  s16,      -1000)
    EXPECT_NZ("bool-t",   flag)
    flag = false;
    EXPECT_Z("bool-f",    flag)

    set_vec(&origin, 3, 4);
    EXPECT_EQ("vec-x",    origin.x, 3)
    EXPECT_EQ("vec-y",    origin.y, 4)

    pv = &origin;
    EXPECT_EQ("vecptr-x", pv->x, 3)
    EXPECT_EQ("vecptr-y", pv->y, 4)

    local_v.x = 10; local_v.y = 20;
    r = sum_vec(origin, local_v);
    EXPECT_EQ("sum-vec",  r, 37)

    red.r = 255; red.g = 0; red.b = 0;
    EXPECT_EQ("rgb-r",    (int)red.r & 0xFF, 255)
    EXPECT_EQ("rgb-g",    (int)red.g & 0xFF, 0)

    szb[0] = sizeof(uint8_t);   szb[1] = sizeof(uint16_t);
    szb[2] = sizeof(int8_t);    szb[3] = sizeof(int16_t);
    szb[4] = sizeof(bool);      szb[5] = sizeof(long);
    szb[6] = sizeof(int32_t);   szb[7] = sizeof(Vec2);

    EXPECT_EQ("sz-u8",   szb[0], 1)
    EXPECT_EQ("sz-u16",  szb[1], 2)
    EXPECT_EQ("sz-i8",   szb[2], 1)
    EXPECT_EQ("sz-i16",  szb[3], 2)
    EXPECT_EQ("sz-bool", szb[4], 1)
    EXPECT_EQ("sz-long", szb[5], 4)
    EXPECT_EQ("sz-i32",  szb[6], 4)
    EXPECT_EQ("sz-vec2", szb[7], 4)

    longset(&big_a, 42);
    longset(&big_b, 42);
    longset(&big_c, 99);

    r = longcmp(&big_a, &big_b);
    EXPECT_EQ("long-eq", r,  0)
    r = longcmp(&big_a, &big_c);
    EXPECT_EQ("long-lt", r, -1)
    r = longcmp(&big_c, &big_a);
    EXPECT_EQ("long-gt", r,  1)

    SUITE_DONE()
}

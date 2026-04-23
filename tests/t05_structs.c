/*
 * t05_structs.c — struct and union tests
 *
 * FINDINGS:
 *   struct assignment (s2 = s1) — NOT supported: "Non-assignable".
 *     Workaround: pointer-based field copy.
 *   unsigned char members — zero-extend correctly when read via (int)member.
 *     (Fix 2: UNSIGNED bit now preserved through the (int) cast; expand()
 *      emits CLRA instead of SEX.  No & 0xFF mask needed.)
 *   union with array member — "Inconsistent member type/offset".
 *     Workaround: use scalar members only.
 *   union byte member on 6809 (big-endian): a char member at offset 0
 *     overlaps the HIGH byte of the int; at offset 1 it overlaps the LOW byte.
 *     sizeof(union{int i; char c;}) = 2.
 */
#include "testutil.h"

extern memcpy();

struct Point { int x; int y; };
struct Rect  { struct Point tl; struct Point br; };
struct Pixel { unsigned char r; unsigned char g; unsigned char b; unsigned char pad; };
union  Word  { int i; char lo; };   /* lo at offset 0 = HIGH byte on 6809 */

make_point(p, x, y)
    struct Point *p; int x, y;
{ p->x = x; p->y = y; }

copy_point(dst, src)
    struct Point *dst; struct Point *src;
{ dst->x = src->x; dst->y = src->y; }

area(r)
    struct Rect *r;
{
    int w, h;
    w = r->br.x - r->tl.x;
    h = r->br.y - r->tl.y;
    return w * h;
}

show_sum(a, b)
    struct Point a, b;
{ return a.x + b.x + a.y + b.y; }

struct Point p1, p2;
struct Rect  r;
struct Pixel px[3];
union  Word  w;

main()
{
    int sz;

    putstr("t05_structs\n");

    /* dot access */
    p1.x = 10; p1.y = 20;
    EXPECT_EQ("dot-x", p1.x, 10)
    EXPECT_EQ("dot-y", p1.y, 20)

    /* arrow through pointer */
    make_point(&p1, 3, 7);
    EXPECT_EQ("arrow-x", p1.x, 3)
    EXPECT_EQ("arrow-y", p1.y, 7)

    /* copy_point instead of struct assignment */
    copy_point(&p2, &p1);
    EXPECT_EQ("copy-x", p2.x, 3)
    EXPECT_EQ("copy-y", p2.y, 7)
    p2.x = 99;
    EXPECT_EQ("copy-orig", p1.x,  3)
    EXPECT_EQ("copy-new",  p2.x, 99)

    /* nested struct */
    r.tl.x = 0; r.tl.y = 0;
    r.br.x = 8; r.br.y = 5;
    EXPECT_EQ("nest-tl-x", r.tl.x, 0)
    EXPECT_EQ("nest-br-y", r.br.y, 5)
    EXPECT_EQ("area",      area(&r), 40)

    /* struct pass-by-value */
    p1.x = 1; p1.y = 2;
    p2.x = 3; p2.y = 4;
    EXPECT_EQ("pass-val",    show_sum(p1, p2), 10)
    EXPECT_EQ("pval-orig-x", p1.x, 1)
    EXPECT_EQ("pval-orig-y", p1.y, 2)

    /* array of structs — unsigned char members zero-extend correctly (Fix 2) */
    px[0].r = 255; px[0].g = 0;   px[0].b = 0;
    px[1].r = 0;   px[1].g = 200; px[1].b = 0;
    px[2].r = 0;   px[2].g = 0;   px[2].b = 128;
    EXPECT_EQ("arr-px0-r", (int)px[0].r, 255)
    EXPECT_EQ("arr-px1-g", (int)px[1].g, 200)
    EXPECT_EQ("arr-px2-b", (int)px[2].b, 128)

    /* sizeof */
    sz = sizeof(struct Point); EXPECT_EQ("sizeof-pt", sz, 4)
    sz = sizeof(struct Pixel); EXPECT_EQ("sizeof-px", sz, 4)
    sz = sizeof(struct Rect);  EXPECT_EQ("sizeof-rc", sz, 8)

    /* union: int member */
    w.i = 0x1234;
    EXPECT_EQ("union-lo-mask", w.i & 0xFF,        0x34)
    EXPECT_EQ("union-hi-mask", (w.i >> 8) & 0xFF, 0x12)

    /* union char member 'lo' at offset 0 = HIGH byte on big-endian 6809 */
    w.i = 0x5678;
    EXPECT_EQ("union-char-hi", (int)w.lo & 0xFF, 0x56)

    /* write char member, read int */
    w.i = 0;
    w.lo = (char)0x42;
    EXPECT_EQ("union-char-to-int-hi", (w.i >> 8) & 0xFF, 0x42)

    sz = sizeof(union Word); EXPECT_EQ("sizeof-union", sz, 2)

    SUITE_DONE()
}

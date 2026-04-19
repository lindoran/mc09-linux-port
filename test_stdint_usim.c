#include "stdint.h"
#include "stdbool.h"

uint8_t   a;
uint16_t  b;
int8_t    c;
int16_t   d;
bool      flag;
long      big;
int32_t   big2;

/* sizeof results */
int sz[7];

main()
{
    a    = 255;
    b    = 1000;
    c    = -100;
    d    = -1000;
    flag = true;

    /* longs are storage only - assign via longset */
    longset(&big,  42);
    longset(&big2, 99);

    sz[0] = sizeof(uint8_t);
    sz[1] = sizeof(uint16_t);
    sz[2] = sizeof(int8_t);
    sz[3] = sizeof(int16_t);
    sz[4] = sizeof(bool);
    sz[5] = sizeof(long);
    sz[6] = sizeof(int32_t);

    putstr("stdint+long test\n");
    putstr("a=");   putnum(a,   10); putchr('\n');
    putstr("b=");   putnum(b,   10); putchr('\n');
    putstr("c=");   putnum(c,   10); putchr('\n');
    putstr("d=");   putnum(d,   10); putchr('\n');
    putstr("flag=");putnum(flag,10); putchr('\n');
    putstr("sz8="); putnum(sz[0],10);putchr('\n');
    putstr("sz16=");putnum(sz[1],10);putchr('\n');
    putstr("sz_bool=");putnum(sz[4],10);putchr('\n');
    putstr("szlong=");putnum(sz[5],10);putchr('\n');
    putstr("PASS\n");
}

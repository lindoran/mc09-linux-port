#include "6809io.h"
#define DEBUG
#include "assert.h"

check(x)
    int x;
{
    assert(x > 0)
}

main()
{
    check(1);
    check(0);
}

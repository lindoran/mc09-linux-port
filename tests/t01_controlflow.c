/*
 * t01_controlflow.c — control flow tests
 * Covers: if/else, while, for, do/while, break, continue, nested loops.
 */
#include "testutil.h"

count_while(limit) int limit; {
    int n; n = 0; while(n < limit) ++n; return n;
}
count_for(limit) int limit; {
    int i, n; n = 0; for(i = 0; i < limit; ++i) ++n; return n;
}
count_dowhile(limit) int limit; {
    int n; n = 0; do { ++n; } while(n < limit); return n;
}

int brk_arr[5];

find_break(target) int target; {
    int i;
    for(i = 0; i < 5; ++i)
        if(brk_arr[i] >= target) break;
    if(i == 5) return -1;
    return i;
}

int evn_arr[5];

sum_evens() {
    int i, s; s = 0;
    for(i = 0; i < 5; ++i) {
        if(evn_arr[i] & 1) continue;
        s += evn_arr[i];
    }
    return s;
}

count_pairs(limit) int limit; {
    int i, j, n; n = 0;
    for(i = 0; i < limit; ++i)
        for(j = 0; j < limit; ++j)
            if(i + j < limit) ++n;
    return n;
}

classify(n) int n; {
    if(n < 0)       return -1;
    else if(n != 0) {
        if(n < 10)  return 1;
        else        return 2;
    }
    return 0;
}

main()
{
    int r;
    putstr("t01_controlflow\n");

    brk_arr[0]=2; brk_arr[1]=5; brk_arr[2]=7; brk_arr[3]=10; brk_arr[4]=14;
    evn_arr[0]=2; evn_arr[1]=5; evn_arr[2]=7; evn_arr[3]=10; evn_arr[4]=14;

    EXPECT_EQ("if-true",  classify(5),   1)
    EXPECT_EQ("if-neg",   classify(-1), -1)
    EXPECT_EQ("if-zero",  classify(0),   0)
    EXPECT_EQ("if-hi",    classify(42),  2)

    EXPECT_EQ("while-0",  count_while(0),  0)
    EXPECT_EQ("while-1",  count_while(1),  1)
    EXPECT_EQ("while-10", count_while(10), 10)

    EXPECT_EQ("for-0",  count_for(0),  0)
    EXPECT_EQ("for-5",  count_for(5),  5)
    EXPECT_EQ("for-20", count_for(20), 20)

    EXPECT_EQ("dowhile-1", count_dowhile(1), 1)
    EXPECT_EQ("dowhile-5", count_dowhile(5), 5)

    EXPECT_EQ("break-found", find_break(7),  2)
    EXPECT_EQ("break-first", find_break(1),  0)
    EXPECT_EQ("break-miss",  find_break(99), -1)

    r = sum_evens();   /* 2 + 10 + 14 = 26 */
    EXPECT_EQ("continue-evens", r, 26)

    EXPECT_EQ("nested-3", count_pairs(3), 6)
    EXPECT_EQ("nested-4", count_pairs(4), 10)

    SUITE_DONE()
}

/*
 * t02_switch.c — switch/case/default tests
 * Covers: basic dispatch, default, fall-through, char switch, switch in loop.
 */
#include "testutil.h"

day_type(d) int d; {
    switch(d) {
        case 0: return 0;
        case 6: return 0;
        default: return 1;
    }
}

fallthrough_count(n) int n; {
    int count; count = 0;
    switch(n) {
        case 3: ++count;
        case 2: ++count;
        case 1: ++count;
        case 0: ++count;
        default: break;
    }
    return count;
}

vowel_check(c) char c; {
    switch(c) {
        case 'a': case 'e': case 'i': case 'o': case 'u': return 1;
        default: return 0;
    }
}

int sw_arr[6];

sum_of_kind() {
    int i, s; s = 0;
    for(i = 0; i < 6; ++i) {
        switch(sw_arr[i] % 3) {
            case 0: s += sw_arr[i]; break;
            case 1: s -= sw_arr[i]; break;
            case 2: break;
        }
    }
    return s;
}

large_case(n) int n; {
    switch(n) {
        case 100: return 1;
        case 200: return 2;
        case 300: return 3;
        default:  return 0;
    }
}

main()
{
    int r;
    putstr("t02_switch\n");

    sw_arr[0]=0; sw_arr[1]=1; sw_arr[2]=2; sw_arr[3]=3; sw_arr[4]=4; sw_arr[5]=5;

    EXPECT_EQ("sw-sun", day_type(0), 0)
    EXPECT_EQ("sw-sat", day_type(6), 0)
    EXPECT_EQ("sw-mon", day_type(1), 1)
    EXPECT_EQ("sw-fri", day_type(5), 1)
    EXPECT_EQ("sw-def", day_type(99), 1)

    EXPECT_EQ("fall-3",  fallthrough_count(3),  4)
    EXPECT_EQ("fall-2",  fallthrough_count(2),  3)
    EXPECT_EQ("fall-1",  fallthrough_count(1),  2)
    EXPECT_EQ("fall-0",  fallthrough_count(0),  1)
    EXPECT_EQ("fall-99", fallthrough_count(99), 0)

    EXPECT_NZ("vowel-a", vowel_check('a'))
    EXPECT_NZ("vowel-e", vowel_check('e'))
    EXPECT_Z ("vowel-b", vowel_check('b'))
    EXPECT_Z ("vowel-z", vowel_check('z'))

    /* sw_arr mod3==0: add 0+3=3; mod3==1: sub 1+4=5; mod3==2: skip 2,5 → net=-2 */
    r = sum_of_kind();
    EXPECT_EQ("sw-loop", r, -2)

    EXPECT_EQ("large-100", large_case(100), 1)
    EXPECT_EQ("large-200", large_case(200), 2)
    EXPECT_EQ("large-300", large_case(300), 3)
    EXPECT_EQ("large-def", large_case(50),  0)

    SUITE_DONE()
}

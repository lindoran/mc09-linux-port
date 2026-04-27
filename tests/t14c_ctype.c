/*
 * t14c_ctype.c — character classification tests, part 3
 *
 * Covers: islower, isalnum
 *
 * Split from t14b because combined output exceeds coco_test output buffer.
 * Part 4: t14d_ctype.c
 */
#include "testutil.h"

extern islower(), isalnum();

main()
{
    putstr("t14c_ctype\n");

    EXPECT_NZ("islwr-a",   islower('a'))
    EXPECT_NZ("islwr-z",   islower('z'))
    EXPECT_Z ("islwr-A",   islower('A'))
    EXPECT_Z ("islwr-0",   islower('0'))

    EXPECT_NZ("isalnum-a", isalnum('a'))
    EXPECT_NZ("isalnum-0", isalnum('0'))
    EXPECT_Z ("isalnum-sp",isalnum(' '))
    EXPECT_Z ("isalnum-!", isalnum('!'))

    SUITE_DONE()
}

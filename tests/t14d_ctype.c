/*
 * t14d_ctype.c — character classification tests, part 4
 *
 * Covers: isprint, ispunct
 *
 * Split from t14c because combined output exceeds coco_test output buffer.
 */
#include "testutil.h"

extern isprint(), ispunct();

main()
{
    putstr("t14d_ctype\n");

    EXPECT_NZ("isprint-A", isprint('A'))
    EXPECT_NZ("isprint-sp",isprint(' '))
    EXPECT_Z ("isprint-nl",isprint('\n'))

    EXPECT_NZ("ispunct-!", ispunct('!'))
    EXPECT_NZ("ispunct-.", ispunct('.'))
    EXPECT_Z ("ispunct-a", ispunct('a'))
    EXPECT_Z ("ispunct-0", ispunct('0'))

    SUITE_DONE()
}

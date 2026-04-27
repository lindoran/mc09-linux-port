/*
 * t14b_ctype.c — character classification tests, part 1
 *
 * Covers: isdigit, isalpha, isspace, isupper
 *
 * Split from original t14b because combined output exceeds the
 * coco_test output buffer. Part 2: t14c_ctype.c
 */
#include "testutil.h"

extern isdigit(), isalpha(), isspace(), isupper();

main()
{
    putstr("t14b_ctype\n");

    EXPECT_NZ("isdig-0",  isdigit('0'))
    EXPECT_NZ("isdig-9",  isdigit('9'))
    EXPECT_Z ("isdig-a",  isdigit('a'))
    EXPECT_Z ("isdig-sp", isdigit(' '))

    EXPECT_NZ("isalp-a",  isalpha('a'))
    EXPECT_NZ("isalp-Z",  isalpha('Z'))
    EXPECT_Z ("isalp-0",  isalpha('0'))
    EXPECT_Z ("isalp-sp", isalpha(' '))

    EXPECT_NZ("isspc-sp", isspace(' '))
    EXPECT_NZ("isspc-nl", isspace('\n'))
    EXPECT_NZ("isspc-tb", isspace('\t'))
    EXPECT_Z ("isspc-a",  isspace('a'))

    EXPECT_NZ("isupr-A",  isupper('A'))
    EXPECT_NZ("isupr-Z",  isupper('Z'))
    EXPECT_Z ("isupr-a",  isupper('a'))
    EXPECT_Z ("isupr-0",  isupper('0'))

    SUITE_DONE()
}

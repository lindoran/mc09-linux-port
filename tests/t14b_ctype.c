/*
 * t14b_ctype.c — character classification library tests
 *
 * Covers: isdigit, isalpha, isspace, isupper, islower, isalnum,
 *         isprint, ispunct (ISFUNS.ASM).
 *
 * ISFUNS.ASM is kept in a separate file from t14_stdlib because combined
 * they overflow the 4KB ROM space of the usim09 target.
 */
#include "testutil.h"

extern isdigit(), isalpha(), isspace();
extern isupper(), islower(), isalnum();
extern isprint(), ispunct(), isascii();

main()
{
    putstr("t14b_ctype\n");

    EXPECT_NZ("isdig-0", isdigit('0'))
    EXPECT_NZ("isdig-9", isdigit('9'))
    EXPECT_Z ("isdig-a", isdigit('a'))
    EXPECT_Z ("isdig-sp",isdigit(' '))

    EXPECT_NZ("isalp-a", isalpha('a'))
    EXPECT_NZ("isalp-Z", isalpha('Z'))
    EXPECT_Z ("isalp-0", isalpha('0'))
    EXPECT_Z ("isalp-sp",isalpha(' '))

    EXPECT_NZ("isspc-sp", isspace(' '))
    EXPECT_NZ("isspc-nl", isspace('\n'))
    EXPECT_NZ("isspc-tb", isspace('\t'))
    EXPECT_Z ("isspc-a",  isspace('a'))

    EXPECT_NZ("isupr-A", isupper('A'))
    EXPECT_NZ("isupr-Z", isupper('Z'))
    EXPECT_Z ("isupr-a", isupper('a'))
    EXPECT_Z ("isupr-0", isupper('0'))

    EXPECT_NZ("islwr-a", islower('a'))
    EXPECT_NZ("islwr-z", islower('z'))
    EXPECT_Z ("islwr-A", islower('A'))
    EXPECT_Z ("islwr-0", islower('0'))

    EXPECT_NZ("isalnum-a", isalnum('a'))
    EXPECT_NZ("isalnum-0", isalnum('0'))
    EXPECT_Z ("isalnum-sp",isalnum(' '))
    EXPECT_Z ("isalnum-!", isalnum('!'))

    EXPECT_NZ("isprint-A", isprint('A'))
    EXPECT_NZ("isprint-sp",isprint(' '))
    EXPECT_Z ("isprint-nl",isprint('\n'))

    EXPECT_NZ("ispunct-!", ispunct('!'))
    EXPECT_NZ("ispunct-.", ispunct('.'))
    EXPECT_Z ("ispunct-a", ispunct('a'))
    EXPECT_Z ("ispunct-0", ispunct('0'))

    SUITE_DONE()
}

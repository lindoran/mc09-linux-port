/*
 * t04_strings.c — string library tests
 *
 * Covers: strlen, strcpy, strcat, strcmp, strchr.
 *
 * IMPORTANT — strcmp argument order quirk:
 *   Micro-C pushes args left-to-right, so the first C argument lands at
 *   the HIGHER stack offset (4,S), and the second lands at 2,S (nearest SP).
 *   STRING2.ASM loads X from 2,S ("string1") and U from 4,S ("string2"),
 *   so it treats the SECOND C argument as string1 and the FIRST as string2.
 *   Result: strcmp(s1,s2) returns +1 when s1 < s2 (inverted from ANSI C).
 *   This is documented in LIB09.TXT: "1=string1>string2; -1=string2>string1"
 *   where "string1" maps to the 2nd C argument.
 *   Use strcmp only for equality (return==0); avoid relying on sign.
 */
#include "testutil.h"

extern strlen(), strcpy(), strcat(), strcmp(), strchr();

char buf1[32];
char buf2[32];

main()
{
    char *p;

    putstr("t04_strings\n");

    /* strlen */
    EXPECT_EQ("strlen-empty", strlen(""),        0)
    EXPECT_EQ("strlen-1",     strlen("x"),       1)
    EXPECT_EQ("strlen-hello", strlen("hello"),   5)
    EXPECT_EQ("strlen-mixed", strlen("abc123"),  6)

    /* strcpy */
    strcpy(buf1, "hello");
    EXPECT_STR("strcpy-basic", buf1, "hello")

    strcpy(buf1, "");
    EXPECT_STR("strcpy-empty", buf1, "")

    strcpy(buf1, "Micro-C");
    EXPECT_EQ("strcpy-len", strlen(buf1), 7)

    /* strcat */
    strcpy(buf1, "foo");
    strcat(buf1, "bar");
    EXPECT_STR("strcat-basic",     buf1, "foobar")
    EXPECT_EQ ("strcat-len",       strlen(buf1), 6)

    strcpy(buf1, "");
    strcat(buf1, "baz");
    EXPECT_STR("strcat-empty-dst", buf1, "baz")

    strcpy(buf1, "hello");
    strcat(buf1, "");
    EXPECT_STR("strcat-empty-src", buf1, "hello")

    /* strcmp — equality only (sign is inverted vs ANSI C, see file header) */
    EXPECT_Z ("strcmp-eq",    strcmp("abc", "abc"))
    EXPECT_Z ("strcmp-empty", strcmp("", ""))
    EXPECT_NZ("strcmp-diff",  strcmp("abc", "abd"))

    /* sign IS inverted: strcmp("abc","abd") -> +1 (abc<abd but returns positive) */
    EXPECT_EQ("strcmp-inv-lt", strcmp("abc","abd"),  1)
    EXPECT_EQ("strcmp-inv-gt", strcmp("abd","abc"), -1)

    /* round-trip */
    strcpy(buf2, "round-trip");
    strcpy(buf1, buf2);
    EXPECT_STR("roundtrip", buf1, "round-trip")

    /* strchr */
    strcpy(buf1, "hello world");

    p = strchr(buf1, 'o');
    EXPECT_NZ("strchr-found",   p)
    EXPECT_EQ("strchr-char",    *p, 'o')
    EXPECT_EQ("strchr-offset",  (int)(p - buf1), 4)

    p = strchr(buf1, 'w');
    EXPECT_NZ("strchr-w", p)
    EXPECT_EQ("strchr-w-off",   (int)(p - buf1), 6)

    p = strchr(buf1, 'z');
    EXPECT_Z ("strchr-missing", p)

    p = strchr(buf1, 'h');
    EXPECT_EQ("strchr-first",   (int)(p - buf1), 0)

    /* strchr null terminator */
    p = strchr(buf1, 0);
    EXPECT_NZ("strchr-null",    p)
    EXPECT_EQ("strchr-null-off",(int)(p - buf1), 11)

    SUITE_DONE()
}

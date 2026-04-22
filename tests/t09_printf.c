/*
 * t09_printf.c — printf and sprintf tests
 *
 * Covers: %d, %u, %x, %s, %c, field width, left-justify, zero-fill.
 * sprintf writes to a buffer; we verify the buffer with strcmp.
 *
 * NOTES:
 *   printf/sprintf are declared 'register' — the compiler passes arg count
 *   in D before the call. Declare with 'extern register'.
 *   printf has a 100-byte internal buffer. Keep format output under 100 chars.
 *   No %f, %e, %g — no floating point anywhere in the library.
 */
#include "testutil.h"

extern strlen(), strcmp();
extern register printf(), sprintf();

char buf[64];

main()
{
    putstr("t09_printf\n");

    /* sprintf %d */
    sprintf(buf, "%d", 0);        EXPECT_STR("sp-d-0",   buf, "0")
    sprintf(buf, "%d", 42);       EXPECT_STR("sp-d-42",  buf, "42")
    sprintf(buf, "%d", -1);       EXPECT_STR("sp-d-neg", buf, "-1")
    sprintf(buf, "%d", 32767);    EXPECT_STR("sp-d-max", buf, "32767")

    /* sprintf %u */
    sprintf(buf, "%u", 0);        EXPECT_STR("sp-u-0",   buf, "0")
    sprintf(buf, "%u", 65535);    EXPECT_STR("sp-u-max", buf, "65535")
    sprintf(buf, "%u", 1000);     EXPECT_STR("sp-u-1k",  buf, "1000")

    /* sprintf %x */
    sprintf(buf, "%x", 0);        EXPECT_STR("sp-x-0",   buf, "0")
    sprintf(buf, "%x", 255);      EXPECT_STR("sp-x-ff",  buf, "FF")
    sprintf(buf, "%x", 0x1234);   EXPECT_STR("sp-x-1234",buf, "1234")
    sprintf(buf, "%x", 0xABCD);   EXPECT_STR("sp-x-abcd",buf, "ABCD")

    /* sprintf %s */
    sprintf(buf, "%s", "hello");  EXPECT_STR("sp-s-hello", buf, "hello")
    sprintf(buf, "%s", "");       EXPECT_STR("sp-s-empty", buf, "")

    /* sprintf %c */
    sprintf(buf, "%c", 'A');      EXPECT_STR("sp-c-A", buf, "A")
    sprintf(buf, "%c", '0');      EXPECT_STR("sp-c-0", buf, "0")

    /* field width: right-justify */
    sprintf(buf, "%5d", 42);      EXPECT_STR("sp-w5d",   buf, "   42")
    sprintf(buf, "%5d", -1);      EXPECT_STR("sp-w5neg", buf, "-   1")
    sprintf(buf, "%5u", 100);     EXPECT_STR("sp-w5u",   buf, "  100")

    /* left-justify with '-' flag */
    sprintf(buf, "%-5d", 42);     EXPECT_STR("sp-lj5d",  buf, "42   ")
    sprintf(buf, "%-5s", "hi");   EXPECT_STR("sp-lj5s",  buf, "hi   ")

    /* zero-fill */
    sprintf(buf, "%05d", 42);     EXPECT_STR("sp-z5d",   buf, "00042")
    sprintf(buf, "%05x", 0xFF);   EXPECT_STR("sp-z5x",   buf, "000FF")

    /* multiple specifiers */
    sprintf(buf, "%d+%d=%d", 3, 4, 7);
    EXPECT_STR("sp-multi", buf, "3+4=7")

    /* literal percent and chars in format */
    sprintf(buf, "n=%d!", 99);
    EXPECT_STR("sp-mix", buf, "n=99!")

    SUITE_DONE()
}

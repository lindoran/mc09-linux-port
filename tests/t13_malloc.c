/*
 * t13_malloc.c — heap allocation tests
 *
 * Covers: malloc, free, allocate multiple blocks, free and reallocate,
 *         coalescing of adjacent free blocks, stack guard check.
 *
 * MALLOC NOTES (from STDLIB.md):
 *   malloc(size) — returns pointer to block, or 0 on failure.
 *   free(ptr)    — release block; coalesces adjacent free blocks.
 *   Heap grows upward from ?heap (end of BSS). Stack guard: 1000 bytes
 *   between heap top and SP. On usim09: stack at $7F00, code+BSS at
 *   ~$E000+ (ROM) and $0100+ (RAM). Heap is in the RAM region.
 *   malloc(0) is undefined; test with size >= 2.
 */
#include "testutil.h"

extern malloc(), free(), memset(), memcpy(), strlen(), strcmp(), strcpy();

char *pa, *pb, *pc;

main()
{
    int r;

    putstr("t13_malloc\n");

    /* basic alloc */
    pa = malloc(10);
    EXPECT_NZ("malloc-10", pa)

    pb = malloc(20);
    EXPECT_NZ("malloc-20", pb)

    /* blocks don't overlap — pb > pa */
    EXPECT_NZ("no-overlap", (int)pb > (int)pa)

    /* write and read back */
    strcpy(pa, "hello");
    EXPECT_STR("write-pa", pa, "hello")

    strcpy(pb, "world!");
    EXPECT_STR("write-pb", pb, "world!")

    /* pa still intact after writing pb */
    EXPECT_STR("pa-intact", pa, "hello")

    /* free pb, reallocate — should get same or overlapping space */
    free(pb);
    pc = malloc(10);
    EXPECT_NZ("realloc", pc)

    /* write to pc, pa must still be intact */
    strcpy(pc, "again");
    EXPECT_STR("pc-write",  pc, "again")
    EXPECT_STR("pa-still",  pa, "hello")

    /* free all */
    free(pa);
    free(pc);

    /* allocate again after full free */
    pa = malloc(16);
    EXPECT_NZ("after-free", pa)
    strcpy(pa, "fresh");
    EXPECT_STR("fresh-str", pa, "fresh")
    free(pa);

    /* multiple small allocations */
    pa = malloc(2);
    pb = malloc(2);
    pc = malloc(2);
    EXPECT_NZ("sm-a", pa)
    EXPECT_NZ("sm-b", pb)
    EXPECT_NZ("sm-c", pc)
    EXPECT_NZ("sm-ab", (int)pb != (int)pa)
    EXPECT_NZ("sm-bc", (int)pc != (int)pb)

    /* write to each */
    pa[0] = 'A'; pa[1] = 0;
    pb[0] = 'B'; pb[1] = 0;
    pc[0] = 'C'; pc[1] = 0;
    EXPECT_STR("sm-pa", pa, "A")
    EXPECT_STR("sm-pb", pb, "B")
    EXPECT_STR("sm-pc", pc, "C")

    free(pb);          /* free middle block */
    free(pa);
    free(pc);

    SUITE_DONE()
}

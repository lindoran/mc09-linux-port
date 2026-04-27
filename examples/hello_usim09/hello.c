/*
 * hello.c — Micro-C 6809 hello world for the usim09 simulator
 *
 * Build and run:
 *   make
 *   make run
 *
 * Or manually:
 *   MCDIR=../.. MCINCLUDE=../../include MCLIBDIR=../../targets/usim09/lib09 \
 *     ../../cc09 hello.c -Iq
 *   echo "" | usim09 hello.HEX
 *
 * Expected output:
 *   Hello from Micro-C 6809!
 *   Count: 1 2 3 4 5
 *
 * Target notes (usim09):
 *   CODE_ORG  = $E000   ROM region, 8KB ($E000-$FFFF)
 *   STACK_TOP = $7F00   Top of 32KB RAM
 *   RAM_ORG   = $0100   Globals placed here (above zero page)
 *   I/O       = MC6850 ACIA at $C000 — putstr/putchr write to status/data
 *   Exit      = spin ($E000 BRA loop) — usim09 stops when output ends
 *   Vectors   = emitted at $FFF0 by mktarget, RESET -> $E000 (?begin)
 *
 * K&R C notes (Micro-C uses K&R, not ANSI):
 *   - Parameter types declared separately, not in the argument list
 *   - All local variable declarations must appear at the TOP of a function,
 *     before any statements
 *   - No // comments — use only the slash-star form
 */

extern putstr(), putchr();

/*
 * itoa_simple — write a decimal integer to the console.
 *
 * Micro-C does not include printf by default in the usim09 build.
 * This simple recursive routine handles non-negative integers only.
 */
itoa_simple(n)
    int n;
{
    if (n >= 10)
        itoa_simple(n / 10);
    putchr('0' + n % 10);
}

main()
{
    int i;

    putstr("Hello from Micro-C 6809!\r\n");

    putstr("Count:");
    i = 1;
    while (i <= 5) {
        putchr(' ');
        itoa_simple(i);
        i = i + 1;
    }
    putstr("\r\n");
}

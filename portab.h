/*
 * portab.h - Linux portability shims for Dunfield Micro-C / asm09
 *
 * Fixes:
 *  1. abort(msg) - Dunfield uses it with a message string; ANSI abort() takes
 *                  none. Route via macro to avoid redefining the libc symbol.
 *  2. strupr()   - MSVC/Borland, absent on glibc.
 */
#ifndef PORTAB_H
#define PORTAB_H

/* Dunfield abort(msg) -> die(msg).  Macro avoids redefining ANSI abort(). */
#define die(msg)   do { fputs((msg), stderr); exit(1); } while(0)

#endif /* PORTAB_H */

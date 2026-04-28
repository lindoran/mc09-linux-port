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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Dunfield abort(msg) -> die(msg).  Macro avoids redefining ANSI abort(). */
#define die(msg)   do { fputs((msg), stderr); exit(1); } while(0)
//#define abort(msg) die(msg)

/* strupr() - not in POSIX */
#ifndef _STRUPR_DEFINED
#define _STRUPR_DEFINED
//static char *strupr(char *s) {
//    char *p = s;
//    while (*p) { *p = (char)toupper((unsigned char)*p); ++p; }
//    return s;
//}
#endif

#endif /* PORTAB_H */

/*
 * stdint.h - Fixed-width integer types for Micro-C 6809
 *
 * On the 6809 with Micro-C:
 *   char  = 8 bits
 *   int   = 16 bits
 *   long  = 32 bits (storage type; use LONGMATH library for arithmetic)
 */

#ifndef STDINT_H
#define STDINT_H

/* --- Exact-width signed types --- */
typedef char            int8_t;
typedef int             int16_t;
typedef long            int32_t;

/* --- Exact-width unsigned types --- */
typedef unsigned char   uint8_t;
typedef unsigned int    uint16_t;
typedef unsigned long   uint32_t;

/* --- Least-width types (same as exact on this target) --- */
typedef int8_t          int_least8_t;
typedef int16_t         int_least16_t;
typedef int32_t         int_least32_t;
typedef uint8_t         uint_least8_t;
typedef uint16_t        uint_least16_t;
typedef uint32_t        uint_least32_t;

/* --- Fast types (same as exact on this target) --- */
typedef int8_t          int_fast8_t;
typedef int16_t         int_fast16_t;
typedef int32_t         int_fast32_t;
typedef uint8_t         uint_fast8_t;
typedef uint16_t        uint_fast16_t;
typedef uint32_t        uint_fast32_t;

/* --- Pointer-sized type --- */
typedef int             intptr_t;
typedef unsigned int    uintptr_t;

/* --- Maximum-width type --- */
typedef int32_t         intmax_t;
typedef uint32_t        uintmax_t;

/* --- Value limits --- */
#define INT8_MIN        -128
#define INT8_MAX        127
#define UINT8_MAX       255

#define INT16_MIN       -32768
#define INT16_MAX       32767
#define UINT16_MAX      65535

/* INT32 limits as hex to avoid parser issues with large decimal constants */
#define INT32_MIN       0x80000000
#define INT32_MAX       0x7FFFFFFF
#define UINT32_MAX      0xFFFFFFFF

#define INTPTR_MIN      INT16_MIN
#define INTPTR_MAX      INT16_MAX
#define UINTPTR_MAX     UINT16_MAX

#define INTMAX_MIN      INT32_MIN
#define INTMAX_MAX      INT32_MAX
#define UINTMAX_MAX     UINT32_MAX

#endif

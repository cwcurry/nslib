/* cpl_bswap.h -- Byte swapping and endian conversion macros.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.


   If the size-independent macros below are called with constants, then they
   should be cast to the appropriate size so the return type can be determined.

   CPL_BSWAP (x) - Perform byte order swapping for uint16_t, uint32_t, or uint64_t
   CPL_BSWAPF (x) - Perform byte order swapping for float or double

   CPL_HTOLE (x) - Host to little endian conversion for uint16_t, uint32_t, or uint64_t
   CPL_HTOBE (x) - Host to big endian conversion for uint16_t, uint32_t, or uint64_t
   CPL_LETOH (x) - Little endian to host conversion for uint16_t, uint32_t, or uint64_t
   CPL_BETOH (x) - Big endian to host conversion for uint16_t, uint32_t, or uint64_t
   CPL_HTOLEF (x) - Host to little endian conversion for float or double
   CPL_HTOBEF (x) - Host to big endian conversion for float or double
   CPL_LETOHF (x) - Little endian to host conversion for float or double
   CPL_BETOHF (x) - Big endian to host conversion for float or double

   CPL_BYTE_CAST (x) - Convert type x to byte array

   cpl_bhtole16 (x) - Convert byte-array from host to little endian uint16_t
   cpl_bhtole32 (x) - Convert byte-array from host to little endian uint32_t
   cpl_bhtole64 (x) - Convert byte-array from host to little endian uint64_t
   cpl_bhtoleff (x) - Convert byte-array from host to little endian float
   cpl_bhtoledd (x) - Convert byte-array from host to little endian double
   cpl_bhtobe16 (x) - Convert byte-array from host to big endian uint16_t
   cpl_bhtobe32 (x) - Convert byte-array from host to big endian uint32_t
   cpl_bhtobe64 (x) - Convert byte-array from host to big endian uint64_t
   cpl_bhtobeff (x) - Convert byte-array from host to big endian float
   cpl_bhtobedd (x) - Convert byte-array from host to big endian double
   cpl_bletoh16 (x) - Convert byte-array from little endian to host uint16_t
   cpl_bletoh32 (x) - Convert byte-array from little endian to host uint32_t
   cpl_bletoh64 (x) - Convert byte-array from little endian to host uint64_t
   cpl_bletohff (x) - Convert byte-array from little endian to host float
   cpl_bletohdd (x) - Convert byte-array from little endian to host double
   cpl_bbetoh16 (x) - Convert byte-array from big endian to host uint16_t
   cpl_bbetoh32 (x) - Convert byte-array from big endian to host uint32_t
   cpl_bbetoh64 (x) - Convert byte-array from big endian to host uint64_t
   cpl_bbetohff (x) - Convert byte-array from big endian to host float
   cpl_bbetohdd (x) - Convert byte-array from big endian to host double

   cpl_is_big_endian () - Return non-zero if big endian

   Some of these routines are macros and may evaluate their input more than once. */

#ifndef CPL_BSWAP_H
#define CPL_BSWAP_H

#define CPL_NEED_FIXED_WIDTH_T /* Tell cpl_spec.h we need fixed-width types. */
#include "cpl_spec.h"

CPL_CLINKAGE_START

/* Define HAVE_NONALIGNED_MEMORY_ACCESS for x86-compatible CPU. */
#ifndef HAVE_NONALIGNED_MEMORY_ACCESS
# if (defined __i386__ || defined __i486__ || defined __pentium__ ||          \
    defined __pentiumpro__ || defined __pentium4__ || defined __k8__ ||       \
    defined __athlon__ || defined __k6__ || defined __core2__ ||              \
    defined __x86_64__)
#  define HAVE_NONALIGNED_MEMORY_ACCESS
# endif
#endif

#ifdef _MSC_VER
# define BIG_ENDIAN      4321
# define LITTLE_ENDIAN   1234
# define BYTE_ORDER      LITTLE_ENDIAN
#elif !defined (BYTE_ORDER)
# include <sys/param.h> /* for BYTE_ORDER, LITTLE_ENDIAN, BIG_ENDIAN */
#endif


/* Platform-dependent byte order swapping builtins. */

#if CPL_GNUC_PREREQ (4, 8)
# define cpl_bswap16(x)  __builtin_bswap16 (x)
#elif CPL_MSC_PREREQ (13, 0)
# include <stdlib.h>
# include <intrin.h>
# define cpl_bswap16(x) _byteswap_ushort (x)
# pragma intrinsic (_byteswap_ushort)
#endif

#if CPL_GNUC_PREREQ (4, 3)
# define cpl_bswap32(x)  __builtin_bswap32 (x)
#elif CPL_MSC_PREREQ (13, 0)
# include <stdlib.h>
# include <intrin.h>
# define cpl_bswap32(x) _byteswap_ulong (x)
# pragma intrinsic (_byteswap_ulong)
#endif

#if CPL_GNUC_PREREQ (4, 3)
# define cpl_bswap64(x)  __builtin_bswap64 (x)
#elif CPL_MSC_PREREQ (13, 0)
# include <stdlib.h>
# include <intrin.h>
# define cpl_bswap64(x) _byteswap_uint64 (x)
# pragma intrinsic (_byteswap_uint64)
#endif


#ifndef cpl_bswap16
    static inline uint16_t cpl_bswap16 (uint16_t x) {
        return ((((x) >> 8) & 0x00FF) | (((x) & 0x00FF) << 8));
    }
#endif

#ifndef cpl_bswap32
    static inline uint32_t cpl_bswap32 (uint32_t x) {
        return ( (((x) & 0xFF000000) >> 24) | (((x) & 0x00FF0000) >>  8)      \
               | (((x) & 0x0000FF00) <<  8) | (((x) & 0x000000FF) << 24));
    }
#endif

#ifndef cpl_bswap64
    static inline uint64_t cpl_bswap64 (uint64_t x) {
        return ( (((x) & 0xFF00000000000000ull) >> 56)                        \
               | (((x) & 0x00FF000000000000ull) >> 40)                        \
               | (((x) & 0x0000FF0000000000ull) >> 24)                        \
               | (((x) & 0x000000FF00000000ull) >> 8)                         \
               | (((x) & 0x00000000FF000000ull) << 8)                         \
               | (((x) & 0x0000000000FF0000ull) << 24)                        \
               | (((x) & 0x000000000000FF00ull) << 40)                        \
               | (((x) & 0x00000000000000FFull) << 56));
    }
#endif


/*********************** Floating Point Byte Swapping ************************/

#if CPL_GNUC_PREREQ (2, 0)
# define cpl_bswap_f32(x)                                                     \
    ( CPL_GNUC_EXTENSION ({                                                   \
    union {                                                                   \
        uint32_t u32;                                                         \
        float    f32;                                                         \
    } __t2;                                                                   \
    __t2.f32 = (x);                                                           \
    __t2.u32 = cpl_bswap32 (__t2.u32);                                        \
    __t2.f32; }))
#else
static inline float cpl_bswap_f32 (float x) {
    union {
        uint32_t u32;
        float    f32;
    } u;
    u.f32 = x;
    u.u32 = cpl_bswap32 (u.u32);
    return u.f32;
}
#endif

#if CPL_GNUC_PREREQ (2, 0)
# define cpl_bswap_f64(x)                                                     \
    ( CPL_GNUC_EXTENSION ({                                                   \
    union {                                                                   \
        uint64_t u64;                                                         \
        double   f64;                                                         \
    } __t2;                                                                   \
    __t2.f64 = (x);                                                           \
    __t2.u64 = cpl_bswap64 (__t2.u64);                                        \
    __t2.f64; }))
#else
static inline double cpl_bswap_f64 (double x) {
    union {
        uint64_t u64;
        double   f64;
    } u;
    u.f64 = x;
    u.u64 = cpl_bswap64 (u.u64);
    return u.f64;
}
#endif


/********************** Size-independent Byte Swapping ***********************/

#define CPL_BSWAP(x)                                                          \
    (sizeof (x) == sizeof (uint8_t) ? (x) :                                   \
    (sizeof (x) == sizeof (uint16_t) ? cpl_bswap16 (x) :                      \
    (sizeof (x) == sizeof (uint32_t) ? cpl_bswap32 (x) :                      \
    (sizeof (x) == sizeof (uint64_t) ? cpl_bswap64 (x) : (0)))))

#define CPL_BSWAPF(x)                                                         \
    (sizeof (x) == sizeof (float)  ? cpl_bswap_f32 (x) :                      \
    (sizeof (x) == sizeof (double) ? cpl_bswap_f64 (x) : (0.0)))


/*********************** Endian Type Conversion Macros ***********************/

#if defined (BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
# define cpl_htole16(x) (x)
# define cpl_htole32(x) (x)
# define cpl_htole64(x) (x)
# define cpl_htoleff(x) (x)
# define cpl_htoledd(x) (x)
# define cpl_htobe16(x) cpl_bswap16 (x)
# define cpl_htobe32(x) cpl_bswap32 (x)
# define cpl_htobe64(x) cpl_bswap64 (x)
# define cpl_htobeff(x) cpl_bswap_f32 (x)
# define cpl_htobedd(x) cpl_bswap_f64 (x)
# define cpl_letoh16(x) (x)
# define cpl_letoh32(x) (x)
# define cpl_letoh64(x) (x)
# define cpl_letohff(x) (x)
# define cpl_letohdd(x) (x)
# define cpl_betoh16(x) cpl_bswap16 (x)
# define cpl_betoh32(x) cpl_bswap32 (x)
# define cpl_betoh64(x) cpl_bswap64 (x)
# define cpl_betohff(x) cpl_bswap_f32 (x)
# define cpl_betohdd(x) cpl_bswap_f64 (x)
#elif defined (BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
# define cpl_htole16(x) cpl_bswap16 (x)
# define cpl_htole32(x) cpl_bswap32 (x)
# define cpl_htole64(x) cpl_bswap64 (x)
# define cpl_htoleff(x) cpl_bswap_f32 (x)
# define cpl_htoledd(x) cpl_bswap_f64 (x)
# define cpl_htobe16(x) (x)
# define cpl_htobe32(x) (x)
# define cpl_htobe64(x) (x)
# define cpl_htobeff(x) (x)
# define cpl_htodedd(x) (x)
# define cpl_letoh16(x) cpl_bswap16 (x)
# define cpl_letoh32(x) cpl_bswap32 (x)
# define cpl_letoh64(x) cpl_bswap64 (x)
# define cpl_letohff(x) cpl_bswap_f32 (x)
# define cpl_letohdd(x) cpl_bswap_f64 (x)
# define cpl_betoh16(x) (x)
# define cpl_betoh32(x) (x)
# define cpl_betoh64(x) (x)
# define cpl_betohff(x) (x)
# define cpl_betohdd(x) (x)
#else
# error "BYTE_ORDER not defined"
#endif


/************ Size-independent Integer Endian Conversion Functions ***********/

#define CPL_HTOLE(x)                                                          \
    (sizeof (x) == 1 ? (x) :                                                  \
    (sizeof (x) == 2 ? cpl_htole16 (x) :                                      \
    (sizeof (x) == 4 ? cpl_htole32 (x) :                                      \
    (sizeof (x) == 8 ? cpl_htole64 (x) : (0)))))

#define CPL_HTOBE(x)                                                          \
    (sizeof (x) == 1 ? (x) :                                                  \
    (sizeof (x) == 2 ? cpl_htobe16 (x) :                                      \
    (sizeof (x) == 4 ? cpl_htobe32 (x) :                                      \
    (sizeof (x) == 8 ? cpl_htobe64 (x) : (0)))))

#define CPL_LETOH(x)                                                          \
    (sizeof (x) == 1 ? (x) :                                                  \
    (sizeof (x) == 2 ? cpl_letoh16 (x) :                                      \
    (sizeof (x) == 4 ? cpl_letoh32 (x) :                                      \
    (sizeof (x) == 8 ? cpl_letoh64 (x) : (0)))))

#define CPL_BETOH(x)                                                          \
    (sizeof (x) == 1 ? (x) :                                                  \
    (sizeof (x) == 2 ? cpl_betoh16 (x) :                                      \
    (sizeof (x) == 4 ? cpl_betoh32 (x) :                                      \
    (sizeof (x) == 8 ? cpl_betoh64 (x) : (0)))))


/******** Size-independent Floating Point Endian Conversion Functions ********/

/* IEEE754 float (32-bit) and IEEE754 double (64-bit) are assumed. */

#define CPL_HTOLEF(x)                                                         \
    (sizeof (x) == sizeof (float)  ? cpl_htoleff (x) :                        \
    (sizeof (x) == sizeof (double) ? cpl_htoledd (x) : (0)))

#define CPL_HTOBEF(x)                                                         \
    (sizeof (x) == sizeof (float)  ? cpl_htobeff (x) :                        \
    (sizeof (x) == sizeof (double) ? cpl_htobedd (x) : (0)))

#define CPL_LETOHF(x)                                                         \
    (sizeof (x) == sizeof (float)  ? cpl_letohff (x) :                        \
    (sizeof (x) == sizeof (double) ? cpl_letohdd (x) : (0)))

#define CPL_BETOHF(x)                                                         \
    (sizeof (x) == sizeof (float)  ? cpl_betohff (x) :                        \
    (sizeof (x) == sizeof (double) ? cpl_betohdd (x) : (0)))


/* Functions to encode and decode integer and float types to and from byte arrays. */

/* Convert variable to byte-array.  Since the pointer cast goes from
   a more restrictive alignment to a less restrictive alignment, there
   shouldn't be any memory alignment problems. */

#define CPL_BYTE_CAST(x) (CPL_PTR_CAST (uint8_t *, (&(x))))

/* Convert byte-array to variable type.  These functions use a union to
   guarantee the proper memory alignment of the type, unless nonaligned
   memory access is possible. */

#define cpl_bytes_to_u16_aligned(x) (*((uint16_t *) (x)))
#define cpl_bytes_to_u32_aligned(x) (*((uint32_t *) (x)))
#define cpl_bytes_to_u64_aligned(x) (*((uint64_t *) (x)))
#define cpl_bytes_to_f32_aligned(x) (*((float    *) (x)))
#define cpl_bytes_to_f64_aligned(x) (*((double   *) (x)))

#define cpl_u16_to_bytes_aligned(p, x)                                        \
    do {                                                                      \
        uint16_t __x = (x);                                                   \
        uint8_t *__p = (p);                                                   \
        *((uint16_t *) __p) = __x;                                            \
} while (0)

#define cpl_u32_to_bytes_aligned(p, x)                                        \
    do {                                                                      \
        uint32_t __x = (x);                                                   \
        uint8_t *__p = (p);                                                   \
        *((uint32_t *) __p) = __x;                                            \
} while (0)

#define cpl_u64_to_bytes_aligned(p, x)                                        \
    do {                                                                      \
        uint64_t __x = (x);                                                   \
        uint8_t *__p = (p);                                                   \
        *((uint64_t *) __p) = __x;                                            \
} while (0)

#define cpl_f32_to_bytes_aligned(p, x)                                        \
    do {                                                                      \
        float __x = (x);                                                      \
        uint8_t *__p = (p);                                                   \
        *((float *) __p) = __x;                                               \
} while (0)

#define cpl_f64_to_bytes_aligned(p, x)                                        \
    do {                                                                      \
        double __x = (x);                                                     \
        uint8_t *__p = (p);                                                   \
        *((double *) __p) = __x;                                              \
} while (0)

#ifdef HAVE_NONALIGNED_MEMORY_ACCESS

# define cpl_bytes_to_u16(x) cpl_bytes_to_u16_aligned (x)
# define cpl_bytes_to_u32(x) cpl_bytes_to_u32_aligned (x)
# define cpl_bytes_to_u64(x) cpl_bytes_to_u64_aligned (x)
# define cpl_bytes_to_f32(x) cpl_bytes_to_f32_aligned (x)
# define cpl_bytes_to_f64(x) cpl_bytes_to_f64_aligned (x)

# define cpl_bytes_to_u16_swap(x) cpl_bswap16 (bytes_to_u16 (x))
# define cpl_bytes_to_u32_swap(x) cpl_bswap32 (bytes_to_u32 (x))
# define cpl_bytes_to_u64_swap(x) cpl_bswap64 (bytes_to_u64 (x))
# define cpl_bytes_to_f32_swap(x) cpl_bswap_f32 (bytes_to_f32 (x))
# define cpl_bytes_to_f64_swap(x) cpl_bswap_f64 (bytes_to_f64 (x))

# define cpl_u16_to_bytes(p, x) cpl_u16_to_bytes_aligned (p, x)
# define cpl_u32_to_bytes(p, x) cpl_u32_to_bytes_aligned (p, x)
# define cpl_u64_to_bytes(p, x) cpl_u64_to_bytes_aligned (p, x)
# define cpl_f32_to_bytes(p, x) cpl_f32_to_bytes_aligned (p, x)
# define cpl_f64_to_bytes(p, x) cpl_f64_to_bytes_aligned (p, x)

#else

static inline void cpl_u16_to_bytes (uint8_t *p, const uint16_t x) {
    const uint8_t *c = CPL_BYTE_CAST (x);

    p[0] = c[0];
    p[1] = c[1];
}

static inline void cpl_u32_to_bytes (uint8_t *p, const uint32_t x) {
    const uint8_t *c = CPL_BYTE_CAST (x);

    p[0] = c[0];
    p[1] = c[1];
    p[2] = c[2];
    p[3] = c[3];
}

static inline void cpl_u64_to_bytes (uint8_t *p, const uint64_t x) {
    const uint8_t *c = CPL_BYTE_CAST (x);

    p[0] = c[0];
    p[1] = c[1];
    p[2] = c[2];
    p[3] = c[3];
    p[4] = c[4];
    p[5] = c[5];
    p[6] = c[6];
    p[7] = c[7];
}

static inline void cpl_f32_to_bytes (uint8_t *p, const float x) {
    const uint8_t *c = CPL_BYTE_CAST (x);

    p[0] = c[0];
    p[1] = c[1];
    p[2] = c[2];
    p[3] = c[3];
}

static inline void cpl_f64_to_bytes (uint8_t *p, const double x) {
    const uint8_t *c = CPL_BYTE_CAST (x);

    p[0] = c[0];
    p[1] = c[1];
    p[2] = c[2];
    p[3] = c[3];
    p[4] = c[4];
    p[5] = c[5];
    p[6] = c[6];
    p[7] = c[7];
}

static inline uint16_t cpl_bytes_to_u16 (const uint8_t *x) {
    union {
        uint16_t u;
        uint8_t c[2];
    }  r;
    r.c[0] = x[0];
    r.c[1] = x[1];
    return r.u;
}

static inline uint32_t cpl_bytes_to_u32 (const uint8_t *x) {
    union {
        uint32_t u;
        uint8_t c[4];
    } r;
    r.c[0] = x[0];
    r.c[1] = x[1];
    r.c[2] = x[2];
    r.c[3] = x[3];
    return r.u;
}

static inline uint64_t cpl_bytes_to_u64 (const uint8_t *x) {
    union {
        uint64_t u;
        uint8_t c[8];
    } r;
    r.c[0] = x[0];
    r.c[1] = x[1];
    r.c[2] = x[2];
    r.c[3] = x[3];
    r.c[4] = x[4];
    r.c[5] = x[5];
    r.c[6] = x[6];
    r.c[7] = x[7];
    return r.u;
}

static inline float cpl_bytes_to_f32 (const uint8_t *x) {
    union {
        float u;
        uint8_t c[4];
    } r;
    r.c[0] = x[0];
    r.c[1] = x[1];
    r.c[2] = x[2];
    r.c[3] = x[3];
    return r.u;
}

static inline double cpl_bytes_to_f64 (const uint8_t *x) {
    union {
        double u;
        uint8_t c[8];
    } r;
    r.c[0] = x[0];
    r.c[1] = x[1];
    r.c[2] = x[2];
    r.c[3] = x[3];
    r.c[4] = x[4];
    r.c[5] = x[5];
    r.c[6] = x[6];
    r.c[7] = x[7];
    return r.u;
}

static inline uint16_t cpl_bytes_to_u16_swap (const uint8_t *x) {
    union {
        uint16_t u;
        uint8_t c[2];
    }  r;
    r.c[0] = x[1];
    r.c[1] = x[0];
    return r.u;
}

static inline uint32_t cpl_bytes_to_u32_swap (const uint8_t *x) {
    union {
        uint32_t u;
        uint8_t c[4];
    } r;
    r.c[0] = x[3];
    r.c[1] = x[2];
    r.c[2] = x[1];
    r.c[3] = x[0];
    return r.u;
}

static inline uint64_t cpl_bytes_to_u64_swap (const uint8_t *x) {
    union {
        uint64_t u;
        uint8_t c[8];
    } r;
    r.c[0] = x[7];
    r.c[1] = x[6];
    r.c[2] = x[5];
    r.c[3] = x[4];
    r.c[4] = x[3];
    r.c[5] = x[2];
    r.c[6] = x[1];
    r.c[7] = x[0];
    return r.u;
}

static inline float cpl_bytes_to_f32_swap (const uint8_t *x) {
    union {
        float u;
        uint8_t c[4];
    } r;
    r.c[0] = x[3];
    r.c[1] = x[2];
    r.c[2] = x[1];
    r.c[3] = x[0];
    return r.u;
}

static inline double cpl_bytes_to_f64_swap (const uint8_t *x) {
    union {
        double u;
        uint8_t c[8];
    } r;
    r.c[0] = x[7];
    r.c[1] = x[6];
    r.c[2] = x[5];
    r.c[3] = x[4];
    r.c[4] = x[3];
    r.c[5] = x[2];
    r.c[6] = x[1];
    r.c[7] = x[0];
    return r.u;
}
#endif /* !NONALIGNED_MEMORY_ACCESS */


/******************* Endian Byte-Array Conversion Macros *********************/

#if defined (BYTE_ORDER) && (BYTE_ORDER == LITTLE_ENDIAN)
# define cpl_bhtole16(x) cpl_bytes_to_u16 (x)
# define cpl_bhtole32(x) cpl_bytes_to_u32 (x)
# define cpl_bhtole64(x) cpl_bytes_to_u64 (x)
# define cpl_bhtoleff(x) cpl_bytes_to_f32 (x)
# define cpl_bhtoledd(x) cpl_bytes_to_f64 (x)
# define cpl_bhtobe16(x) cpl_bytes_to_u16_swap (x)
# define cpl_bhtobe32(x) cpl_bytes_to_u32_swap (x)
# define cpl_bhtobe64(x) cpl_bytes_to_u64_swap (x)
# define cpl_bhtobeff(x) cpl_bytes_to_f32_swap (x)
# define cpl_bhtobedd(x) cpl_bytes_to_f64_swap (x)
# define cpl_bletoh16(x) cpl_bytes_to_u16 (x)
# define cpl_bletoh32(x) cpl_bytes_to_u32 (x)
# define cpl_bletoh64(x) cpl_bytes_to_u64 (x)
# define cpl_bletohff(x) cpl_bytes_to_f32 (x)
# define cpl_bletohdd(x) cpl_bytes_to_f64 (x)
# define cpl_bbetoh16(x) cpl_bytes_to_u16_swap (x)
# define cpl_bbetoh32(x) cpl_bytes_to_u32_swap (x)
# define cpl_bbetoh64(x) cpl_bytes_to_u64_swap (x)
# define cpl_bbetohff(x) cpl_bytes_to_f32_swap (x)
# define cpl_bbetohdd(x) cpl_bytes_to_f64_swap (x)
#elif defined (BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
# define cpl_bhtole16(x) cpl_bytes_to_u16_swap (x)
# define cpl_bhtole32(x) cpl_bytes_to_u32_swap (x)
# define cpl_bhtole64(x) cpl_bytes_to_u64_swap (x)
# define cpl_bhtoleff(x) cpl_bytes_to_f32_swap (x)
# define cpl_bhtoledd(x) cpl_bytes_to_f64_swap (x)
# define cpl_bhtobe16(x) cpl_bytes_to_u16 (x)
# define cpl_bhtobe32(x) cpl_bytes_to_u32 (x)
# define cpl_bhtobe64(x) cpl_bytes_to_u64 (x)
# define cpl_bhtobeff(x) cpl_bytes_to_f32 (x)
# define cpl_bhtodedd(x) cpl_bytes_to_f64 (x)
# define cpl_bletoh16(x) cpl_bytes_to_u16_swap (x)
# define cpl_bletoh32(x) cpl_bytes_to_u32_swap (x)
# define cpl_bletoh64(x) cpl_bytes_to_u64_swap (x)
# define cpl_bletohff(x) cpl_bytes_to_f32_swap (x)
# define cpl_bletohdd(x) cpl_bytes_to_f64_swap (x)
# define cpl_bbetoh16(x) cpl_bytes_to_u16 (x)
# define cpl_bbetoh32(x) cpl_bytes_to_u32 (x)
# define cpl_bbetoh64(x) cpl_bytes_to_u64 (x)
# define cpl_bbetohff(x) cpl_bytes_to_f32 (x)
# define cpl_bbetohdd(x) cpl_bytes_to_f64 (x)
#else
# error "BYTE_ORDER not defined"
#endif


/* Compile-time test for endianess. */
#if defined (BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
# define CPL_IS_BIG_ENDIAN 1
#else
# define CPL_IS_BIG_ENDIAN 0
#endif


int cpl_is_big_endian (void) CPL_ATTRIBUTE_PURE;


CPL_CLINKAGE_END

#endif /* CPL_BSWAP_H */

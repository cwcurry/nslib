/* cpl_spec.h -- Portable function and variable attribute support.

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
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA. */

/* Provide definition of fixed-width integer types if requested. */
#ifdef CPL_NEED_FIXED_WIDTH_T
/* Define types for fixed-width integers (C99). */
# if defined (_MSC_VER) && (_MSC_VER <= 1500) /* Fix for MSVC */
typedef signed __int8  int8_t;   /* Need signed keyword here. */
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
# else
#  include <stdint.h>
# endif
#endif


#ifndef CPL_SPEC_H
#define CPL_SPEC_H

#ifdef __FILE__
#define CPL_FILE  __FILE__
#else
#define CPL_FILE  ""
#endif

#ifdef __LINE__
#define CPL_LINE  __LINE__
#else
#define CPL_LINE  -1
#endif

/* Macro to specify Windows API. */
#if (defined (_WIN32) || defined (__WIN32__)) && ! defined (__CYGWIN__)
# define CPL_WIN32_API  1
#endif

/* Macro to convert a macro argument into a string constant. */
#define CPL_STRINGIFY(s) #s

/* Macro to convert the result of the expansion of a macro argument into a string constant. */
#define CPL_STRINGIFY_EX(s) CPL_STRINGIFY(s)

/* Macro to concatenate string constants. */
#define CPL_CONCAT(s1, s2) s1 ## s2

/* Test for FILE type. */
#if defined (FILE) || defined (H_STDIO) || defined (_H_STDIO) || defined (STDIO_H) || defined (STDIO_H_) || defined (__STDIO_H) || defined (__STDIO_H__) \
 || defined (_STDIO_INCLUDED) || defined (__dj_include_stdio_h_) || defined (_FILE_DEFINED) || defined (__STDIO__) || defined (_MSL_STDIO_H)             \
 || defined (_STDIO_H_INCLUDED) || defined (_ISO_STDIO_ISO_H) || defined (__STDIO_LOADED) || defined (__DEFINED_FILE)
# define CPL_HAVE_FILE 1
#endif

/* Macros for C Linkage specification. */
#ifdef __cplusplus
# define CPL_CLINKAGE_START  extern "C" {
# define CPL_CLINKAGE_END    }
#else
# define CPL_CLINKAGE_START  /* empty */
# define CPL_CLINKAGE_END    /* empty */
#endif

/* Test for GCC >= maj.min. */
#if defined (__GNUC__) && defined (__GNUC_MINOR__)
# define CPL_GNUC_PREREQ(major, minor) (__GNUC__ > (major) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
# define CPL_GNUC_PREREQ(major, minor)  0
#endif

/* Test for MSC >= maj.min. */
#if defined (_MSC_VER)
# define CPL_MSC_PREREQ(major, minor) (_MSC_VER >= ((major) * 100 + (minor) * 10))
#else
# define CPL_MSC_PREREQ(major, minor)  0
#endif

/* Add pragma support. */
#if (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)) || CPL_GNUC_PREREQ (3, 0) || defined (__clang__)
# define CPL_PRAGMA(s)  _Pragma(s)
#elif CPL_MSC_PREREQ (15, 0)
# define CPL_PRAGMA(s) __pragma(s)
#else
# define CPL_PRAGMA(s)  /* empty */
#endif

/* Add GNU attribute support. */
#ifndef __attribute__
# if defined (__STRICT_ANSI__) || !CPL_GNUC_PREREQ (2, 5)
#  define __attribute__(spec)  /* empty */
# endif
#endif

/* Use __extension__ to suppress -pedantic warnings about GCC extensions. */
#if !CPL_GNUC_PREREQ (2, 8)
# define CPL_GNUC_EXTENSION __extension__
#else
# define CPL_GNUC_EXTENSION  /* empty */
#endif

/* Tell the compiler to use the smallest alignment or space for the struct, union, or variable.
   This may be used with __gcc_struct__ on Windows platforms. */
#if CPL_GNUC_PREREQ (2, 7)
# define CPL_ATTRIBUTE_PACKED __attribute__ ((__packed__))
#else
# define CPL_ATTRIBUTE_PACKED  /* empty */
#endif

/* On Windows platforms tell the compiler to pack the struct differently from the Microsoft ABI,
   which may add extra padding.  Same as -mno-ms-bitfields. */
#if CPL_GNUC_PREREQ (3, 4) && !defined (__clang__)
# define CPL_ATTRIBUTE_GCC_STRUCT __attribute__ ((__gcc_struct__))
#else
# define CPL_ATTRIBUTE_GCC_STRUCT  /* empty */
#endif

/* Tell the compiler that the nth function parameter should be a non-NULL pointer.
   This allows some optimization and a warning if a NULL pointer is used if -Wnonnull is enabled. */
#if CPL_GNUC_PREREQ (3, 3)
# define CPL_ATTRIBUTE_NONNULL(n)  __attribute__ ((__nonnull__ (n)))
#else
# define CPL_ATTRIBUTE_NONNULL(n)  /* empty */
#endif

/* Tell the compiler that the nth and mth function parameters should be non-NULL pointers. */
#if CPL_GNUC_PREREQ (3, 3)
# define CPL_ATTRIBUTE_NONNULL2(n, m)  __attribute__ ((__nonnull__ (n, m)))
#else
# define CPL_ATTRIBUTE_NONNULL2(n, m)  /* empty */
#endif

/* Tell the compiler that the nth, mth, and pth function parameters should be non-NULL pointers. */
#if CPL_GNUC_PREREQ (3, 3)
# define CPL_ATTRIBUTE_NONNULL3(n, m, p)  __attribute__ ((__nonnull__ (n, m, p)))
#else
# define CPL_ATTRIBUTE_NONNULL3(n, m, p)  /* empty */
#endif

/* Tell the compiler that all function parameters should be non-NULL pointers.
   This should be used instead of the previous macros to avoid compiler warnings
   about empty macro arguments. */
#if CPL_GNUC_PREREQ (3, 3)
# define CPL_ATTRIBUTE_NONNULL_ALL  __attribute__ ((__nonnull__))
#else
# define CPL_ATTRIBUTE_NONNULL_ALL  /* empty */
#endif

/* Tell the compiler that the function return value should be a non-NULL pointer. */
#if CPL_GNUC_PREREQ (4, 9)
# define CPL_ATTRIBUTE_RETURNS_NONNULL  __attribute__ ((__returns_nonnull__))
#else
# define CPL_ATTRIBUTE_RETURNS_NONNULL  /* empty */
#endif

/* Tell the compiler that the last argument of the function should be NULL. */
#if CPL_GNUC_PREREQ (3, 5)
# define CPL_ATTRIBUTE_SENTINEL  __attribute__ ((__sentinel__))
#else
# define CPL_ATTRIBUTE_SENTINEL  /* empty */
#endif

/* Tell the compiler that the function, type, or variable is possibly unused and not to issue a warning. */
#if CPL_GNUC_PREREQ (3, 4)
# define CPL_ATTRIBUTE_UNUSED  __attribute__ ((__unused__))
#else
# define CPL_ATTRIBUTE_UNUSED  /* empty */
#endif

/* Tell the compiler to inline the function even if no optimization level was specified. */
#if CPL_GNUC_PREREQ (3, 1)
# define CPL_ATTRIBUTE_ALWAYS_INLINE  __attribute__ ((__always_inline__))
#else
# define CPL_ATTRIBUTE_ALWAYS_INLINE  /* empty */
#endif

/* Tell the compiler not to inline the function even if no optimization level was specified. */
#if CPL_GNUC_PREREQ (3, 1)
# define CPL_ATTRIBUTE_NOINLINE  __attribute__ ((__noinline__))
#else
# define CPL_ATTRIBUTE_NOINLINE  /* empty */
#endif

/* Issue a warning if the caller of the function does not use its return value. */
#if CPL_GNUC_PREREQ (3, 4)
# define CPL_ATTRIBUTE_WARN_UNUSED_RESULT  __attribute__ ((__warn_unused_result__))
#else
# define CPL_ATTRIBUTE_WARN_UNUSED_RESULT  /* empty */
#endif

/* "pure" means that the function has no effects except the return value, which
   depends only on the function parameters and/or global variables.  We add the additional
   restriction to this definition that if the function has no effect except the return
   value, then the return value should not be left unused. */
#if CPL_GNUC_PREREQ (2, 96)
# define CPL_ATTRIBUTE_PURE  __attribute__ ((__pure__)) CPL_ATTRIBUTE_WARN_UNUSED_RESULT
#else
# define CPL_ATTRIBUTE_PURE  /* empty */
#endif

/* "const" means a function does nothing but examine its arguments and gives a return
   value, it doesn't read or write any memory (neither global nor pointed to by arguments),
   and has no other side-effects.  This is more restrictive than "pure".  We add the additional
   restriction to this definition that if the function has no effect except the return
   value, then the return value should not be left unused. */
#if CPL_GNUC_PREREQ (2, 5)
# define CPL_ATTRIBUTE_CONST  __attribute__ ((__const__)) CPL_ATTRIBUTE_WARN_UNUSED_RESULT
#else
# define CPL_ATTRIBUTE_CONST  /* empty */
#endif

/* Tell the compiler that the return value of the function points to memory,
   where the size is given by one of the function parameters. */
#if CPL_GNUC_PREREQ (4, 3)
# define CPL_ATTRIBUTE_ALLOC_SIZE(n)  __attribute__ ((__alloc_size__ (n)))
#else
# define CPL_ATTRIBUTE_ALLOC_SIZE(n)  /* empty */
#endif

/* Tell the compiler that the return value of the function points to memory,
   where the size is given by the product of two function parameters. */
#if CPL_GNUC_PREREQ (4, 3)
# define CPL_ATTRIBUTE_ALLOC_SIZE2(n, elsize)  __attribute__ ((__alloc_size__ (n, elsize)))
#else
# define CPL_ATTRIBUTE_ALLOC_SIZE2(n, elsize)  /* empty */
#endif

/* Tell the compiler that any non-NULL pointer returned by the function cannot
   alias any other pointer when the function returns and the contents of the memory
   cannot contain a pointer that aliases any other pointer valid when the function
   returns.  realloc-like functions do not have this property as the memory pointed
   to can contain pointers that alias already valid pointers. */
#if CPL_GNUC_PREREQ (2, 96)
# define CPL_ATTRIBUTE_MALLOC  __attribute__ ((__malloc__))
#else
# define CPL_ATTRIBUTE_MALLOC  /* empty */
#endif

/* Tell the compiler that the function is to be optimized more aggressively and
   grouped together to improve locality. */
#if CPL_GNUC_PREREQ (4, 3)
# define CPL_ATTRIBUTE_HOT  __attribute__ ((__hot__))
#else
# define CPL_ATTRIBUTE_HOT  /* empty */
#endif

/* Tell the compiler that the function is unlikely to execute and to optimize
   for size, group functions together to improve locality, and optimize branch
   predictions. */
#if CPL_GNUC_PREREQ (4, 3)
# define CPL_ATTRIBUTE_COLD  __attribute__ ((__cold__))
#else
# define CPL_ATTRIBUTE_COLD  /* empty */
#endif

/* Tell the compiler that the function is deprecated and will result in a warning if
 * the function is used anywhere in the source file. */
#if CPL_GNUC_PREREQ (3, 2)
# define CPL_ATTRIBUTE_DEPRECATED  __attribute__ ((__deprecated__))
#else
# define CPL_ATTRIBUTE_DEPRECATED  /* empty */
#endif

/* Tell the compiler to assume that the function does not return. */
#if CPL_GNUC_PREREQ (2, 7)
# define CPL_ATTRIBUTE_NORETURN  __attribute__ ((__noreturn__))
#else
# define CPL_ATTRIBUTE_NORETURN  /* empty */
#endif

/* Tell the compiler that the function takes printf-style arguments which should
   be type-checked against a format string.  m is the number of the "format string"
   parameter and n is the number of the first variadic parameter. */
#if CPL_GNUC_PREREQ (2, 96)
# define CPL_ATTRIBUTE_PRINTF(m, n)  __attribute__ ((__format__ (__printf__, m, n))) CPL_ATTRIBUTE_NONNULL (m)
#else
# define CPL_ATTRIBUTE_PRINTF(m, n)  /* empty */
#endif

/* Tell the compiler that the case statement explicitly falls through in a switch statement
   and to indicate that the -Wimplicit-fallthrough warning should not be emitted. */
#if CPL_GNUC_PREREQ (7, 0)
# define CPL_ATTRIBUTE_FALLTHROUGH  __attribute__ ((fallthrough))
#else
# define CPL_ATTRIBUTE_FALLTHROUGH  /* empty */
#endif

/* The type attribute aligned specifies the minimum alignment in bytes for
   the variables of the given type. */
#if CPL_GNUC_PREREQ (2, 7)
# define CPL_ATTRIBUTE_ALIGNED(x)  __attribute__ ((__aligned__ (x)))
#else
# define CPL_ATTRIBUTE_ALIGNED(x)  /* empty */
#endif

/* Allows the compiler to assume that the pointer PTR is at least N-bytes aligned. */
#if CPL_GNUC_PREREQ (4, 7)
# define CPL_ASSUME_ALIGNED(ptr, n) __builtin_assume_aligned (ptr, n)
#else
# define CPL_ASSUME_ALIGNED(ptr, n) (ptr)
#endif

/* Tell the compiler that the referenced argument must be an open file descriptor. */
#if defined __has_attribute
# if __has_attribute (fd_arg)
#  define CPL_ATTRIBUTE_FD_ARG __attribute__ ((fd_arg))
# endif
#endif

/* Tell the compiler that if referenced argument is non-null, then it is a C-style null-terminated string. */
#if defined __has_attribute
# if __has_attribute (null_terminated_string_arg)
#  define CPL_ATTRIBUTE_NULL_TERMINATED_STRING_ARG(n) __attribute__ ((null_terminated_string_arg (n)))
# endif
#endif

/* An empty "throw ()" means the function doesn't throw any C++ exceptions,
   which can save some stack frame info in applications. */

/* Tell the compiler that the function does not throw an exception. */
#ifdef __GNUC__
# if !defined (__cplusplus) && CPL_GNUC_PREREQ (3, 3)
#  define CPL_ATTRIBUTE_NOTHROW  __attribute__ ((__nothrow__))
# else
#  if defined (__cplusplus) && CPL_GNUC_PREREQ (2, 8)
#   define CPL_ATTRIBUTE_NOTHROW throw ()
#  else
#   define CPL_ATTRIBUTE_NOTHROW  /* empty */
#  endif
# endif
#else
# define CPL_ATTRIBUTE_NOTHROW    /* empty */
#endif

/* Use __builtin_constant_p to determine if a value is known to be a constant at compile-time. */
#if !CPL_GNUC_PREREQ (2, 0)
# define __builtin_constant_p(x) 0
#endif

/* Give the compiler branch prediction information. */
#if CPL_GNUC_PREREQ (3, 0)
# define CPL_LIKELY(cond)   __builtin_expect ((cond) != 0, 1)
# define CPL_UNLIKELY(cond) __builtin_expect ((cond) != 0, 0)
#else
# define CPL_LIKELY(cond)   (cond)
# define CPL_UNLIKELY(cond) (cond)
#endif

/* Tell the compiler that for the life of the pointer, only it or a value derived
   from it will be used to access the object to which it points. */
#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) && !defined (__cplusplus)
# define CPL_RESTRICT restrict
#elif CPL_GNUC_PREREQ (3, 1)
# define CPL_RESTRICT __restrict__
#elif CPL_MSC_PREREQ (14, 0)
# define CPL_RESTRICT __restrict
#else
# define CPL_RESTRICT  /* empty */
#endif

/* CPL_PTR_CAST allows use of static_cast in C/C++ headers, so macros are clean to "g++ -Wold-style-cast". */
#ifdef __cplusplus
#define CPL_PTR_CAST(T, expr)  (static_cast<T> (expr))
#else
#define CPL_PTR_CAST(T, expr)  ((T) (expr))
#endif

/* Macro to find the enclosing structure from a pointer to a nested element. */
#if CPL_GNUC_PREREQ (2, 0)
# define CPL_CONTAINER_OF(ptr, type, member)              \
    CPL_GNUC_EXTENSION ({                                 \
    const typeof (((type *)0)->member) *__mptr = (ptr);   \
    (type *) ((char *) __mptr - offsetof (type, member)); \
})
#else
# define CPL_CONTAINER_OF(ptr, type, member)              \
    ((type *) ((char *) (ptr) - offsetof (type, member)))
#endif  /* __GNUC__ 2.0 */

/* Macro to find the number of elements in a static array. */
#define CPL_NUMBEROF(T) (sizeof (T) / sizeof (*T))

#endif /* CPL_SPEC_H */

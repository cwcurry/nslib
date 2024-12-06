/* cpl_error.h -- Header file for cpl_error.c

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

#ifndef CPL_ERROR_H
#define CPL_ERROR_H

#if defined (__cplusplus)
#include <cstdarg>
#else
#include <stdarg.h>
#endif

#include "cpl_spec.h"


/* Error Conditions */
#define CS_ENONE      0
#define CS_EINVAL    -1
#define CS_ENOMEM    -2
#define CS_EOPEN     -3
#define CS_ECLOSE    -4
#define CS_EREAD     -5
#define CS_EWRITE    -6
#define CS_ESEEK     -7
#define CS_EDOM      -8
#define CS_EFAIL     -9
#define CS_ENOPROG   -10
#define CS_EMAXITER  -11
#define CS_ESING     -12
#define CS_EBADFN    -13
#define CS_EBADDATA  -14
#define CS_EUNSUP    -15
#define CS_EOVERFLOW -16


#define CPL_ERROR(...)                               \
    do {                                             \
        cpl_error (CPL_FILE, CPL_LINE, __VA_ARGS__); \
    } while (0)


/* Type of function to handle errors. */
typedef void (* cpl_error_handler) (const char *file, const int line, const char *format, va_list ap);


/******************************* API Functions *******************************/

CPL_CLINKAGE_START

void cpl_error (const char *, const int, const char *, ...) CPL_ATTRIBUTE_NORETURN CPL_ATTRIBUTE_PRINTF (3, 4);
cpl_error_handler cpl_set_error_handler (cpl_error_handler);
const char * cpl_error_get_string (const int) CPL_ATTRIBUTE_WARN_UNUSED_RESULT CPL_ATTRIBUTE_RETURNS_NONNULL;
const char * cpl_error_get_name (const int) CPL_ATTRIBUTE_WARN_UNUSED_RESULT CPL_ATTRIBUTE_RETURNS_NONNULL;


CPL_CLINKAGE_END

#endif /* CPL_ERROR_H */

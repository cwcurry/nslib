/* cpl_debug.h -- Header file for cpl_debug.c

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

#ifndef CPL_DEBUG_H
#define CPL_DEBUG_H


#define CPL_DEBUG_ENABLE  1

#ifdef NDEBUG  /* If NDEBUG is defined, then remove debug print functions. */
# define cpl_debug(debug_enable, ...) do { /* empty */ } while (0)
#else

# if defined (__cplusplus)
#  include <cstdio>
# else
#  include <stdio.h>
# endif

# include "cpl_str.h"
# include "cpl_spec.h"

# define cpl_debug(debug_enable, ...) do {                                                         \
    if (debug_enable) {                                                                            \
        fprintf (stderr, "DEBUG: %s(%d): %s: ", cpl_str_basename (CPL_FILE), CPL_LINE, __func__);  \
        fprintf (stderr, __VA_ARGS__);                                                             \
        fflush (stderr);                                                                           \
    }                                                                                              \
} while (0)
#endif

#endif /* CPL_DEBUG_H */

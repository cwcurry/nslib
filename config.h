/* config.h -- Configuration.

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

#ifndef CONFIG_H
#define CONFIG_H

/* Define if you have the 'isfinite' function. */
#define HAVE_ISFINITE 1

/* Define if you have the 'finite' function. */
/* #undef HAVE_FINITE */

/* Define if you have the 'fseeko' function. */
#define HAVE_FSEEKO 1

/* Define if you have the 'realpath' function. */
/* #undef HAVE_REALPATH */

/* Define if you have the 'clock_gettime' function. */
#define HAVE_CLOCK_GETTIME 1

/* Define if you have the 'gmtime_r' function. */
/* #undef HAVE_GMTIME_R */

/* Define if you have the 'memrchr' function. */
/* #undef HAVE_MEMRCHR */

/* Define if you have the 'memccpy' function. */
#define HAVE_MEMCCPY 1

/* Define if you have the 'mempcpy' function. */
#define HAVE_MEMPCPY 1

/* Define if you have the 'strchrnul' function. */
/* #undef HAVE_STRCHRNUL */

/* Define if you have the 'strpbrk' function. */
#define HAVE_STRPBRK 1

/* Define if you have the 'strtok_r' function. */
#define HAVE_STRTOK_R 1

/* Define if you have the 'strlcpy' function. */
/* #undef HAVE_STRLCPY */

/* Define if you have the 'strlcat' function. */
/* #undef HAVE_STRLCAT */

/* Define if you have the 'stpcpy' function. */
/* #undef HAVE_STPCPY */

/* Define if you have the 'strnlen' function. */
#define HAVE_STRNLEN 1

/* Define if you have the 'strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define if you have the 'strncasecmp' function. */
#define HAVE_STRNCASECMP 1

/* Define if you have the 'hypot' function. */
#define HAVE_HYPOT 1

/* Define if you have the 'getpwuid' function. */
/* #undef HAVE_GETPWUID */

/* Define if memory can be accessed on non-aligned boundaries. */
#define HAVE_NONALIGNED_MEMORY_ACCESS 1

/* Define if you have the 'flockfile/funlockfile' functions. */
/* #undef HAVE_FLOCKFILE */

#endif /* CONFIG_H */

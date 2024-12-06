/* cpl_str.h -- Header file for cpl_str.c

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

#ifndef CPL_STR_H
#define CPL_STR_H

#if defined (__cplusplus)
#include <cstdio>
#else
#include <stdio.h>
#endif

#include "cpl_spec.h"


/* Macros for ASCII characters */
#define CPL_ISASCII(c) (!((c) & ~0x7F))
#define CPL_ISALPHA(c) ((((unsigned int) (unsigned char) (c)) | 32) - 'a' < 26)  /* SEI CERT STR37-C */
#define CPL_ISALNUM(c) (CPL_ISALPHA (c) || CPL_ISDIGIT (c))
#define CPL_ISBLANK(c) (((c) == ' ') || ((c) == '\t'))
#define CPL_ISSPACE(c) ((c == ' ') || (((unsigned int) (unsigned char) (c) - '\t') < 5))
#define CPL_ISCNTRL(c) ((((unsigned int) (unsigned char) (c)) < 0x20) || ((c) == 0x7F))
#define CPL_ISGRAPH(c) (((unsigned int) (unsigned char) (c) - 0x21) < 0x5E)
#define CPL_ISPRINT(c) (((unsigned int) (unsigned char) (c) - 0x20) <= 0x5E)
#define CPL_ISDIGIT(c) (((unsigned int) (unsigned char) (c) - '0') < 10)
#define CPL_ISXDIGIT(c) (CPL_ISDIGIT (c) || ((((unsigned int) (unsigned char) (c) | 32) - 'a') < 6))
#define CPL_ISUPPER(c) (((unsigned int) (unsigned char) (c) - 'A') < 26)
#define CPL_ISLOWER(c) (((unsigned int) (unsigned char) (c) - 'a') < 26)
#define CPL_TOUPPER(c) (CPL_ISLOWER (c) ? ((c) - 32) : c)
#define CPL_TOLOWER(c) (CPL_ISUPPER (c) ? ((c) + 32) : c)

/* Buffer size for cpl_itoa to hold a string large enough for a 64-bit signed value. */
#define CPL_ITOA_SIZE 21


/******************************* API Functions *******************************/

CPL_CLINKAGE_START

void * cpl_memdup (const void *, size_t) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
void * cpl_memrchr (const void *, const int, const size_t) CPL_ATTRIBUTE_NONNULL_ALL;
void * cpl_memccpy (void * CPL_RESTRICT, const void * CPL_RESTRICT, const int, const size_t) CPL_ATTRIBUTE_NONNULL_ALL;
void * cpl_mempcpy (void * CPL_RESTRICT, const void * CPL_RESTRICT, const size_t) CPL_ATTRIBUTE_NONNULL_ALL;
char * cpl_strdup (const char *) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
char * cpl_strndup (const char *, const size_t) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
char * cpl_strchrnul (const char *, const int) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_RETURNS_NONNULL;
char * cpl_str_copy (char * CPL_RESTRICT, const char * CPL_RESTRICT) CPL_ATTRIBUTE_NONNULL_ALL;
int cpl_str_replace (char *, const char, const char);
char * cpl_str_replace_str (const char *, const char *, const char *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT CPL_ATTRIBUTE_NONNULL (3);
char * cpl_strpbrk (const char *, const char *) CPL_ATTRIBUTE_PURE;
char * cpl_str_tolower (char *);
char * cpl_str_toupper (char *);
char * cpl_str_ltrim (const char *) CPL_ATTRIBUTE_PURE;
char * cpl_str_rtrim (char *);
char * cpl_str_strip (char *);
char * cpl_str_skip_arg (const char *) CPL_ATTRIBUTE_PURE;
int cpl_str_count_args (const char *) CPL_ATTRIBUTE_PURE;
int cpl_str_count_args_token (const char *, const int) CPL_ATTRIBUTE_PURE;
char * cpl_strtok1 (char **, const int) CPL_ATTRIBUTE_NONNULL_ALL;
char * cpl_strtok_r (char *, const char *, char **);
char * cpl_strcmp_tok (const char *, const char *, const int) CPL_ATTRIBUTE_PURE CPL_ATTRIBUTE_NONNULL (2);
char * cpl_strrstr (const char * CPL_RESTRICT, const char * CPL_RESTRICT) CPL_ATTRIBUTE_PURE;
char * cpl_str_has_prefix (const char *, const char *) CPL_ATTRIBUTE_NONNULL (2) CPL_ATTRIBUTE_PURE;
char * cpl_str_has_suffix (const char *, const char *) CPL_ATTRIBUTE_NONNULL (2) CPL_ATTRIBUTE_PURE;
char * cpl_str_dot (const char *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
char * cpl_str_basename (const char *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
char * cpl_str_escape (const char *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
void cpl_fputs_escaped (const char *, FILE *);
char * cpl_asprintf (const char *, ...) CPL_ATTRIBUTE_PRINTF (1, 2) CPL_ATTRIBUTE_WARN_UNUSED_RESULT CPL_ATTRIBUTE_NONNULL (1);
char * cpl_itoa (const long int, char *) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_PURE;
size_t cpl_strlcat (char *, const char *, const size_t) CPL_ATTRIBUTE_NONNULL_ALL;
size_t cpl_strlcpy (char * CPL_RESTRICT, const char * CPL_RESTRICT, const size_t) CPL_ATTRIBUTE_NONNULL_ALL;
size_t cpl_strnlen (const char *, const size_t) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_PURE;
int cpl_str_empty (const char *) CPL_ATTRIBUTE_PURE;
int cpl_strequal (const char *, const char *) CPL_ATTRIBUTE_PURE;
int cpl_strnequal (const char *, const char *, const size_t) CPL_ATTRIBUTE_PURE;
int cpl_strcaseequal (const char *, const char *) CPL_ATTRIBUTE_PURE;
int cpl_strncaseequal (const char *, const char *, const size_t) CPL_ATTRIBUTE_PURE;
int cpl_strcasecmp (const char *, const char *) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_PURE;
int cpl_strncasecmp (const char *, const char *, const size_t) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_PURE;
int cpl_strcasewscmp (const char *, const char *) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_PURE;
int cpl_strncasewscmp (const char *, const char *, const size_t) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_PURE;
int cpl_getsubopt (char **, const char * const *, char **) CPL_ATTRIBUTE_NONNULL2 (2, 3);
char * cpl_str_set_name_value (const char *, const char *, const int) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
char * cpl_str_get_name (const char *, const int) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
char * cpl_str_get_value (const char *, const char *, const int) CPL_ATTRIBUTE_PURE;
int cpl_str_to_bool (const char *) CPL_ATTRIBUTE_PURE;
int cpl_str_hex_to_uint (unsigned int *, const char *, char **) CPL_ATTRIBUTE_NONNULL (1);
int cpl_str_hex_to_ulong (unsigned long *, const char *, char **) CPL_ATTRIBUTE_NONNULL (1);
int cpl_str_to_int (int *, const char *, char **) CPL_ATTRIBUTE_NONNULL (1);
int cpl_str_to_ulong (unsigned long *, const char *, char **) CPL_ATTRIBUTE_NONNULL (1);
int cpl_str_to_double (double *, const char *, char **) CPL_ATTRIBUTE_NONNULL (1);
int cpl_strtod (double *, const char *, char **) CPL_ATTRIBUTE_NONNULL (1);
size_t cpl_str_hash (const char *) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_PURE;
char * cpl_str_cat (const char *, const char *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
char * cpl_strvcat (const char *, ...) CPL_ATTRIBUTE_SENTINEL CPL_ATTRIBUTE_WARN_UNUSED_RESULT CPL_ATTRIBUTE_NONNULL (1);
int cpl_getline (char **, size_t *, const size_t, FILE *) CPL_ATTRIBUTE_NONNULL_ALL;
int cpl_getdelim (char **, size_t *, const size_t, const int, FILE *) CPL_ATTRIBUTE_NONNULL_ALL;


CPL_CLINKAGE_END

#endif /* CPL_STR_H */

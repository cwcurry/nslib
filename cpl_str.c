/* cpl_str.c -- Implementation of portable string functions.

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


   This file provides useful string functions and replacements for missing library
   functions.  These string functions are written to allow propagation of NULL
   values and wrap memory allocating functions, such as strdup, with our own
   memory allocator. */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>
#include <errno.h>
#include <assert.h>
#include "cpl_str.h"
#include "cpl_alloc.h"
#include "cpl_error.h"
#include "cpl_debug.h"


/* Private Function Prototypes */
static int cpl_strtoul_1 (unsigned long *, const char *, char **, int *);


/* External Variables */
extern int cpl_lib_debug;


/******************************************************************************
*
* cpl_memdup - Copy an object P of size N bytes.  The newly-allocated object
*   must be free'd by the caller.
*
* Return: A pointer to the newly-allocated object, or
*         NULL if memory allocation failed.
*
******************************************************************************/

void * cpl_memdup (
    const void *p,
    size_t n) {

    void *ptr = cpl_malloc (n);


    assert (p);

    if (!ptr) return NULL;

    return memcpy (ptr, p, n);
}


/******************************************************************************
*
* cpl_memrchr - Find the last occurrence of C_IN in the buffer S of length N
*   and return a pointer to it or return NULL if no match was found.
*
******************************************************************************/

void * cpl_memrchr (
    const void *s,
    const int c_in,
    const size_t n) {

#ifdef HAVE_MEMRCHR
    assert (s);

    return memrchr (s, c_in, n);
#else
    const unsigned char *p = (unsigned char *) s;
    unsigned char c = (unsigned char) c_in;
    size_t m = n;


    assert (s);

    while (m--) {
        if (p[m] == c) return (void *) ((const char *) s + m);
    }

    return NULL;
#endif
}


/******************************************************************************
*
* cpl_memccpy - Copies bytes from the object pointed to by SRC to the object
*   pointed to by DST, stopping after either N bytes are copied or the character
*   C is found (and copied).
*
* Return: If C is found, then return a pointer to the next byte in DST after C, or
*         NULL if C was not found.
*
******************************************************************************/

void * cpl_memccpy (
    void * CPL_RESTRICT dst,
    const void * CPL_RESTRICT src,
    const int c,
    const size_t n) {

#ifdef HAVE_MEMCCPY
    assert (dst);
    assert (src);

    return memccpy (dst, src, c, n);
#else
    size_t m = n;


    assert (dst);
    assert (src);

    if (m > 0) {
        unsigned char *q = dst;
        const unsigned char *p = src;
        unsigned char uc = c;

        do {
            if ((*q++ == *p++) == uc) return q;
        } while (--m != 0);
    }

    return NULL;
#endif
}


/******************************************************************************
*
* cpl_mempcpy - Copy N bytes from object pointed to by SRC to the object pointed
*   to by DST.  Return a pointer to the byte following the last written byte.
*
* Return: A pointer to byte after the last written byte.
*
******************************************************************************/

void * cpl_mempcpy (
    void * CPL_RESTRICT dst,
    const void * CPL_RESTRICT src,
    const size_t n) {

    assert (dst);
    assert (src);

#ifdef HAVE_MEMPCPY
    return mempcpy (dst, src, n);
#else
    return (char *) memcpy (dst, src, n) + n;
#endif
}


/******************************************************************************
*
* cpl_strdup - Copy the string S into a newly-allocated string with space for S.
*   The newly-allocated string must be free'd by the caller.  This function is
*   preferred over the library implementation because it makes memory allocation
*   through our own memory allocator.
*
* Return: A pointer to the newly-allocated string, or
*         NULL if memory allocation failed.
*
******************************************************************************/

char * cpl_strdup (
    const char *s) {

    assert (s);

    return (char *) cpl_memdup (s, strlen (s) + 1);
}


/******************************************************************************
*
* cpl_strndup - Copies at most N characters of the string S plus one byte for a
*   terminating NUL into a newly-allocated string.  The newly-allocated string
*   must be free'd by the caller.  This function is preferred over the library
*   implementation because it makes memory allocation through our own memory
*   allocator.
*
* Return: A pointer to the newly-allocated string, or
*         NULL if memory allocation failed.
*
******************************************************************************/

char * cpl_strndup (
    const char *s,
    const size_t n) {

    size_t len;
    char *p;


    assert (s);

    len = cpl_strnlen (s, n);
    p = (char *) cpl_malloc (len + 1);
    if (!p) return NULL;

    p[len] = '\0';
    return (char *) memcpy (p, s, len);
}


/******************************************************************************
*
* cpl_strchrnul - Find the first occurrence of C in S or the final NUL byte
*   and return a pointer to it.
*
******************************************************************************/

char * cpl_strchrnul (
    const char *s,
    const int c_in) {

#ifdef HAVE_STRCHRNUL
    assert (s);
    return strchrnul (s, c_in);
#else
    char c = c_in;

    assert (s);
    while (*s && (*s != c)) s++;

    return (char *) s;
#endif
}


/******************************************************************************
*
* cpl_str_replace - Find each occurrence of the character FIND and replace it
*   with REPLACE.
*
* Return: The number of substitutions made.
*
******************************************************************************/

int cpl_str_replace (
    char *s,
    const char find,
    const char replace) {

    char *p = s;
    int n = 0;


    if (s) {
        while ((p = strchr (p, find))) {
            *p = replace;
            p++;
            n++;
        }
    }

    return n;
}


/******************************************************************************
*
* cpl_str_replace_str - Find each occurrence of the substring OLD and replace
*   it with NEW.
*
* Return: A newly-allocated string with the replaced substrings, or
*         NULL if memory allocation failed or the input string is NULL.
*
******************************************************************************/

char * cpl_str_replace_str (
    const char *s,
    const char *old,
    const char *new) {

    const char *p;
    char *result;
    char *sp;
    size_t len_old;
    size_t len_new;
    size_t c;


    assert (new);

    if (!s || !old) return NULL;

    len_old = strlen (old);
    if (len_old == 0) return NULL;

    len_new = strlen (new);

    p = s;

    /* Count the number of replacements. */
    for (c = 0; (sp = strstr (p, old)); c++) {
        p = sp + len_old;
    }

    /* Allocate space for the original string with all replacements. */
    sp = result = cpl_malloc ((strlen (s) + len_new * c + 1) - len_old * c);
    if (!result) return NULL;

    while (c--) {
        size_t n;
        p = strstr (s, old);
        n = p - s;
        sp = strncpy (sp, s, n) + n;
        sp = strcpy (sp, new) + len_new;
        s += n + len_old;
    }

    strcpy (sp, s);

    return result;
}


/******************************************************************************
*
* cpl_str_copy - Copy the string SRC to the destination DST including the nul
*   terminator and return a pointer to the trailing nul byte.
*
******************************************************************************/

char * cpl_str_copy (
    char * CPL_RESTRICT dst,
    const char * CPL_RESTRICT src) {

#ifdef HAVE_STPCPY
    assert (dst);
    assert (src);

    return stpcpy (dst, src);
#else
    const char *s = src;
    char *d = dst;


    assert (dst);
    assert (src);

    do {
        *d++ = *s;
    } while (*s++ != '\0');

    return d-1;
#endif
}


/******************************************************************************
*
* cpl_strpbrk - Find the first occurrence in S of any character in ACCEPT and
*   return a pointer to it or return NULL if not found.
*
******************************************************************************/

char * cpl_strpbrk (
    const char *s,
    const char *accept) {

    assert (s);
    assert (accept);

#ifdef HAVE_STRPBRK
    return strpbrk (s, accept);
#else
    while (*s) {
        const char *a = accept;
        while (*a) {
            if (*a++ == *s) return (char *) s;
        }
        s++;
    }

    return NULL;
#endif
}


/******************************************************************************
*
* cpl_str_tolower - Converts a string S to lower case by modifying it in-place.
*
* Return: A pointer to the processed string, or
*         NULL if the input string was NULL.
*
******************************************************************************/

char * cpl_str_tolower (
    char *s) {

    if (s) {
        char *q = s;

        while ((*q = CPL_TOLOWER (*q))) q++;
    }

    return s;
}


/******************************************************************************
*
* cpl_str_toupper - Converts a string S to upper case by modifying it in-place.
*
* Return: A pointer to the processed string, or
*         NULL if the input string was NULL.
*
******************************************************************************/

char * cpl_str_toupper (
    char *s) {

    if (s) {
        char *q = s;

        while ((*q = CPL_TOUPPER (*q))) q++;
    }

    return s;
}


/******************************************************************************
*
* cpl_str_ltrim - Remove any leading whitespace from a string S.
*
* Return: A pointer to the first non-whitespace character in the string, or
*         NULL if the input string is NULL.
*
******************************************************************************/

char * cpl_str_ltrim (
    const char *s) {

    if (s) {
        while (CPL_ISSPACE (*s)) s++;
    }

    return (char *) s;
}


/******************************************************************************
*
* cpl_str_rtrim - Remove any trailing whitespace from a string S by modifying
*   it in-place.
*
* Return: A pointer to the processed string, or
*         NULL if the input string was NULL.
*
******************************************************************************/

char * cpl_str_rtrim (
    char *s) {

    if (s) {
        size_t size = strlen (s);
        char *end = s + size - 1;

        while ((end >= s) && (CPL_ISSPACE (*end))) end--;
        *(end + 1) = '\0';
    }

    return s;
}


/******************************************************************************
*
* cpl_str_strip - Remove leading and trailing whitespace from a string S by
*   modifying it in-place.
*
* Return: A pointer to the processed string, or
*         NULL if the input string was NULL.
*
******************************************************************************/

char * cpl_str_strip (
    char *s) {

    if (s) {
        size_t size;
        char *end;

        while (CPL_ISSPACE (*s)) s++;
        if (*s == '\0') return s;

        size = strlen (s);
        end = s + size - 1;

        while ((end >= s) && (CPL_ISSPACE (*end))) end--;
        *(end + 1) = '\0';
    }

    return s;
}


/******************************************************************************
*
* cpl_str_skip_arg - Skip past any non-whitespace characters in a string S.
*
* Return: A pointer to the processed string, or
*         NULL if the input string was NULL.
*
******************************************************************************/

char * cpl_str_skip_arg (
    const char *s) {

    if (s) {
        while (*s && !CPL_ISSPACE (*s)) s++;
    }

    return (char *) s;
}


/******************************************************************************
*
* cpl_str_count_args - Return the number of arguments (split by whitespace) that
*   are contained within the string S or zero if S is NULL or contains only
*   whitespace.
*
******************************************************************************/

int cpl_str_count_args (
    const char *s) {

    int count = 0;


    if (s) {
        while (*s) {
            s = cpl_str_ltrim (s);
            if (*s) {
                count++;
                s = cpl_str_skip_arg (s);
            }
        }
    }

    return count;
}


/******************************************************************************
*
* cpl_str_count_args_token - Return the number of arguments (split by the token C)
*   that are contained within the string S or zero if S is NULL.
*
******************************************************************************/

int cpl_str_count_args_token (
    const char *s,
    const int c) {

    int count = 0;


    while (s) {
        s = strchr (s, c);
        if (s) s++;
        count++;
    }

    return count;
}


/******************************************************************************
*
* cpl_strtok1 - Tokenize the delimited message string pointed to by SPTR.  The
*   delimiter is given by C.  If the end of the string is reached, then a NULL
*   pointer is returned.  Successive delimiters will return an empty string.
*   This is the same as strsep() with only one delimiter.
*
* Return: A pointer to the token string, or
*         NULL if the input string is NULL.
*
******************************************************************************/

char * cpl_strtok1 (
    char **sptr,
    const int c) {

    char *tok;


    assert (sptr);
    assert (c != 0);

    if ((tok = *sptr)) {
        char *s = strchr (tok, c);

        if (s) *s++ = '\0';
        *sptr = s;
    }

    return tok;
}


/******************************************************************************
*
* cpl_strtok_r - Tokenize the delimited message string pointed to by S.  The
*   delimiters are given in DELIM.  If the end of the string is reached, then
*   a NULL pointer is returned.  If S is NULL, then the pointer LAST is used
*   as the next starting point.
*
******************************************************************************/

char * cpl_strtok_r (
    char *s,
    const char *delim,
    char **last) {

#ifdef HAVE_STRTOK_R
    return strtok_r (s, delim, last);
#else
    char *end;


    if (!s) s = *last;

    if (*s == '\0') {
        *last = s;
        return NULL;
    }

    /* Scan leading delimiters. */
    s += strspn (s, delim);
    if (*s == '\0') {
        *last = s;
        return NULL;
    }

    /* Find the end of the token. */
    end = s + strcspn (s, delim);
    if (*end == '\0') {
        *last = end;
        return s;
    }

    /* Terminate the token and make *LAST point past it. */
    *end = '\0';
    *last = end + 1;

    return s;
#endif
}


/******************************************************************************
*
* cpl_strcmp_tok - Search the tokens of the delimited string S and return a
*   pointer to the token if the string KEY matches it.  The delimiter character
*   is given by C.  Return NULL if no match is found.  The string comparison is
*   case insensitive.
*
******************************************************************************/

char * cpl_strcmp_tok (
    const char *s,
    const char *key,
    const int c) {

    assert (key);
    assert (c != 0);

    while (s) {

        char *endp = cpl_strchrnul (s, c);
        size_t n = endp - s;

        if ((n != 0) && (cpl_strncasecmp (key, s, n) == 0)) {
            return (char *) s;
        }

        if (*endp) {
            s = ++endp;
        } else {
            s = NULL;
        }
    }

    return NULL;
}


/******************************************************************************
*
* cpl_strrstr - Finds the last occurrence in the string pointed to by S1 of the
*   string pointed to be S2.  If S2 points to a string of zero length, then a
*   pointer to the end of S1 is returned.  The test is case sensitive.
*
* Return: A pointer to the located string, or
*         NULL if the string could be found or either s1 or s2 is NULL.
*
******************************************************************************/

char * cpl_strrstr (
    const char * CPL_RESTRICT s1,
    const char * CPL_RESTRICT s2) {

    if (s1 && s2) {
        const size_t s1_len = strlen (s1);
        const size_t s2_len = strlen (s2);
        const char *p;

        if (s2_len > s1_len) return NULL;

        for (p = s1 + s1_len - s2_len; p >= s1; p--) {
            if (strncmp (p, s2, s2_len) == 0) return (char *) p;
        }
    }

    return NULL;
}


/******************************************************************************
*
* cpl_str_has_prefix - Return a pointer to the PREFIX contained at the beginning
*   of the string S or NULL if it does not exist or S is NULL.  The test is case
*   sensitive.
*
* Return: A pointer to the prefix string, or
*         NULL if the prefix could not be found or the input string is NULL.
*
******************************************************************************/

char * cpl_str_has_prefix (
    const char *s,
    const char *prefix) {

    assert (prefix);

    if (s) {
        if (strncmp (s, prefix, strlen (prefix)) == 0) return (char *) s;
    }

    return NULL;
}


/******************************************************************************
*
* cpl_str_has_suffix - Return a pointer to the SUFFIX contained at the end of
*   the string S or NULL if it does not exist or S is NULL.  The test is case
*   sensitive.
*
* Return: A pointer to the suffix string, or
*         NULL if the suffix could not be found or the input string is NULL.
*
******************************************************************************/

char * cpl_str_has_suffix (
    const char *s,
    const char *suffix) {

    assert (suffix);

    if (s) {
        const size_t suffix_len = strlen (suffix);
        const size_t s_len = strlen (s);

        if (suffix_len <= s_len) {
            const char *p = s + s_len - suffix_len;
            if (strcmp (p, suffix) == 0) return (char *) p;
        }
    }

    return NULL;
}


/******************************************************************************
*
* cpl_str_dot - Return a pointer to the last occurrence of a '.' in a file name
*   in the string S.  Any path name in the file name is removed before searching
*   since the path name may contain '.'.
*
* Return: A pointer to the file extension, or
*         NULL if the input string was NULL or no file extension exists.
*
******************************************************************************/

char * cpl_str_dot (
    const char *s) {

    if (s) {

        /* Remove any path name from the file name. */
        s = cpl_str_basename (s);

        /* Set s to the last occurrence of '.' or NULL if not found. */
        s = strrchr (s, '.');
    }

    return (char *) s;
}


/******************************************************************************
*
* cpl_str_basename - Return a pointer to the first character of the file name
*   after the path name in the string S.  The original string is unmodified.
*   Unlike the POSIX function basename, this function assumes S is a file name
*   and not a path name.
*
*   "/usr/file"  -> "file"        "/usr/.file" -> ".file"
*   "/a\ b"      -> "a\ b"        "/usr/"      -> ""
*
* Return: A pointer to the processed string, or
*         NULL if the input string was NULL.
*
******************************************************************************/

char * cpl_str_basename (
    const char *s) {

    if (s) {
        const char *p;

        /* Search for any base name starting after a forward slash. */
        if ((p = strrchr (s, '/'))) s = p + 1;

#if (defined (_WIN32)  || defined (__WIN32__) || defined (__CYGWIN__) || defined (__EMX__) || \
     defined (__OS2__) || defined (__MSDOS__) || defined (__DJGPP__))
        /* DOS-based file systems can use both forward and backward slashes as
           path separators.  Search for the backward slash on these systems.
           But only for these systems, since the backward slash can be used as
           an escape character in a file name on other systems. */
        if ((p = strrchr (s, '\\'))) s = p + 1;
#endif
    }

    return (char *) s;
}


/******************************************************************************
*
* cpl_str_escape - Escapes the special characters '\b', '\f', '\n', '\r', '\t',
*   '\' and '"' in the string S by inserting a '\' before them.  Additionally
*   all characters in the range 0x01-0x1F (everything below SPACE) and in the
*   range 0x7F-0xFF (all non-ASCII chars) are replaced with a '\' followed by
*   their octal representation.  The returned string must be free'd by the caller.
*
* Return: A pointer to the processed string, or
*         NULL if the input string was NULL or memory allocation failed.
*
******************************************************************************/

char * cpl_str_escape (
    const char *s) {

    const char *p = s;
    char *dest, *q;


    if (!s) return NULL;

    /* Each source byte may need up to a maximum of four destination bytes. */
    dest = q = (char *) cpl_malloc (4 * strlen (s) + 1);
    if (!dest) return NULL;

    while (*p) {

        switch (*p) {

            case '\b' : *q++ = '\\'; *q++ = 'b';  break;
            case '\f' : *q++ = '\\'; *q++ = 'f';  break;
            case '\n' : *q++ = '\\'; *q++ = 'n';  break;
            case '\r' : *q++ = '\\'; *q++ = 'r';  break;
            case '\t' : *q++ = '\\'; *q++ = 't';  break;
            case '\v' : *q++ = '\\'; *q++ = 'v';  break;
            case '\\' : *q++ = '\\'; *q++ = '\\'; break;
            case '"' :  *q++ = '\\'; *q++ = '"';  break;

            default :
                if ((*p < ' ') || (*p >= 0177)) {
                    *q++ = '\\';
                    *q++ = '0' + (((*p) >> 6) & 07);
                    *q++ = '0' + (((*p) >> 3) & 07);
                    *q++ = '0' + ((*p) & 07);
                } else {
                    *q++ = *p;
                }
                break;
        }
        p++;
    }

    *q = '\0';

    return dest;
}


/******************************************************************************
*
* cpl_fputs_escaped - Print the string S to the file stream FP with non-printable
*   characters escaped.  The special characters '\b', '\f', '\n', '\r', '\t',
*   '\' and '"' in the string S are replaced by inserting a '\' before them.
*   Additionally all characters in the range 0x01-0x1F (everything below SPACE)
*   and in the range 0x7F-0xFF (all non-ASCII chars) are replaced with a '\'
*   followed by their octal representation.
*
******************************************************************************/

void cpl_fputs_escaped (
    const char *s,
    FILE *fp) {

    const char *p = s;


    /* TODO: Modify this function to return an error if fputs/fputc fails. */
    assert (fp);

    if (!s) {
        fputs ("(null)", fp);
        return;
    }

    while (*p) {

        switch (*p) {

            case '\b' : fputs ("\\b", fp);  break;
            case '\f' : fputs ("\\f", fp);  break;
            case '\n' : fputs ("\\n", fp);  break;
            case '\r' : fputs ("\\r", fp);  break;
            case '\t' : fputs ("\\t", fp);  break;
            case '\v' : fputs ("\\v", fp);  break;
            case '\\' : fputs ("\\\\", fp); break;
            case '"'  : fputs ("\\\"", fp); break;

            default :
                if ((*p < ' ') || (*p >= 0177)) {
                    fputc ('\\', fp);
                    fputc ('0' + (((*p) >> 6) & 07), fp);
                    fputc ('0' + (((*p) >> 3) & 07), fp);
                    fputc ('0' + ((*p) & 07), fp);
                } else {
                    fputc (*p, fp);
                }
                break;
        }
        p++;
    }
}


/******************************************************************************
*
* cpl_asprintf - Print output of string determined by FORMAT to a character
*   string dynamically allocated.  The memory for the resulting string must be
*   free'd by the caller.  This is a portable version of asprintf.
*
* Return: A pointer to the newly-allocated string, or
*         NULL if memory allocation failed.
*
******************************************************************************/

char * cpl_asprintf (
    const char *format,
    ...) {

    int size = 100;  /* Initial size guess. */
    va_list ap;
    char *p;


    assert (format);

    /* Allocate initial size. */
    p = (char *) cpl_malloc (size);
    /* If memory allocation failed, the NULL will be returned below. */

    while (p) {

        void *ptr;
        int n;

        /* Try to print in the allocated space. */
        va_start (ap, format);
        n = vsnprintf (p, size, format, ap);
        va_end (ap);
        /* Some versions of vsnprintf (glibc < 2.1) will return -1 if there is not
           enough space to store the string.  The following will work around this. */

        /* Return string if successful. */
        if ((n > -1) && (n < size)) break;

        /* Otherwise allocate more space. */
        if (n > -1) size = n+1;
        else size = size + (size + 1) / 2;  /* This is for glibc < 2.1. */

        /* Reallocate the string.  If memory allocation fails, then return NULL. */
        ptr = cpl_realloc (p, size);
        if (!ptr) cpl_free (p);
        p = (char *) ptr;
    }

    return p;
}


/******************************************************************************
*
* cpl_itoa - Convert an integer N to an ASCII string in BUF, which must be at
*   least CPL_ITOA_SIZE bytes long.  Return the address of the string, which
*   need not start at BUF.  This version avoids reversing the string.
*
******************************************************************************/

char * cpl_itoa (
    const long int n,
    char *buf) {

    long int m = n;
    char *p;


    assert (buf);

    p = buf + CPL_ITOA_SIZE - 1;

    *p = '\0';

    if (m < 0) {
        do {
            *--p = '0' - m % 10;
        } while ((m /= 10) != 0);

        *--p = '-';
    } else {
        do {
            *--p = '0' + m % 10;
        } while ((m /= 10) != 0);
    }

    return p;
}


/******************************************************************************
*
* cpl_strlcat - Appends SRC to string DST of size N (unlike strncat, N is the
*   full size of DST, not the space left).  At most N-1 characters will be
*   copied.  Always NUL terminates (unless N <= strlen (DST)).
*
* Return: strlen (SRC) + MIN (N, strlen (initial DST)).  If return value >= N,
*         then truncation occurred.
*
******************************************************************************/

size_t cpl_strlcat (
    char *dst,
    const char *src,
    const size_t n) {

#ifdef HAVE_STRLCAT
    assert (dst);
    assert (src);

    return strlcat (dst, src, n);
#else
    const char *d = dst;
    const char *s = src;
    size_t m = n;
    size_t dlen;


    assert (dst);
    assert (src);

    while ((m-- != 0) && (*dst != '\0')) dst++;

    dlen = dst - d;
    m = n - dlen;

    if (m-- == 0) return dlen + strlen (src);

    while (*src != '\0') {
        if (m != 0) {
            *dst++ = *src;
            m--;
        }
        src++;
    }

    *dst = '\0';

    return dlen + (src - s);
#endif
}


/******************************************************************************
*
* cpl_strlcpy - Copy string SRC to string DST of size N.  At most N-1 characters
*   will be copied.  Always NUL terminates (unless N == 0).
*
* Return: strlen (SRC); if retval >= N, then truncation occurred.
*
******************************************************************************/

size_t cpl_strlcpy (
    char * CPL_RESTRICT dst,
    const char * CPL_RESTRICT src,
    const size_t n) {

#ifdef HAVE_STRLCPY
    assert (dst);
    assert (src);

    return strlcpy (dst, src, n);
#else
    const char *s = src;
    char *d = dst;
    size_t m = n;


    assert (dst);
    assert (src);

    if (m != 0) {
        while (--m != 0) {
            if ((*d++ = *s++) == '\0') break;
        }
    }

    if (m == 0) {
        if (n != 0) *d = '\0';
        while (*s++);
    }

    return s - src - 1;
#endif
}


/******************************************************************************
*
* cpl_strnlen - Return the length of the string S, but scan at most MAXLEN characters.
*
******************************************************************************/

size_t cpl_strnlen (
    const char *s,
    const size_t maxlen) {

#ifdef HAVE_STRNLEN
    assert (s);

    return strnlen (s, maxlen);
#else
    const char *p;


    assert (s);

    p = (const char *) memchr (s, '\0', maxlen);

    if (p) {
        return (size_t) (p - s);
    } else {
        return maxlen;
    }
#endif
}


/******************************************************************************
*
* cpl_str_empty - Return non-zero if the string S is NULL or empty (i.e., consists
*   of the NUL character '\0').
*
******************************************************************************/

int cpl_str_empty (
    const char *s) {

    if (!s) return 1;
    if (*s == '\0') return 1;
    return 0;
}


/******************************************************************************
*
* cpl_strequal - Return non-zero if the input strings S1 and S2 are equal.  If
*   either S1 or S2 is NULL, then zero is returned.
*
******************************************************************************/

int cpl_strequal (
    const char *s1,
    const char *s2) {

    if (s1 && s2) {
        return strcmp (s1, s2) == 0;
    }

    return 0;
}


/******************************************************************************
*
* cpl_strnequal - Return non-zero if the input strings S1 and S2 are equal up
*   to size N.  If either S1 or S2 is NULL, then zero is returned.
*
******************************************************************************/

int cpl_strnequal (
    const char *s1,
    const char *s2,
    const size_t n) {

    if (s1 && s2) {
        return strncmp (s1, s2, n) == 0;
    }

    return 0;
}


/******************************************************************************
*
* cpl_strcaseequal - Return non-zero if the input strings S1 and S2 are equal
*   without consideration of case.  If either S1 or S2 is NULL, then zero is
*   returned.
*
******************************************************************************/

int cpl_strcaseequal (
    const char *s1,
    const char *s2) {

    if (s1 && s2) {
        return cpl_strcasecmp (s1, s2) == 0;
    }

    return 0;
}


/******************************************************************************
*
* cpl_strncaseequal - Return non-zero if the input strings S1 and S2 are equal
*   without consideration of case up to size N.  If either S1 or S2 is NULL,
*   then zero is returned.
*
******************************************************************************/

int cpl_strncaseequal (
    const char *s1,
    const char *s2,
    const size_t n) {

    if (s1 && s2) {
        return cpl_strncasecmp (s1, s2, n) == 0;
    }

    return 0;
}


/******************************************************************************
*
* cpl_strcasecmp - Compare the strings S1 and S2, ignoring case, returning less
*   than, equal to, or greater than zero if S1 is lexigraphically less than,
*   equal to or greater than S2.
*
******************************************************************************/

int cpl_strcasecmp (
    const char *s1,
    const char *s2) {

    assert (s1);
    assert (s2);

#ifdef _MSC_VER

    return stricmp (s1, s2);

#elif defined HAVE_STRCASECMP

    return strcasecmp (s1, s2);

#else

    for (;;) {
        int c1 = CPL_TOLOWER (*s1);
        int c2 = CPL_TOLOWER (*s2);
        if (c1 != c2) return c1 - c2;
        if (c1 == '\0') break;
        s1++; s2++;
    }

    return 0;
#endif
}


/******************************************************************************
*
* cpl_strncasecmp - Same as cpl_strcasecmp, but only compares up to N characters.
*
******************************************************************************/

int cpl_strncasecmp (
    const char *s1,
    const char *s2,
    const size_t n) {

    assert (s1);
    assert (s2);

#ifdef _MSC_VER
    return strnicmp (s1, s2, n);
#elif defined HAVE_STRNCASECMP
    return strncasecmp (s1, s2, n);
#else
    {
        size_t m = n;


        for (; m != 0; --m) {
            int c1 = CPL_TOLOWER (*s1);
            int c2 = CPL_TOLOWER (*s2);
            if (c1 != c2) return c1 - c2;
            if (c1 == '\0') break;
            s1++; s2++;
        }

    }
    return 0;
#endif
}


/******************************************************************************
*
* cpl_strcasewscmp - Compare the strings S1 and S2, ignoring case and excess
*   whitespace, returning less than, equal to, or greater than zero if S1 is
*   lexigraphically less than, equal to or greater than S2.
*
******************************************************************************/

int cpl_strcasewscmp (
    const char *p1,
    const char *p2) {

    const unsigned char *s1 = (const unsigned char *) p1;
    const unsigned char *s2 = (const unsigned char *) p2;


    assert (p1);
    assert (p2);

    while (CPL_ISSPACE (*s1)) s1++;
    while (CPL_ISSPACE (*s2)) s2++;

    for (;;) {

        int c1, c2;

        if (CPL_ISSPACE (*s1) && CPL_ISSPACE (*s2)) {
            while (CPL_ISSPACE (*s1)) s1++;
            while (CPL_ISSPACE (*s2)) s2++;
        } else if ((*s1 == '\0') && CPL_ISSPACE (*s2)) {
            while (CPL_ISSPACE (*s2)) s2++;
        } else if ((*s2 == '\0') && CPL_ISSPACE (*s1)) {
            while (CPL_ISSPACE (*s1)) s1++;
        }

        c1 = CPL_TOLOWER (*s1);
        c2 = CPL_TOLOWER (*s2);

        if (c1 != c2) return c1 - c2;
        if (c1 == '\0') break;
        s1++; s2++;
    }

    return 0;
}


/******************************************************************************
*
* cpl_strncasewscmp - Same as cpl_strcasewscmp, but only compares up to N
*   characters in the first string.
*
******************************************************************************/

int cpl_strncasewscmp (
    const char *p1,
    const char *p2,
    const size_t n) {

    const unsigned char *s1 = (const unsigned char *) p1;
    const unsigned char *s2 = (const unsigned char *) p2;
    const unsigned char *endp;


    assert (p1);
    assert (p2);

    endp = s1 + n;

    while (CPL_ISSPACE (*s1)) s1++;
    while (CPL_ISSPACE (*s2)) s2++;

    for (;;) {

        int c1, c2;

        if (CPL_ISSPACE (*s1) && CPL_ISSPACE (*s2)) {
            while (CPL_ISSPACE (*s1)) s1++;
            while (CPL_ISSPACE (*s2)) s2++;
        } else if ((*s1 == '\0') && CPL_ISSPACE (*s2)) {
            while (CPL_ISSPACE (*s2)) s2++;
        } else if ((*s2 == '\0') && CPL_ISSPACE (*s1)) {
            while (CPL_ISSPACE (*s1)) s1++;
        }

        if (s1 >= endp) break;

        c1 = CPL_TOLOWER (*s1);
        c2 = CPL_TOLOWER (*s2);

        if (c1 != c2) return c1 - c2;
        if (c1 == '\0') break;
        s1++; s2++;
    }

    return 0;
}


/******************************************************************************
*
* cpl_getsubopt - Parse comma-separated suboptions from *OPTIONP and match against
*   strings in TOKENS.  If found return index and set *VALUEP to the optional value
*   following the equal sign.  If the suboption is not part of TOKENS, return
*   in *VALUEP the beginning of the unknown suboption.  On exit *OPTIONP is set
*   to the beginning of the next token or at the terminating NUL character.
*
* Return: The index of matched string, or
*         -1 if no strings matched.
*
******************************************************************************/

int cpl_getsubopt (
    char **optionp,
    const char * const *tokens,
    char **valuep) {

    char *endp, *vstart;
    int cnt;


    assert (tokens);
    assert (valuep);

    if (!optionp || cpl_str_empty (*optionp)) return -1;

    /* Find end of next token (or end of the string). */
    endp = cpl_strchrnul (*optionp, ',');

    /* Find start of value. */
    vstart = (char *) memchr (*optionp, '=', endp - *optionp);
    if (!vstart) vstart = endp;

    /* Try to match the characters between *OPTIONP and VSTART against one of the TOKENS. */
    for (cnt=0; tokens[cnt] != NULL; cnt++) {
        if ((cpl_strncasecmp (*optionp, tokens[cnt], vstart - *optionp) == 0)
                && (tokens[cnt][vstart - *optionp] == '\0')) {
            /* We found the current option in TOKENS. */
            *valuep = vstart != endp ? vstart + 1 : NULL;

            if (*endp != '\0') *endp++ = '\0';
            *optionp = endp;

            return cnt;
        }
    }

    /* The current suboption does not match any option. */
    *valuep = *optionp;

    if (*endp != '\0') *endp++ = '\0';
    *optionp = endp;

    return -1;
}


/******************************************************************************
*
* cpl_str_set_name_value - Concatenate the input strings to form a attribute-value
*   pair given by NAME and VALUE separated by the delimiter DELIM and return
*   a newly-allocated pointer to it.  If the string VALUE is NULL, then only a
*   pointer to 'NAME' is returned.
*
* Return: A newly-allocated pointer to the concatenated strings, or
*         NULL if memory allocation failed.
*
******************************************************************************/

char * cpl_str_set_name_value (
    const char *name,
    const char *value,
    const int delim) {

    assert (!cpl_str_empty (name));

    if (value) {
        /* Allocate a string large enough to store the name, value, '=', and NUL-terminator. */
        const size_t n1 = strlen (name);
        const size_t n2 = strlen (value);
        char *line = (char *) cpl_malloc (n1 + n2 + 2);
        if (!line) return NULL;

        /* Copy the name string, followed by the delimiter, then copy the value string with NUL-terminator. */
        memcpy (line, name, n1);
        line[n1] = delim;
        memcpy (line + n1 + 1, value, n2 + 1);

        return line;
    }

    /* If value is NULL, then just return a string with name. */
    return cpl_strdup (name);
}


/******************************************************************************
*
* cpl_str_get_name - Return the string of the attribute of the attribute-value
*   pair in the string LINE.  The delimiter of the pair is given by DELIM.  The
*   returned string will be allocated and must be free'd by the caller.  Any
*   leading or trailing whitespace will be removed from the returned string.
*
* Return: A newly-allocated string of the name, or
*         NULL if the input line was NULL or memory allocation failed.
*
******************************************************************************/

char * cpl_str_get_name (
    const char *line,
    const int delim) {

    const char *endp;
    char *name;
    size_t n;


    /* Return NULL if there is no input. */
    if (!line) return NULL;

    /* Trim any leading whitespace from the input line. */
    line = cpl_str_ltrim (line);

    /* Search for the delimiter or the terminating nul character. */
    endp = cpl_strchrnul (line, delim);

    /* Set n to the length of the input line up to and including the delimiter. */
    n = (size_t) (endp - line);

    /* Copy the n+1 bytes of the string line or return on memory allocation failure. */
    name = (char *) cpl_memdup (line, n+1);
    if (!name) return NULL;

    /* Set a terminating nul character on the newly-allocated string. */
    name[n] = '\0';

    /* Trim any trailing whitespace. */
    name = cpl_str_rtrim (name);

    return name;
}


/******************************************************************************
*
* cpl_str_get_value - Return the string value matching the attribute NAME from
*   the attribute-value pair given in the string LINE.  The delimiter of the
*   pair is given by DELIM.  The match is made using a case-insensitive match.
*   Excess whitespace is allowed anywhere in the input string.  If no match is
*   made, then NULL will be returned.
*
*   Note that if whitespace is in the name string, then it must also be in the
*   input string except at the beginning of the line:
*     cpl_str_get_value ("test =value", "test", '=') == 0
*     cpl_str_get_value (" test=value", "test", '=') == 0
*     cpl_str_get_value ("test=value", " test", '=') == 0
*     cpl_str_get_value ("test=value", "test ", '=') != 0
*
* Return: A pointer to the value attribute of the string, or
*         NULL if the name attribute does not match.
*
******************************************************************************/

char * cpl_str_get_value (
    const char *line,
    const char *name,
    const int delim) {

    const char *s1 = line;
    const char *s2 = name;
    const char *endp;


    if (!line) return NULL;
    if (!name) return NULL;

    /* Search for the delimiter, return NULL if not found. */
    endp = strchr (line, delim);
    if (!endp) return NULL;

    /* Consume initial whitespace. */
    while (CPL_ISSPACE (*s1)) s1++;
    while (CPL_ISSPACE (*s2)) s2++;

    for (;;) {

        int c1, c2;

        /* Consume whitespace if both strings have it or they are at the end of line. */
        if (CPL_ISSPACE (*s1) && CPL_ISSPACE (*s2)) {
            while (CPL_ISSPACE (*s1)) s1++;
            while (CPL_ISSPACE (*s2)) s2++;
        } else if ((*s1 == '\0') && CPL_ISSPACE (*s2)) {
            while (CPL_ISSPACE (*s2)) s2++;
        } else if ((*s2 == '\0') && CPL_ISSPACE (*s1)) {
            while (CPL_ISSPACE (*s1)) s1++;
        }

        c1 = CPL_TOLOWER (*s1);
        c2 = CPL_TOLOWER (*s2);

        if (s1 >= endp) c1 = '\0';
        if (c1 != c2) return NULL;
        if (c1 == '\0') break;
        s1++; s2++;
    }

    /* Advance past the delimiter, consume whitespace, then return value. */
    return cpl_str_ltrim (endp+1);
}


/******************************************************************************
*
* cpl_str_to_bool - Convert the string S into a boolean value.
*
* Return: 1 if the input string can be interpreted as true,
*        -1 if the input string can be interpreted as false, or
*         0 if it is undetermined or S is NULL.
*
******************************************************************************/

int cpl_str_to_bool (
    const char *s) {

    /* TODO: Add tailptr? */
    /* TODO: The following fails with any trailing whitespace. */

    if (s) {

        /* Remove any leading whitespace. */
        while (CPL_ISSPACE (*s)) s++;

        if (cpl_strcaseequal (s, "true")) return 1;
        if (cpl_strcaseequal (s, "false")) return -1;

        if (cpl_strcaseequal (s, "yes")) return 1;
        if (cpl_strcaseequal (s, "no")) return -1;

        if (cpl_strcaseequal (s, "on")) return 1;
        if (cpl_strcaseequal (s, "off")) return -1;
    }

    return 0;
}


/******************************************************************************
*
* cpl_str_hex_to_uint - Convert a string S to an unsigned integer and return it
*   in ROP.  The string is assumed to be in base 16 with no locale information.
*   The string will be decomposed as [whitespace][0x|X][hhhhh].  If TAILPTR is
*   not NULL, then it will be set to the last unconverted character.  If the
*   string is empty or overflow occurs, then no conversion is done and non-zero
*   is returned.
*
*   This function converts the initial part of the input string S.  If the
*   string S contains only the number to be converted, then the caller should
*   check TAILPTR to make sure there is no trailing garbage.
*
* Output: *ROP - Unsigned integer converted from string or zero on error.
*         *TAILPTR - Pointer to the first non-digit character at the end of the
*           conversion if not NULL.
*
* Return: 0 if the string was successfully converted,
*         1 if no number could be found to convert, or
*        -1 if overflow occurred in the conversion.
*
******************************************************************************/

int cpl_str_hex_to_uint (
    unsigned int *rop,
    const char *s,
    char **tailptr) {

    unsigned long r;
    int ret;


    assert (rop);

    ret = cpl_str_hex_to_ulong (&r, s, tailptr);
    if (ret) return ret;

    if (r > INT_MAX) return -1;

    *rop = r;

    return 0;
}


/******************************************************************************
*
* cpl_str_hex_to_ulong - Convert a string S to an unsigned long integer and
*   return it in ROP.  The string is assumed to be in base 16 with no locale
*   information.  The string will be decomposed as [whitespace][0x|X][hhhhh].  If
*   TAILPTR is not NULL, then it will be set to the last unconverted character.
*   If the string is empty or overflow occurs, then no conversion is done and
*   non-zero is returned.
*
*   This function converts the initial part of the input string S.  If the
*   string S contains only the number to be converted, then the caller should
*   check TAILPTR to make sure there is no trailing garbage.
*
* Output: *ROP - Unsigned long integer converted from string or zero on error.
*         *TAILPTR - Pointer to the first non-digit character at the end of the
*           conversion if not NULL.
*
* Return: 0 if the string was successfully converted,
*         1 if no number could be found to convert, or
*        -1 if overflow occurred in the conversion.
*
******************************************************************************/

int cpl_str_hex_to_ulong (
    unsigned long *rop,
    const char *s,
    char **tailptr) {

    const char *str = s;
    unsigned long num = 0;
    int overflow = 0;
    int any = 0;
    int n;


    assert (rop);

    if (!s) {
        *rop = 0;
        if (tailptr) *tailptr = NULL;
        return 1;
    }

    /* Remove leading whitespace. */
    while (CPL_ISSPACE (*s)) s++;

    /* Remove any leading 0x hex identifier. */
    if ((s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X'))) {
        s = s + 2;
    }

    /* Remove any leading zeros. */
    while (*s == '0') {
        any = 1;
        s++;
    }

    n = 2 * sizeof (unsigned long);

    /* Extract digits until the first non-hex character. */
    for (;;) {

        unsigned long d;

        /* If the next character is not a hex digit, then exit the loop.  Otherwise, convert to an integer. */
        if ((*s >= '0') && (*s <= '9')) {
            d = *s - '0';
        } else if ((*s >= 'A') && (*s <= 'F')) {
            d = 10 + *s - 'A';
        } else if ((*s >= 'a') && (*s <= 'f')) {
            d = 10 + *s - 'a';
        } else {
            break;
        }

        if (!overflow) {
            /* Check if the result will overflow. */
            if (n == 0) {
                overflow = -1;
                num = ULONG_MAX;
            } else {
                any = 1;
                num = num * 16 + d;
            }
            n--;
        }
        /* Whether we have overflow or not, continue to consume digits. */
        s++;
    }

    /* Set the tailptr to the last unprocessed character in the string. */
    if (tailptr) *tailptr = (char *) (any ? s : str);

    *rop = num;

    /* If no characters were converted, then return 1 else return overflow boolean. */
    return (any == 0) ? 1 : overflow;
}


/******************************************************************************
*
* cpl_str_to_int - Convert a string S to an integer and return it in ROP.  The
*   string is assumed to be in base 10 with no locale information.  The string
*   will be decomposed as [whitespace][+|-][nnnnn].  If TAILPTR is not NULL,
*   then it will be set to the last unconverted character.  If the string is
*   empty or overflow occurs, then no conversion is done and non-zero is
*   returned.
*
*   This function converts the initial part of the input string S.  If the
*   string S contains only the number to be converted, then the caller should
*   check TAILPTR to make sure there is no trailing garbage.
*
* Output: *ROP - Signed integer converted from string or zero on error.
*         *TAILPTR - Pointer to the first non-digit character at the end of the
*           conversion if not NULL.
*
* Return: 0 if the string was successfully converted,
*         1 if no number could be found to convert, or
*        -1 if overflow occurred in the conversion.
*
******************************************************************************/

int cpl_str_to_int (
    int *rop,
    const char *s,
    char **tailptr) {

    unsigned long r;
    int neg = 0;
    int ret;


    assert (rop);

    ret = cpl_strtoul_1 (&r, s, tailptr, &neg);
    if (ret) return ret;

    if (neg) {
        if (r > (unsigned long) INT_MAX + 1) return -1;
        *rop = -r;
    } else {
        if (r > INT_MAX) return -1;
        *rop = r;
    }

    return 0;
}


/******************************************************************************
*
* cpl_str_to_ulong - Convert a string S to an unsigned long integer and return
*   it in ROP.  The string is assumed to be in base 10 with no locale
*   information.  The string will be decomposed as [whitespace][+][nnnnn].  If
*   TAILPTR is not NULL, then it will be set to the last unconverted character.
*   If the string is empty or overflow occurs, then no conversion is done and
*   non-zero is returned.
*
*   This function converts the initial part of the input string S.  If the
*   string S contains only the number to be converted, then the caller should
*   check TAILPTR to make sure there is no trailing garbage.
*
* Output: *ROP - Unsigned long integer converted from string or zero on error.
*         *TAILPTR - Pointer to the first non-digit character at the end of the
*           conversion if not NULL.
*
* Return: 0 if the string was successfully converted,
*         1 if no number could be found to convert, or
*        -1 if overflow occurred in the conversion.
*
******************************************************************************/

int cpl_str_to_ulong (
    unsigned long *rop,
    const char *s,
    char **tailptr) {

    return cpl_strtoul_1 (rop, s, tailptr, NULL);
}


/******************************************************************************
*
* cpl_strtoul_1 - Convert a string S to an unsigned long integer and return
*   it in ROP.  The string is assumed to be in base 10 with no locale
*   information.  The string will be decomposed as [whitespace][+][nnnnn].  If
*   TAILPTR is not NULL, then it will be set to the last unconverted character.
*   If the string is empty or overflow occurs, then no conversion is done and
*   non-zero is returned.
*
*   This function converts the initial part of the input string S.  If the
*   string S contains only the number to be converted, then the caller should
*   check TAILPTR to make sure there is no trailing garbage.
*
* Output: *ROP - Unsigned long integer converted from string or zero on error.
*         *TAILPTR - Pointer to the first non-digit character at the end of the
*           conversion if not NULL.
*         *NEG - Set to non-zero if the string contains a negative sign.
*
* Return: 0 if the string was successfully converted,
*         1 if no number could be found to convert, or
*        -1 if overflow occurred in the conversion.
*
******************************************************************************/

static int cpl_strtoul_1 (
    unsigned long *rop,
    const char *s,
    char **tailptr,
    int *neg) {

    const char *str = s;
    unsigned long num = 0;
    int overflow = 0;
    int any = 0;


    assert (rop);

    if (!s) {
        *rop = 0;
        if (tailptr) *tailptr = NULL;
        return 1;
    }

    /* Remove leading whitespace. */
    while (CPL_ISSPACE (*s)) s++;

    /* Determine the sign, if any. */
    if (neg && (*s == '-')) {
        *neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    /* Remove any leading zeros. */
    while (*s == '0') {
        any = 1;
        s++;
    }

    /* Extract digits until the first non-digit character. */
    for (;;) {

        const unsigned long cutoff = (unsigned long) ULONG_MAX / 10;
        const unsigned long cutlim = (unsigned long) ULONG_MAX % 10;
        unsigned long d;

        /* If the next character is not a digit, then exit the loop.  Otherwise, convert to a number. */
        if ((d = (unsigned long) (*s - '0')) > 9) break;

        if (!overflow) {
            /* Check if the result will overflow. */
            if ((num > cutoff) || ((num == cutoff) && (d > cutlim))) {
                overflow = -1;
                num = ULONG_MAX;
            } else {
                any = 1;
                num = num * 10 + d;
            }
        }
        /* Whether we have overflow or not, continue to consume digits. */
        s++;
    }

    /* Set the tailptr to the last unprocessed character in the string. */
    if (tailptr) *tailptr = (char *) (any ? s : str);

    *rop = num;

    /* If no characters were converted, then return 1 else return overflow boolean. */
    return (any == 0) ? 1 : overflow;
}


/******************************************************************************
*
* cpl_str_to_double - Convert the initial part of the string STR from an ASCII
*   decimal number representation to a double-precision floating point number
*   decomposed as [whitespace][+|-][nnnnn][.nnnnn].  If TAILPTR is not NULL,
*   then return a pointer to the end of the converted string in it.  The
*   converted number is returned in ROP.
*
*   This function does not support scientific notation, locale-dependent decimal,
*   hexadecimal format, or overflow/underflow detection.  This is useful when
*   the string is known to be in a simple format as it is much faster than strtod.
*
*   This function converts the initial part of the input string S.  If the
*   string S contains only the number to be converted, then the caller should
*   check TAILPTR to make sure there is no trailing garbage.
*
* Return: 0 if the string is successfully converted, or
*         1 if no number could be found to convert.
*
******************************************************************************/

int cpl_str_to_double (
    double *rop,
    const char *str,
    char **tailptr) {

    const char *s = str;
    double d = 0.0;
    int neg = 0;
    int any = 0;


    assert (rop);

    if (!s) {
        *rop = 0.0;
        if (tailptr) *tailptr = NULL;
        return 1;
    }

    /* Remove leading whitespace. */
    while (CPL_ISSPACE (*s)) s++;

    /* Determine the sign, if any. */
    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    /* Convert the integer part. */
    while (CPL_ISDIGIT (*s)) {
        d = d * 10.0 + (*s - '0');
        s++;
        any = 1;
    }

    /* Convert the fraction part, if any. */
    if (*s == '.') {
        double pow_10 = 1.0;

        s++;

        while (CPL_ISDIGIT (*s)) {
            /* While we could scale D by 0.1 each iteration and accumulate the digits, '0.1' does not
               have a finite binary representation and will accumulate round-off error.  Instead we
               calculate POW10 with a final division at the end to improve accuracy. */
            pow_10 = pow_10 * 10.0;
            d = d * 10.0 + (*s - '0');
            s++;
            any = 1;
        }

        d = d / pow_10;
    }

    /* Set the tailptr to the last unprocessed character in the string. */
    if (tailptr) *tailptr = (char *) (any ? s : str);

    *rop = neg ? -d : d;

    /* If no characters were converted, then return 1. */
    return (any == 0) ? 1 : 0;
}


/******************************************************************************
*
* cpl_strtod - Convert the initial part of the string S from an ASCII decimal
*   representation to a double-precision floating point number decomposed as
*   [whitespace][+|-][nnnnn][.nnnnn][e|E[+|-]nnnn].  If TAILPTR is not NULL,
*   then return a pointer to the end of the converted string in it.  The
*   converted number is returned in ROP.
*
*   This function converts the initial part of the input string S.  If the
*   string S contains only the number to be converted, then the caller should
*   check TAILPTR to make sure there is no trailing garbage.
*
* Return: 0 if the string is successfully converted,
*         1 if no number could be found to convert, or
*        -1 if overflow occurred in the conversion.
*
******************************************************************************/

int cpl_strtod (
    double *rop,
    const char *s,
    char **tailptr) {

    int err, errno_orig;
    char *endptr;
    double number;


    assert (rop);

    *rop = 0.0;
    if (tailptr) *tailptr = (char *) s;

    if (!s) return 1;

    /* Be pedantic and preserve errno. */
    errno_orig = errno;
    errno = 0;

    number = strtod (s, &endptr);

    err = errno;
    errno = errno_orig;

    /* Test for no conversion. */
    if (endptr == s) return 1;

    /* Check for overflow.  We avoid explicit test for ERANGE for portability. */
    if (err != 0) {
        if (fabs (number) == HUGE_VAL) return -1;
    }

    /* Set the tailptr to the last unprocessed character in the string. */
    if (tailptr) *tailptr = endptr;

    *rop = number;

    return 0;
}


/******************************************************************************
*
* cpl_str_hash - Calculate the hash value of a string S using a multiplicative
*   hash function (modified DJ Bernstein Hash).
*
******************************************************************************/

size_t cpl_str_hash (
    const char *s) {

    size_t hash = 5381;
    char c;


    assert (s);

    while ((c = *s++)) {
        size_t e = (unsigned char) c;
        hash = (hash * 33) ^ e;
    }

    return hash;
}


/******************************************************************************
*
* cpl_str_cat - Concatenate two strings S1 and S2 into a newly-allocated string
*   and return the result.
*
* Return: A pointer to newly-allocated string, or
*         NULL if memory allocation failed or either S1 or S2 are NULL.
*
******************************************************************************/

char * cpl_str_cat (
    const char *s1,
    const char *s2) {

    size_t n1, n2;
    char *s;


    if (!s1 && !s2) return NULL;

    if (!s1) {
        return cpl_strdup (s2);
    }

    if (!s2) {
        return cpl_strdup (s1);
    }

    n1 = strlen (s1);
    n2 = strlen (s2);

    s = (char *) cpl_malloc (n1 + n2 + 1);
    if (!s) return NULL;

    memcpy (s, s1, n1);
    memcpy (s + n1, s2, n2 + 1);

    return s;
}


/******************************************************************************
*
* cpl_strvcat - Concatenate a NULL-terminated list of strings and return the
*   resulting string.  The newly-allocated string must be free'd by the caller.
*   Make sure none of the strings in the list are NULL before calling this.
*
* Return: A newly-allocated string with the concatenated list, or
*         NULL if memory allocation failed.
*
******************************************************************************/

char * cpl_strvcat (
    const char *s,
    ...) {

    va_list ap;
    size_t allocated = 100;
    char *result;
    char *newp, *wp;
    const char *q;


    assert (s);

    va_start (ap, s);

    wp = result = (char *) cpl_malloc (allocated);
    if (!wp) {
        va_end (ap);  /* Clean up stack. */
        return NULL;
    }

    for (q=s; q; q=va_arg (ap, const char *)) {
        size_t len = strlen (q);

        /* Resize the allocated memory if necessary. */
        if (wp + len + 1 > result + allocated) {
            size_t offset = wp - result;
            allocated = (allocated + len) * 2;
            newp = (char *) cpl_realloc (result, allocated);
            if (!newp) {
                cpl_free (result);
                va_end (ap);  /* Clean up stack. */
                return NULL;
            }
            wp = newp + offset;
            result = newp;
        }
        wp = (char *) memcpy (wp, q, len) + len;
    }

    /* Terminate the result string. */
    *wp++ = '\0';

    /* Resize memory to the actual size. */
    newp = (char *) cpl_realloc (result, wp - result);
    if (!newp) cpl_free (result);
    result = newp;

    va_end (ap);  /* Clean up stack. */

    return result;
}


/******************************************************************************
*
* cpl_getline - Read up to (and including) a new-line character from the file
*   stream FP into a character buffer *LINEPTR (and NUL-terminate it).  *LINEPTR
*   is a pointer returned from malloc (or NULL), pointing to *N characters of
*   space.  It is realloc'd as necessary.  The caller must free the line buffer
*   on return.  If MAX_LEN is non-zero, then an error condition (CS_EFAIL) will
*   be returned if it is exceeded.
*
* Return: 0 if the line of text is read successfully, or
*         1 if EOF is reached, or
*         error condition if an error occurred.
*
* Errors: CS_EINVAL
*         CS_EREAD
*         CS_ENOMEM
*         CS_EFAIL
*         CS_EOVERFLOW
*
******************************************************************************/

int cpl_getline (
    char **lineptr,
    size_t *n,
    const size_t max_len,
    FILE *fp) {

    return cpl_getdelim (lineptr, n, max_len, '\n', fp);
}


/******************************************************************************
*
* cpl_getdelim - Read up to (and including) a DELIMITER from the file stream FP
*   into a character buffer *LINEPTR (and NUL-terminate it).  *LINEPTR is a
*   pointer returned from malloc (or NULL), pointing to *N characters of space.
*   It is realloc'd as necessary.  The caller must free the line buffer on
*   return.  If MAX_LEN is non-zero, then an error condition (CS_EFAIL) will
*   be returned if it is exceeded.
*
* Return: 0 if the line of text is read successfully, or
*         1 if EOF is reached, or
*         error condition if an error occurred.
*
* Errors: CS_EINVAL
*         CS_EREAD
*         CS_ENOMEM
*         CS_EFAIL
*         CS_EOVERFLOW
*
******************************************************************************/

int cpl_getdelim (
    char **lineptr,
    size_t *n,
    const size_t max_len,
    const int delimiter,
    FILE *fp) {

    size_t cur_len = 0;
    int status = 0;


    assert (n);
    assert (fp);
    assert (lineptr);

#ifdef HAVE_FLOCKFILE
    flockfile (fp);
#endif

    /* Allocate an initial size if needed. */
    if ((*lineptr == NULL) || (*n == 0)) {
        *n = 200;
        *lineptr = (char *) cpl_malloc (*n);
        if (!(*lineptr)) {
            *n = 0;
            status = CS_ENOMEM;
            goto unlock_return;
        }
    }

    for (;;) {

        int c;

        c = getc (fp);

        /* If an error condition or EOF is reached, then return. */
        if (c == EOF) {

            /* We must use feof to distinguish between an error condition and EOF. */
            if (!feof (fp)) {

                /* If getc returns EOF and the stream is not EOF, then it is an error. */
                cpl_debug (cpl_lib_debug, "getc failed: %s\n", strerror (errno));
                if (errno == ENOMEM) status = CS_ENOMEM;
                else status = CS_EREAD;
                goto unlock_return;
            }

            /* Only return EOF status if we have not read anything into the buffer. */
            if (cur_len == 0) {
                status = 1;
            }

            break;
        }

        /* Allocate enough space for cur_len+1 (for terminating NUL) bytes.  */
        if (cur_len + 1 >= *n) {

            /* Set needed = ceil (1.5 * (cur_len + 1)). */
            size_t needed = (cur_len + 1) + (cur_len + 2) / 2;
            char *new_lineptr;

            /* Check for integer overflow. */
            if (needed < cur_len) {
                (*lineptr)[0] = '\0';
                status = CS_EOVERFLOW;
                goto unlock_return;
            }

            /* Reallocate the buffer as needed. */
            new_lineptr = (char *) cpl_realloc (*lineptr, needed);
            if (!new_lineptr) {
                (*lineptr)[0] = '\0';
                status = CS_ENOMEM;
                goto unlock_return;
            }

            *lineptr = new_lineptr;
            *n = needed;
        }

        (*lineptr)[cur_len] = c;
        cur_len++;

        if (c == delimiter) break;

        /* If max length has been reached, then return an error. */
        if ((max_len > 0) && (cur_len == max_len)) {
            (*lineptr)[0] = '\0';
            status = CS_EFAIL;
            goto unlock_return;
        }
    }

    /* NUL-terminate the line. */
    (*lineptr)[cur_len] = '\0';

unlock_return:

#ifdef HAVE_FLOCKFILE
    funlockfile (fp);
#endif

    return status;
}

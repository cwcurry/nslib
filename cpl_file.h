/* cpl_file.h -- Header file for cpl_file.c

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

#ifndef CPL_FILE_H
#define CPL_FILE_H

#if defined (__cplusplus)
#include <cstdio>
#else
#include <stdio.h>
#endif

#if defined (__cplusplus)
#include <ctime>
#else
#include <time.h>
#endif

#include "cpl_spec.h"


/* off_t and ssize_t are POSIX; get definitions for MSVC */
#if defined(_MSC_VER)
#include <sys/types.h>  /* for off_t */
#include <BaseTsd.h>    /* for SSIZE_T */
typedef SSIZE_T ssize_t;
#endif


/* File Open Access Flags */
#define CPL_OPEN_RDONLY      0x0001   /* Open for reading only.                     */
#define CPL_OPEN_WRONLY      0x0002   /* Open for writing only.                     */
#define CPL_OPEN_APPEND      0x0004   /* Writes will add to the end of the file.    */
#define CPL_OPEN_RDWR        0x0008   /* Open for reading and writing.              */
#define CPL_OPEN_CREATE      0x0100   /* Create the file if it does not exist.      */
#define CPL_OPEN_TRUNC       0x0200   /* Truncate the file if it exists.            */
#define CPL_OPEN_EXCL        0x0400   /* Create the file only if it does not exist. */
#define CPL_OPEN_BINARY      0x8000   /* No CR-LF conversion is done.               */
#define CPL_OPEN_RANDOM      0x0010   /* File is accessed randomly.                 */
#define CPL_OPEN_SEQUENTIAL  0x0020   /* The file will be accessed sequentially.    */


/* File Fopen Access Flags */
#define CPL_FOPEN_READ       0x0001   /* Open the file for reading only (mode="r").      */
#define CPL_FOPEN_WRITE      0x0002   /* Truncate or create file for writing (mode="w"). */
#define CPL_FOPEN_APPEND     0x0004   /* Open or create file for appending (mode="a").   */
#define CPL_FOPEN_UPDATE     0x0008   /* Open file for reading and writing (mode="+").   */
#define CPL_FOPEN_BINARY     0x0010   /* Open the file as a binary file (mode="b").      */
#define CPL_FOPEN_EXCL       0x0100   /* Create the file only if it does not exist.      */


/* File Stat Type */
typedef struct {
    off_t file_size;       /* File size in bytes.         */
    time_t atime;          /* Time of last access.        */
    time_t mtime;          /* Time of last modification.  */
    time_t ctime;          /* Time of last status change. */
    int s_isreg;           /* Non-zero if regular file.   */
    int s_isdir;           /* Non-zero if directory.      */
    int s_islnk;           /* Non-zero if symbolic link.  */
} cpl_stat_t;


/******************************* API Functions *******************************/

CPL_CLINKAGE_START

int cpl_fstat (const int, cpl_stat_t *) CPL_ATTRIBUTE_NONNULL_ALL;
int cpl_stat (const char *, cpl_stat_t *) CPL_ATTRIBUTE_NONNULL_ALL;
int cpl_fseeko (FILE *, const off_t, const int) CPL_ATTRIBUTE_NONNULL_ALL;
off_t cpl_seek (const int, const off_t, const int);
size_t cpl_fread (void *, const size_t, FILE *) CPL_ATTRIBUTE_NONNULL_ALL;
ssize_t cpl_read (void *, const size_t, const int) CPL_ATTRIBUTE_NONNULL_ALL;
ssize_t cpl_write (const int, const void *, const size_t) CPL_ATTRIBUTE_NONNULL_ALL;
int cpl_file_exists (const char *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
FILE * cpl_fopen (const char *, const int) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
int cpl_open (const char *, const int) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
int cpl_create (const char *) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
int cpl_fclose (FILE *);
int cpl_close (const int);
char * cpl_realpath (const char *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT CPL_ATTRIBUTE_NONNULL_ALL;
int cpl_remove (const char *);
int cpl_mkstemp (char *) CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
int cpl_chdir (const char *) CPL_ATTRIBUTE_NONNULL_ALL;


CPL_CLINKAGE_END

#endif /* CPL_FILE_H */

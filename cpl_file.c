/* cpl_file.c -- Portable file I/O functions.

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


 Compiler Defines:
   HAVE_FSEEKO   - Defined if fseeko is available.
   HAVE_REALPATH - Defined if realpath is available. */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include "cpl_file.h"
#include "cpl_debug.h"
#include "cpl_alloc.h"
#include "cpl_str.h"

#if defined (CPL_WIN32_API)
# define WIN32_LEAN_AND_MEAN  /* Exclude Cryptography, DDE, RPC, Shell, and Windows Sockets API. */
# define F_OK  0
# include <windows.h>
# include <direct.h>
#else
# include <unistd.h>
#endif


/* Define to test if EINTR is available. */
#ifdef EINTR
# define CPL_IS_EINTR(x) ((x) == EINTR)
#else
# define CPL_IS_EINTR(x) 0
#endif


/* Define some permissions if needed. */
#ifndef  S_IRUSR
#define  S_IRUSR 0000400 /* Read permission, owner. */
#endif

#ifndef  S_IWUSR
#define  S_IWUSR 0000200 /* Write permission, owner. */
#endif


/* External Variable */
extern int cpl_lib_debug;


/******************************************************************************
*
* cpl_fstat - Return attributes about the file given by the open file descriptor
*   FD and store in the structure STBUF.  This is a portable version of fstat.
*
* Return: 0 if the fstat function was successful, and
*        -1 on failure.
*
******************************************************************************/

int cpl_fstat (
    const int fd,
    cpl_stat_t *stbuf) {

    int retval;


    assert (stbuf);
    assert (fd != -1);

#if defined (_MSC_VER)
# if CPL_MSC_PREREQ (13, 1)
    struct _stat s;
    retval = _fstat (fd, &s);
# else
    struct __stat64 s;
    retval = _fstat64 (fd, &s);
# endif
#else
    struct stat s;
    retval = fstat (fd, &s);
#endif

    if (retval == 0) {
        stbuf->file_size = s.st_size;
        stbuf->ctime = s.st_ctime;
        stbuf->mtime = s.st_mtime;
        stbuf->s_isreg = S_ISREG (s.st_mode);
        stbuf->s_isdir = S_ISDIR (s.st_mode);
#if defined (CPL_WIN32_API)
        stbuf->s_islnk = 0;
#else
        stbuf->s_islnk = S_ISLNK (s.st_mode);
#endif
    } else {
        cpl_debug (cpl_lib_debug, "fstat failed: %s\n", strerror (errno));
    }

    return retval;
}


/******************************************************************************
*
* cpl_stat - Return attributes about the file given by the file name FILE_NAME
*   and store in the structure STBUF.  This is a portable version of stat.
*
* Return: 0 if the stat function was successful, and
*        -1 on failure.
*
******************************************************************************/

int cpl_stat (
    const char *file_name,
    cpl_stat_t *stbuf) {

    int retval = 0;


    assert (file_name);
    assert (stbuf);

#if defined (_MSC_VER)
# if CPL_MSC_PREREQ (13, 1)
    struct _stat s;
    retval = _stat (file_name, &s);
# else
    struct __stat64 s;
    retval = _stat64 (file_name, &s);
# endif
#else
    struct stat s;
    retval = stat (file_name, &s);
#endif

    if (retval == 0) {
        stbuf->file_size = s.st_size;
        stbuf->ctime = s.st_ctime;
        stbuf->mtime = s.st_mtime;
        stbuf->s_isreg = S_ISREG (s.st_mode);
        stbuf->s_isdir = S_ISDIR (s.st_mode);
#if defined (CPL_WIN32_API)
        stbuf->s_islnk = 0;
#else
        stbuf->s_islnk = S_ISLNK (s.st_mode);
#endif
    } else {
        cpl_debug (cpl_lib_debug, "stat failed: %s\n", strerror (errno));
    }

    return retval;
}


/******************************************************************************
*
* cpl_fseeko - Set the current file position of the open file stream FP.  The
*   position is determined by adding OFFSET to the position specified by WHENCE.
*
* Return: 0 if the fseek/fseeko function was successful, and
*         A non-zero value on failure.
*
******************************************************************************/

int cpl_fseeko (
    FILE *fp,
    const off_t offset,
    const int whence) {

    int retval;


    assert (fp);

#if defined (HAVE_FSEEKO)
    retval = fseeko (fp, offset, whence);
#else
# if defined (LONG_MAX)
    if (offset > ((off_t) LONG_MAX)) {
        cpl_debug (cpl_lib_debug, "Offset overflow in fseek\n");
        return -1;
    }
# endif
    retval = fseek (fp, (long int) offset, whence);
#endif

    if (retval != 0) {
        cpl_debug (cpl_lib_debug, "fseek failed: %s\n", strerror (errno));
    }

    return retval;
}


/******************************************************************************
*
* cpl_seek - Set the current file position of the open file descriptor FD.  The
*   position is determined by adding OFFSET to the position specified by WHENCE.
*
* Return: The resulting offset from the beginning of the file, or
*         -1 on failure.
*
******************************************************************************/

off_t cpl_seek (
    const int fd,
    const off_t offset,
    const int whence) {

    off_t retval;


    assert (fd != -1);

    retval = lseek (fd, offset, whence);

    if (retval == (off_t) -1) {
        cpl_debug (cpl_lib_debug, "lseek failed: %s\n", strerror (errno));
    }

    return retval;
}


/******************************************************************************
*
* cpl_fread - Read SIZE bytes from the stream FP into the BUFFER.
*
* Return: Return the number of bytes successfully read, which may be less than
*   SIZE (e.g., at the end of the file or a read error occurred).
*
******************************************************************************/

size_t cpl_fread (
    void *buffer,
    const size_t size,
    FILE *fp) {

    size_t retval = 0;


    assert (fp);
    assert (buffer);

    if (size == 0) return 0;

    retval = fread (buffer, size, 1, fp);

    if ((retval != 1) && (feof (fp) == 0)) {
        cpl_debug (cpl_lib_debug, "Error reading data (%lu bytes) from file: %s\n", (unsigned long) size, strerror (errno));
    }

    return retval;
}


/******************************************************************************
*
* cpl_read - Read up to SIZE bytes into BUFFER from the open file descriptor FD,
*   retrying if interrupted.  The read is atomic for ordinary files, pipes, and
*   FIFOs.  Therefore, no additional work is done for partial reads.  This
*   function should not be used on devices that may return partial reads.
*
* Return: Return the number of bytes successfully read, which may be less than
*           SIZE (e.g., at the end of the file or a read error occurred), or
*        -1 if an error occurred.
*
******************************************************************************/

ssize_t cpl_read (
    void *buffer,
    const size_t size,
    const int fd) {

    ssize_t bytes_read;


    assert (buffer);
    assert (fd != -1);

    errno = 0;

    do {
        bytes_read = read (fd, buffer, size);
    } while ((bytes_read < 0) && (CPL_IS_EINTR (errno)));

    if (bytes_read < 0) {
        cpl_debug (cpl_lib_debug, "Error reading data (%lu bytes) from file (fd=%d): %s\n", (unsigned long) size, fd, strerror (errno));
    }

    return bytes_read;
}


/******************************************************************************
*
* cpl_write - Write up to SIZE bytes at BUFFER to the file descriptor FD, retrying
*   if interrupted.  The write is atomic for ordinary files, pipes, and FIFOs.
*   Therefore, no additional work is done for partial writes.  This function
*   should not be used on devices that may perform partial writes.
*
* Return: Return the number of bytes successfully written, which may be less than
*           SIZE (e.g., if the disk is full), or
*        -1 if an error occurred.
*
******************************************************************************/

ssize_t cpl_write (
    const int fd,
    const void *buffer,
    const size_t size) {

    ssize_t bytes_written;


    assert (buffer);
    assert (fd != -1);

    errno = 0;

    do {
        bytes_written = write (fd, buffer, size);
    } while ((bytes_written < 0) && (CPL_IS_EINTR (errno)));

    if (bytes_written < 0) {
        cpl_debug (cpl_lib_debug, "Error writing data (%lu bytes) to file (fd=%d): %s\n", (unsigned long) size, fd, strerror (errno));
    }

    return bytes_written;
}


/******************************************************************************
*
* cpl_file_exists - Return non-zero if the file given by FILE_NAME already exists.
*   This is generally faster than using stat.
*
******************************************************************************/

int cpl_file_exists (
    const char *file_name) {

    if (cpl_str_empty (file_name)) return 0;
    return (access (file_name, F_OK) == 0) ? 1 : 0;
}


/******************************************************************************
*
* cpl_fopen - Open a stream for I/O for the file given by FILE_NAME and return a
*   pointer to the stream.  The stream is opened in the mode given by the FLAGS.
*
* Return: A pointer to the file stream, or
*         NULL if the file could not be successfully opened with the given mode.
*
******************************************************************************/

FILE * cpl_fopen (
    const char *file_name,
    const int flags) {

    char mode[4];
    FILE *fp;


    assert (file_name);

    mode[1] = '\0';

    if (flags & CPL_FOPEN_READ) {
        mode[0] = 'r';
    } else if (flags & CPL_FOPEN_WRITE) {
        mode[0] = 'w';
    } else if (flags & CPL_FOPEN_APPEND) {
        mode[0] = 'a';
    } else {
        cpl_debug (cpl_lib_debug, "Invalid file open mode: '%s' (%d)\n", file_name, flags);
        return NULL;
    }

    if (flags & CPL_FOPEN_UPDATE) {
        mode[1] = '+';
        mode[2] = '\0';
        if (flags & CPL_FOPEN_BINARY) {
            mode[2] = 'b';
            mode[3] = '\0';
        }
    } else if (flags & CPL_FOPEN_BINARY) {
        mode[1] = 'b';
        mode[2] = '\0';
    }

    /* Don't clobber existing file if requested. */
    if (flags & CPL_FOPEN_EXCL) {
        if (cpl_file_exists (file_name)) {
            cpl_debug (cpl_lib_debug, "File already exists: '%s' (flags=%d, mode='%s')\n", file_name, flags, mode);
            return NULL;
        }
    }

    do {
        fp = fopen (file_name, mode);
    } while ((fp == NULL) && (CPL_IS_EINTR (errno)));

    if (!fp) {
        cpl_debug (cpl_lib_debug, "Error opening file: '%s' (flags=%d, mode='%s'): %s\n", file_name, flags, mode, strerror (errno));
    }

    return fp;
}


/******************************************************************************
*
* cpl_open - Open a file given by FILE_NAME using the specified FLAGS for reading
*   or writing, retrying if interrupted.
*
* Return: The file descriptor of the open file,
*         -1 if the file could not be opened.
*
******************************************************************************/

int cpl_open (
    const char *file_name,
    const int flags) {

    int access_flags = 0;
    int fd;


    assert (file_name);

    if (flags & CPL_OPEN_RDONLY) access_flags |= O_RDONLY;
    if (flags & CPL_OPEN_WRONLY) access_flags |= O_WRONLY;
    if (flags & CPL_OPEN_RDWR)   access_flags |= O_RDWR;
    if (flags & CPL_OPEN_APPEND) access_flags |= O_APPEND;
    if (flags & CPL_OPEN_CREATE) access_flags |= O_CREAT;
    if (flags & CPL_OPEN_TRUNC)  access_flags |= O_TRUNC;
    if (flags & CPL_OPEN_EXCL)   access_flags |= O_EXCL;

#if defined (O_BINARY)
    /* WinAPI requires O_BINARY flag for binary data since the default is O_TEXT. */
    if (flags & CPL_OPEN_BINARY) access_flags |= O_BINARY;
#endif

#if defined (O_SEQUENTIAL)
    if (flags & CPL_OPEN_SEQUENTIAL) access_flags |= O_SEQUENTIAL;
#endif

#if defined (O_RANDOM)
    if (flags & CPL_OPEN_RANDOM) access_flags |= O_RANDOM;
#endif

    do {
        fd = open (file_name, access_flags);
    } while ((fd < 0) && (CPL_IS_EINTR (errno)));

    if (fd == -1) {
        cpl_debug (cpl_lib_debug, "Error opening file: '%s' (flags=%d): %s\n", file_name, flags, strerror (errno));
        return fd;
    }

    /* If the O_SEQUENTIAL flag is not defined, try setting it using posix_fadvise. */
#if !defined (O_SEQUENTIAL) && defined (POSIX_FADV_SEQUENTIAL)
    if (posix_fadvise (fd, 0, 0, POSIX_FADV_SEQUENTIAL) != 0) {
        cpl_debug (cpl_lib_debug, "posix_fadvise failed: %s\n", strerror (errno));
    }
#endif

    /* If the O_RANDOM flag is not defined, try setting it using posix_fadvise. */
#if !defined (O_RANDOM) && defined (POSIX_FADV_RANDOM)
    if (posix_fadvise (fd, 0, 0, POSIX_FADV_RANDOM) != 0) {
        cpl_debug (cpl_lib_debug, "posix_fadvise failed: %s\n", strerror (errno));
    }
#endif

    return fd;
}


/******************************************************************************
*
* cpl_create - Create the binary file given by FILE_NAME for reading and
*   writing, retrying if interrupted.  Permissions are set only for the owner
*   to read and write the file.
*
* Return: The file descriptor of the open file,
*         -1 if failed to open the file or the file already exists.
*
******************************************************************************/

int cpl_create (
    const char *file_name) {

    int fd;


    assert (file_name);

    do {
        /* Read and write permission for the owner. */
#if defined (O_BINARY)
        /* WinAPI requires O_BINARY flag for binary data since the default is O_TEXT. */
        fd = open (file_name, O_CREAT | O_RDWR | O_BINARY, S_IRUSR | S_IWUSR);
#else
        fd = open (file_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
#endif
    } while ((fd < 0) && (CPL_IS_EINTR (errno)));

    return fd;
}


/******************************************************************************
*
* cpl_fclose - Close the file stream given by FP.
*
* Return: 0 if the file is successfully closed.
*
******************************************************************************/

int cpl_fclose (
    FILE *fp) {

    int result = 0;


    if (fp) {
        result = fclose (fp);

        if (result) {
            cpl_debug (cpl_lib_debug, "Error closing file stream: %s\n", strerror (errno));
        }
    }

    return result;
}


/******************************************************************************
*
* cpl_close - Close the file given by the file descriptor FD, retrying if
*   interrupted.
*
* Return: 0 if the file is successfully closed.
*
******************************************************************************/

int cpl_close (
    const int fd) {

    int result = 0;


    if (fd != -1) {
        do {
            result = close (fd);
        } while ((result == -1) && (CPL_IS_EINTR (errno)));

        if (result) cpl_debug (cpl_lib_debug, "Error closing file: %s\n", strerror (errno));
    }

    return result;
}


/******************************************************************************
*
* cpl_realpath - Return the canonicalized name of the FILE_NAME which does not
*   contain a '.' or '..' nor any repeated path separators or symlinks.  The
*   returned value is allocated and must be free'd by the user.  If the function
*   fails, then the original file name will be returned.  This function is not
*   multithread safe.
*
* Return: A newly-allocated string with a canonicalized file name, or
*         NULL if memory allocation failed.
*
******************************************************************************/

char * cpl_realpath (
    const char *file_name) {

    assert (file_name);

#if defined (HAVE_REALPATH)
# if defined (PATH_MAX)
    char buf[PATH_MAX];
    const char *rp = realpath (file_name, buf);
# else
    const char *rp = realpath (file_name, (char *) NULL);
# endif

    if (rp) {
        return cpl_strdup (rp);
    } else {
        cpl_debug (cpl_lib_debug, "realpath failed: %s\n", strerror (errno));
    }

#elif defined (CPL_WIN32_API)
    /* The W32API method.  If we don't have realpath, we assume we don't
       have symlinks and just canonicalize to a Windows absolute path. */
    char buf[MAX_PATH];
    char **basename = NULL;
    DWORD len = GetFullPathName (file_name, MAX_PATH, buf, basename);

    if ((len > 0) && (len < MAX_PATH)) {
        /* The file system is case-preserving, but case-insensitive, so canonicalize to lowercase. */
        cpl_str_tolower (buf);
        return cpl_strdup (buf);
    } else {
        cpl_debug (cpl_lib_debug, "GetFullPathName failed (len=%ld) (%ld)\n", (long int) len, (long int) GetLastError ());
    }
#endif

    /* If file name is NULL, then cpl_strdup will return NULL. */
    return cpl_strdup (file_name);
}


/******************************************************************************
*
* cpl_remove - Remove the file or directory given by FILE_NAME from the system.
*
* Return: 0 if the file is successfully deleted.
*
******************************************************************************/

int cpl_remove (
    const char *file_name) {

    int retval = 0;


    if (file_name) {
        retval = remove (file_name);

        if (retval != 0) {
            cpl_debug (cpl_lib_debug, "Error removing file '%s': %s\n", file_name, strerror (errno));
        }
    }

    return retval;
}


/******************************************************************************
*
* cpl_mkstemp - Open a temporary file with a unique name.  Mode is 0600.
*
*   template = "/tmp/file_XXXXXX"
*
******************************************************************************/

int cpl_mkstemp (
    char *file_template) {

#if defined (__MINGW32__)
    char *file_name;


    assert (file_template);
    file_name = mktemp (file_template);
    if (file_name == NULL) return -1;

    return open (file_name, O_RDWR | O_CREAT, 0600);
#else
    assert (file_template);
    return mkstemp (file_template);
#endif
}


/******************************************************************************
*
* cpl_chdir - Set the process's working directory to FILE_PATH.
*
* Return: 0 if the directory is successfully changed, and
*         non-zero if an error occurred.
*
******************************************************************************/

int cpl_chdir (
    const char *file_path) {

    int retval;


#ifdef CPL_WIN32_API
    retval = _chdir (file_path);
#else
    retval = chdir (file_path);

    if (retval != 0) {
        cpl_debug (cpl_lib_debug, "Error changing directory to '%s': %s\n", file_path, strerror (errno));
    }
#endif

    return retval;
}

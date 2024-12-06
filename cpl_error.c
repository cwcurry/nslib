/* cpl_error.c -- Error reporting functions.

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

#include <stdlib.h>
#include <stdio.h>
#include "cpl_error.h"


/* Private Function Prototypes */
static void cpl_error_default_handler (const char *, const int, const char *, va_list);


/* Private Variables */
static cpl_error_handler error_handler = cpl_error_default_handler;


/******************************************************************************
*
* cpl_error - Report an error by calling the installed error_handler with the
*   caller-supplied FILE and LINE number and printf-style FORMAT string.  The
*   file can be NULL or an empty string if not specified and the line number
*   can be negative if not specified.  After the error handler returns, the
*   program will be aborted.
*
******************************************************************************/

void cpl_error (
    const char *file,
    const int line,
    const char *format,
    ...) {

    va_list ap;


    /* Call the error handler. */
    if (error_handler) {
        va_start (ap, format);
        (* error_handler) (file, line, format, ap);
        va_end (ap);
    }

    /* Terminate the program. */
    abort ();
}


/******************************************************************************
*
* cpl_set_error_handler - Set the error handler to HANDLER and return a pointer
*   to the previously set handler.  The handler function is stored in a static
*   variable, therefore there can only be one handler per program and calling
*   this function is not multi-thread safe.
*
******************************************************************************/

cpl_error_handler cpl_set_error_handler (
    cpl_error_handler handler) {

    cpl_error_handler old_handler;


    old_handler = error_handler;
    error_handler = handler;

    return old_handler;
}


/******************************************************************************
*
* cpl_error_default_handler - Default error handler function.
*
******************************************************************************/

static void cpl_error_default_handler (
    const char *file,
    const int line,
    const char *format,
    va_list ap) {

    if (file && file[0]) {

        if (line > 0) {
            fprintf (stderr, "%s: %d: ", file, line);
        } else {
            fprintf (stderr, "%s: ", file);
        }
    }

    vfprintf (stderr, format, ap);

    fputs ("Aborting program...\n", stderr);

    /* Synchronize stdout and stderr. */
    fflush (stderr);
    fflush (stdout);
}


/******************************************************************************
*
* cpl_error_get_string - Return a const string to the description of the error
*   condition given by ERR.
*
******************************************************************************/

const char * cpl_error_get_string (
    const int err) {

    const char *s;


    switch (err) {
        case CS_ENONE     : s = "No error";               break;
        case CS_EINVAL    : s = "Invalid argument";       break;
        case CS_ENOMEM    : s = "Out of memory";          break;
        case CS_EOPEN     : s = "Open error";             break;
        case CS_ECLOSE    : s = "Close error";            break;
        case CS_EREAD     : s = "Read error";             break;
        case CS_EWRITE    : s = "Write error";            break;
        case CS_ESEEK     : s = "Seek error";             break;
        case CS_EDOM      : s = "Domain error";           break;
        case CS_EFAIL     : s = "Fail";                   break;
        case CS_ENOPROG   : s = "No progress";            break;
        case CS_EMAXITER  : s = "Max iterations reached"; break;
        case CS_ESING     : s = "Singular matrix";        break;
        case CS_EBADFN    : s = "Bad function";           break;
        case CS_EBADDATA  : s = "Bad data";               break;
        case CS_EUNSUP    : s = "Unsupported";            break;
        case CS_EOVERFLOW : s = "Overflow";               break;
        default           : s = "Unknown error";          break;
    }

    return s;
}


/******************************************************************************
*
* cpl_error_get_name - Return a const string to the name of the error condition
*   code given by ERR.
*
******************************************************************************/

const char * cpl_error_get_name (
    const int err) {

    const char *s;


    switch (err) {
        case CS_ENONE     : s = "CS_ENONE";     break;
        case CS_EINVAL    : s = "CS_EINVAL";    break;
        case CS_ENOMEM    : s = "CS_ENOMEM";    break;
        case CS_EOPEN     : s = "CS_EOPEN";     break;
        case CS_ECLOSE    : s = "CS_ECLOSE";    break;
        case CS_EREAD     : s = "CS_EREAD";     break;
        case CS_EWRITE    : s = "CS_EWRITE";    break;
        case CS_ESEEK     : s = "CS_ESEEK";     break;
        case CS_EDOM      : s = "CS_EDOM";      break;
        case CS_EFAIL     : s = "CS_EFAIL";     break;
        case CS_ENOPROG   : s = "CS_ENOPROG";   break;
        case CS_EMAXITER  : s = "CS_EMAXITER";  break;
        case CS_ESING     : s = "CS_ESING";     break;
        case CS_EBADFN    : s = "CS_EBADFN";    break;
        case CS_EBADDATA  : s = "CS_EBADDATA";  break;
        case CS_EUNSUP    : s = "CS_EUNSUP";    break;
        case CS_EOVERFLOW : s = "CS_EOVERFLOW"; break;
        default           : s = "Unknown";      break;
    }

    return s;
}

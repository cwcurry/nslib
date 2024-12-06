/* cpl_alloc.c - Memory allocation functions.

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
#include <string.h>
#include <assert.h>
#include "cpl_alloc.h"
#include "cpl_debug.h"


/* Private Variable Declarations */
static cpl_malloc_fn g_malloc_fn = malloc;
static cpl_calloc_fn g_calloc_fn = calloc;
static cpl_realloc_fn g_realloc_fn = realloc;
static cpl_free_fn g_free_fn = free;


/* External Variable */
extern int cpl_lib_debug;


/******************************************************************************
*
* cpl_malloc - Call the currently set memory allocation function.
*
******************************************************************************/

void * cpl_malloc (
    const size_t size) {

    if (size == 0) {
        cpl_debug (cpl_lib_debug, "Attempt to allocate zero bytes\n");
    }

    return (* g_malloc_fn) (size);
}


/******************************************************************************
*
* cpl_calloc - Call the currently set cleared-memory allocation function.
*
******************************************************************************/

void * cpl_calloc (
    const size_t nelem,
    const size_t elsize) {

    if ((nelem == 0) || (elsize == 0)) {
        cpl_debug (cpl_lib_debug, "Attempt to allocate zero bytes\n");
    }

    return (* g_calloc_fn) (nelem, elsize);
}


/******************************************************************************
*
* cpl_realloc - Call the currently set memory reallocation function.
*
******************************************************************************/

void * cpl_realloc (
    void *ptr,
    const size_t size) {

    return (* g_realloc_fn) (ptr, size);
}


/******************************************************************************
*
* cpl_free - Call the currently set memory deallocation function.
*
******************************************************************************/

void cpl_free (
    void *ptr) {

    if (!ptr) {
        cpl_debug (cpl_lib_debug, "Attempt to free NULL pointer\n");
    }

    (* g_free_fn) (ptr);
}


/******************************************************************************
*
* cpl_realloc_zero - Reallocate a memory block starting at PTR.  The size of the
*   block in bytes is given in OLD_SIZE and the reallocated size in bytes is
*   given by NEW_SIZE.  If the new size is greater than the old size, then the
*   remaining bytes are set to zero.
*
* Return: A pointer to the newly-reallocated memory block, or
*         NULL if memory reallocation failed.
*
******************************************************************************/

void * cpl_realloc_zero (
    void *ptr,
    const size_t old_size,
    const size_t new_size) {

    void *p;


    p = cpl_realloc (ptr, new_size);
    if (CPL_UNLIKELY (!p)) return NULL;
    ptr = p;

    if (new_size > old_size) {
        p = (char *) ptr + old_size;
        memset (p, 0, new_size - old_size);
    }

    return ptr;
}


/******************************************************************************
*
* cpl_realloc2 - Allocate memory storage for at least N objects of size ELSIZE.
*   If N is greater than or equal to the allocated size *PN, then PTR is reallocated
*   to contain more than N objects.  Otherwise, nothing is done.  This function
*   is useful for growing an array without excessive reallocations.
*
******************************************************************************/

void * cpl_realloc2 (
    void *ptr,
    const size_t n,
    const size_t elsize,
    size_t *pn) {

    assert (pn);
    assert (elsize > 0);
    assert (ptr ? *pn > 0 : *pn == 0);

    if (n >= *pn) {
        /* Set *pn = ceil (1.5 * n) so that progress is made if n == 1. */
        *pn = n + (n + 1) / 2;
        if (*pn == 0) *pn = 1;
        return cpl_realloc (ptr, *pn * elsize);
    }

    return ptr;
}


/******************************************************************************
*
* cpl_resize - This is the same as cpl_realloc2 except the contents are not
*   preserved during resizing to avoid copying memory.
*
******************************************************************************/

void * cpl_resize (
    void *ptr,
    const size_t n,
    const size_t elsize,
    size_t *pn) {

    assert (pn);
    assert (elsize > 0);
    assert (ptr ? *pn > 0 : *pn == 0);

    if (n >= *pn) {
        /* Set *pn = ceil (1.5 * n) so that progress is made if n == 1. */
        *pn = n + (n + 1) / 2;
        if (*pn == 0) *pn = 1;
        if (ptr) cpl_free (ptr);
        return cpl_malloc (*pn * elsize);
    }

    return ptr;
}


/******************************************************************************
*
* cpl_ptr_align - Align PTR to the next multiple of ALIGNMENT bytes.  The caller
*   must arrange for ((char *) PTR) through ((char *) PTR + ALIGNMENT - 1) to be
*   addressable locations.  Alignment size must be non-zero.
*
******************************************************************************/

void * cpl_ptr_align (
    const void *ptr,
    const size_t alignment) {

    const char *p0 = (const char *) ptr;
    const char *p1;


    assert (ptr);
    assert (alignment > 0);

    p1 = p0 + alignment - 1;

    return (void *) (p1 - (size_t) p1 % alignment);
}


/******************************************************************************
*
* cpl_steal_pointer - Set the pointer PTR to NULL and return the value it had.
*   Note that PTR is actually a (void **) pointer type.  For example, if
*   we have void *x, *y, then write y = cpl_steal_pointer (&x);
*
******************************************************************************/

void * cpl_steal_pointer (
    void *ptr) {

    /* We want PTR to be a void** type to return the value in it.  However, while
       void* is a generic pointer, void** is not and would require a cast by the
       caller.  So instead we cast it here and declare PTR to be a void* type. */
    void *p, **q;


    assert (ptr);

    q = (void **) ptr;
    p = *q;
    *q = NULL;

    return p;
}


/******************************************************************************
*
* cpl_set_memory_functions - Replace the current memory allocation functions
*   from the arguments.  If an argument is NULL, then the corresponding default
*   function is used.  This must be called before any allocations are made.
*
******************************************************************************/

void cpl_set_memory_functions (
    cpl_malloc_fn new_malloc_fn,
    cpl_calloc_fn new_calloc_fn,
    cpl_realloc_fn new_realloc_fn,
    cpl_free_fn new_free_fn) {

    if (!new_malloc_fn) new_malloc_fn = malloc;
    if (!new_calloc_fn) new_calloc_fn = calloc;
    if (!new_realloc_fn) new_realloc_fn = realloc;
    if (!new_free_fn) new_free_fn = free;

    g_malloc_fn = new_malloc_fn;
    g_calloc_fn = new_calloc_fn;
    g_realloc_fn = new_realloc_fn;
    g_free_fn = new_free_fn;
}


/******************************************************************************
*
* cpl_get_memory_functions - Get the current allocation functions, storing
*   function pointers to the locations given by the arguments. If an argument
*   is NULL, that function pointer is not stored.
*
******************************************************************************/

void cpl_get_memory_functions (
    cpl_malloc_fn *malloc_fn,
    cpl_calloc_fn *calloc_fn,
    cpl_realloc_fn *realloc_fn,
    cpl_free_fn *free_fn) {

    if (malloc_fn) *malloc_fn = g_malloc_fn;
    if (calloc_fn) *calloc_fn = g_calloc_fn;
    if (realloc_fn) *realloc_fn = g_realloc_fn;
    if (free_fn) *free_fn = g_free_fn;
}


/******************************************************************************
*
* cpl_memlist_add - Add a pointer to allocated memory PTR to a linked-list
*   LIST of pointers.  This allows all pointers to be free'd at once.
*
******************************************************************************/

void * cpl_memlist_add (
    const void *list,
    const void *ptr) {

    const void **node = (const void **) cpl_malloc (2 * sizeof (void *));


    if (node) {
        node[0] = list;
        node[1] = ptr;
    }

    return (void *) node;
}


/******************************************************************************
*
* cpl_memlist_replace - Replace a pointer OLDPTR with a new pointer NEWPTR in
*   a linked-list LIST.  This can replace a pointer returned from realloc.
*
******************************************************************************/

void cpl_memlist_replace (
    void *list,
    const void *oldptr,
    void *newptr) {

    while (list) {
        void **node = (void **) list;

        list = node[0];
        if (node[1] == oldptr) {
            node[1] = newptr;
            break;
        }
    }
}


/******************************************************************************
*
* cpl_memlist_free - Free all pointers contained in the linked-list LIST.
*
******************************************************************************/

void cpl_memlist_free (
    void *list) {

    while (list) {
        void **node = (void **) list;

        list = node[0];
        cpl_free (node[1]);
        cpl_free (node);
    }
}

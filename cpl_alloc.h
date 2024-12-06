/* cpl_alloc.h -- Header file for cpl_alloc.c

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

#ifndef CPL_ALLOC_H
#define CPL_ALLOC_H

#if defined (__cplusplus)
#include <cstddef>
#else
#include <stddef.h>
#endif

#include "cpl_spec.h"


/* Type of functions used to allocate, reallocate, and free memory. */
typedef void * (* cpl_malloc_fn) (const size_t);
typedef void * (* cpl_calloc_fn) (const size_t, const size_t);
typedef void * (* cpl_realloc_fn) (void *, const size_t);
typedef void (* cpl_free_fn) (void *);


/******************************* API Functions *******************************/

CPL_CLINKAGE_START

void * cpl_malloc (const size_t) CPL_ATTRIBUTE_MALLOC CPL_ATTRIBUTE_ALLOC_SIZE (1) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
void * cpl_calloc (const size_t, const size_t) CPL_ATTRIBUTE_MALLOC CPL_ATTRIBUTE_ALLOC_SIZE2 (1, 2) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
void * cpl_realloc (void *, const size_t) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
void cpl_free (void *);
void * cpl_realloc_zero (void *, const size_t, const size_t) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
void * cpl_realloc2 (void *, const size_t, const size_t, size_t *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT CPL_ATTRIBUTE_NONNULL (4);
void * cpl_resize (void *, const size_t, const size_t, size_t *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT CPL_ATTRIBUTE_NONNULL (4);
void * cpl_ptr_align (const void *, const size_t) CPL_ATTRIBUTE_WARN_UNUSED_RESULT CPL_ATTRIBUTE_NONNULL_ALL CPL_ATTRIBUTE_RETURNS_NONNULL;
void * cpl_steal_pointer (void *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT CPL_ATTRIBUTE_NONNULL_ALL;
void cpl_set_memory_functions (cpl_malloc_fn, cpl_calloc_fn, cpl_realloc_fn, cpl_free_fn);
void cpl_get_memory_functions (cpl_malloc_fn *, cpl_calloc_fn *, cpl_realloc_fn *, cpl_free_fn *);
void * cpl_memlist_add (const void *, const void *) CPL_ATTRIBUTE_WARN_UNUSED_RESULT;
void cpl_memlist_replace (void *, const void *, void *);
void cpl_memlist_free (void *);


CPL_CLINKAGE_END

#endif /* CPL_ALLOC_H */

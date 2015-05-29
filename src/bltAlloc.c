/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltAlloc.c --
 *
 * Copyright 2015 George A. Howlett. All rights reserved.  
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *   1) Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2) Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the
 *      distribution.
 *   3) Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *   4) Products derived from this software may not be called "BLT" nor may
 *      "BLT" appear in their names without specific prior written
 *      permission from the author.
 *
 *   THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
 *   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"
#include "tclIntDecls.h"

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_MALLOC_H
  #include <malloc.h>
#endif /* HAVE_MALLOC_H */

#ifdef HAVE_MEMORY_H
  #include <memory.h>
#endif /* HAVE_MEMORY_H */

/*
 *   Memory allocation/deallocation within BLT is performed via the global
 *   variables bltMallocPtr, bltFreePtr, and bltReallocPtr.  (Before TCL
 *   8.6) They default to the same routines that TCL uses.  The routine
 *   Blt_AllocInit allows you to specify your own memory allocation and
 *   deallocation routines for BLT on a library-wide basis.
 */
#ifndef WIN32
  #if (_TCL_VERSION < _VERSION(8,1,0)) 
    #if !HAVE_DECL_FREE
       BLT_EXTERN void free (void *);
    #endif
  #endif /* < 8.1.0 */
#endif /* WIN32 */

static Blt_MallocProc *bltMallocPtr;
static Blt_ReallocProc *bltReallocPtr;
static Blt_FreeProc *bltFreePtr;

static int initialized = FALSE;

void *
Blt_Malloc(size_t size) 
{
    return (*bltMallocPtr)(size);
}


void
Blt_Free(const void *mem)
{
    (*bltFreePtr)((void *)mem);
}

void *
Blt_Realloc(void *ptr, size_t size)
{
    return (*bltReallocPtr)(ptr, size);
}

void *
Blt_Calloc(size_t numElem, size_t elemSize)
{
    void *ptr;
    size_t size;

    size = numElem * elemSize;
    ptr = (*bltMallocPtr)(size);
    if (ptr != NULL) {
        memset(ptr, 0, size);
    }
    return ptr;
}

void *
Blt_MallocAbortOnError(size_t size, const char *fileName, int lineNum) 
{
    void *ptr;

    ptr = (*bltMallocPtr)(size);
    if (ptr == NULL) {
        Blt_Warn("line %d of %s: can't allocate %lu bytes of memory\n", 
                lineNum, fileName, (unsigned long)size);
        abort();
    }
    return ptr;
}

void *
Blt_CallocAbortOnError(size_t numElem, size_t elemSize, const char *fileName, 
                       int lineNum)
{
    void *ptr;
    size_t size;

    size = numElem * elemSize;
    ptr = (*bltMallocPtr)(size);
    if (ptr == NULL) {
        Blt_Warn("line %d of %s: can't allocate %lu item(s) of "
                 "size %lu each\n", lineNum, fileName, (unsigned long)numElem, 
                (unsigned long)elemSize);
        abort();
    }
    memset(ptr, 0, size);
    return ptr;
}

void *
Blt_ReallocAbortOnError(void *ptr, size_t size, const char *fileName, 
                       int lineNum)
{
    void *ptr2;

    ptr2 = (*bltReallocPtr)(ptr, size);
    if (ptr2 == NULL) {
        Blt_Warn("line %d of %s: can't reallocate array or size %lu bytes\n", 
                lineNum, fileName, (unsigned long)size);
        abort();
    }
    return ptr2;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Strndup --
 *
 *      Create a copy of the string of length size from heap storage.
 *
 * Results:
 *      Returns a pointer to the need string copy.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_Strndup(const char *string, size_t size)
{
    char *ptr;

    ptr = (*bltMallocPtr)((size + 1) * sizeof(char));
    if (ptr != NULL) {
        strncpy(ptr, string, size);
        ptr[size] = '\0';
    }
    return ptr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Strdup --
 *
 *      Create a copy of the string from heap storage.
 *
 * Results:
 *      Returns a pointer to the need string copy.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_Strdup(const char *string)
{
    size_t size;
    char *ptr;

    size = strlen(string) + 1;
    ptr = (*bltMallocPtr)(size * sizeof(char));
    if (ptr != NULL) {
        strcpy(ptr, string);
    }
    return ptr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_StrdupAbortOnError --
 *
 *      Create a copy of the string from heap storage.
 *
 * Results:
 *      Returns a pointer to the need string copy.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_StrdupAbortOnError(const char *string, const char *fileName, int lineNum)
{
    size_t size;
    char *ptr;

    size = strlen(string) + 1;
    ptr = (*bltMallocPtr)(size * sizeof(char));
    if (ptr == NULL) {
        Blt_Warn("line %d of %s: can't allocate string of %lu bytes\n",
                lineNum, fileName, (unsigned long)size);
        abort();
    }
    strcpy(ptr, string);
    return ptr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_StrndupAbortOnError --
 *
 *      Create a copy of the string from heap storage.
 *
 * Results:
 *      Returns a pointer to the need string copy.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_StrndupAbortOnError(const char *string, size_t size, const char *fileName,
                        int lineNum)
{
    char *ptr;

    ptr = (*bltMallocPtr)((size + 1) * sizeof(char));
    if (ptr == NULL) {
        Blt_Warn("line %d of %s: can't allocate string of %lu bytes\n",
                lineNum, fileName, (unsigned long)size);
        abort();
    }
    strncpy(ptr, string, size);
    ptr[size] = '\0';
    return ptr;
}

void
Blt_AllocInit(Blt_MallocProc *mallocProc, Blt_ReallocProc *reallocProc,
              Blt_FreeProc *freeProc)
{
    Blt_MallocProc *defMallocProc;
    Blt_FreeProc *defFreeProc;
    Blt_ReallocProc *defReallocProc;
    
    if (initialized) {
        Blt_Panic("Allocation routines for BLT have been already set");
    }
    initialized = TRUE;
    /* 
     * Try to use the same memory allocator/deallocator that TCL is
     * using. Before 8.1 it used malloc/free. 
     */
#if (_TCL_VERSION >= _VERSION(8,1,0)) && (_TCL_VERSION < _VERSION(8,6,0)) 
    /* 
     * We're pointing to the private TclpAlloc/TclpFree instead of public
     * Tcl_Alloc/Tcl_Free routines because they don't automatically trigger
     * a panic when not enough memory is available. There are cases (such
     * as allocating a very large vector) where an out-of-memory error is
     * recoverable.
     */
    defMallocProc = (Blt_MallocProc *)TclpAlloc;
    defFreeProc = (Blt_FreeProc *)TclpFree; 
    defReallocProc = (Blt_ReallocProc *)TclpRealloc; 
#else 
    defMallocProc = malloc;
    defFreeProc = free; 
    defReallocProc = realloc;
#endif /* >= 8.1.0 */
    if (bltMallocPtr == NULL) {
        bltMallocPtr = (mallocProc != NULL) ? mallocProc : defMallocProc;
    }
    if (bltFreePtr == NULL) {
        bltFreePtr = (freeProc != NULL) ? freeProc : defFreeProc;
    }
    if (bltReallocPtr == NULL) {
        bltReallocPtr = (reallocProc != NULL) ? reallocProc : defReallocProc;
    }
}


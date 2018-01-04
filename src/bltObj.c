/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltObj.c --
 *
 * This file implements various TCL objects for BLT.
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

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif /* HAVE_ERRNO_H */

#include <bltAlloc.h>
#include "bltMath.h"
#include "bltHash.h"
#include "bltArrayObj.h"

/* 
 * bltArrayObjType --
 *
 *      Array object used by blt::tree.  Tcl_Obj contains a pointer to
 *      a BLT hash table. The string representation of the array is a
 *      name-value TCL list. 
 */
static Tcl_DupInternalRepProc ArrayObjDupInternalRep;
static Tcl_FreeInternalRepProc ArrayObjFreeInternalRep;
static Tcl_UpdateStringProc ArrayObjUpdateStringRep;
static Tcl_SetFromAnyProc ArrayObjSetFromAny;

static Tcl_ObjType bltArrayObjType = {
    (char *)"blt_array",
    ArrayObjFreeInternalRep,            /* Called when an object is
                                         * freed. */
    ArrayObjDupInternalRep,             /* Copies an internal
                                         * representation from one object
                                         * to another. */
    ArrayObjUpdateStringRep,             /* Creates string representation
                                         * from an object's internal
                                         * representation. */
    ArrayObjSetFromAny,                 /* Creates valid internal
                                         * representation from an object's
                                         * string representation. */
};


/* 
 * bltInt64ObjType --
 *
 *      64-bit integer object.  Used by blt::sftp, blt::datatable to parse
 *      64-bit integer numbers.  This biggest difference from the
 *      Tcl_WideIntObj is that leading 0 values are *not* treated as octal.
 *      Many data files contain values with leading zeros. The only base
 *      allowed is 10. No hexidecimal strings.
 */
static Tcl_UpdateStringProc Int64ObjUpdateStringRep;
static Tcl_SetFromAnyProc Int64ObjSetFromAny;

static Tcl_ObjType bltInt64ObjType = {
    (char *)"blt_int64",
    NULL,                                /* Called when an object is
                                          * freed. */
    NULL,                                /* Copies an internal
                                          * representation from one object
                                          * to another. */
    Int64ObjUpdateStringRep,             /* Creates string representation
                                          * from an object's internal
                                          * representation. */
    Int64ObjSetFromAny,                     /* Creates valid internal
                                          * representation from an object's
                                          * string representation. */
};

/* 
 * bltLongObjType --
 *
 *      32 or 64-bit integer object depending upon machine architecture.  
 *      Used by blt::datatable, blt::tree to parse integer numbers.  
 *      This biggest difference from the Tcl_IntObj or Tcl_LongObj is that
 *      is that leading 0 values are *not* treated as octal.
 */
static Tcl_UpdateStringProc LongObjUpdateStringRep;
static Tcl_SetFromAnyProc   LongObjSetFromAny;

static Tcl_ObjType bltLongObjType = {
    (char *)"blt_long",
    NULL,                               /* Called when an object is
                                         * freed. */
    NULL,                               /* Copies an internal
                                         * representation from one object
                                         * to another. */
    LongObjUpdateStringRep,             /* Creates string representation
                                         * from an object's internal
                                         * representation. */
    LongObjSetFromAny,                  /* Creates valid internal
                                         * representation from an object's
                                         * string representation. */
};

/* 
 * bltUnsignedLongObjType --
 *
 *      32 or 64-bit integer object depending upon machine architecture.  
 *      Used by blt::datatable, blt::tree to parse integer numbers.  
 *      This biggest difference from the Tcl_IntObj or Tcl_LongObj is that
 *      is that leading 0 values are *not* treated as octal.
 *
 *      Not currently used.
 */
static Tcl_UpdateStringProc UnsignedLongObjUpdateStringRep;
static Tcl_SetFromAnyProc UnsignedLongObjSetFromAny;

static Tcl_ObjType bltUnsignedLongObjType = {
    (char *)"blt_unsigned_long",
    NULL,                               /* Called when an object is
                                         * freed. */
    NULL,                               /* Copies an internal
                                         * representation from one object
                                         * to another. */
    UnsignedLongObjUpdateStringRep,      /* Creates string representation
                                         * from an object's internal
                                         * representation. */
    UnsignedLongObjSetFromAny,             /* Creates valid internal
                                         * representation from an object's
                                         * string representation. */
};

/* 
 * bltDoubleObjType --
 *
 *      64-bit floating point object.  Used by blt::datatable, blt::tree to
 *      parse double numbers.  This biggest difference from the
 *      Tcl_DoubleObj is that is that -inf, +inf, and NaN are *not* treated
 *      as errors.
 */
static Tcl_UpdateStringProc DoubleObjUpdateStringRep;
static Tcl_SetFromAnyProc   DoubleObjSetFromAny;

static Tcl_ObjType bltDoubleObjType = {
    (char *)"blt_double",
    NULL,                               /* Called when an object is
                                         * freed. */
    NULL,                               /* Copies an internal
                                         * representation from one object *
                                         * to another. */
    DoubleObjUpdateStringRep,           /* Creates string representation
                                         * from an object's internal
                                         * representation. */
    DoubleObjSetFromAny,                /* Creates valid internal
                                         * representation from an object's
                                         * string representation. */
};

static const Tcl_ObjType *tclIntObjTypePtr;
static const Tcl_ObjType *tclDoubleObjTypePtr;
static const Tcl_ObjType *tclWideIntObjTypePtr;

static INLINE void
FreeInternalRep(Tcl_Obj *objPtr)
{
    if ((objPtr->typePtr != NULL) &&
        (objPtr->typePtr->freeIntRepProc != NULL)) {   
	(*objPtr->typePtr->freeIntRepProc)(objPtr);  
    }
}

static int
ArrayObjSetFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Blt_HashTable *tablePtr;
    Tcl_Obj **objv;
    int objc, i;

    if (objPtr->typePtr == &bltArrayObjType) {
        return TCL_OK;
    }
    /* Get the string representation. Make it up-to-date if necessary. */
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    tablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
    Blt_InitHashTable(tablePtr, BLT_STRING_KEYS);
    for (i = 0; i < objc; i += 2) {
        Blt_HashEntry *hPtr;
        Tcl_Obj *elemObjPtr;
        int isNew;
        const char *key;

        key = Tcl_GetString(objv[i]);
        hPtr = Blt_CreateHashEntry(tablePtr, key, &isNew);
        elemObjPtr = objv[i+1];
        Blt_SetHashValue(hPtr, elemObjPtr);

        /* Make sure we increment the reference count */
        Tcl_IncrRefCount(elemObjPtr);
    }
    FreeInternalRep(objPtr);
    objPtr->internalRep.otherValuePtr = tablePtr;
    objPtr->typePtr = &bltArrayObjType;
    return TCL_OK;
}

static void
ArrayObjDupInternalRep(
    Tcl_Obj *srcPtr,                    /* Object with internal rep to
                                         * copy. */
    Tcl_Obj *destPtr)                   /* Object with internal rep to
                                         * set. */
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Blt_HashTable *srcTablePtr, *destTablePtr;

    srcTablePtr = (Blt_HashTable *)srcPtr->internalRep.otherValuePtr;
    destTablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
    Blt_InitHashTable(destTablePtr, BLT_STRING_KEYS);
    for (hPtr = Blt_FirstHashEntry(srcTablePtr, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        Tcl_Obj *valueObjPtr;
        const char *key;
        int isNew;

        key = Blt_GetHashKey(srcTablePtr, hPtr);
        Blt_CreateHashEntry(destTablePtr, key, &isNew);
        valueObjPtr = Blt_GetHashValue(hPtr);
        Blt_SetHashValue(hPtr, valueObjPtr);

        /* Make sure we increment the reference count now that both array
         * objects are using the same elements. */
        Tcl_IncrRefCount(valueObjPtr);
    }
    Tcl_InvalidateStringRep(destPtr);
    destPtr->internalRep.otherValuePtr = (VOID *)destTablePtr;
    destPtr->typePtr = &bltArrayObjType;
}

static void
ArrayObjUpdateStringRep(Tcl_Obj *objPtr) /* Array object w/ string rep to
                                           update. */
{
    Tcl_DString ds;
    Blt_HashTable *tablePtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    tablePtr = (Blt_HashTable *)objPtr->internalRep.otherValuePtr;
    Tcl_DStringInit(&ds);
    for (hPtr = Blt_FirstHashEntry(tablePtr, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        Tcl_Obj *elemObjPtr;

        elemObjPtr = Blt_GetHashValue(hPtr);
        Tcl_DStringAppendElement(&ds, Blt_GetHashKey(tablePtr, hPtr));
        Tcl_DStringAppendElement(&ds, Tcl_GetString(elemObjPtr));
    }
    objPtr->bytes = (char *)Blt_AssertStrdup(Tcl_DStringValue(&ds));
    objPtr->length = strlen(Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
}

static void
ArrayObjFreeInternalRep(Tcl_Obj *objPtr)   /* Array object to release. */
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Blt_HashTable *tablePtr;
    
    Tcl_InvalidateStringRep(objPtr);
    tablePtr = (Blt_HashTable *)objPtr->internalRep.otherValuePtr;
    for (hPtr = Blt_FirstHashEntry(tablePtr, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        Tcl_Obj *elemObjPtr;

        elemObjPtr = Blt_GetHashValue(hPtr);
        Tcl_DecrRefCount(elemObjPtr);
    }
    Blt_DeleteHashTable(tablePtr);
    Blt_Free(tablePtr);
}

int
Blt_GetArrayFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                    Blt_HashTable **tablePtrPtr)
{
    if (objPtr->typePtr == &bltArrayObjType) {
        *tablePtrPtr = (Blt_HashTable *)objPtr->internalRep.otherValuePtr;
        return TCL_OK;
    }
    if (ArrayObjSetFromAny(interp, objPtr) == TCL_OK) {
        *tablePtrPtr = (Blt_HashTable *)objPtr->internalRep.otherValuePtr;
        return TCL_OK;
    }
    return TCL_ERROR;
}
    
Tcl_Obj *
Blt_NewArrayObj(int objc, Tcl_Obj **objv)
{
    Blt_HashTable *tablePtr;
    Tcl_Obj *arrayObjPtr;
    int i;

    tablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
    Blt_InitHashTable(tablePtr, BLT_STRING_KEYS);

    for (i = 0; i < objc; i += 2) {
        Blt_HashEntry *hPtr;
        Tcl_Obj *objPtr;
        int isNew;

        hPtr = Blt_CreateHashEntry(tablePtr, Tcl_GetString(objv[i]), &isNew);
        objPtr = ((i + 1) == objc) ? Tcl_NewStringObj("", -1) : objv[i+1];
        Tcl_IncrRefCount(objPtr);
        if (!isNew) {
            Tcl_Obj *oldObjPtr;

            oldObjPtr = Blt_GetHashValue(hPtr);
            Tcl_DecrRefCount(oldObjPtr);
        }
        Blt_SetHashValue(hPtr, objPtr);
    }
    arrayObjPtr = Tcl_NewObj(); 
    /* 
     * Reference counts for entry objects are initialized to 0. They are
     * incremented as they are inserted into the tree via the
     * Blt_Tree_SetValue call.
     */
    arrayObjPtr->refCount = 0;  
    arrayObjPtr->internalRep.otherValuePtr = (VOID *)tablePtr;
    arrayObjPtr->bytes = NULL;
    arrayObjPtr->length = 0; 
    arrayObjPtr->typePtr = &bltArrayObjType;
    return arrayObjPtr;
}

int
Blt_IsArrayObj(Tcl_Obj *objPtr)
{
    return (objPtr->typePtr == &bltArrayObjType);
}

int
Blt_GetLong(Tcl_Interp *interp, const char *string, long *valuePtr)
{
    char *end;
    const char *p;
    long x;
    
    /*
     * Note: don't depend on strtoul to handle sign characters; it won't
     * in some implementations.
     */
    p = string;
    errno = 0;
    while (isspace(UCHAR(*p))) {
        p++;                            /* Skip leading spaces. */
    }
    if (*p == '-') {
        p++;
        x = -(long)strtoul(p, &end, 10); /* INTL: TCL source. */
    } else if (*p == '+') {
        p++;
        x = strtoul(p, &end, 10);       /* INTL: TCL source. */
    } else {
        x = strtoul(p, &end, 10);       /* INTL: TCL source. */
    }
    if (end == p) {
        badInteger:
        if (interp != NULL) {
            Tcl_AppendResult(interp, "expected integer but got \"", p,
                    "\"", (char *) NULL);
        }
        return TCL_ERROR;
    }
    if (errno == ERANGE) {
        if (interp != NULL) {
            Tcl_SetResult(interp,
                        (char *)"long integer value too large to represent",
                          TCL_STATIC);
            Tcl_SetErrorCode(interp, "ARITH", "IOVERFLOW",
                    Tcl_GetStringResult(interp), (char *) NULL);
        }
        return TCL_ERROR;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) { /* INTL: ISO space. */
        end++;
    }
    if (*end != 0) {
        goto badInteger;
    }
    *valuePtr = x;
    return TCL_OK;
}

static int
LongObjSetFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    long x;
    
    if (objPtr->typePtr == &bltLongObjType) {
        return TCL_OK;
    }
    if (Blt_GetLong(interp, Tcl_GetString(objPtr), &x) != TCL_OK) {
        return TCL_ERROR;
    }
    FreeInternalRep(objPtr);
    objPtr->typePtr = &bltLongObjType;
    objPtr->internalRep.longValue = x;
    return TCL_OK;
}

static void
LongObjUpdateStringRep(Tcl_Obj *objPtr)
{
    size_t numBytes;
    char buffer[TCL_INTEGER_SPACE];

    numBytes = sprintf(buffer, "%ld", objPtr->internalRep.longValue);
    objPtr->bytes = ckalloc(numBytes + 1);
    strcpy(objPtr->bytes, buffer);
    objPtr->length = (int)numBytes;
}

int
Blt_GetLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, long *valuePtr)
{
    if (objPtr->typePtr == &bltLongObjType) {
        *valuePtr = objPtr->internalRep.longValue;
        return TCL_OK;
    }
    if (LongObjSetFromAny(interp, objPtr) == TCL_OK) {
        *valuePtr = objPtr->internalRep.longValue;
        return TCL_OK;
    }
    return TCL_ERROR;
}
    
Tcl_Obj *
Blt_NewLongObj(long value)
{
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewObj(); 
    objPtr->refCount = 0;  
    objPtr->internalRep.longValue = value;
    objPtr->bytes = NULL;
    objPtr->length = 0; 
    objPtr->typePtr = &bltLongObjType;
    return objPtr;
}

int
Blt_SetLongObj(Tcl_Obj *objPtr, long value)
{
    if (Tcl_IsShared(objPtr)) {
	Blt_Panic("Blt_SetLongObj called with shared object %p", objPtr);
    }
    Tcl_InvalidateStringRep(objPtr);
    FreeInternalRep(objPtr);
    objPtr->internalRep.longValue = value;
    objPtr->typePtr = &bltLongObjType;
    return TCL_OK;
}

int
Blt_IsLongObj(Tcl_Obj *objPtr)
{
    return (objPtr->typePtr == &bltLongObjType);
}

int
Blt_GetUnsignedLong(Tcl_Interp *interp, const char *string,
                    unsigned long *valuePtr)
{
    char *end;
    const char *p;
    unsigned long x;
    
    /*
     * Note: don't depend on strtoul to handle sign characters; it won't
     * in some implementations.
     */
    p = string;
    errno = 0;
    while (isspace(UCHAR(*p))) {
        p++;                            /* Skip leading spaces. */
    }
    x = strtoul(p, &end, 10);       /* INTL: TCL source. */
    if (end == p) {
        badInteger:
        if (interp != NULL) {
            Tcl_AppendResult(interp, "expected integer but got \"", p,
                    "\"", (char *) NULL);
        }
        return TCL_ERROR;
    }
    if (errno == ERANGE) {
        if (interp != NULL) {
            Tcl_SetResult(interp,
                        (char *)"long integer value too large to represent",
                          TCL_STATIC);
            Tcl_SetErrorCode(interp, "ARITH", "IOVERFLOW",
                    Tcl_GetStringResult(interp), (char *) NULL);
        }
        return TCL_ERROR;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) { /* INTL: ISO space. */
        end++;
    }
    if (*end != 0) {
        goto badInteger;
    }
    *valuePtr = x;
    return TCL_OK;
}

static int
UnsignedLongObjSetFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    unsigned long x;
    
    if (objPtr->typePtr == &bltUnsignedLongObjType) {
        return TCL_OK;
    }
    if (Blt_GetUnsignedLong(interp, Tcl_GetString(objPtr), &x) != TCL_OK) {
        return TCL_ERROR;
    }
    FreeInternalRep(objPtr);
    objPtr->internalRep.longValue = x;
    objPtr->typePtr = &bltUnsignedLongObjType;
    return TCL_OK;
}

static void
UnsignedLongObjUpdateStringRep(Tcl_Obj *objPtr) 
{
    size_t numBytes;
    char buffer[TCL_INTEGER_SPACE];

    numBytes = sprintf(buffer, "%ld",
        (unsigned long)objPtr->internalRep.longValue);
    objPtr->bytes = ckalloc(numBytes + 1);
    strcpy(objPtr->bytes, buffer);
    objPtr->length = (int)numBytes;
}

int
Blt_GetUnsignedLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                           unsigned long *valuePtr)
{
    if (objPtr->typePtr == &bltUnsignedLongObjType) {
        *valuePtr = (unsigned long)objPtr->internalRep.longValue;
        return TCL_OK;
    }
    if (UnsignedLongObjSetFromAny(interp, objPtr) == TCL_OK) {
        *valuePtr = (unsigned long)objPtr->internalRep.longValue;
        return TCL_OK;
    }
    return TCL_ERROR;
}
    
Tcl_Obj *
Blt_NewUnsignedLongObj(unsigned long value)
{
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewObj(); 
    objPtr->refCount = 0;  
    objPtr->internalRep.longValue = value;
    objPtr->bytes = NULL;
    objPtr->length = 0; 
    objPtr->typePtr = &bltUnsignedLongObjType;
    return objPtr;
}

int
Blt_SetUnsignedLongObj(Tcl_Obj *objPtr, unsigned long value)
{
    if (Tcl_IsShared(objPtr)) {
	Blt_Panic("Blt_SetUnsignedLongObj called with shared object %p", objPtr);
    }
    Tcl_InvalidateStringRep(objPtr);
    objPtr->internalRep.longValue = (unsigned long)value;
    objPtr->typePtr = &bltUnsignedLongObjType;
    return TCL_OK;
}

int
Blt_IsUnsignedLongObj(Tcl_Obj *objPtr)
{
    return (objPtr->typePtr == &bltUnsignedLongObjType);
}

int
Blt_GetInt64(Tcl_Interp *interp, const char *string, int64_t *valuePtr)
{
    char *end;
    const char *p;
    int64_t x;
    
#if (SIZEOF_VOID_P == 8) && (SIZEOF_LONG == 4)
#    define STRTOUL strtoull
#else 
#    define STRTOUL strtoul
#endif
    /*
     * Note: don't depend on strtoul to handle sign characters; it won't
     * in some implementations.
     */
    p = string;
    errno = 0;
    while (isspace(UCHAR(*p))) {
        p++;                            /* Skip leading spaces. */
    }
    if (*p == '-') {
        p++;
        x = -(int64_t)STRTOUL(p, &end, 10); /* INTL: TCL source. */
    } else if (*p == '+') {
        p++;
        x = STRTOUL(p, &end, 10);       /* INTL: TCL source. */
    } else {
        x = STRTOUL(p, &end, 10);       /* INTL: TCL source. */
    }
    if (end == p) {
        badInteger:
        if (interp != NULL) {
            Tcl_AppendResult(interp, "expected integer but got \"", p,
                    "\"", (char *) NULL);
        }
        return TCL_ERROR;
    }
    if (errno == ERANGE) {
        if (interp != NULL) {
            Tcl_SetResult(interp,
                        (char *)"long integer value too large to represent",
                          TCL_STATIC);
            Tcl_SetErrorCode(interp, "ARITH", "IOVERFLOW",
                    Tcl_GetStringResult(interp), (char *) NULL);
        }
        return TCL_ERROR;
    }
    while ((*end != '\0') && isspace(UCHAR(*end))) { /* INTL: ISO space. */
        end++;
    }
    if (*end != 0) {
        goto badInteger;
    }
    *valuePtr = x;
    return TCL_OK;
}

static int
Int64ObjSetFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    int64_t x;
    
    if (objPtr->typePtr == &bltInt64ObjType) {
        return TCL_OK;
    }
    if (Blt_GetInt64(interp, Tcl_GetString(objPtr), &x) != TCL_OK) {
        return TCL_ERROR;
    }
    FreeInternalRep(objPtr);
    objPtr->internalRep.wideValue = x;
    objPtr->typePtr = &bltInt64ObjType;
    return TCL_OK;
}

static void
Int64ObjUpdateStringRep(Tcl_Obj *objPtr) 
{
    size_t numBytes;
    char buffer[TCL_INTEGER_SPACE];

    numBytes = sprintf(buffer, "%" PRId64,
                       (int64_t)objPtr->internalRep.wideValue);
    objPtr->bytes = ckalloc(numBytes + 1);
    strcpy(objPtr->bytes, buffer);
    objPtr->length = (int)numBytes;
}

int
Blt_GetInt64FromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int64_t *valuePtr)
{
    if (objPtr->typePtr == &bltInt64ObjType) {
        *valuePtr = (int64_t)objPtr->internalRep.wideValue;
        return TCL_OK;
    }
    if (Int64ObjSetFromAny(interp, objPtr) == TCL_OK) {
        *valuePtr = (int64_t)objPtr->internalRep.wideValue;
        return TCL_OK;
    }
    return TCL_ERROR;
}
    
Tcl_Obj *
Blt_NewInt64Obj(int64_t value)
{
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewObj(); 
    objPtr->refCount = 0;  
    objPtr->internalRep.wideValue = value;
    objPtr->bytes = NULL;
    objPtr->length = 0; 
    objPtr->typePtr = &bltInt64ObjType;
    return objPtr;
}

int
Blt_SetInt64Obj(Tcl_Obj *objPtr, int64_t value)
{
    if (Tcl_IsShared(objPtr)) {
	Blt_Panic("Blt_SetInt64Obj called with shared object %p", objPtr);
    }
    Tcl_InvalidateStringRep(objPtr);
    objPtr->internalRep.wideValue = value;
    objPtr->typePtr = &bltInt64ObjType;
    return TCL_OK;
}

int
Blt_IsInt64Obj(Tcl_Obj *objPtr)
{
    return (objPtr->typePtr == &bltInt64ObjType);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetDouble --
 *
 *      Converts a string into a double precision number.  This differs
 *      from Tcl's version in that it also allows NaN and +/-Inf.  There
 *      are cases where NaNs are used to indicate holes in the data.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetDouble(Tcl_Interp *interp, const char *s, double *valuePtr)
{
    char *end;
    double d;
    
    errno = 0;
    d = strtod(s, &end); /* INTL: TCL source. */
    if (end == s) {
        badDouble:
        if (interp != NULL) {
            Tcl_AppendResult(interp, "expected floating-point number "
                "but got \"", s, "\"", (char *) NULL);
        }
        return TCL_ERROR;
    }
    if (errno != 0 && (d == HUGE_VAL || d == -HUGE_VAL || d == 0)) {
        if (interp != NULL) {
            char msg[64 + TCL_INTEGER_SPACE];
        
            sprintf(msg, "unknown floating-point error, errno = %d", errno);
            Tcl_AppendToObj(Tcl_GetObjResult(interp), msg, -1);
            Tcl_SetErrorCode(interp, "ARITH", "UNKNOWN", msg, (char *) NULL);
        }
        return TCL_ERROR;
    }
    while ((*end != 0) && isspace(UCHAR(*end))) { /* INTL: ISO space. */
        end++;
    }
    if (*end != 0) {
        goto badDouble;
    }
    *valuePtr = d;
    return TCL_OK;
}

static int
DoubleObjSetFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    double x;
    
    if (objPtr->typePtr == &bltDoubleObjType) {
        return TCL_OK;
    }
    if (objPtr->typePtr == &bltLongObjType) {
        objPtr->internalRep.doubleValue = objPtr->internalRep.longValue;
        objPtr->typePtr = &bltDoubleObjType;
        return TCL_OK;
    }
    if (objPtr->typePtr == &bltInt64ObjType) {
        objPtr->internalRep.doubleValue = objPtr->internalRep.wideValue;
        objPtr->typePtr = &bltDoubleObjType;
        return TCL_OK;
    }
    if (Blt_GetDouble(interp, Tcl_GetString(objPtr), &x) != TCL_OK) {
        return TCL_ERROR;
    }
    FreeInternalRep(objPtr);
    objPtr->internalRep.doubleValue = x;
    objPtr->typePtr = &bltDoubleObjType;
    return TCL_OK;
}

static void
DoubleObjUpdateStringRep(Tcl_Obj *objPtr) 
{
    size_t numBytes;
    char buffer[TCL_DOUBLE_SPACE];

    Tcl_PrintDouble(NULL, objPtr->internalRep.doubleValue, buffer);
    numBytes = strlen(buffer);
    strcpy(objPtr->bytes, buffer);
    objPtr->length = (int)numBytes;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetDoubleFromObj --
 *
 *      Converts a Tcl_Obj into a double precision number.  This differs
 *      from Tcl's version in that it also allows NaN and +/-Inf.  There
 *      are cases where NaNs are used to indicate holes in the data.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 * Side Effects:
 *      tclDoubleType is no longer available (in 8.5) as a global variable.
 *      We have to get a double obj and save its type pointer.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetDoubleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, double *valuePtr)
{
    if (objPtr->typePtr == &bltDoubleObjType) {
        *valuePtr = objPtr->internalRep.doubleValue;
        return TCL_OK;
    }
    if (objPtr->typePtr == tclDoubleObjTypePtr) {
        *valuePtr = objPtr->internalRep.doubleValue;
        return TCL_OK;
    }
    if (DoubleObjSetFromAny(interp, objPtr) == TCL_OK) {
        *valuePtr = objPtr->internalRep.doubleValue;
        return TCL_OK;
    }
    return TCL_ERROR;
}

Tcl_Obj *
Blt_NewDoubleObj(double value)
{
    Tcl_Obj *objPtr;

    objPtr = Tcl_NewObj(); 
    objPtr->refCount = 0;  
    objPtr->internalRep.doubleValue = value;
    objPtr->bytes = NULL;
    objPtr->length = 0; 
    objPtr->typePtr = &bltDoubleObjType;
    return objPtr;
}

int
Blt_SetDoubleObj(Tcl_Obj *objPtr, double value)
{
    if (Tcl_IsShared(objPtr)) {
	Blt_Panic("Blt_SetDoubleObj called with shared object %p", objPtr);
    }
    Tcl_InvalidateStringRep(objPtr);
    objPtr->internalRep.doubleValue = value;
    objPtr->typePtr = &bltDoubleObjType;
    return TCL_OK;
}

int
Blt_IsDoubleObj(Tcl_Obj *objPtr)
{
    return (objPtr->typePtr == &bltDoubleObjType);
}

void
Blt_RegisterObjTypes(void)
{
    tclIntObjTypePtr = Tcl_GetObjType("int");
    tclDoubleObjTypePtr = Tcl_GetObjType("double");
    tclWideIntObjTypePtr = Tcl_GetObjType("wideInt");
    Tcl_RegisterObjType(&bltInt64ObjType);
    Tcl_RegisterObjType(&bltArrayObjType);
    Tcl_RegisterObjType(&bltLongObjType);
    Tcl_RegisterObjType(&bltDoubleObjType);
    Tcl_RegisterObjType(&bltUnsignedLongObjType);
}



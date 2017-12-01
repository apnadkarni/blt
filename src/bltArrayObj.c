/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltArrayObj.c --
 *
 * This file implements an array-based Tcl_Obj.
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

static Tcl_DupInternalRepProc DupArrayInternalRep;
static Tcl_FreeInternalRepProc FreeArrayInternalRep;
static Tcl_UpdateStringProc UpdateStringOfArray;
static Tcl_SetFromAnyProc SetArrayFromAny;

static Tcl_ObjType arrayObjType = {
    (char *)"array",
    FreeArrayInternalRep,               /* Called when an object is
                                         * freed. */
    DupArrayInternalRep,                /* Copies an internal
                                         * representation from one object
                                         * to another. */
    UpdateStringOfArray,                /* Creates string representation
                                         * from an object's internal
                                         * representation. */
    SetArrayFromAny,                    /* Creates valid internal
                                         * representation from an object's
                                         * string representation. */
};

static int
SetArrayFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Blt_HashTable *tablePtr;
    const Tcl_ObjType *oldTypePtr = objPtr->typePtr;
    Tcl_Obj **objv;
    int objc, i;

    if (objPtr->typePtr == &arrayObjType) {
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
    if ((oldTypePtr != NULL) && (oldTypePtr->freeIntRepProc != NULL)) {
        oldTypePtr->freeIntRepProc(objPtr);
    }
    objPtr->internalRep.otherValuePtr = tablePtr;
    objPtr->typePtr = &arrayObjType;
    return TCL_OK;
}

static void
DupArrayInternalRep(
    Tcl_Obj *srcPtr,                    /* Object with internal rep to
                                         * copy. */
    Tcl_Obj *destPtr)                   /* Object with internal rep to
                                         * set. */
{
    Blt_HashEntry *hp;
    Blt_HashSearch iter;
    Blt_HashTable *srcTablePtr, *destTablePtr;

    srcTablePtr = (Blt_HashTable *)srcPtr->internalRep.otherValuePtr;
    destTablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
    Blt_InitHashTable(destTablePtr, BLT_STRING_KEYS);
    for (hp = Blt_FirstHashEntry(srcTablePtr, &iter); hp != NULL;
         hp = Blt_NextHashEntry(&iter)) {
        Tcl_Obj *valueObjPtr;
        const char *key;
        int isNew;

        key = Blt_GetHashKey(srcTablePtr, hp);
        Blt_CreateHashEntry(destTablePtr, key, &isNew);
        valueObjPtr = Blt_GetHashValue(hp);
        Blt_SetHashValue(hp, valueObjPtr);

        /* Make sure we increment the reference count now that both array
         * objects are using the same elements. */
        Tcl_IncrRefCount(valueObjPtr);
    }
    Tcl_InvalidateStringRep(destPtr);
    destPtr->internalRep.otherValuePtr = (VOID *)destTablePtr;
    destPtr->typePtr = &arrayObjType;
}

static void
UpdateStringOfArray(Tcl_Obj *objPtr)    /* Array object w/ string rep to
                                           update. */
{
    Tcl_DString ds;
    Blt_HashTable *tablePtr;
    Blt_HashEntry *hp;
    Blt_HashSearch iter;

    tablePtr = (Blt_HashTable *)objPtr->internalRep.otherValuePtr;
    Tcl_DStringInit(&ds);
    for (hp = Blt_FirstHashEntry(tablePtr, &iter); hp != NULL;
         hp = Blt_NextHashEntry(&iter)) {
        Tcl_Obj *elemObjPtr;

        elemObjPtr = Blt_GetHashValue(hp);
        Tcl_DStringAppendElement(&ds, Blt_GetHashKey(tablePtr, hp));
        Tcl_DStringAppendElement(&ds, Tcl_GetString(elemObjPtr));
    }
    objPtr->bytes = (char *)Blt_AssertStrdup(Tcl_DStringValue(&ds));
    objPtr->length = strlen(Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
}

static void
FreeArrayInternalRep(Tcl_Obj *objPtr)   /* Array object to release. */
{
    Blt_HashEntry *hp;
    Blt_HashSearch iter;
    Blt_HashTable *tablePtr;
    
    Tcl_InvalidateStringRep(objPtr);
    tablePtr = (Blt_HashTable *)objPtr->internalRep.otherValuePtr;
    for (hp = Blt_FirstHashEntry(tablePtr, &iter); hp != NULL;
         hp = Blt_NextHashEntry(&iter)) {
        Tcl_Obj *elemObjPtr;

        elemObjPtr = Blt_GetHashValue(hp);
        Tcl_DecrRefCount(elemObjPtr);
    }
    Blt_DeleteHashTable(tablePtr);
    Blt_Free(tablePtr);
}

int
Blt_GetArrayFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                    Blt_HashTable **tablePtrPtr)
{
    if (objPtr->typePtr == &arrayObjType) {
        *tablePtrPtr = (Blt_HashTable *)objPtr->internalRep.otherValuePtr;
        return TCL_OK;
    }
    if (SetArrayFromAny(interp, objPtr) == TCL_OK) {
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
        Blt_HashEntry *hp;
        Tcl_Obj *objPtr;
        int isNew;

        hp = Blt_CreateHashEntry(tablePtr, Tcl_GetString(objv[i]), &isNew);
        objPtr = ((i + 1) == objc) ? Tcl_NewStringObj("", -1) : objv[i+1];
        Tcl_IncrRefCount(objPtr);
        if (!isNew) {
            Tcl_Obj *oldObjPtr;

            oldObjPtr = Blt_GetHashValue(hp);
            Tcl_DecrRefCount(oldObjPtr);
        }
        Blt_SetHashValue(hp, objPtr);
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
    arrayObjPtr->typePtr = &arrayObjType;
    return arrayObjPtr;
}

int
Blt_IsArrayObj(Tcl_Obj *objPtr)
{
    return (objPtr->typePtr == &arrayObjType);
}

/*ARGSUSED*/
void
Blt_RegisterArrayObj(void)
{
    Tcl_RegisterObjType(&arrayObjType);
}


static Tcl_DupInternalRepProc DupLongInternalRep;
static Tcl_FreeInternalRepProc FreeLongInternalRep;
static Tcl_UpdateStringProc UpdateStringOfLong;
static Tcl_SetFromAnyProc SetLongFromAny;

static Tcl_ObjType longObjType = {
    (char *)"long",
    FreeLongInternalRep,                /* Called when an object is
                                         * freed. */
    DupLongInternalRep,                 /* Copies an internal
                                         * representation from one object
                                         * to another. */
    UpdateStringOfLong,                 /* Creates string representation
                                         * from an object's internal
                                         * representation. */
    SetLongFromAny,                     /* Creates valid internal
                                         * representation from an object's
                                         * string representation. */
};

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
SetLongFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    long x;
    
    if (objPtr->typePtr == &longObjType) {
        return TCL_OK;
    }
    if (Blt_GetLong(interp, Tcl_GetString(objPtr), &x) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_SetLongObj(objPtr, x);
    return TCL_OK;
}

static void
DupLongInternalRep(
    Tcl_Obj *srcPtr,                    /* Object with internal rep to
                                         * copy. */
    Tcl_Obj *destPtr)                   /* Object with internal rep to
                                         * set. */
{
    destPtr->internalRep.longValue = srcPtr->internalRep.longValue;
    Tcl_InvalidateStringRep(destPtr);
    destPtr->typePtr = &longObjType;
}

static void
UpdateStringOfLong(Tcl_Obj *objPtr)     /* Array object w/ string rep to
                                           update. */
{
    size_t numBytes;
    char buffer[TCL_INTEGER_SPACE];

    numBytes = sprintf(buffer, "%ld", objPtr->internalRep.longValue);
    objPtr->bytes = ckalloc(numBytes + 1);
    strcpy(objPtr->bytes, buffer);
    objPtr->length = numBytes;
}

static void
FreeLongInternalRep(Tcl_Obj *objPtr)   /* Array object to release. */
{
    Tcl_InvalidateStringRep(objPtr);
}

int
Blt_GetLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, long *valuePtr)
{
    if (objPtr->typePtr == &longObjType) {
        *valuePtr = objPtr->internalRep.longValue;
        return TCL_OK;
    }
    if (SetArrayFromAny(interp, objPtr) == TCL_OK) {
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
    objPtr->typePtr = &longObjType;
    return objPtr;
}

int
Blt_SetLongObj(Tcl_Obj *objPtr, long value)
{
    Tcl_InvalidateStringRep(objPtr);
    objPtr->internalRep.longValue = value;
    objPtr->typePtr = &longObjType;
    return TCL_OK;
}

int
Blt_IsLongObj(Tcl_Obj *objPtr)
{
    return (objPtr->typePtr == &longObjType);
}

/*ARGSUSED*/
void
Blt_RegisterLongObj(void)
{
    Tcl_RegisterObjType(&longObjType);
}


static Tcl_DupInternalRepProc DupUnsignedLongInternalRep;
static Tcl_FreeInternalRepProc FreeUnsignedLongInternalRep;
static Tcl_UpdateStringProc UpdateStringOfUnsignedLong;
static Tcl_SetFromAnyProc SetUnsignedLongFromAny;

static Tcl_ObjType unsignedLongObjType = {
    (char *)"unsigned long",
    FreeUnsignedLongInternalRep,        /* Called when an object is
                                         * freed. */
    DupUnsignedLongInternalRep,         /* Copies an internal
                                         * representation from one object
                                         * to another. */
    UpdateStringOfUnsignedLong,         /* Creates string representation
                                         * from an object's internal
                                         * representation. */
    SetUnsignedLongFromAny,             /* Creates valid internal
                                         * representation from an object's
                                         * string representation. */
};

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
SetUnsignedLongFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    unsigned long x;
    
    if (objPtr->typePtr == &unsignedLongObjType) {
        return TCL_OK;
    }
    if (Blt_GetUnsignedLong(interp, Tcl_GetString(objPtr), &x) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_SetUnsignedLongObj(objPtr, x);
    return TCL_OK;
}

static void
DupUnsignedLongInternalRep(
    Tcl_Obj *srcPtr,                    /* Object with internal rep to
                                         * copy. */
    Tcl_Obj *destPtr)                   /* Object with internal rep to
                                         * set. */
{
    destPtr->internalRep.longValue =
        (unsigned long)srcPtr->internalRep.longValue;
    Tcl_InvalidateStringRep(destPtr);
    destPtr->typePtr = &unsignedLongObjType;
}

static void
UpdateStringOfUnsignedLong(Tcl_Obj *objPtr) /* Array object w/ string rep to
                                           update. */
{
    size_t numBytes;
    char buffer[TCL_INTEGER_SPACE];

    numBytes = sprintf(buffer, "%ld",
        (unsigned long)objPtr->internalRep.longValue);
    objPtr->bytes = ckalloc(numBytes + 1);
    strcpy(objPtr->bytes, buffer);
    objPtr->length = numBytes;
}

static void
FreeUnsignedLongInternalRep(Tcl_Obj *objPtr) /* Array object to release. */
{
    Tcl_InvalidateStringRep(objPtr);
}

int
Blt_GetUnsignedLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                           unsigned long *valuePtr)
{
    if (objPtr->typePtr == &unsignedLongObjType) {
        *valuePtr = (unsigned long)objPtr->internalRep.longValue;
        return TCL_OK;
    }
    if (SetArrayFromAny(interp, objPtr) == TCL_OK) {
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
    objPtr->typePtr = &unsignedLongObjType;
    return objPtr;
}

int
Blt_SetUnsignedLongObj(Tcl_Obj *objPtr, unsigned long value)
{
    Tcl_InvalidateStringRep(objPtr);
    objPtr->internalRep.longValue = (unsigned long)value;
    objPtr->typePtr = &unsignedLongObjType;
    return TCL_OK;
}

int
Blt_IsUnsignedLongObj(Tcl_Obj *objPtr)
{
    return (objPtr->typePtr == &unsignedLongObjType);
}

/*ARGSUSED*/
void
Blt_RegisterUnsignedLongObj(void)
{
    Tcl_RegisterObjType(&unsignedLongObjType);
}


static Tcl_DupInternalRepProc DupInt64InternalRep;
static Tcl_FreeInternalRepProc FreeInt64InternalRep;
static Tcl_UpdateStringProc UpdateStringOfInt64;
static Tcl_SetFromAnyProc SetInt64FromAny;

static Tcl_ObjType int64ObjType = {
    (char *)"int64",
    FreeInt64InternalRep,                /* Called when an object is
                                          * freed. */
    DupInt64InternalRep,                 /* Copies an internal
                                          * representation from one object
                                         * to another. */
    UpdateStringOfInt64,                 /* Creates string representation
                                          * from an object's internal
                                          * representation. */
    SetInt64FromAny,                     /* Creates valid internal
                                          * representation from an object's
                                          * string representation. */
};

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
SetInt64FromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    int64_t x;
    
    if (objPtr->typePtr == &int64ObjType) {
        return TCL_OK;
    }
    if (Blt_GetInt64(interp, Tcl_GetString(objPtr), &x) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_SetInt64Obj(objPtr, x);
    return TCL_OK;
}

static void
DupInt64InternalRep(
    Tcl_Obj *srcPtr,                    /* Object with internal rep to
                                         * copy. */
    Tcl_Obj *destPtr)                   /* Object with internal rep to
                                         * set. */
{
    destPtr->internalRep.wideValue = srcPtr->internalRep.wideValue;
    Tcl_InvalidateStringRep(destPtr);
    destPtr->typePtr = &int64ObjType;
}

static void
UpdateStringOfInt64(Tcl_Obj *objPtr)     /* Array object w/ string rep to
                                           update. */
{
    size_t numBytes;
    char buffer[TCL_INTEGER_SPACE];

    numBytes = sprintf(buffer, "%I64d", (int64_t)objPtr->internalRep.wideValue);
    objPtr->bytes = ckalloc(numBytes + 1);
    strcpy(objPtr->bytes, buffer);
    objPtr->length = numBytes;
}

static void
FreeInt64InternalRep(Tcl_Obj *objPtr)   /* Array object to release. */
{
    Tcl_InvalidateStringRep(objPtr);
}

int
Blt_GetInt64FromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, long *valuePtr)
{
    if (objPtr->typePtr == &int64ObjType) {
        *valuePtr = (int64_t)objPtr->internalRep.wideValue;
        return TCL_OK;
    }
    if (SetArrayFromAny(interp, objPtr) == TCL_OK) {
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
    objPtr->typePtr = &int64ObjType;
    return objPtr;
}

int
Blt_SetInt64Obj(Tcl_Obj *objPtr, int64_t value)
{
    Tcl_InvalidateStringRep(objPtr);
    objPtr->internalRep.wideValue = value;
    objPtr->typePtr = &int64ObjType;
    return TCL_OK;
}

int
Blt_IsInt64Obj(Tcl_Obj *objPtr)
{
    return (objPtr->typePtr == &int64ObjType);
}

/*ARGSUSED*/
void
Blt_RegisterInt64Obj(void)
{
    Tcl_RegisterObjType(&int64ObjType);
}


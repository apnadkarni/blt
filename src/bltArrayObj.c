/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltArrayObj.c --
 *
 * This file implements an array-based Tcl_Obj.
 *
 *	Copyright (c) 2000 George A. Howlett
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use, copy,
 *	modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"
#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */
#include <bltAlloc.h>
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
    const char **argv, *string;
    int argc, i;

    if (objPtr->typePtr == &arrayObjType) {
	return TCL_OK;
    }
    /* Get the string representation. Make it up-to-date if necessary. */
    string = Tcl_GetString(objPtr);
    if (Tcl_SplitList(interp, string, &argc, &argv) != TCL_OK) {
	return TCL_ERROR;
    }
    tablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
    Blt_InitHashTable(tablePtr, BLT_STRING_KEYS);
    for (i = 0; i < argc; i += 2) {
	Blt_HashEntry *hPtr;
	Tcl_Obj *elemObjPtr;
	int isNew;

	hPtr = Blt_CreateHashEntry(tablePtr, argv[i], &isNew);
	elemObjPtr = Tcl_NewStringObj(argv[i + 1], -1);
	Blt_SetHashValue(hPtr, elemObjPtr);

	/* Make sure we increment the reference count */
	Tcl_IncrRefCount(elemObjPtr);
    }
    
    if ((oldTypePtr != NULL) && (oldTypePtr->freeIntRepProc != NULL)) {
	oldTypePtr->freeIntRepProc(objPtr);
    }
    objPtr->internalRep.otherValuePtr = tablePtr;
    objPtr->typePtr = &arrayObjType;
    Blt_Free(argv);
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
Blt_GetArrayFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
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

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltVecCmd.c --
 *
 * This module implements vector data objects.
 *
 *	Copyright 1995-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Code for binary data read operation was donated by Harold Kirsch.
 *
 */

/*
 * TODO:
 *	o Add H. Kirsch's vector binary read operation
 *		x binread file0
 *		x binread -file file0
 *
 *	o Add ASCII/binary file reader
 *		x read fileName
 *
 *	o Allow Tcl-based client notifications.
 *		vector x
 *		x notify call Display
 *		x notify delete Display
 *		x notify reorder #1 #2
 */

#include "bltVecInt.h"
#undef SIGN
#include "tclTomMath.h"
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltOp.h"
#include "bltNsUtil.h"
#include "bltSwitch.h"

typedef int (VectorCmdProc)(Vector *vPtr, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv);

static Blt_SwitchParseProc ObjToFFTVector;
static Blt_SwitchCustom fftVectorSwitch = {
    ObjToFFTVector, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToIndex;
static Blt_SwitchCustom indexSwitch = {
    ObjToIndex, NULL, NULL, (ClientData)0
};

typedef struct {
    Tcl_Obj *formatObjPtr;
    int from, to;
    int empty;
} ValuesSwitches;

static Blt_SwitchSpec valuesSwitches[] = 
{
    {BLT_SWITCH_OBJ,    "-format", "string", (char *)NULL,
	Blt_Offset(ValuesSwitches, formatObjPtr), 0},
    {BLT_SWITCH_CUSTOM, "-from",   "index", (char *)NULL,
	Blt_Offset(ValuesSwitches, from),         0, 0, &indexSwitch},
    {BLT_SWITCH_CUSTOM, "-to",     "index", (char *)NULL,
	Blt_Offset(ValuesSwitches, to),           0, 0, &indexSwitch},
    {BLT_SWITCH_BOOLEAN, "-empty", "bool", (char *)NULL,
	Blt_Offset(ValuesSwitches, empty),           0, 0},
    {BLT_SWITCH_END}
};

typedef struct {
    int from, to;
    int empty;
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
} ExportSwitches;

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_OBJ,    "-data",   "data", (char *)NULL,
	Blt_Offset(ExportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_OBJ,    "-file",   "fileName", (char *)NULL,
	Blt_Offset(ExportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_CUSTOM, "-from",   "index", (char *)NULL,
	Blt_Offset(ExportSwitches, from),         0, 0, &indexSwitch},
    {BLT_SWITCH_CUSTOM, "-to",     "index", (char *)NULL,
	Blt_Offset(ExportSwitches, to),           0, 0, &indexSwitch},
    {BLT_SWITCH_DOUBLE, "-empty", "value", (char *)NULL,
	Blt_Offset(ExportSwitches, empty),       0, 0},
    {BLT_SWITCH_END}
};

typedef struct {
    int from, to;
} PrintSwitches;

static Blt_SwitchSpec printSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-from",   "index", (char *)NULL,
	Blt_Offset(PrintSwitches, from),         0, 0, &indexSwitch},
    {BLT_SWITCH_CUSTOM, "-to",     "index", (char *)NULL,
	Blt_Offset(PrintSwitches, to),           0, 0, &indexSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    int flags;
} SortSwitches;

#define SORT_DECREASING (1<<0)
#define SORT_UNIQUE	(1<<1)
#define SORT_INDICES	(1<<2)

static Blt_SwitchSpec sortSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-decreasing", "", (char *)NULL,
	Blt_Offset(SortSwitches, flags), 0, SORT_DECREASING},
    {BLT_SWITCH_BITMASK, "-reverse", "", (char *)NULL,
	Blt_Offset(SortSwitches, flags), 0, SORT_DECREASING},
    {BLT_SWITCH_BITMASK, "-uniq", "", (char *)NULL,
	Blt_Offset(SortSwitches, flags), 0, SORT_UNIQUE},
    {BLT_SWITCH_BITMASK, "-indices", "", (char *)NULL,
	Blt_Offset(SortSwitches, flags), 0, SORT_INDICES},
    {BLT_SWITCH_END}
};

typedef struct {
    double delta;
    Vector *imagPtr;		        /* Vector containing imaginary
					 * part. */
    Vector *freqPtr;			/* Vector containing frequencies. */
    VectorInterpData *dataPtr;
    int mask;				/* Flags controlling FFT. */
} FFTData;


static Blt_SwitchSpec fftSwitches[] = {
    {BLT_SWITCH_CUSTOM, "-imagpart",    "vector", (char *)NULL,
	Blt_Offset(FFTData, imagPtr), 0, 0, &fftVectorSwitch},
    {BLT_SWITCH_BITMASK, "-noconstant", "", (char *)NULL,
	Blt_Offset(FFTData, mask), 0, FFT_NO_CONSTANT},
    {BLT_SWITCH_BITMASK, "-spectrum", "", (char *)NULL,
	  Blt_Offset(FFTData, mask), 0, FFT_SPECTRUM},
    {BLT_SWITCH_BITMASK, "-bartlett",  "", (char *)NULL,
	 Blt_Offset(FFTData, mask), 0, FFT_BARTLETT},
    {BLT_SWITCH_DOUBLE, "-delta",   "float", (char *)NULL,
	Blt_Offset(FFTData, mask), 0, 0, },
    {BLT_SWITCH_CUSTOM, "-frequencies", "vector", (char *)NULL,
	Blt_Offset(FFTData, freqPtr), 0, 0, &fftVectorSwitch},
    {BLT_SWITCH_END}
};

/*
 *---------------------------------------------------------------------------
 *
 * ObjToFFTVector --
 *
 *	Convert a string representing a vector into its vector structure.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToFFTVector(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to report results */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* Name of vector. */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    FFTData *dataPtr = (FFTData *)record;
    Vector *vPtr;
    Vector **vPtrPtr = (Vector **)(record + offset);
    int isNew;				/* Not used. */
    char *string;

    string = Tcl_GetString(objPtr);
    vPtr = Blt_Vec_Create(dataPtr->dataPtr, string, string, string, &isNew);
    if (vPtr == NULL) {
	return TCL_ERROR;
    }
    *vPtrPtr = vPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToIndex --
 *
 *	Convert a string representing a vector into its vector structure.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToIndex(
    ClientData clientData,		/* Contains the vector in question to
					 * verify its length. */
    Tcl_Interp *interp,			/* Interpreter to report results */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* Name of vector. */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Vector *vPtr = clientData;
    int *indexPtr = (int *)(record + offset);
    int index;

    if (Blt_Vec_GetIndex(interp, vPtr, Tcl_GetString(objPtr), &index, 
	INDEX_CHECK, (Blt_VectorIndexProc **)NULL) != TCL_OK) {
	return TCL_ERROR;
    }
    *indexPtr = index;
    return TCL_OK;

}

static Tcl_Obj *
GetValues(Vector *vPtr, int first, int last)
{ 
    Tcl_Obj *listObjPtr;
    double *vp, *vend;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (vp = vPtr->valueArr + first, vend = vPtr->valueArr + last; vp <= vend;
	vp++) { 
	Tcl_ListObjAppendElement(vPtr->interp, listObjPtr, 
		Tcl_NewDoubleObj(*vp));
    } 
    return listObjPtr;
}

static void
ReplicateValue(Vector *vPtr, int first, int last, double value)
{ 
    double *vp, *vend;
 
    for (vp = vPtr->valueArr + first, vend = vPtr->valueArr + last; 
	 vp <= vend; vp++) { 
	*vp = value; 
    } 
    vPtr->notifyFlags |= UPDATE_RANGE; 
}

static int
CopyList(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int i;

    if (Blt_Vec_SetLength(interp, vPtr, objc) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
	double value;

	if (Blt_ExprDoubleFromObj(interp, objv[i], &value) != TCL_OK) {
	    Blt_Vec_SetLength(interp, vPtr, i);
	    return TCL_ERROR;
	}
	vPtr->valueArr[i] = value;
    }
    return TCL_OK;
}

static int
AppendVector(Vector *destPtr, Vector *srcPtr)
{
    size_t numBytes;
    size_t oldSize, newSize;

    oldSize = destPtr->length;
    newSize = oldSize + srcPtr->length;
    if (Blt_Vec_ChangeLength(destPtr->interp, destPtr, newSize) != TCL_OK) {
	return TCL_ERROR;
    }
    numBytes = (newSize - oldSize) * sizeof(double);
    memcpy((char *)(destPtr->valueArr + oldSize), srcPtr->valueArr, numBytes);
    destPtr->notifyFlags |= UPDATE_RANGE;
    return TCL_OK;
}

static int
AppendList(Vector *vPtr, int objc, Tcl_Obj *const *objv)
{
    Tcl_Interp *interp = vPtr->interp;
    int count;
    int i;
    double value;
    int oldSize;

    oldSize = vPtr->length;
    if (Blt_Vec_ChangeLength(interp, vPtr, vPtr->length + objc) != TCL_OK) {
	return TCL_ERROR;
    }
    count = oldSize;
    for (i = 0; i < objc; i++) {
	if (Blt_ExprDoubleFromObj(interp, objv[i], &value) != TCL_OK) {
	    Blt_Vec_ChangeLength(interp, vPtr, count);
	    return TCL_ERROR;
	}
	vPtr->valueArr[count++] = value;
    }
    vPtr->notifyFlags |= UPDATE_RANGE;
    return TCL_OK;
}

/* Vector instance option commands */

/*
 *---------------------------------------------------------------------------
 *
 * AppendOp --
 *
 *	Appends one of more TCL lists of values, or vector objects onto the
 *	end of the current vector object.
 *
 * Results:
 *	A standard TCL result.  If a current vector can't be created, 
 *	resized, any of the named vectors can't be found, or one of lists of
 *	values is invalid, TCL_ERROR is returned.
 *
 * Side Effects:
 *	Clients of current vector will be notified of the change.
 *
 *---------------------------------------------------------------------------
 */
static int
AppendOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int i;
    int result;
    Vector *v2Ptr;

    for (i = 2; i < objc; i++) {
	v2Ptr = Blt_Vec_ParseElement((Tcl_Interp *)NULL, vPtr->dataPtr, 
	       Tcl_GetString(objv[i]), (const char **)NULL, NS_SEARCH_BOTH);
	if (v2Ptr != NULL) {
	    result = AppendVector(vPtr, v2Ptr);
	} else {
	    int numElem;
	    Tcl_Obj **elemObjArr;

	    if (Tcl_ListObjGetElements(interp, objv[i], &numElem, &elemObjArr) 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    result = AppendList(vPtr, numElem, elemObjArr);
	}
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (objc > 2) {
	if (vPtr->flush) {
	    Blt_Vec_FlushCache(vPtr);
	}
	Blt_Vec_UpdateClients(vPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ClearOp --
 *
 *	Deletes all the accumulated array indices for the TCL array associated
 *	will the vector.  This routine can be used to free excess memory from
 *	a large vector.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side Effects:
 *	Memory used for the entries of the TCL array variable is freed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ClearOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_Vec_FlushCache(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *	Deletes the given indices from the vector.  If no indices are provided
 *	the entire vector is deleted.
 *
 * Results:
 *	A standard TCL result.  If any of the given indices is invalid,
 *	interp->result will an error message and TCL_ERROR is returned.
 *
 * Side Effects:
 *	The clients of the vector will be notified of the vector
 *	deletions.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    unsigned char *unsetArr;
    int i, j;
    int count;
    char *string;

    /* FIXME: Don't delete vector with no indices.  */
    if (objc == 2) {
	Blt_Vec_Free(vPtr);
	return TCL_OK;
    }

    /* Allocate an "unset" bitmap the size of the vector. */
    unsetArr = Blt_AssertCalloc(sizeof(unsigned char), (vPtr->length + 7) / 8);
#define SetBit(i) \
    unsetArr[(i) >> 3] |= (1 << ((i) & 0x07))
#define GetBit(i) \
    (unsetArr[(i) >> 3] & (1 << ((i) & 0x07)))

    for (i = 2; i < objc; i++) {
	string = Tcl_GetString(objv[i]);
	if (Blt_Vec_GetIndexRange(interp, vPtr, string, 
		(INDEX_COLON | INDEX_CHECK), (Blt_VectorIndexProc **) NULL) 
		!= TCL_OK) {
	    Blt_Free(unsetArr);
	    return TCL_ERROR;
	}
	for (j = 0; j < vPtr->length; j++) {
	    SetBit(j);			/* Mark the element for deletion. */
	}
    }
    count = 0;
    for (i = 0; i < vPtr->length; i++) {
	if (GetBit(i)) {
	    continue;			/* Skip marked elements. */
	}
	if (count < i) {
	    vPtr->valueArr[count] = vPtr->valueArr[i];
	}
	count++;
    }
    Blt_Free(unsetArr);
    vPtr->length = count;
    if (vPtr->flush) {
	Blt_Vec_FlushCache(vPtr);
    }
    Blt_Vec_UpdateClients(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DupOp --
 *
 *	Creates one or more duplicates of the vector object.
 *
 * Results:
 *	A standard TCL result.  If a new vector can't be created,
 *      or and existing vector resized, TCL_ERROR is returned.
 *
 * Side Effects:
 *	Clients of existing vectors will be notified of the change.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DupOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Vector *v2Ptr;
    const char *name;
    int isNew;

    if (objc == 3) {
	name = Tcl_GetString(objv[2]);
	v2Ptr = Blt_Vec_Create(vPtr->dataPtr, name, name, name, &isNew);
    } else {
	name = "#auto";
	v2Ptr = Blt_Vec_Create(vPtr->dataPtr, name, name, name, &isNew);
    }
    if (v2Ptr == NULL) {
	return TCL_ERROR;
    }
    if (v2Ptr == vPtr) {
	return TCL_OK;
    }
    if (Blt_Vec_Duplicate(v2Ptr, vPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!isNew) {
	if (v2Ptr->flush) {
	    Blt_Vec_FlushCache(v2Ptr);
	}
	Blt_Vec_UpdateClients(v2Ptr);
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), v2Ptr->name, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FrequencyOp --
 *
 *	Fills the destination vector with the frequency counts from the 
 *	source vector.
 *
 * Results:
 *	A standard TCL result.  If a new vector can't be created,
 *      or and existing vector resized, TCL_ERROR is returned.
 *
 *	$v frequency $x 10 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FrequencyOp(Vector *destPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Vector *srcPtr;
    const char *name;
    double range;
    int i, numBins;
    Blt_HashTable freqTable;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    name = Tcl_GetString(objv[2]);
    if (Blt_Vec_LookupName(destPtr->dataPtr, name, &srcPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &numBins) != TCL_OK) {
	return TCL_ERROR;
    }
    if (numBins < 1) {
	Tcl_AppendResult(interp, "bad number of bins \"", 
			 Tcl_GetString(objv[3]), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (Blt_Vec_ChangeLength(destPtr->interp, destPtr, numBins) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 0; i < numBins; i++) {
	destPtr->valueArr[i] = 0.0;
    }
    Blt_InitHashTable(&freqTable, BLT_ONE_WORD_KEYS);
    range = srcPtr->max - srcPtr->min;
    for (i = 0; i < srcPtr->length; i++) {
	double value, norm;
	int isNew;
	long bin;
	Blt_HashEntry *hPtr;
	size_t count;

	value = srcPtr->valueArr[i];
	norm = (value - srcPtr->min) / range;
	bin = (int)round(norm * (numBins - 1));
	hPtr = Blt_CreateHashEntry(&freqTable, (char *)bin, &isNew);
	if (isNew) {
	    count = 1;
	} else {
	    count = (long)Blt_GetHashValue(hPtr);
	    count++;
	}
	Blt_SetHashValue(hPtr, count);
    }				     
    for (i = 0, hPtr = Blt_FirstHashEntry(&freqTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter), i++) {
	long count, index;
	
	count = (long)Blt_GetHashValue(hPtr);
	index = (long)Blt_GetHashKey(&freqTable, hPtr);
	destPtr->valueArr[index] = (double)count;
    }
    Blt_DeleteHashTable(&freqTable);
    Blt_Vec_FlushCache(destPtr);
    Blt_Vec_UpdateClients(destPtr);
    return TCL_OK;
}

/* spinellia@acm.org START */

/* fft implementation */
/*ARGSUSED*/
static int
FFTOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Vector *v2Ptr = NULL;
    int isNew;
    FFTData data;
    char *realVecName;

    memset(&data, 0, sizeof(data));
    data.delta = 1.0;

    realVecName = Tcl_GetString(objv[2]);
    v2Ptr = Blt_Vec_Create(vPtr->dataPtr, realVecName, realVecName, 
	realVecName, &isNew);
    if (v2Ptr == NULL) {
        return TCL_ERROR;
    }
    if (v2Ptr == vPtr) {
	Tcl_AppendResult(interp, "real vector \"", realVecName, "\"", 
		" can't be the same as the source", (char *)NULL);
        return TCL_ERROR;
    }
    if (Blt_ParseSwitches(interp, fftSwitches, objc - 3, objv + 3, &data, 
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (Blt_Vec_FFT(interp, v2Ptr, data.imagPtr, data.freqPtr, data.delta,
	      data.mask, vPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Update bookkeeping. */
    if (!isNew) {
	if (v2Ptr->flush) {
	    Blt_Vec_FlushCache(v2Ptr);
	}
	Blt_Vec_UpdateClients(v2Ptr);
    }
    if (data.imagPtr != NULL) {
        if (data.imagPtr->flush) {
            Blt_Vec_FlushCache(data.imagPtr);
        }
        Blt_Vec_UpdateClients(data.imagPtr);
    }
    if (data.freqPtr != NULL) {
        if (data.freqPtr->flush) {
            Blt_Vec_FlushCache(data.freqPtr);
        }
        Blt_Vec_UpdateClients(data.freqPtr);
    }
    return TCL_OK;
}	

/*ARGSUSED*/
static int
InverseFFTOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int isNew;
    char *name;
    Vector *srcImagPtr;
    Vector *destRealPtr;
    Vector *destImagPtr;

    name = Tcl_GetString(objv[2]);
    if (Blt_Vec_LookupName(vPtr->dataPtr, name, &srcImagPtr) != TCL_OK ) {
	return TCL_ERROR;
    }
    name = Tcl_GetString(objv[3]);
    destRealPtr = Blt_Vec_Create(vPtr->dataPtr, name, name, name, &isNew);
    name = Tcl_GetString(objv[4]);
    destImagPtr = Blt_Vec_Create(vPtr->dataPtr, name, name, name, &isNew);

    if (Blt_Vec_InverseFFT(interp, srcImagPtr, destRealPtr, destImagPtr, vPtr) 
	!= TCL_OK ){
	return TCL_ERROR;
    }
    if (destRealPtr->flush) {
	Blt_Vec_FlushCache(destRealPtr);
    }
    Blt_Vec_UpdateClients(destRealPtr);

    if (destImagPtr->flush) {
	Blt_Vec_FlushCache(destImagPtr);
    }
    Blt_Vec_UpdateClients(destImagPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * LengthOp --
 *
 *	Returns the length of the vector.  If a new size is given, the vector
 *	is resized to the new vector.
 *
 * Results:
 *	A standard TCL result.  If the new length is invalid, interp->result
 *	will an error message and TCL_ERROR is returned.  Otherwise
 *	interp->result will contain the length of the vector.
 *
 *---------------------------------------------------------------------------
 */
static int
LengthOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    if (objc == 3) {
	int numElem;

	if (Tcl_GetIntFromObj(interp, objv[2], &numElem) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (numElem < 0) {
	    Tcl_AppendResult(interp, "bad vector size \"", 
		Tcl_GetString(objv[2]), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	if ((Blt_Vec_SetSize(interp, vPtr, numElem) != TCL_OK) ||
	    (Blt_Vec_SetLength(interp, vPtr, numElem) != TCL_OK)) {
	    return TCL_ERROR;
	} 
	if (vPtr->flush) {
	    Blt_Vec_FlushCache(vPtr);
	}
	Blt_Vec_UpdateClients(vPtr);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), vPtr->length);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LimitsOp --
 *
 *	Returns the minimum and maximum value of the vector.
 *
 * Results:
 *	A standard TCL result. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
LimitsOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    objPtr = Tcl_NewDoubleObj(Blt_Vec_Min(vPtr));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewDoubleObj(Blt_Vec_Max(vPtr));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MapOp --
 *
 *	Queries or sets the offset of the array index from the base address of
 *	the data array of values.
 *
 * Results:
 *	A standard TCL result.  If the source vector doesn't exist or the
 *	source list is not a valid list of numbers, TCL_ERROR returned.
 *	Otherwise TCL_OK is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MapOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    if (objc > 2) {
	if (Blt_Vec_MapVariable(interp, vPtr, Tcl_GetString(objv[2])) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (vPtr->arrayName != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), vPtr->arrayName, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MaxOp --
 *
 *	Returns the maximum value of the vector.
 *
 * Results:
 *	A standard TCL result. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MaxOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_SetDoubleObj(Tcl_GetObjResult(interp), Blt_Vec_Max(vPtr));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MergeOp --
 *
 *	Merges the values from the given vectors to the current vector.
 *
 * Results:
 *	A standard TCL result.  If any of the given vectors differ in size,
 *	TCL_ERROR is returned.  Otherwise TCL_OK is returned and the
 *	vector data will contain merged values of the given vectors.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MergeOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Vector **vecArr;
    int refSize, numElem;
    int i;
    double *valuePtr, *valueArr;
    Vector **vPtrPtr;
    
    /* Allocate an array of vector pointers of each vector to be merged in the
     * current vector.  */
    vecArr = Blt_AssertMalloc(sizeof(Vector *) * objc);
    vPtrPtr = vecArr;

    refSize = -1;
    numElem = 0;
    for (i = 2; i < objc; i++) {
	Vector *v2Ptr;
	int length;

	if (Blt_Vec_LookupName(vPtr->dataPtr, Tcl_GetString(objv[i]), &v2Ptr)
		!= TCL_OK) {
	    Blt_Free(vecArr);
	    return TCL_ERROR;
	}
	/* Check that all the vectors are the same length */
	length = v2Ptr->length;
	if (refSize < 0) {
	    refSize = length;
	} else if (length != refSize) {
	    Tcl_AppendResult(vPtr->interp, "vectors \"", vPtr->name,
		"\" and \"", v2Ptr->name, "\" differ in length",
		(char *)NULL);
	    Blt_Free(vecArr);
	    return TCL_ERROR;
	}
	*vPtrPtr++ = v2Ptr;
	numElem += refSize;
    }
    *vPtrPtr = NULL;

    valueArr = Blt_Malloc(sizeof(double) * numElem);
    if (valueArr == NULL) {
	Tcl_AppendResult(vPtr->interp, "not enough memory to allocate ", 
		 Blt_Itoa(numElem), " vector elements", (char *)NULL);
	return TCL_ERROR;
    }

    /* Merge the values from each of the vectors into the current vector */
    valuePtr = valueArr;
    for (i = 0; i < refSize; i++) {
	Vector **vpp;

	for (vpp = vecArr; *vpp != NULL; vpp++) {
	    *valuePtr++ = (*vpp)->valueArr[i];
	}
    }
    Blt_Free(vecArr);
    Blt_Vec_Reset(vPtr, valueArr, numElem, numElem, TCL_DYNAMIC);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MinOp --
 *
 *	Returns the minimum value of the vector.
 *
 * Results:
 *	A standard TCL result. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MinOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_SetDoubleObj(Tcl_GetObjResult(interp), Blt_Vec_Min(vPtr));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NormalizeOp --
 *
 *	Normalizes the vector.
 *
 * Results:
 *	A standard TCL result.  If the density is invalid, TCL_ERROR is
 *	returned.  Otherwise TCL_OK is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NormalizeOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int i;
    double range;

    Blt_Vec_UpdateRange(vPtr);
    range = vPtr->max - vPtr->min;
    if (objc > 2) {
	Vector *v2Ptr;
	int isNew;
	char *string;

	string = Tcl_GetString(objv[2]);
	v2Ptr = Blt_Vec_Create(vPtr->dataPtr, string, string, string, &isNew);
	if (v2Ptr == NULL) {
	    return TCL_ERROR;
	}
	if (Blt_Vec_SetLength(interp, v2Ptr, vPtr->length) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (i = 0; i < vPtr->length; i++) {
	    v2Ptr->valueArr[i] = (vPtr->valueArr[i] - vPtr->min) / range;
	}
	Blt_Vec_UpdateRange(v2Ptr);
	if (!isNew) {
	    if (v2Ptr->flush) {
		Blt_Vec_FlushCache(v2Ptr);
	    }
	    Blt_Vec_UpdateClients(v2Ptr);
	}
    } else {
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (i = 0; i < vPtr->length; i++) {
	    double norm;

	    norm = (vPtr->valueArr[i] - vPtr->min) / range;
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewDoubleObj(norm));
	}
	Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyOp --
 *
 *	Notify clients of vector.
 *
 * Results:
 *	A standard TCL result.  If any of the given vectors differ in size,
 *	TCL_ERROR is returned.  Otherwise TCL_OK is returned and the
 *	vector data will contain merged values of the given vectors.
 *
 *  x vector notify now
 *  x vector notify always
 *  x vector notify whenidle
 *  x vector notify update {}
 *  x vector notify delete {}
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NotifyOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int option;
    int bool;
    enum optionIndices {
	OPTION_ALWAYS, OPTION_NEVER, OPTION_WHENIDLE, 
	OPTION_NOW, OPTION_CANCEL, OPTION_PENDING
    };
    static const char *optionArr[] = {
	"always", "never", "whenidle", "now", "cancel", "pending", NULL
    };

    if (Tcl_GetIndexFromObj(interp, objv[2], optionArr, "qualifier", TCL_EXACT,
	    &option) != TCL_OK) {
	return TCL_OK;
    }
    switch (option) {
    case OPTION_ALWAYS:
	vPtr->notifyFlags &= ~NOTIFY_WHEN_MASK;
	vPtr->notifyFlags |= NOTIFY_ALWAYS;
	break;
    case OPTION_NEVER:
	vPtr->notifyFlags &= ~NOTIFY_WHEN_MASK;
	vPtr->notifyFlags |= NOTIFY_NEVER;
	break;
    case OPTION_WHENIDLE:
	vPtr->notifyFlags &= ~NOTIFY_WHEN_MASK;
	vPtr->notifyFlags |= NOTIFY_WHENIDLE;
	break;
    case OPTION_NOW:
	/* FIXME: How does this play when an update is pending? */
	Blt_Vec_NotifyClients(vPtr);
	break;
    case OPTION_CANCEL:
	if (vPtr->notifyFlags & NOTIFY_PENDING) {
	    vPtr->notifyFlags &= ~NOTIFY_PENDING;
	    Tcl_CancelIdleCall(Blt_Vec_NotifyClients, (ClientData)vPtr);
	}
	break;
    case OPTION_PENDING:
	bool = (vPtr->notifyFlags & NOTIFY_PENDING);
	Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
	break;
    }	
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PopulateOp --
 *
 *	Creates or resizes a new vector based upon the density specified.
 *
 * Results:
 *	A standard TCL result.  If the density is invalid, TCL_ERROR
 *	is returned.  Otherwise TCL_OK is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PopulateOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Vector *v2Ptr;
    int size, density;
    int isNew;
    int i, j;
    double *valuePtr;
    int count;
    char *string;

    string = Tcl_GetString(objv[2]);
    v2Ptr = Blt_Vec_Create(vPtr->dataPtr, string, string, string, &isNew);
    if (v2Ptr == NULL) {
	return TCL_ERROR;
    }
    if (v2Ptr->length == 0) {
	return TCL_OK;			/* Source vector is empty. */
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &density) != TCL_OK) {
	return TCL_ERROR;
    }
    if (density < 1) {
	Tcl_AppendResult(interp, "bad density \"", Tcl_GetString(objv[3]), 
		"\"", (char *)NULL);
	return TCL_ERROR;
    }
    size = (v2Ptr->length - 1) * (density + 1) + 1;
    if (Blt_Vec_SetLength(interp, vPtr, size) != TCL_OK) {
	return TCL_ERROR;
    }
    count = 0;
    valuePtr = vPtr->valueArr;
    for (i = 0; i < (v2Ptr->length - 1); i++) {
	double slice, range;

	range = v2Ptr->valueArr[i + 1] - v2Ptr->valueArr[i];
	slice = range / (double)(density + 1);
	for (j = 0; j <= density; j++) {
	    *valuePtr = v2Ptr->valueArr[i] + (slice * (double)j);
	    valuePtr++;
	    count++;
	}
    }
    count++;
    *valuePtr = v2Ptr->valueArr[i];
    assert(count == vPtr->length);
    if (!isNew) {
	if (vPtr->flush) {
	    Blt_Vec_FlushCache(vPtr);
	}
	Blt_Vec_UpdateClients(vPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ValueGetOp --
 *
 *	Get the value of the index.  This simulates what the vector's variable
 *	does.
 *
 * Results:
 *	A standard TCL result.  If the index is invalid, interp->result will
 *	an error message and TCL_ERROR is returned.  Otherwise interp->result
 *	will contain the values.
 *
 *	vecName value get index 
 *
 *---------------------------------------------------------------------------
 */
static int
ValueGetOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int first, last;
    const char *string;
    Tcl_Obj *listObjPtr;

    string = Tcl_GetString(objv[3]);
    if (Blt_Vec_GetIndexRange(interp, vPtr, string, INDEX_ALL_FLAGS, 
		(Blt_VectorIndexProc **) NULL) != TCL_OK) {
	return TCL_ERROR;
    }
    first = vPtr->first, last = vPtr->last;
    if (first == vPtr->length) {
	Tcl_AppendResult(interp, "can't get index \"", string, "\"",
			 (char *)NULL);
	return TCL_ERROR;		/* Can't read from index "++end" */
    }
    listObjPtr = GetValues(vPtr, first, last);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ValueSetOp --
 *
 *	Sets the value of the index.  This simulates what the vector's
 *	variable does.
 *
 * Results:
 *	A standard TCL result.  If the index is invalid, interp->result will
 *	an error message and TCL_ERROR is returned.  Otherwise interp->result
 *	will contain the values.
 *
 *---------------------------------------------------------------------------
 */
static int
ValueSetOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int first, last;
    const char *string;
    double value;

    string = Tcl_GetString(objv[3]);
    if (Blt_Vec_GetIndexRange(interp, vPtr, string, INDEX_ALL_FLAGS, 
		(Blt_VectorIndexProc **) NULL) != TCL_OK) {
	return TCL_ERROR;
    }
    first = vPtr->first, last = vPtr->last;

    /* FIXME: huh? Why set values here?.  */
    if (first == SPECIAL_INDEX) {
	Tcl_AppendResult(interp, "can't set index \"", string, "\"",
			 (char *)NULL);
	return TCL_ERROR;		/* Tried to set "min" or "max" */
    }
    if (Blt_ExprDoubleFromObj(interp, objv[4], &value) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((first == vPtr->length) || (last == vPtr->length)) {
	if (Blt_Vec_ChangeLength(interp, vPtr, vPtr->length + 1) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    ReplicateValue(vPtr, first, last, value);
    Tcl_SetObjResult(interp, objv[4]);
    if (vPtr->flush) {
	Blt_Vec_FlushCache(vPtr);
    }
    Blt_Vec_UpdateClients(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ValueUnsetOp --
 *
 *	Unsets the value of the index.  This simulates what the vector's
 *	variable does.
 *
 * Results:
 *	A standard TCL result.  If the index is invalid, interp->result will
 *	an error message and TCL_ERROR is returned.  Otherwise interp->result
 *	will contain the values.
 *
 *---------------------------------------------------------------------------
 */
static int
ValueUnsetOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int i;

    for (i = 3; i < objc; i++) {
	int first, last;
	const char *string;

	string = Tcl_GetString(objv[i]);
	if (Blt_Vec_GetIndexRange(interp, vPtr, string, INDEX_ALL_FLAGS, 
			(Blt_VectorIndexProc **) NULL) != TCL_OK) {
	    return TCL_ERROR;
	}
	first = vPtr->first, last = vPtr->last;
	/* FIXME: huh? Why set values here?.  */
	if (first == SPECIAL_INDEX) {
	    Tcl_AppendResult(interp, "can't set index \"", string, "\"",
			     (char *)NULL);
	    return TCL_ERROR;		
	}
	ReplicateValue(vPtr, first, last, Blt_NaN());
    }
    if (vPtr->flush) {
	Blt_Vec_FlushCache(vPtr);
    }
    Blt_Vec_UpdateClients(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ValueOp --
 *
 *	Parses and invokes the appropriate vector instance command option.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec valueOps[] =
{
    {"get",       1, ValueGetOp,   4, 4, "index",},
    {"set",	  1, ValueSetOp,   4, 0, "index value",},
    {"unset",     1, ValueUnsetOp, 3, 0, "?index...?",},
};

static int numValueOps = sizeof(valueOps) / sizeof(Blt_OpSpec);

static int
ValueOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    VectorCmdProc *proc;

    vPtr->first = 0;
    vPtr->last = vPtr->length - 1;
    proc = Blt_GetOpFromObj(interp, numValueOps, valueOps, BLT_OP_ARG2, objc,
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (vPtr, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * ValuesOp --
 *
 *	Print the values vector.
 *
 * Results:
 *	A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ValuesOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ValuesSwitches switches;
    Tcl_Obj *listObjPtr;

    switches.formatObjPtr = NULL;
    switches.from = 0;
    switches.to = vPtr->length - 1;
    switches.empty = TRUE;
    indexSwitch.clientData = vPtr;
    if (Blt_ParseSwitches(interp, valuesSwitches, objc - 2, objv + 2, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (switches.formatObjPtr == NULL) {
	int i;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	if (switches.empty) {
	    for (i = switches.from; i <= switches.to; i++) {
		Tcl_ListObjAppendElement(interp, listObjPtr, 
			Tcl_NewDoubleObj(vPtr->valueArr[i]));
	    }
	} else {
	    for (i = switches.from; i <= switches.to; i++) {
		if (FINITE(vPtr->valueArr[i])) {
		    Tcl_ListObjAppendElement(interp, listObjPtr, 
			Tcl_NewDoubleObj(vPtr->valueArr[i]));
		}
	    }
	}
    } else {
	char buffer[200];
	const char *fmt;
	int i;

	fmt = Tcl_GetString(switches.formatObjPtr);
	if (switches.empty) {
	    for (i = switches.from; i <= switches.to; i++) {
		sprintf(buffer, fmt, vPtr->valueArr[i]);
		Tcl_ListObjAppendElement(interp, listObjPtr, 
			Tcl_NewStringObj(buffer, -1));
	    }
	} else {
	    for (i = switches.from; i <= switches.to; i++) {
		if (FINITE(vPtr->valueArr[i])) {
		    sprintf(buffer, fmt, vPtr->valueArr[i]);
		    Tcl_ListObjAppendElement(interp, listObjPtr, 
			Tcl_NewStringObj(buffer, -1));
		}
	    }
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    Blt_FreeSwitches(valuesSwitches, &switches, 0);
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * AppendFormatToObj --
 *
 *	This function appends a list of Tcl_Obj's to a Tcl_Obj according to
 *	the formatting instructions embedded in the format string. The
 *	formatting instructions are inspired by sprintf(). Returns TCL_OK when
 *	successful. If there's an error in the arguments, TCL_ERROR is
 *	returned, and an error message is written to the interp, if non-NULL.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
#define FMT_ALLOCSEGMENT (1<<0)
#define FMT_CHAR	(1<<1)
#define FMT_HASH	(1<<2)
#define FMT_ISNEGATIVE	(1<<3)
#define FMT_LONG	(1<<4)
#define FMT_LONGLONG	(1<<5)
#define FMT_MINUS	(1<<6)
#define FMT_NEWXPG	(1<<7)
#define FMT_PLUS	(1<<8)
#define FMT_PRECISION	(1<<9)
#define FMT_SEQUENTIAL	(1<<10)
#define FMT_SHORT	(1<<11)
#define FMT_SPACE	(1<<12)
#define FMT_USEWIDE	(1<<13)
#define FMT_WIDTH	(1<<14)
#define FMT_XPG		(1<<15)
#define FMT_ZERO	(1<<16)

typedef struct {
    int precision;			/* Precision to use. */
    int width;				/* Minimum field width. */
    unsigned int flags;
    Tcl_UniChar ch;			/* Last character parsed. */
} FormatParser;

static Tcl_Obj *
FormatDouble(Tcl_Interp *interp, double d, FormatParser *parserPtr)
{
#define MAX_FLOAT_SIZE 320
    Tcl_Obj *objPtr;
    char spec[2*TCL_INTEGER_SPACE + 9];
    char *p;
    static const char *overflow = "max size for a Tcl value exceeded";
    int length;
    char *bytes;

    length = MAX_FLOAT_SIZE;
    p = spec;
    *p++ = '%';
    if (parserPtr->flags & FMT_MINUS) {
	*p++ = '-';
    }
    if (parserPtr->flags & FMT_HASH) {
	*p++ = '#';
    }
    if (parserPtr->flags & FMT_ZERO) {
	*p++ = '0';
    }
    if (parserPtr->flags & FMT_SPACE) {
	*p++ = ' ';
    }
    if (parserPtr->flags & FMT_PLUS) {
	*p++ = '+';
    }
    if (parserPtr->flags & FMT_WIDTH) {
	p += sprintf(p, "%d", parserPtr->width);
	if (parserPtr->width > length) {
	    length = parserPtr->width;
	}
    }
    if (parserPtr->flags & FMT_PRECISION) {
	*p++ = '.';
	p += sprintf(p, "%d", parserPtr->precision);
	if (parserPtr->precision > INT_MAX - length) {
	    Tcl_AppendResult(interp, overflow, (char *)NULL);
	    return NULL;
	}
	length += parserPtr->precision;
    }
    /*
     * Don't pass length modifiers!
     */
    *p++ = (char) parserPtr->ch;
    *p = '\0';
    objPtr = Tcl_NewObj();
    parserPtr->flags |= FMT_ALLOCSEGMENT;
    if (!Tcl_AttemptSetObjLength(objPtr, length)) {
	Tcl_AppendResult(interp, overflow, (char *)NULL);
	return NULL;
    }
    bytes = Tcl_GetString(objPtr);
    if (!Tcl_AttemptSetObjLength(objPtr, sprintf(bytes, spec, d))) {
	Tcl_AppendResult(interp, overflow, (char *)NULL);
	return NULL;
    }
    return objPtr;
}

static Tcl_Obj *
FormatLong(Tcl_Interp *interp, double d, FormatParser *parserPtr)
{
    int64_t ll;
    long l;
    unsigned short s;
    Tcl_Obj *objPtr;
    char spec[2*TCL_INTEGER_SPACE + 9];
    char *p;
    int length;
    char *bytes;
    static const char *overflow = "max size for a Tcl value exceeded";
    length = MAX_FLOAT_SIZE;
    
    parserPtr->flags &= ~FMT_ISNEGATIVE;
    if (parserPtr->flags & FMT_LONGLONG) {
	ll = (int64_t)d;
	if (ll < 0) {
	    parserPtr->flags |= FMT_ISNEGATIVE;
	}
    } else if (parserPtr->flags & FMT_LONG) {
	l = (long)d;
	if (l < 0) {
	    parserPtr->flags |= FMT_ISNEGATIVE;
	}
    } else if (parserPtr->flags & FMT_SHORT) {
	s = (unsigned short int)d;
	if (s < 0) {
	    parserPtr->flags |= FMT_ISNEGATIVE;
	}
    } else {
	l = (long)d;
	if (l < (long)0) {
	    parserPtr->flags |= FMT_ISNEGATIVE;
	}
    }
    objPtr = Tcl_NewObj();
    parserPtr->flags |= FMT_ALLOCSEGMENT;
    Tcl_IncrRefCount(objPtr);
    
    if ((parserPtr->flags & (FMT_ISNEGATIVE | FMT_PLUS | FMT_SPACE)) && 
	((parserPtr->flags & FMT_LONGLONG) || (parserPtr->ch == 'd'))) {
	Tcl_AppendToObj(objPtr, 
			((parserPtr->flags & FMT_ISNEGATIVE) ? "-" : 
			 (parserPtr->flags & FMT_PLUS) ? "+" : " "), 1);
    }
    if (parserPtr->flags & FMT_HASH) {
	switch (parserPtr->ch) {
	case 'o':
	    Tcl_AppendToObj(objPtr, "0", 1);
	    parserPtr->precision--;
	    break;
	case 'x':
	case 'X':
	    Tcl_AppendToObj(objPtr, "0x", 2);
	    break;
	}
    }
    p = spec;
    *p++ = '%';
    if (parserPtr->flags & FMT_MINUS) {
	*p++ = '-';
    }
    if (parserPtr->flags & FMT_HASH) {
	*p++ = '#';
    }
    if (parserPtr->flags & FMT_ZERO) {
	*p++ = '0';
    }
    if (parserPtr->flags & FMT_SPACE) {
	*p++ = ' ';
    }
    if (parserPtr->flags & FMT_PLUS) {
	*p++ = '+';
    }
    if (parserPtr->flags & FMT_WIDTH) {
	p += sprintf(p, "%d", parserPtr->width);
	if (parserPtr->width > length) {
	    length = parserPtr->width;
	}
    }
    if (parserPtr->flags & FMT_PRECISION) {
	*p++ = '.';
	p += sprintf(p, "%d", parserPtr->precision);
	if (parserPtr->precision > INT_MAX - length) {
	    Tcl_AppendResult(interp, overflow, (char *)NULL);
	    return NULL;
	}
	length += parserPtr->precision;
    }
    /*
     * Don't pass length modifiers!
     */
    *p++ = (char)parserPtr->ch;
    *p = '\0';

    objPtr = Tcl_NewObj();
    parserPtr->flags |= FMT_ALLOCSEGMENT;
    if (!Tcl_AttemptSetObjLength(objPtr, length)) {
	Tcl_AppendResult(interp, overflow, (char *)NULL);
	return NULL;
    }
    bytes = Tcl_GetString(objPtr);
    if (!Tcl_AttemptSetObjLength(objPtr, sprintf(bytes, spec, d))) {
	Tcl_AppendResult(interp, overflow, (char *)NULL);
	return NULL;
    }
    return objPtr;
}

static int
AppendFormatToObj(Tcl_Interp *interp, Tcl_Obj *appendObjPtr, const char *format,
		  int *offsetPtr, Vector *vPtr, int maxOffset)
{
    FormatParser parser;
    const char *span = format, *msg;
    int numBytes = 0, index, count;
    int originalLength, limit, offset;
    static const char *mixedXPG =
	    "cannot mix \"%\" and \"%n$\" conversion specifiers";
    static const char *badIndex[2] = {
	"not enough arguments for all format specifiers",
	"\"%n$\" argument index out of range"
    };
    static const char *overflow = "max size for a Tcl value exceeded";

    msg = overflow;
    if (Tcl_IsShared(appendObjPtr)) {
	Tcl_Panic("%s called with shared object", "Tcl_AppendFormatToObj");
    }
    Tcl_GetStringFromObj(appendObjPtr, &originalLength);
    limit = INT_MAX - originalLength;

    memset(&parser, 0, sizeof(parser));
    /*
     * Format string is NUL-terminated.
     */
    span = format;
    count = index = 0;
    offset = *offsetPtr;
    while (*format != '\0') {
	char *end;
	int numChars, segmentNumBytes;
	Tcl_Obj *segment;
	int step, done;
	double d;

	step = Tcl_UtfToUniChar(format, &parser.ch);
	format += step;
	if (parser.ch != '%') {
	    numBytes += step;
	    continue;
	}
	if (numBytes > 0) {
	    if (numBytes > limit) {
		msg = overflow;
		goto errorMsg;
	    }
	    Tcl_AppendToObj(appendObjPtr, span, numBytes);
	    limit -= numBytes;
	    numBytes = 0;
	}

	/*
	 * Saw a '%'. Process the format specifier.
	 *
	 * Step 0. Handle special case of escaped format marker (i.e., %%).
	 */
	step = Tcl_UtfToUniChar(format, &parser.ch);
	if (parser.ch == '%') {
	    span = format;
	    numBytes = step;
	    format += step;
	    continue;			/* This is an escaped percent. */
	}

	/*
	 * Step 1. XPG3 position specifier
	 */
	parser.flags &= ~FMT_NEWXPG;
	if (isdigit(UCHAR(parser.ch))) {
	    int position;
	    char *end;

	    position = strtoul(format, &end, 10);
	    if (*end == '$') {
		parser.flags |= FMT_NEWXPG;
		index = position - 1;
		format = end + 1;
		step = Tcl_UtfToUniChar(format, &parser.ch);
	    }
	}
	if (parser.flags & FMT_NEWXPG) {
	    if (parser.flags & FMT_SEQUENTIAL) {
		msg = mixedXPG;
		goto errorMsg;
	    }
	    parser.flags |= FMT_XPG;
	} else {
	    if (parser.flags & FMT_XPG) {
		msg = mixedXPG;
		goto errorMsg;
	    }
	    parser.flags |= FMT_SEQUENTIAL;
	}
	if (index < 0) {
	    /* Index is outside of available vector elements. */
	    msg = badIndex[parser.flags & FMT_XPG];
	    goto errorMsg;		
	}

	/*
	 * Step 2. Set of parser.flags.
	 */
	parser.flags &= ~(FMT_MINUS|FMT_HASH|FMT_ZERO|FMT_SPACE|FMT_PLUS);
	done = FALSE;
	do {
	    switch (parser.ch) {
	    case '-': parser.flags |= FMT_MINUS;	break;
	    case '#': parser.flags |= FMT_HASH;		break;
	    case '0': parser.flags |= FMT_ZERO;		break;
	    case ' ': parser.flags |= FMT_SPACE;	break;
	    case '+': parser.flags |= FMT_PLUS;		break;
	    default:
		done = TRUE;
	    }
	    if (!done) {
		format += step;
		step = Tcl_UtfToUniChar(format, &parser.ch);
	    }
	} while (!done);

	/*
	 * Step 3. Minimum field width.
	 */
	parser.width = 0;
	if (isdigit(UCHAR(parser.ch))) {
	    parser.flags |= FMT_WIDTH;
	    parser.width = strtoul(format, &end, 10);
	    format = end;
	    step = Tcl_UtfToUniChar(format, &parser.ch);
	} else if (parser.ch == '*') {
	    msg = "can't specify '*' in field width";
	    goto errorMsg;
	}
	if (parser.width > limit) {
	    msg = overflow;
	    goto errorMsg;
	}

	/*
	 * Step 4. Precision.
	 */

	parser.flags &= ~(FMT_PRECISION);
	parser.precision = 0;
	if (parser.ch == '.') {
	    parser.flags |= FMT_PRECISION;
	    format += step;
	    step = Tcl_UtfToUniChar(format, &parser.ch);
	}
	if (isdigit(UCHAR(parser.ch))) {
	    parser.precision = strtoul(format, &end, 10);
	    format = end;
	    step = Tcl_UtfToUniChar(format, &parser.ch);
	} else if (parser.ch == '*') {
	    msg = "can't specify '*' in precision";
	    goto errorMsg;
	}

	/*
	 * Step 5. Length modifier.
	 */
	if (parser.ch == 'h') {
	    format += step;
	    step = Tcl_UtfToUniChar(format, &parser.ch);
	    if (parser.ch == 'h') {
		parser.flags |= FMT_CHAR;
		format += step;
		step = Tcl_UtfToUniChar(format, &parser.ch);
	    } else {
		parser.flags |= FMT_SHORT;
	    }
	} else if (parser.ch == 'l') {
	    format += step;
	    step = Tcl_UtfToUniChar(format, &parser.ch);
	    if (parser.ch == 'l') {
		parser.flags |= FMT_LONGLONG;
		format += step;
		step = Tcl_UtfToUniChar(format, &parser.ch);
	    } else {
		parser.flags |= FMT_LONG;
	    }
	}
	format += step;
	span = format;

	/*
	 * Step 6. The actual conversion character.
	 */
	if ((index + offset) > maxOffset) {
	    continue;
	}
	d = vPtr->valueArr[offset + index];
	numChars = -1;
	if (parser.ch == 'i') {
	    parser.ch = 'd';
	}
	switch (parser.ch) {
	case '\0':
	    msg = "format string ended in middle of field specifier";
	    goto errorMsg;
	case 's':
	    msg = "can't use %s or %c as field specifier";
	    goto errorMsg;
	case 'u':
	    if (parser.flags & FMT_LONGLONG) {
		msg = "unsigned bignum format is invalid";
		goto errorMsg;
	    }
	case 'd':
	case 'o':
	case 'x':
	case 'X': 
	    segment = FormatLong(interp, d, &parser);
	    if (segment == NULL) {
		goto errorMsg;
	    }
	    break;

	case 'e':
	case 'E':
	case 'f':
	case 'g':
	case 'G': 
	    segment = FormatDouble(interp, d, &parser);
	    if (segment == NULL) {
		goto error;
	    }
	    break;

	default:
	    if (interp != NULL) {
		Tcl_SetObjResult(interp,
			Tcl_ObjPrintf("bad field specifier \"%c\"", parser.ch));
	    }
	    goto error;
	}

	switch (parser.ch) {
	case 'E':
	case 'G':
	case 'X': {
	    Tcl_SetObjLength(segment, Tcl_UtfToUpper(Tcl_GetString(segment)));
	}
	}

	if (parser.flags & FMT_WIDTH) {
	    if (numChars < 0) {
		numChars = Tcl_GetCharLength(segment);
	    }
	    if (!(parser.flags & FMT_MINUS)) {
		if (numChars < parser.width) {
		    limit -= (parser.width - numChars);
		}
		while (numChars < parser.width) {
		    Tcl_AppendToObj(appendObjPtr, (FMT_ZERO ? "0" : " "), 1);
		    numChars++;
		}
	    }
	}

	Tcl_GetStringFromObj(segment, &segmentNumBytes);
	if (segmentNumBytes > limit) {
	    if (parser.flags & FMT_ALLOCSEGMENT) {
		Tcl_DecrRefCount(segment);
	    }
	    msg = overflow;
	    goto errorMsg;
	}
	Tcl_AppendObjToObj(appendObjPtr, segment);
	limit -= segmentNumBytes;
	if (parser.flags & FMT_ALLOCSEGMENT) {
	    Tcl_DecrRefCount(segment);
	}
	if (parser.flags & FMT_WIDTH) {
	    if (numChars < parser.width) {
		limit -= (parser.width - numChars);
	    }
	    while (numChars < parser.width) {
		Tcl_AppendToObj(appendObjPtr, (FMT_ZERO ? "0" : " "), 1);
		numChars++;
	    }
	}

	if (parser.flags & FMT_SEQUENTIAL) {
	    index++;
	}
	count++;
    }
    if (numBytes) {
	if (numBytes > limit) {
	    msg = overflow;
	    goto errorMsg;
	}
	Tcl_AppendToObj(appendObjPtr, span, numBytes);
	limit -= numBytes;
    }
    *offsetPtr = offset + count;
    return TCL_OK;

  errorMsg:
    if (interp != NULL) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(msg, -1));
    }
  error:
    Tcl_SetObjLength(appendObjPtr, originalLength);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseFormat --
 *
 *	Parses a printf-like format string into individual components.
 *
 * Results:
 *	A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ParseFormat(const char *format, int *numFmtsPtr, char ***fmtPtr)
{
    const char *p;
    char *q;
    char *string;
    char **fmt;
    size_t addrLength, length;
    int count, first;

    /* Step 1:  Find out how may descriptors exist. */
    count = 0;
    for (p = format; *p != '\0'; p++) {
	if (p[0] == '%') {
	    if (p[1] == '%') {
		p+= 2;
		continue;
	    }
	    count++;
	}
    }
    /* Step 2: Create a format array to hold the individual descriptors. */
    length = count + (p - format) + 1;
    addrLength = (count + 1) * sizeof(char **);
    *numFmtsPtr = count;
    fmt = Blt_AssertMalloc(addrLength + length);
    string = (char *)fmt + addrLength;

    /* Step 3:  Load the format array with individual descriptors. */
    count = 1;
    fmt[0] = string;
    first = 0;
    for (q = string, p = format; *p != '\0'; p++) {
	if (p[0] == '%') {
	    if (p[1] == '%') {
		q[0] = q[1] = '%';
		q += 2;
		p += 2;
		continue;
	    }
	    if (first > 0) {
		*q = '\0';
		q++;
		fmt[count++] = q;
	    }
	    first++;
	    *q = '%';
	    q++;
	    continue;
	}
	*q = *p;
	q++;
    }
    *q = '\0';
    fmt[count] = NULL;
    *fmtPtr = fmt;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PrintOp --
 *
 *	Print the values vector according to the given format.
 *
 * Results:
 *	A standard TCL result.  
 *
 *	$v print $format ?switches?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PrintOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    PrintSwitches switches;
    Tcl_Obj *objPtr;
    char **argv;
    int argc;
    char *fmt;
    int i;

    switches.from = 0;
    switches.to = vPtr->length - 1;
    indexSwitch.clientData = vPtr;

    fmt = Tcl_GetString(objv[2]);
    ParseFormat(fmt, &argc, &argv);
    if (argc == 0) {
        Tcl_AppendResult(interp, "format \"", fmt, "\" contains no specifiers", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    if (Blt_ParseSwitches(interp, printSwitches, objc - 3, objv + 3, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    objPtr = Tcl_NewStringObj("", 0);
    for (i = switches.from; i <= switches.to; /*empty*/) {
	if (FINITE(vPtr->valueArr[i])) {
	    char string[200];
	    int n;

	    n = (i % argc);
	    fmt = argv[n];
	    sprintf(string, fmt, vPtr->valueArr[i]);
	    AppendFormatToObj(interp, objPtr, fmt, &i, vPtr, switches.to);
	}
    }
    Blt_Free(argv);
    Tcl_SetObjResult(interp, objPtr);
    Blt_FreeSwitches(printSwitches, &switches, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RangeOp --
 *
 *	Returns a TCL list of the range of vector values specified.
 *
 * Results:
 *	A standard TCL result.  If the given range is invalid, TCL_ERROR
 *	is returned.  Otherwise TCL_OK is returned and interp->result
 *	will contain the list of values.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RangeOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;
    int first, last;
    int i;

    if (objc == 2) {
	first = 0;
	last = vPtr->length - 1;
    } else if (objc == 4) {
	if ((Blt_Vec_GetIndex(interp, vPtr, Tcl_GetString(objv[2]), &first, 
		0, (Blt_VectorIndexProc **) NULL) != TCL_OK) ||
	    (Blt_Vec_GetIndex(interp, vPtr, Tcl_GetString(objv[3]), &last, 
		0, (Blt_VectorIndexProc **) NULL) != TCL_OK)) {
	    return TCL_ERROR;
	}
	if (first > vPtr->length) {
	    Tcl_AppendResult(interp, "bad first index \"", 
		Tcl_GetString(objv[2]), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	if (last > vPtr->length) {
	    Tcl_AppendResult(interp, "bad last index \"", 
		Tcl_GetString(objv[3]), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
    } else {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		Tcl_GetString(objv[0]), " range ?first last?", (char *)NULL);
	return TCL_ERROR;	
    }
    if (((first == -1) || (last == -1)) && (vPtr->length == 0)) {
	return TCL_OK;			/* Allow range on empty vector */
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (first > last) {
	/* Return the list reversed */
	for (i = last; i <= first; i++) {
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewDoubleObj(vPtr->valueArr[i]));
	}
    } else {
	for (i = first; i <= last; i++) {
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewDoubleObj(vPtr->valueArr[i]));
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InRange --
 *
 *	Determines if a value lies within a given range.
 *
 *	The value is normalized and compared against the interval
 *	[0..1], where 0.0 is the minimum and 1.0 is the maximum.
 *	DBL_EPSILON is the smallest number that can be represented
 *	on the host machine, such that (1.0 + epsilon) != 1.0.
 *
 *	Please note, min cannot be greater than max.
 *
 * Results:
 *	If the value is within of the interval [min..max], 1 is 
 *	returned; 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
INLINE static int
InRange(double value, double min, double max)
{
    double range;

    range = max - min;
    if (range < DBL_EPSILON) {
	return (FABS(max - value) < DBL_EPSILON);
    } else {
	double norm;

	norm = (value - min) / range;
	return ((norm >= -DBL_EPSILON) && ((norm - 1.0) < DBL_EPSILON));
    }
}

enum NativeFormats {
    NF_UNKNOWN = -1,
    NF_UCHAR, NF_CHAR,
    NF_USHORT, NF_SHORT,
    NF_UINT, NF_INT,
    NF_ULONG, NF_LONG,
    NF_FLOAT, NF_DOUBLE
};

/*
 *---------------------------------------------------------------------------
 *
 * GetBinaryFormat
 *
 *      Translates a format string into a native type.  Valid formats are
 *
 *		signed		i1, i2, i4, i8
 *		unsigned 	u1, u2, u4, u8
 *		real		r4, r8, r16
 *
 *	There must be a corresponding native type.  For example, this for
 *	reading 2-byte binary integers from an instrument and converting them
 *	to unsigned shorts or ints.
 *
 *---------------------------------------------------------------------------
 */
static enum NativeFormats
GetBinaryFormat(Tcl_Interp *interp, char *string, int *sizePtr)
{
    char c;

    c = tolower(string[0]);
    if (Tcl_GetInt(interp, string + 1, sizePtr) != TCL_OK) {
	Tcl_AppendResult(interp, "unknown binary format \"", string,
	    "\": incorrect byte size", (char *)NULL);
	return NF_UNKNOWN;
    }
    switch (c) {
    case 'r':
	if (*sizePtr == sizeof(double)) {
	    return NF_DOUBLE;
	} else if (*sizePtr == sizeof(float)) {
	    return NF_FLOAT;
	}
	break;

    case 'i':
	if (*sizePtr == sizeof(char)) {
	    return NF_CHAR;
	} else if (*sizePtr == sizeof(int)) {
	    return NF_INT;
	} else if (*sizePtr == sizeof(long)) {
	    return NF_LONG;
	} else if (*sizePtr == sizeof(short)) {
	    return NF_SHORT;
	}
	break;

    case 'u':
	if (*sizePtr == sizeof(unsigned char)) {
	    return NF_UCHAR;
	} else if (*sizePtr == sizeof(unsigned int)) {
	    return NF_UINT;
	} else if (*sizePtr == sizeof(unsigned long)) {
	    return NF_ULONG;
	} else if (*sizePtr == sizeof(unsigned short)) {
	    return NF_USHORT;
	}
	break;

    default:
	Tcl_AppendResult(interp, "unknown binary format \"", string,
	    "\": should be either i#, r#, u# (where # is size in bytes)",
	    (char *)NULL);
	return NF_UNKNOWN;
    }
    Tcl_AppendResult(interp, "can't handle format \"", string, "\"", 
		     (char *)NULL);
    return NF_UNKNOWN;
}

static int
CopyValues(Vector *vPtr, char *byteArr, enum NativeFormats fmt, int size, 
	int length, int swap, int *indexPtr)
{
    int i, n;
    int newSize;

    if ((swap) && (size > 1)) {
	int numBytes = size * length;
	unsigned char *p;
	int left, right;

	for (i = 0; i < numBytes; i += size) {
	    p = (unsigned char *)(byteArr + i);
	    for (left = 0, right = size - 1; left < right; left++, right--) {
		p[left] ^= p[right];
		p[right] ^= p[left];
		p[left] ^= p[right];
	    }

	}
    }
    newSize = *indexPtr + length;
    if (newSize > vPtr->length) {
	if (Blt_Vec_ChangeLength(vPtr->interp, vPtr, newSize) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
#define CopyArrayToVector(vPtr, arr) \
    for (i = 0, n = *indexPtr; i < length; i++, n++) { \
	(vPtr)->valueArr[n] = (double)(arr)[i]; \
    }

    switch (fmt) {
    case NF_CHAR:
	CopyArrayToVector(vPtr, (char *)byteArr);
	break;

    case NF_UCHAR:
	CopyArrayToVector(vPtr, (unsigned char *)byteArr);
	break;

    case NF_INT:
	CopyArrayToVector(vPtr, (int *)byteArr);
	break;

    case NF_UINT:
	CopyArrayToVector(vPtr, (unsigned int *)byteArr);
	break;

    case NF_LONG:
	CopyArrayToVector(vPtr, (long *)byteArr);
	break;

    case NF_ULONG:
	CopyArrayToVector(vPtr, (unsigned long *)byteArr);
	break;

    case NF_SHORT:
	CopyArrayToVector(vPtr, (short int *)byteArr);
	break;

    case NF_USHORT:
	CopyArrayToVector(vPtr, (unsigned short int *)byteArr);
	break;

    case NF_FLOAT:
	CopyArrayToVector(vPtr, (float *)byteArr);
	break;

    case NF_DOUBLE:
	CopyArrayToVector(vPtr, (double *)byteArr);
	break;

    case NF_UNKNOWN:
	break;
    }
    *indexPtr += length;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BinreadOp --
 *
 *	Reads binary values from a TCL channel. Values are either appended to
 *	the end of the vector or placed at a given index (using the "-at"
 *	option), overwriting existing values.  Data is read until EOF is found
 *	on the channel or a specified number of values are read.  (note that
 *	this is not necessarily the same as the number of bytes).
 *
 *	The following flags are supported:
 *		-swap		Swap bytes
 *		-at index	Start writing data at the index.
 *		-format fmt	Specifies the format of the data.
 *
 *	This binary reader was created and graciously donated by Harald Kirsch
 *	(kir@iitb.fhg.de).  Anything that's wrong is due to my (gah) munging
 *	of the code.
 *
 * Results:
 *	Returns a standard TCL result. The interpreter result will contain the
 *	number of values (not the number of bytes) read.
 *
 * Caveats:
 *	Channel reads must end on an element boundary.
 *
 *	vecName binread channel count ?switches?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BinreadOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_Channel channel;
    char *byteArr;
    char *string;
    enum NativeFormats fmt;
    int arraySize, bytesRead;
    int count, total;
    int first;
    int size, length, mode;
    int swap;
    int i;

    string = Tcl_GetString(objv[2]);
    channel = Tcl_GetChannel(interp, string, &mode);
    if (channel == NULL) {
	return TCL_ERROR;
    }
    if ((mode & TCL_READABLE) == 0) {
	Tcl_AppendResult(interp, "channel \"", string,
	    "\" wasn't opened for reading", (char *)NULL);
	return TCL_ERROR;
    }
    first = vPtr->length;
    fmt = NF_DOUBLE;
    size = sizeof(double);
    swap = FALSE;
    count = 0;

    if (objc > 3) {
	string = Tcl_GetString(objv[3]);
	if (string[0] != '-') {
	    long int value;
	    /* Get the number of values to read.  */
	    if (Blt_GetLongFromObj(interp, objv[3], &value) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (value < 0) {
		Tcl_AppendResult(interp, "count can't be negative", 
				 (char *)NULL);
		return TCL_ERROR;
	    }
	    count = (size_t)value;
	    objc--, objv++;
	}
    }
    /* Process any option-value pairs that remain.  */
    for (i = 3; i < objc; i++) {
	string = Tcl_GetString(objv[i]);
	if (strcmp(string, "-swap") == 0) {
	    swap = TRUE;
	} else if (strcmp(string, "-format") == 0) {
	    i++;
	    if (i >= objc) {
		Tcl_AppendResult(interp, "missing arg after \"", string,
		    "\"", (char *)NULL);
		return TCL_ERROR;
	    }
	    string = Tcl_GetString(objv[i]);
	    fmt = GetBinaryFormat(interp, string, &size);
	    if (fmt == NF_UNKNOWN) {
		return TCL_ERROR;
	    }
	} else if (strcmp(string, "-at") == 0) {
	    i++;
	    if (i >= objc) {
		Tcl_AppendResult(interp, "missing arg after \"", string,
		    "\"", (char *)NULL);
		return TCL_ERROR;
	    }
	    string = Tcl_GetString(objv[i]);
	    if (Blt_Vec_GetIndex(interp, vPtr, string, &first, 0, 
			 (Blt_VectorIndexProc **)NULL) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (first > vPtr->length) {
		Tcl_AppendResult(interp, "index \"", string,
		    "\" is out of range", (char *)NULL);
		return TCL_ERROR;
	    }
	}
    }

#define BUFFER_SIZE 1024
    if (count == 0) {
	arraySize = BUFFER_SIZE * size;
    } else {
	arraySize = count * size;
    }

    byteArr = Blt_AssertMalloc(arraySize);
    /* FIXME: restore old channel translation later? */
    if (Tcl_SetChannelOption(interp, channel, "-translation",
	    "binary") != TCL_OK) {
	return TCL_ERROR;
    }
    total = 0;
    while (!Tcl_Eof(channel)) {
	bytesRead = Tcl_Read(channel, byteArr, arraySize);
	if (bytesRead < 0) {
	    Tcl_AppendResult(interp, "error reading channel: ",
		Tcl_PosixError(interp), (char *)NULL);
	    return TCL_ERROR;
	}
	if ((bytesRead % size) != 0) {
	    Tcl_AppendResult(interp, "error reading channel: short read",
		(char *)NULL);
	    return TCL_ERROR;
	}
	length = bytesRead / size;
	if (CopyValues(vPtr, byteArr, fmt, size, length, swap, &first)
	    != TCL_OK) {
	    return TCL_ERROR;
	}
	total += length;
	if (count > 0) {
	    break;
	}
    }
    Blt_Free(byteArr);

    if (vPtr->flush) {
	Blt_Vec_FlushCache(vPtr);
    }
    Blt_Vec_UpdateClients(vPtr);

    /* Set the result as the number of values read.  */
    Tcl_SetIntObj(Tcl_GetObjResult(interp), total);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExportOp --
 *
 *	Exports the vector as a binary bytearray.
 *
 * Results:
 *	A standard TCL result.  
 *
 *	vecName bytearray ?-empty bool -format float|double -from -to?
 *	vecName bytearray obj
 *
 *	vecName import float -data obj -file file ?switches?
 *	vecName export float -data obj -file file 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ExportOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    ExportSwitches switches;
    size_t numValues;
    char *fmt;
    int format;
    Blt_DBuffer dbuffer;
    int result;

#define FMT_FLOAT	0
#define FMT_DOUBLE	1
    memset(&switches, 0, sizeof(switches));
    switches.from = 0;
    switches.to = vPtr->length - 1;
    switches.empty = Blt_NaN();
    indexSwitch.clientData = vPtr;
    fmt = Tcl_GetString(objv[2]);
    if (strcmp(fmt, "double") == 0) {
	format = FMT_DOUBLE;
    } else if (strcmp(fmt, "float") == 0) {
	format = FMT_FLOAT;
    } else {
	Tcl_AppendResult(interp, "unknown export format \"", fmt, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, 
			  &switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    numValues = switches.to - switches.from + 1;
    dbuffer = Blt_DBuffer_Create();
    if (format == FMT_DOUBLE) {
	double *darray;
	size_t count;

	Blt_DBuffer_SetLength(dbuffer, numValues * sizeof(double));
	darray = (double *)Blt_DBuffer_Bytes(dbuffer);
	count = 0;
	if (switches.empty) {
	    long i;

	    for (i = switches.from; i <= switches.to; i++) {
		darray[count] = vPtr->valueArr[i];
		count++;
	    }
	} else {
	    long i;

	    for (i = switches.from; i <= switches.to; i++) {
		if (FINITE(vPtr->valueArr[i])) {
		    darray[count] = vPtr->valueArr[i];
		    count++;
		}
	    }
	}
	Blt_DBuffer_SetLength(dbuffer, count * sizeof(double));
    } else if (format == FMT_FLOAT) {
	float *farray;
	size_t count;

	Blt_DBuffer_SetLength(dbuffer, numValues * sizeof(float));
	farray = (float *)Blt_DBuffer_Bytes(dbuffer);
	count = 0;
	if (switches.empty) {
	    long i;

	    for (i = switches.from; i <= switches.to; i++) {
		farray[count] = (float)vPtr->valueArr[i];
		count++;
	    }
	} else {
	    long i;

	    for (i = switches.from; i <= switches.to; i++) {
		if (FINITE(vPtr->valueArr[i])) {
		    farray[count] = (float)vPtr->valueArr[i];
		    count++;
		}
	    }
	}
	Blt_DBuffer_SetLength(dbuffer, count * sizeof(float));
    }
    if (switches.fileObjPtr != NULL) {
	const char *fileName;

	/* Write the image into the designated file. */
	fileName = Tcl_GetString(switches.fileObjPtr);
	result = Blt_DBuffer_SaveFile(interp, fileName, dbuffer);
    } else if (switches.dataObjPtr != NULL) {
	Tcl_Obj *objPtr;

	/* Write the image into the designated TCL variable. */
	objPtr = Tcl_ObjSetVar2(interp, switches.dataObjPtr, NULL, 
		Blt_DBuffer_ByteArrayObj(dbuffer), 0);
	result = (objPtr == NULL) ? TCL_ERROR : TCL_OK;
    } else {
	Tcl_Obj *objPtr;

	/* Return the image as a base64 string in the interpreter result. */
	result = TCL_ERROR;
	objPtr = Blt_DBuffer_Base64EncodeToObj(interp, dbuffer);
	if (objPtr != NULL) {
	    Tcl_SetObjResult(interp, objPtr);
	    result = TCL_OK;
	}
    }
    Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
    Blt_DBuffer_Destroy(dbuffer);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * CountOp --
 *
 *	Returns the number of values in the vector.  This excludes
 *	empty values.
 *
 * Results:
 *	A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
static int
CountOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int count;
    const char *string;
    char c;

    string = Tcl_GetString(objv[2]);
    c = string[0];
    count = 0;
    if ((c == 'e') && (strcmp(string, "empty") == 0)) {
	int i;

	for (i = 0; i < vPtr->length; i++) {
	    if (!FINITE(vPtr->valueArr[i])) {
		count++;
	    }
	}
    } else if ((c == 'z') && (strcmp(string, "zero") == 0)) {
	int i;

	for (i = 0; i < vPtr->length; i++) {
	    if (FINITE(vPtr->valueArr[i]) && (vPtr->valueArr[i] == 0.0)) {
		count++;
	    }
	}
    } else if ((c == 'n') && (strcmp(string, "nonzero") == 0)) {
	int i;

	for (i = 0; i < vPtr->length; i++) {
	    if (FINITE(vPtr->valueArr[i]) && (vPtr->valueArr[i] != 0.0)) {
		count++;
	    }
	}
    } else if ((c == 'n') && (strcmp(string, "nonempty") == 0)) {
	int i;

	for (i = 0; i < vPtr->length; i++) {
	    if (FINITE(vPtr->valueArr[i])) {
		count++;
	    }
	}
    } else {
	Tcl_AppendResult(interp, "unknown operation \"", string, 
		"\": should be empty, zero, nonzero, or nonempty",
		(char *)NULL);
	return TCL_ERROR;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), count);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * IndicesOp --
 *
 *	Returns the indices of values in the vector.  This excludes
 *	empty values.
 *
 * Results:
 *	A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
static int
IndicesOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    const char *string;
    char c;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    string = Tcl_GetString(objv[2]);
    c = string[0];
    if ((c == 'e') && (strcmp(string, "empty") == 0)) {
	int i;

	for (i = 0; i < vPtr->length; i++) {
	    if (!FINITE(vPtr->valueArr[i])) {
		Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(i));
	    }
	}
    } else if ((c == 'z') && (strcmp(string, "zero") == 0)) {
	int i;

	for (i = 0; i < vPtr->length; i++) {
	    if (FINITE(vPtr->valueArr[i]) && (vPtr->valueArr[i] == 0.0)) {
		Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(i));
	    }
	}
    } else if ((c == 'n') && (strcmp(string, "nonzero") == 0)) {
	int i;

	for (i = 0; i < vPtr->length; i++) {
	    if (FINITE(vPtr->valueArr[i]) && (vPtr->valueArr[i] != 0.0)) {
		Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(i));
	    }
	}
    } else if ((c == 'n') && (strcmp(string, "nonempty") == 0)) {
	int i;

	for (i = 0; i < vPtr->length; i++) {
	    if (FINITE(vPtr->valueArr[i])) {
		Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(i));
	    }
	}
    } else {
	Tcl_AppendResult(interp, "unknown operation \"", string, 
		"\": should be empty, zero, nonzero, or nonempty",
		(char *)NULL);
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SearchOp --
 *
 *	Searches for a value in the vector. Returns the indices of all vector
 *	elements matching a particular value.
 *
 * Results:
 *	Always returns TCL_OK.  interp->result will contain a list of the
 *	indices of array elements matching value. If no elements match,
 *	interp->result will contain the empty string.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SearchOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    double min, max;
    int i;
    int wantValue;
    char *string;
    Tcl_Obj *listObjPtr;

    wantValue = FALSE;
    string = Tcl_GetString(objv[2]);
    if ((string[0] == '-') && (strcmp(string, "-value") == 0)) {
	wantValue = TRUE;
	objv++, objc--;
    }
    if (Blt_ExprDoubleFromObj(interp, objv[2], &min) != TCL_OK) {
	return TCL_ERROR;
    }
    max = min;
    if (objc > 4) {
 	Tcl_AppendResult(interp, "wrong # arguments: should be \"",
		Tcl_GetString(objv[0]), " search ?-value? min ?max?", 
		(char *)NULL);
	return TCL_ERROR;
    }
    if ((objc > 3) && 
	(Blt_ExprDoubleFromObj(interp, objv[3], &max) != TCL_OK)) {
	return TCL_ERROR;
    }
    if ((min - max) >= DBL_EPSILON) {
	return TCL_OK;		/* Bogus range. Don't bother looking. */
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (wantValue) {
	for (i = 0; i < vPtr->length; i++) {
	    if (InRange(vPtr->valueArr[i], min, max)) {
		Tcl_ListObjAppendElement(interp, listObjPtr, 
			Tcl_NewDoubleObj(vPtr->valueArr[i]));
	    }
	}
    } else {
	for (i = 0; i < vPtr->length; i++) {
	    if (InRange(vPtr->valueArr[i], min, max)) {
		Tcl_ListObjAppendElement(interp, listObjPtr,
			 Tcl_NewIntObj(i + vPtr->offset));
	    }
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OffsetOp --
 *
 *	Queries or sets the offset of the array index from the base address of
 *	the data array of values.
 *
 * Results:
 *	A standard TCL result.  If the source vector doesn't exist or the
 *	source list is not a valid list of numbers, TCL_ERROR returned.
 *	Otherwise TCL_OK is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
OffsetOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    if (objc == 3) {
	int newOffset;

	if (Tcl_GetIntFromObj(interp, objv[2], &newOffset) != TCL_OK) {
	    return TCL_ERROR;
	}
	vPtr->offset = newOffset;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), vPtr->offset);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RandomOp --
 *
 *	Generates random values for the length of the vector.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RandomOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int i;

    if (objc == 3) {
	long seed;

	if (Blt_GetLongFromObj(interp, objv[2], &seed) != TCL_OK) {
	    return TCL_ERROR;
	}
	srand48(seed);
    }
    for (i = 0; i < vPtr->length; i++) {
	vPtr->valueArr[i] = drand48();
    }
    if (vPtr->flush) {
	Blt_Vec_FlushCache(vPtr);
    }
    Blt_Vec_UpdateClients(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SequenceOp --
 *
 *	Generates a sequence of values in the vector.
 *
 * Results:
 *	A standard TCL result.
 *
 *	$v sequence start stop ?step?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SequenceOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    const char *string;
    double start, stop, step;
    int i, numSteps;

    if (Blt_ExprDoubleFromObj(interp, objv[2], &start) != TCL_OK) {
	return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]);
    step = 1.0;
    stop = 0.0;                         /* Suppress compiler warning. */
    numSteps = 0;
    if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
	numSteps = vPtr->length;
    } else if (Blt_ExprDoubleFromObj(interp, objv[3], &stop) != TCL_OK) {
	return TCL_ERROR;
    }
    step = 1.0;				/* By default, increment is 1.0 */
    if ((objc > 4) && 
	(Blt_ExprDoubleFromObj(interp, objv[4], &step) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (numSteps == 0) {
	numSteps = (int)((stop - start) / step) + 1;
    }
    if (numSteps > 0) {
	if (Blt_Vec_SetLength(interp, vPtr, numSteps) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (i = 0; i < numSteps; i++) {
	    vPtr->valueArr[i] = start + (step * (double)i);
	}
	if (vPtr->flush) {
	    Blt_Vec_FlushCache(vPtr);
	}
	Blt_Vec_UpdateClients(vPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LinspaceOp --
 *
 *	Generate linearly spaced values.
 *
 * Results:
 *	A standard TCL result.
 *
 *	$v linspace first last ?numSteps?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
LinspaceOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int numSteps;
    double start, stop;
    
    if (Blt_ExprDoubleFromObj(interp, objv[2], &start) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_ExprDoubleFromObj(interp, objv[3], &stop) != TCL_OK) {
	return TCL_ERROR;
    }
    numSteps = vPtr->length;		/* By default, generate one step
					 * per element in the vector. */
    if ((objc > 4) && 
	(Blt_ExprIntFromObj(interp, objv[4], &numSteps) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (numSteps > 1) {			/* Silently ignore non-positive
					 * numSteps */
	int i;
	double step;

	if (Blt_Vec_SetLength(interp, vPtr, numSteps) != TCL_OK) {
	    return TCL_ERROR;
	}
	step = (stop - start) / (double)(numSteps - 1);
	for (i = 0; i < numSteps; i++) { 
	    vPtr->valueArr[i] = start + (step * i);
	}
	if (vPtr->flush) {
	    Blt_Vec_FlushCache(vPtr);
	}
	Blt_Vec_UpdateClients(vPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetOp --
 *
 *	Sets the data of the vector object from a list of values.
 *
 * Results:
 *	A standard TCL result.  If the source vector doesn't exist or the
 *	source list is not a valid list of numbers, TCL_ERROR returned.
 *	Otherwise TCL_OK is returned.
 *
 * Side Effects:
 *	The vector data is reset.  Clients of the vector are notified.  Any
 *	cached array indices are flushed.
 *
 *
 *	vecName set $list 
 *	vecName set anotherVector
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SetOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int result;
    Vector *v2Ptr;
    int numElem;
    Tcl_Obj **elemObjArr;

    /* The source can be either a list of numbers or another vector.  */

    v2Ptr = Blt_Vec_ParseElement((Tcl_Interp *)NULL, vPtr->dataPtr, 
	   Tcl_GetString(objv[2]), NULL, NS_SEARCH_BOTH);
    if (v2Ptr != NULL) {
	if (vPtr == v2Ptr) {
	    Vector *tmpPtr;
	    /* 
	     * Source and destination vectors are the same.  Copy the source
	     * first into a temporary vector to avoid memory overlaps.
	     */
	    tmpPtr = Blt_Vec_New(vPtr->dataPtr);
	    result = Blt_Vec_Duplicate(tmpPtr, v2Ptr);
	    if (result == TCL_OK) {
		result = Blt_Vec_Duplicate(vPtr, tmpPtr);
	    }
	    Blt_Vec_Free(tmpPtr);
	} else {
	    result = Blt_Vec_Duplicate(vPtr, v2Ptr);
	}
    } else if (Tcl_ListObjGetElements(interp, objv[2], &numElem, &elemObjArr) 
	       == TCL_OK) {
	result = CopyList(vPtr, interp, numElem, elemObjArr);
    } else {
	return TCL_ERROR;
    }

    if (result == TCL_OK) {
	/*
	 * The vector has changed; so flush the array indices (they're wrong
	 * now), find the new range of the data, and notify the vector's
	 * clients that it's been modified.
	 */
	if (vPtr->flush) {
	    Blt_Vec_FlushCache(vPtr);
	}
	Blt_Vec_UpdateClients(vPtr);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * SimplifyOp --
 *
 *	Sets the data of the vector object from a list of values.
 *
 * Results:
 *	A standard TCL result.  If the source vector doesn't exist or the
 *	source list is not a valid list of numbers, TCL_ERROR returned.
 *	Otherwise TCL_OK is returned.
 *
 * Side Effects:
 *	The vector data is reset.  Clients of the vector are notified.  Any
 *	cached array indices are flushed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SimplifyOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    size_t i, n;
    int numPoints;
    int *simple;
    double tolerance = 10.0;
    Point2d *orig, *reduced;

    numPoints = vPtr->length / 2;
    simple  = Blt_AssertMalloc(numPoints * sizeof(int));
    reduced = Blt_AssertMalloc(numPoints * sizeof(Point2d));
    orig = (Point2d *)vPtr->valueArr;
    n = Blt_SimplifyLine(orig, 0, numPoints - 1, tolerance, simple);
    for (i = 0; i < n; i++) {
	reduced[i] = orig[simple[i]];
    }
    Blt_Free(simple);
    Blt_Vec_Reset(vPtr, (double *)reduced, n * 2, vPtr->length, TCL_DYNAMIC);
    /*
     * The vector has changed; so flush the array indices (they're wrong now),
     * find the new range of the data, and notify the vector's clients that
     * it's been modified.
     */
    if (vPtr->flush) {
	Blt_Vec_FlushCache(vPtr);
    }
    Blt_Vec_UpdateClients(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SplitOp --
 *
 *	Copies the values from the vector evenly into one of more vectors.
 *
 * Results:
 *	A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SplitOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int numVectors;

    numVectors = objc - 2;
    if ((vPtr->length % numVectors) != 0) {
	Tcl_AppendResult(interp, "can't split vector \"", vPtr->name, 
	   "\" into ", Blt_Itoa(numVectors), " even parts.", (char *)NULL);
	return TCL_ERROR;
    }
    if (numVectors > 0) {
	Vector *v2Ptr;
	char *string;		/* Name of vector. */
	int i, j, k;
	int oldSize, newSize, extra, isNew;

	extra = vPtr->length / numVectors;
	for (i = 0; i < numVectors; i++) {
	    string = Tcl_GetString(objv[i+2]);
	    v2Ptr = Blt_Vec_Create(vPtr->dataPtr, string, string, string,
		&isNew);
	    oldSize = v2Ptr->length;
	    newSize = oldSize + extra;
	    if (Blt_Vec_SetLength(interp, v2Ptr, newSize) != TCL_OK) {
		return TCL_ERROR;
	    }
	    for (j = i, k = oldSize; j < vPtr->length; j += numVectors, k++) {
		v2Ptr->valueArr[k] = vPtr->valueArr[j];
	    }
	    Blt_Vec_UpdateClients(v2Ptr);
	    if (v2Ptr->flush) {
		Blt_Vec_FlushCache(v2Ptr);
	    }
	}
    }
    return TCL_OK;
}


static Vector **sortVectors;		/* Pointer to the array of values
					 * currently being sorted. */
static int numSortVectors;
static int sortDecreasing;		/* Indicates the ordering of the
					 * sort. If non-zero, the vectors are
					 * sorted in decreasing order. */

static int
CompareVectors(void *a, void *b)
{
    double delta;
    int i;
    int sign;
    Vector *vPtr;

    sign = (sortDecreasing) ? -1 : 1;
    for (i = 0; i < numSortVectors; i++) {
	vPtr = sortVectors[i];
	delta = vPtr->valueArr[*(int *)a] - vPtr->valueArr[*(int *)b];
	if (delta < 0.0) {
	    return (-1 * sign);
	} else if (delta > 0.0) {
	    return (1 * sign);
	}
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_SortMap --
 *
 *	Returns an array of indices that represents the sorted mapping of the
 *	original vector.
 *
 * Results:
 *	A standard TCL result.  If any of the auxiliary vectors are a
 *	different size than the sorted vector object, TCL_ERROR is returned.
 *	Otherwise TCL_OK is returned.
 *
 * Side Effects:
 *	The vectors are sorted.
 *
 *	vecName sort ?switches? vecName vecName...
 *---------------------------------------------------------------------------
 */
void
Blt_Vec_SortMap(Vector **vectors, int numVectors, size_t *sortLengthPtr, 
	size_t **mapPtr)
{
    size_t *map;
    int i;
    Vector *vPtr = *vectors;
    size_t count;

    map = Blt_AssertMalloc(sizeof(size_t) * vPtr->length);
    count = 0;
    for (i = 0; i < vPtr->length; i++) {
	if (FINITE(vPtr->valueArr[i])) {
	    map[count] = i;
	    count++;
	}
    }
    /* Set global variables for sorting routine. */
    sortVectors = vectors;
    numSortVectors = numVectors;
    qsort((char *)map, count, sizeof(size_t), 
	  (QSortCompareProc *)CompareVectors);
    *sortLengthPtr = count;
    *mapPtr = map;
}

static int
SortVectors(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv,
	    size_t *sortLengthPtr, size_t **sortMapPtr)
{
    Vector **vectors, *v2Ptr;
    int i;

    vectors = Blt_AssertMalloc(sizeof(Vector *) * (objc + 1));
    vectors[0] = vPtr;
    for (i = 0; i < objc; i++) {
	if (Blt_Vec_LookupName(vPtr->dataPtr, Tcl_GetString(objv[i]), 
		&v2Ptr) != TCL_OK) {
	    goto error;
	}
	if (v2Ptr->length != vPtr->length) {
	    Tcl_AppendResult(interp, "vector \"", v2Ptr->name,
		"\" is not the same size as \"", vPtr->name, "\"",
		(char *)NULL);
	    goto error;
	}
	vectors[i + 1] = v2Ptr;
    }
    Blt_Vec_SortMap(vectors, objc + 1, sortLengthPtr, sortMapPtr);
    Blt_Free(vectors);
    return TCL_OK;
  error:
    Blt_Free(vectors);
    return TCL_ERROR;
}


/*
 *---------------------------------------------------------------------------
 *
 * SortOp --
 *
 *	Sorts the vector object and any other vectors according to sorting
 *	order of the vector object.
 *
 * Results:
 *	A standard TCL result.  If any of the auxiliary vectors are a
 *	different size than the sorted vector object, TCL_ERROR is returned.
 *	Otherwise TCL_OK is returned.
 *
 * Side Effects:
 *	The vectors are sorted.
 *
 *	vecName sort ?switches? vecName vecName...
 *---------------------------------------------------------------------------
 */
static int
SortOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Vector *v2Ptr;
    double *copy;
    size_t *map;
    size_t sortLength;
    size_t numBytes;
    int result;
    int i;
    SortSwitches switches;

    sortDecreasing = FALSE;
    switches.flags = 0;
    i = Blt_ParseSwitches(interp, sortSwitches, objc - 2, objv + 2, &switches, 
		BLT_SWITCH_OBJV_PARTIAL);
    if (i < 0) {
	return TCL_ERROR;
    }
    objc -= i, objv += i;
    sortDecreasing = (switches.flags & SORT_DECREASING);
    sortLength = 0;
    if (objc > 2) {
	if (SortVectors(vPtr, interp, objc - 2, objv + 2, &sortLength, &map)
	    != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	Blt_Vec_SortMap(&vPtr, 1, &sortLength, &map);
    }
    if (map == NULL) {
	return TCL_ERROR;
    }
    if (switches.flags & SORT_UNIQUE) {
	int count;

	count = 1;
	for (i = 1; i < sortLength; i++) {
	    size_t next, prev;

	    next = map[i];
	    prev = map[i - 1];
	    if (vPtr->valueArr[next] != vPtr->valueArr[prev]) {
		map[count] = next;
		count++;
	    }
	}
	sortLength = count;
    }
    if (objc == 2) {
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	if (switches.flags & SORT_INDICES) {
	    for (i = 0; i < sortLength; i++) {
		Tcl_Obj *objPtr;
		
		objPtr = Tcl_NewLongObj(map[i]);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	} else {
	    for (i = 0; i < sortLength; i++) {
		Tcl_Obj *objPtr;
		
		objPtr = Tcl_NewDoubleObj(vPtr->valueArr[map[i]]);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	}
	Blt_Free(map);
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }

    /*
     * Create an array to store a copy of the current values of the
     * vector. We'll merge the values back into the vector based upon the
     * indices found in the index array.
     */
    numBytes = sizeof(double) * vPtr->length;
    copy = Blt_AssertMalloc(numBytes);

    /* Now sort the designated vectors according to the sort map.  The vectors
     * must be the same size as the map though.  */
    result = TCL_ERROR;
    for (i = 2; i < objc; i++) {
	int j;
	const char *name;

	name = Tcl_GetString(objv[i]);
	if (Blt_Vec_LookupName(vPtr->dataPtr, name, &v2Ptr) != TCL_OK) {
	    goto error;
	}
	memcpy((char *)copy, (char *)v2Ptr->valueArr, numBytes);
	if (sortLength != v2Ptr->length) {
	    Blt_Vec_SetLength(interp, v2Ptr, sortLength);
	}
	for (j = 0; j < sortLength; j++) {
	    v2Ptr->valueArr[j] = copy[map[j]];
	}
	Blt_Vec_UpdateClients(v2Ptr);
	if (v2Ptr->flush) {
	    Blt_Vec_FlushCache(v2Ptr);
	}
    }
    result = TCL_OK;
  error:
    Blt_Free(copy);
    Blt_Free(map);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * InstExprOp --
 *
 *	Computes the result of the expression which may be either a scalar
 *	(single value) or vector (list of values).
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InstExprOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{

    if (Blt_ExprVector(interp, Tcl_GetString(objv[2]), (Blt_Vector *)vPtr) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    if (vPtr->flush) {
	Blt_Vec_FlushCache(vPtr);
    }
    Blt_Vec_UpdateClients(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArithOp --
 *
 * Results:
 *	A standard TCL result.  If the source vector doesn't exist or the
 *	source list is not a valid list of numbers, TCL_ERROR returned.
 *	Otherwise TCL_OK is returned.
 *
 * Side Effects:
 *	The vector data is reset.  Clients of the vector are notified.
 *	Any cached array indices are flushed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ArithOp(Vector *vPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    double value;
    int i;
    Vector *v2Ptr;
    double scalar;
    Tcl_Obj *listObjPtr;
    const char *string;

    v2Ptr = Blt_Vec_ParseElement((Tcl_Interp *)NULL, vPtr->dataPtr, 
	Tcl_GetString(objv[2]), NULL, NS_SEARCH_BOTH);
    if (v2Ptr != NULL) {
	int j;
	int length;

	length = v2Ptr->length;
	if (length != vPtr->length) {
	    Tcl_AppendResult(interp, "vectors \"", Tcl_GetString(objv[0]), 
		"\" and \"", Tcl_GetString(objv[2]), 
		"\" are not the same length", (char *)NULL);
	    return TCL_ERROR;
	}
	string = Tcl_GetString(objv[1]);
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	switch (string[0]) {
	case '*':
	    for (i = 0, j = 0; i < vPtr->length; i++, j++) {
		value = vPtr->valueArr[i] * v2Ptr->valueArr[j];
		Tcl_ListObjAppendElement(interp, listObjPtr,
			 Tcl_NewDoubleObj(value));
	    }
	    break;

	case '/':
	    for (i = 0, j = 0; i < vPtr->length; i++, j++) {
		value = vPtr->valueArr[i] / v2Ptr->valueArr[j];
		Tcl_ListObjAppendElement(interp, listObjPtr,
			 Tcl_NewDoubleObj(value));
	    }
	    break;

	case '-':
	    for (i = 0, j = 0; i < vPtr->length; i++, j++) {
		value = vPtr->valueArr[i] - v2Ptr->valueArr[j];
		Tcl_ListObjAppendElement(interp, listObjPtr,
			 Tcl_NewDoubleObj(value));
	    }
	    break;

	case '+':
	    for (i = 0, j = 0; i < vPtr->length; i++, j++) {
		value = vPtr->valueArr[i] + v2Ptr->valueArr[j];
		Tcl_ListObjAppendElement(interp, listObjPtr,
			 Tcl_NewDoubleObj(value));
	    }
	    break;
	}
	Tcl_SetObjResult(interp, listObjPtr);

    } else if (Blt_ExprDoubleFromObj(interp, objv[2], &scalar) == TCL_OK) {
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	string = Tcl_GetString(objv[1]);
	switch (string[0]) {
	case '*':
	    for (i = 0; i < vPtr->length; i++) {
		value = vPtr->valueArr[i] * scalar;
		Tcl_ListObjAppendElement(interp, listObjPtr,
			 Tcl_NewDoubleObj(value));
	    }
	    break;

	case '/':
	    for (i = 0; i < vPtr->length; i++) {
		value = vPtr->valueArr[i] / scalar;
		Tcl_ListObjAppendElement(interp, listObjPtr,
			 Tcl_NewDoubleObj(value));
	    }
	    break;

	case '-':
	    for (i = 0; i < vPtr->length; i++) {
		value = vPtr->valueArr[i] - scalar;
		Tcl_ListObjAppendElement(interp, listObjPtr,
			 Tcl_NewDoubleObj(value));
	    }
	    break;

	case '+':
	    for (i = 0; i < vPtr->length; i++) {
		value = vPtr->valueArr[i] + scalar;
		Tcl_ListObjAppendElement(interp, listObjPtr,
			 Tcl_NewDoubleObj(value));
	    }
	    break;
	}
	Tcl_SetObjResult(interp, listObjPtr);
    } else {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorInstCmd --
 *
 *	Parses and invokes the appropriate vector instance command option.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec vectorInstOps[] =
{
    {"*",         1, ArithOp,     3, 3, "item",},	/*Deprecated*/
    {"+",         1, ArithOp,     3, 3, "item",},	/*Deprecated*/
    {"-",         1, ArithOp,     3, 3, "item",},	/*Deprecated*/
    {"/",         1, ArithOp,     3, 3, "item",},	/*Deprecated*/
    {"append",    1, AppendOp,    3, 0, "item ?item...?",},
    {"binread",   2, BinreadOp,   3, 0, "channel ?numValues? ?flags?",},
    {"clear",     2, ClearOp,     2, 2, "",},
    {"count",     2, CountOp,     3, 3, "what",},
    {"delete",    2, DeleteOp,    2, 0, "index ?index...?",},
    {"duplicate", 2, DupOp,       2, 3, "?vecName?",},
    {"export",    4, ExportOp,    3, 0, "format ?switches?",},
    {"expr",      4, InstExprOp,  3, 3, "expression",},
    {"fft",	  2, FFTOp,	  3, 0, "vecName ?switches?",},
    {"frequency", 2, FrequencyOp, 4, 4, "vecName numBins",},
    {"indices",   3, IndicesOp,   3, 3, "what",},
    {"inversefft",3, InverseFFTOp,4, 4, "vecName vecName",},
    {"length",    2, LengthOp,    2, 3, "?newSize?",},
    {"limits",    3, LimitsOp,    2, 2, "",},
    {"linspace",  3, LinspaceOp, 4, 5, "first last ?numSteps?",},
    {"maximum",   2, MaxOp,       2, 2, "",},
    {"merge",     2, MergeOp,     3, 0, "vecName ?vecName...?",},
    {"minimum",   2, MinOp,       2, 2, "",},
    {"normalize", 3, NormalizeOp, 2, 3, "?vecName?",},	/*Deprecated*/
    {"notify",    3, NotifyOp,    3, 3, "keyword",},
    {"offset",    1, OffsetOp,    2, 3, "?offset?",},
    {"populate",  2, PopulateOp,  4, 4, "vecName density",},
    {"print",     2, PrintOp,     3, 0, "format ?switches?",},
    {"random",    4, RandomOp,    2, 3, "?seed?",},	/*Deprecated*/
    {"range",     4, RangeOp,     2, 4, "first last",},
    {"search",    3, SearchOp,    3, 5, "?-value? value ?value?",},
    {"sequence",  3, SequenceOp,  4, 5, "start stop ?step?",},
    {"set",       3, SetOp,       3, 3, "list",},
    {"simplify",  2, SimplifyOp,  2, 2, },
    {"sort",      2, SortOp,      2, 0, "?switches? ?vecName...?",},
    {"split",     2, SplitOp,     2, 0, "?vecName...?",},
    {"value",     5, ValueOp,     2, 0, "oper",},
    {"values",    6, ValuesOp,    2, 0, "?switches?",},
    {"variable",  3, MapOp,       2, 3, "?varName?",},
};

static int numInstOps = sizeof(vectorInstOps) / sizeof(Blt_OpSpec);

int
Blt_Vec_InstCmd(ClientData clientData, Tcl_Interp *interp, int objc,
		Tcl_Obj *const *objv)
{
    VectorCmdProc *proc;
    Vector *vPtr = clientData;

    vPtr->first = 0;
    vPtr->last = vPtr->length - 1;
    proc = Blt_GetOpFromObj(interp, numInstOps, vectorInstOps, BLT_OP_ARG1, objc,
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (vPtr, interp, objc, objv);
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_VarTrace --
 *
 * Results:
 *	Returns NULL on success.  Only called from a variable trace.
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------
 */
char *
Blt_Vec_VarTrace(ClientData clientData, Tcl_Interp *interp, const char *part1, 
		 const char *part2, int flags)
{
    Blt_VectorIndexProc *indexProc;
    Vector *vPtr = clientData;
    int first, last;
    int varFlags;
#define MAX_ERR_MSG	1023
    static char message[MAX_ERR_MSG + 1];

    if (part2 == NULL) {
	if (flags & TCL_TRACE_UNSETS) {
	    Blt_Free(vPtr->arrayName);
	    vPtr->arrayName = NULL;
	    if (vPtr->freeOnUnset) {
		Blt_Vec_Free(vPtr);
	    }
	}
	return NULL;
    }
    if (Blt_Vec_GetIndexRange(interp, vPtr, part2, INDEX_ALL_FLAGS, &indexProc)
	 != TCL_OK) {
	goto error;
    }
    first = vPtr->first, last = vPtr->last;
    varFlags = TCL_LEAVE_ERR_MSG | (TCL_GLOBAL_ONLY & flags);
    if (flags & TCL_TRACE_WRITES) {
	double value;
	Tcl_Obj *objPtr;

	if (first == SPECIAL_INDEX) { /* Tried to set "min" or "max" */
	    return (char *)"read-only index";
	}
	objPtr = Tcl_GetVar2Ex(interp, part1, part2, varFlags);
	if (objPtr == NULL) {
	    goto error;
	}
	if (Blt_ExprDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
	    if ((last == first) && (first >= 0)) {
		/* Single numeric index. Reset the array element to
		 * its old value on errors */
		Tcl_SetVar2Ex(interp, part1, part2, objPtr, varFlags);
	    }
	    goto error;
	}
	if (first == vPtr->length) {
	    if (Blt_Vec_ChangeLength((Tcl_Interp *)NULL, vPtr, vPtr->length + 1)
		 != TCL_OK) {
		return (char *)"error resizing vector";
	    }
	}
	/* Set possibly an entire range of values */
	ReplicateValue(vPtr, first, last, value);
    } else if (flags & TCL_TRACE_READS) {
	double value;
	Tcl_Obj *objPtr;

	if (vPtr->length == 0) {
	    if (Tcl_SetVar2(interp, part1, part2, "", varFlags) == NULL) {
		goto error;
	    }
	    return NULL;
	}
	if  (first == vPtr->length) {
	    return (char *)"write-only index";
	}
	if (first == last) {
	    if (first >= 0) {
		value = vPtr->valueArr[first];
	    } else {
		vPtr->first = 0, vPtr->last = vPtr->length - 1;
		value = (*indexProc) ((Blt_Vector *) vPtr);
	    }
	    objPtr = Tcl_NewDoubleObj(value);
	    if (Tcl_SetVar2Ex(interp, part1, part2, objPtr, varFlags) == NULL) {
		Tcl_DecrRefCount(objPtr);
		goto error;
	    }
	} else {
	    objPtr = GetValues(vPtr, first, last);
	    if (Tcl_SetVar2Ex(interp, part1, part2, objPtr, varFlags) == NULL) {
		Tcl_DecrRefCount(objPtr);
		goto error;
	    }
	}
    } else if (flags & TCL_TRACE_UNSETS) {
	int i, j;

	if ((first == vPtr->length) || (first == SPECIAL_INDEX)) {
	    return (char *)"special vector index";
	}
	/*
	 * Collapse the vector from the point of the first unset element.
	 * Also flush any array variable entries so that the shift is
	 * reflected when the array variable is read.
	 */
	for (i = first, j = last + 1; j < vPtr->length; i++, j++) {
	    vPtr->valueArr[i] = vPtr->valueArr[j];
	}
	vPtr->length -= ((last - first) + 1);
	if (vPtr->flush) {
	    Blt_Vec_FlushCache(vPtr);
	}
    } else {
	return (char *)"unknown variable trace flag";
    }
    if (flags & (TCL_TRACE_UNSETS | TCL_TRACE_WRITES)) {
	Blt_Vec_UpdateClients(vPtr);
    }
    Tcl_ResetResult(interp);
    return NULL;

 error: 
    strncpy(message, Tcl_GetStringResult(interp), MAX_ERR_MSG);
    message[MAX_ERR_MSG] = '\0';
    return message;
}



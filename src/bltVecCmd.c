/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltVecCmd.c --
 *
 * This module implements vector data objects.
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
 * Code for binary data read operation was donated by Harold Kirsch.
 *
 */

/*
 * TODO:
 *      o Add H. Kirsch's vector binary read operation
 *              x binread file0
 *              x binread -file file0
 *
 *      o Add ASCII/binary file reader
 *              x read fileName
 *
 *      o Allow Tcl-based client notifications.
 *              vector x
 *              x notify call Display
 *              x notify delete Display
 *              x notify reorder #1 #2
 */

#include "bltVecInt.h"
#undef SIGN
#include "tclTomMath.h"

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltOp.h"
#include "bltNsUtil.h"
#include "bltSwitch.h"

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
#define SORT_UNIQUE     (1<<1)
#define SORT_INDICES    (1<<2)
#define SORT_VALUES     (1<<3)

static Blt_SwitchSpec sortSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-decreasing", "", (char *)NULL,
        Blt_Offset(SortSwitches, flags), 0, SORT_DECREASING},
    {BLT_SWITCH_BITMASK, "-indices", "", (char *)NULL,
        Blt_Offset(SortSwitches, flags), 0, SORT_INDICES},
    {BLT_SWITCH_BITMASK, "-reverse", "", (char *)NULL,
        Blt_Offset(SortSwitches, flags), 0, SORT_DECREASING},
    {BLT_SWITCH_BITMASK, "-unique", "", (char *)NULL,
        Blt_Offset(SortSwitches, flags), 0, SORT_UNIQUE},
    {BLT_SWITCH_BITMASK, "-values", "", (char *)NULL,
        Blt_Offset(SortSwitches, flags), 0, SORT_VALUES},
    {BLT_SWITCH_END}
};

typedef struct {
    double delta;
    Vector *imagPtr;                    /* Vector containing imaginary
                                         * part. */
    Vector *freqPtr;                    /* Vector containing frequencies. */
    VectorCmdInterpData *dataPtr;
    int mask;                           /* Flags controlling FFT. */
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
 * GetVector --
 *
 *      Convert a string representing a vector into its vector structure.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
GetVector(Tcl_Interp *interp, VectorCmdInterpData *dataPtr, Tcl_Obj *objPtr,
          Vector **vPtrPtr)
{
    const char *string;
    Vector *vPtr;
    
    string = Tcl_GetString(objPtr);
    if (Blt_Vec_Find(dataPtr, string, &vPtr) != TCL_OK) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find vector \"", string, "\"",
                (char *)NULL);
        }
        return TCL_ERROR;
    }
    *vPtrPtr = vPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewVector --
 *
 *      Convert a string representing a vector into its vector structure.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Vector *
NewVector(VectorCmdInterpData *dataPtr, const char *string, int *isNewPtr)
{
    return Blt_Vec_Create(dataPtr, string, string, string, isNewPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToFFTVector --
 *
 *      Convert a string representing a vector into its vector structure.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToFFTVector(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to report results */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* Name of vector. */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    FFTData *fftPtr = (FFTData *)record;
    Vector **vPtrPtr = (Vector **)(record + offset);

    return GetVector(interp, fftPtr->dataPtr, objPtr, vPtrPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToIndex --
 *
 *      Convert a string representing a vector into its vector structure.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToIndex(
    ClientData clientData,              /* Contains the vector in question
                                         * to verify its length. */
    Tcl_Interp *interp,                 /* Interpreter to report results */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* Name of vector. */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
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
GetValues(Vector *srcPtr, int first, int last)
{ 
    Tcl_Obj *listObjPtr;
    int i;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = first; i <= last; i++) {
        Tcl_Obj *objPtr;
        
        objPtr = Tcl_NewDoubleObj(srcPtr->valueArr[i]);
        Tcl_ListObjAppendElement(srcPtr->interp, listObjPtr, objPtr);
    } 
    return listObjPtr;
}

static void
ReplicateValue(Vector *destPtr, int first, int last, double value)
{ 
    int i;
 
    for (i = first; i <= last; i++) {
        destPtr->valueArr[i] = value; 
    } 
    destPtr->notifyFlags |= UPDATE_RANGE; 
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
AppendObjv(Vector *vPtr, int objc, Tcl_Obj *const *objv)
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
 *      Appends one of more TCL lists of values, or vector objects onto the
 *      end of the current vector object.
 *
 * Results:
 *      A standard TCL result.  If a current vector can't be created, 
 *      resized, any of the named vectors can't be found, or one of lists of
 *      values is invalid, TCL_ERROR is returned.
 *
 * Side Effects:
 *      Clients of current vector will be notified of the change.
 *
 *      vecName append srcName...
 *---------------------------------------------------------------------------
 */
static int
AppendOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
   Vector *destPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        int result;
        Vector *srcPtr;

        /* It's either a vector name of a list of numbers.  */
        srcPtr = Blt_Vec_ParseElement((Tcl_Interp *)NULL, destPtr->dataPtr, 
               Tcl_GetString(objv[i]), (const char **)NULL, NS_SEARCH_BOTH);
        if (srcPtr != NULL) {
            result = AppendVector(destPtr, srcPtr);
        } else {
            int ec;
            Tcl_Obj **ev;

            if (Tcl_ListObjGetElements(interp, objv[i], &ec, &ev) != TCL_OK) {
                return TCL_ERROR;
            }
            result = AppendObjv(destPtr, ec, ev);
        }
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (objc > 2) {
        if (destPtr->flush) {
            Blt_Vec_FlushCache(destPtr);
        }
        Blt_Vec_UpdateClients(destPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ClearOp --
 *
 *      Deletes all the accumulated array indices for the TCL array associated
 *      will the vector.  This routine can be used to free excess memory from
 *      a large vector.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 * Side Effects:
 *      Memory used for the entries of the TCL array variable is freed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ClearOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;

    Blt_Vec_FlushCache(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes the given indices from the vector.  If no indices are
 *      provided the entire vector is deleted.
 *
 * Results:
 *      A standard TCL result.  If any of the given indices is invalid,
 *      interp->result will an error message and TCL_ERROR is returned.
 *
 * Side Effects:
 *      The clients of the vector will be notified of the vector
 *      deletions.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    unsigned char *unsetArr;
    int i, j;
    int count;

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
        const char *string;

        string = Tcl_GetString(objv[i]);
        if (Blt_Vec_GetIndexRange(interp, vPtr, string, 
                (INDEX_COLON | INDEX_CHECK), (Blt_VectorIndexProc **) NULL) 
                != TCL_OK) {
            Blt_Free(unsetArr);
            return TCL_ERROR;
        }
        for (j = vPtr->first; j <= vPtr->last; j++) {
            SetBit(j);                  /* Mark the element for deletion. */
        }
    }
    count = 0;
    for (i = 0; i < vPtr->length; i++) {
        if (GetBit(i)) {
            continue;                   /* Skip marked elements. */
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
 *      Creates one or more duplicates of the vector object.
 *
 * Results:
 *      A standard TCL result.  If a new vector can't be created, or and
 *      existing vector resized, TCL_ERROR is returned.
 *
 * Side Effects:
 *      Clients of existing vectors will be notified of the change.
 *
 *      vecName dup ?newName?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DupOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Vector *srcPtr = clientData;
    Vector *destPtr;
    int isNew;
    const char *dupName;
    
    if (objc == 3) {
        dupName = Tcl_GetString(objv[2]);
    } else {
        dupName ="#auto";
    }
    destPtr = NewVector(srcPtr->dataPtr, dupName, &isNew);
    if (destPtr == NULL) {
        return TCL_ERROR;
    }
    if (destPtr == srcPtr) {
        /* Source and destination are the same */
        return TCL_OK;
    }
    if (Blt_Vec_Duplicate(destPtr, srcPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (destPtr->flush) {
        Blt_Vec_FlushCache(destPtr);
    }
    Blt_Vec_UpdateClients(destPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), destPtr->name, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FrequencyOp --
 *
 *      Fills the destination vector with the frequency counts from the 
 *      source vector.
 *
 * Results:
 *      A standard TCL result.  If a new vector can't be created,
 *      or and existing vector resized, TCL_ERROR is returned.
 *
 *      vecName frequency srcName 10 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FrequencyOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Vector *destPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Blt_HashTable freqTable;
    Vector *srcPtr;
    double range;
    int i, numBins;

    if (GetVector(interp, destPtr->dataPtr, objv[2], &srcPtr) != TCL_OK) {
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
        Blt_HashEntry *hPtr;
        double value, norm;
        int isNew;
        long bin;
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
FFTOp(ClientData clientData, Tcl_Interp *interp, int objc,
      Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    Vector *realVecPtr = NULL;
    FFTData data;
    
    memset(&data, 0, sizeof(data));
    data.delta = 1.0;

    if (GetVector(interp, vPtr->dataPtr, objv[2], &realVecPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (realVecPtr == vPtr) {
        Tcl_AppendResult(interp, "real vector \"", Tcl_GetString(objv[2]), "\"", 
                " can't be the same as the source", (char *)NULL);
        return TCL_ERROR;
    }
    if (Blt_ParseSwitches(interp, fftSwitches, objc - 3, objv + 3, &data, 
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (Blt_Vec_FFT(interp, realVecPtr, data.imagPtr, data.freqPtr, data.delta,
              data.mask, vPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Update bookkeeping. */
    if (realVecPtr->flush) {
        Blt_Vec_FlushCache(realVecPtr);
    }
    Blt_Vec_UpdateClients(realVecPtr);
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
/* 
 *      vecName inversefft srcImag destReal destImag 
 */
static int
InverseFFTOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    Vector *srcImagPtr;
    Vector *destRealPtr;
    Vector *destImagPtr;

    if (GetVector(interp, vPtr->dataPtr, objv[2], &srcImagPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((GetVector(interp, vPtr->dataPtr, objv[3], &destRealPtr) != TCL_OK) ||
        (GetVector(interp, vPtr->dataPtr, objv[4], &destImagPtr) != TCL_OK)) {
        return TCL_ERROR;
    }
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
 *      Returns the length of the vector.  If a new size is given, the vector
 *      is resized to the new vector.
 *
 * Results:
 *      A standard TCL result.  If the new length is invalid, interp->result
 *      will an error message and TCL_ERROR is returned.  Otherwise
 *      interp->result will contain the length of the vector.
 *
 *---------------------------------------------------------------------------
 */
static int
LengthOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;

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
 *      Returns the minimum and maximum value of the vector.
 *
 * Results:
 *      A standard TCL result. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
LimitsOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
 *      Queries or sets the offset of the array index from the base address
 *      of the data array of values.
 *
 * Results:
 *      A standard TCL result.  If the source vector doesn't exist or the
 *      source list is not a valid list of numbers, TCL_ERROR returned.
 *      Otherwise TCL_OK is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MapOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;

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
 *      Returns the maximum value of the vector.
 *
 * Results:
 *      A standard TCL result. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MaxOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;

    Tcl_SetDoubleObj(Tcl_GetObjResult(interp), Blt_Vec_Max(vPtr));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MergeOp --
 *
 *      Merges the values from the given vectors to the current vector.
 *
 * Results:
 *      A standard TCL result.  If any of the given vectors differ in size,
 *      TCL_ERROR is returned.  Otherwise TCL_OK is returned and the
 *      vector data will contain merged values of the given vectors.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MergeOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    Vector **vecArr;
    int refSize, numElem;
    int i;
    double *valuePtr, *valueArr;
    Vector **vPtrPtr;
    
    /* Allocate an array of vector pointers of each vector to be merged in
     * the current vector.  */
    vecArr = Blt_AssertMalloc(sizeof(Vector *) * objc);
    vPtrPtr = vecArr;

    refSize = -1;
    numElem = 0;
    for (i = 2; i < objc; i++) {
        Vector *srcPtr;

        if (GetVector(interp, vPtr->dataPtr, objv[i], &srcPtr) != TCL_OK) {
            Blt_Free(vecArr);
            return TCL_ERROR;
        }
        /* Check that all the vectors are the same length */
        if (refSize < 0) {
            refSize = srcPtr->length;
        } else if (srcPtr->length != refSize) {
            Tcl_AppendResult(vPtr->interp, "vectors \"", vPtr->name,
                "\" and \"", srcPtr->name, "\" differ in length",
                (char *)NULL);
            Blt_Free(vecArr);
            return TCL_ERROR;
        }
        *vPtrPtr++ = srcPtr;
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
 *      Returns the minimum value of the vector.
 *
 * Results:
 *      A standard TCL result. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MinOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;

    Tcl_SetDoubleObj(Tcl_GetObjResult(interp), Blt_Vec_Min(vPtr));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NormalizeOp --
 *
 *      Normalizes the vector.
 *
 * Results:
 *      A standard TCL result.  If the density is invalid, TCL_ERROR is
 *      returned.  Otherwise TCL_OK is returned.
 *
 *      vecName normalize ?destName?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NormalizeOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    int i;
    double range;
    
    Blt_Vec_UpdateRange(vPtr);
    range = vPtr->max - vPtr->min;
    if (objc > 2) {
        Vector *destPtr;

        if (GetVector(interp, vPtr->dataPtr, objv[2], &destPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Blt_Vec_SetLength(interp, destPtr, vPtr->length) != TCL_OK) {
            return TCL_ERROR;
        }
        for (i = 0; i < vPtr->length; i++) {
            destPtr->valueArr[i] = (vPtr->valueArr[i] - vPtr->min) / range;
        }
        Blt_Vec_UpdateRange(destPtr);
        if (destPtr->flush) {
            Blt_Vec_FlushCache(destPtr);
        }
        Blt_Vec_UpdateClients(destPtr);
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
 *      Notify clients of vector.
 *
 * Results:
 *      A standard TCL result.  If any of the given vectors differ in size,
 *      TCL_ERROR is returned.  Otherwise TCL_OK is returned and the vector
 *      data will contain merged values of the given vectors.
 *
 *  vecName notify now
 *  vecName notify always
 *  vecName notify whenidle
 *  vecName notify update {}
 *  vecName notify delete {}
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NotifyOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
 * PackOp --
 *
 *      Packs the vector, throwing away empty points.
 *
 * Results:
 *      A standard TCL result. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PackOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    int i, j;

    for (i = 0, j = 0; i < vPtr->length; i++) {
        if (FINITE(vPtr->valueArr[i])) {
            if (j < i) {
                vPtr->valueArr[j] = vPtr->valueArr[i];
            }
            j++;
        }
    }
    if (j < i) {
        if (Blt_Vec_SetLength(interp, vPtr, j) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), i - j);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PopulateOp --
 *
 *      Creates or resizes a new vector based upon the density specified.
 *
 * Results:
 *      A standard TCL result.  If the density is invalid, TCL_ERROR
 *      is returned.  Otherwise TCL_OK is returned.
 *
 *      vecName populate srcName density
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PopulateOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    Vector *srcPtr;
    int size, density;
    int i, j;
    double *valuePtr;
    int count;

    if (GetVector(interp, vPtr->dataPtr, objv[2], &srcPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (srcPtr->length == 0) {
        return TCL_OK;                  /* Source vector is empty. */
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &density) != TCL_OK) {
        return TCL_ERROR;
    }
    if (density < 1) {
        Tcl_AppendResult(interp, "bad density \"", Tcl_GetString(objv[3]), 
                "\"", (char *)NULL);
        return TCL_ERROR;
    }
    size = (srcPtr->length - 1) * (density + 1) + 1;
    if (Blt_Vec_SetLength(interp, vPtr, size) != TCL_OK) {
        return TCL_ERROR;
    }
    count = 0;
    valuePtr = vPtr->valueArr;
    for (i = 0; i < (srcPtr->length - 1); i++) {
        double slice, range;

        range = srcPtr->valueArr[i + 1] - srcPtr->valueArr[i];
        slice = range / (double)(density + 1);
        for (j = 0; j <= density; j++) {
            *valuePtr = srcPtr->valueArr[i] + (slice * (double)j);
            valuePtr++;
            count++;
        }
    }
    count++;
    *valuePtr = srcPtr->valueArr[i];    /* Save last value. */
    assert(count == vPtr->length);
    if (vPtr->flush) {
        Blt_Vec_FlushCache(vPtr);
    }
    Blt_Vec_UpdateClients(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ValueGetOp --
 *
 *      Get the value of the index.  This simulates what the vector's
 *      variable does.
 *
 * Results:
 *      A standard TCL result.  If the index is invalid, interp->result
 *      will an error message and TCL_ERROR is returned.  Otherwise
 *      interp->result will contain the values.
 *
 *      vecName value get index 
 *
 *---------------------------------------------------------------------------
 */
static int
ValueGetOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
        return TCL_ERROR;               /* Can't read from index "++end" */
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
 *      Sets the value of the index.  This simulates what the vector's
 *      variable does.
 *
 * Results:
 *      A standard TCL result.  If the index is invalid, interp->result will
 *      an error message and TCL_ERROR is returned.  Otherwise interp->result
 *      will contain the values.
 *
 *---------------------------------------------------------------------------
 */
static int
ValueSetOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
        return TCL_ERROR;               /* Tried to set "min" or "max" */
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
 *      Unsets the value of the index.  This simulates what the vector's
 *      variable does.
 *
 * Results:
 *      A standard TCL result.  If the index is invalid, interp->result will
 *      an error message and TCL_ERROR is returned.  Otherwise interp->result
 *      will contain the values.
 *
 *---------------------------------------------------------------------------
 */
static int
ValueUnsetOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
 *      Parses and invokes the appropriate vector instance command option.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec valueOps[] =
{
    {"get",       1, ValueGetOp,   4, 4, "index",},
    {"set",       1, ValueSetOp,   4, 0, "index value",},
    {"unset",     1, ValueUnsetOp, 3, 0, "?index...?",},
};

static int numValueOps = sizeof(valueOps) / sizeof(Blt_OpSpec);

static int
ValueOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    Tcl_ObjCmdProc *proc;

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
 *      Print the values vector.
 *
 * Results:
 *      A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ValuesOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
 *      This function appends a list of Tcl_Obj's to a Tcl_Obj according to
 *      the formatting instructions embedded in the format string. The
 *      formatting instructions are inspired by sprintf(). Returns TCL_OK
 *      when successful. If there's an error in the arguments, TCL_ERROR is
 *      returned, and an error message is written to the interp, if
 *      non-NULL.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */
#define FMT_ALLOCSEGMENT (1<<0)
#define FMT_CHAR        (1<<1)
#define FMT_HASH        (1<<2)
#define FMT_ISNEGATIVE  (1<<3)
#define FMT_LONG        (1<<4)
#define FMT_LONGLONG    (1<<5)
#define FMT_MINUS       (1<<6)
#define FMT_NEWXPG      (1<<7)
#define FMT_PLUS        (1<<8)
#define FMT_PRECISION   (1<<9)
#define FMT_SEQUENTIAL  (1<<10)
#define FMT_SHORT       (1<<11)
#define FMT_SPACE       (1<<12)
#define FMT_USEWIDE     (1<<13)
#define FMT_WIDTH       (1<<14)
#define FMT_XPG         (1<<15)
#define FMT_ZERO        (1<<16)

typedef struct {
    int precision;                      /* Precision to use. */
    int width;                          /* Minimum field width. */
    unsigned int flags;
    Tcl_UniChar ch;                     /* Last character parsed. */
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
    Tcl_Obj *objPtr;
    short s;
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
        l = (long int)d;
        if (l < 0) {
            parserPtr->flags |= FMT_ISNEGATIVE;
        }
    } else if (parserPtr->flags & FMT_SHORT) {
        s = (short int)d;
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
            continue;                   /* This is an escaped percent. */
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
            case '-': parser.flags |= FMT_MINUS;        break;
            case '#': parser.flags |= FMT_HASH;         break;
            case '0': parser.flags |= FMT_ZERO;         break;
            case ' ': parser.flags |= FMT_SPACE;        break;
            case '+': parser.flags |= FMT_PLUS;         break;
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
 *      Parses a printf-like format string into individual points.
 *
 * Results:
 *      A standard TCL result.  
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
 *      Print the values vector according to the given format.
 *
 * Results:
 *      A standard TCL result.  
 *
 *      $v print $format ?switches?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PrintOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
 *      Returns a TCL list of the range of vector values specified.
 *
 * Results:
 *      A standard TCL result.  If the given range is invalid, TCL_ERROR
 *      is returned.  Otherwise TCL_OK is returned and interp->result
 *      will contain the list of values.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RangeOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
                Tcl_GetString(objv[0]), " range ?first last?\"", (char *)NULL);
        return TCL_ERROR;       
    }
    if (((first == -1) || (last == -1)) && (vPtr->length == 0)) {
        return TCL_OK;                  /* Allow range on empty vector */
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
 *      Determines if a value lies within a given range.
 *
 *      The value is normalized and compared against the interval
 *      [0..1], where 0.0 is the minimum and 1.0 is the maximum.
 *      DBL_EPSILON is the smallest number that can be represented
 *      on the host machine, such that (1.0 + epsilon) != 1.0.
 *
 *      Please note, min cannot be greater than max.
 *
 * Results:
 *      If the value is within of the interval [min..max], 1 is 
 *      returned; 0 otherwise.
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
 *              signed          i1, i2, i4, i8
 *              unsigned        u1, u2, u4, u8
 *              real            r4, r8, r16
 *
 *      There must be a corresponding native type.  For example, this for
 *      reading 2-byte binary integers from an instrument and converting them
 *      to unsigned shorts or ints.
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
 *      Reads binary values from a TCL channel. Values are either appended to
 *      the end of the vector or placed at a given index (using the "-at"
 *      option), overwriting existing values.  Data is read until EOF is found
 *      on the channel or a specified number of values are read.  (note that
 *      this is not necessarily the same as the number of bytes).
 *
 *      The following flags are supported:
 *              -swap           Swap bytes
 *              -at index       Start writing data at the index.
 *              -format fmt     Specifies the format of the data.
 *
 *      This binary reader was created and graciously donated by Harald Kirsch
 *      (kir@iitb.fhg.de).  Anything that's wrong is due to my (gah) munging
 *      of the code.
 *
 * Results:
 *      Returns a standard TCL result. The interpreter result will contain the
 *      number of values (not the number of bytes) read.
 *
 * Caveats:
 *      Channel reads must end on an element boundary.
 *
 *      vecName binread channel count ?switches?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BinreadOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
 *      Exports the vector as a binary bytearray.
 *
 * Results:
 *      A standard TCL result.  
 *
 *      vecName bytearray ?-empty bool -format float|double -from -to?
 *      vecName bytearray obj
 *
 *      vecName import float -data obj -file file ?switches?
 *      vecName export float -data obj -file file 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ExportOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    ExportSwitches switches;
    size_t numValues;
    char *fmt;
    int format;
    Blt_DBuffer dbuffer;
    int result;

#define FMT_FLOAT       0
#define FMT_DOUBLE      1
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
        objPtr = Blt_DBuffer_Base64EncodeToObj(dbuffer);
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
 *      Returns the number of values in the vector.  This excludes
 *      empty values.
 *
 * Results:
 *      A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
static int
CountOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
 *      Returns the indices of values in the vector.  This excludes
 *      empty values.
 *
 * Results:
 *      A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
static int
IndicesOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
 *      Searches for a value in the vector. Returns the indices of all
 *      vector elements matching a particular value.
 *
 * Results:
 *      Always returns TCL_OK.  interp->result will contain a list of the
 *      indices of array elements matching value. If no elements match,
 *      interp->result will contain the empty string.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SearchOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
        return TCL_OK;          /* Bogus range. Don't bother looking. */
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
 *      Queries or sets the offset of the array index from the base address
 *      of the data array of values.
 *
 * Results:
 *      A standard TCL result.  If the source vector doesn't exist or the
 *      source list is not a valid list of numbers, TCL_ERROR returned.
 *      Otherwise TCL_OK is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
OffsetOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
 *      Generates random values for the length of the vector.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RandomOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
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
 *      Generates a sequence of values in the vector.
 *
 * Results:
 *      A standard TCL result.
 *
 *      $v sequence start stop ?step?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SequenceOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    const char *string;
    double start, stop, step;
    int i, numSteps;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &start) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]);
    step = 1.0;
    stop = 0.0;
    numSteps = 0;
    if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
        numSteps = vPtr->length;
    } else if (Tcl_GetDoubleFromObj(interp, objv[3], &stop) != TCL_OK) {
        return TCL_ERROR;
    }
    step = 1.0;                         /* By default, increment is 1.0 */
    if ((objc > 4) && 
        (Tcl_GetDoubleFromObj(interp, objv[4], &step) != TCL_OK)) {
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
 *      Generate linearly spaced values.
 *
 * Results:
 *      A standard TCL result.
 *
 *      $v linspace first last ?numSteps?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
LinspaceOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Vector *destPtr = clientData;
    int numSteps;
    double first, last;
    
    if (Tcl_GetDoubleFromObj(interp, objv[2], &first) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[3], &last) != TCL_OK) {
        return TCL_ERROR;
    }
    numSteps = destPtr->length;         /* By default, generate one step
                                         * for each entry in the vector. */
    if ((objc > 4) && 
        (Tcl_GetIntFromObj(interp, objv[4], &numSteps) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (numSteps > 1) {                 /* Silently ignore non-positive
                                         * numSteps */
        int i;
        double step;

        if (Blt_Vec_SetLength(interp, destPtr, numSteps) != TCL_OK) {
            return TCL_ERROR;
        }
        step = (last - first) / (double)(numSteps - 1);
        for (i = 0; i < numSteps; i++) { 
            destPtr->valueArr[i] = first + (step * i);
        }
        if (destPtr->flush) {
            Blt_Vec_FlushCache(destPtr);
        }
        Blt_Vec_UpdateClients(destPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetOp --
 *
 *      Sets the data of the vector object from a list of values.
 *
 * Results:
 *      A standard TCL result.  If the source vector doesn't exist or the
 *      source list is not a valid list of numbers, TCL_ERROR returned.
 *      Otherwise TCL_OK is returned.
 *
 * Side Effects:
 *      The vector data is reset.  Clients of the vector are notified.  Any
 *      cached array indices are flushed.
 *
 *
 *      vecName set $list 
 *      vecName set anotherVector
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SetOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    int result;
    Vector *srcPtr;
    int numElem;
    Tcl_Obj **elemObjArr;

    /* The source can be either a list of numbers or another vector.  */

    srcPtr = Blt_Vec_ParseElement((Tcl_Interp *)NULL, vPtr->dataPtr, 
           Tcl_GetString(objv[2]), NULL, NS_SEARCH_BOTH);
    if (srcPtr != NULL) {
        if (vPtr == srcPtr) {
            Vector *tmpPtr;
            /* 
             * Source and destination vectors are the same.  Copy the source
             * first into a temporary vector to avoid memory overlaps.
             */
            tmpPtr = Blt_Vec_New(vPtr->dataPtr);
            result = Blt_Vec_Duplicate(tmpPtr, srcPtr);
            if (result == TCL_OK) {
                result = Blt_Vec_Duplicate(vPtr, tmpPtr);
            }
            Blt_Vec_Free(tmpPtr);
        } else {
            result = Blt_Vec_Duplicate(vPtr, srcPtr);
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
 *      Siets the data of the vector object from a list of values.
 *
 * Results:
 *      A standard TCL result.  If the source vector doesn't exist or the
 *      source list is not a valid list of numbers, TCL_ERROR returned.
 *      Otherwise TCL_OK is returned.
 *
 * Side Effects:
 *      The vector data is reset.  Clients of the vector are notified.  Any
 *      cached array indices are flushed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SimplifyOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    Vector *x, *y;
    size_t i, n;
    int numPoints;
    int *indices;
    double tolerance = 10.0;
    Point2d *origPts;
    double *xArr, *yArr;
    
    if (GetVector(interp, vPtr->dataPtr, objv[2], &x) ||
        GetVector(interp, vPtr->dataPtr, objv[3], &y)) {
        return TCL_ERROR;
    }
    if (objc > 4) {
        if (Tcl_GetDoubleFromObj(interp, objv[4], &tolerance) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (x->length != y->length) {
        Tcl_AppendResult(interp, "x and y vectors are not the same length",
                         (char *)NULL);
        return TCL_ERROR;
    }
    numPoints = x->length;
    if (numPoints < 3) {
        Tcl_AppendResult(interp, "too few points in vectors",
                         (char *)NULL);
        return TCL_ERROR;
    }
    origPts = Blt_Malloc(sizeof(Point2d) * numPoints);
    if (origPts == NULL) {
        Tcl_AppendResult(interp, "can't allocate \"", Blt_Itoa(numPoints), 
                "\" points", (char *)NULL);
        return TCL_ERROR;
    }
    xArr = Blt_VecData(x);
    yArr = Blt_VecData(y);
    for (i = 0; i < numPoints; i++) {
        origPts[i].x = xArr[i];
        origPts[i].y = yArr[i];
    }
    indices = Blt_Malloc(sizeof(int) * numPoints);
    if (indices == NULL) {
        Tcl_AppendResult(interp, "can't allocate \"", Blt_Itoa(numPoints), 
                "\" indices for simplication array", (char *)NULL);
        Blt_Free(origPts);
        return TCL_ERROR;
    }
    n = Blt_SimplifyLine(origPts, 0, numPoints - 1, tolerance, indices);
    if (Blt_Vec_ChangeLength(interp, vPtr, n) != TCL_OK) {
        Blt_Free(origPts);
        return TCL_ERROR;
    }
    xArr = Blt_VecData(vPtr);
    for (i = 0; i < n; i++) {
        xArr[i] = (double)indices[i];
    }
    Blt_Free(indices);
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
 *      Copies the values from the vector evenly into one of more vectors.
 *
 * Results:
 *      A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SplitOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    int numVectors;

    numVectors = objc - 2;
    if ((vPtr->length % numVectors) != 0) {
        Tcl_AppendResult(interp, "can't split vector \"", vPtr->name, 
           "\" into ", Blt_Itoa(numVectors), " even parts.", (char *)NULL);
        return TCL_ERROR;
    }
    if (numVectors > 0) {
        int i, j, k;
        int oldSize, newSize, extra;

        extra = vPtr->length / numVectors;
        for (i = 0; i < numVectors; i++) {
            Vector *destPtr;

            if (GetVector(interp, vPtr->dataPtr, objv[i + 2], &destPtr) != TCL_OK) {
                return TCL_ERROR;
            }
            oldSize = destPtr->length;
            newSize = oldSize + extra;
            if (Blt_Vec_SetLength(interp, destPtr, newSize) != TCL_OK) {
                return TCL_ERROR;
            }
            for (j = i, k = oldSize; j < vPtr->length; j += numVectors, k++) {
                destPtr->valueArr[k] = vPtr->valueArr[j];
            }
            Blt_Vec_UpdateClients(destPtr);
            if (destPtr->flush) {
                Blt_Vec_FlushCache(destPtr);
            }
        }
    }
    return TCL_OK;
}


static Vector **sortVectors;            /* Pointer to the array of values
                                         * currently being sorted. */
static int numSortVectors;
static int sortDecreasing;              /* Indicates the ordering of the
                                         * sort. If non-zero, the vectors
                                         * are sorted in decreasing
                                         * order. */

static int
CompareValues(double a, double b)
{
    double d;
    
    if (!FINITE(a)) {
        if (!FINITE(b)) {
            return 0;               /* Both points are empty */
        }
        return 1;
    } else if (!FINITE(b)) {
        return -1;
    }
    d = a - b;
    if (d < 0.0) {
        return -1;
    } else if (d > 0.0) {
        return 1;
    }
    return 0;
}

static int
ComparePoints(const void *a, const void *b)
{
    int i;
    size_t i1 = *(int *)a;
    size_t i2 = *(int *)b;

    for (i = 0; i < numSortVectors; i++) {
        int cond;
        Vector *vPtr;
        
        vPtr = sortVectors[i];
        cond = CompareValues(vPtr->valueArr[i1], vPtr->valueArr[i2]);
        if (cond != 0) {
            return (sortDecreasing) ? -cond : cond;
        }
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_SortMap --
 *
 *      Returns an array of indices that represents the sorted mapping of
 *      the original vector.
 *
 * Results:
 *      A standard TCL result.  If any of the auxiliary vectors are a
 *      different size than the sorted vector object, TCL_ERROR is
 *      returned.  Otherwise TCL_OK is returned.
 *
 * Side Effects:
 *      The vectors are sorted.
 *
 *      vecName sort ?switches? vecName vecName...
 *---------------------------------------------------------------------------
 */
void
Blt_Vec_SortMap(Vector **vectors, int numVectors, size_t **mapPtr)
{
    size_t *map;
    int i;
    Vector *vPtr = vectors[0];

    map = Blt_AssertMalloc(sizeof(size_t) * vPtr->length);
    for (i = 0; i < vPtr->length; i++) {
        map[i] = i;
    }
    /* Set global variables for sorting routine. */
    sortVectors = vectors;
    numSortVectors = numVectors;
    qsort((char *)map, vPtr->length, sizeof(size_t), ComparePoints);
    *mapPtr = map;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_NonemptySortMap --
 *
 *      Returns an array of indices that represents the sorted mapping of
 *      the original vector. Only non-empty points are considered.
 *
 * Results:
 *      A standard TCL result.  If any of the auxiliary vectors are a
 *      different size than the sorted vector object, TCL_ERROR is
 *      returned.  Otherwise TCL_OK is returned.
 *
 * Side Effects:
 *      The vectors are sorted.
 *
 *      vecName sort ?switches? vecName vecName...
 *---------------------------------------------------------------------------
 */

int
Blt_Vec_NonemptySortMap(Vector *vPtr, size_t **mapPtr)
{
    size_t *map;
    int i, j, count;

    count = 0;
    for (i = 0; i < vPtr->length; i++) {
        if (FINITE(vPtr->valueArr[i])) {
            count++;
        }
    }
    map = Blt_AssertMalloc(sizeof(size_t) * count);
    for (i = 0, j = 0; i < vPtr->length; i++) {
        if (FINITE(vPtr->valueArr[i])) {
            map[j] = i;
            j++;
        }
    }
    /* Set global variables for sorting routine. */
    sortVectors = &vPtr;
    numSortVectors = 1;
    qsort((char *)map, count, sizeof(size_t), ComparePoints);
    *mapPtr = map;
    return count;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortOp --
 *
 *      Sorts the vector object and any other vectors according to sorting
 *      order of the vector object.
 *
 * Results:
 *      A standard TCL result.  If any of the auxiliary vectors are a
 *      different size than the sorted vector object, TCL_ERROR is returned.
 *      Otherwise TCL_OK is returned.
 *
 * Side Effects:
 *      The vectors are sorted.
 *
 *      vecName sort ?switches? vecName vecName...
 *---------------------------------------------------------------------------
 */
static int
SortOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    SortSwitches switches;
    Vector **vectors;
    double *copy;
    int i;
    size_t *map;
    size_t numBytes, sortLength, numVectors;

    /* Global flag to to pass to comparison routines.  */
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

    vectors = Blt_AssertMalloc(sizeof(Vector *) * (objc + 1));
    vectors[0] = vPtr;
    numVectors = 1;
    sortLength = vPtr->length;
    for (i = 2; i < objc; i++) {
        Vector *srcPtr;

        if (GetVector(interp, vPtr->dataPtr, objv[i], &srcPtr) != TCL_OK) {
            Blt_Free(vectors);
            return TCL_ERROR;
        }
        if (srcPtr->length != vPtr->length) {
            Tcl_AppendResult(interp, "vector \"", srcPtr->name,
                "\" is not the same size as \"", vPtr->name, "\"",
                (char *)NULL);
            Blt_Free(vectors);
            return TCL_ERROR;
        }
        vectors[numVectors] = srcPtr;
        numVectors++;
    }

    /* Sort the vector. We get a sorted map. */
    Blt_Vec_SortMap(vectors, numVectors, &map);
    if (map == NULL) {
        Blt_Free(vectors);
        return TCL_ERROR;
    }
    /* If all we care about is the unique values then compress the map. */
    if (switches.flags & SORT_UNIQUE) {
        size_t count, i;

        count = 1;
        for (i = 1; i < vPtr->length; i++) {
            size_t next, prev;

            next = map[i];
            prev = map[i - 1];
            if (ComparePoints(&next, &prev) != 0) {
                map[count] = next;
                count++;
            }
        }
        sortLength = count;
    }

    /* If we're returning the indices or values of the sorted points.
     * do that now. */
    if (switches.flags & (SORT_VALUES | SORT_INDICES)) {
        Tcl_Obj *listObjPtr;
        size_t i;
        
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

        if (switches.flags & SORT_INDICES) {
            for (i = 0; i < sortLength; i++) {
                Tcl_Obj *objPtr;

                objPtr = Tcl_NewLongObj(map[i]);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }            
        } else {
            for (i = 0; i < sortLength; i++) {
                int j;
                
                for (j = 0; j < numVectors; j++) {
                    Vector *vPtr;
                    Tcl_Obj *objPtr;
                    
                    vPtr = vectors[j];
                    objPtr = Tcl_NewDoubleObj(vPtr->valueArr[map[i]]);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                }
            }
        }
        Blt_Free(map);
        Blt_Free(vectors);
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

    /* Now rearrange the designated vectors according to the sort map.  The
     * vectors must be the same size as the map.  */
    for (i = 0; i < numVectors; i++) {
        size_t j;
        Vector *destPtr;
        
        destPtr = vectors[i];
        memcpy((char *)copy, (char *)destPtr->valueArr, numBytes);
        if (sortLength != (size_t)destPtr->length) {
            Blt_Vec_SetLength(interp, destPtr, sortLength);
        }
        for (j = 0; j < sortLength; j++) {
            destPtr->valueArr[j] = copy[map[j]];
        }
        Blt_Vec_UpdateClients(destPtr);
        if (destPtr->flush) {
            Blt_Vec_FlushCache(destPtr);
        }
    }
    Blt_Free(vectors);
    Blt_Free(copy);
    Blt_Free(map);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InstExprOp --
 *
 *      Computes the result of the expression which may be either a scalar
 *      (single value) or vector (list of values).
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InstExprOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;

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
 *      A standard TCL result.  If the source vector doesn't exist or the
 *      source list is not a valid list of numbers, TCL_ERROR returned.
 *      Otherwise TCL_OK is returned.
 *
 * Side Effects:
 *      The vector data is reset.  Clients of the vector are notified.
 *      Any cached array indices are flushed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ArithOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Vector *vPtr = clientData;
    double value;
    int i;
    Vector *srcPtr;
    double scalar;
    Tcl_Obj *listObjPtr;
    const char *string;

    srcPtr = Blt_Vec_ParseElement((Tcl_Interp *)NULL, vPtr->dataPtr, 
        Tcl_GetString(objv[2]), NULL, NS_SEARCH_BOTH);
    if (srcPtr != NULL) {
        int j;

        if (srcPtr->length != vPtr->length) {
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
                value = vPtr->valueArr[i] * srcPtr->valueArr[j];
                Tcl_ListObjAppendElement(interp, listObjPtr,
                         Tcl_NewDoubleObj(value));
            }
            break;

        case '/':
            for (i = 0, j = 0; i < vPtr->length; i++, j++) {
                value = vPtr->valueArr[i] / srcPtr->valueArr[j];
                Tcl_ListObjAppendElement(interp, listObjPtr,
                         Tcl_NewDoubleObj(value));
            }
            break;

        case '-':
            for (i = 0, j = 0; i < vPtr->length; i++, j++) {
                value = vPtr->valueArr[i] - srcPtr->valueArr[j];
                Tcl_ListObjAppendElement(interp, listObjPtr,
                         Tcl_NewDoubleObj(value));
            }
            break;

        case '+':
            for (i = 0, j = 0; i < vPtr->length; i++, j++) {
                value = vPtr->valueArr[i] + srcPtr->valueArr[j];
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
 *      Parses and invokes the appropriate vector instance command option.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec vectorInstOps[] =
{
    {"*",         1, ArithOp,     3, 3, "item",},       /*Deprecated*/
    {"+",         1, ArithOp,     3, 3, "item",},       /*Deprecated*/
    {"-",         1, ArithOp,     3, 3, "item",},       /*Deprecated*/
    {"/",         1, ArithOp,     3, 3, "item",},       /*Deprecated*/
    {"append",    1, AppendOp,    3, 0, "item ?item...?",},
    {"binread",   2, BinreadOp,   3, 0, "channel ?numValues? ?flags?",},
    {"clear",     2, ClearOp,     2, 2, "",},
    {"count",     2, CountOp,     3, 3, "what",},
    {"delete",    2, DeleteOp,    2, 0, "index ?index...?",},
    {"duplicate", 2, DupOp,       2, 3, "?vecName?",},
    {"export",    4, ExportOp,    3, 0, "format ?switches?",},
    {"expr",      4, InstExprOp,  3, 3, "expression",},
    {"fft",       2, FFTOp,       3, 0, "vecName ?switches?",},
    {"frequency", 2, FrequencyOp, 4, 4, "vecName numBins",},
    {"indices",   3, IndicesOp,   3, 3, "what",},
    {"inversefft",3, InverseFFTOp,4, 4, "vecName vecName",},
    {"length",    2, LengthOp,    2, 3, "?newSize?",},
    {"limits",    3, LimitsOp,    2, 2, "",},
    {"linspace",  3, LinspaceOp, 4, 5, "first last ?numSteps?",},
    {"maximum",   2, MaxOp,       2, 2, "",},
    {"merge",     2, MergeOp,     3, 0, "vecName ?vecName...?",},
    {"minimum",   2, MinOp,       2, 2, "",},
    {"normalize", 3, NormalizeOp, 2, 3, "?vecName?",},  /*Deprecated*/
    {"notify",    3, NotifyOp,    3, 3, "keyword",},
    {"offset",    1, OffsetOp,    2, 3, "?offset?",},
    {"pack",      2, PackOp,      2, 2, "",},
    {"populate",  2, PopulateOp,  4, 4, "vecName density",},
    {"print",     2, PrintOp,     3, 0, "format ?switches?",},
    {"random",    4, RandomOp,    2, 3, "?seed?",},     /*Deprecated*/
    {"range",     4, RangeOp,     2, 4, "first last",},
    {"search",    3, SearchOp,    3, 5, "?-value? value ?value?",},
    {"sequence",  3, SequenceOp,  4, 5, "start stop ?step?",},
    {"set",       3, SetOp,       3, 3, "item",},
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
    Tcl_ObjCmdProc *proc;
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
 *      Returns NULL on success.  Only called from a variable trace.
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
#define MAX_ERR_MSG     1023
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



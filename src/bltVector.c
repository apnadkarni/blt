/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltVector.c --
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

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef TIME_WITH_SYS_TIME
  #include <sys/time.h>
  #include <time.h>
#else
  #ifdef HAVE_SYS_TIME_H
    #include <sys/time.h>
  #else
    #include <time.h>
  #endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH_SYS_TIME */

#include "bltAlloc.h"
#include <bltMath.h>
#include "bltNsUtil.h"
#include "bltSwitch.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#ifndef TCL_NAMESPACE_ONLY
#define TCL_NAMESPACE_ONLY TCL_GLOBAL_ONLY
#endif

#define DEF_ARRAY_SIZE          64
#define TRACE_ALL  (TCL_TRACE_WRITES | TCL_TRACE_READS | TCL_TRACE_UNSETS)


#define VECTOR_CHAR(c)  ((isalnum(UCHAR(c))) || \
        (c == '_') || (c == ':') || (c == '@') || (c == '.'))


/*
 * VectorClient --
 *
 *      A vector can be shared by several clients.  Each client allocates
 *      this structure that acts as its key for using the vector.  Clients
 *      can also designate a callback routine that is executed whenever the
 *      vector is updated or destroyed.
 *
 */
typedef struct {
    unsigned int magic;                 /* Magic value designating whether
                                         * this really is a vector token or
                                         * not */
    Vector *serverPtr;                  /* Pointer to the master record of
                                         * the vector.  If NULL, indicates
                                         * that the vector has been
                                         * destroyed but as of yet, this
                                         * client hasn't recognized it. */
    Blt_VectorChangedProc *proc;        /* Routine to call when the
                                         * contents of the vector change or
                                         * the vector is deleted. */
    ClientData clientData;              /* Data passed whenever the vector
                                         * change procedure is called. */
    Blt_ChainLink link;                 /* Used to quickly remove this
                                         * entry from its server's client
                                         * chain. */
} VectorClient;

static Tcl_CmdDeleteProc VectorInstDeleteProc;
static Tcl_ObjCmdProc VectorCmd;
static Tcl_InterpDeleteProc VectorInterpDeleteProc;

typedef struct {
    const char *varName;                /* Requested variable name. */
    const char *cmdName;                /* Requested command name. */
    int flush;                          /* Flush */
    int watchUnset;                     /* Watch when variable is unset. */
    int size;
    int first, last;
} CreateSwitches;

static Blt_SwitchSpec createSwitches[] = 
{
    {BLT_SWITCH_STRING, "-variable", "varName", (char *)NULL,
        Blt_Offset(CreateSwitches, varName), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_STRING, "-command", "command", (char *)NULL,
        Blt_Offset(CreateSwitches, cmdName), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_BOOLEAN, "-watchunset", "bool", (char *)NULL,
        Blt_Offset(CreateSwitches, watchUnset), 0},
    {BLT_SWITCH_BOOLEAN, "-flush", "bool", (char *)NULL,
        Blt_Offset(CreateSwitches, flush), 0},
    {BLT_SWITCH_LONG_POS, "-length", "length", (char *)NULL,
        Blt_Offset(CreateSwitches, size), 0},
    {BLT_SWITCH_END}
};

typedef int (VectorCmdProc)(Vector *vecObjPtr, Tcl_Interp *interp, 
        int objc, Tcl_Obj *const *objv);

static Vector *
FindVectorInNamespace(
    VectorCmdInterpData *dataPtr,       /* Interpreter-specific data. */
    Blt_ObjectName *objNamePtr)
{
    Tcl_DString ds;
    const char *name;
    Blt_HashEntry *hPtr;

    name = Blt_MakeQualifiedName(objNamePtr, &ds);
    hPtr = Blt_FindHashEntry(&dataPtr->vectorTable, name);
    Tcl_DStringFree(&ds);
    if (hPtr != NULL) {
        return Blt_GetHashValue(hPtr);
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetVectorObject --
 *
 *      Searches for the vector associated with the name given.  Allow for
 *      a range specification.
 *
 * Results:
 *      Returns a pointer to the vector if found, otherwise NULL.
 *
 *---------------------------------------------------------------------------
 */
static Vector *
GetVectorObject(
    VectorCmdInterpData *dataPtr,       /* Interpreter-specific data. */
    const char *name,
    int flags)
{
    Blt_ObjectName objName;
    Vector *vPtr;
    Tcl_Interp *interp;

    interp = dataPtr->interp;
    if (!Blt_ParseObjectName(interp, name, &objName, 
                BLT_NO_ERROR_MSG | BLT_NO_DEFAULT_NS)) {
        return NULL;                    /* Can't find namespace. */
    } 
    vPtr = NULL;
    if (objName.nsPtr != NULL) {
        vPtr = FindVectorInNamespace(dataPtr, &objName);
    } else {
        if (flags & NS_SEARCH_CURRENT) {
            objName.nsPtr = Tcl_GetCurrentNamespace(interp);
            vPtr = FindVectorInNamespace(dataPtr, &objName);
        }
        if ((vPtr == NULL) && (flags & NS_SEARCH_GLOBAL)) {
            objName.nsPtr = Tcl_GetGlobalNamespace(interp);
            vPtr = FindVectorInNamespace(dataPtr, &objName);
        }
    }
    return vPtr;
}

void
Blt_Vec_UpdateRange(Vector *vPtr)
{
    double min, max;
    double *vp, *vend;

    vp = vPtr->valueArr + vPtr->first;
    vend = vPtr->valueArr + vPtr->last;
    min = max = *vp++;
    for (/* empty */; vp < vend; vp++) {
        if (min > *vp) {
            min = *vp; 
        } else if (max < *vp) { 
            max = *vp; 
        } 
    } 
    vPtr->min = min;
    vPtr->max = max;
    vPtr->notifyFlags &= ~UPDATE_RANGE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_GetSpecialIndex --
 *
 *      Converts the string representing an index in the vector, to its
 *      numeric value.  A valid index may be an numeric string of the
 *      string "end" (indicating the last element in the string).
 *
 * Results:
 *      A standard TCL result.  If the string is a valid index, TCL_OK is
 *      returned.  Otherwise TCL_ERROR is returned and interp->result will
 *      contain an error message.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_GetSpecialIndex(Tcl_Interp *interp, Vector *vPtr, const char *string,
                        Blt_VectorIndexProc **procPtrPtr)
{
    Blt_HashEntry *hPtr;
    
    hPtr = Blt_FindHashEntry(&vPtr->dataPtr->indexProcTable, string);
    if (hPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "bad index \"", string, "\"", 
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    *procPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_GetIndex --
 *
 *      Converts the string representing an index in the vector, to its
 *      numeric value.  A valid index may be an numeric string of the
 *      string "end" (indicating the last element in the string).
 *
 * Results:
 *      A standard TCL result.  If the string is a valid index, TCL_OK is
 *      returned.  Otherwise TCL_ERROR is returned and interp->result will
 *      contain an error message.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_GetIndex(Tcl_Interp *interp, Vector *vPtr, const char *string,
                 int *indexPtr)
{
    char c;
    int index;
    
    c = string[0];

    /* Treat the index "end" like a numeric index.  */

    if ((c == 'e') && (strcmp(string, "end") == 0)) {
        if (vPtr->length == 0) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "bad index \"end\": vector is empty", 
                                 (char *)NULL);
            }
            return TCL_ERROR;
        }
        *indexPtr = vPtr->length - 1;
        return TCL_OK;
    }
    /* Try to process the index as a number. May be negative. */
    if (Tcl_GetInt(interp, string, &index) != TCL_OK) {
        long lval;
        /*   
         * Try to process the index as a numeric expression. 
         *
         * Unlike Tcl_GetInt, Tcl_ExprLong needs a valid interpreter, but
         * the interp passed in may be NULL.  So we have to use
         * vPtr->interp and then reset the result.
         */
        if (Tcl_ExprLong(vPtr->interp, (char *)string, &lval) != TCL_OK) {
            Tcl_ResetResult(vPtr->interp);
            if (interp != NULL) {
                Tcl_AppendResult(interp, "bad index \"", string, "\"", 
                                 (char *)NULL);
            }
            return TCL_ERROR;
        }
        index = lval;
    }
    /*
     * Correct the index by the current value of the offset. This makes all
     * the numeric indices non-negative.
     */
    if ((vPtr->offset > index) || ((index - vPtr->offset) >= vPtr->length))  {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "index \"", string, "\" is out of range", 
                         (char *)NULL);
        }
        return TCL_ERROR;
    }
    *indexPtr = index - vPtr->offset;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_GetRange --
 *
 *      Converts the string representing an index in the vector, to its
 *      numeric value.  A valid index may be an numeric string of the
 *      string "end" (indicating the last element in the string).
 *
 *      "all"
 *      "end"
 *      n:m, n:"end"
 *
 * Results:
 *      A standard TCL result.  If the string is a valid index, TCL_OK is
 *      returned.  Otherwise TCL_ERROR is returned and interp->result will
 *      contain an error message.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_GetRange(Tcl_Interp *interp, Vector *vPtr, const char *string)
{
    char *colon;
    char c;

    c = string[0];
    colon = strchr(string, ':');
    if (colon != NULL) {
        if (string == colon) {
            vPtr->first = 0;            /* Colon is first character.  First
                                         * index is defaults to 0. */
        } else {
            int result;
            int index;

            *colon = '\0';
            result = Blt_Vec_GetIndex(interp, vPtr, string, &index);
            *colon = ':';
            if (result != TCL_OK) {
                return TCL_ERROR;
            }
            vPtr->first = index;
        }
        if (*(colon + 1) == '\0') {
            /* Default to the last index */
            vPtr->last = vPtr->length;
        } else {
            int index;

            if (Blt_Vec_GetIndex(interp, vPtr, colon + 1, &index) != TCL_OK) {
                return TCL_ERROR;
            }
            vPtr->last = index + 1;
        }
        if (vPtr->first >= vPtr->last) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "bad range \"", string,
                         "\" (first >= last)", (char *)NULL);
            }
            return TCL_ERROR;
        }
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
        vPtr->first = 0;
        vPtr->last = vPtr->length;
    } else {
        int index;

        if (Blt_Vec_GetIndex(interp, vPtr, string, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        vPtr->first = index;
        vPtr->last = index + 1;
    }
    return TCL_OK;
}

Vector *
Blt_Vec_ParseElement(
    Tcl_Interp *interp,
    VectorCmdInterpData *dataPtr,       /* Interpreter-specific data. */
    const char *start,
    const char **endPtr,
    int flags)
{
    char *p;
    char saved;
    Vector *vPtr;

    p = (char *)start;
    /* Find the end of the vector name */
    while (VECTOR_CHAR(*p)) {
        p++;
    }
    saved = *p;
    *p = '\0';

    vPtr = GetVectorObject(dataPtr, start, flags);
    if (vPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find a vector named \"", 
                start, "\"", (char *)NULL);
        }
        *p = saved;
        return NULL;
    }
    *p = saved;
    vPtr->first = 0;
    vPtr->last = vPtr->length;
    if (*p == '(') {
        int count, result;

        start = p + 1;
        p++;

        /* Find the matching right parenthesis */
        count = 1;
        while (*p != '\0') {
            if (*p == ')') {
                count--;
                if (count == 0) {
                    break;
                }
            } else if (*p == '(') {
                count++;
            }
            p++;
        }
        if (count > 0) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "unbalanced parentheses \"", start, 
                        "\"", (char *)NULL);
            }
            return NULL;
        }
        *p = '\0';
        result = Blt_Vec_GetRange(interp, vPtr, start);
        *p = ')';
        if (result != TCL_OK) {
            return NULL;
        }
        p++;
    }
    if (endPtr != NULL) {
      *endPtr = p;
    }
    return vPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_NotifyClients --
 *
 *      Notifies each client of the vector that the vector has changed
 *      (updated or destroyed) by calling the provided function back.  The
 *      function pointer may be NULL, in that case the client is not
 *      notified.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The results depend upon what actions the client callbacks
 *      take.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Vec_NotifyClients(ClientData clientData)
{
    Vector *vPtr = clientData;
    Blt_ChainLink link, next;
    Blt_VectorNotify notify;

    notify = (vPtr->notifyFlags & NOTIFY_DESTROYED)
        ? BLT_VECTOR_NOTIFY_DESTROY : BLT_VECTOR_NOTIFY_UPDATE;
    vPtr->notifyFlags &= ~(NOTIFY_UPDATED | NOTIFY_DESTROYED | NOTIFY_PENDING);
    for (link = Blt_Chain_FirstLink(vPtr->chain); link != NULL; link = next) {
        VectorClient *clientPtr;

        next = Blt_Chain_NextLink(link);
        clientPtr = Blt_Chain_GetValue(link);
        if ((clientPtr->proc != NULL) && (clientPtr->serverPtr != NULL)) {
            (*clientPtr->proc) (vPtr->interp, clientPtr->clientData, notify);
        }
    }
    /*
     * Some clients may not handle the "destroy" callback properly (they
     * should call Blt_FreeVectorId to release the client identifier), so
     * mark any remaining clients to indicate that vector's server has gone
     * away.
     */
    if (notify == BLT_VECTOR_NOTIFY_DESTROY) {
        for (link = Blt_Chain_FirstLink(vPtr->chain); link != NULL;
            link = Blt_Chain_NextLink(link)) {
            VectorClient *clientPtr;

            clientPtr = Blt_Chain_GetValue(link);
            clientPtr->serverPtr = NULL;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_UpdateClients --
 *
 *      Notifies each client of the vector that the vector has changed
 *      (updated or destroyed) by calling the provided function back.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The individual client callbacks are eventually invoked.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Vec_UpdateClients(Vector *vPtr)
{
    vPtr->dirty++;
    vPtr->max = vPtr->min = Blt_NaN();
    if (vPtr->notifyFlags & NOTIFY_NEVER) {
        return;
    }
    vPtr->notifyFlags |= NOTIFY_UPDATED;
    if (vPtr->notifyFlags & NOTIFY_ALWAYS) {
        Blt_Vec_NotifyClients(vPtr);
        return;
    }
    if (!(vPtr->notifyFlags & NOTIFY_PENDING)) {
        vPtr->notifyFlags |= NOTIFY_PENDING;
        Tcl_DoWhenIdle(Blt_Vec_NotifyClients, vPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_FlushCache --
 *
 *      Unsets all the elements of the TCL array variable associated with
 *      the vector, freeing memory associated with the variable.  This
 *      includes both the hash table and the hash keys.  The down side is
 *      that this effectively flushes the caching of vector elements in the
 *      array.  This means that the subsequent reads of the array will
 *      require a decimal to string conversion.
 *
 *      This is needed when the vector changes its values, making the array
 *      variable out-of-sync.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      All elements of array variable (except one) are unset, freeing the
 *      memory associated with the variable.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Vec_FlushCache(Vector *vPtr)
{
    Tcl_Interp *interp = vPtr->interp;

    if (vPtr->arrayName == NULL) {
        return;                 /* Doesn't use the variable API */
    }
    /* Turn off the trace temporarily so that we can unset all the elements
     * in the array.  */

    Tcl_UntraceVar2(interp, vPtr->arrayName, (char *)NULL,
        TRACE_ALL | vPtr->varFlags, Blt_Vec_VarTrace, vPtr);

    /* Clear all the element entries from the entire array */
    Tcl_UnsetVar2(interp, vPtr->arrayName, (char *)NULL, vPtr->varFlags);

    /* Restore the "end" index by default and the trace on the entire
     * array */
    Tcl_SetVar2(interp, vPtr->arrayName, "end", "", vPtr->varFlags);
    Tcl_TraceVar2(interp, vPtr->arrayName, (char *)NULL,
        TRACE_ALL | vPtr->varFlags, Blt_Vec_VarTrace, vPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_Find --
 *
 *      Searches for the vector associated with the name given.  Allow for
 *      a range specification.
 *
 * Results:
 *      Returns a pointer to the vector if found, otherwise NULL.  If the
 *      name is not associated with a vector and the TCL_LEAVE_ERR_MSG flag
 *      is set, and interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_Find(VectorCmdInterpData *dataPtr, const char *vecName,
             Vector **vPtrPtr)
{
    Vector *vPtr;
    const char *endPtr;

    vPtr = Blt_Vec_ParseElement(dataPtr->interp, dataPtr, vecName, &endPtr, 
        NS_SEARCH_BOTH);
    if (vPtr == NULL) {
        return TCL_ERROR;
    }
    if (*endPtr != '\0') {
        Tcl_AppendResult(dataPtr->interp, 
                         "extra characters after vector name", (char *)NULL);
        return TCL_ERROR;
    }
    *vPtrPtr = vPtr;
    return TCL_OK;
}

double
Blt_Vec_Min(Vector *vPtr)
{
    long i;
    double min;

    for (i = vPtr->first; i < vPtr->last; i++) {
        if (FINITE(vPtr->valueArr[i])) {
            break;
        }
    }
    if (i == vPtr->last) {
        return Blt_NaN();
    }
    min = vPtr->valueArr[i];
    for (/* empty */; i < vPtr->last; i++) {
        if (!FINITE(vPtr->valueArr[i])) {
            continue;
        }
        if (min > vPtr->valueArr[i]) {
            min = vPtr->valueArr[i]; 
        } 
    } 
    vPtr->min = min;
    return vPtr->min;
}

double
Blt_Vec_Max(Vector *vPtr)
{
    long i;
    double max;

    for (i = vPtr->first; i < vPtr->last; i++) {
        if (FINITE(vPtr->valueArr[i])) {
            break;
        }
    }
    if (i == vPtr->last) {
        return Blt_NaN();
    }
    max = vPtr->valueArr[i];
    for (/* empty */; i < vPtr->last; i++) {
        if (!FINITE(vPtr->valueArr[i])) {
            continue;
        }
        if (max < vPtr->valueArr[i]) {
            max = vPtr->valueArr[i]; 
        } 
    } 
    vPtr->max = max;
    return vPtr->max;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteCommand --
 *
 *      Deletes the TCL command associated with the vector, without
 *      triggering a callback to "VectorInstDeleteProc".
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
DeleteCommand(Vector *vPtr)   
{
    Blt_ObjectName objName;
    Tcl_CmdInfo cmdInfo;
    Tcl_DString ds;
    Tcl_Interp *interp = vPtr->interp;
    const char *qualName;               /* Name of TCL command. */

    Tcl_DStringInit(&ds);
    objName.name = Tcl_GetCommandName(interp, vPtr->cmdToken);
    objName.nsPtr = Blt_GetCommandNamespace(vPtr->cmdToken);
    qualName = Blt_MakeQualifiedName(&objName, &ds);
    if (Tcl_GetCommandInfo(interp, qualName, &cmdInfo)) {
        /* Disable the callback before deleting the TCL command.*/      
        cmdInfo.deleteProc = NULL;      
        Tcl_SetCommandInfo(interp, qualName, &cmdInfo);
        Tcl_DeleteCommandFromToken(interp, vPtr->cmdToken);
    }
    Tcl_DStringFree(&ds);
    vPtr->cmdToken = 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * UnmapVariable --
 *
 *      Destroys the trace on the current TCL variable designated to access
 *      the vector.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
UnmapVariable(Vector *vPtr)
{
    Tcl_Interp *interp = vPtr->interp;

    /* Unset the entire array */
    Tcl_UntraceVar2(interp, vPtr->arrayName, (char *)NULL,
        (TRACE_ALL | vPtr->varFlags), Blt_Vec_VarTrace, vPtr);
    Tcl_UnsetVar2(interp, vPtr->arrayName, (char *)NULL, vPtr->varFlags);

    if (vPtr->arrayName != NULL) {
        Blt_Free(vPtr->arrayName);
        vPtr->arrayName = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_MapVariable --
 *
 *      Sets up traces on a TCL variable to access the vector.
 *
 *      If another variable is already mapped, it's first untraced and
 *      removed.  Don't do anything else for variables named "" (even
 *      though TCL allows this pathology). Saves the name of the new array
 *      variable.
 *
 * Results:
 *      A standard TCL result. If an error occurs setting the variable
 *      TCL_ERROR is returned and an error message is left in the
 *      interpreter.
 *
 * Side effects:
 *      Traces are set for the new variable. The new variable name is saved
 *      in a malloc'ed string in vPtr->arrayName.  If this variable is
 *      non-NULL, it indicates that a TCL variable has been mapped to this
 *      vector.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_MapVariable(
    Tcl_Interp *interp, 
    Vector *vPtr, 
    const char *path)
{
    Blt_ObjectName objName;
    const char *newPath;
    const char *result;
    Tcl_DString ds;

    if (vPtr->arrayName != NULL) {
        UnmapVariable(vPtr);
    }
    if ((path == NULL) || (path[0] == '\0')) {
        return TCL_OK;                  /* If the variable pathname is the
                                         * empty string, simply return
                                         * after removing any existing
                                         * variable. */
    }
    /* Get the variable name (without the namespace qualifier). */
    if (!Blt_ParseObjectName(interp, path, &objName, BLT_NO_DEFAULT_NS)) {
        return TCL_ERROR;
    }
    if (objName.nsPtr == NULL) {
        /* 
         * If there was no namespace qualifier, try harder to see if the
         * variable is non-local.
         */
        objName.nsPtr = Blt_GetVariableNamespace(interp, objName.name);
    } 
    Tcl_DStringInit(&ds);
    vPtr->varFlags = 0;
    if (objName.nsPtr != NULL) {        /* Global or namespace variable. */
        newPath = Blt_MakeQualifiedName(&objName, &ds);
        vPtr->varFlags |= (TCL_NAMESPACE_ONLY | TCL_GLOBAL_ONLY);
    } else {                            /* Local variable. */
        newPath = (char *)objName.name;
    }

    /*
     * To play it safe, delete the variable first.  This has the benefical
     * side-effect of unmapping the variable from another vector that may
     * be currently associated with it.
     */
    Tcl_UnsetVar2(interp, newPath, (char *)NULL, 0);

    /* 
     * Set the index "end" in the array.  This will create the variable
     * immediately so that we can check its namespace context.
     */
    result = Tcl_SetVar2(interp, newPath, "end", "", TCL_LEAVE_ERR_MSG);
    if (result == NULL) {
        Tcl_DStringFree(&ds);
        return TCL_ERROR;
    }
    /* Create a full-array trace on reads, writes, and unsets. */
    Tcl_TraceVar2(interp, newPath, (char *)NULL, TRACE_ALL, Blt_Vec_VarTrace,
        vPtr);
    vPtr->arrayName = Blt_AssertStrdup(newPath);
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_SetSize --
 *
 *      Resizes the vector to the designated new size.
 *
 *      If the new size is the same as the old, simply return.  Otherwise
 *      we're copying the data from one memory location to another.
 *
 *      If the storage changed memory locations, free up the old location
 *      if it was dynamically allocated.
 *
 * Results:
 *      A standard TCL result.  If the reallocation is successful, TCL_OK
 *      is returned, otherwise TCL_ERROR.
 *
 * Side effects:
 *      Memory for the array is reallocated.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_SetSize(Tcl_Interp *interp, Vector *vPtr, int newSize)
{
    if (newSize == 0) {
        newSize = DEF_ARRAY_SIZE;
    }
    if (newSize == vPtr->size) {
        /* Same size, use the current array. */
        return TCL_OK;
    } 
    if (vPtr->freeProc == TCL_DYNAMIC) {
        double *newArr;

        /* Old memory was dynamically allocated, so use realloc. */
        newArr = Blt_Realloc(vPtr->valueArr, newSize * sizeof(double));
        if (newArr == NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "can't reallocate ", 
                                 Blt_Ltoa(newSize), " elements for vector \"", 
                                 vPtr->name, "\"", (char *)NULL); 
            }
            return TCL_ERROR;
        }
        vPtr->size = newSize;
        vPtr->valueArr = newArr;
        return TCL_OK;
    }

    {
        double *newArr;

        /* Old memory was created specially (static or special allocator).
         * Replace with dynamically allocated memory (malloc-ed). */

        newArr = Blt_Calloc(newSize, sizeof(double));
        if (newArr == NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "can't allocate ", 
                                 Blt_Ltoa(newSize), " elements for vector \"", 
                                 vPtr->name, "\"", (char *)NULL); 
            }
            return TCL_ERROR;
        }
        {
            int used, wanted;
            
            /* Copy the contents of the old memory into the new. */
            used = vPtr->length;
            wanted = newSize;
            
            if (used > wanted) {
                used = wanted;
            }
            /* Copy any previous data */
            if (used > 0) {
                memcpy(newArr, vPtr->valueArr, used * sizeof(double));
            }
        }
        
        assert(vPtr->valueArr != NULL);
        
        /* 
         * We're not using the old storage anymore, so free it if it's not
         * TCL_STATIC.  It's static because the user previously reset the
         * vector with a statically allocated array (setting freeProc to
         * TCL_STATIC).
         */
        if (vPtr->freeProc != TCL_STATIC) {
            if (vPtr->freeProc == TCL_DYNAMIC) {
                Blt_Free(vPtr->valueArr);
            } else {
                (*vPtr->freeProc) ((char *)vPtr->valueArr);
            }
        }
        vPtr->freeProc = TCL_DYNAMIC;   /* Set the type of the new
                                         * storage */
        vPtr->valueArr = newArr;
        vPtr->size = newSize;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_SetSize --
 *
 *      Set the length (the number of elements currently in use) of the
 *      vector.  If the new length is greater than the size (total number
 *      of slots), then the vector is grown.
 *
 * Results:
 *      A standard TCL result.  If the reallocation is successful, TCL_OK
 *      is returned, otherwise TCL_ERROR.
 *
 * Side effects:
 *      Memory for the array is possibly reallocated.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_SetLength(Tcl_Interp *interp, Vector *vPtr, int newLength)
{
    if (vPtr->size < newLength) {
        if (Blt_Vec_SetSize(interp, vPtr, newLength) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (newLength > vPtr->length) {
        double emptyValue;
        long i;

        emptyValue = Blt_NaN();
        for (i = vPtr->length; i < newLength; i++) {
            vPtr->valueArr[i] = emptyValue;
        }
    }
    vPtr->length = newLength;
    vPtr->first = 0;
    vPtr->last = newLength;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_ChangeLength --
 *
 *      Resizes the vector to the new size.
 *
 *      The new size of the vector is computed by doubling the size of the
 *      vector until it fits the number of slots needed (designated by
 *      *length*).
 *
 *      If the new size is the same as the old, simply adjust the length of
 *      the vector.  Otherwise we're copying the data from one memory
 *      location to another. The trailing elements of the vector need to be
 *      reset to zero.
 *
 *      If the storage changed memory locations, free up the old location
 *      if it was dynamically allocated.
 *
 * Results:
 *      A standard TCL result.  If the reallocation is successful, TCL_OK
 *      is returned, otherwise TCL_ERROR.
 *
 * Side effects:
 *      Memory for the array is reallocated.
 *
 *---------------------------------------------------------------------------
 */

int
Blt_Vec_ChangeLength(Tcl_Interp *interp, Vector *vPtr, int newLength)
{
    double emptyValue;
    long i;

    if (newLength > vPtr->size) {
        int newSize;                    /* Size of array in elements */
    
        /* Compute the new size of the array.  It's a multiple of
         * DEF_ARRAY_SIZE. */
        newSize = DEF_ARRAY_SIZE;
        while (newSize < newLength) {
            newSize += newSize;
        }
        if (newSize != vPtr->size) {
            if (Blt_Vec_SetSize(interp, vPtr, newSize) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    emptyValue = Blt_NaN();
    for (i = vPtr->length; i < newLength; i++) {
        vPtr->valueArr[i] = emptyValue;
    }
    vPtr->length = newLength;
    vPtr->first = 0;
    vPtr->last = newLength;
    return TCL_OK;
    
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_Reset --
 *
 *      Resets the vector data.  This is called by a client to indicate
 *      that the vector data has changed.  The vector does not need to
 *      point to different memory.  Any clients of the vector will be
 *      notified of the change.
 *
 * Results:
 *      A standard TCL result.  If the new array size is invalid, TCL_ERROR
 *      is returned.  Otherwise TCL_OK is returned and the new vector data
 *      is recorded.
 *
 * Side Effects:
 *      Any client designated callbacks will be posted.  Memory may be
 *      changed for the vector array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Vec_Reset(
    Vector *vPtr,
    double *valueArr,                   /* Array containing the elements of
                                         * the vector. If NULL, indicates
                                         * to reset the vector size to the
                                         * default. */
    int length,                         /* # of elements that the vector
                                         * currently holds. */
    int size,                           /* The # of elements that the
                                         * array can hold. */
    Tcl_FreeProc *freeProc)             /* Address of memory deallocation
                                         * routine for the array of values.
                                         * Can also be TCL_STATIC,
                                         * TCL_DYNAMIC, or TCL_VOLATILE. */
{
    if (vPtr->valueArr != valueArr) {   /* New array of values resides in
                                         * different memory than the
                                         * current vector.  */
        if ((valueArr == NULL) || (size == 0)) {
            /* Empty array. Set up default values */
            valueArr = Blt_Malloc(sizeof(double) * DEF_ARRAY_SIZE);
            size = DEF_ARRAY_SIZE;
            if (valueArr == NULL) {
                Tcl_AppendResult(vPtr->interp, "can't allocate ", 
                        Blt_Ltoa(size), " elements for vector \"", 
                        vPtr->name, "\"", (char *)NULL);
                return TCL_ERROR;
            }
            freeProc = TCL_DYNAMIC;
            length = 0;
        } else if (freeProc == TCL_VOLATILE) {
            double *newArr;

            /* Data is volatile. Make a copy of the value array.  */
            newArr = Blt_Malloc(size * sizeof(double));
            if (newArr == NULL) {
                Tcl_AppendResult(vPtr->interp, "can't allocate ", 
                        Blt_Ltoa(size), " elements for vector \"", 
                        vPtr->name, "\"", (char *)NULL);
                return TCL_ERROR;
            }
            memcpy((char *)newArr, (char *)valueArr, 
                   sizeof(double) * length);
            valueArr = newArr;
            freeProc = TCL_DYNAMIC;
        } 

        if (vPtr->freeProc != TCL_STATIC) {
            /* Old data was dynamically allocated. Free it before attaching
             * new data.  */
            if (vPtr->freeProc == TCL_DYNAMIC) {
                Blt_Free(vPtr->valueArr);
            } else {
                (*freeProc) ((char *)vPtr->valueArr);
            }
        }
        vPtr->freeProc = freeProc;
        vPtr->valueArr = valueArr;
    }
    vPtr->size = size;
    vPtr->length = length;
    if (vPtr->flush) {
        Blt_Vec_FlushCache(vPtr);
    }
    Blt_Vec_UpdateClients(vPtr);
    return TCL_OK;
}

Vector *
Blt_Vec_New(VectorCmdInterpData *dataPtr)
{
    Vector *vPtr;

    vPtr = Blt_AssertCalloc(1, sizeof(Vector));
    vPtr->valueArr = Blt_Malloc(sizeof(double) * DEF_ARRAY_SIZE);
    if (vPtr->valueArr == NULL) {
        Blt_Free(vPtr);
        return NULL;
    }
    vPtr->size = DEF_ARRAY_SIZE;
    vPtr->freeProc = TCL_DYNAMIC;
    vPtr->length = 0;
    vPtr->interp = dataPtr->interp;
    vPtr->hashPtr = NULL;
    vPtr->chain = Blt_Chain_Create();
    vPtr->flush = FALSE;
    vPtr->min = vPtr->max = Blt_NaN();
    vPtr->notifyFlags = NOTIFY_WHENIDLE;
    vPtr->dataPtr = dataPtr;
    return vPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_Free --
 *
 *      Removes the memory and frees resources associated with the vector.
 *
 *      o Removes the trace and the TCL array variable and unsets
 *        the variable.
 *      o Notifies clients of the vector that the vector is being
 *        destroyed.
 *      o Removes any clients that are left after notification.
 *      o Frees the memory (if necessary) allocated for the array.
 *      o Removes the entry from the hash table of vectors.
 *      o Frees the memory allocated for the name.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Vec_Free(Vector *vPtr)
{
    Blt_ChainLink link;

    if (vPtr->cmdToken != 0) {
        DeleteCommand(vPtr);
    }
    if (vPtr->arrayName != NULL) {
        UnmapVariable(vPtr);
    }
    vPtr->length = 0;

    /* Immediately notify clients that vector is going away */
    if (vPtr->notifyFlags & NOTIFY_PENDING) {
        vPtr->notifyFlags &= ~NOTIFY_PENDING;
        Tcl_CancelIdleCall(Blt_Vec_NotifyClients, vPtr);
    }
    vPtr->notifyFlags |= NOTIFY_DESTROYED;
    Blt_Vec_NotifyClients(vPtr);

    for (link = Blt_Chain_FirstLink(vPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        VectorClient *clientPtr;

        clientPtr = Blt_Chain_GetValue(link);
        Blt_Free(clientPtr);
    }
    Blt_Chain_Destroy(vPtr->chain);
    if ((vPtr->valueArr != NULL) && (vPtr->freeProc != TCL_STATIC)) {
        if (vPtr->freeProc == TCL_DYNAMIC) {
            Blt_Free(vPtr->valueArr);
        } else {
            (*vPtr->freeProc) ((char *)vPtr->valueArr);
        }
    }
    if (vPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&vPtr->dataPtr->vectorTable, vPtr->hashPtr);
    }
    Blt_Free(vPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorInstDeleteProc --
 *
 *      Deletes the command associated with the vector.  This is called
 *      only when the command associated with the vector is destroyed.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
VectorInstDeleteProc(ClientData clientData)
{
    Vector *vPtr = clientData;

    vPtr->cmdToken = 0;
    Blt_Vec_Free(vPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Vec_Create --
 *
 *      Creates a vector structure and the following items:
 *
 *      o TCL command
 *      o TCL array variable and establishes traces on the variable
 *      o Adds a  new entry in the vector hash table
 *
 * Results:
 *      A pointer to the new vector structure.  If an error occurred NULL
 *      is returned and an error message is left in interp->result.
 *
 * Side effects:
 *      A new TCL command and array variable is added to the interpreter.
 *
 * ---------------------------------------------------------------------- 
 */
Vector *
Blt_Vec_Create(
    VectorCmdInterpData *dataPtr,       /* Interpreter-specific data. */
    const char *vecName,                /* Namespace-qualified name of the
                                         * vector */
    const char *cmdName,                /* Name of the TCL command mapped
                                         * to the vector */
    const char *varName,                /* Name of the TCL array mapped to
                                         * the vector */
    int *isNewPtr)
{
    Tcl_DString ds;
    Vector *vPtr;
    int isNew;
    Blt_ObjectName objName;
    const char *qualName;
    Blt_HashEntry *hPtr;
    Tcl_Interp *interp = dataPtr->interp;

    isNew = 0;
    vPtr = NULL;

    if (!Blt_ParseObjectName(interp, vecName, &objName, 0)) {
        return NULL;
    }
    Tcl_DStringInit(&ds);
    if ((objName.name[0] == '#') && (strcmp(objName.name, "#auto") == 0)) {
        do {                            /* Generate a unique vector name. */
            char string[200];

            Blt_FmtString(string, 200, "vector%d", dataPtr->nextId++);
            objName.name = string;
            qualName = Blt_MakeQualifiedName(&objName, &ds);
            hPtr = Blt_FindHashEntry(&dataPtr->vectorTable, qualName);
        } while (hPtr != NULL);
    } else {
        const char *p;

        for (p = objName.name; *p != '\0'; p++) {
            if (!VECTOR_CHAR(*p)) {
                Tcl_AppendResult(interp, "bad vector name \"", objName.name,
                "\": must contain only digits, letters, underscore, or period",
                    (char *)NULL);
                goto error;
            }
        }
        qualName = Blt_MakeQualifiedName(&objName, &ds);
        vPtr = Blt_Vec_ParseElement((Tcl_Interp *)NULL, dataPtr, qualName, 
                NULL, NS_SEARCH_CURRENT);
    }
    if (vPtr == NULL) {
        hPtr = Blt_CreateHashEntry(&dataPtr->vectorTable, qualName, &isNew);
        if (!isNew) {
            Tcl_AppendResult(interp, "a vector \"", qualName,
                        "\" already exists", (char *)NULL);
            goto error;
        }
        vPtr = Blt_Vec_New(dataPtr);
        vPtr->hashPtr = hPtr;
        vPtr->nsPtr = objName.nsPtr;

        vPtr->name = Blt_GetHashKey(&dataPtr->vectorTable, hPtr);
        Blt_SetHashValue(hPtr, vPtr);
    } 
    if (cmdName != NULL) {
        Tcl_CmdInfo cmdInfo;

        if ((cmdName == vecName) ||
            ((cmdName[0] == '#') && (strcmp(cmdName, "#auto")==0))) {
            cmdName = qualName;
        } 
        if (Tcl_GetCommandInfo(interp, (char *)cmdName, &cmdInfo)) {
            if (vPtr != cmdInfo.objClientData) {
                Tcl_AppendResult(interp, "a command \"", cmdName,
                         "\" already exists", (char *)NULL);
                goto error;
            }
            /* We get here only if the old name is the same as the new. */
            goto checkVariable;
        }
    }
    if (vPtr->cmdToken != 0) {
        DeleteCommand(vPtr);            /* Command already exists, delete
                                         * old first */
    }
    if (cmdName != NULL) {
        Tcl_DString ds2;
        
        Tcl_DStringInit(&ds2);
        if (cmdName != qualName) {
            if (!Blt_ParseObjectName(interp, cmdName, &objName, 0)) {
                goto error;
            }
            cmdName = Blt_MakeQualifiedName(&objName, &ds2);
        }
        vPtr->cmdToken = Tcl_CreateObjCommand(interp, (char *)cmdName, 
                Blt_Vec_InstCmd, vPtr, VectorInstDeleteProc);
        Tcl_DStringFree(&ds2);
    }
  checkVariable:
    if (varName != NULL) {
        if ((varName[0] == '#') && (strcmp(varName, "#auto") == 0)) {
            varName = qualName;
        }
        if (Blt_Vec_MapVariable(interp, vPtr, varName) != TCL_OK) {
            goto error;
        }
    }

    Tcl_DStringFree(&ds);
    *isNewPtr = isNew;
    return vPtr;

  error:
    Tcl_DStringFree(&ds);
    if (vPtr != NULL) {
        Blt_Vec_Free(vPtr);
    }
    return NULL;
}


int
Blt_Vec_Duplicate(Vector *destPtr, Vector *srcPtr)
{
    int numBytes;

    if (destPtr == srcPtr) {
        /* Copying the same vector. */
    }
    if (Blt_Vec_ChangeLength(destPtr->interp, destPtr, srcPtr->length)
        != TCL_OK) {
        return TCL_ERROR;
    }
    numBytes = srcPtr->length * sizeof(double);
    memcpy(destPtr->valueArr, srcPtr->valueArr + srcPtr->first, numBytes);
    destPtr->offset = srcPtr->offset;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorNamesOp --
 *
 *      Reports the names of all the current vectors in the interpreter.
 *
 * Results:
 *      A standard TCL result.  interp->result will contain a list of
 *      all the names of the vector instances.
 *
 *      blt::vector names ?pattern ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
VectorNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    VectorCmdInterpData *dataPtr = clientData;
    Tcl_Obj *listObjPtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&dataPtr->vectorTable, &cursor);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        const char *name;
        int i;
        int match;
        
        name = Blt_GetHashKey(&dataPtr->vectorTable, hPtr);
        match = (objc == 2);
        for (i = 2; i < objc; i++) {
            char *pattern;

            pattern = Tcl_GetString(objv[i]);
            if (Tcl_StringMatch(name, pattern)) {
                match = TRUE;
                break;
            }
        }
        if (match) {
            Tcl_Obj *objPtr;

            objPtr = Tcl_NewStringObj(name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OldVectorCreate --
 *
 *      Creates a TCL command, and array variable representing an instance
 *      of a vector.
 *
 *      vector a b c 
 *      vector a(1) b(20) c(20)
 *      vector c(-5:14)
 *
 *      blt::vector create ?name? -size 10 -command a -variable b -first 1 \
 *              -last 15
 *      blt::vector create #auto
 *      blt::vector create 

 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
OldVectorCreate(ClientData clientData, Tcl_Interp *interp, int argStart,
                int objc, Tcl_Obj *const *objv)
{
    VectorCmdInterpData *dataPtr = clientData;
    Vector *vPtr;
    long count, i;
    CreateSwitches switches;

    /*
     * Handle switches to the vector command and collect the vector name
     * arguments into an array.
     */
    vPtr = NULL;
    for (i = argStart; i < objc; i++) {
        char *string;

        string = Tcl_GetString(objv[i]);
        if (string[0] == '-') {
            break;
        }
    }
    count = i - argStart;
    if (count == 0) {
        Tcl_AppendResult(interp, "no vector names supplied", (char *)NULL);
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, createSwitches, objc - i, objv + i, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (count > 1) {
        if (switches.cmdName != NULL) {
            Tcl_AppendResult(interp, 
                "can't specify more than one vector with \"-command\" switch",
                (char *)NULL);
            goto error;
        }
        if (switches.varName != NULL) {
            Tcl_AppendResult(interp,
                "can't specify more than one vector with \"-variable\" switch",
                (char *)NULL);
            goto error;
        }
    }
    for (i = 0; i < count; i++) {
        char *leftParen, *rightParen;
        char *string;
        int isNew;
        int size, first, last;

        size = first = last = 0;
        string = Tcl_GetString(objv[i + argStart]);
        leftParen = strchr(string, '(');
        rightParen = strchr(string, ')');
        if (((leftParen != NULL) && (rightParen == NULL)) ||
            ((leftParen == NULL) && (rightParen != NULL)) ||
            (leftParen > rightParen)) {
            Tcl_AppendResult(interp, "bad vector specification \"", string,
                "\"", (char *)NULL);
            goto error;
        }
        if (leftParen != NULL) {
            int result;
            char *colon;

            *rightParen = '\0';
            colon = strchr(leftParen + 1, ':');
            if (colon != NULL) {

                /* Specification is in the form vecName(first:last) */
                *colon = '\0';
                result = Tcl_GetInt(interp, leftParen+1, &first);
                if ((*(colon + 1) != '\0') && (result == TCL_OK)) {
                    result = Tcl_GetInt(interp, colon + 1, &last);
                    if (first >= last) {
                        Tcl_AppendResult(interp, "bad vector range \"",
                            string, "\"", (char *)NULL);
                        result = TCL_ERROR;
                    }
                    size = last - first;
                }
                *colon = ':';
            } else {
                /* Specification is in the form vecName(size) */
                result = Tcl_GetInt(interp, leftParen + 1, &size);
            }
            *rightParen = ')';
            if (result != TCL_OK) {
                goto error;
            }
        }
        if (leftParen != NULL) {
            *leftParen = '\0';
        }
        /* By default, the TCL command is the name of the vector. */
        vPtr = Blt_Vec_Create(dataPtr, string,
            (switches.cmdName == NULL) ? string : switches.cmdName,
            (switches.varName == NULL) ? string : switches.varName, &isNew);
        if (leftParen != NULL) {
            *leftParen = '(';
        }
        if (vPtr == NULL) {
            goto error;
        }
        vPtr->freeOnUnset = switches.watchUnset;
        vPtr->flush = switches.flush;
        vPtr->offset = first;
        if (size > 0) {
            if (Blt_Vec_ChangeLength(interp, vPtr, size) != TCL_OK) {
                goto error;
            }
        }
        if (!isNew) {
            if (vPtr->flush) {
                Blt_Vec_FlushCache(vPtr);
            }
            Blt_Vec_UpdateClients(vPtr);
        }
    }
    Blt_FreeSwitches(createSwitches, (char *)&switches, 0);
    if (vPtr != NULL) {
        /* Return the name of the last vector created  */
        Tcl_SetStringObj(Tcl_GetObjResult(interp), vPtr->name, -1);
    }
    return TCL_OK;
  error:
    Blt_FreeSwitches(createSwitches, (char *)&switches, 0);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * GenerateName --
 *
 *      Generates an unique vector command name.  
 *      
 * Results:
 *      Returns the unique name.  The string itself is stored in the
 *      dynamic string passed into the routine.
 *
 *---------------------------------------------------------------------------
 */
static const char *
GenerateName(VectorCmdInterpData *dataPtr, Tcl_Interp *interp, 
        const char *prefix, const char *suffix, Tcl_DString *resultPtr)
{

    const char *vecName;

    /* 
     * Parse the command and put back so that it's in a consistent
     * format.  
     *
     *  t1         <current namespace>::t1
     *  n1::t1     <current namespace>::n1::t1
     *  ::t1       ::t1
     *  ::n1::t1   ::n1::t1
     */
    vecName = NULL;                     /* Suppress compiler warning. */
    while (dataPtr->nextId < INT_MAX) {
        Blt_ObjectName objName;
        Tcl_DString ds;
        char string[200];

        dataPtr->nextId++;
        Tcl_DStringInit(&ds);
        Tcl_DStringAppend(&ds, prefix, -1);
        Blt_FmtString(string, 200, "vector%d", dataPtr->nextId);
        Tcl_DStringAppend(&ds, string, -1);
        Tcl_DStringAppend(&ds, suffix, -1);
        if (!Blt_ParseObjectName(interp, Tcl_DStringValue(&ds), &objName, 0)) {
            Tcl_DStringFree(&ds);
            return NULL;
        }
        vecName = Blt_MakeQualifiedName(&objName, resultPtr);
        Tcl_DStringFree(&ds);

        if (Blt_VectorExists2(interp, vecName)) {
            continue;                   /* A vector by this name already
                                         * exists. */
        }
        if (Blt_CommandExists(interp, vecName)) {
            continue;                   /* A command by this name already
                                         * exists. */
        }
        break;
    }
    return vecName;
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorCreateOp --
 *
 *      Creates a TCL command, and array variable representing an instance
 *      of a vector.
 *
 *      blt::vector create ?name? -size 10 -command a -variable b -first 1 \
 *              -last 15
 *      blt::vector create #auto
 *      blt::vector create 
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
VectorCreateOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    VectorCmdInterpData *dataPtr = clientData;
    Vector *vPtr;
    CreateSwitches switches;
    int isNew;
    Blt_HashEntry *hPtr;
    const char *qualName, *varName;
    Blt_ObjectName objName;
    const char *name;
    Tcl_DString ds, ds2, ds3;
        
    Tcl_DStringInit(&ds2);
    Tcl_DStringInit(&ds3);
    name = NULL;
    varName = NULL;
    if (objc >= 3) {
        const char *string;

        string = Tcl_GetString(objv[2]);
        if (string[0] != '-') {
            name = string;
            objc--, objv++;
        }
    }
    Tcl_DStringInit(&ds);
    if (name == NULL) {
        qualName = GenerateName(dataPtr, interp, "", "", &ds);
    } else {
        char *p;

        p = strstr(name, "#auto");
        if (p != NULL) {
            *p = '\0';
            qualName = GenerateName(dataPtr, interp, name, p + 5, &ds);
            *p = '#';
        } else {
            Blt_ObjectName objName;

            /* 
             * Parse the command and put back so that it's in a consistent
             * format.
             *
             *  t1         <current namespace>::t1
             *  n1::t1     <current namespace>::n1::t1
             *  ::t1       ::t1
             *  ::n1::t1   ::n1::t1
             */
            if (!Blt_ParseObjectName(interp, name, &objName, 0)) {
                return TCL_ERROR;
            }
            qualName = Blt_MakeQualifiedName(&objName, &ds);
            if (Blt_VectorExists2(interp, qualName)) {
                Tcl_AppendResult(interp, "a vector \"", qualName, 
                        "\" already exists", (char *)NULL);
                Tcl_DStringFree(&ds);
                return TCL_ERROR;
            }
            /* 
             * Check if the command already exists. 
             */
            if (Blt_CommandExists(interp, qualName)) {
                Tcl_AppendResult(interp, "a command \"", qualName,
                                 "\" already exists", (char *)NULL);
                Tcl_DStringFree(&ds);
                return TCL_ERROR;
            }
        } 
    } 
    if (qualName == NULL) {
        goto error;
    }
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, createSwitches, objc - 2, objv + 2, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        Tcl_DStringFree(&ds);
        return TCL_ERROR;
    }
    varName = NULL;
    if (switches.varName == NULL) {
        varName = qualName;
    } else if (switches.varName[0] != '\0') {
        if (!Blt_ParseObjectName(interp, switches.varName, &objName, 0)) {
            goto error;
        }
        varName = Blt_MakeQualifiedName(&objName, &ds3);
    } 
    /* By default, the TCL command is the name of the vector. */
    hPtr = Blt_CreateHashEntry(&dataPtr->vectorTable, qualName, &isNew);
    assert(isNew);
    vPtr = Blt_Vec_New(dataPtr);
    vPtr->hashPtr = hPtr;
    vPtr->nsPtr = objName.nsPtr;

    vPtr->name = Blt_GetHashKey(&dataPtr->vectorTable, hPtr);
    Blt_SetHashValue(hPtr, vPtr);

    vPtr->cmdToken = Tcl_CreateObjCommand(interp, (char *)qualName, 
        Blt_Vec_InstCmd, vPtr, VectorInstDeleteProc);

    if (varName != NULL) {
        if (Blt_Vec_MapVariable(interp, vPtr, varName) != TCL_OK) {
            goto error;
        }
    }
    vPtr->freeOnUnset = switches.watchUnset;
    vPtr->flush = switches.flush;
    vPtr->offset = 0;
    if (switches.size > 0) {
        if (Blt_Vec_ChangeLength(interp, vPtr, switches.size) != TCL_OK) {
            goto error;
        }
    }
    Tcl_DStringFree(&ds2);
    Tcl_DStringFree(&ds3);
    Tcl_DStringFree(&ds);
    Blt_FreeSwitches(createSwitches, (char *)&switches, 0);
    /* Return the name of the last vector created  */
    Tcl_SetStringObj(Tcl_GetObjResult(interp), vPtr->name, -1);
    return TCL_OK;
 error:
    Blt_FreeSwitches(createSwitches, (char *)&switches, 0);
    Tcl_DStringFree(&ds2);
    Tcl_DStringFree(&ds3);
    Tcl_DStringFree(&ds);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorDestroyOp --
 *
 *      Destroys the vector and its related TCL command and array variable
 *      (if they exist).
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      Deletes the vector.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
VectorDestroyOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    VectorCmdInterpData *dataPtr = clientData;
    Vector *vPtr;
    int i;

    for (i = 2; i < objc; i++) {
        if (Blt_Vec_Find(dataPtr, Tcl_GetString(objv[i]), &vPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        Blt_Vec_Free(vPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorExprOp --
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
VectorExprOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    return Blt_ExprVector(interp, Tcl_GetString(objv[2]), (Blt_Vector *)NULL);
}

static Blt_OpSpec vectorCmdOps[] =
{
    {"create",  1, VectorCreateOp,  2, 0, "?vecName? ?switches...?",},
    {"destroy", 1, VectorDestroyOp, 2, 0, "?vecName...?",},
    {"expr",    1, VectorExprOp,    3, 3, "expression",},
    {"names",   1, VectorNamesOp,   2, 3, "?pattern?...",},
};

static int numCmdOps = sizeof(vectorCmdOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
VectorCmd(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    VectorCmdProc *proc;

    /* Try to replicate the old vector command's behavior: */
    if (objc > 1) {
        char *string;
        char c;
        int i;
        Blt_OpSpec *specPtr;

        string = Tcl_GetString(objv[1]);
        c = string[0];
        for (specPtr = vectorCmdOps, i = 0; i < numCmdOps; i++, specPtr++) {
            if ((c == specPtr->name[0]) &&
                (strcmp(string, specPtr->name) == 0)) {
                goto doOp;
            }
        }
        /*
         * The first argument is not an operation, so assume that its
         * actually the name of a vector to be created.
         */
        return OldVectorCreate(clientData, interp, 1, objc, objv);
    }
  doOp:
    /* Do the usual vector operation lookup now. */
    proc = Blt_GetOpFromObj(interp, numCmdOps, vectorCmdOps, BLT_OP_ARG1, 
        objc, objv,0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorInterpDeleteProc --
 *
 *      This is called when the interpreter hosting the "vector" command is
 *      deleted.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Destroys the math and index hash tables.  In addition removes the
 *      hash table managing all vector names.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
VectorInterpDeleteProc(ClientData clientData, Tcl_Interp *interp)
{
    VectorCmdInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    
    for (hPtr = Blt_FirstHashEntry(&dataPtr->vectorTable, &cursor);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        Vector *vPtr;

        vPtr = Blt_GetHashValue(hPtr);
        vPtr->hashPtr = NULL;
        Blt_Vec_Free(vPtr);
    }
    Blt_DeleteHashTable(&dataPtr->vectorTable);

    /* If any user-defined math functions were installed, remove them.  */
    Blt_Vec_UninstallMathFunctions(&dataPtr->mathProcTable);
    Blt_DeleteHashTable(&dataPtr->mathProcTable);

    Blt_DeleteHashTable(&dataPtr->indexProcTable);
    Tcl_DeleteAssocData(interp, VECTOR_THREAD_KEY);
    Blt_Free(dataPtr);
}

VectorCmdInterpData *
Blt_Vec_GetInterpData(Tcl_Interp *interp)
{
    VectorCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (VectorCmdInterpData *)
        Tcl_GetAssocData(interp, VECTOR_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
        dataPtr = Blt_AssertMalloc(sizeof(VectorCmdInterpData));
        dataPtr->interp = interp;
        dataPtr->nextId = 0;
        Tcl_SetAssocData(interp, VECTOR_THREAD_KEY, VectorInterpDeleteProc,
                 dataPtr);
        Blt_InitHashTable(&dataPtr->vectorTable, BLT_STRING_KEYS);
        Blt_InitHashTable(&dataPtr->mathProcTable, BLT_STRING_KEYS);
        Blt_InitHashTable(&dataPtr->indexProcTable, BLT_STRING_KEYS);
        Blt_Vec_InstallMathFunctions(&dataPtr->mathProcTable);
        Blt_Vec_InstallSpecialIndices(&dataPtr->indexProcTable);
#ifdef HAVE_SRAND48
        srand48(time((time_t *) NULL));
#endif
    }
    return dataPtr;
}

/*ARGSUSED*/
static int
SimplifyCmd(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Blt_Vector *x, *y, *xr, *yr;
    Point2d *origPts;
    double *xArr, *yArr;
    long *indices;
    long i;
    long numOrigPts, numReducedPts;
    double tolerance = 0.05;
    
    if ((objc < 5) || (objc > 6)) {
        Tcl_AppendResult(interp, "wrong # arguments: should be \"",
                Tcl_GetString(objv[0]), " simplify x y rx ry ?tol?",
                (char *)NULL);
        return TCL_ERROR;
    }
    if ((Blt_GetVectorFromObj(interp, objv[1], &x) != TCL_OK) ||
        (Blt_GetVectorFromObj(interp, objv[2], &y) != TCL_OK) ||
        (Blt_GetVectorFromObj(interp, objv[3], &xr) != TCL_OK) ||
        (Blt_GetVectorFromObj(interp, objv[4], &yr) != TCL_OK)) {
        return TCL_ERROR;
    }
    if ((objc > 5) &&
        (Tcl_GetDoubleFromObj(interp, objv[5], &tolerance) != TCL_OK)) {
        return TCL_ERROR;
    }
    numOrigPts = Blt_VecLength(x);
    if (numOrigPts < 3) {
        Tcl_AppendResult(interp, "length of vector \"", Tcl_GetString(objv[2]),
                         "\" is < 3", (char *)NULL);
        return TCL_ERROR;
    }
    if (numOrigPts != Blt_VecLength(y)) {
        Tcl_AppendResult(interp, "vectors \"", Tcl_GetString(objv[1]), 
                         "\" and \"", Tcl_GetString(objv[2]),
                         " have different lengths", (char *)NULL);
        return TCL_ERROR;
    }

    origPts = Blt_Malloc(sizeof(Point2d) * numOrigPts);
    if (origPts == NULL) {
        Tcl_AppendResult(interp, "can't allocate \"", Blt_Ltoa(numOrigPts), 
                "\" points", (char *)NULL);
        return TCL_ERROR;
    }
    xArr = Blt_VecData(x);
    yArr = Blt_VecData(y);
    for (i = 0; i < numOrigPts; i++) {
        origPts[i].x = xArr[i];
        origPts[i].y = yArr[i];
    }
    indices = Blt_Malloc(sizeof(long) * numOrigPts);
    if (indices == NULL) {
        Tcl_AppendResult(interp, "can't allocate \"", Blt_Ltoa(numOrigPts), 
                "\" indices for simplication array", (char *)NULL);
        Blt_Free(origPts);
        return TCL_ERROR;
    }
    numReducedPts = Blt_SimplifyLine(origPts, 0, numOrigPts - 1, tolerance,
                indices);
    if ((Blt_ResizeVector(xr, numReducedPts) != TCL_OK) ||
        (Blt_ResizeVector(yr, numReducedPts) != TCL_OK)) {
        return TCL_ERROR;
    }
    xArr = Blt_VecData(xr);
    yArr = Blt_VecData(yr);
    for (i = 0; i < numReducedPts; i++) {
        xArr[i] = origPts[indices[i]].x;
        yArr[i] = origPts[indices[i]].y;
    }
    Blt_Free(origPts);
    Blt_Free(indices);

    /* Finally update the vector. The size of the vector hasn't changed,
     * just the data. Reset the vector using TCL_STATIC to indicate
     * this. */
    if (Blt_ResetVector(xr, Blt_VecData(xr), Blt_VecLength(xr),
            Blt_VecSize(xr), TCL_STATIC) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_ResetVector(yr, Blt_VecData(yr), Blt_VecLength(yr),
            Blt_VecSize(yr), TCL_STATIC) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_VectorCmdInitProc --
 *
 *      This procedure is invoked to initialize the "vector" command.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Creates the new command and adds a new entry into a global Tcl
 *      associative array.
 *
 *---------------------------------------------------------------------------
 */

int
Blt_VectorCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpecs[2] = {
        {"vector", VectorCmd },
        {"simplify", SimplifyCmd }
    };
    cmdSpecs[0].clientData = Blt_Vec_GetInterpData(interp);
    return Blt_InitCmds(interp, "::blt", cmdSpecs, 2);
}



/* C Application interface to vectors */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreateVector --
 *
 *      Creates a new vector by the name and size.
 *
 * Results:
 *      A standard TCL result.  If the new array size is invalid or a
 *      vector already exists by that name, TCL_ERROR is returned.
 *      Otherwise TCL_OK is returned and the new vector is created.
 *
 * Side Effects:
 *      Memory will be allocated for the new vector.  A new TCL command and
 *      TCL array variable will be created.
 *
 *---------------------------------------------------------------------------
 */

/*LINTLIBRARY*/
int
Blt_CreateVector2(
    Tcl_Interp *interp,
    const char *vecName, const char *cmdName, const char *varName,
    int initialSize,
    Blt_Vector **vecPtrPtr)
{
    VectorCmdInterpData *dataPtr;       /* Interpreter-specific data. */
    Vector *vPtr;
    int isNew;
    const char *nameCopy;

    dataPtr = Blt_Vec_GetInterpData(interp);

    nameCopy = Blt_AssertStrdup(vecName);
    vPtr = Blt_Vec_Create(dataPtr, nameCopy, cmdName, varName, &isNew);
    Blt_Free(nameCopy);

    if (vPtr == NULL) {
        return TCL_ERROR;
    }
    if (initialSize > 0) {
        if (Blt_Vec_ChangeLength(interp, vPtr, initialSize) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (vecPtrPtr != NULL) {
        *vecPtrPtr = (Blt_Vector *) vPtr;
    }
    return TCL_OK;
}

int
Blt_CreateVector(Tcl_Interp *interp, const char *name, int size,
                 Blt_Vector **vecPtrPtr)
{
    return Blt_CreateVector2(interp, name, name, name, size, vecPtrPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DeleteVector --
 *
 *      Deletes the vector of the given name.  All clients with designated
 *      callback routines will be notified.
 *
 * Results:
 *      A standard TCL result.  If no vector exists by that name, TCL_ERROR
 *      is returned.  Otherwise TCL_OK is returned and vector is deleted.
 *
 * Side Effects:
 *      Memory will be released for the new vector.  Both the TCL command
 *      and array variable will be deleted.  All clients which set call
 *      back procedures will be notified.
 *
 *---------------------------------------------------------------------------
 */
/*LINTLIBRARY*/
int
Blt_DeleteVector(Blt_Vector *vecPtr)
{
    Vector *vPtr = (Vector *)vecPtr;

    Blt_Vec_Free(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DeleteVectorByName --
 *
 *      Deletes the vector of the given name.  All clients with designated
 *      callback routines will be notified.
 *
 * Results:
 *      A standard TCL result.  If no vector exists by that name, TCL_ERROR
 *      is returned.  Otherwise TCL_OK is returned and vector is deleted.
 *
 * Side Effects:
 *      Memory will be released for the new vector.  Both the TCL command
 *      and array variable will be deleted.  All clients which set call
 *      back procedures will be notified.
 *
 *---------------------------------------------------------------------------
 */
/*LINTLIBRARY*/
int
Blt_DeleteVectorByName(Tcl_Interp *interp, const char *name)
{
    VectorCmdInterpData *dataPtr;       /* Interpreter-specific data. */
    Vector *vPtr;
    const char *nameCopy;
    int result;

    /*
     * If the vector name was passed via a read-only string (e.g. "x"), the
     * Blt_Vec_ParseElement routine will segfault when it tries to write
     * into the string.  Therefore make a writable copy and free it when
     * we're done.
     */
    nameCopy = Blt_AssertStrdup(name);
    dataPtr = Blt_Vec_GetInterpData(interp);
    result = Blt_Vec_Find(dataPtr, nameCopy, &vPtr);
    Blt_Free(nameCopy);

    if (result != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_Vec_Free(vPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_VectorExists2 --
 *
 *      Returns whether the vector associated with the client token still
 *      exists.
 *
 * Results:
 *      Returns 1 is the vector still exists, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_VectorExists2(Tcl_Interp *interp, const char *vecName)
{
    VectorCmdInterpData *dataPtr;       /* Interpreter-specific data. */

    dataPtr = Blt_Vec_GetInterpData(interp);
    if (GetVectorObject(dataPtr, vecName, NS_SEARCH_BOTH) != NULL) {
        return TRUE;
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_VectorExists --
 *
 *      Returns whether the vector associated with the client token still
 *      exists.
 *
 * Results:
 *      Returns 1 is the vector still exists, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_VectorExists(Tcl_Interp *interp, const char *vecName)
{
    const char *nameCopy;
    int result;

    /*
     * If the vector name was passed via a read-only string (e.g. "x"), the
     * Blt_VectorParseName routine will segfault when it tries to write
     * into the string.  Therefore make a writable copy and free it when
     * we're done.
     */
    nameCopy = Blt_AssertStrdup(vecName);
    result = Blt_VectorExists2(interp, nameCopy);
    Blt_Free(nameCopy);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetVector --
 *
 *      Returns a pointer to the vector associated with the given name.
 *
 * Results:
 *      A standard TCL result.  If there is no vector "name", TCL_ERROR is
 *      returned.  Otherwise TCL_OK is returned and vecPtrPtr will point to
 *      the vector.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetVector(Tcl_Interp *interp, const char *name, Blt_Vector **vecPtrPtr)
{
    VectorCmdInterpData *dataPtr;       /* Interpreter-specific data. */
    Vector *vPtr;
    const char *nameCopy;
    int result;

    dataPtr = Blt_Vec_GetInterpData(interp);
    /*
     * If the vector name was passed via a read-only string (e.g. "x"), the
     * Blt_VectorParseName routine will segfault when it tries to write
     * into the string.  Therefore make a writable copy and free it when
     * we're done.
     */
    nameCopy = Blt_AssertStrdup(name);
    result = Blt_Vec_Find(dataPtr, nameCopy, &vPtr);
    Blt_Free(nameCopy);
    if (result != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_Vec_UpdateRange(vPtr);
    *vecPtrPtr = (Blt_Vector *) vPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetVectorFromObj --
 *
 *      Returns a pointer to the vector associated with the given name.
 *
 * Results:
 *      A standard TCL result.  If there is no vector "name", TCL_ERROR is
 *      returned.  Otherwise TCL_OK is returned and vecPtrPtr will point to
 *      the vector.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetVectorFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                     Blt_Vector **vecPtrPtr)
{
    VectorCmdInterpData *dataPtr;       /* Interpreter-specific data. */
    Vector *vPtr;

    dataPtr = Blt_Vec_GetInterpData(interp);
    if (Blt_Vec_Find(dataPtr, Tcl_GetString(objPtr), &vPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_Vec_UpdateRange(vPtr);
    *vecPtrPtr = (Blt_Vector *) vPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResetVector --
 *
 *      Resets the vector data.  This is called by a client to indicate
 *      that the vector data has changed.  The vector does not need to
 *      point to different memory.  Any clients of the vector will be
 *      notified of the change.
 *
 * Results:
 *      A standard TCL result.  If the new array size is invalid, TCL_ERROR
 *      is returned.  Otherwise TCL_OK is returned and the new vector data
 *      is recorded.
 *
 * Side Effects:
 *      Any client designated callbacks will be posted.  Memory may be
 *      changed for the vector array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ResetVector(
    Blt_Vector *vecPtr,
    double *valueArr,                   /* Array containing the elements of
                                         * the vector. If NULL, indicates
                                         * to reset the vector.*/
    int length,                         /* The number of elements that the
                                         * vector currently holds. */
    int size,                           /* The maximum number of elements
                                         * that the array can hold. */
    Tcl_FreeProc *freeProc)             /* Address of memory deallocation
                                         * routine for the array of values.
                                         * Can also be TCL_STATIC,
                                         * TCL_DYNAMIC, or TCL_VOLATILE. */
{
    Vector *vPtr = (Vector *)vecPtr;

    return Blt_Vec_Reset(vPtr, valueArr, length, size, freeProc);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResizeVector --
 *
 *      Changes the size of the vector.  All clients with designated
 *      callback routines will be notified of the size change.
 *
 * Results:
 *      A standard TCL result.  If no vector exists by that name, TCL_ERROR
 *      is returned.  Otherwise TCL_OK is returned and vector is resized.
 *
 * Side Effects:
 *      Memory may be reallocated for the new vector size.  All clients
 *      which set call back procedures will be notified.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ResizeVector(Blt_Vector *vecPtr, int length)
{
    Vector *vPtr = (Vector *)vecPtr;

    if (Blt_Vec_ChangeLength((Tcl_Interp *)NULL, vPtr, length) != TCL_OK) {
        Tcl_AppendResult(vPtr->interp, "can't resize vector \"", vPtr->name,
            "\"", (char *)NULL);
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
 * Blt_AllocVectorId --
 *
 *      Creates an identifier token for an existing vector.  The identifier
 *      is used by the client routines to get call backs when (and if) the
 *      vector changes.
 *
 * Results:
 *      A standard TCL result.  If "vecName" is not associated with a
 *      vector, TCL_ERROR is returned and interp->result is filled with an
 *      error message.
 *
 *---------------------------------------------------------------------------
 */
Blt_VectorId
Blt_AllocVectorId(Tcl_Interp *interp, const char *name)
{
    VectorCmdInterpData *dataPtr;       /* Interpreter-specific data. */
    Vector *vPtr;
    VectorClient *clientPtr;
    Blt_VectorId clientId;
    int result;
    const char *nameCopy;

    dataPtr = Blt_Vec_GetInterpData(interp);
    /*
     * If the vector name was passed via a read-only string (e.g. "x"), the
     * Blt_VectorParseName routine will segfault when it tries to write
     * into the string.  Therefore make a writable copy and free it when
     * we're done.
     */
    nameCopy = Blt_AssertStrdup(name);
    result = Blt_Vec_Find(dataPtr, nameCopy, &vPtr);
    Blt_Free(nameCopy);

    if (result != TCL_OK) {
        return (Blt_VectorId) 0;
    }
    /* Allocate a new client structure */
    clientPtr = Blt_AssertCalloc(1, sizeof(VectorClient));
    clientPtr->magic = VECTOR_MAGIC;

    /* Add the new client to the server's list of clients */
    clientPtr->link = Blt_Chain_Append(vPtr->chain, clientPtr);
    clientPtr->serverPtr = vPtr;
    clientId = (Blt_VectorId) clientPtr;
    return clientId;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_SetVectorChangedProc --
 *
 *      Sets the routine to be called back when the vector is changed or
 *      deleted.  *clientData* will be provided as an argument. If *proc*
 *      is NULL, no callback will be made.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The designated routine will be called when the vector is changed
 *      or deleted.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_SetVectorChangedProc(
    Blt_VectorId clientId,              /* Client token identifying the
                                         * vector */
    Blt_VectorChangedProc *proc,        /* Address of routine to call when
                                         * the contents of the vector
                                         * change. If NULL, no routine will
                                         * be called */
    ClientData clientData)              /* One word of information to pass
                                         * along when the above routine is
                                         * called */
{
    VectorClient *clientPtr = (VectorClient *)clientId;

    if (clientPtr->magic != VECTOR_MAGIC) {
        return;                 /* Not a valid token */
    }
    clientPtr->clientData = clientData;
    clientPtr->proc = proc;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreeVectorId --
 *
 *      Releases the token for an existing vector.  This indicates that the
 *      client is no longer interested the vector.  Any previously
 *      specified callback routine will no longer be invoked when (and if)
 *      the vector changes.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Any previously specified callback routine will no longer be invoked
 *      when (and if) the vector changes.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FreeVectorId(Blt_VectorId clientId)
{
    VectorClient *clientPtr = (VectorClient *)clientId;

    if (clientPtr->magic != VECTOR_MAGIC) {
        return;                         /* Not a valid token */
    }
    if (clientPtr->serverPtr != NULL) {
        /* Remove the client from the server's list */
        Blt_Chain_DeleteLink(clientPtr->serverPtr->chain, clientPtr->link);
    }
    Blt_Free(clientPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NameOfVectorId --
 *
 *      Returns the name of the vector (and array variable).
 *
 * Results:
 *      The name of the array variable is returned.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_NameOfVectorId(Blt_VectorId clientId) 
{
    VectorClient *clientPtr = (VectorClient *)clientId;

    if ((clientPtr->magic != VECTOR_MAGIC) || (clientPtr->serverPtr == NULL)) {
        return NULL;
    }
    return clientPtr->serverPtr->name;
}

const char *
Blt_NameOfVector(Blt_Vector *vecPtr)    /* Vector to query. */
{
    Vector *vPtr = (Vector *)vecPtr;
    return vPtr->name;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_VectorNotifyPending --
 *
 *      Returns the name of the vector (and array variable).
 *
 * Results:
 *      The name of the array variable is returned.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_VectorNotifyPending(Blt_VectorId clientId)
{
    VectorClient *clientPtr = (VectorClient *)clientId;

    if ((clientPtr == NULL) || (clientPtr->magic != VECTOR_MAGIC) || 
        (clientPtr->serverPtr == NULL)) {
        return 0;
    }
    return (clientPtr->serverPtr->notifyFlags & NOTIFY_PENDING);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetVectorById --
 *
 *      Returns a pointer to the vector associated with the client token.
 *
 * Results:
 *      A standard TCL result.  If the client token is not associated with
 *      a vector any longer, TCL_ERROR is returned. Otherwise, TCL_OK is
 *      returned and vecPtrPtr will point to vector.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetVectorById(
    Tcl_Interp *interp,
    Blt_VectorId clientId,              /* Client token identifying the
                                         * vector */
    Blt_Vector **vecPtrPtr)
{
    VectorClient *clientPtr = (VectorClient *)clientId;

    if (clientPtr->magic != VECTOR_MAGIC) {
        Tcl_AppendResult(interp, "bad vector token", (char *)NULL);
        return TCL_ERROR;
    }
    if (clientPtr->serverPtr == NULL) {
        Tcl_AppendResult(interp, "vector no longer exists", (char *)NULL);
        return TCL_ERROR;
    }
    Blt_Vec_UpdateRange(clientPtr->serverPtr);
    *vecPtrPtr = (Blt_Vector *) clientPtr->serverPtr;
    return TCL_OK;
}

/*LINTLIBRARY*/
void
Blt_InstallIndexProc(Tcl_Interp *interp, const char *string, 
                     Blt_VectorIndexProc *procPtr) 
{
    VectorCmdInterpData *dataPtr;       /* Interpreter-specific data. */
    Blt_HashEntry *hPtr;
    int isNew;

    dataPtr = Blt_Vec_GetInterpData(interp);
    hPtr = Blt_CreateHashEntry(&dataPtr->indexProcTable, string, &isNew);
    if (procPtr == NULL) {
        Blt_DeleteHashEntry(&dataPtr->indexProcTable, hPtr);
    } else {
        Blt_SetHashValue(hPtr, procPtr);
    }
}

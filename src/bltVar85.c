/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltVar85.c --
 *
 * This module implements TCL 8.5 variable handler procedures for the BLT
 * toolkit.
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
#include "bltAlloc.h"
#include <bltHash.h>
#include <bltList.h>
#include "bltNsUtil.h"
#include "bltVar.h"


/* 
 * Variable resolver routines.
 *
 * The following bit of magic is from [incr Tcl].  The following routine
 * are taken from [incr Tcl] to roughly duplicate how Tcl internally
 * creates variables.
 *
 * Note: There is no API for the variable resolver routines in the Tcl
 *       library.  The resolver callback is supposed to return a Tcl_Var
 *       back. But the definition of Tcl_Var in tcl.h is opaque.
 */

typedef struct TclVarHashTable {
    Tcl_HashTable table;
    Tcl_Namespace *nsPtr;
} TclVarHashTable;

/*
 * The structure below defines a variable, which associates a string name
 * with a Tcl_Obj value. These structures are kept in procedure call frames
 * (for local variables recognized by the compiler) or in the heap (for
 * global variables and any variable not known to the compiler). For each
 * Var structure in the heap, a hash table entry holds the variable name
 * and a pointer to the Var structure.
 */

typedef struct Var {
    int flags;                  /* Miscellaneous bits of information about
                                 * variable. See below for definitions. */
    union {
        Tcl_Obj *objPtr;        /* The variable's object value. Used for
                                 * scalar variables and array elements. */
        TclVarHashTable *tablePtr;/* For array variables, this points to
                                 * information about the hash table used to
                                 * implement the associative array. Points to
                                 * ckalloc-ed data. */
        struct Var *linkPtr;    /* If this is a global variable being referred
                                 * to in a procedure, or a variable created by
                                 * "upvar", this field points to the
                                 * referenced variable's Var struct. */
    } value;
} Var;

typedef struct VarInHash {
    Var var;
    int refCount;               /* Counts number of active uses of this
                                 * variable: 1 for the entry in the hash
                                 * table, 1 for each additional variable whose
                                 * linkPtr points here, 1 for each nested
                                 * trace active on variable, and 1 if the
                                 * variable is a namespace variable. This
                                 * record can't be deleted until refCount
                                 * becomes 0. */
    Tcl_HashEntry entry;        /* The hash table entry that refers to this
                                 * variable. This is used to find the name of
                                 * the variable and to delete it from its
                                 * hashtable if it is no longer needed. It
                                 * also holds the variable's name. */
} VarInHash;

/*
 * Flag bits for variables. The first two (VAR_ARRAY and VAR_LINK) are
 * mutually exclusive and give the "type" of the variable. If none is set,
 * this is a scalar variable.
 *
 * VAR_ARRAY -                  1 means this is an array variable rather than
 *                              a scalar variable or link. The "tablePtr"
 *                              field points to the array's hashtable for its
 *                              elements.
 * VAR_LINK -                   1 means this Var structure contains a pointer
 *                              to another Var structure that either has the
 *                              real value or is itself another VAR_LINK
 *                              pointer. Variables like this come about
 *                              through "upvar" and "global" commands, or
 *                              through references to variables in enclosing
 *                              namespaces.
 *
 * Flags that indicate the type and status of storage; none is set for
 * compiled local variables (Var structs).
 *
 * VAR_IN_HASHTABLE -           1 means this variable is in a hashtable and
 *                              the Var structure is malloced. 0 if it is a
 *                              local variable that was assigned a slot in a
 *                              procedure frame by the compiler so the Var
 *                              storage is part of the call frame.
 * VAR_DEAD_HASH                1 means that this var's entry in the hashtable
 *                              has already been deleted.
 * VAR_ARRAY_ELEMENT -          1 means that this variable is an array
 *                              element, so it is not legal for it to be an
 *                              array itself (the VAR_ARRAY flag had better
 *                              not be set).
 * VAR_NAMESPACE_VAR -          1 means that this variable was declared as a
 *                              namespace variable. This flag ensures it
 *                              persists until its namespace is destroyed or
 *                              until the variable is unset; it will persist
 *                              even if it has not been initialized and is
 *                              marked undefined. The variable's refCount is
 *                              incremented to reflect the "reference" from
 *                              its namespace.
 *
 * Flag values relating to the variable's trace and search status.
 *
 * VAR_TRACED_READ
 * VAR_TRACED_WRITE
 * VAR_TRACED_UNSET
 * VAR_TRACED_ARRAY
 * VAR_TRACE_ACTIVE -           1 means that trace processing is currently
 *                              underway for a read or write access, so new
 *                              read or write accesses should not cause trace
 *                              procedures to be called and the variable can't
 *                              be deleted.
 * VAR_SEARCH_ACTIVE
 *
 * The following additional flags are used with the CompiledLocal type defined
 * below:
 *
 * VAR_ARGUMENT -               1 means that this variable holds a procedure
 *                              argument.
 * VAR_TEMPORARY -              1 if the local variable is an anonymous
 *                              temporary variable. Temporaries have a NULL
 *                              name.
 * VAR_RESOLVED -               1 if name resolution has been done for this
 *                              variable.
 * VAR_IS_ARGS                  1 if this variable is the last argument and is
 *                              named "args".
 */

/*
 * FLAGS RENUMBERED: everything breaks already, make things simpler.
 *
 * IMPORTANT: skip the values 0x10, 0x20, 0x40, 0x800 corresponding to
 * TCL_TRACE_(READS/WRITES/UNSETS/ARRAY): makes code simpler in tclTrace.c
 *
 * Keep the flag values for VAR_ARGUMENT and VAR_TEMPORARY so that old values
 * in precompiled scripts keep working.
 */


/* Type of value (0 is scalar) */
#define VAR_ARRAY               0x1
#define VAR_LINK                0x2

/* Type of storage (0 is compiled local) */
#define VAR_IN_HASHTABLE        0x4
#define VAR_DEAD_HASH           0x8
#define VAR_ARRAY_ELEMENT       0x1000
#define VAR_NAMESPACE_VAR       0x80      /* KEEP OLD VALUE for Itcl */

#define VAR_ALL_HASH \
        (VAR_IN_HASHTABLE|VAR_DEAD_HASH|VAR_NAMESPACE_VAR|VAR_ARRAY_ELEMENT)

/* Trace and search state */

#define VAR_TRACED_READ        0x10       /* TCL_TRACE_READS  */
#define VAR_TRACED_WRITE       0x20       /* TCL_TRACE_WRITES */
#define VAR_TRACED_UNSET       0x40       /* TCL_TRACE_UNSETS */
#define VAR_TRACED_ARRAY       0x800      /* TCL_TRACE_ARRAY  */
#define VAR_TRACE_ACTIVE       0x2000
#define VAR_SEARCH_ACTIVE      0x4000
#define VAR_ALL_TRACES \
        (VAR_TRACED_READ|VAR_TRACED_WRITE|VAR_TRACED_ARRAY|VAR_TRACED_UNSET)


/* Special handling on initialisation (only CompiledLocal) */
#define VAR_ARGUMENT            0x100     /* KEEP OLD VALUE! See tclProc.c */
#define VAR_TEMPORARY           0x200     /* KEEP OLD VALUE! See tclProc.c */
#define VAR_IS_ARGS             0x400
#define VAR_RESOLVED            0x8000

/*
 * Macros to ensure that various flag bits are set properly for variables.
 * The ANSI C "prototypes" for these macros are:
 *
 * MODULE_SCOPE void    TclSetVarScalar(Var *varPtr);
 * MODULE_SCOPE void    TclSetVarArray(Var *varPtr);
 * MODULE_SCOPE void    TclSetVarLink(Var *varPtr);
 * MODULE_SCOPE void    TclSetVarArrayElement(Var *varPtr);
 * MODULE_SCOPE void    TclSetVarUndefined(Var *varPtr);
 * MODULE_SCOPE void    TclClearVarUndefined(Var *varPtr);
 */

#define TclSetVarScalar(varPtr) \
    (varPtr)->flags &= ~(VAR_ARRAY|VAR_LINK)

#define TclSetVarArray(varPtr) \
    (varPtr)->flags = ((varPtr)->flags & ~VAR_LINK) | VAR_ARRAY

#define TclSetVarLink(varPtr) \
    (varPtr)->flags = ((varPtr)->flags & ~VAR_ARRAY) | VAR_LINK

#define TclSetVarArrayElement(varPtr) \
    (varPtr)->flags = ((varPtr)->flags & ~VAR_ARRAY) | VAR_ARRAY_ELEMENT

#define TclSetVarUndefined(varPtr) \
    (varPtr)->flags &= ~(VAR_ARRAY|VAR_LINK);\
    (varPtr)->value.objPtr = NULL

#define TclClearVarUndefined(varPtr)

#define TclSetVarTraceActive(varPtr) \
    (varPtr)->flags |= VAR_TRACE_ACTIVE

#define TclClearVarTraceActive(varPtr) \
    (varPtr)->flags &= ~VAR_TRACE_ACTIVE

#define TclSetVarNamespaceVar(varPtr) \
    if (!TclIsVarNamespaceVar(varPtr)) {\
        (varPtr)->flags |= VAR_NAMESPACE_VAR;\
        ((VarInHash *)(varPtr))->refCount++;\
    }

#define TclClearVarNamespaceVar(varPtr) \
    if (TclIsVarNamespaceVar(varPtr)) {\
        (varPtr)->flags &= ~VAR_NAMESPACE_VAR;\
        ((VarInHash *)(varPtr))->refCount--;\
    }

/*
 * Macros to read various flag bits of variables.
 * The ANSI C "prototypes" for these macros are:
 *
 * MODULE_SCOPE int     TclIsVarScalar(Var *varPtr);
 * MODULE_SCOPE int     TclIsVarLink(Var *varPtr);
 * MODULE_SCOPE int     TclIsVarArray(Var *varPtr);
 * MODULE_SCOPE int     TclIsVarUndefined(Var *varPtr);
 * MODULE_SCOPE int     TclIsVarArrayElement(Var *varPtr);
 * MODULE_SCOPE int     TclIsVarTemporary(Var *varPtr);
 * MODULE_SCOPE int     TclIsVarArgument(Var *varPtr);
 * MODULE_SCOPE int     TclIsVarResolved(Var *varPtr);
 */

#define TclIsVarScalar(varPtr) \
    !((varPtr)->flags & (VAR_ARRAY|VAR_LINK))

#define TclIsVarLink(varPtr) \
    ((varPtr)->flags & VAR_LINK)

#define TclIsVarArray(varPtr) \
    ((varPtr)->flags & VAR_ARRAY)

#define TclIsVarUndefined(varPtr) \
    ((varPtr)->value.objPtr == NULL)

#define TclIsVarArrayElement(varPtr) \
    ((varPtr)->flags & VAR_ARRAY_ELEMENT)

#define TclIsVarNamespaceVar(varPtr) \
    ((varPtr)->flags & VAR_NAMESPACE_VAR)

#define TclIsVarTemporary(varPtr) \
    ((varPtr)->flags & VAR_TEMPORARY)

#define TclIsVarArgument(varPtr) \
    ((varPtr)->flags & VAR_ARGUMENT)

#define TclIsVarResolved(varPtr) \
    ((varPtr)->flags & VAR_RESOLVED)

#define TclIsVarTraceActive(varPtr) \
    ((varPtr)->flags & VAR_TRACE_ACTIVE)

#define TclIsVarTraced(varPtr) \
   ((varPtr)->flags & VAR_ALL_TRACES)

#define TclIsVarInHash(varPtr) \
    ((varPtr)->flags & VAR_IN_HASHTABLE)

#define TclIsVarDeadHash(varPtr) \
    ((varPtr)->flags & VAR_DEAD_HASH)

#define TclGetVarNsPtr(varPtr) \
    (TclIsVarInHash(varPtr) \
         ? ((TclVarHashTable *) ((((VarInHash *) (varPtr))->entry.tablePtr)))->nsPtr \
         : NULL)

#define VarHashRefCount(varPtr) \
    ((VarInHash *) (varPtr))->refCount

/*
 * Macros for direct variable access by TEBC
 */

#define TclIsVarDirectReadable(varPtr) \
    (   !((varPtr)->flags & (VAR_ARRAY|VAR_LINK|VAR_TRACED_READ)) \
    &&  (varPtr)->value.objPtr)

#define TclIsVarDirectWritable(varPtr) \
    !((varPtr)->flags & (VAR_ARRAY|VAR_LINK|VAR_TRACED_WRITE|VAR_DEAD_HASH))

#define TclIsVarDirectModifyable(varPtr) \
    (   !((varPtr)->flags & (VAR_ARRAY|VAR_LINK|VAR_TRACED_READ|VAR_TRACED_WRITE)) \
    &&  (varPtr)->value.objPtr)

#define TclIsVarDirectReadable2(varPtr, arrayPtr) \
    (TclIsVarDirectReadable(varPtr) &&\
        (!(arrayPtr) || !((arrayPtr)->flags & VAR_TRACED_READ)))

#define TclIsVarDirectWritable2(varPtr, arrayPtr) \
    (TclIsVarDirectWritable(varPtr) &&\
        (!(arrayPtr) || !((arrayPtr)->flags & VAR_TRACED_WRITE)))

#define TclIsVarDirectModifyable2(varPtr, arrayPtr) \
    (TclIsVarDirectModifyable(varPtr) &&\
        (!(arrayPtr) || !((arrayPtr)->flags & (VAR_TRACED_READ|VAR_TRACED_WRITE))))

#define VarHashGetValue(hPtr) \
    ((Var *) ((char *)hPtr - Blt_Offset(VarInHash, entry)))

static INLINE Tcl_Namespace *
NamespaceOfVariable(Var *varPtr)
{
    if (varPtr->flags & VAR_IN_HASHTABLE) {
        VarInHash *vhashPtr = (VarInHash *)varPtr;
        TclVarHashTable *vtablePtr;

        vtablePtr = (TclVarHashTable *)vhashPtr->entry.tablePtr;
        return vtablePtr->nsPtr;
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewVar --
 *
 *      Create a new heap-allocated variable that will eventually be
 *      entered into a hashtable.
 *
 * Results:
 *      The return value is a pointer to the new variable structure. It is
 *      marked as a scalar variable (and not a link or array variable). Its
 *      value initially is NULL. The variable is not part of any hash table
 *      yet. Since it will be in a hashtable and not in a call frame, its
 *      name field is set NULL. It is initially marked as undefined.
 *
 * Side effects:
 *      Storage gets allocated.
 *
 *---------------------------------------------------------------------------
 */
static Var *
NewVar(Tcl_Obj *objPtr)
{
    Var *varPtr;

    varPtr = Blt_AssertMalloc(sizeof(Var));
    varPtr->value.objPtr = objPtr;
    varPtr->flags = 0;
    return varPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetVariableNamespace --
 *
 *      Returns the namespace context of the variable.  If NULL, this
 *      indicates that the variable is local to the call frame.
 *
 * Results:
 *      Returns the context of the namespace in an opaque type.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Namespace *
Blt_GetVariableNamespace(Tcl_Interp *interp, const char *path)
{
    Blt_ObjectName objName;

    if (!Blt_ParseObjectName(interp, path, &objName, BLT_NO_DEFAULT_NS)) {
        return NULL;
    }
    if (objName.nsPtr == NULL) {
        Var *varPtr;

        varPtr = (Var *)Tcl_FindNamespaceVar(interp, (char *)path, 
                (Tcl_Namespace *)NULL, TCL_NAMESPACE_ONLY);
        if (varPtr != NULL) {
            return NamespaceOfVariable(varPtr);
        }
        varPtr = (Var *)Tcl_FindNamespaceVar(interp, (char *)path, 
                (Tcl_Namespace *)NULL, TCL_GLOBAL_ONLY);
        if (varPtr != NULL) {
            return NamespaceOfVariable(varPtr);
        }
    }
    return objName.nsPtr;    
}

Tcl_Var
Blt_GetCachedVar(Blt_HashTable *cacheTablePtr, const char *label, 
                 Tcl_Obj *objPtr)
{
    Blt_HashEntry *hPtr;
    int isNew;
    Var *varPtr;

    assert(objPtr != NULL);
    /* Check if the variable has been cached already. */
    hPtr = Blt_CreateHashEntry(cacheTablePtr, label, &isNew);
    Tcl_IncrRefCount(objPtr);
    if (isNew) {
        varPtr = NewVar(objPtr);
        Blt_SetHashValue(hPtr, varPtr);
    } else {
        varPtr = Blt_GetHashValue(hPtr);
        if (varPtr->value.objPtr != NULL) {
            Tcl_DecrRefCount(varPtr->value.objPtr);
        }
        varPtr->value.objPtr = objPtr;
    }
    return (Tcl_Var)varPtr;
}

void
Blt_FreeCachedVars(Blt_HashTable *tablePtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(tablePtr, &iter); hPtr != NULL; 
         hPtr = Blt_NextHashEntry(&iter)) {
        Var *varPtr;

        varPtr = Blt_GetHashValue(hPtr);
        Tcl_DecrRefCount(varPtr->value.objPtr);
        Blt_Free(varPtr);
    }
    Blt_DeleteHashTable(tablePtr);
}

            

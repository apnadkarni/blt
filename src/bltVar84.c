/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltVar84.c --
 *
 * This module implements TCL 8.4 variable handler procedures for the BLT
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
#include <bltList.h>
#include <bltHash.h>
#include "bltVar.h"
#include "bltNsUtil.h"

/* 
 * Variable resolver routines.
 *
 * The following bit of magic is from [incr Tcl].  The following routine are
 * taken from [incr Tcl] to roughly duplicate how TCL internally creates
 * variables.
 *
 * Note: There is no API for the variable resolver routines in the Tcl
 *	 library.  The resolver callback is supposed to return a Tcl_Var
 *	 back. But the definition of Tcl_Var in tcl.h is opaque.
 */

#define VAR_SCALAR		0x1
#define VAR_ARRAY		0x2
#define VAR_LINK		0x4
#define VAR_UNDEFINED		0x8
#define VAR_IN_HASHTABLE	0x10
#define VAR_TRACE_ACTIVE	0x20
#define VAR_ARRAY_ELEMENT	0x40
#define VAR_NAMESPACE_VAR	0x80

#define VAR_ARGUMENT		0x100
#define VAR_TEMPORARY		0x200
#define VAR_RESOLVED		0x400	

typedef struct ArraySearch ArraySearch;
typedef struct VarTrace VarTrace;
typedef struct Namespace Namespace;

typedef struct Var {
    union {
	Tcl_Obj *objPtr;	/* The variable's object value. Used for
				 * scalar variables and array elements. */
	Tcl_HashTable *tablePtr;/* For array variables, this points to
				 * information about the hash table used to
				 * implement the associative array.  Points to
				 * malloc-ed data. */
	struct Var *linkPtr;	/* If this is a global variable being referred
				 * to in a procedure, or a variable created by
				 * "upvar", this field points to the
				 * referenced variable's Var struct. */
    } value;
    char *name;			/* NULL if the variable is in a hashtable,
				 * otherwise points to the variable's name. It
				 * is used, e.g., by TclLookupVar and "info
				 * locals". The storage for the characters of
				 * the name is not owned by the Var and must
				 * not be freed when freeing the Var. */
    Namespace *nsPtr;		/* Points to the namespace that contains this
				 * variable or NULL if the variable is a local
				 * variable in a TCL procedure. */
    Tcl_HashEntry *hPtr;	/* If variable is in a hashtable, either the
				 * hash table entry that refers to this
				 * variable or NULL if the variable has been
				 * detached from its hash table (e.g. an array
				 * is deleted, but some of its elements are
				 * still referred to in upvars). NULL if the
				 * variable is not in a hashtable. This is
				 * used to delete an variable from its
				 * hashtable if it is no longer needed. */
    int refCount;		/* Counts number of active uses of this
				 * variable, not including its entry in the
				 * call frame or the hash table: 1 for each
				 * additional variable whose linkPtr points
				 * here, 1 for each nested trace active on
				 * variable, and 1 if the variable is a
				 * namespace variable. This record can't be
				 * deleted until refCount becomes 0. */
    VarTrace *tracePtr;		/* First in list of all traces set for this
				 * variable. */
    ArraySearch *searchPtr;	/* First in list of all searches active for
				 * this variable, or NULL if none. */
    int flags;			/* Miscellaneous bits of information about
				 * variable. See below for definitions. */
} Var;

static INLINE Tcl_Namespace *
NamespaceOfVariable(Tcl_Var var)
{
    Var *varPtr = (Var *)var;

    return (Tcl_Namespace *)varPtr->nsPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewVar --
 *
 *	Create a new heap-allocated variable that will eventually be entered
 *	into a hashtable.
 *
 * Results:
 *	The return value is a pointer to the new variable structure. It is
 *	marked as a scalar variable (and not a link or array variable). Its
 *	value initially is NULL. The variable is not part of any hash table
 *	yet. Since it will be in a hashtable and not in a call frame, its name
 *	field is set NULL. It is initially marked as undefined.
 *
 * Side effects:
 *	Storage gets allocated.
 *
 *---------------------------------------------------------------------------
 */
static Var *
NewVar(const char *label, Tcl_Obj *objPtr)
{
    Var *varPtr;

    varPtr = Blt_AssertMalloc(sizeof(Var));
    varPtr->value.objPtr = objPtr;
    Tcl_IncrRefCount(objPtr);
    varPtr->name = (char *)label;
    varPtr->nsPtr = NULL;
    /*
     *  NOTE:  TCL reports a "dangling upvar" error for variables
     *         with a null "hPtr" field.  Put something non-zero
     *         in here to keep Tcl_SetVar2() happy.  The only time
     *         this field is really used is it remove a variable
     *         from the hash table that contains it in CleanupVar,
     *         but since these variables are protected by their
     *         higher refCount, they will not be deleted by CleanupVar
     *         anyway.  These variables are unset and removed in
     *         Blt_FreeCachedVars.
     */
    varPtr->hPtr = (Tcl_HashEntry *)0x01;
    varPtr->refCount = 1;  /* protect from being deleted */
    varPtr->tracePtr = NULL;
    varPtr->searchPtr = NULL;
    varPtr->flags = (VAR_SCALAR | VAR_IN_HASHTABLE);
    return varPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetVariableNamespace --
 *
 *	Returns the namespace context of the named variable.  The variable
 *	named may be fully qualified or not.  Variables in the current 
 *	namespace supersede the global namespace.
 *
 * Results:
 *	Returns the context of the namespace in an opaque type. If return
 *	value is NULL, this indicates that the variable is local to the call
 *	frame.
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
	Tcl_Var var;

	/* Search the current namespace and */
	var = Tcl_FindNamespaceVar(interp, (char *)path, NULL, 
		TCL_NAMESPACE_ONLY);
	if (var != NULL) {
	    return NamespaceOfVariable(var);
	}
	/* then search the global namespace. */
	var = Tcl_FindNamespaceVar(interp, (char *)path, NULL,
		TCL_GLOBAL_ONLY);
	if (var != NULL) {
	    return NamespaceOfVariable(var);
	}
    }
    return objName.nsPtr;    
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetCachedVar --
 *
 *	Returns an opaque TCL variable reference by *name* with the value
 *	pointed to by *objPtr*.  The purpose of this routine is to return a
 *	Tcl_Var for a NamespaceResolver callback and to set the value of that
 *	variable.  The variable is stored in a hash table associated with the
 *	(temporary) namespace.
 *
 * Results:
 *	Returns the opaque type of a Tcl_Var. 
 *
 * Side Effects:
 *	If the variable does not already exist in the hash table, it is
 *	created.  The value of the variable is set to *objPtr*.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Var
Blt_GetCachedVar(Blt_HashTable *tablePtr, const char *label, Tcl_Obj *objPtr)
{
    Blt_HashEntry *hPtr;
    int isNew;
    Var *varPtr;

    assert(objPtr != NULL);
    /* Check if the variable has been cached already. */
    hPtr = Blt_CreateHashEntry(tablePtr, label, &isNew);
    Tcl_IncrRefCount(objPtr);
    if (isNew) {
	varPtr = NewVar(label, objPtr);
	Blt_SetHashValue(hPtr, varPtr);
    } else {
	varPtr = Blt_GetHashValue(hPtr);
	Tcl_DecrRefCount(varPtr->value.objPtr);
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
	varPtr->refCount--;
	if (varPtr->refCount > 1) {
	    Tcl_DecrRefCount(varPtr->value.objPtr);
	    Blt_Free(varPtr);
	}
    }
    Blt_DeleteHashTable(tablePtr);
}

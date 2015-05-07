/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltNsUtil.c --
 *
 * This module implements utility namespace procedures for the BLT toolkit.
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

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltNsUtil.h"

/*
 * A Command structure exists for each command in a namespace. The
 * Tcl_Command opaque type actually refers to these structures.
 */

typedef struct _CompileProc CompileProc;
typedef struct _ImportRef ImportRef;
typedef struct _CommandTrace CommandTrace;

typedef struct {
    Tcl_HashEntry *hPtr;        /* Pointer to the hash table entry that refers
                                 * to this command. The hash table is either a
                                 * namespace's command table or an
                                 * interpreter's hidden command table. This
                                 * pointer is used to get a command's name
                                 * from its Tcl_Command handle. NULL means
                                 * that the hash table entry has been removed
                                 * already (this can happen if deleteProc
                                 * causes the command to be deleted or
                                 * recreated). */
    Tcl_Namespace *nsPtr;       /* Points to the namespace containing this
                                 * command. */
    int refCount;               /* 1 if in command hashtable plus 1 for each
                                 * reference from a CmdName TCL object
                                 * representing a command's name in a ByteCode
                                 * instruction sequence. This structure can be
                                 * freed when refCount becomes zero. */
    int cmdEpoch;               /* Incremented to invalidate any references
                                 * that point to this command when it is
                                 * renamed, deleted, hidden, or exposed. */
    CompileProc *compileProc;   /* Procedure called to compile command. NULL
                                 * if no compile proc exists for command. */
    Tcl_ObjCmdProc *objProc;    /* Object-based command procedure. */
    ClientData objClientData;   /* Arbitrary value passed to object proc. */
    Tcl_CmdProc *proc;          /* String-based command procedure. */
    ClientData clientData;      /* Arbitrary value passed to string proc. */
    Tcl_CmdDeleteProc *deleteProc;
                                /* Procedure invoked when deleting command
                                 * to, e.g., free all client data. */
    ClientData deleteData;      /* Arbitrary value passed to deleteProc. */
    int flags;                  /* Means that the command is in the process of
                                 * being deleted (its deleteProc is currently
                                 * executing). Other attempts to delete the
                                 * command should be ignored. */
    ImportRef *importRefPtr;    /* List of each imported Command created in
                                 * another namespace when this command is
                                 * imported. These imported commands redirect
                                 * invocations back to this command. The list
                                 * is used to remove all those imported
                                 * commands when deleting this "real"
                                 * command. */
    CommandTrace *tracePtr;     /* First in list of all traces set for this
                                 * command. */
} Command;

/*ARGSUSED*/
int
Blt_CommandExists(Tcl_Interp *interp, const char *string)
{
    Tcl_CmdInfo cmdInfo;

    return Tcl_GetCommandInfo(interp, string, &cmdInfo);
}

/*ARGSUSED*/
Tcl_Namespace *
Blt_GetCommandNamespace(Tcl_Command cmdToken)
{
    Command *cmdPtr = (Command *)cmdToken;

    return (Tcl_Namespace *)cmdPtr->nsPtr;
}

Tcl_CallFrame *
Blt_EnterNamespace(Tcl_Interp *interp, Tcl_Namespace *nsPtr)
{
    Tcl_CallFrame *framePtr;

    framePtr = Blt_AssertMalloc(sizeof(Tcl_CallFrame));
    if (Tcl_PushCallFrame(interp, framePtr, (Tcl_Namespace *)nsPtr, 0)
        != TCL_OK) {
        Blt_Free(framePtr);
        return NULL;
    }
    return framePtr;
}

void
Blt_LeaveNamespace(Tcl_Interp *interp, Tcl_CallFrame *framePtr)
{
    Tcl_PopCallFrame(interp);
    Blt_Free(framePtr);
}

int
Blt_ParseObjectName(Tcl_Interp *interp, const char *path, 
                    Blt_ObjectName *namePtr, unsigned int flags)
{
    char *last, *colon;

    namePtr->nsPtr = NULL;
    namePtr->name = NULL;
    colon = NULL;

    /* Find the last namespace separator in the qualified name. */
    last = (char *)(path + strlen(path));
    while (--last > path) {
        if ((*last == ':') && (*(last - 1) == ':')) {
            last++;             /* just after the last "::" */
            colon = last - 2;
            break;
        }
    }
    if (colon == NULL) {
        namePtr->name = path;
        if ((flags & BLT_NO_DEFAULT_NS) == 0) {
            namePtr->nsPtr = Tcl_GetCurrentNamespace(interp);
        }
        return TRUE;            /* No namespace designated in name. */
    }

    /* Separate the namespace and the object name. */
    *colon = '\0';
    if (path[0] == '\0') {
        namePtr->nsPtr = Tcl_GetGlobalNamespace(interp);
    } else {
        namePtr->nsPtr = Tcl_FindNamespace(interp, (char *)path, NULL, 
                (flags & BLT_NO_ERROR_MSG) ? 0 : TCL_LEAVE_ERR_MSG);
    }
    /* Repair the string. */    *colon = ':';

    if (namePtr->nsPtr == NULL) {
        return FALSE;           /* Namespace doesn't exist. */
    }
    namePtr->name =last;
    return TRUE;
}

const char *
Blt_MakeQualifiedName(Blt_ObjectName *namePtr, Tcl_DString *resultPtr)
{
    Tcl_DStringInit(resultPtr);
    if ((namePtr->nsPtr->fullName[0] != ':') || 
        (namePtr->nsPtr->fullName[1] != ':') ||
        (namePtr->nsPtr->fullName[2] != '\0')) {
        Tcl_DStringAppend(resultPtr, namePtr->nsPtr->fullName, -1);
    }
    Tcl_DStringAppend(resultPtr, "::", -1);
    Tcl_DStringAppend(resultPtr, (char *)namePtr->name, -1);
    return Tcl_DStringValue(resultPtr);
}


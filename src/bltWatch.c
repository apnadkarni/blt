/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltWatch.c --
 *
 * This module implements watch procedure callbacks for TCL commands
 * and procedures.
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
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include <bltHash.h>
#include "bltSwitch.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define UNKNOWN_RETURN_CODE     5
static const char *codeNames[] =
{
    "OK", "ERROR", "RETURN", "BREAK", "CONTINUE"
};

#define WATCH_THREAD_KEY "BLT Watch Command Data"
#define WATCH_MAX_LEVEL 10000   /* Maximum depth of TCL traces. */

enum WatchStates {
    WATCH_STATE_DONT_CARE = -1, /* Select watch regardless of state */
    WATCH_STATE_IDLE = 0,       /*  */
    WATCH_STATE_ACTIVE = 1
};

typedef struct {
    Tcl_Interp *interp;         /* Interpreter associated with the watch */
    const char *name;           /* Watch identifier */

    /* User-configurable fields */
    enum WatchStates state;     /* Current state of watch: either
                                 * WATCH_STATE_IDLE or WATCH_STATE_ACTIVE */
    int maxLevel;               /* Maximum depth of tracing allowed */
    Tcl_Obj *preCmdObjPtr;              /* Procedure to be invoked before
                                         * the command is executed (but
                                         * after substitutions have
                                         * occurred). */
    Tcl_Obj *postCmdObjPtr;             /* Procedure to be invoked after
                                         * the command is executed. */
    Tcl_Trace token;                    /* Trace handler which activates
                                         * "pre" command procedures */
    Tcl_AsyncHandler asyncHandle;       /* Async handler which triggers the
                                         * "post" command procedure (if one
                                         * exists) */
    int active;                         /* Indicates if a trace is
                                         * currently active.  This prevents
                                         * recursive tracing of the "pre"
                                         * and "post" procedures. */
    int level;                      /* Current level of traced command. */
    const char *cmdPtr;             /* Command string before substitutions.
                                     * Points to a original command buffer. */
    Tcl_Obj *argsObjPtr;            /* TCL listobj of the command after
                                         * substitutions. List is malloc-ed
                                         * by Tcl_Merge. Must be freed in
                                         * handler procs */
    Blt_HashEntry *hashPtr;
} Watch;

typedef struct {
    Blt_HashTable watchTable;   /* Hash table of watches keyed by address. */
    Tcl_Interp *interp;
} WatchCmdInterpData;

static Blt_SwitchSpec switchSpecs[] = 
{
    {BLT_SWITCH_OBJ, "-precmd", "cmdPrefix", (char *)NULL,
        Blt_Offset(Watch, preCmdObjPtr), 0}, 
    {BLT_SWITCH_OBJ, "-postcmd", "cmdPrefix", (char *)NULL,
        Blt_Offset(Watch, postCmdObjPtr), 0},
    {BLT_SWITCH_BOOLEAN, "-active", "bool", (char *)NULL,
        Blt_Offset(Watch, state), 0},
    {BLT_SWITCH_INT_NNEG, "-maxlevel", "numLevels", (char *)NULL,
        Blt_Offset(Watch, maxLevel), 0},
    {BLT_SWITCH_END}
};

static Tcl_CmdObjTraceProc PreCmdObjProc;
static Tcl_AsyncProc PostCmdProc;
static Tcl_ObjCmdProc WatchCmd;

/*
 *---------------------------------------------------------------------------
 *
 * WatchInterpDeleteProc --
 *
 *      This is called when the interpreter hosting the "watch" command
 *      is deleted.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Removes the hash table managing all watch names.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
WatchInterpDeleteProc(
    ClientData clientData,      /* Interpreter-specific data. */
    Tcl_Interp *interp)
{
    WatchCmdInterpData *dataPtr = clientData;

    /* All watch instances should already have been destroyed when
     * their respective TCL commands were deleted. */
    Blt_DeleteHashTable(&dataPtr->watchTable);
    Tcl_DeleteAssocData(interp, WATCH_THREAD_KEY);
    Blt_Free(dataPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetWatchCmdInterpData --
 *
 *---------------------------------------------------------------------------
 */
static WatchCmdInterpData *
GetWatchCmdInterpData(Tcl_Interp *interp)
{
    WatchCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (WatchCmdInterpData *)
        Tcl_GetAssocData(interp, WATCH_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
        dataPtr = Blt_AssertMalloc(sizeof(WatchCmdInterpData));
        dataPtr->interp = interp;
        Tcl_SetAssocData(interp, WATCH_THREAD_KEY, WatchInterpDeleteProc,
                 dataPtr);
        Blt_InitHashTable(&dataPtr->watchTable, BLT_ONE_WORD_KEYS);
    }
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * PreCmdObjProc --
 *
 *      Procedure callback for Tcl_Trace. Gets called before the
 *      command is executed, but after substitutions have occurred.
 *      If a watch procedure is active, it evals a TCL command.
 *      Activates the "precmd" callback, if one exists.
 *
 *      Stashes some information for the "pre" callback: command
 *      string, substituted argument list, and current level.
 *
 *      Format of "pre" proc:
 *
 *      proc beforeCmd { level cmdStr argList } {
 *
 *      }
 *
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      A Tcl_AsyncHandler may be triggered, if a "post" procedure is
 *      defined.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PreCmdObjProc(ClientData clientData, Tcl_Interp *interp, int level,
              const char *command, Tcl_Command token, int objc,
              Tcl_Obj *const *objv)
{
    Watch *watchPtr = clientData;

    if (watchPtr->active) {
        return TCL_OK;             /* Don't re-enter from Tcl_Eval below */
    }
    watchPtr->cmdPtr = command;
    watchPtr->level = level;

    /*
     * There's no guarantee that the calls to PreCmdObjProc will match
     * up with PostCmdObjProc.  So free up argument lists that are still
     * hanging around before allocating a new one.
     */
    if (watchPtr->argsObjPtr != NULL) {
        Tcl_DecrRefCount(watchPtr->argsObjPtr);
    }
    watchPtr->argsObjPtr = Tcl_NewListObj(objc, objv);
    if (watchPtr->preCmdObjPtr != NULL) {
        int status;
        Tcl_Obj *objPtr, *cmdObjPtr;

        /* pre command */
        cmdObjPtr = Tcl_DuplicateObj(watchPtr->preCmdObjPtr);
        /* level */
        objPtr = Tcl_NewIntObj(watchPtr->level);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        /* command */
        objPtr = Tcl_NewStringObj(watchPtr->cmdPtr, -1);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        /* args */
        Tcl_ListObjAppendElement(interp, cmdObjPtr, watchPtr->argsObjPtr);
        Tcl_IncrRefCount(cmdObjPtr);
        Tcl_Preserve(watchPtr);
        watchPtr->active = 1;
        status = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        watchPtr->active = 0;
        Tcl_Release(watchPtr);
        Tcl_DecrRefCount(cmdObjPtr);
        if (status != TCL_OK) {
            Blt_Warn("%s failed: %s\n", Tcl_GetString(watchPtr->preCmdObjPtr), 
                     Tcl_GetStringResult(interp));
        }
    }
    /* Set the trigger for the "post" command procedure */
    if (watchPtr->postCmdObjPtr != NULL) {
        Tcl_AsyncMark(watchPtr->asyncHandle);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PostCmdProc --
 *
 *      Procedure callback for Tcl_AsyncHandler. Gets called after
 *      the command has executed.  It tests for a "post" command, but
 *      you really can't get here, if one doesn't exist.
 *
 *      Save the current contents of interp->result before calling
 *      the "post" command, and restore it afterwards.
 *
 *      Format of "post" proc:
 *
 *      proc afterCmd { level cmdStr argList retCode results } {
 *
 *      }
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Memory for argument list is released.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PostCmdProc(ClientData clientData, Tcl_Interp *interp, int code)
{
    Tcl_Obj *errorInfoObjPtr, *errorCodeObjPtr;
    Tcl_Obj *objPtr, *cmdObjPtr;
    Tcl_Obj *resultsObjPtr;
    Watch *watchPtr = clientData;
    int status;
    
    if (interp == NULL) {
        return code;                    /* No interpreter available. */
    }
    if (watchPtr->postCmdObjPtr == NULL) {
        return code;
    }
    if (watchPtr->active) {
        return code;
    }
    errorInfoObjPtr = errorCodeObjPtr = NULL;

    /*
     * Save the state of the interpreter.
     */
    errorInfoObjPtr = Tcl_GetVar2Ex(interp, "errorInfo", (char *)NULL,
                                    TCL_GLOBAL_ONLY);
    errorCodeObjPtr = Tcl_GetVar2Ex(interp, "errorCode", (char *)NULL,
                                    TCL_GLOBAL_ONLY);
    resultsObjPtr = Tcl_GetObjResult(interp);

    /* pre command */
    cmdObjPtr = Tcl_DuplicateObj(watchPtr->postCmdObjPtr);
    /* level */
    objPtr = Tcl_NewIntObj(watchPtr->level);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    /* command */
    objPtr = Tcl_NewStringObj(watchPtr->cmdPtr, -1);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    /* args */
    Tcl_ListObjAppendElement(interp, cmdObjPtr, watchPtr->argsObjPtr);
    /* return code */
    if (code < UNKNOWN_RETURN_CODE) {
        objPtr = Tcl_NewStringObj(codeNames[code], -1);
    } else {
        objPtr = Tcl_NewIntObj(code);
    }
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, resultsObjPtr);
    
    Tcl_IncrRefCount(cmdObjPtr);
    Tcl_Preserve(watchPtr);
    watchPtr->active = 1;
    status = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    watchPtr->active = 0;
    Tcl_Release(watchPtr);
    Tcl_DecrRefCount(cmdObjPtr);
    Tcl_DecrRefCount(watchPtr->argsObjPtr);
    watchPtr->argsObjPtr = NULL;
    if (status != TCL_OK) {
        Blt_Warn("%s failed: %s\n", Tcl_GetString(watchPtr->postCmdObjPtr), 
                 Tcl_GetStringResult(interp));
    }
    /*
     * Restore the state of the interpreter.
     */
    if (errorInfoObjPtr != NULL) {
        Tcl_SetVar2Ex(interp, "errorInfo", (char *)NULL, 
                      errorInfoObjPtr, TCL_GLOBAL_ONLY);
    }
    if (errorCodeObjPtr != NULL) {
        Tcl_SetVar2Ex(interp, "errorCode", (char *)NULL, 
                      errorCodeObjPtr, TCL_GLOBAL_ONLY);
    }
    Tcl_SetObjResult(interp, resultsObjPtr);
    return code;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewWatch --
 *
 *      Creates a new watch. The new watch is registered into the
 *      "watchTable" hash table. Also creates a Tcl_AsyncHandler for
 *      triggering "post" events.
 *
 * Results:
 *      If memory for the watch could be allocated, a pointer to
 *      the new watch is returned.  Otherwise NULL, and interp->result
 *      points to an error message.
 *
 * Side Effects:
 *      A new Tcl_AsyncHandler is created. A new hash table entry
 *      is created. Memory the watch structure is allocated.
 *
 *---------------------------------------------------------------------------
 */
static Watch *
NewWatch(Tcl_Interp *interp, WatchCmdInterpData *dataPtr, Blt_HashEntry *hPtr)
{
    Watch *watchPtr;

    watchPtr = Blt_Calloc(1, sizeof(Watch));
    if (watchPtr == NULL) {
        Tcl_AppendResult(interp, "can't allocate watch structure", (char *)NULL);
        return NULL;
    }
    watchPtr->state = WATCH_STATE_ACTIVE;
    watchPtr->maxLevel = WATCH_MAX_LEVEL;
    watchPtr->name = Blt_GetHashKey(&dataPtr->watchTable, hPtr);
    watchPtr->interp = interp;
    watchPtr->asyncHandle = Tcl_AsyncCreate(PostCmdProc, watchPtr);
    watchPtr->hashPtr = hPtr;
    Blt_SetHashValue(hPtr, watchPtr);
    return watchPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyWatch --
 *
 *      Removes the watch. The resources used by the watch
 *      are released.
 *        1) If the watch is active, its trace is deleted.
 *        2) Memory for command strings is free-ed.
 *        3) Entry is removed from watch registry.
 *        4) Async handler is deleted.
 *        5) Memory for watch itself is released.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Everything associated with the watch is freed.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyWatch(WatchCmdInterpData *dataPtr, Watch *watchPtr)
{
    Tcl_AsyncDelete(watchPtr->asyncHandle);
    if (watchPtr->state == WATCH_STATE_ACTIVE) {
        Tcl_DeleteTrace(watchPtr->interp, watchPtr->token);
    }
    if (watchPtr->preCmdObjPtr != NULL) {
        Tcl_DecrRefCount(watchPtr->preCmdObjPtr);
    }
    if (watchPtr->postCmdObjPtr != NULL) {
        Tcl_DecrRefCount(watchPtr->postCmdObjPtr);
    }
    if (watchPtr->argsObjPtr!= NULL) {
        Tcl_DecrRefCount(watchPtr->argsObjPtr);
    }
    if (watchPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&dataPtr->watchTable, watchPtr->hashPtr);
    }
    Blt_Free(watchPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetWatchFromObj --
 *
 *      Searches for the watch represented by the watch name and its
 *      associated interpreter in its directory.
 *
 * Results:
 *      If found, the pointer to the watch structure is returned,
 *      otherwise NULL. If requested, interp-result will be filled
 *      with an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
GetWatchFromObj(
    WatchCmdInterpData *dataPtr,
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Watch **watchPtrPtr)
{
    Blt_HashEntry *hPtr;
    char *string;

    string = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&dataPtr->watchTable, (char *)string);
    if (hPtr == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find any watch named \"", 
                             string, "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    *watchPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListWatches --
 *
 *      Creates a list of all watches in the interpreter.  The
 *      list search may be restricted to selected states by
 *      setting "state" to something other than WATCH_STATE_DONT_CARE.
 *
 * Results:
 *      A standard TCL result.  Interp->result will contain a list
 *      of all watches matches the state criteria.
 *
 *---------------------------------------------------------------------------
 */
static int
ListWatches(WatchCmdInterpData *dataPtr, Tcl_Interp *interp, 
            enum WatchStates state)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Blt_FirstHashEntry(&dataPtr->watchTable, &cursor);
        hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        Watch *watchPtr;

        watchPtr = Blt_GetHashValue(hPtr);
        if ((state == WATCH_STATE_DONT_CARE) ||
            (state == watchPtr->state)) {
            Tcl_ListObjAppendElement(interp, listObjPtr, 
                             Tcl_NewStringObj(watchPtr->name, -1));
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}
/*
 *---------------------------------------------------------------------------
 *
 * ConfigWatch --
 *
 *      Processes argument list of switches and values, setting
 *      Watch fields.
 *
 * Results:
 *      If found, the pointer to the watch structure is returned,
 *      otherwise NULL. If requested, interp-result will be filled
 *      with an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigWatch(Watch *watchPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    if (Blt_ParseSwitches(interp, switchSpecs, objc, objv, watchPtr, 
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    /*
     * If the watch's max depth changed or its state, reset the traces.
     */
    if (watchPtr->token != (Tcl_Trace)0) {
        Tcl_DeleteTrace(interp, watchPtr->token);
        watchPtr->token = (Tcl_Trace)0;
    }
    if (watchPtr->state == WATCH_STATE_ACTIVE) {
        watchPtr->token = Tcl_CreateObjTrace(interp, watchPtr->maxLevel, 0,
            PreCmdObjProc, watchPtr, NULL);
    }
    return TCL_OK;
}

/* TCL interface routines */
/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *      Creates a new watch and processes any extra switches.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      A new watch is created.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    WatchCmdInterpData *dataPtr = clientData;
    Watch *watchPtr;
    int isNew;
    char *string;
    Blt_HashEntry *hPtr;

    string = Tcl_GetString(objv[2]);
    hPtr = Blt_CreateHashEntry(&dataPtr->watchTable, string, &isNew);
    if (!isNew) {
        Tcl_AppendResult(interp, "a watch \"", string, "\" already exists", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    watchPtr = NewWatch(interp, dataPtr, hPtr);
    if (watchPtr == NULL) {
        return TCL_ERROR;       /* Can't create new watch */
    }
    return ConfigWatch(watchPtr, interp, objc - 3, objv + 3);
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes the watch.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Watch *watchPtr;
    WatchCmdInterpData *dataPtr = clientData;

    if (GetWatchFromObj(dataPtr, interp, objv[2], &watchPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    DestroyWatch(dataPtr, watchPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 *      Activate/deactivates the named watch.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    WatchCmdInterpData *dataPtr = clientData;
    Watch *watchPtr;
    enum WatchStates state;
    char *string;

    if (GetWatchFromObj(dataPtr, interp, objv[2], &watchPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[1]);
    state = (string[0] == 'a') ? WATCH_STATE_ACTIVE : WATCH_STATE_IDLE;
    if (state != watchPtr->state) {
        if (watchPtr->token == (Tcl_Trace)0) {
            watchPtr->token = Tcl_CreateObjTrace(interp, watchPtr->maxLevel, 0,
                PreCmdObjProc, watchPtr, NULL);
        } else {
            Tcl_DeleteTrace(interp, watchPtr->token);
            watchPtr->token = (Tcl_Trace)0;
        }
        watchPtr->state = state;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *      Returns the names of all watches in the interpreter.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    WatchCmdInterpData *dataPtr = clientData;
    enum WatchStates state;

    state = WATCH_STATE_DONT_CARE;
    if (objc == 3) {
        char c;
        char *string;

        string = Tcl_GetString(objv[2]);
        c = string[0];
        if ((c == 'a') && (strcmp(string, "active") == 0)) {
            state = WATCH_STATE_ACTIVE;
        } else if ((c == 'i') && (strcmp(string, "idle") == 0)) {
            state = WATCH_STATE_IDLE;
        } else if ((c == 'i') && (strcmp(string, "ignore") == 0)) {
            state = WATCH_STATE_DONT_CARE;
        } else {
            Tcl_AppendResult(interp, "bad state \"", string, "\" should be \
\"active\", \"idle\", or \"ignore\"", (char *)NULL);
            return TCL_ERROR;
        }
    }
    return ListWatches(dataPtr, interp, state);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      Convert the range of the pixel values allowed into a list.
 *
 * Results:
 *      The string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    WatchCmdInterpData *dataPtr = clientData;
    Watch *watchPtr;

    if (GetWatchFromObj(dataPtr, interp, objv[2], &watchPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return ConfigWatch(watchPtr, interp, objc - 3, objv + 3);
}

/*
 *---------------------------------------------------------------------------
 *
 * InfoOp --
 *
 *      Convert the limits of the pixel values allowed into a list.
 *
 * Results:
 *      The string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InfoOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    WatchCmdInterpData *dataPtr = clientData;
    Watch *watchPtr;
    Tcl_Obj *objPtr, *listObjPtr;

    if (GetWatchFromObj(dataPtr, interp, objv[2], &watchPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    if (watchPtr->preCmdObjPtr != NULL) {
        /* -precmd */
        objPtr = Tcl_NewStringObj("-precmd", 7);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        Tcl_ListObjAppendElement(interp, listObjPtr, watchPtr->preCmdObjPtr);
    }
    if (watchPtr->postCmdObjPtr != NULL) {
        /* -postcmd */
        objPtr = Tcl_NewStringObj("-postcmd", 8);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        Tcl_ListObjAppendElement(interp, listObjPtr, watchPtr->postCmdObjPtr);
    }
    /* -maxlevel */
    objPtr = Tcl_NewStringObj("-maxlevel", 9);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(watchPtr->maxLevel);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* -active */
    objPtr = Tcl_NewStringObj("-active", 7);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewBooleanObj(watchPtr->state == WATCH_STATE_ACTIVE);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * WatchCmd --
 *
 *      This procedure is invoked to process the TCL "blt_watch"
 *      command. See the user documentation for details on what
 *      it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

static Blt_OpSpec watchOps[] =
{
    {"activate",   1, ActivateOp, 3, 3, "watchName",},
    {"configure",  2, ConfigureOp, 3, 0, "watchName ?options...?"},
    {"create",     2, CreateOp, 3, 0, "watchName ?switches?",},
    {"deactivate", 3, ActivateOp, 3, 3, "watchName",},
    {"delete",     3, DeleteOp, 3, 3, "watchName",},
    {"info",       1, InfoOp, 3, 3, "watchName",},
    {"names",      1, NamesOp, 2, 3, "?state?",},
};
static int numWatchOps = sizeof(watchOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
WatchCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numWatchOps, watchOps, BLT_OP_ARG1, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

/* Public initialization routine */
/*
 *---------------------------------------------------------------------------
 *
 * Blt_WatchCmdInitProc --
 *
 *      This procedure is invoked to initialize the TCL command
 *      "blt_watch".
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Creates the new command and adds a new entry into a
 *      global  TCL associative array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_WatchCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"watch", WatchCmd, NULL};

    cmdSpec.clientData = GetWatchCmdInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

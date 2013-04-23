
/*
 *
 * bltGrab.c --
 *
 *	Implements a grab stack for the BLT toolkit.
 *
 *	Copyright 2012 George A Howlett.
 *
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
 */

/* 
 * grab 
 * 
 * Start with Tk grab command operations.
 *    grab ?-global? window
 *    grab current ?window?
 *    grab release window
 *    grab set ?-global? window
 *    grab status window
 *
 * Addition operations.
 *    grab push window ?-global?
 *    grab pop ?window? 
 *    grab list 
 *    grab empty 
 */

#define BUILD_BLT_TK_PROCS 1
#include <bltInt.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltSwitch.h"
#include "bltOp.h"
#include "bltInitCmd.h"
#include "tkIntDecls.h"
#include "tkDisplay.h"

#define GRAB_THREAD_KEY "BLT Grab Command Data"

#define GRAB_GLOBAL	(1)
#define GRAB_LOCAL	(0)

typedef struct {
    Tcl_Interp *interp;			/* Intepreter  */
    Blt_Chain chain;			/* Stack of grab instances. */
    Tk_Window tkwin;			/* Main window associated with
					 * interpreter. */
    Blt_HashTable entryTable;		/* Table of unique grab entries */
    int debug;
} GrabCmdInterpData;

/* 
 * GrabEntry --
 *
 *	A GrabEntry represents a window that has been grabbed.  There can be
 *	several instances of grabs, but there will be only one grab entry per
 *	window.  The number of grabs for a window is tracked by a reference
 *	count.
 */
typedef struct {
    Tk_Window tkwin;			/* Window that has been grabbed. */
    GrabCmdInterpData *dataPtr;
    int refCount;			/* How many times this entry has been
					 * grabbed (i.e. on the stack). */
    Blt_HashEntry *hashPtr;
} GrabEntry;

/* 
 * Grab --
 *
 *	A Grab represents an instance of a grab on a window.  There can be
 *	several instances of grabs on the same or different windows. Each
 *	instance contains a reference to window and the flags used to set
 *	the grab (local or global).
 */
typedef struct {
    GrabEntry *entryPtr;
    int flags;				/* Indicates if the the grab was
					 * global or local */
    Blt_ChainLink link;
} Grab;

typedef struct {
    int global;
} PushSwitches;

static Blt_SwitchSpec pushSwitches[] = 
{
    {BLT_SWITCH_VALUE, "-global", "", (char *)NULL,
	Blt_Offset(PushSwitches, global), 0, GRAB_GLOBAL},
    {BLT_SWITCH_VALUE, "-local", "", (char *)NULL,
	Blt_Offset(PushSwitches, global), 0, GRAB_LOCAL},
    {BLT_SWITCH_END}
};

static Tk_EventProc GrabEntryEventProc;

/*
 *---------------------------------------------------------------------------
 *
 * FreeGrabEntry --
 *
 * 	Releases a grab entry for the given window.  When the reference count
 * 	is zero, this means that this entry is no longer on the stack and may
 * 	be safely deleted.  The event handler on the entry's window is 
 * 	deleted and the entry is removed from the hash table of grab entries.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The grab entry is possibly destroyed.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeGrabEntry(GrabEntry *entryPtr)
{
    entryPtr->refCount--;
    if (entryPtr->refCount <= 0) {
	if (entryPtr->tkwin != NULL) {
	    Tk_DeleteEventHandler(entryPtr->tkwin, StructureNotifyMask, 
				  GrabEntryEventProc, entryPtr);
	} 
	if (entryPtr->hashPtr != NULL) {
	    GrabCmdInterpData *dataPtr;
	    
	    dataPtr = entryPtr->dataPtr;
	    Blt_DeleteHashEntry(&dataPtr->entryTable, entryPtr->hashPtr);
	}
	Blt_Free(entryPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetGrabEntry --
 *
 * 	Returns a grab entry for the given window.  If this is the first
 *	time the window has been grabbed, the structure is created and
 *	a event handler is created to watch the window.  We keep track
 *	of how many times this entry is on the instance stack. 
 *
 * Results:
 *	Returns a pointer to the grab entry.
 *
 * Side effects:
 *	A new grab entry is possibly created.
 *
 *---------------------------------------------------------------------------
 */
static GrabEntry *
GetGrabEntry(GrabCmdInterpData *dataPtr, Tk_Window tkwin)
{
    GrabEntry *entryPtr;
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&dataPtr->entryTable, (char *)tkwin, &isNew);
    if (isNew) {
	entryPtr = Blt_AssertCalloc(1, sizeof(GrabEntry));
	entryPtr->tkwin = tkwin;
	entryPtr->dataPtr = dataPtr;
	entryPtr->refCount = 1;
	entryPtr->hashPtr = hPtr;
	Blt_SetHashValue(hPtr, entryPtr);
	Tk_CreateEventHandler(tkwin, StructureNotifyMask, GrabEntryEventProc, 
		entryPtr);
    } else {
	entryPtr = Blt_GetHashValue(hPtr);
	entryPtr->refCount++;
    }
    return entryPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * PopGrab --
 *
 * 	Destroys the given instance of a grab.  The designated window is 
 *	ungrabbed and the grab instance information is popped off the stack.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Grab instance (and possibly entry) is freed.
 *
 *---------------------------------------------------------------------------
 */
static void
PopGrab(GrabCmdInterpData *dataPtr, Grab *grabPtr)
{
    if (grabPtr->link != NULL) {
	Blt_Chain_DeleteLink(dataPtr->chain, grabPtr->link);
    }
    if (grabPtr->entryPtr != NULL) {
	if (grabPtr->entryPtr->tkwin != NULL) {
	    Tk_Ungrab(grabPtr->entryPtr->tkwin);
	}
	FreeGrabEntry(grabPtr->entryPtr);
    }
    Blt_Free(grabPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * PushGrab --
 *
 * 	Creates a new instance of a grab.  The designated window is grabbed
 * 	and the grab instance information is pushed on the stack.
 *
 * Results:
 *	A standard TCL result.  If an error occurred attempting to grab the
 *	window, TCL_ERROR is returned and an error message is the
 *	interpreter's result.
 *
 * Side effects:
 *	New grab instance (and possibly entry) gets added.
 *
 *---------------------------------------------------------------------------
 */
static int
PushGrab(GrabCmdInterpData *dataPtr, Tk_Window tkwin, unsigned int flags)
{
    Grab *grabPtr;

    if (Tk_Grab(dataPtr->interp, tkwin, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    grabPtr = Blt_AssertCalloc(1, sizeof(Grab));
    grabPtr->flags = flags;
    grabPtr->entryPtr = GetGrabEntry(dataPtr, tkwin);
    grabPtr->link = Blt_Chain_Prepend(dataPtr->chain, grabPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTopGrab --
 *
 * 	Returns the grab instance at the top of the stack.
 *
 * Results:
 * 	Returns a pointer to the grab instance.  If the stack
 *	is empty, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Grab *
GetTopGrab(GrabCmdInterpData *dataPtr)
{
    Blt_ChainLink link;

    link = Blt_Chain_FirstLink(dataPtr->chain);
    if (link == NULL) {
	return NULL;
    }
    return Blt_Chain_GetValue(link);
}

/*
 *---------------------------------------------------------------------------
 *
 * DumpStack --
 *
 * 	This routine is called when we find that the grab has been released
 *	by the Tk "grab" command.  We assume that the current stack is 
 *	useless and remove all entries.  This routine is also called when
 *	the interpreter associated with Blt "grab" command has been deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Internal structures get cleaned up.  
 *
 *---------------------------------------------------------------------------
 */
static void
DumpStack(GrabCmdInterpData *dataPtr)
{
    Blt_ChainLink link, next;

    for (link = Blt_Chain_FirstLink(dataPtr->chain); link != NULL;
	 link = next) {
	Grab *grabPtr;

	next = Blt_Chain_NextLink(link);
	grabPtr = Blt_Chain_GetValue(link);
	PopGrab(dataPtr, grabPtr);
    }
    Blt_Chain_Reset(dataPtr->chain);
}

/*
 *---------------------------------------------------------------------------
 *
 * PruneDeadInstances --
 *
 * 	When a window is destroyed, this routine removes all grab instances
 * 	from the stack.  The grab entry itself also removed (this is done by
 * 	decrementing the reference count on the entry).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Internal structures get cleaned up.  
 *
 *---------------------------------------------------------------------------
 */
static void
PruneDeadInstances(GrabEntry *entryPtr)
{
    GrabCmdInterpData *dataPtr;
    Blt_ChainLink link, next;

    dataPtr = entryPtr->dataPtr;
    for (link = Blt_Chain_FirstLink(dataPtr->chain); link != NULL;
	 link = next) {
	Grab *grabPtr;

	next = Blt_Chain_NextLink(link);
	grabPtr = Blt_Chain_GetValue(link);
	if (grabPtr->entryPtr != entryPtr) {
	    continue;
	}
	grabPtr->entryPtr->tkwin = NULL; /* Indicate the window has been
					  * destroyed so FreeGrabEntry
					  * doesn't try to do something with
					  * it. */
	FreeGrabEntry(entryPtr);
	if (grabPtr->link != NULL) {
	    /* Remove the grab from the stack. */
	    Blt_Chain_DeleteLink(dataPtr->chain, grabPtr->link);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GrabCmdInterpDeleteProc --
 *
 *	This is called when the interpreter hosting the "grab" command
 *	is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the chain managing stack of grabbed windows.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
GrabCmdInterpDeleteProc(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp)
{
    Blt_ChainLink link, next;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    GrabCmdInterpData *dataPtr = clientData;

    /* Destroy the grab stack and corrsponding entries. */
    for (hPtr = Blt_FirstHashEntry(&dataPtr->entryTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	GrabEntry *entryPtr;

	entryPtr = Blt_GetHashValue(hPtr);
	Tk_DeleteEventHandler(entryPtr->tkwin, StructureNotifyMask, 
		GrabEntryEventProc, entryPtr);
	Blt_Free(entryPtr);
    }
    for (link = Blt_Chain_FirstLink(dataPtr->chain); link != NULL;
	 link = next) {
	Grab *grabPtr;

	next = Blt_Chain_NextLink(link);
	grabPtr = Blt_Chain_GetValue(link);
	Blt_Free(grabPtr);
    }
    Blt_Chain_Destroy(dataPtr->chain);
    Blt_DeleteHashTable(&dataPtr->entryTable);
    Tcl_DeleteAssocData(interp, GRAB_THREAD_KEY);
    Blt_Free(dataPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GrabEntryEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various
 * 	events on the grabbed windows.  If the window has been destroyed,
 *	remove all grab instances of the window and the grab entry for the
 *	window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  
 *
 *---------------------------------------------------------------------------
 */
static void
GrabEntryEventProc(ClientData clientData, XEvent *eventPtr)
{
    GrabEntry *entryPtr = (GrabEntry *)clientData;

    if (eventPtr->type == DestroyNotify) {
	PruneDeadInstances(entryPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetGrabCmdInterpData --
 *
 *---------------------------------------------------------------------------
 */
static GrabCmdInterpData *
GetGrabCmdInterpData(Tcl_Interp *interp)
{
    GrabCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (GrabCmdInterpData *)
	Tcl_GetAssocData(interp, GRAB_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
	dataPtr = Blt_AssertMalloc(sizeof(GrabCmdInterpData));
	dataPtr->interp = interp;
	Tcl_SetAssocData(interp, GRAB_THREAD_KEY, GrabCmdInterpDeleteProc, 
		dataPtr);
	Blt_InitHashTable(&dataPtr->entryTable, BLT_ONE_WORD_KEYS);
	dataPtr->chain = Blt_Chain_Create();
	dataPtr->tkwin = Tk_MainWindow(interp);
	dataPtr->debug = 0;
    }
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FixCurrent --
 *
 * 	This procedure is called whenever the grab command is invoked.
 * 	It verifies that the current grab on the top of the stack is still 
 *	the grab Tk is using (the grab could have been changed by the Tk 
 *	"grab" command).  If there is no current grab (grab release was
 *	called), then assume the stack is useless and dump the stack. 
 *
 *	If a new grab is current, different from the one on the top of the
 *	stack, replace the top grab when the current.
 *
 *	The most likely scenario is that there is already a current grab (using
 *	the Tk "grab" command) and new we want to push a new grab onto the
 *	stack where the stack is empty.  The current grab is first put on the
 *	stack.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
FixCurrent(Tcl_Interp *interp, GrabCmdInterpData *dataPtr)
{
    TkDisplay *dispPtr;
    TkWindow *winPtr;
    Tk_Window tkwin;

    winPtr = (TkWindow *)dataPtr->tkwin;
    dispPtr = winPtr->dispPtr;
    tkwin = (Tk_Window)dispPtr->eventualGrabWinPtr;

    if (tkwin == NULL) {
	Grab *grabPtr;
	
	/* There's a grab currently made. Check that it's our window.  */
	grabPtr = GetTopGrab(dataPtr);
	/* No currently grabbed window. Verify that the stack is empty.
	 * Otherwise, dump it. */
	if (grabPtr != NULL) {
	    if (grabPtr->entryPtr == NULL) {
		Tcl_AppendResult(interp, "no current grab: dumping grab stack",
				 (char *)NULL);
	    } else {
		Tcl_AppendResult(interp, 
				 "no current grab: dumping grab stack: top=\"",
				 Tk_PathName(grabPtr->entryPtr->tkwin),
				 "\"", (char *)NULL);
		DumpStack(dataPtr);
		return TCL_ERROR;
	    }
	    return TCL_OK;
	}
    } else {
	Grab *grabPtr;
	
	/* There's a grab currently made. Check that it's our window.  */
	grabPtr = GetTopGrab(dataPtr);
	if ((grabPtr != NULL) && (grabPtr->entryPtr->tkwin != tkwin)) {
	    /* Reset the topmost grab into the stack if it isn't ours. */ 
	    Blt_Warn("current grab %s is not the topmost on grab stack %s\n",
		     Tk_PathName(tkwin), Tk_PathName(grabPtr->entryPtr->tkwin));
	    PopGrab(dataPtr, grabPtr);
	    PushGrab(dataPtr, tkwin, (dispPtr->grabFlags & GRAB_GLOBAL));
	}
    }
    return TCL_OK;
}

static int
CurrentOp(
    ClientData clientData,		/* Global data associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    GrabCmdInterpData *dataPtr = clientData;
    Grab *grabPtr;

    grabPtr = GetTopGrab(dataPtr);
    if (grabPtr == NULL) {
	return TCL_OK;			/* Stack is empty. */
    }
    if (objc == 3) {
	TkDisplay *dispPtr;
	TkWindow *winPtr;
	Tk_Window tkwin;
	const char *pathName;

	pathName = Tcl_GetString(objv[2]);
	tkwin = Tk_NameToWindow(interp, pathName, dataPtr->tkwin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	winPtr = (TkWindow *)tkwin;
	dispPtr = winPtr->dispPtr;
	tkwin = (Tk_Window)dispPtr->eventualGrabWinPtr;
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
			 Tk_PathName(grabPtr->entryPtr->tkwin), -1);

    } else if (objc == 2) {
	TkDisplay *dispPtr;

	for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
	     dispPtr = dispPtr->nextPtr) {
	    Tk_Window tkwin;

	    tkwin = (Tk_Window)dispPtr->eventualGrabWinPtr;
	    if (tkwin != NULL) {
		Tcl_AppendElement(interp, Tk_PathName(tkwin));
	    }
	}
    } 
    return TCL_OK;
}

static int
DebugOp(
    ClientData clientData,		/* Global data associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    Grab *grabPtr;
    GrabCmdInterpData *dataPtr = clientData;
    int state;

    if (Tcl_GetBooleanFromObj(interp, objv[2], &state) != TCL_OK) {
	return TCL_ERROR;
    }
    dataPtr->debug = state;
    return TCL_OK;
}

static int
EmptyOp(
    ClientData clientData,		/* Global data associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    Grab *grabPtr;
    GrabCmdInterpData *dataPtr = clientData;
    int state;

    grabPtr = GetTopGrab(dataPtr);
    state = (grabPtr == NULL);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

static int
ListOp(
    ClientData clientData,		/* Global data associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    Blt_ChainLink link;
    GrabCmdInterpData *dataPtr = clientData;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (link = Blt_Chain_FirstLink(dataPtr->chain); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
	Grab *grabPtr;
	Tcl_Obj *objPtr;
	const char *status;

	grabPtr = Blt_Chain_GetValue(link);
	objPtr = Tcl_NewStringObj(Tk_PathName(grabPtr->entryPtr->tkwin), -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	status = (grabPtr->flags & GRAB_GLOBAL) ? "global" : "local";
	objPtr = Tcl_NewStringObj(status, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}


static int
PopOp(
    ClientData clientData,		/* Global data associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    GrabCmdInterpData *dataPtr = clientData;
    Grab *grabPtr;

    if (dataPtr->debug) {
	fprintf(stderr, "grab pop %s\n", 
		(objc == 3) ? Tcl_GetString(objv[2]) : "");
    }
    grabPtr = GetTopGrab(dataPtr);
    if (grabPtr == NULL) {
	/* Blt_Warn("Popping an empty grab stack\n"); */
	return TCL_OK;			/* Stack is empty. */
    }
    if (objc == 3) {
	const char *pathName;
	Tk_Window tkwin;

	pathName = Tcl_GetString(objv[2]);
	tkwin = Tk_NameToWindow(interp, pathName, dataPtr->tkwin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	if (grabPtr->entryPtr->tkwin != tkwin) {
	    Blt_Warn("This is no grab on this window %s, it's on %s\n", 
		     Tk_PathName(tkwin), Tk_PathName(grabPtr->entryPtr->tkwin));
	    return TCL_OK;
	}
    } 
    PopGrab(dataPtr, grabPtr);

    /* Now reset the grab to the top window in the stack.  */
    grabPtr = GetTopGrab(dataPtr);
    if (grabPtr == NULL) {
	return TCL_OK;			/* Stack is empty. */
    }
    if (Tk_Grab(interp, grabPtr->entryPtr->tkwin, grabPtr->flags & GRAB_GLOBAL) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), 
		Tk_PathName(grabPtr->entryPtr->tkwin), -1);
    return TCL_OK;
}

static int
PushOp(
    ClientData clientData,		/* Global data associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    const char *pathName;
    Tk_Window tkwin;
    PushSwitches switches;
    GrabCmdInterpData *dataPtr = clientData;

    pathName = Tcl_GetString(objv[2]);
    if (dataPtr->debug) {
	fprintf(stderr, "grab push %s\n", pathName);
    }
    tkwin = Tk_NameToWindow(interp, pathName, dataPtr->tkwin);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    /* Process switches  */
    switches.global = 0;
    if (Blt_ParseSwitches(interp, pushSwitches, objc - 3, objv + 3, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (Tk_Grab(interp, tkwin, switches.global) != TCL_OK) {
	return TCL_ERROR;
    }
    return PushGrab(dataPtr, tkwin, switches.global);
}

static int
ReleaseOp(
    ClientData clientData,		/* Global data associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    GrabCmdInterpData *dataPtr = clientData;
    Grab *grabPtr;
    const char *pathName;
    Tk_Window tkwin;

    if (dataPtr->debug) {
	fprintf(stderr, "grab release %s\n", Tcl_GetString(objv[2]));
    }
    grabPtr = GetTopGrab(dataPtr);
    if (grabPtr == NULL) {
	return TCL_OK;			/* Stack is empty. */
    }
    pathName = Tcl_GetString(objv[2]);
    tkwin = Tk_NameToWindow(NULL, pathName, dataPtr->tkwin);
    if (tkwin == NULL) {
	return TCL_OK;			/* Unknown window. */
    }
    if (grabPtr->entryPtr->tkwin != tkwin) {
	return TCL_OK;			/* Not currently grabbed. */
    }
    PopGrab(dataPtr, grabPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetOp --
 *
 * 	Sets/resets the grab on the top of the stack.
 *
 * Results:
 *	A standard TCL result.
 *
 *	grab set -global window 
 *	grab set window 
 *	grab -global window
 *	grab window
 *
 *---------------------------------------------------------------------------
 */
static int
SetOp(
    ClientData clientData,		/* Global data associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    Grab *grabPtr;
    GrabCmdInterpData *dataPtr = clientData;
    Tk_Window tkwin;
    unsigned int flag;
    const char *string;

    /* Check if we're using the old style grab syntax. */
    string = Tcl_GetString(objv[1]);
    if (strcmp(string, "set") == 0) {
	objc--, objv++;			/* Skip over the "set" */
    }
    if (objc > 0) {
	string = Tcl_GetString(objv[1]);
	flag = GRAB_LOCAL;
	if (strcmp(string, "-global") == 0) {
	    flag = GRAB_GLOBAL;
	    objc--, objv++;		/* Skip over the "-global" flag. */
	}
    }
    if (objc != 2) {
	Tcl_AppendResult(interp, 
		"wrong # arguments: should be grab set ?-global? window",
		(char *)NULL);
	return TCL_ERROR;
    }
    string = Tcl_GetString(objv[1]);
    if (dataPtr->debug) {
	fprintf(stderr, "grab set %s\n", string);
    }
    tkwin = Tk_NameToWindow(interp, string, dataPtr->tkwin);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    grabPtr = GetTopGrab(dataPtr);
    if ((grabPtr != NULL) && ((grabPtr->entryPtr->tkwin != tkwin) ||
	(grabPtr->flags != flag))) {
	PopGrab(dataPtr, grabPtr);
    }
    return PushGrab(dataPtr, tkwin, flag);
}

static int
StatusOp(
    ClientData clientData,		/* Global data associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    Blt_ChainLink link;
    GrabCmdInterpData *dataPtr = clientData;
    Tk_Window tkwin;
    const char *status, *pathName;

    pathName = Tcl_GetString(objv[2]);
    tkwin = Tk_NameToWindow(interp, pathName, dataPtr->tkwin);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    status = "none";
    for (link = Blt_Chain_FirstLink(dataPtr->chain); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
	Grab *grabPtr;

	grabPtr = Blt_Chain_GetValue(link);
	if (grabPtr->entryPtr->tkwin == tkwin) {
	    status = (grabPtr->flags & GRAB_GLOBAL) ? "global" : "local";
	    break;
	}
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), status, -1);
    return TCL_OK;
}

static int
TopOp(
    ClientData clientData,		/* Global data associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    GrabCmdInterpData *dataPtr = clientData;
    Grab *grabPtr;

    grabPtr = GetTopGrab(dataPtr);
    if (grabPtr == NULL) {
	return TCL_OK;			/* Stack is empty. */
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	Tk_PathName(grabPtr->entryPtr->tkwin), -1);
    return TCL_OK;
}

static Blt_OpSpec grabOps[] =
{
    {"current",     1, CurrentOp,     2, 3, "?window?",},
    {"debug",       1, DebugOp,       3, 3, "bool",},
    {"empty",       1, EmptyOp,       2, 2, "",},
    {"list",        1, ListOp,        2, 2, "",},
    {"pop",         2, PopOp,         2, 3, "?window?",},
    {"push",        2, PushOp,	      3, 0, "window ?switches?",},
    {"release",     1, ReleaseOp,     3, 3, "window",},
    {"set",         2, SetOp,	      3, 4, "?-global? window",},
    {"status",      2, StatusOp,      3, 3, "window",},
    {"top",         1, TopOp,         2, 2, "",},
};


static int numGrabOps = sizeof(grabOps) / sizeof(Blt_OpSpec);

static int
GrabCmd(
    ClientData clientData,		/* Main window associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* # of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    Tcl_ObjCmdProc *proc;
    GrabCmdInterpData *dataPtr = clientData;

    if (FixCurrent(interp, dataPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    proc = Blt_GetOpFromObj(interp, numGrabOps, grabOps, BLT_OP_ARG1, 
	objc, objv, 0);
    if (proc == NULL) {
	if (objc > 1) {
	    const char *string;
	    char c;
	    
	    string = Tcl_GetString(objv[1]);
	    c = string[0];
	    if ((c == '.') || ((c == '-') && (strcmp(string, "-global")==0))) {
		/* Replicate the Tk grab command syntax. Assume this is a
		 * "set" operation. */
		Tcl_ResetResult(interp);
		return SetOp(clientData, interp, objc, objv);
	    }
	}
	return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GrabCmdInitProc --
 *
 *	This procedure is invoked to initialize the "grab" command. 
 *	You should be able to replace the Tk grab command with this one.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the new command and adds a new entry into a global Tcl
 *	associative array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GrabCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { 
	"grab", GrabCmd, 
    };
    cmdSpec.clientData = GetGrabCmdInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

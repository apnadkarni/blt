/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltGrPlay.c --
 *
 * This module implements playback for the BLT graph widget.
 *
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif

#include "bltHash.h"
#include "bltChain.h"
#include "bltOp.h"
#include "bltBind.h"
#include "bltPs.h"
#include "bltBg.h"
#include "bltPicture.h"
#include "bltGraph.h"
#include "bltGrElem.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"

#define LOOP		1
#define HOLD		2

#define FORWARD		0
#define BACK		1
#define BOTH		2

typedef int (GraphPlayProc)(Graph *graphPtr, Tcl_Interp *interp, 
	int objc, Tcl_Obj *const *objv);


#define DEF_PLAY_START		"-1"
#define DEF_PLAY_END		"-1"
#define DEF_PLAY_LOOP		"0"
#define DEF_PLAY_DIRECTION	"forward"
#define DEF_PLAY_DELAY		"0"
#define DEF_PLAY_ENABLED	"0"
#define DEF_PLAY_OFFSET		"0"

static Blt_OptionParseProc ObjToDirection;
static Blt_OptionPrintProc DirectionToObj;
static Blt_CustomOption directionOption =
{
    ObjToDirection, DirectionToObj, NULL, (ClientData)0
};

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_INT, "-first", "first", "First", DEF_PLAY_START, 
	Blt_Offset(Playback, first), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_INT, "-last", "last", "Last", DEF_PLAY_END, 
	Blt_Offset(Playback, last), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-loop", "loop", "loop", 
	DEF_PLAY_LOOP, Blt_Offset(Playback, flags), 
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)LOOP},
    {BLT_CONFIG_BOOLEAN, "-enable", "enable", "enable", 
	DEF_PLAY_ENABLED, Blt_Offset(Playback, enabled), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_INT_NNEG, "-delay", "delay", "delay", DEF_PLAY_DELAY, 
	Blt_Offset(Playback, interval), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-direction", "direction", "Direction", 
	DEF_PLAY_DIRECTION, Blt_Offset(Playback, direction), 
	BLT_CONFIG_DONT_SET_DEFAULT, &directionOption},
    {BLT_CONFIG_INT_NNEG, "-offset", "offset", "offset", DEF_PLAY_OFFSET, 
	Blt_Offset(Playback, offset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

/*
 *---------------------------------------------------------------------------
 *
 * ObjToDirection --
 *
 *	Given a string name, get the direction associated with it.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToDirection(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,		        /* Interpreter to report results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    int *dirPtr = (int *)(widgRec + offset);
    const char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'f') && (strncmp(string, "forward", length) == 0)) {
	*dirPtr = FORWARD;
    } else if ((c == 'b') && (length > 1) && 
	       (strncmp(string, "back", length) == 0)) {
	*dirPtr = BACK;
    } else if ((c == 'b') && (length > 1) && 
	       (strncmp(string, "both", length) == 0)) {
	*dirPtr = BOTH;
    } else {
	Tcl_AppendResult(interp, "unknown direction \"", string, 
		"\": should be forward, back, or both.", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DirectionToObj --
 *
 *	Convert the direction into a string Tcl_Obj.
 *
 * Results:
 *	The string representation of the direction is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
DirectionToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    int direction = *(int *)(widgRec + offset);

    switch (direction) {
    case BACK:
	return Tcl_NewStringObj("back", -1);
    case FORWARD:
	return Tcl_NewStringObj("forward", -1);
    case BOTH:
	return Tcl_NewStringObj("both", -1);
    }
    return Tcl_NewStringObj("???", -1);
}

#ifdef notdef
static void
TimerProc(ClientData clientData)
{
    Graph *graphPtr = clientData;
    Playback *playPtr;
    int delay;

    playPtr = &graphPtr->play;
    delay = 0;
    if (playPtr->interval > 0) {
	delay = playPtr->interval;
    }
    /* Issue callback to let graph change it's timepoints. */
    if (playPtr->flags & HOLD) {
	playPtr->timerToken = Tcl_CreateTimerHandler(delay, TimerProc, 
		graphPtr);
    }
}
#endif

static int
ConfigurePlayback(Graph *graphPtr, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv, int flags)
{
    Playback *playPtr = &graphPtr->play;

    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
	objc, objv, (char *)playPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    if (playPtr->first > playPtr->last) {
	playPtr->t1 = playPtr->last;
	playPtr->t2 = playPtr->first;
    } else {
	playPtr->t1 = playPtr->first;
	playPtr->t2 = playPtr->last;
    }
    if (playPtr->t2 < 0) {
	playPtr->t2 = UINT_MAX;
    }
    return TCL_OK;
}

void
Blt_HoldPlayback(Graph *graphPtr)
{
    Playback *playPtr = &graphPtr->play;

    if (playPtr->flags & HOLD) {
	playPtr->flags &= ~HOLD;
	Blt_EventuallyRedrawGraph(graphPtr);
    }
}

void
Blt_ContinuePlayback(Graph *graphPtr)
{
    Playback *playPtr = &graphPtr->play;

    if ((playPtr->flags & HOLD) == 0) {
	playPtr->flags |= HOLD;
	/* Let the next interval occur. */
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyPlayback --
 *
 * Results:
 *	None
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DestroyPlayback(Graph *graphPtr)
{
    Playback *playPtr = &graphPtr->play;

    Blt_FreeOptions(configSpecs, (char *)playPtr, graphPtr->display, 0);
    if (playPtr->timerToken != (Tcl_TimerToken)0) {
	Tcl_DeleteTimerHandler(playPtr->timerToken);
	playPtr->timerToken = 0;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreatePlayback --
 *
 *	Initializes the playback structure used to replay graphs.
 *
 * Results:
 *	Returns TCL_ERROR if the play structure can't be created,
 *	otherwise TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_CreatePlayback(Graph *graphPtr)
{
    Playback *playPtr;

    playPtr = &graphPtr->play;
    playPtr->flags = 0;
    playPtr->first = -1;
    playPtr->last = -1;
    playPtr->interval = 0;
    playPtr->timerToken = NULL;
    if (ConfigurePlayback(graphPtr, graphPtr->interp, 0, NULL, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *	Queries configuration attributes of the playback.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CgetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Playback *playPtr = &graphPtr->play;
    
    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
				     (char *)playPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *	Queries or resets configuration attributes of the playback.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Playback options are reset.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Playback *playPtr = &graphPtr->play;

    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
		(char *)playPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
		(char *)playPtr, objv[3], 0);
    }
    if (ConfigurePlayback(graphPtr, interp, objc - 3, objv + 3, 
	BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_ConfigModified(configSpecs, "-first", "-last", "-offset", "-enable",
			   (char *)NULL)) {
	graphPtr->flags |= REDRAW_WORLD | CACHE_DIRTY;
	Blt_EventuallyRedrawGraph(graphPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MaxtimeOp --
 *
 *	Queries configuration attributes of the playback.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
MaxtimeOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_ChainLink link;
    int maxNumPts;

    /* Draw with respect to the stacking order. */
    maxNumPts = 0;
    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if ((elemPtr->flags & HIDE) == 0) {
	    int length;

	    length = NUMBEROFPOINTS(elemPtr);
	    if (length > maxNumPts) {
		maxNumPts = length;
	    }
	}
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), maxNumPts);
    return TCL_OK;
}

static Blt_OpSpec playOps[] =
{
    {"cget",      2, CgetOp, 4, 4, "option",},
    {"configure", 2, ConfigureOp, 3, 0, "?options...?",},
    {"maxtime",   1, MaxtimeOp, 3, 3, "",},
};
static int numPlayOps = sizeof(playOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PlaybackOp --
 *
 *	Used to configure the playback.  Playback
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *	.g playback configure -first 0 -last 10
 *---------------------------------------------------------------------------
 */
int
Blt_PlaybackOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj 
	       *const *objv)
{
    GraphPlayProc *proc;

    proc = Blt_GetOpFromObj(interp, numPlayOps, playOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (graphPtr, interp, objc, objv);
}

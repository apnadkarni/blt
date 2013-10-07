
/*
 * bltGrLegend.c --
 *
 * This module implements the legend for the BLT graph widget.
 *
 *	Copyright 1993-2004 George A Howlett.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltOp.h"
#include "bltHash.h"
#include "bltChain.h"
#include "bltBind.h"
#include "bltPicture.h"
#include "bltPs.h"
#include "bltBg.h"
#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"
#include "bltGrElem.h"

/*
 *  Selection related flags:
 *
 *	SELECT_EXPORT		Export the selection to X11.
 *
 *	SELECT_PENDING		A "selection" command idle task is pending.
 *
 *	SELECT_CLEAR		Clear selection flag of entry.
 *
 *	SELECT_SET		Set selection flag of entry.
 *
 *	SELECT_TOGGLE		Toggle selection flag of entry.
 *
 *	SELECT_MASK		Mask of selection set/clear/toggle flags.
 *
 *	SELECT_SORTED		Indicates if the entries in the selection 
 *				should be sorted or displayed in the order 
 *				they were selected.
 *
 */

#define SELECT_CLEAR		(1<<16)
#define SELECT_EXPORT		(1<<17) 
#define SELECT_PENDING		(1<<18)
#define SELECT_SET		(1<<19)
#define SELECT_TOGGLE		(SELECT_SET | SELECT_CLEAR)
#define SELECT_MASK		(SELECT_SET | SELECT_CLEAR)
#define SELECT_SORTED		(1<<20)

#define RAISED			(1<<21)
#define CHANGE_PENDING		(1<<22)

#define SELECT_MODE_SINGLE	(1<<0)
#define SELECT_MODE_MULTIPLE	(1<<1)

typedef int (GraphLegendProc)(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv);

/*
 * Legend --
 *
 * 	Contains information specific to how the legend will be displayed.
 */
struct _Legend {
    unsigned int flags;
    ClassId classId;			/* Type: Element or Marker. */
    int numEntries;			/* # of element entries in table. */
    short int numColumns, numRows;	/* # of columns and rows in legend */
    short int width, height;		/* Dimensions of the legend */
    short int entryWidth, entryHeight;
    int site;
    short int xReq, yReq;		/* User-requested site of legend, not
					 * the actual final position. Used in
					 * conjunction with the anchor below
					 * to determine location of the
					 * legend. */
    Tk_Anchor anchor;			/* Anchor of legend. Used to interpret
					 * the positioning point of the legend
					 * in the graph*/
    int x, y;				/* Computed origin of legend. */
    Graph *graphPtr;
    Tcl_Command cmdToken;		/* Token for graph's widget command. */
    int reqColumns, reqRows;

    Blt_Pad ixPad, iyPad;		/* # of pixels interior padding around
					 * legend entries */
    Blt_Pad xPad, yPad;			/* # of pixels padding to exterior of
					 * legend */
    Tk_Window tkwin;			/* If non-NULL, external window to draw
					 * legend. */
    TextStyle style;
    int maxSymSize;			/* Size of largest symbol to be
					 * displayed.  Used to calculate size
					 * of legend */
    XColor *fgColor;
    Blt_Bg activeBg;			/* Active legend entry background
					 * color. */
    XColor *activeFgColor;
    int activeRelief;			/* 3-D effect on active entry. */
    int entryBW;			/* Border width around each entry in
					 * legend. */
    Blt_Bg normalBg;			/* 3-D effect of legend. */
    int borderWidth;			/* Width of legend 3-D border */
    int relief;				/* 3-d effect of border around the
					 * legend: TK_RELIEF_RAISED etc. */
    Blt_BindTable bindTable;
    int selRelief;
    int selBW;
    XColor *selInFocusFgColor;		/* Text color of a selected entry. */
    XColor *selOutFocusFgColor;
    Blt_Bg selInFocusBg;
    Blt_Bg selOutFocusBg;
    XColor *focusColor;
    Blt_Dashes focusDashes;		/* Dash on-off value. */
    GC focusGC;				/* Graphics context for the active
					 * label. */
    const char *takeFocus;
    int focus;				/* Position of the focus entry. */
    int cursorX, cursorY;		/* Position of the insertion cursor in
					 * the textbox window. */
    short int cursorWidth;		/* Size of the insertion cursor
					 * symbol. */
    short int cursorHeight;
    Element *focusPtr;			/* Element that currently has the
					 * focus. If NULL, no legend entry has
					 * the focus. */
    Element *selAnchorPtr;		/* Fixed end of selection. Used to
					 * extend the selection while
					 * maintaining the other end of the
					 * selection. */
    Element *selMarkPtr;
    Element *selFirstPtr;		/* First element selected in current
					 * pick. */
    Element *selLastPtr;		/* Last element selected in current
					 * pick. */
    Element *activePtr;			/* Element whose label is active in the
					 * legend. */
    int cursorOn;			/* Indicates if the cursor is
					 * displayed. */
    int onTime, offTime;		/* Time in milliseconds to wait before
					 * changing the cursor from off-to-on
					 * and on-to-off. Setting offTime to 0
					 * makes the * cursor steady. */
    Tcl_TimerToken timerToken;		/* Handle for a timer event called
					 * periodically to blink the cursor. */
    Tcl_Obj *selCmdObjPtr;		/* TCL script that's invoked whenever
					 * the selection changes. */
    int selMode;			/* Mode of selection: single or
					 * multiple. */
    Blt_HashTable selTable;		/* Table of selected elements. Used to
					 * quickly determine whether an element
					 * is selected. */
    Blt_Chain selected;			/* List of selected elements. */
    const char *title;
    short int titleWidth, titleHeight;
    TextStyle titleStyle;		/* Legend title attributes */
    Tcl_Obj *cmdObjPtr;			/* Specifies a TCL script to be
					 * invoked whenever the legend has
					 * changed. This is used to keep and
					 * external legend in sync with the
					 * graph. */
};

#define padLeft  	xPad.side1
#define padRight  	xPad.side2
#define padTop		yPad.side1
#define padBottom	yPad.side2
#define PADDING(x)	((x).side1 + (x).side2)
#define LABEL_PAD	2

#define DEF_ACTIVEBACKGROUND 	RGB_SKYBLUE4
#define DEF_ACTIVEBORDERWIDTH    "2"
#define DEF_ACTIVEFOREGROUND	RGB_WHITE
#define DEF_ACTIVERELIEF	"flat"
#define DEF_ANCHOR	   	"n"
#define DEF_BACKGROUND	   	(char *)NULL
#define DEF_BORDERWIDTH		STD_BORDERWIDTH
#define DEF_COMMAND		(char *)NULL
#define DEF_COLUMNS		"0"
#define DEF_EXPORTSELECTION	"no"
#define DEF_FONT		"{Sans Serif} 8"
#define DEF_FOREGROUND		STD_NORMAL_FOREGROUND
#define DEF_HIDE		"no"
#define DEF_IPADX		"1"
#define DEF_IPADY		"1"
#define DEF_PADX		"1"
#define DEF_PADY		"1"
#define DEF_POSITION		"rightmargin"
#define DEF_RAISED       	"no"
#define DEF_RELIEF		"flat"
#define DEF_ROWS		"0"
#define DEF_SELECTBACKGROUND 	RGB_SKYBLUE4
#define DEF_SELECT_BG_MONO  	STD_SELECT_BG_MONO
#define DEF_SELECTBORDERWIDTH	"1"
#define DEF_SELECTMODE		"multiple"
#define DEF_SELECT_FG_MONO  	STD_SELECT_FG_MONO
#define DEF_SELECTFOREGROUND 	RGB_WHITE /*STD_SELECT_FOREGROUND*/
#define DEF_SELECTRELIEF	"flat"
#define DEF_FOCUSDASHES		"dot"
#define DEF_FOCUSEDIT		"no"
#define DEF_FOCUSFOREGROUND	STD_ACTIVE_FOREGROUND
#define DEF_FOCUS_FG_MONO	STD_ACTIVE_FG_MONO
#define DEF_TAKEFOCUS		"1"
#define DEF_TITLE		(char *)NULL
#define	DEF_TITLECOLOR		STD_NORMAL_FOREGROUND
#define DEF_TITLEFONT		"{Sans Serif} 8 bold"

static Blt_OptionParseProc ObjToPosition;
static Blt_OptionPrintProc PositionToObj;
static Blt_CustomOption legendPositionOption =
{
    ObjToPosition, PositionToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToSelectmode;
static Blt_OptionPrintProc SelectmodeToObj;
static Blt_CustomOption selectmodeOption = {
    ObjToSelectmode, SelectmodeToObj, NULL, NULL,
};

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground",
	"ActiveBackground", DEF_ACTIVEBACKGROUND, 
	Blt_Offset(Legend, activeBg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-activeborderwidth", "activeBorderWidth",
	"BorderWidth", DEF_BORDERWIDTH, 
	Blt_Offset(Legend, entryBW), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground",
	"ActiveForeground", DEF_ACTIVEFOREGROUND,
	Blt_Offset(Legend, activeFgColor), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "Relief",
	DEF_ACTIVERELIEF, Blt_Offset(Legend, activeRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_ANCHOR, 
	Blt_Offset(Legend, anchor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_BACKGROUND, Blt_Offset(Legend, normalBg),BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BORDERWIDTH, Blt_Offset(Legend, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_INT_NNEG, "-columns", "columns", "columns",
	DEF_COLUMNS, Blt_Offset(Legend, reqColumns),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_COMMAND, 
	Blt_Offset(Legend, cmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BITMASK, "-exportselection", "exportSelection",
	"ExportSelection", DEF_EXPORTSELECTION, 
	Blt_Offset(Legend, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)SELECT_EXPORT},
    {BLT_CONFIG_DASHES, "-focusdashes", "focusDashes", "FocusDashes",
	DEF_FOCUSDASHES, Blt_Offset(Legend, focusDashes), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-focusforeground", "focusForeground", "FocusForeground",
	DEF_FOCUSFOREGROUND, Blt_Offset(Legend, focusColor),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-focusforeground", "focusForeground", "FocusForeground",
	DEF_FOCUS_FG_MONO, Blt_Offset(Legend, focusColor),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_FONT, 
	Blt_Offset(Legend, style.font), 0},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	DEF_FOREGROUND, Blt_Offset(Legend, fgColor), 0},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_HIDE, 
	Blt_Offset(Legend, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)HIDE},
    {BLT_CONFIG_PAD, "-ipadx", "iPadX", "Pad", DEF_IPADX, 
	Blt_Offset(Legend, ixPad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-ipady", "iPadY", "Pad", DEF_IPADY, 
	Blt_Offset(Legend, iyPad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-nofocusselectbackground", 
	"noFocusSelectBackground", "NoFocusSelectBackground", 
	DEF_SELECTBACKGROUND, Blt_Offset(Legend, selOutFocusBg), 0},
    {BLT_CONFIG_COLOR, "-nofocusselectforeground", "noFocusSelectForeground", 
	"NoFocusSelectForeground", DEF_SELECTFOREGROUND, 
	Blt_Offset(Legend, selOutFocusFgColor), 0},
    {BLT_CONFIG_PAD, "-padx", "padX", "Pad", DEF_PADX, 
	Blt_Offset(Legend, xPad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-pady", "padY", "Pad", DEF_PADY, 
	Blt_Offset(Legend, yPad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-position", "position", "Position", 
	DEF_POSITION, 0, BLT_CONFIG_DONT_SET_DEFAULT, 
        &legendPositionOption},
    {BLT_CONFIG_BITMASK, "-raised", "raised", "Raised", DEF_RAISED, 
	Blt_Offset(Legend, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)RAISED},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RELIEF, 
	Blt_Offset(Legend, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_INT_NNEG, "-rows", "rows", "rows", DEF_ROWS, 
	Blt_Offset(Legend, reqRows),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
	"Background", DEF_SELECTBACKGROUND, 
	Blt_Offset(Legend, selInFocusBg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-selectborderwidth", "selectBorderWidth", 
        "BorderWidth", DEF_SELECTBORDERWIDTH, 
	Blt_Offset(Legend, selBW), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-selectcommand", "selectCommand", "SelectCommand",
	(char *)NULL, Blt_Offset(Legend, selCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Foreground",
	DEF_SELECT_FG_MONO, Blt_Offset(Legend, selInFocusFgColor),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Foreground",
	DEF_SELECTFOREGROUND, Blt_Offset(Legend, selInFocusFgColor),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_CUSTOM, "-selectmode", "selectMode", "SelectMode",
	DEF_SELECTMODE, Blt_Offset(Legend, selMode),
	BLT_CONFIG_DONT_SET_DEFAULT, &selectmodeOption},
    {BLT_CONFIG_RELIEF, "-selectrelief", "selectRelief", "Relief",
	DEF_SELECTRELIEF, Blt_Offset(Legend, selRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_TAKEFOCUS, Blt_Offset(Legend, takeFocus), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-title", "title", "Title", DEF_TITLE, 
	Blt_Offset(Legend, title), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-titlecolor", "titleColor", "Foreground",
	DEF_TITLECOLOR, Blt_Offset(Legend, titleStyle.color), 0},
    {BLT_CONFIG_FONT, "-titlefont", "titleFont", "Font",
	DEF_TITLEFONT, Blt_Offset(Legend, titleStyle.font), 0},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Tcl_IdleProc DisplayLegend;
static Blt_BindPickProc PickEntryProc;
static Tk_EventProc LegendEventProc;
static Tcl_TimerProc BlinkCursorProc;
static Tk_LostSelProc LostSelectionProc;
static Tk_SelectionProc SelectionProc;

BLT_EXTERN Tcl_ObjCmdProc Blt_GraphInstCmdProc;

/*
 *---------------------------------------------------------------------------
 *
 * SelectCmdProc --
 *
 *      Invoked at the next idle point whenever the current selection changes.
 *      Executes some application-specific code in the -selectcommand option.
 *      This provides a way for applications to handle selection changes.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      TCL code gets executed for some application-specific task.
 *
 *---------------------------------------------------------------------------
 */
static void
LegendChangedProc(ClientData clientData) 
{
    Legend *legdPtr = clientData;

    legdPtr->flags &= ~CHANGE_PENDING;
    if (legdPtr->cmdObjPtr != NULL) {
	Tcl_Interp *interp;

	Tcl_Preserve(legdPtr);
	interp = legdPtr->graphPtr->interp;
	if (Tcl_EvalObjEx(interp, legdPtr->cmdObjPtr, TCL_EVAL_GLOBAL) 
	    != TCL_OK) {
	    Tcl_BackgroundError(interp);
	}
	Tcl_Release(legdPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyInvokeChangeCmd --
 *
 *      Queues a request to execute the -command code associated with
 *      the widget at the next idle point.  Invoked whenever the legend
 *      changes.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      TCL code gets executed for some application-specific task.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyInvokeChangeCmd(Legend *legdPtr)
{
    if ((legdPtr->flags & CHANGE_PENDING) == 0) {
	legdPtr->flags |= CHANGE_PENDING;
	Tcl_DoWhenIdle(LegendChangedProc, legdPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Legend_EventuallyRedraw --
 *
 *	Tells the Tk dispatcher to call the graph display routine at the next
 *	idle point.  This request is made only if the window is displayed and
 *	no other redraw request is pending.
 *
 * Results: None.
 *
 * Side effects:
 *	The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Legend_EventuallyRedraw(Graph *graphPtr) 
{
    Legend *legdPtr = graphPtr->legend;

    if (legdPtr->cmdObjPtr != NULL) {
	EventuallyInvokeChangeCmd(legdPtr);
    }
    if ((legdPtr->tkwin != NULL) && !(legdPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayLegend, legdPtr);
	legdPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectCmdProc --
 *
 *      Invoked at the next idle point whenever the current selection changes.
 *      Executes some application-specific code in the -selectcommand option.
 *      This provides a way for applications to handle selection changes.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      TCL code gets executed for some application-specific task.
 *
 *---------------------------------------------------------------------------
 */
static void
SelectCmdProc(ClientData clientData) 
{
    Legend *legdPtr = clientData;

    legdPtr->flags &= ~SELECT_PENDING;
    if (legdPtr->selCmdObjPtr != NULL) {
	Tcl_Interp *interp;

	Tcl_Preserve(legdPtr);
	interp = legdPtr->graphPtr->interp;
	if (Tcl_EvalObjEx(interp, legdPtr->selCmdObjPtr, TCL_EVAL_GLOBAL) 
	    != TCL_OK) {
	    Tcl_BackgroundError(interp);
	}
	Tcl_Release(legdPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyInvokeSelectCmd --
 *
 *      Queues a request to execute the -selectcommand code associated with
 *      the widget at the next idle point.  Invoked whenever the selection
 *      changes.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      TCL code gets executed for some application-specific task.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyInvokeSelectCmd(Legend *legdPtr)
{
    if ((legdPtr->flags & SELECT_PENDING) == 0) {
	legdPtr->flags |= SELECT_PENDING;
	Tcl_DoWhenIdle(SelectCmdProc, legdPtr);
    }
}

static void
ClearSelection(Legend *legdPtr)
{
    Blt_DeleteHashTable(&legdPtr->selTable);
    Blt_InitHashTable(&legdPtr->selTable, BLT_ONE_WORD_KEYS);
    Blt_Chain_Reset(legdPtr->selected);
    Blt_Legend_EventuallyRedraw(legdPtr->graphPtr);
    if (legdPtr->selCmdObjPtr != NULL) {
	EventuallyInvokeSelectCmd(legdPtr);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * LostSelectionProc --
 *
 *	This procedure is called back by Tk when the selection is grabbed away
 *	from a Text widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The existing selection is unhighlighted, and the window is marked as
 *	not containing a selection.
 *
 *---------------------------------------------------------------------------
 */
static void
LostSelectionProc(ClientData clientData)
{
    Legend *legdPtr = clientData;

    if (legdPtr->flags & SELECT_EXPORT) {
	ClearSelection(legdPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * LegendEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various events on
 *	graphs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up.
 *	When it gets exposed, the graph is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
LegendEventProc(ClientData clientData, XEvent *eventPtr)
{
    Graph *graphPtr = clientData;
    Legend *legdPtr;

    legdPtr = graphPtr->legend;
    if (eventPtr->type == Expose) {
	if (eventPtr->xexpose.count == 0) {
	    Blt_Legend_EventuallyRedraw(graphPtr);
	}
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
	if (eventPtr->xfocus.detail == NotifyInferior) {
	    return;
	}
	if (eventPtr->type == FocusIn) {
	    legdPtr->flags |= FOCUS;
	} else {
	    legdPtr->flags &= ~FOCUS;
	}
	Tcl_DeleteTimerHandler(legdPtr->timerToken);
	if ((legdPtr->flags & (FOCUS|ACTIVE)) == (FOCUS|ACTIVE)) {
	    legdPtr->cursorOn = TRUE;
	    if (legdPtr->offTime != 0) {
		legdPtr->timerToken = Tcl_CreateTimerHandler(
			legdPtr->onTime, BlinkCursorProc, graphPtr);
	    }
	} else {
	    legdPtr->cursorOn = FALSE;
	    legdPtr->timerToken = (Tcl_TimerToken)NULL;
	}
	Blt_Legend_EventuallyRedraw(graphPtr);
    } else if (eventPtr->type == DestroyNotify) {
	Graph *graphPtr = legdPtr->graphPtr;

	if (legdPtr->site == LEGEND_WINDOW) {
	    Blt_DeleteWindowInstanceData(legdPtr->tkwin);
	    if (legdPtr->cmdToken != NULL) {
		Tcl_DeleteCommandFromToken(graphPtr->interp, 
					   legdPtr->cmdToken);
		legdPtr->cmdToken = NULL;
	    }
	    legdPtr->tkwin = graphPtr->tkwin;
	}
	if (legdPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayLegend, legdPtr);
	    legdPtr->flags &= ~REDRAW_PENDING;
	}
	legdPtr->site = LEGEND_RIGHT;
	legdPtr->flags |= HIDE;
	graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
	Blt_MoveBindingTable(legdPtr->bindTable, graphPtr->tkwin);
	Blt_EventuallyRedrawGraph(graphPtr);
    } else if (eventPtr->type == ConfigureNotify) {
	Blt_Legend_EventuallyRedraw(graphPtr);
    }
}

static int
CreateLegendWindow(Tcl_Interp *interp, Legend *legdPtr, const char *pathName)
{
    Graph *graphPtr;
    Tk_Window tkwin;

    graphPtr = legdPtr->graphPtr;
    tkwin = Tk_CreateWindowFromPath(interp, graphPtr->tkwin, pathName, NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    Blt_SetWindowInstanceData(tkwin, legdPtr);
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
	  LegendEventProc, graphPtr);
    /* Move the legend's binding table to the new window. */
    Blt_MoveBindingTable(legdPtr->bindTable, tkwin);
    if (legdPtr->tkwin != graphPtr->tkwin) {
	Tk_DestroyWindow(legdPtr->tkwin);
    }
    /* Create a command by the same name as the legend window so that Legend
     * bindings can use %W interchangably.  */
    legdPtr->cmdToken = Tcl_CreateObjCommand(interp, pathName, 
	Blt_GraphInstCmdProc, graphPtr, NULL);
    legdPtr->tkwin = tkwin;
    legdPtr->site = LEGEND_WINDOW;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPosition --
 *
 *	Convert the string representation of a legend XY position into window
 *	coordinates.  The form of the string must be "@x,y" or none.
 *
 * Results:
 *	The return value is a standard TCL result.  The symbol type is written
 *	into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPosition(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* New legend position string */
    char *widgRec,			/* Widget record */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
{
    Graph *graphPtr;
    Legend *legdPtr = (Legend *)widgRec;
    char c;
    int length;
    const char *string;

    graphPtr = legdPtr->graphPtr;
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if (c == '\0') {
	legdPtr->site = LEGEND_RIGHT;
    } else if ((c == 'l') && (strncmp(string, "leftmargin", length) == 0)) {
	legdPtr->site = LEGEND_LEFT;
    } else if ((c == 'r') && (strncmp(string, "rightmargin", length) == 0)) {
	legdPtr->site = LEGEND_RIGHT;
    } else if ((c == 't') && (strncmp(string, "topmargin", length) == 0)) {
	legdPtr->site = LEGEND_TOP;
    } else if ((c == 'b') && (strncmp(string, "bottommargin", length) == 0)) {
	legdPtr->site = LEGEND_BOTTOM;
    } else if ((c == 'p') && (strncmp(string, "plotarea", length) == 0)) {
	legdPtr->site = LEGEND_PLOT;
    } else if (c == '@') {
	char *comma;
	long x, y;
	int result;
	
	comma = strchr(string + 1, ',');
	if (comma == NULL) {
	    Tcl_AppendResult(interp, "bad screen position \"", string,
			     "\": should be @x,y", (char *)NULL);
	    return TCL_ERROR;
	}
	x = y = 0;
	*comma = '\0';
	result = ((Tcl_ExprLong(interp, string + 1, &x) == TCL_OK) &&
		  (Tcl_ExprLong(interp, comma + 1, &y) == TCL_OK));
	*comma = ',';
	if (!result) {
	    return TCL_ERROR;
	}
	legdPtr->xReq = (short int)x;
	legdPtr->yReq = (short int)y;
	legdPtr->site = LEGEND_XY;
    } else if (c == '.') {
	if (CreateLegendWindow(interp, legdPtr, string) != TCL_OK) {
	    return TCL_ERROR;
	}
	Blt_Legend_EventuallyRedraw(graphPtr);
    } else {
	Tcl_AppendResult(interp, "bad position \"", string, "\": should be  \
\"leftmargin\", \"rightmargin\", \"topmargin\", \"bottommargin\", \
\"plotarea\", windowName or @x,y", (char *)NULL);
	return TCL_ERROR;
    }
    graphPtr->flags |= RESET_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PositionToObj --
 *
 *	Convert the window coordinates into a string.
 *
 * Results:
 *	The string representing the coordinate position is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PositionToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
{
    Legend *legdPtr = (Legend *)widgRec;
    Tcl_Obj *objPtr;

    switch (legdPtr->site) {
    case LEGEND_LEFT:
	objPtr = Tcl_NewStringObj("leftmargin", -1);
	break;

    case LEGEND_RIGHT:
	objPtr = Tcl_NewStringObj("rightmargin", -1);
	break;

    case LEGEND_TOP:
	objPtr = Tcl_NewStringObj("topmargin", -1);
	break;

    case LEGEND_BOTTOM:
	objPtr = Tcl_NewStringObj("bottommargin", -1);
	break;

    case LEGEND_PLOT:
	objPtr = Tcl_NewStringObj("plotarea", -1);
	break;

    case LEGEND_WINDOW:
	objPtr = Tcl_NewStringObj(Tk_PathName(legdPtr->tkwin), -1);
	break;

    case LEGEND_XY:
	{
	    char string[200];

	    Blt_FormatString(string, 200, "@%d,%d", legdPtr->xReq, legdPtr->yReq);
	    objPtr = Tcl_NewStringObj(string, -1);
	}
    default:
	objPtr = Tcl_NewStringObj("unknown legend position", -1);
    }
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSelectmode --
 *
 *	Convert the string reprsenting a select mode, to its numeric form.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	Otherwise, TCL_ERROR is returned and an error message is left
 *	in interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToSelectmode(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* Tcl_Obj representing the new value. */
    char *widgRec,
    int offset,			/* Offset to field in structure */
    int flags)	
{
    char *string;
    char c;
    int *modePtr = (int *)(widgRec + offset);

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 's') && (strcmp(string, "single") == 0)) {
	*modePtr = SELECT_MODE_SINGLE;
    } else if ((c == 'm') && (strcmp(string, "multiple") == 0)) {
	*modePtr = SELECT_MODE_MULTIPLE;
    } else if ((c == 'a') && (strcmp(string, "active") == 0)) {
	*modePtr = SELECT_MODE_SINGLE;
    } else {
	Tcl_AppendResult(interp, "bad select mode \"", string,
	    "\": should be \"single\" or \"multiple\"", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectmodeToObj --
 *
 * Results:
 *	The string representation of the select mode is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SelectmodeToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,
    int offset,				/* Offset to field in structure */
    int flags)	
{
    int mode = *(int *)(widgRec + offset);

    switch (mode) {
    case SELECT_MODE_SINGLE:
	return Tcl_NewStringObj("single", -1);
    case SELECT_MODE_MULTIPLE:
	return Tcl_NewStringObj("multiple", -1);
    default:
	return Tcl_NewStringObj("unknown scroll mode", -1);
    }
}


static void
SetLegendOrigin(Legend *legdPtr)
{
    Graph *graphPtr;
    int x, y, w, h;

    graphPtr = legdPtr->graphPtr;
    x = y = w = h = 0;			/* Suppress compiler warning. */
    switch (legdPtr->site) {
    case LEGEND_RIGHT:
	w = graphPtr->rightMargin.width - graphPtr->rightMargin.axesOffset;
	h = graphPtr->bottom - graphPtr->top;
	x = graphPtr->right + graphPtr->rightMargin.axesOffset;
	y = graphPtr->top;
	break;

    case LEGEND_LEFT:
	w = graphPtr->leftMargin.width - graphPtr->leftMargin.axesOffset;
	h = graphPtr->bottom - graphPtr->top;
	x = graphPtr->inset;
	y = graphPtr->top;
	break;

    case LEGEND_TOP:
	w = graphPtr->right - graphPtr->left;
	h = graphPtr->topMargin.height - graphPtr->topMargin.axesOffset;
	if (graphPtr->title != NULL) {
	    h -= graphPtr->titleHeight;
	}
	x = graphPtr->left;
	y = graphPtr->inset;
	if (graphPtr->title != NULL) {
	    y += graphPtr->titleHeight;
	}
	break;

    case LEGEND_BOTTOM:
	w = graphPtr->right - graphPtr->left;
	h = graphPtr->bottomMargin.height - graphPtr->bottomMargin.axesOffset;
	x = graphPtr->left;
	y = graphPtr->bottom + graphPtr->bottomMargin.axesOffset;
	break;

    case LEGEND_PLOT:
	w = graphPtr->right - graphPtr->left;
	h = graphPtr->bottom - graphPtr->top;
	x = graphPtr->left;
	y = graphPtr->top;
	break;

    case LEGEND_XY:
	w = legdPtr->width;
	h = legdPtr->height;
	x = legdPtr->xReq;
	y = legdPtr->yReq;
	if (x < 0) {
	    x += graphPtr->width;
	}
	if (y < 0) {
	    y += graphPtr->height;
	}
	break;

    case LEGEND_WINDOW:
	legdPtr->anchor = TK_ANCHOR_NW;
	legdPtr->x = legdPtr->y = 0;
	return;
    }

    switch (legdPtr->anchor) {
    case TK_ANCHOR_NW:			/* Upper left corner */
	break;
    case TK_ANCHOR_W:			/* Left center */
	if (h > legdPtr->height) {
	    y += (h - legdPtr->height) / 2;
	}
	break;
    case TK_ANCHOR_SW:			/* Lower left corner */
	if (h > legdPtr->height) {
	    y += (h - legdPtr->height);
	}
	break;
    case TK_ANCHOR_N:			/* Top center */
	if (w > legdPtr->width) {
	    x += (w - legdPtr->width) / 2;
	}
	break;
    case TK_ANCHOR_CENTER:		/* Center */
	if (h > legdPtr->height) {
	    y += (h - legdPtr->height) / 2;
	}
	if (w > legdPtr->width) {
	    x += (w - legdPtr->width) / 2;
	}
	break;
    case TK_ANCHOR_S:			/* Bottom center */
	if (w > legdPtr->width) {
	    x += (w - legdPtr->width) / 2;
	}
	if (h > legdPtr->height) {
	    y += (h - legdPtr->height);
	}
	break;
    case TK_ANCHOR_NE:			/* Upper right corner */
	if (w > legdPtr->width) {
	    x += w - legdPtr->width;
	}
	break;
    case TK_ANCHOR_E:			/* Right center */
	if (w > legdPtr->width) {
	    x += w - legdPtr->width;
	}
	if (h > legdPtr->height) {
	    y += (h - legdPtr->height) / 2;
	}
	break;
    case TK_ANCHOR_SE:		/* Lower right corner */
	if (w > legdPtr->width) {
	    x += w - legdPtr->width;
	}
	if (h > legdPtr->height) {
	    y += (h - legdPtr->height);
	}
	break;
    }
    legdPtr->x = x + legdPtr->padLeft;
    legdPtr->y = y + legdPtr->padTop;
}

static int
EntryIsSelected(Legend *legdPtr, Element *elemPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&legdPtr->selTable, (char *)elemPtr);
    return (hPtr != NULL);
}

static void
SelectElement(Legend *legdPtr, Element *elemPtr)
{
    int isNew;
    Blt_HashEntry *hPtr;

    hPtr = Blt_CreateHashEntry(&legdPtr->selTable, (char *)elemPtr,&isNew);
    if (isNew) {
	Blt_ChainLink link;

	link = Blt_Chain_Append(legdPtr->selected, elemPtr);
	Blt_SetHashValue(hPtr, link);
    }
}

static void
DeselectElement(Legend *legdPtr, Element *elemPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&legdPtr->selTable, (char *)elemPtr);
    if (hPtr != NULL) {
	Blt_ChainLink link;

	link = Blt_GetHashValue(hPtr);
	Blt_Chain_DeleteLink(legdPtr->selected, link);
	Blt_DeleteHashEntry(&legdPtr->selTable, hPtr);
    }
}

static void
SelectEntry(Legend *legdPtr, Element *elemPtr)
{
    Blt_HashEntry *hPtr;

    switch (legdPtr->flags & SELECT_MASK) {
    case SELECT_CLEAR:
	DeselectElement(legdPtr, elemPtr);
	break;

    case SELECT_SET:
	SelectElement(legdPtr, elemPtr);
	break;

    case SELECT_TOGGLE:
	hPtr = Blt_FindHashEntry(&legdPtr->selTable, (char *)elemPtr);
	if (hPtr != NULL) {
	    DeselectElement(legdPtr, elemPtr);
	} else {
	    SelectElement(legdPtr, elemPtr);
	}
	break;
    }
}

#ifdef notdef
static Element *
PointerToElement(Legend *legdPtr, int x, int y)
{
    Graph *graphPtr = legdPtr->graphPtr;
    int w, h;
    int n;

    w = legdPtr->width;
    h = legdPtr->height;

    x -= legdPtr->x + legdPtr->borderWidth;
    y -= legdPtr->y + legdPtr->borderWidth;
    w -= 2 * legdPtr->borderWidth + PADDING(legdPtr->xPad);
    h -= 2 * legdPtr->borderWidth + PADDING(legdPtr->yPad);

    if ((x < 0) || (x >= w) || (y < 0) || (y >= h)) {
	return NULL;
    }

    /* It's in the bounding box, so compute the index. */
    {
	int row, column;

	row    = y / legdPtr->entryHeight;
	column = x / legdPtr->entryWidth;
	n = (column * legdPtr->numRows) + row;
    }
    if (n < legdPtr->numEntries) {
	Blt_ChainLink link;
	int count;

	count = 0;
	for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(link);
	    if (elemPtr->label == NULL) {
		continue;
	    }
	    if (count > n) {
		return NULL;
	    } else if (count == n) {
		return elemPtr;
	    }
	    count++;
	}	      
    }
    return NULL;
}
#endif

/*ARGSUSED*/
static ClientData
PickEntryProc(ClientData clientData, int x, int y, ClientData *contextPtr)
{
    Graph *graphPtr = clientData;
    Legend *legdPtr;
    int w, h;

    legdPtr = graphPtr->legend;
    w = legdPtr->width;
    h = legdPtr->height;

    if (legdPtr->titleHeight > 0) {
	y -= legdPtr->titleHeight + legdPtr->yPad.side1;
    }
    x -= legdPtr->x + legdPtr->borderWidth;
    y -= legdPtr->y + legdPtr->borderWidth;
    w -= 2 * legdPtr->borderWidth + PADDING(legdPtr->xPad);
    h -= 2 * legdPtr->borderWidth + PADDING(legdPtr->yPad);

    if ((x >= 0) && (x < w) && (y >= 0) && (y < h)) {
	int row, column;
	int n;

	/*
	 * It's in the bounding box, so compute the index.
	 */
	row    = y / legdPtr->entryHeight;
	column = x / legdPtr->entryWidth;
	n = (column * legdPtr->numRows) + row;
	if (n < legdPtr->numEntries) {
	    Blt_ChainLink link;
	    int count;

	    /* Legend entries are stored in bottom-to-top. */
	    count = 0;
	    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
		 link != NULL; link = Blt_Chain_NextLink(link)) {
		Element *elemPtr;

		elemPtr = Blt_Chain_GetValue(link);
		if (elemPtr->label != NULL) {
		    if (count == n) {
			return elemPtr;
		    }
		    count++;
		}
	    }	      
	    if (link != NULL) {
		return Blt_Chain_GetValue(link);
	    }	
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MapLegend --
 *
 * 	Calculates the dimensions (width and height) needed for the legend.
 * 	Also determines the number of rows and columns necessary to list all
 * 	the valid element labels.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *   	The following fields of the legend are calculated and set.
 *
 * 	numEntries   - number of valid labels of elements in the
 *		      display list.
 * 	numRows	    - number of rows of entries
 * 	numColumns    - number of columns of entries
 * 	entryHeight - height of each entry
 * 	entryWidth  - width of each entry
 * 	height	    - width of legend (includes borders and padding)
 * 	width	    - height of legend (includes borders and padding)
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MapLegend(
    Graph *graphPtr,
    int plotWidth,			/* Maximum width available in window
					 * to draw the legend. Will calculate
					 * # of columns from this. */
    int plotHeight)			/* Maximum height available in window
					 * to draw the legend. Will calculate
					 * # of rows from this. */
{
    Legend *legdPtr = graphPtr->legend;
    Blt_ChainLink link;
    int numRows, numColumns, numEntries;
    int legendWidth, legendHeight;
    int maxWidth, maxHeight;
    int symbolWidth;
    Blt_FontMetrics fontMetrics;
    unsigned int tw, th;

    /* Initialize legend values to default (no legend displayed) */
    legdPtr->entryWidth = legdPtr->entryHeight = 0;
    legdPtr->numRows = legdPtr->numColumns = legdPtr->numEntries = 0;
    legdPtr->height = legdPtr->width = 0;

    if (legdPtr->site == LEGEND_WINDOW) {
	if (Tk_Width(legdPtr->tkwin) > 1) {
	    plotWidth = Tk_Width(legdPtr->tkwin);
	}
	if (Tk_Height(legdPtr->tkwin) > 1) {
	    plotHeight = Tk_Height(legdPtr->tkwin);
	}
    }
    Blt_Ts_GetExtents(&legdPtr->titleStyle, legdPtr->title, &tw, &th);
    legdPtr->titleWidth = tw;
    legdPtr->titleHeight = th;
    /*   
     * Count the number of legend entries and determine the widest and tallest
     * label.  The number of entries would normally be the number of elements,
     * but elements can have no legend entry (-label "").
     */
    numEntries = 0;
    maxWidth = maxHeight = 0;
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	unsigned int w, h;
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;			/* Element has no legend entry. */
	}
	Blt_Ts_GetExtents(&legdPtr->style, elemPtr->label, &w, &h);
	if (maxWidth < w) {
	    maxWidth = w;
	}
	if (maxHeight < h) {
	    maxHeight = h;
	}
	numEntries++;
    }
    if (numEntries == 0) {
	return;				/* No visible legend entries. */
    }


    Blt_Font_GetMetrics(legdPtr->style.font, &fontMetrics);
    symbolWidth = 2 * fontMetrics.ascent;

    maxWidth += 2 * legdPtr->entryBW + PADDING(legdPtr->ixPad) +
	+ symbolWidth + 3 * LABEL_PAD;

    maxHeight += 2 * legdPtr->entryBW + PADDING(legdPtr->iyPad);

    maxWidth |= 0x01;
    maxHeight |= 0x01;

    legendWidth  = plotWidth;
    legendHeight = plotHeight;
    legendWidth  -= 2 * legdPtr->borderWidth + PADDING(legdPtr->xPad);
    legendHeight -= 2 * legdPtr->borderWidth + PADDING(legdPtr->yPad);

    /*
     * The number of rows and columns is computed as one of the following:
     *
     *	both options set		User defined. 
     *  -rows				Compute columns from rows.
     *  -columns			Compute rows from columns.
     *	neither set			Compute rows and columns from
     *					size of plot.  
     */
    if (legdPtr->reqRows > 0) {
	numRows = MIN(legdPtr->reqRows, numEntries); 
	if (legdPtr->reqColumns > 0) {
	    numColumns = MIN(legdPtr->reqColumns, numEntries);
	} else {
	    numColumns = ((numEntries - 1) / numRows) + 1; /* Only -rows. */
	}
    } else if (legdPtr->reqColumns > 0) { /* Only -columns. */
	numColumns = MIN(legdPtr->reqColumns, numEntries);
	numRows = ((numEntries - 1) / numColumns) + 1;
    } else {			
	/* Compute # of rows and columns from the legend size. */
	numRows = legendHeight / maxHeight;
	numColumns = legendWidth / maxWidth;
	if (numRows < 1) {
	    numRows = numEntries;
	}
	if (numColumns < 1) {
	    numColumns = numEntries;
	}
	if (numRows > numEntries) {
	    numRows = numEntries;
	} 
	switch (legdPtr->site) {
	case LEGEND_TOP:
	case LEGEND_BOTTOM:
	    numRows    = ((numEntries - 1) / numColumns) + 1;
	    break;
	case LEGEND_LEFT:
	case LEGEND_RIGHT:
	default:
	    numColumns = ((numEntries - 1) / numRows) + 1;
	    break;
	}
    }
    if (numColumns < 1) {
	numColumns = 1;
    } 
    if (numRows < 1) {
	numRows = 1;
    }

    legendHeight = (numRows * maxHeight);
    if (legdPtr->titleHeight > 0) {
	legendHeight += legdPtr->titleHeight + legdPtr->yPad.side1;
    }
    legendWidth = numColumns * maxWidth;
    if (legendWidth < legdPtr->titleWidth) {
	legendWidth = legdPtr->titleWidth;
    }
    legdPtr->width = legendWidth + 2 * legdPtr->borderWidth + 
	PADDING(legdPtr->xPad);
    legdPtr->height = legendHeight + 2 * legdPtr->borderWidth + 
	PADDING(legdPtr->yPad);
    legdPtr->numRows     = numRows;
    legdPtr->numColumns  = numColumns;
    legdPtr->numEntries  = numEntries;
    legdPtr->entryHeight = maxHeight;
    legdPtr->entryWidth  = maxWidth;

    {
	int row, col, count;

	row = col = count = 0;
	for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(link);
	    count++;
	    elemPtr->row = row;
	    elemPtr->col = col;
	    row++;
	    if ((count % numRows) == 0) {
		col++;
		row = 0;
	    }
	}
    }
    if ((legdPtr->site == LEGEND_WINDOW) &&
	((Tk_ReqWidth(legdPtr->tkwin) != legdPtr->width) ||
	 (Tk_ReqHeight(legdPtr->tkwin) != legdPtr->height))) {
	Tk_GeometryRequest(legdPtr->tkwin,legdPtr->width,legdPtr->height);
    }
}

void
Blt_DrawLegend(Graph *graphPtr, Drawable drawable)
{
    Blt_Bg bg;
    Blt_ChainLink link;
    Blt_FontMetrics fontMetrics;
    Legend *legdPtr = graphPtr->legend;
    Pixmap pixmap;
    Tk_Window tkwin;
    int count;
    int symbolSize, xMid, yMid;
    int x, y, w, h;
    int xLabel, yStart, xSymbol, ySymbol;

    if ((legdPtr->flags & HIDE) || (legdPtr->numEntries == 0)) {
	return;
    }
    SetLegendOrigin(legdPtr);
    graphPtr = legdPtr->graphPtr;
    tkwin = legdPtr->tkwin;
    if (legdPtr->site == LEGEND_WINDOW) {
	w = Tk_Width(tkwin);
	h = Tk_Height(tkwin);
    } else {
	w = legdPtr->width;
	h = legdPtr->height;
    }

    pixmap = Blt_GetPixmap(graphPtr->display, Tk_WindowId(tkwin), w, h, 
	Tk_Depth(tkwin));

    if (legdPtr->normalBg != NULL) {
	Blt_Bg_FillRectangle(tkwin, pixmap, legdPtr->normalBg, 0, 0, 
		w, h, 0, TK_RELIEF_FLAT);
    } else if (legdPtr->site & LEGEND_PLOTAREA_MASK) {
	/* 
	 * Legend background is transparent and is positioned over the the
	 * plot area.  Either copy the part of the background from the backing
	 * store pixmap or (if no backing store exists) just fill it with the
	 * background color of the plot.
	 */
	if (graphPtr->cache != None) {
	    XCopyArea(graphPtr->display, graphPtr->cache, pixmap, 
		graphPtr->drawGC, legdPtr->x, legdPtr->y, w, h, 0, 0);
        } else {
	    Blt_Bg_FillRectangle(tkwin, pixmap, graphPtr->plotBg, 0, 0, 
		w, h, TK_RELIEF_FLAT, 0);
 	}
    } else {
	int x0, y0;
	/* 
	 * The legend is located in one of the margins or the external window.
	 */
	Blt_Bg_GetOrigin(graphPtr->normalBg, &x0, &y0);
	Blt_Bg_SetOrigin(graphPtr->tkwin, graphPtr->normalBg, 
		x0 - legdPtr->x, y0 - legdPtr->y);
	Blt_Bg_FillRectangle(tkwin, pixmap, graphPtr->normalBg, 0, 0, 
		w, h, 0, TK_RELIEF_FLAT);
	Blt_Bg_SetOrigin(tkwin, graphPtr->normalBg, x0, y0);
    }
    Blt_Font_GetMetrics(legdPtr->style.font, &fontMetrics);

    symbolSize = fontMetrics.ascent;
    xMid = symbolSize + 1 + legdPtr->entryBW;
    yMid = (symbolSize / 2) + 1 + legdPtr->entryBW;
    xLabel = 2 * symbolSize + legdPtr->entryBW + 
	legdPtr->ixPad.side1 + 2 * LABEL_PAD;
    ySymbol = yMid + legdPtr->iyPad.side1; 
    xSymbol = xMid + LABEL_PAD;

    x = legdPtr->padLeft + legdPtr->borderWidth;
    y = legdPtr->padTop + legdPtr->borderWidth;
    Blt_DrawText(tkwin, pixmap, legdPtr->title, &legdPtr->titleStyle, x, y);
    if (legdPtr->titleHeight > 0) {
	y += legdPtr->titleHeight + legdPtr->yPad.side1;
    }
    count = 0;
    yStart = y;
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;
	int isSelected;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;			/* Skip this entry */
	}
	isSelected = EntryIsSelected(legdPtr, elemPtr);
	if (elemPtr == legdPtr->activePtr) {
	    int x0, y0;

	    Blt_Bg_GetOrigin(legdPtr->activeBg, &x0, &y0);
	    Blt_Bg_SetOrigin(tkwin, legdPtr->activeBg, 
		x0 - legdPtr->x, y0 - legdPtr->y);
	    Blt_Ts_SetForeground(legdPtr->style, legdPtr->activeFgColor);
	    Blt_Bg_FillRectangle(tkwin, pixmap, legdPtr->activeBg, x, y, 
		legdPtr->entryWidth, legdPtr->entryHeight, 
		legdPtr->entryBW, legdPtr->activeRelief);
	    Blt_Bg_SetOrigin(tkwin, legdPtr->activeBg, x0, y0);
	} else if (isSelected) {
	    int x0, y0;
	    Blt_Bg bg;
	    XColor *fg;

	    fg = (legdPtr->flags & FOCUS) ?
		legdPtr->selInFocusFgColor : legdPtr->selOutFocusFgColor;
	    bg = (legdPtr->flags & FOCUS) ?
		    legdPtr->selInFocusBg : legdPtr->selOutFocusBg;
	    Blt_Bg_GetOrigin(bg, &x0, &y0);
	    Blt_Bg_SetOrigin(tkwin, bg, x0 - legdPtr->x, y0 - legdPtr->y);
	    Blt_Ts_SetForeground(legdPtr->style, fg);
	    Blt_Bg_FillRectangle(tkwin, pixmap, bg, x, y, 
		legdPtr->entryWidth, legdPtr->entryHeight, 
		legdPtr->selBW, legdPtr->selRelief);
	    Blt_Bg_SetOrigin(tkwin, bg, x0, y0);
	} else {
	    Blt_Ts_SetForeground(legdPtr->style, legdPtr->fgColor);
	    if (elemPtr->legendRelief != TK_RELIEF_FLAT) {
		Blt_Bg_FillRectangle(tkwin, pixmap, graphPtr->normalBg, 
			x, y, legdPtr->entryWidth, 
			legdPtr->entryHeight, legdPtr->entryBW, 
			elemPtr->legendRelief);
	    }
	}
	(*elemPtr->procsPtr->drawSymbolProc) (graphPtr, pixmap, elemPtr,
		x + xSymbol, y + ySymbol, symbolSize);
	Blt_DrawText(tkwin, pixmap, elemPtr->label, &legdPtr->style, 
		x + xLabel, 
		y + legdPtr->entryBW + legdPtr->iyPad.side1);
	count++;
	if (legdPtr->focusPtr == elemPtr) { /* Focus outline */
	    if (isSelected) {
		XColor *color;

		color = (legdPtr->flags & FOCUS) ?
		    legdPtr->selInFocusFgColor :
		    legdPtr->selOutFocusFgColor;
		XSetForeground(graphPtr->display, legdPtr->focusGC, 
			       color->pixel);
	    }
	    XDrawRectangle(graphPtr->display, pixmap, legdPtr->focusGC, 
		x + 1, y + 1, legdPtr->entryWidth - 3, 
		legdPtr->entryHeight - 3);
	    if (isSelected) {
		XSetForeground(graphPtr->display, legdPtr->focusGC, 
			legdPtr->focusColor->pixel);
	    }
	}
	/* Check when to move to the next column */
	if ((count % legdPtr->numRows) > 0) {
	    y += legdPtr->entryHeight;
	} else {
	    x += legdPtr->entryWidth;
	    y = yStart;
	}
    }
    /*
     * Draw the border and/or background of the legend.
     */
    bg = legdPtr->normalBg;
    if (bg == NULL) {
	bg = graphPtr->normalBg;
    }
    /* Disable crosshairs before redisplaying to the screen */
    if (legdPtr->site & LEGEND_PLOTAREA_MASK) {
	Blt_DisableCrosshairs(graphPtr);
    }
    Blt_Bg_DrawRectangle(tkwin, pixmap, bg, 0, 0, w, h, 
	legdPtr->borderWidth, legdPtr->relief);
    XCopyArea(graphPtr->display, pixmap, drawable, graphPtr->drawGC, 0, 0, w, h,
	legdPtr->x, legdPtr->y);
    if (legdPtr->site & LEGEND_PLOTAREA_MASK) {
	Blt_EnableCrosshairs(graphPtr);
    }
    Tk_FreePixmap(graphPtr->display, pixmap);
    graphPtr->flags &= ~DRAW_LEGEND;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LegendToPostScript --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_LegendToPostScript(Graph *graphPtr, Blt_Ps ps)
{
    Legend *legdPtr = graphPtr->legend;
    double x, y, yStart;
    int xLabel, xSymbol, ySymbol;
    int count;
    Blt_ChainLink link;
    int symbolSize, xMid, yMid;
    int width, height;
    Blt_FontMetrics fontMetrics;

    if ((legdPtr->flags & HIDE) || (legdPtr->numEntries == 0)) {
	return;
    }
    SetLegendOrigin(legdPtr);

    x = legdPtr->x, y = legdPtr->y;
    width = legdPtr->width - PADDING(legdPtr->xPad);
    height = legdPtr->height - PADDING(legdPtr->yPad);

    Blt_Ps_Append(ps, "% Legend\n");
    graphPtr = legdPtr->graphPtr;
    if (graphPtr->pageSetup->flags & PS_DECORATIONS) {
	if (legdPtr->normalBg != NULL) {
	    Tk_3DBorder border;

	    border = Blt_Bg_Border(legdPtr->normalBg);
	    Blt_Ps_Fill3DRectangle(ps, border, x, y, width, height, 
		legdPtr->borderWidth, legdPtr->relief);
	} else {
	    Tk_3DBorder border;

	    border = Blt_Bg_Border(graphPtr->normalBg);
	    Blt_Ps_Draw3DRectangle(ps, border, x, y, width, height, 
		legdPtr->borderWidth, legdPtr->relief);
	}
    } else {
	Blt_Ps_SetClearBackground(ps);
	Blt_Ps_XFillRectangle(ps, x, y, width, height);
    }
    Blt_Font_GetMetrics(legdPtr->style.font, &fontMetrics);
    symbolSize = fontMetrics.ascent;
    xMid = symbolSize + 1 + legdPtr->entryBW;
    yMid = (symbolSize / 2) + 1 + legdPtr->entryBW;
    xLabel = 2 * symbolSize + legdPtr->entryBW + legdPtr->ixPad.side1 + 5;
    xSymbol = xMid + legdPtr->ixPad.side1;
    ySymbol = yMid + legdPtr->iyPad.side1;

    x += legdPtr->borderWidth;
    y += legdPtr->borderWidth;
    Blt_Ps_DrawText(ps, legdPtr->title, &legdPtr->titleStyle, x, y);
    if (legdPtr->titleHeight > 0) {
	y += legdPtr->titleHeight + legdPtr->yPad.side1;
    }

    count = 0;
    yStart = y;
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;			/* Skip this label */
	}
	if (elemPtr == legdPtr->activePtr) {
	    Tk_3DBorder border;
	    
	    border = Blt_Bg_Border(legdPtr->activeBg);
	    Blt_Ts_SetForeground(legdPtr->style, legdPtr->activeFgColor);
	    Blt_Ps_Fill3DRectangle(ps, border, x, y, legdPtr->entryWidth, 
		legdPtr->entryHeight, legdPtr->entryBW, 
		legdPtr->activeRelief);
	} else {
	    Blt_Ts_SetForeground(legdPtr->style, legdPtr->fgColor);
	    if (elemPtr->legendRelief != TK_RELIEF_FLAT) {
		Tk_3DBorder border;

		border = Blt_Bg_Border(graphPtr->normalBg);
		Blt_Ps_Draw3DRectangle(ps, border, x, y, legdPtr->entryWidth,
			legdPtr->entryHeight, legdPtr->entryBW, 
			elemPtr->legendRelief);
	    }
	}
	(*elemPtr->procsPtr->printSymbolProc) (graphPtr, ps, elemPtr, 
		x + xSymbol, y + ySymbol, symbolSize);
	Blt_Ps_DrawText(ps, elemPtr->label, &legdPtr->style, 
		x + xLabel, y + legdPtr->entryBW + legdPtr->iyPad.side1);
	count++;
	if ((count % legdPtr->numRows) > 0) {
	    y += legdPtr->entryHeight;
	} else {
	    x += legdPtr->entryWidth;
	    y = yStart;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayLegend --
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayLegend(ClientData clientData)
{
    Legend *legdPtr = clientData;
    Graph *graphPtr;

    legdPtr->flags &= ~REDRAW_PENDING;
    if (legdPtr->tkwin == NULL) {
	return;				/* Window has been destroyed. */
    }
    graphPtr = legdPtr->graphPtr;
    if (legdPtr->site == LEGEND_WINDOW) {
	int w, h;

	w = Tk_Width(legdPtr->tkwin);
	h = Tk_Height(legdPtr->tkwin);
	if ((w != legdPtr->width) || (h != legdPtr->height)) {
	    Blt_MapLegend(graphPtr, w, h);
	}
    }
    if (Tk_IsMapped(legdPtr->tkwin)) {
	Blt_DrawLegend(graphPtr, Tk_WindowId(legdPtr->tkwin));
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigureLegend --
 *
 * 	Routine to configure the legend.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ConfigureLegend(Graph *graphPtr)
{
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;
    Legend *legdPtr;

    legdPtr = graphPtr->legend;
    /* GC for active label. Dashed outline. */
    gcMask = GCForeground | GCLineStyle;
    gcValues.foreground = legdPtr->focusColor->pixel;
    gcValues.line_style = (LineIsDashed(legdPtr->focusDashes))
	? LineOnOffDash : LineSolid;
    newGC = Blt_GetPrivateGC(legdPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(legdPtr->focusDashes)) {
	legdPtr->focusDashes.offset = 2;
	Blt_SetDashes(graphPtr->display, newGC, &legdPtr->focusDashes);
    }
    if (legdPtr->focusGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, legdPtr->focusGC);
    }
    legdPtr->focusGC = newGC;
    
    if (legdPtr->cmdObjPtr != NULL) {
	EventuallyInvokeChangeCmd(legdPtr);
    }
    /*
     *  Update the layout of the graph (and redraw the elements) if any of
     *  the following legend options (all of which affect the size of the
     *  legend) have changed.
     *
     *		-activeborderwidth, -borderwidth
     *		-border
     *		-font
     *		-hide
     *		-ipadx, -ipady, -padx, -pady
     *		-rows
     *
     *  If the position of the legend changed to/from the default
     *  position, also indicate that a new layout is needed.
     *
     */
    if (legdPtr->site == LEGEND_WINDOW) {
	Blt_Legend_EventuallyRedraw(graphPtr);
    } else if (Blt_ConfigModified(configSpecs, "-*border*", "-*pad?",
	"-hide", "-font", "-rows", (char *)NULL)) {
	graphPtr->flags |= RESET_WORLD;
	graphPtr->flags |= (REDRAW_WORLD | CACHE_DIRTY);
	Blt_EventuallyRedrawGraph(graphPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyLegend --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with the legend are freed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DestroyLegend(Graph *graphPtr)
{
    Legend *legdPtr = graphPtr->legend;

    if (graphPtr->legend == NULL) {
	return;
    }

    Blt_FreeOptions(configSpecs, (char *)legdPtr, graphPtr->display, 0);
    Blt_Ts_FreeStyle(graphPtr->display, &legdPtr->style);
    Blt_Ts_FreeStyle(graphPtr->display, &legdPtr->titleStyle);
    Blt_DestroyBindingTable(legdPtr->bindTable);
    
    if (legdPtr->focusGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, legdPtr->focusGC);
    }
    if (legdPtr->timerToken != NULL) {
	Tcl_DeleteTimerHandler(legdPtr->timerToken);
    }
    if (legdPtr->tkwin != NULL) {
	Tk_DeleteSelHandler(legdPtr->tkwin, XA_PRIMARY, XA_STRING);
    }
    if (legdPtr->selected != NULL) {
	Blt_Chain_Destroy(legdPtr->selected);
    }
    if (legdPtr->site == LEGEND_WINDOW) {
	Tk_Window tkwin;
	
	/* The graph may be in the process of being torn down */
	if (legdPtr->cmdToken != NULL) {
	    Tcl_DeleteCommandFromToken(graphPtr->interp, legdPtr->cmdToken);
	}
	if (legdPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayLegend, legdPtr);
	    legdPtr->flags &= ~REDRAW_PENDING;
	}
	tkwin = legdPtr->tkwin;
	legdPtr->tkwin = NULL;
	if (tkwin != NULL) {
	    Tk_DeleteEventHandler(tkwin, ExposureMask | StructureNotifyMask,
		LegendEventProc, graphPtr);
	    Blt_DeleteWindowInstanceData(tkwin);
	    Tk_DestroyWindow(tkwin);
	}
    }
    Blt_Free(legdPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreateLegend --
 *
 * 	Creates and initializes a legend structure with default settings
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Blt_CreateLegend(Graph *graphPtr)
{
    Legend *legdPtr;

    legdPtr = Blt_AssertCalloc(1, sizeof(Legend));
    graphPtr->legend = legdPtr;
    legdPtr->graphPtr = graphPtr;
    legdPtr->tkwin = graphPtr->tkwin;
    legdPtr->xReq = legdPtr->yReq = -SHRT_MAX;
    legdPtr->relief = TK_RELIEF_SUNKEN;
    legdPtr->activeRelief = TK_RELIEF_FLAT;
    legdPtr->entryBW = 2;
    legdPtr->borderWidth = 2;
    legdPtr->ixPad.side1 = legdPtr->ixPad.side2 = 1;
    legdPtr->iyPad.side1 = legdPtr->iyPad.side2 = 1;
    legdPtr->xPad.side1  = legdPtr->xPad.side2  = 1;
    legdPtr->yPad.side1  = legdPtr->yPad.side2  = 1;
    legdPtr->anchor = TK_ANCHOR_N;
    legdPtr->site = LEGEND_RIGHT;
    legdPtr->selMode = SELECT_MODE_MULTIPLE;
    Blt_Ts_InitStyle(legdPtr->style);
    Blt_Ts_InitStyle(legdPtr->titleStyle);
    legdPtr->style.justify = TK_JUSTIFY_LEFT;
    legdPtr->style.anchor = TK_ANCHOR_NW;
    legdPtr->titleStyle.justify = TK_JUSTIFY_LEFT;
    legdPtr->titleStyle.anchor = TK_ANCHOR_NW;
    legdPtr->bindTable = Blt_CreateBindingTable(graphPtr->interp,
	graphPtr->tkwin, graphPtr, PickEntryProc, Blt_GraphTags);

    Blt_InitHashTable(&legdPtr->selTable, BLT_ONE_WORD_KEYS);
    legdPtr->selected = Blt_Chain_Create();
    Tk_CreateSelHandler(legdPtr->tkwin, XA_PRIMARY, XA_STRING, 
	SelectionProc, legdPtr, XA_STRING);
    legdPtr->selRelief = TK_RELIEF_FLAT;
    legdPtr->selBW = 1;
    legdPtr->onTime = 600;
    legdPtr->offTime = 300;
    if (Blt_ConfigureComponentFromObj(graphPtr->interp, graphPtr->tkwin,
	    "legend", "Legend", configSpecs, 0, (Tcl_Obj **)NULL,
	    (char *)legdPtr, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_ConfigureLegend(graphPtr);
    return TCL_OK;
}

static Element *
GetNextRow(Graph *graphPtr, Element *focusPtr)
{
    Blt_ChainLink link;
    int row, col;

    col = focusPtr->col;
    row = focusPtr->row + 1;
    for (link = focusPtr->link; link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;
	}
	if ((elemPtr->col == col) && (elemPtr->row == row)) {
	    return elemPtr;	
	}
    }
    return NULL;
}

static Element *
GetNextColumn(Graph *graphPtr, Element *focusPtr)
{
    Blt_ChainLink link;
    int row, col;

    col = focusPtr->col + 1;
    row = focusPtr->row;
    for (link = focusPtr->link; link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;
	}
	if ((elemPtr->col == col) && (elemPtr->row == row)) {
	    return elemPtr;		/* Don't go beyond legend boundaries. */
	}
    }
    return NULL;
}

static Element *
GetPreviousRow(Graph *graphPtr, Element *focusPtr)
{
    Blt_ChainLink link;
    int row, col;

    col = focusPtr->col;
    row = focusPtr->row - 1;
    for (link = focusPtr->link; link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;
	}
	if ((elemPtr->col == col) && (elemPtr->row == row)) {
	    return elemPtr;	
	}
    }
    return NULL;
}

static Element *
GetPreviousColumn(Graph *graphPtr, Element *focusPtr)
{
    Blt_ChainLink link;
    int row, col;

    col = focusPtr->col - 1;
    row = focusPtr->row;
    for (link = focusPtr->link; link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label == NULL) {
	    continue;
	}
	if ((elemPtr->col == col) && (elemPtr->row == row)) {
	    return elemPtr;	
	}
    }
    return NULL;
}

static Element *
GetFirstElement(Graph *graphPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label != NULL) {
	    return elemPtr;
	}
    }
    return NULL;
}

static Element *
GetLastElement(Graph *graphPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->label != NULL) {
	    return elemPtr;
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetElementFromObj --
 *
 *	Parse an index into an entry and return either its value or an error.
 *
 * Results:
 *	A standard TCL result.  If all went well, then *indexPtr is filled in
 *	with the character index (into entryPtr) corresponding to string.  The
 *	index value is guaranteed to lie between 0 and the number of characters
 *	in the string, inclusive.  If an error occurs then an error message is
 *	left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
GetElementFromObj(Graph *graphPtr, Tcl_Obj *objPtr, Element **elemPtrPtr)
{
    Element *elemPtr;
    Legend *legdPtr;
    Tcl_Interp *interp;
    char c;
    const char *string;

    legdPtr = graphPtr->legend;
    interp = graphPtr->interp;
    string = Tcl_GetString(objPtr);
    c = string[0];
    elemPtr = NULL;

    if ((c == 'a') && (strcmp(string, "anchor") == 0)) {
	elemPtr = legdPtr->selAnchorPtr;
    } else if ((c == 'c') && (strcmp(string, "current") == 0)) {
	GraphObj *objPtr;

	objPtr = Blt_GetCurrentItem(legdPtr->bindTable);
	if ((objPtr != NULL) && (!objPtr->deleted)) {
	    elemPtr = (Element *)objPtr;
	}
    } else if ((c == 'f') && (strcmp(string, "first") == 0)) {
	elemPtr = GetFirstElement(graphPtr);
    } else if ((c == 'f') && (strcmp(string, "focus") == 0)) {
	elemPtr = legdPtr->focusPtr;
    } else if ((c == 'l') && (strcmp(string, "last") == 0)) {
	elemPtr = GetLastElement(graphPtr);
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
	elemPtr = GetLastElement(graphPtr);
    } else if ((c == 'n') && (strcmp(string, "next.row") == 0)) {
	elemPtr = GetNextRow(graphPtr, legdPtr->focusPtr);
    } else if ((c == 'n') && (strcmp(string, "next.column") == 0)) {
	elemPtr = GetNextColumn(graphPtr, legdPtr->focusPtr);
    } else if ((c == 'p') && (strcmp(string, "previous.row") == 0)) {
	elemPtr = GetPreviousRow(graphPtr, legdPtr->focusPtr);
    } else if ((c == 'p') && (strcmp(string, "previous.column") == 0)) {
	elemPtr = GetPreviousColumn(graphPtr, legdPtr->focusPtr);
    } else if ((c == 's') && (strcmp(string, "sel.first") == 0)) {
	elemPtr = legdPtr->selFirstPtr;
    } else if ((c == 's') && (strcmp(string, "sel.last") == 0)) {
	elemPtr = legdPtr->selLastPtr;
    } else if (c == '@') {
	int x, y;

	if (Blt_GetXY(interp, legdPtr->tkwin, string, &x, &y) != TCL_OK) {
	    return TCL_ERROR;
	}
	elemPtr = (Element *)PickEntryProc(graphPtr, x, y, NULL);
    } else {
	if (Blt_GetElement(interp, graphPtr, objPtr, &elemPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (elemPtr->link == NULL) {
	    Tcl_AppendResult(interp, "bad legend index \"", string, "\"",
				 (char *)NULL);
	    return TCL_ERROR;
	}
	if (elemPtr->label == NULL) {
	    elemPtr = NULL;
	}
    }
    *elemPtrPtr = elemPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectRange --
 *
 *	Sets the selection flag for a range of nodes.  The range is determined
 *	by two pointers which designate the first/last nodes of the range.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectRange(Legend *legdPtr, Element *fromPtr, Element *toPtr)
{

    if (Blt_Chain_IsBefore(fromPtr->link, toPtr->link)) {
	Blt_ChainLink link;

	for (link = fromPtr->link; link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(link);
	    SelectEntry(legdPtr, elemPtr);
	    if (link == toPtr->link) {
		break;
	    }
	}
    } else {
	Blt_ChainLink link;

	for (link = fromPtr->link; link != NULL;
	     link = Blt_Chain_PrevLink(link)) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(link);
	    SelectEntry(legdPtr, elemPtr);
	    if (link == toPtr->link) {
		break;
	    }
	}
    }
    return TCL_OK;
}


#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * SelectText --
 *
 *	Modify the selection by moving its un-anchored end.  This could make
 *	the selection either larger or smaller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectText(Legend *legdPtr, Element *elemPtr)
{
    Element *firstPtr, *lastPtr;
    Graph *graphPtr = legdPtr->graphPtr;

    /* Grab the selection if we don't own it already. */
    if ((legdPtr->flags&SELECT_EXPORT) && (legdPtr->selFirstPtr == NULL)) {
	Tk_OwnSelection(legdPtr->tkwin, XA_PRIMARY, LostSelectionProc, 
		legdPtr);
    }
    /* If the anchor hasn't been set, assume the beginning of the legend. */
    if (legdPtr->selAnchorPtr == NULL) {
	legdPtr->selAnchorPtr = GetFirstElement(graphPtr);
    }
    if (legdPtr->selAnchorPtr != elemPtr) {
	firstPtr = legdPtr->selAnchorPtr;
	lastPtr = elemPtr;
    } else {
	firstPtr = elemPtr;
	lastPtr = legdPtr->selAnchorPtr;
    }
    if ((legdPtr->selFirstPtr != firstPtr) || 
	(legdPtr->selLastPtr != lastPtr)) {
	legdPtr->selFirstPtr = firstPtr;
	legdPtr->selLastPtr = lastPtr;
	SelectRange(legdPtr, firstPtr, lastPtr);
	Blt_Legend_EventuallyRedraw(graphPtr);
    }
    return TCL_OK;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 * 	Activates a one label in the legend.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn.
 *
 *	.g legend activate $elem
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Legend *legdPtr = graphPtr->legend;

    if (objc == 4) {
	Element *elemPtr;

	if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((elemPtr != NULL) && (elemPtr != legdPtr->activePtr)) {
	    legdPtr->activePtr = elemPtr;
	    if ((legdPtr->flags & HIDE) == 0) {
		if ((legdPtr->site != LEGEND_WINDOW) && 
		    (graphPtr->flags & REDRAW_PENDING)) {
		    graphPtr->flags |= CACHE_DIRTY;
		    graphPtr->flags |= REDRAW_WORLD; /* Redraw entire graph. */
		} else {
		    Blt_Legend_EventuallyRedraw(graphPtr);
		}
	    }
	}
    }
    if (legdPtr->activePtr != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
			 legdPtr->activePtr->obj.name, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BindOp --
 *
 *	  .t bind index sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (hPtr = Blt_FirstHashEntry(&graphPtr->elements.bindTagTable, &iter);
	    hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    const char *tagName;
	    Tcl_Obj *objPtr;

	    tagName = Blt_GetHashKey(&graphPtr->elements.bindTagTable, hPtr);
	    objPtr = Tcl_NewStringObj(tagName, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    return Blt_ConfigureBindingsFromObj(interp, graphPtr->legend->bindTable,
	Blt_MakeElementTag(graphPtr, Tcl_GetString(objv[3])), objc - 4, 
	objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 * 	Queries or resets options for the legend.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CgetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
	    (char *)graphPtr->legend, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * 	Queries or resets options for the legend.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int flags = BLT_CONFIG_OBJV_ONLY;
    Legend *legdPtr;

    legdPtr = graphPtr->legend;
    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
		(char *)legdPtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
		(char *)legdPtr, objv[3], flags);
    }
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
		objc - 3, objv + 3, (char *)legdPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_ConfigureLegend(graphPtr);
    return TCL_OK;
}


/*ARGSUSED*/
static int
CurselectionOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Legend *legdPtr = graphPtr->legend;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (legdPtr->flags & SELECT_SORTED) {
	Blt_ChainLink link;

	for (link = Blt_Chain_FirstLink(legdPtr->selected); link != NULL;
	     link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	    Tcl_Obj *objPtr;

	    elemPtr = Blt_Chain_GetValue(link);
	    objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    } else {
	Blt_ChainLink link;

	/* List of selected entries is in stacking order. */
	for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;

	    elemPtr = Blt_Chain_GetValue(link);

	    if (EntryIsSelected(legdPtr, elemPtr)) {
		Tcl_Obj *objPtr;

		objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeactivateOp --
 *
 * 	Deactivates all labels in the legend.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn.
 *
 *	.g legend deactivate 
 *
 *---------------------------------------------------------------------------
 */
static int
DeactivateOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Legend *legdPtr = graphPtr->legend;

    if (legdPtr->activePtr != NULL) {
	legdPtr->activePtr = NULL;
	if ((legdPtr->flags & HIDE) == 0) {
	    if ((legdPtr->site != LEGEND_WINDOW) && 
		(graphPtr->flags & REDRAW_PENDING)) {
		graphPtr->flags |= CACHE_DIRTY;
		graphPtr->flags |= REDRAW_WORLD; /* Redraw entire graph. */
	    } else {
		Blt_Legend_EventuallyRedraw(graphPtr);
	    }
	}
    }
    return TCL_OK;
}

/*ARGSUSED*/
static int
FocusOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Legend *legdPtr = graphPtr->legend;

    if (objc == 4) {
	Element *elemPtr;

	if (GetElementFromObj(graphPtr, objv[3], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((elemPtr != NULL) && (elemPtr != legdPtr->focusPtr)) {
	    /* Changing focus can only affect the visible entries.  The entry
	     * layout stays the same. */
	    legdPtr->focusPtr = elemPtr;
	}
	Blt_SetFocusItem(legdPtr->bindTable, legdPtr->focusPtr, 
			 CID_LEGEND_ENTRY);
	Blt_Legend_EventuallyRedraw(graphPtr);
    }
    if (legdPtr->focusPtr != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
		legdPtr->focusPtr->obj.name, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 * 	Find the legend entry from the given argument.  The argument can be
 * 	either a screen position "@x,y" or the name of an element.
 *
 *	I don't know how useful it is to test with the name of an element.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Legend *legdPtr = graphPtr->legend;

    if (((legdPtr->flags & HIDE) == 0) && (legdPtr->numEntries > 0)) {
	Element *elemPtr;

	if (GetElementFromObj(graphPtr, objv[3], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (elemPtr != NULL) {
	    Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name,-1);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * IconOp --
 *
 * 	Find the legend entry from the given argument.  The argument
 *	can be either a screen position "@x,y" or the name of an
 *	element.
 *
 *	I don't know how useful it is to test with the name of an
 *	element.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new legend attributes.
 *
 *	.g legend icon elemName image
 *
 *----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IconOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_Picture picture;
    Element *elemPtr;
    Legend *legdPtr = graphPtr->legend;
    Pixmap pixmap;
    Blt_FontMetrics fontMetrics;
    Tk_PhotoHandle photo;
    const char *imageName;
    int isPicture;
    int w, h, x, y, s;

    if (GetElementFromObj(graphPtr, objv[3], &elemPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (elemPtr == NULL) {
	return TCL_OK;			/* Unknown index. */
    }
    imageName = Tcl_GetString(objv[4]);
    photo = Tk_FindPhoto(interp, imageName);
    if (photo != NULL) {
	isPicture = FALSE;
    } else if (Blt_GetPicture(interp, imageName, &picture) == TCL_OK) {
	isPicture = TRUE;
    } else {
	return TCL_ERROR;
    }
    Blt_Font_GetMetrics(legdPtr->style.font, &fontMetrics);
    s = fontMetrics.ascent;
    h = s + PADDING(legdPtr->iyPad) + 1;
    w = s + s + 1 + PADDING(legdPtr->ixPad);
    x = (w / 2);
    y = (h / 2);
    
    pixmap = Blt_GetPixmap(graphPtr->display, Tk_RootWindow(graphPtr->tkwin),
	w, h, Tk_Depth(graphPtr->tkwin));
    Blt_Bg_FillRectangle(graphPtr->tkwin, pixmap, graphPtr->plotBg, 
		0, 0, w, h, TK_RELIEF_FLAT, 0);
    (*elemPtr->procsPtr->drawSymbolProc) (graphPtr, pixmap, elemPtr, x, y, s);
    picture = Blt_DrawableToPicture(graphPtr->tkwin, pixmap, 0, 0, w, h, 1.0);
    Tk_FreePixmap(graphPtr->display, pixmap);
    if (picture == NULL) {
	Tcl_AppendResult(interp, "can't get picture of symbol.", (char *)NULL);
	return TCL_ERROR;
    }
    /* Make the background transparent. */
    {
	int y;
	Blt_Pixel bg;
	Blt_Pixel *destRowPtr;
	XColor *colorPtr;

	colorPtr = Blt_Bg_BorderColor(graphPtr->plotBg);
	bg.Red   = colorPtr->red >> 8;
	bg.Green = colorPtr->green >> 8;
	bg.Blue  = colorPtr->blue >> 8;
	bg.Alpha = 0xFF;
	
	destRowPtr = Blt_PictureBits(picture);
	for (y = 0; y < h; y++) {
	    Blt_Pixel *dp, *dend;

	    for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
		if (dp->u32 == bg.u32) {
		    dp->Alpha = 0x0;
		}
	    }
	    destRowPtr += Blt_PictureStride(picture);
	}
    }
    Blt_ClassifyPicture(picture);
    if (isPicture) {
	Blt_ResetPicture(interp, imageName, picture);
    } else {
	Blt_PictureToPhoto(picture, photo);
	Blt_FreePicture(picture);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionAnchorOp --
 *
 *	Sets the selection anchor to the element given by a index.  The
 *	selection anchor is the end of the selection that is fixed while
 *	dragging out a selection with the mouse.  The index "anchor" may be
 *	used to refer to the anchor element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionAnchorOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    Legend *legdPtr = graphPtr->legend;
    Element *elemPtr;

    if (GetElementFromObj(graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Set both the anchor and the mark. Indicates that a single entry
     * is selected. */
    legdPtr->selAnchorPtr = elemPtr;
    legdPtr->selMarkPtr = NULL;
    if (elemPtr != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name, -1);
    }
    Blt_Legend_EventuallyRedraw(graphPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * SelectionClearallOp
 *
 *	Clears the entire selection.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionClearallOp(Graph *graphPtr, Tcl_Interp *interp, int objc,
		    Tcl_Obj *const *objv)
{
    Legend *legdPtr = graphPtr->legend;

    ClearSelection(legdPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionIncludesOp
 *
 *	Returns 1 if the element indicated by index is currently
 *	selected, 0 if it isn't.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionIncludesOp(Graph *graphPtr, Tcl_Interp *interp, int objc,
		    Tcl_Obj *const *objv)
{
    Legend *legdPtr = graphPtr->legend;
    Element *elemPtr;
    int bool;

    if (GetElementFromObj(graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    bool = EntryIsSelected(legdPtr, elemPtr);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionMarkOp --
 *
 *	Sets the selection mark to the element given by a index.  The
 *	selection anchor is the end of the selection that is movable while
 *	dragging out a selection with the mouse.  The index "mark" may be used
 *	to refer to the anchor element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionMarkOp(Graph *graphPtr, Tcl_Interp *interp, int objc,
		Tcl_Obj *const *objv)
{
    Legend *legdPtr = graphPtr->legend;
    Element *elemPtr;

    if (GetElementFromObj(graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (legdPtr->selAnchorPtr == NULL) {
	Tcl_AppendResult(interp, "selection anchor must be set first", 
		 (char *)NULL);
	return TCL_ERROR;
    }
    if (legdPtr->selMarkPtr != elemPtr) {
	Blt_ChainLink link, next;

	/* Deselect entry from the list all the way back to the anchor. */
	for (link = Blt_Chain_LastLink(legdPtr->selected); link != NULL; 
	     link = next) {
	    Element *selectPtr;

	    next = Blt_Chain_PrevLink(link);
	    selectPtr = Blt_Chain_GetValue(link);
	    if (selectPtr == legdPtr->selAnchorPtr) {
		break;
	    }
	    DeselectElement(legdPtr, selectPtr);
	}
	legdPtr->flags &= ~SELECT_MASK;
	legdPtr->flags |= SELECT_SET;
	SelectRange(legdPtr, legdPtr->selAnchorPtr, elemPtr);
	Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name, -1);
	legdPtr->selMarkPtr = elemPtr;

	Blt_Legend_EventuallyRedraw(graphPtr);
	if (legdPtr->selCmdObjPtr != NULL) {
	    EventuallyInvokeSelectCmd(legdPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionPresentOp
 *
 *	Returns 1 if there is a selection and 0 if it isn't.
 *
 * Results:
 *	A standard TCL result.  interp->result will contain a boolean string
 *	indicating if there is a selection.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionPresentOp(Graph *graphPtr, Tcl_Interp *interp, int objc,
		   Tcl_Obj *const *objv)
{
    Legend *legdPtr = graphPtr->legend;
    int bool;

    bool = (Blt_Chain_GetLength(legdPtr->selected) > 0);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionSetOp
 *
 *	Selects, deselects, or toggles all of the elements in the range
 *	between first and last, inclusive, without affecting the selection
 *	state of elements outside that range.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *	.g legend selection set first last
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionSetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Legend *legdPtr = graphPtr->legend;
    Element *firstPtr, *lastPtr;
    const char *string;

    legdPtr->flags &= ~SELECT_MASK;
    string = Tcl_GetString(objv[3]);
    switch (string[0]) {
    case 's':
	legdPtr->flags |= SELECT_SET;
	break;
    case 'c':
	legdPtr->flags |= SELECT_CLEAR;
	break;
    case 't':
	legdPtr->flags |= SELECT_TOGGLE;
	break;
    }
    if (GetElementFromObj(graphPtr, objv[4], &firstPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((firstPtr->flags & HIDE) && ((legdPtr->flags & SELECT_CLEAR)==0)) {
	Tcl_AppendResult(interp, "can't select hidden node \"", 
		Tcl_GetString(objv[4]), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    lastPtr = firstPtr;
    if (objc > 5) {
	if (GetElementFromObj(graphPtr, objv[5], &lastPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((lastPtr->flags & HIDE) && 
	    ((legdPtr->flags & SELECT_CLEAR) == 0)) {
	    Tcl_AppendResult(interp, "can't select hidden node \"", 
		     Tcl_GetString(objv[5]), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (firstPtr == lastPtr) {
	SelectEntry(legdPtr, firstPtr);
    } else {
	SelectRange(legdPtr, firstPtr, lastPtr);
    }
    /* Set both the anchor and the mark. Indicates that a single entry is
     * selected. */
    if (legdPtr->selAnchorPtr == NULL) {
	legdPtr->selAnchorPtr = firstPtr;
    }
    if (legdPtr->flags & SELECT_EXPORT) {
	Tk_OwnSelection(legdPtr->tkwin, XA_PRIMARY, LostSelectionProc, 
			legdPtr);
    }
    Blt_Legend_EventuallyRedraw(graphPtr);
    if (legdPtr->selCmdObjPtr != NULL) {
	EventuallyInvokeSelectCmd(legdPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionOp --
 *
 *	This procedure handles the individual options for text selections.
 *	The selected text is designated by start and end indices into the text
 *	pool.  The selected segment has both a anchored and unanchored ends.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *	.g legend selection anchor 
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec selectionOps[] =
{
    {"anchor",   1, SelectionAnchorOp,   5, 5, "elem",},
    {"clear",    5, SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
    {"clearall", 6, SelectionClearallOp, 4, 4, "",},
    {"includes", 1, SelectionIncludesOp, 5, 5, "elem",},
    {"mark",     1, SelectionMarkOp,     5, 5, "elem",},
    {"present",  1, SelectionPresentOp,  4, 4, "",},
    {"set",      1, SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
    {"toggle",   1, SelectionSetOp,      5, 6, "firstElem ?lastElem?",},
};
static int numSelectionOps = sizeof(selectionOps) / sizeof(Blt_OpSpec);

static int
SelectionOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    GraphLegendProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numSelectionOps, selectionOps, BLT_OP_ARG3, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (graphPtr, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LegendOp --
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Legend is possibly redrawn.
 *
 *---------------------------------------------------------------------------
 */

static Blt_OpSpec legendOps[] =
{
    {"activate",     1, ActivateOp,      3, 4, "?elem?",},
    {"bind",         1, BindOp,          3, 6, "elem sequence command",},
    {"cget",         2, CgetOp,          4, 4, "option",},
    {"configure",    2, ConfigureOp,     3, 0, "?option value?...",},
    {"curselection", 2, CurselectionOp,  3, 3, "",},
    {"deactivate",   1, DeactivateOp,    3, 3, "",},
    {"focus",        1, FocusOp,         4, 4, "elem",},
    {"get",          1, GetOp,           4, 4, "elem",},
    {"icon",         1, IconOp,          5, 5, "elem image",},
    {"selection",    1, SelectionOp,     3, 0, "args"},
};
static int numLegendOps = sizeof(legendOps) / sizeof(Blt_OpSpec);

int
Blt_LegendOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    GraphLegendProc *proc;

    proc = Blt_GetOpFromObj(interp, numLegendOps, legendOps, BLT_OP_ARG2, 
	objc, objv,0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (graphPtr, interp, objc, objv);
}

int 
Blt_Legend_Site(Graph *graphPtr)
{
    return graphPtr->legend->site;
}

int 
Blt_Legend_Width(Graph *graphPtr)
{
    return graphPtr->legend->width;
}

int 
Blt_Legend_Height(Graph *graphPtr)
{
    return graphPtr->legend->height;
}

int 
Blt_Legend_IsHidden(Graph *graphPtr)
{
    return (graphPtr->legend->flags & HIDE);
}

int 
Blt_Legend_IsRaised(Graph *graphPtr)
{
    return (graphPtr->legend->flags & RAISED);
}

int 
Blt_Legend_X(Graph *graphPtr)
{
    return graphPtr->legend->x;
}

int 
Blt_Legend_Y(Graph *graphPtr)
{
    return graphPtr->legend->y;
}

void
Blt_Legend_RemoveElement(Graph *graphPtr, Element *elemPtr)
{
    Blt_DeleteBindings(graphPtr->legend->bindTable, elemPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionProc --
 *
 *	This procedure is called back by Tk when the selection is requested by
 *	someone.  It returns part or all of the selection in a buffer provided
 *	by the caller.
 *
 * Results:
 *	The return value is the number of non-NULL bytes stored at buffer.
 *	Buffer is filled (or partially filled) with a NUL-terminated string
 *	containing part or all of the selection, as given by offset and
 *	maxBytes.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectionProc(
    ClientData clientData,		/* Information about the widget. */
    int offset,				/* Offset within selection of first
					 * character to be returned. */
    char *buffer,			/* Location in which to place
					 * selection. */
    int maxBytes)			/* Maximum number of bytes to place at
					 * buffer, not including terminating
					 * NULL character. */
{
    Legend *legdPtr = clientData;
    int numBytes;
    Tcl_DString ds;

    if ((legdPtr->flags & SELECT_EXPORT) == 0) {
	return -1;
    }
    /* Retrieve the names of the selected entries. */
    Tcl_DStringInit(&ds);
    if (legdPtr->flags & SELECT_SORTED) {
	Blt_ChainLink link;

	for (link = Blt_Chain_FirstLink(legdPtr->selected); 
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;

	    elemPtr = Blt_Chain_GetValue(link);
	    Tcl_DStringAppend(&ds, elemPtr->obj.name, -1);
	    Tcl_DStringAppend(&ds, "\n", -1);
	}
    } else {
	Blt_ChainLink link;
	Graph *graphPtr;

	graphPtr = legdPtr->graphPtr;
	/* List of selected entries is in stacking order. */
	for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	     link != NULL; link = Blt_Chain_NextLink(link)) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(link);
	    if (EntryIsSelected(legdPtr, elemPtr)) {
		Tcl_DStringAppend(&ds, elemPtr->obj.name, -1);
		Tcl_DStringAppend(&ds, "\n", -1);
	    }
	}
    }
    numBytes = Tcl_DStringLength(&ds) - offset;
    strncpy(buffer, Tcl_DStringValue(&ds) + offset, maxBytes);
    Tcl_DStringFree(&ds);
    buffer[maxBytes] = '\0';
    return MIN(numBytes, maxBytes);
}


/*
 *---------------------------------------------------------------------------
 *
 * BlinkCursorProc --
 *
 *	This procedure is called as a timer handler to blink the insertion
 *	cursor off and on.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cursor gets turned on or off, redisplay gets invoked, and this
 *	procedure reschedules itself.
 *
 *---------------------------------------------------------------------------
 */
static void
BlinkCursorProc(ClientData clientData)
{
    Graph *graphPtr = clientData;
    Legend *legdPtr;

    legdPtr = graphPtr->legend;
    if (!(legdPtr->flags & FOCUS) || (legdPtr->offTime == 0)) {
	return;
    }
    if (legdPtr->flags & ACTIVE) {
	int time;

	legdPtr->cursorOn ^= 1;
	time = (legdPtr->cursorOn) ? legdPtr->onTime : legdPtr->offTime;
	legdPtr->timerToken = Tcl_CreateTimerHandler(time, BlinkCursorProc, 
		graphPtr);
	Blt_Legend_EventuallyRedraw(graphPtr);
    }
}

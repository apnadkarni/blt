/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrLegend.c --
 *
 * This module implements the legend for the BLT graph widget.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
  #include <string.h>
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
 *      SELECT_EXPORT           Export the selection to X11.
 *
 *      SELECT_PENDING          A "selection" command idle task is pending.
 *
 *      SELECT_CLEAR            Clear selection flag of entry.
 *
 *      SELECT_SET              Set selection flag of entry.
 *
 *      SELECT_TOGGLE           Toggle selection flag of entry.
 *
 *      SELECT_MASK             Mask of selection set/clear/toggle flags.
 *
 *      SELECT_SORTED           Indicates if the entries in the selection 
 *                              should be sorted or displayed in the order 
 *                              they were selected.
 *
 */

#define SELECT_CLEAR            (1<<16)
#define SELECT_EXPORT           (1<<17) 
#define SELECT_PENDING          (1<<18)
#define SELECT_SET              (1<<19)
#define SELECT_TOGGLE           (SELECT_SET | SELECT_CLEAR)
#define SELECT_MASK             (SELECT_SET | SELECT_CLEAR)
#define SELECT_SORTED           (1<<20)

#define RAISED                  (1<<21)
#define LEGEND_PENDING          (1<<22)

#define SELECT_MODE_SINGLE      (1<<0)
#define SELECT_MODE_MULTIPLE    (1<<1)

/*
 * Legend --
 *
 *      Contains information specific to how the legend will be displayed.
 */
struct _Legend {
    GraphObj obj;                       /* Must be first field in
                                         * marker. */
    unsigned int flags;
    int numEntries;                     /* # of element entries in
                                         * table. */
    short int numColumns, numRows;      /* # of columns and rows in
                                         * legend */
    short int width, height;            /* Dimensions of the legend */
    short int entryWidth, entryHeight;
    int site;
    short int xReq, yReq;               /* User-requested site of legend,
                                         * not the actual final
                                         * position. Used in conjunction
                                         * with the anchor below to
                                         * determine location of the
                                         * legend. */
    Tk_Anchor anchor;                   /* Anchor of legend. Used to
                                         * interpret the positioning point
                                         * of the legend in the graph*/
    int x, y;                           /* Computed origin of legend. */
    Tcl_Command cmdToken;               /* Token for graph's widget
                                         *command. */
    int reqColumns, reqRows;

    Blt_Pad iPadX, iPadY;               /* # of pixels interior padding
                                         * around legend entries */
    Blt_Pad padX, padY;                 /* # of pixels padding to exterior
                                         * of legend */
    Tk_Window tkwin;                    /* If non-NULL, external window to
                                         * draw legend. */
    TextStyle style;
    int maxSymSize;                     /* Size of largest symbol to be
                                         * displayed.  Used to calculate
                                         * size of legend */
    XColor *fgColor;
    Blt_Bg activeBg;                    /* Active legend entry background
                                         * color. */
    XColor *activeFgColor;
    int activeRelief;                   /* 3-D effect on active entry. */
    int entryBorderWidth;               /* Border width around each entry
                                         * in legend. */
    Blt_Bg normalBg;                    /* 3-D effect of legend. */
    int borderWidth;                    /* Width of legend 3-D border */
    int relief;                         /* 3-d effect of border around the
                                         * legend: TK_RELIEF_RAISED etc. */
    Blt_BindTable bindTable;
    int selRelief;
    int selBW;
    XColor *selInFocusFgColor;          /* Text color of a selected entry. */
    XColor *selOutFocusFgColor;
    Blt_Bg selInFocusBg;
    Blt_Bg selOutFocusBg;
    XColor *focusColor;
    Blt_Dashes focusDashes;             /* Dash on-off value. */
    GC focusGC;                         /* Graphics context for the active
                                         * label. */
    const char *takeFocus;
    int focus;                          /* Position of the focus entry. */
    int cursorX, cursorY;               /* Position of the insertion cursor in
                                         * the textbox window. */
    short int cursorWidth;              /* Size of the insertion cursor
                                         * symbol. */
    short int cursorHeight;
    Element *focusPtr;                  /* Element that currently has the
                                         * focus. If NULL, no legend entry has
                                         * the focus. */
    Element *selAnchorPtr;              /* Fixed end of selection. Used to
                                         * extend the selection while
                                         * maintaining the other end of the
                                         * selection. */
    Element *selMarkPtr;
    Element *selFirstPtr;               /* First element selected in
                                         * current pick. */
    Element *selLastPtr;                /* Last element selected in current
                                         * pick. */
    Element *activePtr;                 /* Element whose label is active in
                                         * the legend. */
    Tcl_Obj *selCmdObjPtr;              /* TCL script that's invoked
                                         * whenever the selection
                                         * changes. */
    int selMode;                        /* Mode of selection: single or
                                         * multiple. */
    Blt_HashTable selTable;             /* Table of selected elements. Used
                                         * to quickly determine whether an
                                         * element is selected. */
    Blt_Chain selected;                 /* List of selected elements. */
    const char *title;
    short int titleWidth, titleHeight;
    TextStyle titleStyle;               /* Legend title attributes */
    Tcl_Obj *cmdObjPtr;                 /* Specifies a TCL script to be
                                         * invoked whenever the legend has
                                         * changed. This is used to keep
                                         * and external legend in sync with
                                         * the graph. */
};

#define LABEL_PAD       2

#define DEF_ACTIVEBACKGROUND    RGB_SKYBLUE4
#define DEF_ACTIVEBORDERWIDTH    "2"
#define DEF_ACTIVEFOREGROUND    RGB_WHITE
#define DEF_ACTIVERELIEF        "flat"
#define DEF_ANCHOR              "n"
#define DEF_BACKGROUND          (char *)NULL
#define DEF_BORDERWIDTH         STD_BORDERWIDTH
#define DEF_COMMAND             (char *)NULL
#define DEF_COLUMNS             "0"
#define DEF_EXPORTSELECTION     "no"
#define DEF_FONT                "{Sans Serif} 8"
#define DEF_FOREGROUND          STD_NORMAL_FOREGROUND
#define DEF_HIDE                "no"
#define DEF_IPADX               "1"
#define DEF_IPADY               "1"
#define DEF_PADX                "1"
#define DEF_PADY                "1"
#define DEF_POSITION            "right"
#define DEF_RAISED              "no"
#define DEF_RELIEF              "flat"
#define DEF_ROWS                "0"
#define DEF_SELECTBACKGROUND    RGB_SKYBLUE4
#define DEF_SELECT_BG_MONO      STD_SELECT_BG_MONO
#define DEF_SELECTBORDERWIDTH   "1"
#define DEF_SELECTMODE          "multiple"
#define DEF_SELECT_FG_MONO      STD_SELECT_FG_MONO
#define DEF_SELECTFOREGROUND    RGB_WHITE /*STD_SELECT_FOREGROUND*/
#define DEF_SELECTRELIEF        "flat"
#define DEF_FOCUSDASHES         "dot"
#define DEF_FOCUSEDIT           "no"
#define DEF_FOCUSFOREGROUND     STD_ACTIVE_FOREGROUND
#define DEF_FOCUS_FG_MONO       STD_ACTIVE_FG_MONO
#define DEF_TAKEFOCUS           "1"
#define DEF_TITLE               (char *)NULL
#define DEF_TITLECOLOR          STD_NORMAL_FOREGROUND
#define DEF_TITLEFONT           "{Sans Serif} 8 bold"

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
        Blt_Offset(Legend, entryBorderWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground",
        "ActiveForeground", DEF_ACTIVEFOREGROUND,
        Blt_Offset(Legend, activeFgColor), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "Relief",
        DEF_ACTIVERELIEF, Blt_Offset(Legend, activeRelief),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_ANCHOR, 
        Blt_Offset(Legend, anchor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BACKGROUND, Blt_Offset(Legend, normalBg),BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BORDERWIDTH, Blt_Offset(Legend, borderWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
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
    {BLT_CONFIG_SYNONYM, "-fg", "foreground"},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
        DEF_FOREGROUND, Blt_Offset(Legend, fgColor), 0},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_HIDE, 
        Blt_Offset(Legend, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_PAD, "-ipadx", "iPadX", "Pad", DEF_IPADX, 
        Blt_Offset(Legend, iPadX), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-ipady", "iPadY", "Pad", DEF_IPADY, 
        Blt_Offset(Legend, iPadY), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-nofocusselectbackground", 
        "noFocusSelectBackground", "NoFocusSelectBackground", 
        DEF_SELECTBACKGROUND, Blt_Offset(Legend, selOutFocusBg), 0},
    {BLT_CONFIG_COLOR, "-nofocusselectforeground", "noFocusSelectForeground", 
        "NoFocusSelectForeground", DEF_SELECTFOREGROUND, 
        Blt_Offset(Legend, selOutFocusFgColor), 0},
    {BLT_CONFIG_PAD, "-padx", "padX", "Pad", DEF_PADX, 
        Blt_Offset(Legend, padX), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-pady", "padY", "Pad", DEF_PADY, 
        Blt_Offset(Legend, padY), BLT_CONFIG_DONT_SET_DEFAULT},
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

typedef struct {
    unsigned int flags;
} BBoxSwitches;

#define BBOX_ROOT     (1<<0)

static Blt_SwitchSpec bboxSwitches[] = 
{
    {BLT_SWITCH_BITS_NOARG, "-root", "", (char *)NULL,
        Blt_Offset(BBoxSwitches, flags), 0, BBOX_ROOT},
    {BLT_SWITCH_END}
};

static Tcl_IdleProc DisplayProc;
static Blt_BindPickProc PickEntryProc;
static Tk_EventProc LegendEventProc;
static Tk_LostSelProc LostSelectionProc;
static Tk_SelectionProc SelectionProc;

BLT_EXTERN Tcl_ObjCmdProc Blt_GraphInstCmdProc;

/*
 *---------------------------------------------------------------------------
 *
 * SelectCmdProc --
 *
 *      Invoked at the next idle point whenever the current selection
 *      changes.  Executes some application-specific code in the
 *      -selectcommand option.  This provides a way for applications to
 *      handle selection changes.
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
    Legend *legendPtr = clientData;

    legendPtr->flags &= ~LEGEND_PENDING;
    if (legendPtr->cmdObjPtr != NULL) {
        Tcl_Interp *interp;

        Tcl_Preserve(legendPtr);
        interp = legendPtr->obj.graphPtr->interp;
        if (Tcl_EvalObjEx(interp, legendPtr->cmdObjPtr, TCL_EVAL_GLOBAL) 
            != TCL_OK) {
            Tcl_BackgroundError(interp);
        }
        Tcl_Release(legendPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyInvokeChangeCmd --
 *
 *      Queues a request to execute the -command code associated with the
 *      widget at the next idle point.  Invoked whenever the legend
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
EventuallyInvokeChangeCmd(Legend *legendPtr)
{
    if ((legendPtr->flags & LEGEND_PENDING) == 0) {
        legendPtr->flags |= LEGEND_PENDING;
        Tcl_DoWhenIdle(LegendChangedProc, legendPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Legend_EventuallyRedraw --
 *
 *      Tells the Tk dispatcher to call the graph display routine at the
 *      next idle point.  This request is made only if the window is
 *      displayed and no other redraw request is pending.
 *
 * Results: None.
 *
 * Side effects:
 *      The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Legend_EventuallyRedraw(Graph *graphPtr) 
{
    Legend *legendPtr = graphPtr->legend;

    if (legendPtr->cmdObjPtr != NULL) {
        EventuallyInvokeChangeCmd(legendPtr);
    }
    if ((legendPtr->tkwin != NULL) && !(legendPtr->flags & REDRAW_PENDING)) {
        Tcl_DoWhenIdle(DisplayProc, legendPtr);
        legendPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectCmdProc --
 *
 *      Invoked at the next idle point whenever the current selection
 *      changes.  Executes some application-specific code in the
 *      -selectcommand option.  This provides a way for applications to
 *      handle selection changes.
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
    Legend *legendPtr = clientData;

    legendPtr->flags &= ~SELECT_PENDING;
    if (legendPtr->selCmdObjPtr != NULL) {
        Tcl_Interp *interp;

        Tcl_Preserve(legendPtr);
        interp = legendPtr->obj.graphPtr->interp;
        if (Tcl_EvalObjEx(interp, legendPtr->selCmdObjPtr, TCL_EVAL_GLOBAL) 
            != TCL_OK) {
            Tcl_BackgroundError(interp);
        }
        Tcl_Release(legendPtr);
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
EventuallyInvokeSelectCmd(Legend *legendPtr)
{
    if ((legendPtr->flags & SELECT_PENDING) == 0) {
        legendPtr->flags |= SELECT_PENDING;
        Tcl_DoWhenIdle(SelectCmdProc, legendPtr);
    }
}

static void
ClearSelection(Legend *legendPtr)
{
    Blt_DeleteHashTable(&legendPtr->selTable);
    Blt_InitHashTable(&legendPtr->selTable, BLT_ONE_WORD_KEYS);
    Blt_Chain_Reset(legendPtr->selected);
    Blt_Legend_EventuallyRedraw(legendPtr->obj.graphPtr);
    if (legendPtr->selCmdObjPtr != NULL) {
        EventuallyInvokeSelectCmd(legendPtr);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * LostSelectionProc --
 *
 *      This procedure is called back by Tk when the selection is grabbed away
 *      from a Text widget.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The existing selection is unhighlighted, and the window is marked as
 *      not containing a selection.
 *
 *---------------------------------------------------------------------------
 */
static void
LostSelectionProc(ClientData clientData)
{
    Legend *legendPtr = clientData;

    if (legendPtr->flags & SELECT_EXPORT) {
        ClearSelection(legendPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * LegendEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various events on
 *      graphs.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      When the window gets deleted, internal structures get cleaned up.
 *      When it gets exposed, the graph is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
LegendEventProc(ClientData clientData, XEvent *eventPtr)
{
    Graph *graphPtr = clientData;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            Blt_Legend_EventuallyRedraw(graphPtr);
        }
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
        if (eventPtr->xfocus.detail == NotifyInferior) {
            return;
        }
        if (eventPtr->type == FocusIn) {
            legendPtr->flags |= FOCUS;
        } else {
            legendPtr->flags &= ~FOCUS;
        }
        Blt_Legend_EventuallyRedraw(graphPtr);
    } else if (eventPtr->type == DestroyNotify) {
        Graph *graphPtr = legendPtr->obj.graphPtr;

        if (legendPtr->site == LEGEND_WINDOW) {
            Blt_DeleteWindowInstanceData(legendPtr->tkwin);
            if (legendPtr->cmdToken != NULL) {
                Tcl_DeleteCommandFromToken(graphPtr->interp, 
                                           legendPtr->cmdToken);
                legendPtr->cmdToken = NULL;
            }
            legendPtr->tkwin = graphPtr->tkwin;
        }
        if (legendPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayProc, legendPtr);
            legendPtr->flags &= ~REDRAW_PENDING;
        }
        if (legendPtr->flags & LEGEND_PENDING) {
            Tcl_CancelIdleCall(LegendChangedProc, legendPtr);
            legendPtr->flags &= ~LEGEND_PENDING;
        }
        legendPtr->site = LEGEND_RIGHT;
        legendPtr->flags |= HIDDEN;
        graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
        Blt_MoveBindingTable(legendPtr->bindTable, graphPtr->tkwin);
        Blt_EventuallyRedrawGraph(graphPtr);
    } else if (eventPtr->type == ConfigureNotify) {
        Blt_Legend_EventuallyRedraw(graphPtr);
    }
}

static int
CreateLegendWindow(Tcl_Interp *interp, Legend *legendPtr, const char *pathName)
{
    Graph *graphPtr;
    Tk_Window tkwin;

    graphPtr = legendPtr->obj.graphPtr;
    tkwin = Tk_CreateWindowFromPath(interp, graphPtr->tkwin, pathName, NULL);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    Blt_SetWindowInstanceData(tkwin, legendPtr);
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
          LegendEventProc, graphPtr);
    /* Move the legend's binding table to the new window. */
    Blt_MoveBindingTable(legendPtr->bindTable, tkwin);
    if (legendPtr->tkwin != graphPtr->tkwin) {
        Tk_DestroyWindow(legendPtr->tkwin);
    }
    /* Create a command by the same name as the legend window so that Legend
     * bindings can use %W interchangably.  */
    legendPtr->cmdToken = Tcl_CreateObjCommand(interp, pathName, 
        Blt_GraphInstCmdProc, graphPtr, NULL);
    legendPtr->tkwin = tkwin;
    legendPtr->site = LEGEND_WINDOW;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPosition --
 *
 *      Convert the string representation of a legend XY position into
 *      window coordinates.  The form of the string must be "@x,y" or none.
 *
 * Results:
 *      The return value is a standard TCL result.  The symbol type is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPosition(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Graph *graphPtr;
    Legend *legendPtr = (Legend *)widgRec;
    char c;
    int length;
    const char *string;

    graphPtr = legendPtr->obj.graphPtr;
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if (c == '\0') {
        legendPtr->site = LEGEND_RIGHT;
    } else if ((c == 'l') && (strncmp(string, "left", length) == 0)) {
        legendPtr->site = LEGEND_LEFT;
    } else if ((c == 'r') && (strncmp(string, "right", length) == 0)) {
        legendPtr->site = LEGEND_RIGHT;
    } else if ((c == 't') && (strncmp(string, "top", length) == 0)) {
        legendPtr->site = LEGEND_TOP;
    } else if ((c == 'b') && (strncmp(string, "bottom", length) == 0)) {
        legendPtr->site = LEGEND_BOTTOM;
    } else if ((c == 'p') && (strncmp(string, "plotarea", length) == 0)) {
        legendPtr->site = LEGEND_PLOT;
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
        legendPtr->xReq = (short int)x;
        legendPtr->yReq = (short int)y;
        legendPtr->site = LEGEND_XY;
    } else if (c == '.') {
        if (CreateLegendWindow(interp, legendPtr, string) != TCL_OK) {
            return TCL_ERROR;
        }
        Blt_Legend_EventuallyRedraw(graphPtr);
    } else {
        Tcl_AppendResult(interp, "bad position \"", string, "\": should be  "
                "left, right, top, bottom, plotarea, windowName or @x,y",
                (char *)NULL);
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
 *      Convert the window coordinates into a string.
 *
 * Results:
 *      The string representing the coordinate position is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PositionToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              char *widgRec, int offset, int flags)
{
    Legend *legendPtr = (Legend *)widgRec;
    Tcl_Obj *objPtr;

    switch (legendPtr->site) {
    case LEGEND_LEFT:
        objPtr = Tcl_NewStringObj("left", 4);
        break;

    case LEGEND_RIGHT:
        objPtr = Tcl_NewStringObj("right", 5);
        break;

    case LEGEND_TOP:
        objPtr = Tcl_NewStringObj("top", 3);
        break;

    case LEGEND_BOTTOM:
        objPtr = Tcl_NewStringObj("bottom", 6);
        break;

    case LEGEND_PLOT:
        objPtr = Tcl_NewStringObj("plotarea", 8);
        break;

    case LEGEND_WINDOW:
        objPtr = Tcl_NewStringObj(Tk_PathName(legendPtr->tkwin), -1);
        break;

    case LEGEND_XY:
        {
            char string[200];

            Blt_FormatString(string, 200, "@%d,%d", legendPtr->xReq, 
                             legendPtr->yReq);
            objPtr = Tcl_NewStringObj(string, -1);
        }
        break;

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
 *      Convert the string reprsenting a select mode, to its numeric form.
 *
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.
 *      Otherwise, TCL_ERROR is returned and an error message is left
 *      in interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToSelectmode(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    const char *string;
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
            "\": should be single or multiple", (char *)NULL);
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
 *      The string representation of the select mode is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SelectmodeToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                char *widgRec, int offset, int flags)  
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
SetLegendOrigin(Legend *legendPtr)
{
    Graph *graphPtr;
    int x, y, w, h;

    graphPtr = legendPtr->obj.graphPtr;
    x = y = w = h = 0;                  /* Suppress compiler warning. */
    switch (legendPtr->site) {
    case LEGEND_RIGHT:
        w = graphPtr->rightPtr->width -
            graphPtr->rightPtr->axesOffset;
        h = graphPtr->y2 - graphPtr->y1;
        x = graphPtr->x2 + graphPtr->rightPtr->axesOffset;
        y = graphPtr->y1;
        break;

    case LEGEND_LEFT:
        w = graphPtr->leftPtr->width - graphPtr->leftPtr->axesOffset;
        h = graphPtr->y2 - graphPtr->y1;
        x = graphPtr->inset;
        y = graphPtr->y1;
        break;

    case LEGEND_TOP:
        w = graphPtr->x2 - graphPtr->x1;
        h = graphPtr->topPtr->height - graphPtr->topPtr->axesOffset;
        if (graphPtr->title != NULL) {
            h -= graphPtr->titleHeight;
        }
        x = graphPtr->x1;
        y = graphPtr->inset;
        if (graphPtr->title != NULL) {
            y += graphPtr->titleHeight;
        }
        break;

    case LEGEND_BOTTOM:
        w = graphPtr->x2 - graphPtr->x1;
        h = graphPtr->bottomPtr->height - graphPtr->bottomPtr->axesOffset;
        x = graphPtr->x1;
        y = graphPtr->y2 + graphPtr->bottomPtr->axesOffset;
        break;

    case LEGEND_PLOT:
        w = graphPtr->x2 - graphPtr->x1;
        h = graphPtr->y2 - graphPtr->y1;
        x = graphPtr->x1;
        y = graphPtr->y1;
        break;

    case LEGEND_XY:
        w = legendPtr->width;
        h = legendPtr->height;
        x = legendPtr->xReq;
        y = legendPtr->yReq;
        if (x < 0) {
            x += graphPtr->width;
        }
        if (y < 0) {
            y += graphPtr->height;
        }
        break;

    case LEGEND_WINDOW:
        legendPtr->anchor = TK_ANCHOR_NW;
        legendPtr->x = legendPtr->y = 0;
        return;
    }

    switch (legendPtr->anchor) {
    case TK_ANCHOR_NW:                  /* Upper left corner */
        break;
    case TK_ANCHOR_W:                   /* Left center */
        if (h > legendPtr->height) {
            y += (h - legendPtr->height) / 2;
        }
        break;
    case TK_ANCHOR_SW:                  /* Lower left corner */
        if (h > legendPtr->height) {
            y += (h - legendPtr->height);
        }
        break;
    case TK_ANCHOR_N:                   /* Top center */
        if (w > legendPtr->width) {
            x += (w - legendPtr->width) / 2;
        }
        break;
    case TK_ANCHOR_CENTER:              /* Center */
        if (h > legendPtr->height) {
            y += (h - legendPtr->height) / 2;
        }
        if (w > legendPtr->width) {
            x += (w - legendPtr->width) / 2;
        }
        break;
    case TK_ANCHOR_S:                   /* Bottom center */
        if (w > legendPtr->width) {
            x += (w - legendPtr->width) / 2;
        }
        if (h > legendPtr->height) {
            y += (h - legendPtr->height);
        }
        break;
    case TK_ANCHOR_NE:                  /* Upper right corner */
        if (w > legendPtr->width) {
            x += w - legendPtr->width;
        }
        break;
    case TK_ANCHOR_E:                   /* Right center */
        if (w > legendPtr->width) {
            x += w - legendPtr->width;
        }
        if (h > legendPtr->height) {
            y += (h - legendPtr->height) / 2;
        }
        break;
    case TK_ANCHOR_SE:          /* Lower right corner */
        if (w > legendPtr->width) {
            x += w - legendPtr->width;
        }
        if (h > legendPtr->height) {
            y += (h - legendPtr->height);
        }
        break;
    }
    legendPtr->x = x;
    legendPtr->y = y;
}

static int
EntryIsSelected(Legend *legendPtr, Element *elemPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&legendPtr->selTable, (char *)elemPtr);
    return (hPtr != NULL);
}

static void
SelectElement(Legend *legendPtr, Element *elemPtr)
{
    int isNew;
    Blt_HashEntry *hPtr;

    hPtr = Blt_CreateHashEntry(&legendPtr->selTable, (char *)elemPtr,&isNew);
    if (isNew) {
        Blt_ChainLink link;

        link = Blt_Chain_Append(legendPtr->selected, elemPtr);
        Blt_SetHashValue(hPtr, link);
    }
}

static void
DeselectElement(Legend *legendPtr, Element *elemPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&legendPtr->selTable, (char *)elemPtr);
    if (hPtr != NULL) {
        Blt_ChainLink link;

        link = Blt_GetHashValue(hPtr);
        Blt_Chain_DeleteLink(legendPtr->selected, link);
        Blt_DeleteHashEntry(&legendPtr->selTable, hPtr);
    }
}

static void
SelectEntry(Legend *legendPtr, Element *elemPtr)
{
    Blt_HashEntry *hPtr;

    switch (legendPtr->flags & SELECT_MASK) {
    case SELECT_CLEAR:
        DeselectElement(legendPtr, elemPtr);
        break;

    case SELECT_SET:
        SelectElement(legendPtr, elemPtr);
        break;

    case SELECT_TOGGLE:
        hPtr = Blt_FindHashEntry(&legendPtr->selTable, (char *)elemPtr);
        if (hPtr != NULL) {
            DeselectElement(legendPtr, elemPtr);
        } else {
            SelectElement(legendPtr, elemPtr);
        }
        break;
    }
}

#ifdef notdef
static Element *
PointerToElement(Legend *legendPtr, int x, int y)
{
    Graph *graphPtr = legendPtr->obj.graphPtr;
    int w, h;
    int n;

    w = legendPtr->width;
    h = legendPtr->height;

    x -= legendPtr->x + legendPtr->borderWidth;
    y -= legendPtr->y + legendPtr->borderWidth;
    w -= 2 * legendPtr->borderWidth + PADDING(legendPtr->padX);
    h -= 2 * legendPtr->borderWidth + PADDING(legendPtr->padY);

    if ((x < 0) || (x >= w) || (y < 0) || (y >= h)) {
        return NULL;
    }

    /* It's in the bounding box, so compute the index. */
    {
        int row, column;

        row    = y / legendPtr->entryHeight;
        column = x / legendPtr->entryWidth;
        n = (column * legendPtr->numRows) + row;
    }
    if (n < legendPtr->numEntries) {
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
    Legend *legendPtr;
    int w, h;

    legendPtr = graphPtr->legend;
    w = legendPtr->width;
    h = legendPtr->height;

    if (legendPtr->titleHeight > 0) {
        y -= legendPtr->titleHeight + legendPtr->padY.side1;
    }
    x -= legendPtr->x + legendPtr->borderWidth;
    y -= legendPtr->y + legendPtr->borderWidth;
    w -= 2 * legendPtr->borderWidth + PADDING(legendPtr->padX);
    h -= 2 * legendPtr->borderWidth + PADDING(legendPtr->padY);

    if ((x >= 0) && (x < w) && (y >= 0) && (y < h)) {
        int row, column;
        int n;

        /*
         * It's in the bounding box, so compute the index.
         */
        row    = y / legendPtr->entryHeight;
        column = x / legendPtr->entryWidth;
        n = (column * legendPtr->numRows) + row;
        if (n < legendPtr->numEntries) {
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
 *      Calculates the dimensions (width and height) needed for the legend.
 *      Also determines the number of rows and columns necessary to list all
 *      the valid element labels.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The following fields of the legend are calculated and set.
 *
 *      numEntries   - number of valid labels of elements in the
 *                    display list.
 *      numRows     - number of rows of entries
 *      numColumns    - number of columns of entries
 *      entryHeight - height of each entry
 *      entryWidth  - width of each entry
 *      height      - width of legend (includes borders and padding)
 *      width       - height of legend (includes borders and padding)
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MapLegend(
    Graph *graphPtr,
    int plotWidth,                      /* Maximum width available in
                                         * window to draw the legend. Will
                                         * calculate # of columns from
                                         * this. */
    int plotHeight)                     /* Maximum height available in
                                         * window to draw the legend. Will
                                         * calculate # of rows from
                                         * this. */
{
    Legend *legendPtr = graphPtr->legend;
    Blt_ChainLink link;
    int numRows, numColumns, numEntries;
    int cavityWidth, cavityHeight;
    int maxEntryWidth, maxEntryHeight;
    int symbolWidth;
    Blt_FontMetrics fontMetrics;
    unsigned int tw, th;

    /* Initialize legend values to default (no legend displayed) */
    legendPtr->entryWidth = legendPtr->entryHeight = 0;
    legendPtr->numRows = legendPtr->numColumns = legendPtr->numEntries = 0;
    legendPtr->height = legendPtr->width = 0;

    if (legendPtr->site == LEGEND_WINDOW) {
        if (Tk_Width(legendPtr->tkwin) > 1) {
            plotWidth = Tk_Width(legendPtr->tkwin);
        }
        if (Tk_Height(legendPtr->tkwin) > 1) {
            plotHeight = Tk_Height(legendPtr->tkwin);
        }
    }
    Blt_Ts_GetExtents(&legendPtr->titleStyle, legendPtr->title, &tw, &th);
    legendPtr->titleWidth = tw;
    legendPtr->titleHeight = th;
    /*   
     * Count the number of legend entries and determine the widest and
     * tallest label.  The number of entries would normally be the number
     * of elements, but elements can have no legend entry (-label "").
     */
    numEntries = 0;
    maxEntryWidth = maxEntryHeight = 0;
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
        link != NULL; link = Blt_Chain_NextLink(link)) {
        unsigned int w, h;
        Element *elemPtr;

        elemPtr = Blt_Chain_GetValue(link);
        if (elemPtr->label == NULL) {
            continue;                   /* Element has no legend entry. */
        }
        Blt_Ts_GetExtents(&legendPtr->style, elemPtr->label, &w, &h);
        if (maxEntryWidth < w) {
            maxEntryWidth = w;
        }
        if (maxEntryHeight < h) {
            maxEntryHeight = h;
        }
        numEntries++;
    }
    if (numEntries == 0) {
        return;                         /* No visible legend entries. */
    }


    Blt_Font_GetMetrics(legendPtr->style.font, &fontMetrics);
    symbolWidth = 2 * fontMetrics.ascent;

    maxEntryWidth += 2 * legendPtr->entryBorderWidth +
        PADDING(legendPtr->iPadX) + symbolWidth + 3 * LABEL_PAD;
    maxEntryHeight += 2 * legendPtr->entryBorderWidth +
        PADDING(legendPtr->iPadY);

    /* Make entry width and height odd so that dotted lines mesh. */
    maxEntryWidth |= 0x01;              
    maxEntryHeight |= 0x01;

    cavityWidth  = plotWidth;
    cavityHeight = plotHeight;
    cavityWidth  -= 2 * legendPtr->borderWidth + PADDING(legendPtr->padX);
    cavityHeight -= 2 * legendPtr->borderWidth + PADDING(legendPtr->padY);

    /*
     * The number of rows and columns is computed as one of the following:
     *
     *  both options set                User defined. 
     *  -rows                           Compute columns from rows.
     *  -columns                        Compute rows from columns.
     *  neither set                     Compute rows and columns from
     *                                  size of plot.  
     */
    if (legendPtr->reqRows > 0) {
        numRows = MIN(legendPtr->reqRows, numEntries); 
        if (legendPtr->reqColumns > 0) {
            numColumns = MIN(legendPtr->reqColumns, numEntries);
        } else {
            numColumns = ((numEntries - 1) / numRows) + 1; /* Only -rows. */
        }
    } else if (legendPtr->reqColumns > 0) { /* Only -columns. */
        numColumns = MIN(legendPtr->reqColumns, numEntries);
        numRows = ((numEntries - 1) / numColumns) + 1;
    } else {                    
        /* Compute # of rows and columns from the legend size. */
        numRows = cavityHeight / maxEntryHeight;
        numColumns = cavityWidth / maxEntryWidth;
        if (numRows < 1) {
            numRows = numEntries;
        }
        if (numColumns < 1) {
            numColumns = numEntries;
        }
        if (numRows > numEntries) {
            numRows = numEntries;
        } 
        switch (legendPtr->site) {
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

    cavityHeight = (numRows * maxEntryHeight);
    if (legendPtr->titleHeight > 0) {
        cavityHeight += legendPtr->titleHeight + legendPtr->padY.side1;
    }
    cavityWidth = numColumns * maxEntryWidth;
    if (cavityWidth < legendPtr->titleWidth) {
        cavityWidth = legendPtr->titleWidth;
    }
    legendPtr->width = cavityWidth + 2 * legendPtr->borderWidth + 
        PADDING(legendPtr->padX);
    legendPtr->height = cavityHeight + 2 * legendPtr->borderWidth + 
        PADDING(legendPtr->padY);
    legendPtr->numRows     = numRows;
    legendPtr->numColumns  = numColumns;
    legendPtr->numEntries  = numEntries;
    legendPtr->entryHeight = maxEntryHeight;
    legendPtr->entryWidth  = maxEntryWidth;

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
    if ((legendPtr->site == LEGEND_WINDOW) &&
        ((Tk_ReqWidth(legendPtr->tkwin) != legendPtr->width) ||
         (Tk_ReqHeight(legendPtr->tkwin) != legendPtr->height))) {
        Tk_GeometryRequest(legendPtr->tkwin, legendPtr->width,
                legendPtr->height);
    }
}

void
Blt_DrawLegend(Graph *graphPtr, Drawable drawable)
{
    Blt_Bg bg;
    Blt_ChainLink link;
    Blt_FontMetrics fontMetrics;
    Legend *legendPtr = graphPtr->legend;
    Pixmap pixmap;
    Tk_Window tkwin;
    int count;
    int symbolSize, xMid, yMid;
    int x, y, w, h;
    int xLabel, yStart, xSymbol, ySymbol;

    if ((legendPtr->flags & HIDDEN) || (legendPtr->numEntries == 0)) {
        return;
    }
    SetLegendOrigin(legendPtr);
    graphPtr = legendPtr->obj.graphPtr;
    tkwin = legendPtr->tkwin;
    if (legendPtr->site == LEGEND_WINDOW) {
        w = Tk_Width(tkwin);
        h = Tk_Height(tkwin);
    } else {
        w = legendPtr->width;
        h = legendPtr->height;
    }

    pixmap = Blt_GetPixmap(graphPtr->display, Tk_WindowId(tkwin), w, h, 
        Tk_Depth(tkwin));

    if (legendPtr->normalBg != NULL) {
        Blt_Bg_FillRectangle(tkwin, pixmap, legendPtr->normalBg, 0, 0, 
                w, h, 0, TK_RELIEF_FLAT);
    } else if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
        /* 
         * Legend background is transparent and is positioned over the the
         * plot area.  Either copy the part of the background from the
         * backing store pixmap or (if no backing store exists) just fill
         * it with the background color of the plot.
         */
        if (graphPtr->cache != None) {
            XCopyArea(graphPtr->display, graphPtr->cache, pixmap, 
                graphPtr->drawGC, legendPtr->x, legendPtr->y, w, h, 0, 0);
        } else {
            Blt_Bg_FillRectangle(tkwin, pixmap, graphPtr->plotBg, 0, 0, 
                w, h, TK_RELIEF_FLAT, 0);
        }
    } else {
        int x0, y0;
        /* 
         * The legend is located in one of the margins or the external
         * window.
         */
        Blt_Bg_GetOrigin(graphPtr->normalBg, &x0, &y0);
        Blt_Bg_SetOrigin(graphPtr->tkwin, graphPtr->normalBg, 
                x0 + legendPtr->x, y0 + legendPtr->y);
        Blt_Bg_FillRectangle(tkwin, pixmap, graphPtr->normalBg, 0, 0, 
                w, h, 0, TK_RELIEF_FLAT);
        Blt_Bg_SetOrigin(tkwin, graphPtr->normalBg, x0, y0);
    }
    Blt_Font_GetMetrics(legendPtr->style.font, &fontMetrics);

    symbolSize = fontMetrics.ascent;
    xMid = symbolSize + 1 + legendPtr->entryBorderWidth;
    yMid = (symbolSize / 2) + 1 + legendPtr->entryBorderWidth;
    xLabel = 2 * symbolSize + legendPtr->entryBorderWidth + 
        legendPtr->iPadX.side1 + 2 * LABEL_PAD;
    ySymbol = yMid + legendPtr->iPadY.side1; 
    xSymbol = xMid + LABEL_PAD;

    x = legendPtr->padLeft + legendPtr->borderWidth;
    y = legendPtr->padTop + legendPtr->borderWidth;
    Blt_DrawText(tkwin, pixmap, legendPtr->title, &legendPtr->titleStyle, x, y);
    if (legendPtr->titleHeight > 0) {
        y += legendPtr->titleHeight + legendPtr->padY.side1;
    }
    count = 0;
    yStart = y;
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
         link != NULL; link = Blt_Chain_NextLink(link)) {
        Element *elemPtr;
        int isSelected;

        elemPtr = Blt_Chain_GetValue(link);
        if (elemPtr->label == NULL) {
            continue;                   /* Skip this entry */
        }
        isSelected = EntryIsSelected(legendPtr, elemPtr);
        if (elemPtr == legendPtr->activePtr) {
            int x0, y0;

            Blt_Bg_GetOrigin(legendPtr->activeBg, &x0, &y0);
            Blt_Bg_SetOrigin(tkwin, legendPtr->activeBg, 
                x0 - legendPtr->x, y0 - legendPtr->y);
            Blt_Ts_SetForeground(legendPtr->style, legendPtr->activeFgColor);
            Blt_Bg_FillRectangle(tkwin, pixmap, legendPtr->activeBg, x, y, 
                legendPtr->entryWidth, legendPtr->entryHeight, 
                legendPtr->entryBorderWidth, legendPtr->activeRelief);
            Blt_Bg_SetOrigin(tkwin, legendPtr->activeBg, x0, y0);
        } else if (isSelected) {
            int x0, y0;
            Blt_Bg bg;
            XColor *fg;

            fg = (legendPtr->flags & FOCUS) ?
                legendPtr->selInFocusFgColor : legendPtr->selOutFocusFgColor;
            bg = (legendPtr->flags & FOCUS) ?
                    legendPtr->selInFocusBg : legendPtr->selOutFocusBg;
            Blt_Bg_GetOrigin(bg, &x0, &y0);
            Blt_Bg_SetOrigin(tkwin, bg, x0 - legendPtr->x, y0 - legendPtr->y);
            Blt_Ts_SetForeground(legendPtr->style, fg);
            Blt_Bg_FillRectangle(tkwin, pixmap, bg, x, y, 
                legendPtr->entryWidth, legendPtr->entryHeight, 
                legendPtr->selBW, legendPtr->selRelief);
            Blt_Bg_SetOrigin(tkwin, bg, x0, y0);
        } else {
            Blt_Ts_SetForeground(legendPtr->style, legendPtr->fgColor);
            if (elemPtr->legendRelief != TK_RELIEF_FLAT) {
                Blt_Bg_FillRectangle(tkwin, pixmap, graphPtr->normalBg, 
                        x, y, legendPtr->entryWidth, 
                        legendPtr->entryHeight, legendPtr->entryBorderWidth, 
                        elemPtr->legendRelief);
            }
        }
        (*elemPtr->procsPtr->drawSymbolProc) (graphPtr, pixmap, elemPtr,
                x + xSymbol, y + ySymbol, symbolSize);
        Blt_DrawText(tkwin, pixmap, elemPtr->label, &legendPtr->style, 
                x + xLabel, 
                y + legendPtr->entryBorderWidth + legendPtr->iPadY.side1);
        count++;
        if (legendPtr->focusPtr == elemPtr) { /* Focus outline */
            if (isSelected) {
                XColor *color;

                color = (legendPtr->flags & FOCUS) ?
                    legendPtr->selInFocusFgColor :
                    legendPtr->selOutFocusFgColor;
                XSetForeground(graphPtr->display, legendPtr->focusGC, 
                               color->pixel);
            }
            XDrawRectangle(graphPtr->display, pixmap, legendPtr->focusGC, 
                x + 1, y + 1, legendPtr->entryWidth - 3, 
                legendPtr->entryHeight - 3);
            if (isSelected) {
                XSetForeground(graphPtr->display, legendPtr->focusGC, 
                        legendPtr->focusColor->pixel);
            }
        }
        /* Check when to move to the next column */
        if ((count % legendPtr->numRows) > 0) {
            y += legendPtr->entryHeight;
        } else {
            x += legendPtr->entryWidth;
            y = yStart;
        }
    }
    /*
     * Draw the border and/or background of the legend.
     */
    bg = legendPtr->normalBg;
    if (bg == NULL) {
        bg = graphPtr->normalBg;
    }
    /* Disable crosshairs before redisplaying to the screen */
    if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
        Blt_DisableCrosshairs(graphPtr);
    }
    if ((w > 0) && (h > 0)) {
        Blt_Bg_DrawRectangle(tkwin, pixmap, bg, 0, 0, w, h, 
                legendPtr->borderWidth, legendPtr->relief);
        XCopyArea(graphPtr->display, pixmap, drawable, graphPtr->drawGC, 0, 0,
                  w, h, legendPtr->x, legendPtr->y);
    }
    if (legendPtr->site & LEGEND_PLOTAREA_MASK) {
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
    Legend *legendPtr = graphPtr->legend;
    double x, y, yStart;
    int xLabel, xSymbol, ySymbol;
    int count;
    Blt_ChainLink link;
    int symbolSize, xMid, yMid;
    int width, height;
    Blt_FontMetrics fontMetrics;

    if ((legendPtr->flags & HIDDEN) || (legendPtr->numEntries == 0)) {
        return;
    }
    SetLegendOrigin(legendPtr);

    x = legendPtr->x, y = legendPtr->y;
    width = legendPtr->width - PADDING(legendPtr->padX);
    height = legendPtr->height - PADDING(legendPtr->padY);

    Blt_Ps_Append(ps, "% Legend\n");
    graphPtr = legendPtr->obj.graphPtr;
    if (graphPtr->pageSetup->flags & PS_DECORATIONS) {
        if (legendPtr->normalBg != NULL) {
            Tk_3DBorder border;

            border = Blt_Bg_Border(legendPtr->normalBg);
            Blt_Ps_Fill3DRectangle(ps, border, x, y, width, height, 
                legendPtr->borderWidth, legendPtr->relief);
        } else {
            Tk_3DBorder border;

            border = Blt_Bg_Border(graphPtr->normalBg);
            Blt_Ps_Draw3DRectangle(ps, border, x, y, width, height, 
                legendPtr->borderWidth, legendPtr->relief);
        }
    } else {
        Blt_Ps_SetClearBackground(ps);
        Blt_Ps_XFillRectangle(ps, x, y, width, height);
    }
    Blt_Font_GetMetrics(legendPtr->style.font, &fontMetrics);
    symbolSize = fontMetrics.ascent;
    xMid = symbolSize + 1 + legendPtr->entryBorderWidth;
    yMid = (symbolSize / 2) + 1 + legendPtr->entryBorderWidth;
    xLabel = 2 * symbolSize + legendPtr->entryBorderWidth +
        legendPtr->iPadX.side1 + 5;
    xSymbol = xMid + legendPtr->iPadX.side1;
    ySymbol = yMid + legendPtr->iPadY.side1;

    x += legendPtr->borderWidth;
    y += legendPtr->borderWidth;
    Blt_Ps_DrawText(ps, legendPtr->title, &legendPtr->titleStyle, x, y);
    if (legendPtr->titleHeight > 0) {
        y += legendPtr->titleHeight + legendPtr->padY.side1;
    }

    count = 0;
    yStart = y;
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
        link != NULL; link = Blt_Chain_NextLink(link)) {
        Element *elemPtr;

        elemPtr = Blt_Chain_GetValue(link);
        if (elemPtr->label == NULL) {
            continue;                   /* Skip this label */
        }
        if (elemPtr == legendPtr->activePtr) {
            Tk_3DBorder border;
            
            border = Blt_Bg_Border(legendPtr->activeBg);
            Blt_Ts_SetForeground(legendPtr->style, legendPtr->activeFgColor);
            Blt_Ps_Fill3DRectangle(ps, border, x, y, legendPtr->entryWidth, 
                legendPtr->entryHeight, legendPtr->entryBorderWidth, 
                legendPtr->activeRelief);
        } else {
            Blt_Ts_SetForeground(legendPtr->style, legendPtr->fgColor);
            if (elemPtr->legendRelief != TK_RELIEF_FLAT) {
                Tk_3DBorder border;

                border = Blt_Bg_Border(graphPtr->normalBg);
                Blt_Ps_Draw3DRectangle(ps, border, x, y, legendPtr->entryWidth,
                        legendPtr->entryHeight, legendPtr->entryBorderWidth, 
                        elemPtr->legendRelief);
            }
        }
        (*elemPtr->procsPtr->printSymbolProc) (graphPtr, ps, elemPtr, 
                x + xSymbol, y + ySymbol, symbolSize);
        Blt_Ps_DrawText(ps, elemPtr->label, &legendPtr->style, x + xLabel,
                y + legendPtr->entryBorderWidth + legendPtr->iPadY.side1);
        count++;
        if ((count % legendPtr->numRows) > 0) {
            y += legendPtr->entryHeight;
        } else {
            x += legendPtr->entryWidth;
            y = yStart;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayProc --
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayProc(ClientData clientData)
{
    Legend *legendPtr = clientData;
    Graph *graphPtr;

    legendPtr->flags &= ~REDRAW_PENDING;
    if (legendPtr->tkwin == NULL) {
        return;                         /* Window has been destroyed. */
    }
    graphPtr = legendPtr->obj.graphPtr;
    if (legendPtr->site == LEGEND_WINDOW) {
        int w, h;

        w = Tk_Width(legendPtr->tkwin);
        h = Tk_Height(legendPtr->tkwin);
        if ((w != legendPtr->width) || (h != legendPtr->height)) {
            Blt_MapLegend(graphPtr, w, h);
        }
    }
    if (Tk_IsMapped(legendPtr->tkwin)) {
        Blt_DrawLegend(graphPtr, Tk_WindowId(legendPtr->tkwin));
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ConfigureLegend --
 *
 *      Routine to configure the legend.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ConfigureLegend(Graph *graphPtr)
{
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    /* GC for active label. Dashed outline. */
    gcMask = GCForeground | GCLineStyle;
    gcValues.foreground = legendPtr->focusColor->pixel;
    gcValues.line_style = (LineIsDashed(legendPtr->focusDashes))
        ? LineOnOffDash : LineSolid;
    newGC = Blt_GetPrivateGC(legendPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(legendPtr->focusDashes)) {
        legendPtr->focusDashes.offset = 2;
        Blt_SetDashes(graphPtr->display, newGC, &legendPtr->focusDashes);
    }
    if (legendPtr->focusGC != NULL) {
        Blt_FreePrivateGC(graphPtr->display, legendPtr->focusGC);
    }
    legendPtr->focusGC = newGC;
    
    if (legendPtr->cmdObjPtr != NULL) {
        EventuallyInvokeChangeCmd(legendPtr);
    }
    /*
     *  Update the layout of the graph (and redraw the elements) if any of
     *  the following legend options (all of which affect the size of the
     *  legend) have changed.
     *
     *          -activeborderwidth, -borderwidth
     *          -border
     *          -font
     *          -hide
     *          -ipadx, -ipady, -padx, -pady
     *          -rows
     *
     *  If the position of the legend changed to/from the default
     *  position, also indicate that a new layout is needed.
     *
     */
    if (legendPtr->site == LEGEND_WINDOW) {
        Blt_Legend_EventuallyRedraw(graphPtr);
    } else if (Blt_ConfigModified(configSpecs, "-*border*", "-*pad?",
        "-hide", "-font", "-rows", "-*background", "-*foreground",
        "-*relief", 
        (char *)NULL)) {
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
 *      None.
 *
 * Side effects:
 *      Resources associated with the legend are freed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DestroyLegend(Graph *graphPtr)
{
    Legend *legendPtr = graphPtr->legend;

    if (graphPtr->legend == NULL) {
        return;
    }

    Blt_FreeOptions(configSpecs, (char *)legendPtr, graphPtr->display, 0);
    Blt_Ts_FreeStyle(graphPtr->display, &legendPtr->style);
    Blt_Ts_FreeStyle(graphPtr->display, &legendPtr->titleStyle);
    Blt_DestroyBindingTable(legendPtr->bindTable);
    
    if (legendPtr->focusGC != NULL) {
        Blt_FreePrivateGC(graphPtr->display, legendPtr->focusGC);
    }
    if (legendPtr->tkwin != NULL) {
        Tk_DeleteSelHandler(legendPtr->tkwin, XA_PRIMARY, XA_STRING);
    }
    if (legendPtr->selected != NULL) {
        Blt_Chain_Destroy(legendPtr->selected);
    }
    if (legendPtr->site == LEGEND_WINDOW) {
        Tk_Window tkwin;
        
        /* The graph may be in the process of being torn down */
        if (legendPtr->cmdToken != NULL) {
            Tcl_DeleteCommandFromToken(graphPtr->interp, legendPtr->cmdToken);
        }
        if (legendPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayProc, legendPtr);
            legendPtr->flags &= ~REDRAW_PENDING;
        }
        tkwin = legendPtr->tkwin;
        legendPtr->tkwin = NULL;
        if (tkwin != NULL) {
            Tk_DeleteEventHandler(tkwin, ExposureMask | StructureNotifyMask,
                LegendEventProc, graphPtr);
            Blt_DeleteWindowInstanceData(tkwin);
            Tk_DestroyWindow(tkwin);
        }
    }
    if (legendPtr->flags & LEGEND_PENDING) {
        Tcl_CancelIdleCall(LegendChangedProc, legendPtr);
        legendPtr->flags &= ~LEGEND_PENDING;
    }
    Blt_Free(legendPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreateLegend --
 *
 *      Creates and initializes a legend structure with default settings
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Blt_CreateLegend(Graph *graphPtr)
{
    Legend *legendPtr;

    legendPtr = Blt_AssertCalloc(1, sizeof(Legend));
    graphPtr->legend = legendPtr;
    legendPtr->obj.graphPtr = graphPtr;
    legendPtr->obj.className = "Legend";
    legendPtr->obj.classId = CID_LEGEND;
    legendPtr->tkwin = graphPtr->tkwin;
    legendPtr->xReq = legendPtr->yReq = -SHRT_MAX;
    legendPtr->relief = TK_RELIEF_SUNKEN;
    legendPtr->activeRelief = TK_RELIEF_FLAT;
    legendPtr->entryBorderWidth = 2;
    legendPtr->borderWidth = 2;
    legendPtr->iPadX.side1 = legendPtr->iPadX.side2 = 1;
    legendPtr->iPadY.side1 = legendPtr->iPadY.side2 = 1;
    legendPtr->padX.side1  = legendPtr->padX.side2  = 1;
    legendPtr->padY.side1  = legendPtr->padY.side2  = 1;
    legendPtr->anchor = TK_ANCHOR_N;
    legendPtr->site = LEGEND_RIGHT;
    legendPtr->selMode = SELECT_MODE_MULTIPLE;
    Blt_Ts_InitStyle(legendPtr->style);
    Blt_Ts_InitStyle(legendPtr->titleStyle);
    legendPtr->style.justify = TK_JUSTIFY_LEFT;
    legendPtr->style.anchor = TK_ANCHOR_NW;
    legendPtr->titleStyle.justify = TK_JUSTIFY_LEFT;
    legendPtr->titleStyle.anchor = TK_ANCHOR_NW;
    legendPtr->bindTable = Blt_CreateBindingTable(graphPtr->interp,
        graphPtr->tkwin, graphPtr, PickEntryProc, Blt_GraphTags);

    Blt_InitHashTable(&legendPtr->selTable, BLT_ONE_WORD_KEYS);
    legendPtr->selected = Blt_Chain_Create();
    Tk_CreateSelHandler(legendPtr->tkwin, XA_PRIMARY, XA_STRING, 
        SelectionProc, legendPtr, XA_STRING);
    legendPtr->selRelief = TK_RELIEF_FLAT;
    legendPtr->selBW = 1;
    if (Blt_ConfigureComponentFromObj(graphPtr->interp, graphPtr->tkwin,
            "legend", "Legend", configSpecs, 0, (Tcl_Obj **)NULL,
            (char *)legendPtr, 0) != TCL_OK) {
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
            return elemPtr;             /* Don't go beyond legend
                                         * boundaries. */
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
 *      Parse an index into an entry and return either its value or an
 *      error.
 *
 * Results:
 *      A standard TCL result.  If all went well, then *indexPtr is filled
 *      in with the character index (into entryPtr) corresponding to
 *      string.  The index value is guaranteed to lie between 0 and the
 *      number of characters in the string, inclusive.  If an error occurs
 *      then an error message is left in the interp's result.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
GetElementFromObj(Graph *graphPtr, Tcl_Obj *objPtr, Element **elemPtrPtr)
{
    Element *elemPtr;
    Legend *legendPtr;
    Tcl_Interp *interp;
    char c;
    const char *string;

    legendPtr = graphPtr->legend;
    interp = graphPtr->interp;
    string = Tcl_GetString(objPtr);
    c = string[0];
    elemPtr = NULL;

    if ((c == 'a') && (strcmp(string, "anchor") == 0)) {
        elemPtr = legendPtr->selAnchorPtr;
    } else if ((c == 'c') && (strcmp(string, "current") == 0)) {
        GraphObj *objPtr;

        objPtr = Blt_GetCurrentItem(legendPtr->bindTable);
        if ((objPtr != NULL) && (!objPtr->deleted)) {
            elemPtr = (Element *)objPtr;
        }
    } else if ((c == 'f') && (strcmp(string, "first") == 0)) {
        elemPtr = GetFirstElement(graphPtr);
    } else if ((c == 'f') && (strcmp(string, "focus") == 0)) {
        elemPtr = legendPtr->focusPtr;
    } else if ((c == 'l') && (strcmp(string, "last") == 0)) {
        elemPtr = GetLastElement(graphPtr);
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
        elemPtr = GetLastElement(graphPtr);
    } else if ((c == 'n') && (strcmp(string, "next.row") == 0)) {
        elemPtr = GetNextRow(graphPtr, legendPtr->focusPtr);
    } else if ((c == 'n') && (strcmp(string, "next.column") == 0)) {
        elemPtr = GetNextColumn(graphPtr, legendPtr->focusPtr);
    } else if ((c == 'p') && (strcmp(string, "previous.row") == 0)) {
        elemPtr = GetPreviousRow(graphPtr, legendPtr->focusPtr);
    } else if ((c == 'p') && (strcmp(string, "previous.column") == 0)) {
        elemPtr = GetPreviousColumn(graphPtr, legendPtr->focusPtr);
    } else if ((c == 's') && (strcmp(string, "sel.first") == 0)) {
        elemPtr = legendPtr->selFirstPtr;
    } else if ((c == 's') && (strcmp(string, "sel.last") == 0)) {
        elemPtr = legendPtr->selLastPtr;
    } else if (c == '@') {
        int x, y;

        if (Blt_GetXY(interp, legendPtr->tkwin, string, &x, &y) != TCL_OK) {
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
 *      Sets the selection flag for a range of nodes.  The range is determined
 *      by two pointers which designate the first/last nodes of the range.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectRange(Legend *legendPtr, Element *fromPtr, Element *toPtr)
{

    if (Blt_Chain_IsBefore(fromPtr->link, toPtr->link)) {
        Blt_ChainLink link;

        for (link = fromPtr->link; link != NULL; 
             link = Blt_Chain_NextLink(link)) {
            Element *elemPtr;
            
            elemPtr = Blt_Chain_GetValue(link);
            SelectEntry(legendPtr, elemPtr);
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
            SelectEntry(legendPtr, elemPtr);
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
 *      Modify the selection by moving its un-anchored end.  This could make
 *      the selection either larger or smaller.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectText(Legend *legendPtr, Element *elemPtr)
{
    Element *firstPtr, *lastPtr;
    Graph *graphPtr = legendPtr->obj.graphPtr;

    /* Grab the selection if we don't own it already. */
    if ((legendPtr->flags&SELECT_EXPORT) && (legendPtr->selFirstPtr == NULL)) {
        Tk_OwnSelection(legendPtr->tkwin, XA_PRIMARY, LostSelectionProc, 
                legendPtr);
    }
    /* If the anchor hasn't been set, assume the beginning of the legend. */
    if (legendPtr->selAnchorPtr == NULL) {
        legendPtr->selAnchorPtr = GetFirstElement(graphPtr);
    }
    if (legendPtr->selAnchorPtr != elemPtr) {
        firstPtr = legendPtr->selAnchorPtr;
        lastPtr = elemPtr;
    } else {
        firstPtr = elemPtr;
        lastPtr = legendPtr->selAnchorPtr;
    }
    if ((legendPtr->selFirstPtr != firstPtr) || 
        (legendPtr->selLastPtr != lastPtr)) {
        legendPtr->selFirstPtr = firstPtr;
        legendPtr->selLastPtr = lastPtr;
        SelectRange(legendPtr, firstPtr, lastPtr);
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
 *      Activates a one label in the legend.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Graph will be redrawn.
 *
 *      pathName legend activate $elem
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    if (objc == 4) {
        Element *elemPtr;

        if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if ((elemPtr != NULL) && (elemPtr != legendPtr->activePtr)) {
            legendPtr->activePtr = elemPtr;
            if ((legendPtr->flags & HIDDEN) == 0) {
                if ((legendPtr->site != LEGEND_WINDOW) && 
                    (graphPtr->flags & REDRAW_PENDING)) {
                    graphPtr->flags |= CACHE_DIRTY;
                    graphPtr->flags |= REDRAW_WORLD; /* Redraw entire graph. */
                } else {
                    Blt_Legend_EventuallyRedraw(graphPtr);
                }
            }
        }
    }
    if (legendPtr->activePtr != NULL) {
        Tcl_SetStringObj(Tcl_GetObjResult(interp), 
                         legendPtr->activePtr->obj.name, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BBoxOp --
 *
 *      Returns the bounding box of the legend entry.
 *
 *      pathName legend bbox elemName ?switches?
 *
 *---------------------------------------------------------------------------
 */
static int
BBoxOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_FontMetrics fontMetrics;
    Element *elemPtr;
    Graph *graphPtr = clientData;
    Legend *legendPtr;
    Tcl_Obj *listObjPtr, *objPtr;
    int x, y, w, offset, symbolSize;
    BBoxSwitches switches;

    legendPtr = graphPtr->legend;
    if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Process switches  */
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, bboxSwitches, objc - 4, objv + 4, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    Blt_Font_GetMetrics(legendPtr->style.font, &fontMetrics);
    symbolSize = fontMetrics.ascent;
    x = legendPtr->padLeft + legendPtr->borderWidth;
    offset = 2 * symbolSize + legendPtr->entryBorderWidth +
        legendPtr->iPadX.side1 + 2 * LABEL_PAD;
    x += offset;
    w = legendPtr->entryWidth - offset;
    y = legendPtr->padTop + legendPtr->borderWidth;
    if (legendPtr->titleHeight > 0) {
        y += legendPtr->titleHeight + legendPtr->padY.side1;
    }
    y += elemPtr->row * legendPtr->entryHeight;
    x += elemPtr->col * w;

    if (switches.flags & BBOX_ROOT) {
        int rootX, rootY;

        Tk_GetRootCoords(graphPtr->tkwin, &rootX, &rootY);
        if (rootX < 0) {
            rootX = 0;
        }
        if (rootY < 0) {
            rootY = 0;
        }
        x += rootX;
        y += rootY;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    /* x */
    objPtr = Tcl_NewIntObj(x);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* y */
    objPtr = Tcl_NewIntObj(y);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* width */
    objPtr = Tcl_NewIntObj(x + w);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    /* height */
    objPtr = Tcl_NewIntObj(y + legendPtr->entryHeight);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BindOp --
 *
 *        pathName legend bind eventSequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;

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
 *      Queries or resets options for the legend.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
            (char *)graphPtr->legend, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      Queries or resets options for the legend.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;

    if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
                (char *)graphPtr->legend, NULL, BLT_CONFIG_OBJV_ONLY);
    } else if (objc == 4) {
        return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
                (char *)graphPtr->legend, objv[3], BLT_CONFIG_OBJV_ONLY);
    }
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
        objc - 3, objv + 3, (char *)graphPtr->legend, BLT_CONFIG_OBJV_ONLY)
        != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_ConfigureLegend(graphPtr);
    return TCL_OK;
}


/*ARGSUSED*/
static int
CurselectionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Legend *legendPtr;
    Tcl_Obj *listObjPtr;

    legendPtr = graphPtr->legend;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (legendPtr->flags & SELECT_SORTED) {
        Blt_ChainLink link;

        /* Return the sorted list of selected entries. */
        for (link = Blt_Chain_FirstLink(legendPtr->selected); link != NULL;
             link = Blt_Chain_NextLink(link)) {
            Element *elemPtr;
            Tcl_Obj *objPtr;

            elemPtr = Blt_Chain_GetValue(link);
            objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    } else {
        Blt_ChainLink link;

        /* Return a list of selected entries according to stacking
         * order. */
        for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
             link != NULL; link = Blt_Chain_NextLink(link)) {
            Element *elemPtr;

            elemPtr = Blt_Chain_GetValue(link);

            if (EntryIsSelected(legendPtr, elemPtr)) {
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
 *      Deactivates all labels in the legend.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Graph will be redrawn.
 *
 *      pathName legend deactivate 
 *
 *---------------------------------------------------------------------------
 */
static int
DeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    if (legendPtr->activePtr != NULL) {
        legendPtr->activePtr = NULL;
        if ((legendPtr->flags & HIDDEN) == 0) {
            if ((legendPtr->site != LEGEND_WINDOW) && 
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

/*
 *---------------------------------------------------------------------------
 *
 * FocusOp --
 *
 * Results:
 *      A standard TCL result.
 *
 *      pathName legend focus ?elemName?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FocusOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    if (objc == 4) {
        Element *elemPtr;

        if (GetElementFromObj(graphPtr, objv[3], &elemPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if ((elemPtr != NULL) && (elemPtr != legendPtr->focusPtr)) {
            /* Changing focus can only affect the visible entries.  The entry
             * layout stays the same. */
            legendPtr->focusPtr = elemPtr;
        }
        Blt_SetFocusItem(legendPtr->bindTable, legendPtr->focusPtr, 
                         CID_LEGEND_ENTRY);
        Blt_Legend_EventuallyRedraw(graphPtr);
    }
    if (legendPtr->focusPtr != NULL) {
        Tcl_SetStringObj(Tcl_GetObjResult(interp), 
                legendPtr->focusPtr->obj.name, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 * Results:
 *      A standard TCL result.
 *
 *      pathName legend get elemName
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GetOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    if (((legendPtr->flags & HIDDEN) == 0) && (legendPtr->numEntries > 0)) {
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
 *---------------------------------------------------------------------------
 *
 * IconOp --
 *
 *      Returns the image of the symbol icon for the given entry. The image
 *      is a Tk photo or picture.  This can be used to build your own
 *      legend.
 *
 *      pathName legend icon elemName imageName
 *
 *---------------------------------------------------------------------------
  */
/*ARGSUSED*/
static int
IconOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_FontMetrics fontMetrics;
    Blt_Picture picture;
    Element *elemPtr;
    Graph *graphPtr = clientData;
    Legend *legendPtr;
    Pixmap pixmap;
    Tk_PhotoHandle photo;
    const char *imageName;
    int isPicture;
    int w, h, x, y, s;

    legendPtr = graphPtr->legend;
    if (GetElementFromObj(graphPtr, objv[3], &elemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (elemPtr == NULL) {
        return TCL_OK;                  /* Unknown index. */
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
    Blt_Font_GetMetrics(legendPtr->style.font, &fontMetrics);
    s = fontMetrics.ascent;
    h = s + PADDING(legendPtr->iPadY) + 1;
    w = s + s + 1 + PADDING(legendPtr->iPadX);
    x = (w / 2);
    y = (h / 2);
    
    /* Draw the symbol into a pixmap and snap a picture of it */
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
    /* Make the background transparent. Not quite as good as compositing
     * it. */
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
        
        destRowPtr = Blt_Picture_Bits(picture);
        for (y = 0; y < h; y++) {
            Blt_Pixel *dp, *dend;

            for (dp = destRowPtr, dend = dp + w; dp < dend; dp++) {
                if (dp->u32 == bg.u32) {
                    dp->Alpha = 0x0;
                }
            }
            destRowPtr += Blt_Picture_Stride(picture);
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
 *      Sets the selection anchor to the element given by a index.  The
 *      selection anchor is the end of the selection that is fixed while
 *      dragging out a selection with the mouse.  The index "anchor" may be
 *      used to refer to the anchor element.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName legend selection anchor elemName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Element *elemPtr;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    if (GetElementFromObj(graphPtr, objv[4], &elemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Set both the anchor and the mark. Indicates that a single entry is
     * selected. */
    legendPtr->selAnchorPtr = elemPtr;
    legendPtr->selMarkPtr = NULL;
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
 *      Clears the entire selection.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName legend selection clearall
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionClearallOp(ClientData clientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;

    ClearSelection(graphPtr->legend);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionIncludesOp
 *
 *      Returns 1 if the element indicated by index is currently selected,
 *      0 if it isn't.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName legend selection includes elemName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionIncludesOp(ClientData clientData, Tcl_Interp *interp, int objc,
                    Tcl_Obj *const *objv)
{
    Element *elemPtr;
    Graph *graphPtr = clientData;
    int state;

    if (GetElementFromObj(graphPtr, objv[4], &elemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    state = EntryIsSelected(graphPtr->legend, elemPtr);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionMarkOp --
 *
 *      Sets the selection mark to the element given by a index.  The
 *      selection anchor is the end of the selection that is movable while
 *      dragging out a selection with the mouse.  The index "mark" may be
 *      used to refer to the anchor element.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName legend selection mark elemName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionMarkOp(ClientData clientData, Tcl_Interp *interp, int objc,
                Tcl_Obj *const *objv)
{
    Element *elemPtr;
    Graph *graphPtr = clientData;
    Legend *legendPtr;

    legendPtr = graphPtr->legend;
    if (GetElementFromObj(graphPtr, objv[4], &elemPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (legendPtr->selAnchorPtr == NULL) {
        Tcl_AppendResult(interp, "selection anchor must be set first", 
                 (char *)NULL);
        return TCL_ERROR;
    }
    if (legendPtr->selMarkPtr != elemPtr) {
        Blt_ChainLink link, next;

        /* Deselect entry from the list all the way back to the anchor. */
        for (link = Blt_Chain_LastLink(legendPtr->selected); link != NULL; 
             link = next) {
            Element *selectPtr;

            next = Blt_Chain_PrevLink(link);
            selectPtr = Blt_Chain_GetValue(link);
            if (selectPtr == legendPtr->selAnchorPtr) {
                break;
            }
            DeselectElement(legendPtr, selectPtr);
        }
        legendPtr->flags &= ~SELECT_MASK;
        legendPtr->flags |= SELECT_SET;
        SelectRange(legendPtr, legendPtr->selAnchorPtr, elemPtr);
        Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name, -1);
        legendPtr->selMarkPtr = elemPtr;

        Blt_Legend_EventuallyRedraw(graphPtr);
        if (legendPtr->selCmdObjPtr != NULL) {
            EventuallyInvokeSelectCmd(legendPtr);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionPresentOp
 *
 *      Returns 1 if there is a selection and 0 if it isn't.
 *
 * Results:
 *      A standard TCL result.  interp->result will contain a boolean
 *      string indicating if there is a selection.
 *
 *      pathName legend selection present
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionPresentOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    int state;

    state = (Blt_Chain_GetLength(graphPtr->legend->selected) > 0);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionSetOp
 *
 *      Selects, deselects, or toggles all of the elements in the range
 *      between first and last, inclusive, without affecting the selection
 *      state of elements outside that range.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName legend selection set first last
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Element *firstPtr, *lastPtr;
    Graph *graphPtr = clientData;
    Legend *legendPtr;
    const char *string;

    legendPtr = graphPtr->legend;
    legendPtr->flags &= ~SELECT_MASK;
    string = Tcl_GetString(objv[3]);
    switch (string[0]) {
    case 's':
        legendPtr->flags |= SELECT_SET;
        break;
    case 'c':
        legendPtr->flags |= SELECT_CLEAR;
        break;
    case 't':
        legendPtr->flags |= SELECT_TOGGLE;
        break;
    }
    if (GetElementFromObj(graphPtr, objv[4], &firstPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((firstPtr->flags & HIDDEN) && ((legendPtr->flags & SELECT_CLEAR)==0)) {
        Tcl_AppendResult(interp, "can't select hidden node \"", 
                Tcl_GetString(objv[4]), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    lastPtr = firstPtr;
    if (objc > 5) {
        if (GetElementFromObj(graphPtr, objv[5], &lastPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if ((lastPtr->flags & HIDDEN) && 
            ((legendPtr->flags & SELECT_CLEAR) == 0)) {
            Tcl_AppendResult(interp, "can't select hidden node \"", 
                     Tcl_GetString(objv[5]), "\"", (char *)NULL);
            return TCL_ERROR;
        }
    }
    if (firstPtr == lastPtr) {
        SelectEntry(legendPtr, firstPtr);
    } else {
        SelectRange(legendPtr, firstPtr, lastPtr);
    }
    /* Set both the anchor and the mark. Indicates that a single entry is
     * selected. */
    if (legendPtr->selAnchorPtr == NULL) {
        legendPtr->selAnchorPtr = firstPtr;
    }
    if (legendPtr->flags & SELECT_EXPORT) {
        Tk_OwnSelection(legendPtr->tkwin, XA_PRIMARY, LostSelectionProc, 
                        legendPtr);
    }
    Blt_Legend_EventuallyRedraw(graphPtr);
    if (legendPtr->selCmdObjPtr != NULL) {
        EventuallyInvokeSelectCmd(legendPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionOp --
 *
 *      This procedure handles the individual options for text selections.
 *      The selected text is designated by start and end indices into the
 *      text pool.  The selected segment has both a anchored and unanchored
 *      ends.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName legend selection arg arg...
 *
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
SelectionOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numSelectionOps, selectionOps, BLT_OP_ARG3, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LegendOp --
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Legend is possibly redrawn.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec legendOps[] =
{
    {"activate",     1, ActivateOp,      3, 4, "?elemName?",},
    {"bbox",         2, BBoxOp,          4, 0, "elemName ?switches?",},
    {"bind",         2, BindOp,          3, 6, "elemName sequence command",},
    {"cget",         2, CgetOp,          4, 4, "option",},
    {"configure",    2, ConfigureOp,     3, 0, "?option value ...?",},
    {"curselection", 2, CurselectionOp,  3, 3, "",},
    {"deactivate",   1, DeactivateOp,    3, 3, "",},
    {"focus",        1, FocusOp,         4, 4, "elemName",},
    {"get",          1, GetOp,           4, 4, "elemName",},
    {"icon",         1, IconOp,          5, 5, "elemName imageName",},
    {"selection",    1, SelectionOp,     3, 0, "args..."},
};
static int numLegendOps = sizeof(legendOps) / sizeof(Blt_OpSpec);

int
Blt_LegendOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numLegendOps, legendOps, BLT_OP_ARG2, 
        objc, objv,0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
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
    return (graphPtr->legend->flags & HIDDEN);
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
 *      This procedure is called back by Tk when the selection is requested
 *      by someone.  It returns part or all of the selection in a buffer
 *      provided by the caller.
 *
 * Results:
 *      The return value is the number of non-NULL bytes stored at buffer.
 *      Buffer is filled (or partially filled) with a NUL-terminated string
 *      containing part or all of the selection, as given by offset and
 *      maxBytes.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectionProc(
    ClientData clientData,              /* Information about the widget. */
    int offset,                         /* Offset within selection of first
                                         * character to be returned. */
    char *buffer,                       /* Location in which to place
                                         * selection. */
    int maxBytes)                       /* Maximum number of bytes to place
                                         * at buffer, not including
                                         * terminating NULL character. */
{
    Legend *legendPtr = clientData;
    int numBytes;
    Tcl_DString ds;

    if ((legendPtr->flags & SELECT_EXPORT) == 0) {
        return -1;
    }
    /* Retrieve the names of the selected entries. */
    Tcl_DStringInit(&ds);
    if (legendPtr->flags & SELECT_SORTED) {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(legendPtr->selected); 
             link != NULL; link = Blt_Chain_NextLink(link)) {
            Element *elemPtr;

            elemPtr = Blt_Chain_GetValue(link);
            Tcl_DStringAppend(&ds, elemPtr->obj.name, -1);
            Tcl_DStringAppend(&ds, "\n", -1);
        }
    } else {
        Blt_ChainLink link;
        Graph *graphPtr;

        graphPtr = legendPtr->obj.graphPtr;
        /* List of selected entries is in stacking order. */
        for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
             link != NULL; link = Blt_Chain_NextLink(link)) {
            Element *elemPtr;
            
            elemPtr = Blt_Chain_GetValue(link);
            if (EntryIsSelected(legendPtr, elemPtr)) {
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


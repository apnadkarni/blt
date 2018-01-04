/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltScrollbar.c --
 *
 * This module implements a scrollbar widgets for the Tk toolkit.  A
 * scrollbar displays a slider and two arrows; mouse clicks on features
 * within the scrollbar cause scrolling commands to be invoked.
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
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 *   This software is copyrighted by the Regents of the University of
 *   California, Sun Microsystems, Inc., and other parties.  The following
 *   terms apply to all files associated with the software unless
 *   explicitly disclaimed in individual files.
 * 
 *   The authors hereby grant permission to use, copy, modify, distribute,
 *   and license this software and its documentation for any purpose,
 *   provided that existing copyright notices are retained in all copies
 *   and that this notice is included verbatim in any distributions. No
 *   written agreement, license, or royalty fee is required for any of the
 *   authorized uses.  Modifications to this software may be copyrighted by
 *   their authors and need not follow the licensing terms described here,
 *   provided that the new terms are clearly indicated on the first page of
 *   each file where they apply.
 * 
 *   IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 *   FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *   ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 *   DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 * 
 *   THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 *   NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND
 *   THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *   MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *   GOVERNMENT USE: If you are acquiring this software on behalf of the
 *   U.S. government, the Government shall have only "Restricted Rights" in
 *   the software and related documentation as defined in the Federal
 *   Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *   are acquiring the software on behalf of the Department of Defense, the
 *   software shall be classified as "Commercial Computer Software" and the
 *   Government shall have only "Restricted Rights" as defined in Clause
 *   252.227-7013 (b) (3) of DFARs.  Notwithstanding the foregoing, the
 *   authors grant the U.S. Government and others acting in its behalf
 *   permission to use and distribute the software in accordance with the
 *   terms specified in this license.
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_TKSCROLLBAR

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */
#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include "bltAlloc.h"
#include "bltBg.h"
#include "bltInitCmd.h"

#define NORMAL_BG       "#d9d9d9"
#define ACTIVE_BG       "#ececec"
#define SELECT_BG       "#c3c3c3"
#define TROUGH          "#c3c3c3"
#define INDICATOR       "#b03060"
#define DISABLED        "#a3a3a3"

#define STYLE_XP        (0)
#define STYLE_TK        (1)
#define STYLE_VISTA     (2)

/*
 * Defaults for scrollbars:
 */
#define DEF_ACTIVE_BACKGROUND   ACTIVE_BG
#define DEF_ACTIVE_BG_MONO      RGB_BLACK
#define DEF_ACTIVE_RELIEF       "raised"
#define DEF_ARROW_COLOR         "black" 
#define DEF_DISABLED_ARROW_COLOR RGB_GREY50
#define DEF_BACKGROUND          NORMAL_BG
#define DEF_BG_MONO             RGB_WHITE
#define DEF_BORDERWIDTH         "2"
#define DEF_COMMAND             ""
#define DEF_CURSOR              ""
#define DEF_ELEMENT_BORDERWIDTH "1"
#define DEF_HIGHLIGHT           RGB_BLACK
#define DEF_HIGHLIGHT_BG        NORMAL_BG
#define DEF_HIGHLIGHT_WIDTH     "2"
#define DEF_JUMP                "0"
#define DEF_MIN_SLIDER_LENGTH   "12"
#define DEF_ORIENT              "vertical"
#define DEF_STYLE              "xp"
#define DEF_RELIEF              "sunken"
#define DEF_REPEAT_DELAY        "300"
#define DEF_REPEAT_INTERVAL     "100"
#define DEF_SELECT_BACKGROUND   SELECT_BG
#define DEF_TAKE_FOCUS          (char *)NULL
#define DEF_TROUGH_COLOR        "grey" /*TROUGH*/
#define DEF_TROUGH_MONO         RGB_WHITE
#define DEF_WIDTH               ".3c"
#define DEF_SELECT_RELIEF       "sunken"

/*
 * A data structure of the following type is kept for each scrollbar widget
 * managed by this file:
 */

typedef struct {
    Tk_Window tkwin;                    /* Window that embodies the
                                         * scrollbar.  NULL means that the
                                         * window has been destroyed but
                                         * the data structures haven't yet
                                         * been cleaned up.*/
    Display *display;                   /* Display containing widget.
                                         * Used, among other things, so
                                         * that resources can be freed even
                                         * after tkwin has gone away. */
    Tcl_Interp *interp;                 /* Interpreter associated with
                                         * scrollbar. */
    Tcl_Command widgetCmd;              /* Token for scrollbar's widget
                                         * command. */
    char *orientation;                  /* Orientation for window
                                         * ("vertical" or "horizontal"). */
    int vertical;                       /* Non-zero means vertical
                                         * orientation requested, zero
                                         * means horizontal. */
    int width;                          /* Desired narrow dimension of
                                         * scrollbar, in pixels. */
    char *command;                      /* Command prefix to use when
                                         * invoking scrolling commands.
                                         * NULL means don't invoke
                                         * commands.  Malloc'ed. */
    int commandSize;                    /* Number of non-NULL bytes in
                                         * command. */
    int repeatDelay;                    /* How long to wait before
                                         * auto-repeating on scrolling
                                         * actions (in * ms). */
    int repeatInterval;                 /* Interval between autorepeats (in
                                         * ms). */
    int jump;                           /* Value of -jump option. */

    /*
     * Information used when displaying widget:
     */
    int style;
    int borderWidth;                    /* Width of 3-D borders. */
    Blt_Bg bg;                          /* Used for drawing background (all
                                         * flat surfaces except for
                                         * trough). */
    Blt_Bg activeBg;                    /* For drawing backgrounds when
                                         * active (i.e.  when mouse is
                                         * positioned over element). */
    Blt_Bg selBg;
    Blt_Bg troughBg;                    /* For drawing trough. */
    GC copyGC;                          /* Used for copying from pixmap
                                         * onto screen. */
    XColor *disabledArrowColor;         /* Used for drawing the arrow. */
    XColor *arrowColor;                 /* Used for drawing the arrow. */
    int relief;                         /* Indicates whether window as a
                                         * whole is raised, sunken, or
                                         * flat. */
    int highlightWidth;                 /* Width in pixels of highlight to
                                         * draw around widget when it has
                                         * the focus.  <= 0 means don't
                                         * draw a highlight. */
    XColor *highlightBgColorPtr;        /* Color for drawing traversal
                                         * highlight area when highlight is
                                         * off. */
    XColor *highlightColorPtr;          /* Color for drawing traversal
                                         * highlight. */
    int inset;                          /* Total width of all borders,
                                         * including traversal highlight
                                         * and 3-D * border.  Indicates how
                                         * much interior stuff must be
                                         * offset from outside edges to
                                         * leave room for borders. */
    int minSliderLength;                /* Minimum size of thumb. */
    int elementBW;                      /* Width of border to draw around
                                         * elements inside scrollbar (arrows
                                         * and * slider).  -1 means use
                                         * borderWidth. */
    int arrowLength;                    /* Length of arrows along long
                                         * dimension of scrollbar, including
                                         * space for a small gap between the
                                         * arrow and the slider.  Recomputed
                                         * on window size changes. */
    int sliderFirst;                    /* Pixel coordinate of top or left
                                         * edge of slider area, including
                                         * border. */
    int sliderLast;                     /* Coordinate of pixel just after
                                         * bottom or right edge of slider
                                         * area, including border. */
    int activeField;                    /* Names field to be displayed in
                                         * active colors, such as
                                         * TOP_ARROW, or 0 for no field. */
    int activeRelief;                   /* Value of -activeRelief option:
                                         * relief to use for active
                                         * element. */
    int selRelief;
    int selField;                       /* Names field to be displayed in
                                         * active colors, such as
                                         * TOP_ARROW, or 0 for no field. */
   /*
     * Information describing the application related to the scrollbar.
     * This information is provided by the application by invoking the
     * "set" widget command.  This information can now be provided in two
     * ways: the "old" form (totalUnits, windowUnits, firstUnit, and
     * lastUnit), or the "new" form (firstFraction and lastFraction).
     * FirstFraction and lastFraction will always be valid, but the
     * old-style information is only valid if the NEW_STYLE_COMMANDS flag
     * is 0.
     */

    int totalUnits;                     /* Total dimension of application, in
                                         * units.  Valid only if the
                                         * NEW_STYLE_COMMANDS flag isn't
                                         * set. */
    int windowUnits;                    /* Maximum number of units that can be
                                         * displayed in the window at once.
                                         * Valid * only if the
                                         * NEW_STYLE_COMMANDS flag isn't
                                         * set. */
    int firstUnit;                      /* Number of last unit visible in
                                         * application's window.  Valid
                                         * only if the NEW_STYLE_COMMANDS
                                         * flag isn't set. */
    int lastUnit;                       /* Index of last unit visible in
                                         * window.  Valid only if the
                                         * NEW_STYLE_COMMANDS flag isn't
                                         * set. */
    double firstFraction;               /* Position of first visible thing
                                         * in window, specified as a
                                         * fraction between 0 and 1.0. */
    double lastFraction;                /* Position of last visible thing
                                         * in window, specified as a
                                         * fraction between 0 and 1.0. */
    /*
     * Miscellaneous information:
     */
    Tk_Cursor cursor;                   /* Current cursor for window, or
                                         * None. */
    char *takeFocus;                    /* Value of -takefocus option; not
                                         * used in the C code, but used by
                                         * keyboard traversal scripts.
                                         * Malloc'ed, but may be * NULL. */
    int flags;                          /* Various flags; see below for
                                         * definitions. */
    Blt_Picture upArrow, downArrow;
    Blt_Painter painter;
} Scrollbar;

/*
 * Legal values for "activeField" field of Scrollbar structures.  These are
 * also the return values from the ScrollbarPosition procedure.
 */

#define OUTSIDE         0
#define TOP_ARROW       1
#define TOP_GAP         2
#define SLIDER          3
#define BOTTOM_GAP      4
#define BOTTOM_ARROW    5

/*
 * Flag bits for scrollbars:
 *
 * REDRAW_PENDING:              Non-zero means a DoWhenIdle handler
 *                              has already been queued to redraw
 *                              this window.
 * NEW_STYLE_COMMANDS:          Non-zero means the new style of commands
 *                              should be used to communicate with the
 *                              widget:  ".t yview scroll 2 lines", instead
 *                              of ".t yview 40", for example.
 * GOT_FOCUS:                   Non-zero means this window has the input
 *                              focus.
 */

#define REDRAW_PENDING          1
#define NEW_STYLE_COMMANDS      2
#define GOT_FOCUS               4

/*
 * Minimum slider length, in pixels (designed to make sure that the slider
 * is always easy to grab with the mouse).
 */

#define MIN_SLIDER_LENGTH       12

static Blt_OptionParseProc ObjToStyle;
static Blt_OptionPrintProc StyleToObj;
static Blt_CustomOption styleOption = {
    ObjToStyle, StyleToObj, NULL, (ClientData)0
};

/*
 * Information used for objv parsing.
 */

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
        "Foreground", DEF_ACTIVE_BACKGROUND, Blt_Offset(Scrollbar, activeBg),
        BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
        "Foreground", DEF_ACTIVE_BG_MONO, Blt_Offset(Scrollbar, activeBg),
        BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "Relief",
        DEF_ACTIVE_RELIEF, Blt_Offset(Scrollbar, activeRelief), 0},
    {BLT_CONFIG_COLOR, "-arrowcolor", "arrowColor", "ArrowColor", 
        DEF_ARROW_COLOR, Blt_Offset(Scrollbar, arrowColor), 0},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BACKGROUND, Blt_Offset(Scrollbar, bg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BG_MONO, Blt_Offset(Scrollbar, bg), BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BORDERWIDTH, Blt_Offset(Scrollbar, borderWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-command", "command", "Command",
        DEF_COMMAND, Blt_Offset(Scrollbar, command),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
        DEF_CURSOR, Blt_Offset(Scrollbar, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-disabledarrowcolor", "disabledArrowColor", 
        "DisabledArrowColor", DEF_DISABLED_ARROW_COLOR, 
        Blt_Offset(Scrollbar, disabledArrowColor), 0},
    {BLT_CONFIG_PIXELS, "-elementborderwidth", "elementBorderWidth",
        "BorderWidth", DEF_ELEMENT_BORDERWIDTH,
        Blt_Offset(Scrollbar, elementBW), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
        "HighlightBackground", DEF_HIGHLIGHT_BG,
        Blt_Offset(Scrollbar, highlightBgColorPtr), 0},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
        DEF_HIGHLIGHT,
        Blt_Offset(Scrollbar, highlightColorPtr), 0},
    {BLT_CONFIG_PIXELS, "-highlightthickness", "highlightThickness",
        "HighlightThickness",
        DEF_HIGHLIGHT_WIDTH, Blt_Offset(Scrollbar, highlightWidth), 0},
    {BLT_CONFIG_BOOLEAN, "-jump", "jump", "Jump",
        DEF_JUMP, Blt_Offset(Scrollbar, jump), 0},
    {BLT_CONFIG_PIXELS_POS, "-minsliderlength", "minSliderLength",
        "MinSliderLength", DEF_MIN_SLIDER_LENGTH, 
        Blt_Offset(Scrollbar, minSliderLength), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-orient", "orient", "Orient",
        DEF_ORIENT, Blt_Offset(Scrollbar, orientation), 0},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief",
        DEF_RELIEF, Blt_Offset(Scrollbar, relief), 0},
    {BLT_CONFIG_INT, "-repeatdelay", "repeatDelay", "RepeatDelay",
        DEF_REPEAT_DELAY, Blt_Offset(Scrollbar, repeatDelay), 0},
    {BLT_CONFIG_INT, "-repeatinterval", "repeatInterval", "RepeatInterval",
        DEF_REPEAT_INTERVAL, Blt_Offset(Scrollbar, repeatInterval), 0},
    {BLT_CONFIG_BACKGROUND, "-selectbackground", "selectBackground", 
        "Foreground", DEF_SELECT_BACKGROUND, Blt_Offset(Scrollbar, selBg), 0},
    {BLT_CONFIG_RELIEF, "-selectrelief", "selectRelief", "Relief", 
        DEF_ACTIVE_RELIEF, Blt_Offset(Scrollbar, selRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-style", "style", "Style", DEF_STYLE,
        Blt_Offset(Scrollbar, style), BLT_CONFIG_DONT_SET_DEFAULT,
        &styleOption},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
        DEF_TAKE_FOCUS, Blt_Offset(Scrollbar, takeFocus),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-troughcolor", "troughColor", "Background",
        DEF_TROUGH_COLOR, Blt_Offset(Scrollbar, troughBg), 0},
    {BLT_CONFIG_PIXELS, "-width", "width", "Width", DEF_WIDTH, 
        Blt_Offset(Scrollbar, width), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void ComputeScrollbarGeometry(Scrollbar *scrollPtr);
static int ConfigureScrollbar(Tcl_Interp *interp, Scrollbar *scrollPtr, 
        int objc, Tcl_Obj *const *objv, int flags);
static void DestroyScrollbar(DestroyData *memPtr);
static void DisplayScrollbar(ClientData clientData);
static void EventuallyRedraw(Scrollbar *scrollPtr);
static void ScrollbarCmdDeletedProc(ClientData clientData);
static void ScrollbarEventProc(ClientData clientData, XEvent *eventPtr);
static int ScrollbarPosition(Scrollbar *scrollPtr, int x, int y);
static int ScrollbarWidgetCmd(ClientData clientData, Tcl_Interp *, int objc, 
        Tcl_Obj *const *objv);

static Blt_BackgroundChangedProc BackgroundChangedProc;
static Tcl_ObjCmdProc ScrollbarCmd;


static const char *
NameOfField(int field)
{
    switch (field) {
    case BOTTOM_ARROW:  return "arrow2";
    case BOTTOM_GAP:    return "trough2";
    case SLIDER:        return "slider";
    case TOP_ARROW:     return "arrow1";
    case TOP_GAP:       return "trough1";
    default:
        return "???";
    }
}

static int
GetFieldFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *fieldPtr)
{
    char *string;
    int length;
    char c;

    string  = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'a') && (strcmp(string, "arrow1") == 0)) {
        *fieldPtr = TOP_ARROW;
    } else if ((c == 'a') && (strcmp(string, "arrow2") == 0)) {
        *fieldPtr = BOTTOM_ARROW;
    } else if ((c == 's') && (strncmp(string, "slider", length) == 0)) {
        *fieldPtr = SLIDER;
    } else {
        *fieldPtr = OUTSIDE;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToStyle --
 *
 *      Converts the style name into its integer representation.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToStyle(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    char c;
    const char *string;
    int *stylePtr = (int *)(widgRec + offset);
    int length;
    int oldStyle;

    oldStyle = *stylePtr;
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];

    if ((c == 't') && (strncmp(string, "tk", length) == 0)) {
        *stylePtr = STYLE_TK;
    } else if ((c == 'x') && (strncmp(string, "xp", length) == 0)) {
        *stylePtr = STYLE_XP;
    } else if ((c == 'v') && (strncmp(string, "vista", length) == 0)) {
        *stylePtr = STYLE_VISTA;
    } else {
        Tcl_AppendResult(interp, "unknown style \"", string,
                         "\": should be tk, xp, or vista.", (char *)NULL);
        return TCL_ERROR;
    }
    if (oldStyle != *stylePtr) {
        Scrollbar *scrollPtr = (Scrollbar *)widgRec;

        if (scrollPtr->upArrow != NULL) {
            Blt_FreePicture(scrollPtr->upArrow);
            scrollPtr->upArrow = NULL;
        }
        if (scrollPtr->downArrow != NULL) {
            Blt_FreePicture(scrollPtr->downArrow);
            scrollPtr->downArrow = NULL;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StyleToObj --
 *
 *      Returns the name of the style.
 *
 * Results:
 *      The name representing the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StyleToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)  
{
    int style = *(int *)(widgRec + offset);
    Tcl_Obj *objPtr;
    
    switch (style) {
    case STYLE_TK:
        objPtr = Tcl_NewStringObj("tk", 2);
        break;
    case STYLE_XP:
        objPtr = Tcl_NewStringObj("xp", 2);
        break;
    case STYLE_VISTA:
        objPtr = Tcl_NewStringObj("vista", 5);
        break;
    default:
        objPtr = Tcl_NewStringObj("???", 3);
        break;
    } 
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarCmd --
 *
 *      This procedure is invoked to process the "scrollbar" TCL command.  See
 *      the user documentation for details on what it does.
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
ScrollbarCmd(
    ClientData clientData,      /* Main window associated with
                                 * interpreter. */
    Tcl_Interp *interp,         /* Current interpreter. */
    int objc,                   /* Number of arguments. */
    Tcl_Obj *const *objv)       /* Argument strings. */
{
    Scrollbar *scrollPtr;
    Tk_Window tkwin;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
            Tcl_GetString(objv[0]), " pathName ?options?\"", (char *)NULL);
        return TCL_ERROR;
    }
    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), 
        Tcl_GetString(objv[1]), (char *)NULL);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    /*
     * Initialize fields that won't be initialized by ConfigureScrollbar, or
     * which ConfigureScrollbar expects to have reasonable values
     * (e.g. resource pointers).
     */
    scrollPtr = Blt_AssertCalloc(1, sizeof(Scrollbar));
    scrollPtr->tkwin = tkwin;
    scrollPtr->display = Tk_Display(tkwin);
    scrollPtr->interp = interp;
    scrollPtr->widgetCmd = Tcl_CreateObjCommand(interp,
        Tk_PathName(scrollPtr->tkwin), ScrollbarWidgetCmd,
        (ClientData)scrollPtr, ScrollbarCmdDeletedProc);
    scrollPtr->relief = TK_RELIEF_FLAT;
    scrollPtr->elementBW = 2;
    scrollPtr->borderWidth = 1;
    scrollPtr->selRelief = TK_RELIEF_SUNKEN;
    scrollPtr->activeRelief = TK_RELIEF_RAISED;
    scrollPtr->minSliderLength = MIN_SLIDER_LENGTH;
    scrollPtr->style = STYLE_XP;
    
    Tk_SetClass(scrollPtr->tkwin, "BltTkScrollbar");
#define SCROLLBAR_EVENTS  (ExposureMask|StructureNotifyMask|FocusChangeMask)
    Tk_CreateEventHandler(scrollPtr->tkwin, SCROLLBAR_EVENTS,
                          ScrollbarEventProc, (ClientData)scrollPtr);
    if (ConfigureScrollbar(interp, scrollPtr, objc - 2, objv + 2, 0) != TCL_OK){
        goto error;
    }
    Tcl_SetObjResult(interp, objv[1]);
    return TCL_OK;

  error:
    Tk_DestroyWindow(scrollPtr->tkwin);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarWidgetCmd --
 *
 *      This procedure is invoked to process the TCL command that corresponds
 *      to a widget managed by this module.  See the user documentation for
 *      details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

static int
ScrollbarWidgetCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    Scrollbar *scrollPtr = clientData;
    int result = TCL_OK;
    int length;
    int c;
    char *string;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
            objv[0], " option ?arg arg ...?\"", (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * First time in this interpreter, invoke a procedure to initialize
     * various bindings on the combomenu widget.  If the procedure doesn't
     * already exist, source it from "$blt_library/bltScrollbar.tcl".  We
     * deferred sourcing the file until now so that the variable
     * $blt_library could be set within a script.
     */
    if (!Blt_CommandExists(interp, "::blt::TkScrollbar::ScrollButtonDown")) {
        const char cmd[] = "source [file join $blt_library bltScrollbar.tcl]";
        if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
            char info[200];
            Blt_FmtString(info, 200, "\n\t(while loading bindings for %.50s)", 
                    Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            return TCL_ERROR;
        }
    }
    Tcl_Preserve((ClientData)scrollPtr);
    string = Tcl_GetStringFromObj(objv[1], &length);
    c = string[0];
    if ((c == 'a') && (strncmp(string, "activate", length) == 0)) {
        if (objc > 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " activate element\"", (char *)NULL);
            goto error;
        }
        if (objc == 2) {
            Tcl_SetStringObj(Tcl_GetObjResult(interp), 
                             NameOfField(scrollPtr->activeField), -1);
        } else {
            int activeField;

            GetFieldFromObj(interp, objv[2], &activeField);
            if (activeField != scrollPtr->activeField) {
                scrollPtr->activeField = activeField;
                EventuallyRedraw(scrollPtr);
            }
        }
    } else if ((c == 'c') && (length >= 2) && 
               (strncmp(string, "cget", length) == 0)) {
        if (objc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " cget option\"", (char *)NULL);
            goto error;
        }
        result = Blt_ConfigureValueFromObj(interp, scrollPtr->tkwin, 
                configSpecs, (char *)scrollPtr, objv[2], 0);
    } else if ((c == 'c') && (length >= 2) && 
               (strncmp(string, "configure", length) == 0)) {
        if (objc == 2) {
            result = Blt_ConfigureInfoFromObj(interp, scrollPtr->tkwin, 
                configSpecs, (char *)scrollPtr, (Tcl_Obj *)NULL, 0);
        } else if (objc == 3) {
            result = Blt_ConfigureInfoFromObj(interp, scrollPtr->tkwin, 
                configSpecs, (char *)scrollPtr, objv[2], 0);
        } else {
            result = ConfigureScrollbar(interp, scrollPtr, objc - 2, objv + 2,
                BLT_CONFIG_OBJV_ONLY);
        }
    } else if ((c == 'd') && (strncmp(string, "delta", length) == 0)) {
        int xDelta, yDelta, pixels, barWidth;
        double fraction;

        if (objc != 4) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " delta xDelta yDelta\"", (char *)NULL);
            goto error;
        }
        if ((Tcl_GetIntFromObj(interp, objv[2], &xDelta) != TCL_OK) || 
            (Tcl_GetIntFromObj(interp, objv[3], &yDelta) != TCL_OK)) {
            goto error;
        }
        if (scrollPtr->vertical) {
            pixels = yDelta;
            barWidth = Tk_Height(scrollPtr->tkwin) - 1
                - 2 * (scrollPtr->arrowLength + scrollPtr->inset);
        } else {
            pixels = xDelta;
            barWidth = Tk_Width(scrollPtr->tkwin) - 1
                - 2 * (scrollPtr->arrowLength + scrollPtr->inset);
        }
        if (barWidth == 0) {
            fraction = 0.0;
        } else {
            fraction = ((double)pixels / (double)barWidth);
        }
        Tcl_SetDoubleObj(Tcl_GetObjResult(interp), fraction);
    } else if ((c == 'f') && (strncmp(string, "fraction", length) == 0)) {
        int x, y, pos, barWidth;
        double fraction;

        if (objc != 4) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " fraction x y\"", (char *)NULL);
            goto error;
        }
        if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) || 
            (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
            goto error;
        }
        if (scrollPtr->vertical) {
            pos = y - (scrollPtr->arrowLength + scrollPtr->inset);
            barWidth = Tk_Height(scrollPtr->tkwin) - 1
                - 2 * (scrollPtr->arrowLength + scrollPtr->inset);
        } else {
            pos = x - (scrollPtr->arrowLength + scrollPtr->inset);
            barWidth = Tk_Width(scrollPtr->tkwin) - 1
                - 2 * (scrollPtr->arrowLength + scrollPtr->inset);
        }
        if (barWidth == 0) {
            fraction = 0.0;
        } else {
            fraction = ((double)pos / (double)barWidth);
        }
        if (fraction < 0.0) {
            fraction = 0.0;
        } else if (fraction > 1.0) {
            fraction = 1.0;
        }
        Tcl_SetDoubleObj(Tcl_GetObjResult(interp), fraction);
    } else if ((c == 'g') && (strncmp(string, "get", length) == 0)) {
        if (objc != 2) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " get\"", (char *)NULL);
            goto error;
        }
        if (scrollPtr->flags & NEW_STYLE_COMMANDS) {
            Tcl_Obj *listObjPtr;

            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
            Tcl_ListObjAppendElement(interp, listObjPtr,
                Tcl_NewDoubleObj(scrollPtr->firstFraction));
            Tcl_ListObjAppendElement(interp, listObjPtr,
                Tcl_NewDoubleObj(scrollPtr->lastFraction));
            Tcl_SetObjResult(interp, listObjPtr);
        } else {
            Tcl_Obj *listObjPtr;

            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
            Tcl_ListObjAppendElement(interp, listObjPtr,
                Tcl_NewIntObj(scrollPtr->totalUnits));
            Tcl_ListObjAppendElement(interp, listObjPtr,
                Tcl_NewIntObj(scrollPtr->windowUnits));
            Tcl_ListObjAppendElement(interp, listObjPtr,
                Tcl_NewIntObj(scrollPtr->firstUnit));
            Tcl_ListObjAppendElement(interp, listObjPtr,
                Tcl_NewIntObj(scrollPtr->lastUnit));
            Tcl_SetObjResult(interp, listObjPtr);
        }
    } else if ((c == 'i') && (strncmp(string, "identify", length) == 0)) {
        int x, y, thing;

        if (objc != 4) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " identify x y\"", (char *)NULL);
            goto error;
        }
        if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) || 
            (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
            goto error;
        }
        thing = ScrollbarPosition(scrollPtr, x, y);
        Tcl_SetStringObj(Tcl_GetObjResult(interp), NameOfField(thing), -1);
    } else if ((c == 's') && (strncmp(string, "set", length) == 0)) {
        int totalUnits, windowUnits, firstUnit, lastUnit;

        if (objc == 4) {
            double first, last;

            if ((Tcl_GetDoubleFromObj(interp, objv[2], &first) != TCL_OK) ||
                (Tcl_GetDoubleFromObj(interp, objv[3], &last) != TCL_OK)) {
                goto error;
            }
            if (first < 0) {
                scrollPtr->firstFraction = 0;
            } else if (first > 1.0) {
                scrollPtr->firstFraction = 1.0;
            } else {
                scrollPtr->firstFraction = first;
            }
            if (last < scrollPtr->firstFraction) {
                scrollPtr->lastFraction = scrollPtr->firstFraction;
            } else if (last > 1.0) {
                scrollPtr->lastFraction = 1.0;
            } else {
                scrollPtr->lastFraction = last;
            }
            scrollPtr->flags |= NEW_STYLE_COMMANDS;
        } else if (objc == 6) {
            if (Tcl_GetIntFromObj(interp, objv[2], &totalUnits) != TCL_OK) {
                goto error;
            }
            if (totalUnits < 0) {
                totalUnits = 0;
            }
            if (Tcl_GetIntFromObj(interp, objv[3], &windowUnits) != TCL_OK) {
                goto error;
            }
            if (windowUnits < 0) {
                windowUnits = 0;
            }
            if (Tcl_GetIntFromObj(interp, objv[4], &firstUnit) != TCL_OK) {
                goto error;
            }
            if (Tcl_GetIntFromObj(interp, objv[5], &lastUnit) != TCL_OK) {
                goto error;
            }
            if (totalUnits > 0) {
                if (lastUnit < firstUnit) {
                    lastUnit = firstUnit;
                }
            } else {
                firstUnit = lastUnit = 0;
            }
            scrollPtr->totalUnits = totalUnits;
            scrollPtr->windowUnits = windowUnits;
            scrollPtr->firstUnit = firstUnit;
            scrollPtr->lastUnit = lastUnit;
            if (scrollPtr->totalUnits == 0) {
                scrollPtr->firstFraction = 0.0;
                scrollPtr->lastFraction = 1.0;
            } else {
                scrollPtr->firstFraction = ((double)firstUnit) / totalUnits;
                scrollPtr->lastFraction = ((double)(lastUnit + 1)) / totalUnits;
            }
            scrollPtr->flags &= ~NEW_STYLE_COMMANDS;
        } else {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), 
                " set firstFraction lastFraction\" or \"",
                Tcl_GetString(objv[0]),
                " set totalUnits windowUnits firstUnit lastUnit\"",
                (char *)NULL);
            goto error;
        }
        ComputeScrollbarGeometry(scrollPtr);
        EventuallyRedraw(scrollPtr);
    } else if ((c == 's') && (strncmp(string, "select", length) == 0)) {
        if (objc > 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " select element\"", (char *)NULL);
            goto error;
        }
        if (objc == 2) {
            Tcl_SetStringObj(Tcl_GetObjResult(interp), 
                             NameOfField(scrollPtr->selField), -1);
        } else {
            GetFieldFromObj(interp, objv[2], &scrollPtr->selField);
            EventuallyRedraw(scrollPtr);
        }
    } else {
        Tcl_AppendResult(interp, "bad option \"", Tcl_GetString(objv[1]),
            "\": must be activate, cget, configure, delta, fraction, ",
            "get, identify, or set", (char *)NULL);
        goto error;
    }
    Tcl_Release((ClientData)scrollPtr);
    return result;

  error:
    Tcl_Release((ClientData)scrollPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyScrollbar --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the internal structure of a scrollbar at a safe time (when
 *      no-one is using it anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the scrollbar is freed up.
 *
 *---------------------------------------------------------------------------
 */

static void
DestroyScrollbar(DestroyData *memPtr) /* Info about scrollbar widget. */
{
    Scrollbar *scrollPtr = (Scrollbar *)memPtr;

    /*
     * Free up all the stuff that requires special handling, then let
     * Blt_FreeOptions handle all the standard option-related stuff.
     */
    if (scrollPtr->copyGC != None) {
        Tk_FreeGC(scrollPtr->display, scrollPtr->copyGC);
    }
    if (scrollPtr->painter != NULL) {
        Blt_FreePainter(scrollPtr->painter);
    }
    if (scrollPtr->upArrow != NULL) {
        Blt_FreePicture(scrollPtr->upArrow);
    }
    if (scrollPtr->downArrow != NULL) {
        Blt_FreePicture(scrollPtr->downArrow);
    }
    Blt_FreeOptions(configSpecs, (char *)scrollPtr, scrollPtr->display, 0);
    Blt_Free(scrollPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * BackgroundChangedProc
 *
 *      Routine for background change notifications.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
BackgroundChangedProc(ClientData clientData)
{
    Scrollbar *scrollPtr = clientData;

    if (scrollPtr->tkwin != NULL) {
        EventuallyRedraw(scrollPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureScrollbar --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) a scrollbar
 *      widget.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is returned,
 *      then interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as colors, border width, etc. get set
 *      for scrollPtr; old resources get freed, if there were any.
 *
 *---------------------------------------------------------------------------
 */

static int
ConfigureScrollbar(
    Tcl_Interp *interp,         /* Used for error reporting. */
    Scrollbar *scrollPtr,       /* Information about widget; may or
                                 * may not already have values for
                                 * some fields. */
    int objc,                   /* Number of valid entries in objv. */
    Tcl_Obj *const *objv,       /* Arguments. */
    int flags)                  /* Flags to pass to
                                 * Blt_ConfigureWidget. */
{
    size_t length;
    XGCValues gcValues;

    if (Blt_ConfigureWidgetFromObj(interp, scrollPtr->tkwin, configSpecs,
            objc, objv, (char *)scrollPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    /*
     * A few options need special processing, such as parsing the orientation
     * or setting the background from a 3-D border.
     */
    length = strlen(scrollPtr->orientation);
    if (strncmp(scrollPtr->orientation, "vertical", length) == 0) {
        scrollPtr->vertical = 1;
    } else if (strncmp(scrollPtr->orientation, "horizontal", length) == 0) {
        scrollPtr->vertical = 0;
    } else {
        Tcl_AppendResult(interp, "bad orientation \"", scrollPtr->orientation,
            "\": must be vertical or horizontal", (char *)NULL);
        return TCL_ERROR;
    }
    if (scrollPtr->command != NULL) {
        scrollPtr->commandSize = strlen(scrollPtr->command);
    } else {
        scrollPtr->commandSize = 0;
    }
    if (scrollPtr->activeBg != NULL) {
        Blt_Bg_SetChangedProc(scrollPtr->activeBg, BackgroundChangedProc,
                scrollPtr);
    }
    if (scrollPtr->bg != NULL) {
        Blt_Bg_SetChangedProc(scrollPtr->bg, BackgroundChangedProc,
                scrollPtr);
    }
    Blt_Bg_SetFromBackground(scrollPtr->tkwin, scrollPtr->bg);

    if (scrollPtr->copyGC == None) {
        gcValues.graphics_exposures = False;
        scrollPtr->copyGC = Tk_GetGC(scrollPtr->tkwin, GCGraphicsExposures,
            &gcValues);
    }
    scrollPtr->width |= 0x1;
    /*
     * Register the desired geometry for the window (leave enough space for
     * the two arrows plus a minimum-size slider, plus border around the whole
     * window, if any).  Then arrange for the window to be redisplayed.
     */

    ComputeScrollbarGeometry(scrollPtr);
    EventuallyRedraw(scrollPtr);
    return TCL_OK;
}

static Blt_Picture
GetArrowPicture(Scrollbar *scrollPtr, int w, int h, int type, int direction)
{
    switch (direction) {
    case ARROW_UP:
    case ARROW_LEFT:
        if (scrollPtr->upArrow == NULL) {
            Blt_Picture picture;
            unsigned int color;

            picture = Blt_CreatePicture(w, h);
            Blt_BlankPicture(picture, 0x0);
            color = Blt_XColorToPixel(scrollPtr->arrowColor);
            if (type == STYLE_XP) {
                Blt_PaintArrowHead(picture, 0, 0, w, h, color, direction);
            } else {
                Blt_PaintChevron(picture, 0, 0, w, h, color, direction);
            }
            scrollPtr->upArrow = picture;
        }
        return scrollPtr->upArrow;
    case ARROW_DOWN:
    case ARROW_RIGHT:
        if (scrollPtr->downArrow == NULL) {
            Blt_Picture picture;
            unsigned int color;

            picture = Blt_CreatePicture(w, h);
            Blt_BlankPicture(picture, 0x0);
            color = Blt_XColorToPixel(scrollPtr->arrowColor);
            if (type == STYLE_XP) {
                Blt_PaintArrowHead(picture, 0, 0, w, h, color, direction);
            } else {
                Blt_PaintChevron(picture, 0, 0, w, h, color, direction);
            }
            scrollPtr->downArrow = picture;
        }
        return scrollPtr->downArrow;
    default:
        abort();
        return NULL;
    }
}

static void
DrawArrowTkStyle(Scrollbar *scrollPtr, Drawable drawable, int size,
                 int borderWidth, int direction)
{
    int relief;
    Blt_Bg bg;
    XPoint points[3];
    
    bg = NULL;                          /* Suppress compiler warnings. */
    relief = TK_RELIEF_FLAT;
    switch (direction) {

   /*
    * Draw the top or left arrow.  The coordinates of the polygon points
    * probably seem odd, but they were carefully chosen with respect to X's
    * rules for filling polygons.  These point choices cause the arrows to
    * just fill the narrow dimension of the scrollbar and be properly
    * centered.
    */
    case ARROW_UP:
        if (scrollPtr->selField == TOP_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == TOP_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        points[0].x = scrollPtr->inset - 1;
        points[0].y = scrollPtr->arrowLength + scrollPtr->inset - 1;
        points[1].x = size + scrollPtr->inset;
        points[1].y = points[0].y;
        points[2].x = size / 2 + scrollPtr->inset;
        points[2].y = scrollPtr->inset - 1;
        break;
    case ARROW_LEFT:
        if (scrollPtr->selField == TOP_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == TOP_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        points[0].x = scrollPtr->arrowLength + scrollPtr->inset - 1;
        points[0].y = scrollPtr->inset - 1;
        points[1].x = scrollPtr->inset;
        points[1].y = size / 2 + scrollPtr->inset;
        points[2].x = points[0].x;
        points[2].y = size + scrollPtr->inset;
        break;
    case ARROW_DOWN:
        if (scrollPtr->selField == BOTTOM_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == BOTTOM_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        points[0].x = scrollPtr->inset;
        points[0].y = Tk_Height(scrollPtr->tkwin) - scrollPtr->arrowLength
            - scrollPtr->inset + 1;
        points[1].x = size / 2 + scrollPtr->inset;
        points[1].y = Tk_Height(scrollPtr->tkwin) - scrollPtr->inset;
        points[2].x = size + scrollPtr->inset;
        points[2].y = points[0].y;
        break;
    case ARROW_RIGHT:
        if (scrollPtr->selField == BOTTOM_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == BOTTOM_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        points[0].x = Tk_Width(scrollPtr->tkwin) - scrollPtr->arrowLength
            - scrollPtr->inset + 1;
        points[0].y = scrollPtr->inset - 1;
        points[1].x = points[0].x;
        points[1].y = size + scrollPtr->inset;
        points[2].x = Tk_Width(scrollPtr->tkwin) - scrollPtr->inset;
        points[2].y = size / 2 + scrollPtr->inset;
        break;
    }
    Blt_Bg_FillPolygon(scrollPtr->tkwin, drawable, bg, points, 3, borderWidth,
        relief);
}

static void
DrawArrowXPStyle(Scrollbar *scrollPtr, Drawable drawable, int size,
                 int borderWidth, int direction)
{                
    int bx, by, ax, ay, aw, ah, cavityWidth;
    Blt_Picture picture;
    int relief;
    Blt_Bg bg;

    cavityWidth = size - 2 * borderWidth;
    switch (direction) {
    case ARROW_UP:
        if (scrollPtr->selField == TOP_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == TOP_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        bx = by = scrollPtr->inset;
        ax = bx + borderWidth;
        ay = by + borderWidth;
        aw = cavityWidth * 90 /100;
        ah = cavityWidth * 60/100;
        break;
    case ARROW_LEFT:
        if (scrollPtr->selField == TOP_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == TOP_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        bx = by = scrollPtr->inset;
        ax = bx + borderWidth;
        ay = by + borderWidth;
        ah = cavityWidth * 90 /100;
        aw = cavityWidth * 60/100;
        break;
    case ARROW_DOWN:
        if (scrollPtr->selField == BOTTOM_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == BOTTOM_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        aw = cavityWidth * 90 /100;
        ah = cavityWidth * 60/100;
        bx = Tk_Width(scrollPtr->tkwin) - scrollPtr->inset - size;
        by = Tk_Height(scrollPtr->tkwin) - scrollPtr->inset - size;
        ax = bx + borderWidth;
        ay = by + borderWidth;
        break;
    default:
    case ARROW_RIGHT:
        if (scrollPtr->selField == BOTTOM_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == BOTTOM_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        ah = cavityWidth * 90 /100;
        aw = cavityWidth * 60/100;
        bx = Tk_Width(scrollPtr->tkwin) - scrollPtr->inset - size;
        by = Tk_Height(scrollPtr->tkwin) - scrollPtr->inset - size;
        ax = bx + borderWidth;
        ay = by + borderWidth;
        break;
    }
    if ((aw > 0)  && (ah > 0)) {
        Blt_Bg_FillRectangle(scrollPtr->tkwin, drawable, bg, bx, by, size, size,
                             borderWidth, relief); 
        picture = GetArrowPicture(scrollPtr, aw, ah, STYLE_XP, direction);
        if (scrollPtr->painter == NULL) {
            scrollPtr->painter = Blt_GetPainter(scrollPtr->tkwin, 1.0);
        }
        ax += (cavityWidth - aw) / 2;
        ay += (cavityWidth - ah) / 2;
        Blt_PaintPicture(scrollPtr->painter, drawable, picture, 0, 0, 
                         aw, ah, ax, ay, 0);
    }
}

static void
DrawArrowVistaStyle(Scrollbar *scrollPtr, Drawable drawable, int size,
                 int borderWidth, int direction)
{                
    int bx, by, ax, ay, aw, ah, cavityWidth;
    Blt_Picture picture;
    int relief;
    Blt_Bg bg;

    cavityWidth = size - 2 * borderWidth;
    switch (direction) {
    case ARROW_UP:
        if (scrollPtr->selField == TOP_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == TOP_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        bx = by = scrollPtr->inset;
        ax = bx + borderWidth;
        ay = by + borderWidth;
        aw = cavityWidth * 90 /100;
        ah = cavityWidth * 75/100;
        break;
    case ARROW_LEFT:
        if (scrollPtr->selField == TOP_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == TOP_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        bx = by = scrollPtr->inset;
        ax = bx + borderWidth;
        ay = by + borderWidth;
        ah = cavityWidth * 90 /100;
        aw = cavityWidth * 75/100;
        break;
    case ARROW_DOWN:
        if (scrollPtr->selField == BOTTOM_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == BOTTOM_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        aw = cavityWidth * 90 /100;
        ah = cavityWidth * 75/100;
        bx = Tk_Width(scrollPtr->tkwin) - scrollPtr->inset - size;
        by = Tk_Height(scrollPtr->tkwin) - scrollPtr->inset - size;
        ax = bx + borderWidth;
        ay = by + borderWidth;
        break;
    default:
    case ARROW_RIGHT:
        if (scrollPtr->selField == BOTTOM_ARROW) {
            bg = scrollPtr->selBg;
            relief = scrollPtr->selRelief;
        } else if (scrollPtr->activeField == BOTTOM_ARROW) {
            bg = scrollPtr->activeBg;
            relief = scrollPtr->activeRelief;
        } else {
            bg = scrollPtr->bg;
            relief = TK_RELIEF_RAISED;
        }
        ah = cavityWidth * 90 /100;
        aw = cavityWidth * 75/100;
        bx = Tk_Width(scrollPtr->tkwin) - scrollPtr->inset - size;
        by = Tk_Height(scrollPtr->tkwin) - scrollPtr->inset - size;
        ax = bx + borderWidth;
        ay = by + borderWidth;
        break;
    }
    Blt_Bg_FillRectangle(scrollPtr->tkwin, drawable, bg, bx, by, size, size,
                         borderWidth, relief); 
    picture = GetArrowPicture(scrollPtr, aw, ah, STYLE_VISTA, direction);
    if (scrollPtr->painter == NULL) {
        scrollPtr->painter = Blt_GetPainter(scrollPtr->tkwin, 1.0);
    }
    ax += (cavityWidth - aw) / 2;
    ay += (cavityWidth - ah) / 2;
    Blt_PaintPicture(scrollPtr->painter, drawable, picture, 0, 0, 
                     aw, ah, ax, ay, 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayScrollbar --
 *
 *      This procedure redraws the contents of a scrollbar window.  It is
 *      invoked as a do-when-idle handler, so it only runs when there's
 *      nothing else for the application to do.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Information appears on the screen.
 *
 *---------------------------------------------------------------------------
 */

static void
DisplayScrollbar(ClientData clientData) /* Information about window. */
{
    Blt_Bg bg;
    Pixmap pixmap;
    Scrollbar *scrollPtr = clientData;
    Tk_Window tkwin;
    int relief, width, elementBW;

    scrollPtr->flags &= ~REDRAW_PENDING;
    tkwin = scrollPtr->tkwin;
    if ((tkwin == NULL) || !Tk_IsMapped(tkwin)) {
        return;
    }
    if ((Tk_Width(tkwin) <= 1) || (Tk_Height(tkwin) <= 1)) {
        return;
    }
    if (scrollPtr->vertical) {
        width = Tk_Width(tkwin) - 2 * scrollPtr->inset;
    } else {
        width = Tk_Height(tkwin) - 2 * scrollPtr->inset;
    }
    elementBW = scrollPtr->elementBW;
    if (elementBW < 0) {
        elementBW = scrollPtr->borderWidth;
    }
    if (width <= 0) {
        return;
    }
    /*
     * In order to avoid screen flashes, this procedure redraws the scrollbar
     * in a pixmap, then copies the pixmap to the screen in a single
     * operation.  This means that there's no point in time where the on-sreen
     * image has been cleared.
     */
    pixmap = Blt_GetPixmap(scrollPtr->display, Tk_WindowId(tkwin),
        Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));

    if (scrollPtr->highlightWidth != 0) {
        GC gc;

        if (scrollPtr->flags & GOT_FOCUS) {
            gc = Tk_GCForColor(scrollPtr->highlightColorPtr, pixmap);
        } else {
            gc = Tk_GCForColor(scrollPtr->highlightBgColorPtr, pixmap);
        }
        Tk_DrawFocusHighlight(tkwin, gc, scrollPtr->highlightWidth, pixmap);
    } 
    Blt_Bg_FillRectangle(tkwin, pixmap, scrollPtr->troughBg,
        scrollPtr->highlightWidth, scrollPtr->highlightWidth,
        Tk_Width(tkwin) - 2 * scrollPtr->highlightWidth,
        Tk_Height(tkwin) - 2 * scrollPtr->highlightWidth,
        scrollPtr->borderWidth, scrollPtr->relief);

    /*
     * Display the top or left arrow.
     */
    switch (scrollPtr->style) {
    case STYLE_VISTA:
        DrawArrowVistaStyle(scrollPtr, pixmap, width, elementBW, 
                        (scrollPtr->vertical) ? ARROW_UP : ARROW_LEFT);
        break;
    case STYLE_XP:
        DrawArrowXPStyle(scrollPtr, pixmap, width, elementBW, 
                        (scrollPtr->vertical) ? ARROW_UP : ARROW_LEFT);
        break;
    case STYLE_TK:
        DrawArrowTkStyle(scrollPtr, pixmap, width, elementBW, 
                        (scrollPtr->vertical) ? ARROW_UP : ARROW_LEFT);
        break;
    }
    /*
     * Display the bottom or right arrow.
     */
    switch (scrollPtr->style) {
    case STYLE_VISTA:
        DrawArrowVistaStyle(scrollPtr, pixmap, width, elementBW, 
                        (scrollPtr->vertical) ? ARROW_DOWN : ARROW_RIGHT);
        break;
    case STYLE_XP:
        DrawArrowXPStyle(scrollPtr, pixmap, width, elementBW, 
                        (scrollPtr->vertical) ? ARROW_DOWN : ARROW_RIGHT);
        break;
    case STYLE_TK:
        DrawArrowTkStyle(scrollPtr, pixmap, width, elementBW, 
                        (scrollPtr->vertical) ? ARROW_DOWN : ARROW_RIGHT);
        break;
    }
    /*
     * Display the slider.
     */
    if (scrollPtr->activeField == SLIDER) {
        bg = scrollPtr->activeBg;
        relief = TK_RELIEF_RAISED;
    } else if (scrollPtr->activeField == SLIDER) {
        bg = scrollPtr->activeBg;
        relief = scrollPtr->activeRelief;
    } else {
        bg = scrollPtr->bg;
        relief = TK_RELIEF_RAISED;
    }
    if (scrollPtr->sliderLast > scrollPtr->sliderFirst) {
        if (scrollPtr->vertical) {
            Blt_Bg_FillRectangle(tkwin, pixmap, bg, scrollPtr->inset, 
                scrollPtr->sliderFirst, width, 
                scrollPtr->sliderLast - scrollPtr->sliderFirst,
                elementBW, relief);
        } else {
            Blt_Bg_FillRectangle(tkwin, pixmap, bg, scrollPtr->sliderFirst, 
                scrollPtr->inset, 
                scrollPtr->sliderLast - scrollPtr->sliderFirst, width,
                elementBW, relief);
        }
    }

    /*
     * Copy the information from the off-screen pixmap onto the screen, then
     * delete the pixmap.
     */

    XCopyArea(scrollPtr->display, pixmap, Tk_WindowId(tkwin),
        scrollPtr->copyGC, 0, 0, (unsigned)Tk_Width(tkwin),
        (unsigned)Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(scrollPtr->display, pixmap);
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various events on
 *      scrollbars.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      When the window gets deleted, internal structures get cleaned up.
 *      When it gets exposed, it is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
ScrollbarEventProc(
    ClientData clientData,      /* Information about window. */
    XEvent *eventPtr)           /* Information about event. */
{
    Scrollbar *scrollPtr = clientData;

    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
        EventuallyRedraw(scrollPtr);
    } else if (eventPtr->type == DestroyNotify) {
        if (scrollPtr->tkwin != NULL) {
            scrollPtr->tkwin = NULL;
            Tcl_DeleteCommandFromToken(scrollPtr->interp,scrollPtr->widgetCmd);
        }
        if (scrollPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayScrollbar, (ClientData)scrollPtr);
        }
        Tcl_EventuallyFree((ClientData)scrollPtr,
            (Tcl_FreeProc *)DestroyScrollbar);
    } else if (eventPtr->type == ConfigureNotify) {
        ComputeScrollbarGeometry(scrollPtr);
        EventuallyRedraw(scrollPtr);
    } else if (eventPtr->type == FocusIn) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            scrollPtr->flags |= GOT_FOCUS;
            if (scrollPtr->highlightWidth > 0) {
                EventuallyRedraw(scrollPtr);
            }
        }
    } else if (eventPtr->type == FocusOut) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            scrollPtr->flags &= ~GOT_FOCUS;
            if (scrollPtr->highlightWidth > 0) {
                EventuallyRedraw(scrollPtr);
            }
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarCmdDeletedProc --
 *
 *      This procedure is invoked when a widget command is deleted.  If the
 *      widget isn't already in the process of being destroyed, this command
 *      destroys it.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The widget is destroyed.
 *
 *---------------------------------------------------------------------------
 */
static void
ScrollbarCmdDeletedProc(ClientData clientData)
{
    Scrollbar *scrollPtr = clientData;
    Tk_Window tkwin = scrollPtr->tkwin;

    /*
     * This procedure could be invoked either because the window was destroyed
     * and the command was then deleted (in which case tkwin is NULL) or
     * because the command was deleted, and then this procedure destroys the
     * widget.
     */

    if (tkwin != NULL) {
        scrollPtr->tkwin = NULL;
        Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeScrollbarGeometry --
 *
 *      After changes in a scrollbar's size or configuration, this procedure
 *      recomputes various geometry information used in displaying the
 *      scrollbar.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The scrollbar will be displayed differently.
 *
 *---------------------------------------------------------------------------
 */

static void
ComputeScrollbarGeometry(Scrollbar *scrollPtr) 
{
    int width, fieldLength, xSize, ySize;

    if (scrollPtr->highlightWidth < 0) {
        scrollPtr->highlightWidth = 0;
    }
    scrollPtr->inset = scrollPtr->highlightWidth + scrollPtr->borderWidth;
    if (scrollPtr->vertical) {
        width = Tk_Width(scrollPtr->tkwin);
        fieldLength = Tk_Height(scrollPtr->tkwin);
    } else {
        width = Tk_Height(scrollPtr->tkwin);
        fieldLength = Tk_Width(scrollPtr->tkwin);
    }
    fieldLength -= 2 * (scrollPtr->arrowLength + scrollPtr->inset);
    if (fieldLength < 0) {
        fieldLength = 0;
    }
    scrollPtr->arrowLength = width - (2 * scrollPtr->inset + 1);
#ifdef notdef
    width = (scrollPtr->vertical) ? 
        Tk_Width(scrollPtr->tkwin) : Tk_Height(scrollPtr->tkwin);
    scrollPtr->arrowLength = width - (2 * scrollPtr->inset + 1);
    fieldLength = (scrollPtr->vertical ? 
                   Tk_Height(scrollPtr->tkwin) : 
                   Tk_Width(scrollPtr->tkwin)) - 
        2 * (scrollPtr->arrowLength + scrollPtr->inset);
    if (fieldLength < 0) {
        fieldLength = 0;
    }
#endif
    scrollPtr->sliderFirst = (int)(fieldLength * scrollPtr->firstFraction);
    scrollPtr->sliderLast = (int)(fieldLength * scrollPtr->lastFraction);

    /*
     * Adjust the slider so that some piece of it is always discrollPtrlayed in the
     * scrollbar and so that it has at least a minimal width (so it can be
     * grabbed with the mouse).
     */
    {
        int minSliderLength, sliderLength;

        minSliderLength = scrollPtr->minSliderLength;
        if (minSliderLength > fieldLength) {
            minSliderLength = fieldLength;
        }
        sliderLength = scrollPtr->sliderLast - scrollPtr->sliderFirst;
        if (sliderLength < minSliderLength) {
            fieldLength -= minSliderLength - sliderLength;
            scrollPtr->sliderFirst = 
                (int)(fieldLength * scrollPtr->firstFraction);
            scrollPtr->sliderLast = scrollPtr->sliderFirst + minSliderLength;
        } else {
            if (scrollPtr->sliderFirst > 
                (fieldLength - 2 * scrollPtr->borderWidth)) {
                scrollPtr->sliderFirst = fieldLength - 
                    2 * scrollPtr->borderWidth;
            }
            if (scrollPtr->sliderFirst < 0) {
                scrollPtr->sliderFirst = 0;
            }
            if (scrollPtr->sliderLast > fieldLength) {
                scrollPtr->sliderLast = fieldLength;
            }
        }
    }
    scrollPtr->sliderFirst += scrollPtr->arrowLength + scrollPtr->inset;
    scrollPtr->sliderLast  += scrollPtr->arrowLength + scrollPtr->inset;

    /*
     * Register the desired geometry for the window (leave enough space for
     * the two arrows plus a minimum-size slider, plus border around the whole
     * window, if any).  Then arrange for the window to be redisplayed.
     */

    if (scrollPtr->vertical) {
        xSize = scrollPtr->width + 2 * scrollPtr->inset;
        ySize = 2 * (scrollPtr->arrowLength + scrollPtr->borderWidth + 
                     scrollPtr->inset);
    } else {
        xSize = 2 * (scrollPtr->arrowLength + scrollPtr->borderWidth + 
                     scrollPtr->inset);
        ySize = scrollPtr->width + 2 * scrollPtr->inset;
    }
    Tk_GeometryRequest(scrollPtr->tkwin, xSize, ySize);
    Tk_SetInternalBorder(scrollPtr->tkwin, scrollPtr->inset);

    /* Assume that any geometry change requires the arrows to be
     * redrawn. */
    if (scrollPtr->upArrow != NULL) {
        Blt_FreePicture(scrollPtr->upArrow);
        scrollPtr->upArrow = NULL;
    }
    if (scrollPtr->downArrow != NULL) {
        Blt_FreePicture(scrollPtr->downArrow);
        scrollPtr->downArrow = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollbarPosition --
 *
 *      Determine the scrollbar element corresponding to a given position.
 *
 * Results:
 *      One of TOP_ARROW, TOP_GAP, etc., indicating which element of the
 *      scrollbar covers the position given by (x, y).  If (x,y) is outside
 *      the scrollbar entirely, then OUTSIDE is returned.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
ScrollbarPosition(
    Scrollbar *scrollPtr,       /* Scrollbar widget record. */
    int x, int y)               /* Coordinates within scrollPtr's 
                                 * window. */
{
    int length, width, tmp;

    if (scrollPtr->vertical) {
        length = Tk_Height(scrollPtr->tkwin);
        width = Tk_Width(scrollPtr->tkwin);
    } else {
        tmp = x;
        x = y;
        y = tmp;
        length = Tk_Width(scrollPtr->tkwin);
        width = Tk_Height(scrollPtr->tkwin);
    }

    if ((x < scrollPtr->inset) || (x >= (width - scrollPtr->inset)) || 
        (y < scrollPtr->inset) || (y >= (length - scrollPtr->inset))) {
        return OUTSIDE;
    }
    /*
     * All of the calculations in this procedure mirror those in
     * DisplayScrollbar.  Be sure to keep the two consistent.
     */

    if (y < (scrollPtr->inset + scrollPtr->arrowLength)) {
        return TOP_ARROW;
    }
    if (y < scrollPtr->sliderFirst) {
        return TOP_GAP;
    }
    if (y < scrollPtr->sliderLast) {
        return SLIDER;
    }
    if (y >= (length - (scrollPtr->arrowLength + scrollPtr->inset))) {
        return BOTTOM_ARROW;
    }
    return BOTTOM_GAP;
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *      Arrange for one or more of the fields of a scrollbar to be redrawn.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

static void
EventuallyRedraw(Scrollbar *scrollPtr) /* Information about widget. */
{
    if ((scrollPtr->tkwin == NULL) || (!Tk_IsMapped(scrollPtr->tkwin))) {
        return;
    }
    if ((scrollPtr->flags & REDRAW_PENDING) == 0) {
        Tcl_DoWhenIdle(DisplayScrollbar, (ClientData)scrollPtr);
        scrollPtr->flags |= REDRAW_PENDING;
    }
}

int
Blt_ScrollbarCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "scrollbar", ScrollbarCmd, };

    return Blt_InitCmd(interp, "::blt::tk", &cmdSpec);
}

#endif /* NO_TKSCROLLBAR */

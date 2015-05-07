/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * tkMenubutton.c --
 *
 *      This module implements button-like widgets that are used
 *      to invoke pull-down menus.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 *
 *   See the file "license.terms" for information on usage and
 *   redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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

/*
 * Defaults for menubuttons:
 */

#define DEF_MENUBUTTON_ANCHOR           "center"
#define DEF_MENUBUTTON_ACTIVE_BG        STD_ACTIVE_BACKGROUND
#define DEF_MENUBUTTON_ACTIVE_FG        STD_ACTIVE_FOREGROUND
#define DEF_MENUBUTTON_BG               STD_NORMAL_BACKGROUND
#define DEF_MENUBUTTON_BITMAP           ""
#define DEF_MENUBUTTON_BORDERWIDTH      "2"
#define DEF_MENUBUTTON_CURSOR           ""
#define DEF_MENUBUTTON_DIRECTION        "below"
#define DEF_MENUBUTTON_DISABLED_FG      STD_DISABLED_FOREGROUND
#define DEF_MENUBUTTON_FONT             "Helvetica -12 bold"
#define DEF_MENUBUTTON_FG               STD_NORMAL_FOREGROUND
#define DEF_MENUBUTTON_HEIGHT           "0"
#define DEF_MENUBUTTON_HIGHLIGHT_BG     DEF_MENUBUTTON_BG
#define DEF_MENUBUTTON_HIGHLIGHT        RGB_BLACK
#define DEF_MENUBUTTON_HIGHLIGHT_WIDTH  "0"
#define DEF_MENUBUTTON_IMAGE            (char *) NULL
#define DEF_MENUBUTTON_INDICATOR        "0"
#define DEF_MENUBUTTON_JUSTIFY          "center"
#define DEF_MENUBUTTON_MENU             ""
#define DEF_MENUBUTTON_PADX             "4p"
#define DEF_MENUBUTTON_PADY             "3p"
#define DEF_MENUBUTTON_RELIEF           "flat"
#define DEF_MENUBUTTON_STATE            "normal"
#define DEF_MENUBUTTON_TAKE_FOCUS       "0"
#define DEF_MENUBUTTON_TEXT             ""
#define DEF_MENUBUTTON_TEXT_VARIABLE    ""
#define DEF_MENUBUTTON_UNDERLINE        "-1"
#define DEF_MENUBUTTON_WIDTH            "0"
#define DEF_MENUBUTTON_WRAP_LENGTH      "0"

/*
 * A data structure of the following type is kept for each
 * widget managed by this file:
 */

typedef struct {
    Tk_Window tkwin;            /* Window that embodies the widget.  NULL
                                 * means that the window has been destroyed
                                 * but the data structures haven't yet been
                                 * cleaned up.*/
    Display *display;           /* Display containing widget.  Needed, among
                                 * other things, so that resources can bee
                                 * freed up even after tkwin has gone away. */
    Tcl_Interp *interp;         /* Interpreter associated with menubutton. */
    Tcl_Command widgetCmd;      /* Token for menubutton's widget command. */
    char *menuName;             /* Name of menu associated with widget.
                                 * Malloc-ed. */

    /*
     * Information about what's displayed in the menu button:
     */

    char *text;                 /* Text to display in button (malloc'ed)
                                 * or NULL. */
    int numChars;               /* # of characters in text. */
    int underline;              /* Index of character to underline. */
    char *textVarName;          /* Name of variable (malloc'ed) or NULL.
                                 * If non-NULL, button displays the contents
                                 * of this variable. */
    Pixmap bitmap;              /* Bitmap to display or None.  If not None
                                 * then text and textVar and underline
                                 * are ignored. */
    char *imageString;          /* Name of image to display (malloc'ed), or
                                 * NULL.  If non-NULL, bitmap, text, and
                                 * textVarName are ignored. */
    Tk_Image image;             /* Image to display in window, or NULL if
                                 * none. */

    /*
     * Information used when displaying widget:
     */

    int state;                  /* State of button for display purposes:
                                 * normal, active, or disabled. */
    Tk_3DBorder normalBorder;   /* Structure used to draw 3-D
                                 * border and background when window
                                 * isn't active.  NULL means no such
                                 * border exists. */
    Tk_3DBorder activeBorder;   /* Structure used to draw 3-D
                                 * border and background when window
                                 * is active.  NULL means no such
                                 * border exists. */
    int borderWidth;            /* Width of border. */
    int relief;                 /* 3-d effect: TK_RELIEF_RAISED, etc. */
    int highlightWidth;         /* Width in pixels of highlight to draw
                                 * around widget when it has the focus.
                                 * <= 0 means don't draw a highlight. */
    XColor *highlightBgColorPtr;
    /* Color for drawing traversal highlight
                                 * area when highlight is off. */
    XColor *highlightColorPtr;  /* Color for drawing traversal highlight. */
    int inset;                  /* Total width of all borders, including
                                 * traversal highlight and 3-D border.
                                 * Indicates how much interior stuff must
                                 * be offset from outside edges to leave
                                 * room for borders. */
    XFontStruct *fontPtr;       /* Information about text font, or NULL. */
    XColor *normalFg;           /* Foreground color in normal mode. */
    XColor *activeFg;           /* Foreground color in active mode.  NULL
                                 * means use normalFg instead. */
    XColor *disabledFg;         /* Foreground color when disabled.  NULL
                                 * means use normalFg with a 50% stipple
                                 * instead. */
    GC normalTextGC;            /* GC for drawing text in normal mode. */
    GC activeTextGC;            /* GC for drawing text in active mode (NULL
                                 * means use normalTextGC). */
    Pixmap gray;                /* Pixmap for displaying disabled text/icon if
                                 * disabledFg is NULL. */
    GC disabledGC;              /* Used to produce disabled effect.  If
                                 * disabledFg isn't NULL, this GC is used to
                                 * draw button text or icon.  Otherwise
                                 * text or icon is drawn with normalGC and
                                 * this GC is used to stipple background
                                 * across it. */
    int leftBearing;            /* Distance from text origin to leftmost drawn
                                 * pixel (positive means to right). */
    int rightBearing;           /* Amount text sticks right from its origin. */
    char *widthString;          /* Value of -width option.  Malloc'ed. */
    char *heightString;         /* Value of -height option.  Malloc'ed. */
    int width, height;          /* If > 0, these specify dimensions to request
                                 * for window, in characters for text and in
                                 * pixels for bitmaps.  In this case the actual
                                 * size of the text string or bitmap is
                                 * ignored in computing desired window size. */
    int wrapLength;             /* Line length (in pixels) at which to wrap
                                 * onto next line.  <= 0 means don't wrap
                                 * except at newlines. */
    int xPad, yPad;             /* Extra space around text or bitmap (pixels
                                 * on each side). */
    Tk_Anchor anchor;           /* Where text/bitmap should be displayed
                                 * inside window region. */
    Tk_Justify justify;         /* Justification to use for multi-line text. */
    int textWidth;              /* Width needed to display text as requested,
                                 * in pixels. */
    int textHeight;             /* Height needed to display text as requested,
                                 * in pixels. */
    int indicatorOn;            /* Non-zero means display indicator;  0 means
                                 * don't display. */
    int indicatorHeight;        /* Height of indicator in pixels.  This same
                                 * amount of extra space is also left on each
                                 * side of the indicator. 0 if no indicator. */
    int indicatorWidth;         /* Width of indicator in pixels, including
                                 * indicatorHeight in padding on each side.
                                 * 0 if no indicator. */

    /*
     * Miscellaneous information:
     */

    Tk_Cursor cursor;           /* Current cursor for window, or None. */
    char *takeFocus;            /* Value of -takefocus option;  not used in
                                 * the C code, but used by keyboard traversal
                                 * scripts.  Malloc'ed, but may be NULL. */
    int flags;                  /* Various flags;  see below for
                                 * definitions. */
} MenuButton;

/*
 * Flag bits for buttons:
 *
 * REDRAW_PENDING:              Non-zero means a DoWhenIdle handler
 *                              has already been queued to redraw
 *                              this window.
 * POSTED:                      Non-zero means that the menu associated
 *                              with this button has been posted (typically
 *                              because of an active button press).
 * GOT_FOCUS:                   Non-zero means this button currently
 *                              has the input focus.
 */

#define REDRAW_PENDING          1
#define POSTED                  2
#define GOT_FOCUS               4

/*
 * The following constants define the dimensions of the cascade indicator,
 * which is displayed if the "-indicatoron" option is true.  The units for
 * these options are 1/10 millimeters.
 */

#define INDICATOR_WIDTH         40
#define INDICATOR_HEIGHT        17

/*
 * Information used for parsing configuration specs:
 */

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BORDER, "-activebackground", "activeBackground", "Foreground",
        DEF_MENUBUTTON_ACTIVE_BG, Blt_Offset(MenuButton, activeBorder), 0},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground", "Background",
        DEF_MENUBUTTON_ACTIVE_FG, Blt_Offset(MenuButton, activeFg), 0},
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",
        DEF_MENUBUTTON_ANCHOR, Blt_Offset(MenuButton, anchor), 0},
    {BLT_CONFIG_BORDER, "-background", "background", "Background",
        DEF_MENUBUTTON_BG, Blt_Offset(MenuButton, normalBorder), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL,
        (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL,
        (char *)NULL, 0, 0},
    {BLT_CONFIG_BITMAP, "-bitmap", "bitmap", "Bitmap", DEF_MENUBUTTON_BITMAP, 
        Blt_Offset(MenuButton, bitmap), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_MENUBUTTON_BORDERWIDTH, Blt_Offset(MenuButton, borderWidth), 0},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor", 
        DEF_MENUBUTTON_CURSOR, Blt_Offset(MenuButton, cursor), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
        "DisabledForeground", DEF_MENUBUTTON_DISABLED_FG, 
        Blt_Offset(MenuButton, disabledFg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_MENUBUTTON_FONT, 
        Blt_Offset(MenuButton, fontPtr), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
        DEF_MENUBUTTON_FG, Blt_Offset(MenuButton, normalFg), 0},
    {BLT_CONFIG_STRING, "-height", "height", "Height",
        DEF_MENUBUTTON_HEIGHT, Blt_Offset(MenuButton, heightString), 0},
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
        "HighlightBackground", DEF_MENUBUTTON_HIGHLIGHT_BG,
        Blt_Offset(MenuButton, highlightBgColorPtr), 0},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
        DEF_MENUBUTTON_HIGHLIGHT, Blt_Offset(MenuButton, highlightColorPtr), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
        "HighlightThickness", DEF_MENUBUTTON_HIGHLIGHT_WIDTH,
        Blt_Offset(MenuButton, highlightWidth), 0},
    {BLT_CONFIG_STRING, "-image", "image", "Image", DEF_MENUBUTTON_IMAGE, 
        Blt_Offset(MenuButton, imageString), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-indicatoron", "indicatorOn", "IndicatorOn",
        DEF_MENUBUTTON_INDICATOR, Blt_Offset(MenuButton, indicatorOn), 0},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", 
        DEF_MENUBUTTON_JUSTIFY, Blt_Offset(MenuButton, justify), 0},
    {BLT_CONFIG_STRING, "-menu", "menu", "Menu", DEF_MENUBUTTON_MENU, 
        Blt_Offset(MenuButton, menuName), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-padx", "padX", "Pad", DEF_MENUBUTTON_PADX, 
        Blt_Offset(MenuButton, xPad), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-pady", "padY", "Pad", DEF_MENUBUTTON_PADY, 
        Blt_Offset(MenuButton, yPad), 0}, 
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_MENUBUTTON_RELIEF, 
        Blt_Offset(MenuButton, relief), 0},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_MENUBUTTON_STATE, 
        Blt_Offset(MenuButton, state), 0},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
        DEF_MENUBUTTON_TAKE_FOCUS, Blt_Offset(MenuButton, takeFocus),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-text", "text", "Text", DEF_MENUBUTTON_TEXT, 
        Blt_Offset(MenuButton, text), 0},
    {BLT_CONFIG_STRING, "-textvariable", "textVariable", "Variable",
        DEF_MENUBUTTON_TEXT_VARIABLE, Blt_Offset(MenuButton, textVarName),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_INT, "-underline", "underline", "Underline",
        DEF_MENUBUTTON_UNDERLINE, Blt_Offset(MenuButton, underline), 0},
    {BLT_CONFIG_STRING, "-width", "width", "Width", DEF_MENUBUTTON_WIDTH, 
        Blt_Offset(MenuButton, widthString), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-wraplength", "wrapLength", "WrapLength",
        DEF_MENUBUTTON_WRAP_LENGTH, Blt_Offset(MenuButton, wrapLength), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
        (char *)NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static Tcl_CmdDeleteProc MenuButtonCmdDeletedProc;
static Tk_EventProc MenuButtonEventProc;
static Tk_ImageChangedProc MenuButtonImageProc;
static Tcl_VarTraceProc MenuButtonTextVarProc;
static Tcl_ObjCmdProc MenuButtonWidgetCmd;
static Tcl_FreeProc DestroyMenuButton;
static Tcl_IdleProc DisplayMenuButton;

static int ConfigureMenuButton (Tcl_Interp *interp, MenuButton *mbPtr, 
        int objc, Tcl_Obj *const *objv, int flags);
static void ComputeMenuButtonGeometry (MenuButton *mbPtr);
/*
 *---------------------------------------------------------------------------
 *
 * Tk_MenubuttonCmd --
 *
 *      This procedure is invoked to process the "button", "label",
 *      "radiobutton", and "checkbutton" TCL commands.  See the
 *      user documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

int
Tk_MenubuttonCmd(
    ClientData clientData,      /* Main window associated with
                                 * interpreter. */
    Tcl_Interp *interp,         /* Current interpreter. */
    int objc,                   /* Number of arguments. */
    Tcl_Obj *const *objv)       /* Argument strings. */
{
    register MenuButton *mbPtr;
    Tk_Window tkwin;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
            Tcl_GetString(objv[0]), " pathName ?options?\"", (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * Create the new window.
     */

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), 
                Tcl_GetString(objv[1]), (char *)NULL);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    /*
     * Initialize the data structure for the button.
     */

    mbPtr = Blt_AssertCalloc(1, sizeof(MenuButton));
    mbPtr->tkwin = tkwin;
    mbPtr->display = Tk_Display(tkwin);
    mbPtr->interp = interp;
    mbPtr->widgetCmd = Tcl_CreateObjCommand(interp, Tk_PathName(mbPtr->tkwin),
        MenuButtonWidgetCmd, (ClientData)mbPtr, MenuButtonCmdDeletedProc);
    mbPtr->underline = -1;
    mbPtr->state = STATE_NORMAL;
    mbPtr->relief = TK_RELIEF_FLAT;
    mbPtr->anchor = TK_ANCHOR_CENTER;
    mbPtr->justify = TK_JUSTIFY_CENTER;

    Tk_SetClass(mbPtr->tkwin, "Menubutton");
    Tk_CreateEventHandler(mbPtr->tkwin,
        ExposureMask | StructureNotifyMask | FocusChangeMask,
        MenuButtonEventProc, (ClientData)mbPtr);
    if (ConfigureMenuButton(interp, mbPtr, objc - 2, objv + 2, 0) != TCL_OK) {
        Tk_DestroyWindow(mbPtr->tkwin);
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objv[1]);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MenuButtonWidgetCmd --
 *
 *      This procedure is invoked to process the TCL command
 *      that corresponds to a widget managed by this module.
 *      See the user documentation for details on what it does.
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
MenuButtonWidgetCmd(
    ClientData clientData,      /* Information about button widget. */
    Tcl_Interp *interp,         /* Current interpreter. */
    int objc,                   /* Number of arguments. */
    Tcl_Obj *const *objv)       /* Argument strings. */
{
    MenuButton *mbPtr = clientData;
    char *string;
    int c;
    int length;
    int result = TCL_OK;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " option ?arg arg ...?\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    Tcl_Preserve(mbPtr);
    string = Tcl_GetString(objv[1], &length);
    c = string[0];
    if ((c == 'c') && (length >= 2) && (strncmp(string, "cget", length) == 0)) {
        if (objc != 3) {
            Tcl_AppendResult(interp, "wrong # args: should be \"",
                Tcl_GetString(objv[0]), " cget option\"", (char *)NULL);
            goto error;
        }
        result = Blt_ConfigureValueFromObj(interp, mbPtr->tkwin, configSpecs,
            (char *)mbPtr, objv[2], 0);
    } else if ((c == 'c') && (length >= 2) && 
               (strncmp(string, "configure", length) == 0)) {
        if (objc == 2) {
            result = Blt_ConfigureInfoFromObj(interp, mbPtr->tkwin, configSpecs,
                (char *)mbPtr, (Tcl_Obj *)NULL, 0);
        } else if (objc == 3) {
            result = Blt_ConfigureInfoFromObj(interp, mbPtr->tkwin, configSpecs,
                (char *)mbPtr, objv[2], 0);
        } else {
            result = ConfigureMenuButton(interp, mbPtr, objc - 2, objv + 2,
                BLT_CONFIG_OBJV_ONLY);
        }
    } else {
        Tcl_AppendResult(interp, "bad option \"", string,
            "\": must be cget or configure", (char *)NULL);
        goto error;
    }
    Tcl_Release(mbPtr);
    return result;

  error:
    Tcl_Release(mbPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyMenuButton --
 *
 *      This procedure is invoked to recycle all of the resources
 *      associated with a button widget.  It is invoked as a
 *      when-idle handler in order to make sure that there is no
 *      other use of the button pending at the time of the deletion.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the widget is freed up.
 *
 *---------------------------------------------------------------------------
 */

static void
DestroyMenuButton(char *memPtr) /* Info about button widget. */
{
    register MenuButton *mbPtr = (MenuButton *)memPtr;

    /*
     * Free up all the stuff that requires special handling, then
     * let Blt_FreeOptions handle all the standard option-related
     * stuff.
     */

    if (mbPtr->textVarName != NULL) {
        Tcl_UntraceVar(mbPtr->interp, mbPtr->textVarName,
            TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
            MenuButtonTextVarProc, (ClientData)mbPtr);
    }
    if (mbPtr->image != NULL) {
        Tk_FreeImage(mbPtr->image);
    }
    if (mbPtr->normalTextGC != None) {
        Tk_FreeGC(mbPtr->display, mbPtr->normalTextGC);
    }
    if (mbPtr->activeTextGC != None) {
        Tk_FreeGC(mbPtr->display, mbPtr->activeTextGC);
    }
    if (mbPtr->gray != None) {
        Tk_FreeBitmap(mbPtr->display, mbPtr->gray);
    }
    if (mbPtr->disabledGC != None) {
        Tk_FreeGC(mbPtr->display, mbPtr->disabledGC);
    }
    Blt_FreeOptions(configSpecs, (char *)mbPtr, mbPtr->display, 0);
    Blt_Free(mbPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureMenuButton --
 *
 *      This procedure is called to process an objv/objc list, plus
 *      the Tk option database, in order to configure (or
 *      reconfigure) a menubutton widget.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is
 *      returned, then interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for mbPtr;  old resources get freed, if there
 *      were any.  The menubutton is redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static int
ConfigureMenuButton(
    Tcl_Interp *interp,         /* Used for error reporting. */
    MenuButton *mbPtr,          /* Information about widget;  may or may
                                 * not already have values for some fields. */
    int objc,                   /* Number of valid entries in objv. */
    Tcl_Obj *const *objv,       /* Arguments. */
    int flags)                  /* Flags to pass to Blt_ConfigureWidget. */
{
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    int result;
    Tk_Image image;

    /*
     * Eliminate any existing trace on variables monitored by the menubutton.
     */

    if (mbPtr->textVarName != NULL) {
        Tcl_UntraceVar(interp, mbPtr->textVarName,
            TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
            MenuButtonTextVarProc, (ClientData)mbPtr);
    }
    result = Blt_ConfigureWidgetFromObj(interp, mbPtr->tkwin, configSpecs,
        objc, objv, (char *)mbPtr, flags);
    if (result != TCL_OK) {
        return TCL_ERROR;
    }
    /*
     * A few options need special processing, such as setting the
     * background from a 3-D border, or filling in complicated
     * defaults that couldn't be specified to Blt_ConfigureWidget.
     */

    if ((mbPtr->state == STATE_ACTIVE) && !Tk_StrictMotif(mbPtr->tkwin)) {
        Tk_SetBackgroundFromBorder(mbPtr->tkwin, mbPtr->activeBorder);
    } else {
        Tk_SetBackgroundFromBorder(mbPtr->tkwin, mbPtr->normalBorder);
        if ((mbPtr->state != STATE_NORMAL) && (mbPtr->state != STATE_ACTIVE)
            && (mbPtr->state != STATE_DISABLED)) {
            Tcl_AppendResult(interp, "bad state value \"", 
                Blt_Itoa(mbPtr->state),
                "\": must be normal, active, or disabled", (char *)NULL);
            mbPtr->state = STATE_NORMAL;
            return TCL_ERROR;
        }
    }

    if (mbPtr->highlightWidth < 0) {
        mbPtr->highlightWidth = 0;
    }
    gcValues.font = mbPtr->fontPtr->fid;
    gcValues.foreground = mbPtr->normalFg->pixel;
    gcValues.background = Tk_3DBorderColor(mbPtr->normalBorder)->pixel;

    /*
     * Note: GraphicsExpose events are disabled in GC's because they're
     * used to copy stuff from an off-screen pixmap onto the screen (we know
     * that there's no problem with obscured areas).
     */

    gcValues.graphics_exposures = False;
    newGC = Tk_GetGC(mbPtr->tkwin,
        GCForeground | GCBackground | GCFont | GCGraphicsExposures, &gcValues);
    if (mbPtr->normalTextGC != None) {
        Tk_FreeGC(mbPtr->display, mbPtr->normalTextGC);
    }
    mbPtr->normalTextGC = newGC;

    gcValues.font = mbPtr->fontPtr->fid;
    gcValues.foreground = mbPtr->activeFg->pixel;
    gcValues.background = Tk_3DBorderColor(mbPtr->activeBorder)->pixel;
    newGC = Tk_GetGC(mbPtr->tkwin, GCForeground | GCBackground | GCFont,
        &gcValues);
    if (mbPtr->activeTextGC != None) {
        Tk_FreeGC(mbPtr->display, mbPtr->activeTextGC);
    }
    mbPtr->activeTextGC = newGC;

    gcValues.font = mbPtr->fontPtr->fid;
    gcValues.background = Tk_3DBorderColor(mbPtr->normalBorder)->pixel;
    if ((mbPtr->disabledFg != NULL) && (mbPtr->imageString == NULL)) {
        gcValues.foreground = mbPtr->disabledFg->pixel;
        mask = GCForeground | GCBackground | GCFont;
    } else {
        gcValues.foreground = gcValues.background;
        if (mbPtr->gray == None) {
            mbPtr->gray = Tk_GetBitmap(interp, mbPtr->tkwin,
                Tk_GetUid("gray50"));
            if (mbPtr->gray == None) {
                return TCL_ERROR;
            }
        }
        gcValues.fill_style = FillStippled;
        gcValues.stipple = mbPtr->gray;
        mask = GCForeground | GCFillStyle | GCStipple;
    }
    newGC = Tk_GetGC(mbPtr->tkwin, mask, &gcValues);
    if (mbPtr->disabledGC != None) {
        Tk_FreeGC(mbPtr->display, mbPtr->disabledGC);
    }
    mbPtr->disabledGC = newGC;

    if (mbPtr->xPad < 0) {
        mbPtr->xPad = 0;
    }
    if (mbPtr->yPad < 0) {
        mbPtr->yPad = 0;
    }
    /*
     * Get the image for the widget, if there is one.  Allocate the
     * new image before freeing the old one, so that the reference
     * count doesn't go to zero and cause image data to be discarded.
     */

    if (mbPtr->imageString != NULL) {
        image = Tk_GetImage(mbPtr->interp, mbPtr->tkwin,
            mbPtr->imageString, MenuButtonImageProc, (ClientData)mbPtr);
        if (image == NULL) {
            return TCL_ERROR;
        }
    } else {
        image = NULL;
    }
    if (mbPtr->image != NULL) {
        Tk_FreeImage(mbPtr->image);
    }
    mbPtr->image = image;

    if ((mbPtr->image == NULL) && (mbPtr->bitmap == None)
        && (mbPtr->textVarName != NULL)) {
        /*
         * The menubutton displays a variable.  Set up a trace to watch
         * for any changes in it.
         */

        char *value;

        value = Tcl_GetVar(interp, mbPtr->textVarName, TCL_GLOBAL_ONLY);
        if (value == NULL) {
            Tcl_SetVar(interp, mbPtr->textVarName, mbPtr->text,
                TCL_GLOBAL_ONLY);
        } else {
            if (mbPtr->text != NULL) {
                Blt_Free(mbPtr->text);
            }
            mbPtr->text = Blt_StrdupAsset(value);
        }
        Tcl_TraceVar(interp, mbPtr->textVarName,
            TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
            MenuButtonTextVarProc, (ClientData)mbPtr);
    }
    /*
     * Recompute the geometry for the button.
     */

    if ((mbPtr->bitmap != None) || (mbPtr->image != NULL)) {
        if (Tk_GetPixels(interp, mbPtr->tkwin, mbPtr->widthString,
                &mbPtr->width) != TCL_OK) {
          widthError:
            Tcl_AddErrorInfo(interp, "\n    (processing -width option)");
            return TCL_ERROR;
        }
        if (Tk_GetPixels(interp, mbPtr->tkwin, mbPtr->heightString,
                &mbPtr->height) != TCL_OK) {
          heightError:
            Tcl_AddErrorInfo(interp, "\n    (processing -height option)");
            return TCL_ERROR;
        }
    } else {
        if (Tcl_GetInt(interp, mbPtr->widthString, &mbPtr->width)
            != TCL_OK) {
            goto widthError;
        }
        if (Tcl_GetInt(interp, mbPtr->heightString, &mbPtr->height)
            != TCL_OK) {
            goto heightError;
        }
    }
    ComputeMenuButtonGeometry(mbPtr);

    /*
     * Lastly, arrange for the button to be redisplayed.
     */

    if (Tk_IsMapped(mbPtr->tkwin) && !(mbPtr->flags & REDRAW_PENDING)) {
        Tcl_DoWhenIdle(DisplayMenuButton, (ClientData)mbPtr);
        mbPtr->flags |= REDRAW_PENDING;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayMenuButton --
 *
 *      This procedure is invoked to display a menubutton widget.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Commands are output to X to display the menubutton in its
 *      current mode.
 *
 *---------------------------------------------------------------------------
 */

static void
DisplayMenuButton(ClientData clientData) /* Information about widget. */
{
    register MenuButton *mbPtr = clientData;
    GC gc;
    Tk_3DBorder border;
    Pixmap pixmap;
    int x = 0;                  /* Initialization needed only to stop
                                 * compiler warning. */
    int y;
    register Tk_Window tkwin = mbPtr->tkwin;
    int width, height;

    mbPtr->flags &= ~REDRAW_PENDING;
    if ((mbPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
        return;
    }
    if ((mbPtr->state == STATE_DISABLED) && (mbPtr->disabledFg != NULL)) {
        gc = mbPtr->disabledGC;
        border = mbPtr->normalBorder;
    } else if ((mbPtr->state == STATE_ACTIVE) && !Tk_StrictMotif(mbPtr->tkwin)) {
        gc = mbPtr->activeTextGC;
        border = mbPtr->activeBorder;
    } else {
        gc = mbPtr->normalTextGC;
        border = mbPtr->normalBorder;
    }

    /*
     * In order to avoid screen flashes, this procedure redraws
     * the menu button in a pixmap, then copies the pixmap to the
     * screen in a single operation.  This means that there's no
     * point in time where the on-sreen image has been cleared.
     */

    pixmap = Blt_GetPixmap(mbPtr->display, Tk_WindowId(tkwin),
        Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
    Blt_Fill3DRectangle(tkwin, pixmap, border, 0, 0, Tk_Width(tkwin),
        Tk_Height(tkwin), 0, TK_RELIEF_FLAT);

    /*
     * Display image or bitmap or text for button.
     */

    if (mbPtr->image != None) {
        Tk_SizeOfImage(mbPtr->image, &width, &height);

      imageOrBitmap:
        switch (mbPtr->anchor) {
        case TK_ANCHOR_NW:
        case TK_ANCHOR_W:
        case TK_ANCHOR_SW:
            x += mbPtr->inset;
            break;
        case TK_ANCHOR_N:
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_S:
            x += ((int)(Tk_Width(tkwin) - width
                    - mbPtr->indicatorWidth)) / 2;
            break;
        default:
            x += Tk_Width(tkwin) - mbPtr->inset - width
                - mbPtr->indicatorWidth;
            break;
        }
        switch (mbPtr->anchor) {
        case TK_ANCHOR_NW:
        case TK_ANCHOR_N:
        case TK_ANCHOR_NE:
            y = mbPtr->inset;
            break;
        case TK_ANCHOR_W:
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_E:
            y = ((int)(Tk_Height(tkwin) - height)) / 2;
            break;
        default:
            y = Tk_Height(tkwin) - mbPtr->inset - height;
            break;
        }
        if (mbPtr->image != NULL) {
            Tk_RedrawImage(mbPtr->image, 0, 0, width, height, pixmap,
                x, y);
        } else {
            XCopyPlane(mbPtr->display, mbPtr->bitmap, pixmap,
                gc, 0, 0, (unsigned)width, (unsigned)height, x, y, 1);
        }
    } else if (mbPtr->bitmap != None) {
        Tk_SizeOfBitmap(mbPtr->display, mbPtr->bitmap, &width, &height);
        goto imageOrBitmap;
    } else {
        width = mbPtr->textWidth;
        height = mbPtr->textHeight;
        switch (mbPtr->anchor) {
        case TK_ANCHOR_NW:
        case TK_ANCHOR_W:
        case TK_ANCHOR_SW:
            x = mbPtr->inset + mbPtr->xPad;
            break;
        case TK_ANCHOR_N:
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_S:
            x = ((int)(Tk_Width(tkwin) - width
                    - mbPtr->indicatorWidth)) / 2;
            break;
        default:
            x = Tk_Width(tkwin) - width - mbPtr->xPad - mbPtr->inset
                - mbPtr->indicatorWidth;
            break;
        }
        switch (mbPtr->anchor) {
        case TK_ANCHOR_NW:
        case TK_ANCHOR_N:
        case TK_ANCHOR_NE:
            y = mbPtr->inset + mbPtr->yPad;
            break;
        case TK_ANCHOR_W:
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_E:
            y = ((int)(Tk_Height(tkwin) - height)) / 2;
            break;
        default:
            y = Tk_Height(tkwin) - mbPtr->inset - mbPtr->yPad - height;
            break;
        }
        TkDisplayText(mbPtr->display, pixmap, mbPtr->fontPtr,
            mbPtr->text, mbPtr->numChars, x, y, mbPtr->textWidth,
            mbPtr->justify, mbPtr->underline, gc);
    }

    /*
     * If the menu button is disabled with a stipple rather than a special
     * foreground color, generate the stippled effect.
     */

    if ((mbPtr->state == STATE_DISABLED)
        && ((mbPtr->disabledFg == NULL) || (mbPtr->image != NULL))) {
        XFillRectangle(mbPtr->display, pixmap, mbPtr->disabledGC,
            mbPtr->inset, mbPtr->inset,
            (unsigned)(Tk_Width(tkwin) - 2 * mbPtr->inset),
            (unsigned)(Tk_Height(tkwin) - 2 * mbPtr->inset));
    }
    /*
     * Draw the cascade indicator for the menu button on the
     * right side of the window, if desired.
     */

    if (mbPtr->indicatorOn) {
        int borderWidth;

        borderWidth = (mbPtr->indicatorHeight + 1) / 3;
        if (borderWidth < 1) {
            borderWidth = 1;
        }
        Blt_Fill3DRectangle(tkwin, pixmap, border,
            Tk_Width(tkwin) - mbPtr->inset - mbPtr->indicatorWidth
            + mbPtr->indicatorHeight,
            y + ((int)(height - mbPtr->indicatorHeight)) / 2,
            mbPtr->indicatorWidth - 2 * mbPtr->indicatorHeight,
            mbPtr->indicatorHeight, borderWidth, TK_RELIEF_RAISED);
    }
    /*
     * Draw the border and traversal highlight last.  This way, if the
     * menu button's contents overflow onto the border they'll be covered
     * up by the border.
     */

    if (mbPtr->relief != TK_RELIEF_FLAT) {
        Blt_Draw3DRectangle(tkwin, pixmap, border,
            mbPtr->highlightWidth, mbPtr->highlightWidth,
            Tk_Width(tkwin) - 2 * mbPtr->highlightWidth,
            Tk_Height(tkwin) - 2 * mbPtr->highlightWidth,
            mbPtr->borderWidth, mbPtr->relief);
    }
    if (mbPtr->highlightWidth != 0) {
        GC gc;

        if (mbPtr->flags & GOT_FOCUS) {
            gc = Tk_GCForColor(mbPtr->highlightColorPtr, pixmap);
        } else {
            gc = Tk_GCForColor(mbPtr->highlightBgColorPtr, pixmap);
        }
        Tk_DrawFocusHighlight(tkwin, gc, mbPtr->highlightWidth, pixmap);
    }
    /*
     * Copy the information from the off-screen pixmap onto the screen,
     * then delete the pixmap.
     */

    XCopyArea(mbPtr->display, pixmap, Tk_WindowId(tkwin),
        mbPtr->normalTextGC, 0, 0, (unsigned)Tk_Width(tkwin),
        (unsigned)Tk_Height(tkwin), 0, 0);
    Tk_FreePixmap(mbPtr->display, pixmap);
}

/*
 *---------------------------------------------------------------------------
 *
 * MenuButtonEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various
 *      events on buttons.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      When the window gets deleted, internal structures get
 *      cleaned up.  When it gets exposed, it is redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static void
MenuButtonEventProc(
    ClientData clientData,      /* Information about window. */
    XEvent *eventPtr)           /* Information about event. */
{
    MenuButton *mbPtr = clientData;
    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
        goto redraw;
    } else if (eventPtr->type == ConfigureNotify) {
        /*
         * Must redraw after size changes, since layout could have changed
         * and borders will need to be redrawn.
         */

        goto redraw;
    } else if (eventPtr->type == DestroyNotify) {
        if (mbPtr->tkwin != NULL) {
            mbPtr->tkwin = NULL;
            Tcl_DeleteCommandFromToken(mbPtr->interp, mbPtr->widgetCmd);
        }
        if (mbPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayMenuButton, (ClientData)mbPtr);
        }
        Tcl_EventuallyFree((ClientData)mbPtr, DestroyMenuButton);
    } else if (eventPtr->type == FocusIn) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            mbPtr->flags |= GOT_FOCUS;
            if (mbPtr->highlightWidth > 0) {
                goto redraw;
            }
        }
    } else if (eventPtr->type == FocusOut) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            mbPtr->flags &= ~GOT_FOCUS;
            if (mbPtr->highlightWidth > 0) {
                goto redraw;
            }
        }
    }
    return;

  redraw:
    if ((mbPtr->tkwin != NULL) && !(mbPtr->flags & REDRAW_PENDING)) {
        Tcl_DoWhenIdle(DisplayMenuButton, (ClientData)mbPtr);
        mbPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MenuButtonCmdDeletedProc --
 *
 *      This procedure is invoked when a widget command is deleted.  If
 *      the widget isn't already in the process of being destroyed,
 *      this command destroys it.
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
MenuButtonCmdDeletedProc(ClientData clientData) /* Pointer to widget
                                                   record for
                                                   widget. */
{
    MenuButton *mbPtr = clientData;
    Tk_Window tkwin = mbPtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
        mbPtr->tkwin = NULL;
        Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeMenuButtonGeometry --
 *
 *      After changes in a menu button's text or bitmap, this procedure
 *      recomputes the menu button's geometry and passes this information
 *      along to the geometry manager for the window.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The menu button's window may change size.
 *
 *---------------------------------------------------------------------------
 */

static void
ComputeMenuButtonGeometry(MenuButton *mbPtr) 
{
    int width, height, mm, pixels;

    mbPtr->inset = mbPtr->highlightWidth + mbPtr->borderWidth;
    if (mbPtr->image != None) {
        Tk_SizeOfImage(mbPtr->image, &width, &height);
        if (mbPtr->width > 0) {
            width = mbPtr->width;
        }
        if (mbPtr->height > 0) {
            height = mbPtr->height;
        }
    } else if (mbPtr->bitmap != None) {
        Tk_SizeOfBitmap(mbPtr->display, mbPtr->bitmap, &width, &height);
        if (mbPtr->width > 0) {
            width = mbPtr->width;
        }
        if (mbPtr->height > 0) {
            height = mbPtr->height;
        }
    } else {
        mbPtr->numChars = strlen(mbPtr->text);
        TkComputeTextGeometry(mbPtr->fontPtr, mbPtr->text,
            mbPtr->numChars, mbPtr->wrapLength, &mbPtr->textWidth,
            &mbPtr->textHeight);
        width = mbPtr->textWidth;
        height = mbPtr->textHeight;
        if (mbPtr->width > 0) {
            width = mbPtr->width * XTextWidth(mbPtr->fontPtr, "0", 1);
        }
        if (mbPtr->height > 0) {
            height = mbPtr->height * (mbPtr->fontPtr->ascent
                + mbPtr->fontPtr->descent);
        }
        width += 2 * mbPtr->xPad;
        height += 2 * mbPtr->yPad;
    }

    if (mbPtr->indicatorOn) {
        mm = WidthMMOfScreen(Tk_Screen(mbPtr->tkwin));
        pixels = WidthOfScreen(Tk_Screen(mbPtr->tkwin));
        mbPtr->indicatorHeight = (INDICATOR_HEIGHT * pixels) / (10 * mm);
        mbPtr->indicatorWidth = (INDICATOR_WIDTH * pixels) / (10 * mm)
            + 2 * mbPtr->indicatorHeight;
        width += mbPtr->indicatorWidth;
    } else {
        mbPtr->indicatorHeight = 0;
        mbPtr->indicatorWidth = 0;
    }

    Tk_GeometryRequest(mbPtr->tkwin, (int)(width + 2 * mbPtr->inset),
        (int)(height + 2 * mbPtr->inset));
    Tk_SetInternalBorder(mbPtr->tkwin, mbPtr->inset);
}

/*
 *---------------------------------------------------------------------------
 *
 * MenuButtonTextVarProc --
 *
 *      This procedure is invoked when someone changes the variable
 *      whose contents are to be displayed in a menu button.
 *
 * Results:
 *      NULL is always returned.
 *
 * Side effects:
 *      The text displayed in the menu button will change to match the
 *      variable.
 *
 *---------------------------------------------------------------------------
 */

 /* ARGSUSED */
static char *
MenuButtonTextVarProc(
    ClientData clientData,      /* Information about button. */
    Tcl_Interp *interp,         /* Interpreter containing variable. */
    char *name1,                /* Name of variable. */
    char *name2,                /* Second part of variable name. */
    int flags)                  /* Information about what happened. */
{
    register MenuButton *mbPtr = clientData;
    char *value;

    /*
     * If the variable is unset, then immediately recreate it unless
     * the whole interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
        if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
            Tcl_SetVar(interp, mbPtr->textVarName, mbPtr->text,
                TCL_GLOBAL_ONLY);
            Tcl_TraceVar(interp, mbPtr->textVarName,
                TCL_GLOBAL_ONLY | TCL_TRACE_WRITES | TCL_TRACE_UNSETS,
                MenuButtonTextVarProc, clientData);
        }
        return (char *) NULL;
    }
    value = Tcl_GetVar(interp, mbPtr->textVarName, TCL_GLOBAL_ONLY);
    if (value == NULL) {
        value = "";
    }
    if (mbPtr->text != NULL) {
        Blt_Free(mbPtr->text);
    }
    mbPtr->text = Blt_AssertStrdup(value);
    ComputeMenuButtonGeometry(mbPtr);

    if ((mbPtr->tkwin != NULL) && Tk_IsMapped(mbPtr->tkwin)
        && !(mbPtr->flags & REDRAW_PENDING)) {
        Tcl_DoWhenIdle(DisplayMenuButton, (ClientData)mbPtr);
        mbPtr->flags |= REDRAW_PENDING;
    }
    return (char *) NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * MenuButtonImageProc --
 *
 *      This procedure is invoked by the image code whenever the manager
 *      for an image does something that affects the size of contents
 *      of an image displayed in a button.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the button to get redisplayed.
 *
 *---------------------------------------------------------------------------
 */

static void
MenuButtonImageProc(
    ClientData clientData,      /* Pointer to widget record. */
    int x, int y,               /* Upper left pixel (within image)
                                 * that must be redisplayed. */
    int width, int height,      /* Dimensions of area to redisplay
                                 * (may be <= 0). */
    int imgWidth, int imgHeight) /* New dimensions of image. */
{
    register MenuButton *mbPtr = clientData;

    if (mbPtr->tkwin != NULL) {
        ComputeMenuButtonGeometry(mbPtr);
        if (Tk_IsMapped(mbPtr->tkwin) && !(mbPtr->flags & REDRAW_PENDING)) {
            Tcl_DoWhenIdle(DisplayMenuButton, (ClientData)mbPtr);
            mbPtr->flags |= REDRAW_PENDING;
        }
    }
}

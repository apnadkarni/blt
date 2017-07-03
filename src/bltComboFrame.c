/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltComboFrame.c --
 *
 * This module implements a comboframe widget for the BLT toolkit.
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

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include <bltAlloc.h>
#include "bltChain.h"
#include "bltHash.h"
#include "bltImage.h"
#include "bltBg.h"
#include "bltSwitch.h"
#include "bltOp.h"
#include "bltInitCmd.h"

static const char emptyString[] = "";

#define REDRAW_PENDING          (1<<0)  /* Indicates that the widget will
                                         * be redisplayed at the next idle
                                         * point. */
#define FOCUS                   (1<<3)  /* Indicates that the comboframe
                                         * currently has focus. */
#define POSTED                  (1<<5)  /* Indicates the comboframe is
                                         * currently posted. */

#define INSTALL_CHILD           (1<<8)  /* Indicates that the x scrollbar
                                         * is scheduled to be installed at
                                         * the next idle point. */
#define RESTRICT_MIN            (1<<10) /* Indicates to constrain the width
                                         * of the menu to the minimum size
                                         * of the parent widget that posted
                                         * the menu. */
#define RESTRICT_MAX            (1<<11) /* Indicates to constrain the width
                                         * of the menu of the maximum size
                                         * of the parent widget that posted
                                         * the menu. */
#define RESTRICT_NONE           (0)
#define RESTRICT_BOTH           (RESTRICT_MIN|RESTRICT_MAX)
#define INITIALIZED             (1<<22)

#define VAR_FLAGS (TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS)

#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))
#define CLAMP(x,min,max) ((((x) < (min)) ? (min) : ((x) > (max)) ? (max) : (x)))

#define DEF_ANCHOR        "center"
#define DEF_BACKGROUND    RGB_WHITE
#define DEF_BORDERWIDTH   "1"
#define DEF_CLASS         "BltComboFrame"
#define DEF_COMMAND       ((char *)NULL)
#define DEF_CURSOR        ((char *)NULL)
#define DEF_FILL          "both"
#define DEF_HEIGHT        "0"
#define DEF_HIGHLIGHT_BG  STD_NORMAL_BACKGROUND
#define DEF_HIGHLIGHT_COLOR             RGB_BLACK
#define DEF_HIGHLIGHT_WIDTH             "2"
#define DEF_ICON_VARIABLE ((char *)NULL)
#define DEF_PADX          "0"
#define DEF_PADY          "0"
#define DEF_POSTCOMMAND   ((char *)NULL)
#define DEF_RELIEF        "solid"
#define DEF_TAKE_FOCUS    "0"
#define DEF_TEXT_VARIABLE ((char *)NULL)
#define DEF_UNPOSTCOMMAND ((char *)NULL)
#define DEF_WIDTH         "0"
#define DEF_WINDOW        ((char *)NULL)

static Blt_OptionParseProc ObjToRestrictProc;
static Blt_OptionPrintProc RestrictToObjProc;
static Blt_CustomOption restrictOption = {
    ObjToRestrictProc, RestrictToObjProc, NULL, (ClientData)0
};

extern Blt_CustomOption bltLimitsOption;

typedef struct _ComboFrame ComboFrame;

typedef struct {
    unsigned int flags;                 /* Various flags: see below. */
    int x1, y1, x2, y2;                 /* Coordinates of area representing
                                         * the parent that posted this
                                         * menu.  */
    Tk_Window tkwin;                    /* Parent window that posted this
                                         * menu. */
    int menuWidth, menuHeight;
    int lastMenuWidth;
    int align;
} PostInfo;

struct _ComboFrame {

    /* FIXME: check that menuName offset is the same. */
    /*
     * This works around a bug in the Tk API.  Under Win32, Tk tries to
     * read the widget record of toplevel windows (TopLevel or Frame
     * widget), to get its menu name field.  What this means is that we
     * must carefully arrange the fields of this widget so that the
     * menuName field is at the same offset in the structure.
     */

    Tk_Window tkwin;                    /* Window that embodies the frame.
                                         * NULL means that the window has
                                         * been destroyed but the data
                                         * structures haven't yet been
                                         * cleaned up. */
    Display *display;                   /* Display containing widget.
                                         * Used, among other things, so
                                         * that resources can be freed even
                                         * after tkwin has gone away. */
    Tcl_Interp *interp;                 /* Interpreter associated with
                                         * widget.  Used to delete widget
                                         * command. */
    Tcl_Command cmdToken;               /* Token for widget's command. */
    Tcl_Obj *cmdObjPtr;                 /* If non-NULL, command to be
                                         * executed when this menu is
                                         * has been updated. */
    Tcl_Obj *postCmdObjPtr;             /* If non-NULL, command to be
                                         * executed when this menu is
                                         * posted. */
    Tcl_Obj *unpostCmdObjPtr;           /* If non-NULL, command to be
                                         * executed when this menu is
                                         * unposted. */
    unsigned int flags;
    Tcl_Obj *takeFocusObjPtr;           /* Value of -takefocus option; not
                                         * used in the C code, but used by
                                         * keyboard * traversal scripts. */
    const char *menuName;               /* Textual description of menu to
                                         * use for menubar. Malloc-ed, may
                                         * be NULL. */
    const char *className;              /* Class name for widget (from
                                         * configuration option).
                                         * Malloc-ed. */
    Tk_Cursor cursor;                   /* Current cursor for window or
                                         * None. */

    Tcl_Obj *childObjPtr;               /* Path name of child window. */
    Tk_Window child;
    short int width, height;
    int normalWidth, normalHeight;
    Blt_Limits reqWidth, reqHeight;     
    int relief;
    int borderWidth;
    Blt_Bg bg;
    int highlightWidth;                 /* Width in pixels of highlight to
                                         * draw around widget when it has
                                         * the focus.  <= 0 means don't
                                         * draw a highlight. */
    XColor *highlightBgColor;           /* Color for drawing traversal
                                         * highlight area when highlight is
                                         * off. */
    XColor *highlightColor;             /* Color for drawing traversal
                                         * highlight. */
    Tk_Anchor anchor;                   /* Specifies how the child is
                                         * positioned if extra space is
                                         * available in the widget */
    Blt_Pad padX;                       /* Extra padding placed left and
                                         * right of the child. */
    Blt_Pad padY;                       /* Extra padding placed above and
                                         * below the child */
    int fill;                           /* Indicates if the child should
                                         * fill the unused space in the
                                         * widget. */
    GC copyGC;
    PostInfo post;
};

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_ANCHOR, 
        Blt_Offset(ComboFrame, anchor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BACKGROUND, Blt_Offset(ComboFrame, bg)},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth"},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BORDERWIDTH, Blt_Offset(ComboFrame, borderWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-class", "class", "Class", DEF_CLASS, 
        Blt_Offset(ComboFrame, className)},
    {BLT_CONFIG_OBJ, "-command", (char *)NULL, (char *)NULL, DEF_COMMAND, 
        Blt_Offset(ComboFrame, cmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor", DEF_CURSOR, 
        Blt_Offset(ComboFrame, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FILL, "-fill", "fill", "Fill", DEF_FILL, 
        Blt_Offset(ComboFrame, fill), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-height", "height", "Height", DEF_HEIGHT, 
        Blt_Offset(ComboFrame, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT,
        &bltLimitsOption},
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
        "HighlightBackground", DEF_HIGHLIGHT_BG,
        Blt_Offset(ComboFrame, highlightBgColor), 0},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
        DEF_HIGHLIGHT_COLOR, Blt_Offset(ComboFrame, highlightColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
        "HighlightThickness", DEF_HIGHLIGHT_WIDTH, 
        Blt_Offset(ComboFrame, highlightWidth), 0},
    {BLT_CONFIG_PAD, "-padx", "padX", "PadX", DEF_PADX,
        Blt_Offset(ComboFrame, padX), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-pady", "padY", "PadY", DEF_PADY,
        Blt_Offset(ComboFrame, padY), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-postcommand", "postCommand", "PostCommand", 
        DEF_POSTCOMMAND, Blt_Offset(ComboFrame, postCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RELIEF, 
        Blt_Offset(ComboFrame, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-restrictwidth", "restrictWidth", "RestrictWidth", 
        (char *)NULL, Blt_Offset(ComboFrame, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, &restrictOption},
    {BLT_CONFIG_OBJ, "-takefocus", "takeFocus", "TakeFocus",
        DEF_TAKE_FOCUS, Blt_Offset(ComboFrame, takeFocusObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-unpostcommand", "unpostCommand", "UnpostCommand", 
        DEF_UNPOSTCOMMAND, Blt_Offset(ComboFrame, unpostCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-width", "width", "Width", DEF_WIDTH, 
        Blt_Offset(ComboFrame, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT,
        &bltLimitsOption},
    {BLT_CONFIG_OBJ, "-window", "window", "Window", DEF_WINDOW, 
        Blt_Offset(ComboFrame, childObjPtr), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END}
};

static Tk_GeomRequestProc ChildGeometryProc;
static Tk_GeomLostSlaveProc ChildCustodyProc;
static Tk_GeomMgr comboMgrInfo = {
    (char *)"comboframe",               /* Name of geometry manager used by
                                         * winfo. */
    ChildGeometryProc,                  /* Procedure to for new geometry
                                         * requests. */
    ChildCustodyProc,                   /* Procedure when child is taken
                                         * away. */
};



static Blt_SwitchParseProc PostBoxSwitchProc;
static Blt_SwitchCustom postBoxSwitch = {
    PostBoxSwitchProc, NULL, NULL, 0, 
};
static Blt_SwitchParseProc PostAlignSwitchProc;
static Blt_SwitchCustom postAlignSwitch = {
    PostAlignSwitchProc, NULL, NULL, 0, 
};
static Blt_SwitchParseProc PostWindowSwitchProc;
static Blt_SwitchCustom postWindowSwitch = {
    PostWindowSwitchProc, NULL, NULL, 0, 
};

typedef struct {
    unsigned int flags;                 /* Various flags: see below. */
    int x1, y1, x2, y2;                 /* Coordinates of area representing
                                         * the parent that posted this
                                         * menu.  */
    Tk_Window tkwin;                    /* Parent window that posted this
                                         * menu. */
} PostSwitches;

#define ALIGN_LEFT      (0)             /* Menu is aligned to the center of
                                         * the parent. */
#define ALIGN_CENTER    (1)             /* Menu is aligned on the left side
                                         * of the parent.  */
#define ALIGN_RIGHT     (2)             /* Menu is aligned on the right
                                         * side of the parent. */

#define POST_PARENT     (0)             /* Use parent geometry for location
                                         * of button. */
#define POST_WINDOW     (3)             /* Window representing the
                                         * parent. */
#define POST_REGION     (4)             /* Bounding box representing the
                                         * parent area. The x1, y2, x2, y2
                                         * coordinates are in root
                                         * coordinates. */

static Blt_SwitchSpec postSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-align", "left|right|center", (char *)NULL,
        Blt_Offset(ComboFrame, post.align), 0, 0, &postAlignSwitch},
    {BLT_SWITCH_CUSTOM, "-box", "x1 y1 x2 y2", (char *)NULL,
        0, 0, 0, &postBoxSwitch},
    {BLT_SWITCH_CUSTOM, "-window", "path", (char *)NULL,
        Blt_Offset(ComboFrame, post.tkwin), 0, 0, &postWindowSwitch},
    {BLT_SWITCH_END}
};

static Tcl_IdleProc DisplayProc;
static Tcl_FreeProc DestroyProc;
static Tk_EventProc ChildEventProc;
static Tk_EventProc EventProc;
static Tcl_ObjCmdProc InstCmdProc;
static Tcl_CmdDeleteProc InstCmdDeletedProc;

static inline int
GetWidth(ComboFrame *comboPtr)
{
    int w;

    w = comboPtr->width;
    if (w < 2) {
        w = Tk_Width(comboPtr->tkwin);
    }
    if (w < 2) {
        w = Tk_ReqWidth(comboPtr->tkwin);
    }
    return w;
}

static inline int
GetHeight(ComboFrame *comboPtr)
{
    int h;

    h = comboPtr->height;
    if (h < 2) {
        h = Tk_Height(comboPtr->tkwin);
    }
    if (h < 2) {
        h = Tk_ReqHeight(comboPtr->tkwin);
    }
    return h;
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *      Tells the Tk dispatcher to call the comboframe display routine at
 *      the next idle point.  This request is made only if the window is
 *      displayed and no other redraw request is pending.
 *
 * Results: None.
 *
 * Side effects:
 *      The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyRedraw(ComboFrame *comboPtr) 
{
    if ((comboPtr->tkwin != NULL) && !(comboPtr->flags & REDRAW_PENDING)) {
        Tcl_DoWhenIdle(DisplayProc, comboPtr);
        comboPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBoundedWidth --
 *
 *      Bounds a given width value to the limits described in the limit
 *      structure.  The initial starting value may be overridden by the
 *      nominal value in the limits.
 *
 * Results:
 *      Returns the constrained value.
 *
 *---------------------------------------------------------------------------
 */
static int
GetBoundedWidth(ComboFrame *comboPtr, int w)     
{
    /*
     * Check widgets for requested width values;
     */
    if (comboPtr->reqWidth.flags & LIMITS_NOM_SET) {
        w = comboPtr->reqWidth.nom;     /* Override initial value */
    }
    if (w < comboPtr->reqWidth.min) {
        w = comboPtr->reqWidth.min;     /* Bounded by minimum value */
    }
    if (w > comboPtr->reqWidth.max) {
        w = comboPtr->reqWidth.max;     /* Bounded by maximum value */
    }
    if (comboPtr->flags & (RESTRICT_MIN|RESTRICT_MAX)) {
        if ((comboPtr->flags & RESTRICT_MIN) &&
            (w < comboPtr->post.menuWidth)) {
            w = comboPtr->post.menuWidth;
        }
        if ((comboPtr->flags & RESTRICT_MAX) &&
            (w > comboPtr->post.menuWidth)) {
            w = comboPtr->post.menuWidth;
        }
    }
    {
        int screenWidth, screenHeight;

        Blt_SizeOfScreen(comboPtr->tkwin, &screenWidth, &screenHeight);
        if (w > screenWidth) {
            w = screenWidth;
        }
    }
    return w;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBoundedHeight --
 *
 *      Bounds a given value to the limits described in the limit
 *      structure.  The initial starting value may be overridden by the
 *      nominal value in the limits.
 *
 * Results:
 *      Returns the constrained value.
 *
 *---------------------------------------------------------------------------
 */
static int
GetBoundedHeight(ComboFrame *comboPtr, int h)    
{
    /*
     * Check widgets for requested height values;
     */
    if (comboPtr->reqHeight.flags & LIMITS_NOM_SET) {
        h = comboPtr->reqHeight.nom;    /* Override initial value */
    }
    if (h < comboPtr->reqHeight.min) {
        h = comboPtr->reqHeight.min;    /* Bounded by minimum value */
    }
    if (h > comboPtr->reqHeight.max) {
        h = comboPtr->reqHeight.max;    /* Bounded by maximum value */
    }
    if (h > HeightOfScreen(Tk_Screen(comboPtr->tkwin))) {
        h = HeightOfScreen(Tk_Screen(comboPtr->tkwin));
    }
    return h;
}

static void
FixMenuCoords(ComboFrame *comboPtr, int *xPtr, int *yPtr)
{
    int x, y, w, h;
    int sw, sh;

    Blt_SizeOfScreen(comboPtr->tkwin, &sw, &sh);
    x = *xPtr;
    y = *yPtr;
    w = GetWidth(comboPtr);
    h = GetHeight(comboPtr);

    if ((y + h) > sh) {
        y -= h;                         /* Shift the menu up by the height
                                         * of the menu. */
        y -= comboPtr->post.menuHeight; /* Add the height of the parent.  */
        if (y < 0) {
            y = 0;
        }
    }
    if ((x + w) > sw) {
        x = x + comboPtr->post.menuWidth - w;
                                        /* Flip the menu anchor to the
                                         * other end of the menu
                                         * button/entry */
        if (x < 0) {
            x = 0;
        }
    }
    *xPtr = x;
    *yPtr = y;
}

static int
WithdrawMenu(ComboFrame *comboPtr)
{
    if (!Tk_IsMapped(comboPtr->child)) {
        return FALSE;                /* This frame is already withdrawn. */
    }
    if (Tk_IsMapped(comboPtr->tkwin)) {
        Tk_UnmapWindow(comboPtr->tkwin);
    }
    return TRUE;
}

static void
ComputeGeometry(ComboFrame *comboPtr)
{
    int w, h;

    w = Tk_ReqWidth(comboPtr->child);
    h = Tk_ReqHeight(comboPtr->child);
    w += 2 * (comboPtr->highlightWidth + comboPtr->borderWidth) + 
        PADDING(comboPtr->padX);
    h += 2 * (comboPtr->highlightWidth + comboPtr->borderWidth) +
        PADDING(comboPtr->padY);
    comboPtr->normalWidth = w;
    comboPtr->normalHeight = h;

    w = GetBoundedWidth(comboPtr, w);
    h = GetBoundedHeight(comboPtr, h);

    if (w < comboPtr->post.menuWidth) {
        w = comboPtr->post.menuWidth;
    }
    comboPtr->width = w;
    comboPtr->height = h;
    if ((w != Tk_ReqWidth(comboPtr->tkwin)) ||
        (h != Tk_ReqHeight(comboPtr->tkwin))) {
        Tk_GeometryRequest(comboPtr->tkwin, w, h);
    }
}


static void
UnmanageChild(ComboFrame *comboPtr)
{
    if (comboPtr->child != NULL) {
        Tk_DeleteEventHandler(comboPtr->child, StructureNotifyMask,
              ChildEventProc, comboPtr);
        Tk_ManageGeometry(comboPtr->child, (Tk_GeomMgr *)NULL, comboPtr);
        if (Tk_IsMapped(comboPtr->child)) {
            Tk_UnmapWindow(comboPtr->child);
        }
    }
}

static void
ManageChild(ComboFrame *comboPtr, Tk_Window child)
{
    if (child != NULL) {
        Tk_CreateEventHandler(child, StructureNotifyMask, ChildEventProc, 
                comboPtr);
        Tk_ManageGeometry(child, &comboMgrInfo, comboPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * InstallChild --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
InstallChild(ClientData clientData) 
{
    ComboFrame *comboPtr = clientData;
    Tk_Window tkwin;
    const char *string;
    int length;

    if (comboPtr->childObjPtr == NULL) {
        comboPtr->child = NULL;
        return;
    }
    string = Tcl_GetStringFromObj(comboPtr->childObjPtr, &length);
    if (length == 0) {
        comboPtr->child = NULL;
        return;
    }
    tkwin = Tk_NameToWindow(comboPtr->interp, string, comboPtr->tkwin);
    if (tkwin == NULL) {
        Tcl_BackgroundError(comboPtr->interp);
        return;
    }
    if (Tk_Parent(tkwin) != comboPtr->tkwin) {
        Tcl_AppendResult(comboPtr->interp, "widget \"", Tk_PathName(tkwin), 
                         "\" must be a child of comboframe.", (char *)NULL);
        Tcl_BackgroundError(comboPtr->interp);
        return;
    }
    ManageChild(comboPtr, tkwin);
    comboPtr->child = tkwin;
}


static int
ConfigureProc(Tcl_Interp *interp, ComboFrame *comboPtr, int objc,
              Tcl_Obj *const *objv, int flags)
{
    XGCValues gcValues;
    unsigned long gcMask;
    GC newGC;

    if (Blt_ConfigureWidgetFromObj(interp, comboPtr->tkwin, configSpecs, 
        objc, objv, (char *)comboPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    gcMask = 0;
    newGC = Tk_GetGC(comboPtr->tkwin, gcMask, &gcValues);
    if (comboPtr->copyGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->copyGC);
    }
    comboPtr->copyGC = newGC;

    /* Install the embedded child widget as needed.  We defer installing
     * the child so the widget doesn't have to exist when it is specified
     * by -window option. The down-side is that errors found in the widget
     * name will be backgrounded. */
    if (Blt_ConfigModified(configSpecs, "-window", (char *)NULL)) {
        if (comboPtr->child != NULL) {
            UnmanageChild(comboPtr);
            comboPtr->child = NULL;
        }
        if ((comboPtr->flags & INSTALL_CHILD) == 0) {
            Tcl_DoWhenIdle(InstallChild, comboPtr);
            comboPtr->flags |= INSTALL_CHILD;
        }           
    }
    return TCL_OK;
}

/* Widget Callbacks */

/*
 *---------------------------------------------------------------------------
 *
 * EventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various events
 *      on comboframe widgets.
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
EventProc(ClientData clientData, XEvent *eventPtr)
{
    ComboFrame *comboPtr = clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedraw(comboPtr);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        EventuallyRedraw(comboPtr);
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
        if (eventPtr->xfocus.detail == NotifyInferior) {
            return;
        }
        if (eventPtr->type == FocusIn) {
            comboPtr->flags |= FOCUS;
        } else {
            comboPtr->flags &= ~FOCUS;
        }
        EventuallyRedraw(comboPtr);
    } else if (eventPtr->type == DestroyNotify) {
        if (comboPtr->tkwin != NULL) {
            comboPtr->tkwin = NULL; 
        }
        if (comboPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayProc, comboPtr);
        }
        Tcl_EventuallyFree(comboPtr, DestroyProc);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildEventProc --
 *
 *      This procedure is invoked by the Tk event handler when
 *      StructureNotify events occur in a child managed by the widget.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
ChildEventProc(ClientData clientData, XEvent *eventPtr)
{
    ComboFrame *comboPtr = clientData;

    if (eventPtr->type == ConfigureNotify) {
        EventuallyRedraw(comboPtr);
    } else if (eventPtr->type == DestroyNotify) {
        if ((comboPtr->child != NULL) &&
            (eventPtr->xany.window == Tk_WindowId(comboPtr->child))) {
            comboPtr->child = NULL;
        } 
        EventuallyRedraw(comboPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildCustodyProc --
 *
 *      This procedure is invoked when a child has been stolen by
 *      another geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *     Arranges for the comboframe to have its layout re-arranged at the
 *     next idle point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ChildCustodyProc(ClientData clientData, Tk_Window tkwin)
{
    ComboFrame *comboPtr = (ComboFrame *)clientData;

    if (tkwin == comboPtr->child) {
        comboPtr->child = NULL;
    }
    Tk_UnmaintainGeometry(tkwin, comboPtr->tkwin);
    EventuallyRedraw(comboPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildGeometryProc --
 *
 *      This procedure is invoked by Tk_GeometryRequest for widgets
 *      managed by the comboframe.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the comboframe to have its layout re-computed and
 *      re-arranged at the next idle point.
 *
 * -------------------------------------------------------------------------- 
 */
/* ARGSUSED */
static void
ChildGeometryProc(ClientData clientData, Tk_Window tkwin)
{
    ComboFrame *comboPtr = (ComboFrame *)clientData;

    EventuallyRedraw(comboPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToRestrictProc --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToRestrictProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                  Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    char *string;
    int flag;

    string = Tcl_GetString(objPtr);
    if (strcmp(string, "min") == 0) {
        flag = RESTRICT_MIN;
    } else if (strcmp(string, "max") == 0) {
        flag = RESTRICT_MAX;
    } else if (strcmp(string, "both") == 0) {
        flag = RESTRICT_BOTH;
    } else if (strcmp(string, "none") == 0) {
        flag = 0;
    } else {
        Tcl_AppendResult(interp, "unknown state \"", string, 
                "\": should be active, disabled, or normal.", (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~RESTRICT_BOTH;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RestrictToObjProc --
 *
 *      Return the string representation of the restrict flags.
 *
 * Results:
 *      The name representing the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
RestrictToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                  char *widgRec, int offset, int flags)  
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);

    switch (*flagsPtr & RESTRICT_BOTH) {
    case RESTRICT_MIN:
        return Tcl_NewStringObj("min", -1);     
    case RESTRICT_MAX:
        return Tcl_NewStringObj("max", -1);
    case RESTRICT_BOTH:
        return Tcl_NewStringObj("both", -1);
    case RESTRICT_NONE:
        return Tcl_NewStringObj("none", -1);
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetCoordsFromObj --
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
GetCoordsFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *xPtr, int *yPtr)
{
    int objc;
    Tcl_Obj **objv;
    int x, y;
    
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc != 2) {
        Tcl_AppendResult(interp, "wrong # of arguments: should be \"x y\"",
                (char *)NULL);
        return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[0], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[1], &y) != TCL_OK)) {
        return TCL_ERROR;
    }
    *xPtr = x;
    *yPtr = y;
    return TCL_OK;
}

    
/*
 *---------------------------------------------------------------------------
 *
 * GetAlignFromObj --
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
GetAlignFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *alignPtr)
{
    char c;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'l') && (strncmp(string, "left", length) == 0)) {
        *alignPtr = ALIGN_LEFT;
    } else if ((c == 'r') && (strncmp(string, "right", length) == 0)) {
        *alignPtr = ALIGN_RIGHT;
    } else if ((c == 'c') && (strncmp(string, "center", length) == 0)) {
        *alignPtr = ALIGN_CENTER;
    } else {
        Tcl_AppendResult(interp, "bad alignment value \"", string, 
                "\": should be left, right, or center.", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBoxFromObj --
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
GetBoxFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Box2d *boxPtr)
{
    int objc;
    Tcl_Obj **objv;
    int x1, y1, x2, y2;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc != 4) {
        Tcl_AppendResult(interp,
                "wrong # of arguments: should be \"x1 y1 x2 y2\"",
                (char *)NULL);
        return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[0], &x1) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[1], &y1) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[2], &x2) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[3], &y2) != TCL_OK)) {
        return TCL_ERROR;
    }
    boxPtr->x1 = MIN(x1, x2);
    boxPtr->y1 = MIN(y1, y2);
    boxPtr->x2 = MAX(x2, x1);
    boxPtr->y2 = MAX(y2, y1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PostWindowSwitchProc --
 *
 *      Converts a window name into Tk window.
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
PostWindowSwitchProc(ClientData clientData, Tcl_Interp *interp,
                    const char *switchName, Tcl_Obj *objPtr, char *record,
                    int offset, int flags)
{
    ComboFrame *comboPtr = (ComboFrame *)record;
    Tk_Window tkwin;
    const char *string;
    int length;

    tkwin = NULL;
    string = Tcl_GetStringFromObj(objPtr, &length);
    if (length == 0) {
        tkwin = NULL;
    } else {
        tkwin = Tk_NameToWindow(interp, string, comboPtr->tkwin);
        if (tkwin == NULL) {
            return TCL_ERROR;
        }
    }
    comboPtr->post.flags = POST_WINDOW;
    comboPtr->post.tkwin = tkwin;
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * PostAlignSwitchProc --
 *
 *      Converts string into x and y coordinates.
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
PostAlignSwitchProc(ClientData clientData, Tcl_Interp *interp,
                    const char *switchName, Tcl_Obj *objPtr, char *record,
                    int offset, int flags)
{
    ComboFrame *comboPtr = (ComboFrame *)record;
    int align;
    
    if (GetAlignFromObj(interp, objPtr, &align) != TCL_OK) {
        return TCL_ERROR;
    }
    comboPtr->post.align = align;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PostBoxSwitchProc --
 *
 *      Converts string into x1, y1, x2, and y2 coordinates.
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
PostBoxSwitchProc(ClientData clientData, Tcl_Interp *interp,
                  const char *switchName, Tcl_Obj *objPtr, char *record,
                  int offset, int flags)
{
    ComboFrame *comboPtr = (ComboFrame *)record;
    Box2d box;
    
    if (GetBoxFromObj(interp, objPtr, &box) != TCL_OK) {
        return TCL_ERROR;
    }
    comboPtr->post.x1 = box.x1;
    comboPtr->post.y1 = box.y1;
    comboPtr->post.x2 = box.x2;
    comboPtr->post.y2 = box.y2;
    comboPtr->post.flags = POST_REGION;
    return TCL_OK;
}

/* Widget Operations */

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      pathName configure ?option value ... ?
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    ComboFrame *comboPtr = clientData;
    int result;

    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, 
                configSpecs, (char *)comboPtr, (Tcl_Obj *)NULL,  0);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, comboPtr->tkwin, 
                configSpecs, (char *)comboPtr, objv[2], 0);
    }
    Tcl_Preserve(comboPtr);
    result = ConfigureProc(interp, comboPtr, objc - 2, objv + 2, 
                BLT_CONFIG_OBJV_ONLY);
    Tcl_Release(comboPtr);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    EventuallyRedraw(comboPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      pathName cget option
 *
 *---------------------------------------------------------------------------
 */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    ComboFrame *comboPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, comboPtr->tkwin, configSpecs,
        (char *)comboPtr, objv[2], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * OverButtonOp --
 *
 *      Returns whether the x, y coordinate is contained within the
 *      region that represents the button that posted the menu.
 *
 * Results:
 *      Standard TCL result.  A boolean is returned in the interpreter.
 *
 *      pathName overbutton x y 
 *
 *---------------------------------------------------------------------------
 */
static int
OverButtonOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    ComboFrame *comboPtr = clientData;
    int x, y;
    int state;
    
    if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
        return TCL_ERROR;
    }
    state = FALSE;
    if ((x >= comboPtr->post.x1) && (x < comboPtr->post.x2) &&
        (y >= comboPtr->post.y1) && (y < comboPtr->post.y2)) {
        state = TRUE;
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PostOp --
 *
 *      Posts this menu at the given root window coordinates.
 *
 *      pathName post ?switches ...?
 *              -align left|right|center
 *              -box coordList
 *              -window path
 *
 *---------------------------------------------------------------------------
 */
static int
PostOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    ComboFrame *comboPtr = clientData;
    int x, y;

    memset(&comboPtr->post, 0, sizeof(PostInfo));
    comboPtr->post.tkwin = Tk_Parent(comboPtr->tkwin);
    comboPtr->post.menuWidth = comboPtr->normalWidth;
    /* Process switches  */
    if (Blt_ParseSwitches(interp, postSwitches, objc - 2, objv + 2, comboPtr,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    switch (comboPtr->post.flags) {
    case POST_PARENT:
    case POST_WINDOW:
        {
            Tk_Window tkwin, parent;
            int x, y, w, h;
            int rootX, rootY;
            
            tkwin = comboPtr->post.tkwin;
            parent = Tk_Parent(tkwin);
            w = Tk_Width(tkwin);
            h = Tk_Height(tkwin);
            
            x = Tk_X(tkwin);
            y = Tk_Y(tkwin);
            Tk_GetRootCoords(parent, &rootX, &rootY);
            x += rootX;
            y += rootY;
            comboPtr->post.x1 = x;
            comboPtr->post.y1 = y;
            comboPtr->post.x2 = x + w;
            comboPtr->post.y2 = y + h;
        }
        break;
    case POST_REGION:
        break;
    }
    comboPtr->post.menuWidth = comboPtr->post.x2 - comboPtr->post.x1;
    comboPtr->post.menuHeight = comboPtr->post.y2 - comboPtr->post.y1;
    if ((comboPtr->post.menuWidth != comboPtr->post.lastMenuWidth)) {
        ComputeGeometry(comboPtr);
    }
    comboPtr->post.lastMenuWidth = comboPtr->post.menuWidth;
    x = 0;                              /* Suppress compiler warning; */
    y = comboPtr->post.y2;
    switch (comboPtr->post.align) {
    case ALIGN_LEFT:
        x = comboPtr->post.x1;
        break;
    case ALIGN_CENTER:
        {
            int w;

            w = comboPtr->post.x2 - comboPtr->post.x1;
            x = comboPtr->post.x1 + (w - comboPtr->normalWidth) / 2; 
        }
        break;
    case ALIGN_RIGHT:
        if (comboPtr->post.menuWidth > comboPtr->normalWidth) {
            x = comboPtr->post.x2 - comboPtr->post.menuWidth;
        } else {
            x = comboPtr->post.x2 - comboPtr->normalWidth;
        }
        break;
    }
    FixMenuCoords(comboPtr, &x, &y);
    /*
     * If there is a post command for the menu, execute it.  This may
     * change the size of the menu, so be sure to recompute the menu's
     * geometry if needed.
     */
    if (comboPtr->postCmdObjPtr != NULL) {
        int result;

        Tcl_IncrRefCount(comboPtr->postCmdObjPtr);
        result = Tcl_EvalObjEx(interp, comboPtr->postCmdObjPtr, 
                TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(comboPtr->postCmdObjPtr);
        if (result != TCL_OK) {
            return result;
        }
        /*
         * The post commands could have deleted the menu, which means we
         * are dead and should go away.
         */
        if (comboPtr->tkwin == NULL) {
            return TCL_OK;
        }
        ComputeGeometry(comboPtr);
    }

    /*
     * Adjust the position of the menu if necessary to keep it visible on the
     * screen.  There are two special tricks to make this work right:
     *
     * 1. If a virtual root window manager is being used then
     *    the coordinates are in the virtual root window of
     *    menuPtr's parent;  since the menu uses override-redirect
     *    mode it will be in the *real* root window for the screen,
     *    so we have to map the coordinates from the virtual root
     *    (if any) to the real root.  Can't get the virtual root
     *    from the menu itself (it will never be seen by the wm)
     *    so use its parent instead (it would be better to have an
     *    an option that names a window to use for this...).
     * 2. The menu may not have been mapped yet, so its current size
     *    might be the default 1x1.  To compute how much space it
     *    needs, use its requested size, not its actual size.
     *
     * Note that this code assumes square screen regions and all positive
     * coordinates. This does not work on a Mac with multiple monitors. But
     * then again, Tk has other problems with this.
     */
    {
        int rootX, rootY, rootWidth, rootHeight;
        int sw, sh;
        Tk_Window parent;

        parent = Tk_Parent(comboPtr->tkwin);
        Blt_SizeOfScreen(comboPtr->tkwin, &sw, &sh);
        Tk_GetVRootGeometry(parent, &rootX, &rootY, &rootWidth, &rootHeight);
        x += rootX;
        y += rootY;
        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }
        if ((x + comboPtr->width) > sw) {
            x = sw - comboPtr->width;
        }
        if ((y + comboPtr->height) > sh) {
            y = sh - comboPtr->height;
        }
        Tk_MoveToplevelWindow(comboPtr->tkwin, x, y);
        Tk_MapWindow(comboPtr->tkwin);
        Blt_MapToplevelWindow(comboPtr->tkwin);
        Blt_RaiseToplevelWindow(comboPtr->tkwin);
#ifdef notdef
        TkWmRestackToplevel(comboPtr->tkwin, Above, NULL);
#endif
    }
    comboPtr->flags |= POSTED;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * UnpostOp --
 *
 *      Unposts this menu.
 *
 *      pathName post 
 *
 *---------------------------------------------------------------------------
 */
static int
UnpostOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    ComboFrame *comboPtr = clientData;

    if (!WithdrawMenu(comboPtr)) {
        fprintf(stderr, "menu is already unposted\n");
        return TCL_OK;          /* This menu is already unposted. */
    }
    /*
     * If there is a unpost command for the menu, execute it.  
     */
    if (comboPtr->unpostCmdObjPtr != NULL) {
        int result;

        Tcl_IncrRefCount(comboPtr->unpostCmdObjPtr);
        result = Tcl_EvalObjEx(interp, comboPtr->unpostCmdObjPtr, 
                TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(comboPtr->unpostCmdObjPtr);
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }
    comboPtr->flags &= ~POSTED;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * WithdrawOp --
 *
 *      Hides the menu but doesn't call the unpost command. Technically
 *      the menu is still posted.
 *
 *      pathName withdraw 
 *
 *---------------------------------------------------------------------------
 */
static int
WithdrawOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    ComboFrame *comboPtr = clientData;

    WithdrawMenu(comboPtr);
    return TCL_OK;      
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyProc --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the internal structure of the widget at a safe time (when
 *      no-one is using it anymore).
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Everything associated with the widget is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyProc(DestroyData dataPtr)        /* Pointer to the widget record. */
{
    ComboFrame *comboPtr = (ComboFrame *)dataPtr;

    if (comboPtr->flags & REDRAW_PENDING) {
        Tcl_CancelIdleCall(DisplayProc, comboPtr);
    }
    if (comboPtr->flags & INSTALL_CHILD) {
        Tcl_CancelIdleCall(InstallChild, comboPtr);
    }       
    if (comboPtr->copyGC != NULL) {
        Tk_FreeGC(comboPtr->display, comboPtr->copyGC);
    }
    Blt_FreeOptions(configSpecs, (char *)comboPtr, comboPtr->display, 0);
    Tcl_DeleteCommandFromToken(comboPtr->interp, comboPtr->cmdToken);
    Blt_Free(comboPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewComboFrame --
 *
 *---------------------------------------------------------------------------
 */
static ComboFrame *
NewComboFrame(Tcl_Interp *interp, Tk_Window tkwin)
{
    ComboFrame *comboPtr;

    comboPtr = Blt_AssertCalloc(1, sizeof(ComboFrame));
    comboPtr->borderWidth = 1;
    comboPtr->display = Tk_Display(tkwin);
    comboPtr->interp = interp;
    comboPtr->relief = TK_RELIEF_SOLID;
    comboPtr->tkwin = tkwin;
    Blt_ResetLimits(&comboPtr->reqWidth);
    Blt_ResetLimits(&comboPtr->reqHeight);
    Blt_SetWindowInstanceData(tkwin, comboPtr);
    return comboPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboFrameCmd --
 *
 *      This procedure is invoked to process the "comboframe" command.  See
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
static Blt_OpSpec frameOps[] =
{
    {"cget",        2, CgetOp,        3, 3, "option",},
    {"configure",   2, ConfigureOp,   2, 0, "?option value ...?",},
    {"overbutton",  1, OverButtonOp,  4, 4, "x y",},
    {"post",        4, PostOp,        2, 0, "switches ...",},
    {"unpost",      1, UnpostOp,      2, 2, "",},
    {"withdraw",    1, WithdrawOp,    2, 2, "",},
};

static int numFrameOps = sizeof(frameOps) / sizeof(Blt_OpSpec);

static int
InstCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    ComboFrame *comboPtr = clientData;
    int result;

    proc = Blt_GetOpFromObj(interp, numFrameOps, frameOps, BLT_OP_ARG1,
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    Tcl_Preserve(comboPtr);
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(comboPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * InstCmdDeletedProc --
 *
 *      This procedure can be called if the window was destroyed (tkwin
 *      will be NULL) and the command was deleted automatically.  In this
 *      case, we need to do nothing.
 *
 *      Otherwise this routine was called because the command was deleted.
 *      Then we need to clean-up and destroy the widget.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The widget is destroyed.
 *
 *---------------------------------------------------------------------------
 */
static void
InstCmdDeletedProc(ClientData clientData)
{
    ComboFrame *comboPtr = clientData;   /* Pointer to widget record. */

    if (comboPtr->tkwin != NULL) {
        Tk_Window tkwin;

        tkwin = comboPtr->tkwin;
        comboPtr->tkwin = NULL;
        Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComboFrameCmd --
 *
 *      This procedure is invoked to process the TCL command that corresponds
 *      to a widget managed by this module. See the user documentation for
 *      details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ComboFrameCmd(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    ComboFrame *comboPtr;
    Tk_Window tkwin;
    XSetWindowAttributes attrs;
    char *path;
    unsigned int mask;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " pathName ?option value ...?\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * First time in this interpreter, invoke a procedure to initialize
     * various bindings on the comboframe widget.  If the procedure doesn't
     * already exist, source it from "$blt_library/bltComboFrame.tcl".  We
     * deferred sourcing the file until now so that the variable
     * $blt_library could be set within a script.
     */
    if (!Blt_CommandExists(interp, "::blt::ComboFrame::PostCascade")) {
        if (Tcl_GlobalEval(interp, 
                "source [file join $blt_library bltComboFrame.tcl]") != TCL_OK) {
            char info[200];

            Blt_FormatString(info, 200, "\n    (while loading bindings for %.50s)", 
                    Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            return TCL_ERROR;
        }
    }
    path = Tcl_GetString(objv[1]);
#define TOP_LEVEL_SCREEN ""
    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), path, 
        TOP_LEVEL_SCREEN);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    Tk_SetClass(tkwin, "BltComboFrame");
    comboPtr = NewComboFrame(interp, tkwin);
    if (ConfigureProc(interp, comboPtr, objc - 2, objv + 2, 0) != TCL_OK) {
        Tk_DestroyWindow(comboPtr->tkwin);
        return TCL_ERROR;
    }
    mask = (ExposureMask | StructureNotifyMask | FocusChangeMask);
    Tk_CreateEventHandler(tkwin, mask, EventProc, comboPtr);
    comboPtr->cmdToken = Tcl_CreateObjCommand(interp, path, InstCmdProc, 
        comboPtr, InstCmdDeletedProc);

    attrs.override_redirect = True;
    attrs.backing_store = WhenMapped;
    attrs.save_under = True;
    mask = (CWOverrideRedirect | CWSaveUnder | CWBackingStore);
    Tk_ChangeWindowAttributes(tkwin, mask, &attrs);

    Tk_MakeWindowExist(tkwin);
    Tcl_SetObjResult(interp, objv[1]);
    return TCL_OK;
}
    
/*
 *---------------------------------------------------------------------------
 *
 * TranslateAnchor --
 *
 *      Translate the coordinates of a given bounding box based upon the
 *      anchor specified.  The anchor indicates where the given xy position is
 *      in relation to the bounding box.
 *
 *              nw --- n --- ne
 *              |            |     x,y ---+
 *              w   center   e      |     |
 *              |            |      +-----+
 *              sw --- s --- se
 *
 * Results:
 *      The translated coordinates of the bounding box are returned.
 *
 *---------------------------------------------------------------------------
 */
static void
TranslateAnchor(
    int dx, int dy,             /* Difference between outer and inner
                                 * regions */
    Tk_Anchor anchor,           /* Direction of the anchor */
    int *xPtr, int *yPtr)
{
    int x, y;

    x = y = 0;
    switch (anchor) {
    case TK_ANCHOR_NW:          /* Upper left corner */
        break;
    case TK_ANCHOR_W:           /* Left center */
        y = (dy / 2);
        break;
    case TK_ANCHOR_SW:          /* Lower left corner */
        y = dy;
        break;
    case TK_ANCHOR_N:           /* Top center */
        x = (dx / 2);
        break;
    case TK_ANCHOR_CENTER:      /* Centered */
        x = (dx / 2);
        y = (dy / 2);
        break;
    case TK_ANCHOR_S:           /* Bottom center */
        x = (dx / 2);
        y = dy;
        break;
    case TK_ANCHOR_NE:          /* Upper right corner */
        x = dx;
        break;
    case TK_ANCHOR_E:           /* Right center */
        x = dx;
        y = (dy / 2);
        break;
    case TK_ANCHOR_SE:          /* Lower right corner */
        x = dx;
        y = dy;
        break;
    }
    *xPtr = (*xPtr) + x;
    *yPtr = (*yPtr) + y;
}

static void
ArrangeChild(ComboFrame *comboPtr)
{
    int dx, dy;
    int cavityWidth, cavityHeight;
    int winWidth, winHeight;
    int x, y;
    int inset;

    x = comboPtr->padX.side1 + Tk_Changes(comboPtr->child)->border_width;
    y = comboPtr->padY.side1 + Tk_Changes(comboPtr->child)->border_width;

    if ((x >= Tk_Width(comboPtr->tkwin)) || (y >= Tk_Height(comboPtr->tkwin))) {
        if (Tk_IsMapped(comboPtr->child)) {
            Tk_UnmapWindow(comboPtr->child);
        }
        return;
    }
    inset = comboPtr->borderWidth + comboPtr->highlightWidth;
    cavityWidth = Tk_Width(comboPtr->tkwin) - PADDING(comboPtr->padX) - 
        2 * inset;
    cavityHeight = Tk_Height(comboPtr->tkwin) - PADDING(comboPtr->padY) -
        2 * inset;

    winWidth = Tk_ReqWidth(comboPtr->child);
    winHeight = Tk_ReqHeight(comboPtr->child);

    /*
     *
     * Compare the child's requested size to the size of the widget.
     *
     * 1) If the child is larger than the cavity or if the fill flag is
     *    set, make the child the size of the cavity. Check that the new size
     *    is within the bounds set for the widget.
     *
     * 2) Otherwise, position the child in the space according to its
     *    anchor.
     *
     */
    if ((cavityWidth <= winWidth) || (comboPtr->fill & FILL_X)) {
        winWidth = cavityWidth;
        if (winWidth > comboPtr->reqWidth.max) {
            winWidth = comboPtr->reqWidth.max;
        }
    }
    if ((cavityHeight <= winHeight) || (comboPtr->fill & FILL_Y)) {
        winHeight = cavityHeight;
        if (winHeight > comboPtr->reqHeight.max) {
            winHeight = comboPtr->reqHeight.max;
        }
    }
    dx = dy = 0;
    if (cavityWidth > winWidth) {
        dx = (cavityWidth - winWidth);
    }
    if (cavityHeight > winHeight) {
        dy = (cavityHeight - winHeight);
    }
    if ((dx > 0) || (dy > 0)) {
        TranslateAnchor(dx, dy, comboPtr->anchor, &x, &y);
    }
    /*
     * Clip the widget at the bottom and/or right edge of the container.
     */
    if (winWidth > (Tk_Width(comboPtr->tkwin) - x)) {
        winWidth = (Tk_Width(comboPtr->tkwin) - x);
    }
    if (winHeight > (Tk_Height(comboPtr->tkwin) - y)) {
        winHeight = (Tk_Height(comboPtr->tkwin) - y);
    }

    /*
     * If the widget is too small (i.e. it has only an external border)
     * then unmap it.
     */
    if ((winWidth < 1) || (winHeight < 1)) {
        if (Tk_IsMapped(comboPtr->child)) {
            Tk_UnmapWindow(comboPtr->tkwin);
        }
        return;
    }

    /*
     * Resize and/or move the widget as necessary.
     */
#ifdef notdef
    fprintf(stderr, "ArrangeChild: %s rw=%d rh=%d w=%d h=%d\n",
                Tk_PathName(comboPtr->child), Tk_ReqWidth(comboPtr->child),
                Tk_ReqHeight(comboPtr->child), winWidth, winHeight);
#endif
    x += inset;
    y += inset;
    if ((x != Tk_X(comboPtr->child)) || (y != Tk_Y(comboPtr->child)) ||
        (winWidth != Tk_Width(comboPtr->child)) ||
        (winHeight != Tk_Height(comboPtr->child))) {
#ifdef notdef
        fprintf(stderr, "MoveResize: %s x=%d y=%d w=%d h=%d\n",
                Tk_PathName(comboPtr->child), x, y, winWidth, winHeight);
#endif
        Tk_MoveResizeWindow(comboPtr->child, x, y, winWidth, winHeight);
    }
    if (!Tk_IsMapped(comboPtr->child)) {
        Tk_MapWindow(comboPtr->child);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayProc --
 *
 *      This procedure is invoked to display a comboframe widget.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Commands are output to X to display the menu.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayProc(ClientData clientData)
{
    ComboFrame *comboPtr = clientData;
    Pixmap drawable;
    int w, h;                           /* Window width and height. */
    int screenWidth, screenHeight;

    comboPtr->flags &= ~REDRAW_PENDING;
    if (comboPtr->tkwin == NULL) {
        return;                         /* Window destroyed (should not get
                                         * here) */
    }
#ifdef notdef
    fprintf(stderr, "Calling DisplayProc(%s) w=%d h=%d\n", 
            Tk_PathName(comboPtr->tkwin), Tk_Width(comboPtr->tkwin),
            Tk_Height(comboPtr->tkwin));
#endif
    w = Tk_Width(comboPtr->tkwin);
    h = Tk_Height(comboPtr->tkwin);
    if ((w <= 1) || (w <= 1)){
        /* Don't bother computing the layout until the window size is
         * something reasonable. */
        return;
    }
    if (!Tk_IsMapped(comboPtr->tkwin)) {
        /* The menu's window isn't displayed, so don't bother drawing
         * anything.  By getting this far, we've at least computed the
         * coordinates of the comboframe's new layout.  */
        return;
    }
    ComputeGeometry(comboPtr);
    /*
     * Create a pixmap the size of the window for double buffering.
     */
    Blt_SizeOfScreen(comboPtr->tkwin, &screenWidth, &screenHeight);
    w = CLAMP(w, 1, screenWidth);
    h = CLAMP(h, 1, screenHeight);
    drawable = Blt_GetPixmap(comboPtr->display, Tk_WindowId(comboPtr->tkwin),
        w, h, Tk_Depth(comboPtr->tkwin));
#ifdef WIN32
    assert(drawable != None);
#endif  /* WIN32 */
    /* 
     * Shadowed menu.  Request window size slightly bigger than menu.  Get
     * snapshot of background from root menu.
     */

    /* Fill the entire background, even the portion under the child.  This
     * will cover the corner if both scrollbars are displayed. */
    Blt_Bg_FillRectangle(comboPtr->tkwin, drawable, comboPtr->bg,
         0, 0, w, h, 0, TK_RELIEF_FLAT);

    /* Draw 3D border just inside of the focus highlight ring. */
    if ((w > 0) && (h > 0) && (comboPtr->borderWidth > 0) &&
        (comboPtr->relief != TK_RELIEF_FLAT)) {
        Blt_Bg_DrawRectangle(comboPtr->tkwin, drawable, comboPtr->bg, 
                comboPtr->highlightWidth, comboPtr->highlightWidth, 
                w - 2 * comboPtr->highlightWidth, 
                h - 2 * comboPtr->highlightWidth, 
                comboPtr->borderWidth, comboPtr->relief);
    }
    /* Draw focus highlight ring. */
    if ((comboPtr->highlightWidth > 0) && (comboPtr->flags & FOCUS)) {
        GC gc;

        gc = Tk_GCForColor(comboPtr->highlightColor, drawable);
        Tk_DrawFocusHighlight(comboPtr->tkwin, gc, comboPtr->highlightWidth,
            drawable);
    }
    XCopyArea(comboPtr->display, drawable, Tk_WindowId(comboPtr->tkwin),
        comboPtr->copyGC, 0, 0, w, h, 0, 0);
    Tk_FreePixmap(comboPtr->display, drawable);
    if (comboPtr->child != NULL) {
        ArrangeChild(comboPtr);
    }
}

int
Blt_ComboFrameInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec[1] = { 
        { "comboframe", ComboFrameCmd }, 
    };
    return Blt_InitCmds(interp, "::blt", cmdSpec, 1);
}

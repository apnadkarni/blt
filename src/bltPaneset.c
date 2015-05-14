/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPaneset.c --
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

/* 
 * Modes 
 *
 *  1.  enlarge/reduce:  change only the panes touching the anchor.
 *  2.  slinky:          change all panes on both sides of the anchor.
 *  3.  hybrid:          one side slinky, the other enlarge/reduce
 *  4.  locked minus 1   changed only the pane to the left of the anchor.
 *  5.  filmstrip        move the panes left or right.
 */
#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#include "bltAlloc.h"
#include "bltChain.h"
#include "bltList.h"
#include "bltHash.h"
#include "bltSwitch.h"
#include "bltBg.h"
#include "bltOp.h"
#include "bltInitCmd.h"
#include "bltTags.h"

#define GETATTR(t,attr)         \
   (((t)->attr != NULL) ? (t)->attr : (t)->setPtr->attr)
#define VPORTWIDTH(s) \
    (ISVERT(s)) ? Tk_Height((s)->tkwin) : Tk_Width((s)->tkwin);

#define TRACE   0
#define TRACE1  0

#define SCREEN(x)       ((x) - setPtr->scrollOffset) 
#define ISVERT(s)       ((s)->flags & VERTICAL)
#define ISHORIZ(s)      (((s)->flags & VERTICAL) == 0)

/* 
 * The following are the adjustment modes for the paneset widget.
 */
typedef enum AdjustModes {
    MODE_SLINKY,                        /* Adjust all panes when
                                         * resizing */
    MODE_GIVETAKE,                      /* Adjust panes to immediate
                                         * left/right or top/bottom of
                                         * active handle. */
    MODE_SPREADSHEET,                   /* Adjust only the left pane and
                                         * the last pane. */
} AdjustMode;

typedef struct _Paneset Paneset;
typedef struct _Pane Pane;
typedef int (LimitsProc)(int value, Blt_Limits *limitsPtr);
typedef int (SizeProc)(Pane *panePtr);

/*
 * Default values for widget attributes.
 */
#define DEF_ACTIVEHANDLECOLOR   STD_ACTIVE_BACKGROUND
#define DEF_ACTIVEHANDLERELIEF  "flat"
#define DEF_ANIMATE             "0"
#define DEF_BACKGROUND          STD_NORMAL_BACKGROUND
#define DEF_BORDERWIDTH         "0"
#define DEF_HANDLEBORDERWIDTH   "1"
#define DEF_HANDLECOLOR         STD_NORMAL_BACKGROUND
#define DEF_HANDLEPAD           "0"
#define DEF_HANDLERELIEF        "flat"
#define DEF_HANDLETHICKNESS     "2"
#define DEF_HCURSOR             "sb_h_double_arrow"
#define DEF_HEIGHT              "0"
#define DEF_HIGHLIGHT_THICKNESS "1"
#define DEF_MODE                "givetake"
#define DEF_ORIENT              "horizontal"
#define DEF_PAD                 "0"
#define DEF_PANE_ANCHOR         "nw"
#define DEF_PANE_ANCHOR         "nw"
#define DEF_PANE_BORDERWIDTH    "0"
#define DEF_PANE_CURSOR         (char *)NULL
#define DEF_PANE_FILL           "none"
#define DEF_PANE_HIDE           "0"
#define DEF_PANE_HIGHLIGHT_BACKGROUND   STD_NORMAL_BACKGROUND
#define DEF_PANE_HIGHLIGHT_COLOR        RGB_BLACK
#define DEF_PANE_PAD            "0"
#define DEF_PANE_PADX           "0"
#define DEF_PANE_PADY           "0"
#define DEF_PANE_RESIZE         "shrink"
#define DEF_PANE_SHOWHANDLE     "1"
#define DEF_PANE_WEIGHT         "1.0"
#define DEF_PANE_VARIABLE       (char *)NULL
#define DEF_SCROLLCOMMAND       "0"
#define DEF_SCROLLDELAY         "30"
#define DEF_SCROLLINCREMENT     "10"
#define DEF_SIDE                "right"
#define DEF_TAKEFOCUS           "1"
#define DEF_VCURSOR             "sb_v_double_arrow"
#define DEF_WEIGHT              "0"
#define DEF_WIDTH               "0"
#define DEF_PANE_TAGS           (char *)NULL

#define PANE_DEF_ANCHOR         TK_ANCHOR_NW
#define PANE_DEF_FILL           FILL_BOTH
#define PANE_DEF_IPAD           0
#define PANE_DEF_PAD            0
#define PANE_DEF_PAD            0
#define PANE_DEF_RESIZE         RESIZE_BOTH
#define PANE_DEF_WEIGHT         1.0

#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))
#define VAR_FLAGS (TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS)

typedef struct {
    const char *handleClassName;
    const char *objectName;
    Blt_ConfigSpec *specs;
    unsigned int type;
} WidgetClass;

/*
 * Paneset structure
 *
 *      The paneset is a set of windows (panes) that may be resized in one
 *      dimension (horizontally or vertically).  How the panes are resized
 *      is dependent upon the paneset's bearing and the adjustment mode in
 *      place.
 *
 *      The bearing is the position of the handle last moved.  By default
 *      it's the last handle.  The position is the just outside of the
 *      handle. So if the window starting at 100 has a width of 200 and the
 *      handle size is 10, the bearing is 310.
 *
 *      The bearing divides the panes into two. Each side is resized
 *      according to the adjustment mode.
 *
 *      givetake        The panes immediately to the left and right of 
 *                      the bearing are grown/shrunk.
 *      slinky          All the panes on either side of the bearing are
 *                      grown/shrunk.
 *      spreadsheet     The pane to the left of the bearing and the last pane
 *                      on the right side are grown/shrunk.  Intervening
 *                      panes are unaffected.
 */

struct _Paneset {
    int flags;                          /* See the flags definitions
                                         * below. */
    WidgetClass *classPtr;              /* Type of widget: PANESET or
                                         * FILMSTRIP. */
    Display *display;                   /* Display of the widget. */
    Tk_Window tkwin;                    /* The container window into which
                                         * other widgets are arranged. For
                                         * the paneset and filmstrip, this
                                         * window is created.  For the
                                         * drawer we use an existing
                                         * window. */
    Tcl_Interp *interp;                 /* Interpreter associated with all
                                         * widgets and handles. */
    Tcl_Command cmdToken;               /* Command token associated with
                                         * this widget. For panesets and
                                         * filmstrips this is the path name
                                         * of the window created. For
                                         * drawers, this is a generated
                                         * name. */
    char *name;                         /* The generated name of the drawer
                                         * or the pathname of the window
                                         * created (panesets and
                                         * filmstrips). */
    AdjustMode mode;                    /* Panesets only: Mode to use to
                                           resize panes when the user
                                           adjusts a handle. */
    int highlightThickness;             /* Width in pixels of highlight to
                                         * draw around the handle when it
                                         * has the focus.  <= 0 means don't
                                         * draw a highlight. */
    int normalWidth;                    /* Normal dimensions of the paneset */
    int normalHeight;
    int reqWidth, reqHeight;            /* Constraints on the paneset's
                                         * normal width and
                                         * height. Overrides the requested
                                         * width of the window. */

    Tk_Cursor defVertCursor;            /* Default vertical X cursor */
    Tk_Cursor defHorzCursor;            /* Default horizontal X cursor */

    short int width, height;            /* Requested size of the widget. */

    Blt_Bg bg;                          /* 3D border surrounding the window
                                         * (viewport). */
    /*
     * Scrolling information (filmstrip only):
     */
    int worldWidth;
    int scrollOffset;                   /* Offset of viewport in world
                                         * coordinates. */
    Tcl_Obj *scrollCmdObjPtr;           /* Command strings to control
                                         * scrollbar.*/

    /* 
     * Automated scrolling information (filmstrip or drawer). 
     */
    int scrollUnits;                    /* Smallest unit of scrolling for
                                         * tabs. */
    int scrollTarget;                   /* Target offset to scroll to. */
    int scrollIncr;                     /* Current increment. */
    int interval;                       /* Current increment. */
    Tcl_TimerToken timerToken;          /* Token for timer to automatically
                                         * scroll the pane or drawer. */

    /*
     * Focus highlight ring
     */
    XColor *highlightColor;             /* Color for drawing traversal
                                         * highlight. */
    int relief;
    int activeRelief;
    Blt_Pad handlePad;
    int handleBW;
    int handleThickness;                /*  */
    int handleSize;
    Blt_Bg handleBg;
    Blt_Bg activeHandleBg;
    int handleAnchor;                   /* Last known location of handle
                                         * during a move. */

    Blt_Chain chain;                    /* List of panes/drawers. In
                                         * paneset and filmstrip widgets,
                                         * describes the order of the panes
                                         * in the widget. In the drawer
                                         * widget, represents the stacking
                                         * order of the drawers. */

    Blt_HashTable paneTable;            /* Table of panes.  Serves as a
                                         * directory to look up panes from
                                         * windows. */
    Blt_HashTable handleTable;          /* Table of handles.  Serves as a
                                         * directory to look up panes from
                                         * handle windows. */
    struct _Blt_Tags tags;              /* Table of tags. */
    Pane *activePtr;                    /* Indicates the pane with the
                                         * active handle. */
    Pane *anchorPtr;                    /* Pane that is currently
                                         * anchored */
    int bearing;                        /* Location of the split (paneset).
                                         * the drawer (drawer). */
    Tcl_Obj *cmdObjPtr;                 /* Command to invoke when the "invoke"
                                         * operation is performed. */
    size_t numVisible;                  /* # of visible panes. */
    GC gc;
    size_t nextId;                      /* Counter to generate unique
                                         * pane/drawer names. */
    size_t nextHandleId;                /* Counter to generate unique
                                         * pane/drawer names. */
};

/*
 * Paneset flags definitions
 */
#define REDRAW_PENDING  (1<<0)          /* A redraw request is pending. */
#define LAYOUT_PENDING  (1<<1)          /* Get the requested sizes of the
                                         * widgets before
                                         * expanding/shrinking the size of
                                         * the container.  It's necessary
                                         * to recompute the layout every
                                         * time a pane is added,
                                         * reconfigured, or deleted, but
                                         * not when the container is
                                         * resized. */
#define SCROLL_PENDING  (1<<2)          /* Get the requested sizes of the
                                         * widgets before
                                         * expanding/shrinking the size of
                                         * the container.  It's necessary
                                         * to recompute the layout every
                                         * time a pane is added,
                                         * reconfigured, or deleted, but
                                         * not when the container is
                                         * resized. */
#define ANIMATE         (1<<3)          /* Animate pane moves. */

#define FOCUS           (1<<6)

#define VERTICAL        (1<<7)

#define PANESET         (BLT_CONFIG_USER_BIT << 1)
#define FILMSTRIP       (BLT_CONFIG_USER_BIT << 2)
#define ALL             (PANESET|FILMSTRIP)

/*
 * Pane --
 *
 *      A pane holds a window and a possibly a handle.  It describes how the
 *      window should appear in the pane.  The handle is a rectangle on the
 *      far edge of the pane (horizontal right, vertical bottom).  Normally
 *      the last pane does not have a handle.  Handles may be hidden.
 *
 *      Initially, the size of a pane consists of
 *       1. the requested size embedded window,
 *       2. any requested internal padding, and
 *       3. the size of the handle (if one is displayed). 
 *
 *      Note: There is no 3D border around the pane.  This can be added
 *            by embedding a frame.  This simplifies the widget so that
 *            there is only one window for the widget.  Windows outside of
 *            the boundary of the pane are occluded.
 */
struct _Pane  {
    Tk_Window tkwin;                    /* Widget to be managed. */
    Tk_Window handle;                   /* Handle subwindow. */

    Tk_Cursor cursor;                   /* X Cursor */

    const char *name;                   /* Name of pane */

    unsigned int side;                  /* The side of the widget where this
                                         * drawer is attached. */
    unsigned int flags;

    Paneset *setPtr;                    /* Paneset widget managing this pane. */

    int borderWidth;                    /* The external border width of the
                                         * widget. This is needed to check if
                                         * Tk_Changes(tkwin)->border_width
                                         * changes. */

    XColor *highlightBgColor;           /* Color for drawing traversal
                                         * highlight area when highlight is
                                         * off. */
    XColor *highlightColor;             /* Color for drawing traversal
                                         * highlight. */

    const char *takeFocus;              /* Says whether to select this widget
                                         * during tab traveral operations.
                                         * This value isn't used in C code,
                                         * but for the widget's TCL
                                         * bindings. */

    Blt_Limits reqWidth, reqHeight;     /* Bounds for width and height
                                         * requests made by the widget. */
    Tk_Anchor anchor;                   /* Anchor type: indicates how the
                                         * widget is positioned if extra space
                                         * is available in the pane. */

    Blt_Pad xPad;                       /* Extra padding placed left and right
                                         * of the widget. */
    Blt_Pad yPad;                       /* Extra padding placed above and below
                                         * the widget */

    int iPadX, iPadY;                   /* Extra padding added to the interior
                                         * of the widget (i.e. adds to the
                                         * requested size of the widget) */

    int fill;                           /* Indicates how the widget should fill
                                         * the pane it occupies. */
    int resize;                         /* Indicates if the pane should
                                         * expand/shrink. */

    int x, y;                           /* Origin of pane wrt container. */

    short int width, height;            /* Size of pane, including handle. */

    Blt_ChainLink link;                 /* Pointer of this pane into the list
                                         * of panes. */

    Blt_HashEntry *hashPtr;             /* Pointer of this pane into hashtable
                                         * of panes. */
    Blt_HashEntry *handleHashPtr;       /* Pointer of this pane into
                                         * hashtable of handles. */

    int index;                          /* Index of the pane. */

    int size;                           /* Current size of the pane. This size
                                         * is bounded by min and max. */

    /*
     * nom and size perform similar duties.  I need to keep track of the
     * amount of space allocated to the pane (using size).  But at the same
     * time, I need to indicate that space can be parcelled out to this pane.
     * If a nominal size was set for this pane, I don't want to add space.
     */

    int nom;                            /* The nominal size (neither expanded
                                         * nor shrunk) of the pane based upon
                                         * the requested size of the widget
                                         * embedded in this pane. */

    int min, max;                       /* Size constraints on the pane */

    float weight;                       /* Weight of pane. */

    Blt_Limits reqSize;                 /* Requested bounds for the size of
                                         * the pane. The pane will not expand
                                         * or shrink beyond these limits,
                                         * regardless of how it was specified
                                         * (max widget size).  This includes
                                         * any extra padding which may be
                                         * specified. */
    Blt_Bg handleBg;
    Blt_Bg activeHandleBg;
    Blt_Bg bg;                  /* 3D background border surrounding
                                         * the widget */
    Tcl_Obj *cmdObjPtr;

    Tcl_TimerToken timerToken;
    int scrollTarget;                   /* Target offset to scroll to. */
    int scrollIncr;                     /* Current increment. */

    Tcl_Obj *variableObjPtr;            /* Name of TCL variable.  If non-NULL,
                                         * this variable will be set to the
                                         * value string of the selected
                                         * item. */

};

/* Pane/handle flags.  */

#define HIDDEN          (1<<8)          /* Do not display the pane. */
#define DISABLED        (1<<9)          /* Handle is disabled. */
#define ONSCREEN        (1<<10)         /* Pane is on-screen. */
#define HANDLE_ACTIVE   (1<<11)         /* Handle is currently active. */
#define HANDLE          (1<<12)         /* The pane has a handle. */
#define SHOW_HANDLE     (1<<13)         /* Display the pane. */
#define SHRINK          (1<<14)         /* Shrink the window to fit the
                                         * drawer, instead of moving it. */
#define VIRGIN          (1<<24)

/* Orientation. */
#define SIDE_VERTICAL   (SIDE_TOP|SIDE_BOTTOM)
#define SIDE_HORIZONTAL (SIDE_LEFT|SIDE_RIGHT)

/* Handle positions. */
#define HANDLE_LEFT     SIDE_RIGHT
#define HANDLE_RIGHT    SIDE_LEFT
#define HANDLE_TOP      SIDE_BOTTOM
#define HANDLE_BOTTOM   SIDE_TOP

#define HANDLE_FARSIDE  (HANDLE_RIGHT|HANDLE_BOTTOM)    
#define HANDLE_NEARSIDE (HANDLE_LEFT|HANDLE_TOP)        


static Tk_GeomRequestProc PaneGeometryProc;
static Tk_GeomLostSlaveProc PaneCustodyProc;
static Tk_GeomMgr panesetMgrInfo =
{
    (char *)"paneset",                  /* Name of geometry manager used by
                                         * winfo */
    PaneGeometryProc,                   /* Procedure to for new geometry
                                         * requests */
    PaneCustodyProc,                    /* Procedure when widget is taken
                                         * away */
};

static Blt_OptionParseProc ObjToChild;
static Blt_OptionPrintProc ChildToObj;
static Blt_CustomOption childOption = {
    ObjToChild, ChildToObj, NULL, (ClientData)0,
};

extern Blt_CustomOption bltLimitsOption;

static Blt_OptionParseProc ObjToOrientProc;
static Blt_OptionPrintProc OrientToObjProc;
static Blt_CustomOption orientOption = {
    ObjToOrientProc, OrientToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToMode;
static Blt_OptionPrintProc ModeToObj;
static Blt_CustomOption adjustOption = {
    ObjToMode, ModeToObj, NULL, (ClientData)0,
};

static Blt_OptionFreeProc FreeTagsProc;
static Blt_OptionParseProc ObjToTagsProc;
static Blt_OptionPrintProc TagsToObjProc;
static Blt_CustomOption tagsOption = {
    ObjToTagsProc, TagsToObjProc, FreeTagsProc, (ClientData)0
};

static Blt_ConfigSpec paneSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activesashcolor", "activeSashColor", 
        "ActiveSashColor", DEF_ACTIVEHANDLECOLOR, 
        Blt_Offset(Pane, activeHandleBg), 
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_ANCHOR, "-anchor", (char *)NULL, (char *)NULL, DEF_PANE_ANCHOR,
        Blt_Offset(Pane, anchor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        (char *)NULL, Blt_Offset(Pane, bg), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor",
        DEF_PANE_CURSOR, Blt_Offset(Pane, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FILL, "-fill", "fill", "Fill", DEF_PANE_FILL, 
        Blt_Offset(Pane, fill), BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_SYNONYM, "-height", "reqHeight", (char *)NULL, (char *)NULL, 
        Blt_Offset(Pane, reqHeight), 0},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_PANE_HIDE, 
        Blt_Offset(Pane, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDDEN },
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
        "HighlightBackground", DEF_PANE_HIGHLIGHT_BACKGROUND, 
        Blt_Offset(Pane, highlightBgColor), 0},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
        DEF_PANE_HIGHLIGHT_COLOR, Blt_Offset(Pane, highlightColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-ipadx", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(Pane, iPadX), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-ipady", (char *)NULL, (char *)NULL, 
        (char *)NULL, Blt_Offset(Pane, iPadY), 0},
    {BLT_CONFIG_CUSTOM, "-reqheight", "reqHeight", (char *)NULL, (char *)NULL, 
        Blt_Offset(Pane, reqHeight), 0, &bltLimitsOption},
    {BLT_CONFIG_CUSTOM, "-reqwidth", "reqWidth", (char *)NULL, (char *)NULL, 
        Blt_Offset(Pane, reqWidth), 0, &bltLimitsOption},
    {BLT_CONFIG_RESIZE, "-resize", "resize", "Resize", DEF_PANE_RESIZE,
        Blt_Offset(Pane, resize), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-sashcolor", "sashColor", "SashColor",
        DEF_HANDLECOLOR, Blt_Offset(Pane, handleBg), 
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BITMASK, "-showsash", "showSash", "showSash", 
        DEF_PANE_SHOWHANDLE, Blt_Offset(Pane, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SHOW_HANDLE},
    {BLT_CONFIG_CUSTOM, "-size", (char *)NULL, (char *)NULL, (char *)NULL, 
        Blt_Offset(Pane, reqSize), 0, &bltLimitsOption},
     {BLT_CONFIG_CUSTOM, "-tags", (char *)NULL, (char *)NULL,
        DEF_PANE_TAGS, 0, BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
        DEF_TAKEFOCUS, Blt_Offset(Pane, takeFocus), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FLOAT, "-weight", "weight", "Weight", DEF_PANE_WEIGHT,
        Blt_Offset(Pane, weight), BLT_CONFIG_DONT_SET_DEFAULT | PANESET},
    {BLT_CONFIG_SYNONYM, "-width", "reqWidth", (char *)NULL, (char *)NULL, 
        Blt_Offset(Pane, reqWidth), 0},
    {BLT_CONFIG_CUSTOM, "-window", "window", "Window", (char *)NULL, 
        Blt_Offset(Pane, tkwin), BLT_CONFIG_NULL_OK, &childOption },
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec frameSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activehandlecolor", "activeHandleColor", 
        "HandleColor", DEF_ACTIVEHANDLECOLOR, Blt_Offset(Pane, activeHandleBg), 
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        (char *)NULL, Blt_Offset(Pane, bg), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor",
        DEF_PANE_CURSOR, Blt_Offset(Pane, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FILL, "-fill", "fill", "Fill", DEF_PANE_FILL, 
        Blt_Offset(Pane, fill), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-handlecolor", "handleColor", "HandleColor",
        DEF_HANDLECOLOR, Blt_Offset(Pane, handleBg), 
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-height", "reqHeight", (char *)NULL, (char *)NULL, 
        Blt_Offset(Pane, reqHeight), 0},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_PANE_HIDE, 
        Blt_Offset(Pane, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDDEN },
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
        "HighlightBackground", DEF_PANE_HIGHLIGHT_BACKGROUND, 
        Blt_Offset(Pane, highlightBgColor), 0},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
        DEF_PANE_HIGHLIGHT_COLOR, Blt_Offset(Pane, highlightColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-ipadx", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(Pane, iPadX), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-ipady", (char *)NULL, (char *)NULL, 
        (char *)NULL, Blt_Offset(Pane, iPadY), 0},
    {BLT_CONFIG_CUSTOM, "-reqheight", "reqHeight", (char *)NULL, (char *)NULL, 
        Blt_Offset(Pane, reqHeight), 0, &bltLimitsOption},
    {BLT_CONFIG_CUSTOM, "-reqwidth", "reqWidth", (char *)NULL, (char *)NULL, 
        Blt_Offset(Pane, reqWidth), 0, &bltLimitsOption},
    {BLT_CONFIG_RESIZE, "-resize", "resize", "Resize", DEF_PANE_RESIZE,
        Blt_Offset(Pane, resize), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-showhandle", "showHandle", "showHandle", 
        DEF_PANE_SHOWHANDLE, Blt_Offset(Pane, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SHOW_HANDLE },
    {BLT_CONFIG_CUSTOM, "-size", (char *)NULL, (char *)NULL, (char *)NULL, 
        Blt_Offset(Pane, reqSize), 0, &bltLimitsOption},
     {BLT_CONFIG_CUSTOM, "-tags", (char *)NULL, (char *)NULL,
        DEF_PANE_TAGS, 0, BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
        DEF_TAKEFOCUS, Blt_Offset(Pane, takeFocus), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-width", "reqWidth", (char *)NULL, (char *)NULL, 
        Blt_Offset(Pane, reqWidth), 0},
    {BLT_CONFIG_CUSTOM, "-window", "window", "Window", (char *)NULL, 
        Blt_Offset(Pane, tkwin), BLT_CONFIG_NULL_OK, &childOption },
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

/* 
 * Hide the handle. 
 *
 *      .p configure -handlethickness 0
 *      .p pane configure -hide yes 
 *      Put all the drawers in the paneset widget, hidden by default.
 *      Reveal/hide drawers to pop them out.
 *      plotarea | sidebar | scroller
 */
static Blt_ConfigSpec panesetSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activehandlecolor", "activeHandleColor", 
        "HandleColor", DEF_ACTIVEHANDLECOLOR, 
        Blt_Offset(Paneset, activeHandleBg), FILMSTRIP},
    {BLT_CONFIG_RELIEF, "-activehandlerelief", "activeHandleRelief", 
        "HandleRelief", DEF_ACTIVEHANDLERELIEF, 
        Blt_Offset(Paneset, activeRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT | FILMSTRIP },
    {BLT_CONFIG_BACKGROUND, "-activesashcolor", "activeSashColor", 
        "SashColor", DEF_ACTIVEHANDLECOLOR, 
        Blt_Offset(Paneset, activeHandleBg), PANESET},
    {BLT_CONFIG_RELIEF, "-activesashrelief", "activeSashRelief", 
        "SashRelief", DEF_ACTIVEHANDLERELIEF, 
        Blt_Offset(Paneset, activeRelief), BLT_CONFIG_DONT_SET_DEFAULT|PANESET},
    {BLT_CONFIG_BITMASK, "-animate", "animate", "Animate", DEF_ANIMATE, 
        Blt_Offset(Paneset, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT | FILMSTRIP, (Blt_CustomOption *)ANIMATE },
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BACKGROUND, Blt_Offset(Paneset, bg), ALL },
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 
        0, ALL},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_HEIGHT,
        Blt_Offset(Paneset, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT | ALL },
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
        "HighlightThickness", DEF_HIGHLIGHT_THICKNESS, 
        Blt_Offset(Paneset, highlightThickness), 
        BLT_CONFIG_DONT_SET_DEFAULT | ALL},
    {BLT_CONFIG_CUSTOM, "-orient", "orient", "Orient", DEF_ORIENT, 
        Blt_Offset(Paneset, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT | PANESET | FILMSTRIP, &orientOption},
    {BLT_CONFIG_CUSTOM, "-mode", "mode", "Mode", DEF_MODE,
        Blt_Offset(Paneset, mode), BLT_CONFIG_DONT_SET_DEFAULT | PANESET, 
        &adjustOption},
    {BLT_CONFIG_CUSTOM, "-reqheight", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(Paneset, reqHeight), ALL, &bltLimitsOption},
    {BLT_CONFIG_CUSTOM, "-reqwidth", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(Paneset, reqWidth), ALL, &bltLimitsOption},
    {BLT_CONFIG_PIXELS_NNEG, "-handleborderwidth", "handleBorderWidth", 
        "HandleBorderWidth", DEF_HANDLEBORDERWIDTH, 
        Blt_Offset(Paneset, handleBW), 
        BLT_CONFIG_DONT_SET_DEFAULT | FILMSTRIP },
    {BLT_CONFIG_BACKGROUND, "-handlecolor", "handleColor", "HandleColor",
        DEF_HANDLECOLOR, Blt_Offset(Paneset, handleBg), FILMSTRIP},
    {BLT_CONFIG_PAD, "-handlepad", "handlePad", "HandlePad", DEF_HANDLEPAD, 
        Blt_Offset(Paneset, handlePad), 
        BLT_CONFIG_DONT_SET_DEFAULT | FILMSTRIP},
    {BLT_CONFIG_RELIEF, "-handlerelief", "handleRelief", "HandleRelief", 
        DEF_HANDLERELIEF, Blt_Offset(Paneset, relief), 
        BLT_CONFIG_DONT_SET_DEFAULT | FILMSTRIP },
    {BLT_CONFIG_PIXELS_NNEG, "-handlethickness", "handleThickness", 
        "HandleThickness", DEF_HANDLETHICKNESS, 
        Blt_Offset(Paneset, handleThickness), 
        BLT_CONFIG_DONT_SET_DEFAULT| FILMSTRIP },
    {BLT_CONFIG_PIXELS_NNEG, "-sashborderwidth", "sashBorderWidth", 
        "SashBorderWidth", DEF_HANDLEBORDERWIDTH, 
        Blt_Offset(Paneset, handleBW), BLT_CONFIG_DONT_SET_DEFAULT | PANESET},
    {BLT_CONFIG_BACKGROUND, "-sashcolor", "sashColor", "SashColor",
        DEF_HANDLECOLOR, Blt_Offset(Paneset, handleBg), PANESET},
    {BLT_CONFIG_PAD, "-sashpad", "sashPad", "SashPad", DEF_HANDLEPAD, 
        Blt_Offset(Paneset, handlePad), BLT_CONFIG_DONT_SET_DEFAULT | PANESET},
    {BLT_CONFIG_RELIEF, "-sashrelief", "sashRelief", "SashRelief", 
        DEF_HANDLERELIEF, Blt_Offset(Paneset, relief), 
        BLT_CONFIG_DONT_SET_DEFAULT | PANESET },
    {BLT_CONFIG_PIXELS_NNEG, "-sashthickness", "sashThickness", 
        "SashThickness", DEF_HANDLETHICKNESS, 
        Blt_Offset(Paneset, handleThickness), 
        BLT_CONFIG_DONT_SET_DEFAULT| PANESET },
    {BLT_CONFIG_OBJ, "-scrollcommand", "scrollCommand", "ScrollCommand",
        (char *)NULL, Blt_Offset(Paneset, scrollCmdObjPtr),
        BLT_CONFIG_NULL_OK | FILMSTRIP },
    {BLT_CONFIG_PIXELS_POS, "-scrollincrement", "scrollIncrement",
        "ScrollIncrement", DEF_SCROLLINCREMENT, Blt_Offset(Paneset,scrollUnits),
        BLT_CONFIG_DONT_SET_DEFAULT | FILMSTRIP },
    {BLT_CONFIG_INT_NNEG, "-scrolldelay", "scrollDelay", "ScrollDelay",
        DEF_SCROLLDELAY, Blt_Offset(Paneset, interval),
        BLT_CONFIG_DONT_SET_DEFAULT | FILMSTRIP },
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_WIDTH,
        Blt_Offset(Paneset, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT | ALL},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};


/*
 * PaneIterator --
 *
 *      Panes may be tagged with strings.  A pane may have many tags.  The
 *      same tag may be used for many panes.
 *      
 */
typedef enum { 
    ITER_SINGLE, ITER_ALL, ITER_TAG, ITER_PATTERN, 
} IteratorType;

typedef struct _Iterator {
    Paneset *setPtr;                   /* Paneset that we're iterating over. */

    IteratorType type;                  /* Type of iteration:
                                         * ITER_TAG      By item tag.
                                         * ITER_ALL      By every item.
                                         * ITER_SINGLE   Single item: either 
                                         *               tag or index.
                                         * ITER_PATTERN  Over a consecutive 
                                         *               range of indices.
                                         */

    Pane *startPtr;                     /* Starting pane.  Starting point of
                                         * search, saved if iterator is reused.
                                         * Used for ITER_ALL and ITER_SINGLE
                                         * searches. */
    Pane *endPtr;                       /* Ending pend (inclusive). */

    Pane *nextPtr;                      /* Next pane. */

    /* For tag-based searches. */
    char *tagName;                      /* If non-NULL, is the tag that we are
                                         * currently iterating over. */
    Blt_ChainLink link;
} PaneIterator;


static WidgetClass panesetClass = {
    "BltPanesetSash",
    "pane",
    paneSpecs,
    PANESET,
};

static WidgetClass filmstripClass = {
    "BltFilmStripHandle",
    "frame",
    frameSpecs,
    FILMSTRIP,
};


/*
 * Forward declarations
 */
static Tcl_FreeProc PanesetFreeProc;
static Tcl_IdleProc DisplayProc;
static Tcl_IdleProc DisplayHandle;
static Tcl_ObjCmdProc PanesetCmd;
static Tcl_ObjCmdProc FilmstripCmd;
static Tk_EventProc PanesetEventProc;
static Tk_EventProc PaneEventProc;
static Tk_EventProc HandleEventProc;
static Tcl_FreeProc PaneFreeProc;
static Tcl_ObjCmdProc PanesetInstCmdProc;
static Tcl_CmdDeleteProc PanesetInstCmdDeleteProc;
static Tcl_TimerProc MotionTimerProc;

static int GetPaneIterator(Tcl_Interp *interp, Paneset *setPtr, Tcl_Obj *objPtr,
        PaneIterator *iterPtr);
static int GetPaneFromObj(Tcl_Interp *interp, Paneset *setPtr, Tcl_Obj *objPtr, 
        Pane **panePtrPtr);

static int 
ScreenX(Pane *panePtr)
{
    Paneset *setPtr;
    int x;

    x = panePtr->x;
    setPtr = panePtr->setPtr;
    if ((setPtr->classPtr->type == FILMSTRIP) && (ISHORIZ(setPtr))) {
        x -= setPtr->scrollOffset;
    }
    return x;
}

static int 
ScreenY(Pane *panePtr)
{
    Paneset *setPtr;
    int y;

    setPtr = panePtr->setPtr;
    y = panePtr->y;
    if ((setPtr->classPtr->type == FILMSTRIP) && (ISVERT(setPtr))) {
        y -= setPtr->scrollOffset;
    }
    return y;
}

/*
 *---------------------------------------------------------------------------
 *
 * BoundWidth --
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
BoundWidth(int width, Blt_Limits *limitsPtr)    
{
    /*
     * Check widgets for requested width values;
     */
    if (limitsPtr->flags & LIMITS_NOM_SET) {
        width = limitsPtr->nom;         /* Override initial value */
    }
    if (width < limitsPtr->min) {
        width = limitsPtr->min;         /* Bounded by minimum value */
    }
    if (width > limitsPtr->max) {
        width = limitsPtr->max;         /* Bounded by maximum value */
    }
    return width;
}

/*
 *---------------------------------------------------------------------------
 *
 * BoundHeight --
 *
 *      Bounds a given value to the limits described in the limit structure.
 *      The initial starting value may be overridden by the nominal value in
 *      the limits.
 *
 * Results:
 *      Returns the constrained value.
 *
 *---------------------------------------------------------------------------
 */
static int
BoundHeight(int height, Blt_Limits *limitsPtr)
{
    /*
     * Check widgets for requested height values;
     */
    if (limitsPtr->flags & LIMITS_NOM_SET) {
        height = limitsPtr->nom;        /* Override initial value */
    }
    if (height < limitsPtr->min) {
        height = limitsPtr->min;        /* Bounded by minimum value */
    } 
    if (height > limitsPtr->max) {
        height = limitsPtr->max;        /* Bounded by maximum value */
    }
    return height;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetReqWidth --
 *
 *      Returns the width requested by the window embedded in the given pane.
 *      The requested space also includes any internal padding which has been
 *      designated for this widget.
 *
 *      The requested width of the widget is always bounded by the limits set
 *      in panePtr->reqWidth.
 *
 * Results:
 *      Returns the requested width of the widget.
 *
 *---------------------------------------------------------------------------
 */
static int
GetReqWidth(Pane *panePtr)
{
    int w;

    w = (2 * panePtr->iPadX);           /* Start with any addition padding
                                         * requested for the pane. */
    if (panePtr->tkwin != NULL) {       /* Add in the requested width. */
        w += Tk_ReqWidth(panePtr->tkwin);
    }
    return BoundWidth(w, &panePtr->reqWidth);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetReqHeight --
 *
 *      Returns the height requested by the widget starting in the given pane.
 *      The requested space also includes any internal padding which has been
 *      designated for this widget.
 *
 *      The requested height of the widget is always bounded by the limits set
 *      in panePtr->reqHeight.
 *
 * Results:
 *      Returns the requested height of the widget.
 *
 *---------------------------------------------------------------------------
 */
static int
GetReqHeight(Pane *panePtr)
{
    int h;

    h = 2 * panePtr->iPadY;
    if (panePtr->tkwin != NULL) {
        h += Tk_ReqHeight(panePtr->tkwin);
    }
    h = BoundHeight(h, &panePtr->reqHeight);
    return h;
}

static void
EventuallyRedraw(Paneset *setPtr)
{
    if ((setPtr->flags & REDRAW_PENDING) == 0) {
        setPtr->flags |= REDRAW_PENDING;
        Tcl_DoWhenIdle(DisplayProc, setPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SetTag --
 *
 *      Associates a tag with a given row.  Individual row tags are
 *      stored in hash tables keyed by the tag name.  Each table is in
 *      turn stored in a hash table keyed by the row location.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      A tag is stored for a particular row.
 *
 *---------------------------------------------------------------------------
 */
static int
SetTag(Tcl_Interp *interp, Pane *panePtr, const char *tagName)
{
    Paneset *setPtr;
    long dummy;
    
    if ((strcmp(tagName, "all") == 0) || (strcmp(tagName, "end") == 0)) {
        return TCL_OK;                  /* Don't need to create reserved
                                         * tags. */
    }
    if (tagName[0] == '\0') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tagName, "\" can't be empty.", 
                (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (tagName[0] == '-') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tagName, 
                "\" can't start with a '-'.", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (Blt_GetLong(NULL, (char *)tagName, &dummy) == TCL_OK) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tagName, "\" can't be a number.",
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    setPtr = panePtr->setPtr;
    Blt_Tags_AddItemToTag(&setPtr->tags, tagName, panePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyPane --
 *
 *      Removes the Pane structure from the hash table and frees the memory
 *      allocated by it.  
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the pane is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyPane(Pane *panePtr)
{
    Paneset *setPtr;

    setPtr = panePtr->setPtr;
    Blt_Tags_ClearTagsFromItem(&setPtr->tags, panePtr);
    Blt_FreeOptions(paneSpecs, (char *)panePtr, setPtr->display, 0);
    if (panePtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(panePtr->timerToken);
        panePtr->timerToken = 0;
    }
    if (setPtr->anchorPtr == panePtr) {
        setPtr->anchorPtr = NULL;
    }
    if (panePtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&setPtr->paneTable, panePtr->hashPtr);
        panePtr->hashPtr = NULL;
    }
    if (panePtr->link != NULL) {
        Blt_Chain_DeleteLink(setPtr->chain, panePtr->link);
        panePtr->link = NULL;
    }
    if (panePtr->tkwin != NULL) {
        Tk_Window tkwin;

        tkwin = panePtr->tkwin;
        Tk_DeleteEventHandler(tkwin, StructureNotifyMask, PaneEventProc, 
                panePtr);
        Tk_ManageGeometry(tkwin, (Tk_GeomMgr *)NULL, panePtr);
        Tk_DestroyWindow(tkwin);
    }
    if (panePtr->handleHashPtr != NULL) {
        Blt_DeleteHashEntry(&setPtr->handleTable, panePtr->handleHashPtr);
        panePtr->handleHashPtr = NULL;
    }
    if (panePtr->handle != NULL) {
        Tk_Window tkwin;

        tkwin = panePtr->handle;
        Tk_DeleteEventHandler(tkwin, 
                ExposureMask|FocusChangeMask|StructureNotifyMask, 
                HandleEventProc, panePtr);
        Tk_ManageGeometry(tkwin, (Tk_GeomMgr *)NULL, panePtr);
        panePtr->handle = NULL;
        Tk_DestroyWindow(tkwin);
    }
    Blt_Free(panePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToChild --
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
ObjToChild(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to report results */
    Tk_Window parent,                   /* Parent window */
    Tcl_Obj *objPtr,                    /* String representation. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Pane *panePtr = (Pane *)widgRec;
    Paneset *setPtr;
    Tk_Window *tkwinPtr = (Tk_Window *)(widgRec + offset);
    Tk_Window old, tkwin;
    char *string;

    old = *tkwinPtr;
    tkwin = NULL;
    setPtr = panePtr->setPtr;
    string = Tcl_GetString(objPtr);
    if (string[0] != '\0') {
        tkwin = Tk_NameToWindow(interp, string, setPtr->tkwin);
        if (tkwin == NULL) {
            return TCL_ERROR;
        }
        if (tkwin == old) {
            return TCL_OK;
        }
        /*
         * Allow only widgets that are children of the paneset/drawer window
         * to be used.  We are using the window as viewport to clip the
         * children are necessary.
         */
        parent = Tk_Parent(tkwin);
        if (parent != setPtr->tkwin) {
            Tcl_AppendResult(interp, "can't manage \"", Tk_PathName(tkwin),
                "\" in paneset \"", Tk_PathName(setPtr->tkwin), "\"",
                (char *)NULL);
            return TCL_ERROR;
        }
        Tk_ManageGeometry(tkwin, &panesetMgrInfo, panePtr);
        Tk_CreateEventHandler(tkwin, StructureNotifyMask, PaneEventProc, 
                panePtr);
        /*
         * We need to make the window to exist immediately.  If the window is
         * torn off (placed into another container window), the timing between
         * the container and the its new child (this window) gets tricky.
         * This should work for Tk 4.2.
         */
        Tk_MakeWindowExist(tkwin);
    }
    if (old != NULL) {
        Tk_DeleteEventHandler(old, StructureNotifyMask, PaneEventProc, panePtr);
        Tk_ManageGeometry(old, (Tk_GeomMgr *)NULL, panePtr);
        Tk_UnmapWindow(old);
    }
    *tkwinPtr = tkwin;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildToObj --
 *
 *      Converts the Tk window back to a Tcl_Obj (i.e. its name).
 *
 * Results:
 *      The name of the window is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ChildToObj(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window parent,                   /* Not used. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Tk_Window tkwin = *(Tk_Window *)(widgRec + offset);
    Tcl_Obj *objPtr;

    if (tkwin == NULL) {
        objPtr = Tcl_NewStringObj("", -1);
    } else {
        objPtr = Tcl_NewStringObj(Tk_PathName(tkwin), -1);
    }
    return objPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToMode --
 *
 *      Converts an adjust mode name into a enum.
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
ObjToMode(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to report results. */
    Tk_Window parent,                   /* Parent window */
    Tcl_Obj *objPtr,                    /* String representation. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    AdjustMode *modePtr = (AdjustMode *)(widgRec + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (strcmp(string, "slinky") == 0) {
        *modePtr = MODE_SLINKY;
    } else if (strcmp(string, "givetake") == 0) {
        *modePtr = MODE_GIVETAKE;
    } else if (strcmp(string, "spreadsheet") == 0) {
        *modePtr = MODE_SPREADSHEET;
    } else {
        Tcl_AppendResult(interp, "unknown mode \"", string, "\": should be "
                "givetake, slinky, or spreadsheet\"", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildToObj --
 *
 *      Converts the enum back to a mode string (i.e. its name).
 *
 * Results:
 *      The name of the mode is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ModeToObj(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window parent,                   /* Not used. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    AdjustMode mode = *(AdjustMode *)(widgRec + offset);
    const char *string;

    switch (mode) {
    case MODE_SLINKY: 
        string = "slinky"; 
        break;
    case MODE_GIVETAKE: 
        string = "givetake"; 
        break;
    case MODE_SPREADSHEET: 
        string = "spreadsheet"; 
        break;
    default:
        string = "???"; 
        break;
    }
    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToOrientProc --
 *
 *      Converts the string representing a state into a bitflag.
 *
 * Results:
 *      The return value is a standard TCL result.  The state flags are
 *      updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToOrientProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing state. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Paneset *setPtr = (Paneset *)(widgRec);
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    const char *string;
    int orient;
    int length;

    string = Tcl_GetString(objPtr);
    length = strlen(string);
    if (strncmp(string, "vertical", length) == 0) {
        orient = VERTICAL;
    } else if (strncmp(string, "horizontal", length) == 0) {
        orient = 0;
    } else {
        Tcl_AppendResult(interp, "bad orientation \"", string,
            "\": must be vertical or horizontal", (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~VERTICAL;
    *flagsPtr |= orient;
    setPtr->flags |= LAYOUT_PENDING;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OrientToObjProc --
 *
 *      Return the name of the style.
 *
 * Results:
 *      The name representing the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
OrientToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget information record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    unsigned int orient = *(unsigned int *)(widgRec + offset);
    const char *string;

    if (orient & VERTICAL) {
        string = "vertical";    
    } else {
        string = "horizontal";  
    }
    return Tcl_NewStringObj(string, -1);
}

/*ARGSUSED*/
static void
FreeTagsProc(
    ClientData clientData,
    Display *display,                   /* Not used. */
    char *widgRec,
    int offset)
{
    Paneset *setPtr;
    Pane *panePtr = (Pane *)widgRec;

    setPtr = panePtr->setPtr;
    Blt_Tags_ClearTagsFromItem(&setPtr->tags, panePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTagsProc --
 *
 *      Convert the string representation of a list of tags.
 *
 * Results:
 *      The return value is a standard TCL result.  The tags are
 *      save in the widget.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTagsProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing style. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Paneset *setPtr;
    Pane *panePtr = (Pane *)widgRec;
    int i;
    char *string;
    int objc;
    Tcl_Obj **objv;

    setPtr = panePtr->setPtr;
    Blt_Tags_ClearTagsFromItem(&setPtr->tags, panePtr);
    string = Tcl_GetString(objPtr);
    if ((string[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
        return TCL_OK;
    }
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
        SetTag(interp, panePtr, Tcl_GetString(objv[i]));
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagsToObjProc --
 *
 *      Return the name of the style.
 *
 * Results:
 *      The name representing the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TagsToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget information record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Paneset *setPtr;
    Pane *panePtr = (Pane *)widgRec;
    Tcl_Obj *listObjPtr;

    setPtr = panePtr->setPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Blt_Tags_AppendTagsToObj(&setPtr->tags, panePtr, listObjPtr);
    return listObjPtr;
}

static void
EventuallyRedrawHandle(Pane *panePtr)
{
    if ((panePtr->flags & REDRAW_PENDING) == 0) {
        panePtr->flags |= REDRAW_PENDING;
        Tcl_DoWhenIdle(DisplayHandle, panePtr);
    }
}


static Pane *
FirstPane(Paneset *setPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(setPtr->chain); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;

        panePtr = Blt_Chain_GetValue(link);
        if ((panePtr->flags & (HIDDEN|DISABLED)) == 0) {
            return panePtr;
        }
    }
    return NULL;
}

static Pane *
LastPane(Paneset *setPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(setPtr->chain); link != NULL;
         link = Blt_Chain_PrevLink(link)) {
        Pane *panePtr;

        panePtr = Blt_Chain_GetValue(link);
        if ((panePtr->flags & (HIDDEN|DISABLED)) == 0) {
            return panePtr;
        }
    }
    return NULL;
}


static Pane *
NextPane(Pane *panePtr)
{
    if (panePtr != NULL) {
        Blt_ChainLink link;

        for (link = Blt_Chain_NextLink(panePtr->link); link != NULL; 
             link = Blt_Chain_NextLink(link)) {
            panePtr = Blt_Chain_GetValue(link);
            if ((panePtr->flags & (HIDDEN|DISABLED)) == 0) {
                return panePtr;
            }
        }
    }
    return NULL;
}

static Pane *
PrevPane(Pane *panePtr)
{
    if (panePtr != NULL) {
        Blt_ChainLink link;
        
        for (link = Blt_Chain_PrevLink(panePtr->link); link != NULL; 
             link = Blt_Chain_PrevLink(link)) {
            panePtr = Blt_Chain_GetValue(link);
            if ((panePtr->flags & HIDDEN) == 0) {
                return panePtr;
            }
        }
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * PanesetEventProc --
 *
 *      This procedure is invoked by the Tk event handler when the container
 *      widget is reconfigured or destroyed.
 *
 *      The paneset will be rearranged at the next idle point if the container
 *      widget has been resized or moved. There's a distinction made between
 *      parent and non-parent container arrangements.  When the container is
 *      the parent of the embedded widgets, the widgets will automatically
 *      keep their positions relative to the container, even when the
 *      container is moved.  But if the container is not the parent, those
 *      widgets have to be moved manually.  This can be a performance hit in
 *      rare cases where we're scrolling the container (by moving the window)
 *      and there are lots of non-child widgets arranged inside.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the paneset associated with tkwin to have its layout
 *      re-computed and drawn at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static void
PanesetEventProc(ClientData clientData, XEvent *eventPtr)
{
    Paneset *setPtr = clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedraw(setPtr);
        }
    } else if (eventPtr->type == DestroyNotify) {
        if (setPtr->tkwin != NULL) {
            Blt_DeleteWindowInstanceData(setPtr->tkwin);
            setPtr->tkwin = NULL;
            Tcl_DeleteCommandFromToken(setPtr->interp, setPtr->cmdToken);
        }
        if (setPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayProc, setPtr);
        }
        Tcl_EventuallyFree(setPtr, PanesetFreeProc);
    } else if (eventPtr->type == ConfigureNotify) {
        setPtr->anchorPtr = LastPane(setPtr); /* Reset anchor pane. */
        setPtr->flags |= SCROLL_PENDING;
        EventuallyRedraw(setPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PaneEventProc --
 *
 *      This procedure is invoked by the Tk event handler when StructureNotify
 *      events occur in a widget managed by the paneset.
 *
 *      For example, when a managed widget is destroyed, it frees the
 *      corresponding pane structure and arranges for the paneset layout to be
 *      re-computed at the next idle point.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      If the managed widget was deleted, the Pane structure gets cleaned up
 *      and the paneset is rearranged.
 *
 *---------------------------------------------------------------------------
 */
static void
PaneEventProc(
    ClientData clientData,              /* Pointer to Pane structure for
                                         * widget referred to by eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    Pane *panePtr = (Pane *)clientData;
    Paneset *setPtr = panePtr->setPtr;

    if (eventPtr->type == ConfigureNotify) {
        int borderWidth;

        if (panePtr->tkwin == NULL) {
            return;
        }
        borderWidth = Tk_Changes(panePtr->tkwin)->border_width;
        if (panePtr->borderWidth != borderWidth) {
            panePtr->borderWidth = borderWidth;
            EventuallyRedraw(setPtr);
        }
    } else if (eventPtr->type == DestroyNotify) {
        if (panePtr->tkwin != NULL) {
            Tcl_EventuallyFree(panePtr, PaneFreeProc);
        }
        setPtr->flags |= LAYOUT_PENDING;
        EventuallyRedraw(setPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PaneCustodyProc --
 *
 *      This procedure is invoked when a widget has been stolen by another
 *      geometry manager.  The information and memory associated with the
 *      widget is released.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the paneset to have its layout recomputed at the next
 *      idle point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
PaneCustodyProc(ClientData clientData, Tk_Window tkwin)
{
    Pane *panePtr = (Pane *)clientData;
    Paneset *setPtr = panePtr->setPtr;

    if (Tk_IsMapped(panePtr->tkwin)) {
        Tk_UnmapWindow(panePtr->tkwin);
    }
    DestroyPane(panePtr);
    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * PaneGeometryProc --
 *
 *      This procedure is invoked by Tk_GeometryRequest for widgets managed by
 *      the paneset geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the paneset to have its layout re-computed and re-arranged
 *      at the next idle point.
 *
 * ----------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
PaneGeometryProc(ClientData clientData, Tk_Window tkwin)
{
    Pane *panePtr = (Pane *)clientData;

    panePtr->setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(panePtr->setPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * HandleEventProc --
 *
 *      This procedure is invoked by the Tk event handler when various events
 *      occur in the pane/drawer handle subwindow maintained by this widget.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
HandleEventProc(
    ClientData clientData,              /* Pointer to Pane structure for
                                         * handle referred to by eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    Pane *panePtr = (Pane *)clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedrawHandle(panePtr);
        }
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            if (eventPtr->type == FocusIn) {
                panePtr->flags |= FOCUS;
            } else {
                panePtr->flags &= ~FOCUS;
            }
            EventuallyRedrawHandle(panePtr);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        if (panePtr->handle == NULL) {
            return;
        }
        EventuallyRedrawHandle(panePtr);
    } else if (eventPtr->type == DestroyNotify) {
        panePtr->handle = NULL;
    } 
}

static INLINE Pane *
BeginPane(Paneset *setPtr)
{
    Blt_ChainLink link;

    link = Blt_Chain_FirstLink(setPtr->chain); 
    if (link != NULL) {
        return Blt_Chain_GetValue(link);
    }
    return NULL;
}

static INLINE Pane *
EndPane(Paneset *setPtr)
{
    Blt_ChainLink link;

    link = Blt_Chain_LastLink(setPtr->chain); 
    if (link != NULL) {
        return Blt_Chain_GetValue(link);
    }
    return NULL;
}

#ifdef notdef
static Pane *
StepPane(Pane *panePtr)
{
    if (panePtr != NULL) {
        Blt_ChainLink link;

        link = Blt_Chain_NextLink(panePtr->link); 
        if (link != NULL) {
            return Blt_Chain_GetValue(link);
        }
    }
    return NULL;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * NextTaggedPane --
 *
 *      Returns the next pane derived from the given tag.
 *
 * Results:
 *      Returns the pointer to the next pane in the iterator.  If no more panes
 *      are available, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Pane *
NextTaggedPane(PaneIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
    case ITER_ALL:
        if (iterPtr->link != NULL) {
            Pane *panePtr;
            
            panePtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return panePtr;
        }
        break;
    case ITER_PATTERN:
        {
            Blt_ChainLink link;
            
            for (link = iterPtr->link; link != NULL; 
                 link = Blt_Chain_NextLink(link)) {
                Pane *panePtr;
                
                panePtr = Blt_Chain_GetValue(iterPtr->link);
                if (Tcl_StringMatch(panePtr->name, iterPtr->tagName)) {
                    iterPtr->link = Blt_Chain_NextLink(link);
                    return panePtr;
                }
            }
            break;
        }
    default:
        break;
    }   
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * FirstTaggedPane --
 *
 *      Returns the first pane derived from the given tag.
 *
 * Results:
 *      Returns the first pane in the sequence.  If no more panes are in the
 *      list, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Pane *
FirstTaggedPane(PaneIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
    case ITER_ALL:
        if (iterPtr->link != NULL) {
            Pane *panePtr;
            
            panePtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return panePtr;
        }
        break;

    case ITER_PATTERN:
        {
            Blt_ChainLink link;
            
            for (link = iterPtr->link; link != NULL; 
                 link = Blt_Chain_NextLink(link)) {
                Pane *panePtr;
                
                panePtr = Blt_Chain_GetValue(iterPtr->link);
                if (Tcl_StringMatch(panePtr->name, iterPtr->tagName)) {
                    iterPtr->link = Blt_Chain_NextLink(link);
                    return panePtr;
                }
            }
            break;
        }
    case ITER_SINGLE:
        return iterPtr->startPtr;
    } 
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetPaneFromObj --
 *
 *      Gets the pane associated the given index, tag, or label.  This routine
 *      is used when you want only one pane.  It's an error if more than one
 *      pane is specified (e.g. "all" tag or range "1:4").  It's also an error
 *      if the tag is empty (no panes are currently tagged).
 *
 *---------------------------------------------------------------------------
 */
static int 
GetPaneFromObj(Tcl_Interp *interp, Paneset *setPtr, Tcl_Obj *objPtr,
              Pane **panePtrPtr)
{
    PaneIterator iter;
    Pane *firstPtr;

    if (GetPaneIterator(interp, setPtr, objPtr, &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    firstPtr = FirstTaggedPane(&iter);
    if (firstPtr != NULL) {
        Pane *nextPtr;

        nextPtr = NextTaggedPane(&iter);
        if (nextPtr != NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "multiple panes specified by \"", 
                        Tcl_GetString(objPtr), "\"", (char *)NULL);
            }
            return TCL_ERROR;
        }
    }
    *panePtrPtr = firstPtr;
    return TCL_OK;
}

static int
GetPaneByIndex(Tcl_Interp *interp, Paneset *setPtr, const char *string, 
              int length, Pane **panePtrPtr)
{
    Pane *panePtr;
    char c;
    long pos;

    panePtr = NULL;
    c = string[0];
    if (Blt_GetLong(NULL, string, &pos) == TCL_OK) {
        Blt_ChainLink link;

        link = Blt_Chain_GetNthLink(setPtr->chain, pos);
        if (link != NULL) {
            panePtr = Blt_Chain_GetValue(link);
        } 
        if (panePtr == NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "can't find pane: bad index \"", 
                        string, "\"", (char *)NULL);
            }
            return TCL_ERROR;
        }               
    } else if ((c == 'a') && (strcmp(string, "active") == 0)) {
        panePtr = setPtr->activePtr;
    } else if ((c == 'f') && (strcmp(string, "first") == 0)) {
        panePtr = FirstPane(setPtr);
    } else if ((c == 'l') && (strcmp(string, "last") == 0)) {
        panePtr = LastPane(setPtr);
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
        panePtr = LastPane(setPtr);
    } else if ((c == 'n') && (strcmp(string, "none") == 0)) {
        panePtr = NULL;
    } else {
        return TCL_CONTINUE;
    }
    *panePtrPtr = panePtr;
    return TCL_OK;
}

static Pane *
GetPaneByName(Paneset *setPtr, const char *string)
{
    Blt_HashEntry *hPtr;
    
    hPtr = Blt_FindHashEntry(&setPtr->paneTable, string);
    if (hPtr == NULL) {
        return NULL;
    }
    return Blt_GetHashValue(hPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * GetPaneIterator --
 *
 *      Converts a string representing a pane index into an pane pointer.  The
 *      index may be in one of the following forms:
 *
 *       number         Pane at index in the list of panes.
 *       @x,y           Pane closest to the specified X-Y screen coordinates.
 *       "active"       Pane where mouse pointer is located.
 *       "posted"       Pane is the currently posted cascade pane.
 *       "next"         Next pane from the focus pane.
 *       "previous"     Previous pane from the focus pane.
 *       "end"          Last pane.
 *       "none"         No pane.
 *
 *       number         Pane at position in the list of panes.
 *       @x,y           Pane closest to the specified X-Y screen coordinates.
 *       "active"       Pane mouse is located over.
 *       "focus"        Pane is the widget's focus.
 *       "select"       Currently selected pane.
 *       "right"        Next pane from the focus pane.
 *       "left"         Previous pane from the focus pane.
 *       "up"           Next pane from the focus pane.
 *       "down"         Previous pane from the focus pane.
 *       "end"          Last pane in list.
 *      "name:string"   Pane named "string".
 *      "index:number"  Pane at index number in list of panes.
 *      "tag:string"    Pane(s) tagged by "string".
 *      "label:pattern" Pane(s) with label matching "pattern".
 *      
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.  The
 *      pointer to the node is returned via panePtrPtr.  Otherwise, TCL_ERROR
 *      is returned and an error message is left in interpreter's result
 *      field.
 *
 *---------------------------------------------------------------------------
 */
static int
GetPaneIterator(Tcl_Interp *interp, Paneset *setPtr, Tcl_Obj *objPtr,
               PaneIterator *iterPtr)
{
    Pane *panePtr;
    Blt_Chain chain;
    char *string;
    char c;
    int numBytes;
    int length;
    int result;

    iterPtr->setPtr = setPtr;
    iterPtr->type = ITER_SINGLE;
    iterPtr->link = NULL;
    iterPtr->tagName = Tcl_GetStringFromObj(objPtr, &numBytes);
    iterPtr->nextPtr = NULL;
    iterPtr->startPtr = iterPtr->endPtr = NULL;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    iterPtr->startPtr = iterPtr->endPtr = setPtr->activePtr;
    iterPtr->type = ITER_SINGLE;
    result = GetPaneByIndex(interp, setPtr, string, length, &panePtr);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (result == TCL_OK) {
        iterPtr->startPtr = iterPtr->endPtr = panePtr;
        return TCL_OK;
    }
    if (c == '.') {
        Blt_HashEntry *hPtr;
        
        hPtr = Blt_FindHashEntry(&setPtr->handleTable, string);
        if (hPtr != NULL) {
            panePtr = Blt_GetHashValue(hPtr);
            iterPtr->startPtr = iterPtr->endPtr = panePtr;
            iterPtr->type = ITER_SINGLE;
            return TCL_OK;
        }
        return TCL_ERROR;
    } else if ((c == 'a') && (strcmp(iterPtr->tagName, "all") == 0)) {
        iterPtr->type  = ITER_ALL;
        iterPtr->link = Blt_Chain_FirstLink(setPtr->chain);
    } else if ((c == 'i') && (length > 6) && 
               (strncmp(string, "index:", 6) == 0)) {
        if (GetPaneByIndex(interp, setPtr, string + 6, length - 6, &panePtr) 
            != TCL_OK) {
            return TCL_ERROR;
        }
        iterPtr->startPtr = iterPtr->endPtr = panePtr;
    } else if ((c == 'n') && (length > 5) && 
               (strncmp(string, "name:", 5) == 0)) {
        panePtr = GetPaneByName(setPtr, string + 5);
        if (panePtr == NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "can't find a pane named \"", 
                        string + 5, "\" in \"", Tk_PathName(setPtr->tkwin), 
                        "\"", (char *)NULL);
            }
            return TCL_ERROR;
        }
        iterPtr->startPtr = iterPtr->endPtr = panePtr;
    } else if ((c == 't') && (length > 4) && 
               (strncmp(string, "tag:", 4) == 0)) {
        Blt_Chain chain;

        chain = Blt_Tags_GetItemList(&setPtr->tags, string + 4);
        if (chain == NULL) {
            return TCL_OK;
        }
        iterPtr->tagName = string + 4;
        iterPtr->link = Blt_Chain_FirstLink(chain);
        iterPtr->type = ITER_TAG;
    } else if ((c == 'l') && (length > 6) && 
               (strncmp(string, "label:", 6) == 0)) {
        iterPtr->link = Blt_Chain_FirstLink(setPtr->chain);
        iterPtr->tagName = string + 6;
        iterPtr->type = ITER_PATTERN;
    } else if ((panePtr = GetPaneByName(setPtr, string)) != NULL) {
        iterPtr->startPtr = iterPtr->endPtr = panePtr;
    } else if ((chain = Blt_Tags_GetItemList(&setPtr->tags, string)) != NULL) {
        iterPtr->tagName = string;
        iterPtr->link = Blt_Chain_FirstLink(chain);
        iterPtr->type = ITER_TAG;
    } else {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find pane index, name, or tag \"", 
                string, "\" in \"", Tk_PathName(setPtr->tkwin), "\"", 
                             (char *)NULL);
        }
        return TCL_ERROR;
    }   
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewPane --
 *
 *      This procedure creates and initializes a new Pane structure to hold
 *      a widget.  A valid widget has a parent widget that is either a) the
 *      container widget itself or b) a mutual ancestor of the container
 *      widget.
 *
 * Results:
 *      Returns a pointer to the new structure describing the new widget
 *      pane.  If an error occurred, then the return value is NULL and an
 *      error message is left in interp->result.
 *
 * Side effects:
 *      Memory is allocated and initialized for the Pane structure.
 *
 * ---------------------------------------------------------------------------- 
 */
static Pane *
NewPane(Tcl_Interp *interp, Paneset *setPtr, const char *name)
{
    Blt_HashEntry *hPtr;
    Pane *panePtr;
    int isNew;
    char *handleName;
    char string[200];

    {
        char *path;

        /* Generate an unique subwindow name.  In theory you could have
         * more than one drawer widget assigned to the same window.  */
        path = Blt_AssertMalloc(strlen(Tk_PathName(setPtr->tkwin)) + 200);
        do {
            sprintf(string, "%s%lu", setPtr->classPtr->objectName, 
                (unsigned long)setPtr->nextId++);
            sprintf(path, "%s.%s", Tk_PathName(setPtr->tkwin), string);
        } while (Tk_NameToWindow(interp, path, setPtr->tkwin) != NULL);
        Blt_Free(path);
        handleName = string;
    } 
    if (name == NULL) {
        name = handleName;
    }
    hPtr = Blt_CreateHashEntry(&setPtr->paneTable, name, &isNew);
    if (!isNew) {
        Tcl_AppendResult(interp, setPtr->classPtr->objectName, " \"", name, 
                "\" already exists.", (char *)NULL);
        return NULL;
    }
    panePtr = Blt_AssertCalloc(1, sizeof(Pane));
    Blt_ResetLimits(&panePtr->reqWidth);
    Blt_ResetLimits(&panePtr->reqHeight);
    Blt_ResetLimits(&panePtr->reqSize);
    panePtr->setPtr = setPtr;
    panePtr->name = Blt_GetHashKey(&setPtr->paneTable, hPtr);
    panePtr->hashPtr = hPtr;
    panePtr->anchor = TK_ANCHOR_CENTER;
    panePtr->fill = FILL_BOTH;
    panePtr->nom  = LIMITS_NOM;
    panePtr->size = panePtr->index = 0;
    panePtr->flags = VIRGIN | SHOW_HANDLE;
    panePtr->resize = RESIZE_BOTH;
    panePtr->side = HANDLE_FARSIDE;
    panePtr->weight = 1.0f;
    Blt_SetHashValue(hPtr, panePtr);

    panePtr->handle = Tk_CreateWindow(interp, setPtr->tkwin, handleName, 
                (char *)NULL);
    if (panePtr->handle == NULL) {
            return NULL;
    }
    Tk_CreateEventHandler(panePtr->handle, 
                          ExposureMask|FocusChangeMask|StructureNotifyMask, 
                          HandleEventProc, panePtr);
    Tk_SetClass(panePtr->handle, setPtr->classPtr->handleClassName);

    /* Also add drawer to handle table */
    hPtr = Blt_CreateHashEntry(&setPtr->handleTable, 
                Tk_PathName(panePtr->handle), &isNew);
    panePtr->handleHashPtr = hPtr;
    assert(isNew);
    Blt_SetHashValue(hPtr, panePtr);

    return panePtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * PaneFreeProc --
 *
 *      Removes the Pane structure from the hash table and frees the memory
 *      allocated by it.  
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the pane is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
PaneFreeProc(DestroyData dataPtr)
{
    Pane *panePtr = (Pane *)dataPtr;

    DestroyPane(panePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewPaneset --
 *
 *      This procedure creates and initializes a new Paneset structure with
 *      tkwin as its container widget. The internal structures associated with
 *      the paneset are initialized.
 *
 * Results:
 *      Returns the pointer to the new Paneset structure describing the new
 *      paneset geometry manager.  If an error occurred, the return value will
 *      be NULL and an error message is left in interp->result.
 *
 * Side effects:
 *      Memory is allocated and initialized, an event handler is set up to
 *      watch tkwin, etc.
 *
 *---------------------------------------------------------------------------
 */
static Paneset *
NewPaneset(Tcl_Interp *interp, Tcl_Obj *objPtr, int type)
{
    Paneset *setPtr;
    Tk_Window tkwin;

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), 
        Tcl_GetString(objPtr), (char *)NULL);
    if (tkwin == NULL) {
        return NULL;
    }
    setPtr = Blt_AssertCalloc(1, sizeof(Paneset));
    if (type == PANESET) {
        Tk_SetClass(tkwin, (char *)"BltPaneset");
        setPtr->classPtr = &panesetClass;
    } else if (type == FILMSTRIP) {
        Tk_SetClass(tkwin, (char *)"BltFilmstrip");
        setPtr->classPtr = &filmstripClass;
    }
    setPtr->tkwin = tkwin;
    setPtr->interp = interp;
    setPtr->display = Tk_Display(tkwin);
    setPtr->chain = Blt_Chain_Create();
    setPtr->handleThickness = 2;
    setPtr->handlePad.side1 = setPtr->handlePad.side2 = 2;
    setPtr->relief = TK_RELIEF_FLAT;
    setPtr->activeRelief = TK_RELIEF_RAISED;
    setPtr->handleBW = 1;
    setPtr->flags = LAYOUT_PENDING;
    setPtr->mode = MODE_GIVETAKE;
    setPtr->interval = 30;
    setPtr->scrollUnits = 10;
    setPtr->highlightThickness = 2;
    Blt_SetWindowInstanceData(tkwin, setPtr);
    Blt_InitHashTable(&setPtr->paneTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&setPtr->handleTable, BLT_STRING_KEYS);
    Blt_Tags_Init(&setPtr->tags);
    Tk_CreateEventHandler(tkwin, ExposureMask|StructureNotifyMask, 
                          PanesetEventProc, setPtr);
    setPtr->chain = Blt_Chain_Create();
    setPtr->cmdToken = Tcl_CreateObjCommand(interp, Tk_PathName(tkwin), 
        PanesetInstCmdProc, setPtr, PanesetInstCmdDeleteProc);
    setPtr->defVertCursor = Tk_GetCursor(interp, tkwin, DEF_VCURSOR);
    setPtr->defHorzCursor = Tk_GetCursor(interp, tkwin, DEF_HCURSOR);
    return setPtr;
}

static void
RenumberPanes(Paneset *setPtr)
{
    int count;
    Blt_ChainLink link;

    count = 0;
    for (link = Blt_Chain_FirstLink(setPtr->chain); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;
        
        panePtr = Blt_Chain_GetValue(link);
        panePtr->index = count;
        count++;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyPaneset --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the Paneset structure at a safe time (when no-one is using it
 *      anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the paneset geometry manager is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyPaneset(Paneset *setPtr)         /* Paneset structure */
{
    Blt_ChainLink link;

    Blt_FreeOptions(panesetSpecs, (char *)setPtr, setPtr->display, 0);
    /* Release the chain of entries. */
    for (link = Blt_Chain_FirstLink(setPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;

        panePtr = Blt_Chain_GetValue(link);
        panePtr->link = NULL;           /* Don't disrupt this chain of
                                         * entries. */
        panePtr->hashPtr = NULL;
        DestroyPane(panePtr);
    }
    Tk_FreeCursor(setPtr->display, setPtr->defHorzCursor);
    Tk_FreeCursor(setPtr->display, setPtr->defVertCursor);
    Blt_Tags_Reset(&setPtr->tags);
    Blt_Chain_Destroy(setPtr->chain);
    Blt_DeleteHashTable(&setPtr->paneTable);
    Blt_Free(setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * PanesetFreeProc --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the Paneset structure at a safe time (when no-one is using it
 *      anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the paneset geometry manager is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
PanesetFreeProc(DestroyData dataPtr)    /* Paneset structure */
{
    Paneset *setPtr = (Paneset *)dataPtr;

    DestroyPaneset(setPtr);
}


#ifdef notdef
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
    int dx, int dy,                     /* Difference between outer and inner
                                         * regions. */
    Tk_Anchor anchor,                   /* Direction of the anchor */
    int *xPtr, int *yPtr)
{
    int x, y;

    x = y = 0;
    switch (anchor) {
    case TK_ANCHOR_NW:                  /* Upper left corner */
        break;
    case TK_ANCHOR_W:                   /* Left center */
        y = (dy / 2);
        break;
    case TK_ANCHOR_SW:                  /* Lower left corner */
        y = dy;
        break;
    case TK_ANCHOR_N:                   /* Top center */
        x = (dx / 2);
        break;
    case TK_ANCHOR_CENTER:              /* Centered */
        x = (dx / 2);
        y = (dy / 2);
        break;
    case TK_ANCHOR_S:                   /* Bottom center */
        x = (dx / 2);
        y = dy;
        break;
    case TK_ANCHOR_NE:                  /* Upper right corner */
        x = dx;
        break;
    case TK_ANCHOR_E:                   /* Right center */
        x = dx;
        y = (dy / 2);
        break;
    case TK_ANCHOR_SE:                  /* Lower right corner */
        x = dx;
        y = dy;
        break;
    }
    *xPtr = (*xPtr) + x;
    *yPtr = (*yPtr) + y;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * LeftSpan --
 *
 *      Sums the space requirements of all the panes.
 *
 * Results:
 *      Returns the space currently used by the paneset widget.
 *
 *---------------------------------------------------------------------------
 */
static int
LeftSpan(Paneset *setPtr)
{
    int total;
    Pane *panePtr;

    total = 0;
    /* The left span is every pane before and including) the anchor pane. */
    for (panePtr = setPtr->anchorPtr; panePtr != NULL; 
         panePtr = PrevPane(panePtr)) {
        total += panePtr->size;
    }
    return total;
}


/*
 *---------------------------------------------------------------------------
 *
 * RightSpan --
 *
 *      Sums the space requirements of all the panes.
 *
 * Results:
 *      Returns the space currently used by the paneset widget.
 *
 *---------------------------------------------------------------------------
 */
static int
RightSpan(Paneset *setPtr)
{
    int total;
    Pane *panePtr;

    total = 0;
    for (panePtr = NextPane(setPtr->anchorPtr); panePtr != NULL; 
         panePtr = NextPane(panePtr)) {
        total += panePtr->size;
    }
    return total;
}


static int
LeftSpanLimits(Paneset *setPtr, int *minPtr, int *maxPtr)
{
    int total, min, max;
    Pane *panePtr;

    total = min = max = 0;
    /* The left span is every pane before and including) the anchor pane. */
    for (panePtr = setPtr->anchorPtr; panePtr != NULL; 
         panePtr = PrevPane(panePtr)) {
        total += panePtr->size;
        max += panePtr->max;
        min += panePtr->min;
    }
    *minPtr = min;
    *maxPtr = max;
    return total;
}


static int
RightSpanLimits(Paneset *setPtr, int *minPtr, int *maxPtr)
{
    int min, max, total;
    Pane *panePtr;

    total = min = max = 0;
    for (panePtr = NextPane(setPtr->anchorPtr); panePtr != NULL; 
         panePtr = NextPane(panePtr)) {
        total += panePtr->size;
        max += panePtr->max;
        min += panePtr->min;
    }
    *minPtr = min;
    *maxPtr = max;
    return total;
}


static int
GetReqPaneWidth(Pane *panePtr)
{
    int w;

    w = GetReqWidth(panePtr) + PADDING(panePtr->xPad); 
    if ((ISHORIZ(panePtr->setPtr)) && (panePtr->flags & HANDLE)) {
        w += panePtr->setPtr->handleSize;
    }
    return w;
}

static int
GetReqPaneHeight(Pane *panePtr)
{
    int h;

    h = GetReqHeight(panePtr) + PADDING(panePtr->yPad);
    if ((ISVERT(panePtr->setPtr)) && (panePtr->flags & HANDLE)) {
        h += panePtr->setPtr->handleSize;
    }
    return h;
}


/*
 *---------------------------------------------------------------------------
 *
 * GrowPane --
 *
 *      Expand the span by the amount of the extra space needed.  This
 *      procedure is used in Layout*Panes to grow the panes to their minimum
 *      nominal size, starting from a zero width and height space.
 *
 *      On the first pass we try to add space to panes which have not been
 *      touched yet (i.e. have no nominal size).
 *
 *      If there is still extra space after the first pass, this means that
 *      there were no panes could be expanded. This pass will try to remedy
 *      this by parcelling out the left over space evenly among the rest of
 *      the panes.
 *
 *      On each pass, we have to keep iterating over the list, evenly doling
 *      out slices of extra space, because we may hit pane limits as space is
 *      donated.  In addition, if there are left over pixels because of
 *      round-off, this will distribute them as evenly as possible.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The panes in the span may be expanded.
 *
 *---------------------------------------------------------------------------
 */
static void
GrowPane(Pane *panePtr, int extra)
{
    if ((panePtr->nom == LIMITS_NOM) && (panePtr->max > panePtr->size)) {
        int avail;

        avail = panePtr->max - panePtr->size;
        if (avail > extra) {
            panePtr->size += extra;
            return;
        } else {
            extra -= avail;
            panePtr->size += avail;
        }
    }
    /* Find out how many panes still have space available */
    if ((panePtr->resize & RESIZE_EXPAND) && (panePtr->max > panePtr->size)) {
        int avail;

        avail = panePtr->max - panePtr->size;
        if (avail > extra) {
            panePtr->size += extra;
            return;
        } else {
            extra -= avail;
            panePtr->size += avail;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GrowSpan --
 *
 *      Grow the span by the designated amount.  Size constraints on the panes
 *      may prevent any or all of the spacing adjustments.
 *
 *      This is very much like the GrowPane procedure, but in this case we are
 *      expanding all the panes. It uses a two pass approach, first giving
 *      space to panes which are smaller than their nominal sizes. This is
 *      because constraints on the panes may cause resizing to be non-linear.
 *
 *      If there is still extra space, this means that all panes are at least
 *      to their nominal sizes.  The second pass will try to add the left over
 *      space evenly among all the panes which still have space available
 *      (i.e. haven't reached their specified max sizes).
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The size of the pane may be increased.
 *
 *---------------------------------------------------------------------------
 */
static void
GrowSpan(Blt_Chain chain, int adjustment)       
{
    int delta;                          /* Amount of space needed */
    int numAdjust;                      /* Number of rows/columns that still can
                                         * be adjusted. */
    Blt_ChainLink link;
    float totalWeight;

    /*
     * Pass 1:  First adjust the size of panes that still haven't reached their
     *          nominal size.
     */
    delta = adjustment;

    numAdjust = 0;
    totalWeight = 0.0f;
    for (link = Blt_Chain_LastLink(chain); link != NULL; 
         link = Blt_Chain_PrevLink(link)) {
        Pane *panePtr;

        panePtr = Blt_Chain_GetValue(link);
        if ((panePtr->weight > 0.0f) && (panePtr->nom > panePtr->size)) {
            numAdjust++;
            totalWeight += panePtr->weight;
        }
    }

    while ((numAdjust > 0) && (totalWeight > 0.0f) && (delta > 0)) {
        Blt_ChainLink link;
        int ration;                     /* Amount of space to add to each
                                         * row/column. */
        ration = (int)(delta / totalWeight);
        if (ration == 0) {
            ration = 1;
        }
        for (link = Blt_Chain_LastLink(chain); (link != NULL) && (delta > 0);
            link = Blt_Chain_PrevLink(link)) {
            Pane *panePtr;

            panePtr = Blt_Chain_GetValue(link);
            if (panePtr->weight > 0.0f) {
                int avail;              /* Amount of space still available. */

                avail = panePtr->nom - panePtr->size;
                if (avail > 0) {
                    int size;           /* Amount of space requested for a
                                         * particular row/column. */
                    size = (int)(ration * panePtr->weight);
                    if (size > delta) {
                        size = delta;
                    }
                    if (size < avail) {
                        delta -= size;
                        panePtr->size += size;
                    } else {
                        delta -= avail;
                        panePtr->size += avail;
                        numAdjust--;
                        totalWeight -= panePtr->weight;
                    }
                }
            }
        }
    }

    /*
     * Pass 2: Adjust the panes with space still available
     */
    numAdjust = 0;
    totalWeight = 0.0f;
    for (link = Blt_Chain_LastLink(chain); link != NULL; 
         link = Blt_Chain_PrevLink(link)) {
        Pane *panePtr;

        panePtr = Blt_Chain_GetValue(link);
        if ((panePtr->weight > 0.0f) && (panePtr->max > panePtr->size)) {
            numAdjust++;
            totalWeight += panePtr->weight;
        }
    }
    while ((numAdjust > 0) && (totalWeight > 0.0f) && (delta > 0)) {
        Blt_ChainLink link;
        int ration;                     /* Amount of space to add to each
                                         * row/column. */

        ration = (int)(delta / totalWeight);
        if (ration == 0) {
            ration = 1;
        }
        for (link = Blt_Chain_LastLink(chain); (link != NULL) && (delta > 0);
            link = Blt_Chain_PrevLink(link)) {
            Pane *panePtr;

            panePtr = Blt_Chain_GetValue(link);
            if (panePtr->weight > 0.0f) {
                int avail;              /* Amount of space still available */

                avail = (panePtr->max - panePtr->size);
                if (avail > 0) {
                    int size;           /* Amount of space requested for a
                                         * particular row/column. */
                    size = (int)(ration * panePtr->weight);
                    if (size > delta) {
                        size = delta;
                    }
                    if (size < avail) {
                        delta -= size;
                        panePtr->size += size;
                    } else {
                        delta -= avail;
                        panePtr->size += avail;
                        numAdjust--;
                        totalWeight -= panePtr->weight;
                    }
                }
            }
        }
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * ShrinkSpan --
 *
 *      Shrink the span by the amount specified.  Size constraints on the
 *      panes may prevent any or all of the spacing adjustments.
 *
 *      This is very much like the GrowPane procedure, but in this case we are
 *      shrinking the panes. It uses a two pass approach, first subtracting
 *      space to panes which are larger than their nominal sizes. This is
 *      because constraints on the panes may cause resizing to be non-linear.
 *
 *      After pass 1, if there is still extra to be removed, this means that
 *      all panes are at least to their nominal sizes.  The second pass will
 *      try to remove the extra space evenly among all the panes which still
 *      have space available (i.e haven't reached their respective min sizes).
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The size of the panes may be decreased.
 *
 *---------------------------------------------------------------------------
 */
static void
ShrinkSpan(Blt_Chain chain, int adjustment)
{
    Blt_ChainLink link;
    int extra;                          /* Amount of space needed */
    int numAdjust;                      /* Number of panes that still can be
                                         * adjusted. */
    float totalWeight;

    extra = -adjustment;
    /*
     * Pass 1: First adjust the size of panes that still aren't at their
     *         nominal size.
     */
    
    numAdjust = 0;
    totalWeight = 0.0f;
    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;

        panePtr = Blt_Chain_GetValue(link);
        if ((panePtr->weight > 0.0f) && (panePtr->nom < panePtr->size)) {
            numAdjust++;
            totalWeight += panePtr->weight;
        }
    }

    while ((numAdjust > 0) && (totalWeight > 0.0f) && (extra > 0)) {
        Blt_ChainLink link;
        int ration;                     /* Amount of space to subtract from each
                                         * row/column. */
        ration = (int)(extra / totalWeight);
        if (ration == 0) {
            ration = 1;
        }
        for (link = Blt_Chain_FirstLink(chain); (link != NULL) && (extra > 0);
            link = Blt_Chain_NextLink(link)) {
            Pane *panePtr;

            panePtr = Blt_Chain_GetValue(link);
            if (panePtr->weight > 0.0f) {
                int avail;              /* Amount of space still available */

                avail = panePtr->size - panePtr->nom;
                if (avail > 0) {
                    int slice;          /* Amount of space requested for a
                                         * particular row/column. */
                    slice = (int)(ration * panePtr->weight);
                    if (slice > extra) {
                        slice = extra;
                    }
                    if (avail > slice) {
                        extra -= slice;
                        panePtr->size -= slice;  
                    } else {
                        extra -= avail;
                        panePtr->size -= avail;
                        numAdjust--;    /* Goes to zero (nominal). */
                        totalWeight -= panePtr->weight;
                    }
                }
            }
        }
    }
    /*
     * Pass 2: Now adjust the panes with space still available (i.e.
     *         are bigger than their minimum size).
     */
    numAdjust = 0;
    totalWeight = 0.0f;
    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;
        int avail;

        panePtr = Blt_Chain_GetValue(link);
        avail = panePtr->size - panePtr->min;
        if ((panePtr->weight > 0.0f) && (avail > 0)) {
            numAdjust++;
            totalWeight += panePtr->weight;
        }
    }
    while ((numAdjust > 0) && (totalWeight > 0.0f) && (extra > 0)) {
        Blt_ChainLink link;
        int ration;                     /* Amount of space to subtract from each
                                         * pane. */
        ration = (int)(extra / totalWeight);
        if (ration == 0) {
            ration = 1;
        }
        for (link = Blt_Chain_FirstLink(chain); (link != NULL) && (extra > 0);
            link = Blt_Chain_NextLink(link)) {
            Pane *panePtr;

            panePtr = Blt_Chain_GetValue(link);
            if (panePtr->weight > 0.0f) {
                int avail;              /* Amount of space still available */

                avail = panePtr->size - panePtr->min;
                if (avail > 0) {
                    int slice;          /* Amount of space requested for a
                                         * particular pane. */
                    slice = (int)(ration * panePtr->weight);
                    if (slice > extra) {
                        slice = extra;
                    }
                    if (avail > slice) {
                        extra -= slice;
                        panePtr->size -= slice;
                    } else {
                        extra -= avail;
                        panePtr->size -= avail;
                        numAdjust--;
                        totalWeight -= panePtr->weight;
                    }
                }
            }
        }
    }
}


/* |pos     anchor| */
static void 
ShrinkLeftGrowRight(Paneset *setPtr, Pane *leftPtr, Pane *rightPtr, int delta)
{
    int extra;
    Pane *panePtr;

    extra = delta;
    for (panePtr = leftPtr; (panePtr != NULL) && (extra > 0); 
         panePtr = PrevPane(panePtr)) {
        int avail;                      /* Space available to shrink */
        
        avail = panePtr->size - panePtr->min;
        if (avail > 0) {
            if (avail > extra) {
                panePtr->size -= extra;
                extra = 0;
            } else {
                panePtr->size -= avail;
                extra -= avail;
            }
        }
    }
    extra = delta - extra;
    for (panePtr = rightPtr; (panePtr != NULL) && (extra > 0); 
         panePtr = NextPane(panePtr)) {
        int avail;                      /* Space available to grow. */
        
        avail = panePtr->max - panePtr->size;
        if (avail > 0) {
            if (avail > extra) {
                panePtr->size += extra;
                extra = 0;
            } else {
                panePtr->size += avail;
                extra -= avail;
            }
        }
    }
}

/* |anchor     pos| */
static void 
GrowLeftShrinkRight(Paneset *setPtr, Pane *leftPtr, Pane *rightPtr, int delta)
{
    int extra;
    Pane *panePtr;

    extra = delta;
    for (panePtr = rightPtr; (panePtr != NULL) && (extra > 0); 
         panePtr = NextPane(panePtr)) {
        int avail;                      /* Space available to shrink */
        
        avail = panePtr->size - panePtr->min;
        if (avail > 0) {
            if (avail > extra) {
                panePtr->size -= extra;
                extra = 0;
            } else {
                panePtr->size -= avail;
                extra -= avail;
            }
        }
    }
    extra = delta - extra;
    for (panePtr = leftPtr; (panePtr != NULL) && (extra > 0); 
         panePtr = PrevPane(panePtr)) {
        int avail;                      /* Space available to grow. */
        
        avail = panePtr->max - panePtr->size;
        if (avail > 0) {
            if (avail > extra) {
                panePtr->size += extra;
                extra = 0;
            } else {
                panePtr->size += avail;
                extra -= avail;
            }
        }
    }
}


/* |pos     anchor| */
static void 
ShrinkLeftGrowLast(Paneset *setPtr, Pane *leftPtr, Pane *rightPtr, int delta)
{
    int extra;
    Pane *panePtr;

    extra = delta;
    for (panePtr = leftPtr; (panePtr != NULL) && (extra > 0); 
         panePtr = PrevPane(panePtr)) {
        int avail;                      /* Space available to shrink */
        
        avail = panePtr->size - panePtr->min;
        if (avail > 0) {
            if (avail > extra) {
                panePtr->size -= extra;
                extra = 0;
            } else {
                panePtr->size -= avail;
                extra -= avail;
            }
        }
    }
#ifdef notdef
    extra = delta - extra;
    for (panePtr = LastPane(setPtr);(panePtr != leftPtr) && (extra > 0); 
         panePtr = PrevPane(panePtr)) {
        int avail;                      /* Space available to grow. */
        
        avail = panePtr->max - panePtr->size;
        if (avail > 0) {
            if (avail > extra) {
                panePtr->size += extra;
                extra = 0;
            } else {
                panePtr->size += avail;
                extra -= avail;
            }
        }
    }
#endif
}

/* |anchor     pos| */
static void 
GrowLeftShrinkLast(Paneset *setPtr, Pane *leftPtr, Pane *rightPtr, int delta)
{
    int extra;
    Pane *panePtr;

    extra = delta;
#ifdef notdef
    for (panePtr = LastPane(setPtr);(panePtr != leftPtr) && (extra > 0); 
         panePtr = PrevPane(panePtr)) {
        int avail;                      /* Space available to shrink */
        
        avail = panePtr->size - panePtr->min;
        if (avail > 0) {
            if (avail > extra) {
                panePtr->size -= extra;
                extra = 0;
            } else {
                panePtr->size -= avail;
                extra -= avail;
            }
        }
    }
    extra = delta - extra;
#endif
    for (panePtr = leftPtr; (panePtr != NULL) && (extra > 0); 
         panePtr = PrevPane(panePtr)) {
        int avail;                      /* Space available to grow. */
        
        avail = panePtr->max - panePtr->size;
        if (avail > 0) {
            if (avail > extra) {
                panePtr->size += extra;
                extra = 0;
            } else {
                panePtr->size += avail;
                extra -= avail;
            }
        }
    }
}

static Blt_Chain 
SortedSpan(Paneset *setPtr, Pane *firstPtr, Pane *lastPtr)
{
    Blt_Chain chain;
    SizeProc *proc;
    Pane *panePtr;

    proc = ISVERT(setPtr) ? GetReqPaneHeight : GetReqPaneWidth;
    chain = Blt_Chain_Create();
    for (panePtr = firstPtr; panePtr != lastPtr; 
         panePtr = NextPane(panePtr)) {
        int d1;
        Blt_ChainLink link, before, newLink;

        d1 = (*proc)(panePtr) - panePtr->size;
        before = NULL;
        for (link = Blt_Chain_FirstLink(chain); link != NULL; 
             link = Blt_Chain_NextLink(link)) {
            Pane *pane2Ptr;
            int d2;

            pane2Ptr = Blt_Chain_GetValue(link);
            d2 = (*proc)(pane2Ptr) - pane2Ptr->size;
            if (d2 >= d1) {
                before = link;
                break;
            }
        }
        newLink = Blt_Chain_NewLink();
        Blt_Chain_SetValue(newLink, panePtr);
        if (before != NULL) {
            Blt_Chain_LinkBefore(chain, newLink, before);
        } else {
            Blt_Chain_LinkAfter(chain, newLink, NULL);
        }
    }
    return chain;
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetPanes --
 *
 *      Sets/resets the size of each pane to the minimum limit of the pane
 *      (this is usually zero). This routine gets called when new widgets are
 *      added, deleted, or resized.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The size of each pane is re-initialized to its minimum size.
 *
 *---------------------------------------------------------------------------
 */
static void
ResetPanes(Paneset *setPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(setPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;
        int extra, size;

        panePtr = Blt_Chain_GetValue(link);
        /*
         * The constraint procedure below also has the desired side-effect of
         * setting the minimum, maximum, and nominal values to the requested
         * size of its associated widget (if one exists).
         */
        if (ISVERT(setPtr)) {
            size = BoundHeight(0, &panePtr->reqSize);
            extra = PADDING(panePtr->yPad);
        } else {
            size = BoundWidth(0, &panePtr->reqSize);
            extra = PADDING(panePtr->xPad);
        }
        if (panePtr->flags & HANDLE) {
            extra += setPtr->handleSize;
        }
        if (panePtr->reqSize.flags & LIMITS_NOM_SET) {
            /*
             * This could be done more cleanly.  We want to ensure that the
             * requested nominal size is not overridden when determining the
             * normal sizes.  So temporarily fix min and max to the nominal
             * size and reset them back later.
             */
            panePtr->min = panePtr->max = panePtr->size = panePtr->nom = 
                size + extra;
        } else {
            /* The range defaults to 0..MAXINT */
            panePtr->min = panePtr->reqSize.min + extra;
            panePtr->max = panePtr->reqSize.max + extra;
            panePtr->nom = LIMITS_NOM;
            panePtr->size = size + extra;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SetNominalSizes
 *
 *      Sets the normal sizes for each pane.  The pane size is the requested
 *      widget size plus an amount of padding.  In addition, adjust the
 *      min/max bounds of the pane depending upon the resize flags (whether
 *      the pane can be expanded or shrunk from its normal size).
 *
 * Results:
 *      Returns the total space needed for the all the panes.
 *
 * Side Effects:
 *      The nominal size of each pane is set.  This is later used to determine
 *      how to shrink or grow the table if the container can't be resized to
 *      accommodate the exact size requirements of all the panes.
 *
 *---------------------------------------------------------------------------
 */
static int
SetNominalSizes(Paneset *setPtr)
{
    Blt_ChainLink link;
    int total;

    total = 0;
    for (link = Blt_Chain_FirstLink(setPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;
        int extra;

        panePtr = Blt_Chain_GetValue(link);
        if (ISVERT(setPtr)) {
            extra = PADDING(panePtr->yPad);
        } else {
            extra = PADDING(panePtr->xPad);
        }
        if (panePtr->flags & HANDLE) {
            extra += setPtr->handleSize;
        }
        /*
         * Restore the real bounds after temporarily setting nominal size.
         * These values may have been set in ResetPanes to restrict the size
         * of the pane to the requested range.
         */
        panePtr->min = panePtr->reqSize.min + extra;
        panePtr->max = panePtr->reqSize.max + extra;
        if (panePtr->size > panePtr->max) {
            panePtr->size = panePtr->max;
        } 
        if (panePtr->size < panePtr->min) {
            panePtr->size = panePtr->min;
        }
        panePtr->nom = panePtr->size;
        /*
         * If a pane can't be resized (to either expand or shrink), hold its
         * respective limit at its normal size.
         */
        if ((panePtr->resize & RESIZE_EXPAND) == 0) {
            panePtr->max = panePtr->nom;
        }
        if ((panePtr->resize & RESIZE_SHRINK) == 0) {
            panePtr->min = panePtr->nom;
        }
        total += panePtr->nom;
    }
    return total;
}

/*
 *---------------------------------------------------------------------------
 *
 * LayoutHorizontalPanes --
 *
 *      Calculates the normal space requirements for panes.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The sum of normal sizes set here will be used as the normal size for
 *      the container widget.
 *
 *---------------------------------------------------------------------------
 */
static void
LayoutHorizontalPanes(Paneset *setPtr)
{
    Blt_ChainLink link, next;
    int total;
    int maxHeight;
    int x, y;

    maxHeight = 0;
    ResetPanes(setPtr);
    for (link = Blt_Chain_FirstLink(setPtr->chain); link != NULL; link = next) {
        Pane *panePtr;
        int width, height;

        next = Blt_Chain_NextLink(link);
        panePtr = Blt_Chain_GetValue(link);
        panePtr->flags &= ~HANDLE;
        if (panePtr->flags & HIDDEN) {
            if (Tk_IsMapped(panePtr->tkwin)) {
                Tk_UnmapWindow(panePtr->tkwin);
            }
            if (Tk_IsMapped(panePtr->handle)) {
                Tk_UnmapWindow(panePtr->handle);
            }
            continue;
        }
        if ((next != NULL) || (setPtr->mode == MODE_SPREADSHEET)) {
            /* Add the size of the handle to the pane. */
            /* width += setPtr->handleSize; */
            if (panePtr->flags & SHOW_HANDLE) {
                panePtr->flags |= HANDLE;
            }
        }
        width = GetReqPaneWidth(panePtr);
        if (width <= 0) {
            /* continue; */
        }
        height = GetReqPaneHeight(panePtr);
        if (maxHeight < height) {
            maxHeight = height;
        }
        if (width > panePtr->size) {
            GrowPane(panePtr, width - panePtr->size);
        }
    }
    x = y = 0;
    for (link = Blt_Chain_FirstLink(setPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;

        panePtr = Blt_Chain_GetValue(link);
        panePtr->height = maxHeight;
        panePtr->width = panePtr->size;
        panePtr->x = x;
        panePtr->y = y;
        x += panePtr->size;
    }
    total = SetNominalSizes(setPtr);
    setPtr->worldWidth = total;
    setPtr->normalWidth = total + 2 * Tk_InternalBorderWidth(setPtr->tkwin);
    setPtr->normalHeight = maxHeight + 2*Tk_InternalBorderWidth(setPtr->tkwin);
    if (setPtr->normalWidth < 1) {
        setPtr->normalWidth = 1;
    }
    if (setPtr->normalHeight < 1) {
        setPtr->normalHeight = 1;
    }
    setPtr->flags &= ~LAYOUT_PENDING;
    setPtr->flags |= SCROLL_PENDING;
}

/*
 *---------------------------------------------------------------------------
 *
 * LayoutVerticalPanes --
 *
 *      Calculates the normal space requirements for panes.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The sum of normal sizes set here will be used as the normal size for
 *      the container widget.
 *
 *---------------------------------------------------------------------------
 */
static void
LayoutVerticalPanes(Paneset *setPtr)
{
    Blt_ChainLink link, next;
    int total;
    int maxWidth;
    int x, y;

    maxWidth = 0;
#if TRACE
    fprintf(stderr, "LayoutVerticalPanes\n");
#endif
    ResetPanes(setPtr);
    for (link = Blt_Chain_FirstLink(setPtr->chain); link != NULL; link = next) {
        Pane *panePtr;
        int width, height;
        
        next = Blt_Chain_NextLink(link);
        panePtr = Blt_Chain_GetValue(link);
        if (panePtr->flags & HIDDEN) {
            if (Tk_IsMapped(panePtr->tkwin)) {
                Tk_UnmapWindow(panePtr->tkwin);
            }
            if (Tk_IsMapped(panePtr->handle)) {
                Tk_UnmapWindow(panePtr->handle);
            }
            continue;
        }
        panePtr->flags &= ~HANDLE;
        if ((next != NULL) || (setPtr->mode == MODE_SPREADSHEET)) {
            /* height += setPtr->handleSize; */
            if (panePtr->flags & SHOW_HANDLE) {
                panePtr->flags |= HANDLE;
            }
        }
        height = GetReqPaneHeight(panePtr);
        if (height <= 0) {
            continue;
        }
        width = GetReqPaneWidth(panePtr);
        if (maxWidth < width) {
            maxWidth = width;
        }
        if (height > panePtr->size) {
            GrowPane(panePtr, height - panePtr->size);
        }
    }
    x = y = 0;
    for (link = Blt_Chain_FirstLink(setPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;

        panePtr = Blt_Chain_GetValue(link);
        panePtr->width = maxWidth;
        panePtr->height = panePtr->size;
        panePtr->x = x;
        panePtr->y = y;
        y += panePtr->size;
    }
    total = SetNominalSizes(setPtr);
    setPtr->worldWidth = total;
    setPtr->normalHeight = total + 2*Tk_InternalBorderWidth(setPtr->tkwin);
    setPtr->normalWidth = maxWidth + 2*Tk_InternalBorderWidth(setPtr->tkwin);
    if (setPtr->normalWidth < 1) {
        setPtr->normalWidth = 1;
    }
    if (setPtr->normalHeight < 1) {
        setPtr->normalHeight = 1;
    }
    setPtr->flags &= ~LAYOUT_PENDING;
    setPtr->flags |= SCROLL_PENDING;
}

static void
ArrangeWindow(Pane *panePtr, int x, int y) 
{
    Paneset *setPtr;
    int cavityWidth, cavityHeight;

    setPtr = panePtr->setPtr;
    if (ISVERT(setPtr)) {
        panePtr->height = panePtr->size;
        panePtr->width = Tk_Width(setPtr->tkwin);
    } else {
        panePtr->width = panePtr->size;
        panePtr->height = Tk_Height(setPtr->tkwin);
    }
    cavityWidth  = panePtr->width;
    cavityHeight = panePtr->height;
    if (panePtr->tkwin != NULL) {
        int w, h;
        int xMax, yMax;

        xMax = x + panePtr->width;
        yMax = y + panePtr->height;
        x += Tk_Changes(panePtr->tkwin)->border_width;
        y += Tk_Changes(panePtr->tkwin)->border_width;
        if (panePtr->flags & HANDLE) {
            if (ISVERT(setPtr)) {
                cavityHeight -= setPtr->handleSize;
                if (panePtr->side & HANDLE_FARSIDE) {
                    yMax -= setPtr->handleSize;
                } else {
                    y += setPtr->handleSize;
                }
            } else {
                cavityWidth -= setPtr->handleSize;
                if (panePtr->side & HANDLE_FARSIDE) {
                    xMax -= setPtr->handleSize;
                } else {
                    x += setPtr->handleSize;
                }
            }
        }
        
        /*
         * Unmap any widgets that start beyond of the right edge of the
         * container.
         */
        if ((x >= xMax) || (y >= yMax)) {
            if (Tk_IsMapped(panePtr->tkwin)) {
                Tk_UnmapWindow(panePtr->tkwin);
            }
            return;
        }
        w = GetReqWidth(panePtr);
        h = GetReqHeight(panePtr);
        
        /*
         *
         * Compare the widget's requested size to the size of the cavity.
         *
         * 1) If the widget is larger than the cavity or if the fill flag is
         * set, make the widget the size of the cavity. Check that the new size
         * is within the bounds set for the widget.
         *
         * 2) Otherwise, position the widget in the space according to its
         *    anchor.
         *
         */
        if ((cavityWidth <= w) || (panePtr->fill & FILL_X)) {
            w = cavityWidth;
        } 
        if (w > panePtr->reqWidth.max) {
            w = panePtr->reqWidth.max;
        }
        if ((cavityHeight <= h) || (panePtr->fill & FILL_Y)) {
            h = cavityHeight;
        }
        if (h > panePtr->reqHeight.max) {
            h = panePtr->reqHeight.max;
        }
        /*
         * Clip the widget at the bottom and/or right edge of the container.
         */
        if (h > (yMax - y)) {
            h = (yMax - y);
        }
        if (w > (xMax - x)) {
            w = (xMax - x);
        }
#ifdef notdef
        {
            int dx, dy;
            dx = dy = 0;
            if (cavityWidth > w) {
                dx = (cavityWidth - w);
            }
            if (cavityHeight > h) {
                dy = (cavityHeight - h);
            }
            if ((dx > 0) || (dy > 0)) {
                TranslateAnchor(dx, dy, panePtr->anchor, &x, &y);
            }
            TranslateAnchor(w, h, panePtr->anchor, &x, &y);
            fprintf(stderr, "pane=%s x=%d,y=%d, w=%d h=%d\n", 
                    panePtr->name, x, y, w, h);
        }
#endif
        /*
         * If the widget is too small (i.e. it has only an external border)
         * then unmap it.
         */
        if ((w < 1) || (h < 1)) {
            if (Tk_IsMapped(panePtr->tkwin)) {
                Tk_UnmapWindow(panePtr->tkwin);
            }
            return;
        }
        /*
         * Resize and/or move the widget as necessary.
         */
        if ((x != Tk_X(panePtr->tkwin)) || 
            (y != Tk_Y(panePtr->tkwin)) ||
            (w != Tk_Width(panePtr->tkwin)) || 
            (h != Tk_Height(panePtr->tkwin))) {
            Tk_MoveResizeWindow(panePtr->tkwin, x, y, w, h);
        }
        if (!Tk_IsMapped(panePtr->tkwin)) {
            Tk_MapWindow(panePtr->tkwin);
        }
    }
}

static void
ArrangeHandle(Pane *panePtr, int x, int y) 
{
    Paneset *setPtr;

    setPtr = panePtr->setPtr;
    if (panePtr->flags & HANDLE) {
        int w, h;
        
        if (ISVERT(setPtr)) {
            x = 0;
            if (panePtr->side & HANDLE_FARSIDE) {
                y += panePtr->size - setPtr->handleSize;
            }
            w = Tk_Width(setPtr->tkwin);
            h = setPtr->handleSize; 
        } else {
            y = 0;
            if (panePtr->side & HANDLE_FARSIDE) {
                x += panePtr->size - setPtr->handleSize;
            } 
            h = Tk_Height(setPtr->tkwin);
            w = setPtr->handleSize; 
        }
        if ((x != Tk_X(panePtr->tkwin)) || 
            (y != Tk_Y(panePtr->tkwin)) ||
            (w != Tk_Width(panePtr->tkwin)) ||
            (h != Tk_Height(panePtr->tkwin))) {
            Tk_MoveResizeWindow(panePtr->handle, x, y, w, h);
        }
        if (!Tk_IsMapped(panePtr->handle)) {
            Tk_MapWindow(panePtr->handle);
        }
        XRaiseWindow(setPtr->display, Tk_WindowId(panePtr->handle));
    } else if (Tk_IsMapped(panePtr->handle)) {
        Tk_UnmapWindow(panePtr->handle);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ArrangePane
 *
 *      Places each window at its proper location.  First determines the size
 *      and position of the each window.  It then considers the following:
 *
 *        1. translation of widget position its parent widget.
 *        2. fill style
 *        3. anchor
 *        4. external and internal padding
 *        5. widget size must be greater than zero
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The size of each pane is re-initialized its minimum size.
 *
 *---------------------------------------------------------------------------
 */
static void
ArrangePane(Pane *panePtr, int x, int y) 
{
    Paneset *setPtr;

    setPtr = panePtr->setPtr;
    if (ISVERT(setPtr)) {
        panePtr->height = panePtr->size;
        panePtr->width = Tk_Width(setPtr->tkwin);
    } else {
        panePtr->width = panePtr->size;
        panePtr->height = Tk_Height(setPtr->tkwin);
    }
    ArrangeWindow(panePtr, x, y);
    ArrangeHandle(panePtr, x, y);
}


/*
 *---------------------------------------------------------------------------
 *
 * VerticalPanes --
 *
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The widgets in the paneset are possibly resized and redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
VerticalPanes(Paneset *setPtr)
{
    int height;
    int top, bottom;
    int y;
    int yPad;
    Pane *panePtr;

    /*
     * If the paneset has no children anymore, then don't do anything at all:
     * just leave the container widget's size as-is.
     */
#if TRACE
    fprintf(stderr, "VerticalPanes\n");
#endif
    panePtr = LastPane(setPtr);
    if (panePtr == NULL) {
        Blt_Warn("VPanes: last pane is null\n");
        return;
    }
    if (setPtr->anchorPtr == NULL) {
        setPtr->anchorPtr = panePtr;
    }
    if (setPtr->anchorPtr == panePtr) {
        setPtr->bearing = Tk_Height(setPtr->tkwin);
#if TRACE
        fprintf(stderr, "VerticalPanes: bearing = %d\n", setPtr->bearing);
#endif
    }
    if (setPtr->flags & LAYOUT_PENDING) {
        LayoutVerticalPanes(setPtr);
    }
    /*
     * Save the width and height of the container so we know when its size has
     * changed during ConfigureNotify events.
     */
    top = LeftSpan(setPtr);
    bottom = RightSpan(setPtr);
    setPtr->worldWidth = height = top + bottom;

    yPad = 2 * Tk_InternalBorderWidth(setPtr->tkwin);
    /*
     * If the previous geometry request was not fulfilled (i.e. the size of
     * the container is different from paneset space requirements), try to
     * adjust size of the panes to fit the widget.
     */
    if (setPtr->classPtr->type == PANESET) {
        Pane *firstPtr, *lastPtr;
        Blt_Chain span;
        int dy;

        dy = setPtr->bearing - top;
        firstPtr = FirstPane(setPtr);
        lastPtr = NextPane(setPtr->anchorPtr);
        if (firstPtr != lastPtr) {
            span = SortedSpan(setPtr, firstPtr, lastPtr);
            if (dy > 0) {
                GrowSpan(span, dy);
            } else if (dy < 0) {
                ShrinkSpan(span, dy);
            }
            top = LeftSpan(setPtr) + yPad;
            Blt_Chain_Destroy(span);
        }
        dy = (Tk_Height(setPtr->tkwin) - setPtr->bearing) - bottom;
        span = SortedSpan(setPtr, lastPtr, NULL);
        if (dy > 0) {
            GrowSpan(span, dy);
        } else if (dy < 0) {
            ShrinkSpan(span, dy);
        }
        Blt_Chain_Destroy(span);
        bottom = RightSpan(setPtr) + yPad;
        setPtr->worldWidth = height = top + bottom;
    }

    /*
     * If after adjusting the size of the panes the space required does not
     * equal the size of the widget, do one of the following:
     *
     * 1) If it's smaller, center the paneset in the widget.
     * 2) If it's bigger, clip the panes that extend beyond the edge of the
     *    container.
     *
     * Set the row and column offsets (including the container's internal
     * border width). To be used later when positioning the widgets.
     */

#ifdef notdef
    if (height < setPtr->containerHeight) {
        y += (setPtr->containerHeight - height) / 2;
    }
#endif
    y = 0;
    for (panePtr = FirstPane(setPtr); panePtr != NULL; 
         panePtr = NextPane(panePtr)) {
        panePtr->y = y;
        ArrangePane(panePtr, 0, SCREEN(y));
        y += panePtr->size;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * HorizontalPanes --
 *
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The widgets in the paneset are possibly resized and redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
HorizontalPanes(Paneset *setPtr)
{
    int width;
    int left, right;
    int x;
    int xPad, yPad;
    Pane *panePtr;

    /*
     * If the paneset has no children anymore, then don't do anything at all:
     * just leave the container widget's size as-is.
     */
    panePtr = LastPane(setPtr);
    if (panePtr == NULL) {
        return;
    }
    if (setPtr->anchorPtr == NULL) {
        setPtr->anchorPtr = panePtr;
    }
    if (setPtr->anchorPtr == panePtr) {
        setPtr->bearing = Tk_Width(setPtr->tkwin);
#if TRACE
        fprintf(stderr, "HorizontalPanes: bearing = %d\n", setPtr->bearing);
#endif
    }
    if (setPtr->flags & LAYOUT_PENDING) {
        LayoutHorizontalPanes(setPtr);
    }
    /*
     * Save the width and height of the container so we know when its size has
     * changed during ConfigureNotify events.
     */
    xPad = yPad = 2 * Tk_InternalBorderWidth(setPtr->tkwin);

    left = LeftSpan(setPtr);
    right = RightSpan(setPtr);
    setPtr->worldWidth = width = left + right;
    
    /*
     * If the previous geometry request was not fulfilled (i.e. the size of
     * the paneset is different from the total panes space requirements), try
     * to adjust size of the panes to fit the widget.
     */
    if (setPtr->classPtr->type == PANESET) {
        Pane *firstPtr, *lastPtr;
        Blt_Chain span;
        int dx;

        dx = setPtr->bearing - left;
        firstPtr = FirstPane(setPtr);
        lastPtr = NextPane(setPtr->anchorPtr);
        if (firstPtr != lastPtr) {
            span = SortedSpan(setPtr, firstPtr, lastPtr);
            if (dx > 0) {
                GrowSpan(span, dx);
            } else if (dx < 0) {
                ShrinkSpan(span, dx);
            }
            left = LeftSpan(setPtr) + xPad;
            Blt_Chain_Destroy(span);
        }
        dx = (Tk_Width(setPtr->tkwin) - setPtr->bearing) - right;
        span = SortedSpan(setPtr, lastPtr, NULL);
        if (dx > 0) {
            GrowSpan(span, dx);
        } else if (dx < 0) {
            ShrinkSpan(span, dx);
        }
        Blt_Chain_Destroy(span);
        right = RightSpan(setPtr) + xPad;
        setPtr->worldWidth = width = left + right;
    }

    /*
     * If after adjusting the size of the panes the space required does not
     * equal the size of the widget, do one of the following:
     *
     * 1) If it's smaller, center the paneset in the widget.  
     * 2) If it's bigger, clip the panes that extend beyond the edge of the 
     *    container.
     *
     * Set the row and column offsets (including the container's internal
     * border width). To be used later when positioning the widgets.
     */

    x = 0;
    for (panePtr = FirstPane(setPtr); panePtr != NULL; 
         panePtr = NextPane(panePtr)) {
        panePtr->x = x;
        ArrangePane(panePtr, SCREEN(x), 0);
        x += panePtr->size;
    }
}

static void
ComputeGeometry(Paneset *setPtr) 
{
    if (ISVERT(setPtr)) {
        LayoutVerticalPanes(setPtr);
    } else {
        LayoutHorizontalPanes(setPtr);
    }
    setPtr->flags &= ~LAYOUT_PENDING;
    /* Override computed layout with requested width/height */
    if (setPtr->reqWidth > 0) {
        setPtr->normalWidth = setPtr->reqWidth;
    }
    if (setPtr->reqHeight > 0) {
        setPtr->normalHeight = setPtr->reqHeight;
    }
    if ((setPtr->normalWidth != Tk_ReqWidth(setPtr->tkwin)) ||
        (setPtr->normalHeight != Tk_ReqHeight(setPtr->tkwin))) {
        Tk_GeometryRequest(setPtr->tkwin, setPtr->normalWidth,
                           setPtr->normalHeight);
    }
}

static void
ConfigurePaneset(Paneset *setPtr) 
{
    setPtr->handleSize = MAX(PADDING(setPtr->handlePad),setPtr->highlightThickness) + setPtr->handleThickness;
}



static int
AdjustPanesetDelta(Paneset *setPtr, int delta)
{
    int total;
    int lmin, lmax, rmin, rmax;
    int oldBearing;
    Pane *panePtr;

    total = (ISVERT(setPtr)) 
        ? Tk_Height(setPtr->tkwin) : Tk_Width(setPtr->tkwin);
    
    LeftSpanLimits(setPtr, &lmin, &lmax);
    RightSpanLimits(setPtr, &rmin, &rmax);
    
    rmin = total - rmin;
    rmax = total - rmax;
    oldBearing = setPtr->bearing;
    setPtr->bearing += delta;
    if (setPtr->bearing < lmin) {
        setPtr->bearing = lmin;
    } else if (setPtr->bearing > lmax) {
        setPtr->bearing = lmax;
    }
    if (setPtr->bearing >= rmin) {
        setPtr->bearing = rmin;
    } else if (setPtr->bearing < rmax) {
        setPtr->bearing = rmax;
    }
    for (panePtr = FirstPane(setPtr); panePtr != NULL; 
         panePtr = NextPane(panePtr)) {
        panePtr->nom = panePtr->size;
    }
    return setPtr->bearing - oldBearing;
}

static void
AdjustHandle(Paneset *setPtr, int delta)
{
    Pane *panePtr;

    if (setPtr->classPtr->type != FILMSTRIP) {
        delta = AdjustPanesetDelta(setPtr, delta);
        for (panePtr = FirstPane(setPtr); panePtr != NULL; 
             panePtr = NextPane(panePtr)) {
            panePtr->nom = panePtr->size;
        }
    }
    if (setPtr->classPtr->type == FILMSTRIP) {
        setPtr->scrollOffset -= delta;
        setPtr->flags |= SCROLL_PENDING;
    } else {
        switch (setPtr->mode) {
        case MODE_GIVETAKE:
            {
                Pane *leftPtr, *rightPtr;
                
                leftPtr = setPtr->anchorPtr;
                rightPtr = NextPane(leftPtr);
                if (delta > 0) {
                    GrowLeftShrinkRight(setPtr, leftPtr, rightPtr, delta);
                } else if (delta < 0) {
                    ShrinkLeftGrowRight(setPtr, leftPtr, rightPtr, -delta);
                }
            } 
            break;
        case MODE_SPREADSHEET:
            {
                Pane *leftPtr, *rightPtr;
                
                leftPtr = setPtr->anchorPtr;
                rightPtr = NextPane(leftPtr);
                if (delta > 0) {
                    GrowLeftShrinkLast(setPtr, leftPtr, rightPtr, delta);
                } else if (delta < 0) {
                    ShrinkLeftGrowLast(setPtr, leftPtr, rightPtr, -delta);
                }
            } 
            break;
        case MODE_SLINKY:
            break;
        }
        for (panePtr = FirstPane(setPtr); panePtr != NULL; 
             panePtr = NextPane(panePtr)) {
            panePtr->nom = panePtr->size;
        }
    }
    EventuallyRedraw(setPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * AddOp --
 *
 *      Appends a pane into the widget.
 *
 * Results:
 *      Returns a standard TCL result.  The index of the pane is left in
 *      interp->result.
 *
 *      .p add ?name? ?option value...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AddOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{ 
    Paneset *setPtr = clientData;
    Pane *panePtr;
    const char *name;

    name = NULL;
    if (objc > 2) {
        const char *string;

        string = Tcl_GetString(objv[2]);
        if (string[0] != '-') {
            if (GetPaneFromObj(NULL, setPtr, objv[2], &panePtr) == TCL_OK) {
                Tcl_AppendResult(interp, "pane \"", string, 
                        "\" already exists", (char *)NULL);
                return TCL_ERROR;
            }
            name = string;
            objc--, objv++;
        }
    }
    panePtr = NewPane(interp, setPtr, name);
    if (panePtr == NULL) {
        return TCL_ERROR;
    }
    if (Blt_ConfigureWidgetFromObj(interp, panePtr->handle, 
        setPtr->classPtr->specs, objc - 2, objv + 2, (char *)panePtr, 0) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    panePtr->link = Blt_Chain_Append(setPtr->chain, panePtr);
    RenumberPanes(setPtr);
    EventuallyRedraw(setPtr);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), panePtr->index);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *      Returns the name, position and options of a widget in the paneset.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the widget attributes is
 *      left in interp->result.
 *
 *      .p cget option
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, setPtr->tkwin, panesetSpecs, 
        (char *)setPtr, objv[2], setPtr->classPtr->type);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      Returns the name, position and options of a widget in the paneset.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the paneset configuration
 *      option information is left in interp->result.
 *
 *      .p configure option value
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;

    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, setPtr->tkwin, panesetSpecs, 
                (char *)setPtr, (Tcl_Obj *)NULL, setPtr->classPtr->type);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, setPtr->tkwin, panesetSpecs, 
                (char *)setPtr, objv[2], setPtr->classPtr->type);
    }
    if (Blt_ConfigureWidgetFromObj(interp, setPtr->tkwin, panesetSpecs,
        objc - 2, objv + 2, (char *)setPtr, 
        BLT_CONFIG_OBJV_ONLY|setPtr->classPtr->type) != TCL_OK) {
        return TCL_ERROR;
    }
    ConfigurePaneset(setPtr);
    /* Arrange for the paneset layout to be computed at the next idle point. */
    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes the specified panes from the widget.  Note that the pane
 *      indices can be fixed only after all the deletions have occurred.
 *
 *      .p delete widget
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    PaneIterator iter;
    Pane *panePtr;

    if (GetPaneIterator(interp, setPtr, objv[2], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (panePtr = FirstTaggedPane(&iter); panePtr != NULL; 
         panePtr = NextTaggedPane(&iter)) {
        Tcl_EventuallyFree(panePtr, PaneFreeProc);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *      Indicates if given pane exists.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      widget index pane
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr;
    int state;

    state = FALSE;
    if (GetPaneFromObj(NULL, setPtr, objv[2], &panePtr) == TCL_OK) {
        if (panePtr != NULL) {
            state = TRUE;
        }
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * HandleActivateOp --
 *
 *      Changes the cursor and schedules to redraw the handle in its
 *      activate state (different relief, colors, etc).
 *
 *      pathName handle activate drawer 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    Pane *panePtr;
    Paneset *setPtr = clientData;

    if (GetPaneFromObj(interp, setPtr, objv[3], &panePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (panePtr->flags & (DISABLED|HIDDEN)) {
        return TCL_OK;
    }
    if (panePtr != setPtr->activePtr) {
        Tk_Cursor cursor;
        int vert;

        if (setPtr->activePtr != NULL) {
            EventuallyRedrawHandle(setPtr->activePtr);
        }
        if (panePtr != NULL) {
            EventuallyRedrawHandle(panePtr);
        }
        setPtr->activePtr = panePtr;
        vert = ISVERT(setPtr);
        if (panePtr->cursor != None) {
            cursor = panePtr->cursor;
        } else if (vert) {
            cursor = setPtr->defVertCursor;
        } else {
            cursor = setPtr->defHorzCursor;
        }
        Tk_DefineCursor(panePtr->handle, cursor);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HandleAnchorOp --
 *
 *      Set the anchor for the resize/moving the pane/drawer.  Only one of
 *      the x and y coordinates are used depending upon the orientation of
 *      the pane.
 *
 *      pathName handle anchor drawer x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Pane *panePtr;
    Paneset *setPtr = clientData;
    int x, y;
    int vert;

    if (GetPaneFromObj(interp, setPtr, objv[3], &panePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (panePtr->flags & (DISABLED|HIDDEN)) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    } 
    setPtr = panePtr->setPtr;
    setPtr->anchorPtr = setPtr->activePtr = panePtr;
    setPtr->flags |= HANDLE_ACTIVE;
    vert = ISVERT(setPtr);
    if (vert) {
        setPtr->bearing = ScreenY(panePtr);
        setPtr->handleAnchor = y;
    } else {
        setPtr->bearing = ScreenX(panePtr);
        setPtr->handleAnchor = x;
    } 
    setPtr->bearing += panePtr->size;
    AdjustHandle(setPtr, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HandleDeactivateOp --
 *
 *      Changes the cursor and schedules to redraw the handle in its
 *      inactivate state (different relief, colors, etc).
 *
 *      pathName handle deactivate 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;

    if (setPtr->activePtr != NULL) {
        EventuallyRedrawHandle(setPtr->activePtr);
        setPtr->activePtr = NULL;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HandleMarkOp --
 *
 *      Sets the current mark for moving the handle.  The change between
 *      the mark and the anchor is the amount to move the handle from its
 *      previous location.
 *
 *      pathName handle mark drawer x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleMarkOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Pane *panePtr;
    Paneset *setPtr = clientData;
    int x, y;                           /* Root coordinates of the pointer
                                         * over the handle. */
    int delta, mark, vert;

    if (GetPaneFromObj(interp, setPtr, objv[3], &panePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (panePtr->flags & (DISABLED|HIDDEN)) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    } 
    setPtr = panePtr->setPtr;
    setPtr->anchorPtr = panePtr;
    vert = ISVERT(setPtr);
    mark = (vert) ? y : x;
    delta = mark - setPtr->handleAnchor;
    AdjustHandle(setPtr, delta);
    setPtr->handleAnchor = mark;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HandleMoveOp --
 *
 *      Moves the handle.  The handle is moved the given distance from its
 *      previous location.
 *
 *      pathName handle move drawer x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleMoveOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Pane *panePtr;
    Paneset *setPtr = clientData;
    int delta, x, y;
    int vert;

    if (GetPaneFromObj(interp, setPtr, objv[3], &panePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (panePtr->flags & (DISABLED|HIDDEN)) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    } 
    setPtr = panePtr->setPtr;
    setPtr->anchorPtr = panePtr;
    vert = ISVERT(setPtr);
    if (vert) {
        delta = y;
        setPtr->bearing = ScreenY(panePtr);
    } else {
        delta = x;
        setPtr->bearing = ScreenX(panePtr);
    }
    setPtr->bearing += panePtr->size;
    AdjustHandle(setPtr, delta);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HandleSetOp --
 *
 *      Sets the location of the handle to coordinate (x or y) specified.
 *      The windows are move and/or arranged according to the mode.
 *
 *      pathName handle set drawer $x $y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Pane *panePtr;
    Paneset *setPtr = clientData;
    int x, y;
    int delta, mark, vert;

    if (GetPaneFromObj(interp, setPtr, objv[3], &panePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (panePtr->flags & (DISABLED|HIDDEN)) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    } 
    setPtr = panePtr->setPtr;
    setPtr->flags &= ~HANDLE_ACTIVE;
    vert = ISVERT(setPtr);
    mark = (vert) ? y : x;
    delta = mark - setPtr->handleAnchor;
    AdjustHandle(setPtr, delta);
    setPtr->handleAnchor = mark;
    return TCL_OK;
}

static Blt_OpSpec handleOps[] =
{
    {"activate",   2, HandleActivateOp,   4, 4, "drawer"},
    {"anchor",     2, HandleAnchorOp,     6, 6, "drawer x y"},
    {"deactivate", 1, HandleDeactivateOp, 3, 3, ""},
    {"mark",       2, HandleMarkOp,       6, 6, "drawer x y"},
    {"move",       2, HandleMoveOp,       6, 6, "drawer x y"},
    {"set",        1, HandleSetOp,        6, 6, "drawer x y"},
};

static int numHandleOps = sizeof(handleOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * HandleOp --
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
HandleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numHandleOps, handleOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * IndexOp --
 *
 *      Returns the index of the given pane.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      widget index pane
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr;
    int index;

    index = -1;
    if (GetPaneFromObj(NULL, setPtr, objv[2], &panePtr) == TCL_OK) {
        if (panePtr != NULL) {
            index = panePtr->index;
        }
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InsertOp --
 *
 *      Add new entries into a pane set.
 *
 *      .t insert position ?label? option-value label option-value...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InsertOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr;
    Blt_ChainLink link, before;
    char c;
    const char *string;
    const char *name;

    string = Tcl_GetString(objv[2]);
    c = string[0];
    if ((c == 'e') && (strcmp(string, "end") == 0)) {
        before = NULL;
    } else if (isdigit(UCHAR(c))) {
        int pos;

        if (Tcl_GetIntFromObj(interp, objv[2], &pos) != TCL_OK) {
            return TCL_ERROR;
        }
        if (pos < 0) {
            before = Blt_Chain_FirstLink(setPtr->chain);
        } else if (pos > Blt_Chain_GetLength(setPtr->chain)) {
            before = NULL;
        } else {
            before = Blt_Chain_GetNthLink(setPtr->chain, pos);
        }
    } else {
        Pane *beforePtr;

        if (GetPaneFromObj(interp, setPtr, objv[2], &beforePtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (beforePtr == NULL) {
            Tcl_AppendResult(interp, "can't find a pane \"", 
                Tcl_GetString(objv[2]), "\" in \"", Tk_PathName(setPtr->tkwin), 
                "\"", (char *)NULL);
            return TCL_ERROR;
        }
        before = beforePtr->link;
    }
    name = NULL;
    if (objc > 3) {
        string = Tcl_GetString(objv[3]);
        if (string[0] != '-') {
            name = string;
            objc--, objv++;
        }
    }
    panePtr = NewPane(interp, setPtr, name);
    if (panePtr == NULL) {
        return TCL_ERROR;
    }
    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
    if (Blt_ConfigureWidgetFromObj(interp, panePtr->handle, 
        setPtr->classPtr->specs, objc - 3, objv + 3, (char *)panePtr, 0) 
        != TCL_OK) {
        DestroyPane(panePtr);
        return TCL_ERROR;
    }
    link = Blt_Chain_NewLink();
    if (before != NULL) {
        Blt_Chain_LinkBefore(setPtr->chain, link, before);
    } else {
        Blt_Chain_AppendLink(setPtr->chain, link);
    }
    panePtr->link = link;
    Blt_Chain_SetValue(link, panePtr);
    RenumberPanes(setPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), panePtr->name, -1);
    return TCL_OK;

}

/*
 *---------------------------------------------------------------------------
 *
 * InvokeOp --
 *
 *      This procedure is called to invoke a selection command.
 *
 *        .ps invoke pane
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *      contains an error message.
 *
 * Side Effects:
 *      Configuration information, such as text string, colors, font, etc. get
 *      set; old resources get freed, if there were any.  The widget is
 *      redisplayed if needed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InvokeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr;
    Tcl_Obj *cmdObjPtr;

    if (GetPaneFromObj(interp, setPtr, objv[2], &panePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((panePtr == NULL) || (panePtr->flags & (DISABLED|HIDDEN))) {
        return TCL_OK;
    }
    cmdObjPtr = GETATTR(panePtr, cmdObjPtr);
    if (cmdObjPtr != NULL) {
        Tcl_Obj *objPtr;
        int result;

        cmdObjPtr = Tcl_DuplicateObj(cmdObjPtr);
        objPtr = Tcl_NewIntObj(panePtr->index);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        Tcl_IncrRefCount(cmdObjPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MoveOp --
 *
 *      Moves a pane to a new location.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MoveOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr, *fromPtr;
    char c;
    const char *string;
    int isBefore;
    int length;

    if (GetPaneFromObj(interp, setPtr, objv[2], &panePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((panePtr == NULL) || (panePtr->flags & DISABLED)) {
        return TCL_OK;
    }
    string = Tcl_GetStringFromObj(objv[3], &length);
    c = string[0];
    if ((c == 'b') && (strncmp(string, "before", length) == 0)) {
        isBefore = TRUE;
    } else if ((c == 'a') && (strncmp(string, "after", length) == 0)) {
        isBefore = FALSE;
    } else {
        Tcl_AppendResult(interp, "bad key word \"", string,
            "\": should be \"after\" or \"before\"", (char *)NULL);
        return TCL_ERROR;
    }
    if (GetPaneFromObj(interp, setPtr, objv[4], &fromPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (fromPtr == NULL) {
        Tcl_AppendResult(interp, "can't find a pane \"", 
                Tcl_GetString(objv[4]), "\" in \"", Tk_PathName(setPtr->tkwin), 
                "\"", (char *)NULL);
        return TCL_ERROR;
    }
    if (panePtr == fromPtr) {
        return TCL_OK;
    }
    Blt_Chain_UnlinkLink(setPtr->chain, panePtr->link);
    if (isBefore) {
        Blt_Chain_LinkBefore(setPtr->chain, panePtr->link, fromPtr->link);
    } else {
        Blt_Chain_LinkAfter(setPtr->chain, panePtr->link, fromPtr->link);
    }
    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *        .ps names pattern
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)   
{
    Paneset *setPtr = clientData;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 2) {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(setPtr->chain); link != NULL;
            link = Blt_Chain_NextLink(link)) {
            Pane *panePtr;

            panePtr = Blt_Chain_GetValue(link);
            Tcl_ListObjAppendElement(interp, listObjPtr, 
                        Tcl_NewStringObj(panePtr->name, -1));
        }
    } else {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(setPtr->chain); link != NULL;
             link = Blt_Chain_NextLink(link)) {
            Pane *panePtr;
            int i;

            panePtr = Blt_Chain_GetValue(link);
            for (i = 2; i < objc; i++) {
                if (Tcl_StringMatch(panePtr->name, Tcl_GetString(objv[i]))) {
                    Tcl_ListObjAppendElement(interp, listObjPtr, 
                                Tcl_NewStringObj(panePtr->name, -1));
                    break;
                }
            }
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PaneCgetOp --
 *
 *      Returns the name, position and options of a widget in the paneset.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the widget attributes is
 *      left in interp->result.
 *
 *      .p pane cget pane option
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PaneCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr;

    if (GetPaneFromObj(interp, setPtr, objv[3], &panePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, setPtr->tkwin, 
        setPtr->classPtr->specs, (char *)panePtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * PaneConfigureOp --
 *
 *      Returns the name, position and options of a widget in the paneset.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the paneset configuration
 *      option information is left in interp->result.
 *
 *      .p pane configure pane option value
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PaneConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr;
    PaneIterator iter;

    if (objc == 4) {
        if (GetPaneFromObj(interp, setPtr, objv[3], &panePtr) != TCL_OK) {
            return TCL_ERROR;
        }
        return Blt_ConfigureInfoFromObj(interp, panePtr->handle, 
                setPtr->classPtr->specs, (char *)panePtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 5) {
        if (GetPaneFromObj(interp, setPtr, objv[3], &panePtr) != TCL_OK) {
            return TCL_ERROR;
        }
        return Blt_ConfigureInfoFromObj(interp, panePtr->handle, 
                setPtr->classPtr->specs, (char *)panePtr, objv[4], 0);
    }
    if (GetPaneIterator(interp, setPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (panePtr = FirstTaggedPane(&iter); panePtr != NULL; 
         panePtr = NextTaggedPane(&iter)) {
        if (Blt_ConfigureWidgetFromObj(interp, panePtr->handle, 
                setPtr->classPtr->specs, objc - 4, objv + 4, (char *)panePtr, 
                BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    setPtr->anchorPtr = NULL;
    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

static Blt_OpSpec paneOps[] =
{
    {"cget",       2, PaneCgetOp,      5, 5, "pane option",},
    {"configure",  2, PaneConfigureOp, 4, 0, "pane ?option value?",},
};

static int numPaneOps = sizeof(paneOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * PaneOp --
 *
 *      This procedure is invoked to process the TCL command that corresponds
 *      to the paneset geometry manager.  See the user documentation for
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
PaneOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numPaneOps, paneOps, BLT_OP_ARG2, 
                            objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}


/*
 *---------------------------------------------------------------------------
 *
 * MotionTimerProc --
 *
 *---------------------------------------------------------------------------
 */
static void
MotionTimerProc(ClientData clientData)
{
    Paneset *setPtr = clientData;

    if (setPtr->scrollTarget > setPtr->scrollOffset) {
        setPtr->scrollOffset += setPtr->scrollIncr;
        if (setPtr->scrollOffset > setPtr->scrollTarget) {
            setPtr->scrollOffset = setPtr->scrollTarget;
        } 
    } else if (setPtr->scrollTarget < setPtr->scrollOffset) {
        setPtr->scrollOffset -= setPtr->scrollIncr;
        if (setPtr->scrollOffset < setPtr->scrollTarget) {
            setPtr->scrollOffset = setPtr->scrollTarget;
        } 
    }
    setPtr->scrollIncr += setPtr->scrollIncr;
    if (setPtr->scrollTarget == setPtr->scrollOffset) {
        if (setPtr->timerToken != (Tcl_TimerToken)0) {
            Tcl_DeleteTimerHandler(setPtr->timerToken);
            setPtr->timerToken = 0;
            setPtr->scrollIncr = setPtr->scrollUnits;
        }
    } else {
        setPtr->timerToken = Tcl_CreateTimerHandler(setPtr->interval, 
                MotionTimerProc, setPtr);
    }
    setPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(setPtr);
}


/*ARGSUSED*/
static int
SeeOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr;
    int left, right, width, margin;

    if (GetPaneFromObj(interp, setPtr, objv[2], &panePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((panePtr == NULL) || (panePtr->flags & (HIDDEN|DISABLED))) {
        return TCL_OK;
    }

    width = VPORTWIDTH(setPtr);
    left = setPtr->scrollOffset;
    right = setPtr->scrollOffset + width;
    margin = 20;
    
    /* If the pane is partially obscured, scroll so that it's entirely in
     * view. */
    if (panePtr->x < left) {
        setPtr->scrollTarget = panePtr->x - (width - panePtr->size)/2;
        if ((panePtr->size + margin) < width) {
            setPtr->scrollTarget -= margin;
        }
    } else if ((panePtr->x + panePtr->size) >= right) {
        setPtr->scrollTarget = panePtr->x - (width - panePtr->size)/2;
        if ((panePtr->size + margin) < width) {
            setPtr->scrollTarget += margin;
        }
    }
    if (setPtr->flags & ANIMATE) {
        setPtr->scrollIncr = setPtr->scrollUnits;
        setPtr->timerToken = Tcl_CreateTimerHandler(setPtr->interval, 
                MotionTimerProc, setPtr);
    } else {
        setPtr->scrollOffset = setPtr->scrollTarget;
    }
    setPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(setPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * TagAddOp --
 *
 *      .t tag add tagName pane1 pane2 pane2 pane4
 *
 *---------------------------------------------------------------------------
 */
static int
TagAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    const char *tag;
    long paneId;

    tag = Tcl_GetString(objv[3]);
    if (Blt_GetLongFromObj(NULL, objv[3], &paneId) == TCL_OK) {
        Tcl_AppendResult(interp, "bad tag \"", tag, 
                 "\": can't be a number.", (char *)NULL);
        return TCL_ERROR;
    }
    if (strcmp(tag, "all") == 0) {
        Tcl_AppendResult(interp, "can't add reserved tag \"", tag, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        /* No nodes specified.  Just add the tag. */
        Blt_Tags_AddTag(&setPtr->tags, tag);
    } else {
        int i;

        for (i = 4; i < objc; i++) {
            Pane *panePtr;
            PaneIterator iter;
            
            if (GetPaneIterator(interp, setPtr, objv[i], &iter) != TCL_OK) {
                return TCL_ERROR;
            }
            for (panePtr = FirstTaggedPane(&iter); panePtr != NULL; 
                 panePtr = NextTaggedPane(&iter)) {
                Blt_Tags_AddItemToTag(&setPtr->tags, tag, panePtr);
            }
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * TagDeleteOp --
 *
 *      .t delete tagName pane1 pane2 pane3
 *
 *---------------------------------------------------------------------------
 */
static int
TagDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    const char *tag;
    long paneId;
    int i;

    tag = Tcl_GetString(objv[3]);
    if (Blt_GetLongFromObj(NULL, objv[3], &paneId) == TCL_OK) {
        Tcl_AppendResult(interp, "bad tag \"", tag, 
                 "\": can't be a number.", (char *)NULL);
        return TCL_ERROR;
    }
    if (strcmp(tag, "all") == 0) {
        Tcl_AppendResult(interp, "can't delete reserved tag \"", tag, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        Pane *panePtr;
        PaneIterator iter;
        
        if (GetPaneIterator(interp, setPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (panePtr = FirstTaggedPane(&iter); panePtr != NULL; 
             panePtr = NextTaggedPane(&iter)) {
            Blt_Tags_RemoveItemFromTag(&setPtr->tags, tag, panePtr);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagExistsOp --
 *
 *      Returns the existence of the one or more tags in the given node.  If
 *      the node has any the tags, true is return in the interpreter.
 *
 *      .t tag exists pane tag1 tag2 tag3...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    int i;
    PaneIterator iter;

    if (GetPaneIterator(interp, setPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        const char *tag;
        Pane *panePtr;

        tag = Tcl_GetString(objv[i]);
        for (panePtr = FirstTaggedPane(&iter); panePtr != NULL; 
             panePtr = NextTaggedPane(&iter)) {
            if (Blt_Tags_ItemHasTag(&setPtr->tags, panePtr, tag)) {
                Tcl_SetBooleanObj(Tcl_GetObjResult(interp), TRUE);
                return TCL_OK;
            }
        }
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), FALSE);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagForgetOp --
 *
 *      Removes the given tags from all panes.
 *
 *      .ts tag forget tag1 tag2 tag3...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagForgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    int i;

    for (i = 3; i < objc; i++) {
        const char *tag;
        long paneId;

        tag = Tcl_GetString(objv[i]);
        if (Blt_GetLongFromObj(NULL, objv[i], &paneId) == TCL_OK) {
            Tcl_AppendResult(interp, "bad tag \"", tag, 
                             "\": can't be a number.", (char *)NULL);
            return TCL_ERROR;
        }
        Blt_Tags_ForgetTag(&setPtr->tags, tag);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagGetOp --
 *
 *      Returns tag names for a given node.  If one of more pattern
 *      arguments are provided, then only those matching tags are returned.
 *
 *      .t tag get pane pat1 pat2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagGetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr; 
    PaneIterator iter;
    Tcl_Obj *listObjPtr;

    if (GetPaneIterator(interp, setPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (panePtr = FirstTaggedPane(&iter); panePtr != NULL; 
         panePtr = NextTaggedPane(&iter)) {
        if (objc == 4) {
            Blt_Tags_AppendTagsToObj(&setPtr->tags, panePtr, listObjPtr);
            Tcl_ListObjAppendElement(interp, listObjPtr, 
                        Tcl_NewStringObj("all", 3));
        } else {
            int i;
            
            /* Check if we need to add the special tags "all" */
            for (i = 4; i < objc; i++) {
                const char *pattern;

                pattern = Tcl_GetString(objv[i]);
                if (Tcl_StringMatch("all", pattern)) {
                    Tcl_Obj *objPtr;

                    objPtr = Tcl_NewStringObj("all", 3);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                    break;
                }
            }
            /* Now process any standard tags. */
            for (i = 4; i < objc; i++) {
                Blt_ChainLink link;
                const char *pattern;
                Blt_Chain chain;

                chain = Blt_Chain_Create();
                Blt_Tags_AppendTagsToChain(&setPtr->tags, panePtr, chain);
                pattern = Tcl_GetString(objv[i]);
                for (link = Blt_Chain_FirstLink(chain); link != NULL; 
                     link = Blt_Chain_NextLink(link)) {
                    const char *tag;
                    Tcl_Obj *objPtr;

                    tag = (const char *)Blt_Chain_GetValue(link);
                    if (!Tcl_StringMatch(tag, pattern)) {
                        continue;
                    }
                    objPtr = Tcl_NewStringObj(tag, -1);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                }
                Blt_Chain_Destroy(chain);
            }
        }    
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagNamesOp --
 *
 *      Returns the names of all the tags in the paneset.  If one of more
 *      node arguments are provided, then only the tags found in those
 *      nodes are returned.
 *
 *      .t tag names pane pane pane...
 *
 *---------------------------------------------------------------------------
 */
static int
TagNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewStringObj("all", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    if (objc == 3) {
        Blt_Tags_AppendAllTagsToObj(&setPtr->tags, listObjPtr);
    } else {
        Blt_HashTable uniqTable;
        int i;

        Blt_InitHashTable(&uniqTable, BLT_STRING_KEYS);
        for (i = 3; i < objc; i++) {
            PaneIterator iter;
            Pane *panePtr;

            if (GetPaneIterator(interp, setPtr, objPtr, &iter) != TCL_OK) {
                goto error;
            }
            for (panePtr = FirstTaggedPane(&iter); panePtr != NULL; 
                 panePtr = NextTaggedPane(&iter)) {
                Blt_ChainLink link;
                Blt_Chain chain;

                chain = Blt_Chain_Create();
                Blt_Tags_AppendTagsToChain(&setPtr->tags, panePtr, chain);
                for (link = Blt_Chain_FirstLink(chain); link != NULL; 
                     link = Blt_Chain_NextLink(link)) {
                    const char *tag;
                    int isNew;

                    tag = Blt_Chain_GetValue(link);
                    Blt_CreateHashEntry(&uniqTable, tag, &isNew);
                }
                Blt_Chain_Destroy(chain);
            }
        }
        {
            Blt_HashEntry *hPtr;
            Blt_HashSearch hiter;

            for (hPtr = Blt_FirstHashEntry(&uniqTable, &hiter); hPtr != NULL;
                 hPtr = Blt_NextHashEntry(&hiter)) {
                objPtr = Tcl_NewStringObj(Blt_GetHashKey(&uniqTable, hPtr), -1);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        }
        Blt_DeleteHashTable(&uniqTable);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
 error:
    Tcl_DecrRefCount(listObjPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagIndicesOp --
 *
 *      Returns the indices associated with the given tags.  The indices
 *      returned will represent the union of panes for all the given tags.
 *
 *      .t tag indices tag1 tag2 tag3...
 *
 *---------------------------------------------------------------------------
 */
static int
TagIndicesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Blt_HashTable paneTable;
    int i;
        
    Blt_InitHashTable(&paneTable, BLT_ONE_WORD_KEYS);
    for (i = 3; i < objc; i++) {
        long paneId;
        const char *tag;

        tag = Tcl_GetString(objv[i]);
        if (Blt_GetLongFromObj(NULL, objv[i], &paneId) == TCL_OK) {
            Tcl_AppendResult(interp, "bad tag \"", tag, 
                             "\": can't be a number.", (char *)NULL);
            goto error;
        }
        if (strcmp(tag, "all") == 0) {
            break;
        } else {
            Blt_Chain chain;

            chain = Blt_Tags_GetItemList(&setPtr->tags, tag);
            if (chain != NULL) {
                Blt_ChainLink link;

                for (link = Blt_Chain_FirstLink(chain); link != NULL; 
                     link = Blt_Chain_NextLink(link)) {
                    Pane *panePtr;
                    int isNew;
                    
                    panePtr = Blt_Chain_GetValue(link);
                    Blt_CreateHashEntry(&paneTable, (char *)panePtr, &isNew);
                }
            }
            continue;
        }
        Tcl_AppendResult(interp, "can't find a tag \"", tag, "\"",
                         (char *)NULL);
        goto error;
    }
    {
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        for (hPtr = Blt_FirstHashEntry(&paneTable, &iter); hPtr != NULL; 
             hPtr = Blt_NextHashEntry(&iter)) {
            Pane *panePtr;
            Tcl_Obj *objPtr;

            panePtr = (Pane *)Blt_GetHashKey(&paneTable, hPtr);
            objPtr = Tcl_NewLongObj(panePtr->index);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
    }
    Blt_DeleteHashTable(&paneTable);
    return TCL_OK;

 error:
    Blt_DeleteHashTable(&paneTable);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagSetOp --
 *
 *      Sets one or more tags for a given pane.  Tag names can't start with a
 *      digit (to distinquish them from node ids) and can't be a reserved tag
 *      ("all").
 *
 *      .t tag set pane tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    int i;
    PaneIterator iter;

    if (GetPaneIterator(interp, setPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        const char *tag;
        Pane *panePtr;
        long paneId;

        tag = Tcl_GetString(objv[i]);
        if (Blt_GetLongFromObj(NULL, objv[i], &paneId) == TCL_OK) {
            Tcl_AppendResult(interp, "bad tag \"", tag, 
                             "\": can't be a number.", (char *)NULL);
            return TCL_ERROR;
        }
        if (strcmp(tag, "all") == 0) {
            Tcl_AppendResult(interp, "can't add reserved tag \"", tag, "\"",
                             (char *)NULL);     
            return TCL_ERROR;
        }
        for (panePtr = FirstTaggedPane(&iter); panePtr != NULL; 
             panePtr = NextTaggedPane(&iter)) {
            Blt_Tags_AddItemToTag(&setPtr->tags, tag, panePtr);
        }    
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagUnsetOp --
 *
 *      Removes one or more tags from a given pane. If a tag doesn't exist or
 *      is a reserved tag ("all"), nothing will be done and no error
 *      message will be returned.
 *
 *      .t tag unset pane tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagUnsetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr;
    PaneIterator iter;

    if (GetPaneIterator(interp, setPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (panePtr = FirstTaggedPane(&iter); panePtr != NULL; 
         panePtr = NextTaggedPane(&iter)) {
        int i;
        for (i = 4; i < objc; i++) {
            const char *tag;

            tag = Tcl_GetString(objv[i]);
            Blt_Tags_RemoveItemFromTag(&setPtr->tags, tag, panePtr);
        }    
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagOp --
 *
 *      This procedure is invoked to process tag operations.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec tagOps[] =
{
    {"add",     1, TagAddOp,      2, 0, "?name? ?option value...?",},
    {"delete",  1, TagDeleteOp,   2, 0, "?pane...?",},
    {"exists",  1, TagExistsOp,   4, 0, "pane ?tag...?",},
    {"forget",  1, TagForgetOp,   3, 0, "?tag...?",},
    {"get",     1, TagGetOp,      4, 0, "pane ?pattern...?",},
    {"indices", 1, TagIndicesOp,  3, 0, "?tag...?",},
    {"names",   1, TagNamesOp,    3, 0, "?pane...?",},
    {"set",     1, TagSetOp,      4, 0, "pane ?tag...",},
    {"unset",   1, TagUnsetOp,    4, 0, "pane ?tag...",},
};

static int numTagOps = sizeof(tagOps) / sizeof(Blt_OpSpec);

static int
TagOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numTagOps, tagOps, BLT_OP_ARG2,
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc)(clientData, interp, objc, objv);
    return result;
}

static int
ViewOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    int width;

    width = VPORTWIDTH(setPtr);
    if (objc == 2) {
        double fract;
        Tcl_Obj *listObjPtr, *objPtr;

        /* Report first and last fractions */
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        /*
         * Note: we are bounding the fractions between 0.0 and 1.0 to support
         * the "canvas"-style of scrolling.
         */
        fract = (double)setPtr->scrollOffset / setPtr->worldWidth;
        objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        fract = (double)(setPtr->scrollOffset + width) / setPtr->worldWidth;
        objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, 
                &setPtr->scrollOffset, setPtr->worldWidth, width, 
                setPtr->scrollUnits, BLT_SCROLL_MODE_HIERBOX) != TCL_OK) {
        return TCL_ERROR;
    }
    setPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Paneset operations.
 *
 * The fields for Blt_OpSpec are as follows:
 *
 *   - operation name
 *   - minimum number of characters required to disambiguate the operation name.
 *   - function associated with operation.
 *   - minimum number of arguments required.
 *   - maximum number of arguments allowed (0 indicates no limit).
 *   - usage string
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec panesetOps[] =
{
    {"add",        1, AddOp,       2, 0, "?name? ?option value?...",},
    {"cget",       2, CgetOp,      3, 3, "option",},
    {"configure",  2, ConfigureOp, 2, 0, "?option value?",},
    {"delete",     1, DeleteOp,    3, 3, "pane",},
    {"exists",     1, ExistsOp,    3, 3, "pane",},
    {"handle",     1, HandleOp,    2, 0, "oper ?args?",},
    {"index",      3, IndexOp,     3, 3, "pane",},
    {"insert",     3, InsertOp,    3, 0, "position ?name? ?option value?...",},
    {"invoke",     3, InvokeOp,    3, 3, "pane",},
    {"move",       1, MoveOp,      3, 0, "pane after|before pane",},
    {"names",      1, NamesOp,     2, 0, "?pattern...?",},
    {"pane",       1, PaneOp,      2, 0, "oper ?args?",},
    {"tag",        1, TagOp,       2, 0, "oper args",},
};

static int numPanesetOps = sizeof(panesetOps) / sizeof(Blt_OpSpec);

static Blt_OpSpec filmstripOps[] =
{
    {"add",        1, AddOp,       2, 0, "?name? ?option value?...",},
    {"cget",       2, CgetOp,      3, 3, "option",},
    {"configure",  2, ConfigureOp, 2, 0, "?option value?",},
    {"delete",     1, DeleteOp,    3, 3, "pane",},
    {"exists",     1, ExistsOp,    3, 3, "pane",},
    {"frame",      1, PaneOp,      2, 0, "oper ?args?",},
    {"handle",     1, HandleOp,    2, 0, "oper ?args?",},
    {"index",      3, IndexOp,     3, 3, "pane",},
    {"insert",     3, InsertOp,    3, 0, "position ?name? ?option value?...",},
    {"invoke",     3, InvokeOp,    3, 3, "pane",},
    {"move",       1, MoveOp,      3, 0, "pane after|before pane",},
    {"names",      1, NamesOp,     2, 0, "?pattern...?",},
    {"see",        1, SeeOp,       3, 3, "pane",},
    {"tag",        1, TagOp,       2, 0, "oper args",},
    {"view",       1, ViewOp,      2, 5, 
        "?moveto fract? ?scroll number what?",},
};

static int numFilmstripOps = sizeof(filmstripOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * PanesetInstCmdDeleteProc --
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
PanesetInstCmdDeleteProc(ClientData clientData)
{
    Paneset *setPtr = clientData;

    /*
     * This procedure could be invoked either because the window was destroyed
     * and the command was then deleted (in which case tkwin is NULL) or
     * because the command was deleted, and then this procedure destroys the
     * widget.
     */
    if (setPtr->tkwin != NULL) {
        Tk_Window tkwin;

        tkwin = setPtr->tkwin;
        setPtr->tkwin = NULL;
        Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PanesetInstCmdProc --
 *
 *      This procedure is invoked to process the TCL command that corresponds
 *      to the paneset geometry manager.  See the user documentation for
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
PanesetInstCmdProc(
    ClientData clientData,      /* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    Paneset *setPtr = clientData;

    if (setPtr->classPtr->type == FILMSTRIP) {
        proc = Blt_GetOpFromObj(interp, numFilmstripOps, filmstripOps, 
                BLT_OP_ARG1, objc, objv, 0);
    } else {
        proc = Blt_GetOpFromObj(interp, numPanesetOps, panesetOps, BLT_OP_ARG1, 
                objc, objv, 0);
    }
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * PanesetCmd --
 *
 *      This procedure is invoked to process the TCL command that corresponds
 *      to a widget managed by this module. See the user documentation for
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
/* ARGSUSED */
static int
PanesetCmd(
    ClientData clientData,              /* Main window associated with
                                         * interpreter. */
    Tcl_Interp *interp,                 /* Current interpreter. */
    int objc,                           /* # of arguments. */
    Tcl_Obj *const *objv)               /* Argument strings. */
{
    Paneset *setPtr;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * Invoke a procedure to initialize various bindings on treeview
     * entries.  If the procedure doesn't already exist, source it from
     * "$blt_library/bltPaneset.tcl".  We deferred sourcing the file until
     * now so that the variable $blt_library could be set within a script.
     */
    if (!Blt_CommandExists(interp, "::blt::Paneset::Initialize")) {
        const char cmd[] = "source [file join $blt_library bltPaneset.tcl]\n";
        if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
            char info[200];
            
            Blt_FormatString(info, 200, "\n    (while loading bindings for %.50s)", 
                      Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            return TCL_ERROR;
        }
    }
    setPtr = NewPaneset(interp, objv[1], PANESET);
    if (setPtr == NULL) {
        goto error;
    }
    if (Blt_ConfigureWidgetFromObj(interp, setPtr->tkwin, panesetSpecs, 
        objc - 2, objv + 2, (char *)setPtr, setPtr->classPtr->type) != TCL_OK) {
        goto error;
    }
    ConfigurePaneset(setPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(setPtr->tkwin),-1);
    return TCL_OK;
  error:
    if (setPtr != NULL) {
        Tk_DestroyWindow(setPtr->tkwin);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilmstripCmd --
 *
 *      This procedure is invoked to process the TCL command that corresponds
 *      to a widget managed by this module. See the user documentation for
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
/* ARGSUSED */
static int
FilmstripCmd(
    ClientData clientData,              /* Main window associated with
                                         * interpreter. */
    Tcl_Interp *interp,                 /* Current interpreter. */
    int objc,                           /* # of arguments. */
    Tcl_Obj *const *objv)               /* Argument strings. */
{
    Paneset *setPtr;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * Invoke a procedure to initialize various bindings on treeview
     * entries.  If the procedure doesn't already exist, source it from
     * "$blt_library/bltFilmstrip.tcl".  We deferred sourcing the file
     * until now so that the variable $blt_library could be set within a
     * script.
     */
    if (!Blt_CommandExists(interp, "::blt::Filmstrip::Initialize")) {
        const char cmd[] = "source [file join $blt_library bltFilmstrip.tcl]";
        if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
            char info[200];

            Blt_FormatString(info, 200, "\n    (while loading bindings for %.50s)", 
                    Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            return TCL_ERROR;;
        }
    }

    setPtr = NewPaneset(interp, objv[1], FILMSTRIP);
    if (setPtr == NULL) {
        goto error;
    }

    if (Blt_ConfigureWidgetFromObj(interp, setPtr->tkwin, panesetSpecs, 
        objc - 2, objv + 2, (char *)setPtr, setPtr->classPtr->type) != TCL_OK) {
        goto error;
    }
    ConfigurePaneset(setPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(setPtr->tkwin),-1);
    return TCL_OK;
  error:
    if (setPtr != NULL) {
        Tk_DestroyWindow(setPtr->tkwin);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PanesetCmdInitProc --
 *
 *      This procedure is invoked to initialize the TCL command that
 *      corresponds to the paneset geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Creates the new TCL command.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_PanesetCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpecs[] = {
        { "filmstrip", FilmstripCmd },
        { "paneset",   PanesetCmd }
    };
    return Blt_InitCmds(interp, "::blt", cmdSpecs, 2);
}


/*
 *---------------------------------------------------------------------------
 *
 * DisplayProc --
 *
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The widgets in the paneset are possibly resized and redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayProc(ClientData clientData)
{
    Paneset *setPtr = clientData;
    Pixmap drawable;
    unsigned int w, h;

    setPtr->flags &= ~REDRAW_PENDING;
#if TRACE
    fprintf(stderr, "DisplayProc(%s)\n", Tk_PathName(setPtr->tkwin));
#endif
    if (setPtr->flags & LAYOUT_PENDING) {
        ComputeGeometry(setPtr);
    }
    if ((Tk_Width(setPtr->tkwin) <= 1) || (Tk_Height(setPtr->tkwin) <=1)) {
        /* Don't bother computing the layout until the size of the window is
         * something reasonable. */
        return;
    }
    if (!Tk_IsMapped(setPtr->tkwin)) {
        /* The paneset's window isn't displayed, so don't bother drawing
         * anything.  By getting this far, we've at least computed the
         * coordinates of the new layout.  */
        return;
    }
    if ((setPtr->classPtr->type == FILMSTRIP) && 
        (setPtr->flags & SCROLL_PENDING)) {
        int width;

        width = VPORTWIDTH(setPtr);
        setPtr->scrollOffset = Blt_AdjustViewport(setPtr->scrollOffset,
                setPtr->worldWidth, width, setPtr->scrollUnits, 
                BLT_SCROLL_MODE_HIERBOX);
        if (setPtr->scrollCmdObjPtr != NULL) {
            Blt_UpdateScrollbar(setPtr->interp, setPtr->scrollCmdObjPtr,
                setPtr->scrollOffset, setPtr->scrollOffset + width,
                setPtr->worldWidth);
        }
        setPtr->flags &= ~SCROLL_PENDING;
    }
    setPtr->numVisible = Blt_Chain_GetLength(setPtr->chain);
    w = Tk_Width(setPtr->tkwin);
    h = Tk_Height(setPtr->tkwin);
    drawable = Blt_GetPixmap(setPtr->display, Tk_WindowId(setPtr->tkwin), w, h, 
        Tk_Depth(setPtr->tkwin));
    Blt_Bg_FillRectangle(setPtr->tkwin, drawable, setPtr->bg, 0, 0, w, h, 
        0, TK_RELIEF_FLAT);
    XCopyArea(setPtr->display, drawable, Tk_WindowId(setPtr->tkwin),
        setPtr->gc, 0, 0, w, h, 0, 0);
    if (setPtr->numVisible > 0) {
        if (ISVERT(setPtr)) {
            VerticalPanes(setPtr);
        } else {
            HorizontalPanes(setPtr);
        }
    }
    Tk_FreePixmap(setPtr->display, drawable);
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayHandle
 *
 *      Draws the pane's handle at its proper location.  First determines the
 *      size and position of the each window.  It then considers the
 *      following:
 *
 *        1. translation of widget position its parent widget.
 *        2. fill style
 *        3. anchor
 *        4. external and internal padding
 *        5. widget size must be greater than zero
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The size of each pane is re-initialized its minimum size.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayHandle(ClientData clientData)
{
    Pane *panePtr = clientData;
    Blt_Bg bg;
    int relief;
    Paneset *setPtr;
    Drawable drawable;

    panePtr->flags &= ~REDRAW_PENDING;
    if (panePtr->handle == NULL) {
        return;
    }
    setPtr = panePtr->setPtr;
    if (setPtr->activePtr == panePtr) {
        bg = GETATTR(panePtr, activeHandleBg);
        relief = setPtr->activeRelief;
    } else {
        bg = GETATTR(panePtr, handleBg);
        relief = setPtr->relief;
    }
    drawable = Tk_WindowId(panePtr->handle);
    Blt_Bg_FillRectangle(panePtr->handle, drawable, bg, 
        0, 0, Tk_Width(panePtr->handle), Tk_Height(panePtr->handle), 
        0, TK_RELIEF_FLAT);
    if (relief != TK_RELIEF_FLAT) {
        Blt_Bg_DrawRectangle(panePtr->handle, drawable, bg, 
                setPtr->handlePad.side1, setPtr->handlePad.side1, 
                Tk_Width(panePtr->handle) - PADDING(setPtr->handlePad), 
                Tk_Height(panePtr->handle) - PADDING(setPtr->handlePad),
                setPtr->handleBW, relief);
    }
    if ((setPtr->highlightThickness > 0) && (panePtr->flags & FOCUS)) {
        GC gc;

        gc = Tk_GCForColor(panePtr->highlightColor, drawable);
        Tk_DrawFocusHighlight(panePtr->handle, gc, setPtr->highlightThickness,
                drawable);
    }
}

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
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
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
                                         * active sash. */
    MODE_SPREADSHEET,                   /* Adjust only the left pane and
                                         * the last pane. */
} AdjustMode;

typedef enum {
    INSERT_AFTER,                       /* Insert after named pane. */
    INSERT_BEFORE                       /* Insert before named pane. */
} InsertOrder;

typedef struct _Paneset Paneset;
typedef struct _Pane Pane;
typedef int (LimitsProc)(int value, Blt_Limits *limitsPtr);
typedef int (SizeProc)(Pane *panePtr);

/*
 * Default values for widget attributes.
 */
#define DEF_ACTIVE_SASH_COLOR   STD_ACTIVE_BACKGROUND
#define DEF_DISABLED_SASH_COLOR STD_DISABLED_BACKGROUND
#define DEF_ACTIVE_SASH_RELIEF  "flat"
#define DEF_ANIMATE             "0"
#define DEF_BACKGROUND          STD_NORMAL_BACKGROUND
#define DEF_BORDERWIDTH         "0"
#define DEF_HCURSOR             "sb_h_double_arrow"
#define DEF_HEIGHT              "0"
#define DEF_MODE                "givetake"
#define DEF_ORIENT              "horizontal"
#define DEF_PAD                 "0"
#define DEF_PANE_ANCHOR         "nw"
#define DEF_PANE_ANCHOR         "nw"
#define DEF_PANE_BORDERWIDTH    "0"
#define DEF_PANE_DATA           (char *)NULL
#define DEF_PANE_DELETE_COMMAND (char *)NULL
#define DEF_PANE_FILL           "none"
#define DEF_PANE_HIDE           "0"
#define DEF_PANE_HIGHLIGHT_BACKGROUND   STD_NORMAL_BACKGROUND
#define DEF_PANE_HIGHLIGHT_COLOR        RGB_BLACK
#define DEF_PANE_PADX           "0"
#define DEF_PANE_PADY           "0"
#define DEF_PANE_RESIZE         "shrink"
#define DEF_PANE_SHOW_HANDLE     "1"
#define DEF_PANE_TAGS           (char *)NULL
#define DEF_PANE_VARIABLE       (char *)NULL
#define DEF_PANE_WEIGHT         "1.0"
#define DEF_SASH_BORDERWIDTH  "1"
#define DEF_SASH_COLOR         STD_NORMAL_BACKGROUND
#define DEF_SASH_CURSOR         (char *)NULL
#define DEF_SASH_HIGHLIGHT_BACKGROUND   STD_NORMAL_BACKGROUND
#define DEF_SASH_HIGHLIGHT_COLOR        RGB_BLACK
#define DEF_SASH_HIGHLIGHT_THICKNESS "1"
#define DEF_SASH_PAD            "0"
#define DEF_SASH_RELIEF       "flat"
#define DEF_SASH_THICKNESS    "2"
#define DEF_SHOW_SASH           "1"
#define DEF_SASH_STATE          "normal"
#define DEF_SIDE                "right"
#define DEF_TAKEFOCUS           "1"
#define DEF_VCURSOR             "sb_v_double_arrow"
#define DEF_WEIGHT              "0"
#define DEF_WIDTH               "0"

#define PANE_DEF_ANCHOR         TK_ANCHOR_NW
#define PANE_DEF_FILL           FILL_BOTH
#define PANE_DEF_IPAD           0
#define PANE_DEF_PAD            0
#define PANE_DEF_PAD            0
#define PANE_DEF_RESIZE         RESIZE_BOTH
#define PANE_DEF_WEIGHT         1.0

#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))
#define VAR_FLAGS (TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS)

/*
 * Paneset structure
 *
 *      The paneset is a set of windows (panes) that may be resized in one
 *      dimension (horizontally or vertically).  How the panes are resized
 *      is dependent upon the paneset's bearing and the adjustment mode in
 *      place.
 *
 *      The bearing is the position of the sash last moved.  By default
 *      it's the last sash.  The position is the just outside of the
 *      sash. So if the window starting at 100 has a width of 200 and the
 *      sash size is 10, the bearing is 310.
 *
 *      The bearing divides the panes into two. Each side is resized
 *      according to the adjustment mode.
 *
 *      givetake       The panes immediately to the left and right of 
 *                     the bearing are grown/shrunk.
 *      slinky         All the panes on either side of the bearing are
 *                     grown/shrunk.
 *      spreadsheet    The pane to the left of the bearing and the last
 *                     pane on the right side are grown/shrunk.
 *                     Intervening panes are unaffected.
 */

struct _Paneset {
    int flags;                          /* See the flags definitions
                                         * below. */
    unsigned int side;                  /* The side of the pane where the
                                         * sash is attached. */
    Display *display;                   /* Display of the widget. */
    Tk_Window tkwin;                    /* The container window into which
                                         * other widgets are arranged. */
    Tcl_Interp *interp;                 /* Interpreter associated with all
                                         * widgets and sashes. */
    Tcl_Command cmdToken;               /* Command token associated with
                                         * this widget. */
    const char *name;                   /* The generated name of the pane
                                         * or the pathname of the window
                                         * created (panesets and
                                         * filmstrips). */
    AdjustMode mode;                    /* Panesets only: Mode to use to
                                         * resize panes when the user
                                         * adjusts a sash. */
    int normalWidth;                    /* Normal dimensions of the paneset */
    int normalHeight;
    int reqWidth, reqHeight;            /* Constraints on the paneset's
                                         * normal width and
                                         * height. Overrides the requested
                                         * width of the window. */

    Tk_Cursor defVertCursor;            /* Default vertical X cursor */
    Tk_Cursor defHorzCursor;            /* Default horizontal X cursor */

    short int width, height;            /* Requested size of the widget. */

    Blt_Bg bg;                          /* 3-D border surrounding the
                                         * window (viewport). */
    /*
     * Scrolling information (filmstrip only):
     */
    int worldWidth;
    /*
     * Focus highlight ring
     */
    XColor *sashHighlightColor;         /* Color for drawing traversal
                                         * highlight. */
    int relief, activeRelief;
    Blt_Pad sashPad;
    int sashBorderWidth;
    int sashThickness;                  /*  */
    int sashSize;
    Blt_Bg sashBg, activeSashBg, disabledSashBg;
    int sashAnchor;                     /* Last known location of sash
                                         * during a move. */
    Blt_Chain panes;                    /* List of panes.  Describes the
                                         * order of the panes in the
                                         * widget.  */
    Blt_HashTable paneTable;            /* Table of panes.  Serves as a
                                         * directory to look up panes from
                                         * windows. */
    Blt_HashTable sashTable;            /* Table of sashes.  Serves as a
                                         * directory to look up panes from
                                         * sash windows. */
    struct _Blt_Tags tags;              /* Table of tags. */
    Pane *activePtr;                    /* Indicates the pane with the
                                         * active sash. */
    Pane *anchorPtr;                    /* Pane that is currently
                                         * anchored */
    int bearing;                        /* Location of the split
                                         * (paneset). */
    size_t numVisible;                  /* # of visible panes. */
    GC gc;
    size_t nextId;                      /* Counter to generate unique
                                         * pane names. */
    size_t nextSashId;                  /* Counter to generate unique
                                         * pane names. */
    Tk_Cursor cursor;                   /* X Cursor */
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
 *      A pane holds a window and a possibly a sash.  It describes how
 *      the window should appear in the pane.  The sash is a rectangle on
 *      the far edge of the pane (horizontal right, vertical bottom).
 *      Normally the last pane does not have a sash.  Sashes may be
 *      hidden.
 *
 *      Initially, the size of a pane consists of
 *       1. the requested size embedded window,
 *       2. any requested internal padding, and
 *       3. the size of the sash (if one is displayed). 
 *
 *      Note: There is no 3D border around the pane.  This can be added by
 *            embedding a frame.  This simplifies the widget so that there
 *            is only one window for the widget.  Windows outside of the
 *            boundary of the pane are occluded.
 */
struct _Pane  {
    unsigned int flags;
    const char *name;                   /* Name of pane */
    Paneset *setPtr;                    /* Paneset widget managing this
                                         * pane. */
    Tk_Window tkwin;                    /* Child widget of paneset to be
                                         * managed. */
    Tk_Window sash;                     /* Sash subwindow. */
    int extBorderWidth;                 /* The external border width of the
                                         * widget. This is needed to check
                                         * if
                                         * Tk_Changes(tkwin)->border_width
                                         * changes. */
    const char *takeFocus;              /* Says whether to select this
                                         * widget during tab traveral
                                         * operations.  This value isn't
                                         * used in C code, but for the
                                         * widget's TCL bindings. */
    Blt_Limits reqWidth, reqHeight;     /* Bounds for width and height
                                         * requests made by the widget. */
    Tk_Anchor anchor;                   /* Anchor type: indicates how the
                                         * widget is positioned if extra
                                         * space is available in the
                                         * pane. */
    Blt_Pad padX;                       /* Extra padding placed left and
                                         * right of the widget. */
    Blt_Pad padY;                       /* Extra padding placed above and
                                         * below the widget */
    int iPadX, iPadY;                   /* Extra padding added to the
                                         * interior of the widget
                                         * (i.e. adds to the requested size
                                         * of the widget) */
    int fill;                           /* Indicates how the widget should
                                         * fill the pane it occupies. */
    int resize;                         /* Indicates if the pane should
                                         * expand/shrink. */
    int x, y;                           /* Origin of pane wrt container. */
    short int width, height;            /* Size of pane, including
                                         * sash. */
    Blt_ChainLink link;                 /* Pointer of this pane into the
                                         * list of panes. */
    Blt_HashEntry *hashPtr;             /* Pointer of this pane into
                                         * hashtable of panes. */
    Blt_HashEntry *sashHashPtr;       /* Pointer of this pane into
                                         * hashtable of sashes. */
    int index;                          /* Index of the pane. */
    int size;                           /* Current size of the pane. This
                                         * size is bounded by min and
                                         * max. */
    /*
     * nom and size perform similar duties.  I need to keep track of the
     * amount of space allocated to the pane (using size).  But at the same
     * time, I need to indicate that space can be parcelled out to this
     * pane.  If a nominal size was set for this pane, I don't want to add
     * space.
     */
    int nom;                            /* The nominal size (neither
                                         * expanded nor shrunk) of the pane
                                         * based upon the requested size of
                                         * the widget embedded in this
                                         * pane. */
    int min, max;                       /* Size constraints on the pane */
    float weight;                       /* Weight of pane. */
    Blt_Limits reqSize;                 /* Requested bounds for the size of
                                         * the pane. The pane will not
                                         * expand or shrink beyond these
                                         * limits, regardless of how it was
                                         * specified (max widget size).
                                         * This includes any extra padding
                                         * which may be specified. */
    Blt_Bg sashBg;
    Blt_Bg activeSashBg;
    Blt_Bg disabledSashBg;
    Blt_Bg bg;                          /* 3-D background border
                                         * surrounding the widget */
    Tcl_TimerToken timerToken;
    int scrollTarget;                   /* Target offset to scroll to. */
    int scrollIncr;                     /* Current increment. */
    Tcl_Obj *variableObjPtr;            /* Name of TCL variable.  If
                                         * non-NULL, this variable will be
                                         * set to the value string of the
                                         * selected item. */
    Tcl_Obj *deleteCmdObjPtr;           /* If non-NULL, Routine to call
                                         * when pane is deleted. */
    Tcl_Obj *dataObjPtr;                /* User-defined data associated
                                         * with this pane. */
};

/* Pane/sash flags.  */

#define HIDDEN          (1<<8)          /* Do not display the pane. */
#define DISABLED        (1<<9)          /* Sash is disabled. */
#define ONSCREEN        (1<<10)         /* Pane is on-screen. */
#define SASH_ACTIVE     (1<<11)         /* Sash is currently active. */
#define SASH            (1<<12)         /* The pane has a sash. */
#define SHOW_SASH       (1<<13)         /* Display the pane. */

#define VIRGIN          (1<<24)

/* Orientation. */
#define SIDE_VERTICAL   (SIDE_TOP|SIDE_BOTTOM)
#define SIDE_HORIZONTAL (SIDE_LEFT|SIDE_RIGHT)

/* Sash positions. */
#define SASH_LEFT     SIDE_RIGHT
#define SASH_RIGHT    SIDE_LEFT
#define SASH_TOP      SIDE_BOTTOM
#define SASH_BOTTOM   SIDE_TOP

#define SASH_FARSIDE  (SASH_RIGHT|SASH_BOTTOM)    
#define SASH_NEARSIDE (SASH_LEFT|SASH_TOP)        


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

static Blt_OptionParseProc ObjToOrient;
static Blt_OptionPrintProc OrientToObj;
static Blt_CustomOption orientOption = {
    ObjToOrient, OrientToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToAdjustMode;
static Blt_OptionPrintProc AdjustModeToObj;
static Blt_CustomOption adjustModeOption = {
    ObjToAdjustMode, AdjustModeToObj, NULL, (ClientData)0,
};

static Blt_OptionFreeProc FreeTagsProc;
static Blt_OptionParseProc ObjToTags;
static Blt_OptionPrintProc TagsToObj;
static Blt_CustomOption tagsOption = {
    ObjToTags, TagsToObj, FreeTagsProc, (ClientData)0
};

static Blt_OptionParseProc ObjToStateProc;
static Blt_OptionPrintProc StateToObjProc;
static Blt_CustomOption stateOption = {
    ObjToStateProc, StateToObjProc, NULL, (ClientData)0
};

static Blt_ConfigSpec paneSetSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activesashcolor", "activeSashColor", 
        "ActiveSashColor", DEF_ACTIVE_SASH_COLOR,
        Blt_Offset(Paneset, activeSashBg), 0},
    {BLT_CONFIG_RELIEF, "-activesashrelief", "activeSashRelief", 
        "ActiveSashRelief", DEF_ACTIVE_SASH_RELIEF, 
        Blt_Offset(Paneset, activeRelief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BACKGROUND, Blt_Offset(Paneset, bg), 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_BACKGROUND, "-disabledsashcolor", "disabledSashColor", 
        "DisabledSashColor", DEF_DISABLED_SASH_COLOR,
        Blt_Offset(Paneset, disabledSashBg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_HEIGHT,
        Blt_Offset(Paneset, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_CUSTOM, "-mode", "mode", "Mode", DEF_MODE,
        Blt_Offset(Paneset, mode), BLT_CONFIG_DONT_SET_DEFAULT,
        &adjustModeOption},
    {BLT_CONFIG_CUSTOM, "-orient", "orient", "Orient", DEF_ORIENT, 
        Blt_Offset(Paneset, flags), BLT_CONFIG_DONT_SET_DEFAULT, &orientOption},
    {BLT_CONFIG_PIXELS_NNEG, "-sashborderwidth", "sashBorderWidth", 
        "SashBorderWidth", DEF_SASH_BORDERWIDTH,
        Blt_Offset(Paneset, sashBorderWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-sashcolor", "sashColor", "SashColor",
        DEF_SASH_COLOR, Blt_Offset(Paneset, sashBg), 0},
    {BLT_CONFIG_CURSOR, "-sashcursor", "sashCursor", "SashCursor",
        DEF_SASH_CURSOR, Blt_Offset(Paneset, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PAD, "-sashpad", "sashPad", "SashPad", DEF_SASH_PAD, 
        Blt_Offset(Paneset, sashPad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-sashrelief", "sashRelief", "SashRelief", 
        DEF_SASH_RELIEF, Blt_Offset(Paneset, relief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-sashthickness", "sashThickness", "SashThickness",
        DEF_SASH_THICKNESS, Blt_Offset(Paneset, sashThickness),
        BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_WIDTH,
        Blt_Offset(Paneset, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec paneSpecs[] =
{
    {BLT_CONFIG_ANCHOR, "-anchor", (char *)NULL, (char *)NULL, DEF_PANE_ANCHOR,
        Blt_Offset(Pane, anchor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        (char *)NULL, Blt_Offset(Pane, bg), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_OBJ, "-data", "data", "Data", DEF_PANE_DATA, 
        Blt_Offset(Pane, dataObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-deletecommand", "deleteCommand", "DeleteCommand",
        DEF_PANE_DELETE_COMMAND, Blt_Offset(Pane, deleteCmdObjPtr),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FILL, "-fill", "fill", "Fill", DEF_PANE_FILL, 
        Blt_Offset(Pane, fill), BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_CUSTOM, "-height", "height", "Height", (char *)NULL, 
        Blt_Offset(Pane, reqHeight), 0, &bltLimitsOption},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_PANE_HIDE, 
        Blt_Offset(Pane, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDDEN },
    {BLT_CONFIG_PIXELS_NNEG, "-ipadx", "iPadX", "IPadX", (char *)NULL,
        Blt_Offset(Pane, iPadX), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-ipady", "iPadY", "IPadY", (char *)NULL, 
        Blt_Offset(Pane, iPadY), 0},
    {BLT_CONFIG_RESIZE, "-resize", "resize", "Resize", DEF_PANE_RESIZE,
        Blt_Offset(Pane, resize), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-showsash", "showSash", "showSash", 
        DEF_SHOW_SASH, Blt_Offset(Pane, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SHOW_SASH},
    {BLT_CONFIG_CUSTOM, "-size", "size", "Size", (char *)NULL, 
        Blt_Offset(Pane, reqSize), 0, &bltLimitsOption},
    {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_SASH_STATE, 
        Blt_Offset(Pane, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        &stateOption},
     {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_PANE_TAGS, 0,
        BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
        DEF_TAKEFOCUS, Blt_Offset(Pane, takeFocus), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FLOAT, "-weight", "weight", "Weight", DEF_PANE_WEIGHT,
        Blt_Offset(Pane, weight), BLT_CONFIG_DONT_SET_DEFAULT | PANESET},
    {BLT_CONFIG_CUSTOM, "-width", "width", "Width", (char *)NULL, 
        Blt_Offset(Pane, reqWidth), 0, &bltLimitsOption},
    {BLT_CONFIG_CUSTOM, "-window", "window", "Window", (char *)NULL, 
        Blt_Offset(Pane, tkwin), BLT_CONFIG_NULL_OK, &childOption},
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
    Paneset *setPtr;                   /* Paneset that we're iterating
                                        * over. */
    IteratorType type;                  /* Type of iteration:
                                         * ITER_TAG      By item tag.
                                         * ITER_ALL      By every item.
                                         * ITER_SINGLE   Single item: either 
                                         *               tag or index.
                                         * ITER_PATTERN  Over a consecutive 
                                         *               range of indices.
                                         */
    Pane *startPtr;                     /* Starting pane.  Starting point
                                         * of search, saved if iterator is
                                         * reused.  Used for ITER_ALL and
                                         * ITER_SINGLE searches. */
    Pane *endPtr;                       /* Ending pend (inclusive). */
    Pane *nextPtr;                      /* Next pane. */

    /* For tag-based searches. */
    const char *tagName;                /* If non-NULL, is the tag that we
                                         * are currently iterating over. */
    Blt_ChainLink link;
} PaneIterator;

/*
 * Forward declarations
 */
static Tcl_FreeProc PanesetFreeProc;
static Tcl_IdleProc DisplayProc;
static Tcl_IdleProc DisplaySashProc;
static Tcl_ObjCmdProc PanesetCmd;
static Tk_EventProc PanesetEventProc;
static Tk_EventProc PaneEventProc;
static Tk_EventProc SashEventProc;
static Tcl_FreeProc PaneFreeProc;
static Tcl_ObjCmdProc PanesetInstCmdProc;
static Tcl_CmdDeleteProc PanesetInstCmdDeleteProc;

static int GetPaneIterator(Tcl_Interp *interp, Paneset *setPtr, Tcl_Obj *objPtr,
        PaneIterator *iterPtr);
static int GetPaneFromObj(Tcl_Interp *interp, Paneset *setPtr, Tcl_Obj *objPtr, 
        Pane **panePtrPtr);

static INLINE int 
ScreenX(Pane *panePtr)
{
    return panePtr->x;
}

static INLINE int 
ScreenY(Pane *panePtr)
{
    return panePtr->y;
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
    
    if (strcmp(tagName, "all") == 0) {
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
    if (panePtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(panePtr->timerToken);
        panePtr->timerToken = 0;
    }
    if (panePtr->tkwin != NULL) {
        Tk_DeleteEventHandler(panePtr->tkwin, StructureNotifyMask,
                PaneEventProc, panePtr);
        Tk_ManageGeometry(panePtr->tkwin, (Tk_GeomMgr *)NULL, panePtr);
        if (Tk_IsMapped(panePtr->tkwin)) {
            Tk_UnmapWindow(panePtr->tkwin);
        }
    }
    if (panePtr->deleteCmdObjPtr != NULL) {
        if (Tcl_EvalObjEx(setPtr->interp, panePtr->deleteCmdObjPtr,
                TCL_EVAL_GLOBAL) != TCL_OK) {
            Tcl_BackgroundError(setPtr->interp);
        }
    }
    if (panePtr->sash != NULL) {
        Tk_Window tkwin;

        tkwin = panePtr->sash;
        Tk_DeleteEventHandler(tkwin, 
                ExposureMask|FocusChangeMask|StructureNotifyMask, 
                SashEventProc, panePtr);
        Tk_ManageGeometry(tkwin, (Tk_GeomMgr *)NULL, panePtr);
        panePtr->sash = NULL;
        Tk_DestroyWindow(tkwin);
    }
    Blt_Tags_ClearTagsFromItem(&setPtr->tags, panePtr);
    Blt_FreeOptions(paneSpecs, (char *)panePtr, setPtr->display, 0);
    if (setPtr->anchorPtr == panePtr) {
        setPtr->anchorPtr = NULL;
    }
    if (panePtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&setPtr->paneTable, panePtr->hashPtr);
        panePtr->hashPtr = NULL;
    }
    if (panePtr->link != NULL) {
        Blt_Chain_DeleteLink(setPtr->panes, panePtr->link);
        panePtr->link = NULL;
    }
    if (panePtr->sashHashPtr != NULL) {
        Blt_DeleteHashEntry(&setPtr->sashTable, panePtr->sashHashPtr);
        panePtr->sashHashPtr = NULL;
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
ObjToChild(ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
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
         * Allow only widgets that are children of the paneset window to be
         * used.  We are using the paneset window as a viewport to clip the
         * children as needed.
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
ChildToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
           char *widgRec, int offset, int flags)  
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
 * ObjToAdjustMode --
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
ObjToAdjustMode(ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
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
 * AdjustModeToObj --
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
AdjustModeToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
                char *widgRec, int offset, int flags)  
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
 * ObjToOrient --
 *
 *      Converts the string representing orientation into a bitflag.
 *
 * Results:
 *      The return value is a standard TCL result.  The flags are
 *      updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToOrient(ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
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
 * OrientToObj --
 *
 *      Return the name of the orientaition.
 *
 * Results:
 *      The name representing the orientation is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
OrientToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
            char *widgRec, int offset, int flags)  
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
FreeTagsProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Paneset *setPtr;
    Pane *panePtr = (Pane *)widgRec;

    setPtr = panePtr->setPtr;
    Blt_Tags_ClearTagsFromItem(&setPtr->tags, panePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTags --
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
ObjToTags(ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
          Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
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
 * TagsToObj --
 *
 *      Return the tags used by the pane.
 *
 * Results:
 *      A TCL list representing the tags is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TagsToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
          char *widgRec, int offset, int flags)  
{
    Paneset *setPtr;
    Pane *panePtr = (Pane *)widgRec;
    Tcl_Obj *listObjPtr;

    setPtr = panePtr->setPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Blt_Tags_AppendTagsToObj(&setPtr->tags, panePtr, listObjPtr);
    return listObjPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToStateProc --
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
ObjToStateProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
               Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    char *string;
    int flag;

    string = Tcl_GetString(objPtr);
    if (strcmp(string, "disabled") == 0) {
        flag = DISABLED;
    } else if (strcmp(string, "normal") == 0) {
        flag = 0;
    } else {
        Tcl_AppendResult(interp, "unknown state \"", string, 
            "\": should be disabled, or normal.", (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~DISABLED;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StateToObjProc --
 *
 *      Return the name of the state.
 *
 * Results:
 *      The name representing the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StateToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
               char *widgRec, int offset, int flags)  
{
    unsigned int state = *(unsigned int *)(widgRec + offset);
    const char *string;

    if (state & DISABLED) {
        string = "disabled";
    } else {
        string = "normal";
    }
    return Tcl_NewStringObj(string, -1);
}


static void
EventuallyRedrawSash(Pane *panePtr)
{
    if ((panePtr->flags & REDRAW_PENDING) == 0) {
        panePtr->flags |= REDRAW_PENDING;
        Tcl_DoWhenIdle(DisplaySashProc, panePtr);
    }
}

static Pane *
FirstPane(Paneset *setPtr, unsigned int hateFlags)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(setPtr->panes); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;

        panePtr = Blt_Chain_GetValue(link);
        if ((panePtr->flags & hateFlags) == 0) {
            return panePtr;
        }
    }
    return NULL;
}

static Pane *
LastPane(Paneset *setPtr, unsigned int hateFlags)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(setPtr->panes); link != NULL;
         link = Blt_Chain_PrevLink(link)) {
        Pane *panePtr;

        panePtr = Blt_Chain_GetValue(link);
        if ((panePtr->flags & hateFlags) == 0) {
            return panePtr;
        }
    }
    return NULL;
}


static Pane *
NextPane(Pane *panePtr, unsigned int hateFlags)
{
    if (panePtr != NULL) {
        Blt_ChainLink link;

        for (link = Blt_Chain_NextLink(panePtr->link); link != NULL; 
             link = Blt_Chain_NextLink(link)) {
            panePtr = Blt_Chain_GetValue(link);
            if ((panePtr->flags & hateFlags) == 0) {
                return panePtr;
            }
        }
    }
    return NULL;
}

static Pane *
PrevPane(Pane *panePtr, unsigned int hateFlags)
{
    if (panePtr != NULL) {
        Blt_ChainLink link;
        
        for (link = Blt_Chain_PrevLink(panePtr->link); link != NULL; 
             link = Blt_Chain_PrevLink(link)) {
            panePtr = Blt_Chain_GetValue(link);
            if ((panePtr->flags & hateFlags) == 0) {
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
 *      This procedure is invoked by the Tk event handler when the
 *      container widget is reconfigured or destroyed.
 *
 *      The paneset will be rearranged at the next idle point if the
 *      container widget has been resized or moved. There's a distinction
 *      made between parent and non-parent container arrangements.  When
 *      the container is the parent of the embedded widgets, the widgets
 *      will automatically keep their positions relative to the container,
 *      even when the container is moved.  But if the container is not the
 *      parent, those widgets have to be moved manually.  This can be a
 *      performance hit in rare cases where we're scrolling the container
 *      (by moving the window) and there are lots of non-child widgets
 *      arranged inside.
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
        setPtr->anchorPtr = LastPane(setPtr, HIDDEN); /* Reset anchor pane. */
        setPtr->flags |= SCROLL_PENDING;
        EventuallyRedraw(setPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PaneEventProc --
 *
 *      This procedure is invoked by the Tk event handler when
 *      StructureNotify events occur in a widget managed by the paneset.
 *
 *      For example, when a managed widget is destroyed, it frees the
 *      corresponding pane structure and arranges for the paneset layout to
 *      be re-computed at the next idle point.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      If the managed widget was deleted, the Pane structure gets cleaned
 *      up and the paneset is rearranged.
 *
 *---------------------------------------------------------------------------
 */
static void
PaneEventProc(
    ClientData clientData,              /* Pointer to Pane structure for
                                         * widget referred to by
                                         * eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    Pane *panePtr = (Pane *)clientData;
    Paneset *setPtr = panePtr->setPtr;

    if (eventPtr->type == ConfigureNotify) {
        int extBorderWidth;

        if (panePtr->tkwin == NULL) {
            return;
        }
        extBorderWidth = Tk_Changes(panePtr->tkwin)->border_width;
        if (panePtr->extBorderWidth != extBorderWidth) {
            panePtr->extBorderWidth = extBorderWidth;
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
 *      This procedure is invoked by Tk_GeometryRequest for widgets managed
 *      by the paneset geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the paneset to have its layout re-computed and
 *      re-arranged at the next idle point.
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
 * SashEventProc --
 *
 *      This procedure is invoked by the Tk event handler when various
 *      events occur in the pane's sash subwindow maintained by this
 *      widget.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
SashEventProc(
    ClientData clientData,              /* Pointer to Pane structure for
                                         * sash referred to by eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    Pane *panePtr = (Pane *)clientData;

    if (eventPtr->type == Expose) {     
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedrawSash(panePtr);
        }
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            if (eventPtr->type == FocusIn) {
                panePtr->flags |= FOCUS;
            } else {
                panePtr->flags &= ~FOCUS;
            }
            EventuallyRedrawSash(panePtr);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        if (panePtr->sash == NULL) {
            return;
        }
        EventuallyRedrawSash(panePtr);
    } else if (eventPtr->type == DestroyNotify) {
        panePtr->sash = NULL;
    } 
}

/*
 *---------------------------------------------------------------------------
 *
 * NextTaggedPane --
 *
 *      Returns the next pane derived from the given tag.
 *
 * Results:
 *      Returns the pointer to the next pane in the iterator.  If no more
 *      panes are available, then NULL is returned.
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
 *      Returns the first pane in the sequence.  If no more panes are in
 *      the list, then NULL is returned.
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
 *      Gets the pane associated the given index, tag, or label.  This
 *      routine is used when you want only one pane.  It's an error if more
 *      than one pane is specified (e.g. "all" tag).  It's also an error if
 *      the tag is empty (no panes are currently tagged).
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

        link = Blt_Chain_GetNthLink(setPtr->panes, pos);
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
        panePtr = FirstPane(setPtr, HIDDEN | DISABLED);
    } else if ((c == 'l') && (strcmp(string, "last") == 0)) {
        panePtr = LastPane(setPtr, HIDDEN | DISABLED);
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
        panePtr = LastPane(setPtr, 0);
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
 *      Converts a string representing a pane index into an pane pointer.
 *      The index may be in one of the following forms:
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
        
        hPtr = Blt_FindHashEntry(&setPtr->sashTable, string);
        if (hPtr != NULL) {
            panePtr = Blt_GetHashValue(hPtr);
            iterPtr->startPtr = iterPtr->endPtr = panePtr;
            iterPtr->type = ITER_SINGLE;
            return TCL_OK;
        }
        return TCL_ERROR;
    } else if ((c == 'a') && (strcmp(iterPtr->tagName, "all") == 0)) {
        iterPtr->type  = ITER_ALL;
        iterPtr->link = Blt_Chain_FirstLink(setPtr->panes);
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
        iterPtr->link = Blt_Chain_FirstLink(setPtr->panes);
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
 *      The pane will also contain a Tk window to represent the sash.  It's
 *      name is automatically generated as "sash0", "sash1", etc.
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

    if (name == NULL) {
        char string[200];

        /* Generate an unique pane name. */
        do {
            sprintf(string, "pane%lu", (unsigned long)setPtr->nextId++);
            hPtr = Blt_CreateHashEntry(&setPtr->paneTable, string, &isNew);
        } while (!isNew);
    }  else {
        hPtr = Blt_CreateHashEntry(&setPtr->paneTable, name, &isNew);
        if (!isNew) {
            Tcl_AppendResult(interp, "pane \"", name, "\" already exists.",
                             (char *)NULL);
            return NULL;
        }
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
    panePtr->flags = VIRGIN | SHOW_SASH;
    panePtr->resize = RESIZE_BOTH;
    panePtr->weight = 1.0f;
    panePtr->link = Blt_Chain_NewLink();
    panePtr->index = Blt_Chain_GetLength(setPtr->panes);
    Blt_Chain_SetValue(panePtr->link, panePtr);
    Blt_SetHashValue(hPtr, panePtr);
    
    /* Generate an unique subwindow name. It will be in the form "sash0",
     * "sash1", etc. which hopefully will not collide with any existing
     * child window names for this widget. */
    {
        char string[200];
        char *path;

        path = Blt_AssertMalloc(strlen(Tk_PathName(setPtr->tkwin)) + 200);
        do {
            sprintf(string, "sash%lu", (unsigned long)setPtr->nextSashId++);
            sprintf(path, "%s.%s", Tk_PathName(setPtr->tkwin), string);
        } while (Tk_NameToWindow(NULL, path, setPtr->tkwin) != NULL);
        Blt_Free(path);
        panePtr->sash = Tk_CreateWindow(interp, setPtr->tkwin, string,
                (char *)NULL);
        if (panePtr->sash == NULL) {
            DestroyPane(panePtr);
            return NULL;
        }
    } 
    Tk_CreateEventHandler(panePtr->sash, 
                          ExposureMask|FocusChangeMask|StructureNotifyMask, 
                          SashEventProc, panePtr);
    Tk_SetClass(panePtr->sash, "BltPanesetSash");

    /* Also add pane to sash table */
    hPtr = Blt_CreateHashEntry(&setPtr->sashTable, Tk_PathName(panePtr->sash),
                               &isNew);
    panePtr->sashHashPtr = hPtr;
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
    Paneset *setPtr = panePtr->setPtr;

    DestroyPane(panePtr);
    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
}

static int
GetInsertOrder(Tcl_Interp *interp, Tcl_Obj *objPtr, InsertOrder *ordPtr)
{
    const char *string;
    char c;
    int length;
    
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'b') && (strncmp(string, "before", length) == 0)) {
        *ordPtr = INSERT_BEFORE;
    } else if ((c == 'a') && (strncmp(string, "after", length) == 0)) {
        *ordPtr = INSERT_AFTER;
    } else {
        Tcl_AppendResult(interp, "bad key word \"", string,
                "\": should be after or before", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

static void
RenumberPanes(Paneset *setPtr)
{
    int count;
    Blt_ChainLink link;

    count = 0;
    for (link = Blt_Chain_FirstLink(setPtr->panes); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;
        
        panePtr = Blt_Chain_GetValue(link);
        panePtr->index = count;
        count++;
    }
}

static void
MovePane(Paneset *setPtr, Pane *panePtr, InsertOrder order, Pane *relPtr)
{
    if (Blt_Chain_GetLength(setPtr->panes) == 1) {
        return;                         /* Can't rearrange one item. */
    }
    Blt_Chain_UnlinkLink(setPtr->panes, panePtr->link);
    switch (order) {
    case INSERT_AFTER:                  /* After */
        Blt_Chain_LinkAfter(setPtr->panes, panePtr->link, relPtr->link);
        break;
    case INSERT_BEFORE:                 /* Before */
        Blt_Chain_LinkBefore(setPtr->panes, panePtr->link, relPtr->link);
        break;
    }
    RenumberPanes(setPtr);
    setPtr->flags |= LAYOUT_PENDING;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewPaneset --
 *
 *      This procedure creates and initializes a new Paneset structure with
 *      tkwin as its container widget. The internal structures associated
 *      with the paneset are initialized.
 *
 * Results:
 *      Returns the pointer to the new Paneset structure describing the new
 *      paneset geometry manager.  If an error occurred, the return value
 *      will be NULL and an error message is left in interp->result.
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
    Tk_SetClass(tkwin, "BltPaneset");
    setPtr->tkwin = tkwin;
    setPtr->interp = interp;
    setPtr->display = Tk_Display(tkwin);
    setPtr->sashThickness = 2;
    setPtr->sashPad.side1 = setPtr->sashPad.side2 = 2;
    setPtr->relief = TK_RELIEF_FLAT;
    setPtr->activeRelief = TK_RELIEF_RAISED;
    setPtr->sashBorderWidth = 1;
    setPtr->flags = LAYOUT_PENDING;
    setPtr->mode = MODE_GIVETAKE;
    setPtr->side = SASH_FARSIDE;
    Blt_SetWindowInstanceData(tkwin, setPtr);
    Blt_InitHashTable(&setPtr->paneTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&setPtr->sashTable, BLT_STRING_KEYS);
    Blt_Tags_Init(&setPtr->tags);
    Tk_CreateEventHandler(tkwin, ExposureMask|StructureNotifyMask, 
                          PanesetEventProc, setPtr);
    setPtr->panes = Blt_Chain_Create();
    setPtr->cmdToken = Tcl_CreateObjCommand(interp, Tk_PathName(tkwin), 
        PanesetInstCmdProc, setPtr, PanesetInstCmdDeleteProc);
    setPtr->defVertCursor = Tk_GetCursor(interp, tkwin, DEF_VCURSOR);
    setPtr->defHorzCursor = Tk_GetCursor(interp, tkwin, DEF_HCURSOR);
    return setPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * DestroyPaneset --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the Paneset structure at a safe time (when no-one is using
 *      it anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the paneset geometry manager is freed
 *      up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyPaneset(Paneset *setPtr)         /* Paneset structure */
{
    Blt_ChainLink link;

    Blt_FreeOptions(paneSetSpecs, (char *)setPtr, setPtr->display, 0);
    /* Release the chain of entries. */
    for (link = Blt_Chain_FirstLink(setPtr->panes); link != NULL;
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
    Blt_Chain_Destroy(setPtr->panes);
    Blt_DeleteHashTable(&setPtr->paneTable);
    Blt_Free(setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * PanesetFreeProc --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the Paneset structure at a safe time (when no-one is using
 *      it anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the paneset geometry manager is freed
 *      up.
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
 *      anchor specified.  The anchor indicates where the given xy position
 *      is in relation to the bounding box.
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
    int dx, int dy,                     /* Difference between outer and
                                         * inner regions. */
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
         panePtr = PrevPane(panePtr, HIDDEN)) {
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
    for (panePtr = NextPane(setPtr->anchorPtr, HIDDEN); panePtr != NULL; 
         panePtr = NextPane(panePtr, HIDDEN)) {
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
    /* The left span is every pane before and including) the anchor
     * pane. */
    for (panePtr = setPtr->anchorPtr; panePtr != NULL; 
         panePtr = PrevPane(panePtr, HIDDEN)) {
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
    for (panePtr = NextPane(setPtr->anchorPtr, HIDDEN); panePtr != NULL; 
         panePtr = NextPane(panePtr, HIDDEN)) {
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

    w = GetReqWidth(panePtr) + PADDING(panePtr->padX); 
    if ((ISHORIZ(panePtr->setPtr)) && (panePtr->flags & SASH)) {
        w += panePtr->setPtr->sashSize;
    }
    return w;
}

static int
GetReqPaneHeight(Pane *panePtr)
{
    int h;

    h = GetReqHeight(panePtr) + PADDING(panePtr->padY);
    if ((ISVERT(panePtr->setPtr)) && (panePtr->flags & SASH)) {
        h += panePtr->setPtr->sashSize;
    }
    return h;
}


/*
 *---------------------------------------------------------------------------
 *
 * GrowPane --
 *
 *      Expand the span by the amount of the extra space needed.  This
 *      procedure is used in Layout*Panes to grow the panes to their
 *      minimum nominal size, starting from a zero width and height space.
 *
 *      On the first pass we try to add space to panes which have not been
 *      touched yet (i.e. have no nominal size).
 *
 *      If there is still extra space after the first pass, this means that
 *      there were no panes could be expanded. This pass will try to remedy
 *      this by parcelling out the left over space evenly among the rest of
 *      the panes.
 *
 *      On each pass, we have to keep iterating over the list, evenly
 *      doling out slices of extra space, because we may hit pane limits as
 *      space is donated.  In addition, if there are left over pixels
 *      because of round-off, this will distribute them as evenly as
 *      possible.
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
 *      Grow the span by the designated amount.  Size constraints on the
 *      panes may prevent any or all of the spacing adjustments.
 *
 *      This is very much like the GrowPane procedure, but in this case we
 *      are expanding all the panes. It uses a two pass approach, first
 *      giving space to panes which are smaller than their nominal
 *      sizes. This is because constraints on the panes may cause resizing
 *      to be non-linear.
 *
 *      If there is still extra space, this means that all panes are at
 *      least to their nominal sizes.  The second pass will try to add the
 *      left over space evenly among all the panes which still have space
 *      available (i.e. haven't reached their specified max sizes).
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
    int numAdjust;                      /* Number of rows/columns that
                                         * still can be adjusted. */
    Blt_ChainLink link;
    float totalWeight;

    /*
     * Pass 1: First adjust the size of panes that still haven't reached
     *         their nominal size.
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
                int avail;              /* Amount of space still
                                         * available. */

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
 *      This is very much like the GrowPane procedure, but in this case we
 *      are shrinking the panes. It uses a two pass approach, first
 *      subtracting space to panes which are larger than their nominal
 *      sizes. This is because constraints on the panes may cause resizing
 *      to be non-linear.
 *
 *      After pass 1, if there is still extra to be removed, this means
 *      that all panes are at least to their nominal sizes.  The second
 *      pass will try to remove the extra space evenly among all the panes
 *      which still have space available (i.e haven't reached their
 *      respective min sizes).
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
    int numAdjust;                      /* Number of panes that still can
                                         * be adjusted. */
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
        int ration;                     /* Amount of space to subtract from
                                         * each row/column. */
        ration = (int)(extra / totalWeight);
        if (ration == 0) {
            ration = 1;
        }
        for (link = Blt_Chain_FirstLink(chain); (link != NULL) && (extra > 0);
            link = Blt_Chain_NextLink(link)) {
            Pane *panePtr;

            panePtr = Blt_Chain_GetValue(link);
            if (panePtr->weight > 0.0f) {
                int avail;              /* Amount of space still
                                         * available */

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
     * Pass 2: Now adjust the panes with space still available (i.e.  are
     *         bigger than their minimum size).
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
        int ration;                     /* Amount of space to subtract from
                                         * each pane. */
        ration = (int)(extra / totalWeight);
        if (ration == 0) {
            ration = 1;
        }
        for (link = Blt_Chain_FirstLink(chain); (link != NULL) && (extra > 0);
            link = Blt_Chain_NextLink(link)) {
            Pane *panePtr;

            panePtr = Blt_Chain_GetValue(link);
            if (panePtr->weight > 0.0f) {
                int avail;              /* Amount of space still
                                         * available */

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
         panePtr = PrevPane(panePtr, HIDDEN)) {
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
         panePtr = NextPane(panePtr, HIDDEN)) {
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
         panePtr = NextPane(panePtr, HIDDEN)) {
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
         panePtr = PrevPane(panePtr, HIDDEN)) {
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
         panePtr = PrevPane(panePtr, HIDDEN)) {
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
    for (panePtr = LastPane(setPtr, HIDDEN);
         (panePtr != leftPtr) && (extra > 0); 
         panePtr = PrevPane(panePtr, HIDDEN)) {
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
    for (panePtr = LastPane(setPtr, HIDDEN);(panePtr != leftPtr) && (extra > 0);
         panePtr = PrevPane(panePtr, HIDDEN)) {
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
         panePtr = PrevPane(panePtr, HIDDEN)) {
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
         panePtr = NextPane(panePtr, HIDDEN)) {
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
 *      (this is usually zero). This routine gets called when new widgets
 *      are added, deleted, or resized.
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

    for (link = Blt_Chain_FirstLink(setPtr->panes); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;
        int extra, size;

        panePtr = Blt_Chain_GetValue(link);
        /*
         * The constraint procedure below also has the desired side-effect
         * of setting the minimum, maximum, and nominal values to the
         * requested size of its associated widget (if one exists).
         */
        if (ISVERT(setPtr)) {
            size = BoundHeight(0, &panePtr->reqSize);
            extra = PADDING(panePtr->padY);
        } else {
            size = BoundWidth(0, &panePtr->reqSize);
            extra = PADDING(panePtr->padX);
        }
        if (panePtr->flags & SASH) {
            extra += setPtr->sashSize;
        }
        if (panePtr->reqSize.flags & LIMITS_NOM_SET) {
            /*
             * This could be done more cleanly.  We want to ensure that the
             * requested nominal size is not overridden when determining
             * the normal sizes.  So temporarily fix min and max to the
             * nominal size and reset them back later.
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
 *      Sets the normal sizes for each pane.  The pane size is the
 *      requested widget size plus an amount of padding.  In addition,
 *      adjust the min/max bounds of the pane depending upon the resize
 *      flags (whether the pane can be expanded or shrunk from its normal
 *      size).
 *
 * Results:
 *      Returns the total space needed for the all the panes.
 *
 * Side Effects:
 *      The nominal size of each pane is set.  This is later used to
 *      determine how to shrink or grow the table if the container can't be
 *      resized to accommodate the exact size requirements of all the
 *      panes.
 *
 *---------------------------------------------------------------------------
 */
static int
SetNominalSizes(Paneset *setPtr)
{
    Blt_ChainLink link;
    int total;

    total = 0;
    for (link = Blt_Chain_FirstLink(setPtr->panes); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;
        int extra;

        panePtr = Blt_Chain_GetValue(link);
        if (ISVERT(setPtr)) {
            extra = PADDING(panePtr->padY);
        } else {
            extra = PADDING(panePtr->padX);
        }
        if (panePtr->flags & SASH) {
            extra += setPtr->sashSize;
        }
        /*
         * Restore the real bounds after temporarily setting nominal size.
         * These values may have been set in ResetPanes to restrict the
         * size of the pane to the requested range.
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
         * If a pane can't be resized (to either expand or shrink), hold
         * its respective limit at its normal size.
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
 *      The sum of normal sizes set here will be used as the normal size
 *      for the container widget.
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
    for (link = Blt_Chain_FirstLink(setPtr->panes); link != NULL; link = next) {
        Pane *panePtr;
        int width, height;

        next = Blt_Chain_NextLink(link);
        panePtr = Blt_Chain_GetValue(link);
        panePtr->flags &= ~SASH;
        if (panePtr->flags & HIDDEN) {
            if (Tk_IsMapped(panePtr->tkwin)) {
                Tk_UnmapWindow(panePtr->tkwin);
            }
            if (Tk_IsMapped(panePtr->sash)) {
                Tk_UnmapWindow(panePtr->sash);
            }
            continue;
        }
        if ((next != NULL) || (setPtr->mode == MODE_SPREADSHEET)) {
            /* Add the size of the sash to the pane. */
            /* width += setPtr->sashSize; */
            if (panePtr->flags & SHOW_SASH) {
                panePtr->flags |= SASH;
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
    for (link = Blt_Chain_FirstLink(setPtr->panes); link != NULL;
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
 *      The sum of normal sizes set here will be used as the normal size
 *      for the container widget.
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
    for (link = Blt_Chain_FirstLink(setPtr->panes); link != NULL; link = next) {
        Pane *panePtr;
        int width, height;
        
        next = Blt_Chain_NextLink(link);
        panePtr = Blt_Chain_GetValue(link);
        if (panePtr->flags & HIDDEN) {
            if (Tk_IsMapped(panePtr->tkwin)) {
                Tk_UnmapWindow(panePtr->tkwin);
            }
            if (Tk_IsMapped(panePtr->sash)) {
                Tk_UnmapWindow(panePtr->sash);
            }
            continue;
        }
        panePtr->flags &= ~SASH;
        if ((next != NULL) || (setPtr->mode == MODE_SPREADSHEET)) {
            /* height += setPtr->sashSize; */
            if (panePtr->flags & SHOW_SASH) {
                panePtr->flags |= SASH;
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
    for (link = Blt_Chain_FirstLink(setPtr->panes); link != NULL;
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
        if (panePtr->flags & SASH) {
            if (ISVERT(setPtr)) {
                cavityHeight -= setPtr->sashSize;
                if (setPtr->side & SASH_FARSIDE) {
                    yMax -= setPtr->sashSize;
                } else {
                    y += setPtr->sashSize;
                }
            } else {
                cavityWidth -= setPtr->sashSize;
                if (setPtr->side & SASH_FARSIDE) {
                    xMax -= setPtr->sashSize;
                } else {
                    x += setPtr->sashSize;
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
         * Compare the widget's requested size to the size of the cavity.
         *
         * 1) If the widget is larger than the cavity or if the fill flag
         *    is set, make the widget the size of the cavity. Check that
         *    the new size is within the bounds set for the widget.
         *
         * 2) Otherwise, position the widget in the space according to its
         *    anchor.
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
ArrangeSash(Pane *panePtr, int x, int y) 
{
    Paneset *setPtr;

    setPtr = panePtr->setPtr;
    if (panePtr->flags & SASH) {
        int w, h;
        
        if (ISVERT(setPtr)) {
            x = 0;
            if (setPtr->side & SASH_FARSIDE) {
                y += panePtr->size - setPtr->sashSize;
            }
            w = Tk_Width(setPtr->tkwin);
            h = setPtr->sashSize; 
        } else {
            y = 0;
            if (setPtr->side & SASH_FARSIDE) {
                x += panePtr->size - setPtr->sashSize;
            } 
            h = Tk_Height(setPtr->tkwin);
            w = setPtr->sashSize; 
        }
        if ((x != Tk_X(panePtr->tkwin)) || 
            (y != Tk_Y(panePtr->tkwin)) ||
            (w != Tk_Width(panePtr->tkwin)) ||
            (h != Tk_Height(panePtr->tkwin))) {
            Tk_MoveResizeWindow(panePtr->sash, x, y, w, h);
        }
        if (!Tk_IsMapped(panePtr->sash)) {
            Tk_MapWindow(panePtr->sash);
        }
        XRaiseWindow(setPtr->display, Tk_WindowId(panePtr->sash));
    } else if (Tk_IsMapped(panePtr->sash)) {
        Tk_UnmapWindow(panePtr->sash);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ArrangePane
 *
 *      Places each window at its proper location.  First determines the
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
    ArrangeSash(panePtr, x, y);
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
    int padY;
    Pane *panePtr;

    /*
     * If the paneset has no children anymore, then don't do anything at all:
     * just leave the container widget's size as-is.
     */
#if TRACE
    fprintf(stderr, "VerticalPanes\n");
#endif
    panePtr = LastPane(setPtr, HIDDEN);
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
     * Save the width and height of the container so we know when its size
     * has changed during ConfigureNotify events.
     */
    top = LeftSpan(setPtr);
    bottom = RightSpan(setPtr);
    setPtr->worldWidth = height = top + bottom;

    padY = 2 * Tk_InternalBorderWidth(setPtr->tkwin);
    /*
     * If the previous geometry request was not fulfilled (i.e. the size of
     * the container is different from paneset space requirements), try to
     * adjust size of the panes to fit the widget.
     */
    {
        Pane *firstPtr, *lastPtr;
        Blt_Chain span;
        int dy;

        dy = setPtr->bearing - top;
        firstPtr = FirstPane(setPtr, HIDDEN);
        lastPtr = NextPane(setPtr->anchorPtr, HIDDEN);
        if (firstPtr != lastPtr) {
            span = SortedSpan(setPtr, firstPtr, lastPtr);
            if (dy > 0) {
                GrowSpan(span, dy);
            } else if (dy < 0) {
                ShrinkSpan(span, dy);
            }
            top = LeftSpan(setPtr) + padY;
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
        bottom = RightSpan(setPtr) + padY;
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
    for (panePtr = FirstPane(setPtr, HIDDEN); panePtr != NULL; 
         panePtr = NextPane(panePtr, HIDDEN)) {
        panePtr->y = y;
        ArrangePane(panePtr, 0, y);
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
    int padX, padY;
    Pane *panePtr;

    /*
     * If the paneset has no children anymore, then don't do anything at all:
     * just leave the container widget's size as-is.
     */
    panePtr = LastPane(setPtr, HIDDEN);
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
     * Save the width and height of the container so we know when its size
     * has changed during ConfigureNotify events.
     */
    padX = padY = 2 * Tk_InternalBorderWidth(setPtr->tkwin);

    left = LeftSpan(setPtr);
    right = RightSpan(setPtr);
    setPtr->worldWidth = width = left + right;
    
    /*
     * If the previous geometry request was not fulfilled (i.e. the size of
     * the paneset is different from the total panes space requirements),
     * try to adjust size of the panes to fit the widget.
     */
    {
        Pane *firstPtr, *lastPtr;
        Blt_Chain span;
        int dx;

        dx = setPtr->bearing - left;
        firstPtr = FirstPane(setPtr, HIDDEN);
        lastPtr = NextPane(setPtr->anchorPtr, HIDDEN);
        if (firstPtr != lastPtr) {
            span = SortedSpan(setPtr, firstPtr, lastPtr);
            if (dx > 0) {
                GrowSpan(span, dx);
            } else if (dx < 0) {
                ShrinkSpan(span, dx);
            }
            left = LeftSpan(setPtr) + padX;
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
        right = RightSpan(setPtr) + padX;
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
    for (panePtr = FirstPane(setPtr, HIDDEN); panePtr != NULL; 
         panePtr = NextPane(panePtr, HIDDEN)) {
        panePtr->x = x;
        ArrangePane(panePtr, x, 0);
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
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    setPtr->sashSize = PADDING(setPtr->sashPad) + setPtr->sashThickness;
    gcMask = 0;
    newGC = Tk_GetGC(setPtr->tkwin, gcMask, &gcValues);
    if (setPtr->gc != NULL) {
        Tk_FreeGC(setPtr->display, setPtr->gc);
    }
    setPtr->gc = newGC;
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
    for (panePtr = FirstPane(setPtr, HIDDEN); panePtr != NULL; 
         panePtr = NextPane(panePtr, HIDDEN)) {
        panePtr->nom = panePtr->size;
    }
    return setPtr->bearing - oldBearing;
}

static void
AdjustSash(Paneset *setPtr, int delta)
{
    Pane *panePtr;

    delta = AdjustPanesetDelta(setPtr, delta);
    for (panePtr = FirstPane(setPtr, HIDDEN); panePtr != NULL; 
         panePtr = NextPane(panePtr, HIDDEN)) {
        panePtr->nom = panePtr->size;
    }
    switch (setPtr->mode) {
    case MODE_GIVETAKE:
        {
            Pane *leftPtr, *rightPtr;
            
            leftPtr = setPtr->anchorPtr;
            rightPtr = NextPane(leftPtr, HIDDEN);
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
            rightPtr = NextPane(leftPtr, HIDDEN);
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
    for (panePtr = FirstPane(setPtr, HIDDEN); panePtr != NULL; 
         panePtr = NextPane(panePtr, HIDDEN)) {
        panePtr->nom = panePtr->size;
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
 *      pathName add ?label? ?option value...?
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
    Blt_Chain_AppendLink(setPtr->panes, panePtr->link);
    if (Blt_ConfigureWidgetFromObj(interp, panePtr->sash, paneSpecs,
        objc - 2, objv + 2, (char *)panePtr, 0) != TCL_OK) {
        DestroyPane(panePtr);
        return TCL_ERROR;
    }
    EventuallyRedraw(setPtr);
    setPtr->flags |= LAYOUT_PENDING;
    Tcl_SetStringObj(Tcl_GetObjResult(interp), panePtr->name, -1);
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
 *      pathName cget option
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, setPtr->tkwin, paneSetSpecs,
        (char *)setPtr, objv[2], 0);
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
 *      pathName configure option value
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;

    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, setPtr->tkwin, paneSetSpecs,
                (char *)setPtr, (Tcl_Obj *)NULL, BLT_CONFIG_OBJV_ONLY);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, setPtr->tkwin, paneSetSpecs,
                (char *)setPtr, objv[2], BLT_CONFIG_OBJV_ONLY);
    }
    if (Blt_ConfigureWidgetFromObj(interp, setPtr->tkwin, paneSetSpecs,
        objc - 2, objv + 2, (char *)setPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
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
 *      pathName delete widget
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
 *      pathName insert how whereName ?label? ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InsertOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr, *relPtr;
    InsertOrder order;
    const char *name;
        
    if (GetInsertOrder(interp, objv[2], &order) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetPaneFromObj(interp, setPtr, objv[3], &relPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    name = NULL;
    if (objc > 4) {
        const char *string;

        string = Tcl_GetString(objv[4]);
        if (string[0] != '-') {
            if (GetPaneFromObj(NULL, setPtr, objv[4], &panePtr) == TCL_OK) {
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
    MovePane(setPtr, panePtr, order, relPtr);
    EventuallyRedraw(setPtr);

    if (Blt_ConfigureWidgetFromObj(interp, panePtr->sash, paneSpecs,
        objc - 4, objv + 4, (char *)panePtr, 0) != TCL_OK) {
        DestroyPane(panePtr);
        return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), panePtr->name, -1);
    return TCL_OK;

}

/*
 *---------------------------------------------------------------------------
 *
 * MoveOp --
 *
 *      Moves a pane to a new location.
 *
 *      pathName move before whereName paneName 
 *      pathName move after  whereName paneName
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MoveOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;
    Pane *panePtr, *relPtr;
    InsertOrder order;

    if (GetInsertOrder(interp, objv[2], &order) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetPaneFromObj(interp, setPtr, objv[3], &relPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetPaneFromObj(interp, setPtr, objv[4], &panePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((panePtr == NULL) || (panePtr->flags & DISABLED) ||
        (panePtr == relPtr)) {
        return TCL_OK;
    }
    MovePane(setPtr, panePtr, order, relPtr);
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *        pathName names ?pattern ...?
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
    Blt_ChainLink link;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    for (link = Blt_Chain_FirstLink(setPtr->panes); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Pane *panePtr;
        int i;
        int match;
        
        panePtr = Blt_Chain_GetValue(link);
        match = (objc == 2);
        for (i = 2; i < objc; i++) {
            if (Tcl_StringMatch(panePtr->name, Tcl_GetString(objv[i]))) {
                match = TRUE;
                break;
            }
        }
        if (match) {
            Tcl_Obj *objPtr;
            
            objPtr = Tcl_NewStringObj(panePtr->name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
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
 *      pathName pane cget pane option
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
    return Blt_ConfigureValueFromObj(interp, setPtr->tkwin, paneSpecs,
        (char *)panePtr, objv[4], 0);
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
 *      pathName pane configure pane option value
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
        return Blt_ConfigureInfoFromObj(interp, panePtr->sash, paneSpecs,
                (char *)panePtr, (Tcl_Obj *)NULL,0);
    } else if (objc == 5) {
        if (GetPaneFromObj(interp, setPtr, objv[3], &panePtr) != TCL_OK) {
            return TCL_ERROR;
        }
        return Blt_ConfigureInfoFromObj(interp, panePtr->sash, paneSpecs,
                (char *)panePtr, objv[4], 0);
    }
    if (GetPaneIterator(interp, setPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (panePtr = FirstTaggedPane(&iter); panePtr != NULL; 
         panePtr = NextTaggedPane(&iter)) {
        if (Blt_ConfigureWidgetFromObj(interp, panePtr->sash, paneSpecs,
                objc - 4, objv + 4, (char *)panePtr, BLT_CONFIG_OBJV_ONLY)
                != TCL_OK) {
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
 * SashActivateOp --
 *
 *      Changes the cursor and schedules to redraw the sash in its
 *      activate state (different relief, colors, etc).
 *
 *      pathName sash activate paneName 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SashActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
            EventuallyRedrawSash(setPtr->activePtr);
        }
        if (panePtr != NULL) {
            EventuallyRedrawSash(panePtr);
        }
        setPtr->activePtr = panePtr;
        vert = ISVERT(setPtr);
        if (setPtr->cursor != None) {
            cursor = setPtr->cursor;
        } else if (vert) {
            cursor = setPtr->defVertCursor;
        } else {
            cursor = setPtr->defHorzCursor;
        }
        Tk_DefineCursor(panePtr->sash, cursor);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SashAnchorOp --
 *
 *      Set the anchor for the resize/moving the pane.  Only one of the x
 *      and y coordinates are used depending upon the orientation of the
 *      pane.
 *
 *      pathName sash anchor paneName x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SashAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
    setPtr->flags |= SASH_ACTIVE;
    vert = ISVERT(setPtr);
    if (vert) {
        setPtr->bearing = ScreenY(panePtr);
        setPtr->sashAnchor = y;
    } else {
        setPtr->bearing = ScreenX(panePtr);
        setPtr->sashAnchor = x;
    } 
    setPtr->bearing += panePtr->size;
    AdjustSash(setPtr, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SashDeactivateOp --
 *
 *      Changes the cursor and schedules to redraw the sash in its
 *      inactivate state (different relief, colors, etc).
 *
 *      pathName sash deactivate 
 *      pathName sash deactivate 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SashDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    Paneset *setPtr = clientData;

    if (setPtr->activePtr != NULL) {
        EventuallyRedrawSash(setPtr->activePtr);
        setPtr->activePtr = NULL;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SashMarkOp --
 *
 *      Sets the current mark for moving the sash.  The change between
 *      the mark and the anchor is the amount to move the sash from its
 *      previous location.
 *
 *      pathName sash mark paneName x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SashMarkOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Pane *panePtr;
    Paneset *setPtr = clientData;
    int x, y;                           /* Root coordinates of the pointer
                                         * over the sash. */
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
    delta = mark - setPtr->sashAnchor;
    AdjustSash(setPtr, delta);
    setPtr->sashAnchor = mark;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SashMoveOp --
 *
 *      Moves the sash.  The sash is moved the given distance from its
 *      previous location.
 *
 *      pathName sash move paneName x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SashMoveOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
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
    AdjustSash(setPtr, delta);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SashSetOp --
 *
 *      Sets the location of the sash to coordinate (x or y) specified.
 *      The windows are move and/or arranged according to the mode.
 *
 *      pathName sash set paneName x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SashSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
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
    setPtr->flags &= ~SASH_ACTIVE;
    vert = ISVERT(setPtr);
    mark = (vert) ? y : x;
    delta = mark - setPtr->sashAnchor;
    AdjustSash(setPtr, delta);
    setPtr->sashAnchor = mark;
    return TCL_OK;
}

static Blt_OpSpec sashOps[] = {
    {"activate",   2, SashActivateOp,   4, 4, "paneName"},
    {"anchor",     2, SashAnchorOp,     6, 6, "paneName x y"},
    {"deactivate", 1, SashDeactivateOp, 3, 3, ""},
    {"mark",       2, SashMarkOp,       6, 6, "paneName x y"},
    {"move",       2, SashMoveOp,       6, 6, "paneName x y"},
    {"set",        1, SashSetOp,        6, 6, "paneName x y"},
};

static int numSashOps = sizeof(sashOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * SashOp --
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
SashOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numSashOps, sashOps, BLT_OP_ARG2, 
                            objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * TagAddOp --
 *
 *      pathName tag add tagName ?paneName ...?
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
 *      pathName tag delete tagName ?paneName ...?
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
 *      pathName tag exists paneName ?tag ...?
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
 *      pathName tag forget ?tag ...?
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
 *      pathName tag get paneName ?pattern ...?
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
 *      pathName tag names ?paneName ...?
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
 *      pathName tag indices ?tag ...?
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
 *      pathName tag set paneName ?tag ...?
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
 *      Removes one or more tags from a given pane. If a tag doesn't exist
 *      or is a reserved tag ("all"), nothing will be done and no error
 *      message will be returned.
 *
 *      pathName tag unset paneName ?tag ...?
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
    {"add",     1, TagAddOp,      2, 0, "?paneName? ?option value ...?",},
    {"delete",  1, TagDeleteOp,   2, 0, "?paneName ...?",},
    {"exists",  1, TagExistsOp,   4, 0, "paneName ?tag ...?",},
    {"forget",  1, TagForgetOp,   3, 0, "?tag ...?",},
    {"get",     1, TagGetOp,      4, 0, "paneName ?pattern ...?",},
    {"indices", 1, TagIndicesOp,  3, 0, "?tag...?",},
    {"names",   1, TagNamesOp,    3, 0, "?paneName ...?",},
    {"set",     1, TagSetOp,      4, 0, "paneName ?tag ...?",},
    {"unset",   1, TagUnsetOp,    4, 0, "paneName ?tag ...?",},
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


/*
 *---------------------------------------------------------------------------
 *
 * PanesetInstCmdDeleteProc --
 *
 *      This procedure is invoked when a widget command is deleted.  If the
 *      widget isn't already in the process of being destroyed, this
 *      command destroys it.
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
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin is
     * NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
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
 *      This procedure is invoked to process the TCL command that
 *      corresponds to the paneset geometry manager.  See the user
 *      documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

static Blt_OpSpec panesetOps[] =
{
    {"add",        1, AddOp,       2, 0, "?name? ?option value?...",},
    {"cget",       2, CgetOp,      3, 3, "option",},
    {"configure",  2, ConfigureOp, 2, 0, "?option value?",},
    {"delete",     1, DeleteOp,    3, 3, "?paneName ...?",},
    {"exists",     1, ExistsOp,    3, 3, "paneName",},
    {"index",      3, IndexOp,     3, 3, "paneName",},
    {"insert",     3, InsertOp,    4, 0, "after|before whereName ?label? ?option value ...?",},
    {"move",       1, MoveOp,      4, 0, "after|before whereName paneName",},
    {"names",      1, NamesOp,     2, 0, "?pattern...?",},
    {"pane",       1, PaneOp,      2, 0, "oper ?args?",},
    {"sash",       1, SashOp,      2, 0, "oper ?args?",},
    {"tag",        1, TagOp,       2, 0, "oper args",},
};

static int numPanesetOps = sizeof(panesetOps) / sizeof(Blt_OpSpec);

static int
PanesetInstCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numPanesetOps, panesetOps, BLT_OP_ARG1, 
        objc, objv, 0);
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
 *      This procedure is invoked to process the TCL command that
 *      corresponds to a widget managed by this module. See the user
 *      documentation for details on what it does.
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
    ClientData clientData,              /* Not used. */
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
        static const  char cmd[] = {
            "source [file join $blt_library bltPaneset.tcl]\n"
        };
        if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
            char info[200];
            
            Blt_FormatString(info, 200,
                             "\n    (while loading bindings for %.50s)", 
                      Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            return TCL_ERROR;
        }
    }
    setPtr = NewPaneset(interp, objv[1], PANESET);
    if (setPtr == NULL) {
        goto error;
    }
    if (Blt_ConfigureWidgetFromObj(interp, setPtr->tkwin, paneSetSpecs,
        objc - 2, objv + 2, (char *)setPtr, 0) != TCL_OK) {
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
        { "paneset",   PanesetCmd }
    };
    return Blt_InitCmds(interp, "::blt", cmdSpecs, 1);
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
    setPtr->numVisible = Blt_Chain_GetLength(setPtr->panes);
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
 * DisplaySashProc
 *
 *      Draws the pane's sash at its proper location.  First determines the
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
DisplaySashProc(ClientData clientData)
{
    Pane *panePtr = clientData;
    Blt_Bg bg;
    int relief;
    Paneset *setPtr;
    Drawable drawable;
    int w, h;
    
    panePtr->flags &= ~REDRAW_PENDING;
    if (panePtr->sash == NULL) {
        return;
    }
    setPtr = panePtr->setPtr;
    if (panePtr->flags & DISABLED) {
        bg = GETATTR(panePtr, disabledSashBg);
        relief = setPtr->relief;
    } else if (setPtr->activePtr == panePtr) {
        bg = GETATTR(panePtr, activeSashBg);
        relief = setPtr->activeRelief;
    } else {
        bg = GETATTR(panePtr, sashBg);
        relief = setPtr->relief;
    }
    drawable = Tk_WindowId(panePtr->sash);
    w = Tk_Width(panePtr->sash);
    h = Tk_Height(panePtr->sash);
    if ((w > 0) && (h > 0)) {
        Blt_Bg_FillRectangle(panePtr->sash, drawable, bg, 0, 0, w, h,
                                0, TK_RELIEF_FLAT);
        if (relief != TK_RELIEF_FLAT) {
            w -= PADDING(setPtr->sashPad);
            h -= PADDING(setPtr->sashPad);
            if ((w > 0) && (h > 0)) {
                Blt_Bg_DrawRectangle(panePtr->sash, drawable, bg, 
                        setPtr->sashPad.side1, setPtr->sashPad.side1, 
                        w, h, setPtr->sashBorderWidth, relief);
            }
        }
    }
}

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltDrawerset.c --
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
#define CHOOSE(a1,a2)           (((a1) != NULL) ? (a1) : (a2))
#define VPORTWIDTH(s) \
    (ISVERT(s)) ? Tk_Height((s)->tkwin) : Tk_Width((s)->tkwin);

#define TRACE   0

#define ISVERT(s)       ((s)->flags & VERTICAL)
#define ISHORIZ(s)      (((s)->flags & VERTICAL) == 0)

#define MOTION_MASK     (MOTION_LINEAR|MOTION_LOG)

typedef struct _Drawerset Drawerset;
typedef struct _Drawer Drawer;
typedef struct _Handle Handle;
typedef int (LimitsProc)(int value, Blt_Limits *limitsPtr);
typedef int (SizeProc)(Drawer *drawPtr);

typedef enum {
    INSERT_AFTER,                       /* Insert after named pane. */
    INSERT_BEFORE                       /* Insert before named pane. */
} InsertOrder;

/*
 * Default values for widget attributes.
 */
#define DEF_ACTIVE_HANDLE_COLOR   STD_ACTIVE_BACKGROUND
#define DEF_ACTIVE_HANDLE_RELIEF  "flat"
#define DEF_ANIMATE             "1"
#define DEF_AUTORAISE           "0"
#define DEF_BACKGROUND          STD_NORMAL_BACKGROUND
#define DEF_DELAY               "20"
#define DEF_HCURSOR             "sb_h_double_arrow"
#define DEF_HEIGHT              "0"
#define DEF_PAD                 "0"
#define DEF_SIDE                "right"
#define DEF_TAKEFOCUS           "1"
#define DEF_VCURSOR             "sb_v_double_arrow"
#define DEF_WIDTH               "0"

#define DEF_DRAWER_ANCHOR       "c"
#define DEF_DRAWER_BORDERWIDTH  "0"
#define DEF_DRAWER_CLOSEVALUE   "0"
#define DEF_DRAWER_CLOSE_COMMAND (char *)NULL
#define DEF_DRAWER_DATA         (char *)NULL
#define DEF_DRAWER_DELETE_COMMAND (char *)NULL
#define DEF_DRAWER_FILL         "1"
#define DEF_DRAWER_HIDE         "0"
#define DEF_DRAWER_OPENVALUE    "1"
#define DEF_DRAWER_OPEN_COMMAND (char *)NULL
#define DEF_DRAWER_PAD          "0"
#define DEF_DRAWER_PADX         "0"
#define DEF_DRAWER_PADY         "0"
#define DEF_DRAWER_SCALE         "log"
#define DEF_DRAWER_SHOW_HANDLE   "1"
#define DEF_DRAWER_RESIZE       "1"
#define DEF_DRAWER_STEPS        "8"
#define DEF_DRAWER_TAGS         (char *)NULL
#define DEF_DRAWER_VARIABLE     (char *)NULL

#define DEF_HANDLE_BORDERWIDTH   "1"
#define DEF_HANDLE_COLOR         STD_NORMAL_BACKGROUND
#define DEF_HANDLE_CURSOR       (char *)NULL
#define DEF_HANDLE_PAD           "0"
#define DEF_HANDLE_RELIEF        "flat"
#define DEF_HANDLE_STATE         "normal"
#define DEF_HANDLE_THICKNESS     "2"

#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))
#define VAR_FLAGS (TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS)

/*
 * Handle --
 *
 *      A handle is the thin window abutting the child widget (drawer). It
 *      may be used to resize or move the drawer.
 */
struct _Handle {
    Drawer *drawPtr;                    /* Drawer to which this handle
                                         * belongs. */
    Tk_Window tkwin;                    /* Handle window to be managed. */
    Blt_HashEntry *hashPtr;             /* Pointer of this handle into the
                                         * draweset's hashtable of
                                         * handles. */
    Blt_Bg bg;
    Blt_Bg activeBg;
    Tk_Cursor cursor;                   /* X Cursor */
};

/*
 * Drawer --
 *
 *   A drawer contains two windows: the embeded child widget and a handle
 *   subwindow.  The drawer controls how the embedded window should appear
 *   in the drawer.  The handle is a rectanglar window on the far edge of
 *   the drawer (horizontal=right, vertical=bottom).  Normally the last
 *   drawer does not have a handle.  Handles may be hidden.
 *
 *   Note: There is no 3D border around the drawer.  This can be added by
 *         embedding a frame.  This simplifies the widget so that there is
 *         only one window for the widget.  Windows outside of the boundary
 *         of the drawer are occluded.
 */
struct _Drawer  {
    const char *name;                   /* Name of drawer */

    int index;                          /* Index of the drawer. */
    Drawerset *setPtr;                  /* Drawerset widget containing this
                                         * drawer. */
    Blt_HashEntry *hashPtr;             /* Pointer of this drawer into
                                         * hashtable of drawer. */
    Blt_ChainLink link;                 /* Pointer of this drawer into the
                                         * list of drawers. */
    unsigned int flags;
    unsigned int side;                  /* The side of the widget where the
                                         * drawer is attached. */
    Tk_Window tkwin;                    /* Embedded child widget to be
                                         * managed. */
    int extBorderWidth;                 /* The external border width of the
                                         * embedded widget. This is needed
                                         * to check if "border_width" of
                                         * Tk_Changes(tkwin) changes. */
    int numSteps;                       /* # of steps to transition between
                                         * open/closed. */
    int step;                           /* Current step in animation */

    int offset;                         /* Origin of drawer window wrt
                                         * container. */
    int scrollMin, scrollMax;           /* Minumum and maximum positions of
                                         * drawer. */
    const char *takeFocus;              /* Says whether to select this
                                         * widget during tab traveral
                                         * operations.  This value isn't
                                         * used in C code, but for the
                                         * widget's TCL bindings. */
    float relWidth, relHeight;
    Blt_Limits reqWidth, reqHeight;     /* Bounds for width and height
                                         * requests made by the widget. */

    Tk_Anchor anchor;                   /* Anchor type: indicates how the
                                         * widget is positioned if extra
                                         * space is available in the
                                         * drawer. */
    int fill;                           /* Indicates how the widget should
                                         * fill the drawer it occupies. */
    int size;                           /* Current size of the drawer. This
                                         * size is bounded by min and max
                                         * below. */
    /*
     * nom and size perform similar duties.  I need to keep track of the
     * amount of space allocated to the drawer (using size).  But at the
     * same time, I need to indicate that space can be parcelled out to
     * this drawer.  If a nominal size was set for this drawer, I don't
     * want to add space.
     */
    int nom;                            /* The nominal size (neither
                                         * expanded nor shrunk) of the
                                         * drawer based upon the requested
                                         * size of the widget embedded in
                                         * this drawer. */
    int min, max;                       /* Size constraints for the
                                         * drawer. */
    int normalSize;                     /* Requested size of the
                                         * drawer. This size is bounded by
                                         * -reqwidth or -reqheight. */
    Blt_Bg bg;                          /* 3D background border surrounding
                                         * the widget */
    Tcl_TimerToken timerToken;
    Tcl_Obj *varNameObjPtr;             /* Name of TCL variable.  If
                                         * non-NULL, this variable will be
                                         * set to the value string of the
                                         * selected item. */

    /* Open and close values. */
    Tcl_Obj *openValueObjPtr;           /* Open drawer value. */
    Tcl_Obj *closeValueObjPtr;          /* Close drawer value. */
    Tcl_Obj *openCmdObjPtr;             /* Command to be invoked whenever
                                         * the drawer has been opened. */
    Tcl_Obj *closeCmdObjPtr;            /* Command to be invoked whenever
                                         * the drawer has been closed. */
    Tcl_Obj *deleteCmdObjPtr;           /* If non-NULL, Routine to call
                                         * when drawer is deleted. */
    Tcl_Obj *dataObjPtr;
    Handle handle;
    int delay;                          /* Delay between steps of automated 
                                         * scrolling. */
};

/*
 * Drawerset structure
 *
 *      The drawerset is a set of windows (drawers) that may be resized in
 *      one dimension (horizontally or vertically).
 */
struct _Drawerset {
    int flags;                          /* See the flags definitions
                                         * below. */
    Display *display;                   /* Display of the widget. */
    Tk_Window tkwin;                    /* The container window into which
                                         * other widgets are arranged. */
    Tcl_Interp *interp;                 /* Interpreter associated with all
                                         * widgets and handles. */
    Tcl_Command cmdToken;               /* Command token associated with
                                         * this widget. */
    int borderWidth;                    /* The border width of the
                                         * widget. */
    int normalWidth;                    /* Normal dimensions of the
                                         * drawerset. */
    int normalHeight;
    int reqWidth, reqHeight;            /* Constraints on the drawerset's
                                         * normal width and
                                         * height. Overrides the requested
                                         * width of the window. */

    Tk_Cursor defVertCursor;            /* Default vertical X cursor */
    Tk_Cursor defHorzCursor;            /* Default horizontal X cursor */

    short int width, height;            /* Requested size of the widget. */

    Blt_Bg bg;                          /* 3D border surrounding the window
                                         * (viewport). */

    Tk_Window child;                    /* Window of base layer. */

    /* 
     * Automated scrolling information. 
     */
    int scrollUnits;                    /* Smallest unit of scrolling for
                                         * drawer. */
    int scrollIncr;                     /* Current increment. */
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

    Blt_Chain drawers;                  /* List of drawers. Represents the
                                         * stacking order of the
                                         * drawers. */

    Blt_HashTable drawerTable;          /* Table of drawers.  Serves as a
                                         * directory to look up drawers
                                         * from their windows. */
    Blt_HashTable handleTable;          /* Table of handle path name.
                                         * Serves as look up for drawers
                                         * from their handle windows. */
    struct _Blt_Tags tags;              /* Table of tags. */
    Drawer *activePtr;                  /* Indicates the drawer with the
                                         * active handle. */
    size_t numVisible;                  /* # of visible drawers. */
    GC gc;
    size_t nextId;                      /* Counter to generate unique
                                         * drawer names. */
    size_t nextHandleId;                /* Counter to generate unique
                                         * drawer names. */
    Tcl_Obj *baseObjPtr;                /* Child window at the base of the
                                         * drawerset. */
    Tk_Window base;                     /* Child window at the base of the
                                         * drawerset. */
};


/*
 * Drawerset flags definitions
 */
#define REDRAW_PENDING  (1<<0)          /* A redraw request is pending. */
#define LAYOUT_PENDING  (1<<1)          /* Get the requested sizes of the
                                         * widgets before
                                         * expanding/shrinking the size of
                                         * the container.  It's necessary
                                         * to recompute the layout every
                                         * time a drawer is added,
                                         * reconfigured, or deleted, but
                                         * not when the container is
                                         * resized. */
#define WARD_PENDING   (1<<2)          /* A request to install a new
                                        * embedded widget is pending.  */
#define ANIMATE         (1<<3)          /* Animate drawer moves and drawer
                                         * open/closes */

#define AUTORAISE       (1<<4)
#define FOCUS           (1<<5)

#define VERTICAL        (1<<6)
#define RESTACK         (1<<7)

/* Drawer/handle flags.  */

#define CLOSING         (1<<20)
#define HIDDEN          (1<<9)          /* Do not display the handle or
                                         * drawer. */
#define CLOSED          (1<<9)          /* Do not display the drawer. */
#define DISABLED        (1<<10)         /* Handle is disabled. Can't move
                                         * drawer by dragging handle. */
#define ONSCREEN        (1<<11)         /* Drawer is on-screen. */
#define HANDLE_ACTIVE   (1<<12)         /* Handle is currently active. */
#define SHOW_HANDLE     (1<<14)         /* Display the drawer. */
#define RESIZE          (1<<15)         /* Resize the window to fit the
                                         * drawer, instead of moving it. */
#define VIRGIN          (1<<24)
#define MOTION_LINEAR   (1<<21)
#define MOTION_LOG      (1<<22)

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

static Tk_GeomRequestProc BaseGeometryProc;
static Tk_GeomLostSlaveProc BaseCustodyProc;
static Tk_GeomMgr baseMgrInfo =
{
    (char *)"drawerset",                /* Name of geometry manager used by
                                         * winfo */
    BaseGeometryProc,                   /* Procedure to for new geometry
                                         * requests */
    BaseCustodyProc,                    /* Procedure when widget is taken
                                         * away */
};

static Tk_GeomRequestProc DrawerGeometryProc;
static Tk_GeomLostSlaveProc DrawerCustodyProc;
static Tk_GeomMgr drawerMgrInfo =
{
    (char *)"drawer",                   /* Name of geometry manager used by
                                         * winfo */
    DrawerGeometryProc,                 /* Procedure to for new geometry
                                         * requests */
    DrawerCustodyProc,                  /* Procedure when widget is taken
                                         * away */
};

static Blt_OptionParseProc ObjToMotionScaling;
static Blt_OptionPrintProc MotionScalingToObj;
static Blt_CustomOption motionScalingOption =
{
    ObjToMotionScaling, MotionScalingToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToChild;
static Blt_OptionPrintProc ChildToObj;
static Blt_CustomOption childOption = {
    ObjToChild, ChildToObj, NULL, (ClientData)0,
};

extern Blt_CustomOption bltLimitsOption;

static Blt_OptionFreeProc FreeTraceVarProc;
static Blt_OptionParseProc ObjToTraceVar;
static Blt_OptionPrintProc TraceVarToObj;
static Blt_CustomOption traceVarOption = {
    ObjToTraceVar, TraceVarToObj, FreeTraceVarProc, (ClientData)0
};

static Blt_OptionFreeProc FreeTagsProc;
static Blt_OptionParseProc ObjToTags;
static Blt_OptionPrintProc TagsToObj;
static Blt_CustomOption tagsOption = {
    ObjToTags, TagsToObj, FreeTagsProc, (ClientData)0
};

static Blt_OptionParseProc ObjToStateProc;
static Blt_OptionPrintProc StateToObj;
static Blt_CustomOption stateOption = {
    ObjToStateProc, StateToObj, NULL, (ClientData)0
};

static Blt_ConfigSpec drawerSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activehandlecolor", "activeHandleColor", 
        "HandleColor", DEF_ACTIVE_HANDLE_COLOR,
        Blt_Offset(Drawer, handle.activeBg), 
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_ANCHOR, "-anchor", (char *)NULL, (char *)NULL, 
        DEF_DRAWER_ANCHOR, Blt_Offset(Drawer, anchor), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        (char *)NULL, Blt_Offset(Drawer, bg), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_OBJ, "-closecommand", "closeCommand", "CloseCommand", 
        DEF_DRAWER_CLOSE_COMMAND, Blt_Offset(Drawer, closeCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-closevalue", "closeValue", "CloseValue",
        DEF_DRAWER_CLOSEVALUE, Blt_Offset(Drawer, closeValueObjPtr), 
        BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_OBJ, "-data", "data", "Data", DEF_DRAWER_DATA, 
        Blt_Offset(Drawer, dataObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_INT_NNEG, "-delay", "delay", "Delay", DEF_DELAY,
        Blt_Offset(Drawer, delay), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-deletecommand", "deleteCommand", "DeleteCommand",
        DEF_DRAWER_DELETE_COMMAND, Blt_Offset(Drawer, deleteCmdObjPtr),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FILL, "-fill", "fill", "Fill", DEF_DRAWER_FILL, 
        Blt_Offset(Drawer, fill), BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_BACKGROUND, "-handlecolor", "handleColor", "HandleColor",
        DEF_HANDLE_COLOR, Blt_Offset(Drawer, handle.bg), 
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CURSOR, "-handlecursor", "handleCursor", "HandleCursor",
        DEF_HANDLE_CURSOR, Blt_Offset(Drawer, handle.cursor),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-height", "height", "Height", (char *)NULL, 
        Blt_Offset(Drawer, reqHeight), 0, &bltLimitsOption},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_DRAWER_HIDE, 
        Blt_Offset(Drawer, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDDEN },
    {BLT_CONFIG_OBJ, "-opencommand", "openCommand", "OpenCommand", 
        DEF_DRAWER_OPEN_COMMAND, Blt_Offset(Drawer, openCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-openvalue", "openValue", "OpenValue", 
        DEF_DRAWER_OPENVALUE, Blt_Offset(Drawer, openValueObjPtr), 
        BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_BITMASK, "-resize", "resize", "resize", 
        DEF_DRAWER_RESIZE, Blt_Offset(Drawer, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)RESIZE },
    {BLT_CONFIG_CUSTOM, "-scale", "scale", "Scale",
        DEF_DRAWER_SCALE, Blt_Offset(Drawer, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, &motionScalingOption},
    {BLT_CONFIG_BITMASK, "-showhandle", "showHandle", "showHandle", 
        DEF_DRAWER_SHOW_HANDLE, Blt_Offset(Drawer, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SHOW_HANDLE },
    {BLT_CONFIG_SIDE, "-side", (char *)NULL, (char *)NULL, DEF_SIDE, 
        Blt_Offset(Drawer, side), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_HANDLE_STATE, 
        Blt_Offset(Drawer, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        &stateOption},
    {BLT_CONFIG_INT_POS, "-steps", "steps", "Steps", DEF_DRAWER_STEPS,
        Blt_Offset(Drawer, numSteps), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_DRAWER_TAGS, 0,
        BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
        DEF_TAKEFOCUS, Blt_Offset(Drawer, takeFocus), BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_CUSTOM, "-variable", (char *)NULL, (char *)NULL, 
        DEF_DRAWER_VARIABLE, Blt_Offset(Drawer, varNameObjPtr), 
        BLT_CONFIG_NULL_OK, &traceVarOption},
    {BLT_CONFIG_CUSTOM, "-width", "width", "Width", (char *)NULL, 
        Blt_Offset(Drawer, reqWidth), 0, &bltLimitsOption},
    {BLT_CONFIG_CUSTOM, "-window", "window", "Window", (char *)NULL, 
        Blt_Offset(Drawer, tkwin), BLT_CONFIG_NULL_OK, &childOption },
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec drawersetSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activehandlecolor", "activeHandleColor", 
        "HandleColor", DEF_ACTIVE_HANDLE_COLOR, 
        Blt_Offset(Drawerset, activeHandleBg), 0},
    {BLT_CONFIG_RELIEF, "-activehandlerelief", "activeHandleRelief", 
        "HandleRelief", DEF_ACTIVE_HANDLE_RELIEF, 
        Blt_Offset(Drawerset, activeRelief), BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_BITMASK, "-animate", "animate", "Animate", DEF_ANIMATE, 
        Blt_Offset(Drawerset, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)ANIMATE },
    {BLT_CONFIG_BITMASK, "-autoraise", "autoRaise", "AutoRaise", DEF_AUTORAISE, 
        Blt_Offset(Drawerset, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)AUTORAISE },
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BACKGROUND, Blt_Offset(Drawerset, bg), 0 },
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_PIXELS_NNEG, "-handleborderwidth", "handleBorderWidth", 
        "HandleBorderWidth", DEF_HANDLE_BORDERWIDTH, 
        Blt_Offset(Drawerset, handleBW), BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_BACKGROUND, "-handlecolor", "handleColor", "HandleColor",
        DEF_HANDLE_COLOR, Blt_Offset(Drawerset, handleBg), 0},
    {BLT_CONFIG_PAD, "-handlepad", "handlePad", "HandlePad", DEF_HANDLE_PAD, 
        Blt_Offset(Drawerset, handlePad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-handlerelief", "handleRelief", "HandleRelief", 
        DEF_HANDLE_RELIEF, Blt_Offset(Drawerset, relief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-handlethickness", "handleThickness", 
        "HandleThickness", DEF_HANDLE_THICKNESS, 
        Blt_Offset(Drawerset, handleThickness), BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_HEIGHT,
        Blt_Offset(Drawerset, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_CUSTOM, "-reqheight", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(Drawerset, reqHeight), 0, &bltLimitsOption},
    {BLT_CONFIG_CUSTOM, "-reqwidth", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(Drawerset, reqWidth), 0, &bltLimitsOption},
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_WIDTH,
        Blt_Offset(Drawerset, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_OBJ, "-window", "window", "Window", (char *)NULL, 
        Blt_Offset(Drawerset, baseObjPtr), BLT_CONFIG_NULL_OK },
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};


/*
 * DrawerIterator --
 *
 *   Drawers may be tagged with strings.  A drawer may have many tags.  The
 *   same tag may be used for many drawers.
 *      
 */
typedef enum { 
    ITER_SINGLE, ITER_ALL, ITER_TAG, ITER_PATTERN, 
} IteratorType;

typedef struct _Iterator {
    Drawerset *setPtr;                 /* The drawerset that we're
                                        * iterating over. */
    IteratorType type;                  /* Type of iteration:
                                         * ITER_TAG      By item tag.
                                         * ITER_ALL      By every item.
                                         * ITER_SINGLE   Single item: either 
                                         *               tag or index.
                                         * ITER_PATTERN  Over a consecutive 
                                         *               range of indices.
                                         */
    Drawer *startPtr;                   /* Starting drawer.  Starting point
                                         * of search, saved if iterator is
                                         * reused.  Used for ITER_ALL and
                                         * ITER_SINGLE searches. */
    Drawer *endPtr;                     /* Ending pend (inclusive). */
    Drawer *nextPtr;                    /* Next drawer. */

    /* For tag-based searches. */
    char *tagName;                      /* If non-NULL, is the tag that we
                                         * are currently iterating over. */
    Blt_HashSearch cursor;              /* Search iterator for tag hash
                                         * table. */
    Blt_ChainLink link;
} DrawerIterator;

/*
 * Forward declarations
 */
static Tcl_FreeProc DrawersetFreeProc;
static Tcl_IdleProc DisplayProc;
static Tcl_IdleProc DisplayHandle;
static Tcl_ObjCmdProc DrawersetCmdProc;
static Tk_EventProc DrawersetEventProc;
static Tk_EventProc DrawerEventProc;
static Tk_EventProc HandleEventProc;
static Tcl_FreeProc DrawerFreeProc;
static Tcl_ObjCmdProc DrawersetInstCmdProc;
static Tcl_CmdDeleteProc DrawersetInstCmdDeleteProc;
static Tcl_TimerProc DrawerTimerProc;
static Tcl_VarTraceProc DrawerVarTraceProc;

static int GetDrawerIterator(Tcl_Interp *interp, Drawerset *setPtr, 
        Tcl_Obj *objPtr, DrawerIterator *iterPtr);
static int GetDrawerFromObj(Tcl_Interp *interp, Drawerset *setPtr, 
        Tcl_Obj *objPtr, Drawer **drawPtrPtr);


static void
EventuallyRedraw(Drawerset *setPtr)
{
    if ((setPtr->flags & REDRAW_PENDING) == 0) {
        setPtr->flags |= REDRAW_PENDING;
        Tcl_DoWhenIdle(DisplayProc, setPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BaseEventProc --
 *
 *      This procedure is invoked by the Tk event handler when
 *      StructureNotify events occur in a widget managed by the drawerset.
 *
 *      For example, when a managed widget is destroyed, it frees the
 *      corresponding drawer structure and arranges for the drawerset
 *      layout to be re-computed at the next idle point.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      If the managed widget was deleted, the Drawer structure gets
 *      cleaned up and the drawerset is rearranged.
 *
 *---------------------------------------------------------------------------
 */
static void
BaseEventProc(
    ClientData clientData,              /* Pointer to Drawerset structure
                                         * for widget referred to by
                                         * eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    Drawerset *setPtr = clientData;

    if (eventPtr->type == ConfigureNotify) {
        EventuallyRedraw(setPtr);
    } else if (eventPtr->type == DestroyNotify) {
        setPtr->base = NULL;
        setPtr->flags |= LAYOUT_PENDING;
        EventuallyRedraw(setPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BaseCustodyProc --
 *
 *      This procedure is invoked when a widget has been stolen by another
 *      geometry manager.  The information and memory associated with the
 *      widget is released.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the drawerset to have its layout recomputed at the
 *      next idle point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
BaseCustodyProc(ClientData clientData, Tk_Window tkwin)
{
    Drawerset *setPtr = clientData;

    if (Tk_IsMapped(setPtr->base)) {
        Tk_UnmapWindow(setPtr->base);
    }
    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * BaseGeometryProc --
 *
 *      This procedure is invoked by Tk_GeometryRequest for widgets managed
 *      by the drawerset geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the drawerset to have its layout re-computed and
 *      re-arranged at the next idle point.
 *
 * ----------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
BaseGeometryProc(ClientData clientData, Tk_Window tkwin)
{
    Drawerset *setPtr = clientData;

    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
}

static void
UnmanageBase(Drawerset *setPtr, Tk_Window tkwin)
{
    Tk_DeleteEventHandler(tkwin, StructureNotifyMask, BaseEventProc, setPtr);
    Tk_ManageGeometry(tkwin, (Tk_GeomMgr *)NULL, setPtr);
    if (Tk_IsMapped(tkwin)) {
        Tk_UnmapWindow(tkwin);
    }
}

static void
ManageBase(Drawerset *setPtr, Tk_Window tkwin)
{
    setPtr->flags |= RESTACK;
    Tk_CreateEventHandler(tkwin, StructureNotifyMask, BaseEventProc, setPtr);
    Tk_ManageGeometry(tkwin, &baseMgrInfo, setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetWindowFromObj --
 *
 *      Convert the pathname of a Tk window into a Tk_Window.
 *
 * Results:
 *      The return value is a standard TCL result.  The window pointer is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GetWindowFromObj(
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Drawerset *setPtr,
    Tcl_Obj *objPtr,                    /* String representing Tk
                                         * window. */
    Tk_Window *tkwinPtr)
{
    Tk_Window tkwin;

    if (objPtr == NULL) {
        Tcl_AppendResult(interp, "window name is NULL", (char *)NULL);
        *tkwinPtr = NULL;
        return TCL_ERROR;
    }
    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objPtr), setPtr->tkwin);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    if (Tk_Parent(tkwin) != setPtr->tkwin) {
        Tcl_AppendResult(interp, "window \"", Tk_PathName(tkwin), 
                "\" is not a child of ", Tk_PathName(setPtr->tkwin),
                (char *)NULL);
        return TCL_ERROR;
    }
    *tkwinPtr = tkwin;
    return TCL_OK;
}

static void
InstallBase(ClientData clientData)
{
    Drawerset *setPtr = clientData;
    Tcl_Interp *interp;

    interp = setPtr->interp;
    if (setPtr->tkwin == NULL) {
        return;                         /* Widget has been destroyed. */
    }
    if (GetWindowFromObj(interp, setPtr, setPtr->baseObjPtr, &setPtr->base) 
        != TCL_OK) {
        Tcl_BackgroundError(interp);
        return;
    }
    ManageBase(setPtr, setPtr->base);
}

/*
 *---------------------------------------------------------------------------
 *
 * RaiseDrawer --
 *
 *      Raises the drawer to the top of the stack of drawers.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Drawer is redrawn at the top of the stack.
 *
 *---------------------------------------------------------------------------
 */
static void
RaiseDrawer(Drawer *drawPtr)
{
    if (drawPtr->link != NULL) {
        Drawerset *setPtr;

        setPtr = drawPtr->setPtr;
        Blt_Chain_UnlinkLink(setPtr->drawers, drawPtr->link);
        Blt_Chain_AppendLink(setPtr->drawers, drawPtr->link);
        setPtr->flags |= RESTACK;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * LowerDrawer --
 *
 *      Lowers the drawer to the bottom of the stack of drawers.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Drawer is redraw at the bottom of the stack.
 *
 *---------------------------------------------------------------------------
 */
static void
LowerDrawer(Drawer *drawPtr)
{
    if (drawPtr->link != NULL) {
        Drawerset *setPtr;

        setPtr = drawPtr->setPtr;
        Blt_Chain_UnlinkLink(setPtr->drawers, drawPtr->link);
        Blt_Chain_PrependLink(setPtr->drawers, drawPtr->link);
        setPtr->flags |= RESTACK;
    }
}

static INLINE void
SetNominal(int nom, Blt_Limits *limitsPtr)
{
    limitsPtr->nom = nom;
    limitsPtr->flags |= LIMITS_NOM_SET;
}

/*
 *---------------------------------------------------------------------------
 *
 * BoundSize --
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
BoundSize(int size, Blt_Limits *limitsPtr)    
{
    if (limitsPtr->flags & LIMITS_NOM_SET) {
        size = limitsPtr->nom;         /* Override initial value */
    }
    if (size < limitsPtr->min) {
        size = limitsPtr->min;         /* Bounded by minimum value */
    }
    if (size > limitsPtr->max) {
        size = limitsPtr->max;         /* Bounded by maximum value */
    }
    return size;
}

#ifdef notdef
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
BoundHeight(int height, Blt_Limits *limitsPtr)
{
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
#endif

static void
GetHorizontalDrawerGeometry(Drawerset *setPtr, Drawer *drawPtr, int *widthPtr,
                            int *heightPtr)
{
    int reqWidth, reqHeight, w, h;

    if (drawPtr->relWidth > 0.0) {
        reqWidth = (int)(Tk_Width(setPtr->tkwin) * drawPtr->relWidth);
    } else {
        reqWidth = Tk_ReqWidth(drawPtr->tkwin);
    }
    if (drawPtr->relHeight > 0.0) {
        reqHeight = (int)(Tk_Height(setPtr->tkwin) * drawPtr->relHeight);
    } else {
        reqHeight = Tk_ReqHeight(drawPtr->tkwin);
    }
    h = Tk_Height(setPtr->tkwin);
    w = Tk_Width(setPtr->tkwin);
    if (w < 2) {
        w = Tk_ReqWidth(setPtr->tkwin);
    }
    if (drawPtr->flags & SHOW_HANDLE) {
        w -= setPtr->handleSize;        /* Subtract the size of the handle
                                         * from the maximum window size. */
    }
    if ((reqWidth < w) && ((drawPtr->fill & FILL_X) == 0)) {
        w = reqWidth;
    }
    if ((reqHeight < h) && ((drawPtr->fill & FILL_Y) == 0)) {
        h = reqHeight;
    }
    w = BoundSize(w, &drawPtr->reqWidth);
    h = BoundSize(h, &drawPtr->reqHeight);
    /* Track limits of drawer. */
    if (w > Tk_Width(setPtr->tkwin)) {
        w = Tk_Width(setPtr->tkwin);
    }
    if (h > Tk_Height(setPtr->tkwin)) {
        h = Tk_Height(setPtr->tkwin);
    }
    drawPtr->scrollMin = 0;
    drawPtr->scrollMax = w;
    *widthPtr = w;
    *heightPtr = h;
}

static void
GetVerticalDrawerGeometry(Drawerset *setPtr, Drawer *drawPtr, int *widthPtr,
                          int *heightPtr)
{
    int reqWidth, reqHeight, w, h;

    if (drawPtr->relWidth > 0.0) {
        reqWidth = (int)(Tk_Width(setPtr->tkwin) * drawPtr->relWidth);
    } else {
        reqWidth = Tk_ReqWidth(drawPtr->tkwin);
    }
    if (drawPtr->relHeight > 0.0) {
        reqHeight = (int)(Tk_Height(setPtr->tkwin) * drawPtr->relHeight);
    } else {
        reqHeight = Tk_ReqHeight(drawPtr->tkwin);
    }
    h = Tk_Height(setPtr->tkwin);
    w  = Tk_Width(setPtr->tkwin);
    if (h < 2) {
        h = Tk_ReqHeight(setPtr->tkwin);
    }
    if (drawPtr->flags & SHOW_HANDLE) {
        h -= setPtr->handleSize;        /* Subtract the size of the handle
                                         * from the maximum window size. */
    }
    if ((reqHeight < h) && ((drawPtr->fill & FILL_Y) == 0)) {
        h = reqHeight;
    }
    if ((reqWidth < w) && ((drawPtr->fill & FILL_X) == 0)) {
        w = reqWidth;
    }
    w = BoundSize(w, &drawPtr->reqWidth);
    h = BoundSize(h, &drawPtr->reqHeight);
    if (w > Tk_Width(setPtr->tkwin)) {
        w = Tk_Width(setPtr->tkwin);
    }
    if (h > Tk_Height(setPtr->tkwin)) {
        h = Tk_Height(setPtr->tkwin);
    }
    /* Track limits of drawer. */
    drawPtr->scrollMin = 0;
    drawPtr->scrollMax = h;
    *widthPtr = w;
    *heightPtr = h;
    drawPtr->size = h;
}

static int
InvokeDrawerCommand(Tcl_Interp *interp, Drawer *drawPtr, Tcl_Obj *cmdObjPtr) 
{
    Tcl_Obj *objPtr;
    int result;

    cmdObjPtr = Tcl_DuplicateObj(cmdObjPtr);
    objPtr = Tcl_NewIntObj(drawPtr->index);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    return result;
}


static void
OpenDrawer(Drawer *drawPtr) 
{
    if (drawPtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(drawPtr->timerToken);
        drawPtr->timerToken = 0;
    }
    drawPtr->flags &= ~(CLOSED | CLOSING);
    drawPtr->step = 0;
    if (drawPtr->openCmdObjPtr != NULL) {
        int result;
        Drawerset *setPtr;

        setPtr = drawPtr->setPtr;
        result = InvokeDrawerCommand(setPtr->interp, drawPtr, 
                drawPtr->openCmdObjPtr);
        if (result != TCL_OK) {
            Tcl_BackgroundError(setPtr->interp);
        }
    }
}

static void
EventuallyOpenDrawer(Drawer *drawPtr) 
{
    Drawerset *setPtr;

    if ((drawPtr->flags & CLOSED) == 0) {
        return;                         /* Already open. */
    }
    setPtr = drawPtr->setPtr;
    drawPtr->flags &= ~(CLOSED | CLOSING);
    if ((setPtr->flags & AUTORAISE) && ((drawPtr->flags & DISABLED) == 0)) {
        RaiseDrawer(drawPtr);
    }
    drawPtr->step = 0;
    if (setPtr->flags & ANIMATE) {
        if (drawPtr->side & SIDE_VERTICAL) {
            if (setPtr->flags & LAYOUT_PENDING) {
                int w, h;
                GetVerticalDrawerGeometry(setPtr, drawPtr, &w, &h);
            } 
        } else {
            if (setPtr->flags & LAYOUT_PENDING) {
                int w, h;
                GetHorizontalDrawerGeometry(setPtr, drawPtr, &w, &h);
            } 
        }
        if (drawPtr->offset < drawPtr->scrollMin) {
            drawPtr->offset = drawPtr->scrollMin;
        }
        if (drawPtr->offset > drawPtr->scrollMax) {
            drawPtr->offset = drawPtr->scrollMax;
        }
        if (drawPtr->timerToken != (Tcl_TimerToken)0) {
            Tcl_DeleteTimerHandler(drawPtr->timerToken);
            drawPtr->timerToken = 0;
        }
        drawPtr->timerToken = Tcl_CreateTimerHandler(drawPtr->delay, 
                DrawerTimerProc, drawPtr);
    } else {
        drawPtr->offset = drawPtr->scrollMax;
    }
    EventuallyRedraw(setPtr);
}

static void
CloseDrawer(Drawer *drawPtr) 
{
    if (drawPtr->flags & CLOSED) {
        return;                         /* Already closed. */
    }
    if (drawPtr->tkwin != NULL) {
        if (Tk_IsMapped(drawPtr->tkwin)) {
            Tk_UnmapWindow(drawPtr->tkwin);
        }
    }
    if (Tk_IsMapped(drawPtr->handle.tkwin)) {
        Tk_UnmapWindow(drawPtr->handle.tkwin);
    }
    /* drawPtr->offset = -1; */
    drawPtr->step = 0;
    if (drawPtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(drawPtr->timerToken);
        drawPtr->timerToken = 0;
    }
    drawPtr->flags &= ~CLOSING;
    drawPtr->flags |= CLOSED;
    if (drawPtr->closeCmdObjPtr != NULL) {
        int result;
        Drawerset *setPtr;

        setPtr = drawPtr->setPtr;
        result = InvokeDrawerCommand(setPtr->interp, drawPtr, 
                                     drawPtr->closeCmdObjPtr);
        if (result != TCL_OK) {
            Tcl_BackgroundError(setPtr->interp);
        }
    }
}

static void
EventuallyCloseDrawer(Drawer *drawPtr) 
{
    Drawerset *setPtr;

    if (drawPtr->flags & CLOSED) {
        return;                         /* Already closed. */
    }
    setPtr = drawPtr->setPtr;
    if (setPtr->flags & ANIMATE) {
        drawPtr->step = 0;
        drawPtr->flags |= CLOSING;
        if (drawPtr->timerToken != (Tcl_TimerToken)0) {
            Tcl_DeleteTimerHandler(drawPtr->timerToken);
            drawPtr->timerToken = 0;
        }
        drawPtr->timerToken = Tcl_CreateTimerHandler(drawPtr->delay, 
                DrawerTimerProc, drawPtr);
    } else {
        CloseDrawer(drawPtr);
    }
    EventuallyRedraw(setPtr);
}

static int
SetDrawerVariable(Drawer *drawPtr) 
{
    int state;
    int result;

    state = (drawPtr->flags & CLOSED) ? FALSE : TRUE;
    result = TCL_OK;
    if (drawPtr->varNameObjPtr == NULL) { 
        /* No variable set. No trace to trigger open/close of the drawer. */
        if (state) {
            EventuallyOpenDrawer(drawPtr);
        } else {
            EventuallyCloseDrawer(drawPtr);
        }
    } else {
        Tcl_Obj *objPtr;
        Drawerset *setPtr;

        objPtr = (state) ? drawPtr->openValueObjPtr : drawPtr->closeValueObjPtr;
        if (objPtr == NULL) {
            objPtr = Tcl_NewBooleanObj(state);
        }
        setPtr = drawPtr->setPtr;
        Tcl_IncrRefCount(objPtr);
        if (Tcl_ObjSetVar2(setPtr->interp, drawPtr->varNameObjPtr, NULL, 
                objPtr, TCL_GLOBAL_ONLY|TCL_LEAVE_ERR_MSG) == NULL) {
            result = TCL_ERROR;
        }
        Tcl_DecrRefCount(objPtr);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyDrawer --
 *
 *      Removes the Drawer structure from the hash table and frees the
 *      memory allocated by it.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the drawer is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyDrawer(Drawer *drawPtr)
{
    Drawerset *setPtr;

    setPtr = drawPtr->setPtr;
    if (drawPtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(drawPtr->timerToken);
        drawPtr->timerToken = 0;
    }
    if (drawPtr->tkwin != NULL) {
        Tk_DeleteEventHandler(drawPtr->tkwin, StructureNotifyMask,
                DrawerEventProc, drawPtr);
        Tk_ManageGeometry(drawPtr->tkwin, (Tk_GeomMgr *)NULL, drawPtr);
        if (Tk_IsMapped(drawPtr->tkwin)) {
            Tk_UnmapWindow(drawPtr->tkwin);
        }
    }
    if (drawPtr->deleteCmdObjPtr != NULL) {
        if (Tcl_EvalObjEx(setPtr->interp, drawPtr->deleteCmdObjPtr,
                TCL_EVAL_GLOBAL) != TCL_OK) {
            Tcl_BackgroundError(setPtr->interp);
        }
    }
    if (drawPtr->handle.tkwin != NULL) {
        Tk_Window tkwin;
        Handle *handlePtr;
        
        handlePtr = &drawPtr->handle;
        tkwin = handlePtr->tkwin;
        Tk_DeleteEventHandler(tkwin, 
                ExposureMask|FocusChangeMask|StructureNotifyMask, 
                HandleEventProc, drawPtr);
        Tk_ManageGeometry(tkwin, (Tk_GeomMgr *)NULL, drawPtr);
        handlePtr->tkwin = NULL;
        Tk_DestroyWindow(tkwin);
        if (handlePtr->hashPtr != NULL) {
            Blt_DeleteHashEntry(&setPtr->handleTable, handlePtr->hashPtr);
            handlePtr->hashPtr = NULL;
        }
    }
    Blt_Tags_ClearTagsFromItem(&setPtr->tags, drawPtr);
    Blt_FreeOptions(drawerSpecs, (char *)drawPtr, setPtr->display, 0);
    if (drawPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&setPtr->drawerTable, drawPtr->hashPtr);
        drawPtr->hashPtr = NULL;
    }
    if (drawPtr->link != NULL) {
        Blt_Chain_DeleteLink(setPtr->drawers, drawPtr->link);
        drawPtr->link = NULL;
    }
    Blt_Free(drawPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * DrawerTimerProc --
 *
 *---------------------------------------------------------------------------
 */
static void
DrawerTimerProc(ClientData clientData)
{
    Drawer *drawPtr = clientData;
    Drawerset *setPtr;
    double frac;
    int range;

    setPtr = drawPtr->setPtr;
    if (setPtr->flags & LAYOUT_PENDING) {
        int w, h;
        
        if (drawPtr->side & SIDE_VERTICAL) {
            GetVerticalDrawerGeometry(setPtr, drawPtr, &w, &h);
        } else {
            GetHorizontalDrawerGeometry(setPtr, drawPtr, &w, &h);
        }
    } 
    drawPtr->step++;
    frac = (double)drawPtr->step / (double)drawPtr->numSteps;
    range = drawPtr->scrollMax - drawPtr->scrollMin;
    if (drawPtr->flags & MOTION_LOG) {
        frac = log10(9.0 * frac + 1.0);
    }
    if (drawPtr->flags & CLOSING) {
        frac = 1.0 - frac;
        range = drawPtr->offset - drawPtr->scrollMin;
    }
    drawPtr->offset = (int)(range * frac);
    if (drawPtr->step < drawPtr->numSteps) {
        drawPtr->timerToken = Tcl_CreateTimerHandler(drawPtr->delay, 
                DrawerTimerProc, drawPtr);
    } else {
        if (drawPtr->timerToken != (Tcl_TimerToken)0) {
            Tcl_DeleteTimerHandler(drawPtr->timerToken);
        }
        if (drawPtr->flags & CLOSING) {
            CloseDrawer(drawPtr);
        } else {
            OpenDrawer(drawPtr);
        }
    }
    EventuallyRedraw(setPtr);
}

/*
 *---------------------------------------------------------------------------
 * 
 * DrawerVarTraceProc --
 *
 *      This procedure is invoked when someone changes the state variable
 *      associated with a radiobutton or checkbutton entry.  The entry's
 *      selected state is set to match the value of the variable.
 *
 * Results:
 *      NULL is always returned.
 *
 * Side effects:
 *      The drawer may become opened or closed.
 *
 *---------------------------------------------------------------------------
 */
static char *
DrawerVarTraceProc(ClientData clientData, Tcl_Interp *interp, const char *name1,
                   const char *name2, int flags)
{
    Drawer *drawPtr = clientData;
    Tcl_Obj *objPtr;
    int bool;

    assert(drawPtr->varNameObjPtr != NULL);
    if (flags & TCL_INTERP_DESTROYED) {
        return NULL;                    /* Interpreter is going away. */
    }
    /*
     * If the variable is being unset, then re-establish the trace.
     */
    if (flags & TCL_TRACE_UNSETS) {
        drawPtr->flags &= ~CLOSED;
        if (flags & TCL_TRACE_DESTROYED) {
            char *varName;

            varName = Tcl_GetString(drawPtr->varNameObjPtr);
            Tcl_TraceVar(interp, varName, VAR_FLAGS, DrawerVarTraceProc, 
                clientData);
        }
        goto done;
    }

    /*
     * Use the value of the variable to update the opened/closed status of
     * the drawer.
     */
    objPtr = Tcl_ObjGetVar2(interp, drawPtr->varNameObjPtr, NULL,
        TCL_GLOBAL_ONLY);
    if (objPtr == NULL) {
        return NULL;                    /* Can't get value of variable. */
    }
    bool = 0;
    if (drawPtr->openValueObjPtr == NULL) {
        if (Tcl_GetBooleanFromObj(interp, objPtr, &bool) != TCL_OK) {
            return NULL;
        }
    } else {
        bool = (strcmp(Tcl_GetString(objPtr), 
                       Tcl_GetString(drawPtr->openValueObjPtr)) == 0);
    }
    if (bool) {
        EventuallyOpenDrawer(drawPtr);
    } else {
        EventuallyCloseDrawer(drawPtr);
    }
 done:
    EventuallyRedraw(drawPtr->setPtr);
    return NULL;                        /* Done. */
}

/*ARGSUSED*/
static void
FreeTraceVarProc(ClientData clientData, Display *display, char *widgRec, 
                 int offset)
{
    Tcl_Obj **varObjPtrPtr = (Tcl_Obj **)(widgRec + offset);

    if (*varObjPtrPtr != NULL) {
        Drawer *drawPtr = (Drawer *)(widgRec);
        Drawerset *setPtr;
        const char *varName;

        setPtr = drawPtr->setPtr;
        varName = Tcl_GetString(*varObjPtrPtr);
        Tcl_UntraceVar(setPtr->interp, varName, VAR_FLAGS, DrawerVarTraceProc, 
                drawPtr);
        Tcl_DecrRefCount(*varObjPtrPtr);
        *varObjPtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTraceVar --
 *
 *      Convert the string representation of a color into a XColor pointer.
 *
 * Results:
 *      The return value is a standard TCL result.  The color pointer is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTraceVar(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    Tcl_Obj **varObjPtrPtr = (Tcl_Obj **)(widgRec + offset);
    Drawer *drawPtr = (Drawer *)(widgRec);
    const char *varName;

    /* Remove the current trace on the variable. */
    if (*varObjPtrPtr != NULL) {
        varName = Tcl_GetString(*varObjPtrPtr);
        Tcl_UntraceVar(interp, varName, VAR_FLAGS, DrawerVarTraceProc, drawPtr);
        Tcl_DecrRefCount(*varObjPtrPtr);
        *varObjPtrPtr = NULL;
    }
    varName = Tcl_GetString(objPtr);
    if ((varName[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
        return TCL_OK;
    }
    *varObjPtrPtr = objPtr;
    Tcl_IncrRefCount(objPtr);
    Tcl_TraceVar(interp, varName, VAR_FLAGS, DrawerVarTraceProc, drawPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TraceVarToObj --
 *
 *      Return the name of the trace variable.
 *
 * Results:
 *      The name of the variable representing the drawer is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TraceVarToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              char *widgRec, int offset, int flags)  
{
    Tcl_Obj *objPtr = *(Tcl_Obj **)(widgRec + offset); 

    if (objPtr != NULL) {
        return objPtr;
    } 
    return Tcl_NewStringObj("", -1);
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
    Drawer *drawPtr = (Drawer *)widgRec;
    Drawerset *setPtr;
    Tk_Window *tkwinPtr = (Tk_Window *)(widgRec + offset);
    Tk_Window old, tkwin;
    char *string;

    old = *tkwinPtr;
    tkwin = NULL;
    setPtr = drawPtr->setPtr;
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
         * Allow only widgets that are children of the drawerset window can
         * be used.  We are using the window as viewport to clip the
         * children as necessary.
         */
        parent = Tk_Parent(tkwin);
        if (parent != setPtr->tkwin) {
            Tcl_AppendResult(interp, "can't manage \"", Tk_PathName(tkwin),
                "\" in drawerset \"", Tk_PathName(setPtr->tkwin), "\"",
                (char *)NULL);
            return TCL_ERROR;
        }
        Tk_ManageGeometry(tkwin, &drawerMgrInfo, drawPtr);
        Tk_CreateEventHandler(tkwin, StructureNotifyMask, DrawerEventProc, 
                drawPtr);
    }
    if (old != NULL) {
        Tk_DeleteEventHandler(old, StructureNotifyMask, DrawerEventProc, 
                              drawPtr);
        Tk_ManageGeometry(old, (Tk_GeomMgr *)NULL, drawPtr);
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
 * ObjToMotionScaling --
 *
 *      Translates the given string to the motion scale it represents.  
 *      Valid scales are "linear" or "logarithmic".
 *
 * Results:
 *      A standard TCL result.  If successful the field in the structure
 *      is updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToMotionScaling(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                  Tcl_Obj *objPtr, char *widgRec, int offset, int flags)        
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    const char *string;
    int length;
    char c;
    int flag;
    
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    flag = 0;
    if ((c == 'l') && (strcmp(string, "linear") == 0)) {
        flag = MOTION_LINEAR;
    } else if ((c == 'l') && (length > 2) && 
               (strncmp(string, "logarithmic", length) == 0)) {
        flag = MOTION_LOG;
    } else {
        Tcl_AppendResult(interp, "unknown coloring scaling \"", string, "\"",
                         ": should be linear or logarithmic.",
                         (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~MOTION_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MotionScalingToObj --
 *
 *      Convert the motion scaling flag into a string Tcl_Obj.
 *
 * Results:
 *      The string representation of the color scaling flag is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
MotionScalingToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                   char *widgRec, int offset, int flags) 
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    Tcl_Obj *objPtr;
    
    switch (*flagsPtr & MOTION_MASK) {
    case MOTION_LINEAR:
        objPtr = Tcl_NewStringObj("linear", 6);         break;
    case MOTION_LOG:
        objPtr = Tcl_NewStringObj("log", 3);            break;
    default:
        objPtr = Tcl_NewStringObj("???", 3);            break;
    }
    return objPtr;
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
 * StateToObj --
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
StateToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
SetTag(Tcl_Interp *interp, Drawer *drawPtr, const char *tagName)
{
    Drawerset *setPtr;
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
    setPtr = drawPtr->setPtr;
    Blt_Tags_AddItemToTag(&setPtr->tags, tagName, drawPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static void
FreeTagsProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Drawerset *setPtr;
    Drawer *drawPtr = (Drawer *)widgRec;

    setPtr = drawPtr->setPtr;
    Blt_Tags_ClearTagsFromItem(&setPtr->tags, drawPtr);
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
    Drawerset *setPtr;
    Drawer *drawPtr = (Drawer *)widgRec;
    int i;
    char *string;
    int objc;
    Tcl_Obj **objv;

    setPtr = drawPtr->setPtr;
    Blt_Tags_ClearTagsFromItem(&setPtr->tags, drawPtr);
    string = Tcl_GetString(objPtr);
    if ((string[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
        return TCL_OK;
    }
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
        SetTag(interp, drawPtr, Tcl_GetString(objv[i]));
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
    Drawerset *setPtr;
    Drawer *drawPtr = (Drawer *)widgRec;
    Tcl_Obj *listObjPtr;

    setPtr = drawPtr->setPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Blt_Tags_AppendTagsToObj(&setPtr->tags, drawPtr, listObjPtr);
    return listObjPtr;
}

static void
EventuallyRedrawHandle(Drawer *drawPtr)
{
    if ((drawPtr->flags & REDRAW_PENDING) == 0) {
        drawPtr->flags |= REDRAW_PENDING;
        Tcl_DoWhenIdle(DisplayHandle, drawPtr);
    }
}


static Drawer *
FirstDrawer(Drawerset *setPtr, unsigned int hateFlags)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(setPtr->drawers); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Drawer *drawPtr;

        drawPtr = Blt_Chain_GetValue(link);
        if ((drawPtr->flags & hateFlags) == 0) {
            return drawPtr;
        }
    }
    return NULL;
}

static Drawer *
LastDrawer(Drawerset *setPtr, unsigned int hateFlags)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(setPtr->drawers); link != NULL;
         link = Blt_Chain_PrevLink(link)) {
        Drawer *drawPtr;

        drawPtr = Blt_Chain_GetValue(link);
        if ((drawPtr->flags & hateFlags) == 0) {
            return drawPtr;
        }
    }
    return NULL;
}


static Drawer *
NextDrawer(Drawer *drawPtr, unsigned int hateFlags)
{
    if (drawPtr != NULL) {
        Blt_ChainLink link;

        for (link = Blt_Chain_NextLink(drawPtr->link); link != NULL; 
             link = Blt_Chain_NextLink(link)) {
            drawPtr = Blt_Chain_GetValue(link);
            if ((drawPtr->flags & hateFlags) == 0) {
                return drawPtr;
            }
        }
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawersetEventProc --
 *
 *      This procedure is invoked by the Tk event handler when the
 *      container widget is reconfigured or destroyed.
 *
 *      The drawerset will be rearranged at the next idle point if the
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
 *      Arranges for the drawerset associated with tkwin to have its layout
 *      re-computed and drawn at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawersetEventProc(ClientData clientData, XEvent *eventPtr)
{
    Drawerset *setPtr = clientData;

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
        Tcl_EventuallyFree(setPtr, DrawersetFreeProc);
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            if (eventPtr->type == FocusIn) {
                setPtr->flags |= FOCUS;
            } else {
                setPtr->flags &= ~FOCUS;
            }
            EventuallyRedraw(setPtr);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        setPtr->flags |= LAYOUT_PENDING;
        EventuallyRedraw(setPtr);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * DrawerEventProc --
 *
 *      This procedure is invoked by the Tk event handler when
 *      StructureNotify events occur in a widget managed by the drawerset.
 *
 *      For example, when a managed widget is destroyed, it frees the
 *      corresponding drawer structure and arranges for the drawerset
 *      layout to be re-computed at the next idle point.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      If the managed widget was deleted, the Drawer structure gets
 *      cleaned up and the drawerset is rearranged.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawerEventProc(ClientData clientData, XEvent *eventPtr)
{
    Drawer *drawPtr = (Drawer *)clientData;

    if (eventPtr->type == ConfigureNotify) {
        int extBorderWidth;

        if (drawPtr->tkwin == NULL) {
            return;
        }
        extBorderWidth = Tk_Changes(drawPtr->tkwin)->border_width;
        if (drawPtr->extBorderWidth != extBorderWidth) {
            drawPtr->extBorderWidth = extBorderWidth;
            EventuallyRedraw(drawPtr->setPtr);
        }
    } else if (eventPtr->type == DestroyNotify) {
        if (drawPtr->setPtr != NULL) {
            drawPtr->setPtr->flags |= LAYOUT_PENDING;
            EventuallyRedraw(drawPtr->setPtr);
        }
        if (drawPtr->tkwin != NULL) {
            Tcl_EventuallyFree(drawPtr, DrawerFreeProc);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawerCustodyProc --
 *
 *      This procedure is invoked when a widget has been stolen by another
 *      geometry manager.  The information and memory associated with the
 *      widget is released.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the drawerset to have its layout recomputed at the next
 *      idle point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
DrawerCustodyProc(ClientData clientData, Tk_Window tkwin)
{
    Drawer *drawPtr = (Drawer *)clientData;

    if (Tk_IsMapped(drawPtr->tkwin)) {
        Tk_UnmapWindow(drawPtr->tkwin);
    }
    DestroyDrawer(drawPtr);
    drawPtr->setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(drawPtr->setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawerGeometryProc --
 *
 *      This procedure is invoked by Tk_GeometryRequest for widgets managed
 *      by the drawerset geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the drawerset to have its layout re-computed and
 *      re-arranged at the next idle point.
 *
 * ----------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
DrawerGeometryProc(ClientData clientData, Tk_Window tkwin)
{
    Drawer *drawPtr = (Drawer *)clientData;

    drawPtr->setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(drawPtr->setPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * HandleEventProc --
 *
 *      This procedure is invoked by the Tk event handler when various
 *      events occur in the drawer handle subwindow maintained by this
 *      widget.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
HandleEventProc(
    ClientData clientData,              /* Pointer to Drawer structure for
                                         * handle referred to by
                                         * eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    Drawer *drawPtr = (Drawer *)clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedrawHandle(drawPtr);
        }
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            if (eventPtr->type == FocusIn) {
                drawPtr->flags |= FOCUS;
            } else {
                drawPtr->flags &= ~FOCUS;
            }
            EventuallyRedrawHandle(drawPtr);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        if (drawPtr->handle.tkwin == NULL) {
            return;
        }
        EventuallyRedrawHandle(drawPtr);
    } else if (eventPtr->type == DestroyNotify) {
        drawPtr->handle.tkwin = NULL;
    } 
}

/*
 *---------------------------------------------------------------------------
 *
 * NextTaggedDrawer --
 *
 *      Returns the next drawer derived from the given tag.
 *
 * Results:
 *      Returns the pointer to the next drawer in the iterator.  If no more
 *      drawers are available, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Drawer *
NextTaggedDrawer(DrawerIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
    case ITER_ALL:
        if (iterPtr->link != NULL) {
            Drawer *drawPtr;
            
            drawPtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return drawPtr;
        }
        break;
    case ITER_PATTERN:
        {
            Blt_ChainLink link;
            
            for (link = iterPtr->link; link != NULL; 
                 link = Blt_Chain_NextLink(link)) {
                Drawer *drawPtr;
                
                drawPtr = Blt_Chain_GetValue(iterPtr->link);
                if (Tcl_StringMatch(drawPtr->name, iterPtr->tagName)) {
                    iterPtr->link = Blt_Chain_NextLink(link);
                    return drawPtr;
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
 * FirstTaggedDrawer --
 *
 *      Returns the first drawer derived from the given tag.
 *
 * Results:
 *      Returns the first drawer in the sequence.  If no more drawers are
 *      in the list, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Drawer *
FirstTaggedDrawer(DrawerIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
    case ITER_ALL:
        if (iterPtr->link != NULL) {
            Drawer *drawPtr;
            
            drawPtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return drawPtr;
        }
        break;

    case ITER_PATTERN:
        {
            Blt_ChainLink link;
            
            for (link = iterPtr->link; link != NULL; 
                 link = Blt_Chain_NextLink(link)) {
                Drawer *drawPtr;
                
                drawPtr = Blt_Chain_GetValue(iterPtr->link);
                if (Tcl_StringMatch(drawPtr->name, iterPtr->tagName)) {
                    iterPtr->link = Blt_Chain_NextLink(link);
                    return drawPtr;
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
 * GetDrawerFromObj --
 *
 *      Gets the drawer associated the given index, tag, or label.  This
 *      routine is used when you want only one drawer.  It's an error if
 *      more than one drawer is specified (e.g. "all" tag or range "1:4").
 *      It's also an error if the tag is empty (no drawers are currently
 *      tagged).
 *
 *---------------------------------------------------------------------------
 */
static int 
GetDrawerFromObj(Tcl_Interp *interp, Drawerset *setPtr, Tcl_Obj *objPtr,
                 Drawer **drawPtrPtr)
{
    DrawerIterator iter;
    Drawer *firstPtr;

    if (GetDrawerIterator(interp, setPtr, objPtr, &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    firstPtr = FirstTaggedDrawer(&iter);
    if (firstPtr != NULL) {
        Drawer *nextPtr;

        nextPtr = NextTaggedDrawer(&iter);
        if (nextPtr != NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "multiple drawers specified by \"", 
                        Tcl_GetString(objPtr), "\"", (char *)NULL);
            }
            return TCL_ERROR;
        }
    }
    *drawPtrPtr = firstPtr;
    return TCL_OK;
}

static int
GetDrawerByIndex(Tcl_Interp *interp, Drawerset *setPtr, const char *string, 
                 int length, Drawer **drawPtrPtr)
{
    Drawer *drawPtr;
    char c;
    long pos;

    drawPtr = NULL;
    c = string[0];
    if (Blt_GetLong(NULL, string, &pos) == TCL_OK) {
        Blt_ChainLink link;

        link = Blt_Chain_GetNthLink(setPtr->drawers, pos);
        if (link != NULL) {
            drawPtr = Blt_Chain_GetValue(link);
        } 
        if (drawPtr == NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "can't find drawer: bad index \"", 
                        string, "\"", (char *)NULL);
            }
            return TCL_ERROR;
        }               
    } else if ((c == 'a') && (strcmp(string, "active") == 0)) {
        drawPtr = setPtr->activePtr;
    } else if ((c == 'f') && (strcmp(string, "first") == 0)) {
        drawPtr = FirstDrawer(setPtr, DISABLED);
    } else if ((c == 'l') && (strcmp(string, "last") == 0)) {
        drawPtr = LastDrawer(setPtr, DISABLED);
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
        drawPtr = LastDrawer(setPtr, 0);
    } else if ((c == 'n') && (strcmp(string, "none") == 0)) {
        drawPtr = NULL;
    } else {
        return TCL_CONTINUE;
    }
    *drawPtrPtr = drawPtr;
    return TCL_OK;
}

static Drawer *
GetDrawerByName(Drawerset *setPtr, const char *string)
{
    Blt_HashEntry *hPtr;
    
    hPtr = Blt_FindHashEntry(&setPtr->drawerTable, string);
    if (hPtr == NULL) {
        return NULL;
    }
    return Blt_GetHashValue(hPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * GetDrawerIterator --
 *
 *      Converts a string representing a drawer index into an drawer
 *      pointer.  The index may be in one of the following forms:
 *
 *       number         Drawer at index in the list of drawers.
 *       @x,y           Drawer closest to the specified X-Y screen coordinates.
 *       "active"       Drawer where mouse pointer is located.
 *       "posted"       Drawer is the currently posted cascade drawer.
 *       "next"         Next drawer from the focus drawer.
 *       "previous"     Previous drawer from the focus drawer.
 *       "end"          Last drawer.
 *       "none"         No drawer.
 *
 *       number         Drawer at position in the list of drawers.
 *       @x,y           Drawer closest to the specified X-Y screen coordinates.
 *       "active"       Drawer mouse is located over.
 *       "focus"        Drawer is the widget's focus.
 *       "select"       Currently selected drawer.
 *       "right"        Next drawer from the focus drawer.
 *       "left"         Previous drawer from the focus drawer.
 *       "up"           Next drawer from the focus drawer.
 *       "down"         Previous drawer from the focus drawer.
 *       "end"          Last drawer in list.
 *      "name:string"   Drawer named "string".
 *      "index:number"  Drawer at index number in list of drawers.
 *      "tag:string"    Drawer(s) tagged by "string".
 *      "label:pattern" Drawer(s) with label matching "pattern".
 *      
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.  The
 *      pointer to the node is returned via drawPtrPtr.  Otherwise,
 *      TCL_ERROR is returned and an error message is left in interpreter's
 *      result field.
 *
 *---------------------------------------------------------------------------
 */
static int
GetDrawerIterator(Tcl_Interp *interp, Drawerset *setPtr, Tcl_Obj *objPtr,
                  DrawerIterator *iterPtr)
{
    Drawer *drawPtr;
    Blt_Chain chain;
    char *string;
    char c;
    int numBytes;
    int length;
    int result;

    iterPtr->setPtr = setPtr;
    iterPtr->link = NULL;
    iterPtr->type = ITER_SINGLE;
    iterPtr->tagName = Tcl_GetStringFromObj(objPtr, &numBytes);
    iterPtr->nextPtr = NULL;
    iterPtr->startPtr = iterPtr->endPtr = NULL;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    iterPtr->startPtr = iterPtr->endPtr = setPtr->activePtr;
    iterPtr->type = ITER_SINGLE;
    result = GetDrawerByIndex(interp, setPtr, string, length, &drawPtr);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (result == TCL_OK) {
        iterPtr->startPtr = iterPtr->endPtr = drawPtr;
        return TCL_OK;
    }
    if (c == '.') {
        Blt_HashEntry *hPtr;
        
        hPtr = Blt_FindHashEntry(&setPtr->handleTable, string);
        if (hPtr != NULL) {
            drawPtr = Blt_GetHashValue(hPtr);
            iterPtr->startPtr = iterPtr->endPtr = drawPtr;
            iterPtr->type = ITER_SINGLE;
            return TCL_OK;
        }
        Tcl_AppendResult(interp, "unknown handle window \"", string, "\"",
                         (char *)NULL);
        return TCL_ERROR;
    } else if ((c == 'a') && (strcmp(iterPtr->tagName, "all") == 0)) {
        iterPtr->type  = ITER_ALL;
        iterPtr->link = Blt_Chain_FirstLink(setPtr->drawers);
    } else if ((c == 'i') && (length > 6) && 
               (strncmp(string, "index:", 6) == 0)) {
        if (GetDrawerByIndex(interp, setPtr, string + 6, length - 6, &drawPtr) 
            != TCL_OK) {
            return TCL_ERROR;
        }
        iterPtr->startPtr = iterPtr->endPtr = drawPtr;
    } else if ((c == 'n') && (length > 5) && 
               (strncmp(string, "name:", 5) == 0)) {
        drawPtr = GetDrawerByName(setPtr, string + 5);
        if (drawPtr == NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "can't find a drawer named \"", 
                        string + 5, "\" in \"", Tk_PathName(setPtr->tkwin), 
                        "\"", (char *)NULL);
            }
            return TCL_ERROR;
        }
        iterPtr->startPtr = iterPtr->endPtr = drawPtr;
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
        iterPtr->link = Blt_Chain_FirstLink(setPtr->drawers);
        iterPtr->tagName = string + 6;
        iterPtr->type = ITER_PATTERN;
    } else if ((drawPtr = GetDrawerByName(setPtr, string)) != NULL) {
        iterPtr->startPtr = iterPtr->endPtr = drawPtr;
    } else if ((chain = Blt_Tags_GetItemList(&setPtr->tags, string)) != NULL) {
        iterPtr->tagName = string;
        iterPtr->link = Blt_Chain_FirstLink(chain);
        iterPtr->type = ITER_TAG;
    } else {
        if (interp != NULL) {
            Tcl_AppendResult(interp, 
                "can't find drawer index, name, or tag \"", 
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
 * NewDrawer --
 *
 *      This procedure creates and initializes a new Drawer structure to
 *      hold a widget.  A valid widget has a parent widget that is either
 *      a) the container widget itself or b) a mutual ancestor of the
 *      container widget.
 *
 * Results:
 *      Returns a pointer to the new structure describing the new widget
 *      drawer.  If an error occurred, then the return value is NULL and an
 *      error message is left in interp->result.
 *
 * Side effects:
 *      Memory is allocated and initialized for the Drawer structure.
 *
 * ---------------------------------------------------------------------------- 
 */
static Drawer *
NewDrawer(Tcl_Interp *interp, Drawerset *setPtr, const char *name)
{
    Blt_HashEntry *hPtr;
    Drawer *drawPtr;

    if (name == NULL) {
        char string[200];
        int isNew;

        /* Generate an unique drawer name. */
        do {
            sprintf(string, "drawer%lu", (unsigned long)setPtr->nextId++);
            hPtr = Blt_CreateHashEntry(&setPtr->drawerTable, string, &isNew);
        } while (!isNew);
    }  else {
        int isNew;

        hPtr = Blt_CreateHashEntry(&setPtr->drawerTable, name, &isNew);
        if (!isNew) {
            Tcl_AppendResult(interp, "drawer \"", name, "\" already exists.",
                             (char *)NULL);
            return NULL;
        }
    }
    drawPtr = Blt_AssertCalloc(1, sizeof(Drawer));
    Blt_ResetLimits(&drawPtr->reqWidth);
    Blt_ResetLimits(&drawPtr->reqHeight);
    drawPtr->anchor = TK_ANCHOR_CENTER;
    drawPtr->fill = FILL_NONE;
    drawPtr->flags = VIRGIN | SHOW_HANDLE | CLOSED | MOTION_LOG | RESIZE;
    drawPtr->hashPtr = hPtr;
    drawPtr->name = Blt_GetHashKey(&setPtr->drawerTable, hPtr);
    drawPtr->nom  = LIMITS_NOM;
    drawPtr->setPtr = setPtr;
    drawPtr->side = SIDE_RIGHT;
    drawPtr->size = drawPtr->index = 0;
    drawPtr->numSteps = 8;
    drawPtr->delay = 30;
    Blt_SetHashValue(hPtr, drawPtr);
    {
        Tk_Window tkwin;
        char string[200];
        char *path;
        int isNew;
        Handle *handlePtr;
        
        handlePtr = &drawPtr->handle;
        /* Generate an unique handle subwindow name. */
        path = Blt_AssertMalloc(strlen(Tk_PathName(setPtr->tkwin)) + 200);
        do {
            sprintf(string, "handle%lu", (unsigned long)setPtr->nextHandleId++);
            sprintf(path, "%s.%s", Tk_PathName(setPtr->tkwin), string);
        } while (Tk_NameToWindow(interp, path, setPtr->tkwin) != NULL);
        Blt_Free(path);
        tkwin = Tk_CreateWindow(interp, setPtr->tkwin, string, (char *)NULL);
        if (tkwin == NULL) {
            DestroyDrawer(drawPtr);
            return NULL;
        }        
        /* Also add window pathname of drawer to handle table */
        hPtr = Blt_CreateHashEntry(&setPtr->handleTable, Tk_PathName(tkwin),
                &isNew);
        assert(isNew);
        Blt_SetHashValue(hPtr, drawPtr);
        handlePtr->hashPtr = hPtr;
        handlePtr->tkwin = tkwin;
        handlePtr->drawPtr = drawPtr;
        Tk_SetClass(tkwin, "BltDrawerHandle");
        Tk_CreateEventHandler(tkwin, 
                ExposureMask|FocusChangeMask|StructureNotifyMask, 
                HandleEventProc, drawPtr);
    }
    return drawPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * DrawerFreeProc --
 *
 *      Removes the Drawer structure from the hash table and frees the
 *      memory allocated by it.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the drawer is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawerFreeProc(DestroyData dataPtr)
{
    Drawer *drawPtr = (Drawer *)dataPtr;
    Drawerset *setPtr = drawPtr->setPtr;

    DestroyDrawer(drawPtr);
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
RenumberDrawers(Drawerset *setPtr)
{
    int count;
    Drawer *drawPtr;

    count = 0;
    for (drawPtr = FirstDrawer(setPtr, 0); drawPtr != NULL;
         drawPtr = NextDrawer(drawPtr, 0)) {
        drawPtr->index = count;
        count++;
    }
}

static void
MoveDrawer(Drawerset *setPtr, Drawer *drawPtr, InsertOrder order,
           Drawer *relPtr)
{
    if (Blt_Chain_GetLength(setPtr->drawers) == 1) {
        return;                         /* Can't rearrange one item. */
    }
    Blt_Chain_UnlinkLink(setPtr->drawers, drawPtr->link);
    switch (order) {
    case INSERT_AFTER:                  /* After */
        Blt_Chain_LinkAfter(setPtr->drawers, drawPtr->link, relPtr->link);
        break;
    case INSERT_BEFORE:                 /* Before */
        Blt_Chain_LinkBefore(setPtr->drawers, drawPtr->link, relPtr->link);
        break;
    }
    RenumberDrawers(setPtr);
    setPtr->flags |= LAYOUT_PENDING;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewDrawerset --
 *
 *      This procedure creates and initializes a new Drawerset structure
 *      with tkwin as its container widget. The internal structures
 *      associated with the drawerset are initialized.
 *
 * Results:
 *      Returns the pointer to the new Drawerset structure describing the
 *      new drawerset geometry manager.  If an error occurred, the return
 *      value will be NULL and an error message is left in interp->result.
 *
 * Side effects:
 *      Memory is allocated and initialized, an event handler is set up to
 *      watch tkwin, etc.
 *
 *---------------------------------------------------------------------------
 */
static Drawerset *
NewDrawerset(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Drawerset *setPtr;
    Tk_Window tkwin;

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), 
        Tcl_GetString(objPtr), (char *)NULL);
    if (tkwin == NULL) {
        return NULL;
    }
    Tk_SetClass(tkwin, "BltDrawerset");
    setPtr = Blt_AssertCalloc(1, sizeof(Drawerset));
    setPtr->tkwin = tkwin;
    setPtr->interp = interp;
    setPtr->display = Tk_Display(tkwin);
    setPtr->handleThickness = 2;
    setPtr->handlePad.side1 = setPtr->handlePad.side2 = 2;
    setPtr->relief = TK_RELIEF_FLAT;
    setPtr->activeRelief = TK_RELIEF_RAISED;
    setPtr->handleBW = 1;
    setPtr->flags = LAYOUT_PENDING | RESTACK | ANIMATE;
    setPtr->scrollUnits = 10;
    Blt_SetWindowInstanceData(tkwin, setPtr);
    Blt_InitHashTable(&setPtr->drawerTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&setPtr->handleTable, BLT_STRING_KEYS);
    Blt_Tags_Init(&setPtr->tags);
    Tk_CreateEventHandler(tkwin, 
        ExposureMask|FocusChangeMask|StructureNotifyMask, 
         DrawersetEventProc, setPtr);
    setPtr->drawers = Blt_Chain_Create();
    setPtr->cmdToken = Tcl_CreateObjCommand(interp, Tk_PathName(tkwin), 
        DrawersetInstCmdProc, setPtr, DrawersetInstCmdDeleteProc);
    setPtr->defVertCursor = Tk_GetCursor(interp, tkwin, DEF_VCURSOR);
    setPtr->defHorzCursor = Tk_GetCursor(interp, tkwin, DEF_HCURSOR);
    return setPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyDrawerset --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the Drawerset structure at a safe time (when no-one is
 *      using it anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the drawerset geometry manager is freed
 *      up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyDrawerset(Drawerset *setPtr)             
{
    Blt_ChainLink link;

    Blt_FreeOptions(drawersetSpecs, (char *)setPtr, setPtr->display, 0);
    /* Release the chain of entries. */
    for (link = Blt_Chain_FirstLink(setPtr->drawers); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Drawer *drawPtr;

        drawPtr = Blt_Chain_GetValue(link);
        drawPtr->link = NULL;           /* Don't disrupt this chain of
                                         * entries. */
        drawPtr->hashPtr = NULL;
        DestroyDrawer(drawPtr);
    }
    Tk_FreeCursor(setPtr->display, setPtr->defHorzCursor);
    Tk_FreeCursor(setPtr->display, setPtr->defVertCursor);
    Blt_Tags_Reset(&setPtr->tags);
    Blt_Chain_Destroy(setPtr->drawers);
    Blt_DeleteHashTable(&setPtr->drawerTable);
    Blt_DeleteHashTable(&setPtr->handleTable);
    Blt_Free(setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawersetFreeProc --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the Drawerset structure at a safe time (when no-one is
 *      using it anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the drawerset geometry manager is freed
 *      up.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawersetFreeProc(DestroyData dataPtr)  /* Drawerset structure */
{
    Drawerset *setPtr = (Drawerset *)dataPtr;

    DestroyDrawerset(setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetDrawers --
 *
 *      Sets/resets the size of each drawer to the minimum limit of the
 *      drawer (this is usually zero). This routine gets called when new
 *      widgets are added, deleted, or resized.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The size of each drawer is re-initialized to its minimum size.
 *
 *---------------------------------------------------------------------------
 */
static void
ResetDrawers(Drawerset *setPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(setPtr->drawers); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Drawer *drawPtr;
        int extra, size;
        Blt_Limits *limitsPtr;
        
        drawPtr = Blt_Chain_GetValue(link);
        /*
         * The constraint procedure below also has the desired side-effect
         * of setting the minimum, maximum, and nominal values to the
         * requested size of its associated widget (if one exists).
         */
        extra = 0;
        if (drawPtr->side & SIDE_VERTICAL) {
            limitsPtr = &drawPtr->reqHeight;
            size = BoundSize(0, limitsPtr);
            
        } else {
            limitsPtr = &drawPtr->reqWidth;
            size = BoundSize(0, limitsPtr);
        }
        if (drawPtr->flags & SHOW_HANDLE) {
            extra += setPtr->handleSize;
        }
        if (limitsPtr->flags & LIMITS_NOM_SET) {
            /*
             * This could be done more cleanly.  We want to ensure that the
             * requested nominal size is not overridden when determining
             * the normal sizes.  So temporarily fix min and max to the
             * nominal size and reset them back later.
             */
            drawPtr->min = drawPtr->max = drawPtr->normalSize = drawPtr->nom = 
                size + extra;
        } else {
            /* The range defaults to 0..MAXINT */
            drawPtr->min = limitsPtr->min + extra;
            drawPtr->max = limitsPtr->max + extra;
            drawPtr->nom = LIMITS_NOM;
            drawPtr->normalSize = size + extra;
        }
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * GetDrawersGeometry --
 *
 *      Calculates the normal space requirements for drawers.  
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
GetDrawersGeometry(Drawerset *setPtr)
{
    Drawer *drawPtr;

    ResetDrawers(setPtr);
    for (drawPtr = FirstDrawer(setPtr, 0); drawPtr != NULL;
         drawPtr = NextDrawer(drawPtr, 0)) {
        int w, h;
        
        drawPtr->offset = drawPtr->scrollMax;
        if (drawPtr->side & SIDE_VERTICAL) {
            GetVerticalDrawerGeometry(setPtr, drawPtr, &w, &h);
        } else {
            GetHorizontalDrawerGeometry(setPtr, drawPtr, &w, &h);
        }
        drawPtr->normalSize = drawPtr->scrollMax - drawPtr->scrollMin;
        if (drawPtr->flags & CLOSED) {
            if (drawPtr->tkwin != NULL) {
                if (Tk_IsMapped(drawPtr->tkwin)) {
                    Tk_UnmapWindow(drawPtr->tkwin);
                }
                if (Tk_IsMapped(drawPtr->handle.tkwin)) {
                    Tk_UnmapWindow(drawPtr->handle.tkwin);
                }
            }
            continue;
        }
    }
}


static void
ArrangeHorizontalDrawer(Drawer *drawPtr) 
{
    Drawerset *setPtr;
    int x, y, w, h;

#if TRACE
    fprintf(stderr, "ArrangeHorizontalDrawer(%s, offset=%d)\n", drawPtr->name, drawPtr->offset);
#endif
    setPtr = drawPtr->setPtr;
    GetHorizontalDrawerGeometry(setPtr, drawPtr, &w, &h);

    /* If x is beyond normal maximum, allow window to grow. */
    if (drawPtr->offset > drawPtr->scrollMax) {
        drawPtr->offset = drawPtr->scrollMax;
    }
    if (drawPtr->offset < drawPtr->scrollMin) {
        drawPtr->offset = drawPtr->scrollMin;
    }
    y = 0;
    if (drawPtr->side == SIDE_LEFT) {
        x = drawPtr->offset - drawPtr->scrollMax;
    } else {
        x = Tk_Width(setPtr->tkwin) - drawPtr->offset;
    }
    if (Tk_Height(setPtr->tkwin) > h) {
        switch (drawPtr->anchor) {
        case TK_ANCHOR_NW:              /* Upper left corner */
        case TK_ANCHOR_SW:              /* Lower left corner */
        case TK_ANCHOR_W:               /* Left center */
            break;
        case TK_ANCHOR_N:               /* Top center */
        case TK_ANCHOR_CENTER:          /* Centered */
        case TK_ANCHOR_S:               /* Bottom center */
            y += (Tk_Height(setPtr->tkwin) - h) / 2;
            break;
        case TK_ANCHOR_E:               /* Right center */
        case TK_ANCHOR_SE:              /* Lower right corner */
        case TK_ANCHOR_NE:              /* Upper right corner */
            y += Tk_Height(setPtr->tkwin) - h;
            break;                      
        }
    }
    if ((w < 1) || (h < 1)) {
        if (Tk_IsMapped(drawPtr->tkwin)) {
            Tk_UnmapWindow(drawPtr->tkwin);
        }
    } else {
        /* Resize and/or move the widget as necessary. */
        if ((x != Tk_X(drawPtr->tkwin)) || (y != Tk_Y(drawPtr->tkwin)) ||
            (w != Tk_Width(drawPtr->tkwin)) ||
            (h != Tk_Height(drawPtr->tkwin))) {
            Tk_MoveResizeWindow(drawPtr->tkwin, x, y, w, h);
        }
        if (!Tk_IsMapped(drawPtr->tkwin)) {
            Tk_MapWindow(drawPtr->tkwin);
        }
        drawPtr->flags &= ~VIRGIN;
    }
    if (drawPtr->flags & SHOW_HANDLE) {
        Handle *handlePtr = &drawPtr->handle;

        if (drawPtr->side == SIDE_LEFT) {
            x += w;
        } else {
            x -= setPtr->handleSize;
        }
        w = setPtr->handleSize; 
        /* Same y and h.  */
        if ((x != Tk_X(handlePtr->tkwin)) || 
            (y != Tk_Y(handlePtr->tkwin)) ||
            (w != Tk_Width(handlePtr->tkwin)) ||
            (h != Tk_Height(handlePtr->tkwin))) {
            Tk_MoveResizeWindow(handlePtr->tkwin, x, y, w, h);
        }
        if (!Tk_IsMapped(handlePtr->tkwin)) {
            Tk_MapWindow(handlePtr->tkwin);
        }
    } else if (Tk_IsMapped(drawPtr->handle.tkwin)) {
        Tk_UnmapWindow(drawPtr->handle.tkwin);
    }
}

static void
ArrangeVerticalDrawer(Drawer *drawPtr) 
{
    Drawerset *setPtr;
    int x, y, w, h;

#if TRACE
    fprintf(stderr, "ArrangeVerticalDrawer(%s)\n", drawPtr->name);
#endif
    setPtr = drawPtr->setPtr;
    GetVerticalDrawerGeometry(setPtr, drawPtr, &w, &h);

    /* If y is beyond normal maximum, allow window to grow. */
    if (drawPtr->offset > drawPtr->scrollMax) {
        drawPtr->offset = drawPtr->scrollMax;
    }
    if (drawPtr->offset < drawPtr->scrollMin) {
        drawPtr->offset = drawPtr->scrollMin;
    }
    x = 0;
    if (drawPtr->side == SIDE_TOP) {
        y = drawPtr->offset - drawPtr->scrollMax;
    } else {
        y = Tk_Height(setPtr->tkwin) - drawPtr->offset;
    }
    if (Tk_Width(setPtr->tkwin) > w) {
        switch (drawPtr->anchor) {
        case TK_ANCHOR_NW:              /* Upper left corner */
        case TK_ANCHOR_SW:              /* Lower left corner */
        case TK_ANCHOR_W:               /* Left center */
            break;
        case TK_ANCHOR_N:               /* Top center */
        case TK_ANCHOR_CENTER:          /* Centered */
        case TK_ANCHOR_S:               /* Bottom center */
            x += (Tk_Width(setPtr->tkwin) - w) / 2;
            break;
        case TK_ANCHOR_E:               /* Right center */
        case TK_ANCHOR_SE:              /* Lower right corner */
        case TK_ANCHOR_NE:              /* Upper right corner */
            x += Tk_Width(setPtr->tkwin) - w;
            break;                      
        }
    }
    if ((w < 1) || (h < 1)) {
        if (Tk_IsMapped(drawPtr->tkwin)) {
            Tk_UnmapWindow(drawPtr->tkwin);
        }
    } else {
        /*
         * Resize and/or move the widget as necessary.
         */
        if ((x != Tk_X(drawPtr->tkwin)) || 
            (y != Tk_Y(drawPtr->tkwin)) ||
            (w != Tk_Width(drawPtr->tkwin)) || 
            (h != Tk_Height(drawPtr->tkwin))) {
            Tk_MoveResizeWindow(drawPtr->tkwin, x, y, w, h);
        }
        if (!Tk_IsMapped(drawPtr->tkwin)) {
            Tk_MapWindow(drawPtr->tkwin);
        }
        drawPtr->flags &= ~VIRGIN;
    }
    if ((drawPtr->flags & SHOW_HANDLE) || (setPtr->handleSize < 1)) {
        Handle *handlePtr = &drawPtr->handle;

        if (drawPtr->side == SIDE_TOP) {
            y += h;
        } else {
            y -= setPtr->handleSize;
        }
        h = setPtr->handleSize; 
        /* Same x and w.  y */
        if ((x != Tk_X(handlePtr->tkwin)) || 
            (y != Tk_Y(handlePtr->tkwin)) ||
            (w != Tk_Width(handlePtr->tkwin)) ||
            (h != Tk_Height(handlePtr->tkwin))) {
            Tk_MoveResizeWindow(handlePtr->tkwin, x, y, w, h);
        }
        if (!Tk_IsMapped(handlePtr->tkwin)) {
            Tk_MapWindow(handlePtr->tkwin);
        }
    } else if (Tk_IsMapped(drawPtr->handle.tkwin)) {
        Tk_UnmapWindow(drawPtr->handle.tkwin);
    }
}

static void
ArrangeBase(Drawerset *setPtr) 
{
    int x, y;
    unsigned int w, h;

    w = Tk_Width(setPtr->tkwin);
    h = Tk_Height(setPtr->tkwin);
    x = Tk_Changes(setPtr->tkwin)->border_width;
    y = Tk_Changes(setPtr->tkwin)->border_width;
    
    /* Resize and/or move the widget as necessary. */
    if ((x != Tk_X(setPtr->base)) || (y != Tk_Y(setPtr->base)) ||
        (w != Tk_Width(setPtr->base)) || (h != Tk_Height(setPtr->base))) {
        Tk_MoveResizeWindow(setPtr->base, x, y, w, h);
    }
    if (!Tk_IsMapped(setPtr->base)) {
        Tk_MapWindow(setPtr->base);
    }
    XLowerWindow(setPtr->display, Tk_WindowId(setPtr->base));
}

/*
 *---------------------------------------------------------------------------
 *
 * ArrangeDrawers --
 *
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The widgets in the drawers are possibly resized and redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
ArrangeDrawers(Drawerset *setPtr)
{
    Drawer *drawPtr;

    for (drawPtr = FirstDrawer(setPtr, HIDDEN); drawPtr != NULL;
         drawPtr = NextDrawer(drawPtr, HIDDEN)) {
        if (drawPtr->tkwin == NULL) {
            if (Tk_IsMapped(drawPtr->handle.tkwin)) {
                Tk_UnmapWindow(drawPtr->handle.tkwin);
            }
            continue;
        }
        if (drawPtr->side & SIDE_VERTICAL) {
            ArrangeVerticalDrawer(drawPtr);
        } else {
            ArrangeHorizontalDrawer(drawPtr);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * RestackDrawers --
 *
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The widgets in the drawers are possibly resized and redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
RestackDrawers(Drawerset *setPtr)
{
    Blt_ChainLink link;

    if (setPtr->base != NULL) {
        Tk_RestackWindow(setPtr->base, Above, NULL);
    }
    for (link = Blt_Chain_FirstLink(setPtr->drawers); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Drawer *drawPtr;

        drawPtr = Blt_Chain_GetValue(link);
        if (drawPtr->tkwin != NULL) {
            Tk_RestackWindow(drawPtr->tkwin, Above, NULL);
        }
        if ((drawPtr->flags & SHOW_HANDLE) && (drawPtr->handle.tkwin != NULL)) {
            Tk_RestackWindow(drawPtr->handle.tkwin, Above, NULL);
        }
    }
}

static void
ComputeGeometry(Drawerset *setPtr) 
{
    GetDrawersGeometry(setPtr);
    setPtr->normalWidth = setPtr->normalHeight = 0;
    if (setPtr->base != NULL) {
        setPtr->normalWidth = Tk_ReqWidth(setPtr->base);
        setPtr->normalHeight = Tk_ReqHeight(setPtr->base);
    }
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
    setPtr->flags &= ~LAYOUT_PENDING;
}

static void
ConfigureDrawerset(Drawerset *setPtr) 
{
    if (Blt_ConfigModified(drawersetSpecs, "-window", (char *)NULL)) {
        if (setPtr->base != NULL) {
            UnmanageBase(setPtr, setPtr->base);
            setPtr->base = NULL;
        }
        if ((setPtr->flags & WARD_PENDING) == 0) {
            Tcl_DoWhenIdle(InstallBase, setPtr);
            setPtr->flags |= WARD_PENDING;
        }           
    }
    setPtr->normalWidth = setPtr->normalHeight = 200;
    if (setPtr->base != NULL) {
        setPtr->normalWidth = Tk_ReqWidth(setPtr->base);
        setPtr->normalHeight = Tk_ReqHeight(setPtr->base);
    }
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
    setPtr->handleSize = PADDING(setPtr->handlePad) + 
        setPtr->handleThickness;
}

static void
ResizeOrSlideDrawer(Drawer *drawPtr, int delta)
{
    Drawerset *setPtr;
    Blt_Limits *limitsPtr;
    int size;
    
    setPtr = drawPtr->setPtr;
#if TRACE
    fprintf(stderr, "ResizeOrSlideDrawer(%s, delta=%d) size=%d, max=%d, min=%d, offset=%d\n",
            drawPtr->name, delta, drawPtr->scrollMax - drawPtr->scrollMin,
            drawPtr->scrollMax, drawPtr->scrollMin, drawPtr->offset);
#endif
    if (delta == 0) {
        return;
    }
    if (drawPtr->side & (SIDE_LEFT|SIDE_TOP)) {
        delta = -delta;
    }
    if (drawPtr->side & SIDE_VERTICAL) {
        int maxHeight;
        
        limitsPtr = &drawPtr->reqHeight;
        maxHeight = Tk_Height(setPtr->tkwin);
        if (drawPtr->flags & SHOW_HANDLE) {
            maxHeight -= setPtr->handleSize;
        }
        if ((drawPtr->scrollMax - ABS(delta)) > maxHeight) {
            delta = maxHeight - drawPtr->scrollMax;
        }
    } else {
        int maxWidth;
        
        limitsPtr = &drawPtr->reqWidth;
        maxWidth = Tk_Width(setPtr->tkwin);
        if (drawPtr->flags & SHOW_HANDLE) {
            maxWidth -= setPtr->handleSize;
        }
        if ((drawPtr->scrollMax - ABS(delta)) > maxWidth) {
            delta = maxWidth - drawPtr->scrollMax;
        }
    }
    drawPtr->offset -= delta;
    size = drawPtr->scrollMax - delta;
    if ((drawPtr->flags & RESIZE) && (drawPtr->offset >= limitsPtr->min)) {
        SetNominal(size, limitsPtr);
    }
    size = BoundSize(size, limitsPtr);
    EventuallyRedraw(setPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * AddOp --
 *
 *      Adds a drawer into the widget. The drawer is by default draw above
 *      the previously created drawers.
 *
 * Results:
 *      Returns a standard TCL result.  The index of the drawer is left in
 *      interp->result.
 *
 *      pathName add ?label? ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AddOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{ 
    Drawer *drawPtr;
    Drawerset *setPtr = clientData;
    const char *name;

    name = NULL;
    if (objc > 2) {
        const char *string;

        string = Tcl_GetString(objv[2]);
        if (string[0] != '-') {
            if (GetDrawerFromObj(NULL, setPtr, objv[2], &drawPtr) == TCL_OK) {
                Tcl_AppendResult(interp, "drawer \"", string, 
                        "\" already exists", (char *)NULL);
                return TCL_ERROR;
            }
            name = string;
            objc--, objv++;
        }
    }
    drawPtr = NewDrawer(interp, setPtr, name);
    if (drawPtr == NULL) {
        return TCL_ERROR;
    }
    if (Blt_ConfigureWidgetFromObj(interp, drawPtr->handle.tkwin, drawerSpecs, 
        objc - 2, objv + 2, (char *)drawPtr, 0) != TCL_OK) {
        return TCL_ERROR;
    }
    drawPtr->link = Blt_Chain_Append(setPtr->drawers, drawPtr);
    RenumberDrawers(setPtr);
    EventuallyRedraw(setPtr);
    setPtr->flags |= LAYOUT_PENDING;
    Tcl_SetIntObj(Tcl_GetObjResult(interp), drawPtr->index);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *      Returns the name, position and options of a widget in the drawerset.
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
    Drawerset *setPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, setPtr->tkwin, drawersetSpecs, 
        (char *)setPtr, objv[2], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * CloseOp --
 *
 *      Closes the specified drawer. 
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      pathName open drawerName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CloseOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Drawer *drawPtr;
    DrawerIterator iter;
    Drawerset *setPtr = clientData;

    if (GetDrawerIterator(interp, setPtr, objv[2], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
         drawPtr = NextTaggedDrawer(&iter)) {
        drawPtr->flags |= CLOSED;
        SetDrawerVariable(drawPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * Results:
 *      Returns a standard TCL result.  A list of the drawerset
 *      configuration option information is left in interp->result.
 *
 *      pathName configure option value
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;

    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, setPtr->tkwin, drawersetSpecs, 
                (char *)setPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, setPtr->tkwin, drawersetSpecs, 
                (char *)setPtr, objv[2], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, setPtr->tkwin, drawersetSpecs,
        objc - 2, objv + 2, (char *)setPtr, BLT_CONFIG_OBJV_ONLY) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    ConfigureDrawerset(setPtr);

    /* Arrange for the layout to be computed at the next idle point. */
    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes the specified drawers from the widget.  Note that the
 *      drawer indices can be fixed only after all the deletions have
 *      occurred.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      pathName delete drawerName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    DrawerIterator iter;
    Drawer *drawPtr;

    if (GetDrawerIterator(interp, setPtr, objv[2], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
         drawPtr = NextTaggedDrawer(&iter)) {
        Tcl_EventuallyFree(drawPtr, DrawerFreeProc);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawerCgetOp --
 *
 * Results:
 *      Returns a standard TCL result.  A list of the widget attributes is
 *      left in interp->result.
 *
 *      pathName drawer cget drawerName option
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DrawerCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Drawer *drawPtr;
    Drawerset *setPtr = clientData;

    if (GetDrawerFromObj(interp, setPtr, objv[3], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, setPtr->tkwin, drawerSpecs, 
        (char *)drawPtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawerConfigureOp --
 *
 *      Configure the options for drawers.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the drawer configuration
 *      option information is left in interp->result.
 *
 *      pathName drawer configure drawerName ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DrawerConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    Drawer *drawPtr;
    Drawerset *setPtr = clientData;

    if (GetDrawerFromObj(interp, setPtr, objv[3], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        return Blt_ConfigureInfoFromObj(interp, drawPtr->handle.tkwin,
                drawerSpecs, (char *)drawPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 5) {
        return Blt_ConfigureInfoFromObj(interp, drawPtr->handle.tkwin,
                drawerSpecs, (char *)drawPtr, objv[4], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, drawPtr->handle.tkwin, drawerSpecs, 
        objc-4, objv+4, (char *)drawPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

static Blt_OpSpec drawerOps[] =
{
    {"cget",       2, DrawerCgetOp,      5, 5, "drawer option",},
    {"configure",  2, DrawerConfigureOp, 4, 0, "drawer ?option value?",},
};

static int numDrawerOps = sizeof(drawerOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * DrawerOp --
 *
 *      This procedure is invoked to process the TCL command that
 *      corresponds to the drawer.  See the user documentation for details
 *      on what it does.
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
DrawerOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numDrawerOps, drawerOps, BLT_OP_ARG2, 
                            objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *      Indicates if a drawer by the given name exists.
 *
 * Results:
 *      Returns a standard TCL boolean result. 
 *
 *      pathName exists drawerName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;
    int state;

    state = FALSE;
    if (GetDrawerFromObj(NULL, setPtr, objv[2], &drawPtr) == TCL_OK) {
        if (drawPtr != NULL) {
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
 *      pathName handle activate drawerName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;

    if (GetDrawerFromObj(interp, setPtr, objv[3], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((drawPtr == NULL) || (drawPtr->flags & (DISABLED|HIDDEN))) {
        return TCL_OK;
    }
    if (drawPtr != setPtr->activePtr) {
        Tk_Cursor cursor;

        if (setPtr->activePtr != NULL) {
            EventuallyRedrawHandle(setPtr->activePtr);
        }
        if (drawPtr != NULL) {
            EventuallyRedrawHandle(drawPtr);
        }
        setPtr->activePtr = drawPtr;
        if (drawPtr->handle.cursor != None) {
            cursor = drawPtr->handle.cursor;
        } else if (drawPtr->side & SIDE_VERTICAL) {
            cursor = setPtr->defVertCursor;
        } else {
            cursor = setPtr->defHorzCursor;
        }
        Tk_DefineCursor(drawPtr->handle.tkwin, cursor);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HandleAnchorOp --
 *
 *      Set the anchor for the resize/moving the drawer.  Only one of the x
 *      and y coordinates are used depending upon the orientation of the
 *      drawer or drawer.
 *
 *      pathName handle anchor drawerName x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;
    int x, y;

    if (GetDrawerFromObj(interp, setPtr, objv[3], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((drawPtr == NULL) || (drawPtr->flags & (DISABLED|HIDDEN))) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    } 
    setPtr->flags |= HANDLE_ACTIVE;
    if (drawPtr->side & SIDE_VERTICAL) {
        setPtr->handleAnchor = y;
    } else {
        setPtr->handleAnchor = x;
    } 
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
    Drawerset *setPtr = clientData;

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
 *      pathName handle mark drawerName x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleMarkOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;
    int x, y;                           /* Root coordinates of the pointer
                                         * over the handle. */
    int delta, mark;

    if (GetDrawerFromObj(interp, setPtr, objv[3], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((drawPtr == NULL) || (drawPtr->flags & (DISABLED|HIDDEN))) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    } 
    mark = (drawPtr->side & SIDE_VERTICAL) ? y : x;
    delta = mark - setPtr->handleAnchor;
    ResizeOrSlideDrawer(drawPtr, delta);
    setPtr->handleAnchor = mark;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HandleMoveToOp --
 *
 *      Sets the location of the handle to coordinate (x or y) specified.
 *
 *      pathName handle moveto drawerName x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleMoveToOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;
    int x, y;
    int delta, mark;

    if (GetDrawerFromObj(interp, setPtr, objv[3], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((drawPtr == NULL) || (drawPtr->flags & (DISABLED|HIDDEN))) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    } 
    setPtr->flags &= ~HANDLE_ACTIVE;
    mark = (drawPtr->side & SIDE_VERTICAL) ? y : x;
    delta = mark - setPtr->handleAnchor;
    if (delta != 0) {
        ResizeOrSlideDrawer(drawPtr, delta);
    }
    setPtr->handleAnchor = mark;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HandleSizeOp --
 *
 *      Sets and/or gets the size of the opening for the drawer.  This does
 *      not affect the size of the window.
 *
 *      pathName handle size DrawerName numPixels
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;
    int size;

    if (GetDrawerFromObj(interp, setPtr, objv[3], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((drawPtr == NULL) || (drawPtr->flags & (DISABLED|HIDDEN))) {
        return TCL_OK;
    }
    if (objc == 4) {
        size = drawPtr->size;
    } else {
        int newSize;

        if (Blt_GetPixelsFromObj(interp, setPtr->tkwin, objv[4], PIXELS_NNEG, 
                &newSize) != TCL_OK) {
            return TCL_ERROR;
        }
        drawPtr->size = newSize;
        if (drawPtr->side & SIDE_VERTICAL) {
            drawPtr->reqHeight.nom = newSize;
            drawPtr->reqHeight.flags |= LIMITS_NOM_SET;
        } else {
            drawPtr->reqWidth.nom = newSize;
            drawPtr->reqWidth.flags |= LIMITS_NOM_SET;
        }
        drawPtr->offset = newSize;
        EventuallyRedraw(setPtr);
        size = newSize;
    } 
    Tcl_SetIntObj(Tcl_GetObjResult(interp), size);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HandleSlideOp --
 *
 *      Moves the handle. The handle is moved the given distance from its
 *      previous location.
 *
 *      pathName handle slide drawerName dx dy
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
HandleSlideOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;
    int delta, dx, dy;

    if (GetDrawerFromObj(interp, setPtr, objv[3], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((drawPtr == NULL) || (drawPtr->flags & (DISABLED|HIDDEN))) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &dx) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &dy) != TCL_OK)) {
        return TCL_ERROR;
    } 
    if (drawPtr->side & SIDE_VERTICAL) {
        delta = dy;
    } else {
        delta = dx;
    }
    if (delta != 0)  {
        ResizeOrSlideDrawer(drawPtr, delta);
    }
    return TCL_OK;
}

static Blt_OpSpec handleOps[] =
{
    {"activate",   2, HandleActivateOp,   4, 4, "drawerName"},
    {"anchor",     2, HandleAnchorOp,     6, 6, "drawerName x y"},
    {"deactivate", 1, HandleDeactivateOp, 3, 3, ""},
    {"mark",       2, HandleMarkOp,       6, 6, "drawerName x y"},
    {"moveto",     2, HandleMoveToOp,     6, 6, "drawerName x y"},
    {"size",       2, HandleSizeOp,       4, 5, "drawerName ?numPixels?"},
    {"slide",      2, HandleSlideOp,      6, 6, "drawerName dx dy"},
};

static int numHandleOps = sizeof(handleOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * HandleOp --
 *
 *      This procedure is invoked to process the TCL command that
 *      corresponds to the drawerset geometry manager.  See the user
 *      documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *      pathName handle args...
 *
 *---------------------------------------------------------------------------
 */
static int
HandleOp(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
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
 *      Returns the index of the given drawer.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      pathName index drawerName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;
    int index;

    index = -1;
    if (GetDrawerFromObj(NULL, setPtr, objv[2], &drawPtr) == TCL_OK) {
        if (drawPtr != NULL) {
            index = drawPtr->index;
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
 *      Inserts a new drawer into the drawerset.
 *
 *      pathName insert after whereName ?label? ?option value ...?
 *      pathName insert before whereName ?label? ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InsertOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr, *relPtr;
    InsertOrder order;
    const char *name;
        
    if (GetInsertOrder(interp, objv[2], &order) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetDrawerFromObj(interp, setPtr, objv[3], &relPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    name = NULL;
    if (objc > 4) {
        const char *string;

        string = Tcl_GetString(objv[4]);
        if (string[0] != '-') {
            if (GetDrawerFromObj(NULL, setPtr, objv[4], &drawPtr) == TCL_OK) {
                Tcl_AppendResult(interp, "drawer \"", string, 
                        "\" already exists", (char *)NULL);
                return TCL_ERROR;
            }
            name = string;
            objc--, objv++;
        }
    }
    drawPtr = NewDrawer(interp, setPtr, name);
    if (drawPtr == NULL) {
        return TCL_ERROR;
    }
    MoveDrawer(setPtr, drawPtr, order, relPtr);
    EventuallyRedraw(setPtr);
    setPtr->flags |= LAYOUT_PENDING;

    if (Blt_ConfigureWidgetFromObj(interp, drawPtr->handle.tkwin, drawerSpecs, 
        objc - 4, objv + 4, (char *)drawPtr, 0) != TCL_OK) {
        DestroyDrawer(drawPtr);
        return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), drawPtr->name, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsOpenOp --
 *
 *      Indicates if drawer is open or closed.
 *
 * Results:
 *      Returns a standard TCL boolean result.
 *
 *      pathName isopen drawerName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsOpenOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Drawer *drawPtr;
    Drawerset *setPtr = clientData;
    int state;

    if (GetDrawerFromObj(interp, setPtr, objv[2], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    state = FALSE;
    if (drawPtr != NULL) {
        state = (drawPtr->flags & CLOSED) ? 0 : 1;
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LowerOp --
 *
 *      Lowers the specified drawer to bottom of the stack of drawers.  
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      pathName raise drawerName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
LowerOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Drawer *drawPtr;
    DrawerIterator iter;
    Drawerset *setPtr = clientData;

    if (GetDrawerIterator(interp, setPtr, objv[2], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
         drawPtr = NextTaggedDrawer(&iter)) {
        if (drawPtr->flags & HIDDEN) {
            continue;
        }
        LowerDrawer(drawPtr);
    }
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MoveOp --
 *
 *      Moves the given drawer to a position in the stacking list.
 *
 *      pathName move after afterName drawerName
 *      pathName move before beforeName drawerName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MoveOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr, *relPtr;
    InsertOrder order;

    if (GetInsertOrder(interp, objv[2], &order) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetDrawerFromObj(interp, setPtr, objv[3], &relPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetDrawerFromObj(interp, setPtr, objv[4], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((drawPtr == NULL) || (drawPtr == relPtr)) {
        return TCL_OK;
    }
    MoveDrawer(setPtr, drawPtr, order, relPtr);
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *      Returns the names of the drawers whose name matches the given
 *      pattern.
 *
 *      pathName names ?pattern ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)   
{
    Drawerset *setPtr = clientData;
    Tcl_Obj *listObjPtr;
    Blt_ChainLink link;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    for (link = Blt_Chain_FirstLink(setPtr->drawers); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Drawer *drawPtr;
        int i;
        int match;
        
        drawPtr = Blt_Chain_GetValue(link);
        match = (objc == 2);
        for (i = 2; i < objc; i++) {
            if (Tcl_StringMatch(drawPtr->name, Tcl_GetString(objv[i]))) {
                match = TRUE;
                break;
            }
        }
        if (match) {
            Tcl_Obj *objPtr;
            
            objPtr = Tcl_NewStringObj(drawPtr->name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OpenOp --
 *
 *      Opens the specified drawer. 
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      pathName open drawerName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
OpenOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Drawer *drawPtr;
    DrawerIterator iter;
    Drawerset *setPtr = clientData;

    if (GetDrawerIterator(interp, setPtr, objv[2], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
         drawPtr = NextTaggedDrawer(&iter)) {
        if (drawPtr->flags & HIDDEN) {
            continue;                   /* Can't open hidden drawers. */
        }
        if ((drawPtr->flags & CLOSED) == 0) {
            continue;                   /* Don't do anything if the drawer
                                         * is already open. */
        }
        drawPtr->flags &= ~CLOSED;
        SetDrawerVariable(drawPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RaiseOp --
 *
 *      Raises the specified drawer to the top of the stack of drawers.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      pathName raise drawerName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RaiseOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;
    DrawerIterator iter;

    if (GetDrawerIterator(interp, setPtr, objv[2], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
         drawPtr = NextTaggedDrawer(&iter)) {
        if (drawPtr->flags & HIDDEN) {
            continue;                   /* Can't raised hidden drawers. */
        }
        RaiseDrawer(drawPtr);
    }
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SlideOp --
 *
 *      Resizes the drawer. The handle is moved the given distance from its
 *      previous location.
 *
 *      pathName slide drawerName dx dy 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SlideOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;
    int dx, dy;

    if (GetDrawerFromObj(interp, setPtr, objv[3], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((drawPtr == NULL) || (drawPtr->flags & (DISABLED|HIDDEN))) {
        return TCL_OK;
    }
    if (Tcl_GetIntFromObj(interp, objv[4], &dx) != TCL_OK) {
        return TCL_ERROR;
    } 
    if (Tcl_GetIntFromObj(interp, objv[4], &dy) != TCL_OK) {
        return TCL_ERROR;
    } 
    if (drawPtr->side & SIDE_VERTICAL) {
        if (dy != 0) {
            ResizeOrSlideDrawer(drawPtr, dy);
        }
    } else {
        if (dx != 0) {
            ResizeOrSlideDrawer(drawPtr, dx);
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * SizeOp --
 *
 *      pathName size drawerName ?numPixels?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;
    int size;

    if (GetDrawerFromObj(interp, setPtr, objv[3], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 3) {
        size = drawPtr->size;
    } else {
        int newSize;

        if (Blt_GetPixelsFromObj(interp, setPtr->tkwin, objv[3], PIXELS_NNEG, 
                &newSize) != TCL_OK) {
            return TCL_ERROR;
        }
        drawPtr->size = newSize;
        drawPtr->offset = newSize;
        EventuallyRedraw(setPtr);
        size = newSize;
    } 
    Tcl_SetIntObj(Tcl_GetObjResult(interp), size);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagAddOp --
 *
 *      pathName tag add tag ?drawerName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    const char *tag;
    long drawerId;

    tag = Tcl_GetString(objv[3]);
    if (Blt_GetLongFromObj(NULL, objv[3], &drawerId) == TCL_OK) {
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
            Drawer *drawPtr;
            DrawerIterator iter;
            
            if (GetDrawerIterator(interp, setPtr, objv[i], &iter) != TCL_OK) {
                return TCL_ERROR;
            }
            for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
                 drawPtr = NextTaggedDrawer(&iter)) {
                Blt_Tags_AddItemToTag(&setPtr->tags, tag, drawPtr);
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
 *      pathName delete tagName ?drawerName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    const char *tag;
    long drawerId;
    int i;

    tag = Tcl_GetString(objv[3]);
    if (Blt_GetLongFromObj(NULL, objv[3], &drawerId) == TCL_OK) {
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
        Drawer *drawPtr;
        DrawerIterator iter;
        
        if (GetDrawerIterator(interp, setPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
             drawPtr = NextTaggedDrawer(&iter)) {
            Blt_Tags_RemoveItemFromTag(&setPtr->tags, tag, drawPtr);
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * TagExistsOp --
 *
 *      Returns the existence of the one or more tags in the given node.
 *      If the node has any the tags, true is return in the interpreter.
 *
 *      pathName tag exists drawerName ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    DrawerIterator iter;
    Drawerset *setPtr = clientData;
    int i;

    if (GetDrawerIterator(interp, setPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        Drawer *drawPtr;
        const char *tag;

        tag = Tcl_GetString(objv[i]);
        for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
             drawPtr = NextTaggedDrawer(&iter)) {
            if (Blt_Tags_ItemHasTag(&setPtr->tags, drawPtr, tag)) {
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
 *      Removes the given tags from all drawers.
 *
 *      pathNames tag forget ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagForgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    int i;

    for (i = 3; i < objc; i++) {
        const char *tag;
        long drawerId;

        tag = Tcl_GetString(objv[i]);
        if (Blt_GetLongFromObj(NULL, objv[i], &drawerId) == TCL_OK) {
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
 *      pathName tag get drawerName ?pattern ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagGetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr; 
    DrawerIterator iter;
    Tcl_Obj *listObjPtr;

    if (GetDrawerIterator(interp, setPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
         drawPtr = NextTaggedDrawer(&iter)) {
        if (objc == 4) {
            Blt_Tags_AppendTagsToObj(&setPtr->tags, drawPtr, listObjPtr);
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
                Blt_Tags_AppendTagsToChain(&setPtr->tags, drawPtr, chain);
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
 *      Returns the names of all the tags in the drawerset.  If one of more
 *      node arguments are provided, then only the tags found in those
 *      nodes are returned.
 *
 *      pathName tag names ?drawerName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
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
            DrawerIterator iter;
            Drawer *drawPtr;

            if (GetDrawerIterator(interp, setPtr, objPtr, &iter) != TCL_OK) {
                goto error;
            }
            for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
                 drawPtr = NextTaggedDrawer(&iter)) {
                Blt_ChainLink link;
                Blt_Chain chain;

                chain = Blt_Chain_Create();
                Blt_Tags_AppendTagsToChain(&setPtr->tags, drawPtr, chain);
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
 *      returned will represent the union of drawers for all the given
 *      tags.
 *
 *      pathName tag indices ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagIndicesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Blt_HashTable drawerTable;
    int i;
        
    Blt_InitHashTable(&drawerTable, BLT_ONE_WORD_KEYS);
    for (i = 3; i < objc; i++) {
        long drawerId;
        const char *tag;

        tag = Tcl_GetString(objv[i]);
        if (Blt_GetLongFromObj(NULL, objv[i], &drawerId) == TCL_OK) {
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
                    Drawer *drawPtr;
                    int isNew;
                    
                    drawPtr = Blt_Chain_GetValue(link);
                    Blt_CreateHashEntry(&drawerTable, (char *)drawPtr, &isNew);
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
        for (hPtr = Blt_FirstHashEntry(&drawerTable, &iter); hPtr != NULL; 
             hPtr = Blt_NextHashEntry(&iter)) {
            Drawer *drawPtr;
            Tcl_Obj *objPtr;

            drawPtr = (Drawer *)Blt_GetHashKey(&drawerTable, hPtr);
            objPtr = Tcl_NewLongObj(drawPtr->index);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
    }
    Blt_DeleteHashTable(&drawerTable);
    return TCL_OK;

 error:
    Blt_DeleteHashTable(&drawerTable);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagSetOp --
 *
 *      Sets one or more tags for a given drawer.  Tag names can't start
 *      with a digit (to distinquish them from node ids) and can't be a
 *      reserved tag ("all").
 *
 *      pathName tag set drawerName ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    int i;
    DrawerIterator iter;

    if (GetDrawerIterator(interp, setPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        const char *tag;
        Drawer *drawPtr;
        long drawerId;

        tag = Tcl_GetString(objv[i]);
        if (Blt_GetLongFromObj(NULL, objv[i], &drawerId) == TCL_OK) {
            Tcl_AppendResult(interp, "bad tag \"", tag, 
                             "\": can't be a number.", (char *)NULL);
            return TCL_ERROR;
        }
        if (strcmp(tag, "all") == 0) {
            Tcl_AppendResult(interp, "can't add reserved tag \"", tag, "\"",
                             (char *)NULL);     
            return TCL_ERROR;
        }
        for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
             drawPtr = NextTaggedDrawer(&iter)) {
            Blt_Tags_AddItemToTag(&setPtr->tags, tag, drawPtr);
        }    
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagUnsetOp --
 *
 *      Removes one or more tags from a given drawer. If a tag doesn't
 *      exist or is a reserved tag ("all"), nothing will be done and no
 *      error message will be returned.
 *
 *      pathName tag unset drawerName ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagUnsetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;
    DrawerIterator iter;

    if (GetDrawerIterator(interp, setPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (drawPtr = FirstTaggedDrawer(&iter); drawPtr != NULL; 
         drawPtr = NextTaggedDrawer(&iter)) {
        int i;
        for (i = 4; i < objc; i++) {
            const char *tag;

            tag = Tcl_GetString(objv[i]);
            Blt_Tags_RemoveItemFromTag(&setPtr->tags, tag, drawPtr);
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
    {"add",     1, TagAddOp,      2, 0, "?name? ?option value ...?",},
    {"delete",  1, TagDeleteOp,   2, 0, "?drawerName ...?",},
    {"exists",  1, TagExistsOp,   4, 0, "drawerName  ?tag ...?",},
    {"forget",  1, TagForgetOp,   3, 0, "?tag ...?",},
    {"get",     1, TagGetOp,      4, 0, "drawerName ?pattern ...?",},
    {"indices", 1, TagIndicesOp,  3, 0, "?tag ...?",},
    {"names",   1, TagNamesOp,    3, 0, "?drawerName ...?",},
    {"set",     1, TagSetOp,      4, 0, "drawerName ?tag ...?",},
    {"unset",   1, TagUnsetOp,    4, 0, "drawerName ?tag ...?",},
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
 * ToggleOp --
 *
 *      Toggle to opening/closing of the specified drawer. 
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      pathName toggle drawerName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ToggleOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Drawerset *setPtr = clientData;
    Drawer *drawPtr;

    if (GetDrawerFromObj(interp, setPtr, objv[2], &drawPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (drawPtr->tkwin == NULL) {
        return TCL_OK;
    }
    if (drawPtr->flags & CLOSED) {
        drawPtr->flags &= ~CLOSED;
    } else {
        drawPtr->flags |= CLOSED;
    }
    SetDrawerVariable(drawPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawersetInstCmdDeleteProc --
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
DrawersetInstCmdDeleteProc(ClientData clientData)
{
    Drawerset *setPtr = clientData;

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
 * Drawerset operations.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec drawersetOps[] =
{
    {"add",        1, AddOp,       2, 0, "?label? ?option value?...",},
    {"cget",       2, CgetOp,      3, 3, "option",},
    {"close",      2, CloseOp,     3, 3, "drawerName",},
    {"configure",  2, ConfigureOp, 2, 0, "?option value ...?",},
    {"delete",     2, DeleteOp,    3, 3, "?drawerName ...?",},
    {"drawer",     2, DrawerOp,    2, 0, "oper ?args?",},
    {"exists",     1, ExistsOp,    3, 3, "drawerName",},
    {"handle",     1, HandleOp,    2, 0, "oper ?args?",},
    {"index",      3, IndexOp,     3, 3, "drawerName",},
    {"insert",     3, InsertOp,    4, 0, "after|before who ?label? ?option value ...?",},
    {"isopen",     2, IsOpenOp,    3, 3, "drawerName",},
    {"lower",      1, LowerOp,     3, 3, "drawerName",},
    {"move",       1, MoveOp,      3, 0, "before|after whereName drawerName",},
    {"names",      1, NamesOp,     2, 0, "?pattern...?",},
    {"open",       1, OpenOp,      3, 3, "drawerName",},
    {"raise",      1, RaiseOp,     3, 3, "drawerName",},
    {"size",       2, SizeOp,      2, 4, "?drawerName? ?numPixels?",},
    {"slide",      2, SlideOp,     5, 5, "drawerName dx dy"},
    {"tag",        2, TagOp,       2, 0, "oper args",},
    {"toggle",     2, ToggleOp,    3, 3, "drawerName",},
};

static int numDrawersetOps = sizeof(drawersetOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * DrawersetInstCmdProc --
 *
 *      This procedure is invoked to process the TCL command that
 *      corresponds to the drawerset geometry manager.  See the user
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
static int
DrawersetInstCmdProc(
    ClientData clientData,              /* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numDrawersetOps, drawersetOps, BLT_OP_ARG1, 
                objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}


/*
 *---------------------------------------------------------------------------
 *
 * DrawersetCmdProc --
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
DrawersetCmdProc(
    ClientData clientData,              /* Main window associated with
                                         * interpreter. */
    Tcl_Interp *interp,                 /* Current interpreter. */
    int objc,                           /* # of arguments. */
    Tcl_Obj *const *objv)               /* Argument strings. */
{
    Drawerset *setPtr;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * Invoke a procedure to initialize various bindings on treeview
     * entries.  If the procedure doesn't already exist, source it from
     * "$blt_library/bltDrawerset.tcl".  We deferred sourcing the file until
     * now so that the variable $blt_library could be set within a script.
     */
    if (!Blt_CommandExists(interp, "::blt::Drawerset::Initialize")) {
        char cmd[] = "source [file join $blt_library bltDrawerset.tcl]\n";

        if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
            char info[200];
            Blt_FormatString(info, 200, 
                             "\n    (while loading bindings for %.50s)", 
                             Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            return TCL_ERROR;
        }
    }
    setPtr = NewDrawerset(interp, objv[1]);
    if (setPtr == NULL) {
        goto error;
    }

    if (Blt_ConfigureWidgetFromObj(interp, setPtr->tkwin, drawersetSpecs, 
        objc - 2, objv + 2, (char *)setPtr, 0) != TCL_OK) {
        goto error;
    }
    ConfigureDrawerset(setPtr);
    Tcl_SetObjResult(interp, objv[1]);
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
 * Blt_DrawersetCmdInitProc --
 *
 *      This procedure is invoked to initialize the TCL command that
 *      corresponds to the drawerset geometry manager.
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
Blt_DrawersetCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"drawerset", DrawersetCmdProc};

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
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
 *      The widgets in the drawerset are possibly resized and redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayProc(ClientData clientData)
{
    Drawerset *setPtr = clientData;
    int w, h;
    
    setPtr->flags &= ~REDRAW_PENDING;
    w = Tk_Width(setPtr->tkwin);
    h = Tk_Height(setPtr->tkwin);
#if TRACE
    fprintf(stderr, "DisplayProc(%s) w=%d h=%d\n",
            Tk_PathName(setPtr->tkwin), w, h);
#endif
    if ((w <= 1) || (h <=1)) {
        /* Don't bother computing the layout until the size of the window
         * is something reasonable. */
        return;
    }
    if (setPtr->flags & LAYOUT_PENDING) {
        ComputeGeometry(setPtr);
    }
    if (!Tk_IsMapped(setPtr->tkwin)) {
        /* The drawerset's window isn't displayed, so don't bother drawing
         * anything.  By getting this far, we've at least computed the
         * coordinates of the new layout.  */
        return;
    }
    setPtr->numVisible = Blt_Chain_GetLength(setPtr->drawers);
    Blt_Bg_FillRectangle(setPtr->tkwin, Tk_WindowId(setPtr->tkwin), 
        setPtr->bg, 0, 0, w, h, setPtr->borderWidth, setPtr->relief);
    if (setPtr->base != NULL) {
        ArrangeBase(setPtr);
    }
    if (setPtr->numVisible > 0) {
        ArrangeDrawers(setPtr);
    }
    if (setPtr->flags & RESTACK) {
        setPtr->flags &= ~RESTACK;
        RestackDrawers(setPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayHandle
 *
 *      Draws the drawer's handle at its proper location.  First determines
 *      the size and position of the each window.  It then considers the
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
 *      The size of each drawer is re-initialized its minimum size.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayHandle(ClientData clientData)
{
    Blt_Bg bg;
    Drawable drawable;
    Drawer *drawPtr = clientData;
    Drawerset *setPtr;
    int relief;
    Handle *handlePtr;
    int w, h;
    
    drawPtr->flags &= ~REDRAW_PENDING;
    handlePtr = &drawPtr->handle;
    if (handlePtr->tkwin == NULL) {
        return;
    }
    setPtr = drawPtr->setPtr;
    if (setPtr->activePtr == drawPtr) {
        bg = CHOOSE(drawPtr->handle.activeBg, setPtr->activeHandleBg);
        relief = setPtr->activeRelief;
    } else {
        bg = CHOOSE(drawPtr->handle.bg, setPtr->handleBg);
        relief = setPtr->relief;
    }
    drawable = Tk_WindowId(handlePtr->tkwin);
    w = Tk_Width(handlePtr->tkwin);
    h = Tk_Height(handlePtr->tkwin);
    if ((w > 0) && (h > 0)) {
        Blt_Bg_FillRectangle(handlePtr->tkwin, drawable, bg, 
                0, 0, w, h, 0, TK_RELIEF_FLAT);
    }
    w -= PADDING(setPtr->handlePad);
    h -= PADDING(setPtr->handlePad);
    if ((w > 0) && (h > 0)) {
        Blt_Bg_DrawRectangle(handlePtr->tkwin, drawable, bg, 
                setPtr->handlePad.side1, setPtr->handlePad.side1, w, h, 
                setPtr->handleBW, relief);
    }
}

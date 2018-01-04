/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltFilmstrip.c --
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
   (((t)->attr != NULL) ? (t)->attr : (t)->filmPtr->attr)
#define VPORTWIDTH(s) \
    ((s)->flags & VERTICAL) ? Tk_Height((s)->tkwin) : Tk_Width((s)->tkwin);

#define TRACE   0
#define TRACE1  0

#define SCREEN(s,x)     ((x) - (s)->scrollOffset) 
#define WORLD(s,x)      ((x) + (s)->scrollOffset) 
#define ISHORIZ(s)      (((s)->flags & VERTICAL) == 0)
#define WORLDSIZE(s) \
    (((s)->flags & VERTICAL) ? s->worldHeight : s->worldWidth)

typedef enum {
    INSERT_AFTER,                       /* Insert after named frame. */
    INSERT_BEFORE                       /* Insert before named frame. */
} InsertOrder;

typedef struct _Filmstrip Filmstrip;
typedef struct _Frame Frame;
typedef struct _Grip Grip;
typedef int (LimitsProc)(int value, Blt_Limits *limitsPtr);
typedef int (SizeProc)(Frame *framePtr);

/*
 * Default values for widget attributes.
 */
#define DEF_ACTIVE_GRIP_COLOR   STD_ACTIVE_BACKGROUND
#define DEF_ACTIVE_GRIP_RELIEF  "flat"
#define DEF_ANCHOR              "c"
#define DEF_ANIMATE             "0"
#define DEF_BACKGROUND          STD_NORMAL_BACKGROUND
#define DEF_FRAME_ACTIVE_RELIEF "flat"
#define DEF_FRAME_ANCHOR        "c"
#define DEF_FRAME_BORDERWIDTH   "0"
#define DEF_FRAME_BORDERWIDTH   "0"
#define DEF_FRAME_DATA          (char *)NULL
#define DEF_FRAME_DELETE_COMMAND (char *)NULL
#define DEF_FRAME_FILL          "none"
#define DEF_FRAME_HIDE          "0"
#define DEF_FRAME_HIGHLIGHT_BACKGROUND   STD_NORMAL_BACKGROUND
#define DEF_FRAME_HIGHLIGHT_COLOR        RGB_BLACK
#define DEF_FRAME_PADX          "0"
#define DEF_FRAME_PADY          "0"
#define DEF_FRAME_RELIEF        "flat"
#define DEF_FRAME_RESIZE        "shrink"
#define DEF_FRAME_SHOW_GRIP     "1"
#define DEF_FRAME_STEPS         "8"
#define DEF_FRAME_TAGS          (char *)NULL
#define DEF_FRAME_VARIABLE      (char *)NULL
#define DEF_GRIP_BORDERWIDTH    "1"
#define DEF_GRIP_COLOR          STD_NORMAL_BACKGROUND
#define DEF_GRIP_CURSOR         (char *)NULL
#define DEF_GRIP_HIGHLIGHT_BACKGROUND   STD_NORMAL_BACKGROUND
#define DEF_GRIP_HIGHLIGHT_COLOR        RGB_BLACK
#define DEF_GRIP_PAD            "0"
#define DEF_GRIP_PAD            "0"
#define DEF_GRIP_RELIEF         "flat"
#define DEF_GRIP_THICKNESS      "3"
#define DEF_HCURSOR             "sb_h_double_arrow"
#define DEF_HEIGHT              "0"
#define DEF_MODE                "givetake"
#define DEF_ORIENT              "horizontal"
#define DEF_PAD                 "0"
#define DEF_POST_SCROLL_COMMAND (char *)NULL
#define DEF_RELATIVE_HEIGHT     "0.0"
#define DEF_RELATIVE_WIDTH      "0.0"
#define DEF_SCROLL_COMMAND      (char *)NULL
#define DEF_SCROLL_DELAY        "30"
#define DEF_SCROLL_INCREMENT    "10"
#define DEF_SHOW_GRIP           "1"
#define DEF_SIDE                "right"
#define DEF_TAKEFOCUS           "1"
#define DEF_VCURSOR             "sb_v_double_arrow"
#define DEF_WIDTH               "0"

#define FRAME_DEF_ANCHOR         TK_ANCHOR_C
#define FRAME_DEF_FILL           FILL_BOTH
#define FRAME_DEF_IPAD           0
#define FRAME_DEF_PAD            0
#define FRAME_DEF_PAD            0
#define FRAME_DEF_RESIZE         RESIZE_BOTH

#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))
#define VAR_FLAGS (TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS)

struct _Filmstrip {
    unsigned int flags;                 /* See the flags definitions
                                         * below. */
    Display *display;                   /* Display of the widget. */
    Tk_Window tkwin;                    /* The container window into which
                                         * other widgets are arranged. */
    Tcl_Interp *interp;                 /* Interpreter associated with all
                                         * widgets and grips. */
    Tcl_Command cmdToken;               /* Command token associated with
                                         * this widget.  */
    const char *name;                   /* The generated name of the frame
                                         * or the pathname of the window
                                         * created. */
    int side;                           /* Side where grip is located. */
    int normalWidth;                    /* Normal dimensions of the
                                         * filmstrip */
    int normalHeight;
    int reqWidth, reqHeight;            /* Constraints on the filmstrip's
                                         * normal width and
                                         * height. Overrides the requested
                                         * width of the window. */

    Tk_Cursor defVertCursor;            /* Default vertical cursor */
    Tk_Cursor defHorzCursor;            /* Default horizontal cursor */

    float relWidth, relHeight;          /* Relative size of frame's
                                         * width/height to widget's window
                                         * size. */
    short int width, height;            /* Requested size of the widget. */

    Blt_Bg bg;                          /* Background color of film strip. */
    /*
     * Scrolling information.
     */
    int worldWidth, worldHeight;
    int scrollOffset;                   /* Offset of viewport in world
                                         * coordinates. */
    Tcl_Obj *scrollCmdObjPtr;           /* Command strings to control
                                         * scrollbar.*/
    Tcl_Obj *postScrollCmdObjPtr;       /* Command to be executed when 
                                         * scrolling stops. */

    /* 
     * Automated scrolling information. 
     */
#ifdef notdef
    int numSteps;                       /* Number of steps to take to
                                         * scroll to proper frame. */
#endif
    int step;                           /* Current step in animation */

    int scrollUnits;                    /* Smallest unit of scrolling for
                                         * frames. */
    int scrollDistance;                 /* Distance to scroll. */
    int scrollTarget;                   /* Target offset to scroll to. */
    int scrollIncr;                     /* Current increment. */
    int delay;                          /* Current increment. */
    Tcl_TimerToken timerToken;          /* Token for timer to automatically
                                         * scroll the frame. */

    /*
     * Focus highlight ring 
     */
    XColor *gripHighlightColor;         /* Color for drawing traversal
                                         * highlight. */
    int gripRelief;
    int gripActiveRelief;
    Blt_Pad gripPad;
    int gripBorderWidth;
    int gripThickness;
    int gripSize;
    
    Blt_Bg gripBg;
    Blt_Bg activeGripBg;
    Frame *currentPtr;
    int anchor;                         /* Anchor of frames in
                                         * filmstrip. This only matters if
                                         * there is more space */
    int gripAnchor;                     /* Last known location of grip
                                         * during a move. */
    Blt_Chain frames;                   /* List of frames.  Describes the
                                         * order of the frames in the
                                         * widget.  */

    Blt_HashTable frameTable;           /* Table of frames.  Serves as a
                                         * directory to look up frames from
                                         * windows. */
    Blt_HashTable gripTable;            /* Table of grips.  Serves as a
                                         * directory to look up frames from
                                         * grip windows. */
    struct _Blt_Tags tags;              /* Table of tags. */
    Grip *activePtr;                    /* Indicates the active grip. */
    Grip *anchorPtr;                    /* Grip of frame that is currently
                                         * anchored */
    size_t numVisible;                  /* # of visible frames. */
    GC gc;
    size_t nextId;                      /* Counter to generate unique frame
                                         * names. */
    size_t nextGripId;                  /* Counter to generate unique frame
                                         * grip names. */
};

/*
 * Filmstrip flags definitions
 */
#define REDRAW_PENDING  (1<<0)          /* A redraw request is pending. */
#define LAYOUT_PENDING  (1<<1)          /* Get the requested sizes of the
                                         * widgets before
                                         * expanding/shrinking the size of
                                         * the container.  It's necessary
                                         * to recompute the layout every
                                         * time a frame is added,
                                         * reconfigured, or deleted, but
                                         * not when the container is
                                         * resized. */
#define SCROLL_PENDING  (1<<2)          /* Get the requested sizes of the
                                         * widgets before
                                         * expanding/shrinking the size of
                                         * the container.  It's necessary
                                         * to recompute the layout every
                                         * time a frame is added,
                                         * reconfigured, or deleted, but
                                         * not when the container is
                                         * resized. */
#define ANIMATE         (1<<3)          /* Animate frame moves. */

#define FOCUS           (1<<6)

#define VERTICAL        (1<<7)

/*
 * Grip --
 *
 *      A grip is the thin area where the frame may be gripped.
 */
struct _Grip {
    Frame *framePtr;                    /* Frame to which this grip
                                         * belongs. */
    Tk_Window tkwin;                    /* Grip window to be managed. */
    Blt_HashEntry *hashPtr;             /* Pointer of this grip into the
                                         * filmstrip's hashtable of
                                         * grips. */
};

/*
 * Frame --
 *
 *      A frame contains two windows: the embedded child window and an
 *      optional small window uses to grip the frame.  It describes how the
 *      window should appear in the frame.  The grip is a rectangle on the
 *      far edge of the frame (horizontal right, vertical bottom). Grips
 *      may be hidden. Normally the grip of the last frame is not
 *      displayed.
 *
 *      Initially, the size of a frame consists of
 *       1. the requested size embedded window,
 *       2. any requested internal padding, and
 *       3. the size of the grip (if one is displayed). 
 *
 *      Note: There is no 3D border around the frame.  This can be added by
 *            embedding a frame.  This simplifies the widget so that there
 *            is only one window for the widget.  Windows outside of the
 *            boundary of the frame are occluded.
 */
struct _Frame  {
    Tk_Window tkwin;                    /* Widget to be managed. */
    Tk_Cursor cursor;                   /* X Cursor */
    const char *name;                   /* Name of frame */
    unsigned int flags;
    Filmstrip *filmPtr;                 /* Filmstrip widget managing this
                                         * frame. */
    int extBorderWidth;                 /* The external border width of the
                                         * widget. This is needed to check
                                         * if
                                         * Tk_Changes(tkwin)->border_width
                                         * changes. */
    int borderWidth;                    /* The width of the 3D border
                                         * around the exterior of the
                                         * frame. */
    int relief;
    XColor *highlightBgColor;           /* Color for drawing traversal
                                         * highlight area when highlight is
                                         * off. */
    Grip grip;
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
                                         * frame. */
    Blt_Pad padX;                       /* Extra padding placed left and
                                         * right of the widget. */
    Blt_Pad padY;                       /* Extra padding placed above and
                                         * below the widget */
    int iPadX, iPadY;                   /* Extra padding added to the
                                         * interior of the widget
                                         * (i.e. adds to the requested size
                                         * of the widget) */
    int fill;                           /* Indicates how the widget should
                                         * fill the frame it occupies. */
    int resize;                         /* Indicates if the frame should
                                         * expand/shrink. */
    short int width, height;            /* Size of frame, including
                                         * grip. */
    Blt_ChainLink link;                 /* Pointer of this frame into the
                                         * list of frames. */
    Blt_HashEntry *hashPtr;             /* Pointer of this frame into
                                         * hashtable of frames. */
    int index;                          /* Index of the frame. */
    int size;                           /* Current size of the frame. This
                                         * size is bounded by min and
                                         * max. */
    /*
     * nom and size perform similar duties.  I need to keep track of the
     * amount of space allocated to the frame (using size).  But at the
     * same time, I need to indicate that space can be parcelled out to
     * this frame.  If a nominal size was set for this frame, I don't want
     * to add space.
     */
    int nom;                            /* The nominal size (neither
                                         * expanded nor shrunk) of the
                                         * frame based upon the requested
                                         * size of the widget embedded in
                                         * this frame. */
    int min, max;                       /* Size constraints on the frame */
    Blt_Limits reqSize;                 /* Requested bounds for the size of
                                         * the frame. The frame will not
                                         * expand or shrink beyond these
                                         * limits, regardless of how it was
                                         * specified (max widget size).
                                         * This includes any extra padding
                                         * which may be specified. */
    Tcl_TimerToken timerToken;
    int scrollTarget;                   /* Target offset to scroll to. */
    int scrollIncr;                     /* Current increment. */
    Tcl_Obj *variableObjPtr;            /* Name of TCL variable.  If
                                         * non-NULL, this variable will be
                                         * set to the value string of the
                                         * selected item. */
    int worldX, worldY;
    Tcl_Obj *dataObjPtr;                /* User-defined data associated
                                         * with this frame. */
    Tcl_Obj *deleteCmdObjPtr;           /* If non-NULL, Routine to call
                                         * when frame is deleted. */
};

/* Frame/grip flags.  */

#define HIDDEN          (1<<8)          /* Do not display the frame. */
#define DISABLED        (1<<9)          /* Grip is disabled. */
#define ONSCREEN        (1<<10)         /* Frame is on-screen. */
#define GRIP_ACTIVE     (1<<11)         /* Grip is currently active. */
#define GRIP            (1<<12)         /* The frame has a grip. */
#define SHOW_GRIP       (1<<13)         /* Display the frame's grip. */

/* Orientation. */
#define SIDE_VERTICAL   (SIDE_TOP|SIDE_BOTTOM)
#define SIDE_HORIZONTAL (SIDE_LEFT|SIDE_RIGHT)

/* Grip positions. */
#define GRIP_LEFT     SIDE_RIGHT
#define GRIP_RIGHT    SIDE_LEFT
#define GRIP_TOP      SIDE_BOTTOM
#define GRIP_BOTTOM   SIDE_TOP

#define GRIP_FARSIDE  (GRIP_RIGHT|GRIP_BOTTOM)    
#define GRIP_NEARSIDE (GRIP_LEFT|GRIP_TOP)        


static Tk_GeomRequestProc FrameGeometryProc;
static Tk_GeomLostSlaveProc FrameCustodyProc;
static Tk_GeomMgr filmstripMgrInfo =
{
    (char *)"filmstrip",                /* Name of geometry manager used by
                                         * winfo */
    FrameGeometryProc,                  /* Procedure called for new
                                         * geometry requests */
    FrameCustodyProc,                   /* Procedure called when widget is
                                         * taken away */
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

static Blt_OptionFreeProc FreeTagsProc;
static Blt_OptionParseProc ObjToTags;
static Blt_OptionPrintProc TagsToObj;
static Blt_CustomOption tagsOption = {
    ObjToTags, TagsToObj, FreeTagsProc, (ClientData)0
};

static Blt_ConfigSpec frameSpecs[] =
{
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_FRAME_BORDERWIDTH, Blt_Offset(Frame, borderWidth),
        BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_OBJ, "-data", "data", "Data", DEF_FRAME_DATA, 
        Blt_Offset(Frame, dataObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-deletecommand", "deleteCommand", "DeleteCommand",
        DEF_FRAME_DELETE_COMMAND, Blt_Offset(Frame, deleteCmdObjPtr),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FILL, "-fill", "fill", "Fill", DEF_FRAME_FILL, 
        Blt_Offset(Frame, fill), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-height", "height", "Height", (char *)NULL, 
        Blt_Offset(Frame, reqHeight), 0, &bltLimitsOption},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_FRAME_HIDE, 
        Blt_Offset(Frame, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDDEN },
    {BLT_CONFIG_PIXELS_NNEG, "-ipadx", "iPadX", "IPadX", (char *)NULL,
        Blt_Offset(Frame, iPadX), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-ipady", "iPadY", "IPadY", (char *)NULL,
        Blt_Offset(Frame, iPadY), 0},
    {BLT_CONFIG_PAD, "-padx", "padx", "PadX", (char *)NULL,
        Blt_Offset(Frame, padX), 0},
    {BLT_CONFIG_PAD, "-pady", "pady", "PadY", (char *)NULL,
        Blt_Offset(Frame, padY), 0},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_FRAME_RELIEF,
        Blt_Offset(Frame, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RESIZE, "-resize", "resize", "Resize", DEF_FRAME_RESIZE,
        Blt_Offset(Frame, resize), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-showgrip", "showGrip", "showGrip", 
        DEF_SHOW_GRIP, Blt_Offset(Frame, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SHOW_GRIP },
    {BLT_CONFIG_CUSTOM, "-size", "size", "Size", (char *)NULL, 
        Blt_Offset(Frame, reqSize), 0, &bltLimitsOption},
     {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_FRAME_TAGS, 0,
        BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
        DEF_TAKEFOCUS, Blt_Offset(Frame, takeFocus), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-width", "width", "Width", (char *)NULL, 
        Blt_Offset(Frame, reqWidth), 0, &bltLimitsOption},
    {BLT_CONFIG_CUSTOM, "-window", "window", "Window", (char *)NULL, 
        Blt_Offset(Frame, tkwin), BLT_CONFIG_NULL_OK, &childOption },
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec filmStripSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activegripcolor", "activeGripColor", 
        "GripColor", DEF_ACTIVE_GRIP_COLOR,
        Blt_Offset(Filmstrip, activeGripBg)},
    {BLT_CONFIG_RELIEF, "-activegriprelief", "activeGripRelief", 
        "GripRelief", DEF_ACTIVE_GRIP_RELIEF,
        Blt_Offset(Filmstrip, gripActiveRelief), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_ANCHOR,
        Blt_Offset(Filmstrip, anchor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-animate", "animate", "Animate", DEF_ANIMATE, 
        Blt_Offset(Filmstrip, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)ANIMATE },
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BACKGROUND, Blt_Offset(Filmstrip, bg), 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
    {BLT_CONFIG_PIXELS_NNEG, "-gripborderwidth", "gripBorderWidth", 
        "GripBorderWidth", DEF_GRIP_BORDERWIDTH, 
        Blt_Offset(Filmstrip, gripBorderWidth), BLT_CONFIG_DONT_SET_DEFAULT },
    {BLT_CONFIG_BACKGROUND, "-gripcolor", "gripColor", "GripColor",
        DEF_GRIP_COLOR, Blt_Offset(Filmstrip, gripBg), 0},
    {BLT_CONFIG_PAD, "-grippad", "gripPad", "GripPad", DEF_GRIP_PAD, 
        Blt_Offset(Filmstrip, gripPad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-griprelief", "gripRelief", "GripRelief", 
        DEF_GRIP_RELIEF, Blt_Offset(Filmstrip, gripRelief),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-gripthickness", "gripThickness", 
        "GripThickness", DEF_GRIP_THICKNESS,
        Blt_Offset(Filmstrip, gripThickness), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_HEIGHT,
        Blt_Offset(Filmstrip, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-orient", "orient", "Orient", DEF_ORIENT, 
        Blt_Offset(Filmstrip, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        &orientOption},
    {BLT_CONFIG_OBJ, "-postscrollcommand", "postScrollCommand",
        "PostScrollCommand", DEF_POST_SCROLL_COMMAND,
        Blt_Offset(Filmstrip, postScrollCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FLOAT, "-relheight", "relHeight", "RelHeight",
        DEF_RELATIVE_HEIGHT, Blt_Offset(Filmstrip, relHeight),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FLOAT, "-relwidth", "relWidth", "RelWidth", DEF_RELATIVE_WIDTH,
        Blt_Offset(Filmstrip, relWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-scrollcommand", "scrollCommand", "ScrollCommand",
        DEF_SCROLL_COMMAND, Blt_Offset(Filmstrip, scrollCmdObjPtr),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_INT_NNEG, "-scrolldelay", "scrollDelay", "ScrollDelay",
        DEF_SCROLL_DELAY, Blt_Offset(Filmstrip, delay),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_POS, "-scrollincrement", "scrollIncrement",
        "ScrollIncrement", DEF_SCROLL_INCREMENT,
        Blt_Offset(Filmstrip,scrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
#ifdef notdef
    {BLT_CONFIG_INT_POS, "-steps", "steps", "Steps", DEF_FRAME_STEPS,
        Blt_Offset(Filmstrip, numSteps), BLT_CONFIG_DONT_SET_DEFAULT},
#endif
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_WIDTH,
        Blt_Offset(Filmstrip, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

/*
 * FrameIterator --
 *
 *      Frames may be tagged with strings.  A frame may have many tags.
 *      The same tag may be used for many frames.
 *      
 */
typedef enum { 
    ITER_SINGLE, ITER_ALL, ITER_TAG, ITER_PATTERN, 
} IteratorType;

typedef struct _Iterator {
    Filmstrip *filmPtr;                 /* Filmstrip that we're iterating
                                         * over. */
    IteratorType type;                  /* Type of iteration:
                                         * ITER_TAG      By item tag.
                                         * ITER_ALL      By every item.
                                         * ITER_SINGLE   Single item: either 
                                         *               tag or index.
                                         * ITER_PATTERN  Over a consecutive 
                                         *               range of indices.
                                         */
    Frame *startPtr;                     /* Starting frame.  Starting point
                                         * of search, saved if iterator is
                                         * reused.  Used for ITER_ALL and
                                         * ITER_SINGLE searches. */
    Frame *endPtr;                       /* Ending pend (inclusive). */
    Frame *nextPtr;                      /* Next frame. */

    /* For tag-based searches. */
    const char *tagName;                /* If non-NULL, is the tag that we
                                         * are currently iterating over. */
    Blt_ChainLink link;
} FrameIterator;


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

/*
 * Forward declarations
 */
static Tcl_FreeProc FilmstripFreeProc;
static Tcl_IdleProc DisplayProc;
static Tcl_IdleProc DisplayGrip;
static Tcl_ObjCmdProc FilmstripCmd;
static Tcl_ObjCmdProc FilmstripCmd;
static Tk_EventProc FilmstripEventProc;
static Tk_EventProc ChildEventProc;
static Tk_EventProc GripEventProc;
static Tcl_FreeProc FrameFreeProc;
static Tcl_ObjCmdProc FilmstripInstCmdProc;
static Tcl_CmdDeleteProc FilmstripInstCmdDeleteProc;
static Tcl_TimerProc MotionTimerProc;

static int GetFrameIterator(Tcl_Interp *interp, Filmstrip *filmPtr,
        Tcl_Obj *objPtr, FrameIterator *iterPtr);
static int GetFrameFromObj(Tcl_Interp *interp, Filmstrip *filmPtr,
        Tcl_Obj *objPtr, Frame **framePtrPtr);

static int
BoundSize(int size, Blt_Limits *limitsPtr)    
{
    /*
     * Check widgets for requested width values;
     */
    if (limitsPtr->flags & LIMITS_NOM_SET) {
        size = limitsPtr->nom;          /* Override initial value */
    }
    if (size < limitsPtr->min) {
        size = limitsPtr->min;          /* Bounded by minimum value */
    }
    if (size > limitsPtr->max) {
        size = limitsPtr->max;          /* Bounded by maximum value */
    }
    return size;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetReqWidth --
 *
 *      Returns the width requested by the window embedded in the given
 *      frame.  The requested space also includes any internal padding
 *      which has been designated for this widget.
 *
 *      The requested width of the widget is always bounded by the limits
 *      set in framePtr->reqWidth.
 *
 * Results:
 *      Returns the requested width of the widget.
 *
 *---------------------------------------------------------------------------
 */
static int
GetReqWidth(Frame *framePtr)
{
    int w;

    w = (2 * framePtr->iPadX);           /* Start with any addition padding
                                          * requested for the frame. */
    if (framePtr->tkwin != NULL) {       /* Add in the requested width. */
        w += Tk_ReqWidth(framePtr->tkwin);
    }
    return BoundSize(w, &framePtr->reqWidth);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetReqHeight --
 *
 *      Returns the height requested by the widget starting in the given
 *      frame.  The requested space also includes any internal padding
 *      which has been designated for this widget.
 *
 *      The requested height of the widget is always bounded by the limits
 *      set in framePtr->reqHeight.
 *
 * Results:
 *      Returns the requested height of the widget.
 *
 *---------------------------------------------------------------------------
 */
static int
GetReqHeight(Frame *framePtr)
{
    int h;

    h = 2 * framePtr->iPadY;
    if (framePtr->tkwin != NULL) {
        h += Tk_ReqHeight(framePtr->tkwin);
    }
    h = BoundSize(h, &framePtr->reqHeight);
    return h;
}

static void
EventuallyRedraw(Filmstrip *filmPtr)
{
    if ((filmPtr->flags & REDRAW_PENDING) == 0) {
        filmPtr->flags |= REDRAW_PENDING;
        Tcl_DoWhenIdle(DisplayProc, filmPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SetTag --
 *
 *      Associates a tag with a given row.  Individual row tags are stored
 *      in hash tables keyed by the tag name.  Each table is in turn stored
 *      in a hash table keyed by the row location.
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
SetTag(Tcl_Interp *interp, Frame *framePtr, Tcl_Obj *objPtr)
{
    Filmstrip *filmPtr;
    const char *string;
    char c;
    
    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((strcmp(string, "all") == 0) || (strcmp(string, "end") == 0)) {
        return TCL_OK;                  /* Don't need to create reserved
                                         * tags. */
    }
    if (c == '\0') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", string, "\" can't be empty.", 
                (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (c == '-') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", string, 
                "\" can't start with a '-'.", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if ((isdigit(c)) && (Blt_ObjIsInteger(objPtr))) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", string, "\" can't be a number.",
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    filmPtr = framePtr->filmPtr;
    Blt_Tags_AddItemToTag(&filmPtr->tags, string, framePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyFrame --
 *
 *      Removes the Frame structure from the hash table and frees the
 *      memory allocated by it.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the frame is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyFrame(Frame *framePtr)
{
    Filmstrip *filmPtr;
    Grip *gripPtr;
    
    filmPtr = framePtr->filmPtr;
    gripPtr = &framePtr->grip;
    if (framePtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(framePtr->timerToken);
        framePtr->timerToken = 0;
    }
    if (framePtr->tkwin != NULL) {
        Tk_DeleteEventHandler(framePtr->tkwin, StructureNotifyMask,
                ChildEventProc, framePtr);
        Tk_ManageGeometry(framePtr->tkwin, (Tk_GeomMgr *)NULL, framePtr);
        if (Tk_IsMapped(framePtr->tkwin)) {
            Tk_UnmapWindow(framePtr->tkwin);
        }
    }
    if (framePtr->deleteCmdObjPtr != NULL) {
        if (Tcl_EvalObjEx(filmPtr->interp, framePtr->deleteCmdObjPtr,
                TCL_EVAL_GLOBAL) != TCL_OK) {
            Tcl_BackgroundError(filmPtr->interp);
        }
    }
    if (gripPtr->tkwin != NULL) {
        Tk_Window tkwin;

        tkwin = gripPtr->tkwin;
        Tk_DeleteEventHandler(gripPtr->tkwin, 
                ExposureMask|FocusChangeMask|StructureNotifyMask, 
                GripEventProc, framePtr);
        Tk_ManageGeometry(tkwin, (Tk_GeomMgr *)NULL, gripPtr);
        gripPtr->tkwin = NULL;
        Tk_DestroyWindow(tkwin);
    }
    Blt_Tags_ClearTagsFromItem(&filmPtr->tags, framePtr);
    Blt_FreeOptions(frameSpecs, (char *)framePtr, filmPtr->display, 0);
    if (filmPtr->anchorPtr == gripPtr) {
        filmPtr->anchorPtr = NULL;
    }
    if (framePtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&filmPtr->frameTable, framePtr->hashPtr);
        framePtr->hashPtr = NULL;
    }
    if (framePtr->link != NULL) {
        Blt_Chain_DeleteLink(filmPtr->frames, framePtr->link);
        framePtr->link = NULL;
    }
    if (gripPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&filmPtr->gripTable, gripPtr->hashPtr);
        gripPtr->hashPtr = NULL;
    }
    Blt_Free(framePtr);
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
 *      Otherwise, TCL_ERROR is returned and an error message is left in
 *      interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToChild(ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    Frame *framePtr = (Frame *)widgRec;
    Filmstrip *filmPtr;
    Tk_Window *tkwinPtr = (Tk_Window *)(widgRec + offset);
    Tk_Window old, tkwin;
    char *string;

    old = *tkwinPtr;
    tkwin = NULL;
    filmPtr = framePtr->filmPtr;
    string = Tcl_GetString(objPtr);
    if (string[0] != '\0') {
        tkwin = Tk_NameToWindow(interp, string, filmPtr->tkwin);
        if (tkwin == NULL) {
            return TCL_ERROR;
        }
        if (tkwin == old) {
            return TCL_OK;
        }
        /*
         * Allow only widgets that are children of the filmstrip window to
         * be used.  We are using the filmstrip window as a viewport to
         * clip the children as needed.
         */
        parent = Tk_Parent(tkwin);
        if (parent != filmPtr->tkwin) {
            Tcl_AppendResult(interp, "can't manage \"", Tk_PathName(tkwin),
                "\" in filmstrip \"", Tk_PathName(filmPtr->tkwin), "\"",
                (char *)NULL);
            return TCL_ERROR;
        }
        Tk_ManageGeometry(tkwin, &filmstripMgrInfo, framePtr);
        Tk_CreateEventHandler(tkwin, StructureNotifyMask, ChildEventProc, 
                framePtr);
    }
    if (old != NULL) {
        Tk_DeleteEventHandler(old, StructureNotifyMask, ChildEventProc,
                framePtr);
        Tk_ManageGeometry(old, (Tk_GeomMgr *)NULL, framePtr);
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
 *      Converts the Tk window back to a Tcl_Obj (its pathname).
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
 * ObjToOrient --
 *
 *      Converts the string representing a orientation into a bitflag.
 *
 * Results:
 *      The return value is a standard TCL result.  The state flags are
 *      updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToOrient(ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    Filmstrip *filmPtr = (Filmstrip *)(widgRec);
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
    filmPtr->flags |= LAYOUT_PENDING;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OrientToObj --
 *
 *      Returns the name of the current orientation as a Tcl_Obj.
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
    Filmstrip *filmPtr;
    Frame *framePtr = (Frame *)widgRec;

    filmPtr = framePtr->filmPtr;
    Blt_Tags_ClearTagsFromItem(&filmPtr->tags, framePtr);
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
    Filmstrip *filmPtr;
    Frame *framePtr = (Frame *)widgRec;
    int i;
    char *string;
    int objc;
    Tcl_Obj **objv;

    filmPtr = framePtr->filmPtr;
    Blt_Tags_ClearTagsFromItem(&filmPtr->tags, framePtr);
    string = Tcl_GetString(objPtr);
    if ((string[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
        return TCL_OK;
    }
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
        SetTag(interp, framePtr, objv[i]);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagsToObj --
 *
 *      Return a TCL list of tags for the frame.
 *
 * Results:
 *      The tags associated with the frame are returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TagsToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window parent,
          char *widgRec, int offset, int flags)  
{
    Filmstrip *filmPtr;
    Frame *framePtr = (Frame *)widgRec;
    Tcl_Obj *listObjPtr;

    filmPtr = framePtr->filmPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Blt_Tags_AppendTagsToObj(&filmPtr->tags, framePtr, listObjPtr);
    return listObjPtr;
}

static void
EventuallyRedrawGrip(Grip *gripPtr)
{
    if ((gripPtr->framePtr->flags & REDRAW_PENDING) == 0) {
        gripPtr->framePtr->flags |= REDRAW_PENDING;
        Tcl_DoWhenIdle(DisplayGrip, gripPtr);
    }
}

static Frame *
FirstFrame(Filmstrip *filmPtr, unsigned int hateFlags)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(filmPtr->frames); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Frame *framePtr;

        framePtr = Blt_Chain_GetValue(link);
        if ((framePtr->flags & hateFlags) == 0) {
            return framePtr;
        }
    }
    return NULL;
}

static Frame *
LastFrame(Filmstrip *filmPtr, unsigned int hateFlags)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(filmPtr->frames); link != NULL;
         link = Blt_Chain_PrevLink(link)) {
        Frame *framePtr;

        framePtr = Blt_Chain_GetValue(link);
        if ((framePtr->flags & hateFlags) == 0) {
            return framePtr;
        }
    }
    return NULL;
}

static Frame *
NextFrame(Frame *framePtr, unsigned int hateFlags)
{
    if (framePtr != NULL) {
        Blt_ChainLink link;

        for (link = Blt_Chain_NextLink(framePtr->link); link != NULL; 
             link = Blt_Chain_NextLink(link)) {
            framePtr = Blt_Chain_GetValue(link);
            if ((framePtr->flags & hateFlags) == 0) {
                return framePtr;
            }
        }
    }
    return NULL;
}

static Frame *
PrevFrame(Frame *framePtr, unsigned int hateFlags)
{
    if (framePtr != NULL) {
        Blt_ChainLink link;
        
        for (link = Blt_Chain_PrevLink(framePtr->link); link != NULL; 
             link = Blt_Chain_PrevLink(link)) {
            framePtr = Blt_Chain_GetValue(link);
            if ((framePtr->flags & hateFlags) == 0) {
                return framePtr;
            }
        }
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilmstripEventProc --
 *
 *      This procedure is invoked by the Tk event handler when the
 *      container widget is reconfigured or destroyed.
 *
 *      The filmstrip will be rearranged at the next idle point if the
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
 *      Arranges for the filmstrip associated with tkwin to have its layout
 *      re-computed and drawn at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static void
FilmstripEventProc(ClientData clientData, XEvent *eventPtr)
{
    Filmstrip *filmPtr = clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedraw(filmPtr);
        }
    } else if (eventPtr->type == DestroyNotify) {
        if (filmPtr->tkwin != NULL) {
            Blt_DeleteWindowInstanceData(filmPtr->tkwin);
            filmPtr->tkwin = NULL;
            Tcl_DeleteCommandFromToken(filmPtr->interp, filmPtr->cmdToken);
        }
        if (filmPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayProc, filmPtr);
        }
        Tcl_EventuallyFree(filmPtr, FilmstripFreeProc);
    } else if (eventPtr->type == ConfigureNotify) {
        Frame *framePtr;

        /* Reset anchor frame. */
        framePtr = LastFrame(filmPtr, HIDDEN); 
        filmPtr->anchorPtr = &framePtr->grip;

        filmPtr->flags |= LAYOUT_PENDING;
        EventuallyRedraw(filmPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildEventProc --
 *
 *      This procedure is invoked by the Tk event handler when
 *      StructureNotify events occur in a child widget managed by the
 *      filmstrip.
 *
 *      For example, when a managed widget is destroyed, it frees the
 *      corresponding frame structure and arranges for the filmstrip layout
 *      to be re-computed at the next idle point.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      If the managed widget was deleted, the Frame structure gets cleaned
 *      up and the filmstrip is rearranged.
 *
 *---------------------------------------------------------------------------
 */
static void
ChildEventProc(
    ClientData clientData,              /* Pointer to Frame structure for
                                         * widget referred to by eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    Frame *framePtr = (Frame *)clientData;
    Filmstrip *filmPtr = framePtr->filmPtr;

    if (eventPtr->type == ConfigureNotify) {
        int extBorderWidth;

        if (framePtr->tkwin == NULL) {
            return;
        }
        extBorderWidth = Tk_Changes(framePtr->tkwin)->border_width;
        if (framePtr->extBorderWidth != extBorderWidth) {
            framePtr->extBorderWidth = extBorderWidth;
            EventuallyRedraw(filmPtr);
        }
    } else if (eventPtr->type == DestroyNotify) {
        if (framePtr->tkwin != NULL) {
            Tcl_EventuallyFree(framePtr, FrameFreeProc);
        }
        filmPtr->flags |= LAYOUT_PENDING;
        EventuallyRedraw(filmPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FrameCustodyProc --
 *
 *      This procedure is invoked when a widget has been stolen by another
 *      geometry manager.  The information and memory associated with the
 *      widget is released.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the filmstrip to have its layout recomputed at the next
 *      idle point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
FrameCustodyProc(ClientData clientData, Tk_Window tkwin)
{
    Frame *framePtr = (Frame *)clientData;
    Filmstrip *filmPtr = framePtr->filmPtr;

    if (Tk_IsMapped(framePtr->tkwin)) {
        Tk_UnmapWindow(framePtr->tkwin);
    }
    DestroyFrame(framePtr);
    filmPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(filmPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FrameGeometryProc --
 *
 *      This procedure is invoked by Tk_GeometryRequest for widgets managed
 *      by the filmstrip geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the filmstrip to have its layout re-computed and
 *      re-arranged at the next idle point.
 *
 * ----------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
FrameGeometryProc(ClientData clientData, Tk_Window tkwin)
{
    Frame *framePtr = (Frame *)clientData;

    framePtr->filmPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(framePtr->filmPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GripEventProc --
 *
 *      This procedure is invoked by the Tk event handler when various
 *      events occur in the frame grip subwindow maintained by this
 *      widget.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
GripEventProc(
    ClientData clientData,              /* Pointer to Frame structure for
                                         * grip referred to by
                                         * eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    Grip *gripPtr = (Grip *)clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedrawGrip(gripPtr);
        }
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
        if (eventPtr->xfocus.detail != NotifyInferior) {
            if (eventPtr->type == FocusIn) {
                gripPtr->framePtr->flags |= FOCUS;
            } else {
                gripPtr->framePtr->flags &= ~FOCUS;
            }
            EventuallyRedrawGrip(gripPtr);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        if (gripPtr->tkwin == NULL) {
            return;
        }
        EventuallyRedrawGrip(gripPtr);
    } else if (eventPtr->type == DestroyNotify) {
        gripPtr->tkwin = NULL;
    } 
}

/*
 *---------------------------------------------------------------------------
 *
 * SearchForVerticalFrame --
 *
 *      Finds the frame closest to the given scroll offset.
 *
 * Results:
 *      Returns the pointer to the closest frame. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Frame *
SearchForVerticalFrame(Filmstrip *filmPtr, int offset)
{
    Frame *framePtr;

    for (framePtr = FirstFrame(filmPtr, 0); framePtr != NULL;
         framePtr = NextFrame(framePtr, 0)) {
        if (offset >= (framePtr->worldY + framePtr->height)) {
            continue;                   /* Point is after frame */
        }
        if (offset < (framePtr->worldY + framePtr->height)) {
            return framePtr;
        }
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * SearchForHorizontalFrame --
 *
 *      Finds the frame closest to the given scroll offset.
 *
 * Results:
 *      Returns the pointer to the closest frame. 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Frame *
SearchForHorizontalFrame(Filmstrip *filmPtr, int offset)
{
    Frame *framePtr;

    for (framePtr = FirstFrame(filmPtr, 0); framePtr != NULL;
         framePtr = NextFrame(framePtr, 0)) {
        if (offset >= (framePtr->worldX + framePtr->width)) {
            continue;                   /* Point is after frame */
        }
        if (offset < (framePtr->worldX + framePtr->width)) {
            return framePtr;
        }
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * NearestFrame --
 *
 *      Finds the frame closest to the given screen X-Y coordinates in the
 *      viewport.
 *
 * Results:
 *      Returns the pointer to the closest frame.  If no frame is visible
 *      (frames may be hidden), NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Frame *
NearestFrame(Filmstrip *filmPtr, int x, int y)
{
    if (ISHORIZ(filmPtr)) {
        return SearchForHorizontalFrame(filmPtr, WORLD(filmPtr, x));
    } else {
        return SearchForVerticalFrame(filmPtr, WORLD(filmPtr, y));
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NextTaggedFrame --
 *
 *      Returns the next frame derived from the given tag.
 *
 * Results:
 *      Returns the pointer to the next frame in the iterator.  If no more
 *      frames are available, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Frame *
NextTaggedFrame(FrameIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
    case ITER_ALL:
        if (iterPtr->link != NULL) {
            Frame *framePtr;
            
            framePtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return framePtr;
        }
        break;
    case ITER_PATTERN:
        {
            Blt_ChainLink link;
            
            for (link = iterPtr->link; link != NULL; 
                 link = Blt_Chain_NextLink(link)) {
                Frame *framePtr;
                
                framePtr = Blt_Chain_GetValue(iterPtr->link);
                if (Tcl_StringMatch(framePtr->name, iterPtr->tagName)) {
                    iterPtr->link = Blt_Chain_NextLink(link);
                    return framePtr;
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
 * FirstTaggedFrame --
 *
 *      Returns the first frame derived from the given tag.
 *
 * Results:
 *      Returns the first frame in the sequence.  If no more frames are in
 *      the list, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Frame *
FirstTaggedFrame(FrameIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
    case ITER_ALL:
        if (iterPtr->link != NULL) {
            Frame *framePtr;
            
            framePtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return framePtr;
        }
        break;

    case ITER_PATTERN:
        {
            Blt_ChainLink link;
            
            for (link = iterPtr->link; link != NULL; 
                 link = Blt_Chain_NextLink(link)) {
                Frame *framePtr;
                
                framePtr = Blt_Chain_GetValue(iterPtr->link);
                if (Tcl_StringMatch(framePtr->name, iterPtr->tagName)) {
                    iterPtr->link = Blt_Chain_NextLink(link);
                    return framePtr;
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
 * GetFrameFromObj --
 *
 *      Gets the frame associated the given index, tag, or label.  This
 *      routine is used when you want only one frame.  It's an error if
 *      more than one frame is specified (e.g. "all" tag).  It's also an
 *      error if the tag is empty (no frames are currently tagged).
 *
 *---------------------------------------------------------------------------
 */
static int 
GetFrameFromObj(Tcl_Interp *interp, Filmstrip *filmPtr, Tcl_Obj *objPtr,
              Frame **framePtrPtr)
{
    FrameIterator iter;
    Frame *firstPtr;

    if (GetFrameIterator(interp, filmPtr, objPtr, &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    firstPtr = FirstTaggedFrame(&iter);
    if (firstPtr != NULL) {
        Frame *nextPtr;

        nextPtr = NextTaggedFrame(&iter);
        if (nextPtr != NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "multiple frames specified by \"", 
                        Tcl_GetString(objPtr), "\"", (char *)NULL);
            }
            return TCL_ERROR;
        }
    }
    *framePtrPtr = firstPtr;
    return TCL_OK;
}

static int
GetFrameByIndex(Tcl_Interp *interp, Filmstrip *filmPtr, const char *string, 
                int length, Frame **framePtrPtr)
{
    Frame *framePtr;
    char c;
    int64_t pos;

    framePtr = NULL;
    c = string[0];
    if (Blt_GetInt64(NULL, string, &pos) == TCL_OK) {
        Blt_ChainLink link;

        link = Blt_Chain_GetNthLink(filmPtr->frames, pos);
        if (link != NULL) {
            framePtr = Blt_Chain_GetValue(link);
        } 
        if (framePtr == NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "can't find frame: bad index \"", 
                        string, "\"", (char *)NULL);
            }
            return TCL_ERROR;
        }               
    } else if (c == '@') {
        int x, y;

        if (Blt_GetXY(filmPtr->interp, filmPtr->tkwin, string, &x, &y) 
            == TCL_OK) {
            framePtr = NearestFrame(filmPtr, x, y);
        }
    } else if ((c == 'a') && (strcmp(string, "active") == 0)) {
        if (filmPtr->activePtr != NULL) {
            framePtr = filmPtr->activePtr->framePtr;
        }
    } else if ((c == 'c') && (strcmp(string, "current") == 0)) {
        if (filmPtr->currentPtr != NULL) {
            framePtr = filmPtr->currentPtr;
        }
    } else if ((c == 'f') && (strcmp(string, "first") == 0)) {
        framePtr = FirstFrame(filmPtr, HIDDEN | DISABLED);
    } else if ((c == 'l') && (strcmp(string, "last") == 0)) {
        framePtr = LastFrame(filmPtr, HIDDEN | DISABLED);
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
        framePtr = LastFrame(filmPtr, 0);
    } else if ((c == 'n') && (strcmp(string, "next") == 0)) {
        if (filmPtr->currentPtr != NULL) {
            framePtr = NextFrame(filmPtr->currentPtr, HIDDEN | DISABLED);
        }
    } else if ((c == 'p') && (strncmp(string, "previous", length) == 0)) {
        if (filmPtr->currentPtr != NULL) {
            framePtr = PrevFrame(filmPtr->currentPtr, HIDDEN | DISABLED);
        }
    } else if ((c == 'n') && (strcmp(string, "none") == 0)) {
        framePtr = NULL;
    } else {
        return TCL_CONTINUE;
    }
    *framePtrPtr = framePtr;
    return TCL_OK;
}

static Frame *
GetFrameByName(Filmstrip *filmPtr, const char *string)
{
    Blt_HashEntry *hPtr;
    
    hPtr = Blt_FindHashEntry(&filmPtr->frameTable, string);
    if (hPtr == NULL) {
        return NULL;
    }
    return Blt_GetHashValue(hPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * GetFrameIterator --
 *
 *      Converts a string representing a frame index into an frame pointer.
 *      The index may be in one of the following forms:
 *
 *       number         Frame at index in the list of frames.
 *       @x,y           Frame closest to the specified X-Y screen coordinates.
 *       "active"       Frame where mouse pointer is located.
 *       "posted"       Frame is the currently posted cascade frame.
 *       "next"         Next frame from the focus frame.
 *       "previous"     Previous frame from the focus frame.
 *       "end"          Last frame.
 *       "none"         No frame.
 *
 *       number         Frame at position in the list of frames.
 *       @x,y           Frame closest to the specified X-Y screen coordinates.
 *       "active"       Frame mouse is located over.
 *       "focus"        Frame is the widget's focus.
 *       "select"       Currently selected frame.
 *       "right"        Next frame from the focus frame.
 *       "left"         Previous frame from the focus frame.
 *       "up"           Next frame from the focus frame.
 *       "down"         Previous frame from the focus frame.
 *       "end"          Last frame in list.
 *      "name:string"   Frame named "string".
 *      "index:number"  Frame at index number in list of frames.
 *      "tag:string"    Frame(s) tagged by "string".
 *      "label:pattern" Frame(s) with label matching "pattern".
 *      
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.  The
 *      pointer to the node is returned via framePtrPtr.  Otherwise,
 *      TCL_ERROR is returned and an error message is left in interpreter's
 *      result field.
 *
 *---------------------------------------------------------------------------
 */
static int
GetFrameIterator(Tcl_Interp *interp, Filmstrip *filmPtr, Tcl_Obj *objPtr,
                 FrameIterator *iterPtr)
{
    Frame *framePtr;
    Blt_Chain chain;
    char *string;
    char c;
    int numBytes;
    int length;
    int result;

    iterPtr->filmPtr = filmPtr;
    iterPtr->type = ITER_SINGLE;
    iterPtr->link = NULL;
    iterPtr->tagName = Tcl_GetStringFromObj(objPtr, &numBytes);
    iterPtr->nextPtr = NULL;
    iterPtr->startPtr = iterPtr->endPtr = NULL;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    framePtr = NULL;
    if (filmPtr->activePtr != NULL) {
        framePtr = filmPtr->activePtr->framePtr;
    } 
    iterPtr->startPtr = iterPtr->endPtr = framePtr;
    iterPtr->type = ITER_SINGLE;
    result = GetFrameByIndex(interp, filmPtr, string, length, &framePtr);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (result == TCL_OK) {
        iterPtr->startPtr = iterPtr->endPtr = framePtr;
        return TCL_OK;
    }
    if (c == '.') {
        Blt_HashEntry *hPtr;
        
        hPtr = Blt_FindHashEntry(&filmPtr->gripTable, string);
        if (hPtr != NULL) {
	    Grip *gripPtr;

            gripPtr = Blt_GetHashValue(hPtr);
            iterPtr->startPtr = iterPtr->endPtr = gripPtr->framePtr;
            iterPtr->type = ITER_SINGLE;
            return TCL_OK;
        }
        return TCL_ERROR;
    } else if ((c == 'a') && (strcmp(iterPtr->tagName, "all") == 0)) {
        iterPtr->type  = ITER_ALL;
        iterPtr->link = Blt_Chain_FirstLink(filmPtr->frames);
    } else if ((c == 'i') && (length > 6) && 
               (strncmp(string, "index:", 6) == 0)) {
        if (GetFrameByIndex(interp, filmPtr, string + 6, length - 6, &framePtr) 
            != TCL_OK) {
            return TCL_ERROR;
        }
        iterPtr->startPtr = iterPtr->endPtr = framePtr;
    } else if ((c == 'n') && (length > 5) && 
               (strncmp(string, "name:", 5) == 0)) {
        framePtr = GetFrameByName(filmPtr, string + 5);
        if (framePtr == NULL) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "can't find a frame named \"", 
                        string + 5, "\" in \"", Tk_PathName(filmPtr->tkwin), 
                        "\"", (char *)NULL);
            }
            return TCL_ERROR;
        }
        iterPtr->startPtr = iterPtr->endPtr = framePtr;
    } else if ((c == 't') && (length > 4) && 
               (strncmp(string, "tag:", 4) == 0)) {
        Blt_Chain chain;

        chain = Blt_Tags_GetItemList(&filmPtr->tags, string + 4);
        if (chain == NULL) {
            return TCL_OK;
        }
        iterPtr->tagName = string + 4;
        iterPtr->link = Blt_Chain_FirstLink(chain);
        iterPtr->type = ITER_TAG;
    } else if ((c == 'l') && (length > 6) && 
               (strncmp(string, "label:", 6) == 0)) {
        iterPtr->link = Blt_Chain_FirstLink(filmPtr->frames);
        iterPtr->tagName = string + 6;
        iterPtr->type = ITER_PATTERN;
    } else if ((framePtr = GetFrameByName(filmPtr, string)) != NULL) {
        iterPtr->startPtr = iterPtr->endPtr = framePtr;
    } else if ((chain = Blt_Tags_GetItemList(&filmPtr->tags, string)) != NULL) {
        iterPtr->tagName = string;
        iterPtr->link = Blt_Chain_FirstLink(chain);
        iterPtr->type = ITER_TAG;
    } else {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find frame index, name, or tag \"", 
                string, "\" in \"", Tk_PathName(filmPtr->tkwin), "\"", 
                             (char *)NULL);
        }
        return TCL_ERROR;
    }   
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewFrame --
 *
 *      This procedure creates and initializes a new Frame structure to
 *      hold a widget.  A valid widget has a parent widget that is either
 *      a) the container widget itself or b) a mutual ancestor of the
 *      container widget.
 *
 * Results:
 *      Returns a pointer to the new structure describing the new widget
 *      frame.  If an error occurred, then the return value is NULL and an
 *      error message is left in interp->result.
 *
 * Side effects:
 *      Memory is allocated and initialized for the Frame structure.
 *
 * ---------------------------------------------------------------------------- 
 */
static Frame *
NewFrame(Tcl_Interp *interp, Filmstrip *filmPtr, const char *name)
{
    Blt_HashEntry *hPtr;
    Frame *framePtr;
    int isNew;
    Grip *gripPtr;
    
    if (name == NULL) {
        char string[200];

        /* Generate an unique frame name. */
        do {
            sprintf(string, "frame%lu", (unsigned long)filmPtr->nextId++);
            hPtr = Blt_CreateHashEntry(&filmPtr->frameTable, string, &isNew);
        } while (!isNew);
    } else {
        hPtr = Blt_CreateHashEntry(&filmPtr->frameTable, name, &isNew);
        if (!isNew) {
            Tcl_AppendResult(interp, "frame \"", name, "\" already exists.",
                             (char *)NULL);
            return NULL;
        }
    }
    framePtr = Blt_AssertCalloc(1, sizeof(Frame));
    gripPtr = &framePtr->grip;
    Blt_ResetLimits(&framePtr->reqWidth);
    Blt_ResetLimits(&framePtr->reqHeight);
    Blt_ResetLimits(&framePtr->reqSize);
    framePtr->anchor = TK_ANCHOR_CENTER;
    framePtr->fill = FILL_BOTH;
    framePtr->filmPtr = filmPtr;
    framePtr->relief = TK_RELIEF_FLAT;
    gripPtr->framePtr = framePtr;
    framePtr->hashPtr = hPtr;
    framePtr->index = Blt_Chain_GetLength(filmPtr->frames);
    framePtr->link = Blt_Chain_Append(filmPtr->frames, framePtr);
    framePtr->name = Blt_GetHashKey(&filmPtr->frameTable, hPtr);
    framePtr->nom  = LIMITS_NOM;
    framePtr->resize = RESIZE_BOTH;
    Blt_SetHashValue(hPtr, framePtr);

    /* Generate an unique subwindow name. It will be in the form "grip0",
     * "grip1", etc. which hopefully will not collide with any existing
     * child window names for this widget. */
    {
        char string[200];
        char *path;

        path = Blt_AssertMalloc(strlen(Tk_PathName(filmPtr->tkwin)) + 200);
        do {
            sprintf(string, "grip%lu", (unsigned long)filmPtr->nextGripId++);
            sprintf(path, "%s.%s", Tk_PathName(filmPtr->tkwin), string);
        } while (Tk_NameToWindow(NULL, path, filmPtr->tkwin) != NULL);
        Blt_Free(path);
        gripPtr->tkwin = Tk_CreateWindow(interp, filmPtr->tkwin, string,
                (char *)NULL);
        if (gripPtr->tkwin == NULL) {
            DestroyFrame(framePtr);
            return NULL;
        }
    } 
    Tk_SetClass(gripPtr->tkwin, "BltFilmstripGrip");
    Tk_CreateEventHandler(gripPtr->tkwin,
                ExposureMask|FocusChangeMask|StructureNotifyMask, 
                GripEventProc, gripPtr);

    /* Also add frame to grips table */
    hPtr = Blt_CreateHashEntry(&filmPtr->gripTable, Tk_PathName(gripPtr->tkwin),
                &isNew);
    assert(isNew);
    gripPtr->hashPtr = hPtr;
    Blt_SetHashValue(hPtr, gripPtr);
    return framePtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * FrameFreeProc --
 *
 *      Removes the Frame structure from the hash table and frees the
 *      memory allocated by it.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the frame is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
FrameFreeProc(DestroyData dataPtr)
{
    Frame *framePtr = (Frame *)dataPtr;
    Filmstrip *filmPtr = framePtr->filmPtr;

    DestroyFrame(framePtr);
    filmPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(filmPtr);
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
RenumberFrames(Filmstrip *filmPtr)
{
    int count;
    Blt_ChainLink link;

    count = 0;
    for (link = Blt_Chain_FirstLink(filmPtr->frames); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Frame *framePtr;
        
        framePtr = Blt_Chain_GetValue(link);
        framePtr->index = count;
        count++;
    }
}

static void
MoveFrame(Filmstrip *filmPtr, Frame *framePtr, InsertOrder order, Frame *relPtr)
{
    if (Blt_Chain_GetLength(filmPtr->frames) == 1) {
        return;                         /* Can't rearrange one item. */
    }
    Blt_Chain_UnlinkLink(filmPtr->frames, framePtr->link);
    switch (order) {
    case INSERT_AFTER:                  /* After */
        Blt_Chain_LinkAfter(filmPtr->frames, framePtr->link, relPtr->link);
        break;
    case INSERT_BEFORE:                 /* Before */
        Blt_Chain_LinkBefore(filmPtr->frames, framePtr->link, relPtr->link);
        break;
    }
    RenumberFrames(filmPtr);
    filmPtr->flags |= LAYOUT_PENDING;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewFilmstrip --
 *
 *      This procedure creates and initializes a new Filmstrip structure
 *      with tkwin as its container widget. The internal structures
 *      associated with the filmstrip are initialized.
 *
 * Results:
 *      Returns the pointer to the new Filmstrip structure describing the
 *      new filmstrip geometry manager.  If an error occurred, the return
 *      value will be NULL and an error message is left in interp->result.
 *
 * Side effects:
 *      Memory is allocated and initialized, an event handler is set up to
 *      watch tkwin, etc.
 *
 *---------------------------------------------------------------------------
 */
static Filmstrip *
NewFilmstrip(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Filmstrip *filmPtr;
    Tk_Window tkwin;

    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), 
        Tcl_GetString(objPtr), (char *)NULL);
    if (tkwin == NULL) {
        return NULL;
    }
    filmPtr = Blt_AssertCalloc(1, sizeof(Filmstrip));
    Tk_SetClass(tkwin, "BltFilmstrip");
    filmPtr->gripActiveRelief = TK_RELIEF_RAISED;
    filmPtr->anchor = TK_ANCHOR_CENTER;
    filmPtr->display = Tk_Display(tkwin);
    filmPtr->flags = LAYOUT_PENDING;
    filmPtr->gripBorderWidth = 1;
    filmPtr->gripPad.side1 = filmPtr->gripPad.side2 = 2;
    filmPtr->gripThickness = 3;
    filmPtr->interp = interp;
    filmPtr->delay = 30;
    filmPtr->gripRelief = TK_RELIEF_FLAT;
    filmPtr->scrollUnits = 10;
    filmPtr->tkwin = tkwin;
    filmPtr->side = GRIP_FARSIDE;
    Blt_SetWindowInstanceData(tkwin, filmPtr);
    Blt_InitHashTable(&filmPtr->frameTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&filmPtr->gripTable, BLT_STRING_KEYS);
    Blt_Tags_Init(&filmPtr->tags);
    Tk_CreateEventHandler(tkwin, ExposureMask|StructureNotifyMask, 
                          FilmstripEventProc, filmPtr);
    filmPtr->frames = Blt_Chain_Create();
    filmPtr->cmdToken = Tcl_CreateObjCommand(interp, Tk_PathName(tkwin), 
        FilmstripInstCmdProc, filmPtr, FilmstripInstCmdDeleteProc);
    filmPtr->defVertCursor = Tk_GetCursor(interp, tkwin, DEF_VCURSOR);
    filmPtr->defHorzCursor = Tk_GetCursor(interp, tkwin, DEF_HCURSOR);
    return filmPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * DestroyFilmstrip --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the Filmstrip structure at a safe time (when no-one is
 *      using it anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the filmstrip geometry manager is freed
 *      up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyFilmstrip(Filmstrip *filmPtr)         /* Filmstrip structure */
{
    Blt_ChainLink link;

    Blt_FreeOptions(filmStripSpecs, (char *)filmPtr, filmPtr->display, 0);
    /* Release the chain of entries. */
    for (link = Blt_Chain_FirstLink(filmPtr->frames); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Frame *framePtr;

        framePtr = Blt_Chain_GetValue(link);
        framePtr->link = NULL;           /* Don't disrupt this chain of
                                          * entries. */
        framePtr->hashPtr = NULL;
        DestroyFrame(framePtr);
    }
    Tk_FreeCursor(filmPtr->display, filmPtr->defHorzCursor);
    Tk_FreeCursor(filmPtr->display, filmPtr->defVertCursor);
    Blt_Tags_Reset(&filmPtr->tags);
    Blt_Chain_Destroy(filmPtr->frames);
    Blt_DeleteHashTable(&filmPtr->frameTable);
    Blt_Free(filmPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FilmstripFreeProc --
 *
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the Filmstrip structure at a safe time (when no-one is
 *      using it anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the filmstrip geometry manager is freed
 *      up.
 *
 *---------------------------------------------------------------------------
 */
static void
FilmstripFreeProc(DestroyData dataPtr)    /* Filmstrip structure */
{
    Filmstrip *filmPtr = (Filmstrip *)dataPtr;

    DestroyFilmstrip(filmPtr);
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

static void
UnmapFrameAndGrip(Frame *framePtr)
{
    Grip *gripPtr;
    Filmstrip *filmPtr;
    
    gripPtr = &framePtr->grip;
    filmPtr = framePtr->filmPtr;
    if (Tk_IsMapped(framePtr->tkwin)) {
        Tk_UnmapWindow(framePtr->tkwin);
    }
    if (Tk_IsMapped(gripPtr->tkwin)) {
        if (filmPtr->anchorPtr == gripPtr) {
            /* Can't unmap a grip window that's being used. Just move it
             * off-screen. */
            Tk_MoveWindow(gripPtr->tkwin, -1000, -1000);
        } else {
            Tk_UnmapWindow(gripPtr->tkwin);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetFrames --
 *
 *      Sets/resets the size of each frame to the minimum limit of the
 *      frame (this is usually zero). This routine gets called when new
 *      widgets are added, deleted, or resized.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The size of each frame is re-initialized to its minimum size.
 *
 *---------------------------------------------------------------------------
 */
static void
ResetFrames(Filmstrip *filmPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(filmPtr->frames); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Frame *framePtr;
        int extra, size;

        framePtr = Blt_Chain_GetValue(link);
        /*
         * The constraint procedure below also has the desired side-effect
         * of setting the minimum, maximum, and nominal values to the
         * requested size of its associated widget (if one exists).
         */
        size = BoundSize(0, &framePtr->reqSize);
        if (filmPtr->flags & VERTICAL) {
            extra = PADDING(framePtr->padY);
        } else {
            extra = PADDING(framePtr->padX);
        }
        if (framePtr->flags & GRIP) {
            extra += filmPtr->gripSize;
        }
        if (framePtr->reqSize.flags & LIMITS_NOM_SET) {
            /*
             * This could be done more cleanly.  We want to ensure that the
             * requested nominal size is not overridden when determining
             * the normal sizes.  So temporarily fix min and max to the
             * nominal size and reset them back later.
             */
            framePtr->min = framePtr->max = framePtr->size = framePtr->nom = 
                size + extra;
        } else {
            /* The range defaults to 0..MAXINT */
            framePtr->min = framePtr->reqSize.min + extra;
            framePtr->max = framePtr->reqSize.max + extra;
            framePtr->nom = LIMITS_NOM;
            framePtr->size = size + extra;
        }
    }
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * SetNominalSizes
 *
 *      Sets the normal sizes for each frame.  The frame size is the
 *      requested widget size plus an amount of padding.  In addition,
 *      adjust the min/max bounds of the frame depending upon the resize
 *      flags (whether the frame can be expanded or shrunk from its normal
 *      size).
 *
 * Results:
 *      Returns the total space needed for the all the frames.
 *
 * Side Effects:
 *      The nominal size of each frame is set.  This is later used to
 *      determine how to shrink or grow the table if the container can't be
 *      resized to accommodate the exact size requirements of all the
 *      frames.
 *
 *---------------------------------------------------------------------------
 */
static int
SetNominalSizes(Filmstrip *filmPtr)
{
    Blt_ChainLink link;
    int total;

    total = 0;
    for (link = Blt_Chain_FirstLink(filmPtr->frames); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Frame *framePtr;
        int extra;

        framePtr = Blt_Chain_GetValue(link);
        if (filmPtr->flags & VERTICAL) {
            extra = PADDING(framePtr->padY);
        } else {
            extra = PADDING(framePtr->padX);
        }
        if (framePtr->flags & GRIP) {
            extra += filmPtr->gripSize;
        }
        /*
         * Restore the real bounds after temporarily setting nominal size.
         * These values may have been set in ResetFrames to restrict the
         * size of the frame to the requested range.
         */
        framePtr->min = framePtr->reqSize.min + extra;
        framePtr->max = framePtr->reqSize.max + extra;
        if (framePtr->size > framePtr->max) {
            framePtr->size = framePtr->max;
        } 
        if (framePtr->size < framePtr->min) {
            framePtr->size = framePtr->min;
        }
        framePtr->nom = framePtr->size;
        /*
         * If a frame can't be resized (to either expand or shrink), hold
         * its respective limit at its normal size.
         */
        if ((framePtr->resize & RESIZE_EXPAND) == 0) {
            framePtr->max = framePtr->nom;
        }
        if ((framePtr->resize & RESIZE_SHRINK) == 0) {
            framePtr->min = framePtr->nom;
        }
        total += framePtr->nom;
    }
    return total;
}
#endif

static void
GetHorizontalFrameGeometry(Filmstrip *filmPtr, Frame *framePtr)
{
    int w, h;

#if TRACE
    fprintf(stderr, "GetHorizontalFrameGeometry(%s)\n", framePtr->name);
#endif
    if (filmPtr->relHeight > 0.0) {
        h = (int)(Tk_Height(filmPtr->tkwin) * filmPtr->relHeight);
    } else {
        h = GetReqHeight(framePtr) + PADDING(framePtr->padY) +
            2 * framePtr->borderWidth;
    }
    if (filmPtr->relWidth > 0.0) {
        w = (int)(Tk_Width(filmPtr->tkwin) * filmPtr->relWidth);
    } else {
        w = GetReqWidth(framePtr) + PADDING(framePtr->padX) +
            2 * framePtr->borderWidth;
        if (framePtr->flags & SHOW_GRIP) {
            w += filmPtr->gripSize;     /* Add the size of the grip from
                                         * the maximum window size. */
        }
    }
    framePtr->height = h;
    framePtr->width  = w;
#if TRACE
    fprintf(stderr, "GetHorizontalFrameGeometry: w=%d h=%d\n",
            framePtr->width, framePtr->height);
#endif
}

static void
GetVerticalFrameGeometry(Filmstrip *filmPtr, Frame *framePtr)
{
    int w, h;

#if TRACE
    fprintf(stderr, "GetVerticalFrameGeometry(%s)\n", framePtr->name);
#endif
    if (filmPtr->relWidth > 0.0) {
        w = (int)(Tk_Width(filmPtr->tkwin) * filmPtr->relWidth);
    } else {
        w = GetReqWidth(framePtr) + PADDING(framePtr->padX) +
            2 * framePtr->borderWidth;
    }
    if (filmPtr->relHeight > 0.0) {
        h = (int)(Tk_Height(filmPtr->tkwin) * filmPtr->relHeight);
    } else {
        h = GetReqHeight(framePtr)  + PADDING(framePtr->padY) +
            2 * framePtr->borderWidth;
        if (framePtr->flags & SHOW_GRIP) {
            h += filmPtr->gripSize;     /* Add the size of the grip from
                                         * the maximum window size. */
        }
    }
    framePtr->width  = w;
    framePtr->height = h;
#if TRACE
    fprintf(stderr, "GetVerticalFrameGeometry: w=%d h=%d\n",
            framePtr->width, framePtr->height);
#endif
}

static void
ComputeVerticalScroll(Filmstrip *filmPtr, Frame *framePtr)
{
    int y1, y2, h, margin;

    h = VPORTWIDTH(filmPtr);
    y1 = filmPtr->scrollOffset;
    y2 = filmPtr->scrollOffset + h;
    margin = 20;
    
    /* If the frame is partially obscured, scroll so that it's entirely
     * in view. */
    if (framePtr->worldY < y1) {
        filmPtr->scrollTarget = framePtr->worldY - (h - framePtr->height)/2;
        if ((framePtr->height + margin) < h) {
            filmPtr->scrollTarget -= margin;
        }
    } else if ((framePtr->worldY + framePtr->height) >= y2) {
        filmPtr->scrollTarget = framePtr->worldY - (h - framePtr->height)/2;
        if ((framePtr->height + margin) < h) {
            filmPtr->scrollTarget += margin;
        }
    }
}
static void
ComputeHorizontalScroll(Filmstrip *filmPtr, Frame *framePtr)
{
    int x1, x2, w, margin;
    
    w = VPORTWIDTH(filmPtr);
    x1 = filmPtr->scrollOffset;
    x2 = filmPtr->scrollOffset + w;
    margin = 20;
    
    /* If the frame is partially obscured, scroll so that it's entirely in
     * view. */
    if (framePtr->worldX < x1) {
        filmPtr->scrollTarget = framePtr->worldX - (w - framePtr->width)/2;
        if ((framePtr->width + margin) < w) {
            filmPtr->scrollTarget -= margin;
        }
    } else if ((framePtr->worldX + framePtr->width) >= x2) {
        filmPtr->scrollTarget = framePtr->worldX - (w - framePtr->width)/2;
        if ((framePtr->width + margin) < w) {
            filmPtr->scrollTarget += margin;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * LayoutHorizontalFrames --
 *
 *      Calculates the normal space requirements for frames.  
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
LayoutHorizontalFrames(Filmstrip *filmPtr)
{
    int maxWidth, maxHeight, worldWidth;
    Frame *framePtr;
    int x;

    maxWidth = maxHeight = 0;
    worldWidth = 0;
    ResetFrames(filmPtr);
    for (framePtr = FirstFrame(filmPtr, 0); framePtr != NULL;
         framePtr = NextFrame(framePtr, 0)) {
        GetHorizontalFrameGeometry(filmPtr, framePtr);
        framePtr->flags &= ~GRIP;
        if (framePtr->flags & HIDDEN) {
            UnmapFrameAndGrip(framePtr);
            continue;
        }
        if ((NextFrame(framePtr, 0) != NULL) && (framePtr->flags & SHOW_GRIP)) {
            worldWidth += filmPtr->gripSize;
        }
        if (maxHeight < framePtr->height) {
            maxHeight = framePtr->height;
        }
        if (maxWidth < framePtr->width) {
            maxWidth = framePtr->width;
        }
        worldWidth += framePtr->width;
    }
    x = 0;
    for (framePtr = FirstFrame(filmPtr, HIDDEN); framePtr != NULL;
         framePtr = NextFrame(framePtr, HIDDEN)) {
        framePtr->worldX = x;
        framePtr->worldY = 0;
        x += framePtr->width;
        if ((NextFrame(framePtr, 0) != NULL) && (framePtr->flags & SHOW_GRIP)) {
            x += filmPtr->gripSize;
        }
    }
    if ((filmPtr->relWidth > 0.0) && (filmPtr->currentPtr != NULL)) {
        ComputeHorizontalScroll(filmPtr, filmPtr->currentPtr);
        filmPtr->scrollOffset = filmPtr->scrollTarget;
    }
    filmPtr->worldHeight = maxHeight;
    filmPtr->worldWidth = worldWidth;
    filmPtr->normalWidth = worldWidth;
    filmPtr->normalHeight = maxHeight;
    filmPtr->flags &= ~LAYOUT_PENDING;
    filmPtr->flags |= SCROLL_PENDING;
}

/*
 *---------------------------------------------------------------------------
 *
 * LayoutVerticalFrames --
 *
 *      Calculates the normal space requirements for frames.  
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
LayoutVerticalFrames(Filmstrip *filmPtr)
{
    int maxWidth, maxHeight, worldHeight;
    Frame *framePtr;
    int y;

#if TRACE
    fprintf(stderr, "LayoutVerticalFrames\n");
#endif
    maxHeight = maxWidth = 0;
    worldHeight = 0;
    ResetFrames(filmPtr);
    for (framePtr = FirstFrame(filmPtr, 0); framePtr != NULL;
         framePtr = NextFrame(framePtr, 0)) {
        GetVerticalFrameGeometry(filmPtr, framePtr);
        framePtr->flags &= ~GRIP;
        if (framePtr->flags & HIDDEN) {
            UnmapFrameAndGrip(framePtr);
            continue;
        }
        if ((NextFrame(framePtr, 0) != NULL) && (framePtr->flags & SHOW_GRIP)) {
            framePtr->flags |= GRIP;
        }
        if (maxHeight < framePtr->height) {
            maxHeight = framePtr->height;
        }
        if (maxWidth < framePtr->width) {
            maxWidth = framePtr->width;
        }
        worldHeight += framePtr->height;
    }
    y = 0;
    for (framePtr = FirstFrame(filmPtr, HIDDEN); framePtr != NULL;
         framePtr = NextFrame(framePtr, HIDDEN)) {
        framePtr->worldX = 0;
        framePtr->worldY = y;
        y += framePtr->height;
        if ((NextFrame(framePtr, 0) != NULL) && (framePtr->flags & SHOW_GRIP)) {
            y += filmPtr->gripSize;
        }
    }
    if ((filmPtr->relHeight > 0.0) && (filmPtr->currentPtr != NULL)) {
        ComputeVerticalScroll(filmPtr, filmPtr->currentPtr);
        filmPtr->scrollOffset = filmPtr->scrollTarget;
    }
    filmPtr->normalHeight = filmPtr->worldHeight = worldHeight;
    filmPtr->normalWidth  = filmPtr->worldWidth = maxWidth;
    filmPtr->flags &= ~LAYOUT_PENDING;
    filmPtr->flags |= SCROLL_PENDING;
}

static void
DrawHorizontalFrame(Filmstrip *filmPtr, Frame *framePtr, Drawable drawable) 
{
    int reqWidth;
    int cavityHeight;
    int x, y, w, h;
    Grip *gripPtr;
    
    if (framePtr->tkwin == NULL) {
        return;
    }
    if ((framePtr->width == 0) || (framePtr->height == 0)) {
        return;
    }
    x = SCREEN(filmPtr, framePtr->worldX);
    y = 0;
    /* Unmap any widget that isn't viewable in the the widget. */
    if (((x > Tk_Width(filmPtr->tkwin)) || ((x + framePtr->width + filmPtr->gripSize) < 0))) {
        UnmapFrameAndGrip(framePtr);
        return;
    }
    if ((framePtr->relief != TK_RELIEF_FLAT) && (framePtr->borderWidth > 0)) {
        Blt_Bg_DrawRectangle(filmPtr->tkwin, drawable, filmPtr->bg, 
                             x, y, framePtr->width, framePtr->height,
                             framePtr->borderWidth, framePtr->relief);
    }
    x += framePtr->padX.side1 + framePtr->borderWidth;
    y = framePtr->padY.side1 + framePtr->borderWidth;

    /* If the frame height is taller that the filmstrip widget or if -fill
     * y is set, resize the height of the child to be the height of the
     * filmstrip. */
    cavityHeight = Tk_Height(filmPtr->tkwin);
    if ((framePtr->height > cavityHeight) || (framePtr->fill & FILL_Y)) {
        h = cavityHeight;
    } else {
        h = framePtr->height;
    }
    h -= PADDING(framePtr->padY) + 2 * framePtr->borderWidth;
    cavityHeight -= PADDING(framePtr->padY) + 2 * framePtr->borderWidth;
    
    reqWidth = Tk_ReqWidth(framePtr->tkwin);
    if ((reqWidth < framePtr->width) && ((framePtr->fill & FILL_X) == 0)) {
        w = reqWidth;
    } else {
        w = framePtr->width;
    }
    w -= PADDING(framePtr->padX) + 2 * framePtr->borderWidth;

    /* If the filmstrip area is bigger than the set of frames inside of it,
     * adjust the starting x position according to the global anchor for
     * the widget. */
    if (Tk_Width(filmPtr->tkwin) > filmPtr->worldWidth) {
        switch (filmPtr->anchor) {
        case TK_ANCHOR_NW:
        case TK_ANCHOR_SW:
        case TK_ANCHOR_W: 
            break;
        case TK_ANCHOR_N: 
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_S: 
            x += (Tk_Width(filmPtr->tkwin) - filmPtr->worldWidth) / 2;
            break;
        case TK_ANCHOR_E:
        case TK_ANCHOR_SE:
        case TK_ANCHOR_NE:
            x += Tk_Width(filmPtr->tkwin) - filmPtr->worldWidth;
            break;                      
        }
    }

    /* If the widget is bigger than the frame, adjust the position of the
     * child widget according to the anchor specified. The default is
     * center.  */
#ifdef notdef
    cavityHeight = Tk_Height(filmPtr->tkwin) - PADDING(framePtr->padY) -
        2 * framePtr->borderWidth;
#endif
    if (cavityHeight > h) {
        switch (framePtr->anchor) {
        case TK_ANCHOR_N:
        case TK_ANCHOR_NE:
        case TK_ANCHOR_NW:
            break;
        case TK_ANCHOR_W: 
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_E:     
            y += (cavityHeight - h) / 2;
            break;
        case TK_ANCHOR_S:     
        case TK_ANCHOR_SE:    
        case TK_ANCHOR_SW:    
            y += cavityHeight - h;
            break;                      
        }
    }
    /*
     * Resize and/or move the widget as necessary.
     */
    if ((x != Tk_X(framePtr->tkwin)) || 
        (y != Tk_Y(framePtr->tkwin)) ||
        (w != Tk_Width(framePtr->tkwin)) || 
        (h != Tk_Height(framePtr->tkwin))) {
        assert(w > 0 && h > 0);
        Tk_MoveResizeWindow(framePtr->tkwin, x, y, w, h);
    }
    if (!Tk_IsMapped(framePtr->tkwin)) {
        Tk_MapWindow(framePtr->tkwin);
    }
    gripPtr = &framePtr->grip;
    if (framePtr->flags & SHOW_GRIP) {
        y = 0;
        x += w + framePtr->borderWidth + framePtr->padX.side1;
        h = Tk_Height(filmPtr->tkwin);
        w = filmPtr->gripSize; 
        if ((x != Tk_X(gripPtr->tkwin)) || (y != Tk_Y(gripPtr->tkwin)) ||
            (w != Tk_Width(gripPtr->tkwin)) ||
            (h != Tk_Height(gripPtr->tkwin))) {
            assert(w > 0 && h > 0);
            Tk_MoveResizeWindow(gripPtr->tkwin, x, y, w, h);
        }
        if (!Tk_IsMapped(gripPtr->tkwin)) {
            Tk_MapWindow(gripPtr->tkwin);
        }
        XRaiseWindow(filmPtr->display, Tk_WindowId(gripPtr->tkwin));
    } else if (Tk_IsMapped(gripPtr->tkwin)) {
        Tk_UnmapWindow(gripPtr->tkwin);
    }
}

static void
DrawVerticalFrame(Filmstrip *filmPtr, Frame *framePtr, Drawable drawable) 
{
    int reqHeight;
    int cavityWidth;
    int x, y, w, h;
    Grip *gripPtr;

    if (framePtr->tkwin == NULL) {
        return;
    }
    if ((framePtr->width == 0) || (framePtr->height == 0)) {
        return;
    }
    x = 0;
    y = SCREEN(filmPtr, framePtr->worldY);

    /* Unmap any widget that isn't viewable in the the widget. */
    if (((y > Tk_Height(filmPtr->tkwin)) || ((y + framePtr->height + filmPtr->gripSize) < 0))) {
        UnmapFrameAndGrip(framePtr);
        return;
    }
    if ((framePtr->relief != TK_RELIEF_FLAT) && (framePtr->borderWidth > 0)) {
        Blt_Bg_DrawRectangle(filmPtr->tkwin, drawable, filmPtr->bg, 
                x, y, framePtr->width, framePtr->height,
                framePtr->borderWidth, framePtr->relief);
    }
    x = framePtr->padX.side1 + framePtr->borderWidth;
    y += framePtr->padY.side1 + framePtr->borderWidth;

    /* If the frame width is wider that the filmstrip widget or if -fill
     * x is set, resize the width of the child to be the width of the
     * filmstrip. */
    cavityWidth = Tk_Width(filmPtr->tkwin);
    if ((framePtr->width > cavityWidth) || (framePtr->fill & FILL_X)) {
        w = cavityWidth;
    } else {
        w = framePtr->width;
    }
    w -= PADDING(framePtr->padX) + 2 * framePtr->borderWidth;
    cavityWidth -= PADDING(framePtr->padX) + 2 * framePtr->borderWidth;

    reqHeight = Tk_ReqHeight(framePtr->tkwin);
    if ((reqHeight < framePtr->height) && ((framePtr->fill & FILL_Y) == 0)) {
        h = reqHeight;
    } else {
        h = framePtr->height;
    }
    h -= PADDING(framePtr->padY) + 2 * framePtr->borderWidth;

    /* If the filmstrip area is taller than the set of frames inside of it,
     * adjust the starting y position according to the global anchor for
     * the widget. */
    fprintf(stderr, "h=%d wh=%d fh=%d\n",
            Tk_Height(filmPtr->tkwin), filmPtr->worldHeight, framePtr->height);
    if (Tk_Height(filmPtr->tkwin) > filmPtr->worldHeight) {
        switch (filmPtr->anchor) {
        case TK_ANCHOR_NW:              /* Upper left corner */
        case TK_ANCHOR_SW:              /* Lower left corner */
        case TK_ANCHOR_W:               /* Left center */
            break;
        case TK_ANCHOR_N:               /* Top center */
        case TK_ANCHOR_CENTER:          /* Centered */
        case TK_ANCHOR_S:               /* Bottom center */
            y += (Tk_Height(filmPtr->tkwin) - filmPtr->worldHeight) / 2;
            break;
        case TK_ANCHOR_E:               /* Right center */
        case TK_ANCHOR_SE:              /* Lower right corner */
        case TK_ANCHOR_NE:              /* Upper right corner */
            y += Tk_Height(filmPtr->tkwin) - filmPtr->worldHeight;
            break;                      
        }
    }

    /* If the widget is bigger than the frame, adjust the position of the
     * child widget according to the anchor specified. The default is
     * center.  */
    cavityWidth = Tk_Width(filmPtr->tkwin) - PADDING(framePtr->padX) -
        2 * framePtr->borderWidth;
    if (cavityWidth > w) {
        switch (framePtr->anchor) {
        case TK_ANCHOR_N:
        case TK_ANCHOR_NE:
        case TK_ANCHOR_NW:
            break;
        case TK_ANCHOR_W: 
        case TK_ANCHOR_CENTER:
        case TK_ANCHOR_E:     
            x += (cavityWidth - w) / 2;
            break;
        case TK_ANCHOR_S:     
        case TK_ANCHOR_SE:    
        case TK_ANCHOR_SW:    
            x += cavityWidth - w;
            break;                      
        }
    }
    /*
     * Resize and/or move the widget as necessary.
     */
    if ((x != Tk_X(framePtr->tkwin)) || 
        (y != Tk_Y(framePtr->tkwin)) ||
        (w != Tk_Width(framePtr->tkwin)) || 
        (h != Tk_Height(framePtr->tkwin))) {
        assert(w > 0 && h > 0);
        Tk_MoveResizeWindow(framePtr->tkwin, x, y, w, h);
    }
    if (!Tk_IsMapped(framePtr->tkwin)) {
        Tk_MapWindow(framePtr->tkwin);
    }
    gripPtr = &framePtr->grip;
    if (framePtr->flags & SHOW_GRIP) {
        x = 0;
        y += h + framePtr->borderWidth + framePtr->padY.side1;
        w = Tk_Width(filmPtr->tkwin);
        h = filmPtr->gripSize; 
        if ((x != Tk_X(gripPtr->tkwin)) || (y != Tk_Y(gripPtr->tkwin)) ||
            (w != Tk_Width(gripPtr->tkwin)) ||
            (h != Tk_Height(gripPtr->tkwin))) {
            assert(w > 0 && h > 0);
            Tk_MoveResizeWindow(gripPtr->tkwin, x, y, w, h);
        }
        if (!Tk_IsMapped(gripPtr->tkwin)) {
            Tk_MapWindow(gripPtr->tkwin);
        }
        XRaiseWindow(filmPtr->display, Tk_WindowId(gripPtr->tkwin));
    } else if (Tk_IsMapped(gripPtr->tkwin)) {
        Tk_UnmapWindow(gripPtr->tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayVerticalFrames --
 *
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The widgets in the filmstrip are possibly resized and redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayVerticalFrames(Filmstrip *filmPtr, Drawable drawable)
{
    Frame *framePtr;

    /*
     * If the filmstrip has no children anymore, then don't do anything at
     * all: just leave the container widget's size as-is.
     */
    if (filmPtr->anchorPtr == NULL) {
        framePtr = FirstFrame(filmPtr, HIDDEN);
        filmPtr->anchorPtr = &framePtr->grip;
    }
    if (filmPtr->flags & LAYOUT_PENDING) {
        LayoutVerticalFrames(filmPtr);
    }
    for (framePtr = FirstFrame(filmPtr, HIDDEN); framePtr != NULL; 
         framePtr = NextFrame(framePtr, HIDDEN)) {
        DrawVerticalFrame(filmPtr, framePtr, drawable);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayHorizontalFrames --
 *
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The widgets in the filmstrip are possibly resized and redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayHorizontalFrames(Filmstrip *filmPtr, Drawable drawable)
{
    Frame *framePtr;

    /*
     * If the filmstrip has no children anymore, then don't do anything at
     * all: just leave the container widget's size as-is.
     */
    if (filmPtr->anchorPtr == NULL) {
        framePtr = FirstFrame(filmPtr, HIDDEN);
        filmPtr->anchorPtr = &framePtr->grip;
    }
    if (filmPtr->flags & LAYOUT_PENDING) {
        LayoutHorizontalFrames(filmPtr);
    }
    for (framePtr = FirstFrame(filmPtr, HIDDEN); framePtr != NULL; 
         framePtr = NextFrame(framePtr, HIDDEN)) {
        DrawHorizontalFrame(filmPtr, framePtr, drawable);
    }
}

static void
ComputeGeometry(Filmstrip *filmPtr) 
{
#if TRACE
    fprintf(stderr, "ComputeGeometry\n");
#endif
    if (filmPtr->flags & VERTICAL) {
        LayoutVerticalFrames(filmPtr);
    } else {
        LayoutHorizontalFrames(filmPtr);
    }
    filmPtr->flags &= ~LAYOUT_PENDING;
    /* Override computed layout with requested width/height */
    if (filmPtr->reqWidth > 0) {
        filmPtr->normalWidth = filmPtr->reqWidth;
    }
    if (filmPtr->reqHeight > 0) {
        filmPtr->normalHeight = filmPtr->reqHeight;
    }
#if TRACE
    fprintf(stderr, "ComputeGeometry: rw=%d rh=%d urw=%d urh=%d nw=%d nh=%d\n",
            Tk_ReqWidth(filmPtr->tkwin), Tk_ReqHeight(filmPtr->tkwin),
            filmPtr->reqWidth, filmPtr->reqHeight,
            filmPtr->normalWidth, filmPtr->normalHeight);
#endif
    if ((filmPtr->normalWidth != Tk_ReqWidth(filmPtr->tkwin)) ||
        (filmPtr->normalHeight != Tk_ReqHeight(filmPtr->tkwin))) {
        Tk_GeometryRequest(filmPtr->tkwin, filmPtr->normalWidth,
                           filmPtr->normalHeight);
    }
}

static void
ConfigureFilmstrip(Filmstrip *filmPtr) 
{
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    filmPtr->gripSize = filmPtr->gripThickness + PADDING(filmPtr->gripPad);
    gcMask = 0;
    newGC = Tk_GetGC(filmPtr->tkwin, gcMask, &gcValues);
    if (filmPtr->gc != NULL) {
        Tk_FreeGC(filmPtr->display, filmPtr->gc);
    }
    filmPtr->gc = newGC;
}

static void
AdjustGrip(Filmstrip *filmPtr, int delta)
{
    filmPtr->scrollOffset -= delta;
    filmPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(filmPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * AddOp --
 *
 *      Appends a frame into the widget.
 *
 * Results:
 *      Returns a standard TCL result.  The index of the frame is left in
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
    Filmstrip *filmPtr = clientData;
    Frame *framePtr;
    const char *name;
    Grip *gripPtr;

    name = NULL;
    if (objc > 2) {
        const char *string;

        string = Tcl_GetString(objv[2]);
        if (string[0] != '-') {
            if (GetFrameFromObj(NULL, filmPtr, objv[2], &framePtr) == TCL_OK) {
                Tcl_AppendResult(interp, "frame \"", string, 
                        "\" already exists", (char *)NULL);
                return TCL_ERROR;
            }
            name = string;
            objc--, objv++;
        }
    }
    framePtr = NewFrame(interp, filmPtr, name);
    if (framePtr == NULL) {
        return TCL_ERROR;
    }
    gripPtr = &framePtr->grip;
    if (Blt_ConfigureWidgetFromObj(interp, gripPtr->tkwin, frameSpecs,
                objc - 2, objv + 2, (char *)framePtr, 0) != TCL_OK) {
        DestroyFrame(framePtr);
        return TCL_ERROR;
    }
    filmPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(filmPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), framePtr->name, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BoxOp --
 *
 *      pathName bbox frameName ?switches?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BboxOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    BBoxSwitches switches;
    Frame *framePtr;
    Tcl_Obj *listObjPtr;
    int x1, y1, x2, y2;

    if (GetFrameFromObj(interp, filmPtr, objv[2], &framePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Process switches  */
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, bboxSwitches, objc - 3, objv + 3, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (framePtr->flags & HIDDEN) {
        return TCL_OK;
    }
    if (ISHORIZ(filmPtr)) {
        x1 = SCREEN(filmPtr, framePtr->worldX);
        x2 = x1 + framePtr->width;
        y1 = 0;
        y2 = framePtr->height;
    } else {
        x1 = 0;
        x2 = framePtr->width;
        y1 = SCREEN(filmPtr, framePtr->worldY);
        y2 = y1 + framePtr->height;
    }
    if (switches.flags & BBOX_ROOT) {
        int rootX, rootY;

        Tk_GetRootCoords(filmPtr->tkwin, &rootX, &rootY);
        if (rootX < 0) {
            rootX = 0;
        }
        if (rootY < 0) {
            rootY = 0;
        }
        x1 += rootX;
        x2 += rootX;
        y1 += rootY;
        y2 += rootY;
    }

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y1));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x2));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y2));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *      Returns the name, position and options of a widget in the filmstrip.
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
    Filmstrip *filmPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, filmPtr->tkwin, filmStripSpecs,
                (char *)filmPtr, objv[2], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      Returns the name, position and options of a widget in the filmstrip.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the filmstrip configuration
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
    Filmstrip *filmPtr = clientData;

    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, filmPtr->tkwin, filmStripSpecs,
                (char *)filmPtr, (Tcl_Obj *)NULL, BLT_CONFIG_OBJV_ONLY);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, filmPtr->tkwin, filmStripSpecs,
                (char *)filmPtr, objv[2], BLT_CONFIG_OBJV_ONLY);
    }
    if (Blt_ConfigureWidgetFromObj(interp, filmPtr->tkwin, filmStripSpecs,
        objc - 2, objv + 2, (char *)filmPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    ConfigureFilmstrip(filmPtr);
    /* Arrange for the layout to be computed at the next idle point. */
    filmPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(filmPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes the specified frames from the widget.  Note that the frame
 *      indices can be fixed only after all the deletions have occurred.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      pathName delete widget
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    FrameIterator iter;
    Frame *framePtr;

    if (GetFrameIterator(interp, filmPtr, objv[2], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (framePtr = FirstTaggedFrame(&iter); framePtr != NULL; 
         framePtr = NextTaggedFrame(&iter)) {
        Tcl_EventuallyFree(framePtr, FrameFreeProc);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *      Indicates if given frame exists.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      widget index frame
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Frame *framePtr;
    int state;

    state = FALSE;
    if (GetFrameFromObj(NULL, filmPtr, objv[2], &framePtr) == TCL_OK) {
        if (framePtr != NULL) {
            state = TRUE;
        }
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * GripActivateOp --
 *
 *      Changes the cursor and schedules to redraw the grip in its
 *      activate state (different relief, colors, etc).
 *
 *      pathName grip activate frameName 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GripActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    Frame *framePtr;
    Filmstrip *filmPtr = clientData;
    Grip *gripPtr;
    
    if (GetFrameFromObj(interp, filmPtr, objv[3], &framePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (framePtr->flags & (DISABLED|HIDDEN)) {
        return TCL_OK;
    }
    gripPtr = &framePtr->grip;
    if (gripPtr != filmPtr->activePtr) {
        Tk_Cursor cursor;

        if (filmPtr->activePtr != NULL) {
            EventuallyRedrawGrip(filmPtr->activePtr);
        }
        if (framePtr != NULL) {
            EventuallyRedrawGrip(gripPtr);
        }
        filmPtr->activePtr = gripPtr;
        if (framePtr->cursor != None) {
            cursor = framePtr->cursor;
        } else if (filmPtr->flags & VERTICAL) {
            cursor = filmPtr->defVertCursor;
        } else {
            cursor = filmPtr->defHorzCursor;
        }
        Tk_DefineCursor(gripPtr->tkwin, cursor);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GripAnchorOp --
 *
 *      Set the anchor for the resize/moving the frame.  Only one of the x
 *      and y coordinates are used depending upon the orientation of the
 *      frame.
 *
 *      pathName grip anchor frameName x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GripAnchorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Frame *framePtr;
    Filmstrip *filmPtr = clientData;
    int x, y;

    if (GetFrameFromObj(interp, filmPtr, objv[3], &framePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (framePtr->flags & (DISABLED|HIDDEN)) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    } 
    filmPtr = framePtr->filmPtr;
    filmPtr->anchorPtr = filmPtr->activePtr = &framePtr->grip;
    filmPtr->flags |= GRIP_ACTIVE;
    if (filmPtr->flags & VERTICAL) {
        filmPtr->gripAnchor = y;
    } else {
        filmPtr->gripAnchor = x;
    } 
    AdjustGrip(filmPtr, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GripDeactivateOp --
 *
 *      Changes the cursor and schedules to redraw the grip in its
 *      inactivate state (different relief, colors, etc).
 *
 *      pathName grip deactivate 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GripDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;

    if (filmPtr->activePtr != NULL) {
        EventuallyRedrawGrip(filmPtr->activePtr);
        filmPtr->activePtr = NULL;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GripMarkOp --
 *
 *      Sets the current mark for moving the grip.  The change between
 *      the mark and the anchor is the amount to move the grip from its
 *      previous location.
 *
 *      pathName grip mark frameName x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GripMarkOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Frame *framePtr;
    Filmstrip *filmPtr = clientData;
    int x, y;                           /* Root coordinates of the pointer
                                         * over the grip. */
    int delta, mark;

    if (GetFrameFromObj(interp, filmPtr, objv[3], &framePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (framePtr->flags & (DISABLED|HIDDEN)) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    } 
    filmPtr = framePtr->filmPtr;
    filmPtr->anchorPtr = &framePtr->grip;
    mark = (filmPtr->flags & VERTICAL) ? y : x;
    delta = mark - filmPtr->gripAnchor;
    AdjustGrip(filmPtr, delta);
    filmPtr->gripAnchor = mark;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GripMoveOp --
 *
 *      Moves the grip.  The grip is moved the given distance from its
 *      previous location.
 *
 *      pathName grip move frameName x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GripMoveOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Frame *framePtr;
    Filmstrip *filmPtr = clientData;
    int delta, x, y;

    if (GetFrameFromObj(interp, filmPtr, objv[3], &framePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (framePtr->flags & (DISABLED|HIDDEN)) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    } 
    filmPtr = framePtr->filmPtr;
    filmPtr->anchorPtr = &framePtr->grip;
    delta = (filmPtr->flags & VERTICAL) ? y : x;
    AdjustGrip(filmPtr, delta);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GripSetOp --
 *
 *      Sets the location of the grip to coordinate (x or y) specified.
 *      The windows are move and/or arranged according to the mode.
 *
 *      pathName grip set frameName x y
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GripSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Frame *framePtr;
    Filmstrip *filmPtr = clientData;
    int x, y;
    int delta, mark;

    if (GetFrameFromObj(interp, filmPtr, objv[3], &framePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (framePtr->flags & (DISABLED|HIDDEN)) {
        return TCL_OK;
    }
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
        return TCL_ERROR;
    } 
    filmPtr = framePtr->filmPtr;
    filmPtr->flags &= ~GRIP_ACTIVE;
    mark = (filmPtr->flags & VERTICAL) ? y : x;
    delta = mark - filmPtr->gripAnchor;
    AdjustGrip(filmPtr, delta);
    filmPtr->gripAnchor = mark;
    return TCL_OK;
}

static Blt_OpSpec gripOps[] =
{
    {"activate",   2, GripActivateOp,   4, 4, "frameName"},
    {"anchor",     2, GripAnchorOp,     6, 6, "frameName x y"},
    {"deactivate", 1, GripDeactivateOp, 3, 3, ""},
    {"mark",       2, GripMarkOp,       6, 6, "frameName x y"},
    {"move",       2, GripMoveOp,       6, 6, "frameName x y"},
    {"set",        1, GripSetOp,        6, 6, "frameName x y"},
};

static int numGripOps = sizeof(gripOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * GripOp --
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
GripOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numGripOps, gripOps, BLT_OP_ARG2, 
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
 *      Returns the index of the given frame.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      pathName index frameName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Frame *framePtr;
    int index;

    index = -1;
    if (GetFrameFromObj(NULL, filmPtr, objv[2], &framePtr) == TCL_OK) {
        if (framePtr != NULL) {
            index = framePtr->index;
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
 *      Inserts a new frame into the filmstrip.
 *
 *      pathName insert after|before frameName ?label? ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InsertOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Frame *framePtr, *relPtr;
    InsertOrder order;
    const char *name;
    Grip *gripPtr;
        
    if (GetInsertOrder(interp, objv[2], &order) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetFrameFromObj(interp, filmPtr, objv[3], &relPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    name = NULL;
    if (objc > 3) {
        const char *string;

        string = Tcl_GetString(objv[2]);
        if (string[0] != '-') {
            if (GetFrameFromObj(NULL, filmPtr, objv[4], &framePtr) == TCL_OK) {
                Tcl_AppendResult(interp, "frame \"", string, 
                        "\" already exists", (char *)NULL);
                return TCL_ERROR;
            }
            name = string;
            objc--, objv++;
        }
    }
    framePtr = NewFrame(interp, filmPtr, name);
    if (framePtr == NULL) {
        return TCL_ERROR;
    }
    MoveFrame(filmPtr, framePtr, order, relPtr);
    EventuallyRedraw(filmPtr);
    gripPtr = &framePtr->grip;
    if (Blt_ConfigureWidgetFromObj(interp, gripPtr->tkwin, frameSpecs,
                objc - 4, objv + 4, (char *)framePtr, 0) != TCL_OK) {
        DestroyFrame(framePtr);
        return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), framePtr->name, -1);
    return TCL_OK;

}

/*
 *---------------------------------------------------------------------------
 *
 * MoveOp --
 *
 *      Moves a frame to a new location.
 *
 *      pathName move before whereName frameName 
 *      pathName move after  whereName frameName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MoveOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Frame *framePtr, *relPtr;
    InsertOrder order;

    if (GetInsertOrder(interp, objv[2], &order) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetFrameFromObj(interp, filmPtr, objv[3], &relPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetFrameFromObj(interp, filmPtr, objv[4], &framePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((framePtr == NULL) || (framePtr->flags & DISABLED) ||
        (framePtr == relPtr)) {
        return TCL_OK;
    }
    MoveFrame(filmPtr, framePtr, order, relPtr);
    EventuallyRedraw(filmPtr);
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
    Filmstrip *filmPtr = clientData;
    Tcl_Obj *listObjPtr;
    Blt_ChainLink link;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    for (link = Blt_Chain_FirstLink(filmPtr->frames); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Frame *framePtr;
        int i;
        int match;
        
        framePtr = Blt_Chain_GetValue(link);
        match = (objc == 2);
        for (i = 2; i < objc; i++) {
            if (Tcl_StringMatch(framePtr->name, Tcl_GetString(objv[i]))) {
                match = TRUE;
                break;
            }
        }
        if (match) {
            Tcl_Obj *objPtr;
            
            objPtr = Tcl_NewStringObj(framePtr->name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FrameCgetOp --
 *
  * Results:
 *      Returns a standard TCL result.  A list of the widget attributes is
 *      left in interp->result.
 *
 *      pathName frame cget frameName option
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FrameCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Frame *framePtr;

    if (GetFrameFromObj(interp, filmPtr, objv[3], &framePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, filmPtr->tkwin, frameSpecs,
        (char *)framePtr, objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * FrameConfigureOp --
 *
 * Results:
 *      Returns a standard TCL result.  A list of the filmstrip configuration
 *      option information is left in interp->result.
 *
 *      pathName frame configure frameName ?option value ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FrameConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                 Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Frame *framePtr;
    FrameIterator iter;
    Tk_Window tkwin;

    if (objc == 4) {
        if (GetFrameFromObj(interp, filmPtr, objv[3], &framePtr) != TCL_OK) {
            return TCL_ERROR;
        }
        tkwin = framePtr->grip.tkwin;
        return Blt_ConfigureInfoFromObj(interp, tkwin, frameSpecs,
                (char *)framePtr, (Tcl_Obj *)NULL,0);
    } else if (objc == 5) {
        if (GetFrameFromObj(interp, filmPtr, objv[3], &framePtr) != TCL_OK) {
            return TCL_ERROR;
        }
        tkwin = framePtr->grip.tkwin;
        return Blt_ConfigureInfoFromObj(interp, tkwin, frameSpecs,
                (char *)framePtr, objv[4], 0);
    }
    if (GetFrameIterator(interp, filmPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (framePtr = FirstTaggedFrame(&iter); framePtr != NULL; 
         framePtr = NextTaggedFrame(&iter)) {
        tkwin = framePtr->grip.tkwin;
        if (Blt_ConfigureWidgetFromObj(interp, tkwin, frameSpecs,
                objc - 4, objv + 4, (char *)framePtr, BLT_CONFIG_OBJV_ONLY)
            != TCL_OK) {
            return TCL_ERROR;
        }
    }
    filmPtr->anchorPtr = NULL;
    filmPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(filmPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FrameLabelOp --
 *
 *      Returns the label of the given frame.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      pathName frame label frameName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FrameLabelOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Frame *framePtr;

    if (GetFrameFromObj(NULL, filmPtr, objv[3], &framePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), framePtr->name, -1);
    return TCL_OK;
}


static Blt_OpSpec frameOps[] =
{
    {"cget",       2, FrameCgetOp,      5, 5, "frameName option",},
    {"configure",  2, FrameConfigureOp, 4, 0, "frameName ?option value?",},
    {"label",      1, FrameLabelOp,     4, 4, "frameName",},
};

static int numFrameOps = sizeof(frameOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * FrameOp --
 *
 *      This procedure is invoked to process the TCL command that
 *      corresponds to the filmstrip geometry manager.  See the user
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
FrameOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numFrameOps, frameOps, BLT_OP_ARG2, 
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
    Filmstrip *filmPtr = clientData;

    if (filmPtr->scrollTarget == filmPtr->scrollOffset) {
        return;
    }
#ifdef notdef
    filmPtr->step++;
    frac = (double)filmPtr->step / (double)filmPtr->numSteps;
    range = filmPtr->scrollTarget - filmPtr->scrollOffset;
    frac = log10(9.0 * frac + 1.0);
    filmPtr->scrollOffset += (int)(range * frac);
#endif
    
    if (filmPtr->scrollTarget > filmPtr->scrollOffset) {
        filmPtr->scrollOffset += filmPtr->scrollIncr;
        if (filmPtr->scrollOffset > filmPtr->scrollTarget) {
            filmPtr->scrollOffset = filmPtr->scrollTarget;
        } 
    } else if (filmPtr->scrollTarget < filmPtr->scrollOffset) {
        filmPtr->scrollOffset -= filmPtr->scrollIncr;
        if (filmPtr->scrollOffset < filmPtr->scrollTarget) {
            filmPtr->scrollOffset = filmPtr->scrollTarget;
        } 
    }
    filmPtr->scrollIncr += filmPtr->scrollIncr;
    if (filmPtr->scrollTarget == filmPtr->scrollOffset) {
        if (filmPtr->timerToken != (Tcl_TimerToken)0) {
            Tcl_DeleteTimerHandler(filmPtr->timerToken);
            filmPtr->timerToken = 0;
            filmPtr->scrollIncr = filmPtr->scrollUnits;
        }
        if (filmPtr->postScrollCmdObjPtr != NULL) {
            if (Tcl_EvalObjEx(filmPtr->interp, filmPtr->postScrollCmdObjPtr,
                TCL_EVAL_GLOBAL) != TCL_OK) {
                Tcl_BackgroundError(filmPtr->interp);
            }
        }
    } else {
        filmPtr->timerToken = Tcl_CreateTimerHandler(filmPtr->delay, 
                MotionTimerProc, filmPtr);
    }
    filmPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(filmPtr);
}


/*ARGSUSED*/
static int
SeeOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Frame *framePtr;

    if (GetFrameFromObj(interp, filmPtr, objv[2], &framePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((framePtr == NULL) || (framePtr->flags & HIDDEN)) {
        return TCL_OK;
    }
    if (filmPtr->flags & VERTICAL) {
        ComputeVerticalScroll(filmPtr, framePtr);
    } else {
        ComputeHorizontalScroll(filmPtr, framePtr);
    }
    if (filmPtr->flags & ANIMATE) {
        filmPtr->scrollIncr = filmPtr->scrollUnits;
        filmPtr->timerToken = Tcl_CreateTimerHandler(filmPtr->delay, 
                MotionTimerProc, filmPtr);
    } else {
        filmPtr->scrollOffset = filmPtr->scrollTarget;
        filmPtr->flags |= SCROLL_PENDING;
        if (filmPtr->postScrollCmdObjPtr != NULL) {
            if (Tcl_EvalObjEx(filmPtr->interp, filmPtr->postScrollCmdObjPtr,
                TCL_EVAL_GLOBAL) != TCL_OK) {
                Tcl_BackgroundError(filmPtr->interp);
            }
        }
        EventuallyRedraw(filmPtr);
    }
    filmPtr->currentPtr = framePtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SizeOp --
 *
 *      Returns the number of frames in the film strip.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      pathName size
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;

    Tcl_SetIntObj(Tcl_GetObjResult(interp),
                  Blt_Chain_GetLength(filmPtr->frames));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagAddOp --
 *
 *      pathName tag add tagName ?frameName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    const char *string;
    char c;
    
    string = Tcl_GetString(objv[3]);
    c = string[0];
    if ((isdigit(c)) && (Blt_ObjIsInteger(objv[3]))) {
        Tcl_AppendResult(interp, "bad tag \"", string, 
                 "\": can't be a number.", (char *)NULL);
        return TCL_ERROR;
    }
    if ((c == 'a') && (strcmp(string, "all") == 0)) {
        Tcl_AppendResult(interp, "can't add reserved tag \"", string, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        /* No nodes specified.  Just add the tag. */
        Blt_Tags_AddTag(&filmPtr->tags, string);
    } else {
        int i;

        for (i = 4; i < objc; i++) {
            Frame *framePtr;
            FrameIterator iter;
            
            if (GetFrameIterator(interp, filmPtr, objv[i], &iter) != TCL_OK) {
                return TCL_ERROR;
            }
            for (framePtr = FirstTaggedFrame(&iter); framePtr != NULL; 
                 framePtr = NextTaggedFrame(&iter)) {
                Blt_Tags_AddItemToTag(&filmPtr->tags, string, framePtr);
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
 *      pathName delete tagName ?frameName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    const char *string;
    int i;
    char c;

    string = Tcl_GetString(objv[3]);
    c = string[0];
    if ((isdigit(c)) && (Blt_ObjIsInteger(objv[3]))) {
        Tcl_AppendResult(interp, "bad tag \"", string, 
                 "\": can't be a number.", (char *)NULL);
        return TCL_ERROR;
    }
    if ((c == 'a') && (strcmp(string, "all") == 0)) {
        Tcl_AppendResult(interp, "can't delete reserved tag \"", string, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        Frame *framePtr;
        FrameIterator iter;
        
        if (GetFrameIterator(interp, filmPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (framePtr = FirstTaggedFrame(&iter); framePtr != NULL; 
             framePtr = NextTaggedFrame(&iter)) {
            Blt_Tags_RemoveItemFromTag(&filmPtr->tags, string, framePtr);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagExistsOp --
 *
 *      Returns the existence of the one or more tags in the given frame.
 *      If the frame has any the tags, 1 is returned in the interpreter.
 *
 *      pathName tag exists frameName ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    int i;
    FrameIterator iter;

    if (GetFrameIterator(interp, filmPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        const char *tag;
        Frame *framePtr;

        tag = Tcl_GetString(objv[i]);
        for (framePtr = FirstTaggedFrame(&iter); framePtr != NULL; 
             framePtr = NextTaggedFrame(&iter)) {
            if (Blt_Tags_ItemHasTag(&filmPtr->tags, framePtr, tag)) {
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
 *      Removes the given tags from all frames.
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
    Filmstrip *filmPtr = clientData;
    int i;

    for (i = 3; i < objc; i++) {
        const char *string;

        string = Tcl_GetString(objv[i]);
        if ((isdigit(string[0])) && (Blt_ObjIsInteger(objv[i]))) {
            Tcl_AppendResult(interp, "bad tag \"", string, 
                             "\": can't be a number.", (char *)NULL);
            return TCL_ERROR;
        }
        Blt_Tags_ForgetTag(&filmPtr->tags, string);
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
 *      pathName tag get frameName ?pat1 pat2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagGetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Frame *framePtr; 
    FrameIterator iter;
    Tcl_Obj *listObjPtr;

    if (GetFrameIterator(interp, filmPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (framePtr = FirstTaggedFrame(&iter); framePtr != NULL; 
         framePtr = NextTaggedFrame(&iter)) {
        if (objc == 4) {
            Blt_Tags_AppendTagsToObj(&filmPtr->tags, framePtr, listObjPtr);
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
                Blt_Tags_AppendTagsToChain(&filmPtr->tags, framePtr, chain);
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
 *      Returns the names of all the tags in the filmstrip.  If one of more
 *      node arguments are provided, then only the tags found in those
 *      nodes are returned.
 *
 *      pathName tag names ?frameName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewStringObj("all", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    if (objc == 3) {
        Blt_Tags_AppendAllTagsToObj(&filmPtr->tags, listObjPtr);
    } else {
        Blt_HashTable uniqTable;
        int i;

        Blt_InitHashTable(&uniqTable, BLT_STRING_KEYS);
        for (i = 3; i < objc; i++) {
            FrameIterator iter;
            Frame *framePtr;

            if (GetFrameIterator(interp, filmPtr, objPtr, &iter) != TCL_OK) {
                goto error;
            }
            for (framePtr = FirstTaggedFrame(&iter); framePtr != NULL; 
                 framePtr = NextTaggedFrame(&iter)) {
                Blt_ChainLink link;
                Blt_Chain chain;

                chain = Blt_Chain_Create();
                Blt_Tags_AppendTagsToChain(&filmPtr->tags, framePtr, chain);
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
 *      returned will represent the union of frames for all the given tags.
 *
 *      pathName tag indices ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagIndicesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Blt_HashTable frameTable;
    int i;
        
    Blt_InitHashTable(&frameTable, BLT_ONE_WORD_KEYS);
    for (i = 3; i < objc; i++) {
        const char *string;
        char c;
        
        string = Tcl_GetString(objv[i]);
        c = string[0];
        if ((isdigit(c)) && (Blt_ObjIsInteger(objv[i]))) {
            Tcl_AppendResult(interp, "bad tag \"", string, 
                             "\": can't be a number.", (char *)NULL);
            goto error;
        }
        if ((c == 'a') && (strcmp(string, "all") == 0)) {
            break;
        } else {
            Blt_Chain chain;

            chain = Blt_Tags_GetItemList(&filmPtr->tags, string);
            if (chain != NULL) {
                Blt_ChainLink link;

                for (link = Blt_Chain_FirstLink(chain); link != NULL; 
                     link = Blt_Chain_NextLink(link)) {
                    Frame *framePtr;
                    int isNew;
                    
                    framePtr = Blt_Chain_GetValue(link);
                    Blt_CreateHashEntry(&frameTable, (char *)framePtr, &isNew);
                }
            }
            continue;
        }
        Tcl_AppendResult(interp, "can't find a tag \"", string, "\"",
                         (char *)NULL);
        goto error;
    }
    {
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        for (hPtr = Blt_FirstHashEntry(&frameTable, &iter); hPtr != NULL; 
             hPtr = Blt_NextHashEntry(&iter)) {
            Frame *framePtr;
            Tcl_Obj *objPtr;

            framePtr = (Frame *)Blt_GetHashKey(&frameTable, hPtr);
            objPtr = Tcl_NewLongObj(framePtr->index);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
    }
    Blt_DeleteHashTable(&frameTable);
    return TCL_OK;

 error:
    Blt_DeleteHashTable(&frameTable);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagSetOp --
 *
 *      Sets one or more tags for a given frame.  Tag names can't start
 *      with a digit (to distinquish them from node ids) and can't be a
 *      reserved tag ("all").
 *
 *      pathName tag set frameName ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    int i;
    FrameIterator iter;

    if (GetFrameIterator(interp, filmPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        const char *string;
        Frame *framePtr;
        char c;

        string = Tcl_GetString(objv[i]);
        c = string[0];
        if ((isdigit(c)) && (Blt_ObjIsInteger(objv[i]))) {
            Tcl_AppendResult(interp, "bad tag \"", string, 
                             "\": can't be a number.", (char *)NULL);
            return TCL_ERROR;
        }
        if ((c == 'a') && (strcmp(string, "all") == 0)) {
            Tcl_AppendResult(interp, "can't add reserved tag \"", string, "\"",
                             (char *)NULL);     
            return TCL_ERROR;
        }
        for (framePtr = FirstTaggedFrame(&iter); framePtr != NULL; 
             framePtr = NextTaggedFrame(&iter)) {
            Blt_Tags_AddItemToTag(&filmPtr->tags, string, framePtr);
        }    
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagUnsetOp --
 *
 *      Removes one or more tags from a given frame. If a tag doesn't exist
 *      or is a reserved tag ("all"), nothing will be done and no error
 *      message will be returned.
 *
 *      pathName tag unset frameName ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagUnsetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Filmstrip *filmPtr = clientData;
    Frame *framePtr;
    FrameIterator iter;

    if (GetFrameIterator(interp, filmPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (framePtr = FirstTaggedFrame(&iter); framePtr != NULL; 
         framePtr = NextTaggedFrame(&iter)) {
        int i;
        for (i = 4; i < objc; i++) {
            const char *tag;

            tag = Tcl_GetString(objv[i]);
            Blt_Tags_RemoveItemFromTag(&filmPtr->tags, tag, framePtr);
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
    {"add",     1, TagAddOp,      2, 0, "?label? ?option value...?",},
    {"delete",  1, TagDeleteOp,   2, 0, "?frameName ...?",},
    {"exists",  1, TagExistsOp,   4, 0, "frameName  ?tag ...?",},
    {"forget",  1, TagForgetOp,   3, 0, "?tag ...?",},
    {"get",     1, TagGetOp,      4, 0, "frameName ?pattern ...?",},
    {"indices", 1, TagIndicesOp,  3, 0, "?tag ...?",},
    {"names",   1, TagNamesOp,    3, 0, "?frameName ...?",},
    {"set",     1, TagSetOp,      4, 0, "frameName ?tag...",},
    {"unset",   1, TagUnsetOp,    4, 0, "frameName ?tag...",},
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
    Filmstrip *filmPtr = clientData;
    int width;

    width = VPORTWIDTH(filmPtr);
    if (objc == 2) {
        double fract;
        Tcl_Obj *listObjPtr, *objPtr;

        /* Report first and last fractions */
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        /*
         * Note: we are bounding the fractions between 0.0 and 1.0 to support
         * the "canvas"-style of scrolling.
         */
        fract = (double)filmPtr->scrollOffset / WORLDSIZE(filmPtr);
        objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        fract = (double)(filmPtr->scrollOffset + width) / WORLDSIZE(filmPtr);
        objPtr = Tcl_NewDoubleObj(FCLAMP(fract));
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, 
                &filmPtr->scrollOffset, WORLDSIZE(filmPtr), width, 
                filmPtr->scrollUnits, BLT_SCROLL_MODE_HIERBOX) != TCL_OK) {
        return TCL_ERROR;
    }
    filmPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(filmPtr);
    return TCL_OK;
}


static Blt_OpSpec filmstripOps[] =
{
    {"add",        1, AddOp,       2, 0, "?label? ?option value ...?",},
    {"bbox",       1, BboxOp,      3, 0, "frameName ?switches?",},
    {"cget",       2, CgetOp,      3, 3, "option",},
    {"configure",  2, ConfigureOp, 2, 0, "?option value ...?",},
    {"delete",     1, DeleteOp,    3, 3, "?frameName ...?",},
    {"exists",     1, ExistsOp,    3, 3, "frameName",},
    {"frame",      1, FrameOp,     2, 0, "oper ?args?",},
    {"grip",       1, GripOp,      2, 0, "oper ?args?",},
    {"index",      3, IndexOp,     3, 3, "frameName",},
    {"insert",     3, InsertOp,    4, 0, "after|before whereName ?label? ?option value ...?",},
    {"move",       1, MoveOp,      4, 0, "after|before whereName frameName",},
    {"names",      1, NamesOp,     2, 0, "?pattern ...?",},
    {"see",        1, SeeOp,       3, 3, "frameName",},
    {"size",       1, SizeOp,      2, 2, "",},
    {"tag",        1, TagOp,       2, 0, "oper args",},
    {"view",       1, ViewOp,      2, 5, 
        "?moveto fract? ?scroll number what?",},
};

static int numFilmstripOps = sizeof(filmstripOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * FilmstripInstCmdDeleteProc --
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
FilmstripInstCmdDeleteProc(ClientData clientData)
{
    Filmstrip *filmPtr = clientData;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin is
     * NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */
    if (filmPtr->tkwin != NULL) {
        Tk_Window tkwin;

        tkwin = filmPtr->tkwin;
        filmPtr->tkwin = NULL;
        Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FilmstripInstCmdProc --
 *
 *      This procedure is invoked to process the TCL command that
 *      corresponds to the filmstrip geometry manager.  See the user
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
FilmstripInstCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numFilmstripOps, filmstripOps, 
                BLT_OP_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * FilmstripCmd --
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
FilmstripCmd(
    ClientData clientData,              /* Main window associated with
                                         * interpreter. */
    Tcl_Interp *interp,                 /* Current interpreter. */
    int objc,                           /* # of arguments. */
    Tcl_Obj *const *objv)               /* Argument strings. */
{
    Filmstrip *filmPtr;

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

            Blt_FmtString(info, 200,
                             "\n\t(while loading bindings for %.50s)", 
                    Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            return TCL_ERROR;;
        }
    }

    filmPtr = NewFilmstrip(interp, objv[1]);
    if (filmPtr == NULL) {
        goto error;
    }

    if (Blt_ConfigureWidgetFromObj(interp, filmPtr->tkwin, filmStripSpecs,
                objc - 2, objv + 2, (char *)filmPtr, 0) != TCL_OK) {
        goto error;
    }
    ConfigureFilmstrip(filmPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(filmPtr->tkwin),-1);
    return TCL_OK;
  error:
    if (filmPtr != NULL) {
        Tk_DestroyWindow(filmPtr->tkwin);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FilmstripCmdInitProc --
 *
 *      This procedure is invoked to initialize the TCL command that
 *      corresponds to the filmstrip geometry manager.
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
Blt_FilmstripCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpecs[] = {
        { "filmstrip", FilmstripCmd },
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
 *      The widgets in the filmstrip are possibly resized and redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayProc(ClientData clientData)
{
    Filmstrip *filmPtr = clientData;
    Pixmap drawable;
    unsigned int w, h;

    filmPtr->flags &= ~REDRAW_PENDING;
#if TRACE 
    fprintf(stderr, "DisplayProc(%s) w=%d h=%d numFrames=%d\n",
            Tk_PathName(filmPtr->tkwin), Tk_Width(filmPtr->tkwin),
            Tk_Height(filmPtr->tkwin), Blt_Chain_GetLength(filmPtr->frames));
#endif
    if (filmPtr->flags & LAYOUT_PENDING) {
        ComputeGeometry(filmPtr);
    }
    if ((Tk_Width(filmPtr->tkwin) <= 1) || (Tk_Height(filmPtr->tkwin) <=1)) {
        /* Don't bother computing the layout until the size of the window
         * is something reasonable. */
        return;
    }
    if (!Tk_IsMapped(filmPtr->tkwin)) {
        /* The filmstrip's window isn't mapped, so don't bother drawing
         * anything.  By getting this far, we've at least computed the
         * coordinates of the new layout.  */
        return;
    }
    if (filmPtr->flags & SCROLL_PENDING) {
        int width;

        width = VPORTWIDTH(filmPtr);
        filmPtr->scrollOffset = Blt_AdjustViewport(filmPtr->scrollOffset,
                WORLDSIZE(filmPtr), width, filmPtr->scrollUnits, 
                BLT_SCROLL_MODE_HIERBOX);
        if (filmPtr->scrollCmdObjPtr != NULL) {
            Blt_UpdateScrollbar(filmPtr->interp, filmPtr->scrollCmdObjPtr,
                filmPtr->scrollOffset, filmPtr->scrollOffset + width,
                WORLDSIZE(filmPtr));
        }
        filmPtr->flags &= ~SCROLL_PENDING;
    }
    filmPtr->numVisible = Blt_Chain_GetLength(filmPtr->frames);
    if (filmPtr->currentPtr == NULL) {
        filmPtr->currentPtr = FirstFrame(filmPtr, HIDDEN | DISABLED);
    }
    w = Tk_Width(filmPtr->tkwin);
    h = Tk_Height(filmPtr->tkwin);
    drawable = Blt_GetPixmap(filmPtr->display, Tk_WindowId(filmPtr->tkwin),
        w, h, Tk_Depth(filmPtr->tkwin));
    Blt_Bg_FillRectangle(filmPtr->tkwin, drawable, filmPtr->bg, 0, 0, w, h, 
        0, TK_RELIEF_FLAT);
    if (filmPtr->numVisible > 0) {
        if (filmPtr->flags & VERTICAL) {
            DisplayVerticalFrames(filmPtr, drawable);
        } else {
            DisplayHorizontalFrames(filmPtr, drawable);
        }
    }
    XCopyArea(filmPtr->display, drawable, Tk_WindowId(filmPtr->tkwin),
        filmPtr->gc, 0, 0, w, h, 0, 0);
    Tk_FreePixmap(filmPtr->display, drawable);
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayGrip
 *
 *      Draws the frame's grip at its proper location.  First determines the
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
 *      The size of each frame is re-initialized its minimum size.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayGrip(ClientData clientData)
{
    Blt_Bg bg;
    Drawable drawable;
    Filmstrip *filmPtr;
    Grip *gripPtr = clientData;
    Frame *framePtr;
    int relief;
    int w, h;
    
    framePtr = gripPtr->framePtr;
    framePtr->flags &= ~REDRAW_PENDING;
    if (gripPtr->tkwin == NULL) {
        return;
    }
    filmPtr = framePtr->filmPtr;

    if (filmPtr->activePtr == gripPtr) {
        bg = filmPtr->activeGripBg;
        relief = filmPtr->gripActiveRelief;
    } else {
        bg =  filmPtr->gripBg;
        relief = filmPtr->gripRelief;
    }
    w = Tk_Width(gripPtr->tkwin);
    h = Tk_Height(gripPtr->tkwin);
    drawable = Tk_WindowId(gripPtr->tkwin);
    if ((w > 0) && (h > 0)) {
        Blt_Bg_FillRectangle(gripPtr->tkwin, drawable, bg, 0, 0, w, h, 0,
                TK_RELIEF_FLAT);
    }
    w -= PADDING(filmPtr->gripPad);
    h -= PADDING(filmPtr->gripPad);
    if ((relief != TK_RELIEF_FLAT) && (w > 0) && (h > 0)) {
        Blt_Bg_DrawRectangle(gripPtr->tkwin, drawable, bg, 
                filmPtr->gripPad.side1, filmPtr->gripPad.side1, w, h,
                filmPtr->gripBorderWidth, relief);
    }
}

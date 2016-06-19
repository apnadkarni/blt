/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltScrollset.c --
 *
 * This module implements a scrollset widget for the BLT toolkit.
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

#ifdef HAVE_LIMITS_H
  #include <limits.h>
#endif  /* HAVE_LIMITS_H */

#include "bltAlloc.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltBind.h"
#include "bltImage.h"
#include "bltPicture.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltBg.h"
#include "bltPainter.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define VPORTWIDTH(s)   (Tk_Width((s)->tkwin) - (s)->yScrollbarWidth)
#define VPORTHEIGHT(s)  (Tk_Height((s)->tkwin) - (s)->xScrollbarHeight)
#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))

#define REDRAW_PENDING          (1<<0)  /* A redraw request is pending.  */
#define LAYOUT_PENDING          (1<<1)  /* A request to recompute the layout of
                                         * the scrollbars and slave widget is
                                         * pending.  */
#define UPDATE_PENDING          (1<<2)  /* A request to call the widget update
                                         * procedure because scrollbars or 
                                         * slave widget has been configured. */
#define SCROLLX                 (1<<3)  /* A request to set the X scrollbar is
                                         * pending. */
#define SCROLLY                 (1<<4)  /* A request to set the Y scrollbar is
                                         * pending. */
#define SCROLL_PENDING          (SCROLLX|SCROLLY)

#define XSCROLLBAR_PENDING      (1<<5)  /* A request to install a new
                                         * x-scrollbar widget is pending. */
#define YSCROLLBAR_PENDING      (1<<6)  /* A request to install a new
                                         * y-scrollbar widget is pending. */
#define SLAVE_PENDING           (1<<7)  /* A request to install a new slave
                                         * widget is pending.  */
#define DISPLAY_X               (1<<8)  /* Display the x-scrollbar. */
#define DISPLAY_Y               (1<<9)  /* Display the y-scrollbar. */
#define SLAVE_XVIEW             (1<<10) /* The slave widget has a "xview"
                                         * operation */
#define SLAVE_YVIEW             (1<<11) /* The slave widget has a "yview"
                                         * operation. */

#define DEF_ANCHOR              "center"
#define DEF_BACKGROUND          STD_NORMAL_BACKGROUND
#define DEF_CURSOR              ((char *)NULL)
#define DEF_FILL                "both"
#define DEF_HEIGHT              "0"
#define DEF_IPADX               "0"
#define DEF_IPADY               "0"
#define DEF_PADX                "0"
#define DEF_PADY                "0"
#define DEF_WIDTH               "0"
#define DEF_WINDOW              ((char *)NULL)
#define DEF_XSCROLLBAR          ((char *)NULL)
#define DEF_XSCROLLCOMMAND      ((char *)NULL)
#define DEF_XSCROLLINCREMENT    "2"
#define DEF_XVIEWCOMMAND        ((char *)NULL)
#define DEF_YSCROLLBAR          ((char *)NULL)
#define DEF_YSCROLLCOMMAND      ((char *)NULL)
#define DEF_YSCROLLINCREMENT    "2"
#define DEF_YVIEWCOMMAND        ((char *)NULL)

/*
 * Limits --
 *
 *      Defines the bounding of a size (width or height) in the table.  It
 *      may be related to the partition, entry, or table size.  The widget
 *      pointers are used to associate sizes with the requested size of
 *      other widgets.
 */

typedef struct {
    int flags;                          /* Flags indicate whether using
                                         * default values for limits or
                                         * not. See flags below. */
    int max, min;                       /* Values for respective limits. */
    int nom;                            /* Nominal starting value. */
} Limits;

typedef struct {
    /*
     * This works around a bug in the Tk API.  Under under Win32, Tk tries
     * to read the widget record of toplevel windows (TopLevel or Frame
     * widget), to get its menu name field.  What this means is that we
     * must carefully arrange the fields of this widget so that the
     * menuName field is at the same offset in the structure.
     */

    Tk_Window tkwin;                    /* Window that embodies the frame.
                                         * NULL means that the window has been
                                         * destroyed but the data structures
                                         * haven't yet been cleaned up. */

    Display *display;                   /* Display containing widget.  Used,
                                         * among other things, so that
                                         * resources can * be freed even after
                                         * tkwin has gone away. */

    Tcl_Interp *interp;                 /* Interpreter associated with widget.
                                         * Used to delete widget command. */

    Tcl_Command cmdToken;               /* Token for widget's command. */

    Tcl_Obj *slaveObjPtr;               /* Name of the slave widget to be
                                         * embed into the widget window. */
    Tk_Window slave;                    /* Slave window to be managed by this
                                         * widget. */
    Tk_Window shangle;

    Limits reqSlaveWidth, reqSlaveHeight; /* Requested sizes for slave. */
    int reqWidth, reqHeight;
    Tk_Anchor anchor;                   /* Anchor type: indicates how the
                                         * slave is positioned if extra space
                                         * is available in the widget */
    Blt_Pad padX;                       /* Extra padding placed left and right
                                         * of the slave. */
    Blt_Pad padY;                       /* Extra padding placed above and
                                         * below the slave */
    Blt_Bg bg;
    int iPadX, iPadY;                   /* Extra padding added to the interior
                                         * of the widget (i.e. adds to the
                                         * requested size of the widget) */
    int fill;                           /* Indicates how the widget should
                                         * fill the span of cells it
                                         * occupies. */
    int slaveX, slaveY;                 /* Origin of widget wrt container. */
    int slaveWidth, slaveHeight;        /* Dimension of slave widget. */
    unsigned int flags;
    Tk_Cursor cursor;                   /* Current cursor for window or
                                         * None. */

    int xScrollUnits, yScrollUnits;     /* Unit of distance to move when
                                         * clicking on the scrollbar
                                         * button. */
    /* Names of scrollbars to embed into the widget window. */
    Tcl_Obj *xScrollbarObjPtr, *yScrollbarObjPtr;

    /* Commands to control horizontal and vertical scrollbars. */
    Tcl_Obj *xReqScrollCmdObjPtr, *yReqScrollCmdObjPtr;

    Tcl_Obj *xScrollCmdObjPtr, *yScrollCmdObjPtr;

    /* Commands to control slave's horizontal and vertical views. */
    Tcl_Obj *xViewCmdObjPtr, *yViewCmdObjPtr;

    int xOffset, yOffset;               /* Scroll offsets of viewport in
                                         * world. */ 
    int worldWidth, worldHeight;        /* Dimension of entire menu. */
    int cavityWidth, cavityHeight;      /* Dimension of entire menu. */

    Tk_Window xScrollbar;               /* Horizontal scrollbar to be used if
                                         * necessary. If NULL, no x-scrollbar
                                         * is used. */
    Tk_Window yScrollbar;               /* Vertical scrollbar to be used if
                                         * necessary. If NULL, no y-scrollbar
                                         * is used. */

    short int xScrollbarWidth, xScrollbarHeight;
    short int yScrollbarWidth, yScrollbarHeight;
    short int shangleWidth, shangleHeight;
    /*
     * Scanning Information:
     */
    int scanAnchorX;                    /* Horizontal scan anchor in screen
                                         * x-coordinates. */
    int scanX;                          /* x-offset of the start of the
                                         * horizontal scan in world
                                         * coordinates.*/
    int scanAnchorY;                    /* Vertical scan anchor in screen
                                         * y-coordinates. */
    int scanY;                          /* y-offset of the start of the
                                         * vertical scan in world
                                         * coordinates.*/
    short int width, height;
} Scrollset;

static Blt_OptionParseProc ObjToLimits;
static Blt_OptionPrintProc LimitsToObj;
static Blt_CustomOption limitsOption =
{
    ObjToLimits, LimitsToObj, NULL, (ClientData)0
};

static Blt_ConfigSpec scrollsetSpecs[] =
{
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",  DEF_ANCHOR, 
        Blt_Offset(Scrollset, anchor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BACKGROUND, Blt_Offset(Scrollset, bg), 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor", DEF_CURSOR, 
        Blt_Offset(Scrollset, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FILL, "-fill", "fill", "Fill", DEF_FILL, 
        Blt_Offset(Scrollset, fill), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-height", "height", "Height", DEF_HEIGHT, 
        Blt_Offset(Scrollset, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-ipadx", "iPadX", "IPadX", DEF_IPADX,
        Blt_Offset(Scrollset, iPadX), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-ipady", "iPadY", "IPadY", DEF_IPADY,
        Blt_Offset(Scrollset, iPadY), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-padx", "padX", "PadX", DEF_PADX, 
        Blt_Offset(Scrollset, padX), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-pady", "padY", "PadY", DEF_PADY, 
        Blt_Offset(Scrollset, padY), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-reqheight", "reqHeight", "ReqHeight", (char *)NULL, 
        Blt_Offset(Scrollset, reqSlaveHeight), 0, &limitsOption},
    {BLT_CONFIG_CUSTOM, "-reqwidth", "reqSlaveWidth", "ReqWidth", (char *)NULL, 
        Blt_Offset(Scrollset, reqSlaveWidth), 0, &limitsOption},
    {BLT_CONFIG_OBJ, "-xscrollbar", "xScrollbar", "Scrollbar", DEF_XSCROLLBAR, 
        Blt_Offset(Scrollset, xScrollbarObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
        DEF_XSCROLLCOMMAND, Blt_Offset(Scrollset, xReqScrollCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-xscrollincrement", "xScrollIncrement",
        "ScrollIncrement", DEF_XSCROLLINCREMENT, 
        Blt_Offset(Scrollset, xScrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-xviewcommand", "xViewCommand", "ViewCommand",
        DEF_XVIEWCOMMAND, Blt_Offset(Scrollset, xViewCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-yscrollbar", "yScrollbar", "Scrollbar", DEF_YSCROLLBAR, 
        Blt_Offset(Scrollset, yScrollbarObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
        DEF_YSCROLLCOMMAND, Blt_Offset(Scrollset, yReqScrollCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-yscrollincrement", "yScrollIncrement",
        "ScrollIncrement", DEF_YSCROLLINCREMENT, 
         Blt_Offset(Scrollset, yScrollUnits),BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-yviewcommand", "yViewCommand", "ViewCommand",
        DEF_YVIEWCOMMAND, Blt_Offset(Scrollset, yViewCmdObjPtr), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS, "-width", "width", "Width", DEF_WIDTH, 
        Blt_Offset(Scrollset, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-window", "window", "Window", DEF_WINDOW, 
        Blt_Offset(Scrollset, slaveObjPtr), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
        0, 0}
};

static Tk_GeomRequestProc ScrollsetGeometryProc;
static Tk_GeomLostSlaveProc ScrollsetCustodyProc;
static Tk_GeomMgr scrollsetMgrInfo = {
    (char *)"scrollset",                /* Name of geometry manager used by
                                         * winfo. */
    ScrollsetGeometryProc,              /* Procedure to for new geometry
                                         * requests. */
    ScrollsetCustodyProc,               /* Procedure when scrollbar is taken
                                         * away. */
};

static Tcl_IdleProc DisplayScrollset;
static Tcl_FreeProc DestroyScrollset;
static Tk_EventProc WindowEventProc;
static Tk_EventProc ScrollsetEventProc;
static Tcl_ObjCmdProc ScrollsetInstCmdProc;
static Tcl_CmdDeleteProc ScrollsetInstCmdDeletedProc;

typedef int (ScrollsetCmdProc)(Scrollset *setPtr, Tcl_Interp *interp, 
        int objc, Tcl_Obj *const *objv);

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *      Tells the Tk dispatcher to call the scrollset display routine at the
 *      next idle point.  This request is made only if the window is displayed
 *      and no other redraw request is pending.
 *
 * Results: None.
 *
 * Side effects:
 *      The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyRedraw(Scrollset *setPtr) 
{
    if ((setPtr->tkwin != NULL) && !(setPtr->flags & REDRAW_PENDING)) {
        Tcl_DoWhenIdle(DisplayScrollset, setPtr);
        setPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLimits --
 *
 *      Converts the list of elements into zero or more pixel values which
 *      determine the range of pixel values possible.  An element can be in any
 *      form accepted by Tk_GetPixels. The list has a different meaning based
 *      upon the number of elements.
 *
 *          # of elements:
 *
 *          0 - the limits are reset to the defaults.
 *          1 - the minimum and maximum values are set to this
 *              value, freezing the range at a single value.
 *          2 - first element is the minimum, the second is the
 *              maximum.
 *          3 - first element is the minimum, the second is the
 *              maximum, and the third is the nominal value.
 *
 *      Any element may be the empty string which indicates the default.
 *
 * Results:
 *      The return value is a standard TCL result.  The min and max fields
 *      of the range are set.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToLimits(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Widget of table */
    Tcl_Obj *objPtr,                    /* New width list */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Limits *limitsPtr = (Limits *)(widgRec + offset);
    int objc;
    int limits[3];
    int limitsFlags;

    /* Initialize limits to default values */
    limits[2] = LIMITS_NOM;
    limits[1] = LIMITS_MAX;
    limits[0] = LIMITS_MIN;
    limitsFlags = 0;

    objc = 0;
    if (objPtr != NULL) {
        Tcl_Obj **objv;
        int size;
        int i;

        if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
            return TCL_ERROR;
        }
        if (objc > 3) {
            Tcl_AppendResult(interp, "wrong # limits \"", 
                Tcl_GetString(objPtr), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        for (i = 0; i < objc; i++) {
            const char *string;

            string = Tcl_GetString(objv[i]);
            if (string[0] == '\0') {
                continue;       /* Empty string: use default value */
            }
            limitsFlags |= (1 << i);
            if (Blt_GetPixelsFromObj(interp, tkwin, objv[i], PIXELS_ANY,
                                     &size) != TCL_OK) {
                return TCL_ERROR;
            }
            if ((size < LIMITS_MIN) || (size > LIMITS_MAX)) {
                Tcl_AppendResult(interp, "bad limits \"", Tcl_GetString(objPtr),
                                 "\"", (char *)NULL);
                return TCL_ERROR;
            }
            limits[i] = size;
        }
    }
    /*
    * Check the limits specified.  
    */
    switch (objc) {
    case 1:
        limitsFlags |= (LIMITS_MIN | LIMITS_MAX);
        limits[1] = limits[0];          /* Set minimum and maximum to value */
        break;

    case 2:
        if (limits[1] < limits[0]) {
            Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
                "\": min > max", (char *)NULL);
            return TCL_ERROR;           /* Minimum is greater than maximum */
        }
        break;

    case 3:
        if (limits[1] < limits[0]) {
            Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
                             "\": min > max", (char *)NULL);
            return TCL_ERROR;           /* Minimum is greater than maximum */
        }
        if ((limits[2] < limits[0]) || (limits[2] > limits[1])) {
            Tcl_AppendResult(interp, "nominal value \"", 
                             Tcl_GetString(objPtr),
                             "\" out of range", (char *)NULL);
            return TCL_ERROR;           /* Nominal is outside of range defined
                                         * by minimum and maximum */
        }
        break;
    }
    limitsPtr->min = limits[0];
    limitsPtr->max = limits[1];
    limitsPtr->nom = limits[2];
    limitsPtr->flags = limitsFlags;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LimitsToObj --
 *
 *      Convert the limits of the pixel values allowed into a list.
 *
 * Results:
 *      The string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LimitsToObj(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Not used. */
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Row/column structure record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Limits *limitsPtr = (Limits *)(widgRec + offset);
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (limitsPtr->flags & LIMITS_MIN) {
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                                 Tcl_NewIntObj(limitsPtr->min));
    } else {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (limitsPtr->flags & LIMITS_MAX) {
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                                 Tcl_NewIntObj(limitsPtr->max));
    } else {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    if (limitsPtr->flags & LIMITS_NOM) {
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                                 Tcl_NewIntObj(limitsPtr->nom));
    } else {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("", -1));
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetLimits --
 *
 *      Resets the limits to their default values.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
INLINE static void
ResetLimits(Limits *limitsPtr)          /* Limits to be imposed on the value */
{
    limitsPtr->flags = 0;
    limitsPtr->min = LIMITS_MIN;
    limitsPtr->max = LIMITS_MAX;
    limitsPtr->nom = LIMITS_NOM;
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
    int dx, int dy,                     /* Difference between outer and inner
                                         * regions */
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

/*
 *---------------------------------------------------------------------------
 *
 * GetBoundedWidth --
 *
 *      Bounds a given width value to the limits described in the limit
 *      structure.  The initial starting value may be overridden by the nominal
 *      value in the limits.
 *
 * Results:
 *      Returns the constrained value.
 *
 *---------------------------------------------------------------------------
 */
static INLINE int
GetBoundedWidth(
    int width,                          /* Initial value to be constrained */
    Limits *limitsPtr)                  /* Limits to be imposed on the
                                         * value. */
{
    if (limitsPtr->flags & LIMITS_NOM) {
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
 * GetBoundedHeight --
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
static INLINE int
GetBoundedHeight(
    int height,                         /* Initial value to be constrained */
    Limits *limitsPtr)                  /* Limits to be imposed on the
                                         * value. */
{
    if (limitsPtr->flags & LIMITS_NOM) {
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
 * GetSlaveReqWidth --
 *
 *      Returns the width requested by the widget starting in the given entry.
 *      The requested space also includes any internal padding which has been
 *      designated for this widget.
 *
 *      The requested width of the widget is always bounded by the limits set
 *      in tePtr->reqWidth.
 *
 * Results:
 *      Returns the requested width of the widget.
 *
 *---------------------------------------------------------------------------
 */
static INLINE int
GetSlaveReqWidth(Scrollset *setPtr)
{
    int width;

    width = 2 * setPtr->iPadX;
    if (setPtr->slave != NULL) {
        width += Tk_ReqWidth(setPtr->slave);
    }
    width = GetBoundedWidth(width, &setPtr->reqSlaveWidth);
    return width;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetSlaveReqHeight --
 *
 *      Returns the height requested by the widget starting in the given entry.
 *      The requested space also includes any internal padding which has been
 *      designated for this widget.
 *
 *      The requested height of the widget is always bounded by the limits set
 *      in setPtr->reqSlaveHeight.
 *
 * Results:
 *      Returns the requested height of the widget.
 *
 *---------------------------------------------------------------------------
 */
static INLINE int
GetSlaveReqHeight(Scrollset *setPtr)
{
    int height;

    height = 2 * setPtr->iPadY;
    if (setPtr->slave != NULL) {
        height += Tk_ReqHeight(setPtr->slave);
    }
    height = GetBoundedHeight(height, &setPtr->reqSlaveHeight);
    return height;
}

static void
UnmanageWindow(Scrollset *setPtr, Tk_Window tkwin)
{
    Tk_DeleteEventHandler(tkwin, StructureNotifyMask, WindowEventProc, setPtr);
    Tk_ManageGeometry(tkwin, (Tk_GeomMgr *)NULL, setPtr);
    if (Tk_IsMapped(tkwin)) {
        Tk_UnmapWindow(tkwin);
    }
}

static void
ManageWindow(Scrollset *setPtr, Tk_Window tkwin)
{
    Tk_CreateEventHandler(tkwin, StructureNotifyMask, WindowEventProc, setPtr);
    Tk_ManageGeometry(tkwin, &scrollsetMgrInfo, setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * InstallWindow --
 *
 *      Convert the path of a Tk window into a Tk_Window pointer.
 *
 * Results:
 *      The return value is a standard TCL result.  The window pointer is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InstallWindow(
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Scrollset *setPtr,
    Tcl_Obj *objPtr,                    /* String representing scrollbar
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
                         "\" must be a slave of scrollset.", (char *)NULL);
        return TCL_ERROR;
    }
    ManageWindow(setPtr, tkwin);
    *tkwinPtr = tkwin;
    return TCL_OK;
}

static void
InstallXScrollbar(ClientData clientData)
{
    Scrollset *setPtr = clientData;
    Tcl_Interp *interp;

    interp = setPtr->interp;
    setPtr->flags &= ~XSCROLLBAR_PENDING;
    if (setPtr->tkwin == NULL) {
        return;                         /* Widget has been destroyed. */
    }
    if (setPtr->xScrollbarObjPtr == NULL) {
        return;                         /* Scrollbar has been undefined. */
    }
    if (InstallWindow(interp, setPtr, setPtr->xScrollbarObjPtr, 
        &setPtr->xScrollbar) != TCL_OK) {
        Tcl_BackgroundError(setPtr->interp);
        return;
    } 
    if (setPtr->xScrollCmdObjPtr != NULL) {
        Tcl_DecrRefCount(setPtr->xScrollCmdObjPtr);
        setPtr->xScrollCmdObjPtr = NULL;
    }
    if (setPtr->xReqScrollCmdObjPtr != NULL) {
        Tcl_IncrRefCount(setPtr->xReqScrollCmdObjPtr);
        setPtr->xScrollCmdObjPtr = setPtr->xReqScrollCmdObjPtr;
    } else {
        Tcl_Obj *listObjPtr;

        /* Defaults to "scrollbar set". */
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj(Tk_PathName(setPtr->xScrollbar), -1));
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj("set", 3));
        Tcl_IncrRefCount(listObjPtr);
        setPtr->xScrollCmdObjPtr = listObjPtr;
    }
}

static void
InstallYScrollbar(ClientData clientData)
{
    Scrollset *setPtr = clientData;
    Tcl_Interp *interp;

    interp = setPtr->interp;
    setPtr->flags &= ~YSCROLLBAR_PENDING;
    if (setPtr->tkwin == NULL) {
        return;                         /* Widget has been destroyed. */
    }
    if (setPtr->yScrollbarObjPtr == NULL) {
        return;                         /* Scrollbar has been undefined */
    }
    if (InstallWindow(interp, setPtr, setPtr->yScrollbarObjPtr, 
        &setPtr->yScrollbar) != TCL_OK) {
        Tcl_BackgroundError(setPtr->interp);
        return;
    }
    if (setPtr->yScrollCmdObjPtr != NULL) {
        Tcl_DecrRefCount(setPtr->yScrollCmdObjPtr);
        setPtr->yScrollCmdObjPtr = NULL;
    }
    if (setPtr->yReqScrollCmdObjPtr != NULL) {
        Tcl_IncrRefCount(setPtr->yReqScrollCmdObjPtr);
        setPtr->yScrollCmdObjPtr = setPtr->yReqScrollCmdObjPtr;
    } else {
        Tcl_Obj *listObjPtr;

        /* Defaults to "scrollbar set". */
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj(Tk_PathName(setPtr->yScrollbar), -1));
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj("set",3));
        Tcl_IncrRefCount(listObjPtr);
        setPtr->yScrollCmdObjPtr = listObjPtr;
    }
}


static void
InstallSlave(ClientData clientData)
{
    Scrollset *setPtr = clientData;
    Tcl_Interp *interp;
    Tcl_Obj *cmdObjPtr;
    int result;

    interp = setPtr->interp;
    setPtr->flags &= ~(SLAVE_PENDING | SLAVE_XVIEW | SLAVE_YVIEW);
    if (setPtr->tkwin == NULL) {
        return;                         /* Widget has been destroyed. */
    }
    if (InstallWindow(interp, setPtr, setPtr->slaveObjPtr, &setPtr->slave) 
        != TCL_OK) {
        Tcl_BackgroundError(interp);
        return;
    }

    /* Check if the slave widget has a "yview" operation. */
    if (setPtr->yViewCmdObjPtr != NULL) {
        cmdObjPtr = Tcl_DuplicateObj(setPtr->yViewCmdObjPtr);
    } else {
        cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, setPtr->slaveObjPtr);
        Tcl_ListObjAppendElement(interp, cmdObjPtr,Tcl_NewStringObj("yview",5));
    }
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    Tcl_ResetResult(interp);
    if (result == TCL_OK) {
        setPtr->flags |= SLAVE_YVIEW;
    }

    /* Check if the slave widget has a "xview" operation. */
    if (setPtr->xViewCmdObjPtr != NULL) {
        cmdObjPtr = Tcl_DuplicateObj(setPtr->xViewCmdObjPtr);
    } else {
        cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, setPtr->slaveObjPtr);
        Tcl_ListObjAppendElement(interp, cmdObjPtr,Tcl_NewStringObj("xview",5));
    }
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    Tcl_ResetResult(interp);
    if (result == TCL_OK) {
        setPtr->flags |= SLAVE_XVIEW;
    }
}

static void
UpdateScrollset(ClientData clientData)
{
    Scrollset *setPtr = clientData;
    Tcl_Interp *interp;

    interp = setPtr->interp;
    /* 
     * Execute the initialization procedure on this widget.
     */
    setPtr->flags &= ~UPDATE_PENDING;
    if (setPtr->tkwin == NULL) {
        return;                         /* Widget has been destroyed. */
    }
    if (Tcl_VarEval(interp, "::blt::Scrollset::ConfigureScrollbars ", 
                Tk_PathName(setPtr->tkwin), (char *)NULL) != TCL_OK) {
        Tcl_BackgroundError(interp);
    }
}

static void
ComputeSlaveGeometry(Scrollset *setPtr)
{
    int cavityWidth, cavityHeight;
    int dx, dy;
    int slaveWidth, slaveHeight;
    int x, y;
    
    cavityWidth  = Tk_Width(setPtr->tkwin);
    cavityHeight = Tk_Height(setPtr->tkwin);
    slaveWidth = GetSlaveReqWidth(setPtr);
    slaveHeight = GetSlaveReqHeight(setPtr);
    setPtr->flags &= ~(DISPLAY_X | DISPLAY_Y);
    if ((setPtr->flags & SLAVE_XVIEW) == 0) {
        setPtr->worldWidth = slaveWidth;
    }
    if ((setPtr->flags & SLAVE_YVIEW) == 0) {
        setPtr->worldHeight = slaveHeight;
    }
    setPtr->xScrollbarHeight = setPtr->yScrollbarWidth = 0;
    if ((setPtr->xScrollbar != NULL) && (setPtr->worldWidth > cavityWidth)) {
        setPtr->xScrollbarHeight = Tk_ReqHeight(setPtr->xScrollbar);
        cavityHeight -= setPtr->xScrollbarHeight;
        setPtr->flags |= DISPLAY_X;
    } 
    if ((setPtr->yScrollbar != NULL) && (setPtr->worldHeight > cavityHeight)) {
        setPtr->yScrollbarWidth = Tk_ReqWidth(setPtr->yScrollbar);
        cavityWidth -= setPtr->yScrollbarWidth;
        setPtr->flags |= DISPLAY_Y;
    }
    if ((setPtr->xScrollbar != NULL) && (setPtr->xScrollbarHeight == 0) && 
        (setPtr->worldWidth > cavityWidth)) {
        setPtr->xScrollbarHeight = Tk_ReqHeight(setPtr->xScrollbar);
        cavityHeight -= setPtr->xScrollbarHeight;
        setPtr->flags |= DISPLAY_X;
    }
    if ((setPtr->yScrollbar != NULL) && (setPtr->yScrollbarWidth == 0) && 
        (setPtr->worldHeight > cavityHeight)) {
        setPtr->yScrollbarWidth = Tk_ReqWidth(setPtr->yScrollbar);
        cavityWidth -= setPtr->yScrollbarWidth;
        setPtr->flags |= DISPLAY_Y;
    }
    dx = dy = 0;
    if ((cavityWidth - PADDING(setPtr->padX)) > slaveWidth) {
        cavityWidth -= PADDING(setPtr->padX);
        if (setPtr->fill & FILL_X) {
            slaveWidth = cavityWidth;
            if (slaveWidth > setPtr->reqSlaveWidth.max) {
                slaveWidth = setPtr->reqSlaveWidth.max;
            }
        } else {
            dx = (cavityWidth - slaveWidth);
        }
        setPtr->xOffset = 0;
    } else if (setPtr->flags & SLAVE_XVIEW) {
        slaveWidth = cavityWidth;
    }
    if ((cavityHeight - PADDING(setPtr->padY)) > slaveHeight) {
        cavityHeight -= PADDING(setPtr->padY);
        if (setPtr->fill & FILL_Y) {
            slaveHeight = cavityHeight;
            if (slaveHeight > setPtr->reqSlaveHeight.max) {
                slaveHeight = setPtr->reqSlaveHeight.max;
            } 
        } else {
            dy = (cavityHeight - slaveHeight);
        }
        setPtr->yOffset = 0;
    } else if (setPtr->flags & SLAVE_YVIEW) {
        slaveHeight = cavityHeight;
    }
    x = y = 0;
    if ((dx > 0) || (dy > 0)) {
        TranslateAnchor(dx, dy, setPtr->anchor, &x, &y);
    }
    setPtr->shangleHeight = setPtr->xScrollbarHeight;
    setPtr->shangleWidth  = setPtr->yScrollbarWidth;
    setPtr->yScrollbarHeight = cavityHeight - setPtr->xScrollbarHeight;
    setPtr->xScrollbarWidth  = cavityWidth  - setPtr->yScrollbarWidth;
        
    if (setPtr->slave != NULL) {
        if ((setPtr->xScrollbar == NULL) && (slaveWidth > cavityWidth)) {
            slaveWidth = cavityWidth;
            if (slaveWidth < setPtr->reqSlaveWidth.min) {
                slaveWidth = setPtr->reqSlaveWidth.min;
            } 
        }
        if ((setPtr->yScrollbar == NULL) && (slaveHeight > cavityHeight)) {
            slaveHeight = cavityHeight;
            if (slaveHeight < setPtr->reqSlaveHeight.min) {
                slaveHeight = setPtr->reqSlaveHeight.min;
            } 
        }
        if (cavityWidth > slaveWidth) {
            x += Tk_Changes(setPtr->slave)->border_width;
        } else {
            x = Tk_Changes(setPtr->slave)->border_width;
        }
        if (cavityHeight > slaveHeight) {
            y += Tk_Changes(setPtr->slave)->border_width;
        } else { 
            y = Tk_Changes(setPtr->slave)->border_width;
        }
    }
    setPtr->slaveX = x;
    setPtr->slaveY = y;
    setPtr->slaveWidth = slaveWidth;
    setPtr->slaveHeight = slaveHeight;
    setPtr->cavityWidth = cavityWidth;
    setPtr->cavityHeight = cavityHeight;
    
    /* Adjust the scroll offsets to put as much of the slave widget in view as
     * possible. */
    if ((setPtr->flags & SLAVE_YVIEW) == 0) {
        if ((slaveHeight - setPtr->yOffset) < cavityHeight) {
            setPtr->yOffset = slaveHeight - cavityHeight;
        }
        setPtr->flags |= SCROLLY;
    }
    if ((setPtr->flags & SLAVE_XVIEW) == 0) {
        if ((slaveWidth - setPtr->xOffset) < cavityWidth) {
            setPtr->xOffset = slaveWidth - cavityWidth;
        }
        setPtr->flags |= SCROLLX;
    }
    setPtr->flags &= ~LAYOUT_PENDING;
}

static int
ConfigureScrollset(Tcl_Interp *interp, Scrollset *setPtr, int objc,
                   Tcl_Obj *const *objv, int flags)
{
    int updateNeeded;

    if (Blt_ConfigureWidgetFromObj(interp, setPtr->tkwin, scrollsetSpecs, 
        objc, objv, (char *)setPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    /* 
     * Install the scrollbars and slave widget at a later time after the
     * scrollset window has been created.  We defer installing the slave and
     * scrollbars so the scrollbar widgets don't have to exist when they are
     * specified by the -xscrollbar, -yscrollbar, and -window options
     * respectively. The downside is that errors messages will be 
     * backgrounded.
     */
    updateNeeded = FALSE;
    if (Blt_ConfigModified(scrollsetSpecs, "-xscrollbar", (char *)NULL)) {
        if (setPtr->xScrollbar != NULL) {
            /* Immediately unmanage the current scrollbar. */
            UnmanageWindow(setPtr, setPtr->xScrollbar);
            setPtr->xScrollbar = NULL;
        }
        if ((setPtr->flags & XSCROLLBAR_PENDING) == 0) {
            Tcl_DoWhenIdle(InstallXScrollbar, setPtr);
            setPtr->flags |= XSCROLLBAR_PENDING;
        }           
        updateNeeded = TRUE;
    }
    if (Blt_ConfigModified(scrollsetSpecs, "-yscrollbar", (char *)NULL)) {
        if (setPtr->yScrollbar != NULL) {
            /* Immediately unmanage the current scrollbar. */
            UnmanageWindow(setPtr, setPtr->yScrollbar);
            setPtr->yScrollbar = NULL;
        }
        if ((setPtr->flags & YSCROLLBAR_PENDING) == 0) {
            Tcl_DoWhenIdle(InstallYScrollbar, setPtr);
            setPtr->flags |= YSCROLLBAR_PENDING;
        }           
        updateNeeded = TRUE;
    }
    if (Blt_ConfigModified(scrollsetSpecs, "-window", (char *)NULL)) {
        if (setPtr->slave != NULL) {
            UnmanageWindow(setPtr, setPtr->slave);
            setPtr->slave = NULL;
        }
        if ((setPtr->flags & SLAVE_PENDING) == 0) {
            Tcl_DoWhenIdle(InstallSlave, setPtr);
            setPtr->flags |= SLAVE_PENDING;
        }           
        updateNeeded = TRUE;
    }
    if (updateNeeded) {
        if ((setPtr->flags & UPDATE_PENDING) == 0) {
            Tcl_DoWhenIdle(UpdateScrollset, setPtr);
            setPtr->flags |= UPDATE_PENDING;
        }           
    }
    return TCL_OK;
}

/* Widget Callbacks */

/*
 *---------------------------------------------------------------------------
 *
 * ScrollsetEventProc --
 *
 *      This procedure is invoked by the Tk dispatcher for various events on
 *      comboentry widgets.
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
ScrollsetEventProc(ClientData clientData, XEvent *eventPtr)
{
    Scrollset *setPtr = clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedraw(setPtr);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        setPtr->flags |= LAYOUT_PENDING;
        EventuallyRedraw(setPtr);
    } else if (eventPtr->type == DestroyNotify) {
        if (setPtr->tkwin != NULL) {
            setPtr->tkwin = NULL; 
        }
        if (setPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayScrollset, setPtr);
        }
        if (setPtr->flags & XSCROLLBAR_PENDING) {
            Tcl_CancelIdleCall(InstallXScrollbar, setPtr);
        }
        if (setPtr->flags & YSCROLLBAR_PENDING) {
            Tcl_CancelIdleCall(InstallYScrollbar, setPtr);
        }
        if (setPtr->flags & SLAVE_PENDING) {
            Tcl_CancelIdleCall(InstallSlave, setPtr);
        }           
        if (setPtr->flags & UPDATE_PENDING) {
            Tcl_CancelIdleCall(UpdateScrollset, setPtr);
        }           
        Tcl_EventuallyFree(setPtr, DestroyScrollset);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowEventProc --
 *
 *      This procedure is invoked by the Tk event handler when StructureNotify
 *      events occur in a scrollbar managed by the widget.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
WindowEventProc(
    ClientData clientData,              /* Pointer to Entry structure for widget
                                         * referred to by eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    Scrollset *setPtr = clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedraw(setPtr);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        EventuallyRedraw(setPtr);
    } else if (eventPtr->type == DestroyNotify) {
        if ((setPtr->yScrollbar != NULL) && 
            (eventPtr->xany.window == Tk_WindowId(setPtr->yScrollbar))) {
            setPtr->yScrollbar = NULL;
        } else if ((setPtr->xScrollbar != NULL) && 
                   (eventPtr->xany.window == Tk_WindowId(setPtr->xScrollbar))) {
            setPtr->xScrollbar = NULL;
        } else if ((setPtr->slave != NULL) && 
                   (eventPtr->xany.window == Tk_WindowId(setPtr->slave))) {
            setPtr->slave = NULL;
        } else if ((setPtr->shangle != NULL) && 
                   (eventPtr->xany.window == Tk_WindowId(setPtr->shangle))) {
            setPtr->shangle = NULL;
        } 
        setPtr->flags |= LAYOUT_PENDING;
        EventuallyRedraw(setPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollsetCustodyProc --
 *
 *      This procedure is invoked when a scrollbar has been stolen by another
 *      geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
  *     Arranges for the scrollset to have its layout re-arranged at the next
 *      idle point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ScrollsetCustodyProc(ClientData clientData, Tk_Window tkwin)
{
    Scrollset *setPtr = (Scrollset *)clientData;

    if (tkwin == setPtr->yScrollbar) {
        setPtr->yScrollbar = NULL;
        setPtr->yScrollbarWidth = 0;
        setPtr->flags &= ~DISPLAY_Y;
    } else if (tkwin == setPtr->xScrollbar) {
        setPtr->xScrollbar = NULL;
        setPtr->xScrollbarHeight = 0;
        setPtr->flags &= ~DISPLAY_X;
    } else if (tkwin == setPtr->slave) {
        setPtr->slave = NULL;
        setPtr->slaveWidth = setPtr->slaveHeight = 0;
        setPtr->flags &= ~(DISPLAY_X|DISPLAY_Y);
    }
    Tk_UnmaintainGeometry(tkwin, setPtr->tkwin);
    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollsetGeometryProc --
 *
 *      This procedure is invoked by Tk_GeometryRequest for scrollbars managed
 *      by the scrollset.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the scrollset to have its layout re-computed and
 *      re-arranged at the next idle point.
 *
 * -------------------------------------------------------------------------- 
*/
/* ARGSUSED */
static void
ScrollsetGeometryProc(ClientData clientData, Tk_Window tkwin)
{
    Scrollset *setPtr = (Scrollset *)clientData;

    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
}

/* Widget Operations */

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      Commands may get excecuted; variables may get set; sub-menus may
 *      get posted.
 *
 *      .cm configure ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(Scrollset *setPtr, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    int result;

    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, setPtr->tkwin, scrollsetSpecs, 
                (char *)setPtr, (Tcl_Obj *)NULL,  0);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, setPtr->tkwin, scrollsetSpecs, 
                (char *)setPtr, objv[2], 0);
    }
    Tcl_Preserve(setPtr);
    result = ConfigureScrollset(interp, setPtr, objc - 2, objv + 2, 
                BLT_CONFIG_OBJV_ONLY);
    Tcl_Release(setPtr);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    setPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(setPtr);
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
 *      .cm cget option
 *
 *---------------------------------------------------------------------------
 */
static int
CgetOp(Scrollset *setPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    return Blt_ConfigureValueFromObj(interp, setPtr->tkwin, scrollsetSpecs,
        (char *)setPtr, objv[2], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * SetOp --
 *
 *      Relays the "set" operation from the slave widget to scrollbar.  This
 *      routine checks to see if the first/last values are 0 and 1 respectively,
 *      indicating no scrollbar is necessary.  This is used for slave widgets
 *      with scrolling capabilities to determine when scrollbars are needed.
 *
 *      .ss xset first last 
 *      .ss yset first last 
 *
 *---------------------------------------------------------------------------
 */
static int
SetOp(Scrollset *setPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    const char *string;
    const char *scrollbar;
    unsigned int flag;
    int useX;

    scrollbar = NULL;
    string = Tcl_GetString(objv[1]);
    flag = 0;
    if (string[0] == 'x') {
        if (setPtr->xScrollbar != NULL) {
            scrollbar = Tk_PathName(setPtr->xScrollbar);
        }
        useX = TRUE;
        if (setPtr->flags & SLAVE_XVIEW) {
            flag = DISPLAY_X;
        }
    } else if (string[0] == 'y') {
        if (setPtr->yScrollbar != NULL) {
            scrollbar = Tk_PathName(setPtr->yScrollbar);
        }
        useX = FALSE;
        if (setPtr->flags & SLAVE_YVIEW) {
            flag = DISPLAY_Y;
        }
    } else {
        Tcl_AppendResult(interp, 
                "bad scrollset option: should be xset or yset", (char *)NULL);
        return TCL_ERROR;
    }
    if (flag != 0) { 
        double first, last;

        /* Examine the set values from the slave. */
        if (Tcl_GetDoubleFromObj(interp, objv[2], &first) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Tcl_GetDoubleFromObj(interp, objv[3], &last) != TCL_OK) {
            return TCL_ERROR;
        }
        if ((first <= 0.0) && (last >= 1.0)) {
            setPtr->flags &= ~flag;     /* Hide scrollbar. */
            if (useX) {
                setPtr->worldWidth = 0;
            } else {
                setPtr->worldHeight = 0;
            }
        } else if (useX) {
            setPtr->worldWidth = 0;
            if ((last > first) && (Tk_Width(setPtr->slave) > 1)) {
                setPtr->worldWidth = 
                    (int)((double)Tk_Width(setPtr->slave) / (last - first));
            } 
            setPtr->flags |= DISPLAY_X; /* Display x-scrollbar. */
        } else {
            setPtr->worldHeight = 0;
            if ((last > first) && (Tk_Height(setPtr->slave) > 1)) {
                setPtr->worldHeight = 
                    (int)((double)Tk_Height(setPtr->slave) / (last - first));
            }
            setPtr->flags |= DISPLAY_Y; /* Display y-scrollbar. */
        }
        setPtr->flags |= LAYOUT_PENDING;
        EventuallyRedraw(setPtr);
    }
    if (scrollbar != NULL) {
        Tcl_Obj *cmdObjPtr, *objPtr;
        int result;

        cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        objPtr = Tcl_NewStringObj(scrollbar, -1);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        objPtr = Tcl_NewStringObj("set", 3);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objv[2]);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objv[3]);
        Tcl_IncrRefCount(cmdObjPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
XviewOp(Scrollset *setPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int w;

    if (setPtr->flags & SLAVE_XVIEW) {
        Tcl_Obj *cmdObjPtr;
        int i;
        int result;

        /* The slave widget has a "xview" operation.  Simply relay the
         * information on to the slave widget by calling its "xview"
         * operation. */
        if (setPtr->xViewCmdObjPtr != NULL) {
            cmdObjPtr = Tcl_DuplicateObj(setPtr->xViewCmdObjPtr);
        } else {
            cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
            Tcl_ListObjAppendElement(interp, cmdObjPtr, setPtr->slaveObjPtr);
            Tcl_ListObjAppendElement(interp, cmdObjPtr, 
                Tcl_NewStringObj("xview", 5));
        }
        for (i = 2; i < objc; i++) {
            Tcl_ListObjAppendElement(interp, cmdObjPtr, objv[i]);
        }
        Tcl_IncrRefCount(cmdObjPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(cmdObjPtr);
        return result;
    }
    w = VPORTWIDTH(setPtr);
    if (objc == 2) {
        double first, last;
        Tcl_Obj *listObjPtr;

        /*
         * Note: we are bounding the fractions between 0.0 and 1.0 to support
         * the "canvas"-style of scrolling.
         */
        if (setPtr->worldWidth < 1) {
            first = 0.0, last = 1.0;
        } else {
            first = (double)setPtr->xOffset / setPtr->worldHeight;
            last  = (double)(setPtr->xOffset + w) / setPtr->worldWidth;
            first = FCLAMP(first);
            last  = FCLAMP(last);
        }
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(first));
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(last));
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &setPtr->xOffset,
        setPtr->worldWidth, w, setPtr->xScrollUnits, BLT_SCROLL_MODE_HIERBOX) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    setPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(setPtr);
    return TCL_OK;
}


static int
YviewOp(Scrollset *setPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int height;

    if (setPtr->flags & SLAVE_YVIEW) {
        Tcl_Obj *cmdObjPtr;
        int i;
        int result;

        /* The slave widget has a "yview" operation.  Simply relay the
         * information on to the slave widget by calling its "yview"
         * operation. */
        if (setPtr->yViewCmdObjPtr != NULL) {
            cmdObjPtr = Tcl_DuplicateObj(setPtr->yViewCmdObjPtr);
        } else {
            cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
            Tcl_ListObjAppendElement(interp, cmdObjPtr, setPtr->slaveObjPtr);
            Tcl_ListObjAppendElement(interp, cmdObjPtr, 
                                     Tcl_NewStringObj("yview", 5));
        }
        for (i = 2; i < objc; i++) {
            Tcl_ListObjAppendElement(interp, cmdObjPtr, objv[i]);
        }
        Tcl_IncrRefCount(cmdObjPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(cmdObjPtr);
        return result;
    }

    height = VPORTHEIGHT(setPtr);
    if (objc == 2) {
        double first, last;

        /*
         * Note: we are bounding the fractions between 0.0 and 1.0 to support
         * the "canvas"-style of scrolling.
         */
        first = (double)setPtr->yOffset / setPtr->worldHeight;
        last  = (double)(setPtr->yOffset + height) / setPtr->worldHeight;
        Tcl_AppendElement(interp, Blt_Dtoa(interp, FCLAMP(first)));
        Tcl_AppendElement(interp, Blt_Dtoa(interp, FCLAMP(last)));
        return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &setPtr->yOffset,
        setPtr->worldHeight, height, setPtr->yScrollUnits, 
        BLT_SCROLL_MODE_HIERBOX) != TCL_OK) {
        return TCL_ERROR;
    }
    setPtr->flags |= SCROLL_PENDING;
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyScrollset --
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
DestroyScrollset(DestroyData dataPtr)           /* Pointer to the widget
                                                 * record. */
{
    Scrollset *setPtr = (Scrollset *)dataPtr;

    if (setPtr->flags & REDRAW_PENDING) {
        Tcl_CancelIdleCall(DisplayScrollset, setPtr);
    }
    if (setPtr->flags & XSCROLLBAR_PENDING) {
        Tcl_CancelIdleCall(InstallXScrollbar, setPtr);
    }
    if (setPtr->flags & YSCROLLBAR_PENDING) {
        Tcl_CancelIdleCall(InstallYScrollbar, setPtr);
    }
    if (setPtr->flags & SLAVE_PENDING) {
        Tcl_CancelIdleCall(InstallSlave, setPtr);
    }       
    if (setPtr->flags & UPDATE_PENDING) {
        Tcl_CancelIdleCall(UpdateScrollset, setPtr);
    }       
    if (setPtr->xScrollCmdObjPtr != NULL) {
        Tcl_DecrRefCount(setPtr->xScrollCmdObjPtr);
    }
    if (setPtr->yScrollCmdObjPtr != NULL) {
        Tcl_DecrRefCount(setPtr->yScrollCmdObjPtr);
    }
    Blt_FreeOptions(scrollsetSpecs, (char *)setPtr, setPtr->display, 0);
    Tcl_DeleteCommandFromToken(setPtr->interp, setPtr->cmdToken);
    Blt_Free(setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewScrollset --
 *
 *---------------------------------------------------------------------------
 */
static Scrollset *
NewScrollset(Tcl_Interp *interp, Tk_Window tkwin)
{
    Scrollset *setPtr;

    setPtr = Blt_AssertCalloc(1, sizeof(Scrollset));

    Tk_SetClass(tkwin, "BltScrollset");
    setPtr->tkwin = tkwin;
    setPtr->display = Tk_Display(tkwin);
    setPtr->interp = interp;
    setPtr->flags |= (LAYOUT_PENDING | SCROLL_PENDING);
    setPtr->xScrollUnits = 2;
    setPtr->yScrollUnits = 2;
    setPtr->anchor = TK_ANCHOR_CENTER;
    setPtr->fill = FILL_BOTH;
    ResetLimits(&setPtr->reqSlaveWidth);
    ResetLimits(&setPtr->reqSlaveHeight);
    Blt_SetWindowInstanceData(tkwin, setPtr);
    return setPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollsetCmd --
 *
 *      This procedure is invoked to process the "scrollset" command.  See the
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
static Blt_OpSpec scrollsetOps[] =
{
    {"cget",        2, CgetOp,        3, 3, "option",},
    {"configure",   2, ConfigureOp,   2, 0, "?option value?...",},
    {"xset",        2, SetOp,         4, 4, "first last",},
    {"xview",       2, XviewOp,       2, 5, 
        "?moveto fract? ?scroll number what?",},
    {"yset",        2, SetOp,         4, 4, "first last",},
    {"yview",       2, YviewOp,       2, 5, 
        "?moveto fract? ?scroll number what?",},
};

static int numScrollsetOps = sizeof(scrollsetOps) / sizeof(Blt_OpSpec);

typedef int (ScrollsetInstOp)(Scrollset *setPtr, Tcl_Interp *interp, int objc,
                          Tcl_Obj *const *objv);

static int
ScrollsetInstCmdProc(
    ClientData clientData,              /* Information about the widget. */
    Tcl_Interp *interp,                 /* Interpreter to report errors back
                                         * to. */
    int objc,                           /* Number of arguments. */
    Tcl_Obj *const *objv)               /* Argument vector. */
{
    ScrollsetInstOp *proc;
    Scrollset *setPtr = clientData;
    int result;

    proc = Blt_GetOpFromObj(interp, numScrollsetOps, scrollsetOps, BLT_OP_ARG1, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    Tcl_Preserve(setPtr);
    result = (*proc) (setPtr, interp, objc, objv);
    Tcl_Release(setPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollsetInstCmdDeletedProc --
 *
 *      This procedure can be called if the window was destroyed (tkwin will
 *      be NULL) and the command was deleted automatically.  In this case, we
 *      need to do nothing.
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
ScrollsetInstCmdDeletedProc(ClientData clientData)
{
    Scrollset *setPtr = clientData;     /* Pointer to widget record. */

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
 * ScrollsetCmd --
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
ScrollsetCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Scrollset *setPtr;
    Tk_Window tkwin;
    unsigned int mask;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
                (char *)NULL);
        return TCL_ERROR;
    }
    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), 
                Tcl_GetString(objv[1]), (char *)NULL);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    setPtr = NewScrollset(interp, tkwin);
    if (ConfigureScrollset(interp, setPtr, objc - 2, objv + 2, 0) != TCL_OK) {
        Tk_DestroyWindow(setPtr->tkwin);
        return TCL_ERROR;
    }
    mask = (ExposureMask | StructureNotifyMask);
    Tk_CreateEventHandler(tkwin, mask, ScrollsetEventProc, setPtr);
    setPtr->cmdToken = Tcl_CreateObjCommand(interp, Tk_PathName(tkwin), 
        ScrollsetInstCmdProc, setPtr, ScrollsetInstCmdDeletedProc);

    /* 
     * The shangle is a small window covering the lower-right corner of the
     * widget that is unobscurred by both the horizontal of vertical
     * scrollbar. You can pack a widget into it or use "bind" to add
     * shangle-like resize behaviors. 
     */
    setPtr->shangle = Tk_CreateWindow(interp, tkwin, "shangle", (char *)NULL);
    Tk_CreateEventHandler(setPtr->shangle, mask, WindowEventProc, setPtr);

    /*
     * First time in this interpreter, load in a procedure to initialize
     * various bindings on the scrollset widget.  We deferred sourcing the
     * file until now so that the variable $blt_library can be set within a
     * script.
     */
    if (!Blt_CommandExists(interp, "::blt::Scrollset::ConfigureScrollbars")) {
        if (Tcl_GlobalEval(interp, 
                "source [file join $blt_library bltScrollset.tcl]") != TCL_OK) {
            char info[200];
            Blt_FormatString(info, 200, "\n    (while loading bindings for %.50s)", 
                    Tcl_GetString(objv[0]));
            Tcl_AddErrorInfo(interp, info);
            return TCL_ERROR;
        }
    }
    Tcl_SetObjResult(interp, objv[1]); 
    return TCL_OK;
}

int
Blt_ScrollsetCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "scrollset", ScrollsetCmd, };

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

static void
DrawScrollset(Scrollset *setPtr)
{
    if (setPtr->slave != NULL) {
        /*
         * If the widget is too small (i.e. it has only an external border)
         * then unmap it.
         */
        if ((setPtr->slaveWidth < 1) || (setPtr->slaveHeight < 1)) {
            if (Tk_IsMapped(setPtr->slave)) {
                if (setPtr->tkwin != Tk_Parent(setPtr->slave)) {
                    Tk_UnmaintainGeometry(setPtr->slave, setPtr->tkwin);
                }
                Tk_UnmapWindow(setPtr->slave);
            }
        } else {
            int x, y;

            x = setPtr->slaveX;
            y = setPtr->slaveY;
            if (setPtr->xOffset > 0) {
                x -= setPtr->xOffset;
            }
            if (setPtr->yOffset > 0) {
                y -= setPtr->yOffset;
            }
            if (setPtr->tkwin != Tk_Parent(setPtr->slave)) {
                Tk_MaintainGeometry(setPtr->slave, setPtr->tkwin, x, y,
                                    setPtr->slaveWidth, setPtr->slaveHeight);
            } else {
                if ((x != Tk_X(setPtr->slave)) || (y != Tk_Y(setPtr->slave)) ||
                    (setPtr->slaveWidth != Tk_Width(setPtr->slave)) ||
                    (setPtr->slaveHeight != Tk_Height(setPtr->slave))) {
                    Tk_MoveResizeWindow(setPtr->slave, x, y, 
                        setPtr->slaveWidth, setPtr->slaveHeight);
                }
                if (!Tk_IsMapped(setPtr->slave)) {
                    Tk_MapWindow(setPtr->slave);
                }
            }
        }
    }

    /* Manage the geometry of the scrollbars. */
    
    if (setPtr->yScrollbarWidth > 0) {
        int x, y;
        int yScrollbarHeight;

        x = VPORTWIDTH(setPtr);
        y = 0;
        yScrollbarHeight = VPORTHEIGHT(setPtr);
        if ((Tk_Width(setPtr->yScrollbar) != setPtr->yScrollbarWidth) ||
            (Tk_Height(setPtr->yScrollbar) != yScrollbarHeight) ||
            (x != Tk_X(setPtr->yScrollbar)) || 
            (y != Tk_Y(setPtr->yScrollbar))) {
            Tk_MoveResizeWindow(setPtr->yScrollbar, x, y, 
                setPtr->yScrollbarWidth, yScrollbarHeight);
        }
        if (!Tk_IsMapped(setPtr->yScrollbar)) {
            Tk_MapWindow(setPtr->yScrollbar);
            XRaiseWindow(setPtr->display, Tk_WindowId(setPtr->yScrollbar));
        }
    } else if ((setPtr->yScrollbar != NULL) && 
               (Tk_IsMapped(setPtr->yScrollbar))) {
        Tk_UnmapWindow(setPtr->yScrollbar);
    }

    if (setPtr->xScrollbarHeight > 0) {
        int x, y;
        int xScrollbarWidth;

        x = 0;
        y = VPORTHEIGHT(setPtr);
        xScrollbarWidth = VPORTWIDTH(setPtr);
        if ((Tk_Width(setPtr->xScrollbar) != xScrollbarWidth) ||
            (Tk_Height(setPtr->xScrollbar) != setPtr->xScrollbarHeight) ||
            (x != Tk_X(setPtr->xScrollbar)) || 
            (y != Tk_Y(setPtr->xScrollbar))) {
            Tk_MoveResizeWindow(setPtr->xScrollbar, x, y, xScrollbarWidth,
                setPtr->xScrollbarHeight);
        }
        if (!Tk_IsMapped(setPtr->xScrollbar)) {
            Tk_MapWindow(setPtr->xScrollbar);
            XRaiseWindow(setPtr->display, Tk_WindowId(setPtr->xScrollbar));
        }
    } else if ((setPtr->xScrollbar != NULL) && 
               (Tk_IsMapped(setPtr->xScrollbar))) {
        Tk_UnmapWindow(setPtr->xScrollbar);
    }

    if ((setPtr->yScrollbarWidth > 0) && (setPtr->xScrollbarHeight > 0)) {
        int shangleX, shangleY;
        int shangleWidth, shangleHeight;

        shangleX = VPORTWIDTH(setPtr);
        shangleY = VPORTHEIGHT(setPtr);
        shangleWidth = setPtr->yScrollbarWidth;
        shangleHeight = setPtr->xScrollbarHeight;
        if ((shangleX != Tk_X(setPtr->shangle)) || 
            (shangleY != Tk_Y(setPtr->shangle)) ||
            (shangleWidth != Tk_Width(setPtr->shangle)) ||
            (shangleHeight != Tk_Height(setPtr->shangle))) {
            Tk_MoveResizeWindow(setPtr->shangle, shangleX, 
                shangleY, shangleWidth, shangleHeight);
        }
        if (!Tk_IsMapped(setPtr->shangle)) {
            Tk_MapWindow(setPtr->shangle);
            XRaiseWindow(setPtr->display, Tk_WindowId(setPtr->shangle));
        }
        Blt_Bg_FillRectangle(setPtr->shangle, 
                Tk_WindowId(setPtr->shangle), setPtr->bg, 
                0, 0, shangleWidth, shangleHeight, 0, TK_RELIEF_FLAT);

    } else {
        Tk_UnmapWindow(setPtr->shangle);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayScrollset --
 *
 *      This procedure is invoked to display a scrollset widget.
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
DisplayScrollset(ClientData clientData)
{
    Scrollset *setPtr = clientData;
    int reqWidth, reqHeight;

    setPtr->flags &= ~REDRAW_PENDING;
    if (setPtr->tkwin == NULL) {
        return;                         /* Window destroyed (should not get
                                         * here) */
    }
    /*
     * The requested size of the host container is always the requested
     * size of the slave widget without scrollbars. Only when the size of
     * the window is less than the requested size to we add scrollbars
     * (non-scrolling widgets).
     */
    reqWidth = (setPtr->reqWidth > 0) ? setPtr->reqWidth : 
        GetSlaveReqWidth(setPtr) + setPtr->yScrollbarWidth;
    reqHeight = (setPtr->reqHeight > 0) ? setPtr->reqHeight : 
        GetSlaveReqHeight(setPtr) + setPtr->xScrollbarHeight;

    if ((reqWidth != Tk_ReqWidth(setPtr->tkwin)) || 
        (reqHeight != Tk_ReqHeight(setPtr->tkwin))) {
        Tk_GeometryRequest(setPtr->tkwin, reqWidth, reqHeight);
    }

#ifdef notdef
    if ((Tk_Width(setPtr->tkwin) <= 1) || (Tk_Height(setPtr->tkwin) <= 1)){
        /* Don't bother computing the layout until the window size is
         * something reasonable. */
        return;
    }
#endif
    if (setPtr->flags & LAYOUT_PENDING) {
        ComputeSlaveGeometry(setPtr);
    }
    if (!Tk_IsMapped(setPtr->tkwin)) {
        /* The scrollset's window isn't displayed, so don't bother drawing
         * anything.  By getting this far, we've at least computed the
         * coordinates of the scrollset's new layout.  */
        return;
    }
    if (setPtr->flags & SCROLL_PENDING) {
        int w, h;
        /* 
         * The view port has changed. The scrollbars need to be updated.
         */
        w = VPORTWIDTH(setPtr);
        h = VPORTHEIGHT(setPtr);
        if ((setPtr->xScrollCmdObjPtr != NULL) && (setPtr->flags & SCROLLX)) {
            Blt_UpdateScrollbar(setPtr->interp, setPtr->xScrollCmdObjPtr,
                setPtr->xOffset, setPtr->xOffset + w, setPtr->worldWidth);
        }
        if ((setPtr->yScrollCmdObjPtr != NULL) && (setPtr->flags & SCROLLY)) {
            Blt_UpdateScrollbar(setPtr->interp, setPtr->yScrollCmdObjPtr,
                setPtr->yOffset, setPtr->yOffset + h, setPtr->worldHeight);
        }
        setPtr->flags &= ~SCROLL_PENDING;
    }
    if ((setPtr->slaveWidth < VPORTWIDTH(setPtr)) ||
        (setPtr->slaveHeight < VPORTHEIGHT(setPtr))) {
        Blt_Bg_FillRectangle(setPtr->tkwin, Tk_WindowId(setPtr->tkwin), 
                setPtr->bg, 0, 0, VPORTWIDTH(setPtr), VPORTHEIGHT(setPtr), 
                0, TK_RELIEF_FLAT);
    }
    DrawScrollset(setPtr);
}

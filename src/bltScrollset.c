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
 *   SUBSTITUTE GOOD OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
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

#ifdef HAVE_STRING_H
  #include <string.h>
#endif  /* HAVE_STRING_H */

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

#define DEBUG                   0
#define VPORTWIDTH(s)   (Tk_Width((s)->tkwin) - (s)->yScrollbarWidth)
#define VPORTHEIGHT(s)  (Tk_Height((s)->tkwin) - (s)->xScrollbarHeight)
#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))

#define REDRAW_PENDING          (1<<0)  /* A redraw request is pending.  */
#define LAYOUT_PENDING          (1<<1)  /* A request to recompute the
                                         * layout of the scrollbars and the
                                         * embedded widget is pending.  */
#define UPDATE_PENDING          (1<<2)  /* A request to call the widget
                                         * update procedure because either
                                         * the scrollbars or embedded
                                         * widget has been configured. */
#define GEOMETRY                (1<<3)  /* The scrollset needs to have its
                                         * geometry computed. */
#define X_SCROLL                (1<<4)  /* A request to set the X scrollbar
                                         * is pending. */
#define Y_SCROLL                (1<<5)  /* A request to set the Y scrollbar
                                         * is pending. */
#define SCROLL_PENDING          (X_SCROLL|Y_SCROLL)

#define X_INSTALL_PENDING       (1<<6)  /* A request to install a new
                                         * x-scrollbar widget is pending. */
#define Y_INSTALL_PENDING       (1<<7)  /* A request to install a new
                                         * y-scrollbar widget is pending. */
#define WARD_INSTALL_PENDING    (1<<8)  /* A request to install a new
                                         * embedded widget is pending.  */
#define X_DISPLAY               (1<<9)  /* Display the x-scrollbar. */
#define Y_DISPLAY               (1<<10) /* Display the y-scrollbar. */
#define WARD_XVIEW              (1<<11) /* The embedded widget has a
                                         * "xview" operation */
#define WARD_YVIEW              (1<<12) /* The embedded widget has a
                                         * "yview" operation. */
#define X_STATIC                (1<<13) /* The x-scrollbar should always be
                                         * displayed. */
#define Y_STATIC                (1<<14) /* The y-scrollbar should always be
                                         * displayed. */

#define DEF_ANCHOR              "center"
#define DEF_BACKGROUND          STD_NORMAL_BACKGROUND
#define DEF_CURSOR              ((char *)NULL)
#define DEF_FILL                "both"
#define DEF_HEIGHT              "0"
#define DEF_IPADX               "0"
#define DEF_IPADY               "0"
#define DEF_WIDTH               "0"
#define DEF_WINDOW              ((char *)NULL)
#define DEF_XMODE               "auto"
#define DEF_XSCROLLBAR          ((char *)NULL)
#define DEF_XSCROLLCOMMAND      ((char *)NULL)
#define DEF_XSCROLLINCREMENT    "2"
#define DEF_XVIEWCOMMAND        ((char *)NULL)
#define DEF_YMODE               "auto"
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
    unsigned int flags;
    Tcl_Interp *interp;                 /* Interpreter associated with
                                         * widget.  Used to delete widget
                                         * command. */
    Display *display;                   /* Display containing widget.
                                         * Used, among other things, so
                                         * that resources can * be freed
                                         * even after tkwin has gone
                                         * away. */
    Tk_Window tkwin;                    /* Window that embodies the frame.
                                         * NULL means that the window has
                                         * been destroyed but the data
                                         * structures haven't yet been
                                         * cleaned up. */
    Tcl_Command cmdToken;               /* Token for widget's command. */
    Tcl_Obj *winObjPtr;                 /* Name of the widget to be embed
                                         * into the scrollset window:
                                         * -window option.*/
    Tk_Window ward;                     /* Embedded window to be managed by
                                         * this widget. */
    Tk_Window shangle;
    Limits reqWardWidth, reqWardHeight; /* Requested sizes for embedded
                                         * widget. */
    int reqWidth, reqHeight;
    Tk_Anchor anchor;                   /* Anchor type: indicates how the
                                         * embedded widget is positioned if
                                         * extra space is available in the
                                         * scrollset. */
    Blt_Bg bg;
    int iPadX, iPadY;                   /* Extra padding added to the
                                         * interior of the widget
                                         * (i.e. adds to the requested size
                                         * of the widget) */
    int fill;                           /* Indicates how the embedded
                                         * widget should fill the extra
                                         * space of the scrollset. */
    int wardX, wardY;                   /* Origin of embedded widget wrt
                                         * container. */
    int wardWidth, wardHeight;          /* Dimension of embedded widget. */
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

    /* Commands to control embedded widget's horizontal and vertical
     * views. */
    Tcl_Obj *xViewCmdObjPtr, *yViewCmdObjPtr;

    int xOffset, yOffset;               /* Scroll offsets of viewport in
                                         * world. */ 
    int worldWidth, worldHeight;        /* Dimension of entire scrolled
                                         * widget. This is used only for
                                         * widget that don't have native
                                         * scrolling capabilities. */
    int cavityWidth, cavityHeight;      /* Dimension of entire menu. */

    Tk_Window xScrollbar;               /* Horizontal scrollbar to be used
                                         * if necessary. If NULL, no
                                         * x-scrollbar is used. */
    Tk_Window yScrollbar;               /* Vertical scrollbar to be used if
                                         * necessary. If NULL, no
                                         * y-scrollbar is used. */

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
static Blt_CustomOption limitsOption = {
    ObjToLimits, LimitsToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToMode;
static Blt_OptionPrintProc ModeToObj;
static Blt_CustomOption xModeOption = {
    ObjToMode, ModeToObj, NULL, (ClientData)X_STATIC
};
static Blt_CustomOption yModeOption = {
    ObjToMode, ModeToObj, NULL, (ClientData)Y_STATIC
};

static Blt_ConfigSpec scrollsetSpecs[] =
{
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",  DEF_ANCHOR, 
        Blt_Offset(Scrollset, anchor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BACKGROUND, Blt_Offset(Scrollset, bg), 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background"},
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
    {BLT_CONFIG_CUSTOM, "-reqheight", "reqHeight", "ReqHeight", (char *)NULL, 
        Blt_Offset(Scrollset, reqWardHeight), 0, &limitsOption},
    {BLT_CONFIG_CUSTOM, "-reqwidth", "reqWidth", "ReqWidth", (char *)NULL, 
        Blt_Offset(Scrollset, reqWardWidth), 0, &limitsOption},
    {BLT_CONFIG_CUSTOM, "-xmode", "xMode", "Mode", DEF_XMODE,
        Blt_Offset(Scrollset, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        &xModeOption},
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
    {BLT_CONFIG_CUSTOM, "-ymode", "xMode", "Mode", DEF_YMODE,
        Blt_Offset(Scrollset, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        &yModeOption},
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
        Blt_Offset(Scrollset, winObjPtr), 
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

static Tcl_IdleProc DisplayProc;
static Tcl_FreeProc DestroyProc;
static Tk_EventProc WindowEventProc;
static Tk_EventProc ScrollsetEventProc;
static Tcl_ObjCmdProc ScrollsetInstCmdProc;
static Tcl_CmdDeleteProc ScrollsetInstCmdDeletedProc;

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
        Tcl_DoWhenIdle(DisplayProc, setPtr);
        setPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjMode --
 *
 *      Converts the obj to the a scrollbar mode. The possible values
 *      are "auto" or "static".
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToMode(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    uintptr_t bitMask = (uintptr_t)clientData;
    const char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'a') && (strncmp(string, "auto", length) == 0)) {
        *flagsPtr &= ~bitMask;
    } else if ((c == 's') && (strncmp(string, "static", length) == 0)) {
        *flagsPtr |= bitMask;
    } else {
        Tcl_AppendResult(interp, "unknown mode value \"", string,
                         "\": should be auto or static.", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ModeToObj --
 *
 *      Convert the mode to its string representation.
 *
 * Results:
 *      The string representation of the mode is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ModeToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            char *widgRec, int offset, int flags)  
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    uintptr_t bitMask = (uintptr_t)clientData;
    const char *string;

    string = (*flagsPtr & bitMask) ?  "static" : "auto" ;
    return Tcl_NewStringObj(string, -1);
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
 * GetWardReqWidth --
 *
 *      Returns the width requested by the embedded widget.  The requested
 *      space also includes any internal padding which has been designated
 *      for this widget.
 *
 * Results:
 *      Returns the requested width of the widget.
 *
 *---------------------------------------------------------------------------
 */
static INLINE int
GetWardReqWidth(Scrollset *setPtr)
{
    int width;

    width = 2 * setPtr->iPadX;
    if (setPtr->ward != NULL) {
        width += Tk_ReqWidth(setPtr->ward);
    }
    width = GetBoundedWidth(width, &setPtr->reqWardWidth);
    return width;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetWardReqHeight --
 *
 *      Returns the height requested by the embedded widget.  The requested
 *      space also includes any internal padding which has been designated
 *      for this widget.
 *
 *      The requested height of the widget is always bounded by the limits set
 *      in setPtr->reqWardHeight.
 *
 * Results:
 *      Returns the requested height of the widget.
 *
 *---------------------------------------------------------------------------
 */
static INLINE int
GetWardReqHeight(Scrollset *setPtr)
{
    int height;

    height = 2 * setPtr->iPadY;
    if (setPtr->ward != NULL) {
        height += Tk_ReqHeight(setPtr->ward);
    }
    height = GetBoundedHeight(height, &setPtr->reqWardHeight);
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
                         "\" must be a child of scrollset.", (char *)NULL);
        return TCL_ERROR;
    }
    ManageWindow(setPtr, tkwin);
    *tkwinPtr = tkwin;
    setPtr->flags |= GEOMETRY;
    return TCL_OK;
}

static void
InstallXScrollbarProc(ClientData clientData)
{
    Scrollset *setPtr = clientData;
    Tcl_Interp *interp;

    interp = setPtr->interp;
    setPtr->flags &= ~X_INSTALL_PENDING;
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
InstallYScrollbarProc(ClientData clientData)
{
    Scrollset *setPtr = clientData;
    Tcl_Interp *interp;

    interp = setPtr->interp;
    setPtr->flags &= ~Y_INSTALL_PENDING;
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

/*
 *---------------------------------------------------------------------------
 *
 * InstallWardProc --
 *
 *      Idle callback to install the designated embedded widget in the
 *      scrollset widget.  Part of the installation is to try and run
 *      the "xview" and "yview" operations of the embedded widget to see
 *      if they exist.  This tells us if the embedded widget natively
 *      handles scrolling operations or if we have to move its window
 *      to simulate scrolling.
 *
 *      This is done in an idle event to eliminate the chicken-and-the-egg
 *      problem where the embedded widget must be child of the scrollset
 *      widget, but you want to specify the -window option when you create
 *      the scrollset, not in a separate command afterwards.
 *
 *      Deferring the installation requires some changes to way embedded
 *      windows are handled.  Normally, Tk_GeometryRequest is called from
 *      the configure routine. Because we get the requested size from the
 *      embedded widget, we have to wait until after this routine is called
 *      to set the requested size of the scrollset.
 *
 *      Further, the scrolling region as set by the "xset" or "yset"
 *      operations may not be correct.  Scrollbars do not report the
 *      correct first and last values when the window is only 1x1.  We have
 *      to ignore these calls until the window is made some reasonable
 *      size.
 *
 * Results:
 *      The return value is a standard TCL result.  The window pointer is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
static void
InstallWardProc(ClientData clientData)
{
    Scrollset *setPtr = clientData;
    Tcl_Interp *interp;
    Tcl_Obj *cmdObjPtr;
    int result;

    interp = setPtr->interp;
    setPtr->flags &= ~(WARD_INSTALL_PENDING | WARD_XVIEW | WARD_YVIEW);
    if (setPtr->tkwin == NULL) {
        return;                         /* Widget has been destroyed. */
    }
    if (InstallWindow(interp, setPtr, setPtr->winObjPtr, &setPtr->ward) 
        != TCL_OK) {
        Tcl_BackgroundError(interp);
        return;
    }
    /* Check if the embedded widget has a "yview" operation. */
    if (setPtr->yViewCmdObjPtr != NULL) {
        cmdObjPtr = Tcl_DuplicateObj(setPtr->yViewCmdObjPtr);
    } else {
        cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, setPtr->winObjPtr);
        Tcl_ListObjAppendElement(interp, cmdObjPtr,Tcl_NewStringObj("yview",5));
    }
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    Tcl_ResetResult(interp);
    if (result == TCL_OK) {
        setPtr->flags |= WARD_YVIEW;
    }

    /* Check if the embedded widget has a "xview" operation. */
    if (setPtr->xViewCmdObjPtr != NULL) {
        cmdObjPtr = Tcl_DuplicateObj(setPtr->xViewCmdObjPtr);
    } else {
        cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, setPtr->winObjPtr);
        Tcl_ListObjAppendElement(interp, cmdObjPtr,Tcl_NewStringObj("xview",5));
    }
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    Tcl_ResetResult(interp);
    if (result == TCL_OK) {
        setPtr->flags |= WARD_XVIEW;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureScrollbarsProc --
 *
 *      Idle callback to run a pre-defined TCL procedure to configure the
 *      scrollbars that have been installed.  This procedure sets the
 *      (-orient) orientation of the scrollbars and configures the
 *      scrolling command (-command) for the scrollbars.
 *
 *      This procedure gets called any time a scrollbar is installed.
 *
 * Results:
 *      None.
 *
 * Side Effects: 
 *      The scrollset widget and its scrollbars are configured for
 *      scrolling.
 *
 *---------------------------------------------------------------------------
 */
static void
ConfigureScrollbarsProc(ClientData clientData)
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

/*
 *---------------------------------------------------------------------------
 *
 * ComputeGeometry --
 *      
 *      Calls Tk_GeometryRequest to set the requested size of the scrollset
 *      widget.  This is usually the requested size of the embedded widget, 
 *      but can be overridden by the -reqwidth and -reqheight options.
 *
 *      Normally this would be called from the configure routine.  But
 *      since the actual installation of embedded window is deferred, this
 *      gets called from the DisplayProc routine when the widget flag
 *      GEOMETRY is set.
 *
 * Results:
 *      None.
 *
 * Side Effects: 
 *      The requested size of the scrollset widget is set.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeGeometry(Scrollset *setPtr)
{
    int wardWidth, wardHeight;
    int w, h;
    
    wardWidth = GetWardReqWidth(setPtr);
    wardHeight = GetWardReqHeight(setPtr);
    w = wardWidth;
    h = wardHeight;

    /* Override the computed requested size of the scrollset window if
     * the user has specified a size. */
    if (setPtr->reqWidth > 0) {
        w = setPtr->reqWidth;
    } else {
        if ((setPtr->yScrollbar != NULL) &&
            ((setPtr->flags & Y_STATIC) || (h > setPtr->reqHeight))) {
            w += setPtr->yScrollbarWidth;
        }
    }
    if (setPtr->reqHeight > 0) {
        h = setPtr->reqHeight;
    } else {
        if ((setPtr->xScrollbar != NULL) &&
            ((setPtr->flags & X_STATIC) || (w > setPtr->reqWidth))) {
            h += setPtr->xScrollbarHeight;
        }
    }
    setPtr->worldWidth = setPtr->worldHeight = 0;
    if ((setPtr->flags & WARD_XVIEW) == 0) {
        /* Embedded window doesn't have a xview operation. */
        setPtr->worldWidth = wardWidth;
    } 
    if ((setPtr->flags & WARD_YVIEW) == 0) { 
        /* Embedded window doesn't have a yview operation. */
        setPtr->worldHeight = wardHeight;
    }
    if ((w != Tk_ReqWidth(setPtr->tkwin)) || 
        (h != Tk_ReqHeight(setPtr->tkwin))) {
        Tk_GeometryRequest(setPtr->tkwin, w, h);
    } 
    setPtr->flags &= ~GEOMETRY;
    setPtr->flags |= LAYOUT_PENDING;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArrangeWindows --
 *      
 *      Does the work of arrange the child windows in the scrollset.  It
 *      determines if scrollbars are needed (based upon the sizes of the
 *      scrollset and embedded widgets). Up to four windows are arranged:
 *      the embedded widget, two scrollbars, and a shangle.  The shangle
 *      is a small window to cover the corner where the two scrollbars
 *      meet.
 *
 *      This routine gets called when the size of the scrollset of any of
 *      its child windows changes.
 *
 * Results:
 *      None.
 *
 * Side Effects: 
 *      The child windows are moved, resized, and mapped.
 *
 *---------------------------------------------------------------------------
 */
static void
ArrangeWindows(Scrollset *setPtr)
{ 
    int viewWidth, viewHeight;
    int dx, dy;
    int wardWidth, wardHeight;
    int x, y;
    
    viewWidth  = Tk_Width(setPtr->tkwin);
    viewHeight = Tk_Height(setPtr->tkwin);
    x = 0;
    y = 0;

    wardWidth = GetWardReqWidth(setPtr);
    wardHeight = GetWardReqHeight(setPtr);

    /* For non-native scrolling widgets, reset to no scrollbars. */
    if ((setPtr->flags & WARD_XVIEW) == 0) {
        setPtr->flags &= ~X_DISPLAY;
        setPtr->xScrollbarHeight = 0;
        setPtr->worldWidth = wardWidth;
    }
    if ((setPtr->flags & WARD_YVIEW) == 0) {
        setPtr->flags &= ~Y_DISPLAY;
        setPtr->yScrollbarWidth = 0;
        setPtr->worldHeight = wardHeight;
    }
    /* Step 1. If scrollbars are static, set them now. */
    if ((setPtr->xScrollbar != NULL) && (setPtr->flags & (X_DISPLAY|X_STATIC))){
        viewHeight -= setPtr->xScrollbarHeight;
        wardWidth = viewWidth;
        setPtr->xScrollbarHeight = Tk_ReqHeight(setPtr->xScrollbar);
        setPtr->flags |= X_DISPLAY;
    }
    if ((setPtr->yScrollbar != NULL) && (setPtr->flags & (Y_DISPLAY|Y_STATIC))){
        viewWidth -= setPtr->yScrollbarWidth;
        wardHeight = viewHeight;
        setPtr->yScrollbarWidth = Tk_ReqWidth(setPtr->yScrollbar);
        setPtr->flags |= Y_DISPLAY;
    }

    /* Step 2: For non-native scrolling widgets, compare the requested
     *         size of the embedded window versus the viewport size. */
    if ((setPtr->xScrollbar != NULL) &&
        ((setPtr->flags & (WARD_XVIEW|X_DISPLAY)) == 0) &&
        (viewWidth < wardWidth)) {
        /* Reduce the viewport height by the height of the x-scrollbar. */
        viewHeight -= setPtr->xScrollbarHeight;
        wardWidth = viewWidth;
        setPtr->xScrollbarHeight = Tk_ReqHeight(setPtr->xScrollbar);
        setPtr->flags |= X_DISPLAY;
    } 
    if ((setPtr->yScrollbar != NULL) &&
        ((setPtr->flags & (WARD_YVIEW|Y_DISPLAY)) == 0) &&
        (viewHeight < wardHeight)) {
        /* Reduce the viewport width by the width of the y-scrollbar. */
        viewWidth -= setPtr->yScrollbarWidth;
        wardHeight = viewHeight;
        setPtr->yScrollbarWidth = Tk_ReqWidth(setPtr->yScrollbar);
        setPtr->flags |= Y_DISPLAY;
    }

    /* Step 3: Did the addition of a scrollbar affect the size in the other
     *         dimension? Limit this to non-native scrolling widgets. We'll
     *         let the "set" callback tell us if a scrollbar is needed for
     *         native widgets. */
    if ((setPtr->xScrollbar != NULL) &&
        ((setPtr->flags & (WARD_XVIEW|X_DISPLAY)) == 0) &&
        (viewWidth < wardWidth)) {
        viewHeight -= setPtr->xScrollbarHeight;
        wardWidth = viewWidth;
        setPtr->xScrollbarHeight = Tk_ReqHeight(setPtr->xScrollbar);
        setPtr->flags |= X_DISPLAY;
    }
    if ((setPtr->yScrollbar != NULL) &&
        ((setPtr->flags & (WARD_YVIEW|Y_DISPLAY)) == 0) &&
        (viewHeight < wardHeight)) {
        viewWidth -= setPtr->yScrollbarWidth;
        wardHeight = viewHeight;
        setPtr->yScrollbarWidth = Tk_ReqWidth(setPtr->yScrollbar);
        setPtr->flags |= Y_DISPLAY;
    }

    /* Step 4: If the embedded widget is smaller than the viewport, adjust
     *         the size of the ward to fill the viewport. */
    if (viewWidth > wardWidth) {
        if (setPtr->fill & FILL_X) {
            wardWidth = viewWidth;
        } 
        setPtr->xOffset = 0;
    } else if (viewWidth < wardWidth) {
        if (setPtr->flags & WARD_XVIEW) {
            /* Only native x-scrolling widgets. */
            wardWidth = viewWidth; 
        }
    }
    if (viewHeight > wardHeight) {
        if (setPtr->fill & FILL_Y) {
            wardHeight = viewHeight;
        }
        setPtr->yOffset = 0;
    } else if (viewHeight < wardHeight) {
        if (setPtr->flags & WARD_YVIEW) {
            /* Only native y-scrolling widgets. */
            wardHeight = viewHeight;
        }
    }
    x = y = 0;
    dx = viewWidth - wardWidth;
    dy = viewHeight - wardHeight;
    if ((dx > 0) || (dy > 0)) {
        TranslateAnchor(dx, dy, setPtr->anchor, &x, &y);
    }
    setPtr->shangleHeight = setPtr->xScrollbarHeight;
    setPtr->shangleWidth  = setPtr->yScrollbarWidth;
    setPtr->yScrollbarHeight = viewHeight - setPtr->xScrollbarHeight;
    setPtr->xScrollbarWidth  = viewWidth  - setPtr->yScrollbarWidth;
    
    if (setPtr->ward != NULL) {
        if ((setPtr->xScrollbar == NULL) && (wardWidth > viewWidth)) {
            wardWidth = viewWidth;
            if (wardWidth < setPtr->reqWardWidth.min) {
                wardWidth = setPtr->reqWardWidth.min;
            } 
        }
        if ((setPtr->yScrollbar == NULL) && (wardHeight > viewHeight)) {
            wardHeight = viewHeight;
            if (wardHeight < setPtr->reqWardHeight.min) {
                wardHeight = setPtr->reqWardHeight.min;
            } 
        }
        if (viewWidth > wardWidth) {
            x += Tk_Changes(setPtr->ward)->border_width;
        } else {
            x = Tk_Changes(setPtr->ward)->border_width;
        }
        if (viewHeight > wardHeight) {
            y += Tk_Changes(setPtr->ward)->border_width;
        } else { 
            y = Tk_Changes(setPtr->ward)->border_width;
        }
    }
    setPtr->wardX = x;
    setPtr->wardY = y;
    
    /* For non-scrolling widgets, adjust the scroll offsets to put as much
     * of the ward widget in view as possible. */
    if ((setPtr->flags & WARD_YVIEW) == 0) {
        wardHeight = setPtr->worldHeight;
        setPtr->flags |= Y_SCROLL;
    }
    if ((setPtr->flags & WARD_XVIEW) == 0) {
        wardWidth = setPtr->worldWidth;
        setPtr->flags |= X_SCROLL;
    }
    /*
     * If the widget is too small (i.e. it has only an external border)
     * then unmap it.
     */
    if ((wardWidth < 1) || (wardHeight < 1)) {
        if (Tk_IsMapped(setPtr->ward)) {
            if (setPtr->tkwin != Tk_Parent(setPtr->ward)) {
                Tk_UnmaintainGeometry(setPtr->ward, setPtr->tkwin);
            }
            Tk_UnmapWindow(setPtr->ward);
        }
    } else {
        
        if (setPtr->xOffset > 0) {
            x -= setPtr->xOffset;
        }
        if (setPtr->yOffset > 0) {
            y -= setPtr->yOffset;
        }
        if (setPtr->tkwin != Tk_Parent(setPtr->ward)) {
            Tk_MaintainGeometry(setPtr->ward, setPtr->tkwin, x, y,
                                wardWidth, wardHeight);
        } else {
#ifdef notdef
            fprintf(stderr, "x=%d,y=%d, wardX=%d wardY=%d wardWidth=%d wardHeight=%d, vw=%d vh=%d\n",
                    x, y, Tk_X(setPtr->ward), Tk_Y(setPtr->ward),
                    wardWidth, wardHeight, viewWidth, viewHeight);
#endif
            if ((x != Tk_X(setPtr->ward)) || (y != Tk_Y(setPtr->ward)) ||
                (wardWidth != Tk_Width(setPtr->ward)) ||
                (wardHeight != Tk_Height(setPtr->ward))) {
                
                Tk_MoveResizeWindow(setPtr->ward, x, y, 
                                    wardWidth, wardHeight);
                setPtr->flags |= SCROLL_PENDING;
            }
            if (!Tk_IsMapped(setPtr->ward)) {
                Tk_MapWindow(setPtr->ward);
            }
        }
    }
    /* Manage the geometry of the scrollbars. */
    
    if (setPtr->flags & Y_DISPLAY) {
        int x, y;
        int yScrollbarHeight;
        
        x = VPORTWIDTH(setPtr);
        y = 0;
        yScrollbarHeight = VPORTHEIGHT(setPtr);
#ifdef notdef
            fprintf(stderr, "Yscrollbar x=%d,y=%d, ysX=%d ysY=%d ysWidth=%d cw=%d ysHeight=%d, ch=%d\n",
                    x, y, Tk_X(setPtr->yScrollbar), Tk_Y(setPtr->yScrollbar),
                    setPtr->yScrollbarWidth, Tk_Width(setPtr->yScrollbar),
                    yScrollbarHeight, Tk_Height(setPtr->yScrollbar));
#endif

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
    
    if (setPtr->flags & X_DISPLAY) {
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
        
        shangleX = VPORTWIDTH(setPtr);
        shangleY = VPORTHEIGHT(setPtr);
        if ((shangleX != Tk_X(setPtr->shangle)) || 
            (shangleY != Tk_Y(setPtr->shangle)) ||
            (setPtr->shangleWidth != Tk_Width(setPtr->shangle)) ||
            (setPtr->shangleHeight != Tk_Height(setPtr->shangle))) {
            Tk_MoveResizeWindow(setPtr->shangle, shangleX, shangleY,
                   setPtr->shangleWidth, setPtr->shangleHeight);
        }
        if (!Tk_IsMapped(setPtr->shangle)) {
            Tk_MapWindow(setPtr->shangle);
            XRaiseWindow(setPtr->display, Tk_WindowId(setPtr->shangle));
        }
    } else {
        Tk_UnmapWindow(setPtr->shangle);
    }
    setPtr->flags &= ~LAYOUT_PENDING;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureScrollset --
 *      
 * Results:
 *      A standard TCL result.
 *
 * Side Effects: 
 *      The child windows are moved, resized, and mapped.
 *
 *---------------------------------------------------------------------------
 */
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
     * Install the scrollbars and ward widget at a later time after the
     * scrollset window has been created.  We defer installing the ward and
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
        if ((setPtr->flags & X_INSTALL_PENDING) == 0) {
            Tcl_DoWhenIdle(InstallXScrollbarProc, setPtr);
            setPtr->flags |= X_INSTALL_PENDING;
        }           
        updateNeeded = TRUE;
    }
    if (Blt_ConfigModified(scrollsetSpecs, "-yscrollbar", (char *)NULL)) {
        if (setPtr->yScrollbar != NULL) {
            /* Immediately unmanage the current scrollbar. */
            UnmanageWindow(setPtr, setPtr->yScrollbar);
            setPtr->yScrollbar = NULL;
        }
        if ((setPtr->flags & Y_INSTALL_PENDING) == 0) {
            Tcl_DoWhenIdle(InstallYScrollbarProc, setPtr);
            setPtr->flags |= Y_INSTALL_PENDING;
        }           
        updateNeeded = TRUE;
    }
    if (Blt_ConfigModified(scrollsetSpecs, "-window", (char *)NULL)) {
        if (setPtr->ward != NULL) {
            UnmanageWindow(setPtr, setPtr->ward);
            setPtr->ward = NULL;
        }
        if ((setPtr->flags & WARD_INSTALL_PENDING) == 0) {
            Tcl_DoWhenIdle(InstallWardProc, setPtr);
            setPtr->flags |= WARD_INSTALL_PENDING;
        }           
        updateNeeded = TRUE;
    }
    if (updateNeeded) {
        if ((setPtr->flags & UPDATE_PENDING) == 0) {
            Tcl_DoWhenIdle(ConfigureScrollbarsProc, setPtr);
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
 *      scrollset widgets.
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
        setPtr->flags |= GEOMETRY;
        EventuallyRedraw(setPtr);
    } else if (eventPtr->type == DestroyNotify) {
        if (setPtr->tkwin != NULL) {
            setPtr->tkwin = NULL; 
        }
        if (setPtr->flags & REDRAW_PENDING) {
            Tcl_CancelIdleCall(DisplayProc, setPtr);
        }
        if (setPtr->flags & X_INSTALL_PENDING) {
            Tcl_CancelIdleCall(InstallXScrollbarProc, setPtr);
        }
        if (setPtr->flags & Y_INSTALL_PENDING) {
            Tcl_CancelIdleCall(InstallYScrollbarProc, setPtr);
        }
        if (setPtr->flags & WARD_INSTALL_PENDING) {
            Tcl_CancelIdleCall(InstallWardProc, setPtr);
        }           
        if (setPtr->flags & UPDATE_PENDING) {
            Tcl_CancelIdleCall(ConfigureScrollbarsProc, setPtr);
        }           
        Tcl_EventuallyFree(setPtr, DestroyProc);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowEventProc --
 *
 *      This procedure is invoked by the Tk event handler when
 *      StructureNotify events occur in a scrollbar or widget managed by
 *      the scrollset widget.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
WindowEventProc(
    ClientData clientData,              /* Pointer to Scrollset structure
                                         * for widget referred to by
                                         * eventPtr. */
    XEvent *eventPtr)                   /* Describes what just happened. */
{
    Scrollset *setPtr = clientData;

    if (eventPtr->type == Expose) {
        if (eventPtr->xexpose.count == 0) {
            EventuallyRedraw(setPtr);
        }
    } else if (eventPtr->type == ConfigureNotify) {
        setPtr->flags |= GEOMETRY;
        EventuallyRedraw(setPtr);
    } else if (eventPtr->type == DestroyNotify) {
        if ((setPtr->yScrollbar != NULL) && 
            (eventPtr->xany.window == Tk_WindowId(setPtr->yScrollbar))) {
            setPtr->yScrollbar = NULL;
        } else if ((setPtr->xScrollbar != NULL) && 
                   (eventPtr->xany.window == Tk_WindowId(setPtr->xScrollbar))) {
            setPtr->xScrollbar = NULL;
        } else if ((setPtr->ward != NULL) && 
                   (eventPtr->xany.window == Tk_WindowId(setPtr->ward))) {
            setPtr->ward = NULL;
        } else if ((setPtr->shangle != NULL) && 
                   (eventPtr->xany.window == Tk_WindowId(setPtr->shangle))) {
            setPtr->shangle = NULL;
        } 
        setPtr->flags |= GEOMETRY;
        EventuallyRedraw(setPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollsetCustodyProc --
 *
 *      This procedure is invoked when a scrollbar or embedded widget has
 *      been stolen by another geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the scrollset to have its layout re-arranged at the
 *      next idle point.
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
        setPtr->flags &= ~Y_DISPLAY;
    } else if (tkwin == setPtr->xScrollbar) {
        setPtr->xScrollbar = NULL;
        setPtr->xScrollbarHeight = 0;
        setPtr->flags &= ~X_DISPLAY;
    } else if (tkwin == setPtr->ward) {
        setPtr->ward = NULL;
        setPtr->wardWidth = setPtr->wardHeight = 0;
        setPtr->flags &= ~(X_DISPLAY|Y_DISPLAY);
    }
    Tk_UnmaintainGeometry(tkwin, setPtr->tkwin);
    setPtr->flags |= GEOMETRY;
    EventuallyRedraw(setPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ScrollsetGeometryProc --
 *
 *      This procedure is invoked by Tk_GeometryRequest for scrollbars
 *      managed by the scrollset.
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

    setPtr->flags |= GEOMETRY;
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
 *      pathName configure ?option value?...
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
    setPtr->flags |= GEOMETRY;
    EventuallyRedraw(setPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *      Returns the value of the named widget option.
 *
 * Results:
 *      Standard TCL result.
 *
 *      pathName cget option
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
 *      This command is only for embedded widgets that have scrolling
 *      capabilities: ie. the ward will issue "set" commands to what it
 *      thinks is its scrollbar. This procedure acts as a relay the "set"
 *      operation from the ward widget to scrollbar.  This routine checks
 *      to see if the first/last values are 0 and 1 respectively,
 *      indicating no scrollbar is necessary.
 *
 *      Because the Tk_GeometryRequest call for the scrollset is deferred
 *      until the embedded widget has been created, it is possible for a
 *      scrollbar to call this routine when the scrollset is still 1x1.
 *      The first and last numbers will be bogus.  
 *
 *      pathName xset first last 
 *      pathName yset first last 
 *
 *---------------------------------------------------------------------------
 */
static int
SetOp(Scrollset *setPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    const char *string;
    const char *scrollbar;
    double first, last;
    
    scrollbar = NULL;
    string = Tcl_GetString(objv[1]);

    /* Examine the set values from the ward. */
    if (Tcl_GetDoubleFromObj(interp, objv[2], &first) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[3], &last) != TCL_OK) {
        return TCL_ERROR;
    }
#ifdef notdef
    if ((Tk_Width(setPtr->tkwin) <= 1) || (Tk_Height(setPtr->tkwin) <= 1)) {
        /* The first and last values are not reliable when the size of the
         * viewport is 1x1. */
        return TCL_OK;
    }
#endif
    first = FCLAMP(first);
    last = FCLAMP(last);
#ifdef notdef
    fprintf(stderr, "SetOp: %s first=%g last=%g\n", 
            Tk_PathName(setPtr->tkwin), first, last);
#endif
    if (string[0] == 'x') {
        if (setPtr->flags & WARD_XVIEW) {
            if (setPtr->xScrollbar != NULL) {
                scrollbar = Tk_PathName(setPtr->xScrollbar);
            }
            /* Peek at the last and first values to see if we need a
             * scrollbar. */
            if ((first <= 0.0) && (last >= 1.0)) {
                setPtr->flags &= ~X_DISPLAY; /* Hide scrollbar. */
                setPtr->xScrollbarHeight = 0;
            } else {
                setPtr->flags |= X_DISPLAY; /* Display x-scrollbar. */
                setPtr->xScrollbarHeight = Tk_ReqHeight(setPtr->xScrollbar);
            }            
            setPtr->flags |= GEOMETRY;
            EventuallyRedraw(setPtr);
        }
    } else if (string[0] == 'y') {
        if (setPtr->flags & WARD_YVIEW) {
            if (setPtr->yScrollbar != NULL) {
                scrollbar = Tk_PathName(setPtr->yScrollbar);
            }
            if ((first <= 0.0) && (last >= 1.0)) {
                setPtr->flags &= ~Y_DISPLAY;     /* Hide scrollbar. */
                setPtr->yScrollbarWidth = 0;
            } else {
                setPtr->flags |= Y_DISPLAY; /* Display x-scrollbar. */
                setPtr->yScrollbarWidth = Tk_ReqWidth(setPtr->yScrollbar);
            }
            setPtr->flags |= GEOMETRY;
            EventuallyRedraw(setPtr);
        }
    } else {
        Tcl_AppendResult(interp, 
                "bad scrollset option: should be xset or yset", (char *)NULL);
        return TCL_ERROR;
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

    if (setPtr->flags & WARD_XVIEW) {
        Tcl_Obj *cmdObjPtr;
        int i;
        int result;

        /* The ward widget has a "xview" operation.  Simply relay the
         * information on to the ward widget by calling its "xview"
         * operation. */
        if (setPtr->xViewCmdObjPtr != NULL) {
            cmdObjPtr = Tcl_DuplicateObj(setPtr->xViewCmdObjPtr);
        } else {
            cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
            Tcl_ListObjAppendElement(interp, cmdObjPtr, setPtr->winObjPtr);
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
    setPtr->flags |= LAYOUT_PENDING | X_SCROLL;
    EventuallyRedraw(setPtr);
    return TCL_OK;
}


static int
YviewOp(Scrollset *setPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int viewHeight;

    if (setPtr->flags & WARD_YVIEW) {
        Tcl_Obj *cmdObjPtr;
        int i;
        int result;

        /* The ward widget has a "yview" operation.  Simply relay the
         * information on to the ward widget by calling its "yview"
         * operation. */
        if (setPtr->yViewCmdObjPtr != NULL) {
            cmdObjPtr = Tcl_DuplicateObj(setPtr->yViewCmdObjPtr);
        } else {
            cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
            Tcl_ListObjAppendElement(interp, cmdObjPtr, setPtr->winObjPtr);
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

    viewHeight = VPORTHEIGHT(setPtr);
    if (objc == 2) {
        double first, last;

        /*
         * Note: we are bounding the fractions between 0.0 and 1.0 to support
         * the "canvas"-style of scrolling.
         */
        first = (double)setPtr->yOffset / setPtr->worldHeight;
        last  = (double)(setPtr->yOffset + viewHeight) / setPtr->worldHeight;
        Tcl_AppendElement(interp, Blt_Dtoa(interp, FCLAMP(first)));
        Tcl_AppendElement(interp, Blt_Dtoa(interp, FCLAMP(last)));
        return TCL_OK;
    }
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &setPtr->yOffset,
        setPtr->worldHeight, viewHeight, setPtr->yScrollUnits, 
        BLT_SCROLL_MODE_HIERBOX) != TCL_OK) {
        return TCL_ERROR;
    }
    setPtr->flags |= LAYOUT_PENDING | Y_SCROLL;
    EventuallyRedraw(setPtr);
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
DestroyProc(DestroyData dataPtr)        /* Pointer to the widget
                                         * record. */
{
    Scrollset *setPtr = (Scrollset *)dataPtr;

    if (setPtr->flags & REDRAW_PENDING) {
        Tcl_CancelIdleCall(DisplayProc, setPtr);
    }
    if (setPtr->flags & X_INSTALL_PENDING) {
        Tcl_CancelIdleCall(InstallXScrollbarProc, setPtr);
    }
    if (setPtr->flags & Y_INSTALL_PENDING) {
        Tcl_CancelIdleCall(InstallYScrollbarProc, setPtr);
    }
    if (setPtr->flags & WARD_INSTALL_PENDING) {
        Tcl_CancelIdleCall(InstallWardProc, setPtr);
    }       
    if (setPtr->flags & UPDATE_PENDING) {
        Tcl_CancelIdleCall(ConfigureScrollbarsProc, setPtr);
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
    setPtr->anchor = TK_ANCHOR_CENTER;
    setPtr->display = Tk_Display(tkwin);
    setPtr->fill = FILL_BOTH;
    setPtr->flags |= (GEOMETRY | SCROLL_PENDING);
    setPtr->interp = interp;
    setPtr->tkwin = tkwin;
    setPtr->xScrollUnits = 2;
    setPtr->yScrollUnits = 2;
    ResetLimits(&setPtr->reqWardWidth);
    ResetLimits(&setPtr->reqWardHeight);
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
            Blt_FmtString(info, 200, "\n\t(while loading bindings for %.50s)", 
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
DrawShangle(Scrollset *setPtr)
{
    if (!Tk_IsMapped(setPtr->shangle)) {
        Tk_MapWindow(setPtr->shangle);
        XRaiseWindow(setPtr->display, Tk_WindowId(setPtr->shangle));
    }
    Blt_Bg_FillRectangle(setPtr->shangle, Tk_WindowId(setPtr->shangle),
        setPtr->bg, 0, 0, setPtr->shangleWidth, setPtr->shangleHeight,
        0, TK_RELIEF_FLAT);
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayProc --
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
DisplayProc(ClientData clientData)
{
    Scrollset *setPtr = clientData;

    setPtr->flags &= ~REDRAW_PENDING;
    if (setPtr->tkwin == NULL) {
        return;                         /* Window destroyed (should not get
                                         * here) */
    }
    if (setPtr->flags & GEOMETRY) {
        ComputeGeometry(setPtr);
    }
    if ((Tk_Width(setPtr->tkwin) <= 1) || (Tk_Height(setPtr->tkwin) <= 1)){
        /* Don't bother arranging the window and scrollbars until the
         * widget's size is something reasonable. */
        return;
    }
    if (setPtr->flags & LAYOUT_PENDING) {
        ArrangeWindows(setPtr);
    }
    if (!Tk_IsMapped(setPtr->tkwin)) {
        return;
    }
    if (setPtr->flags & SCROLL_PENDING) {
        int w, h;
        /* 
         * The view port has changed. The scrollbars need to be updated.
         */
        w = VPORTWIDTH(setPtr);
        h = VPORTHEIGHT(setPtr);
        if ((setPtr->xScrollCmdObjPtr != NULL) && (setPtr->flags & X_SCROLL)) {
            Blt_UpdateScrollbar(setPtr->interp, setPtr->xScrollCmdObjPtr,
                setPtr->xOffset, setPtr->xOffset + w, setPtr->worldWidth);
        }
        if ((setPtr->yScrollCmdObjPtr != NULL) && (setPtr->flags & Y_SCROLL)) {
            Blt_UpdateScrollbar(setPtr->interp, setPtr->yScrollCmdObjPtr,
                setPtr->yOffset, setPtr->yOffset + h, setPtr->worldHeight);
        }
        setPtr->flags &= ~SCROLL_PENDING;
    }
    if ((setPtr->wardWidth < VPORTWIDTH(setPtr)) ||
        (setPtr->wardHeight < VPORTHEIGHT(setPtr))) {
        /* Only need to draw the background of the scrollset window if the
         * window is than the embedded widget. */
        Blt_Bg_FillRectangle(setPtr->tkwin, Tk_WindowId(setPtr->tkwin), 
                setPtr->bg, 0, 0, VPORTWIDTH(setPtr), VPORTHEIGHT(setPtr), 
                0, TK_RELIEF_FLAT);
    }
    if ((setPtr->shangleWidth > 0) && (setPtr->shangleHeight > 0)) {
        DrawShangle(setPtr);
    }
}

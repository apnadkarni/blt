/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrHairs.c --
 *
 * This module implements crosshairs for the BLT graph widget.
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
#include "bltAlloc.h"
#include "bltHash.h"
#include "bltChain.h"
#include "bltOp.h"
#include "bltBind.h"
#include "bltPs.h"
#include "bltBg.h"
#include "bltPicture.h"
#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"

/*
 *---------------------------------------------------------------------------
 *
 * Crosshairs
 *
 *      Contains the line segments positions and graphics context used to
 *      simulate crosshairs by XORing a line on the graph.
 *
 *---------------------------------------------------------------------------
 */
struct _Crosshairs {
    unsigned int flags;                 /* HIDDEN and ACTIVE. */
    int x, y;                           /* Hot spot for crosshairs */
    int lineWidth;                      /* Width of the crosshair lines */
    Blt_Dashes dashes;                  /* Dash-style of the
                                         * crosshairs. This represents an
                                         * array of alternatingly drawn
                                         * pixel values. If NULL, the hairs
                                         * are drawn as a solid line */
    XSegment segArr[2];                 /* Positions of line segments
                                         * representing the crosshairs. */
    XColor *colorPtr;                   /* Foreground color of
                                         * crosshairs */
    GC gc;                              /* Graphics context for
                                         * crosshairs. Set to GXxor to not
                                         * require redraws of graph */
};

#define DEF_DASHES              (char *)NULL
#define DEF_FOREGROUND          RGB_BLACK
#define DEF_LINE_WIDTH          "0"
#define DEF_HIDE                "yes"
#define DEF_POSITION            (char *)NULL
#define DEF_X                   (char *)NULL
#define DEF_Y                   (char *)NULL

static Blt_OptionParseProc ObjToPosition;
static Blt_OptionPrintProc PositionToObj;
Blt_CustomOption positionOption =
{
    ObjToPosition, PositionToObj, NULL, (ClientData)0
};

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_COLOR, "-color", "color", "Color", DEF_FOREGROUND, 
        Blt_Offset(Crosshairs, colorPtr), 0},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes", DEF_DASHES, 
        Blt_Offset(Crosshairs, dashes), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_HIDE, 
       Blt_Offset(Crosshairs, flags), BLT_CONFIG_DONT_SET_DEFAULT,
       (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "Linewidth",
        DEF_LINE_WIDTH, Blt_Offset(Crosshairs, lineWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-position", "position", "Position", DEF_POSITION,
        0, 0, &positionOption},
    {BLT_CONFIG_INT, "-x", "x", "X", DEF_X, Blt_Offset(Crosshairs, x), 0},
    {BLT_CONFIG_INT, "-y", "y", "Y", DEF_Y, Blt_Offset(Crosshairs, y), 0},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPosition --
 *
 *      Convert the string representation of a screen XY position into
 *      window coordinates.  The form of the string must be "@x,y" or none.
 *
 * Results:
 *      A standard TCL result.  The position is written into the Crosshairs
 *      record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPosition(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Crosshairs *chPtr = (Crosshairs *)widgRec;
    int x, y;

    if (Blt_GetXY(interp, tkwin, Tcl_GetString(objPtr), &x, &y) != TCL_OK) {
        return TCL_ERROR;
    }
    chPtr->x = x, chPtr->y = y;
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
    Crosshairs *chPtr = (Crosshairs *)widgRec;
    char string[200];
    
    Blt_FmtString(string, 200, "@%d,%d", chPtr->x, chPtr->y);
    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * TurnOffHairs --
 *
 *      XOR's the existing line segments (representing the crosshairs),
 *      thereby erasing them.  The internal state of the crosshairs is
 *      tracked.
 *
 * Results:
 *      None
 *
 * Side Effects:
 *      Crosshairs are erased.
 *
 *---------------------------------------------------------------------------
 */
static void
TurnOffHairs(Tk_Window tkwin, Crosshairs *chPtr)
{
    if (Tk_IsMapped(tkwin) && (chPtr->flags & ACTIVE)) {
        XDrawSegments(Tk_Display(tkwin), Tk_WindowId(tkwin), chPtr->gc,
            chPtr->segArr, 2);
        chPtr->flags &= ~ACTIVE;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TurnOnHairs --
 *
 *      Draws (by XORing) new line segments, creating the effect of
 *      crosshairs. The internal state of the crosshairs is tracked.
 *
 * Results:
 *      None
 *
 * Side Effects:
 *      Crosshairs are displayed.
 *
 *---------------------------------------------------------------------------
 */
static void
TurnOnHairs(Graph *graphPtr, Crosshairs *chPtr)
{
    if (Tk_IsMapped(graphPtr->tkwin) && ((chPtr->flags & ACTIVE) == 0)) {
        if (!PointInGraph(graphPtr, chPtr->x, chPtr->y)) {
            return;             /* Coordinates are off the graph */
        }
        XDrawSegments(graphPtr->display, Tk_WindowId(graphPtr->tkwin),
            chPtr->gc, chPtr->segArr, 2);
        chPtr->flags |= ACTIVE;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureCrosshairs --
 *
 *      Configures attributes of the crosshairs such as line width,
 *      dashes, and position.  The crosshairs are first turned off
 *      before any of the attributes changes.
 *
 * Results:
 *      None
 *
 * Side Effects:
 *      Crosshair GC is allocated.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ConfigureCrosshairs(Graph *graphPtr)
{
    XGCValues gcValues;
    unsigned long gcMask;
    GC newGC;
    unsigned long int pixel;
    Crosshairs *chPtr = graphPtr->crosshairs;

    /*
     * Turn off the crosshairs temporarily. This is in case the new
     * configuration changes the size, style, or position of the lines.
     */
    TurnOffHairs(graphPtr->tkwin, chPtr);
    gcValues.function = GXxor;

    if (graphPtr->plotBg == NULL) {
        /* The graph's color option may not have been set yet */
        pixel = WhitePixelOfScreen(Tk_Screen(graphPtr->tkwin));
    } else {
        pixel = Blt_Bg_BorderColor(graphPtr->plotBg)->pixel;
    }
    gcValues.background = pixel;
    gcValues.foreground = (pixel ^ chPtr->colorPtr->pixel);

    gcValues.line_width = LineWidth(chPtr->lineWidth);
    gcMask = (GCForeground | GCBackground | GCFunction | GCLineWidth);
    if (LineIsDashed(chPtr->dashes)) {
        gcValues.line_style = LineOnOffDash;
        gcMask |= GCLineStyle;
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(chPtr->dashes)) {
        Blt_SetDashes(graphPtr->display, newGC, &chPtr->dashes);
    }
    if (chPtr->gc != NULL) {
        Blt_FreePrivateGC(graphPtr->display, chPtr->gc);
    }
    chPtr->gc = newGC;

    /*
     * Are the new coordinates on the graph?
     */
    chPtr->segArr[0].x2 = chPtr->segArr[0].x1 = chPtr->x;
    chPtr->segArr[0].y1 = graphPtr->y2;
    chPtr->segArr[0].y2 = graphPtr->y1;
    chPtr->segArr[1].y2 = chPtr->segArr[1].y1 = chPtr->y;
    chPtr->segArr[1].x1 = graphPtr->x1;
    chPtr->segArr[1].x2 = graphPtr->x2;

    if ((chPtr->flags & HIDDEN) == 0) {
        TurnOnHairs(graphPtr, chPtr);
    }
}

void
Blt_EnableCrosshairs(Graph *graphPtr)
{
    if ((graphPtr->crosshairs->flags & HIDDEN) == 0) {
        TurnOnHairs(graphPtr, graphPtr->crosshairs);
    }
}

void
Blt_DisableCrosshairs(Graph *graphPtr)
{
    if ((graphPtr->crosshairs->flags & HIDDEN) == 0) {
        TurnOffHairs(graphPtr->tkwin, graphPtr->crosshairs);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * UpdateCrosshairs --
 *
 *      Update the length of the hairs (not the hot spot).
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_UpdateCrosshairs(Graph *graphPtr)
{
    Crosshairs *chPtr = graphPtr->crosshairs;

    chPtr->segArr[0].y1 = graphPtr->y2;
    chPtr->segArr[0].y2 = graphPtr->y1;
    chPtr->segArr[1].x1 = graphPtr->x1;
    chPtr->segArr[1].x2 = graphPtr->x2;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyCrosshairs --
 *
 * Results:
 *      None
 *
 * Side Effects:
 *      Crosshair GC is allocated.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DestroyCrosshairs(Graph *graphPtr)
{
    if (graphPtr->crosshairs != NULL) {
        Crosshairs *chPtr = graphPtr->crosshairs;

        Blt_FreeOptions(configSpecs, (char *)chPtr, graphPtr->display, 0);
        if (chPtr->gc != NULL) {
            Blt_FreePrivateGC(graphPtr->display, chPtr->gc);
        }
        Blt_Free(chPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreateCrosshairs --
 *
 *      Creates and initializes a new crosshair structure.
 *
 * Results:
 *      Returns TCL_ERROR if the crosshair structure can't be created,
 *      otherwise TCL_OK.
 *
 * Side Effects:
 *      Crosshair GC is allocated.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_CreateCrosshairs(Graph *graphPtr)
{
    Crosshairs *chPtr;

    chPtr = Blt_AssertCalloc(1, sizeof(Crosshairs));
    chPtr->flags = HIDDEN;
    chPtr->x = chPtr->y = -1;
    graphPtr->crosshairs = chPtr;

    if (Blt_ConfigureComponentFromObj(graphPtr->interp, graphPtr->tkwin,
            "crosshairs", "Crosshairs", configSpecs, 0, (Tcl_Obj **)NULL,
            (char *)chPtr, 0) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *      Queries configuration attributes of the crosshairs such as
 *      line width, dashes, and position.
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
    Crosshairs *chPtr = graphPtr->crosshairs;

    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
            (char *)chPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      Queries or resets configuration attributes of the crosshairs
 *      such as line width, dashes, and position.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Crosshairs are reset.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Crosshairs *chPtr = graphPtr->crosshairs;

    if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
                (char *)chPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
        return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
                (char *)chPtr, objv[3], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
        objc - 3, objv + 3, (char *)chPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_ConfigureCrosshairs(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OnOp --
 *
 *      Maps the crosshairs.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Crosshairs are reset if necessary.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
OnOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Crosshairs *chPtr = graphPtr->crosshairs;

    if (chPtr->flags & HIDDEN) {
        TurnOnHairs(graphPtr, chPtr);
        chPtr->flags &= ~HIDDEN;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OffOp --
 *
 *      Unmaps the crosshairs.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Crosshairs are reset if necessary.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
OffOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Crosshairs *chPtr = graphPtr->crosshairs;

    if ((chPtr->flags & HIDDEN) == 0) {
        TurnOffHairs(graphPtr->tkwin, chPtr);
        chPtr->flags |= HIDDEN;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ToggleOp --
 *
 *      Toggles the state of the crosshairs.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Crosshairs are reset.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ToggleOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Crosshairs *chPtr = graphPtr->crosshairs;

    if (chPtr->flags & HIDDEN) {
        chPtr->flags &= ~HIDDEN;
        TurnOnHairs(graphPtr, chPtr);
    } else {
        chPtr->flags |= HIDDEN;
        TurnOffHairs(graphPtr->tkwin, chPtr);
    }
    return TCL_OK;
}


static Blt_OpSpec xhairOps[] =
{
    {"cget",      2, CgetOp,      4, 4, "option",},
    {"configure", 2, ConfigureOp, 3, 0, "?options...?",},
    {"off",       2, OffOp,       3, 3, "",},
    {"on",        2, OnOp,        3, 3, "",},
    {"toggle",    1, ToggleOp,    3, 3, "",},
};
static int numXhairOps = sizeof(xhairOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CrosshairsOp --
 *
 *      User routine to configure crosshair simulation.  Crosshairs
 *      are simulated by drawing line segments parallel to both axes
 *      using the XOR drawing function. The allows the lines to be
 *      erased (by drawing them again) without redrawing the entire
 *      graph.  Care must be taken to erase crosshairs before redrawing
 *      the graph and redraw them after the graph is redraw.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 * Side Effects:
 *      Crosshairs may be drawn in the plotting area.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_CrosshairsOp(ClientData clientData, Tcl_Interp *interp, int objc,
                 Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numXhairOps, xhairOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

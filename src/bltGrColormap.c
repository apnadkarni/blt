/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrColormap.c --
 *
 * This module implements colormap components for the BLT graph widget.
 *
 *	Copyright 2011 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use, copy,
 *	modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltChain.h"
#include "bltPicture.h"
#include "bltHash.h"
#include "bltBind.h"
#include "bltPs.h"
#include "bltBg.h"
#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltConfig.h"
#include "bltOp.h"

#define DEF_AXIS	"z"
#define DEF_PALETTE	"blue"

#define HORIZONTAL	(1<<26)
/* Indicates how to rotate axis title for each margin. */
static float titleAngle[4] =		
{
    0.0, 90.0, 0.0, 270.0
};

typedef struct {
    int axis;				/* Length of the axis.  */
    int t1;			        /* Length of a major tick (in
					 * pixels). */
    int t2;			        /* Length of a minor tick (in
					 * pixels). */
    int label;				/* Distance from axis to tick
                                         * label. */
} AxisInfo;

typedef struct {
    const char *name;
    ClassId classId;
    int margin, invertMargin;
} AxisName;

static Blt_OptionFreeProc FreePaletteProc;
static Blt_OptionParseProc ObjToPaletteProc;
static Blt_OptionPrintProc PaletteToObjProc;
static Blt_CustomOption paletteOption =
{
    ObjToPaletteProc, PaletteToObjProc, FreePaletteProc, (ClientData)0
};


BLT_EXTERN Blt_CustomOption bltLimitOption;
BLT_EXTERN Blt_CustomOption bltAxisOption;

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-axis", "axis", "Axis", DEF_AXIS,
	Blt_Offset(GraphColormap, axisPtr), 0, &bltAxisOption},
    {BLT_CONFIG_CUSTOM, "-max", "max", "Max", (char *)NULL, 
	Blt_Offset(GraphColormap, reqMax), 0, &bltLimitOption},
    {BLT_CONFIG_CUSTOM, "-min", "min", "Min", (char *)NULL, 
	Blt_Offset(GraphColormap, reqMin), 0, &bltLimitOption},
    {BLT_CONFIG_CUSTOM, "-palette", "palette", "Palette", DEF_PALETTE, 
	Blt_Offset(GraphColormap, palette), 0, &paletteOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

/*
 *---------------------------------------------------------------------------
 *
 * GetColormap --
 *
 *	Gets the named colormap.  The colormap must already exist or an
 *	error message is returned.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
GetColormap(Tcl_Interp *interp, Graph *graphPtr, const char *string,
	    GraphColormap **cmapPtrPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&graphPtr->colormapTable, string);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find a colormap \"", string, "\"",
			 (char *)NULL);
	}
	return TCL_ERROR;
    }
    *cmapPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetColormapFromObj --
 *
 *	Gets the named colormap.  The colormap must already exist or an
 *	error message is returned.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
GetColormapFromObj(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr, 
		   GraphColormap **cmapPtrPtr)
{
    return GetColormap(interp, graphPtr, Tcl_GetString(objPtr), cmapPtrPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * PaletteChangedProc
 *
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
PaletteChangedProc(Blt_Palette palette, ClientData clientData, 
		   unsigned int flags)
{
    GraphColormap *cmapPtr = clientData;
    Graph *graphPtr;

    if (flags & PALETTE_DELETE_NOTIFY) {
	cmapPtr->palette = NULL;
    }
    graphPtr = cmapPtr->graphPtr;
    graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
}


/*ARGSUSED*/
static void
FreePaletteProc(
    ClientData clientData,		/* Not used. */
    Display *display,			/* Not used. */
    char *widgRec,
    int offset)
{
    Blt_Palette *palPtr = (Blt_Palette *)(widgRec + offset);
    GraphColormap *cmapPtr = (GraphColormap *)widgRec;
    
    if (*palPtr != NULL) {
        Blt_Palette_DeleteNotifier(*palPtr, cmapPtr);
        *palPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPaletteProc --
 *
 *	Convert the string representation of a palette into its token.
 *
 * Results:
 *	The return value is a standard TCL result.  The palette token is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPaletteProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing symbol type */
    char *widgRec,			/* Element information record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Blt_Palette *palPtr = (Blt_Palette *)(widgRec + offset);
    GraphColormap *cmapPtr = (GraphColormap *)widgRec;
    const char *string;
    
    string = Tcl_GetString(objPtr);
    if ((string == NULL) || (string[0] == '\0')) {
	FreePaletteProc(clientData, Tk_Display(tkwin), widgRec, offset);
	return TCL_OK;
    }
    if (Blt_Palette_GetFromObj(interp, objPtr, palPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_Palette_CreateNotifier(*palPtr, PaletteChangedProc, cmapPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PaletteToObjProc --
 *
 *	Convert the palette token into a string.
 *
 * Results:
 *	The string representing the symbol type or line style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PaletteToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,
    char *widgRec,			/* Element information record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Blt_Palette palette = *(Blt_Palette *)(widgRec + offset);
    if (palette == NULL) {
	return Tcl_NewStringObj("", -1);
    } 
    return Tcl_NewStringObj(Blt_Palette_Name(palette), -1);
}

static void
NotifyClients(GraphColormap *cmapPtr, unsigned int flags)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&cmapPtr->notifierTable, &iter); 
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	GraphColormapNotifyProc *proc;
	ClientData clientData;

	proc = Blt_GetHashValue(hPtr);
	clientData = Blt_GetHashKey(&cmapPtr->notifierTable, hPtr);
	(*proc)(cmapPtr, clientData, flags);
    }
}

static GraphColormap *
NewColormap(Tcl_Interp *interp, Graph *graphPtr, Blt_HashEntry *hPtr)
{
    GraphColormap *cmapPtr;

    cmapPtr = Blt_AssertCalloc(1, sizeof(GraphColormap));
    cmapPtr->graphPtr = graphPtr;
    cmapPtr->hashPtr = hPtr;
    cmapPtr->name = Blt_GetHashKey(&graphPtr->colormapTable, hPtr);
    cmapPtr->reqMin = Blt_NaN();
    cmapPtr->reqMax = Blt_NaN();
    Blt_InitHashTable(&cmapPtr->notifierTable, BLT_ONE_WORD_KEYS);
    Blt_SetHashValue(hPtr, cmapPtr);
    return cmapPtr;
}

static void
DestroyColormap(GraphColormap *cmapPtr)
{
    Graph *graphPtr;

    NotifyClients(cmapPtr, COLORMAP_DELETE_NOTIFY);
    graphPtr = cmapPtr->graphPtr;
    if (cmapPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&graphPtr->colormapTable, cmapPtr->hashPtr);
    }
    Blt_FreeOptions(configSpecs, (char *)cmapPtr, graphPtr->display, 0);
    Blt_DeleteHashTable(&cmapPtr->notifierTable);
}

static int
ConfigureColormap(Tcl_Interp *interp, GraphColormap *cmapPtr, int objc, 
		  Tcl_Obj *const *objv, int flags)
{
    Graph *graphPtr;

    graphPtr = cmapPtr->graphPtr;
    if (Blt_ConfigureComponentFromObj(interp, graphPtr->tkwin, cmapPtr->name, 
	"Colormap", configSpecs, objc, objv, (char *)cmapPtr, flags) != TCL_OK){
	return TCL_ERROR;
    }
    NotifyClients(cmapPtr, COLORMAP_CHANGE_NOTIFY);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *	$g colormap cget cm option
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    GraphColormap *cmapPtr;

    if (GetColormapFromObj(interp, graphPtr, objv[3], &cmapPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    if (Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
		(char *)cmapPtr, objv[4], 0) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new colormap.
 *
 *	$g colormap configure $cm ?value options?
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    GraphColormap *cmapPtr;

    if (GetColormapFromObj(interp, graphPtr, objv[3], &cmapPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
		configSpecs, (char *)cmapPtr, (Tcl_Obj *)NULL, 
		BLT_CONFIG_OBJV_ONLY);
    } else if (objc == 5) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
		configSpecs, (char *)cmapPtr, objv[4], BLT_CONFIG_OBJV_ONLY);
    } 
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
	objc - 4, objv + 4, (char *)cmapPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Update the pixmap if any configuration option changed */
    graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
    NotifyClients(cmapPtr, COLORMAP_CHANGE_NOTIFY);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *	Creates a colormap.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the element names.
 *
 *	$graph colormap create ?$name? ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Graph *graphPtr = clientData;
    GraphColormap *cmapPtr;
    int isNew;

    hPtr = NULL;			/* Suppress compiler warning. */
    if (objc > 3) {
	const char *string;

	string = Tcl_GetString(objv[3]);
	if (string[0] != '-') {
	    hPtr = Blt_CreateHashEntry(&graphPtr->colormapTable, string, 
		&isNew);
	    if (!isNew) {
		Tcl_AppendResult(interp, "colormap \"", string, 
		  "\" already exists", (char *)NULL); return TCL_ERROR;
		return TCL_ERROR;
	    }
	    objc--, objv++;
	}
    } 
    if (hPtr == NULL) {
	/* If no name was given for the colormap, make up one. */
	do { 
	    char ident[200];

	    Blt_FormatString(ident, 200, "colormap%d",
                        graphPtr->nextColormapId++);
	    hPtr = Blt_CreateHashEntry(&graphPtr->colormapTable, ident, &isNew);
	} while (!isNew);
    }
    cmapPtr = NewColormap(interp, graphPtr, hPtr);
    if (ConfigureColormap(interp, cmapPtr, objc - 3, objv + 3, 0) != TCL_OK) {
	DestroyColormap(cmapPtr);
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), cmapPtr->name, -1);
    return TCL_OK;
}

static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
	GraphColormap *cmapPtr;

	if (GetColormapFromObj(interp, graphPtr, objv[i], &cmapPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	DestroyColormap(cmapPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *	Returns a list of colormap identifiers in interp->result;
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 2) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&graphPtr->colormapTable, &iter); 
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    GraphColormap *cmapPtr;

	    cmapPtr = Blt_GetHashValue(hPtr);
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj(cmapPtr->name, -1));
	}
    } else {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&graphPtr->colormapTable, &iter); 
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    GraphColormap *cmapPtr;
	    int i;

	    cmapPtr = Blt_GetHashValue(hPtr);
	    for (i = 2; i < objc; i++) {
		const char *pattern;

		pattern = Tcl_GetString(objv[i]);
		if (Tcl_StringMatch(cmapPtr->name, pattern)) {
		    Tcl_ListObjAppendElement(interp, listObjPtr,
			Tcl_NewStringObj(cmapPtr->name, -1));
		    break;
		}
	    }
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

void
Blt_DestroyColormaps(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&graphPtr->colormapTable, &iter); 
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	GraphColormap *cmapPtr;

	cmapPtr = Blt_GetHashValue(hPtr);
	cmapPtr->hashPtr = NULL;
	DestroyColormap(cmapPtr);
    }
    Blt_DeleteHashTable(&graphPtr->colormapTable);
}

void
Blt_Colormap_Free(GraphColormap *cmapPtr)
{
    if (cmapPtr != NULL) {
	DestroyColormap(cmapPtr);
    }
}

void
Blt_Colormap_Init(GraphColormap *cmapPtr)
{
    if (cmapPtr->palette != NULL) {
	cmapPtr->min = cmapPtr->axisPtr->valueRange.min;
	cmapPtr->max = cmapPtr->axisPtr->valueRange.max;
	if (DEFINED(cmapPtr->reqMin)) {
	    cmapPtr->min = cmapPtr->reqMin;
	}
	if (DEFINED(cmapPtr->reqMax)) {
	    cmapPtr->max = cmapPtr->reqMax;
	}
	Blt_Palette_SetRange(cmapPtr->palette, cmapPtr->min, cmapPtr->max);
    }
}

int
Blt_Colormap_Get(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr, 
		 GraphColormap **cmapPtrPtr)
{
    return GetColormap(interp, graphPtr, Tcl_GetString(objPtr), cmapPtrPtr);
}

void
Blt_Colormap_CreateNotifier(GraphColormap *cmapPtr, 
			    GraphColormapNotifyProc *proc, 
			    ClientData clientData)
{
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&cmapPtr->notifierTable, clientData, &isNew);
    Blt_SetHashValue(hPtr, proc);
}

void
Blt_Colormap_DeleteNotifier(GraphColormap *cmapPtr, ClientData clientData)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&cmapPtr->notifierTable, clientData);
    if (hPtr != NULL) {
	Blt_DeleteHashEntry(&cmapPtr->notifierTable, hPtr);
    }
}

/* 
 *   ---------------------------------
 *   colormap
 *   ---------------------------------
 *   |    |    |    |   |   |   |    |
 *   0    1    2    3   4   5   6    7
 * 
 */
int
Blt_Colormap_GetGeometry(Graph *graphPtr, GraphColormap *cmapPtr)
{
    int isHoriz = 0;
    int l, w, h;
    
    if (graphPtr->flags & GET_AXIS_GEOMETRY) {
        Blt_GetAxisGeometry(graphPtr, cmapPtr->axisPtr);
    }
    l = w = h = 0;
    /* Add in the bar thickness. */
    if (isHoriz) {
        if (h < cmapPtr->axisPtr->height) {
            h = cmapPtr->axisPtr->height;
        }
        h += cmapPtr->thickness;
    } else {
        if (w < cmapPtr->axisPtr->width) {
            w = cmapPtr->axisPtr->width;
        }
        w += cmapPtr->thickness;
    }
    cmapPtr->width = w;
    cmapPtr->height = h;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisOffsets --
 *
 *	Determines the sites of the axis, major and minor ticks, and title
 *	of the axis.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
AxisOffsets(Axis *axisPtr, int side, int offset, AxisInfo *infoPtr)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    int pad;				/* Offset of axis from interior
					 * region. This includes a possible
					 * border and the axis line
					 * width. */
    int axisLine;
    int t1, t2, labelOffset;
    int tickLabel, axisPad;
    int inset, mark;
    int x, y;
    float fangle;

    axisPtr->titleAngle = titleAngle[side];
    tickLabel = axisLine = t1 = t2 = 0;
    labelOffset = AXIS_PAD_TITLE;
    if (axisPtr->lineWidth > 0) {
	if (axisPtr->flags & SHOWTICKS) {
	    t1 = axisPtr->tickLength;
	    t2 = (t1 * 10) / 15;
	}
	labelOffset = t1 + AXIS_PAD_TITLE;
	if (axisPtr->flags & EXTERIOR) {
	    labelOffset += axisPtr->lineWidth;
	}
    }
    axisPad = 0;
    if (graphPtr->plotRelief != TK_RELIEF_SOLID) {
	axisPad = 0;
    }
    /* Adjust offset for the interior border width and the line width */
    pad = 1;
    if (graphPtr->plotBW > 0) {
	pad += graphPtr->plotBW + 1;
    }
    pad = 0;				/* FIXME: test */
    /*
     * Pre-calculate the x-coordinate positions of the axis, tick labels,
     * and the individual major and minor ticks.
     */
    inset = pad + axisPtr->lineWidth / 2;
    switch (side) {
    case MARGIN_TOP:
	axisLine = graphPtr->top;
	if (axisPtr->flags & EXTERIOR) {
	    axisLine -= graphPtr->plotBW + axisPad + axisPtr->lineWidth / 2;
	    tickLabel = axisLine - 2;
	    if (axisPtr->lineWidth > 0) {
		tickLabel -= axisPtr->tickLength;
	    }
	} else {
	    if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
		axisLine--;
	    } 
	    axisLine -= axisPad + axisPtr->lineWidth / 2;
	    tickLabel = graphPtr->top -  graphPtr->plotBW - 2;
	}
	mark = graphPtr->top - offset - pad;
	axisPtr->tickAnchor = TK_ANCHOR_S;
	axisPtr->left = axisPtr->screenMin - inset - 2;
	axisPtr->right = axisPtr->screenMin + axisPtr->screenRange + inset - 1;
	if (graphPtr->stackAxes) {
	    axisPtr->top = mark - marginPtr->axesOffset;
	} else {
	    axisPtr->top = mark - axisPtr->height;
	}
	axisPtr->bottom = mark;
	if (axisPtr->titleAlternate) {
	    x = graphPtr->right + AXIS_PAD_TITLE;
	    y = mark - (axisPtr->height  / 2);
	    axisPtr->titleAnchor = TK_ANCHOR_W;
	} else {
	    x = (axisPtr->right + axisPtr->left) / 2;
	    if (graphPtr->stackAxes) {
		y = mark - marginPtr->axesOffset + AXIS_PAD_TITLE;
	    } else {
		y = mark - axisPtr->height + AXIS_PAD_TITLE;
	    }
	    axisPtr->titleAnchor = TK_ANCHOR_N;
	}
	axisPtr->titlePos.x = x;
	axisPtr->titlePos.y = y;
	break;

    case MARGIN_BOTTOM:
	/*
	 *  ----------- bottom + plot borderwidth
	 *      mark --------------------------------------------
	 *          ===================== axisLine (linewidth)
	 *                   tick
	 *		    title
	 *
	 *          ===================== axisLine (linewidth)
	 *  ----------- bottom + plot borderwidth
	 *      mark --------------------------------------------
	 *                   tick
	 *		    title
	 */
	axisLine = graphPtr->bottom;
	if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
	    axisLine++;
	} 
	if (axisPtr->flags & EXTERIOR) {
	    axisLine += graphPtr->plotBW + axisPad + axisPtr->lineWidth / 2;
	    tickLabel = axisLine + 2;
	    if (axisPtr->lineWidth > 0) {
		tickLabel += axisPtr->tickLength;
	    }
	} else {
            axisLine -= axisPad + axisPtr->lineWidth / 2;
	    tickLabel = graphPtr->bottom +  graphPtr->plotBW + 2;
	}
	mark = graphPtr->bottom + offset;
	fangle = FMOD(axisPtr->tickAngle, 90.0f);
	if (fangle == 0.0) {
	    axisPtr->tickAnchor = TK_ANCHOR_N;
	} else {
	    int quadrant;

	    quadrant = (int)(axisPtr->tickAngle / 90.0);
	    if ((quadrant == 0) || (quadrant == 2)) {
		axisPtr->tickAnchor = TK_ANCHOR_NE;
	    } else {
		axisPtr->tickAnchor = TK_ANCHOR_NW;
	    }
	}
	axisPtr->left = axisPtr->screenMin - inset - 2;
	axisPtr->right = axisPtr->screenMin + axisPtr->screenRange + inset - 1;
	axisPtr->top = graphPtr->bottom + labelOffset - t1;
	if (graphPtr->stackAxes) {
	    axisPtr->bottom = mark - 1;
	} else {
	    axisPtr->bottom = mark + axisPtr->height - 1;
	}
	if (axisPtr->titleAlternate) {
	    x = graphPtr->right + AXIS_PAD_TITLE;
	    y = mark + (axisPtr->height / 2);
	    axisPtr->titleAnchor = TK_ANCHOR_W; 
	} else {
	    x = (axisPtr->right + axisPtr->left) / 2;
	    if (graphPtr->stackAxes) {
		y = mark - AXIS_PAD_TITLE;
	    } else {
		y = mark + axisPtr->height - AXIS_PAD_TITLE;
	    }
	    axisPtr->titleAnchor = TK_ANCHOR_S; 
	}
	axisPtr->titlePos.x = x;
	axisPtr->titlePos.y = y;
	break;

    case MARGIN_LEFT:
	/*
	 *                    mark
	 *                  |  : 
	 *                  |  :      
	 *                  |  : 
	 *                  |  :
	 *                  |  : 
	 *     axisLine
	 */
	/* 
	 * Exterior axis 
	 *     + plotarea right
	 *     |A|B|C|D|E|F|G|H
	 *           |right
	 * A = plot pad 
	 * B = plot border width
	 * C = axis pad
	 * D = axis line
	 * E = tick length
	 * F = tick label 
	 * G = graph border width
	 * H = highlight thickness
	 */
	/* 
	 * Interior axis 
	 *     + plotarea right
	 *     |A|B|C|D|E|F|G|H
	 *           |right
	 * A = plot pad 
	 * B = tick length
	 * C = axis line width
	 * D = axis pad
	 * E = plot border width
	 * F = tick label 
	 * G = graph border width
	 * H = highlight thickness
	 */
	axisLine = graphPtr->left;
	if (axisPtr->flags & EXTERIOR) {
	    axisLine -= graphPtr->plotBW + axisPad + axisPtr->lineWidth / 2;
	    tickLabel = axisLine - 2;
	    if (axisPtr->lineWidth > 0) {
		tickLabel -= axisPtr->tickLength;
	    }
	} else {
	    if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
		axisLine--;
	    } 
	    axisLine += axisPad + axisPtr->lineWidth / 2;
	    tickLabel = graphPtr->left - graphPtr->plotBW - 2;
	}
	mark = graphPtr->left - offset;
	axisPtr->tickAnchor = TK_ANCHOR_E;
        axisPtr->left = mark - axisPtr->width;
	axisPtr->right = mark - 3;
	axisPtr->top = axisPtr->screenMin - inset - 2;
	axisPtr->bottom = axisPtr->screenMin + axisPtr->screenRange + inset - 1;
	if (axisPtr->titleAlternate) {
	    x = mark - (axisPtr->width / 2);
	    y = graphPtr->top - AXIS_PAD_TITLE;
	    axisPtr->titleAnchor = TK_ANCHOR_SW; 
	} else {
            x = mark - axisPtr->width + AXIS_PAD_TITLE;
	    y = (axisPtr->bottom + axisPtr->top) / 2;
	    axisPtr->titleAnchor = TK_ANCHOR_W; 
	} 
	axisPtr->titlePos.x = x;
	axisPtr->titlePos.y = y;
	break;

    case MARGIN_RIGHT:
	axisLine = graphPtr->right;
	if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
	    axisLine++;			/* Draw axis line within solid plot
					 * border. */
	} 
	if (axisPtr->flags & EXTERIOR) {
	    axisLine += graphPtr->plotBW + axisPad + axisPtr->lineWidth / 2;
	    tickLabel = axisLine + 2;
	    if (axisPtr->lineWidth > 0) {
		tickLabel += axisPtr->tickLength;
	    }
	} else {
	    axisLine -= axisPad + axisPtr->lineWidth / 2;
	    tickLabel = graphPtr->right + graphPtr->plotBW + 2;
	}
	mark = graphPtr->right + offset + pad;
	axisPtr->tickAnchor = TK_ANCHOR_W;
	axisPtr->left = mark;
        axisPtr->right = mark + axisPtr->width - 1;
	axisPtr->top = axisPtr->screenMin - inset - 2;
	axisPtr->bottom = axisPtr->screenMin + axisPtr->screenRange + inset -1;
	if (axisPtr->titleAlternate) {
	    x = mark + (axisPtr->width / 2);
	    y = graphPtr->top - AXIS_PAD_TITLE;
	    axisPtr->titleAnchor = TK_ANCHOR_SE; 
	} else {
            x = mark + axisPtr->width - AXIS_PAD_TITLE;
	    y = (axisPtr->bottom + axisPtr->top) / 2;
	    axisPtr->titleAnchor = TK_ANCHOR_E;
	}
        axisPtr->titlePos.x = x;
	axisPtr->titlePos.y = y;
	break;

    case MARGIN_NONE:
	axisLine = 0;
	break;
    }
    if ((side == MARGIN_LEFT) || (side == MARGIN_TOP)) {
	t1 = -t1, t2 = -t2;
	labelOffset = -labelOffset;
    }
    infoPtr->axis = axisLine;
    infoPtr->t1 = axisLine + t1;
    infoPtr->t2 = axisLine + t2;
    if (tickLabel > 0) {
	infoPtr->label = tickLabel;
    } else {
	infoPtr->label = axisLine + labelOffset;
    }
    if ((axisPtr->flags & EXTERIOR) == 0) {
	/*infoPtr->label = axisLine + labelOffset - t1; */
	infoPtr->t1 = axisLine - t1;
	infoPtr->t2 = axisLine - t2;
    } 
}

static void
MakeAxisLine(Axis *axisPtr, int line, Segment2d *s)
{
    double min, max;

    min = axisPtr->axisRange.min;
    max = axisPtr->axisRange.max;
    if (axisPtr->logScale) {
	min = EXP10(min);
	max = EXP10(max);
    }
    if (axisPtr->flags & HORIZONTAL) {
	s->p.x = Blt_HMap(axisPtr, min);
	s->q.x = Blt_HMap(axisPtr, max);
	s->p.y = s->q.y = line;
    } else {
	s->q.x = s->p.x = line;
	s->p.y = Blt_VMap(axisPtr, min);
	s->q.y = Blt_VMap(axisPtr, max);
    }
}

static void
MakeTick(Axis *axisPtr, double value, int tick, int line, Segment2d *s)
{
    if (axisPtr->logScale) {
	value = EXP10(value);
    }
    if (axisPtr->flags & HORIZONTAL) {
	s->p.x = s->q.x = Blt_HMap(axisPtr, value);
	s->p.y = line;
	s->q.y = tick;
    } else {
	s->p.x = line;
	s->p.y = s->q.y = Blt_VMap(axisPtr, value);
	s->q.x = tick;
    }
}

static void
MakeSegments(Axis *axisPtr, AxisInfo *infoPtr)
{
    int arraySize;
    int numMajorTicks, numMinorTicks;
    Segment2d *segments;
    Segment2d *s;

    if (axisPtr->segments != NULL) {
	Blt_Free(axisPtr->segments);
    }
    numMajorTicks = axisPtr->major.ticks.numSteps;
    numMinorTicks = axisPtr->minor.ticks.numSteps;
    arraySize = 1 + (numMajorTicks * (numMinorTicks + 1));
    segments = Blt_AssertMalloc(arraySize * sizeof(Segment2d));
    s = segments;
    if (axisPtr->lineWidth > 0) {
	/* Axis baseline */
	MakeAxisLine(axisPtr, infoPtr->axis, s);
	s++;
    }
    if (axisPtr->flags & SHOWTICKS) {
	Blt_ChainLink link;
	double labelPos;
        Tick left, right;

	link = Blt_Chain_FirstLink(axisPtr->tickLabels);
	labelPos = (double)infoPtr->label;
	for (left = FirstMajorTick(axisPtr); left.isValid; left = right) {

            right = NextMajorTick(axisPtr);
            if (right.isValid) {
                Tick minor;

                /* If this isn't the last major tick, add minor ticks. */
                axisPtr->minor.ticks.range = right.value - left.value;
                axisPtr->minor.ticks.initial = left.value;
                for (minor = FirstMinorTick(axisPtr); minor.isValid; 
                     minor = NextMinorTick(axisPtr)) {
                    if (InRange(minor.value, &axisPtr->axisRange)) {
                        /* Add minor tick. */
                        MakeTick(axisPtr, minor.value, infoPtr->t2, 
                                infoPtr->axis, s);
                        s++;
                    }else {
 fprintf(stderr, "minor %d not in range %.15g min=%.15g max=%.15g\n",
         axisPtr->minor.ticks.index, minor.value, axisPtr->axisRange.min, 
                        axisPtr->axisRange.max);
                    }
                }        
            }
            if (InRange(left.value, &axisPtr->axisRange)) {
                double mid;

                /* Add major tick. This could be the last major tick. */
                MakeTick(axisPtr, left.value, infoPtr->t1, infoPtr->axis, s);
                
                mid = left.value;
                if ((axisPtr->labelOffset) && (right.isValid)) {
                    mid = (right.value - left.value) * 0.5;
                }
                if (InRange(mid, &axisPtr->axisRange)) {
                    TickLabel *labelPtr;

                    labelPtr = Blt_Chain_GetValue(link);
                    link = Blt_Chain_NextLink(link);
                    
                    /* Set the position of the tick label. */
                    if (axisPtr->flags & HORIZONTAL) {
                        labelPtr->anchorPos.x = s->p.x;
                        labelPtr->anchorPos.y = labelPos;
                    } else {
                        labelPtr->anchorPos.x = labelPos;
                        labelPtr->anchorPos.y = s->p.y;
                    }
                }
                s++;
            }
        }
    }
    axisPtr->segments = segments;
    axisPtr->numSegments = s - segments;
    assert(axisPtr->numSegments <= arraySize);
}

/*
 *---------------------------------------------------------------------------
 *
 * MapAxis --
 *
 *	Pre-calculates positions of the axis, ticks, and labels (to be used
 *	later when displaying the axis).  Calculates the values for each major
 *	and minor tick and checks to see if they are in range (the outer ticks
 *	may be outside of the range of plotted values).
 *
 *	Line segments for the minor and major ticks are saved into one
 *	XSegment array so that they can be drawn by a single XDrawSegments
 *	call. The positions of the tick labels are also computed and saved.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Line segments and tick labels are saved and used later to draw the
 *	axis.
 *
 *---------------------------------------------------------------------------
 */
static void
MapColorbar(Graph *graphPtr, GraphColormap *cmapPtr)
{
    AxisInfo info;
    Axis *axisPtr;

    axisPtr = cmapPtr->axisPtr;
    if (axisPtr->flags & HORIZONTAL) {
	axisPtr->screenMin = graphPtr->hOffset;
	axisPtr->width = graphPtr->right - graphPtr->left;
	axisPtr->screenRange = graphPtr->hRange;
    } else {
	axisPtr->screenMin = graphPtr->vOffset;
	axisPtr->height = graphPtr->bottom - graphPtr->top;
	axisPtr->screenRange = graphPtr->vRange;
	axisPtr->height -= graphPtr->top + Blt_Legend_Height(graphPtr);
	axisPtr->screenRange = graphPtr->vRange - Blt_Legend_Height(graphPtr);
    }
    axisPtr->screenScale = 1.0 / axisPtr->screenRange;
    AxisOffsets(axisPtr, cmapPtr->side, offset, &info);
    MakeSegments(axisPtr, &info);
}

int
Blt_Colormap_MapColorbar(Graph *graphPtr, GraphColormap *cmapPtr)
{

}

/*
 *---------------------------------------------------------------------------
 *
 * ColormapCmd --
 *
 *	.g colormap create ?name? ?value option?
 *	.g colormap configure name ?value option?
 *	.g colormap cget name value
 *	.g colormap names ?pattern?
 *	.g colormap delete name
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec colormapOps[] =
{
    {"cget",        2, CgetOp,       3, 4, "name option",},
    {"configure",   2, ConfigureOp,  2, 0, "name ?option value?...",},
    {"create",      2, CreateOp,     3, 0, "?name? ?option value?...",},
    {"delete",      1, DeleteOp,     2, 0, "?name?...",},
    {"names",       1, NamesOp,      2, 0, "?pattern?...",},
};
static int nColormapOps = sizeof(colormapOps) / sizeof(Blt_OpSpec);

int
Blt_ColormapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, nColormapOps, colormapOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}


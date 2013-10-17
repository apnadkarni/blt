
/*
 * bltGrColormap.c --
 *
 * This module implements colormap components for the BLT graph widget.
 *
 *	Copyright 2011 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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
 *	Gets the named colormap.  The colormap must already exist or
 *	an error message is returned.
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
 *	Gets the named colormap.  The colormap must already exist or
 *	an error message is returned.
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
    
    Blt_Palette_DeleteNotifier(*palPtr, cmapPtr);
    *palPtr = NULL;
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

	    Blt_FormatString(ident, 200, "colormap%d", graphPtr->nextColormapId++);
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


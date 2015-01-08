/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPalette.c --
 *
 * This module implements palettes for contour elements for the BLT graph
 * widget.
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

#include "bltMath.h"
#include "bltPicture.h"

#define PALETTE_THREAD_KEY "BLT Palette Command Interface"
#define RCLAMP(c)	((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))

#define imul8x8(a,b,t)	((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))
#define CLAMP(c)	((((c) < 0) ? 0 : ((c) > 255) ? 255 : (c)))

/*
 * PaletteCmdInterpData --
 *
 *	Structure containing global data, used on a interpreter by
 *	interpreter basis.
 *
 *	This structure holds the hash table of instances of datatable
 *	commands associated with a particular interpreter.
 */
typedef struct {
    Blt_HashTable paletteTable;		/* Tracks palettes in use. */
    Tcl_Interp *interp;
    int nextPaletteCmdId;
} PaletteCmdInterpData;

typedef struct {
    Blt_PaletteEntry *colors;		/* Array of color ranges. */
    Blt_PaletteEntry *opacities;	/* Array of opacity ranges. */
    int numColors;			/* # of entries in color array. */
    int numOpacities;			/* # of entries in opacity array. */
    int alpha;

    /* Additional fields for TCL API.  */
    PaletteCmdInterpData *dataPtr;	/*  */
    const char *name;			/* Namespace-specific name for this
					 * palette. */
    Blt_HashEntry *hashPtr;		/* Pointer to this entry in palette
					 * hash table.  */
    Blt_HashTable notifierTable;	/* Table of notifications
					 * registered by clients of the
					 * palette. */
    int opacity;			/* Overall opacity adjustment. */
} PaletteCmd;

static Blt_SwitchParseProc ObjToRGBColors;
static Blt_SwitchPrintProc ColorsToObj;
static Blt_SwitchCustom rgbColorsSwitch =
{
    ObjToRGBColors, ColorsToObj, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToXRGBColors;
static Blt_SwitchPrintProc ColorsToObj;
static Blt_SwitchCustom xrgbColorsSwitch =
{
    ObjToXRGBColors, ColorsToObj, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToColors;
static Blt_SwitchPrintProc ColorsToObj;
static Blt_SwitchCustom colorsSwitch =
{
    ObjToColors, ColorsToObj, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToOpacities;
static Blt_SwitchPrintProc OpacitiesToObj;
static Blt_SwitchCustom opacitiesSwitch =
{
    ObjToOpacities, OpacitiesToObj, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToFade;
static Blt_SwitchPrintProc FadeToObj;
static Blt_SwitchCustom fadeSwitch =
{
    ObjToFade, FadeToObj, NULL, (ClientData)0
};

static Blt_SwitchSpec paletteSpecs[] =
{
    {BLT_SWITCH_CUSTOM, "-colors", (char *)NULL, (char *)NULL, 
	Blt_Offset(PaletteCmd, colors), 0, 0, &colorsSwitch},
    {BLT_SWITCH_CUSTOM, "-opacities", (char *)NULL, (char *)NULL, 
	Blt_Offset(PaletteCmd, opacities), 0, 0, &opacitiesSwitch},
    {BLT_SWITCH_CUSTOM, "-rgbcolors", (char *)NULL, (char *)NULL, 
	Blt_Offset(PaletteCmd, colors), 0, 0, &rgbColorsSwitch},
    {BLT_SWITCH_CUSTOM, "-xrgbcolors", (char *)NULL, (char *)NULL, 
	Blt_Offset(PaletteCmd, colors), 0, 0, &xrgbColorsSwitch},
    {BLT_SWITCH_CUSTOM, "-fade", (char *)NULL, (char *)NULL,
        Blt_Offset(PaletteCmd, alpha), 0, 0, &fadeSwitch},
    {BLT_SWITCH_END}
};

static PaletteCmdInterpData *GetPaletteCmdInterpData(Tcl_Interp *interp);

typedef struct {
    double min;
    double max;
} InterpolateSwitches;

static Blt_SwitchSpec interpolateSwitches[] = 
{
    {BLT_SWITCH_DOUBLE, "-min", "", (char *)NULL,
	Blt_Offset(InterpolateSwitches, min), 0, 0},
    {BLT_SWITCH_DOUBLE, "-max", "", (char *)NULL,
	Blt_Offset(InterpolateSwitches, max), 0, 0},
    {BLT_SWITCH_END}
};

INLINE static int64_t
Round(double x)
{
    return (int64_t) (x + ((x < 0.0) ? -0.5 : 0.5));
}

#define MAXRELERROR 0.0005
#define MAXABSERROR 0.0000005

static int 
RelativeError(double x, double y) 
{
    double e;

    if (fabs(x - y) < MAXABSERROR) {
	return TRUE;
    }
   if (fabs(x) > fabs(y)) {
	e = fabs((x - y) / y);
    } else {
	e = fabs((x - y) / x);
    }
    if (e <= MAXRELERROR) {
        return TRUE;
    }
    return FALSE;
}

static int
InRange(double x, double min, double max)
{
    double range;

    range = max - min;
    if (fabs(range) < DBL_EPSILON) {
	return Blt_AlmostEquals(x, max);
    } else {
	double t;

	t = (x - min) / range;
	if ((t >= 0.0) &&  (t <= 1.0)) {
	    return TRUE;
	}
	if (RelativeError(0.0, t) || RelativeError(1.0, t)) {
	    return TRUE;
	}
    }
    return FALSE;
}

static void
NotifyClients(PaletteCmd *cmdPtr, unsigned int flags)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&cmdPtr->notifierTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Blt_Palette_NotifyProc *proc;
	ClientData clientData;

	proc = Blt_GetHashValue(hPtr);
	clientData = Blt_GetHashKey(&cmdPtr->notifierTable, hPtr);
	(*proc)((Blt_Palette)cmdPtr, clientData, flags);
    }
}

static int
GetPaletteCmd(Tcl_Interp *interp, PaletteCmdInterpData *dataPtr, 
	      const char *string, PaletteCmd **cmdPtrPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&dataPtr->paletteTable, string);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find a palette \"", string, "\"",
			 (char *)NULL);
	}
	return TCL_ERROR;
    }
    *cmdPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

static int
GetPaletteCmdFromObj(Tcl_Interp *interp, PaletteCmdInterpData *dataPtr, 
                     Tcl_Obj *objPtr, PaletteCmd **cmdPtrPtr)
{
    return GetPaletteCmd(interp, dataPtr, Tcl_GetString(objPtr), cmdPtrPtr);
}

static int
GetPointFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_PalettePoint *pointPtr)
{
    char *p, *string;

    string = Tcl_GetString(objPtr);
    p = strchr(string, '%');
    if (p == NULL) {
	if (Blt_GetDoubleFromObj(interp, objPtr, &pointPtr->value) != TCL_OK) {
            return TCL_ERROR;
	}
	pointPtr->isAbsolute = TRUE;
	pointPtr->norm = -1.0;
    } else {
	double value;
	int result;

	*p = '\0';
	result = Tcl_GetDouble(interp, string, &value);
	*p = '%';
	if (result != TCL_OK) {
            return TCL_ERROR;
	}
	if ((value < 0.0) || (value > 100.0)) {
            return TCL_ERROR;
	}
	pointPtr->isAbsolute = FALSE;
	pointPtr->value = pointPtr->norm = value * 0.01;
    }
    return TCL_OK;
}

static int
GetOpacityFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Pixel *pixelPtr)
{
    double value;

    if (Blt_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((value < 0.0) || (value > 1.0)) {
	Tcl_AppendResult(interp, "bad opacity value: should be 0.0 - 1.0",
		(char *)NULL);
	return TCL_ERROR;
    }
    pixelPtr->u32 = 0;
    pixelPtr->Alpha = (int)(value * 255.0);
    return TCL_OK;
}


static int
GetRGBFromObjv(Tcl_Interp *interp, Tcl_Obj *const *objv, Blt_Pixel *colorPtr) 
{
    double r, g, b;
    Blt_Pixel color;

    if (Blt_GetDoubleFromObj(interp, objv[0], &r) != TCL_OK) {
	return TCL_ERROR;
    }
    if (r < 0.0) { 
	r = 0.0;
    } else if (r > 1.0) {
	r = 1.0;
    }
    if (Blt_GetDoubleFromObj(interp, objv[1], &g) != TCL_OK) {
	return TCL_ERROR;
    }
    if (g < 0.0) { 
	g = 0.0;
    } else if (g > 1.0) {
	g = 1.0;
    }
    if (Blt_GetDoubleFromObj(interp, objv[2], &b) != TCL_OK) {
	return TCL_ERROR;
    }
    if (b < 0.0) { 
	b = 0.0;
    } else if (b > 1.0) {
	b = 1.0;
    }
    color.Red   = (int)(r * 255.0);
    color.Green = (int)(g * 255.0);
    color.Blue  = (int)(b * 255.0);
    color.Alpha = 0xFF;
    colorPtr->u32 = color.u32;
    return TCL_OK;
}

/*
 *	-rgbcolors "c1 c2 c3 c4.."
 *	-colors "{v1 c1} {v2 c2} {v3 c3}..."
 *	-colors "{v1 c1 v2 c2} {v3 c3 v4 c4}..."
 */
static int
ParseRGBColors(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
	       Tcl_Obj *const *objv)
{
    Blt_PaletteEntry *entries;
    double step;
    int i, j, numEntries;

    numEntries = (objc / 3) - 1;
    entries = Blt_AssertMalloc(sizeof(Blt_PaletteEntry) * numEntries);
    step = 1.0 / numEntries;
    for (i = j = 0; j < numEntries; i += 3, j++) {
	Blt_PaletteEntry *entryPtr;

	entryPtr = entries + j;
	if ((GetRGBFromObjv(interp, objv+i,     &entryPtr->low) != TCL_OK) ||
            (GetRGBFromObjv(interp, objv+(i+3), &entryPtr->high) != TCL_OK)) {
            goto error;
	}
	entryPtr->min.value = entryPtr->min.norm = j * step;
	entryPtr->max.value = entryPtr->max.norm = (j+1) * step;
	entryPtr->min.isAbsolute = entryPtr->max.isAbsolute = FALSE;
    }
    if (cmdPtr->colors != NULL) {
	Blt_Free(cmdPtr->colors);
    }
    cmdPtr->colors = entries;
    cmdPtr->numColors = numEntries;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *	-xrgbcolors "x r g b ..."
 */
static int
ParseXRGBColors(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
	       Tcl_Obj *const *objv)
{
    Blt_PaletteEntry *entries, *entryPtr;
    int i, numEntries;
    Blt_PalettePoint lastValue;
    Blt_Pixel lastColor;
    
    numEntries = (objc / 4) - 1;
    entries = Blt_AssertMalloc(sizeof(Blt_PaletteEntry) * numEntries);
    if (GetPointFromObj(interp, objv[0], &lastValue) != TCL_OK) {
        goto error;
    }
    if (GetRGBFromObjv(interp, objv + 1, &lastColor) != TCL_OK) {
        goto error;
    }
    for (entryPtr = entries, i = 4; i < objc; i += 4, entryPtr++) {
        entryPtr->min = lastValue;
        entryPtr->low = lastColor;
        if (GetPointFromObj(interp, objv[i], &entryPtr->max) != TCL_OK) {
            goto error;
        }
	if (GetRGBFromObjv(interp, objv+(i+1), &entryPtr->high) != TCL_OK) {
            goto error;
	}
        lastColor = entryPtr->high;
        lastValue = entryPtr->max;
    }
    if (cmdPtr->colors != NULL) {
	Blt_Free(cmdPtr->colors);
    }
    cmdPtr->colors = entries;
    cmdPtr->numColors = numEntries;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *	-colors "c1 c2 c3 c4.."
 *	-colors "{v1 c1} {v2 c2} {v3 c3}..."
 *	-colors "{v1 c1 v2 c2} {v3 c3 v4 c4}..."
 */
static int
ParseColors(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
	    Tcl_Obj *const *objv)
{
    Blt_PaletteEntry *entries;
    double step;
    int i, numEntries;

    numEntries = objc - 1;
    entries = Blt_AssertMalloc(sizeof(Blt_PaletteEntry) * numEntries);
    step = 1.0 / numEntries;
    for (i = 0; i < numEntries; i++) {
	Blt_PaletteEntry *entryPtr;
        Blt_Pixel low, high;
        
	entryPtr = entries + i;
	if ((Blt_GetPixelFromObj(interp, objv[i], &low) != TCL_OK) ||
            (Blt_GetPixelFromObj(interp, objv[i+1], &high) != TCL_OK)) {
            goto error;
	}
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
	entryPtr->min.value = entryPtr->min.norm = i * step;
	entryPtr->max.value = entryPtr->max.norm = (i+1) * step;
	entryPtr->min.isAbsolute = entryPtr->max.isAbsolute = FALSE;
    }
    if (cmdPtr->colors != NULL) {
	Blt_Free(cmdPtr->colors);
    }
    cmdPtr->colors = entries;
    cmdPtr->numColors = numEntries;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

static int 
GetColorPoint(Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_PalettePoint *pointPtr, 
	      Blt_Pixel *colorPtr)
{
    Tcl_Obj **objv; 
    int objc;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc != 2) {
	Tcl_AppendResult(interp, "wrong # elements: should be 2",
			 (char *)NULL);
	return TCL_ERROR;
    }
    if ((GetPointFromObj(interp,     objv[0], pointPtr) != TCL_OK) ||
        (Blt_GetPixelFromObj(interp, objv[1], colorPtr) != TCL_OK)) {
	return TCL_ERROR;
    }
    Blt_AssociateColor(colorPtr);
    return TCL_OK;
}

static int
ParseColorPoints(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
		 Tcl_Obj *const *objv)
{
    Blt_PaletteEntry *entries;
    Blt_Pixel color;
    Blt_PalettePoint point;
    int i, numEntries;

    numEntries = objc - 1;
    if (numEntries <= 0) {
	return TCL_OK;                  /* No interval w/ only 1 point. */
    }
    entries = Blt_AssertMalloc(sizeof(Blt_PaletteEntry) * numEntries);
    if (GetColorPoint(interp, objv[0], &point, &color) != TCL_OK) {
	goto error;
    }
    for (i = 1; i < objc; i++) {
	Blt_PaletteEntry *entryPtr;
	
	entryPtr = entries + (i - 1);
	entryPtr->min = point;
	entryPtr->low.u32 = color.u32;
	if (GetColorPoint(interp, objv[i], &point, &color) != TCL_OK) {
	    goto error;
	}
	entryPtr->max = point;
	entryPtr->high.u32 = color.u32;
    }
    if (cmdPtr->colors != NULL) {
	Blt_Free(cmdPtr->colors);
    }
    cmdPtr->colors = entries;
    cmdPtr->numColors = numEntries;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

static int
ParseColorRanges(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
		 Tcl_Obj *const *objv)
{
    Blt_PaletteEntry *entries;
    int i, numEntries;

    numEntries = objc;
    entries = Blt_AssertMalloc(sizeof(Blt_PaletteEntry) * numEntries);
    for (i = 0; i < numEntries; i++) {
	Tcl_Obj **ev; 
	int ec;
	Blt_PaletteEntry *entryPtr;

	entryPtr = entries + i;
	if (Tcl_ListObjGetElements(interp, objv[i], &ec, &ev) != TCL_OK) {
	    goto error;
	}
	if (ec != 4) {
	    Tcl_AppendResult(interp, "wrong # elements: should be 4",
		(char *)NULL);
	    goto error;
	}
	if ((GetPointFromObj(interp,     ev[0], &entryPtr->min) != TCL_OK) ||
            (Blt_GetPixelFromObj(interp, ev[1], &entryPtr->low) != TCL_OK) ||
            (GetPointFromObj(interp,     ev[2], &entryPtr->max) != TCL_OK) ||
            (Blt_GetPixelFromObj(interp, ev[3], &entryPtr->high) != TCL_OK)) {
	    goto error;
	}
    }
    if (cmdPtr->colors != NULL) {
	Blt_Free(cmdPtr->colors);
    }
    cmdPtr->colors = entries;
    cmdPtr->numColors = numEntries;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}


/*
 *	-opacity "z1 z2 z3 z3.."
 *	-opacity "{z1 o1} {z2 o2} {z3 o3}..."
 *	-opacity "{z1 o1 z2 o2} {z3 o3 z4 o4}..."
 */
static int
ParseOpacities(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
	       Tcl_Obj *const *objv)
{
    Blt_PaletteEntry *entries;
    int i, numEntries;

    numEntries = objc - 1;
    if (numEntries <= 0) {
	return TCL_OK;                  /* No interval w/ only 1 point. */
    }
    entries = Blt_AssertMalloc(sizeof(Blt_PaletteEntry) * numEntries);
    for (i = 0; i < numEntries; i++) {
	Blt_PaletteEntry *entryPtr;

	entryPtr = entries + i;
	if (GetPointFromObj(interp, objv[i], &entryPtr->min) != TCL_OK) {
	    goto error;
	}
	entryPtr->low.u32 = 0x00000000; /* Transparent */
	if (GetPointFromObj(interp, objv[i+1], &entryPtr->max) != TCL_OK) {
	    goto error;
	}
	entryPtr->high.u32 = 0xFF000000; /* Opaque */
    }
    if (cmdPtr->opacities != NULL) {
	Blt_Free(cmdPtr->opacities);
    }
    cmdPtr->opacities = entries;
    cmdPtr->numOpacities = numEntries;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *	-opacity "z1 z2 z3 z3.."
 *	-opacity "{z1 o1} {z2 o2} {z3 o3}..."
 *	-opacity "{z1 o1 z2 o2} {z3 o3 z4 o4}..."
 */
static int
ParseOpacityPoints(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
		   Tcl_Obj *const *objv)
{
    Blt_PaletteEntry *entries;
    int i, numEntries;

    numEntries = (objc - 1);
    if (numEntries <= 0) {
	return TCL_OK;                  /* No interval w/ only 1 point. */
    }
    entries = Blt_AssertMalloc(sizeof(Blt_PaletteEntry) * numEntries);
    for (i = 0; i < numEntries; i++) {
	Blt_PaletteEntry *entryPtr;
	Tcl_Obj **ev;
	int ec;

	if (Tcl_ListObjGetElements(interp, objv[i], &ec, &ev) != TCL_OK) {
	    goto error;
	}
	if (ec != 2) {
	    Tcl_AppendResult(interp, "wrong # of elements: should be 2",
			     (char *)NULL);
	    goto error;
	}
	entryPtr = entries + i;
	if ((GetPointFromObj(interp,   ev[0], &entryPtr->min) != TCL_OK) ||
            (GetOpacityFromObj(interp, ev[1], &entryPtr->low) != TCL_OK)) {
	    goto error;
	}
	if (Tcl_ListObjGetElements(interp, objv[i+1], &ec, &ev) != TCL_OK) {
	    Tcl_AppendResult(interp, "wrong # of elements: should be 2",
			     (char *)NULL);
	    goto error;
	}
	if (ec != 2) {
	    Tcl_AppendResult(interp, "wrong # of elements: should be 2",
			     (char *)NULL);
	    goto error;
	}
	if ((GetPointFromObj(interp,   ev[0], &entryPtr->max) != TCL_OK) || 
            (GetOpacityFromObj(interp, ev[1], &entryPtr->high) != TCL_OK)) {
	    goto error;
	}
    }
    if (cmdPtr->opacities != NULL) {
	Blt_Free(cmdPtr->opacities);
    }
    cmdPtr->opacities = entries;
    cmdPtr->numOpacities = numEntries;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}

/*
 *	-opacity "z1 z2 z3 z3.."
 *	-opacity "{z1 o1} {z2 o2} {z3 o3}..."
 *	-opacity "{z1 o1 z2 o2} {z3 o3 z4 o4}..."
 */
static int
ParseOpacityRanges(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
		   Tcl_Obj *const *objv)
{
    Blt_PaletteEntry *entries;
    int i;

    entries = Blt_AssertMalloc(sizeof(Blt_PaletteEntry) * objc);
    for (i = 0; i < objc; i++) {
	Tcl_Obj **ev;
	int ec;
	Blt_PaletteEntry *entryPtr;

	if (Tcl_ListObjGetElements(interp, objv[i], &ec, &ev) != TCL_OK) {
	    goto error;
	}
	if (ec != 4) {
	    Tcl_AppendResult(interp, "wrong # elements: should be 4",
			     (char *)NULL);
	    goto error;
	}
	entryPtr = entries + i;
	if (GetPointFromObj(interp, ev[0], &entryPtr->min) != TCL_OK) {
	    goto error;
	}
	if (GetOpacityFromObj(interp, ev[1], &entryPtr->low) != TCL_OK) {
	    goto error;
	}
	if (GetPointFromObj(interp, ev[2], &entryPtr->max) != TCL_OK) {
	    goto error;
	}
	if (GetOpacityFromObj(interp, ev[3], &entryPtr->high) != TCL_OK) {
	    goto error;
	}
    }
    if (cmdPtr->opacities != NULL) {
	Blt_Free(cmdPtr->opacities);
    }
    cmdPtr->opacities = entries;
    cmdPtr->numOpacities = objc;
    return TCL_OK;
 error:
    Blt_Free(entries);
    return TCL_ERROR;
}
 

/*
 *---------------------------------------------------------------------------
 *
 * ObjToColors --
 *
 *	Converts the -colors string into its numeric representation.
 *
 *	Valid color strings are:
 *
 *      -colors "c1 c2 c3 c4..."   Color ramp evenly spaced.
 *
 * 	-colors "{c1 z1} {c2 z2} {c3 z3}..."   
 *			   Color ramp with relative/absolute points. 
 *
 * 	-colors "{c1 z1 c2 z2} {c3 z3 c4 z4}..."   
 *			   Color ranges. 
 *
 * 	-colors "{c1 z1 c2 z2} {c3 z3 c4 z4}..."   
 *			   Color ranges. 
 *
 * 	-colors "{c1 z1% c2 z2%} {c3 z3 c4 z4}..."   
 *			   Color ranges. 
 *
 *	-opacity "z1 z2 z3 z3.."
 *	-opacity "{z1 o1} {z2 o2} {z3 o3}..."
 *	-opacity "{z1 o1 z2 o2} {z3 o3 z4 o4}..."
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToRGBColors(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,
    Tcl_Obj *objPtr,			/* Mode style string */
    char *record,			/* Cubicle structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    PaletteCmd *cmdPtr = (PaletteCmd *)record;
    Tcl_Obj **objv;
    int objc;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 0) {
	if (cmdPtr->colors != NULL) {
	    Blt_Free(cmdPtr->colors);
	    cmdPtr->colors = NULL;
	}
	cmdPtr->numColors = 0;
	return TCL_OK;
    }
    if (objc < 2) {
	Tcl_AppendResult(interp, 
		"too few elements: must have at least 2 color values",
		(char *)NULL);
	return TCL_ERROR;
    }
    if ((objc % 3) != 0) {
	Tcl_AppendResult(interp, "wrong # of elements: should be r g b...",
		(char *)NULL);
	return TCL_ERROR;
    }
    return ParseRGBColors(interp, cmdPtr, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToXRGBColors --
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToXRGBColors(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,
    Tcl_Obj *objPtr,			/* Mode style string */
    char *record,			/* Cubicle structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    PaletteCmd *cmdPtr = (PaletteCmd *)record;
    Tcl_Obj **objv;
    int objc;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 0) {
	if (cmdPtr->colors != NULL) {
	    Blt_Free(cmdPtr->colors);
	    cmdPtr->colors = NULL;
	}
	cmdPtr->numColors = 0;
	return TCL_OK;
    }
    if (objc < 8) {
	Tcl_AppendResult(interp, 
		"too few elements: must have at least 2 color values",
		(char *)NULL);
	return TCL_ERROR;
    }
    if ((objc % 4) != 0) {
	Tcl_AppendResult(interp, "wrong # of elements: should be r g b...",
		(char *)NULL);
	return TCL_ERROR;
    }
    return ParseXRGBColors(interp, cmdPtr, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToColors --
 *
 *	Converts the -colors string into its numeric representation.
 *
 *	Valid color strings are:
 *
 *      -colors "c1 c2 c3 c4..."   Color ramp evenly spaced.
 *
 * 	-colors "{c1 z1} {c2 z2} {c3 z3}..."   
 *			   Color ramp with relative/absolute points. 
 *
 * 	-colors "{c1 z1 c2 z2} {c3 z3 c4 z4}..."   
 *			   Color ranges. 
 *
 * 	-colors "{c1 z1 c2 z2} {c3 z3 c4 z4}..."   
 *			   Color ranges. 
 *
 * 	-colors "{c1 z1% c2 z2%} {c3 z3 c4 z4}..."   
 *			   Color ranges. 
 *
 *	-opacity "z1 z2 z3 z3.."
 *	-opacity "{z1 o1} {z2 o2} {z3 o3}..."
 *	-opacity "{z1 o1 z2 o2} {z3 o3 z4 o4}..."
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColors(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,
    Tcl_Obj *objPtr,			/* Mode style string */
    char *record,			/* Cubicle structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    PaletteCmd *cmdPtr = (PaletteCmd *)record;
    Tcl_Obj **objv, **ev;
    int objc, ec;
    int result;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 0) {
	if (cmdPtr->colors != NULL) {
	    Blt_Free(cmdPtr->colors);
	    cmdPtr->colors = NULL;
	}
	cmdPtr->numColors = 0;
	return TCL_OK;
    }
    if (objc < 2) {
	Tcl_AppendResult(interp, 
		"too few elements: must have at least 2 color values",
		(char *)NULL);
	return TCL_ERROR;
    }
    /* Examine the first element in the list to determine how its formatted. */
    if (Tcl_ListObjGetElements(interp, objv[0], &ec, &ev) != TCL_OK) {
	return TCL_ERROR;
    }
    switch (ec) {
    case 1:
	result = ParseColors(interp, cmdPtr, objc, objv);
	break;
    case 2:
	result = ParseColorPoints(interp, cmdPtr, objc, objv);
	break;
    case 4:
	result = ParseColorRanges(interp, cmdPtr, objc, objv);
	break;
    default:
	Tcl_AppendResult(interp, "wrong # of elements in color specification",
		(char *)NULL);
	return TCL_ERROR;
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorsToObj --
 *
 *	Returns the palette style string based upon the mode flags.
 *
 * Results:
 *	The mode style string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ColorsToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    char *record,			/* Row/column structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    PaletteCmd *cmdPtr = (PaletteCmd *)record;
    Tcl_Obj *listObjPtr;
    int i;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 0; i < cmdPtr->numColors; i++) {
        Blt_PaletteEntry *entryPtr;
	Tcl_Obj *objPtr, *subListObjPtr;
	char string[200];

        entryPtr = cmdPtr->colors + i;
	subListObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	if (entryPtr->min.isAbsolute) {
	    objPtr = Tcl_NewDoubleObj(entryPtr->min.value);
	} else {
	    objPtr = Tcl_NewDoubleObj(entryPtr->min.value * 100.0);
	    Tcl_AppendToObj(objPtr, "%", 1);
	}
	Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);
	sprintf(string, "0x%x", entryPtr->low.u32);
	objPtr = Tcl_NewStringObj(string, -1);
	Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);
	objPtr = Tcl_NewDoubleObj(entryPtr->max.value);
	if (entryPtr->max.isAbsolute) {
	    objPtr = Tcl_NewDoubleObj(entryPtr->max.value);
	} else {
	    objPtr = Tcl_NewDoubleObj(entryPtr->max.value * 100.0);
	    Tcl_AppendToObj(objPtr, "%", 1);
	}
	Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);
	sprintf(string, "0x%x", entryPtr->high.u32);
	objPtr = Tcl_NewStringObj(string, -1);
	Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);
	Tcl_ListObjAppendElement(interp, listObjPtr, subListObjPtr);
    }	
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToOpacities --
 *
 *	Converts the -colors string into its numeric representation.
 *
 *	Valid color strings are:
 *
 *      -colors "c1 c2 c3 c4..."   Color ramp evenly spaced.
 *
 * 	-colors "{c1 z1} {c2 z2} {c3 z3}..."   
 *			   Color ramp with relative/absolute points. 
 *
 * 	-colors "{c1 z1 c2 z2} {c3 z3 c4 z4}..."   
 *			   Color ranges. 
 *
 * 	-colors "{c1 z1 c2 z2} {c3 z3 c4 z4}..."   
 *			   Color ranges. 
 *
 * 	-colors "{c1 z1% c2 z2%} {c3 z3 c4 z4}..."   
 *			   Color ranges. 
 *
 *	-opacity "z1 z2 z3 z3.."
 *	-opacity "{z1 o1} {z2 o2} {z3 o3}..."
 *	-opacity "{z1 o1 z2 o2} {z3 o3 z4 o4}..."
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToOpacities(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,
    Tcl_Obj *objPtr,			/* Opacity string */
    char *record,			/* PaletteCmd structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    PaletteCmd *cmdPtr = (PaletteCmd *)record;
    Tcl_Obj **objv, **ev;
    int objc, ec;
    int result;
    
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 0) {
	if (cmdPtr->opacities != NULL) {
	    Blt_Free(cmdPtr->opacities);
	    cmdPtr->opacities = NULL;
	}
	cmdPtr->numOpacities = 0;
	return TCL_OK;
    }
    if (objc < 2) {
	Tcl_AppendResult(interp, 
		"too few elements: must have at least 2 opacity values",
		(char *)NULL);
	return TCL_ERROR;
    }
    if (Tcl_ListObjGetElements(interp, objv[0], &ec, &ev) != TCL_OK) {
	return TCL_ERROR;
    }
    switch (ec) {
    case 1:
	result = ParseOpacities(interp, cmdPtr, objc, objv);
	break;
    case 2:
	result = ParseOpacityPoints(interp, cmdPtr, objc, objv);
	break;
    case 4:
	result = ParseOpacityRanges(interp, cmdPtr, objc, objv);
	break;
    default:
	Tcl_AppendResult(interp, "wrong # of elements in opacity specification",
		(char *)NULL);
	return TCL_ERROR;
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * OpacitiesToObj --
 *
 *	Returns the palette style string based upon the mode flags.
 *
 * Results:
 *	The mode style string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
OpacitiesToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    char *record,			/* PaletteCmd structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    PaletteCmd *cmdPtr = (PaletteCmd *)record;
    Tcl_Obj *listObjPtr;
    int i;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 0; i < cmdPtr->numOpacities; i++) {
        Blt_PaletteEntry *entryPtr;
	Tcl_Obj *objPtr, *subListObjPtr;
	double value;

        entryPtr = cmdPtr->opacities + i;
	subListObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	objPtr = Tcl_NewDoubleObj(entryPtr->min.value);
	if (entryPtr->min.isAbsolute) {
	    objPtr = Tcl_NewDoubleObj(entryPtr->min.value);
	} else {
	    objPtr = Tcl_NewDoubleObj(entryPtr->min.value * 100.0);
	    Tcl_AppendToObj(objPtr, "%", 1);
	}
	Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);
	value = entryPtr->low.Alpha / 255.0;
	objPtr = Tcl_NewDoubleObj(value);
	Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);
	if (entryPtr->max.isAbsolute) {
	    objPtr = Tcl_NewDoubleObj(entryPtr->max.value);
	} else {
	    objPtr = Tcl_NewDoubleObj(entryPtr->max.value * 100.0);
	    Tcl_AppendToObj(objPtr, "%", 1);
	}
	Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);
	value = entryPtr->high.Alpha / 255.0;
	objPtr = Tcl_NewDoubleObj(value);
	Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);
	Tcl_ListObjAppendElement(interp, listObjPtr, subListObjPtr);
    }	
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToFade --
 *
 *	Convert the string representation of opacity (a percentage) to
 *	an alpha value 0..255.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToFade(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,
    Tcl_Obj *objPtr,			/* Opacity string */
    char *record,			/* PaletteCmd structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    int *alphaPtr = (int *)(record + offset);
    double fade, opacity;

    if (Tcl_GetDoubleFromObj(interp, objPtr, &fade) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((fade < 0.0) || (fade > 100.0)) {
	Tcl_AppendResult(interp, "invalid percent opacity \"", 
		Tcl_GetString(objPtr), "\" should be 0 to 100", (char *)NULL);
	return TCL_ERROR;
    }
    opacity = (1.0 - (fade / 100.0)) * 255.0;
    *alphaPtr = ROUND(opacity);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FadeToObj --
 *
 *	Convert the string representation of opacity (a percentage) to
 *	an alpha value 0..255.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
FadeToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    char *record,			/* PaletteCmd structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    int *alphaPtr = (int *)(record + offset);
    double fade;

    fade = (1.0 - (*alphaPtr / 255.0)) * 100.0;
    return Tcl_NewDoubleObj(fade);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewPaletteCmd --
 *
 *	Creates a new palette command structure and inserts it into the
 *	interpreter's hash table.
 *
 *---------------------------------------------------------------------------
 */
static PaletteCmd *
NewPaletteCmd(Tcl_Interp *interp, PaletteCmdInterpData *dataPtr, 
	      const char *name)
{
    PaletteCmd *cmdPtr;
    Blt_HashEntry *hPtr;
    int isNew;

    cmdPtr = Blt_AssertCalloc(1, sizeof(PaletteCmd));
    hPtr = Blt_CreateHashEntry(&dataPtr->paletteTable, name, &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, "palette \"", name, "\" already exists",
			 (char *)NULL);
	return NULL;
    }
    cmdPtr->alpha = 0xFF;
    cmdPtr->name = Blt_GetHashKey(&dataPtr->paletteTable, hPtr);
    Blt_SetHashValue(hPtr, cmdPtr);
    cmdPtr->hashPtr = hPtr;
    cmdPtr->dataPtr = dataPtr;
    Blt_InitHashTable(&cmdPtr->notifierTable, BLT_ONE_WORD_KEYS);
    return cmdPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyPaletteCmd --
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyPaletteCmd(PaletteCmd *cmdPtr)
{
    if (cmdPtr->hashPtr != NULL) {
	NotifyClients(cmdPtr, PALETTE_DELETE_NOTIFY);
	Blt_DeleteHashEntry(&cmdPtr->dataPtr->paletteTable, cmdPtr->hashPtr);
    }
    Blt_FreeSwitches(paletteSpecs, (char *)cmdPtr, 0);
    Blt_DeleteHashTable(&cmdPtr->notifierTable);
    if (cmdPtr->colors != NULL) {
	Blt_Free(cmdPtr->colors);
    }
    if (cmdPtr->opacities != NULL) {
	Blt_Free(cmdPtr->opacities);
    }
    Blt_Free(cmdPtr);
}

static void
DestroyPaletteCmds(PaletteCmdInterpData *dataPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&dataPtr->paletteTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	PaletteCmd *cmdPtr;

	cmdPtr = Blt_GetHashValue(hPtr);
	cmdPtr->hashPtr = NULL;
	DestroyPaletteCmd(cmdPtr);
    }
}

static int
ConfigurePaletteCmd(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
	      Tcl_Obj *const *objv, int flags)
{
    if (Blt_ParseSwitches(interp, paletteSpecs, objc, objv, (char *)cmdPtr, 
	flags | BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    NotifyClients(cmdPtr, PALETTE_CHANGE_NOTIFY);
    return TCL_OK;
}

static unsigned int
ColorLerp(Blt_PaletteEntry *entryPtr, double t)
{
    Blt_Pixel color;
    int alpha, beta, t1, t2;
    int r, g, b, a;

    a = (int)(t * 255.0);
    alpha = CLAMP(a);
    if (alpha == 0xFF) {
	return entryPtr->high.u32;
    } else if (alpha == 0x00) {
	return entryPtr->low.u32;
    }
    beta = alpha ^ 0xFF;		/* beta = 1 - alpha */
    r = imul8x8(beta, entryPtr->low.Red, t1) + 
	imul8x8(alpha, entryPtr->high.Red, t2);
    g = imul8x8(beta, entryPtr->low.Green, t1) + 
	imul8x8(alpha, entryPtr->high.Green, t2);
    b = imul8x8(beta, entryPtr->low.Blue, t1) + 
	imul8x8(alpha, entryPtr->high.Blue, t2);
    a = imul8x8(beta, entryPtr->low.Alpha, t1) + 
	imul8x8(alpha, entryPtr->high.Alpha, t2);

    color.Red   = CLAMP(r);
    color.Green = CLAMP(g);
    color.Blue  = CLAMP(b);
    color.Alpha = CLAMP(a);
    color.Alpha = 0xFF;
    return color.u32;
}

static unsigned int
OpacityLerp(Blt_PaletteEntry *entryPtr, double t)
{
    int alpha, beta, t1, t2;
    int a;

    a = (int)(t * 255.0);
    alpha = CLAMP(a);
    if (alpha == 0xFF) {
	return entryPtr->high.Alpha;
    } else if (alpha == 0x00) {
	return entryPtr->low.Alpha;
    }
    beta = alpha ^ 0xFF;		/* beta = 1 - alpha */
    a = imul8x8(beta, entryPtr->low.Alpha, t1) + 
	imul8x8(alpha, entryPtr->high.Alpha, t2);
    return CLAMP(a);
}

/*
 *---------------------------------------------------------------------------
 *
 * NormalizePalette --
 *
 *      Converts all color and opacity entries with absolute values 
 *      to relative values 0.0 - 1.0 based upon the given range.
 *
 *---------------------------------------------------------------------------
 */
static void
NormalizePalette(PaletteCmd *cmdPtr, double min, double max)
{
    double range, scale;
    int i;
    
    range = max - min;
    scale = 1.0 / range;
    for (i = 0; i < cmdPtr->numColors; i++) {
        Blt_PaletteEntry *entryPtr;

        entryPtr = cmdPtr->colors + i;
	if (entryPtr->min.isAbsolute) {
	    entryPtr->min.norm = (entryPtr->min.value - min) * scale;
	} 
	if (entryPtr->max.isAbsolute) {
	    entryPtr->max.norm = (entryPtr->max.value - min) * scale;
	}
    }
    for (i = 0; i < cmdPtr->numOpacities; i++) {
        Blt_PaletteEntry *entryPtr;

        entryPtr = cmdPtr->opacities + i;
	if (entryPtr->min.isAbsolute) {
	    entryPtr->min.norm = (entryPtr->min.value - min) * scale;
	}
	if (entryPtr->max.isAbsolute) {
	    entryPtr->min.norm = (entryPtr->max.value - min) * scale;
	}
    }
}

static int 
Interpolate(PaletteCmd *cmdPtr, double norm, Blt_Pixel *colorPtr)
{
    int i;
    Blt_Pixel color;

    norm = RCLAMP(norm);
    color.u32 = 0x00;			/* Default to empty. */
    for (i = 0; i < cmdPtr->numColors; i++) {
        Blt_PaletteEntry *entryPtr;

        entryPtr = cmdPtr->colors + i;
	if (InRange(norm, entryPtr->min.norm, entryPtr->max.norm)){
	    double t;
	    
	    t = (norm - entryPtr->min.norm) / 
		(entryPtr->max.norm - entryPtr->min.norm);
	    color.u32 = ColorLerp(entryPtr, t);
	    break;
	}
    }
    if (i == cmdPtr->numColors) {
#ifndef notdef
	fprintf(stderr, "can't interpolate: norm=%.17g\n", norm);
#endif
	abort();
	return FALSE;
    }
    for (i = 0;  i < cmdPtr->numOpacities; i++) {
        Blt_PaletteEntry *entryPtr;

        entryPtr = cmdPtr->opacities + i;
	if (InRange(norm, entryPtr->min.norm, entryPtr->max.norm)){
	    double t;
            unsigned int alpha;

	    t = (norm - entryPtr->min.norm) / 
		(entryPtr->max.norm - entryPtr->min.norm);
	    alpha = OpacityLerp(entryPtr, t);
            Blt_FadeColor(&color, alpha);
	    break;
	}
    }
    *colorPtr = color;
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * DefaultPalettes --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DefaultPalettes(Tcl_Interp *interp, PaletteCmdInterpData *dataPtr)
{
    static char cmd[] = "source [file join $blt_library palette.tcl]";

    if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
	char info[200];

	Blt_FormatString(info, 200, "\n    (while loading palettes)");
	Tcl_AddErrorInfo(interp, info);
	Tcl_BackgroundError(interp);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *	blt::palette cget $name -option
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    PaletteCmd *cmdPtr;

    if (GetPaletteCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named palette. */
    }
    return Blt_SwitchValue(interp, paletteSpecs, (char *)cmdPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *	.blt::palette configure $name ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    PaletteCmd *cmdPtr;

    if (GetPaletteCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named palette. */
    }
    if (objc == 3) {
	return Blt_SwitchInfo(interp, paletteSpecs, cmdPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
	return Blt_SwitchInfo(interp, paletteSpecs, cmdPtr, objv[3], 0);
    }
    if (ConfigurePaletteCmd(interp, cmdPtr, objc - 3, objv + 3, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    NotifyClients(cmdPtr, PALETTE_CHANGE_NOTIFY);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *	Creates a palette.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter result will
 *	contain a TCL list of the element names.
 *
 *	.g palette create ?$name? ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    PaletteCmd *cmdPtr;
    const char *name;
    char ident[200];
    const char *string;

    name = NULL;
    if (objc > 2) {
	string = Tcl_GetString(objv[2]);
        if (Blt_FindHashEntry(&dataPtr->paletteTable, string) != NULL) {
            Tcl_AppendResult(interp, "palette \"", string, 
                             "\" already exists", (char *)NULL);
            return TCL_ERROR;
        }
        name = string;
        objc--, objv++;
    }
    /* If no name was given for the marker, make up one. */
    if (name == NULL) {
        do {
            Blt_FormatString(ident, 200, "palette%d",
                             dataPtr->nextPaletteCmdId++);
        } while (Blt_FindHashEntry(&dataPtr->paletteTable, ident));
        name = ident;
    }
    cmdPtr = NewPaletteCmd(interp, dataPtr, name);
    if (cmdPtr == NULL) {
	return TCL_ERROR;
    }
    if (ConfigurePaletteCmd(interp, cmdPtr, objc - 2, objv + 2, 0) != TCL_OK) {
	DestroyPaletteCmd(cmdPtr);
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), cmdPtr->name, -1);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *	Deletes one or more palettees from the graph.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter result will
 *	contain a TCL list of the element names.
 *
 *	blt::palette delete $name...
 *
 *---------------------------------------------------------------------------
 */
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
	PaletteCmd *cmdPtr;

	if (GetPaletteCmdFromObj(interp, dataPtr, objv[i], &cmdPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	DestroyPaletteCmd(cmdPtr);
    }
    return TCL_OK;
}    


/*
 *---------------------------------------------------------------------------
 *
 * DrawOp --
 *
 *	Draws the palette onto a picture.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the element names.
 *
 *	blt::palette draw $name $picture
 *
 *---------------------------------------------------------------------------
 */
static int
DrawOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    PaletteCmd *cmdPtr;
    Blt_Picture picture;
    int w, h;

    if (GetPaletteCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named palette. */
    }
    if (Blt_GetPictureFromObj(interp, objv[3], &picture) != TCL_OK) {
	return TCL_ERROR;
    }
    w = Blt_Picture_Width(picture);
    h = Blt_Picture_Height(picture);
    if (w > h) {
	int x;

	for (x = 0; x < w; x++) {
	    double value;
	    int y;
	    Blt_Pixel color;

	    value = ((double)x / (double)(w-1));
	    Interpolate(cmdPtr, value, &color);
	    /* Draw band. */
	    for (y = 0; y < h; y++) {
		Blt_Pixel *dp;

		dp = Blt_Picture_Pixel(picture, x, y);
		dp->u32 = color.u32;
	    }
	}
    } else {
	int y;

	for (y = 0; y < h; y++) {
	    int x;
	    double value;
	    Blt_Pixel color;

	    value = ((double)h / (double)(h-1));
	    Interpolate(cmdPtr, value, &color);
	    /* Draw band. */
	    for (x = 0; x < w; x++) {
		Blt_Pixel *dp;

		dp = Blt_Picture_Pixel(picture, x, y);
		dp->u32 = color.u32;
	    }
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *	Indicates if a palette by the given name exists in the element.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter result will
 *	contain a TCL list of the element names.
 *
 *	blt::palette exists $name
 *
 *---------------------------------------------------------------------------
 */
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    PaletteCmd *cmdPtr;
    int bool;

    bool = (GetPaletteCmdFromObj(NULL, dataPtr, objv[2], &cmdPtr) == TCL_OK);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * InterpolateOp --
 *
 *	Computes the interpolated color value from the value given.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter result will
 *	contain a TCL list of the element names.
 *
 *	blt::palette interpolate $name value
 *
 *---------------------------------------------------------------------------
 */
static int
InterpolateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    PaletteCmd *cmdPtr;
    Tcl_Obj *listObjPtr;
    InterpolateSwitches switches;
    double norm;
    Blt_Pixel color;
    Blt_PalettePoint point;

    if (GetPaletteCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetPointFromObj(interp, objv[3], &point) != TCL_OK) {
	return TCL_ERROR;
    }
    switches.min = 0.0;
    switches.max = 1.0;
    /* Process switches  */
    if (Blt_ParseSwitches(interp, interpolateSwitches, objc - 4, objv + 4, 
		&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (point.isAbsolute) {
	norm = (point.value - switches.min) / (switches.max - switches.min);
    } else {
	norm = point.value;
    }
    NormalizePalette(cmdPtr, switches.min, switches.max);
    if (!Interpolate(cmdPtr, norm, &color)) {
	Tcl_AppendResult(interp, "value \"", Tcl_GetString(objv[3]), 
		"\" not in any range", (char *)NULL);
	return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(color.Alpha));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(color.Red));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(color.Green));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(color.Blue));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *	Returns the names of the palette in the graph matching one of more
 *	patterns provided.  If no pattern arguments are given, then all
 *	palette names will be returned.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter result will
 *	contain a TCL list of the element names.
 *
 *	blt::palette names $pattern
 *
 *---------------------------------------------------------------------------
 */
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    PaletteCmdInterpData *dataPtr = clientData;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 2) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&dataPtr->paletteTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    PaletteCmd *cmdPtr;
	    Tcl_Obj *objPtr;

	    cmdPtr = Blt_GetHashValue(hPtr);
	    objPtr = Tcl_NewStringObj(cmdPtr->name, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    } else {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&dataPtr->paletteTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    PaletteCmd *cmdPtr;
	    int i;

	    cmdPtr = Blt_GetHashValue(hPtr);
	    for (i = 2; i < objc; i++) {
		const char *pattern;

		pattern = Tcl_GetString(objv[i]);
		if (Tcl_StringMatch(cmdPtr->name, pattern)) {
		    Tcl_Obj *objPtr;

		    objPtr = Tcl_NewStringObj(cmdPtr->name, -1);
		    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
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
 * PaletteCmd --
 *
 *	blt::palette cget $name -x 
 *	blt::palette configure $name -colors {} -opacity {}
 *	blt::palette create ?name? -colors {} -opacity {}
 *	blt::palette delete $name
 *	blt::palette exists $name
 *	blt::palette names ?pattern?
 *	blt::palette interpolate $name $value
 *	blt::palette draw $name $picture
 *	blt::palette ranges $name
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec paletteOps[] = {
    {"cget",        2, CgetOp,        4, 4, "name option",},
    {"configure",   2, ConfigureOp,   3, 0, "name ?option value?...",},
    {"create",      2, CreateOp,      2, 0, "?name? ?option value?...",},
    {"delete",      2, DeleteOp,      2, 0, "?name?...",},
    {"draw",        2, DrawOp,        4, 4, "name picture",},
    {"exists",      1, ExistsOp,      3, 3, "name",},
    {"interpolate", 1, InterpolateOp, 4, 0, "name value ?switches?",},
    {"names",       1, NamesOp,       2, 0, "?pattern?...",},
};
static int numPaletteOps = sizeof(paletteOps) / sizeof(Blt_OpSpec);

static int
PaletteObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numPaletteOps, paletteOps, BLT_OP_ARG1, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * PaletteInterpDeleteProc --
 *
 *	This is called when the interpreter registering the "contourpalette"
 *	command is deleted.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Removes the hash table managing all table names.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
PaletteInterpDeleteProc(ClientData clientData, Tcl_Interp *interp)
{
    PaletteCmdInterpData *dataPtr = clientData;

    /* All table instances should already have been destroyed when their
     * respective TCL commands were deleted. */
    DestroyPaletteCmds(dataPtr);
    Blt_DeleteHashTable(&dataPtr->paletteTable);
    Tcl_DeleteAssocData(interp, PALETTE_THREAD_KEY);
    Blt_Free(dataPtr);
}

/*
 *
 * GetPaletteCmdInterpData --
 *
 */
static PaletteCmdInterpData *
GetPaletteCmdInterpData(Tcl_Interp *interp)
{
    PaletteCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (PaletteCmdInterpData *)
	Tcl_GetAssocData(interp, PALETTE_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
	dataPtr = Blt_AssertMalloc(sizeof(PaletteCmdInterpData));
	dataPtr->interp = interp;
	Tcl_SetAssocData(interp, PALETTE_THREAD_KEY, PaletteInterpDeleteProc, 
		dataPtr);
	Blt_InitHashTable(&dataPtr->paletteTable, BLT_STRING_KEYS);
	dataPtr->nextPaletteCmdId = 0;
    }
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PaletteCmdInitProc --
 *
 *	This procedure is invoked to initialize the "contourpalette" command.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Creates the new command and adds a new entry into a global Tcl
 *	associative array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_PaletteCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "palette", PaletteObjCmd, };

    cmdSpec.clientData = GetPaletteCmdInterpData(interp);
    if (Blt_InitCmd(interp, "::blt", &cmdSpec) != TCL_OK) {
	return TCL_ERROR;
    }
    return DefaultPalettes(interp, cmdSpec.clientData);
}


int
Blt_Palette_GetFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
		       Blt_Palette *palPtr)
{
    PaletteCmdInterpData *dataPtr;
    PaletteCmd *cmdPtr;

    dataPtr = GetPaletteCmdInterpData(interp);
    if (GetPaletteCmdFromObj(interp, dataPtr, objPtr, &cmdPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    *palPtr = (Blt_Palette)cmdPtr;
    return TCL_OK;
}

int
Blt_Palette_GetFromString(Tcl_Interp *interp, const char *string, 
			  Blt_Palette *palPtr)
{
    PaletteCmdInterpData *dataPtr;
    PaletteCmd *cmdPtr;

    dataPtr = GetPaletteCmdInterpData(interp);
    if (GetPaletteCmd(interp, dataPtr, string, &cmdPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    *palPtr = (Blt_Palette)cmdPtr;
    return TCL_OK;
}

void
Blt_Palette_SetRange(Blt_Palette palette, double min, double max)
{
    PaletteCmd *cmdPtr = (PaletteCmd *)palette;

    NormalizePalette(cmdPtr, min, max);
}

int
Blt_Palette_GetAssociatedColorFromAbsoluteValue(
    Blt_Palette palette, double value, double min, double max)
{
    Blt_Pixel color;
    PaletteCmd *cmdPtr = (PaletteCmd *)palette;
    double norm;

    norm = (value - min) / (max - min);
    if (!Interpolate(cmdPtr, norm, &color)) {
	color.u32 = 0x00;
    } 
    Blt_FadeColor(&color, cmdPtr->alpha);
    return color.u32;
}

int
Blt_Palette_GetAssociatedColor(Blt_Palette palette, double norm)
{
    Blt_Pixel color;
    PaletteCmd *cmdPtr = (PaletteCmd *)palette;

    if (!Interpolate(cmdPtr, norm, &color)) {
	color.u32 = 0x00;
    } 
    Blt_FadeColor(&color, cmdPtr->alpha);
    return color.u32;
}

void
Blt_Palette_CreateNotifier(Blt_Palette palette, Blt_Palette_NotifyProc *proc, 
			   ClientData clientData)
{
    Blt_HashEntry *hPtr;
    int isNew;
    PaletteCmd *cmdPtr = (PaletteCmd *)palette;

    hPtr = Blt_CreateHashEntry(&cmdPtr->notifierTable, clientData, &isNew);
    Blt_SetHashValue(hPtr, proc);
}

void
Blt_Palette_DeleteNotifier(Blt_Palette palette, ClientData clientData)
{
    Blt_HashEntry *hPtr;
    PaletteCmd *cmdPtr = (PaletteCmd *)palette;

    hPtr = Blt_FindHashEntry(&cmdPtr->notifierTable, clientData);
    Blt_DeleteHashEntry(&cmdPtr->notifierTable, hPtr);
}

const char *
Blt_Palette_Name(Blt_Palette palette)
{
    PaletteCmd *cmdPtr = (PaletteCmd *)palette;

    return cmdPtr->name;
}

Blt_Palette
Blt_Palette_TwoColorPalette(int low, int high)
{
    struct _Blt_Palette *palPtr;

    palPtr = Blt_AssertCalloc(1, sizeof(struct _Blt_Palette));
    palPtr->colors = Blt_AssertMalloc(sizeof(Blt_PaletteEntry));
    palPtr->colors[0].low.u32 = low;
    palPtr->colors[0].high.u32 = high;
    palPtr->colors[0].min.isAbsolute = FALSE;
    palPtr->colors[0].max.isAbsolute = FALSE;
    palPtr->colors[0].min.value = 0.0;
    palPtr->colors[0].max.value = 1.0;
    palPtr->numColors = 1;
    return palPtr;
}

void
Blt_Palette_Free(Blt_Palette palette)
{
    struct _Blt_Palette *palPtr = (struct _Blt_Palette *)palette;

    if (palPtr->colors != NULL) {
	Blt_Free(palPtr->colors);
    }
    if (palPtr->opacities != NULL) {
	Blt_Free(palPtr->opacities);
    }
    Blt_Free(palPtr);
}


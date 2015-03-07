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

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "bltMath.h"
#include "bltPicture.h"
#include "bltPalette.h"

#define PALETTE_THREAD_KEY "BLT Palette Command Interface"
#define RCLAMP(c)	((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))

#define imul8x8(a,b,t)	((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))
#define CLAMP(c)	((((c) < 0) ? 0 : ((c) > 255) ? 255 : (c)))

#define LOADED  (1<<0)

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

/*
 *---------------------------------------------------------------------------
 *
 * PaletteStep --
 *
 *      Represents a point in the palette.  The value of the point may be
 *	relative (between 0 and 1) or absolute.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    double value;
    double norm;                        /* Normalized values 0..1 */
    int isAbsolute;			/* Indicates if the value is
					 * relative (normalized) or an
					 * absolute value. */
} PaletteStep;

/*
 *---------------------------------------------------------------------------
 *
 * PaletteInterval --
 *
 *      Represents an interval in the palette.  The interval is represented
 *	by its minimum and maximum values and the low and high colors to
 *	interpolate between.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    Blt_Pixel low, high;		/* Range of colors. */
    PaletteStep min, max;		/* Range of z values. */
} PaletteInterval;


typedef struct _Blt_Palette {
    unsigned int flags;
    PaletteInterval *colors;		/* Array of color ranges. */
    PaletteInterval *opacities;         /* Array of opacity ranges. */
    double colorMin, colorMax;
    double dummy;
    double min, max;
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
    Tcl_Obj *colorFileObjPtr;           /* Name of file to use for color
                                         * data. */
    Tcl_Obj *colorDataObjPtr;           /* String to use for color data. */
    Tcl_Obj *opacityFileObjPtr;         /* Name of file to use for opacity
                                         * data. */
    Tcl_Obj *opacityDataObjPtr;         /* String to use for opacity
                                         * data. */
    unsigned int colorFlags;
    unsigned int opacityFlags;
} PaletteCmd;

static Blt_SwitchParseProc ObjToSpacing;
static Blt_SwitchCustom spacingSwitch =
{
    ObjToSpacing, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToColorFormat;
static Blt_SwitchCustom colorFormatSwitch =
{
    ObjToColorFormat, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToFade;
static Blt_SwitchCustom fadeSwitch =
{
    ObjToFade, NULL, (ClientData)0
};

#define SPACING_REGULAR         (1<<0)
#define SPACING_IRREGULAR       (1<<1)
#define SPACING_INTERVAL        (1<<2)
#define SPACING_MASK         \
        (SPACING_REGULAR|SPACING_IRREGULAR|SPACING_INTERVAL)
#define COLOR_NAME             (1<<3)
#define COLOR_RGB              (1<<4)
#define COLOR_HSV              (1<<5)
#define COLOR_MASK             (COLOR_RGB|COLOR_NAME|COLOR_HSV)

static Blt_SwitchSpec paletteSpecs[] =
{
    {BLT_SWITCH_OBJ, "-cdata", "string", (char *)NULL, 
	Blt_Offset(PaletteCmd, colorDataObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-cfile", "file", (char *)NULL, 
	Blt_Offset(PaletteCmd, colorFileObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-colordata", "string", (char *)NULL, 
	Blt_Offset(PaletteCmd, colorDataObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-colorfile", "file", (char *)NULL, 
	Blt_Offset(PaletteCmd, colorFileObjPtr), 0, 0},
    {BLT_SWITCH_CUSTOM, "-colorformat", "rgb|name|hsv", (char *)NULL, 
	Blt_Offset(PaletteCmd, colorFlags), 0, 0, &colorFormatSwitch},
    {BLT_SWITCH_CUSTOM, "-colorspacing", "regular|irregular|interval",
        (char *)NULL, Blt_Offset(PaletteCmd, colorFlags), 0, 0,
        &spacingSwitch},
    {BLT_SWITCH_CUSTOM, "-fade", "0-100", (char *)NULL,
        Blt_Offset(PaletteCmd, alpha), 0, 0, &fadeSwitch},
    {BLT_SWITCH_DOUBLE, "-max", "value", (char *)NULL, 
	Blt_Offset(PaletteCmd, dummy),            0, 0},
    {BLT_SWITCH_DOUBLE, "-min", "value", (char *)NULL, 
	Blt_Offset(PaletteCmd, colorMin),            0, 0},
    {BLT_SWITCH_OBJ, "-odata", "string", (char *)NULL, 
	Blt_Offset(PaletteCmd, opacityDataObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-ofile", "file", (char *)NULL, 
	Blt_Offset(PaletteCmd, opacityFileObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-opacityfile", "file", (char *)NULL, 
	Blt_Offset(PaletteCmd, opacityFileObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-opacitydata", "string", (char *)NULL, 
	Blt_Offset(PaletteCmd, opacityDataObjPtr), 0, 0},
    {BLT_SWITCH_CUSTOM, "-opacityspacing", "regular|irregular|interval",
        (char *)NULL, Blt_Offset(PaletteCmd, opacityFlags), 0, 0,
        &spacingSwitch},
    {BLT_SWITCH_END}
};

static PaletteCmdInterpData *GetPaletteCmdInterpData(Tcl_Interp *interp);

static int LoadData(Tcl_Interp *interp, PaletteCmd *cmdPtr);

typedef int (GetColorProc)(Tcl_Interp *interp, PaletteCmd *cmdPtr,
                           Tcl_Obj **objv, Blt_Pixel *colorPtr);

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

#define SetColor(c,r,g,b) ((c)->Red = (int)((r) * 255.0), \
			   (c)->Green = (int)((g) * 255.0), \
			   (c)->Blue = (int)((b) * 255.0))

static void
HSVToPixel(double hue, double sat, double val, Blt_Pixel *colorPtr)
{
    double xhue, p, q, t;
    double frac;
    int quadrant;

    if (val < 0.0) {
	val = 0.0;
    } else if (val > 1.0) {
	val = 1.0;
    }
    if (sat == 0.0) {
	SetColor(colorPtr, val, val, val);
	return;
    }
    xhue = FMOD(hue, 360.0) / 60.0;
    quadrant = (int)floor(xhue);
    frac = hue - quadrant;
    p = val * (1 - (sat));
    q = val * (1 - (sat * frac));
    t = val * (1 - (sat * (1 - frac)));

    switch (quadrant) {
    case 0:
	SetColor(colorPtr, val, t, p);
	break;
    case 1:
	SetColor(colorPtr, q, val, p);
	break;
    case 2:
	SetColor(colorPtr, p, val, t);
	break;
    case 3:
	SetColor(colorPtr, p, q, val);
	break;
    case 4:
	SetColor(colorPtr, t, p, val);
	break;
    case 5:
	SetColor(colorPtr, val, p, q);
	break;
    }
}

static void
PrintEntries(size_t numEntries, PaletteInterval *entries)
{
    int i;
    
    for (i = 0; i < numEntries; i++) {
	PaletteInterval *entryPtr;

	entryPtr = entries + i;
        fprintf(stderr, "entry %d: min (value=%g norm=%g) max (value=%g norm=%g)\n",
                i, entryPtr->min.value, entryPtr->min.norm,
                entryPtr->max.value, entryPtr->max.norm);
    }
}

static int 
CompareEntries(const void *a, const void *b)
{
    const PaletteInterval *e1Ptr, *e2Ptr;

    e1Ptr = a;
    e2Ptr = b;
    if (e1Ptr->min.norm < e2Ptr->min.norm) {
        return -1;
    } else if (e1Ptr->min.norm > e2Ptr->min.norm) {
        return 1;
    }
    return 0;
}

static void
SortEntries(size_t numEntries, PaletteInterval *entries)
{
    qsort((char *)entries, numEntries, sizeof(PaletteInterval),
          CompareEntries);
}

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
GetStepFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, PaletteStep *stepPtr)
{
    char *p, *string;

    string = Tcl_GetString(objPtr);
    p = strchr(string, '%');
    if (p == NULL) {
	if (Blt_GetDoubleFromObj(interp, objPtr, &stepPtr->value) != TCL_OK) {
            return TCL_ERROR;
	}
	stepPtr->isAbsolute = TRUE;
	stepPtr->norm = -1.0;
    } else {
	double value;
	int result;

	*p = '\0';
	result = Blt_GetDoubleFromString(interp, string, &value);
	*p = '%';
	if (result != TCL_OK) {
            return TCL_ERROR;
	}
	if ((value < 0.0) || (value > 100.0)) {
            return TCL_ERROR;
	}
	stepPtr->isAbsolute = FALSE;
	stepPtr->value = stepPtr->norm = value * 0.01;
    }
    return TCL_OK;
}

static int
GetOpacity(Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Pixel *pixelPtr)
{
    double value;

    if (Blt_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((value < 0.0) || (value > 1.0)) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "bad opacity value \"",
                Tcl_GetString(objPtr), "\": should be 0.0 - 1.0",
		(char *)NULL);
        }
	return TCL_ERROR;
    }
    pixelPtr->u32 = 0;
    pixelPtr->Alpha = (int)(value * 255.0);
    return TCL_OK;
}

static int
AutoTestRGBs(Tcl_Interp *interp, PaletteCmd *cmdPtr, int numComponents,
             int objc, Tcl_Obj **objv)
{
    int i;
    double max;
    
    max = 0.0;
    switch (numComponents) {
    case 3:                             /* Regular RGB triplets */
        for (i = 0; i < objc; i++) {
            double x;
            
            if (Blt_GetDoubleFromObj(interp, objv[i], &x) != TCL_OK) {
                return TCL_ERROR;
            }
            if (x > max) {
                max = x;
            }
        }
        break;
    case 4:                             /* Irregular RGB triplets */
        for (i = 0; i < objc; i += 4) {
            int j;
            
            for (j = 1; j < 4; j++) {
                double x;
                if (Blt_GetDoubleFromObj(interp, objv[i+j], &x) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (x > max) {
                    max = x;
                }
            }
        }
        break;
    case 8:                             /* Interval RGB triplets  */
        for (i = 0; i < objc; i += 8) {
            int j;
            
            for (j = 1; j < 4; j++) {
                double x;

                if (Blt_GetDoubleFromObj(interp, objv[i+j], &x) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (x > max) {
                    max = x;
                }
            }
            for (j = 5; j < 8; j++) {
                double x;

                if (Blt_GetDoubleFromObj(interp, objv[i+j], &x) != TCL_OK) {
                    return TCL_ERROR;
                }
                if (x > max) {
                    max = x;
                }
            }
        }
        break;
    }
     
    if (max > 255.0) {
        cmdPtr->colorMax = 65535;
    } else if (max > 1.0) {
        cmdPtr->colorMax = 255;
    } else {
        cmdPtr->colorMax = 1.0;
    }
    return TCL_OK;
}

static int
GetRGBFromObjv(Tcl_Interp *interp, PaletteCmd *cmdPtr, Tcl_Obj **objv,
               Blt_Pixel *colorPtr) 
{
    double r, g, b, range;
    Blt_Pixel color;

    if (Blt_GetDoubleFromObj(interp, objv[0], &r) != TCL_OK) {
	return TCL_ERROR;
    }
    if (r < cmdPtr->colorMin) { 
	r = cmdPtr->colorMin;
    } else if (r > cmdPtr->colorMax) {
	r = cmdPtr->colorMax;
    }
    if (Blt_GetDoubleFromObj(interp, objv[1], &g) != TCL_OK) {
	return TCL_ERROR;
    }
    if (g < cmdPtr->colorMin) { 
	g = cmdPtr->colorMin;
    } else if (g > cmdPtr->colorMax) {
	g = cmdPtr->colorMax;
    }
    if (Blt_GetDoubleFromObj(interp, objv[2], &b) != TCL_OK) {
	return TCL_ERROR;
    }
    if (b < cmdPtr->colorMin) { 
	b = cmdPtr->colorMin;
    } else if (b > cmdPtr->colorMax) {
	b = cmdPtr->colorMax;
    }
    range = cmdPtr->colorMax - cmdPtr->colorMin;
    color.Red   = (int)(((r - cmdPtr->colorMin) / range) * 255.0);
    color.Green = (int)(((g - cmdPtr->colorMin) / range) * 255.0);
    color.Blue  = (int)(((b - cmdPtr->colorMin) / range) * 255.0);
    color.Alpha = 0xFF;
    colorPtr->u32 = color.u32;
    return TCL_OK;
}

static int
GetHSVFromObjv(Tcl_Interp *interp, PaletteCmd *cmdPtr, Tcl_Obj **objv,
               Blt_Pixel *colorPtr) 
{
    double h, s, v;

    if (Blt_GetDoubleFromObj(interp, objv[0], &h) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_GetDoubleFromObj(interp, objv[1], &s) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_GetDoubleFromObj(interp, objv[2], &v) != TCL_OK) {
	return TCL_ERROR;
    }
    HSVToPixel(h, s, v, colorPtr);
    colorPtr->Alpha = 0xFF;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSpacing --
 *
 *	Converts the Tcl_Obj into its spacing flag.
 *
 *	Valid spacing values are:
 *
 *      -spacing regular                Uniform spacing of entries.  Each
 *                                      entry consists of 1 or 3 color values.
 *      -spacing irregular              Nonuniform spacing of entries. Each
 *                                      entry will also contain a value 
 *                                      (absolute or relative) indicating
 *                                      the location of entry.
 *      -spacing interval               Each entry describes an interval
 *                                      consisting of the min value and
 *                                      associated color and the max value
 *                                      and its associate color.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToSpacing(ClientData clientData, Tcl_Interp *interp, const char *switchName,
             Tcl_Obj *objPtr, char *record, int offset, int flags)	
{
    unsigned int *flagsPtr = (unsigned int *)(record + offset);
    char c;
    const char *string;
    unsigned int flag;
    
    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'r') && (strcmp(string, "regular") == 0)) {
        flag = SPACING_REGULAR;
    } else if ((c == 'i') && (strcmp(string, "irregular") == 0)) {
        flag = SPACING_IRREGULAR;
    } else if ((c == 'i') && (strcmp(string, "interval") == 0)) {
        flag = SPACING_INTERVAL;
    } else {
	Tcl_AppendResult(interp, "bad spacing value \"", string,
                         "\": should be regular, irregular, or interval.",
                         (char *)NULL);
	return TCL_ERROR;
    }
    *flagsPtr &= ~SPACING_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToColorFormat --
 *
 *	Converts the Tcl_Obj into its color format flag.
 *
 *	Valid color format values are:
 *
 *      -colorformat name               Each color is specified by a single
 *                                      color name or #hex string.
 *      -colorformat rgb                Each color is a triplet of red, green,
 *                                      and blue values (0..1).
 *      -colorformat hsv                Each color is a triplet of hue, sat,
 *                                      and val (0..1).
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColorFormat(ClientData clientData, Tcl_Interp *interp,
                const char *switchName, Tcl_Obj *objPtr, char *record,
                int offset, int flags)	
{
    unsigned int *flagsPtr = (unsigned int *)(record + offset);
    char c;
    const char *string;
    unsigned int flag;
    
    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'r') && (strcmp(string, "rgb") == 0)) {
        flag = COLOR_RGB;
    } else if ((c == 'n') && (strcmp(string, "name") == 0)) {
        flag = COLOR_NAME;
    } else if ((c == 'h') && (strcmp(string, "hsv") == 0)) {
        flag = COLOR_HSV;
    } else {
	Tcl_AppendResult(interp, "bad color format value \"", string,
                         "\": should be rgb or name.",
                         (char *)NULL);
	return TCL_ERROR;
    }
    *flagsPtr &= ~COLOR_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
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
    cmdPtr->colorFlags = COLOR_RGB | SPACING_REGULAR;
    cmdPtr->opacityFlags = SPACING_REGULAR;
    cmdPtr->alpha = 0xFF;
    cmdPtr->name = Blt_GetHashKey(&dataPtr->paletteTable, hPtr);
    Blt_SetHashValue(hPtr, cmdPtr);
    cmdPtr->colorMin = 0.0;
    cmdPtr->colorMax = 1.0;
    cmdPtr->min = 0.0;
    cmdPtr->max = 1.0;
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

/*
 *	"c1 c2 c3 c4.."
 */
static int
ParseRegularColorNames(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
                      Tcl_Obj **objv)
{
    PaletteInterval *entries;
    double step;
    int i, numEntries;

    numEntries = objc - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    step = 1.0 / numEntries;
    for (i = 0; i < numEntries; i++) {
	PaletteInterval *entryPtr;
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

/*
 *	"r g b  r b g  r g b"...
 */
static int
ParseRegularColorTriplets(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
                          Tcl_Obj **objv)
{
    PaletteInterval *entries;
    double step;
    int i, j, numEntries;
    GetColorProc *proc;

    proc = (cmdPtr->colorFlags & COLOR_RGB) ? GetRGBFromObjv : GetHSVFromObjv;
    numEntries = (objc / 3) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    step = 1.0 / numEntries;
    for (i = j = 0; j < numEntries; i += 3, j++) {
	PaletteInterval *entryPtr;

	entryPtr = entries + j;
	if (((*proc)(interp, cmdPtr, objv+i, &entryPtr->low) != TCL_OK) ||
            ((*proc)(interp, cmdPtr, objv+(i+3), &entryPtr->high) != TCL_OK)) {
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
 *	"0.0 red 0.4 green 8.2 blue..."
 */
static int
ParseIrregularColorNames(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
                         Tcl_Obj **objv)
{
    PaletteInterval *entries, *entryPtr;
    int i, numEntries;
    Blt_Pixel low;
    PaletteStep min;

    numEntries = (objc / 2) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    if (GetStepFromObj(interp, objv[0], &min) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_GetPixelFromObj(interp, objv[1], &low) != TCL_OK) {
        return TCL_ERROR;
    }
    for (entryPtr = entries, i = 2; i < objc; i += 2, entryPtr++) {
        Blt_Pixel high;
        PaletteStep max;
        
        if (GetStepFromObj(interp, objv[i], &max) != TCL_OK) {
            goto error;
        }
        if (Blt_GetPixelFromObj(interp, objv[i+1], &high) != TCL_OK) {
            goto error;
        }
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
	entryPtr->min = min;
	entryPtr->max = max;

        low.u32 = high.u32;
        min = max;
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
 *	"0.0 0.1 0.4 0.2 0.1 0.1 0.2 0.3"...
 */
static int
ParseIrregularColorTriplets(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
                            Tcl_Obj **objv)
{
    PaletteInterval *entries, *entryPtr;
    int i, numEntries;
    Blt_Pixel low;
    PaletteStep min;
    GetColorProc *proc;

    proc = (cmdPtr->colorFlags & COLOR_RGB) ? GetRGBFromObjv : GetHSVFromObjv;
    numEntries = (objc / 4) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    if (GetStepFromObj(interp, objv[0], &min) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((*proc)(interp, cmdPtr, objv + 1, &low) != TCL_OK) {
        return TCL_ERROR;
    }
    for (entryPtr = entries, i = 4; i < objc; i += 4, entryPtr++) {
        Blt_Pixel high;
        PaletteStep max;
        
        if (GetStepFromObj(interp, objv[i], &max) != TCL_OK) {
            goto error;
        }
        if ((*proc)(interp, cmdPtr, objv + i + 1, &high) != TCL_OK) {
            goto error;
        }
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
	entryPtr->min = min;
	entryPtr->max = max;

        low.u32 = high.u32;
        min = max;
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
 *	"0.0 red 0.4 green 0.4 blue 0.5 violet..."
 */
static int
ParseIntervalColorNames(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
                        Tcl_Obj **objv)
{
    PaletteInterval *entries, *entryPtr;
    int i, numEntries;

    numEntries = (objc / 4) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    for (entryPtr = entries, i = 0; i < objc; i += 4, entryPtr++) {
        Blt_Pixel low, high;
        PaletteStep min, max;
        
        if (GetStepFromObj(interp, objv[i], &min) != TCL_OK) {
            goto error;
        }
        if (Blt_GetPixelFromObj(interp, objv[i+1], &low) != TCL_OK) {
            goto error;
        }
        if (GetStepFromObj(interp, objv[i+2], &max) != TCL_OK) {
            goto error;
        }
        if (Blt_GetPixelFromObj(interp, objv[i+3], &high) != TCL_OK) {
            goto error;
        }
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
	entryPtr->min = min;
	entryPtr->max = max;
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
 *	"0.0 0.1 0.1 0.1 0.4 green 0.4 blue 0.5 violet..."
 */
static int
ParseIntervalColorTriplets(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
                           Tcl_Obj **objv)
{
    PaletteInterval *entries, *entryPtr;
    int i, numEntries;
    GetColorProc *proc;

    proc = (cmdPtr->colorFlags & COLOR_RGB) ? GetRGBFromObjv : GetHSVFromObjv;
    numEntries = (objc / 8) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    for (entryPtr = entries, i = 0; i < objc; i += 8, entryPtr++) {
        Blt_Pixel low, high;
        PaletteStep min, max;
        
        if (GetStepFromObj(interp, objv[i], &min) != TCL_OK) {
            goto error;
        }
        if ((*proc)(interp, cmdPtr, objv + i + 1,  &low) != TCL_OK) {
            goto error;
        }
        if (GetStepFromObj(interp, objv[i + 4], &max) != TCL_OK) {
            goto error;
        }
        if ((*proc)(interp, cmdPtr, objv + i + 5, &high) != TCL_OK) {
            goto error;
        }
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
	entryPtr->min = min;
	entryPtr->max = max;
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
 */
static int
ParseColorData(Tcl_Interp *interp, PaletteCmd *cmdPtr, Tcl_Obj *objPtr)
{
    Tcl_Obj **objv;
    int objc, result, numComponents;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 0) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "no color component data", (char *)NULL);
        }
        return TCL_ERROR;
    }
    numComponents = 0;
    if ((cmdPtr->colorFlags & SPACING_REGULAR) == 0)  {
        numComponents++;                /* Add 1 component for value.  */
    }
    if (cmdPtr->colorFlags & (COLOR_RGB|COLOR_HSV)) {
        numComponents += 3;             /* Add 3 components for R,G, and B. */
    } else {
        numComponents++;                 /* Add 1 component for color name.  */
    }
    if (cmdPtr->colorFlags & SPACING_INTERVAL) {
        numComponents += numComponents; /* Double the number components.  */
    }
    if ((objc % numComponents) != 0) {
        if (interp != NULL) {
            char mesg[200];
            sprintf(mesg, "wrong # of color components (%d) (%s): should be %d "
                    "components per entry.", objc, Tcl_GetString(objPtr),
                    numComponents);
            Tcl_AppendResult(interp, mesg, (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (cmdPtr->colorFlags & COLOR_RGB) {
        if (AutoTestRGBs(interp, cmdPtr, numComponents, objc, objv) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    switch (numComponents) {
    case 1:                             /* Uniform color names. */
        result = ParseRegularColorNames(interp, cmdPtr, objc, objv);
        break;
    case 2:                             /* Nonuniform color names. */
        result = ParseIrregularColorNames(interp, cmdPtr, objc, objv);
        break;
    case 3:                             /* Uniform RGB|HSV values. */
        result = ParseRegularColorTriplets(interp, cmdPtr, objc, objv);
        break;
    case 4:                             /* Nonuniform RGB|HSV values. */
        if (cmdPtr->colorFlags & (COLOR_RGB|COLOR_HSV)) {
            result = ParseIrregularColorTriplets(interp, cmdPtr, objc, objv);
        } else {
            result = ParseIntervalColorNames(interp, cmdPtr, objc, objv);
        }
        break;
    case 8:                             /* Interval RGB|HSV colors. */
        result = ParseIntervalColorTriplets(interp, cmdPtr, objc, objv);
        break;
    default:
        if (interp != NULL) {
            Tcl_AppendResult(interp, "unknown number of color components \"",
                             Blt_Itoa(numComponents), "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (result == TCL_OK) {
        SortEntries(cmdPtr->numColors, cmdPtr->colors);
    }
    return result;
}

/*
 *	"o1 o2 o3 o4.."
 */
static int
ParseRegularOpacity(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
                    Tcl_Obj **objv)
{
    PaletteInterval *entries, *entryPtr;
    double step;
    int i, numEntries;
    Blt_Pixel low;
    
    numEntries = objc - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    step = 1.0 / numEntries;

    if (GetOpacity(interp, objv[0], &low) != TCL_OK) {
        goto error;
    }
    for (entryPtr = entries, i = 1; i < objc; i++, entryPtr++) {
        Blt_Pixel high;
        
	if (GetOpacity(interp, objv[i], &high) != TCL_OK) {
            goto error;
	}
        entryPtr->low.u32 = low.u32;
        entryPtr->high.u32 = high.u32;
	entryPtr->min.value = entryPtr->min.norm = i * step;
	entryPtr->max.value = entryPtr->max.norm = (i+1) * step;
	entryPtr->min.isAbsolute = entryPtr->max.isAbsolute = FALSE;
	low.u32 = high.u32;
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
 *	"x1 o1 x2 o2 x3 o3 x4 o4.."
 */
static int
ParseIrregularOpacity(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
                      Tcl_Obj **objv)
{
    PaletteInterval *entries, *entryPtr;
    double step;
    int i, numEntries;
    PaletteStep min;
    Blt_Pixel low;
    
    numEntries = (objc / 2) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    step = 1.0 / numEntries;

    if (GetStepFromObj(interp, objv[0], &min) != TCL_OK) {
        goto error;
    }
    if (GetOpacity(interp, objv[1], &low) != TCL_OK) {
        goto error;
    }
    for (entryPtr = entries, i = 2; i < objc; i += 2, entryPtr++) {
        Blt_Pixel high;
        PaletteStep max;
        
        if (GetStepFromObj(interp, objv[i], &max) != TCL_OK) {
            goto error;
        }
	if (GetOpacity(interp, objv[i+1], &high) != TCL_OK) {
            goto error;
	}
        entryPtr->low.u32 = low.u32;
        entryPtr->high.u32 = high.u32;
	entryPtr->min.value = entryPtr->min.norm = i * step;
	entryPtr->max.value = entryPtr->max.norm = (i+1) * step;
	entryPtr->min.isAbsolute = entryPtr->max.isAbsolute = FALSE;
	low.u32 = high.u32;
        min = max;
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
 *	"0.0 0.1 0.4 0.4 0.4 0.4 0.5 0.6..."
 */
static int
ParseIntervalOpacity(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
                     Tcl_Obj **objv)
{
    PaletteInterval *entries, *entryPtr;
    int i, numEntries;

    numEntries = (objc / 4) - 1;
    entries = Blt_AssertMalloc(sizeof(PaletteInterval) * numEntries);
    for (entryPtr = entries, i = 0; i < objc; i += 4, entryPtr++) {
        Blt_Pixel low, high;
        PaletteStep min, max;
        
        if (GetStepFromObj(interp, objv[i], &min) != TCL_OK) {
            goto error;
        }
        if (GetOpacity(interp, objv[i+1], &low) != TCL_OK) {
            goto error;
        }
        if (GetStepFromObj(interp, objv[i+2], &max) != TCL_OK) {
            goto error;
        }
        if (GetOpacity(interp, objv[i+3], &high) != TCL_OK) {
            goto error;
        }
        entryPtr->high.u32 = high.u32;
        entryPtr->low.u32 = low.u32;
	entryPtr->min = min;
	entryPtr->max = max;
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
 */
static int
ParseOpacityData(Tcl_Interp *interp, PaletteCmd *cmdPtr, Tcl_Obj *objPtr)
{
    Tcl_Obj **objv;
    int objc, result, numComponents;
    
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 0) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "no opacity component data", (char *)NULL);
        }
        return TCL_ERROR;
    }
    numComponents = 0;
    if ((cmdPtr->opacityFlags & SPACING_REGULAR) == 0)  {
        numComponents++;                /* Add 1 component for value.  */
    }
    numComponents++;                    /* Add 1 component for opacity. */
    if (cmdPtr->colorFlags & SPACING_INTERVAL) {
        numComponents += numComponents; /* Double the number components.  */
    }
    if ((objc % numComponents) != 0) {
        if (interp != NULL) {
            Tcl_AppendResult(interp,
                             "wrong # of opacity components: should be ",
                             Blt_Itoa(numComponents), " components per entry.",
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    switch (numComponents) {
    case 1:                             /* Uniform color names. */
        result = ParseRegularOpacity(interp, cmdPtr, objc, objv);
        break;
    case 2:                             /* Nonuniform color names. */
        result = ParseIrregularOpacity(interp, cmdPtr, objc, objv);
        break;
    case 4:                             /* Nonuniform RGB values. */
        result = ParseIntervalOpacity(interp, cmdPtr, objc, objv);
        break;
    default:
        if (interp != NULL) {
            Tcl_AppendResult(interp, "unknown number of opacity components \"",
                         Blt_Itoa(numComponents), "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (result == TCL_OK) {
        SortEntries(cmdPtr->numColors, cmdPtr->colors);
    }
    return result;
}

static int
LoadData(Tcl_Interp *interp, PaletteCmd *cmdPtr)
{
    int result;
    
    fprintf(stderr, "LoadData palette=%s\n", cmdPtr->name);
    cmdPtr->flags |= LOADED;
    result = TCL_ERROR;
    if (cmdPtr->colorFileObjPtr != NULL) {
        const char *fileName;
        Blt_DBuffer dbuffer;
        Tcl_Obj *objPtr;
        
        dbuffer = Blt_DBuffer_Create();
        fileName = Tcl_GetString(cmdPtr->colorFileObjPtr);
        if (Blt_DBuffer_LoadFile(interp, fileName, dbuffer) != TCL_OK) {
            return TCL_ERROR;
        }
        objPtr = Blt_DBuffer_StringObj(dbuffer);
        Tcl_IncrRefCount(objPtr);
        result = ParseColorData(interp, cmdPtr, objPtr);
        Tcl_DecrRefCount(objPtr);
        Blt_DBuffer_Destroy(dbuffer);
    } else if (cmdPtr->colorDataObjPtr != NULL) {
        result = ParseColorData(interp, cmdPtr, cmdPtr->colorDataObjPtr);
    }
    if (result != TCL_OK) {
        return TCL_ERROR;
    }
    if (cmdPtr->opacityFileObjPtr != NULL) {
        const char *fileName;
        Blt_DBuffer dbuffer;
        Tcl_Obj *objPtr;
        
        dbuffer = Blt_DBuffer_Create();
        fileName = Tcl_GetString(cmdPtr->opacityFileObjPtr);
        if (Blt_DBuffer_LoadFile(interp, fileName, dbuffer) != TCL_OK) {
            return TCL_ERROR;
        }
        objPtr = Blt_DBuffer_StringObj(dbuffer);
        Tcl_IncrRefCount(objPtr);
        result = ParseOpacityData(interp, cmdPtr, objPtr);
        Tcl_DecrRefCount(objPtr);
        Blt_DBuffer_Destroy(dbuffer);
    } else if (cmdPtr->opacityDataObjPtr != NULL) {
        result = ParseOpacityData(interp, cmdPtr, cmdPtr->opacityDataObjPtr);
    } else {
        return TCL_OK;
    }
    NotifyClients(cmdPtr, PALETTE_CHANGE_NOTIFY);
    return result;
}

static int
ConfigurePaletteCmd(Tcl_Interp *interp, PaletteCmd *cmdPtr, int objc, 
	      Tcl_Obj *const *objv, int flags)
{
    if (Blt_ParseSwitches(interp, paletteSpecs, objc, objv, (char *)cmdPtr, 
	flags | BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }

    if ((cmdPtr->colorFileObjPtr != NULL) && (cmdPtr->colorDataObjPtr != NULL)){
        Tcl_AppendResult(interp,
                         "can't set both -colorfile and -colordata flags",
                         (char *)NULL);
        return TCL_ERROR;
    }
    if ((cmdPtr->colorFileObjPtr == NULL) && (cmdPtr->colorDataObjPtr == NULL)){
        Tcl_AppendResult(interp,
                         "one of -colorfile and -colordata flags are required",
                         (char *)NULL);
        return TCL_ERROR;
    }
    if (cmdPtr->colorFileObjPtr != NULL) {
        const char *fileName;

        fileName = Tcl_GetString(cmdPtr->colorFileObjPtr);
        if (access(fileName, R_OK) != 0) {
            Tcl_AppendResult(interp, "can't access \"", fileName, "\":",
                             Tcl_PosixError(interp), (char *)NULL);
            return TCL_ERROR;
        }
    }
    if ((cmdPtr->opacityFileObjPtr != NULL) &&
        (cmdPtr->opacityDataObjPtr != NULL)) {
        Tcl_AppendResult(interp,
                         "can't set both -opacityfile and -opacitydata flags",
                         (char *)NULL);
        return TCL_ERROR;
    }
    if (cmdPtr->opacityFileObjPtr != NULL) {
        const char *fileName;

        fileName = Tcl_GetString(cmdPtr->opacityFileObjPtr);
        if (access(fileName, R_OK) != 0) {
            Tcl_AppendResult(interp, "can't access \"", fileName, "\":",
                             Tcl_PosixError(interp), (char *)NULL);
            return TCL_ERROR;
        }
    }
    NotifyClients(cmdPtr, PALETTE_CHANGE_NOTIFY);
    return TCL_OK;
}

static unsigned int
ColorLerp(PaletteInterval *entryPtr, double t)
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
OpacityLerp(PaletteInterval *entryPtr, double t)
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
    
    cmdPtr->min = min;
    cmdPtr->max = max;

    range = max - min;
    scale = 1.0 / range;
    for (i = 0; i < cmdPtr->numColors; i++) {
        PaletteInterval *entryPtr;

        entryPtr = cmdPtr->colors + i;
	if (entryPtr->min.isAbsolute) {
	    entryPtr->min.norm = (entryPtr->min.value - min) * scale;
	} 
	if (entryPtr->max.isAbsolute) {
	    entryPtr->max.norm = (entryPtr->max.value - min) * scale;
	}
    }
    for (i = 0; i < cmdPtr->numOpacities; i++) {
        PaletteInterval *entryPtr;

        entryPtr = cmdPtr->opacities + i;
	if (entryPtr->min.isAbsolute) {
	    entryPtr->min.norm = (entryPtr->min.value - min) * scale;
	}
	if (entryPtr->max.isAbsolute) {
	    entryPtr->min.norm = (entryPtr->max.value - min) * scale;
	}
    }
}

static PaletteInterval * 
SearchForEntry(size_t length, PaletteInterval *entries, double norm)
{
    int low, high;

    low = 0;
    high = length - 1;
    while (low <= high) {
        PaletteInterval *entryPtr;
	int median;
        
	median = (low + high) >> 1;
        entryPtr = entries + median;
        if (InRange(norm, entryPtr->min.norm, entryPtr->max.norm)) {
            return entryPtr;
        }
        if (norm < entryPtr->min.norm) {
	    high = median - 1;
        } else if (norm > entryPtr->max.norm) {
	    low = median + 1;
        } else {
            return NULL;
        }
    }
    return NULL;                       /* Can't find number. */
}


static int 
Interpolate(PaletteCmd *cmdPtr, double norm, Blt_Pixel *colorPtr)
{
    PaletteInterval *entryPtr;
    Blt_Pixel color;
    double t;

    norm = RCLAMP(norm);
    color.u32 = 0x00;			/* Default to empty. */

    if (cmdPtr->numColors == 0) {
        return FALSE;
    }
    if (cmdPtr->colorFlags & SPACING_REGULAR) {
        int i;
        i = (int)(norm * (cmdPtr->numColors));
        if (i >= cmdPtr->numColors) {
            i = cmdPtr->numColors - 1;
        }
        entryPtr = cmdPtr->colors + i;
#ifdef notdef
        if (!InRange(norm, entryPtr->min.norm, entryPtr->max.norm)) {
            fprintf(stderr, "norm=%g index=%d max=%d\n", norm, i,
                    cmdPtr->numColors);
            fprintf(stderr, "not in range norm=%g min=%g max=%g\n", norm,
                    entryPtr->min.norm, entryPtr->max.norm);
        }
#endif
    } else {
        entryPtr = SearchForEntry(cmdPtr->numColors, cmdPtr->colors, norm);
    }
    if (entryPtr == NULL) {
#ifndef notdef
	fprintf(stderr, "can't interpolate: norm=%.17g\n", norm);
#endif
        PrintEntries(cmdPtr->numColors, cmdPtr->colors);
	abort();
	return FALSE;
    }
    t = (norm - entryPtr->min.norm) / (entryPtr->max.norm - entryPtr->min.norm);
    color.u32 = ColorLerp(entryPtr, t);

    if (cmdPtr->numOpacities > 0) {
        if (cmdPtr->opacityFlags & SPACING_REGULAR) {
            int i;
            i = (int)(norm * (cmdPtr->numOpacities));
            if (i >= cmdPtr->numOpacities) {
                i = cmdPtr->numOpacities - 1;
            }
            entryPtr = cmdPtr->opacities + i;
        } else {
            entryPtr = SearchForEntry(cmdPtr->numOpacities, cmdPtr->opacities,
                norm);
            if (entryPtr != NULL) {
                unsigned int alpha;
                
                t = (norm - entryPtr->min.norm) /
                    (entryPtr->max.norm - entryPtr->min.norm);
                alpha = OpacityLerp(entryPtr, t);
                Blt_FadeColor(&color, alpha);
            }
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
#ifdef notdef
    static char cmd[] = "source [file join $blt_library palette.tcl]";
#else
    static char cmd[] = "if { [catch {source [file join $blt_library palette.tcl]}] != 0} { global errorInfo; puts stderr errorInfo=$errorInfo }";
#endif
    if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
	char info[2000];

	Blt_FormatString(info, 2000, "\n    (while loading palettes)");
        fprintf(stderr, "error load palettes: %s: %s\n", info,
                Tcl_GetString(Tcl_GetObjResult(interp)));
	Tcl_AddErrorInfo(interp, info);
	Tcl_BackgroundError(interp);
	return TCL_ERROR;
    }
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
    if ((cmdPtr->flags & LOADED) == 0) {
        if (LoadData(interp, cmdPtr) != TCL_OK) {
            return TCL_ERROR;
        }
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

	    value = ((double)y / (double)(h-1));
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
    PaletteStep step;

    if (GetPaletteCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetStepFromObj(interp, objv[3], &step) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((cmdPtr->flags & LOADED) == 0) {
        if (LoadData(interp, cmdPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    switches.min = 0.0;
    switches.max = 1.0;
    /* Process switches  */
    if (Blt_ParseSwitches(interp, interpolateSwitches, objc - 4, objv + 4, 
		&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (step.isAbsolute) {
	norm = (step.value - switches.min) / (switches.max - switches.min);
    } else {
	norm = step.value;
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
    {"create",      1, CreateOp,      2, 0, "?name? ?option value?...",},
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
    if ((cmdPtr->flags & LOADED) == 0) {
        if (LoadData(interp, cmdPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
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

    if ((cmdPtr->flags & LOADED) == 0) {
        if (LoadData(NULL, cmdPtr) != TCL_OK) {
            return;
        }
    }
    if ((cmdPtr->min != min) || (cmdPtr->max != max)) {
        NormalizePalette(cmdPtr, min, max);
    }
}

int
Blt_Palette_GetAssociatedColorFromAbsoluteValue(
    Blt_Palette palette, double value, double min, double max)
{
    Blt_Pixel color;
    PaletteCmd *cmdPtr = (PaletteCmd *)palette;
    double norm;

    if ((cmdPtr->flags & LOADED) == 0) {
        if (LoadData(NULL, cmdPtr) != TCL_OK) {
            return 0x0;
        }
    }
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

    if ((cmdPtr->flags & LOADED) == 0) {
        if (LoadData(NULL, cmdPtr) != TCL_OK) {
            return 0x0;
        }
    }
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
    palPtr->colors = Blt_AssertMalloc(sizeof(PaletteInterval));
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


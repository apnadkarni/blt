
/*
 * bltGrClrbar.c --
 *
 *	This module implements colorbar widget for the BLT graph widget.
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

/* 
 * $graph colorbar $w -axis z -colormap  
 * 
 */
#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include <X11/Xutil.h>
#include "bltAlloc.h"
#include "bltMath.h"
#include "bltHash.h"
#include "bltChain.h"
#include "bltBind.h"
#include "bltPs.h"
#include "bltBg.h"
#include "bltPicture.h"
#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"
#include "bltGrElem.h"
#include "bltOp.h"

/* Axis flags: */

#define AUTO_MAJOR	(1<<16)		/* Auto-generate major ticks. */
#define AUTO_MINOR	(1<<17)		/* Auto-generate minor ticks. */
#define USE		(1<<18)		/* Axis is displayed on the screen via
					 * the "use" operation */
#define GRID		(1<<19)		/* Display grid lines. */
#define GRIDMINOR	(1<<20)		/* Display grid lines for minor
					 * ticks. */
#define SHOWTICKS	(1<<21)		/* Display axis ticks. */
#define EXTERIOR	(1<<22)		/* Axis is exterior to the plot. */
#define CHECK_LIMITS	(1<<23)		/* Validate user-defined axis
					 * limits. */
#define LOGSCALE	(1<<24)
#define DECREASING	(1<<25)
#define HORIZONTAL	(1<<26)

#define MAXTICKS	10001

#define FCLAMP(x)	((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))

/*
 * Round x in terms of units
 */
#define UROUND(x,u)	(Round((x)/(u))*(u))
#define UCEIL(x,u)	(ceil((x)/(u))*(u))
#define UFLOOR(x,u)	(floor((x)/(u))*(u))

#define NUMDIGITS	15		/* Specifies the number of digits of
					 * accuracy used when outputting axis
					 * tick labels. */
enum TickRange {
    TIGHT, LOOSE, ALWAYS_LOOSE
};

#define AXIS_PAD_TITLE		2	/* Padding for axis title. */

#define HORIZMARGIN(m)	(!((m)->site & 0x1)) /* Even sites are horizontal */

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
    int label;				/* Distance from axis to tick label. */
} AxisInfo;

typedef struct {
    const char *name;
    ClassId classId;
    int margin, invertMargin;
} AxisName;

static AxisName axisNames[] = { 
    { "x",  CID_AXIS_X, MARGIN_BOTTOM, MARGIN_LEFT   },
    { "y",  CID_AXIS_Y, MARGIN_LEFT,   MARGIN_BOTTOM },
    { "x2", CID_AXIS_X, MARGIN_TOP,    MARGIN_RIGHT  },
    { "y2", CID_AXIS_Y, MARGIN_RIGHT,  MARGIN_TOP    },
    { "z",  CID_AXIS_Z, MARGIN_NONE,   MARGIN_NONE   }
} ;
static int numAxisNames = sizeof(axisNames) / sizeof(AxisName);

static Blt_OptionParseProc ObjToLimitProc;
static Blt_OptionPrintProc LimitToObjProc;
Blt_CustomOption bltLimitOption = {
    ObjToLimitProc, LimitToObjProc, NULL, (ClientData)0
};

static Blt_OptionFreeProc  FreeTicksProc;
static Blt_OptionParseProc ObjToTicksProc;
static Blt_OptionPrintProc TicksToObjProc;
static Blt_CustomOption majorTicksOption = {
    ObjToTicksProc, TicksToObjProc, FreeTicksProc, (ClientData)AUTO_MAJOR,
};
static Blt_CustomOption minorTicksOption = {
    ObjToTicksProc, TicksToObjProc, FreeTicksProc, (ClientData)AUTO_MINOR,
};
static Blt_OptionFreeProc  FreeAxisProc;
static Blt_OptionPrintProc AxisToObjProc;
static Blt_OptionParseProc ObjToAxisProc;
Blt_CustomOption bltXAxisOption = {
    ObjToAxisProc, AxisToObjProc, FreeAxisProc, (ClientData)CID_AXIS_X
};
Blt_CustomOption bltYAxisOption = {
    ObjToAxisProc, AxisToObjProc, FreeAxisProc, (ClientData)CID_AXIS_Y
};
Blt_CustomOption bltZAxisOption = {
    ObjToAxisProc, AxisToObjProc, FreeAxisProc, (ClientData)CID_AXIS_Z
};

Blt_CustomOption bltAxisOption = {
    ObjToAxisProc, AxisToObjProc, FreeAxisProc, (ClientData)CID_NONE
};

static Blt_OptionParseProc ObjToLooseProc;
static Blt_OptionPrintProc LooseToObjProc;
static Blt_CustomOption looseOption = {
    ObjToLooseProc, LooseToObjProc, NULL, (ClientData)0,
};

static Blt_OptionParseProc ObjToUseProc;
static Blt_OptionPrintProc UseToObjProc;
static Blt_CustomOption useOption = {
    ObjToUseProc, UseToObjProc, NULL, (ClientData)0
};

static Blt_OptionFreeProc FreePaletteProc;
static Blt_OptionParseProc ObjToPaletteProc;
static Blt_OptionPrintProc PaletteToObjProc;
static Blt_CustomOption paletteOption =
{
    ObjToPaletteProc, PaletteToObjProc, FreePaletteProc, (ClientData)0
};

#define DEF_ACTIVEBACKGROUND	STD_ACTIVE_BACKGROUND
#define DEF_ACTIVEFOREGROUND	STD_ACTIVE_FOREGROUND
#define DEF_ACTIVERELIEF	"flat"
#define DEF_ANGLE		"0.0"
#define DEF_BACKGROUND		(char *)NULL
#define DEF_BORDERWIDTH		"0"
#define DEF_CHECKLIMITS		"0"
#define DEF_COMMAND		(char *)NULL
#define DEF_DESCENDING		"0"
#define DEF_FOREGROUND		RGB_BLACK
#define DEF_GRID_BARCHART	"1"
#define DEF_GRIDCOLOR		RGB_GREY64
#define DEF_GRIDDASHES		"dot"
#define DEF_GRID_GRAPH		"0"
#define DEF_GRIDLINEWIDTH	"0"
#define DEF_GRIDMINOR		"1"
#define DEF_GRIDMINOR_COLOR	RGB_GREY64
#define DEF_HIDE		"0"
#define DEF_JUSTIFY		"c"
#define DEF_LIMITS_FORMAT	(char *)NULL
#define DEF_LINEWIDTH		"1"
#define DEF_LOGSCALE		"0"
#define DEF_LOOSE		"0"
#define DEF_PALETTE		(char *)NULL
#define DEF_RANGE		"0.0"
#define DEF_RELIEF		"flat"
#define DEF_SCROLL_INCREMENT 	"10"
#define DEF_SHIFTBY		"0.0"
#define DEF_SHOWTICKS		"1"
#define DEF_STEP		"0.0"
#define DEF_SUBDIVISIONS	"2"
#define DEF_TAGS		"all"
#define DEF_EXTERIOR		"1"
#define DEF_TICK_ANCHOR		"c"
#define DEF_LIMITS_FONT		STD_FONT_NUMBERS
#define DEF_TICKFONT_GRAPH	STD_FONT_NUMBERS
#define DEF_TICKFONT_BARCHART	STD_FONT_SMALL
#define DEF_TICKLENGTH		"4"
#define DEF_DIVISIONS		"10"
#define DEF_TITLE_ALTERNATE	"0"
#define DEF_TITLE_FG		RGB_BLACK
#define DEF_TITLE_FONT		"{Sans Serif} 10"

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_ACTIVEBACKGROUND, 
	Blt_Offset(Colorbar, activeBg), ALL_GRAPHS | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground",
	"ActiveForeground", DEF_ACTIVEFOREGROUND,
	Blt_Offset(Colorbar, activeFgColor), ALL_GRAPHS}, 
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "Relief",
	DEF_ACTIVERELIEF, Blt_Offset(Colorbar, activeRelief),
	ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT}, 
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_BACKGROUND, Blt_Offset(Colorbar, normalBg),
	ALL_GRAPHS | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_LIST, "-bindtags", "bindTags", "BindTags", DEF_TAGS, 
	Blt_Offset(Colorbar, obj.tags), ALL_GRAPHS | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
	0, ALL_GRAPHS},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BORDERWIDTH, Blt_Offset(Colorbar, borderWidth),
	ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-color", "color", "Color",
	DEF_FOREGROUND, Blt_Offset(Colorbar, tickColor), ALL_GRAPHS},
    {BLT_CONFIG_OBJ, "-formatcommand", "formatCommand", "FormatCommand",
	DEF_COMMAND, Blt_Offset(Colorbar, fmtCmdObjPtr),
	BLT_CONFIG_NULL_OK | ALL_GRAPHS},
    {BLT_CONFIG_BOOLEAN, "-descending", "descending", "Descending",
	DEF_DESCENDING, Blt_Offset(Colorbar, descending),
	ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-decreasing", "decreasing", "Decreasing",
	DEF_DESCENDING, Blt_Offset(Colobar, descending),
	ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-exterior", "exterior", "exterior", DEF_EXTERIOR,
	Blt_Offset(Colorbar, flags), ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, 
 	(Blt_CustomOption *)EXTERIOR},
    {BLT_CONFIG_SYNONYM, "-fg", "color", (char *)NULL, 
        (char *)NULL, 0, ALL_GRAPHS},
    {BLT_CONFIG_SYNONYM, "-foreground", "color", (char *)NULL, 
        (char *)NULL, 0, ALL_GRAPHS},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_HIDE, 
	Blt_Offset(Colorbar, flags), ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDE},
    {BLT_CONFIG_BOOLEAN, "-labeloffset", "labelOffset", "LabelOffset",
        (char *)NULL, Blt_Offset(Colorbar, labelOffset), ALL_GRAPHS}, 
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth",
	DEF_LINEWIDTH, Blt_Offset(Colorbar, lineWidth),
	ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-logscale", "logScale", "LogScale",
	DEF_LOGSCALE, Blt_Offset(Colorbar, logScale),
	ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-loose", "loose", "Loose", DEF_LOOSE, 0, 
	ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, &looseOption},
    {BLT_CONFIG_CUSTOM, "-majorticks", "majorTicks", "MajorTicks",
	(char *)NULL, Blt_Offset(Colorbar, t1Ptr),
	BLT_CONFIG_NULL_OK | ALL_GRAPHS, &majorTicksOption},
    {BLT_CONFIG_CUSTOM, "-max", "max", "Max", (char *)NULL, 
	Blt_Offset(Colorbar, reqMax), ALL_GRAPHS, &bltLimitOption},
    {BLT_CONFIG_CUSTOM, "-min", "min", "Min", (char *)NULL, 
	Blt_Offset(Colorbar, reqMin), ALL_GRAPHS, &bltLimitOption},
    {BLT_CONFIG_CUSTOM, "-minorticks", "minorTicks", "MinorTicks",
	(char *)NULL, Blt_Offset(Colorbar, t2Ptr), 
	BLT_CONFIG_NULL_OK | ALL_GRAPHS, &minorTicksOption},
    {BLT_CONFIG_CUSTOM, "-orient", "orient", "Orient", DEF_ORIENT, 
	Blt_Offset(Colorbar, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	&orientOption},
    {BLT_CONFIG_CUSTOM, "-colormap", "colormap", "Colormap", DEF_PALETTE, 
	Blt_Offset(Colorbar, colormap), ALL_GRAPHS, &colormapOption},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_RELIEF, Blt_Offset(Colorbar, relief), 
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FLOAT, "-rotate", "rotate", "Rotate", DEF_ANGLE, 
	Blt_Offset(Colorbar, tickAngle), 
	ALL_GRAPHS|BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-showticks", "showTicks", "ShowTicks",
	DEF_SHOWTICKS, Blt_Offset(Colorbar, flags), 
	ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)SHOWTICKS},
    {BLT_CONFIG_DOUBLE, "-stepsize", "stepSize", "StepSize",
	DEF_STEP, Blt_Offset(Colorbar, reqStep),
	ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_INT, "-subdivisions", "subdivisions", "Subdivisions",
	DEF_SUBDIVISIONS, Blt_Offset(Colorbar, reqNumMinorTicks), ALL_GRAPHS},
    {BLT_CONFIG_ANCHOR, "-tickanchor", "tickAnchor", "Anchor",
	DEF_TICK_ANCHOR, Blt_Offset(Colorbar, reqTickAnchor), ALL_GRAPHS},
    {BLT_CONFIG_FONT, "-tickfont", "tickFont", "Font",
	DEF_TICKFONT_GRAPH, Blt_Offset(Colorbar, tickFont), 
	GRAPH | STRIPCHART | CONTOUR},
    {BLT_CONFIG_FONT, "-tickfont", "tickFont", "Font",
	DEF_TICKFONT_BARCHART, Blt_Offset(Colorbar, tickFont), BARCHART},
    {BLT_CONFIG_PIXELS_NNEG, "-ticklength", "tickLength", "TickLength",
	DEF_TICKLENGTH, Blt_Offset(Colorbar, tickLength), 
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_INT, "-tickdefault", "tickDefault", "TickDefault",
	DEF_DIVISIONS, Blt_Offset(Colorbar, reqNumMajorTicks),
	ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-title", "title", "Title",
	(char *)NULL, Blt_Offset(Colorbar, title),
	BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK | ALL_GRAPHS},
    {BLT_CONFIG_COLOR, "-titlecolor", "titleColor", "Color", 
	DEF_FOREGROUND, Blt_Offset(Colorbar, titleColor), 	
	ALL_GRAPHS},
    {BLT_CONFIG_FONT, "-titlefont", "titleFont", "Font", DEF_TITLE_FONT, 
	Blt_Offset(Colorbar, titleFont), ALL_GRAPHS},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

/* Forward declarations */
static void DestroyColorbar(Axis *axisPtr);
static Tcl_FreeProc ColorbarFreeProc;
static int lastMargin;

INLINE static double
Clamp(double x) 
{
    return (x < 0.0) ? 0.0 : (x > 1.0) ? 1.0 : x;
}

INLINE static int
Round(double x)
{
    return (int) (x + ((x < 0.0) ? -0.5 : 0.5));
}

static void
SetColorbarRange(AxisRange *rangePtr, double min, double max)
{
    rangePtr->min = min;
    rangePtr->max = max;
    rangePtr->range = max - min;
    if (FABS(rangePtr->range) < DBL_EPSILON) {
	rangePtr->range = 1.0;
    }
    rangePtr->scale = 1.0 / rangePtr->range;
}

/*
 *---------------------------------------------------------------------------
 *
 * InRange --
 *
 *	Determines if a value lies within a given range.
 *
 *	The value is normalized and compared against the interval [0..1],
 *	where 0.0 is the minimum and 1.0 is the maximum.  DBL_EPSILON is the
 *	smallest number that can be represented on the host machine, such that
 *	(1.0 + epsilon) != 1.0.
 *
 *	Please note, *max* can't equal *min*.
 *
 * Results:
 *	If the value is within the interval [min..max], 1 is returned; 0
 *	otherwise.
 *
 *---------------------------------------------------------------------------
 */
INLINE static int
InRange(double x, AxisRange *rangePtr)
{
    if (rangePtr->range < DBL_EPSILON) {
	return (FABS(rangePtr->max - x) >= DBL_EPSILON);
    } else {
	double norm;

	norm = (x - rangePtr->min) * rangePtr->scale;
	return ((norm > -DBL_EPSILON) && ((norm - 1.0) <= DBL_EPSILON));
    }
}

INLINE static int
ColorbarIsHorizontal(Axis *axisPtr)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;

    return ((axisPtr->obj.classId == CID_AXIS_Y) == graphPtr->inverted);
}


static void
ReleaseAxis(Axis *axisPtr)
{
    if (axisPtr != NULL) {
	axisPtr->refCount--;
	assert(axisPtr->refCount >= 0);
	if (axisPtr->refCount == 0) {
	    axisPtr->flags |= DELETE_PENDING;
	    Tcl_EventuallyFree(axisPtr, FreeAxis);
	}
    }
}

/*
 *-----------------------------------------------------------------------------
 * Custom option parse and print procedures
 *-----------------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
FreeAxisProc(
    ClientData clientData,		/* Not used. */
    Display *display,			/* Not used. */
    char *widgRec,
    int offset)
{
    Axis **axisPtrPtr = (Axis **)(widgRec + offset);

    if (*axisPtrPtr != NULL) {
	ReleaseAxis(*axisPtrPtr);
	*axisPtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToAxisProc --
 *
 *	Converts the name of an axis to a pointer to its axis structure.
 *
 * Results:
 *	The return value is a standard TCL result.  The axis flags are written
 *	into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToAxisProc(
    ClientData clientData,		/* Class identifier of the type of
					 * axis we are looking for. */
    Tcl_Interp *interp,			/* Interpreter to report results. */
    Tk_Window tkwin,			/* Used to look up pointer to graph. */
    Tcl_Obj *objPtr,			/* String representing new value. */
    char *widgRec,			/* Pointer to structure record. */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    ClassId classId = (ClassId)clientData;
    Axis **axisPtrPtr = (Axis **)(widgRec + offset);
    Axis *axisPtr;
    Graph *graphPtr;

    if (flags & BLT_CONFIG_NULL_OK) {
	const char *string;

	string  = Tcl_GetString(objPtr);
	if (string[0] == '\0') {
	    ReleaseAxis(*axisPtrPtr);
	    *axisPtrPtr = NULL;
	    return TCL_OK;
	}
    }
    graphPtr = Blt_GetGraphFromWindowData(tkwin);
    assert(graphPtr);
    if (GetAxisByClass(interp, graphPtr, objPtr, classId, &axisPtr) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    if (*axisPtrPtr != NULL) {
	ReleaseAxis(*axisPtrPtr);
    }
    *axisPtrPtr = axisPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisToObjProc --
 *
 *	Convert the window coordinates into a string.
 *
 * Results:
 *	The string representing the coordinate position is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
AxisToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Pointer to structure record .*/
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Axis *axisPtr = *(Axis **)(widgRec + offset);
    const char *name;

    name = (axisPtr == NULL) ? "" : axisPtr->obj.name;
    return Tcl_NewStringObj(name, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLimitProc --
 *
 *	Convert the string representation of an axis limit into its numeric
 *	form.
 *
 * Results:
 *	The return value is a standard TCL result.  The symbol type is written
 *	into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToLimitProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing new value. */
    char *widgRec,			/* Pointer to structure record. */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    double *limitPtr = (double *)(widgRec + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
	*limitPtr = Blt_NaN();
    } else if (Blt_ExprDoubleFromObj(interp, objPtr, limitPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LimitToObjProc --
 *
 *	Convert the floating point axis limits into a string.
 *
 * Results:
 *	The string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */

/*ARGSUSED*/
static Tcl_Obj *
LimitToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    double limit = *(double *)(widgRec + offset);
    Tcl_Obj *objPtr;

    if (DEFINED(limit)) {
	objPtr = Tcl_NewDoubleObj(limit);
    } else {
	objPtr = Tcl_NewStringObj("", -1);
    }
    return objPtr;
}


/*ARGSUSED*/
static void
FreeTicksProc(ClientData clientData, Display *display, char *widgRec, 
	      int offset)
{
    Colorbar *cbarPtr = (Axis *)widgRec;
    Ticks **ticksPtrPtr = (Ticks **) (widgRec + offset);
    unsigned long mask = (unsigned long)clientData;

    cbarPtr->flags |= mask;
    if (*ticksPtrPtr != NULL) {
	Blt_Free(*ticksPtrPtr);
    }
    *ticksPtrPtr = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTicksProc --
 *
 *
 * Results:
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTicksProc(
    ClientData clientData,		/* Either AUTO_MAJOR or AUTO_MINOR. */
    Tcl_Interp *interp,		        /* Interpreter to send results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing new value. */
    char *widgRec,			/* Pointer to structure record. */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Colorbar *cbarPtr = (Colorbar *)widgRec;
    Tcl_Obj **objv;
    Ticks **ticksPtrPtr = (Ticks **) (widgRec + offset);
    Ticks *ticksPtr;
    int objc;
    unsigned long mask = (unsigned long)clientData;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    cbarPtr->flags |= mask;
    ticksPtr = NULL;
    if (objc > 0) {
	int i;

	ticksPtr = Blt_AssertMalloc(sizeof(Ticks) + (objc*sizeof(double)));
	for (i = 0; i < objc; i++) {
	    double value;

	    if (Blt_ExprDoubleFromObj(interp, objv[i], &value) != TCL_OK) {
		Blt_Free(ticksPtr);
		return TCL_ERROR;
	    }
	    ticksPtr->values[i] = value;
	}
	ticksPtr->numTicks = objc;
	axisPtr->flags &= ~mask;
    }
    FreeTicksProc(clientData, Tk_Display(tkwin), widgRec, offset);
    *ticksPtrPtr = ticksPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TicksToObjProc --
 *
 *	Convert array of tick coordinates to a list.
 *
 * Results:
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TicksToObjProc(
    ClientData clientData,		/* Either AUTO_MAJOR or AUTO_MINOR. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Colorbar *cbarPtr = (Colorbar *)widgRec;
    Tcl_Obj *listObjPtr;
    Ticks *ticksPtr = *(Ticks **) (widgRec + offset);
    unsigned long mask;

    mask = (unsigned long)clientData;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if ((ticksPtr != NULL) && ((axisPtr->flags & mask) == 0)) {
	int i;

	for (i = 0; i < ticksPtr->numTicks; i++) {
	    Tcl_Obj *objPtr;

	    objPtr = Tcl_NewDoubleObj(ticksPtr->values[i]);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    }
    return listObjPtr;
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
    Axis *axisPtr = clientData;

    if (flags & PALETTE_DELETE_NOTIFY) {
	axisPtr->palette = NULL;
    }
    axisPtr->flags |= MAP_ITEM;
    axisPtr->obj.graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(axisPtr->obj.graphPtr);
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
    Axis *axisPtr = (Axis *)widgRec;

    Blt_Palette_DeleteNotifier(*palPtr, axisPtr);
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
    Axis *axisPtr = (Axis *)widgRec;
    const char *string;
    
    string = Tcl_GetString(objPtr);
    if ((string == NULL) || (string[0] == '\0')) {
	FreePaletteProc(clientData, Tk_Display(tkwin), widgRec, offset);
	return TCL_OK;
    }
    if (Blt_Palette_GetFromObj(interp, objPtr, palPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_Palette_CreateNotifier(*palPtr, PaletteChangedProc, axisPtr);
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
FreeTickLabels(Blt_Chain chain)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	TickLabel *labelPtr;

	labelPtr = Blt_Chain_GetValue(link);
	Blt_Free(labelPtr);
    }
    Blt_Chain_Reset(chain);
}

/*
 *---------------------------------------------------------------------------
 *
 * MakeLabel --
 *
 *	Converts a floating point tick value to a string to be used as its
 *	label.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Returns a new label in the string character buffer.  The formatted
 *	tick label will be displayed on the graph.
 *
 * -------------------------------------------------------------------------- 
 */
static TickLabel *
MakeLabel(Colorbar *cbarPtr, double value)
{
#define TICK_LABEL_SIZE		200
    char string[TICK_LABEL_SIZE + 1];
    TickLabel *labelPtr;

    /* Generate a default tick label based upon the tick value.  */
    if (cbarPtr->logScale) {
	Blt_FormatString(string, TICK_LABEL_SIZE, "1E%d", ROUND(value));
    } else {
	Blt_FormatString(string, TICK_LABEL_SIZE, "%.*G", NUMDIGITS, value);
    }

    if (cbarPtr->fmtCmdObjPtr != NULL) {
	Graph *graphPtr;
	Tcl_Interp *interp;
	Tk_Window tkwin;
	
	graphPtr = axisPtr->obj.graphPtr;
	interp = graphPtr->interp;
	tkwin = graphPtr->tkwin;
	/*
	 * A TCL proc was designated to format tick labels. Append the path
	 * name of the widget and the default tick label as arguments when
	 * invoking it. Copy and save the new label from interp->result.
	 */
	Tcl_ResetResult(interp);
	if (Tcl_VarEval(interp, axisPtr->formatCmd, " ", Tk_PathName(tkwin),
		" ", string, (char *)NULL) != TCL_OK) {
	    Tcl_BackgroundError(interp);
	} else {
	    /* 
	     * The proc could return a string of any length, so arbitrarily
	     * limit it to what will fit in the return string.
	     */
	    strncpy(string, Tcl_GetStringResult(interp), TICK_LABEL_SIZE);
	    string[TICK_LABEL_SIZE] = '\0';
	    
	    Tcl_ResetResult(interp); /* Clear the interpreter's result. */
	}
    }
    labelPtr = Blt_AssertMalloc(sizeof(TickLabel) + strlen(string));
    strcpy(labelPtr->string, string);
    labelPtr->anchorPos.x = labelPtr->anchorPos.y = DBL_MAX;
    return labelPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * NiceNum --
 *
 *	Reference: Paul Heckbert, "Nice Numbers for Graph Labels",
 *		   Graphics Gems, pp 61-63.  
 *
 *	Finds a "nice" number approximately equal to x.
 *
 *---------------------------------------------------------------------------
 */
static double
NiceNum(double x, int round)		/* If non-zero, round. Otherwise take
					 * ceiling of value. */
{
    double expt;			/* Exponent of x */
    double frac;			/* Fractional part of x */
    double nice;			/* Nice, rounded fraction */

    expt = floor(log10(x));
    frac = x / EXP10(expt);		/* between 1 and 10 */
    if (round) {
	if (frac < 1.5) {
	    nice = 1.0;
	} else if (frac < 3.0) {
	    nice = 2.0;
	} else if (frac < 7.0) {
	    nice = 5.0;
	} else {
	    nice = 10.0;
	}
    } else {
	if (frac <= 1.0) {
	    nice = 1.0;
	} else if (frac <= 2.0) {
	    nice = 2.0;
	} else if (frac <= 5.0) {
	    nice = 5.0;
	} else {
	    nice = 10.0;
	}
    }
    return nice * EXP10(expt);
}

static Ticks *
GenerateTicks(TickSweep *sweepPtr)
{
    Ticks *ticksPtr;

    ticksPtr = Blt_AssertMalloc(sizeof(Ticks) + 
	(sweepPtr->numSteps * sizeof(double)));
    ticksPtr->numTicks = 0;

    if (sweepPtr->step == 0.0) { 
	/* Hack: A zero step indicates to use log values. */
	int i;
	/* Precomputed log10 values [1..10] */
	static double logTable[] = {
	    0.0, 
	    0.301029995663981, 
	    0.477121254719662, 
	    0.602059991327962, 
	    0.698970004336019, 
	    0.778151250383644, 
	    0.845098040014257,
	    0.903089986991944, 
	    0.954242509439325, 
	    1.0
	};
	for (i = 0; i < sweepPtr->numSteps; i++) {
	    ticksPtr->values[i] = logTable[i];
	}
    } else {
	double value;
	int i;
    
	value = sweepPtr->initial;	/* Start from smallest axis tick */
	for (i = 0; i < sweepPtr->numSteps; i++) {
	    value = UROUND(value, sweepPtr->step);
	    ticksPtr->values[i] = value;
	    value += sweepPtr->step;
	}
    }
    ticksPtr->numTicks = sweepPtr->numSteps;
    return ticksPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * LogAxis --
 *
 * 	Determine the range and units of a log scaled axis.
 *
 * 	Unless the axis limits are specified, the axis is scaled
 * 	automatically, where the smallest and largest major ticks encompass
 * 	the range of actual data values.  When an axis limit is specified,
 * 	that value represents the smallest(min)/largest(max) value in the
 * 	displayed range of values.
 *
 * 	Both manual and automatic scaling are affected by the step used.  By
 * 	default, the step is the largest power of ten to divide the range in
 * 	more than one piece.
 *
 *	Automatic scaling:
 *	Find the smallest number of units which contain the range of values.
 *	The minimum and maximum major tick values will be represent the
 *	range of values for the axis. This greatest number of major ticks
 *	possible is 10.
 *
 * 	Manual scaling:
 *   	Make the minimum and maximum data values the represent the range of
 *   	the values for the axis.  The minimum and maximum major ticks will be
 *   	inclusive of this range.  This provides the largest area for plotting
 *   	and the expected results when the axis min and max values have be set
 *   	by the user (.e.g zooming).  The maximum number of major ticks is 20.
 *
 *   	For log scale, there's the possibility that the minimum and
 *   	maximum data values are the same magnitude.  To represent the
 *   	points properly, at least one full decade should be shown.
 *   	However, if you zoom a log scale plot, the results should be
 *   	predictable. Therefore, in that case, show only minor ticks.
 *   	Lastly, there should be an appropriate way to handle numbers
 *   	<=0.
 *
 *          maxY
 *            |    units = magnitude (of least significant digit)
 *            |    high  = largest unit tick < max axis value
 *      high _|    low   = smallest unit tick > min axis value
 *            |
 *            |    range = high - low
 *            |    # ticks = greatest factor of range/units
 *           _|
 *        U   |
 *        n   |
 *        i   |
 *        t  _|
 *            |
 *            |
 *            |
 *       low _|
 *            |
 *            |_minX________________maxX__
 *            |   |       |      |       |
 *     minY  low                        high
 *           minY
 *
 *
 * 	numTicks = Number of ticks
 * 	min = Minimum value of axis
 * 	max = Maximum value of axis
 * 	range    = Range of values (max - min)
 *
 * 	If the number of decades is greater than ten, it is assumed
 *	that the full set of log-style ticks can't be drawn properly.
 *
 * Results:
 *	None
 *
 * -------------------------------------------------------------------------- 
 */
static void
LogAxis(Axis *axisPtr, double min, double max)
{
    double range;
    double tickMin, tickMax;
    double majorStep, minorStep;
    int numMajor, numMinor;

    numMajor = numMinor = 0;
    /* Suppress compiler warnings. */
    majorStep = minorStep = 0.0;
    tickMin = tickMax = Blt_NaN();
    if (min < max) {
	min = (min != 0.0) ? log10(FABS(min)) : 0.0;
	max = (max != 0.0) ? log10(FABS(max)) : 1.0;

	tickMin = floor(min);
	tickMax = ceil(max);
	range = tickMax - tickMin;
	
	if (range > 10) {
	    /* There are too many decades to display a major tick at every
	     * decade.  Instead, treat the axis as a linear scale.  */
	    range = NiceNum(range, 0);
	    majorStep = NiceNum(range / axisPtr->reqNumMajorTicks, 1);
	    tickMin = UFLOOR(tickMin, majorStep);
	    tickMax = UCEIL(tickMax, majorStep);
	    numMajor = (int)((tickMax - tickMin) / majorStep) + 1;
	    minorStep = EXP10(floor(log10(majorStep)));
	    if (minorStep == majorStep) {
		nMinor = 4, minorStep = 0.2;
	    } else {
		nMinor = Round(majorStep / minorStep) - 1;
	    }
	} else {
	    if (tickMin == tickMax) {
		tickMax++;
	    }
	    majorStep = 1.0;
	    numMajor = (int)(tickMax - tickMin + 1); /* FIXME: Check this. */
	    
	    minorStep = 0.0;		/* This is a special hack to pass
					 * information to the GenerateTicks
					 * routine. An interval of 0.0 tells 1)
					 * this is a minor sweep and 2) the axis
					 * is log scale. */
	    numMinor = 10;
	}
	if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) && 
	     (DEFINED(axisPtr->reqMin)))) {
	    tickMin = min;
	    numMajor++;
	}
	if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
	     (DEFINED(axisPtr->reqMax)))) {
	    tickMax = max;
	}
    }
    axisPtr->majorSweep.step = majorStep;
    axisPtr->majorSweep.initial = floor(tickMin);
    axisPtr->majorSweep.numSteps = numMajor;
    axisPtr->minorSweep.initial = axisPtr->minorSweep.step = minorStep;
    axisPtr->minorSweep.numSteps = numMinor;
    SetAxisRange(&axisPtr->axisRange, tickMin, tickMax);
}

/*
 *---------------------------------------------------------------------------
 *
 * LinearAxis --
 *
 * 	Determine the units of a linear scaled axis.
 *
 *	The axis limits are either the range of the data values mapped
 *	to the axis (autoscaled), or the values specified by the -min
 *	and -max options (manual).
 *
 *	If autoscaled, the smallest and largest major ticks will
 *	encompass the range of data values.  If the -loose option is
 *	selected, the next outer ticks are choosen.  If tight, the
 *	ticks are at or inside of the data limits are used.
 *
 * 	If manually set, the ticks are at or inside the data limits
 * 	are used.  This makes sense for zooming.  You want the
 * 	selected range to represent the next limit, not something a
 * 	bit bigger.
 *
 *	Note: I added an "always" value to the -loose option to force
 *	      the manually selected axes to be loose. It's probably
 *	      not a good idea.
 *
 *          maxY
 *            |    units = magnitude (of least significant digit)
 *            |    high  = largest unit tick < max axis value
 *      high _|    low   = smallest unit tick > min axis value
 *            |
 *            |    range = high - low
 *            |    # ticks = greatest factor of range/units
 *           _|
 *        U   |
 *        n   |
 *        i   |
 *        t  _|
 *            |
 *            |
 *            |
 *       low _|
 *            |
 *            |_minX________________maxX__
 *            |   |       |      |       |
 *     minY  low                        high
 *           minY
 *
 * 	numTicks = Number of ticks
 * 	min = Minimum value of axis
 * 	max = Maximum value of axis
 * 	range    = Range of values (max - min)
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The axis tick information is set.  The actual tick values will
 *	be generated later.
 *
 *---------------------------------------------------------------------------
 */
static void
LinearAxis(Axis *axisPtr, double min, double max)
{
    double step;
    double tickMin, tickMax;
    double axisMin, axisMax;
    unsigned int numTicks;

    numTicks = 0;
    step = 1.0;
    /* Suppress compiler warning. */
    axisMin = axisMax = tickMin = tickMax = Blt_NaN();
    if (min < max) {
	double range;

	range = max - min;
	/* Calculate the major tick stepping. */
	if (axisPtr->reqStep > 0.0) {
	    /* An interval was designated by the user.  Keep scaling it until
	     * it fits comfortably within the current range of the axis.  */
	    step = axisPtr->reqStep;
	    while ((2 * step) >= range) {
		step *= 0.5;
	    }
	} else {
	    range = NiceNum(range, 0);
	    step = NiceNum(range / axisPtr->reqNumMajorTicks, 1);
	}
	
	/* Find the outer tick values. Add 0.0 to prevent getting -0.0. */
	axisMin = tickMin = floor(min / step) * step + 0.0;
	axisMax = tickMax = ceil(max / step) * step + 0.0;
	
	nTicks = Round((tickMax - tickMin) / step) + 1;
    } 
    axisPtr->majorSweep.step = step;
    axisPtr->majorSweep.initial = tickMin;
    axisPtr->majorSweep.numSteps = numTicks;

    /*
     * The limits of the axis are either the range of the data ("tight") or at
     * the next outer tick interval ("loose").  The looseness or tightness has
     * to do with how the axis fits the range of data values.  This option is
     * overridden when the user sets an axis limit (by either -min or -max
     * option).  The axis limit is always at the selected limit (otherwise we
     * assume that user would have picked a different number).
     */
    if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) &&
	 (DEFINED(axisPtr->reqMin)))) {
	axisMin = min;
    }
    if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
	 (DEFINED(axisPtr->reqMax)))) {
	axisMax = max;
    }
    SetAxisRange(&axisPtr->axisRange, axisMin, axisMax);

    /* Now calculate the minor tick step and number. */

    if ((axisPtr->reqNumMinorTicks > 0) && (axisPtr->flags & AUTO_MAJOR)) {
	nTicks = axisPtr->reqNumMinorTicks - 1;
	step = 1.0 / (nTicks + 1);
    } else {
	nTicks = 0;			/* No minor ticks. */
	step = 0.5;			/* Don't set the minor tick interval
					 * to 0.0. It makes the GenerateTicks
					 * routine * create minor log-scale
					 * tick marks.  */
    }
    axisPtr->minorSweep.initial = axisPtr->minorSweep.step = step;
    axisPtr->minorSweep.numSteps = numTicks;
}


static void
SweepTicks(Axis *axisPtr)
{
    if (axisPtr->flags & AUTO_MAJOR) {
	if (axisPtr->t1Ptr != NULL) {
	    Blt_Free(axisPtr->t1Ptr);
	}
	axisPtr->t1Ptr = GenerateTicks(&axisPtr->majorSweep);
    }
    if (axisPtr->flags & AUTO_MINOR) {
	if (axisPtr->t2Ptr != NULL) {
	    Blt_Free(axisPtr->t2Ptr);
	}
	axisPtr->t2Ptr = GenerateTicks(&axisPtr->minorSweep);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResetAxes --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ResetAxes(Graph *graphPtr)
{
    Blt_ChainLink link;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    /* FIXME: This should be called whenever the display list of
     *	      elements change. Maybe yet another flag INIT_STACKS to
     *	      indicate that the element display list has changed.
     *	      Needs to be done before the axis limits are set.
     */
    Blt_InitSetTable(graphPtr);
    if ((graphPtr->mode == BARS_STACKED) && (graphPtr->numBarGroups > 0)) {
	Blt_ComputeStacks(graphPtr);
    }
    /*
     * Step 1:  Reset all axes. Initialize the data limits of the axis to
     *		impossible values.
     */
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
	hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Axis *axisPtr;

	axisPtr = Blt_GetHashValue(hPtr);
	axisPtr->min = axisPtr->valueRange.min = DBL_MAX;
	axisPtr->max = axisPtr->valueRange.max = -DBL_MAX;
	if (AxisIsHorizontal(axisPtr)) {
	    axisPtr->flags |= HORIZONTAL;
	} else {
	    axisPtr->flags &= ~HORIZONTAL;
	}
    }

    /*
     * Step 2:  For each element that's to be displayed, get the smallest
     *		and largest data values mapped to each X and Y-axis.  This
     *		will be the axis limits if the user doesn't override them 
     *		with -min and -max options.
     */
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if ((graphPtr->flags & UNMAP_HIDDEN) && (elemPtr->flags & HIDE)) {
	    continue;
	}
	(*elemPtr->procsPtr->extentsProc) (elemPtr);
    }
    /*
     * Step 3:  Now that we know the range of data values for each axis,
     *		set axis limits and compute a sweep to generate tick values.
     */
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Axis *axisPtr;
	double min, max;

	axisPtr = Blt_GetHashValue(hPtr);
	FixAxisRange(axisPtr);

	/* Calculate min/max tick (major/minor) layouts */
	min = axisPtr->min;
	max = axisPtr->max;
	if ((DEFINED(axisPtr->scrollMin)) && (min < axisPtr->scrollMin)) {
	    min = axisPtr->scrollMin;
	}
	if ((DEFINED(axisPtr->scrollMax)) && (max > axisPtr->scrollMax)) {
	    max = axisPtr->scrollMax;
	}
	if (axisPtr->logScale) {
	    LogAxis(axisPtr, min, max);
	} else if (axisPtr->timeScale) {
	    TimeAxis(axisPtr, min, max);
	} else {
	    LinearAxis(axisPtr, min, max);
	}
	if ((axisPtr->flags & (DIRTY|USE)) == (DIRTY|USE)) {
	    graphPtr->flags |= CACHE_DIRTY;
	}
    }

    graphPtr->flags &= ~RESET_AXES;

    /*
     * When any axis changes, we need to layout the entire graph.
     */
    graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | 
			MAP_ALL | REDRAW_WORLD);
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetTextStyles --
 *
 *	Configures axis attributes (font, line width, label, etc) and
 *	allocates a new (possibly shared) graphics context.  Line cap style is
 *	projecting.  This is for the problem of when a tick sits directly at
 *	the end point of the axis.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 * Side Effects:
 *	Axis resources are allocated (GC, font). Axis layout is deferred until
 *	the height and width of the window are known.
 *
 *---------------------------------------------------------------------------
 */
static void
ResetTextStyles(Axis *axisPtr)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    Blt_Ts_ResetStyle(graphPtr->tkwin, &axisPtr->limitsTextStyle);

    gcMask = (GCForeground | GCLineWidth | GCCapStyle);
    gcValues.foreground = axisPtr->tickColor->pixel;
    gcValues.font = Blt_Font_Id(axisPtr->tickFont);
    gcValues.line_width = LineWidth(axisPtr->lineWidth);
    gcValues.cap_style = CapProjecting;

    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (axisPtr->tickGC != NULL) {
	Tk_FreeGC(graphPtr->display, axisPtr->tickGC);
    }
    axisPtr->tickGC = newGC;

    /* Assuming settings from above GC */
    gcValues.foreground = axisPtr->activeFgColor->pixel;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (axisPtr->activeTickGC != NULL) {
	Tk_FreeGC(graphPtr->display, axisPtr->activeTickGC);
    }
    axisPtr->activeTickGC = newGC;

    gcValues.background = gcValues.foreground = axisPtr->major.color->pixel;
    gcValues.line_width = LineWidth(axisPtr->major.lineWidth);
    gcMask = (GCForeground | GCBackground | GCLineWidth);
    if (LineIsDashed(axisPtr->major.dashes)) {
	gcValues.line_style = LineOnOffDash;
	gcMask |= GCLineStyle;
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(axisPtr->major.dashes)) {
	Blt_SetDashes(graphPtr->display, newGC, &axisPtr->major.dashes);
    }
    if (axisPtr->major.gc != NULL) {
	Blt_FreePrivateGC(graphPtr->display, axisPtr->major.gc);
    }
    axisPtr->major.gc = newGC;

    gcValues.background = gcValues.foreground = axisPtr->minor.color->pixel;
    gcValues.line_width = LineWidth(axisPtr->minor.lineWidth);
    gcMask = (GCForeground | GCBackground | GCLineWidth);
    if (LineIsDashed(axisPtr->minor.dashes)) {
	gcValues.line_style = LineOnOffDash;
	gcMask |= GCLineStyle;
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(axisPtr->minor.dashes)) {
	Blt_SetDashes(graphPtr->display, newGC, &axisPtr->minor.dashes);
    }
    if (axisPtr->minor.gc != NULL) {
	Blt_FreePrivateGC(graphPtr->display, axisPtr->minor.gc);
    }
    axisPtr->minor.gc = newGC;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyAxis --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources (font, color, gc, labels, etc.) associated with the axis are
 *	deallocated.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyColorbar(Colorbar *cbarPtr)
{
    Graph *graphPtr = cbarPtr->obj.graphPtr;
    int flags;

    flags = Blt_GraphType(graphPtr);
    Blt_FreeOptions(configSpecs, (char *)cbarPtr, graphPtr->display, flags);
    if (cbarPtr->bindTable != NULL) {
	Blt_DeleteBindings(cbarPtr->bindTable, cbarPtr);
    }
    if (cbarPtr->tickGC != NULL) {
	Tk_FreeGC(graphPtr->display, cbarPtr->tickGC);
    }
    if (cbarPtr->activeTickGC != NULL) {
	Tk_FreeGC(graphPtr->display, cbarPtr->activeTickGC);
    }
    if (cbarPtr->major.gc != NULL) {
	Blt_FreePrivateGC(graphPtr->display, cbarPtr->major.gc);
    }
    if (cbarPtr->minor.gc != NULL) {
	Blt_FreePrivateGC(graphPtr->display, cbarPtr->minor.gc);
    }
    FreeTickLabels(cbarPtr->tickLabels);
    Blt_Chain_Destroy(cbarPtr->tickLabels);
    if (cbarPtr->segments != NULL) {
	Blt_Free(cbarPtr->segments);
    }
    Blt_Free(cbarPtr);
}

static void
FreeColorbar(DestroyData data)
{
    Colorbar *cbarPtr = (Colorbar *)data;

    DestroyColorbar(cbarPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisOffsets --
 *
 *	Determines the sites of the axis, major and minor ticks, and title of
 *	the axis.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
ColorbarOffsets(
    Colorbar *cbarPtr,
    int margin,
    int offset,
    AxisInfo *infoPtr)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    Margin *marginPtr;
    int pad;				/* Offset of axis from interior
					 * region. This includes a possible
					 * border and the axis line width. */
    int axisLine;
    int t1, t2, labelOffset;
    int tickLabel, axisPad;
    int inset, mark;
    int x, y;
    float fangle;

    cbarPtr->titleAngle = titleAngle[margin];
    marginPtr = graphPtr->margins + margin;

    tickLabel = axisLine = t1 = t2 = 0;
    labelOffset = AXIS_PAD_TITLE;
    if (cbarPtr->lineWidth > 0) {
	if (cbarPtr->flags & SHOWTICKS) {
	    t1 = cbarPtr->tickLength;
	    t2 = (t1 * 10) / 15;
	}
	labelOffset = t1 + AXIS_PAD_TITLE;
	if (cbarPtr->flags & EXTERIOR) {
	    labelOffset += cbarPtr->lineWidth;
	}
    }
    /* Adjust offset for the interior border width and the line width */
    pad = 1;
    if (graphPtr->plotBW > 0) {
	pad += graphPtr->plotBW + 1;
    }
    pad = 0;				/* FIXME: test */
    /*
     * Pre-calculate the x-coordinate positions of the axis, tick labels, and
     * the individual major and minor ticks.
     */
    inset = pad + cbarPtr->lineWidth / 2;
    switch (cbarPtr->flags & ORIENTATION) {
    case ORIENT_HORIZONAL:
	axisLine = graphPtr->top;
	if (axisPtr->flags & EXTERIOR) {
	    axisLine -= graphPtr->plotBW + cbarPtr->lineWidth / 2;
	    tickLabel = axisLine - 2;
	    if (axisPtr->lineWidth > 0) {
		tickLabel -= cbarPtr->tickLength;
	    }
#ifdef notdef
	    fprintf(stderr, "axisLine=%d, axisPad=%d plotBW=%d axisPtr->lineWidth/2=%d\n", axisLine, axisPad, graphPtr->plotBW, axisPtr->lineWidth /2 );
#endif
	} else {
	    axisLine -= cbarPtr->lineWidth / 2;
	    tickLabel = graphPtr->top -  graphPtr->plotBW - 2;
#ifdef notdef
	    fprintf(stderr, "axisLine=%d, axisPad=%d plotBW=%d axisPtr->lineWidth/2=%d\n", axisLine, axisPad, graphPtr->plotBW, axisPtr->lineWidth /2 );
#endif
	}
	mark = graphPtr->top - offset - pad;
	cbarPtr->tickAnchor = TK_ANCHOR_S;
	cbarPtr->left = cbarPtr->screenMin - inset - 2;
	cbarPtr->right = cbarPtr->screenMin + cbarPtr->screenRange + inset - 1;
	cbarPtr->top = mark - cbarPtr->height;
	cbarPtr->bottom = mark;
	x = (cbarPtr->right + cbarPtr->left) / 2;
	y = mark - cbarPtr->height + AXIS_PAD_TITLE;
	cbarPtr->titleAnchor = TK_ANCHOR_N;
	cbarPtr->titlePos.x = x;
	cbarPtr->titlePos.y = y;
	break;

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
	    axisPtr->bottom = mark + marginPtr->axesOffset - 1;
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
		y = mark + marginPtr->axesOffset - AXIS_PAD_TITLE;
	    } else {
		y = mark + axisPtr->height - AXIS_PAD_TITLE;
	    }
	    axisPtr->titleAnchor = TK_ANCHOR_S; 
	}
	axisPtr->titlePos.x = x;
	axisPtr->titlePos.y = y;
	break;

    case ORIENT_VERTICAL:
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
	if (graphPtr->stackAxes) {
	    axisPtr->left = mark - marginPtr->axesOffset;
	} else {
	    axisPtr->left = mark - axisPtr->width;
	}
	axisPtr->right = mark - 3;
	axisPtr->top = axisPtr->screenMin - inset - 2;
	axisPtr->bottom = axisPtr->screenMin + axisPtr->screenRange + inset - 1;
	if (axisPtr->titleAlternate) {
	    x = mark - (axisPtr->width / 2);
	    y = graphPtr->top - AXIS_PAD_TITLE;
	    axisPtr->titleAnchor = TK_ANCHOR_SW; 
	} else {
	    if (graphPtr->stackAxes) {
		x = mark - marginPtr->axesOffset;
	    } else {
		x = mark - axisPtr->width + AXIS_PAD_TITLE;
	    }
	    y = (axisPtr->bottom + axisPtr->top) / 2;
	    axisPtr->titleAnchor = TK_ANCHOR_W; 
	} 
	axisPtr->titlePos.x = x;
	axisPtr->titlePos.y = y;
	break;
    }
    if ((margin == MARGIN_LEFT) || (margin == MARGIN_TOP)) {
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
MakeAxisLine(Colorbar *cbarPtr, int line, Segment2d *s)
{
    double min, max;

    min = cbarPtr->axisRange.min;
    max = cbarPtr->axisRange.max;
    if (cbarPtr->logScale) {
	min = EXP10(min);
	max = EXP10(max);
    }
    if (cbarPtr->flags & HORIZONTAL) {
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
    numMajorTicks = numMinorTicks = 0;
    if (axisPtr->t1Ptr != NULL) {
	nMajorTicks = axisPtr->t1Ptr->numTicks;
    }
    if (axisPtr->t2Ptr != NULL) {
	nMinorTicks = axisPtr->t2Ptr->numTicks;
    }
    arraySize = 1 + (nMajorTicks * (nMinorTicks + 1));
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
	int i;

	for (i = 0; i < numMajorTicks; i++) {
	    double t1, t2;
	    int j;

	    t1 = axisPtr->t1Ptr->values[i];
	    /* Minor ticks */
	    for (j = 0; j < numMinorTicks; j++) {
		t2 = t1 + (axisPtr->majorSweep.step * 
			   axisPtr->t2Ptr->values[j]);
		if (InRange(t2, &axisPtr->axisRange)) {
		    MakeTick(axisPtr, t2, infoPtr->t2, infoPtr->axis, s);
		    s++;
		}
	    }
	    if (!InRange(t1, &axisPtr->axisRange)) {
		continue;
	    }
	    /* Major tick */
	    MakeTick(axisPtr, t1, infoPtr->t1, infoPtr->axis, s);
	    s++;
	}

	link = Blt_Chain_FirstLink(axisPtr->tickLabels);
	labelPos = (double)infoPtr->label;

	for (i = 0; i < numMajorTicks; i++) {
	    double t1;
	    TickLabel *labelPtr;
	    Segment2d seg;

	    t1 = axisPtr->t1Ptr->values[i];
	    if (axisPtr->labelOffset) {
		t1 += axisPtr->majorSweep.step * 0.5;
	    }
	    if (!InRange(t1, &axisPtr->axisRange)) {
		continue;
	    }
	    labelPtr = Blt_Chain_GetValue(link);
	    link = Blt_Chain_NextLink(link);
	    MakeTick(axisPtr, t1, infoPtr->t1, infoPtr->axis, &seg);
	    /* Save tick label X-Y position. */
	    if (axisPtr->flags & HORIZONTAL) {
		labelPtr->anchorPos.x = seg.p.x;
		labelPtr->anchorPos.y = labelPos;
	    } else {
		labelPtr->anchorPos.x = labelPos;
		labelPtr->anchorPos.y = seg.p.y;
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
MapAxis(Axis *axisPtr, int offset, int margin)
{
    AxisInfo info;
    Graph *graphPtr = axisPtr->obj.graphPtr;

    if (axisPtr->flags & HORIZONTAL) {
	axisPtr->screenMin = graphPtr->hOffset;
	axisPtr->width = graphPtr->right - graphPtr->left;
	axisPtr->screenRange = graphPtr->hRange;
    } else {
	axisPtr->screenMin = graphPtr->vOffset;
	axisPtr->height = graphPtr->bottom - graphPtr->top;
	axisPtr->screenRange = graphPtr->vRange;
    }
    axisPtr->screenScale = 1.0 / axisPtr->screenRange;
    AxisOffsets(axisPtr, margin, offset, &info);
    MakeSegments(axisPtr, &info);
}

/*
 *---------------------------------------------------------------------------
 *
 * MapStackedAxis --
 *
 *	Pre-calculates positions of the axis, ticks, and labels (to be used
 *	later when displaying the axis).  Calculates the values for each major
 *	and minor tick and checks to see if they are in range (the outer ticks
 *	may be outside of the range of plotted values).
 *
 *	Line segments for the minor and major ticks are saved into one XSegment
 *	array so that they can be drawn by a single XDrawSegments call. The
 *	positions of the tick labels are also computed and saved.
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
MapStackedAxis(Axis *axisPtr, int count, int margin)
{
    AxisInfo info;
    Graph *graphPtr = axisPtr->obj.graphPtr;
    unsigned int slice, n, w, h;
   
    if ((graphPtr->margins[axisPtr->margin].axes->numLinks > 1) ||
	(axisPtr->reqNumMajorTicks <= 0)) {
	axisPtr->reqNumMajorTicks = 4;
    }
    n = graphPtr->margins[margin].axes->numLinks;
    if (axisPtr->flags & HORIZONTAL) {
	slice = graphPtr->hRange / n;
	axisPtr->screenMin = graphPtr->hOffset;
	axisPtr->screenRange = graphPtr->hRange;
	axisPtr->width = slice;
    } else {
	slice = graphPtr->vRange / n;
	axisPtr->screenMin = graphPtr->vOffset;
	axisPtr->screenMin = graphPtr->vOffset;
	axisPtr->screenRange = graphPtr->vRange;
	axisPtr->height = slice;
    }
#define AXIS_PAD 2
    Blt_GetTextExtents(axisPtr->tickFont, 0, "0", 1, &w, &h);
    if (n > 1) {
	axisPtr->screenMin += (slice * count) + AXIS_PAD + h / 2;
	axisPtr->screenRange = slice - 2 * AXIS_PAD - h;
    }
    axisPtr->screenScale = 1.0f / axisPtr->screenRange;
    AxisOffsets(axisPtr, margin, 0, &info);
    MakeSegments(axisPtr, &info);
}

/*
 *---------------------------------------------------------------------------
 *
 * AdjustViewport --
 *
 *	Adjusts the offsets of the viewport according to the scroll mode.  This
 *	is to accommodate both "listbox" and "canvas" style scrolling.
 *
 *	"canvas"	The viewport scrolls within the range of world
 *			coordinates.  This way the viewport always displays
 *			a full page of the world.  If the world is smaller
 *			than the viewport, then (bizarrely) the world and
 *			viewport are inverted so that the world moves up
 *			and down within the viewport.
 *
 *	"listbox"	The viewport can scroll beyond the range of world
 *			coordinates.  Every entry can be displayed at the
 *			top of the viewport.  This also means that the
 *			scrollbar thumb weirdly shrinks as the last entry
 *			is scrolled upward.
 *
 * Results:
 *	The corrected offset is returned.
 *
 *---------------------------------------------------------------------------
 */
static double
AdjustViewport(double offset, double windowSize)
{
    /*
     * Canvas-style scrolling allows the world to be scrolled within the window.
     */
    if (windowSize > 1.0) {
	if (windowSize < (1.0 - offset)) {
	    offset = 1.0 - windowSize;
	}
	if (offset > 0.0) {
	    offset = 0.0;
	}
    } else {
	if ((offset + windowSize) > 1.0) {
	    offset = 1.0 - windowSize;
	}
	if (offset < 0.0) {
	    offset = 0.0;
	}
    }
    return offset;
}

static int
GetAxisScrollInfo(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv,
    double *offsetPtr,
    double windowSize,
    double scrollUnits,
    double scale)
{
    const char *string;
    char c;
    double offset;
    int length;

    offset = *offsetPtr;
    string = Tcl_GetStringFromObj(objv[0], &length);
    c = string[0];
    scrollUnits *= scale;
    if ((c == 's') && (strncmp(string, "scroll", length) == 0)) {
	int count;
	double fract;

	assert(objc == 3);
	/* Scroll number unit/page */
	if (Tcl_GetIntFromObj(interp, objv[1], &count) != TCL_OK) {
	    return TCL_ERROR;
	}
	string = Tcl_GetStringFromObj(objv[2], &length);
	c = string[0];
	if ((c == 'u') && (strncmp(string, "units", length) == 0)) {
	    fract = count * scrollUnits;
	} else if ((c == 'p') && (strncmp(string, "pages", length) == 0)) {
	    /* A page is 90% of the view-able window. */
	    fract = (int)(count * windowSize * 0.9 + 0.5);
	} else if ((c == 'p') && (strncmp(string, "pixels", length) == 0)) {
	    fract = count * scale;
	} else {
	    Tcl_AppendResult(interp, "unknown \"scroll\" units \"", string,
		"\"", (char *)NULL);
	    return TCL_ERROR;
	}
	offset += fract;
    } else if ((c == 'm') && (strncmp(string, "moveto", length) == 0)) {
	double fract;

	assert(objc == 2);
	/* moveto fraction */
	if (Blt_GetDoubleFromObj(interp, objv[1], &fract) != TCL_OK) {
	    return TCL_ERROR;
	}
	offset = fract;
    } else {
	int count;
	double fract;

	/* Treat like "scroll units" */
	if (Tcl_GetIntFromObj(interp, objv[0], &count) != TCL_OK) {
	    return TCL_ERROR;
	}
	fract = (double)count * scrollUnits;
	offset += fract;
	/* CHECK THIS: return TCL_OK; */
    }
    *offsetPtr = AdjustViewport(offset, windowSize);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawAxis --
 *
 *	Draws the axis, ticks, and labels onto the canvas.
 *
 *	Initializes and passes text attribute information through TextStyle
 *	structure.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Axis gets drawn on window.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawAxis(Axis *axisPtr, Drawable drawable)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    int isHoriz;

    if (axisPtr->normalBg != NULL) {
	Blt_Bg_FillRectangle(graphPtr->tkwin, drawable, 
		axisPtr->normalBg, 
		axisPtr->left, axisPtr->top, 
		axisPtr->right - axisPtr->left, 
		axisPtr->bottom - axisPtr->top, axisPtr->borderWidth, 
		axisPtr->relief);
    }
    if (axisPtr->title != NULL) {
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetAngle(ts, axisPtr->titleAngle);
	Blt_Ts_SetFont(ts, axisPtr->titleFont);
	Blt_Ts_SetPadding(ts, 1, 2, 0, 0);
	Blt_Ts_SetAnchor(ts, axisPtr->titleAnchor);
	Blt_Ts_SetJustify(ts, axisPtr->titleJustify);
	if (axisPtr->flags & ACTIVE) {
	    Blt_Ts_SetForeground(ts, axisPtr->activeFgColor);
	} else {
	    Blt_Ts_SetForeground(ts, axisPtr->titleColor);
	}
	Blt_Ts_SetForeground(ts, axisPtr->titleColor);
	if ((axisPtr->titleAngle == 90.0) || (axisPtr->titleAngle == 270.0)) {
	    Blt_Ts_SetMaxLength(ts, axisPtr->height);
	} else {
	    Blt_Ts_SetMaxLength(ts, axisPtr->width);
	}
	Blt_Ts_DrawText(graphPtr->tkwin, drawable, axisPtr->title, -1, &ts, 
		(int)axisPtr->titlePos.x, (int)axisPtr->titlePos.y);
    }
    if (axisPtr->scrollCmdObjPtr != NULL) {
	double viewWidth, viewMin, viewMax;
	double worldWidth, worldMin, worldMax;
	double fract;

	worldMin = axisPtr->valueRange.min;
	worldMax = axisPtr->valueRange.max;
	if (DEFINED(axisPtr->scrollMin)) {
	    worldMin = axisPtr->scrollMin;
	}
	if (DEFINED(axisPtr->scrollMax)) {
	    worldMax = axisPtr->scrollMax;
	}
	viewMin = axisPtr->min;
	viewMax = axisPtr->max;
	if (viewMin < worldMin) {
	    viewMin = worldMin;
	}
	if (viewMax > worldMax) {
	    viewMax = worldMax;
	}
	if (axisPtr->logScale) {
	    worldMin = log10(worldMin);
	    worldMax = log10(worldMax);
	    viewMin = log10(viewMin);
	    viewMax = log10(viewMax);
	}
	worldWidth = worldMax - worldMin;	
	viewWidth = viewMax - viewMin;
	isHoriz = ((axisPtr->flags & HORIZONTAL) == HORIZONTAL);

	if (isHoriz != axisPtr->descending) {
	    fract = (viewMin - worldMin) / worldWidth;
	} else {
	    fract = (worldMax - viewMax) / worldWidth;
	}
	fract = AdjustViewport(fract, viewWidth / worldWidth);

	if (isHoriz != axisPtr->descending) {
	    viewMin = (fract * worldWidth);
	    axisPtr->min = viewMin + worldMin;
	    axisPtr->max = axisPtr->min + viewWidth;
	    viewMax = viewMin + viewWidth;
	    if (axisPtr->logScale) {
		axisPtr->min = EXP10(axisPtr->min);
		axisPtr->max = EXP10(axisPtr->max);
	    }
	    Blt_UpdateScrollbar(graphPtr->interp, axisPtr->scrollCmdObjPtr,
		(int)viewMin, (int)viewMax, (int)worldWidth);
	} else {
	    viewMax = (fract * worldWidth);
	    axisPtr->max = worldMax - viewMax;
	    axisPtr->min = axisPtr->max - viewWidth;
	    viewMin = viewMax + viewWidth;
	    if (axisPtr->logScale) {
		axisPtr->min = EXP10(axisPtr->min);
		axisPtr->max = EXP10(axisPtr->max);
	    }
	    Blt_UpdateScrollbar(graphPtr->interp, axisPtr->scrollCmdObjPtr,
		(int)viewMax, (int)viewMin, (int)worldWidth);
	}
    }
    if (axisPtr->flags & SHOWTICKS) {
	Blt_ChainLink link;
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetAngle(ts, axisPtr->tickAngle);
	Blt_Ts_SetFont(ts, axisPtr->tickFont);
	Blt_Ts_SetPadding(ts, 2, 0, 0, 0);
	Blt_Ts_SetAnchor(ts, axisPtr->tickAnchor);
	if (axisPtr->flags & ACTIVE) {
	    Blt_Ts_SetForeground(ts, axisPtr->activeFgColor);
	} else {
	    Blt_Ts_SetForeground(ts, axisPtr->tickColor);
	}
	for (link = Blt_Chain_FirstLink(axisPtr->tickLabels); link != NULL;
	    link = Blt_Chain_NextLink(link)) {	
	    TickLabel *labelPtr;

	    labelPtr = Blt_Chain_GetValue(link);
	    /* Draw major tick labels */
	    Blt_DrawText(graphPtr->tkwin, drawable, labelPtr->string, &ts, 
		(int)labelPtr->anchorPos.x, (int)labelPtr->anchorPos.y);
	}
    }
    if ((axisPtr->numSegments > 0) && (axisPtr->lineWidth > 0)) {	
	GC gc;

	if (axisPtr->flags & ACTIVE) {
	    gc = axisPtr->activeTickGC;
	} else {
	    gc = axisPtr->tickGC;
	}
	/* Draw the tick marks and axis line. */
	Blt_DrawSegments2d(graphPtr->display, drawable, gc, axisPtr->segments, 
		axisPtr->numSegments);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisToPostScript --
 *
 *	Generates PostScript output to draw the axis, ticks, and labels.
 *
 *	Initializes and passes text attribute information through TextStyle
 *	structure.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	PostScript output is left in graphPtr->interp->result;
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
AxisToPostScript(Blt_Ps ps, Axis *axisPtr)
{
    Blt_Ps_Format(ps, "%% Axis \"%s\"\n", axisPtr->obj.name);
    if (axisPtr->normalBg != NULL) {
	Tk_3DBorder border;

	border = Blt_Bg_Border(axisPtr->normalBg);
	Blt_Ps_Fill3DRectangle(ps, border, 
		(double)axisPtr->left, (double)axisPtr->top, 
		axisPtr->right - axisPtr->left, axisPtr->bottom - axisPtr->top, 
		axisPtr->borderWidth, axisPtr->relief);
    }
    if (axisPtr->title != NULL) {
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetAngle(ts, axisPtr->titleAngle);
	Blt_Ts_SetFont(ts, axisPtr->titleFont);
	Blt_Ts_SetPadding(ts, 1, 2, 0, 0);
	Blt_Ts_SetAnchor(ts, axisPtr->titleAnchor);
	Blt_Ts_SetJustify(ts, axisPtr->titleJustify);
	Blt_Ts_SetForeground(ts, axisPtr->titleColor);
	Blt_Ps_DrawText(ps, axisPtr->title, &ts, axisPtr->titlePos.x, 
		axisPtr->titlePos.y);
    }
    if (axisPtr->flags & SHOWTICKS) {
	Blt_ChainLink link;
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetAngle(ts, axisPtr->tickAngle);
	Blt_Ts_SetFont(ts, axisPtr->tickFont);
	Blt_Ts_SetPadding(ts, 2, 0, 0, 0);
	Blt_Ts_SetAnchor(ts, axisPtr->tickAnchor);
	Blt_Ts_SetForeground(ts, axisPtr->tickColor);

	for (link = Blt_Chain_FirstLink(axisPtr->tickLabels); link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    TickLabel *labelPtr;

	    labelPtr = Blt_Chain_GetValue(link);
	    Blt_Ps_DrawText(ps, labelPtr->string, &ts, labelPtr->anchorPos.x, 
		labelPtr->anchorPos.y);
	}
    }
    if ((axisPtr->numSegments > 0) && (axisPtr->lineWidth > 0)) {
	Blt_Ps_XSetLineAttributes(ps, axisPtr->tickColor, axisPtr->lineWidth, 
		(Blt_Dashes *)NULL, CapButt, JoinMiter);
	Blt_Ps_DrawSegments2d(ps, axisPtr->numSegments, axisPtr->segments);
    }
}

static void
MakeGridLine(Axis *axisPtr, double value, Segment2d *s)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;

    if (axisPtr->logScale) {
	value = EXP10(value);
    }
    /* Grid lines run orthogonally to the axis */
    if (axisPtr->flags & HORIZONTAL) {
	s->p.y = graphPtr->top;
	s->q.y = graphPtr->bottom;
	s->p.x = s->q.x = Blt_HMap(axisPtr, value);
    } else {
	s->p.x = graphPtr->left;
	s->q.x = graphPtr->right;
	s->p.y = s->q.y = Blt_VMap(axisPtr, value);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapGridlines --
 *
 *	Assembles the grid lines associated with an axis. Generates tick
 *	positions if necessary (this happens when the axis is not a logical axis
 *	too).
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
MapGridlines(Axis *axisPtr)
{
    Segment2d *s1, *s2;
    Ticks *t1Ptr, *t2Ptr;
    int needed;
    int i;

    if (axisPtr == NULL) {
	return;
    }
    t1Ptr = axisPtr->t1Ptr;
    if (t1Ptr == NULL) {
	t1Ptr = GenerateTicks(&axisPtr->majorSweep);
    }
    t2Ptr = axisPtr->t2Ptr;
    if (t2Ptr == NULL) {
	t2Ptr = GenerateTicks(&axisPtr->minorSweep);
    }
    needed = t1Ptr->numTicks;
    if (axisPtr->flags & GRIDMINOR) {
	needed += (t1Ptr->numTicks * t2Ptr->numTicks);
    }
    if (needed == 0) {
	return;			
    }
    needed = t1Ptr->numTicks;
    if (needed != axisPtr->major.numAllocated) {
	if (axisPtr->major.segments != NULL) {
	    Blt_Free(axisPtr->major.segments);
	}
	axisPtr->major.segments = Blt_AssertMalloc(sizeof(Segment2d) * needed);
	axisPtr->major.numAllocated = needed;
    }
    needed = (t1Ptr->numTicks * t2Ptr->numTicks);
    if (needed != axisPtr->minor.numAllocated) {
	if (axisPtr->minor.segments != NULL) {
	    Blt_Free(axisPtr->minor.segments);
	}
	axisPtr->minor.segments = Blt_AssertMalloc(sizeof(Segment2d) * needed);
	axisPtr->minor.numAllocated = needed;
    }
    s1 = axisPtr->major.segments, s2 = axisPtr->minor.segments;
    for (i = 0; i < t1Ptr->numTicks; i++) {
	double value;

	value = t1Ptr->values[i];
	if (axisPtr->flags & GRIDMINOR) {
	    int j;

	    for (j = 0; j < t2Ptr->numTicks; j++) {
		double subValue;

		subValue = value + (axisPtr->majorSweep.step * 
				    t2Ptr->values[j]);
		if (InRange(subValue, &axisPtr->axisRange)) {
		    MakeGridLine(axisPtr, subValue, s2);
		    s2++;
		}
	    }
	}
	if (InRange(value, &axisPtr->axisRange)) {
	    MakeGridLine(axisPtr, value, s1);
	    s1++;
	}
    }
    if (t1Ptr != axisPtr->t1Ptr) {
	Blt_Free(t1Ptr);		/* Free generated ticks. */
    }
    if (t2Ptr != axisPtr->t2Ptr) {
	Blt_Free(t2Ptr);		/* Free generated ticks. */
    }
    axisPtr->major.numUsed = s1 - axisPtr->major.segments;
    axisPtr->minor.numUsed = s2 - axisPtr->minor.segments;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetAxisGeometry --
 *
 * Results:
 *	None.
 *
 * Exterior axis:
 *                    l       r
 *  |a|b|c|d|e|f|g|h|i|   j   |i|h|g|f|e|d|c|d|a|
 *
 * Interior axis: 
 *                  l           r
 *  |a|b|c|d|h|g|f|e|     j     |e|f|g|h|d|c|b|a|
 *               i..             ..i 
 * a = highlight thickness
 * b = graph borderwidth
 * c = axis title
 * d = tick label 
 * e = tick 
 * f = axis line
 * g = 1 pixel pad
 * h = plot borderwidth
 * i = plot pad
 * j = plot area 
 *---------------------------------------------------------------------------
 */
void
Blt_GetAxisGeometry(Graph *graphPtr, Axis *axisPtr)
{
    int y;

    FreeTickLabels(axisPtr->tickLabels);
    y = 0;

    if ((axisPtr->flags & EXTERIOR) && 
	(graphPtr->plotRelief != TK_RELIEF_SOLID)) {
	/* Leave room for axis baseline and padding */
	y += axisPtr->lineWidth + 2;
    }

    axisPtr->maxTickLabelHeight = axisPtr->maxTickLabelWidth = 0;
    if (axisPtr->flags & SHOWTICKS) {
	unsigned int pad;
	unsigned int i, numLabels, numTicks;

	SweepTicks(axisPtr);
	
	nTicks = 0;
	if (axisPtr->t1Ptr != NULL) {
	    numTicks = axisPtr->t1Ptr->numTicks;
	}
	assert(nTicks <= MAXTICKS);
	
	nLabels = 0;
	for (i = 0; i < numTicks; i++) {
	    TickLabel *labelPtr;
	    double x, x2;
	    int lw, lh;			/* Label width and height. */

	    x2 = x = axisPtr->t1Ptr->values[i];
	    if (axisPtr->labelOffset) {
		x2 += axisPtr->majorSweep.step * 0.5;
	    }
	    if (!InRange(x2, &axisPtr->axisRange)) {
		continue;
	    }
	    labelPtr = MakeLabel(axisPtr, x);
	    Blt_Chain_Append(axisPtr->tickLabels, labelPtr);
	    numLabels++;
	    /* 
	     * Get the dimensions of each tick label.  Remember tick labels
	     * can be multi-lined and/or rotated.
	     */
	    Blt_GetTextExtents(axisPtr->tickFont, 0, labelPtr->string, -1, 
		&labelPtr->width, &labelPtr->height);

	    if (axisPtr->tickAngle != 0.0f) {
		double rlw, rlh;	/* Rotated label width and height. */
		Blt_GetBoundingBox(labelPtr->width, labelPtr->height, 
			axisPtr->tickAngle, &rlw, &rlh, NULL);
		lw = ROUND(rlw), lh = ROUND(rlh);
	    } else {
		lw = labelPtr->width;
		lh = labelPtr->height;
	    }
	    if (axisPtr->maxTickLabelWidth < lw) {
		axisPtr->maxTickLabelWidth = lw;
	    }
	    if (axisPtr->maxTickLabelHeight < lh) {
		axisPtr->maxTickLabelHeight = lh;
	    }
	}
	assert(nLabels <= numTicks);
	
	pad = 0;
	if (axisPtr->flags & EXTERIOR) {
	    /* Because the axis cap style is "CapProjecting", we need to
	     * account for an extra 1.5 linewidth at the end of each line.  */
	    pad = ((axisPtr->lineWidth * 12) / 8);
	}
	if (axisPtr->flags & HORIZONTAL) {
	    y += axisPtr->maxTickLabelHeight + pad;
	} else {
	    y += axisPtr->maxTickLabelWidth + pad;
	    if (axisPtr->maxTickLabelWidth > 0) {
		y += 5;			/* Pad either size of label. */
	    }  
	}
	y += 2 * AXIS_PAD_TITLE;
	if ((axisPtr->lineWidth > 0) && (axisPtr->flags & EXTERIOR)) {
	    /* Distance from axis line to tick label. */
	    y += axisPtr->tickLength;
	}
    }

    if (axisPtr->title != NULL) {
	if (axisPtr->titleAlternate) {
	    if (y < axisPtr->titleHeight) {
		y = axisPtr->titleHeight;
	    }
	} else {
	    y += axisPtr->titleHeight + AXIS_PAD_TITLE;
	}
    }

    /* Correct for orientation of the axis. */
    if (axisPtr->flags & HORIZONTAL) {
	axisPtr->height = y;
    } else {
	axisPtr->width = y;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Colorbar_GetGeometry --
 *
 *	Examines all the axes in the given margin and determines the area
 *	required to display them.
 *
 *	Note: For multiple axes, the titles are displayed in another
 *	      margin. So we must keep track of the widest title.
 *	
 * Results:
 *	Returns the width or height of the margin, depending if it runs
 *	horizontally along the graph or vertically.
 *
 * Side Effects:
 *	The area width and height set in the margin.  Note again that this may
 *	be corrected later (mulitple axes) to adjust for the longest title in
 *	another margin.
 *
 *---------------------------------------------------------------------------
 */
static void
Blt_Colorbar_GetGeometry(Graph *graphPtr, Margin *marginPtr)
{
    Blt_ChainLink link;
    int l, w, h;			/* Length, width, and height. */
    int isHoriz;
    unsigned int numVisible;
    Axis *axisPtr;

    isHoriz = HORIZMARGIN(marginPtr);
    cbarPtr->height = cbarPtr->width = 0;
    if ((cbarPtr->axisPtr == NULL) || (cbarPtr->flags & HIDE) ||
	(cbarPtr->colormap == NULL)) {
	return;
    }
    axisPtr = cbarPtr->axisPtr;
    if (graphPtr->flags & GET_AXIS_GEOMETRY) {
	Blt_GetAxisGeometry(graphPtr, axisPtr);
    }
    if (isHoriz) {
	cbarPtr->height += axisPtr->height;
	cbarPtr->width += cbarPtr->barWidth + axisPtr->width;
    } else {
	cbarPtr->width += axisPtr->width;
	cbarPtr->height += cbarPtr->barWidth + axisPtr->height;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LayoutGraph --
 *
 *	Calculate the layout of the graph.  Based upon the data, axis limits,
 *	X and Y titles, and title height, determine the cavity left which is
 *	the plotting surface.  The first step get the data and axis limits for
 *	calculating the space needed for the top, bottom, left, and right
 *	margins.
 *
 * 	1) The LEFT margin is the area from the left border to the Y axis 
 *	   (not including ticks). It composes the border width, the width an 
 *	   optional Y axis label and its padding, and the tick numeric labels. 
 *	   The Y axis label is rotated 90 degrees so that the width is the 
 *	   font height.
 *
 * 	2) The RIGHT margin is the area from the end of the graph
 *	   to the right window border. It composes the border width,
 *	   some padding, the font height (this may be dubious. It
 *	   appears to provide a more even border), the max of the
 *	   legend width and 1/2 max X tick number. This last part is
 *	   so that the last tick label is not clipped.
 *
 *           Window Width
 *      ___________________________________________________________
 *      |          |                               |               |
 *      |          |   TOP  height of title        |               |
 *      |          |                               |               |
 *      |          |           x2 title            |               |
 *      |          |                               |               |
 *      |          |        height of x2-axis      |               |
 *      |__________|_______________________________|_______________|  W
 *      |          | -plotpady                     |               |  i
 *      |__________|_______________________________|_______________|  n
 *      |          | top                   right   |               |  d
 *      |          |                               |               |  o
 *      |   LEFT   |                               |     RIGHT     |  w
 *      |          |                               |               |
 *      | y        |     Free area = 104%          |      y2       |  H
 *      |          |     Plotting surface = 100%   |               |  e
 *      | t        |     Tick length = 2 + 2%      |      t        |  i
 *      | i        |                               |      i        |  g
 *      | t        |                               |      t  legend|  h
 *      | l        |                               |      l   width|  t
 *      | e        |                               |      e        |
 *      |    height|                               |height         |
 *      |       of |                               | of            |
 *      |    y-axis|                               |y2-axis        |
 *      |          |                               |               |
 *      |          |origin 0,0                     |               |
 *      |__________|_left_________________bottom___|_______________|
 *      |          |-plotpady                      |               |
 *      |__________|_______________________________|_______________|
 *      |          | (xoffset, yoffset)            |               |
 *      |          |                               |               |
 *      |          |       height of x-axis        |               |
 *      |          |                               |               |
 *      |          |   BOTTOM   x title            |               |
 *      |__________|_______________________________|_______________|
 *
 * 3) The TOP margin is the area from the top window border to the top
 *    of the graph. It composes the border width, twice the height of
 *    the title font (if one is given) and some padding between the
 *    title.
 *
 * 4) The BOTTOM margin is area from the bottom window border to the
 *    X axis (not including ticks). It composes the border width, the height
 *    an optional X axis label and its padding, the height of the font
 *    of the tick labels.
 *
 * The plotting area is between the margins which includes the X and Y axes
 * including the ticks but not the tick numeric labels. The length of the
 * ticks and its padding is 5% of the entire plotting area.  Hence the entire
 * plotting area is scaled as 105% of the width and height of the area.
 *
 * The axis labels, ticks labels, title, and legend may or may not be
 * displayed which must be taken into account.
 *
 * if reqWidth > 0 : set outer size
 * if reqPlotWidth > 0 : set plot size
 *---------------------------------------------------------------------------
 */
void
Blt_LayoutGraph(Graph *graphPtr)
{
    int left, right, top, bottom;
    int plotWidth, plotHeight;
    int inset, inset2;
    int width, height;
    int pad;

    width = graphPtr->width;
    height = graphPtr->height;

    /* 
     * Step 1:  Compute the amount of space needed to display the axes
     *		associated with each margin.  They can be overridden by 
     *		-leftmargin, -rightmargin, -bottommargin, and -topmargin
     *		graph options, respectively.
     */
    left   = GetMarginGeometry(graphPtr, &graphPtr->leftMargin);
    right  = GetMarginGeometry(graphPtr, &graphPtr->rightMargin);
    top    = GetMarginGeometry(graphPtr, &graphPtr->topMargin);
    bottom = GetMarginGeometry(graphPtr, &graphPtr->bottomMargin);

    pad = graphPtr->bottomMargin.maxTickLabelWidth;
    if (pad < graphPtr->topMargin.maxTickLabelWidth) {
	pad = graphPtr->topMargin.maxTickLabelWidth;
    }
    pad = pad / 2 + 3;
    if (right < pad) {
	right = pad;
    }
    if (left < pad) {
	left = pad;
    }
    pad = graphPtr->leftMargin.maxTickLabelHeight;
    if (pad < graphPtr->rightMargin.maxTickLabelHeight) {
	pad = graphPtr->rightMargin.maxTickLabelHeight;
    }
    pad = pad / 2;
    if (top < pad) {
	top = pad;
    }
    if (bottom < pad) {
	bottom = pad;
    }

    if (graphPtr->leftMargin.reqSize > 0) {
	left = graphPtr->leftMargin.reqSize;
    }
    if (graphPtr->rightMargin.reqSize > 0) {
	right = graphPtr->rightMargin.reqSize;
    }
   if (graphPtr->topMargin.reqSize > 0) {
	top = graphPtr->topMargin.reqSize;
    }
    if (graphPtr->bottomMargin.reqSize > 0) {
	bottom = graphPtr->bottomMargin.reqSize;
    }

    /* 
     * Step 2:  Add the graph title height to the top margin. 
     */
    if (graphPtr->title != NULL) {
	top += graphPtr->titleHeight + 6;
    }
    inset = (graphPtr->inset + graphPtr->plotBW);
    inset2 = 2 * inset;

    /* 
     * Step 3: Estimate the size of the plot area from the remaining
     *	       space.  This may be overridden by the -plotwidth and
     *	       -plotheight graph options.  We use this to compute the
     *	       size of the legend. 
     */
    if (width == 0) {
	width = 400;
    }
    if (height == 0) {
	height = 400;
    }
    plotWidth  = (graphPtr->reqPlotWidth > 0) ? graphPtr->reqPlotWidth :
	width - (inset2 + left + right); /* Plot width. */
    plotHeight = (graphPtr->reqPlotHeight > 0) ? graphPtr->reqPlotHeight : 
	height - (inset2 + top + bottom); /* Plot height. */
    Blt_MapLegend(graphPtr, plotWidth, plotHeight);

    /* 
     * Step 4:  Add the legend to the appropiate margin. 
     */
    if (!Blt_Legend_IsHidden(graphPtr)) {
	switch (Blt_Legend_Site(graphPtr)) {
	case LEGEND_RIGHT:
	    right += Blt_Legend_Width(graphPtr) + 2;
	    break;
	case LEGEND_LEFT:
	    left += Blt_Legend_Width(graphPtr) + 2;
	    break;
	case LEGEND_TOP:
	    top += Blt_Legend_Height(graphPtr) + 2;
	    break;
	case LEGEND_BOTTOM:
	    bottom += Blt_Legend_Height(graphPtr) + 2;
	    break;
	case LEGEND_XY:
	case LEGEND_PLOT:
	case LEGEND_WINDOW:
	    /* Do nothing. */
	    break;
	}
    }
#ifdef notdef
    if (graphPtr->colorbar.site != graphPtr->legend.site) {
	GetColorbarGeometry(plotWidth, plotHeight);
    }
#endif
    /* 
     * Recompute the plotarea or graph size, now accounting for the legend. 
     */
    if (graphPtr->reqPlotWidth == 0) {
	plotWidth = width  - (inset2 + left + right);
	if (plotWidth < 1) {
	    plotWidth = 1;
	}
    }
    if (graphPtr->reqPlotHeight == 0) {
	plotHeight = height - (inset2 + top + bottom);
	if (plotHeight < 1) {
	    plotHeight = 1;
	}
    }
#ifdef notdef
    if (graphPtr->colorbar.site == graphPtr->legend.site) {
	GetColorbarGeometry(plotWidth, plotHeight);
    } 
#endif
    /*
     * Step 5: If necessary, correct for the requested plot area aspect
     *	       ratio.
     */
    if ((graphPtr->reqPlotWidth == 0) && (graphPtr->reqPlotHeight == 0) && 
	(graphPtr->aspect > 0.0f)) {
	float ratio;

	/* 
	 * Shrink one dimension of the plotarea to fit the requested
	 * width/height aspect ratio.
	 */
	ratio = (float)plotWidth / (float)plotHeight;
	if (ratio > graphPtr->aspect) {
	    int sw;

	    /* Shrink the width. */
	    sw = (int)(plotHeight * graphPtr->aspect);
	    if (sw < 1) {
		sw = 1;
	    }
	    /* Add the difference to the right margin. */
	    /* CHECK THIS: w = sw; */
	    right += (plotWidth - sw);
	} else {
	    int sh;

	    /* Shrink the height. */
	    sh = (int)(plotWidth / graphPtr->aspect);
	    if (sh < 1) {
		sh = 1;
	    }
	    /* Add the difference to the top margin. */
	    /* CHECK THIS: h = sh; */
	    top += (plotHeight - sh); 
	}
    }

    /* 
     * Step 6: If there are multiple axes in a margin, the axis titles will be
     *	       displayed in the adjoining margins.  Make sure there's room 
     *	       for the longest axis titles.
     */
    if (top < graphPtr->leftMargin.axesTitleLength) {
	top = graphPtr->leftMargin.axesTitleLength;
    }
    if (right < graphPtr->bottomMargin.axesTitleLength) {
	right = graphPtr->bottomMargin.axesTitleLength;
    }
    if (top < graphPtr->rightMargin.axesTitleLength) {
	top = graphPtr->rightMargin.axesTitleLength;
    }
    if (right < graphPtr->topMargin.axesTitleLength) {
	right = graphPtr->topMargin.axesTitleLength;
    }

    /* 
     * Step 7: Override calculated values with requested margin sizes.
     */
    if (graphPtr->leftMargin.reqSize > 0) {
	left = graphPtr->leftMargin.reqSize;
    }
    if (graphPtr->rightMargin.reqSize > 0) {
	right = graphPtr->rightMargin.reqSize;
    }
    if (graphPtr->topMargin.reqSize > 0) {
	top = graphPtr->topMargin.reqSize;
    }
    if (graphPtr->bottomMargin.reqSize > 0) {
	bottom = graphPtr->bottomMargin.reqSize;
    }
    if (graphPtr->reqPlotWidth > 0) {	
	int w;

	/* 
	 * Width of plotarea is constained.  If there's extra space, add it to
	 * th left and/or right margins.  If there's too little, grow the
	 * graph width to accomodate it.
	 */
	w = plotWidth + inset2 + left + right;
	if (width > w) {		/* Extra space in window. */
	    int extra;

	    extra = (width - w) / 2;
	    if (graphPtr->leftMargin.reqSize == 0) { 
		left += extra;
		if (graphPtr->rightMargin.reqSize == 0) { 
		    right += extra;
		} else {
		    left += extra;
		}
	    } else if (graphPtr->rightMargin.reqSize == 0) {
		right += extra + extra;
	    }
	} else if (width < w) {
	    width = w;
	}
    } 
    if (graphPtr->reqPlotHeight > 0) {	/* Constrain the plotarea height. */
	int h;

	/* 
	 * Height of plotarea is constained.  If there's extra space, 
	 * add it to th top and/or bottom margins.  If there's too little,
	 * grow the graph height to accomodate it.
	 */
	h = plotHeight + inset2 + top + bottom;
	if (height > h) {		/* Extra space in window. */
	    int extra;

	    extra = (height - h) / 2;
	    if (graphPtr->topMargin.reqSize == 0) { 
		top += extra;
		if (graphPtr->bottomMargin.reqSize == 0) { 
		    bottom += extra;
		} else {
		    top += extra;
		}
	    } else if (graphPtr->bottomMargin.reqSize == 0) {
		bottom += extra + extra;
	    }
	} else if (height < h) {
	    height = h;
	}
    }	
    graphPtr->width  = width;
    graphPtr->height = height;
    graphPtr->left   = left + inset;
    graphPtr->top    = top + inset;
    graphPtr->right  = width - right - inset;
    graphPtr->bottom = height - bottom - inset;

    graphPtr->leftMargin.width    = left   + graphPtr->inset;
    graphPtr->rightMargin.width   = right  + graphPtr->inset;
    graphPtr->topMargin.height    = top    + graphPtr->inset;
    graphPtr->bottomMargin.height = bottom + graphPtr->inset;
	    
    graphPtr->vOffset = graphPtr->top + graphPtr->padTop;
    graphPtr->vRange  = plotHeight - PADDING(graphPtr->yPad);
    graphPtr->hOffset = graphPtr->left + graphPtr->padLeft;
    graphPtr->hRange  = plotWidth  - PADDING(graphPtr->xPad);

    if (graphPtr->vRange < 1) {
	graphPtr->vRange = 1;
    }
    if (graphPtr->hRange < 1) {
	graphPtr->hRange = 1;
    }
    graphPtr->hScale = 1.0f / (float)graphPtr->hRange;
    graphPtr->vScale = 1.0f / (float)graphPtr->vRange;

    /*
     * Calculate the placement of the graph title so it is centered within the
     * space provided for it in the top margin
     */
    graphPtr->titleY = 3 + graphPtr->inset;
    graphPtr->titleX = (graphPtr->right + graphPtr->left) / 2;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureAxis --
 *
 *	Configures axis attributes (font, line width, label, etc).
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 * Side Effects:
 *	Axis layout is deferred until the height and width of the window are
 *	known.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureAxis(Axis *axisPtr)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    float angle;

    /* Check the requested axis limits. Can't allow -min to be greater than
     * -max.  Do this regardless of -checklimits option. We want to always 
     * detect when the user has zoomed in beyond the precision of the data.*/
    if (((DEFINED(axisPtr->reqMin)) && (DEFINED(axisPtr->reqMax))) &&
	(axisPtr->reqMin >= axisPtr->reqMax)) {
	char msg[200];

	Blt_FormatString(msg, 200, 
		  "impossible axis limits (-min %g >= -max %g) for \"%s\"",
		  axisPtr->reqMin, axisPtr->reqMax, axisPtr->obj.name);
	Tcl_AppendResult(graphPtr->interp, msg, (char *)NULL);
	return TCL_ERROR;
    }
    axisPtr->scrollMin = axisPtr->reqScrollMin;
    axisPtr->scrollMax = axisPtr->reqScrollMax;
    if (axisPtr->logScale) {
	if (axisPtr->flags & CHECK_LIMITS) {
	    /* Check that the logscale limits are positive.  */
	    if ((DEFINED(axisPtr->reqMin)) && (axisPtr->reqMin <= 0.0)) {
		Tcl_AppendResult(graphPtr->interp,"bad logscale -min limit \"", 
			Blt_Dtoa(graphPtr->interp, axisPtr->reqMin), 
			"\" for axis \"", axisPtr->obj.name, "\"", 
			(char *)NULL);
		return TCL_ERROR;
	    }
	}
	if ((DEFINED(axisPtr->scrollMin)) && (axisPtr->scrollMin <= 0.0)) {
	    axisPtr->scrollMin = Blt_NaN();
	}
	if ((DEFINED(axisPtr->scrollMax)) && (axisPtr->scrollMax <= 0.0)) {
	    axisPtr->scrollMax = Blt_NaN();
	}
    }
    angle = FMOD(axisPtr->tickAngle, 360.0f);
    if (angle < 0.0f) {
	angle += 360.0f;
    }
    if (axisPtr->normalBg != NULL) {
	Blt_Bg_SetChangedProc(axisPtr->normalBg, Blt_UpdateGraph, 
		graphPtr);
    }
    if (axisPtr->activeBg != NULL) {
	Blt_Bg_SetChangedProc(axisPtr->activeBg, Blt_UpdateGraph, 
		graphPtr);
    }
    axisPtr->tickAngle = angle;
    ResetTextStyles(axisPtr);

    axisPtr->titleWidth = axisPtr->titleHeight = 0;
    if (axisPtr->title != NULL) {
	unsigned int w, h;

	Blt_GetTextExtents(axisPtr->titleFont, 0, axisPtr->title, -1, &w, &h);
	axisPtr->titleWidth = (unsigned short int)w;
	axisPtr->titleHeight = (unsigned short int)h;
    }

    /* 
     * Don't bother to check what configuration options have changed.  Almost
     * every option changes the size of the plotting area (except for -color
     * and -titlecolor), requiring the graph and its contents to be completely
     * redrawn.
     *
     * Recompute the scale and offset of the axis in case -min, -max options
     * have changed.
     */
    graphPtr->flags |= REDRAW_WORLD;
    graphPtr->flags |= MAP_WORLD | RESET_AXES | CACHE_DIRTY;
    axisPtr->flags |= DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewAxis --
 *
 *	Create and initialize a structure containing information to display
 *	a graph axis.
 *
 * Results:
 *	The return value is a pointer to an Axis structure.
 *
 *---------------------------------------------------------------------------
 */
static Axis *
NewAxis(Graph *graphPtr, const char *name, int margin)
{
    Axis *axisPtr;
    Blt_HashEntry *hPtr;
    int isNew;

    if (name[0] == '-') {
	Tcl_AppendResult(graphPtr->interp, "name of axis \"", name, 
		"\" can't start with a '-'", (char *)NULL);
	return NULL;
    }
    hPtr = Blt_CreateHashEntry(&graphPtr->axes.nameTable, name, &isNew);
    if (!isNew) {
	axisPtr = Blt_GetHashValue(hPtr);
	if ((axisPtr->flags & DELETE_PENDING) == 0) {
	    Tcl_AppendResult(graphPtr->interp, "axis \"", name,
		"\" already exists in \"", Tk_PathName(graphPtr->tkwin), "\"",
		(char *)NULL);
	    return NULL;
	}
	axisPtr->flags &= ~DELETE_PENDING;
    } else {
	axisPtr = Blt_Calloc(1, sizeof(Axis));
	if (axisPtr == NULL) {
	    Tcl_AppendResult(graphPtr->interp, 
		"can't allocate memory for axis \"", name, "\"", (char *)NULL);
	    return NULL;
	}
	axisPtr->obj.name = Blt_AssertStrdup(name);
	axisPtr->hashPtr = hPtr;
	Blt_GraphSetObjectClass(&axisPtr->obj, CID_NONE);
	axisPtr->obj.graphPtr = graphPtr;
	axisPtr->looseMin = axisPtr->looseMax = TIGHT;
	axisPtr->reqNumMinorTicks = 2;
	axisPtr->reqNumMajorTicks = 4 /*10*/;
	axisPtr->margin = MARGIN_NONE;
	axisPtr->tickLength = 8;
	axisPtr->scrollUnits = 10;
	axisPtr->reqMin = axisPtr->reqMax = Blt_NaN();
	axisPtr->reqScrollMin = axisPtr->reqScrollMax = Blt_NaN();
	axisPtr->flags = (SHOWTICKS|GRIDMINOR|AUTO_MAJOR|
			  AUTO_MINOR | EXTERIOR);
	if (graphPtr->classId == CID_ELEM_BAR) {
	    axisPtr->flags |= GRID;
	}
	if ((graphPtr->classId == CID_ELEM_BAR) && 
	    ((margin == MARGIN_TOP) || (margin == MARGIN_BOTTOM))) {
	    axisPtr->reqStep = 1.0;
	    axisPtr->reqNumMinorTicks = 0;
	} 
	if ((margin == MARGIN_RIGHT) || (margin == MARGIN_TOP)) {
	    axisPtr->flags |= HIDE;
	}
	Blt_Ts_InitStyle(axisPtr->limitsTextStyle);
	axisPtr->tickLabels = Blt_Chain_Create();
	axisPtr->lineWidth = 1;
	Blt_SetHashValue(hPtr, axisPtr);
    }
    return axisPtr;
}

static int
GetAxisFromObj(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr, 
	       Axis **axisPtrPtr)
{
    Blt_HashEntry *hPtr;
    const char *name;

    *axisPtrPtr = NULL;
    name = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&graphPtr->axes.nameTable, name);
    if (hPtr != NULL) {
	Axis *axisPtr;

	axisPtr = Blt_GetHashValue(hPtr);
	if ((axisPtr->flags & DELETE_PENDING) == 0) {
	    *axisPtrPtr = axisPtr;
	    return TCL_OK;
	}
    }
    if (interp != NULL) {
	Tcl_AppendResult(interp, "can't find axis \"", name, "\" in \"", 
		Tk_PathName(graphPtr->tkwin), "\"", (char *)NULL);
    }
    return TCL_ERROR;
}

static int
GetAxisByClass(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
	       ClassId classId, Axis **axisPtrPtr)
{
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objPtr, &axisPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (classId != CID_NONE) {
	if ((axisPtr->refCount == 0) || (axisPtr->obj.classId == CID_NONE)) {
	    /* Set the axis type on the first use of it. */
	    Blt_GraphSetObjectClass(&axisPtr->obj, classId);
	} else if (axisPtr->obj.classId != classId) {
	    if (interp != NULL) {
  	        Tcl_AppendResult(interp, "axis \"", Tcl_GetString(objPtr),
		    "\" is already in use on an opposite ", 
			axisPtr->obj.className, "-axis", 
			(char *)NULL);
	    }
	    return TCL_ERROR;
	}
	axisPtr->refCount++;
    }
    *axisPtrPtr = axisPtr;
    return TCL_OK;
}

void
Blt_DestroyAxes(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    int margin;
    
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Axis *axisPtr;
	
	axisPtr = Blt_GetHashValue(hPtr);
	axisPtr->hashPtr = NULL;
	DestroyAxis(axisPtr);
    }
    Blt_DeleteHashTable(&graphPtr->axes.nameTable);
    for (margin = 0; margin < 4; margin++) {
	Blt_Chain_Destroy(graphPtr->axisChain[margin]);
    }
    Blt_DeleteHashTable(&graphPtr->axes.bindTagTable);
    Blt_Chain_Destroy(graphPtr->axes.displayList);
}

void
Blt_ConfigureAxes(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Axis *axisPtr;
	
	axisPtr = Blt_GetHashValue(hPtr);
	ConfigureAxis(axisPtr);
    }
}

int
Blt_DefaultAxes(Graph *graphPtr)
{
    int i, margin;
    int flags;

    flags = Blt_GraphType(graphPtr);
    for (margin = 0; margin < 4; margin++) {
	Blt_Chain chain;
	Axis *axisPtr;

	chain = Blt_Chain_Create();
	graphPtr->axisChain[margin] = chain;

	/* Create a default axis for each chain. */
	axisPtr = NewAxis(graphPtr, axisNames[margin].name, margin);
	if (axisPtr == NULL) {
	    return TCL_ERROR;
	}
	axisPtr->refCount = 1;	/* Default axes are assumed in use. */
	axisPtr->margin = margin;
	axisPtr->flags |= USE;
	Blt_GraphSetObjectClass(&axisPtr->obj, axisNames[margin].classId);
 	if (Blt_ConfigureComponentFromObj(graphPtr->interp, graphPtr->tkwin,
		axisPtr->obj.name, "Axis", configSpecs, 0, (Tcl_Obj **)NULL,
		(char *)axisPtr, flags) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (ConfigureAxis(axisPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	axisPtr->link = Blt_Chain_Append(chain, axisPtr);
	axisPtr->chain = chain;
    }
    /* The extra axes are not attached to a specific margin. */
    for (i = 4; i < numAxisNames; i++) {
	Blt_Chain chain;
	Axis *axisPtr;

	axisPtr = NewAxis(graphPtr, axisNames[i].name, MARGIN_NONE);
	if (axisPtr == NULL) {
	    return TCL_ERROR;
	}
	axisPtr->refCount = 1;		
	axisPtr->margin = MARGIN_NONE;
	axisPtr->flags |= USE;
	Blt_GraphSetObjectClass(&axisPtr->obj, axisNames[i].classId);
 	if (Blt_ConfigureComponentFromObj(graphPtr->interp, graphPtr->tkwin,
		axisPtr->obj.name, "Axis", configSpecs, 0, (Tcl_Obj **)NULL,
		(char *)axisPtr, flags) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (ConfigureAxis(axisPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 * 	Activates the axis, drawing the axis with its -activeforeground,
 *	-activebackgound, -activerelief attributes.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new axis attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(Tcl_Interp *interp, Axis *axisPtr, int objc, Tcl_Obj *const *objv)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    const char *string;

    string = Tcl_GetString(objv[2]);
    if (string[0] == 'a') {
	axisPtr->flags |= ACTIVE;
    } else {
	axisPtr->flags &= ~ACTIVE;
    }
    if ((axisPtr->flags & (USE|HIDE)) == USE) {
	graphPtr->flags |= DRAW_MARGINS | CACHE_DIRTY;
	Blt_EventuallyRedrawGraph(graphPtr);
    }
    return TCL_OK;
}

/*-------------------------------------------------------------------------------
 *
 * BindOp --
 *
 *    .g axis bind axisName sequence command
 *
 *---------------------------------------------------------------------------
 */
static int
BindOp(Tcl_Interp *interp, Axis *axisPtr, int objc, Tcl_Obj *const *objv)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;

    return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable,
          Blt_MakeAxisTag(graphPtr, axisPtr->obj.name), objc, objv);
}
          
/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *	Queries axis attributes (font, line width, label, etc).
 *
 * Results:
 *	Return value is a standard TCL result.  If querying configuration
 *	values, interp->result will contain the results.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CgetOp(Tcl_Interp *interp, Axis *axisPtr, int objc, Tcl_Obj *const *objv)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;

    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
	(char *)axisPtr, objv[0], Blt_GraphType(graphPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *	Queries or resets axis attributes (font, line width, label, etc).
 *
 * Results:
 *	Return value is a standard TCL result.  If querying configuration
 *	values, interp->result will contain the results.
 *
 * Side Effects:
 *	Axis resources are possibly allocated (GC, font). Axis layout is
 *	deferred until the height and width of the window are known.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(Tcl_Interp *interp, Axis *axisPtr, int objc, Tcl_Obj *const *objv)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    int flags;

    flags = BLT_CONFIG_OBJV_ONLY | Blt_GraphType(graphPtr);
    if (objc == 0) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
	    (char *)axisPtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 1) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
	    (char *)axisPtr, objv[0], flags);
    }
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
	objc, objv, (char *)axisPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    if (ConfigureAxis(axisPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (axisPtr->flags & USE) {
	if (Blt_ConfigModified(configSpecs, "-autorange", "-bd", "-borderwidth",
		"-command", "-descending", "-hide", "-justify", "-labeloffset", 
		"-limitsfont", "-limitsformat", "-linewidth", "-logscale", 
		"-loose", "-majorticks", "-max", "-min", "-minorticks", 
		"-relief", "-rotate", "-scrollmax", "-scrollmin", "-shiftby", 
		"-showticks", "-stepsize", "-tickdivider", "-subdivisions", 
		"-tickfont", "-ticklength", "-title", "-titlealternate", 
		"-titlefont", "titleFont", (char *)NULL)) {
	    graphPtr->flags |= CACHE_DIRTY;
	}
	Blt_EventuallyRedrawGraph(graphPtr);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * LimitsOp --
 *
 *	This procedure returns a string representing the axis limits
 *	of the graph.  The format of the string is { left top right bottom}.
 *
 * Results:
 *	Always returns TCL_OK.  The interp->result field is
 *	a list of the graph axis limits.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
LimitsOp(Tcl_Interp *interp, Axis *axisPtr, int objc, Tcl_Obj *const *objv)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    Tcl_Obj *listObjPtr;
    double min, max;

    if (graphPtr->flags & RESET_AXES) {
	Blt_ResetAxes(graphPtr);
    }
    if (axisPtr->logScale) {
	min = EXP10(axisPtr->axisRange.min);
	max = EXP10(axisPtr->axisRange.max);
    } else {
	min = axisPtr->axisRange.min;
	max = axisPtr->axisRange.max;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(min));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(max));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InvTransformOp --
 *
 *	Maps the given window coordinate into an axis-value.
 *
 * Results:
 *	Returns a standard TCL result.  interp->result contains
 *	the axis value. If an error occurred, TCL_ERROR is returned
 *	and interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InvTransformOp(Tcl_Interp *interp, Axis *axisPtr, int objc, 
	       Tcl_Obj *const *objv)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    double y;				/* Real graph coordinate */
    int sy;				/* Integer window coordinate*/

    if (graphPtr->flags & RESET_AXES) {
	Blt_ResetAxes(graphPtr);
    }
    if (Tcl_GetIntFromObj(interp, objv[0], &sy) != TCL_OK) {
	return TCL_ERROR;
    }
    /*
     * Is the axis vertical or horizontal?
     *
     * Check the site where the axis was positioned.  If the axis is
     * virtual, all we have to go on is how it was mapped to an
     * element (using either -mapx or -mapy options).  
     */
    if (axisPtr->flags & HORIZONTAL) {
	y = Blt_InvHMap(axisPtr, (double)sy);
    } else {
	y = Blt_InvVMap(axisPtr, (double)sy);
    }
    Tcl_SetDoubleObj(Tcl_GetObjResult(interp), y);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * MarginOp --
 *
 *	This procedure returns a string representing the margin the axis
 *	resides.  The format of the string is { left top right bottom}.
 *
 * Results:
 *	Always returns TCL_OK.  The interp->result field is
 *	a list of the graph axis limits.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MarginOp(Tcl_Interp *interp, Axis *axisPtr, int objc, Tcl_Obj *const *objv)
{
    const char *marginName;

    marginName = "";
    if (axisPtr->flags & USE) {
	marginName = axisNames[axisPtr->margin].name;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), marginName, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TransformOp --
 *
 *	Maps the given axis-value to a window coordinate.
 *
 * Results:
 *	Returns a standard TCL result.  interp->result contains
 *	the window coordinate. If an error occurred, TCL_ERROR
 *	is returned and interp->result will contain an error
 *	message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TransformOp(Tcl_Interp *interp, Axis *axisPtr, int objc, Tcl_Obj *const *objv)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    double x;

    if (graphPtr->flags & RESET_AXES) {
	Blt_ResetAxes(graphPtr);
    }
    if (Blt_ExprDoubleFromObj(interp, objv[0], &x) != TCL_OK) {
	return TCL_ERROR;
    }
    if (axisPtr->flags & HORIZONTAL) {
	x = Blt_HMap(axisPtr, x);
    } else {
	x = Blt_VMap(axisPtr, x);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), (int)x);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 *	This procedure returns a string representing the margin the axis
 *	resides.  The format of the string is "x", "y", or "".
 *
 * Results:
 *	Always returns TCL_OK.  The interp->result field is the type of 
 *	axis.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeOp(Tcl_Interp *interp, Axis *axisPtr, int objc, Tcl_Obj *const *objv)
{
    const char *typeName;

    typeName = "";
    if (axisPtr->flags & USE) {
	switch (axisPtr->obj.classId) {
	case CID_AXIS_X:
	    typeName = "x";	    break;
	case CID_AXIS_Y:
	    typeName = "y";	    break;
	case CID_AXIS_Z:
	    typeName = "z";	    break;
	default:
	    break;
	}
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), typeName, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * UseOp --
 *
 *	Sets the default axis for a margin.
 *
 * Results:
 *	A standard TCL result.  If the named axis doesn't exist
 *	an error message is put in interp->result.
 *
 * .g xaxis use "abc def gah"
 * .g xaxis use [lappend abc [.g axis use]]
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
UseOp(Tcl_Interp *interp, Axis *axisPtr, int objc, Tcl_Obj *const *objv)
{
    Graph *graphPtr = (Graph *)axisPtr;
    Blt_Chain chain;
    Blt_ChainLink link;
    Tcl_Obj **axisObjv;
    ClassId classId;
    int axisObjc;
    int i;

    chain = graphPtr->margins[lastMargin].axes;
    if (objc == 0) {
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	     link = Blt_Chain_NextLink(link)) {
	    Axis *axisPtr;

	    axisPtr = Blt_Chain_GetValue(link);
	    Tcl_ListObjAppendElement(interp, listObjPtr,
		Tcl_NewStringObj(axisPtr->obj.name, -1));
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    if ((lastMargin == MARGIN_BOTTOM) || (lastMargin == MARGIN_TOP)) {
	classId = (graphPtr->inverted) ? CID_AXIS_Y : CID_AXIS_X;
    } else {
	classId = (graphPtr->inverted) ? CID_AXIS_X : CID_AXIS_Y;
    }
    if (Tcl_ListObjGetElements(interp, objv[0], &axisObjc, &axisObjv) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    for (link = Blt_Chain_FirstLink(chain); link!= NULL; 
	 link = Blt_Chain_NextLink(link)) {
	Axis *axisPtr;

	axisPtr = Blt_Chain_GetValue(link);
	axisPtr->link = NULL;
	axisPtr->flags &= ~USE;
	/* Clear the axis type if it's not currently used.*/
	if (axisPtr->refCount == 0) {
	    Blt_GraphSetObjectClass(&axisPtr->obj, CID_NONE);
	}
    }
    Blt_Chain_Reset(chain);
    for (i = 0; i < axisObjc; i++) {
	Axis *axisPtr;

	if (GetAxisFromObj(interp, graphPtr, axisObjv[i], &axisPtr) != TCL_OK){
	    return TCL_ERROR;
	}
	if (axisPtr->obj.classId == CID_NONE) {
	    Blt_GraphSetObjectClass(&axisPtr->obj, classId);
	} else if (axisPtr->obj.classId != classId) {
	    Tcl_AppendResult(interp, "wrong type axis \"", 
		axisPtr->obj.name, "\": can't use ", 
		axisPtr->obj.className, " type axis.", (char *)NULL); 
	    return TCL_ERROR;
	}
	if (axisPtr->link != NULL) {
	    /* Move the axis from the old margin's "use" list to the new. */
	    Blt_Chain_UnlinkLink(axisPtr->chain, axisPtr->link);
	    Blt_Chain_AppendLink(chain, axisPtr->link);
	} else {
	    axisPtr->link = Blt_Chain_Append(chain, axisPtr);
	}
	axisPtr->chain = chain;
	axisPtr->flags |= USE;
	axisPtr->margin = lastMargin;
    }
    graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | RESET_AXES);
    /* When any axis changes, we need to layout the entire graph.  */
    graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

static int
ViewOp(Tcl_Interp *interp, Axis *axisPtr, int objc, Tcl_Obj *const *objv)
{
    Graph *graphPtr;
    double axisOffset, axisScale;
    double fract;
    double viewMin, viewMax, worldMin, worldMax;
    double viewWidth, worldWidth;

    graphPtr = axisPtr->obj.graphPtr;
    worldMin = axisPtr->valueRange.min;
    worldMax = axisPtr->valueRange.max;
    /* Override data dimensions with user-selected limits. */
    if (DEFINED(axisPtr->scrollMin)) {
	worldMin = axisPtr->scrollMin;
    }
    if (DEFINED(axisPtr->scrollMax)) {
	worldMax = axisPtr->scrollMax;
    }
    viewMin = axisPtr->min;
    viewMax = axisPtr->max;
    /* Bound the view within scroll region. */ 
    if (viewMin < worldMin) {
	viewMin = worldMin;
    } 
    if (viewMax > worldMax) {
	viewMax = worldMax;
    }
    if (axisPtr->logScale) {
	worldMin = log10(worldMin);
	worldMax = log10(worldMax);
	viewMin  = log10(viewMin);
	viewMax  = log10(viewMax);
    }
    worldWidth = worldMax - worldMin;
    viewWidth  = viewMax - viewMin;

    /* Unlike horizontal axes, vertical axis values run opposite of the
     * scrollbar first/last values.  So instead of pushing the axis minimum
     * around, we move the maximum instead. */
    if (AxisIsHorizontal(axisPtr) != axisPtr->descending) {
	axisOffset  = viewMin - worldMin;
	axisScale = graphPtr->hScale;
    } else {
	axisOffset  = worldMax - viewMax;
	axisScale = graphPtr->vScale;
    }
    if (objc == 4) {
	Tcl_Obj *listObjPtr;
	double first, last;

	first = Clamp(axisOffset / worldWidth);
	last = Clamp((axisOffset + viewWidth) / worldWidth);
	listObjPtr = Tcl_NewListObj(0, NULL);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(first));
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(last));
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    fract = axisOffset / worldWidth;
    if (GetAxisScrollInfo(interp, objc, objv, &fract, 
	viewWidth / worldWidth, axisPtr->scrollUnits, axisScale) != TCL_OK) {
	return TCL_ERROR;
    }
    if (AxisIsHorizontal(axisPtr) != axisPtr->descending) {
	axisPtr->reqMin = (fract * worldWidth) + worldMin;
	axisPtr->reqMax = axisPtr->reqMin + viewWidth;
    } else {
	axisPtr->reqMax = worldMax - (fract * worldWidth);
	axisPtr->reqMin = axisPtr->reqMax - viewWidth;
    }
    if (axisPtr->logScale) {
	axisPtr->reqMin = EXP10(axisPtr->reqMin);
	axisPtr->reqMax = EXP10(axisPtr->reqMax);
    }
    graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | RESET_AXES);
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisCreateOp --
 *
 *	Creates a new axis.
 *
 * Results:
 *	Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AxisCreateOp(Tcl_Interp *interp, Graph *graphPtr, int objc, 
	     Tcl_Obj *const *objv)
{
    Axis *axisPtr;
    int flags;

    axisPtr = NewAxis(graphPtr, Tcl_GetString(objv[3]), MARGIN_NONE);
    if (axisPtr == NULL) {
	return TCL_ERROR;
    }
    flags = Blt_GraphType(graphPtr);
    if ((Blt_ConfigureComponentFromObj(interp, graphPtr->tkwin, 
	axisPtr->obj.name, "Axis", configSpecs, objc - 4, objv + 4, 
	(char *)axisPtr, flags) != TCL_OK) || 
	(ConfigureAxis(axisPtr) != TCL_OK)) {
	DestroyAxis(axisPtr);
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), axisPtr->obj.name, -1);
    return TCL_OK;
}
/*
 *---------------------------------------------------------------------------
 *
 * AxisActivateOp --
 *
 * 	Activates the axis, drawing the axis with its -activeforeground,
 *	-activebackgound, -activerelief attributes.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new axis attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisActivateOp(Tcl_Interp *interp, Graph *graphPtr, int objc, 
	       Tcl_Obj *const *objv)
{
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return ActivateOp(interp, axisPtr, objc, objv);
}


/*-------------------------------------------------------------------------------
 *
 * AxisBindOp --
 *
 *    .g axis bind axisName sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AxisBindOp(Tcl_Interp *interp, Graph *graphPtr, int objc, 
	      Tcl_Obj *const *objv)
{
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch cursor;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.bindTagTable, &cursor);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    const char *tagName;
	    Tcl_Obj *objPtr;

	    tagName = Blt_GetHashKey(&graphPtr->axes.bindTagTable, hPtr);
	    objPtr = Tcl_NewStringObj(tagName, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable, 
	Blt_MakeAxisTag(graphPtr, Tcl_GetString(objv[3])), objc - 4, objv + 4);
}


/*
 *---------------------------------------------------------------------------
 *
 * AxisCgetOp --
 *
 *	Queries axis attributes (font, line width, label, etc).
 *
 * Results:
 *	Return value is a standard TCL result.  If querying configuration
 *	values, interp->result will contain the results.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
AxisCgetOp(Tcl_Interp *interp, Graph *graphPtr, int objc, Tcl_Obj *const *objv)
{
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return CgetOp(interp, axisPtr, objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisConfigureOp --
 *
 *	Queries or resets axis attributes (font, line width, label, etc).
 *
 * Results:
 *	Return value is a standard TCL result.  If querying configuration
 *	values, interp->result will contain the results.
 *
 * Side Effects:
 *	Axis resources are possibly allocated (GC, font). Axis layout is
 *	deferred until the height and width of the window are known.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisConfigureOp(Tcl_Interp *interp, Graph *graphPtr, int objc, 
		Tcl_Obj *const *objv)
{
    Tcl_Obj *const *options;
    int i;
    int numNames, numOpts;

    /* Figure out where the option value pairs begin */
    objc -= 3;
    objv += 3;
    for (i = 0; i < objc; i++) {
	Axis *axisPtr;
	const char *string;

	string = Tcl_GetString(objv[i]);
	if (string[0] == '-') {
	    break;
	}
	if (GetAxisFromObj(interp, graphPtr, objv[i], &axisPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    numNames = i;				/* Number of pen names specified */
    numOpts = objc - i;			/* Number of options specified */
    options = objv + i;			/* Start of options in objv  */

    for (i = 0; i < numNames; i++) {
	Axis *axisPtr;

	if (GetAxisFromObj(interp, graphPtr, objv[i], &axisPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (ConfigureOp(interp, axisPtr, numOpts, options) != TCL_OK) {
	    break;
	}
    }
    if (i < numNames) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisDeleteOp --
 *
 *	Deletes one or more axes.  The actual removal may be deferred until the
 *	axis is no longer used by any element. The axis can't be referenced by
 *	its name any longer and it may be recreated.
 *
 * Results:
 *	Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AxisDeleteOp(Tcl_Interp *interp, Graph *graphPtr, int objc, 
	     Tcl_Obj *const *objv)
{
    int i;

    for (i = 3; i < objc; i++) {
	Axis *axisPtr;

	if (GetAxisFromObj(interp, graphPtr, objv[i], &axisPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	axisPtr->flags |= DELETE_PENDING;
	if (axisPtr->refCount == 0) {
	    Tcl_EventuallyFree(axisPtr, FreeAxis);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisFocusOp --
 *
 * 	Activates the axis, drawing the axis with its -activeforeground,
 *	-activebackgound, -activerelief attributes.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new axis attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisFocusOp(Tcl_Interp *interp, Graph *graphPtr, int objc, Tcl_Obj *const *objv)
{
    if (objc > 3) {
	Axis *axisPtr;
	const char *string;

	axisPtr = NULL;
	string = Tcl_GetString(objv[3]);
	if ((string[0] != '\0') && 
	    (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)) {
	    return TCL_ERROR;
	}
	graphPtr->focusPtr = NULL;
	if ((axisPtr != NULL) && 
	    ((axisPtr->flags & (USE|HIDE)) == USE)) {
	    graphPtr->focusPtr = axisPtr;
	}
	Blt_SetFocusItem(graphPtr->bindTable, graphPtr->focusPtr, NULL);
    }
    /* Return the name of the axis that has focus. */
    if (graphPtr->focusPtr != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
		graphPtr->focusPtr->obj.name, -1);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * AxisGetOp --
 *
 *    Returns the name of the picked axis (using the axis bind operation).
 *    Right now, the only name accepted is "current".
 *
 * Results:
 *    A standard TCL result.  The interpreter result will contain the name of
 *    the axis.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AxisGetOp(Tcl_Interp *interp, Graph *graphPtr, int objc, Tcl_Obj *const *objv)
{
    GraphObj *objPtr;

    objPtr = Blt_GetCurrentItem(graphPtr->bindTable);
    /* Report only on axes. */
    if ((objPtr != NULL) && (!objPtr->deleted) &&
	((objPtr->classId == CID_AXIS_X) || (objPtr->classId == CID_AXIS_Y) || 
	 (objPtr->classId == CID_NONE))) {
	char c;
	const char *string;

	string = Tcl_GetString(objv[3]);
	c = string[0];
	if ((c == 'c') && (strcmp(string, "current") == 0)) {
	    Tcl_SetStringObj(Tcl_GetObjResult(interp), objPtr->name,-1);
	} else if ((c == 'd') && (strcmp(string, "detail") == 0)) {
	    Axis *axisPtr;

	    axisPtr = (Axis *)objPtr;
	    Tcl_SetStringObj(Tcl_GetObjResult(interp), axisPtr->detail, -1);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisInvTransformOp --
 *
 *	Maps the given window coordinate into an axis-value.
 *
 * Results:
 *	Returns a standard TCL result.  interp->result contains the axis
 *	value. If an error occurred, TCL_ERROR is returned and interp->result
 *	will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisInvTransformOp(Tcl_Interp *interp, Graph *graphPtr, int objc, 
		   Tcl_Obj *const *objv)
{
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return InvTransformOp(interp, axisPtr, objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisLimitsOp --
 *
 *	This procedure returns a string representing the axis limits of the
 *	graph.  The format of the string is { left top right bottom}.
 *
 * Results:
 *	Always returns TCL_OK.  The interp->result field is
 *	a list of the graph axis limits.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisLimitsOp(Tcl_Interp *interp, Graph *graphPtr, int objc, 
	     Tcl_Obj *const *objv)
{
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return LimitsOp(interp, axisPtr, objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisMarginOp --
 *
 *	This procedure returns a string representing the axis limits of the
 *	graph.  The format of the string is "left top right bottom".
 *
 * Results:
 *	Always returns TCL_OK.  The interp->result field is a list of the
 *	graph axis limits.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisMarginOp(Tcl_Interp *interp, Graph *graphPtr, int objc, 
	     Tcl_Obj *const *objv)
{
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return MarginOp(interp, axisPtr, objc - 4, objv + 4);
}


/*
 *---------------------------------------------------------------------------
 *
 * AxisNamesOp --
 *
 *	Return a list of the names of all the axes.
 *
 * Results:
 *	Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
AxisNamesOp(Tcl_Interp *interp, Graph *graphPtr, int objc, Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch cursor;

	for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    Axis *axisPtr;

	    axisPtr = Blt_GetHashValue(hPtr);
	    if (axisPtr->flags & DELETE_PENDING) {
		continue;
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		     Tcl_NewStringObj(axisPtr->obj.name, -1));
	}
    } else {
	Blt_HashEntry *hPtr;
	Blt_HashSearch cursor;

	for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    Axis *axisPtr;
	    int i;

	    axisPtr = Blt_GetHashValue(hPtr);
	    for (i = 3; i < objc; i++) {
		const char *pattern;

		pattern = Tcl_GetString(objv[i]);
		if (Tcl_StringMatch(axisPtr->obj.name, pattern)) {
		    Tcl_Obj *objPtr;

		    objPtr = Tcl_NewStringObj(axisPtr->obj.name, -1);
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
 * AxisTransformOp --
 *
 *	Maps the given axis-value to a window coordinate.
 *
 * Results:
 *	Returns the window coordinate via interp->result.  If an error occurred,
 *	TCL_ERROR is returned and interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisTransformOp(Tcl_Interp *interp, Graph *graphPtr, int objc, 
		Tcl_Obj *const *objv)
{
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TransformOp(interp, axisPtr, objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisMarginOp --
 *
 *	This procedure returns a string representing the axis limits of the
 *	graph.  The format of the string is { left top right bottom}.
 *
 * Results:
 *	Always returns TCL_OK.  The interp->result field is
 *	a list of the graph axis limits.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisTypeOp(Tcl_Interp *interp, Graph *graphPtr, int objc, 
	     Tcl_Obj *const *objv)
{
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TypeOp(interp, axisPtr, objc - 4, objv + 4);
}


static int
AxisViewOp(Tcl_Interp *interp, Graph *graphPtr, int objc, Tcl_Obj *const *objv)
{
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return ViewOp(interp, axisPtr, objc - 4, objv + 4);
}

static Blt_OpSpec virtAxisOps[] = {
    {"activate",     1, AxisActivateOp,     4, 4, "axisName"},
    {"bind",         1, AxisBindOp,         3, 6, "axisName sequence command"},
    {"cget",         2, AxisCgetOp,         5, 5, "axisName option"},
    {"configure",    2, AxisConfigureOp,    4, 0, "axisName ?axisName?... "
	"?option value?..."},
    {"create",       2, AxisCreateOp,       4, 0, "axisName ?option value?..."},
    {"deactivate",   3, AxisActivateOp,     4, 4, "axisName"},
    {"delete",       3, AxisDeleteOp,       3, 0, "?axisName?..."},
    {"focus",        1, AxisFocusOp,        3, 4, "?axisName?"},
    {"get",          1, AxisGetOp,          4, 4, "name"},
    {"invtransform", 1, AxisInvTransformOp, 5, 5, "axisName value"},
    {"limits",       1, AxisLimitsOp,       4, 4, "axisName"},
    {"margin",       1, AxisMarginOp,       4, 4, "axisName"},
    {"names",        1, AxisNamesOp,        3, 0, "?pattern?..."},
    {"transform",    2, AxisTransformOp,    5, 5, "axisName value"},
    {"type",         2, AxisTypeOp,       4, 4, "axisName"},
    {"view",         1, AxisViewOp,         4, 7, "axisName ?moveto fract? "
	"?scroll number what?"},
};
static int numVirtAxisOps = sizeof(virtAxisOps) / sizeof(Blt_OpSpec);

int
Blt_VirtualAxisOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    GraphVirtualAxisProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numVirtAxisOps, virtAxisOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (interp, graphPtr, objc, objv);
    return result;
}

static Blt_OpSpec axisOps[] = {
    {"activate",     1, ActivateOp,     3, 3, "",},
    {"bind",         1, BindOp,         2, 5, "sequence command",},
    {"cget",         2, CgetOp,         4, 4, "option",},
    {"configure",    2, ConfigureOp,    3, 0, "?option value?...",},
    {"deactivate",   1, ActivateOp,     3, 3, "",},
    {"invtransform", 1, InvTransformOp, 4, 4, "value",},
    {"limits",       1, LimitsOp,       3, 3, "",},
    {"transform",    1, TransformOp,    4, 4, "value",},
    {"use",          1, UseOp,          3, 4, "?axisName?",},
    {"view",         1, ViewOp,         3, 6, "?moveto fract? ",},
};

static int numAxisOps = sizeof(axisOps) / sizeof(Blt_OpSpec);

int
Blt_ColorbarOp(Tcl_Interp *interp, Graph *graphPtr, int margin, int objc,
	   Tcl_Obj *const *objv)
{
    int result;
    GraphAxisProc *proc;

    proc = Blt_GetOpFromObj(interp, numAxisOps, axisOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    if (proc == UseOp) {
	lastMargin = margin;		/* Set global variable to the margin
					 * in the argument list. Needed only
					 * for UseOp. */
	result = (*proc)(interp, (Axis *)graphPtr, objc - 3, objv + 3);
    } else {
	Axis *axisPtr;

	axisPtr = Blt_GetFirstAxis(graphPtr->margins[margin].axes);
	if (axisPtr == NULL) {
	    return TCL_OK;
	}
	result = (*proc)(interp, axisPtr, objc - 3, objv + 3);
    }
    return result;
}


void
Blt_Colorbar_Map(Graph *graphPtr)
{
    Colorbar *cbarPtr;
    Margin *marginPtr;
    Blt_Chain chain;
    Blt_ChainLink link;
    int count, offset;
    Axis *axisPtr;
    
    cbarPtr = &graphPtr->colorbar;
    marginPtr = cbarPtr->marginPtr;
    axisPtr = cbarPtr->axisPtr;

    count = offset = 0;
	
    if (axisPtr->flags & (HIDE|DELETE_PENDING)) {
	return;
    }
    if (axisPtr->reqNumMajorTicks <= 0) {
	axisPtr->reqNumMajorTicks = 4;
    }
    MapBar(cbarPtr);
    MapAxis(axisPtr, offset, cbarPtr->margin);
}

void
Blt_AxesToPostScript(Graph *graphPtr, Blt_Ps ps) 
{
    int margin;

    for (margin = 0; margin < 4; margin++) {
	Blt_ChainLink link;
	Margin *marginPtr;
	
	marginPtr = graphPtr->margins + margin;
	for (link = Blt_Chain_FirstLink(marginPtr->axes); link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    Axis *axisPtr;

	    axisPtr = Blt_Chain_GetValue(link);
	    if ((axisPtr->flags & (DELETE_PENDING|HIDE|USE)) == USE) {
		AxisToPostScript(ps, axisPtr);
	    }
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawAxisLimits --
 *
 *	Draws the min/max values of the axis in the plotting area.  The text
 *	strings are formatted according to the "sprintf" format descriptors in
 *	the limitsFormats array.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Draws the numeric values of the axis limits into the outer regions of
 *	the plotting area.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DrawAxisLimits(Graph *graphPtr, Drawable drawable)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    char minString[200], maxString[200];
    int vMin, hMin, vMax, hMax;

#define SPACING 8
    vMin = vMax = graphPtr->left + graphPtr->padLeft + 2;
    hMin = hMax = graphPtr->bottom - graphPtr->padBottom - 2;	/* Offsets */

    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
	hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Axis *axisPtr;
	Dim2d textDim;
	const char *minFmt, *maxFmt;
	char *minPtr, *maxPtr;

	axisPtr = Blt_GetHashValue(hPtr);
	if (axisPtr->flags & DELETE_PENDING) {
	    continue;
	} 
	if (axisPtr->numFormats == 0) {
	    continue;
	}
	minPtr = maxPtr = NULL;
	minFmt = maxFmt = axisPtr->limitsFormats[0];
	if (axisPtr->numFormats > 1) {
	    maxFmt = axisPtr->limitsFormats[1];
	}
	if (minFmt[0] != '\0') {
	    minPtr = minString;
	    Blt_FormatString(minString, 200, minFmt, axisPtr->axisRange.min);
	}
	if (maxFmt[0] != '\0') {
	    maxPtr = maxString;
	    Blt_FormatString(maxString, 200, maxFmt, axisPtr->axisRange.max);
	}
	if (axisPtr->descending) {
	    char *tmp;

	    tmp = minPtr, minPtr = maxPtr, maxPtr = tmp;
	}
	if (maxPtr != NULL) {
	    if (axisPtr->flags & HORIZONTAL) {
		Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
		Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SE);
		Blt_DrawText2(graphPtr->tkwin, drawable, maxPtr,
		    &axisPtr->limitsTextStyle, graphPtr->right, hMax, &textDim);
		hMax -= (textDim.height + SPACING);
	    } else {
		Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
		Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_NW);
		Blt_DrawText2(graphPtr->tkwin, drawable, maxPtr,
		    &axisPtr->limitsTextStyle, vMax, graphPtr->top, &textDim);
		vMax += (textDim.width + SPACING);
	    }
	}
	if (minPtr != NULL) {
	    Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SW);
	    if (axisPtr->flags & HORIZONTAL) {
		Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
		Blt_DrawText2(graphPtr->tkwin, drawable, minPtr,
		    &axisPtr->limitsTextStyle, graphPtr->left, hMin, &textDim);
		hMin -= (textDim.height + SPACING);
	    } else {
		Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
		Blt_DrawText2(graphPtr->tkwin, drawable, minPtr,
		    &axisPtr->limitsTextStyle, vMin, graphPtr->bottom, &textDim);
		vMin += (textDim.width + SPACING);
	    }
	}
    } /* Loop on axes */
}

void
Blt_AxisLimitsToPostScript(Graph *graphPtr, Blt_Ps ps)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    double vMin, hMin, vMax, hMax;
    char string[200];

#define SPACING 8
    vMin = vMax = graphPtr->left + graphPtr->padLeft + 2;
    hMin = hMax = graphPtr->bottom - graphPtr->padBottom - 2;	/* Offsets */
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Axis *axisPtr;
	const char *minFmt, *maxFmt;
	unsigned int textWidth, textHeight;

	axisPtr = Blt_GetHashValue(hPtr);
	if (axisPtr->flags & DELETE_PENDING) {
	    continue;
	} 
	if (axisPtr->numFormats == 0) {
	    continue;
	}
	minFmt = maxFmt = axisPtr->limitsFormats[0];
	if (axisPtr->numFormats > 1) {
	    maxFmt = axisPtr->limitsFormats[1];
	}
	if (*maxFmt != '\0') {
	    Blt_FormatString(string, 200, maxFmt, axisPtr->axisRange.max);
	    Blt_GetTextExtents(axisPtr->tickFont, 0, string, -1, &textWidth,
		&textHeight);
	    if ((textWidth > 0) && (textHeight > 0)) {
		if (axisPtr->obj.classId == CID_AXIS_X) {
		    Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
		    Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SE);
		    Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle, 
			(double)graphPtr->right, hMax);
		    hMax -= (textWidth + SPACING);
		} else {
		    Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
		    Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_NW);
		    Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle,
			vMax, (double)graphPtr->top);
		    vMax += (textWidth + SPACING);
		}
	    }
	}
	if (*minFmt != '\0') {
	    Blt_FormatString(string, 200, minFmt, axisPtr->axisRange.min);
	    Blt_GetTextExtents(axisPtr->tickFont, 0, string, -1, &textWidth,
		&textHeight);
	    if ((textWidth > 0) && (textHeight > 0)) {
		Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SW);
		if (axisPtr->obj.classId == CID_AXIS_X) {
		    Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
		    Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle, 
			(double)graphPtr->left, hMin);
		    hMin -= (textWidth + SPACING);
		} else {
		    Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
		    Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle, 
			vMin, (double)graphPtr->bottom);
		    vMin += (textWidth + SPACING);
		}
	    }
	}
    }
}

Axis *
Blt_GetFirstAxis(Blt_Chain chain)
{
    Blt_ChainLink link;

    link = Blt_Chain_FirstLink(chain);
    if (link == NULL) {
	return NULL;
    }
    return Blt_Chain_GetValue(link);
}

Axis *
Blt_NearestAxis(Graph *graphPtr, int x, int y)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor); 
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Axis *axisPtr;

	axisPtr = Blt_GetHashValue(hPtr);
	if ((axisPtr->flags & (DELETE_PENDING|HIDE|USE)) != USE) {
	    continue;
	}
	if (axisPtr->flags & SHOWTICKS) {
	    Blt_ChainLink link;

	    for (link = Blt_Chain_FirstLink(axisPtr->tickLabels); link != NULL; 
		 link = Blt_Chain_NextLink(link)) {	
		TickLabel *labelPtr;
		Point2d t;
		double rw, rh;
		Point2d bbox[5];

		labelPtr = Blt_Chain_GetValue(link);
		Blt_GetBoundingBox(labelPtr->width, labelPtr->height, 
			axisPtr->tickAngle, &rw, &rh, bbox);
		t = Blt_AnchorPoint(labelPtr->anchorPos.x, 
			labelPtr->anchorPos.y, rw, rh, axisPtr->tickAnchor);
		t.x = x - t.x - (rw * 0.5);
		t.y = y - t.y - (rh * 0.5);

		bbox[4] = bbox[0];
		if (Blt_PointInPolygon(&t, bbox, 5)) {
		    axisPtr->detail = "label";
		    return axisPtr;
		}
	    }
	}
	if (axisPtr->title != NULL) {	/* and then the title string. */
	    Point2d bbox[5];
	    Point2d t;
	    double rw, rh;
	    unsigned int w, h;

	    Blt_GetTextExtents(axisPtr->titleFont, 0, axisPtr->title,-1,&w,&h);
	    Blt_GetBoundingBox(w, h, axisPtr->titleAngle, &rw, &rh, bbox);
	    t = Blt_AnchorPoint(axisPtr->titlePos.x, axisPtr->titlePos.y, 
		rw, rh, axisPtr->titleAnchor);
	    /* Translate the point so that the 0,0 is the upper left 
	     * corner of the bounding box.  */
	    t.x = x - t.x - (rw * 0.5);
	    t.y = y - t.y - (rh * 0.5);
	    
	    bbox[4] = bbox[0];
	    if (Blt_PointInPolygon(&t, bbox, 5)) {
		axisPtr->detail = "title";
		return axisPtr;
	    }
	}
	if (axisPtr->lineWidth > 0) {	/* Check for the axis region */
	    if ((x <= axisPtr->right) && (x >= axisPtr->left) && 
		(y <= axisPtr->bottom) && (y >= axisPtr->top)) {
		axisPtr->detail = "line";
		return axisPtr;
	    }
	}
    }
    return NULL;
}
 
ClientData
Blt_MakeAxisTag(Graph *graphPtr, const char *tagName)
{
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&graphPtr->axes.bindTagTable, tagName, &isNew);
    return Blt_GetHashKey(&graphPtr->axes.bindTagTable, hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TimeAxis --
 *
 * 	Determine the units of a linear scaled axis.
 *
 *	The axis limits are either the range of the data values mapped
 *	to the axis (autoscaled), or the values specified by the -min
 *	and -max options (manual).
 *
 *	If autoscaled, the smallest and largest major ticks will
 *	encompass the range of data values.  If the -loose option is
 *	selected, the next outer ticks are choosen.  If tight, the
 *	ticks are at or inside of the data limits are used.
 *
 * 	If manually set, the ticks are at or inside the data limits
 * 	are used.  This makes sense for zooming.  You want the
 * 	selected range to represent the next limit, not something a
 * 	bit bigger.
 *
 *	Note: I added an "always" value to the -loose option to force
 *	      the manually selected axes to be loose. It's probably
 *	      not a good idea.
 *
 *          maxY
 *            |    units = magnitude (of least significant digit)
 *            |    high  = largest unit tick < max axis value
 *      high _|    low   = smallest unit tick > min axis value
 *            |
 *            |    range = high - low
 *            |    # ticks = greatest factor of range/units
 *           _|
 *        U   |
 *        n   |
 *        i   |
 *        t  _|
 *            |
 *            |
 *            |
 *       low _|
 *            |
 *            |_minX________________maxX__
 *            |   |       |      |       |
 *     minY  low                        high
 *           minY
 *
 * 	numTicks = Number of ticks
 * 	min = Minimum value of axis
 * 	max = Maximum value of axis
 * 	range    = Range of values (max - min)
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The axis tick information is set.  The actual tick values will
 *	be generated later.
 *
 *---------------------------------------------------------------------------
 */
static void
TimeAxis(Axis *axisPtr, double min, double max)
{
#ifdef notdef
    double step;
    double tickMin, tickMax;
    double axisMin, axisMax;
    unsigned int numTicks;

    range = max - min;

#define SECONDS(x)	(x)
#define MINUTES(x)	((x) * 60)
#define HOURS(x)	((x) * 60 * 60)
#define DAYS(x)		((x) * 60 * 60 * 24)
#define MONTHS(x)	((x) * 60 * 60 * 24 * 30)
#define YEARS(x)	((x) * 60 * 60 * 24 * 365)
    div = numTicks - 1;
    if (range > (MONTHS(6) * div)) {
	unit = TICK_YEAR;
    } else if (range > (MONTHS(3) * div)) {
	unit = TICKS_6MONTH;
	first = timefloor(min, unit);
	last = timeceil(max, unit);
    } else if (range > (MONTHS(2) * div)) {
	unit = TICKS_3MONTH;
	first = timefloor(min, unit);
	last = timeceil(max, unit);
    } else if (range > (MONTHS(1) * div)) {
	unit = TICKS_2MONTH;
	first = timefloor(min, unit);
	last = timeceil(max, unit);
    } else if (range > (DAYS(15) * div)) {
	unit = TICKS_1MONTH;
    } else if (range > (DAYS(10) * div)) {
	unit = TICKS_15DAY;
    } else if (range > (DAYS(1) * div)) {
	unit = TICKS_10DAY;
    } else if (range > (HOURS(12) * div)) {
	unit = TICKS_1DAY;
    } else if (range > (HOURS(6) * div)) {
	unit = TICKS_12HOUR;
    } else if (range > (HOURS(3) * div)) {
	unit = TICKS_6HOUR;
    } else if (range > (HOURS(1) * div)) {
	unit = TICKS_3HOUR;
    } else if (range > (MINUTES(30) * div)) {
	unit = TICKS_HOUR;
    } else if (range > (MINUTES(20) * div)) {
	unit = TICKS_30MIN;
    } else if (range > (MINUTES(15) * div)) {
	unit = TICKS_20MIN;
    } else if (range > (MINUTES(10) * div)) {
	unit = TICKS_15MIN;
    } else if (range > (MINUTES(1) * div)) {
	unit = TICKS_10MIN;
    } else if (range > (SECONDS(30) * div)) {
	unit = TICKS_1MIN;
    } else if (range > (SECONDS(20) * div)) {
	unit = TICKS_30SEC;
    } else if (range > (SECONDS(15) * div)) {
	unit = TICKS_20SEC;
    } else if (range > (SECONDS(10) * div)) {
	unit = TICKS_15SEC;
    } else if (range > (SECONDS(1) * div)) {
	unit = TICKS_10SEC;
    } else {
	unit = TICKS_1SEC;
    }

    } else {
	unit = TICKS_SECS;
    }
    numTicks = 0;
    step = 1.0;
    /* Suppress compiler warning. */
    axisMin = axisMax = tickMin = tickMax = Blt_NaN();
    if (min < max) {
	double range;

	range = max - min;
	/* Calculate the major tick stepping. */
	if (axisPtr->reqStep > 0.0) {
	    /* An interval was designated by the user.  Keep scaling it until
	     * it fits comfortably within the current range of the axis.  */
	    step = axisPtr->reqStep;
	    while ((2 * step) >= range) {
		step *= 0.5;
	    }
	} else {
	    range = NiceNum(range, 0);
	    step = NiceNum(range / axisPtr->reqNumMajorTicks, 1);
	}
	
	/* Find the outer tick values. Add 0.0 to prevent getting -0.0. */
	axisMin = tickMin = floor(min / step) * step + 0.0;
	axisMax = tickMax = ceil(max / step) * step + 0.0;
	
	nTicks = Round((tickMax - tickMin) / step) + 1;
    } 
    axisPtr->majorSweep.step = step;
    axisPtr->majorSweep.initial = tickMin;
    axisPtr->majorSweep.numSteps = numTicks;

    /*
     * The limits of the axis are either the range of the data ("tight") or at
     * the next outer tick interval ("loose").  The looseness or tightness has
     * to do with how the axis fits the range of data values.  This option is
     * overridden when the user sets an axis limit (by either -min or -max
     * option).  The axis limit is always at the selected limit (otherwise we
     * assume that user would have picked a different number).
     */
    if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) &&
	 (DEFINED(axisPtr->reqMin)))) {
	axisMin = min;
    }
    if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
	 (DEFINED(axisPtr->reqMax)))) {
	axisMax = max;
    }
    SetAxisRange(&axisPtr->axisRange, axisMin, axisMax);

    /* Now calculate the minor tick step and number. */

    if ((axisPtr->reqNumMinorTicks > 0) && (axisPtr->flags & AUTO_MAJOR)) {
	nTicks = axisPtr->reqNumMinorTicks - 1;
	step = 1.0 / (nTicks + 1);
    } else {
	nTicks = 0;			/* No minor ticks. */
	step = 0.5;			/* Don't set the minor tick interval to
					 * 0.0. It makes the GenerateTicks
					 * routine * create minor log-scale tick
					 * marks.  */
    }
    axisPtr->minorSweep.initial = axisPtr->minorSweep.step = step;
    axisPtr->minorSweep.numSteps = numTicks;
#endif
}

#ifdef notdef
static Ticks *
TimeGenerateTicks(TickSweep *sweepPtr)
{
    Ticks *ticksPtr;

    ticksPtr = Blt_AssertMalloc(sizeof(Ticks) + 
	(sweepPtr->numSteps * sizeof(double)));
    ticksPtr->numTicks = 0;

    if (sweepPtr->step == 0.0) { 
	/* Hack: A zero step indicates to use log values. */
	int i;
	/* Precomputed log10 values [1..10] */
	static double logTable[] = {
	    0.0, 
	    0.301029995663981, 
	    0.477121254719662, 
	    0.602059991327962, 
	    0.698970004336019, 
	    0.778151250383644, 
	    0.845098040014257,
	    0.903089986991944, 
	    0.954242509439325, 
	    1.0
	};
	for (i = 0; i < sweepPtr->numSteps; i++) {
	    ticksPtr->values[i] = logTable[i];
	}
    } else {
	double value;
	int i;
    
	value = sweepPtr->initial;	/* Start from smallest axis tick */
	for (i = 0; i < sweepPtr->numSteps; i++) {
	    value = UROUND(value, sweepPtr->step);
	    ticksPtr->values[i] = value;
	    value += sweepPtr->step;
	}
    }
    ticksPtr->numTicks = sweepPtr->numSteps;
    return ticksPtr;
}

static double
TimeFloor(double min, int unit)
{
    unsigned long ticks;

    ticks = (long)floor(min);
    localtime_r(&ticks, &tm);
    switch(unit) {
	case TICK_6MONTHS:
	    tm.sec = 0;
	    tm.min = 0;
	    tm.day = 0;
	    tm.
    }
}
static double
TimeCeil(double max, int unit)
{
    
}

#endif


/*
 * bltGrColorbar.c --
 *
 * This module implements contour elements for the BLT graph widget.
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
#include <X11/Xutil.h>
#include "bltHash.h"
#include "bltChain.h"
#include "bltBind.h"
#include "bltPs.h"
#include "bltBg.h"
#include "bltPool.h"
#include "bltOp.h"
#include "bltImage.h"
#include "bltBitmap.h"
#include "bltPainter.h"
#include "bltGraph.h"
#include "bltPicture.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"
#include "bltGrMesh.h"
#include "bltGrElem.h"

#define DEF_ASPECT_RATIO		"0.0"
#define DEF_BAR_BASELINE		"0.0"
#define DEF_BAR_MODE		"normal"
#define DEF_BAR_WIDTH		"0.9"
#define DEF_BACKGROUND		STD_NORMAL_BACKGROUND
#define DEF_BORDERWIDTH		STD_BORDERWIDTH
#define DEF_BUFFER_ELEMENTS	"yes"
#define DEF_BUFFER_GRAPH		"1"
#define DEF_CURSOR		"crosshair"
#define DEF_FONT			"{Sans Serif} 12"
#define DEF_HALO			"2m"
#define DEF_HALO_BAR		"0.1i"
#define DEF_HEIGHT		"4i"
#define DEF_HIGHLIGHT_BACKGROUND	STD_NORMAL_BACKGROUND
#define DEF_HIGHLIGHT_COLOR	RGB_BLACK
#define DEF_HIGHLIGHT_WIDTH	"2"
#define DEF_INVERT_XY		"0"
#define DEF_JUSTIFY		"center"
#define DEF_MARGIN		"0"
#define DEF_MARGIN_VAR		(char *)NULL
#define DEF_PLOT_BACKGROUND	RGB_WHITE
#define DEF_PLOT_BORDERWIDTH	"1"
#define DEF_PLOT_PADX		"0"
#define DEF_PLOT_PADY		"0"
#define DEF_PLOT_RELIEF		"solid"
#define DEF_RELIEF		"flat"
#define DEF_SHOW_VALUES		"no"
#define DEF_STACK_AXES		"no"
#define DEF_TAKE_FOCUS		""
#define DEF_TITLE			(char *)NULL
#define DEF_TITLE_COLOR		STD_NORMAL_FOREGROUND
#define DEF_WIDTH			"5i"
#define DEF_DATA			(char *)NULL
#define DEF_DATA_COMMAND		(char *)NULL
#define DEF_UNMAP_HIDDEN_ELEMENTS	"0"

/*
 * Colorbar --
 *
 * 	Contains information specific to how the legend will be displayed.
 *
 *	Much of the information indicating how to display the colorbar is
 *	derived from the associated contour element's z-axis and palette:
 *
 *	log scale (axis).
 *	tick length (axis).
 *	line width (axis).
 *	colors (palette).
 *	title (axis).
 *	isoline symbol (element)
 *	isoline value (element)
 *	isoline line style (element)
 *	min/max value (axis)
 *	
 */
struct _Colorbar {
    GraphObj obj;			/* Must be first field in axis. */

    unsigned int flags;		

    Blt_HashEntry *hashPtr;

    /* Fields specific to axes. */

    unsigned int flags;
    ClassId classId;			/* Type: colorbar. */

    Element *elemPtr;			/* Contour element whose palette
					 * is to be displayed. */
    const char *elemObjPtr;

#ifdef notdef
    short int left, right, top, bottom;	/* Area occupied by the colorbar. */
    short int width, height;		/* Extents of colorbar. */
    XColor *isoColor;			/* Color of normal isolines. */
    XColor *activeIsoColor;		/* Color of active isolines.  */


    int relief;
    int borderWidth;
    int activeRelief;

    float tickAngle;	
    Blt_Font tickFont;
    Tk_Anchor tickAnchor;
    Tk_Anchor reqTickAnchor;
    XColor *tickColor;
    GC tickGC;				/* Graphics context for axis and tick
					 * labels */
    GC activeTickGC;

    double titleAngle;	
    Blt_Font titleFont;
    Tk_Anchor titleAnchor;
    Tk_Justify titleJustify;
    XColor *titleColor;
    
    Grid major, minor;			/* Axis grid information. */

    double screenScale;
    int screenMin, screenRange;

	/*  */


    short int numColumns, numRows;	        /* Number of columns and rows in
					 * legend */
    short int width, height;		/* Dimensions of the legend */
    short int entryWidth, entryHeight;

    int site;
    short int xReq, yReq;		/* User-requested site of legend, not
					 * the final actual position. Used in
					 * conjunction with the anchor below
					 * to determine location of the
					 * legend. */

    Tk_Anchor anchor;			/* Anchor of legend. Used to interpret
					 * the positioning point of the legend
					 * in the graph*/

    int x, y;				/* Computed origin of legend. */
#endif

    Graph *graphPtr;
    Tcl_Command cmdToken;		/* Token for graph's widget command. */
    int reqColumns, reqRows;

    Blt_Pad ixPad, iyPad;		/* # of pixels interior padding around
					 * legend entries */
    Blt_Pad xPad, yPad;			/* # of pixels padding to exterior of
					 * legend */
    Tk_Window tkwin;			/* If non-NULL, external window to draw
					 * legend. */
    TextStyle style;

    Blt_Bg activeBg;			/* Active legend entry background
					 * color. */
    XColor *activeFgColor;
    int activeRelief;			/* 3-D effect on active entry. */
    int entryBW;			/* Border width around each entry in
					 * legend. */
    Blt_Bg normalBg;			/* 3-D effect of legend. */
    int borderWidth;			/* Width of legend 3-D border */
    int relief;				/* 3-d effect of border around the
					 * legend: TK_RELIEF_RAISED etc. */

    Blt_BindTable bindTable;

    int selRelief;
    int selBW;

    XColor *selInFocusFgColor;		/* Text color of a selected entry. */
    XColor *selOutFocusFgColor;

    Blt_Bg selInFocusBg;
    Blt_Bg selOutFocusBg;

    XColor *focusColor;
    Blt_Dashes focusDashes;		/* Dash on-off value. */
    GC focusGC;				/* Graphics context for the active
					 * label. */

    const char *takeFocus;
    int focus;				/* Position of the focus entry. */

    int cursorX, cursorY;		/* Position of the insertion cursor in
					 * the textbox window. */
    short int cursorWidth;		/* Size of the insertion cursor
					 * symbol. */
    short int cursorHeight;
    Element *focusPtr;			/* Element that currently has the
					 * focus. If NULL, no legend entry has
					 * the focus. */
    Element *selAnchorPtr;		/* Fixed end of selection. Used to
					 * extend the selection while
					 * maintaining the other end of the
					 * selection. */
    Element *selMarkPtr;
    Element *selFirstPtr;		/* First element selected in current
					 * pick. */
    Element *selLastPtr;		/* Last element selected in current
					 * pick. */
    int exportSelection;
    int active;
    int cursorOn;			/* Indicates if the cursor is
					 * displayed. */
    int onTime, offTime;		/* Time in milliseconds to wait before
					 * changing the cursor from off-to-on
					 * and on-to-off. Setting offTime to 0
					 * makes the * cursor steady. */
    Tcl_TimerToken timerToken;		/* Handle for a timer event called
					 * periodically to blink the cursor. */
    const char *selectCmd;		/* TCL script that's invoked whenever
					 * the selection changes. */
    int selMode;			/* Mode of selection: single or
					 * multiple. */
    Blt_HashTable selTable;		/* Table of selected elements. Used to
					 * quickly determine whether an element
					 * is selected. */
    Blt_Chain selected;			/* List of selected elements. */

    const char *title;
    unsigned int titleWidth, titleHeight;
    TextStyle titleStyle;		/* Legend title attributes */
};

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_FLOAT, "-aspect", "aspect", "Aspect", DEF_ASPECT_RATIO, 
	Blt_Offset(Colorbar, aspect), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_BACKGROUND, Blt_Offset(Colorbar, normalBg), 0},
    {BLT_CONFIG_CUSTOM, "-barmode", "barMode", "BarMode", DEF_BAR_MODE, 
	Blt_Offset(Colorbar, mode), BLT_CONFIG_DONT_SET_DEFAULT, 
	&bltBarModeOption},
    {BLT_CONFIG_FLOAT, "-barwidth", "barWidth", "BarWidth", 
	DEF_BAR_WIDTH, Blt_Offset(Colorbar, barWidth), 0},
    {BLT_CONFIG_FLOAT, "-baseline", "baseline", "Baseline",
	DEF_BAR_BASELINE, Blt_Offset(Colorbar, baseline), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL,0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-bm", "bottomMargin", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BORDERWIDTH, Blt_Offset(Colorbar, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-element", "element", "Element", DEF_ELEMENT, 
	Blt_Offset(Colorbar, elemObjPtr), 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_FONT, 
	Blt_Offset(Colorbar, font), 0},

    {BLT_CONFIG_STRING, "-fonbottomvariable", "bottomVariable", "BottomVariable",
	DEF_MARGIN_VAR, Blt_Offset(Colorbar, bottomMargin.varName), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-bufferelements", "bufferElements", "BufferElements",
	DEF_BUFFER_ELEMENTS, Blt_Offset(Colorbar, backingStore),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-buffergraph", "bufferColorbar", "BufferColorbar",
	DEF_BUFFER_GRAPH, Blt_Offset(Colorbar, doubleBuffer),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_CURSOR, Blt_Offset(Colorbar, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-data", "data", "Data", 
        (char *)NULL, Blt_Offset(Colorbar, data), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-datacommand", "dataCommand", "DataCommand", 
        (char *)NULL, Blt_Offset(Colorbar, dataCmd), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	DEF_TITLE_COLOR, Blt_Offset(Colorbar, titleTextStyle.color), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-halo", "halo", "Halo", DEF_HALO, 
	Blt_Offset(Colorbar, halo), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_HEIGHT, 
	Blt_Offset(Colorbar, reqHeight), 0},
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_HIGHLIGHT_BACKGROUND, 
	Blt_Offset(Colorbar, highlightBgColor), 0},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_HIGHLIGHT_COLOR, Blt_Offset(Colorbar, highlightColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_HIGHLIGHT_WIDTH, 
	Blt_Offset(Colorbar, highlightWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-unmaphiddenelements", "unmapHiddenElements", 
	"UnmapHiddenElements", DEF_UNMAP_HIDDEN_ELEMENTS, 
	Blt_Offset(Colorbar, flags), ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)UNMAP_HIDDEN},
    {BLT_CONFIG_BOOLEAN, "-invertxy", "invertXY", "InvertXY", 
	DEF_INVERT_XY, Blt_Offset(Colorbar, inverted),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_JUSTIFY, 
	Blt_Offset(Colorbar, titleTextStyle.justify), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-leftmargin", "leftMargin", "Margin", 
	DEF_MARGIN, Blt_Offset(Colorbar, leftMargin.reqSize), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-leftvariable", "leftVariable", "LeftVariable",
	DEF_MARGIN_VAR, Blt_Offset(Colorbar, leftMargin.varName), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-lm", "leftMargin", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_BACKGROUND, "-plotbackground", "plotBackground", "Background",
	DEF_PLOT_BACKGROUND, Blt_Offset(Colorbar, plotBg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-plotborderwidth", "plotBorderWidth", 
        "PlotBorderWidth", DEF_PLOT_BORDERWIDTH, 
	Blt_Offset(Colorbar, plotBW), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-plotpadx", "plotPadX", "PlotPad", DEF_PLOT_PADX, 
	Blt_Offset(Colorbar, xPad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-plotpady", "plotPadY", "PlotPad", DEF_PLOT_PADY, 
	Blt_Offset(Colorbar, yPad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-plotrelief", "plotRelief", "Relief", 
	DEF_PLOT_RELIEF, Blt_Offset(Colorbar, plotRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RELIEF, 
	Blt_Offset(Colorbar, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-rightmargin", "rightMargin", "Margin",
	DEF_MARGIN, Blt_Offset(Colorbar, rightMargin.reqSize),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-rightvariable", "rightVariable", "RightVariable",
	DEF_MARGIN_VAR, Blt_Offset(Colorbar, rightMargin.varName), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-rm", "rightMargin", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_BOOLEAN, "-stackaxes", "stackAxes", "StackAxes", 
	DEF_STACK_AXES, Blt_Offset(Colorbar, stackAxes),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_TAKE_FOCUS, Blt_Offset(Colorbar, takeFocus), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-title", "title", "Title", DEF_TITLE, 
	Blt_Offset(Colorbar, title), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-tm", "topMargin", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-topmargin", "topMargin", "Margin", 
	DEF_MARGIN, Blt_Offset(Colorbar, topMargin.reqSize), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-topvariable", "topVariable", "TopVariable",
	DEF_MARGIN_VAR, Blt_Offset(Colorbar, topMargin.varName), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_WIDTH, 
	Blt_Offset(Colorbar, reqWidth), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-plotwidth", "plotWidth", "PlotWidth", 
	(char *)NULL, Blt_Offset(Colorbar, reqPlotWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-plotheight", "plotHeight", "PlotHeight", 
	(char *)NULL, Blt_Offset(Colorbar, reqPlotHeight), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static int 
GetContourElement(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr, 
		  ContourElement **elemPtrPtr)
{
    Element *basePtr;

    if (Blt_GetElement(interp, graphPtr, objPtr, &basePtr) != TCL_OK) {
	return TCL_ERROR;	/* Can't find named element */
    }
    if (basePtr->obj.classId != CID_ELEM_CONTOUR) {
	Tcl_AppendResult(interp, "element \"", Tcl_GetString(objPtr), 
		"\" is not a contour element", (char *)NULL);
	return TCL_ERROR;
    }
    *elemPtrPtr = (ContourElement *)basePtr;
    return TCL_OK;
}

static int 
GetColorbarGeometry(Graph *graphPtr, int width, int height) 
{
    Axis *axisPtr;
    int right, left, top, bottom;

    axisPtr = elemPtr->zAxisPtr;
    /* 
     * horizontal color bar
     *
     *	min ---------------------- max 
     *      |  |   |   |    |    |
     */
    if (Blt_GetAxisGeometry(graphPtr, axisPtr)) {
    }
    if (!Blt_Legend_IsHidden(graphPtr)) {
	switch (Blt_Legend_Site(graphPtr)) {
	case LEGEND_RIGHT:
	    right += Blt_Legend_Width(graphPtr) + 2;
	    break;
	case LEGEND_LEFT:
	    left += Blt_Legend_Width(graphPtr) + 2;
	    break;
	case LEGEND_TOP:
	    top += Blt_Legend_Height(graphPtr) + 2;
	    break;
	case LEGEND_BOTTOM:
	    bottom += Blt_Legend_Height(graphPtr) + 2;
	    break;
	case LEGEND_XY:
	case LEGEND_PLOT:
	case LEGEND_WINDOW:
	    /* Do nothing. */
	    break;
	}
    }
#ifdef notdef
    fprintf(stderr, "legend site=%d, x=%d y=%d w=%d h=%d\n", 
	    Blt_Legend_Site(graphPtr), 
	    Blt_Legend_X(graphPtr), 
	    Blt_Legend_Y(graphPtr), 
	    Blt_Legend_Width(graphPtr), 
	    Blt_Legend_Height(graphPtr));
#4endif
    return TCL_OK;
}

static void
DrawColorbar(Graph *graphPtr, Tcl_Interp *interp, Drawable *drawable) 
{
    Colorbar *cbarPtr = &graphPtr->colorbar;
    ContourElement *elemPtr;
    Axis *axisPtr;

    /* Always look up the associated element to see if it's been deleted. */
    if (cbarPtr->elemObjPtr == NULL) {
	return;				/* No element associated. */
    }
    if (GetContourElement(interp, graphPtr, cbarPtr->elemObjPtr, &elemPtr) 
	!= TCL_OK) {
	return;				/* Not a contour element. */
    }
    pixmap = Blt_GetPixmap(graphPtr->display, drawable, cbarPtr->width, 
	cbarPtr->height, Tk_Depth(graphPtr->tkwin));

    if (cbarPtr->bg != NULL) {
	/* Fill background of area. */
	Blt_Bg_FillRectangle(tkwin, pixmap, cbarPtr->bg, 0, 0, 
		cbarPtr->width, cbarPtr->height, 0, TK_RELIEF_FLAT);
    }
    axisPtr = elemPtr->
    /*  */
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *	Queries color attributes (font, line width, label, etc).
 *
 * Results:
 *	A standard TCL result.  If querying configuration values,
 *	interp->result will contain the results.
 *
 *	.g colorbar cget -option 
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Colorbar *cbarPtr;

    cbarPtr = &graphPtr->colorbar;
    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, 
	configSpecs, (char *)cbarPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *	Queries or resets pen attributes (font, line width, color, etc).
 *
 * Results:
 *	A standard TCL result.  If querying configuration values,
 *	interp->result will contain the results.
 *
 * Side Effects:
 *	Colorbar resources are possibly allocated (GC, font).
 *
 *	.g colorbar configure -option value...
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Colorbar *cbarPtr;

    cbarPtr = &graphPtr->colorbar;
    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
	    configSpecs, (char *)cbarPtr, (Tcl_Obj *)NULL, BLT_CONFIG_OBJV_ONLY);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
	    configSpecs, (char *)cbarPtr, objv[3], BLT_CONFIG_OBJV_ONLY);
    }
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
	 objc - 3, objv + 3, (char *)cbarPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    ConfigureColorbar(graphPtr, cbarPtr);
    graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}


void
Blt_Colorbar_Geometry(Graph *graphPtr)
{
    Colorbar *cbarPtr;
    Colormap *cmapPtr;
    
    cbarPtr = &graphPtr->colorbar;
    cmapPtr = cbarPtr->colormapPtr;
    cbarPtr->width = cbarPtr->height = 0;
    if (cmapPtr == NULL) {
	return;				/* No colormap attached. */
    }
    if (cmapPtr->axisPtr == NULL) {
	return;				/* No axis specified. */
    }
    if (cmapPtr->palette == NULL) {
	return;				/* No palette specified. */
    }
    cbarPtr->width = COLORBAR_WIDTH + cmapPtr->axisPtr->width;
    cbarPtr->height = cmapPtr->axisPtr->height;
    
}

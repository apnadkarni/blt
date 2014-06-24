/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltTable.c --
 *
 * This module implements a table-based geometry manager for the BLT toolkit.
 *
 *	Copyright 1993-2004 George A Howlett.
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
 * To do:
 *
 * 3) No way to detect if widget is already a container of another geometry
 *    manager.  This one is especially bad with toplevel widgets, causing the
 *    window manager to lock-up trying to handle the myriads of resize requests.
 *
 *   Note:  This problem continues in Tk 8.x.  It's possible for a widget
 *	    to be a container for two different geometry managers.  Each manager
 *	    will set its own requested geometry for the container widget. The
 *	    winner sets the geometry last (sometimes ad infinitum).
 *
 * 7) Relative sizing of partitions?
 *
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif	/* HAVE_LIMITS_H */

#include "bltAlloc.h"
#include "bltTable.h"
#include "bltSwitch.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define TABLE_THREAD_KEY	"BLT Table Data"
#define TABLE_DEF_PAD		0

/*
 * Default values for widget attributes.
 */
#define DEF_TABLE_ANCHOR	"center"
#define DEF_TABLE_COLUMNS 	"0"
#define DEF_TABLE_FILL		"none"
#define DEF_TABLE_PAD		"0"
#define DEF_TABLE_PROPAGATE 	"1"
#define DEF_TABLE_RESIZE	"both"
#define DEF_TABLE_ROWS		"0"
#define DEF_TABLE_SPAN		"1"
#define DEF_TABLE_CONTROL	"normal"
#define DEF_TABLE_WEIGHT	"1.0"

#define ENTRY_DEF_PAD		0
#define ENTRY_DEF_ANCHOR	TK_ANCHOR_CENTER
#define ENTRY_DEF_FILL		FILL_NONE
#define ENTRY_DEF_SPAN		1
#define ENTRY_DEF_CONTROL	CONTROL_NORMAL
#define ENTRY_DEF_IPAD		0

#define ROWCOL_DEF_RESIZE	(RESIZE_BOTH | RESIZE_VIRGIN)
#define ROWCOL_DEF_PAD		0
#define ROWCOL_DEF_WEIGHT	1.0

#define	MATCH_PATTERN   (1<<0)	/* Find widgets whose path names
				 * match a given pattern */
#define	MATCH_SPAN	(1<<1)	/* Find widgets that span index  */
#define	MATCH_START	(1<<2)	/* Find widgets that start at index */


static Blt_Uid rowUid, columnUid;

static Tk_GeomRequestProc WidgetGeometryProc;
static Tk_GeomLostSlaveProc WidgetCustodyProc;
static Tk_GeomMgr tableMgrInfo =
{
    (char *)"table",		/* Name of geometry manager used by winfo */
    WidgetGeometryProc,		/* Procedure to for new geometry requests */
    WidgetCustodyProc,		/* Procedure when widget is taken away */
};

static Blt_OptionParseProc ObjToLimits;
static Blt_OptionPrintProc LimitsToObj;
static Blt_CustomOption limitsOption =
{
    ObjToLimits, LimitsToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToControl;
static Blt_OptionPrintProc ControlToObj;
static Blt_CustomOption controlOption =
{
    ObjToControl, ControlToObj, NULL, (ClientData)0
};

static Blt_ConfigSpec rowConfigSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-height", (char *)NULL, (char *)NULL, (char *)NULL, 
	Blt_Offset(RowColumn, reqSize), 0, &limitsOption},
    {BLT_CONFIG_PAD, "-pady", (char *)NULL, (char *)NULL, DEF_TABLE_PAD, 
	Blt_Offset(RowColumn, pad), BLT_CONFIG_DONT_SET_DEFAULT, },
    {BLT_CONFIG_RESIZE, "-resize", (char *)NULL, (char *)NULL, DEF_TABLE_RESIZE,
	Blt_Offset(RowColumn, resize), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FLOAT, "-weight", (char *)NULL, (char *)NULL, DEF_TABLE_WEIGHT,
	Blt_Offset(RowColumn, weight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec columnConfigSpecs[] =
{
    {BLT_CONFIG_PAD, "-padx", (char *)NULL, (char *)NULL, DEF_TABLE_PAD, 
	Blt_Offset(RowColumn, pad), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_RESIZE, "-resize", (char *)NULL, (char *)NULL, DEF_TABLE_RESIZE,
	Blt_Offset(RowColumn, resize), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FLOAT, "-weight", (char *)NULL, (char *)NULL, DEF_TABLE_WEIGHT,
	Blt_Offset(RowColumn, weight), BLT_CONFIG_DONT_SET_DEFAULT, 
	&limitsOption},
    {BLT_CONFIG_CUSTOM, "-width", (char *)NULL, (char *)NULL, (char *)NULL, 
	Blt_Offset(RowColumn, reqSize), 0, &limitsOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};


static Blt_ConfigSpec entryConfigSpecs[] =
{
    {BLT_CONFIG_ANCHOR, "-anchor", (char *)NULL, (char *)NULL,
	DEF_TABLE_ANCHOR, Blt_Offset(TableEntry, anchor),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_INT, "-columnspan", "columnSpan", (char *)NULL,
	DEF_TABLE_SPAN, Blt_Offset(TableEntry, column.span),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-columncontrol", "columnControl", (char *)NULL,
	DEF_TABLE_CONTROL, Blt_Offset(TableEntry, column.control),
	BLT_CONFIG_DONT_SET_DEFAULT, &controlOption},
    {BLT_CONFIG_SYNONYM, "-cspan", "columnSpan", (char *)NULL, (char *)NULL, 
	Blt_Offset(TableEntry, column.span), 0},
    {BLT_CONFIG_SYNONYM, "-ccontrol", "columnControl", (char *)NULL, 
	(char *)NULL, Blt_Offset(TableEntry, column.control), 0},
    {BLT_CONFIG_FILL, "-fill", (char *)NULL, (char *)NULL, DEF_TABLE_FILL, 
	Blt_Offset(TableEntry, fill), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-height", "reqHeight", (char *)NULL,
	(char *)NULL, Blt_Offset(TableEntry, reqHeight), 0},
    {BLT_CONFIG_PAD, "-padx", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(TableEntry, xPad), 0},
    {BLT_CONFIG_PAD, "-pady", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(TableEntry, yPad), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-ipadx", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(TableEntry, ixPad), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-ipady", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(TableEntry, iyPad), 0},
    {BLT_CONFIG_CUSTOM, "-reqheight", "reqHeight", (char *)NULL, (char *)NULL, 
	Blt_Offset(TableEntry, reqHeight), 0, &limitsOption},
    {BLT_CONFIG_CUSTOM, "-reqwidth", "reqWidth", (char *)NULL, (char *)NULL, 
	Blt_Offset(TableEntry, reqWidth), 0, &limitsOption},
    {BLT_CONFIG_INT, "-rowspan", "rowSpan", (char *)NULL, DEF_TABLE_SPAN, 
	Blt_Offset(TableEntry, row.span), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-rowcontrol", "rowControl", (char *)NULL,
	DEF_TABLE_CONTROL, Blt_Offset(TableEntry, row.control),
	BLT_CONFIG_DONT_SET_DEFAULT, &controlOption},
    {BLT_CONFIG_SYNONYM, "-rspan", "rowSpan", (char *)NULL, (char *)NULL, 
	Blt_Offset(TableEntry, row.span), 0},
    {BLT_CONFIG_SYNONYM, "-rcontrol", "rowControl", (char *)NULL, (char *)NULL,
	Blt_Offset(TableEntry, row.control), 0},
    {BLT_CONFIG_SYNONYM, "-width", "reqWidth", (char *)NULL, (char *)NULL, 
	Blt_Offset(TableEntry, reqWidth), 0},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};


static Blt_ConfigSpec tableConfigSpecs[] =
{
    {BLT_CONFIG_PAD, "-padx", (char *)NULL, (char *)NULL,
	DEF_TABLE_PAD, Blt_Offset(Table, xPad),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-pady", (char *)NULL, (char *)NULL,
	DEF_TABLE_PAD, Blt_Offset(Table, yPad),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-propagate", (char *)NULL, (char *)NULL,
	DEF_TABLE_PROPAGATE, Blt_Offset(Table, propagate),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-reqheight", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(Table, reqHeight), 0, &limitsOption},
    {BLT_CONFIG_CUSTOM, "-reqwidth", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(Table, reqWidth), 0, &limitsOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_SwitchParseProc ObjToPosition;
static Blt_SwitchCustom positionSwitch =
{
    ObjToPosition, NULL, NULL, (ClientData)0,
};

typedef struct {
    int rspan, cspan;
    int rstart, cstart;
    int flags;
    const char *pattern;
    Table *tablePtr;
} SearchSwitches;

static Blt_SwitchSpec searchSwitches[] = 
{
    {BLT_SWITCH_STRING, "-pattern", "pattern", (char *)NULL,
	Blt_Offset(SearchSwitches, pattern), 0, 0,},
    {BLT_SWITCH_CUSTOM, "-span",    "int", (char *)NULL,
	0, 0, 0, &positionSwitch},
    {BLT_SWITCH_CUSTOM, "-start",   "position", (char *)NULL,
	0, 0, 0, &positionSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    const char *pattern;
    const char *slave;
} NamesSwitches;

static Blt_SwitchSpec namesSwitches[] = 
{
    {BLT_SWITCH_STRING, "-pattern", "pattern", (char *)NULL,
	Blt_Offset(NamesSwitches, pattern), 0, 0,},
    {BLT_SWITCH_STRING, "-slave", "widget", (char *)NULL,
	Blt_Offset(NamesSwitches, slave), 0, 0},
    {BLT_SWITCH_END}
};

/*
 * Forward declarations
 */
static Tcl_FreeProc DestroyTable;
static Tcl_IdleProc ArrangeTable;
static Tcl_InterpDeleteProc TableInterpDeleteProc;
static Tcl_ObjCmdProc TableCmd;
static Tk_EventProc TableEventProc;
static Tk_EventProc WidgetEventProc;

static void DestroyEntry(TableEntry * tePtr);
static void BinEntry(Table *tablePtr, TableEntry * tePtr);
static RowColumn *InitSpan(PartitionInfo * piPtr, int start, int span);
static int ParseItem(Table *tablePtr, const char *string, int *rowPtr, 
	int *colPtr);

static EntrySearchProc FindEntry;

typedef int (TableCmdProc)(TableInterpData *dataPtr, Tcl_Interp *interp, 
	int objc, Tcl_Obj *const *objv);

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLimits --
 *
 *	Converts the list of elements into zero or more pixel values which
 *	determine the range of pixel values possible.  An element can be in
 *	any form accepted by Tk_GetPixels. The list has a different meaning
 *	based upon the number of elements.
 *
 *	    # of elements:
 *
 *	    0 - the limits are reset to the defaults.
 *	    1 - the minimum and maximum values are set to this
 *		value, freezing the range at a single value.
 *	    2 - first element is the minimum, the second is the
 *		maximum.
 *	    3 - first element is the minimum, the second is the
 *		maximum, and the third is the nominal value.
 *
 *	Any element may be the empty string which indicates the default.
 *
 * Results:
 *	The return value is a standard TCL result.  The min and max fields
 *	of the range are set.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToLimits(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Widget of table */
    Tcl_Obj *objPtr,		/* New width list */
    char *widgRec,		/* Widget record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    Limits *limitsPtr = (Limits *)(widgRec + offset);
    const char **argv;
    int argc;
    int limArr[3];
    Tk_Window winArr[3];
    int limitsFlags;

    argv = NULL;
    argc = 0;

    /* Initialize limits to default values */
    limArr[2] = LIMITS_NOM;
    limArr[1] = LIMITS_MAX;
    limArr[0] = LIMITS_MIN;
    winArr[0] = winArr[1] = winArr[2] = NULL;
    limitsFlags = 0;

    if (objPtr != NULL) {
	int size;
	int i;
	const char *string;

	string = Tcl_GetString(objPtr);
	if (Tcl_SplitList(interp, string, &argc, &argv) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (argc > 3) {
	    Tcl_AppendResult(interp, "wrong # limits \"", string, "\"",
		(char *)NULL);
	    goto error;
	}
	for (i = 0; i < argc; i++) {
	    if (argv[i][0] == '\0') {
		continue;	/* Empty string: use default value */
	    }
	    limitsFlags |= (LIMITS_SET_BIT << i);
	    if ((argv[i][0] == '.') &&
		((argv[i][1] == '\0') || isalpha(UCHAR(argv[i][1])))) {
		Tk_Window tkwin2;

		/* Widget specified: save pointer to widget */
		tkwin2 = Tk_NameToWindow(interp, argv[i], tkwin);
		if (tkwin2 == NULL) {
		    goto error;
		}
		winArr[i] = tkwin2;
	    } else {
		if (Tk_GetPixels(interp, tkwin, argv[i], &size) != TCL_OK) {
		    goto error;
		}
		if ((size < LIMITS_MIN) || (size > LIMITS_MAX)) {
		    Tcl_AppendResult(interp, "bad limits \"", string, "\"",
			(char *)NULL);
		    goto error;
		}
		limArr[i] = size;
	    }
	}
	Blt_Free(argv);
    }
    /*
    * Check the limits specified.  We can't check the requested
    * size of widgets.
    */
    switch (argc) {
    case 1:
	limitsFlags |= (LIMITS_SET_MIN | LIMITS_SET_MAX);
	if (winArr[0] == NULL) {
	    limArr[1] = limArr[0];	/* Set minimum and maximum to value */
	} else {
	    winArr[1] = winArr[0];
	}
	break;

    case 2:
	if ((winArr[0] == NULL) && (winArr[1] == NULL) &&
	    (limArr[1] < limArr[0])) {
	    Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
		"\": min > max", (char *)NULL);
	    return TCL_ERROR;	/* Minimum is greater than maximum */
	}
	break;

    case 3:
	if ((winArr[0] == NULL) && (winArr[1] == NULL)) {
	    if (limArr[1] < limArr[0]) {
		Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
		    "\": min > max", (char *)NULL);
		return TCL_ERROR;	/* Minimum is greater than maximum */
	    }
	    if ((winArr[2] == NULL) &&
		((limArr[2] < limArr[0]) || (limArr[2] > limArr[1]))) {
		Tcl_AppendResult(interp, "nominal value \"", 
		    Tcl_GetString(objPtr),
		    "\" out of range", (char *)NULL);
		return TCL_ERROR;	/* Nominal is outside of range defined
					 * by minimum and maximum */
	    }
	}
	break;
    }
    limitsPtr->min = limArr[0];
    limitsPtr->max = limArr[1];
    limitsPtr->nom = limArr[2];
    limitsPtr->wMin = winArr[0];
    limitsPtr->wMax = winArr[1];
    limitsPtr->wNom = winArr[2];
    limitsPtr->flags = limitsFlags;
    return TCL_OK;
  error:
    Blt_Free(argv);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetLimits --
 *
 *	Resets the limits to their default values.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
INLINE static void
ResetLimits(Limits *limitsPtr)	/* Limits to be imposed on the value */
{
    limitsPtr->flags = 0;
    limitsPtr->min = LIMITS_MIN;
    limitsPtr->max = LIMITS_MAX;
    limitsPtr->nom = LIMITS_NOM;
    limitsPtr->wNom = limitsPtr->wMax = limitsPtr->wMin = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBoundedWidth --
 *
 *	Bounds a given width value to the limits described in the limit
 *	structure.  The initial starting value may be overridden by the
 *	nominal value in the limits.
 *
 * Results:
 *	Returns the constrained value.
 *
 *---------------------------------------------------------------------------
 */
static int
GetBoundedWidth(
    int width,			/* Initial value to be constrained */
    Limits *limitsPtr)		/* Limits to be imposed on the value */
{
    /*
     * Check widgets for requested width values;
     */
    if (limitsPtr->wMin != NULL) {
	limitsPtr->min = Tk_ReqWidth(limitsPtr->wMin);
    }
    if (limitsPtr->wMax != NULL) {
	limitsPtr->max = Tk_ReqWidth(limitsPtr->wMax);
    }
    if (limitsPtr->wNom != NULL) {
	limitsPtr->nom = Tk_ReqWidth(limitsPtr->wNom);
    }
    if (limitsPtr->flags & LIMITS_SET_NOM) {
	width = limitsPtr->nom;	/* Override initial value */
    }
    if (width < limitsPtr->min) {
	width = limitsPtr->min;	/* Bounded by minimum value */
    } else if (width > limitsPtr->max) {
	width = limitsPtr->max;	/* Bounded by maximum value */
    }
    return width;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBoundedHeight --
 *
 *	Bounds a given value to the limits described in the limit structure.
 *	The initial starting value may be overridden by the nominal value in
 *	the limits.
 *
 * Results:
 *	Returns the constrained value.
 *
 *---------------------------------------------------------------------------
 */
static int
GetBoundedHeight(
    int height,			/* Initial value to be constrained */
    Limits *limitsPtr)		/* Limits to be imposed on the value */
{
    /*
     * Check widgets for requested height values;
     */
    if (limitsPtr->wMin != NULL) {
	limitsPtr->min = Tk_ReqHeight(limitsPtr->wMin);
    }
    if (limitsPtr->wMax != NULL) {
	limitsPtr->max = Tk_ReqHeight(limitsPtr->wMax);
    }
    if (limitsPtr->wNom != NULL) {
	limitsPtr->nom = Tk_ReqHeight(limitsPtr->wNom);
    }
    if (limitsPtr->flags & LIMITS_SET_NOM) {
	height = limitsPtr->nom;/* Override initial value */
    }
    if (height < limitsPtr->min) {
	height = limitsPtr->min;/* Bounded by minimum value */
    } else if (height > limitsPtr->max) {
	height = limitsPtr->max;/* Bounded by maximum value */
    }
    return height;
}

/*
 *---------------------------------------------------------------------------
 *
 * NameOfLimits --
 *
 *	Convert the values into a list representing the limits.
 *
 * Results:
 *	The static string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */
static const char *
NameOfLimits(Limits *limitsPtr)
{
    Tcl_DString buffer;
#define STRING_SPACE 200
    static char string[STRING_SPACE + 1];

    Tcl_DStringInit(&buffer);

    if (limitsPtr->wMin != NULL) {
	Tcl_DStringAppendElement(&buffer, Tk_PathName(limitsPtr->wMin));
    } else if (limitsPtr->flags & LIMITS_SET_MIN) {
	Tcl_DStringAppendElement(&buffer, Blt_Itoa(limitsPtr->min));
    } else {
	Tcl_DStringAppendElement(&buffer, "");
    }

    if (limitsPtr->wMax != NULL) {
	Tcl_DStringAppendElement(&buffer, Tk_PathName(limitsPtr->wMax));
    } else if (limitsPtr->flags & LIMITS_SET_MAX) {
	Tcl_DStringAppendElement(&buffer, Blt_Itoa(limitsPtr->max));
    } else {
	Tcl_DStringAppendElement(&buffer, "");
    }

    if (limitsPtr->wNom != NULL) {
	Tcl_DStringAppendElement(&buffer, Tk_PathName(limitsPtr->wNom));
    } else if (limitsPtr->flags & LIMITS_SET_NOM) {
	Tcl_DStringAppendElement(&buffer, Blt_Itoa(limitsPtr->nom));
    } else {
	Tcl_DStringAppendElement(&buffer, "");
    }
    strncpy(string, Tcl_DStringValue(&buffer), STRING_SPACE);
    string[STRING_SPACE] = '\0';
    return string;
}

/*
 *---------------------------------------------------------------------------
 *
 * LimitsToObj --
 *
 *	Convert the limits of the pixel values allowed into a list.
 *
 * Results:
 *	The string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LimitsToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Not used. */
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Row/column structure record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    Limits *limitsPtr = (Limits *)(widgRec + offset);

    return Tcl_NewStringObj(NameOfLimits(limitsPtr), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToControl --
 *
 *	Converts the control string into its numeric representation.  Valid
 *	control strings are "none", "normal", and "full".
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToControl(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* Control style string */
    char *widgRec,		/* Entry structure record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    const char *string;
    char c;
    float *controlPtr = (float *)(widgRec + offset);
    int bool;
    int length;

    if (Tcl_GetBooleanFromObj(NULL, objPtr, &bool) == TCL_OK) {
	*controlPtr = (float)bool;
	return TCL_OK;
    }
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (length > 1) &&
	(strncmp(string, "normal", length) == 0)) {
	*controlPtr = CONTROL_NORMAL;
    } else if ((c == 'n') && (length > 1) &&
	(strncmp(string, "none", length) == 0)) {
	*controlPtr = CONTROL_NONE;
    } else if ((c == 'f') && (strncmp(string, "full", length) == 0)) {
	*controlPtr = CONTROL_FULL;
    } else {
	double control;

	if ((Tcl_GetDoubleFromObj(interp, objPtr, &control) != TCL_OK) ||
	    (control < 0.0)) {
	    Tcl_AppendResult(interp, "bad control argument \"", string,
		"\": should be \"normal\", \"none\", or \"full\"",
		(char *)NULL);
	    return TCL_ERROR;
	}
	*controlPtr = (float)control;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NameOfControl --
 *
 *	Converts the control value into its string representation.
 *
 * Results:
 *	Returns a pointer to the static name string.
 *
 *---------------------------------------------------------------------------
 */
static const char *
NameOfControl(float control)
{
    if (control == CONTROL_NORMAL) {
	return "normal";
    } else if (control == CONTROL_NONE) {
	return "none";
    } else if (control == CONTROL_FULL) {
	return "full";
    } else {
	static char string[TCL_DOUBLE_SPACE];

	Blt_FormatString(string, TCL_DOUBLE_SPACE, "%g", (double)control);
	return string;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ControlToObj --
 *
 *	Returns control mode string based upon the control flags.
 *
 * Results:
 *	The control mode string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ControlToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Not used. */
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Row/column structure record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    float control = *(float *)(widgRec + offset);

    return Tcl_NewStringObj(NameOfControl(control), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPosition --
 *
 *	Converts the position mode into its numeric representation.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPosition(
    ClientData clientData,	/* Flag indicating if the node is considered
				 * before or after the insertion position. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    const char *switchName,	/* Not used. */
    Tcl_Obj *objPtr,		/* String representation */
    char *record,		/* Structure record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    SearchSwitches *searchPtr = (SearchSwitches *)record;
    int row, column;

    if (ParseItem(searchPtr->tablePtr, Tcl_GetString(objPtr), &row, &column) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    if (strcmp(switchName, "-span") == 0) {
	searchPtr->flags |= MATCH_SPAN;
	searchPtr->rspan = row;
	searchPtr->cspan = column;
    } else if (strcmp(switchName, "-start") == 0) {
	searchPtr->flags |= MATCH_START;
	searchPtr->rstart = row;
	searchPtr->cstart = column;
    }
    return TCL_OK;
}


static void
EventuallyArrangeTable(Table *tablePtr)
{
    if (!(tablePtr->flags & ARRANGE_PENDING)) {
	tablePtr->flags |= ARRANGE_PENDING;
	Tcl_DoWhenIdle(ArrangeTable, tablePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TableEventProc --
 *
 *	This procedure is invoked by the Tk event handler when the container
 *	widget is reconfigured or destroyed.
 *
 *	The table will be rearranged at the next idle point if the container
 *	widget has been resized or moved. There's a distinction made between
 *	parent and non-parent container arrangements.  When the container is
 *	the parent of the embedded widgets, the widgets will automatically
 *	keep their positions relative to the container, even when the
 *	container is moved.  But if the container is not the parent, those
 *	widgets have to be moved manually.  This can be a performance hit in
 *	rare cases where we're scrolling the container (by moving the window)
 *	and there are lots of non-child widgets arranged inside.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the table associated with tkwin to have its layout
 *	re-computed and drawn at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static void
TableEventProc(
    ClientData clientData,	/* Information about widget */
    XEvent *eventPtr)		/* Information about event */
{
    Table *tablePtr = clientData;

    if (eventPtr->type == ConfigureNotify) {
	if ((tablePtr->container.width != Tk_Width(tablePtr->tkwin)) ||
	    (tablePtr->container.height != Tk_Height(tablePtr->tkwin))
	    || (tablePtr->flags & NON_PARENT)) {
	    EventuallyArrangeTable(tablePtr);
	}
    } else if (eventPtr->type == DestroyNotify) {
	if (tablePtr->flags & ARRANGE_PENDING) {
	    Tcl_CancelIdleCall(ArrangeTable, tablePtr);
	}
	tablePtr->tkwin = NULL;
	Tcl_EventuallyFree(tablePtr, DestroyTable);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * WidgetEventProc --
 *
 *	This procedure is invoked by the Tk event handler when StructureNotify
 *	events occur in a widget managed by the table.
 *
 *	For example, when a managed widget is destroyed, it frees the
 *	corresponding entry structure and arranges for the table layout to be
 *	re-computed at the next idle point.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the managed widget was deleted, the Entry structure gets cleaned up
 *	and the table is rearranged.
 *
 *---------------------------------------------------------------------------
 */
static void
WidgetEventProc(
    ClientData clientData,	/* Pointer to Entry structure for widget
				 * referred to by eventPtr. */
    XEvent *eventPtr)		/* Describes what just happened. */
{
    TableEntry *tePtr = clientData;
    Table *tablePtr;

    tablePtr = tePtr->tablePtr;
    if (eventPtr->type == ConfigureNotify) {
	int borderWidth;

	tablePtr->flags |= REQUEST_LAYOUT;
	borderWidth = Tk_Changes(tePtr->tkwin)->border_width;
	if (tePtr->borderWidth != borderWidth) {
	    tePtr->borderWidth = borderWidth;
	    EventuallyArrangeTable(tablePtr);
	}
    } else if (eventPtr->type == DestroyNotify) {
	DestroyEntry(tePtr);
	tablePtr->flags |= REQUEST_LAYOUT;
	EventuallyArrangeTable(tablePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * WidgetCustodyProc --
 *
 * 	This procedure is invoked when a widget has been stolen by another
 * 	geometry manager.  The information and memory associated with the
 * 	widget is released.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the table to have its layout re-arranged at the next idle
 *	point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
WidgetCustodyProc(
    ClientData clientData,	/* Information about the widget */
    Tk_Window tkwin)		/* Not used. */
{
    TableEntry *tePtr = (TableEntry *) clientData;
    Table *tablePtr = tePtr->tablePtr;

    if (Tk_IsMapped(tePtr->tkwin)) {
	Tk_UnmapWindow(tePtr->tkwin);
    }
    Tk_UnmaintainGeometry(tePtr->tkwin, tablePtr->tkwin);
    DestroyEntry(tePtr);
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * WidgetGeometryProc --
 *
 *	This procedure is invoked by Tk_GeometryRequest for widgets managed by
 *	the table geometry manager.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the table to have its layout re-computed and re-arranged
 *	at the next idle point.
 *
 * ---------------------------------------------------------------------------- */
/* ARGSUSED */
static void
WidgetGeometryProc(
    ClientData clientData,	/* Information about widget that got new
				 * preferred geometry.  */
    Tk_Window tkwin)		/* Other Tk-related information about the
			         * widget. */
{
    TableEntry *tePtr = (TableEntry *) clientData;

    tePtr->tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tePtr->tablePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FindEntry --
 *
 *	Searches for the table entry corresponding to the given widget.
 *
 * Results:
 *	If a structure associated with the widget exists, a pointer to that
 *	structure is returned. Otherwise NULL.
 *
 *---------------------------------------------------------------------------
 */
static TableEntry *
FindEntry(
    Table *tablePtr,
    Tk_Window tkwin)		/* Widget associated with table entry */
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&tablePtr->entryTable, (char *)tkwin);
    if (hPtr == NULL) {
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}


static int
GetEntry(Tcl_Interp *interp, Table *tablePtr, const char *string, 
	 TableEntry **tePtrPtr)
{
    Tk_Window tkwin;
    TableEntry *tePtr;

    tkwin = Tk_NameToWindow(interp, string, tablePtr->tkwin);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    tePtr = FindEntry(tablePtr, tkwin);
    if (tePtr == NULL) {
	Tcl_AppendResult(interp, "\"", Tk_PathName(tkwin),
	    "\" is not managed by any table", (char *)NULL);
	return TCL_ERROR;
    }
    *tePtrPtr = tePtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateEntry --
 *
 *	This procedure creates and initializes a new Entry structure to hold a
 *	widget.  A valid widget has a parent widget that is either a) the
 *	container widget itself or b) a mutual ancestor of the container
 *	widget.
 *
 * Results:
 *	Returns a pointer to the new structure describing the new widget
 *	entry.  If an error occurred, then the return value is NULL and an
 *	error message is left in interp->result.
 *
 * Side effects:
 *	Memory is allocated and initialized for the Entry structure.
 *
 * ---------------------------------------------------------------------------- 
 */
static TableEntry *
CreateEntry(
    Table *tablePtr,
    Tk_Window tkwin)
{
    TableEntry *tePtr;
    int dummy;
    Tk_Window parent, ancestor;

    /*
     * Check that this widget can be managed by this table.  A valid widget
     * has a parent widget that either
     *
     *    1) is the container widget, or
     *    2) is a mutual ancestor of the container widget.
     */
    ancestor = Tk_Parent(tkwin);
    for (parent = tablePtr->tkwin; (parent != ancestor) &&
	(!Tk_IsTopLevel(parent)); parent = Tk_Parent(parent)) {
	/* empty */
    }
    if (ancestor != parent) {
	Tcl_AppendResult(tablePtr->interp, "can't manage \"",
	    Tk_PathName(tkwin), "\" in table \"", Tk_PathName(tablePtr->tkwin),
	    "\"", (char *)NULL);
	return NULL;
    }
    tePtr = Blt_AssertCalloc(1, sizeof(TableEntry));

    /* Initialize the entry structure */

    tePtr->tkwin = tkwin;
    tePtr->tablePtr = tablePtr;
    tePtr->borderWidth = Tk_Changes(tkwin)->border_width;
    tePtr->fill = ENTRY_DEF_FILL;
    tePtr->row.control = tePtr->column.control = ENTRY_DEF_CONTROL;
    tePtr->anchor = ENTRY_DEF_ANCHOR;
    tePtr->row.span = tePtr->column.span = ENTRY_DEF_SPAN;
    ResetLimits(&tePtr->reqWidth);
    ResetLimits(&tePtr->reqHeight);

    /*
     * Add the entry to the following data structures.
     *
     * 	1) A chain of widgets managed by the table.
     *   2) A hash table of widgets managed by the table.
     */
    tePtr->link = Blt_Chain_Append(tablePtr->chain, tePtr);
    tePtr->hashPtr = Blt_CreateHashEntry(&tablePtr->entryTable,
	(char *)tkwin, &dummy);
    Blt_SetHashValue(tePtr->hashPtr, tePtr);

    Tk_CreateEventHandler(tkwin, StructureNotifyMask, WidgetEventProc, 
	tePtr);
    Tk_ManageGeometry(tkwin, &tableMgrInfo, tePtr);

    return tePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyEntry --
 *
 *	Removes the Entry structure from the hash table and frees the memory
 *	allocated by it.  If the table is still in use (i.e. was not called
 *	from DestoryTable), remove its entries from the lists of row and
 *	column sorted partitions.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the entry is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyEntry(TableEntry *tePtr)
{
    Table *tablePtr = tePtr->tablePtr;

    if (tePtr->row.link != NULL) {
	Blt_Chain_DeleteLink(tePtr->row.chain, tePtr->row.link);
    }
    if (tePtr->column.link != NULL) {
	Blt_Chain_DeleteLink(tePtr->column.chain,
	    tePtr->column.link);
    }
    if (tePtr->link != NULL) {
	Blt_Chain_DeleteLink(tablePtr->chain, tePtr->link);
    }
    if (tePtr->tkwin != NULL) {
	Tk_DeleteEventHandler(tePtr->tkwin, StructureNotifyMask, 
		WidgetEventProc, tePtr);
	Tk_ManageGeometry(tePtr->tkwin, (Tk_GeomMgr *)NULL, tePtr);
	if ((tablePtr->tkwin != NULL) && 
	    (Tk_Parent(tePtr->tkwin) != tablePtr->tkwin)) {
	    Tk_UnmaintainGeometry(tePtr->tkwin, tablePtr->tkwin);
	}
	if (Tk_IsMapped(tePtr->tkwin)) {
	    Tk_UnmapWindow(tePtr->tkwin);
	}
    }
    if (tePtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&tablePtr->entryTable, tePtr->hashPtr);
    }
    Blt_Free(tePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureEntry --
 *
 *	This procedure is called to process an objv/objc list, plus the Tk
 *	option database, in order to configure (or reconfigure) one or more
 *	entries.  Entries hold information about widgets managed by the table
 *	geometry manager.
 *
 * 	Note:	You can query only one widget at a time.  But several can be
 *		reconfigured at once.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is returned,
 *	then interp->result contains an error message.
 *
 * Side effects:
 *	The table layout is recomputed and rearranged at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureEntry(Table *tablePtr, Tcl_Interp *interp, TableEntry *tePtr,
	       int objc, Tcl_Obj *const *objv)
{
    int oldRowSpan, oldColSpan;

    if (tePtr->tablePtr != tablePtr) {
	Tcl_AppendResult(interp, "widget  \"", Tk_PathName(tePtr->tkwin),
	    "\" does not belong to table \"", Tk_PathName(tablePtr->tkwin),
	    "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (objc == 0) {
	return Blt_ConfigureInfoFromObj(interp, tePtr->tkwin, 
		entryConfigSpecs, (char *)tePtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 1) {
	return Blt_ConfigureInfoFromObj(interp, tePtr->tkwin, 
		entryConfigSpecs, (char *)tePtr, objv[0], 0);
    }
    oldRowSpan = tePtr->row.span;
    oldColSpan = tePtr->column.span;

    if (Blt_ConfigureWidgetFromObj(interp, tePtr->tkwin, entryConfigSpecs,
	    objc, objv, (char *)tePtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((tePtr->column.span < 1) || (tePtr->column.span > USHRT_MAX)) {
	Tcl_AppendResult(interp, "bad column span specified for \"",
	    Tk_PathName(tePtr->tkwin), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if ((tePtr->row.span < 1) || (tePtr->row.span > USHRT_MAX)) {
	Tcl_AppendResult(interp, "bad row span specified for \"",
	    Tk_PathName(tePtr->tkwin), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if ((oldColSpan != tePtr->column.span) ||
	(oldRowSpan != tePtr->row.span)) {
	BinEntry(tablePtr, tePtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PrintEntry --
 *
 *	Returns the name, position and options of a widget in the table.
 *
 * Results:
 *	Returns a standard TCL result.  A list of the widget attributes is
 *	left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
PrintEntry(TableEntry *tePtr, Tcl_DString *resultPtr)
{
    char string[200];

    Blt_FormatString(string, 200, "    %d,%d  ", tePtr->row.rcPtr->index,
	tePtr->column.rcPtr->index);
    Tcl_DStringAppend(resultPtr, string, -1);
    Tcl_DStringAppend(resultPtr, Tk_PathName(tePtr->tkwin), -1);
    if (tePtr->ixPad != ENTRY_DEF_PAD) {
	Tcl_DStringAppend(resultPtr, " -ipadx ", -1);
	Tcl_DStringAppend(resultPtr, Blt_Itoa(tePtr->ixPad), -1);
    }
    if (tePtr->iyPad != ENTRY_DEF_PAD) {
	Tcl_DStringAppend(resultPtr, " -ipady ", -1);
	Tcl_DStringAppend(resultPtr, Blt_Itoa(tePtr->iyPad), -1);
    }
    if (tePtr->row.span != ENTRY_DEF_SPAN) {
	Tcl_DStringAppend(resultPtr, " -rowspan ", -1);
	Tcl_DStringAppend(resultPtr, Blt_Itoa(tePtr->row.span), -1);
    }
    if (tePtr->column.span != ENTRY_DEF_SPAN) {
	Tcl_DStringAppend(resultPtr, " -columnspan ", -1);
	Tcl_DStringAppend(resultPtr, Blt_Itoa(tePtr->column.span), -1);
    }
    if (tePtr->anchor != ENTRY_DEF_ANCHOR) {
	Tcl_DStringAppend(resultPtr, " -anchor ", -1);
	Tcl_DStringAppend(resultPtr, Tk_NameOfAnchor(tePtr->anchor), -1);
    }
    if ((tePtr->padLeft != ENTRY_DEF_PAD) ||
	(tePtr->padRight != ENTRY_DEF_PAD)) {
	Tcl_DStringAppend(resultPtr, " -padx ", -1);
	Blt_FormatString(string, 200, "{%d %d}", tePtr->padLeft, tePtr->padRight);
	Tcl_DStringAppend(resultPtr, string, -1);
    }
    if ((tePtr->padTop != ENTRY_DEF_PAD) ||
	(tePtr->padBottom != ENTRY_DEF_PAD)) {
	Tcl_DStringAppend(resultPtr, " -pady ", -1);
	Blt_FormatString(string, 200, "{%d %d}", tePtr->padTop, tePtr->padBottom);
	Tcl_DStringAppend(resultPtr, string, -1);
    }
    if (tePtr->fill != ENTRY_DEF_FILL) {
	Tcl_DStringAppend(resultPtr, " -fill ", -1);
	Tcl_DStringAppend(resultPtr, Blt_NameOfFill(tePtr->fill), -1);
    }
    if (tePtr->column.control != ENTRY_DEF_CONTROL) {
	Tcl_DStringAppend(resultPtr, " -columncontrol ", -1);
	Tcl_DStringAppend(resultPtr, NameOfControl(tePtr->column.control), -1);
    }
    if (tePtr->row.control != ENTRY_DEF_CONTROL) {
	Tcl_DStringAppend(resultPtr, " -rowcontrol ", -1);
	Tcl_DStringAppend(resultPtr, NameOfControl(tePtr->row.control), -1);
    }
    if ((tePtr->reqWidth.nom != LIMITS_NOM) ||
	(tePtr->reqWidth.min != LIMITS_MIN) ||
	(tePtr->reqWidth.max != LIMITS_MAX)) {
	Tcl_DStringAppend(resultPtr, " -reqwidth {", -1);
	Tcl_DStringAppend(resultPtr, NameOfLimits(&tePtr->reqWidth), -1);
	Tcl_DStringAppend(resultPtr, "}", -1);
    }
    if ((tePtr->reqHeight.nom != LIMITS_NOM) ||
	(tePtr->reqHeight.min != LIMITS_MIN) ||
	(tePtr->reqHeight.max != LIMITS_MAX)) {
	Tcl_DStringAppend(resultPtr, " -reqheight {", -1);
	Tcl_DStringAppend(resultPtr, NameOfLimits(&tePtr->reqHeight), -1);
	Tcl_DStringAppend(resultPtr, "}", -1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * InfoEntry --
 *
 *	Returns the name, position and options of a widget in the table.
 *
 * Results:
 *	Returns a standard TCL result.  A list of the widget attributes is
 *	left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InfoEntry(Tcl_Interp *interp, Table *tablePtr, TableEntry *tePtr)
{
    Tcl_DString ds;

    if (tePtr->tablePtr != tablePtr) {
	Tcl_AppendResult(interp, "widget  \"", Tk_PathName(tePtr->tkwin),
	    "\" does not belong to table \"", Tk_PathName(tablePtr->tkwin),
	    "\"", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_DStringInit(&ds);
    PrintEntry(tePtr, &ds);
    Tcl_DStringResult(interp, &ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateRowColumn --
 *
 *	Creates and initializes a structure that manages the size of a row or
 *	column in the table. There will be one of these structures allocated
 *	for each row and column in the table, regardless if a widget is
 *	contained in it or not.
 *
 * Results:
 *	Returns a pointer to the newly allocated row or column structure.
 *
 *---------------------------------------------------------------------------
 */
static RowColumn *
CreateRowColumn(void)
{
    RowColumn *rcPtr;

    rcPtr = Blt_AssertMalloc(sizeof(RowColumn));
    rcPtr->resize = ROWCOL_DEF_RESIZE;
    ResetLimits(&rcPtr->reqSize);
    rcPtr->nom = LIMITS_NOM;
    rcPtr->pad.side1 = rcPtr->pad.side2 = ROWCOL_DEF_PAD;
    rcPtr->size = rcPtr->index = rcPtr->minSpan = 0;
    rcPtr->weight = ROWCOL_DEF_WEIGHT;
    return rcPtr;
}

static PartitionInfo *
ParseRowColumn2(Table *tablePtr, const char *string, int *numberPtr)
{
    char c;
    int n;
    PartitionInfo *piPtr;

    c = tolower(string[0]);
    if (c == 'c') {
	piPtr = &tablePtr->cols;
    } else if (c == 'r') {
	piPtr = &tablePtr->rows;
    } else {
	Tcl_AppendResult(tablePtr->interp, "bad index \"", string,
	    "\": must start with \"r\" or \"c\"", (char *)NULL);
	return NULL;
    }
    /* Handle row or column configuration queries */
    if (Tcl_GetInt(tablePtr->interp, string + 1, &n) != TCL_OK) {
	return NULL;
    }
    *numberPtr = (int)n;
    return piPtr;
}

static PartitionInfo *
ParseRowColumn(Table *tablePtr, Tcl_Obj *objPtr, int *numberPtr)
{
    int n;
    PartitionInfo *piPtr;
    const char *string;

    string = Tcl_GetString(objPtr);
    piPtr = ParseRowColumn2(tablePtr, string, &n);
    if (piPtr == NULL) {
	return NULL;
    }
    if ((n < 0) || (n >= Blt_Chain_GetLength(piPtr->chain))) {
	Tcl_AppendResult(tablePtr->interp, "bad ", piPtr->type, " index \"",
	    string, "\"", (char *)NULL);
	return NULL;
    }
    *numberPtr = (int)n;
    return piPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetRowColumn --
 *
 *	Gets the designated row or column from the table.  If the row or
 *	column index is greater than the size of the table, new rows/columns
 *	will be automatically allocated.
 *
 * Results:
 *	Returns a pointer to the row or column structure.
 *
 *---------------------------------------------------------------------------
 */
static RowColumn *
GetRowColumn(PartitionInfo *piPtr, int n)
{
    Blt_ChainLink link;
    int i;

    for (i = Blt_Chain_GetLength(piPtr->chain); i <= n; i++) {
	RowColumn *rcPtr;

	rcPtr = CreateRowColumn();
	rcPtr->index = i;
	rcPtr->link = Blt_Chain_Append(piPtr->chain, rcPtr);
    }
    link = Blt_Chain_GetNthLink(piPtr->chain, n);
    if (link == NULL) {
	return NULL;
    }
    return Blt_Chain_GetValue(link);
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteRowColumn --
 *
 *	Deletes a span of rows/columns from the table. The number of
 *	rows/columns to be deleted is given by span.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The size of the column partition array may be extended and
 *	initialized.
 *
 *---------------------------------------------------------------------------
 */
static void
DeleteRowColumn(
    Table *tablePtr,
    PartitionInfo *piPtr,
    RowColumn *rcPtr)
{
    /*
     * Remove any entries that start in the row/column to be deleted.  They
     * point to memory that will be freed.
     */
    if (piPtr->type == rowUid) {
	Blt_ChainLink link, next;

	for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
	    link = next) {
	    TableEntry *tePtr;

	    next = Blt_Chain_NextLink(link);
	    tePtr = Blt_Chain_GetValue(link);
	    if (tePtr->row.rcPtr->index == rcPtr->index) {
		DestroyEntry(tePtr);
	    }
	}
    } else {
	Blt_ChainLink link, next;

	for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
	    link = next) {
	    TableEntry *tePtr;

	    next = Blt_Chain_NextLink(link);
	    tePtr = Blt_Chain_GetValue(link);
	    if (tePtr->column.rcPtr->index == rcPtr->index) {
		DestroyEntry(tePtr);
	    }
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureRowColumn --
 *
 *	This procedure is called to process an objv/objc list in order to
 *	configure a row or column in the table geometry manager.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is returned,
 *	then interp->result holds an error message.
 *
 * Side effects:
 *	Partition configuration options (bounds, resize flags, etc) get set.
 *	New partitions may be created as necessary. The table is recalculated
 *	and arranged at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureRowColumn(Table *tablePtr, PartitionInfo *piPtr, const char *pattern,
		   int objc, Tcl_Obj *const *objv)
{
    Blt_ChainLink link;
    int numMatches;

    numMatches = 0;
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;
	char string[200];

	rcPtr = Blt_Chain_GetValue(link);
	Blt_FormatString(string, 200, "%c%d", pattern[0], rcPtr->index);
	if (Tcl_StringMatch(string, pattern)) {
	    if (objc == 0) {
		return Blt_ConfigureInfoFromObj(tablePtr->interp, 
			tablePtr->tkwin, piPtr->configSpecs, (char *)rcPtr, 
			(Tcl_Obj *)NULL, 0);
	    } else if (objc == 1) {
		return Blt_ConfigureInfoFromObj(tablePtr->interp, 
			tablePtr->tkwin, piPtr->configSpecs, (char *)rcPtr, 
			objv[0], 0);
	    } else {
		if (Blt_ConfigureWidgetFromObj(tablePtr->interp, 
			tablePtr->tkwin, piPtr->configSpecs, objc, objv, 
			(char *)rcPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    numMatches++;
	}
    }
    if (numMatches == 0) {
	int n;
	RowColumn *rcPtr;

	/* 
	 * We found no existing partitions matching this pattern, so see if
	 * this designates an new partition (one beyond the current range).
	 */
	if ((Tcl_GetInt(NULL, pattern + 1, &n) != TCL_OK) || (n < 0)) {
	    Tcl_AppendResult(tablePtr->interp, "pattern \"", pattern, 
		     "\" matches no ", piPtr->type, " in table \"", 
		     Tk_PathName(tablePtr->tkwin), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	rcPtr = GetRowColumn(piPtr, n);
	assert(rcPtr);
	if (Blt_ConfigureWidgetFromObj(tablePtr->interp, tablePtr->tkwin,
	       piPtr->configSpecs, objc, objv, (char *)rcPtr,
	       BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

static void
PrintRowColumn(
    Tcl_Interp *interp,
    PartitionInfo *piPtr,
    RowColumn *rcPtr,
    Tcl_DString *resultPtr)
{
    char string[200];
    const char *padFmt, *sizeFmt;

    if (piPtr->type == rowUid) {
	padFmt = " -pady {%d %d}";
	sizeFmt = " -height {%s}";
    } else {
	padFmt = " -padx {%d %d}";
	sizeFmt = " -width {%s}";
    }
    if (rcPtr->resize != ROWCOL_DEF_RESIZE) {
	Tcl_DStringAppend(resultPtr, " -resize ", -1);
	Tcl_DStringAppend(resultPtr, Blt_NameOfResize(rcPtr->resize), -1);
    }
    if ((rcPtr->pad.side1 != ROWCOL_DEF_PAD) ||
	(rcPtr->pad.side2 != ROWCOL_DEF_PAD)) {
	Blt_FormatString(string, 200, padFmt, rcPtr->pad.side1, rcPtr->pad.side2);
	Tcl_DStringAppend(resultPtr, string, -1);
    }
    if (rcPtr->weight != ROWCOL_DEF_WEIGHT) {
	Tcl_DStringAppend(resultPtr, " -weight ", -1);
	Tcl_DStringAppend(resultPtr, Blt_Dtoa(interp, rcPtr->weight), -1);
    }
    if ((rcPtr->reqSize.min != LIMITS_MIN) ||
	(rcPtr->reqSize.nom != LIMITS_NOM) ||
	(rcPtr->reqSize.max != LIMITS_MAX)) {
	Blt_FormatString(string, 200, sizeFmt, NameOfLimits(&rcPtr->reqSize));
	Tcl_DStringAppend(resultPtr, string, -1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * InfoRowColumn --
 *
 *	Returns the options of a partition in the table.
 *
 * Results:
 *	Returns a standard TCL result.  A list of the partition
 *	attributes is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InfoRowColumn(Table *tablePtr, Tcl_Interp *interp, const char *pattern)
{
    PartitionInfo *piPtr;
    char c;
    Blt_ChainLink link, last;
    Tcl_DString ds;

    c = pattern[0];
    if ((c == 'r') || (c == 'R')) {
	piPtr = &tablePtr->rows;
    } else {
	piPtr = &tablePtr->cols;
    }
    Tcl_DStringInit(&ds);
    last = Blt_Chain_LastLink(piPtr->chain);
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;
	char string[200];

	rcPtr = Blt_Chain_GetValue(link);
	Blt_FormatString(string, 200, "%c%d", piPtr->type[0], rcPtr->index);
	if (Tcl_StringMatch(string, pattern)) {
	    Tcl_DStringAppend(&ds, string, -1);
	    PrintRowColumn(interp, piPtr, rcPtr, &ds);
	    if (link != last) {
		Tcl_DStringAppend(&ds, " \\\n", -1);
	    } else {
		Tcl_DStringAppend(&ds, "\n", -1);
	    }
	}
    }
    Tcl_DStringResult(interp, &ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitSpan --
 *
 *	Checks the size of the column partitions and extends the size if a
 *	larger array is needed.
 *
 * Results:
 *	Always return a RowColumn pointer.
 *
 * Side effects:
 *	The size of the column partition array may be extended and
 *	initialized.
 *
 *---------------------------------------------------------------------------
 */
static RowColumn *
InitSpan(PartitionInfo *piPtr, int start, int span)
{
    int length;
    int i;
    Blt_ChainLink link;

    length = Blt_Chain_GetLength(piPtr->chain);
    for (i = length; i < (start + span); i++) {
	RowColumn *rcPtr;

	rcPtr = CreateRowColumn();
	rcPtr->index = i;
	rcPtr->link = Blt_Chain_Append(piPtr->chain, rcPtr);
    }
    link = Blt_Chain_GetNthLink(piPtr->chain, start);
    return Blt_Chain_GetValue(link);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetTableFromObj --
 *
 *	Searches for a table associated by the path name of the widget
 *	container.
 *
 *	Errors may occur because
 *	  1) pathName isn't a valid for any Tk widget, or
 *	  2) there's no table associated with that widget as a container.
 *
 * Results:
 *	If a table entry exists, a pointer to the Table structure is
 *	returned. Otherwise NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
/*LINTLIBRARY*/
int
Blt_GetTableFromObj(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,		/* Interpreter to report errors back to. */
    Tcl_Obj *objPtr,		/* Path name of the container widget. */
    Table **tablePtrPtr)
{
    Blt_HashEntry *hPtr;
    Tk_Window tkwin;
    const char *pathName;

    pathName = Tcl_GetString(objPtr);
    tkwin = Tk_NameToWindow(interp, pathName, dataPtr->tkMain);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    hPtr = Blt_FindHashEntry(&dataPtr->tableTable, (char *)tkwin);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "no table associated with widget \"",
		pathName, "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    *tablePtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateTable --
 *
 *	This procedure creates and initializes a new Table structure with
 *	tkwin as its container widget. The internal structures associated with
 *	the table are initialized.
 *
 * Results:
 *	Returns the pointer to the new Table structure describing the new
 *	table geometry manager.  If an error occurred, the return value will
 *	be NULL and an error message is left in interp->result.
 *
 * Side effects:
 *	Memory is allocated and initialized, an event handler is set up to
 *	watch tkwin, etc.
 *
 *---------------------------------------------------------------------------
 */
static Table *
CreateTable(
    TableInterpData *dataPtr,
    Tcl_Interp *interp,		/* Interpreter associated with table. */
    const char *pathName)	/* Path name of the container widget to be
				 * associated with the new table. */
{
    Table *tablePtr;
    Tk_Window tkwin;
    int dummy;
    Blt_HashEntry *hPtr;

    tkwin = Tk_NameToWindow(interp, pathName, dataPtr->tkMain);
    if (tkwin == NULL) {
	return NULL;
    }
    tablePtr = Blt_AssertCalloc(1, sizeof(Table));
    tablePtr->tkwin = tkwin;
    tablePtr->interp = interp;
    tablePtr->rows.type = rowUid;
    tablePtr->rows.configSpecs = rowConfigSpecs;
    tablePtr->rows.chain = Blt_Chain_Create();
    tablePtr->cols.type = columnUid;
    tablePtr->cols.configSpecs = columnConfigSpecs;
    tablePtr->cols.chain = Blt_Chain_Create();
    tablePtr->propagate = TRUE;

    tablePtr->arrangeProc = ArrangeTable;
    Blt_InitHashTable(&tablePtr->entryTable, BLT_ONE_WORD_KEYS);
    tablePtr->findEntryProc = FindEntry;

    ResetLimits(&tablePtr->reqWidth);
    ResetLimits(&tablePtr->reqHeight);

    tablePtr->chain = Blt_Chain_Create();
    tablePtr->rows.list = Blt_List_Create(BLT_ONE_WORD_KEYS);
    tablePtr->cols.list = Blt_List_Create(BLT_ONE_WORD_KEYS);

    Tk_CreateEventHandler(tablePtr->tkwin, StructureNotifyMask,
	TableEventProc, tablePtr);
    hPtr = Blt_CreateHashEntry(&dataPtr->tableTable, (char *)tkwin, &dummy);
    tablePtr->hashPtr = hPtr;
    tablePtr->tablePtr = &dataPtr->tableTable;
    Blt_SetHashValue(hPtr, tablePtr);
    return tablePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureTable --
 *
 *	This procedure is called to process an objv/objc list in order to
 *	configure the table geometry manager.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is returned,
 *	then interp->result contains an error message.
 *
 * Side effects:
 *	Table configuration options (-padx, -pady, etc.) get set.  The table
 *	is recalculated and arranged at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureTable(
    Table *tablePtr,		/* Table to be configured */
    Tcl_Interp *interp,		/* Interpreter to report results back to */
    int objc,
    Tcl_Obj *const *objv)	/* Option-value pairs */
{
    if (objc == 0) {
	return Blt_ConfigureInfoFromObj(interp, tablePtr->tkwin, 
		tableConfigSpecs, (char *)tablePtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 1) {
	return Blt_ConfigureInfoFromObj(interp, tablePtr->tkwin, 
		tableConfigSpecs, (char *)tablePtr, objv[0], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, tablePtr->tkwin, tableConfigSpecs,
	    objc, objv, (char *)tablePtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Arrange for the table layout to be computed at the next idle point. */
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

static void
PrintTable(
    Table *tablePtr,
    Tcl_DString *resultPtr)
{
    char string[200];

    if ((tablePtr->padLeft != TABLE_DEF_PAD) ||
	(tablePtr->padRight != TABLE_DEF_PAD)) {
	Blt_FormatString(string, 200, " -padx {%d %d}", tablePtr->padLeft, 
		tablePtr->padRight);
	Tcl_DStringAppend(resultPtr, string, -1);
    }
    if ((tablePtr->padTop != TABLE_DEF_PAD) ||
	(tablePtr->padBottom != TABLE_DEF_PAD)) {
	Blt_FormatString(string, 200, " -pady {%d %d}", tablePtr->padTop, 
		tablePtr->padBottom);
	Tcl_DStringAppend(resultPtr, string, -1);
    }
    if (!tablePtr->propagate) {
	Tcl_DStringAppend(resultPtr, " -propagate no", -1);
    }
    if ((tablePtr->reqWidth.min != LIMITS_MIN) ||
	(tablePtr->reqWidth.nom != LIMITS_NOM) ||
	(tablePtr->reqWidth.max != LIMITS_MAX)) {
	Tcl_DStringAppend(resultPtr, " -reqwidth {%s}", -1);
	Tcl_DStringAppend(resultPtr, NameOfLimits(&tablePtr->reqWidth), -1);
    }
    if ((tablePtr->reqHeight.min != LIMITS_MIN) ||
	(tablePtr->reqHeight.nom != LIMITS_NOM) ||
	(tablePtr->reqHeight.max != LIMITS_MAX)) {
	Tcl_DStringAppend(resultPtr, " -reqheight {%s}", -1);
	Tcl_DStringAppend(resultPtr, NameOfLimits(&tablePtr->reqHeight), -1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyPartitions --
 *
 *	Clear each of the lists managing the entries.  The entries in the
 *	lists of row and column spans are themselves lists which need to be
 *	cleared.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyPartitions(PartitionInfo *piPtr)
{
    if (piPtr->list != NULL) {
	Blt_ListNode node;

	for (node = Blt_List_FirstNode(piPtr->list); node != NULL;
	    node = Blt_List_NextNode(node)) {
	    Blt_Chain chain;

	    chain = Blt_List_GetValue(node);
	    if (chain != NULL) {
		Blt_Chain_Destroy(chain);
	    }
	}
	Blt_List_Destroy(piPtr->list);
    }
    if (piPtr->chain != NULL) {
	Blt_ChainLink link;

	for (link = Blt_Chain_FirstLink(piPtr->chain);
	    link != NULL; link = Blt_Chain_NextLink(link)) {
	    RowColumn *rcPtr;

	    rcPtr = Blt_Chain_GetValue(link);
	    Blt_Free(rcPtr);
	}
	Blt_Chain_Destroy(piPtr->chain);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyTable --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *	clean up the Table structure at a safe time (when no-one is using it
 *	anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the table geometry manager is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyTable(DestroyData dataPtr) /* Table structure */
{
    Blt_ChainLink link, next;
    Table *tablePtr = (Table *)dataPtr;

    /* Release the chain of entries. */
    for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL; 
	 link = next) {
	TableEntry *tePtr;

	next = Blt_Chain_NextLink(link);
	tePtr = Blt_Chain_GetValue(link);
	DestroyEntry(tePtr);
    }
    Blt_Chain_Destroy(tablePtr->chain);

    DestroyPartitions(&tablePtr->rows);
    DestroyPartitions(&tablePtr->cols);
    Blt_DeleteHashTable(&tablePtr->entryTable);
    if (tablePtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(tablePtr->tablePtr, tablePtr->hashPtr);
    }
    Blt_Free(tablePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * BinEntry --
 *
 *	Adds the entry to the lists of both row and column spans.  The layout
 *	of the table is done in order of partition spans, from shorted to
 *	longest.  The widgets spanning a particular number of partitions are
 *	stored in a linked list.  Each list is in turn, contained within a
 *	master list.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The entry is added to both the lists of row and columns spans.  This
 *	will effect the layout of the widgets.
 *
 *---------------------------------------------------------------------------
 */
static void
BinEntry(Table *tablePtr, TableEntry *tePtr)
{
    Blt_ListNode node;
    Blt_List list;
    Blt_Chain chain;
    long key;

    /*
     * Remove the entry from both row and column lists.  It will be
     * re-inserted into the table at the new position.
     */
    if (tePtr->column.link != NULL) {
	Blt_Chain_UnlinkLink(tePtr->column.chain,
	    tePtr->column.link);
    }
    if (tePtr->row.link != NULL) {
	Blt_Chain_UnlinkLink(tePtr->row.chain, tePtr->row.link);
    }
    list = tablePtr->rows.list;
    key = 0;			/* Initialize key to bogus span */
    for (node = Blt_List_FirstNode(list); node != NULL;
	node = Blt_List_NextNode(node)) {

	key = (long)Blt_List_GetKey(node);
	if (tePtr->row.span <= key) {
	    break;
	}
    }
    if (key != tePtr->row.span) {
	Blt_ListNode newNode;

	/*
	 * Create a new list (bucket) to hold entries of that size span and
	 * and link it into the list of buckets.
	 */
	newNode = Blt_List_CreateNode(list, (char *)tePtr->row.span);
	Blt_List_SetValue(newNode, (char *)Blt_Chain_Create());
	Blt_List_LinkBefore(list, newNode, node);
	node = newNode;
    }
    chain = Blt_List_GetValue(node);
    if (tePtr->row.link == NULL) {
	tePtr->row.link = Blt_Chain_Append(chain, tePtr);
    } else {
	Blt_Chain_LinkAfter(chain, tePtr->row.link, NULL);
    }
    tePtr->row.chain = chain;

    list = tablePtr->cols.list;
    key = 0;
    for (node = Blt_List_FirstNode(list); node != NULL;
	node = Blt_List_NextNode(node)) {
	key = (long)Blt_List_GetKey(node);
	if (tePtr->column.span <= key) {
	    break;
	}
    }
    if (key != tePtr->column.span) {
	Blt_ListNode newNode;

	/*
	 * Create a new list (bucket) to hold entries of that size span and
	 * and link it into the list of buckets.
	 */
	newNode = Blt_List_CreateNode(list, (char *)tePtr->column.span);
	Blt_List_SetValue(newNode, (char *)Blt_Chain_Create());
	Blt_List_LinkBefore(list, newNode, node);
	node = newNode;
    }
    chain = Blt_List_GetValue(node);

    /* Add the new entry to the span bucket */
    if (tePtr->column.link == NULL) {
	tePtr->column.link = Blt_Chain_Append(chain, tePtr);
    } else {
	Blt_Chain_LinkAfter(chain, tePtr->column.link, NULL);
    }
    tePtr->column.chain = chain;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseIndex --
 *
 *	Parse the entry index string and return the row and column numbers in
 *	their respective parameters.  The format of a table entry index is
 *	row,column where row is the row number and column is the column
 *	number.  Rows and columns are numbered starting from zero.
 *
 * Results:
 *	Returns a standard TCL result.  If TCL_OK is returned, the row and
 *	column numbers are returned via rowPtr and colPtr respectively.
 *
 *---------------------------------------------------------------------------
 */
static int
ParseIndex(Tcl_Interp *interp, const char *string, int *rowPtr, int *colPtr)
{
    char *comma;
    long row, column;
    int result;

    comma = (char *)strchr(string, ',');
    if (comma == NULL) {
	Tcl_AppendResult(interp, "bad index \"", string,
	    "\": should be \"row,column\"", (char *)NULL);
	return TCL_ERROR;

    }
    *comma = '\0';
    result = ((Tcl_ExprLong(interp, string, &row) != TCL_OK) ||
	(Tcl_ExprLong(interp, comma + 1, &column) != TCL_OK));
    *comma = ',';		/* Repair the argument */
    if (result) {
	return TCL_ERROR;
    }
    if ((row < 0) || (row > (long)USHRT_MAX)) {
	Tcl_AppendResult(interp, "bad index \"", string,
	    "\": row is out of range", (char *)NULL);
	return TCL_ERROR;

    }
    if ((column < 0) || (column > (long)USHRT_MAX)) {
	Tcl_AppendResult(interp, "bad index \"", string,
	    "\": column is out of range", (char *)NULL);
	return TCL_ERROR;
    }
    *rowPtr = (int)row;
    *colPtr = (int)column;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ManageEntry --
 *
 *	Inserts the given widget into the table at a given row and column
 *	position.  The widget can already be managed by this or another table.
 *	The widget will be simply moved to the new location in this table.
 *
 *	The new widget is inserted into both a hash table (this is used to
 *	locate the information associated with the widget) and a list (used to
 *	indicate relative ordering of widgets).
 *
 * Results:
 *	Returns a standard TCL result.  If an error occurred, TCL_ERROR is
 *	returned and an error message is left in interp->result.
 *
 * Side Effects:
 *	The table is re-computed and arranged at the next idle point.
 *
 * ---------------------------------------------------------------------------- */
static int
ManageEntry(
    Tcl_Interp *interp,
    Table *tablePtr,
    Tk_Window tkwin,
    int row, int column,
    int objc,
    Tcl_Obj *const *objv)
{
    TableEntry *tePtr;
    int result = TCL_OK;

    tePtr = FindEntry(tablePtr, tkwin);
    if ((tePtr != NULL) && (tePtr->tablePtr != tablePtr)) {
	/* The entry for the widget already exists. If it's managed by another
	 * table, delete it.  */
	DestroyEntry(tePtr);
	tePtr = NULL;
    }
    if (tePtr == NULL) {
	tePtr = CreateEntry(tablePtr, tkwin);
	if (tePtr == NULL) {
	    return TCL_ERROR;
	}
    }
    if (objc > 0) {
	result = Blt_ConfigureWidgetFromObj(tablePtr->interp, tePtr->tkwin,
	    entryConfigSpecs, objc, objv, (char *)tePtr,
	    BLT_CONFIG_OBJV_ONLY);
    }
    if ((tePtr->column.span < 1) || (tePtr->row.span < 1)) {
	Tcl_AppendResult(tablePtr->interp, "bad span specified for \"",
	    Tk_PathName(tkwin), "\"", (char *)NULL);
	DestroyEntry(tePtr);
	return TCL_ERROR;
    }
    tePtr->column.rcPtr = InitSpan(&tablePtr->cols, column, tePtr->column.span);
    tePtr->row.rcPtr = InitSpan(&tablePtr->rows, row, tePtr->row.span);
    /* Insert the entry into both the row and column layout lists */
    BinEntry(tablePtr, tePtr);

    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * BuildTable --
 *
 *	Processes an objv/objc list of table entries to add and configure new
 *	widgets into the table.  A table entry consists of the widget path
 *	name, table index, and optional configuration options.  The first
 *	argument in the objv list is the name of the table.  If no table
 *	exists for the given widget, a new one is created.
 *
 * Results:
 *	Returns a standard TCL result.  If an error occurred,
 *	TCL_ERROR is returned and an error message is left in
 *	interp->result.
 *
 * Side Effects:
 *	Memory is allocated, a new table is possibly created, etc.
 *	The table is re-computed and arranged at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static int
BuildTable(
    Table *tablePtr,		/* Table to manage new widgets */
    Tcl_Interp *interp,		/* Interpreter to report errors back to */
    int objc,			/*  */
    Tcl_Obj *const *objv)	/* List of widgets, indices, and options */
{
    Tk_Window tkwin;
    int row, column;
    int nextRow, nextColumn;
    int i;

    /* Process any options specific to the table */
    for (i = 2; i < objc; i += 2) {
	const char *string;

	string = Tcl_GetString(objv[i]);
	if (string[0] != '-') {
	    break;
	}
    }
    if (i > objc) {
	i = objc;
    }
    if (i > 2) {
	if (ConfigureTable(tablePtr, interp, i - 2, objv + 2) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    nextRow = tablePtr->numRows;
    nextColumn = 0;
    objc -= i, objv += i;
    while (objc > 0) {
	const char *string;
	/*
	 * Allow the name of the widget and row/column index to be specified
	 * in any order.
	 */
	string = Tcl_GetString(objv[0]);
	if (string[0] == '.') {
	    tkwin = Tk_NameToWindow(interp, string, tablePtr->tkwin);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    if (objc == 1) {
		/* No row,column index, use defaults instead */
		row = nextRow, column = nextColumn;
		objc--, objv++;
	    } else {
		string = Tcl_GetString(objv[1]);
		if (string[0] == '-') {
		    /* No row,column index, use defaults instead */
		    row = nextRow, column = nextColumn;
		    objc--, objv++;
		} else {
		    if (ParseIndex(interp, string, &row, &column) != TCL_OK) {
			return TCL_ERROR;	/* Invalid row,column index */
		    }
		    /* Skip over the widget pathname and table index. */
		    objc -= 2, objv += 2;
		}
	    }
	} else {
	    if (ParseIndex(interp, Tcl_GetString(objv[0]), &row, &column) 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    if (objc == 1) {
		Tcl_AppendResult(interp, "missing widget pathname after \"",
			 Tcl_GetString(objv[0]), "\"", (char *)NULL);
		return TCL_ERROR;
	    }
	    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[1]), 
			    tablePtr->tkwin);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    /* Skip over the widget pathname and table index. */
	    objc -= 2, objv += 2;
	}

	/* Find the end of the widget's option-value pairs */
	for (i = 0; i < objc; i += 2) {
	    string = Tcl_GetString(objv[i]);
	    if (string[0] != '-') {
		break;
	    }
	}
	if (i > objc) {
	    i = objc;
	}
	if (ManageEntry(interp, tablePtr, tkwin, row,
		column, i, objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	nextColumn = column + 1;
	objc -= i, objv += i;
    }
    /* Arrange for the new table layout to be calculated. */
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);

    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(tablePtr->tkwin),-1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseItem --
 *
 *	Parses a string representing an item in the table.  An item may be one
 *	of the following:
 *		Rn	- Row index, where n is the index of row
 *		Cn	- Column index, where n is the index of column
 *		r,c	- Cell index, where r is the row index and c
 *			  is the column index.
 *
 * Results:
 *	Returns a standard TCL result.  If no error occurred, TCL_OK is
 *	returned.  *RowPtr* will return the row index.  *ColumnPtr* will
 *	return the column index.  If the row or column index is not
 *	applicable, -1 is returned via *rowPtr* or *colPtr*.
 *
 *---------------------------------------------------------------------------
 */
static int
ParseItem(Table *tablePtr, const char *string, int *rowPtr, int *colPtr)
{
    char c;
    long partNum;

    c = tolower(string[0]);
    *rowPtr = *colPtr = -1;
    if (c == 'r') {
	if (Tcl_ExprLong(tablePtr->interp, string + 1, &partNum) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((partNum < 0) || (partNum >= tablePtr->numRows)) {
	    Tcl_AppendResult(tablePtr->interp, "row index \"", string,
		"\" is out of range", (char *)NULL);
	    return TCL_ERROR;
	}
	*rowPtr = (int)partNum;
    } else if (c == 'c') {
	if (Tcl_ExprLong(tablePtr->interp, string + 1, &partNum) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((partNum < 0) || (partNum >= tablePtr->numColumns)) {
	    Tcl_AppendResult(tablePtr->interp, "column index \"", string,
		"\" is out of range", (char *)NULL);
	    return TCL_ERROR;
	}
	*colPtr = (int)partNum;
    } else {
	if (ParseIndex(tablePtr->interp, string, rowPtr, colPtr) != TCL_OK) {
	    return TCL_ERROR;	/* Invalid row,column index */
	}
	if ((*rowPtr < 0) || (*rowPtr >= tablePtr->numRows) ||
	    (*colPtr < 0) || (*colPtr >= tablePtr->numColumns)) {
	    Tcl_AppendResult(tablePtr->interp, "index \"", string,
		"\" is out of range", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TranslateAnchor --
 *
 * 	Translate the coordinates of a given bounding box based upon the
 * 	anchor specified.  The anchor indicates where the given xy position is
 * 	in relation to the bounding box.
 *
 *  		nw --- n --- ne
 *  		|            |     x,y ---+
 *  		w   center   e      |     |
 *  		|            |      +-----+
 *  		sw --- s --- se
 *
 * Results:
 *	The translated coordinates of the bounding box are returned.
 *
 *---------------------------------------------------------------------------
 */
static void
TranslateAnchor(
    int dx, int dy,		/* Difference between outer and inner
				 * regions */
    Tk_Anchor anchor,		/* Direction of the anchor */
    int *xPtr, int *yPtr)
{
    int x, y;

    x = y = 0;
    switch (anchor) {
    case TK_ANCHOR_NW:		/* Upper left corner */
	break;
    case TK_ANCHOR_W:		/* Left center */
	y = (dy / 2);
	break;
    case TK_ANCHOR_SW:		/* Lower left corner */
	y = dy;
	break;
    case TK_ANCHOR_N:		/* Top center */
	x = (dx / 2);
	break;
    case TK_ANCHOR_CENTER:	/* Centered */
	x = (dx / 2);
	y = (dy / 2);
	break;
    case TK_ANCHOR_S:		/* Bottom center */
	x = (dx / 2);
	y = dy;
	break;
    case TK_ANCHOR_NE:		/* Upper right corner */
	x = dx;
	break;
    case TK_ANCHOR_E:		/* Right center */
	x = dx;
	y = (dy / 2);
	break;
    case TK_ANCHOR_SE:		/* Lower right corner */
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
 * GetReqWidth --
 *
 *	Returns the width requested by the widget starting in the given entry.
 *	The requested space also includes any internal padding which has been
 *	designated for this widget.
 *
 *	The requested width of the widget is always bounded by the limits set
 *	in tePtr->reqWidth.
 *
 * Results:
 *	Returns the requested width of the widget.
 *
 *---------------------------------------------------------------------------
 */
static int
GetReqWidth(TableEntry *tePtr)
{
    int width;

    width = Tk_ReqWidth(tePtr->tkwin) + (2 * tePtr->ixPad);
    width = GetBoundedWidth(width, &tePtr->reqWidth);
    return width;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetReqHeight --
 *
 *	Returns the height requested by the widget starting in the given
 *	entry.  The requested space also includes any internal padding which
 *	has been designated for this widget.
 *
 *	The requested height of the widget is always bounded by the limits set
 *	in tePtr->reqHeight.
 *
 * Results:
 *	Returns the requested height of the widget.
 *
 *---------------------------------------------------------------------------
 */
static int
GetReqHeight(TableEntry *tePtr)
{
    int height;

    height = Tk_ReqHeight(tePtr->tkwin) + (2 * tePtr->iyPad);
    height = GetBoundedHeight(height, &tePtr->reqHeight);
    return height;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTotalSpan --
 *
 *	Sums the row/column space requirements for the entire table.
 *
 * Results:
 *	Returns the space currently used in the span of partitions.
 *
 *---------------------------------------------------------------------------
 */
static int
GetTotalSpan(PartitionInfo *piPtr)
{
    int spaceUsed;
    Blt_ChainLink link;

    spaceUsed = 0;
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;		/* Start of partitions */

	rcPtr = Blt_Chain_GetValue(link);
	spaceUsed += rcPtr->size;
    }
    return spaceUsed;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetSpan --
 *
 *	Determines the space used by rows/columns for an entry.
 *
 * Results:
 *	Returns the space currently used in the span of partitions.
 *
 *---------------------------------------------------------------------------
 */
static int
GetSpan(PartitionInfo *piPtr, TableEntry *tePtr)
{
    RowColumn *startPtr;
    int spaceUsed;
    int count;
    Blt_ChainLink link;
    RowColumn *rcPtr;		/* Start of partitions */
    int span;			/* Number of partitions spanned */

    if (piPtr->type == rowUid) {
	rcPtr = tePtr->row.rcPtr;
	span = tePtr->row.span;
    } else {
	rcPtr = tePtr->column.rcPtr;
	span = tePtr->column.span;
    }

    count = spaceUsed = 0;
    link = rcPtr->link;
    startPtr = Blt_Chain_GetValue(link);
    for ( /*empty*/ ; (link != NULL) && (count < span);
	link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	spaceUsed += rcPtr->size;
	count++;
    }
    /*
     * Subtract off the padding on either side of the span, since the
     * widget can't grow into it.
     */
    rcPtr->pad.side2 = 0;
    spaceUsed -= (startPtr->pad.side1 + rcPtr->pad.side2 + piPtr->ePad);
    return spaceUsed;
}

/*
 *---------------------------------------------------------------------------
 *
 * GrowSpan --
 *
 *	Expand the span by the amount of the extra space needed.  This
 *	procedure is used in LayoutPartitions to grow the partitions to their
 *	minimum nominal size, starting from a zero width and height space.
 *
 *	This looks more complicated than it really is.  The idea is to make
 *	the size of the partitions correspond to the smallest entry spans.
 *	For example, if widget A is in column 1 and widget B spans both
 *	columns 0 and 1, any extra space needed to fit widget B should come
 *	from column 0.
 *
 *	On the first pass we try to add space to partitions which have not
 *	been touched yet (i.e. have no nominal size).  Since the row and
 *	column lists are sorted in ascending order of the number of rows or
 *	columns spanned, the space is distributed amongst the smallest spans
 *	first.
 *
 *	The second pass handles the case of widgets which have the same span.
 *	For example, if A and B, which span the same number of partitions are
 *	the only widgets to span column 1, column 1 would grow to contain the
 *	bigger of the two slices of space.
 *
 *	If there is still extra space after the first two passes, this means
 *	that there were no partitions of with no widget spans or the same
 *	order span that could be expanded. The third pass will try to remedy
 *	this by parcelling out the left over space evenly among the rest of
 *	the partitions.
 *
 *	On each pass, we have to keep iterating over the span, evenly doling
 *	out slices of extra space, because we may hit partition limits as
 *	space is donated.  In addition, if there are left over pixels because
 *	of round-off, this will distribute them as evenly as possible.  For
 *	the worst case, it will take *span* passes to expand the span.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 * 	The partitions in the span may be expanded.
 *
 *---------------------------------------------------------------------------
 */
static void
GrowSpan(
    Table *tablePtr, 
    PartitionInfo *piPtr,
    TableEntry *tePtr,
    int growth)			/* The amount of extra space needed to grow
				 * the span. */
{
    Blt_ChainLink link;
    int numOpen;			/* # of partitions with space available */
    int n;
    RowColumn *startPtr;	/* Starting (column/row) partition  */
    int span;			/* Number of partitions in the span */

    if (piPtr->type == rowUid) {
	startPtr = tePtr->row.rcPtr;
	span = tePtr->row.span;
    } else {
	startPtr = tePtr->column.rcPtr;
	span = tePtr->column.span;
    }

    /*
     * Pass 1: First add space to rows/columns that haven't determined
     *	       their nominal sizes yet.
     */

    numOpen = 0;
    /* Find out how many partitions have no size yet */

    for (n = 0, link = startPtr->link; (link != NULL) && (n < span); 
	 n++, link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;

	rcPtr = Blt_Chain_GetValue(link);
	if ((rcPtr->nom == LIMITS_NOM) && (rcPtr->max > rcPtr->size)) {
	    numOpen++;
	}
    }

    while ((numOpen > 0) && (growth > 0)) {
	int ration;

	ration = growth / numOpen;
	if (ration == 0) {
	    ration = 1;
	}
	link = startPtr->link;
	for (n = 0; (n < span) && (growth > 0); n++) {
	    RowColumn *rcPtr;
	    int avail;

	    rcPtr = Blt_Chain_GetValue(link);
	    avail = rcPtr->max - rcPtr->size;
	    if ((rcPtr->nom == LIMITS_NOM) && (avail > 0)) {
		if (ration < avail) {
		    growth -= ration;
		    rcPtr->size += ration;
		} else {
		    growth -= avail;
		    rcPtr->size += avail;
		    numOpen--;
		}
		rcPtr->minSpan = span;
		rcPtr->control = tePtr;
	    }
	    link = Blt_Chain_NextLink(link);
	}
    }

    /*
     * Pass 2: Add space to partitions which have the same minimum span
     */

    numOpen = 0;
    link = startPtr->link;
    for (n = 0; n < span; n++) {
	RowColumn *rcPtr;

	rcPtr = Blt_Chain_GetValue(link);
	if ((rcPtr->minSpan == span) && (rcPtr->max > rcPtr->size)) {
	    numOpen++;
	}
	link = Blt_Chain_NextLink(link);
    }
    while ((numOpen > 0) && (growth > 0)) {
	int ration;

	ration = growth / numOpen;
	if (ration == 0) {
	    ration = 1;
	}
	link = startPtr->link;
	for (n = 0; (n < span) && (growth > 0); n++) {
	    RowColumn *rcPtr;
	    int avail;

	    rcPtr = Blt_Chain_GetValue(link);
	    avail = rcPtr->max - rcPtr->size;
	    if ((rcPtr->minSpan == span) && (avail > 0)) {
		if (ration < avail) {
		    growth -= ration;
		    rcPtr->size += ration;
		} else {
		    growth -= avail;
		    rcPtr->size += avail;
		    numOpen--;
		}
		rcPtr->control = tePtr;
	    }
	    link = Blt_Chain_NextLink(link);
	}
    }

    /*
     * Pass 3: Try to expand all the partitions with space still available
     */

    /* Find out how many partitions still have space available */
    numOpen = 0;
    link = startPtr->link;
    for (n = 0; n < span; n++) {
	RowColumn *rcPtr;

	rcPtr = Blt_Chain_GetValue(link);
	if ((rcPtr->resize & RESIZE_EXPAND) && (rcPtr->max > rcPtr->size)) {
	    numOpen++;
	}
	/* Set the nominal size of the row/column. */
	rcPtr->nom = rcPtr->size;
	link = Blt_Chain_NextLink(link);
    }
    while ((numOpen > 0) && (growth > 0)) {
	int ration;

	ration = growth / numOpen;
	if (ration == 0) {
	    ration = 1;
	}
	link = startPtr->link;
	for (n = 0; (n < span) && (growth > 0); n++) {
	    RowColumn *rcPtr;
	    int avail;

	    rcPtr = Blt_Chain_GetValue(link);
	    link = Blt_Chain_NextLink(link);
	    if (!(rcPtr->resize & RESIZE_EXPAND)) {
		continue;
	    }
	    avail = rcPtr->max - rcPtr->size;
	    if (avail > 0) {
		if (ration < avail) {
		    growth -= ration;
		    rcPtr->size += ration;
		} else {
		    growth -= avail;
		    rcPtr->size += avail;
		    numOpen--;
		}
		rcPtr->nom = rcPtr->size;
		rcPtr->control = tePtr;
	    }
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GrowPartitions --
 *
 *	Grow the span by the designated amount.  Size constraints on the
 *	partitions may prevent any or all of the spacing adjustments.
 *
 *	This is very much like the GrowSpan procedure, but in this case we are
 *	expanding all the (row or column) partitions. It uses a two pass
 *	approach, first giving space to partitions which are smaller than
 *	their nominal sizes. This is because constraints on the partitions may
 *	cause resizing to be non-linear.
 *
 *	If there is still extra space, this means that all partitions are at
 *	least to their nominal sizes.  The second pass will try to add the
 *	left over space evenly among all the partitions which still have space
 *	available (i.e. haven't reached their specified max sizes).
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The size of the partitions may be increased.
 *
 *---------------------------------------------------------------------------
 */
static void
GrowPartitions(
    PartitionInfo *piPtr,	/* Array of (column/row) partitions  */
    int adjustment)		/* The amount of extra space to grow the
				 * span. If negative, it represents the amount
				 * of space to add. */
{
    int delta;			/* Amount of space needed */
    int numAdjust;		/* Number of rows/columns that still can be
				 * adjusted. */
    Blt_Chain chain;
    Blt_ChainLink link;
    float totalWeight;

    chain = piPtr->chain;

    /*
     * Pass 1: First adjust the size of rows/columns that still haven't
     *	      reached their nominal size.
     */
    delta = adjustment;

    numAdjust = 0;
    totalWeight = 0.0;
    for (link = Blt_Chain_FirstLink(chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;

	rcPtr = Blt_Chain_GetValue(link);
	if ((rcPtr->weight > 0.0) && (rcPtr->nom > rcPtr->size)) {
	    numAdjust++;
	    totalWeight += rcPtr->weight;
	}
    }

    while ((numAdjust > 0) && (totalWeight > 0.0) && (delta > 0)) {
	Blt_ChainLink link;
	int ration;		/* Amount of space to add to each
				 * row/column. */
	ration = (int)(delta / totalWeight);
	if (ration == 0) {
	    ration = 1;
	}
	for (link = Blt_Chain_FirstLink(chain); (link != NULL) && (delta > 0);
	    link = Blt_Chain_NextLink(link)) {
	    RowColumn *rcPtr;

	    rcPtr = Blt_Chain_GetValue(link);
	    if (rcPtr->weight > 0.0) {
		int avail;	/* Amount of space still available. */

		avail = rcPtr->nom - rcPtr->size;
		if (avail > 0) {
		    int size;	/* Amount of space requested for a particular
				 * row/column. */
		    size = (int)(ration * rcPtr->weight);
		    if (size > delta) {
			size = delta;
		    }
		    if (size < avail) {
			delta -= size;
			rcPtr->size += size;
		    } else {
			delta -= avail;
			rcPtr->size += avail;
			numAdjust--;
			totalWeight -= rcPtr->weight;
		    }
		}
	    }
	}
    }

    /*
     * Pass 2: Adjust the partitions with space still available
     */
    numAdjust = 0;
    totalWeight = 0.0;
    for (link = Blt_Chain_FirstLink(chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;

	rcPtr = Blt_Chain_GetValue(link);
	if ((rcPtr->weight > 0.0) && (rcPtr->max > rcPtr->size)) {
	    numAdjust++;
	    totalWeight += rcPtr->weight;
	}
    }
    while ((numAdjust > 0) && (totalWeight > 0.0) && (delta > 0)) {
	Blt_ChainLink link;
	int ration;		/* Amount of space to add to each
				 * row/column. */

	ration = (int)(delta / totalWeight);
	if (ration == 0) {
	    ration = 1;
	}
	for (link = Blt_Chain_FirstLink(chain); (link != NULL) && (delta > 0);
	    link = Blt_Chain_NextLink(link)) {
	    RowColumn *rcPtr;

	    rcPtr = Blt_Chain_GetValue(link);
	    if (rcPtr->weight > 0.0) {
		int avail;	/* Amount of space still available */

		avail = (rcPtr->max - rcPtr->size);
		if (avail > 0) {
		    int size;	/* Amount of space requested for a particular
				 * row/column. */
		    size = (int)(ration * rcPtr->weight);
		    if (size > delta) {
			size = delta;
		    }
		    if (size < avail) {
			delta -= size;
			rcPtr->size += size;
		    } else {
			delta -= avail;
			rcPtr->size += avail;
			numAdjust--;
			totalWeight -= rcPtr->weight;
		    }
		}
	    }
	}
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * ShrinkPartitions --
 *
 *	Shrink the span by the amount specified.  Size constraints on the
 *	partitions may prevent any or all of the spacing adjustments.
 *
 *	This is very much like the GrowSpan procedure, but in this case we are
 *	shrinking or expanding all the (row or column) partitions. It uses a
 *	two pass approach, first subtracting space to partitions which are
 *	larger than their nominal sizes. This is because constraints on the
 *	partitions may cause resizing to be non-linear.
 *
 *	After pass 1, if there is still extra to be removed, this means that
 *	all partitions are at least to their nominal sizes.  The second pass
 *	will try to remove the extra space evenly among all the partitions
 *	which still have space available (i.e haven't reached their respective
 *	min sizes).
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The size of the partitions may be decreased.
 *
 *---------------------------------------------------------------------------
 */
static void
ShrinkPartitions(
    PartitionInfo *piPtr,	/* Array of (column/row) partitions  */
    int adjustment)		/* The amount of extra space to shrink the
				 * span. It represents the amount of space to
				 * remove. */
{
    Blt_ChainLink link;
    int extra;			/* Amount of space needed */
    int numAdjust;		/* Number of rows/columns that still can be
				 * adjusted. */
    Blt_Chain chain;
    float totalWeight;

    chain = piPtr->chain;

    /*
     * Pass 1: First adjust the size of rows/columns that still aren't
     *	       at their nominal size.
     */
    extra = -adjustment;

    numAdjust = 0;
    totalWeight = 0.0;
    for (link = Blt_Chain_FirstLink(chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;

	rcPtr = Blt_Chain_GetValue(link);
	if ((rcPtr->weight > 0.0) && (rcPtr->nom < rcPtr->size)) {
	    numAdjust++;
	    totalWeight += rcPtr->weight;
	}
    }

    while ((numAdjust > 0) && (totalWeight > 0.0) && (extra > 0)) {
	Blt_ChainLink link;
	int ration;		/* Amount of space to subtract from each
				 * row/column. */
	ration = (int)(extra / totalWeight);
	if (ration == 0) {
	    ration = 1;
	}
	for (link = Blt_Chain_FirstLink(chain); (link != NULL) && (extra > 0);
	    link = Blt_Chain_NextLink(link)) {
	    RowColumn *rcPtr;

	    rcPtr = Blt_Chain_GetValue(link);
	    if (rcPtr->weight > 0.0) {
		int avail;	/* Amount of space still available */

		avail = rcPtr->size - rcPtr->nom;
		if (avail > 0) {
		    int slice;	/* Amount of space requested for a particular
				 * row/column. */
		    slice = (int)(ration * rcPtr->weight);
		    if (slice > extra) {
			slice = extra;
		    }
		    if (avail > slice) {
			extra -= slice;
			rcPtr->size -= slice;  
		    } else {
			extra -= avail;
			rcPtr->size -= avail;
			numAdjust--; /* Goes to zero (nominal). */
			totalWeight -= rcPtr->weight;
		    }
		}
	    }
	}
    }
    /*
     * Pass 2: Now adjust the partitions with space still available (i.e.
     *	       are bigger than their minimum size).
     */
    numAdjust = 0;
    totalWeight = 0.0;
    for (link = Blt_Chain_FirstLink(chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;

	rcPtr = Blt_Chain_GetValue(link);
	if ((rcPtr->weight > 0.0) && (rcPtr->size > rcPtr->min)) {
	    numAdjust++;
	    totalWeight += rcPtr->weight;
	}
    }
    while ((numAdjust > 0) && (totalWeight > 0.0) && (extra > 0)) {
	Blt_ChainLink link;
	int ration;		/* Amount of space to subtract from each
				 * row/column. */
	ration = (int)(extra / totalWeight);
	if (ration == 0) {
	    ration = 1;
	}
	for (link = Blt_Chain_FirstLink(chain); (link != NULL) && (extra > 0);
	    link = Blt_Chain_NextLink(link)) {
	    RowColumn *rcPtr;

	    rcPtr = Blt_Chain_GetValue(link);
	    if (rcPtr->weight > 0.0) {
		int avail;	/* Amount of space still available */

		avail = rcPtr->size - rcPtr->min;
		if (avail > 0) {
		    int slice;	/* Amount of space requested for a particular
				 * row/column. */
		    slice = (int)(ration * rcPtr->weight);
		    if (slice > extra) {
			slice = extra;
		    }
		    if (avail > slice) {
			extra -= slice;
			rcPtr->size -= slice;
		    } else {
			extra -= avail;
			rcPtr->size -= avail;
			numAdjust--;
			totalWeight -= rcPtr->weight;
		    }
		}
	    }
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetPartitions --
 *
 *	Sets/resets the size of each row and column partition to the minimum
 *	limit of the partition (this is usually zero). This routine gets
 *	called when new widgets are added, deleted, or resized.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 * 	The size of each partition is re-initialized to its minimum size.
 *
 *---------------------------------------------------------------------------
 */
static void
ResetPartitions(Table *tablePtr, PartitionInfo *piPtr, LimitsProc *limitsProc)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;
	int pad, size;

	rcPtr = Blt_Chain_GetValue(link);

	/*
	 * The constraint procedure below also has the desired side-effect of
	 * setting the minimum, maximum, and nominal values to the requested
	 * size of its associated widget (if one exists).
	 */
	size = (*limitsProc)(0, &rcPtr->reqSize);

	pad = PADDING(rcPtr->pad) + piPtr->ePad;
	if (rcPtr->reqSize.flags & LIMITS_SET_NOM) {

	    /*
	     * This could be done more cleanly.  We want to ensure that the
	     * requested nominal size is not overridden when determining the
	     * normal sizes.  So temporarily fix min and max to the nominal
	     * size and reset them back later.
	     */
	    rcPtr->min = rcPtr->max = rcPtr->size = rcPtr->nom = 
		size + pad;

	} else {
	    /* The range defaults to 0..MAXINT */
	    rcPtr->min = rcPtr->reqSize.min + pad;
	    rcPtr->max = rcPtr->reqSize.max + pad;
	    rcPtr->nom = LIMITS_NOM;
	    rcPtr->size = pad;
	}
	rcPtr->minSpan = 0;
	rcPtr->control = NULL;
	rcPtr->count = 0;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SetNominalSizes
 *
 *	Sets the normal sizes for each partition.  The partition size is the
 *	requested widget size plus an amount of padding.  In addition, adjust
 *	the min/max bounds of the partition depending upon the resize flags
 *	(whether the partition can be expanded or shrunk from its normal
 *	size).
 *
 * Results:
 *	Returns the total space needed for the all the partitions.
 *
 * Side Effects:
 *	The nominal size of each partition is set.  This is later used to
 *	determine how to shrink or grow the table if the container can't be
 *	resized to accommodate the exact size requirements of all the
 *	partitions.
 *
 *---------------------------------------------------------------------------
 */
static int
SetNominalSizes(Table *tablePtr, PartitionInfo *piPtr)
{
    Blt_ChainLink link;
    int total;

    total = 0;
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;
	int pad, size;

	rcPtr = Blt_Chain_GetValue(link);
	pad = PADDING(rcPtr->pad) + piPtr->ePad;

	/*
	 * Restore the real bounds after temporarily setting nominal size.
	 * These values may have been set in ResetPartitions to restrict the
	 * size of the partition to the requested range.
	 */

	rcPtr->min = rcPtr->reqSize.min + pad;
	rcPtr->max = rcPtr->reqSize.max + pad;

	size = rcPtr->size;
	if (size > rcPtr->max) {
	    size = rcPtr->max;
	} else if (size < rcPtr->min) {
	    size = rcPtr->min;
	}
	if ((piPtr->ePad > 0) && (size < tablePtr->editPtr->min)) {
	    size = tablePtr->editPtr->min;
	}
	rcPtr->nom = rcPtr->size = size;

	/*
	 * If a partition can't be resized (to either expand or shrink), hold
	 * its respective limit at its normal size.
	 */
	if (!(rcPtr->resize & RESIZE_EXPAND)) {
	    rcPtr->max = rcPtr->nom;
	}
	if (!(rcPtr->resize & RESIZE_SHRINK)) {
	    rcPtr->min = rcPtr->nom;
	}
	if (rcPtr->control == NULL) {
	    /* If a row/column contains no entries, then its size should be
	     * locked. */
	    if (rcPtr->resize & RESIZE_VIRGIN) {
		rcPtr->max = rcPtr->min = size;
	    } else {
		if (!(rcPtr->resize & RESIZE_EXPAND)) {
		    rcPtr->max = size;
		}
		if (!(rcPtr->resize & RESIZE_SHRINK)) {
		    rcPtr->min = size;
		}
	    }
	    rcPtr->nom = size;
	}
 	total += rcPtr->nom;
    }
    return total;
}

/*
 *---------------------------------------------------------------------------
 *
 * LockPartitions
 *
 *	Sets the maximum size of a row or column, if the partition has a
 *	widget that controls it.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
LockPartitions(PartitionInfo *piPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;

	rcPtr = Blt_Chain_GetValue(link);
	if (rcPtr->control != NULL) {
	    /* Partition is controlled by this widget */
	    rcPtr->max = rcPtr->size;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * LayoutPartitions --
 *
 *	Calculates the normal space requirements for both the row and column
 *	partitions.  Each widget is added in order of the number of rows or
 *	columns spanned, which defines the space needed among in the
 *	partitions spanned.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 * 	The sum of normal sizes set here will be used as the normal size for
 * 	the container widget.
 *
 *---------------------------------------------------------------------------
 */
static void
LayoutPartitions(Table *tablePtr)
{
    Blt_ListNode node;
    int total;
    PartitionInfo *piPtr;

    piPtr = &tablePtr->cols;

    ResetPartitions(tablePtr, piPtr, GetBoundedWidth);

    for (node = Blt_List_FirstNode(piPtr->list); node != NULL;
	node = Blt_List_NextNode(node)) {
	Blt_Chain chain;
	Blt_ChainLink link;

	chain = Blt_List_GetValue(node);

	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    TableEntry *tePtr;
	    int needed, used;

	    tePtr = Blt_Chain_GetValue(link);
	    if (tePtr->column.control != CONTROL_FULL) {
		continue;
	    }
	    needed = GetReqWidth(tePtr) + PADDING(tePtr->xPad) +
		2 * (tePtr->borderWidth + tablePtr->eEntryPad);
	    if (needed <= 0) {
		continue;
	    }
	    used = GetSpan(piPtr, tePtr);
	    if (needed > used) {
		GrowSpan(tablePtr, piPtr, tePtr, needed - used);
	    }
	}
    }

    LockPartitions(piPtr);

    for (node = Blt_List_FirstNode(piPtr->list); node != NULL;
	node = Blt_List_NextNode(node)) {
	Blt_Chain chain;
	Blt_ChainLink link;

	chain = Blt_List_GetValue(node);

	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    TableEntry *tePtr;
	    int needed, used;

	    tePtr = Blt_Chain_GetValue(link);

	    needed = GetReqWidth(tePtr) + PADDING(tePtr->xPad) +
		2 * (tePtr->borderWidth + tablePtr->eEntryPad);

	    if (tePtr->column.control >= 0.0) {
		needed = (int)(needed * tePtr->column.control);
	    }
	    if (needed <= 0) {
		continue;
	    }
	    used = GetSpan(piPtr, tePtr);
	    if (needed > used) {
		GrowSpan(tablePtr, piPtr, tePtr, needed - used);
	    }
	}
    }
    total = SetNominalSizes(tablePtr, piPtr);
    tablePtr->normal.width = GetBoundedWidth(total, &tablePtr->reqWidth) +
	PADDING(tablePtr->xPad) +
	2 * (tablePtr->eTablePad + Tk_InternalBorderWidth(tablePtr->tkwin));

    piPtr = &tablePtr->rows;

    ResetPartitions(tablePtr, piPtr, GetBoundedHeight);

    for (node = Blt_List_FirstNode(piPtr->list); node != NULL;
	node = Blt_List_NextNode(node)) {
	Blt_Chain chain;
	Blt_ChainLink link;

	chain = Blt_List_GetValue(node);

	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    TableEntry *tePtr;
	    int needed, used;

	    tePtr = Blt_Chain_GetValue(link);
	    if (tePtr->row.control != CONTROL_FULL) {
		continue;
	    }
	    needed = GetReqHeight(tePtr) + PADDING(tePtr->yPad) +
		2 * (tePtr->borderWidth + tablePtr->eEntryPad);
	    if (needed <= 0) {
		continue;
	    }
	    used = GetSpan(piPtr, tePtr);
	    if (needed > used) {
		GrowSpan(tablePtr, piPtr, tePtr, needed - used);
	    }
	}
    }
    LockPartitions(&tablePtr->rows);

    for (node = Blt_List_FirstNode(piPtr->list); node != NULL;
	node = Blt_List_NextNode(node)) {
	Blt_Chain chain;
	Blt_ChainLink link;

	chain = Blt_Chain_GetValue(node);
	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    TableEntry *tePtr;
	    int needed, used;

	    tePtr = Blt_Chain_GetValue(link);
	    needed = GetReqHeight(tePtr) + PADDING(tePtr->yPad) +
		2 * (tePtr->borderWidth + tablePtr->eEntryPad);
	    if (tePtr->row.control >= 0.0) {
		needed = (int)(needed * tePtr->row.control);
	    }
	    if (needed <= 0) {
		continue;
	    }
	    used = GetSpan(piPtr, tePtr);
	    if (needed > used) {
		GrowSpan(tablePtr, piPtr, tePtr, needed - used);
	    }
	}
    }
    total = SetNominalSizes(tablePtr, piPtr);
    tablePtr->normal.height = GetBoundedHeight(total, &tablePtr->reqHeight) +
	PADDING(tablePtr->yPad) +
	2 * (tablePtr->eTablePad + Tk_InternalBorderWidth(tablePtr->tkwin));
}

/*
 *---------------------------------------------------------------------------
 *
 * ArrangeEntries
 *
 *	Places each widget at its proper location.  First determines the size
 *	and position of the each widget.  It then considers the following:
 *
 *	  1. translation of widget position its parent widget.
 *	  2. fill style
 *	  3. anchor
 *	  4. external and internal padding
 *	  5. widget size must be greater than zero
 *
 * Results:
 *	None.
 *
 * Side Effects:
 * 	The size of each partition is re-initialized its minimum size.
 *
 *---------------------------------------------------------------------------
 */
static void
ArrangeEntries(Table *tablePtr)		/* Table widget structure */
{
    Blt_ChainLink link;
    int xMax, yMax;

    xMax = tablePtr->container.width -
	(Tk_InternalBorderWidth(tablePtr->tkwin) + tablePtr->padRight +
	tablePtr->eTablePad);
    yMax = tablePtr->container.height -
	(Tk_InternalBorderWidth(tablePtr->tkwin) + tablePtr->padBottom +
	tablePtr->eTablePad);

    for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	TableEntry *tePtr;
	int dx, dy;
	int extra;
	int spanWidth, spanHeight;
	int winWidth, winHeight;
	int x, y;

	tePtr = Blt_Chain_GetValue(link);

	x = tePtr->column.rcPtr->offset +
	    tePtr->column.rcPtr->pad.side1 +
	    tePtr->padLeft +
	    Tk_Changes(tePtr->tkwin)->border_width +
	    tablePtr->eEntryPad;
	y = tePtr->row.rcPtr->offset +
	    tePtr->row.rcPtr->pad.side1 +
	    tePtr->padTop +
	    Tk_Changes(tePtr->tkwin)->border_width +
	    tablePtr->eEntryPad;

	/*
	 * Unmap any widgets that start beyond of the right edge of the
	 * container.
	 */
	if ((x >= xMax) || (y >= yMax)) {
#ifdef notdef
 fprintf(stderr, "arrange entries: unmapping window %s %d>=%d %d>=%d\n", 
         Tk_PathName(tePtr->tkwin), x, xMax, y, yMax);
#endif
	    if (Tk_IsMapped(tePtr->tkwin)) {
		if (Tk_Parent(tePtr->tkwin) != tablePtr->tkwin) {
		    Tk_UnmaintainGeometry(tePtr->tkwin, tablePtr->tkwin);
		}
		Tk_UnmapWindow(tePtr->tkwin);
	    }
	    continue;
	}
	extra = 2 * (tePtr->borderWidth + tablePtr->eEntryPad);
	spanWidth = GetSpan(&tablePtr->cols, tePtr) -
	    (extra + PADDING(tePtr->xPad));
	spanHeight = GetSpan(&tablePtr->rows, tePtr) - 
	    (extra + PADDING(tePtr->yPad));

	winWidth = GetReqWidth(tePtr);
	winHeight = GetReqHeight(tePtr);

	/*
	 *
	 * Compare the widget's requested size to the size of the span.
	 *
	 * 1) If the widget is larger than the span or if the fill flag is
	 *    set, make the widget the size of the span. Check that the new size
	 *    is within the bounds set for the widget.
	 *
	 * 2) Otherwise, position the widget in the space according to its
	 *    anchor.
	 *
	 */
	if ((spanWidth <= winWidth) || (tePtr->fill & FILL_X)) {
	    winWidth = spanWidth;
	    if (winWidth > tePtr->reqWidth.max) {
		winWidth = tePtr->reqWidth.max;
	    }
	}
	if ((spanHeight <= winHeight) || (tePtr->fill & FILL_Y)) {
	    winHeight = spanHeight;
	    if (winHeight > tePtr->reqHeight.max) {
		winHeight = tePtr->reqHeight.max;
	    }
	}

	dx = dy = 0;
	if (spanWidth > winWidth) {
	    dx = (spanWidth - winWidth);
	}
	if (spanHeight > winHeight) {
	    dy = (spanHeight - winHeight);
	}
	if ((dx > 0) || (dy > 0)) {
	    TranslateAnchor(dx, dy, tePtr->anchor, &x, &y);
	}
	/*
	 * Clip the widget at the bottom and/or right edge of the container.
	 */
	if (winWidth > (xMax - x)) {
	    winWidth = (xMax - x);
	}
	if (winHeight > (yMax - y)) {
	    winHeight = (yMax - y);
	}

	/*
	 * If the widget is too small (i.e. it has only an external border)
	 * then unmap it.
	 */
	if ((winWidth < 1) || (winHeight < 1)) {
	    if (Tk_IsMapped(tePtr->tkwin)) {
		if (tablePtr->tkwin != Tk_Parent(tePtr->tkwin)) {
		    Tk_UnmaintainGeometry(tePtr->tkwin, tablePtr->tkwin);
		}
		Tk_UnmapWindow(tePtr->tkwin);
	    }
	    continue;
	}

	/*
	 * Resize and/or move the widget as necessary.
	 */
	tePtr->x = x;
	tePtr->y = y;

#ifdef notdef
        fprintf(stderr, "ArrangeEntries: %s rw=%d rh=%d w=%d h=%d\n",
                Tk_PathName(tePtr->tkwin), Tk_ReqWidth(tePtr->tkwin),
                Tk_ReqHeight(tePtr->tkwin), winWidth, winHeight);
#endif
	if (tablePtr->tkwin != Tk_Parent(tePtr->tkwin)) {
	    Tk_MaintainGeometry(tePtr->tkwin, tablePtr->tkwin, x, y,
		winWidth, winHeight);
	} else {
	    if ((x != Tk_X(tePtr->tkwin)) || (y != Tk_Y(tePtr->tkwin)) ||
		(winWidth != Tk_Width(tePtr->tkwin)) ||
		(winHeight != Tk_Height(tePtr->tkwin))) {
#ifdef notdef
		fprintf(stderr, "ArrangeEntries: %s rw=%d rh=%d w=%d h=%d\n",
			Tk_PathName(tePtr->tkwin), Tk_ReqWidth(tePtr->tkwin),
			Tk_ReqHeight(tePtr->tkwin), winWidth, winHeight);
#endif
		Tk_MoveResizeWindow(tePtr->tkwin, x, y, winWidth, winHeight);
	    }
	    if (!Tk_IsMapped(tePtr->tkwin)) {
		Tk_MapWindow(tePtr->tkwin);
	    }
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ArrangeTable --
 *
 *
 * Results:
 *	None.
 *
 * Side Effects:
 * 	The widgets in the table are possibly resized and redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
ArrangeTable(ClientData clientData)
{
    Table *tablePtr = clientData;
    int width, height;
    int offset, delta;
    int xPad, yPad;
    int outerPad;
    Blt_ChainLink link;

#ifdef notdef
    fprintf(stderr, "ArrangeTable(%s)\n", Tk_PathName(tablePtr->tkwin));
#endif
    Tcl_Preserve(tablePtr);
    tablePtr->flags &= ~ARRANGE_PENDING;

    tablePtr->rows.ePad = tablePtr->cols.ePad = tablePtr->eTablePad =
	tablePtr->eEntryPad = 0;
    if (tablePtr->editPtr != NULL) {
	tablePtr->rows.ePad = tablePtr->cols.ePad =
	    tablePtr->editPtr->gridLineWidth;
	tablePtr->eTablePad = tablePtr->editPtr->gridLineWidth;
	tablePtr->eEntryPad = tablePtr->editPtr->entryPad;
    }
    /*
     * If the table has no children anymore, then don't do anything at all:
     * just leave the container widget's size as-is.
     */
    if ((Blt_Chain_GetLength(tablePtr->chain) == 0) || 
	(tablePtr->tkwin == NULL)) {
	Tcl_Release(tablePtr);
	return;
    }
    if (tablePtr->flags & REQUEST_LAYOUT) {
	tablePtr->flags &= ~REQUEST_LAYOUT;
	LayoutPartitions(tablePtr);
    }
    /*
     * Initially, try to fit the partitions exactly into the container by
     * resizing the container.  If the widget's requested size is different,
     * send a request to the container widget's geometry manager to resize.
     */
    if ((tablePtr->propagate) &&
	((tablePtr->normal.width != Tk_ReqWidth(tablePtr->tkwin)) ||
	    (tablePtr->normal.height != Tk_ReqHeight(tablePtr->tkwin)))) {
	Tk_GeometryRequest(tablePtr->tkwin, tablePtr->normal.width,
	    tablePtr->normal.height);
	EventuallyArrangeTable(tablePtr);
	Tcl_Release(tablePtr);
	return;
    }
    /*
     * Save the width and height of the container so we know when its size has
     * changed during ConfigureNotify events.
     */
    tablePtr->container.width  = Tk_Width(tablePtr->tkwin);
    tablePtr->container.height = Tk_Height(tablePtr->tkwin);
    outerPad = 2 * (Tk_InternalBorderWidth(tablePtr->tkwin) +
	tablePtr->eTablePad);
    xPad = outerPad + tablePtr->cols.ePad + PADDING(tablePtr->xPad);
    yPad = outerPad + tablePtr->rows.ePad + PADDING(tablePtr->yPad);

    width = GetTotalSpan(&tablePtr->cols) + xPad;
    height = GetTotalSpan(&tablePtr->rows) + yPad;

    /*
     * If the previous geometry request was not fulfilled (i.e. the size of
     * the container is different from partitions' space requirements), try to
     * adjust size of the partitions to fit the widget.
     */
    delta = tablePtr->container.width - width;
    if (delta != 0) {
	if (delta > 0) {
	    GrowPartitions(&tablePtr->cols, delta);
	} else {
	    ShrinkPartitions(&tablePtr->cols, delta);
	}
	width = GetTotalSpan(&tablePtr->cols) + xPad;
    }
    delta = tablePtr->container.height - height;
    if (delta != 0) {
	if (delta > 0) {
	    GrowPartitions(&tablePtr->rows, delta);
	} else {
	    ShrinkPartitions(&tablePtr->rows, delta);
	}
	height = GetTotalSpan(&tablePtr->rows) + yPad;
    }

    /*
     * If after adjusting the size of the partitions the space required does
     * not equal the size of the widget, do one of the following:
     *
     * 1) If it's smaller, center the table in the widget.
     * 2) If it's bigger, clip the partitions that extend beyond
     *    the edge of the container.
     *
     * Set the row and column offsets (including the container's internal
     * border width). To be used later when positioning the widgets.
     */

    offset = Tk_InternalBorderWidth(tablePtr->tkwin) + tablePtr->padLeft +
	tablePtr->eTablePad;
    if (width < tablePtr->container.width) {
	offset += (tablePtr->container.width - width) / 2;
    }
    for (link = Blt_Chain_FirstLink(tablePtr->cols.chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	RowColumn *colPtr;

	colPtr = Blt_Chain_GetValue(link);
	colPtr->offset = offset + tablePtr->cols.ePad;
	offset += colPtr->size;
    }

    offset = Tk_InternalBorderWidth(tablePtr->tkwin) + tablePtr->padTop +
	tablePtr->eTablePad;
    if (height < tablePtr->container.height) {
	offset += (tablePtr->container.height - height) / 2;
    }
    for (link = Blt_Chain_FirstLink(tablePtr->rows.chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	RowColumn *rowPtr;

	rowPtr = Blt_Chain_GetValue(link);
	rowPtr->offset = offset + tablePtr->rows.ePad;
	offset += rowPtr->size;
    }

    ArrangeEntries(tablePtr);
    if (tablePtr->editPtr != NULL) {
	/* Redraw the editor */
	(*tablePtr->editPtr->drawProc) (tablePtr->editPtr);
    }
    Tcl_Release(tablePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ArrangeOp --
 *
 *	Forces layout of the table geometry manager.  This is useful mostly
 *	for debugging the geometry manager.  You can get the geometry manager
 *	to calculate the normal (requested) width and height of each row and
 *	column.  Otherwise, you need to first withdraw the container widget,
 *	invoke "update", and then query the geometry manager.
 *
 * Results:
 *	Returns a standard TCL result.  If the table is successfully
 *	rearranged, TCL_OK is returned. Otherwise, TCL_ERROR is returned and
 *	an error message is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ArrangeOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,		/* Interpreter to report errors to */
    int objc,
    Tcl_Obj *const *objv)	/* Path name of container associated with the
				 * table */
{
    Table *tablePtr;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    tablePtr->flags |= REQUEST_LAYOUT;
    ArrangeTable(tablePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *	Returns the name, position and options of a widget in the table.
 *
 * Results:
 *	Returns a standard TCL result.  A list of the widget attributes is
 *	left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    PartitionInfo *piPtr;
    Table *tablePtr;
    const char *string;
    char c;
    int length;
    int n;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 4) {
	return Blt_ConfigureValueFromObj(interp, tablePtr->tkwin, 
		tableConfigSpecs, (char *)tablePtr, objv[3], 0);
    }
    string = Tcl_GetStringFromObj(objv[3], &length);
    c = string[0];
    if (c == '.') {		/* Configure widget */
	TableEntry *tePtr;

	if (GetEntry(interp, tablePtr, Tcl_GetString(objv[3]), &tePtr) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
	return Blt_ConfigureValueFromObj(interp, tePtr->tkwin, 
		entryConfigSpecs, (char *)tePtr, objv[4], 0);
    } else if ((c == 'c') && (strncmp(string, "container", length) == 0)) {
	return Blt_ConfigureValueFromObj(interp, tablePtr->tkwin, 
		tableConfigSpecs, (char *)tablePtr, objv[4], 0);
    }
    piPtr = ParseRowColumn(tablePtr, objv[3], &n);
    if (piPtr == NULL) {
	return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, tablePtr->tkwin, 
	piPtr->configSpecs, (char *)GetRowColumn(piPtr, n), objv[4], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *	Returns the name, position and options of a widget in the table.
 *
 * Results:
 *	Returns a standard TCL result.  A list of the table configuration
 *	option information is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ConfigureOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Table *tablePtr;
    int count;
    int result;
    Tcl_Obj *const *items;
    int i;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    /*
     * Find the end of the items. Search until we see an option (-).
     */
    objc -= 3, objv += 3;
    for (count = 0; count < objc; count++) {
	const char *string;
	
	string = Tcl_GetString(objv[count]);
	if (string[0] == '-') {
	    break;
	}
    }
    items = objv;		/* Save the start of the item list */
    objc -= count;		/* Move beyond the items to the options */
    objv += count;
    result = TCL_ERROR;		/* Suppress compiler warning */

    if (count == 0) {
	result = ConfigureTable(tablePtr, interp, objc, objv);
    }
    for (i = 0; i < count; i++) {
	const char *string;
	char c1, c2;
	int length;

	string = Tcl_GetStringFromObj(items[i], &length);
	c1 = string[0];
	c2 = string[1];
	if (c1 == '.') {		/* Configure widget */
	    TableEntry *tePtr;

	    if (GetEntry(interp, tablePtr, string, &tePtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    result = ConfigureEntry(tablePtr, interp, tePtr, objc, objv);
	} else if ((c1 == 'r') || (c1 == 'R')) {
	    result = ConfigureRowColumn(tablePtr, &tablePtr->rows,
		string, objc, objv);
	} else if ((c1 == 'c') && (c2 == 'o') &&
	    (strncmp(string, "container", length) == 0)) {
	    result = ConfigureTable(tablePtr, interp, objc, objv);
	} else if ((c1 == 'c') || (c1 == 'C')) {
	    result = ConfigureRowColumn(tablePtr, &tablePtr->cols,
		string, objc, objv);
	} else {
	    Tcl_AppendResult(interp, "unknown item \"", string,
		"\": should be widget, row or column index, or \"container\"",
		(char *)NULL);
	    return TCL_ERROR;
	}
	if (result == TCL_ERROR) {
	    break;
	}
	if ((i + 1) < count) {
	    Tcl_AppendResult(interp, "\n", (char *)NULL);
	}
    }
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *	Deletes the specified rows and/or columns from the table.  Note that
 *	the row/column indices can be fixed only after all the deletions have
 *	occurred.
 *
 *		table delete .f r0 r1 r4 c0
 *
 * Results:
 *	Returns a standard TCL result.
 *
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Table *tablePtr;
    int matches;
    int i;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 3; i < objc; i++) {
	const char *pattern;
	char c;

	pattern = Tcl_GetString(objv[i]);
	c = tolower(pattern[0]);
	if ((c != 'r') && (c != 'c')) {
	    Tcl_AppendResult(interp, "bad index \"", pattern,
		"\": must start with \"r\" or \"c\"", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    matches = 0;
    for (i = 3; i < objc; i++) {
	Blt_ChainLink link, next;
	PartitionInfo *piPtr;
	const char *pattern;
	char c;

	pattern = Tcl_GetString(objv[i]);
	c = tolower(pattern[0]);
	piPtr = (c == 'r') ? &tablePtr->rows : &tablePtr->cols;
	for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
	    link = next) {
	    RowColumn *rcPtr;
	    char ident[200];

	    next = Blt_Chain_NextLink(link);
	    rcPtr = Blt_Chain_GetValue(link);
	    Blt_FormatString(ident, 200, "%c%d", c, rcPtr->index);
	    if (Tcl_StringMatch(ident, pattern)) {
		matches++;
		DeleteRowColumn(tablePtr, piPtr, rcPtr);
		Blt_Chain_DeleteLink(piPtr->chain, link);
	    }
	}
    }
    if (matches > 0) {		/* Fix indices */
	Blt_ChainLink link;

	i = 0;
	for (link = Blt_Chain_FirstLink(tablePtr->cols.chain);
	    link != NULL; link = Blt_Chain_NextLink(link)) {
	    RowColumn *rcPtr;

	    rcPtr = Blt_Chain_GetValue(link);
	    rcPtr->index = i++;
	}
	i = 0;
	for (link = Blt_Chain_FirstLink(tablePtr->rows.chain);
	    link != NULL; link = Blt_Chain_NextLink(link)) {
	    RowColumn *rcPtr;

	    rcPtr = Blt_Chain_GetValue(link);
	    rcPtr->index = i++;
	}
	tablePtr->flags |= REQUEST_LAYOUT;
	EventuallyArrangeTable(tablePtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * JoinOp --
 *
 *	Joins the specified span of rows/columns together into a partition.
 *	The row/column indices can be fixed only after all the deletions have
 *	occurred.
 *
 *		table join .f r0 r3
 *		table join .f c2 c4
 * Results:
 *	Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
JoinOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Table *tablePtr;
    Blt_ChainLink link, nextl, froml;
    PartitionInfo *piPtr, *info2Ptr;
    TableEntry *tePtr;
    int from, to;		/* Indices marking the span of partitions to
				 * be joined together.  */
    int start, end;		/* Entry indices. */
    int i;
    RowColumn *rcPtr;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    piPtr = ParseRowColumn(tablePtr, objv[3], &from);
    if (piPtr == NULL) {
	return TCL_ERROR;
    }
    info2Ptr = ParseRowColumn(tablePtr, objv[4], &to);
    if (info2Ptr == NULL) {
	return TCL_ERROR;
    }
    if (piPtr != info2Ptr) {
	Tcl_AppendResult(interp,
	    "\"from\" and \"to\" must both be rows or columns",
	    (char *)NULL);
	return TCL_ERROR;
    }
    if (from >= to) {
	return TCL_OK;		/* No-op. */
    }
    froml = Blt_Chain_GetNthLink(piPtr->chain, from);
    rcPtr = Blt_Chain_GetValue(froml);

    /*
     *	Reduce the span of all entries that currently cross any of the
     *	trailing rows/columns.  Also, if the entry starts in one of these
     *	rows/columns, moved it to the designated "joined" row/column.
     */
    if (piPtr->type == rowUid) {
	for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    tePtr = Blt_Chain_GetValue(link);
	    start = tePtr->row.rcPtr->index + 1;
	    end = tePtr->row.rcPtr->index + tePtr->row.span - 1;
	    if ((end < from) || ((start > to))) {
		continue;
	    }
	    tePtr->row.span -= to - start + 1;
	    if (start >= from) {/* Entry starts in a trailing partition. */
		tePtr->row.rcPtr = rcPtr;
	    }
	}
    } else {
	for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    tePtr = Blt_Chain_GetValue(link);
	    start = tePtr->column.rcPtr->index + 1;
	    end = tePtr->column.rcPtr->index + tePtr->column.span - 1;
	    if ((end < from) || ((start > to))) {
		continue;
	    }
	    tePtr->column.span -= to - start + 1;
	    if (start >= from) {/* Entry starts in a trailing partition. */
		tePtr->column.rcPtr = rcPtr;
	    }
	}
    }
    link = Blt_Chain_NextLink(froml);
    for (i = from + 1; i <= to; i++) {
	nextl = Blt_Chain_NextLink(link);
	rcPtr = Blt_Chain_GetValue(link);
	DeleteRowColumn(tablePtr, piPtr, rcPtr);
	Blt_Chain_DeleteLink(piPtr->chain, link);
	link = nextl;
    }
    i = 0;
    for (link = Blt_Chain_FirstLink(piPtr->chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	rcPtr->index = i++;
    }
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExtentsOp --
 *
 *	Returns a list of all the pathnames of the widgets managed by a table.
 *	The table is determined from the name of the container widget
 *	associated with the table.
 *
 *		table extents .frame r0 c0 container
 *
 * Results:
 *	Returns a standard TCL result.  If no error occurred, TCL_OK is
 *	returned and a list of widgets managed by the table is left in
 *	interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ExtentsOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,		/* Interpreter to return results to. */
    int objc,			/* # of arguments */
    Tcl_Obj *const *objv)	/* Command line arguments. */
{
    Table *tablePtr;
    Blt_ChainLink link;
    PartitionInfo *piPtr;
    Tcl_DString ds;
    char *pattern;
    char c;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    pattern = Tcl_GetString(objv[3]);
    c = tolower(pattern[0]);
    if (c == 'r') {
	piPtr = &tablePtr->rows;
    } else if (c == 'c') {
	piPtr = &tablePtr->cols;
    } else {
	Tcl_AppendResult(interp, "unknown item \"", pattern, 
		"\": should be widget, row, or column", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_DStringInit(&ds);
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;
	char ident[200];

	rcPtr = Blt_Chain_GetValue(link);
	Blt_FormatString(ident, 200, "%c%d", c, rcPtr->index);
	if (Tcl_StringMatch(ident, pattern)) {
	    int x, y, width, height;
	    RowColumn *c1Ptr, *r1Ptr, *c2Ptr, *r2Ptr;

	    if (c == 'r') {
		r1Ptr = r2Ptr = rcPtr;
		c1Ptr = GetRowColumn(&tablePtr->cols, 0);
		c2Ptr = GetRowColumn(&tablePtr->cols, 
				     tablePtr->numColumns - 1);
	    } else {
		c1Ptr = c2Ptr = rcPtr;
		r1Ptr = GetRowColumn(&tablePtr->rows, 0);
		r2Ptr = GetRowColumn(&tablePtr->rows, tablePtr->numRows - 1);
	    }
	    x = c1Ptr->offset;
	    y = r1Ptr->offset;
	    width = c2Ptr->offset + c2Ptr->size - x;
	    height = r2Ptr->offset + r2Ptr->size - y;
	    Tcl_DStringAppend(&ds, ident, -1);
	    Tcl_DStringAppend(&ds, " ", 1);
	    Tcl_DStringAppend(&ds, Blt_Itoa(rcPtr->index), -1);
	    Tcl_DStringAppend(&ds, " ", 1);
	    Tcl_DStringAppend(&ds, Blt_Itoa(x), -1);
	    Tcl_DStringAppend(&ds, " ", 1);
	    Tcl_DStringAppend(&ds, Blt_Itoa(y), -1);
	    Tcl_DStringAppend(&ds, " ", 1);
	    Tcl_DStringAppend(&ds, Blt_Itoa(width), -1);
	    Tcl_DStringAppend(&ds, " ", 1);
	    Tcl_DStringAppend(&ds, Blt_Itoa(height), -1);
	    Tcl_DStringAppend(&ds, "\n", 1);
	}
    }
    Tcl_DStringResult(interp, &ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ForgetOp --
 *
 *	Processes an objv/objc list of widget names and purges their entries
 *	from their respective tables.  The widgets are unmapped and the tables
 *	are rearranged at the next idle point.  Note that all the named
 *	widgets do not need to exist in the same table.
 *
 * Results:
 *	Returns a standard TCL result.  If an error occurred, TCL_ERROR is
 *	returned and an error message is left in interp->result.
 *
 * Side Effects:
 *	Memory is deallocated (the entry is destroyed), etc.  The affected
 *	tables are is re-computed and arranged at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static int
ForgetOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    TableEntry *tePtr;
    int i;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Table *tablePtr;
    Tk_Window tkwin;
    char *string;

    tablePtr = NULL;
    for (i = 2; i < objc; i++) {
	tePtr = NULL;
	string = Tcl_GetString(objv[i]);
	tkwin = Tk_NameToWindow(interp, string, dataPtr->tkMain);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	for (hPtr = Blt_FirstHashEntry(&dataPtr->tableTable, &iter);
	    hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    tablePtr = Blt_GetHashValue(hPtr);
	    if (tablePtr->interp != interp) {
		continue;
	    }
	    tePtr = FindEntry(tablePtr, tkwin);
	    if (tePtr != NULL) {
		break;
	    }
	}
	if (tePtr == NULL) {
	    Tcl_AppendResult(interp, "\"", string,
		"\" is not managed by any table", (char *)NULL);
	    return TCL_ERROR;
	}
	if (Tk_IsMapped(tePtr->tkwin)) {
	    Tk_UnmapWindow(tePtr->tkwin);
	}
	/* Arrange for the call back here in the loop, because the widgets may
	 * not belong to the same table.  */
	tablePtr->flags |= REQUEST_LAYOUT;
	EventuallyArrangeTable(tablePtr);
	DestroyEntry(tePtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InfoOp --
 *
 *	Returns the options of a widget or partition in the table.
 *
 * Results:
 *	Returns a standard TCL result.  A list of the widget attributes is
 *	left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InfoOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Table *tablePtr;
    int result;
    char c;
    int i;
    char *string;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 3; i < objc; i++) {
	string = Tcl_GetString(objv[i]);
	c = string[0];
	if (c == '.') {		/* Entry information */
	    TableEntry *tePtr;

	    if (GetEntry(interp, tablePtr, string, &tePtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    result = InfoEntry(interp, tablePtr, tePtr);
	} else if ((c == 'r') || (c == 'R') || (c == 'c') || (c == 'C')) {
	    result = InfoRowColumn(tablePtr, interp, string);
	} else {
	    Tcl_AppendResult(interp, "unknown item \"", string,
		"\": should be widget, row, or column", (char *)NULL);
	    return TCL_ERROR;
	}
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((i + 1) < objc) {
	    Tcl_AppendResult(interp, "\n", (char *)NULL);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InsertOp --
 *
 *	Inserts a span of rows/columns into the table.
 *
 *		table insert .f r0 2
 *		table insert .f c0 5
 *
 * Results:
 *	Returns a standard TCL result.  A list of the widget attributes is
 *	left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InsertOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{ 
    Table *tablePtr;
    long int span;
    int iBefore;
    PartitionInfo *piPtr;
    RowColumn *rcPtr;
    int i;
    Blt_ChainLink before, link;
    int linkBefore;
    char *string;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    linkBefore = TRUE;

    string = Tcl_GetString(objv[3]);
    if (string[0] == '-') {
	if (strcmp(string, "-before") == 0) {
	    linkBefore = TRUE;
	    objv++; objc--;
	} else if (strcmp(string, "-after") == 0) {
	    linkBefore = FALSE;
	    objv++; objc--;
	}	    
    } 
    if (objc == 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), "insert ", Tcl_GetString(objv[2]), 
		"row|column ?span?", (char *)NULL);
	return TCL_ERROR;
    }
    piPtr = ParseRowColumn(tablePtr, objv[3], &iBefore);
    if (piPtr == NULL) {
	return TCL_ERROR;
    }
    span = 1;
    if ((objc > 4) && 
	(Tcl_ExprLong(interp, Tcl_GetString(objv[4]), &span) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (span < 1) {
	Tcl_AppendResult(interp, "span value \"", Tcl_GetString(objv[4]),
	    "\" can't be negative", (char *)NULL);
	return TCL_ERROR;
    }
    before = Blt_Chain_GetNthLink(piPtr->chain, iBefore);
    /*
     * Insert the new rows/columns from the designated point in the
     * chain.
     */
    for (i = 0; i < span; i++) {
	rcPtr = CreateRowColumn();
	link = Blt_Chain_NewLink();
	Blt_Chain_SetValue(link, rcPtr);
	if (linkBefore) {
	    Blt_Chain_LinkBefore(piPtr->chain, link, before);
	} else {
	    Blt_Chain_LinkAfter(piPtr->chain, link, before);
	}
	rcPtr->link = link;
    }
    i = 0;
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	/* Reset the indices of the trailing rows/columns.  */
	rcPtr->index = i++;
    }
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SplitOp --
 *
 *	Splits a single row/column into multiple partitions. Any widgets that
 *	span this row/column will be automatically corrected to include the
 *	new rows/columns.
 *
 *		table split .f r0 3
 *		table split .f c2 2
 * Results:
 *	Returns a standard TCL result.  A list of the widget attributes is
 *	left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SplitOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Table *tablePtr;
    int number, split;
    int start, end;
    PartitionInfo *piPtr;
    RowColumn *rcPtr;
    int i;
    Blt_ChainLink after, link;
    TableEntry *tePtr;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    piPtr = ParseRowColumn(tablePtr, objv[3], &number);
    if (piPtr == NULL) {
	return TCL_ERROR;
    }
    split = 2;
    if (objc > 4) {
	if (Tcl_GetIntFromObj(interp, objv[4], &split) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (split < 2) {
	Tcl_AppendResult(interp, "bad split value \"", Tcl_GetString(objv[4]),
	    "\": should be 2 or greater", (char *)NULL);
	return TCL_ERROR;
    }
    after = Blt_Chain_GetNthLink(piPtr->chain, number);

    /*
     * Append (split - 1) additional rows/columns starting
     * from the current point in the chain.
     */

    for (i = 1; i < split; i++) {
	rcPtr = CreateRowColumn();
	link = Blt_Chain_NewLink();
	Blt_Chain_SetValue(link, rcPtr);
	Blt_Chain_LinkAfter(piPtr->chain, link, after);
	rcPtr->link = link;
    }

    /*
     * Also increase the span of all entries that span this row/column by
     * split - 1.
     */
    if (piPtr->type == rowUid) {
	for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    tePtr = Blt_Chain_GetValue(link);
	    start = tePtr->row.rcPtr->index;
	    end = tePtr->row.rcPtr->index + tePtr->row.span;
	    if ((start <= number) && (number < end)) {
		tePtr->row.span += (split - 1);
	    }
	}
    } else {
	for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    tePtr = Blt_Chain_GetValue(link);
	    start = tePtr->column.rcPtr->index;
	    end = tePtr->column.rcPtr->index + tePtr->column.span;
	    if ((start <= number) && (number < end)) {
		tePtr->column.span += (split - 1);
	    }
	}
    }
    /*
     * Be careful to renumber the rows or columns only after processing each
     * entry.  Otherwise row/column numbering will be out of sync with the
     * index.
     */
    i = number;
    for (link = after; link != NULL; link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	rcPtr->index = i++;	/* Renumber the trailing indices.  */
    }

    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowColumnSearch --
 *
 * 	Searches for the row or column designated by an x or y coordinate.
 *
 * Results:
 *	Returns a pointer to the row/column containing the given point.  If no
 *	row/column contains the coordinate, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static RowColumn *
RowColumnSearch(PartitionInfo *piPtr, int x)
{
    Blt_ChainLink link;

    /* 
     * This search assumes that rows/columns are organized in increasing
     * order.
     */
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;

	rcPtr = Blt_Chain_GetValue(link);
	/*
	 *|         |offset    |offset+size  |        |
	 *            ^
	 *            x
	 */
	if (x < rcPtr->offset) { 
	    return NULL;	/* Too far, can't find row/column. */
	}
	if (x < (rcPtr->offset + rcPtr->size)) {
	    return rcPtr;
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * LocateOp --
 *
 *
 *	Returns the row,column index given a screen coordinate.
 *
 * Results:
 *	Returns a standard TCL result.
 *
 *	table locate .t %X %Y
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
LocateOp(TableInterpData *dataPtr, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    RowColumn *rowPtr, *colPtr;
    Table *tablePtr;
    Tcl_Obj *listObjPtr;
    int x, y;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_GetPixelsFromObj(interp, tablePtr->tkwin, objv[3], PIXELS_ANY, &x)
	!= TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_GetPixelsFromObj(interp, tablePtr->tkwin, objv[4], PIXELS_ANY, &y)
	!= TCL_OK) {
	return TCL_ERROR;
    }
    rowPtr = RowColumnSearch(&tablePtr->rows, y);
    if (rowPtr == NULL) {
	return TCL_OK;
    }
    colPtr = RowColumnSearch(&tablePtr->cols, x);
    if (colPtr == NULL) {
	return TCL_OK;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(rowPtr->index));
    Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewIntObj(colPtr->index));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *	Returns a list of tables currently in use. A table is associated by
 *	the name of its container widget.  All tables matching a given pattern
 *	are included in this list.  If no pattern is present (objc == 0), all
 *	tables are included.
 *
 * Results:
 *	Returns a standard TCL result.  If no error occurred, TCL_OK is
 *	returned and a list of tables is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NamesOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,		/* Interpreter to return list of names to */
    int objc,
    Tcl_Obj *const *objv)	/* Contains 0-1 arguments: search pattern */
{
    NamesSwitches switches;

    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, namesSwitches, objc - 3, objv + 3, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (switches.slave != NULL) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;
	Tk_Window tkwin;

	tkwin = Tk_NameToWindow(interp, switches.slave, dataPtr->tkMain);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	for (hPtr = Blt_FirstHashEntry(&dataPtr->tableTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Table *tablePtr;

	    tablePtr = Blt_GetHashValue(hPtr);
	    if (FindEntry(tablePtr, tkwin) != NULL) {
		Tcl_AppendElement(interp, Tk_PathName(tablePtr->tkwin));
	    }
	}
    } else {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;
	const char *pattern;

	pattern = (switches.pattern == NULL) ? Tcl_GetString(objv[3]) : 
	    switches.pattern;
	for (hPtr = Blt_FirstHashEntry(&dataPtr->tableTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Table *tablePtr;

	    tablePtr = Blt_GetHashValue(hPtr);
	    if (tablePtr->interp == interp) {
		if ((pattern == NULL) ||
		    (Tcl_StringMatch(Tk_PathName(tablePtr->tkwin), pattern))) {
		    Tcl_AppendElement(interp, Tk_PathName(tablePtr->tkwin));
		}
	    }
	}
    }	
    Blt_FreeSwitches(namesSwitches, (char *)&switches, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SaveOp --
 *
 *	Returns a list of all the commands necessary to rebuild the the table.
 *	This includes the layout of the widgets and any row, column, or table
 *	options set.
 *
 * Results:
 *	Returns a standard TCL result.  If no error occurred, TCL_OK is
 *	returned and a list of widget path names is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SaveOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Table *tablePtr;
    Blt_ChainLink link, lastl;
    PartitionInfo *piPtr;
    Tcl_DString ds;
    int start, last;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, "\n# Table widget layout\n\n", -1);
    Tcl_DStringAppend(&ds, Tcl_GetString(objv[0]), -1);
    Tcl_DStringAppend(&ds, " ", -1);
    Tcl_DStringAppend(&ds, Tk_PathName(tablePtr->tkwin), -1);
    Tcl_DStringAppend(&ds, " \\\n", -1);
    lastl = Blt_Chain_LastLink(tablePtr->chain);
    for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	TableEntry *tePtr;

	tePtr = Blt_Chain_GetValue(link);
	PrintEntry(tePtr, &ds);
	if (link != lastl) {
	    Tcl_DStringAppend(&ds, " \\\n", -1);
	}
    }
    Tcl_DStringAppend(&ds, "\n\n# Row configuration options\n\n", -1);
    piPtr = &tablePtr->rows;
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;

	rcPtr = Blt_Chain_GetValue(link);
	start = Tcl_DStringLength(&ds);
	Tcl_DStringAppend(&ds, Tcl_GetString(objv[0]), -1);
	Tcl_DStringAppend(&ds, " configure ", -1);
	Tcl_DStringAppend(&ds, Tk_PathName(tablePtr->tkwin), -1);
	Tcl_DStringAppend(&ds, " r", -1);
	Tcl_DStringAppend(&ds, Blt_Itoa(rcPtr->index), -1);
	last = Tcl_DStringLength(&ds);
	PrintRowColumn(interp, piPtr, rcPtr, &ds);
	if (Tcl_DStringLength(&ds) == last) {
	    Tcl_DStringSetLength(&ds, start);
	} else {
	    Tcl_DStringAppend(&ds, "\n", -1);
	}
    }
    Tcl_DStringAppend(&ds, "\n\n# Column configuration options\n\n", -1);
    piPtr = &tablePtr->cols;
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	RowColumn *rcPtr;

	rcPtr = Blt_Chain_GetValue(link);
	start = Tcl_DStringLength(&ds);
	Tcl_DStringAppend(&ds, Tcl_GetString(objv[0]), -1);
	Tcl_DStringAppend(&ds, " configure ", -1);
	Tcl_DStringAppend(&ds, Tk_PathName(tablePtr->tkwin), -1);
	Tcl_DStringAppend(&ds, " c", -1);
	Tcl_DStringAppend(&ds, Blt_Itoa(rcPtr->index), -1);
	last = Tcl_DStringLength(&ds);
	PrintRowColumn(interp, piPtr, rcPtr, &ds);
	if (Tcl_DStringLength(&ds) == last) {
	    Tcl_DStringSetLength(&ds, start);
	} else {
	    Tcl_DStringAppend(&ds, "\n", -1);
	}
    }
    start = Tcl_DStringLength(&ds);
    Tcl_DStringAppend(&ds, "\n\n# Table configuration options\n\n", -1);
    Tcl_DStringAppend(&ds, Tcl_GetString(objv[0]), -1);
    Tcl_DStringAppend(&ds, " configure ", -1);
    Tcl_DStringAppend(&ds, Tk_PathName(tablePtr->tkwin), -1);
    last = Tcl_DStringLength(&ds);
    PrintTable(tablePtr, &ds);
    if (Tcl_DStringLength(&ds) == last) {
	Tcl_DStringSetLength(&ds, start);
    } else {
	Tcl_DStringAppend(&ds, "\n", -1);
    }
    Tcl_DStringResult(interp, &ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SearchOp --
 *
 *	Returns a list of all the pathnames of the widgets managed by a table
 *	geometry manager.  The table is given by the path name of a container
 *	widget associated with the table.
 *
 * Results:
 *	Returns a standard TCL result.  If no error occurred, TCL_OK is
 *	returned and a list of widget path names is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SearchOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,		/* Interpreter to return list of names to */
    int objc,			/* Number of arguments */
    Tcl_Obj *const *objv)	/* Contains 1-2 arguments: pathname of
				 * container widget associated with the table
				 * and search pattern */
{
    Table *tablePtr;
    Blt_ChainLink link;
    SearchSwitches switches;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }

    memset(&switches, 0, sizeof(switches));
    switches.tablePtr = tablePtr;
    if (Blt_ParseSwitches(interp, searchSwitches, objc - 3, objv + 3, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }

    /* Find entries that match the search criteria. */

    for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	TableEntry *tePtr;

	tePtr = Blt_Chain_GetValue(link);
	if (switches.pattern != NULL) {
	    if (!Tcl_StringMatch(Tk_PathName(tePtr->tkwin), switches.pattern)) {
		continue;
	    }
	}
	if (switches.flags & MATCH_SPAN) {
	    if ((switches.rspan >= 0) && 
		(tePtr->row.rcPtr->index > switches.rspan) &&
		((tePtr->row.rcPtr->index + tePtr->row.span) < switches.rspan)){
		continue;
	    } 
	    if ((switches.cspan >= 0) && 
		((tePtr->column.rcPtr->index > switches.cspan) ||
		((tePtr->column.rcPtr->index + tePtr->column.span) <
		 switches.cspan))) {
		continue;
	    }
	}
	if (switches.flags & MATCH_START) {
	    if ((switches.rstart >= 0) && 
		(tePtr->row.rcPtr->index != switches.rstart)) {
		continue;
	    }
	    if ((switches.cstart >= 0) && 
		(tePtr->column.rcPtr->index != switches.cstart)) {
		continue;
	    }
	}
	Tcl_AppendElement(interp, Tk_PathName(tePtr->tkwin));
    }
    Blt_FreeSwitches(searchSwitches, (char *)&switches, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Table operations.
 *
 * The fields for Blt_OpSpec are as follows:
 *
 *   - operation name
 *   - minimum number of characters required to disambiguate the operation name.
 *   - function associated with operation.
 *   - minimum number of arguments required.
 *   - maximum number of arguments allowed (0 indicates no limit).
 *   - usage string
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec tableOps[] =
{
    {"arrange",    1, ArrangeOp,   3, 3, "container",},
    {"cget",       2, CgetOp,      4, 5, 
	"container ?row|column|widget? option",},
    {"configure",  4, ConfigureOp, 3, 0,
	"container ?row|column|widget?... ?option value?...",},
    {"containers", 4, NamesOp,     2, 4, "?switch? ?arg?",},
    {"delete",     1, DeleteOp,    3, 0, "container row|column ?row|column?",},
    {"extents",    1, ExtentsOp,   4, 4, "container row|column|widget",},
    {"forget",     1, ForgetOp,    3, 0, "widget ?widget?...",},
    {"info",       3, InfoOp,      3, 0, "container ?row|column|widget?...",},
    {"insert",     3, InsertOp,    4, 6,
	"container ?-before|-after? row|column ?count?",},
    {"join",       1, JoinOp,      5, 5, "container first last",},
    {"locate",     1, LocateOp,    5, 5, "container x y",},
    {"names",      1, NamesOp,     2, 4, "?switch? ?arg?",},
    {"save",       2, SaveOp,      3, 3, "container",},
    {"search",     2, SearchOp,    3, 0, "container ?switch arg?...",},
    {"split",      2, SplitOp,     4, 5, "container row|column div",},
};

static int numTableOps = sizeof(tableOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * TableCmd --
 *
 *	This procedure is invoked to process the TCL command that corresponds
 *	to the table geometry manager.  See the user documentation for details
 *	on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static int
TableCmd(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    TableCmdProc *proc;

    if (objc > 1) {
	char *string;

	string = Tcl_GetString(objv[1]);
	if (string[0] == '.') {
	    Table *tablePtr;

	    if (Blt_GetTableFromObj(clientData, interp, objv[1], &tablePtr) 
		!= TCL_OK) {
		Tcl_ResetResult(interp);
		tablePtr = CreateTable(dataPtr, interp, string);
		if (tablePtr == NULL) {
		    return TCL_ERROR;
		}
	    }
	    return BuildTable(tablePtr, interp, objc, objv);
	}
    }
    proc = Blt_GetOpFromObj(interp, numTableOps, tableOps, BLT_OP_ARG1, 
		objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc)(dataPtr, interp, objc, objv);
}


/*
 *---------------------------------------------------------------------------
 *
 * TableInterpDeleteProc --
 *
 *	This is called when the interpreter hosting the table command is
 *	destroyed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destroys all the hash table maintaining the names of the table
 *	geometry managers.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
TableInterpDeleteProc(
    ClientData clientData,	/* Thread-specific data. */
    Tcl_Interp *interp)
{
    TableInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&dataPtr->tableTable, &iter);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	Table *tablePtr;

	tablePtr = Blt_GetHashValue(hPtr);
	tablePtr->hashPtr = NULL;
	DestroyTable((DestroyData)tablePtr);
    }
    Blt_DeleteHashTable(&dataPtr->tableTable);
    Tcl_DeleteAssocData(interp, TABLE_THREAD_KEY);
    Blt_Free(dataPtr);
}

static TableInterpData *
GetTableInterpData(Tcl_Interp *interp)
{
    TableInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (TableInterpData *)
	Tcl_GetAssocData(interp, TABLE_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
	dataPtr = Blt_AssertMalloc(sizeof(TableInterpData));
	dataPtr->tkMain = Tk_MainWindow(interp);
	Tcl_SetAssocData(interp, TABLE_THREAD_KEY, TableInterpDeleteProc, 
		dataPtr);
	Blt_InitHashTable(&dataPtr->tableTable, BLT_ONE_WORD_KEYS);
    }
    return dataPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_TableMgrCmdInitProc --
 *
 *	This procedure is invoked to initialize the TCL command that
 *	corresponds to the table geometry manager.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the new TCL command.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_TableMgrCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "table", TableCmd, };

    cmdSpec.clientData = GetTableInterpData(interp);
    rowUid = (Blt_Uid)Tk_GetUid("row");
    columnUid = (Blt_Uid)Tk_GetUid("column");
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

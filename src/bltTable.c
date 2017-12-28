/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTable.c --
 *
 * This module implements a table-based geometry manager for the BLT
 * toolkit.
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

/*
 * To do:
 *
 * 3) No way to detect if widget is already a container of another geometry
 *    manager.  This one is especially bad with toplevel widgets, causing
 *    the window manager to lock-up trying to handle the myriads of resize
 *    requests.
 *
 *   Note: This problem continues in Tk 8.x.  It's possible for a widget to
 *          be a container for two different geometry managers.  Each
 *          manager will set its own requested geometry for the container
 *          widget. The winner sets the geometry last (sometimes ad
 *          infinitum).
 *
 * 7) Relative sizing of partitions?
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

#ifdef HAVE_LIMITS_H
  #include <limits.h>
#endif  /* HAVE_LIMITS_H */

#include "bltAlloc.h"
#include "bltTable.h"
#include "bltSwitch.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define TABLE_THREAD_KEY        "BLT Table Data"
#define TABLE_DEF_PAD           0

/*
 * Default values for widget attributes.
 */
#define DEF_TABLE_ANCHOR        "center"
#define DEF_TABLE_COLUMNS       "0"
#define DEF_TABLE_FILL          "none"
#define DEF_TABLE_PAD           "0"
#define DEF_TABLE_PROPAGATE     "1"
#define DEF_TABLE_RESIZE        "both"
#define DEF_TABLE_ROWS          "0"
#define DEF_TABLE_SPAN          "1"
#define DEF_TABLE_CONTROL       "normal"
#define DEF_TABLE_WEIGHT        "1.0"

#define ENTRY_DEF_PAD           0
#define ENTRY_DEF_ANCHOR        TK_ANCHOR_CENTER
#define ENTRY_DEF_FILL          FILL_NONE
#define ENTRY_DEF_SPAN          1
#define ENTRY_DEF_CONTROL       CONTROL_NORMAL
#define ENTRY_DEF_IPAD          0

#define ROWCOL_DEF_RESIZE       (RESIZE_BOTH | RESIZE_VIRGIN)
#define ROWCOL_DEF_PAD          0
#define ROWCOL_DEF_WEIGHT       1.0

#define MATCH_PATTERN   (1<<0)  /* Find widgets whose path names
                                 * match a given pattern */
#define MATCH_SPAN      (1<<1)  /* Find widgets that span index  */
#define MATCH_START     (1<<2)  /* Find widgets that start at index */


static Blt_Uid rowUid, columnUid;

static Tk_GeomRequestProc WidgetGeometryProc;
static Tk_GeomLostSlaveProc WidgetCustodyProc;
static Tk_GeomMgr tableMgrInfo =
{
    (char *)"table",                    /* Name of geometry manager used by
                                         * winfo */
    WidgetGeometryProc,                 /* Procedure to for new geometry
                                         * requests */
    WidgetCustodyProc,                  /* Procedure when widget is taken
                                         * away */
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
    {BLT_CONFIG_SYNONYM, "-cspan", "columnSpan"},
    {BLT_CONFIG_SYNONYM, "-ccontrol", "columnControl"},
    {BLT_CONFIG_FILL, "-fill", (char *)NULL, (char *)NULL, DEF_TABLE_FILL, 
        Blt_Offset(TableEntry, fill), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-height", "reqHeight"},
    {BLT_CONFIG_PIXELS_NNEG, "-ipadx", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(TableEntry, iPadX), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-ipady", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(TableEntry, iPadY), 0},
    {BLT_CONFIG_PAD, "-padx", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(TableEntry, padX), 0},
    {BLT_CONFIG_PAD, "-pady", (char *)NULL, (char *)NULL,
        (char *)NULL, Blt_Offset(TableEntry, padY), 0},
    {BLT_CONFIG_CUSTOM, "-reqheight", "reqHeight", (char *)NULL, (char *)NULL, 
        Blt_Offset(TableEntry, reqHeight), 0, &limitsOption},
    {BLT_CONFIG_CUSTOM, "-reqwidth", "reqWidth", (char *)NULL, (char *)NULL, 
        Blt_Offset(TableEntry, reqWidth), 0, &limitsOption},
    {BLT_CONFIG_INT, "-rowspan", "rowSpan", (char *)NULL, DEF_TABLE_SPAN, 
        Blt_Offset(TableEntry, row.span), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-rowcontrol", "rowControl", (char *)NULL,
        DEF_TABLE_CONTROL, Blt_Offset(TableEntry, row.control),
        BLT_CONFIG_DONT_SET_DEFAULT, &controlOption},
    {BLT_CONFIG_SYNONYM, "-rspan", "rowSpan"},
    {BLT_CONFIG_SYNONYM, "-rcontrol", "rowControl"},
    {BLT_CONFIG_SYNONYM, "-width", "reqWidth"},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};


static Blt_ConfigSpec tableConfigSpecs[] =
{
    {BLT_CONFIG_PAD, "-padx", (char *)NULL, (char *)NULL,
        DEF_TABLE_PAD, Blt_Offset(Table, padX),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-pady", (char *)NULL, (char *)NULL,
        DEF_TABLE_PAD, Blt_Offset(Table, padY),
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

static Blt_SwitchParseProc ObjToColumn;
static Blt_SwitchCustom columnSwitch = {
    ObjToColumn, NULL, NULL, (ClientData)0,
};

static Blt_SwitchParseProc ObjToRow;
static Blt_SwitchCustom rowSwitch = {
    ObjToRow, NULL, NULL, (ClientData)0,
};

typedef struct {
    RowColumn *beforePtr;
    RowColumn *afterPtr;
    int count;
} InsertSwitches;

static Blt_SwitchSpec rowInsertSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-after", "index", (char *)NULL,
        Blt_Offset(InsertSwitches, afterPtr), 0, 0, &rowSwitch},
    {BLT_SWITCH_CUSTOM, "-before", "index", (char *)NULL,
        Blt_Offset(InsertSwitches, beforePtr), 0, 0, &rowSwitch},
    {BLT_SWITCH_INT_POS, "-numrows", "number", (char *)NULL,
        Blt_Offset(InsertSwitches, count), 0, 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec columnInsertSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-after", "index", (char *)NULL,
        Blt_Offset(InsertSwitches, afterPtr), 0, 0, &columnSwitch},
    {BLT_SWITCH_CUSTOM, "-before", "index", (char *)NULL,
        Blt_Offset(InsertSwitches, beforePtr), 0, 0, &columnSwitch},
    {BLT_SWITCH_INT_POS, "-numcolumns", "number", (char *)NULL,
        Blt_Offset(InsertSwitches, count), 0, 0},
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

static void DestroyEntry(TableEntry * entryPtr);
static void BinEntry(Table *tablePtr, TableEntry * entryPtr);
static RowColumn *InitSpan(PartitionInfo * piPtr, int start, int span);
static int ParseItem(Table *tablePtr, const char *string, int *rowPtr, 
        int *colPtr);

static EntrySearchProc FindEntry;

typedef int (TableCmdProc)(TableInterpData *dataPtr, Tcl_Interp *interp, 
        int objc, Tcl_Obj *const *objv);


static void
RenumberIndices(Blt_Chain chain)
{
    int i;
    Blt_ChainLink link;
    
    i = 0;
    for (link = Blt_Chain_FirstLink(chain);
        link != NULL; link = Blt_Chain_NextLink(link)) {
        RowColumn *rcPtr;

        rcPtr = Blt_Chain_GetValue(link);
        rcPtr->index = i;
        i++;
    }
}

static int
GetColumnFromObj(Tcl_Interp *interp, Table *tablePtr, Tcl_Obj *objPtr,
                 RowColumn **rcPtrPtr)
{
    int colIndex;
    Blt_ChainLink link;
    PartitionInfo *piPtr;
    const char *string;
    char c;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'e') && (strcmp(string, "end") == 0)) {
        colIndex = NumColumns(tablePtr) - 1;
    } else {
        if (Tcl_GetIntFromObj(interp, objPtr, &colIndex) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if ((colIndex < 0) || (colIndex >= NumColumns(tablePtr))) {
        Tcl_AppendResult(interp, "invalid row index \"", Blt_Itoa(colIndex),
                         "\"", (char *)NULL);
        return TCL_ERROR;
    }
    piPtr = &tablePtr->columns;
    link = Blt_Chain_GetNthLink(piPtr->chain, colIndex);
    *rcPtrPtr = Blt_Chain_GetValue(link);
    return TCL_OK;
}

static int
GetRowFromObj(Tcl_Interp *interp, Table *tablePtr, Tcl_Obj *objPtr,
              RowColumn **rcPtrPtr)
{
    Blt_ChainLink link;
    PartitionInfo *piPtr;
    char c;
    const char *string;
    int rowIndex;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'e') && (strcmp(string, "end") == 0)) {
        rowIndex = NumRows(tablePtr) - 1;
    } else {
        if (Tcl_GetIntFromObj(interp, objPtr, &rowIndex) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if ((rowIndex < 0) || (rowIndex >= NumRows(tablePtr))) {
        Tcl_AppendResult(interp, "invalid row index \"", Blt_Itoa(rowIndex),
                         "\"", (char *)NULL);
        return TCL_ERROR;
    }
    piPtr = &tablePtr->rows;
    link = Blt_Chain_GetNthLink(piPtr->chain, rowIndex);
    *rcPtrPtr = Blt_Chain_GetValue(link);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnSearch --
 *
 *      Searches for the row or column designated by an x or y coordinate.
 *
 * Results:
 *      Returns a pointer to the row/column containing the given point.  If no
 *      row/column contains the coordinate, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static RowColumn *
ColumnSearch(Table *tablePtr, int x)
{
    Blt_ChainLink link;
    PartitionInfo *piPtr = &tablePtr->columns;

    /* We assume that columns are organized in increasing order. */
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
            return NULL;        /* Too far, can't find column. */
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
 * RowSearch --
 *
 *      Searches for the row or column designated by an x or y coordinate.
 *
 * Results:
 *      Returns a pointer to the row/column containing the given point.  If
 *      no row/column contains the coordinate, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static RowColumn *
RowSearch(Table *tablePtr, int y)
{
    Blt_ChainLink link;
    PartitionInfo *piPtr = &tablePtr->rows;

    /* We assume that rows are organized in increasing order. */
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        RowColumn *rcPtr;

        rcPtr = Blt_Chain_GetValue(link);
        /*
         *|         |offset    |offset+size  |        |
         *            ^
         *            y
         */
        if (y < rcPtr->offset) { 
            return NULL;        /* Too far, can't find row. */
        }
        if (y < (rcPtr->offset + rcPtr->size)) {
            return rcPtr;
        }
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLimits --
 *
 *      Converts the list of elements into zero or more pixel values which
 *      determine the range of pixel values possible.  An element can be in
 *      any form accepted by Tk_GetPixels. The list has a different meaning
 *      based upon the number of elements.
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
    ClientData clientData,        /* Not used. */
    Tcl_Interp *interp,           /* Interpreter to send results back to */
    Tk_Window tkwin,              /* Widget of table */
    Tcl_Obj *objPtr,              /* New width list */
    char *widgRec,                /* Widget record */
    int offset,                   /* Offset to field in structure */
    int flags)  
{
    Limits *limitsPtr = (Limits *)(widgRec + offset);
    int numArgs;
    int limArr[3];
    Tk_Window winArr[3];
    int limitsFlags;

    /* Initialize limits to default values */
    limArr[2] = LIMITS_NOM;
    limArr[1] = LIMITS_MAX;
    limArr[0] = LIMITS_MIN;
    winArr[0] = winArr[1] = winArr[2] = NULL;
    limitsFlags = 0;

    numArgs = 0;
    if (objPtr != NULL) {
        Tcl_Obj **objv;
        int objc;
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
            char c0, c1;
            
            string = Tcl_GetString(objv[i]);
            c0 = string[0];
            if (c0 == '\0') {
                continue;               /* Empty string: use default
                                         * value */
            }
            c1 = string[1];
            limitsFlags |= (LIMITS_SET_BIT << i);
            if ((c0 == '.') && ((c1 == '\0') || isalpha(UCHAR(c1)))) {
                Tk_Window tkwin2;

                /* Widget specified: save pointer to widget */
                tkwin2 = Tk_NameToWindow(interp, string, tkwin);
                if (tkwin2 == NULL) {
                    return TCL_ERROR;
                }
                winArr[i] = tkwin2;
            } else {
                if (Tk_GetPixelsFromObj(interp, tkwin, objv[i], &size)
                    != TCL_OK) {
                    return TCL_ERROR;
                }
                if ((size < LIMITS_MIN) || (size > LIMITS_MAX)) {
                    Tcl_AppendResult(interp, "bad limits \"", string, "\"",
                        (char *)NULL);
                    return TCL_ERROR;
                }
                limArr[i] = size;
            }
        }
        numArgs = objc;
    }
    /*
     * Check the limits specified.  We can't check the requested size of
     * widgets.
     */
    switch (numArgs) {
    case 1:
        limitsFlags |= (LIMITS_SET_MIN | LIMITS_SET_MAX);
        if (winArr[0] == NULL) {
            limArr[1] = limArr[0];      /* Set minimum and maximum to
                                         * value */
        } else {
            winArr[1] = winArr[0];
        }
        break;

    case 2:
        if ((winArr[0] == NULL) && (winArr[1] == NULL) &&
            (limArr[1] < limArr[0])) {
            Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
                "\": min > max", (char *)NULL);
            return TCL_ERROR;           /* Minimum is greater than
                                         * maximum */
        }
        break;

    case 3:
        if ((winArr[0] == NULL) && (winArr[1] == NULL)) {
            if (limArr[1] < limArr[0]) {
                Tcl_AppendResult(interp, "bad range \"", Tcl_GetString(objPtr),
                    "\": min > max", (char *)NULL);
                return TCL_ERROR;       /* Minimum is greater than maximum */
            }
            if ((winArr[2] == NULL) &&
                ((limArr[2] < limArr[0]) || (limArr[2] > limArr[1]))) {
                Tcl_AppendResult(interp, "nominal value \"", 
                    Tcl_GetString(objPtr), "\" out of range", (char *)NULL);
                return TCL_ERROR;       /* Nominal is outside of range defined
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
ResetLimits(Limits *limitsPtr)  /* Limits to be imposed on the value */
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
 *      Bounds a given width value to the limits described in the limit
 *      structure.  The initial starting value may be overridden by the
 *      nominal value in the limits.
 *
 * Results:
 *      Returns the constrained value.
 *
 *---------------------------------------------------------------------------
 */
static int
GetBoundedWidth(
    int width,                  /* Initial value to be constrained */
    Limits *limitsPtr)          /* Limits to be imposed on the value */
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
        width = limitsPtr->nom; /* Override initial value */
    }
    if (width < limitsPtr->min) {
        width = limitsPtr->min; /* Bounded by minimum value */
    } else if (width > limitsPtr->max) {
        width = limitsPtr->max; /* Bounded by maximum value */
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
static int
GetBoundedHeight(
    int height,                 /* Initial value to be constrained */
    Limits *limitsPtr)          /* Limits to be imposed on the value */
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
 *      Convert the values into a list representing the limits.
 *
 * Results:
 *      The static string representation of the limits is returned.
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
    ClientData clientData,      /* Not used. */
    Tcl_Interp *interp,         /* Not used. */
    Tk_Window tkwin,            /* Not used. */
    char *widgRec,              /* Row/column structure record */
    int offset,                 /* Offset to field in structure */
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
 *      Converts the control string into its numeric representation.  Valid
 *      control strings are "none", "normal", and "full".
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToControl(
    ClientData clientData,      /* Not used. */
    Tcl_Interp *interp,         /* Interpreter to send results back to */
    Tk_Window tkwin,            /* Not used. */
    Tcl_Obj *objPtr,            /* Control style string */
    char *widgRec,              /* Entry structure record */
    int offset,                 /* Offset to field in structure */
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
 *      Converts the control value into its string representation.
 *
 * Results:
 *      Returns a pointer to the static name string.
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

        Blt_FmtString(string, TCL_DOUBLE_SPACE, "%g", (double)control);
        return string;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ControlToObj --
 *
 *      Returns control mode string based upon the control flags.
 *
 * Results:
 *      The control mode string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ControlToObj(
    ClientData clientData,      /* Not used. */
    Tcl_Interp *interp,         /* Not used. */
    Tk_Window tkwin,            /* Not used. */
    char *widgRec,              /* Row/column structure record */
    int offset,                 /* Offset to field in structure */
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
 *      Converts the position mode into its numeric representation.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPosition(
    ClientData clientData,      /* Flag indicating if the node is considered
                                 * before or after the insertion position. */
    Tcl_Interp *interp,         /* Interpreter to send results back to */
    const char *switchName,     /* Not used. */
    Tcl_Obj *objPtr,            /* String representation */
    char *record,               /* Structure record */
    int offset,                 /* Offset to field in structure */
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

/*
 *---------------------------------------------------------------------------
 *
 * ObjToColumn --
 *
 *      Converts the column index into a pointer to the column.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColumn(ClientData clientData, Tcl_Interp *interp, const char *switchName,
            Tcl_Obj *objPtr, char *record, int offset, int flags)       
{
    Table *tablePtr = clientData;
    RowColumn **colPtrPtr = (RowColumn **)(record + offset);
    RowColumn *colPtr;

    if (GetColumnFromObj(interp, tablePtr, objPtr, &colPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    *colPtrPtr = colPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToRow --
 *
 *      Converts the row index into a pointer to the row.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToRow(ClientData clientData, Tcl_Interp *interp, const char *switchName,
            Tcl_Obj *objPtr, char *record, int offset, int flags)       
{
    Table *tablePtr = clientData;
    RowColumn **rowPtrPtr = (RowColumn **)(record + offset);
    RowColumn *rowPtr;

    if (GetRowFromObj(interp, tablePtr, objPtr, &rowPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    *rowPtrPtr = rowPtr;
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
 *      This procedure is invoked by the Tk event handler when the container
 *      widget is reconfigured or destroyed.
 *
 *      The table will be rearranged at the next idle point if the container
 *      widget has been resized or moved. There's a distinction made between
 *      parent and non-parent container arrangements.  When the container is
 *      the parent of the embedded widgets, the widgets will automatically
 *      keep their positions relative to the container, even when the
 *      container is moved.  But if the container is not the parent, those
 *      widgets have to be moved manually.  This can be a performance hit in
 *      rare cases where we're scrolling the container (by moving the window)
 *      and there are lots of non-child widgets arranged inside.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the table associated with tkwin to have its layout
 *      re-computed and drawn at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static void
TableEventProc(
    ClientData clientData,      /* Information about widget */
    XEvent *eventPtr)           /* Information about event */
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
 *      This procedure is invoked by the Tk event handler when StructureNotify
 *      events occur in a widget managed by the table.
 *
 *      For example, when a managed widget is destroyed, it frees the
 *      corresponding entry structure and arranges for the table layout to be
 *      re-computed at the next idle point.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      If the managed widget was deleted, the Entry structure gets cleaned up
 *      and the table is rearranged.
 *
 *---------------------------------------------------------------------------
 */
static void
WidgetEventProc(
    ClientData clientData,      /* Pointer to Entry structure for widget
                                 * referred to by eventPtr. */
    XEvent *eventPtr)           /* Describes what just happened. */
{
    TableEntry *entryPtr = clientData;
    Table *tablePtr;

    tablePtr = entryPtr->tablePtr;
    if (eventPtr->type == ConfigureNotify) {
        int borderWidth;

        tablePtr->flags |= REQUEST_LAYOUT;
        borderWidth = Tk_Changes(entryPtr->tkwin)->border_width;
        if (entryPtr->borderWidth != borderWidth) {
            entryPtr->borderWidth = borderWidth;
            EventuallyArrangeTable(tablePtr);
        }
    } else if (eventPtr->type == DestroyNotify) {
        DestroyEntry(entryPtr);
        tablePtr->flags |= REQUEST_LAYOUT;
        EventuallyArrangeTable(tablePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * WidgetCustodyProc --
 *
 *      This procedure is invoked when a widget has been stolen by another
 *      geometry manager.  The information and memory associated with the
 *      widget is released.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the table to have its layout re-arranged at the next idle
 *      point.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
WidgetCustodyProc(
    ClientData clientData,      /* Information about the widget */
    Tk_Window tkwin)            /* Not used. */
{
    TableEntry *entryPtr = (TableEntry *) clientData;
    Table *tablePtr = entryPtr->tablePtr;

    if (Tk_IsMapped(entryPtr->tkwin)) {
        Tk_UnmapWindow(entryPtr->tkwin);
    }
    Tk_UnmaintainGeometry(entryPtr->tkwin, tablePtr->tkwin);
    DestroyEntry(entryPtr);
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * WidgetGeometryProc --
 *
 *      This procedure is invoked by Tk_GeometryRequest for widgets managed by
 *      the table geometry manager.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the table to have its layout re-computed and re-arranged
 *      at the next idle point.
 *
 * ---------------------------------------------------------------------------- */
/* ARGSUSED */
static void
WidgetGeometryProc(
    ClientData clientData,      /* Information about widget that got new
                                 * preferred geometry.  */
    Tk_Window tkwin)            /* Other Tk-related information about the
                                 * widget. */
{
    TableEntry *entryPtr = (TableEntry *) clientData;

    entryPtr->tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(entryPtr->tablePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FindEntry --
 *
 *      Searches for the table entry corresponding to the given widget.
 *
 * Results:
 *      If a structure associated with the widget exists, a pointer to that
 *      structure is returned. Otherwise NULL.
 *
 *---------------------------------------------------------------------------
 */
static TableEntry *
FindEntry(
    Table *tablePtr,
    Tk_Window tkwin)            /* Widget associated with table entry */
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
         TableEntry **entryPtrPtr)
{
    Tk_Window tkwin;
    TableEntry *entryPtr;

    tkwin = Tk_NameToWindow(interp, string, tablePtr->tkwin);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    entryPtr = FindEntry(tablePtr, tkwin);
    if (entryPtr == NULL) {
        Tcl_AppendResult(interp, "\"", Tk_PathName(tkwin),
            "\" is not managed by any table", (char *)NULL);
        return TCL_ERROR;
    }
    *entryPtrPtr = entryPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateEntry --
 *
 *      This procedure creates and initializes a new Entry structure to hold a
 *      widget.  A valid widget has a parent widget that is either a) the
 *      container widget itself or b) a mutual ancestor of the container
 *      widget.
 *
 * Results:
 *      Returns a pointer to the new structure describing the new widget
 *      entry.  If an error occurred, then the return value is NULL and an
 *      error message is left in interp->result.
 *
 * Side effects:
 *      Memory is allocated and initialized for the Entry structure.
 *
 * ---------------------------------------------------------------------------- 
 */
static TableEntry *
CreateEntry(
    Table *tablePtr,
    Tk_Window tkwin)
{
    TableEntry *entryPtr;
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
    entryPtr = Blt_AssertCalloc(1, sizeof(TableEntry));

    /* Initialize the entry structure */

    entryPtr->tkwin = tkwin;
    entryPtr->tablePtr = tablePtr;
    entryPtr->borderWidth = Tk_Changes(tkwin)->border_width;
    entryPtr->fill = ENTRY_DEF_FILL;
    entryPtr->row.control = entryPtr->column.control = ENTRY_DEF_CONTROL;
    entryPtr->anchor = ENTRY_DEF_ANCHOR;
    entryPtr->row.span = entryPtr->column.span = ENTRY_DEF_SPAN;
    ResetLimits(&entryPtr->reqWidth);
    ResetLimits(&entryPtr->reqHeight);

    /*
     * Add the entry to the following data structures.
     *
     *  1) A chain of widgets managed by the table.
     *   2) A hash table of widgets managed by the table.
     */
    entryPtr->link = Blt_Chain_Append(tablePtr->chain, entryPtr);
    entryPtr->hashPtr = Blt_CreateHashEntry(&tablePtr->entryTable,
        (char *)tkwin, &dummy);
    Blt_SetHashValue(entryPtr->hashPtr, entryPtr);

    Tk_CreateEventHandler(tkwin, StructureNotifyMask, WidgetEventProc, 
        entryPtr);
    Tk_ManageGeometry(tkwin, &tableMgrInfo, entryPtr);

    return entryPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyEntry --
 *
 *      Removes the Entry structure from the hash table and frees the memory
 *      allocated by it.  If the table is still in use (i.e. was not called
 *      from DestoryTable), remove its entries from the lists of row and
 *      column sorted partitions.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the entry is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyEntry(TableEntry *entryPtr)
{
    Table *tablePtr = entryPtr->tablePtr;

    if (entryPtr->row.link != NULL) {
        Blt_Chain_DeleteLink(entryPtr->row.chain, entryPtr->row.link);
    }
    if (entryPtr->column.link != NULL) {
        Blt_Chain_DeleteLink(entryPtr->column.chain,
            entryPtr->column.link);
    }
    if (entryPtr->link != NULL) {
        Blt_Chain_DeleteLink(tablePtr->chain, entryPtr->link);
    }
    if (entryPtr->tkwin != NULL) {
        Tk_DeleteEventHandler(entryPtr->tkwin, StructureNotifyMask, 
                WidgetEventProc, entryPtr);
        Tk_ManageGeometry(entryPtr->tkwin, (Tk_GeomMgr *)NULL, entryPtr);
        if ((tablePtr->tkwin != NULL) && 
            (Tk_Parent(entryPtr->tkwin) != tablePtr->tkwin)) {
            Tk_UnmaintainGeometry(entryPtr->tkwin, tablePtr->tkwin);
        }
        if (Tk_IsMapped(entryPtr->tkwin)) {
            Tk_UnmapWindow(entryPtr->tkwin);
        }
    }
    if (entryPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&tablePtr->entryTable, entryPtr->hashPtr);
    }
    Blt_Free(entryPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureEntry --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) one or more
 *      entries.  Entries hold information about widgets managed by the table
 *      geometry manager.
 *
 *      Note:   You can query only one widget at a time.  But several can be
 *              reconfigured at once.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is returned,
 *      then interp->result contains an error message.
 *
 * Side effects:
 *      The table layout is recomputed and rearranged at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureEntry(Table *tablePtr, Tcl_Interp *interp, TableEntry *entryPtr,
               int objc, Tcl_Obj *const *objv)
{
    int oldRowSpan, oldColSpan;

    if (entryPtr->tablePtr != tablePtr) {
        Tcl_AppendResult(interp, "widget  \"", Tk_PathName(entryPtr->tkwin),
            "\" does not belong to table \"", Tk_PathName(tablePtr->tkwin),
            "\"", (char *)NULL);
        return TCL_ERROR;
    }
    if (objc == 0) {
        return Blt_ConfigureInfoFromObj(interp, entryPtr->tkwin, 
                entryConfigSpecs, (char *)entryPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 1) {
        return Blt_ConfigureInfoFromObj(interp, entryPtr->tkwin, 
                entryConfigSpecs, (char *)entryPtr, objv[0], 0);
    }
    oldRowSpan = entryPtr->row.span;
    oldColSpan = entryPtr->column.span;

    if (Blt_ConfigureWidgetFromObj(interp, entryPtr->tkwin, entryConfigSpecs,
            objc, objv, (char *)entryPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((entryPtr->column.span < 1) || (entryPtr->column.span > USHRT_MAX)) {
        Tcl_AppendResult(interp, "bad column span specified for \"",
            Tk_PathName(entryPtr->tkwin), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    if ((entryPtr->row.span < 1) || (entryPtr->row.span > USHRT_MAX)) {
        Tcl_AppendResult(interp, "bad row span specified for \"",
            Tk_PathName(entryPtr->tkwin), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    if ((oldColSpan != entryPtr->column.span) ||
        (oldRowSpan != entryPtr->row.span)) {
        BinEntry(tablePtr, entryPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PrintEntry --
 *
 *      Returns the name, position and options of a widget in the table.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the widget attributes is
 *      left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
PrintEntry(TableEntry *entryPtr, Blt_DBuffer dbuffer)
{
    Blt_DBuffer_Format(dbuffer, "    %d,%d  %s",
                       entryPtr->row.rcPtr->index,
                       entryPtr->column.rcPtr->index,
                       Tk_PathName(entryPtr->tkwin));
    if (entryPtr->iPadX != ENTRY_DEF_PAD) {
        Blt_DBuffer_Format(dbuffer, " -ipadx %d", entryPtr->iPadX);
    }
    if (entryPtr->iPadY != ENTRY_DEF_PAD) {
        Blt_DBuffer_Format(dbuffer, " -ipady %d", entryPtr->iPadY);
    }
    if (entryPtr->row.span != ENTRY_DEF_SPAN) {
        Blt_DBuffer_Format(dbuffer, " -rowspan %d", entryPtr->row.span);
    }
    if (entryPtr->column.span != ENTRY_DEF_SPAN) {
        Blt_DBuffer_Format(dbuffer, " -columnspan %d", entryPtr->column.span);
    }
    if (entryPtr->anchor != ENTRY_DEF_ANCHOR) {
        Blt_DBuffer_Format(dbuffer, " -anchor %s",
                           Tk_NameOfAnchor(entryPtr->anchor));
    }
    if ((entryPtr->padX.side1 != ENTRY_DEF_PAD) ||
        (entryPtr->padX.side2 != ENTRY_DEF_PAD)) {
        Blt_DBuffer_Format(dbuffer, " -padx {%d %d}", entryPtr->padX.side1,
                           entryPtr->padX.side2);
    }
    if ((entryPtr->padY.side1 != ENTRY_DEF_PAD) ||
        (entryPtr->padY.side2 != ENTRY_DEF_PAD)) {
        Blt_DBuffer_Format(dbuffer, " -pady {%d %d}", entryPtr->padY.side1,
                           entryPtr->padY.side2);
    }
    if (entryPtr->fill != ENTRY_DEF_FILL) {
        Blt_DBuffer_Format(dbuffer, " -fill %s",
                           Blt_NameOfFill(entryPtr->fill));
    }
    if (entryPtr->column.control != ENTRY_DEF_CONTROL) {
        Blt_DBuffer_Format(dbuffer, " -columncontrol %s",
                           NameOfControl(entryPtr->column.control));
    }
    if (entryPtr->row.control != ENTRY_DEF_CONTROL) {
        Blt_DBuffer_Format(dbuffer, " -rowcontrol %s",
                           NameOfControl(entryPtr->row.control));
    }
    if ((entryPtr->reqWidth.nom != LIMITS_NOM) ||
        (entryPtr->reqWidth.min != LIMITS_MIN) ||
        (entryPtr->reqWidth.max != LIMITS_MAX)) {
        Blt_DBuffer_Format(dbuffer, " -reqwidth %s",
                           NameOfLimits(&entryPtr->reqWidth));
    }
    if ((entryPtr->reqHeight.nom != LIMITS_NOM) ||
        (entryPtr->reqHeight.min != LIMITS_MIN) ||
        (entryPtr->reqHeight.max != LIMITS_MAX)) {
        Blt_DBuffer_Format(dbuffer, " -reqheight %s",
                           NameOfLimits(&entryPtr->reqHeight));
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * InfoEntry --
 *
 *      Returns the name, position and options of a widget in the table.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the widget attributes is
 *      left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InfoEntry(Tcl_Interp *interp, Table *tablePtr, TableEntry *entryPtr)
{
    Blt_DBuffer dbuffer;

    if (entryPtr->tablePtr != tablePtr) {
        Tcl_AppendResult(interp, "widget  \"", Tk_PathName(entryPtr->tkwin),
            "\" does not belong to table \"", Tk_PathName(tablePtr->tkwin),
            "\"", (char *)NULL);
        return TCL_ERROR;
    }
    dbuffer = Blt_DBuffer_Create();
    PrintEntry(entryPtr, dbuffer);
    Tcl_SetObjResult(interp, Blt_DBuffer_StringObj(dbuffer));
    Blt_DBuffer_Destroy(dbuffer);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateRowColumn --
 *
 *      Creates and initializes a structure that manages the size of a row
 *      or column in the table. There will be one of these structures
 *      allocated for each row and column in the table, regardless if a
 *      widget is contained in it or not.
 *
 * Results:
 *      Returns a pointer to the newly allocated row or column structure.
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
        piPtr = &tablePtr->columns;
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
 *      Gets the designated row or column from the table.  If the row or
 *      column index is greater than the size of the table, new
 *      rows/columns will be automatically allocated.
 *
 * Results:
 *      Returns a pointer to the row or column structure.
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
 *      Deletes a span of rows/columns from the table. The number of
 *      rows/columns to be deleted is given by span.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The size of the column partition array may be extended and
 *      initialized.
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
            TableEntry *entryPtr;

            next = Blt_Chain_NextLink(link);
            entryPtr = Blt_Chain_GetValue(link);
            if (entryPtr->row.rcPtr->index == rcPtr->index) {
                DestroyEntry(entryPtr);
            }
        }
    } else {
        Blt_ChainLink link, next;

        for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
            link = next) {
            TableEntry *entryPtr;

            next = Blt_Chain_NextLink(link);
            entryPtr = Blt_Chain_GetValue(link);
            if (entryPtr->column.rcPtr->index == rcPtr->index) {
                DestroyEntry(entryPtr);
            }
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureRowColumn --
 *
 *      This procedure is called to process an objv/objc list in order to
 *      configure a row or column in the table geometry manager.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is returned,
 *      then interp->result holds an error message.
 *
 * Side effects:
 *      Partition configuration options (bounds, resize flags, etc) get set.
 *      New partitions may be created as necessary. The table is recalculated
 *      and arranged at the next idle point.
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
        Blt_FmtString(string, 200, "%c%d", pattern[0], rcPtr->index);
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
PrintRowColumn(Tcl_Interp *interp, PartitionInfo *piPtr, RowColumn *rcPtr,
               Blt_DBuffer dbuffer)
{
    const char *padFmt, *sizeFmt;

    if (piPtr->type == rowUid) {
        padFmt = " -pady {%d %d}";
        sizeFmt = " -height {%s}";
    } else {
        padFmt = " -padx {%d %d}";
        sizeFmt = " -width {%s}";
    }
    if (rcPtr->resize != ROWCOL_DEF_RESIZE) {
        Blt_DBuffer_Format(dbuffer, " -resize %s",
                           Blt_NameOfResize(rcPtr->resize));
    }
    if ((rcPtr->pad.side1 != ROWCOL_DEF_PAD) ||
        (rcPtr->pad.side2 != ROWCOL_DEF_PAD)) {
        Blt_DBuffer_Format(dbuffer, padFmt, rcPtr->pad.side1, rcPtr->pad.side2);
    }
    if (rcPtr->weight != ROWCOL_DEF_WEIGHT) {
        Blt_DBuffer_Format(dbuffer, " -weight %g", rcPtr->weight);
    }
    if ((rcPtr->reqSize.min != LIMITS_MIN) ||
        (rcPtr->reqSize.nom != LIMITS_NOM) ||
        (rcPtr->reqSize.max != LIMITS_MAX)) {
        Blt_DBuffer_Format(dbuffer, sizeFmt, NameOfLimits(&rcPtr->reqSize));
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * InitSpan --
 *
 *      Checks the size of the column partitions and extends the size if a
 *      larger array is needed.
 *
 * Results:
 *      Always return a RowColumn pointer.
 *
 * Side effects:
 *      The size of the column partition array may be extended and
 *      initialized.
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
 *      Searches for a table associated by the path name of the widget
 *      container.
 *
 *      Errors may occur because
 *        1) pathName isn't a valid for any Tk widget, or
 *        2) there's no table associated with that widget as a container.
 *
 * Results:
 *      If a table entry exists, a pointer to the Table structure is
 *      returned. Otherwise NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
/*LINTLIBRARY*/
int
Blt_GetTableFromObj(TableInterpData *dataPtr, Tcl_Interp *interp,
                    Tcl_Obj *objPtr, Table **tablePtrPtr)
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
 *      This procedure creates and initializes a new Table structure with
 *      tkwin as its container widget. The internal structures associated with
 *      the table are initialized.
 *
 * Results:
 *      Returns the pointer to the new Table structure describing the new
 *      table geometry manager.  If an error occurred, the return value will
 *      be NULL and an error message is left in interp->result.
 *
 * Side effects:
 *      Memory is allocated and initialized, an event handler is set up to
 *      watch tkwin, etc.
 *
 *---------------------------------------------------------------------------
 */
static Table *
CreateTable(TableInterpData *dataPtr, Tcl_Interp *interp, const char *pathName)
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
    tablePtr->columns.type = columnUid;
    tablePtr->columns.configSpecs = columnConfigSpecs;
    tablePtr->columns.chain = Blt_Chain_Create();
    tablePtr->propagate = TRUE;

    tablePtr->arrangeProc = ArrangeTable;
    Blt_InitHashTable(&tablePtr->entryTable, BLT_ONE_WORD_KEYS);
    tablePtr->findEntryProc = FindEntry;

    ResetLimits(&tablePtr->reqWidth);
    ResetLimits(&tablePtr->reqHeight);

    tablePtr->chain = Blt_Chain_Create();
    tablePtr->rows.list = Blt_List_Create(BLT_ONE_WORD_KEYS);
    tablePtr->columns.list = Blt_List_Create(BLT_ONE_WORD_KEYS);

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
 *      This procedure is called to process an objv/objc list in order to
 *      configure the table geometry manager.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is returned,
 *      then interp->result contains an error message.
 *
 * Side effects:
 *      Table configuration options (-padx, -pady, etc.) get set.  The table
 *      is recalculated and arranged at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureTable(
    Table *tablePtr,            /* Table to be configured */
    Tcl_Interp *interp,         /* Interpreter to report results back to */
    int objc,
    Tcl_Obj *const *objv)       /* Option-value pairs */
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
    /* Arrange for the layout to be computed at the next idle point. */
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

static void
PrintTable(Table *tablePtr, Blt_DBuffer dbuffer)
{
    if ((tablePtr->padX.side1 != TABLE_DEF_PAD) ||
        (tablePtr->padX.side2 != TABLE_DEF_PAD)) {
        Blt_DBuffer_Format(dbuffer, " -padx {%d %d}", tablePtr->padX.side1, 
                           tablePtr->padX.side2);
    }
    if ((tablePtr->padY.side1 != TABLE_DEF_PAD) ||
        (tablePtr->padY.side2 != TABLE_DEF_PAD)) {
        Blt_DBuffer_Format(dbuffer, " -pady {%d %d}", tablePtr->padY.side1, 
                           tablePtr->padY.side2);
    }
    if (!tablePtr->propagate) {
        Blt_DBuffer_Format(dbuffer, " -propagate no");
    }
    if ((tablePtr->reqWidth.min != LIMITS_MIN) ||
        (tablePtr->reqWidth.nom != LIMITS_NOM) ||
        (tablePtr->reqWidth.max != LIMITS_MAX)) {
        Blt_DBuffer_Format(dbuffer, " -reqwidth {%s}",
                           NameOfLimits(&tablePtr->reqWidth));
    }
    if ((tablePtr->reqHeight.min != LIMITS_MIN) ||
        (tablePtr->reqHeight.nom != LIMITS_NOM) ||
        (tablePtr->reqHeight.max != LIMITS_MAX)) {
        Blt_DBuffer_Format(dbuffer, " -reqheight {%s}",
                           NameOfLimits(&tablePtr->reqHeight));
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyPartitions --
 *
 *      Clear each of the lists managing the entries.  The entries in the
 *      lists of row and column spans are themselves lists which need to be
 *      cleared.
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
 *      This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 *      clean up the Table structure at a safe time (when no-one is using
 *      it anymore).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the table geometry manager is freed up.
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
        TableEntry *entryPtr;

        next = Blt_Chain_NextLink(link);
        entryPtr = Blt_Chain_GetValue(link);
        DestroyEntry(entryPtr);
    }
    Blt_Chain_Destroy(tablePtr->chain);

    DestroyPartitions(&tablePtr->rows);
    DestroyPartitions(&tablePtr->columns);
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
 *      Adds the entry to the lists of both row and column spans.  The
 *      layout of the table is done in order of partition spans, from
 *      shorted to longest.  The widgets spanning a particular number of
 *      partitions are stored in a linked list.  Each list is in turn,
 *      contained within a master list.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The entry is added to both the lists of row and columns spans.  This
 *      will effect the layout of the widgets.
 *
 *---------------------------------------------------------------------------
 */
static void
BinEntry(Table *tablePtr, TableEntry *entryPtr)
{
    Blt_ListNode node;
    Blt_List list;
    Blt_Chain chain;
    size_t key;

    /*
     * Remove the entry from both row and column lists.  It will be
     * re-inserted into the table at the new position.
     */
    if (entryPtr->column.link != NULL) {
        Blt_Chain_UnlinkLink(entryPtr->column.chain,
            entryPtr->column.link);
    }
    if (entryPtr->row.link != NULL) {
        Blt_Chain_UnlinkLink(entryPtr->row.chain, entryPtr->row.link);
    }
    list = tablePtr->rows.list;
    key = 0;                    /* Initialize key to bogus span */
    for (node = Blt_List_FirstNode(list); node != NULL;
        node = Blt_List_NextNode(node)) {

        key = (size_t)Blt_List_GetKey(node);
        if (entryPtr->row.span <= key) {
            break;
        }
    }
    if (key != entryPtr->row.span) {
        Blt_ListNode newNode;

        /*
         * Create a new list (bucket) to hold entries of that size span and
         * and link it into the list of buckets.
         */
        newNode = Blt_List_CreateNode(list, (char *)(uintptr_t)entryPtr->row.span);
        Blt_List_SetValue(newNode, (char *)Blt_Chain_Create());
        Blt_List_LinkBefore(list, newNode, node);
        node = newNode;
    }
    chain = Blt_List_GetValue(node);
    if (entryPtr->row.link == NULL) {
        entryPtr->row.link = Blt_Chain_Append(chain, entryPtr);
    } else {
        Blt_Chain_LinkAfter(chain, entryPtr->row.link, NULL);
    }
    entryPtr->row.chain = chain;

    list = tablePtr->columns.list;
    key = 0;
    for (node = Blt_List_FirstNode(list); node != NULL;
        node = Blt_List_NextNode(node)) {
        key = (size_t)Blt_List_GetKey(node);
        if (entryPtr->column.span <= key) {
            break;
        }
    }
    if (key != entryPtr->column.span) {
        Blt_ListNode newNode;

        /*
         * Create a new list (bucket) to hold entries of that size span and
         * and link it into the list of buckets.
         */
        newNode = Blt_List_CreateNode(list,
                     (char *)(intptr_t)entryPtr->column.span);
        Blt_List_SetValue(newNode, (char *)Blt_Chain_Create());
        Blt_List_LinkBefore(list, newNode, node);
        node = newNode;
    }
    chain = Blt_List_GetValue(node);

    /* Add the new entry to the span bucket */
    if (entryPtr->column.link == NULL) {
        entryPtr->column.link = Blt_Chain_Append(chain, entryPtr);
    } else {
        Blt_Chain_LinkAfter(chain, entryPtr->column.link, NULL);
    }
    entryPtr->column.chain = chain;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseIndex --
 *
 *      Parse the entry index string and return the row and column numbers in
 *      their respective parameters.  The format of a table entry index is
 *      row,column where row is the row number and column is the column
 *      number.  Rows and columns are numbered starting from zero.
 *
 * Results:
 *      Returns a standard TCL result.  If TCL_OK is returned, the row and
 *      column numbers are returned via rowPtr and colPtr respectively.
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
    result = ((Tcl_ExprLong(interp, string, &row) == TCL_OK) &&
              (Tcl_ExprLong(interp, comma + 1, &column) == TCL_OK));
    *comma = ',';               /* Repair the argument */
    if (!result) {
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
 *      Inserts the given widget into the table at a given row and column
 *      position.  The widget can already be managed by this or another table.
 *      The widget will be simply moved to the new location in this table.
 *
 *      The new widget is inserted into both a hash table (this is used to
 *      locate the information associated with the widget) and a list (used to
 *      indicate relative ordering of widgets).
 *
 * Results:
 *      Returns a standard TCL result.  If an error occurred, TCL_ERROR is
 *      returned and an error message is left in interp->result.
 *
 * Side Effects:
 *      The table is re-computed and arranged at the next idle point.
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
    TableEntry *entryPtr;
    int result = TCL_OK;

    entryPtr = FindEntry(tablePtr, tkwin);
    if ((entryPtr != NULL) && (entryPtr->tablePtr != tablePtr)) {
        /* The entry for the widget already exists. If it's managed by another
         * table, delete it.  */
        DestroyEntry(entryPtr);
        entryPtr = NULL;
    }
    if (entryPtr == NULL) {
        entryPtr = CreateEntry(tablePtr, tkwin);
        if (entryPtr == NULL) {
            return TCL_ERROR;
        }
    }
    if (objc > 0) {
        result = Blt_ConfigureWidgetFromObj(tablePtr->interp, entryPtr->tkwin,
            entryConfigSpecs, objc, objv, (char *)entryPtr,
            BLT_CONFIG_OBJV_ONLY);
    }
    if ((entryPtr->column.span < 1) || (entryPtr->row.span < 1)) {
        Tcl_AppendResult(tablePtr->interp, "bad span specified for \"",
            Tk_PathName(tkwin), "\"", (char *)NULL);
        DestroyEntry(entryPtr);
        return TCL_ERROR;
    }
    entryPtr->column.rcPtr = InitSpan(&tablePtr->columns, column, entryPtr->column.span);
    entryPtr->row.rcPtr = InitSpan(&tablePtr->rows, row, entryPtr->row.span);
    /* Insert the entry into both the row and column layout lists */
    BinEntry(tablePtr, entryPtr);

    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * BuildTable --
 *
 *      Processes an objv/objc list of table entries to add and configure new
 *      widgets into the table.  A table entry consists of the widget path
 *      name, table index, and optional configuration options.  The first
 *      argument in the objv list is the name of the table.  If no table
 *      exists for the given widget, a new one is created.
 *
 * Results:
 *      Returns a standard TCL result.  If an error occurred,
 *      TCL_ERROR is returned and an error message is left in
 *      interp->result.
 *
 * Side Effects:
 *      Memory is allocated, a new table is possibly created, etc.
 *      The table is re-computed and arranged at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static int
BuildTable(
    Table *tablePtr,            /* Table to manage new widgets */
    Tcl_Interp *interp,         /* Interpreter to report errors back to */
    int objc,                   /*  */
    Tcl_Obj *const *objv)       /* List of widgets, indices, and options */
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
    nextRow = NumRows(tablePtr);
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
                        return TCL_ERROR;       /* Invalid row,column index */
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
 *      Parses a string representing an item in the table.  An item may be one
 *      of the following:
 *              Rn      - Row index, where n is the index of row
 *              Cn      - Column index, where n is the index of column
 *              r,c     - Cell index, where r is the row index and c
 *                        is the column index.
 *
 * Results:
 *      Returns a standard TCL result.  If no error occurred, TCL_OK is
 *      returned.  *RowPtr* will return the row index.  *ColumnPtr* will
 *      return the column index.  If the row or column index is not
 *      applicable, -1 is returned via *rowPtr* or *colPtr*.
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
        if ((partNum < 0) || (partNum >= NumRows(tablePtr))) {
            Tcl_AppendResult(tablePtr->interp, "row index \"", string,
                "\" is out of range", (char *)NULL);
            return TCL_ERROR;
        }
        *rowPtr = (int)partNum;
    } else if (c == 'c') {
        if (Tcl_ExprLong(tablePtr->interp, string + 1, &partNum) != TCL_OK) {
            return TCL_ERROR;
        }
        if ((partNum < 0) || (partNum >= NumColumns(tablePtr))) {
            Tcl_AppendResult(tablePtr->interp, "column index \"", string,
                "\" is out of range", (char *)NULL);
            return TCL_ERROR;
        }
        *colPtr = (int)partNum;
    } else {
        if (ParseIndex(tablePtr->interp, string, rowPtr, colPtr) != TCL_OK) {
            return TCL_ERROR;   /* Invalid row,column index */
        }
        if ((*rowPtr < 0) || (*rowPtr >= NumRows(tablePtr)) ||
            (*colPtr < 0) || (*colPtr >= NumColumns(tablePtr))) {
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
    int dx, int dy,             /* Difference between outer and inner
                                 * regions */
    Tk_Anchor anchor,           /* Direction of the anchor */
    int *xPtr, int *yPtr)
{
    int x, y;

    x = y = 0;
    switch (anchor) {
    case TK_ANCHOR_NW:          /* Upper left corner */
        break;
    case TK_ANCHOR_W:           /* Left center */
        y = (dy / 2);
        break;
    case TK_ANCHOR_SW:          /* Lower left corner */
        y = dy;
        break;
    case TK_ANCHOR_N:           /* Top center */
        x = (dx / 2);
        break;
    case TK_ANCHOR_CENTER:      /* Centered */
        x = (dx / 2);
        y = (dy / 2);
        break;
    case TK_ANCHOR_S:           /* Bottom center */
        x = (dx / 2);
        y = dy;
        break;
    case TK_ANCHOR_NE:          /* Upper right corner */
        x = dx;
        break;
    case TK_ANCHOR_E:           /* Right center */
        x = dx;
        y = (dy / 2);
        break;
    case TK_ANCHOR_SE:          /* Lower right corner */
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
 *      Returns the width requested by the widget starting in the given entry.
 *      The requested space also includes any internal padding which has been
 *      designated for this widget.
 *
 *      The requested width of the widget is always bounded by the limits set
 *      in entryPtr->reqWidth.
 *
 * Results:
 *      Returns the requested width of the widget.
 *
 *---------------------------------------------------------------------------
 */
static int
GetReqWidth(TableEntry *entryPtr)
{
    int width;

    width = Tk_ReqWidth(entryPtr->tkwin) + (2 * entryPtr->iPadX);
    width = GetBoundedWidth(width, &entryPtr->reqWidth);
    return width;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetReqHeight --
 *
 *      Returns the height requested by the widget starting in the given
 *      entry.  The requested space also includes any internal padding which
 *      has been designated for this widget.
 *
 *      The requested height of the widget is always bounded by the limits set
 *      in entryPtr->reqHeight.
 *
 * Results:
 *      Returns the requested height of the widget.
 *
 *---------------------------------------------------------------------------
 */
static int
GetReqHeight(TableEntry *entryPtr)
{
    int height;

    height = Tk_ReqHeight(entryPtr->tkwin) + (2 * entryPtr->iPadY);
    height = GetBoundedHeight(height, &entryPtr->reqHeight);
    return height;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTotalSpan --
 *
 *      Sums the row/column space requirements for the entire table.
 *
 * Results:
 *      Returns the space currently used in the span of partitions.
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
        RowColumn *rcPtr;               /* Start of partitions */

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
 *      Determines the space used by rows/columns for an entry.
 *
 * Results:
 *      Returns the space currently used in the span of partitions.
 *
 *---------------------------------------------------------------------------
 */
static int
GetSpan(PartitionInfo *piPtr, TableEntry *entryPtr)
{
    RowColumn *startPtr;
    int spaceUsed;
    int count;
    Blt_ChainLink link;
    RowColumn *rcPtr;           /* Start of partitions */
    int span;                   /* Number of partitions spanned */

    if (piPtr->type == rowUid) {
        rcPtr = entryPtr->row.rcPtr;
        span = entryPtr->row.span;
    } else {
        rcPtr = entryPtr->column.rcPtr;
        span = entryPtr->column.span;
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
 *      Expand the span by the amount of the extra space needed.  This
 *      procedure is used in LayoutPartitions to grow the partitions to their
 *      minimum nominal size, starting from a zero width and height space.
 *
 *      This looks more complicated than it really is.  The idea is to make
 *      the size of the partitions correspond to the smallest entry spans.
 *      For example, if widget A is in column 1 and widget B spans both
 *      columns 0 and 1, any extra space needed to fit widget B should come
 *      from column 0.
 *
 *      On the first pass we try to add space to partitions which have not
 *      been touched yet (i.e. have no nominal size).  Since the row and
 *      column lists are sorted in ascending order of the number of rows or
 *      columns spanned, the space is distributed amongst the smallest spans
 *      first.
 *
 *      The second pass handles the case of widgets which have the same span.
 *      For example, if A and B, which span the same number of partitions are
 *      the only widgets to span column 1, column 1 would grow to contain the
 *      bigger of the two slices of space.
 *
 *      If there is still extra space after the first two passes, this means
 *      that there were no partitions of with no widget spans or the same
 *      order span that could be expanded. The third pass will try to remedy
 *      this by parcelling out the left over space evenly among the rest of
 *      the partitions.
 *
 *      On each pass, we have to keep iterating over the span, evenly doling
 *      out slices of extra space, because we may hit partition limits as
 *      space is donated.  In addition, if there are left over pixels because
 *      of round-off, this will distribute them as evenly as possible.  For
 *      the worst case, it will take *span* passes to expand the span.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The partitions in the span may be expanded.
 *
 *---------------------------------------------------------------------------
 */
static void
GrowSpan(
    Table *tablePtr, 
    PartitionInfo *piPtr,
    TableEntry *entryPtr,
    int growth)                 /* The amount of extra space needed to grow
                                 * the span. */
{
    Blt_ChainLink link;
    int numOpen;                        /* # of partitions with space available */
    int n;
    RowColumn *startPtr;        /* Starting (column/row) partition  */
    int span;                   /* Number of partitions in the span */

    if (piPtr->type == rowUid) {
        startPtr = entryPtr->row.rcPtr;
        span = entryPtr->row.span;
    } else {
        startPtr = entryPtr->column.rcPtr;
        span = entryPtr->column.span;
    }

    /*
     * Pass 1: First add space to rows/columns that haven't determined
     *         their nominal sizes yet.
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
                rcPtr->control = entryPtr;
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
                rcPtr->control = entryPtr;
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
                rcPtr->control = entryPtr;
            }
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GrowPartitions --
 *
 *      Grow the span by the designated amount.  Size constraints on the
 *      partitions may prevent any or all of the spacing adjustments.
 *
 *      This is very much like the GrowSpan procedure, but in this case we are
 *      expanding all the (row or column) partitions. It uses a two pass
 *      approach, first giving space to partitions which are smaller than
 *      their nominal sizes. This is because constraints on the partitions may
 *      cause resizing to be non-linear.
 *
 *      If there is still extra space, this means that all partitions are at
 *      least to their nominal sizes.  The second pass will try to add the
 *      left over space evenly among all the partitions which still have space
 *      available (i.e. haven't reached their specified max sizes).
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The size of the partitions may be increased.
 *
 *---------------------------------------------------------------------------
 */
static void
GrowPartitions(
    PartitionInfo *piPtr,       /* Array of (column/row) partitions  */
    int adjustment)             /* The amount of extra space to grow the
                                 * span. If negative, it represents the amount
                                 * of space to add. */
{
    int delta;                  /* Amount of space needed */
    int numAdjust;              /* Number of rows/columns that still can be
                                 * adjusted. */
    Blt_Chain chain;
    Blt_ChainLink link;
    float totalWeight;

    chain = piPtr->chain;

    /*
     * Pass 1: First adjust the size of rows/columns that still haven't
     *        reached their nominal size.
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
        int ration;             /* Amount of space to add to each
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
                int avail;      /* Amount of space still available. */

                avail = rcPtr->nom - rcPtr->size;
                if (avail > 0) {
                    int size;   /* Amount of space requested for a particular
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
        int ration;             /* Amount of space to add to each
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
                int avail;      /* Amount of space still available */

                avail = (rcPtr->max - rcPtr->size);
                if (avail > 0) {
                    int size;   /* Amount of space requested for a particular
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
 *      Shrink the span by the amount specified.  Size constraints on the
 *      partitions may prevent any or all of the spacing adjustments.
 *
 *      This is very much like the GrowSpan procedure, but in this case we are
 *      shrinking or expanding all the (row or column) partitions. It uses a
 *      two pass approach, first subtracting space to partitions which are
 *      larger than their nominal sizes. This is because constraints on the
 *      partitions may cause resizing to be non-linear.
 *
 *      After pass 1, if there is still extra to be removed, this means that
 *      all partitions are at least to their nominal sizes.  The second pass
 *      will try to remove the extra space evenly among all the partitions
 *      which still have space available (i.e haven't reached their respective
 *      min sizes).
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The size of the partitions may be decreased.
 *
 *---------------------------------------------------------------------------
 */
static void
ShrinkPartitions(
    PartitionInfo *piPtr,       /* Array of (column/row) partitions  */
    int adjustment)             /* The amount of extra space to shrink the
                                 * span. It represents the amount of space to
                                 * remove. */
{
    Blt_ChainLink link;
    int extra;                  /* Amount of space needed */
    int numAdjust;              /* Number of rows/columns that still can be
                                 * adjusted. */
    Blt_Chain chain;
    float totalWeight;

    chain = piPtr->chain;

    /*
     * Pass 1: First adjust the size of rows/columns that still aren't
     *         at their nominal size.
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
        int ration;             /* Amount of space to subtract from each
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
                int avail;      /* Amount of space still available */

                avail = rcPtr->size - rcPtr->nom;
                if (avail > 0) {
                    int slice;  /* Amount of space requested for a particular
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
     *         are bigger than their minimum size).
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
        int ration;             /* Amount of space to subtract from each
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
                int avail;      /* Amount of space still available */

                avail = rcPtr->size - rcPtr->min;
                if (avail > 0) {
                    int slice;  /* Amount of space requested for a particular
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
 *      Sets/resets the size of each row and column partition to the minimum
 *      limit of the partition (this is usually zero). This routine gets
 *      called when new widgets are added, deleted, or resized.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The size of each partition is re-initialized to its minimum size.
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
 *      Sets the normal sizes for each partition.  The partition size is the
 *      requested widget size plus an amount of padding.  In addition, adjust
 *      the min/max bounds of the partition depending upon the resize flags
 *      (whether the partition can be expanded or shrunk from its normal
 *      size).
 *
 * Results:
 *      Returns the total space needed for the all the partitions.
 *
 * Side Effects:
 *      The nominal size of each partition is set.  This is later used to
 *      determine how to shrink or grow the table if the container can't be
 *      resized to accommodate the exact size requirements of all the
 *      partitions.
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
 *      Sets the maximum size of a row or column, if the partition has a
 *      widget that controls it.
 *
 * Results:
 *      None.
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
 *      Calculates the normal space requirements for both the row and column
 *      partitions.  Each widget is added in order of the number of rows or
 *      columns spanned, which defines the space needed among in the
 *      partitions spanned.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The sum of normal sizes set here will be used as the normal size for
 *      the container widget.
 *
 *---------------------------------------------------------------------------
 */
static void
LayoutPartitions(Table *tablePtr)
{
    Blt_ListNode node;
    int total;
    PartitionInfo *piPtr;

    piPtr = &tablePtr->columns;

    ResetPartitions(tablePtr, piPtr, GetBoundedWidth);

    for (node = Blt_List_FirstNode(piPtr->list); node != NULL;
        node = Blt_List_NextNode(node)) {
        Blt_Chain chain;
        Blt_ChainLink link;

        chain = Blt_List_GetValue(node);

        for (link = Blt_Chain_FirstLink(chain); link != NULL;
            link = Blt_Chain_NextLink(link)) {
            TableEntry *entryPtr;
            int needed, used;

            entryPtr = Blt_Chain_GetValue(link);
            if (entryPtr->column.control != CONTROL_FULL) {
                continue;
            }
            needed = GetReqWidth(entryPtr) + PADDING(entryPtr->padX) +
                2 * (entryPtr->borderWidth + tablePtr->eEntryPad);
            if (needed <= 0) {
                continue;
            }
            used = GetSpan(piPtr, entryPtr);
            if (needed > used) {
                GrowSpan(tablePtr, piPtr, entryPtr, needed - used);
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
            TableEntry *entryPtr;
            int needed, used;

            entryPtr = Blt_Chain_GetValue(link);

            needed = GetReqWidth(entryPtr) + PADDING(entryPtr->padX) +
                2 * (entryPtr->borderWidth + tablePtr->eEntryPad);

            if (entryPtr->column.control >= 0.0) {
                needed = (int)(needed * entryPtr->column.control);
            }
            if (needed <= 0) {
                continue;
            }
            used = GetSpan(piPtr, entryPtr);
            if (needed > used) {
                GrowSpan(tablePtr, piPtr, entryPtr, needed - used);
            }
        }
    }
    total = SetNominalSizes(tablePtr, piPtr);
    tablePtr->normal.width = GetBoundedWidth(total, &tablePtr->reqWidth) +
        PADDING(tablePtr->padX) +
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
            TableEntry *entryPtr;
            int needed, used;

            entryPtr = Blt_Chain_GetValue(link);
            if (entryPtr->row.control != CONTROL_FULL) {
                continue;
            }
            needed = GetReqHeight(entryPtr) + PADDING(entryPtr->padY) +
                2 * (entryPtr->borderWidth + tablePtr->eEntryPad);
            if (needed <= 0) {
                continue;
            }
            used = GetSpan(piPtr, entryPtr);
            if (needed > used) {
                GrowSpan(tablePtr, piPtr, entryPtr, needed - used);
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
            TableEntry *entryPtr;
            int needed, used;

            entryPtr = Blt_Chain_GetValue(link);
            needed = GetReqHeight(entryPtr) + PADDING(entryPtr->padY) +
                2 * (entryPtr->borderWidth + tablePtr->eEntryPad);
            if (entryPtr->row.control >= 0.0) {
                needed = (int)(needed * entryPtr->row.control);
            }
            if (needed <= 0) {
                continue;
            }
            used = GetSpan(piPtr, entryPtr);
            if (needed > used) {
                GrowSpan(tablePtr, piPtr, entryPtr, needed - used);
            }
        }
    }
    total = SetNominalSizes(tablePtr, piPtr);
    tablePtr->normal.height = GetBoundedHeight(total, &tablePtr->reqHeight) +
        PADDING(tablePtr->padY) +
        2 * (tablePtr->eTablePad + Tk_InternalBorderWidth(tablePtr->tkwin));
}

/*
 *---------------------------------------------------------------------------
 *
 * ArrangeEntries
 *
 *      Places each widget at its proper location.  First determines the size
 *      and position of the each widget.  It then considers the following:
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
 *      The size of each partition is re-initialized its minimum size.
 *
 *---------------------------------------------------------------------------
 */
static void
ArrangeEntries(Table *tablePtr)         /* Table widget structure */
{
    Blt_ChainLink link;
    int xMax, yMax;

    xMax = tablePtr->container.width -
        (Tk_InternalBorderWidth(tablePtr->tkwin) + tablePtr->padX.side2 +
        tablePtr->eTablePad);
    yMax = tablePtr->container.height -
        (Tk_InternalBorderWidth(tablePtr->tkwin) + tablePtr->padY.side2 +
        tablePtr->eTablePad);

    for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        TableEntry *entryPtr;
        int dx, dy;
        int extra;
        int spanWidth, spanHeight;
        int winWidth, winHeight;
        int x, y;

        entryPtr = Blt_Chain_GetValue(link);

        x = entryPtr->column.rcPtr->offset +
            entryPtr->column.rcPtr->pad.side1 +
            entryPtr->padX.side1 +
            Tk_Changes(entryPtr->tkwin)->border_width +
            tablePtr->eEntryPad;
        y = entryPtr->row.rcPtr->offset +
            entryPtr->row.rcPtr->pad.side1 +
            entryPtr->padY.side1 +
            Tk_Changes(entryPtr->tkwin)->border_width +
            tablePtr->eEntryPad;

        /*
         * Unmap any widgets that start beyond of the right edge of the
         * container.
         */
        if ((x >= xMax) || (y >= yMax)) {
            if (Tk_IsMapped(entryPtr->tkwin)) {
                if (Tk_Parent(entryPtr->tkwin) != tablePtr->tkwin) {
                    Tk_UnmaintainGeometry(entryPtr->tkwin, tablePtr->tkwin);
                }
                Tk_UnmapWindow(entryPtr->tkwin);
            }
            continue;
        }
        extra = 2 * (entryPtr->borderWidth + tablePtr->eEntryPad);
        spanWidth = GetSpan(&tablePtr->columns, entryPtr) -
            (extra + PADDING(entryPtr->padX));
        spanHeight = GetSpan(&tablePtr->rows, entryPtr) - 
            (extra + PADDING(entryPtr->padY));

        winWidth = GetReqWidth(entryPtr);
        winHeight = GetReqHeight(entryPtr);

        /*
         *
         * Compare the widget's requested size to the size of the span.
         *
         * 1) If the widget is larger than the span or if the fill flag is
         *    set, make the widget the size of the span. Check that the new
         *    size is within the bounds set for the widget.
         *
         * 2) Otherwise, position the widget in the space according to its
         *    anchor.
         *
         */
        if ((spanWidth <= winWidth) || (entryPtr->fill & FILL_X)) {
            winWidth = spanWidth;
            if (winWidth > entryPtr->reqWidth.max) {
                winWidth = entryPtr->reqWidth.max;
            }
        }
        if ((spanHeight <= winHeight) || (entryPtr->fill & FILL_Y)) {
            winHeight = spanHeight;
            if (winHeight > entryPtr->reqHeight.max) {
                winHeight = entryPtr->reqHeight.max;
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
            TranslateAnchor(dx, dy, entryPtr->anchor, &x, &y);
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
            if (Tk_IsMapped(entryPtr->tkwin)) {
                if (tablePtr->tkwin != Tk_Parent(entryPtr->tkwin)) {
                    Tk_UnmaintainGeometry(entryPtr->tkwin, tablePtr->tkwin);
                }
                Tk_UnmapWindow(entryPtr->tkwin);
            }
            continue;
        }

        /*
         * Resize and/or move the widget as necessary.
         */
        entryPtr->x = x;
        entryPtr->y = y;

#ifdef notdef
        fprintf(stderr, "ArrangeEntries: %s rw=%d rh=%d w=%d h=%d\n",
                Tk_PathName(entryPtr->tkwin), Tk_ReqWidth(entryPtr->tkwin),
                Tk_ReqHeight(entryPtr->tkwin), winWidth, winHeight);
#endif
        if (tablePtr->tkwin != Tk_Parent(entryPtr->tkwin)) {
            Tk_MaintainGeometry(entryPtr->tkwin, tablePtr->tkwin, x, y,
                winWidth, winHeight);
        } else {
            if ((x != Tk_X(entryPtr->tkwin)) || (y != Tk_Y(entryPtr->tkwin)) ||
                (winWidth != Tk_Width(entryPtr->tkwin)) ||
                (winHeight != Tk_Height(entryPtr->tkwin))) {
#ifdef notdef
                fprintf(stderr, "ArrangeEntries: %s rw=%d rh=%d w=%d h=%d\n",
                        Tk_PathName(entryPtr->tkwin), Tk_ReqWidth(entryPtr->tkwin),
                        Tk_ReqHeight(entryPtr->tkwin), winWidth, winHeight);
#endif
                Tk_MoveResizeWindow(entryPtr->tkwin, x, y, winWidth, winHeight);
            }
            if (!Tk_IsMapped(entryPtr->tkwin)) {
                Tk_MapWindow(entryPtr->tkwin);
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
 *      None.
 *
 * Side Effects:
 *      The widgets in the table are possibly resized and redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
ArrangeTable(ClientData clientData)
{
    Table *tablePtr = clientData;
    int width, height;
    int offset, delta;
    int padX, padY;
    int outerPad;
    Blt_ChainLink link;

#ifdef notdef
    fprintf(stderr, "ArrangeTable(%s)\n", Tk_PathName(tablePtr->tkwin));
#endif
    Tcl_Preserve(tablePtr);
    tablePtr->flags &= ~ARRANGE_PENDING;

    tablePtr->rows.ePad = tablePtr->columns.ePad = tablePtr->eTablePad =
        tablePtr->eEntryPad = 0;
    if (tablePtr->editPtr != NULL) {
        tablePtr->rows.ePad = tablePtr->columns.ePad =
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
    padX = outerPad + tablePtr->columns.ePad + PADDING(tablePtr->padX);
    padY = outerPad + tablePtr->rows.ePad    + PADDING(tablePtr->padY);

    width  = GetTotalSpan(&tablePtr->columns) + padX;
    height = GetTotalSpan(&tablePtr->rows) + padY;

    /*
     * If the previous geometry request was not fulfilled (i.e. the size of
     * the container is different from partitions' space requirements), try to
     * adjust size of the partitions to fit the widget.
     */
    delta = tablePtr->container.width - width;
    if (delta != 0) {
        if (delta > 0) {
            GrowPartitions(&tablePtr->columns, delta);
        } else {
            ShrinkPartitions(&tablePtr->columns, delta);
        }
        width = GetTotalSpan(&tablePtr->columns) + padX;
    }
    delta = tablePtr->container.height - height;
    if (delta != 0) {
        if (delta > 0) {
            GrowPartitions(&tablePtr->rows, delta);
        } else {
            ShrinkPartitions(&tablePtr->rows, delta);
        }
        height = GetTotalSpan(&tablePtr->rows) + padY;
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

    offset = Tk_InternalBorderWidth(tablePtr->tkwin) + tablePtr->padX.side1 +
        tablePtr->eTablePad;
    if (width < tablePtr->container.width) {
        offset += (tablePtr->container.width - width) / 2;
    }
    for (link = Blt_Chain_FirstLink(tablePtr->columns.chain); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        RowColumn *colPtr;

        colPtr = Blt_Chain_GetValue(link);
        colPtr->offset = offset + tablePtr->columns.ePad;
        offset += colPtr->size;
    }

    offset = Tk_InternalBorderWidth(tablePtr->tkwin) + tablePtr->padY.side1 +
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
 *      Forces layout of the table geometry manager.  This is useful mostly
 *      for debugging the geometry manager.  You can get the geometry manager
 *      to calculate the normal (requested) width and height of each row and
 *      column.  Otherwise, you need to first withdraw the container widget,
 *      invoke "update", and then query the geometry manager.
 *
 * Results:
 *      Returns a standard TCL result.  If the table is successfully
 *      rearranged, TCL_OK is returned. Otherwise, TCL_ERROR is returned and
 *      an error message is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ArrangeOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv) 
{
    TableInterpData *dataPtr = clientData;
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
 *      Returns the name, position and options of a widget in the table.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the widget attributes is
 *      left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
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
    if (c == '.') {             /* Configure widget */
        TableEntry *entryPtr;

        if (GetEntry(interp, tablePtr, Tcl_GetString(objv[3]), &entryPtr) 
            != TCL_OK) {
            return TCL_ERROR;
        }
        return Blt_ConfigureValueFromObj(interp, entryPtr->tkwin, 
                entryConfigSpecs, (char *)entryPtr, objv[4], 0);
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
 * ColumnCgetOp --
 *
 *      Returns the name, position and options of a widget in the table.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the widget attributes is
 *      left in interp->result.
 *
 *      table column cget contName columnIndex option
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnCgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    PartitionInfo *piPtr;
    Table *tablePtr;
    long colIndex;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_GetCountFromObj(interp, objv[4], COUNT_NNEG, &colIndex) != TCL_OK) {
        return TCL_ERROR;
    }
    piPtr = &tablePtr->columns;
    return Blt_ConfigureValueFromObj(interp, tablePtr->tkwin, 
        piPtr->configSpecs, (char *)GetRowColumn(piPtr, colIndex), objv[5], 0);
}

static int
ConfigureColumn(Tcl_Interp *interp, Table *tablePtr, Tcl_Obj *patternObjPtr,
             int objc, Tcl_Obj *const *objv)
{
    Blt_ChainLink link;
    int numMatches;
    PartitionInfo *piPtr = &tablePtr->columns;
    const char *pattern;

    pattern = Tcl_GetString(patternObjPtr);
    numMatches = 0;
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        RowColumn *rcPtr;
        char string[200];

        rcPtr = Blt_Chain_GetValue(link);
        Blt_FmtString(string, 200, "%d", rcPtr->index);
        if (Tcl_StringMatch(string, pattern)) {
            if (Blt_ConfigureWidgetFromObj(interp, tablePtr->tkwin,
                piPtr->configSpecs, objc, objv, (char *)rcPtr,
                        BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
                return TCL_ERROR;
            }
            numMatches++;
        }
    }
    if (numMatches == 0) {
        int colIndex;
        RowColumn *rcPtr;

        /* 
         * We found no existing partitions matching this pattern, so see if
         * this designates an new partition (one beyond the current range).
         */
        if ((Tcl_GetIntFromObj(NULL, patternObjPtr, &colIndex) != TCL_OK) ||
            (colIndex < 0)) {
            Tcl_AppendResult(interp, "pattern \"", pattern, 
                "\" matches no column in table \"", Tk_PathName(tablePtr->tkwin),
                "\"", (char *)NULL);
            return TCL_ERROR;
        }
        rcPtr = GetRowColumn(piPtr, colIndex);
        assert(rcPtr);
        if (Blt_ConfigureWidgetFromObj(interp, tablePtr->tkwin,
                piPtr->configSpecs, objc, objv, (char *)rcPtr,
                BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnConfigureOp --
 *
 *      Returns the name, position and options of a widget in the table.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the table configuration
 *      option information is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    PartitionInfo *piPtr;
    Table *tablePtr;
    TableInterpData *dataPtr = clientData;
    Tcl_Obj *const *columns;
    int i, numCols;
    int result;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    /*
     * Find the end of the items. Search until we see an option (-).
     */
    objc -= 3, objv += 3;
    for (numCols = 0; numCols < objc; numCols++) {
        const char *string;
        
        string = Tcl_GetString(objv[numCols]);
        if (string[0] == '-') {
            break;
        }
    }
    columns = objv;                     /* Save the start of the item
                                         * list */
    objc -= numCols;                    /* Move beyond the columns to the
                                         * options */
    objv += numCols;
    result = TCL_ERROR;                 /* Suppress compiler warning */

    piPtr = &tablePtr->columns;
    if (numCols == 0) {
        return TCL_OK;
    }
    if ((objc == 0) || (objc == 1)) {
        RowColumn *colPtr;
        
        if (numCols > 1) {
            
        }
        if (GetColumnFromObj(interp, tablePtr, columns[0], &colPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (objc == 0) {
            return Blt_ConfigureInfoFromObj(interp, tablePtr->tkwin,
                piPtr->configSpecs, (char *)colPtr, (Tcl_Obj *)NULL, 0);
        } else {
            return Blt_ConfigureInfoFromObj(interp, tablePtr->tkwin,
                piPtr->configSpecs, (char *)colPtr, objv[0], 0);
        }
    }
    for (i = 0; i < numCols; i++) {
        if (ConfigureColumn(interp, tablePtr, columns[i], objc, objv)
            != TCL_OK) {
            return TCL_ERROR;
        }
    }
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnDeleteOp --
 *
 *      Deletes the specified columns from the table.  
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      blt::table column delete tableName firstIndex lastIndex
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    Table *tablePtr;
    int numMatches;
    PartitionInfo *piPtr;
    Blt_ChainLink link, next;
    RowColumn *firstPtr, *lastPtr;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetColumnFromObj(interp, tablePtr, objv[4], &firstPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    lastPtr = firstPtr;                 /* By default, delete only one
                                         * column. */
    if ((objc > 5) && 
        (GetColumnFromObj(interp, tablePtr, objv[5], &lastPtr) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (firstPtr->index > lastPtr->index) {
        return TCL_OK;                  /* No range defined. */
    }
    piPtr = &tablePtr->columns;
    numMatches = 0;
    for (link = firstPtr->link; link != NULL; link = next) {
        RowColumn *rcPtr;
        
        next = Blt_Chain_NextLink(link);
        rcPtr = Blt_Chain_GetValue(link);
        DeleteRowColumn(tablePtr, piPtr, rcPtr);
        Blt_Chain_DeleteLink(piPtr->chain, link);
        numMatches++;
        if (link == lastPtr->link) {
            break;
        }
    }
    if (numMatches > 0) {               /* Fix indices */
        RenumberIndices(piPtr->chain);
        tablePtr->flags |= REQUEST_LAYOUT;
        EventuallyArrangeTable(tablePtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnExtentsOp --
 *
 *      Returns a list of all the pathnames of the widgets managed by a table.
 *      The table is determined from the name of the container widget
 *      associated with the table.
 *
 * Results:
 *      Returns a standard TCL result.  If no error occurred, TCL_OK is
 *      returned and a list of widgets managed by the table is left in
 *      interp->result.
 *
 *      blt::table extents tableName columnIndex
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnExtentsOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    Table *tablePtr;
    RowColumn *colPtr;
    RowColumn *r1Ptr, *r2Ptr, *c1Ptr, *c2Ptr;
    Tcl_Obj *listObjPtr;
    int x, y, w, h;
    
    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetColumnFromObj(interp, tablePtr, objv[4], &colPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    c1Ptr = c2Ptr = colPtr;
    r1Ptr = GetRowColumn(&tablePtr->rows, 0);
    r2Ptr = GetRowColumn(&tablePtr->rows, NumRows(tablePtr) - 1);
    x = c1Ptr->offset;
    y = r1Ptr->offset;
    w = c2Ptr->offset + c2Ptr->size - x;
    h = r2Ptr->offset + r2Ptr->size - y;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(w));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(h));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ColumnFindOp --
 *
 *
 *      Returns the column index given a screen coordinate.
 *
 *      blt::table column find tableName x
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ColumnFindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    RowColumn *colPtr;
    Table *tablePtr;
    int index, x;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_GetPixelsFromObj(interp, tablePtr->tkwin, objv[4], PIXELS_ANY, &x)
        != TCL_OK) {
        return TCL_ERROR;
    }
    index = -1;
    colPtr = ColumnSearch(tablePtr, x);
    if (colPtr != NULL) {
        index = colPtr->index;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnInfoOp --
 *
 *      Returns the options of the column in the table.
 *
 *      blt::table column info tableName columnIndex
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnInfoOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    Table *tablePtr;
    const char *pattern;
    Blt_ChainLink last, link;
    Blt_DBuffer dbuffer;
    PartitionInfo *piPtr;
    
    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    pattern = Tcl_GetString(objv[4]);
    piPtr = &tablePtr->columns;
    last = Blt_Chain_LastLink(piPtr->chain);
    dbuffer = Blt_DBuffer_Create();
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        RowColumn *rcPtr;
        char string[200];

        rcPtr = Blt_Chain_GetValue(link);
        Blt_FmtString(string, 200, "%d", rcPtr->index);
        if (Tcl_StringMatch(string, pattern)) {
            Blt_DBuffer_Format(dbuffer, "%d", rcPtr->index);
            PrintRowColumn(interp, piPtr, rcPtr, dbuffer);
            if (link != last) {
                Blt_DBuffer_AppendString(dbuffer, " \\\n", 2);
            } else {
                Blt_DBuffer_AppendString(dbuffer, "\n", 1);
            }
        }
    }
    Tcl_SetObjResult(interp, Blt_DBuffer_StringObj(dbuffer));
    Blt_DBuffer_Destroy(dbuffer);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnInsertOp --
 *
 *      Inserts a span of columns into the table.
 *
 *      blt::table column insert tableName ?switches ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnInsertOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{ 
    InsertSwitches switches;
    PartitionInfo *piPtr;
    Table *tablePtr;
    TableInterpData *dataPtr = clientData;
    int i;
    
    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    switches.count = 1;
    if (Blt_ParseSwitches(interp, columnInsertSwitches, objc - 4, objv + 4,
                &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    piPtr = &tablePtr->columns;

    /* Insert the new columns from the designated point in the chain. */
    for (i = 0; i < switches.count; i++) {
        RowColumn *rcPtr;
        Blt_ChainLink link;
        
        rcPtr = CreateRowColumn();
        link = Blt_Chain_NewLink();
        Blt_Chain_SetValue(link, rcPtr);
        if (switches.afterPtr != NULL) {
            Blt_Chain_LinkAfter(piPtr->chain, link, switches.afterPtr->link);
        } else if (switches.beforePtr != NULL) {
            Blt_Chain_LinkBefore(piPtr->chain, link, switches.beforePtr->link);
        } else {
            Blt_Chain_LinkAfter(piPtr->chain, link, NULL);
        }
        rcPtr->link = link;
    }
    RenumberIndices(piPtr->chain);
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnJoinOp --
 *
 *      Joins the specified span of columns together into a partition.
 *
 *      blt::table column join tableName firstIndex lastIndex
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnJoinOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Blt_ChainLink link;
    PartitionInfo *piPtr;
    RowColumn *fromPtr, *toPtr;
    Table *tablePtr;
    TableInterpData *dataPtr = clientData;
    int i;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetColumnFromObj(interp, tablePtr, objv[4], &fromPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetColumnFromObj(interp, tablePtr, objv[5], &toPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (fromPtr->index >= toPtr->index) {
        return TCL_OK;          /* No-op. */
    }
    piPtr = &tablePtr->columns;
    /*
     *  Reduce the span of all entries that currently cross any of the
     *  trailing columns.  Also, if the entry starts in one of these
     *  columns, moved it to the designated "joined" column.
     */
    for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        TableEntry *entryPtr;
        int start, end;         /* Entry indices. */

        entryPtr = Blt_Chain_GetValue(link);
        start = entryPtr->column.rcPtr->index + 1;
        end = entryPtr->column.rcPtr->index + entryPtr->column.span - 1;
        if ((end < fromPtr->index) || ((start > toPtr->index))) {
            continue;
        }
        entryPtr->column.span -= toPtr->index - start + 1;
        if (start >= fromPtr->index) { /* Entry starts in a trailing
                                        * partition. */
            entryPtr->column.rcPtr = fromPtr;
        }
    }
    link = Blt_Chain_NextLink(fromPtr->link);
    for (i = fromPtr->index + 1; i <= toPtr->index; i++) {
        Blt_ChainLink next;
        RowColumn *rcPtr;
        
        next = Blt_Chain_NextLink(link);
        rcPtr = Blt_Chain_GetValue(link);
        DeleteRowColumn(tablePtr, piPtr, rcPtr);
        Blt_Chain_DeleteLink(piPtr->chain, link);
        link = next;
    }
    RenumberIndices(piPtr->chain);
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnSplitOp --
 *
 *      Splits a the designated column into multiple columns. Any widgets
 *      that span this column/column will be automatically corrected to
 *      include the new columns.
 *
 *      blt::table column split tableName colIndex numDivisions
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnSplitOp(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    Blt_ChainLink link;
    PartitionInfo *piPtr;
    RowColumn *colPtr;
    Table *tablePtr;
    TableInterpData *dataPtr = clientData;
    int i, numCols;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetColumnFromObj(interp, tablePtr, objv[4], &colPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    numCols = 2;
    if ((objc > 5) &&
        (Tcl_GetIntFromObj(interp, objv[5], &numCols) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (numCols < 2) {
        Tcl_AppendResult(interp, "bad split value \"", Tcl_GetString(objv[5]),
            "\": should be 2 or greater", (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * Append (split - 1) additional columns starting from the current
     * point in the chain.
     */
    piPtr = &tablePtr->columns;
    for (i = 1; i < numCols; i++) {
        RowColumn *rcPtr;
        Blt_ChainLink link;
        
        rcPtr = CreateRowColumn();
        link = Blt_Chain_NewLink();
        Blt_Chain_SetValue(link, rcPtr);
        Blt_Chain_LinkAfter(piPtr->chain, link, colPtr->link);
        rcPtr->link = link;
    }

    /*
     * Also increase the span of all entries that span this column/column by
     * numCols - 1.
     */
    for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        TableEntry *entryPtr;
        int start, end;

        entryPtr = Blt_Chain_GetValue(link);
        start = entryPtr->column.rcPtr->index;
        end = entryPtr->column.rcPtr->index + entryPtr->column.span;
        if ((start <= colPtr->index) && (colPtr->index < end)) {
            entryPtr->column.span += (numCols - 1);
        }
    }
    RenumberIndices(tablePtr->columns.chain);
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

static Blt_OpSpec columnOps[] =
{
    {"cget",       2, ColumnCgetOp,      6, 6, "tableName columnIndex option",},
    {"configure",  2, ColumnConfigureOp, 4, 0, "tableName columnIndex ?option value ... ?",},
    {"delete", 1, ColumnDeleteOp, 4, 0, "tableName firstIndex ?lastIndex?",},
    {"extents", 1, ColumnExtentsOp,   5, 5, "tableName columnIndex",},
    {"find", 1, ColumnFindOp,      6, 6, "tableName x y",},
    {"info", 3, ColumnInfoOp,      5, 5, "tableName columnIndex",},
    {"insert", 3, ColumnInsertOp,    5, 0, "tableName ?switches ...?",},
    {"join", 1, ColumnJoinOp,      6, 6, "tableName firstColumn lastColumn",},
    {"split", 2, ColumnSplitOp,     5, 6, "tableName columnIndex ?numColumns?",},
};

static int numColumnOps = sizeof(columnOps) / sizeof(Blt_OpSpec);

static int
ColumnOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    
    proc = Blt_GetOpFromObj(interp, numColumnOps, columnOps, BLT_OP_ARG2, 
                objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      Returns the name, position and options of a widget in the table.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the table configuration
 *      option information is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
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
    items = objv;               /* Save the start of the item list */
    objc -= count;              /* Move beyond the items to the options */
    objv += count;
    result = TCL_ERROR;         /* Suppress compiler warning */

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
        if (c1 == '.') {                /* Configure widget */
            TableEntry *entryPtr;

            if (GetEntry(interp, tablePtr, string, &entryPtr) != TCL_OK) {
                return TCL_ERROR;
            }
            result = ConfigureEntry(tablePtr, interp, entryPtr, objc, objv);
        } else if ((c1 == 'r') || (c1 == 'R')) {
            result = ConfigureRowColumn(tablePtr, &tablePtr->rows,
                string, objc, objv);
        } else if ((c1 == 'c') && (c2 == 'o') &&
            (strncmp(string, "container", length) == 0)) {
            result = ConfigureTable(tablePtr, interp, objc, objv);
        } else if ((c1 == 'c') || (c1 == 'C')) {
            result = ConfigureRowColumn(tablePtr, &tablePtr->columns,
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
 * FindOp --
 *
 *
 *      Returns the row,column index given a screen coordinate.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      table locate .t %X %Y
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
FindOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
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
    rowPtr = RowSearch(tablePtr, y);
    if (rowPtr == NULL) {
        return TCL_OK;
    }
    colPtr = ColumnSearch(tablePtr, x);
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
 * ForgetOp --
 *
 *      Processes an objv/objc list of widget names and purges their entries
 *      from their respective tables.  The widgets are unmapped and the tables
 *      are rearranged at the next idle point.  Note that all the named
 *      widgets do not need to exist in the same table.
 *
 * Results:
 *      Returns a standard TCL result.  If an error occurred, TCL_ERROR is
 *      returned and an error message is left in interp->result.
 *
 * Side Effects:
 *      Memory is deallocated (the entry is destroyed), etc.  The affected
 *      tables are is re-computed and arranged at the next idle point.
 *
 *      blt::table forget ?slaveName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
ForgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;
        Table *tablePtr;
        TableEntry *entryPtr;
        Tk_Window tkwin;
        const char *string;

        entryPtr = NULL;
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
            entryPtr = FindEntry(tablePtr, tkwin);
            if (entryPtr != NULL) {
                break;
            }
        }
        if (entryPtr == NULL) {
            Tcl_AppendResult(interp, "\"", string,
                "\" is not managed by any table", (char *)NULL);
            return TCL_ERROR;
        }
        if (Tk_IsMapped(entryPtr->tkwin)) {
            Tk_UnmapWindow(entryPtr->tkwin);
        }
        /* Arrange for the call back here in the loop, because the widgets
         * may not belong to the same table.  */
        tablePtr->flags |= REQUEST_LAYOUT;
        EventuallyArrangeTable(tablePtr);
        DestroyEntry(entryPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InfoOp --
 *
 *      Returns the options of a widget in the table.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the widget attributes is
 *      left in interp->result.
 *
 *      blt::table info tableName pathName
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InfoOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    Table *tablePtr;
    int i;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 3; i < objc; i++) {
        TableEntry *entryPtr;
        const char *string;
        int result;

        string = Tcl_GetString(objv[i]);
        if (GetEntry(interp, tablePtr, string, &entryPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        result = InfoEntry(interp, tablePtr, entryPtr);
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
 * NamesOp --
 *
 *      Returns a list of tables currently in use. A table is associated by
 *      the name of its container widget.  All tables matching a given pattern
 *      are included in this list.  If no pattern is present (objc == 0), all
 *      tables are included.
 *
 * Results:
 *      Returns a standard TCL result.  If no error occurred, TCL_OK is
 *      returned and a list of tables is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
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
 * RowCgetOp --
 *
 *      Returns the name, position and options of a widget in the table.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the widget attributes is
 *      left in interp->result.
 *
 *      table row cget contName rowIndex option
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowCgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    PartitionInfo *piPtr;
    Table *tablePtr;
    long rowIndex;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_GetCountFromObj(interp, objv[4], COUNT_NNEG, &rowIndex) != TCL_OK) {
        return TCL_ERROR;
    }
    piPtr = &tablePtr->rows;
    return Blt_ConfigureValueFromObj(interp, tablePtr->tkwin, 
        piPtr->configSpecs, (char *)GetRowColumn(piPtr, rowIndex), objv[5], 0);
}

static int
ConfigureRow(Tcl_Interp *interp, Table *tablePtr, Tcl_Obj *patternObjPtr,
             int objc, Tcl_Obj *const *objv)
{
    Blt_ChainLink link;
    int numMatches;
    PartitionInfo *piPtr = &tablePtr->rows;
    const char *pattern;

    pattern = Tcl_GetString(patternObjPtr);
    numMatches = 0;
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        RowColumn *rcPtr;
        char string[200];

        rcPtr = Blt_Chain_GetValue(link);
        Blt_FmtString(string, 200, "%d", rcPtr->index);
        if (Tcl_StringMatch(string, pattern)) {
            if (Blt_ConfigureWidgetFromObj(interp, tablePtr->tkwin,
                piPtr->configSpecs, objc, objv, (char *)rcPtr,
                        BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
                return TCL_ERROR;
            }
            numMatches++;
        }
    }
    if (numMatches == 0) {
        int rowIndex;
        RowColumn *rcPtr;

        /* 
         * We found no existing partitions matching this pattern, so see if
         * this designates an new partition (one beyond the current range).
         */
        if ((Tcl_GetIntFromObj(NULL, patternObjPtr, &rowIndex) != TCL_OK) ||
            (rowIndex < 0)) {
            Tcl_AppendResult(interp, "pattern \"", pattern, 
                "\" matches no row in table \"", Tk_PathName(tablePtr->tkwin),
                "\"", (char *)NULL);
            return TCL_ERROR;
        }
        rcPtr = GetRowColumn(piPtr, rowIndex);
        assert(rcPtr);
        if (Blt_ConfigureWidgetFromObj(interp, tablePtr->tkwin,
                piPtr->configSpecs, objc, objv, (char *)rcPtr,
                BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowConfigureOp --
 *
 *      Returns the name, position and options of a widget in the table.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the table configuration
 *      option information is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    PartitionInfo *piPtr;
    Table *tablePtr;
    int i, numRows;
    int result;
    Tcl_Obj *const *rows;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    /*
     * Find the end of the items. Search until we see an option (-).
     */
    objc -= 3, objv += 3;
    for (numRows = 0; numRows < objc; numRows++) {
        const char *string;
        
        string = Tcl_GetString(objv[numRows]);
        if (string[0] == '-') {
            break;
        }
    }
    rows = objv;                /* Save the start of the item list */
    objc -= numRows;                  /* Move beyond the items to the options */
    objv += numRows;
    result = TCL_ERROR;         /* Suppress compiler warning */

    piPtr = &tablePtr->rows;
    if (numRows == 0) {
        return TCL_OK;
    }
    if ((objc == 0) || (objc == 1)) {
        RowColumn *rowPtr;
        
        if (numRows > 1) {
            
        }
        if (GetRowFromObj(interp, tablePtr, rows[0], &rowPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (objc == 0) {
            return Blt_ConfigureInfoFromObj(interp, tablePtr->tkwin,
                piPtr->configSpecs, (char *)rowPtr, (Tcl_Obj *)NULL, 0);
        } else {
            return Blt_ConfigureInfoFromObj(interp, tablePtr->tkwin,
                piPtr->configSpecs, (char *)rowPtr, objv[0], 0);
        }
    }
    for (i = 0; i < numRows; i++) {
        if (ConfigureRow(interp, tablePtr, rows[i], objc, objv) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowDeleteOp --
 *
 *      Deletes the specified rows and/or columns from the table.  Note that
 *      the row/column indices can be fixed only after all the deletions have
 *      occurred.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *
 *      blt::table row delete tableName firstIndex lastIndex
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    Table *tablePtr;
    int numMatches;
    PartitionInfo *piPtr;
    Blt_ChainLink link, next;
    RowColumn *firstPtr, *lastPtr;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetRowFromObj(interp, tablePtr, objv[4], &firstPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    lastPtr = firstPtr;                 /* By default, delete only one
                                         * row. */
    if ((objc > 5) &&
        (GetRowFromObj(interp, tablePtr, objv[5], &lastPtr) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (firstPtr->index > lastPtr->index) {
        return TCL_OK;                  /* No range defined. */
    }
    piPtr = &tablePtr->rows;
    numMatches = 0;
    for (link = firstPtr->link; link != NULL; link = next) {
        RowColumn *rcPtr;
        
        next = Blt_Chain_NextLink(link);
        rcPtr = Blt_Chain_GetValue(link);
        DeleteRowColumn(tablePtr, piPtr, rcPtr);
        Blt_Chain_DeleteLink(piPtr->chain, link);
        numMatches++;
        if (link == lastPtr->link) {
            break;
        }
    }
    if (numMatches > 0) {               /* Fix indices */
        RenumberIndices(piPtr->chain);
        tablePtr->flags |= REQUEST_LAYOUT;
        EventuallyArrangeTable(tablePtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowExtentsOp --
 *
 *      Returns a list of all the pathnames of the widgets managed by a
 *      table.  The table is determined from the name of the container
 *      widget associated with the table.
 *
 * Results:
 *      Returns a standard TCL result.  If no error occurred, TCL_OK is
 *      returned and a list of widgets managed by the table is left in
 *      interp->result.
 *
 *      blt::table extents tableName rowIndex
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowExtentsOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    Table *tablePtr;
    RowColumn *rowPtr;
    RowColumn *r1Ptr, *r2Ptr, *c1Ptr, *c2Ptr;
    Tcl_Obj *listObjPtr;
    int x, y, w, h;
    
    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetRowFromObj(interp, tablePtr, objv[4], &rowPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    r1Ptr = r2Ptr = rowPtr;
    c1Ptr = GetRowColumn(&tablePtr->columns, 0);
    c2Ptr = GetRowColumn(&tablePtr->columns, NumColumns(tablePtr) - 1);
    x = c1Ptr->offset;
    y = r1Ptr->offset;
    w = c2Ptr->offset + c2Ptr->size - x;
    h = r2Ptr->offset + r2Ptr->size - y;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(w));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(h));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * RowFindOp --
 *
 *
 *      Returns the row index given a screen coordinate.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *      blt::table row find tableName y
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
RowFindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    RowColumn *rowPtr;
    Table *tablePtr;
    int index, y;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_GetPixelsFromObj(interp, tablePtr->tkwin, objv[4], PIXELS_ANY, &y)
        != TCL_OK) {
        return TCL_ERROR;
    }
    index = -1;
    rowPtr = RowSearch(tablePtr, y);
    if (rowPtr != NULL) {
        index = rowPtr->index;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowInfoOp --
 *
 *      Returns the options of a widget or partition in the table.
 *
 * Results:
 *      Returns a standard TCL result.  A list of the widget attributes is
 *      left in interp->result.
 *
 *      blt::table row info tableName rowIndex
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowInfoOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    Table *tablePtr;
    const char *pattern;
    Blt_ChainLink last, link;
    Blt_DBuffer dbuffer;
    PartitionInfo *piPtr;
    
    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    pattern = Tcl_GetString(objv[4]);
    piPtr = &tablePtr->rows;
    last = Blt_Chain_LastLink(piPtr->chain);
    dbuffer = Blt_DBuffer_Create();
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        RowColumn *rcPtr;
        char string[200];

        rcPtr = Blt_Chain_GetValue(link);
        Blt_FmtString(string, 200, "%d", rcPtr->index);
        if (Tcl_StringMatch(string, pattern)) {
            Blt_DBuffer_Format(dbuffer, "%d", rcPtr->index);
            PrintRowColumn(interp, piPtr, rcPtr, dbuffer);
            if (link != last) {
                Blt_DBuffer_AppendString(dbuffer, " \\\n", 2);
            } else {
                Blt_DBuffer_AppendString(dbuffer, "\n", 1);
            }
        }
    }
    Tcl_SetObjResult(interp, Blt_DBuffer_StringObj(dbuffer));
    Blt_DBuffer_Destroy(dbuffer);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowInsertOp --
 *
 *      Inserts a span of rows into the table.
 *
 *      blt::table row insert tableName ?switches ...? 
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowInsertOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{ 
    InsertSwitches switches;
    PartitionInfo *piPtr;
    Table *tablePtr;
    TableInterpData *dataPtr = clientData;
    int i;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    switches.count = 1;
    if (Blt_ParseSwitches(interp, rowInsertSwitches, objc - 4, objv + 4,
                          &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    piPtr = &tablePtr->rows;
    /*
     * Insert the new rows from the designated point in the chain.
     */
    for (i = 0; i < switches.count; i++) {
        RowColumn *rcPtr;
        Blt_ChainLink link;
        
        rcPtr = CreateRowColumn();
        link = Blt_Chain_NewLink();
        Blt_Chain_SetValue(link, rcPtr);
        if (switches.afterPtr != NULL) {
            Blt_Chain_LinkAfter(piPtr->chain, link, switches.afterPtr->link);
        } else if (switches.beforePtr != NULL) {
            Blt_Chain_LinkBefore(piPtr->chain, link, switches.beforePtr->link);
        } else {
            Blt_Chain_LinkAfter(piPtr->chain, link, NULL);
        }
        rcPtr->link = link;
    }
    RenumberIndices(piPtr->chain);
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowJoinOp --
 *
 *      Joins the specified span of rows/columns together into a partition.
 *
 *      blt::table row join tableName firstIndex lastIndex
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowJoinOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
    Table *tablePtr;
    RowColumn *fromPtr, *toPtr;
    Blt_ChainLink link;
    PartitionInfo *piPtr;
    int i;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetRowFromObj(interp, tablePtr, objv[4], &fromPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetRowFromObj(interp, tablePtr, objv[5], &toPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (fromPtr->index >= toPtr->index) {
        return TCL_OK;          /* No-op. */
    }
    piPtr = &tablePtr->rows;
    /*
     *  Reduce the span of all entries that currently cross any of the
     *  trailing rows.  Also, if the entry starts in one of these rows,
     *  moved it to the designated "joined" row.
     */
    for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        TableEntry *entryPtr;
        int start, end;         /* Entry indices. */

        entryPtr = Blt_Chain_GetValue(link);
        start = entryPtr->row.rcPtr->index + 1;
        end = entryPtr->row.rcPtr->index + entryPtr->row.span - 1;
        if ((end < fromPtr->index) || ((start > toPtr->index))) {
            continue;
        }
        entryPtr->row.span -= toPtr->index - start + 1;
        if (start >= fromPtr->index) { /* Entry starts in a trailing
                                        * partition. */
            entryPtr->row.rcPtr = fromPtr;
        }
    }
    link = Blt_Chain_NextLink(fromPtr->link);
    for (i = fromPtr->index + 1; i <= toPtr->index; i++) {
        Blt_ChainLink next;
        RowColumn *rcPtr;
        
        next = Blt_Chain_NextLink(link);
        rcPtr = Blt_Chain_GetValue(link);
        DeleteRowColumn(tablePtr, piPtr, rcPtr);
        Blt_Chain_DeleteLink(piPtr->chain, link);
        link = next;
    }
    RenumberIndices(piPtr->chain);
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RowSplitOp --
 *
 *      Splits a the designated row into multiple rows. Any widgets that
 *      span this row/column will be automatically corrected to include the
 *      new rows.
 *
 *      blt::table row splot tableName rowIndex numDivisions
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowSplitOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_ChainLink link;
    PartitionInfo *piPtr;
    RowColumn *rowPtr;
    Table *tablePtr;
    TableInterpData *dataPtr = clientData;
    int i, numRows;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[3], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (GetRowFromObj(interp, tablePtr, objv[4], &rowPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    numRows = 2;
    if (objc > 5) {
        if (Tcl_GetIntFromObj(interp, objv[5], &numRows) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (numRows < 2) {
        Tcl_AppendResult(interp, "bad split value \"", Tcl_GetString(objv[5]),
            "\": should be 2 or greater", (char *)NULL);
        return TCL_ERROR;
    }
    /*
     * Append (split - 1) additional rows/columns starting from the current
     * point in the chain.
     */
    piPtr = &tablePtr->rows;
    for (i = 1; i < numRows; i++) {
        RowColumn *rcPtr;
        Blt_ChainLink link;
        
        rcPtr = CreateRowColumn();
        link = Blt_Chain_NewLink();
        Blt_Chain_SetValue(link, rcPtr);
        Blt_Chain_LinkAfter(piPtr->chain, link, rowPtr->link);
        rcPtr->link = link;
    }

    /*
     * Also increase the span of all entries that span this row/column by
     * numRows - 1.
     */
    for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        TableEntry *entryPtr;
        int start, end;

        entryPtr = Blt_Chain_GetValue(link);
        start = entryPtr->row.rcPtr->index;
        end = entryPtr->row.rcPtr->index + entryPtr->row.span;
        if ((start <= rowPtr->index) && (rowPtr->index < end)) {
            entryPtr->row.span += (numRows - 1);
        }
    }
    RenumberIndices(tablePtr->rows.chain);
    tablePtr->flags |= REQUEST_LAYOUT;
    EventuallyArrangeTable(tablePtr);
    return TCL_OK;
}

static Blt_OpSpec rowOps[] =
{
    {"cget",       2, RowCgetOp,      6, 6, "tableName rowIndex option"},
    {"configure",  2, RowConfigureOp, 5, 0, "tableName rowIndex ?option value ... ?"},
    {"delete",     1, RowDeleteOp,    4, 0, "tableName firstIndex ?lastIndex?"},
    {"extents",    1, RowExtentsOp,   5, 5, "tableName rowIndex"},
    {"find",       1, RowFindOp,      6, 6, "tableName x y"},
    {"info",       3, RowInfoOp,      5, 5, "tableName rowIndex"},
    {"insert",     3, RowInsertOp,    5, 7, "tableName ?switches ...?",},
    {"join",       1, RowJoinOp,      6, 6, "tableName firstRow lastRow"},
    {"split",      2, RowSplitOp,     5, 6, "tableName rowIndex ?numRows?"},
};

static int numRowOps = sizeof(rowOps) / sizeof(Blt_OpSpec);

static int
RowOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    
    proc = Blt_GetOpFromObj(interp, numRowOps, rowOps, BLT_OP_ARG2, 
                objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * SaveOp --
 *
 *      Returns a list of all the commands necessary to rebuild the the
 *      table.  This includes the layout of the widgets and any row,
 *      column, or table options set.
 *
 * Results:
 *      Returns a standard TCL result.  If no error occurred, TCL_OK is
 *      returned and a list of widget path names is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SaveOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_ChainLink link, lastl;
    PartitionInfo *piPtr;
    Table *tablePtr;
    TableInterpData *dataPtr = clientData;
    Blt_DBuffer dbuffer;
    int start, last;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    dbuffer = Blt_DBuffer_Create();
    Blt_DBuffer_Format(dbuffer, "\n# Table layout\n\n");
    Blt_DBuffer_Format(dbuffer, "%s %s \\\n", Tcl_GetString(objv[0]),
                       Tk_PathName(tablePtr->tkwin));

    lastl = Blt_Chain_LastLink(tablePtr->chain);
    for (link = Blt_Chain_FirstLink(tablePtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        TableEntry *entryPtr;

        entryPtr = Blt_Chain_GetValue(link);
        PrintEntry(entryPtr, dbuffer);
        if (link != lastl) {
            Blt_DBuffer_AppendString(dbuffer, " \\\n", 3);
        }
    }
    Blt_DBuffer_Format(dbuffer, "\n\n# Row configuration options\n\n");
    piPtr = &tablePtr->rows;
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        RowColumn *rcPtr;
        int start, last;

        rcPtr = Blt_Chain_GetValue(link);
        start = Blt_DBuffer_Length(dbuffer);
        Blt_DBuffer_Format(dbuffer, "%s configure %s r%d ",
                Tcl_GetString(objv[0]), Tk_PathName(tablePtr->tkwin),
                rcPtr->index);
        last = Blt_DBuffer_Length(dbuffer);
        PrintRowColumn(interp, piPtr, rcPtr, dbuffer);
        if (Blt_DBuffer_Length(dbuffer) == last) {
            Blt_DBuffer_SetLength(dbuffer, start);
        } else {
            Blt_DBuffer_AppendString(dbuffer, "\n", 1);
        }
    }
    Blt_DBuffer_Format(dbuffer, "\n\n# Column configuration options\n\n");
    piPtr = &tablePtr->columns;
    for (link = Blt_Chain_FirstLink(piPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        RowColumn *rcPtr;
        int start, last;

        rcPtr = Blt_Chain_GetValue(link);
        start = Blt_DBuffer_Length(dbuffer);
        Blt_DBuffer_Format(dbuffer, "%s configure %s c%d ",
                Tcl_GetString(objv[0]), Tk_PathName(tablePtr->tkwin),
                rcPtr->index);
        last = Blt_DBuffer_Length(dbuffer);
        PrintRowColumn(interp, piPtr, rcPtr, dbuffer);
        if (Blt_DBuffer_Length(dbuffer) == last) {
            Blt_DBuffer_SetLength(dbuffer, start);
        } else {
            Blt_DBuffer_AppendString(dbuffer, "\n", 1);
        }
    }
    start = Blt_DBuffer_Length(dbuffer);
    Blt_DBuffer_Format(dbuffer, "\n\n# Table configuration options\n\n");
    Blt_DBuffer_Format(dbuffer, "%s configure %s ",
        Tcl_GetString(objv[0]), Tk_PathName(tablePtr->tkwin)); 
 
    last = Blt_DBuffer_Length(dbuffer);
    PrintTable(tablePtr, dbuffer);
    if (Blt_DBuffer_Length(dbuffer) == last) {
        Blt_DBuffer_SetLength(dbuffer, start);
    } else {
        Blt_DBuffer_AppendString(dbuffer, "\n", 1);
    }
    Tcl_SetObjResult(interp, Blt_DBuffer_StringObj(dbuffer));
    Blt_DBuffer_Destroy(dbuffer);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SearchOp --
 *
 *      Returns a list of all the pathnames of the widgets managed by a
 *      table geometry manager.  The table is given by the path name of a
 *      container widget associated with the table.
 *
 * Results:
 *      Returns a standard TCL result.  If no error occurred, TCL_OK is
 *      returned and a list of widget path names is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SearchOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    TableInterpData *dataPtr = clientData;
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
        TableEntry *entryPtr;

        entryPtr = Blt_Chain_GetValue(link);
        if (switches.pattern != NULL) {
            if (!Tcl_StringMatch(Tk_PathName(entryPtr->tkwin),
                                 switches.pattern)) {
                continue;
            }
        }
        if (switches.flags & MATCH_SPAN) {
            if ((switches.rspan >= 0) && 
                (entryPtr->row.rcPtr->index > switches.rspan) &&
                ((entryPtr->row.rcPtr->index + entryPtr->row.span) <
                 switches.rspan)){
                continue;
            } 
            if ((switches.cspan >= 0) && 
                ((entryPtr->column.rcPtr->index > switches.cspan) ||
                ((entryPtr->column.rcPtr->index + entryPtr->column.span) <
                 switches.cspan))) {
                continue;
            }
        }
        if (switches.flags & MATCH_START) {
            if ((switches.rstart >= 0) && 
                (entryPtr->row.rcPtr->index != switches.rstart)) {
                continue;
            }
            if ((switches.cstart >= 0) && 
                (entryPtr->column.rcPtr->index != switches.cstart)) {
                continue;
            }
        }
        Tcl_AppendElement(interp, Tk_PathName(entryPtr->tkwin));
    }
    Blt_FreeSwitches(searchSwitches, (char *)&switches, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Table operations.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec tableOps[] =
{
    {"arrange",    1, ArrangeOp,   3, 3, "tableName",},
    {"cget",       2, CgetOp,      4, 5, "tableName ?row|column|widget? option",},
    {"column",     3, ColumnOp,    2, 0, "args ...",},
    {"configure",  4, ConfigureOp, 3, 0, "tableName ?row|column|widget?... ?option value?...",},
    {"containers", 4, NamesOp,     2, 4, "?switch? ?arg?",},
    {"find",       2, FindOp,      5, 5, "tableName x y",},
    {"forget",     2, ForgetOp,    3, 0, "pathName ?pathName?...",},
    {"info",       3, InfoOp,      3, 0, "tableName pathName",},
    {"names",      1, NamesOp,     2, 4, "?switch? ?arg?",},
    {"row",        1, RowOp,       2, 0, "args ...",},
    {"save",       2, SaveOp,      3, 3, "tableName",},
    {"search",     2, SearchOp,    3, 0, "tableName ?switch arg?...",},
};

static int numTableOps = sizeof(tableOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * TableCmd --
 *
 *      This procedure is invoked to process the TCL command that
 *      corresponds to the table geometry manager.  See the user
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
TableCmd(ClientData clientData, Tcl_Interp *interp, int objc,
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
 *      This is called when the interpreter hosting the table command is
 *      destroyed.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Destroys all the hash table maintaining the names of the table
 *      geometry managers.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
TableInterpDeleteProc(
    ClientData clientData,      /* Thread-specific data. */
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
 *      This procedure is invoked to initialize the TCL command that
 *      corresponds to the table geometry manager.
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
Blt_TableMgrCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "table", TableCmd, };

    cmdSpec.clientData = GetTableInterpData(interp);
    rowUid = (Blt_Uid)Tk_GetUid("row");
    columnUid = (Blt_Uid)Tk_GetUid("column");
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

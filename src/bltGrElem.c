/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrElem.c --
 *
 * This module implements generic elements for the BLT graph widget.
 *
 *	Copyright 1993-2004 George A Howlett.
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

#include <X11/Xutil.h>
#include "bltAlloc.h"
#include "bltMath.h"
#include "bltHash.h"
#include "bltChain.h"
#include "bltOp.h"
#include "bltBind.h"
#include "bltPs.h"
#include "bltBg.h"
#include "bltPicture.h"
#include "bltTags.h"
#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"
#include "bltDataTable.h"

#define GRAPH_KEY		"BLT Graph Data"

/* Ignore elements that aren't in the display list. */
#define IGNORE_ELEMENT(e) ((e)->link == NULL)

typedef struct {
    BLT_TABLE table;
    int refCount;
} TableClient;

/*
 * ElementIterator --
 *
 *	Elements may be tagged with strings.  An element may have many
 *	tags.  The same tag may be used for many elements.
 *	
 */
typedef enum { 
    ITER_SINGLE, ITER_ALL, ITER_TAG, 
} IteratorType;

typedef struct _Iterator {
    Graph *graphPtr;		        /* Graph that we're iterating
                                         * over. */

    IteratorType type;			/* Type of iteration:
					 * ITER_TAG	 By item tag.
					 * ITER_ALL      By every item.
					 * ITER_SINGLE   Single item: either 
					 *               tag or name.
					 */
    Element *elemPtr;
    const char *tagName;		/* If non-NULL, is the tag that we
					 * are currently iterating over. */
    Blt_HashTable *tablePtr;		/* Pointer to tag hash table. */
    Blt_HashSearch cursor;		/* Search iterator for tag hash
					 * table. */
    Blt_ChainLink link;
} ElementIterator;

static Blt_OptionParseProc ObjToAlong;
static Blt_OptionPrintProc AlongToObj;
static Blt_CustomOption alongOption =
{
    ObjToAlong, AlongToObj, NULL, (ClientData)0
};
static Blt_OptionFreeProc FreeValues;
static Blt_OptionParseProc ObjToValues;
static Blt_OptionPrintProc ValuesToObj;
Blt_CustomOption bltValuesOption =
{
    ObjToValues, ValuesToObj, FreeValues, (ClientData)0
};
static Blt_OptionFreeProc FreeValuePairs;
static Blt_OptionParseProc ObjToValuePairs;
static Blt_OptionPrintProc ValuePairsToObj;
Blt_CustomOption bltValuePairsOption =
{
    ObjToValuePairs, ValuePairsToObj, FreeValuePairs, (ClientData)0
};

static Blt_OptionFreeProc  FreeStyles;
static Blt_OptionParseProc ObjToStyles;
static Blt_OptionPrintProc StylesToObj;
Blt_CustomOption bltLineStylesOption =
{
    ObjToStyles, StylesToObj, FreeStyles, (ClientData)0,
};

Blt_CustomOption bltBarStylesOption =
{
    ObjToStyles, StylesToObj, FreeStyles, (ClientData)0,
};

#include "bltGrElem.h"

static Blt_VectorChangedProc VectorChangedProc;

static void FindRange(ElemValues *valuesPtr);
static void FreeDataValues(ElemValues *valuesPtr);

typedef int (GraphElementProc)(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv);

static Tcl_FreeProc FreeElement;

static void
FreeElement(DestroyData data)
{
    Blt_Free(data);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyTableClients --
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
void
Blt_DestroyTableClients(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    
    for (hPtr = Blt_FirstHashEntry(&graphPtr->dataTables, &iter);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	TableClient *clientPtr;

	clientPtr = Blt_GetHashValue(hPtr);
	if (clientPtr->table != NULL) {
	    blt_table_close(clientPtr->table);
	}
	Blt_Free(clientPtr);
    }
    Blt_DeleteHashTable(&graphPtr->dataTables);
}

/*
 *---------------------------------------------------------------------------
 * Custom option parse and print procedures
 *---------------------------------------------------------------------------
 */
static int
GetPenStyleFromObj(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
		   ClassId classId, PenStyle *stylePtr)
{
    Pen *penPtr;
    Tcl_Obj **objv;
    int objc;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((objc != 1) && (objc != 3)) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "bad style entry \"", 
			Tcl_GetString(objPtr), 
			"\": should be \"penName\" or \"penName min max\"", 
			(char *)NULL);
	}
	return TCL_ERROR;
    }
    if (Blt_GetPenFromObj(interp, graphPtr, objv[0], classId, &penPtr) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 3) {
	double min, max;

	if ((Tcl_GetDoubleFromObj(interp, objv[1], &min) != TCL_OK) ||
	    (Tcl_GetDoubleFromObj(interp, objv[2], &max) != TCL_OK)) {
	    return TCL_ERROR;
	}
	SetWeight(stylePtr->weight, min, max);
    }
    stylePtr->penPtr = penPtr;
    return TCL_OK;
}

static void
FreeVectorSource(ElemValues *valuesPtr)
{
    if (valuesPtr->vectorSource.vector != NULL) { 
	Blt_SetVectorChangedProc(valuesPtr->vectorSource.vector, NULL, NULL);
	Blt_FreeVectorId(valuesPtr->vectorSource.vector); 
	valuesPtr->vectorSource.vector = NULL;
    }
}

static int
FetchVectorValues(Tcl_Interp *interp, ElemValues *valuesPtr, Blt_Vector *vector)
{
    double *array;
    
    if (valuesPtr->values == NULL) {
	array = Blt_Malloc(Blt_VecLength(vector) * sizeof(double));
    } else {
	array = Blt_Realloc(valuesPtr->values, 
			    Blt_VecLength(vector) * sizeof(double));
    }
    if (array == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't allocate new vector", (char *)NULL);
	}
	return TCL_ERROR;
    }
    memcpy(array, Blt_VecData(vector), sizeof(double) * Blt_VecLength(vector));
    valuesPtr->min = Blt_VecMin(vector);
    valuesPtr->max = Blt_VecMax(vector);
    valuesPtr->values = array;
    valuesPtr->numValues = Blt_VecLength(vector);
    /* FindRange(valuesPtr); */
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorChangedProc --
 *
 * Results:
 *     	None.
 *
 * Side Effects:
 *	Graph is redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
VectorChangedProc(Tcl_Interp *interp, ClientData clientData, 
		  Blt_VectorNotify notify)
{
    ElemValues *valuesPtr = clientData;

    if (notify == BLT_VECTOR_NOTIFY_DESTROY) {
	FreeDataValues(valuesPtr);
    } else {
	Blt_Vector *vector;
	
	Blt_GetVectorById(interp, valuesPtr->vectorSource.vector, &vector);
	if (FetchVectorValues(NULL, valuesPtr, vector) != TCL_OK) {
	    return;
	}
    }
    {
	Element *elemPtr = valuesPtr->elemPtr;
	Graph *graphPtr;
	
	graphPtr = elemPtr->obj.graphPtr;
	graphPtr->flags |= RESET_AXES;
	elemPtr->flags |= MAP_ITEM;
	if (!IGNORE_ELEMENT(elemPtr)) {
	    graphPtr->flags |= CACHE_DIRTY;
	    Blt_EventuallyRedrawGraph(graphPtr);
	}
    }
}

static int 
GetVectorData(Tcl_Interp *interp, ElemValues *valuesPtr, const char *vecName)
{
    Blt_Vector *vecPtr;
    VectorDataSource *srcPtr;

    srcPtr = &valuesPtr->vectorSource;
    srcPtr->vector = Blt_AllocVectorId(interp, vecName);
    if (Blt_GetVectorById(interp, srcPtr->vector, &vecPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (FetchVectorValues(interp, valuesPtr, vecPtr) != TCL_OK) {
	FreeVectorSource(valuesPtr);
	return TCL_ERROR;
    }
    Blt_SetVectorChangedProc(srcPtr->vector, VectorChangedProc, valuesPtr);
    valuesPtr->type = ELEM_SOURCE_VECTOR;
    return TCL_OK;
}

static int
FetchTableValues(Tcl_Interp *interp, ElemValues *valuesPtr, 
		 BLT_TABLE_COLUMN col, Tcl_Obj *objPtr)
{
    long i;
    double *array;
    BLT_TABLE table;
    BLT_TABLE_ITERATOR ri;
    BLT_TABLE_ROW row;

    table = valuesPtr->tableSource.table;
    if (objPtr != NULL) {
        if (blt_table_iterate_row(interp, table, objPtr, &ri) != TCL_OK) {
            return TCL_ERROR;
        }
        if (ri.numEntries == 0) {
            Tcl_AppendResult(interp, "no values in tag \"", 
                Tcl_GetString(objPtr), "\"", (char *)NULL);
            return TCL_ERROR;
        }
    } else {
        blt_table_iterate_all_rows(table, &ri);
    }
    array = Blt_Malloc(sizeof(double) * ri.numEntries);
    if (array == NULL) {
	return TCL_ERROR;
    }
    i = 0;
    for (row = blt_table_first_tagged_row(&ri); row != NULL; 
         row = blt_table_next_tagged_row(&ri)) {
	array[i] = blt_table_get_double(table, row, col);
        i++;
    }
    if (valuesPtr->values != NULL) {
	Blt_Free(valuesPtr->values);
    }
    valuesPtr->numValues = i;
    valuesPtr->values = array;
    FindRange(valuesPtr);
    return TCL_OK;
}

static void
FreeTableSource(ElemValues *valuesPtr)
{
    TableDataSource *srcPtr;

    srcPtr = &valuesPtr->tableSource;
    if (srcPtr->trace != NULL) {
	blt_table_delete_trace(srcPtr->table, srcPtr->trace);
    }
    if (srcPtr->notifier != NULL) {
	blt_table_delete_notifier(srcPtr->table, srcPtr->notifier);
    }
    if (srcPtr->hashPtr != NULL) {
	TableClient *clientPtr;

	clientPtr = Blt_GetHashValue(srcPtr->hashPtr);
	clientPtr->refCount--;
	if (clientPtr->refCount == 0) {
	    Graph *graphPtr;

	    graphPtr = valuesPtr->elemPtr->obj.graphPtr;
	    if (srcPtr->table != NULL) {
		blt_table_close(srcPtr->table);
	    }
	    Blt_Free(clientPtr);
	    Blt_DeleteHashEntry(&graphPtr->dataTables, srcPtr->hashPtr);
	    srcPtr->hashPtr = NULL;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TableNotifyProc --
 *
 *
 * Results:
 *     	None.
 *
 * Side Effects:
 *	Graph is redrawn.
 *
 *---------------------------------------------------------------------------
 */
static int
TableNotifyProc(ClientData clientData, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    ElemValues *valuesPtr = clientData;
    Element *elemPtr;
    Graph *graphPtr;

    elemPtr = valuesPtr->elemPtr;
    graphPtr = elemPtr->obj.graphPtr;
    if ((eventPtr->type == TABLE_NOTIFY_COLUMNS_DELETED) || 
	(FetchTableValues(graphPtr->interp, valuesPtr, 
                eventPtr->column, NULL)) != TCL_OK) {
	FreeTableSource(valuesPtr);
	return TCL_ERROR;
    } 
    /* Always redraw the element. */
    graphPtr->flags |= RESET_AXES;
    elemPtr->flags |= MAP_ITEM;
    if (!IGNORE_ELEMENT(elemPtr)) {
	graphPtr->flags |= CACHE_DIRTY;
	Blt_EventuallyRedrawGraph(graphPtr);
    }
    return TCL_OK;
}
 
/*
 *---------------------------------------------------------------------------
 *
 * TableTraceProc --
 *
 *
 * Results:
 *     	None.
 *
 * Side Effects:
 *	Graph is redrawn.
 *
 *---------------------------------------------------------------------------
 */
static int
TableTraceProc(ClientData clientData, BLT_TABLE_TRACE_EVENT *eventPtr)
{
    ElemValues *valuesPtr = clientData;
    Element *elemPtr;
    Graph *graphPtr;

    elemPtr = valuesPtr->elemPtr;
    graphPtr = elemPtr->obj.graphPtr;
    assert((BLT_TABLE_COLUMN)eventPtr->column == valuesPtr->tableSource.column);
    if (FetchTableValues(eventPtr->interp, valuesPtr, eventPtr->column, NULL) 
	!= TCL_OK) {
	FreeTableSource(valuesPtr);
	return TCL_ERROR;
    }
    graphPtr->flags |= RESET_AXES;
    elemPtr->flags |= MAP_ITEM;
    if (!IGNORE_ELEMENT(elemPtr)) {
	graphPtr->flags |= CACHE_DIRTY;
	Blt_EventuallyRedrawGraph(graphPtr);
    }
    return TCL_OK;
}

static int
GetTableData(Tcl_Interp *interp, ElemValues *valuesPtr, int objc, 
             Tcl_Obj **objv)
{
    TableDataSource *srcPtr;
    TableClient *clientPtr;
    int isNew;
    Graph *graphPtr;
    Tcl_Obj *tagObjPtr;

    memset(&valuesPtr->tableSource, 0, sizeof(TableDataSource));
    srcPtr = &valuesPtr->tableSource;
    graphPtr = valuesPtr->elemPtr->obj.graphPtr;
    /* See if the graph is already using this table. */
    srcPtr->hashPtr = Blt_CreateHashEntry(&graphPtr->dataTables, 
        Tcl_GetString(objv[0]), &isNew);
    if (isNew) {
	if (blt_table_open(interp, Tcl_GetString(objv[0]), &srcPtr->table) 
            != TCL_OK) {
	    return TCL_ERROR;
	}
	clientPtr = Blt_AssertMalloc(sizeof(TableClient));
	clientPtr->table = srcPtr->table;
	clientPtr->refCount = 1;
	Blt_SetHashValue(srcPtr->hashPtr, clientPtr);
    } else {
	clientPtr = Blt_GetHashValue(srcPtr->hashPtr);
	srcPtr->table = clientPtr->table;
	clientPtr->refCount++;
    }
    srcPtr->column = blt_table_get_column(interp, srcPtr->table, objv[1]);
    if (srcPtr->column == NULL) {
	goto error;
    }
    tagObjPtr = (objc == 3) ? objv[2] : NULL;
    if (FetchTableValues(interp, valuesPtr, srcPtr->column, tagObjPtr) 
        != TCL_OK) {
	goto error;
    }
    srcPtr->notifier = blt_table_create_column_notifier(interp, srcPtr->table, 
	srcPtr->column, TABLE_NOTIFY_WHENIDLE | TABLE_NOTIFY_COLUMN_CHANGED, 
	TableNotifyProc, 
	(BLT_TABLE_NOTIFIER_DELETE_PROC *)NULL, valuesPtr);
    srcPtr->trace = blt_table_create_column_trace(srcPtr->table, srcPtr->column,
	(TABLE_TRACE_WHENIDLE | TABLE_TRACE_WCU), TableTraceProc,
	(BLT_TABLE_TRACE_DELETE_PROC *)NULL, valuesPtr);
    valuesPtr->type = ELEM_SOURCE_TABLE;
    return TCL_OK;
 error:
    FreeTableSource(valuesPtr);
    return TCL_ERROR;
}

static int
ParseValues(Tcl_Interp *interp, Tcl_Obj *objPtr, int *numValuesPtr,
	    double **arrayPtr)
{
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    *arrayPtr = NULL;
    *numValuesPtr = 0;
    if (objc > 0) {
	double *array;
	double *p;
	int i;

	array = Blt_Malloc(sizeof(double) * objc);
	if (array == NULL) {
	    Tcl_AppendResult(interp, "can't allocate new vector", (char *)NULL);
	    return TCL_ERROR;
	}
	for (p = array, i = 0; i < objc; i++, p++) {
	    if (Blt_ExprDoubleFromObj(interp, objv[i], p) != TCL_OK) {
		Blt_Free(array);
		return TCL_ERROR;
	    }
	}
	*arrayPtr = array;
	*numValuesPtr = objc;
    }
    return TCL_OK;
}

static void
FreeDataValues(ElemValues *valuesPtr)
{
    switch (valuesPtr->type) {
    case ELEM_SOURCE_VECTOR: 
	FreeVectorSource(valuesPtr);	break;
    case ELEM_SOURCE_TABLE:
	FreeTableSource(valuesPtr);	break;
    case ELEM_SOURCE_VALUES:
					break;
    }
    if (valuesPtr->values != NULL) {
	Blt_Free(valuesPtr->values);
    }
    valuesPtr->values = NULL;
    valuesPtr->numValues = 0;
    valuesPtr->type = ELEM_SOURCE_VALUES;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindRange --
 *
 *	Find the minimum, positive minimum, and maximum values in a given
 *	vector and store the results in the vector structure.
 *
 * Results:
 *     	None.
 *
 * Side Effects:
 *	Minimum, positive minimum, and maximum values are stored in the
 *	vector.
 *
 *---------------------------------------------------------------------------
 */
static void
FindRange(ElemValues *valuesPtr)
{
    int i;
    double *x;
    double min, max;

    if ((valuesPtr->numValues < 1) || (valuesPtr->values == NULL)) {
	return;			/* This shouldn't ever happen. */
    }
    x = valuesPtr->values;

    min = DBL_MAX, max = -DBL_MAX;
    for(i = 0; i < valuesPtr->numValues; i++) {
	if (FINITE(x[i])) {
	    min = max = x[i];
	    break;
	}
    }
    /*  Initialize values to track the vector range */
    for (/* empty */; i < valuesPtr->numValues; i++) {
	if (FINITE(x[i])) {
	    if (x[i] < min) {
		min = x[i];
	    } else if (x[i] > max) {
		max = x[i];
	    }
	}
    }
    valuesPtr->min = min, valuesPtr->max = max;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FindElemValuesMinimum --
 *
 *	Find the minimum, positive minimum, and maximum values in a given
 *	vector and store the results in the vector structure.
 *
 * Results:
 *     	None.
 *
 * Side Effects:
 *	Minimum, positive minimum, and maximum values are stored in the
 *	vector.
 *
 *---------------------------------------------------------------------------
 */
double
Blt_FindElemValuesMinimum(ElemValues *valuesPtr, double minLimit)
{
    int i;
    double min;

    min = DBL_MAX;
    for (i = 0; i < valuesPtr->numValues; i++) {
	double x;

	x = valuesPtr->values[i];
	if (x < 0.0) {
	    /* What do you do about negative values when using log
	     * scale values seems like a grey area.  Mirror. */
	    x = -x;
	}
	if ((x > minLimit) && (min > x)) {
	    min = x;
	}
    }
    if (min == DBL_MAX) {
	min = minLimit;
    }
    return min;
}

/*ARGSUSED*/
static void
FreeValues(
    ClientData clientData,	/* Not used. */
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)
{
    ElemValues *valuesPtr = (ElemValues *)(widgRec + offset);

    FreeDataValues(valuesPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToValues --
 *
 *	Given a TCL list of numeric expression representing the element
 *	values, convert into an array of double precision values. In addition,
 *	the minimum and maximum values are saved.  Since elastic values are
 *	allow (values which translate to the min/max of the graph), we must
 *	try to get the non-elastic minimum and maximum.
 *
 * Results:
 *	The return value is a standard TCL result.  The vector is passed
 *	back via the valuesPtr.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToValues(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* TCL list of expressions */
    char *widgRec,		/* Element record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    ElemValues *valuesPtr = (ElemValues *)(widgRec + offset);
    Element *elemPtr = (Element *)widgRec;
    Tcl_Obj **objv;
    int objc;
    int result;
    const char *string;

    valuesPtr->elemPtr = elemPtr;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    elemPtr->flags |= MAP_ITEM;

    /* Release the current data sources. */
    FreeDataValues(valuesPtr);
    if (objc == 0) {
	return TCL_OK;			/* Empty list of values. */
    }
    string = Tcl_GetString(objv[0]);
    if ((objc == 1) && (Blt_VectorExists2(interp, string))) {
	result = GetVectorData(interp, valuesPtr, string);
    } else if (((objc == 2) || (objc == 3)) && 
               (blt_table_exists(interp, string))) {
	result = GetTableData(interp, valuesPtr, objc, objv);
    } else {
	double *values;
	int numValues;

	result = ParseValues(interp, objPtr, &numValues, &values);
	if (result != TCL_OK) {
	    return TCL_ERROR;		/* Can't parse the values as numbers. */
	}
	FreeDataValues(valuesPtr);
	if (numValues > 0) {
	    valuesPtr->values = values;
	}
	valuesPtr->numValues = numValues;
	FindRange(valuesPtr);
	valuesPtr->type = ELEM_SOURCE_VALUES;
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ValuesToObj --
 *
 *	Convert the vector of floating point values into a TCL list.
 *
 * Results:
 *	The string representation of the vector is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ValuesToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Element record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    ElemValues *valuesPtr = (ElemValues *)(widgRec + offset);

    switch (valuesPtr->type) {
    case ELEM_SOURCE_VECTOR:
	{
	    const char *vecName;
	    
	    vecName = Blt_NameOfVectorId(valuesPtr->vectorSource.vector);
	    return Tcl_NewStringObj(vecName, -1);
	}
    case ELEM_SOURCE_TABLE:
	{
	    Tcl_Obj *listObjPtr;
	    const char *tableName;
	    long i;
	    
	    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	    tableName = blt_table_name(valuesPtr->tableSource.table);
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj(tableName, -1));
	    
	    i = blt_table_column_index(valuesPtr->tableSource.column);
	    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewLongObj(i));
	    return listObjPtr;
	}
    case ELEM_SOURCE_VALUES:
	{
	    Tcl_Obj *listObjPtr;
	    double *vp, *vend; 
	    
	    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	    for (vp = valuesPtr->values, vend = vp + valuesPtr->numValues; 
		 vp < vend; vp++) {
		Tcl_ListObjAppendElement(interp, listObjPtr, 
					 Tcl_NewDoubleObj(*vp));
	    }
	    return listObjPtr;
	}
    default:
	abort();
    }
    return Tcl_NewStringObj("", 0);
}

/*ARGSUSED*/
static void
FreeValuePairs(
    ClientData clientData,	/* Not used. */
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)			/* Not used. */
{
    Element *elemPtr = (Element *)widgRec;

    FreeDataValues(&elemPtr->x);
    FreeDataValues(&elemPtr->y);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToValuePairs --
 *
 *	This procedure is like ObjToValues except that it interprets
 *	the list of numeric expressions as X Y coordinate pairs.  The
 *	minimum and maximum for both the X and Y vectors are
 *	determined.
 *
 * Results:
 *	The return value is a standard TCL result.  The vectors are
 *	passed back via the widget record (elemPtr).
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToValuePairs(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* TCL list of numeric expressions */
    char *widgRec,		/* Element record */
    int offset,			/* Not used. */
    int flags)			/* Not used. */
{
    Element *elemPtr = (Element *)widgRec;
    double *values;
    int numValues;
    size_t newSize;

    if (ParseValues(interp, objPtr, &numValues, &values) != TCL_OK) {
	return TCL_ERROR;
    }
    if (numValues & 1) {
	Tcl_AppendResult(interp, "odd number of data points", (char *)NULL);
	Blt_Free(values);
	return TCL_ERROR;
    }
    numValues /= 2;
    newSize = numValues * sizeof(double);
    FreeDataValues(&elemPtr->x);	/* Release the current data sources. */
    FreeDataValues(&elemPtr->y);
    if (newSize > 0) {
	double *p;
	int i;

	elemPtr->x.values = Blt_AssertMalloc(newSize);
	elemPtr->y.values = Blt_AssertMalloc(newSize);
	elemPtr->x.numValues = elemPtr->y.numValues = numValues;
	for (p = values, i = 0; i < numValues; i++) {
	    elemPtr->x.values[i] = *p++;
	    elemPtr->y.values[i] = *p++;
	}
	Blt_Free(values);
	FindRange(&elemPtr->x);
	FindRange(&elemPtr->y);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ValuePairsToObj --
 *
 *	Convert pairs of floating point values in the X and Y arrays
 *	into a TCL list.
 *
 * Results:
 *	The return value is a string (TCL list).
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ValuePairsToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Element information record */
    int offset,			/* Not used. */
    int flags)			/* Not used. */
{
    Element *elemPtr = (Element *)widgRec;
    Tcl_Obj *listObjPtr;
    int i;
    int length;

    length = NUMBEROFPOINTS(elemPtr);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 0; i < length; i++) {
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewDoubleObj(elemPtr->x.values[i]));
	Tcl_ListObjAppendElement(interp, listObjPtr, 
				 Tcl_NewDoubleObj(elemPtr->y.values[i]));
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToAlong --
 *
 *	Given a TCL list of numeric expression representing the element
 *	values, convert into an array of double precision values. In
 *	addition, the minimum and maximum values are saved.  Since
 *	elastic values are allow (values which translate to the
 *	min/max of the graph), we must try to get the non-elastic
 *	minimum and maximum.
 *
 * Results:
 *	The return value is a standard TCL result.  The vector is passed
 *	back via the valuesPtr.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToAlong(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* String representation of value. */
    char *widgRec,		/* Widget record. */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    int *searchPtr = (int *)(widgRec + offset);
    char *string;

    string = Tcl_GetString(objPtr);
    if ((string[0] == 'x') && (string[1] == '\0')) {
	*searchPtr = NEAREST_SEARCH_X;
    } else if ((string[0] == 'y') && (string[1] == '\0')) { 
	*searchPtr = NEAREST_SEARCH_Y;
    } else if ((string[0] == 'b') && (strcmp(string, "both") == 0)) {
	*searchPtr = NEAREST_SEARCH_XY;
    } else {
	Tcl_AppendResult(interp, "bad along value \"", string, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AlongToObj --
 *
 *	Convert the vector of floating point values into a TCL list.
 *
 * Results:
 *	The string representation of the vector is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
AlongToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Not used. */
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Widget record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    int search = *(int *)(widgRec + offset);
    Tcl_Obj *objPtr;

    switch (search) {
    case NEAREST_SEARCH_X:
	objPtr = Tcl_NewStringObj("x", 1);
	break;
    case NEAREST_SEARCH_Y:
	objPtr = Tcl_NewStringObj("y", 1);
	break;
    case NEAREST_SEARCH_XY:
	objPtr = Tcl_NewStringObj("both", 4);
	break;
    default:
	objPtr = Tcl_NewStringObj("unknown along value", 4);
	break;
    }
    return objPtr;
}

void
Blt_FreeStyles(Blt_Chain styles)
{
    Blt_ChainLink link;

    /* Skip the first slot. It contains the built-in "normal" pen of
     * the element.  */
    link = Blt_Chain_FirstLink(styles);
    if (link != NULL) {
	Blt_ChainLink next;

	for (link = Blt_Chain_NextLink(link); link != NULL; link = next) {
	    PenStyle *stylePtr;

	    next = Blt_Chain_NextLink(link);
	    stylePtr = Blt_Chain_GetValue(link);
	    Blt_FreePen(stylePtr->penPtr);
	    Blt_Chain_DeleteLink(styles, link);
	}
    }
}

/*ARGSUSED*/
static void
FreeStyles(
    ClientData clientData,	/* Not used. */
    Display *display,		/* Not used. */
    char *widgRec,
    int offset)
{
    Blt_Chain styles = *(Blt_Chain *)(widgRec + offset);

    Blt_FreeStyles(styles);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ObjToStyles --
 *
 *	Parse the list of style names.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToStyles(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* String representing style list */
    char *widgRec,		/* Element information record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    Blt_Chain styles = *(Blt_Chain *)(widgRec + offset);
    Blt_ChainLink link;
    Element *elemPtr = (Element *)(widgRec);
    PenStyle *stylePtr;
    Tcl_Obj **objv;
    int objc;
    int i;
    size_t size = (size_t)clientData;


    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Reserve the first entry for the "normal" pen. We'll set the
     * style later */
    Blt_FreeStyles(styles);
    link = Blt_Chain_FirstLink(styles);
    if (link == NULL) {
	link = Blt_Chain_AllocLink(size);
	Blt_Chain_LinkAfter(styles, link, NULL);
    }
    stylePtr = Blt_Chain_GetValue(link);
    stylePtr->penPtr = elemPtr->normalPenPtr;
    for (i = 0; i < objc; i++) {
	link = Blt_Chain_AllocLink(size);
	stylePtr = Blt_Chain_GetValue(link);
	stylePtr->weight.min = (double)i;
	stylePtr->weight.max = (double)i + 1.0;
	stylePtr->weight.range = 1.0;
	if (GetPenStyleFromObj(interp, elemPtr->obj.graphPtr, objv[i], 
		elemPtr->obj.classId, (PenStyle *)stylePtr) != TCL_OK) {
	    Blt_FreeStyles(styles);
	    return TCL_ERROR;
	}
	Blt_Chain_LinkAfter(styles, link, NULL);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StylesToObj --
 *
 *	Convert the style information into a Tcl_Obj.
 *
 * Results:
 *	The string representing the style information is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StylesToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Element information record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    Blt_Chain styles = *(Blt_Chain *)(widgRec + offset);
    Blt_ChainLink link;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    link = Blt_Chain_FirstLink(styles);
    if (link != NULL) {
	/* Skip the first style (it's the default) */
	for (link = Blt_Chain_NextLink(link); link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    PenStyle *stylePtr;
	    Tcl_Obj *subListObjPtr;

	    stylePtr = Blt_Chain_GetValue(link);
	    subListObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	    Tcl_ListObjAppendElement(interp, subListObjPtr, 
		Tcl_NewStringObj(stylePtr->penPtr->name, -1));
	    Tcl_ListObjAppendElement(interp, subListObjPtr, 
				     Tcl_NewDoubleObj(stylePtr->weight.min));
	    Tcl_ListObjAppendElement(interp, subListObjPtr, 
				     Tcl_NewDoubleObj(stylePtr->weight.max));
	    Tcl_ListObjAppendElement(interp, listObjPtr, subListObjPtr);
	}
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_StyleMap --
 *
 *	Creates an array of style indices and fills it based on the weight
 *	of each data point.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed and allocated for the index array.
 *
 *---------------------------------------------------------------------------
 */

PenStyle **
Blt_StyleMap(Element *elemPtr)
{
    int i;
    int numWeights;			/* Number of weights to be examined.
					 * If there are more data points than
					 * weights, they will default to the
					 * normal pen. */
    PenStyle **dataToStyle;		/* Directory of styles.  Each array
					 * element represents the style for *
					 * the data point at that index */
    Blt_ChainLink link;
    PenStyle *stylePtr;
    double *w;				/* Weight vector */
    int numPoints;

    numPoints = NUMBEROFPOINTS(elemPtr);
    numWeights = MIN(elemPtr->w.numValues, numPoints);
    w = elemPtr->w.values;
    link = Blt_Chain_FirstLink(elemPtr->styles);
    stylePtr = Blt_Chain_GetValue(link);

    /* 
     * Create a style mapping array (data point index to style), initialized
     * to the default style.
     */
    dataToStyle = Blt_AssertMalloc(numPoints * sizeof(PenStyle *));
    for (i = 0; i < numPoints; i++) {
	dataToStyle[i] = stylePtr;
    }

    for (i = 0; i < numWeights; i++) {
	for (link = Blt_Chain_LastLink(elemPtr->styles); link != NULL; 
	     link = Blt_Chain_PrevLink(link)) {
	    stylePtr = Blt_Chain_GetValue(link);

	    if (stylePtr->weight.range > 0.0) {
		double norm;

		norm = (w[i] - stylePtr->weight.min) / stylePtr->weight.range;
		if (((norm - 1.0) <= DBL_EPSILON) && 
		    (((1.0 - norm) - 1.0) <= DBL_EPSILON)) {
		    dataToStyle[i] = stylePtr;
		    break;		/* Done: found range that matches. */
		}
	    }
	}
    }
    return dataToStyle;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextTaggedElement --
 *
 *	Returns the next element derived from the given tag.
 *
 * Results:
 *	Returns the pointer to the next element in the iterator.  If 
 *	no more elements are available, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Element *
NextTaggedElement(ElementIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
	if (iterPtr->link != NULL) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(iterPtr->link);
	    iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
	    return elemPtr;
	}
	break;
    case ITER_ALL:
	{
	    Blt_HashEntry *hPtr;
	    
	    hPtr = Blt_NextHashEntry(&iterPtr->cursor); 
	    if (hPtr != NULL) {
		return Blt_GetHashValue(hPtr);
	    }
	    break;
	}
    default:
	break;
    }	
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * FirstTaggedElement --
 *
 *	Returns the first element derived from the given tag.
 *
 * Results:
 *	Returns the first element in the sequence.  If no more elements are in
 *	the list, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Element *
FirstTaggedElement(ElementIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
	if (iterPtr->link != NULL) {
	    Element *elemPtr;
	    
	    elemPtr = Blt_Chain_GetValue(iterPtr->link);
	    iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
	    return elemPtr;
	}
	break;
    case ITER_ALL:
	{
	    Blt_HashEntry *hPtr;
	    
	    hPtr = Blt_FirstHashEntry(iterPtr->tablePtr, &iterPtr->cursor);
	    if (hPtr != NULL) {
		return Blt_GetHashValue(hPtr);
	    }
	}
    case ITER_SINGLE:
	return iterPtr->elemPtr;
    } 
    return NULL;
}


/*
 *---------------------------------------------------------------------------
 *
 * GetElementByName --
 *
 *	Find the element represented the given name, returning a pointer to
 *	its data structure via elemPtrPtr.
 *
 * Results:
 *     	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
GetElementByName(Tcl_Interp *interp, Graph *graphPtr, const char *name, 
		 Element **elemPtrPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&graphPtr->elements.nameTable, name);
    if (hPtr == NULL) {
	if (interp != NULL) {
 	    Tcl_AppendResult(interp, "can't find element \"", name,
		"\" in \"", Tk_PathName(graphPtr->tkwin), "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    *elemPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * GetElementIterator --
 *
 *	Converts a string representing one or more elements into an iterator.  
 *	The string in one of the following forms:
 *
 *	 string		Name of element.
 *	 tag		All elements having the tag.
 *	 @x,y		Element nearest to the specified X-Y screen coordinates.
 *	 "active"	All active elements.
 *	 "all"		All elements.
 *	 "current"      Currently selected element.
 *	"name:string"   Element named "string".
 *	"tag:string"	Elements tagged by "string".
 *	"label:pattern"	Elements with label matching "pattern".
 *	
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.  The
 *	pointer to the element is returned via elemPtrPtr.  Otherwise, 
 *	TCL_ERROR is returned and an error message is left in interpreter's 
 *	result field.
 *
 *---------------------------------------------------------------------------
 */
static int
GetElementIterator(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
	       ElementIterator *iterPtr)
{
    Element *elemPtr;
    Blt_Chain chain;
    const char *string;
    char c;
    int numBytes;
    int length;

    iterPtr->graphPtr = graphPtr;
    iterPtr->type = ITER_SINGLE;
    iterPtr->link = NULL;
    iterPtr->tagName = Tcl_GetStringFromObj(objPtr, &numBytes);
    iterPtr->elemPtr = NULL;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    elemPtr = NULL;
    if (c == '\0') {
	elemPtr = NULL;
    } 
    if ((c == 'a') && (strcmp(iterPtr->tagName, "all") == 0)) {
	iterPtr->type  = ITER_ALL;
	iterPtr->tablePtr = &graphPtr->elements.nameTable;
    } else if ((c == 'c') && (strcmp(string, "current") == 0)) {
	GraphObj *objPtr;

	objPtr = Blt_GetCurrentItem(graphPtr->bindTable);
	/* Report only on elements. */
	if ((objPtr != NULL) && (!objPtr->deleted) &&
	    (objPtr->classId >= CID_ELEM_BAR) &&
	    (objPtr->classId <= CID_ELEM_STRIP)) {
	    iterPtr->type = ITER_SINGLE;
	    iterPtr->elemPtr = (Element *)objPtr;
	}
    } else if ((c == 'n') && (length > 5) && 
	       (strncmp(string, "name:", 5) == 0)) {
	if (GetElementByName(interp, graphPtr, string + 5, &elemPtr) != TCL_OK){
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "can't find an element named \"", 
			string + 5, "\" in \"", Tk_PathName(graphPtr->tkwin), 
			"\"", (char *)NULL);
	    }
	    return TCL_ERROR;
	}
	iterPtr->type = ITER_SINGLE;
	iterPtr->elemPtr = elemPtr;
    } else if ((c == 't') && (length > 4) && 
	       (strncmp(string, "tag:", 4) == 0)) {
	Blt_Chain chain;

	chain = Blt_Tags_GetItemList(&graphPtr->elements.tags, string + 4);
	if (chain == NULL) {
	    return TCL_OK;
	}
	iterPtr->tagName = string + 4;
	iterPtr->link = Blt_Chain_FirstLink(chain);
	iterPtr->type = ITER_TAG;
    } else if (GetElementByName(NULL, graphPtr, string, &elemPtr) == TCL_OK) {
	iterPtr->type = ITER_SINGLE;
	iterPtr->elemPtr = elemPtr;
    } else if ((chain = Blt_Tags_GetItemList(&graphPtr->elements.tags, string)) 
               != NULL) {
	iterPtr->tagName = string;
	iterPtr->link = Blt_Chain_FirstLink(chain);
	iterPtr->type = ITER_TAG;
    } else {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find element name or tag \"", 
		string, "\" in \"", Tk_PathName(graphPtr->tkwin), "\"", 
			     (char *)NULL);
	}
	return TCL_ERROR;
    }	
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetElementFromObj --
 *
 *	Gets the element associated the given index, tag, or label.  This
 *	routine is used when you want only one tab.  It's an error if more
 *	than one tab is specified (e.g. "all" tag).  It's also an error if the
 *	tag is empty (no tabs are currently tagged).
 *
 *---------------------------------------------------------------------------
 */
static int 
GetElementFromObj(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
		  Element **elemPtrPtr)
{
    ElementIterator iter;
    Element *firstPtr;

    if (GetElementIterator(interp, graphPtr, objPtr, &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    firstPtr = FirstTaggedElement(&iter);
    if (firstPtr != NULL) {
	Element *nextPtr;

	nextPtr = NextTaggedElement(&iter);
	if (nextPtr != NULL) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "multiple elements specified by \"", 
			Tcl_GetString(objPtr), "\"", (char *)NULL);
	    }
	    return TCL_ERROR;
	}
    }
    *elemPtrPtr = firstPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetIndex --
 *
 *	Given a string representing the index of a pair of x,y coordinates,
 *	return the numeric index.
 *
 * Results:
 *     	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
GetIndex(Tcl_Interp *interp, Element *elemPtr, Tcl_Obj *objPtr, int *indexPtr)
{
    char *string;

    string = Tcl_GetString(objPtr);
    if ((*string == 'e') && (strcmp("end", string) == 0)) {
	*indexPtr = NUMBEROFPOINTS(elemPtr) - 1;
    } else if (Blt_ExprIntFromObj(interp, objPtr, indexPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetElement --
 *
 *	Find the element represented the given name, returning a pointer to
 *	its data structure via elemPtrPtr.
 *
 * Results:
 *     	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetElement(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr, 
	       Element **elemPtrPtr)
{
    return GetElementFromObj(interp, graphPtr, objPtr, elemPtrPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyElement --
 *
 *	Add a new element to the graph.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyElement(Element *elemPtr)
{
    Graph *graphPtr = elemPtr->obj.graphPtr;

    elemPtr->obj.deleted = TRUE;	/* Mark it as deleted. */

    /* Remove the element for the graph's hash table of elements */
    if (elemPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&graphPtr->elements.nameTable, elemPtr->hashPtr);
    }
    /* Remove it also from the element display list */
    if (elemPtr->link != NULL) {
	Blt_Chain_DeleteLink(graphPtr->elements.displayList, elemPtr->link);
    }
    Blt_Tags_ClearTagsFromItem(&graphPtr->elements.tags, elemPtr);
    Blt_DeleteBindings(graphPtr->bindTable, elemPtr);
    Blt_Legend_RemoveElement(graphPtr, elemPtr);
    Blt_DeleteHashTable(&elemPtr->activeTable);
    Blt_FreeOptions(elemPtr->configSpecs, (char *)elemPtr,graphPtr->display, 0);
    /*
     * Call the element's own destructor to release the memory and resources
     * allocated for it.
     */
    (*elemPtr->procsPtr->destroyProc) (graphPtr, elemPtr);
    if (elemPtr->label != NULL) {
	Blt_Free(elemPtr->label);
    }
    Tcl_EventuallyFree(elemPtr, FreeElement);
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateElement --
 *
 *	Add a new element to the graph.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateElement(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv, ClassId classId)
{
    Element *elemPtr;
    Blt_HashEntry *hPtr;
    int isNew;
    const char *string, *prefix;

    switch (classId) {
    case CID_ELEM_BAR:
	prefix = "bar";		break;
    case CID_ELEM_STRIP:
	prefix = "strip";	break;
    case CID_ELEM_LINE:
	prefix = "line";	break;
    case CID_ELEM_CONTOUR:
	prefix = "contour";	break;
	break;
    default:
	Tcl_AppendResult(interp, "unknown element type (", Blt_Itoa(classId),
			 ")", (char *)NULL);
	return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]);
    if (string[0] == '-') {
	int i;

	for (i = 1; i < INT_MAX; i++) {
	    char ident[200];
	    int isNew;

	    /* Generate an element name. */
	    Blt_FormatString(ident, 200, "%s%d", prefix, i);
	    hPtr = Blt_CreateHashEntry(&graphPtr->elements.nameTable, ident, 
		&isNew);
	    if (isNew) {
		break;
	    }
	}
	assert(i < INT_MAX);
    } else {
	hPtr = Blt_CreateHashEntry(&graphPtr->elements.nameTable, string, 
				   &isNew);
	if (!isNew) {
	    Tcl_AppendResult(interp, "element \"", string, 
		"\" already exists in \"", Tcl_GetString(objv[0]), 
		"\"", (char *)NULL);
	    return TCL_ERROR;
	}
	objv++, objc--;
    }
    switch (classId) {
    case CID_ELEM_BAR:
	elemPtr = Blt_BarElement(graphPtr, hPtr);
	break;
    case CID_ELEM_STRIP:
    case CID_ELEM_LINE:
	/* Stripcharts are line graphs with different options. */	
#ifdef OLDLINES
	elemPtr = Blt_LineElement(graphPtr, classId, hPtr);
#else
	elemPtr = Blt_LineElement2(graphPtr, classId, hPtr);
#endif
	break;
    case CID_ELEM_CONTOUR:
	elemPtr = Blt_ContourElement(graphPtr, classId, hPtr);
	break;
    default:
	return TCL_ERROR;
    }
    Blt_InitHashTable(&elemPtr->activeTable, BLT_ONE_WORD_KEYS);
    elemPtr->numActiveIndices = -1;
    if (Blt_ConfigureComponentFromObj(interp, graphPtr->tkwin, 
	elemPtr->obj.name, "Element", elemPtr->configSpecs, objc-3, objv+3,
	(char *)elemPtr, 0) != TCL_OK) {
	DestroyElement(elemPtr);
	return TCL_ERROR;
    }
    (*elemPtr->procsPtr->configProc) (graphPtr, elemPtr);
    elemPtr->link = Blt_Chain_Append(graphPtr->elements.displayList, elemPtr);
    graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
    elemPtr->flags |= MAP_ITEM;
    graphPtr->flags |= RESET_AXES;
    Tcl_SetStringObj(Tcl_GetObjResult(interp), elemPtr->obj.name, -1);
    return TCL_OK;
}

void
Blt_DestroyElementTags(Graph *graphPtr)
{
    Blt_Tags_Reset(&graphPtr->elements.tags);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DestroyElements --
 *
 *	Removes all the graph's elements. This routine is called when the
 *	graph is destroyed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated for the graph's elements is freed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DestroyElements(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&graphPtr->elements.nameTable, &iter);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	Element *elemPtr;

	elemPtr = Blt_GetHashValue(hPtr);
	elemPtr->hashPtr = NULL;
	DestroyElement(elemPtr);
    }
    Blt_DeleteHashTable(&graphPtr->elements.nameTable);
    Blt_DeleteHashTable(&graphPtr->elements.bindTagTable);
    Blt_Tags_Reset(&graphPtr->elements.tags);
    Blt_Chain_Destroy(graphPtr->elements.displayList);
}

void
Blt_ConfigureElements(Graph *graphPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	(*elemPtr->procsPtr->configProc) (graphPtr, elemPtr);
    }
}

void
Blt_MapElements(Graph *graphPtr)
{
    Blt_ChainLink link;

    if (graphPtr->mode != BARS_INFRONT) {
	Blt_ResetGroups(graphPtr);
    }
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (IGNORE_ELEMENT(elemPtr)) {
	    continue;
	}
	if ((graphPtr->flags & MAP_ALL) || (elemPtr->flags & MAP_ITEM)) {
	    (*elemPtr->procsPtr->mapProc) (graphPtr, elemPtr);
	    elemPtr->flags &= ~MAP_ITEM;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawElements --
 *
 *	Calls the individual element drawing routines for each element.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Elements are drawn into the drawable (pixmap) which will eventually be
 *	displayed in the graph window.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DrawElements(Graph *graphPtr, Drawable drawable)
{
    Blt_ChainLink link;

    /* Draw with respect to the stacking order. */
    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if ((elemPtr->flags & HIDE) == 0) {
	    (*elemPtr->procsPtr->drawNormalProc)(graphPtr, drawable, elemPtr);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawActiveElements --
 *
 *	Calls the individual element drawing routines to display the active
 *	colors for each element.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Elements are drawn into the drawable (pixmap) which will eventually be
 *	displayed in the graph window.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DrawActiveElements(Graph *graphPtr, Drawable drawable)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if ((elemPtr->flags & (HIDE|ACTIVE)) == ACTIVE) {
	    (*elemPtr->procsPtr->drawActiveProc)(graphPtr, drawable, elemPtr);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ElementsToPostScript --
 *
 *	Generates PostScript output for each graph element in the element
 *	display list.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ElementsToPostScript(Graph *graphPtr, Blt_Ps ps)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->flags & HIDE) {
	    continue;
	}
	/* Comment the PostScript to indicate the start of the element */
	Blt_Ps_Format(ps, "\n%% Element \"%s\"\n\n", elemPtr->obj.name);
	(*elemPtr->procsPtr->printNormalProc) (graphPtr, ps, elemPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ActiveElementsToPostScript --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ActiveElementsToPostScript( Graph *graphPtr, Blt_Ps ps)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if ((elemPtr->flags & (HIDE|ACTIVE)) == ACTIVE) {
	    Blt_Ps_Format(ps, "\n%% Active Element \"%s\"\n\n", 
		elemPtr->obj.name);
	    (*elemPtr->procsPtr->printActiveProc)(graphPtr, ps, elemPtr);
	}
    }
}



/*
 *---------------------------------------------------------------------------
 *
 * ActiveClearOp --
 *
 *	Clears the active indices for the named element.
 *
 * Results:
 *	Returns TCL_OK if no errors occurred.
 *
 *	.g element active clear $elem
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ActiveClearOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Element *elemPtr;
    ElementIterator iter;

    if (GetElementIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	 elemPtr = NextTaggedElement(&iter)) {
	elemPtr->flags &= ~(ACTIVE | ACTIVE_PENDING);
	Blt_DeleteHashTable(&elemPtr->activeTable);
	Blt_InitHashTable(&elemPtr->activeTable, BLT_ONE_WORD_KEYS);
	elemPtr->numActiveIndices = 0;
	elemPtr->flags |= (ACTIVE | ACTIVE_PENDING);
	Blt_EventuallyRedrawGraph(graphPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActiveIndicesOp --
 *
 *	Returns the indices off all the active points in the element.
 *
 * Results:
 *	Returns TCL_OK if no errors occurred.
 *
 *	.g element active indices $elem
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ActiveIndicesOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Element *elemPtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Obj *listObjPtr;

    if (GetElementFromObj(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Blt_FirstHashEntry(&elemPtr->activeTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	long lindex;

	lindex = (long)Blt_GetHashValue(hPtr);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewLongObj(lindex));
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActiveSetOp --
 *
 *	Activates the points of the designated indices in the element.
 *
 * Results:
 *	Returns TCL_OK if no errors occurred.
 *
 *	.g element active set $elem ?indices...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ActiveSetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Element *elemPtr;
    int i;

    if (GetElementFromObj(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    for (i = 5; i < objc; i++) {
	int index;
	long lindex;
	Blt_HashEntry *hPtr;
	int isNew;

	if (GetIndex(interp, elemPtr, objv[i], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	lindex = (long)index;
	hPtr = Blt_CreateHashEntry(&elemPtr->activeTable, (char *)lindex, 
				   &isNew);
	if (hPtr == NULL) {
	    Tcl_AppendResult(interp, "can't set index \"", 
		Tcl_GetString(objv[i]), "\" to active.", (char *)NULL);
	    return TCL_ERROR;
	}
	Blt_SetHashValue(hPtr, lindex);
    }
    elemPtr->flags |= (ACTIVE | ACTIVE_PENDING);
    elemPtr->numActiveIndices = elemPtr->activeTable.numEntries;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActiveToggleOp --
 *
 *	Toggle the activation of the points of the designated indices in 
 *	the element.
 *
 * Results:
 *	Returns TCL_OK if no errors occurred.
 *
 *	.g element active toggle $elem ?indices...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ActiveToggleOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Element *elemPtr;
    int i;

    if (GetElementFromObj(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    for (i = 5; i < objc; i++) {
	int index;
	long lindex;
	Blt_HashEntry *hPtr;
	int isNew;

	if (GetIndex(interp, elemPtr, objv[i], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	lindex = (long)index;
	hPtr = Blt_CreateHashEntry(&elemPtr->activeTable, (char *)lindex, 
				   &isNew);
	if (hPtr == NULL) {
	    Tcl_AppendResult(interp, "can't set index \"", 
		Tcl_GetString(objv[i]), "\" to active.", (char *)NULL);
	    return TCL_ERROR;
	}
	if (!isNew) {
	    Blt_DeleteHashEntry(&elemPtr->activeTable, hPtr);
	} else {
	    Blt_SetHashValue(hPtr, lindex);
	}
    }
    elemPtr->flags |= (ACTIVE | ACTIVE_PENDING);
    elemPtr->numActiveIndices = elemPtr->activeTable.numEntries;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActiveUnsetOp --
 *
 *	Activates the points of the designated indices in the element.
 *
 * Results:
 *	Returns TCL_OK if no errors occurred.
 *
 *	.g element active unset $elem ?indices...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ActiveUnsetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Element *elemPtr;
    int i;

    if (GetElementFromObj(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    for (i = 5; i < objc; i++) {
	int index;
	long lindex;
	Blt_HashEntry *hPtr;

	if (GetIndex(interp, elemPtr, objv[i], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	lindex = (long)index;
	hPtr = Blt_FindHashEntry(&elemPtr->activeTable, (char *)lindex);
	if (hPtr == NULL) {
	    continue;
	}
	Blt_DeleteHashEntry(&elemPtr->activeTable, hPtr);
    }
    elemPtr->flags |= (ACTIVE | ACTIVE_PENDING);
    elemPtr->numActiveIndices = elemPtr->activeTable.numEntries;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActiveOp --
 *
 *	Marks data points of elements (given by their index) as active.
 *
 * Results:
 *	Returns TCL_OK if no errors occurred.
 *
 *	.g element active set $elem $indices
 *	.g element active unset $elem $indices
 *	.g element active clear $elem
 *	.g element active indices $elem
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec activeOps[] =
{
    {"clear",        1, ActiveClearOp,     5, 5, "elemName",},
    {"indices",      1, ActiveIndicesOp,   5, 5, "elemName",},
    {"set",          1, ActiveSetOp,	   5, 0, "elemName ?indices...?",},
    {"toggle",       1, ActiveToggleOp,	   5, 0, "elemName ?indices...?",},
    {"unset",        1, ActiveUnsetOp,     5, 0, "elemName ?indices...?",},
};
static int numActiveOps = sizeof(activeOps) / sizeof(Blt_OpSpec);

static int
ActiveOp(
    Graph *graphPtr,			/* Graph widget */
    Tcl_Interp *interp,			/* Interpreter to report errors to */
    int objc,				/* Number of element names */
    Tcl_Obj *const *objv)		/* List of element names */
{
    GraphElementProc *proc;

    proc = Blt_GetOpFromObj(interp, numActiveOps, activeOps, BLT_OP_ARG3, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (graphPtr, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 *	Marks data points of elements (given by their index) as active.
 *
 * Results:
 *	Returns TCL_OK if no errors occurred.
 *
 *	.g element activate
 *	.g element activate $elem 
 *	.g element activate $elem $indices
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(
    Graph *graphPtr,			/* Graph widget */
    Tcl_Interp *interp,			/* Interpreter to report errors to */
    int objc,				/* Number of element names */
    Tcl_Obj *const *objv)		/* List of element names */
{

    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	/* List all the currently active elements */
	for (hPtr = Blt_FirstHashEntry(&graphPtr->elements.nameTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Element *elemPtr;	    

	    elemPtr = Blt_GetHashValue(hPtr);
	    if (elemPtr->flags & ACTIVE) {
		Tcl_ListObjAppendElement(interp, listObjPtr, 
			Tcl_NewStringObj(elemPtr->obj.name, -1));
	    }
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    } else if (objc == 4) {
	Element *elemPtr;
	ElementIterator iter;

	if (GetElementIterator(NULL, graphPtr, objv[3], &iter) != TCL_OK) {
	    return TCL_OK;		/* Can't find named tag or element.
					 * Just ignore the request. */
	}
	for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	     elemPtr = NextTaggedElement(&iter)) {
	    Blt_DeleteHashTable(&elemPtr->activeTable);
	    Blt_InitHashTable(&elemPtr->activeTable, BLT_ONE_WORD_KEYS);
	    elemPtr->numActiveIndices = -1;
	    elemPtr->flags |= ACTIVE | ACTIVE_PENDING;
	    Blt_EventuallyRedrawGraph(graphPtr);
	}
    } else if (objc > 4) {
	int i;
	int numIndices;
	Element *elemPtr;

	if (Blt_GetElement(NULL, graphPtr, objv[3], &elemPtr) != TCL_OK) {
	    return TCL_OK;		/* Can't find named tag or element.
					 * Just ignore the request. */
	}
	for (i = 4; i < objc; i++) {
	    int index, isNew;
	    long lindex;
	    Blt_HashEntry *hPtr;

	    if (GetIndex(interp, elemPtr, objv[i], &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    lindex = (long)index;
	    hPtr = Blt_CreateHashEntry(&elemPtr->activeTable, (char *)lindex, 
		&isNew);
	    if (hPtr == NULL) {
		Tcl_AppendResult(interp, "can't set index \"", 
			Tcl_GetString(objv[i]), "\" to active.", (char *)NULL);
		return TCL_ERROR;
	    }
	}
	numIndices = elemPtr->activeTable.numEntries;
	elemPtr->numActiveIndices = numIndices;
	elemPtr->flags |= ACTIVE | ACTIVE_PENDING;
	Blt_EventuallyRedrawGraph(graphPtr);
    }
    return TCL_OK;
}

ClientData
Blt_MakeElementTag(Graph *graphPtr, const char *tagName)
{
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&graphPtr->elements.bindTagTable, tagName, 
	&isNew);
    return Blt_GetHashKey(&graphPtr->elements.bindTagTable, hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * BindOp --
 *
 *	.g element bind elemName sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (hPtr = Blt_FirstHashEntry(&graphPtr->elements.bindTagTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    const char *tagName;
	    Tcl_Obj *objPtr;

	    tagName = Blt_GetHashKey(&graphPtr->elements.bindTagTable, hPtr);
	    objPtr = Tcl_NewStringObj(tagName, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable,
	Blt_MakeElementTag(graphPtr, Tcl_GetString(objv[3])), 
	objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *	Add a new element to the graph (using the default type of the graph).
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv,
    ClassId classId)
{
    return CreateElement(graphPtr, interp, objc, objv, classId);
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    Element *elemPtr;

    if (GetElementFromObj(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    if (Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, elemPtr->configSpecs,
				  (char *)elemPtr, objv[4], 0) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NearestOp --
 *
 *	Find the element nearest to the specified screen coordinates.
 *	Options:
 *	-halo		Consider points only with this maximum distance
 *			from the picked coordinate.
 *	-interpolate	Find nearest point along element traces, not just
 *			data points.
 *	-along
 *	-all		Return the elements and points of every point 
 *			within the halo.
 *
 * Results:
 *	A standard TCL result. If an element could be found within the halo
 *	distance, the interpreter result is "1", otherwise "0".  If a nearest
 *	element exists, the designated TCL array variable will be set with the
 *	following information:
 *
 *	1) the element name,
 *	2) the index of the nearest point,
 *	3) the distance (in screen coordinates) from the picked X-Y
 *	   coordinate and the nearest point,
 *	4) the X coordinate (graph coordinate) of the nearest point,
 *	5) and the Y-coordinate.
 *
 *	.g element nearest x y ?switches? ?element...?
 *	
 *---------------------------------------------------------------------------
 */

static Blt_ConfigSpec nearestSpecs[] = {
    {BLT_CONFIG_PIXELS_NNEG, "-halo", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(NearestElement, halo), 0},
    {BLT_CONFIG_BOOLEAN, "-interpolate", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(NearestElement, mode), 0 }, 
    {BLT_CONFIG_CUSTOM, "-along", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(NearestElement, along), 0, &alongOption},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static int
NearestOp(
    Graph *graphPtr,			/* Graph widget */
    Tcl_Interp *interp,			/* Interpreter to report results to */
    int objc,				/* # of arguments */
    Tcl_Obj *const *objv)		/* List of element names */
{
    Element *elemPtr;
    NearestElement nearest;
    int i, x, y;
    char *string;

    if (graphPtr->flags & RESET_AXES) {
	Blt_ResetAxes(graphPtr);
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) {
	Tcl_AddErrorInfo(interp, "\n    (bad window x-coordinate)");
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
	Tcl_AddErrorInfo(interp, "\n    (bad window y-coordinate)");
	return TCL_ERROR;
    }
    for (i = 5; i < objc; i += 2) {	/* Count switches-value pairs */
	string = Tcl_GetString(objv[i]);
	if ((string[0] != '-') || 
	    ((string[1] == '-') && (string[2] == '\0'))) {
	    break;
	}
    }
    if (i > objc) {
	i = objc;
    }

    memset(&nearest, 0, sizeof(NearestElement));
    nearest.mode = NEAREST_SEARCH_POINTS;
    nearest.halo = graphPtr->halo;
    nearest.along = NEAREST_SEARCH_XY;
    nearest.x = x;
    nearest.y = y;

    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, nearestSpecs, i - 5,
	objv + 5, (char *)&nearest, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;		/* Error occurred processing an
					 * option. */
    }
    if (i < objc) {
	string = Tcl_GetString(objv[i]);
	if (string[0] == '-') {
	    i++;			/* Skip "--" */
	}
    }
    nearest.maxDistance = nearest.halo;
    nearest.distance = nearest.maxDistance + 1;

    if (i < objc) {
	for ( /* empty */ ; i < objc; i++) {
	    ElementIterator iter;
	    Element *elemPtr;

	    if (GetElementIterator(interp, graphPtr, objv[i], &iter) != TCL_OK){
		return TCL_ERROR;
	    }
	    for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
		 elemPtr = NextTaggedElement(&iter)) {
		if (IGNORE_ELEMENT(elemPtr)) {
		    continue;
		}
		if (elemPtr->flags & (HIDE|MAP_ITEM)) {
		    continue;
		}
		(*elemPtr->procsPtr->nearestProc) (graphPtr, elemPtr, &nearest);
	    }
	}
    } else {
	Blt_ChainLink link;

	/* 
	 * Find the nearest point from the set of displayed elements,
	 * searching the display list from back to front.  That way if the
	 * points from two different elements overlay each other exactly, the
	 * last one picked will be the topmost.
	 */
	for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	     link != NULL; link = Blt_Chain_PrevLink(link)) {
	    elemPtr = Blt_Chain_GetValue(link);
	    if (elemPtr->flags & (HIDE|MAP_ITEM)) {
		continue;
	    }
	    (*elemPtr->procsPtr->nearestProc)(graphPtr, elemPtr, &nearest);
	}
    }
    if (nearest.distance <= nearest.maxDistance) {
	Tcl_Obj *objPtr, *listObjPtr;
	Element *elemPtr;

	/* Return a list of name value pairs. */
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

	elemPtr = nearest.item;
	/* Name of nearest element. */
	objPtr = Tcl_NewStringObj("name", 4);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1); 
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

	/* Index of nearest data point in element.  */
	objPtr = Tcl_NewStringObj("index", 5);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	objPtr = Tcl_NewIntObj(nearest.index);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
		
	/* X-coordindate of nearest data point. */
	objPtr = Tcl_NewStringObj("x", 1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	objPtr = Tcl_NewDoubleObj(nearest.point.x);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

	/* Y-coordindate of nearest data point. */
	objPtr = Tcl_NewStringObj("y", 1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	objPtr = Tcl_NewDoubleObj(nearest.point.y);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
		
	/* Distance to nearest point. */
	objPtr = Tcl_NewStringObj("dist", 4);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	objPtr = Tcl_NewDoubleObj(nearest.distance);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
		
	Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ClosestOp --
 *
 *	Find the element nearest to the specified screen coordinates.
 *	Options:
 *	-halo		Consider points only with this maximum distance
 *			from the picked coordinate.
 *	-interpolate	Find nearest point along element traces, not just
 *			data points.
 *	-along
 *	-all		Return the elements and points of every point 
 *			within the halo.
 *
 * Results:
 *	A standard TCL result. If an element could be found within the halo
 *	distance, the interpreter result is "1", otherwise "0".  If a nearest
 *	element exists, the designated TCL array variable will be set with the
 *	following information:
 *
 *	1) the element name,
 *	2) the index of the nearest point,
 *	3) the distance (in screen coordinates) from the picked X-Y
 *	   coordinate and the nearest point,
 *	4) the X coordinate (graph coordinate) of the nearest point,
 *	5) and the Y-coordinate.
 *
 *	.g element closest x y varName ?switches? elements
 *	
 *---------------------------------------------------------------------------
 */

static int
ClosestOp(
    Graph *graphPtr,			/* Graph widget */
    Tcl_Interp *interp,			/* Interpreter to report results to */
    int objc,				/* # of arguments */
    Tcl_Obj *const *objv)		/* List of element names */
{
    NearestElement nearest;
    int i, x, y;
    const char *string;
    const char *varName;

    if (graphPtr->flags & RESET_AXES) {
	Blt_ResetAxes(graphPtr);
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) {
	Tcl_AddErrorInfo(interp, "\n    (bad window x-coordinate)");
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) {
	Tcl_AddErrorInfo(interp, "\n    (bad window y-coordinate)");
	return TCL_ERROR;
    }
    varName = Tcl_GetString(objv[5]);
    for (i = 6; i < objc; i += 2) {	/* Count switches-value pairs */
	string = Tcl_GetString(objv[i]);
	if ((string[0] != '-') || 
	    ((string[1] == '-') && (string[2] == '\0'))) {
	    break;
	}
    }
    if (i > objc) {
	i = objc;
    }

    memset(&nearest, 0, sizeof(NearestElement));
    nearest.mode = NEAREST_SEARCH_POINTS;
    nearest.halo = graphPtr->halo;
    nearest.along = NEAREST_SEARCH_XY;
    nearest.x = x;
    nearest.y = y;

    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, nearestSpecs, i - 6,
	objv + 6, (char *)&nearest, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;		/* Error occurred processing an
					 * option. */
    }
    if (i < objc) {
	string = Tcl_GetString(objv[i]);
	if (string[0] == '-') {
	    i++;			/* Skip "--" */
	}
    }
    nearest.maxDistance = nearest.halo;
    nearest.distance = nearest.maxDistance + 1;

    if (i < objc) {
	for ( /* empty */ ; i < objc; i++) {
	    ElementIterator iter;
	    Element *elemPtr;

	    if (GetElementIterator(interp, graphPtr, objv[i], &iter) != TCL_OK){
		return TCL_ERROR;
	    }
	    for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
		 elemPtr = NextTaggedElement(&iter)) {

		if (IGNORE_ELEMENT(elemPtr)) {
		    continue;
		}
		if (elemPtr->flags & (HIDE|MAP_ITEM)) {
		    continue;
		}
		(*elemPtr->procsPtr->nearestProc) (graphPtr, elemPtr, &nearest);
	    }
	}
    } else {
	Blt_ChainLink link;

	/* 
	 * Find the nearest point from the set of displayed elements,
	 * searching the display list from back to front.  That way if the
	 * points from two different elements overlay each other exactly, the
	 * last one picked will be the topmost.
	 */
	for (link = Blt_Chain_LastLink(graphPtr->elements.displayList); 
	     link != NULL; link = Blt_Chain_PrevLink(link)) {
	    Element *elemPtr;

	    elemPtr = Blt_Chain_GetValue(link);
	    if (elemPtr->flags & (HIDE|MAP_ITEM)) {
		continue;
	    }
	    (*elemPtr->procsPtr->nearestProc)(graphPtr, elemPtr, &nearest);
	}
    }
    if (nearest.distance <= nearest.maxDistance) {
	Tcl_Obj *objPtr;
	int flags = TCL_LEAVE_ERR_MSG;
	Element *elemPtr;
	/*
	 *  Return an array of 5 elements
	 */
	elemPtr = nearest.item;
	objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
	if (Tcl_SetVar2Ex(interp, varName, "name", objPtr, flags) == NULL) {
	    return TCL_ERROR;
	}
	objPtr = Tcl_NewIntObj(nearest.index);
	if (Tcl_SetVar2Ex(interp, varName, "index", objPtr, flags) == NULL) {
	    return TCL_ERROR;
	}
	objPtr = Tcl_NewDoubleObj(nearest.point.x);
	if (Tcl_SetVar2Ex(interp, varName, "x", objPtr, flags) == NULL) {
	    return TCL_ERROR;
	}
	objPtr = Tcl_NewDoubleObj(nearest.point.y);
	if (Tcl_SetVar2Ex(interp, varName, "y", objPtr, flags) == NULL) {
	    return TCL_ERROR;
	}
	objPtr = Tcl_NewDoubleObj(nearest.distance);
	if (Tcl_SetVar2Ex(interp, varName, "dist", objPtr, flags) == NULL) {
	    return TCL_ERROR;
	}
	Tcl_SetBooleanObj(Tcl_GetObjResult(interp), 1);
    } else {
	int flags = TCL_LEAVE_ERR_MSG;
	if (Tcl_SetVar2(interp, varName, "name", "", flags) == NULL) {
	    return TCL_ERROR;
	}
	Tcl_SetBooleanObj(Tcl_GetObjResult(interp), 0);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *	Sets the element specifications by the given the command line
 *	arguments and calls the element specification configuration
 *	routine. If zero or one command line options are given, only
 *	information about the option(s) is returned in interp->result.  If the
 *	element configuration has changed and the element is currently
 *	displayed, the axis limits are updated and recomputed.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new display list.
 *
 *	$g element configure $elem ?value options?
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    Element *elemPtr;
    ElementIterator iter;

    /* Figure out where the option value pairs begin */
    if (objc == 4) {
	if (GetElementFromObj(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;		/* Can't find named element */
	}
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
		elemPtr->configSpecs, (char *)elemPtr, (Tcl_Obj *)NULL, 
		BLT_CONFIG_OBJV_ONLY);
    } else if (objc == 5) {
	if (GetElementFromObj(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
	    return TCL_ERROR;		/* Can't find named element */
	}
	return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
		elemPtr->configSpecs, (char *)elemPtr, objv[4], 
		BLT_CONFIG_OBJV_ONLY);

    } 
    if (GetElementIterator(interp, graphPtr, objv[3], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	 elemPtr = NextTaggedElement(&iter)) {

	if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, 
		elemPtr->configSpecs, objc - 4, objv + 4, (char *)elemPtr, 
		BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((*elemPtr->procsPtr->configProc) (graphPtr, elemPtr) != TCL_OK) {
	    return TCL_ERROR;		/* Failed to configure element */
	}
	if (Blt_ConfigModified(elemPtr->configSpecs, "-hide", (char *)NULL)) {
	    graphPtr->flags |= RESET_AXES;
	    elemPtr->flags |= MAP_ITEM;
	}
	/* If data points or axes have changed, reset the axes (may affect
	 * autoscaling) and recalculate the screen points of the element. */

	if (Blt_ConfigModified(elemPtr->configSpecs, "-*data", "-map*", "-x",
		"-y", (char *)NULL)) {
	    graphPtr->flags |= RESET_WORLD;
	    elemPtr->flags |= MAP_ITEM;
	}
	/* The new label may change the size of the legend */
	if (Blt_ConfigModified(elemPtr->configSpecs, "-label", (char *)NULL)) {
	    graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
	}
    }
    /* Update the pixmap if any configuration option changed */
    graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeactivateOp --
 *
 *	Clears the active bit for the named elements.
 *
 * Results:
 *	Returns TCL_OK if no errors occurred.
 *
 *	.g element deactivate elem1 elem2 elem3...
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeactivateOp(
    Graph *graphPtr,			/* Graph widget */
    Tcl_Interp *interp,			/* Not used. */
    int objc,				/* Number of element names */
    Tcl_Obj *const *objv)		/* List of element names */
{
    int i;

    for (i = 3; i < objc; i++) {
	Element *elemPtr;
	ElementIterator iter;

	if (GetElementIterator(NULL, graphPtr, objv[i], &iter) != TCL_OK) {
	    continue;			/* Can't find named tag or element.
					 * Just ignore the request. */
	}
	for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	     elemPtr = NextTaggedElement(&iter)) {
	    elemPtr->flags &= ~(ACTIVE | ACTIVE_PENDING);
	    if (elemPtr->activeTable.numEntries > 0) {
		Blt_DeleteHashTable(&elemPtr->activeTable);
		Blt_InitHashTable(&elemPtr->activeTable, BLT_ONE_WORD_KEYS);
	    }
	    elemPtr->numActiveIndices = -1;
	    Blt_EventuallyRedrawGraph(graphPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *	Delete the named elements from the graph.
 *
 * Results:
 *	TCL_ERROR is returned if any of the named elements can not be found.
 *	Otherwise TCL_OK is returned;
 *
 * Side Effects:
 *	If the element is currently displayed, the plotting area of the graph
 *	is redrawn. Memory and resources allocated by the elements are
 *	released.
 *
 *	$g element delete tagOrName...
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(
    Graph *graphPtr,			/* Graph widget */
    Tcl_Interp *interp,			/* Not used. */
    int objc,				/* Number of element names */
    Tcl_Obj *const *objv)		/* List of element names */
{
    int i;
    Blt_HashTable selected;
    Blt_HashSearch cursor;
    Blt_HashEntry *hPtr;

    Blt_InitHashTable(&selected, BLT_ONE_WORD_KEYS);
    for (i = 3; i < objc; i++) {
	Element *elemPtr;
	ElementIterator iter;

	if (GetElementIterator(interp, graphPtr, objv[i], &iter) != TCL_OK) {
	    Blt_DeleteHashTable(&selected);
	    return TCL_ERROR;
	}
	for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	     elemPtr = NextTaggedElement(&iter)) {
	    int isNew;
	    Blt_HashEntry *hPtr;

	    hPtr = Blt_CreateHashEntry(&selected, (char *)elemPtr, &isNew);
	    Blt_SetHashValue(hPtr, elemPtr);
	}
    }
    for (hPtr = Blt_FirstHashEntry(&selected, &cursor); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&cursor)) {
	Element *elemPtr;

	elemPtr = Blt_GetHashValue(hPtr);
	DestroyElement(elemPtr);
	graphPtr->flags |= RESET_WORLD;
	Blt_EventuallyRedrawGraph(graphPtr);
    }
    Blt_DeleteHashTable(&selected);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *	Indicates if the named element exists in the graph.
 *
 * Results:
 *	The return value is a standard TCL result.  The interpreter result
 *	will contain "1" or "0".
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ExistsOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    const char *name;

    name = Tcl_GetString(objv[3]);
    hPtr = Blt_FindHashEntry(&graphPtr->elements.nameTable, name);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (hPtr != NULL));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindOp --
 *
 *	Indicates if the named element exists in the graph.
 *
 * Results:
 *	The return value is a standard TCL result.  The interpreter result
 *	will contain "1" or "0".
 *
 *	.g element find $elem x1 y1 x2 y2
 *	.g element find $elem xCenter yCenter radius
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
FindOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Element *elemPtr;

    if (Blt_GetElement(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    if (objc == 7) {
	Blt_Chain chain;
	Blt_ChainLink link;
	Tcl_Obj *listObjPtr;
	int x, y, r;

	if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
	    (Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK) ||
	    (Tcl_GetIntFromObj(interp, objv[6], &r) != TCL_OK)) {
	    return TCL_ERROR;
	}
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	chain = (*elemPtr->procsPtr->findProc)(graphPtr, elemPtr, x, y, r);
	for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    long i;

	    i = (long)Blt_Chain_GetValue(link);
	    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewLongObj(i));
	}
	Blt_Chain_Destroy(chain);
	Tcl_SetObjResult(interp, listObjPtr);
    } else if (objc == 8) {
	int numPoints;
	int i;
	Tcl_Obj *listObjPtr;
	double *x, *y;
	double x1, x2, y1, y2, tmp;

	if ((Tcl_GetDoubleFromObj(interp, objv[4], &x1) != TCL_OK) ||
	    (Tcl_GetDoubleFromObj(interp, objv[5], &y1) != TCL_OK) ||
	    (Tcl_GetDoubleFromObj(interp, objv[6], &x2) != TCL_OK) ||
	    (Tcl_GetDoubleFromObj(interp, objv[7], &y2) != TCL_OK)) {
	    return TCL_ERROR;
	}
	if (x1 > x2) {
	    tmp = x1, x1 = x2, x2 = tmp;
	}
	if (y1 > y2) {
	    tmp = y1, y1 = y2, y2 = tmp;
	}
	numPoints = NUMBEROFPOINTS(elemPtr);
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	x = elemPtr->x.values;
	y = elemPtr->y.values;
	for (i = 0; i < numPoints; i++) {
	    if ((!FINITE(x[i])) || (!FINITE(y[i]))) {
		continue;
	    }
	    if ((x[i] < x1) || (x[i] > x2) || (y[i] < y1) || (y[i] > y2)) {
		continue;
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(i));
	}
	Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 * 	Returns the name of the picked element (using the element * bind
 * 	operation).  Right now, the only name accepted is "current".
 *
 * Results:
 *	A standard TCL result.  The interpreter result will contain the name
 *	of the element.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GetOp(
    Graph *graphPtr,
    Tcl_Interp *interp,
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    char *string;

    string = Tcl_GetString(objv[3]);
    if ((string[0] == 'c') && (strcmp(string, "current") == 0)) {
	GraphObj *objPtr;

	objPtr = Blt_GetCurrentItem(graphPtr->bindTable);
	/* Report only on elements. */
	if ((objPtr != NULL) && (!objPtr->deleted) &&
	    (objPtr->classId >= CID_ELEM_BAR) &&
	    (objPtr->classId <= CID_ELEM_STRIP)) {
	    Tcl_SetStringObj(Tcl_GetObjResult(interp), objPtr->name, -1);
	}
    }
    return TCL_OK;
}

static Tcl_Obj *
DisplayListObj(Graph *graphPtr)
{
    Tcl_Obj *listObjPtr;
    Blt_ChainLink link;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;
	Tcl_Obj *objPtr;

	elemPtr = Blt_Chain_GetValue(link);
	objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
	Tcl_ListObjAppendElement(graphPtr->interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * LowerOp --
 *
 *	Lowers the named elements to the bottom of the display list.
 *
 * Results:
 *	A standard TCL result. The interpreter result will contain the new
 *	display list of element names.
 *
 *	.g element lower elem ?elem...?
 *
 *---------------------------------------------------------------------------
 */
static int
LowerOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_Chain chain;
    Blt_ChainLink link, next;
    Blt_HashTable selected;
    int i;

    Blt_InitHashTable(&selected, BLT_ONE_WORD_KEYS);
    chain = Blt_Chain_Create();
    /* Move the links of lowered elements out of the display list into a
     * temporary list. */
    for (i = 3; i < objc; i++) {
	Element *elemPtr;
	ElementIterator iter;

	if (GetElementIterator(interp, graphPtr, objv[i], &iter) != TCL_OK) {
	    Blt_DeleteHashTable(&selected);
	    return TCL_ERROR;
	}
	for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	     elemPtr = NextTaggedElement(&iter)) {
	    int isNew;

	    Blt_CreateHashEntry(&selected, (char *)elemPtr, &isNew);
	    if (isNew) {
		Blt_Chain_UnlinkLink(graphPtr->elements.displayList, 
				     elemPtr->link); 
		Blt_Chain_LinkAfter(chain, elemPtr->link, NULL); 
	    }
	}
    }
    Blt_DeleteHashTable(&selected);
    /* Append the links to end of the display list. */
    for (link = Blt_Chain_FirstLink(chain); link != NULL; link = next) {
	next = Blt_Chain_NextLink(link);
	Blt_Chain_UnlinkLink(chain, link); 
	Blt_Chain_LinkAfter(graphPtr->elements.displayList, link, NULL); 
    }	
    Blt_Chain_Destroy(chain);
    Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
    graphPtr->flags |= RESET_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *	Returns the names of the elements is the graph matching one of more
 *	patterns provided.  If no pattern arguments are given, then all
 *	element names will be returned.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter result will
 *	contain a TCL list of the element names.
 *
 *---------------------------------------------------------------------------
 */
static int
NamesOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&graphPtr->elements.nameTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Element *elemPtr;
	    Tcl_Obj *objPtr;

	    elemPtr = Blt_GetHashValue(hPtr);
	    objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    } else {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&graphPtr->elements.nameTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Element *elemPtr;
	    int i;

	    elemPtr = Blt_GetHashValue(hPtr);
	    for (i = 3; i < objc; i++) {
		if (Tcl_StringMatch(elemPtr->obj.name,Tcl_GetString(objv[i]))) {
		    Tcl_Obj *objPtr;

		    objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
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
 * RaiseOp --
 *
 *	Reset the element within the display list.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter result will
 *	contain the new display list of element names.
 *
 *	.g element raise ?elem...?
 *
 *---------------------------------------------------------------------------
 */
static int
RaiseOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_Chain chain;
    Blt_ChainLink link, prev;
    Blt_HashTable selected;
    int i;

    Blt_InitHashTable(&selected, BLT_ONE_WORD_KEYS);
    chain = Blt_Chain_Create();
    /* Move the links of lowered elements out of the display list into a
     * temporary list. */
    for (i = 3; i < objc; i++) {
	Element *elemPtr;
	ElementIterator iter;

	if (GetElementIterator(interp, graphPtr, objv[i], &iter) != TCL_OK) {
	    Blt_DeleteHashTable(&selected);
	    return TCL_ERROR;
	}
	for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	     elemPtr = NextTaggedElement(&iter)) {
	    int isNew;

	    Blt_CreateHashEntry(&selected, (char *)elemPtr, &isNew);
	    if (isNew) {
		Blt_Chain_UnlinkLink(graphPtr->elements.displayList, 
				     elemPtr->link); 
		Blt_Chain_LinkAfter(chain, elemPtr->link, NULL); 
	    }
	}
    }
    Blt_DeleteHashTable(&selected);
    /* Prepend the links to beginning of the display list in reverse order. */
    for (link = Blt_Chain_LastLink(chain); link != NULL; link = prev) {
	prev = Blt_Chain_PrevLink(link);
	Blt_Chain_UnlinkLink(chain, link); 
	Blt_Chain_LinkBefore(graphPtr->elements.displayList, link, NULL); 
    }	
    Blt_Chain_Destroy(chain);
    Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
    graphPtr->flags |= RESET_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ShowOp --
 *
 *	Map one of more elements. Queries or resets the element display list.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter result will
 *	contain the new display list of element names.
 *
 *---------------------------------------------------------------------------
 */
static int
ShowOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_Chain chain;
    Blt_ChainLink link;
    Tcl_Obj **elem;
    int i, n;
    Blt_HashTable selected;
    
    if (objc == 3) {
	Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
	return TCL_OK;
    }
    /* Map the named elements */
    if (Tcl_ListObjGetElements(interp, objv[3], &n, &elem) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Collect the named elements into a list. */
    chain = Blt_Chain_Create();
    Blt_InitHashTable(&selected, BLT_ONE_WORD_KEYS);
    for (i = 0; i < n; i++) {
	Element *elemPtr;
	ElementIterator iter;
	
	if (GetElementIterator(interp, graphPtr, elem[i], &iter) != TCL_OK){
	    goto error;
	}
	for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	     elemPtr = NextTaggedElement(&iter)) {
	    int isNew;
	    
	    Blt_CreateHashEntry(&selected, (char *)elemPtr, &isNew);
	    if (isNew) {
		Blt_Chain_Append(chain, elemPtr);
	    }
	}
    }
    Blt_DeleteHashTable(&selected);
    /* Start by unmapping the currently displayed elements (remove them
     * from the display list).  */
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList); 
	 link != NULL; link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;
	
	elemPtr = Blt_Chain_GetValue(link);
	elemPtr->link = NULL;
    }
    Blt_Chain_Destroy(graphPtr->elements.displayList);
    graphPtr->elements.displayList = chain;
    /* Now add the elements to the display list.  */
    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	Element *elemPtr;
	
	elemPtr = Blt_Chain_GetValue(link);
	elemPtr->link = link;
    }
    graphPtr->flags |= RESET_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    Tcl_SetObjResult(interp, DisplayListObj(graphPtr));
    return TCL_OK;
 error:
    Blt_DeleteHashTable(&selected);
    Blt_Chain_Destroy(chain);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagAddOp --
 *
 *	.g element tag add tagName elem1 elem2 elem2 elem4
 *
 *---------------------------------------------------------------------------
 */
static int
TagAddOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    const char *tag;

    tag = Tcl_GetString(objv[4]);
    if (strcmp(tag, "all") == 0) {
	Tcl_AppendResult(interp, "can't add reserved tag \"", tag, "\"", 
			 (char *)NULL);
	return TCL_ERROR;
    }
    if (objc == 5) {
	/* No elements specified.  Just add the tag. */
	Blt_Tags_AddTag(&graphPtr->elements.tags, tag);
    } else {
	int i;

	for (i = 5; i < objc; i++) {
	    Element *elemPtr;
	    ElementIterator iter;
	    
	    if (GetElementIterator(interp, graphPtr, objv[i], &iter) != TCL_OK){
		return TCL_ERROR;
	    }
	    for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
		 elemPtr = NextTaggedElement(&iter)) {
		Blt_Tags_AddItemToTag(&graphPtr->elements.tags, elemPtr, tag);
	    }
	}
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * TagDeleteOp --
 *
 *	.g element tag delete tagName tab1 tab2 tab3
 *
 *---------------------------------------------------------------------------
 */
static int
TagDeleteOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    const char *tag;
    int i;

    tag = Tcl_GetString(objv[4]);
    if (strcmp(tag, "all") == 0) {
	Tcl_AppendResult(interp, "can't delete reserved tag \"", tag, "\"", 
			 (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        Element *elemPtr;
        ElementIterator iter;
        
        if (GetElementIterator(interp, graphPtr, objv[i], &iter) != TCL_OK){
            return TCL_ERROR;
        }
        for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
             elemPtr = NextTaggedElement(&iter)) {
            Blt_Tags_RemoveItemFromTag(&graphPtr->elements.tags, elemPtr, tag);
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * TagExistsOp --
 *
 *	Returns the existence of the one or more tags in the given node.  If
 *	the node has any the tags, true is return in the interpreter.
 *
 *	.g element tag exists elem tag1 tag2 tag3...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagExistsOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int i;
    ElementIterator iter;

    if (GetElementIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 5; i < objc; i++) {
	const char *tag;
	Element *elemPtr;

	tag = Tcl_GetString(objv[i]);
	for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	     elemPtr = NextTaggedElement(&iter)) {
	    if (Blt_Tags_ItemHasTag(&graphPtr->elements.tags, elemPtr, tag)) {
		Tcl_SetBooleanObj(Tcl_GetObjResult(interp), TRUE);
		return TCL_OK;
	    }
	}
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), FALSE);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagForgetOp --
 *
 *	Removes the given tags from all tabs.
 *
 *	.g element tag forget tag1 tag2 tag3...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagForgetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int i;

    for (i = 4; i < objc; i++) {
	const char *tag;

	tag = Tcl_GetString(objv[i]);
	Blt_Tags_ForgetTag(&graphPtr->elements.tags, tag);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagGetOp --
 *
 *	Returns tag names for a given node.  If one of more pattern arguments
 *	are provided, then only those matching tags are returned.
 *
 *	.t element tag get elem pat1 pat2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagGetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Element *elemPtr; 
    ElementIterator iter;
    Tcl_Obj *listObjPtr;

    if (GetElementIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	 elemPtr = NextTaggedElement(&iter)) {
	if (objc == 5) {
            Blt_Tags_AppendTagsToObj(&graphPtr->elements.tags, elemPtr, 
                listObjPtr);
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj("all", 3));
	} else {
	    int i;
	    
	    /* Check if we need to add the special tags "all" */
	    for (i = 5; i < objc; i++) {
		const char *pattern;

		pattern = Tcl_GetString(objv[i]);
		if (Tcl_StringMatch("all", pattern)) {
		    Tcl_Obj *objPtr;

		    objPtr = Tcl_NewStringObj("all", 3);
		    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
		    break;
		}
	    }
	    /* Now process any standard tags. */
	    for (i = 5; i < objc; i++) {
		Blt_ChainLink link;
		const char *pattern;
                Blt_Chain chain;

                chain = Blt_Chain_Create();
                Blt_Tags_AppendTagsToChain(&graphPtr->elements.tags, elemPtr, 
                        chain);
		pattern = Tcl_GetString(objv[i]);
		for (link = Blt_Chain_FirstLink(chain); link != NULL; 
                     link = Blt_Chain_NextLink(link)) {
		    const char *tag;
                    Tcl_Obj *objPtr;

		    tag = (const char *)Blt_Chain_GetValue(link);
		    if (!Tcl_StringMatch(tag, pattern)) {
			continue;
		    }
                    objPtr = Tcl_NewStringObj(tag, -1);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                }
                Blt_Chain_Destroy(chain);
	    }
	}    
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagNamesOp --
 *
 *	Returns the names of all the tags in the graph.  If one of more
 *	element arguments are provided, then only the tags found in those
 *	elements are returned.
 *
 *	.g element tag names elem elem elem...
 *
 *---------------------------------------------------------------------------
 */
static int
TagNamesOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewStringObj("all", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    if (objc == 4) {
        Blt_Tags_AppendAllTagsToObj(&graphPtr->elements.tags, listObjPtr);
    } else {
	Blt_HashTable selected;
	int i;

	Blt_InitHashTable(&selected, BLT_STRING_KEYS);
	for (i = 4; i < objc; i++) {
	    ElementIterator iter;
	    Element *elemPtr;

	    if (GetElementIterator(interp, graphPtr, objv[i], &iter) != TCL_OK){
		goto error;
	    }
	    for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
		 elemPtr = NextTaggedElement(&iter)) {
		Blt_ChainLink link;
                Blt_Chain chain;

                chain = Blt_Chain_Create();
                Blt_Tags_AppendTagsToChain(&graphPtr->elements.tags, elemPtr, 
                        chain);
		for (link = Blt_Chain_FirstLink(chain); link != NULL; 
                     link = Blt_Chain_NextLink(link)) {
		    const char *tag;
                    int isNew;

		    tag = Blt_Chain_GetValue(link);
                    Blt_CreateHashEntry(&selected, tag, &isNew);
		}
                Blt_Chain_Destroy(chain);
	    }
	}
	{
	    Blt_HashEntry *hPtr;
	    Blt_HashSearch hiter;

	    for (hPtr = Blt_FirstHashEntry(&selected, &hiter); hPtr != NULL;
		 hPtr = Blt_NextHashEntry(&hiter)) {
		objPtr = Tcl_NewStringObj(Blt_GetHashKey(&selected, hPtr), -1);
		Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    }
	}
	Blt_DeleteHashTable(&selected);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
 error:
    Tcl_DecrRefCount(listObjPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagIndicesOp --
 *
 *	Returns the indices associated with the given tags.  The indices
 *	returned will represent the union of tabs for all the given tags.
 *
 *	.g element tag elements tag1 tag2 tag3...
 *
 *---------------------------------------------------------------------------
 */
static int
TagIndicesOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Blt_HashTable selected;
    int i;
	
    Blt_InitHashTable(&selected, BLT_ONE_WORD_KEYS);
    for (i = 4; i < objc; i++) {
	const char *tag;

	tag = Tcl_GetString(objv[i]);
	if (strcmp(tag, "all") == 0) {
	    break;
	} else {
            Blt_Chain chain;

            chain = Blt_Tags_GetItemList(&graphPtr->elements.tags, tag);
            if (chain != NULL) {
                Blt_ChainLink link;

                for (link = Blt_Chain_FirstLink(chain); link != NULL; 
                     link = Blt_Chain_NextLink(link)) {
                    Element *elemPtr;
                    int isNew;
                    
                    elemPtr = Blt_Chain_GetValue(link);
                    Blt_CreateHashEntry(&selected, (char *)elemPtr, &isNew);
                }
            }
            continue;
	}
	Tcl_AppendResult(interp, "can't find a tag \"", tag, "\"",
			 (char *)NULL);
	goto error;
    }
    {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
	for (hPtr = Blt_FirstHashEntry(&selected, &iter); hPtr != NULL; 
	     hPtr = Blt_NextHashEntry(&iter)) {
	    Element *elemPtr;
	    Tcl_Obj *objPtr;

	    elemPtr = (Element *)Blt_GetHashKey(&selected, hPtr);
	    objPtr = Tcl_NewStringObj(elemPtr->obj.name, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
    }
    Blt_DeleteHashTable(&selected);
    return TCL_OK;

 error:
    Blt_DeleteHashTable(&selected);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagSetOp --
 *
 *	Sets one or more tags for a given tab.  Tag names can't start with a
 *	digit (to distinquish them from node ids) and can't be a reserved tag
 *	("all").
 *
 *	.t element tag set elem tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagSetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int i;
    ElementIterator iter;

    if (GetElementIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 5; i < objc; i++) {
	const char *tag;
	Element *elemPtr;

	tag = Tcl_GetString(objv[i]);
	if (strcmp(tag, "all") == 0) {
	    Tcl_AppendResult(interp, "can't add reserved tag \"", tag, "\"",
			     (char *)NULL);	
	    return TCL_ERROR;
	}
	for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	     elemPtr = NextTaggedElement(&iter)) {
	    Blt_Tags_AddItemToTag(&graphPtr->elements.tags, elemPtr, tag);
	}    
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagUnsetOp --
 *
 *	Removes one or more tags from a given element. If a tag doesn't exist 
 *	or is a reserved tag ("all"), nothing will be done and no error
 *	message will be returned.
 *
 *	.g element tag unset elem tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagUnsetOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Element *elemPtr;
    ElementIterator iter;

    if (GetElementIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (elemPtr = FirstTaggedElement(&iter); elemPtr != NULL; 
	 elemPtr = NextTaggedElement(&iter)) {
	int i;

	for (i = 5; i < objc; i++) {
            const char *tag;

            tag = Tcl_GetString(objv[i]);
	    Blt_Tags_RemoveItemFromTag(&graphPtr->elements.tags, elemPtr, tag);
	}    
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagOp --
 *
 * 	This procedure is invoked to process tag operations.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec tagOps[] =
{
    {"add",      1, TagAddOp,      5, 0, "elem ?tag...?",},
    {"delete",   1, TagDeleteOp,   5, 0, "elem ?tag...?",},
    {"exists",   2, TagExistsOp,   5, 0, "elem ?tag...?",},
    {"forget",   1, TagForgetOp,   4, 0, "?tag...?",},
    {"get",      1, TagGetOp,      5, 0, "elem ?pattern...?",},
    {"indices",  1, TagIndicesOp,  4, 0, "?tag...?",},
    {"names",    1, TagNamesOp,    4, 0, "?elem...?",},
    {"set",      1, TagSetOp,      5, 0, "elem ?tag...",},
    {"unset",    1, TagUnsetOp,    5, 0, "elem ?tag...",},
};

static int numTagOps = sizeof(tagOps) / sizeof(Blt_OpSpec);

static int
TagOp(Graph *graphPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    GraphElementProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numTagOps, tagOps, BLT_OP_ARG2,
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc)(graphPtr, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 *	Returns the type of the element of an element.
 *
 * Results:
 *	A standard TCL result. Returns the type of the element in
 *	interp->result. If the identifier given doesn't represent an
 *	element, then an error message is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeOp(
    Graph *graphPtr,			/* Graph widget */
    Tcl_Interp *interp,
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)		/* Element name */
{
    Element *elemPtr;
    const char *string;

    if (GetElementFromObj(interp, graphPtr, objv[3], &elemPtr) != TCL_OK) {
	return TCL_ERROR;	/* Can't find named element */
    }
    switch (elemPtr->obj.classId) {
    case CID_ELEM_BAR:		
    	string = "bar";		break;
    case CID_ELEM_CONTOUR:	
    	string = "contour";	break;
    case CID_ELEM_LINE:		
    	string = "line";	break;
    case CID_ELEM_STRIP:	
    	string = "strip";	break;
    default:			
    	string = "???";		break;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), string, -1);
    return TCL_OK;
}

/*
 * Global routines:
 */
static Blt_OpSpec elemOps[] = {
    {"activate",   6, ActivateOp,    3, 0, "?elemName? ?index...?",},
    {"active",     6, ActiveOp,      2, 0, "args",},
    {"bind",       1, BindOp,        3, 6, "elemName sequence command",},
    {"cget",       2, CgetOp,        5, 5, "elemName option",},
    {"closest",    2, ClosestOp,     6, 0,
	"x y varName ?option value?... ?elemName?...",},
    {"configure",  2, ConfigureOp,   4, 0,
	"elemName ?elemName?... ?option value?...",},
    {"create",     2, CreateOp,      4, 0, "elemName ?option value?...",},
    {"deactivate", 3, DeactivateOp,  3, 0, "?elemName?...",},
    {"delete",     3, DeleteOp,      3, 0, "?elemName?...",},
    {"exists",     1, ExistsOp,      4, 4, "elemName",},
    {"find",       1, FindOp,        7, 8, "elemName x1 y1 x2 y2",},
    {"get",        1, GetOp,         4, 4, "elemName",},
    {"isoline",    4, Blt_IsolineOp, 2, 0, "args...",},
    {"isotag",     4, Blt_IsoTagOp,  2, 0, "args...",},
    {"lower",      1, LowerOp,       3, 0, "?elemName?...",},
    {"names",      2, NamesOp,       3, 0, "?pattern?...",},
    {"nearest",    2, NearestOp,     5, 0,
	"x y ?option value?... ?elemName?...",},
    {"raise",      1, RaiseOp,       3, 0, "?elemName?...",},
    {"show",       1, ShowOp,        3, 4, "?elemList?",},
    {"tag",        2, TagOp,         2, 0, "args",},
    {"type",       2, TypeOp,        4, 4, "elemName",},
};
static int numElemOps = sizeof(elemOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ElementOp --
 *
 *	This procedure is invoked to process the TCL command that corresponds
 *	to a widget managed by this module.  See the user documentation for
 *	details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_ElementOp(
    Graph *graphPtr,			/* Graph widget record */
    Tcl_Interp *interp,
    int objc,				/* # arguments */
    Tcl_Obj *const *objv,		/* Argument list */
    ClassId classId)
{
    void *ptr;
    int result;

    ptr = Blt_GetOpFromObj(interp, numElemOps, elemOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (ptr == NULL) {
	return TCL_ERROR;
    }
    if (ptr == CreateOp) {
	result = CreateOp(graphPtr, interp, objc, objv, classId);
    } else {
	GraphElementProc *proc;
	
	proc = ptr;
	result = (*proc) (graphPtr, interp, objc, objv);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GraphExtents --
 *
 *	Generates a bounding box representing the plotting area of the
 *	graph. This data structure is used to clip the points and line
 *	segments of the line element.
 *
 *	The clip region is the plotting area plus such arbitrary extra space.
 *	The reason we clip with a bounding box larger than the plot area is so
 *	that symbols will be drawn even if their center point isn't in the
 *	plotting area.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The bounding box is filled with the dimensions of the plotting area.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_GraphExtents(void *ptr, Region2d *r)
{
    Element *elemPtr = (Element *)ptr;
    Graph *graphPtr;
    Axis *x, *y;

    graphPtr = elemPtr->obj.graphPtr;
    if (graphPtr->inverted) {
	y = elemPtr->axes.x;
	x = elemPtr->axes.y;
    } else {
	x = elemPtr->axes.x;

	y = elemPtr->axes.y;
    }
    r->left   = (double)x->screenMin;
    r->top    = (double)y->screenMin;
    r->right  = (double)(x->screenMin + x->screenRange);
    r->bottom = (double)(y->screenMin + y->screenRange);
}

Element *
Blt_NearestElement(Graph *graphPtr, int x, int y)
{
    NearestElement nearest;
    Blt_ChainLink link;

    memset(&nearest, 0, sizeof(NearestElement));
    nearest.along = NEAREST_SEARCH_XY;
    nearest.maxDistance = graphPtr->halo;
    nearest.distance = nearest.maxDistance + 1;
    nearest.x = x;
    nearest.y = y;
    nearest.mode = NEAREST_SEARCH_AUTO;
    
    for (link = Blt_Chain_LastLink(graphPtr->elements.displayList);
	 link != NULL; link = Blt_Chain_PrevLink(link)) {
	Element *elemPtr;

	elemPtr = Blt_Chain_GetValue(link);
	if (elemPtr->flags & (HIDE|MAP_ITEM)) {
	    continue;
	}
	(*elemPtr->procsPtr->nearestProc) (graphPtr, elemPtr, &nearest);
    }
    if (nearest.distance <= nearest.maxDistance) {
	return nearest.item;	/* Found an element within the minimum
				 * halo distance. */
    }
    return NULL;
}

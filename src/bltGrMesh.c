
/*
 * bltGrMesh.c --
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

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltVector.h"
#include "bltNsUtil.h"
#include "bltDataTable.h"
#include "bltGrMesh.h"
#include "bltSwitch.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define MESH_THREAD_KEY "BLT Mesh Command Interface"

#define DELETE_PENDING		(1<<1) /* 0x0002 */
#define CONFIG_PENDING		(1<<2) /* 0x0002 */

/*
 * MeshCmdInterpData --
 *
 *	Structure containing global data, used on a interpreter by interpreter
 *	basis.
 *
 *	This structure holds the hash table of instances of datatable commands
 *	associated with a particular interpreter.
 */
struct _MeshCmdInterpData {
    Blt_HashTable meshTable;		/* Tracks tables in use. */
    Tcl_Interp *interp;
    int nextMeshId;
};

typedef struct {
    double x, y;
    int index;
} HullVertex;

typedef struct _DataSourceResult DataSourceResult;

typedef int (DataSourceGetProc)(Tcl_Interp *interp, DataSource *srcPtr, 
	DataSourceResult *resultPtr);
typedef void (DataSourceDestroyProc)(DataSource *srcPtr);
typedef Tcl_Obj * (DataSourcePrintProc)(DataSource *srcPtr);

typedef int (DataSourceChangedProc)(DataSource *srcPtr);

typedef enum SourceTypes {
    SOURCE_LIST, SOURCE_VECTOR, SOURCE_TABLE, SOURCE_NONE
} SourceType;

typedef struct {
    enum SourceTypes type;		/* Selects the type of data populating
					 * this data source: SOURCE_VECTOR,
					 * SOURCE_TABLE, or SOURCE_LIST */
    const char *name;
    DataSourceGetProc *getProc;
    DataSourceDestroyProc *destroyProc;
    DataSourcePrintProc *printProc;
} DataSourceClass;

struct _DataSourceResult {
    double min, max;
    double *values;
    double numValues;
};

struct _DataSource {
    ClientData clientData;
    DataSourceClass *classPtr;
};

typedef struct {
    ClientData clientData;
    DataSourceClass *classPtr;

    /* Vector-specific fields. */
    Blt_VectorId vector;
} VectorDataSource;

typedef struct {
    ClientData clientData;
    DataSourceClass *classPtr;

    /* List-specific fields. */
    double *values;
    int numValues;
} ListDataSource;

typedef struct {
    BLT_TABLE table;
    int refCount;
} TableClient;

typedef struct {
    ClientData clientData;
    DataSourceClass *classPtr;

    /* Table-specific fields. */
    BLT_TABLE table;			/* Data table. */ 
    BLT_TABLE_COLUMN column;		/* Column of data used. */
    BLT_TABLE_NOTIFIER notifier;	/* Notifier used for column
					 * (destroy). */
    BLT_TABLE_TRACE trace;		/* Trace used for column
					 * (set/get/unset). */
    Blt_HashEntry *hashPtr;		/* Pointer to entry of source in
					 * meshes' hash table of meshes. One
					 * mesh may use multiple columns from
					 * the same data table. */
} TableDataSource;

typedef int (MeshConfigureProc)(Tcl_Interp *interp, Mesh *meshPtr);

typedef enum MeshTypes {
    MESH_CLOUD, MESH_REGULAR, MESH_IRREGULAR, MESH_TRIANGLE
} MeshType;

typedef struct {
    double x, y;
} MeshKey;

struct _MeshClass {
    enum MeshTypes type;		/* Type of mesh. */
    const char *name;			/* Name of mesh class. */
    Blt_SwitchSpec *specs;		/* Mesh configuration
					 * specifications. */
    MeshConfigureProc *configProc;	/* Configure procedure. */
};

static MeshConfigureProc CloudMeshConfigureProc;
static MeshConfigureProc IrregularMeshConfigureProc;
static MeshConfigureProc RegularMeshConfigureProc;
static MeshConfigureProc TriangleMeshConfigureProc;

static DataSourceGetProc     ListDataSourceGetProc;
static DataSourcePrintProc   ListDataSourcePrintProc;
static DataSourceDestroyProc ListDataSourceDestroyProc;

static DataSourceGetProc     TableDataSourceGetProc;
static DataSourcePrintProc   TableDataSourcePrintProc;
static DataSourceDestroyProc TableDataSourceDestroyProc;

static DataSourceGetProc     VectorDataSourceGetProc;
static DataSourcePrintProc   VectorDataSourcePrintProc;
static DataSourceDestroyProc VectorDataSourceDestroyProc;

static DataSourceClass listDataSourceClass = {
    SOURCE_LIST, 
    "List",
    ListDataSourceGetProc,
    ListDataSourceDestroyProc,
    ListDataSourcePrintProc
};

static DataSourceClass vectorDataSourceClass = {
    SOURCE_VECTOR, "Vector",
    VectorDataSourceGetProc,
    VectorDataSourceDestroyProc,
    VectorDataSourcePrintProc
};

static DataSourceClass tableDataSourceClass = {
    SOURCE_TABLE, "Table",
    TableDataSourceGetProc,
    TableDataSourceDestroyProc,
    TableDataSourcePrintProc
};

static Blt_SwitchFreeProc FreeTrianglesProc;
static Blt_SwitchParseProc ObjToTrianglesProc;
static Blt_SwitchPrintProc TrianglesToObjProc;
static Blt_SwitchCustom trianglesSwitch =
{
    ObjToTrianglesProc, TrianglesToObjProc, FreeTrianglesProc, (ClientData)0
};

static Blt_SwitchFreeProc FreeDataSourceProc;
static Blt_SwitchParseProc ObjToDataSourceProc;
static Blt_SwitchPrintProc DataSourceToObjProc;
Blt_SwitchCustom bltDataSourceSwitch =
{
    ObjToDataSourceProc, DataSourceToObjProc, FreeDataSourceProc, (ClientData)0
};

static Blt_SwitchSpec cloudMeshSpecs[] =
{
    {BLT_SWITCH_CUSTOM, "-x", (char *)NULL, (char *)NULL, 
	Blt_Offset(Mesh, x), 0, 0, &bltDataSourceSwitch},
    {BLT_SWITCH_CUSTOM, "-y", (char *)NULL, (char *)NULL, 
	Blt_Offset(Mesh, y), 0, 0, &bltDataSourceSwitch},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec regularMeshSpecs[] =
{
    {BLT_SWITCH_CUSTOM, "-x", (char *)NULL, (char *)NULL, 
	Blt_Offset(Mesh, x), 0, 0, &bltDataSourceSwitch},
    {BLT_SWITCH_CUSTOM, "-y", (char *)NULL, (char *)NULL, 
	Blt_Offset(Mesh, y), 0, 0, &bltDataSourceSwitch},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec irregularMeshSpecs[] =
{
    {BLT_SWITCH_CUSTOM, "-x", (char *)NULL, (char *)NULL, 
	Blt_Offset(Mesh, x), 0, 0, &bltDataSourceSwitch},
    {BLT_SWITCH_CUSTOM, "-y", (char *)NULL, (char *)NULL, 
	Blt_Offset(Mesh, y), 0, 0, &bltDataSourceSwitch},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec triangleMeshSpecs[] =
{
    {BLT_SWITCH_CUSTOM, "-x",  (char *)NULL, (char *)NULL, 
	Blt_Offset(Mesh, x), 0, 0, &bltDataSourceSwitch},
    {BLT_SWITCH_CUSTOM, "-y", (char *)NULL, (char *)NULL, 
	Blt_Offset(Mesh, y), 0, 0, &bltDataSourceSwitch},
    {BLT_SWITCH_CUSTOM, "-triangles", (char *)NULL, (char *)NULL, 
	Blt_Offset(Mesh, triangles), 0, 0, &trianglesSwitch},
    {BLT_SWITCH_END}
};

static MeshClass cloudMeshClass = {
    MESH_CLOUD, 
    "cloud",
    cloudMeshSpecs,
    CloudMeshConfigureProc,

};

static MeshClass regularMeshClass = {
    MESH_REGULAR, 
    "regular", 
    regularMeshSpecs,
    RegularMeshConfigureProc,
};

static MeshClass triangleMeshClass = {
    MESH_TRIANGLE, 
    "triangle",
    triangleMeshSpecs,
    TriangleMeshConfigureProc,
};

static MeshClass irregularMeshClass = {
    MESH_IRREGULAR, 
    "irregular",
    irregularMeshSpecs,
    IrregularMeshConfigureProc,
};

static MeshCmdInterpData *GetMeshCmdInterpData(Tcl_Interp *interp);
static int ComputeMesh(Mesh *meshPtr);


/*
 *---------------------------------------------------------------------------
 *
 * ObjToTrianglesProc --
 *
 *	Given a string representation of a data source, converts it into its
 *	equivalent data source.  A data source may be a list of numbers, a
 *	vector name, or a two element list of table name and column.
 *
 * Results:
 *	The return value is a standard TCL result.  The data source is passed
 *	back via the srcPtr.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTrianglesProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* TCL list of indices */
    char *record,			/* Record */
    int offset,				/* Offset to field in record */
    int flags)				/* Not used. */
{
    Mesh *meshPtr = (Mesh *)record;
    Tcl_Obj **objv;
    int objc;
    MeshTriangle *t, *triples;
    int i, numTriples;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 0) {
	if (meshPtr->triples != NULL) {
	    Blt_Free(meshPtr->triples);
	}
	meshPtr->triples = NULL;
	meshPtr->numTriples = 0;
	return TCL_OK;
    }
    if ((objc & 3) != 0) {
	Tcl_AppendResult(interp, "wrong # of elements in triangle list: ",
			 "must be have 3 indices for each triangle",
			 (char *)NULL);
	return TCL_ERROR;
    }
    numTriples = objc / 3;
    triples = Blt_Malloc(sizeof(MeshTriangle) * numTriples);
    if (triples == NULL) {
	Tcl_AppendResult(interp, "can't allocate array of ",
			 Blt_Itoa(numTriples), " triangles.", (char *)NULL);
	return TCL_ERROR;
    }
    t = triples;
    for (i = 0; i < objc; i += 3) {
	int a, b, c;

	if ((Tcl_GetIntFromObj(interp, objv[i], &a) != TCL_OK) || (a <= 0)) {
	    Tcl_AppendResult(interp, "bad triangle index \"", 
			     Tcl_GetString(objv[i]), "\"", (char *)NULL);
	    goto error;
	}
	if ((Tcl_GetIntFromObj(interp, objv[i+1], &b) != TCL_OK) || (b <= 0)) {
	    Tcl_AppendResult(interp, "bad triangle index \"", 
			     Tcl_GetString(objv[i+1]), "\"", (char *)NULL);
	    goto error;
	}
	if ((Tcl_GetIntFromObj(interp, objv[i+2], &c) != TCL_OK) || (c <= 0)) {
	    Tcl_AppendResult(interp, "bad triangle index \"", 
			     Tcl_GetString(objv[i+2]), "\"", (char *)NULL);
	    goto error;
	}
	t->a = a - 1;
	t->b = b - 1;
	t->c = c - 1;
	t++;
    }
    if (meshPtr->triples != NULL) {
	Blt_Free(meshPtr->triples);
    }
    meshPtr->triples = triples;
    meshPtr->numTriples = numTriples;
    return TCL_OK;
 error:
    if (triples != NULL) {
	Blt_Free(triples);
    }
    meshPtr->numTriples = 0;
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TrianglesToObjProc --
 *
 *	Converts the data source to its equivalent string representation.
 *	The data source may be a table, vector, or list.
 *
 * Results:
 *	The string representation of the data source is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TrianglesToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Mesh *meshPtr = (Mesh *)record;
    MeshTriangle *t, *tend;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (t = meshPtr->triples, tend = t + meshPtr->numTriples; t < tend; t++) {
	Tcl_Obj *subListObjPtr;
	
	subListObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	Tcl_ListObjAppendElement(interp, subListObjPtr, Tcl_NewIntObj(t->a));
	Tcl_ListObjAppendElement(interp, subListObjPtr, Tcl_NewIntObj(t->b));
	Tcl_ListObjAppendElement(interp, subListObjPtr, Tcl_NewIntObj(t->c));
	Tcl_ListObjAppendElement(interp, listObjPtr, subListObjPtr);
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeTrianglesProc --
 *
 *	Free the table used.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FreeTrianglesProc(ClientData clientData, char *record, int offset, int flags)
{
    Mesh *meshPtr = (Mesh *)record;

    if (meshPtr->triples != NULL) {
	Blt_Free(meshPtr->triples);
	meshPtr->triples = NULL;
	meshPtr->numTriples = 0;
    }
}

static void
NotifyClients(Mesh *meshPtr, unsigned int flags)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&meshPtr->notifierTable, &iter); 
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	MeshNotifyProc *proc;
	ClientData clientData;

	proc = Blt_GetHashValue(hPtr);
	clientData = Blt_GetHashKey(&meshPtr->notifierTable, hPtr);
	(*proc)(meshPtr, clientData, flags);
    }
}

static void
ConfigureMesh(ClientData clientData)
{
    Mesh *meshPtr = clientData;

    if ((*meshPtr->classPtr->configProc)(meshPtr->interp, meshPtr) != TCL_OK) {
	Tcl_BackgroundError(meshPtr->interp);
	return;				/* Failed to configure element */
    }
    if ((meshPtr->numVertices == 0) || (meshPtr->vertices == NULL)) {
	return;
    }
    NotifyClients(meshPtr, MESH_CHANGE_NOTIFY);
}

static void
EventuallyConfigureMesh(ClientData clientData)
{
    Mesh *meshPtr = clientData;

    if ((meshPtr->flags & CONFIG_PENDING) == 0) {
	meshPtr->flags |= CONFIG_PENDING;
	Tcl_DoWhenIdle(ConfigureMesh, meshPtr);
    }
}

static void
DestroyDataSource(DataSource *srcPtr)
{
    Mesh *meshPtr = srcPtr->clientData;

    if ((srcPtr->classPtr != NULL) && (srcPtr->classPtr->destroyProc != NULL)) {
	(*srcPtr->classPtr->destroyProc)(srcPtr);
	NotifyClients(meshPtr, MESH_DELETE_NOTIFY);
    }
    if (meshPtr->x == srcPtr) {
	meshPtr->x = NULL;
    } else if (meshPtr->y == srcPtr) {
	meshPtr->y = NULL;
    }
    memset(srcPtr, 0, sizeof(*srcPtr));
    Blt_Free(srcPtr);
}

static Tcl_Obj *
VectorDataSourcePrintProc(DataSource *basePtr)
{
    VectorDataSource *srcPtr = (VectorDataSource *)basePtr;
	    
    return Tcl_NewStringObj(Blt_NameOfVectorId(srcPtr->vector), -1);
}

static void
VectorDataSourceDestroyProc(DataSource *basePtr)
{
    VectorDataSource *srcPtr = (VectorDataSource *)basePtr;

    Blt_SetVectorChangedProc(srcPtr->vector, NULL, NULL);
    if (srcPtr->vector != NULL) { 
	Blt_FreeVectorId(srcPtr->vector); 
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * VectorChangedProc --
 *
 * Results:
 *     	None.
 *
 *---------------------------------------------------------------------------
 */
static void
VectorChangedProc(Tcl_Interp *interp, ClientData clientData, 
		  Blt_VectorNotify notify)
{
    VectorDataSource *srcPtr = clientData;
    Mesh *meshPtr;

    meshPtr = srcPtr->clientData;
    if (notify == BLT_VECTOR_NOTIFY_DESTROY) {
	VectorDataSourceDestroyProc((DataSource *)srcPtr);
	NotifyClients(meshPtr, MESH_DELETE_NOTIFY);
	return;
    } 
    /* Reconfigure the mesh now that one the vector has changed. */
    EventuallyConfigureMesh(meshPtr);
}

static DataSource *
NewVectorDataSource(Tcl_Interp *interp, Mesh *meshPtr, const char *name)
{
    Blt_Vector *vecPtr;
    VectorDataSource *srcPtr;
    
    srcPtr = Blt_AssertCalloc(1, sizeof(VectorDataSource));
    srcPtr->classPtr = &vectorDataSourceClass;
    srcPtr->vector = Blt_AllocVectorId(interp, name);
    if (Blt_GetVectorById(interp, srcPtr->vector, &vecPtr) != TCL_OK) {
	Blt_Free(srcPtr);
	return NULL;
    }
    Blt_SetVectorChangedProc(srcPtr->vector, VectorChangedProc, srcPtr);
    return (DataSource *)srcPtr;
}

static int
VectorDataSourceGetProc(Tcl_Interp *interp, DataSource *basePtr, 
			DataSourceResult *resultPtr)
{
    VectorDataSource *srcPtr = (VectorDataSource *)basePtr;
    size_t numBytes;
    Blt_Vector *vector;
    double *values;

    if (Blt_GetVectorById(interp, srcPtr->vector, &vector) != TCL_OK) {
	return TCL_ERROR;
    }
    numBytes = Blt_VecLength(vector) * sizeof(double);
    values = Blt_Malloc(numBytes);
    if (values == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't allocate new vector", (char *)NULL);
	}
	return TCL_ERROR;
    }
    {
	double min, max;
	double *p; 
	int i;

	p = Blt_VecData(vector);
	min = max = *p;
	for (i = 0; i < Blt_VecLength(vector); i++, p++) {
	    values[i] = *p;
	    if (*p > max) {
		max = (double)*p;
	    } else if (*p < min) {
		min = (double)*p;
	    }
	}
	resultPtr->min = min;
	resultPtr->max = max;
    }
    resultPtr->values = values;
    resultPtr->numValues = Blt_VecLength(vector);
    return TCL_OK;
}

static Tcl_Obj *
ListDataSourcePrintProc(DataSource *basePtr)
{
    ListDataSource *srcPtr = (ListDataSource *)basePtr;
    Tcl_Interp *interp;
    Tcl_Obj *listObjPtr;
    double *p, *pend; 
    Mesh *meshPtr;

    meshPtr = srcPtr->clientData;
    interp = meshPtr->interp;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (p = srcPtr->values, pend = p + srcPtr->numValues; p < pend; p++) {
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(*p));
    }
    return listObjPtr;
}

static void
ListDataSourceDestroyProc(DataSource *basePtr)
{
    ListDataSource *srcPtr = (ListDataSource *)basePtr;

    if (srcPtr->values != NULL) {
	Blt_Free(srcPtr->values);
	srcPtr->values = NULL;
    }
}

static DataSource *
NewListDataSource(Tcl_Interp *interp, Mesh *meshPtr, int objc, Tcl_Obj **objv)
{
    double *values;
    ListDataSource *srcPtr;

    srcPtr = Blt_AssertMalloc(sizeof(ListDataSource));
    srcPtr->classPtr = &listDataSourceClass;
    values = NULL;
    if (objc > 0) {
	int i;

	values = Blt_Malloc(sizeof(double) * objc);
	if (values == NULL) {
	    Tcl_AppendResult(interp, "can't allocate new vector", (char *)NULL);
	    Blt_Free(srcPtr);
	    return NULL;
	}
	for (i = 0; i < objc; i++) {
	    if (Blt_ExprDoubleFromObj(interp, objv[i], values + i) != TCL_OK) {
		Blt_Free(values);
		Blt_Free(srcPtr);
		return NULL;
	    }
	}
	srcPtr->values = values;
	srcPtr->numValues = objc;
    }
    return (DataSource *)srcPtr;
}

static int
ListDataSourceGetProc(Tcl_Interp *interp, DataSource *basePtr, 
		      DataSourceResult *resultPtr)
{
    size_t i;
    double *values;
    double min, max;
    ListDataSource *srcPtr = (ListDataSource *)basePtr;

    values = Blt_Malloc(sizeof(double) * srcPtr->numValues);
    if (values == NULL) {
	return TCL_ERROR;
    }
    min = max = srcPtr->values[0];
    for (i = 0; i < srcPtr->numValues; i++) {
	values[i] = srcPtr->values[i];
	if (values[i] > max) {
	    max = values[i];
	} else if (values[i] < min) {
	    min = values[i];
	}
    }
    resultPtr->min = min;
    resultPtr->max = max;
    resultPtr->values = values;
    resultPtr->numValues = srcPtr->numValues;
    return TCL_OK;
}


static void
TableDataSourceDestroyProc(DataSource *basePtr)
{
    TableDataSource *srcPtr = (TableDataSource *)basePtr;

    /* Shouldn't be here if column was destroyed. Need to mark source
     * as deleted. */
    if (srcPtr->trace != NULL) {
	blt_table_delete_trace(srcPtr->trace);
    }
    if (srcPtr->notifier != NULL) {
	blt_table_delete_notifier(srcPtr->notifier);
    }
    if (srcPtr->hashPtr != NULL) {
	TableClient *clientPtr;

	clientPtr = Blt_GetHashValue(srcPtr->hashPtr);
	clientPtr->refCount--;
	if (clientPtr->refCount == 0) {
	    Mesh *meshPtr;

	    meshPtr = srcPtr->clientData;
	    if (srcPtr->table != NULL) {
		blt_table_close(srcPtr->table);
	    }
	    Blt_Free(clientPtr);
	    Blt_DeleteHashEntry(&meshPtr->tableTable, srcPtr->hashPtr);
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
 *---------------------------------------------------------------------------
 */
static int
TableNotifyProc(ClientData clientData, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    DataSource *srcPtr = clientData;
    Mesh *meshPtr;

    meshPtr = srcPtr->clientData;
    if (eventPtr->type == TABLE_NOTIFY_COLUMNS_DELETED) {
	DestroyDataSource(srcPtr);
	if (meshPtr->x == srcPtr) {
	    meshPtr->x = NULL;
	} else if (meshPtr->y == srcPtr) {
	    meshPtr->y = NULL;
	}
    } 
    EventuallyConfigureMesh(meshPtr);
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
 *---------------------------------------------------------------------------
 */
static int
TableTraceProc(ClientData clientData, BLT_TABLE_TRACE_EVENT *eventPtr)
{
    TableDataSource *srcPtr = clientData;
    Mesh *meshPtr;

    meshPtr = srcPtr->clientData;
    assert(eventPtr->column == srcPtr->column);
    EventuallyConfigureMesh(meshPtr);
    return TCL_OK;
}


static DataSource *
NewTableDataSource(Tcl_Interp *interp, Mesh *meshPtr, const char *name, 
		   Tcl_Obj *colObjPtr)
{
    TableDataSource *srcPtr;
    TableClient *clientPtr;
    int isNew;

    srcPtr = Blt_AssertMalloc(sizeof(TableDataSource));
    srcPtr->classPtr = &tableDataSourceClass;

    /* See if the mesh is already using this table. */
    srcPtr->hashPtr = Blt_CreateHashEntry(&meshPtr->tableTable, name, &isNew);
    if (isNew) {
	if (blt_table_open(interp, name, &srcPtr->table) != TCL_OK) {
	    return NULL;
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
    srcPtr->column = blt_table_get_column(interp, srcPtr->table, colObjPtr);
    if (srcPtr->column == NULL) {
	goto error;
    }
    srcPtr->notifier = blt_table_create_column_notifier(interp, srcPtr->table,
	srcPtr->column, TABLE_NOTIFY_COLUMN_CHANGED, TableNotifyProc, 
	(BLT_TABLE_NOTIFIER_DELETE_PROC *)NULL, srcPtr);
    srcPtr->trace = blt_table_set_column_trace(srcPtr->table, srcPtr->column,
	TABLE_TRACE_WCU, TableTraceProc, (BLT_TABLE_TRACE_DELETE_PROC *)NULL, 
	srcPtr);
    return (DataSource *)srcPtr;
 error:
    DestroyDataSource((DataSource *)srcPtr);
    return NULL;
}

static Tcl_Obj *
TableDataSourcePrintProc(DataSource *basePtr)
{
    TableDataSource *srcPtr = (TableDataSource *)basePtr;
    Tcl_Obj *listObjPtr;
    const char *name;
    long index;
    Tcl_Interp *interp;
    Mesh *meshPtr;

    meshPtr = srcPtr->clientData;
    interp = meshPtr->interp;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

    name = blt_table_name(srcPtr->table);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewStringObj(name, -1));
    
    index = blt_table_column_index(srcPtr->column);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewLongObj(index));
    return listObjPtr;
}

static int
TableDataSourceGetProc(Tcl_Interp *interp, DataSource *basePtr, 
		       DataSourceResult *resultPtr)
{
    TableDataSource *srcPtr = (TableDataSource *)basePtr;
    BLT_TABLE table;
    double *values;
    double minValue, maxValue;
    long i;

    table = srcPtr->table;
    values = Blt_Malloc(sizeof(double) * blt_table_num_rows(table));
    if (values == NULL) {
	return TCL_ERROR;
    }
    minValue = FLT_MAX, maxValue = -FLT_MAX;
    for (i = 0; i < blt_table_num_rows(table); i++) {
	BLT_TABLE_ROW row;

	row = blt_table_row(table, i);
	values[i] = blt_table_get_double(table, row, srcPtr->column);
	if (values[i] < minValue) {
	    minValue = values[i];
	}
	if (values[i] > maxValue) {
	    maxValue = values[i];
	}
    }
    resultPtr->min = minValue;
    resultPtr->max = maxValue;
    resultPtr->values = values;
    resultPtr->numValues = i;
    return TCL_OK;
}

/*ARGSUSED*/
static void
FreeDataSourceProc(
    ClientData clientData,		/* Not used. */
    char *record,
    int offset, 
    int flags)
{
    DataSource **srcPtrPtr = (DataSource **)(record + offset);

    if (*srcPtrPtr != NULL) {
	DestroyDataSource(*srcPtrPtr);
    }
    *srcPtrPtr = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToDataSourceProc --
 *
 *	Given a string representation of a data source, converts it into its
 *	equivalent data source.  A data source may be a list of numbers, a
 *	vector name, or a two element list of table name and column.
 *
 * Results:
 *	The return value is a standard TCL result.  The data source is passed
 *	back via the srcPtr.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToDataSourceProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* TCL list of expressions */
    char *record,			/* Record */
    int offset,				/* Offset to field in record */
    int flags)				/* Not used. */
{
    Mesh *meshPtr = (Mesh *)record;
    DataSource *srcPtr;
    DataSource **srcPtrPtr = (DataSource **)(record + offset);
    Tcl_Obj **objv;
    int objc;
    const char *string;
    
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 0) {
	if (*srcPtrPtr != NULL) {
	    DestroyDataSource(*srcPtrPtr);
	}
	*srcPtrPtr = NULL;
	return TCL_OK;
    }
    string = Tcl_GetString(objv[0]);
    if ((objc == 1) && (Blt_VectorExists2(interp, string))) {
	srcPtr = NewVectorDataSource(interp, meshPtr, string);
    } else if ((objc == 2) && (blt_table_exists(interp, string))) {
 	srcPtr = NewTableDataSource(interp, meshPtr, string, objv[1]);
    } else {
	srcPtr = NewListDataSource(interp, meshPtr, objc, objv);
    }
    srcPtr->clientData = meshPtr;
    *srcPtrPtr = srcPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DataSourceToObjProc --
 *
 *	Converts the data source to its equivalent string representation.
 *	The data source may be a table, vector, or list.
 *
 * Results:
 *	The string representation of the data source is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
DataSourceToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    DataSource *srcPtr = *(DataSource **)(record + offset);

    if ((srcPtr != NULL) && (srcPtr->classPtr != NULL)) {
	return (srcPtr->classPtr->printProc)(srcPtr);
    }
    return Tcl_NewStringObj("", -1);
}

static void
DestroyMesh(Mesh *meshPtr)
{
    MeshCmdInterpData *dataPtr;

    dataPtr = meshPtr->dataPtr;
    if (meshPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&dataPtr->meshTable, meshPtr->hashPtr);
    }
    Blt_FreeSwitches(meshPtr->classPtr->specs, (char *)meshPtr, 0);
    if (meshPtr->triangles != NULL) {
	Blt_Free(meshPtr->triangles);
    }
    if (meshPtr->vertices != NULL) {
	Blt_Free(meshPtr->vertices);
    }
    if (meshPtr->hull != NULL) {
	Blt_Free(meshPtr->hull);
    }
    Blt_DeleteHashTable(&meshPtr->notifierTable);
    Blt_DeleteHashTable(&meshPtr->hideTable);
    Blt_Free(meshPtr);
}

static int
GetMesh(Tcl_Interp *interp, MeshCmdInterpData *dataPtr, const char *string,
	Mesh **meshPtrPtr)
{
    Blt_HashEntry *hPtr;
    Blt_ObjectName objName;
    Tcl_DString ds;
    const char *name;

    /* 
     * Parse the command and put back so that it's in a consistent
     * format.
     *
     *	t1         <current namespace>::t1
     *	n1::t1     <current namespace>::n1::t1
     *	::t1	   ::t1
     *  ::n1::t1   ::n1::t1
     */
    if (!Blt_ParseObjectName(interp, string, &objName, 0)) {
	return TCL_ERROR;
    }
    name = Blt_MakeQualifiedName(&objName, &ds);
    hPtr = Blt_FindHashEntry(&dataPtr->meshTable, name);
    Tcl_DStringFree(&ds);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find a mesh \"", string, "\"",
			 (char *)NULL);
	}
	return TCL_ERROR;
    }
    *meshPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

static int
GetMeshFromObj(Tcl_Interp *interp, MeshCmdInterpData *dataPtr, Tcl_Obj *objPtr, 
	       Mesh **meshPtrPtr)
{
    return GetMesh(interp, dataPtr, Tcl_GetString(objPtr),  meshPtrPtr);
}

static INLINE double 
IsLeft(HullVertex *p0, HullVertex *p1, HullVertex *p2) 
{
    return (((p1->x - p0->x) * (p2->y - p0->y)) - 
	    ((p2->x - p0->x) * (p1->y - p0->y)));
}

/*
 *---------------------------------------------------------------------------
 *
 * ChainHull2d --
 *
 *	Computes the convex hull from the vertices of the mesh.  
 *
 *	Notes: 1. The array of vertices is assumed to be sorted.  
 *	       2. An array to contain the vertices is assumed.  This is 
 *		  allocated by the caller.
 *	
 * Results:
 *     	The number of vertices in the convex hull is returned. The coordinates
 *	of the hull will be written in the given point array.
 *
 *	Copyright 2001, softSurfer (www.softsurfer.com) This code may be
 *	freely used and modified for any purpose providing that this copyright
 *	notice is included with it.  SoftSurfer makes no warranty for this
 *	code, and cannot be held liable for any real or imagined damage
 *	resulting from its use.  Users of this code must verify correctness
 *	for their application.
 *
 *---------------------------------------------------------------------------
 */
#define PUSH(i)	    (top++, hull[top] = (i))
#define POP()	    (top--)

static int 
ChainHull2d(int numPoints, HullVertex *points, int *hull) 
{
    int bot, top, i; 
    int minMin, minMax;
    int maxMin, maxMax;
    double xMin, xMax;

    bot = 0;				/* Index to bottom of stack. */
    top = -1;				/* Index to top of stack. */
    minMin = 0;

    /* 
     * Step 1. Get the indices of points with max x-coord and min|max
     *	       y-coord .
     */
    xMin = points[0].x;
    for (i = 1; i < numPoints; i++) {
	if (points[i].x != xMin) {
	    break;
	}
    }
    minMax = i - 1;
    
    if (minMax == (numPoints - 1)) {	/* Degenerate case: all x-coords ==
					 * xMin */
	PUSH(minMin);
	if (points[minMax].y != points[minMin].y) {
	    PUSH(minMax);		/* A nontrivial segment. */
	}
	PUSH(minMin);
	return top + 1;
    }

    maxMax = numPoints - 1;
    xMax = points[maxMax].x;
    for (i = numPoints - 2; i >= 0; i--) {
	if (points[i].x != xMax) {
	    break;
	}
    }
    maxMin = i + 1;

    /* Step 2. Compute the lower hull on the stack. */

    PUSH(minMin);			/* Push minMin point onto stack. */
    i = minMax;

    while (++i <= maxMin) {
	/* The lower line joins v[minMin] with v[maxMin]. */
	if ((IsLeft(points + minMin, points + maxMin, points + i) >= 0.0) && 
	    (i < maxMin)) {
	    continue;			/* Ignore points[i] above or on the
					 * lower line */
	}
	while (top > 0) {		/* There are at least 2 vertices on
					 * the stack. */

	    /* Test if points[i] is left of the line at the stack top. */
	    if (IsLeft(points+hull[top-1], points+hull[top], points+i) > 0.0) {
		break;			/* points[i] is a new hull vertex. */
	    } else {
		POP();			/* Pop top point off stack */
	    }
	}
	PUSH(i);			/* Push point[i] onto stack. */
    }

    /* Step 3. Compute the upper hull on the stack above the bottom hull. */
    if (maxMax != maxMin)  {		/* if distinct xMax points */
        PUSH(maxMax);			/* Push maxMax point onto stack. */
    }

    bot = top;				/* The bottom point of the upper hull
					   stack. */
    i = maxMin;

    while (--i >= minMax) {
        /* The upper line joins points[maxMax] with points[minMax]. */
        if ((IsLeft(points + maxMax, points + minMax, points + i) >= 0.0) && 
	    (i > minMax)) {
            continue;			/* Ignore points[i] below or on the
					 * upper line. */
	}
        while (top > bot) {		/* At least 2 points on the upper
					 * stack. */

            /*  Test if points[i] is left of the line at the stack top. */
            if (IsLeft(points+hull[top-1], points+hull[top], points+i) > 0.0) {
                break;			/* v[i] is a new hull vertex. */
	    } else {
                POP();			/* Pop top point off stack. */
	    }
        }
        PUSH(i);			/* Push points[i] onto stack. */
    }
    if (minMax != minMin) {
        PUSH(minMin);		       /* Push joining endpoint onto stack. */
    }
    return top + 1;
}


static int
CompareVertices(const void *a, const void *b)
{
    const HullVertex *v1 = a;
    const HullVertex *v2 = b;

    if (v1->y < v2->y) {
	return -1;
    }
    if (v1->y > v2->y) {
	return 1;
    }
    if (v1->x < v2->x) {
	return -1;
    }
    if (v1->x > v2->x) {
	return 1;
    }
    return 0;
}

static int
ConvexHull(Tcl_Interp *interp, int numPoints, Point2d *points, Mesh *meshPtr) 
{
    int *hull;
    HullVertex *vertices;
    int i, numVertices;
    
    vertices = Blt_Malloc(numPoints * sizeof(HullVertex));
    for (i = 0; i < numPoints; i++) {
	vertices[i].x = points[i].x;
	vertices[i].y = points[i].y;
	vertices[i].index = i;
    }
    if (vertices == NULL) {
	Tcl_AppendResult(interp, "can't allocate vertices for convex hull ", 
		(char *)NULL);
	return TCL_ERROR;
    }
    qsort(vertices, numPoints, sizeof(HullVertex), CompareVertices);

    /* Allocate worst-case storage initially for the hull. */
    hull = Blt_Malloc(numPoints * sizeof(int));
    if (hull == NULL) {
	Tcl_AppendResult(interp, "can't allocate memory for convex hull ", 
		(char *)NULL);
	Blt_Free(vertices);
	return TCL_ERROR;
    }

    /* Compute the convex hull. */
    numVertices = ChainHull2d(numPoints, vertices, hull);
    /* Resize the hull array to the actual # of boundary points. */
    if (numVertices < numPoints) {
	hull = Blt_Realloc(hull, numVertices * sizeof(int));
	if (hull == NULL) {
	    Tcl_AppendResult(interp, "can't reallocate memory for convex hull ",
		(char *)NULL);
	    Blt_Free(vertices);
	    return TCL_ERROR;
	}
    }
    if (meshPtr->hull != NULL) {
	Blt_Free(meshPtr->hull);
    }
    /* Remap the indices back to the unsorted point array. */
    for (i = 0; i < numVertices; i++) {
	hull[i] = vertices[hull[i]].index;
    }
    Blt_Free(vertices);
    meshPtr->hull = hull;
    meshPtr->numHullPts = numVertices;
    return TCL_OK;
}

static int 
ComputeMesh(Mesh *meshPtr)
{
    MeshTriangle *triangles;
    long numTriangles;
    long i, count;
    triangles = NULL;
    numTriangles = 0;
    
    if (meshPtr->numVertices > 0) {
	/* Compute the convex hull first, this will provide an estimate for
	 * the boundary vertices and therefore the number of triangles. */
	if (ConvexHull(meshPtr->interp, meshPtr->numVertices, meshPtr->vertices,
		       meshPtr) != TCL_OK) {
	    Tcl_AppendResult(meshPtr->interp, "can't compute convex hull", 
			     (char *)NULL);
	    goto error;
	}
	/* Determine the number of triangles. */
	numTriangles = 2 * meshPtr->numVertices;
	triangles = Blt_Malloc(numTriangles * sizeof(MeshTriangle));
	if (triangles == NULL) {
	    Tcl_AppendResult(meshPtr->interp, "can't allocate ", 
		Blt_Itoa(numTriangles), " triangles", (char *)NULL);
	    goto error;
	}
	numTriangles = Blt_Triangulate(meshPtr->interp, meshPtr->numVertices, 
		meshPtr->vertices, FALSE, triangles);
	if (numTriangles == 0) {
	    Tcl_AppendResult(meshPtr->interp, "error triangulating mesh", 
			     (char *)NULL);
	    goto error;
	}
    }
    /* Compress the triangle array. This is because there are hidden triangles
     * designated or we over-allocated the initial array of triangles. */
    count = 0;
    for (i = 0; i < numTriangles; i++) {
	if (Blt_FindHashEntry(&meshPtr->hideTable, (char *)i)) {
	    continue;
	}
	if (i > count) {
	    triangles[count] = triangles[i];
	}
	count++;
    }   
    if (count > 0) {
	triangles = Blt_Realloc(triangles, count * sizeof(MeshTriangle));
    }
    if (meshPtr->triangles != NULL) {
	Blt_Free(meshPtr->triangles);
    }
    meshPtr->numTriangles = numTriangles;
    meshPtr->triangles = triangles;
    return TCL_OK;
 error:
    if (triangles != NULL) {
	Blt_Free(triangles);
    }
    return TCL_ERROR;
}

static int
RegularMeshConfigureProc(Tcl_Interp *interp, Mesh *meshPtr)
{
    int i;
    double xStep, yStep;
    DataSourceResult x, y;
    double xMin, xMax, xNum, yNum, yMin, yMax;
    Point2d *vertices;
    int numVertices;

    interp = meshPtr->interp;
    if ((meshPtr->x == NULL) || (meshPtr->y == NULL)) {
	return TCL_OK;			/* Missing x or y vectors. */
    }
    if ((*meshPtr->x->classPtr->getProc)(interp, meshPtr->x, &x) != TCL_OK) {
	return TCL_ERROR;
    }
    if (x.numValues != 3) {
	Tcl_AppendResult(interp, 
		"wrong # of elements for x regular mesh description.",
		(char *)NULL);
	return TCL_ERROR;
    }
    if ((*meshPtr->y->classPtr->getProc)(interp, meshPtr->y, &y) != TCL_OK) {
	return TCL_ERROR;
    }
    if (y.numValues != 3) {
	Tcl_AppendResult(interp, 
		"wrong # of elements for y rectangular mesh description.",
		(char *)NULL);
	return TCL_ERROR;
    }
    xMin = x.values[0];
    xMax = x.values[1];
    xNum = (int)x.values[2];
    yMin = y.values[0];
    yMax = y.values[1];
    yNum = (int)y.values[2];
    Blt_Free(x.values);
    Blt_Free(y.values);
    
    if (xNum < 2) {
	Tcl_AppendResult(interp, "too few x-values (", Blt_Itoa(xNum), 
		") for rectangular mesh", (char *)NULL);
	return TCL_ERROR;
    }
    if (yNum < 2) {
	Tcl_AppendResult(interp, "too few y-values  (", Blt_Itoa(xNum), 
		") for rectangular mesh", (char *)NULL);
	return TCL_ERROR;
    }
    if (xMin == xMax) {
	return TCL_ERROR;
    }
    if (yMin == yMax) {
	return TCL_ERROR;
    }
    numVertices = xNum * yNum;
    vertices = Blt_Malloc(numVertices * sizeof(Point2d));
    if (vertices == NULL) {
	Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(numVertices), 
		" vertices", (char *)NULL);
	return TCL_ERROR;
    }
    xStep = (xMax - xMin) / (double)(xNum - 1);
    yStep = (yMax - yMin) / (double)(yNum - 1);
    {
	int count;
	Point2d *p;

	p = vertices;
	count = 0;
	for (i = 0; i < yNum; i++) {
	    double y0;
	    int j;
	    
	    y0 = yMin + (yStep * i);
	    for (j = 0; j < xNum; j++) {
		p->x = xMin + (xStep * j);
		p->y = y0;
		count++;
		p++;
	    }
	}
    }
    if (meshPtr->vertices != NULL) {
	Blt_Free(meshPtr->vertices);
    }
    meshPtr->xMin = xMin;
    meshPtr->xMax = xMax;
    meshPtr->yMin = yMin;
    meshPtr->yMax = yMax;
    meshPtr->vertices = vertices;
    meshPtr->numVertices = numVertices;
    return ComputeMesh(meshPtr);
}

static int
IrregularMeshConfigureProc(Tcl_Interp *interp, Mesh *meshPtr)
{
    DataSourceResult x, y;
    Point2d *vertices;
    int numVertices;

    if ((meshPtr->x == NULL) || (meshPtr->y == NULL)) {
	return TCL_OK;
    }
    if ((meshPtr->x->classPtr == NULL) || (meshPtr->y->classPtr == NULL)) {
	return TCL_OK;
    }
    if ((*meshPtr->x->classPtr->getProc)(interp, meshPtr->x, &x) != TCL_OK) {
	return TCL_ERROR;
    }
    if (x.numValues < 2) {
	Tcl_AppendResult(interp, "wrong # of x-values (", Blt_Itoa(x.numValues),
		 ") for irregular mesh description.", (char *)NULL);
	return TCL_ERROR;
    }
    meshPtr->xMin = x.min;
    meshPtr->xMax = x.max;
    if ((*meshPtr->y->classPtr->getProc)(meshPtr->interp, meshPtr->y, &y) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    if (y.numValues < 2) {
	Tcl_AppendResult(interp, "wrong # of y-values (", Blt_Itoa(y.numValues),
		 ") for irregular mesh description.", (char *)NULL);
	return TCL_ERROR;
    }
    meshPtr->yMin = y.min;
    meshPtr->yMax = y.max;
    numVertices = x.numValues * y.numValues;
    vertices = Blt_Malloc(numVertices * sizeof(Point2d));
    if (vertices == NULL) {
	Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(numVertices), 
		" vertices", (char *)NULL);
	return TCL_ERROR;
    }
    {
	int i;
	Point2d *p;

	p = vertices;
	for (i = 0; i < y.numValues; i++) {
	    int j;
	    
	    for (j = 0; j < x.numValues; j++) {
		p->x = x.values[j];
		p->y = y.values[i];
		p++;
	    }
	}
    }
    Blt_Free(x.values);
    Blt_Free(y.values);
    if (meshPtr->vertices != NULL) {
	Blt_Free(meshPtr->vertices);
    }
    meshPtr->xMin = x.min;
    meshPtr->xMax = x.max;
    meshPtr->yMin = y.min;
    meshPtr->yMax = y.max;
    meshPtr->vertices = vertices;
    meshPtr->numVertices = numVertices;
    return ComputeMesh(meshPtr);
}

static int
CloudMeshConfigureProc(Tcl_Interp *interp, Mesh *meshPtr)
{
    Blt_HashTable table;
    DataSourceResult x, y;
    Point2d *vertices;
    int i, numVertices, count;

    if ((meshPtr->x == NULL) || (meshPtr->y == NULL)) {
	return TCL_OK;
    }
    if ((meshPtr->x->classPtr == NULL) || (meshPtr->y->classPtr == NULL)) {
	return TCL_OK;
    }
    if ((*meshPtr->x->classPtr->getProc)(interp, meshPtr->x, &x) != TCL_OK) {
	return TCL_ERROR;
    }
    if (x.numValues < 3) {
	Tcl_AppendResult(interp, "bad cloud mesh: too few x-coordinates \"",
		Blt_Itoa(x.numValues), "\".", (char *)NULL);
	return TCL_ERROR;
    }
    if ((*meshPtr->y->classPtr->getProc)(interp, meshPtr->y, &y) != TCL_OK) {
	return TCL_ERROR;
    }
    if (y.numValues < 3) {
	Tcl_AppendResult(interp, "bad cloud mesh: too few y-coordinates \"",
		Blt_Itoa(y.numValues), "\".", (char *)NULL);
	return TCL_ERROR;
    }
    if (x.numValues != y.numValues) {
	Tcl_AppendResult(interp, 
	"bad cloud mesh: # of values for x and y coordinates do not match.",
		(char *)NULL);
	return TCL_ERROR;
    }
    numVertices = x.numValues;
    vertices = Blt_Malloc(numVertices * sizeof(Point2d));
    if (vertices == NULL) {
	Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(numVertices), 
			 " vertices", (char *)NULL);
	return TCL_ERROR;
    }
    Blt_InitHashTable(&table, sizeof(MeshKey) / sizeof(int));
    count = 0;
    for (i = 0; i < numVertices; i++) {
	Blt_HashEntry *hPtr;
	int isNew;
	MeshKey key;
	long index;

	key.x = x.values[i];
	key.y = y.values[i];
	hPtr = Blt_CreateHashEntry(&table, (char *)&key, &isNew);
	assert(hPtr != NULL);
	if (!isNew) {
	    index = (long)Blt_GetHashValue(hPtr);
	    fprintf(stderr, "duplicate point %d x=%g y=%g, old=%ld x=%g y=%g\n",
		    i, x.values[i], y.values[i], index, x.values[index], 
		    y.values[index]);
	    continue;
	}
	index = (long)i;
	Blt_SetHashValue(hPtr, (ClientData)index);
	vertices[count].x = x.values[i];
	vertices[count].y = y.values[i];
	count++;
    }
    Blt_DeleteHashTable(&table);
    Blt_Free(x.values);
    Blt_Free(y.values);
    if (meshPtr->vertices != NULL) {
	Blt_Free(meshPtr->vertices);
    }
    meshPtr->xMin = x.min;
    meshPtr->xMax = x.max;
    meshPtr->yMin = y.min;
    meshPtr->yMax = y.max;
    meshPtr->vertices = vertices;
    meshPtr->numVertices = count;
    return ComputeMesh(meshPtr);
}

static int
TriangleMeshConfigureProc(Tcl_Interp *interp, Mesh *meshPtr)
{
    DataSourceResult x, y;
    Point2d *vertices;
    int i, numVertices, numTriangles, count;
    MeshTriangle *triangles;

    if ((meshPtr->x == NULL) || (meshPtr->y == NULL)) {
	return TCL_OK;
    }
    if (meshPtr->numTriples == 0) {
	return TCL_OK;
    }
    if ((meshPtr->x->classPtr == NULL) || (meshPtr->y->classPtr == NULL)) {
	return TCL_OK;
    }
    if ((*meshPtr->x->classPtr->getProc)(interp, meshPtr->x, &x) != TCL_OK) {
	return TCL_ERROR;
    }
    if (x.numValues < 2) {
	Tcl_AppendResult(interp, "wrong # of x-values (", Blt_Itoa(x.numValues),
		 ") for irregular mesh description.", (char *)NULL);
	return TCL_ERROR;
    }
    if ((*meshPtr->y->classPtr->getProc)(meshPtr->interp, meshPtr->y, &y) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    if (y.numValues < 2) {
	Tcl_AppendResult(interp, "wrong # of y-values (", Blt_Itoa(y.numValues),
		 ") for irregular mesh description.", (char *)NULL);
	return TCL_ERROR;
    }
    if (x.numValues != y.numValues) {
	Tcl_AppendResult(interp, " # of values for x and y do not match.",
		(char *)NULL);
	return TCL_ERROR;
    }
    numVertices = x.numValues;
    vertices = Blt_Malloc(numVertices * sizeof(Point2d));
    if (vertices == NULL) {
	Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(numVertices), 
		" vertices", (char *)NULL);
	return TCL_ERROR;
    }
   /* Fill the vertices array with the sorted x-y coordinates. */
    for (i = 0; i < numVertices; i++) {
	vertices[i].x = x.values[i];
	vertices[i].y = y.values[i];
    }
    Blt_Free(x.values);
    Blt_Free(y.values);
    triangles = NULL;
    if (ConvexHull(meshPtr->interp, numVertices, vertices, meshPtr) != TCL_OK) {
	Tcl_AppendResult(meshPtr->interp, "can't compute convex hull", 
			 (char *)NULL);
	goto error;
    }
    numTriangles = meshPtr->numTriples;
    /* Fill the triangles array with the sorted indices of the vertices. */
    triangles = Blt_AssertCalloc(numTriangles, sizeof(MeshTriangle));
    for (i = 0; i < numTriangles; i++) {
	MeshTriangle *t;

	t = meshPtr->triples + i;
	if ((t->a < 0) || (t->a >= numVertices)) {
	    Tcl_AppendResult(meshPtr->interp, "first index on triangle ",
			     Blt_Itoa(i), " is out of range.",
			     (char *)NULL);
	    goto error;
	}

	if ((t->b < 0) || (t->b >= numVertices)) {
	    Tcl_AppendResult(meshPtr->interp, "second index on triangle ",
			     Blt_Itoa(i), " is out of range.",
			     (char *)NULL);
	    goto error;
	}
	if ((t->c < 0) || (t->c >= numVertices)) {
	    Tcl_AppendResult(meshPtr->interp, "third index on triangle ",
			     Blt_Itoa(i), " is out of range.",
			     (char *)NULL);
	    goto error;
	}
	triangles[i].a = t->a;
	triangles[i].b = t->b;
	triangles[i].c = t->c;
    }
    /* Compress the triangle array. */
    count = 0;
    for (i = 0; i < numTriangles; i++) {
	long index;

	index = (long)i;
	if (Blt_FindHashEntry(&meshPtr->hideTable, (char *)index)) {
	    continue;
	}
	if (i > count) {
	    meshPtr->triangles[count] = meshPtr->triangles[i];
	}
	count++;
    }   
    if ((count > 0) && (count != numTriangles)) {
	triangles = Blt_Realloc(triangles, count * sizeof(MeshTriangle));
	if (triangles == NULL) {
	    Tcl_AppendResult(meshPtr->interp, 
		"can't reallocated triangle array for mesh \"", meshPtr->name,
		"\".", (char *)NULL);
	    goto error;
	}
	numTriangles = count;
    }
    if (meshPtr->vertices != NULL) {
	Blt_Free(meshPtr->vertices);
    }
    meshPtr->vertices = vertices;
    meshPtr->numVertices = numVertices;
    if (meshPtr->triangles != NULL) {
	Blt_Free(meshPtr->triangles);
    }
    meshPtr->triangles = triangles;
    meshPtr->numTriangles = numTriangles;
    meshPtr->xMin = x.min;
    meshPtr->xMax = x.max;
    meshPtr->yMin = y.min;
    meshPtr->yMax = y.max;
    return TCL_OK;
 error:
    if (vertices != NULL) {
	Blt_Free(vertices);
    }
    if (triangles != NULL) {
	Blt_Free(triangles);
    }
    return TCL_ERROR;
}

static Mesh *
NewMesh(Tcl_Interp *interp, MeshCmdInterpData *dataPtr, int type, 
	Blt_HashEntry *hPtr)
{
    Mesh *meshPtr;

    /* Allocate memory for the new mesh. */
    meshPtr = Blt_AssertCalloc(1, sizeof(Mesh));
    switch (type) {
    case MESH_IRREGULAR:
	meshPtr->classPtr = &irregularMeshClass;
	break;
    case MESH_REGULAR:
	meshPtr->classPtr = &regularMeshClass;
	break;
    case MESH_TRIANGLE:
	meshPtr->classPtr = &triangleMeshClass;
	break;
    case MESH_CLOUD:
	meshPtr->classPtr = &cloudMeshClass;
	break;
    default:
	return NULL;
    }
    meshPtr->name = Blt_GetHashKey(&dataPtr->meshTable, hPtr);
    meshPtr->hashPtr = hPtr;
    meshPtr->interp = interp;
    meshPtr->dataPtr = dataPtr;
    Blt_SetHashValue(hPtr, meshPtr);
    Blt_InitHashTable(&meshPtr->notifierTable, BLT_ONE_WORD_KEYS);
    Blt_InitHashTable(&meshPtr->tableTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&meshPtr->hideTable, BLT_ONE_WORD_KEYS);
    return meshPtr;
}

/* ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    MeshCmdInterpData *dataPtr = clientData;
    Mesh *meshPtr;


    if (GetMeshFromObj(interp, dataPtr, objv[2], &meshPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return Blt_SwitchValue(interp, meshPtr->classPtr->specs, (char *)meshPtr, 
	objv[3], 0);
}

static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    Mesh *meshPtr;
    MeshCmdInterpData *dataPtr = clientData;

    if (GetMeshFromObj(interp, dataPtr, objv[2], &meshPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 3) {
	return Blt_SwitchInfo(interp, meshPtr->classPtr->specs, meshPtr,
		(Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
	return Blt_SwitchInfo(interp, meshPtr->classPtr->specs, meshPtr, 
		objv[3], 0);
    }
    bltDataSourceSwitch.clientData = meshPtr;
    if (Blt_ParseSwitches(interp, meshPtr->classPtr->specs, objc - 3, objv + 3,
	(char *)meshPtr, 0) < 0) {
	return TCL_ERROR;
    }
    ConfigureMesh(meshPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *	Creates a mesh.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the element names.
 *
 *	blt::mesh create $type ?$name? ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    MeshCmdInterpData *dataPtr = clientData;
    Mesh *meshPtr;
    Blt_HashEntry *hPtr;
    char ident[200];
    const char *name;
    int isNew;
    char c;
    int length;
    const char *string;
    Tcl_DString ds;
    int type;

    string = Tcl_GetString(objv[2]);
    c = string[0];
    length = strlen(string);
    if ((c == 't') && (strncmp(string, "triangle", length) == 0)) {
	type = MESH_TRIANGLE;
    } else if ((c == 'r') && (strncmp(string, "regular", length) == 0)) {
	type = MESH_REGULAR;
    } else if ((c == 'i') && (strncmp(string, "irregular", length) == 0)) {
	type = MESH_IRREGULAR;
    } else if ((c == 'c') && (strncmp(string, "cloud", length) == 0)) {
	type = MESH_CLOUD;
    } else {
	Tcl_AppendResult(interp, "unknown mesh type \"", string, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    name = NULL;
    Tcl_DStringInit(&ds);
    if (objc > 3) {
	string = Tcl_GetString(objv[3]);
	if (string[0] != '-') {
	    Blt_ObjectName objName;

	    /* 
	     * Parse the command and put back so that it's in a consistent
	     * format.
	     *
	     *	t1         <current namespace>::t1
	     *	n1::t1     <current namespace>::n1::t1
	     *	::t1	   ::t1
	     *  ::n1::t1   ::n1::t1
	     */
	    if (!Blt_ParseObjectName(interp, string, &objName, 0)) {
		return TCL_ERROR;
	    }
	    name = Blt_MakeQualifiedName(&objName, &ds);
	    if (Blt_FindHashEntry(&dataPtr->meshTable, name) != NULL) {
		Tcl_AppendResult(interp, "mesh \"", name, 
			"\" already exists", (char *)NULL);
		return TCL_ERROR;
	    }
	    objc--, objv++;
	}
    }
    /* If no name was given for the marker, make up one. */
    if (name == NULL) {
	Blt_ObjectName objName;

	Blt_FormatString(ident, 200, "mesh%d", dataPtr->nextMeshId++);
	if (!Blt_ParseObjectName(interp, ident, &objName, 0)) {
	    return TCL_ERROR;
	}
	name = Blt_MakeQualifiedName(&objName, &ds);
    }
    hPtr = Blt_CreateHashEntry(&dataPtr->meshTable, name, &isNew);
    Tcl_DStringFree(&ds);
    if (!isNew) {
	Tcl_AppendResult(interp, "mesh \"", name, "\" already exists",
			 (char *)NULL);
	return TCL_ERROR;
    }
    switch (type) {
    case MESH_TRIANGLE:
	meshPtr = NewMesh(interp, dataPtr, MESH_TRIANGLE, hPtr);
	break;
    case MESH_REGULAR:
	meshPtr = NewMesh(interp, dataPtr, MESH_REGULAR, hPtr);
	break;
    case MESH_IRREGULAR:
	meshPtr = NewMesh(interp, dataPtr, MESH_IRREGULAR, hPtr);
	break;
    case MESH_CLOUD:
	meshPtr = NewMesh(interp, dataPtr, MESH_CLOUD, hPtr);
	break;
    }
    /* Parse the configuration options. */
    if (Blt_ParseSwitches(interp, meshPtr->classPtr->specs, objc - 3, objv + 3, 
	(char *)meshPtr, BLT_SWITCH_INITIALIZE) < 0) {
	DestroyMesh(meshPtr);
	return TCL_ERROR;
    }
    if (!isNew) {
	Mesh *oldMeshPtr;
	oldMeshPtr = Blt_GetHashValue(hPtr);
	if ((oldMeshPtr->flags & DELETE_PENDING) == 0) {
	    Tcl_AppendResult(interp, "mesh \"", meshPtr->name,
		"\" already exists", (char *)NULL);
	    DestroyMesh(meshPtr);
	    return TCL_ERROR;
	}
	oldMeshPtr->hashPtr = NULL;	/* Remove the mesh from the table. */
    }
    if ((meshPtr->classPtr->configProc)(interp, meshPtr) != TCL_OK) {
	DestroyMesh(meshPtr);
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), meshPtr->name, -1);
    return TCL_OK;
}

static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    MeshCmdInterpData *dataPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
	Mesh *meshPtr;

	if (GetMeshFromObj(interp, dataPtr, objv[i], &meshPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	DestroyMesh(meshPtr);
    }
    return TCL_OK;
}

static int
HullOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    MeshCmdInterpData *dataPtr = clientData;
    Mesh *meshPtr;
    int i;
    Tcl_Obj *listObjPtr;
    int wantPoints;

    if (GetMeshFromObj(interp, dataPtr, objv[2], &meshPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    wantPoints = FALSE;
    if (objc > 3) {
	const char *string;

	string = Tcl_GetString(objv[3]);
	if (strcmp(string, "-vertices") == 0) {
	    wantPoints = TRUE;
	}
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (wantPoints) {
	for (i = 0; i < meshPtr->numHullPts; i++) {
	    Tcl_Obj *objPtr;
	    Point2d *p;

	    p = meshPtr->vertices + meshPtr->hull[i];
	    objPtr = Tcl_NewDoubleObj(p->x);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    objPtr = Tcl_NewDoubleObj(p->y);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    } else {
	for (i = 0; i < meshPtr->numHullPts; i++) {
	    Tcl_Obj *objPtr;
	    
	    objPtr = Tcl_NewIntObj(meshPtr->hull[i]);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *	Returns a list of marker identifiers in interp->result;
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
    MeshCmdInterpData *dataPtr = clientData;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 2) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&dataPtr->meshTable, &iter); 
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Mesh *meshPtr;

	    meshPtr = Blt_GetHashValue(hPtr);
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj(meshPtr->name, -1));
	}
    } else {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&dataPtr->meshTable, &iter); 
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Mesh *meshPtr;
	    int i;

	    meshPtr = Blt_GetHashValue(hPtr);
	    for (i = 2; i < objc; i++) {
		const char *pattern;

		pattern = Tcl_GetString(objv[i]);
		if (Tcl_StringMatch(meshPtr->name, pattern)) {
		    Tcl_ListObjAppendElement(interp, listObjPtr,
			Tcl_NewStringObj(meshPtr->name, -1));
		    break;
		}
	    }
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
VerticesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    MeshCmdInterpData *dataPtr = clientData;
    Mesh *meshPtr;
    size_t i;
    Tcl_Obj *listObjPtr;

    if (GetMeshFromObj(interp, dataPtr, objv[2], &meshPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (i = 0; i <  meshPtr->numVertices; i++) {
	Tcl_Obj *subListObjPtr, *objPtr;
	Point2d *p;

	p = meshPtr->vertices + i;
	subListObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

	objPtr = Tcl_NewIntObj(i);
	Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);
	objPtr = Tcl_NewDoubleObj(p->x);
	Tcl_ListObjAppendElement (interp, subListObjPtr, objPtr);
	objPtr = Tcl_NewDoubleObj(p->y);
	Tcl_ListObjAppendElement(interp, subListObjPtr, objPtr);

	Tcl_ListObjAppendElement(interp, listObjPtr, subListObjPtr);

    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
TrianglesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    MeshCmdInterpData *dataPtr = clientData;
    Mesh *meshPtr;
    MeshTriangle *t, *tend;
    Tcl_Obj *listObjPtr;

    if (GetMeshFromObj(interp, dataPtr, objv[2], &meshPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (t = meshPtr->triangles, tend = t + meshPtr->numTriangles; 
	 t < tend; t++) {
	Tcl_Obj *subListObjPtr;
	
	subListObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	Tcl_ListObjAppendElement(interp, subListObjPtr, Tcl_NewIntObj(t->a));
	Tcl_ListObjAppendElement(interp, subListObjPtr, Tcl_NewIntObj(t->b));
	Tcl_ListObjAppendElement(interp, subListObjPtr, Tcl_NewIntObj(t->c));
	Tcl_ListObjAppendElement(interp, listObjPtr, subListObjPtr);

    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static int
TypeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    MeshCmdInterpData *dataPtr = clientData;
    Mesh *meshPtr;

    if (GetMeshFromObj(interp, dataPtr, objv[2], &meshPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), meshPtr->classPtr->name, -1);
    return TCL_OK;
}

static int
HideOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    MeshCmdInterpData *dataPtr = clientData;
    Mesh *meshPtr;
    int i;

    if (GetMeshFromObj(interp, dataPtr, objv[2], &meshPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (meshPtr->hideTable.numEntries > 0) {
	Blt_DeleteHashTable(&meshPtr->hideTable);
    }
    Blt_InitHashTable(&meshPtr->hideTable, BLT_ONE_WORD_KEYS);
    for (i = 3; i < objc; i++) {
	long index;
	Blt_HashEntry *hPtr;
	int isNew;

	if (Blt_GetLongFromObj(interp, objv[i], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	hPtr = Blt_CreateHashEntry(&meshPtr->hideTable, (char *)index, &isNew);
	Blt_SetHashValue(hPtr, index);
    }
    if (meshPtr->classPtr->type != MESH_TRIANGLE) {
	ComputeMesh(meshPtr);
    }
    NotifyClients(meshPtr, MESH_CHANGE_NOTIFY);
    return TCL_OK;
}

static void
DestroyMeshes(MeshCmdInterpData *dataPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&dataPtr->meshTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Mesh *meshPtr;

	meshPtr = Blt_GetHashValue(hPtr);
	meshPtr->hashPtr = NULL;
	DestroyMesh(meshPtr);
    }
}

void
Blt_FreeMesh(Mesh *meshPtr)
{
    if (meshPtr != NULL) {
	DestroyMesh(meshPtr);
    }
}

int
Blt_GetMeshFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Mesh **meshPtrPtr)
{
    Mesh *meshPtr;
    MeshCmdInterpData *dataPtr;

    dataPtr = GetMeshCmdInterpData(interp);
    if (GetMeshFromObj(interp, dataPtr, objPtr, &meshPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    *meshPtrPtr = meshPtr;
    return TCL_OK;
}

int
Blt_GetMesh(Tcl_Interp *interp, const char *string, Mesh **meshPtrPtr)
{
    Blt_HashEntry *hPtr;
    MeshCmdInterpData *dataPtr;

    dataPtr = GetMeshCmdInterpData(interp);
    hPtr = Blt_FindHashEntry(&dataPtr->meshTable, string);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "can't find a mesh \"", string, "\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    *meshPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

void
Blt_CreateMeshNotifier(Mesh *meshPtr, MeshNotifyProc *proc, 
		       ClientData clientData)
{
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&meshPtr->notifierTable, clientData, &isNew);
    Blt_SetHashValue(hPtr, proc);
}

void
Blt_DeleteMeshNotifier(Mesh *meshPtr, ClientData clientData)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&meshPtr->notifierTable, clientData);
    Blt_DeleteHashEntry(&meshPtr->notifierTable, hPtr);
}

const char *
Blt_NameOfMesh(Mesh *meshPtr)
{
    return meshPtr->name;
}

/*
 *---------------------------------------------------------------------------
 *
 * MeshCmd --
 *
 *	.g mesh create regular ?$name? -x {x0 xN n} -y {y0 yN n}
 *	.g mesh create irregular ?$name? -x $xvalues -y $yvalues 
 *	.g mesh create triangle ?$name? -x x -y y -triangles $triangles
 *	.g mesh create cloud ?$name? -x x -y y 
 *	.g mesh type $name
 *	.g mesh names ?pattern?
 *	.g mesh delete $name
 *	.g mesh configure $name -hide no -linewidth 1 -color black
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec meshOps[] =
{
    {"cget",        2, CgetOp,       3, 4, "name option",},
    {"configure",   2, ConfigureOp,  2, 0, "name ?option value?...",},
    {"create",      2, CreateOp,     3, 0, "type ?name? ?option value?...",},
    {"delete",      1, DeleteOp,     2, 0, "?name?...",},
    {"hide",        2, HideOp,       3, 4, "name ?indices...?",},
    {"hull",        2, HullOp,       3, 4, "name ?-vertices?",},
    {"names",       1, NamesOp,      2, 0, "?pattern?...",},
    {"triangles",   2, TrianglesOp,  3, 3, "name",},
    {"type",        2, TypeOp,       3, 3, "name",},
    {"vertices",    1, VerticesOp,   3, 3, "name",},
};
static int numMeshOps = sizeof(meshOps) / sizeof(Blt_OpSpec);

static int
MeshCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numMeshOps, meshOps, BLT_OP_ARG1, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * MeshInterpDeleteProc --
 *
 *	This is called when the interpreter registering the "mesh"
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
MeshInterpDeleteProc(ClientData clientData, Tcl_Interp *interp)
{
    MeshCmdInterpData *dataPtr = clientData;

    /* All table instances should already have been destroyed when their
     * respective TCL commands were deleted. */
    DestroyMeshes(dataPtr);
    Blt_DeleteHashTable(&dataPtr->meshTable);
    Tcl_DeleteAssocData(interp, MESH_THREAD_KEY);
    Blt_Free(dataPtr);
}

/*
 *
 * GetMeshCmdInterpData --
 *
 */
static MeshCmdInterpData *
GetMeshCmdInterpData(Tcl_Interp *interp)
{
    MeshCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (MeshCmdInterpData *)
	Tcl_GetAssocData(interp, MESH_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
	dataPtr = Blt_AssertMalloc(sizeof(MeshCmdInterpData));
	dataPtr->interp = interp;
	Tcl_SetAssocData(interp, MESH_THREAD_KEY, MeshInterpDeleteProc, 
		dataPtr);
	Blt_InitHashTable(&dataPtr->meshTable, BLT_STRING_KEYS);
	dataPtr->nextMeshId = 0;
    }
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MeshCmdInitProc --
 *
 *	This procedure is invoked to initialize the "mesh" command.
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
Blt_MeshCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "mesh", MeshCmd, };

    cmdSpec.clientData = GetMeshCmdInterpData(interp);
    if (Blt_InitCmd(interp, "::blt", &cmdSpec) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

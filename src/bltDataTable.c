
/*
 *
 * bltDataTable.c --
 *
 *	Copyright 1998-2005 George A Howlett.
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

#define BUILD_BLT_TCL_PROCS 1
#include <bltInt.h>
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */
#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */
#include <bltAlloc.h>
#include "bltMath.h"
#include <bltHash.h>
#include <bltPool.h>
#include <bltNsUtil.h>
#include <bltArrayObj.h>
#include <bltDataTable.h>


/*
 * Row and Column Information Structures
 *
 *	Map:	Array of pointers to headers, representing the logical 
 *		view of row/column.  
 * 
 *	x = pointer to row/column header.
 *	y = row/column header.
 *		label
 *		index:  logical location of row/column.
 *		offset: physical location of row/column in table storage.
 *		type:   column type.
 *		flags:  
 *
 *	[x] [x] [x] [x] [x] [x] [x] [x] [ ] [ ] [ ]
 *	 |   |   |   |   |   |   |   |
 *       v   v   v   v   v   v   v   v 
 *      [y] [y] [y] [y] [y] [y] [y] [y]
 *
 *
 *	Free list:  Chain of free locations. Holds the physical offset
 *		    of next free row or column.  The offsets of deleted 
 *		    rows/columns are prepended to this list.
 *
 *	x = offset of free row/column in table storage.
 *
 *      [x]->[x]->[x]->[x]->[x]
 *
 *	Header pool: Pool of row/column headers.  Act as smart pointers
 *		     to row/column locations.  Will remain valid even if 
 *		     the logical view is changed (i.e. sorting) or physical 
 *		     storage is compacted.
 *
 *  Data Vectors.
 *	Vectors: array of Value arrays.  
 *
 *	    x = pointer to Value array.
 *	    y = array of Values.
 *
 *	Array of vectors: [x] [x] [x] [x] [x] [ ] [x] [x] [x] [ ] [ ] [ ]
 *			  [y] [y] [y] [y] [y]     [y] [y] [y]
 *			  [y] [y] [y] [y] [y]     [y] [y] [y]
 *			  [y] [y] [ ] [y] [y]     [y] [y] [y]
 *			  [y] [y] [y] [y] [y]     [y] [y] [y]
 *			  [y] [y] [y] [y] [y]     [y] [y] [y]
 *			  [y] [y] [y] [ ] [y]     [y] [y] [y]
 *			  [y] [y] [y] [y] [y]     [y] [y] [y]
 *			  [y] [y] [y] [y] [y]     [y] [ ] [y]
 *			  [y] [y] [y] [y] [y]     [y] [y] [y]
 *			  [y] [y] [y] [y] [y]     [y] [y] [y]
 *			  [y] [y] [y] [y] [y]     [y] [y] [y]
 *			  [y] [y] [y] [y] [y]     [y] [y] [y]
 *			  [y] [y] [y] [y] [y]     [y] [y] [y]
 *			  [y] [y] [y] [y] [y]     [y] [y] [y]
 *			  [ ] [ ] [ ] [ ] [ ]     [ ] [ ] [ ]
 *			  [ ] [ ] [ ] [ ] [ ]     [ ] [ ] [ ]
 *			  [ ] [ ] [ ] [ ] [ ]     [ ] [ ] [ ]
 *			  [ ] [ ] [ ] [ ] [ ]     [ ] [ ] [ ]
 *			  [ ] [ ] [ ] [ ] [ ]     [ ] [ ] [ ]
 *
 */

#define NumColumnsAllocated(t)		((t)->corePtr->columns.numAllocated)
#define NumRowsAllocated(t)		((t)->corePtr->rows.numAllocated)

#define TABLE_THREAD_KEY		"BLT DataTable Data"
#define TABLE_MAGIC			((unsigned int) 0xfaceface)
#define TABLE_DESTROYED			(1<<0)

#define TABLE_ALLOC_MAX_DOUBLE_SIZE	(1<<16)
#define TABLE_ALLOC_MAX_CHUNK		(1<<16)

#define TABLE_KEYS_DIRTY		(1<<0)
#define TABLE_KEYS_UNIQUE		(1<<1)

/* Column flag. */
#define TABLE_COLUMN_PRIMARY_KEY	(1<<0)

typedef struct _BLT_TABLE_VALUE Value;

typedef struct {
    long numRows, numCols;
    long mtime, ctime;
    const char *fileName;
    long numLines;
    unsigned int flags;
    int argc;
    const char **argv;
    Blt_HashTable rowIndices, colIndices;
} RestoreData;

typedef struct _BLT_TABLE Table;
typedef struct _BLT_TABLE_TAGS Tags;
typedef struct _BLT_TABLE_TRACE Trace;
typedef struct _BLT_TABLE_NOTIFIER Notifier;

static const char *valueTypes[] = {
    "string", "int", "double", "long", "time",
};

/*
 * _BLT_TABLE_TAGS --
 *
 *	Structure representing tags used by a client of the table.
 *
 *	Table rows and columns may be tagged with strings.  A row may have
 *	many tags.  The same tag may be used for many rows.  Tags are used and
 *	stored by clients of a table.  Tags can also be shared between clients
 *	of the same table.
 *	
 *	Both rowTable and columnTable are hash tables keyed by the physical
 *	row or column location in the table respectively.  This is not the
 *	same as the client's view (the order of rows or columns as seen by the
 *	client).  This is so that clients (which may have different views) can
 *	share tags without sharing the same view.
 */
struct _BLT_TABLE_TAGS {
    Blt_HashTable rowTable;		/* Table of row indices.  Each entry
					 * is itself a hash table of tag
					 * names. */
    Blt_HashTable columnTable;		/* Table of column indices.  Each
					 * entry is itself a hash table of tag
					 * names. */
    int refCount;			/* Tracks the number of clients
					 * currently using these tags. If
					 * refCount goes to zero, this means
					 * the table can safely be freed. */
};

typedef struct {
    Blt_HashTable clientTable;		/* Tracks all table clients. */
    unsigned int nextId;
    Tcl_Interp *interp;
} InterpData;

typedef struct _BLT_TABLE_HEADER Header;
typedef struct _BLT_TABLE_ROW Row;
typedef struct _BLT_TABLE_COLUMN Column;
typedef struct _BLT_TABLE_CORE TableObject;
typedef struct _BLT_TABLE_ROWCOLUMN RowColumn;

typedef struct {
    BLT_TABLE_ROW row;
    BLT_TABLE_COLUMN column;
} RowColumnKey;

static BLT_TABLE_ROWCOLUMN_CLASS rowClass = { 
    "row", sizeof(struct _BLT_TABLE_ROW)
};

static BLT_TABLE_ROWCOLUMN_CLASS columnClass = { 
    "column", sizeof(struct _BLT_TABLE_COLUMN) 
};

static Tcl_InterpDeleteProc TableInterpDeleteProc;
static void DestroyClient(Table *tablePtr);
static void NotifyClients(Table *tablePtr, BLT_TABLE_NOTIFY_EVENT *eventPtr);

static void
FreeRowColumn(RowColumn *rcPtr)
{
    Header **hpp, **hend;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    for (hPtr = Blt_FirstHashEntry(&rcPtr->labelTable, &cursor); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&cursor)) {
	Blt_HashTable *tablePtr;
	
	tablePtr = Blt_GetHashValue(hPtr);
	Blt_DeleteHashTable(tablePtr);
	Blt_Free(tablePtr);
    }
    Blt_DeleteHashTable(&rcPtr->labelTable);
    Blt_Chain_Destroy(rcPtr->freeList);

    for (hpp = rcPtr->map, hend = hpp + rcPtr->numUsed; hpp < hend; hpp++) {
	Blt_Pool_FreeItem(rcPtr->headerPool, *hpp);
    }
    Blt_Pool_Destroy(rcPtr->headerPool);
    Blt_Free(rcPtr->map);
}

static void
UnsetLabel(RowColumn *rcPtr, Header *headerPtr)
{
    Blt_HashEntry *hPtr;

    if (headerPtr->label == NULL) {
	return;
    }
    hPtr = Blt_FindHashEntry(&rcPtr->labelTable, headerPtr->label);
    if (hPtr != NULL) {
	Blt_HashTable *tablePtr;
	Blt_HashEntry *hPtr2;

	tablePtr = Blt_GetHashValue(hPtr);
	hPtr2 = Blt_FindHashEntry(tablePtr, (char *)headerPtr);
	if (hPtr2 != NULL) {
	    Blt_DeleteHashEntry(tablePtr, hPtr2);
	}
	if (tablePtr->numEntries == 0) {
	    Blt_DeleteHashEntry(&rcPtr->labelTable, hPtr);
	    Blt_DeleteHashTable(tablePtr);
	    Blt_Free(tablePtr);
	}
    }	
    headerPtr->label = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetLabel --
 *
 *	Changes the label for the row or column.  Labels aren't necessarily
 *	unique. It's not enforced.  The rationale is that it is convenient
 *	to be able to add rows/columns to a table, and then change the 
 *	labels.  For example, when importing table data from a file,
 *	you can't apriori change the labels.  We could add #n to make the 
 *	label unique, but detecting and changing them is a pain.
 *	
 *
 * Results:
 *	Returns a pointer to the new object is successful, NULL otherwise.  If
 *	a table object can't be generated, interp->result will contain an
 *	error message.
 *
 * -------------------------------------------------------------------------- 
 */
static void
SetLabel(RowColumn *rcPtr, Header *headerPtr, const char *newLabel)
{
    Blt_HashEntry *hPtr;
    int isNew;
    Blt_HashTable *tablePtr;		/* Secondary table. */

    if (headerPtr->label != NULL) {
	UnsetLabel(rcPtr, headerPtr);
    }
    if (newLabel == NULL) {
	return;
    }
    /* Check the primary label table for the bucket.  */
    hPtr = Blt_CreateHashEntry(&rcPtr->labelTable, newLabel, &isNew);
    if (isNew) {
	tablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
	Blt_InitHashTable(tablePtr, BLT_ONE_WORD_KEYS);
	Blt_SetHashValue(hPtr, tablePtr);
    } else {
	tablePtr = Blt_GetHashValue(hPtr);
    }
    /* Save the label as the hash entry key.  */
    headerPtr->label = Blt_GetHashKey(&rcPtr->labelTable, hPtr);
    /* Now look the header in the secondary table. */
    hPtr = Blt_CreateHashEntry(tablePtr, (char *)headerPtr, &isNew);
    if (!isNew) {
	return;				/* It's already there. */
    }
    /* Add it to the secondary table. */
    Blt_SetHashValue(hPtr, headerPtr);
}

static int
CheckLabel(Tcl_Interp *interp, RowColumn *rcPtr, const char *label)
{
    char c;

    c = label[0];
    /* This is so we know where switches end. */
    if (c == '-') {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, rcPtr->classPtr->name, " label \"", 
			label, "\" can't start with a '-'.", (char *)NULL);
	}
	return TCL_ERROR;
    }
    if (isdigit(UCHAR(c))) {
	long index;

	if (Blt_GetLong(NULL, (char *)label, &index) == TCL_OK) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, rcPtr->classPtr->name, " label \"", 
			label, "\" can't be a number.", (char *)NULL);
	    }
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

static int
SetHeaderLabel(Tcl_Interp *interp, RowColumn *rcPtr, Header *headerPtr, 
	       const char *newLabel)
{
    if (CheckLabel(interp, rcPtr, newLabel) != TCL_OK) {
	return TCL_ERROR;
    }
    SetLabel(rcPtr, headerPtr, newLabel);
    return TCL_OK;
}

#if (SIZEOF_VOID_P == 8)  
#define LABEL_FMT	"%c%ld"
#else
#define LABEL_FMT	"%c%d" 
#endif

static void
GetNextLabel(RowColumn *rcPtr, Header *headerPtr)
{
    char label[200];

    for(;;) {
	Blt_HashEntry *hPtr;

	Blt_FormatString(label, 200, LABEL_FMT, rcPtr->classPtr->name[0], 
		rcPtr->nextId++);
	hPtr = Blt_FindHashEntry(&rcPtr->labelTable, label);
	if (hPtr == NULL) {
	    SetLabel(rcPtr, headerPtr, label);
	    return;
	}
    }
}

static long
GetMapSize(long oldLen, long extra)
{
    long newLen, reqLen;

    reqLen = oldLen + extra;
    newLen = oldLen;
    if (newLen == 0) {
	newLen = 1;
    }
    if (reqLen < TABLE_ALLOC_MAX_DOUBLE_SIZE) {
	while (newLen < reqLen) {
	    newLen += newLen;
	}
    } else {
	while (newLen < reqLen) {
	    newLen += TABLE_ALLOC_MAX_CHUNK;
	}
    }
    return newLen;
}

static int
GrowHeaders(RowColumn *rcPtr, long extra)
{
    long newSize, oldSize;
    Header **map;

    newSize = GetMapSize(rcPtr->numAllocated, extra);
    oldSize = rcPtr->numAllocated;
    map = rcPtr->map;
    if (map == NULL) {
	map = Blt_Malloc(sizeof(Header *) * newSize);
    } else {
	map = Blt_Realloc(rcPtr->map, sizeof(Header *) * newSize);
    }
    if (map == NULL) {
	return FALSE;
    }
    {
	Header **mp;
	long i;

	/* Initialize the new extra header slots in the map to NULL and add
	 * them the free list. */
	for (i = oldSize, mp = map + oldSize; i < newSize; i++, mp++) {
	    Blt_Chain_Append(rcPtr->freeList, (ClientData)i); 
	    *mp = NULL;		       /* Initialize new slots in the map.  */
	}
    }
    rcPtr->map = map;
    rcPtr->numAllocated = newSize;
    return TRUE;
}

static int
GrowColumns(Table *tablePtr, long extra)
{
    if (extra > 0) { 
	long oldCols, newCols;
	Value **data, **vp, **vend;

	oldCols = NumColumnsAllocated(tablePtr);
	if (!GrowHeaders(&tablePtr->corePtr->columns, extra)) {
	    return FALSE;
	}
	newCols = NumColumnsAllocated(tablePtr);

	/* Resize the vector array to have as many slots as columns. */
	data = tablePtr->corePtr->data;
	if (data == NULL) {
	    data = Blt_Malloc(newCols * sizeof(Value *));
	} else {
	    data = Blt_Realloc(data, newCols * sizeof(Value *));
	}
	if (data == NULL) {
	    return FALSE;
	}
	/* Initialize the new vector slots to NULL. */
	for (vp = data + oldCols, vend = data + newCols; vp < vend; vp++) {
	    *vp = NULL;
	}
	tablePtr->corePtr->data = data;
    }
    return TRUE;
}

static int
GrowRows(Table *tablePtr, long extra)
{
    if (extra > 0) {
	long oldRows, newRows;
	Value **vpp, **vpend;

	oldRows = NumRowsAllocated(tablePtr);
	if (!GrowHeaders(&tablePtr->corePtr->rows, extra)) {
	    return FALSE;
	}
	newRows = NumRowsAllocated(tablePtr);

	/* Resize all the vectors. Leave the empty vectors alone.  They are
	 * allocated when data is added to them. */
	for (vpp = tablePtr->corePtr->data, 
		 vpend = vpp + NumColumnsAllocated(tablePtr); 
	     vpp < vpend; vpp++) {
	    if (*vpp != NULL) {
		Value *vector, *vp, *vend;
		
		vector = Blt_Realloc(*vpp, newRows * sizeof(Value));
		for (vp = vector + oldRows, vend = vector + newRows; 
		     vp < vend; vp++) {
		    vp->string = NULL;
		}
		*vpp = vector;
	    }
	}
    }
    return TRUE;
}

static void
ExtendHeaders(RowColumn *rcPtr, long n, Blt_Chain chain)
{
    Blt_ChainLink link;
    long nextIndex;
    long i;

    /* 
     * At this point we're guaranteed to have as many free rows/columns in
     * the table as requested.
     */
    link = Blt_Chain_FirstLink(rcPtr->freeList);
    nextIndex = rcPtr->numUsed; 
    for (i = 0; i < n; i++) {
	Blt_ChainLink next;
	Header *headerPtr;

	headerPtr = Blt_Pool_AllocItem(rcPtr->headerPool, 
		rcPtr->classPtr->headerSize);
	memset(headerPtr, 0, rcPtr->classPtr->headerSize);
	GetNextLabel(rcPtr, headerPtr);
	headerPtr->offset = (long)Blt_Chain_GetValue(link);
	rcPtr->map[nextIndex] = headerPtr;
	headerPtr->index = nextIndex;
	nextIndex++;

	/* Remove the link the freelist and append it to the output chain. */
	next = Blt_Chain_NextLink(link);
	Blt_Chain_UnlinkLink(rcPtr->freeList, link);
	Blt_Chain_AppendLink(chain, link);
	Blt_Chain_SetValue(link, headerPtr);
	link = next;
    }
    rcPtr->numUsed += n;
}

static int
ExtendRows(Table *tablePtr, long n, Blt_Chain chain)
{
    long numFree;

    numFree = Blt_Chain_GetLength(tablePtr->corePtr->rows.freeList);
    if (n > numFree) {
	long needed;

	needed = n - numFree;
	if (!GrowRows(tablePtr, needed)) {
	    return FALSE;
	}
    }
    ExtendHeaders(&tablePtr->corePtr->rows, n, chain);
    tablePtr->flags |= TABLE_KEYS_DIRTY;
    return TRUE;
}

static int
ExtendColumns(Table *tablePtr, long n, Blt_Chain chain)
{
    long numFree;

    numFree = Blt_Chain_GetLength(tablePtr->corePtr->columns.freeList);
    if (n > numFree) {
	if (!GrowColumns(tablePtr, n - numFree)) {
	    return FALSE;
	}
    }
    ExtendHeaders(&tablePtr->corePtr->columns, n, chain);
    return TRUE;
}

BLT_TABLE_COLUMN_TYPE
blt_table_name_to_column_type(const char *string)
{
    char c;

    c = string[0];
    if ((c == 's') && (strcmp(string, "string") == 0)) {
	return TABLE_COLUMN_TYPE_STRING;
    } else if ((c == 'i') && (strcmp(string, "integer") == 0)) {
	return TABLE_COLUMN_TYPE_INT;
    } else if ((c == 'd') && (strcmp(string, "double") == 0)) {
	return TABLE_COLUMN_TYPE_DOUBLE;
    } else if ((c == 'l') && (strcmp(string, "long") == 0)) {
	return TABLE_COLUMN_TYPE_LONG;
    } else if ((c == 't') && (strcmp(string, "time") == 0)) {
	return TABLE_COLUMN_TYPE_TIME;
    } else {
	return TABLE_COLUMN_TYPE_UNKNOWN;
    }
}

static INLINE int
IsEmpty(Value *valuePtr)
{
    return ((valuePtr == NULL) || (valuePtr->string == NULL));
}

static INLINE void
FreeValue(Value *valuePtr)
{
    if (valuePtr->string != NULL) {
	Blt_Free(valuePtr->string);
    }
    valuePtr->string = NULL;
}


static void
FreeVector(Value *vector, long length)
{
    if (vector != NULL) {
	Value *vp, *vend;

	for (vp = vector, vend = vp + length; vp < vend; vp++) {
	    FreeValue(vp);
	}
	Blt_Free(vector);
    }
}

static Value *
AllocateVector(Table *tablePtr, long offset)
{
    Value *vector;

    vector = tablePtr->corePtr->data[offset];
    if (vector == NULL) {

	vector = Blt_Calloc(NumRowsAllocated(tablePtr), sizeof(Value));
	if (vector == NULL) {
	    return NULL;
	}
	tablePtr->corePtr->data[offset] = vector;
    }
    return vector;
}

static Value *
GetValue(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    Value *vector;

    vector = tablePtr->corePtr->data[colPtr->offset];
    if (vector == NULL) {
	vector = AllocateVector(tablePtr, colPtr->offset);
    }
    return vector + rowPtr->offset;
}

static Tcl_Obj *
GetObjFromValue(Tcl_Interp *interp, BLT_TABLE_COLUMN_TYPE type, Value *valuePtr)
{
    Tcl_Obj *objPtr;

    if (IsEmpty(valuePtr)) {
	return NULL;
    } 
    objPtr = NULL;			/* Suppress compiler warning. */
    switch (type) {
    case TABLE_COLUMN_TYPE_UNKNOWN:
    case TABLE_COLUMN_TYPE_STRING:	/* string */
	objPtr = Tcl_NewStringObj(valuePtr->string, -1);
	break;
    case TABLE_COLUMN_TYPE_TIME:	/* time */
    case TABLE_COLUMN_TYPE_DOUBLE:	/* double */
	objPtr = Tcl_NewDoubleObj(valuePtr->datum.d);
	break;
    case TABLE_COLUMN_TYPE_LONG:	/* long */
	objPtr = Tcl_NewLongObj(valuePtr->datum.l);
	break;
    case TABLE_COLUMN_TYPE_INT:		/* int */
	objPtr = Tcl_NewIntObj((int)valuePtr->datum.l);
	break;
    }
    return objPtr;
}

static int
SetValueFromObj(Tcl_Interp *interp, BLT_TABLE_COLUMN_TYPE type, Tcl_Obj *objPtr,
		Value *valuePtr)
{
    int length;
    const char *s;

    FreeValue(valuePtr);
    if (objPtr == NULL) {
	return TCL_OK;
    }
    switch (type) {
    case TABLE_COLUMN_TYPE_TIME:	/* double */
	if (Blt_GetTimeFromObj(interp, objPtr, &valuePtr->datum.d) != TCL_OK) {
	    return TCL_ERROR;
	}
	break;

    case TABLE_COLUMN_TYPE_DOUBLE:	/* double */
	if (Blt_GetDoubleFromObj(interp, objPtr, &valuePtr->datum.d)!=TCL_OK) {
	    return TCL_ERROR;
	}
	break;
    case TABLE_COLUMN_TYPE_LONG:	/* long */
    case TABLE_COLUMN_TYPE_INT:		/* int */
	if (Blt_GetLongFromObj(interp, objPtr, &valuePtr->datum.l) != TCL_OK) {
	    return TCL_ERROR;
	}
	break;
    default:
	break;
    }
    s = Tcl_GetStringFromObj(objPtr, &length);
    valuePtr->string = Blt_AssertMalloc(length + 1);
    strcpy(valuePtr->string, s);
    return TCL_OK;
}


static int
SetValueFromString(Tcl_Interp *interp, BLT_TABLE_COLUMN_TYPE type, 
		   const char *s, int length, Value *valuePtr)
{
    double d;
    long l;
    char *string;

    if (length < 0) {
	length = strlen(s);
    }
    /* Make a copy of the string, eventually used for string rep.  */
    string = Blt_AssertMalloc(length + 1);
    strncpy(string, s, length);
    string[length] = '\0';

    switch (type) {
    case TABLE_COLUMN_TYPE_TIME:	/* time */
	if (Blt_GetTime(interp, string, &d) != TCL_OK) {
	    Blt_Free(string);
	    return TCL_ERROR;
	}
	valuePtr->datum.d = d;
	break;

    case TABLE_COLUMN_TYPE_DOUBLE:	/* double */
	if (Blt_GetDoubleFromString(interp, string, &d) != TCL_OK) {
	    Blt_Free(string);
	    return TCL_ERROR;
	}
	valuePtr->datum.d = d;
	break;
    case TABLE_COLUMN_TYPE_LONG:	/* long */
    case TABLE_COLUMN_TYPE_INT:		/* int */
	if (Blt_GetLong(interp, string, &l) != TCL_OK) {
	    Blt_Free(string);
	    return TCL_ERROR;
	}
	valuePtr->datum.l = l;
	break;
    default:
	break;
    }
    FreeValue(valuePtr);
    valuePtr->string = string;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewTableObject --
 *
 *	Creates and initializes a new table object. 
 *
 * Results:
 *	Returns a pointer to the new object is successful, NULL otherwise.  If
 *	a table object can't be generated, interp->result will contain an
 *	error message.
 *
 * -------------------------------------------------------------------------- 
 */
static TableObject *
NewTableObject(void)
{
    TableObject *corePtr;

    corePtr = Blt_Calloc(1, sizeof(TableObject));
    if (corePtr == NULL) {
	return NULL;
    }
    corePtr->clients = Blt_Chain_Create();

    Blt_InitHashTableWithPool(&corePtr->columns.labelTable, BLT_STRING_KEYS);
    Blt_InitHashTableWithPool(&corePtr->rows.labelTable, BLT_STRING_KEYS);
    corePtr->columns.classPtr = &columnClass;
    corePtr->columns.freeList = Blt_Chain_Create();
    corePtr->columns.headerPool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    corePtr->columns.nextId = 1;
    corePtr->rows.classPtr = &rowClass;
    corePtr->rows.freeList = Blt_Chain_Create();
    corePtr->rows.headerPool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    corePtr->rows.nextId = 1;
    return corePtr;
}

static void
DestroyTraces(Table *tablePtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&tablePtr->traces, &iter); hPtr != NULL; 
	 hPtr = Blt_NextHashEntry(&iter)) {
	Trace *tracePtr;

	tracePtr = Blt_GetHashValue(hPtr);
	blt_table_delete_trace(tablePtr, tracePtr);
    }
    Blt_Chain_Destroy(tablePtr->readTraces);
    Blt_Chain_Destroy(tablePtr->writeTraces);
    Blt_DeleteHashTable(&tablePtr->traces);
}

/*
 *---------------------------------------------------------------------------
 *
 * DoTrace --
 *
 *	Fires a trace set by a client of the table object.  Trace procedures
 *	should return a standard TCL result.
 *
 *	   TCL_OK	procedure executed successfully.
 *	   TCL_ERROR	procedure failed.
 *	   TCL_BREAK	don't execute any further trace procedures.
 *	   TCL_CONTINUE	treat like TCL_OK.
 *
 *	A trace procedure can in turn trigger more traces.  Traces are
 *	prohibited from recursively reentering their own trace procedures.  A
 *	hash table in the trace structure tracks the cells currently actively
 *	traced.  If a cell is already being traced, the trace procedure is not
 *	called and TCL_OK is blindly returned.
 *
 * Results:
 *	Returns the result of trace procedure.  If the trace is already
 *	active, then TCL_OK is returned.
 *
 * Side Effects:
 *	Traces on the table location may be fired.
 *
 *---------------------------------------------------------------------------
 */
static int
DoTrace(Trace *tracePtr, BLT_TABLE_TRACE_EVENT *eventPtr)
{
    int result;

    /* 
     * Check for individual traces on a cell.  Each trace has a hash table
     * that tracks what cells are actively being traced. This is to prevent
     * traces from triggering recursive callbacks.
     */
    Tcl_Preserve(tracePtr);
    tracePtr->flags |= TABLE_TRACE_ACTIVE;
    result = (*tracePtr->proc)(tracePtr->clientData, eventPtr);
    tracePtr->flags &= ~TABLE_TRACE_ACTIVE;
    Tcl_Release(tracePtr);

    if (result == TCL_ERROR) {
	Blt_Warn("error in trace callback: %s\n", 
		Tcl_GetStringResult(eventPtr->interp));
	Tcl_BackgroundError(eventPtr->interp);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TraceIdleProc --
 *
 *	Used to invoke a table trace handler routines at some idle point.  
 *	This routine is called from the TCL event loop.  Errors generated 
 *	by the event handler routines are backgrounded.
 *	
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
TraceIdleProc(ClientData clientData)
{
    Trace *tracePtr = clientData;

    tracePtr->flags &= ~TABLE_TRACE_PENDING;

    /* Protect the notifier in case it's deleted by the callback. */
    Tcl_Preserve(tracePtr);
    DoTrace(tracePtr, &tracePtr->event);
    Tcl_Release(tracePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyIdleProc --
 *
 *	Used to invoke event handler routines at some idle point.  This
 *	routine is called from the TCL event loop.  Errors generated by the
 *	event handler routines are backgrounded.
 *	
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
NotifyIdleProc(ClientData clientData)
{
    Notifier *notifierPtr = clientData;
    int result;

    notifierPtr->flags &= ~TABLE_NOTIFY_PENDING;

    /* Protect the notifier in case it's deleted by the callback. */
    Tcl_Preserve(notifierPtr);
    notifierPtr->flags |= TABLE_NOTIFY_ACTIVE;
    result = (*notifierPtr->proc)(notifierPtr->clientData, &notifierPtr->event);
    notifierPtr->flags &= ~TABLE_NOTIFY_ACTIVE;
    if (result == TCL_ERROR) {
	Tcl_BackgroundError(notifierPtr->interp);
    }
    Tcl_Release(notifierPtr);
}

static void
FreeNotifier(Notifier *notifierPtr) 
{
    if (notifierPtr->tag != NULL) {
	Blt_Free(notifierPtr->tag);
    }
    if (notifierPtr->link != NULL){
	Blt_Chain_DeleteLink(notifierPtr->chain, notifierPtr->link);
    }
    Blt_Free(notifierPtr);
}

static void
DestroyNotifiers(Table *tablePtr, Blt_Chain chain)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	Notifier *notifierPtr;

	notifierPtr = Blt_Chain_GetValue(link);
	notifierPtr->link = NULL;
	blt_table_delete_notifier(tablePtr, notifierPtr);
    }
    Blt_Chain_Destroy(chain);
}


/*
 *---------------------------------------------------------------------------
 *
 * DumpTags --
 *
 *	Retrieves all tags for a given row or column into a tcl list.  
 *
 * Results:
 *	Returns the number of tags in the list.
 *
 *---------------------------------------------------------------------------
 */
static void
DumpTags(Blt_HashTable *tagTablePtr, Header *headerPtr, Blt_Chain chain)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    for (hPtr = Blt_FirstHashEntry(tagTablePtr, &cursor); hPtr != NULL; 
	 hPtr = Blt_NextHashEntry(&cursor)) {
	Blt_HashEntry *hPtr2;
	Blt_HashTable *tablePtr;

	tablePtr = Blt_GetHashValue(hPtr);
	hPtr2 = Blt_FindHashEntry(tablePtr, (char *)headerPtr);
	if (hPtr2 != NULL) {
	    Blt_Chain_Append(chain, Blt_GetHashKey(tagTablePtr, hPtr));
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ClearTags --
 *
 *	Removes all tags for a given row or column.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *      All tags associcated with the row are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
ClearTags(Blt_HashTable *tagTablePtr, Header *headerPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    for (hPtr = Blt_FirstHashEntry(tagTablePtr, &cursor); hPtr != NULL; 
	 hPtr = Blt_NextHashEntry(&cursor)) {
	Blt_HashEntry *hPtr2;
	Blt_HashTable *tablePtr;

	tablePtr = Blt_GetHashValue(hPtr);
	hPtr2 = Blt_FindHashEntry(tablePtr, (char *)headerPtr);
	if (hPtr2 != NULL) {
	    Blt_DeleteHashEntry(tablePtr, hPtr2);
	}
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * DestroyTableObject --
 *
 *	Destroys the table object.  This is the final clean up of the object.
 *	The object's entry is removed from the hash table of tables.
 *
 * Results: 
 *	None.
 *
 * -------------------------------------------------------------------------- 
 */
static void
DestroyTableObject(TableObject *corePtr)
{
    corePtr->flags |= TABLE_DESTROYED;

    assert(Blt_Chain_GetLength(corePtr->clients) == 0);
    Blt_Chain_Destroy(corePtr->clients);

    /* Free the headers containing row and column info. */
    /* Free the data in each row. */
    if (corePtr->data != NULL) {
	Value **vp, **vend;
	long i;

	for (i = 0, vp = corePtr->data, vend = vp+corePtr->columns.numAllocated;
	     vp < vend; vp++, i++) {
	    if (*vp != NULL) {
		FreeVector(*vp, corePtr->rows.numAllocated);
	    }
	}
	Blt_Free(corePtr->data);
    }
    FreeRowColumn(&corePtr->rows);
    FreeRowColumn(&corePtr->columns);
    Blt_Free(corePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TableInterpDeleteProc --
 *
 *	This is called when the interpreter hosting the table object is
 *	deleted from the interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Destroys all remaining tables and removes the hash table used to
 *	register table names.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
TableInterpDeleteProc(ClientData clientData, Tcl_Interp *interp)
{
    InterpData *dataPtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    
    dataPtr = clientData;
    for (hPtr = Blt_FirstHashEntry(&dataPtr->clientTable, &cursor);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Blt_Chain chain;
	Blt_ChainLink link, next;

	chain = Blt_GetHashValue(hPtr);
	for (link = Blt_Chain_FirstLink(chain); link != NULL; link = next) {
	    Table *tablePtr;

	    next = Blt_Chain_NextLink(link);
	    tablePtr = Blt_Chain_GetValue(link);
	    tablePtr->link2 = NULL;
	    /* Don't destroy the client here. Let the client code close the
	     * table. */
	}
	Blt_Chain_Destroy(chain);
    }
    Blt_DeleteHashTable(&dataPtr->clientTable);
    Tcl_DeleteAssocData(interp, TABLE_THREAD_KEY);
    Blt_Free(dataPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetInterpData --
 *
 *	Creates or retrieves data associated with tuple data objects for a
 *	particular interpreter.  
 *
 * Results:
 *	Returns a pointer to the tuple interpreter data.
 *
 * -------------------------------------------------------------------------- 
 */
static InterpData *
GetInterpData(Tcl_Interp *interp)
{
    Tcl_InterpDeleteProc *proc;
    InterpData *dataPtr;

    dataPtr = (InterpData *)
	Tcl_GetAssocData(interp, TABLE_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
	dataPtr = Blt_AssertMalloc(sizeof(InterpData));
	dataPtr->interp = interp;
	Tcl_SetAssocData(interp, TABLE_THREAD_KEY, TableInterpDeleteProc, 
		dataPtr);
	Blt_InitHashTable(&dataPtr->clientTable, BLT_STRING_KEYS);
    }
    return dataPtr;
}


const char *
blt_table_column_type_to_name(BLT_TABLE_COLUMN_TYPE type)
{
    return valueTypes[type];
}



/*
 *---------------------------------------------------------------------------
 *
 * NewTags --
 *
 *---------------------------------------------------------------------------
 */
static BLT_TABLE_TAGS
NewTags(void)
{
    Tags *tagsPtr;

    tagsPtr = Blt_Malloc(sizeof(Tags));
    if (tagsPtr != NULL) {
	Blt_InitHashTable(&tagsPtr->rowTable, BLT_STRING_KEYS);
	Blt_InitHashTable(&tagsPtr->columnTable, BLT_STRING_KEYS);
	tagsPtr->refCount = 1;
    }
    return tagsPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindClientInNamespace --
 *
 *	Searches for a table client in a given namespace.
 *
 * Results:
 *	Returns a pointer to the table client if found, otherwise NULL.
 *
 *---------------------------------------------------------------------------
 */
static Table *
FindClientInNamespace(InterpData *dataPtr, Blt_ObjectName *namePtr)
{
    Blt_Chain chain;
    Blt_ChainLink link;
    Blt_HashEntry *hPtr;
    Tcl_DString ds;
    const char *qualName;

    qualName = Blt_MakeQualifiedName(namePtr, &ds);
    hPtr = Blt_FindHashEntry(&dataPtr->clientTable, qualName);
    Tcl_DStringFree(&ds);
    if (hPtr == NULL) {
	return NULL;
    }
    chain = Blt_GetHashValue(hPtr);
    link = Blt_Chain_FirstLink(chain);
    return Blt_Chain_GetValue(link);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTable --
 *
 *	Searches for the table client associated by the name given.
 *
 * Results:
 *	Returns a pointer to the table client if found, otherwise NULL.
 *
 *---------------------------------------------------------------------------
 */
static BLT_TABLE
GetTable(InterpData *dataPtr, const char *name, unsigned int flags)
{
    Blt_ObjectName objName;
    BLT_TABLE table;
    Tcl_Interp *interp;

    table = NULL;
    interp = dataPtr->interp;
    if (!Blt_ParseObjectName(interp, name, &objName, BLT_NO_DEFAULT_NS)) {
	return NULL;
    }
    if (objName.nsPtr != NULL) { 
	table = FindClientInNamespace(dataPtr, &objName);
    } else { 
	if (flags & NS_SEARCH_CURRENT) {
	    /* Look first in the current namespace. */
	    objName.nsPtr = Tcl_GetCurrentNamespace(interp);
	    table = FindClientInNamespace(dataPtr, &objName);
	}
	if ((table == NULL) && (flags & NS_SEARCH_GLOBAL)) {
	    objName.nsPtr = Tcl_GetGlobalNamespace(interp);
	    table = FindClientInNamespace(dataPtr, &objName);
	}
    }
    return table;
}

static void
DestroyClient(Table *tablePtr)
{
    if (tablePtr->magic != TABLE_MAGIC) {
	Blt_Warn("invalid table object token 0x%lx\n", 
		 (unsigned long)tablePtr);
	return;
    }
    /* Remove any traces that were set by this client. */
    DestroyTraces(tablePtr);
    /* Also remove all event handlers created by this client. */
    DestroyNotifiers(tablePtr, tablePtr->rowNotifiers);
    DestroyNotifiers(tablePtr, tablePtr->columnNotifiers);
    blt_table_unset_keys(tablePtr);
    if (tablePtr->tags != NULL) {
	blt_table_release_tags(tablePtr);
    }
    if ((tablePtr->corePtr != NULL) && (tablePtr->link != NULL)) {
	TableObject *corePtr;

	corePtr = tablePtr->corePtr;
	/* Remove the client from the server's list */
	Blt_Chain_DeleteLink(corePtr->clients, tablePtr->link);
	if (Blt_Chain_GetLength(corePtr->clients) == 0) {
	    DestroyTableObject(corePtr);
	}
    }
    tablePtr->magic = 0;
    Blt_Free(tablePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewTable --
 *
 *	Creates a new table client.  Clients shared a tuple data object.  They
 *	individually manage traces and events on tuple objects.  Returns a
 *	pointer to the malloc'ed structure.  This is passed to the client as a
 *	tuple token.
 *	
 * Results:
 *	A pointer to a Table is returned.  If one can't be allocated, NULL
 *	is returned.
 *
 *---------------------------------------------------------------------------
 */
static Table *
NewTable(
    InterpData *dataPtr, 
    TableObject *corePtr,	        /* Table object serving this
					 * client. */
    const char *qualName)		/* Full namespace qualified name of
					 * table. A table by this name may
					 * already exist. */
{
    Blt_Chain chain;			/* List of clients using the same
					 * name. */
    Table *tablePtr;
    int isNew;

    tablePtr = Blt_Calloc(1, sizeof(Table));
    if (tablePtr == NULL) {
	return NULL;
    }
    tablePtr->magic = TABLE_MAGIC;
    tablePtr->interp = dataPtr->interp;
    /* Add client to table object's list of clients. */
    tablePtr->link = Blt_Chain_Append(corePtr->clients, tablePtr);

    /* By default, use own sets of tags. */
    tablePtr->tags = NewTags();
    tablePtr->rowTags = &tablePtr->tags->rowTable;
    tablePtr->columnTags = &tablePtr->tags->columnTable;

    tablePtr->clientTablePtr = &dataPtr->clientTable;
    /* Table names are not unique.  More than one client may open the same
     * table.  The name remains in use so long as one client is still using
     * the table. This is so other clients can refer to the table, even though
     * the original client that created the data no longer exists. */
    tablePtr->hPtr = Blt_CreateHashEntry(&dataPtr->clientTable, qualName, 
	&isNew);
    if (isNew) {
	chain = Blt_Chain_Create();
	Blt_SetHashValue(tablePtr->hPtr, chain);
    } else {
	chain = Blt_GetHashValue(tablePtr->hPtr);
    }
    tablePtr->name = Blt_GetHashKey(&dataPtr->clientTable, tablePtr->hPtr);
    tablePtr->link2 = Blt_Chain_Append(chain, tablePtr);
    tablePtr->rowNotifiers = Blt_Chain_Create();
    tablePtr->columnNotifiers = Blt_Chain_Create();
    tablePtr->readTraces = Blt_Chain_Create();
    tablePtr->writeTraces = Blt_Chain_Create();
    Blt_InitHashTable(&tablePtr->notifiers, BLT_ONE_WORD_KEYS);
    Blt_InitHashTable(&tablePtr->traces, BLT_ONE_WORD_KEYS);

    tablePtr->corePtr = corePtr;
    return tablePtr;
}

static Blt_HashTable *
GetLabelTable(RowColumn *rcPtr, const char *label)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&rcPtr->labelTable, label);
    if (hPtr != NULL) {
	return Blt_GetHashValue(hPtr);
    }
    return NULL;
}

static Header *
FindLabel(RowColumn *rcPtr, const char *label)
{
    Blt_HashTable *tablePtr;

    tablePtr = GetLabelTable(rcPtr, label);
    if (tablePtr != NULL) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	hPtr = Blt_FirstHashEntry(tablePtr, &iter);
	if (hPtr != NULL) {
	    return Blt_GetHashValue(hPtr);
	}
    }
    return NULL;
}

static int
SetType(Table *tablePtr, struct _BLT_TABLE_COLUMN *colPtr, 
	BLT_TABLE_COLUMN_TYPE type)
{
    int i;

    if (type == colPtr->type) {
	return TCL_OK;			/* Already the requested type. */
    }
    /* Test first that every value in the column can be converted. */
    for (i = 0; i < blt_table_num_rows(tablePtr); i++) {
	Row *rowPtr;
	Value *valuePtr;

	rowPtr = blt_table_row(tablePtr, i);
	valuePtr = GetValue(tablePtr, rowPtr, colPtr);
	if (!IsEmpty(valuePtr)) {
	    Value value;

	    memset(&value, 0, sizeof(Value));
	    if (SetValueFromString(tablePtr->interp, type, valuePtr->string, -1,
		&value) != TCL_OK) {
		return TCL_ERROR;
	    }
	    FreeValue(&value);
	}
    }
    /* Now replace the column with the converted the values. */
    for (i = 0; i < blt_table_num_rows(tablePtr); i++) {
	Row *rowPtr;
	Value *valuePtr;

	rowPtr = blt_table_row(tablePtr, i);
	valuePtr = GetValue(tablePtr, rowPtr, colPtr);
	if (!IsEmpty(valuePtr)) {
	    if (SetValueFromString(tablePtr->interp, type, valuePtr->string, -1,
		valuePtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    colPtr->type = type;
    return TCL_OK;
}


static void
ResetMap(RowColumn *rcPtr)
{
    long i;

    /* Reset the reverse lookup: from header to index. */
    for (i = 0; i < rcPtr->numUsed; i++) {
	rcPtr->map[i]->index = i;
    }
}

static void
DeleteHeader(RowColumn *rcPtr, Header *headerPtr)
{
    /* If there is a label is associated with the column, free it. */
    if (headerPtr->label != NULL) {
	UnsetLabel(rcPtr, headerPtr);
    }
    { 
	long p, q;

	/* Compress the index-to-offset map. */
	for (p = headerPtr->index, q = p + 1; q < rcPtr->numUsed; p++, q++) {
	    /* Update the index as we slide down the headers in the map. */
	    rcPtr->map[p] = rcPtr->map[q];
	    rcPtr->map[p]->index = p;
	}
	rcPtr->map[p] = NULL;
    }
    /* Finally free the header. */
    Blt_Pool_FreeItem(rcPtr->headerPool, headerPtr);
    rcPtr->numUsed--;
}

/*
 *---------------------------------------------------------------------------
 *
 * ClearRowNotifiers --
 *
 *	Removes all event handlers set for the designated row.  Note that this
 *	doesn't remove handlers triggered by row or column tags.  Row and
 *	column event notifiers are stored in a chain.
 *
 *---------------------------------------------------------------------------
 */
static void
ClearRowNotifiers(Table *tablePtr, Row *rowPtr)
{
    Blt_ChainLink link, next;

    for (link = Blt_Chain_FirstLink(tablePtr->rowNotifiers); link != NULL;
	 link = next) {
	Notifier *notifierPtr;

	next = Blt_Chain_NextLink(link);
	notifierPtr = Blt_Chain_GetValue(link);
	if (notifierPtr->row == rowPtr) {
	    blt_table_delete_notifier(tablePtr, notifierPtr);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ClearColumnNotifiers --
 *
 *	Removes all event handlers set for the designated row.  Note that this
 *	doesn't remove handlers triggered by row or column tags.  Row and
 *	column event notifiers are stored in a chain.
 *
 *---------------------------------------------------------------------------
 */
static void
ClearColumnNotifiers(Table *tablePtr, Column *colPtr)
{
    Blt_ChainLink link, next;

    for (link = Blt_Chain_FirstLink(tablePtr->columnNotifiers); link != NULL; 
	link = next) {
	Notifier *notifierPtr;

	next = Blt_Chain_NextLink(link);
	notifierPtr = Blt_Chain_GetValue(link);
	if (notifierPtr->column == colPtr) {
	    blt_table_delete_notifier(tablePtr, notifierPtr);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DoNotify --
 *
 *	Traverses the client's list of event callbacks and checks if one
 *	matches the given event.  A client may trigger an action that causes
 *	the itself to be notified again.  This can be prevented by setting the
 *	TABLE_NOTIFY_FOREIGN_ONLY bit in the event handler.
 *
 *	If a matching handler is found, a callback may be called either
 *	immediately or at the next idle time depending upon the
 *	TABLE_NOTIFY_WHENIDLE bit.
 *
 *	Since a handler routine may trigger yet another call to itself,
 *	callbacks are ignored while the event handler is executing.
 *	
 *---------------------------------------------------------------------------
 */
static void
DoNotify(Table *tablePtr, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    Blt_ChainLink link;
    unsigned int eventMask;
    Blt_Chain chain;

    chain = (eventPtr->type & TABLE_NOTIFY_COLUMN) ?
	tablePtr->columnNotifiers : tablePtr->rowNotifiers;
    /* Check the client table for matching notifiers.  Issue callbacks
     * indicating that the structure of the table has changed.  */
    eventMask = eventPtr->type & TABLE_NOTIFY_MASK;
    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	Notifier *notifierPtr;
	int match;

	notifierPtr = Blt_Chain_GetValue(link);
	if ((notifierPtr->flags & eventMask) == 0) {
	    continue;			/* Event type doesn't match */
	}
	if ((eventPtr->self) && 
	    (notifierPtr->flags & TABLE_NOTIFY_FOREIGN_ONLY)) {
	    continue;			/* Don't notify yourself. */
	}
	if (notifierPtr->flags & TABLE_NOTIFY_ACTIVE) {
	    continue;			/* Ignore callbacks that are generated
					 * inside of a notify handler
					 * routine. */
	}
	match = FALSE;
	if (notifierPtr->tag != NULL) {
	    if (notifierPtr->flags & TABLE_NOTIFY_ROW) {
		if (blt_table_row_has_tag(tablePtr, eventPtr->row, 
			notifierPtr->tag)) {
		    match++;
		}
	    } else {
		if (blt_table_column_has_tag(tablePtr, eventPtr->column, 
			notifierPtr->tag)) {
		    match++;
		}
	    }
	} else if ((notifierPtr->flags & TABLE_NOTIFY_ROW) && 
		   ((notifierPtr->row == NULL) || 
		    (notifierPtr->row == eventPtr->row))) {
	    match++;			/* Offsets match. */
	} else if ((notifierPtr->flags & TABLE_NOTIFY_COLUMN) && 
		   ((notifierPtr->column == NULL) ||
		    (notifierPtr->column == eventPtr->column))) {
	    match++;			/* Offsets match. */
	}
	if (!match) {
	    continue;			/* Row or column doesn't match. */
	}
	if (notifierPtr->flags & TABLE_NOTIFY_WHENIDLE) {
	    if ((notifierPtr->flags & TABLE_NOTIFY_PENDING) == 0) {
		notifierPtr->flags |= TABLE_NOTIFY_PENDING;
		notifierPtr->event = *eventPtr;
		Tcl_DoWhenIdle(NotifyIdleProc, notifierPtr);
	    }
	} else {
	    notifierPtr->event = *eventPtr;
	    NotifyIdleProc(notifierPtr);
	}
    }
}


static void
InitNotifyEvent(Table *tablePtr, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    memset(eventPtr, 0, sizeof(BLT_TABLE_NOTIFY_EVENT));
    eventPtr->table = tablePtr;
    eventPtr->interp = tablePtr->interp;
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyClients --
 *
 *	Traverses the list of event callbacks and checks if one matches the
 *	given event.  A client may trigger an action that causes the table
 *	object to notify it.  This can be prevented by setting the
 *	TABLE_NOTIFY_FOREIGN_ONLY bit in the event handler.
 *
 *	If a matching handler is found, a callback may be called either
 *	immediately or at the next idle time depending upon the
 *	TABLE_NOTIFY_WHENIDLE bit.
 *
 *	Since a handler routine may trigger yet another call to itself,
 *	callbacks are ignored while the event handler is executing.
 *	
 *---------------------------------------------------------------------------
 */
static void
NotifyClients(Table *tablePtr, BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    Blt_ChainLink link, next;
    
    for (link = Blt_Chain_FirstLink(tablePtr->corePtr->clients); link != NULL; 
	 link = next) {
	Table *clientPtr;
	
	next = Blt_Chain_NextLink(link);
	clientPtr = Blt_Chain_GetValue(link);
	eventPtr->self = (clientPtr == tablePtr);
	DoNotify(clientPtr, eventPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyColumnChanged --
 *
 *	Traverses the list of event callbacks and checks if one matches the
 *	given event.  A client may trigger an action that causes the table
 *	object to notify it.  This can be prevented by setting the
 *	TABLE_NOTIFY_FOREIGN_ONLY bit in the event handler.
 *
 *	If a matching handler is found, a callback may be called either
 *	immediately or at the next idle time depending upon the
 *	TABLE_NOTIFY_WHENIDLE bit.
 *
 *	Since a handler routine may trigger yet another call to itself,
 *	callbacks are ignored while the event handler is executing.
 *	
 *---------------------------------------------------------------------------
 */
static void
NotifyColumnChanged(Table *tablePtr, Column *colPtr, unsigned int flags)
{
    BLT_TABLE_NOTIFY_EVENT event;

    InitNotifyEvent(tablePtr, &event);
    event.type = flags | TABLE_NOTIFY_COLUMN;
    if (colPtr == NULL) {		/* Indicates to trigger notifications
					 * for all columns. */
	for (colPtr = blt_table_first_column(tablePtr); colPtr != NULL;
	     colPtr = blt_table_next_column(tablePtr, colPtr)) {
	    event.column = colPtr;
	    NotifyClients(tablePtr, &event);
	} 
    } else {
	event.column = colPtr;
	NotifyClients(tablePtr, &event);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyRowChanged --
 *
 *	Traverses the list of event callbacks and checks if one matches the
 *	given event.  A client may trigger an action that causes the table
 *	object to notify it.  This can be prevented by setting the
 *	TABLE_NOTIFY_FOREIGN_ONLY bit in the event handler.
 *
 *	If a matching handler is found, a callback may be called either
 *	immediately or at the next idle time depending upon the
 *	TABLE_NOTIFY_WHENIDLE bit.
 *
 *	Since a handler routine may trigger yet another call to itself,
 *	callbacks are ignored while the event handler is executing.
 *	
 *---------------------------------------------------------------------------
 */
static void
NotifyRowChanged(Table *tablePtr, Row *rowPtr, unsigned int flags)
{
    BLT_TABLE_NOTIFY_EVENT event;

    InitNotifyEvent(tablePtr, &event);
    event.type = flags | TABLE_NOTIFY_ROW;
    if (rowPtr == TABLE_NOTIFY_ALL) {	
	/* Trigger notifications for all rows. */
	for (rowPtr = blt_table_first_row(tablePtr); rowPtr != NULL;
	     rowPtr = blt_table_next_row(tablePtr, rowPtr)) {
	    event.row = rowPtr;
	    NotifyClients(tablePtr, &event);
	} 
    } else {
	event.row = rowPtr;
	NotifyClients(tablePtr, &event);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_clear_row_traces --
 *
 *	Removes all traces set for this row.  Note that this doesn't remove
 *	traces set for specific cells (row,column).  Row traces are stored in
 *	a chain, which in turn is held in a hash table, keyed by the row.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_clear_row_traces(Table *tablePtr, Row *rowPtr)
{
    Blt_ChainLink link, next;

    for (link = Blt_Chain_FirstLink(tablePtr->readTraces); link != NULL; 
	 link = next) {
	Trace *tracePtr;

	next = Blt_Chain_NextLink(link);
	tracePtr = Blt_Chain_GetValue(link);
	if (tracePtr->row == rowPtr) {
	    blt_table_delete_trace(tablePtr, tracePtr);
	}
    }
    for (link = Blt_Chain_FirstLink(tablePtr->writeTraces); link != NULL; 
	 link = next) {
	Trace *tracePtr;

	next = Blt_Chain_NextLink(link);
	tracePtr = Blt_Chain_GetValue(link);
	if (tracePtr->row == rowPtr) {
	    blt_table_delete_trace(tablePtr, tracePtr);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_clear_column_traces --
 *
 *	Removes all traces set for this column.  Note that this doesn't remove
 *	traces set for specific cells (row,column).  Column traces are stored
 *	in a chain, which in turn is held in a hash table, keyed by the
 *	column.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_clear_column_traces(Table *tablePtr, BLT_TABLE_COLUMN column)
{
    Blt_ChainLink link, next;

    for (link = Blt_Chain_FirstLink(tablePtr->readTraces); link != NULL; 
	 link = next) {
	Trace *tracePtr;

	next = Blt_Chain_NextLink(link);
	tracePtr = Blt_Chain_GetValue(link);
	if (tracePtr->column == column) {
	    blt_table_delete_trace(tablePtr, tracePtr);
	}
    }
    for (link = Blt_Chain_FirstLink(tablePtr->writeTraces); link != NULL; 
	 link = next) {
	Trace *tracePtr;

	next = Blt_Chain_NextLink(link);
	tracePtr = Blt_Chain_GetValue(link);
	if (tracePtr->column == column) {
	    blt_table_delete_trace(tablePtr, tracePtr);
	}
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * CallClientTraces --
 *
 *	Examines the traces set for a specific client of the table object and
 *	fires any matching traces.
 *
 *	Traces match on row and column tag and indices and flags.
 *	Traces can match on
 *	     flag		type of trace (read, write, unset, create)
 *	     row index
 *	     column index
 *	     row tag
 *	     column tag
 *
 *	If the TABLE_TRACE_FOREIGN_ONLY is set in the handler, it means to
 *	ignore actions that are initiated by that client of the object.  Only
 *	actions by other clients are handled.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side Effects:
 *	Traces on the tuple table location may be fired.
 *
 *---------------------------------------------------------------------------
 */
static void
CallClientTraces(Table *tablePtr, Table *clientPtr, Row *rowPtr, Column *colPtr,
		 unsigned int flags)
{
    Blt_Chain chain;
    Blt_ChainLink link, next;
    BLT_TABLE_TRACE_EVENT event;

    /* Initialize trace event information. */
    event.table = clientPtr;
    event.row = rowPtr;
    event.column = colPtr;
    event.interp = clientPtr->interp;
    if (tablePtr == clientPtr) {
	flags |= TABLE_TRACE_SELF;
    }
    event.mask = flags;
    if (flags & TABLE_TRACE_READS) {
	chain = clientPtr->readTraces;
    } else {
	chain = clientPtr->writeTraces;
    }
    for (link = Blt_Chain_FirstLink(chain); link != NULL; link = next) {
	Trace *tracePtr;
	int match;

	next = Blt_Chain_NextLink(link);
	tracePtr = Blt_Chain_GetValue(link);
	if ((tracePtr->flags & flags) == 0) {
	    continue;		/* Doesn't match trace flags. */
	}
	if (tracePtr->flags & TABLE_TRACE_ACTIVE) {
	    continue;		/* Ignore callbacks that were triggered from
				 * the active trace handler routine. */
	}
	match = FALSE;
	if (tracePtr->colTag != NULL) {
	    if (blt_table_column_has_tag(clientPtr, colPtr, 
			tracePtr->colTag)) {
		match++;
	    }
	} else if ((tracePtr->column == colPtr) || (tracePtr->column == NULL)) {
	    match++;
	}
	if (tracePtr->rowTag != NULL) {
	    if (blt_table_row_has_tag(clientPtr, rowPtr, tracePtr->rowTag)) {
		match++;
	    }
	} else if ((tracePtr->row == rowPtr) || (tracePtr->row == NULL)) {
	    match++;
	}
	if (!match) {
	    continue;
	}
	if (tracePtr->flags & TABLE_TRACE_WHENIDLE) {
	    if ((tracePtr->flags & TABLE_TRACE_PENDING) == 0) {
		tracePtr->flags |= TABLE_TRACE_PENDING;
		tracePtr->event = event;
		Tcl_DoWhenIdle(TraceIdleProc, tracePtr);
	    }
	} else {
	    if (DoTrace(tracePtr, &event) == TCL_BREAK) {
		return;		/* Don't complete traces on break. */
	    }
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CallTraces --
 *
 *	Examines the traces set for each client of the table object and fires
 *	any matching traces.
 *
 *	Traces match on row and column indices and flags.
 *	The order is 
 *	  1. column traces.
 *	  2. row traces.
 *	  3. cell (row,column) traces.
 *
 *	If no matching criteria is specified (no tag, key, or tuple address)
 *	then only the bit flag has to match.
 *
 *	If the TABLE_TRACE_FOREIGN_ONLY is set in the handler, it means to
 *	ignore actions that are initiated by that client of the object.  Only
 *	actions by other clients are handled.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side Effects:
 *	Traces on the tuple table location may be fired.
 *
 *---------------------------------------------------------------------------
 */
static void
CallTraces(Table *tablePtr, Row *rowPtr, Column *colPtr, 
		 unsigned int flags)
{
    Blt_ChainLink link, next;

    for (link = Blt_Chain_FirstLink(tablePtr->corePtr->clients); link != NULL; 
	 link = next) {
	Table *clientPtr;

	next = Blt_Chain_NextLink(link);
	clientPtr = Blt_Chain_GetValue(link);
	CallClientTraces(tablePtr, clientPtr, rowPtr, colPtr, flags);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * UnsetValue --
 *
 *	Removes the value from the selected row, column location in the table.
 *	The row, column location must be within the actual table limits, but
 *	it's okay if there isn't a value there to remove.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The objPtr representing the value is released.
 *
 *---------------------------------------------------------------------------
 */
static void
UnsetValue(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    Value *valuePtr;

    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    if (!IsEmpty(valuePtr)) {
	/* Indicate the keytables need to be regenerated. */
	if (colPtr->flags & TABLE_COLUMN_PRIMARY_KEY) {
	    tablePtr->flags |= TABLE_KEYS_DIRTY;
	}
    }
    FreeValue(valuePtr);
}

static void
UnsetRowValues(Table *tablePtr, Row *rowPtr)
{
    Column *colPtr;

    for (colPtr = blt_table_first_column(tablePtr); colPtr != NULL;
	 colPtr = blt_table_next_column(tablePtr, colPtr)) {
	UnsetValue(tablePtr, rowPtr, colPtr);
    }
}

static void
UnsetColumnValues(Table *tablePtr, Column *colPtr)
{
    Value *vector;
    Row *rowPtr;

    for (rowPtr = blt_table_first_row(tablePtr); rowPtr != NULL;
	 rowPtr = blt_table_next_row(tablePtr, rowPtr)) {
	UnsetValue(tablePtr, rowPtr, colPtr);
    }
    vector = tablePtr->corePtr->data[colPtr->offset];
    if (vector != NULL) {
	FreeVector(vector, tablePtr->corePtr->rows.numAllocated);
	tablePtr->corePtr->data[colPtr->offset] = NULL;
    }
}

typedef struct {
    BLT_TABLE table;
    BLT_TABLE_SORT_ORDER *order;
    Blt_HashTable freqTable;
    long numColumns;
    unsigned int flags;
} TableSortData;


static TableSortData sortData;


static int
CompareDictionaryRows(ClientData clientData, Column *colPtr, Row *rowPtr1, 
		      Row *rowPtr2)
{
    Value *valuePtr1, *valuePtr2, *vector;
    Table *tablePtr;

    tablePtr = sortData.table;
    valuePtr1 = valuePtr2 = NULL;
    vector = tablePtr->corePtr->data[colPtr->offset];
    if (vector != NULL) {
	valuePtr1 = vector + rowPtr1->offset;
	if (IsEmpty(valuePtr1)) {
	    valuePtr1 = NULL;
	}
	valuePtr2 = vector + rowPtr2->offset;
	if (IsEmpty(valuePtr2)) {
	    valuePtr2 = NULL;
	}
    }
    if (IsEmpty(valuePtr1)) {
	if (IsEmpty(valuePtr2)) {
	    return 0;
	}
	return 1;
    } else if (IsEmpty(valuePtr2)) {
	return -1;
    }
    return Blt_DictionaryCompare(valuePtr2->string, valuePtr2->string);
}

static int
CompareFrequencyRows(ClientData clientData, Column *colPtr, Row *rowPtr1, 
		     Row *rowPtr2)
{
    Value *valuePtr1, *valuePtr2, *vector;
    Blt_HashEntry *hPtr1, *hPtr2;
    long f1, f2;
    Table *tablePtr;

    tablePtr = sortData.table;
    valuePtr1 = valuePtr2 = NULL;
    vector = tablePtr->corePtr->data[colPtr->offset];
    if (vector != NULL) {
	valuePtr1 = vector + rowPtr1->offset;
	if (IsEmpty(valuePtr1)) {
	    valuePtr1 = NULL;
	}
	valuePtr2 = vector + rowPtr2->offset;
	if (IsEmpty(valuePtr2)) {
	    valuePtr2 = NULL;
	}
    }
    if (IsEmpty(valuePtr1)) {
	if (IsEmpty(valuePtr2)) {
	    return 0;
	}
	return 1;
    } else if (IsEmpty(valuePtr2)) {
	return -1;
    }
    hPtr1 = Blt_FindHashEntry(&sortData.freqTable, valuePtr1->string);
    hPtr2 = Blt_FindHashEntry(&sortData.freqTable, valuePtr2->string);
    f1 = (long)Blt_GetHashValue(hPtr1);
    f2 = (long)Blt_GetHashValue(hPtr2);
    return f1 - f2;
}

static int
CompareAsciiRows(ClientData clientData, Column *colPtr, Row *rowPtr1, 
		 Row *rowPtr2)
{
    Value *valuePtr1, *valuePtr2, *vector;
    Table *tablePtr;

    tablePtr = sortData.table;
    valuePtr1 = valuePtr2 = NULL;
    vector = tablePtr->corePtr->data[colPtr->offset];
    if (vector != NULL) {
	valuePtr1 = vector + rowPtr1->offset;
	if (IsEmpty(valuePtr1)) {
	    valuePtr1 = NULL;
	}
	valuePtr2 = vector + rowPtr2->offset;
	if (IsEmpty(valuePtr2)) {
	    valuePtr2 = NULL;
	}
    }
    if (IsEmpty(valuePtr1)) {
	if (IsEmpty(valuePtr2)) {
	    return 0;
	}
	return 1;
    } else if (IsEmpty(valuePtr2)) {
	return -1;
    }
    return strcmp(valuePtr1->string, valuePtr2->string);
}

static int
CompareIntegerRows(ClientData clientData, Column *colPtr, Row *rowPtr1, 
		   Row *rowPtr2)
{
    Value *valuePtr1, *valuePtr2, *vector;
    Table *tablePtr;

    tablePtr = sortData.table;
    valuePtr1 = valuePtr2 = NULL;
    vector = tablePtr->corePtr->data[colPtr->offset];
    if (vector != NULL) {
	valuePtr1 = vector + rowPtr1->offset;
	if (IsEmpty(valuePtr1)) {
	    valuePtr1 = NULL;
	}
	valuePtr2 = vector + rowPtr2->offset;
	if (IsEmpty(valuePtr2)) {
	    valuePtr2 = NULL;
	}
    }
    if (IsEmpty(valuePtr1)) {
	if (IsEmpty(valuePtr2)) {
	    return 0;
	}
	return 1;
    } else if (IsEmpty(valuePtr2)) {
	return -1;
    }
    return valuePtr1->datum.l - valuePtr2->datum.l;
}

static int
CompareDoubleRows(ClientData clientData, Column *colPtr, Row *rowPtr1, 
		  Row *rowPtr2)
{
    Value *valuePtr1, *valuePtr2, *vector;
    Table *tablePtr;

    tablePtr = sortData.table;
    valuePtr1 = valuePtr2 = NULL;
    vector = tablePtr->corePtr->data[colPtr->offset];
    if (vector != NULL) {
	valuePtr1 = vector + rowPtr1->offset;
	if (IsEmpty(valuePtr1)) {
	    valuePtr1 = NULL;
	}
	valuePtr2 = vector + rowPtr2->offset;
	if (IsEmpty(valuePtr2)) {
	    valuePtr2 = NULL;
	}
    }
    if (IsEmpty(valuePtr1)) {
	if (IsEmpty(valuePtr2)) {
	    return 0;
	}
	return 1;
    } else if (IsEmpty(valuePtr2)) {
	return -1;
    }
    if (valuePtr1->datum.d < valuePtr2->datum.d) {
	return -1;
    } else if (valuePtr1->datum.d > valuePtr2->datum.d) {
	return 1;
    }
    return 0;
}

static int
CompareRows(void *a, void *b)
{
    Row *rowPtr1, *rowPtr2;
    long i, result;

    rowPtr1 = *(Row **)a;
    rowPtr2 = *(Row **)b;
    for (i = 0; i < sortData.numColumns; i++) {
	BLT_TABLE_SORT_ORDER *sp;

	sp = sortData.order + i;
	result = (*sp->cmpProc)(sp->clientData, sp->column, rowPtr1, rowPtr2);
	if (result != 0) {
	    return (sortData.flags & TABLE_SORT_DECREASING) ? -result : result;
	}
    }
    result = rowPtr1->index - rowPtr2->index;
    return (sortData.flags & TABLE_SORT_DECREASING) ? -result : result;
}


BLT_TABLE_COMPARE_PROC *
blt_table_get_compare_proc(Table *tablePtr, Column *colPtr, unsigned int flags)
{
    BLT_TABLE_COMPARE_PROC *proc;

    if ((flags & TABLE_SORT_TYPE_MASK) == TABLE_SORT_NONE) {
	switch (colPtr->type) {
	case TABLE_COLUMN_TYPE_INT:
	case TABLE_COLUMN_TYPE_LONG:
	    proc = CompareIntegerRows;
	    break;
	case TABLE_COLUMN_TYPE_TIME:
	case TABLE_COLUMN_TYPE_DOUBLE:
	    proc = CompareDoubleRows;
	    break;
	case TABLE_COLUMN_TYPE_STRING:
	case TABLE_COLUMN_TYPE_UNKNOWN:
	default:
	    proc = CompareAsciiRows;
	    break;
	}
    } else {
	switch (flags & TABLE_SORT_TYPE_MASK) {
	case TABLE_SORT_DICTIONARY:
	    proc = CompareDictionaryRows;
	    break;
	case TABLE_SORT_FREQUENCY:
	    proc = CompareFrequencyRows;
	    break;
	default:
	case TABLE_SORT_ASCII:
	    proc = CompareAsciiRows;
	    break;
	}
    }
    return proc;
}

static Header **
SortHeaders(RowColumn *rcPtr, QSortCompareProc *proc)
{
    long i;
    Header **map;

    /* Make a copy of the current row map. */
    map = Blt_Malloc(sizeof(Header *) * rcPtr->numAllocated);
    if (map == NULL) {
	return NULL;
    }
    for (i = 0; i < rcPtr->numAllocated; i++) {
	map[i] = rcPtr->map[i];
    }
    /* Sort the map and return it. */
    qsort((char *)map, rcPtr->numUsed, sizeof(Header *), proc);
    return map;
}

static void
ReplaceMap(RowColumn *rcPtr, Header **map)
{
    Blt_Free(rcPtr->map);
    rcPtr->map = map;
    ResetMap(rcPtr);
}

static int
MoveIndices(
    RowColumn *rcPtr,
    Header *srcPtr,		/* Starting source index.  */
    Header *destPtr,		/* Starting destination index. */
    long count)			/* # of rows or columns to move. */
{
    Header **newMap;		/* Resulting reordered map. */
    long src, dest;

#ifdef notdef
    fprintf(stderr, "src=%ld, dest=%ld, count=%d\n", srcPtr->index, 
	destPtr->index, count);
    fprintf(stderr, "%s numUsed=%d, numAllocated=%d\n", rcPtr->classPtr->name,
	    rcPtr->numUsed, rcPtr->numAllocated);
#endif
    if (srcPtr == destPtr) {
	return TRUE;
    }
    src = srcPtr->index, dest = destPtr->index;
    newMap = Blt_Malloc(sizeof(Header *) * rcPtr->numAllocated);
    if (newMap == NULL) {
	return FALSE;
    }
    if (dest < src) {
	long i, j;

	/*
	 *     dest   src
	 *      v     v
	 * | | | | | |x|x|x|x| |
	 *  A A B B B C C C C D
	 * | | |x|x|x|x| | | | |
	 *
	 * Section C is the selected region to move.
	 */
	/* Section A: copy everything from 0 to "dest" */
	for (i = 0; i < dest; i++) {
	    newMap[i] = rcPtr->map[i];
	}
	/* Section C: append the selected region. */
	for (i = src, j = dest; i < (src + count); i++, j++) {
	    newMap[j] = rcPtr->map[i];
	}
	/* Section B: shift the preceding indices from "dest" to "src".  */
	for (i = dest; i < src; i++, j++) {
	    newMap[j] = rcPtr->map[i];
	}
	/* Section D: append trailing indices until the end. */
	for (i = src + count; i < rcPtr->numUsed; i++, j++) {
	    newMap[j] = rcPtr->map[i];
	}
    } else if (src < dest) {
	long i, j;

	/*
	 *     src        dest
	 *      v           v
	 * | | |x|x|x|x| | | | |
	 *  A A C C C C B B B D
	 * | | | | | |x|x|x|x| |
	 *
	 * Section C is the selected region to move.
	 */
	/* Section A: copy everything from 0 to "src" */
	for (j = 0; j < src; j++) {
	    newMap[j] = rcPtr->map[j];
	}
	/* Section B: shift the trailing indices from "src" to "dest".  */
	for (i = (src + count); j < dest; i++, j++) {
	    newMap[j] = rcPtr->map[i];
	}
	/* Section C: append the selected region. */
	for (i = src; i < (src + count); i++, j++) {
	    newMap[j] = rcPtr->map[i];
	}
	/* Section D: append trailing indices until the end. */
	for (i = dest + count; i < rcPtr->numUsed; i++, j++) {
	    newMap[j] = rcPtr->map[i];
	}
    }
    /* Reset the inverse offset-to-index map. */
    ReplaceMap(rcPtr, newMap);
    return TRUE;
}


/*
 *---------------------------------------------------------------------------
 *
 * ParseDumpRecord --
 *
 *	Gets the next full record in the dump string, returning the
 *	record as a list. Blank lines and comments are ignored.
 *
 * Results: 
 *	TCL_RETURN	The end of the string is reached.
 *	TCL_ERROR	An error occurred and an error message 
 *			is left in the interpreter result.  
 *	TCL_OK		The next record has been successfully parsed.
 *
 *---------------------------------------------------------------------------
 */
static int
ParseDumpRecord(
    Tcl_Interp *interp,
    char **sp,				/* (in/out) points to current location
					 * in in dump string. Updated after
					 * parsing record. */
    RestoreData *restorePtr)
{
    char *entry, *eol;
    char saved;
    int result;

    restorePtr->argc = 0;
    entry = *sp;
    /* Get first line, ignoring blank lines and comments. */
    for (;;) {
	char *first;

	first = NULL;
	restorePtr->numLines++;
	/* Find the end of the first line. */
	for (eol = entry; (*eol != '\n') && (*eol != '\0'); eol++) {
	    if ((first == NULL) && (!isspace(UCHAR(*eol)))) {
		first = eol;	/* Track first non-whitespace
				 * character. */
	    }
	}
	if (first == NULL) {
	    if (*eol == '\0') {
		return TCL_RETURN;
	    }
	} else if (*first != '#') {
	    break;		/* Not a comment or blank line. */
	}
	entry = eol + 1;
    }
    saved = *eol;
    *eol = '\0';
    while (!Tcl_CommandComplete(entry)) {
	*eol = saved;
	if (*eol == '\0') {
	    Tcl_AppendResult(interp, "incomplete dump record: \"", entry, 
		"\"", (char *)NULL);
	    return TCL_ERROR;		/* Found EOF (incomplete
					 * entry) or error. */
	}
	/* Get the next line. */
	for (eol = eol + 1; (*eol != '\n') && (*eol != '\0'); eol++) {
	    /*empty*/
	}
	restorePtr->numLines++;
	saved = *eol;
	*eol = '\0';
    }
    if (entry == eol) {
	return TCL_RETURN;
    }
    result = Tcl_SplitList(interp, entry, &restorePtr->argc, &restorePtr->argv);
    *eol = saved;
    *sp = eol + 1;
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadDumpRecord --
 *
 *	Reads the next full record from the given channel, returning the
 *	record as a list. Blank lines and comments are ignored.
 *
 * Results: 
 *	TCL_RETURN	The end of the file has been reached.
 *	TCL_ERROR	A read error has occurred and an error message 
 *			is left in the interpreter result.  
 *	TCL_OK		The next record has been successfully parsed.
 *
 *---------------------------------------------------------------------------
 */
static int
ReadDumpRecord(Tcl_Interp *interp, Tcl_Channel channel, RestoreData *restorePtr)
{
    int result;
    Tcl_DString ds;

    Tcl_DStringInit(&ds);
    /* Get first line, ignoring blank lines and comments. */
    for (;;) {
	const char *cp;
	int numChars;

	Tcl_DStringSetLength(&ds, 0);
	numChars = Tcl_Gets(channel, &ds);
	if (numChars < 0) {
	    if (Tcl_Eof(channel)) {
		return TCL_RETURN;
	    }
	    return TCL_ERROR;
	}
	restorePtr->numLines++;
	for (cp = Tcl_DStringValue(&ds); *cp != '\0'; cp++) {
	    if (!isspace(UCHAR(*cp))) {
		break;
	    }
	}
	if ((*cp != '\0') && (*cp != '#')) {
	    break;		/* Not a comment or blank line. */
	}
    }

    Tcl_DStringAppend(&ds, "\n", 1);
    while (!Tcl_CommandComplete(Tcl_DStringValue(&ds))) {
	int numChars;

	/* Process additional lines if needed */
	numChars = Tcl_Gets(channel, &ds);
	if (numChars < 0) {
	    Tcl_AppendResult(interp, "error reading file: ", 
			     Tcl_PosixError(interp), (char *)NULL);
	    Tcl_DStringFree(&ds);
	    return TCL_ERROR;		/* Found EOF (incomplete
					 * entry) or error. */
	}
	restorePtr->numLines++;
	Tcl_DStringAppend(&ds, "\n", 1);
    }
    result = Tcl_SplitList(interp, Tcl_DStringValue(&ds), &restorePtr->argc, 
			   &restorePtr->argv);
    Tcl_DStringFree(&ds);
    return result;
}

static void
RestoreError(Tcl_Interp *interp, RestoreData *restorePtr)
{
    Tcl_DString ds;

    Tcl_DStringInit(&ds);
    Tcl_DStringGetResult(interp, &ds);
    Tcl_AppendResult(interp, restorePtr->fileName, ":", 
	Blt_Ltoa(restorePtr->numLines), ": error: ", Tcl_DStringValue(&ds), 
	(char *)NULL);
    Tcl_DStringFree(&ds);
}

static int
RestoreHeader(Tcl_Interp *interp, BLT_TABLE table, RestoreData *restorePtr)
{
    long numCols, numRows;
    long count, time;

    /* i rows columns ctime mtime */
    if (restorePtr->argc != 5) {
	RestoreError(interp, restorePtr);
	Tcl_AppendResult(interp, "wrong # of elements in restore header.", 
		(char *)NULL);
	return TCL_ERROR;
    }	
    if (Blt_GetLong(interp, restorePtr->argv[1], &count) != TCL_OK) {
	RestoreError(interp, restorePtr);
	return TCL_ERROR;
    }
    if (count < 1) {
	RestoreError(interp, restorePtr);
	Tcl_AppendResult(interp, "bad # of rows \"", restorePtr->argv[1], "\"", 
			 (char *)NULL);
	return TCL_ERROR;
    }
    numRows = count;
    if (Blt_GetLong(interp, restorePtr->argv[2], &count) != TCL_OK) {
	RestoreError(interp, restorePtr);
	return TCL_ERROR;
    }
    if (count < 1) {
	RestoreError(interp, restorePtr);
	Tcl_AppendResult(interp, "bad # of columns \"", restorePtr->argv[2], 
		"\"", (char *)NULL);
	return TCL_ERROR;
    }
    numCols = count;
    if ((restorePtr->flags & TABLE_RESTORE_OVERWRITE) == 0) {
	numRows += restorePtr->numRows;
	numCols += restorePtr->numCols;
    }
    if (numCols > blt_table_num_columns(table)) {
	long needed;

	needed = numCols - blt_table_num_columns(table);
	if (!GrowColumns(table, needed)) {
	    RestoreError(interp, restorePtr);
	    Tcl_AppendResult(interp, "can't allocate \"", Blt_Ltoa(needed),
			"\"", " extra columns.", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (numRows > blt_table_num_rows(table)) {
	long needed;

	needed = numRows - blt_table_num_rows(table);
	if (!GrowRows(table, needed)) {
	    RestoreError(interp, restorePtr);
	    Tcl_AppendResult(interp, "can't allocate \"", Blt_Ltoa(needed), 
		"\" extra rows.", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (Blt_GetLong(interp, restorePtr->argv[3], &time) != TCL_OK) {
	RestoreError(interp, restorePtr);
	return TCL_ERROR;
    }
    restorePtr->ctime = (unsigned long)time;
    if (Blt_GetLong(interp, restorePtr->argv[4], &time) != TCL_OK) {
	RestoreError(interp, restorePtr);
	return TCL_ERROR;
    }
    restorePtr->mtime = (unsigned long)time;
    return TCL_OK;
}

static int
RestoreColumn(Tcl_Interp *interp, BLT_TABLE table, RestoreData *restorePtr)
{
    long index;
    Column *colPtr;
    int type;
    const char *label;
    int isNew;
    Blt_HashEntry *hPtr;

    /* c index label type ?tagList? */
    if ((restorePtr->argc < 4) || (restorePtr->argc > 5)) {
	RestoreError(interp, restorePtr);
	Tcl_AppendResult(interp, "wrong # elements in restore column entry", 
		(char *)NULL);
	return TCL_ERROR;
    }	
    if (Blt_GetLong(interp, restorePtr->argv[1], &index) != TCL_OK) {
	RestoreError(interp, restorePtr);
	return TCL_ERROR;
    }
    if (index < 0) {
	RestoreError(interp, restorePtr);
	Tcl_AppendResult(interp, "bad column index \"", restorePtr->argv[1], 
		"\"", (char *)NULL);
	return TCL_ERROR;
    }
    label = restorePtr->argv[2];
    colPtr = blt_table_get_column_by_label(table, label);
    if ((colPtr == NULL) || 
	((restorePtr->flags & TABLE_RESTORE_OVERWRITE) == 0)) {
	colPtr = blt_table_create_column(interp, table, label);
	if (colPtr == NULL) {
	    RestoreError(interp, restorePtr);
	    Tcl_AppendResult(interp, "can't append column \"", label, "\"",
			     (char *)NULL);
	    return TCL_ERROR;
	}
    }
    hPtr = Blt_CreateHashEntry(&restorePtr->colIndices, (char *)index, &isNew);
    Blt_SetHashValue(hPtr, colPtr);

    type = blt_table_name_to_column_type(restorePtr->argv[3]);
    if (type == TABLE_COLUMN_TYPE_UNKNOWN) {
	RestoreError(interp, restorePtr);
	Tcl_AppendResult(interp, "bad column type \"", restorePtr->argv[3], 
			 "\"", (char *)NULL);
	return TCL_ERROR;
    }
    colPtr->type = type;
    if ((restorePtr->argc == 5) && 
	((restorePtr->flags & TABLE_RESTORE_NO_TAGS) == 0)) {
	int i, elc;
	const char **elv;

	if (Tcl_SplitList(interp, restorePtr->argv[4], &elc, &elv) != TCL_OK) {
	    RestoreError(interp, restorePtr);
	    return TCL_ERROR;
	}
	
	for (i = 0; i < elc; i++) {
	    if (blt_table_set_column_tag(interp, table, colPtr, elv[i]) 
		!= TCL_OK) {
		Blt_Free(elv);
		return TCL_ERROR;
	    }
	}
	Blt_Free(elv);
    }
    return TCL_OK;
}

static int
RestoreRow(Tcl_Interp *interp, BLT_TABLE table, RestoreData *restorePtr)
{
    BLT_TABLE_ROW row;
    Blt_HashEntry *hPtr;
    const char **elv;
    const char *label;
    int elc;
    int isNew;
    long index;

    /* r index label ?tagList? */
    if ((restorePtr->argc < 3) || (restorePtr->argc > 4)) {
	RestoreError(interp, restorePtr);
	Tcl_AppendResult(interp, "wrong # of elements in restore row entry", 
			 (char *)NULL);
	return TCL_ERROR;
    }	
    if (Blt_GetLong(interp, restorePtr->argv[1], &index) != TCL_OK) {
	RestoreError(interp, restorePtr);
	return TCL_ERROR;
    }
    if (index < 0) {
	RestoreError(interp, restorePtr);
	Tcl_AppendResult(interp, "bad row index \"", restorePtr->argv[1], "\"",
		(char *)NULL);
	return TCL_ERROR;
    }
    label = restorePtr->argv[2];
    row = blt_table_get_row_by_label(table, label);
    if ((row == NULL) || ((restorePtr->flags & TABLE_RESTORE_OVERWRITE) == 0)) {
	row = blt_table_create_row(interp, table, label);
	if (row == NULL) {
	    RestoreError(interp, restorePtr);
	    Tcl_AppendResult(interp, "can't append row \"", label, "\"",
		     (char *)NULL);
	    return TCL_ERROR;
	}
    }
    hPtr = Blt_CreateHashEntry(&restorePtr->rowIndices, (char *)index, &isNew);
    Blt_SetHashValue(hPtr, row);
    if ((restorePtr->argc == 5) && 
	((restorePtr->flags & TABLE_RESTORE_NO_TAGS) == 0)) {
	int i;

	if (Tcl_SplitList(interp, restorePtr->argv[3], &elc, &elv) != TCL_OK) {
	    RestoreError(interp, restorePtr);
	    return TCL_ERROR;
	}
	for (i = 0; i < elc; i++) {
	    if (blt_table_set_row_tag(interp, table, row, elv[i]) != TCL_OK) {
		Blt_Free(elv);
		return TCL_ERROR;
	    }
	}
	Blt_Free(elv);
    }
    return TCL_OK;
}

static int
RestoreValue(Tcl_Interp *interp, BLT_TABLE table, RestoreData *restorePtr)
{
    Blt_HashEntry *hPtr;
    int result;
    BLT_TABLE_ROW row;
    Column *colPtr;
    Value *valuePtr;
    long index;

    /* d row column value */
    if (restorePtr->argc != 4) {
	RestoreError(interp, restorePtr);
	Tcl_AppendResult(interp, "wrong # elements in restore data entry", 
		(char *)NULL);
	return TCL_ERROR;
    }	
    if (Blt_GetLong(interp, restorePtr->argv[1], &index) != TCL_OK) {
	RestoreError(interp, restorePtr);
	return TCL_ERROR;
    }
    hPtr = Blt_FindHashEntry(&restorePtr->rowIndices, (char *)index);
    if (hPtr == NULL) {
	RestoreError(interp, restorePtr);
	Tcl_AppendResult(interp, "bad row index \"", restorePtr->argv[1], "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    row = Blt_GetHashValue(hPtr);
    if (Blt_GetLong(interp, restorePtr->argv[2], &index) != TCL_OK) {
	RestoreError(interp, restorePtr);
	return TCL_ERROR;
    }
    hPtr = Blt_FindHashEntry(&restorePtr->colIndices, (char *)index);
    if (hPtr == NULL) {
	RestoreError(interp, restorePtr);
	Tcl_AppendResult(interp, "bad column index \"", restorePtr->argv[2], 
		"\"", (char *)NULL);
	return TCL_ERROR;
    }
    colPtr = Blt_GetHashValue(hPtr);

    valuePtr = GetValue(table, row, colPtr);
    result = SetValueFromString(interp, colPtr->type, restorePtr->argv[3], -1,
	valuePtr);
    if (result != TCL_OK) {
	RestoreError(interp, restorePtr);
    }
    return result;
}

BLT_TABLE_ROW *
blt_table_get_row_map(Table *tablePtr)  
{
    return (BLT_TABLE_ROW *)tablePtr->corePtr->rows.map;
}

BLT_TABLE_COLUMN *
blt_table_get_column_map(Table *tablePtr)  
{
    return (BLT_TABLE_COLUMN *)tablePtr->corePtr->columns.map;
}

Blt_HashEntry *
blt_table_first_row_tag(Table *tablePtr, Blt_HashSearch *cursorPtr)  
{
    return Blt_FirstHashEntry(tablePtr->rowTags, cursorPtr);
}

Blt_HashEntry *
blt_table_first_column_tag(Table *tablePtr, Blt_HashSearch *cursorPtr)  
{
    return Blt_FirstHashEntry(tablePtr->columnTags, cursorPtr);
}

int 
blt_table_same_object(Table *tablePtr1, Table *tablePtr2)  
{
    return tablePtr1->corePtr == tablePtr2->corePtr;
}

Blt_Chain
blt_table_row_tags(Table *tablePtr, Row *rowPtr)  
{
    Blt_Chain chain;

    chain = Blt_Chain_Create();
    DumpTags(tablePtr->rowTags, (Header *)rowPtr, chain);
    return chain;
}

BLT_TABLE_ROW
blt_table_get_row_by_index(Table *tablePtr, long index)  
{
    if ((index >= 0) && (index < blt_table_num_rows(tablePtr))) {
	return blt_table_row(tablePtr, index);
    }
    return NULL;
}

BLT_TABLE_COLUMN
blt_table_get_column_by_index(Table *tablePtr, long index)  
{
    if ((index >= 0) && (index < blt_table_num_columns(tablePtr))) {
	return blt_table_column(tablePtr, index);
    }
    return NULL;
}

Blt_Chain
blt_table_column_tags(Table *tablePtr, BLT_TABLE_COLUMN col)  
{
    Blt_Chain chain;

    chain = Blt_Chain_Create();
    DumpTags(tablePtr->columnTags, (Header *)col, chain);
    return chain;
}


BLT_TABLE_ROWCOLUMN_SPEC
blt_table_row_spec(BLT_TABLE table, Tcl_Obj *objPtr, const char **sp)
{
    const char *p;
    const char *string;
    long index;
    char c;

    string = Tcl_GetString(objPtr);
    *sp = string;
    c = string[0];
    if ((isdigit(UCHAR(c))) && 
	(Blt_GetLongFromObj((Tcl_Interp *)NULL, objPtr, &index) == TCL_OK)) {
	return TABLE_SPEC_INDEX;
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
	return TABLE_SPEC_TAG;
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
	return TABLE_SPEC_TAG;
    } else if ((c == 'r') && (strncmp(string, "range=", 6) == 0)) {
	*sp = string + 6;
	return TABLE_SPEC_RANGE;
    } else if ((c == 'i') && (strncmp(string, "index=", 6) == 0)) {
	*sp = string + 6;
	return TABLE_SPEC_INDEX;
    } else if ((c == 'l') && (strncmp(string, "label=", 6) == 0)) {
	*sp = string + 6;
	return TABLE_SPEC_LABEL;
    } else if ((c == 't') && (strncmp(string, "tag=", 4) == 0)) {
	*sp = string + 4;
	return TABLE_SPEC_TAG;
    } else if (blt_table_get_row_by_label(table, string) != NULL) {
	return TABLE_SPEC_LABEL;
    } else if (blt_table_get_row_tag_table(table, string) != NULL) {
	return TABLE_SPEC_TAG;
    }
    p = strchr(string, '-');
    if (p != NULL) {
	Tcl_Obj *rangeObjPtr;
	BLT_TABLE_ROW row;

	rangeObjPtr = Tcl_NewStringObj(string, p - string);
	row = blt_table_get_row((Tcl_Interp *)NULL, table, rangeObjPtr);
	Tcl_DecrRefCount(rangeObjPtr);
        if (row != NULL) {
	    rangeObjPtr = Tcl_NewStringObj(p + 1, -1);
	    row = blt_table_get_row((Tcl_Interp *)NULL, table, rangeObjPtr);
	    Tcl_DecrRefCount(rangeObjPtr);
	    if (row != NULL) {
		return TABLE_SPEC_RANGE;
	    }
	}
    } 
    return TABLE_SPEC_UNKNOWN;
}

BLT_TABLE_COLUMN
blt_table_first_column(Table *tablePtr)  
{
    if (tablePtr->corePtr->columns.numUsed > 0) {
	return (BLT_TABLE_COLUMN)tablePtr->corePtr->columns.map[0];
    }
    return NULL;
}

BLT_TABLE_COLUMN
blt_table_next_column(Table *tablePtr, Column *colPtr)  
{
    long index;

    index = colPtr->index + 1;
    if (index < tablePtr->corePtr->columns.numUsed) {
	return (BLT_TABLE_COLUMN)tablePtr->corePtr->columns.map[index];
    }
    return NULL;
}

BLT_TABLE_ROW
blt_table_first_row(Table *tablePtr)  
{
    if (tablePtr->corePtr->rows.numUsed > 0) {
	return (BLT_TABLE_ROW)tablePtr->corePtr->rows.map[0];
    }
    return NULL;
}

BLT_TABLE_ROW
blt_table_next_row(Table *tablePtr, Row *rowPtr)  
{
    long index;

    index = rowPtr->index + 1;
    if (index < tablePtr->corePtr->rows.numUsed) {
	return (BLT_TABLE_ROW)tablePtr->corePtr->rows.map[index];
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_iterate_row --
 *
 *	Returns the id of the first row derived from the given tag,
 *	label or index represented in objPtr.  
 *
 * Results:
 *	Returns the row location of the first item.  If no row 
 *	can be found, then -1 is returned and an error message is
 *	left in the interpreter.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_iterate_row(Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr, 
		      BLT_TABLE_ITERATOR *iterPtr)
{
    BLT_TABLE_ROW from, to;
    const char *tagName, *p;
    int result;
    Tcl_Obj *rangeObjPtr;
    long index;
    BLT_TABLE_ROWCOLUMN_SPEC spec;

    memset(iterPtr, 0, sizeof(BLT_TABLE_ITERATOR));
    iterPtr->table = table;
    iterPtr->type = TABLE_ITERATOR_INDEX;

    spec = blt_table_row_spec(table, objPtr, &tagName);
    switch (spec) {
    case TABLE_SPEC_INDEX:
	p = Tcl_GetString(objPtr);
	if (p == tagName) {
	    result = Blt_GetLongFromObj((Tcl_Interp *)NULL, objPtr, &index);
	} else {
	    result = Blt_GetLong((Tcl_Interp *)NULL, (char *)tagName, &index);
	}
	if (result != TCL_OK) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "badly formed row index \"", tagName, 
			"\"", (char *)NULL);
	    }
	    return TCL_ERROR;
	}
	if ((index < 0) || (index >= blt_table_num_rows(table))) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "bad row index \"", 
			Tcl_GetString(objPtr), "\"", (char *)NULL);
	    }
	    return TCL_ERROR;
	}		
	iterPtr->start = index;
	iterPtr->end = iterPtr->start + 1;
	iterPtr->tagName = tagName;
	iterPtr->numEntries = 1;
	return TCL_OK;

    case TABLE_SPEC_LABEL:
	iterPtr->tablePtr = blt_table_row_get_label_table(table, tagName);
	if (iterPtr->tablePtr == NULL) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "can't find row label \"", tagName, 
			"\" in ", blt_table_name(table), (char *)NULL);
	    }
	    return TCL_ERROR;
	}
	iterPtr->type = TABLE_ITERATOR_TAG;
	iterPtr->tagName = tagName;
	iterPtr->numEntries = iterPtr->tablePtr->numEntries;
	return TCL_OK;

    case TABLE_SPEC_TAG:
	if (strcmp(tagName, "all") == 0) {
	    iterPtr->type = TABLE_ITERATOR_ALL;
	    iterPtr->start = 0;
	    iterPtr->end = iterPtr->numEntries = blt_table_num_rows(table);
	    iterPtr->tagName = tagName;
	} else if (strcmp(tagName, "end") == 0) {
	    iterPtr->tagName = tagName;
	    iterPtr->start = blt_table_num_rows(table) - 1;
	    iterPtr->end = iterPtr->start + 1;
	    iterPtr->numEntries = 1;
	} else {
	    iterPtr->tablePtr = blt_table_get_row_tag_table(iterPtr->table, 
		tagName);
	    if (iterPtr->tablePtr == NULL) {
		if (interp != NULL) {
		    Tcl_AppendResult(interp, "can't find row tag \"", tagName, 
			"\" in ", blt_table_name(table), (char *)NULL);
		}
		return TCL_ERROR;
	    }
	    iterPtr->type = TABLE_ITERATOR_TAG;
	    iterPtr->tagName = tagName;
	    iterPtr->numEntries = iterPtr->tablePtr->numEntries;
	}
	return TCL_OK;

    case TABLE_SPEC_RANGE:
	p = strchr(tagName, '-');
	if (p == NULL) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "bad range specification \"", tagName, 
			"\"", (char *)NULL);
	    }
	    return TCL_ERROR;
	}
	rangeObjPtr = Tcl_NewStringObj(tagName, p - tagName);
	from = blt_table_get_row(interp, table, rangeObjPtr);
	Tcl_DecrRefCount(rangeObjPtr);
        if (from == NULL) {
	    return TCL_ERROR;
	}
	rangeObjPtr = Tcl_NewStringObj(p + 1, -1);
	to = blt_table_get_row(interp, table, rangeObjPtr);
	Tcl_DecrRefCount(rangeObjPtr);
        if (to == NULL) {
	    return TCL_ERROR;
	}
	iterPtr->start = blt_table_row_index(from);
	iterPtr->end   = blt_table_row_index(to) + 1;
	iterPtr->numEntries = iterPtr->end - iterPtr->start;
	iterPtr->type = TABLE_ITERATOR_RANGE;
	iterPtr->table = table;
	iterPtr->next = -1;
	iterPtr->tagName = tagName;
	return TCL_OK;

    default:
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "unknown row specification \"", tagName, 
		"\" in ", blt_table_name(table), (char *)NULL);
	}
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_first_tagged_row --
 *
 *	Returns the id of the next row derived from the given tag.
 *
 * Results:
 *	Returns the row location of the first item.  If no more rows
 *	can be found, then -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_ROW
blt_table_first_tagged_row(BLT_TABLE_ITERATOR *iterPtr)
{
    if (iterPtr->type == TABLE_ITERATOR_TAG) {
	Blt_HashEntry *hPtr;

	hPtr = Blt_FirstHashEntry(iterPtr->tablePtr, &iterPtr->cursor);
	if (hPtr == NULL) {
	    return NULL;
	}
	return Blt_GetHashValue(hPtr);
    } else if (iterPtr->type == TABLE_ITERATOR_CHAIN) {
	iterPtr->link = Blt_Chain_FirstLink(iterPtr->chain);
	if (iterPtr->link != NULL) {
	    return Blt_Chain_GetValue(iterPtr->link);
	}
    } else if (iterPtr->start < iterPtr->end) {
	BLT_TABLE_ROW row;
	
	row = blt_table_row(iterPtr->table, iterPtr->start);
	iterPtr->next = iterPtr->start + 1;
	return row;
    } 
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_next_tagged_row --
 *
 *	Returns the id of the next row derived from the given tag.
 *
 * Results:
 *	Returns the row location of the first item.  If no more rows
 *	can be found, then -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_ROW
blt_table_next_tagged_row(BLT_TABLE_ITERATOR *iterPtr)
{
    if (iterPtr->type == TABLE_ITERATOR_TAG) {
	Blt_HashEntry *hPtr;

	hPtr = Blt_NextHashEntry(&iterPtr->cursor); 
	if (hPtr != NULL) {
	    return Blt_GetHashValue(hPtr);
	}
    } else if (iterPtr->type == TABLE_ITERATOR_CHAIN) {
	iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
	if (iterPtr->link != NULL) {
	    return Blt_Chain_GetValue(iterPtr->link);
	}
    } else if (iterPtr->next < iterPtr->end) {
	BLT_TABLE_ROW row;
	
	row = blt_table_row(iterPtr->table, iterPtr->next);
	iterPtr->next++;
	return row;
    }	
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_row --
 *
 *	Gets the row offset associated the given row index, tag, or
 *	label.  This routine is used when you want only one row index.
 *	It's an error if more than one row is specified (e.g. "all"
 *	tag or range "1:4").  It's also an error if the row tag is
 *	empty (no rows are currently tagged).
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_ROW
blt_table_get_row(Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr)
{
    BLT_TABLE_ITERATOR iter;
    BLT_TABLE_ROW first, next;

    if (blt_table_iterate_row(interp, table, objPtr, &iter) != TCL_OK) {
	return NULL;
    }
    first = blt_table_first_tagged_row(&iter);
    if (first == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "no rows specified by \"", 
			     Tcl_GetString(objPtr), "\"", (char *)NULL);
	}
	return NULL;
    }
    next = blt_table_next_tagged_row(&iter);
    if (next != NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "multiple rows specified by \"", 
			     Tcl_GetString(objPtr), "\"", (char *)NULL);
	}
	return NULL;
    }
    return first;
}

BLT_TABLE_ROWCOLUMN_SPEC
blt_table_column_spec(BLT_TABLE table, Tcl_Obj *objPtr, const char **sp)
{
    const char *p;
    const char *string;
    long index;
    char c;

    string = Tcl_GetString(objPtr);
    *sp = string;
    c = string[0];
    if ((isdigit(c)) && 
	Blt_GetLongFromObj((Tcl_Interp *)NULL, objPtr, &index) == TCL_OK) {
	return TABLE_SPEC_INDEX;
    } else if ((c == 'e') && (strcmp(string, "end") == 0)) {
	return TABLE_SPEC_TAG;
    } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
	return TABLE_SPEC_TAG;
    } else if ((c == 'r') && (strncmp(string, "range=", 6) == 0)) {
	*sp = string + 6;
	return TABLE_SPEC_RANGE;
    } else if ((c == 'i') && (strncmp(string, "index=", 6) == 0)) {
	*sp = string + 6;
	return TABLE_SPEC_INDEX;
    } else if ((c == 'l') && (strncmp(string, "label=", 6) == 0)) {
	*sp = string + 6;
	return TABLE_SPEC_LABEL;
    } else if ((c == 't') && (strncmp(string, "tag=", 4) == 0)) {
	*sp = string + 4;
	return TABLE_SPEC_TAG;
    } else if (blt_table_get_column_tag_table(table, string) != NULL) {
	return TABLE_SPEC_TAG;
    } else if (blt_table_get_column_by_label(table, string) != NULL) {
	return TABLE_SPEC_LABEL;
    }
    p = strchr(string, '-');
    if (p != NULL) {
	Tcl_Obj *objPtr;
	BLT_TABLE_COLUMN col;

	objPtr = Tcl_NewStringObj(string, p - string);
	Tcl_IncrRefCount(objPtr);
	col = blt_table_get_column(NULL, table, objPtr);
	Tcl_DecrRefCount(objPtr);
        if (col != NULL) {
	    objPtr = Tcl_NewStringObj(p + 1, -1);
	    col = blt_table_get_column(NULL, table, objPtr);
	    Tcl_DecrRefCount(objPtr);
	    if (col != NULL) {
		return TABLE_SPEC_RANGE;
	    }
	}
    }
    return TABLE_SPEC_UNKNOWN;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_iterate_column --
 *
 *	Returns the id of the first column derived from the given tag,
 *	label or index represented in objPtr.  
 *
 * Results:
 *	Returns the column location of the first item.  If no column 
 *	can be found, then -1 is returned and an error message is
 *	left in the interpreter.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_iterate_column(Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr, 
			 BLT_TABLE_ITERATOR *iterPtr)
{
    BLT_TABLE_COLUMN from, to;
    const char *tagName, *p;
    int result;
    Tcl_Obj *fromObjPtr, *toObjPtr;
    long index;
    BLT_TABLE_ROWCOLUMN_SPEC spec;

    iterPtr->table = table;
    iterPtr->type = TABLE_ITERATOR_INDEX;

    spec = blt_table_column_spec(table, objPtr, &tagName);
    switch (spec) {
    case TABLE_SPEC_INDEX:
	p = Tcl_GetString(objPtr);
	if (p == tagName) {
	    result = Blt_GetLongFromObj((Tcl_Interp *)NULL, objPtr, &index);
	} else {
	    result = Blt_GetLong((Tcl_Interp *)NULL, (char *)tagName, &index);
	}
	if (result != TCL_OK) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "badly formed column index \"", 
			tagName, "\"", (char *)NULL);
	    }
	    return TCL_ERROR;
	}
	if ((index < 0) || (index >= blt_table_num_columns(table))) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "bad column index \"", 
			Tcl_GetString(objPtr), "\"", (char *)NULL);
	    }
	    return TCL_ERROR;
	}		
	iterPtr->start = index;
	iterPtr->end = iterPtr->start + 1;
	iterPtr->tagName = tagName;
	iterPtr->numEntries = 1;
	return TCL_OK;

    case TABLE_SPEC_LABEL:
	iterPtr->tablePtr = blt_table_column_get_label_table(table, tagName);
	if (iterPtr->tablePtr == NULL) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "can't find column label \"", tagName, 
			"\" in ", blt_table_name(table), (char *)NULL);
	    }
	    return TCL_ERROR;
	}
	iterPtr->type = TABLE_ITERATOR_TAG;
	iterPtr->tagName = tagName;
	iterPtr->numEntries = iterPtr->tablePtr->numEntries;
	return TCL_OK;

    case TABLE_SPEC_TAG:
	if (strcmp(tagName, "all") == 0) {
	    iterPtr->type = TABLE_ITERATOR_ALL;
	    iterPtr->start = 0;
	    iterPtr->end = iterPtr->numEntries = blt_table_num_columns(table);
	    iterPtr->tagName = tagName;
	} else if (strcmp(tagName, "end") == 0) {
	    iterPtr->tagName = tagName;
	    iterPtr->start = blt_table_num_columns(table) - 1;
	    iterPtr->end = iterPtr->start + 1;
	    iterPtr->numEntries = 1;
	} else {
	    iterPtr->tablePtr = blt_table_get_column_tag_table(iterPtr->table,
		tagName);
	    if (iterPtr->tablePtr == NULL) {
		if (interp != NULL) {
		    Tcl_AppendResult(interp, "can't find column tag \"", 
			tagName, "\" in ", blt_table_name(table), (char *)NULL);
		}
		return TCL_ERROR;
	    }
	    iterPtr->type = TABLE_ITERATOR_TAG;
	    iterPtr->tagName = tagName;
	    iterPtr->numEntries = iterPtr->tablePtr->numEntries;
	}
	return TCL_OK;

    case TABLE_SPEC_RANGE:
	p = strchr(tagName, '-');
	if (p == NULL) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "bad range specification \"", tagName, 
			"\"", (char *)NULL);
	    }
	    return TCL_ERROR;
	}
	fromObjPtr = Tcl_NewStringObj(tagName, p - tagName);
	from = blt_table_get_column(interp, table, fromObjPtr);
	Tcl_DecrRefCount(fromObjPtr);
        if (from == NULL) {
	    return TCL_ERROR;
	}
	toObjPtr = Tcl_NewStringObj(p + 1, -1);
	to = blt_table_get_column(interp, table, toObjPtr);
	Tcl_DecrRefCount(toObjPtr);
        if (to == NULL) {
	    return TCL_ERROR;
	}
	iterPtr->start = blt_table_column_index(from);
	iterPtr->end   = blt_table_column_index(to) + 1;
	iterPtr->numEntries = iterPtr->end - iterPtr->start;
	iterPtr->type  = TABLE_ITERATOR_RANGE;
	iterPtr->tagName = tagName;
	return TCL_OK;

    default:
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "unknown column specification \"", 
		tagName, "\" in ", blt_table_name(table),(char *)NULL);
	}
    }
    return TCL_ERROR;
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * blt_table_iterate_column --
 *
 *	Initials the table iterator to walk through the columns tagged by the
 *	given tag, label, or index, as represented in objPtr.
 *
 *	Notes: 
 *
 *	1) A tag doesn't need to point to any columns. It can be empty.  This
 *	routine does not check if a tag represents any columns, only that the
 *	tag itself exists.
 *
 *	2) If a column label and tag are the same string, the label always
 *	wins.
 *
 *	3) A range of columns can be represented by "from x to y" x:y x-y {x y}
 *
 * Results:
 *	A standard TCL result.  If there is an error parsing the index or tag,
 *	then TCL_ERROR is returned and an error message is left in the
 *	interpreter.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_iterate_column(Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr, 
			 BLT_TABLE_ITERATOR *iterPtr)
{
    long index;
    const char *p, *rp, *pend;
    const char *tagName;
    int numBytes;
    int badrange;

    iterPtr->table = table;
    iterPtr->type = TABLE_ITERATOR_INDEX;
    iterPtr->next = -1;

    tagName = Tcl_GetStringFromObj(objPtr, &nBytes);
    rp = NULL;
    for (p = tagName, pend = p + numBytes; p < pend; p++) {
	if (*p != '-') {
	    continue;
	}
	if (rp != NULL) {
	    /* Found more than one range specifier. We'll assume it's
	     * not a range and try is as a regular index, tag, or
	     * label. */
	    rp = NULL;
	    break;
	}
	rp = p;
    } 
    badrange = FALSE;
    if ((rp != NULL) && (rp != tagName) && (rp != (pend - 1))) {
	long length;
	Tcl_Obj *objPtr1, *objPtr2;
	BLT_TABLE_COLUMN from, to;
	
	length = rp - tagName;
	objPtr1 = Tcl_NewStringObj(tagName, length);
	rp++;
	objPtr2 = Tcl_NewStringObj(rp, pend - rp);
	from = blt_table_get_column(interp, table, objPtr1);
	if (from != NULL) {
	    to = blt_table_get_column(interp, table, objPtr2);
	}
	Tcl_DecrRefCount(objPtr1);
	Tcl_DecrRefCount(objPtr2);
	if (to != NULL) {
	    iterPtr->start = blt_table_column_index(from);
	    iterPtr->end   = blt_table_column_index(to) + 1;
	    iterPtr->type  = TABLE_ITERATOR_RANGE;
	    return TCL_OK;
	}
	badrange = TRUE;
    }
    if (Blt_GetLongFromObj((Tcl_Interp *)NULL, objPtr, &index) == TCL_OK) {
	if ((index < 0) || (index >= blt_table_num_columns(table))) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, 
			"can't find column: bad column index \"", 
			Tcl_GetString(objPtr), "\"", (char *)NULL);
	    }
	    return TCL_ERROR;
	}		
	iterPtr->start = index;
	iterPtr->end = iterPtr->start + 1;
	return TCL_OK;
    } else if (strcmp(tagName, "all") == 0) {
	iterPtr->type = TABLE_ITERATOR_ALL;
	iterPtr->start = 0;
	iterPtr->end = blt_table_num_columns(table);
	return TCL_OK;
    } else if (strcmp(tagName, "end") == 0) {
	iterPtr->start = blt_table_num_columns(table) - 1;
	iterPtr->end   = iterPtr->start + 1;
	return TCL_OK;
    } else {
	Column *colPtr;

	colPtr = blt_table_get_column_by_label(table, tagName);
	if (colPtr != NULL) {
	    iterPtr->start = colPtr->index;
	    iterPtr->end = iterPtr->start + 1;
	    return TCL_OK;
	}
	iterPtr->tablePtr = blt_table_get_column_tag_table(iterPtr->table, 
		tagName);
	if (iterPtr->tablePtr != NULL) {
	    iterPtr->type = TABLE_ITERATOR_TAG;
	    return TCL_OK;
	}
    }
    if ((interp != NULL) && (!badrange)) {
	Tcl_AppendResult(interp, "can't find column tag \"", tagName, 
		"\" in ", blt_table_name(table), (char *)NULL);
    }
    return TCL_ERROR;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_first_tagged_column --
 *
 *	Returns the first column based upon given iterator.
 *
 * Results:
 *	Returns the column location of the first item.  If no more columns
 *	can be found, then -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_COLUMN
blt_table_first_tagged_column(BLT_TABLE_ITERATOR *iterPtr)
{
    if (iterPtr->type == TABLE_ITERATOR_TAG) {
	Blt_HashEntry *hPtr;

	hPtr = Blt_FirstHashEntry(iterPtr->tablePtr, &iterPtr->cursor);
	if (hPtr == NULL) {
	    return NULL;
	}
	return Blt_GetHashValue(hPtr);
    } else if (iterPtr->type == TABLE_ITERATOR_CHAIN) {
	iterPtr->link = Blt_Chain_FirstLink(iterPtr->chain);
	if (iterPtr->link != NULL) {
	    return Blt_Chain_GetValue(iterPtr->link);
	}
    } else if (iterPtr->start < iterPtr->end) {
	BLT_TABLE_COLUMN col;
	
	col = blt_table_column(iterPtr->table, iterPtr->start);
	iterPtr->next = iterPtr->start + 1;
	return col;
    } 
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_next_tagged_column --
 *
 *	Returns the column location of the next column using the given
 *	iterator.
 *
 * Results:
 *	Returns the column location of the next item.  If no more columns can
 *	be found, then -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_COLUMN
blt_table_next_tagged_column(BLT_TABLE_ITERATOR *iterPtr)
{
    if (iterPtr->type == TABLE_ITERATOR_TAG) {
	Blt_HashEntry *hPtr;

	hPtr = Blt_NextHashEntry(&iterPtr->cursor); 
	if (hPtr != NULL) {
	    return Blt_GetHashValue(hPtr);
	}
    } else if (iterPtr->type == TABLE_ITERATOR_CHAIN) {
	iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
	if (iterPtr->link != NULL) {
	    return Blt_Chain_GetValue(iterPtr->link);
	}
    } else if (iterPtr->next < iterPtr->end) {
	BLT_TABLE_COLUMN col;
	
	col = blt_table_column(iterPtr->table, iterPtr->next);
	iterPtr->next++;
	return col;
    }	
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_column --
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_COLUMN
blt_table_get_column(Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr)
{
    BLT_TABLE_ITERATOR iter;
    BLT_TABLE_COLUMN first, next;

    if (blt_table_iterate_column(interp, table, objPtr, &iter) != TCL_OK) {
	return NULL;
    }
    first = blt_table_first_tagged_column(&iter);
    if (first == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "no columns specified by \"", 
		Tcl_GetString(objPtr), "\"", (char *)NULL);
	}
	return NULL;
    }
    next = blt_table_next_tagged_column(&iter);
    if (next != NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "multiple columns specified by \"", 
		Tcl_GetString(objPtr), "\"", (char *)NULL);
	}
	return NULL;
    }
    return first;
}


int
blt_table_list_columns(Tcl_Interp *interp, BLT_TABLE table, int objc, 
		      Tcl_Obj *const *objv, Blt_Chain chain)
{
    Blt_ChainLink link;
    Blt_HashTable cols;
    int i;

    Blt_InitHashTableWithPool(&cols, BLT_ONE_WORD_KEYS);
    /* Initialize the hash table with the existing entries. */
    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	int isNew;
	BLT_TABLE_COLUMN col;

	col = Blt_Chain_GetValue(link);
	Blt_CreateHashEntry(&cols, (char *)col, &isNew);
    }
    /* Collect the columns into a hash table. */
    for (i = 0; i < objc; i++) {
	BLT_TABLE_ITERATOR iter;
	BLT_TABLE_COLUMN col;

	if (blt_table_iterate_column(interp, table, objv[i], &iter) 
	    != TCL_OK) {
	    Blt_DeleteHashTable(&cols);
	    return TCL_ERROR;
	}
	for (col = blt_table_first_tagged_column(&iter); col != NULL; 
	     col = blt_table_next_tagged_column(&iter)) {
	    int isNew;

	    Blt_CreateHashEntry(&cols, (char *)col, &isNew);
	    if (isNew) {
		Blt_Chain_Append(chain, col);
	    }
	}
    }
    Blt_DeleteHashTable(&cols);
    return TCL_OK;
}

int
blt_table_list_rows(Tcl_Interp *interp, BLT_TABLE table, int objc, 
		   Tcl_Obj *const *objv, Blt_Chain chain)
{
    Blt_ChainLink link;
    Blt_HashTable rows;
    int i;

    Blt_InitHashTableWithPool(&rows, BLT_ONE_WORD_KEYS);
    /* Initialize the hash table with the existing entries. */
    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	int isNew;
	BLT_TABLE_ROW row;

	row = Blt_Chain_GetValue(link);
	Blt_CreateHashEntry(&rows, (char *)row, &isNew);
    }
    for (i = 0; i < objc; i++) {
	BLT_TABLE_ITERATOR iter;
	BLT_TABLE_ROW row;

	if (blt_table_iterate_row(interp, table, objv[i], &iter) != TCL_OK){
	    Blt_DeleteHashTable(&rows);
	    return TCL_ERROR;
	}
	/* Append the new rows onto the chain. */
	for (row = blt_table_first_tagged_row(&iter); row != NULL; 
	     row = blt_table_next_tagged_row(&iter)) {
	    int isNew;

	    Blt_CreateHashEntry(&rows, (char *)row, &isNew);
	    if (isNew) {
		Blt_Chain_Append(chain, row);
	    }
	}
    }
    Blt_DeleteHashTable(&rows);
    return TCL_OK;
}

int
blt_table_iterate_row_objv(Tcl_Interp *interp, BLT_TABLE table, int objc, 
			  Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr)
{
    Blt_Chain chain;

    chain = Blt_Chain_Create();
    if (blt_table_list_rows(interp, table, objc, objv, chain) != TCL_OK) {
	Blt_Chain_Destroy(chain);
	return TCL_ERROR;
    }
    iterPtr->type = TABLE_ITERATOR_CHAIN;
    iterPtr->next = -1;
    iterPtr->start = 0;
    iterPtr->end = iterPtr->numEntries = Blt_Chain_GetLength(chain);
    iterPtr->chain = chain;
    iterPtr->tagName = "";
    return TCL_OK;
}

void
blt_table_iterate_all_rows(BLT_TABLE table, BLT_TABLE_ITERATOR *iterPtr)
{
    iterPtr->table = table;
    iterPtr->type = TABLE_ITERATOR_ALL;
    iterPtr->next = -1;
    iterPtr->start = 0;
    iterPtr->end = iterPtr->numEntries = blt_table_num_rows(table);
    iterPtr->tagName = "all";
    iterPtr->chain = NULL;
}

int
blt_table_iterate_column_objv(Tcl_Interp *interp, BLT_TABLE table, int objc, 
			     Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr)
{
    Blt_Chain chain;

    chain = Blt_Chain_Create();
    if (blt_table_list_columns(interp, table, objc, objv, chain) != TCL_OK) {
	Blt_Chain_Destroy(chain);
	return TCL_ERROR;
    }
    iterPtr->table = table;
    iterPtr->type = TABLE_ITERATOR_CHAIN;
    iterPtr->next = -1;
    iterPtr->start = 0;
    iterPtr->end = iterPtr->numEntries = Blt_Chain_GetLength(chain);
    iterPtr->chain = chain;
    iterPtr->tagName = "";
    return TCL_OK;
}

void
blt_table_iterate_all_columns(BLT_TABLE table, BLT_TABLE_ITERATOR *iterPtr)
{
    iterPtr->table = table;
    iterPtr->type = TABLE_ITERATOR_ALL;
    iterPtr->next = -1;
    iterPtr->start = 0;
    iterPtr->end = iterPtr->numEntries = blt_table_num_columns(table);
    iterPtr->tagName = "all";
    iterPtr->chain = NULL;
}

void
blt_table_free_iterator_objv(BLT_TABLE_ITERATOR *iterPtr)
{
    if ((iterPtr->type == TABLE_ITERATOR_CHAIN) && (iterPtr->chain != NULL)) {
	Blt_Chain_Destroy(iterPtr->chain);
	iterPtr->chain = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeTrace --
 *
 *	Memory is deallocated for the trace.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeTrace(Trace *tracePtr)
{
    if (tracePtr->rowTag != NULL) {
	Blt_Free(tracePtr->rowTag);
    }
    if (tracePtr->colTag != NULL) {
	Blt_Free(tracePtr->colTag);
    }
    Blt_Free(tracePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_delete_trace --
 *
 *	Deletes a trace.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Memory is deallocated for the trace.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_delete_trace(Table *tablePtr, Trace *tracePtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&tablePtr->traces, tracePtr);
    if (hPtr == NULL) {
	return;				/* Invalid trace token. */
    }
    Blt_DeleteHashEntry(&tablePtr->traces, hPtr);
    if ((tracePtr->flags & TABLE_TRACE_DESTROYED) == 0) {
	if (tracePtr->deleteProc != NULL) {
	    (*tracePtr->deleteProc)(tracePtr->clientData);
	}
	if (tracePtr->flags & TABLE_TRACE_PENDING) {
	    Tcl_CancelIdleCall(TraceIdleProc, tracePtr);
	}
	/* 
	 * This accomplishes two things.  
	 * 1) It doesn't let it anything match the trace and 
	 * 2) marks the trace as invalid. 
	 */
	/* Take it out of the list so you can't use it anymore. */
	if (tracePtr->readLink != NULL) {
	    Blt_Chain_DeleteLink(tablePtr->readTraces, tracePtr->readLink);
	    tracePtr->readLink = NULL;
	}
	if (tracePtr->writeLink != NULL) {
	    Blt_Chain_DeleteLink(tablePtr->writeTraces, tracePtr->writeLink);
	    tracePtr->writeLink = NULL;
	}
	tracePtr->flags = TABLE_TRACE_DESTROYED;	
	Tcl_EventuallyFree(tracePtr, (Tcl_FreeProc *)FreeTrace);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_create_trace --
 *
 *	Creates a trace for one or more rows/columns in the table based upon
 *	the given criteria.  Whenever a matching action occurs in the table
 *	object, the specified procedure is executed.  
 *
 *	The trace may be put into 1 or 2 callback lists: one for read traces
 *	and another for write, create, and/or unset traces.  This is done
 *	to improve performance on reads.  We only examine the list of 
 *	read traces, which in most cases will me empty.  
 *
 * Results:
 *	Returns a token for the trace.
 *
 * Side Effects:
 *	Memory is allocated for the trace.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_TRACE
blt_table_create_trace(
    Table *tablePtr,			/* Table to be traced. */
    Row *rowPtr, 
    Column *colPtr,			/* Cell in table. */
    const char *rowTag, 
    const char *colTag,
    unsigned int flags,			/* Bit mask indicating what actions to
					 * trace. */
    BLT_TABLE_TRACE_PROC *proc,	        /* Callback procedure for the trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)		/* One-word of data passed along when
					 * the callback is executed. */
{
    Trace *tracePtr;
    int isNew;

    tracePtr = Blt_Calloc(1, sizeof (Trace));
    if (tracePtr == NULL) {
	return NULL;
    }
    tracePtr->row = rowPtr;
    tracePtr->column = colPtr;
    if (rowTag != NULL) {
	tracePtr->rowTag = Blt_AssertStrdup(rowTag);
    }
    if (colTag != NULL) {
	tracePtr->colTag = Blt_AssertStrdup(colTag);
    }
    tracePtr->flags = flags;
    tracePtr->proc = proc;
    tracePtr->deleteProc = deleteProc;
    tracePtr->clientData = clientData;
    if (tracePtr->flags & TABLE_TRACE_READS) {
	tracePtr->readLink = Blt_Chain_Append(tablePtr->readTraces, tracePtr);
    }
    if (tracePtr->flags & 
	(TABLE_TRACE_WRITES | TABLE_TRACE_UNSETS | TABLE_TRACE_CREATES)) {
	tracePtr->writeLink = Blt_Chain_Append(tablePtr->writeTraces, tracePtr);
    }
    Blt_CreateHashEntry(&tablePtr->traces, tracePtr, &isNew);
    assert(isNew);
    tracePtr->table = tablePtr;
    return tracePtr;
}

void
blt_table_trace_column(
    Table *tablePtr,			/* Table to be traced. */
    Column *colPtr,			/* Cell in table. */
    unsigned int flags,			/* Bit mask indicating what actions to
					 * trace. */
    BLT_TABLE_TRACE_PROC *proc,	       /* Callback procedure for the trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)		/* One-word of data passed along when
					 * the callback is executed. */
{
    blt_table_create_trace(tablePtr, NULL, colPtr, NULL, NULL, flags,
			   proc, deleteProc, clientData);
}

BLT_TABLE_TRACE
blt_table_create_column_trace(
    Table *tablePtr,			/* Table to be traced. */
    Column *colPtr,			/* Cell in table. */
    unsigned int flags,			/* Bit mask indicating what actions to
					 * trace. */
    BLT_TABLE_TRACE_PROC *proc,	       /* Callback procedure for the trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)		/* One-word of data passed along when
					 * the callback is executed. */
{
    return blt_table_create_trace(tablePtr, NULL, colPtr, NULL, NULL, flags,
		proc, deleteProc, clientData);
}

BLT_TABLE_TRACE
blt_table_create_column_tag_trace(
    Table *tablePtr,			/* Table to be traced. */
    const char *colTag,			/* Cell in table. */
    unsigned int flags,			/* Bit mask indicating what actions to
					 * trace. */
    BLT_TABLE_TRACE_PROC *proc,	       /* Callback procedure for the trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)		/* One-word of data passed along when
					 * the callback is executed. */
{
    return blt_table_create_trace(tablePtr, NULL, NULL, NULL, colTag, flags,
		proc, deleteProc, clientData);
}

void
blt_table_trace_row(
    Table *tablePtr,			/* Table to be traced. */
    Row *rowPtr,			/* Cell in table. */
    unsigned int flags,			/* Bit mask indicating what actions to
					 * trace. */
    BLT_TABLE_TRACE_PROC *proc,	        /* Callback procedure for the
					 * trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)		/* One-word of data passed along when
					 * the callback is executed. */
{
    blt_table_create_trace(tablePtr, rowPtr, NULL, NULL, NULL, flags,
	proc, deleteProc, clientData);
}

BLT_TABLE_TRACE
blt_table_create_row_trace(
    Table *tablePtr,			/* Table to be traced. */
    Row *rowPtr,			/* Cell in table. */
    unsigned int flags,			/* Bit mask indicating what actions to
					 * trace. */
    BLT_TABLE_TRACE_PROC *proc,	        /* Callback procedure for the
					 * trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)		/* One-word of data passed along when
					 * the callback is executed. */
{
    return blt_table_create_trace(tablePtr, rowPtr, NULL, NULL, NULL, flags,
		proc, deleteProc, clientData);
}

BLT_TABLE_TRACE
blt_table_create_row_tag_trace(
    Table *tablePtr,			/* Table to be traced. */
    const char *rowTag,			/* Cell in table. */
    unsigned int flags,			/* Bit mask indicating what actions to
					 * trace. */
    BLT_TABLE_TRACE_PROC *proc,	        /* Callback procedure for the trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)		/* One-word of data passed along when
					 * the callback is executed. */
{
    return blt_table_create_trace(tablePtr, NULL, NULL, rowTag, NULL, flags,
		proc, deleteProc, clientData);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_release_tags --
 *
 *	Releases the tag table used by this client.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	If no client is using the table, then it is freed.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_release_tags(Table *tablePtr)
{
    Tags *tagsPtr;

    tagsPtr = tablePtr->tags;
    tagsPtr->refCount--;
    if (tagsPtr->refCount <= 0) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch cursor;

	for (hPtr = Blt_FirstHashEntry(&tagsPtr->rowTable, &cursor); 
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    Blt_HashTable *tablePtr;

	    tablePtr = Blt_GetHashValue(hPtr); 
	    Blt_DeleteHashTable(tablePtr);
	    Blt_Free(tablePtr);
	}
	Blt_DeleteHashTable(&tagsPtr->rowTable);
	tablePtr->rowTags = NULL;
	for (hPtr = Blt_FirstHashEntry(&tagsPtr->columnTable, &cursor); 
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    Blt_HashTable *tablePtr;

	    tablePtr = Blt_GetHashValue(hPtr); 
	    Blt_DeleteHashTable(tablePtr);
	    Blt_Free(tablePtr);
	}
	Blt_DeleteHashTable(&tagsPtr->columnTable);
	Blt_Free(tagsPtr);
	tablePtr->columnTags = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BLT_TABLE_TAGSAreShared --
 *
 *	Returns whether the tag table is shared with another client.
 *
 * Results:
 *	Returns TRUE if the current tag table is shared with another
 *	client, FALSE otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_tags_are_shared(Table *tablePtr)
{
    return (tablePtr->tags->refCount > 1);
}   

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_row_tag_table --
 *
 *	Returns the hash table containing row indices for a tag.
 *
 * Results:
 *	Returns a pointer to the hash table containing indices for the given
 *	tag.  If the row has no tags, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
Blt_HashTable *
blt_table_get_row_tag_table(Table *tablePtr, const char *tagName)		
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(tablePtr->rowTags, tagName);
    if (hPtr == NULL) {
	return NULL;		/* Row isn't tagged. */
    }
    return Blt_GetHashValue(hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_column_tag_table --
 *
 *	Returns the hash table containing column indices for a tag.
 *
 * Results:
 *	Returns a pointer to the hash table containing indices for the given
 *	tag.  If the tag has no indices, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
Blt_HashTable *
blt_table_get_column_tag_table(Table *tablePtr, const char *tagName)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(tablePtr->columnTags, tagName);
    if (hPtr == NULL) {
	return NULL;		
    }
    return Blt_GetHashValue(hPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * blt_table_forget_row_tag --
 *
 *	Removes a tag from the row tag table.  Row tags are contained in hash
 *	tables keyed by the tag name.  Each table is in turn hashed by the row
 *	index in the row tag table.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Entries for the given tag in the corresponding row in hash tables may
 *	be removed.
 *	
 *---------------------------------------------------------------------------
 */
int
blt_table_forget_row_tag(Tcl_Interp *interp, Table *tablePtr, const char *tagName)
{
    Blt_HashEntry *hPtr;
    Blt_HashTable *tagTablePtr;

    if ((strcmp(tagName, "all") == 0) || (strcmp(tagName, "end") == 0)) {
	return TCL_OK;			/* Can't forget reserved tags. */
    }
    hPtr = Blt_FindHashEntry(tablePtr->rowTags, tagName);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "unknown row tag \"", tagName, "\"", 
			     (char *)NULL);
	}
	return TCL_ERROR;		/* No such row tag. */
    }
    tagTablePtr = Blt_GetHashValue(hPtr);
    Blt_DeleteHashTable(tagTablePtr);
    Blt_Free(tagTablePtr);
    Blt_DeleteHashEntry(tablePtr->rowTags, hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_forget_column_tag --
 *
 *	Removes a tag from the column tag table.  Column tags are contained in
 *	hash tables keyed by the tag name.  Each table is in turn hashed by
 *	the column offset in the column tag table.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Entries for the given tag in the corresponding column in hash tables
 *	may be removed.
 *	
 *---------------------------------------------------------------------------
 */
int
blt_table_forget_column_tag(Tcl_Interp *interp, Table *tablePtr, 
			  const char *tagName)
{
    Blt_HashEntry *hPtr;
    Blt_HashTable *tagTablePtr;

    if ((strcmp(tagName, "all") == 0) || (strcmp(tagName, "end") == 0)) {
	return TCL_OK;			/* Can't forget reserved tags. */
    }
    hPtr = Blt_FindHashEntry(tablePtr->columnTags, tagName);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "unknown column tag \"", tagName, "\"", 
			     (char *)NULL);
	}
	return TCL_ERROR;		/* No such column tag. */
    }
    tagTablePtr = Blt_GetHashValue(hPtr);
    Blt_DeleteHashTable(tagTablePtr);
    Blt_Free(tagTablePtr);
    Blt_DeleteHashEntry(tablePtr->columnTags, hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_row_tag --
 *
 *	Associates a tag with a given row.  Individual row tags are stored in
 *	hash tables keyed by the tag name.  Each table is in turn stored in a
 *	hash table keyed by the row location.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A tag is stored for a particular row.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_row_tag(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr, 
		    const char *tagName)
{
    Blt_HashEntry *hPtr;
    Blt_HashTable *tagTablePtr;
    int isNew;
    long dummy;

    if ((strcmp(tagName, "all") == 0) || (strcmp(tagName, "end") == 0)) {
	return TCL_OK;		/* Don't need to create reserved tags. */
    }
    if (tagName[0] == '\0') {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "tag \"", tagName, "\" can't be empty.", 
		(char *)NULL);
	}
	return TCL_ERROR;
    }
    if (tagName[0] == '-') {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "tag \"", tagName, 
		"\" can't start with a '-'.", (char *)NULL);
	}
	return TCL_ERROR;
    }
    if (Blt_GetLong(NULL, (char *)tagName, &dummy) == TCL_OK) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "tag \"", tagName, "\" can't be a number.",
			     (char *)NULL);
	}
	return TCL_ERROR;
    }
    hPtr = Blt_CreateHashEntry(tablePtr->rowTags, tagName, &isNew);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't add tag \"", tagName, 
			 "\": out of memory", (char *)NULL);
	}
	return TCL_ERROR;
    }
    if (isNew) {
	tagTablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
	Blt_InitHashTable(tagTablePtr, BLT_ONE_WORD_KEYS);
	Blt_SetHashValue(hPtr, tagTablePtr);
    } else {
	tagTablePtr = Blt_GetHashValue(hPtr);
    }
    if (rowPtr != NULL) {
	hPtr = Blt_CreateHashEntry(tagTablePtr, (char *)rowPtr, &isNew);
	if (isNew) {
	    Blt_SetHashValue(hPtr, rowPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_column_tag --
 *
 *	Associates a tag with a given column.  Individual column tags
 *	are stored in hash tables keyed by the tag name.  Each table
 *	is in turn stored in a hash table keyed by the column
 *	location.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A tag is stored for a particular column.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_column_tag(Tcl_Interp *interp, Table *tablePtr, Column *columnPtr, 
		       const char *tagName)
{
    Blt_HashEntry *hPtr;
    Blt_HashTable *tagTablePtr;
    int isNew;
    long dummy;

    if ((strcmp(tagName, "all") == 0) || (strcmp(tagName, "end") == 0)) {
	return TCL_OK;			/* Don't create reserved tags. */
    }
    if (tagName[0] == '\0') {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "tag \"", tagName, "\" can't be empty.", 
			     (char *)NULL);
	}
	return TCL_ERROR;
    }
    if (tagName[0] == '-') {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "tag \"", tagName, 
		"\" can't start with a '-'.", (char *)NULL);
	}
	return TCL_ERROR;
    }
    if (Blt_GetLong(NULL, (char *)tagName, &dummy) == TCL_OK) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "tag \"", tagName, "\" can't be a number.",
			     (char *)NULL);
	}
	return TCL_ERROR;
    }
    hPtr = Blt_CreateHashEntry(tablePtr->columnTags, tagName, &isNew);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't add tag \"", tagName, 
			 "\": out of memory", (char *)NULL);
	}
	return TCL_ERROR;
    }
    if (isNew) {
	tagTablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
	Blt_InitHashTable(tagTablePtr, BLT_ONE_WORD_KEYS);
	Blt_SetHashValue(hPtr, tagTablePtr);
    } else {
	tagTablePtr = Blt_GetHashValue(hPtr);
    }
    if (columnPtr != NULL) {
	hPtr = Blt_CreateHashEntry(tagTablePtr, (char *)columnPtr, &isNew);
	if (isNew) {
	    Blt_SetHashValue(hPtr, columnPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_row_has_tag --
 *
 *	Checks if a tag is associated with the given row.  
 *
 * Results:
 *	Returns TRUE if the tag is found, FALSE otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_row_has_tag(Table *tablePtr, Row *rowPtr, const char *tagName)
{
    Blt_HashTable *tagTablePtr;
    Blt_HashEntry *hPtr;

    if (strcmp(tagName, "all") == 0) {
	return TRUE;		/* "all" tags matches every row. */
    }
    if (strcmp(tagName, "end") == 0) {
	return (blt_table_row_index(rowPtr) == (blt_table_num_rows(tablePtr)-1));
    }
    tagTablePtr = blt_table_get_row_tag_table(tablePtr, tagName);
    if (tagTablePtr == NULL) {
	return FALSE;
    }
    hPtr = Blt_FindHashEntry(tagTablePtr, (char *)rowPtr);
    if (hPtr != NULL) {
	return TRUE;		/* Found tag in row tag table. */
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_column_has_tag --
 *
 *	Checks if a tag is associated with the given column.  
 *
 * Results:
 *	Returns TRUE if the tag is found, FALSE otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_column_has_tag(Table *tablePtr, Column *colPtr, const char *tagName)
{
    Blt_HashTable *tagTablePtr;
    Blt_HashEntry *hPtr;

    if (strcmp(tagName, "all") == 0) {
	return TRUE;			/* "all" tags matches every column. */
    }
    if (strcmp(tagName, "end") == 0) {
	return (blt_table_column_index(colPtr) == 
		(blt_table_num_columns(tablePtr)-1));
    }
    tagTablePtr = blt_table_get_column_tag_table(tablePtr, tagName);
    if (tagTablePtr == NULL) {
	return FALSE;
    }
    hPtr = Blt_FindHashEntry(tagTablePtr, (char *)colPtr);
    if (hPtr != NULL) {
	return TRUE;			/* Found tag in column tag table. */
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_unset_row_tag --
 *
 *	Removes a tag from a given row.  
 *
 * Results:
 *	A standard TCL result.  If an error occurred, TCL_ERROR
 *	is returned and the interpreter result contains the error
 *	message.
 *
 * Side Effects:
 *      The tag associated with the row is freed.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_unset_row_tag(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr, 
		      const char *tagName)
{
    Blt_HashEntry *hPtr;
    Blt_HashTable *tagTablePtr;

    if ((strcmp(tagName, "all") == 0) || (strcmp(tagName, "end") == 0)) {
	return TCL_OK;			/* Can't remove reserved tags. */
    } 
    tagTablePtr = blt_table_get_row_tag_table(tablePtr, tagName);
    if (tagTablePtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "unknown row tag \"", tagName, "\"", 
		(char *)NULL);
	}
	return TCL_ERROR;
    }
    hPtr = Blt_FindHashEntry(tagTablePtr, (char *)rowPtr);
    if (hPtr != NULL) {
	Blt_DeleteHashEntry(tagTablePtr, hPtr);
    }
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_unset_column_tag --
 *
 *	Removes a tag from a given column.  
 *
 * Results:
 *	A standard TCL result.  If an error occurred, TCL_ERROR
 *	is returned and the interpreter result contains the error
 *	message.
 *
 * Side Effects:
 *      The tag associated with the column is freed.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_unset_column_tag(Tcl_Interp *interp, Table *tablePtr, Column *colPtr, 
			 const char *tagName)
{
    Blt_HashEntry *hPtr;
    Blt_HashTable *tagTablePtr;

    if ((strcmp(tagName, "all") == 0) || (strcmp(tagName, "end") == 0)) {
	return TCL_OK;			/* Can't remove reserved tags. */
    } 
    tagTablePtr = blt_table_get_column_tag_table(tablePtr, tagName);
    if (tagTablePtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "unknown column tag \"", tagName, "\"", 
		(char *)NULL);
	}
	return TCL_ERROR;
    }
    hPtr = Blt_FindHashEntry(tagTablePtr, (char *)colPtr);
    if (hPtr != NULL) {
	Blt_DeleteHashEntry(tagTablePtr, hPtr);
    }
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_clear_row_tags --
 *
 *	Removes all tags for a given row.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *      All tags associated with the row are freed.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_clear_row_tags(Table *tablePtr, Row *rowPtr)
{
    ClearTags(tablePtr->rowTags, (Header *)rowPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_clear_column_tags --
 *
 *	Removes all tags for a given column.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *      All tags associated with the column are freed.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_clear_column_tags(Table *tablePtr, Column *colPtr)
{
    ClearTags(tablePtr->columnTags, (Header *)colPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_value --
 *
 *	Gets a scalar Tcl_Obj value from the table at the designated
 *	row, column location.  "Read" traces may be fired *before* the
 *	value is retrieved.  If no value exists at that location,
 *	*objPtrPtr is set to NULL.
 *
 * Results:
 *	A standard TCL result.  Returns TCL_OK if successful accessing
 *	the table location.  If an error occurs, TCL_ERROR is returned
 *	and an error message is left in the interpreter.
 *
 * -------------------------------------------------------------------------- 
 */
BLT_TABLE_VALUE
blt_table_get_value(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    return GetValue(tablePtr, rowPtr, colPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_value --
 *
 *	Sets a scalar Tcl_Obj value in the table at the designated row and
 *	column.  "Write" and possibly "create" or "unset" traces may be fired
 *	*after* the value is set.  If valuePtr is NULL, this indicates to
 *	unset the old value.
 *
 * Results:
 *	A standard TCL result.  Returns TCL_OK if successful setting the value
 *	at the table location.  If an error occurs, TCL_ERROR is returned and
 *	an error message is left in the interpreter.
 *
 * -------------------------------------------------------------------------- 
 */
int
blt_table_set_value(Table *tablePtr, Row *rowPtr, Column *colPtr, Value *newPtr)
{
    Value *valuePtr;
    int flags;

    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    flags = TABLE_TRACE_WRITES;
    if (IsEmpty(newPtr)) {		/* New value is empty. Effectively
					 * unsetting the value. */
	flags |= TABLE_TRACE_UNSETS;
    } else if (IsEmpty(valuePtr)) {
	flags |= TABLE_TRACE_CREATES;	/* Old value was empty. */
    } 
    FreeValue(valuePtr);
    *valuePtr = *newPtr;		/* Copy the value. */
    if (newPtr->string != NULL) {
	valuePtr->string = Blt_AssertStrdup(newPtr->string);
    }
    CallTraces(tablePtr, rowPtr, colPtr, flags);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_obj --
 *
 *	Gets a scalar Tcl_Obj value from the table at the designated row,
 *	column location.  "Read" traces may be fired *before* the value is
 *	retrieved.  If no value exists at that location, *objPtrPtr is set to
 *	NULL.
 *
 * Results:
 *	A standard TCL result.  Returns TCL_OK if successful accessing the
 *	table location.  If an error occurs, TCL_ERROR is returned and an
 *	error message is left in the interpreter.
 *
 * -------------------------------------------------------------------------- 
 */
Tcl_Obj *
blt_table_get_obj(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    Value *valuePtr;
    Tcl_Obj *objPtr;

    CallTraces(tablePtr, rowPtr, colPtr, TABLE_TRACE_READS);
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    if (IsEmpty(valuePtr)) {
	return NULL;
    }
    objPtr = GetObjFromValue(tablePtr->interp, colPtr->type, valuePtr);
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_obj --
 *
 *	Sets a scalar Tcl_Obj value in the table at the designated row and
 *	column.  "Write" and possibly "create" or "unset" traces may be fired
 *	*after* the value is set.  If valueObjPtr is NULL, this indicates to
 *	unset the old value.
 *
 * Results:
 *	A standard TCL result.  Returns TCL_OK if successful setting the value
 *	at the table location.  If an error occurs, TCL_ERROR is returned and
 *	an error message is left in the interpreter.
 *
 * -------------------------------------------------------------------------- 
 */
int
blt_table_set_obj(Table *tablePtr, Row *rowPtr, Column *colPtr, 
		 Tcl_Obj *objPtr)
{
    unsigned int flags;
    Value *valuePtr;

    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    flags = TABLE_TRACE_WRITES;
    if (objPtr == NULL) {		/* New value is empty. Effectively
					 * unsetting the value. */
	flags |= TABLE_TRACE_UNSETS;
    } else if (IsEmpty(valuePtr)) {
	flags |= TABLE_TRACE_CREATES;
    } 
    if (SetValueFromObj(tablePtr->interp, colPtr->type, objPtr, valuePtr) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    CallTraces(tablePtr, rowPtr, colPtr, flags);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_unset_value --
 *
 *	Unsets a scalar Tcl_Obj value in the table at the designated row,
 *	column location.  It's okay is there is presently no value at the
 *	location. Unset traces may be fired *before* the value is unset.
 *
 * Results:
 *	A standard TCL result.  Returns TCL_OK if successful unsetting the
 *	value at the table location.  If an error occurs, TCL_ERROR is
 *	returned and an error message is left in the interpreter.
 *
 * -------------------------------------------------------------------------- 
 */
int
blt_table_unset_value(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    Value *valuePtr;

    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    if (!IsEmpty(valuePtr)) {
	CallTraces(tablePtr, rowPtr, colPtr, TABLE_TRACE_UNSETS);
	/* Indicate the keytables need to be regenerated. */
	if (colPtr->flags & TABLE_COLUMN_PRIMARY_KEY) {
	    tablePtr->flags |= TABLE_KEYS_DIRTY;
	}
	FreeValue(valuePtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_create --
 *
 *	Creates a table object by the designated name.  It's an error if a
 *	table object already exists by that name.
 *
 * Results:
 *	A standard TCL result.  If successful, a new table object is created
 *	and TCL_OK is returned.  If an object already exists or the table
 *	object can't be allocated, then TCL_ERROR is returned and an error
 *	message is left in the interpreter.
 *
 * Side Effects:
 *	A new table object is created.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_create(
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    const char *name,			/* Name of tuple in namespace.  Object
					 * must not already exist. */
    Table **tablePtrPtr)		/* (out) Client token of newly created
					 * table object.  Releasing the token
					 * will free the tuple.  If NULL, no
					 * token is generated. */
{
    InterpData *dataPtr;
    TableObject *corePtr;
    Blt_ObjectName objName;
    Table *newClientPtr;
    Tcl_DString ds;
    const char *qualName;
    char string[200];

    dataPtr = GetInterpData(interp);
    if (name != NULL) {
	/* Check if a client by this name already exist in the current
	 * namespace. */
	if (GetTable(dataPtr, name, NS_SEARCH_CURRENT) != NULL) {
	    Tcl_AppendResult(interp, "a table object \"", name,
		"\" already exists", (char *)NULL);
	    return TCL_ERROR;
	}
    } else {
	/* Generate a unique name in the current namespace. */
	do  {
	    Blt_FormatString(string, 200, "datatable%d", dataPtr->nextId++);
	} while (GetTable(dataPtr, name, NS_SEARCH_CURRENT) != NULL);
	name = string;
    } 
    /* 
     * Tear apart and put back together the namespace-qualified name of the
     * object.  This is to ensure that naming is consistent.
     */ 
    if (!Blt_ParseObjectName(interp, name, &objName, 0)) {
	return TCL_ERROR;
    }
    corePtr = NewTableObject();
    if (corePtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate table object.", (char *)NULL);
	Tcl_DStringFree(&ds);
	return TCL_ERROR;
    }
    qualName = Blt_MakeQualifiedName(&objName, &ds);
    newClientPtr = NewTable(dataPtr, corePtr, qualName);
    Tcl_DStringFree(&ds);
    if (newClientPtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate table token", (char *)NULL);
	return TCL_ERROR;
    }
    
    if (tablePtrPtr != NULL) {
	*tablePtrPtr = newClientPtr;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_open --
 *
 *	Allocates a token for the table object designated by name.  It's an
 *	error if no table object exists by that name.  The token returned is
 *	passed to various routines to manipulate the object.  Traces and event
 *	notifications are also made through the token.
 *
 * Results:
 *	A new token is returned representing the table object.  
 *
 * Side Effects:
 *	If this is the remaining client, then the table object itself is
 *	freed.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_open(
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    const char *name,			/* Name of table object in
					 * namespace. */
    Table **tablePtrPtr)
{
    Table *tablePtr, *newClientPtr;
    InterpData *dataPtr;

    dataPtr = GetInterpData(interp);
    tablePtr = GetTable(dataPtr, name, NS_SEARCH_BOTH);
    if ((tablePtr == NULL) || (tablePtr->corePtr == NULL)) {
	Tcl_AppendResult(interp, "can't find a table object \"", name, "\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    newClientPtr = NewTable(dataPtr, tablePtr->corePtr, name);
    if (newClientPtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate token for table \"", name, 
		"\"", (char *)NULL);
	return TCL_ERROR;
    }
    *tablePtrPtr = newClientPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_close --
 *
 *	Releases the tuple token, indicating this the client is no longer
 *	using the object. The client is removed from the tuple object's client
 *	list.  If this is the last client, then the object itself is destroyed
 *	and memory is freed.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	If this is the remaining client, then the table object itself
 *	is freed.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_close(Table *tablePtr)
{
    if (tablePtr->magic != TABLE_MAGIC) {
	Blt_Warn("invalid table object token 0x%lx\n", 
		 (unsigned long)tablePtr);
	return;
    }
    if (tablePtr->link2 != NULL) {
	Blt_Chain chain;

	/* Remove the client from the list of clients using the table name.
	 * This is different from the table core. */
	chain = Blt_GetHashValue(tablePtr->hPtr);
	Blt_Chain_DeleteLink(chain, tablePtr->link2);
	if (Blt_Chain_GetLength(chain) == 0) {
	    /* If no more clients are using this name, then remove it from the
	     * interpreter-specific hash table. */	
	    Blt_DeleteHashEntry(tablePtr->clientTablePtr, tablePtr->hPtr);
	}
    }
    DestroyClient(tablePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_exists --
 *
 *	Indicates if a table object by the given name exists in either the
 *	current or global namespace.
 *
 * Results:
 *	Returns 1 if a table object exists and 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_exists(Tcl_Interp *interp, const char *name)
{
    InterpData *dataPtr;

    dataPtr = GetInterpData(interp);
    return (GetTable(dataPtr, name, NS_SEARCH_BOTH) != NULL);
}

static Notifier *
CreateNotifier(Tcl_Interp *interp, Blt_Chain chain, unsigned int mask,
	       BLT_TABLE_NOTIFY_EVENT_PROC *proc, 
	       BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, 
	       ClientData clientData)
{
    Notifier *notifierPtr;

    notifierPtr = Blt_AssertMalloc(sizeof (Notifier));
    notifierPtr->proc = proc;
    notifierPtr->deleteProc = deleteProc;
    notifierPtr->chain = chain;
    notifierPtr->clientData = clientData;
    notifierPtr->column = NULL;		/* All columns. */
    notifierPtr->row = NULL;		/* All rows. */
    notifierPtr->tag = NULL;		/* No tag. */
    notifierPtr->flags = mask | TABLE_NOTIFY_COLUMN | TABLE_NOTIFY_ROW;
    notifierPtr->interp = interp;
    notifierPtr->link = Blt_Chain_Append(chain, notifierPtr);
    return notifierPtr;
}

static Notifier *
CreateNotifierForRows(Tcl_Interp *interp, Blt_Chain chain, unsigned int mask,
		      Row *rowPtr, const char *tag, 
		      BLT_TABLE_NOTIFY_EVENT_PROC *proc,
		      BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, 
		      ClientData clientData)
{
    Notifier *notifierPtr;

    notifierPtr = Blt_AssertMalloc(sizeof (Notifier));
    notifierPtr->proc = proc;
    notifierPtr->deleteProc = deleteProc;
    notifierPtr->chain = chain;
    notifierPtr->clientData = clientData;
    notifierPtr->row = rowPtr;
    notifierPtr->column = NULL;
    notifierPtr->tag = (tag != NULL) ? Blt_AssertStrdup(tag) : NULL;
    notifierPtr->flags = mask | TABLE_NOTIFY_ROW;
    notifierPtr->interp = interp;
    notifierPtr->link = Blt_Chain_Append(chain, notifierPtr);
    return notifierPtr;
}

static Notifier *
CreateNotifierForColumns(Tcl_Interp *interp, Blt_Chain chain, unsigned int mask,
	Column *colPtr, const char *tag, BLT_TABLE_NOTIFY_EVENT_PROC *proc,
	BLT_TABLE_NOTIFIER_DELETE_PROC *deleteProc, ClientData clientData)
{
    Notifier *notifierPtr;

    notifierPtr = Blt_AssertMalloc(sizeof (Notifier));
    notifierPtr->proc = proc;
    notifierPtr->deleteProc = deleteProc;
    notifierPtr->chain = chain;
    notifierPtr->clientData = clientData;
    notifierPtr->column = colPtr;
    notifierPtr->row = NULL;
    notifierPtr->tag = (tag != NULL) ? Blt_AssertStrdup(tag) : NULL;
    notifierPtr->flags = mask | TABLE_NOTIFY_COLUMN;
    notifierPtr->interp = interp;
    notifierPtr->link = Blt_Chain_Append(chain, notifierPtr);
    return notifierPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_create_notifier --
 *
 *	Creates an event handler using the following three pieces of
 *	information: 
 *		1. C function pointer, 
 *		2. one-word of data passed on each call, and 
 *		3. event mask indicating which events are of interest.  
 *	If an event already exists matching all of the above criteria,
 *	it is repositioned on the end of the event handler list.  This
 *	means that it will be the last to fire.
 *
 * Results:
 *      Returns a pointer to the event handler.
 *
 * Side Effects:
 *	Memory for the event handler is possibly allocated.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_NOTIFIER
blt_table_create_notifier(Tcl_Interp *interp, Table *tablePtr, 
			  unsigned int mask, BLT_TABLE_NOTIFY_EVENT_PROC *proc,
			  BLT_TABLE_NOTIFIER_DELETE_PROC *deletedProc,
			  ClientData clientData)
{
    return CreateNotifier(interp, tablePtr->columnNotifiers, mask,  proc, 
			  deletedProc, clientData);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_create_column_notifier --
 *
 *	Creates an event handler using the following three pieces of
 *	information: 
 *		1. C function pointer, 
 *		2. one-word of data passed on each call, and 
 *		3. event mask indicating which events are of interest.  
 *	If an event already exists matching all of the above criteria,
 *	it is repositioned on the end of the event handler list.  This
 *	means that it will be the last to fire.
 *
 * Results:
 *      Returns a pointer to the event handler.
 *
 * Side Effects:
 *	Memory for the event handler is possibly allocated.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_NOTIFIER
blt_table_create_column_notifier(Tcl_Interp *interp, Table *tablePtr,
			       BLT_TABLE_COLUMN col, unsigned int mask,
			       BLT_TABLE_NOTIFY_EVENT_PROC *proc,
			       BLT_TABLE_NOTIFIER_DELETE_PROC *deletedProc,
			       ClientData clientData)
{
    return CreateNotifierForColumns(interp, tablePtr->columnNotifiers, mask, 
		col, NULL, proc, deletedProc, clientData);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_create_column_tag_notifier --
 *
 *	Creates an event handler using the following three pieces of
 *	information: 
 *		1. C function pointer, 
 *		2. one-word of data passed on each call, and 
 *		3. event mask indicating which events are of interest.  
 *	If an event already exists matching all of the above criteria,
 *	it is repositioned on the end of the event handler list.  This
 *	means that it will be the last to fire.
 *
 * Results:
 *      Returns a pointer to the event handler.
 *
 * Side Effects:
 *	Memory for the event handler is possibly allocated.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_NOTIFIER
blt_table_create_column_tag_notifier(Tcl_Interp *interp, Table *tablePtr,
				  const char *tag, unsigned int mask,
				  BLT_TABLE_NOTIFY_EVENT_PROC *proc,
				  BLT_TABLE_NOTIFIER_DELETE_PROC *deletedProc,
				  ClientData clientData)
{
    return CreateNotifierForColumns(interp, tablePtr->columnNotifiers, mask, 
	NULL, tag, proc, deletedProc, clientData);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_create_row_notifier --
 *
 *	Creates an event handler using the following three pieces of
 *	information: 
 *		1. C function pointer, 
 *		2. one-word of data passed on each call, and 
 *		3. event mask indicating which events are of interest.  
 *	If an event already exists matching all of the above criteria,
 *	it is repositioned on the end of the event handler list.  This
 *	means that it will be the last to fire.
 *
 * Results:
 *      Returns a pointer to the event handler.
 *
 * Side Effects:
 *	Memory for the event handler is possibly allocated.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_NOTIFIER
blt_table_create_row_notifier(Tcl_Interp *interp, Table *tablePtr, 
			    BLT_TABLE_ROW row, unsigned int mask,
			    BLT_TABLE_NOTIFY_EVENT_PROC *proc,
			    BLT_TABLE_NOTIFIER_DELETE_PROC *deletedProc,
			    ClientData clientData)
{
    return CreateNotifierForRows(interp, tablePtr->rowNotifiers, mask, row, 
	NULL, proc, deletedProc, clientData);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_create_column_tag_notifier --
 *
 *	Creates an event handler using the following three pieces of
 *	information: 
 *		1. C function pointer, 
 *		2. one-word of data passed on each call, and 
 *		3. event mask indicating which events are of interest.  
 *	If an event already exists matching all of the above criteria,
 *	it is repositioned on the end of the event handler list.  This
 *	means that it will be the last to fire.
 *
 * Results:
 *      Returns a pointer to the event handler.
 *
 * Side Effects:
 *	Memory for the event handler is possibly allocated.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_NOTIFIER
blt_table_create_row_tag_notifier(Tcl_Interp *interp, Table *tablePtr,
			       const  char *tag, unsigned int mask,
			       BLT_TABLE_NOTIFY_EVENT_PROC *proc,
			       BLT_TABLE_NOTIFIER_DELETE_PROC *deletedProc,
			       ClientData clientData)
{
    return CreateNotifierForRows(interp, tablePtr->rowNotifiers, mask, NULL, 
	tag, proc, deletedProc, clientData);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_delete_notifier --
 *
 *	Removes the event handler designated by following three pieces
 *	of information: 
 *	   1. C function pointer, 
 *	   2. one-word of data passed on each call, and 
 *	   3. event mask indicating which events are of interest.
 *
 * Results:
 *      Nothing.
 *
 * Side Effects:
 *	Memory for the event handler is freed.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_delete_notifier(Table *tablePtr, Notifier *notifierPtr)
{
    /* Check if notifier is already being deleted. */
    if ((notifierPtr->flags & TABLE_NOTIFY_DESTROYED) == 0) {
	if (notifierPtr->deleteProc != NULL) {
	    (*notifierPtr->deleteProc)(notifierPtr->clientData);
	}
	if (notifierPtr->flags & TABLE_NOTIFY_PENDING) {
	    Tcl_CancelIdleCall(NotifyIdleProc, notifierPtr);
	}
	notifierPtr->flags = TABLE_NOTIFY_DESTROYED;
	Tcl_EventuallyFree(notifierPtr, (Tcl_FreeProc *)FreeNotifier);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_row_by_label --
 *
 *	Returns the offset of the row given its label.  If the row label is
 *	invalid, then -1 is returned.
 *
 * Results:
 *	Returns the offset of the row or -1 if not found.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_ROW
blt_table_get_row_by_label(Table *tablePtr, const char *label)
{
    return (BLT_TABLE_ROW)FindLabel(&tablePtr->corePtr->rows, label);
}


/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_column_by_label --
 *
 *	Returns the offset of the column given its label.  If the column label
 *	is invalid, then -1 is returned.
 *
 * Results:
 *	Returns the offset of the column or -1 if not found.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_COLUMN
blt_table_get_column_by_label(Table *tablePtr, const char *label)
{
    return (BLT_TABLE_COLUMN)FindLabel(&tablePtr->corePtr->columns, label);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_column_get_label_table --
 *
 *	Returns the offset of the column given its label.  If the column label
 *	is invalid, then -1 is returned.
 *
 * Results:
 *	Returns the offset of the column or -1 if not found.
 *
 *---------------------------------------------------------------------------
 */
Blt_HashTable *
blt_table_column_get_label_table(Table *tablePtr, const char *label)
{
    return GetLabelTable(&tablePtr->corePtr->columns, label);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_row_get_label_table --
 *
 *	Returns the offset of the column given its label.  If the column label
 *	is invalid, then -1 is returned.
 *
 * Results:
 *	Returns the offset of the column or -1 if not found.
 *
 *---------------------------------------------------------------------------
 */
Blt_HashTable *
blt_table_row_get_label_table(Table *tablePtr, const char *label)
{
    return GetLabelTable(&tablePtr->corePtr->rows, label);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_row_label --
 *
 *	Returns the label of the row.  If the row offset is invalid or the row
 *	has no label, then NULL is returned.
 *
 * Results:
 *	Returns the label of the row.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_row_label(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr, 
			const char *label)
{
    BLT_TABLE_NOTIFY_EVENT event;
	
    InitNotifyEvent(tablePtr, &event);
    event.type = TABLE_NOTIFY_RELABEL;
    event.row = rowPtr;
    if (SetHeaderLabel(interp, &tablePtr->corePtr->rows, (Header *)rowPtr,
	label) != TCL_OK) {
	return TCL_ERROR;
    }
    NotifyClients(tablePtr, &event);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_column_label --
 *
 *	Sets the label of the column.  If the column offset is invalid, then
 *	no label is set.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_column_label(Tcl_Interp *interp, Table *tablePtr, Column *colPtr, 
			 const char *label)
{
    BLT_TABLE_NOTIFY_EVENT event;

    if (SetHeaderLabel(interp, &tablePtr->corePtr->columns, (Header *)colPtr,
	label) != TCL_OK) {
	return TCL_ERROR;
    }

    InitNotifyEvent(tablePtr, &event);
    event.type = TABLE_NOTIFY_COLUMNS_RELABEL;
    event.column = colPtr;
    NotifyClients(tablePtr, &event);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_column_type --
 *
 *	Sets the type of the given column.  
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_column_type(Table *tablePtr, Column *colPtr, 
			  BLT_TABLE_COLUMN_TYPE type)
{
    return SetType(tablePtr, colPtr, type);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_value_exists --
 *
 *	Indicates if a value exists for a given row,column offset in the
 *	tuple.  Note that this routine does not fire read traces.
 *
 * Results:
 *	Returns 1 is a value exists, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_value_exists(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    if ((colPtr == NULL) || (rowPtr == NULL)) {
	return 0;
    }
    return !IsEmpty(GetValue(tablePtr, rowPtr, colPtr));
}


/*
 *---------------------------------------------------------------------------
 *
 * blt_table_extend_rows --
 *
 *	Adds new rows to the table.  Rows are slots in an array of Rows.  The
 *	array grows by doubling its size, so there may be more slots than
 *	needed (# rows).
 *
 * Results:
 *	Returns TCL_OK is the tuple is resized and TCL_ERROR if an not enough
 *	memory was available.
 *
 * Side Effects:
 *	If more rows are needed, the array which holds the tuples is
 *	reallocated by doubling its size.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_extend_rows(Tcl_Interp *interp, Table *tablePtr, size_t numExtra, 
		      Row **rows)
{
    long i;
    Blt_Chain chain;
    Blt_ChainLink link;

    if (numExtra == 0) {
	return TCL_OK;
    }
    chain = Blt_Chain_Create();
    if (!ExtendRows(tablePtr, numExtra, chain)) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't extend table by ", 
		Blt_Ltoa(numExtra), " rows: out of memory.", (char *)NULL);
	}
	Blt_Chain_Destroy(chain);
	return TCL_ERROR;
    }
    for (i = 0, link = Blt_Chain_FirstLink(chain); link != NULL; 
	 link = Blt_Chain_NextLink(link), i++) {
	BLT_TABLE_ROW row;

	row = Blt_Chain_GetValue(link);
	if (rows != NULL) {
	    rows[i] = row;
	}
    }
    assert(Blt_Chain_GetLength(chain) > 0);
    NotifyRowChanged(tablePtr, NULL, TABLE_NOTIFY_ROWS_CREATED);
    Blt_Chain_Destroy(chain);
    return TCL_OK;
}

int
blt_table_delete_row(Table *tablePtr, Row *rowPtr)
{
    DeleteHeader(&tablePtr->corePtr->rows, (Header *)rowPtr);
    UnsetRowValues(tablePtr, rowPtr);
    NotifyRowChanged(tablePtr, rowPtr, TABLE_NOTIFY_ROWS_DELETED);
    NotifyColumnChanged(tablePtr, NULL, TABLE_NOTIFY_COLUMNS_DELETED);
    blt_table_clear_row_tags(tablePtr, rowPtr);
    blt_table_clear_row_traces(tablePtr, rowPtr);
    ClearRowNotifiers(tablePtr, rowPtr);
    tablePtr->flags |= TABLE_KEYS_DIRTY;
    return TCL_OK;
}

BLT_TABLE_ROW
blt_table_create_row(Tcl_Interp *interp, BLT_TABLE table, const char *label)
{
    Row *rowPtr;

    if (blt_table_extend_rows(interp, table, 1, &rowPtr) != TCL_OK) {
	return NULL;
    }
    if (label != NULL) {
	if (blt_table_set_row_label(interp, table, rowPtr, label) != TCL_OK) {
	    blt_table_delete_row(table, rowPtr);
	    return NULL;
	}
    }
    return rowPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_move_row --
 *
 *	Move one of more rows to a new location in the tuple.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
blt_table_move_row(Tcl_Interp *interp, Table *tablePtr, Row *srcPtr, 
		   Row *destPtr, size_t count)
{
    if (srcPtr == destPtr) {
	return TCL_OK;		/* Move to the same location. */
    }
    if (!MoveIndices(&tablePtr->corePtr->rows, (Header *)srcPtr, 
		     (Header *)destPtr, count)) {
	Tcl_AppendResult(interp, "can't allocate new map for \"", 
		blt_table_name(tablePtr), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    NotifyRowChanged(tablePtr, NULL, TABLE_NOTIFY_ROWS_MOVED);
    return TCL_OK;
}

void
blt_table_set_row_map(Table *tablePtr, Row **map)
{
    ReplaceMap(&tablePtr->corePtr->rows, (Header **)map);
    NotifyRowChanged(tablePtr, NULL, TABLE_NOTIFY_ROWS_MOVED);
}

void
blt_table_sort_init(BLT_TABLE table, BLT_TABLE_SORT_ORDER *order, 
		    size_t numColumns, unsigned int flags)
{
    size_t i;

    sortData.table = table;
    sortData.order = order;
    sortData.numColumns = numColumns;
    sortData.flags = flags;
    Blt_InitHashTable(&sortData.freqTable, BLT_STRING_KEYS);
    for (i = 0; i < numColumns; i++) {
	BLT_TABLE_SORT_ORDER *sortPtr;

	sortPtr = order + i;
	sortPtr->clientData = table;
	sortPtr->cmpProc = blt_table_get_compare_proc(table, sortPtr->column, 
		flags);
    }
    if (flags & TABLE_SORT_FREQUENCY) {
	BLT_TABLE_COLUMN col;
	long i;

	col = order[0].column;
	for (i = 0; i < blt_table_num_rows(table); i++) {
	    BLT_TABLE_ROW row;
	    Blt_HashEntry *hPtr;
	    const char *string;
	    int isNew;
	    long refCount;

	    row = blt_table_row(table, i);
	    string = blt_table_get_string(table, row, col);
	    hPtr = Blt_CreateHashEntry(&sortData.freqTable, string, &isNew);
	    if (isNew) {
		refCount = 0;
	    } else {
		refCount = (long)Blt_GetHashValue(hPtr);
	    }
	    refCount++;
	    Blt_SetHashValue(hPtr, refCount);
	}
    }
}

void
blt_table_sort_finish()
{
    Blt_DeleteHashTable(&sortData.freqTable);
}

BLT_TABLE_ROW *
blt_table_sort_rows(Table *tablePtr)
{
    return (BLT_TABLE_ROW *)SortHeaders(&tablePtr->corePtr->rows, 
	(QSortCompareProc *)CompareRows);
}

void
blt_table_sort_rows_subset(Table *tablePtr, long numRows, BLT_TABLE_ROW *rows)
{
    /* Sort the map and return it. */
    qsort((char *)rows, numRows, sizeof(BLT_TABLE_ROW), 
	  (QSortCompareProc *)CompareRows);
}

int
blt_table_get_column_limits(Tcl_Interp *interp, Table *tablePtr, Column *colPtr,
			  Tcl_Obj **minObjPtrPtr, Tcl_Obj **maxObjPtrPtr)
{
    Row *rowPtr, *minRowPtr, *maxRowPtr;

    if (blt_table_num_rows(tablePtr) == 0) {
	return TCL_OK;
    }
    rowPtr = blt_table_first_row(tablePtr);
    maxRowPtr = minRowPtr = rowPtr;
    sortData.table = tablePtr;
    for (/*empty*/; rowPtr != NULL; 
		  rowPtr = blt_table_next_row(tablePtr, rowPtr)) {
	BLT_TABLE_COMPARE_PROC *proc;

	proc = blt_table_get_compare_proc(tablePtr, colPtr, 0);
	if ((*proc)(NULL, colPtr, rowPtr, minRowPtr) < 0) {
		minRowPtr = rowPtr;
	}
	if ((*proc)(NULL, colPtr, rowPtr, maxRowPtr) > 0) {
	    maxRowPtr = rowPtr;
	}
    }
    *minObjPtrPtr = blt_table_get_obj(tablePtr, minRowPtr, colPtr);
    *maxObjPtrPtr = blt_table_get_obj(tablePtr, maxRowPtr, colPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_delete_column --
 *
 *	Remove the designated column from the table.  The actual space
 *	contained by the column isn't freed.  The map is compressed.  Tcl_Objs
 *	stored as column values are released.  Traces and tags associated with
 *	the column are removed.
 *
 * Side Effects:
 *	Traces may fire when column values are unset.  Also notifier events
 *	may be triggered, indicating the column has been deleted.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_delete_column(Table *tablePtr, Column *colPtr)
{
    /* If the deleted column is a primary key, the generated keytables
     * are now invalid. So remove them. */
    if (colPtr->flags & TABLE_COLUMN_PRIMARY_KEY) {
	blt_table_unset_keys(tablePtr);
    }
    UnsetColumnValues(tablePtr, colPtr);
    NotifyColumnChanged(tablePtr, colPtr, TABLE_NOTIFY_COLUMNS_DELETED);
    NotifyRowChanged(tablePtr, NULL, TABLE_NOTIFY_ROWS_DELETED);
    blt_table_clear_column_traces(tablePtr, colPtr);
    blt_table_clear_column_tags(tablePtr, colPtr);
    ClearColumnNotifiers(tablePtr, colPtr);
    DeleteHeader(&tablePtr->corePtr->columns, (Header *)colPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_extend_columns --
 *
 *	Adds new columns to the table.  Columns are slots in an array of
 *	Columns.  The array columns by doubling its size, so there may be more
 *	slots than needed (# columns).
 *
 * Results:
 *	Returns TCL_OK is the tuple is resized and TCL_ERROR if an
 *	not enough memory was available.
 *
 * Side Effects:
 *	If more columns are needed, the array which holds the tuples is
 *	reallocated by doubling its size.  
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_extend_columns(Tcl_Interp *interp, BLT_TABLE table, size_t numExtra, 
			Column **columns)
{
    size_t i;
    Blt_Chain chain;
    Blt_ChainLink link;

    chain = Blt_Chain_Create();
    if (!ExtendColumns(table, numExtra, chain)) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't extend table by ", 
		Blt_Ltoa(numExtra), " columns: out of memory.", (char *)NULL);
	}
	Blt_Chain_Destroy(chain);
	return TCL_ERROR;
    }
    for (i = 0, link = Blt_Chain_FirstLink(chain); link != NULL; 
	 link = Blt_Chain_NextLink(link), i++) {
	Column *colPtr;

	colPtr = Blt_Chain_GetValue(link);
	if (columns != NULL) {
	    columns[i] = colPtr;
	}
	colPtr->type = TABLE_COLUMN_TYPE_STRING;
    }
    NotifyColumnChanged(table, NULL, TABLE_NOTIFY_COLUMNS_CREATED);
    Blt_Chain_Destroy(chain);
    return TCL_OK;
}

BLT_TABLE_COLUMN
blt_table_create_column(Tcl_Interp *interp, BLT_TABLE table, const char *label)
{
    Column *colPtr;

    if (blt_table_extend_columns(interp, table, 1, &colPtr) != TCL_OK) {
	return NULL;
    }
    if (label != NULL) {
	if (blt_table_set_column_label(interp, table, colPtr, label) != TCL_OK) {
	    blt_table_delete_column(table, colPtr);
	    return NULL;
	}
    }
    return colPtr;
}

void
blt_table_set_column_map(Table *tablePtr, Column **map)
{
    NotifyColumnChanged(tablePtr, NULL, TABLE_NOTIFY_COLUMNS_MOVED);
    ReplaceMap(&tablePtr->corePtr->columns, (Header **)map);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_move_column --
 *
 *	Move one of more rows to a new location in the tuple.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
blt_table_move_column(Tcl_Interp *interp, Table *tablePtr, Column *srcPtr, 
		      Column *destPtr, size_t count)
{
    if (srcPtr == destPtr) {
	return TCL_OK;		/* Move to the same location. */
    }
    if (!MoveIndices(&tablePtr->corePtr->columns, (Header *)srcPtr, 
		(Header *)destPtr, count)) {
	Tcl_AppendResult(interp, "can't allocate new map for \"", 
		blt_table_name(tablePtr), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    NotifyColumnChanged(tablePtr, NULL, TABLE_NOTIFY_COLUMNS_MOVED);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_restore --
 *
 *	Restores data to the given table based upon the dump string.
 *	The dump string should have been generated by blt_table_dump.
 *	Two bit flags may be set.
 *	
 *	TABLE_RESTORE_NO_TAGS	Don't restore tag information.
 *	TABLE_RESTORE_OVERWRITE	Look for row and columns with the 
 *				same label. Overwrite if necessary.
 *
 * Results:
 *	A standard TCL result.  If the restore was successful, TCL_OK
 *	is returned.  Otherwise, TCL_ERROR is returned and an error
 *	message is left in the interpreter result.
 *
 * Side Effects:
 *	New row and columns are created in the table and may possibly
 *	generate event notifier or trace callbacks.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_restore(Tcl_Interp *interp, BLT_TABLE table, char *data, 
		      unsigned int flags)
{
    RestoreData restore;
    int result;

    restore.argc = 0;
    restore.mtime = restore.ctime = 0L;
    restore.argv = NULL;
    restore.fileName = "data string";
    restore.numLines = 0;
    restore.flags = flags;
    restore.numCols = blt_table_num_columns(table);
    restore.numRows = blt_table_num_rows(table);
    Blt_InitHashTableWithPool(&restore.rowIndices, BLT_ONE_WORD_KEYS);
    Blt_InitHashTableWithPool(&restore.colIndices, BLT_ONE_WORD_KEYS);
    result = TCL_ERROR;		
    /* Read dump information */
    for (;;) {
	char c1, c2;

	result = ParseDumpRecord(interp, &data, &restore);
	if (result != TCL_OK) {
	    break;
	}
	if (restore.argc == 0) {
	    continue;
	}
	c1 = restore.argv[0][0], c2 = restore.argv[0][1];
	if ((c1 == 'i') && (c2 == '\0')) {
	    result = RestoreHeader(interp, table, &restore);
	} else if ((c1 == 'r') && (c2 == '\0')) {
	    result = RestoreRow(interp, table, &restore);
	} else if ((c1 == 'c') && (c2 == '\0')) {
	    result = RestoreColumn(interp, table, &restore);
	} else if ((c1 == 'd') && (c2 == '\0')) {
	    result = RestoreValue(interp, table, &restore);
	} else {
	    Tcl_AppendResult(interp, restore.fileName, ":", 
		Blt_Ltoa(restore.numLines), ": error: unknown entry \"", 
		restore.argv[0], "\"", (char *)NULL);
	    result = TCL_ERROR;
	}
	Blt_Free(restore.argv);
	if (result != TCL_OK) {
	    break;
	}
    }
    Blt_DeleteHashTable(&restore.rowIndices);
    Blt_DeleteHashTable(&restore.colIndices);
    if (result == TCL_ERROR) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_file_restore --
 *
 *	Restores data to the given table based upon the dump file
 *	provided. The dump file should have been generated by
 *	blt_table_dump or blt_table_file_dump.  
 *
 *	If the filename starts with an '@', then it is the name of an
 *	already opened channel to be used. Two bit flags may be set.
 *	
 *	TABLE_RESTORE_NO_TAGS	Don't restore tag information.
 *	TABLE_RESTORE_OVERWRITE	Look for row and columns with 
 *				the same label. Overwrite if necessary.
 *
 * Results:
 *	A standard TCL result.  If the restore was successful, TCL_OK
 *	is returned.  Otherwise, TCL_ERROR is returned and an error
 *	message is left in the interpreter result.
 *
 * Side Effects:
 *	Row and columns are created in the table and may possibly
 *	generate trace or notifier event callbacks.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_file_restore(Tcl_Interp *interp, BLT_TABLE table, const char *fileName,
		      unsigned int flags)
{
    RestoreData restore;
    Tcl_Channel channel;
    int closeChannel;
    int result;

    closeChannel = TRUE;
    if ((fileName[0] == '@') && (fileName[1] != '\0')) {
	int mode;
	
	channel = Tcl_GetChannel(interp, fileName+1, &mode);
	if (channel == NULL) {
	    return TCL_ERROR;
	}
	if ((mode & TCL_READABLE) == 0) {
	    Tcl_AppendResult(interp, "channel \"", fileName, 
		"\" not opened for reading", (char *)NULL);
	    return TCL_ERROR;
	}
	closeChannel = FALSE;
    } else {
	channel = Tcl_OpenFileChannel(interp, fileName, "r", 0);
	if (channel == NULL) {
	    return TCL_ERROR;	/* Can't open dump file. */
	}
    }
    restore.argc = 0;
    restore.mtime = restore.ctime = 0L;
    restore.argv = NULL;
    restore.fileName = fileName;
    restore.numLines = 0;
    restore.flags = flags;
    restore.numCols = blt_table_num_columns(table);
    restore.numRows = blt_table_num_rows(table);
    Blt_InitHashTableWithPool(&restore.rowIndices, BLT_ONE_WORD_KEYS);
    Blt_InitHashTableWithPool(&restore.colIndices, BLT_ONE_WORD_KEYS);

    /* Process dump information record by record. */
    result = TCL_ERROR;		
    for (;;) {
	char c1, c2;

	result = ReadDumpRecord(interp, channel, &restore);
	if (result != TCL_OK) {
	    break;
	}
	if (restore.argc == 0) {
	    continue;
	}
	c1 = restore.argv[0][0], c2 = restore.argv[0][1];
	if ((c1 == 'i') && (c2 == '\0')) {
	    result = RestoreHeader(interp, table, &restore);
	} else if ((c1 == 'r') && (c2 == '\0')) {
	    result = RestoreRow(interp, table, &restore);
	} else if ((c1 == 'c') && (c2 == '\0')) {
	    result = RestoreColumn(interp, table, &restore);
	} else if ((c1 == 'd') && (c2 == '\0')) {
	    result = RestoreValue(interp, table, &restore);
	} else {
	    Tcl_AppendResult(interp, fileName, ":", Blt_Ltoa(restore.numLines), 
		": error: unknown entry \"", restore.argv[0], "\"", 
		(char *)NULL);
	    result = TCL_ERROR;
	}
	if (result != TCL_OK) {
	    Blt_Free(restore.argv);
	    break;
	}
	Blt_Free(restore.argv);
    }
    Blt_DeleteHashTable(&restore.rowIndices);
    Blt_DeleteHashTable(&restore.colIndices);
    if (closeChannel) {
	Tcl_Close(interp, channel);
    }
    if (result == TCL_ERROR) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static void
FreePrimaryKeys(Table *tablePtr)
{
    Blt_ChainLink link;
    
    for (link = Blt_Chain_FirstLink(tablePtr->primaryKeys); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
	Column *columnPtr;
	
	columnPtr = Blt_Chain_GetValue(link);
	columnPtr->flags &= ~TABLE_COLUMN_PRIMARY_KEY;
    }
    Blt_Chain_Destroy(tablePtr->primaryKeys);
    tablePtr->primaryKeys = NULL;
}

static void
FreeKeyTables(Table *tablePtr)
{
    long i;

    for (i = 0; i < tablePtr->numKeys; i++) {
	Blt_DeleteHashTable(tablePtr->keyTables + i);
    }
    if (tablePtr->keyTables != NULL) {
	Blt_Free(tablePtr->keyTables);
    }
    if (tablePtr->masterKey != NULL) {
	Blt_Free(tablePtr->masterKey);
	Blt_DeleteHashTable(&tablePtr->masterKeyTable);
    }
    tablePtr->keyTables = NULL;
    tablePtr->numKeys = 0;
    tablePtr->masterKey = NULL;
}

void
blt_table_unset_keys(Table *tablePtr)
{
    FreeKeyTables(tablePtr);
    FreePrimaryKeys(tablePtr);
    tablePtr->flags &= ~(TABLE_KEYS_DIRTY | TABLE_KEYS_UNIQUE);
}

Blt_Chain 
blt_table_get_keys(Table *tablePtr)
{
    return tablePtr->primaryKeys;
}

int
blt_table_set_keys(Table *tablePtr, Blt_Chain primaryKeys, int unique)
{
    Blt_ChainLink link;

    if (tablePtr->primaryKeys != NULL) {
	FreePrimaryKeys(tablePtr);
    }
    tablePtr->primaryKeys = primaryKeys;

    /* Mark the designated columns as primary keys.  This flag is used to
     * check if a primary column is deleted, it's rows are added or changed,
     * or it's values set or unset.  The generated keytables are invalid and
     * need to be regenerated. */
    for (link = Blt_Chain_FirstLink(tablePtr->primaryKeys); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	Column *columnPtr;
	
	columnPtr = Blt_Chain_GetValue(link);
	columnPtr->flags |= TABLE_COLUMN_PRIMARY_KEY;
    }
    tablePtr->flags |= TABLE_KEYS_DIRTY;
    if (unique) {
	tablePtr->flags |= TABLE_KEYS_UNIQUE;
    }
    return TCL_OK;
}

static int
MakeKeyTables(Tcl_Interp *interp, Table *tablePtr)
{
    long i;
    size_t masterKeySize;
    long numKeys;

    FreeKeyTables(tablePtr);
    tablePtr->flags &= ~TABLE_KEYS_DIRTY;

    numKeys = Blt_Chain_GetLength(tablePtr->primaryKeys);

    /* Create a hashtable for each key. */
    tablePtr->keyTables = Blt_Malloc(sizeof(Blt_HashTable) * numKeys);
    if (tablePtr->keyTables == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't allocated keytables for ",
		Blt_Itoa(numKeys), " keys.", (char *)NULL);
	}
	return TCL_ERROR;
    }
    tablePtr->numKeys = numKeys;
    for (i = 0; i < numKeys; i++) {
	Blt_InitHashTable(tablePtr->keyTables + i, BLT_STRING_KEYS);
    }
    masterKeySize = sizeof(BLT_TABLE_ROW) * numKeys;
    tablePtr->masterKey = Blt_AssertMalloc(masterKeySize);
    Blt_InitHashTable(&tablePtr->masterKeyTable, masterKeySize / sizeof(int));

    /* For each row, create hash entries the the individual key columns, but
     * also for the combined keys for the row.  The hash of the combined keys
     * must be unique. */
    for (i = 0; i < blt_table_num_rows(tablePtr); i++) {
	Blt_ChainLink link;
	Row *rowPtr;
	long j;

	rowPtr = blt_table_row(tablePtr, i);
	for (j = 0, link = Blt_Chain_FirstLink(tablePtr->primaryKeys); 
	     link != NULL; link = Blt_Chain_NextLink(link), j++) {
	    Column *colPtr;
	    Blt_HashEntry *hPtr;
	    int isNew;
	    Value *valuePtr;

	    colPtr = Blt_Chain_GetValue(link);
	    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
	    if (IsEmpty(valuePtr)) {
		break;		/* Skip this row since one of the key values
				 * is empty. */
	    }
	    hPtr = Blt_CreateHashEntry(tablePtr->keyTables + j, 
				       valuePtr->string, &isNew);
	    if (isNew) {
		Blt_SetHashValue(hPtr, rowPtr);
	    }
	    tablePtr->masterKey[j] = Blt_GetHashValue(hPtr);
	}
	if (j == numKeys) {
	    Blt_HashEntry *hPtr;
	    int isNew;

	    /* If we created all the hashkeys necessary for this row, then
	     * generate an entry for the row in the master key table. */
	    hPtr = Blt_CreateHashEntry(&tablePtr->masterKeyTable, 
		(char *)tablePtr->masterKey, &isNew);
	    if (isNew) {
		Blt_SetHashValue(hPtr, rowPtr);
	    } else if (tablePtr->flags & TABLE_KEYS_UNIQUE) {
		BLT_TABLE_ROW dupRow;
		
		dupRow = Blt_GetHashValue(hPtr);
		if (interp != NULL) {

		    dupRow = Blt_GetHashValue(hPtr);
		    Tcl_AppendResult(interp, "primary keys are not unique:",
			"rows \"", blt_table_row_label(dupRow), "\" and \"",
			blt_table_row_label(rowPtr), 
			"\" have the same keys.", (char *)NULL);
		}
		blt_table_unset_keys(tablePtr);
		return TCL_ERROR; /* Bail out. Keys aren't unique. */
	    }
	}
    }
    tablePtr->flags &= ~TABLE_KEYS_UNIQUE;
    return TCL_OK;
}
	    
int
blt_table_key_lookup(Tcl_Interp *interp, Table *tablePtr, int objc, 
		 Tcl_Obj *const *objv, Row **rowPtrPtr)
{
    long i;
    Blt_HashEntry *hPtr;

    *rowPtrPtr = NULL;
    if (objc != Blt_Chain_GetLength(tablePtr->primaryKeys)) {
	if (interp != NULL) {
	    Blt_ChainLink link;

	    Tcl_AppendResult(interp, "wrong # of values: should be ",
		Blt_Itoa(tablePtr->numKeys), " value(s) of ", (char *)NULL);
	    for (link = Blt_Chain_FirstLink(tablePtr->primaryKeys);
		 link != NULL; link = Blt_Chain_NextLink(link)) {
		BLT_TABLE_COLUMN col;

		col = Blt_Chain_GetValue(link);
		Tcl_AppendResult(interp, blt_table_column_label(col), " ", 
				 (char *)NULL);
	    }
	}
	return TCL_ERROR;	/* Bad number of keys supplied. */
    }
    if (tablePtr->primaryKeys == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "no primary keys designated",
			 (char *)NULL);
	}
	return TCL_ERROR;
    }
    if ((tablePtr->flags & TABLE_KEYS_DIRTY) && 
	(MakeKeyTables(interp, tablePtr) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (tablePtr->numKeys == 0) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "failed to generate key tables",
			     (char *)NULL);
	}
	return TCL_ERROR;
    }
    for (i = 0; i < tablePtr->numKeys; i++) {
	const char *string;

	string = Tcl_GetString(objv[i]);
	hPtr = Blt_FindHashEntry(tablePtr->keyTables + i, string);
	if (hPtr == NULL) {
	    return TCL_OK;	/* Can't find one of the keys, so
				 * the whole search fails. */
	}
	tablePtr->masterKey[i] = Blt_GetHashValue(hPtr);
    }
    hPtr = Blt_FindHashEntry(&tablePtr->masterKeyTable, 
			     (char *)tablePtr->masterKey);
    if (hPtr == NULL) {
	Blt_Warn("can't find master key\n");
	return TCL_OK;
    }
    *rowPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_long --
 *
 *	Sets the value of the selected row, column location in the table.  The
 *	row, column location must be within the actual table limits.
 *
 * Results:
 *	Returns the objPtr representing the old value.  If no previous value
 *	was present, the NULL is returned.
 *
 * Side Effects:
 *	New tuples may be allocated created.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_long(Table *tablePtr, Row *rowPtr, Column *colPtr, long value)
{
    Value *valuePtr;
    char string[200];

    if (colPtr->type != TABLE_COLUMN_TYPE_LONG) {
	Tcl_AppendResult(tablePtr->interp, "wrong column type \"",
		blt_table_column_type_to_name(colPtr->type), "\": should be \"int\"",
		(char *)NULL);
	return TCL_ERROR;
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    FreeValue(valuePtr);
    valuePtr->datum.l = value;
    sprintf(string, "%ld", value);
    valuePtr->string = Blt_AssertStrdup(string);

    /* Indicate the keytables need to be regenerated. */
    if (colPtr->flags & TABLE_COLUMN_PRIMARY_KEY) {
	tablePtr->flags |= TABLE_KEYS_DIRTY;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_string --
 *
 *	Sets the value of the selected row, column location in the table.  The
 *	row, column location must be within the actual table limits.
 *
 * Results:
 *	Returns the objPtr representing the old value.  If no previous value
 *	was present, the NULL is returned.
 *
 * Side Effects:
 *	New tuples may be allocated created.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_string(Table *tablePtr, Row *rowPtr, Column *colPtr, 
			 const char *string, int length)
{
    Value *valuePtr;

    if (colPtr->type != TABLE_COLUMN_TYPE_STRING) {
	Tcl_AppendResult(tablePtr->interp, "column \"", colPtr->label, 
			 "\" is not type string.", (char *)NULL);
	return TCL_ERROR;
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    FreeValue(valuePtr);
    if (SetValueFromString(tablePtr->interp, colPtr->type, string, length, 
		valuePtr) != TCL_OK) {

	return TCL_ERROR;
    }
    /* Indicate the keytables need to be regenerated. */
    if (colPtr->flags & TABLE_COLUMN_PRIMARY_KEY) {
	tablePtr->flags |= TABLE_KEYS_DIRTY;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_append_string --
 *
 *	Sets the value of the selected row, column location in the table.  The
 *	row, column location must be within the actual table limits.
 *
 * Results:
 *	Returns the objPtr representing the old value.  If no previous value
 *	was present, the NULL is returned.
 *
 * Side Effects:
 *	New tuples may be allocated created.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_append_string(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr, 
		       Column *colPtr, const char *s, int length)
{
    Value *valuePtr;
    char *string;
    long l;
    double d;
    
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    if (IsEmpty(valuePtr)) {
	string = Blt_AssertStrdup(s);
    } else {
	int oldLen;

	oldLen = strlen(valuePtr->string);
	string = Blt_AssertMalloc(oldLen + length + 1);
	strcpy(string, valuePtr->string);
	strncpy(string + oldLen, s, length);
	string[oldLen + length] = '\0';
    }
    switch (colPtr->type) {

    case TABLE_COLUMN_TYPE_TIME:	/* double */
	if (Blt_GetTime(interp, string, &d) != TCL_OK) {
	    Blt_Free(string);
	    return TCL_ERROR;
	}
	valuePtr->datum.d = d;
	break;

    case TABLE_COLUMN_TYPE_DOUBLE:	/* double */
	if (Blt_GetDoubleFromString(interp, string, &d) != TCL_OK) {
	    Blt_Free(string);
	    return TCL_ERROR;
	}
	valuePtr->datum.d = d;
	break;
    case TABLE_COLUMN_TYPE_LONG:	/* long */
    case TABLE_COLUMN_TYPE_INT:		/* int */
	if (Blt_GetLong(interp, string, &l) != TCL_OK) {
	    Blt_Free(string);
	    return TCL_ERROR;
	}
	valuePtr->datum.l = l;
	break;
    default:
	break;
    }
    FreeValue(valuePtr);
    valuePtr->string = string;

    /* Indicate the keytables need to be regenerated. */
    if (colPtr->flags & TABLE_COLUMN_PRIMARY_KEY) {
	tablePtr->flags |= TABLE_KEYS_DIRTY;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_double --
 *
 *	Sets the value of the selected row, column location in the table.  The
 *	row, column location must be within the actual table limits.
 *
 * Results:
 *	Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_double(Table *tablePtr, Row *rowPtr, Column *colPtr, double value)
{
    Value *valuePtr;
    char string[200];

    if ((colPtr->type != TABLE_COLUMN_TYPE_DOUBLE) &&
	(colPtr->type != TABLE_COLUMN_TYPE_STRING) &&
	(colPtr->type != TABLE_COLUMN_TYPE_TIME)) {
	Tcl_AppendResult(tablePtr->interp, "column \"", colPtr->label, 
			 "\" is not type double.", (char *)NULL);
	return TCL_ERROR;
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    FreeValue(valuePtr);
    if (!isnan(value)) {
	valuePtr->datum.d = value;
	sprintf(string, "%.17g", value);
	valuePtr->string = Blt_Strdup(string);
    }
    /* Indicate the keytables need to be regenerated. */
    if (colPtr->flags & TABLE_COLUMN_PRIMARY_KEY) {
	tablePtr->flags |= TABLE_KEYS_DIRTY;
    }
    CallTraces(tablePtr, rowPtr, colPtr, TABLE_TRACE_WRITES);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_string --
 *
 *	Sets the value of the selected row, column location in the table.  The
 *	row, column location must be within the actual table limits.
 *
 * Results:
 *	Returns the objPtr representing the old value.  If no previous value
 *	was present, the NULL is returned.
 *
 * Side Effects:
 *	New tuples may be allocated created.
 *
 *---------------------------------------------------------------------------
 */
const char *
blt_table_get_string(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    Value *valuePtr;

    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    if (IsEmpty(valuePtr)) {
	return NULL;
    }
    return valuePtr->string;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_double --
 *
 *	Gets the double value of the selected row, column location in the
 *	table.  The row, column location must be within the actual table
 *	limits.
 *
 * Results:
 *	Returns the objPtr representing the old value.  If no previous value
 *	was present, the NULL is returned.
 *
 * Side Effects:
 *	New tuples may be allocated created.
 *
 *---------------------------------------------------------------------------
 */
double
blt_table_get_double(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    Value *valuePtr;
    double d;

    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    if (IsEmpty(valuePtr)) {
	return Blt_NaN();
    }
    if ((colPtr->type == TABLE_COLUMN_TYPE_DOUBLE) ||
	(colPtr->type == TABLE_COLUMN_TYPE_TIME)) {
	return valuePtr->datum.d;
    }
    if (Blt_GetDoubleFromString(tablePtr->interp, valuePtr->string, &d) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    return d;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_long --
 *
 *	Gets the double value of the selected row, column location in the
 *	table.  The row, column location must be within the actual table
 *	limits.
 *
 * Results:
 *	Returns a long value.  If the value is empty, the default value 
 *	is returned.
 *
 * Side Effects:
 *	New tuples may be allocated created.
 *
 *---------------------------------------------------------------------------
 */
long
blt_table_get_long(Table *tablePtr, Row *rowPtr, Column *colPtr, long defVal)
{
    Value *valuePtr;
    long l;

    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    if (IsEmpty(valuePtr)) {
	return defVal;
    }
    if (colPtr->type == TABLE_COLUMN_TYPE_LONG) {
	return valuePtr->datum.l;
    }
    if (Blt_GetLong(tablePtr->interp, valuePtr->string, &l) != TCL_OK) {
	return TCL_ERROR;
    }
    return l;
}

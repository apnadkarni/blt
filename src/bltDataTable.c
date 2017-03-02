/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *
 * bltDataTable.c --
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

#define BUILD_BLT_TCL_PROCS 1
#include <bltInt.h>

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#include <bltAlloc.h>
#include "bltMath.h"
#include <bltHash.h>
#include <bltPool.h>
#include <bltNsUtil.h>
#include <bltArrayObj.h>
#include <bltTags.h>
#include <bltDataTable.h>

/*
 * Row and Column Information Structures
 *
 *      Row map:    Array of pointers to rows, representing the logical 
 *                  view of row/column.  
 *      Row list:   Actual rows
 *      x = pointer to row/column header.
 *      y = row/column header.
 *              label
 *              index:  logical location of row/column.
 *              offset: physical location of row/column in table storage.
 *              type:   column type.
 *              flags:  
 *
 *      [x] [x] [x] [x] [x] [x] [x] [x] [ ] [ ] [ ]
 *       |   |   |   |   |   |   |   |
 *       v   v   v   v   v   v   v   v 
 *      [y] [y] [y] [y] [y] [y] [y] [y]
 *
 *
 *      Free list:  Chain of free locations. Holds the physical offset
 *                  of next free row or column.  The offsets of deleted 
 *                  rows/columns are prepended to this list.
 *
 *      x = offset of free row/column in table storage.
 *
 *      [x]->[x]->[x]->[x]->[x]
 *
 *      Header pool: Pool of row/column headers.  Act as smart pointers
 *                   to row/column locations.  Will remain valid even if 
 *                   the logical view is changed (i.e. sorting) or physical 
 *                   storage is compacted.
 *
 *  Data Vectors.
 *      Vectors: array of Value arrays.  
 *
 *          x = pointer to Value array.
 *          y = array of Values.
 *
 *      Array of vectors: [x] [x] [x] [x] [x] [ ] [x] [x] [x] [ ] [ ] [ ]
 *                        [y] [y] [y] [y] [y]     [y] [y] [y]
 *                        [y] [y] [y] [y] [y]     [y] [y] [y]
 *                        [y] [y] [ ] [y] [y]     [y] [y] [y]
 *                        [y] [y] [y] [y] [y]     [y] [y] [y]
 *                        [y] [y] [y] [y] [y]     [y] [y] [y]
 *                        [y] [y] [y] [ ] [y]     [y] [y] [y]
 *                        [y] [y] [y] [y] [y]     [y] [y] [y]
 *                        [y] [y] [y] [y] [y]     [y] [ ] [y]
 *                        [y] [y] [y] [y] [y]     [y] [y] [y]
 *                        [y] [y] [y] [y] [y]     [y] [y] [y]
 *                        [y] [y] [y] [y] [y]     [y] [y] [y]
 *                        [y] [y] [y] [y] [y]     [y] [y] [y]
 *                        [y] [y] [y] [y] [y]     [y] [y] [y]
 *                        [y] [y] [y] [y] [y]     [y] [y] [y]
 *                        [ ] [ ] [ ] [ ] [ ]     [ ] [ ] [ ]
 *                        [ ] [ ] [ ] [ ] [ ]     [ ] [ ] [ ]
 *                        [ ] [ ] [ ] [ ] [ ]     [ ] [ ] [ ]
 *                        [ ] [ ] [ ] [ ] [ ]     [ ] [ ] [ ]
 *                        [ ] [ ] [ ] [ ] [ ]     [ ] [ ] [ ]
 *
 */

#define NumColumnsAllocated(t)          ((t)->corePtr->columns.numAllocated)
#define NumRowsAllocated(t)             ((t)->corePtr->rows.numAllocated)

#define TABLE_THREAD_KEY                "BLT DataTable Data"
#define TABLE_MAGIC                     ((unsigned int) 0xfaceface)
#define TABLE_DESTROYED                 (1<<0)

#define TABLE_ALLOC_MAX_DOUBLE_SIZE     (1<<16)
#define TABLE_ALLOC_MAX_CHUNK           (1<<16)
#define TABLE_ALLOC_INIT_SIZE           (32)

#define TABLE_KEYS_DIRTY                (1<<0)
#define TABLE_KEYS_UNIQUE               (1<<1)

/* Column flag. */
#define TABLE_COLUMN_PRIMARY_KEY        (1<<0)

#define REINDEX                         (1<<21)

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
    "string", "double", "long", "time", "boolean", "blob", 
};

/*
 * _BLT_TABLE_TAGS --
 *
 *      Structure representing tags used by a client of the table.
 *
 *      Table rows and columns may be tagged with strings.  A row may have
 *      many tags.  The same tag may be used for many rows.  Tags are used
 *      and stored by clients of a table.  Tags can also be shared between
 *      clients of the same table.
 *      
 *      Both rowTable and columnTable are hash tables keyed by the physical
 *      row or column location in the table respectively.  This is not the
 *      same as the client's view (the order of rows or columns as seen by
 *      the client).  This is so that clients (which may have different
 *      views) can share tags without sharing the same view.
 */
struct _BLT_TABLE_TAGS {
    struct _Blt_Tags rowTags;           /* Table of row indices.  Each
                                         * entry is itself a hash table of
                                         * tag names. */
    struct _Blt_Tags columnTags;        /* Table of column indices.  Each
                                         * entry is itself a hash table of
                                         * tag names. */
    int refCount;                       /* Tracks the number of clients
                                         * currently using these tags. If
                                         * refCount goes to zero, this
                                         * means the table can safely be
                                         * freed. */
};

typedef struct {
    Blt_HashTable clientTable;          /* Tracks all table clients. */
    unsigned int nextId;
    Tcl_Interp *interp;
} InterpData;

typedef struct _BLT_TABLE_ROW Row;
typedef struct _BLT_TABLE_COLUMN Column;
typedef struct _BLT_TABLE_CORE TableObject;
typedef struct _BLT_TABLE_ROWS Rows;
typedef struct _BLT_TABLE_COLUMNS Columns;

typedef struct {
    BLT_TABLE_ROW row;
    BLT_TABLE_COLUMN column;
} RowColumnKey;

static Tcl_InterpDeleteProc TableInterpDeleteProc;
static void DestroyClient(Table *tablePtr);
static void NotifyClients(Table *tablePtr, BLT_TABLE_NOTIFY_EVENT *eventPtr);

static void
UnsetRowLabel(Rows *rowsPtr, Row *rowPtr)
{
    Blt_HashEntry *hPtr;

    assert(rowPtr->label != NULL);
    hPtr = Blt_FindHashEntry(&rowsPtr->labelTable, rowPtr->label);
    assert(hPtr != NULL);
    if (hPtr != NULL) {
        Blt_HashTable *tablePtr;
        Blt_HashEntry *hPtr2;

        tablePtr = Blt_GetHashValue(hPtr);
        hPtr2 = Blt_FindHashEntry(tablePtr, (char *)rowPtr);
        if (hPtr2 != NULL) {
            Blt_DeleteHashEntry(tablePtr, hPtr2);
        }
        if (tablePtr->numEntries == 0) {
            Blt_DeleteHashEntry(&rowsPtr->labelTable, hPtr);
            Blt_DeleteHashTable(tablePtr);
            Blt_Free(tablePtr);
        }
    }   
    rowPtr->label = NULL;
}

static void
UnsetColumnLabel(Columns *columnsPtr, Column *colPtr)
{
    Blt_HashEntry *hPtr;

    if (colPtr->label == NULL) {
        return;
    }
    hPtr = Blt_FindHashEntry(&columnsPtr->labelTable, colPtr->label);
    if (hPtr != NULL) {
        Blt_HashTable *tablePtr;
        Blt_HashEntry *hPtr2;

        tablePtr = Blt_GetHashValue(hPtr);
        hPtr2 = Blt_FindHashEntry(tablePtr, (char *)colPtr);
        if (hPtr2 != NULL) {
            Blt_DeleteHashEntry(tablePtr, hPtr2);
        }
        if (tablePtr->numEntries == 0) {
            Blt_DeleteHashEntry(&columnsPtr->labelTable, hPtr);
            Blt_DeleteHashTable(tablePtr);
            Blt_Free(tablePtr);
        }
    }   
    colPtr->label = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetRowLabel --
 *
 *      Changes the label for the row.  Labels aren't necessarily
 *      unique. It's not enforced.  The rationale is that it is convenient
 *      to be able to add rows/columns to a table, and then change the
 *      labels.  For example, when importing table data from a file, you
 *      can't apriori change the labels.  We could add #n to make the label
 *      unique, but detecting and changing them is a pain.
 *      
 *
 * Results:
 *      Returns a pointer to the new object is successful, NULL otherwise.
 *      If a table object can't be generated, interp->result will contain
 *      an error message.
 *
 * -------------------------------------------------------------------------- 
 */
static void
SetRowLabel(Rows *rowsPtr, Row *rowPtr, const char *newLabel)
{
    Blt_HashEntry *hPtr, *hPtr2;
    Blt_HashTable *tablePtr;            /* Secondary table. */
    int isNew;

    if (rowPtr->label != NULL) {
        UnsetRowLabel(rowsPtr, rowPtr);
    }
    if (newLabel == NULL) {
        return;
    }
    /* Check the primary label table for the bucket.  */
    hPtr = Blt_CreateHashEntry(&rowsPtr->labelTable, newLabel, &isNew);
    if (isNew) {
        tablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
        Blt_InitHashTable(tablePtr, BLT_ONE_WORD_KEYS);
        Blt_SetHashValue(hPtr, tablePtr);
    } else {
        tablePtr = Blt_GetHashValue(hPtr);
    }
    /* Save the label as the hash entry key.  */
    rowPtr->label = Blt_GetHashKey(&rowsPtr->labelTable, hPtr);
    /* Now look for the row in the secondary table. */
    hPtr2 = Blt_CreateHashEntry(tablePtr, (char *)rowPtr, &isNew);
    if (!isNew) {
        return;                         /* It's already there. */
    }
    /* Add it to the secondary table. */
    Blt_SetHashValue(hPtr2, rowPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * SetColumnLabel --
 *
 *      Changes the label for the column.  Labels aren't necessarily
 *      unique. It's not enforced.  The rationale is that it is convenient
 *      to be able to add rows/columns to a table, and then change the
 *      labels.  For example, when importing table data from a file, you
 *      can't apriori change the labels.  We could add #n to make the label
 *      unique, but detecting and changing them is a pain.
 *      
 *
 * Results:
 *      Returns a pointer to the new object is successful, NULL otherwise.
 *      If a table object can't be generated, interp->result will contain
 *      an error message.
 *
 * -------------------------------------------------------------------------- 
 */
static void
SetColumnLabel(Columns *columnsPtr, Column *colPtr, const char *newLabel)
{
    Blt_HashEntry *hPtr;
    Blt_HashTable *tablePtr;            /* Secondary table. */
    int isNew;

    if (colPtr->label != NULL) {
        UnsetColumnLabel(columnsPtr, colPtr);
    }
    if (newLabel == NULL) {
        return;
    }
    /* Check the primary label table for the bucket.  */
    hPtr = Blt_CreateHashEntry(&columnsPtr->labelTable, newLabel, &isNew);
    if (isNew) {
        tablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
        Blt_InitHashTable(tablePtr, BLT_ONE_WORD_KEYS);
        Blt_SetHashValue(hPtr, tablePtr);
    } else {
        tablePtr = Blt_GetHashValue(hPtr);
    }
    /* Save the label as the hash entry key.  */
    colPtr->label = Blt_GetHashKey(&columnsPtr->labelTable, hPtr);
    /* Now look the column in the secondary table. */
    hPtr = Blt_CreateHashEntry(tablePtr, (char *)colPtr, &isNew);
    if (!isNew) {
        return;                         /* It's already there. */
    }
    /* Add it to the secondary table. */
    Blt_SetHashValue(hPtr, colPtr);
}

#ifdef notdef
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
#endif

#if (SIZEOF_VOID_P == 8)  
#define LABEL_FMT       "%s%ld"
#else
#define LABEL_FMT       "%s%d" 
#endif

static void
GetNextRowLabel(Rows *rowsPtr, Row *rowPtr)
{
    char label[200];

    for(;;) {
        Blt_HashEntry *hPtr;

        Blt_FormatString(label, 200, LABEL_FMT, "r", rowsPtr->nextRowId++);
        hPtr = Blt_FindHashEntry(&rowsPtr->labelTable, label);
        if (hPtr == NULL) {
            SetRowLabel(rowsPtr, rowPtr, label);
            break;
        }
    }
}

static void
GetNextColumnLabel(Columns *columnsPtr, Column *colPtr)
{
    char label[200];

    for(;;) {
        Blt_HashEntry *hPtr;

        Blt_FormatString(label, 200, LABEL_FMT, "c",
                         columnsPtr->nextColumnId++);
        hPtr = Blt_FindHashEntry(&columnsPtr->labelTable, label);
        if (hPtr == NULL) {
            SetColumnLabel(columnsPtr, colPtr, label);
            break;
        }
    }
}

static long
GetMapSize(long oldLen, long numExtra)
{
    long newLen, reqLen;

    reqLen = oldLen + numExtra;
    newLen = TABLE_ALLOC_INIT_SIZE;
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

static Row *
NewRow(Rows *rowsPtr) 
{
    Row *rowPtr;

    rowPtr = Blt_Pool_AllocItem(rowsPtr->pool, sizeof(Row));
    memset(rowPtr, 0, sizeof(Row));
    GetNextRowLabel(rowsPtr, rowPtr);
    if (rowsPtr->headPtr == NULL) {
        rowsPtr->tailPtr = rowsPtr->headPtr = rowPtr;
    } else {
        rowPtr->prevPtr = rowsPtr->tailPtr;
        if (rowsPtr->tailPtr != NULL) {
            rowsPtr->tailPtr->nextPtr = rowPtr;
        }
        rowsPtr->tailPtr = rowPtr;
    }
    rowPtr->index = rowsPtr->numUsed;
    rowsPtr->numUsed++;
    return rowPtr;
}

static Column *
NewColumn(Columns *columnsPtr) 
{
    Column *colPtr;

    colPtr = Blt_Pool_AllocItem(columnsPtr->pool, sizeof(Column));
    memset(colPtr, 0, sizeof(Column));
    GetNextColumnLabel(columnsPtr, colPtr);
    if (columnsPtr->headPtr == NULL) {
        columnsPtr->tailPtr = columnsPtr->headPtr = colPtr;
    } else {
        colPtr->prevPtr = columnsPtr->tailPtr;
        if (columnsPtr->tailPtr != NULL) {
            columnsPtr->tailPtr->nextPtr = colPtr;
        }
        columnsPtr->tailPtr = colPtr;
    }
    colPtr->index = columnsPtr->numUsed;
    columnsPtr->numUsed++;
    return colPtr;
}


static int
ExtendRows(Table *tablePtr, size_t numExtraRows, Blt_Chain chain)
{
    Rows *rowsPtr;
    size_t i, prevUsed;

    rowsPtr = &tablePtr->corePtr->rows;
    prevUsed = rowsPtr->numUsed;

    /* If we are going to exceed the current number of allocated rows,
     * re-allocate a bigger row map and column vectors. */
    if ((numExtraRows + rowsPtr->numUsed) > rowsPtr->numAllocated) {
        size_t newSize;
        Row **map;
        Column *colPtr;
    
        newSize = GetMapSize(rowsPtr->numAllocated, numExtraRows);

        /* Resize the row map. */
        if (rowsPtr->map == NULL) {
            map = Blt_Malloc(sizeof(Row *) * newSize);
        } else {
            map = Blt_Realloc(rowsPtr->map, sizeof(Row *) * newSize);
        }
        if (map == NULL) {
            return FALSE;
        }
        rowsPtr->map = map;
        rowsPtr->numAllocated = newSize;

        /* Resize the individual column vectors.  */
        for (colPtr = tablePtr->corePtr->columns.headPtr; colPtr != NULL;
             colPtr = colPtr->nextPtr) {
            if (colPtr->vector != NULL) {
                Value *vector;
                
                vector = Blt_Realloc(colPtr->vector, newSize * sizeof(Value));
                memset(vector + prevUsed, 0, numExtraRows * sizeof(Value));
                colPtr->vector = vector;
            }
        }        
    }

    /* Fill in the new slots in the map in with actual row structures.
     * Reset the row index and offset.  Reuse row offsets from the free
     * list first. */
    for (i = 0; i < numExtraRows; i++) {
        Row *rowPtr;
        size_t offset, nextIndex;

        rowPtr = NewRow(rowsPtr);
        offset = nextIndex = i + prevUsed;
        if (Blt_Chain_GetLength(rowsPtr->freeList) > 0) {
            Blt_ChainLink link;
            
            link = Blt_Chain_FirstLink(rowsPtr->freeList);
            offset = (long)Blt_Chain_GetValue(link);
            Blt_Chain_DeleteLink(rowsPtr->freeList, link);
        }
        if (chain != NULL) {
            Blt_Chain_Append(chain, rowPtr);
        }
        rowsPtr->map[nextIndex] = rowPtr;
        rowPtr->offset = offset;
    }
    return TRUE;
}

static int
ExtendColumns(Table *tablePtr, size_t numExtraColumns, Blt_Chain chain)
{
    long i;
    size_t prevUsed;
    Columns *columnsPtr;

    columnsPtr = &tablePtr->corePtr->columns;
    prevUsed = columnsPtr->numUsed;
    if ((numExtraColumns + columnsPtr->numUsed) > columnsPtr->numAllocated) {
        Column **map;
        size_t newSize;

        /* Resize the map. */
        newSize = GetMapSize(columnsPtr->numAllocated, numExtraColumns);
        if (columnsPtr->map == NULL) {
            map = Blt_Malloc(sizeof(Column *) * newSize);
        } else {
            map = Blt_Realloc(columnsPtr->map, sizeof(Column *) * newSize);
        }
        if (map == NULL) {
            return FALSE;
        }
        columnsPtr->map = map;
        columnsPtr->numAllocated = newSize;
    }

    /* Fill the map in with the actual columns. There's no storage yet for
     * values.  We'll allocate that as needed. */
    for (i = 0; i < numExtraColumns; i++) {
        Column *colPtr;

        colPtr = NewColumn(columnsPtr);
        columnsPtr->map[i + prevUsed] = colPtr;
        if (chain != NULL) {
            Blt_Chain_Append(chain, colPtr);
        }
    }
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetRowMap --
 *
 *      The map and the rows are out-of-sync.  There may be empty slots in
 *      the map (because rows were deleted). 
 *      
 *---------------------------------------------------------------------------
 */
static void
ResetRowMap(Rows *rowsPtr)
{
    size_t count;
    Row *rowPtr;
    
    /* Reset the map to the current list of columns. */
    count = 0;
    for (rowPtr = rowsPtr->headPtr; rowPtr != NULL;
         rowPtr = rowPtr->nextPtr) {
        rowsPtr->map[count] = rowPtr;
        rowPtr->index = count;
        count++;
    }
    assert(count == rowsPtr->numUsed);
    rowsPtr->flags &= ~REINDEX;
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetColumnMap --
 *
 *      The map and the columns are out-of-sync.  There may be empty slots
 *      in the map (because columns were deleted). 
 *      
 *---------------------------------------------------------------------------
 */
static void
ResetColumnMap(Columns *columnsPtr)
{
    size_t count;
    Column *colPtr;
    
    /* Reset the map to the current list of columns. */
    count = 0;
    for (colPtr = columnsPtr->headPtr; colPtr != NULL;
         colPtr = colPtr->nextPtr) {
        columnsPtr->map[count] = colPtr;
        colPtr->index = count;
        count++;
    }
    assert(count == columnsPtr->numUsed);
    columnsPtr->flags &= ~REINDEX;
}


BLT_TABLE_ROW 
blt_table_row(BLT_TABLE table, long index)  
{
    Rows *rowsPtr;

    rowsPtr = blt_table_rows(table);
    if (rowsPtr->flags & REINDEX) {
        ResetRowMap(rowsPtr);
    }
    return rowsPtr->map[index];
}

BLT_TABLE_COLUMN 
blt_table_column(BLT_TABLE table, long index)  
{
    Columns *columnsPtr;

    columnsPtr = blt_table_columns(table);
    if (columnsPtr->flags & REINDEX) {
        ResetColumnMap(columnsPtr);
    }
    return columnsPtr->map[index];
}

long 
blt_table_row_index(BLT_TABLE table, Row *rowPtr)
{
    Rows *rowsPtr;

    rowsPtr = blt_table_rows(table);
    if (rowsPtr->flags & REINDEX) {
        ResetRowMap(rowsPtr);
    }
    return rowPtr->index;
}

long 
blt_table_column_index(BLT_TABLE table, Column *colPtr)
{
    Columns *columnsPtr;

    columnsPtr = blt_table_columns(table);
    if (columnsPtr->flags & REINDEX) {
        ResetColumnMap(columnsPtr);
    }
    return colPtr->index;
}

BLT_TABLE_COLUMN_TYPE
blt_table_name_to_column_type(const char *s)
{
    char c;
    size_t len;
    
    c = s[0];
    len = strlen(s);
    if ((c == 's') && (len > 2) && (strncmp(s, "string", len) == 0)) {
        return TABLE_COLUMN_TYPE_STRING;
    } else if ((c == 'i') && (len > 2) && (strncmp(s, "integer", len) == 0)) {
        return TABLE_COLUMN_TYPE_LONG;
    } else if ((c == 'n') && (len > 2) && (strncmp(s, "number", len) == 0)) {
        return TABLE_COLUMN_TYPE_DOUBLE;
    } else if ((c == 'd') && (strcmp(s, "double") == 0)) {
        return TABLE_COLUMN_TYPE_DOUBLE;
    } else if ((c == 'l') && (strcmp(s, "long") == 0)) {
        return TABLE_COLUMN_TYPE_LONG;
    } else if ((c == 't') && (strcmp(s, "time") == 0)) {
        return TABLE_COLUMN_TYPE_TIME;
    } else if ((c == 'b') && (strcmp(s, "blob") == 0)) {
        return TABLE_COLUMN_TYPE_BLOB;
    } else if ((c == 'b') && (strcmp(s, "boolean") == 0)) {
        return TABLE_COLUMN_TYPE_BOOLEAN;
    } else {
        return TABLE_COLUMN_TYPE_UNKNOWN;
    }
}

static INLINE const char *
GetValueString(Value *valuePtr)
{
    if (valuePtr->string == TABLE_VALUE_STORE) {
        return (const char *)valuePtr->store;
    }
    return valuePtr->string;
}

static INLINE const unsigned char *
GetValueBytes(Value *valuePtr)
{
    if (valuePtr->string == TABLE_VALUE_STORE) {
        return (const unsigned char *)valuePtr->store;
    }
    return (const unsigned char *)valuePtr->string;
}

static INLINE size_t
GetValueLength(Value *valuePtr)
{
    return valuePtr->length;
}

const char *
blt_table_value_string(Value *valuePtr)
{
    return GetValueString(valuePtr);
}

const unsigned char *
blt_table_value_bytes(Value *valuePtr)
{
    return GetValueBytes(valuePtr);
}

size_t
blt_table_value_length(Value *valuePtr)
{
    return GetValueLength(valuePtr);
}

static INLINE int
IsEmptyValue(Value *valuePtr)
{
    return ((valuePtr == NULL) || (valuePtr->string == NULL));
}

static INLINE int
IsEmpty(Row *rowPtr, Column *colPtr)
{
    if (colPtr->vector != NULL) {
        Value *valuePtr;

        valuePtr = colPtr->vector + rowPtr->offset;
        return IsEmptyValue(valuePtr);
    }
    return TRUE;
}

static INLINE void
ResetValue(Value *valuePtr)
{
    if ((valuePtr->string != NULL) &&
        (valuePtr->string != TABLE_VALUE_STORE)) {
        Blt_Free(valuePtr->string);
    }
    valuePtr->length = 0;
    valuePtr->string = NULL;
}


static Value *
GetValue(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    if (colPtr->vector == NULL) {
        Value *vector;

        assert(tablePtr->corePtr->rows.numAllocated > 0);
        vector = Blt_Calloc(tablePtr->corePtr->rows.numAllocated,sizeof(Value));
        if (vector == NULL) {
            return NULL;
        }
        colPtr->vector = vector;
    }
    return colPtr->vector + rowPtr->offset;
}

static Tcl_Obj *
GetObjFromValue(BLT_TABLE_COLUMN_TYPE type, Value *valuePtr)
{
    Tcl_Obj *objPtr;

    assert(!IsEmptyValue(valuePtr));
    objPtr = NULL;                      /* Suppress compiler warning. */
    assert(type != TABLE_COLUMN_TYPE_UNKNOWN);
    switch (type) {
    case TABLE_COLUMN_TYPE_BLOB:
        objPtr = Tcl_NewByteArrayObj(GetValueBytes(valuePtr),
                                     GetValueLength(valuePtr));
        break;
    case TABLE_COLUMN_TYPE_TIME:        /* time */
    case TABLE_COLUMN_TYPE_DOUBLE:      /* double */
        objPtr = Tcl_NewDoubleObj(valuePtr->datum.d);
        break;
    case TABLE_COLUMN_TYPE_LONG:        /* long */
        objPtr = Tcl_NewLongObj(valuePtr->datum.l);
        break;
    case TABLE_COLUMN_TYPE_BOOLEAN:      /* boolean */
        objPtr = Tcl_NewBooleanObj(valuePtr->datum.l);
        break;
    default:
    case TABLE_COLUMN_TYPE_STRING:      /* string */
        objPtr = Tcl_NewStringObj(GetValueString(valuePtr),
                                  GetValueLength(valuePtr));
        break;
    }
    return objPtr;
}

static int
SetValueFromObj(Tcl_Interp *interp, BLT_TABLE_COLUMN_TYPE type, 
                Tcl_Obj *objPtr, Value *valuePtr)
{
    int length;
    const char *s;

    if (objPtr == NULL) {
        return TCL_OK;
    }
    ResetValue(valuePtr);
    switch (type) {
    case TABLE_COLUMN_TYPE_TIME:        /* time */
        if (Blt_GetTimeFromObj(interp, objPtr, &valuePtr->datum.d) != TCL_OK) {
            return TCL_ERROR;
        }
        break;

    case TABLE_COLUMN_TYPE_BOOLEAN:     /* boolean */
        { 
            int i;
            
            if (Tcl_GetBooleanFromObj(interp, objPtr, &i) != TCL_OK) {
                return TCL_ERROR;
            }
            valuePtr->datum.l = i;
        }
        break;

    case TABLE_COLUMN_TYPE_DOUBLE:      /* double */
        if (Blt_GetDoubleFromObj(interp, objPtr, &valuePtr->datum.d)!=TCL_OK) {
            return TCL_ERROR;
        }
        break;
    case TABLE_COLUMN_TYPE_LONG:        /* long */
        if (Blt_GetLongFromObj(interp, objPtr, &valuePtr->datum.l) != TCL_OK) {
            return TCL_ERROR;
        }
        break;
    default:
        break;
    }
    s = Tcl_GetStringFromObj(objPtr, &length);
    if (length >= TABLE_VALUE_LENGTH) {
        valuePtr->string = Blt_Strndup(s, length);
        valuePtr->length = length;
    } else {
        strncpy(valuePtr->store, s, length);
        valuePtr->store[length] = '\0';
        valuePtr->string = TABLE_VALUE_STORE;
        valuePtr->length = length;
    }
    return TCL_OK;
}


static int
SetValueFromString(Tcl_Interp *interp, BLT_TABLE_COLUMN_TYPE type, 
                   const char *s, int length, Value *valuePtr)
{
    double d;
    long l;
    Tcl_Obj *objPtr;

    objPtr = NULL;
    if ((type != TABLE_COLUMN_TYPE_STRING) ||
        (type != TABLE_COLUMN_TYPE_BLOB)) {

        /* For the non-string types, make a copy of the string as a
         * Tcl_Obj.  This will give us a canonical string representation
         * and also verify that the string is valid.  */
        objPtr = Tcl_NewStringObj(s, length);
        Tcl_IncrRefCount(objPtr);
        
        switch (type) {
        case TABLE_COLUMN_TYPE_TIME:    
            if (Blt_GetTimeFromObj(interp, objPtr, &d) != TCL_OK) {
                Tcl_DecrRefCount(objPtr);
                return TCL_ERROR;
            }
            valuePtr->datum.d = d;      /* double */
            break;
            
        case TABLE_COLUMN_TYPE_DOUBLE:  
            if (Blt_GetDoubleFromObj(interp, objPtr, &d) != TCL_OK) {
                Tcl_DecrRefCount(objPtr);
                return TCL_ERROR;
            }
            valuePtr->datum.d = d;      /* double */
            break;

        case TABLE_COLUMN_TYPE_LONG:    
            if (Blt_GetLongFromObj(interp, objPtr, &l) != TCL_OK) {
                Tcl_DecrRefCount(objPtr);
                return TCL_ERROR;
            }
            valuePtr->datum.l = l;      /* long */
            break;

        case TABLE_COLUMN_TYPE_BOOLEAN: 
            {
                int ival;
                
                if (Tcl_GetBooleanFromObj(interp, objPtr, &ival) != TCL_OK) {
                    Tcl_DecrRefCount(objPtr);
                    return TCL_ERROR;
                }
                valuePtr->datum.l = ival; /* long */
            }
            break;
        default:
            break;
        }
        s = Tcl_GetStringFromObj(objPtr, &length);
    }        

    ResetValue(valuePtr);
    if (length >= TABLE_VALUE_LENGTH) {
        valuePtr->string = Blt_Strndup(s, length);
        valuePtr->length = length;
    } else {
        strncpy(valuePtr->store, s, length);
        valuePtr->store[length] = '\0';
        valuePtr->string = TABLE_VALUE_STORE;
        valuePtr->length = length;
    }
    if (objPtr != NULL) {
        Tcl_DecrRefCount(objPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewTableObject --
 *
 *      Creates and initializes a new table object. 
 *
 * Results:
 *      Returns a pointer to the new object is successful, NULL otherwise.
 *      If a table object can't be generated, interp->result will contain
 *      an error message.
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
    corePtr->columns.pool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    corePtr->columns.nextColumnId = 1;
    corePtr->rows.freeList = Blt_Chain_Create();
    corePtr->rows.pool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    corePtr->rows.nextRowId = 1;
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
 *      Fires a trace set by a client of the table object.  Trace
 *      procedures should return a standard TCL result.
 *
 *         TCL_OK       procedure executed successfully.
 *         TCL_ERROR    procedure failed.
 *         TCL_BREAK    don't execute any further trace procedures.
 *         TCL_CONTINUE treat like TCL_OK.
 *
 *      A trace procedure can in turn trigger more traces.  Traces are
 *      prohibited from recursively reentering their own trace procedures.
 *      A hash table in the trace structure tracks the cells currently
 *      actively traced.  If a cell is already being traced, the trace
 *      procedure is not called and TCL_OK is blindly returned.
 *
 * Results:
 *      Returns the result of trace procedure.  If the trace is already
 *      active, then TCL_OK is returned.
 *
 * Side Effects:
 *      Traces on the table location may be fired.
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
                 Tcl_GetString(Tcl_GetObjResult(eventPtr->interp)));
        Tcl_BackgroundError(eventPtr->interp);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TraceIdleProc --
 *
 *      Used to invoke a table trace handler routines at some idle point.
 *      This routine is called from the TCL event loop.  Errors generated
 *      by the event handler routines are backgrounded.
 *      
 * Results:
 *      None.
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
 *      Used to invoke event handler routines at some idle point.  This
 *      routine is called from the TCL event loop.  Errors generated by the
 *      event handler routines are backgrounded.
 *      
 * Results:
 *      None.
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


static void
FreeRows(TableObject *corePtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Rows *rowsPtr;

    rowsPtr = &corePtr->rows;
    for (hPtr = Blt_FirstHashEntry(&rowsPtr->labelTable, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        Blt_HashTable *tablePtr;
        
        tablePtr = Blt_GetHashValue(hPtr);
        Blt_DeleteHashTable(tablePtr);
        Blt_Free(tablePtr);
    }
    Blt_DeleteHashTable(&rowsPtr->labelTable);
    Blt_Pool_Destroy(rowsPtr->pool);
    if (rowsPtr->freeList != NULL) {
        Blt_Chain_Destroy(rowsPtr->freeList);
    }
    if (rowsPtr->map != NULL) {
        Blt_Free(rowsPtr->map);
        rowsPtr->map = NULL;
    }
    rowsPtr->numAllocated = rowsPtr->numUsed = 0;
}

static void
FreeColumns(TableObject *corePtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Column *colPtr;
    Columns *columnsPtr;

    columnsPtr = &corePtr->columns;
    for (colPtr = columnsPtr->headPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr) {
        if (colPtr->vector != NULL) {
            Row *rowPtr;

            for (rowPtr = corePtr->rows.headPtr; rowPtr != NULL;
                 rowPtr = rowPtr->nextPtr) {
                assert(rowPtr->offset < corePtr->rows.numAllocated);
                ResetValue(colPtr->vector + rowPtr->offset);
            }
            Blt_Free(colPtr->vector);
            colPtr->vector = NULL;
        }
    }
    for (hPtr = Blt_FirstHashEntry(&columnsPtr->labelTable, &iter); 
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        Blt_HashTable *tablePtr;
        
        tablePtr = Blt_GetHashValue(hPtr);
        Blt_DeleteHashTable(tablePtr);
        Blt_Free(tablePtr);
    }
    Blt_DeleteHashTable(&columnsPtr->labelTable);
    if (columnsPtr->pool != NULL) {
        Blt_Pool_Destroy(columnsPtr->pool);
    }
    if (columnsPtr->map != NULL) {
        Blt_Free(columnsPtr->map);
        columnsPtr->map = NULL;
    }
    columnsPtr->numAllocated = columnsPtr->numUsed = 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyTableObject --
 *
 *      Destroys the table object.  This is the final clean up of the
 *      object.  The object's entry is removed from the hash table of
 *      tables.
 *
 * Results: 
 *      None.
 *
 * -------------------------------------------------------------------------- 
 */
static void
DestroyTableObject(TableObject *corePtr)
{
    corePtr->flags |= TABLE_DESTROYED;

    assert(Blt_Chain_GetLength(corePtr->clients) == 0);
    Blt_Chain_Destroy(corePtr->clients);

    FreeColumns(corePtr);
    FreeRows(corePtr);
    Blt_Free(corePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TableInterpDeleteProc --
 *
 *      This is called when the interpreter hosting the table object is
 *      deleted from the interpreter.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Destroys all remaining tables and removes the hash table used to
 *      register table names.
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
 *      Creates or retrieves data associated with tuple data objects for a
 *      particular interpreter.
 *
 * Results:
 *      Returns a pointer to the tuple interpreter data.
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
        Blt_Tags_Init(&tagsPtr->rowTags);
        Blt_Tags_Init(&tagsPtr->columnTags);
        tagsPtr->refCount = 1;
    }
    return tagsPtr;
}

static void
InitTags(Table *tablePtr)
{
    tablePtr->tags = NewTags();
    tablePtr->rowTags = &tablePtr->tags->rowTags;
    tablePtr->columnTags = &tablePtr->tags->columnTags;
}

/*
 *---------------------------------------------------------------------------
 *
 * ShareTags --
 *
 *---------------------------------------------------------------------------
 */
static void
ShareTags(Table *srcPtr, Table *destPtr)
{
    srcPtr->tags->refCount++;
    if (destPtr->tags != NULL) {
        blt_table_release_tags(destPtr);
    }
    destPtr->tags = srcPtr->tags;
    destPtr->rowTags = &destPtr->tags->rowTags;
    destPtr->columnTags = &destPtr->tags->columnTags;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindClientInNamespace --
 *
 *      Searches for a table client in a given namespace.
 *
 * Results:
 *      Returns a pointer to the table client if found, otherwise NULL.
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
    assert(link != NULL);
    return Blt_Chain_GetValue(link);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTable --
 *
 *      Searches for the table client associated by the name given.
 *
 * Results:
 *      Returns a pointer to the table client if found, otherwise NULL.
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
 *      Creates a new table client.  Clients shared a tuple data object.
 *      They individually manage traces and events on tuple objects.
 *      Returns a pointer to the malloc'ed structure.  This is passed to
 *      the client as a tuple token.
 *      
 * Results:
 *      A pointer to a Table is returned.  If one can't be allocated, NULL
 *      is returned.
 *
 *---------------------------------------------------------------------------
 */
static Table *
NewTable(
    InterpData *dataPtr, 
    TableObject *corePtr,               /* Table object serving this
                                         * client. */
    const char *qualName)               /* Full namespace qualified name of
                                         * table. A table by this name may
                                         * already exist. */
{
    Blt_Chain chain;                    /* List of clients using the same
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

    /* Create a new set of tags. */
    InitTags(tablePtr);

    tablePtr->clientTablePtr = &dataPtr->clientTable;
    /* Table names are not unique.  More than one client may open the same
     * table.  The name remains in use so long as one client is still using
     * the table. This is so other clients can refer to the table, even
     * though the original client that created the data no longer
     * exists. */
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
GetColumnLabelTable(Columns *columnsPtr, const char *label)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&columnsPtr->labelTable, label);
    if (hPtr != NULL) {
        return Blt_GetHashValue(hPtr);
    }
    return NULL;
}

static Blt_HashTable *
GetRowLabelTable(Rows *rowsPtr, const char *label)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&rowsPtr->labelTable, label);
    if (hPtr != NULL) {
        return Blt_GetHashValue(hPtr);
    }
    return NULL;
}

static Row *
FindRowLabel(Rows *rowsPtr, const char *label)
{
    Blt_HashTable *tablePtr;

    tablePtr = GetRowLabelTable(rowsPtr, label);
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

static Column *
FindColumnLabel(Columns *columnsPtr, const char *label)
{
    Blt_HashTable *tablePtr;

    tablePtr = GetColumnLabelTable(columnsPtr, label);
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
SetColumnType(Tcl_Interp *interp, Table *tablePtr, Column *colPtr, 
              BLT_TABLE_COLUMN_TYPE type)
{
    Row *rowPtr;

    if (type == colPtr->type) {
        return TCL_OK;                  /* Already the requested type. */
    }
    /* First test that every value in the column can be converted. */
    for (rowPtr = tablePtr->corePtr->rows.headPtr; rowPtr != NULL; 
         rowPtr = rowPtr->nextPtr) {
        if (!IsEmpty(rowPtr, colPtr)) {
            Value value;
            Value *valuePtr;

            valuePtr = GetValue(tablePtr, rowPtr, colPtr);
            memset(&value, 0, sizeof(Value));
            if (SetValueFromString(interp, type, GetValueString(valuePtr),
                        GetValueLength(valuePtr), &value) != TCL_OK) {
                return TCL_ERROR;
            }
            ResetValue(&value);
        }
    }
    /* Now replace the column with the converted the values. */
    for (rowPtr = tablePtr->corePtr->rows.headPtr; rowPtr != NULL; 
         rowPtr = rowPtr->nextPtr) {
        if (!IsEmpty(rowPtr, colPtr)) {
            Value *valuePtr;

            valuePtr = GetValue(tablePtr, rowPtr, colPtr);
            if (SetValueFromString(interp, type, GetValueString(valuePtr),
                 GetValueLength(valuePtr), valuePtr) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    colPtr->type = type;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * UnsetValue --
 *
 *      Removes the value from the selected row, column location in the
 *      table.  The row, column location must be within the actual table
 *      limits, but it's okay if there isn't a value there to remove.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The objPtr representing the value is released.
 *
 *---------------------------------------------------------------------------
 */
static void
UnsetValue(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    Value *valuePtr;

    if (colPtr->vector == NULL) {
        return;
    }
    valuePtr = colPtr->vector + rowPtr->offset;
    if (!IsEmptyValue(valuePtr)) {
        /* Indicate the keytables need to be regenerated. */
        if (colPtr->flags & TABLE_COLUMN_PRIMARY_KEY) {
            tablePtr->flags |= TABLE_KEYS_DIRTY;
        }
    }
    ResetValue(valuePtr);
}

static void
UnsetRowValues(Table *tablePtr, Row *rowPtr)
{
    Column *colPtr;

    for (colPtr = tablePtr->corePtr->columns.headPtr; colPtr != NULL;
         colPtr = colPtr->nextPtr) {
        if (colPtr->vector != NULL) {
            UnsetValue(tablePtr, rowPtr, colPtr);
        }
    }
}

static void
DeleteRow(Rows *rowsPtr, Row *rowPtr)
{
    /* If there is a label is associated with the row, free it. */
    if (rowPtr->label != NULL) {
        UnsetRowLabel(rowsPtr, rowPtr);
    }
    /* Unlink the row from the list of rows. */
    if (rowsPtr->headPtr == rowPtr) {
        rowsPtr->headPtr = rowPtr->nextPtr;
    }
    if (rowsPtr->tailPtr == rowPtr) {
        rowsPtr->tailPtr = rowPtr->prevPtr;
    }
    if (rowPtr->nextPtr != NULL) {
        rowPtr->nextPtr->prevPtr = rowPtr->prevPtr;
    }
    if (rowPtr->prevPtr != NULL) {
        rowPtr->prevPtr->nextPtr = rowPtr->nextPtr;
    }
    /* Mark the map entry as empty. */
    rowsPtr->map[rowPtr->index] = NULL;
    rowsPtr->flags |= REINDEX;
    if (rowsPtr->freeList != NULL) {
        /* We don't delete row storage, just add the row offset back onto
         * the free list. */
        Blt_Chain_Append(rowsPtr->freeList, (ClientData)rowPtr->offset);
    }
    /* Finally free the row itself. */
    Blt_Pool_FreeItem(rowsPtr->pool, rowPtr);
    rowsPtr->numUsed--;
}

static void
DeleteColumn(Table *tablePtr, Column *colPtr)
{
    Columns *columnsPtr;

    columnsPtr = &tablePtr->corePtr->columns;
    /* If there is a label is associated with the column, free it. */
    if (colPtr->label != NULL) {
        UnsetColumnLabel(columnsPtr, colPtr);
    }
    if (columnsPtr->headPtr == colPtr) {
        columnsPtr->headPtr = colPtr->nextPtr;
    }
    if (columnsPtr->tailPtr == colPtr) {
        columnsPtr->tailPtr = colPtr->prevPtr;
    }
    if (colPtr->nextPtr != NULL) {
        colPtr->nextPtr->prevPtr = colPtr->prevPtr;
    }
    if (colPtr->prevPtr != NULL) {
        colPtr->prevPtr->nextPtr = colPtr->nextPtr;
    }
    colPtr->prevPtr = colPtr->nextPtr = NULL;
    columnsPtr->map[colPtr->index] = NULL;
    columnsPtr->flags |= REINDEX;
    if (colPtr->vector != NULL) {
        Row *rowPtr;

        for (rowPtr = tablePtr->corePtr->rows.headPtr; rowPtr != NULL;
             rowPtr = rowPtr->nextPtr) {
            ResetValue(colPtr->vector + rowPtr->offset);
        }
        Blt_Free(colPtr->vector);
        colPtr->vector = NULL;
    }
    /* Finally free the column. */
    Blt_Pool_FreeItem(columnsPtr->pool, colPtr);
    columnsPtr->numUsed--;
}

/*
 *---------------------------------------------------------------------------
 *
 * ClearRowNotifiers --
 *
 *      Removes all event handlers set for the designated row.  Note that
 *      this doesn't remove handlers triggered by row or column tags.  Row
 *      and column event notifiers are stored in a chain.
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
 *      Removes all event handlers set for the designated row.  Note that
 *      this doesn't remove handlers triggered by row or column tags.  Row
 *      and column event notifiers are stored in a chain.
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
 *      Traverses the client's list of event callbacks and checks if one
 *      matches the given event.  A client may trigger an action that
 *      causes the itself to be notified again.  This can be prevented by
 *      setting the TABLE_NOTIFY_FOREIGN_ONLY bit in the event handler.
 *
 *      If a matching handler is found, a callback may be called either
 *      immediately or at the next idle time depending upon the
 *      TABLE_NOTIFY_WHENIDLE bit.
 *
 *      Since a handler routine may trigger yet another call to itself,
 *      callbacks are ignored while the event handler is executing.
 *      
 *---------------------------------------------------------------------------
 */
static void
DoNotify(Table *tablePtr, Blt_Chain notifiers, 
         BLT_TABLE_NOTIFY_EVENT *eventPtr)
{
    Blt_ChainLink link;
    unsigned int eventMask;

    /* Check the client table for matching notifiers.  Issue callbacks
     * indicating that the structure of the table has changed.  */
    eventMask = eventPtr->type & TABLE_NOTIFY_MASK;
    for (link = Blt_Chain_FirstLink(notifiers); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Notifier *notifierPtr;
        int match;

        notifierPtr = Blt_Chain_GetValue(link);
        if ((notifierPtr->flags & eventMask) == 0) {
            continue;                   /* Event type doesn't match */
        }
        if ((eventPtr->self) && 
            (notifierPtr->flags & TABLE_NOTIFY_FOREIGN_ONLY)) {
            continue;                   /* Don't notify yourself. */
        }
        if (notifierPtr->flags & TABLE_NOTIFY_ACTIVE) {
            continue;                   /* Ignore callbacks that are
                                         * generated inside of a notify
                                         * handler routine. */
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
            match++;                    /* Offsets match. */
        } else if ((notifierPtr->flags & TABLE_NOTIFY_COLUMN) && 
                   ((notifierPtr->column == NULL) ||
                    (notifierPtr->column == eventPtr->column))) {
            match++;                    /* Offsets match. */
        }
        if (!match) {
            continue;                   /* Row or column doesn't match. */
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
 *      Traverses the list of event callbacks and checks if one matches the
 *      given event.  A client may trigger an action that causes the table
 *      object to notify it.  This can be prevented by setting the
 *      TABLE_NOTIFY_FOREIGN_ONLY bit in the event handler.
 *
 *      If a matching handler is found, a callback may be called either
 *      immediately or at the next idle time depending upon the
 *      TABLE_NOTIFY_WHENIDLE bit.
 *
 *      Since a handler routine may trigger yet another call to itself,
 *      callbacks are ignored while the event handler is executing.
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
        Blt_Chain chain;
        
        next = Blt_Chain_NextLink(link);
        clientPtr = Blt_Chain_GetValue(link);
        eventPtr->self = (clientPtr == tablePtr);
        chain = (eventPtr->type & TABLE_NOTIFY_COLUMN) ?
            clientPtr->columnNotifiers : clientPtr->rowNotifiers;
        if (Blt_Chain_GetLength(chain) > 0) {
            DoNotify(clientPtr, chain, eventPtr);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyColumnChanged --
 *
 *      Traverses the list of event callbacks and checks if one matches the
 *      given event.  A client may trigger an action that causes the table
 *      object to notify it.  This can be prevented by setting the
 *      TABLE_NOTIFY_FOREIGN_ONLY bit in the event handler.
 *
 *      If a matching handler is found, a callback may be called either
 *      immediately or at the next idle time depending upon the
 *      TABLE_NOTIFY_WHENIDLE bit.
 *
 *      Since a handler routine may trigger yet another call to itself,
 *      callbacks are ignored while the event handler is executing.
 *      
 *---------------------------------------------------------------------------
 */
static void
NotifyColumnChanged(Table *tablePtr, Column *colPtr, unsigned int flags)
{
    BLT_TABLE_NOTIFY_EVENT event;

    InitNotifyEvent(tablePtr, &event);
    event.type = flags | TABLE_NOTIFY_COLUMN;
    event.column = colPtr;
    NotifyClients(tablePtr, &event);
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyRowChanged --
 *
 *      Traverses the list of event callbacks and checks if one matches the
 *      given event.  A client may trigger an action that causes the table
 *      object to notify it.  This can be prevented by setting the
 *      TABLE_NOTIFY_FOREIGN_ONLY bit in the event handler.
 *
 *      If a matching handler is found, a callback may be called either
 *      immediately or at the next idle time depending upon the
 *      TABLE_NOTIFY_WHENIDLE bit.
 *
 *      Since a handler routine may trigger yet another call to itself,
 *      callbacks are ignored while the event handler is executing.
 *      
 *---------------------------------------------------------------------------
 */
static void
NotifyRowChanged(Table *tablePtr, Row *rowPtr, unsigned int flags)
{
    BLT_TABLE_NOTIFY_EVENT event;

    InitNotifyEvent(tablePtr, &event);
    event.type = flags | TABLE_NOTIFY_ROW;
    event.row = rowPtr;
    NotifyClients(tablePtr, &event);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_clear_row_traces --
 *
 *      Removes all traces set for this row.  Note that this doesn't remove
 *      traces set for specific cells (row,column).  Row traces are stored
 *      in a chain, which in turn is held in a hash table, keyed by the
 *      row.
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
 *      Removes all traces set for this column.  Note that this doesn't
 *      remove traces set for specific cells (row,column).  Column traces
 *      are stored in a chain, which in turn is held in a hash table, keyed
 *      by the column.
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
 *      Examines the traces set for a specific client of the table object
 *      and fires any matching traces.
 *
 *      Traces match on row and column tag and indices and flags.
 *      Traces can match on
 *           flag               type of trace (read, write, unset, create)
 *           row index
 *           column index
 *           row tag
 *           column tag
 *
 *      If the TABLE_TRACE_FOREIGN_ONLY is set in the handler, it means to
 *      ignore actions that are initiated by that client of the object.
 *      Only actions by other clients are handled.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 * Side Effects:
 *      Traces on the tuple table location may be fired.
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
            continue;                   /* Doesn't match trace flags. */
        }
        if (tracePtr->flags & TABLE_TRACE_ACTIVE) {
            continue;                   /* Ignore callbacks that were
                                         * triggered from the active trace
                                         * handler routine. */
        }
        match = 0;
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
        if (match < 2) {
            continue;                   /* Must match both row and
                                         * column.  */
        }
        if (tracePtr->flags & TABLE_TRACE_WHENIDLE) {
            if ((tracePtr->flags & TABLE_TRACE_PENDING) == 0) {
                tracePtr->flags |= TABLE_TRACE_PENDING;
                tracePtr->event = event;
                Tcl_DoWhenIdle(TraceIdleProc, tracePtr);
            }
        } else {
            if (DoTrace(tracePtr, &event) == TCL_BREAK) {
                return;                 /* Don't complete traces on
                                         * break. */
            }
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CallTraces --
 *
 *      Examines the traces set for each client of the table object and
 *      fires any matching traces.
 *
 *      Traces match on row and column indices and flags.
 *      The order is 
 *        1. column traces.
 *        2. row traces.
 *        3. cell (row,column) traces.
 *
 *      If no matching criteria is specified (no tag, key, or tuple
 *      address) then only the bit flag has to match.
 *
 *      If the TABLE_TRACE_FOREIGN_ONLY is set in the handler, it means to
 *      ignore actions that are initiated by that client of the object.
 *      Only actions by other clients are handled.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 * Side Effects:
 *      Traces on the tuple table location may be fired.
 *
 *---------------------------------------------------------------------------
 */
static void
CallTraces(Table *tablePtr, Row *rowPtr, Column *colPtr, unsigned int flags)
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


typedef struct {
    BLT_TABLE table;
    BLT_TABLE_SORT_ORDER *order;
    Blt_HashTable freqTable;
    long numColumns;
    unsigned int flags;
} TableSortData;


static TableSortData sortData;

static int
CompareDictionaryStrings(ClientData clientData, Column *colPtr, Row *rowPtr1, 
                         Row *rowPtr2)
{
    Value *valuePtr1, *valuePtr2;
    
    valuePtr1 = valuePtr2 = NULL;
    if (colPtr->vector != NULL) {
        valuePtr1 = colPtr->vector + rowPtr1->offset;
        if (IsEmptyValue(valuePtr1)) {
            valuePtr1 = NULL;
        }
        valuePtr2 = colPtr->vector + rowPtr2->offset;
        if (IsEmptyValue(valuePtr2)) {
            valuePtr2 = NULL;
        }
    }
    if (IsEmptyValue(valuePtr1)) {
        if (IsEmptyValue(valuePtr2)) {
            return 0;
        }
        return 1;
    } else if (IsEmptyValue(valuePtr2)) {
        return -1;
    }
    assert(valuePtr1 != NULL);
    assert(valuePtr2 != NULL);
    return Blt_DictionaryCompare(GetValueString(valuePtr1),
                                 GetValueString(valuePtr2));
}

static int
CompareFrequencyRows(ClientData clientData, Column *colPtr, Row *rowPtr1, 
                     Row *rowPtr2)
{
    Value *valuePtr1, *valuePtr2;
    Blt_HashEntry *hPtr1, *hPtr2;
    long f1, f2;

    valuePtr1 = valuePtr2 = NULL;
    if (colPtr->vector != NULL) {
        valuePtr1 = colPtr->vector + rowPtr1->offset;
        if (IsEmptyValue(valuePtr1)) {
            valuePtr1 = NULL;
        }
        valuePtr2 = colPtr->vector + rowPtr2->offset;
        if (IsEmptyValue(valuePtr2)) {
            valuePtr2 = NULL;
        }
    }
    if (IsEmptyValue(valuePtr1)) {
        if (IsEmptyValue(valuePtr2)) {
            return 0;
        }
        return 1;
    } else if (IsEmptyValue(valuePtr2)) {
        return -1;
    }
    /* FIXME: Frequency only works for string types. */
    hPtr1 = Blt_FindHashEntry(&sortData.freqTable, GetValueString(valuePtr1));
    hPtr2 = Blt_FindHashEntry(&sortData.freqTable, GetValueString(valuePtr2));
    f1 = (long)Blt_GetHashValue(hPtr1);
    f2 = (long)Blt_GetHashValue(hPtr2);
    return f1 - f2;
}

static int
CompareAsciiStrings(ClientData clientData, Column *colPtr, Row *rowPtr1, 
                    Row *rowPtr2)
{
    Value *valuePtr1, *valuePtr2;

    valuePtr1 = valuePtr2 = NULL;
    if (colPtr->vector != NULL) {
        valuePtr1 = colPtr->vector + rowPtr1->offset;
        if (IsEmptyValue(valuePtr1)) {
            valuePtr1 = NULL;
        }
        valuePtr2 = colPtr->vector + rowPtr2->offset;
        if (IsEmptyValue(valuePtr2)) {
            valuePtr2 = NULL;
        }
    }
    if (IsEmptyValue(valuePtr1)) {
        if (IsEmptyValue(valuePtr2)) {
            return 0;
        }
        return 1;
    } else if (IsEmptyValue(valuePtr2)) {
        return -1;
    }
    return strcmp(GetValueString(valuePtr1), GetValueString(valuePtr2));
}

static int
CompareAsciiStringsIgnoreCase(ClientData clientData, Column *colPtr,
                              Row *rowPtr1, Row *rowPtr2)
{
    Value *valuePtr1, *valuePtr2;

    valuePtr1 = valuePtr2 = NULL;
    if (colPtr->vector != NULL) {
        valuePtr1 = colPtr->vector + rowPtr1->offset;
        if (IsEmptyValue(valuePtr1)) {
            valuePtr1 = NULL;
        }
        valuePtr2 = colPtr->vector + rowPtr2->offset;
        if (IsEmptyValue(valuePtr2)) {
            valuePtr2 = NULL;
        }
    }
    if (IsEmptyValue(valuePtr1)) {
        if (IsEmptyValue(valuePtr2)) {
            return 0;
        }
        return 1;
    } else if (IsEmptyValue(valuePtr2)) {
        return -1;
    }
    return strcasecmp(GetValueString(valuePtr1), GetValueString(valuePtr2));
}

static int
CompareIntegers(ClientData clientData, Column *colPtr, Row *rowPtr1, 
                Row *rowPtr2)
{
    Value *valuePtr1, *valuePtr2;

    valuePtr1 = valuePtr2 = NULL;
    if (colPtr->vector != NULL) {
        valuePtr1 = colPtr->vector + rowPtr1->offset;
        if (IsEmptyValue(valuePtr1)) {
            valuePtr1 = NULL;
        }
        valuePtr2 = colPtr->vector + rowPtr2->offset;
        if (IsEmptyValue(valuePtr2)) {
            valuePtr2 = NULL;
        }
    }
    if (IsEmptyValue(valuePtr1)) {
        if (IsEmptyValue(valuePtr2)) {
            return 0;
        }
        return 1;
    } else if (IsEmptyValue(valuePtr2)) {
        return -1;
    }
    return valuePtr1->datum.l - valuePtr2->datum.l;
}

static int
CompareDoubles(ClientData clientData, Column *colPtr, Row *rowPtr1, 
                    Row *rowPtr2)
{
    Value *valuePtr1, *valuePtr2;


    valuePtr1 = valuePtr2 = NULL;
    if (colPtr->vector != NULL) {
        valuePtr1 = colPtr->vector + rowPtr1->offset;
        if (IsEmptyValue(valuePtr1)) {
            valuePtr1 = NULL;
        }
        valuePtr2 = colPtr->vector + rowPtr2->offset;
        if (IsEmptyValue(valuePtr2)) {
            valuePtr2 = NULL;
        }
    }
    if (IsEmptyValue(valuePtr1)) {
        if (IsEmptyValue(valuePtr2)) {
            return 0;
        }
        return 1;
    } else if (IsEmptyValue(valuePtr2)) {
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

    if ((flags & TABLE_SORT_TYPE_MASK) == TABLE_SORT_AUTO) {
        switch (colPtr->type) {
        case TABLE_COLUMN_TYPE_LONG:
        case TABLE_COLUMN_TYPE_BOOLEAN:
            proc = CompareIntegers;
            break;
        case TABLE_COLUMN_TYPE_TIME:
        case TABLE_COLUMN_TYPE_DOUBLE:
            proc = CompareDoubles;
            break;
        case TABLE_COLUMN_TYPE_STRING:
        default:
            proc = CompareDictionaryStrings;
            break;
        }
    } else {
        switch (flags & TABLE_SORT_TYPE_MASK) {
        case TABLE_SORT_DICTIONARY:
            proc = CompareDictionaryStrings;
            break;
        case TABLE_SORT_FREQUENCY:
            proc = CompareFrequencyRows;
            break;
        default:
        case TABLE_SORT_ASCII:
            if (flags & TABLE_SORT_IGNORECASE) {
                proc = CompareAsciiStringsIgnoreCase;
            } else {
                proc = CompareAsciiStrings;
            }
            break;
        }
    }
    return proc;
}

static Row **
SortRows(Rows *rowsPtr, QSortCompareProc *proc)
{
    long i;
    Row **map;

    /* Make a copy of the current row map. */
    map = Blt_Malloc(sizeof(Row *) * rowsPtr->numAllocated);
    if (map == NULL) {
        return NULL;
    }
    for (i = 0; i < rowsPtr->numAllocated; i++) {
        map[i] = rowsPtr->map[i];
    }
    /* Sort the map and return it. */
    qsort((char *)map, rowsPtr->numUsed, sizeof(Row *), proc);
    return map;
}

static void
ReplaceRowMap(Rows *rowsPtr, Row **map)
{
    long i;

    /* Rethread list according to the map. */
    for (i = 0; i < rowsPtr->numUsed; i++) {
        Row *rowPtr;
        Row *prevPtr, *nextPtr;

        prevPtr = (i > 0) ? map[i-1] : NULL;
        nextPtr = ((i+i) < rowsPtr->numUsed) ? map[i+1] : NULL;
        rowPtr = map[i];
        rowPtr->prevPtr = prevPtr;
        rowPtr->nextPtr = nextPtr;
        rowPtr->index = i;
    }
    rowsPtr->headPtr = map[0];
    rowsPtr->tailPtr = map[rowsPtr->numUsed-1];
    Blt_Free(rowsPtr->map);
    rowsPtr->map = map;
}

static void
ReplaceColumnMap(Columns *columnsPtr, Column **map)
{
    long i;
    Column *prevPtr;
    Column *colPtr;

    /* Rethread column list according to the map. */
    prevPtr = NULL;
    for (i = 0; i < (columnsPtr->numUsed - 1); i++) {
        colPtr = map[i];
        colPtr->index = i;
        colPtr->nextPtr = map[i + 1];
        colPtr->prevPtr = prevPtr;
        prevPtr =  map[i];
    }
    colPtr = map[i];
    colPtr->nextPtr = NULL;
    colPtr->prevPtr = prevPtr;
    columnsPtr->headPtr = map[0];
    columnsPtr->tailPtr = colPtr;
    if (columnsPtr->map != NULL) {
        Blt_Free(columnsPtr->map);
    }
    columnsPtr->map = map;
}

static int
MoveRows(Rows *rowsPtr, Row *srcPtr, Row *destPtr, size_t numRows, int before)
{
    Row *startPtr, *endPtr;

    if (srcPtr == destPtr) {
        return TRUE;
    }
    if (rowsPtr->flags & REINDEX) {
        ResetRowMap(rowsPtr);
    }
    startPtr = srcPtr;
    if ((numRows + srcPtr->index) > rowsPtr->numUsed) {
        return FALSE;
    }
    endPtr = rowsPtr->map[srcPtr->index + numRows - 1];
    if (startPtr->index > endPtr->index) {
        /* Assume that nothing needs to be done. */
        return TRUE;
    }
    /* Check is destination outside the range of rows to be moved. */
    if ((destPtr->index >= startPtr->index) &&
        (destPtr->index <= endPtr->index)) {
        return FALSE;
    }
    /* Unlink the sub-list from the list of rows. */
    if (rowsPtr->headPtr == startPtr) {
        rowsPtr->headPtr = endPtr->nextPtr;
    }
    if (rowsPtr->tailPtr == endPtr) {
        rowsPtr->tailPtr = endPtr->prevPtr;
    }
    if (endPtr->nextPtr != NULL) {
        endPtr->nextPtr->prevPtr = startPtr->prevPtr;
    }
    if (startPtr->prevPtr != NULL) {
        startPtr->prevPtr->nextPtr = endPtr->nextPtr;
    }
    startPtr->prevPtr = endPtr->nextPtr = NULL;
    /* Now attach the detached list to the destination. */
    if (before) {
        if (destPtr->prevPtr == NULL) {
            rowsPtr->headPtr = startPtr;
        } else {
            destPtr->prevPtr->nextPtr = startPtr;
        }
        startPtr->prevPtr = destPtr->prevPtr;
        endPtr->nextPtr = destPtr;
        destPtr->prevPtr = endPtr;
    } else {
        startPtr->prevPtr = destPtr;
        endPtr->nextPtr = destPtr->nextPtr;
        destPtr->prevPtr = startPtr;
    }
    ResetRowMap(rowsPtr);
    return TRUE;
}

static int
MoveColumns(Columns *columnsPtr, Column *srcPtr, Column *destPtr, 
            size_t numColumns, int before)
{
    Column *startPtr, *endPtr;

    if (srcPtr == destPtr) {
        return TRUE;
    }
    if (columnsPtr->flags & REINDEX) {
        ResetColumnMap(columnsPtr);
    }
    startPtr = srcPtr;
    if ((numColumns + srcPtr->index) > columnsPtr->numUsed) {
        /* list of columns extend beyond the end  */
        return FALSE;
    }
    endPtr = columnsPtr->map[srcPtr->index + numColumns - 1];
    if (startPtr->index > endPtr->index) {
        /* Assume that nothing needs to be done. */
        return TRUE;
    }
    /* Check is destination outside the range of columns to be moved. */
    if ((destPtr->index >= startPtr->index) &&
        (destPtr->index <= endPtr->index)) {
        return FALSE;
    }
    /* Unlink the sub-list from the list of columns. */
    if (columnsPtr->headPtr == startPtr) {
        columnsPtr->headPtr = endPtr->nextPtr;
    }
    if (columnsPtr->tailPtr == endPtr) {
        columnsPtr->tailPtr = endPtr->prevPtr;
    }
    if (endPtr->nextPtr != NULL) {
        endPtr->nextPtr->prevPtr = startPtr->prevPtr;
    }
    if (startPtr->prevPtr != NULL) {
        startPtr->prevPtr->nextPtr = endPtr->nextPtr;
    }
    startPtr->prevPtr = endPtr->nextPtr = NULL;

    /* Now attach the detached list to the destination. */
    if (before) { 
        if (destPtr->prevPtr == NULL) {
            columnsPtr->headPtr = startPtr;
        } else {
            destPtr->prevPtr->nextPtr = startPtr;
        }
        startPtr->prevPtr = destPtr->prevPtr;
        endPtr->nextPtr = destPtr;
        destPtr->prevPtr = endPtr;
    } else {
        startPtr->prevPtr = destPtr;
        endPtr->nextPtr = destPtr->nextPtr;
        destPtr->prevPtr = startPtr;
    }
    ResetColumnMap(columnsPtr);
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseDumpRecord --
 *
 *      Gets the next full record in the dump string, returning the
 *      record as a list. Blank lines and comments are ignored.
 *
 * Results: 
 *      TCL_RETURN      The end of the string is reached.
 *      TCL_ERROR       An error occurred and an error message 
 *                      is left in the interpreter result.  
 *      TCL_OK          The next record has been successfully parsed.
 *
 *---------------------------------------------------------------------------
 */
static int
ParseDumpRecord(
    Tcl_Interp *interp,
    char **sp,                          /* (in/out) points to current
                                         * location in in dump
                                         * string. Updated after parsing
                                         * record. */
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
                first = eol;    /* Track first non-whitespace
                                 * character. */
            }
        }
        if (first == NULL) {
            if (*eol == '\0') {
                return TCL_RETURN;
            }
        } else if (*first != '#') {
            break;                      /* Not a comment or blank line. */
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
            return TCL_ERROR;           /* Found EOF (incomplete entry) or
                                         * error. */
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
 *      Reads the next full record from the given channel, returning the
 *      record as a list. Blank lines and comments are ignored.
 *
 * Results: 
 *      TCL_RETURN      The end of the file has been reached.
 *      TCL_ERROR       A read error has occurred and an error message 
 *                      is left in the interpreter result.  
 *      TCL_OK          The next record has been successfully parsed.
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
            break;              /* Not a comment or blank line. */
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
            return TCL_ERROR;           /* Found EOF (incomplete entry) or
                                         * error. */
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
#ifdef notdef
    if (numCols > blt_table_num_columns(table)) {
        long needed;

        needed = numCols - blt_table_num_columns(table);
        if (!GrowColumnStorage(table, needed)) {
            RestoreError(interp, restorePtr);
            Tcl_AppendResult(interp, "can't allocate \"", Blt_Ltoa(needed),
                        "\"", " extra columns.", (char *)NULL);
            return TCL_ERROR;
        }
    }
    if (numRows > blt_table_num_rows(table)) {
        long needed;

        needed = numRows - blt_table_num_rows(table);
        if (!GrowRowMap(table, needed)) {
            RestoreError(interp, restorePtr);
            Tcl_AppendResult(interp, "can't allocate \"", Blt_Ltoa(needed), 
                "\" extra rows.", (char *)NULL);
            return TCL_ERROR;
        }
    }
#endif
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
        int i, argc;
        const char **argv;

        if (Tcl_SplitList(interp, restorePtr->argv[4], &argc, &argv)!= TCL_OK) {
            RestoreError(interp, restorePtr);
            return TCL_ERROR;
        }
        
        for (i = 0; i < argc; i++) {
            if (blt_table_set_column_tag(interp, table, colPtr, argv[i]) 
                != TCL_OK) {
                Tcl_Free((char *)argv);
                return TCL_ERROR;
            }
        }
        Tcl_Free((char *)argv);
    }
    return TCL_OK;
}

static int
RestoreRow(Tcl_Interp *interp, BLT_TABLE table, RestoreData *restorePtr)
{
    BLT_TABLE_ROW row;
    Blt_HashEntry *hPtr;
    const char *label;
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
        const char **argv;
        int argc;
        int i;

        if (Tcl_SplitList(interp, restorePtr->argv[3], &argc, &argv)!= TCL_OK) {
            RestoreError(interp, restorePtr);
            return TCL_ERROR;
        }
        for (i = 0; i < argc; i++) {
            if (blt_table_set_row_tag(interp, table, row, argv[i]) != TCL_OK) {
                Tcl_Free((char *)argv);
                return TCL_ERROR;
            }
        }
        Tcl_Free((char *)argv);
    }
    return TCL_OK;
}

static int
RestoreValue(Tcl_Interp *interp, BLT_TABLE table, RestoreData *restorePtr)
{
    Blt_HashEntry *hPtr;
    int result;
    Row *rowPtr;
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
    rowPtr = Blt_GetHashValue(hPtr);
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
    valuePtr = GetValue(table, rowPtr, colPtr);
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
    if (tablePtr->corePtr->rows.flags & REINDEX) {
        ResetRowMap(&tablePtr->corePtr->rows);
    }
    return tablePtr->corePtr->rows.map;
}

BLT_TABLE_COLUMN *
blt_table_get_column_map(Table *tablePtr)  
{
    if (tablePtr->corePtr->columns.flags & REINDEX) {
        ResetColumnMap(&tablePtr->corePtr->columns);
    }
    return tablePtr->corePtr->columns.map;
}

Blt_Chain
blt_table_get_row_tags(Table *tablePtr, Row *rowPtr)  
{
    Blt_Chain chain;

    chain = Blt_Chain_Create();
    Blt_Tags_AppendTagsToChain(tablePtr->rowTags, rowPtr, chain);
    return chain;
}

Blt_HashTable *
blt_table_get_row_tag_table(Table *tablePtr)  
{
    return Blt_Tags_GetTable(tablePtr->rowTags);
}

Blt_HashTable *
blt_table_get_column_tag_table(Table *tablePtr)  
{
    return Blt_Tags_GetTable(tablePtr->columnTags);
}

Blt_Chain
blt_table_get_column_tags(Table *tablePtr, Column *colPtr)  
{
    Blt_Chain chain;

    chain = Blt_Chain_Create();
    Blt_Tags_AppendTagsToChain(tablePtr->columnTags, colPtr, chain);
    return chain;
}

Blt_Chain
blt_table_get_tagged_columns(Table *tablePtr, const char *tag)  
{
    return Blt_Tags_GetItemList(tablePtr->columnTags, tag);
}

Blt_Chain
blt_table_get_tagged_rows(Table *tablePtr, const char *tag)  
{
    return Blt_Tags_GetItemList(tablePtr->rowTags, tag);
}


int 
blt_table_same_object(Table *tablePtr1, Table *tablePtr2)  
{
    return tablePtr1->corePtr == tablePtr2->corePtr;
}

BLT_TABLE_ROW
blt_table_get_row_by_index(Table *tablePtr, long index)  
{
    if (tablePtr->corePtr->rows.flags & REINDEX) {
        ResetRowMap(&tablePtr->corePtr->rows);
    }
    if ((index >= 0) && (index < blt_table_num_rows(tablePtr))) {
        return blt_table_row(tablePtr, index);
    }
    return NULL;
}

BLT_TABLE_COLUMN
blt_table_get_column_by_index(Table *tablePtr, long index)  
{
    if (tablePtr->corePtr->columns.flags & REINDEX) {
        ResetColumnMap(&tablePtr->corePtr->columns);
    }
    if ((index >= 0) && (index < blt_table_num_columns(tablePtr))) {
        return blt_table_column(tablePtr, index);
    }
    return NULL;
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
    if (c == '@') {
        *sp = string + 1;
        return TABLE_SPEC_TAG;
    } else if ((isdigit(UCHAR(c))) && 
        (Blt_GetLongFromObj((Tcl_Interp *)NULL, objPtr, &index) == TCL_OK)) {
        return TABLE_SPEC_INDEX;
    } else if ((c == 'r') && (strncmp(string, "range:", 6) == 0)) {
        *sp = string + 6;
        return TABLE_SPEC_RANGE;
    } else if ((c == 'i') && (strncmp(string, "index:", 6) == 0)) {
        *sp = string + 6;
        return TABLE_SPEC_INDEX;
    } else if ((c == 'l') && (strncmp(string, "label:", 6) == 0)) {
        *sp = string + 6;
        return TABLE_SPEC_LABEL;
    } else if ((c == 't') && (strncmp(string, "tag:", 4) == 0)) {
        *sp = string + 4;
        return TABLE_SPEC_TAG;
    } else if (blt_table_get_row_by_label(table, string) != NULL) {
        return TABLE_SPEC_LABEL;
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
    return tablePtr->corePtr->columns.headPtr;
}

BLT_TABLE_COLUMN
blt_table_last_column(Table *tablePtr)  
{
    return tablePtr->corePtr->columns.tailPtr;
}

BLT_TABLE_COLUMN
blt_table_next_column(Column *colPtr)  
{
    return colPtr->nextPtr;
}

BLT_TABLE_COLUMN
blt_table_previous_column(Column *colPtr)  
{
    return colPtr->prevPtr;
}

BLT_TABLE_ROW
blt_table_first_row(Table *tablePtr)  
{
    return tablePtr->corePtr->rows.headPtr;
}

BLT_TABLE_ROW
blt_table_last_row(Table *tablePtr)  
{
    return tablePtr->corePtr->rows.tailPtr;
}

BLT_TABLE_ROW
blt_table_next_row(Row *rowPtr)  
{
    return rowPtr->nextPtr;
}

BLT_TABLE_ROW
blt_table_previous_row(Row *rowPtr)  
{
    return rowPtr->prevPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_iterate_rows --
 *
 *      Returns the id of the first row derived from the given tag,
 *      label or index represented in objPtr.  
 *
 * Results:
 *      Returns the row location of the first item.  If no row 
 *      can be found, then -1 is returned and an error message is
 *      left in the interpreter.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_iterate_rows(Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr, 
                      BLT_TABLE_ITERATOR *iterPtr)
{
    const char *tag, *p;
    int result;
    Tcl_Obj *rangeObjPtr;
    long index;
    BLT_TABLE_ROWCOLUMN_SPEC spec;

    memset(iterPtr, 0, sizeof(BLT_TABLE_ITERATOR));
    iterPtr->table = table;
    iterPtr->type = TABLE_ITERATOR_INDEX;
    iterPtr->link = NULL;
    iterPtr->numEntries = 0;

    spec = blt_table_row_spec(table, objPtr, &tag);
    switch (spec) {
    case TABLE_SPEC_INDEX:
        {
            p = Tcl_GetString(objPtr);
            if (p == tag) {
                result = Blt_GetLongFromObj((Tcl_Interp *)NULL, objPtr, &index);
            } else {
                result = Blt_GetLong((Tcl_Interp *)NULL, (char *)tag, &index);
            }
            if (result != TCL_OK) {
                if (interp != NULL) {
                    Tcl_AppendResult(interp, "badly formed row index \"", tag, 
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
            iterPtr->firstPtr = iterPtr->lastPtr = blt_table_row(table, index);
            if (iterPtr->firstPtr != NULL) {
                iterPtr->numEntries = 1;
            }
            iterPtr->tag = tag;
            return TCL_OK;
        }
        break;

    case TABLE_SPEC_LABEL:
        iterPtr->tablePtr = blt_table_row_get_label_table(table, tag);
        if (iterPtr->tablePtr != NULL) {
            iterPtr->type = TABLE_ITERATOR_LABEL;
            iterPtr->tag = tag;
            iterPtr->numEntries = iterPtr->tablePtr->numEntries;
            return TCL_OK;
        }
        break;
        
    case TABLE_SPEC_TAG:
        {
            Blt_Chain chain;

            if (strcmp(tag, "all") == 0) {
                Row *firstPtr, *lastPtr;
                
                iterPtr->type = TABLE_ITERATOR_ALL;
                firstPtr = blt_table_first_row(table);
                lastPtr = blt_table_last_row(table);
                iterPtr->tag = tag;
                if (firstPtr != NULL) {
                    iterPtr->numEntries = lastPtr->index - firstPtr->index + 1;
                }
                iterPtr->firstPtr = firstPtr;
                iterPtr->lastPtr = lastPtr;
                return TCL_OK;
            }
            if (strcmp(tag, "end") == 0) {
                iterPtr->tag = tag;
                iterPtr->firstPtr = iterPtr->lastPtr = 
                    blt_table_last_row(table);
                if (iterPtr->firstPtr != NULL) {
                    iterPtr->numEntries = 1;
                }
                return TCL_OK;
            }
            chain = blt_table_get_tagged_rows(iterPtr->table, tag);
            if (chain != NULL) {
                iterPtr->link = Blt_Chain_FirstLink(chain);
                iterPtr->type = TABLE_ITERATOR_TAG;
                iterPtr->tag = tag;
                iterPtr->chain = NULL;  /* Don't free row tag chain. */
                iterPtr->numEntries = Blt_Chain_GetLength(chain);
            }
        }
        return TCL_OK;
        
    case TABLE_SPEC_RANGE:
        {
            Row *firstPtr, *lastPtr;

            p = strchr(tag, '-');
            if (p == NULL) {
                if (interp != NULL) {
                    Tcl_AppendResult(interp, "bad range specification \"", tag, 
                                     "\"", (char *)NULL);
                }
                return TCL_ERROR;
            }
            rangeObjPtr = Tcl_NewStringObj(tag, p - tag);
            firstPtr = blt_table_get_row(interp, table, rangeObjPtr);
            Tcl_DecrRefCount(rangeObjPtr);
            if (firstPtr == NULL) {
                return TCL_ERROR;
            }
            rangeObjPtr = Tcl_NewStringObj(p + 1, -1);
            lastPtr = blt_table_get_row(interp, table, rangeObjPtr);
            Tcl_DecrRefCount(rangeObjPtr);
            if (lastPtr == NULL) {
                return TCL_ERROR;
            }
            iterPtr->firstPtr = firstPtr;
            iterPtr->lastPtr = lastPtr;
            iterPtr->type = TABLE_ITERATOR_RANGE;
            iterPtr->table = table;
            iterPtr->tag = tag;
            if (firstPtr != NULL) {
                iterPtr->numEntries = lastPtr->index - firstPtr->index + 1;
            }
        }
        return TCL_OK;

    default:
        break;
    }
    if (interp != NULL) {
        Tcl_AppendResult(interp, "unknown row specification \"", tag, 
                         "\" in ", blt_table_name(table), (char *)NULL);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_first_tagged_row --
 *
 *      Returns the id of the next row derived from the given tag.
 *
 * Results:
 *      Returns the row location of the first item.  If no more rows
 *      can be found, then -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_ROW
blt_table_first_tagged_row(BLT_TABLE_ITERATOR *iterPtr)
{
    BLT_TABLE_ROW row;

    switch (iterPtr->type) {
    case TABLE_ITERATOR_TAG:
    case TABLE_ITERATOR_CHAIN:
        /* iterPtr->link is already set by blt_table_row_iterator */
        if (iterPtr->link != NULL) {
            row = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return row;
        }
        break;
    case TABLE_ITERATOR_LABEL:
        {
            Blt_HashEntry *hPtr;

            hPtr = Blt_FirstHashEntry(iterPtr->tablePtr, &iterPtr->cursor);
            if (hPtr == NULL) {
                return NULL;
            }
            return Blt_GetHashValue(hPtr);
        }
        break;

    default:
        if (iterPtr->firstPtr != NULL) {
            Row *rowPtr;

            rowPtr = iterPtr->firstPtr;
            if (rowPtr == iterPtr->lastPtr) {
                iterPtr->nextPtr = NULL;
            } else {
                iterPtr->nextPtr = rowPtr->nextPtr;
            }
            return rowPtr;
        } 
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_next_tagged_row --
 *
 *      Returns the id of the next row derived from the given tag.
 *
 * Results:
 *      Returns the row location of the first item.  If no more rows can be
 *      found, then -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_ROW
blt_table_next_tagged_row(BLT_TABLE_ITERATOR *iterPtr)
{
    BLT_TABLE_ROW row;
            
    switch (iterPtr->type) {
    case TABLE_ITERATOR_CHAIN:
    case TABLE_ITERATOR_TAG:
        if (iterPtr->link != NULL) {
            row = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return row;
        }
        break;
    case TABLE_ITERATOR_LABEL:
        {
            Blt_HashEntry *hPtr;
            
            hPtr = Blt_NextHashEntry(&iterPtr->cursor); 
            if (hPtr == NULL) {
                return NULL;
            }
            return Blt_GetHashValue(hPtr);
        }
        break;
    default:
        if (iterPtr->nextPtr != NULL) {
            Row *rowPtr;

            rowPtr = iterPtr->nextPtr;
            if (rowPtr == iterPtr->lastPtr) {
                iterPtr->nextPtr = NULL;
            } else {
                iterPtr->nextPtr = rowPtr->nextPtr;
            }
            return rowPtr;
        }       
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_row --
 *
 *      Gets the row offset associated the given row index, tag, or label.
 *      This routine is used when you want only one row index.  It's an
 *      error if more than one row is specified (e.g. "all" tag or range
 *      "1-4").  It's also an error if the row tag is empty (no rows are
 *      currently tagged).
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_ROW
blt_table_get_row(Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr)
{
    BLT_TABLE_ITERATOR iter;
    BLT_TABLE_ROW first, next;
    
    if (blt_table_iterate_rows(interp, table, objPtr, &iter) != TCL_OK) {
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
            const char *tag;
            
            blt_table_row_spec(table, objPtr, &tag);
            Tcl_AppendResult(interp, "multiple rows specified by \"", 
                             tag, "\"", (char *)NULL);
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
    if (c == '@') {
        *sp = string + 1;
        return TABLE_SPEC_TAG;
    } else if ((isdigit(c)) && 
        Blt_GetLongFromObj((Tcl_Interp *)NULL, objPtr, &index) == TCL_OK) {
        return TABLE_SPEC_INDEX;
    } else if ((c == 'r') && (strncmp(string, "range:", 6) == 0)) {
        *sp = string + 6;
        return TABLE_SPEC_RANGE;
    } else if ((c == 'i') && (strncmp(string, "index:", 6) == 0)) {
        *sp = string + 6;
        return TABLE_SPEC_INDEX;
    } else if ((c == 'l') && (strncmp(string, "label:", 6) == 0)) {
        *sp = string + 6;
        return TABLE_SPEC_LABEL;
    } else if ((c == 't') && (strncmp(string, "tag:", 4) == 0)) {
        *sp = string + 4;
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
 * blt_table_iterate_columns --
 *
 *      Returns the id of the first column derived from the given tag,
 *      label or index represented in objPtr.
 *
 * Results:
 *      Returns the column location of the first item.  If no column can be
 *      found, then -1 is returned and an error message is left in the
 *      interpreter.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_iterate_columns(Tcl_Interp *interp, BLT_TABLE table, Tcl_Obj *objPtr, 
                         BLT_TABLE_ITERATOR *iterPtr)
{
    const char *tag, *p;
    int result;
    Tcl_Obj *fromObjPtr, *toObjPtr;
    long index;
    BLT_TABLE_ROWCOLUMN_SPEC spec;
    Columns *columnsPtr;

    columnsPtr = blt_table_columns(table);
    if (columnsPtr->flags & REINDEX) {
        ResetColumnMap(columnsPtr);
    }
    iterPtr->table = table;
    iterPtr->type = TABLE_ITERATOR_INDEX;
    iterPtr->link = NULL;
    iterPtr->numEntries = 0;

    spec = blt_table_column_spec(table, objPtr, &tag);
#ifdef notdef
    fprintf(stderr, "iterate_column: tag=%s spec=%d\n", tag, spec);
#endif
    switch (spec) {
    case TABLE_SPEC_INDEX:
        p = Tcl_GetString(objPtr);
        if (p == tag) {
            result = Blt_GetLongFromObj((Tcl_Interp *)NULL, objPtr, &index);
        } else {
            result = Blt_GetLong((Tcl_Interp *)NULL, (char *)tag, &index);
        }
        if (result != TCL_OK) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "badly formed column index \"", 
                        tag, "\"", (char *)NULL);
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
        iterPtr->firstPtr = iterPtr->lastPtr = blt_table_column(table, index);
        if (iterPtr->firstPtr != NULL) {
            iterPtr->numEntries = 1;
        }
        iterPtr->tag = tag;
        return TCL_OK;

    case TABLE_SPEC_LABEL:
        iterPtr->tablePtr = blt_table_column_get_label_table(table, tag);
        if (iterPtr->tablePtr != NULL) {
            iterPtr->type = TABLE_ITERATOR_LABEL;
            iterPtr->tag = tag;
            iterPtr->numEntries = iterPtr->tablePtr->numEntries;
            return TCL_OK;
        }
        break;
        
    case TABLE_SPEC_TAG:
        {
            Blt_Chain chain;

            if (strcmp(tag, "all") == 0) {
                Row *firstPtr, *lastPtr;

                iterPtr->type = TABLE_ITERATOR_ALL;
                iterPtr->tag = tag;
                firstPtr = blt_table_first_column(table);
                lastPtr = blt_table_last_column(table);
                if (firstPtr != NULL) {
                    iterPtr->numEntries = lastPtr->index - firstPtr->index + 1;
                }
                iterPtr->firstPtr = firstPtr;
                iterPtr->lastPtr = lastPtr;
                return TCL_OK;
            }
            if (strcmp(tag, "end") == 0) {
                iterPtr->tag = tag;
                iterPtr->firstPtr = iterPtr->lastPtr = 
                    blt_table_last_column(table);
                if (iterPtr->firstPtr != NULL) {
                    iterPtr->numEntries = 1;
                }
                return TCL_OK;
            }
            
            chain = blt_table_get_tagged_columns(iterPtr->table, tag);
            if (chain != NULL) {
                iterPtr->link = Blt_Chain_FirstLink(chain);
                iterPtr->type = TABLE_ITERATOR_TAG;
                iterPtr->tag = tag;
                iterPtr->chain = NULL;  /* Don't free column tag chain.  */
                iterPtr->numEntries = Blt_Chain_GetLength(chain);
                return TCL_OK;
            }
        }
        break;

    case TABLE_SPEC_RANGE:
        {
            Column *firstPtr, *lastPtr;
            
            p = strchr(tag, '-');
            if (p == NULL) {
                if (interp != NULL) {
                    Tcl_AppendResult(interp, "bad range specification \"", tag, 
                                     "\"", (char *)NULL);
                }
                return TCL_ERROR;
            }
            fromObjPtr = Tcl_NewStringObj(tag, p - tag);
            firstPtr = blt_table_get_column(interp, table, fromObjPtr);
            Tcl_DecrRefCount(fromObjPtr);
            if (firstPtr == NULL) {
                return TCL_ERROR;
            }
            toObjPtr = Tcl_NewStringObj(p + 1, -1);
            lastPtr = blt_table_get_column(interp, table, toObjPtr);
            Tcl_DecrRefCount(toObjPtr);
            if (lastPtr == NULL) {
                return TCL_ERROR;
            }
            iterPtr->firstPtr = firstPtr;
            iterPtr->lastPtr = lastPtr;
            iterPtr->type  = TABLE_ITERATOR_RANGE;
            iterPtr->tag = tag;
            if (firstPtr != NULL) {
                iterPtr->numEntries = lastPtr->index - firstPtr->index + 1;
            }
        }
        return TCL_OK;

    default:
        break;
    }
    if (interp != NULL) {
        Tcl_AppendResult(interp, "unknown column specification \"", 
                         tag, "\" in ", blt_table_name(table),(char *)NULL);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_first_tagged_column --
 *
 *      Returns the first column based upon given iterator.
 *
 * Results:
 *      Returns the column location of the first item.  If no more columns
 *      can be found, then -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_COLUMN
blt_table_first_tagged_column(BLT_TABLE_ITERATOR *iterPtr)
{
    BLT_TABLE_COLUMN col;

    switch (iterPtr->type) {
    case TABLE_ITERATOR_TAG:
    case TABLE_ITERATOR_CHAIN:
        /* iterPtr->link is already set by blt_table_column_iterator */
        if (iterPtr->link != NULL) {
            col = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return col;
        }
        break;
    case TABLE_ITERATOR_LABEL:
        {
            Blt_HashEntry *hPtr;
            
            hPtr = Blt_FirstHashEntry(iterPtr->tablePtr, &iterPtr->cursor);
            if (hPtr == NULL) {
                return NULL;
            }
            return Blt_GetHashValue(hPtr);
        }
        break;
    default:
        if (iterPtr->firstPtr != NULL) {
            Column *colPtr;

            colPtr = iterPtr->firstPtr;
            if (colPtr == iterPtr->lastPtr) {
                iterPtr->nextPtr = NULL;
            } else {
                iterPtr->nextPtr = colPtr->nextPtr;
            }
            return colPtr;
        } 
        break;
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_next_tagged_column --
 *
 *      Returns the column location of the next column using the given
 *      iterator.
 *
 * Results:
 *      Returns the column location of the next item.  If no more columns
 *      can be found, then -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_COLUMN
blt_table_next_tagged_column(BLT_TABLE_ITERATOR *iterPtr)
{
    BLT_TABLE_COLUMN col;
            
    switch (iterPtr->type) {
    case TABLE_ITERATOR_TAG:
    case TABLE_ITERATOR_CHAIN:
        if (iterPtr->link != NULL) {
            col = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return col;
        }
        break;
    case TABLE_ITERATOR_LABEL:
        {
            Blt_HashEntry *hPtr;

            hPtr = Blt_NextHashEntry(&iterPtr->cursor); 
            if (hPtr == NULL) {
                return NULL;
            }
            return Blt_GetHashValue(hPtr);
        }
        break;
    default:
        if (iterPtr->nextPtr != NULL) {
            Column *colPtr;

            colPtr = iterPtr->nextPtr;
            if (colPtr == iterPtr->lastPtr) {
                iterPtr->nextPtr = NULL;
            } else {
                iterPtr->nextPtr = colPtr->nextPtr;
            }
            return colPtr;
        }       
        break;
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

    if (blt_table_iterate_columns(interp, table, objPtr, &iter) != TCL_OK) {
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
            const char *tag;
            
            blt_table_column_spec(table, objPtr, &tag);
            Tcl_AppendResult(interp, "multiple columns specified by \"", 
                tag, "\"", (char *)NULL);
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

        if (blt_table_iterate_columns(interp, table, objv[i], &iter) 
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

        if (blt_table_iterate_rows(interp, table, objv[i], &iter) != TCL_OK){
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
blt_table_iterate_rows_objv(Tcl_Interp *interp, BLT_TABLE table, int objc, 
                            Tcl_Obj *const *objv, BLT_TABLE_ITERATOR *iterPtr)
{
    Blt_Chain chain;

    chain = Blt_Chain_Create();
    if (blt_table_list_rows(interp, table, objc, objv, chain) != TCL_OK) {
        Blt_Chain_Destroy(chain);
        return TCL_ERROR;
    }
    iterPtr->type = TABLE_ITERATOR_CHAIN;
    iterPtr->link = Blt_Chain_FirstLink(chain);
    /* Indicate to free created chain. */
    iterPtr->chain = chain;
    iterPtr->tag = "";
    return TCL_OK;
}

void
blt_table_iterate_all_rows(BLT_TABLE table, BLT_TABLE_ITERATOR *iterPtr)
{
    Row *firstPtr, *lastPtr;
    
    iterPtr->chain = NULL;
    iterPtr->link = NULL;
    iterPtr->numEntries = 0;
    iterPtr->table = table;
    iterPtr->tag = "all";
    iterPtr->type = TABLE_ITERATOR_ALL;

    firstPtr = blt_table_first_row(table);
    lastPtr = blt_table_last_row(table);
    if (firstPtr != NULL) {
        iterPtr->numEntries = lastPtr->index - firstPtr->index + 1;
    } 
    iterPtr->firstPtr = firstPtr;
    iterPtr->lastPtr = lastPtr;
}

int
blt_table_iterate_columns_objv(Tcl_Interp *interp, BLT_TABLE table, int objc, 
                               Tcl_Obj *const *objv,
                               BLT_TABLE_ITERATOR *iterPtr)
{
    Blt_Chain chain;

    chain = Blt_Chain_Create();
    if (blt_table_list_columns(interp, table, objc, objv, chain) != TCL_OK) {
        Blt_Chain_Destroy(chain);
        return TCL_ERROR;
    }
    iterPtr->table = table;
    iterPtr->type = TABLE_ITERATOR_CHAIN;
    iterPtr->link = Blt_Chain_FirstLink(chain);    
    /* Indicate to free created chain. */
    iterPtr->chain = chain;
    iterPtr->tag = "";
    return TCL_OK;
}

void
blt_table_iterate_all_columns(BLT_TABLE table, BLT_TABLE_ITERATOR *iterPtr)
{
    Column *firstPtr, *lastPtr;

    iterPtr->chain = NULL;
    iterPtr->link = NULL;
    iterPtr->numEntries = 0;
    iterPtr->table = table;
    iterPtr->tag = "all";
    iterPtr->type = TABLE_ITERATOR_ALL;

    firstPtr = blt_table_first_column(table);
    lastPtr = blt_table_last_column(table);
    if (firstPtr != NULL) {
        iterPtr->numEntries = lastPtr->index - firstPtr->index + 1;
    } 
    iterPtr->firstPtr = firstPtr;
    iterPtr->lastPtr = lastPtr;
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
 *      Memory is deallocated for the trace.
 *
 * Results:
 *      None.
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
 *      Deletes a trace.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Memory is deallocated for the trace.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_delete_trace(Table *tablePtr, Trace *tracePtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&tablePtr->traces, tracePtr);
    if (hPtr == NULL) {
        return;                         /* Invalid trace token. */
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
 *      Creates a trace for one or more rows/columns in the table based
 *      upon the given criteria.  Whenever a matching action occurs in the
 *      table object, the specified procedure is executed.
 *
 *      The trace may be put into 1 or 2 callback lists: one for read
 *      traces and another for write, create, and/or unset traces.  This is
 *      done to improve performance on reads.  We only examine the list of
 *      read traces, which in most cases will me empty.
 *
 * Results:
 *      Returns a token for the trace.
 *
 * Side Effects:
 *      Memory is allocated for the trace.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_TRACE
blt_table_create_trace(
    Table *tablePtr,                    /* Table to be traced. */
    Row *rowPtr, 
    Column *colPtr,                     /* Cell in table. */
    const char *rowTag, 
    const char *colTag,
    unsigned int flags,                 /* Bit mask indicating what actions
                                         * to trace. */
    BLT_TABLE_TRACE_PROC *proc,         /* Callback procedure for the
                                         * trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)              /* One-word of data passed along
                                         * when the callback is
                                         * executed. */
{
    Blt_HashEntry *hPtr;
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
    hPtr = Blt_CreateHashEntry(&tablePtr->traces, tracePtr, &isNew);
    Tcl_SetHashValue(hPtr, tracePtr);
    assert(isNew);
    tracePtr->table = tablePtr;
    return tracePtr;
}

void
blt_table_trace_column(
    Table *tablePtr,                    /* Table to be traced. */
    Column *colPtr,                     /* Cell in table. */
    unsigned int flags,                 /* Bit mask indicating what actions
                                         * to trace. */
    BLT_TABLE_TRACE_PROC *proc,        /* Callback procedure for the
                                        * trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)              /* One-word of data passed along
                                         * when the callback is
                                         * executed. */
{
    blt_table_create_trace(tablePtr, NULL, colPtr, NULL, NULL, flags,
                           proc, deleteProc, clientData);
}

BLT_TABLE_TRACE
blt_table_create_column_trace(
    Table *tablePtr,                    /* Table to be traced. */
    Column *colPtr,                     /* Cell in table. */
    unsigned int flags,                 /* Bit mask indicating what actions
                                         * to trace. */
    BLT_TABLE_TRACE_PROC *proc,         /* Callback procedure for the
                                         * trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)              /* One-word of data passed along
                                         * when the callback is
                                         * executed. */
{
    return blt_table_create_trace(tablePtr, NULL, colPtr, NULL, NULL, flags,
                proc, deleteProc, clientData);
}

BLT_TABLE_TRACE
blt_table_create_column_tag_trace(
    Table *tablePtr,                    /* Table to be traced. */
    const char *colTag,                 /* Cell in table. */
    unsigned int flags,                 /* Bit mask indicating what actions
                                         * to trace. */
    BLT_TABLE_TRACE_PROC *proc,         /* Callback procedure for the
                                         * trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)              /* One-word of data passed along
                                         * when the callback is
                                         * executed. */
{
    return blt_table_create_trace(tablePtr, NULL, NULL, NULL, colTag, flags,
                proc, deleteProc, clientData);
}

void
blt_table_trace_row(
    Table *tablePtr,                    /* Table to be traced. */
    Row *rowPtr,                        /* Cell in table. */
    unsigned int flags,                 /* Bit mask indicating what actions
                                         * to trace. */
    BLT_TABLE_TRACE_PROC *proc,         /* Callback procedure for the
                                         * trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)              /* One-word of data passed along
                                         * when the callback is
                                         * executed. */
{
    blt_table_create_trace(tablePtr, 
        rowPtr,                         /* Row */
        (BLT_TABLE_COLUMN)NULL,         /* Column */
        (const char *)NULL,             /* Row tag. */
        (const char *)NULL,             /* Column tag. */
        flags,                          /* Flags. */
        proc,                           /* Callback procedure. */
        deleteProc,                     /* Delete callback procedure. */
        clientData);                    /* Data passed to callbacks. */
}

BLT_TABLE_TRACE
blt_table_create_row_trace(
    Table *tablePtr,                    /* Table to be traced. */
    Row *rowPtr,                        /* Cell in table. */
    unsigned int flags,                 /* Bit mask indicating what actions
                                         * to trace. */
    BLT_TABLE_TRACE_PROC *proc,         /* Callback procedure for the
                                         * trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)              /* One-word of data passed along
                                         * when the callback is
                                         * executed. */
{
    return blt_table_create_trace(tablePtr, 
        rowPtr, 
        (Column *)NULL, 
        (const char *)NULL, 
        (const char *)NULL, 
        flags,
        proc, 
        deleteProc, 
        clientData);
}

BLT_TABLE_TRACE
blt_table_create_row_tag_trace(
    Table *tablePtr,                    /* Table to be traced. */
    const char *rowTag,                 /* Cell in table. */
    unsigned int flags,                 /* Bit mask indicating what actions
                                         * to trace. */
    BLT_TABLE_TRACE_PROC *proc,         /* Callback procedure for the
                                         * trace. */
    BLT_TABLE_TRACE_DELETE_PROC *deleteProc, 
    ClientData clientData)              /* One-word of data passed along
                                         * when the callback is
                                         * executed. */
{
    return blt_table_create_trace(tablePtr, NULL, NULL, rowTag, NULL, flags,
                proc, deleteProc, clientData);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_release_tags --
 *
 *      Releases the tag table used by this client.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      If no client is using the table, then it is freed.
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
        Blt_Tags_Reset(&tagsPtr->rowTags);
        tablePtr->rowTags = NULL;
        Blt_Tags_Reset(&tagsPtr->columnTags);
        tablePtr->columnTags = NULL;
        Blt_Free(tagsPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_tags_are_shared --
 *
 *      Returns whether the tag table is shared with another client.
 *
 * Results:
 *      Returns TRUE if the current tag table is shared with another
 *      client, FALSE otherwise.
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
 * blt_table_new_tags --
 *
 *      Releases the old tag table (if it exists) and creates a new empty
 *      tag table.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Possible frees memory for the old tag table (if no one else is
 *      using it) and resets the pointers to the new tag tables.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_new_tags(Table *tablePtr)
{
    if (tablePtr->tags != NULL) {
        blt_table_release_tags(tablePtr);
    }
    tablePtr->tags = NewTags();
    tablePtr->rowTags = &tablePtr->tags->rowTags;
    tablePtr->columnTags = &tablePtr->tags->columnTags;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_forget_row_tag --
 *
 *      Removes a tag from the row tag table.  Row tags are contained in
 *      hash tables keyed by the tag name.  Each table is in turn hashed by
 *      the row index in the row tag table.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Entries for the given tag in the corresponding row in hash tables
 *      may be removed.
 *      
 *---------------------------------------------------------------------------
 */
int
blt_table_forget_row_tag(Tcl_Interp *interp, Table *tablePtr, const char *tag)
{
    if ((strcmp(tag, "all") == 0) || (strcmp(tag, "end") == 0)) {
        return TCL_OK;                  /* Can't forget reserved tags. */
    }
    Blt_Tags_ForgetTag(tablePtr->rowTags, tag);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_forget_column_tag --
 *
 *      Removes a tag from the column tag table.  Column tags are contained
 *      in hash tables keyed by the tag name.  Each table is in turn hashed
 *      by the column offset in the column tag table.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Entries for the given tag in the corresponding column in hash tables
 *      may be removed.
 *      
 *---------------------------------------------------------------------------
 */
int
blt_table_forget_column_tag(Tcl_Interp *interp, Table *tablePtr, 
                            const char *tag)
{
    if ((strcmp(tag, "all") == 0) || (strcmp(tag, "end") == 0)) {
        return TCL_OK;                  /* Can't forget reserved tags. */
    }
    Blt_Tags_ForgetTag(tablePtr->columnTags, tag);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_row_tag --
 *
 *      Associates a tag with a given row.  Individual row tags are stored
 *      in hash tables keyed by the tag name.  Each table is in turn stored
 *      in a hash table keyed by the row location. If the row is NULL, this
 *      indicates to simply create the tag entry.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      A tag is stored for a particular row.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_row_tag(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr, 
                    const char *tag)
{
    long dummy;

    if ((strcmp(tag, "all") == 0) || (strcmp(tag, "end") == 0)) {
        return TCL_OK;          /* Don't need to create reserved tags. */
    }
    if (tag[0] == '\0') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tag, "\" can't be empty.", 
                (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (tag[0] == '-') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tag, 
                "\" can't start with a '-'.", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (Blt_GetLong(NULL, (char *)tag, &dummy) == TCL_OK) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tag, "\" can't be a number.",
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (rowPtr == NULL) {
        Blt_Tags_AddTag(tablePtr->rowTags, tag);
    } else {
        Blt_Tags_AddItemToTag(tablePtr->rowTags, tag, rowPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_column_tag --
 *
 *      Associates a tag with a given column.  Individual column tags are
 *      stored in hash tables keyed by the tag name.  Each table is in turn
 *      stored in a hash table keyed by the column location.  If the column
 *      is NULL, then this means to simply create the tag.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      A tag is stored for a particular column.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_column_tag(Tcl_Interp *interp, Table *tablePtr, Column *colPtr, 
                         const char *tag)
{
    long dummy;

    if ((strcmp(tag, "all") == 0) || (strcmp(tag, "end") == 0)) {
        return TCL_OK;                  /* Don't create reserved tags. */
    }
    if (tag[0] == '\0') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tag, "\" can't be empty.", 
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (tag[0] == '-') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tag, 
                "\" can't start with a '-'.", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (Blt_GetLong(NULL, (char *)tag, &dummy) == TCL_OK) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tag, "\" can't be a number.",
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (colPtr == NULL) {
        Blt_Tags_AddTag(tablePtr->columnTags, tag);
    } else {
        Blt_Tags_AddItemToTag(tablePtr->columnTags, tag, colPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_row_has_tag --
 *
 *      Checks if a tag is associated with the given row.  
 *
 * Results:
 *      Returns TRUE if the tag is found, FALSE otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_row_has_tag(Table *tablePtr, Row *rowPtr, const char *tag)
{
    if (strcmp(tag, "all") == 0) {
        return TRUE;            /* "all" tags matches every row. */
    }
    if (strcmp(tag, "end") == 0) {
        return (blt_table_row_index(tablePtr, rowPtr)==
                (blt_table_num_rows(tablePtr)-1));
    }
    return Blt_Tags_ItemHasTag(tablePtr->rowTags, rowPtr, tag);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_column_has_tag --
 *
 *      Checks if a tag is associated with the given column.  
 *
 * Results:
 *      Returns TRUE if the tag is found, FALSE otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_column_has_tag(Table *tablePtr, Column *colPtr, const char *tag)
{
    if (strcmp(tag, "all") == 0) {
        return TRUE;                    /* "all" tags matches every column. */
    }
    if (strcmp(tag, "end") == 0) {
        return (colPtr == (Column *)tablePtr->corePtr->columns.tailPtr);
    }
    return Blt_Tags_ItemHasTag(tablePtr->columnTags, colPtr, tag);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_unset_row_tag --
 *
 *      Removes a tag from a given row.  
 *
 * Results:
 *      A standard TCL result.  If an error occurred, TCL_ERROR is returned
 *      and the interpreter result contains the error message.
 *
 * Side Effects:
 *      The tag associated with the row is freed.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_unset_row_tag(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr, 
                        const char *tag)
{
    if ((strcmp(tag, "all") == 0) || (strcmp(tag, "end") == 0)) {
        return TCL_OK;                  /* Can't remove reserved tags. */
    } 
    Blt_Tags_RemoveItemFromTag(tablePtr->rowTags, tag, rowPtr);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_unset_column_tag --
 *
 *      Removes a tag from a given column.  
 *
 * Results:
 *      A standard TCL result.  If an error occurred, TCL_ERROR
 *      is returned and the interpreter result contains the error
 *      message.
 *
 * Side Effects:
 *      The tag associated with the column is freed.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_unset_column_tag(Tcl_Interp *interp, Table *tablePtr, Column *colPtr, 
                         const char *tag)
{
    if ((strcmp(tag, "all") == 0) || (strcmp(tag, "end") == 0)) {
        return TCL_OK;                  /* Can't remove reserved tags. */
    } 
    Blt_Tags_RemoveItemFromTag(tablePtr->columnTags, tag, colPtr);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_clear_row_tags --
 *
 *      Removes all tags for a given row.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      All tags associated with the row are freed.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_clear_row_tags(Table *tablePtr, Row *rowPtr)
{
    Blt_Tags_ClearTagsFromItem(tablePtr->rowTags, rowPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_clear_column_tags --
 *
 *      Removes all tags for a given column.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      All tags associated with the column are freed.
 *
 *---------------------------------------------------------------------------
 */
void
blt_table_clear_column_tags(Table *tablePtr, Column *colPtr)
{
    Blt_Tags_ClearTagsFromItem(tablePtr->columnTags, colPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_value --
 *
 *      Gets a scalar Tcl_Obj value from the table at the designated
 *      row, column location.  "Read" traces may be fired *before* the
 *      value is retrieved.  If no value exists at that location,
 *      *objPtrPtr is set to NULL.
 *
 * Results:
 *      A standard TCL result.  Returns TCL_OK if successful accessing
 *      the table location.  If an error occurs, TCL_ERROR is returned
 *      and an error message is left in the interpreter.
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
 *      Sets a scalar Tcl_Obj value in the table at the designated row and
 *      column.  "Write" and possibly "create" or "unset" traces may be fired
 *      *after* the value is set.  If valuePtr is NULL, this indicates to
 *      unset the old value.
 *
 * Results:
 *      A standard TCL result.  Returns TCL_OK if successful setting the value
 *      at the table location.  If an error occurs, TCL_ERROR is returned and
 *      an error message is left in the interpreter.
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
    if (IsEmptyValue(newPtr)) {         /* New value is empty. This is the
                                         * same as unsetting the value. */
        flags |= TABLE_TRACE_UNSETS;
    } else if (IsEmptyValue(valuePtr)) {
        flags |= TABLE_TRACE_CREATES;   /* Old value was empty. */
    } 
    if (newPtr != valuePtr) {
        ResetValue(valuePtr);
        *valuePtr = *newPtr;            /* Copy the value. */
        if ((newPtr->string != NULL) &&
            (newPtr->string != TABLE_VALUE_STORE)) {
            valuePtr->string = Blt_AssertStrdup(newPtr->string);
        }
        CallTraces(tablePtr, rowPtr, colPtr, flags);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_obj --
 *
 *      Gets a scalar Tcl_Obj value from the table at the designated row,
 *      column location.  "Read" traces may be fired *before* the value is
 *      retrieved.  If no value exists at that location, the return value
 *      is NULL.
 *
 * Results:
 *      A standard TCL result.  Returns TCL_OK if successful accessing the
 *      table location.  If an error occurs, TCL_ERROR is returned.
 *
 * -------------------------------------------------------------------------- 
 */
Tcl_Obj *
blt_table_get_obj(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    Value *valuePtr;
    Tcl_Obj *objPtr;

    CallTraces(tablePtr, rowPtr, colPtr, TABLE_TRACE_READS);
    if (IsEmpty(rowPtr, colPtr)) {
        return NULL;
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    objPtr = GetObjFromValue(colPtr->type, valuePtr);
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_obj --
 *
 *      Sets a scalar Tcl_Obj value in the table at the designated row and
 *      column.  "Write" and possibly "create" or "unset" traces may be fired
 *      *after* the value is set.  If valueObjPtr is NULL, this indicates to
 *      unset the old value.
 *
 * Results:
 *      A standard TCL result.  Returns TCL_OK if successful setting the value
 *      at the table location.  If an error occurs, TCL_ERROR is returned and
 *      an error message is left in the interpreter.
 *
 * -------------------------------------------------------------------------- 
 */
int
blt_table_set_obj(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr,
                  Column *colPtr, Tcl_Obj *objPtr)
{
    unsigned int flags;
    Value *valuePtr;

    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    flags = TABLE_TRACE_WRITES;
    if (objPtr == NULL) {               /* New value is empty. This is the 
                                         * same as unsetting the value. */
        flags |= TABLE_TRACE_UNSETS;
    } else if (IsEmptyValue(valuePtr)) {
        flags |= TABLE_TRACE_CREATES;
    } 
    if (SetValueFromObj(interp, colPtr->type, objPtr, valuePtr) != TCL_OK) {
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
 *      Unsets a scalar Tcl_Obj value in the table at the designated row,
 *      column location.  It's okay is there is presently no value at the
 *      location. Unset traces may be fired *before* the value is unset.
 *
 * Results:
 *      A standard TCL result.  Returns TCL_OK if successful unsetting the
 *      value at the table location.  If an error occurs, TCL_ERROR is
 *      returned and an error message is left in the interpreter.
 *
 * -------------------------------------------------------------------------- 
 */
int
blt_table_unset_value(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    if (!IsEmpty(rowPtr, colPtr)) {
        Value *valuePtr;

        valuePtr = GetValue(tablePtr, rowPtr, colPtr);
        CallTraces(tablePtr, rowPtr, colPtr, TABLE_TRACE_UNSETS);
        /* Indicate the keytables need to be regenerated. */
        if (colPtr->flags & TABLE_COLUMN_PRIMARY_KEY) {
            tablePtr->flags |= TABLE_KEYS_DIRTY;
        }
        ResetValue(valuePtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_create --
 *
 *      Creates a table object by the designated name.  It's an error if a
 *      table object already exists by that name.
 *
 * Results:
 *      A standard TCL result.  If successful, a new table object is created
 *      and TCL_OK is returned.  If an object already exists or the table
 *      object can't be allocated, then TCL_ERROR is returned and an error
 *      message is left in the interpreter.
 *
 * Side Effects:
 *      A new table object is created.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_create(
    Tcl_Interp *interp,                 /* Interpreter to report errors back
                                         * to. */
    const char *name,                   /* Name of tuple in namespace.  Object
                                         * must not already exist. */
    Table **tablePtrPtr)                /* (out) Client token of newly created
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
 *      Allocates a token for the table object designated by name.  It's an
 *      error if no table object exists by that name.  The token returned is
 *      passed to various routines to manipulate the object.  Traces and event
 *      notifications are also made through the token.
 *
 * Results:
 *      A new token is returned representing the table object.  
 *
 * Side Effects:
 *      If this is the remaining client, then the table object itself is
 *      freed.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_open(
    Tcl_Interp *interp,                 /* Interpreter to report errors back
                                         * to. */
    const char *name,                   /* Name of table object in
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
    /* By default, share tags with an existing table. Clients can can
     * blt_table_new_tags to get a new tags table. */
    ShareTags(tablePtr, newClientPtr);
    *tablePtrPtr = newClientPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_close --
 *
 *      Releases the tuple token, indicating this the client is no longer
 *      using the object. The client is removed from the tuple object's client
 *      list.  If this is the last client, then the object itself is destroyed
 *      and memory is freed.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      If this is the remaining client, then the table object itself
 *      is freed.
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
 *      Indicates if a table object by the given name exists in either the
 *      current or global namespace.
 *
 * Results:
 *      Returns 1 if a table object exists and 0 otherwise.
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
    notifierPtr->column = NULL;         /* All columns. */
    notifierPtr->row = NULL;            /* All rows. */
    notifierPtr->tag = NULL;            /* No tag. */
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
 *      Creates an event handler using the following three pieces of
 *      information: 
 *              1. C function pointer, 
 *              2. one-word of data passed on each call, and 
 *              3. event mask indicating which events are of interest.  
 *      If an event already exists matching all of the above criteria,
 *      it is repositioned on the end of the event handler list.  This
 *      means that it will be the last to fire.
 *
 * Results:
 *      Returns a pointer to the event handler.
 *
 * Side Effects:
 *      Memory for the event handler is possibly allocated.
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
 *      Creates an event handler using the following three pieces of
 *      information: 
 *              1. C function pointer, 
 *              2. one-word of data passed on each call, and 
 *              3. event mask indicating which events are of interest.  
 *      If an event already exists matching all of the above criteria,
 *      it is repositioned on the end of the event handler list.  This
 *      means that it will be the last to fire.
 *
 * Results:
 *      Returns a pointer to the event handler.
 *
 * Side Effects:
 *      Memory for the event handler is possibly allocated.
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
 *      Creates an event handler using the following three pieces of
 *      information: 
 *              1. C function pointer, 
 *              2. one-word of data passed on each call, and 
 *              3. event mask indicating which events are of interest.  
 *      If an event already exists matching all of the above criteria,
 *      it is repositioned on the end of the event handler list.  This
 *      means that it will be the last to fire.
 *
 * Results:
 *      Returns a pointer to the event handler.
 *
 * Side Effects:
 *      Memory for the event handler is possibly allocated.
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
 *      Creates an event handler using the following three pieces of
 *      information: 
 *              1. C function pointer, 
 *              2. one-word of data passed on each call, and 
 *              3. event mask indicating which events are of interest.  
 *      If an event already exists matching all of the above criteria,
 *      it is repositioned on the end of the event handler list.  This
 *      means that it will be the last to fire.
 *
 * Results:
 *      Returns a pointer to the event handler.
 *
 * Side Effects:
 *      Memory for the event handler is possibly allocated.
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
 *      Creates an event handler using the following three pieces of
 *      information: 
 *              1. C function pointer, 
 *              2. one-word of data passed on each call, and 
 *              3. event mask indicating which events are of interest.  
 *      If an event already exists matching all of the above criteria,
 *      it is repositioned on the end of the event handler list.  This
 *      means that it will be the last to fire.
 *
 * Results:
 *      Returns a pointer to the event handler.
 *
 * Side Effects:
 *      Memory for the event handler is possibly allocated.
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
 *      Removes the event handler designated by following three pieces
 *      of information: 
 *         1. C function pointer, 
 *         2. one-word of data passed on each call, and 
 *         3. event mask indicating which events are of interest.
 *
 * Results:
 *      Nothing.
 *
 * Side Effects:
 *      Memory for the event handler is freed.
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
 *      Returns the offset of the row given its label.  If the row label is
 *      invalid, then -1 is returned.
 *
 * Results:
 *      Returns the offset of the row or -1 if not found.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_ROW
blt_table_get_row_by_label(Table *tablePtr, const char *label)
{
    return FindRowLabel(&tablePtr->corePtr->rows, label);
}


/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_column_by_label --
 *
 *      Returns the offset of the column given its label.  If the column label
 *      is invalid, then -1 is returned.
 *
 * Results:
 *      Returns the offset of the column or -1 if not found.
 *
 *---------------------------------------------------------------------------
 */
BLT_TABLE_COLUMN
blt_table_get_column_by_label(Table *tablePtr, const char *label)
{
    return FindColumnLabel(&tablePtr->corePtr->columns, label);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_column_get_label_table --
 *
 *      Returns the offset of the column given its label.  If the column label
 *      is invalid, then -1 is returned.
 *
 * Results:
 *      Returns the offset of the column or -1 if not found.
 *
 *---------------------------------------------------------------------------
 */
Blt_HashTable *
blt_table_column_get_label_table(Table *tablePtr, const char *label)
{
    return GetColumnLabelTable(&tablePtr->corePtr->columns, label);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_row_get_label_table --
 *
 *      Returns the offset of the column given its label.  If the column label
 *      is invalid, then -1 is returned.
 *
 * Results:
 *      Returns the offset of the column or -1 if not found.
 *
 *---------------------------------------------------------------------------
 */
Blt_HashTable *
blt_table_row_get_label_table(Table *tablePtr, const char *label)
{
    return GetRowLabelTable(&tablePtr->corePtr->rows, label);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_row_label --
 *
 *      Returns the label of the row.  If the row offset is invalid or the row
 *      has no label, then NULL is returned.
 *
 * Results:
 *      Returns the label of the row.
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
    SetRowLabel(&tablePtr->corePtr->rows, rowPtr, label);
    NotifyClients(tablePtr, &event);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_column_label --
 *
 *      Sets the label of the column.  If the column offset is invalid, then
 *      no label is set.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_column_label(Tcl_Interp *interp, Table *tablePtr, Column *colPtr, 
                           const char *label)
{
    BLT_TABLE_NOTIFY_EVENT event;

    InitNotifyEvent(tablePtr, &event);
    event.type = TABLE_NOTIFY_COLUMNS_RELABEL;
    event.column = colPtr;
    SetColumnLabel(&tablePtr->corePtr->columns, colPtr, label);
    NotifyClients(tablePtr, &event);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_set_column_type --
 *
 *      Sets the type of the given column.  
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_column_type(Tcl_Interp *interp, Table *tablePtr, Column *colPtr, 
                          BLT_TABLE_COLUMN_TYPE type)
{
    if (type == colPtr->type) {
        return TCL_OK;                  /* Already the requested type. */
    }
    return SetColumnType(interp, tablePtr, colPtr, type);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_value_exists --
 *
 *      Indicates if a value exists for a given row,column offset in the
 *      tuple.  Note that this routine does not fire read traces.
 *
 * Results:
 *      Returns 1 is a value exists, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_value_exists(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    if ((colPtr == NULL) || (rowPtr == NULL)) {
        return 0;
    }
    return !IsEmpty(rowPtr, colPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * blt_table_extend_rows --
 *
 *      Adds new rows to the table.  Rows are slots in an array of Rows.  The
 *      array grows by doubling its size, so there may be more slots than
 *      needed (# rows).
 *
 * Results:
 *      Returns TCL_OK is the tuple is resized and TCL_ERROR if an not enough
 *      memory was available.
 *
 * Side Effects:
 *      If more rows are needed, the array which holds the tuples is
 *      reallocated by doubling its size.
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
        NotifyRowChanged(tablePtr, row, TABLE_NOTIFY_ROWS_CREATED);
    }
    assert(Blt_Chain_GetLength(chain) > 0);
    Blt_Chain_Destroy(chain);
    return TCL_OK;
}

int
blt_table_delete_row(Table *tablePtr, Row *rowPtr)
{
    UnsetRowValues(tablePtr, rowPtr);
    NotifyRowChanged(tablePtr, rowPtr, TABLE_NOTIFY_ROWS_DELETED);
    Blt_Tags_ClearTagsFromItem(tablePtr->rowTags, rowPtr);
    blt_table_clear_row_traces(tablePtr, rowPtr);
    ClearRowNotifiers(tablePtr, rowPtr);
    tablePtr->flags |= TABLE_KEYS_DIRTY;
    DeleteRow(&tablePtr->corePtr->rows, rowPtr);
    return TCL_OK;
}

BLT_TABLE_ROW
blt_table_create_row(Tcl_Interp *interp, BLT_TABLE table, const char *label)
{
    Row *rowPtr;

    rowPtr = NULL;                      /* Suppress compiler warning. */
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
 *      Move one of more rows to a new location in the tuple.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
blt_table_move_row(Tcl_Interp *interp, Table *tablePtr, Row *srcPtr, 
                   Row *destPtr, size_t count)
{
    if (srcPtr == destPtr) {
        return TCL_OK;          /* Move to the same location. */
    }
    if (!MoveRows(&tablePtr->corePtr->rows, srcPtr, destPtr, count, 1)) {
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
    ReplaceRowMap(&tablePtr->corePtr->rows, map);
    NotifyRowChanged(tablePtr, NULL, TABLE_NOTIFY_ROWS_MOVED);
}

void
blt_table_sort_init(Table *tablePtr, BLT_TABLE_SORT_ORDER *order, 
                    size_t numColumns, unsigned int flags)
{
    size_t i;

    sortData.table = tablePtr;
    sortData.order = order;
    sortData.numColumns = numColumns;
    sortData.flags = flags;
    Blt_InitHashTable(&sortData.freqTable, BLT_STRING_KEYS);
    for (i = 0; i < numColumns; i++) {
        BLT_TABLE_SORT_ORDER *sortPtr;

        sortPtr = order + i;
        sortPtr->clientData = tablePtr;
        sortPtr->cmpProc = blt_table_get_compare_proc(tablePtr, 
                sortPtr->column, flags);
    }
    if (flags & TABLE_SORT_FREQUENCY) {
        Row *rowPtr;
        BLT_TABLE_COLUMN col;

        col = order[0].column;
        for (rowPtr = (Row *)tablePtr->corePtr->rows.headPtr; rowPtr != NULL; 
             rowPtr = rowPtr->nextPtr) {
            Blt_HashEntry *hPtr;
            const char *string;
            int isNew;
            long refCount;

            if (!blt_table_value_exists(tablePtr, rowPtr, col)) {
                continue;
            }
            string = blt_table_get_string(tablePtr, rowPtr, col);
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
    return SortRows(&tablePtr->corePtr->rows, (QSortCompareProc *)CompareRows);
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
    for (/*empty*/; rowPtr != NULL; rowPtr = rowPtr->nextPtr) {
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
 *      Remove the designated column from the table.  The actual space
 *      contained by the column isn't freed.  The map is compressed.
 *      Tcl_Objs stored as column values are released.  Traces and tags
 *      associated with the column are removed.
 *
 * Side Effects:
 *      Traces may fire when column values are unset.  Also notifier events
 *      may be triggered, indicating the column has been deleted.
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
    NotifyColumnChanged(tablePtr, colPtr, TABLE_NOTIFY_COLUMNS_DELETED);
    blt_table_clear_column_traces(tablePtr, colPtr);
    Blt_Tags_ClearTagsFromItem(tablePtr->columnTags, colPtr);
    ClearColumnNotifiers(tablePtr, colPtr);
    DeleteColumn(tablePtr, colPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_extend_columns --
 *
 *      Adds new columns to the table.  Columns are slots in an array of
 *      Columns.  The array columns by doubling its size, so there may be more
 *      slots than needed (# columns).
 *
 * Results:
 *      Returns TCL_OK is the tuple is resized and TCL_ERROR if an
 *      not enough memory was available.
 *
 * Side Effects:
 *      If more columns are needed, the array which holds the tuples is
 *      reallocated by doubling its size.  
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
        NotifyColumnChanged(table, colPtr, TABLE_NOTIFY_COLUMNS_CREATED);
    }
    Blt_Chain_Destroy(chain);
    return TCL_OK;
}

BLT_TABLE_COLUMN
blt_table_create_column(Tcl_Interp *interp, BLT_TABLE table, const char *label)
{
    Column *colPtr;

    colPtr = NULL;                      /* Suppress compiler warning. */
    if (blt_table_extend_columns(interp, table, 1, &colPtr) != TCL_OK) {
        return NULL;
    }
    if (label != NULL) {
        if (blt_table_set_column_label(interp, table, colPtr, label)!=TCL_OK) {
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
    ReplaceColumnMap(&tablePtr->corePtr->columns, map);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_move_column --
 *
 *      Move one of more rows to a new location in the tuple.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
blt_table_move_column(Tcl_Interp *interp, Table *tablePtr, Column *srcPtr, 
                      Column *destPtr, size_t count)
{
    if (srcPtr == destPtr) {
        return TCL_OK;          /* Move to the same location. */
    }
    if (!MoveColumns(&tablePtr->corePtr->columns, srcPtr, destPtr, count, 1)) {
        Tcl_AppendResult(interp, "can't move columns in \"", 
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
 *      Restores data to the given table based upon the dump string.
 *      The dump string should have been generated by blt_table_dump.
 *      Two bit flags may be set.
 *      
 *      TABLE_RESTORE_NO_TAGS   Don't restore tag information.
 *      TABLE_RESTORE_OVERWRITE Look for row and columns with the 
 *                              same label. Overwrite if necessary.
 *
 * Results:
 *      A standard TCL result.  If the restore was successful, TCL_OK
 *      is returned.  Otherwise, TCL_ERROR is returned and an error
 *      message is left in the interpreter result.
 *
 * Side Effects:
 *      New row and columns are created in the table and may possibly
 *      generate event notifier or trace callbacks.
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
    /* Read dump information */
    result = TCL_ERROR;
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
        Tcl_Free((char *)restore.argv);
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
 *      Restores data to the given table based upon the dump file
 *      provided. The dump file should have been generated by
 *      blt_table_dump or blt_table_file_dump.
 *
 *      If the filename starts with an '@', then it is the name of an
 *      already opened channel to be used. Two bit flags may be set.
 *      
 *      TABLE_RESTORE_NO_TAGS   Don't restore tag information.
 *      TABLE_RESTORE_OVERWRITE Look for row and columns with 
 *                              the same label. Overwrite if necessary.
 *
 * Results:
 *      A standard TCL result.  If the restore was successful, TCL_OK
 *      is returned.  Otherwise, TCL_ERROR is returned and an error
 *      message is left in the interpreter result.
 *
 * Side Effects:
 *      Row and columns are created in the table and may possibly
 *      generate trace or notifier event callbacks.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_file_restore(Tcl_Interp *interp, BLT_TABLE table, 
                       const char *fileName, unsigned int flags)
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
            return TCL_ERROR;   /* Can't open dump file. */
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

    result = TCL_ERROR;
    /* Process dump information record by record. */
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
    int i;
    
    for (i = 0; i < tablePtr->numKeys; i++) {
        Column *colPtr;
        
        colPtr = tablePtr->primaryKeys[i];
        colPtr->flags &= ~TABLE_COLUMN_PRIMARY_KEY;
    }
    Blt_Free(tablePtr->primaryKeys);
    tablePtr->primaryKeys = NULL;
    tablePtr->numKeys = 0;
}

static void
FreeKeyTables(Table *tablePtr)
{
    if (tablePtr->keyTables != NULL) {
        int i;

        for (i = 0; i < tablePtr->numKeys; i++) {
            Blt_DeleteHashTable(tablePtr->keyTables + i);
        }
        Blt_Free(tablePtr->keyTables);
        tablePtr->keyTables = NULL;
    }
    if (tablePtr->masterKey != NULL) {
        Blt_Free(tablePtr->masterKey);
        Blt_DeleteHashTable(&tablePtr->masterKeyTable);
        tablePtr->masterKey = NULL;
    }
}

void
blt_table_unset_keys(Table *tablePtr)
{
    FreeKeyTables(tablePtr);
    FreePrimaryKeys(tablePtr);
    tablePtr->flags &= ~(TABLE_KEYS_DIRTY | TABLE_KEYS_UNIQUE);
}

int
blt_table_get_keys(Table *tablePtr, BLT_TABLE_COLUMN **keysPtr)
{
    *keysPtr = tablePtr->primaryKeys;
    return tablePtr->numKeys;
}

static int
SameKeys(int len1, BLT_TABLE_COLUMN *keys1, int len2, BLT_TABLE_COLUMN *keys2)
{
    int i;
    
    if (len1 != len2) {
        return FALSE;
    }
    for (i = 0; i < len1; i++) {
        if (keys1[i] != keys2[i]) {
            fprintf(stderr, "different keys\n");
            return FALSE;
        }
    } 
    return TRUE;
}

int
blt_table_set_keys(Table *tablePtr, int numKeys, BLT_TABLE_COLUMN *keys,
                   int unique)
{
    int i;
    
    if (SameKeys(tablePtr->numKeys, tablePtr->primaryKeys, numKeys, keys)) {
        return TCL_OK;
    }
    if (tablePtr->primaryKeys != NULL) {
        FreePrimaryKeys(tablePtr);
    }
    tablePtr->primaryKeys = keys;
    tablePtr->numKeys = numKeys;
    
    /* Mark the designated columns as primary keys.  This flag is used to
     * check if a primary column is deleted, it's rows are added or
     * changed, or it's values set or unset.  The generated keytables are
     * invalid and need to be regenerated. */
    for (i = 0; i < numKeys; i++) {
        Column *colPtr;
        
        colPtr = keys[i];
        colPtr->flags |= TABLE_COLUMN_PRIMARY_KEY;
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
    Row *rowPtr;
    int i;
    size_t masterKeySize;

    FreeKeyTables(tablePtr);
    tablePtr->flags &= ~TABLE_KEYS_DIRTY;

    /* Create a hashtable for each key. */
    tablePtr->keyTables =
        Blt_Malloc(sizeof(Blt_HashTable) * tablePtr->numKeys);
    if (tablePtr->keyTables == NULL) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't allocated keytables for ",
                Blt_Itoa(tablePtr->numKeys), " keys.", (char *)NULL);
        }
        return TCL_ERROR;
    }
    for (i = 0; i < tablePtr->numKeys; i++) {
        size_t size;
        Column *colPtr;

        colPtr = tablePtr->primaryKeys[i];
        switch (colPtr->type) {
        case TABLE_COLUMN_TYPE_DOUBLE:
        case TABLE_COLUMN_TYPE_TIME:
            size = sizeof(double)/sizeof(int);
            break;
        case TABLE_COLUMN_TYPE_BOOLEAN:
        case TABLE_COLUMN_TYPE_LONG:
            size = BLT_ONE_WORD_KEYS;
            break;
        default:
        case TABLE_COLUMN_TYPE_STRING:
            size = BLT_STRING_KEYS;
            break;
        }
        Blt_InitHashTable(tablePtr->keyTables + i, size);
    }
    /* Generate a master table of key tuple combinations for each row.  
     * We uniquely identify the key by its row in the column. The combination
     * of these keys (in the exact order) is how we find the row. */
    masterKeySize = sizeof(BLT_TABLE_ROW) * tablePtr->numKeys;
    tablePtr->masterKey = Blt_AssertMalloc(masterKeySize);
    Blt_InitHashTable(&tablePtr->masterKeyTable, masterKeySize / sizeof(int));

    /* For each row, create hash entries for the individual key columns,
     * but also for the combined keys for the row.  The hash of the
     * combined keys must be unique. */
    for (rowPtr = (Row *)tablePtr->corePtr->rows.headPtr; rowPtr != NULL; 
         rowPtr = rowPtr->nextPtr) {
        int j;

        for (j = 0; j < tablePtr->numKeys; j++) {
            Blt_HashTable *keyTablePtr;
            Column *colPtr;
            int isNew;
            Blt_HashEntry *hPtr;
            Value *valuePtr;
            
            colPtr = tablePtr->primaryKeys[j];
            if (IsEmpty(rowPtr, colPtr)) {
                break;                  /* Skip this row since one of the
                                         * key values is empty. */
            }
            keyTablePtr = tablePtr->keyTables + j;
            valuePtr = GetValue(tablePtr, rowPtr, colPtr);
            switch (colPtr->type) {
            case TABLE_COLUMN_TYPE_DOUBLE:
            case TABLE_COLUMN_TYPE_TIME:
                hPtr = Blt_CreateHashEntry(keyTablePtr,
                        (char *)&valuePtr->datum.d, &isNew);
                break;
            case TABLE_COLUMN_TYPE_BOOLEAN:
            case TABLE_COLUMN_TYPE_LONG:
                hPtr = Blt_CreateHashEntry(keyTablePtr,
                        (char *)valuePtr->datum.l, &isNew);
                break;
            case TABLE_COLUMN_TYPE_STRING:
            default:
                hPtr = Blt_CreateHashEntry(keyTablePtr,
                                           GetValueString(valuePtr), &isNew);
                break;
            }
            if (isNew) {
                Blt_SetHashValue(hPtr, rowPtr);
            }
            tablePtr->masterKey[j] = Blt_GetHashValue(hPtr);
        }
        if (j == tablePtr->numKeys) {
            Blt_HashEntry *hPtr;
            int isNew;

            /* If we created all the hashkeys necessary for this row, then
             * generate an entry for the row in the master key table. */
            hPtr = Blt_CreateHashEntry(&tablePtr->masterKeyTable, 
                (char *)tablePtr->masterKey, &isNew);
            if (isNew) {
                Blt_SetHashValue(hPtr, rowPtr);
            } else if (tablePtr->flags & TABLE_KEYS_UNIQUE) {
                if (interp != NULL) {
                    BLT_TABLE_ROW dupRow;
        
                    dupRow = Blt_GetHashValue(hPtr);
                    Tcl_AppendResult(interp, "primary keys are not unique:",
                        "rows \"", blt_table_row_label(dupRow), "\" and \"",
                        blt_table_row_label(rowPtr), 
                        "\" have the same keys.", (char *)NULL);
                }
                blt_table_unset_keys(tablePtr);
                return TCL_ERROR;       /* Bail out. Keys aren't unique. */
            }
        }
    }
    tablePtr->flags &= ~TABLE_KEYS_UNIQUE;
    return TCL_OK;
}
            
/*
 *---------------------------------------------------------------------------
 *
 * blt_table_key_lookup --
 *
 *      Look up the (hopefully) unique row that matches all the values
 *      for each of the key columns.  
 *
 * Results:
 *      Returns a standard TCL result.  *rowPtrPtr will return the 
 *      pointer to the found row.  NULL is the row wasn't found.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_key_lookup(Tcl_Interp *interp, Table *tablePtr, int objc, 
                     Tcl_Obj *const *objv, Row **rowPtrPtr)
{
    int i;
    Blt_HashEntry *hPtr;
    
    *rowPtrPtr = NULL;
    if (objc != tablePtr->numKeys) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "wrong # of values: should be ",
                Blt_Itoa(tablePtr->numKeys), " value(s) of ", (char *)NULL);
            for (i = 0; i < tablePtr->numKeys; i++) {
                BLT_TABLE_COLUMN col;

                col = tablePtr->primaryKeys[i];
                Tcl_AppendResult(interp, blt_table_column_label(col), " ", 
                                 (char *)NULL);
            }
        }
        return TCL_ERROR;               /* Wrong # of keys supplied. */
    }
    if (tablePtr->numKeys == 0) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "no primary keys designated",
                         (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (tablePtr->flags & TABLE_KEYS_DIRTY) {
        if (MakeKeyTables(interp, tablePtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    /* Create a master search key by getting the rows from the individual
     * key hash tables for each key.  Use this key to search the master
     * table. */
    for (i = 0; i < tablePtr->numKeys; i++) {
        Blt_HashTable *keyTablePtr;
        Column *colPtr;
        Blt_HashEntry *hPtr;

        colPtr = tablePtr->primaryKeys[i];
        keyTablePtr = tablePtr->keyTables + i;
        switch (colPtr->type) {
        case TABLE_COLUMN_TYPE_DOUBLE:
        case TABLE_COLUMN_TYPE_TIME:
            {
                double dval;

                if (Blt_GetDoubleFromObj(interp, objv[i], &dval) != TCL_OK) {
                    return TCL_ERROR;
                }
                hPtr = Blt_FindHashEntry(keyTablePtr, (char *)&dval);
            }
            break;
        case TABLE_COLUMN_TYPE_BOOLEAN:
            {
                int ival;
                long lval;
                
                if (Tcl_GetBooleanFromObj(interp, objv[i], &ival) != TCL_OK) {
                    return TCL_ERROR;
                }
                lval = ival;
                hPtr = Blt_FindHashEntry(keyTablePtr, (char *)lval);
            }
            break;
        case TABLE_COLUMN_TYPE_LONG:
            {
                long lval;

                if (Blt_GetLongFromObj(interp, objv[i], &lval) != TCL_OK) {
                    return TCL_ERROR;
                }
                hPtr = Blt_FindHashEntry(keyTablePtr, (char *)lval);
            }
            break;
        default:
        case TABLE_COLUMN_TYPE_STRING:
            {
                const char *string;

                string = Tcl_GetString(objv[i]);
                hPtr = Blt_FindHashEntry(keyTablePtr, string);
            }
            break;
        }
        if (hPtr == NULL) {
            return TCL_OK;              /* Can't find one of the keys, so
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
 *      Sets the double value of the selected row, column location in the
 *      table.  No checking is done the see if row and column are valid
 *      (it's assumed they are).  The column type must be "integer" or
 *      "string", otherwise an error is returned.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 * Side Effects:
 *      New tuples may be allocated created.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_long(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr,
                   Column *colPtr, long value)
{
    Value *valuePtr;
    char string[200];

    if ((colPtr->type != TABLE_COLUMN_TYPE_LONG) &&
        (colPtr->type != TABLE_COLUMN_TYPE_STRING)) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "wrong column type \"",
                             blt_table_column_type_to_name(colPtr->type), 
                             "\": should be \"integer\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    ResetValue(valuePtr);
    valuePtr->datum.l = value;
    valuePtr->length = sprintf(string, "%ld", value);
    if (strlen(string) >= TABLE_VALUE_LENGTH) {
        valuePtr->string = Blt_AssertStrdup(string);
    } else {
        strcpy(valuePtr->store, string);
        valuePtr->string = TABLE_VALUE_STORE;
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
 * blt_table_set_boolean --
 *
 *      Sets the double value of the selected row, column location in the
 *      table.  No checking is done the see if row and column are valid
 *      (it's assumed they are).  The column type must be "boolean" or
 *      "string", otherwise an error is returned.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 * Side Effects:
 *      New tuples may be allocated created.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_boolean(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr,
                      Column *colPtr, int value)
{
    Value *valuePtr;
    char string[200];

    if ((colPtr->type != TABLE_COLUMN_TYPE_BOOLEAN) &&
        (colPtr->type != TABLE_COLUMN_TYPE_STRING)) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "wrong column type \"",
                             blt_table_column_type_to_name(colPtr->type), 
                             "\": should be \"boolean\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    ResetValue(valuePtr);
    valuePtr->datum.l = (long)value;
    valuePtr->length = sprintf(string, "%d", value);
    if (strlen(string) >= TABLE_VALUE_LENGTH) {
        valuePtr->string = Blt_AssertStrdup(string);
    } else {
        strcpy(valuePtr->store, string);
        valuePtr->string = valuePtr->store;
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
 * blt_table_set_string_rep --
 *
 *      Sets the value of the selected row, column location in the table.
 *      The row, column location must be within the actual table limits.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 * Side Effects:
 *      New tuples may be allocated created.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_string_rep(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr,
                         Column *colPtr, const char *string, int length)
{
    Value *valuePtr;

    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    ResetValue(valuePtr);
    if (SetValueFromString(interp, colPtr->type, string, length, valuePtr)
        != TCL_OK) {
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
 * blt_table_set_string --
 *
 *      Sets the string value of the selected row, column location in the
 *      table.  No checking is done the see if row and column are valid
 *      (it's assumed they are).  The column type must be "string",
 *      otherwise an error is returned.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 * Side Effects:
 *      New tuples may be allocated created.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_string(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr,
                     Column *colPtr, const char *string, int length)
{
    if (colPtr->type != TABLE_COLUMN_TYPE_STRING) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "column \"", colPtr->label, 
                         "\" is not type string.", (char *)NULL);
        }
        return TCL_ERROR;
    }
    return blt_table_set_string_rep(interp, tablePtr, rowPtr, colPtr, string,
        length);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_append_string --
 *
 *      Append the string value of the selected row, column location in the
 *      table.  No checking is done the see if row and column are valid
 *      (it's assumed they are).  After the string is appended, the column 
 *      is reconverted to its designated type.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 * Side Effects:
 *      New tuples may be allocated created.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_append_string(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr, 
                       Column *colPtr, const char *s, int length)
{
    Value *valuePtr;
    long l;
    double d;
    Tcl_Obj *objPtr;

    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    if (IsEmptyValue(valuePtr)) {
        objPtr = Tcl_NewStringObj(s, length);
    } else {
        objPtr = Tcl_NewStringObj(GetValueString(valuePtr),
                                  GetValueLength(valuePtr));
        Tcl_AppendToObj(objPtr, s, length);
    }
    Tcl_IncrRefCount(objPtr);
    switch (colPtr->type) {
    case TABLE_COLUMN_TYPE_TIME:        /* Time: is a double */
        if (Blt_GetTimeFromObj(interp, objPtr, &d) != TCL_OK) {
            Tcl_DecrRefCount(objPtr);
            return TCL_ERROR;
        }
        valuePtr->datum.d = d;
        break;

    case TABLE_COLUMN_TYPE_DOUBLE:      /* double */
        if (Blt_GetDoubleFromObj(interp, objPtr, &d) != TCL_OK) {
            Tcl_DecrRefCount(objPtr);
            return TCL_ERROR;
        }
        valuePtr->datum.d = d;
        break;
    case TABLE_COLUMN_TYPE_BOOLEAN:      /* boolean */
        {
            int ival;
            
            if (Tcl_GetBooleanFromObj(interp, objPtr, &ival) != TCL_OK) {
                Tcl_DecrRefCount(objPtr);
                return TCL_ERROR;
            }
            valuePtr->datum.l = ival;
        }
        break;
    case TABLE_COLUMN_TYPE_LONG:        /* long */
        if (Blt_GetLongFromObj(interp, objPtr, &l) != TCL_OK) {
            Tcl_DecrRefCount(objPtr);
            return TCL_ERROR;
        }
        valuePtr->datum.l = l;
        break;
    default:                            /* string. */
        break;
    }
    s = Tcl_GetStringFromObj(objPtr, &length);
    ResetValue(valuePtr);
    if (length >= TABLE_VALUE_LENGTH) {
        valuePtr->string = Blt_Strndup(s, length + 1);
        valuePtr->length = length;
    } else {
        strncpy(valuePtr->store, s, length);
        valuePtr->store[length] = '\0';
        valuePtr->string = TABLE_VALUE_STORE;
        valuePtr->length = length;
    }
    Tcl_DecrRefCount(objPtr);

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
 *      Sets the double value of the selected row, column location in the
 *      table.  No checking is done the see if row and column are valid
 *      (it's assumed they are).  The column type must be "double", "time",
 *      or "string", otherwise an error is returned.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_double(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr,
                     Column *colPtr, double value)
{
    Value *valuePtr;
    char string[200];

    if ((colPtr->type != TABLE_COLUMN_TYPE_DOUBLE) &&
        (colPtr->type != TABLE_COLUMN_TYPE_STRING) &&
        (colPtr->type != TABLE_COLUMN_TYPE_TIME)) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "column \"", colPtr->label,
                             "\" is not type double.", (char *)NULL);
        }
        return TCL_ERROR;
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    ResetValue(valuePtr);
    if (!isnan(value)) {
        valuePtr->datum.d = value;
        valuePtr->length = sprintf(string, "%.17g", value);
        if (strlen(string) >= TABLE_VALUE_LENGTH) {
            valuePtr->string = Blt_AssertStrdup(string);
        } else {
            strcpy(valuePtr->store, string);
            valuePtr->string = TABLE_VALUE_STORE;
        }
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
 * blt_table_set_bytes --
 *
 *      Sets the blob value of the selected row, column location in the
 *      table.  No checking is done the see if row and column are valid
 *      (it's assumed they are).  The column type must be "blob" or an
 *      error is returned.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 * Side Effects:
 *      New tuples may be allocated created.  
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_set_bytes(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr,
                    Column *colPtr, const unsigned char *bytes, int numBytes)
{
    Value *valuePtr;

    if (colPtr->type != TABLE_COLUMN_TYPE_BLOB) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "column \"", colPtr->label, 
                         "\" is not type blob.", (char *)NULL);
        }
        return TCL_ERROR;
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    ResetValue(valuePtr);
    if (SetValueFromString(interp, colPtr->type, (const char *)bytes, numBytes,
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
 * blt_table_get_string --
 *
 *      Retrieves the string value of the selected row, column location in
 *      the table.  No checking is done the see if row and column are valid
 *      (it's assumed they are).  The current string representation of the
 *      value is returned.
 *
 * Results:
 *      Returns a string value.  If the value is empty, the default value 
 *      is returned.
 *
 *---------------------------------------------------------------------------
 */
const char *
blt_table_get_string(Table *tablePtr, Row *rowPtr, Column *colPtr)
{
    Value *valuePtr;

    if (IsEmpty(rowPtr, colPtr)) {
        return NULL;
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    return GetValueString(valuePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_double --
 *
 *      Retrieves the double value of the selected row, column location in
 *      the table.  No checking is done the see if row and column are valid
 *      (it's assumed they are).  If the current type of the column isn't
 *      "double", the double value is computed.
 *
 * Results:
 *      Returns a double value.  If the value is empty, the default value 
 *      is returned (NaN).
 *
 *---------------------------------------------------------------------------
 */
double
blt_table_get_double(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr,
                     Column *colPtr)
{
    Value *valuePtr;
    double d;

    if (IsEmpty(rowPtr, colPtr)) {
        return Blt_NaN();
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    if ((colPtr->type == TABLE_COLUMN_TYPE_DOUBLE) ||
        (colPtr->type == TABLE_COLUMN_TYPE_TIME)) {
        return valuePtr->datum.d;
    }
    if (Blt_GetDoubleFromString(interp, GetValueString(valuePtr), &d)
        != TCL_OK) {
        return Blt_NaN();
    }
    return d;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_long --
 *
 *      Retrieves the long integer value of the selected row, column
 *      location in the table.  No checking is done the see if row and
 *      column are valid (it's assumed they are).  If the current type of
 *      the column isn't "integer", the long value is computed.
 *
 * Results:
 *      Returns a long value.  If the value is empty, the default value 
 *      is returned.
 *
 *---------------------------------------------------------------------------
 */
long
blt_table_get_long(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr,
                   Column *colPtr, long defVal)
{
    Value *valuePtr;
    long l;

    if (IsEmpty(rowPtr, colPtr)) {
        return defVal;
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    if (colPtr->type == TABLE_COLUMN_TYPE_LONG) {
        return valuePtr->datum.l;
    }
    if (Blt_GetLong(interp, GetValueString(valuePtr), &l) != TCL_OK) {
        return TCL_ERROR;
    }
    return l;
}

/*
 *---------------------------------------------------------------------------
 *
 * blt_table_get_boolean --
 *
 *      Retrieves the boolean integer value of the selected row, column
 *      location in the table.  No checking is done the see if row and
 *      column are valid (it's assumed they are).  If the current type of
 *      the column isn't "boolean", the boolean value is computed.
 *
 * Results:
 *      Returns a boolean value.  If the value is empty, the default value 
 *      is returned.
 *
 *---------------------------------------------------------------------------
 */
int
blt_table_get_boolean(Tcl_Interp *interp, Table *tablePtr, Row *rowPtr,
                      Column *colPtr, int defVal)
{
    Value *valuePtr;
    int state;

    if (IsEmpty(rowPtr, colPtr)) {
        return defVal;
    }
    valuePtr = GetValue(tablePtr, rowPtr, colPtr);
    if (colPtr->type == TABLE_COLUMN_TYPE_BOOLEAN) {
        return (int)valuePtr->datum.l;
    }
    if (Tcl_GetBoolean(interp, GetValueString(valuePtr), &state) != TCL_OK) {
        return TCL_ERROR;
    }
    return state;
}

void
blt_table_clear(Table *tablePtr)
{
    TableObject *corePtr;

    corePtr = tablePtr->corePtr;
    FreeColumns(corePtr);
    FreeRows(corePtr);
    /* Re-initialize rows and columns. */
    Blt_InitHashTableWithPool(&corePtr->columns.labelTable, BLT_STRING_KEYS);
    Blt_InitHashTableWithPool(&corePtr->rows.labelTable, BLT_STRING_KEYS);
    corePtr->columns.pool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    corePtr->columns.nextColumnId = 1;
    corePtr->rows.freeList = Blt_Chain_Create();
    corePtr->rows.pool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    corePtr->rows.nextRowId = 1;
}

void
blt_table_pack(Table *tablePtr)
{
    Column *colPtr;
    Columns *columnsPtr;
    Rows *rowsPtr;

    rowsPtr = &tablePtr->corePtr->rows;
    columnsPtr = &tablePtr->corePtr->columns;
    /* Replace each vector with one exactly the number of used rows.  */
    for (colPtr = columnsPtr->headPtr; colPtr != NULL; 
         colPtr = colPtr->nextPtr) {
        if (colPtr->vector != NULL) {
            Row *rowPtr;
            Value *vector, *valuePtr;

            vector = Blt_Malloc(rowsPtr->numUsed * sizeof(Value));
            if (vector == NULL) {
            }    
            valuePtr = vector;
            for (rowPtr = rowsPtr->headPtr; rowPtr != NULL;
                 rowPtr = rowPtr->nextPtr) {
                *valuePtr = colPtr->vector[rowPtr->offset];
                valuePtr++;
            }
            Blt_Free(colPtr->vector);
            colPtr->vector = vector;
        }
    }
    {
        size_t count;
        Row *rowPtr;
        
        count = 0;
        for (rowPtr = rowsPtr->headPtr; rowPtr != NULL;
             rowPtr = rowPtr->nextPtr) {
            rowPtr->offset = count;
            rowPtr->index = count;
            count++;
        }
        assert(count == rowsPtr->numUsed);
        if (count > 0) {
            Row **map;
            
            /* Resize the row map to the exact number of rows. */
            if (rowsPtr->map == NULL) {
                map = Blt_Malloc(sizeof(Row *) * count);
            } else {
                map = Blt_Realloc(rowsPtr->map, sizeof(Row *) * count);
            }
            rowsPtr->map = map;
            rowsPtr->numAllocated = count;
        }
        if (rowsPtr->freeList != NULL) {
            /* Dump the free list. */
            Blt_Chain_Destroy(rowsPtr->freeList);
            rowsPtr->freeList = Blt_Chain_Create();
        }
    }
    {
        size_t count;
        Column *colPtr;
        
        count = 0;
        for (colPtr = columnsPtr->headPtr; colPtr != NULL;
             colPtr = colPtr->nextPtr) {
            count++;
        }
        assert(count == columnsPtr->numUsed);
        if (count > 0) {
            Column **map;

            /* Resize the column map to the exact number of columns. */
            if (columnsPtr->map == NULL) {
                map = Blt_Malloc(sizeof(Column *) * count);
            } else {
                map = Blt_Realloc(columnsPtr->map, sizeof(Column *) * count);
            }
            columnsPtr->map = map;
            columnsPtr->numAllocated = count;
        }
    }
}

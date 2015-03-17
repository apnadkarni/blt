/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *
 * bltDataTableSqlite.c --
 *
 *	Copyright 2015 George A Howlett.
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

#include <bltInt.h>

#ifndef NO_DATATABLE

#include "config.h"
#ifdef HAVE_LIBSQLITE
#include <tcl.h>
#include <bltDataTable.h>
#include <bltAlloc.h>
#include <bltSwitch.h>
#include <sqlite3.h>
#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif /* HAVE_MEMORY_H */

DLLEXPORT extern Tcl_AppInitProc blt_table_sqlite_init;
DLLEXPORT extern Tcl_AppInitProc blt_table_sqlite_safe_init;

/*
 * Format	Import		Export
 * csv		file/data	file/data
 * tree		data		data
 * vector	data		data
 * xml		file/data	file/data
 * sqlite	file		file
 */

/*
 * ImportArgs --
 */
typedef struct {
    Tcl_Obj *fileObjPtr;                /* File to read. */
    Tcl_Obj *queryObjPtr;               /* If non-NULL, query to make. */
} ImportArgs;

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-file",  "fileName", (char *)NULL,
	Blt_Offset(ImportArgs, fileObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-query", "string", (char *)NULL,
	Blt_Offset(ImportArgs, queryObjPtr), 0, 0},
    {BLT_SWITCH_END}
};

/*
 * ExportArgs --
 */
typedef struct {
    BLT_TABLE_ITERATOR ri, ci;
    unsigned int flags;
    Tcl_Obj *fileObjPtr;
    Tcl_Obj *tableObjPtr;
    const char *tableName;
} ExportArgs;

#define EXPORT_ROWLABELS	(1<<0)

static Blt_SwitchFreeProc ColumnIterFreeProc;
static Blt_SwitchParseProc ColumnIterSwitchProc;
static Blt_SwitchCustom columnIterSwitch = {
    ColumnIterSwitchProc, NULL, ColumnIterFreeProc, 0,
};
static Blt_SwitchFreeProc RowIterFreeProc;
static Blt_SwitchParseProc RowIterSwitchProc;
static Blt_SwitchCustom rowIterSwitch = {
    RowIterSwitchProc, NULL, RowIterFreeProc, 0,
};

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-name", "tableName", (char *)NULL,
	Blt_Offset(ExportArgs, tableName), 0, 0},
    {BLT_SWITCH_CUSTOM, "-columns",   "columns" ,(char *)NULL,
	Blt_Offset(ExportArgs, ci),   0, 0, &columnIterSwitch},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
	Blt_Offset(ExportArgs, fileObjPtr), 0, 0},
    {BLT_SWITCH_CUSTOM, "-rows",      "rows", (char *)NULL,
	Blt_Offset(ExportArgs, ri),   0, 0, &rowIterSwitch},
    {BLT_SWITCH_BITMASK, "-rowlabels",  "", (char *)NULL,
	Blt_Offset(ExportArgs, flags), 0, EXPORT_ROWLABELS},
    {BLT_SWITCH_END}
};

static BLT_TABLE_EXPORT_PROC ExportSqliteProc;

#define DEF_CLIENT_FLAGS (CLIENT_MULTI_STATEMENTS|CLIENT_MULTI_RESULTS)

static BLT_TABLE_IMPORT_PROC ImportSqliteProc;

/*
 *---------------------------------------------------------------------------
 *
 * ColumnIterFreeProc --
 *
 *	Free the storage associated with the -columns switch.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
ColumnIterFreeProc(ClientData clientData, char *record, int offset, int flags)
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);

    blt_table_free_iterator_objv(iterPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ColumnIterSwitchProc --
 *
 *	Convert a Tcl_Obj representing an offset in the table.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnIterSwitchProc(ClientData clientData, Tcl_Interp *interp,
                     const char *switchName, Tcl_Obj *objPtr, char *record,
                     int offset, int flags)
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);
    BLT_TABLE table;
    Tcl_Obj **objv;
    int objc;

    table = clientData;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (blt_table_iterate_column_objv(interp, table, objc, objv, iterPtr)
	!= TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * RowIterFreeProc --
 *
 *	Free the storage associated with the -rows switch.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
RowIterFreeProc(ClientData clientData, char *record, int offset, int flags)
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);

    blt_table_free_iterator_objv(iterPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * RowIterSwitchProc --
 *
 *	Convert a Tcl_Obj representing an offset in the table.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowIterSwitchProc(ClientData clientData, Tcl_Interp *interp,
                     const char *switchName, Tcl_Obj *objPtr, char *record,
                     int offset, int flags)
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);
    BLT_TABLE table;
    Tcl_Obj **objv;
    int objc;

    table = clientData;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    if (blt_table_iterate_row_objv(interp, table, objc, objv, iterPtr)
	!= TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

static BLT_TABLE_COLUMN_TYPE
SqliteTypeToColumnType(int type) 
{
    switch(type) {
    case SQLITE_BLOB: 
	return TABLE_COLUMN_TYPE_UNKNOWN;
    case SQLITE_INTEGER: 
        return TABLE_COLUMN_TYPE_LONG;
    case SQLITE_FLOAT: 
	return TABLE_COLUMN_TYPE_DOUBLE;
    case SQLITE_NULL: 
        return TABLE_COLUMN_TYPE_STRING;
    default: 
        return TABLE_COLUMN_TYPE_STRING;
    }
}

static int
SqliteConnect(Tcl_Interp *interp, const char *fileName, sqlite3 **dbPtr)
{
    sqlite3  *db;			/* Connection handler. */

    if (sqlite3_open(fileName, &db) != SQLITE_OK) {
	Tcl_AppendResult(interp, "can't open sqlite database", "\"",
                fileName, "\": ", sqlite3_errmsg(db), (char *)NULL);
	sqlite3_close(db);
	return TCL_ERROR;
    }
    *dbPtr = db;
    return TCL_OK;
}

static void
SqliteDisconnect(sqlite3 *db) 
{
    sqlite3_close(db);
}

static int
SqliteImportLabel(Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN col,
                  sqlite3_stmt *stmt, int index)
{
    const char *label;
    int type;
                    
    label = (char *)sqlite3_column_name(stmt, index);
    if (blt_table_set_column_label(interp, table, col, label) != TCL_OK) { 
        return TCL_ERROR;
    }
    type = sqlite3_column_type(stmt, index);
    type = SqliteTypeToColumnType(type);
    if (blt_table_set_column_type(table, col, type) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
SqliteImportValue(Tcl_Interp *interp, BLT_TABLE table, BLT_TABLE_COLUMN col,
                  sqlite3_stmt *stmt, int index)
{
    int type;
    BLT_TABLE_ROW row;

    if (blt_table_extend_rows(interp, table, 1, &row) != TCL_OK) {
        return TCL_ERROR;
    }
    type = sqlite3_column_type(stmt, index);
    switch (type) {
    case SQLITE_INTEGER: 
        {
            long lval;
            
            lval = sqlite3_column_int64(stmt, index);
            if (blt_table_set_long(table, row, col, lval) != TCL_OK) {
                return TCL_ERROR;
            }
        }
        break;
    case SQLITE_FLOAT: 
        {
            double dval;
            
            dval = sqlite3_column_double(stmt, index);
            if (blt_table_set_double(table, row, col, dval) != TCL_OK) {
                return TCL_ERROR;
            }
        }
        break;
    case SQLITE_NULL:
        break;                          /* Ignore empty cells. */
    default:
    case SQLITE_BLOB: 
    case SQLITE_TEXT:
        {
            const char *sval;
            int length;
            
            sval = (const char *)sqlite3_column_text(stmt, index);
            length = sqlite3_column_bytes(stmt, index);
            if (blt_table_set_string(table, row, col, sval, length) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    return TCL_OK;
}

static int
SqliteImport(Tcl_Interp *interp, BLT_TABLE table, sqlite3 *db, 
             ImportArgs *argsPtr) 
{
    int length;
    long numColumns;
    const char *query, *left;
    sqlite3_stmt *stmt;
    int initialized;
    BLT_TABLE_COLUMN *cols;
    int result;
    
    stmt = NULL;
    query = Tcl_GetStringFromObj(argsPtr->queryObjPtr, &length);
    if (sqlite3_prepare_v2(db, query, length, &stmt, &left) != SQLITE_OK) {
        Tcl_AppendResult(interp, "error in query \"", query, "\": ", 
                         sqlite3_errmsg(db), (char *)NULL);
        return TCL_ERROR;
    }
    if (stmt == NULL) {
        Tcl_AppendResult(interp, "empty statement in query \"", query, "\": ", 
                         sqlite3_errmsg(db), (char *)NULL);
        return TCL_ERROR;
    }
    if (left != NULL) {
        Tcl_AppendResult(interp, "extra statements follow query \"", left,
                         "\": ", sqlite3_errmsg(db), (char *)NULL);
        return TCL_ERROR;
    }
    numColumns = sqlite3_column_count(stmt);
    cols = Blt_Malloc(sizeof(BLT_TABLE_COLUMN) * numColumns);
    if (cols == NULL) {
        Tcl_AppendResult(interp, "can't allocate ", Blt_Itoa(numColumns),
                         " column slots.", (char *)NULL);
        goto error;
    }
    if (blt_table_extend_columns(interp, table, numColumns, cols) != TCL_OK) {
        goto error;                     /* Can't create new columns. */
    }
    initialized = FALSE;
    result = SQLITE_OK;
    do {
        result = sqlite3_step(stmt);
        if (result == SQLITE_OK) {
            int i;
            
            if (!initialized) {
                /* After the first step, step the column labels and type.  */
                for (i = 0; i < numColumns; i++) {
                    if (SqliteImportLabel(interp, table, cols[i], stmt, i)
                        != TCL_OK) {
                        goto error;
                    }
                }
                initialized = TRUE;
            }
            for (i = 0; i < numColumns; i++) {
                if (SqliteImportValue(interp, table, cols[i], stmt, i)
                    != TCL_OK) {
                    goto error;
                }
            }
        } else if (result != SQLITE_DONE) {
            Tcl_AppendResult(interp, "step failed \": ", 
                             sqlite3_errmsg(db), (char *)NULL);
            goto error;
        }
    } while (result == SQLITE_OK);
    Blt_Free(cols);
    sqlite3_finalize(stmt);
    return TCL_OK;
 error:
    if (stmt != NULL) {
        sqlite3_finalize(stmt);
    }
    if (cols != NULL) {
        Blt_Free(cols);
    }
    return TCL_ERROR;
}

static int
SqliteCreateTable(Tcl_Interp *interp, sqlite3 *db, BLT_TABLE table,
                  ExportArgs *argsPtr)
{
    BLT_TABLE_COLUMN col;
    Blt_DBuffer dbuffer;
    const char *query;
    int first;
    int length;
    int result;
    sqlite3_stmt *stmt;
    
    dbuffer = Blt_DBuffer_Create();
    Blt_DBuffer_Format(dbuffer, "CREATE TABLE IF NOT EXISTS %s (",
                       argsPtr->tableName);
    first = TRUE;
    for (col = blt_table_first_tagged_column(&argsPtr->ci); col != NULL; 
	 col = blt_table_next_tagged_column(&argsPtr->ci)) {
        int type;
        const char *label;
        
        type = blt_table_column_type(col);
        label = blt_table_column_label(col);
        if (!first) {
            Blt_DBuffer_Format(dbuffer, ", ");
        }
        Blt_DBuffer_Format(dbuffer, "[%s] ", label);
        switch(type) {
        case TABLE_COLUMN_TYPE_UNKNOWN:
            Blt_DBuffer_Format(dbuffer, "BLOB");        break;
        case TABLE_COLUMN_TYPE_LONG:
            Blt_DBuffer_Format(dbuffer, "INTEGER");     break;
        case TABLE_COLUMN_TYPE_DOUBLE:
            Blt_DBuffer_Format(dbuffer, "FLOAT");       break;
        case TABLE_COLUMN_TYPE_STRING:
            Blt_DBuffer_Format(dbuffer, "TEXT");        break;
        }
        first = FALSE;
    }
    Blt_DBuffer_Format(dbuffer, ");"); 
    query = (const char *)Blt_DBuffer_Bytes(dbuffer);
    length = Blt_DBuffer_Length(dbuffer);
    result = sqlite3_prepare_v2(db, query, length, &stmt, NULL);
    Blt_DBuffer_Destroy(dbuffer);
    if (result != SQLITE_OK) {
        Tcl_AppendResult(interp, "error in query \"", query, "\": ", 
                         sqlite3_errmsg(db), (char *)NULL);
        goto error;
    }
    do {
        result = sqlite3_step(stmt);
    } while (result == SQLITE_OK);
    sqlite3_finalize(stmt);
    return TCL_OK;
 error:
    if (stmt != NULL) {
        sqlite3_finalize(stmt);
    }
    return TCL_ERROR;
}

static int
SqliteExportValues(Tcl_Interp *interp, sqlite3 *db, BLT_TABLE table,
                   ExportArgs *argsPtr)
{
    BLT_TABLE_COLUMN col;
    BLT_TABLE_ROW row;
    Blt_DBuffer dbuffer, dbuffer2;
    const char *query;
    int length;
    int result;
    long i;
    sqlite3_stmt *stmt;
    
    dbuffer = Blt_DBuffer_Create();
    dbuffer2 = Blt_DBuffer_Create();
    Blt_DBuffer_Format(dbuffer, "INSERT INTO %s (", argsPtr->tableName);
    Blt_DBuffer_Format(dbuffer2, "(");
    for (i  = 0, col = blt_table_first_tagged_column(&argsPtr->ci);
         col != NULL; col = blt_table_next_tagged_column(&argsPtr->ci), i++) {
        int type;
        const char *label;
        
        type = blt_table_column_type(col);
        label = blt_table_column_label(col);
        if (i > 0) {
            Blt_DBuffer_Format(dbuffer, ", ");
            Blt_DBuffer_Format(dbuffer2, ", ");
        }
        Blt_DBuffer_Format(dbuffer, "[%s] ", label);
        switch(type) {
        case TABLE_COLUMN_TYPE_UNKNOWN:
            Blt_DBuffer_Format(dbuffer, "BLOB");        break;
        case TABLE_COLUMN_TYPE_LONG:
            Blt_DBuffer_Format(dbuffer, "INTEGER");     break;
        case TABLE_COLUMN_TYPE_DOUBLE:
            Blt_DBuffer_Format(dbuffer, "FLOAT");       break;
        case TABLE_COLUMN_TYPE_STRING:
            Blt_DBuffer_Format(dbuffer, "TEXT");        break;
        }
        Blt_DBuffer_Format(dbuffer2, "?%d", i + 1);
    }
    Blt_DBuffer_Format(dbuffer2, ");");
    Blt_DBuffer_Format(dbuffer, ") values ");
    Blt_DBuffer_Concat(dbuffer, dbuffer2);
    Blt_DBuffer_Destroy(dbuffer2);
    query = (const char *)Blt_DBuffer_Bytes(dbuffer);
    length = Blt_DBuffer_Length(dbuffer);
    result = sqlite3_prepare_v2(db, query, length, &stmt, NULL);
    Blt_DBuffer_Destroy(dbuffer);

    if (result != SQLITE_OK) {
        Tcl_AppendResult(interp, "error in query \"", query, "\": ", 
                         sqlite3_errmsg(db), (char *)NULL);
        goto error;
    }
    if (stmt == NULL) {
        Tcl_AppendResult(interp, "empty statement in query \"", query, "\": ", 
                         sqlite3_errmsg(db), (char *)NULL);
        return TCL_ERROR;
    }
    for (row = blt_table_first_tagged_row(&argsPtr->ri); row != NULL; 
	 row = blt_table_next_tagged_row(&argsPtr->ri)) {
        int i;
        
        for (i = 0, col = blt_table_first_tagged_column(&argsPtr->ci);
             col != NULL; col = blt_table_next_tagged_column(&argsPtr->ci),
                 i++) {
            int type;
            
            type = blt_table_column_type(col);
            if (!blt_table_value_exists(table, row, col)) {
                sqlite3_bind_null(stmt, i);
                continue;
            }
            switch(type) {
            case TABLE_COLUMN_TYPE_LONG:
                {
                    long lval;
                    
                    lval = blt_table_get_long(table, row, col, 0);
                    sqlite3_bind_int64(stmt, i, lval);
                }
                break;
            case TABLE_COLUMN_TYPE_DOUBLE:
                {
                    double dval;
                    
                    dval = blt_table_get_double(table, row, col);
                    sqlite3_bind_double(stmt, i, dval);
                }
                break;
            default:
            case TABLE_COLUMN_TYPE_UNKNOWN:
            case TABLE_COLUMN_TYPE_STRING:
                {
                    const char *sval;
                    
                    sval = blt_table_get_string(table, row, col);
                    sqlite3_bind_text(stmt, i, sval, -1, NULL);
                }
                break;
            }
        }
        do {
            result = sqlite3_step(stmt);
        } while (result == SQLITE_OK);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    return TCL_OK;
 error:
    if (stmt != NULL) {
        sqlite3_finalize(stmt);
    }
    return TCL_ERROR;
}

static int
ImportSqliteProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    ImportArgs args;
    sqlite3 *db;
    const char *fileName;
    int result;
    
    memset(&args, 0, sizeof(args));
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
		&args, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (args.fileObjPtr == NULL) {
        Tcl_AppendResult(interp, "-file switch is required.", (char *)NULL);
        return TCL_ERROR;
    }
    if (args.queryObjPtr == NULL) {
	return TCL_OK;
    }
    db = NULL;                          /* Suppress compiler warning. */
    fileName = Tcl_GetString(args.fileObjPtr);
    result = SqliteConnect(interp, fileName, &db);
    if (result == TCL_OK) {
        result = SqliteImport(interp, table, db, &args);
    }
    SqliteDisconnect(db);
    Blt_FreeSwitches(importSwitches, &args, 0);
    return result;
}

static int
ExportSqliteProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    ExportArgs args;
    sqlite3 *db;
    int result;
    const char *fileName;
    
    if ((blt_table_num_rows(table) == 0) ||
        (blt_table_num_columns(table) == 0)) {
        return TCL_OK;                         /* Empty table. */
    }
    memset(&args, 0, sizeof(args));
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, 
		&args, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (args.tableObjPtr != NULL) {
        args.tableName = Tcl_GetString(args.tableObjPtr);
    } else {
        args.tableName = blt_table_name(table);
    }
    if (args.fileObjPtr == NULL) {
        Tcl_AppendResult(interp, "-file switch is required.", (char *)NULL);
        return TCL_ERROR;
    }
    db = NULL;                          /* Suppress compiler warning. */
    fileName = Tcl_GetString(args.fileObjPtr);
    result = SqliteConnect(interp, fileName, &db);
    if (result == TCL_OK) {
        result = SqliteCreateTable(interp, db, table, &args);
    }
    if (result == TCL_OK) {
        result = SqliteExportValues(interp, db, table, &args);
    }
    SqliteDisconnect(db);
    Blt_FreeSwitches(exportSwitches, &args, 0);
    return result;
}

int 
blt_table_sqlite_init(Tcl_Interp *interp)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    };
#endif
#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    };
#else
    if (Tcl_PkgRequire(interp, "blt_tcl", BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    }
#endif    
    if (Tcl_PkgProvide(interp, "blt_datatable_sqlite", BLT_VERSION) != TCL_OK) { 
	return TCL_ERROR;
    }
    return blt_table_register_format(interp,
	"sqlite",		/* Name of format. */
	ImportSqliteProc,	/* Import procedure. */
	ExportSqliteProc);      /* Export procedure. */

}

int 
blt_table_sqlite_safe_init(Tcl_Interp *interp)
{
    return blt_table_sqlite_init(interp);
}

#endif /* HAVE_LIBSQLITE */
#endif /* NO_DATATABLE */


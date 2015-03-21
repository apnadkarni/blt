/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *
 * bltDataTableMysql.c --
 *
 *	Copyright 1998-2015 George A Howlett.
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
#ifdef HAVE_LIBMYSQL
#include <tcl.h>
#include <bltDataTable.h>
#include <bltAlloc.h>
#include <bltSwitch.h>
#include <mysql/mysql.h>
#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif /* HAVE_MEMORY_H */

DLLEXPORT extern Tcl_AppInitProc blt_table_mysql_init;
DLLEXPORT extern Tcl_AppInitProc blt_table_mysql_safe_init;

/*
 * $table import mysql -host $host -password $pw -db $db -port $port \
 *      -query "SELECT..."
 * $table export mysql -host $host -password $pw -db $db -port $port \
 *      -table "my table"
 */
/*
 * ImportArgs --
 */
typedef struct {
    char *host;                         /* If non-NULL, name of remote host
                                         * of mysql server.  Otherwise
                                         * "localhost" is used. */
    char *user;                         /* If non-NULL, name of user
                                         * account to access mysql server.
                                         * Otherwise the current username
                                         * is used. */
    char *pw;                           /* If non-NULL, is password to use
                                         * to access mysql server. */
    char *db;                           /* If non-NULL, name of mysql
                                         * database to access. */
    Tcl_Obj *queryObjPtr;               /* If non-NULL, query to make. */
    int port;                           /* Port number to use. */

    /* Private data. */
    Tcl_Interp *interp;
    unsigned int flags;
    char *buffer;                       /* Buffer to read data into. */
    int numBytes;			/* # of bytes in the buffer. */
} ImportArgs;

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_STRING, "-db",       "dbName", (char *)NULL,
	Blt_Offset(ImportArgs, db), 0, 0},
    {BLT_SWITCH_STRING, "-host",     "hostName", (char *)NULL,
	Blt_Offset(ImportArgs, host), 0, 0},
    {BLT_SWITCH_STRING, "-user",     "userName", (char *)NULL,
	Blt_Offset(ImportArgs, user), 0, 0},
    {BLT_SWITCH_STRING, "-password", "password", (char *)NULL,
	Blt_Offset(ImportArgs, pw), 0, 0},
    {BLT_SWITCH_INT_NNEG, "-port",     "number", (char *)NULL,
	Blt_Offset(ImportArgs, port), 0, 0},
    {BLT_SWITCH_OBJ,    "-query",    "string", (char *)NULL,
	Blt_Offset(ImportArgs, queryObjPtr), 0, 0},
    {BLT_SWITCH_END}
};

/*
 * ExportArgs --
 */
typedef struct {
    char *host;                         /* If non-NULL, name of remote host
                                         * of mysql server.  Otherwise
                                         * "localhost" is used. */
    char *user;                         /* If non-NULL, name of user
                                         * account to access mysql server.
                                         * Otherwise the current username
                                         * is used. */
    char *pw;                           /* If non-NULL, is password to use
                                         * to access mysql server. */
    char *db;                           /* If non-NULL, name of mysql
                                         * database to access. */
    int port;                           /* Port number to use. */

    Tcl_Obj *tableObjPtr;               /* Name of table. */
    const char *tableName;
    BLT_TABLE_ITERATOR ri, ci;
    unsigned int flags;
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
	Blt_Offset(ExportArgs, tableObjPtr), 0, 0},
    {BLT_SWITCH_STRING, "-db",       "dbName", (char *)NULL,
	Blt_Offset(ExportArgs, db), 0, 0},
    {BLT_SWITCH_STRING, "-host",     "hostName", (char *)NULL,
	Blt_Offset(ExportArgs, host), 0, 0},
    {BLT_SWITCH_STRING, "-user",     "userName", (char *)NULL,
	Blt_Offset(ExportArgs, user), 0, 0},
    {BLT_SWITCH_STRING, "-password", "password", (char *)NULL,
	Blt_Offset(ExportArgs, pw), 0, 0},
    {BLT_SWITCH_INT_NNEG, "-port",     "number", (char *)NULL,
	Blt_Offset(ExportArgs, port), 0, 0},
    {BLT_SWITCH_CUSTOM, "-columns",   "columns" ,(char *)NULL,
	Blt_Offset(ExportArgs, ci),   0, 0, &columnIterSwitch},
    {BLT_SWITCH_CUSTOM, "-rows",      "rows", (char *)NULL,
	Blt_Offset(ExportArgs, ri),   0, 0, &rowIterSwitch},
    {BLT_SWITCH_END}
};

static BLT_TABLE_EXPORT_PROC ExportMysqlProc;

#define DEF_CLIENT_FLAGS (CLIENT_MULTI_STATEMENTS|CLIENT_MULTI_RESULTS)

static BLT_TABLE_IMPORT_PROC ImportMysqlProc;


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
MysqlFieldToColumnType(int type) 
{
    switch (type) {
    case FIELD_TYPE_DECIMAL:
    case FIELD_TYPE_TINY:
    case FIELD_TYPE_SHORT:
    case FIELD_TYPE_INT24:
    case FIELD_TYPE_LONG:
    case FIELD_TYPE_LONGLONG:
	return TABLE_COLUMN_TYPE_LONG;
    case FIELD_TYPE_FLOAT:
    case FIELD_TYPE_DOUBLE:
	return TABLE_COLUMN_TYPE_DOUBLE;
    case FIELD_TYPE_TINY_BLOB:
    case FIELD_TYPE_MEDIUM_BLOB:
    case FIELD_TYPE_LONG_BLOB:
    case FIELD_TYPE_BLOB:
	return TABLE_COLUMN_TYPE_BLOB;
    case FIELD_TYPE_NULL:
    case FIELD_TYPE_TIMESTAMP:
    case FIELD_TYPE_DATE:
    case FIELD_TYPE_TIME:
    case FIELD_TYPE_DATETIME:
    case FIELD_TYPE_YEAR:
    case FIELD_TYPE_NEWDATE:
    case FIELD_TYPE_ENUM:
    case FIELD_TYPE_SET:
    case FIELD_TYPE_VAR_STRING:
    case FIELD_TYPE_STRING:
    default:
	return TABLE_COLUMN_TYPE_STRING;
    }
}


static int
MysqlConnect(Tcl_Interp *interp, const char *host, const char *user, 
	     const char *pw, const char *db, unsigned int port,
	     unsigned long flags, MYSQL **cpPtr)			
{
    MYSQL  *cp;			/* Connection handler. */

    cp = mysql_init(NULL); 
    if (cp == NULL) {
	Tcl_AppendResult(interp, "can't initialize mysql connection.",
		(char *)NULL);
	return TCL_ERROR;
    }
    if (host == NULL) {
	host = "localhost";
    }
    cp->reconnect = 1;
#if defined(MYSQL_VERSION_ID) && MYSQL_VERSION_ID >= 32200 /* 3.22 and up */
    if (mysql_real_connect(cp, host, user, pw, db, port, NULL, flags) == NULL) {
	Tcl_AppendResult(interp, "can't connect to mysql server on \"", host, 
			"\": ", mysql_error(cp), (char *)NULL);
	return TCL_ERROR;
    }
#else              /* pre-3.22 */
    if (mysql_real_connect (cp, host, user, pw, port, NULL, flags) == NULL) {
	Tcl_AppendResult(interp, "can't connect to mysql server on \"",
			 host, "\": ", mysql_error(cp), (char *)NULL);
	return TCL_ERROR;
    }
    if (db != NULL) {
	if (mysql_select_db(cp, db) != 0) {
	    Tcl_AppendResult(interp, "can't select database \"", db, "\": ", 
			     mysql_error(cp), (char *)NULL);
	    mysql_close(cp);
	    cp = NULL;
	    return TCL_ERROR;
	}
    }
#endif
    *cpPtr = cp;
    return TCL_OK;
}

static void
MysqlDisconnect(MYSQL *cp) 
{
    mysql_close(cp);
}

static int
MysqlQueryFromObj(Tcl_Interp *interp, MYSQL *cp, Tcl_Obj *objPtr) 
{
    int numBytes;
    const char *query;
    
    query = Tcl_GetStringFromObj(objPtr, &numBytes);
    if (mysql_real_query(cp, query, (unsigned long)numBytes) != 0) {
	Tcl_AppendResult(interp, "error in query \"", query, "\": ", 
			 mysql_error(cp), (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static void
MysqlFreeResults(MYSQL_RES *myResults)
{
    mysql_free_result (myResults);
}

static int
MysqlResults(Tcl_Interp *interp, MYSQL *cp, MYSQL_RES **resultsPtr, 
	     long *numFieldsPtr) 
{
    MYSQL_RES *results;

    results = mysql_store_result(cp);
    if (results != NULL) {
	*numFieldsPtr = mysql_num_fields(results);
    } else if (mysql_field_count(cp) == 0) {
	*numFieldsPtr = 0;
    } else {
	Tcl_AppendResult(interp, "error collecting results: ", mysql_error(cp), 
			 (char *)NULL);
	return TCL_ERROR;
    }
    *resultsPtr = results;
    return TCL_OK;
}


static int
MysqlImportLabels(Tcl_Interp *interp, BLT_TABLE table, MYSQL_RES *myResults, 
		  size_t numCols, BLT_TABLE_COLUMN *cols) 
{
    size_t i;

    for (i = 0; i < numCols; i++) {
	MYSQL_FIELD *fp;
	BLT_TABLE_COLUMN_TYPE type;

	fp = mysql_fetch_field(myResults);
	if (blt_table_set_column_label(interp, table, cols[i], fp->name) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
	type = MysqlFieldToColumnType(fp->type);
	if (blt_table_set_column_type(table, cols[i], type) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

static int
MysqlImportRows(Tcl_Interp *interp, BLT_TABLE table, MYSQL_RES *myResults, 
		size_t numCols, BLT_TABLE_COLUMN *cols) 
{
    size_t numRows;
    size_t i;
    
    numRows = mysql_num_rows(myResults);
    /* First check that there are enough rows in the table to accomodate
     * the new data. Add more if necessary. */
    if (numRows  > blt_table_num_rows(table)) {
        size_t needed;

        needed = numRows - blt_table_num_rows(table);
        if (blt_table_extend_rows(interp, table, needed, NULL) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    for (i = 0; /*empty*/; i++) {
        BLT_TABLE_ROW row;
        MYSQL_ROW myRow;
        size_t j;
        unsigned long *lengths;

        row = blt_table_row(table, i);
	myRow = mysql_fetch_row(myResults);
	if (myRow == NULL) {
	    if (i < numRows) {
		Tcl_AppendResult(interp, "didn't complete fetching all rows",
				 (char *)NULL);
		return TCL_ERROR;
	    }
	    break;
	}
	lengths = mysql_fetch_lengths(myResults);
	for (j = 0; j < numCols; j++) {
	    if (myRow[j] == NULL) {
		continue;		/* Empty value. */
	    }
	    if (blt_table_set_string_rep(table, row, cols[j], myRow[j],
                        lengths[j]) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    return TCL_OK;
}

static int
MysqlCreateTable(Tcl_Interp *interp, MYSQL *conn, BLT_TABLE table,
                 ExportArgs *argsPtr)
{
    BLT_TABLE_COLUMN col;
    Blt_DBuffer dbuffer;
    const char *query;
    int first;
    int result;
    
    dbuffer = Blt_DBuffer_Create();
    Blt_DBuffer_Format(dbuffer, "DROP TABLE IF EXISTS [%s];",
                       argsPtr->tableName);
    query = (const char *)Blt_DBuffer_String(dbuffer);
    result =  mysql_query(conn, query);
    if (result != 0) {
        Tcl_AppendResult(interp, "error in query \"", query, "\": ", 
                         mysql_error(conn), (char *)NULL);
        Blt_DBuffer_Destroy(dbuffer);
        return TCL_ERROR;
    }
    Blt_DBuffer_SetLength(dbuffer, 0);
    Blt_DBuffer_Format(dbuffer, "CREATE TABLE %s (",
                       argsPtr->tableName);
    if (argsPtr->flags & EXPORT_ROWLABELS) {
        Blt_DBuffer_Format(dbuffer, "_rowId TEXT, ");
    }        
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
        case TABLE_COLUMN_TYPE_LONG:
            Blt_DBuffer_Format(dbuffer, "INTEGER");     break;
        case TABLE_COLUMN_TYPE_DOUBLE:
            Blt_DBuffer_Format(dbuffer, "FLOAT");       break;
        default:
        case TABLE_COLUMN_TYPE_STRING:
            Blt_DBuffer_Format(dbuffer, "TEXT");        break;
        }
        first = FALSE;
    }
    Blt_DBuffer_Format(dbuffer, ");"); 
    query = (const char *)Blt_DBuffer_String(dbuffer);
    result =  mysql_query(conn, query);
    if (result != 0) {
        Tcl_AppendResult(interp, "error in query \"", query, "\": ", 
                         mysql_error(conn), (char *)NULL);
        Blt_DBuffer_Destroy(dbuffer);
        return TCL_ERROR;
    }
    Blt_DBuffer_Destroy(dbuffer);
    return TCL_OK;
}

static int
MysqlExportValues(Tcl_Interp *interp, MYSQL *conn, BLT_TABLE table,
                  ExportArgs *argsPtr)
{
    BLT_TABLE_COLUMN col;
    BLT_TABLE_ROW row;
    Blt_DBuffer dbuffer, dbuffer2;
    MYSQL_BIND *bind;
    MYSQL_STMT *stmt;
    const char *query;
    int count, numParams;
    int length, result;
    
    stmt = mysql_stmt_init(conn);
    if (stmt == NULL) {
	Tcl_AppendResult(interp, "can't create statement \": ",
                         mysql_error(conn), (char *)NULL);
	return TCL_ERROR;

    }
    dbuffer = Blt_DBuffer_Create();
    dbuffer2 = Blt_DBuffer_Create();
    Blt_DBuffer_Format(dbuffer, "INSERT INTO %s (", argsPtr->tableName);
    Blt_DBuffer_Format(dbuffer2, "(");
    count = 0;
    if (argsPtr->flags & EXPORT_ROWLABELS) {
        Blt_DBuffer_Format(dbuffer, "_rowId TEXT ");
        Blt_DBuffer_Format(dbuffer2, "?");
        count++;
    }        
    for (col = blt_table_first_tagged_column(&argsPtr->ci); col != NULL;
         col = blt_table_next_tagged_column(&argsPtr->ci)) {
        const char *label;
        
        label = blt_table_column_label(col);
        if (count > 0) {
            Blt_DBuffer_Format(dbuffer, ", ");
            Blt_DBuffer_Format(dbuffer2, ", ");
        }
        Blt_DBuffer_Format(dbuffer, "[%s]", label);
        Blt_DBuffer_Format(dbuffer2, "?");
    }
    Blt_DBuffer_Format(dbuffer2, ");");
    Blt_DBuffer_Format(dbuffer, ") values ");
    Blt_DBuffer_Concat(dbuffer, dbuffer2);
    Blt_DBuffer_Destroy(dbuffer2);
    query = Blt_DBuffer_String(dbuffer);
    length = Blt_DBuffer_Length(dbuffer);
    my_bool true = 1;
    my_bool false = 1;
    
    bind = NULL;
    result = mysql_stmt_prepare(stmt, query, length);
    if (result != 0) {
        Tcl_AppendResult(interp, "error in insert statment \"", query, "\": ", 
                         mysql_error(conn), (char *)NULL);
        Blt_DBuffer_Destroy(dbuffer);
        goto error;
    }
    Blt_DBuffer_Destroy(dbuffer);

    numParams = mysql_stmt_param_count(stmt);
    assert(numParams == count);
    bind = Blt_AssertCalloc(count, sizeof(MYSQL_BIND));
    for (row = blt_table_first_tagged_row(&argsPtr->ri); row != NULL; 
	 row = blt_table_next_tagged_row(&argsPtr->ri)) {
        int count;                      
        
        count = 0;                      /* mysql parameter indices start
                                         * from 0. */
        if (argsPtr->flags & EXPORT_ROWLABELS) {
            const char *label;
                    
            label = blt_table_row_label(row);
            bind[count].buffer_type = MYSQL_TYPE_STRING;
            bind[count].buffer = (char *)label;
            bind[count].buffer_length = strlen(label);
            bind[count].is_null = &false;
            count++;
        }
        for (col = blt_table_first_tagged_column(&argsPtr->ci); col != NULL;
             col = blt_table_next_tagged_column(&argsPtr->ci)) {
            if (!blt_table_value_exists(table, row, col)) {
                bind[count].buffer_type = MYSQL_TYPE_STRING;
                bind[count].buffer = (char *)"";
                bind[count].buffer_length = 0;
                bind[count].is_null = &true;
            } else {
                BLT_TABLE_VALUE value;

                /* Let mysql do the conversions.  This is the safest
                 * way to push data out. */
                value = blt_table_get_value(table, row, col);
                bind[count].buffer_type = MYSQL_TYPE_STRING;
                bind[count].buffer = (char *)blt_table_value_string(value);
                bind[count].buffer_length = blt_table_value_length(value);
                bind[count].is_null = &false;
            }
            count++;
        }
        result = mysql_stmt_bind_param(stmt, bind);
        if (result != 0) {
            Tcl_AppendResult(interp, "error in bind \": ", 
                             mysql_error(conn), (char *)NULL);
            goto error;
        }
    }
    mysql_stmt_close(stmt);
    Blt_Free(bind);
    return TCL_OK;
 error:
    if (stmt != NULL) {
        mysql_stmt_close(stmt);
    }
    if (bind != NULL) {
        Blt_Free(bind);
    }
    return TCL_ERROR;
}

static int
ImportMysqlProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    ImportArgs args;
    MYSQL *conn;
    MYSQL_RES *myResults;
    long numCols;
    int result;
    
    conn = NULL;
    memset(&args, 0, sizeof(args));
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
		&args, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (args.queryObjPtr == NULL) {
        Tcl_AppendResult(interp, "-query switch is required.", (char *)NULL);
	return TCL_ERROR;
    }
    if (MysqlConnect(interp, args.host, args.user, args.pw,
	args.db, args.port, DEF_CLIENT_FLAGS, &conn) != TCL_OK) {
        Blt_FreeSwitches(importSwitches, &args, 0);
	return TCL_ERROR;
    }
    myResults = NULL;
    result = MysqlQueryFromObj(interp, conn, args.queryObjPtr);
    if (result == TCL_OK) {
        result = MysqlResults(interp, conn, &myResults, &numCols);
    }
    if (result == TCL_OK) {
        BLT_TABLE_COLUMN *cols;

        /* Create columns to hold the new values. */
        cols = Blt_AssertMalloc(numCols * sizeof(BLT_TABLE_COLUMN));
        result = blt_table_extend_columns(interp, table, numCols, cols);
        if (result == TCL_OK) {
            result = MysqlImportLabels(interp, table, myResults, numCols, cols);
        }
        if (result == TCL_OK) {
            result = MysqlImportRows(interp, table, myResults, numCols, cols);
        }
        Blt_Free(cols);
    }
    if (myResults != NULL) {
        MysqlFreeResults(myResults);
    }
    MysqlDisconnect(conn);
    Blt_FreeSwitches(importSwitches, &args, 0);
    return result;
}

static int
ExportMysqlProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    ExportArgs args;
    MYSQL *conn;
    int result;
    
    if ((blt_table_num_rows(table) == 0) ||
        (blt_table_num_columns(table) == 0)) {
        return TCL_OK;                  /* Empty table. */
    }
    memset(&args, 0, sizeof(args));
    rowIterSwitch.clientData = table;
    columnIterSwitch.clientData = table;
    blt_table_iterate_all_rows(table, &args.ri);
    blt_table_iterate_all_columns(table, &args.ci);
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, 
		&args, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (args.tableObjPtr != NULL) {
        args.tableName = Tcl_GetString(args.tableObjPtr);
    } else {
        args.tableName = "bltDataTable";
    }
    conn = NULL;                          /* Suppress compiler warning. */
    result = MysqlConnect(interp, args.host, args.user, args.pw,
                args.db, args.port, DEF_CLIENT_FLAGS, &conn);
    if (result == TCL_OK) {
        result = MysqlCreateTable(interp, conn, table, &args);
    }
    if (result == TCL_OK) {
        result = MysqlExportValues(interp, conn, table, &args);
    }
    if (conn != NULL) {
        MysqlDisconnect(conn);
    }
    Blt_FreeSwitches(exportSwitches, &args, 0);
    return result;
}

int 
blt_table_mysql_init(Tcl_Interp *interp)
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
    if (Tcl_PkgProvide(interp, "blt_datatable_mysql", BLT_VERSION) != TCL_OK) { 
	return TCL_ERROR;
    }
    return blt_table_register_format(interp,
	"mysql",                        /* Name of format. */
	ImportMysqlProc,                /* Import procedure. */
	ExportMysqlProc);               /* Export procedure. */

}

int 
blt_table_mysql_safe_init(Tcl_Interp *interp)
{
    return blt_table_mysql_init(interp);
}

#endif /* HAVE_LIBMYSQL */
#endif /* NO_DATATABLE */


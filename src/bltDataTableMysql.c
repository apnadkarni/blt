/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *
 * bltDataTableMysql.c --
 *
 *	Copyright 1998-2005 George A Howlett.
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
 * Format	Import		Export
 * csv		file/data	file/data
 * tree		data		data
 * vector	data		data
 * xml		file/data	file/data
 * mysql	data		data
 *
 * $table import csv -file fileName -data dataString 
 * $table export csv -file defaultFileName 
 * $table import tree $treeName $node ?switches? 
 * $table export tree $treeName $node "label" "label" "label"
 * $table import vector $vecName label $vecName label...
 * $table export vector label $vecName label $vecName...
 * $table import xml -file fileName -data dataString ?switches?
 * $table export xml -file fileName -data dataString ?switches?
 * $table import mysql -host $host -password $pw -db $db -port $port 
 */
/*
 * ImportSwitches --
 */
typedef struct {
    char *host;			/* If non-NULL, name of remote host of
				 * MySql server.  Otherwise "localhost"
				 * is used. */
    char *user;			/* If non-NULL, name of user account
				 * to access MySql server.  Otherwise
				 * the current username is used. */
    char *pw;			/* If non-NULL, is password to use to
				 * access MySql server. */
    char *db;			/* If non-NULL, name of MySql database
				 * to access. */
    Tcl_Obj *query;		/* If non-NULL, query to make. */
    int port;			/* Port number to use. */

    /* Private data. */
    Tcl_Interp *interp;
    unsigned int flags;
    char *buffer;		/* Buffer to read data into. */
    int numBytes;			/* # of bytes in the buffer. */
} ImportSwitches;

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_STRING, "-db",       "dbName", (char *)NULL,
	Blt_Offset(ImportSwitches, db), 0, 0},
    {BLT_SWITCH_STRING, "-host",     "hostName", (char *)NULL,
	Blt_Offset(ImportSwitches, host), 0, 0},
    {BLT_SWITCH_STRING, "-user",     "userName", (char *)NULL,
	Blt_Offset(ImportSwitches, user), 0, 0},
    {BLT_SWITCH_STRING, "-password", "password", (char *)NULL,
	Blt_Offset(ImportSwitches, pw), 0, 0},
    {BLT_SWITCH_INT_NNEG, "-port",     "number", (char *)NULL,
	Blt_Offset(ImportSwitches, port), 0, 0},
    {BLT_SWITCH_OBJ,    "-query",    "string", (char *)NULL,
	Blt_Offset(ImportSwitches, query), 0, 0},
    {BLT_SWITCH_END}
};

#ifdef EXPORT_MYSQL
/*
 * ExportSwitches --
 */
typedef struct {
    Blt_Chain rowChain;
    Blt_Chain colChain;
    Tcl_Obj *rows, *cols;	/* Selected rows and columns to export. */
    unsigned int flags;
    Tcl_Obj *fileObj;
    Tcl_Channel channel;	/* If non-NULL, channel to write output to. */
    Tcl_DString *dsPtr;
    int length;			/* Length of dynamic string. */
    int count;			/* Number of fields in current record. */
    Tcl_Interp *interp;
    char *quote;		/* Quoted string delimiter. */
    char *sep;			/* Separator character. */
} ExportSwitches;

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-columns", "columns", (char *)NULL,
	Blt_Offset(ExportSwitches, cols), 0, 0},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
	Blt_Offset(ExportSwitches, fileObj), 0, 0},
    {BLT_SWITCH_STRING, "-quote", "char", (char *)NULL,
	Blt_Offset(ExportSwitches, quote), 0, 0},
    {BLT_SWITCH_OBJ, "-rows", "rows", (char *)NULL,
	Blt_Offset(ExportSwitches, rows), 0, 0},
    {BLT_SWITCH_STRING, "-separator", "char", (char *)NULL,
	Blt_Offset(ExportSwitches, sep), 0, 0},
    {BLT_SWITCH_END}
};

static BLT_TABLE_EXPORT_PROC ExportMysqlProc;
#endif

#define DEF_CLIENT_FLAGS (CLIENT_MULTI_STATEMENTS|CLIENT_MULTI_RESULTS)

static BLT_TABLE_IMPORT_PROC ImportMysqlProc;

static BLT_TABLE_COLUMN_TYPE
MySqlFieldToColumnType(int type) 
{
    switch (type) {
    case FIELD_TYPE_LONG:
    case FIELD_TYPE_LONGLONG:
	return TABLE_COLUMN_TYPE_LONG;
    case FIELD_TYPE_DECIMAL:
    case FIELD_TYPE_TINY:
    case FIELD_TYPE_SHORT:
    case FIELD_TYPE_INT24:
	return TABLE_COLUMN_TYPE_INT;
    case FIELD_TYPE_FLOAT:
    case FIELD_TYPE_DOUBLE:
	return TABLE_COLUMN_TYPE_DOUBLE;
    case FIELD_TYPE_TINY_BLOB:
    case FIELD_TYPE_MEDIUM_BLOB:
    case FIELD_TYPE_LONG_BLOB:
    case FIELD_TYPE_BLOB:
	return TABLE_COLUMN_TYPE_UNKNOWN;
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
MySqlImportLabels(Tcl_Interp *interp, BLT_TABLE table, MYSQL_RES *myResults, 
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
	type = MySqlFieldToColumnType(fp->type);
	if (blt_table_set_column_type(table, cols[i], type) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

static int
MySqlImportRows(Tcl_Interp *interp, BLT_TABLE table, MYSQL_RES *myResults, 
		size_t numCols, BLT_TABLE_COLUMN *cols) 
{
    size_t numRows;
    size_t i;

    numRows = mysql_num_rows(myResults);
    if (numRows > blt_table_num_rows(table)) {
	size_t needed;

	/* Add the number of rows needed */
	needed = numRows - blt_table_num_rows(table);
	if (blt_table_extend_rows(interp, table, needed, NULL) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    for (i = 0; /*empty*/; i++) {
	BLT_TABLE_ROW row;
	size_t j;
	MYSQL_ROW myRow;
	unsigned long *fieldLengths;

	myRow = mysql_fetch_row(myResults);
	if (myRow == NULL) {
	    if (i < numRows) {
		Tcl_AppendResult(interp, "didn't complete fetching all rows",
				 (char *)NULL);
		return TCL_ERROR;
	    }
	    break;
	}
	fieldLengths = mysql_fetch_lengths(myResults);
	row = blt_table_row(table, i);
	for (j = 0; j < numCols; j++) {
	    int result;
	    Tcl_Obj *objPtr;

	    if (myRow[j] == NULL) {
		continue;		/* Empty value. */
	    }
	    objPtr = Tcl_NewByteArrayObj((unsigned char *)myRow[j], 
					 (int)fieldLengths[j]);
	    Tcl_IncrRefCount(objPtr);
	    result = blt_table_set_obj(table, row, cols[j], objPtr);
	    Tcl_DecrRefCount(objPtr);
	    if (result != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    return TCL_OK;
}

static void
MySqlDisconnect(MYSQL *cp) 
{
    mysql_close(cp);
}

static int
MySqlQueryFromObj(Tcl_Interp *interp, MYSQL *cp, Tcl_Obj *objPtr) 
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
MySqlFreeResults(MYSQL_RES *myResults)
{
    mysql_free_result (myResults);
}

static int
MySqlResults(Tcl_Interp *interp, MYSQL *cp, MYSQL_RES **resultsPtr, 
	     long *nFieldsPtr) 
{
    MYSQL_RES *results;

    results = mysql_store_result(cp);
    if (results != NULL) {
	*nFieldsPtr = mysql_num_fields(results);
    } else if (mysql_field_count(cp) == 0) {
	*nFieldsPtr = 0;
    } else {
	Tcl_AppendResult(interp, "error collecting results: ", mysql_error(cp), 
			 (char *)NULL);
	return TCL_ERROR;
    }
    *resultsPtr = results;
    return TCL_OK;
}

static int
MySqlConnect(Tcl_Interp *interp, const char *host, const char *user, 
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

static int
ImportMysqlProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    ImportSwitches switches;
    MYSQL *cp;
    MYSQL_RES *myResults;
    long numCols;
    BLT_TABLE_COLUMN *cols;

    myResults = NULL;
    cp = NULL;
    cols = NULL;
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
		&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (MySqlConnect(interp, switches.host, switches.user, switches.pw,
	switches.db, switches.port, DEF_CLIENT_FLAGS, &cp) != TCL_OK) {
	goto error;
    }
    if (switches.query == NULL) {
	goto done;
    }
    if (MySqlQueryFromObj(interp, cp, switches.query) != TCL_OK) {
	goto error;
    }
    if (MySqlResults(interp, cp, &myResults, &numCols) != TCL_OK) {
	goto error;
    }
    /* Step 1. Create columns to hold the new values.  Label
     *	       the columns using the title. */
    cols = Blt_AssertMalloc(numCols * sizeof(BLT_TABLE_COLUMN));
    if (blt_table_extend_columns(interp, table, numCols, cols) != TCL_OK) {
	goto error;
    }
    if (MySqlImportLabels(interp, table, myResults, numCols, cols) != TCL_OK) {
	goto error;
    }
    if (MySqlImportRows(interp, table, myResults, numCols, cols) != TCL_OK) {
	goto error;
    }
    Blt_Free(cols);
    MySqlFreeResults(myResults);
 done:
    MySqlDisconnect(cp);
    Blt_FreeSwitches(importSwitches, &switches, 0);
    return TCL_OK;
 error:
    if (myResults != NULL) {
	MySqlFreeResults(myResults);
    }
    if (cols != NULL) {
	Blt_Free(cols);
    }
    if (cp != NULL) {
	MySqlDisconnect(cp);
    }
    Blt_FreeSwitches(importSwitches, &switches, 0);
    return TCL_ERROR;
}

#ifdef EXPORT_MYSQL
static int
ExportMysqlProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    return TCL_OK;
}
#endif

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
        "mysql",		/* Name of format. */
	ImportMysqlProc,	/* Import procedure. */
	NULL);			/* Export procedure. */

}

int 
blt_table_mysql_safe_init(Tcl_Interp *interp)
{
    return blt_table_mysql_init(interp);
}

#endif /* HAVE_LIBMYSQL */
#endif /* NO_DATATABLE */


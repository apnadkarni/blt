/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *
 * bltDataTablePsql.c --
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
#ifdef HAVE_LIBPQ
#include <tcl.h>
#include <bltDataTable.h>
#include <bltAlloc.h>
#include <bltSwitch.h>
#ifdef HAVE_POSTGRESQL_LIBPQ_FE_H
#include <postgresql/libpq-fe.h>
#endif /* HAVE_POSTGRESQL_LIBPQ_FE_H */
#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif /* HAVE_MEMORY_H */

DLLEXPORT extern Tcl_AppInitProc blt_table_psql_init;
DLLEXPORT extern Tcl_AppInitProc blt_table_psql_safe_init;

/*
 * Format	Import		Export
 * csv		file/data	file/data
 * tree		data		data
 * vector	data		data
 * xml		file/data	file/data
 * psql	data		data
 *
 * $table import csv -file fileName -data dataString 
 * $table export csv -file defaultFileName 
 * $table import tree $treeName $node ?switches? 
 * $table export tree $treeName $node "label" "label" "label"
 * $table import vector $vecName label $vecName label...
 * $table export vector label $vecName label $vecName...
 * $table import xml -file fileName -data dataString ?switches?
 * $table export xml -file fileName -data dataString ?switches?
 * $table import psql -host $host -password $pw -db $db -port $port 
 */
/*
 * ImportSwitches --
 */
typedef struct {
    Tcl_Obj *hostObjPtr;                /* If non-NULL, name of remote host
                                         * of Postgres server.  Otherwise
                                         * "localhost" is used. */
    Tcl_Obj *userObjPtr;                /* If non-NULL, name of user
                                         * account to access Postgres server.
                                         * Otherwise the current username
                                         * is used. */
    Tcl_Obj *pwObjPtr;                  /* If non-NULL, is password to use
                                         * to access Postgres server. */
    Tcl_Obj *dbObjPtr;                  /* If non-NULL, name of Postgres SQL
                                         * database to access. */
    Tcl_Obj *queryObjPtr;               /* If non-NULL, query to make. */
    Tcl_Obj *optionsObjPtr;             /* If non-NULL, query to make. */
    Tcl_Obj *portObjPtr;                /* Port number to use. */

    /* Private data. */
    Tcl_Interp *interp;
    unsigned int flags;
    char *buffer;                       /* Buffer to read data into. */
    int numBytes;			/* # of bytes in the buffer. */
} ImportArgs;

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_STRING, "-db",       "dbName", (char *)NULL,
	Blt_Offset(ImportArgs, dbObjPtr), 0, 0},
    {BLT_SWITCH_STRING, "-host",     "hostName", (char *)NULL,
	Blt_Offset(ImportArgs, hostObjPtr), 0, 0},
    {BLT_SWITCH_STRING, "-user",     "userName", (char *)NULL,
	Blt_Offset(ImportArgs, userObjPtr), 0, 0},
    {BLT_SWITCH_STRING, "-password", "password", (char *)NULL,
	Blt_Offset(ImportArgs, pwObjPtr), 0, 0},
    {BLT_SWITCH_INT_NNEG, "-port",     "number", (char *)NULL,
	Blt_Offset(ImportArgs, portObjPtr), 0, 0},
    {BLT_SWITCH_OBJ,    "-query",    "string", (char *)NULL,
	Blt_Offset(ImportArgs, queryObjPtr), 0, 0},
    {BLT_SWITCH_OBJ,    "-options",    "string", (char *)NULL,
	Blt_Offset(ImportArgs, optionsObjPtr), 0, 0},
    {BLT_SWITCH_END}
};

#ifdef EXPORT_PSQL
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

static BLT_TABLE_EXPORT_PROC ExportPsqlProc;
#endif

#define DEF_CLIENT_FLAGS (CLIENT_MULTI_STATEMENTS|CLIENT_MULTI_RESULTS)

static BLT_TABLE_IMPORT_PROC ImportPsqlProc;

static int
PsqlImportLabels(Tcl_Interp *interp, BLT_TABLE table, PGresult *res, 
                 size_t numCols, BLT_TABLE_COLUMN *cols) 
{
    size_t i;

    for (i = 0; i < numCols; i++) {
        const char *label;

	label = PQfname(res, i);
	if (blt_table_set_column_label(interp, table, cols[i], label) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

static int
PsqlImportRows(Tcl_Interp *interp, BLT_TABLE table, PGresult *res, 
		size_t numCols, BLT_TABLE_COLUMN *cols) 
{
    size_t numRows;
    size_t i;

    numRows = PQntuples(res);
    if (numRows > blt_table_num_rows(table)) {
	size_t needed;

	/* Add the number of rows needed */
	needed = numRows - blt_table_num_rows(table);
	if (blt_table_extend_rows(interp, table, needed, NULL) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    for (i = 0; i < numRows; i++) {
	BLT_TABLE_ROW row;
	size_t j;
	row = blt_table_row(table, i);
	for (j = 0; j < numCols; j++) {
	    int length;
	    const char *value;

            value = PQgetvalue(res, i, j);
            length = PQgetlength(res, i, j);
	    if (blt_table_set_string(table, row, cols[j], value, length) !=
                TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    return TCL_OK;
}

static void
PsqlDisconnect(PGconn *conn) 
{
    PQfinish(conn);
}

static PGresult *
PsqlQuery(Tcl_Interp *interp, PGconn *conn, Tcl_Obj *objPtr) 
{
    int numBytes;
    PGresult *res;
    const char *query;
    
    query = Tcl_GetStringFromObj(objPtr, &numBytes);
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	Tcl_AppendResult(interp, "error in query \"", query, "\": ", 
			 PQerrorMessage(conn), (char *)NULL);
        PQclear(res);
	return NULL;
    }
    return res;
}

static int
PsqlConnect(Tcl_Interp *interp, ImportArgs *argsPtr, PGconn **connPtr)			
{
    PGconn  *conn;
    Tcl_Obj *objPtr;
    
    objPtr = Tcl_NewStringObj("", -1);
    Tcl_IncrRefCount(objPtr);
    Tcl_AppendToObj(objPtr, " host=", 6);
    if (argsPtr->hostObjPtr != NULL) {
        Tcl_AppendObjToObj(objPtr, argsPtr->hostObjPtr);
    } else {
        Tcl_AppendToObj(objPtr, "localhost", 9);
    }
    if (argsPtr->userObjPtr != NULL) {
        Tcl_AppendToObj(objPtr, " user=", 6);
        Tcl_AppendObjToObj(objPtr, argsPtr->userObjPtr);
    }
    if (argsPtr->pwObjPtr != NULL) {
        Tcl_AppendToObj(objPtr, " password=", 10);
        Tcl_AppendObjToObj(objPtr, argsPtr->pwObjPtr);
    }
    if (argsPtr->dbObjPtr != NULL) {
        Tcl_AppendToObj(objPtr, " dbname=", 8);
        Tcl_AppendObjToObj(objPtr, argsPtr->dbObjPtr);
    }
    if (argsPtr->portObjPtr != NULL) {
        Tcl_AppendToObj(objPtr, " port=", 6);
        Tcl_AppendObjToObj(objPtr, argsPtr->portObjPtr);
    }
    if (argsPtr->optionsObjPtr != NULL) {
        Tcl_AppendToObj(objPtr, " ", 1);
        Tcl_AppendObjToObj(objPtr, argsPtr->optionsObjPtr);
    }
    conn = PQconnectdb(Tcl_GetString(objPtr)); 
    Tcl_DecrRefCount(objPtr);
    if (PQstatus(conn) != CONNECTION_OK) {
	Tcl_AppendResult(interp, "can't connect to psql server on \"",
                (argsPtr->hostObjPtr != NULL) ?
                         Tcl_GetString(argsPtr->hostObjPtr) : "localhost", 
                         "\": ", PQerrorMessage(conn), (char *)NULL);
	return TCL_ERROR;
    }
    *connPtr = conn;
    return TCL_OK;
}

static int
ImportPsqlProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    BLT_TABLE_COLUMN *cols;
    ImportArgs args;
    PGconn *conn;
    PGresult *res;
    int result;
    long numCols;
    
    result = TCL_ERROR;
    res = NULL;
    conn = NULL;
    cols = NULL;
    memset(&args, 0, sizeof(args));
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
		&args, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (PsqlConnect(interp, &args, &conn) != TCL_OK) {
	goto error;
    }
    if (args.queryObjPtr == NULL) {
        result = TCL_OK;
	goto error;
    }
    res = PsqlQuery(interp, conn, args.queryObjPtr);
    if (res == NULL) {
	goto error;
    }
    numCols = PQnfields(res);
    if (numCols < 1) {
        Tcl_AppendResult(interp,
                "server returned invalid number of columns", (char *)NULL);
	goto error;
    }
    cols = Blt_AssertMalloc(numCols * sizeof(BLT_TABLE_COLUMN));
    result = blt_table_extend_columns(interp, table, numCols, cols);
    if (result == TCL_OK) {
        result = PsqlImportLabels(interp, table, res, numCols, cols);
    }
    if (result == TCL_OK) {
        result = PsqlImportRows(interp, table, res, numCols, cols);
    }
 error:
    PsqlDisconnect(conn);
    if (cols != NULL) {
	Blt_Free(cols);
    }
    if (res != NULL) {
	PQclear(res);
    }
    if (conn != NULL) {
        PQfinish(conn);
    }
    Blt_FreeSwitches(importSwitches, &args, 0);
    return result;
}

#ifdef EXPORT_PSQL
static int
ExportPsqlProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    return TCL_OK;
}
#endif

int 
blt_table_psql_init(Tcl_Interp *interp)
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
    if (Tcl_PkgProvide(interp, "blt_datatable_psql", BLT_VERSION) != TCL_OK) { 
	return TCL_ERROR;
    }
    return blt_table_register_format(interp,
        "psql",                         /* Name of format. */
	ImportPsqlProc,                 /* Import procedure. */
	NULL);                          /* Export procedure. */

}

int 
blt_table_psql_safe_init(Tcl_Interp *interp)
{
    return blt_table_psql_init(interp);
}

#endif /* HAVE_LIBPQ */
#endif /* NO_DATATABLE */


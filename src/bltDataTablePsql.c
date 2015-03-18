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
    const char *host;                /* If non-NULL, name of remote host
					 * of Postgres server.  Otherwise
					 * "localhost" is used. */
    const char *user;                /* If non-NULL, name of user
					 * account to access Postgres server.
					 * Otherwise the current username
					 * is used. */
    const char *pw;                  /* If non-NULL, is password to use
					 * to access Postgres server. */
    const char *db;                  /* If non-NULL, name of Postgres SQL
					 * database to access. */
    const char *query;               /* If non-NULL, query to make. */
    const char *options;             /* If non-NULL, query to make. */
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
    {BLT_SWITCH_STRING,    "-query",    "string", (char *)NULL,
	Blt_Offset(ImportArgs, query), 0, 0},
    {BLT_SWITCH_STRING,    "-options",    "string", (char *)NULL,
	Blt_Offset(ImportArgs, options), 0, 0},
    {BLT_SWITCH_END}
};

#ifdef EXPORT_PSQL
/*
 * ExportArgs --
 */
typedef struct {
    const char *host;                /* If non-NULL, name of remote host
					 * of Postgres server.  Otherwise
					 * "localhost" is used. */
    const char *user;                /* If non-NULL, name of user
					 * account to access Postgres server.
					 * Otherwise the current username
					 * is used. */
    const char *pw;                  /* If non-NULL, is password to use
					 * to access Postgres server. */
    const char *db;                  /* If non-NULL, name of Postgres SQL
					 * database to access. */
    const char *query;               /* If non-NULL, query to make. */
    const char *options;             /* If non-NULL, query to make. */
    int port;                           /* Port number to use. */

    const char *table;               /* Name of table. */
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
    {BLT_SWITCH_STRING, "-name",        "tableName", (char *)NULL,
	Blt_Offset(ExportArgs, table), 0, 0},
    {BLT_SWITCH_STRING, "-db",       "dbName", (char *)NULL,
	Blt_Offset(ExportArgs, db), 0, 0},
    {BLT_SWITCH_STRING, "-host",     "hostName", (char *)NULL,
	Blt_Offset(ExportArgs, host), 0, 0},
    {BLT_SWITCH_STRING, "-user",     "userName", (char *)NULL,
	Blt_Offset(ExportArgs, user), 0, 0},
    {BLT_SWITCH_STRING, "-password", "password", (char *)NULL,
	Blt_Offset(ExportArgs, pw), 0, 0},
    {BLT_SWITCH_INT_NNEG, "-port",   "number", (char *)NULL,
	Blt_Offset(ExportArgs, port), 0, 0},
    {BLT_SWITCH_CUSTOM, "-columns",   "columns" ,(char *)NULL,
	Blt_Offset(ExportArgs, ci),   0, 0, &columnIterSwitch},
    {BLT_SWITCH_CUSTOM, "-rows",      "rows", (char *)NULL,
	Blt_Offset(ExportArgs, ri),   0, 0, &rowIterSwitch},
    {BLT_SWITCH_END}
};
static BLT_TABLE_EXPORT_PROC ExportPsqlProc;
#endif

#define DEF_CLIENT_FLAGS (CLIENT_MULTI_STATEMENTS|CLIENT_MULTI_RESULTS)

static BLT_TABLE_IMPORT_PROC ImportPsqlProc;

static int
PsqlConnect(Tcl_Interp *interp, ImportArgs *argsPtr, PGconn **connPtr) 
{
    PGconn  *conn;
    Blt_DBuffer dbuffer;
    const char *string;
    
    dbuffer = Blt_DBuffer_Create();
    if (argsPtr->host != NULL) {
        Blt_DBuffer_Format(dbuffer, "host=%s ", argsPtr->host);
    } else {
        Blt_DBuffer_Format(dbuffer, "host=%s ", "localhost");
    }
    if (argsPtr->user != NULL) {
        Blt_DBuffer_Format(dbuffer, "user=%s ", argsPtr->user);
    }
    if (argsPtr->pw != NULL) {
        Blt_DBuffer_Format(dbuffer, "password=%s ", argsPtr->pw);
    }
    if (argsPtr->db != NULL) {
        Blt_DBuffer_Format(dbuffer, "dbname=%s ", argsPtr->db);
    }
    Blt_DBuffer_Format(dbuffer, "port=%d ", argsPtr->port);

    if (argsPtr->options != NULL) {
        Blt_DBuffer_Format(dbuffer, "%s ", argsPtr->options);
    }
    string = Blt_DBuffer_String(dbuffer);
    conn = PQconnectdb(string); 
    if (PQstatus(conn) != CONNECTION_OK) {
	Tcl_AppendResult(interp, "can't connect to psql server \"",
		string, "\": ", PQerrorMessage(conn), (char *)NULL);
    }
    Blt_DBuffer_Destroy(dbuffer);
    if (PQstatus(conn) != CONNECTION_OK) {
	return TCL_ERROR;
    }
    *connPtr = conn;
    return TCL_OK;
}

static void
PsqlDisconnect(PGconn *conn) 
{
    PQfinish(conn);
}

static int
PsqlQuery(Tcl_Interp *interp, PGconn *conn, const char *query,
          PGresult **resPtr) 
{
    PGresult *res;
    
    res = PQexec(conn, query);
#ifdef notdef
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    }
#endif
    if (res == NULL) {
        fprintf(stderr, "status=%d\n", PQresultStatus(res));
	Tcl_AppendResult(interp, "error in query \"", query, "\": ", 
			 PQerrorMessage(conn), (char *)NULL);
	PQclear(res);
	return TCL_ERROR;
    }
    *resPtr = res;
    return TCL_OK;
}

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
PsqlImportValues(Tcl_Interp *interp, BLT_TABLE table, PGresult *res, 
                 size_t numCols, BLT_TABLE_COLUMN *cols) 
{
    size_t numRows;
    size_t i;
    BLT_TABLE_ROW *rows;
    
    numRows = PQntuples(res);
    rows = Blt_Malloc(sizeof(BLT_TABLE_ROW) * numRows);
    if (rows == NULL) {
        return TCL_ERROR;
    }
    if (blt_table_extend_rows(interp, table, numRows, rows) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 0; i < numRows; i++) {
	size_t j;
	for (j = 0; j < numCols; j++) {
	    int length;
	    const char *value;

	    value = PQgetvalue(res, i, j);
	    length = PQgetlength(res, i, j);
	    if (blt_table_set_string_rep(table, rows[i], cols[j], value,
                        length) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    return TCL_OK;
}

#ifdef notdef
static int
PsqlImportColumnTypes(Tcl_Interp *interp, BLT_TABLE table, size_t numCols,
                      BLT_TABLE_COLUMN *cols) 
{
    size_t numRows;
    size_t i;
    BLT_TABLE_ROW *rows;
    const char *query;
    

    Blt_DBuffer_Format(dbuffer, "select column_name,data_type from "
                       "INFORMATION_SCHEMA.COLUMNS where table_name = '%s' "
                       "AND column_name in (", argsPtr->tableName);
    for (i = 0; i < numCols; i++) {
        const char *label;
        
        label = blt_table_column_label(cols[i]);
        Blt_DBuffer_Format(dbuffer, "'%s'%s", label, (i > 0) ? ", " : " ");
    }            
    Blt_DBuffer_Format(dbuffer, ");");
    query = Blt_DBuffer_String(dbuffer);
    result = PsqlQuery(interp, conn, query, &res);
    if (result == TCL_ERROR) {
        
    }
    Blt_DBuffer_Destory(dbuffer);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    numRows = PQntuples(res);
    assert (numRows == numCols);
    for (i = 0; i < numCols; i++) {
        const char *name;
        int length;
        
        /* Column label */
        label = PQgetvalue(res, i, 0);
        length = PQgetlength(res, i, 0);
        col = blt_table_get_column_from_label(table, label);
        if (col == NULL) {
            return TCL_ERROR;
        }
        /* Column type */
        string = PQgetvalue(res, i, 1);
        length = PQgetlength(res, i, 1);
        type = PsqlConvertoColumnType(string, length);
        if (blt_table_set_column_type(table, col, type) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    PQclear(res);
    return TCL_OK;
}
#endif

static int
ImportPsqlProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    ImportArgs args;
    PGconn *conn;
    PGresult *res;
    int result;
    
    res = NULL;
    memset(&args, 0, sizeof(args));
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
		&args, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (args.query == NULL) {
        Tcl_AppendResult(interp, "-query switch is required.", (char *)NULL);
	return TCL_ERROR;
    }
    conn = NULL;
    result = PsqlConnect(interp, &args, &conn);
    res = NULL;
    if (result == TCL_OK) {
        result = PsqlQuery(interp, conn, args.query, &res);
    }
    if (result == TCL_OK) {
        long numCols;
        BLT_TABLE_COLUMN *cols;

        numCols = PQnfields(res);
        cols = Blt_AssertMalloc(numCols * sizeof(BLT_TABLE_COLUMN));
        result = blt_table_extend_columns(interp, table, numCols, cols);
        if (result == TCL_OK) {
            result = PsqlImportLabels(interp, table, res, numCols, cols);
        }
        if (result == TCL_OK) {
            result = PsqlImportValues(interp, table, res, numCols, cols);
        }
	Blt_Free(cols);
    }
    if (res != NULL) {
	PQclear(res);
    }
    if (conn != NULL) {
        PsqlDisconnect(conn);
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


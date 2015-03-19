/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltDataTablePsql.c --
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
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */
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
 * $table import psql -host $host -password $pw -db $db -port $port 
 */

typedef struct {
    const char *host;                   /* If non-NULL, name of remote host
					 * of Postgres server.  Otherwise
					 * "localhost" is used. */
    const char *user;                   /* If non-NULL, name of user
					 * account to access Postgres server.
					 * Otherwise the current username
					 * is used. */
    const char *pw;                     /* If non-NULL, is password to use
					 * to access Postgres server. */
    const char *db;                     /* If non-NULL, name of Postgres
					 * SQL database to access. */
    const char *query;                  /* If non-NULL, query to make. */
    const char *options;                /* If non-NULL, query to make. */
    int port;                           /* Port number to use. */
} PsqlConnection;

/*
 * ImportSwitches --
 */
typedef struct {
    PsqlConnection params;
    const char *query;                  /* Query to make. */
    const char *table;                  /* Name of table. */
    
    /* Private data. */
    Tcl_Interp *interp;
    unsigned int flags;
    char *buffer;                       /* Buffer to read data into. */
    int numBytes;			/* # of bytes in the buffer. */
    const char *tableName;
} ImportArgs;

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_STRING, "-db",        "dbName", (char *)NULL,
	Blt_Offset(ImportArgs, params.db), 0, 0},
    {BLT_SWITCH_STRING, "-host",      "hostName", (char *)NULL,
	Blt_Offset(ImportArgs, params.host), 0, 0},
    {BLT_SWITCH_STRING, "-user",      "userName", (char *)NULL,
	Blt_Offset(ImportArgs, params.user), 0, 0},
    {BLT_SWITCH_STRING, "-password",  "password", (char *)NULL,
	Blt_Offset(ImportArgs, params.pw), 0, 0},
    {BLT_SWITCH_INT_NNEG, "-port",    "number", (char *)NULL,
	Blt_Offset(ImportArgs, params.port), 0, 0},
    {BLT_SWITCH_STRING,    "-options", "string", (char *)NULL,
	Blt_Offset(ImportArgs, params.options), 0, 0},
    {BLT_SWITCH_STRING,    "-query",  "string", (char *)NULL,
	Blt_Offset(ImportArgs, query), 0, 0},
    {BLT_SWITCH_STRING, "-name",        "tableName", (char *)NULL,
	Blt_Offset(ImportArgs, table), 0, 0},
    {BLT_SWITCH_END}
};

/*
 * ExportArgs --
 */
typedef struct {
    PsqlConnection params;
    const char *query;                  /* If non-NULL, query to make. */
    const char *table;                  /* Name of table. */
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
    {BLT_SWITCH_STRING, "-db",       "dbName", (char *)NULL,
	Blt_Offset(ExportArgs, params.db), 0, 0},
    {BLT_SWITCH_STRING, "-host",     "hostName", (char *)NULL,
	Blt_Offset(ExportArgs, params.host), 0, 0},
    {BLT_SWITCH_STRING, "-user",     "userName", (char *)NULL,
	Blt_Offset(ExportArgs, params.user), 0, 0},
    {BLT_SWITCH_STRING, "-password", "password", (char *)NULL,
	Blt_Offset(ExportArgs, params.pw), 0, 0},
    {BLT_SWITCH_INT_NNEG, "-port",   "number", (char *)NULL,
	Blt_Offset(ExportArgs, params.port), 0, 0},
    {BLT_SWITCH_CUSTOM, "-columns",   "columns" ,(char *)NULL,
	Blt_Offset(ExportArgs, ci),   0, 0, &columnIterSwitch},
    {BLT_SWITCH_CUSTOM, "-rows",      "rows", (char *)NULL,
	Blt_Offset(ExportArgs, ri),   0, 0, &rowIterSwitch},
    {BLT_SWITCH_STRING, "-name",        "tableName", (char *)NULL,
	Blt_Offset(ExportArgs, table), 0, 0},
    {BLT_SWITCH_END}
};

static BLT_TABLE_EXPORT_PROC ExportPsqlProc;
static BLT_TABLE_IMPORT_PROC ImportPsqlProc;

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

/*
 *---------------------------------------------------------------------------
 *
 * PsqlConvertToColumnType --
 *
 *	Convert the postgres type (represented by a string) in the 
 *      the associated table column type.
 *
 * Results:
 *	The return value is the converted column type.
 *
 *---------------------------------------------------------------------------
 */
static BLT_TABLE_COLUMN_TYPE
PsqlConvertToColumnType(const char *string, size_t length)
{
    fprintf(stderr, "Postgres column type is \"%s\"\n", string);
    return TABLE_COLUMN_TYPE_STRING;
}

/*
 *---------------------------------------------------------------------------
 *
 * PsqlConnect --
 *
 *	Connects to the postgres server by first constructing the 
 *      parameters string for user, password, host,  post, etc.
 *
 * Results:
 *	The return value is a standard TCL result.  The connection
 *      is returned via *connPtr*.
 *
 *---------------------------------------------------------------------------
 */
static int
PsqlConnect(Tcl_Interp *interp, PsqlConnection *paramsPtr, PGconn **connPtr) 
{
    PGconn  *conn;
    Blt_DBuffer dbuffer;
    const char *string;
    
    dbuffer = Blt_DBuffer_Create();
    if (paramsPtr->host != NULL) {
        Blt_DBuffer_Format(dbuffer, "host=%s ", paramsPtr->host);
    } else {
        Blt_DBuffer_Format(dbuffer, "host=%s ", "localhost");
    }
    if (paramsPtr->user != NULL) {
        Blt_DBuffer_Format(dbuffer, "user=%s ", paramsPtr->user);
    } else {
        Blt_DBuffer_Format(dbuffer, "user=%s ", getenv("USER"));
    }
    if (paramsPtr->pw != NULL) {
        Blt_DBuffer_Format(dbuffer, "password=%s ", paramsPtr->pw);
    }
    if (paramsPtr->db != NULL) {
        Blt_DBuffer_Format(dbuffer, "dbname=%s ", paramsPtr->db);
    }
    Blt_DBuffer_Format(dbuffer, "port=%d ", paramsPtr->port);

    if (paramsPtr->options != NULL) {
        Blt_DBuffer_Format(dbuffer, "%s ", paramsPtr->options);
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

/*
 *---------------------------------------------------------------------------
 *
 * PsqlQuery --
 *
 *	Executes the postgres command/query.
 *
 * Results:
 *	The return value is a standard TCL result.  The result set
 *      is returned via *resultPtr*.
 *
 *---------------------------------------------------------------------------
 */
static int
PsqlQuery(Tcl_Interp *interp, PGconn *conn, const char *query,
          PGresult **resultPtr) 
{
    PGresult *result;
    
    result = PQexec(conn, query);
    if (result == NULL) {
	Tcl_AppendResult(interp, "error in query \"", query, "\": ", 
			 PQerrorMessage(conn), (char *)NULL);
	PQclear(result);
	return TCL_ERROR;
    }
    *resultPtr = result;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PsqlImportLabels --
 *
 *	Sets the column labels in the table for the new columns. The column
 *      names are the postgres field names.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
static int
PsqlImportLabels(Tcl_Interp *interp, BLT_TABLE table, PGresult *result, 
		 size_t numCols, BLT_TABLE_COLUMN *cols) 
{
    size_t i;

    for (i = 0; i < numCols; i++) {
	const char *label;

	label = PQfname(result, i);
	if (blt_table_set_column_label(interp, table, cols[i], label) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PsqlImportValues --
 *
 *	Sets the cell values in the table for the new columns.  
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
static int
PsqlImportValues(Tcl_Interp *interp, BLT_TABLE table, PGresult *result, 
                 size_t numCols, BLT_TABLE_COLUMN *cols) 
{
    size_t numRows;
    size_t i;
    
    numRows = PQntuples(result);
    /* First check that there are enough rows in the table to accomodate
     * the new data. Add more if necessary. */
    if (numRows  > blt_table_num_rows(table)) {
        size_t needed;

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

	    value = PQgetvalue(result, i, j);
	    length = PQgetlength(result, i, j);
	    if (blt_table_set_string_rep(table, row, cols[j], value, length)
                != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PsqlImportColumnTypes --
 *
 *      Set the table column types from the postgres scheme information.
 *	Postgres does not report the type with the field.  You can however
 *	query to column types for a particular table from the server.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
static int
PsqlImportColumnTypes(Tcl_Interp *interp, BLT_TABLE table, PGconn *conn,
                      size_t numCols, BLT_TABLE_COLUMN *cols,
                      const char *tableName) 
{
    Blt_DBuffer dbuffer;
    Blt_HashTable nameTable;
    PGresult *result;
    const char *query;
    size_t i;
    size_t numRows;

    /* Ask postgres for the data types of the named table. */
    dbuffer = Blt_DBuffer_Create();
    Blt_DBuffer_Format(dbuffer, "select column_name,data_type from "
        "INFORMATION_SCHEMA.COLUMNS where table_name = '%s';", tableName);
    query = Blt_DBuffer_String(dbuffer);
    if (PsqlQuery(interp, conn, query, &result) != TCL_OK) {
        Blt_DBuffer_Destroy(dbuffer);
        return TCL_ERROR;
    }
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
	Tcl_AppendResult(interp, "error in query \"", query, "\": ", 
                PQresultErrorMessage(result), (char *)NULL);
        Blt_DBuffer_Destroy(dbuffer);
        PQclear(result);
        return TCL_ERROR;
    }
    Blt_DBuffer_Destroy(dbuffer);

    numRows = PQntuples(result);
    assert (numRows > numCols);         /* There has to be at least as many
                                         * rows returned as there are
                                         * columns in the table.  */

    /* Create a hash table of the new column labels. We'll use it to search
     * for column_name entries that we get back from postgres. [Note: We
     * could use the table itself to look up column names, but there could
     * be duplicate column names and we don't want to reset the type of an
     * existing column.] */
    Blt_InitHashTable(&nameTable, BLT_STRING_KEYS);
    for (i = 0; i < numCols; i++) {
        Blt_HashEntry *hPtr;
        const char *label;
        int isNew;
        
        label = blt_table_column_label(cols[i]);
        hPtr = Blt_CreateHashEntry(&nameTable, label, &isNew);
        assert(isNew);
        Blt_SetHashValue(hPtr, cols[i]);
    }            
    for (i = 0; i < numCols; i++) {
        BLT_TABLE_COLUMN col;
        BLT_TABLE_COLUMN_TYPE type;
        Blt_HashEntry *hPtr;
        const char *label, *string;
        int length;
        
        /* Column label */
        label = PQgetvalue(result, i, 0);
        hPtr = Blt_FindHashEntry(&nameTable, label);
        if (hPtr == NULL) {
            continue;                   /* Not a column we know about. */
        }
        /* Column type */
        col = Blt_GetHashValue(hPtr);
        string = PQgetvalue(result, i, 1);
        length = PQgetlength(result, i, 1);
        type = PsqlConvertToColumnType(string, length);
        if (blt_table_set_column_type(table, col, type) != TCL_OK) {
            Blt_DeleteHashTable(&nameTable);
            return TCL_ERROR;           /* Failed to convert column values
                                         * to the requested type.*/
        }
    }
    Blt_DeleteHashTable(&nameTable);
    PQclear(result);
    return TCL_OK;
}

static int
PsqlCreateTable(Tcl_Interp *interp, PGconn *conn, BLT_TABLE table,
                ExportArgs *argsPtr)
{
    BLT_TABLE_COLUMN col;
    Blt_DBuffer dbuffer;
    PGresult *result;
    const char *query;
    int first;
    
    dbuffer = Blt_DBuffer_Create();
    Blt_DBuffer_Format(dbuffer, "DROP TABLE IF EXISTS %s; CREATE TABLE %s (",
                       argsPtr->tableName, argsPtr->tableName);
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
        case TABLE_COLUMN_TYPE_TIME:
        case TABLE_COLUMN_TYPE_STRING:
            Blt_DBuffer_Format(dbuffer, "TEXT");        break;
        }
        first = FALSE;
    }
    Blt_DBuffer_Format(dbuffer, ");"); 
    query = (const char *)Blt_DBuffer_String(dbuffer);
    if (PsqlQuery(interp, conn, query, &result) != TCL_OK) {
        Tcl_AppendResult(interp, "error in table create \"", query, "\": ", 
                PQerrorMessage(conn), (char *)NULL);
        Blt_DBuffer_Destroy(dbuffer);
        return TCL_ERROR;
    }
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
	Tcl_AppendResult(interp, "error in query \"", query, "\": ", 
                PQresultErrorMessage(result), (char *)NULL);
        Blt_DBuffer_Destroy(dbuffer);
        PQclear(result);
        return TCL_ERROR;
    }
    Blt_DBuffer_Destroy(dbuffer);
    PQclear(result);
    return TCL_OK;
}

static int
PsqlExportValues(Tcl_Interp *interp, PGconn *conn, BLT_TABLE table,
                 ExportArgs *argsPtr)
{
    BLT_TABLE_COLUMN col;
    BLT_TABLE_ROW row;
    Blt_DBuffer dbuffer, dbuffer2;
    const char *query;
    int count, numParams; 
    int *formats;
    const char **values;
    int *lengths;
    PGresult *result;
    const char *stmtName;

    stmtName = "TableInsert";
    /* Build up the insert string including the parameters */
    dbuffer = Blt_DBuffer_Create();
    dbuffer2 = Blt_DBuffer_Create();
    Blt_DBuffer_Format(dbuffer, "INSERT INTO '%s' (", argsPtr->tableName);
    Blt_DBuffer_Format(dbuffer2, " VALUES (");
    count = 0;
    if (argsPtr->flags & EXPORT_ROWLABELS) {
        Blt_DBuffer_Format(dbuffer, "_rowId TEXT ");
        Blt_DBuffer_Format(dbuffer2, "?%d", count);
        count++;
    }        
    for (col = blt_table_first_tagged_column(&argsPtr->ci); col != NULL;
         col = blt_table_next_tagged_column(&argsPtr->ci)) {
        const char *label;
        
        label = blt_table_column_label(col);
        if (count > 1) {
            Blt_DBuffer_Format(dbuffer, ", ");
            Blt_DBuffer_Format(dbuffer2, ", ");
        }
        Blt_DBuffer_Format(dbuffer, "[%s]", label);
        Blt_DBuffer_Format(dbuffer2, "?%d", count);
        count++;
    }
    Blt_DBuffer_Format(dbuffer2, ");");
    Blt_DBuffer_Format(dbuffer, ")");
    Blt_DBuffer_Concat(dbuffer, dbuffer2);
    Blt_DBuffer_Destroy(dbuffer2);

    numParams = count;
    /* Arrays for parameters. */
    formats = Blt_AssertCalloc(numParams, sizeof(int)); /* Formats are
                                                         * always text. */
    values = Blt_AssertMalloc(sizeof(char *) * numParams);
    lengths = Blt_AssertMalloc(sizeof(int) * numParams);

    query = Blt_DBuffer_String(dbuffer);
    result = PQprepare(conn, stmtName, query, numParams, NULL);
    if (result == NULL) {
        Tcl_AppendResult(interp, "error in parameters \": ", 
                PQerrorMessage(conn), (char *)NULL);
        Blt_DBuffer_Destroy(dbuffer);
        goto error;
    }
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        Tcl_AppendResult(interp, "error in insert statment \"", query, "\": ", 
                PQresultErrorMessage(result), (char *)NULL);
        goto error;
    }
    PQclear(result);

    Blt_DBuffer_Destroy(dbuffer);
    for (row = blt_table_first_tagged_row(&argsPtr->ri); row != NULL; 
	 row = blt_table_next_tagged_row(&argsPtr->ri)) {
        int count;                      
        
        count = 0;                      
        if (argsPtr->flags & EXPORT_ROWLABELS) {
            const char *label;
                    
            label = blt_table_row_label(row);
            values[count] = (char *)label;
            lengths[count] = strlen(label);
            count++;
        }
        for (col = blt_table_first_tagged_column(&argsPtr->ci); col != NULL;
             col = blt_table_next_tagged_column(&argsPtr->ci)) {
            struct _BLT_TABLE_VALUE *valuePtr;

            valuePtr = blt_table_get_value(table, row, col);
            if (valuePtr == NULL) {
                values[count] = NULL;
                lengths[count] = 0;
            } else {
                values[count] = (char *)valuePtr->string;
                lengths[count] = valuePtr->length;
            }
            count++;
        }
        result = PQexecPrepared(conn, stmtName, numParams, values, lengths,
                formats, 0);
        if (result == NULL) {
            Tcl_AppendResult(interp, "error in parameters \": ", 
                PQerrorMessage(conn), (char *)NULL);
            goto error;
        }
        if (PQresultStatus(result) != PGRES_COMMAND_OK) {
            Tcl_AppendResult(interp, "error in parameters \": ", 
                PQresultErrorMessage(result), (char *)NULL);
            goto error;
        }
        PQclear(result);
    }
    Blt_Free(formats);
    Blt_Free(values);
    Blt_Free(lengths);
    return TCL_OK;
 error:
    if (formats != NULL) {
        Blt_Free(formats);
     }
    if (values != NULL) {
        Blt_Free(values);
    }
    if (lengths != NULL) {
        Blt_Free(lengths);
    }
    return TCL_ERROR;
}


static int
ImportPsqlProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    ImportArgs args;
    PGconn *conn;
    PGresult *resultset;
    int result;
    
    memset(&args, 0, sizeof(args));
    args.params.port = 5432;
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
		&args, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (args.query == NULL) {
        Tcl_AppendResult(interp, "-query switch is required.", (char *)NULL);
	return TCL_ERROR;
    }
    conn = NULL;
    resultset = NULL;
    result = PsqlConnect(interp, &args.params, &conn);
    if (result == TCL_OK) {
        result = PsqlQuery(interp, conn, args.query, &resultset);
        if ((result == TCL_OK) &&
            (PQresultStatus(resultset) != PGRES_TUPLES_OK)) {
            Tcl_AppendResult(interp, "error in query \"", args.query,
                "\": ", PQresultErrorMessage(resultset), (char *)NULL);
            PQclear(resultset);
            result = TCL_ERROR;
        }
    }
    if (result == TCL_OK) {
        long numCols;
        BLT_TABLE_COLUMN *cols;

        numCols = PQnfields(resultset);
        cols = Blt_AssertMalloc(numCols * sizeof(BLT_TABLE_COLUMN));
        result = blt_table_extend_columns(interp, table, numCols, cols);
        if (result == TCL_OK) {
            result = PsqlImportLabels(interp, table, resultset, numCols, cols);
        }
        if (result == TCL_OK) {
            result = PsqlImportValues(interp, table, resultset, numCols, cols);
        }
        if ((result == TCL_OK) && (args.table != NULL)) {
            result = PsqlImportColumnTypes(interp, table, conn, numCols, cols,
                args.table);
        }
	Blt_Free(cols);
    }
    if (resultset != NULL) {
	PQclear(resultset);
    }
    if (conn != NULL) {
        PQfinish(conn);
    }
    Blt_FreeSwitches(importSwitches, &args, 0);
    return result;
}

static int
ExportPsqlProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    ExportArgs args;
    PGconn *conn;
    int result;
    
    if ((blt_table_num_rows(table) == 0) ||
        (blt_table_num_columns(table) == 0)) {
        return TCL_OK;                  /* Empty table. */
    }
    memset(&args, 0, sizeof(args));
    args.params.port = 5432;
    rowIterSwitch.clientData = table;
    columnIterSwitch.clientData = table;
    blt_table_iterate_all_rows(table, &args.ri);
    blt_table_iterate_all_columns(table, &args.ci);
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, 
		&args, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (args.table != NULL) {
        args.tableName = args.table;
    } else {
        args.tableName = "bltDataTable";
    }
    conn = NULL;                        /* Suppress compiler warning. */
    result = PsqlConnect(interp, &args.params, &conn);
    if (result == TCL_OK) {
        result = PsqlCreateTable(interp, conn, table, &args);
    }
    if (result == TCL_OK) {
        result = PsqlExportValues(interp, conn, table, &args);
    }
    if (conn != NULL) {
        PQfinish(conn);
    }
    Blt_FreeSwitches(exportSwitches, &args, 0);
    return result;
}

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
	ExportPsqlProc);                /* Export procedure. */

}

int 
blt_table_psql_safe_init(Tcl_Interp *interp)
{
    return blt_table_psql_init(interp);
}

#endif /* HAVE_LIBPQ */
#endif /* NO_DATATABLE */


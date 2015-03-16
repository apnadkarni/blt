/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *
 * bltDataTableVec.c --
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

#include "config.h"
#include <assert.h>
#include <tcl.h>

DLLEXPORT extern Tcl_AppInitProc blt_table_vector_init;
DLLEXPORT extern Tcl_AppInitProc blt_table_vector_safe_init;

/*
 * Format	Import		Export
 * csv		file/data	file/data
 * tree		data		data
 * vector	data		data
 * xml		file/data	file/data
 * sql		data		data
 *
 * $table import vector $vecName label $vecName label...
 * $table export vector label $vecName label $vecName...
 */

static BLT_TABLE_IMPORT_PROC ImportVectorProc;
static BLT_TABLE_EXPORT_PROC ExportVectorProc;

/* 
 * $table export vector col vecName col vecName...
 */
static int
ExportVectorProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    int i;
    long numRows;
    
    if ((objc - 3) & 1) {
	Tcl_AppendResult(interp, "odd # of column/vector pairs: should be \"", 
		Tcl_GetString(objv[0]), 
		" export vector col vecName ?col vecName?...", (char *)NULL);
	return TCL_ERROR;
    }
    numRows = blt_table_num_rows(table);
    for (i = 3; i < objc; i += 2) {
	Blt_Vector *vector;
	size_t size;
	double *array;
	int k;
	BLT_TABLE_COLUMN col;

	col = blt_table_get_column(interp, table, objv[i]);
	if (col == NULL) {
	    return TCL_ERROR;
	}
	if (Blt_GetVectorFromObj(interp, objv[i+1], &vector) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Blt_VecLength(vector) != numRows) {
	    if (Blt_ResizeVector(vector, numRows) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	array = Blt_VecData(vector);
	size = Blt_VecSize(vector);
	for (k = 0; k < Blt_VecLength(vector); k++) {
	    BLT_TABLE_ROW row;

	    row = blt_table_row(table, k);
	    array[k] = blt_table_get_double(table, row, col);
	}
	if (Blt_ResetVector(vector, array, numRows, size, TCL_STATIC) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImportVectorProc --
 *
 *	Imports the given vector into the named column in the table.
 *      If the column doesn't already exist, it is created.  The values
 *      from the vector will overwrite any existing data in the column.
 *      Any leftover values in the column will be unset.
 *	
 * Results:
 *	Returns a standard TCL result.  It is the result of import
 *	operation.
 *
 *
 *      $table import vector col vecName col vecName...
 *
 *---------------------------------------------------------------------------
 */
static int
ImportVectorProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    int i;

    if ((objc - 3) & 1) {
	Tcl_AppendResult(interp, "odd # of vector/column pairs: should be \"", 
		Tcl_GetString(objv[0]), 
		" import vector vecName col vecName col...", (char *)NULL);
	return TCL_ERROR;
    }
    for (i = 3; i < objc; i += 2) {
	BLT_TABLE_COLUMN col;
	Blt_Vector *vector;
	double *array;
	long j;
	size_t numElems, numRows;

	if (Blt_GetVectorFromObj(interp, objv[i+1], &vector) != TCL_OK) {
	    return TCL_ERROR;
	}
	numElems = Blt_VecLength(vector);
	numRows = blt_table_num_rows(table);
	col = blt_table_get_column(NULL, table, objv[i+1]);
	if (col == NULL) {
	    const char *name;

	    /* Create column if it doesn't already exist */
	    name = Tcl_GetString(objv[i+1]);
	    col = blt_table_create_column(interp, table, name);
	    if (col == NULL) {
		return TCL_ERROR;
	    }
	}
	if (numElems > blt_table_num_rows(table)) {
	    size_t extra;

	    extra = numElems - numRows;
	    if (blt_table_extend_rows(interp, table, extra, NULL) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	array = Blt_VecData(vector);
	/* Write the vector values into the table (possibly overwriting
	 * existing cell values).  */
	for (j = 0; j < numElems; j++) {
	    BLT_TABLE_ROW row;

	    row = blt_table_row(table, j);
	    if (blt_table_set_double(table, row, col, array[j]) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	/* Unset any remaining cells. */
	for (j = numElems; j < numRows; j++) {
	    BLT_TABLE_ROW row;

	    row = blt_table_row(table, j);
	    if (blt_table_unset_value(table, row, col) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	blt_table_set_column_type(table, col, TABLE_COLUMN_TYPE_DOUBLE);
    }
    return TCL_OK;
}
    
int 
blt_table_vector_init(Tcl_Interp *interp)
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
    if (Tcl_PkgProvide(interp, "blt_datatable_vector", BLT_VERSION) != TCL_OK){ 
	return TCL_ERROR;
    }
    return blt_table_register_format(interp,
	"vector",			/* Name of format. */
	ImportVectorProc,		/* Import procedure. */
	ExportVectorProc);		/* Export procedure. */
}

int 
blt_table_vector_safe_init(Tcl_Interp *interp)
{
    return blt_table_vector_init(interp);
}

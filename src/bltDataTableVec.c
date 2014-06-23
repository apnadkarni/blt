/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 *
 * bltDataTableVec.c --
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
 * $table export vector -file fileName ?switches...?
 * $table export vector ?switches...?
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
	if (Blt_ResetVector(vector, array, numRows, size, TCL_STATIC) != TCL_OK) {
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
 *	Parses the given command line and calls one of several
 *	export-specific operations.
 *	
 * Results:
 *	Returns a standard TCL result.  It is the result of 
 *	operation called.
 *
 *	$table import vector v1 col v2 col v3 col
 *
 *---------------------------------------------------------------------------
 */
static int
ImportVectorProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    int i;
    size_t oldLength;

    if ((objc - 3) & 1) {
	Tcl_AppendResult(interp, "odd # of vector/column pairs: should be \"", 
		Tcl_GetString(objv[0]), 
		" import vector vecName col vecName col...", (char *)NULL);
	return TCL_ERROR;
    }
    oldLength = blt_table_num_rows(table);
    for (i = 3; i < objc; i += 2) {
	BLT_TABLE_COLUMN col;
	Blt_Vector *vector;
	double *array;
	size_t j, k;
	size_t numElems;
	size_t size, start;

	if (Blt_GetVectorFromObj(interp, objv[i], &vector) != TCL_OK) {
	    return TCL_ERROR;
	}
	size = Blt_VecLength(vector);
	col = blt_table_get_column(NULL, table, objv[i+1]);
	if (col == NULL) {
	    col = blt_table_create_column(interp, table, 
                Tcl_GetString(objv[i+1]));
	    if (col == NULL) {
		return TCL_ERROR;
	    }
	    blt_table_set_column_type(table, col, TABLE_COLUMN_TYPE_DOUBLE);
	    start = 0;
	} else {
	    size += oldLength;
	    start = oldLength;
	}
	if (size > blt_table_num_rows(table)) {
	    size_t needed;

	    needed = size - oldLength;
	    if (blt_table_extend_rows(interp, table, needed, NULL) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	array = Blt_VecData(vector);
	numElems = Blt_VecLength(vector);
	for (j = 0, k = start; j < numElems; j++, k++) {
	    BLT_TABLE_ROW row;

	    row = blt_table_row(table, k);
	    if (blt_table_set_double(table, row, col, array[j]) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
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

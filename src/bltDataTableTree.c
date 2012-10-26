
/*
 *
 * bltDtTree.c --
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

#ifndef NO_DATATABLE
#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif /* HAVE_MEMORY_H */

#include <tcl.h>

/*
 * Format	Import		Export
 * csv		file/data	file/data
 * tree		data		data
 * vector	data		data
 * xml		file/data	file/data
 * sql		data		data
 *
 * $table import csv -file fileName -data dataString 
 * $table export csv -file defaultFileName 
 * $table import tree $treeName $node ?switches? 
 * $table export tree $treeName $node "label" "label" "label"
 * $table import vector $vecName label $vecName label...
 * $table export vector label $vecName label $vecName...
 * $table import xml -file fileName -data dataString ?switches?
 * $table export xml -file fileName -data dataString ?switches?
 * $table import sql -host $host -password $pw -db $db -port $port 
 */

/*
 * ImportSwitches --
 */
typedef struct {
    /* Private data. */
    Blt_TreeNode node;

    /* Public fields */
    size_t maxDepth;
    unsigned int flags;
} ImportSwitches;

#define IMPORT_INODES	(1<<0)

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_INT_NNEG, "-depth", "number", (char *)NULL,
	Blt_Offset(ImportSwitches, maxDepth), 0},
    {BLT_SWITCH_BITMASK, "-inodes",  "", (char *)NULL,
        Blt_Offset(ImportSwitches, flags), 0, IMPORT_INODES},
    {BLT_SWITCH_END}
};

/*
 * ExportSwitches --
 */
typedef struct {
    /* Private data. */
    Blt_TreeNode node;

    /* Public fields */
    BLT_TABLE_ITERATOR rIter, cIter;
    BLT_TABLE_ITERATOR hIter;
    Tcl_Obj *nodeObjPtr;
} ExportSwitches;

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
    {BLT_SWITCH_CUSTOM, "-columns", "columns", (char *)NULL,
	Blt_Offset(ExportSwitches, cIter), 0, 0, &columnIterSwitch},
    {BLT_SWITCH_CUSTOM, "-hierarchy", "columns", (char *)NULL,
	Blt_Offset(ExportSwitches, hIter), 0, 0, &columnIterSwitch},
    {BLT_SWITCH_OBJ, "-node", "node", (char *)NULL,
	Blt_Offset(ExportSwitches, nodeObjPtr), 0},
    {BLT_SWITCH_CUSTOM, "-rows", "rows", (char *)NULL,
        Blt_Offset(ExportSwitches, rIter), 0, 0, &rowIterSwitch},
    {BLT_SWITCH_END}
};


DLLEXPORT extern Tcl_AppInitProc blt_table_tree_init;
DLLEXPORT extern Tcl_AppInitProc blt_table_tree_safe_init;

static BLT_TABLE_IMPORT_PROC ImportTreeProc;
static BLT_TABLE_EXPORT_PROC ExportTreeProc;
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
ColumnIterFreeProc(ClientData clientData, char *record, 
				int offset, int flags)
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
ColumnIterSwitchProc(
    ClientData clientData,		/* Flag indicating if the node is
					 * considered before or after the
					 * insertion position. */
    Tcl_Interp *interp,			/* Interpreter to report results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
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
RowIterFreeProc(ClientData clientData, char *record, int offset, 
			     int flags)
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
RowIterSwitchProc(
    ClientData clientData,		/* Flag indicating if the node is
					 * considered before or after the
					 * insertion position. */
    Tcl_Interp *interp,			/* Interpreter to report results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
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

static int
ImportTree(Tcl_Interp *interp, BLT_TABLE table, Blt_Tree tree, 
	   Blt_TreeNode top, ImportSwitches *switchesPtr)
{
    Blt_TreeNode node;
    int maxDepth, topDepth;
    long rowIndex;

    /* 
     * Fill in the table data in 2 passes.  We need to know the
     * maximum depth of the leaf nodes, to generate columns for each
     * level of the hierarchy.  We have to do this before adding
     * node data values.
     */

    /* Pass 1.  Create row entries for all the nodes. Add entries for 
     *          the node and it's ancestor's labels. */
    maxDepth = topDepth = Blt_Tree_NodeDepth(top);
    for (node = Blt_Tree_NextNode(top, top); node != NULL;
	 node = Blt_Tree_NextNode(top, node)) {
	Blt_TreeNode parent;
	int depth;
	BLT_TABLE_ROW row;
	size_t colIndex;

	depth = Blt_Tree_NodeDepth(node);
	if ((switchesPtr->maxDepth > 0) && 
	    (depth > (topDepth + switchesPtr->maxDepth))) {
	    /* Skipping node because is it beyond the maximum depth desired. */
	    continue;
	}
	if (depth > maxDepth) {
	    BLT_TABLE_COLUMN col;

	    if (blt_table_extend_columns(interp, table, 1, &col) != TCL_OK) {
		return TCL_ERROR;
	    }
	    colIndex = blt_table_column_index(col);
	    maxDepth = depth;
	} else {
	    colIndex = depth - topDepth - 1;
	}
	if (blt_table_extend_rows(interp, table, 1, &row) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (parent = node; parent != top; 
	     parent = Blt_Tree_ParentNode(parent)){
	    const char *label;
	    BLT_TABLE_COLUMN col;

	    col = blt_table_get_column(table, colIndex);
	    label = Blt_Tree_NodeLabel(parent);
	    if (blt_table_set_string(table, row, col, label, -1) !=TCL_OK) {
		return TCL_ERROR;
	    }
	    if (switchesPtr->flags & IMPORT_INODES) {
		Tcl_Obj *objPtr;
		const char *label;

		label = "inode";
		col = blt_table_column_find_by_label(table, label);
		if (col == NULL) {
		    col = blt_table_column_create(interp, table, label);
		    if (col == NULL) {
			return TCL_ERROR;
		    }
		}
		objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(node));
		if (blt_table_set_obj(table, row, col, objPtr) != TCL_OK) {
		    return TCL_ERROR;
		}
	    }
	    colIndex--;
	}
    }
    /* Pass 2.  Fill in entries for all the data fields found. */
    for (rowIndex = 0, node = Blt_Tree_NextNode(top, top); node != NULL;
	 node = Blt_Tree_NextNode(top, node)) {
	Blt_TreeKey key;
	Blt_TreeKeyIterator iter;
	BLT_TABLE_ROW row;
	long depth;

	depth = Blt_Tree_NodeDepth(node);
	if ((switchesPtr->maxDepth > 0) && 
	    (depth > (topDepth + switchesPtr->maxDepth))) {
	    /* Skipping node because is it beyond the maximum depth desired. */
	    continue;
	}
	row = blt_table_get_row(table, rowIndex);
	for (key = Blt_Tree_FirstKey(tree, node, &iter); key != NULL;
	     key = Blt_Tree_NextKey(tree, &iter)) {
	    BLT_TABLE_COLUMN col;
	    Tcl_Obj *objPtr;

	    if (Blt_Tree_GetValue(interp, tree, node, key, &objPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    col = blt_table_column_find_by_label(table, key);
	    if (col == NULL) {
		col = blt_table_column_create(interp, table, key);
		if (col == NULL) {
		    return TCL_ERROR;
		}
	    }
	    if (blt_table_set_obj(table, row, col, objPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	rowIndex++;
    }
    return TCL_OK;
}

static int
ImportTreeProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Blt_Tree tree;
    Blt_TreeNode node;
    ImportSwitches switches;
    long inode;

    /* FIXME: 
     *	 2. Export *GetNode tag parsing routines from bltTreeCmd.c,
     *	    instead of using node id to select the top node.
     */
    tree = Blt_Tree_GetFromObj(interp, objv[3]);
    if (tree == NULL) {
	return TCL_ERROR;
    }

    if (Blt_GetLongFromObj(interp, objv[4], &inode) != TCL_OK) {
	return TCL_ERROR;
    }
    node = Blt_Tree_GetNode(tree, inode);
    if (node == NULL) {
	return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, importSwitches, objc - 5, objv + 5, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    return ImportTree(interp, table, tree, node, &switches);
}

static int
ExportTree(Tcl_Interp *interp, BLT_TABLE table, Blt_Tree tree, 
	   Blt_TreeNode parent, ExportSwitches *switchesPtr) 
{
    BLT_TABLE_ROW row;

    for (row = blt_table_first_tagged_row(&switchesPtr->rIter); row != NULL;
	 row = blt_table_next_tagged_row(&switchesPtr->rIter)) {
	BLT_TABLE_COLUMN col;
	Blt_TreeNode node;
	const char *label;

	label = blt_table_row_label(row);
	node = Blt_Tree_FindChild(parent, label);
	if (node == NULL) {
	    node = Blt_Tree_CreateNode(tree, parent, label, -1);
	}
	for (col = blt_table_first_tagged_column(&switchesPtr->cIter); 
	     col != NULL;
	     col = blt_table_next_tagged_column(&switchesPtr->cIter)) {
	    Tcl_Obj *objPtr;
	    const char *key;

	    objPtr = blt_table_get_obj(table, row, col);
	    key = blt_table_column_label(col);
	    if (Blt_Tree_SetValue(interp, tree, node, key, objPtr) != TCL_OK) {
		return TCL_ERROR;
	    }		
	}
    }
    return TCL_OK;
}

static int
ExportTreeProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Blt_Tree tree;
    Blt_TreeNode node;
    ExportSwitches switches;
    int result;

    if (objc < 4) {
	Tcl_AppendResult(interp, "wrong # arguments: should be \"", 
		Tcl_GetString(objv[0]), " export tree treeName ?switches?\"",
		(char *)NULL);
	return TCL_ERROR;
    }
    tree = Blt_Tree_GetFromObj(interp, objv[3]);
    if (tree == NULL) {
	return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    rowIterSwitch.clientData = table;
    columnIterSwitch.clientData = table;
    blt_table_iterate_all_rows(table, &switches.rIter);
    blt_table_iterate_all_columns(table, &switches.cIter);
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 4, objv + 4, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (switches.nodeObjPtr != NULL) {
	long inode;

	if (Blt_GetLongFromObj(interp, switches.nodeObjPtr, &inode) != TCL_OK) {
	    return TCL_ERROR;
	}
	node = Blt_Tree_GetNode(tree, inode);
	if (node == NULL) {
	    return TCL_ERROR;
	}
    } else {
	node = Blt_Tree_RootNode(tree);
    }
    result = ExportTree(interp, table, tree, node, &switches);
    Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
    return result;
}

int 
blt_table_tree_init(Tcl_Interp *interp)
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
    if (Tcl_PkgProvide(interp, "blt_datatable_tree", BLT_VERSION) != TCL_OK) { 
	return TCL_ERROR;
    }
    return blt_table_register_format(interp,
        "tree",			/* Name of format. */
	ImportTreeProc,		/* Import procedure. */
	ExportTreeProc);	/* Export procedure. */

}

int 
blt_table_tree_safe_init(Tcl_Interp *interp)
{
    return blt_table_tree_init(interp);
}

#endif /* NO_DATATABLE */


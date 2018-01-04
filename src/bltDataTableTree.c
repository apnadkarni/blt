/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltDataTableTree.c --
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

#include <bltInt.h>

#include "config.h"

#ifndef NO_DATATABLE

#ifdef HAVE_MEMORY_H
  #include <memory.h>
#endif /* HAVE_MEMORY_H */

#include <tcl.h>

/*
 * Format       Import          Export
 * csv          file/data       file/data
 * tree         data            data
 * vector       data            data
 * xml          file/data       file/data
 * sql          data            data
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

static Blt_SwitchParseProc NodeSwitchProc;
static Blt_SwitchCustom nodeSwitch = {
    NodeSwitchProc, NULL, NULL, 0,
};

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

/*
 * ImportArgs --
 */
typedef struct {
    Blt_TreeNode root;
    size_t maxDepth;
    unsigned int flags;
} ImportArgs;

#define IMPORT_INODES   (1<<0)

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_INT_NNEG, "-depth", "number", (char *)NULL,
        Blt_Offset(ImportArgs, maxDepth), 0},
    {BLT_SWITCH_BITS_NOARG, "-inodes",  "", (char *)NULL,
        Blt_Offset(ImportArgs, flags), 0, IMPORT_INODES},
    {BLT_SWITCH_CUSTOM, "-root", "node", (char *)NULL,
        Blt_Offset(ImportArgs, root), 0, 0, &nodeSwitch},
    {BLT_SWITCH_END}
};

/*
 * ExportArgs --
 */
typedef struct {
    Blt_TreeNode root;
    BLT_TABLE_ITERATOR ri, ci;
} ExportArgs;

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-columns", "columns", (char *)NULL,
        Blt_Offset(ExportArgs, ci), 0, 0, &columnIterSwitch},
    {BLT_SWITCH_CUSTOM, "-root", "node", (char *)NULL,
        Blt_Offset(ExportArgs, root), 0, 0, &nodeSwitch},
    {BLT_SWITCH_CUSTOM, "-rows", "rows", (char *)NULL,
        Blt_Offset(ExportArgs, ri), 0, 0, &rowIterSwitch},
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
 *      Free the storage associated with the -columns switch.
 *
 * Results:
 *      None.
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
 *      Convert a Tcl_Obj representing an offset in the table.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColumnIterSwitchProc(
    ClientData clientData,              /* Flag indicating if the node is
                                         * considered before or after the
                                         * insertion position. */
    Tcl_Interp *interp,                 /* Interpreter to report results. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);
    BLT_TABLE table;
    Tcl_Obj **objv;
    int objc;

    table = clientData;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (blt_table_iterate_columns_objv(interp, table, objc, objv, iterPtr)
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
 *      Free the storage associated with the -rows switch.
 *
 * Results:
 *      None.
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
 *      Convert a Tcl_Obj representing an offset in the table.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RowIterSwitchProc(
    ClientData clientData,              /* Flag indicating if the node is
                                         * considered before or after the
                                         * insertion position. */
    Tcl_Interp *interp,                 /* Interpreter to report results. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    BLT_TABLE_ITERATOR *iterPtr = (BLT_TABLE_ITERATOR *)(record + offset);
    BLT_TABLE table;
    Tcl_Obj **objv;
    int objc;

    table = clientData;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (blt_table_iterate_rows_objv(interp, table, objc, objv, iterPtr)
        != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
NodeSwitchProc(ClientData clientData, Tcl_Interp *interp,
               const char *switchName, Tcl_Obj *objPtr, char *record,
               int offset, int flags)
{
    Blt_Tree tree = clientData;
    Blt_TreeNode *nodePtr = (Blt_TreeNode *)(record + offset);
    Blt_TreeNode node;
    
    if (Blt_Tree_GetNodeFromObj(interp, tree, objPtr, &node) != TCL_OK) {
        return TCL_ERROR;
    }
    *nodePtr = node;
    return TCL_OK;
}

static int
ImportTree(Tcl_Interp *interp, BLT_TABLE table, Blt_Tree tree,
           ImportArgs *argsPtr)
{
    Blt_TreeNode node;
    int maxDepth, topDepth;
    long rowIndex;

    /* 
     * Fill in the table data in 2 passes.  We need to know the maximum
     * depth of the leaf nodes so that we can generate columns for each
     * level of the hierarchy.  We have to do this before adding node data
     * values.
     */

    /* Pass 1.  Create row entries for all the nodes. Add entries for 
     *          the node and it's ancestor's labels. */
    maxDepth = topDepth = Blt_Tree_NodeDepth(argsPtr->root);
    for (node = Blt_Tree_NextNode(argsPtr->root, argsPtr->root); node != NULL;
         node = Blt_Tree_NextNode(argsPtr->root, node)) {
        BLT_TABLE_ROW row;
        Blt_TreeNode parent;
        int depth;
        size_t colIndex;

        depth = Blt_Tree_NodeDepth(node);
        if ((argsPtr->maxDepth > 0) && 
            (depth > (topDepth + argsPtr->maxDepth))) {
            /* Skipping node because is it beyond the maximum depth desired. */
            continue;
        }
        if (depth > maxDepth) {
            BLT_TABLE_COLUMN col;

            if (blt_table_extend_columns(interp, table, 1, &col) != TCL_OK) {
                return TCL_ERROR;
            }
            colIndex = blt_table_column_index(table, col);
            maxDepth = depth;
        } else {
            colIndex = depth - topDepth - 1;
        }
        if (blt_table_extend_rows(interp, table, 1, &row) != TCL_OK) {
            return TCL_ERROR;
        }
        for (parent = node; parent != argsPtr->root; 
             parent = Blt_Tree_ParentNode(parent)){
            const char *label;
            BLT_TABLE_COLUMN col;

            col = blt_table_column(table, colIndex);
            label = Blt_Tree_NodeLabel(parent);
            if (blt_table_set_string_rep(interp, table, row, col, label, -1)
                != TCL_OK) {
                return TCL_ERROR;
            }
            if (argsPtr->flags & IMPORT_INODES) {
                Tcl_Obj *objPtr;
                const char *label;

                label = "inode";
                col = blt_table_get_column_by_label(table, label);
                if (col == NULL) {
                    col = blt_table_create_column(interp, table, label);
                    if (col == NULL) {
                        return TCL_ERROR;
                    }
                }
                objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(node));
                if (blt_table_set_obj(interp, table, row, col, objPtr)
                    != TCL_OK) {
                    return TCL_ERROR;
                }
            }
            colIndex--;
        }
    }
    /* Pass 2.  Fill in entries for all the data fields found. */
    for (rowIndex = 0, node = Blt_Tree_NextNode(argsPtr->root, argsPtr->root);
         node != NULL; node = Blt_Tree_NextNode(argsPtr->root, node)) {
        Blt_TreeKey key;
        Blt_TreeKeyIterator iter;
        BLT_TABLE_ROW row;
        long depth;

        depth = Blt_Tree_NodeDepth(node);
        if ((argsPtr->maxDepth > 0) && 
            (depth > (topDepth + argsPtr->maxDepth))) {
            /* Skipping node because is it beyond the maximum depth desired. */
            continue;
        }
        row = blt_table_row(table, rowIndex);
        for (key = Blt_Tree_FirstKey(tree, node, &iter); key != NULL;
             key = Blt_Tree_NextKey(tree, &iter)) {
            BLT_TABLE_COLUMN col;
            Tcl_Obj *objPtr;

            if (Blt_Tree_GetValue(interp, tree, node, key, &objPtr) != TCL_OK) {
                return TCL_ERROR;
            }
            col = blt_table_get_column_by_label(table, key);
            if (col == NULL) {
                col = blt_table_create_column(interp, table, key);
                if (col == NULL) {
                    return TCL_ERROR;
                }
            }
            if (blt_table_set_obj(interp, table, row, col, objPtr) != TCL_OK) {
                return TCL_ERROR;
            }
        }
        rowIndex++;
    }
    return TCL_OK;
}

static int
ExportTree(Tcl_Interp *interp, BLT_TABLE table, Blt_Tree tree, 
           ExportArgs *argsPtr) 
{
    BLT_TABLE_ROW row;

    for (row = blt_table_first_tagged_row(&argsPtr->ri); row != NULL;
         row = blt_table_next_tagged_row(&argsPtr->ri)) {
        BLT_TABLE_COLUMN col;
        Blt_TreeNode node;
        const char *rowName;

        rowName = blt_table_row_label(row);
        node = Blt_Tree_FindChild(argsPtr->root, rowName);
        if (node == NULL) {
            node = Blt_Tree_CreateNode(tree, argsPtr->root, rowName, -1);
        }
        for (col = blt_table_first_tagged_column(&argsPtr->ci); col != NULL;
             col = blt_table_next_tagged_column(&argsPtr->ci)) {
            Tcl_Obj *objPtr;
            const char *colName;

            objPtr = blt_table_get_obj(table, row, col);
            colName = blt_table_column_label(col);
            if (Blt_Tree_SetValue(interp, tree, node, colName, objPtr) 
                != TCL_OK) {
                return TCL_ERROR;
            }           
        }
    }
    return TCL_OK;
}

/* 
 * tableName import tree treeName switches...
 */
static int
ImportTreeProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Blt_Tree tree;
    ImportArgs args;
    int result;
    
    if (objc < 4) {
        Tcl_AppendResult(interp, "wrong # arguments: should be \"", 
                Tcl_GetString(objv[0]), " import tree treeName ?switches?\"",
                (char *)NULL);
        return TCL_ERROR;
    }
    tree = Blt_Tree_GetFromObj(interp, objv[3]);
    if (tree == NULL) {
        return TCL_ERROR;
    }
    memset(&args, 0, sizeof(args));
    nodeSwitch.clientData = tree;
    args.root = Blt_Tree_RootNode(tree);
    if (Blt_ParseSwitches(interp, importSwitches, objc - 4, objv + 4, &args,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    result = ImportTree(interp, table, tree, &args);
    Blt_FreeSwitches(importSwitches, &args, 0);
    return result;
}

/* 
 * tableName export tree treeName switches...
 */
static int
ExportTreeProc(BLT_TABLE table, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Blt_Tree tree;
    ExportArgs args;
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
    memset(&args, 0, sizeof(args));
    args.root = Blt_Tree_RootNode(tree);
    rowIterSwitch.clientData = table;
    columnIterSwitch.clientData = table;
    nodeSwitch.clientData = tree;
    blt_table_iterate_all_rows(table, &args.ri);
    blt_table_iterate_all_columns(table, &args.ci);
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 4, objv + 4, &args,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    result = ExportTree(interp, table, tree, &args);
    Blt_FreeSwitches(exportSwitches, (char *)&args, 0);
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
        "tree",                 /* Name of format. */
        ImportTreeProc,         /* Import procedure. */
        ExportTreeProc);        /* Export procedure. */

}

int 
blt_table_tree_safe_init(Tcl_Interp *interp)
{
    return blt_table_tree_init(interp);
}

#endif /* NO_DATATABLE */


/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltTclPkgsInit.c --
 *
 * This module initials the BLT toolkit, registering its commands with the
 * Tcl/Tk interpreter.
 *
 *	Copyright 1991-2004 George A Howlett.
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

#define BUILD_BLT_TCL_PROCS 1
#include <bltInt.h>
/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */

BLT_EXTERN Tcl_AppInitProc Blt_TclPkgsInit;
BLT_EXTERN Tcl_AppInitProc Blt_TclInit;
BLT_EXTERN Tcl_AppInitProc Blt_TclSafeInit;

/* Data table format packages. */
BLT_EXTERN Tcl_AppInitProc blt_table_txt_init;
BLT_EXTERN Tcl_AppInitProc blt_table_txt_safe_init;
BLT_EXTERN Tcl_AppInitProc blt_table_csv_init;
BLT_EXTERN Tcl_AppInitProc blt_table_csv_safe_init;
#ifdef HAVE_LIBMYSQL
BLT_EXTERN Tcl_AppInitProc blt_table_mysql_init;
BLT_EXTERN Tcl_AppInitProc blt_table_mysql_safe_init;
#endif	/* HAVE_LIBMYSQL */
BLT_EXTERN Tcl_AppInitProc blt_table_tree_init;
BLT_EXTERN Tcl_AppInitProc blt_table_vector_init;
BLT_EXTERN Tcl_AppInitProc blt_table_tree_safe_init;
BLT_EXTERN Tcl_AppInitProc blt_table_vector_safe_init;
#ifdef HAVE_LIBEXPAT
BLT_EXTERN Tcl_AppInitProc blt_table_xml_init;
BLT_EXTERN Tcl_AppInitProc blt_table_xml_safe_init;
#endif

/* Tree format packages. */
#ifdef HAVE_LIBEXPAT
BLT_EXTERN Tcl_AppInitProc Blt_TreeXmlInit;
BLT_EXTERN Tcl_AppInitProc Blt_TreeXmlSafeInit;
#endif

int
Blt_TclPkgsInit(Tcl_Interp *interp)	/* Interpreter for application. */
{
    if (Tcl_PkgRequire(interp, "blt_tcl", BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    }
    /* Tcl-only static packages */
    /* Data table packages. */
    /* TXT */
    if (blt_table_txt_init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_datatable_txt", blt_table_txt_init, 
	blt_table_txt_safe_init);
    /* CSV */
    if (blt_table_csv_init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_datatable_csv", blt_table_csv_init, 
	blt_table_csv_safe_init);
    /* MYSQL */
#ifdef HAVE_LIBMYSQL
    if (blt_table_mysql_init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_datatable_mysql", blt_table_mysql_init, 
	blt_table_mysql_safe_init);
#endif	/* HAVE_LIBMYSQL */
    /* TREE */
    if (blt_table_tree_init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_datatable_tree", blt_table_tree_init, 
	blt_table_tree_safe_init);
    /* VECTOR */
    if (blt_table_vector_init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_datatable_vector", blt_table_vector_init,
	blt_table_vector_safe_init);
    /* XML */
#ifdef HAVE_LIBEXPAT
    if (blt_table_xml_init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_datatable_xml", blt_table_xml_init, 
	blt_table_xml_safe_init);
#endif	/* HAVE_LIBEXPAT */

    /* Tree packages. */
#ifdef HAVE_LIBEXPAT
    if (Blt_TreeXmlInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_tree_xml", Blt_TreeXmlInit, 
	Blt_TreeXmlSafeInit);
#endif	/* HAVE_LIBEXPAT */
    return TCL_OK;
}


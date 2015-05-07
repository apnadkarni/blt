/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTclPkgsInit.c --
 *
 * This module initials the BLT toolkit, registering its commands with the
 * Tcl/Tk interpreter.
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
#endif  /* HAVE_LIBMYSQL */
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
Blt_TclPkgsInit(Tcl_Interp *interp)     /* Interpreter for application. */
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
#endif  /* HAVE_LIBMYSQL */
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
#endif  /* HAVE_LIBEXPAT */

    /* Tree packages. */
#ifdef HAVE_LIBEXPAT
    if (Blt_TreeXmlInit(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "blt_tree_xml", Blt_TreeXmlInit, 
        Blt_TreeXmlSafeInit);
#endif  /* HAVE_LIBEXPAT */
    return TCL_OK;
}


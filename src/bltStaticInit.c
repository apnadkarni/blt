/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltStaticInit.c --
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

#include <bltInt.h>

extern Tcl_AppInitProc Blt_TclInit;
extern Tcl_AppInitProc Blt_TclPkgsInit;
extern Tcl_AppInitProc Blt_TkInit;
extern Tcl_AppInitProc Blt_TkPkgsInit;
extern Tcl_AppInitProc Blt_StaticInit;

int
Blt_StaticInit(Tcl_Interp *interp)	/* Interpreter for application. */
{
    if (Blt_TclInit(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (Blt_TclPkgsInit(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (Blt_TkInit(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (Blt_TkPkgsInit(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    return TCL_OK;
}


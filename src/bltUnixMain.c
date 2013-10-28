/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltUnixMain.c --
 *
 * Provides a default version of the Tcl_AppInit procedure for
 * use in wish and similar Tk-based applications.
 *
 *	Copyright 1998-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file was adapted from the Tk distribution.
 *
 *	Copyright (c) 1993 The Regents of the University of
 *	California. All rights reserved.
 *
 *	Permission is hereby granted, without written agreement and
 *	without license or royalty fees, to use, copy, modify, and
 *	distribute this software and its documentation for any
 *	purpose, provided that the above copyright notice and the
 *	following two paragraphs appear in all copies of this
 *	software.
 *
 *	IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
 *	ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
 *	CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS
 *	SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 *	CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
 *	WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *	PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 *	BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO *
 *	PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
 *	MODIFICATIONS.
 *
 */

#include "config.h"
#ifdef USE_TCL_STUBS
#  define HAVE_TCL_STUBS 1
#  undef USE_TCL_STUBS
#endif
#include <tcl.h>
#ifndef TCL_ONLY
#include <tk.h>
#endif
#include <blt.h>

/*
 * The following variable is a special hack that is needed in order for
 * Sun shared libraries to be used for Tcl.
 */
#ifdef NEED_MATHERR
BLT_EXTERN int matherr();
int *tclDummyMathPtr = (int *)matherr;
#endif

#ifdef TCL_ONLY

BLT_EXTERN Tcl_AppInitProc Blt_TclInit;
BLT_EXTERN Tcl_AppInitProc Blt_TclSafeInit;

#ifdef STATIC_PKGS
BLT_EXTERN Tcl_AppInitProc Blt_TclPkgsInit;
#endif /* STATIC_PKGS */

static int
Initialize(Tcl_Interp *interp) 
{
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_Init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
#ifdef TCLLIBPATH
    /* 
     * It seems that some distributions of TCL don't compile-in a
     * default location of the library.  This causes Tcl_Init to fail
     * if bltwish and bltsh are moved to another directory. The
     * workaround is to set the magic variable "tclDefaultLibrary".
     */
    Tcl_SetVar(interp, "tclDefaultLibrary", TCLLIBPATH, TCL_GLOBAL_ONLY);
#endif /* TCLLIBPATH */

#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    };
#else
    if (Blt_TclInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
#endif	/*USE_BLT_STUBS*/
#ifdef STATIC_PKGS
    if (Blt_TclPkgsInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
#endif	/*STATIC_PACKAGES*/
    Tcl_SetVar(interp, "tcl_rcFileName", "~/tclshrc.tcl", TCL_GLOBAL_ONLY);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tk_Main never returns here, so this procedure never
 *	returns either.
 *
 * Side effects:
 *	Whatever the application does.
 *
 *---------------------------------------------------------------------------
 */
int
main(int argc, char **argv)
{
    Tcl_Main(argc, argv, Initialize);
    return 0;				/* Suppress compiler warning. */
}

#else 

BLT_EXTERN Tcl_AppInitProc Blt_TclInit;
BLT_EXTERN Tcl_AppInitProc Blt_TclSafeInit;
BLT_EXTERN Tcl_AppInitProc Blt_TkInit;
BLT_EXTERN Tcl_AppInitProc Blt_TkSafeInit;

#ifdef STATIC_PKGS
BLT_EXTERN Tcl_AppInitProc Blt_TclPkgsInit;
BLT_EXTERN Tcl_AppInitProc Blt_TkPkgsInit;
#endif /* STATIC_PKGS */

static int
Initialize(Tcl_Interp *interp) 
{
    if (Tcl_PkgRequire(interp, "Tcl", TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_Init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_PkgRequire(interp, "Tk", TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	return TCL_ERROR;
    }
#ifdef TCLLIBPATH
    /* 
     * It seems that some distributions of TCL don't compile-in a
     * default location of the library.  This causes Tcl_Init to fail
     * if bltwish and bltsh are moved to another directory. The
     * workaround is to set the magic variable "tclDefaultLibrary".
     */
    Tcl_SetVar(interp, "tclDefaultLibrary", TCLLIBPATH, TCL_GLOBAL_ONLY);
#endif /* TCLLIBPATH */
#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    };
    if (Blt_InitTkStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
	return TCL_ERROR;
    };
#else 
    if (Blt_TclInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_TkInit(interp)  != TCL_OK) {
	return TCL_ERROR;
    }
#endif	/*USE_BLT_STUBS*/
#ifdef STATIC_PKGS
    if (Blt_TclPkgsInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_TkPkgsInit(interp) != TCL_OK) {
	return TCL_ERROR;
    }
#endif	/*STATIC_PACKAGES*/
    Tcl_SetVar(interp, "tcl_rcFileName", "~/wishrc.tcl", TCL_GLOBAL_ONLY);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tk_Main never returns here, so this procedure never
 *	returns either.
 *
 * Side effects:
 *	Whatever the application does.
 *
 *---------------------------------------------------------------------------
 */
int
main(int argc, char **argv)
{
    Tcl_Interp *interp;

    interp = Tcl_CreateInterp();
    if (Tcl_Init(interp) != TCL_OK) {
	Tcl_Panic("can't initialize Tcl");
    }
#ifdef HAVE_TCL_STUBS
#undef Tcl_InitStubs
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	Tcl_Panic("Can't initialize TCL stubs");
    }
#endif
#ifdef USE_TK_STUBS
    if (Tk_InitStubs(interp, TK_VERSION_COMPILED, PKG_ANY) == NULL) {
	Tcl_Panic("Can't initialize Tk stubs");
    }
#else
    if (Tk_Init(interp) != TCL_OK) {
	return TCL_ERROR;
    }
#endif
    Tk_MainEx(argc, argv, Initialize, interp);
    return 0;				/* Suppress compiler warning. */
}
#endif /*TCL_ONLY*/

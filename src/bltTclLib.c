/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltTclLib.c --
 *
 *	Stub object that will be statically linked into extensions that wish
 *	to access Tcl-related routines in BLT.
 *
 * Copyright (c) 1998 Paul Duffin.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/*
 * We need to ensure that we use the stub macros so that this file contains no
 * references to any of the stub functions. This will make it possible to
 * build an extension that references Tcl_InitStubs but doesn't end up
 * including the rest of the stub functions.
 */
#include "bltInt.h"

BltTclProcs *bltTclProcsPtr = NULL;
BltTclIntProcs *bltTclIntProcsPtr = NULL;
/*
 * Use our own isdigit to avoid linking to libc on windows
 */
static int 
IsDigit(const int c)
{
    return (c >= '0' && c <= '9');
}

/*
 *----------------------------------------------------------------------
 *
 * Blt_InitTclStubs --
 *
 *	Checks that the correct version of TCL is loaded and that it supports
 *	stubs. It then initialises the stub table pointers.
 *
 * Results:
 *	The actual version of TCL that satisfies the request, or NULL to
 *	indicate that an error occurred.
 *
 * Side effects:
 *	Sets the stub table pointers.
 *
 *----------------------------------------------------------------------
 */

#ifdef Blt_InitTclStubs
#undef Blt_InitTclStubs
#endif

const char *
Blt_InitTclStubs(Tcl_Interp *interp, const char *version, int exact)
{
    const char *actual;
    ClientData clientData;

    /* Initialize the stubs table before calling any TCL routines in the BLT
     * library.  The application may have initialized it's own stub table for
     * TCL, but that's different from the one we use in the BLT library. */
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
	Tcl_Panic("Can't initialize TCL stubs");
    }
    actual = Tcl_PkgRequireEx(interp, "blt_tcl", version, 0, &clientData);
    if (actual == NULL) {
	return NULL;
    }
    if (exact) {
	const char *p;
	int count = 0;

	p = version;
	count = 0;
	while (*p != '\0') {
	    count += !IsDigit(*p++);
	}
	if (count == 1) {
	    const char *q;

	    q = actual;
	    p = version;
	    while (*p && (*p == *q)) {
		p++; q++;
	    }
	    if (*p) {
		/* Construct error message */
		Tcl_PkgRequireEx(interp, "blt_tcl", version, 1, NULL);
		return NULL;

	    }
	} else {
	    actual = Tcl_PkgRequireEx(interp, "blt_tcl", version, 1,NULL);
	    if (actual == NULL) {
		return NULL;
	    }
	}
    }

    if (clientData == NULL) {
	Tcl_AppendResult(interp,
		"This implementation of BLT TCL does not support stubs",
			 (char *)NULL);
	return NULL;
    }
    bltTclProcsPtr = clientData;
    bltTclIntProcsPtr = bltTclProcsPtr->hooks->bltTclIntProcs;
    return actual;
}

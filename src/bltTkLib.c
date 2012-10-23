
/*
 * bltTkLib.c --
 *
 *	Stub object that will be statically linked into extensions that wish
 *	to access Tk-related routines in BLT.
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
 * build an extension that references Blt_InitTkStubs but doesn't end up
 * including the rest of the stub functions.
 */
#include "bltInt.h"

BltTkProcs *bltTkProcsPtr = NULL;
BltTkIntProcs *bltTkIntProcsPtr = NULL;

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
 * Blt_InitTkStubs --
 *
 *	Checks that the correct version of Tk is loaded and that it supports
 *	stubs. It then initialises the stub table pointers.
 *
 * Results:
 *	The actual version of Tk that satisfies the request, or NULL to
 *	indicate that an error occurred.
 *
 * Side effects:
 *	Sets the stub table pointers.
 *
 *----------------------------------------------------------------------
 */

#ifdef Blt_InitTkStubs
#undef Blt_InitTkStubs
#endif

const char *
Blt_InitTkStubs(Tcl_Interp *interp, const char *version, int exact)
{
    const char *actual;
    ClientData clientData;

    actual = Tcl_PkgRequireEx(interp, "blt_tk", version, PKG_ANY, &clientData);
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
		Tcl_PkgRequireEx(interp, "blt_tk", version, PKG_EXACT, NULL);
                return NULL;

            }
        } else {
            actual = Tcl_PkgRequireEx(interp, "blt_tk", version, PKG_EXACT, 
		NULL);
            if (actual == NULL) {
                return NULL;
            }
        }
    }

    if (clientData == NULL) {
	Tcl_AppendResult(interp,
		"This implementation of Blt Tk does not support stubs",
			 (char *)NULL);
	return NULL;
    }
    bltTkProcsPtr = clientData;
    bltTkIntProcsPtr = bltTkProcsPtr->hooks->bltTkIntProcs;
    return actual;
}

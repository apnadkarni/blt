/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTclLib.c --
 *
 * Stub object that will be statically linked into extensions that want to
 * access Tcl-related routines in BLT.
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

#include "bltInt.h"

extern const char *Blt_InitTclStubs(Tcl_Interp *interp, const char *version,
        int exact);

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

#ifdef Blt_InitTclStubs
#undef Blt_InitTclStubs
#endif

/*
 *----------------------------------------------------------------------
 *
 * Blt_InitTclStubs --
 *
 *      Checks that the correct version of TCL is loaded and that it
 *      supports stubs. It then initialises the stub table pointers.
 *
 * Results:
 *      The actual version of TCL that satisfies the request, or NULL to
 *      indicate that an error occurred.
 *
 * Side effects:
 *      Sets the stub table pointers.
 *
 *----------------------------------------------------------------------
 */
const char *
Blt_InitTclStubs(Tcl_Interp *interp, const char *version, int exact)
{
    const char *actual;
    ClientData clientData;

    /* Initialize the stubs table before calling any TCL routines in the
     * BLT library.  The application may have initialized it's own stub
     * table for TCL, but that's different from the one we use in the BLT
     * library. */
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
        Tcl_AppendResult(interp, "This implementation of the BLT TCL ",
                "module does not support stubs", (char *)NULL);
        return NULL;
    }
    bltTclProcsPtr = clientData;
    bltTclIntProcsPtr = bltTclProcsPtr->hooks->bltTclIntProcs;
    return actual;
}

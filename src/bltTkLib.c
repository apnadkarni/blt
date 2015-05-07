/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTkLib.c --
 *
 *      Stub object that will be statically linked into extensions that wish
 *      to access Tk-related routines in BLT.
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
 * Copyright (c) 1998 Paul Duffin.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 *   See the file "license.terms" for information on usage and
 *   redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/*
 * We need to ensure that we use the stub macros so that this file contains no
 * references to any of the stub functions. This will make it possible to
 * build an extension that references Blt_InitTkStubs but doesn't end up
 * including the rest of the stub functions.
 */
#include "bltInt.h"

extern const char *Blt_InitTkStubs(Tcl_Interp *interp, const char *version,
        int exact);

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
 *      Checks that the correct version of Tk is loaded and that it supports
 *      stubs. It then initialises the stub table pointers.
 *
 * Results:
 *      The actual version of Tk that satisfies the request, or NULL to
 *      indicate that an error occurred.
 *
 * Side effects:
 *      Sets the stub table pointers.
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

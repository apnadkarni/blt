/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltStaticInit.c --
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

#include <bltInt.h>

extern Tcl_AppInitProc Blt_TclInit;
extern Tcl_AppInitProc Blt_TclPkgsInit;
extern Tcl_AppInitProc Blt_TkInit;
extern Tcl_AppInitProc Blt_TkPkgsInit;
extern Tcl_AppInitProc Blt_StaticInit;

int
Blt_StaticInit(Tcl_Interp *interp)      /* Interpreter for application. */
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


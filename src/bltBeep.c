/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltBeep.c --
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_BEEP
#include "bltInitCmd.h"

static Tcl_ObjCmdProc BeepCmd;

/*
 *---------------------------------------------------------------------------
 *
 * Blt_BeepCmd --
 *
 *      This procedure is invoked to process the "beep" command.  See the
 *      user documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
BeepCmd(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    int percent;

    if (objc > 2) {
        Tcl_AppendResult(interp, "wrong # args: should be \"",
                 Tcl_GetString(objv[0]), " ?volumePercent?\"", (char *)NULL);
        return TCL_ERROR;
    }
    percent = 50;               /* Default setting */
    if (objc == 2) {
        if (Tcl_GetIntFromObj(interp, objv[1], &percent) != TCL_OK) {
            return TCL_ERROR;
        }
        if (percent < -100) {
            percent = -100;
        } else if (percent > 100) {
            percent = 100;
        }
    }
    XBell(Tk_Display(Tk_MainWindow(interp)), percent);
    return TCL_OK;
}

int
Blt_BeepCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "beep", BeepCmd, };

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}
#endif /* NO_BEEP */

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltBeep.c --
 *
 *	Copyright 1993-2003 George A Howlett.
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
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_BEEP
#include "bltInitCmd.h"

/*
 *---------------------------------------------------------------------------
 *
 * Blt_BeepCmd --
 *
 *	This procedure is invoked to process the "beep" command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static Tcl_ObjCmdProc BeepCmd;

/* ARGSUSED */
static int
BeepCmd(
    ClientData clientData,	/* Main window associated with interpreter.*/
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Argument strings. */
{
    int percent;

    if (objc > 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		 Tcl_GetString(objv[0]), " ?volumePercent?\"", (char *)NULL);
	return TCL_ERROR;
    }
    percent = 50;		/* Default setting */
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

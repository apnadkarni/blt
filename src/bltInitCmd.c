/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltInitCmd.c --
 *
 * The functions in this module are used to initialize commands of the BLT
 * toolkit with the TCL interpreter.
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

#define BUILD_BLT_TCL_PROCS 1
#include "bltInt.h"
#include "bltNsUtil.h"
#include "bltInitCmd.h"

/*
 *---------------------------------------------------------------------------
 *
 * Blt_InitCmd --
 *
 *      Given the name of a command, return a pointer to the clientData field
 *      of the command.
 *
 * Results:
 *      A standard TCL result. If the command is found, TCL_OK is returned
 *      and clientDataPtr points to the clientData field of the command (if
 *      the clientDataPtr in not NULL).
 *
 * Side effects:
 *      If the command is found, clientDataPtr is set to the address of the
 *      clientData of the command.  If not found, an error message is left
 *      in interp->result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Blt_InitCmd(Tcl_Interp *interp, const char *nsName, Blt_CmdSpec *specPtr)
{
    const char *cmdPath;
    Tcl_DString ds;
    Tcl_Command cmdToken;
    Tcl_Namespace *nsPtr;

    Tcl_DStringInit(&ds);
    if (nsName != NULL) {
	Tcl_DStringAppend(&ds, nsName, -1);
    } 
    Tcl_DStringAppend(&ds, "::", -1);
    Tcl_DStringAppend(&ds, specPtr->name, -1);

    cmdPath = Tcl_DStringValue(&ds);
    cmdToken = Tcl_FindCommand(interp, cmdPath, (Tcl_Namespace *)NULL, 0);
    if (cmdToken != NULL) {
	Tcl_DStringFree(&ds);
	return TCL_OK;		/* Assume command was already initialized */
    }
    cmdToken = Tcl_CreateObjCommand(interp, cmdPath, specPtr->cmdProc, 
	specPtr->clientData, specPtr->cmdDeleteProc);
    Tcl_DStringFree(&ds);
    nsPtr = Tcl_FindNamespace(interp, nsName, (Tcl_Namespace *)NULL,
	TCL_LEAVE_ERR_MSG);
    if (nsPtr == NULL) {
	return TCL_ERROR;
    }
    if (Tcl_Export(interp, nsPtr, specPtr->name, FALSE) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_InitCmds --
 *
 *      Given the name of a command, return a pointer to the clientData field
 *      of the command.
 *
 * Results:
 *      A standard TCL result. If the command is found, TCL_OK is returned and
 *      clientDataPtr points to the clientData field of the command (if the
 *      clientDataPtr in not NULL).
 *
 * Side effects:
 *      If the command is found, clientDataPtr is set to the address of the
 *      clientData of the command.  If not found, an error message is left in
 *      interp->result.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_InitCmds(Tcl_Interp *interp, const char *nsName, Blt_CmdSpec *specs, 
	     int numCmds)
{
    int i;

    for (i = 0; i < numCmds; i++) {
	if (Blt_InitCmd(interp, nsName, specs + i) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

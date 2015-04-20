/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltInitCmd.c --
 *
 * The functions in this module are used to initialize commands of the BLT
 * toolkit with the TCL interpreter.
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
    Tcl_CreateObjCommand(interp, cmdPath, specPtr->cmdProc, specPtr->clientData,
			 specPtr->cmdDeleteProc);
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

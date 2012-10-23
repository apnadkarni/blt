
/*
 * bltCutbuffer.c --
 *
 *	Copyright 1993-1998 George A Howlett.
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

#ifndef NO_CUTBUFFER

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifndef WIN32
#include <X11/Xproto.h>
#endif

#include "bltAlloc.h"
#include "bltOp.h"
#include "bltInitCmd.h"

static int
GetCutNumberFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *bufferPtr)
{
    int number;

    if (Tcl_GetIntFromObj(interp, objPtr, &number) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((number < 0) || (number > 7)) {
	Tcl_AppendResult(interp, "bad buffer # \"", Tcl_GetString(objPtr), 
		"\"", (char *)NULL);
	return TCL_ERROR;
    }
    *bufferPtr = number;
    return TCL_OK;
}

/* ARGSUSED */
static int
RotateErrorProc(ClientData clientData, XErrorEvent *errEventPtr)
{
    int *errorPtr = clientData;

    *errorPtr = TCL_ERROR;
    return 0;
}

static int
GetOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tk_Window tkwin = clientData;
    char *string;
    int buffer;
    int numBytes;

    buffer = 0;
    if (objc == 3) {
	if (GetCutNumberFromObj(interp, objv[2], &buffer) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    string = XFetchBuffer(Tk_Display(tkwin), &numBytes, buffer);
    if (string != NULL) {
	int limit;
	char *p;
	int i;

	if (string[numBytes - 1] == '\0') {
	    limit = numBytes - 1;
	} else {
	    limit = numBytes;
	}
	for (p = string, i = 0; i < limit; i++, p++) {
	    int c;

	    c = (unsigned char)*p;
	    if (c == 0) {
		*p = ' ';	/* Convert embedded NUL bytes */
	    }
	}
	if (limit == numBytes) {
	    char *newPtr;

	    /*
	     * Need to copy the string into a bigger buffer so we can
	     * add a NUL byte on the end.
	     */
	    newPtr = Blt_AssertMalloc(numBytes + 1);
	    memcpy(newPtr, string, numBytes);
	    newPtr[numBytes] = '\0';
	    Blt_Free(string);
	    string = newPtr;
	}
	Tcl_SetStringObj(Tcl_GetObjResult(interp), string, numBytes);
    }
    return TCL_OK;
}

static int
RotateOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tk_Window tkwin = clientData;
    int count;
    int result;
    Tk_ErrorHandler handler;

    count = 1;			/* Default: rotate one position */
    if (objc == 3) {
	if (Tcl_GetIntFromObj(interp, objv[2], &count) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((count < 0) || (count > 8)) {
	    Tcl_AppendResult(interp, "bad rotate count \"", 
		Tcl_GetString(objv[2]), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    result = TCL_OK;
    handler = Tk_CreateErrorHandler(Tk_Display(tkwin), BadMatch,
	X_RotateProperties, -1, RotateErrorProc, &result);
    XRotateBuffers(Tk_Display(tkwin), count);
    Tk_DeleteErrorHandler(handler);
    XSync(Tk_Display(tkwin), False);
    if (result != TCL_OK) {
	Tcl_AppendResult(interp, "can't rotate cutbuffers unless all are set",
	    (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}


static int
SetOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tk_Window tkwin = clientData;
    int buffer;
    char *string;
    int length;

    buffer = 0;
    if (objc == 4) {
	if (GetCutNumberFromObj(interp, objv[3], &buffer) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    string = Tcl_GetStringFromObj(objv[2],  &length);
    XStoreBuffer(Tk_Display(tkwin), string, length + 1, buffer);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BLT Sub-command specification:
 *
 *	- Name of the sub-command.
 *	- Minimum number of characters needed to unambiguously
 *        recognize the sub-command.
 *	- Pointer to the function to be called for the sub-command.
 *	- Minimum number of arguments accepted.
 *	- Maximum number of arguments accepted.
 *	- String to be displayed for usage.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec cbOps[] =
{
    {"get",    1, GetOp,    2, 3, "?buffer?",},
    {"rotate", 1, RotateOp, 2, 3, "?count?",},
    {"set",    1, SetOp,    3, 4, "value ?buffer?",},
};
static int numCbOps = sizeof(cbOps) / sizeof(Blt_OpSpec);


/*
 *---------------------------------------------------------------------------
 *
 * CutBufferCmd --
 *
 *	This procedure is invoked to process the "cutbuffer" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CutbufferCmd(
    ClientData clientData,	/* Main window associated with
				 * interpreter.*/
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Argument strings. */
{
    Tk_Window tkwin;
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numCbOps, cbOps, BLT_OP_ARG1, 
		    objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    tkwin = Tk_MainWindow(interp);
    result = (*proc) (tkwin, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CutbufferCmdInitProc --
 *
 *	This procedure is invoked to initialize the "cutbuffer" Tcl
 *	command. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_CutbufferCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"cutbuffer", CutbufferCmd,};

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#endif /* NO_CUTBUFFER */


/*
 * bltOldConfig.c --
 *
 * This module implements custom configuration options for the BLT
 * toolkit.
 *
 *	Copyright 1991-2004 George A Howlett.
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

#ifdef HAVE_LIMITS_H
#  include <limits.h>
#endif	/* HAVE_LIMITS_H */

#ifdef HAVE_STDARG_H
#  include <stdarg.h>
#endif	/* HAVE_STDARG_H */

#include "bltAlloc.h"

static int StringToPad(ClientData clientData, Tcl_Interp *interp, 
	Tk_Window tkwin, const char *string, char *widgRec, int offset);
static char *PadToString(ClientData clientData, Tk_Window tkwin, char *widgRec,
	int offset, Tcl_FreeProc **freeProcPtr);

Tk_CustomOption bltPadOption =
{
    StringToPad, PadToString, (ClientData)0
};

static int StringToDistance(ClientData clientData, Tcl_Interp *interp, 
	Tk_Window tkwin, const char *string, char *widgRec, int flags);
static char *DistanceToString(ClientData, Tk_Window, char *widgRec, int offset,
	Tcl_FreeProc **freeProcPtr);

Tk_CustomOption bltDistanceOption =
{
    StringToDistance, DistanceToString, (ClientData)PIXELS_NNEG
};

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPixels --
 *
 *	Like Tk_GetPixels, but checks for negative, zero.
 *	Can be PIXELS_POS, PIXELS_NNEG, or PIXELS_ANY.
 *
 * Results:
 *	A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetPixels(Tcl_Interp *interp, Tk_Window tkwin, const char *string, 
	      int check, int *valuePtr)
{
    int length;

    if (Tk_GetPixels(interp, tkwin, string, &length) != TCL_OK) {
	return TCL_ERROR;
    }
    if (length >= SHRT_MAX) {
	Tcl_AppendResult(interp, "bad distance \"", string, "\": ",
	    "too big to represent", (char *)NULL);
	return TCL_ERROR;
    }
    switch (check) {
    case PIXELS_NNEG:
	if (length < 0) {
	    Tcl_AppendResult(interp, "bad distance \"", string, "\": ",
		"can't be negative", (char *)NULL);
	    return TCL_ERROR;
	}
	break;
    case PIXELS_POS:
	if (length <= 0) {
	    Tcl_AppendResult(interp, "bad distance \"", string, "\": ",
		"must be positive", (char *)NULL);
	    return TCL_ERROR;
	}
	break;
    case PIXELS_ANY:
	break;
    }
    *valuePtr = length;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StringToDistance --
 *
 *	Like TK_CONFIG_PIXELS, but adds an extra check for negative
 *	values.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToDistance(
    ClientData clientData,	/* Indicated how to check distance */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Window */
    const char *string,		/* Pixel value string */
    char *widgRec,		/* Widget record */
    int offset)			/* Offset of pixel size in record */
{
    int *valuePtr = (int *)(widgRec + offset);
    return Blt_GetPixels(interp, tkwin, string, (unsigned long)clientData, 
	valuePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * DistanceToString --
 *
 *	Returns the string representing the positive pixel size.
 *
 * Results:
 *	The pixel size string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static char *
DistanceToString(
    ClientData clientData,	/* Not used. */
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Widget structure record */
    int offset,			/* Offset in widget record */
    Tcl_FreeProc **freeProcPtr)	/* Not used. */
{
    int value = *(int *)(widgRec + offset);
    char *result;

    result = Blt_AssertStrdup(Blt_Itoa(value));
    *freeProcPtr = (Tcl_FreeProc *)Blt_Free;
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * StringToPad --
 *
 *	Convert a string into two pad values.  The string may be in one of
 *	the following forms:
 *
 *	    n    - n is a non-negative integer. This sets both
 *		   pad values to n.
 *	  {n m}  - both n and m are non-negative integers. side1
 *		   is set to n, side2 is set to m.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	Otherwise, TCL_ERROR is returned and an error message is left in
 *	interp->result.
 *
 * Side Effects:
 *	The padding structure passed is updated with the new values.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
StringToPad(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Window */
    const char *string,		/* Pixel value string */
    char *widgRec,		/* Widget record */
    int offset)			/* Offset of pad in widget */
{
    Blt_Pad *padPtr = (Blt_Pad *)(widgRec + offset);
    const char **argv;
    int argc;
    int pad, result;

    if (Tcl_SplitList(interp, string, &argc, &argv) != TCL_OK) {
	return TCL_ERROR;
    }
    result = TCL_ERROR;
    if ((argc < 1) || (argc > 2)) {
	Tcl_AppendResult(interp, "wrong # elements in padding list",
	    (char *)NULL);
	goto error;
    }
    if (Blt_GetPixels(interp, tkwin, argv[0], PIXELS_NNEG, &pad)
	!= TCL_OK) {
	goto error;
    }
    padPtr->side1 = pad;
    if ((argc > 1) && (Blt_GetPixels(interp, tkwin, argv[1], PIXELS_NNEG, &pad)
	    != TCL_OK)) {
	goto error;
    }
    padPtr->side2 = pad;
    result = TCL_OK;

  error:
    Blt_Free(argv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * PadToString --
 *
 *	Converts the two pad values into a TCL list.  Each pad has two
 *	pixel values.  For vertical pads, they represent the top and bottom
 *	margins.  For horizontal pads, they're the left and right margins.
 *	All pad values are non-negative integers.
 *
 * Results:
 *	The padding list is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static char *
PadToString(
    ClientData clientData,	/* Not used. */
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Structure record */
    int offset,			/* Offset of pad in record */
    Tcl_FreeProc **freeProcPtr)	/* Not used. */
{
    Blt_Pad *padPtr = (Blt_Pad *)(widgRec + offset);
    char *result;
    char string[200];

    Blt_FormatString(string, 200, "%d %d", padPtr->side1, padPtr->side2);
    result = Blt_AssertStrdup(string);
    *freeProcPtr = (Tcl_FreeProc *)Blt_Free;
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_OldConfigModified --
 *
 *      Given the configuration specifications and one or more option
 *	patterns (terminated by a NULL), indicate if any of the matching
 *	configuration options has been reset.
 *
 * Results:
 *      Returns 1 if one of the options has changed, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
int 
Blt_OldConfigModified(Tk_ConfigSpec *specs, ...)
{
    va_list args;
    char *option;

    va_start(args, specs);
    while ((option = va_arg(args, char *)) != NULL) {
	Tk_ConfigSpec *specPtr;

	for (specPtr = specs; specPtr->type != TK_CONFIG_END; specPtr++) {
	    if ((Tcl_StringMatch(specPtr->argvName, option)) &&
		(specPtr->specFlags & TK_CONFIG_OPTION_SPECIFIED)) {
		va_end(args);
		return 1;
	    }
	}
    }
    va_end(args);
    return 0;
}


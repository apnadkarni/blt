/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltUtil.h --
 *
 *	Copyright 1993-2004 George A Howlett.
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

#ifndef _BLT_UTIL_H
#define _BLT_UTIL_H

#ifndef TCL_USE_STUBS
extern int TclpHasSockets(Tcl_Interp *interp);
#endif

BLT_EXTERN FILE *Blt_OpenFile(Tcl_Interp *interp, const char *fileName, 
	const char *mode);

BLT_EXTERN int Blt_ExprDoubleFromObj (Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *valuePtr);

BLT_EXTERN int Blt_ExprIntFromObj (Tcl_Interp *interp, Tcl_Obj *objPtr, 
	int *valuePtr);

BLT_EXTERN const char *Blt_Itoa(int value);

BLT_EXTERN const char *Blt_Ltoa(long value);

BLT_EXTERN const char *Blt_Utoa(unsigned int value);

BLT_EXTERN const char *Blt_Dtoa(Tcl_Interp *interp, double value);

BLT_EXTERN unsigned char *Blt_Base64_Decode(Tcl_Interp *interp, 
	const char *string, size_t *lengthPtr);

BLT_EXTERN const char *Blt_Base64_Encode(const unsigned char *buffer, 
        size_t bufsize);

BLT_EXTERN const char *Blt_Base85_Encode(const unsigned char *buffer, 
        size_t bufsize);

BLT_EXTERN const char *Blt_Base16_Encode(const unsigned char *buffer, 
        size_t bufsize);

BLT_EXTERN int Blt_IsBase64(const char *buf, size_t length);

BLT_EXTERN int Blt_GlobalEvalObjv(Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv);
BLT_EXTERN int Blt_GlobalEvalListObj(Tcl_Interp *interp, Tcl_Obj *cmdObjPtr);

BLT_EXTERN int Blt_GetDoubleFromString(Tcl_Interp *interp, const char *s, 
	double *valuePtr);
BLT_EXTERN int Blt_GetDoubleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *valuePtr);

BLT_EXTERN int Blt_GetTimeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *valuePtr);
BLT_EXTERN int Blt_GetTime(Tcl_Interp *interp, const char *string, 
	double *valuePtr);

BLT_EXTERN int Blt_GetDateFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	double *timePtr);
BLT_EXTERN int Blt_GetDate(Tcl_Interp *interp, const char *string, 
	double *timePtr);

#ifndef USE_TCL_STUBS
BLT_EXTERN int TclGetLong(Tcl_Interp *interp, const char *s, long *longPtr);
#endif	/*USE_TCL_STUBS*/

BLT_EXTERN int Blt_GetLong(Tcl_Interp *interp, const char *s, long *longPtr);
BLT_EXTERN int Blt_GetLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	long *longPtr);

#ifndef HAVE_SPRINTF_S
BLT_EXTERN int Blt_FormatString(char *s, size_t size, const char *fmt, /*args*/ ...);
#endif	/* HAVE_SPRINTF_S */

#endif /*BLT_UTIL_H*/

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltUtil.h --
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

BLT_EXTERN unsigned char *Blt_DecodeBase64(Tcl_Interp *interp, 
        const char *string, size_t *lengthPtr);

BLT_EXTERN const char *Blt_EncodeBase64(const unsigned char *buffer, 
        size_t bufsize);

BLT_EXTERN const char *Blt_EncodeBase85(const unsigned char *buffer, 
        size_t bufsize);

BLT_EXTERN const char *Blt_EncodeHexadecimal(const unsigned char *buffer, 
        size_t bufsize);

BLT_EXTERN int Blt_IsBase64(const char *buf, size_t length);

BLT_EXTERN int Blt_GlobalEvalObjv(Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv);
BLT_EXTERN int Blt_GlobalEvalListObj(Tcl_Interp *interp, Tcl_Obj *cmdObjPtr);

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
#endif  /*USE_TCL_STUBS*/

#ifndef HAVE_SPRINTF_S
BLT_EXTERN int Blt_FormatString(char *s, size_t size, const char *fmt, /*args*/ ...);
#endif  /* HAVE_SPRINTF_S */

#endif /*BLT_UTIL_H*/

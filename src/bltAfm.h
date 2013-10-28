/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltAfm.h --
 *
 *	Copyright 2012 George A Howlett.
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

#ifndef _BLT_AFM_H
#define _BLT_AFM_H

BLT_EXTERN void Blt_Afm_SetPrinting(Tcl_Interp *interp, int value);
BLT_EXTERN int Blt_Afm_IsPrinting(void);

BLT_EXTERN int Blt_Afm_TextWidth(Blt_Font font, const char *s, int numBytes);

BLT_EXTERN int Blt_Afm_GetMetrics(Blt_Font font, Blt_FontMetrics *fmPtr);

BLT_EXTERN const char *Blt_Afm_GetPostscriptFamily(const char *family);
BLT_EXTERN void Blt_Afm_GetPostscriptName(const char *family, int flags, 
	Tcl_DString *resultPtr);

#endif /* BLT_PS_H */

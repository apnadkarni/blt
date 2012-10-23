
/*
 * bltPictFmts.h --
 *
 * This module implements the various image format conversion routines for
 * picture in the BLT toolkit.
 *
 *	Copyright 2003-2004 George A Howlett.
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
 *
 * The JPEG reader/writer is adapted from jdatasrc.c and jdatadst.c
 * in the Independent JPEG Group (version 6b) library distribution.
 *
 *	The authors make NO WARRANTY or representation, either express
 *	or implied, with respect to this software, its quality,
 *	accuracy, merchantability, or fitness for a particular
 *	purpose.  This software is provided "AS IS", and you, its
 *	user, assume the entire risk as to its quality and accuracy.
 *
 *	This software is copyright (C) 1991-1998, Thomas G. Lane.  All
 *	Rights Reserved except as specified below.
 *
 *	Permission is hereby granted to use, copy, modify, and
 *	distribute this software (or portions thereof) for any
 *	purpose, without fee, subject to these conditions: (1) If any
 *	part of the source code for this software is distributed, then
 *	this README file must be included, with this copyright and
 *	no-warranty notice unaltered; and any additions, deletions, or
 *	changes to the original files must be clearly indicated in
 *	accompanying documentation.  (2) If only executable code is
 *	distributed, then the accompanying documentation must state
 *	that "this software is based in part on the work of the
 *	Independent JPEG Group".  (3) Permission for use of this
 *	software is granted only if the user accepts full
 *	responsibility for any undesirable consequences; the authors
 *	accept NO LIABILITY for damages of any kind.
 *
 * The GIF reader is from converters/other/giftopnm.c in the netpbm
 * (version 10.19) distribution.
 *
 *	Copyright 1990, 1991, 1993, David Koblas.  (koblas@netcom.com)
 *	Permission to use, copy, modify, and distribute this software
 *	and its documentation for any purpose and without fee is
 *	hereby granted, provided that the above copyright notice
 *	appear in all copies and that both that copyright notice and
 *	this permission notice appear in supporting documentation.
 *	This software is provided "as is" without express or implied
 *	warranty.
 *
 */

#ifndef _BLT_PIC_FMTS_H
#define _BLT_PIC_FMTS_H

#include <bltChain.h>

#define PIC_PROGRESSIVE	(1<<0)
#define PIC_NOQUANTIZE	(1<<1)

#define PIC_FMT_ISASCII	(1<<3)


typedef int (Blt_PictureIsFmtProc)(Blt_DBuffer buffer);

typedef Blt_Chain (Blt_PictureReadDataProc)(Tcl_Interp *interp, 
	const char *fileName, Blt_DBuffer buffer);

typedef Tcl_Obj *(Blt_PictureWriteDataProc)(Tcl_Interp *interp, 
	Blt_Picture picture);

typedef Blt_Chain (Blt_PictureImportProc)(Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv, const char **fileNamePtr);

typedef int (Blt_PictureExportProc)(Tcl_Interp *interp, unsigned int index,
	Blt_Chain chain, int objc, Tcl_Obj *const *objv);

BLT_EXTERN int Blt_PictureRegisterFormat(Tcl_Interp *interp, 
	const char *name, 
	Blt_PictureIsFmtProc *isFmtProc,
	Blt_PictureReadDataProc *readProc, 
	Blt_PictureWriteDataProc *writeProc,
	Blt_PictureImportProc *importProc, 
	Blt_PictureExportProc *exportProc);

BLT_EXTERN Blt_Picture Blt_GetNthPicture(Blt_Chain chain, size_t index);

typedef struct {
    const char *name;			/* Name of format. */
    unsigned int flags;
    Blt_PictureIsFmtProc *isFmtProc;
    Blt_PictureReadDataProc *readProc;	/* Used for -file and -data
					 * configuration options. */
    Blt_PictureWriteDataProc *writeProc; /* Used for cget -data. */
    Blt_PictureImportProc *importProc;
    Blt_PictureExportProc *exportProc;
} Blt_PictFormat;

BLT_EXTERN Blt_PictFormat *Blt_FindPictureFormat(Tcl_Interp *interp,
	const char *ext);

#endif /* _BLT_PIC_FMTS_H */

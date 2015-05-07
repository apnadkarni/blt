/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictFmts.h --
 *
 * This module implements the various image format conversion routines for
 * picture in the BLT toolkit.
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
 * The JPEG reader/writer is adapted from jdatasrc.c and jdatadst.c in the
 * Independent JPEG Group (version 6b) library distribution.
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.  All Rights Reserved except as
 * specified below.  
 *
 *    Permission is hereby granted to use, copy, modify, and distribute
 *    this software (or portions thereof) for any purpose, without fee,
 *    subject to these conditions: (1) If any part of the source code for
 *    this software is distributed, then this README file must be included,
 *    with this copyright and no-warranty notice unaltered; and any
 *    additions, deletions, or changes to the original files must be
 *    clearly indicated in accompanying documentation.  (2) If only
 *    executable code is distributed, then the accompanying documentation
 *    must state that "this software is based in part on the work of the
 *    Independent JPEG Group".  (3) Permission for use of this software is
 *    granted only if the user accepts full responsibility for any
 *    undesirable consequences; the authors accept NO LIABILITY for damages
 *    of any kind.
 *
 *    The authors make NO WARRANTY or representation, either express or
 *    implied, with respect to this software, its quality, accuracy,
 *    merchantability, or fitness for a particular purpose.  This software
 *    is provided "AS IS", and you, its user, assume the entire risk as to
 *    its quality and accuracy.
 *
 * The GIF reader is from converters/other/giftopnm.c in the netpbm
 * (version 10.19) distribution.
 *
 * Copyright 1990, 1991, 1993, David Koblas.  (koblas@netcom.com)
 *
 *   Permission to use, copy, modify, and distribute this software and its
 *   documentation for any purpose and without fee is hereby granted,
 *   provided that the above copyright notice appear in all copies and that
 *   both that copyright notice and this permission notice appear in
 *   supporting documentation.  This software is provided "as is" without
 *   express or implied warranty.
 *
 */

#ifndef _BLT_PIC_FMTS_H
#define _BLT_PIC_FMTS_H

#include <bltChain.h>

#define PIC_PROGRESSIVE (1<<0)
#define PIC_NOQUANTIZE  (1<<1)

#define PIC_FMT_ISASCII (1<<3)


typedef int (Blt_PictureIsFmtProc)(Blt_DBuffer buffer);

typedef Blt_Chain (Blt_PictureReadProc)(Tcl_Interp *interp, 
        const char *fileName, Blt_DBuffer buffer);

typedef Tcl_Obj *(Blt_PictureWriteProc)(Tcl_Interp *interp,
        Blt_Picture picture);

typedef Blt_Chain (Blt_PictureImportProc)(Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv, const char **fileNamePtr);

typedef int (Blt_PictureExportProc)(Tcl_Interp *interp, int index,
        Blt_Chain chain, int objc, Tcl_Obj *const *objv);

BLT_EXTERN int Blt_PictureRegisterFormat(Tcl_Interp *interp, 
        const char *name, 
        Blt_PictureIsFmtProc  *isFmtProc,
        Blt_PictureReadProc   *readProc, 
        Blt_PictureWriteProc  *writeProc,
        Blt_PictureImportProc *importProc, 
        Blt_PictureExportProc *exportProc);

BLT_EXTERN Blt_Picture Blt_GetNthPicture(Blt_Chain chain, size_t index);

typedef struct {
    const char *name;                   /* Name of format. */
    unsigned int flags;
    Blt_PictureIsFmtProc *isFmtProc;
    Blt_PictureReadProc *readProc;      /* Used for -file and -data
                                         * configuration options. */
    Blt_PictureWriteProc *writeProc;    /* Used for cget -data. */
    Blt_PictureImportProc *importProc;
    Blt_PictureExportProc *exportProc;
} Blt_PictFormat;

BLT_EXTERN Blt_PictFormat *Blt_FindPictureFormat(Tcl_Interp *interp,
        const char *ext);

#endif /* _BLT_PIC_FMTS_H */

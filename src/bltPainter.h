/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPainter.h --
 *
 * This module implements generic image painting procedures for
 * the BLT toolkit.
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
 * The color allocation routines are adapted from tkImgPhoto.c of the Tk
 * library distrubution.  The photo image type was designed and implemented
 * by Paul Mackerras.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 *
 * Copyright (c) 19941998 Sun Microsystems, Inc.
 * 
 */

#ifndef _BLT_PAINTER_H
#define _BLT_PAINTER_H

typedef struct _Blt_Painter *Blt_Painter;

BLT_EXTERN void Blt_FreePainter(Blt_Painter painter);

BLT_EXTERN Blt_Painter Blt_GetPainter(Tk_Window tkwin, float gamma);

BLT_EXTERN Blt_Painter Blt_GetPainterFromDrawable(Display *display, 
	Drawable drawable, float gamma);

BLT_EXTERN GC Blt_PainterGC(Blt_Painter painter);

BLT_EXTERN int Blt_PainterDepth(Blt_Painter painter);

BLT_EXTERN int Blt_PaintPicture(Blt_Painter painter, Drawable drawable, 
	Blt_Picture src, int srcX, int srcY, int width, int height, 
	int destX, int destY, unsigned int flags);

BLT_EXTERN int Blt_PaintPictureWithBlend(Blt_Painter painter, Drawable drawable, 
	Blt_Picture src, int srcX, int srcY, int width, int height, 
	int destX, int destY, unsigned int flags);

BLT_EXTERN Blt_Picture Blt_PaintCheckbox(int width, int height, 
	XColor *fillColor, XColor *outlineColor, XColor *checkColor, int isOn);

BLT_EXTERN Blt_Picture Blt_PaintRadioButton(int width, int height, Blt_Bg bg, 
	XColor *fill, XColor *outline, int isOn);
BLT_EXTERN Blt_Picture Blt_PaintRadioButtonOld(int width, int height, 
	XColor *bg, XColor *fill, XColor *outline, XColor *check, int isOn);

BLT_EXTERN Blt_Picture Blt_PaintDelete(int width, int height, XColor *bgColor,
	XColor *fillColor, XColor *symColor, int isActive);

#endif /* _BLT_PAINTER_H */

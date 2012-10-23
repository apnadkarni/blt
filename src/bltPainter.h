
/*
 * bltPainter.h --
 *
 * This module implements generic image painting procedures for
 * the BLT toolkit.
 *
 *	Copyright 1998-2004 George A Howlett.
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
 *
 * The color allocation routines are adapted from tkImgPhoto.c of the
 * Tk library distrubution.  The photo image type was designed and
 * implemented by Paul Mackerras.
 *
 *	Copyright (c) 1987-1993 The Regents of the University of
 *	California.
 *
 *	Copyright (c) 19941998 Sun Microsystems, Inc.
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

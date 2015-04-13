/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPalette.h --
 *
 *	Copyright 2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use, copy,
 *	modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */

#ifndef _BLT_PALETTE_H
#define _BLT_PALETTE_H

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Palette --
 *
 *      Represents a gradient color palette.  The color palette is made up
 *	of an array of palette entries.  There can also be an array opacity
 *	entries.
 *
 *---------------------------------------------------------------------------
 */
typedef struct _Blt_Palette *Blt_Palette;

typedef void (Blt_Palette_NotifyProc) (Blt_Palette palette, 
	ClientData clientData, unsigned int flags);

#define PALETTE_CHANGE_NOTIFY	(1<<0)
#define PALETTE_DELETE_NOTIFY	(1<<1)

BLT_EXTERN int Blt_Palette_GetFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
	Blt_Palette *palPtr);
BLT_EXTERN int Blt_Palette_GetFromString(Tcl_Interp *interp, const char *string,
	Blt_Palette *palPtr);
BLT_EXTERN int Blt_Palette_GetAssociatedColor(Blt_Palette palette, double val);
BLT_EXTERN void Blt_Palette_CreateNotifier(Blt_Palette palette, 
	Blt_Palette_NotifyProc *proc, ClientData clientData);
BLT_EXTERN void Blt_Palette_DeleteNotifier(Blt_Palette palette, 
	ClientData clientData);
BLT_EXTERN const char *Blt_Palette_Name(Blt_Palette palette);
BLT_EXTERN Blt_Palette Blt_Palette_TwoColorPalette(int low, int high);
BLT_EXTERN void Blt_Palette_Free(Blt_Palette palette);

BLT_EXTERN int Blt_Palette_GetRGBColor(Blt_Palette palette, double value);

#endif /*_BLT_PALETTE_H*/

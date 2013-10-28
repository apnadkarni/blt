/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltBg.h --
 *
 *	Copyright 1995-2004 George A Howlett.
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

#ifndef _BLT_BG_H
#define _BLT_BG_H

typedef struct _Blt_Bg *Blt_Bg;

typedef void Blt_Bg_ChangedProc(ClientData clientData);

BLT_EXTERN Blt_Bg Blt_GetBg(Tcl_Interp *interp, Tk_Window tkwin,
	const char *styleName);

BLT_EXTERN Blt_Bg Blt_GetBgFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
	Tcl_Obj *objPtr);

BLT_EXTERN XColor *Blt_Bg_BorderColor(Blt_Bg bg);

BLT_EXTERN Tk_3DBorder Blt_Bg_Border(Blt_Bg bg);

BLT_EXTERN Blt_Paintbrush *Blt_Bg_Paintbrush(Blt_Bg bg);

BLT_EXTERN const char *Blt_Bg_Name(Blt_Bg bg);

BLT_EXTERN void Blt_FreeBg(Blt_Bg bg);

BLT_EXTERN void Blt_Bg_DrawRectangle(Tk_Window tkwin, Drawable drawable,
	Blt_Bg bg, int x, int y, int width, int height, int borderWidth,
	int relief);

BLT_EXTERN void Blt_Bg_FillRectangle(Tk_Window tkwin, Drawable drawable,
	Blt_Bg bg, int x, int y, int width, int height, int borderWidth, 
	int relief);

BLT_EXTERN void Blt_Bg_DrawPolygon(Tk_Window tkwin, Drawable drawable, 
	Blt_Bg bg, XPoint *points, int numPoints, int borderWidth, 
	int leftRelief);

BLT_EXTERN void Blt_Bg_FillPolygon(Tk_Window tkwin, Drawable drawable, 
	Blt_Bg bg, XPoint *points, int numPoints, int borderWidth, 
	int leftRelief);

BLT_EXTERN void Blt_Bg_GetOrigin(Blt_Bg bg, int *xPtr, int *yPtr);

BLT_EXTERN void Blt_Bg_SetChangedProc(Blt_Bg bg, Blt_Bg_ChangedProc *notifyProc,
	ClientData clientData);

BLT_EXTERN void Blt_Bg_SetOrigin(Tk_Window tkwin, Blt_Bg bg, int x, int y);

BLT_EXTERN void Blt_Bg_DrawFocus(Tk_Window tkwin, Blt_Bg bg, 
	int highlightWidth, Drawable drawable);

BLT_EXTERN GC Blt_Bg_BorderGC(Tk_Window tkwin, Blt_Bg bg, int which);

BLT_EXTERN void Blt_Bg_SetFromBackground(Tk_Window tkwin, Blt_Bg bg);

BLT_EXTERN void Blt_Bg_UnsetClipRegion(Tk_Window tkwin, Blt_Bg bg);

BLT_EXTERN void Blt_Bg_SetClipRegion(Tk_Window tkwin, Blt_Bg bg, TkRegion rgn);

#endif /* BLT_BG_H */

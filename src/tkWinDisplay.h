
/*
 * tkWinDisplay.h --
 *
 *	Copyright 2003-2004 George A Howlett.
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
 * This file contains excerpts from tkInt.h of the Tk library distribution.
 *
 *	Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 *	See the file "license.terms" for information on usage and
 *	redistribution of this file, and for a DISCLAIMER OF ALL
 *	WARRANTIES.
 *
 */

#ifndef _TK_WIN_DISPLAY_H
#define _TK_WIN_DISPLAY_H

/*
 * The TkWinDrawable is the internal implementation of an X Drawable
 * (either a Window or a Pixmap).  The following constants define the
 * valid Drawable types.
 */

#define TWD_BITMAP	1
#define TWD_WINDOW	2
#define TWD_WINDC	3

typedef struct {
    int type;
    HWND handle;
    TkWindow *winPtr;
} TkWinWindow;

typedef struct {
    int type;
    HBITMAP handle;
    Colormap colormap;
    int depth;
} TkWinBitmap;

typedef struct {
    int type;
    HDC hdc;
} TkWinDC;

typedef union {
    int type;
    TkWinWindow window;
    TkWinBitmap bitmap;
    TkWinDC winDC;
} TkWinDrawable;

/*
 * The TkWinDCState is used to save the state of a device context
 * so that it can be restored later.
 */
typedef struct {
    HPALETTE palette;
    int bkmode;			/* This field was added in Tk
				 * 8.3.1. Be careful that you don't 
				 * use this structure in a context
				 * where its size is important.  */
} TkWinDCState;

#ifdef USE_TK_STUBS
#include "tkIntPlatDecls.h"
#else 
extern HDC TkWinGetDrawableDC(Display *display, Drawable drawable, 
	TkWinDCState *state);

extern HDC TkWinReleaseDrawableDC(Drawable drawable, HDC dc, 
	TkWinDCState *state);

extern HINSTANCE Tk_GetHINSTANCE(void);

extern HWND Tk_GetHWND(Window window);

extern Window Tk_AttachHWND(Tk_Window tkwin, HWND hWnd);

#endif /* USE_TK_STUBS */
#endif /* _TK_WIN_DISPLAY_H */


/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * tkWinDisplay.h --
 *
 * This file contains excerpts from tkInt.h of the Tk library distribution.
 *
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 *   See the file "license.terms" for information on usage and
 *   redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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


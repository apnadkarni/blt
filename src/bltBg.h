/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltBg.h --
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

#ifndef _BLT_BG_H
#define _BLT_BG_H

typedef struct _Blt_Bg *Blt_Bg;

typedef void Blt_BackgroundChangedProc(ClientData clientData);

BLT_EXTERN int Blt_GetBg(Tcl_Interp *interp, Tk_Window tkwin,
        const char *bgName, Blt_Bg *bgPtr);

BLT_EXTERN int Blt_GetBgFromObj(Tcl_Interp *interp, Tk_Window tkwin, 
        Tcl_Obj *objPtr, Blt_Bg *bgPtr);

BLT_EXTERN XColor *Blt_Bg_BorderColor(Blt_Bg bg);

BLT_EXTERN Tk_3DBorder Blt_Bg_Border(Blt_Bg bg);

BLT_EXTERN Blt_PaintBrush Blt_Bg_PaintBrush(Blt_Bg bg);

BLT_EXTERN const char *Blt_Bg_Name(Blt_Bg bg);

BLT_EXTERN void Blt_Bg_Free(Blt_Bg bg);

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

BLT_EXTERN void Blt_Bg_SetChangedProc(Blt_Bg bg, Blt_BackgroundChangedProc *notifyProc,
        ClientData clientData);

BLT_EXTERN void Blt_Bg_SetOrigin(Tk_Window tkwin, Blt_Bg bg, int x, int y);

BLT_EXTERN void Blt_Bg_DrawFocus(Tk_Window tkwin, Blt_Bg bg, 
        int highlightWidth, Drawable drawable);

BLT_EXTERN GC Blt_Bg_BorderGC(Tk_Window tkwin, Blt_Bg bg, int which);

BLT_EXTERN void Blt_Bg_SetFromBackground(Tk_Window tkwin, Blt_Bg bg);

BLT_EXTERN void Blt_Bg_UnsetClipRegion(Tk_Window tkwin, Blt_Bg bg);

BLT_EXTERN void Blt_Bg_SetClipRegion(Tk_Window tkwin, Blt_Bg bg, TkRegion rgn);

BLT_EXTERN unsigned int Blt_Bg_GetColor(Blt_Bg bg);

BLT_EXTERN void Blt_3DBorder_SetClipRegion(Tk_Window tkwin, Tk_3DBorder border,
        TkRegion rgn);
BLT_EXTERN void Blt_3DBorder_UnsetClipRegion(Tk_Window tkwin,
        Tk_3DBorder border);

#endif /* BLT_BG_H */

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTkWin.h --
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

#ifndef _BLT_TK_WIN_H
#define _BLT_TK_WIN_H

#define _CRT_SECURE_NO_DEPRECATE

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef STRICT
#undef WIN32_LEAN_AND_MEAN
#include <windowsx.h>

#undef STD_NORMAL_BACKGROUND
#undef STD_NORMAL_FOREGROUND
#undef STD_SELECT_BACKGROUND
#undef STD_SELECT_FOREGROUND
#undef STD_TEXT_FOREGROUND
#undef STD_FONT
#undef STD_FONT_LARGE
#undef STD_FONT_SMALL

#define STD_NORMAL_BACKGROUND   "SystemButtonFace"
#define STD_NORMAL_FOREGROUND   "SystemButtonText"
#define STD_SELECT_BACKGROUND   "SystemHighlight"
#define STD_SELECT_FOREGROUND   "SystemHighlightText"
#define STD_TEXT_FOREGROUND     "SystemWindowText"
#define STD_FONT                "Arial 8"
#define STD_FONT_LARGE          "Arial 12"
#define STD_FONT_SMALL          "Arial 6"

/* DOS Encapsulated PostScript File Header */
#pragma pack(2)
typedef struct {
    BYTE magic[4];                      /* Magic number for a DOS EPS file
                                         * C5,D0,D3,C6 */
    DWORD psStart;                      /* Offset of PostScript section. */
    DWORD psLength;                     /* Length of the PostScript
                                         * section. */
    DWORD wmfStart;                     /* Offset of Windows Meta File
                                         * section. */
    DWORD wmfLength;                    /* Length of Meta file section. */
    DWORD tiffStart;                    /* Offset of TIFF section. */
    DWORD tiffLength;                   /* Length of TIFF section. */
    WORD checksum;                      /* Checksum of header. If FFFF,
                                         * ignore. */
} DOSEPSHEADER;

#pragma pack()

/* Aldus Portable Metafile Header */
#pragma pack(2)
typedef struct {
    DWORD key;                          /* Type of metafile */
    WORD hmf;                           /* Unused. Must be NULL. */
    SMALL_RECT bbox;                    /* Bounding rectangle */
    WORD inch;                          /* Units per inch. */
    DWORD reserved;                     /* Unused. */
    WORD checksum;                      /* XOR of previous fields (10
                                         * 32-bit words). */
} APMHEADER;
#pragma pack()

#ifdef __GNUC__ 
#include <wingdi.h>
#include <windowsx.h>
#undef Status
#include <winspool.h>
#define Status int
/*
 * Add definitions missing from windgi.h, windowsx.h, and winspool.h
 */
#include <missing.h> 
#endif /* __GNUC__ */

#undef XCopyArea                
#define XCopyArea               Blt_EmulateXCopyArea
#undef XCopyPlane               
#define XCopyPlane              Blt_EmulateXCopyPlane
#undef XDrawArcs                
#define XDrawArcs               Blt_EmulateXDrawArcs
#undef XDrawLine                
#define XDrawLine               Blt_EmulateXDrawLine
#undef XDrawLines               
#define XDrawLines              Blt_EmulateXDrawLines
#undef XDrawPoints              
#define XDrawPoints             Blt_EmulateXDrawPoints
#undef XDrawRectangle           
#define XDrawRectangle          Blt_EmulateXDrawRectangle
#undef XDrawRectangles          
#define XDrawRectangles         Blt_EmulateXDrawRectangles
#undef XDrawSegments            
#define XDrawSegments           Blt_EmulateXDrawSegments
#undef XDrawString              
#define XDrawString             Blt_EmulateXDrawString
#undef XFillArcs                
#define XFillArcs               Blt_EmulateXFillArcs
#undef XFillPolygon             
#define XFillPolygon            Blt_EmulateXFillPolygon
#undef XFillRectangle           
#define XFillRectangle          Blt_EmulateXFillRectangle
#undef XFillRectangle           
#define XFillRectangle          Blt_EmulateXFillRectangle
#undef XFillRectangles          
#define XFillRectangles         Blt_EmulateXFillRectangles
#undef XFree                    
#define XFree                   Blt_EmulateXFree
#undef XGetWindowAttributes     
#define XGetWindowAttributes    Blt_EmulateXGetWindowAttributes
#undef XLowerWindow             
#define XLowerWindow            Blt_EmulateXLowerWindow
#undef XMaxRequestSize          
#define XMaxRequestSize         Blt_EmulateXMaxRequestSize
#undef XPolygonRegion             
#define XPolygonRegion          Blt_EmulateXPolygonRegion
#undef XRaiseWindow             
#define XRaiseWindow            Blt_EmulateXRaiseWindow
#undef XReparentWindow          
#define XReparentWindow         Blt_EmulateXReparentWindow
#undef XSetDashes               
#define XSetDashes              Blt_EmulateXSetDashes
#undef XUnmapWindow             
#define XUnmapWindow            Blt_EmulateXUnmapWindow
#undef XWarpPointer             
#define XWarpPointer            Blt_EmulateXWarpPointer

extern GC Blt_EmulateXCreateGC(Display *display, Drawable drawable,
    unsigned long mask, XGCValues *valuesPtr);
extern void Blt_EmulateXCopyArea(Display *display, Drawable src, Drawable dest,
    GC gc, int src_x, int src_y, unsigned int width, unsigned int height,
    int dest_x, int dest_y);
extern void Blt_EmulateXCopyPlane(Display *display, Drawable src,
    Drawable dest, GC gc, int src_x, int src_y, unsigned int width,
    unsigned int height, int dest_x, int dest_y, unsigned long plane);
extern void Blt_EmulateXDrawArcs(Display *display, Drawable drawable, GC gc,
    XArc *arcs, int numArcs);
extern void Blt_EmulateXDrawLine(Display *display, Drawable drawable, GC gc,
    int x1, int y1, int x2, int y2);
extern void Blt_EmulateXDrawLines(Display *display, Drawable drawable, 
        GC gc, XPoint *points, int numPoints, int mode);
extern void Blt_EmulateXDrawPoints(Display *display, Drawable drawable, 
        GC gc, XPoint *points, int numPoints, int mode);
extern void Blt_EmulateXDrawRectangle(Display *display, Drawable drawable,
    GC gc, int x, int y, unsigned int width, unsigned int height);
extern void Blt_EmulateXDrawRectangles(Display *display, Drawable drawable,
    GC gc, XRectangle *rects, int numRects);
extern void Blt_EmulateXDrawSegments(Display *display, Drawable drawable,
    GC gc, XSegment *segments, int numSegments);
extern void Blt_EmulateXDrawSegments(Display *display, Drawable drawable,
    GC gc, XSegment *segments, int numSegments);
extern void Blt_EmulateXDrawString(Display *display, Drawable drawable, 
        GC gc, int x, int y, _Xconst char *string, int length);
extern void Blt_EmulateXFillArcs(Display *display, Drawable drawable, GC gc,
    XArc *arcs, int numArcs);
extern void Blt_EmulateXFillPolygon(Display *display, Drawable drawable,
    GC gc, XPoint *points, int numPoints,  int shape, int mode);
extern void Blt_EmulateXFillRectangle(Display *display, Drawable drawable,
    GC gc, int x, int y, unsigned int width, unsigned int height);
extern void Blt_EmulateXFillRectangles(Display *display, Drawable drawable,
    GC gc, XRectangle *rectangles, int numRects);
extern void Blt_EmulateXFree(void *ptr);
extern int Blt_EmulateXGetWindowAttributes(Display *display, Window window,
    XWindowAttributes * attrsPtr);
extern void Blt_EmulateXPolygonRegion(Display *display, XPoint *points, int numPoints,  int mode);
extern void Blt_EmulateXLowerWindow(Display *display, Window window);
extern void Blt_EmulateXMapWindow(Display *display, Window window);
extern long Blt_EmulateXMaxRequestSize(Display *display);
extern void Blt_EmulateXRaiseWindow(Display *display, Window window);
extern void Blt_EmulateXReparentWindow(Display *display, Window window,
    Window parent, int x, int y);
extern void Blt_EmulateXSetDashes(Display *display, GC gc, int dashOffset,
    _Xconst char *dashList, int n);
extern void Blt_EmulateXUnmapWindow(Display *display, Window window);
extern void Blt_EmulateXWarpPointer(Display *display, Window srcWindow,
    Window destWindow, int srcX, int srcY, unsigned int srcWidth,
    unsigned int srcHeight, int destX, int destY);

BLT_EXTERN void Blt_DrawLine2D(Display *display, Drawable drawable, GC gc,
    POINT *points, int numPoints);

BLT_EXTERN unsigned char *Blt_GetBitmapData(Display *display, Pixmap bitmap, 
        int width, int height, int *pitchPtr);

extern HPALETTE Blt_GetSystemPalette(void);

BLT_EXTERN HPEN Blt_GCToPen(HDC dc, GC gc);
BLT_EXTERN void Blt_TextOut(HDC dc, GC gc, HFONT hfont, const char *text, 
        int length, int x, int y);

BLT_EXTERN const char *Blt_PrintError(int error);
BLT_EXTERN int Blt_GetOpenPrinter(Tcl_Interp *interp, const char *id,
    Drawable *drawablePtr);
BLT_EXTERN int Blt_PrintDialog(Tcl_Interp *interp, Drawable *drawablePtr);
BLT_EXTERN int Blt_OpenPrinterDoc(Tcl_Interp *interp, const char *id);
BLT_EXTERN int Blt_ClosePrinterDoc(Tcl_Interp *interp, const char *id);
BLT_EXTERN void Blt_GetPrinterScale(HDC dc, double *xRatio, double *yRatio);
BLT_EXTERN int Blt_StartPrintJob(Tcl_Interp *interp, Drawable drawable);
BLT_EXTERN int Blt_EndPrintJob(Tcl_Interp *interp, Drawable drawable);

typedef int (Blt_DrawCmdProc)(ClientData clientData, int width, int height,
        Drawable drawable);
BLT_EXTERN int Blt_DrawToMetaFile(Tcl_Interp *interp, Tk_Window tkwin, int emf,
        const char *fileName, Blt_DrawCmdProc *proc, ClientData clientData,
        int w, int h);

#endif /*_BLT_TK_WIN_H*/

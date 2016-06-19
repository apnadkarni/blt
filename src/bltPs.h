/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPs.h --
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

#ifndef _BLT_PS_H
#define _BLT_PS_H

/*
 * PageSetup --
 *
 *      Structure contains information specific to the layout of the page for
 *      printing the graph.
 *
 */
typedef struct  {
    /* User configurable fields */
    int reqWidth, reqHeight;            /* If greater than zero, represents
                                         * the requested dimensions of the
                                         * printed graph */
    int reqPaperWidth;
    int reqPaperHeight;                 /* Requested dimensions for the
                                         * PostScript page. Can constrain
                                         * the size of the graph if the
                                         * graph (plus padding) is larger
                                         * than the size of the page. */
    Blt_Pad padX, padY;                 /* Requested padding on the
                                         * exterior of the graph. This
                                         * forms the bounding box for the
                                         * page. */
    const char *colorVarName;           /* If non-NULL, is the name of a
                                         * TCL array variable containing X
                                         * to output device color
                                         * translations */
    const char *fontVarName;            /* If non-NULL, is the name of a
                                         * TCL array variable containing X
                                         * to output device font
                                         * translations */
    int level;                          /* PostScript Language level 1-3 */
    unsigned int flags;
    Tcl_Obj *cmtsObjPtr;                /* User supplied comments to be
                                         * added. */

    /* Computed fields */
    short int left, bottom;             /* Bounding box of the plot in the
                                         * page. */
    short int right, top;
    float scale;                        /* Scale of page. Set if "-maxpect"
                                         * option is set, otherwise 1.0. */

    int paperHeight;
    int paperWidth;
    
} PageSetup;

#define PS_GREYSCALE    (1<<0)
#define PS_LANDSCAPE    (1<<2)
#define PS_CENTER       (1<<3)
#define PS_MAXPECT      (1<<4)
#define PS_DECORATIONS  (1<<5)
#define PS_FOOTER       (1<<6)
#define PS_FMT_NONE     0
#define PS_FMT_MASK     (PS_FMT_WMF|PS_FMT_EPSI|PS_FMT_TIFF)
#define PS_FMT_WMF      (1<<8)
#define PS_FMT_EPSI     (1<<9)
#define PS_FMT_TIFF     (1<<10)

typedef struct _Blt_Ps *Blt_Ps;

BLT_EXTERN Blt_Ps Blt_Ps_Create(Tcl_Interp *interp, PageSetup *setupPtr);

BLT_EXTERN void Blt_Ps_Free(Blt_Ps ps);

BLT_EXTERN const char *Blt_Ps_GetValue(Blt_Ps ps, int *lengthPtr);

BLT_EXTERN Blt_DBuffer Blt_Ps_GetDBuffer(Blt_Ps ps);

BLT_EXTERN Tcl_Interp *Blt_Ps_GetInterp(Blt_Ps ps);

BLT_EXTERN char *Blt_Ps_GetScratchBuffer(Blt_Ps ps);

BLT_EXTERN void Blt_Ps_SetInterp(Blt_Ps ps, Tcl_Interp *interp);

BLT_EXTERN void Blt_Ps_Append(Blt_Ps ps, const char *string);

BLT_EXTERN void Blt_Ps_AppendBytes(Blt_Ps ps, const char *string, int numBytes);

BLT_EXTERN void Blt_Ps_VarAppend(Blt_Ps ps, ...);

BLT_EXTERN void Blt_Ps_Format(Blt_Ps ps, const char *fmt, ...);

BLT_EXTERN void Blt_Ps_SetClearBackground(Blt_Ps ps);

BLT_EXTERN int Blt_Ps_IncludeFile(Tcl_Interp *interp, Blt_Ps ps, 
        const char *fileName);

BLT_EXTERN int Blt_Ps_GetPicaFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        int *picaPtr);

BLT_EXTERN int Blt_Ps_GetPadFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
        Blt_Pad *padPtr);

BLT_EXTERN int Blt_Ps_ComputeBoundingBox(PageSetup *setupPtr, int w, int h);

#ifdef _BLT_PICTURE_H
BLT_EXTERN void Blt_Ps_DrawPicture(Blt_Ps ps, Blt_Picture picture, 
        double x, double y);
#endif

BLT_EXTERN void Blt_Ps_Rectangle(Blt_Ps ps, int x, int y, int w, int h);

BLT_EXTERN void Blt_Ps_Rectangle2(Blt_Ps ps, double x1, double y1, double x2,
        double y2);


BLT_EXTERN int Blt_Ps_SaveFile(Tcl_Interp *interp, Blt_Ps ps, 
        const char *fileName);

#ifdef _TK

#include "bltFont.h"
#include "bltText.h"

BLT_EXTERN void Blt_Ps_XSetLineWidth(Blt_Ps ps, int lineWidth);

BLT_EXTERN void Blt_Ps_XSetBackground(Blt_Ps ps, XColor *colorPtr);

BLT_EXTERN void Blt_Ps_XSetBitmapData(Blt_Ps ps, Display *display, 
        Pixmap bitmap, int width, int height);

BLT_EXTERN void Blt_Ps_XSetForeground(Blt_Ps ps, XColor *colorPtr);

BLT_EXTERN void Blt_Ps_XSetFont(Blt_Ps ps, Blt_Font font);

BLT_EXTERN void Blt_Ps_XSetDashes(Blt_Ps ps, Blt_Dashes *dashesPtr);

BLT_EXTERN void Blt_Ps_XSetLineAttributes(Blt_Ps ps, XColor *colorPtr,
        int lineWidth, Blt_Dashes *dashesPtr, int capStyle, int joinStyle);

BLT_EXTERN void Blt_Ps_XSetStipple(Blt_Ps ps, Display *display, Pixmap bitmap);

BLT_EXTERN void Blt_Ps_Polyline(Blt_Ps ps, int n, Point2d *points);

BLT_EXTERN void Blt_Ps_XDrawLines(Blt_Ps ps, int n, XPoint *points);

BLT_EXTERN void Blt_Ps_XDrawSegments(Blt_Ps ps, int n, XSegment *segments);

BLT_EXTERN void Blt_Ps_DrawPolyline(Blt_Ps ps, int n, Point2d *points);

BLT_EXTERN void Blt_Ps_DrawSegments2d(Blt_Ps ps, int n, Segment2d *segments);

BLT_EXTERN void Blt_Ps_Draw3DRectangle(Blt_Ps ps, Tk_3DBorder border, 
        double x, double y, int width, int height, int borderWidth, int relief);

BLT_EXTERN void Blt_Ps_Fill3DRectangle(Blt_Ps ps, Tk_3DBorder border, double x,
         double y, int width, int height, int borderWidth, int relief);

BLT_EXTERN void Blt_Ps_XFillRectangle(Blt_Ps ps, double x, double y, 
        int width, int height);

BLT_EXTERN void Blt_Ps_XFillRectangles(Blt_Ps ps, int n, XRectangle *rects);

BLT_EXTERN void Blt_Ps_XFillPolygon(Blt_Ps ps, int n, Point2d *points);

BLT_EXTERN void Blt_Ps_DrawPhoto(Blt_Ps ps, Tk_PhotoHandle photoToken,
        double x, double y);

BLT_EXTERN void Blt_Ps_XDrawWindow(Blt_Ps ps, Tk_Window tkwin, 
        double x, double y);

BLT_EXTERN void Blt_Ps_DrawText(Blt_Ps ps, const char *string, 
        TextStyle *attrPtr, double x, double y);

BLT_EXTERN void Blt_Ps_DrawBitmap(Blt_Ps ps, Display *display, Pixmap bitmap, 
        double scaleX, double scaleY);

BLT_EXTERN void Blt_Ps_XSetCapStyle(Blt_Ps ps, int capStyle);

BLT_EXTERN void Blt_Ps_XSetJoinStyle(Blt_Ps ps, int joinStyle);

BLT_EXTERN void Blt_Ps_PolylineFromXPoints(Blt_Ps ps, int n, XPoint *points);

BLT_EXTERN void Blt_Ps_Polygon(Blt_Ps ps, Point2d *points, int numPoints);

BLT_EXTERN void Blt_Ps_SetPrinting(Blt_Ps ps, int value);

#endif /* _TK */

#endif /* BLT_PS_H */

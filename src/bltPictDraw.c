/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictDraw.c --
 *
 * This module implements image drawing primitives (line, circle, rectangle,
 * text, etc.) for picture images in the BLT toolkit.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"
#ifdef HAVE_STRING_H
  #include <string.h>
#endif

#ifdef HAVE_STDLIB_H
  #include <stdlib.h>
#endif

#ifdef HAVE_LIBXFT
  #include <ft2build.h>
  #include FT_FREETYPE_H
  #ifndef TT_CONFIG_OPTION_SUBPIXEL_HINTING
    #define TT_CONFIG_OPTION_SUBPIXEL_HINTING 0
  #endif 
  #include <X11/Xft/Xft.h>
#endif  /* HAVE_LIBXFT */

#include <X11/Xutil.h>
#include "bltAlloc.h"
#include "bltMath.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltPicture.h"
#include "bltPictInt.h"
#include "bltBg.h"
#include "bltPainter.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltSwitch.h"
#include "bltOp.h"
#include "tkIntBorder.h"

#define imul8x8(a,b,t)  ((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))
#define CLAMP(c)        ((((c) < 0.0) ? 0.0 : ((c) > 255.0) ? 255.0 : (c)))

typedef struct {
    size_t numValues;
    void *values;
} Array;

static Blt_SwitchParseProc ArraySwitchProc;
static Blt_SwitchFreeProc ArrayFreeProc;
static Blt_SwitchCustom arraySwitch = {
    ArraySwitchProc, NULL, ArrayFreeProc, (ClientData)0
};

static Blt_SwitchParseProc ShadowSwitchProc;
static Blt_SwitchCustom shadowSwitch = {
    ShadowSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ColorSwitchProc;
static Blt_SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToPaintBrushProc;
static Blt_SwitchFreeProc FreePaintBrushProc;
static Blt_SwitchCustom paintbrushSwitch =
{
    ObjToPaintBrushProc, NULL, FreePaintBrushProc, (ClientData)0,
};

typedef struct {
    Blt_PaintBrush brush;               /* Outline and fill colors for the
                                         * circle. */
    Blt_Shadow shadow;
    int antialiased;
    double lineWidth;                   /* Width of outline.  If zero,
                                         * indicates to draw a solid
                                         * circle. */
    int blend;
} CircleSwitches;

static Blt_SwitchSpec circleSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-color", "color", (char *)NULL,
        Blt_Offset(CircleSwitches, brush),    0, 0, &paintbrushSwitch},
    {BLT_SWITCH_BOOLEAN, "-antialiased", "bool", (char *)NULL,
        Blt_Offset(CircleSwitches, antialiased), 0},
    {BLT_SWITCH_BOOLEAN, "-blend", "bool", (char *)NULL,
        Blt_Offset(CircleSwitches, blend), 0},
    {BLT_SWITCH_DOUBLE, "-linewidth", "value", (char *)NULL,
        Blt_Offset(CircleSwitches, lineWidth), 0},
    {BLT_SWITCH_CUSTOM, "-shadow", "offset", (char *)NULL,
        Blt_Offset(CircleSwitches, shadow), 0, 0, &shadowSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_PaintBrush brush;               /* Outline and fill colors for the
                                         * circle. */
    Blt_Pixel fill;                     /* Fill color of circle. */
    Blt_Pixel outline;                  /* Outline color of circle. */
    Blt_Shadow shadow;
    int antialiased;
    double lineWidth;                   /* Line width of outline.  If zero,
                                         * indicates to draw a solid
                                         * circle. */
    int blend;
} CircleSwitches2;

typedef struct {
    Blt_Pixel fill, outline;            /* Outline and fill colors for the
                                         * circle. */
    Blt_Shadow shadow;
    int antialiased;
    int lineWidth;                      /* Width of outline.  If zero,
                                         * indicates to draw a solid
                                         * circle. */
    int blend;
} EllipseSwitches;

static Blt_SwitchSpec ellipseSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-fill", "color", (char *)NULL,
        Blt_Offset(EllipseSwitches, fill),    0, 0, &colorSwitch},
    {BLT_SWITCH_CUSTOM, "-outline", "color", (char *)NULL,
        Blt_Offset(EllipseSwitches, outline),    0, 0, &colorSwitch},
    {BLT_SWITCH_BOOLEAN, "-antialiased", "bool", (char *)NULL,
        Blt_Offset(EllipseSwitches, antialiased), 0},
    {BLT_SWITCH_BOOLEAN, "-blend", "bool", (char *)NULL,
        Blt_Offset(EllipseSwitches, blend), 0},
    {BLT_SWITCH_INT_NNEG, "-linewidth", "value", (char *)NULL,
        Blt_Offset(EllipseSwitches, lineWidth), 0},
    {BLT_SWITCH_CUSTOM, "-shadow", "offset", (char *)NULL,
        Blt_Offset(EllipseSwitches, shadow), 0, 0, &shadowSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_Pixel bg;                       /* Color of line. */
    int lineWidth;                      /* Width of outline. */
    Array x, y;
    Array coords;
} LineSwitches;

typedef struct {
    Blt_PaintBrush brush;               /* Fill color of polygon. */
    int antialiased;
    Blt_Shadow shadow;
    int lineWidth;                      /* Width of outline. Default is 1,
                                         * If zero, indicates to draw a
                                         * solid polygon. */
    Array coords;
    Array x, y;

} PolygonSwitches;

static Blt_SwitchSpec polygonSwitches[] = 
{
    {BLT_SWITCH_BOOLEAN, "-antialiased", "bool", (char *)NULL,
        Blt_Offset(PolygonSwitches, antialiased), 0},
    {BLT_SWITCH_CUSTOM, "-color", "color", (char *)NULL,
        Blt_Offset(PolygonSwitches, brush),    0, 0, &paintbrushSwitch},
    {BLT_SWITCH_CUSTOM, "-coords", "{x0 y0 x1 y1 ... xn yn}", (char *)NULL,
        Blt_Offset(PolygonSwitches, coords), 0, 0, &arraySwitch},
    {BLT_SWITCH_CUSTOM, "-x", "{x0 x1 ... xn}", (char *)NULL,
        Blt_Offset(PolygonSwitches, x), 0, 0, &arraySwitch},
    {BLT_SWITCH_CUSTOM, "-y", "{x0 x1 ... xn}", (char *)NULL,
        Blt_Offset(PolygonSwitches, y), 0, 0, &arraySwitch},
    {BLT_SWITCH_CUSTOM, "-shadow", "offset", (char *)NULL,
        Blt_Offset(PolygonSwitches, shadow), 0, 0, &shadowSwitch},
    {BLT_SWITCH_INT_POS, "-linewidth", "int", (char *)NULL,
        Blt_Offset(PolygonSwitches, lineWidth), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec lineSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-color", "color", (char *)NULL,
        Blt_Offset(LineSwitches, bg),    0, 0, &colorSwitch},
    {BLT_SWITCH_CUSTOM, "-coords", "{x0 y0 x1 y1 ... xn yn}", (char *)NULL,
        Blt_Offset(LineSwitches, coords), 0, 0, &arraySwitch},
    {BLT_SWITCH_INT_POS, "-linewidth", "int", (char *)NULL,
        Blt_Offset(LineSwitches, lineWidth), 0},
    {BLT_SWITCH_CUSTOM, "-x", "{x0 x1 ... xn}", (char *)NULL,
        Blt_Offset(LineSwitches, x), 0, 0, &arraySwitch},
    {BLT_SWITCH_CUSTOM, "-y", "{x0 x1 ... xn}", (char *)NULL,
        Blt_Offset(LineSwitches, y), 0, 0, &arraySwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_PaintBrush brush;               /* Color of rectangle. */
    Blt_Shadow shadow;
    int lineWidth;                      /* Width of outline. If zero,
                                         * indicates to draw a solid
                                         * rectangle. */
    int radius;                         /* Radius of rounded corner. */
    int antialiased;
    int width, height;
    Array coords;
} RectangleSwitches;

static Blt_SwitchSpec rectangleSwitches[] = 
{
    {BLT_SWITCH_BOOLEAN, "-antialiased", "bool", (char *)NULL,
        Blt_Offset(RectangleSwitches, antialiased), 0},
    {BLT_SWITCH_CUSTOM, "-color", "color", (char *)NULL,
        Blt_Offset(RectangleSwitches, brush),    0, 0, &paintbrushSwitch},
    {BLT_SWITCH_CUSTOM, "-coords", "{x0 y0 x1 y1}", (char *)NULL,
        Blt_Offset(RectangleSwitches, coords), 0, 0, &arraySwitch},
    {BLT_SWITCH_INT_NNEG, "-height", "numPixels", (char *)NULL,
        Blt_Offset(RectangleSwitches, height), 0},
    {BLT_SWITCH_INT_NNEG, "-linewidth", "number", (char *)NULL,
        Blt_Offset(RectangleSwitches, lineWidth), 0}, 
    {BLT_SWITCH_INT_NNEG, "-radius", "numPixels", (char *)NULL,
        Blt_Offset(RectangleSwitches, radius), 0},
    {BLT_SWITCH_CUSTOM, "-shadow", "offset", (char *)NULL,
        Blt_Offset(RectangleSwitches, shadow), 0, 0, &shadowSwitch},
    {BLT_SWITCH_INT_NNEG, "-width", "numPixels", (char *)NULL,
        Blt_Offset(RectangleSwitches, width), 0},
    {BLT_SWITCH_END}
};

extern Tcl_ObjCmdProc Blt_Picture_CircleOp;
extern Tcl_ObjCmdProc Blt_Picture_EllipseOp;
extern Tcl_ObjCmdProc Blt_Picture_LineOp;
extern Tcl_ObjCmdProc Blt_Picture_PolygonOp;
extern Tcl_ObjCmdProc Blt_Picture_RectangleOp;

/*
 *---------------------------------------------------------------------------
 *
 * ColorSwitchProc --
 *
 *      Convert a Tcl_Obj representing a Blt_Pixel color.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColorSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Blt_Pixel *pixelPtr = (Blt_Pixel *)(record + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
        pixelPtr->u32 = 0x00;
        return TCL_OK;
    }
    if (Blt_GetPixelFromObj(interp, objPtr, pixelPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreePaintBrushProc --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FreePaintBrushProc(ClientData clientData, char *record, int offset, int flags)
{
    Blt_PaintBrush *brushPtr = (Blt_PaintBrush *)(record + offset);

    if (*brushPtr != NULL) {
        Blt_FreeBrush(*brushPtr);
    }
    *brushPtr = NULL;
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToPaintBrushProc --
 *
 *      Convert a Tcl_Obj representing a paint brush.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPaintBrushProc(ClientData clientData, Tcl_Interp *interp,
                    const char *switchName, Tcl_Obj *objPtr, char *record,
                    int offset, int flags)      
{
    Blt_PaintBrush *brushPtr = (Blt_PaintBrush *)(record + offset);
    Blt_PaintBrush brush;

    if (Blt_GetPaintBrushFromObj(interp, objPtr, &brush) != TCL_OK) {
        return TCL_ERROR;
    }
    if (*brushPtr != NULL) {
        Blt_FreeBrush(*brushPtr);
    }
    *brushPtr = brush;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArrayFreeProc --
 *
 *      Free the storage associated with the -table switch.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
ArrayFreeProc(ClientData clientData, char *record, int offset, int flags)
{
    Array *arrayPtr = (Array *)(record + offset);

    if (arrayPtr->values != NULL) {
        Blt_Free(arrayPtr->values);
    }
    arrayPtr->values = NULL;
    arrayPtr->numValues = 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * ArraySwitchProc --
 *
 *      Convert a Tcl_Obj list of numbers into an array of doubles.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ArraySwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to return results. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    Tcl_Obj **objv;
    Array *arrayPtr = (Array *)(record + offset);
    double *values;
    int i;
    int objc;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    values = Blt_Malloc(sizeof(double) * objc);
    if (values == NULL) {
        Tcl_AppendResult(interp, "can't allocated coordinate array of ",
                Blt_Itoa(objc), " elements", (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
        if (Tcl_GetDoubleFromObj(interp, objv[i], values + i) != TCL_OK) {
            Blt_Free(values);
            return TCL_ERROR;
        }
    }
    arrayPtr->values = values;
    arrayPtr->numValues = objc;
    return TCL_OK;
}

void
Blt_Shadow_Set(Blt_Shadow *s, int width, int offset, int color, int alpha)
{
    s->color.u32 = color;
    s->color.Alpha = alpha;
    s->offset = offset;
    s->width = width;
}

/*
 *---------------------------------------------------------------------------
 *
 * ShadowSwitchProc --
 *
 *      Convert a Tcl_Obj representing a number for the alpha value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ShadowSwitchProc(ClientData clientData, Tcl_Interp *interp,
                 const char *switchName, Tcl_Obj *objPtr, char *record,
                 int offset, int flags)      
{
    Blt_Shadow *shadowPtr = (Blt_Shadow *)(record + offset);
    int objc;
    Tcl_Obj **objv;
    int i, width;

    shadowPtr->offset = 2;
    shadowPtr->width = 2;
    shadowPtr->color.u32 = 0xA0000000;
    if (Tcl_GetIntFromObj(NULL, objPtr, &width) == TCL_OK) {
        shadowPtr->width = shadowPtr->offset = width;
        return TCL_OK;
    }
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i += 2) {
        const char *string;
        int length;
        char c;

        string = Tcl_GetStringFromObj(objv[i], &length);
        if (string[0] != '-') {
            Tcl_AppendResult(interp, "bad shadow option \"", string, 
                "\": should be -offset, -width, or -color", (char *)NULL);
            return TCL_ERROR;
        }
        c = string[1];
        if ((c == 'w') && (strncmp(string, "-width", length) == 0)) {
            if (Tcl_GetIntFromObj(interp, objv[i+1], &shadowPtr->width) 
                != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'o') && (strncmp(string, "-offset", length) == 0)) {
            if (Tcl_GetIntFromObj(interp, objv[i+1], &shadowPtr->offset) 
                != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'c') && (strncmp(string, "-color", length) == 0)) {
            if (Blt_GetPixelFromObj(interp, objv[i+1], &shadowPtr->color) 
                != TCL_OK) {
                return TCL_ERROR;
            }
        } else if ((c == 'a') && (strncmp(string, "-alpha", length) == 0)) {
            int alpha;

            if (Tcl_GetIntFromObj(interp, objv[i+1], &alpha) != TCL_OK) {
                return TCL_ERROR;
            }
            shadowPtr->color.Alpha = alpha;
        } else {
            Tcl_AppendResult(interp, "unknown shadow option \"", string, 
                "\": should be -offset, -width, or -color", (char *)NULL);
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}


static void INLINE 
PutPixel(Pict *destPtr, int x, int y, Blt_Pixel *colorPtr)  
{
    if ((x >= 0) && (x < destPtr->width) && (y >= 0) && (y < destPtr->height)) {
        Blt_Pixel *dp;

        dp = Blt_Picture_Pixel(destPtr, x, y);
        dp->u32 = colorPtr->u32; 
    }
}


static INLINE Blt_Pixel
PremultiplyAlpha(Blt_Pixel *colorPtr, unsigned int alpha)
{
    Blt_Pixel new;
    int t;

    new.u32 = colorPtr->u32;
    alpha = imul8x8(alpha, colorPtr->Alpha, t);
    if ((alpha != 0xFF) && (alpha != 0x00)) {
        new.Red = imul8x8(alpha, colorPtr->Red, t);
        new.Green = imul8x8(alpha, colorPtr->Green, t);
        new.Blue = imul8x8(alpha, colorPtr->Blue, t);
    }
    new.Alpha = alpha;
    return new;
}

static void INLINE
HorizLine(Pict *destPtr, int x1, int x2, int y, Blt_Pixel *colorPtr)  
{
    Blt_Pixel *destRowPtr;
    Blt_Pixel *dp, *dend;
    size_t length;

    if (x1 > x2) {
        int tmp;

        tmp = x1, x1 = x2, x2 = tmp;
    }
    destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * y) + x1;
    length = x2 - x1 + 1;
    for (dp = destRowPtr, dend = dp + length; dp < dend; dp++) {
        dp->u32 = colorPtr->u32;
    }
}

static void INLINE 
VertLine(Pict *destPtr, int x, int y1, int y2, Blt_Pixel *colorPtr)  
{
    Blt_Pixel *dp;
    int y;

    if (y1 > y2) {
        int tmp;

        tmp = y1, y1 = y2, y2 = tmp;
    }
    dp = destPtr->bits + (destPtr->pixelsPerRow * y1) + x;
    for (y = y1; y <= y2; y++) {
        dp->u32 = colorPtr->u32;
        dp += destPtr->pixelsPerRow;
    }
}

static INLINE void
BlendPixels(Blt_Pixel *bgPtr, Blt_Pixel *colorPtr)
{
    unsigned char beta;
    int t;

    beta = colorPtr->Alpha ^ 0xFF;
    bgPtr->Red   = colorPtr->Red   + imul8x8(beta, bgPtr->Red, t);
    bgPtr->Green = colorPtr->Green + imul8x8(beta, bgPtr->Green, t);
    bgPtr->Blue  = colorPtr->Blue  + imul8x8(beta, bgPtr->Blue, t);
    bgPtr->Alpha = colorPtr->Alpha + imul8x8(beta, bgPtr->Alpha, t);
}
    

static void INLINE 
PutPixel2(Pict *destPtr, int x, int y, Blt_Pixel *colorPtr, 
          unsigned char weight)  
{
    Blt_FadeColor(colorPtr, weight);
    if ((x >= 0) && (x < destPtr->width) && (y >= 0) && (y < destPtr->height)) {
        Blt_Pixel *dp;

        dp = Blt_Picture_Pixel(destPtr, x, y);
        BlendPixels(dp, colorPtr);
    }
}

static void 
PaintLineSegment(
    Pict *destPtr,
    int x1, int y1, 
    int x2, int y2, 
    int lineWidth,
    Blt_Pixel *colorPtr)
{
    int dx, dy, xDir;
    unsigned long error;
    Blt_Pixel edge;

    if (y1 > y2) {
        int tmp;

        tmp = y1, y1 = y2, y2 = tmp;
        tmp = x1, x1 = x2, x2 = tmp;
    }
    edge = PremultiplyAlpha(colorPtr, 255);
    /* First and last Pixels always get Set: */
    PutPixel2(destPtr, x1, y1, colorPtr, 255);
    PutPixel2(destPtr, x2, y2, colorPtr, 255);

    dx = x2 - x1;
    dy = y2 - y1;

    if (dx >= 0) {
        xDir = 1;
    } else {
        xDir = -1;
        dx = -dx;
    }
    if (dy == 0) {                      /* Horizontal line */
        HorizLine(destPtr, x1, x2, y1, &edge);
        return;
    }
    if (dx == 0) {                      /*  Vertical line */
        VertLine(destPtr, x1, y1, y2, &edge);
        return;
    }
    if (dx == dy) {                     /* Diagonal line. */
        Blt_Pixel *dp;

        dp = Blt_Picture_Pixel(destPtr, x1, y1);
        while(dy-- > 0) {
            dp += destPtr->pixelsPerRow + xDir;
            dp->u32 = colorPtr->u32;
        }
        return;
    }

    /* use Wu Antialiasing: */

    error = 0;
    if (dy > dx) {                      /* y-major line */
        unsigned long adjust;

        /* x1 -= lineWidth / 2; */
        adjust = (dx << 16) / dy;
        while(--dy) {
            unsigned int weight;
            
            error += adjust;
            ++y1;
            if (error & ~0xFFFF) {
                x1 += xDir;
                error &= 0xFFFF;
            }
            weight = (unsigned char)(error >> 8);
            PutPixel2(destPtr, x1, y1, colorPtr, ~weight);
            PutPixel2(destPtr, x1 + xDir, y1, colorPtr, weight);
        }
    } else {                            /* x-major line */
        unsigned long adjust;

        /* y1 -= lineWidth / 2; */
        adjust = (dy << 16) / dx;
        while (--dx) {
            unsigned int weight;

            error += adjust;
            x1 += xDir;
            if (error & ~0xFFFF) {
                y1++;
                error &= 0xFFFF;
            }
            weight = (error >> 8) & 0xFF;
            PutPixel2(destPtr, x1, y1, colorPtr, ~weight);
            PutPixel2(destPtr, x1, y1 + 1, colorPtr, weight);
        }
    }
}

typedef struct {
    int left, right;
} ScanLine;

static ScanLine *
MakeScanLines(int numLines)
{
    ScanLine *coords;
    int i;

    coords = Blt_AssertMalloc(sizeof(ScanLine) * numLines);
    for(i = 0; i < numLines; i++) {
        coords[i].left = INT_MAX;
        coords[i].right = -INT_MAX;
    }
    return coords;
}

static void 
AddEllipseCoord(ScanLine *coords, int x, int y)
{
    if (x < coords[y].left) {
        coords[y].left = x;
    } 
    if (x > coords[y].right) {
        coords[y].right = x;
    }
}

static ScanLine *
ComputeEllipseQuadrant(int a, int b)
{
    ScanLine *coords;
    double t;
    int dx, dy;
    double a2, b2;
    double p, px, py;
    a2 = a * a;
    b2 = b * b;
    
    dx = 0;
    dy = b;
    px = 0;                             /* b2 * (dx + 1) */
    py = (double)(a2 + a2) * dy;        /* a2 * (dy - 0.5) */

    coords = MakeScanLines(b + 1);
    if (coords == NULL) {
        return NULL;
    }
    AddEllipseCoord(coords, dx, dy);

    t = (b2 - (a2 * b) + (0.25 * a2));
    p = (int)ROUND(t);
    while (py > px) {
        dx++;
        px += b2 + b2;
        if (dy <= 0) {
            continue;
        }
        if (p < 0) {
            p += b2 + px;
        } else {
            dy--;
            py -= a2 + a2;
            p += b2 + px - py;
        }
        AddEllipseCoord(coords, dx, dy);
    }
    {
        double dx2, dy2;
 
        dx2 = (dx + 0.5) * (dx + 0.5);
        dy2 = (dy - 1) * (dy - 1);
        t = (b2 * dx2 + a2 * dy2 - (a2 * b2));
        p = (int)ROUND(t);
    }
    while (dy > 0) {
        dy--;
        py -= a2 + a2;
        if (p > 0) {
            p += a2 - py;
        } else {
            dx++;
            px += b2 + b2;
            p += a2 - py + px;
        }
        AddEllipseCoord(coords, dx, dy);
    }
    AddEllipseCoord(coords, dx, dy);
    return coords;
}

static void INLINE
PaintPixel(Pict *destPtr, int x, int y, Blt_Pixel *colorPtr) 
{
    if ((x >= 0) && (x < destPtr->width) && (y >= 0) && (y < destPtr->height)) {
        BlendPixels(Blt_Picture_Pixel(destPtr, x, y), colorPtr);
    }
}

static void INLINE
PaintHorizontalLine(Pict *destPtr, int x1, int x2, int y, 
                    Blt_PaintBrush brush, int blend)  
{
    if ((y >= 0) && (y < destPtr->height)) {
        Blt_Pixel *dp, *dend;

        if (x1 > x2) {
            int tmp;
            
            tmp = x1, x1 = x2, x2 = tmp;
        }
        x1 = MAX(x1, 0);
        x2 = MIN(x2, destPtr->width);
        dp   = destPtr->bits + (y * destPtr->pixelsPerRow) + x1;
        dend = destPtr->bits + (y * destPtr->pixelsPerRow) + x2;
        if (Blt_IsVerticalLinearBrush(brush)) {
            int x;
            Blt_Pixel color;
            
            color.u32 = Blt_GetAssociatedColorFromBrush(brush, x1, y);
            for (x = x1; x <= x2; x++, dp++) {
                BlendPixels(dp, &color);
            }
            return;
        }
        if (blend) {
            int x;

            for (x = x1; dp < dend; dp++, x++) {
                Blt_Pixel color;

                color.u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
                BlendPixels(dp, &color);
            }
        } else {
            int x;

            dp->u32 = Blt_GetAssociatedColorFromBrush(brush, x1, y);
            for (x = x1; dp < dend; dp++, x++) {
                dp->u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
            }
        }
    }
}

static void INLINE
FillHorizontalLine(Pict *destPtr, int x1, int x2, int y, Blt_Pixel *colorPtr, 
                   int blend)  
{
    if ((y >= 0) && (y < destPtr->height)) {
        Blt_Pixel *dp, *dend;

        if (x1 > x2) {
            int tmp;
            
            tmp = x1, x1 = x2, x2 = tmp;
        }
        x1 = MAX(x1, 0);
        x2 = MIN(x2, destPtr->width - 1);
        dp   = destPtr->bits + (y * destPtr->pixelsPerRow) + x1;
        dend = destPtr->bits + (y * destPtr->pixelsPerRow) + x2;
        if (blend) {
            for (/*empty*/; dp <= dend; dp++) {
                BlendPixels(dp, colorPtr);
            }
        } else {
            for (/*empty*/; dp < dend; dp++) {
                dp->u32 = colorPtr->u32;
            }
        }
    }
}

static void INLINE 
FillVerticalLine(Pict *destPtr, int x, int y1, int y2, Blt_Pixel *colorPtr, 
                 int blend)  
{
    if ((x >= 0) && (x < destPtr->width)) {
        Blt_Pixel *dp, *dend;

        if (y1 > y2) {
            int tmp;
            
            tmp = y1, y1 = y2, y2 = tmp;
        }
        y1 = MAX(y1, 0);
        y2 = MIN(y2, destPtr->height - 1);
        dp   = destPtr->bits + (y1 * destPtr->pixelsPerRow) + x;
        dend = destPtr->bits + (y2 * destPtr->pixelsPerRow) + x;
        if (blend) {
            for (/*empty*/; dp <= dend; dp += destPtr->pixelsPerRow) {
                BlendPixels(dp, colorPtr);
            }
        } else {
            for (/*empty*/; dp <= dend; dp += destPtr->pixelsPerRow) {
                dp->u32 = colorPtr->u32;
            }
        }           
    }
}

static void
BrushHorizontalLine(Pict *destPtr, int x1, int x2, int y, Blt_PaintBrush brush)
{
    Blt_Pixel *dp;
    int x;

    if (x1 > x2) {
        int tmp;

        tmp = x1, x1 = x2, x2 = tmp;
    }
    dp = destPtr->bits + (destPtr->pixelsPerRow * y) + x1;
    /* Cheat for vertical linear brushes. */
    /* If linear and vertical get color once and set across */
    if (Blt_IsVerticalLinearBrush(brush)) {
        Blt_Pixel color;

        color.u32 = Blt_GetAssociatedColorFromBrush(brush, x1, y);
        for (x = x1; x <= x2; x++, dp++) {
            BlendPixels(dp, &color);
        }
    } else {
        for (x = x1; x <= x2; x++, dp++) {
            Blt_Pixel color;
            
            color.u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
            BlendPixels(dp, &color);
        }
    }
}

static INLINE double 
sqr(double x) 
{
    return x * x;
}
    
static void
PaintCircle4(Pict *destPtr, double cx, double cy, double r, double lineWidth, 
             Blt_PaintBrush brush, int blend)
{
    int x, y, i;
    int x1, x2, y1, y2;
    double outer, inner, outer2, inner2;
    double *squares;
    Blt_Pixel *destRowPtr;

    /* Determine some helpful values (singles) */
    outer = r;
    if (lineWidth > 0) {
        inner = r - lineWidth;
    } else {
        inner = 0;
    }
    outer2 = outer * outer;
    inner2 = inner * inner;

    /* Determine bounds: */
    x1 = (int)floor(cx - outer);
    if (x1 < 0) {
        x1 = 0;
    }
    x2 = (int)ceil(cx + outer) + 1;
    if (x2 >= destPtr->width) {
        x2 = destPtr->width;
    }
    y1 = (int)floor(cy - outer);
    if (y1 < 0) {
        y1 = 0;
    }
    y2 = (int)ceil(cy + outer) + 1;
    if (y2 >= destPtr->height) {
        y2 = destPtr->height;
    }
    if ((x1 >= destPtr->width) || (y1 >= destPtr->height) ||
        (x2 < 0) || (y2 < 0)) {
        return;
    }
    /* Optimization run: find squares of X first */
    squares = Blt_AssertMalloc(sizeof(double) * ABS(x2 - x1));
    for (i = 0, x = x1; x < x2; x++, i++) {
        squares[i] = (x - cx) * (x - cx);
    }
    /* Loop through Y values */
    destRowPtr = destPtr->bits + (y1 * destPtr->pixelsPerRow) + x1;
    for (y = y1; y < y2; y++) {
        Blt_Pixel *dp;
        double dy2;
        
        dy2 = (y - cy) * (y - cy);
        for (dp = destRowPtr, x = x1; x < x2; x++, dp++) {
            double dx2, d2, d;
            unsigned int a;
            double outerf, innerf;

            dx2 = squares[x - x1];
            /* Compute distance from circle center to this pixel. */
            d2 = dy2 + dx2;
            if (d2 > outer2) {
                continue;
            }
            if (d2 < inner2) {
                continue;
            }
            /* Mix the color.*/
            d = sqrt(d2);
            outerf = outer - d;
            innerf = d - inner;
#ifdef notdef
            dp->u32 = colorPtr->u32;
#endif
            if (outerf < 1.0) {
                a = (int)(outerf * 255.0 + 0.5);
            } else if ((inner > 0) && (innerf < 1.0)) {
                a = (int)(innerf * 255.0 + 0.5);
            } else {
                a = 255;
            }
            if (blend) {
                Blt_Pixel color;

                a = UCLAMP(a);
#ifdef notdef
                if (dp->Alpha != 0) {
                    a = imul8x8(a, dp->Alpha, t);
                }
#endif
                color.u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
                Blt_FadeColor(&color, a);
                BlendPixels(dp, &color);
            } else {
                int t;
                /* FIXME: This is overriding the alpha of a premultiplied
                 * color. */
                a = UCLAMP(a);
                dp->u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
                dp->Alpha = imul8x8(a, dp->Alpha, t);
            }
        }
        destRowPtr += destPtr->pixelsPerRow;
    }
    destPtr->flags &= ~BLT_PIC_PREMULT_COLORS;
    Blt_Free(squares);
}

static void
PaintThickEllipse(
    Blt_Picture picture, 
    int x, int y,               /* Center of the ellipse. */
    int a,                      /* Half the width of the ellipse. */
    int b,                      /* Half the height of the ellipse. */
    int lineWidth,              /* Line width of the ellipse. Must be 1 or
                                 * greater. */
    Blt_Pixel *colorPtr,
    int blend)
{
    ScanLine *outer, *inner;
    Blt_Pixel fill;
    int dy;
    int dx1, dx2;

    lineWidth--;
    outer = ComputeEllipseQuadrant(a, b);
    if (outer == NULL) {
        return;
    }
    inner = ComputeEllipseQuadrant(a - lineWidth, b - lineWidth);
    if (blend) {
        fill = PremultiplyAlpha(colorPtr, 255);
    } else {
        fill.u32 = colorPtr->u32;
    }
    dx1 = outer[0].right;
    dx2 = inner[0].left;
    FillHorizontalLine(picture, x + dx2, x + dx1, y, &fill, blend);
    FillHorizontalLine(picture, x - dx1, x - dx2, y, &fill, blend);
    for (dy = 1; dy < (b - lineWidth); dy++) {
        dx1 = outer[dy].right;
        dx2 = inner[dy].left;
        FillHorizontalLine(picture, x + dx2, x + dx1, y - dy, &fill, blend);
        FillHorizontalLine(picture, x + dx2, x + dx1, y + dy, &fill, blend);
        FillHorizontalLine(picture, x - dx1, x - dx2, y - dy, &fill, blend);
        FillHorizontalLine(picture, x - dx1, x - dx2, y + dy, &fill, blend);
    }
    for (/* empty */; dy <= b; dy++) {
        int dx;

        dx = outer[dy].right;
        FillHorizontalLine(picture, x - dx, x + dx, y + dy, &fill, blend);
        FillHorizontalLine(picture, x - dx, x + dx, y - dy, &fill, blend);
    }
    Blt_Free(outer);
    Blt_Free(inner);
}


static void
PaintFilledEllipse(
    Blt_Picture picture, 
    int x, int y,               /* Center of the ellipse. */
    int a,                      /* Half the width of the ellipse. */
    int b,                      /* Half the height of the ellipse. */
    Blt_Pixel *colorPtr,
    int blend)
{
    ScanLine *coords;
    Blt_Pixel fill;
    int dx, dy;

    coords = ComputeEllipseQuadrant(a, b);
    if (blend) {
        fill = PremultiplyAlpha(colorPtr, 255);
    } else {
        fill.u32 = colorPtr->u32;
    }
    if (coords == NULL) {
        return;
    }
    FillHorizontalLine(picture, x - a, x + a, y, &fill, blend);
    for (dy = 1; dy <= b; dy++) {
        dx = coords[dy].right;
        FillHorizontalLine(picture, x - dx, x + dx, y + dy, &fill, blend);
        FillHorizontalLine(picture, x - dx, x + dx, y - dy, &fill, blend);
    }
    Blt_Free(coords);
}

static void
PaintEllipse(
    Blt_Picture picture, 
    int x, int y,               /* Center of the ellipse. */
    int a,                      /* Half the width of the ellipse. */
    int b,                      /* Half the height of the ellipse. */
    int lineWidth,              /* Line width of the ellipse.  If zero,
                                 * then draw a solid filled ellipse. */
    Blt_Pixel *colorPtr,
    int blend)
{
    if ((lineWidth >= a) || (lineWidth >= b)) {
        lineWidth = 0;
    }
    if (lineWidth < 1) {
        PaintFilledEllipse(picture, x, y, a, b, colorPtr, blend);
    } else {
        PaintThickEllipse(picture, x, y, a, b, lineWidth, colorPtr, blend);
    }
}

static void
PaintEllipseAA(
    Blt_Picture picture, 
    int x, int y,               /* Center of the ellipse. */
    int a,                      /* Half the width of the ellipse. */
    int b,                      /* Half the height of the ellipse. */
    int lineWidth,              /* Line thickness of the ellipse.  If zero,
                                 * then draw a solid filled ellipse. */
    Blt_Pixel *colorPtr)
{
    PictRegion region;
    Blt_Picture big;
    int numSamples = 3; 
    int ellipseWidth, ellipseHeight;
    int blend = 1;

    if ((lineWidth >= a) || (lineWidth >= b)) {
        lineWidth = 0;
    }
    ellipseWidth = a + a + 3;
    ellipseHeight = b + b + 3;
    region.x = x - (a + 1);
    region.y = y - (b + 1);
    region.w = ellipseWidth;
    region.h = ellipseHeight;
    
    if (!Blt_AdjustRegionToPicture(picture, &region)) {
        return;                 /* Ellipse is totally clipped. */
    }
    /* Scale the region forming the bounding box of the ellipse into a new
     * picture. The bounding box is scaled by *nSamples* times. */
    big = Blt_CreatePicture(ellipseWidth * numSamples, ellipseHeight * numSamples);
    if (big != NULL) {
        Blt_Picture tmp;
        int cx, cy;
        Blt_Pixel color;

        cx = a + 1;
        cy = b + 1;
        /* Now draw an ellipse scaled by the same amount. The center of the
         * ellipse is the center of the picture. */
        Blt_BlankPicture(big, 0x0);
        color.u32 = 0xFF000000;
        Blt_PremultiplyColor(&color);
        PaintEllipse(big, 
            cx * numSamples,    /* Center of ellipse. */
            cy * numSamples, 
            a * numSamples,     
            b * numSamples, 
            lineWidth * numSamples, /* Scaled line width. */
                &color,
                blend); 
            
        /* Reduce the picture back to the original size using a simple box
         * filter for smoothing. */
        tmp = Blt_CreatePicture(ellipseWidth, ellipseHeight);
        Blt_ResamplePicture(tmp, big, bltBoxFilter, bltBoxFilter);
        Blt_FreePicture(big);
        Blt_ApplyColorToPicture(tmp, colorPtr);
        /* Replace the bounding box in the original with the new. */
        Blt_CompositeRegion(picture, tmp, 0, 0, region.w, region.h, 
                region.x, region.y);
        Blt_FreePicture(tmp);
    }
}

static void
PaintRectangleShadow(Blt_Picture picture, int x, int y, int w, int h, int r, 
                     int lineWidth, Blt_Shadow *shadowPtr)
{
    int dw, dh;
    Blt_Picture blur;
    Blt_PaintBrush brush;

    dw = (w + shadowPtr->offset*3);
    dh = (h + shadowPtr->offset*3);
    blur = Blt_CreatePicture(dw, dh);
    Blt_BlankPicture(blur, 0x0);
    brush = Blt_NewColorBrush(shadowPtr->color.u32);
    Blt_PaintRectangle(blur, shadowPtr->offset, shadowPtr->offset, w, h, r, 
                       lineWidth, brush, TRUE);
    Blt_FreeBrush(brush);
    Blt_BlurPicture(blur, blur, shadowPtr->offset, 2);
    Blt_CompositeRegion(picture, blur, 0, 0, dw, dh, x, y);
    Blt_FreePicture(blur);
}

#define UPPER_LEFT      0
#define UPPER_RIGHT     1
#define LOWER_LEFT      2
#define LOWER_RIGHT     3

static void
PaintCorner(Pict *destPtr, int x, int y, int r, int lineWidth, int corner, 
            Blt_PaintBrush brush)
{
    int blend = 1;
    int outer, inner, outer2, inner2;
    int x1, x2, y1, y2, dx, dy;

    outer = r;
    if ((lineWidth > 0) && (lineWidth < r)) {
        inner = r - lineWidth;
    } else {
        inner = 0;
    }
    outer2 = r * r;
    inner2 = floor(inner * inner);
    
    x1 = x2 = y1 = y2 = 0;              /* Suppress compiler warning. */
    switch (corner) {
    case UPPER_LEFT:
        x1 = 0;
        x2 = r;
        y1 = 0;
        y2 = r;
        break;
    case UPPER_RIGHT:
        x1 = r + 1;
        x2 = r + r;
        y1 = 0;
        y2 = r;
        break;
    case LOWER_LEFT:
        x1 = 0;
        x2 = r;
        y1 = r + 1;
        y2 = r + r;
        break;
    case LOWER_RIGHT:
        x1 = r + 1;
        x2 = r + r;
        y1 = r + 1;
        y2 = r + r;
        break;
    }   
    for (dy = y1; dy < y2; dy++) {
        double dy2;

        if (((y + dy) < 0) || ((y + dy) >= destPtr->height)) {
            continue;
        }
        dy2 = (dy - r) * (dy - r);
        for (dx = x1; dx < x2; dx++) {
            double dx2, d2, d;
            unsigned int a;
            double outerf, innerf;
            Blt_Pixel *dp;

            if (((x + dx) < 0) || ((x + dx) >= destPtr->width)) {
                continue;
            }
            dx2 = (dx - r) * (dx - r);
            /* Compute distance from circle center to this pixel. */
            d2 = dy2 + dx2;
            if (d2 > outer2) {
                continue;
            }
            if (d2 < inner2) {
                continue;
            }
            /* Mix the color.*/
            d = sqrt(d2);
            outerf = outer - d;
            innerf = d - inner;
            if (outerf < 1.0) {
                a = (int)(outerf * 255.0 + 0.5);
            } else if ((inner > 0) && (innerf < 1.0)) {
                a = (int)(innerf * 255.0 + 0.5);
            } else {
                a = 255;
            }
            dp = Blt_Picture_Pixel(destPtr, x+dx, y+dy);
            if (blend) {
                Blt_Pixel color;
                
                a = UCLAMP(a);
                color.u32 = Blt_GetAssociatedColorFromBrush(brush,x+dx,y+dy);
                Blt_FadeColor(&color, a);
                BlendPixels(dp, &color);
            } else {
                a = UCLAMP(a);
                dp->u32 = Blt_GetAssociatedColorFromBrush(brush, x+dx, y+dy);
                Blt_FadeColor(dp, a);
            }
        }
    }
}

/* 
 *      
 *      ul  xxxxxxxxxxxxxxxxxx ur       Upper section
 *          xxxxxxxxxxxxxxxxxx  
 *      xxxxxxxxxxxxxxxxxxxxxxxxxx      Middle section
 *      xxxxxxxxxxxxxxxxxxxxxxxxxx 
 *      xxxxxxxxxxxxxxxxxxxxxxxxxx
 *          xxxxxxxxxxxxxxxxxx          Lower section
 *      ll  xxxxxxxxxxxxxxxxxx lr
 *      
 */
void
Blt_PaintRectangle(Blt_Picture picture, int x, int y, int w, int h, int r, 
                   int lineWidth, Blt_PaintBrush brush, int composite)
{
    /* If the linewidth exceeds half the height or width of the rectangle,
     * then paint as a solid rectangle.*/
    if (((lineWidth*2) >= w) || ((lineWidth*2) >= h)) {
        lineWidth = 0;
    }
    /* Radius of each rounded corner can't be bigger than half the width or
     * height of the rectangle. */
    if (r > (w / 2)) {
        r = w / 2;
    }
    if (r > (h / 2)) {
        r = h / 2;
    }
    if (r > 0) {
        if (lineWidth > 0) {
            int x1, x2, x3, x4, y1, y2, dy;

            /* Thick, rounded rectangle. */
            x1 = x + r;
            x2 = x + w - r;
            y1 = y;
            y2 = y + h - 1;
            for (dy = 0; dy < lineWidth; dy++) {
                PaintHorizontalLine(picture, x1, x2, y1+dy, brush, composite);
                PaintHorizontalLine(picture, x1, x2, y2-dy, brush, composite);
            }
            x1 = x;
            x2 = x + lineWidth;
            x3 = x + w - lineWidth;
            x4 = x + w;
            for (dy = r; dy < (h - r); dy++) {
                PaintHorizontalLine(picture, x1, x2, y+dy, brush, composite);
                PaintHorizontalLine(picture, x3, x4, y+dy, brush, composite);
            }
        } else {
            int x1, x2, y1, y2, dy;

            /* Filled, rounded, rectangle. */
            x1 = x + r;
            x2 = x + w - r;
            y1 = y;
            y2 = y + h - 1;
            for (dy = 0; dy < r; dy++) {
                PaintHorizontalLine(picture, x1, x2, y1+dy, brush, composite);
                PaintHorizontalLine(picture, x1, x2, y2-dy, brush, composite);
            }
            x1 = x;
            x2 = x + w;
            for (dy = r; dy < (h - r); dy++) {
                PaintHorizontalLine(picture, x1, x2, y+dy, brush, composite);
            }
        }
        { 
            int x1, y1;
            int d;

            d = r + r;
            /* Draw the rounded corners. */
            x1 = x - 1;
            y1 = y - 1;
            PaintCorner(picture, x1, y1, r + 1, lineWidth+1, 0, brush);
            x1 = x + w - d - 2;
            y1 = y - 1;
            PaintCorner(picture, x1, y1, r + 1, lineWidth+1, 1, brush);
            x1 = x - 1;
            y1 = y + h - d - 2;
            PaintCorner(picture, x1, y1, r + 1, lineWidth+1, 2, brush);
            x1 = x + w - d - 2;
            y1 = y + h - d - 2;
            PaintCorner(picture, x1, y1, r + 1, lineWidth+1, 3, brush);
        }
    } else {
        if (lineWidth > 0) {
            int x1, x2, x3, x4, y1, y2, dy;
            
            /* Thick, non-rounded, rectangle.  */
            x1 = x;
            x2 = x + w;
            y1 = y;
            y2 = y + h - lineWidth;
            for (dy = 0; dy < lineWidth; dy++) {
                PaintHorizontalLine(picture, x1, x2, y1+dy, brush, composite);
                PaintHorizontalLine(picture, x1, x2, y2-dy, brush, composite);
            }
            x1 = x;
            x2 = x + lineWidth;
            x3 = x + w - lineWidth;
            x4 = x + w;
            for (dy = r; dy < (h - lineWidth); dy++) {
                PaintHorizontalLine(picture, x1, x2, y+dy, brush, composite);
                PaintHorizontalLine(picture, x3, x4, y+dy, brush, composite);
            }
        } else {
            int x1, x2, dy;

            /* Filled, non-rounded, rectangle. */
            x1 = x;
            x2 = x + w;
            for (dy = 0; dy < h; dy++) {
                PaintHorizontalLine(picture, x1, x2, y+dy, brush, composite);
            }
        }
    } 
    if ((Blt_GetBrushAlpha(brush) != 0xFF) && (!composite)) {
        Pict *dstPtr = picture;
        dstPtr->flags |= BLT_PIC_COMPOSITE;
    }
}


static void 
PaintPolyline(
    Pict *destPtr,
    int numPoints, 
    Point2d *points, 
    int lineWidth,
    Blt_Pixel *colorPtr)
{
    int i;
    Region2d r;
    Point2d p;

    r.left = r.top = 0;
    r.right = destPtr->width - 1;
    r.bottom = destPtr->height - 1;
    p.x = points[0].x, p.y = points[0].y;
    for (i = 1; i < numPoints; i++) {
        Point2d q, next;

        q.x = points[i].x, q.y = points[i].y;
        next = q;
        PaintLineSegment(destPtr, ROUND(p.x), ROUND(p.y), ROUND(q.x), 
                          ROUND(q.y), 0, colorPtr);
#ifdef notdef
        if (Blt_LineRectClip(&r, &p, &q)) {
            PaintLineSegment(destPtr, ROUND(p.x), ROUND(p.y), ROUND(q.x), 
                ROUND(q.y), 1, colorPtr);
        }
#endif
        p = next;
    }
}


/*
 * Concave Polygon Scan Conversion
 * by Paul Heckbert
 * from "Graphics Gems", Academic Press, 1990
 */

/*
 * concave: scan convert nvert-sided concave non-simple polygon with vertices at
 * (point[i].x, point[i].y) for i in [0..nvert-1] within the window win by
 * calling spanproc for each visible span of pixels.
 * Polygon can be clockwise or counterclockwise.
 * Algorithm does uniform point sampling at pixel centers.
 * Inside-outside test done by Jordan's rule: a point is considered inside if
 * an emanating ray intersects the polygon an odd number of times.
 * drawproc should fill in pixels from xl to xr inclusive on scanline y,
 * e.g:
 *      drawproc(y, xl, xr)
 *      int y, xl, xr;
 *      {
 *          int x;
 *          for (x=xl; x<=xr; x++)
 *              pixel_write(x, y, pixelvalue);
 *      }
 *
 *  Paul Heckbert       30 June 81, 18 Dec 89
 */

#include <stdio.h>
#include <math.h>

typedef struct {                        /* A polygon edge */
    double x;                           /* X coordinate of edge's
                                         * intersection with current
                                         * scanline. */
    double dx;                          /* Change in x with respect to
                                           y. */
    int i;                              /* Edge number: edge i goes from
                                         * pt[i] to pt[i+1] */
} ActiveEdge;

typedef struct {
    int numActive;
    ActiveEdge *active;
} AET;

/* Comparison routines for qsort */
static int n;                           /* # of vertices */
static Point2d *pt;                     /* vertices */

static int 
CompareIndices(const void *a, const void *b)
{
    return (pt[*(int *)a].y <= pt[*(int *)b].y) ? -1 : 1;
}

static int 
CompareActive(const void *a, const void *b)
{
    const ActiveEdge *e1, *e2;

    e1 = a;
    e2 = b;
    return (e1->x <= e2->x) ? -1 : 1;
}

static void
cdelete(AET *tablePtr, int i)           /* Remove edge i from active
                                         * list. */
{
    int j;

    for (j=0; (j < tablePtr->numActive) && (tablePtr->active[j].i != i); j++) {
        /*empty*/
    }
    if (j >= tablePtr->numActive) {
        return;                         /* Edge not in active list; happens
                                         * at win->y0*/
    }
    tablePtr->numActive--;

#ifdef notdef
    bcopy(&tablePtr->active[j+1], &tablePtr->active[j], 
          (tablePtr->numActive-j) *sizeof tablePtr->active[0]);
#else
    memmove(&tablePtr->active[j], &tablePtr->active[j+1], 
          (tablePtr->numActive-j) *sizeof tablePtr->active[0]);
#endif
}

/* append edge i to end of active list */
static void
cinsert(AET *tablePtr, size_t n, Point2d *points, int i, int y)
{
    int j;
    Point2d *p, *q;
    ActiveEdge *edgePtr;

    j = (i < (n - 1)) ? i + 1 : 0;
    if (points[i].y < points[j].y) {
        p = points + i;
        q = points + j;
    } else {
        p = points + j;
        q = points + i;
    }
    edgePtr = tablePtr->active + tablePtr->numActive;
    assert(tablePtr->numActive < n);
    /* Initialize x position at intersection of edge with scanline y. */
    edgePtr->dx = (q->x - p->x) / (q->y - p->y);
    edgePtr->x  = edgePtr->dx * (y + 0.5 - p->y) + p->x;
    edgePtr->i  = i;
    tablePtr->numActive++;
}

void
Blt_PaintPolygon(Pict *destPtr, int numVertices, Point2d *vertices, 
                 Blt_PaintBrush brush)
{
    int y, k;
    int top, bot;
    int *map;                           /* List of vertex indices, sorted by
                                         * pt[map[j]].y */
    AET aet;

    n = numVertices;
    pt = vertices;
    if (n <= 0) {
        return;
    }
    if (destPtr->height == 0) {
        return;
    }
    map = Blt_AssertMalloc(numVertices * sizeof(unsigned int));
    aet.active = Blt_AssertCalloc(numVertices, sizeof(ActiveEdge));

    /* create y-sorted array of indices map[k] into vertex list */
    for (k = 0; k < n; k++) {
        map[k] = k;
    }
    /* sort map by pt[map[k]].y */
    qsort(map, n, sizeof(unsigned int), CompareIndices); 

    aet.numActive = 0;                  /* start with empty active list */
    k = 0;                              /* map[k] is next vertex to process */
    top = MAX(0, ceil(vertices[map[0]].y-.5)); /* ymin of polygon */
    bot = MIN(destPtr->height-1, floor(vertices[map[n-1]].y-.5)); /* ymax */

    for (y = top; y <= bot; y++) {      /* step through scanlines */
        unsigned int i, j;

        /* Scanline y is at y+.5 in continuous coordinates */

        /* Check vertices between previous scanline and current one, if any */
        for (/*empty*/; (k < n) && (vertices[map[k]].y <= (y +.5)); k++) {
            /* to simplify, if pt.y=y+.5, pretend it's above */
            /* invariant: y-.5 < pt[i].y <= y+.5 */
            i = map[k]; 
            /*
             * Insert or delete edges before and after vertex i (i-1 to i, and
             * i to i+1) from active list if they cross scanline y
             */
            /* vertex previous to i */
            j = (i > 0) ? (i - 1) : (n - 1);
            if (vertices[j].y <= (y - 0.5)) {   
                /* old edge, remove from active list */
                cdelete(&aet, j);
            } else if (vertices[j].y > (y + 0.5)) { 
                /* new edge, add to active list */
                cinsert(&aet, numVertices, vertices, j, y);
            }
            j = (i < (n - 1)) ? (i + 1) : 0; /* vertex next after i */
            if (vertices[j].y <= (y-.5)) { 
                /* old edge, remove from active list */
                cdelete(&aet, i);
            } else if (vertices[j].y > (y+.5)){ 
                /* new edge, add to active list */
                cinsert(&aet, numVertices, vertices, i, y);
            }
        }
        /* Sort active edge list by active[j].x */
        qsort(aet.active, aet.numActive, sizeof(ActiveEdge), CompareActive);

        /* Draw horizontal segments for scanline y */
        for (j = 0; j < aet.numActive; j += 2) { /* draw horizontal segments */
            int left, right;
            ActiveEdge *p, *q;

            p = aet.active + j;
            q = p+1;
            /* span 'tween j & j+1 is inside, span tween j+1 & j+2 is outside */
            left  = (int)ceil (p->x - 0.5); /* left end of span */
            right = (int)floor(q->x - 0.5); /* right end of span */
            if (left < 0) {
                left = 0;
            }
            if (right >= (int)destPtr->width) {
                right = destPtr->width - 1;
            }
            if (left <= right) {
                BrushHorizontalLine(destPtr, left, right, y, brush);
            }
            p->x += p->dx;              /* increment edge coords */
            q->x += q->dx;
        }
    }
    Blt_Free(aet.active);
    Blt_Free(map);
}

static void
GetPolygonBoundingBox(size_t numVertices, Point2d *vertices, 
                      Region2d *regionPtr)
{
    int i;
    
    regionPtr->left = regionPtr->top = FLT_MAX;
    regionPtr->right = regionPtr->bottom = -FLT_MAX;
    for (i = 0; i < numVertices; i++) {
        Point2d *p;

        p = vertices + i;
        if (p->x < regionPtr->left) {
            regionPtr->left = p->x;
        } else if (p->x > regionPtr->right) {
            regionPtr->right = p->x;
        }
        if (p->y < regionPtr->top) {
            regionPtr->top = p->y;
        } else if (p->y > regionPtr->bottom) {
            regionPtr->bottom = p->y;
        }
    }
}

static void
TranslatePolygon(Point2d *src, Point2d *dst, size_t numVertices,
                 double x, double y, double scale)
{
    size_t i;

    for (i = 0; i < numVertices; i++) {
        dst[i].x = (src[i].x + x) * scale;
        dst[i].y = (src[i].y + y) * scale;
    }
}

static void
PaintPolygonShadow(Pict *destPtr, size_t numVertices, Point2d *vertices, 
                   Region2d *regionPtr, Blt_Shadow *shadowPtr)
{
    Blt_PaintBrush brush;
    Blt_Picture blur, tmp;
    Point2d *v;
    Region2d r2;
    int w, h;
    int x1, x2, y1, y2;

    x1 = y1 = 0;
    x2 = destPtr->width, y2 = destPtr->height;
    if (regionPtr->left > 0) {
        x1 = (int)regionPtr->left;
    }
    if (regionPtr->top > 0) {
        y1 = (int)regionPtr->top;
    }
    if (regionPtr->right < x2) {
        x2 = (int)ceil(regionPtr->right);
    }
    if (regionPtr->bottom < y2) {
        y2 = (int)ceil(regionPtr->bottom);
    }
    if ((x1 > 0) || (y1 > 0)) {
        v = Blt_AssertMalloc(numVertices * sizeof(Point2d));
        TranslatePolygon(vertices, v, numVertices, -x1, -y1, 1.0);
    } else {
        v = vertices;
    }
    w = (x2 - x1 + shadowPtr->offset*8);
    h = (y2 - y1 + shadowPtr->offset*8);
    tmp = Blt_CreatePicture(w, h);
    Blt_BlankPicture(tmp, 0x0);
    brush = Blt_NewColorBrush(shadowPtr->color.u32);
    GetPolygonBoundingBox(numVertices, v, &r2);
    Blt_PaintPolygon(tmp, numVertices, v, brush);
    Blt_FreeBrush(brush);
    if (v != vertices) {
        Blt_Free(v);
    }
    blur = Blt_CreatePicture(w, h);
    Blt_BlankPicture(blur, 0x0);
    Blt_CopyRegion(blur, tmp, 0, 0, w, h, shadowPtr->offset*2,
                   shadowPtr->offset*2); 
    Blt_BlurPicture(blur, blur, shadowPtr->width, 3);
    Blt_MaskPicture(blur, tmp, 0, 0, w, h, 0, 0, &shadowPtr->color);
    Blt_FreePicture(tmp);
    Blt_CompositeRegion(destPtr, blur, 0, 0, w, h, x1, y1);
    Blt_FreePicture(blur);
}

static void
PaintPolygonAA2(Pict *destPtr, size_t numVertices, Point2d *vertices, 
                Region2d *regionPtr, Blt_PaintBrush brush, 
                Blt_Shadow *shadowPtr)
{
    Blt_Picture big, tmp;
    Region2d r2;
    /* 
     * Get the minimum size region to draw both a supersized polygon and
     * shadow.
     *
     * Draw the shadow and then the polygon. Everything is 4x bigger
     * including the shadow offset.  This is a much bigger blur.
     * 
     * Resample the image back down to 1/4 the size and blend it into the
     * destination picture.
     */
    big = Blt_CreatePicture(destPtr->width * 4, destPtr->height * 4);
    TranslatePolygon(vertices, vertices, numVertices, 0.0f, 0.0f, 4.0);
    Blt_BlankPicture(big, 0x0);
    GetPolygonBoundingBox(numVertices, vertices, &r2);
    if ((shadowPtr != NULL) && (shadowPtr->width > 0)) {
        PaintPolygonShadow(big, numVertices, vertices, &r2, shadowPtr);
    }
    Blt_PaintPolygon(big, numVertices, vertices, brush);
    tmp = Blt_CreatePicture(destPtr->width, destPtr->height);
    Blt_ResamplePicture(tmp, big, bltBoxFilter, bltBoxFilter);
    Blt_FreePicture(big);
    Blt_CompositePictures(destPtr, tmp);
    Blt_FreePicture(tmp);
}

static void
DrawCircleShadow(Blt_Picture picture, int x, int y, double r, 
                 double lineWidth, int blend, Blt_Shadow *shadowPtr)
{
    Pict *tmpPtr;
    int w, h;
    Blt_PaintBrush brush;

    w = h = (r * 2) + 1 + (shadowPtr->width + shadowPtr->offset) * 2;
    tmpPtr = Blt_CreatePicture(w, h);
#ifdef notdef
    fprintf(stderr, "r=%g linewidth=%g offset=%d width=%d w=%d h=%d\n",
            r, lineWidth, shadowPtr->offset, shadowPtr->width, w, h);
#endif
    Blt_BlankPicture(tmpPtr, 0x0);
    brush = Blt_NewColorBrush(shadowPtr->color.u32);
    PaintCircle4(tmpPtr, 
                 r + shadowPtr->offset, /* x */
                 r + shadowPtr->offset, /* y */
                 r,                     /* radius */
                 lineWidth, 
                 brush, 
                 TRUE);
    Blt_FreeBrush(brush);
    if (blend) {
        Blt_BlurPicture(tmpPtr, tmpPtr, shadowPtr->width, 3);
        Blt_CompositeRegion(picture, tmpPtr, 0, 0, w, h, x - r, y - r);
    } else {
        Blt_CopyRegion(picture, tmpPtr, 0, 0, w, h, x - r, y - r);
    }
    Blt_FreePicture(tmpPtr);
}

static void
DrawCircle(Blt_Picture picture, int x, int y, int r, double lineWidth, 
           Blt_PaintBrush brush, int blend)
{
    PaintCircle4(picture, x, y, r, lineWidth, brush, blend);
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_Picture_CircleOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *      $img draw circle x y r ?switches?
 *        -linewidth 1
 *        -color color 
 *        -shadow shadow
 *        -antialiased bool
 *      
 *---------------------------------------------------------------------------
 */
int
Blt_Picture_CircleOp(ClientData clientData, Tcl_Interp *interp, int objc,
                     Tcl_Obj *const *objv)
{
    Blt_Picture picture = clientData;
    CircleSwitches switches;
    int x, y, radius;
    Blt_PaintBrush brush;

    if (objc < 5) {
        Tcl_AppendResult(interp, "wrong # of coordinates for circle",
                         (char *)NULL);
        return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &radius) != TCL_OK)) {
        return TCL_ERROR;
    }
    /* Process switches  */
    switches.lineWidth = 0.0;
    if (Blt_GetPaintBrush(interp, "white", &brush) != TCL_OK) {
        return TCL_ERROR;
    }
    switches.brush = brush;
    switches.blend = 1;
    switches.antialiased = 0;
    Blt_Shadow_Set(&switches.shadow, 0, 0, 0x0, 0xFF);
    if (Blt_ParseSwitches(interp, circleSwitches, objc - 6, objv + 6, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (switches.shadow.width > 0) {
        DrawCircleShadow(picture, x, y, radius, switches.lineWidth, 
                switches.blend, &switches.shadow);
    }
    Blt_SetBrushRegion(switches.brush, x - radius, y - radius, 
        radius + radius, radius + radius);
    DrawCircle(picture, x, y, radius, switches.lineWidth, switches.brush, 
               switches.blend);
    Blt_FreeSwitches(circleSwitches, (char *)&switches, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Picture_EllipseOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Picture_EllipseOp(ClientData clientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const *objv)
{
    Blt_Picture picture = clientData;
    EllipseSwitches switches;
    int x, y, a, b;

    if (objc < 7) {
        Tcl_AppendResult(interp, "wrong # of coordinates for circle",
                         (char *)NULL);
        return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &a) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[6], &b) != TCL_OK)) {
        return TCL_ERROR;
    }
    /* Process switches  */
    switches.lineWidth = 0;
    switches.fill.u32 = 0xFFFFFFFF;
    switches.outline.u32 = 0xFF000000;
    switches.antialiased = FALSE;
    if (Blt_ParseSwitches(interp, ellipseSwitches, objc - 7, objv + 7, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if ((switches.lineWidth >= a) || (switches.lineWidth >= b)) {
        /* If the requested line width is greater than the radius then draw a
         * solid ellipse instead. */
        switches.lineWidth = 0;
    }
    if (switches.antialiased) {
        PaintEllipseAA(picture, x, y, a, b, switches.lineWidth, &switches.fill);
    } else {
        PaintEllipse(picture, x, y, a, b, switches.lineWidth, &switches.fill, 1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Picture_LineOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Picture_LineOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    Blt_Picture picture = clientData;
    LineSwitches switches;
    size_t numPoints;
    Point2d *points;
    
    memset(&switches, 0, sizeof(switches));
    switches.bg.u32 = 0xFFFFFFFF;
    if (Blt_ParseSwitches(interp, lineSwitches, objc - 3, objv + 3, 
                &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (switches.x.numValues != switches.y.numValues) {
        Tcl_AppendResult(interp, "-x and -y coordinate lists must have the ",
                " same number of coordinates.",(char *)NULL);
        return TCL_ERROR;
    }
    points = NULL;
    if (switches.x.numValues > 0) {
        size_t i;
        double *x, *y;

        numPoints = switches.x.numValues;
        points = Blt_Malloc(sizeof(Point2d) * numPoints);
        if (points == NULL) {
            Tcl_AppendResult(interp, "can't allocate memory for ", 
                Blt_Itoa(numPoints + 1), " points", (char *)NULL);
            return TCL_ERROR;
        }
        x = switches.x.values;
        y = switches.y.values;
        for (i = 0; i < numPoints; i++) {
            points[i].x = x[i];
            points[i].y = y[i];
        }
        Blt_Free(switches.x.values);
        Blt_Free(switches.y.values);
        switches.x.values = switches.y.values = NULL;
    } else if (switches.coords.numValues > 0) {
        size_t i, j;
        double *coords;

        if (switches.coords.numValues & 0x1) {
            Tcl_AppendResult(interp, "bad -coords list: ",
                "must have an even number of values", (char *)NULL);
            return TCL_ERROR;
        }
        numPoints = (switches.coords.numValues / 2);
        points = Blt_Malloc(sizeof(Point2d)* numPoints);
        if (points == NULL) {
            Tcl_AppendResult(interp, "can't allocate memory for ", 
                Blt_Itoa(numPoints + 1), " points", (char *)NULL);
            return TCL_ERROR;
        }
        coords = switches.coords.values;
        for (i = 0, j = 0; i < switches.coords.numValues; i += 2, j++) {
            points[j].x = coords[i];
            points[j].y = coords[i+1];
        }
        Blt_Free(switches.coords.values);
        switches.coords.values = NULL;
    }
    if (points != NULL) {
        PaintPolyline(picture, numPoints, points, switches.lineWidth, 
                   &switches.bg);
        Blt_Free(points);
    }
    Blt_FreeSwitches(lineSwitches, (char *)&switches, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Picture_PolygonOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *      $pict draw polygon -coords $coords -color $color 
 *---------------------------------------------------------------------------
 */
int
Blt_Picture_PolygonOp(ClientData clientData, Tcl_Interp *interp, int objc,
                      Tcl_Obj *const *objv)
{
    Pict *destPtr = clientData;
    PolygonSwitches switches;
    size_t numVertices;
    Point2d *vertices;
    Region2d r;
    Blt_PaintBrush brush;

    if (Blt_GetPaintBrush(interp, "black", &brush) != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    switches.brush = brush;
    if (Blt_ParseSwitches(interp, polygonSwitches, objc - 3, objv + 3, 
                &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (switches.x.numValues != switches.y.numValues) {
        Tcl_AppendResult(interp, "-x and -y coordinate lists must have the ",
                " same number of coordinates.",(char *)NULL);
        return TCL_ERROR;
    }
    vertices = NULL;
    r.top = r.left = FLT_MAX, r.bottom = r.right = -FLT_MAX;
    if (switches.x.numValues > 0) {
        size_t i;
        double *x, *y;

        numVertices = switches.x.numValues;
        vertices = Blt_Malloc(sizeof(Point2d) * (switches.x.numValues + 1));
        if (vertices == NULL) {
            Tcl_AppendResult(interp, "can't allocate memory for ", 
                Blt_Itoa(numVertices + 1), " vertices", (char *)NULL);
            return TCL_ERROR;
        }
        x = switches.x.values;
        y = switches.y.values;
        for (i = 0; i < switches.x.numValues; i++) {
            vertices[i].x = x[i];
            vertices[i].y = y[i];
            if (r.left > x[i]) {
                r.left = x[i];
            } else if (r.right < x[i]) {
                r.right = x[i];
            }
            if (r.top > y[i]) {
                r.top = y[i];
            } else if (r.bottom < y[i]) {
                r.bottom = y[i];
            }
        }
        if ((x[0] != x[i-1]) || (y[0] != y[i-1])) {
            vertices[i].x = x[0];
            vertices[i].y = y[0];
            numVertices++;
        }
        Blt_Free(switches.x.values);
        Blt_Free(switches.y.values);
        switches.x.values = switches.y.values = NULL;
    } else if (switches.coords.numValues > 0) {
        size_t i, j;
        double *coords;

        if (switches.coords.numValues & 0x1) {
            Tcl_AppendResult(interp, "bad -coords list: ",
                "must have an even number of values", (char *)NULL);
            return TCL_ERROR;
        }
        numVertices = (switches.coords.numValues / 2);
        vertices = Blt_Malloc(sizeof(Point2d)* (numVertices + 1));
        if (vertices == NULL) {
            Tcl_AppendResult(interp, "can't allocate memory for ", 
                Blt_Itoa(numVertices + 1), " vertices", (char *)NULL);
            return TCL_ERROR;
        }
        coords = switches.coords.values;
        for (i = 0, j = 0; i < switches.coords.numValues; i += 2, j++) {
            vertices[j].x = coords[i];
            vertices[j].y = coords[i+1];
            if (r.left > coords[i]) {
                r.left = coords[i];
            } else if (r.right < coords[i]) {
                r.right = coords[i];
            }
            if (r.top > coords[i+1]) {
                r.top = coords[i+1];
            } else if (r.bottom < coords[i+1]) {
                r.bottom = coords[i+1];
            }
        }
        if ((coords[0] != coords[i-2]) || (coords[1] != coords[i-1])) {
            vertices[j].x = coords[0];
            vertices[j].y = coords[1];
            numVertices++;
        }
        Blt_Free(switches.coords.values);
        switches.coords.values = NULL;
    }
    if (vertices != NULL) {
        if ((r.left < destPtr->width) && (r.right >= 0) &&
            (r.top < destPtr->height) && (r.bottom >= 0)) {
            if (switches.antialiased) {
                PaintPolygonAA2(destPtr, numVertices, vertices, &r, 
                        switches.brush, &switches.shadow);
            } else {
                if (switches.shadow.width > 0) {
                    PaintPolygonShadow(destPtr, numVertices, vertices, &r, 
                                       &switches.shadow);
                }
                Blt_SetBrushRegion(switches.brush, r.left, r.top, 
                                 r.right - r.left, r.bottom - r.top);
                Blt_PaintPolygon(destPtr, numVertices, vertices, 
                        switches.brush);
            }
        }
        Blt_Free(vertices);
    }
    Blt_FreeSwitches(polygonSwitches, (char *)&switches, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Picture_RectangleOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *      imageName draw rectangle x y w h ?switches ...?
 *      imageName draw rectangle x1 y1 x2 y2 ?switches ...?
 *      imageName draw rectangle -coords {x1 y1 x2 y2} ?switches ...?
 *      imageName draw rectangle -coords {x1 y1 x2 y2} ?switches ...?
 *---------------------------------------------------------------------------
 */
int
Blt_Picture_RectangleOp(ClientData clientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *const *objv)   
{
    Blt_Picture picture = clientData;
    RectangleSwitches switches;
    Blt_PaintBrush brush;
    int x, y;

    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
        return TCL_OK;
    }
    if (Blt_GetPaintBrush(interp, "black", &brush) != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    /* Process switches  */
    switches.brush = brush;
    switches.lineWidth = 0;
    switches.width = 10;
    switches.height = 10;
    if (Blt_ParseSwitches(interp, rectangleSwitches, objc - 5, objv + 5, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    Blt_SetBrushRegion(switches.brush, x, y, switches.width, switches.height);
    if (switches.shadow.width > 0) {
        PaintRectangleShadow(picture, x, y, switches.width, switches.height,
                switches.radius, switches.lineWidth, &switches.shadow);
    }
    Blt_PaintRectangle(picture, x, y, switches.width, switches.height,
        switches.radius, switches.lineWidth, switches.brush, TRUE);
    Blt_FreeSwitches(rectangleSwitches, (char *)&switches, 0);
    return TCL_OK;
}

Blt_Picture
Blt_PaintCheckbox(int w, int h, XColor *fillColorPtr, XColor *outlineColorPtr, 
                  XColor *checkColorPtr, int on)
{
    Blt_Shadow shadow;
    int x, y;
    Pict *destPtr;
    Blt_PaintBrush brush;

fprintf(stderr, "PaintCheckbox w=%d h=%d\n", w, h);
    destPtr = Blt_CreatePicture(w, h);
    Blt_Shadow_Set(&shadow, 1, 1, 0x0, 0xA0);
    brush = Blt_NewColorBrush(0x00000000);
    x = y = 0;
    destPtr->flags |= BLT_PIC_COMPOSITE;
    if (fillColorPtr != NULL) {
        Blt_Pixel color;

        color.u32 = Blt_XColorToPixel(fillColorPtr);
        Blt_SetColorBrushColor(brush, color.u32);
        Blt_PaintRectangle(destPtr, x+1, y+1, w-2, h-2, 0, 0, brush, TRUE);
        destPtr->flags &= ~BLT_PIC_COMPOSITE;
    }
    if (outlineColorPtr != NULL) {
        Blt_SetColorBrushColor(brush, Blt_XColorToPixel(outlineColorPtr));
        Blt_PaintRectangle(destPtr, x, y, w, h, 0, 1, brush, TRUE);
    }
    x += 1, y += 1;
    w -= 4, h -= 4;
    if (on) {
        Point2d points[7];
        Region2d r;
        double t, dt, dx, dy;
        double m1, m2;

        t = MAX(w,h) * 0.15;
        points[0].x = x;
        points[0].y = y + 0.6 * h;
        points[4].x = x + w;
        points[4].y = y + 0.2 * h;
        points[5].x = x + 0.4 * w;
        points[5].y = y + h;
        m1 = (points[4].y - points[5].y) / (points[4].x - points[5].x);
        dx = fabs(sin(m1) * t);
        dy = fabs(cos(m1) * t);
        dt = fabs(t / sin(M_PI - m1)) + 1;
        points[3].x = points[4].x - dx;
        points[3].y = points[4].y - dy;
        points[2].x = points[5].x;
        points[2].y = points[5].y - dt;
        m2 = (points[0].y - points[5].y) / (points[0].x - points[5].x);
        dx = fabs(sin(m2) * t);
        dy = fabs(cos(m2) * t);
        points[1].x = points[0].x + dx;
        points[1].y = points[0].y - dy;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        shadow.width = 2, shadow.offset = 2;
        shadow.color.u32 = 0x5F000000;
        r.left = x, r.right = x + w;
        r.top = y, r.bottom = y + h;
        Blt_SetColorBrushColor(brush, Blt_XColorToPixel(checkColorPtr));
        PaintPolygonAA2(destPtr, 7, points, &r, brush, &shadow);
    }
    Blt_FreeBrush(brush);
    destPtr->flags |= BLT_PIC_PREMULT_COLORS;
    return destPtr;
}

Blt_Picture
Blt_PaintRadioButtonOld(
     int w, int h, 
     XColor *bgColorPtr, 
     XColor *fillColorPtr, 
     XColor *outlineColorPtr, 
     XColor *indicatorColorPtr, 
     int on)
{
    Pict *destPtr;
    int x, y, r;
    Blt_Pixel fill, outline, bg;
    Blt_PaintBrush brush;
    Blt_Shadow shadow;

    /* Process switches  */
    brush = Blt_NewColorBrush(Blt_XColorToPixel(fillColorPtr));
    bg.u32 = Blt_XColorToPixel(bgColorPtr);
    Blt_Shadow_Set(&shadow, 1, 2, 0x0, 0xFF);
    w &= ~1;
    destPtr = Blt_CreatePicture(w, h);
    Blt_BlankPicture(destPtr, bg.u32);
    w -= 6, h -= 6;
    x = w / 2 + 1;
    y = h / 2 + 1;
    r = (w+1) / 2;
    if (shadow.width > 0) {
        DrawCircleShadow(destPtr, x, y, r, 0.0, TRUE, &shadow);
    }
    DrawCircle(destPtr, x, y, r, 0.0, brush, TRUE);
    if (fill.u32 != outline.u32) {
        Blt_SetColorBrushColor(brush, Blt_XColorToPixel(outlineColorPtr));
        DrawCircle(destPtr, x, y, r, 1.0, brush, TRUE);
    }
    if (on) {
        r -= 2;
        if (r < 1) {
            r = 2;
        }
        Blt_SetColorBrushColor(brush, Blt_XColorToPixel(indicatorColorPtr));
        DrawCircle(destPtr, x, y, r, 0.0, brush, TRUE);
    }
    Blt_FreeBrush(brush);
    return destPtr;
}


#define MAX_INTENSITY 255

static void
GetShadowColors(Blt_Bg bg, unsigned int *normalColorPtr, 
                unsigned int *lightColorPtr, unsigned int *darkColorPtr)
{
    int r, g, b;
    Blt_Pixel light, dark, normal;
    XColor *colorPtr;

    colorPtr = Blt_Bg_BorderColor(bg);
    normal.u32 = Blt_XColorToPixel(colorPtr);
    r = normal.Red;
    g = normal.Green;
    b = normal.Blue;

    dark.Alpha = 0xFF;
    if (r*0.5*r + g*1.0*g + b*0.28*b < MAX_INTENSITY*0.05*MAX_INTENSITY) {
        dark.Red = (MAX_INTENSITY + 3*r)/4;
        dark.Green = (MAX_INTENSITY + 3*g)/4;
        dark.Blue = (MAX_INTENSITY + 3*b)/4;
        dark.Alpha = 0xFF;
    } else {
        dark.Red = (50 * r)/100;
        dark.Green = (50 * g)/100;
        dark.Blue = (50 * b)/100;
        dark.Alpha = 0xFF;
    }
    /*
     * Compute the light shadow color
     */
    if (g > MAX_INTENSITY*0.95) {
        light.Red = (90 * r)/100;
        light.Green = (90 * g)/100;
        light.Blue = (90 * b)/100;
        light.Alpha = 0xFF;
    } else {
        int tmp1, tmp2;

        tmp1 = (14 * r)/10;
        if (tmp1 > MAX_INTENSITY) {
            tmp1 = MAX_INTENSITY;
        }
        tmp2 = (MAX_INTENSITY + r)/2;
        light.Red = (tmp1 > tmp2) ? tmp1 : tmp2;
        tmp1 = (14 * g)/10;
        if (tmp1 > MAX_INTENSITY) {
            tmp1 = MAX_INTENSITY;
        }
        tmp2 = (MAX_INTENSITY + g)/2;
        light.Green = (tmp1 > tmp2) ? tmp1 : tmp2;
        tmp1 = (14 * b)/10;
        if (tmp1 > MAX_INTENSITY) {
            tmp1 = MAX_INTENSITY;
        }
        tmp2 = (MAX_INTENSITY + b)/2;
        light.Blue = (tmp1 > tmp2) ? tmp1 : tmp2;
        light.Alpha = 0xFF;
    }
    *normalColorPtr = normal.u32;
    *lightColorPtr = light.u32;
    *darkColorPtr = dark.u32;
}


Blt_Picture
Blt_PaintRadioButton(
     int w, int h, 
     Blt_Bg bg, 
     XColor *fillColorPtr, 
     XColor *indicatorColorPtr, 
     int on)
{
    Pict *destPtr;
    int x, y, r;
    Blt_PaintBrush brush, newBrush;
    unsigned int normal, light, dark;

    destPtr = Blt_CreatePicture(w, h);
    w = Blt_Picture_Width(destPtr);
    h = Blt_Picture_Height(destPtr);

    /* Process switches  */
    newBrush = Blt_Bg_PaintBrush(bg);
    Blt_SetBrushRegion(newBrush, 0, 0, w, h); 
    Blt_PaintRectangle(destPtr, 0, 0, w, h, 0, 0, newBrush, TRUE);

    GetShadowColors(bg, &normal, &light, &dark);
    w &= ~1;
    x = w / 2 + 1;
    y = h / 2 + 1;
    w -= 4, h -= 4;
    r = (w+1) / 2;
    brush = Blt_NewColorBrush(dark);
    DrawCircle(destPtr, x-1, y-1, r, 0.0, brush, TRUE);
    Blt_SetColorBrushColor(brush, light);
    DrawCircle(destPtr, x+1, y+1, r, 0.0, brush, TRUE);
    /*Blt_BlurPicture(destPtr, destPtr, 1, 3); */
    Blt_SetColorBrushColor(brush, Blt_XColorToPixel(fillColorPtr));
    DrawCircle(destPtr, x, y, r, 0.0, brush, TRUE);
    if (on) {
        int r1;

        r1 = (r * 2) / 3;
        if (r1 < 1) {
            r1 = 2;
        }
        Blt_SetColorBrushColor(brush, Blt_XColorToPixel(indicatorColorPtr));
        DrawCircle(destPtr, x, y, r1, 0.0, brush, TRUE);
    }
    Blt_FreeBrush(brush);
    return destPtr;
}

Blt_Picture
Blt_PaintDelete(int w, int h, unsigned int fill, unsigned int symbol,
                int isActive)
{
    Blt_Picture picture;
    Point2d points[13];
    Region2d reg;
    double cx, cy, r, d, s;

    Blt_PaintBrush brush;

    brush = Blt_NewColorBrush(fill);
    reg.left = reg.top = 0;
    reg.right = w;
    reg.bottom = h;
    picture = Blt_CreatePicture(w, h);
    Blt_BlankPicture(picture, 0x00);
    cx = cy = w / 2;
    r = cx - 1;
    if (fill != 0x0) {
        DrawCircle(picture, cx, cy, r, 0.0, brush, FALSE);
    }
    r -= 2;
    d = r * 0.28;
    d = d / M_SQRT2;
    s = r * M_SQRT2 * 0.5;
    points[0].x = cx - s;
    points[0].y = cy - s + d; 
    points[1].x = cx - s + d;
    points[1].y = cy - s;
    points[2].x = cx;
    points[2].y = cy - d;
    points[3].x = cx + s - d;
    points[3].y = cy - s;
    points[4].x = cx + s;
    points[4].y = cy - s + d;
    points[5].x = cx + d;
    points[5].y = cy;
    points[6].x = cx + s;
    points[6].y = cy + s - d;
    points[7].x = cx + s - d;
    points[7].y = cy + s;
    points[8].x = cx;
    points[8].y = cy + d;
    points[9].x = cx - s + d;
    points[9].y = cy + s;
    points[10].x = cx - s;
    points[10].y = cy + s - d;
    points[11].x = cx - d;
    points[11].y = cy;
    points[12].x = points[0].x;
    points[12].y = points[0].y;

    Blt_SetColorBrushColor(brush, symbol);
    PaintPolygonAA2(picture, 13, points, &reg, brush, NULL);
    Blt_FreeBrush(brush);
    return picture;
}

void
Blt_PaintArrowHead(Blt_Picture picture, int x, int y, int w, int h,
                   unsigned int color, int direction)
{
    Point2d points[4];
    Region2d reg;
    Blt_PaintBrush brush;

    reg.left = reg.top = 0;
    reg.right = w;
    reg.bottom = h;
    switch (direction) {
    case ARROW_UP:
        points[0].x = x + 0.1 * w;
        points[0].y = y + 0.9 * h;
        points[1].x = x + 0.5 * w;
        points[1].y = y + 0.1 * h;
        points[2].x = x + 0.9 * w;
        points[2].y = y + 0.9 * h;
        points[3].x = points[0].x;
        points[3].y = points[0].y;
        break;
    case ARROW_DOWN:
        points[0].x = x + 0.1 * w;
        points[0].y = y + 0.1 * h;
        points[1].x = x + 0.9 * w;
        points[1].y = y + 0.1 * h;
        points[2].x = x + 0.5 * w;
        points[2].y = y + 0.9 * h;
        points[3].x = points[0].x;
        points[3].y = points[0].y;
        break;
    case ARROW_LEFT:
        points[0].x = x + 0.1 * w;
        points[0].y = y + 0.5 * h;
        points[1].x = x + 0.9 * w;
        points[1].y = y + 0.1 * h;
        points[2].x = x + 0.9 * w;
        points[2].y = y + 0.9 * h;
        points[3].x = points[0].x;
        points[3].y = points[0].y;
        break;
    case ARROW_RIGHT:
        points[0].x = x + 0.1 * w;
        points[0].y = y + 0.1 * h;
        points[1].x = x + 0.9 * w;
        points[1].y = y + 0.5 * h;
        points[2].x = x + 0.1 * w;
        points[2].y = y + 0.9 * h;
        points[3].x = points[0].x;
        points[3].y = points[0].y;
        break;
    }
    brush = Blt_NewColorBrush(color);
    PaintPolygonAA2(picture, 4, points, &reg, brush, NULL);
    Blt_FreeBrush(brush);
}

void
Blt_PaintArrowHead2(Blt_Picture picture, int x, int y, int w, int h,
                   unsigned int color, int direction)
{
    Point2d points[7];
    Region2d reg;
    Blt_PaintBrush brush;
    double dx, dy, m1, m2, dt, t;
    
    reg.left = reg.top = 0;
    reg.right = w;
    reg.bottom = h;
    fprintf(stderr, "w=%d h=%d\n", w, h);
    switch (direction) {
    case ARROW_UP:
        t = w * 0.20;
        y--;
        points[0].x = x + 0.1 * w;
        points[0].y = y + 0.8 * h;
        points[1].x = x + 0.5 * w;
        points[1].y = y + 0.1 * h;
        points[2].x = x + 0.9 * w;
        points[2].y = y + 0.8 * h;
        m1 = (points[1].y - points[2].y) / (points[1].x - points[2].x);
        fprintf(stderr, "u dx=%g dy=%g\n", 
                (points[1].x - points[2].x), (points[1].y - points[2].y));
        dx = fabs(sin(m1) * t);
        dy = fabs(cos(m1) * t);
        fprintf(stderr, "u m1=%g dx=%g dy=%g\n", m1, dx, dy);
        points[3].x = points[2].x - dx;
        points[3].y = points[2].y + dy;
        dt = fabs(t / sin(M_PI - m1)) + 1;
        fprintf(stderr, "u t=%g dt=%g\n", t, dt);
        points[4].x = points[1].x;
        points[4].y = points[1].y + dt;
        m2 = (points[0].y - points[1].y) / (points[0].x - points[1].x);
        dx = fabs(sin(m2) * t);
        dy = fabs(cos(m2) * t);
        fprintf(stderr, "u dx=%g dy=%g\n", dx, dy);
        points[5].x = points[0].x + dx;
        points[5].y = points[0].y + dy;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    case ARROW_DOWN:
        t = w * 0.20;
        points[0].x = x + 0.9 * w;
        points[0].y = y + 0.2 * h;
        points[1].x = x + 0.5 * w;
        points[1].y = y + 0.9 * h;
        points[2].x = x + 0.1 * w;
        points[2].y = y + 0.2 * h;
        m1 = (points[1].y - points[2].y) / (points[1].x - points[2].x);
        dx = fabs(sin(m1) * t);
        dy = fabs(cos(m1) * t);
        fprintf(stderr, "v m1=%g dx=%g dy=%g\n", m1, dx, dy);
        points[3].x = points[2].x + dx;
        points[3].y = points[2].y - dy;
        dt = fabs(t / sin(M_PI - m1)) + 1;
        fprintf(stderr, "v t=%g dt=%g\n", t, dt);
        points[4].x = points[1].x;
        points[4].y = points[1].y - dt;
        m2 = (points[0].y - points[1].y) / (points[0].x - points[1].x);
        dx = fabs(sin(m2) * t);
        dy = fabs(cos(m2) * t);
        fprintf(stderr, "v dx=%g dy=%g\n", dx, dy);
        points[5].x = points[0].x - dx;
        points[5].y = points[0].y - dy;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    case ARROW_LEFT:
        t = h * 0.20;
        x--;
        points[0].x = x + 0.8 * w;
        points[0].y = y + 0.1 * h;
        points[1].x = x + 0.1 * w;
        points[1].y = y + 0.5 * h;
        points[2].x = x + 0.8 * w;
        points[2].y = y + 0.9 * h;
        m1 = (points[1].y - points[2].y) / (points[1].x - points[2].x);
        dx = sin(m1) * t;
        dy = cos(m1) * t;
        points[3].x = points[2].x + dx;
        points[3].y = points[2].y - dy;
        dt = t / sin(-m1);
        points[4].x = points[1].x - dt + 1;
        points[4].y = points[1].y;
        m2 = (points[0].y - points[1].y) / (points[0].x - points[1].x);
        dx = sin(-m2) * t;
        dy = cos(-m2) * t;
        points[5].x = points[0].x + dx;
        points[5].y = points[0].y + dy;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    case ARROW_RIGHT:
        t = h * 0.20;
        x++;
        points[2].x = x + 0.2 * w;
        points[2].y = y + 0.9 * h;
        points[1].x = x + 0.9 * w;
        points[1].y = y + 0.5 * h;
        points[0].x = x + 0.2 * w;
        points[0].y = y + 0.1 * h;
        m1 = (points[1].y - points[2].y) / (points[1].x - points[2].x);
        dx = sin(m1) * t;
        dy = cos(m1) * t;
        points[3].x = points[2].x + dx;
        points[3].y = points[2].y - dy;
        dt = t / sin(-m1);
        points[4].x = points[1].x - dt - 1;
        points[4].y = points[1].y;
        m2 = (points[0].y - points[1].y) / (points[0].x - points[1].x);
        dx = sin(-m2) * t;
        dy = cos(-m2) * t;
        points[5].x = points[0].x + dx;
        points[5].y = points[0].y + dy;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    }
    brush = Blt_NewColorBrush(color);
    {
        int i;
        
        for (i = 0; i < 7; i++) {
            fprintf(stderr, "points[%d] = %g,%g\n", i, points[i].x, points[i].y);
        }
    }
    PaintPolygonAA2(picture, 7, points, &reg, brush, NULL);
    Blt_FreeBrush(brush);
    Blt_Picture_SetCompositeFlag(picture);
}

void
Blt_PaintArrowHead3(Blt_Picture picture, int x, int y, int w, int h,
                   unsigned int color, int direction)
{
    Point2d points[7];
    Region2d reg;
    Blt_PaintBrush brush;
    double dx, dy, m1, m2, dt, t;
    
    reg.left = reg.top = 0;
    reg.right = w;
    reg.bottom = h;
    fprintf(stderr, "w=%d h=%d\n", w, h);
    switch (direction) {
    case ARROW_UP:
        t = w * 0.20;
        y--;
        points[0].x = x + 0.1 * w;
        points[0].y = y + 0.8 * h;
        points[1].x = x + 0.5 * w;
        points[1].y = y + 0.1 * h;
        points[2].x = x + 0.9 * w;
        points[2].y = y + 0.8 * h;
        m1 = (points[1].y - points[2].y) / (points[1].x - points[2].x);
        fprintf(stderr, "u dx=%g dy=%g\n", 
                (points[1].x - points[2].x), (points[1].y - points[2].y));
        dx = fabs(sin(m1) * t);
        dy = fabs(cos(m1) * t);
        fprintf(stderr, "u m1=%g dx=%g dy=%g\n", m1, dx, dy);
        points[3].x = points[2].x - dx;
        points[3].y = points[2].y + dy;
        dt = fabs(t / sin(M_PI - m1)) + 1;
        fprintf(stderr, "u t=%g dt=%g\n", t, dt);
        points[4].x = points[1].x;
        points[4].y = points[1].y + dt;
        m2 = (points[0].y - points[1].y) / (points[0].x - points[1].x);
        dx = fabs(sin(m2) * t);
        dy = fabs(cos(m2) * t);
        fprintf(stderr, "u dx=%g dy=%g\n", dx, dy);
        points[5].x = points[0].x + dx;
        points[5].y = points[0].y + dy;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    case ARROW_DOWN:
        t = w * 0.20;
        points[0].x = x + 0.9 * w;
        points[0].y = y + 0.2 * h;
        points[1].x = x + 0.5 * w;
        points[1].y = y + 0.9 * h;
        points[2].x = x + 0.1 * w;
        points[2].y = y + 0.2 * h;
        m1 = (points[1].y - points[2].y) / (points[1].x - points[2].x);
        dx = fabs(sin(m1) * t);
        dy = fabs(cos(m1) * t);
        fprintf(stderr, "v m1=%g dx=%g dy=%g\n", m1, dx, dy);
        points[3].x = points[2].x + dx;
        points[3].y = points[2].y - dy;
        dt = fabs(t / sin(M_PI - m1)) + 1;
        fprintf(stderr, "v t=%g dt=%g\n", t, dt);
        points[4].x = points[1].x;
        points[4].y = points[1].y - dt;
        m2 = (points[0].y - points[1].y) / (points[0].x - points[1].x);
        dx = fabs(sin(m2) * t);
        dy = fabs(cos(m2) * t);
        fprintf(stderr, "v dx=%g dy=%g\n", dx, dy);
        points[5].x = points[0].x - dx;
        points[5].y = points[0].y - dy;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    case ARROW_LEFT:
        t = h * 0.20;
        x--;
        points[0].x = x + 0.8 * w;
        points[0].y = y + 0.1 * h;
        points[1].x = x + 0.1 * w;
        points[1].y = y + 0.5 * h;
        points[2].x = x + 0.8 * w;
        points[2].y = y + 0.9 * h;
        m1 = (points[1].y - points[2].y) / (points[1].x - points[2].x);
        dx = sin(m1) * t;
        dy = cos(m1) * t;
        points[3].x = points[2].x + dx;
        points[3].y = points[2].y - dy;
        dt = t / sin(-m1);
        points[4].x = points[1].x - dt + 1;
        points[4].y = points[1].y;
        m2 = (points[0].y - points[1].y) / (points[0].x - points[1].x);
        dx = sin(-m2) * t;
        dy = cos(-m2) * t;
        points[5].x = points[0].x + dx;
        points[5].y = points[0].y + dy;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    case ARROW_RIGHT:
        t = h * 0.20;
        x++;
        points[2].x = x + 0.2 * w;
        points[2].y = y + 0.9 * h;
        points[1].x = x + 0.9 * w;
        points[1].y = y + 0.5 * h;
        points[0].x = x + 0.2 * w;
        points[0].y = y + 0.1 * h;
        m1 = (points[1].y - points[2].y) / (points[1].x - points[2].x);
        dx = sin(m1) * t;
        dy = cos(m1) * t;
        points[3].x = points[2].x + dx;
        points[3].y = points[2].y - dy;
        dt = t / sin(-m1);
        points[4].x = points[1].x - dt - 1;
        points[4].y = points[1].y;
        m2 = (points[0].y - points[1].y) / (points[0].x - points[1].x);
        dx = sin(-m2) * t;
        dy = cos(-m2) * t;
        points[5].x = points[0].x + dx;
        points[5].y = points[0].y + dy;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    }
    brush = Blt_NewColorBrush(color);
    {
        int i;
        
        for (i = 0; i < 7; i++) {
            fprintf(stderr, "points[%d] = %g,%g\n", i, points[i].x, points[i].y);
        }
    }
    PaintPolygonAA2(picture, 7, points, &reg, brush, NULL);
    Blt_FreeBrush(brush);
    Blt_Picture_SetCompositeFlag(picture);
}


void
Blt_PaintChevron(Blt_Picture picture, int x, int y, int w, int h,
                 unsigned int color, int direction)
{
    Point2d points[7];
    Region2d reg;
    Blt_PaintBrush brush;
    double t;
    Blt_Shadow shadow;
    
    reg.left = reg.top = 0;
    reg.right = w;
    reg.bottom = h;
    t = w * 0.25;
    switch (direction) {
    case ARROW_UP:
        t = w * 0.25;
        points[0].x = x + 0.1 * w;
        points[0].y = y + 0.9 * h - t;
        points[1].x = x + 0.5 * w;
        points[1].y = y + 0.1 * h;
        points[2].x = x + 0.9 * w;
        points[2].y = y + 0.9 * h - t;
        points[3].x = x + 0.9 * w;
        points[3].y = y + 0.9 * h;
        points[4].x = x + 0.5 * w;
        points[4].y = y + 0.1 * h + t;
        points[5].x = x + 0.1 * w;
        points[5].y = y + 0.9 * h;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    case ARROW_DOWN:
        points[0].x = x + 0.9 * w;
        points[0].y = y + 0.1 * h + t;
        points[1].x = x + 0.5 * w;
        points[1].y = y + 0.9 * h;
        points[2].x = x + 0.1 * w;
        points[2].y = y + 0.1 * h + t;
        points[3].x = x + 0.1 * w;
        points[3].y = y + 0.1 * h;
        points[4].x = x + 0.5 * w;
        points[4].y = y + 0.9 * h - t;
        points[5].x = x + 0.9 * w;
        points[5].y = y + 0.1 * h;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    case ARROW_LEFT:
        t = h * 0.25;
        /*x -= 2; */
        points[0].x = x + 0.9 * w - t;
        points[0].y = y + 0.1 * h;
        points[1].x = x + 0.1 * w;
        points[1].y = y + 0.5 * h;
        points[2].x = x + 0.9 * w - t;
        points[2].y = y + 0.9 * h;
        points[3].x = x + 0.9 * w;
        points[3].y = y + 0.9 * h;
        points[4].x = x + 0.1 * w + t;
        points[4].y = y + 0.5 * h;
        points[5].x = x + 0.9 * w;
        points[5].y = y + 0.1 * h;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    case ARROW_RIGHT:
        /* x++; */
        t = h * 0.25;
        points[0].x = x + 0.1 * w + t;
        points[0].y = y + 0.1 * h;
        points[1].x = x + 0.9 * w;
        points[1].y = y + 0.5 * h;
        points[2].x = x + 0.1 * w + t;
        points[2].y = y + 0.9 * h;
        points[3].x = x + 0.1 * w;
        points[3].y = y + 0.9 * h;
        points[4].x = x + 0.9 * w - t;
        points[4].y = y + 0.5 * h;
        points[5].x = x + 0.1 * w;
        points[5].y = y + 0.1 * h;
        points[6].x = points[0].x;
        points[6].y = points[0].y;
        break;
    }
    brush = Blt_NewColorBrush(color);
#ifdef notdef
        shadow.width = 2, shadow.offset = 2;
        shadow.color.u32 = 0x5F000000;
    PaintPolygonAA2(picture, 7, points, &reg, brush, &shadow);
#endif
    PaintPolygonAA2(picture, 7, points, &reg, brush, NULL);
    Blt_FreeBrush(brush);
    Blt_Picture_SetCompositeFlag(picture);
}

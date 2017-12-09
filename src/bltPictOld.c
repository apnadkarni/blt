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
#include <string.h>

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
    float lineWidth;                    /* Width of outline.  If zero,
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
    {BLT_SWITCH_FLOAT, "-linewidth", "value", (char *)NULL,
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
    float lineWidth;                    /* Line width of outline.  If zero,
                                         * indicates to draw a solid
                                         * circle. */
    int blend;
} CircleSwitches2;

#ifdef notdef
static Blt_SwitchSpec circleSwitches2[] = 
{
    {BLT_SWITCH_CUSTOM, "-color", "color", (char *)NULL,
        Blt_Offset(CircleSwitches2, brush),    0, 0, &paintbrushSwitch},
    {BLT_SWITCH_CUSTOM, "-fill", "fill", (char *)NULL,
        Blt_Offset(CircleSwitches2, fill),    0, 0, &colorSwitch},
    {BLT_SWITCH_CUSTOM, "-outline", "outline", (char *)NULL,
        Blt_Offset(CircleSwitches2, outline),    0, 0, &colorSwitch},
    {BLT_SWITCH_BOOLEAN, "-antialiased", "bool", (char *)NULL,
        Blt_Offset(CircleSwitches2, antialiased), 0},
    {BLT_SWITCH_BOOLEAN, "-blend", "bool", (char *)NULL,
        Blt_Offset(CircleSwitches2, blend), 0},
    {BLT_SWITCH_FLOAT, "-linewidth", "value", (char *)NULL,
        Blt_Offset(CircleSwitches2, lineWidth), 0},
    {BLT_SWITCH_CUSTOM, "-shadow", "offset", (char *)NULL,
        Blt_Offset(CircleSwitches2, shadow), 0, 0, &shadowSwitch},
    {BLT_SWITCH_END}
};
#endif

typedef struct {
    Blt_Pixel fill, outline;     /* Outline and fill colors for the circle. */
    Blt_Shadow shadow;
    int antialiased;
    int lineWidth;                      /* Width of outline.  If zero,
                                         * indicates to draw a solid circle. */
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
} RectangleSwitches;

static Blt_SwitchSpec rectangleSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-color", "color", (char *)NULL,
        Blt_Offset(RectangleSwitches, brush),    0, 0, &paintbrushSwitch},
    {BLT_SWITCH_BOOLEAN, "-antialiased", "bool", (char *)NULL,
        Blt_Offset(RectangleSwitches, antialiased), 0},
    {BLT_SWITCH_INT_NNEG, "-radius", "number", (char *)NULL,
        Blt_Offset(RectangleSwitches, radius), 0},
    {BLT_SWITCH_CUSTOM, "-shadow", "offset", (char *)NULL,
        Blt_Offset(RectangleSwitches, shadow), 0, 0, &shadowSwitch},
    {BLT_SWITCH_INT_NNEG, "-linewidth", "number", (char *)NULL,
        Blt_Offset(RectangleSwitches, lineWidth), 0}, 
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

#ifdef notdef
/*
 *
 *  x,y------+
 *   |       |
 *   |  *----+------------+
 *   |  |    |            |
 *   |  |    |            |
 */  

static void
MergePictures(Pict *destPtr, Pict *srcPtr)
{
    Blt_Pixel *srcRowPtr, *destRowPtr;
    int y;
    int w, h;
    
    if (srcPtr->flags & BLT_PIC_ASSOCIATED_COLORS) {
        Blt_UnassociateColors(srcPtr);
    }
    if (destPtr->flags & BLT_PIC_ASSOCIATED_COLORS) {
        Blt_UnassociateColors(destPtr);
    }
    
    destRowPtr = destPtr->bits;
    srcRowPtr  = srcPtr->bits;
    w = srcPtr->width;
    h = srcPtr->height;
    for (y = 0; y < h; y++) {
        Blt_Pixel *sp, *dp, *dend;

        sp = srcRowPtr;
        for (dp = destRowPtr, dend = dp + w; dp < dend; dp++, sp++) {
            /* Blend the foreground and background together. */
            if (sp->Alpha == 0xFF) {
                *dp = *sp;
            } else if (sp->Alpha != 0x00) {
                int t1, t2;
                int r, g, b, a;

                r = imul8x8(sp->Alpha, sp->Red, t1) +
                    imul8x8(dp->Alpha, dp->Red, t2);
                g = imul8x8(sp->Alpha, sp->Green, t1) +
                    imul8x8(dp->Alpha, dp->Green, t2);
                b = imul8x8(sp->Alpha, sp->Blue, t1) +
                    imul8x8(dp->Alpha, dp->Blue, t2);
                a = imul8x8(sp->Alpha, sp->Alpha, t1) + 
                    imul8x8(dp->Alpha, dp->Alpha, t2);
                dp->Red =   (r > 255) ? 255 : ((r < 0) ? 0 : r);
                dp->Green = (g > 255) ? 255 : ((g < 0) ? 0 : g);
                dp->Blue =  (b > 255) ? 255 : ((b < 0) ? 0 : b);
                dp->Alpha = (a > 255) ? 255 : ((a < 0) ? 0 : a);
            }
        }
        srcRowPtr += srcPtr->pixelsPerRow;
        destRowPtr += destPtr->pixelsPerRow;

}
#endif
    
#ifdef notdef
static void
MarkPicture(Pict *srcPtr)
{
    Blt_Pixel *srcRowPtr;
    int y;

    srcRowPtr = srcPtr->bits;
    for (y = 0; y < srcPtr->height; y++) {
        Blt_Pixel *sp, *send;

        for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; sp++) {
            if (sp->Alpha != 0x0) {
                sp->Alpha = 0xFF;
            }
        }
        srcRowPtr += srcPtr->pixelsPerRow;
    }
}
#endif
 
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
 *      Convert a Tcl_Obj list of numbers into an array of floats.
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
    float *values;
    int i;
    int objc;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    values = Blt_Malloc(sizeof(float) * objc);
    if (values == NULL) {
        Tcl_AppendResult(interp, "can't allocated coordinate array of ",
                Blt_Itoa(objc), " elements", (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
        double x;

        if (Tcl_GetDoubleFromObj(interp, objv[i], &x) != TCL_OK) {
            Blt_Free(values);
            return TCL_ERROR;
        }
        values[i] = (float)x;
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

#include "bltPaintDraw.c"

#ifdef notdef
static void 
PaintLineSegment2(
    Pict *destPtr,
    int x1, int y1, 
    int x2, int y2, 
    int cw,
    Blt_Pixel *colorPtr)
{
    Blt_Pixel interior;
    int dx, dy, xDir;
    unsigned long error;

    if (y1 > y2) {
        int tmp;

        tmp = y1, y1 = y2, y2 = tmp;
        tmp = x1, x1 = x2, x2 = tmp;
        cw = !cw;
    } 
    if (x1 > x2) {
        cw = !cw;
    }
    interior = PremultiplyAlpha(colorPtr, 255);
    /* First and last Pixels always get Set: */
    PutPixel(destPtr, x1, y1, &interior);
    PutPixel(destPtr, x2, y2, &interior);

    dx = x2 - x1;
    dy = y2 - y1;

    if (dx >= 0) {
        xDir = 1;
    } else {
        xDir = -1;
        dx = -dx;
    }
    if (dx == 0) {                      /*  Vertical line */
        VertLine(destPtr, x1, y1, y2, &interior);
        return;
    }
    if (dy == 0) {                      /* Horizontal line */
        HorizLine(destPtr, x1, x2, y1, &interior);
        return;
    }
    if (dx == dy) {                     /* Diagonal line. */
        Blt_Pixel *dp;

        dp = Blt_Picture_Pixel(destPtr, x1, y1);
        while(dy-- > 0) {
            dp += destPtr->pixelsPerRow + xDir;
            dp->u32 = interior.u32;
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
            Blt_Pixel *dp;
            int x;
            unsigned char weight;
            
            error += adjust;
            ++y1;
            if (error & ~0xFFFF) {
                x1 += xDir;
                error &= 0xFFFF;
            }
            dp = Blt_Picture_Pixel(destPtr, x1, y1);
            weight = (unsigned char)(error >> 8);
            x = x1;
            if (x >= 0) {
                if (cw) {
                    *dp = PremultiplyAlpha(colorPtr, weight ^ 0xFF);
                } else {
                    *dp = interior;
                }
            }
            x += xDir;
            dp += xDir;
            if (x >= 0) {
                if (!cw) {
                    *dp = PremultiplyAlpha(colorPtr, weight);
                } else {
                    *dp = interior;
                }
            }
        }
    } else {                            /* x-major line */
        unsigned long adjust;

        /* y1 -= lineWidth / 2; */
        adjust = (dy << 16) / dx;
        while (--dx) {
            Blt_Pixel *dp;
            int y;
            unsigned char weight;

            error += adjust;
            x1 += xDir;
            if (error & ~0xFFFF) {
                y1++;
                error &= 0xFFFF;
            }
            dp = Blt_Picture_Pixel(destPtr, x1, y1);
            weight = (unsigned char)(error >> 8);
            y = y1;
            if (y >= 0) {
                if (!cw) {
                    *dp = PremultiplyAlpha(colorPtr, weight ^ 0xFF);
                } else {
                    *dp = interior;
                }
            }
            dp += destPtr->pixelsPerRow;
            y++;
            if (y >= 0) {
                if (cw) {
                    *dp = PremultiplyAlpha(colorPtr, weight);
                } else {
                    *dp = interior;
                }
            } 
        }
    }
    destPtr->flags |= (BLT_PIC_BLEND | BLT_PIC_ASSOCIATED_COLORS);
}
#endif

static void 
PaintPolyline(
    Pict *destPtr,
    int numPoints, 
    Point2f *points, 
    int lineWidth,
    Blt_Pixel *colorPtr)
{
    int i;
    Region2d r;
    Point2f p;

    r.left = r.top = 0;
    r.right = destPtr->width - 1;
    r.bottom = destPtr->height - 1;
    p.x = points[0].x, p.y = points[0].y;
    for (i = 1; i < numPoints; i++) {
        Point2f q, next;

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

#ifdef notdef
static void 
xPaintArc(Pict *destPtr, int x1, int y1, int x2, int y2, int lineWidth, 
          Blt_Pixel *colorPtr)
{
    Blt_Pixel *dp;
    double t;
    int r2;
    int radius;
    int dx, dy;
    int x, y;
    int xoff, yoff;
    int fill = 1;

    t = 0.0;
    dx = x2 - x1;
    dy = y2 - y1;
    radius = MIN(dx, dy) / 2;
    xoff = x1;
    yoff = y1;
    x = radius;
    y = 0;
    dp = Blt_Picture_Pixel(destPtr, x + xoff - 1, y + yoff);
    dp->u32 = colorPtr->u32;
    r2 = radius * radius;
    if (fill) {
        PaintLineSegment(destPtr, x1, y + yoff, x + xoff - 2, y + yoff, 1, 
                         colorPtr);
    }
    while (x > y) {
        double z;
        double d, q;
        unsigned char a;
        Blt_Pixel color;
        y++;
        z = sqrt(r2 - (y * y));
        d = floor(z) - z;
        if (d < t) {
            x--;
        }
        dp = Blt_Picture_Pixel(destPtr, x + xoff, y + yoff);
        q = FABS(d * 255.0);
        a = (unsigned int)CLAMP(q);
        color.u32 = colorPtr->u32;
        Blt_FadeColor(&color, a);
        BlendPixels(dp, &color);
        dp--;                   /* x - 1 */
        a = (unsigned int)CLAMP(255.0 - q);
        color.u32 = colorPtr->u32;
        Blt_FadeColor(&color, a);
        BlendPixels(dp, &color);
        t = d;
        x1++;
        if (fill) {
            PaintLineSegment(destPtr, x1, y + yoff, x + xoff - 1, y + yoff, 1, colorPtr);
        }
    }
}
#endif

#ifdef notdef
static Point2d
PolygonArea(int numPoints, Point2d *points, double *areaPtr)
{
    Point2d *p, *pend;
    Point2d c;
    double area;
    int i;
    
    area = c.x = c.y = 0.0;
    for (p = points, pend = p + numPoints, i = 0; p < pend; p++, i++) {
        Point2d *q;
        double factor;
        int j;
        
        j = (i + 1) % numPoints;
        q = points + j;
        factor = (p->x * q->y) - (p->y * q->x);
        area += factor;
        c.x += (p->x + q->x) * factor;
        c.y += (p->y + q->y) * factor;
    }
    area *= 0.5;
    c.x /= 6.0 * area;
    c.y /= 6.0 * area;
    *areaPtr = area;
    return c;
}
#endif

#ifdef notdef
static void
BlendLine(Pict *destPtr, int x1, int x2, int y, Blt_Pixel *colorPtr)  
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
        BlendPixels(dp, colorPtr);
    }
}
#endif

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
    for (x = x1; x <= x2; x++, dp++) {
        Blt_Pixel color;

        color.u32 = Blt_GetAssociatedColorFromBrush(brush, x, y);
        BlendPixels(dp, &color);
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
static Point2f *pt;                     /* vertices */

static int 
CompareIndices(const void *a, const void *b)
{
    return (pt[*(int *)a].y <= pt[*(int *)b].y) ? -1 : 1;
}

static int 
CompareActive(const void *a, const void *b)
{
    const ActiveEdge *u, *v;

    u = a, v = b;
    return (u->x <= v->x) ? -1 : 1;
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
cinsert(AET *tablePtr, size_t n, Point2f *points, int i, int y)
{
    int j;
    Point2f *p, *q;
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
Blt_PaintPolygon(Pict *destPtr, int numVertices, Point2f *vertices, 
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
GetPolygonBoundingBox(size_t numVertices, Point2f *vertices, 
                      Region2f *regionPtr)
{
    Point2f *pp, *pend;

    regionPtr->left = regionPtr->top = FLT_MAX;
    regionPtr->right = regionPtr->bottom = -FLT_MAX;
    for (pp = vertices, pend = pp + numVertices; pp < pend; pp++) {
        if (pp->x < regionPtr->left) {
            regionPtr->left = pp->x;
        } else if (pp->x > regionPtr->right) {
            regionPtr->right = pp->x;
        }
        if (pp->y < regionPtr->top) {
            regionPtr->top = pp->y;
        } else if (pp->y > regionPtr->bottom) {
            regionPtr->bottom = pp->y;
        }
    }
}

static void
TranslatePolygon(size_t numVertices, Point2f *vertices, float x, float y, 
                 float scale)
{
    Point2f *pp, *pend;

    for (pp = vertices, pend = pp + numVertices; pp < pend; pp++) {
        pp->x = (pp->x + x) * scale;
        pp->y = (pp->y + y) * scale;
    }
}

#ifdef notdef
static void
PaintPolygonAA(Pict *destPtr, size_t numVertices, Point2f *vertices, 
               Region2f *regionPtr, Blt_PaintBrush brush)
{
    Blt_Picture big, tmp;
    Point2f *v;
    Region2f r2;
    int w, h;

    int x1, x2, y1, y2;

    x1 = y1 = 0;
    x2 = destPtr->width, y2 = destPtr->height;
    if (regionPtr->left > 0) {
        x1 = (int)floor(regionPtr->left);
    }
    if (regionPtr->top > 0) {
        y1 = (int)floor(regionPtr->top);
    }
    if (regionPtr->right < x2) {
        x2 = (int)ceil(regionPtr->right);
    }
    if (regionPtr->bottom < y2) {
        y2 = (int)ceil(regionPtr->bottom);
    }
    v = Blt_AssertMalloc(numVertices * sizeof(Point2f));
    memcpy(v, vertices, sizeof(Point2f) * numVertices);
    TranslatePolygon(numVertices, v, -x1+1, -y1+1, 4.0f);
    GetPolygonBoundingBox(numVertices, v, &r2);
    
    w = (x2 - x1 + 2) * 4;
    h = (y2 - y1 + 2) * 4;
    big = Blt_CreatePicture(w, h);
    Blt_BlankPicture(big, 0x0);
    Blt_PaintPolygon(big, numVertices, v, brush);
    Blt_Free(v);
    w = (x2 - x1 + 2);
    h = (y2 - y1 + 2);
    tmp = Blt_CreatePicture(w, h);
    Blt_ResamplePicture(tmp, big, bltBoxFilter, bltBoxFilter);
    Blt_FreePicture(big);
    /* Replace the bounding box in the original with the new. */
    Blt_BlendRegion(destPtr, tmp, 0, 0, w, h, (int)floor(regionPtr->left)-1, 
                    (int)floor(regionPtr->top)-1);
    Blt_FreePicture(tmp);
}
#endif

static void
PaintPolygonShadow(Pict *destPtr, size_t numVertices, Point2f *vertices, 
                   Region2f *regionPtr, Blt_Shadow *shadowPtr)
{
    Blt_PaintBrush brush;
    Blt_Picture blur, tmp;
    Point2f *v;
    Region2f r2;
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
        v = Blt_AssertMalloc(numVertices * sizeof(Point2f));
        memcpy(v, vertices, sizeof(Point2f) * numVertices);
        TranslatePolygon(numVertices, v, -x1, -y1, 1.0f);
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
    Blt_CopyArea(blur, tmp, 0, 0, w, h, shadowPtr->offset*2, 
                   shadowPtr->offset*2); 
    Blt_BlurPicture(blur, blur, shadowPtr->width, 3);
    Blt_MaskPicture(blur, tmp, 0, 0, w, h, 0, 0, &shadowPtr->color);
    Blt_FreePicture(tmp);
    Blt_BlendRegion(destPtr, blur, 0, 0, w, h, x1, y1);
    Blt_FreePicture(blur);
}

static void
PaintPolygonAA2(Pict *destPtr, size_t numVertices, Point2f *vertices, 
                Region2f *regionPtr, Blt_PaintBrush brush, 
                Blt_Shadow *shadowPtr)
{
    Blt_Picture big, tmp;
    Region2f r2;
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
    TranslatePolygon(numVertices, vertices, 0.0f, 0.0f, 4.0f);
    Blt_BlankPicture(big, 0x0);
    GetPolygonBoundingBox(numVertices, vertices, &r2);
    if ((shadowPtr != NULL) && (shadowPtr->width > 0)) {
        PaintPolygonShadow(big, numVertices, vertices, &r2, shadowPtr);
    }
    Blt_PaintPolygon(big, numVertices, vertices, brush);
    tmp = Blt_CreatePicture(destPtr->width, destPtr->height);
    Blt_ResamplePicture(tmp, big, bltBoxFilter, bltBoxFilter);
    Blt_FreePicture(big);
    Blt_BlendRegion(destPtr, tmp, 0, 0, destPtr->width, destPtr->height, 0,0);
    Blt_FreePicture(tmp);
}

#ifdef notdef
static void
DrawCircle2(Blt_Picture picture, int x, int y, int radius, 
            CircleSwitches *switchesPtr)
{
    int filled;

    filled = (switchesPtr->brush != NULL);
    if (switchesPtr->antialiased) {
        int numSamples = 4; 
        Pict *bigPtr, *tmpPtr;
        int w, h;
        Blt_Pixel color;
        int offset, r, lw;
        int cx, cy;

        r = radius * numSamples;
        w = h = r + r;
        offset = switchesPtr->shadow.offset * numSamples;

        /* Scale the region forming the bounding box of the ellipse into a
         * new picture. The bounding box is scaled by *nSamples* times. */
        bigPtr = Blt_CreatePicture(w+(1+offset)*nSamples,h+(1+offset)*nSamples);

        cx = bigPtr->width / 2;
        cy = bigPtr->height / 2;
        
        Blt_BlankPicture(bigPtr, 0x0);
        if (switchesPtr->shadow.width > 0) {
            color.u32 = switchesPtr->shadow.color.u32;
            /* Either ring or full circle for blur stencil. */
            lw = switchesPtr->lineWidth * numSamples;
            if (filled) {
                lw = 0;
            }
            PaintEllipse(bigPtr, cx, cy, r, r, lw, &color, 0);
            Blt_BlurPicture(bigPtr, bigPtr, offset/2, 3);
            /* Offset the circle from the shadow. */
            cx -= offset;
            cy -= offset;
            offset = switchesPtr->shadow.offset;
            w = h = radius + radius + (1 + offset) * 2;
            tmpPtr = Blt_CreatePicture(w, h);
            Blt_ResamplePicture(tmpPtr, bigPtr, bltBoxFilter, bltBoxFilter);
            Blt_BlendRegion(picture, tmpPtr, 0, 0, w, h, x-w/2+offset, 
                y-h/2+offset);
            Blt_BlankPicture(bigPtr, 0x0);
            Blt_FreePicture(tmpPtr);
        }
        lw = switchesPtr->lineWidth * numSamples;
        if ((lw > 0) && (r > lw) && (switchesPtr->outline.u32 != 0x00)) {
            /* Paint ring outline. */
            PaintEllipse(bigPtr, cx, cy, r, r, lw, &switchesPtr->outline, 0);
            r -= lw;
        }
        if (filled) {
            /* Paint filled interior */
            PaintEllipse(bigPtr, cx, cy, r, r, 0, &switchesPtr->color, 0);
        }
        offset = switchesPtr->shadow.offset;
        w = h = radius + radius + (1 + offset) * 2;
        tmpPtr = Blt_CreatePicture(w, h);
        Blt_ResamplePicture(tmpPtr, bigPtr, bltBoxFilter, bltBoxFilter);
#ifdef notdef
        fprintf(stderr, "big=%dx%d, blur=%d\n", bigPtr->width,bigPtr->height,
                (switchesPtr->shadow.offset * numSamples)/2);
#endif
        Blt_FreePicture(bigPtr);
        /*Blt_ApplyColorToPicture(tmpPtr, &switchesPtr->color); */
#ifdef notdef
        Blt_BlendRegion(picture, tmpPtr, 0, 0, w, h, x-w/2+offset,y-h/2+offset);
#endif
        {
            int yy;
            Blt_Pixel *destRowPtr;

            destRowPtr = Blt_Picture_Bits(tmpPtr);
            for (yy = 0; yy < Blt_Picture_Height(tmpPtr); yy++) {
                Blt_Pixel *dp, *dend;

                for (dp = destRowPtr, dend = dp + Blt_Picture_Width(tmpPtr); 
                     dp < dend; dp++) {
                    if (dp->Alpha != 0x00) {
                        dp->Red = switchesPtr->color.Red;
                        dp->Green = switchesPtr->color.Green;
                        dp->Blue = switchesPtr->color.color;
                    }
                }
                destRowPtr += Blt_Picture_Stride(tmpPtr);
            }
        }           
#ifndef notdef
        Blt_BlendRegion(picture, tmpPtr, 0, 0, w, h, x-w/2, y-h/2);
#else
        Blt_CopyArea(picture, tmpPtr, 0, 0, w, h, x-w/2, y-h/2);
#endif
        Blt_FreePicture(tmpPtr);
    } else if (switchesPtr->shadow.width > 0) {
        Pict *blurPtr;
        int w, h;
        Blt_Pixel color;
        int offset, r, lw;
        int cx, cy;

        w = h = (radius + radius);
        r = radius;
        offset = switchesPtr->shadow.offset;

        /* Scale the region forming the bounding box of the ellipse into a
         * new picture. The bounding box is scaled by *nSamples* times. */
        blurPtr = Blt_CreatePicture(w+(offset*4), h+(offset*4));
        cx = blurPtr->width / 2;
        cy = blurPtr->height / 2;
        Blt_BlankPicture(blurPtr, 0x0);

        color.u32 = switchesPtr->shadow.color.u32;
        /* Either ring or full circle for blur stencil. */
        lw = switchesPtr->lineWidth;
        if (filled) {
            lw = 0;
        }
        PaintEllipse(blurPtr, cx, cy, r, r, lw, &color, 0);
        Blt_BlurPicture(blurPtr, blurPtr, offset, 3);
        /* Offset the circle from the shadow. */
        cx -= offset;
        cy -= offset;
        lw = switchesPtr->lineWidth;
        if ((lw > 0) && (r > lw) && (switchesPtr->outline.u32 != 0x00)) {
            /* Paint ring outline. */
            PaintEllipse(blurPtr, cx, cy, r, r, lw, &switchesPtr->outline, 0); 
            r -= lw;
        }
        if (filled) {
            /* Paint filled interior */
            PaintEllipse(blurPtr, cx, cy, r, r, 0, &switchesPtr->color, 0);
        }
        x -= blurPtr->width/2 + offset;
        if (x < 0) {
            x = 0;
        }
        y -= blurPtr->height/2 + offset;
        if (y < 0) {
            y = 0;
        }
        Blt_BlendRegion(picture, blurPtr, 0, 0, blurPtr->width, blurPtr->height,
                        x, y);
        Blt_FreePicture(blurPtr);
    } else {
        int r;

        r = radius;
        if ((switchesPtr->lineWidth > 0) && (r > switchesPtr->lineWidth)) {
            /* Paint ring outline. */
            PaintEllipse(picture, x, y, r, r, switchesPtr->lineWidth, 
                &switchesPtr->outline, 1); 
            r -= switchesPtr->lineWidth;
        }
        if (filled) {
            /* Paint filled interior */
            PaintEllipse(picture, x, y, r, r, 0, &switchesPtr->color, 1);
        }
    }
}
#endif

static void
DrawCircleShadow(Blt_Picture picture, int x, int y, float r, 
                 float lineWidth, int blend, Blt_Shadow *shadowPtr)
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
        Blt_BlendRegion(picture, tmpPtr, 0, 0, w, h, x - r, y - r);
    } else {
        Blt_CopyArea(picture, tmpPtr, 0, 0, w, h, x - r, y - r);
    }
    Blt_FreePicture(tmpPtr);
}

static void
DrawCircle(Blt_Picture picture, int x, int y, int r, float lineWidth, 
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
    Blt_SetBrushArea(switches.brush, x - radius, y - radius, 
        radius + radius, radius + radius);
    DrawCircle(picture, x, y, radius, switches.lineWidth, switches.brush, 
               switches.blend);
    Blt_FreeSwitches(circleSwitches, (char *)&switches, 0);
    return TCL_OK;
}

#ifdef notdef

static Blt_Picture
CircleShadowLayer(int r, CircleSwitches2 *switchesPtr)
{
    Blt_Shadow *shadowPtr;
    Pict *destPtr;
    int x, y, w, h;

    shadowPtr = &switchesPtr->shadow;
    w = h = ((r + shadowPtr->width) * 2) + 1;
    x = w / 2;
    y = h / 2;
    destPtr = Blt_CreatePicture(w, h);
    Blt_BlankPicture(destPtr, 0x0);
#ifndef notdef
    fprintf(stderr, "r=%d linewidth=%g offset=%d width=%d w=%d h=%d x=%d y=%d\n",
            r, switchesPtr->lineWidth, shadowPtr->offset, shadowPtr->width,
            w, h, x, y);
#endif
    PaintCircle4(destPtr, x, y, r, switchesPtr->lineWidth, 
                 switchesPtr->brush, 0);
    Blt_BlurPicture(destPtr, destPtr, shadowPtr->width, 3);
    destPtr->flags &= ~BLT_PIC_ASSOCIATED_COLORS;
    return destPtr;
}

static Blt_Picture
CircleLayer(int radius, CircleSwitches2 *switchesPtr)
{
    Pict *destPtr;
    int x, y, w, h;

    w = h = (radius * 2) + 1;
    x = w / 2;
    y = h / 2;
    destPtr = Blt_CreatePicture(w, h);
    Blt_BlankPicture(destPtr, 0x0);
    PaintCircle4(destPtr, x, y, radius, switchesPtr->lineWidth, 
                 switchesPtr->brush, 0);
    destPtr->flags &= ~BLT_PIC_ASSOCIATED_COLORS;
    return destPtr;
}
#endif


#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * Circle2Op --
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
static int
CircleOp2(
    Blt_Picture picture,
    Tcl_Interp *interp,                 /* Current interpreter. */
    int objc,                           /* # of arguments. */
    Tcl_Obj *const *objv)               /* Argument objects. */
{
    CircleSwitches2 switches;
    int x, y, r;
    Blt_Picture outline, fill, shadow;
    Pict *mergePtr;
    Blt_PaintBrush brush;

    if (objc < 5) {
        Tcl_AppendResult(interp, "wrong # of coordinates for circle",
                         (char *)NULL);
        return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &r) != TCL_OK)) {
        return TCL_ERROR;
    }
    /* Process switches  */
    switches.lineWidth = 0.0;
    if (Blt_GetPaintBrush(interp, "white", &brush) != TCL_OK) {
        return TCL_ERROR;
    }
    switches.brush = brush;
    switches.blend = 0;
    switches.antialiased = 0;
    Blt_Shadow_Set(&switches.shadow, 0, 0, 0x0, 0xFF);
    if (Blt_ParseSwitches(interp, circleSwitches2, objc - 6, objv + 6, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    mergePtr = shadow = fill = outline = NULL;
    if (switches.shadow.width > 0) {
        mergePtr = CircleShadowLayer(r, &switches);
    }
    if (switches.fill.u32 != 0) {
        fill = CircleLayer(r, &switches);
        if (mergePtr != NULL) {
            MergePictures(mergePtr, fill);
            Blt_FreePicture(fill);
        } else {
            mergePtr = fill;
        }
    } 
    if (switches.outline.u32 != 0) {
        outline = CircleLayer(r, &switches);
        if (mergePtr != NULL) {
            MergePictures(mergePtr, outline);
            Blt_FreePicture(outline);
        } else {
            mergePtr = outline;
        }
    }
    Blt_BlendRegion(picture, mergePtr, 0, 0, mergePtr->width, mergePtr->height,
                    x-r, y-r);
    Blt_FreePicture(mergePtr);
    Blt_FreeSwitches(circleSwitches, (char *)&switches, 0);
    return TCL_OK;
}
#endif

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
    Point2f *points;
    
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
        float *x, *y;

        numPoints = switches.x.numValues;
        points = Blt_Malloc(sizeof(Point2f) * numPoints);
        if (points == NULL) {
            Tcl_AppendResult(interp, "can't allocate memory for ", 
                Blt_Itoa(numPoints + 1), " points", (char *)NULL);
            return TCL_ERROR;
        }
        x = (float *)switches.x.values;
        y = (float *)switches.y.values;
        for (i = 0; i < numPoints; i++) {
            points[i].x = x[i];
            points[i].y = y[i];
        }
        Blt_Free(switches.x.values);
        Blt_Free(switches.y.values);
        switches.x.values = switches.y.values = NULL;
    } else if (switches.coords.numValues > 0) {
        size_t i, j;
        float *coords;

        if (switches.coords.numValues & 0x1) {
            Tcl_AppendResult(interp, "bad -coords list: ",
                "must have an even number of values", (char *)NULL);
            return TCL_ERROR;
        }
        numPoints = (switches.coords.numValues / 2);
        points = Blt_Malloc(sizeof(Point2f)* numPoints);
        if (points == NULL) {
            Tcl_AppendResult(interp, "can't allocate memory for ", 
                Blt_Itoa(numPoints + 1), " points", (char *)NULL);
            return TCL_ERROR;
        }
        coords = (float *)switches.coords.values;
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
    Point2f *vertices;
    Region2f r;
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
        float *x, *y;

        numVertices = switches.x.numValues;
        vertices = Blt_Malloc(sizeof(Point2f) * (switches.x.numValues + 1));
        if (vertices == NULL) {
            Tcl_AppendResult(interp, "can't allocate memory for ", 
                Blt_Itoa(numVertices + 1), " vertices", (char *)NULL);
            return TCL_ERROR;
        }
        x = (float *)switches.x.values;
        y = (float *)switches.y.values;
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
        float *coords;

        if (switches.coords.numValues & 0x1) {
            Tcl_AppendResult(interp, "bad -coords list: ",
                "must have an even number of values", (char *)NULL);
            return TCL_ERROR;
        }
        numVertices = (switches.coords.numValues / 2);
        vertices = Blt_Malloc(sizeof(Point2f)* (numVertices + 1));
        if (vertices == NULL) {
            Tcl_AppendResult(interp, "can't allocate memory for ", 
                Blt_Itoa(numVertices + 1), " vertices", (char *)NULL);
            return TCL_ERROR;
        }
        coords = (float *)switches.coords.values;
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
                Blt_SetBrushArea(switches.brush, r.left, r.top, 
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
 *---------------------------------------------------------------------------
 */
int
Blt_Picture_RectangleOp(ClientData clientData, Tcl_Interp *interp, int objc,
                        Tcl_Obj *const *objv)   
{
    Blt_Picture picture = clientData;
    RectangleSwitches switches;
    PictArea area;
    Blt_PaintBrush brush;
    
    if (Blt_GetAreaFromObjv(interp, 4, objv + 3, &area) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_GetPaintBrush(interp, "black", &brush) != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    /* Process switches  */
    switches.brush = brush;
    switches.lineWidth = 0;
    if (Blt_ParseSwitches(interp, rectangleSwitches, objc - 7, objv + 7, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    Blt_SetBrushArea(switches.brush, area.x1, area.y1, area.x2 - area.x1,
                     area.y2 - area.y1);
    if (switches.shadow.width > 0) {
        PaintRectangleShadow(picture, area.x1, area.y1, area.x2 - area.x1,
                             area.y2 - area.y1, switches.radius, 
                switches.lineWidth, &switches.shadow);
    }
    Blt_PaintRectangle(picture, area.x1, area.y1, area.x2 - area.x1,
                       area.y2 - area.y1, switches.radius, 
                switches.lineWidth, switches.brush, TRUE);
    Blt_FreeSwitches(rectangleSwitches, (char *)&switches, 0);
    return TCL_OK;
}


#ifdef notdef
static void
PaintTextLayout(Pict *destPtr, FtFont *fontPtr, TextLayout *layoutPtr, 
                int x, int y, TextSwitches *switchesPtr)
{
    TextFragment *fp, *fend;

    for (fp = layoutPtr->fragments, fend = fp + layoutPtr->numFragments;
         fp < fend; fp++) {
        PaintText(destPtr, fontPtr, fp->text, fp->count, x + fp->rx, 
                y + fp->ry, switchesPtr->kerning, switchesPtr->brush);
    }
}
#endif


#ifdef notdef
static void 
Polyline2(Pict *destPtr, int x1, int y1, int x2, int y2, Blt_Pixel *colorPtr)
{
    Blt_Pixel *dp;
    int dx, dy, xDir;
    unsigned long error;

    if (y1 > y2) {
        int tmp;

        tmp = y1, y1 = y2, y2 = tmp;
        tmp = x1, x1 = x2, x2 = tmp;
    }

    /* First and last Pixels always get Set: */
    dp = Blt_Picture_Pixel(destPtr, x1, y1);
    dp->u32 = colorPtr->u32;
    dp = Blt_Picture_Pixel(destPtr, x2, y2);
    dp->u32 = colorPtr->u32;

    dx = x2 - x1;
    dy = y2 - y1;

    if (dx >= 0) {
        xDir = 1;
    } else {
        xDir = -1;
        dx = -dx;
    }
    if (dx == 0) {              /*  Vertical line */
        Blt_Pixel *dp;

        dp = Blt_Picture_Pixel(destPtr, x1, y1);
        while (dy-- > 0) {
            dp += destPtr->pixelsPerRow;
            dp->u32 = colorPtr->u32;
        }
        return;
    }
    if (dy == 0) {              /* Horizontal line */
        Blt_Pixel *dp;

        dp = Blt_Picture_Pixel(destPtr, x1, y1);
        while(dx-- > 0) {
            dp += xDir;
            dp->u32 = colorPtr->u32;
        }
        return;
    }
    if (dx == dy) {             /* Diagonal line. */
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
    if (dy > dx) {              /* y-major line */
        unsigned long adjust;

        /* x1 -= lineWidth / 2; */
        adjust = (dx << 16) / dy;
        while(--dy) {
            Blt_Pixel *dp;
            int x;
            unsigned char weight;
            
            error += adjust;
            ++y1;
            if (error & ~0xFFFF) {
                x1 += xDir;
                error &= 0xFFFF;
            }
            dp = Blt_Picture_Pixel(destPtr, x1, y1);
            weight = (unsigned char)(error >> 8);
            x = x1;
            if (x >= 0) {
                dp->u32 = colorPtr->u32;
                dp->Alpha = ~weight;
            }
            x += xDir;
            dp += xDir;
            if (x >= 0) {
                dp->u32 = colorPtr->u32;
                dp->Alpha = weight;
            }
        }
    } else {                    /* x-major line */
        unsigned long adjust;

        /* y1 -= lineWidth / 2; */
        adjust = (dy << 16) / dx;
        while (--dx) {
            Blt_Pixel *dp;
            int y;
            unsigned char weight;

            error += adjust;
            x1 += xDir;
            if (error & ~0xFFFF) {
                y1++;
                error &= 0xFFFF;
            }
            dp = Blt_Picture_Pixel(destPtr, x1, y1);
            weight = (unsigned char)(error >> 8);
            y = y1;
            if (y >= 0) {
                dp->u32 = colorPtr->u32;
                dp->Alpha = ~weight;
            }
            dp += destPtr->pixelsPerRow;
            y++;
            if (y >= 0) {
                dp->u32 = colorPtr->u32;
                dp->Alpha = weight;
            } 
        }
    }
}
#endif

Blt_Picture
Blt_PaintCheckbox(int w, int h, XColor *fillColorPtr, XColor *outlineColorPtr, 
                  XColor *checkColorPtr, int on)
{
    Blt_Shadow shadow;
    int x, y;
    Pict *destPtr;
    Blt_PaintBrush brush;

    destPtr = Blt_CreatePicture(w, h);
    Blt_Shadow_Set(&shadow, 1, 1, 0x0, 0xA0);
    brush = Blt_NewColorBrush(0x00000000);
    x = y = 0;
    if (fillColorPtr != NULL) {
        Blt_SetColorBrushColor(brush, Blt_XColorToPixel(fillColorPtr));
        Blt_PaintRectangle(destPtr, x+1, y+1, w-2, h-2, 0, 0, brush, TRUE);
    }
    if (outlineColorPtr != NULL) {
        Blt_SetColorBrushColor(brush, Blt_XColorToPixel(outlineColorPtr));
        Blt_PaintRectangle(destPtr, x, y, w, h, 0, 1, brush, TRUE);
    }
    x += 2, y += 2;
    w -= 5, h -= 5;
    if (on) {
        Point2f points[7];
        Region2f r;

        points[0].x = points[1].x = points[6].x = x;
        points[0].y = points[6].y = y + (0.4 * h);
        points[1].y = y + (0.6 * h);
        points[2].x = points[5].x = x + (0.4 * w);
        points[2].y = y + h;
        points[3].x = points[4].x = x + w;
        points[3].y = y + (0.2 * h);
        points[4].y = y;
        points[5].y = y + (0.7 * h);
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
    destPtr->flags |= BLT_PIC_BLEND | BLT_PIC_ASSOCIATED_COLORS;
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
    Blt_SetBrushArea(newBrush, 0, 0, w, h); 
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
Blt_PaintDelete(
    int w, int h, 
    XColor *bgColorPtr, 
    XColor *fillColorPtr, 
    XColor *symbolColorPtr, 
    int isActive)
{
    Blt_Picture picture;
    Point2f points[4];
    Region2f reg;
    Blt_Shadow shadow;
    int x, y, r;
    Blt_PaintBrush brush;

    brush = Blt_NewColorBrush(Blt_XColorToPixel(fillColorPtr));
    Blt_Shadow_Set(&shadow, 1, 2, 0x0, 0xA0);
    x = y = 0;
    reg.left = x, reg.right = x + w;
    reg.top = y, reg.bottom = y + h;

    picture = Blt_CreatePicture(w, h);
    Blt_BlankPicture(picture, 0x0);
    x = y = w / 2 - 1;
    r = x - 1;
#ifdef notdef
    if ((isActive) && (shadow.width > 0)) {
        DrawCircleShadow(picture, x, y, r, 0.0, TRUE, &shadow);
    }
#endif
    DrawCircle(picture, x, y, r, 0.0, brush, FALSE);

    points[0].x = x - 2;
    points[0].y = y - 3;
    points[1].x = x - 3;
    points[1].y = y - 2;
    points[2].x = x + 2;
    points[2].y = y + 3;
    points[3].x = x + 3;
    points[3].y = y + 2;

    Blt_SetColorBrushColor(brush, Blt_XColorToPixel(symbolColorPtr));
    PaintPolygonAA2(picture, 4, points, &reg, brush, NULL);

    points[0].x = x + 3;
    points[0].y = y - 2;
    points[1].x = x + 2;
    points[1].y = y - 3;
    points[2].x = x - 3;
    points[2].y = y + 2;
    points[3].x = x - 2;
    points[3].y = y + 3;

    PaintPolygonAA2(picture, 4, points, &reg, brush, NULL);
    Blt_FreeBrush(brush);
    return picture;
}

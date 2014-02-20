/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictDraw.c --
 *
 * This module implements image drawing primitives (line, circle, rectangle,
 * text, etc.) for picture images in the BLT toolkit.
 *
 *	Copyright 1997-2004 George A Howlett.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"
#include <string.h>
#ifdef HAVE_LIBXFT
#include <ft2build.h>
#include FT_FREETYPE_H
#include <X11/Xft/Xft.h>
#endif
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

#define imul8x8(a,b,t)	((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))
#define CLAMP(c)	((((c) < 0.0) ? 0.0 : ((c) > 255.0) ? 255.0 : (c)))


#ifndef WIN32 
#  if defined (HAVE_LIBXFT) && defined(HAVE_X11_XFT_XFT_H)
#    define HAVE_XFT
#    include <X11/Xft/Xft.h>
#  endif	/* HAVE_XFT */
#endif /*WIN32*/

#if defined (HAVE_FT2BUILD_H)
#  define HAVE_FT2
#  include <ft2build.h>
#  include FT_FREETYPE_H
#endif

#ifndef HAVE_FT2
#  define DRAWTEXT 0
#  error("No Freetype")
#else 
#  ifdef WIN32
#    define DRAWTEXT 1
#  else 
#    ifdef HAVE_XFT
#      define DRAWTEXT 1
#    else
#      error("!WIN32 && !XFT")
#      define DRAWTEXT 0
#    endif
#  endif
#endif

#if DRAWTEXT
static FT_Library ftLibrary;

typedef struct {
    FT_Face face;
    FT_Matrix matrix;
    FT_Library lib;
#ifdef HAVE_XFT
    XftFont *xftFont;
#endif
    int fontSize;
    float angle;
    int height;
    int ascent, descent;
} FtFont;    

#endif /*HAVE_FT2BUILD_H && HAVE_LIBXFT */

typedef struct {
    size_t numValues;
    void *values;
} Array;

static Blt_SwitchParseProc ArraySwitchProc;
static Blt_SwitchFreeProc ArrayFreeProc;
static Blt_SwitchCustom arraySwitch = {
    ArraySwitchProc, NULL, ArrayFreeProc, (ClientData)0
};

static Blt_SwitchParseProc AnchorSwitch;
static Blt_SwitchCustom anchorSwitch = {
    AnchorSwitch, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ShadowSwitch;
static Blt_SwitchCustom shadowSwitch = {
    ShadowSwitch, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc JustifySwitch;
static Blt_SwitchCustom justifySwitch = {
    JustifySwitch, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ColorSwitchProc;
static Blt_SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ObjToPaintBrushProc;
static Blt_SwitchFreeProc PaintBrushFreeProc;
static Blt_SwitchCustom paintbrushSwitch =
{
    ObjToPaintBrushProc, NULL, PaintBrushFreeProc, (ClientData)0,
};

typedef struct {
    Blt_PaintBrush *brushPtr;	 /* Outline and fill colors for the circle. */
    Blt_Shadow shadow;
    int antialiased;
    float lineWidth;		/* Width of outline.  If zero, indicates to
				 * draw a solid circle. */
    int blend;
} CircleSwitches;

static Blt_SwitchSpec circleSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-color", "color", (char *)NULL,
	Blt_Offset(CircleSwitches, brushPtr),    0, 0, &paintbrushSwitch},
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
    Blt_PaintBrush *brushPtr;		/* Outline and fill colors for the
					 * circle. */
    Blt_Pixel fill;			/* Fill color of circle. */
    Blt_Pixel outline;			/* Outline color of circle. */
    Blt_Shadow shadow;
    int antialiased;
    float lineWidth;			/* Line width of outline.  If zero,
					 * indicates to draw a solid
					 * circle. */
    int blend;
} CircleSwitches2;

static Blt_SwitchSpec circleSwitches2[] = 
{
    {BLT_SWITCH_CUSTOM, "-color", "color", (char *)NULL,
	Blt_Offset(CircleSwitches2, brushPtr),    0, 0, &paintbrushSwitch},
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

typedef struct {
    Blt_Pixel fill, outline;	 /* Outline and fill colors for the circle. */
    Blt_Shadow shadow;
    int antialiased;
    int lineWidth;			/* Width of outline.  If zero,
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
    Blt_Pixel bg;			/* Color of line. */
    int lineWidth;			/* Width of outline. */
    Array x, y;
    Array coords;
} LineSwitches;

typedef struct {
    Blt_PaintBrush *brushPtr;		/* Fill color of polygon. */
    int antialiased;
    Blt_Shadow shadow;
    int lineWidth;			/* Width of outline. Default is 1, If
					 * zero, indicates to draw a solid
					 * polygon. */
    Array coords;
    Array x, y;

} PolygonSwitches;

static Blt_SwitchSpec polygonSwitches[] = 
{
    {BLT_SWITCH_BOOLEAN, "-antialiased", "bool", (char *)NULL,
	Blt_Offset(PolygonSwitches, antialiased), 0},
    {BLT_SWITCH_CUSTOM, "-color", "color", (char *)NULL,
	Blt_Offset(PolygonSwitches, brushPtr),    0, 0, &paintbrushSwitch},
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
    Blt_PaintBrush *brushPtr;		/* Color of rectangle. */
    Blt_Shadow shadow;
    int lineWidth;			/* Width of outline. If zero,
					 * indicates to draw a solid
					 * rectangle. */
    int radius;				/* Radius of rounded corner. */
    int antialiased;
} RectangleSwitches;

static Blt_SwitchSpec rectangleSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-color", "color", (char *)NULL,
	Blt_Offset(RectangleSwitches, brushPtr),    0, 0, &paintbrushSwitch},
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

typedef struct {
    int kerning;
    Blt_PaintBrush *brushPtr;			/* Color of text. */
    Blt_Shadow shadow;
    int fontSize;
    Tcl_Obj *fontObjPtr;
    int justify;
    Tk_Anchor anchor;
    float angle;
} TextSwitches;

static Blt_SwitchSpec textSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-anchor",   "anchor", (char *)NULL,
	Blt_Offset(TextSwitches, anchor), 0, 0, &anchorSwitch},
    {BLT_SWITCH_CUSTOM,  "-color",    "colorName", (char *)NULL,
	Blt_Offset(TextSwitches, brushPtr),  0, 0, &paintbrushSwitch},
    {BLT_SWITCH_OBJ,     "-font",     "fontName", (char *)NULL,
	Blt_Offset(TextSwitches, fontObjPtr), 0},
    {BLT_SWITCH_CUSTOM,  "-justify", "left|right|center", (char *)NULL,
	Blt_Offset(TextSwitches, justify), 0, 0, &justifySwitch},
    {BLT_SWITCH_BOOLEAN, "-kerning",  "bool", (char *)NULL,
	Blt_Offset(TextSwitches, kerning),  0},
    {BLT_SWITCH_FLOAT,   "-rotate",   "angle", (char *)NULL,
	Blt_Offset(TextSwitches, angle), 0},
    {BLT_SWITCH_INT,     "-size",     "number", (char *)NULL,
	Blt_Offset(TextSwitches, fontSize),  0}, 
    {BLT_SWITCH_CUSTOM, "-shadow", "offset", (char *)NULL,
        Blt_Offset(TextSwitches, shadow), 0, 0, &shadowSwitch},
    {BLT_SWITCH_END}
};

extern Tcl_ObjCmdProc Blt_Picture_CircleOp;
extern Tcl_ObjCmdProc Blt_Picture_EllipseOp;
extern Tcl_ObjCmdProc Blt_Picture_LineOp;
extern Tcl_ObjCmdProc Blt_Picture_PolygonOp;
extern Tcl_ObjCmdProc Blt_Picture_RectangleOp;
extern Tcl_ObjCmdProc Blt_Picture_TextOp;

/*
 *---------------------------------------------------------------------------
 *
 * ColorSwitchProc --
 *
 *	Convert a Tcl_Obj representing a Blt_Pixel color.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColorSwitchProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
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

/*ARGSUSED*/
static void
PaintBrushFreeProc(ClientData clientData, char *record, int offset, int flags)
{
    Blt_PaintBrush **brushPtrPtr = (Blt_PaintBrush **)(record + offset);

    if (*brushPtrPtr != NULL) {
	Blt_PaintBrush_Free(*brushPtrPtr);
	*brushPtrPtr = NULL;
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToPaintBrushProc --
 *
 *	Convert a Tcl_Obj representing a paint brush.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPaintBrushProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Blt_PaintBrush **brushPtrPtr = (Blt_PaintBrush **)(record + offset);
    Blt_PaintBrush *brushPtr;

    if (Blt_PaintBrush_Get(interp, objPtr, &brushPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (*brushPtrPtr != NULL) {
	Blt_PaintBrush_Free(*brushPtrPtr);
    }
    *brushPtrPtr = brushPtr;
    return TCL_OK;
}

/*
 *
 *  x,y------+
 *   |       |
 *   |	*----+------------+
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
}

static Pict *
BgPicture(Pict *srcPtr, int sx, int sy, int w, int h)
{
    Pict *destPtr;
    Blt_Pixel *srcRowPtr, *destRowPtr;
    size_t numBytes;
    int y;

    w = MIN(w, srcPtr->width - sx);
    h = MIN(h, srcPtr->height - sy);
    destPtr = Blt_CreatePicture(w*4, h*4);
    srcRowPtr = srcPtr->bits + (sy * srcPtr->width) + sx;
    destRowPtr = destPtr->bits;
    numBytes = sizeof(Blt_Pixel) * destPtr->pixelsPerRow;
    for (y = 0; y < h; y++) {
	Blt_Pixel *dp, *sp, *send;
	Blt_Pixel *nextRowPtr;

	for (dp = destRowPtr, sp = srcRowPtr, send = sp + w; sp < send; sp++) {
	    Blt_Pixel p;

	    p.u32 = sp->u32;
	    dp[0].u32 = dp[1].u32 = dp[2].u32 = dp[3].u32 = p.u32;
	    dp += 4;
	}

	nextRowPtr = destRowPtr + destPtr->pixelsPerRow;
	memcpy(nextRowPtr, destRowPtr, numBytes);
	nextRowPtr += destPtr->pixelsPerRow;
	memcpy(nextRowPtr, destRowPtr, numBytes);
	nextRowPtr += destPtr->pixelsPerRow;
	memcpy(nextRowPtr, destRowPtr, numBytes);
	nextRowPtr += destPtr->pixelsPerRow;
	destRowPtr = nextRowPtr;
	srcRowPtr += srcPtr->pixelsPerRow;
    }
    return destPtr;
}

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

#if DRAWTEXT
static void MeasureText(FtFont *fontPtr, const char *string, size_t length,
			size_t *widthPtr, size_t *heightPtr);

static size_t GetTextWidth(FtFont *fontPtr, const char *string, size_t length, 
			   int kerning);
#endif /*DRAWTEXT*/

/*
 *---------------------------------------------------------------------------
 *
 * ArrayFreeProc --
 *
 *	Free the storage associated with the -table switch.
 *
 * Results:
 *	None.
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
 *	Convert a Tcl_Obj list of numbers into an array of floats.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ArraySwitchProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to return results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
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

/*
 *---------------------------------------------------------------------------
 *
 * AnchorSwitch --
 *
 *	Convert a Tcl_Obj representing an anchor.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AnchorSwitch(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Tk_Anchor *anchorPtr = (Tk_Anchor *)(record + offset);

    if (Tk_GetAnchorFromObj(interp, objPtr, anchorPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AnchorSwitch --
 *
 *	Convert a Tcl_Obj representing an anchor.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
JustifySwitch(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Tk_Justify *justifyPtr = (Tk_Justify *)(record + offset);

    if (Tk_GetJustifyFromObj(interp, objPtr, justifyPtr) != TCL_OK) {
	return TCL_ERROR;
    }
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
 * ShadowSwitch --
 *
 *	Convert a Tcl_Obj representing a number for the alpha value.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ShadowSwitch(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
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


#if DRAWTEXT
/*
 *---------------------------------------------------------------------------
 *
 * CreateSimpleTextLayout --
 *
 *	Get the extents of a possibly multiple-lined text string.
 *
 * Results:
 *	Returns via *widthPtr* and *heightPtr* the dimensions of the text
 *	string.
 *
 *---------------------------------------------------------------------------
 */
static TextLayout *
CreateSimpleTextLayout(FtFont *fontPtr, const char *text, int textLen, 
		       TextStyle *tsPtr)
{
    TextFragment *fp;
    TextLayout *layoutPtr;
    size_t count;			/* Count # of characters on each
					 * line. */
    int lineHeight;
    int maxHeight, maxWidth;
    int numFrags;
    const char *p, *endp, *start;
    int i;
    size_t size;

    numFrags = 0;
    endp = text + ((textLen < 0) ? strlen(text) : textLen);
    for (p = text; p < endp; p++) {
	if (*p == '\n') {
	    numFrags++;
	}
    }
    if ((p != text) && (*(p - 1) != '\n')) {
	numFrags++;
    }
    size = sizeof(TextLayout) + (sizeof(TextFragment) * (numFrags - 1));

    layoutPtr = Blt_AssertCalloc(1, size);
    layoutPtr->numFragments = numFrags;

    numFrags = count = 0;
    maxWidth = 0;
    maxHeight = tsPtr->padTop;
    lineHeight = fontPtr->height;

    fp = layoutPtr->fragments;
    for (p = start = text; p < endp; p++) {
	if (*p == '\n') {
	    size_t w;

	    if (count > 0) {
		w = GetTextWidth(fontPtr, start, count, 1);
		if (w > maxWidth) {
		    maxWidth = w;
		}
	    } else {
		w = 0;
	    }
	    fp->width = w;
	    fp->count = count;
	    fp->sy = fp->y = maxHeight + fontPtr->ascent;
	    fp->text = start;
	    maxHeight += lineHeight;
	    fp++;
	    numFrags++;
	    start = p + 1;		/* Start the text on the next line */
	    count = 0;			/* Reset to indicate the start of a
					 * new line */
	    continue;
	}
	count++;
    }
    if (numFrags < layoutPtr->numFragments) {
	size_t w;

	w = GetTextWidth(fontPtr, start, count, 1);
	if (w > maxWidth) {
	    maxWidth = w;
	}
	fp->width = w;
	fp->count = count;
	fp->sy = fp->y = maxHeight + fontPtr->ascent;
	fp->text = start;
	maxHeight += lineHeight;
	numFrags++;
    }
    maxHeight += tsPtr->padBottom;
    maxWidth += PADDING(tsPtr->xPad);
    fp = layoutPtr->fragments;
    for (i = 0; i < numFrags; i++, fp++) {
	switch (tsPtr->justify) {
	default:
	case TK_JUSTIFY_LEFT:
	    /* No offset for left justified text strings */
	    fp->x = fp->sx = tsPtr->padLeft;
	    break;
	case TK_JUSTIFY_RIGHT:
	    fp->x = fp->sx = (maxWidth - fp->width) - tsPtr->padRight;
	    break;
	case TK_JUSTIFY_CENTER:
	    fp->x = fp->sx = (maxWidth - fp->width) / 2;
	    break;
	}
    }
    if (tsPtr->underline >= 0) {
	fp = layoutPtr->fragments;
	for (i = 0; i < numFrags; i++, fp++) {
	    int first, last;

	    first = fp->text - text;
	    last = first + fp->count;
	    if ((tsPtr->underline >= first) && (tsPtr->underline < last)) {
		layoutPtr->underlinePtr = fp;
		layoutPtr->underline = tsPtr->underline - first;
		break;
	    }
	}
    }
    layoutPtr->width = maxWidth;
    layoutPtr->height = maxHeight - tsPtr->leader;
    return layoutPtr;
}
#endif

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
    if (dy == 0) {			/* Horizontal line */
	HorizLine(destPtr, x1, x2, y1, &edge);
	return;
    }
    if (dx == 0) {			/*  Vertical line */
	VertLine(destPtr, x1, y1, y2, &edge);
	return;
    }
    if (dx == dy) {			/* Diagonal line. */
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
    if (dy > dx) {			/* y-major line */
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
    } else {				/* x-major line */
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
    if (dx == 0) {			/*  Vertical line */
	VertLine(destPtr, x1, y1, y2, &interior);
	return;
    }
    if (dy == 0) {			/* Horizontal line */
	HorizLine(destPtr, x1, x2, y1, &interior);
	return;
    }
    if (dx == dy) {			/* Diagonal line. */
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
    if (dy > dx) {			/* y-major line */
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
    } else {				/* x-major line */
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

#if DRAWTEXT

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { -1, 0 } };

static const char *
FtError(FT_Error ftError)
{
    struct ft_errors {                                          
	int          code;             
	const char*  msg;
    };
    static struct ft_errors ft_err_mesgs[] = 
#include FT_ERRORS_H            

    struct ft_errors *fp;
    for (fp = ft_err_mesgs; fp->msg != NULL; fp++) {
	if (fp->code == ftError) {
	    return fp->msg;
	}
    }
    return "unknown Freetype error";
}

static void
MeasureText(FtFont *fontPtr, const char *string, size_t length,
	    size_t *widthPtr, size_t *heightPtr)
{
    FT_Vector pen;			/* Untransformed origin  */
    FT_GlyphSlot  slot;
    FT_Matrix matrix;			/* Transformation matrix. */
    int maxX, maxY;
    const char *p, *pend;
    double radians;
    int x;
    FT_Face face;

    radians = 0.0;
    matrix.yy = matrix.xx = (FT_Fixed)(cos(radians) * 65536.0);
    matrix.yx = (FT_Fixed)(sin(radians) * 65536.0);
    matrix.xy = -matrix.yx;

    face = fontPtr->face;
    slot = face->glyph;
    
    maxY = maxX = 0;
    pen.y = 0;
    x = 0;
#ifdef notdef
    fprintf(stderr, "face->height=%d, face->size->height=%d\n",
	    face->height, (int)face->size->metrics.height);
    fprintf(stderr, "face->ascender=%d, face->descender=%d\n",
	    face->ascender, face->descender);
#endif
    for (p = string, pend = p + length; p < pend; p++) {
	maxY += face->size->metrics.height;
	pen.x = x << 6;
	for (/*empty*/; (*p != '\n') && (p < pend); p++) {
	    FT_Error ftError;

	    FT_Set_Transform(face, &matrix, &pen);
	    /* Load glyph image into the slot (erase previous) */
	    ftError = FT_Load_Char(face, *p, FT_LOAD_RENDER);
	    if (ftError != 0) {
		Blt_Warn("can't load character \"%c\": %s\n", *p, 
			FtError(ftError));
		continue;                 /* Ignore errors. */
	    }
	    pen.x += slot->advance.x;
	    pen.y += slot->advance.y;
	}
	if (pen.x > maxX) {
	    maxX = pen.x;
	}
    }	
#ifdef notdef
    fprintf(stderr, "w=%d,h=%d\n", maxX >> 6, maxY >> 6);
#endif
    *widthPtr = (size_t)(maxX >> 6);
    *heightPtr = (size_t)(maxY >> 6);
#ifdef notdef
    fprintf(stderr, "w=%lu,h=%lu\n", (unsigned long)*widthPtr, 
	    (unsigned long)*heightPtr);
#endif
}

static size_t
GetTextWidth(FtFont *fontPtr, const char *string, size_t length, int kerning)
{
    FT_Vector pen;			/* Untransformed origin  */
    FT_GlyphSlot  slot;
    FT_Matrix matrix;			/* Transformation matrix. */
    int maxX;
    const char *p, *pend;
    double radians;
    FT_Face face;
    int previous;
    FT_Error ftError;

    radians = 0.0;
    matrix.yy = matrix.xx = (FT_Fixed)(cos(radians) * 65536.0);
    matrix.yx = (FT_Fixed)(sin(radians) * 65536.0);
    matrix.xy = -matrix.yx;

    face = fontPtr->face;
    slot = face->glyph;
    
    maxX = 0;
    pen.y = pen.x = 0;
#ifdef notdef
    fprintf(stderr, "face->height=%d, face->size->height=%d\n",
	    face->height, (int)face->size->metrics.height);
#endif
    previous = -1;
    for (p = string, pend = p + length; p < pend; p++) {
	int current;

	current = FT_Get_Char_Index(face, *p);
	if ((kerning) && (previous >= 0)) { 
	    FT_Vector delta; 
		
	    FT_Get_Kerning(face, previous, current, FT_KERNING_DEFAULT, &delta);
	    pen.x += delta.x; 
	} 
	FT_Set_Transform(face, &matrix, &pen);
	previous = current;
	/* load glyph image into the slot (erase previous one) */  
	ftError = FT_Load_Glyph(face, current, FT_LOAD_DEFAULT); 
	if (ftError) {
	    Blt_Warn("can't load character \"%c\" (%d): %s\n", *p, current, 
		FtError(ftError));
	    continue;			/* Ignore errors. */
	}
	pen.x += slot->advance.x;
	pen.y += slot->advance.y;
	if (pen.x > maxX) {
	    maxX = pen.x;
	}
    }	
    return maxX >> 6;
}


static void
BlitGlyph(Pict *destPtr, 
    FT_GlyphSlot slot, 
    int dx, int dy,
    int xx, int yy,
    Blt_PaintBrush *brushPtr)
{
    int x1, y1, x2, y2;
#ifdef notdef
    fprintf(stderr, "dx=%d, dy=%d\n", dx, dy);
    fprintf(stderr, "  slot.bitmap.width=%d\n", (int)slot->bitmap.width);
    fprintf(stderr, "  slot.bitmap.rows=%d\n", slot->bitmap.rows);
    fprintf(stderr, "  slot.bitmap_left=%d\n", (int)slot->bitmap_left);
    fprintf(stderr, "  slot.bitmap_top=%d\n", (int)slot->bitmap_top);
    fprintf(stderr, "  slot.bitmap.pixel_mode=%x\n", slot->bitmap.pixel_mode);
    fprintf(stderr, "  slot.advance.x=%d\n", (int)slot->advance.x);
    fprintf(stderr, "  slot.advance.y=%d\n", (int)slot->advance.y);
    
    fprintf(stderr, "  slot.format=%c%c%c%c\n", 
	    (slot->format >> 24) & 0xFF, 
	    (slot->format >> 16) & 0xFF, 
	    (slot->format >> 8) & 0xFF, 
	    (slot->format & 0xFF));
#endif
    if ((dx >= destPtr->width) || ((dx + slot->bitmap.width) <= 0) ||
	(dy >= destPtr->height) || ((dy + slot->bitmap.rows) <= 0)) {
	return;				/* No portion of the glyph is visible
					 * in the picture. */
    }
    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (dx < 0) {			/* Left side of glyph overhangs. */
	x1 -= dx;		
	x2 += dx;
	dx = 0;		
    }
    if (dy < 0) {			/* Top of glyph overhangs. */
	y1 -= dy;
	y2 += dy;
	dy = 0;
    }
    if ((dx + x2) > destPtr->width) {	/* Right side of glyph overhangs. */
	x2 = destPtr->width - dx;
    }
    if ((dy + y2) > destPtr->height) {	/* Bottom of glyph overhangs. */
	y2 = destPtr->height - dy;
    }
    if (slot->bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
	Blt_Pixel *destRowPtr;
	unsigned char *srcRowPtr;
	int y;

	srcRowPtr = slot->bitmap.buffer + (y1 * slot->bitmap.pitch);
	destRowPtr = Blt_Picture_Pixel(destPtr, xx, yy);
	for (y = y1; y < y2; y++) {
	    Blt_Pixel *dp;
	    int x;
	    
	    dp = destRowPtr;
	    for (x = x1; x < x2; x++, dp++) {
		int pixel;

		pixel = srcRowPtr[x >> 3] & (1 << (7 - (x & 0x7)));
		if (pixel != 0x0) {
		    Blt_Pixel color;

		    color.u32 = Blt_PaintBrush_GetAssociatedColor(brushPtr, 
                        x, y);
		    BlendPixels(dp, &color);
		}
	    }
	    srcRowPtr += slot->bitmap.pitch;
	    destRowPtr += destPtr->pixelsPerRow;
	}
    } else {
	Blt_Pixel *destRowPtr;
	unsigned char *srcRowPtr;
	int y;

	srcRowPtr = slot->bitmap.buffer + ((y1 * slot->bitmap.pitch) + x1);
	destRowPtr = Blt_Picture_Pixel(destPtr, dx, dy);
	for (y = y1; y < y2; y++) {
	    Blt_Pixel *dp;
	    unsigned char *sp;
	    int x;

	    dp = destRowPtr;
	    for (x = x1; x < x2; x++, sp++, dp++) {
		if (*sp != 0x0) {
		    Blt_Pixel color;

		    color.u32 = Blt_PaintBrush_GetAssociatedColor(brushPtr, 
                        x, y);
                    Blt_FadeColor(&color, *sp);
		    BlendPixels(dp, &color);
		}
	    }
	    srcRowPtr += slot->bitmap.pitch;
	    destRowPtr += destPtr->pixelsPerRow;
	}
    }
}

static void
CopyGrayGlyph(
    Pict *destPtr, 
    FT_GlyphSlot slot, 
    int xx, int yy, 
    Blt_PaintBrush *brushPtr)
{
    int x1, y1, x2, y2;

#ifdef notdef
    fprintf(stderr, "x=%d, y=%d\n", x, y);
    fprintf(stderr, "  slot.bitmap.width=%d\n", slot->bitmap.width);
    fprintf(stderr, "  slot.bitmap.rows=%d\n", slot->bitmap.rows);
    fprintf(stderr, "  slot.bitmap_left=%d\n", slot->bitmap_left);
    fprintf(stderr, "  slot.bitmap_top=%d\n", slot->bitmap_top);
    fprintf(stderr, "  slot.bitmap.pixel_mode=%x\n", slot->bitmap.pixel_mode);
    fprintf(stderr, "  slot.advance.x=%d\n", slot->advance.x);
    fprintf(stderr, "  slot.advance.y=%d\n", slot->advance.y);
    
    fprintf(stderr, "  slot.format=%c%c%c%c\n", 
	    (slot->format >> 24) & 0xFF, 
	    (slot->format >> 16) & 0xFF, 
	    (slot->format >> 8) & 0xFF, 
	    (slot->format & 0xFF));
#endif
    if ((xx >= destPtr->width) || ((xx + slot->bitmap.width) <= 0) ||
	(yy >= destPtr->height) || ((yy + slot->bitmap.rows) <= 0)) {
	return;			/* No portion of the glyph is visible in the
				 * picture. */
    }

    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (xx < 0) {			/* Left side of glyph overhangs. */
	x1 -= xx;		
	x2 += xx;
	xx = 0;		
    }
    if (yy < 0) {			/* Top of glyph overhangs. */
	y1 -= yy;
	y2 += yy;
	yy = 0;
    }
    if ((xx + x2) > destPtr->width) {	/* Right side of glyph overhangs. */
	x2 = destPtr->width - xx;
    }
    if ((yy + y2) > destPtr->height) {	/* Bottom of glyph overhangs. */
	y2 = destPtr->height - yy;
    }

    {
	Blt_Pixel *destRowPtr;
	unsigned char *srcRowPtr;
	int y;

	srcRowPtr = slot->bitmap.buffer + ((y1 * slot->bitmap.pitch) + x1);
	destRowPtr = Blt_Picture_Pixel(destPtr, xx, yy);
	for (y = y1; y < y2; y++) {
	    Blt_Pixel *dp;
	    unsigned char *sp;
	    int x;

	    dp = destRowPtr;
	    sp = srcRowPtr;
	    for (x = x1; x < x2; x++, sp++, dp++) {
		if (*sp != 0x0) {
		    int t;
		    Blt_Pixel color;

		    color.u32 = Blt_PaintBrush_GetAssociatedColor(brushPtr, 
                        x, y);
		    color.Alpha = imul8x8(*sp, color.Alpha, t);
		    dp->u32 = color.u32;
		}
	    }
	    srcRowPtr += slot->bitmap.pitch;
	    destRowPtr += destPtr->pixelsPerRow;
	}
    }
}

static void
PaintGrayGlyph(
    Pict *destPtr, 
    FT_GlyphSlot slot, 
    int xx, int yy, 
    Blt_PaintBrush *brushPtr)
{
    int x1, y1, x2, y2;

#ifdef notdef
    fprintf(stderr, "x=%d, y=%d\n", x, y);
    fprintf(stderr, "  slot.bitmap.width=%d\n", slot->bitmap.width);
    fprintf(stderr, "  slot.bitmap.rows=%d\n", slot->bitmap.rows);
    fprintf(stderr, "  slot.bitmap_left=%d\n", slot->bitmap_left);
    fprintf(stderr, "  slot.bitmap_top=%d\n", slot->bitmap_top);
    fprintf(stderr, "  slot.bitmap.pixel_mode=%x\n", slot->bitmap.pixel_mode);
    fprintf(stderr, "  slot.advance.x=%d\n", slot->advance.x);
    fprintf(stderr, "  slot.advance.y=%d\n", slot->advance.y);
    
    fprintf(stderr, "  slot.format=%c%c%c%c\n", 
	    (slot->format >> 24) & 0xFF, 
	    (slot->format >> 16) & 0xFF, 
	    (slot->format >> 8) & 0xFF, 
	    (slot->format & 0xFF));
#endif
    if ((xx >= destPtr->width) || ((xx + slot->bitmap.width) <= 0) ||
	(yy >= destPtr->height) || ((yy + slot->bitmap.rows) <= 0)) {
	return;				/* No portion of the glyph is visible
					 * in the picture. */
    }

    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (xx < 0) {			/* Left side of glyph overhangs. */
	x1 -= xx;		
	x2 += xx;
	xx = 0;		
    }
    if (yy < 0) {			/* Top of glyph overhangs. */
	y1 -= yy;
	y2 += yy;
	yy = 0;
    }
    if ((xx + x2) > destPtr->width) {	/* Right side of glyph overhangs. */
	x2 = destPtr->width - xx;
    }
    if ((yy + y2) > destPtr->height) {	/* Bottom of glyph overhangs. */
	y2 = destPtr->height - yy;
    }

    {
	Blt_Pixel *destRowPtr;
	unsigned char *srcRowPtr;
	int y;

	srcRowPtr = slot->bitmap.buffer + ((y1 * slot->bitmap.pitch) + x1);
	destRowPtr = Blt_Picture_Pixel(destPtr, xx, yy);
	for (y = y1; y < y2; y++) {
	    Blt_Pixel *dp;
	    unsigned char *sp;
	    int x;

	    dp = destRowPtr;
	    sp = srcRowPtr;
	    for (x = x1; x < x2; x++, sp++, dp++) {
		if (*sp != 0x0) {
		    Blt_Pixel color;

		    color.u32 = Blt_PaintBrush_GetAssociatedColor(brushPtr, 
                        x, y);
                    Blt_FadeColor(&color, *sp);
		    BlendPixels(dp, &color);
		}
	    }
	    srcRowPtr += slot->bitmap.pitch;
	    destRowPtr += destPtr->pixelsPerRow;
	}
    }
}

static void
CopyMonoGlyph(Pict *destPtr, FT_GlyphSlot slot, int xx, int yy,
	  Blt_PaintBrush *brushPtr)
{
    int x1, y1, x2, y2;

#ifdef notdef
    fprintf(stderr, "dx=%d, dy=%d\n", dx, dy);
    fprintf(stderr, "  slot.bitmap.width=%d\n", slot->bitmap.width);
    fprintf(stderr, "  slot.bitmap.rows=%d\n", slot->bitmap.rows);
    fprintf(stderr, "  slot.bitmap_left=%d\n", slot->bitmap_left);
    fprintf(stderr, "  slot.bitmap_top=%d\n", slot->bitmap_top);
    fprintf(stderr, "  slot.bitmap.pixel_mode=%x\n", slot->bitmap.pixel_mode);
    fprintf(stderr, "  slot.advance.x=%d\n", slot->advance.x);
    fprintf(stderr, "  slot.advance.y=%d\n", slot->advance.y);
    fprintf(stderr, "  slot.format=%c%c%c%c\n", 
	    (slot->format >> 24) & 0xFF, 
	    (slot->format >> 16) & 0xFF, 
	    (slot->format >> 8) & 0xFF, 
	    (slot->format & 0xFF));
#endif
    
    if ((xx >= destPtr->width) || ((xx + slot->bitmap.width) <= 0) ||
	(yy >= destPtr->height) || ((yy + slot->bitmap.rows) <= 0)) {
	return;				/* No portion of the glyph is visible
					 * in the picture. */
    }
    /* By default, set the region to cover the entire glyph */
    x1 = y1 = 0;
    x2 = slot->bitmap.width;
    y2 = slot->bitmap.rows;

    /* Determine the portion of the glyph inside the picture. */

    if (xx < 0) {			/* Left side of glyph overhangs. */
	x1 -= xx;		
	x2 += xx;
	xx = 0;		
    }
    if (yy < 0) {			/* Top of glyph overhangs. */
	y1 -= yy;
	y2 += yy;
	yy = 0;
    }
    if ((xx + x2) > destPtr->width) {	/* Right side of glyph overhangs. */
	x2 = destPtr->width - xx;
    }
    if ((yy + y2) > destPtr->height) {	/* Bottom of glyph overhangs. */
	y2 = destPtr->height - yy;
    }
    {
	Blt_Pixel *destRowPtr;
	unsigned char *srcRowPtr;
	int y;

	srcRowPtr = slot->bitmap.buffer + (y1 * slot->bitmap.pitch);
	destRowPtr = Blt_Picture_Pixel(destPtr, xx, yy);
	for (y = y1; y < y2; y++) {
	    Blt_Pixel *dp;
	    int x;
	    
	    dp = destRowPtr;
	    for (x = x1; x < x2; x++, dp++) {
		int pixel;

		pixel = srcRowPtr[x >> 3] & (1 << (7 - (x & 0x7)));
		if (pixel != 0x0) {
		    dp->u32 = Blt_PaintBrush_GetAssociatedColor(brushPtr, x, y);
		}
	    }
	    srcRowPtr += slot->bitmap.pitch;
	    destRowPtr += destPtr->pixelsPerRow;
	}
    } 
}

static int 
ScaleFont(Tcl_Interp *interp, FtFont *fontPtr, FT_F26Dot6 size)
{
    FT_Error ftError;
    int xdpi, ydpi;

    Blt_ScreenDPI(Tk_MainWindow(interp), &xdpi, &ydpi);
    ftError = FT_Set_Char_Size(fontPtr->face, size, size, xdpi, ydpi);
    if (ftError) {
	Tcl_AppendResult(interp, "can't set font size to \"", Blt_Itoa(size), 
		"\": ", FtError(ftError), (char *)NULL);
	return TCL_ERROR;
    }
    fontPtr->height  = fontPtr->face->size->metrics.height >> 6;
    fontPtr->ascent  = fontPtr->face->size->metrics.ascender >> 6;
    fontPtr->descent = fontPtr->face->size->metrics.descender >> 6;
    return TCL_OK;
}

static void
RotateFont(FtFont *fontPtr, float angle)
{
    /* Set up the transformation matrix. */
    double theta; 

    theta = angle * DEG2RAD;
    fontPtr->matrix.yy = fontPtr->matrix.xx = (FT_Fixed)(cos(theta) * 65536.0);
    fontPtr->matrix.yx = (FT_Fixed)(sin(theta) * 65536.0);
    fontPtr->matrix.xy = -fontPtr->matrix.yx;
}

#if DRAWTEXT
static FtFont *
OpenFont(Tcl_Interp *interp, Tcl_Obj *objPtr, size_t fontSize) 
{
    FtFont *fontPtr;
    FT_Error ftError;
    FT_Face face;
    const char *fileName, *fontName;
    Tcl_Obj *fileObjPtr;

    if (ftLibrary == NULL) {
	ftError = FT_Init_FreeType(&ftLibrary);
	if (ftError) {
	    Tcl_AppendResult(interp, "can't initialize freetype library: ", 
			     FtError(ftError), (char *)NULL);
	    return NULL;
	}
    }
    fontName = Tcl_GetString(objPtr);
    fileObjPtr = NULL;
    face = NULL;
    if (fontName[0] == '@') {
	fileName = fontName + 1;
	fontSize <<= 6;
    } else {
	double size;

	fileObjPtr = Blt_Font_GetFile(interp, objPtr, &size);
	if (fileObjPtr == NULL) {
	    return NULL;
	}
	if (fontSize == 0) {
	    fontSize = (int)(size * 64.0 + 0.5);
	}
	Tcl_IncrRefCount(fileObjPtr);
	fileName = Tcl_GetString(fileObjPtr);
    }
    ftError = FT_New_Face(ftLibrary, fileName, 0, &face);
    if (ftError) {
	Tcl_AppendResult(interp, "can't create face from font file \"", 
		fileName, "\": ", FtError(ftError), (char *)NULL);
	goto error;
    }
    if (!FT_IS_SCALABLE(face)) {
	Tcl_AppendResult(interp, "can't use font \"", fontName, 
			 "\": font isn't scalable.", (char *)NULL);
	goto error;
    }
    if (fileObjPtr != NULL) {
	Tcl_DecrRefCount(fileObjPtr);
    }
    fontPtr = Blt_AssertCalloc(1, sizeof(FtFont));
    fontPtr->face = face;
    if (ScaleFont(interp, fontPtr, fontSize) != TCL_OK) {
	Blt_Free(fontPtr);
	goto error;
    }
    RotateFont(fontPtr, 0.0f);		/* Initializes the rotation matrix. */
    return fontPtr;
 error:
    if (fileObjPtr != NULL) {
	Tcl_DecrRefCount(fileObjPtr);
    }
    if (face != NULL) {
	FT_Done_Face(face);
    }	
    return NULL;
}

#else 

static FtFont *
OpenFont(Tcl_Interp *interp, Tcl_Obj *objPtr, size_t fontSize) 
{
    Tcl_AppendResult(interp, "freetype library not available", (char *)NULL);
    return NULL;
}

#endif /*DRAWTEXT*/

static void
CloseFont(FtFont *fontPtr) 
{
#ifdef HAVE_LIBXFT
    if (fontPtr->xftFont != NULL) {
	XftUnlockFace(fontPtr->xftFont);
    } else {
	FT_Done_Face(fontPtr->face);
    }
#else
    FT_Done_Face(fontPtr->face);
#endif /* HAVE_LIBXFT */ 
    Blt_Free(fontPtr);
}


static int
PaintText(
    Pict *destPtr,
    FtFont *fontPtr, 
    const char *string,
    size_t length,
    int x, int y,			/* Anchor coordinates of text. */
    int kerning,
    Blt_PaintBrush *brushPtr)
{
#if DRAWTEXT
    FT_Error ftError;
    int h;

    FT_Vector pen;			/* Untransformed origin  */
    const char *p, *pend;
    FT_GlyphSlot slot;
    FT_Face face;			/* Face object. */  

    h = destPtr->height;
    face = fontPtr->face;
    slot = face->glyph;
    int yy;
    int previous;

    if (destPtr->flags & BLT_PIC_ASSOCIATED_COLORS) {
	Blt_UnassociateColors(destPtr);
    }
#ifdef notdef
    fprintf(stderr, 
	    "num_faces=%d\n"
	    "face_flags=%x\n"
	    "style_flags=%x\n"
	    "num_glyphs=%d\n"
	    "family_name=%s\n"
	    "style_name=%s\n"
	    "num_fixed_sizes=%d\n"
	    "num_charmaps=%d\n"
	    "units_per_EM=%d\n"
	    "face->size->metrics.height=%d\n"
	    "face->size->metrics.ascender=%d\n"
	    "face->size->metrics.descender=%d\n"
	    "ascender=%d\n"
	    "descender=%d\n"
	    "height=%d\n"
	    "max_advance_width=%d\n"
	    "max_advance_height=%d\n"
	    "underline_position=%d\n"
	    "underline_thickness=%d\n",
	    face->num_faces,
	    face->face_flags,
	    face->style_flags,
	    face->num_glyphs,
	    face->family_name,
	    face->style_name,
	    face->num_fixed_sizes,
	    face->num_charmaps,
	    face->units_per_EM,
	    face->size->metrics.height,
	    face->size->metrics.ascender,
	    face->size->metrics.descender,
	    face->ascender,
	    face->descender,
	    face->height,
	    face->max_advance_width,
	    face->max_advance_height,
	    face->underline_position,
	    (int)face->underline_thickness);
#endif
    yy = y;
    previous = -1;
    FT_Set_Transform(face, &fontPtr->matrix, NULL);
    pen.y = (h - y) << 6;
    pen.x = x << 6;
    for (p = string, pend = p + length; p < pend; p++) {
	int current;

	current = FT_Get_Char_Index(face, *p);
	if ((kerning) && (previous >= 0)) { 
	    FT_Vector delta; 
		
	    FT_Get_Kerning(face, previous, current, FT_KERNING_DEFAULT, &delta);
	    pen.x += delta.x; 
	} 
	FT_Set_Transform(face, &fontPtr->matrix, &pen);
	previous = current;
	/* load glyph image into the slot (erase previous one) */  
	ftError = FT_Load_Glyph(face, current, FT_LOAD_DEFAULT); 
	if (ftError) {
	    Blt_Warn("can't load character \"%c\": %s\n", *p, 
		     FtError(ftError));
	    continue;                 /* Ignore errors */
	}
	ftError = FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
	if (ftError) {
	    Blt_Warn("can't render glyph \"%c\": %s\n", *p, FtError(ftError));
	    continue;                 /* Ignore errors */
	}
	switch(slot->bitmap.pixel_mode) {
	case FT_PIXEL_MODE_MONO:
	    CopyMonoGlyph(destPtr, slot, pen.x >> 6, yy - slot->bitmap_top, 
		brushPtr);
	    break;
	case FT_PIXEL_MODE_LCD:
	case FT_PIXEL_MODE_LCD_V:
	case FT_PIXEL_MODE_GRAY:
#ifdef notdef
	    fprintf(stderr, "h=%d, slot->bitmap_top=%d\n", h, slot->bitmap_top);
#endif
	    PaintGrayGlyph(destPtr, slot, slot->bitmap_left, 
			   h - slot->bitmap_top, brushPtr);
	case FT_PIXEL_MODE_GRAY2:
	case FT_PIXEL_MODE_GRAY4:
	    break;
	}
	pen.x += slot->advance.x; 
	pen.y += slot->advance.y;
	previous = -1;
    }	
#endif /* HAVE_FT2BUILD_H */
    return TCL_OK;
}

#ifdef notdef
static void
PaintTextShadow(
    Blt_Picture picture,
    Tcl_Interp *interp,
    const char *string,
    int x, int y,			/* Anchor coordinates of text. */

    TextSwitches *switchesPtr,
    int offset)
{
    int w, h;
    Blt_Picture blur;
    Blt_Pixel color;

    MeasureText(FT_Face face, char *string, float angle, int *widthPtr, 
	    int *heightPtr)
    w = (width + offset*2);
    h = (height + offset*2);
    blur = Blt_CreatePicture(w, h);
    Blt_BlankPicture(blur, 0x0);
    Blt_PaintBrush_Init(&brush);
    Blt_PaintBrush_SetColor(&brush, 0xA0000000);

    PaintText(blur, fontPtr, string, x+offset/2, y+offset/2, 0, &brush);

    PaintText(blur, interp, string, x+offset/2, y+offset/2, 
	      switchPtr->brushPtr);
    Blt_BlurPicture(blur, blur, offset, 3);
    Blt_BlendRegion(picture, blur, 0, 0, w, h, x+offset/2, y+offset/2);
    Blt_FreePicture(blur);
}
#endif
#endif /*HAVE_FT2BUILD_H*/

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
	dp--;			/* x - 1 */
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

static void
BrushHorizontalLine(Pict *destPtr, int x1, int x2, int y, 
		    Blt_PaintBrush *brushPtr)  
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

	color.u32 = Blt_PaintBrush_GetAssociatedColor(brushPtr, x, y);
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
 *	drawproc(y, xl, xr)
 *	int y, xl, xr;
 *	{
 *	    int x;
 *	    for (x=xl; x<=xr; x++)
 *		pixel_write(x, y, pixelvalue);
 *	}
 *
 *  Paul Heckbert	30 June 81, 18 Dec 89
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
static int n;				/* # of vertices */
static Point2f *pt;			/* vertices */

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
cdelete(AET *tablePtr, int i)		/* Remove edge i from active
					 * list. */
{
    int j;

    for (j=0; (j < tablePtr->numActive) && (tablePtr->active[j].i != i); j++) {
	/*empty*/
    }
    if (j >= tablePtr->numActive) {
	return;				/* Edge not in active list; happens
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
		 Blt_PaintBrush *brushPtr)
{
    int y, k;
    int top, bot;
    int *map;				/* List of vertex indices, sorted by
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

    aet.numActive = 0;			/* start with empty active list */
    k = 0;				/* map[k] is next vertex to process */
    top = MAX(0, ceil(vertices[map[0]].y-.5)); /* ymin of polygon */
    bot = MIN(destPtr->height-1, floor(vertices[map[n-1]].y-.5)); /* ymax */

    for (y = top; y <= bot; y++) {	/* step through scanlines */
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
		BrushHorizontalLine(destPtr, left, right, y, brushPtr);
	    }
	    p->x += p->dx;		/* increment edge coords */
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

static void
PaintPolygonAA(Pict *destPtr, size_t numVertices, Point2f *vertices, 
	       Region2f *regionPtr, Blt_PaintBrush *brushPtr)
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
    Blt_PaintPolygon(big, numVertices, v, brushPtr);
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
    Blt_PaintBrush_Init(&brush);
    Blt_PaintBrush_SetColor(&brush, shadowPtr->color.u32);
    GetPolygonBoundingBox(numVertices, v, &r2);
    Blt_PaintPolygon(tmp, numVertices, v, &brush);
    if (v != vertices) {
	Blt_Free(v);
    }
    blur = Blt_CreatePicture(w, h);
    Blt_BlankPicture(blur, 0x0);
    Blt_CopyPictureBits(blur, tmp, 0, 0, w, h, shadowPtr->offset*2, 
			shadowPtr->offset*2); 
    Blt_BlurPicture(blur, blur, shadowPtr->width, 3);
    Blt_MaskPicture(blur, tmp, 0, 0, w, h, 0, 0, &shadowPtr->color);
    Blt_FreePicture(tmp);
    Blt_BlendRegion(destPtr, blur, 0, 0, w, h, x1, y1);
    Blt_FreePicture(blur);
}

static void
PaintPolygonAA2(Pict *destPtr, size_t numVertices, Point2f *vertices, 
		Region2f *regionPtr, Blt_PaintBrush *brushPtr, 
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
    if ((shadowPtr != NULL) && (shadowPtr->offset > 0)) {
	PaintPolygonShadow(big, numVertices, vertices, &r2, shadowPtr);
    }
    Blt_PaintPolygon(big, numVertices, vertices, brushPtr);
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

    filled = (switchesPtr->brushPtr != NULL);
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
	Blt_CopyPictureBits(picture, tmpPtr, 0, 0, w, h, x-w/2, y-h/2);
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
DrawCircleShadow(Blt_Picture picture, int x, int y, float radius, 
		 float lineWidth, int blend, Blt_Shadow *shadowPtr)
{
    Pict *tmpPtr;
    int w, h;
    Blt_PaintBrush brush;

    w = h = (radius * 2) + 1 + (shadowPtr->width + shadowPtr->offset) * 2;
    tmpPtr = Blt_CreatePicture(w, h);
#ifdef notdef
    fprintf(stderr, "radius=%g linewidth=%g offset=%d width=%d w=%d h=%d\n",
	    radius, lineWidth, shadowPtr->offset, shadowPtr->width,
	    w, h);
#endif
    Blt_BlankPicture(tmpPtr, 0x0);
    Blt_PaintBrush_Init(&brush);
    Blt_PaintBrush_SetColor(&brush, shadowPtr->color.u32);
    PaintCircle4(tmpPtr, 
		 radius + shadowPtr->offset, 
		 radius + shadowPtr->offset, 
		 radius, 
		 lineWidth, 
		 &brush, 
		 TRUE);
    if (blend) {
	Blt_BlurPicture(tmpPtr, tmpPtr, shadowPtr->width, 4);
	Blt_BlendRegion(picture, tmpPtr, 0, 0, w, h, 
			x - radius - shadowPtr->offset,
			y - radius - shadowPtr->offset);
    } else {
	Blt_CopyPictureBits(picture, tmpPtr, 0, 0, w, h, 
			  x - radius - shadowPtr->offset,
			  y - radius - shadowPtr->offset);
    }
    Blt_FreePicture(tmpPtr);
}

static void
DrawCircle(Blt_Picture picture, int x, int y, int r, float lineWidth, 
	   Blt_PaintBrush *brushPtr, int blend)
{
    PaintCircle4(picture, x, y, r, lineWidth, brushPtr, blend);
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_Picture_CircleOp --
 *
 * Results:
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *	$img draw circle x y r ?switches?
 *	  -linewidth 1
 *	  -color color 
 *        -shadow shadow
 *	  -antialiased bool
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
    Blt_PaintBrush *brushPtr;

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
    if (Blt_PaintBrush_GetFromString(interp, "white", &brushPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    switches.brushPtr = brushPtr;
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
    Blt_PaintBrush_Region(switches.brushPtr, x - radius, y - radius, 
	radius + radius, radius + radius);
    DrawCircle(picture, x, y, radius, switches.lineWidth, switches.brushPtr, 
	       switches.blend);
    Blt_FreeSwitches(circleSwitches, (char *)&switches, 0);
    return TCL_OK;
}


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
		 switchesPtr->brushPtr, 0);
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
		 switchesPtr->brushPtr, 0);
    destPtr->flags &= ~BLT_PIC_ASSOCIATED_COLORS;
    return destPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * Circle2Op --
 *
 * Results:
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *	$img draw circle x y r ?switches?
 *	  -linewidth 1
 *	  -color color 
 *        -shadow shadow
 *	  -antialiased bool
 *	
 *---------------------------------------------------------------------------
 */
static int
CircleOp2(
    Blt_Picture picture,
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* # of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    CircleSwitches2 switches;
    int x, y, r;
    Blt_Picture outline, fill, shadow;
    Pict *mergePtr;
    Blt_PaintBrush *brushPtr;

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
    if (Blt_PaintBrush_GetFromString(interp, "white", &brushPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    switches.brushPtr = brushPtr;
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

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Picture_EllipseOp --
 *
 * Results:
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
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
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
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
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *	$pict draw polygon -coords $coords -color $color 
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
    Blt_PaintBrush *brushPtr;

    if (Blt_PaintBrush_GetFromString(interp, "black", &brushPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    switches.brushPtr = brushPtr;
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
			switches.brushPtr, &switches.shadow);
	    } else {
		if (switches.shadow.width > 0) {
		    PaintPolygonShadow(destPtr, numVertices, vertices, &r, 
				       &switches.shadow);
		}
		Blt_PaintBrush_Region(switches.brushPtr, r.left, r.top, 
				 r.right - r.left, r.bottom - r.top);
		Blt_PaintPolygon(destPtr, numVertices, vertices, 
			switches.brushPtr);
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
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Picture_RectangleOp(ClientData clientData, Tcl_Interp *interp, int objc,
			Tcl_Obj *const *objv)	
{
    Blt_Picture picture = clientData;
    RectangleSwitches switches;
    PictRegion r;
    Blt_PaintBrush *brushPtr;
    
    if (Blt_GetBBoxFromObjv(interp, 4, objv + 3, &r) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_PaintBrush_GetFromString(interp, "black", &brushPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    /* Process switches  */
    switches.brushPtr = brushPtr;
    switches.lineWidth = 0;
    if (Blt_ParseSwitches(interp, rectangleSwitches, objc - 7, objv + 7, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    Blt_PaintBrush_Region(switches.brushPtr, r.x, r.y, r.w, r.h);
    if (switches.shadow.width > 0) {
	PaintRectangleShadow(picture, r.x, r.y, r.w, r.h, switches.radius, 
		switches.lineWidth, &switches.shadow);
    }
    Blt_PaintRectangle(picture, r.x, r.y, r.w, r.h, switches.radius, 
	switches.lineWidth, switches.brushPtr);
    Blt_FreeSwitches(rectangleSwitches, (char *)&switches, 0);
    return TCL_OK;
}


static void
PaintTextLayout(Pict *destPtr, FtFont *fontPtr, TextLayout *layoutPtr, 
		int x, int y, TextSwitches *switchesPtr)
{
    TextFragment *fp, *fend;

    for (fp = layoutPtr->fragments, fend = fp + layoutPtr->numFragments;
	 fp < fend; fp++) {
	PaintText(destPtr, fontPtr, fp->text, fp->count, x + fp->sx, 
		y + fp->sy, switchesPtr->kerning, switchesPtr->brushPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Picture_TextOp --
 *
 * Results:
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *	image draw text string x y switches 
 *---------------------------------------------------------------------------
 */
int
Blt_Picture_TextOp(ClientData clientData, Tcl_Interp *interp, int objc,
		   Tcl_Obj *const *objv) 
{ 
#if DRAWTEXT
    Pict *destPtr = clientData;
    Blt_Shadow *shadowPtr;
    FtFont *fontPtr;
    TextSwitches switches;
    const char *string;
    int length;
    int x, y;

    string = Tcl_GetStringFromObj(objv[3], &length);
    if ((Tcl_GetIntFromObj(interp, objv[4], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[5], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    /* Process switches  */
    switches.anchor = TK_ANCHOR_NW;
    switches.angle = 0.0;
    Blt_Shadow_Set(&switches.shadow, 0, 0, 0x0, 0xA0);
    if (Blt_PaintBrush_GetFromString(interp, "black", &switches.brushPtr) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    switches.fontObjPtr = Tcl_NewStringObj("Arial 12", -1);
    switches.fontSize = 0;
    switches.kerning = FALSE;
    switches.justify = TK_JUSTIFY_LEFT;
    shadowPtr = &switches.shadow;
    if (Blt_ParseSwitches(interp, textSwitches, objc - 6, objv + 6, &switches, 
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    fontPtr = OpenFont(interp, switches.fontObjPtr, switches.fontSize);
    if (fontPtr == NULL) {
	return TCL_ERROR;
    }
    if ((destPtr->flags & BLT_PIC_ASSOCIATED_COLORS) == 0) {
	Blt_AssociateColors(destPtr);
    }
    if (switches.angle != 0.0) {
	TextStyle ts;
	TextLayout *layoutPtr;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetJustify(ts, switches.justify);
	layoutPtr = CreateSimpleTextLayout(fontPtr, string, length, &ts);
	if (fontPtr->face->face_flags & FT_FACE_FLAG_SCALABLE) {
	    double rw, rh;
	    TextFragment *fp, *fend;

	    Blt_RotateStartingTextPositions(layoutPtr, switches.angle);
	    Blt_GetBoundingBox(layoutPtr->width, layoutPtr->height, 
		switches.angle, &rw, &rh, (Point2d *)NULL);
	    Blt_TranslateAnchor(x, y, (int)(rw), (int)(rh), switches.anchor, 
		&x, &y);
	    RotateFont(fontPtr, switches.angle);
	    for (fp = layoutPtr->fragments, fend = fp + layoutPtr->numFragments;
		fp < fend; fp++) {
		PaintText(destPtr, fontPtr, fp->text, fp->count, x + fp->sx, 
			y + fp->sy, switches.kerning, switches.brushPtr);
	    }
	} else {
	    Blt_Picture tmp;
	    Pict *rotPtr;
	    int i;
	    
	    tmp = Blt_CreatePicture(layoutPtr->width, layoutPtr->height);
	    Blt_BlankPicture(tmp, 0x00FF0000);
	    for (i = 0; i < layoutPtr->numFragments; i++) {
		TextFragment *fp;

		fp = layoutPtr->fragments + i;
		PaintText(tmp, fontPtr, fp->text, fp->count, fp->sx, 
			fp->sy, switches.kerning, switches.brushPtr);
	    }
	    rotPtr = Blt_RotatePicture(tmp, switches.angle);
	    Blt_FreePicture(tmp);
	    Blt_TranslateAnchor(x, y, rotPtr->width, rotPtr->height, 
		switches.anchor, &x, &y);
	    Blt_BlendRegion(destPtr, rotPtr, 0, 0, rotPtr->width, 
			    rotPtr->height, x, y);
	    Blt_FreePicture(rotPtr);
	}
	Blt_Free(layoutPtr);
    } else { 
	int w, h;
	TextStyle ts;
	TextLayout *layoutPtr;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetJustify(ts, switches.justify);
	layoutPtr = CreateSimpleTextLayout(fontPtr, string, length, &ts);
	Blt_TranslateAnchor(x, y, layoutPtr->width, layoutPtr->height, 
		switches.anchor, &x, &y);
	w = layoutPtr->width;
	h = layoutPtr->height;
	if ((w > 1) && (h > 1) && (switches.shadow.width > 0)) {
	    Blt_Pixel color;
	    Pict *tmpPtr;
	    int extra;
	    Blt_PaintBrush brush;
	    int i;

	    extra = 2 * shadowPtr->width;
	    tmpPtr = Blt_CreatePicture(w + extra, h + extra);
	    color.u32 = shadowPtr->color.u32;
	    color.Alpha = 0x0;
	    Blt_BlankPicture(tmpPtr, color.u32);
	    Blt_PaintBrush_Init(&brush);
	    Blt_PaintBrush_SetColor(&brush, shadowPtr->color.u32);
	    for (i = 0; i < layoutPtr->numFragments; i++) {
		TextFragment *fp;

		fp = layoutPtr->fragments + i;
		PaintText(tmpPtr, fontPtr, fp->text, fp->count, 
			  fp->x + shadowPtr->width, 
			  fp->y + shadowPtr->width, 
			  switches.kerning, &brush);
	    }
	    Blt_BlurPicture(tmpPtr, tmpPtr, shadowPtr->width, 3);
	    Blt_BlendRegion(destPtr, tmpPtr, 0, 0, tmpPtr->width, 
			    tmpPtr->height, x, y);
	    for (i = 0; i < layoutPtr->numFragments; i++) {
		TextFragment *fp;

		fp = layoutPtr->fragments + i;
		PaintText(destPtr, fontPtr, fp->text, fp->count, 
		    x + fp->x + shadowPtr->width - shadowPtr->offset,
		    y + fp->y + shadowPtr->width - shadowPtr->offset, 
                    switches.kerning, switches.brushPtr);
	    }
	    Blt_FreePicture(tmpPtr);
	} else {
	    int i;

	    for (i = 0; i < layoutPtr->numFragments; i++) {
		TextFragment *fp;

		fp = layoutPtr->fragments + i;
		PaintText(destPtr, fontPtr, fp->text, fp->count, 
			x + fp->x, y + fp->y, switches.kerning, 
			switches.brushPtr);
	    }
	}
	Blt_Free(layoutPtr);
    }
    CloseFont(fontPtr);
    Blt_FreeSwitches(textSwitches, (char *)&switches, 0);
#else
    Tcl_AppendResult(interp, "text operation not allowed", (char *)NULL);
    return TCL_ERROR;
#endif
    return TCL_OK;
}

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
    if (dx == 0) {		/*  Vertical line */
	Blt_Pixel *dp;

	dp = Blt_Picture_Pixel(destPtr, x1, y1);
	while (dy-- > 0) {
	    dp += destPtr->pixelsPerRow;
	    dp->u32 = colorPtr->u32;
	}
	return;
    }
    if (dy == 0) {		/* Horizontal line */
	Blt_Pixel *dp;

	dp = Blt_Picture_Pixel(destPtr, x1, y1);
	while(dx-- > 0) {
	    dp += xDir;
	    dp->u32 = colorPtr->u32;
	}
	return;
    }
    if (dx == dy) {		/* Diagonal line. */
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
    if (dy > dx) {		/* y-major line */
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
    } else {			/* x-major line */
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
    Blt_PaintBrush_Init(&brush);
    Blt_BlankPicture(destPtr, 0x0);
    x = y = 0;
    if (fillColorPtr != NULL) {
	Blt_PaintBrush_SetColor(&brush, Blt_XColorToPixel(fillColorPtr));
	Blt_PaintRectangle(destPtr, x+1, y+1, w-2, h-2, 0, 0, &brush);
    }
    if (outlineColorPtr != NULL) {
	Blt_PaintBrush_SetColor(&brush, Blt_XColorToPixel(outlineColorPtr));
	Blt_PaintRectangle(destPtr, x, y, w, h, 0, 1, &brush);
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
	shadow.width = shadow.offset = 2;
	r.left = x, r.right = x + w;
	r.top = y, r.bottom = y + h;
	Blt_PaintBrush_Init(&brush);
	Blt_PaintBrush_SetColor(&brush, Blt_XColorToPixel(checkColorPtr));
	PaintPolygonAA2(destPtr, 7, points, &r, &brush, &shadow);
    }
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
    Blt_PaintBrush_Init(&brush);
    Blt_PaintBrush_SetColor(&brush, Blt_XColorToPixel(fillColorPtr));
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
    DrawCircle(destPtr, x, y, r, 0.0, &brush, TRUE);
    if (fill.u32 != outline.u32) {
	Blt_PaintBrush_SetColor(&brush, Blt_XColorToPixel(outlineColorPtr));
	DrawCircle(destPtr, x, y, r, 1.0, &brush, TRUE);
    }
    if (on) {
	r -= 2;
	if (r < 1) {
	    r = 2;
	}
	Blt_PaintBrush_SetColor(&brush, Blt_XColorToPixel(indicatorColorPtr));
	DrawCircle(destPtr, x, y, r, 0.0, &brush, TRUE);
    }
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
    Blt_PaintBrush brush;
    unsigned int normal, light, dark;

    /* Process switches  */
    Blt_PaintBrush_Init(&brush);
    Blt_PaintBrush_SetColor(&brush, Blt_XColorToPixel(fillColorPtr));
    GetShadowColors(bg, &normal, &light, &dark);
    w &= ~1;
    destPtr = Blt_CreatePicture(w, h);
    Blt_BlankPicture(destPtr, normal);
    x = w / 2 + 1;
    y = h / 2 + 1;
    w -= 4, h -= 4;
    r = (w+1) / 2;
    Blt_PaintBrush_SetColor(&brush, dark);
    DrawCircle(destPtr, x-1, y-1, r, 0.0, &brush, TRUE);
    Blt_PaintBrush_SetColor(&brush, light);
    DrawCircle(destPtr, x+1, y+1, r, 0.0, &brush, TRUE);
    /*Blt_BlurPicture(destPtr, destPtr, 1, 3); */
    Blt_PaintBrush_SetColor(&brush, Blt_XColorToPixel(fillColorPtr));
    DrawCircle(destPtr, x, y, r, 0.0, &brush, TRUE);
    if (on) {
	int r1;

	r1 = (r * 2) / 3;
	if (r1 < 1) {
	    r1 = 2;
	}
	Blt_PaintBrush_SetColor(&brush, Blt_XColorToPixel(indicatorColorPtr));
	DrawCircle(destPtr, x, y, r1, 0.0, &brush, TRUE);
    }
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

    Blt_PaintBrush_Init(&brush);
    Blt_PaintBrush_SetColor(&brush, Blt_XColorToPixel(fillColorPtr));
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
    DrawCircle(picture, x, y, r, 0.0, &brush, FALSE);

    points[0].x = x - 2;
    points[0].y = y - 3;
    points[1].x = x - 3;
    points[1].y = y - 2;
    points[2].x = x + 2;
    points[2].y = y + 3;
    points[3].x = x + 3;
    points[3].y = y + 2;

    Blt_PaintBrush_SetColor(&brush, Blt_XColorToPixel(symbolColorPtr));
    PaintPolygonAA2(picture, 4, points, &reg, &brush, NULL);

    points[0].x = x + 3;
    points[0].y = y - 2;
    points[1].x = x + 2;
    points[1].y = y - 3;
    points[2].x = x - 3;
    points[2].y = y + 2;
    points[3].x = x - 2;
    points[3].y = y + 3;

    PaintPolygonAA2(picture, 4, points, &reg, &brush, NULL);
    return picture;
}

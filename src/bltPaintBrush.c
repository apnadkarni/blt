/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPaintBrush.c --
 *
 * This module creates paintbrush objects for the BLT toolkit.
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

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#include <limits.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltConfig.h"
#include "bltPalette.h"
#include "bltPicture.h"
#include "bltPictInt.h"
#include "bltImage.h"
#include "bltOp.h"
#include "bltInitCmd.h"
#include "bltPaintBrush.h"

#define JCLAMP(c)       ((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))
#define CLAMP(c)        ((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))
#define imul8x8(a,b,t)  ((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))

#define PAINTBRUSH_THREAD_KEY   "BLT PaintBrush Data"
#define REPEAT_MASK \
    (BLT_PAINTBRUSH_REPEAT_NORMAL|BLT_PAINTBRUSH_REPEAT_OPPOSITE)
#define ORIENT_MASK \
    (BLT_PAINTBRUSH_ORIENT_VERTICAL|BLT_PAINTBRUSH_ORIENT_HORIZONTAL)

typedef struct {
    Blt_HashTable instTable;            /* Hash table of paintbrush
                                         * structures keyed by the name. */
    Tcl_Interp *interp;                 /* Interpreter associated with this
                                         * set of background paints. */
    int nextId;                         /* Serial number of the identifier
                                         * to be used for next paintbrush
                                         * created.  */
} PaintBrushCmdInterpData;

typedef int (PaintBrushConfigProc)(Tcl_Interp *interp, Blt_PaintBrush brush);
typedef int (PaintBrushColorProc)(Blt_PaintBrush brush, int x, int y);
typedef void (PaintBrushFreeProc)(Blt_PaintBrush brush);
typedef void (PaintBrushRegionProc)(Blt_PaintBrush brush, int x, int y,
        int w, int h);

struct _Blt_PaintBrushClass {
    int type;
    const char *name;                   /* Class name of paintbrush. */
    PaintBrushConfigProc *configProc;
    PaintBrushRegionProc *initProc;
    PaintBrushColorProc *colorProc;
    PaintBrushFreeProc *freeProc;
};

typedef struct _Blt_PaintBrush PaintBrush;

typedef struct {
    unsigned int flags;                 /* See definitions below. */
    const char *name;                   /* Name of paintbrush. Points to
                                         * hashtable key. */
    Blt_HashEntry *hashPtr;             /* Hash entry of this brush in
                                         * interpreter-specific paintbrush
                                         * table. */
    PaintBrushCmdInterpData *dataPtr;   /* Pointer to interpreter-specific
                                         * data. */
    Tk_Window tkwin;                    /* Main window. Used to query
                                         * background pattern options. */
    Display *display;                   /* Display of this paintbrush. Used
                                         * to free switches. */
    Blt_PaintBrush brush;
    Blt_ConfigSpec *specs;              /* Configuration specifications for
                                         * this type of brush.  */
} PaintBrushCmd;

typedef struct _Blt_PaintBrushNotifier {
    const char *name;                   /* Token id for notifier. */
    Blt_BrushChangedProc *proc;         /* Procedure to be called when the
                                         * paintbrush changes. */
    ClientData clientData;              /* Data to be passed on notifier
                                         * callbacks.  */
} PaintBrushNotifier;

#define DEF_CHECKER_OFFCOLOR    "grey97"
#define DEF_CHECKER_ONCOLOR     "grey90"
#define DEF_CHECKER_STRIDE      "10"
#define DEF_COLOR               STD_NORMAL_BACKGROUND
#define DEF_COLOR_SCALE         "linear"
#define DEF_CONICAL_CENTER       "c"
#define DEF_CONICAL_DIAMETER     "0.0"
#define DEF_CONICAL_HEIGHT       "1.0"
#define DEF_CONICAL_ROTATE       "45.0"
#define DEF_CONICAL_WIDTH        "1.0"
#define DEF_DECREASING          "0"
#define DEF_HIGH_COLOR           "grey50"
#define DEF_TO         (char *)NULL
#define DEF_JITTER              "0"
#define DEF_OPACITY             "100.0"
#define DEF_PALETTE             (char *)NULL
#define DEF_RADIAL_CENTER       "c"
#define DEF_RADIAL_DIAMETER     "0.0"
#define DEF_RADIAL_HEIGHT       "1.0"
#define DEF_RADIAL_WIDTH        "1.0"
#define DEF_REPEAT              "reversing"
#define DEF_LOW_COLOR         "grey90"
#define DEF_FROM       "top center"
#define DEF_STRIPE_OFFCOLOR    "grey97"
#define DEF_STRIPE_ONCOLOR     "grey90"
#define DEF_STRIPE_ORIENT       "vertical"
#define DEF_STRIPE_STRIDE       "2"
#define DEF_XORIGIN             "0"
#define DEF_YORIGIN             "0"

static Blt_OptionParseProc ObjToImage;
static Blt_OptionPrintProc ImageToObj;
static Blt_OptionFreeProc FreeImage;
static Blt_CustomOption imageOption =
{
    ObjToImage, ImageToObj, FreeImage, (ClientData)0
};

static Blt_OptionParseProc ObjToColorScaling;
static Blt_OptionPrintProc ColorScalingToObj;
static Blt_CustomOption colorScalingOption =
{
    ObjToColorScaling, ColorScalingToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToOpacity;
static Blt_OptionPrintProc OpacityToObj;
static Blt_CustomOption opacityOption =
{
    ObjToOpacity, OpacityToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToJitter;
static Blt_OptionPrintProc JitterToObj;
static Blt_CustomOption jitterOption =
{
    ObjToJitter, JitterToObj, NULL, (ClientData)0
};


static Blt_ConfigSpec colorBrushSpecs[] =
{
    {BLT_CONFIG_PIX32, "-color", (char *)NULL, (char *)NULL, DEF_COLOR, 
        Blt_Offset(Blt_ColorBrush, reqColor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL, DEF_JITTER, 
        Blt_Offset(Blt_ColorBrush, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_CUSTOM, "-opacity", (char *)NULL, (char *)NULL, DEF_OPACITY, 
        Blt_Offset(Blt_ColorBrush, alpha),
        BLT_CONFIG_DONT_SET_DEFAULT, &opacityOption},
    {BLT_CONFIG_END}
};


static Blt_ConfigSpec tileBrushSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-image", (char *)NULL, (char *)NULL, (char *)NULL,
        Blt_Offset(Blt_TileBrush, tkImage), BLT_CONFIG_DONT_SET_DEFAULT,
        &imageOption},
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL, DEF_JITTER,
        Blt_Offset(Blt_TileBrush, jitter.range), BLT_CONFIG_DONT_SET_DEFAULT,
        &jitterOption},
    {BLT_CONFIG_CUSTOM, "-opacity", (char *)NULL, (char *)NULL, DEF_OPACITY, 
        Blt_Offset(Blt_TileBrush, alpha), BLT_CONFIG_DONT_SET_DEFAULT,
        &opacityOption},
    {BLT_CONFIG_PIXELS, "-xoffset", (char *)NULL, (char *)NULL, DEF_XORIGIN,
        Blt_Offset(Blt_TileBrush, xOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", (char *)NULL, (char *)NULL, DEF_YORIGIN,
        Blt_Offset(Blt_TileBrush, yOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_OptionParseProc ObjToPosition;
static Blt_OptionPrintProc PositionToObj;
static Blt_CustomOption positionOption =
{
    ObjToPosition, PositionToObj, NULL, (ClientData)0
};
static Blt_OptionParseProc ObjToRepeat;
static Blt_OptionPrintProc RepeatToObj;
static Blt_CustomOption repeatOption =
{
    ObjToRepeat, RepeatToObj, NULL, (ClientData)0
};
static Blt_OptionFreeProc FreePalette;
static Blt_OptionParseProc ObjToPalette;
static Blt_OptionPrintProc PaletteToObj;
static Blt_CustomOption paletteOption =
{
    ObjToPalette, PaletteToObj, FreePalette, (ClientData)0
};

static Blt_ConfigSpec linearGradientBrushSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-colorscale", (char *)NULL, (char *)NULL,
        DEF_COLOR_SCALE, Blt_Offset(Blt_LinearGradientBrush, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, &colorScalingOption},
    {BLT_CONFIG_BITMASK, "-decreasing", (char *)NULL, (char *)NULL,
        DEF_DECREASING, Blt_Offset(Blt_LinearGradientBrush, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)BLT_PAINTBRUSH_DECREASING},
    {BLT_CONFIG_CUSTOM, "-from", (char *)NULL, (char *)NULL,
        DEF_FROM, Blt_Offset(Blt_LinearGradientBrush, from), 
        BLT_CONFIG_DONT_SET_DEFAULT, &positionOption},
    {BLT_CONFIG_PIX32, "-highcolor", (char *)NULL, (char *)NULL,
        DEF_HIGH_COLOR, Blt_Offset(Blt_LinearGradientBrush, high), 0},
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL,
        DEF_JITTER, Blt_Offset(Blt_LinearGradientBrush, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-lowcolor", (char *)NULL, (char *)NULL,
        DEF_LOW_COLOR, Blt_Offset(Blt_LinearGradientBrush, low), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", (char *)NULL, (char *)NULL, DEF_OPACITY, 
        Blt_Offset(Blt_LinearGradientBrush, alpha),
        BLT_CONFIG_DONT_SET_DEFAULT, &opacityOption},
    {BLT_CONFIG_CUSTOM, "-palette", (char *)NULL, (char *)NULL,
        DEF_PALETTE, Blt_Offset(Blt_LinearGradientBrush, palette), 
        BLT_CONFIG_DONT_SET_DEFAULT, &paletteOption},
    {BLT_CONFIG_CUSTOM, "-repeat", (char *)NULL, (char *)NULL,
        DEF_REPEAT, Blt_Offset(Blt_LinearGradientBrush, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, &repeatOption},
    {BLT_CONFIG_CUSTOM, "-to", (char *)NULL, (char *)NULL,
        DEF_TO, Blt_Offset(Blt_LinearGradientBrush, to), 
        BLT_CONFIG_DONT_SET_DEFAULT, &positionOption},
    {BLT_CONFIG_PIXELS, "-xoffset", (char *)NULL, (char *)NULL, DEF_XORIGIN,
        Blt_Offset(Blt_LinearGradientBrush, xOrigin),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", (char *)NULL, (char *)NULL, DEF_YORIGIN,
        Blt_Offset(Blt_LinearGradientBrush, yOrigin),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_OptionParseProc ObjToOrient;
static Blt_OptionPrintProc OrientToObj;
static Blt_CustomOption orientOption =
{
    ObjToOrient, OrientToObj, NULL, (ClientData)0
};

static Blt_ConfigSpec stripeBrushSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL,
        DEF_JITTER, Blt_Offset(Blt_StripeBrush, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-offcolor", (char *)NULL, (char *)NULL,
        DEF_STRIPE_OFFCOLOR, Blt_Offset(Blt_StripeBrush, high)},
    {BLT_CONFIG_PIX32, "-oncolor", (char *)NULL, (char *)NULL,
        DEF_STRIPE_ONCOLOR, Blt_Offset(Blt_StripeBrush, low)},
    {BLT_CONFIG_CUSTOM, "-opacity", (char *)NULL, (char *)NULL, DEF_OPACITY, 
        Blt_Offset(Blt_StripeBrush, alpha), BLT_CONFIG_DONT_SET_DEFAULT,
        &opacityOption},
    {BLT_CONFIG_CUSTOM, "-orient", (char *)NULL, (char *)NULL,
        DEF_STRIPE_ORIENT, Blt_Offset(Blt_StripeBrush, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, &orientOption},
    {BLT_CONFIG_PIXELS, "-xoffset", (char *)NULL, (char *)NULL, DEF_XORIGIN,
        Blt_Offset(Blt_StripeBrush, xOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", (char *)NULL, (char *)NULL, DEF_YORIGIN,
        Blt_Offset(Blt_StripeBrush, yOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_POS, "-stride", (char *)NULL, (char *)NULL,
        DEF_STRIPE_STRIDE, Blt_Offset(Blt_StripeBrush, stride), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec checkerBrushSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL,
        DEF_JITTER, Blt_Offset(Blt_CheckerBrush, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-offcolor", (char *)NULL, (char *)NULL,
        DEF_CHECKER_OFFCOLOR, Blt_Offset(Blt_CheckerBrush, high)},
    {BLT_CONFIG_PIX32, "-oncolor", (char *)NULL, (char *)NULL,
        DEF_CHECKER_ONCOLOR, Blt_Offset(Blt_CheckerBrush, low)},
    {BLT_CONFIG_CUSTOM, "-opacity", (char *)NULL, (char *)NULL, DEF_OPACITY, 
        Blt_Offset(Blt_CheckerBrush, alpha), BLT_CONFIG_DONT_SET_DEFAULT,
        &opacityOption},
    {BLT_CONFIG_PIXELS_POS, "-stride", (char *)NULL, (char *)NULL,
        DEF_CHECKER_STRIDE, Blt_Offset(Blt_CheckerBrush, stride), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-xoffset", (char *)NULL, (char *)NULL, DEF_XORIGIN,
        Blt_Offset(Blt_CheckerBrush, xOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", (char *)NULL, (char *)NULL, DEF_YORIGIN,
        Blt_Offset(Blt_CheckerBrush, yOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec radialGradientBrushSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-center", (char *)NULL, (char *)NULL,
        DEF_RADIAL_CENTER, Blt_Offset(Blt_RadialGradientBrush, center), 
        BLT_CONFIG_DONT_SET_DEFAULT, &positionOption},
    {BLT_CONFIG_CUSTOM, "-colorscale", (char *)NULL, (char *)NULL,
        DEF_COLOR_SCALE, Blt_Offset(Blt_RadialGradientBrush, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, &colorScalingOption},
    {BLT_CONFIG_BITMASK, "-decreasing", (char *)NULL, (char *)NULL,
        DEF_DECREASING, Blt_Offset(Blt_RadialGradientBrush, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)BLT_PAINTBRUSH_DECREASING},
    {BLT_CONFIG_DOUBLE, "-diameter", (char *)NULL, (char *)NULL,
        DEF_RADIAL_DIAMETER, Blt_Offset(Blt_RadialGradientBrush, diameter), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIX32, "-highcolor", (char *)NULL, (char *)NULL,
        DEF_HIGH_COLOR, Blt_Offset(Blt_RadialGradientBrush, high)},
    {BLT_CONFIG_DOUBLE, "-height", (char *)NULL, (char *)NULL,
        DEF_RADIAL_HEIGHT, Blt_Offset(Blt_RadialGradientBrush, height), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL,
        DEF_JITTER, Blt_Offset(Blt_RadialGradientBrush, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-lowcolor", (char *)NULL, (char *)NULL,
        DEF_LOW_COLOR, Blt_Offset(Blt_RadialGradientBrush, low), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", (char *)NULL, (char *)NULL, DEF_OPACITY, 
        Blt_Offset(Blt_RadialGradientBrush, alpha), BLT_CONFIG_DONT_SET_DEFAULT,
        &opacityOption},
    {BLT_CONFIG_CUSTOM, "-palette", (char *)NULL, (char *)NULL,
        DEF_PALETTE, Blt_Offset(Blt_RadialGradientBrush, palette), 
        BLT_CONFIG_DONT_SET_DEFAULT, &paletteOption},
    {BLT_CONFIG_CUSTOM, "-repeat", (char *)NULL, (char *)NULL, DEF_REPEAT,
        Blt_Offset(Blt_RadialGradientBrush, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        &repeatOption},
    {BLT_CONFIG_DOUBLE, "-width", (char *)NULL, (char *)NULL,
        DEF_RADIAL_WIDTH, Blt_Offset(Blt_RadialGradientBrush, width), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-xoffset", (char *)NULL, (char *)NULL, DEF_XORIGIN,
        Blt_Offset(Blt_RadialGradientBrush, xOrigin),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", (char *)NULL, (char *)NULL, DEF_YORIGIN,
        Blt_Offset(Blt_RadialGradientBrush, yOrigin), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec conicalGradientBrushSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-center", (char *)NULL, (char *)NULL,
        DEF_CONICAL_CENTER, Blt_Offset(Blt_ConicalGradientBrush, center), 
        BLT_CONFIG_DONT_SET_DEFAULT, &positionOption},
    {BLT_CONFIG_CUSTOM, "-colorscale", (char *)NULL, (char *)NULL,
        DEF_COLOR_SCALE, Blt_Offset(Blt_ConicalGradientBrush, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, &colorScalingOption},
    {BLT_CONFIG_BITMASK, "-decreasing", (char *)NULL, (char *)NULL,
        DEF_DECREASING, Blt_Offset(Blt_ConicalGradientBrush, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)BLT_PAINTBRUSH_DECREASING},
    {BLT_CONFIG_PIX32, "-highcolor", (char *)NULL, (char *)NULL,
        DEF_HIGH_COLOR, Blt_Offset(Blt_ConicalGradientBrush, high)},
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL,
        DEF_JITTER, Blt_Offset(Blt_ConicalGradientBrush, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_CUSTOM, "-opacity", (char *)NULL, (char *)NULL, DEF_OPACITY, 
        Blt_Offset(Blt_ConicalGradientBrush, alpha),
        BLT_CONFIG_DONT_SET_DEFAULT, &opacityOption},
    {BLT_CONFIG_CUSTOM, "-palette", (char *)NULL, (char *)NULL, DEF_PALETTE,
        Blt_Offset(Blt_ConicalGradientBrush, palette), 
        BLT_CONFIG_DONT_SET_DEFAULT, &paletteOption},
    {BLT_CONFIG_DOUBLE, "-rotate", (char *)NULL, (char *)NULL,
        DEF_CONICAL_ROTATE, Blt_Offset(Blt_ConicalGradientBrush, angle),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIX32, "-lowcolor", (char *)NULL, (char *)NULL,
        DEF_LOW_COLOR, Blt_Offset(Blt_ConicalGradientBrush, low), 0},
    {BLT_CONFIG_PIXELS, "-xoffset", (char *)NULL, (char *)NULL, DEF_XORIGIN,
        Blt_Offset(Blt_ConicalGradientBrush, xOrigin),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", (char *)NULL, (char *)NULL, DEF_YORIGIN,
        Blt_Offset(Blt_ConicalGradientBrush, yOrigin), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static PaintBrushConfigProc ColorBrushConfigProc;
static PaintBrushRegionProc ColorBrushRegionProc;
static PaintBrushColorProc ColorBrushColorProc;

static Blt_PaintBrushClass colorBrushClass = {
    BLT_PAINTBRUSH_COLOR,               /* Type of brush. */
    "color",                            /* Class name. */
    ColorBrushConfigProc,               /* Configuration procedure. */
    ColorBrushRegionProc,               /* Coloring procedure. */
    ColorBrushColorProc,                /* Coloring procedure. */
    NULL                                /* Free procedure. */
};

static PaintBrushConfigProc LinearGradientBrushConfigProc;
static PaintBrushRegionProc LinearGradientBrushRegionProc;
static PaintBrushColorProc LinearGradientBrushColorProc;

static Blt_PaintBrushClass linearGradientBrushClass = {
    BLT_PAINTBRUSH_LINEAR,              /* Type of brush. */
    "linear",                           /* Class name. */
    LinearGradientBrushConfigProc,      /* Configuration procedure. */
    LinearGradientBrushRegionProc,      /* Coloring init procedure. */
    LinearGradientBrushColorProc,       /* Coloring procedure. */
    NULL                                /* Free procedure. */
};

static PaintBrushConfigProc TileBrushConfigProc;
static PaintBrushRegionProc TileBrushRegionProc;
static PaintBrushColorProc TileBrushColorProc;
static PaintBrushFreeProc TileBrushFreeProc;

static Blt_PaintBrushClass tileBrushClass = {
    BLT_PAINTBRUSH_TILE,                /* Type of brush. */
    "tile",                             /* Class name. */
    TileBrushConfigProc,                /* Configuration procedure. */
    TileBrushRegionProc,                /* Coloring region procedure. */
    TileBrushColorProc,                 /* Coloring procedure. */
    TileBrushFreeProc                   /* Free procedure. */
};

static PaintBrushConfigProc StripeBrushConfigProc;
static PaintBrushRegionProc StripeBrushRegionProc;
static PaintBrushColorProc StripeBrushColorProc;

static Blt_PaintBrushClass stripeBrushClass = {
    BLT_PAINTBRUSH_STRIPE,              /* Type of brush. */
    "stripe",                           /* Class name. */
    StripeBrushConfigProc,              /* Configuration procedure. */
    StripeBrushRegionProc,              /* Coloring region procedure. */
    StripeBrushColorProc,               /* Coloring procedure. */
    NULL                                /* Free procedure. */
};

static PaintBrushConfigProc CheckerBrushConfigProc;
static PaintBrushRegionProc CheckerBrushRegionProc;
static PaintBrushColorProc CheckerBrushColorProc;

static Blt_PaintBrushClass checkerBrushClass = {
    BLT_PAINTBRUSH_CHECKER,             /* Type of brush. */
    "checker",                          /* Class name. */
    CheckerBrushConfigProc,             /* Configuration procedure. */
    CheckerBrushRegionProc,             /* Coloring region procedure. */
    CheckerBrushColorProc,              /* Coloring procedure. */
    NULL                                /* Free procedure. */
};

static PaintBrushConfigProc RadialGradientBrushConfigProc;
static PaintBrushRegionProc RadialGradientBrushRegionProc;
static PaintBrushColorProc RadialGradientBrushColorProc;

static Blt_PaintBrushClass radialGradientBrushClass = {
    BLT_PAINTBRUSH_RADIAL,              /* Type of brush. */
    "radial",                           /* Class name. */
    RadialGradientBrushConfigProc,      /* Configuration procedure. */
    RadialGradientBrushRegionProc,      /* Coloring region procedure. */
    RadialGradientBrushColorProc,       /* Coloring procedure. */
    NULL                                /* Free procedure. */
};

static PaintBrushConfigProc ConicalGradientBrushConfigProc;
static PaintBrushRegionProc ConicalGradientBrushRegionProc;
static PaintBrushColorProc ConicalGradientBrushColorProc;

static Blt_PaintBrushClass conicalGradientBrushClass = {
    BLT_PAINTBRUSH_CONICAL,             /* Type of brush. */
    "conical",                          /* Class name. */
    ConicalGradientBrushConfigProc,     /* Configuration procedure. */
    ConicalGradientBrushRegionProc,     /* Coloring region procedure. */
    ConicalGradientBrushColorProc,      /* Coloring procedure. */
    NULL                                /* Free procedure. */
};


static void NotifyClients(Blt_PaintBrush brush);


#define COLOR_SCALING_MASK \
        (BLT_PAINTBRUSH_SCALING_LINEAR|BLT_PAINTBRUSH_SCALING_LOG)

/* 
 * Quick and dirty random number generator. 
 *
 * http://www.shadlen.org/ichbin/random/generators.htm#quick 
 */
#define JITTER_SEED     31337
#define JITTER_A        1099087573U
#define RANDOM_SCALE    2.3283064370807974e-10

static void 
RandomSeed(Blt_Random *randomPtr, unsigned int seed) {
    randomPtr->value = seed;
}

static void
RandomInit(Blt_Random *randomPtr) 
{
    RandomSeed(randomPtr, JITTER_SEED);
}

static INLINE double
RandomNumber(Blt_Random *randomPtr)
{
#if (SIZEOF_INT == 8) 
    /* mask the lower 32 bits on machines where int is a 64-bit quantity */
    randomPtr->value = ((1099087573  * (randomPtr->value))) & ((unsigned int) 0xffffffff);
#else
    /* on machines where int is 32-bits, no need to mask */
    randomPtr->value = (JITTER_A  * randomPtr->value);
#endif  /* SIZEOF_INT == 8 */
    return (double)randomPtr->value * RANDOM_SCALE;
}

static void
JitterInit(Blt_Jitter *jitterPtr) 
{
    RandomInit(&jitterPtr->random);
    jitterPtr->range = 0.0;
    jitterPtr->offset = -0.05;          /* Jitter +/-  */
}

static INLINE double 
Jitter(Blt_Jitter *jitterPtr) {
    double value;

    value = RandomNumber(&jitterPtr->random);
    return (value * jitterPtr->range) + jitterPtr->offset;
}


/*
 *---------------------------------------------------------------------------
 *
 * NotifyClients --
 *
 *      Notify each client that the background has changed.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
NotifyClients(Blt_PaintBrush brush)
{
     Blt_ColorBrush *brushPtr = (Blt_ColorBrush *)brush;
     Blt_ChainLink link;

     for (link = Blt_Chain_FirstLink(brushPtr->notifiers); link != NULL;
        link = Blt_Chain_NextLink(link)) {
         PaintBrushNotifier *notifyPtr;

         /* Notify each client that the paint brush has changed. The client
          * should schedule itself for redrawing.  */
         notifyPtr = Blt_Chain_GetValue(link);
        if (notifyPtr->proc != NULL) {
            (*notifyPtr->proc)(notifyPtr->clientData, brush);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageChangedProc
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ImageChangedProc(ClientData clientData, int x, int y, int w, int h,
                 int iw, int ih)
{
    PaintBrushCmd *cmdPtr = clientData;
    Blt_TileBrush *brushPtr = (Blt_TileBrush *)cmdPtr->brush;
    int isNew;

    /* Get picture from image. */
    if ((brushPtr->tile != NULL) &&
        (brushPtr->flags & BLT_PAINTBRUSH_FREE_PICTURE)) {
        Blt_FreePicture(brushPtr->tile);
    }
    if (Blt_Image_IsDeleted(brushPtr->tkImage)) {
        brushPtr->tkImage = NULL;
        return;                         /* Image was deleted. */
    }
    brushPtr->tile = Blt_GetPictureFromImage(cmdPtr->dataPtr->interp,
        brushPtr->tkImage, &isNew);
    if (Blt_Picture_IsAssociated(brushPtr->tile)) {
        Blt_UnassociateColors(brushPtr->tile);
    }
    if (isNew) {
        brushPtr->flags |= BLT_PAINTBRUSH_FREE_PICTURE;
    } else {
        brushPtr->flags &= ~BLT_PAINTBRUSH_FREE_PICTURE;
    }
}

/*ARGSUSED*/
static void
FreeImage(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Blt_TileBrush *brushPtr = (Blt_TileBrush *)widgRec;

    if (brushPtr->tkImage != NULL) {
        Tk_FreeImage(brushPtr->tkImage);
        brushPtr->tkImage = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToImage --
 *
 *      Given an image name, get the Tk image associated with it.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToImage(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation of value. */
    char *widgRec,                      /* Widget record. */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Tk_Image tkImage;
    Blt_TileBrush *brushPtr = (Blt_TileBrush *)widgRec;
    PaintBrushCmd *cmdPtr = clientData;

    tkImage = Tk_GetImage(interp, cmdPtr->tkwin, Tcl_GetString(objPtr), 
        ImageChangedProc, cmdPtr);
    if (tkImage == NULL) {
        return TCL_ERROR;
    }
    brushPtr->tkImage = tkImage;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageToObj --
 *
 *      Convert the image name into a string Tcl_Obj.
 *
 * Results:
 *      The string representation of the image is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ImageToObj(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Blt_TileBrush *brushPtr = (Blt_TileBrush *)(widgRec);

    if (brushPtr->tkImage == NULL) {
        return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(Blt_Image_Name(brushPtr->tkImage), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPosition --
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPosition(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              Tcl_Obj *objPtr, char *widgRec, int offset, int flags)    
{
    Point2d *pointPtr = (Point2d *)(widgRec + offset);
    Tcl_Obj **objv;
    int objc;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc > 2) {
        Tcl_AppendResult(interp, "unknown position \"", Tcl_GetString(objPtr),
                "\": should be \"top left\" or \"nw\"", (char *)NULL);
        return TCL_ERROR;
    }
    pointPtr->x = 0.0;
    pointPtr->y = 0.0;
    if (objc == 0) {
        pointPtr->x = 0.5;
        pointPtr->y = 0.0;
        return TCL_OK;
    }
    if (objc == 1) {
        const char *string;
        char c;
        
        string = Tcl_GetString(objv[0]);
        c = string[0];
        if ((c == 'n') && (strcmp(string, "nw") == 0)) {
            pointPtr->x = 0.0;
            pointPtr->y = 0.0;
        } else if ((c == 's') && (strcmp(string, "sw") == 0)) {
            pointPtr->x = 0.0;
            pointPtr->y = 1.0;
        } else if ((c == 's') && (strcmp(string, "se") == 0)) {
            pointPtr->x = 1.0;
            pointPtr->y = 1.0;
        } else if ((c == 'n') && (strcmp(string, "ne") == 0)) {
            pointPtr->x = 1.0;
            pointPtr->y = 0.0;
        } else if ((c == 'c') && (strcmp(string, "c") == 0)) {
            pointPtr->x = 0.5;
            pointPtr->y = 0.5;
        } else if ((c == 'n') && (strcmp(string, "n") == 0)) {
            pointPtr->x = 0.5;
            pointPtr->y = 0.0;
        } else if ((c == 's') && (strcmp(string, "s") == 0)) {
            pointPtr->x = 0.5;
            pointPtr->y = 1.0;
        } else if ((c == 'e') && (strcmp(string, "e") == 0)) {
            pointPtr->x = 1.0;
            pointPtr->y = 0.5;
        } else if ((c == 'w') && (strcmp(string, "w") == 0)) {
            pointPtr->x = 0.0;
            pointPtr->y = 0.5;
        } else {
            Tcl_AppendResult(interp, "unknown position \"", string,
                "\": should be nw, n, ne, w, c, e, sw, s, or se.", (char *)NULL);
            return TCL_ERROR;
        }
        return TCL_OK;
    } 
    if (objc == 2) {
        const char *string;
        char c;
        
        string = Tcl_GetString(objv[0]);
        c = string[0];
        if (Tcl_GetDoubleFromObj(NULL, objv[0], &pointPtr->x) != TCL_OK) {
            if ((c == 't') && (strcmp(string, "top") == 0)) {
                pointPtr->y = 0.0;
            } else if ((c == 'b') && (strcmp(string, "bottom") == 0)) {
                pointPtr->y = 1.0;
            } else if ((c == 'c') && (strcmp(string, "center") == 0)) {
                pointPtr->y = 1.0;
            } else {
                Tcl_AppendResult(interp, "unknown position \"", string,
                     "\": should be top, bottom, or center.", (char *)NULL);
                return TCL_ERROR;
            }
        }
        if (Tcl_GetDoubleFromObj(NULL, objv[1], &pointPtr->y) != TCL_OK) {
            if ((c == 'l') && (strcmp(string, "left") == 0)) {
                pointPtr->x = 0.0;
            } else if ((c == 'r') && (strcmp(string, "right") == 0)) {
                pointPtr->x = 1.0;
            } else if ((c == 'c') && (strcmp(string, "center") == 0)) {
                pointPtr->x = 0.5;
            } else {
                Tcl_AppendResult(interp, "unknown position \"", string,
                "\": should be left, right, or center.", (char *)NULL);
                return TCL_ERROR;
            }
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PositionToObj --
 *
 *      Returns the string representing the position.
 *
 * Results:
 *      The string representation of the position is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PositionToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              char *widgRec, int offset, int flags)     
{
    Point2d *pointPtr = (Point2d *)(widgRec + offset);
    Tcl_Obj *objPtr, *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewDoubleObj(pointPtr->x);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewDoubleObj(pointPtr->y);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToRepeat --
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToRepeat(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              Tcl_Obj *objPtr, char *widgRec, int offset, int flags)    
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    int flag;
    const char *string;
    char c;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'n') && (strcmp(string, "no") == 0)) {
        flag = 0;
    } else if ((c == 'y') && (strcmp(string, "yes") == 0)) {
        flag = BLT_PAINTBRUSH_REPEAT_NORMAL;
    } else if ((c == 'r') && (strcmp(string, "reversing") == 0)) {
        flag = BLT_PAINTBRUSH_REPEAT_OPPOSITE;
    } else {
        Tcl_AppendResult(interp, "unknown repeat value \"", string,
                "\": should be yes, no, or reversing.", (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~REPEAT_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
} 

/*
 *---------------------------------------------------------------------------
 *
 * RepeatToObj --
 *
 *      Returns the string representing the repeat flag.
 *
 * Results:
 *      The string representation of the repeat flag is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
RepeatToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              char *widgRec, int offset, int flags)     
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    Tcl_Obj *objPtr;
    
    switch (*flagsPtr & REPEAT_MASK) {
    case BLT_PAINTBRUSH_REPEAT_NORMAL:
        objPtr = Tcl_NewStringObj("yes", 3);       break;
    case BLT_PAINTBRUSH_REPEAT_OPPOSITE:
        objPtr = Tcl_NewStringObj("reversing", 9); break;
    default:
        objPtr = Tcl_NewStringObj("no", 2);        break;
    }
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToOrient --
 *
 *      Translate the given string to the gradient type it represents.
 *      Types are "horizontal", "vertical", "updiagonal", "downdiagonal", 
 *      and "radial"".
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToOrient(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              Tcl_Obj *objPtr, char *widgRec, int offset, int flags)    
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    int flag;
    const char *string;
    char c;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'v') && (strcmp(string, "vertical") == 0)) {
        flag = BLT_PAINTBRUSH_ORIENT_VERTICAL;
    } else if ((c == 'h') && (strcmp(string, "horizontal") == 0)) {
        flag = BLT_PAINTBRUSH_ORIENT_HORIZONTAL;
    } else {
        Tcl_AppendResult(interp, "unknown orient value \"", string,
                "\": should be vertical or horizontal.", (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~ORIENT_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
} 

/*
 *---------------------------------------------------------------------------
 *
 * OrientToObj --
 *
 *      Returns the string representing the orient flag.
 *
 * Results:
 *      The string representation of the orient flag is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
OrientToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              char *widgRec, int offset, int flags)     
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    Tcl_Obj *objPtr;
    
    switch (*flagsPtr & ORIENT_MASK) {
    case BLT_PAINTBRUSH_ORIENT_VERTICAL:
        objPtr = Tcl_NewStringObj("vertical", 8);       break;
    case BLT_PAINTBRUSH_ORIENT_HORIZONTAL:
        objPtr = Tcl_NewStringObj("horizontal", 10);    break;
    default:
        objPtr = Tcl_NewStringObj("???", 3);            break;
    }
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * PaletteChangedProc
 *
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
PaletteChangedProc(Blt_Palette palette, ClientData clientData, 
                   unsigned int flags)
{
     if (flags & PALETTE_DELETE_NOTIFY) {
         PaintBrush *brushPtr = clientData;

         brushPtr->palette = NULL;
    }
}

/*ARGSUSED*/
static void
FreePalette(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Blt_Palette *palPtr = (Blt_Palette *)(widgRec + offset);
    PaintBrush *brushPtr = (PaintBrush *)widgRec;

    Blt_Palette_DeleteNotifier(*palPtr, PaletteChangedProc, brushPtr);
    *palPtr = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPalette --
 *
 *      Convert the string representation of a palette into its token.
 *
 * Results:
 *      The return value is a standard TCL result.  The palette token is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPalette(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
             Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Blt_Palette *palPtr = (Blt_Palette *)(widgRec + offset);
    PaintBrush *brushPtr = (PaintBrush *)(widgRec);
    const char *string;
    
    string = Tcl_GetString(objPtr);
    if ((string == NULL) || (string[0] == '\0')) {
        FreePalette(clientData, Tk_Display(tkwin), widgRec, offset);
        return TCL_OK;
    }
    if (Blt_Palette_GetFromObj(interp, objPtr, palPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_Palette_CreateNotifier(*palPtr, PaletteChangedProc, brushPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PaletteToObj --
 *
 *      Convert the palette token into a string.
 *
 * Results:
 *      The string representing the symbol type or line style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PaletteToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
             char *widgRec, int offset, int flags)
{
    Blt_Palette palette = *(Blt_Palette *)(widgRec + offset);
    if (palette == NULL) {
        return Tcl_NewStringObj("", -1);
    } 
    return Tcl_NewStringObj(Blt_Palette_Name(palette), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToColorScaling --
 *
 *      Translates the given string to the gradient scale it represents.  
 *      Valid scales are "linear" or "logarithmic".
 *
 * Results:
 *      A standard TCL result.  If successful the field in the structure
 *      is updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColorScaling(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                  Tcl_Obj *objPtr, char *widgRec, int offset, int flags)        
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    const char *string;
    int length;
    char c;
    int flag;
    
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    flag = 0;
    if ((c == 'l') && (strcmp(string, "linear") == 0)) {
        flag = BLT_PAINTBRUSH_SCALING_LINEAR;
    } else if ((c == 'l') && (length > 2) && 
               (strncmp(string, "logarithmic", length) == 0)) {
        flag = BLT_PAINTBRUSH_SCALING_LOG;
    } else {
        Tcl_AppendResult(interp, "unknown coloring scaling \"", string, "\"",
                         ": should be linear or logarithmic.",
                         (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~COLOR_SCALING_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorScalingToObj --
 *
 *      Convert the color scaling flag into a string Tcl_Obj.
 *
 * Results:
 *      The string representation of the color scaling flag is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ColorScalingToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                  char *widgRec, int offset, int flags) 
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    Tcl_Obj *objPtr;
    
    switch (*flagsPtr & COLOR_SCALING_MASK) {
    case BLT_PAINTBRUSH_SCALING_LINEAR:
        objPtr = Tcl_NewStringObj("linear", 6);         break;
    case BLT_PAINTBRUSH_SCALING_LOG:
        objPtr = Tcl_NewStringObj("log", 3);            break;
    default:
        objPtr = Tcl_NewStringObj("???", 3);            break;
    }
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToOpacity --
 *
 *      Converts the string representing the percent of opacity to an
 *      alpha value 0..255.
 *
 * Results:
 *      A standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToOpacity(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
             Tcl_Obj *objPtr, char *widgRec, int offset, int flags)     
{
    int *alphaPtr = (int *)(widgRec + offset);
    double opacity;

    if (Tcl_GetDoubleFromObj(interp, objPtr, &opacity) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((opacity < 0.0) || (opacity > 100.0)) {
        Tcl_AppendResult(interp, "invalid percent opacity \"", 
                Tcl_GetString(objPtr), "\": number should be between 0 and 100",
                (char *)NULL);
        return TCL_ERROR;
    }
    opacity = (opacity / 100.0) * 255.0;
    *alphaPtr = ROUND(opacity);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OpacityToObj --
 *
 *      Convert the alpha value into a string Tcl_Obj representing a
 *      percentage.
 *
 * Results:
 *      The string representation of the opacity percentage is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
OpacityToObj(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Widget record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    int *alphaPtr = (int *)(widgRec + offset);
    double opacity;

    opacity = (*alphaPtr / 255.0) * 100.0;
    return Tcl_NewDoubleObj(opacity);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToJitter --
 *
 *      Given a string representation of the jitter value (a percentage),
 *      convert it to a number 0..1.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToJitter(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            Tcl_Obj *objPtr, char *widgRec, int offset, int flags)      
{
    double *jitterPtr = (double *)(widgRec + offset);
    double jitter;

    if (Tcl_GetDoubleFromObj(interp, objPtr, &jitter) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((jitter < 0.0) || (jitter > 100.0)) {
        Tcl_AppendResult(interp, "invalid percent jitter \"", 
                Tcl_GetString(objPtr), "\" number should be between 0 and 100",
                (char *)NULL);
        return TCL_ERROR;
    }
    *jitterPtr = jitter * 0.01;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * JitterToObj --
 *
 *      Convert the double jitter value to a Tcl_Obj.
 *
 * Results:
 *      The string representation of the jitter percentage is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
JitterToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            char *widgRec, int offset, int flags)       
{
    double *jitterPtr = (double *)(widgRec + offset);
    double jitter;

    jitter = (double)*jitterPtr * 100.0;
    return Tcl_NewDoubleObj(jitter);
}


static void 
SetBrushName(Blt_PaintBrush brush, const char *name)
{
    Blt_ColorBrush *brushPtr = (Blt_ColorBrush *)brush;

    brushPtr->name = name;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorBrushRegionProc --
 *
 *      Precomputes fields necessary for computing the color based upon
 *      the region specified.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
ColorBrushRegionProc(Blt_PaintBrush brush, int x, int y, int w, int h)
{
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorBrushConfigProc --
 *
 *      Configures the color brush. 
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
ColorBrushConfigProc(Tcl_Interp *interp, Blt_PaintBrush brush)
{

    Blt_ColorBrush *brushPtr = (Blt_ColorBrush *)brush;
    int t;
    
    brushPtr->color.u32 = brushPtr->reqColor.u32;
    brushPtr->color.Alpha = imul8x8(brushPtr->alpha, brushPtr->color.Alpha, t);
    Blt_AssociateColor(&brushPtr->color);
    return TCL_OK;
}

static int
ColorBrushColorProc(Blt_PaintBrush brush, int x, int y)
{
    Blt_ColorBrush *brushPtr = (Blt_ColorBrush *)brush;
    Blt_Pixel color;

    color.u32 = brushPtr->color.u32; 
    if (brushPtr->jitter.range > 0.0) {
        double t, jitter;
        
        jitter = Jitter(&brushPtr->jitter);

        t = brushPtr->color.Red / 255.0;
        t += jitter * 0.3333333333333;
        t = JCLAMP(t);
        color.Red = (unsigned char)(t * 255.0);
   
        t = brushPtr->color.Green / 255.0;
        t += jitter * 0.3333333333333;
        t = JCLAMP(t);
        color.Green = (unsigned char)(t * 255.0);

        t = brushPtr->color.Blue / 255.0;
        t += jitter * 0.3333333333333;
        t = JCLAMP(t);
        color.Blue = (unsigned char)(t * 255.0);
    }
    return color.u32;
}

/*
 *---------------------------------------------------------------------------
 *
 * LinearGradientBrushRegionProc --
 *
 *      Pre-computes fields necessary for computing the linear gradient
 *      color based upon the region specified.  
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
LinearGradientBrushRegionProc(Blt_PaintBrush brush, int x, int y, int w, int h)
{
    Blt_LinearGradientBrush *brushPtr = (Blt_LinearGradientBrush *)brush;

    /* Factor in the gradient origin when computing the pixel. */
    x -= brushPtr->xOrigin;
    y -= brushPtr->yOrigin;

    /* Convert the line segment into screen coordinates (pixels). */
    brushPtr->x1 = x + (int)(brushPtr->from.x * w);
    brushPtr->y1 = y + (int)(brushPtr->from.y * h);
    brushPtr->x2 = x + (int)(brushPtr->to.x * w);
    brushPtr->y2 = y + (int)(brushPtr->to.y * h);
    
    /* Compute the length of the segment in pixels. */
    brushPtr->length = hypot(brushPtr->x2-brushPtr->x1, brushPtr->y2-brushPtr->y1);
    brushPtr->scaleFactor = 1.0 / brushPtr->length;
    
    /* Rotate the segment if necessary. */
    if (brushPtr->angle != 0.0) {
#ifdef notdef
        double cosTheta, sinTheta;
        cosTheta = cos(brushPtr->angle);
        sinTheta = sin(brushPtr->angle);
#endif
    }
    if (brushPtr->x1 == brushPtr->x2) {
        brushPtr->flags |= BLT_PAINTBRUSH_VERTICAL;
    } else if (brushPtr->y1 == brushPtr->y2) {
        brushPtr->flags |= BLT_PAINTBRUSH_HORIZONTAL;
    } else {
        brushPtr->flags |= BLT_PAINTBRUSH_DIAGONAL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * LinearGradientBrushConfigProc --
 *
 *      Configures the linear gradient brush. 
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
LinearGradientBrushConfigProc(Tcl_Interp *interp, Blt_PaintBrush brush)
{
    Blt_LinearGradientBrush *brushPtr = (Blt_LinearGradientBrush *)brush;
    
    brushPtr->rRange = brushPtr->high.Red   - brushPtr->low.Red;
    brushPtr->gRange = brushPtr->high.Green - brushPtr->low.Green;
    brushPtr->bRange = brushPtr->high.Blue  - brushPtr->low.Blue;
    brushPtr->aRange = brushPtr->high.Alpha - brushPtr->low.Alpha;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LinearGradientBrushColorProc --
 *
 *      Computes the interpolated color from the x-y pixel coordinate
 *      given.
 *
 * Results:
 *      The interpolated color is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
LinearGradientBrushColorProc(Blt_PaintBrush brush, int x, int y)
{
    Blt_LinearGradientBrush *brushPtr = (Blt_LinearGradientBrush *)brush;
    Blt_Pixel color;
    double t;
    int t1;
    
    if (brushPtr->calcProc != NULL) {
        x -= brushPtr->xOrigin;
        y -= brushPtr->yOrigin;
        if ((*brushPtr->calcProc)(brushPtr->clientData, x, y, &t) != TCL_OK) {
            return 0x0;
        }
    } else if (brushPtr->flags & BLT_PAINTBRUSH_HORIZONTAL) {
        t = (x - brushPtr->x1) / (double)(brushPtr->x2 - brushPtr->x1);
    } else if (brushPtr->flags & BLT_PAINTBRUSH_VERTICAL) {
        t = (y - brushPtr->y1) / (double)(brushPtr->y2 - brushPtr->y1);
     } else {
        double d;
        Point2d p;

        /* Get the projection of the the sample point on the infinite line
         * described by the line segment. The distance of the projected
         * point from the start of the line segment over the distance of
         * the line segment is t. */
        p = Blt_GetProjection2(x, y, brushPtr->x1, brushPtr->y1, brushPtr->x2,
                               brushPtr->y2);
        d = hypot(p.x - brushPtr->x1, p.y - brushPtr->y1);
        t = d / brushPtr->length;
    }
    if ((t < 0.0) || (t > 1.0)) {
        double rem;
        int pos;
        
        rem = fmod(t, 1.0);
        pos = (int)(t - rem);
        if (brushPtr->flags & BLT_PAINTBRUSH_REPEAT_OPPOSITE) {
            if (pos & 0x1) {
                rem = 1.0 - rem;
            }
        }
        t = rem;
    }
    if (brushPtr->jitter.range > 0.0) {
        t += Jitter(&brushPtr->jitter);
        t = JCLAMP(t);
    }
    if (brushPtr->flags & BLT_PAINTBRUSH_SCALING_LOG) {
        t = log10(9.0 * t + 1.0);
    } 
    if (brushPtr->flags & BLT_PAINTBRUSH_DECREASING) {
        t = 1.0 - t;
    }        
    if (brushPtr->palette != NULL) {
        return Blt_Palette_GetAssociatedColor(brushPtr->palette, t);
    }
    color.Red   = (unsigned char)(brushPtr->low.Red   + t * brushPtr->rRange);
    color.Green = (unsigned char)(brushPtr->low.Green + t * brushPtr->gRange);
    color.Blue  = (unsigned char)(brushPtr->low.Blue  + t * brushPtr->bRange);
    color.Alpha = (unsigned char)(brushPtr->low.Alpha + t * brushPtr->aRange);
    color.Alpha = imul8x8(brushPtr->alpha, color.Alpha, t1);
    return color.u32;
}

/*
 *---------------------------------------------------------------------------
 *
 * TileBrushRegionProc --
 *
 *      Initializes the tile colors based upon the region specified.  
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
TileBrushRegionProc(Blt_PaintBrush brush, int x, int y, int w, int h)
{
    Blt_TileBrush *brushPtr = (Blt_TileBrush *)brush;

    brushPtr->x = x;
    brushPtr->y = y;
}

/*
 *---------------------------------------------------------------------------
 *
 * TileBrushFreeProc --
 *
 *      Free and memory or resources used by the tile brush.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
TileBrushFreeProc(Blt_PaintBrush brush)
{
    Blt_TileBrush *brushPtr = (Blt_TileBrush *)brush;
    
    if (brushPtr->flags & BLT_PAINTBRUSH_FREE_PICTURE) {
        Blt_FreePicture(brushPtr->tile);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TileBrushConfigProc --
 *
 *      Configures the tile brush. 
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
TileBrushConfigProc(Tcl_Interp *interp, Blt_PaintBrush brush)
{
    Blt_TileBrush *brushPtr = (Blt_TileBrush *)brush;
    
    if (brushPtr->tkImage != NULL) {
        int isNew;
        
        if ((brushPtr->tile != NULL) &&
            (brushPtr->flags & BLT_PAINTBRUSH_FREE_PICTURE)) {
            Blt_FreePicture(brushPtr->tile);
        }
        brushPtr->tile = Blt_GetPictureFromImage(interp, brushPtr->tkImage,
                &isNew);
        if (Blt_Picture_IsAssociated(brushPtr->tile)) {
            Blt_UnassociateColors(brushPtr->tile);
        }
        if (isNew) {
            brushPtr->flags |= BLT_PAINTBRUSH_FREE_PICTURE;
        } else {
            brushPtr->flags &= ~BLT_PAINTBRUSH_FREE_PICTURE;
        }
    }
    /* This is where you initialize the coloring variables. */
    return TCL_OK;
}

static int
TileBrushColorProc(Blt_PaintBrush brush, int x, int y)
{
    Blt_TileBrush *brushPtr = (Blt_TileBrush *)brush;
    Blt_Pixel *pixelPtr;
    Blt_Pixel color;
    int t1;    

    if (brushPtr->tile == NULL) {
        return 0x0;
    }
    /* Factor in the tile origin when computing the pixel in the tile. */
    x -= brushPtr->x;
    y -= brushPtr->y;

    x = x % Blt_Picture_Width(brushPtr->tile);
    y = y % Blt_Picture_Height(brushPtr->tile);

    if (x < 0) {
        x = -x;
    }
    if (y < 0) {
        y = -y;
    }
    pixelPtr = Blt_Picture_Pixel(brushPtr->tile, x, y);
    /* Pixel of tile is unassociated so that we can override the
     * opacity. */
    color.u32 = pixelPtr->u32;
    if (brushPtr->jitter.range > 0.0) {
        double t, jitter;
        
        jitter = Jitter(&brushPtr->jitter);
        t = pixelPtr->Red / 255.0;
        t += jitter;
        t = JCLAMP(t);
        color.Red = (unsigned char)(t * 255.0);
        t = pixelPtr->Green / 255.0;
        t += jitter;
        t = JCLAMP(t);
        color.Green = (unsigned char)(t * 255.0);
        t = pixelPtr->Blue / 255.0;
        t += jitter;
        t = JCLAMP(t);
        color.Blue = (unsigned char)(t * 255.0);
    }
    color.Alpha = imul8x8(brushPtr->alpha, color.Alpha, t1);
    Blt_AssociateColor(&color);
    return color.u32;
}

/*
 *---------------------------------------------------------------------------
 *
 * StripeBrushRegionProc --
 *
 *      Pre-computes fields necessary for computing the stripe gradient
 *      color based upon the region specified.  
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
StripeBrushRegionProc(Blt_PaintBrush brush, int x, int y, int w, int h)
{
}

/*
 *---------------------------------------------------------------------------
 *
 * StripeBrushConfigProc --
 *
 *      Configures the stripe gradient brush. 
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
StripeBrushConfigProc(Tcl_Interp *interp, Blt_PaintBrush brush)
{
    Blt_StripeBrush *brushPtr = (Blt_StripeBrush *)brush;;

    brushPtr->rRange = brushPtr->high.Red   - brushPtr->low.Red;
    brushPtr->gRange = brushPtr->high.Green - brushPtr->low.Green;
    brushPtr->bRange = brushPtr->high.Blue  - brushPtr->low.Blue;
    brushPtr->aRange = brushPtr->high.Alpha - brushPtr->low.Alpha;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StripeBrushColorProc --
 *
 *      Computes the interpolated color from the x-y pixel coordinate
 *      given.
 *
 * Results:
 *      The interpolated color is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
StripeBrushColorProc(Blt_PaintBrush brush, int x, int y)
{
    Blt_Pixel color;
    Blt_StripeBrush *brushPtr = (Blt_StripeBrush *)brush;
    double t;
    int t1;
    
    /* Factor in the gradient origin when computing the pixel. */
    x = (x - brushPtr->xOrigin);
    y = (y - brushPtr->yOrigin);

    if (brushPtr->flags & BLT_PAINTBRUSH_ORIENT_VERTICAL) {
        t = ((x / brushPtr->stride) & 0x1) ? 0.0 : 1.0;
    } else {
        t = ((y / brushPtr->stride) & 0x1) ? 0.0 : 1.0;
    }
    if (brushPtr->jitter.range > 0.0) {
        t += (t == 1.0) ? brushPtr->jitter.offset * 0.5 :
            -brushPtr->jitter.offset * 0.5;
        t += Jitter(&brushPtr->jitter);
        t = JCLAMP(t);
    }
    color.Red   = (unsigned char)(brushPtr->low.Red   + t*brushPtr->rRange);
    color.Green = (unsigned char)(brushPtr->low.Green + t*brushPtr->gRange);
    color.Blue  = (unsigned char)(brushPtr->low.Blue  + t*brushPtr->bRange);
    color.Alpha = (unsigned char)(brushPtr->low.Alpha + t*brushPtr->aRange);
    color.Alpha = imul8x8(brushPtr->alpha, color.Alpha, t1);
    return color.u32;
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckerBrushRegionProc --
 *
 *      Pre-computes fields necessary for computing the checker gradient
 *      color based upon the region specified.  
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
CheckerBrushRegionProc(Blt_PaintBrush brush, int x, int y, int w, int h)
{
    Blt_CheckerBrush *brushPtr = (Blt_CheckerBrush *)brush;
    
    brushPtr->x = (x - brushPtr->xOrigin);
    brushPtr->y = (y - brushPtr->yOrigin);
}


/*
 *---------------------------------------------------------------------------
 *
 * CheckerBrushConfigProc --
 *
 *      Configures the checker gradient brush. 
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
CheckerBrushConfigProc(Tcl_Interp *interp, Blt_PaintBrush brush)
{
    Blt_CheckerBrush *brushPtr = (Blt_CheckerBrush *)brush;

    brushPtr->rRange = brushPtr->high.Red   - brushPtr->low.Red;
    brushPtr->gRange = brushPtr->high.Green - brushPtr->low.Green;
    brushPtr->bRange = brushPtr->high.Blue  - brushPtr->low.Blue;
    brushPtr->aRange = brushPtr->high.Alpha - brushPtr->low.Alpha;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CheckerBrushColorProc --
 *
 *      Computes the interpolated color from the x-y pixel coordinate
 *      given.
 *
 * Results:
 *      The interpolated color is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
CheckerBrushColorProc(Blt_PaintBrush brush, int x, int y)
{
    Blt_CheckerBrush *brushPtr = (Blt_CheckerBrush *)brush;
    double t;
    Blt_Pixel color;
    int t1, t2;
    
    x = (x - brushPtr->x);
    if (x < 0) {
        x = -x;
    }
    y = (y - brushPtr->y);
    if (y < 0) {
        y = -y;
    }
    t1 = ((x / brushPtr->stride) & 0x1);
    t2 = ((y / brushPtr->stride) & 0x1);
    if ((t1 + t2) == 1) {
        t = 0.0;
    } else {
        t = 1.0;
    }
    if (brushPtr->jitter.range > 0.0) {
        t += (t == 1.0) ? brushPtr->jitter.offset * 0.5 :
            -brushPtr->jitter.offset * 0.5;
        t += Jitter(&brushPtr->jitter);
        t = JCLAMP(t);
    }
    color.Red   = (unsigned char)(brushPtr->low.Red   + t*brushPtr->rRange);
    color.Green = (unsigned char)(brushPtr->low.Green + t*brushPtr->gRange);
    color.Blue  = (unsigned char)(brushPtr->low.Blue  + t*brushPtr->bRange);
    color.Alpha = (unsigned char)(brushPtr->low.Alpha + t*brushPtr->aRange);
    color.Alpha = imul8x8(brushPtr->alpha, color.Alpha, t1);
    return color.u32;
}


/*
 *---------------------------------------------------------------------------
 *
 * RadialGradientBrushRegionProc --
 *
 *      Pre-computes fields necessary for computing the radial gradient
 *      color based upon the region specified.  
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
RadialGradientBrushRegionProc(Blt_PaintBrush brush, int x, int y, int w, int h)
{
    Blt_RadialGradientBrush *brushPtr = (Blt_RadialGradientBrush *)brush;

    /* Factor in the gradient origin when computing the pixel. */
    x -= brushPtr->xOrigin;
    y -= brushPtr->yOrigin;

    /* Convert the center point into screen coordinates (pixels). */
    brushPtr->cx = x + (int)(brushPtr->center.x * w);
    brushPtr->cy = y + (int)(brushPtr->center.y * h);

    if (brushPtr->diameter > 0.0) {
        brushPtr->b = brushPtr->a = (int)(brushPtr->diameter * MIN(w,h) * 0.5);
    } else {
        brushPtr->a = (int)(brushPtr->width * w * 0.5);
        brushPtr->b = (int)(brushPtr->height * h * 0.5);
    }

    /* Rotate the segment if necessary. */
    if (brushPtr->angle != 0.0) {
#ifdef notdef
        double cosTheta, sinTheta;
        cosTheta = cos(brushPtr->angle);
        sinTheta = sin(brushPtr->angle);
#endif
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * RadialGradientBrushConfigProc --
 *
 *      Configures the radial gradient brush. 
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
RadialGradientBrushConfigProc(Tcl_Interp *interp, Blt_PaintBrush brush)
{
    Blt_RadialGradientBrush *brushPtr = (Blt_RadialGradientBrush *)brush;;

    brushPtr->rRange = brushPtr->high.Red   - brushPtr->low.Red;
    brushPtr->gRange = brushPtr->high.Green - brushPtr->low.Green;
    brushPtr->bRange = brushPtr->high.Blue  - brushPtr->low.Blue;
    brushPtr->aRange = brushPtr->high.Alpha - brushPtr->low.Alpha;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RadialGradientBrushColorProc --
 *
 *      Computes the interpolated color from the x-y pixel coordinate
 *      given.
 *
 * Results:
 *      The interpolated color is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
RadialGradientBrushColorProc(Blt_PaintBrush brush, int x, int y)
{
    Blt_Pixel color;
    Blt_RadialGradientBrush *brushPtr = (Blt_RadialGradientBrush *)brush;
    double dx, dy, d1, d2;
    double fx, fy, m;
    double t;
    int t1;
    
    dx = x - brushPtr->cx;
    dy = y - brushPtr->cy;
    d1 = hypot(dx, dy);
    if (dx == 0) {
        fy = brushPtr->b, fx = 0;
    } else if (dy == 0) {
        fx = brushPtr->a, fy = 0;
    } else {
        m = atan(dy / dx);
        fx = brushPtr->a * cos(m);
        fy = brushPtr->b * sin(m);
    }
    d2 = hypot(fx, fy);
    t = (d1 / d2);
    if (brushPtr->jitter.range > 0.0) {
        t += Jitter(&brushPtr->jitter);
        t = JCLAMP(t);
    }
    if (brushPtr->flags & BLT_PAINTBRUSH_SCALING_LOG) {
        t = log10(9.0 * t + 1.0);
    } 
    if (brushPtr->flags & BLT_PAINTBRUSH_DECREASING) {
        t = 1.0 - t;
    }        
    if (brushPtr->palette != NULL) {
        return Blt_Palette_GetAssociatedColor(brushPtr->palette, t);
    }
    color.Red   = (unsigned char)(brushPtr->low.Red   + t * brushPtr->rRange);
    color.Green = (unsigned char)(brushPtr->low.Green + t * brushPtr->gRange);
    color.Blue  = (unsigned char)(brushPtr->low.Blue  + t * brushPtr->bRange);
    color.Alpha = (unsigned char)(brushPtr->low.Alpha + t * brushPtr->aRange);
    color.Alpha = imul8x8(brushPtr->alpha, color.Alpha, t1);
    return color.u32;
}


/*
 *---------------------------------------------------------------------------
 *
 * ConicalGradientBrushRegionProc --
 *
 *      Pre-computes fields necessary for computing the conical gradient
 *      color based upon the region specified.  
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
ConicalGradientBrushRegionProc(Blt_PaintBrush brush, int x, int y, int w, int h)
{
    Blt_ConicalGradientBrush *brushPtr = (Blt_ConicalGradientBrush *)brush;

    /* Factor in the gradient origin when computing the pixel. */
    x -= brushPtr->xOrigin;
    y -= brushPtr->yOrigin;

    /* Convert the center point into screen coordinates (pixels). */
    brushPtr->cx = x + (int)(brushPtr->center.x * w);
    brushPtr->cy = y + (int)(brushPtr->center.y * h);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConicalGradientBrushConfigProc --
 *
 *      Configures the conical gradient brush. 
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
ConicalGradientBrushConfigProc(Tcl_Interp *interp, Blt_PaintBrush brush)
{
    Blt_ConicalGradientBrush *brushPtr = (Blt_ConicalGradientBrush *)brush;

    brushPtr->rRange = brushPtr->high.Red   - brushPtr->low.Red;
    brushPtr->gRange = brushPtr->high.Green - brushPtr->low.Green;
    brushPtr->bRange = brushPtr->high.Blue  - brushPtr->low.Blue;
    brushPtr->aRange = brushPtr->high.Alpha - brushPtr->low.Alpha;
    brushPtr->theta = brushPtr->angle * DEG2RAD;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConicalGradientBrushColorProc --
 *
 *      Computes the interpolated color from the x-y pixel coordinate
 *      given.
 *
 * Results:
 *      The interpolated color is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
ConicalGradientBrushColorProc(Blt_PaintBrush brush, int x, int y)
{
    Blt_ConicalGradientBrush *brushPtr = (Blt_ConicalGradientBrush *)brush;
    Blt_Pixel color;
    double dx, dy;
    double t;
    int t1;
    
    /* Translate to the center of the reference window. */
    dx = x - brushPtr->cx;
    dy = y - brushPtr->cy;
    if (dx == 0.0) {
        double theta;                   /* Angle of line (sample point to
                                         * center) in radians. */
        theta = atan(FLT_MAX);
        t = cos(theta + brushPtr->theta);
    } else {
        double theta;                   /* Angle of line (sample point to
                                         * center) in radians. */
        theta = atan(dy / dx);
        t = cos(theta + brushPtr->theta);
    }
    t = fabs(t);
    if (brushPtr->jitter.range > 0.0) {
        t += Jitter(&brushPtr->jitter);
        t = JCLAMP(t);
    }
    if (brushPtr->flags & BLT_PAINTBRUSH_SCALING_LOG) {
        t = log10(9.0 * t + 1.0);
    } 
    if (brushPtr->flags & BLT_PAINTBRUSH_DECREASING) {
        t = 1.0 - t;
    }        
    if (brushPtr->palette != NULL) {
        return Blt_Palette_GetAssociatedColor(brushPtr->palette, t);
    }
    color.Red   = (unsigned char)(brushPtr->low.Red   + t * brushPtr->rRange);
    color.Green = (unsigned char)(brushPtr->low.Green + t * brushPtr->gRange);
    color.Blue  = (unsigned char)(brushPtr->low.Blue  + t * brushPtr->bRange);
    color.Alpha = (unsigned char)(brushPtr->low.Alpha + t * brushPtr->aRange);
    color.Alpha = imul8x8(brushPtr->alpha, color.Alpha, t1);
    return color.u32;
}

const char *
Blt_GetBrushType(Blt_PaintBrush brush)
{
    PaintBrush *brushPtr = (PaintBrush *)brush;
    return brushPtr->classPtr->name;
}

int 
Blt_GetBrushTypeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                        Blt_PaintBrushType *typePtr)
{
    const char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 't') && (length > 1) && (strncmp(string, "tile", length) == 0)) {
        *typePtr = BLT_PAINTBRUSH_TILE;
    } else if ((c == 'l') && (length > 1)  &&
               (strncmp(string, "linear", length) == 0)) {
        *typePtr = BLT_PAINTBRUSH_LINEAR;
    } else if ((c == 'r') && (length > 1)  &&
               (strncmp(string, "radial", length) == 0)) {
        *typePtr = BLT_PAINTBRUSH_RADIAL;
    } else if ((c == 'c') && (length > 2)  &&
               (strncmp(string, "conical", length) == 0)) {
        *typePtr = BLT_PAINTBRUSH_CONICAL;
    } else if ((c == 'c') && (length > 2) &&
               (strncmp(string, "color", length) == 0)) {
        *typePtr = BLT_PAINTBRUSH_COLOR;
    } else if ((c == 's') && (length > 2) &&
               (strncmp(string, "stripe", length) == 0)) {
        *typePtr = BLT_PAINTBRUSH_STRIPE;
    } else if ((c == 'c') && (length > 2) &&
               (strncmp(string, "checker", length) == 0)) {
        *typePtr = BLT_PAINTBRUSH_CHECKER;
    } else {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "unknown paintbrush type \"", string, 
                "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetPaintBrushCmdFromObj --
 *
 *      Retrieves the paintbrush command named by the given the Tcl_Obj.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
GetPaintBrushCmdFromObj(Tcl_Interp *interp, PaintBrushCmdInterpData *dataPtr, 
                        Tcl_Obj *objPtr, PaintBrushCmd **cmdPtrPtr)
{
    Blt_HashEntry *hPtr;
    const char *string;

    string = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&dataPtr->instTable, string);
    if (hPtr == NULL) {
        Tcl_AppendResult(dataPtr->interp, "can't find paintbrush \"", 
                string, "\"", (char *)NULL);
        return TCL_ERROR;
    }
    *cmdPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyPaintBrushCmd --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyPaintBrushCmd(PaintBrushCmd *cmdPtr)
{
    Blt_FreeOptions(cmdPtr->specs, (char *)cmdPtr->brush, cmdPtr->display, 0);
    Blt_FreeBrush(cmdPtr->brush);
    if (cmdPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&cmdPtr->dataPtr->instTable, cmdPtr->hashPtr);
    }
    Blt_Free(cmdPtr);
}

int
Blt_ConfigurePaintBrush(Tcl_Interp *interp, Blt_PaintBrush brush)
{
    PaintBrush *brushPtr = (PaintBrush *)brush;

    if (brushPtr->classPtr->configProc != NULL) {
        return (*brushPtr->classPtr->configProc)(interp, brush);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NewTileBrush --
 *
 *      Creates a new tile paintbrush.
 *
 * Results:
 *      Returns pointer to the new paintbrush.
 *
 *---------------------------------------------------------------------------
 */
Blt_PaintBrush
Blt_NewTileBrush()
{
    Blt_TileBrush *brushPtr;
    
    brushPtr = Blt_AssertCalloc(1, sizeof(Blt_TileBrush));
    brushPtr->classPtr = &tileBrushClass;
    brushPtr->refCount = 1;
    brushPtr->alpha = 0xFF;
    JitterInit(&brushPtr->jitter);
    return (Blt_PaintBrush)brushPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NewLinearGradientBrush --
 *
 *      Creates a new linear gradient paintbrush.
 *
 * Results:
 *      Returns pointer to the new paintbrush.
 *
 *---------------------------------------------------------------------------
 */
Blt_PaintBrush
Blt_NewLinearGradientBrush()
{
    Blt_LinearGradientBrush *brushPtr;
    
    brushPtr = Blt_AssertCalloc(1, sizeof(Blt_LinearGradientBrush));
    brushPtr->classPtr = &linearGradientBrushClass;
    brushPtr->refCount = 1;
    brushPtr->alpha = 0xFF;
    brushPtr->from.x = 0.5;
    brushPtr->from.y = 0.0;
    brushPtr->to.x = 0.5;
    brushPtr->to.y = 1.0;
    JitterInit(&brushPtr->jitter);
    return (Blt_PaintBrush)brushPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewStripeBrush --
 *
 *      Creates a new stripe paintbrush.
 *
 * Results:
 *      Returns pointer to the new paintbrush.
 *
 *---------------------------------------------------------------------------
 */
Blt_PaintBrush
Blt_NewStripeBrush()
{
    Blt_StripeBrush *brushPtr;
    
    brushPtr = Blt_AssertCalloc(1, sizeof(Blt_StripeBrush));
    brushPtr->classPtr = &stripeBrushClass;
    brushPtr->refCount = 1;
    brushPtr->alpha = 0xFF;
    brushPtr->flags = BLT_PAINTBRUSH_ORIENT_VERTICAL;
    brushPtr->stride = 2;
    JitterInit(&brushPtr->jitter);
    return (Blt_PaintBrush)brushPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NewCheckerBrush --
 *
 *      Creates a new checker paintbrush.
 *
 * Results:
 *      Returns pointer to the new paintbrush.
 *
 *---------------------------------------------------------------------------
 */
Blt_PaintBrush
Blt_NewCheckerBrush()
{
    Blt_StripeBrush *brushPtr;
    
    brushPtr = Blt_AssertCalloc(1, sizeof(Blt_CheckerBrush));
    brushPtr->classPtr = &checkerBrushClass;
    brushPtr->refCount = 1;
    brushPtr->alpha = 0xFF;
    brushPtr->stride = 10;
    JitterInit(&brushPtr->jitter);
    return (Blt_PaintBrush)brushPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NewRadialGradientBrush --
 *
 *      Creates a new radial gradient paintbrush.
 *
 * Results:
 *      Returns pointer to the new paintbrush.
 *
 *---------------------------------------------------------------------------
 */
Blt_PaintBrush
Blt_NewRadialGradientBrush()
{
    Blt_RadialGradientBrush *brushPtr;
    
    brushPtr = Blt_AssertCalloc(1, sizeof(Blt_RadialGradientBrush));
    brushPtr->classPtr = &radialGradientBrushClass;
    brushPtr->refCount = 1;
    brushPtr->alpha = 0xFF;
    brushPtr->center.x = 0.5;
    brushPtr->center.y = 0.5;
    brushPtr->width = 1.0;
    brushPtr->height = 1.0;
    JitterInit(&brushPtr->jitter);
    return (Blt_PaintBrush)brushPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewConicalGradientBrush --
 *
 *      Creates a new conical gradient paintbrush.
 *
 * Results:
 *      Returns pointer to the new paintbrush.
 *
 *---------------------------------------------------------------------------
 */
Blt_PaintBrush
Blt_NewConicalGradientBrush()
{
    Blt_ConicalGradientBrush *brushPtr;
    
    brushPtr = Blt_AssertCalloc(1, sizeof(Blt_ConicalGradientBrush));
    brushPtr->classPtr = &conicalGradientBrushClass;
    brushPtr->refCount = 1;
    brushPtr->alpha = 0xFF;
    brushPtr->center.x = 0.5;
    brushPtr->center.y = 0.5;
    brushPtr->angle = 45;
    JitterInit(&brushPtr->jitter);
    return (Blt_PaintBrush)brushPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NewColorBrush --
 *
 *      Creates a new conical gradient paintbrush.
 *
 * Results:
 *      Returns pointer to the new paintbrush.
 *
 *---------------------------------------------------------------------------
 */
Blt_PaintBrush
Blt_NewColorBrush(unsigned int color)
{
    Blt_ColorBrush *brushPtr;
    
    brushPtr = Blt_AssertCalloc(1, sizeof(Blt_ColorBrush));
    brushPtr->refCount = 1;
    brushPtr->classPtr = &colorBrushClass;
    brushPtr->alpha = 0xFF;
    brushPtr->color.u32 = color;
    JitterInit(&brushPtr->jitter);
    return (Blt_PaintBrush)brushPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewPaintBrushCmd --
 *
 *      Creates a new paintbrush.
 *
 * Results:
 *      Returns pointer to the new paintbrush command.
 *
 *---------------------------------------------------------------------------
 */
static PaintBrushCmd *
NewPaintBrushCmd(PaintBrushCmdInterpData *dataPtr, Tcl_Interp *interp, 
                 Blt_PaintBrushType type)
{
    PaintBrushCmd *cmdPtr;

    cmdPtr = Blt_AssertCalloc(1, sizeof(PaintBrushCmd));
    switch (type) {
    case BLT_PAINTBRUSH_TILE:
        cmdPtr->brush = Blt_NewTileBrush();
        cmdPtr->specs = tileBrushSpecs;
        break;
    case BLT_PAINTBRUSH_LINEAR:
        cmdPtr->brush = Blt_NewLinearGradientBrush();
        cmdPtr->specs = linearGradientBrushSpecs;
        break;
    case BLT_PAINTBRUSH_STRIPE:
        cmdPtr->brush = Blt_NewStripeBrush();
        cmdPtr->specs = stripeBrushSpecs;
        break;
    case BLT_PAINTBRUSH_CHECKER:
        cmdPtr->brush = Blt_NewCheckerBrush();
        cmdPtr->specs = checkerBrushSpecs;
        break;
    case BLT_PAINTBRUSH_RADIAL:
        cmdPtr->brush = Blt_NewRadialGradientBrush();
        cmdPtr->specs = radialGradientBrushSpecs;
        break;
    case BLT_PAINTBRUSH_CONICAL:
        cmdPtr->brush = Blt_NewConicalGradientBrush();
        cmdPtr->specs = conicalGradientBrushSpecs;
        break;
    case BLT_PAINTBRUSH_COLOR:
        cmdPtr->brush = Blt_NewColorBrush(0xFFd9d9d9);
        cmdPtr->specs = colorBrushSpecs;
        break;
    default:
        abort();
        break;
    }
    cmdPtr->dataPtr = dataPtr;
    cmdPtr->tkwin = Tk_MainWindow(interp);
    cmdPtr->display = Tk_Display(cmdPtr->tkwin);
    return cmdPtr;
}

static int
ConfigurePaintBrushCmd(Tcl_Interp *interp, PaintBrushCmd *cmdPtr, int objc, 
                       Tcl_Obj *const *objv, int flags)
{
    imageOption.clientData = cmdPtr;
    if (Blt_ConfigureWidgetFromObj(interp, cmdPtr->tkwin, cmdPtr->specs,
        objc, objv, (char *)cmdPtr->brush, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_ConfigurePaintBrush(interp, cmdPtr->brush);
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *      Creates a new paintbrush object.
 *
 *      blt::paintbrush create type ?name? ?option values?...
 *
 *---------------------------------------------------------------------------
 */
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    PaintBrushCmdInterpData *dataPtr = clientData;
    PaintBrushCmd *cmdPtr;
    Blt_PaintBrushType type;
    const char *string;
    Blt_HashEntry *hPtr;

    if (Blt_GetBrushTypeFromObj(interp, objv[2], &type) != TCL_OK) {
        return TCL_ERROR;
    }
    hPtr = NULL;
    if (objc > 3) {
        string = Tcl_GetString(objv[3]);
        if (string[0] != '-') {         
            int isNew;

            hPtr = Blt_CreateHashEntry(&dataPtr->instTable, string, &isNew);
            if (!isNew) {
                Tcl_AppendResult(interp, "a paintbrush named \"", string, 
                                 "\" already exists.", (char *)NULL);
                return TCL_ERROR;
            }
            objc--, objv++;
        }
    }
    if (hPtr == NULL) {
        int isNew;
        char name[200];

        /* Generate a unique name for the paintbrush.  */
        do {
            Blt_FormatString(name, 200, "paintbrush%d", dataPtr->nextId++);
            hPtr = Blt_CreateHashEntry(&dataPtr->instTable, name, &isNew);
        } while (!isNew);
    } 
    cmdPtr = NewPaintBrushCmd(dataPtr, interp, type);
    if (cmdPtr == NULL) {
        Blt_DeleteHashEntry(&dataPtr->instTable, hPtr);
        return TCL_ERROR;
    }
    Blt_SetHashValue(hPtr, cmdPtr);
    cmdPtr->hashPtr = hPtr;
    cmdPtr->name = Blt_Strdup(Blt_GetHashKey(&dataPtr->instTable, hPtr));
    SetBrushName(cmdPtr->brush, cmdPtr->name);
    if (ConfigurePaintBrushCmd(interp, cmdPtr, objc-3, objv+3, 0) != TCL_OK) {
        DestroyPaintBrushCmd(cmdPtr);
        return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), cmdPtr->name, -1);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *      blt::paintbrush cget $brush ?option?...
 *
 *---------------------------------------------------------------------------
 */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    PaintBrushCmdInterpData *dataPtr = clientData;
    PaintBrushCmd *cmdPtr;
    
    if (GetPaintBrushCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    imageOption.clientData = cmdPtr;
    return Blt_ConfigureValueFromObj(interp, cmdPtr->tkwin, cmdPtr->specs,
        (char *)cmdPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      blt::paintbrush configure $brush ?option?...
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    PaintBrushCmdInterpData *dataPtr = clientData;
    PaintBrushCmd *cmdPtr;
    int flags;
    
    if (GetPaintBrushCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    flags = BLT_CONFIG_OBJV_ONLY;
    if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, cmdPtr->tkwin, cmdPtr->specs,
                (char *)cmdPtr->brush, (Tcl_Obj *)NULL, flags);
    } else if (objc == 4) {
        return Blt_ConfigureInfoFromObj(interp, cmdPtr->tkwin, cmdPtr->specs,
                (char *)cmdPtr->brush, objv[3], flags);
    } 
    if (ConfigurePaintBrushCmd(interp, cmdPtr, objc-3, objv+3, flags)!=TCL_OK) {
        return TCL_ERROR;
    }
    NotifyClients(cmdPtr->brush);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes one or more paintbrush objects.
 *
 *      blt::paintbrush delete brushName ... 
 *
 *---------------------------------------------------------------------------
 */
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    PaintBrushCmdInterpData *dataPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        Blt_HashEntry *hPtr;
        PaintBrushCmd *cmdPtr;
        const char *name;

        name = Tcl_GetString(objv[i]);
        hPtr = Blt_FindHashEntry(&dataPtr->instTable, name);
        if (hPtr == NULL) {
            Tcl_AppendResult(interp, "can't find paintbrush \"",
                             name, "\"", (char *)NULL);
            return TCL_ERROR;
        }
        cmdPtr = Blt_GetHashValue(hPtr);
        assert(cmdPtr->hashPtr == hPtr);
        DestroyPaintBrushCmd(cmdPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamesOp --
 *
 *      blt::paintbrush names ?pattern ... ?
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    PaintBrushCmdInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&dataPtr->instTable, &iter);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        PaintBrushCmd *cmdPtr;
        Tcl_Obj *objPtr;
        
        cmdPtr = Blt_GetHashValue(hPtr);
        if (objc == 3) {
            if (!Tcl_StringMatch(cmdPtr->name, Tcl_GetString(objv[2]))) {
                continue;
            }
        }
        objPtr = Tcl_NewStringObj(cmdPtr->name, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 *      Returns the type of the paintbrush 
 *
 *      blt::paintbrush type brushName
 *
 *---------------------------------------------------------------------------
 */
static int
TypeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    PaintBrushCmdInterpData *dataPtr = clientData;
    PaintBrushCmd *cmdPtr;
    Tcl_Obj *objPtr;
    
    if (GetPaintBrushCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    objPtr = Tcl_NewStringObj(Blt_GetBrushType(cmdPtr->brush), -1);
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PaintBrushCmdProc --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec paintbrushOps[] =
{
    {"cget",      2, CgetOp,      4, 4, "brushName option",},
    {"configure", 2, ConfigureOp, 3, 0, "brushName ?option value ...?",},
    {"create",    2, CreateOp,    3, 0, "type ?name? ?option value ...?",},
    {"delete",    1, DeleteOp,    2, 0, "?brushName ...?",},
    {"names",     1, NamesOp,     2, 3, "brushName ?pattern ...?",},
    {"type",      1, TypeOp,      3, 3, "brushName",},
};
static int numPaintBrushOps = sizeof(paintbrushOps) / sizeof(Blt_OpSpec);

static int
PaintBrushCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
                 Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numPaintBrushOps, paintbrushOps, 
        BLT_OP_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * PaintBrushCmdInterpDeleteProc --
 *
 *      Called when the interpreter is destroyed, this routine frees all
 *      the paint brushes previously created and still available, and then
 *      deletes the hash table that keeps track of them.
 *
 *      We have to wait for the deletion of the interpreter and not the
 *      "blt::paintbrush" command because 1) we need all the clients of a
 *      paintbrush to release them before we can remove the hash table and
 *      2) we can't guarantee that "blt::paintbrush" command will be deleted
 *      after all the clients.
 *
 *---------------------------------------------------------------------------
 */
static void
PaintBrushCmdInterpDeleteProc(
    ClientData clientData,              /* Interpreter-specific data. */
    Tcl_Interp *interp)
{
    PaintBrushCmdInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&dataPtr->instTable, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        PaintBrushCmd *cmdPtr;

        cmdPtr = Blt_GetHashValue(hPtr);
        cmdPtr->hashPtr = NULL;
        Blt_Free(cmdPtr);
    }
    Blt_DeleteHashTable(&dataPtr->instTable);
    Tcl_DeleteAssocData(dataPtr->interp, PAINTBRUSH_THREAD_KEY);
    Blt_Free(dataPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetPaintBrushCmdInterpData --
 *
 *---------------------------------------------------------------------------
 */
static PaintBrushCmdInterpData *
GetPaintBrushCmdInterpData(Tcl_Interp *interp)
{
    PaintBrushCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (PaintBrushCmdInterpData *)
        Tcl_GetAssocData(interp, PAINTBRUSH_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
        dataPtr = Blt_AssertMalloc(sizeof(PaintBrushCmdInterpData));
        dataPtr->interp = interp;
        dataPtr->nextId = 1;

        Tcl_SetAssocData(interp, PAINTBRUSH_THREAD_KEY, 
                PaintBrushCmdInterpDeleteProc, dataPtr);
        Blt_InitHashTable(&dataPtr->instTable, BLT_STRING_KEYS);
    }
    return dataPtr;
}

static int
IncrBrushRefCount(Blt_PaintBrush brush)
{
     PaintBrush *brushPtr = (PaintBrush *)brush;
     return brushPtr->refCount++;
}


/*LINTLIBRARY*/
int
Blt_PaintBrushCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "paintbrush", PaintBrushCmdProc, };

    cmdSpec.clientData = GetPaintBrushCmdInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

void
Blt_SetLinearGradientBrushPalette(Blt_PaintBrush brush, Blt_Palette palette)
{
    Blt_LinearGradientBrush *brushPtr = (Blt_LinearGradientBrush *)brush;

    brushPtr->palette = palette;
}


void
Blt_SetBrushOrigin(Blt_PaintBrush brush, int x, int y)
{
    PaintBrush *brushPtr = (PaintBrush *)brush;

    brushPtr->xOrigin = x;
    brushPtr->yOrigin = y;
}


void 
Blt_SetTileBrushPicture(Blt_PaintBrush brush, Blt_Picture picture)
{
    Blt_TileBrush *brushPtr = (Blt_TileBrush *)brush;

    brushPtr->tile = picture;
    if (Blt_Picture_IsAssociated(brushPtr->tile)) {
        Blt_UnassociateColors(brushPtr->tile);
    }
}

void 
Blt_SetColorBrushColor(Blt_PaintBrush brush, unsigned int value)
{
    Blt_ColorBrush *brushPtr = (Blt_ColorBrush *)brush;

    brushPtr->reqColor.u32 = value;
    brushPtr->color.u32 = brushPtr->reqColor.u32;
    brushPtr->color.Alpha = brushPtr->alpha;
    Blt_AssociateColor(&brushPtr->color);
}

void
Blt_SetLinearGradientBrushColors(Blt_PaintBrush brush, Blt_Pixel *lowPtr, 
                                 Blt_Pixel *highPtr)
{
    Blt_LinearGradientBrush *brushPtr = (Blt_LinearGradientBrush *)brush;

    brushPtr->low.u32 = lowPtr->u32;
    brushPtr->high.u32 = highPtr->u32;
}

void
Blt_SetBrushOpacity(Blt_PaintBrush brush, double percent)
{
     PaintBrush *brushPtr = (PaintBrush *)brush;
     double opacity;
     
     opacity = (percent / 100.0) * 255.0;
     brushPtr->alpha = ROUND(opacity);
}

void
Blt_SetBrushRegion(Blt_PaintBrush brush, int x, int y, int w, int h)
{
    PaintBrush *brushPtr = (PaintBrush *)brush;
    
    if (brushPtr->classPtr->initProc != NULL) {
        (*brushPtr->classPtr->initProc)(brush, x, y, w, h);
    }
}

void 
Blt_SetLinearGradientBrushCalcProc(Blt_PaintBrush brush,
                                   Blt_PaintBrushCalcProc *proc,
                                   ClientData clientData)
{
    Blt_LinearGradientBrush *brushPtr = (Blt_LinearGradientBrush *)brush;

    brushPtr->calcProc = proc;
    brushPtr->clientData = clientData;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetAssociatedColorFromBrush --
 *
 *      Gets the color from the paint brush at the given x,y coordinate.
 *      For texture, gradient, and tile brushes, the coordinate is used to
 *      compute the color.  The return color is always associated
 *      (pre-multiplied).
 *
 * Results:
 *      Returns the color at the current x,y coordinate.  The color
 *      is always associated. 
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetAssociatedColorFromBrush(Blt_PaintBrush brush, int x, int y)
{
    PaintBrush *brushPtr = (PaintBrush *)brush;
    
    if (brushPtr->classPtr->colorProc != NULL) {
        return (*brushPtr->classPtr->colorProc)(brush, x, y);
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_CreateBrushNotifier
 *
 *      Adds a callback to invoked when the brush changes.  
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/*LINTLIBRARY*/
void
Blt_CreateBrushNotifier(
    Blt_PaintBrush brush,               /* Paint brush with which to
                                         * register callback. */
    Blt_BrushChangedProc *notifyProc,   /* Function to call when brush has
                                         * changed. */
    ClientData clientData)
{
    PaintBrush *brushPtr = (PaintBrush *)brush;
    PaintBrushNotifier *notifyPtr;
    Blt_ChainLink link;
    
    if (brushPtr->notifiers == NULL) {
        brushPtr->notifiers = Blt_Chain_Create();
    }
     for (link = Blt_Chain_FirstLink(brushPtr->notifiers); link != NULL;
        link = Blt_Chain_NextLink(link)) {
         PaintBrushNotifier *notifyPtr;

         notifyPtr = Blt_Chain_GetValue(link);
         if ((notifyPtr->proc == notifyProc) &&
             (notifyPtr->clientData == clientData)) {
             notifyPtr->clientData = clientData;
             return;                    /* Notifier already exists. */
         }
     }
    link = Blt_Chain_AllocLink(sizeof(PaintBrushNotifier));
    notifyPtr = Blt_Chain_GetValue(link);
    notifyPtr->proc = notifyProc;
    notifyPtr->clientData = clientData;
    Blt_Chain_LinkAfter(brushPtr->notifiers, link, NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DeleteBrushNotifier
 *
 *      Removes the notification callback.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DeleteBrushNotifier(
    Blt_PaintBrush brush,               /* Paint brush with which to
                                         * register callback. */
    Blt_BrushChangedProc *notifyProc,   /* Function to call when brush has
                                         * changed. */
    ClientData clientData)
{
    PaintBrush *brushPtr = (PaintBrush *)brush;
    Blt_ChainLink link;
    
     for (link = Blt_Chain_FirstLink(brushPtr->notifiers); link != NULL;
        link = Blt_Chain_NextLink(link)) {
         PaintBrushNotifier *notifyPtr;

         notifyPtr = Blt_Chain_GetValue(link);
         if ((notifyPtr->proc == notifyProc) &&
             (notifyPtr->clientData == clientData)) {
             Blt_Chain_DeleteLink(brushPtr->notifiers, link);
             return;
         }
     }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FreeBrush
 *
 *      Releases the paintbrush structure.  If no other client is using the
 *      paintbrush structure, then it is freed.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Memory is freed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FreeBrush(Blt_PaintBrush brush)
{
    PaintBrush *brushPtr = (PaintBrush *)brush;

    brushPtr->refCount--;
    if (brushPtr->refCount <= 0) {
        if (brushPtr->classPtr->freeProc != NULL) {
            (*brushPtr->classPtr->freeProc)(brush);
        }
        if (brushPtr->notifiers != NULL) {
            Blt_Chain_Destroy(brushPtr->notifiers);
        }
        Blt_Free(brushPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPaintBrush --
 *
 *      Retrieves the paintbrush object named by the given the string.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetPaintBrush(Tcl_Interp *interp, const char *string, 
                  Blt_PaintBrush *brushPtr)
{
    Blt_HashEntry *hPtr;
    PaintBrushCmd *cmdPtr;
    PaintBrushCmdInterpData *dataPtr;

    dataPtr = GetPaintBrushCmdInterpData(interp);
    hPtr = Blt_FindHashEntry(&dataPtr->instTable, string);
    if (hPtr == NULL) { 
        Blt_Pixel color;
        
        /* The paintbrush doesn't already exist, so see if it's a color
         * name (something that Tk_Get3DBorder will accept). If it's a
         * valid color, then automatically create a single color brush out
         * of it. */
        if (Blt_GetPixel(interp, string, &color) != TCL_OK) {
            return TCL_ERROR;           /* Nope. It's an error. */
        } 
        *brushPtr = Blt_NewColorBrush(color.u32);
        SetBrushName(*brushPtr, Blt_Strdup(string));
    } else {
        cmdPtr = Blt_GetHashValue(hPtr);
        assert(cmdPtr != NULL);
        IncrBrushRefCount(cmdPtr->brush);
        *brushPtr = cmdPtr->brush;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetPaintBrushFromObj --
 *
 *      Retrieves the paintbrush command named by the given the Tcl_Obj.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetPaintBrushFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                   Blt_PaintBrush *brushPtr)
{
    const char *string;

    string = Tcl_GetString(objPtr);
    return Blt_GetPaintBrush(interp, string, brushPtr);
}

int
Blt_GetBrushAlpha(Blt_PaintBrush brush)
{
     PaintBrush *brushPtr = (PaintBrush *)brush;
     return brushPtr->alpha;
}

const char *
Blt_GetBrushName(Blt_PaintBrush brush)
{
     PaintBrush *brushPtr = (PaintBrush *)brush;

     if (brushPtr->name == NULL) {
         return "???";
     }
     return brushPtr->name;
}

const char *
Blt_GetBrushColor(Blt_PaintBrush brush)
{
     Blt_ColorBrush *brushPtr = (Blt_ColorBrush *)brush;
     
     if (brushPtr->classPtr->type != BLT_PAINTBRUSH_COLOR) {
         return "???";
     }
     return Blt_NameOfPixel(&brushPtr->reqColor);
}

void
Blt_GetBrushOrigin(Blt_PaintBrush brush, int *xPtr, int *yPtr)
{
    PaintBrush *brushPtr = (PaintBrush *)brush;

    *xPtr = brushPtr->xOrigin;
    *yPtr = brushPtr->yOrigin;
}


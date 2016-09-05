/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltBg.c --
 *
 * This module creates backgrounds for the BLT toolkit.
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
  #include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#include <limits.h>

#include <X11/Xutil.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltImage.h"
#include "bltPalette.h"
#include "bltPicture.h"
#include "bltBg.h"
#include "bltPainter.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define JCLAMP(c)       ((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))

#define REPEAT_MASK \
    (BLT_PAINTBRUSH_REPEAT_NORMAL|BLT_PAINTBRUSH_REPEAT_OPPOSITE)
#define ORIENT_MASK \
    (BLT_PAINTBRUSH_ORIENT_VERTICAL|BLT_PAINTBRUSH_ORIENT_HORIZONTAL)
#define COLOR_SCALE_MASK \
        (BLT_PAINTBRUSH_SCALING_LINEAR|BLT_PAINTBRUSH_SCALING_LOG)

#define BG_BACKGROUND_THREAD_KEY        "BLT Background Data"

typedef struct _Blt_PaintBrush PaintBrush;

typedef struct {
    Blt_HashTable instTable;            /* Hash table of background
                                         * structures keyed by the name of
                                         * the image. */
    Tcl_Interp *interp;                 /* Interpreter associated with this
                                         * set of backgrounds. */
    int nextId;                         /* Serial number of the identifier
                                         * to be used for next background
                                         * created.  */
} BackgroundInterpData;

typedef struct {
    unsigned int flags;
    BackgroundInterpData *dataPtr;
    Display *display;                   /* Display of this background. */
    Tk_Window tkwin;                    /* Main window. Used to query
                                         * background options. */
    const char *name;                   /* Generated name of background. */
    Blt_HashEntry *hashPtr;             /* Hash entry in background table. */
    Blt_ChainLink link;                 /* Background token that is
                                         * associated with the background
                                         * creation "background
                                         * create...". */
    Blt_Chain chain;                    /* List of background tokens.  Used
                                         * to register callbacks for each
                                         * client of the background. */
    Tk_3DBorder border;                 /* 3D Border.  May be used for all
                                         * background types. */
    Tcl_Obj *refNameObjPtr;             /* Name of reference window. */
    Tk_Window tkRef;                    /* Reference window to use compute
                                         * positions from relative
                                         * coordinates. */
    Blt_PaintBrush brush;               /* Paint brush representing the
                                         * background color. */
    Blt_ConfigSpec *specs;              /* Configuration specifications
                                         * this background. */
    Blt_HashTable instTable;
} BackgroundObject;

#define REFERENCE_PENDING        (1<<0)
#define RELATIVETO_SELF          (1<<1)
#define RELATIVETO_TOPLEVEL      (1<<2)
#define RELATIVETO_WINDOW        (1<<3)
#define RELATIVETO_MASK \
    (RELATIVETO_SELF|RELATIVETO_TOPLEVEL|RELATIVETO_WINDOW)
#define BACKGROUND_SOLID        (1<<5)

struct _Blt_Bg {
    BackgroundObject *corePtr;          /* Pointer to master background. */
    Blt_BackgroundChangedProc *notifyProc;
    ClientData clientData;              /* Data to be passed on notifier
                                         * callbacks.  */
    Blt_ChainLink link;                 /* Entry in notifier list. */
};

typedef struct _Blt_Bg Bg;

/* 
 * BgInstance --
 *
 *      A single background can be used to refer to multiple reference
 *      windows by using the "self" and "toplevel" references.  This
 *      structure is stored in a hash table keyed by the different
 *      reference windows.
 */
typedef struct _BgInstance {
    BackgroundObject *corePtr;          /* Pointer to master background. */
    Blt_HashEntry *hashPtr;
    unsigned int flags;
    Pixmap pixmap;                      /* Cached pixmap associated with
                                         * reference window. */
    GC gc;                              /* GC associated with reference
                                         * window. This is used to draw the
                                         * background rectangles and
                                         * polygons.  The above pixmap will
                                         * be selected as its tile. */
    Display *display;                   /* Display of this reference. */
    Tk_Window tkwin;
    int width, height;                  /* Current size of reference
                                         * window. */
} BgInstance;

#define NOTIFY_PENDING          (1<<16)

#define DEF_BORDER              STD_NORMAL_BACKGROUND
#define DEF_CENTER              "0"
#define DEF_CHECKERS_OFFCOLOR    "grey97"
#define DEF_CHECKERS_ONCOLOR     "grey90"
#define DEF_CHECKERS_STRIDE      "10"
#define DEF_COLOR               STD_NORMAL_BACKGROUND
#define DEF_COLOR_SCALE         "linear"
#define DEF_CONICAL_CENTER       "c"
#define DEF_CONICAL_DIAMETER     "0.0"
#define DEF_CONICAL_HEIGHT       "1.0"
#define DEF_CONICAL_ROTATE       "45.0"
#define DEF_CONICAL_WIDTH        "1.0"
#define DEF_DECREASING          "0"
#define DEF_FROM                "top center"
#define DEF_HIGH_COLOR          "grey90"
#define DEF_JITTER              "0"
#define DEF_LOW_COLOR           "grey50"
#define DEF_PALETTE             (char *)NULL
#define DEF_RADIAL_CENTER       "c"
#define DEF_RADIAL_DIAMETER     "0.0"
#define DEF_RADIAL_HEIGHT       "1.0"
#define DEF_RADIAL_WIDTH        "1.0"
#define DEF_RELATIVETO           "toplevel"
#define DEF_REPEAT              "no"
#define DEF_STRIPES_OFFCOLOR    "grey97"
#define DEF_STRIPES_ONCOLOR     "grey90"
#define DEF_STRIPES_ORIENT      "vertical"
#define DEF_STRIPES_STRIDE      "2"
#define DEF_TEXTURE_TYPE        "stripes"
#define DEF_TO                  (char *)NULL
#define DEF_XORIGIN             "0"
#define DEF_YORIGIN             "0"

static Blt_OptionParseProc ObjToImage;
static Blt_OptionPrintProc ImageToObj;
static Blt_OptionFreeProc FreeImage;
static Blt_CustomOption imageOption =
{
    ObjToImage, ImageToObj, FreeImage, (ClientData)0
};

static Blt_OptionParseProc ObjToColorScale;
static Blt_OptionPrintProc ColorScaleToObj;
static Blt_CustomOption colorScaleOption =
{
    ObjToColorScale, ColorScaleToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToJitter;
static Blt_OptionPrintProc JitterToObj;
static Blt_CustomOption jitterOption =
{
    ObjToJitter, JitterToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToReference;
static Blt_OptionPrintProc ReferenceToObj;
static Blt_OptionFreeProc FreeReference;
static Blt_CustomOption referenceOption =
{
    ObjToReference, ReferenceToObj, FreeReference, (ClientData)0
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

static Blt_OptionParseProc ObjToOrient;
static Blt_OptionPrintProc OrientToObj;
static Blt_CustomOption orientOption =
{
    ObjToOrient, OrientToObj, NULL, (ClientData)0
};

static Blt_ConfigSpec bgSpecs[] =
{
    {BLT_CONFIG_SYNONYM, "-background", "color", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "color", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_BORDER, "-border", "color", "Color", DEF_BORDER, 
        Blt_Offset(BackgroundObject, border), 0},
    {BLT_CONFIG_CUSTOM, "-relativeto", (char *)NULL, (char *)NULL, 
        DEF_RELATIVETO, 0, BLT_CONFIG_DONT_SET_DEFAULT, &referenceOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec tileBrushSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-image", (char *)NULL, (char *)NULL, (char *)NULL,
        Blt_Offset(Blt_TileBrush, tkImage), BLT_CONFIG_DONT_SET_DEFAULT,
        &imageOption},
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL, DEF_JITTER,
        Blt_Offset(Blt_TileBrush, jitter.range), BLT_CONFIG_DONT_SET_DEFAULT,
        &jitterOption},
    {BLT_CONFIG_PIXELS, "-xoffset", (char *)NULL, (char *)NULL, DEF_XORIGIN,
        Blt_Offset(Blt_TileBrush, xOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", (char *)NULL, (char *)NULL, DEF_YORIGIN,
        Blt_Offset(Blt_TileBrush, yOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec linearGradientBrushSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-colorscale", (char *)NULL, (char *)NULL,
        DEF_COLOR_SCALE, Blt_Offset(Blt_LinearGradientBrush, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, &colorScaleOption},
    {BLT_CONFIG_BITMASK, "-decreasing", (char *)NULL, (char *)NULL,
        DEF_DECREASING, Blt_Offset(Blt_LinearGradientBrush, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)BLT_PAINTBRUSH_DECREASING},
    {BLT_CONFIG_CUSTOM, "-from", (char *)NULL, (char *)NULL, DEF_FROM,
        Blt_Offset(Blt_LinearGradientBrush, from), 
        BLT_CONFIG_DONT_SET_DEFAULT, &positionOption},
    {BLT_CONFIG_PIX32, "-highcolor", (char *)NULL, (char *)NULL,
        DEF_HIGH_COLOR, Blt_Offset(Blt_LinearGradientBrush, high), 0},
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL,
        DEF_JITTER, Blt_Offset(Blt_LinearGradientBrush, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-lowcolor", (char *)NULL, (char *)NULL,
        DEF_LOW_COLOR, Blt_Offset(Blt_LinearGradientBrush, low), 0},
    {BLT_CONFIG_CUSTOM, "-palette", (char *)NULL, (char *)NULL,
        DEF_PALETTE, Blt_Offset(Blt_LinearGradientBrush, palette), 
        BLT_CONFIG_DONT_SET_DEFAULT, &paletteOption},
    {BLT_CONFIG_CUSTOM, "-repeat", (char *)NULL, (char *)NULL,
        DEF_REPEAT, Blt_Offset(Blt_LinearGradientBrush, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, &repeatOption},
    {BLT_CONFIG_CUSTOM, "-to", (char *)NULL, (char *)NULL, DEF_TO,
        Blt_Offset(Blt_LinearGradientBrush, to), 
        BLT_CONFIG_DONT_SET_DEFAULT, &positionOption},
    {BLT_CONFIG_PIXELS, "-xoffset", (char *)NULL, (char *)NULL, DEF_XORIGIN,
        Blt_Offset(Blt_LinearGradientBrush, xOrigin),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", (char *)NULL, (char *)NULL, DEF_YORIGIN,
        Blt_Offset(Blt_LinearGradientBrush, yOrigin),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec stripesBrushSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL,
        DEF_JITTER, Blt_Offset(Blt_StripesBrush, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-offcolor", (char *)NULL, (char *)NULL,
        DEF_STRIPES_OFFCOLOR, Blt_Offset(Blt_StripesBrush, high)},
    {BLT_CONFIG_PIX32, "-oncolor", (char *)NULL, (char *)NULL,
        DEF_STRIPES_ONCOLOR, Blt_Offset(Blt_StripesBrush, low)},
    {BLT_CONFIG_CUSTOM, "-orient", (char *)NULL, (char *)NULL,
        DEF_STRIPES_ORIENT, Blt_Offset(Blt_StripesBrush, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, &orientOption},
    {BLT_CONFIG_PIXELS_POS, "-stride", (char *)NULL, (char *)NULL,
        DEF_STRIPES_STRIDE, Blt_Offset(Blt_StripesBrush, stride), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-xoffset", (char *)NULL, (char *)NULL, DEF_XORIGIN,
        Blt_Offset(Blt_StripesBrush, xOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", (char *)NULL, (char *)NULL, DEF_YORIGIN,
        Blt_Offset(Blt_StripesBrush, yOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec checkersBrushSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL,
        DEF_JITTER, Blt_Offset(Blt_CheckersBrush, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-offcolor", (char *)NULL, (char *)NULL,
        DEF_CHECKERS_OFFCOLOR, Blt_Offset(Blt_CheckersBrush, high)},
    {BLT_CONFIG_PIX32, "-oncolor", (char *)NULL, (char *)NULL,
        DEF_CHECKERS_ONCOLOR, Blt_Offset(Blt_CheckersBrush, low)},
    {BLT_CONFIG_PIXELS_POS, "-stride", (char *)NULL, (char *)NULL,
        DEF_CHECKERS_STRIDE, Blt_Offset(Blt_CheckersBrush, stride), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-xoffset", (char *)NULL, (char *)NULL, DEF_XORIGIN,
        Blt_Offset(Blt_CheckersBrush, xOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", (char *)NULL, (char *)NULL, DEF_YORIGIN,
        Blt_Offset(Blt_CheckersBrush, yOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec radialGradientBrushSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-center", (char *)NULL, (char *)NULL,
        DEF_RADIAL_CENTER, Blt_Offset(Blt_RadialGradientBrush, center), 
        BLT_CONFIG_DONT_SET_DEFAULT, &positionOption},
    {BLT_CONFIG_CUSTOM, "-colorscale", (char *)NULL, (char *)NULL,
        DEF_COLOR_SCALE, Blt_Offset(Blt_RadialGradientBrush, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, &colorScaleOption},
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
    {BLT_CONFIG_CUSTOM, "-palette", (char *)NULL, (char *)NULL,
        DEF_PALETTE, Blt_Offset(Blt_RadialGradientBrush, palette), 
        BLT_CONFIG_DONT_SET_DEFAULT, &paletteOption},
    {BLT_CONFIG_CUSTOM, "-repeat", (char *)NULL, (char *)NULL, DEF_REPEAT,
        Blt_Offset(Blt_RadialGradientBrush, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, &repeatOption},
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
        BLT_CONFIG_DONT_SET_DEFAULT, &colorScaleOption},
    {BLT_CONFIG_BITMASK, "-decreasing", (char *)NULL, (char *)NULL,
        DEF_DECREASING, Blt_Offset(Blt_ConicalGradientBrush, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)BLT_PAINTBRUSH_DECREASING},
    {BLT_CONFIG_PIX32, "-highcolor", (char *)NULL, (char *)NULL,
        DEF_HIGH_COLOR, Blt_Offset(Blt_ConicalGradientBrush, high)},
    {BLT_CONFIG_CUSTOM, "-jitter", (char *)NULL, (char *)NULL,
        DEF_JITTER, Blt_Offset(Blt_ConicalGradientBrush, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-lowcolor", (char *)NULL, (char *)NULL, DEF_LOW_COLOR,
        Blt_Offset(Blt_ConicalGradientBrush, low), 0},
    {BLT_CONFIG_CUSTOM, "-palette", (char *)NULL, (char *)NULL, DEF_PALETTE,
        Blt_Offset(Blt_ConicalGradientBrush, palette), 
        BLT_CONFIG_DONT_SET_DEFAULT, &paletteOption},
    {BLT_CONFIG_DOUBLE, "-rotate", (char *)NULL, (char *)NULL,
        DEF_CONICAL_ROTATE, Blt_Offset(Blt_ConicalGradientBrush, angle),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-xoffset", (char *)NULL, (char *)NULL, DEF_XORIGIN,
        Blt_Offset(Blt_ConicalGradientBrush, xOrigin),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", (char *)NULL, (char *)NULL, DEF_YORIGIN,
        Blt_Offset(Blt_ConicalGradientBrush, yOrigin),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static void NotifyClients(BackgroundObject *corePtr);

static Tcl_IdleProc SetReferenceWindowFromPath;
static Tcl_IdleProc NotifyProc;

static Tk_Window
GetReferenceWindow(BackgroundObject *corePtr, Tk_Window tkwin)
{
    Tk_Window tkRef;
    
    tkRef = NULL;
    switch (corePtr->flags & RELATIVETO_MASK) {
    case RELATIVETO_SELF:
        tkRef = tkwin;                              break;
    case RELATIVETO_TOPLEVEL:
        tkRef = Blt_Toplevel(tkwin);                break;
    case RELATIVETO_WINDOW:                      
        tkRef = corePtr->tkRef;                     break;
    }
    return tkRef;
}
        
static void
EventuallyNotify(BgInstance *instPtr)
{
    if ((instPtr->flags & NOTIFY_PENDING) == 0) {
        instPtr->flags |= NOTIFY_PENDING;
        Tcl_DoWhenIdle(NotifyProc, instPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * InstanceEventProc --
 *
 *      This procedure is invoked by the Tk event handler when
 *      StructureNotify events occur in a reference window managed by the
 *      background.  Specifically we need to know if the reference window
 *      was destroyed and dereference the pointer to it.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
InstanceEventProc(ClientData clientData, XEvent *eventPtr)
{
    BgInstance *instPtr = clientData;

    if ((eventPtr->type == DestroyNotify) && (instPtr->tkwin != NULL) &&
        (eventPtr->xany.window == Tk_WindowId(instPtr->tkwin))) {
        instPtr->tkwin = NULL;
    } else if (eventPtr->type == ConfigureNotify) {
        if ((instPtr->corePtr->flags & BACKGROUND_SOLID) == 0) {
            EventuallyNotify(instPtr);
        }
    }
}

static void
DestroyInstance(BgInstance *instPtr)
{
    BackgroundObject *corePtr;
    
    corePtr = instPtr->corePtr;
    if (instPtr->flags & NOTIFY_PENDING) {
        instPtr->flags &= ~NOTIFY_PENDING;
        Tcl_CancelIdleCall(NotifyProc, instPtr);
    }
    if (instPtr->pixmap != None) {
        Tk_FreePixmap(instPtr->display, instPtr->pixmap);
    }
    if (instPtr->gc != None) {
        Blt_FreePrivateGC(instPtr->display, instPtr->gc);
    }
    if (instPtr->tkwin != NULL) {
        Tk_DeleteEventHandler(instPtr->tkwin, StructureNotifyMask,
                InstanceEventProc, instPtr);
    }
    if (instPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&corePtr->instTable, instPtr->hashPtr);
    }
    Blt_Free(instPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ClearInstances --
 *
 *      Remove all the references associated with this background.  
 *      StructureNotify events occur in a reference window managed by the
 *      background.  Specifically we need to know if the reference window
 *      was destroyed and dereference the pointer to it.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
ClearInstances(BackgroundObject *corePtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&corePtr->instTable, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        BgInstance *instPtr;

        instPtr = Blt_GetHashValue(hPtr);
        instPtr->hashPtr = NULL;
        DestroyInstance(instPtr);
    }
    Blt_DeleteHashTable(&corePtr->instTable);
    Blt_InitHashTable(&corePtr->instTable, sizeof(BgInstance)/sizeof(int));
}

static void
NotifyProc(ClientData clientData)
{
    BgInstance *instPtr = clientData;
    BackgroundObject *corePtr;

    corePtr = instPtr->corePtr;
    DestroyInstance(instPtr);
    NotifyClients(corePtr);
}


static int 
GetBackgroundTypeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
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
    } else if ((c == 's') && (length > 2) &&
               (strncmp(string, "stripes", length) == 0)) {
        *typePtr = BLT_PAINTBRUSH_STRIPES;
    } else if ((c == 'c') && (length > 2) &&
               (strncmp(string, "checkers", length) == 0)) {
        *typePtr = BLT_PAINTBRUSH_CHECKERS;
    } else {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "unknown background type \"", string, 
                "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }
    return TCL_OK;
}

static unsigned int
GetBackgroundColor(BackgroundObject *corePtr)
{
    return Blt_XColorToPixel(Tk_3DBorderColor(corePtr->border));
}

/*
 *---------------------------------------------------------------------------
 *
 * SetReferenceWindowFromPath --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
SetReferenceWindowFromPath(ClientData clientData)
{
    BackgroundObject *corePtr = clientData;
    Tcl_Interp *interp;
    Tk_Window tkwin, tkMain;
    const char *string;
    
    interp = corePtr->dataPtr->interp;
    corePtr->flags &= ~REFERENCE_PENDING;
    tkMain = Tk_MainWindow(interp);
    string = Tcl_GetString(corePtr->refNameObjPtr);
    tkwin = Tk_NameToWindow(interp, string, tkMain);
    if (tkwin == NULL) {
        Tcl_BackgroundError(interp);
        return;
    }
    if (corePtr->tkRef != NULL) {
        ClearInstances(corePtr);
    }
    corePtr->tkRef = tkwin;
}

static void
GetReferenceWindowDimensions(BackgroundObject *corePtr, Tk_Window tkwin,
                             int *widthPtr, int *heightPtr)
{
    Tk_Window tkRef;

    *heightPtr = *widthPtr = 0;
    tkRef = GetReferenceWindow(corePtr, tkwin);
    if (tkRef != NULL) {
        *widthPtr = Tk_Width(tkRef);
        *heightPtr = Tk_Height(tkRef);
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
ImageChangedProc(ClientData clientData, int x, int y, int width, int height,
                 int imageWidth, int imageHeight)
{
    BackgroundObject *corePtr = clientData;
    Blt_TileBrush *brushPtr = (Blt_TileBrush *)corePtr->brush;
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
    brushPtr->tile = Blt_GetPictureFromImage(corePtr->dataPtr->interp,
        brushPtr->tkImage, &isNew);
    if (Blt_Picture_IsPremultiplied(brushPtr->tile)) {
        Blt_UnmultiplyColors(brushPtr->tile);
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
    BackgroundObject *corePtr = clientData;

    tkImage = Tk_GetImage(interp, corePtr->tkwin, Tcl_GetString(objPtr), 
        ImageChangedProc, corePtr);
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
        string = Tcl_GetString(objv[1]);
        c = string[0];
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

    if (*palPtr != NULL) {
        Blt_PaintBrush brush = (Blt_PaintBrush)widgRec;

        Blt_Palette_DeleteNotifier(*palPtr, PaletteChangedProc, brush);
        *palPtr = NULL;
    }
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
    Blt_PaintBrush brush = (Blt_PaintBrush)(widgRec);
    const char *string;
    
    string = Tcl_GetString(objPtr);
    if ((string == NULL) || (string[0] == '\0')) {
        FreePalette(clientData, Tk_Display(tkwin), widgRec, offset);
        return TCL_OK;
    }
    if (Blt_Palette_GetFromObj(interp, objPtr, palPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_Palette_CreateNotifier(*palPtr, PaletteChangedProc, brush);
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
 * ObjToColorScale --
 *
 *      Translates the given string to the gradient scale it represents.  
 *      Valid scales are "linear" or "logarithmic"
 *
 * Results:
 *      A standard TCL result.  If successful the field in the structure
 *      is updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColorScale(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
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
        Tcl_AppendResult(interp, "unknown color scale \"", string, "\"",
                         ": should be linear or logarithmic.",
                         (char *)NULL);
        return TCL_ERROR;
    }
    *flagsPtr &= ~COLOR_SCALE_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorScaleToObj --
 *
 *      Convert the color scale flag into a string Tcl_Obj.
 *
 * Results:
 *      The string representation of the color scale flag is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ColorScaleToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                  char *widgRec, int offset, int flags) 
{
    unsigned int coreFlags = *(unsigned int *)(widgRec + offset);
    Tcl_Obj *objPtr;
    
    switch (coreFlags & COLOR_SCALE_MASK) {
    case BLT_PAINTBRUSH_SCALING_LINEAR:
        objPtr = Tcl_NewStringObj("linear", 6);         break;
    case BLT_PAINTBRUSH_SCALING_LOG:
        objPtr = Tcl_NewStringObj("logarithmic", 11);   break;
    default:
        objPtr = Tcl_NewStringObj("???", 3);            break;
    }
    return objPtr;
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

/*ARGSUSED*/
static void
FreeReference(ClientData clientData, Display *display, char *widgRec,
              int offset)
{
    BackgroundObject *corePtr = (BackgroundObject *)(widgRec);

    if (corePtr->refNameObjPtr != NULL) {
        Tcl_DecrRefCount(corePtr->refNameObjPtr);
        corePtr->refNameObjPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToReference --
 *
 *      Converts the given Tcl_Obj to a reference type.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToReference(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation of value. */
    char *widgRec,                      /* Widget record. */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    BackgroundObject *corePtr = (BackgroundObject *)(widgRec);
    const char *string;
    char c;
    int flag;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 's') && (strncmp(string, "self", length) == 0)) {
        flag = RELATIVETO_SELF;
    } else if ((c == 't') && (strncmp(string, "toplevel", length) == 0)) {
        flag = RELATIVETO_TOPLEVEL;
    } else if (c == '.') {
        if ((corePtr->flags & REFERENCE_PENDING) == 0) {
            Tcl_DoWhenIdle(SetReferenceWindowFromPath, corePtr);
            corePtr->flags |= REFERENCE_PENDING;
        }           
        flag = RELATIVETO_WINDOW;
    } else {
        Tcl_AppendResult(interp, "unknown reference type \"", string, "\"",
                         (char *)NULL);
        return TCL_ERROR;
    }
    corePtr->flags &= ~RELATIVETO_MASK;
    corePtr->flags |= flag;
    corePtr->refNameObjPtr = objPtr;
    Tcl_IncrRefCount(corePtr->refNameObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReferenceToObj --
 *
 *      Returns the reference window name string Tcl_Obj.
 *
 * Results:
 *      The string representation of the reference window is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ReferenceToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
               char *widgRec, int offset, int flags)    
{
    BackgroundObject *corePtr = (BackgroundObject *)(widgRec);
    Tcl_Obj *objPtr;
    
    switch (corePtr->flags & RELATIVETO_MASK) {
    case RELATIVETO_SELF:
        objPtr = Tcl_NewStringObj("self", 4);           break;
    case RELATIVETO_TOPLEVEL:
        objPtr = Tcl_NewStringObj("toplevel", 8);       break;
    default:
        if (corePtr->flags & REFERENCE_PENDING) {
            SetReferenceWindowFromPath(corePtr);
        }           
        if (corePtr->refNameObjPtr == NULL) {
            objPtr = Tcl_NewStringObj("", -1);
        } else {
            objPtr = corePtr->refNameObjPtr;
        }
    }
    return objPtr;
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
NotifyClients(BackgroundObject *corePtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(corePtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Bg *bgPtr;

        /* Notify each client that the background has changed. The
         * client should schedule itself for redrawing.  */
        bgPtr = Blt_Chain_GetValue(link);
        if (bgPtr->notifyProc != NULL) {
            (*bgPtr->notifyProc)(bgPtr->clientData);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOrigin --
 *
 *      Computes the x and y offsets for the coordinates using a background
 *      pattern referenced from a particular window.  The x and y
 *      coordinates start as offsets from the most local window.  We add
 *      the offsets of each successive parent window until we reach to
 *      reference window.
 *
 *              +-----------------------+
 *              |                       |
 *              |       +---------+     |
 *              |       |  x,y    |     |
 *              |       |         |     |
 *              |       +---------+     |
 *              +-----------------------+
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void 
GetOffsets(Tk_Window tkwin, BackgroundObject *corePtr, int x, int y, 
           int *xOffsetPtr, int *yOffsetPtr)
{
    Tk_Window tkRef;

    tkRef = GetReferenceWindow(corePtr, tkwin);
    if (tkRef == NULL) {
        *xOffsetPtr = *yOffsetPtr = 0;
        return;                         
    }
    if (corePtr->flags & (RELATIVETO_WINDOW|RELATIVETO_TOPLEVEL)) {
        Tk_Window tkwin2;
        
        tkwin2 = tkwin;
        while ((tkwin2 != tkRef) && (tkwin2 != NULL)) {
            x += Tk_X(tkwin2) + Tk_Changes(tkwin2)->border_width;
            y += Tk_Y(tkwin2) + Tk_Changes(tkwin2)->border_width;
            tkwin2 = Tk_Parent(tkwin2);
        }
        if (tkwin2 == NULL) {
            /* 
             * The window associated with the background isn't an ancestor
             * of the current window. That means we can't use the reference
             * window as a guide to the size of the picture.  Simply
             * convert to a self reference.
             */
            fprintf(stderr, "reference type is %x, refwin=%s tkwin=%s\n",
                    corePtr->flags & RELATIVETO_MASK, Tk_PathName(tkRef),
                    Tk_PathName(tkwin));
                    
#ifdef notdef
            corePtr->flags = RELATIVETO_SELF;
            tkRef = tkwin;
#endif
            abort();
        }
    }
    *xOffsetPtr = -x;
    *yOffsetPtr = -y;
}


static void
GetPolygonBBox(XPoint *points, int n, int *leftPtr, int *rightPtr, int *topPtr, 
               int *bottomPtr)
{
    XPoint *p, *pend;
    int left, right, bottom, top;

    /* Determine the bounding box of the polygon. */
    left = right = points[0].x;
    top = bottom = points[0].y;
    for (p = points, pend = p + n; p < pend; p++) {
        if (p->x < left) {
            left = p->x;
        } 
        if (p->x > right) {
            right = p->x;
        }
        if (p->y < top) {
            top = p->y;
        } 
        if (p->y > bottom) {
            bottom = p->y;
        }
    }
    if (leftPtr != NULL) {
        *leftPtr = left;
    }
    if (rightPtr != NULL) {
        *rightPtr = right;
    }
    if (topPtr != NULL) {
        *topPtr = top;
    }
    if (bottomPtr != NULL) {
        *bottomPtr = bottom;
    }
}

/* 
 * The following routines are directly from tk3d.c.  
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 *   This software is copyrighted by the Regents of the University of
 *   California, Sun Microsystems, Inc., and other parties.  The following
 *   terms apply to all files associated with the software unless
 *   explicitly disclaimed in individual files.
 * 
 *   The authors hereby grant permission to use, copy, modify, distribute,
 *   and license this software and its documentation for any purpose,
 *   provided that existing copyright notices are retained in all copies
 *   and that this notice is included verbatim in any distributions. No
 *   written agreement, license, or royalty fee is required for any of the
 *   authorized uses.  Modifications to this software may be copyrighted by
 *   their authors and need not follow the licensing terms described here,
 *   provided that the new terms are clearly indicated on the first page of
 *   each file where they apply.
 * 
 *   IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY
 *   FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 *   ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY
 *   DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 * 
 *   THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 *   NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, AND
 *   THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 *   MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *   GOVERNMENT USE: If you are acquiring this software on behalf of the
 *   U.S. government, the Government shall have only "Restricted Rights" in
 *   the software and related documentation as defined in the Federal
 *   Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 *   are acquiring the software on behalf of the Department of Defense, the
 *   software shall be classified as "Commercial Computer Software" and the
 *   Government shall have only "Restricted Rights" as defined in Clause
 *   252.227-7013 (b) (3) of DFARs.  Notwithstanding the foregoing, the
 *   authors grant the U.S. Government and others acting in its behalf
 *   permission to use and distribute the software in accordance with the
 *   terms specified in this license.
 *
 *  They fix a problem in the Intersect procedure when the polygon is big
 *  (e.q 1600x1200).  The computation overflows the 32-bit integers used.
 */

/*
 *---------------------------------------------------------------------------
 *
 * ShiftLine --
 *
 *      Given two points on a line, compute a point on a new line that is
 *      parallel to the given line and a given distance away from it.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
ShiftLine(
    XPoint *p,                          /* First point on line. */
    XPoint *q,                          /* Second point on line. */
    int distance,                       /* New line is to be this many
                                         * units to the left of original
                                         * line, when looking from p1 to
                                         * p2.  May be negative. */
    XPoint *r)                          /* Store coords of point on new
                                         * line here. */
{
    int dx, dy, dxNeg, dyNeg;

    /*
     * The table below is used for a quick approximation in computing the
     * new point.  An index into the table is 128 times the slope of the
     * original line (the slope must always be between 0 and 1).  The value
     * of the table entry is 128 times the amount to displace the new line
     * in y for each unit of perpendicular distance.  In other words, the
     * table maps from the tangent of an angle to the inverse of its
     * cosine.  If the slope of the original line is greater than 1, then
     * the displacement is done in x rather than in y.
     */
    static int shiftTable[129];

    /*
     * Initialize the table if this is the first time it is
     * used.
     */

    if (shiftTable[0] == 0) {
        int i;
        double tangent, cosine;

        for (i = 0; i <= 128; i++) {
            tangent = i/128.0;
            cosine = 128/cos(atan(tangent)) + .5;
            shiftTable[i] = (int) cosine;
        }
    }

    *r = *p;
    dx = q->x - p->x;
    dy = q->y - p->y;
    if (dy < 0) {
        dyNeg = 1;
        dy = -dy;
    } else {
        dyNeg = 0;
    }
    if (dx < 0) {
        dxNeg = 1;
        dx = -dx;
    } else {
        dxNeg = 0;
    }
    if (dy <= dx) {
        dy = ((distance * shiftTable[(dy<<7)/dx]) + 64) >> 7;
        if (!dxNeg) {
            dy = -dy;
        }
        r->y += dy;
    } else {
        dx = ((distance * shiftTable[(dx<<7)/dy]) + 64) >> 7;
        if (dyNeg) {
            dx = -dx;
        }
        r->x += dx;
    }
}

/*
 *----------------------------------------------------------------------------
 *
 * Intersect --
 *
 *      Find the intersection point between two lines.
 *
 * Results:
 *      Under normal conditions 0 is returned and the point at *iPtr is
 *      filled in with the intersection between the two lines.  If the two
 *      lines are parallel, then -1 is returned and *iPtr isn't modified.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------------
 */
static int
Intersect(a1Ptr, a2Ptr, b1Ptr, b2Ptr, iPtr)
    XPoint *a1Ptr;              /* First point of first line. */
    XPoint *a2Ptr;              /* Second point of first line. */
    XPoint *b1Ptr;              /* First point of second line. */
    XPoint *b2Ptr;              /* Second point of second line. */
    XPoint *iPtr;               /* Filled in with intersection point. */
{
    float dxadyb, dxbdya, dxadxb, dyadyb, p, q;

    /*
     * The code below is just a straightforward manipulation of two
     * equations of the form y = (x-x1)*(y2-y1)/(x2-x1) + y1 to solve for
     * the x-coordinate of intersection, then the y-coordinate.
     */

    dxadyb = (a2Ptr->x - a1Ptr->x)*(b2Ptr->y - b1Ptr->y);
    dxbdya = (b2Ptr->x - b1Ptr->x)*(a2Ptr->y - a1Ptr->y);
    dxadxb = (a2Ptr->x - a1Ptr->x)*(b2Ptr->x - b1Ptr->x);
    dyadyb = (a2Ptr->y - a1Ptr->y)*(b2Ptr->y - b1Ptr->y);

    if (dxadyb == dxbdya) {
        return -1;
    }
    p = (a1Ptr->x*dxbdya - b1Ptr->x*dxadyb + (b1Ptr->y - a1Ptr->y)*dxadxb);
    q = dxbdya - dxadyb;
    if (q < 0) {
        p = -p;
        q = -q;
    }
    if (p < 0) {
        iPtr->x = - ((-p + q/2)/q);
    } else {
        iPtr->x = (p + q/2)/q;
    }
    p = (a1Ptr->y*dxadyb - b1Ptr->y*dxbdya + (b1Ptr->x - a1Ptr->x)*dyadyb);
    q = dxadyb - dxbdya;
    if (q < 0) {
        p = -p;
        q = -q;
    }
    if (p < 0) {
        iPtr->y = (int)(- ((-p + q/2)/q));
    } else {
        iPtr->y = (int)((p + q/2)/q);
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * Draw3DPolygon --
 *
 *      Draw a border with 3-D appearance around the edge of a given
 *      polygon.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Information is drawn in "drawable" in the form of a 3-D border
 *      borderWidth units width wide on the left of the trajectory given by
 *      pointPtr and n (or -borderWidth units wide on the right side, if
 *      borderWidth is negative).
 *
 *--------------------------------------------------------------
 */

static void
Draw3DPolygon(
    Tk_Window tkwin,                    /* Window for which border was
                                           allocated. */
    Drawable drawable,                  /* X window or pixmap in which to
                                         * draw. */
    Tk_3DBorder border,                 /* Token for border to draw. */
    XPoint *points,                     /* Array of points describing
                                         * polygon.  All points must be
                                         * absolute (CoordModeOrigin). */
    int n,                              /* Number of points at *points. */
    int borderWidth,                    /* Width of border, measured in
                                         * pixels to the left of the
                                         * polygon's trajectory.  May be
                                         * negative. */
    int leftRelief)                     /* TK_RELIEF_RAISED or
                                         * TK_RELIEF_SUNKEN: indicates how
                                         * stuff to left of trajectory
                                         * looks relative to stuff on
                                         * right. */
{
    XPoint poly[4], b1, b2, newB1, newB2;
    XPoint perp, c, shift1, shift2;     /* Used for handling parallel lines. */
    XPoint *p, *q;
    GC gc;
    int i, lightOnLeft, dx, dy, parallel, pointsSeen;

    /* Handle grooves and ridges with recursive calls. */
    if ((leftRelief == TK_RELIEF_GROOVE) || (leftRelief == TK_RELIEF_RIDGE)) {
        int halfWidth, relief;

        halfWidth = borderWidth / 2;
        relief = (leftRelief == TK_RELIEF_GROOVE) 
            ? TK_RELIEF_RAISED : TK_RELIEF_SUNKEN;
        Draw3DPolygon(tkwin, drawable, border, points, n, halfWidth, relief);
        Draw3DPolygon(tkwin, drawable, border, points, n, -halfWidth, relief);
        return;
    }
    /*
     * If the polygon is already closed, drop the last point from it
     * (we'll close it automatically).
     */
    p = points + (n-1);
    q = points;
    if ((p->x == q->x) && (p->y == q->y)) {
        n--;
    }

    /*
     * The loop below is executed once for each vertex in the polgon.
     * At the beginning of each iteration things look like this:
     *
     *          poly[1]       /
     *             *        /
     *             |      /
     *             b1   * poly[0] (points[i-1])
     *             |    |
     *             |    |
     *             |    |
     *             |    |
     *             |    |
     *             |    | *p            *q
     *             b2   *--------------------*
     *             |
     *             |
     *             x-------------------------
     *
     * The job of this iteration is to do the following:
     * (a) Compute x (the border corner corresponding to
     *     points[i]) and put it in poly[2].  As part of
     *     this, compute a new b1 and b2 value for the next
     *     side of the polygon.
     * (b) Put points[i] into poly[3].
     * (c) Draw the polygon given by poly[0..3].
     * (d) Advance poly[0], poly[1], b1, and b2 for the
     *     next side of the polygon.
     */

    /*
     * The above situation doesn't first come into existence until two
     * points have been processed; the first two points are used to "prime
     * the pump", so some parts of the processing are ommitted for these
     * points.  The variable "pointsSeen" keeps track of the priming
     * process; it has to be separate from i in order to be able to ignore
     * duplicate points in the polygon.
     */
    pointsSeen = 0;
    for (i = -2, p = points + (n-2), q = p+1; i < n; i++, p = q, q++) {
        if ((i == -1) || (i == n-1)) {
            q = points;
        }
        if ((q->x == p->x) && (q->y == p->y)) {
            /*
             * Ignore duplicate points (they'd cause core dumps in
             * ShiftLine calls below).
             */
            continue;
        }
        ShiftLine(p, q, borderWidth, &newB1);
        newB2.x = newB1.x + (q->x - p->x);
        newB2.y = newB1.y + (q->y - p->y);
        poly[3] = *p;
        parallel = 0;
        if (pointsSeen >= 1) {
            parallel = Intersect(&newB1, &newB2, &b1, &b2, &poly[2]);

            /*
             * If two consecutive segments of the polygon are parallel,
             * then things get more complex.  Consider the following
             * diagram:
             *
             * poly[1]
             *    *----b1-----------b2------a
             *                                \
             *                                  \
             *         *---------*----------*    b
             *        poly[0]  *q   *p  /
             *                                /
             *              --*--------*----c
             *              newB1    newB2
             *
             * Instead of using x and *p for poly[2] and poly[3], as in the
             * original diagram, use a and b as above.  Then instead of
             * using x and *p for the new poly[0] and poly[1], use b and c
             * as above.
             *
             * Do the computation in three stages:
             * 1. Compute a point "perp" such that the line p-perp
             *    is perpendicular to p-q.
             * 2. Compute the points a and c by intersecting the lines
             *    b1-b2 and newB1-newB2 with p-perp.
             * 3. Compute b by shifting p-perp to the right and
             *    intersecting it with p-q.
             */

            if (parallel) {
                perp.x = p->x + (q->y - p->y);
                perp.y = p->y - (q->x - p->x);
                Intersect(p, &perp, &b1, &b2, &poly[2]);
                Intersect(p, &perp, &newB1, &newB2, &c);
                ShiftLine(p, &perp, borderWidth, &shift1);
                shift2.x = shift1.x + (perp.x - p->x);
                shift2.y = shift1.y + (perp.y - p->y);
                Intersect(p, q, &shift1, &shift2, &poly[3]);
            }
        }
        if (pointsSeen >= 2) {
            dx = poly[3].x - poly[0].x;
            dy = poly[3].y - poly[0].y;
            if (dx > 0) {
                lightOnLeft = (dy <= dx);
            } else {
                lightOnLeft = (dy < dx);
            }
            if (lightOnLeft ^ (leftRelief == TK_RELIEF_RAISED)) {
                gc = Tk_3DBorderGC(tkwin, border, TK_3D_LIGHT_GC);
            } else {
                gc = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
            }   
            XFillPolygon(Tk_Display(tkwin), drawable, gc, poly, 4, Convex,
                         CoordModeOrigin);
        }
        b1.x = newB1.x;
        b1.y = newB1.y;
        b2.x = newB2.x;
        b2.y = newB2.y;
        poly[0].x = poly[3].x;
        poly[0].y = poly[3].y;
        if (parallel) {
            poly[1].x = c.x;
            poly[1].y = c.y;
        } else if (pointsSeen >= 1) {
            poly[1].x = poly[2].x;
            poly[1].y = poly[2].y;
        }
        pointsSeen++;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * IsBackgroundOption --
 *
 *      Checks if the given option is a background option.  We need to do
 *      this because we use separate option specs for the brush and the
 *      background.  
 *
 * Results:
 *      Return 1 is the option is a background option, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
static int
IsBackgroundOption(Tcl_Obj *objPtr)
{
    Blt_ConfigSpec *sp;
    
    for (sp = bgSpecs; sp->type != BLT_CONFIG_END; sp++) {
        const char *string;

        string = Tcl_GetString(objPtr);
        if (strcmp(string, sp->switchName) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureBackground --
 *
 *      Configures the background object depending upon the options and
 *      values given int the objv vector.  We first segregate the
 *      background options and the brush options into separate vectors
 *      because we have to make separate calls to configure each set.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureBackground(Tcl_Interp *interp, BackgroundObject *corePtr, int objc,
                    Tcl_Obj *const *objv, int flags)
{
    Tcl_Obj **bgArgs, **brushArgs;
    int numBgArgs, numBrushArgs;
    int i, result;
    
    bgArgs = Blt_AssertMalloc(sizeof(Tcl_Obj *) * objc);
    brushArgs = Blt_AssertMalloc(sizeof(Tcl_Obj *) * objc);
    numBgArgs = numBrushArgs = 0;
    for (i = 0; i < objc; i += 2) {
        if (IsBackgroundOption(objv[i])) {
            bgArgs[numBgArgs] = objv[i];
            numBgArgs++;
            if ((i + 1) < objc) {
                bgArgs[numBgArgs] = objv[i+1];
                numBgArgs++;
            }
        } else {
            brushArgs[numBrushArgs] = objv[i];
            numBrushArgs++;
            if ((i + 1) < objc) {
                brushArgs[numBrushArgs] = objv[i+1];
                numBrushArgs++;
            }
        }
    }
    imageOption.clientData = corePtr;
    result = Blt_ConfigureWidgetFromObj(interp, corePtr->tkwin, bgSpecs,
        numBgArgs, bgArgs, (char *)corePtr, flags);
    if (result == TCL_OK) {
        result = Blt_ConfigureWidgetFromObj(interp, corePtr->tkwin,
                corePtr->specs, numBrushArgs, brushArgs,
                (char *)corePtr->brush, flags);
    }
    Blt_Free(bgArgs);
    Blt_Free(brushArgs);
    if (result == TCL_OK) {
        result = Blt_ConfigurePaintBrush(interp, corePtr->brush);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetInfo --
 *
 *      Configures the background object depending upon the options and
 *      values given int the objv vector.  We first segregate the
 *      background options and the brush options into separate vectors
 *      because we have to make separate calls to configure each set.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
GetInfo(Tcl_Interp *interp, BackgroundObject *corePtr, Tcl_Obj *objPtr)
{
    if (IsBackgroundOption(objPtr)) {
        return Blt_ConfigureInfoFromObj(interp, corePtr->tkwin, bgSpecs,
                (char *)corePtr, objPtr, 0);
    } else if (corePtr->brush != NULL) {
        return Blt_ConfigureInfoFromObj(interp, corePtr->tkwin,
                corePtr->specs, (char *)corePtr->brush, objPtr, 0);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetValue --
 *
 *      Configures the background object depending upon the options and
 *      values given int the objv vector.  We first segregate the
 *      background options and the brush options into separate vectors
 *      because we have to make separate calls to configure each set.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
GetValue(Tcl_Interp *interp, BackgroundObject *corePtr, Tcl_Obj *objPtr)
{
    if (IsBackgroundOption(objPtr)) {
        return Blt_ConfigureValueFromObj(interp, corePtr->tkwin, bgSpecs,
                (char *)corePtr, objPtr, 0);
    } else if (corePtr->brush != NULL) {
        return Blt_ConfigureValueFromObj(interp, corePtr->tkwin,
                corePtr->specs, (char *)corePtr->brush, objPtr, 0);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOptionLists --
 *
 *      Configures the background object depending upon the options and
 *      values given int the objv vector.  We first segregate the
 *      background options and the brush options into separate vectors
 *      because we have to make separate calls to configure each set.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
GetOptionLists(Tcl_Interp *interp, BackgroundObject *corePtr)
{
    Tcl_Obj *objPtr, *listObjPtr;
    Tcl_Obj **objv;
    int i, objc;
    
    if (Blt_ConfigureInfoFromObj(interp, corePtr->tkwin, bgSpecs,
                (char *)corePtr, (Tcl_Obj *)NULL, 0) != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_GetObjResult(interp);
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
        Tcl_ListObjAppendElement(interp, listObjPtr, objv[i]);
    }
    Tcl_ResetResult(interp);
    if (corePtr->brush != NULL) {
        if (Blt_ConfigureInfoFromObj(interp, corePtr->tkwin, corePtr->specs,
                (char *)corePtr->brush, (Tcl_Obj *)NULL, 0) != TCL_OK) {
            return TCL_ERROR;
        }
        objPtr = Tcl_GetObjResult(interp);
        if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
            return TCL_ERROR;
        }
        for (i = 0; i < objc; i++) {
            Tcl_ListObjAppendElement(interp, listObjPtr, objv[i]);
        }
        Tcl_ResetResult(interp);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

static BgInstance *
GetBgInstance(Tk_Window tkwin, int w, int h, BackgroundObject *corePtr)
{
    Blt_HashEntry *hPtr;
    Blt_Painter painter;
    Blt_Picture picture;
    GC newGC;
    BgInstance *instPtr;
    Tk_Window tkRef;
    XGCValues gcValues;
    int isNew;
    unsigned int gcMask;
    
    tkRef = GetReferenceWindow(corePtr, tkwin);
    hPtr = Blt_CreateHashEntry(&corePtr->instTable, (char *)tkRef, &isNew);
    if (!isNew) {
        return Tcl_GetHashValue(hPtr);
    }
    picture = Blt_CreatePicture(w, h);
    if (picture == NULL) {
        return NULL;                    /* Can't allocate picture */
    }
    instPtr = Blt_AssertCalloc(1, sizeof(BgInstance));
    instPtr->corePtr = corePtr;
    instPtr->width = w;
    instPtr->height = h;
    instPtr->tkwin = tkRef;
    instPtr->display = corePtr->display;
    instPtr->hashPtr = hPtr;
    Tk_CreateEventHandler(instPtr->tkwin, StructureNotifyMask,
              InstanceEventProc, instPtr);
    Blt_SetBrushRegion(corePtr->brush, 0, 0, w, h);
    Blt_PaintRectangle(picture, 0, 0, w, h, 0, 0, corePtr->brush, TRUE);

    /* Create a pixmap the size of the reference window. */
    instPtr->pixmap = Blt_GetPixmap(corePtr->display, Tk_WindowId(tkRef),
        w, h, Tk_Depth(tkRef));

    painter = Blt_GetPainter(tkwin, 1.0);
    Blt_PaintPicture(painter, instPtr->pixmap, picture, 0, 0, w, h, 0, 0, 0);
    Blt_FreePicture(picture);

    gcMask = (GCTile | GCFillStyle);
    gcValues.fill_style = FillTiled;
    gcValues.tile = instPtr->pixmap;
    newGC = Blt_GetPrivateGC(tkRef, gcMask, &gcValues);
    if (instPtr->gc != NULL) {
        Blt_FreePrivateGC(corePtr->display, instPtr->gc);
    }
    instPtr->gc = newGC;
    Blt_SetHashValue(hPtr, instPtr);
    return instPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawBackgroundRectangle --
 *
 *      Draws the background as a rectangle defined by x, y, w, and h.
 *      First paints a image using the background's brush and then draws it
 *      to the given drawable.  If the brush isn't opaque, we first get a
 *      snapshot of the current screen background.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawBackgroundRectangle(Tk_Window tkwin, Drawable drawable, Bg *bgPtr,
                        int x, int y, int w, int h)
{
    BackgroundObject *corePtr = bgPtr->corePtr;
    int rw, rh;

    if ((h <= 0) || (w <= 0)) {
        return;
    }
    /* Handle the simple case where it's a solid color background. */
    if (corePtr->flags & BACKGROUND_SOLID) {
        Tk_Fill3DRectangle(tkwin, drawable, corePtr->border, x, y, w, h, 0,
                           TK_RELIEF_FLAT);
        return;
    }
    GetReferenceWindowDimensions(corePtr, tkwin, &rw, &rh);
    if ((rw > 0) && (rh > 0)) {
        BgInstance *instPtr;
        int xOffset, yOffset;           /* Starting upper left corner of
                                         * region. */
        GetOffsets(tkwin, corePtr, 0, 0, &xOffset, &yOffset);
        instPtr = GetBgInstance(tkwin, rw, rh, corePtr);
        if (instPtr == NULL) {
            return;
        }
        XSetTSOrigin(corePtr->display, instPtr->gc, xOffset, yOffset);
	XFillRectangle(corePtr->display, drawable, instPtr->gc, x, y, w, h);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawBackgroundPolygon --
 *
 *      Draws the background as a polygon defined by the given vertices.
 *      First paints a image using the background's brush and then draws it
 *      to the given drawable.  If the brush isn't opaque, we first get a
 *      snapshot of the current screen background.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawBackgroundPolygon(Tk_Window tkwin, Drawable drawable, Bg *bgPtr,
                      int numPoints, XPoint *points)
{
    BackgroundObject *corePtr = bgPtr->corePtr;
    int rw, rh;
    
    /* Handle the simple case where it's a solid color background. */
    if (corePtr->flags & BACKGROUND_SOLID) {
        Tk_Fill3DPolygon(tkwin, drawable, corePtr->border, points, numPoints,
                         0, TK_RELIEF_FLAT);
        return;
    }
    GetReferenceWindowDimensions(corePtr, tkwin, &rw, &rh);
    if ((rw > 0) && (rh > 0)) {
        BgInstance *instPtr;
        int x1, x2, y1, y2;
        int xOffset, yOffset;           /* Starting upper left corner of
                                         * region. */

        /* Grab the rectangular background that contains the polygon. */
        GetPolygonBBox(points, numPoints, &x1, &x2, &y1, &y2);
        GetOffsets(tkwin, corePtr, x1, y1, &xOffset, &yOffset);
        instPtr = GetBgInstance(tkwin, rw, rh, corePtr);
        if (instPtr == NULL) {
            return;
        }
        XSetTSOrigin(corePtr->display, instPtr->gc, xOffset, yOffset);
        XFillPolygon(corePtr->display, drawable, instPtr->gc, points,
                     numPoints, Complex, CoordModeOrigin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyBackgroundObject --
 *
 *      Removes the client from the servers's list of clients and memory
 *      used by the client token is released.  When the last client is
 *      deleted, the server is also removed.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyBackgroundObject(BackgroundObject *corePtr)
{
    if (corePtr->brush != NULL) {
        if (corePtr->specs != NULL) {
            Blt_FreeOptions(corePtr->specs, (char *)corePtr->brush,
                        corePtr->display, 0);
        }
        Blt_FreeBrush(corePtr->brush);
    }
    Blt_FreeOptions(bgSpecs, (char *)corePtr, corePtr->display, 0);
    if (corePtr->border != NULL) {
        Tk_Free3DBorder(corePtr->border);
    }
    if (corePtr->flags & REFERENCE_PENDING) {
        Tcl_CancelIdleCall(SetReferenceWindowFromPath, corePtr);
    }
    if (corePtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&corePtr->dataPtr->instTable, corePtr->hashPtr);
    }
    ClearInstances(corePtr);
    Blt_Chain_Destroy(corePtr->chain);
    Blt_Free(corePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewBackgroundObject --
 *
 *      Creates a new background.
 *
 * Results:
 *      Returns pointer to the new background.
 *
 *---------------------------------------------------------------------------
 */
static BackgroundObject *
NewBackgroundObject(BackgroundInterpData *dataPtr, Tcl_Interp *interp,
                    Blt_PaintBrushType type, Tk_3DBorder border)
{
    BackgroundObject *corePtr;

    corePtr = Blt_AssertCalloc(1, sizeof(BackgroundObject));
    corePtr->flags = RELATIVETO_TOPLEVEL;
    corePtr->chain = Blt_Chain_Create();
    corePtr->tkwin = Tk_MainWindow(interp);
    corePtr->display = Tk_Display(corePtr->tkwin);
    corePtr->dataPtr = dataPtr;
    corePtr->border = border;
    Blt_InitHashTable(&corePtr->instTable, sizeof(BgInstance)/sizeof(int));
    switch (type) {
    case BLT_PAINTBRUSH_TILE:
        corePtr->brush = Blt_NewTileBrush();
        corePtr->specs = tileBrushSpecs;
        break;
    case BLT_PAINTBRUSH_LINEAR:
        corePtr->brush = Blt_NewLinearGradientBrush();
        corePtr->specs = linearGradientBrushSpecs;
        break;
    case BLT_PAINTBRUSH_STRIPES:
        corePtr->brush = Blt_NewStripesBrush();
        corePtr->specs = stripesBrushSpecs;
        break;
    case BLT_PAINTBRUSH_CHECKERS:
        corePtr->brush = Blt_NewCheckersBrush();
        corePtr->specs = checkersBrushSpecs;
        break;
    case BLT_PAINTBRUSH_RADIAL:
        corePtr->brush = Blt_NewRadialGradientBrush();
        corePtr->specs = radialGradientBrushSpecs;
        break;
    case BLT_PAINTBRUSH_CONICAL:
        corePtr->brush = Blt_NewConicalGradientBrush();
        corePtr->specs = conicalGradientBrushSpecs;
        break;
    case BLT_PAINTBRUSH_COLOR:
        { 
            Blt_Pixel color;
            
            color.u32 = GetBackgroundColor(corePtr);
            corePtr->brush = Blt_NewColorBrush(color.u32);
            corePtr->flags |= BACKGROUND_SOLID;
        }
        break;                          
    default:
        abort();
        break;
    }
    return corePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyBackground --
 *
 *      Removes the client from the servers's list of clients and memory
 *      used by the client token is released.  When the last client is
 *      deleted, the server is also removed.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyBackground(Bg *bgPtr)
{
    BackgroundObject *corePtr = bgPtr->corePtr;

    Blt_Chain_DeleteLink(corePtr->chain, bgPtr->link);
    if (Blt_Chain_GetLength(corePtr->chain) <= 0) {
        DestroyBackgroundObject(corePtr);
    }
    Blt_Free(bgPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBackgroundFromObj --
 *
 *      Retrieves the background named by the given the Tcl_Obj.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
GetBackgroundFromObj(Tcl_Interp *interp, BackgroundInterpData *dataPtr,
                     Tcl_Obj *objPtr, BackgroundObject **corePtrPtr)
{
    Blt_HashEntry *hPtr;
    const char *string;

    string = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&dataPtr->instTable, string);
    if (hPtr == NULL) {
        Tcl_AppendResult(dataPtr->interp, "can't find background \"", 
                string, "\"", (char *)NULL);
        return TCL_ERROR;
    }
    *corePtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 * background create type ?name? ?option values?...
 */
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    BackgroundInterpData *dataPtr = clientData;
    BackgroundObject *corePtr;
    Bg *bgPtr;
    Blt_HashEntry *hPtr;
    Blt_PaintBrushType type;
                           
    if (GetBackgroundTypeFromObj(interp, objv[2], &type) != TCL_OK) {
        return TCL_ERROR;
    }
    hPtr = NULL;
    if (objc > 3) {
        const char *string;

        string = Tcl_GetString(objv[3]);
        if (string[0] != '-') {         
            int isNew;

            hPtr = Blt_CreateHashEntry(&dataPtr->instTable, string, &isNew);
            if (!isNew) {
                Tcl_AppendResult(interp, "a background named \"", string, 
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
            Blt_FormatString(name, 200, "background%d", dataPtr->nextId++);
            hPtr = Blt_CreateHashEntry(&dataPtr->instTable, name, &isNew);
        } while (!isNew);
    } 
    corePtr = NewBackgroundObject(dataPtr, interp, type, NULL);
    if (corePtr == NULL) {
        Blt_DeleteHashEntry(&dataPtr->instTable, hPtr);
        return TCL_ERROR;
    }
    Blt_SetHashValue(hPtr, corePtr);
    corePtr->hashPtr = hPtr;
    corePtr->name = Blt_GetHashKey(&dataPtr->instTable, hPtr);
    if (ConfigureBackground(interp, corePtr, objc - 3, objv + 3, 0) != TCL_OK) {
        DestroyBackgroundObject(corePtr);
        return TCL_ERROR;
    }

    /* Create the container for the background. */
    bgPtr = Blt_Calloc(1, sizeof(Bg));
    if (bgPtr == NULL) {
        Tcl_AppendResult(interp, "can't allocate background.", (char *)NULL);
        DestroyBackgroundObject(corePtr);
        return TCL_ERROR;
    }
    /* Add the container to the background object's list of clients. */
    bgPtr->link = Blt_Chain_Append(corePtr->chain, bgPtr);
    corePtr->link = bgPtr->link;
    bgPtr->corePtr = corePtr;
    Tcl_SetStringObj(Tcl_GetObjResult(interp), corePtr->name, -1);
    return TCL_OK;
}    

/*
 * background cget bgName option
 */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    BackgroundInterpData *dataPtr = clientData;
    BackgroundObject *corePtr;

    if (GetBackgroundFromObj(interp, dataPtr, objv[2], &corePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return GetValue(interp, corePtr, objv[3]);
}

/*
 * background configure bgName ?option value ...?
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    BackgroundInterpData *dataPtr = clientData;
    BackgroundObject *corePtr;
    int flags;

    if (GetBackgroundFromObj(interp, dataPtr, objv[2], &corePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    flags = BLT_CONFIG_OBJV_ONLY;
    if (objc == 3) {
        return GetOptionLists(interp, corePtr);
    } else if (objc == 4) {
        return GetInfo(interp, corePtr, objv[3]);
    } else {
        if (ConfigureBackground(interp, corePtr, objc - 3, objv + 3, flags)
            != TCL_OK) {
            return TCL_ERROR;
        }
        NotifyClients(corePtr);
        return TCL_OK;
    }
}

/*
 * background delete bgName ... 
 */
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    BackgroundInterpData *dataPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        Blt_HashEntry *hPtr;
        BackgroundObject *corePtr;
        const char *name;

        name = Tcl_GetString(objv[i]);
        hPtr = Blt_FindHashEntry(&dataPtr->instTable, name);
        if (hPtr == NULL) {
            Tcl_AppendResult(interp, "can't find background \"",
                             name, "\"", (char *)NULL);
            return TCL_ERROR;
        }
        corePtr = Blt_GetHashValue(hPtr);
        assert(corePtr->hashPtr == hPtr);

        /* FIXME: Assuming that the first background token is always
         * associated with the command. Need to known when background was
         * created by background command.  Does background delete #ffffff make
         * sense? */
        /* 
         * Look up clientData from command hash table. If it's found it
         * represents a command?
         */
        if (corePtr->link != NULL) {
            Bg *bgPtr;

            bgPtr = Blt_Chain_GetValue(corePtr->link);
            assert(corePtr->link == bgPtr->link);
            /* Take the background entry out of the hash table.  */
            Blt_DeleteHashEntry(&corePtr->dataPtr->instTable, 
                                corePtr->hashPtr);
            corePtr->name = NULL;
            corePtr->hashPtr = NULL;
            corePtr->link = NULL;       /* Disconnect background. */
            DestroyBackground(bgPtr);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistOp --
 *
 *      Indicates if the named background exists.
 *
 *      background exists bgName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    BackgroundInterpData *dataPtr = clientData;
    BackgroundObject *corePtr;
    int state;
    
    state = FALSE;
    if (GetBackgroundFromObj(NULL, dataPtr, objv[2], &corePtr) == TCL_OK) {
        state = TRUE;
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamesOp --
 *
 *      background names ?pattern ... ?
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    BackgroundInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&dataPtr->instTable, &iter);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        BackgroundObject *corePtr;
        int match;
        int i;
        
        corePtr = Blt_GetHashValue(hPtr);
        match = (objc == 2);
        for (i = 2; i < objc; i++) {
            if (Tcl_StringMatch(corePtr->name, Tcl_GetString(objv[i]))) {
                match = TRUE;
                break;
            }
        }
        if (match) {
            Tcl_Obj *objPtr;

            objPtr = Tcl_NewStringObj(corePtr->name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 * background type bgName
 */
static int
TypeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    BackgroundInterpData *dataPtr = clientData;
    BackgroundObject *corePtr;

    if (GetBackgroundFromObj(interp, dataPtr, objv[2], &corePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (corePtr->brush != NULL) {
        Tcl_SetStringObj(Tcl_GetObjResult(interp),
                     Blt_GetBrushType(corePtr->brush), -1);
    }
    return TCL_OK;
}

static Blt_OpSpec backgroundOps[] =
{
    {"cget",      2, CgetOp,      4, 4, "bgName option",},
    {"configure", 2, ConfigureOp, 3, 0, "bgName ?option value ...?",},
    {"create",    2, CreateOp,    3, 0, "type ?bgName? ?args ...?",},
    {"delete",    1, DeleteOp,    2, 0, "?bgName ...?",},
    {"exists",    1, ExistsOp,    3, 3, "bgName",},
    {"names",     1, NamesOp,     2, 3, "?pattern?",},
    {"type",      1, TypeOp,      3, 3, "bgName",},
};
static int numBackgroundOps = sizeof(backgroundOps) / sizeof(Blt_OpSpec);

static int
BackgroundCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
                 Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numBackgroundOps, backgroundOps, 
                BLT_OP_ARG1, objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/* FIXME: Should clean up backgrounds on interpreter deletion not command
 * deletion.  Could be widgets still with a reference to the background.  */
static void
BackgroundDeleteCmdProc(ClientData clientData) 
{
    BackgroundInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&dataPtr->instTable, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        BackgroundObject *corePtr;
        Blt_ChainLink link, next;

        corePtr = Blt_GetHashValue(hPtr);
        corePtr->hashPtr = NULL;
        for (link = Blt_Chain_FirstLink(corePtr->chain); link != NULL; 
             link = next) {
            Bg *bgPtr;

            next = Blt_Chain_NextLink(link);
            bgPtr = Blt_Chain_GetValue(link);
            DestroyBackground(bgPtr);
        }
    }
    Blt_DeleteHashTable(&dataPtr->instTable);
    Tcl_DeleteAssocData(dataPtr->interp, BG_BACKGROUND_THREAD_KEY);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBackgroundInterpData --
 *
 *---------------------------------------------------------------------------
 */
static BackgroundInterpData *
GetBackgroundInterpData(Tcl_Interp *interp)
{
    BackgroundInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (BackgroundInterpData *)
        Tcl_GetAssocData(interp, BG_BACKGROUND_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
        dataPtr = Blt_AssertMalloc(sizeof(BackgroundInterpData));
        dataPtr->interp = interp;
        dataPtr->nextId = 1;


        /* FIXME: Create interp delete proc to teardown the hash table and
         * data entry.  Must occur after all the widgets have been destroyed
         * (clients of the background). See above FIXME: */

        Tcl_SetAssocData(interp, BG_BACKGROUND_THREAD_KEY, 
                (Tcl_InterpDeleteProc *)NULL, dataPtr);
        Blt_InitHashTable(&dataPtr->instTable, BLT_STRING_KEYS);
    }
    return dataPtr;
}


/*LINTLIBRARY*/
int
Blt_BackgroundCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {
        "background", BackgroundCmdProc, BackgroundDeleteCmdProc,
    };
    cmdSpec.clientData = GetBackgroundInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetBg
 *
 *      Creates a new background from the given description.  The
 *      background structure returned is a token for the client to use the
 *      background.  If the background isn't a solid background (i.e. a
 *      solid color that Tk_Get3DBorder will accept) then the background
 *      must already exist.  Solid backgrounds are the exception to this
 *      rule.  This lets "-background #ffffff" work without already having
 *      allocated a background "#ffffff".
 *
 * Results:
 *      Returns a background token.
 *
 * Side Effects:
 *      Memory is allocated for the new token.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetBg(Tcl_Interp *interp, Tk_Window tkwin, const char *name, Bg **bgPtrPtr)
{
    BackgroundObject *corePtr;
    BackgroundInterpData *dataPtr;
    Bg *bgPtr;                          /* BackgroundObject container. */
    Blt_HashEntry *hPtr;
    int isNew;
    
    /* Create new token for the background. */
    bgPtr = Blt_Calloc(1, sizeof(Bg));
    if (bgPtr == NULL) {
        Tcl_AppendResult(interp, "can't allocate background \"", name, "\".", 
                (char *)NULL);
        return TCL_ERROR;
    }
    dataPtr = GetBackgroundInterpData(interp);
    hPtr = Blt_CreateHashEntry(&dataPtr->instTable, name, &isNew);
    if (isNew) {
        Tk_3DBorder border;
        
        /* BackgroundObject doesn't already exist, see if it's a color name
         * (i.e. something that Tk_Get3DBorder will accept). */
        border = Tk_Get3DBorder(interp, tkwin, name);
        if (border == NULL) {
            goto error;                 /* Nope. It's an error. */
        } 
        corePtr = NewBackgroundObject(dataPtr, interp, BLT_PAINTBRUSH_COLOR,
                border);
        if (corePtr == NULL) {
            Tk_Free3DBorder(border);
            goto error;                 /* Can't allocate new background. */
        }
        /* Configure the GC with the border color. */
        corePtr->hashPtr = hPtr;
        corePtr->name = Blt_GetHashKey(&dataPtr->instTable, hPtr);
        corePtr->link = NULL;
        Blt_SetHashValue(hPtr, corePtr);
    } else {
        corePtr = Blt_GetHashValue(hPtr);
        assert(corePtr != NULL);
    }
    /* Add the new background to the background's list of clients. */
    bgPtr->link = Blt_Chain_Append(corePtr->chain, bgPtr);
    bgPtr->corePtr = corePtr;
    *bgPtrPtr = bgPtr;
    return TCL_OK;
 error:
    Blt_Free(bgPtr);
    Blt_DeleteHashEntry(&dataPtr->instTable, hPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetBgFromObj
 *
 *      Retrieves a new token of a background from the named background.
 *
 * Results:
 *      Returns a background token.
 *
 * Side Effects:
 *      Memory is allocated for the new token.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_GetBgFromObj(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr,
                 Bg **bgPtrPtr)
{
    return Blt_GetBg(interp, tkwin, Tcl_GetString(objPtr), bgPtrPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_SetChangedProc
 *
 *      Sets the routine to called when an image changes.  
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The designated routine will be called the next time the image
 *      associated with the tile changes.
 *
 *---------------------------------------------------------------------------
 */
/*LINTLIBRARY*/
void
Blt_Bg_SetChangedProc(
    Bg *bgPtr,                          /* Background with which to
                                         * register callback. */
    Blt_BackgroundChangedProc *notifyProc, /* Function to call when background
                                         * has changed. NULL indicates to
                                         * unset the callback.*/
    ClientData clientData)
{
    bgPtr->notifyProc = notifyProc;
    bgPtr->clientData = clientData;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_Free
 *
 *      Removes the background token.
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
Blt_Bg_Free(Bg *bgPtr)
{
    BackgroundObject *corePtr = bgPtr->corePtr;

    assert(corePtr != NULL);
    DestroyBackground(bgPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_GetOrigin
 *
 *      Returns the coordinates of the origin of the background referenced
 *      by the token.
 *
 * Results:
 *      Returns the coordinates of the origin.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_GetOrigin(Bg *bgPtr, int *xPtr, int *yPtr)
{
    *xPtr = 0, *yPtr = 0;
    if (bgPtr->corePtr->brush != NULL) {
        Blt_GetBrushOrigin(bgPtr->corePtr->brush, xPtr, yPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_SetOrigin
 *
 *      Sets the origin of the background referenced by the token.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_SetOrigin(Tk_Window tkwin, Bg *bgPtr, int x, int y)
{
    if (bgPtr->corePtr->brush != NULL) {
        Blt_SetBrushOrigin(bgPtr->corePtr->brush, x, y);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_Name
 *
 *      Returns the name of the core background referenced by the
 *      token.
 *
 * Results:
 *      Return the name of the background.
 *
 *---------------------------------------------------------------------------
 */
const char *
Blt_Bg_Name(Bg *bgPtr)
{
    if (bgPtr->corePtr->name == NULL) {
        return "";
    }
    return bgPtr->corePtr->name;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_BorderColor
 *
 *      Returns the border color of the background referenced by the token.
 *
 * Results:
 *      Returns the XColor representing the border color of the background.
 *
 *---------------------------------------------------------------------------
 */
XColor *
Blt_Bg_BorderColor(Bg *bgPtr)
{
    return Tk_3DBorderColor(bgPtr->corePtr->border);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_Border
 *
 *      Returns the border of the background referenced by the token.
 *
 * Results:
 *      Return the border of the background.
 *
 *---------------------------------------------------------------------------
 */
Tk_3DBorder
Blt_Bg_Border(Bg *bgPtr)
{
    return bgPtr->corePtr->border;
}

Blt_PaintBrush 
Blt_Bg_PaintBrush(Bg *bgPtr)
{
    return bgPtr->corePtr->brush;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_DrawRectangle
 *
 *      Draws the background in the designated window.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_DrawRectangle(Tk_Window tkwin, Drawable drawable, Bg *bgPtr, int x, 
                     int y, int w, int h, int borderWidth, int relief)
{
    if ((h < 1) || (w < 1)) {
        fprintf(stderr, "Blt_Bg_DrawRectangle %s x=%d y=%d w=%d h=%d\n",
                Tk_PathName(tkwin), x, y, w, h);
        abort();
        return;
    }
    Tk_Draw3DRectangle(tkwin, drawable, bgPtr->corePtr->border, x, y, w, h, 
        borderWidth, relief);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_FillRectangle
 *
 *      Draws the background in the designated window.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_FillRectangle(Tk_Window tkwin, Drawable drawable, Bg *bgPtr, int x, 
                     int y, int w, int h, int borderWidth, int relief)
{
    if ((h < 1) || (w < 1)) {
        fprintf(stderr, "Blt_Bg_FillRectangle %s x=%d y=%d w=%d h=%d\n",
                Tk_PathName(tkwin), x, y, w, h);
        abort();
        return;
    }
    DrawBackgroundRectangle(tkwin, drawable, bgPtr, x, y, w, h);
    if ((relief != TK_RELIEF_FLAT) && (borderWidth > 0)) {
        Tk_Draw3DRectangle(tkwin, drawable, bgPtr->corePtr->border, x, y, w, h,
                borderWidth, relief);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_DrawPolygon
 *
 *      Draws the background in the designated window.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_DrawPolygon(Tk_Window tkwin, Drawable drawable, Bg *bgPtr, 
                   XPoint *points, int numPoints, int borderWidth, int relief)
{
    if (numPoints < 3) {
        return;
    }
    Draw3DPolygon(tkwin, drawable, bgPtr->corePtr->border, points, numPoints,
                  borderWidth, relief);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_FillPolygon
 *
 *      Draws the background in the designated window.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_FillPolygon(Tk_Window tkwin, Drawable drawable, Bg *bgPtr,
                   XPoint *points, int numPoints, int borderWidth, int relief)
{
    if (numPoints < 3) {
        return;
    }
    DrawBackgroundPolygon(tkwin, drawable, bgPtr, numPoints, points);
    if ((relief != TK_RELIEF_FLAT) && (borderWidth != 0)) {
        Draw3DPolygon(tkwin, drawable, bgPtr->corePtr->border, points,
                      numPoints, borderWidth, relief);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_DrawFocus
 *
 *      Draws the background in the designated picture.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_DrawFocus(Tk_Window tkwin, Bg *bgPtr, int highlightThickness, 
                 Drawable drawable)
{
    int w, h, t;

    w = Tk_Width(tkwin);
    h = Tk_Height(tkwin);
    t = highlightThickness;
    /* Top */
    DrawBackgroundRectangle(tkwin, drawable, bgPtr, 0, 0, w, t);
    /* Bottom */
    DrawBackgroundRectangle(tkwin, drawable, bgPtr, 0, h - t, w, t);
    /* Left */
    DrawBackgroundRectangle(tkwin, drawable, bgPtr, 0, t, t, h - 2 * t);
    /* Right */
    DrawBackgroundRectangle(tkwin, drawable, bgPtr, w - t, t, t, h - 2 * t);
}


#ifdef notdef
static void 
Draw3DRectangle(Tk_Window tkwin, Drawable drawable, Bg *bgPtr, 
                int x, int y, int w, int h, int borderWidth, int relief)
{
    int i, n;
    XSegment *segments, *sp;

    n = borderWidth + borderWidth;
    segments = Blt_AssertMalloc(sizeof(XSegment) * n);
    sp = segments;
    for (i = 0; i < borderWidth; i++) {
        sp->x1 = x + i;
        sp->y1 = y + i;
        sp->x2 = x + (w - 1) - i;
        sp->y2 = y + i;
        sp++;
        sp->x1 = x + i;
        sp->y1 = y + i;
        sp->x2 = x + i;
        sp->y2 = y + (h - 1) - i;
        sp++;
    }
    gc = Tk_3DBorderGC(tkwin, bgPtr->corePtr->border, TK_3D_LIGHT_GC);
    XDrawSegments(Tk_Display(tkwin), drawable, gc, segments, n);

    sp = segments;
    for (i = 0; i < borderWidth; i++) {
        sp->x1 = x + i;
        sp->y1 = y + (h - 1) - i;
        sp->x2 = x + (w - 1) - i;
        sp->y2 = y + (h - 1) - i;
        sp++;
        sp->x1 = x + (w - 1 ) - i;
        sp->y1 = y + i;
        sp->x2 = x + (w - 1) - i;
        sp->y2 = y + (h - 1) - i;
        sp++;
    }
    gc = Tk_3DBorderGC(tkwin, bgPtr->corePtr->border, TK_3D_DARK_GC);
    XDrawSegments(Tk_Display(tkwin), drawable, gc, segments, n);
}
#endif

void
Blt_Bg_SetFromBackground(Tk_Window tkwin, Bg *bgPtr)
{
    Tk_SetBackgroundFromBorder(tkwin, bgPtr->corePtr->border);
}

GC
Blt_Bg_BorderGC(Tk_Window tkwin, Bg *bgPtr, int which)
{
    return Tk_3DBorderGC(tkwin, bgPtr->corePtr->border, which);
}

void
Blt_3DBorder_SetClipRegion(Tk_Window tkwin, Tk_3DBorder border, TkRegion rgn)
{
    GC gc;
    Display *display;

    display = Tk_Display(tkwin);
    gc = Tk_3DBorderGC(tkwin, border, TK_3D_LIGHT_GC);
    TkSetRegion(display, gc, rgn);
    gc = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
    TkSetRegion(display, gc, rgn);
    gc = Tk_3DBorderGC(tkwin, border, TK_3D_FLAT_GC);
    TkSetRegion(display, gc, rgn);
}

void
Blt_3DBorder_UnsetClipRegion(Tk_Window tkwin, Tk_3DBorder border)
{
    Display *display;
    GC gc;

    display = Tk_Display(tkwin);
    gc = Tk_3DBorderGC(tkwin, border, TK_3D_LIGHT_GC);
    XSetClipMask(display, gc, None);
    gc = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
    XSetClipMask(display, gc, None);
    gc = Tk_3DBorderGC(tkwin, border, TK_3D_FLAT_GC);
    XSetClipMask(display, gc, None);
}

void
Blt_Bg_SetClipRegion(Tk_Window tkwin, Bg *bgPtr, TkRegion rgn)
{
    Blt_Painter painter;

    Blt_3DBorder_SetClipRegion(tkwin, bgPtr->corePtr->border, rgn);
    painter = Blt_GetPainter(tkwin, 1.0);
    Blt_SetPainterClipRegion(painter, rgn);
}

void
Blt_Bg_UnsetClipRegion(Tk_Window tkwin, Bg *bgPtr)
{
    Blt_Painter painter;

    Blt_3DBorder_UnsetClipRegion(tkwin, bgPtr->corePtr->border);
    painter = Blt_GetPainter(tkwin, 1.0);
    Blt_UnsetPainterClipRegion(painter);
}

unsigned int
Blt_Bg_GetColor(Bg *bgPtr)
{
    return GetBackgroundColor(bgPtr->corePtr);
}

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltBg.c --
 *
 * This module creates backgrounds for the BLT toolkit.
 *
 *	Copyright 1995-2004 George A Howlett.
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

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include <limits.h>

#include <X11/Xutil.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltImage.h"
#include "bltPicture.h"
#include "bltBg.h"
#include "bltPainter.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define JCLAMP(c)	((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))

#define BG_BACKGROUND_THREAD_KEY	"BLT Background Data"

/* 
   blt::background create type 
	-image $image
	-color $color
	-darkcolor $color
	-lightcolor $color
	-resamplefilter $filter
	-opacity $alpha
	-xorigin $x
	-yorigin $y
	-tile yes
	-center yes
	-scale no
	-relativeto self|toplevel|window
	-mask image|bitmap

  blt::background create tile 
	-relativeto self|toplevel|window
	-image $image
	-bg $color
  blt::background create picture 
        -image $image
	-filter $filterName
	-bg $color
  blt::background create gradient 
	-type radial|horizontal|vertical|updiagonal|downdiagonal|conical
	-low $color
	-high $color
	-bg $color
	-palette palette 
  blt::background create border 
	-bg $color
	-alpha $color 

  blt::background create texture -type metal|wind|??? 
	-bg $color

  blt::background names
  blt::background configure $tile
  blt::background delete $tile
*/

/* 
 * Types of backgrounds: "solid", "tile", "gradient", "texture"
 */
typedef enum BackgroundTypes {
    BACKGROUND_GRADIENT,		/* Color gradient. */
    BACKGROUND_TILE,			/* Tiled or resizable color picture. */
    BACKGROUND_SOLID,			/* Solid background. */
    BACKGROUND_TEXTURE,			/* Procedural texture. */
} BackgroundType;

static const char *backgroundTypes[] = {
    "gradient",
    "tile",
    "solid",
    "texture"
};

typedef enum ReferenceTypes {
    REFERENCE_SELF,			/* Current window. */
    REFERENCE_TOPLEVEL,			/* Toplevel of current window. */
    REFERENCE_WINDOW,			/* Specifically named window. */
    REFERENCE_NONE,		        /* Don't use reference
					 * window. Background region will be
					 * defined by user. */
} ReferenceType;

typedef struct {
    Blt_HashTable instTable;		/* Hash table of background structures
					 * keyed by the name of the image. */
    Tcl_Interp *interp;			/* Interpreter associated with this set
					 * of backgrounds. */
    int nextId;				/* Serial number of the identifier to
					 * be used for next background
					 * created.  */
} BackgroundInterpData;

typedef struct _Background BackgroundObject;

typedef void (DestroyBackgroundProc)(BackgroundObject *corePtr);
typedef int (ConfigureBackgroundProc)(Tcl_Interp *interp, 
	BackgroundObject *corePtr, int objc, Tcl_Obj *const *objv, 
	unsigned int flags);
typedef void (DrawRectangleProc)(Tk_Window tkwin, Drawable drawable, 
	BackgroundObject *corePtr, int x, int y, int w, int h);
typedef void (DrawPolygonProc)(Tk_Window tkwin, Drawable drawable, 
	BackgroundObject *corePtr, int numPoints, XPoint *points);

typedef struct {
    BackgroundType type;		/* Type of background: solid, tile,
					 * texture, or gradient. */
    Blt_ConfigSpec *configSpecs;
    DestroyBackgroundProc *destroyProc;
    ConfigureBackgroundProc *configProc;
    DrawRectangleProc *drawRectangleProc;
    DrawPolygonProc *drawPolygonProc;
} BackgroundClass;

struct _Background {
    const char *name;			/* Generated name of background. */
    BackgroundClass *classPtr;
    BackgroundInterpData *dataPtr;
    Tk_Window tkwin;			/* Main window. Used to query
					 * background options. */
    Display *display;			/* Display of this background. */
    unsigned int flags;			/* See definitions below. */
    Blt_HashEntry *hashPtr;		/* Hash entry in background table. */
    Blt_Chain chain;			/* List of background tokens.  Used to
					 * register callbacks for each client of
					 * the background. */
    Blt_ChainLink link;			/* Background token that is associated
					 * with the background creation
					 * "background create...". */
    Tk_3DBorder border;			/* 3D Border.  May be used for all
					 * background types. */
    Tk_Window refWindow;		/* Refer to coordinates in this window
					 * when determining the tile/gradient
					 * origin. */
    ReferenceType reference;		/* "self", "toplevel", or "window". */
    Blt_HashTable pictTable;		/* Table of pictures cached for each
					 * background reference. */
    int xOrigin, yOrigin;
    Blt_PaintBrush brush;		/* Paint brush representing the
					 * background color. */
};

typedef struct {
    const char *name;			/* Generated name of background. */
    BackgroundClass *classPtr;
    BackgroundInterpData *dataPtr;
    Tk_Window tkwin;			/* Main window. Used to query
					 * background options. */
    Display *display;			/* Display of this background. */
    unsigned int flags;			/* See definitions below. */
    Blt_HashEntry *hashPtr;		/* Link to original client. */
    Blt_Chain chain;			/* List of background tokens.  Used to
					 * register callbacks for each client
					 * of the background. */
    Blt_ChainLink link;			/* Background token that is associated
					 * with the background creation
					 * "background create...". */
    Tk_3DBorder border;			/* 3D Border.  May be used for all
					 * background types. */
    Tk_Window refWindow;		/* Refer to coordinates in this window
					 * when determining the tile/gradient
					 * origin. */
    ReferenceType reference;		/* "self", "toplevel", or "window". */
    Blt_HashTable pictTable;		/* Table of pictures cached for each
					 * background reference. */
    int xOrigin, yOrigin;
    Blt_PaintBrush brush;		/* Paint brush representing the
					 * background color. */

    /* Solid background specific fields. */
    int alpha;				/* Transparency value. */
} SolidBackground;

typedef struct {
    const char *name;			/* Generated name of background. */
    BackgroundClass *classPtr;
    BackgroundInterpData *dataPtr;
    Tk_Window tkwin;			/* Main window. Used to query
					 * background options. */
    Display *display;			/* Display of this background. Used to
					 * free configuration options. */
    unsigned int flags;			/* See definitions below. */
    Blt_HashEntry *hashPtr;		/* Link to original client. */
    Blt_Chain chain;			/* List of background tokens.  Used to
					 * register callbacks for each client of
					 * the background. */
    Blt_ChainLink link;			/* Background token that is associated
					 * with the background creation
					 * "background create...". */
    Tk_3DBorder border;			/* 3D Border.  May be used for all
					 * background types. */
    Tk_Window refWindow;		/* Refer to coordinates in this window
					 * when determining the tile/gradient
					 * origin. */
    ReferenceType reference;		/* "self", "toplevel", or "window". */
    Blt_HashTable pictTable;		/* Table of pictures cached for each
					 * background reference. */
    int xOrigin, yOrigin;
    Blt_PaintBrush brush;		/* Paint brush representing the
					 * background color. */

    /* Tile specific fields. */
    Tk_Image tkImage;			/* Original image (before
					 * resampling). */
    Blt_Picture tile;
    Blt_ResampleFilter filter;		/* 1-D image filter to use to when
					 * resizing the original picture. */
    int alpha;				/* Transparency value. */
} TileBackground;

typedef struct {
    const char *name;			/* Generated name of background. */
    BackgroundClass *classPtr;
    BackgroundInterpData *dataPtr;
    Tk_Window tkwin;			/* Main window. Used to query
					 * background options. */
    Display *display;			/* Display of this background.*/
    unsigned int flags;			/* See definitions below. */
    Blt_HashEntry *hashPtr;		/* Link to original client. */
    Blt_Chain chain;			/* List of background tokens.  Used
					 * to register callbacks for each
					 * client of the background. */
    Blt_ChainLink link;			/* Background token that is
					 * associated with the background
					 * creation "background
					 * create...". */
    Tk_3DBorder border;			/* 3D Border.  May be used for all
					 * background types. */
    Tk_Window refWindow;		/* Refer to coordinates in this
					 * window when determining the
					 * tile/gradient origin. */
    ReferenceType reference;		/* "self", "toplevel", or
                                           "window". */
    Blt_HashTable pictTable;		/* Table of pictures cached for
					 * each background reference. */
    int xOrigin, yOrigin;
    Blt_PaintBrush brush;		/* Paint brush representing the
					 * background color. */

    /* Gradient background specific fields. */
    Blt_Gradient gradient;
    int aRange, rRange, gRange, bRange;
    Blt_Pixel low, high;		/* Texture or gradient colors. */
    int alpha;				/* Transparency value. */
    Blt_Jitter jitter;
    Blt_Palette palette;
} GradientBackground;

typedef struct {
    const char *name;			/* Generated name of background. */
    BackgroundClass *classPtr;
    BackgroundInterpData *dataPtr;
    Tk_Window tkwin;			/* Main window. Used to query
					 * background options. */
    Display *display;			/* Display of this background. */
    unsigned int flags;			/* See definitions below. */
    Blt_HashEntry *hashPtr;		/* Link to original client. */
    Blt_Chain chain;			/* List of background tokens.  Used
					 * to register callbacks for each
					 * client of the background. */
    Blt_ChainLink link;			/* Background token that is
					 * associated with the background
					 * creation "background
					 * create...". */
    Tk_3DBorder border;			/* 3D Border.  May be used for all
					 * background types. */
    Tk_Window refWindow;		/* Refer to coordinates in this
					 * window when determining the
					 * tile/gradient origin. */
    ReferenceType reference;		/* "self", "toplevel", or
                                           "window". */
    Blt_HashTable pictTable;		/* Table of pictures cached for
					 * each background reference. */
    int xOrigin, yOrigin;
    Blt_PaintBrush brush;		/* Paint brush representing the
					 * background color. */

    /* Texture background specific fields. */
    Blt_Pixel low, high;		/* Texture colors. */
    int aRange, rRange, gRange, bRange;
    int alpha;				/* Transparency value. */
    Blt_TextureType type;
    Blt_Jitter jitter;
    Blt_Palette palette;
} TextureBackground;

struct _Blt_Bg {
    BackgroundObject *corePtr;	       /* Pointer to master background. */
    Blt_Bg_ChangedProc *notifyProc;
    ClientData clientData;		/* Data to be passed on notifier
					 * callbacks.  */
    Blt_ChainLink link;			/* Entry in notifier list. */
};

#define DELETE_PENDING		(1<<0)
#define BG_CENTER		(1<<2)
#define BG_SCALE		(1<<3)
#define FREE_TILE		(1<<4)

typedef struct _Blt_Bg Bg;

#define DEF_OPACITY		"100.0"
#define DEF_ORIGIN_X		"0"
#define DEF_ORIGIN_Y		"0"
#define DEF_BORDER		STD_NORMAL_BACKGROUND
#define DEF_GRADIENT_PATH	"y"
#define DEF_GRADIENT_HIGH	"grey90"
#define DEF_GRADIENT_JITTER	"0.0"
#define DEF_GRADIENT_LOGSCALE	"yes"
#define DEF_GRADIENT_LOW	"grey50"
#define DEF_GRADIENT_TYPE	"vertical"
#define DEF_GRADIENT_SCALE	"linear"
#define DEF_REFERENCE		"toplevel"
#define DEF_RESAMPLE_FILTER	"box"
#define DEF_TEXTURE_JITTER	"0.0"
#define DEF_SCALE		"no"
#define DEF_CENTER		"no"
#define DEF_TILE		"no"

static Blt_OptionParseProc ObjToImageProc;
static Blt_OptionPrintProc ImageToObjProc;
static Blt_OptionFreeProc FreeImageProc;
static Blt_CustomOption imageOption =
{
    ObjToImageProc, ImageToObjProc, FreeImageProc, (ClientData)0
};

extern Blt_CustomOption bltFilterOption;

static Blt_OptionParseProc ObjToReferenceTypeProc;
static Blt_OptionPrintProc ReferenceTypeToObjProc;
static Blt_CustomOption referenceTypeOption =
{
    ObjToReferenceTypeProc, ReferenceTypeToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToGradientTypeProc;
static Blt_OptionPrintProc GradientTypeToObjProc;
static Blt_CustomOption gradientTypeOption =
{
    ObjToGradientTypeProc, GradientTypeToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToGradientScaleProc;
static Blt_OptionPrintProc GradientScaleToObjProc;
static Blt_CustomOption scaleOption =
{
    ObjToGradientScaleProc, GradientScaleToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToOpacityProc;
static Blt_OptionPrintProc OpacityToObjProc;
static Blt_CustomOption opacityOption =
{
    ObjToOpacityProc, OpacityToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToJitterProc;
static Blt_OptionPrintProc JitterToObjProc;
static Blt_CustomOption jitterOption =
{
    ObjToJitterProc, JitterToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToTextureTypeProc;
static Blt_OptionPrintProc TextureTypeToObjProc;
static Blt_CustomOption textureTypeOption =
{
    ObjToTextureTypeProc, TextureTypeToObjProc, NULL, (ClientData)0
};

static Blt_ConfigSpec solidConfigSpecs[] =
{
    {BLT_CONFIG_SYNONYM, "-background", "color", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_SYNONYM, "-bg", "color", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_BORDER, "-color", "color", "Color", DEF_BORDER, 
	Blt_Offset(SolidBackground, border), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", "100.0", 
	Blt_Offset(SolidBackground, alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec tileConfigSpecs[] =
{
    {BLT_CONFIG_BITMASK, "-center", "center", "Center", DEF_CENTER,
        Blt_Offset(TileBackground, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)BG_CENTER},
    {BLT_CONFIG_BORDER, "-color", "color", "Color", DEF_BORDER, 
	Blt_Offset(TileBackground, border), 0},
    {BLT_CONFIG_BORDER, "-darkcolor", "darkColor", "DarkColor", DEF_BORDER, 
	Blt_Offset(TileBackground, border), 0},
    {BLT_CONFIG_CUSTOM, "-filter", "filter", "Filter", DEF_RESAMPLE_FILTER, 
	Blt_Offset(TileBackground, filter), 0, &bltFilterOption},
    {BLT_CONFIG_CUSTOM, "-image", "image", "Image", (char *)NULL,
        Blt_Offset(TileBackground, tkImage), BLT_CONFIG_DONT_SET_DEFAULT, 
	&imageOption},
    {BLT_CONFIG_BORDER, "-lightcolor", "lightColor", "LightColor", DEF_BORDER, 
	Blt_Offset(TileBackground, border), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", "100.0", 
	Blt_Offset(TileBackground, alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_CUSTOM, "-relativeto", "relativeTo", "RelativeTo", 
	DEF_REFERENCE, Blt_Offset(TileBackground, reference), 
	BLT_CONFIG_DONT_SET_DEFAULT, &referenceTypeOption},
    {BLT_CONFIG_BITMASK, "-scale", "scale", "scale", DEF_SCALE,
        Blt_Offset(TileBackground, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)BG_SCALE},
    {BLT_CONFIG_PIXELS, "-xorigin", "xOrigin", "XOrigin", DEF_ORIGIN_X,
        Blt_Offset(TileBackground, xOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yorigin", "yOrigin", "YOrigin", DEF_ORIGIN_Y,
        Blt_Offset(TileBackground, yOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec gradientConfigSpecs[] =
{
    {BLT_CONFIG_BORDER, "-background", "background", "Background", DEF_BORDER,
	Blt_Offset(GradientBackground, border), 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIX32, "-high", "high", "High", DEF_GRADIENT_HIGH,
        Blt_Offset(GradientBackground, high), 0},
    {BLT_CONFIG_CUSTOM, "-jitter", "jitter", "Jitter", DEF_GRADIENT_JITTER, 
	Blt_Offset(GradientBackground, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-low", "low", "Low", DEF_GRADIENT_LOW,
        Blt_Offset(GradientBackground, low), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", "100.0", 
	Blt_Offset(GradientBackground, alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_CUSTOM, "-relativeto", "relativeTo", "RelativeTo", 
	DEF_REFERENCE, Blt_Offset(GradientBackground, reference), 
	BLT_CONFIG_DONT_SET_DEFAULT, &referenceTypeOption},
    {BLT_CONFIG_CUSTOM, "-scale", "scale", "Scale", 
	DEF_GRADIENT_SCALE, Blt_Offset(GradientBackground, gradient.scale),
	BLT_CONFIG_DONT_SET_DEFAULT, &scaleOption},
    {BLT_CONFIG_CUSTOM, "-type", "type", "type", DEF_GRADIENT_TYPE, 
	Blt_Offset(GradientBackground, gradient.type), 
	BLT_CONFIG_DONT_SET_DEFAULT, &gradientTypeOption},
    {BLT_CONFIG_PIXELS, "-xorigin", "xOrigin", "XOrigin", DEF_ORIGIN_X,
        Blt_Offset(GradientBackground, xOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yorigin", "yOrigin", "YOrigin", DEF_ORIGIN_Y,
        Blt_Offset(GradientBackground, yOrigin), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec textureConfigSpecs[] =
{
    {BLT_CONFIG_BORDER, "-background", "background", "Background", DEF_BORDER,
	Blt_Offset(TextureBackground, border), 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIX32, "-high", "high", "High", DEF_GRADIENT_HIGH,
        Blt_Offset(TextureBackground, high), 0},
    {BLT_CONFIG_CUSTOM, "-jitter", "jitter", "Jitter", DEF_TEXTURE_JITTER, 
	Blt_Offset(TextureBackground, jitter.range), 
        BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-low", "low", "Low", DEF_GRADIENT_LOW,
        Blt_Offset(TextureBackground, low), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", "100.0", 
	Blt_Offset(TextureBackground, alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_CUSTOM, "-relativeto", "relativeTo", "RelativeTo", 
	DEF_REFERENCE, Blt_Offset(TextureBackground, reference), 
	BLT_CONFIG_DONT_SET_DEFAULT, &referenceTypeOption},
    {BLT_CONFIG_CUSTOM, "-type", "type", "Type", 
	DEF_REFERENCE, Blt_Offset(TextureBackground, type), 
	BLT_CONFIG_DONT_SET_DEFAULT, &textureTypeOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static void NotifyClients(BackgroundObject *corePtr);

/* 
 * Quick and dirty random number generator. 
 *
 * http://www.shadlen.org/ichbin/random/generators.htm#quick 
 */
#define JITTER_SEED	31337
#define JITTER_A	1099087573U
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
#endif	/* SIZEOF_INT == 8 */
    return (double)randomPtr->value * RANDOM_SCALE;
}

static void
JitterInit(Blt_Jitter *jitterPtr) 
{
    RandomInit(&jitterPtr->random);
    jitterPtr->range = 0.1;
    jitterPtr->offset = -0.05;		/* Jitter +/-  */
}

static INLINE double 
Jitter(Blt_Jitter *jitterPtr) {
    double value;

    value = RandomNumber(&jitterPtr->random);
    return (value * jitterPtr->range) + jitterPtr->offset;
}

static Blt_Picture
ImageToPicture(TileBackground *corePtr)
{
    Tcl_Interp *interp;
    Blt_Picture picture;
    int isNew;

    interp = corePtr->dataPtr->interp;
    picture = Blt_GetPictureFromImage(interp, corePtr->tkImage, &isNew);
    if (isNew) {
	corePtr->flags |= FREE_TILE;
    } else {
	corePtr->flags &= ~FREE_TILE;
    }
    return picture;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageChangedProc
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ImageChangedProc(
    ClientData clientData,
    int x, int y, int width, int height, /* Not used. */
    int imageWidth, int imageHeight)	 /* Not used. */
{
    BackgroundObject *corePtr = clientData;

    /* Propagate the change in the image to all the clients. */
    NotifyClients(corePtr);
}

/*ARGSUSED*/
static void
FreeImageProc(
    ClientData clientData,
    Display *display,			/* Not used. */
    char *widgRec,
    int offset)
{
    TileBackground *corePtr = (TileBackground *)(widgRec);

    if (corePtr->tkImage != NULL) {
	Tk_FreeImage(corePtr->tkImage);
	corePtr->tkImage = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToImageProc --
 *
 *	Given an image name, get the Tk image associated with it.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToImageProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,		        /* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    TileBackground *corePtr = (TileBackground *)(widgRec);
    Tk_Image tkImage;

    tkImage = Tk_GetImage(interp, corePtr->tkwin, Tcl_GetString(objPtr), 
	ImageChangedProc, corePtr);
    if (tkImage == NULL) {
	return TCL_ERROR;
    }
    corePtr->tkImage = tkImage;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageToObjProc --
 *
 *	Convert the image name into a string Tcl_Obj.
 *
 * Results:
 *	The string representation of the image is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ImageToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    TileBackground *corePtr = (TileBackground *)(widgRec);

    if (corePtr->tkImage == NULL) {
	return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(Blt_Image_Name(corePtr->tkImage), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToReferenceType --
 *
 *	Converts the given Tcl_Obj to a reference type.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToReferenceTypeProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,		        /* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    BackgroundObject *corePtr = (BackgroundObject *)(widgRec);
    ReferenceType *referencePtr = (ReferenceType *)(widgRec + offset);
    const char *string;
    char c;
    int refType;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 's') && (strncmp(string, "self", length) == 0)) {
	refType = REFERENCE_SELF;
    } else if ((c == 't') && (strncmp(string, "toplevel", length) == 0)) {
	refType = REFERENCE_TOPLEVEL;
    } else if ((c == 'n') && (strncmp(string, "none", length) == 0)) {
	refType = REFERENCE_NONE;
    } else if (c == '.') {
	Tk_Window tkwin, tkMain;

	tkMain = Tk_MainWindow(interp);
	tkwin = Tk_NameToWindow(interp, string, tkMain);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	refType = REFERENCE_WINDOW;
	corePtr->refWindow = tkwin;
    } else {
	Tcl_AppendResult(interp, "unknown reference type \"", string, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    *referencePtr = refType;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReferenceTypeToObjProc --
 *
 *	Converts the background reference window type into a string Tcl_Obj.
 *
 * Results:
 *	The string representation of the reference window type is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ReferenceTypeToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    ReferenceType reference = *(ReferenceType *)(widgRec + offset);
    const char *string;

    switch (reference) {
    case REFERENCE_SELF:
	string = "self";
	break;

    case REFERENCE_TOPLEVEL:
	string = "toplevel";
	break;

    case REFERENCE_NONE:
	string = "none";
	break;

    case REFERENCE_WINDOW:
	{
	    BackgroundObject *corePtr = (BackgroundObject *)(widgRec);

	    string = Tk_PathName(corePtr->refWindow);
	}
	break;

    default:
	string = "???";
	break;
    }
    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToGradientTypeProc --
 *
 *	Translate the given string to the gradient type it represents.
 *	Types are "horizontal", "vertical", "updiagonal", "downdiagonal", 
 *	and "radial"".
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToGradientTypeProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Blt_GradientType *typePtr = (Blt_GradientType *)(widgRec + offset);
    const char *string;
    char c;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'v') && (strcmp(string, "vertical") == 0)) {
	*typePtr = BLT_GRADIENT_TYPE_VERTICAL;
    } else if ((c == 'h') && (strcmp(string, "horizontal") == 0)) {
	*typePtr = BLT_GRADIENT_TYPE_HORIZONTAL;
    } else if ((c == 'r') && (strcmp(string, "radial") == 0)) {
	*typePtr = BLT_GRADIENT_TYPE_RADIAL;
    } else if ((c == 'u') && (strcmp(string, "updiagonal") == 0)) {
	*typePtr = BLT_GRADIENT_TYPE_DIAGONAL_UP;
    } else if ((c == 'd') && (strcmp(string, "downdiagonal") == 0)) {
	*typePtr = BLT_GRADIENT_TYPE_DIAGONAL_DOWN;
    } else {
	Tcl_AppendResult(interp, "unknown gradient type \"", string, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static const char *
NameOfGradientType(Blt_GradientType type) 
{
    switch (type) {
    case BLT_GRADIENT_TYPE_VERTICAL:
	return "vertical";
    case BLT_GRADIENT_TYPE_HORIZONTAL:
	return "horizontal";
    case BLT_GRADIENT_TYPE_RADIAL:
	return "radial";
    case BLT_GRADIENT_TYPE_DIAGONAL_UP:
	return "updiagonal";
    case BLT_GRADIENT_TYPE_DIAGONAL_DOWN:
	return "downdiagonal";	
    default:
	return "???";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GradientTypeToObjProc --
 *
 *	Returns the string representing the current gradiant shape.
 *
 * Results:
 *	The string representation of the shape is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
GradientTypeToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Blt_GradientType type = *(Blt_GradientType *)(widgRec + offset);
    
    return Tcl_NewStringObj(NameOfGradientType(type), -1);
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToGradientScaleProc --
 *
 *	Translates the given string to the gradient scale it represents.  Valid
 *	paths are "linear", "log", "atan".
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToGradientScaleProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Blt_GradientScale *scalePtr = (Blt_GradientScale *)(widgRec + offset);
    const char *string;
    int length;
    char c;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'l') && (strcmp(string, "linear") == 0)) {
	*scalePtr = BLT_GRADIENT_SCALE_LINEAR;
    } else if ((c == 'l') && (length > 2) && 
	       (strncmp(string, "logarithmic", length) == 0)) {
	*scalePtr = BLT_GRADIENT_SCALE_LOG;
    } else if ((c == 'a') && (strcmp(string, "atan") == 0)) {
	*scalePtr = BLT_GRADIENT_SCALE_ATAN;
    } else {
	Tcl_AppendResult(interp, "unknown gradient scale \"", string, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static const char *
NameOfGradientScale(Blt_GradientScale scale) 
{
    switch (scale) {
    case BLT_GRADIENT_SCALE_LINEAR:
	return "linear";
    case BLT_GRADIENT_SCALE_LOG:
	return "log";
    case BLT_GRADIENT_SCALE_ATAN:
	return "atan";
    default:
	return "?? unknown scale ??";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GradientScaleToObjProc --
 *
 *	Convert the scale into a string Tcl_Obj.
 *
 * Results:
 *	The string representation of the scale is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
GradientScaleToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Blt_GradientScale scale = *(Blt_GradientScale *)(widgRec + offset);

    return Tcl_NewStringObj(NameOfGradientScale(scale), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToOpacityProc --
 *
 *	Convert the string representation of opacity (a percentage) to
 *	an alpha value 0..255.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToOpacityProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    int *alphaPtr = (int *)(widgRec + offset);
    double opacity;

    if (Tcl_GetDoubleFromObj(interp, objPtr, &opacity) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((opacity < 0.0) || (opacity > 100.0)) {
	Tcl_AppendResult(interp, "invalid percent opacity \"", 
		Tcl_GetString(objPtr), "\" should be 0 to 100", (char *)NULL);
	return TCL_ERROR;
    }
    opacity = (opacity / 100.0) * 255.0;
    *alphaPtr = ROUND(opacity);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OpacityToObjProc --
 *
 *	Convert the alpha value into a string Tcl_Obj representing a
 *	percentage.
 *
 * Results:
 *	The string representation of the filter is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
OpacityToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
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
 * ObjToJitterProc --
 *
 *	Given a string representation of the jitter value (a percentage),
 *	convert it to a number 0..1.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToJitterProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    double *jitterPtr = (double *)(widgRec + offset);
    double jitter;

    if (Tcl_GetDoubleFromObj(interp, objPtr, &jitter) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((jitter < 0.0) || (jitter > 100.0)) {
	Tcl_AppendResult(interp, "invalid percent jitter \"", 
		Tcl_GetString(objPtr), "\" should be 0 to 100", (char *)NULL);
	return TCL_ERROR;
    }
    *jitterPtr = jitter * 0.01;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OpacityToObjProc --
 *
 *	Convert the picture filter into a string Tcl_Obj.
 *
 * Results:
 *	The string representation of the filter is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
JitterToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    double *jitterPtr = (double *)(widgRec + offset);
    double jitter;

    jitter = (double)*jitterPtr * 100.0;
    return Tcl_NewDoubleObj(jitter);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTextureTypeProc --
 *
 *	Translate the given string to the texture type it represents.  
 *	Types are "checker", "striped"".
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTextureTypeProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    char *string;
    char c;
    Blt_TextureType *typePtr = (Blt_TextureType *)(widgRec + offset);
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 's') && (strncmp(string, "striped", length) == 0)) {
	*typePtr = BLT_TEXTURE_TYPE_STRIPED;
    } else if ((c == 'c') && (strncmp(string, "checkered", length) == 0)) {
	*typePtr = BLT_TEXTURE_TYPE_CHECKERED;
    } else {
	Tcl_AppendResult(interp, "unknown texture type \"", string, "\"",
			 (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static const char *
NameOfTextureType(Blt_TextureType type) 
{
    switch (type) {
    case BLT_TEXTURE_TYPE_STRIPED:
	return "striped";
    case BLT_TEXTURE_TYPE_CHECKERED:
	return "checkered";
    default:
	return "???";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TextureTypeToObjProc --
 *
 *	Returns the string representing the background type.
 *
 * Results:
 *	The string representation of the background type is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TextureTypeToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Blt_TextureType type = *(Blt_TextureType *)(widgRec + offset);

    return Tcl_NewStringObj(NameOfTextureType(type), -1);
}

static int
GradientColorProc(Blt_PaintBrush *paintPtr, int x, int y)
{
    double t;
    Blt_Pixel color;
    GradientBackground *corePtr;
    Blt_Gradient *gradPtr;

    corePtr = paintPtr->clientData;
    gradPtr = &corePtr->gradient;
    switch (gradPtr->type) {
    case BLT_GRADIENT_TYPE_RADIAL:
	{
	    double dx, dy, d;

	    dx = x - gradPtr->xOffset;
	    dy = y - gradPtr->yOffset;
	    d = sqrt(dx * dx + dy * dy);
	    t = 1.0 - (d * gradPtr->scaleFactor);
	}
	break;
    case BLT_GRADIENT_TYPE_DIAGONAL_DOWN:
    case BLT_GRADIENT_TYPE_DIAGONAL_UP:
	{
	    double cx, cy, rx;
	    
	    /* Translate to the center of the reference window. */
	    cx = x - gradPtr->xOffset;
	    cy = y - gradPtr->yOffset;
	    /* Rotate x-coordinate by the slope of the diagonal. */
	    rx = (cx * gradPtr->cosTheta) - (cy * gradPtr->sinTheta);
	    /* Translate back.  */
	    rx += gradPtr->length * 0.5;
	    assert(rx >= 0 && rx < gradPtr->length);
	    t = rx * gradPtr->scaleFactor;
	}
	break;
    case BLT_GRADIENT_TYPE_HORIZONTAL:
	t = (double)x * gradPtr->scaleFactor;
	break;
    default:
    case BLT_GRADIENT_TYPE_VERTICAL:
	t = (double)y * gradPtr->scaleFactor;
	break;
    }
    if (corePtr->jitter.range > 0.0) {
	t += Jitter(&corePtr->jitter);
	t = JCLAMP(t);
    }
    if (gradPtr->scale == BLT_GRADIENT_SCALE_LOG) {
	t = log10(9.0 * t + 1.0);
    } else if (gradPtr->scale == BLT_GRADIENT_SCALE_ATAN) {
	t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
    } 
    if (corePtr->palette != NULL) {
	return Blt_Palette_GetAssociatedColor(corePtr->palette, t);
    }
    color.Red   = (unsigned char)(corePtr->low.Red   + t * corePtr->rRange);
    color.Green = (unsigned char)(corePtr->low.Green + t * corePtr->gRange);
    color.Blue  = (unsigned char)(corePtr->low.Blue  + t * corePtr->bRange);
    color.Alpha = (unsigned char)(corePtr->low.Alpha + t * corePtr->aRange);
    Blt_AssociateColor(&color);
    return color.u32;
}

static int
TextureColorProc(Blt_PaintBrush *paintPtr, int x, int y)
{
    double t;
    Blt_Pixel color;
    TextureBackground *corePtr;

    corePtr = paintPtr->clientData;
    switch (corePtr->type) {
    default:
    case BLT_TEXTURE_TYPE_STRIPED:
	t = ((y / 2) & 0x1) ? 0 : 1;
	break;
    case BLT_TEXTURE_TYPE_CHECKERED:
	{
	    int oddx, oddy;
	    
	    oddx = (x / 8) & 0x01;
	    oddy = (y / 8) & 0x01;
	    t = ((oddy + oddx) == 1) ? 0 : 1;
	}
	break;
    }
    if (corePtr->jitter.range > 0.0) {
	t += Jitter(&corePtr->jitter);
	t = JCLAMP(t);
    }
    color.Red   = (unsigned char)(corePtr->low.Red   + t * corePtr->rRange);
    color.Green = (unsigned char)(corePtr->low.Green + t * corePtr->gRange);
    color.Blue  = (unsigned char)(corePtr->low.Blue  + t * corePtr->bRange);
    color.Alpha = (unsigned char)(corePtr->low.Alpha + t * corePtr->aRange);
    Blt_AssociateColor(&color);
    return color.u32;
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyClients --
 *
 *	Notify each client that the background has changed.
 *
 * Results:
 *	None.
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

static const char *
NameOfBackgroundType(BackgroundObject *corePtr) 
{
    return backgroundTypes[corePtr->classPtr->type];
}

static int 
GetBackgroundTypeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
			 BackgroundType *typePtr)
{
    const char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 't') && (length > 1) && (strncmp(string, "tile", length) == 0)) {
	*typePtr = BACKGROUND_TILE;
    } else if ((c == 'g') && (strncmp(string, "gradient", length) == 0)) {
	*typePtr = BACKGROUND_GRADIENT;
    } else if ((c == 's') && (strncmp(string, "solid", length) == 0)) {
	*typePtr = BACKGROUND_SOLID;
    } else if ((c == 't') && (length > 1)  &&
	       (strncmp(string, "texture", length) == 0)) {
	*typePtr = BACKGROUND_TEXTURE;
    } else {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "unknown background \"", string, 
		"\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    return TCL_OK;
}

static void
ClearCache(BackgroundObject *corePtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&corePtr->pictTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Blt_Picture picture;

	picture = Blt_GetHashValue(hPtr);
	Blt_FreePicture(picture);
    }
}

static void 
GetOffsets(Tk_Window tkwin, BackgroundObject *corePtr, int x, int y, 
	   int *xOffsetPtr, int *yOffsetPtr)
{
    Tk_Window refWindow;

    if (corePtr->reference == REFERENCE_SELF) {
	refWindow = tkwin;
    } else if (corePtr->reference == REFERENCE_TOPLEVEL) {
	refWindow = Blt_Toplevel(tkwin);
    } else if (corePtr->reference == REFERENCE_WINDOW) {
	refWindow = corePtr->refWindow;
    } else if (corePtr->reference == REFERENCE_NONE) {
	refWindow = NULL;
    } else {
	return;		/* Unknown reference window. */
    }
    if ((corePtr->reference == REFERENCE_WINDOW) ||
	(corePtr->reference == REFERENCE_TOPLEVEL)) {
	Tk_Window tkwin2;
	
	tkwin2 = tkwin;
	while ((tkwin2 != refWindow) && (tkwin2 != NULL)) {
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
	    corePtr->reference = REFERENCE_SELF;
	    refWindow = tkwin;
	    abort();
	}
    }
    x += corePtr->xOrigin;
    y += corePtr->yOrigin;
    *xOffsetPtr = -x;
    *yOffsetPtr = -y;
#ifdef notdef
    fprintf(stderr, "Tile offsets x0=%d y0=%d x=%d,y=%d sx=%d,sy=%d\n",
	    x0, y0, x, y, *xOffsetPtr, *yOffsetPtr);
#endif
}

static Tk_Window
GetReferenceWindow(BackgroundObject *corePtr, Tk_Window tkwin, int *widthPtr, 
		   int *heightPtr)
{
    Tk_Window refWindow;

    if (corePtr->reference == REFERENCE_SELF) {
	refWindow = tkwin;
    } else if (corePtr->reference == REFERENCE_TOPLEVEL) {
	refWindow = Blt_Toplevel(tkwin);
    } else if (corePtr->reference == REFERENCE_WINDOW) {
	refWindow = corePtr->refWindow;
    } else {
	refWindow = tkwin;		/* Default to self. */
    }
    *widthPtr = Tk_Width(refWindow);
    *heightPtr = Tk_Height(refWindow);
    return refWindow;
}

static void
InitGradient(GradientBackground *corePtr, int refWidth, int refHeight)
{
    Blt_Gradient *gradPtr;

    gradPtr = &corePtr->gradient;
    if (corePtr->jitter.range > 0.0) {
	JitterInit(&corePtr->jitter);
    }
    if (corePtr->palette != NULL) {
	Blt_Palette_SetRange(corePtr->palette, 0.0, 1.0);
    }
    corePtr->rRange = corePtr->high.Red   - corePtr->low.Red;
    corePtr->gRange = corePtr->high.Green - corePtr->low.Green;
    corePtr->bRange = corePtr->high.Blue  - corePtr->low.Blue;
    corePtr->aRange = corePtr->high.Alpha - corePtr->low.Alpha;
    switch (gradPtr->type) {
    case BLT_GRADIENT_TYPE_HORIZONTAL:
	gradPtr->scaleFactor = 0.0;
	if (refWidth > 1) {
	    gradPtr->scaleFactor = 1.0 / (refWidth - 1);
	} 
	break;
    default:
    case BLT_GRADIENT_TYPE_VERTICAL:
	gradPtr->scaleFactor = 0.0;
	if (refHeight > 1) {
	    gradPtr->scaleFactor = 1.0 / (refHeight - 1);
	} 
	break;
    case BLT_GRADIENT_TYPE_DIAGONAL_UP:
    case BLT_GRADIENT_TYPE_DIAGONAL_DOWN:
	gradPtr->xOffset = refWidth * 0.5;
	gradPtr->yOffset = refHeight * 0.5;
	gradPtr->length = sqrt(refWidth * refWidth + refHeight * refHeight);
	gradPtr->cosTheta = refWidth / gradPtr->length;
	gradPtr->sinTheta = refHeight / gradPtr->length;
	if (gradPtr->type == BLT_GRADIENT_TYPE_DIAGONAL_DOWN) {
	    gradPtr->sinTheta = -gradPtr->sinTheta;
	}
	gradPtr->scaleFactor = 0.0;
	if (gradPtr->length > 1) {
	    gradPtr->scaleFactor = 1.0 / (gradPtr->length - 1);
	} 
	break;
    case BLT_GRADIENT_TYPE_RADIAL:
	gradPtr->xOffset = refWidth * 0.5;
	gradPtr->yOffset = refHeight * 0.5;
	gradPtr->length = sqrt(refWidth * refWidth + refHeight * refHeight);
	gradPtr->scaleFactor = 0.0;
	if (gradPtr->length > 1) {
	    gradPtr->scaleFactor = 1.0 / ((gradPtr->length * 0.5) - 1);
	} 
	break;
    }
}

static void
InitTexture(TextureBackground *corePtr, int refWidth, int refHeight)
{
    if (corePtr->jitter.range > 0.0) {
	JitterInit(&corePtr->jitter);
    }
    if (corePtr->palette != NULL) {
	Blt_Palette_SetRange(corePtr->palette, 0.0, 1.0);
    }
    corePtr->rRange = corePtr->high.Red   - corePtr->low.Red;
    corePtr->gRange = corePtr->high.Green - corePtr->low.Green;
    corePtr->bRange = corePtr->high.Blue  - corePtr->low.Blue;
    corePtr->aRange = corePtr->high.Alpha - corePtr->low.Alpha;
}

#ifdef notdef
static void 
GetTileOffsets(Tk_Window tkwin, BackgroundObject *corePtr, Blt_Picture picture, 
	       int x, int y, int *xOffsetPtr, int *yOffsetPtr)
{
    int dx, dy;
    int x0, y0;
    int tw, th;
    Tk_Window refWindow;

    if (corePtr->reference == REFERENCE_SELF) {
	refWindow = tkwin;
    } else if (corePtr->reference == REFERENCE_TOPLEVEL) {
	refWindow = Blt_Toplevel(tkwin);
    } else if (corePtr->reference == REFERENCE_WINDOW) {
	refWindow = corePtr->refWindow;
    } else if (corePtr->reference == REFERENCE_NONE) {
	refWindow = NULL;
    } else {
	return;		/* Unknown reference window. */
    }
    if ((corePtr->reference == REFERENCE_WINDOW) ||
	(corePtr->reference == REFERENCE_TOPLEVEL)) {
	Tk_Window tkwin2;
	
	tkwin2 = tkwin;
	while ((tkwin2 != refWindow) && (tkwin2 != NULL)) {
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
	    corePtr->reference = REFERENCE_SELF;
	    refWindow = tkwin;
	    abort();
	}
    }

    x0 = corePtr->xOrigin;
    y0 = corePtr->yOrigin;
    tw = Blt_PictureWidth(picture);
    th = Blt_PictureHeight(picture);

    /* Compute the starting x and y offsets of the tile/gradient from the
     * coordinates of the origin. */
    dx = (x0 - x) % tw;
    if (dx > 0) {
	dx = (tw - dx);
    } else if (dx < 0) {
	dx = x - x0;
    } 
    dy = (y0 - y) % th;
    if (dy > 0) {
	dy = (th - dy);
    } else if (dy < 0) {
	dy = y - y0;
    }
    *xOffsetPtr = dx % tw;
    *yOffsetPtr = dy % th;
#ifdef notdef
    fprintf(stderr, "Tile offsets x0=%d y0=%d x=%d,y=%d sx=%d,sy=%d\n",
	    x0, y0, x, y, *xOffsetPtr, *yOffsetPtr);
#endif
}

static void
Tile(
    Tk_Window tkwin,
    Drawable drawable,
    BackgroundObject *corePtr,
    Blt_Picture picture,		/* Picture used as the tile. */
    int x, int y, int w, int h)		/* Region of destination picture to
					 * be tiled. */
{
    Blt_Painter painter;
    int xOffset, yOffset;		/* Starting upper left corner of
					 * region. */
    int tileWidth, tileHeight;		/* Tile dimensions. */
    int right, bottom, left, top;

    tileWidth = Blt_PictureWidth(picture);
    tileHeight = Blt_PictureHeight(picture);
    GetTileOffsets(tkwin, corePtr, picture, x, y, &xOffset, &yOffset);

#ifdef notdef
    fprintf(stderr, "tile is (xo=%d,yo=%d,tw=%d,th=%d)\n", 
	corePtr->xOrigin, corePtr->yOrigin, tileWidth, tileHeight);
    fprintf(stderr, "region is (x=%d,y=%d,w=%d,h=%d)\n", x, y, w, h);
    fprintf(stderr, "starting offsets at sx=%d,sy=%d\n", xOffset, yOffset);
#endif

    left = x;
    top = y;
    right = x + w;
    bottom = y + h;
    
    painter = Blt_GetPainter(tkwin, 1.0);
    for (y = (top - yOffset); y < bottom; y += tileHeight) {
	int sy, dy, ih;

	if (y < top) {
	    dy = top;
	    ih = MIN(tileHeight - yOffset, bottom - top);
	    sy = yOffset;
	} else {
	    dy = y;
	    ih = MIN(tileHeight, bottom - y);
	    sy = 0;
	}

	for (x = (left - xOffset); x < right; x += tileWidth) {
	    int sx, dx, iw;	

	    if (x < left) {
		dx = left;
		iw = MIN(tileWidth - xOffset, right - left);
		sx = xOffset;
	    } else {
		dx = x;
		iw = MIN(tileWidth, right - x);
		sx = 0;
	    }

	    Blt_PaintPicture(painter, drawable, picture, sx, sy, iw, ih, 
			     dx, dy, /*flags*/0);
#ifdef notdef
	    fprintf(stderr, "drawing background (sx=%d,sy=%d,iw=%d,ih=%d) at dx=%d,dy=%d\n",
		    sx, sy, iw, ih, dx, dy);
#endif
	}
    }
}
#endif

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
 *       tk3d.c --
 *
 *	This module provides procedures to draw borders in
 *	the three-dimensional Motif style.
 *
 *      Copyright (c) 1990-1994 The Regents of the University of California.
 *      Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 *      See the file "license.terms" for information on usage and
 *      redistribution of this file, and for a DISCLAIMER OF ALL
 *      WARRANTIES.
 *
 *  They fix a problem in the Intersect procedure when the polygon is big
 *  (e.q 1600x1200).  The computation overflows the 32-bit integers used.
 */

/*
 *---------------------------------------------------------------------------
 *
 * ShiftLine --
 *
 *	Given two points on a line, compute a point on a new line that is
 *	parallel to the given line and a given distance away from it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
ShiftLine(
    XPoint *p,				/* First point on line. */
    XPoint *q,				/* Second point on line. */
    int distance,			/* New line is to be this many
					 * units to the left of original
					 * line, when looking from p1 to
					 * p2.  May be negative. */
    XPoint *r)				/* Store coords of point on new
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
 *	Find the intersection point between two lines.
 *
 * Results:
 *	Under normal conditions 0 is returned and the point at *iPtr is
 *	filled in with the intersection between the two lines.  If the two
 *	lines are parallel, then -1 is returned and *iPtr isn't modified.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------------
 */
static int
Intersect(a1Ptr, a2Ptr, b1Ptr, b2Ptr, iPtr)
    XPoint *a1Ptr;		/* First point of first line. */
    XPoint *a2Ptr;		/* Second point of first line. */
    XPoint *b1Ptr;		/* First point of second line. */
    XPoint *b2Ptr;		/* Second point of second line. */
    XPoint *iPtr;		/* Filled in with intersection point. */
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
 *	Draw a border with 3-D appearance around the edge of a given
 *	polygon.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information is drawn in "drawable" in the form of a 3-D border
 *	borderWidth units width wide on the left of the trajectory given by
 *	pointPtr and n (or -borderWidth units wide on the right side, if
 *	borderWidth is negative).
 *
 *--------------------------------------------------------------
 */

static void
Draw3DPolygon(
    Tk_Window tkwin,			/* Window for which border was
					   allocated. */
    Drawable drawable,			/* X window or pixmap in which to
					 * draw. */
    Tk_3DBorder border,			/* Token for border to draw. */
    XPoint *points,			/* Array of points describing
					 * polygon.  All points must be
					 * absolute (CoordModeOrigin). */
    int n,				/* Number of points at *points. */
    int borderWidth,			/* Width of border, measured in
					 * pixels to the left of the
					 * polygon's trajectory.  May be
					 * negative. */
    int leftRelief)			/* TK_RELIEF_RAISED or
					 * TK_RELIEF_SUNKEN: indicates how
					 * stuff to left of trajectory
					 * looks relative to stuff on
					 * right. */
{
    XPoint poly[4], b1, b2, newB1, newB2;
    XPoint perp, c, shift1, shift2;	/* Used for handling parallel lines. */
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
     *	   this, compute a new b1 and b2 value for the next
     *	   side of the polygon.
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

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * DestroySolidBackgroundProc --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroySolidBackgroundProc(BackgroundObject *corePtr)
{
}
#endif

static int
ConfigureSolidBackgroundProc(Tcl_Interp *interp, BackgroundObject *basePtr, 
			     int objc, Tcl_Obj *const *objv, unsigned int flags)
{
    SolidBackground *corePtr = (SolidBackground *)basePtr;
    Blt_Pixel color;

    if (Blt_ConfigureWidgetFromObj(interp, corePtr->tkwin, 
	corePtr->classPtr->configSpecs, objc, objv, (char *)corePtr, 
	flags) != TCL_OK) {
	return TCL_ERROR;
    }
    color.u32 = Blt_XColorToPixel(Tk_3DBorderColor(corePtr->border));
    color.Alpha = corePtr->alpha;
    Blt_PaintBrush_SetColor(&corePtr->brush, color.u32);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawSolidRectangle --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawSolidRectangleProc(Tk_Window tkwin, Drawable drawable, 
		       BackgroundObject *basePtr, int x, int y, int w, int h)
{
    SolidBackground *corePtr = (SolidBackground *)basePtr;

    if ((h <= 0) || (w <= 0)) {
	return;
    }
    if (corePtr->alpha == 0xFF) {
	XFillRectangle(Tk_Display(tkwin), drawable, 
		     Tk_3DBorderGC(tkwin, corePtr->border, TK_3D_FLAT_GC),
		     x, y, w, h);
    } else if (corePtr->alpha != 0x00) {
	Blt_Painter painter;
	Blt_Picture picture;
	
	picture = Blt_DrawableToPicture(tkwin, drawable, x, y, w, h, 1.0);
	if (picture == NULL) {
	    return;			/* Background is obscured. */
	}
	Blt_PaintRectangle(picture, 0, 0, w, h, 0, 0, &corePtr->brush);
	painter = Blt_GetPainter(tkwin, 1.0);
	Blt_PaintPicture(painter, drawable, picture, 0, 0, w, h, x, y, 0);
	Blt_FreePicture(picture);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawSolidPolygonProc --
 *
 *	Draw a single color filled polygon.  If the color is completely
 *	opaque, we use the standard XFillPolygon routine.  If the 
 *	color is partially transparent, then we paint the polygon.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawSolidPolygonProc(Tk_Window tkwin, Drawable drawable, 
		     BackgroundObject *basePtr, int n, XPoint *points)
{
    SolidBackground *corePtr = (SolidBackground *)basePtr;

    if (corePtr->alpha == 0xFF) {
	XFillPolygon(Tk_Display(tkwin), drawable, 
		     Tk_3DBorderGC(tkwin, corePtr->border, TK_3D_FLAT_GC),
		     points, n, Complex, CoordModeOrigin);
    } else if (corePtr->alpha != 0x00) {
	Blt_Picture picture;
	Blt_Painter painter;
	int x1, x2, y1, y2;
	int i;
	Point2f *vertices;
	int w, h;

	/* Get polygon bounding box. */
	GetPolygonBBox(points, n, &x1, &x2, &y1, &y2);
	vertices = Blt_AssertMalloc(n * sizeof(Point2f));
	/* Translate the polygon */
	for (i = 0; i < n; i++) {
	    vertices[i].x = (float)(points[i].x - x1);
	    vertices[i].y = (float)(points[i].y - y1);
	}
	/* Grab the background rectangle containing the polygon. */
	w = x2 - x1 + 1;
	h = y2 - y1 + 1;
	picture = Blt_DrawableToPicture(tkwin, drawable, x1, y1, w, h, 1.0);
	if (picture == NULL) {
	    return;			/* Background is obscured. */
	}
	Blt_PaintPolygon(picture, n, vertices, &corePtr->brush);
	Blt_Free(vertices);
	painter = Blt_GetPainter(tkwin, 1.0);
	Blt_PaintPicture(painter, drawable, picture, 0, 0, w, h, x1, y1, 0);
	Blt_FreePicture(picture);
    }
}

static BackgroundClass solidBackgroundClass = {
    BACKGROUND_SOLID,
    solidConfigSpecs,
    NULL,				/* DestroySolidBackgroundProc */
    ConfigureSolidBackgroundProc,
    DrawSolidRectangleProc,
    DrawSolidPolygonProc
};

/*
 *---------------------------------------------------------------------------
 *
 * NewSolidBackground --
 *
 *	Creates a new solid background.
 *
 * Results:
 *	Returns pointer to the new background.
 *
 *---------------------------------------------------------------------------
 */
static BackgroundObject *
NewSolidBackground(void)
{
    SolidBackground *corePtr;

    corePtr = Blt_Calloc(1, sizeof(SolidBackground));
    if (corePtr == NULL) {
	return NULL;
    }
    corePtr->classPtr = &solidBackgroundClass;
    corePtr->alpha = 0xFF;
    return (BackgroundObject *)corePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyTileBackgroundProc --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyTileBackgroundProc(BackgroundObject *basePtr)
{
    TileBackground *corePtr = (TileBackground *)basePtr;

    if ((corePtr->tile != NULL) && (corePtr->flags & FREE_TILE)) {
	Blt_FreePicture(corePtr->tile);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawTileRectangleProc --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawTileRectangleProc(Tk_Window tkwin, Drawable drawable, 
		      BackgroundObject *basePtr, int x, int y, int w, int h)
{
    TileBackground *corePtr = (TileBackground *)basePtr;
    Blt_Painter painter;

    if ((h <= 0) || (w <= 0)) {
	return;
    }
    if (corePtr->tkImage == NULL) {
	/* No image so draw solid color background using border. */
	Tk_Fill3DRectangle(tkwin, drawable, corePtr->border, x, y, w, h,
		0, TK_RELIEF_FLAT);
	return;
    }
    if (corePtr->flags & BG_SCALE) {
	Blt_Picture picture;
	int refWidth, refHeight;
	Blt_HashEntry *hPtr;

	hPtr = NULL;
	picture = NULL;
	refWidth = w, refHeight = h;
	if (corePtr->reference != REFERENCE_NONE) {
	    int isNew;

	    /* See if a picture has previously been generated. There will
	     * be a picture for each reference window. */
	    hPtr = Blt_CreateHashEntry(&corePtr->pictTable, 
		(char *)corePtr->refWindow, &isNew);
	    if (!isNew) {
		picture = Blt_GetHashValue(hPtr);
	    } 
	    refWidth = Tk_Width(corePtr->refWindow);
	    refHeight = Tk_Height(corePtr->refWindow);
	}
	if ((picture == NULL) || 
	    (Blt_PictureWidth(picture) != refWidth) ||
	    (Blt_PictureHeight(picture) != refHeight)) {
	    
	    /* 
	     * Either the size of the reference window has changed or one
	     * of the background options has been reset. Resize the picture
	     * if necessary and regenerate the background.
	     */
	    if (picture == NULL) {
		picture = Blt_CreatePicture(refWidth, refHeight);
		if (hPtr != NULL) {
		    Blt_SetHashValue(hPtr, picture);
		}
	    } else {
		Blt_ResizePicture(picture, refWidth, refHeight);
	    }
	    if (corePtr->tile != NULL) {
		Blt_ResamplePicture(picture, corePtr->tile, 
			corePtr->filter, corePtr->filter);
	    }
	}
	painter = Blt_GetPainter(tkwin, 1.0);
	Blt_PaintPicture(painter, drawable, picture, 0, 0, w, h, x, y, 0);
    } else {
	Blt_Painter painter;
	Blt_Picture bg;
	int xOffset, yOffset;		/* Starting upper left corner of
					 * region. */

	xOffset = yOffset = 0;		/* Suppress compiler warning. */
	if (corePtr->alpha != 0xFF) {
	    bg = Blt_DrawableToPicture(tkwin, drawable, x, y, w, h, 1.0);
	} else {
	    bg = Blt_CreatePicture(w, h);
	}
	if (bg == NULL) {
	    return;			/* Background is obscured. */
	}
	GetOffsets(tkwin, basePtr, x, y, &xOffset, &yOffset);
	Blt_PaintBrush_SetOrigin(&corePtr->brush, xOffset, yOffset);
	Blt_PaintRectangle(bg, 0, 0, w, h, 0, 0, &corePtr->brush);
	painter = Blt_GetPainter(tkwin, 1.0);
	Blt_PaintPicture(painter, drawable, bg, 0, 0, w, h, x, y, 0);
	Blt_FreePicture(bg);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawTilePolygonProc --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawTilePolygonProc(Tk_Window tkwin, Drawable drawable, 
		    BackgroundObject *basePtr, int n, XPoint *points)
{
    TileBackground *corePtr = (TileBackground *)basePtr;
    int xOffset, yOffset;		/* Starting upper left corner of
					 * region. */
    Blt_Picture bg;
    Blt_Painter painter;
    int i;
    int w, h;
    int x1, x2, y1, y2;
    Point2f *vertices;
    int refWidth, refHeight;

    xOffset = yOffset = 0; 		/* Suppress compiler warning. */
    /* Grab the rectangular background that covers the polygon. */
    GetPolygonBBox(points, n, &x1, &x2, &y1, &y2);
    w = x2 - x1 + 1;
    h = y2 - y1 + 1;
    bg = Blt_DrawableToPicture(tkwin, drawable, x1, y1, w, h, 1.0);
    if (bg == NULL) {
	return;				/* Background is obscured. */
    }
    vertices = Blt_AssertMalloc(n * sizeof(Point2f));
    /* Translate the polygon */
    for (i = 0; i < n; i++) {
	vertices[i].x = (float)(points[i].x - x1);
	vertices[i].y = (float)(points[i].y - y1);
    }
    GetReferenceWindow(basePtr, tkwin, &refWidth, &refHeight);
    GetOffsets(tkwin, basePtr, x1, y1, &xOffset, &yOffset);
    Blt_PaintBrush_SetOrigin(&corePtr->brush, xOffset, yOffset);
    Blt_PaintPolygon(bg, n, vertices, &corePtr->brush);
    Blt_Free(vertices);
    painter = Blt_GetPainter(tkwin, 1.0);
    Blt_PaintPicture(painter, drawable, bg, 0, 0, w, h, x1, y1, 0);
    Blt_FreePicture(bg);
}

static int
ConfigureTileBackgroundProc(Tcl_Interp *interp, BackgroundObject *basePtr, 
			    int objc, Tcl_Obj *const *objv, unsigned int flags)
{
    TileBackground *corePtr = (TileBackground *)basePtr;

    if (Blt_ConfigureWidgetFromObj(interp, corePtr->tkwin, 
	corePtr->classPtr->configSpecs, objc, objv, (char *)corePtr, 
	flags) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((corePtr->tile != NULL) && (corePtr->flags & FREE_TILE)) {
	Blt_FreePicture(corePtr->tile);
    }
    corePtr->tile = ImageToPicture(corePtr);
    corePtr->brush.alpha = corePtr->alpha;
    Blt_PaintBrush_SetTile(&corePtr->brush, corePtr->tile);
    return TCL_OK;
}

static BackgroundClass tileBackgroundClass = {
    BACKGROUND_TILE,
    tileConfigSpecs,
    DestroyTileBackgroundProc,
    ConfigureTileBackgroundProc,
    DrawTileRectangleProc,		/* DrawRectangleProc */
    DrawTilePolygonProc			/* DrawPolygonProc */
};

/*
 *---------------------------------------------------------------------------
 *
 * NewTileBackground --
 *
 *	Creates a new image background.
 *
 * Results:
 *	Returns pointer to the new background.
 *
 *---------------------------------------------------------------------------
 */
static BackgroundObject *
NewTileBackground(void)
{
    TileBackground *corePtr;

    corePtr = Blt_Calloc(1, sizeof(TileBackground));
    if (corePtr == NULL) {
	return NULL;
    }
    corePtr->classPtr = &tileBackgroundClass;
    corePtr->reference = REFERENCE_TOPLEVEL;
    corePtr->alpha = 0xFF;
    return (BackgroundObject *)corePtr;
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * DestroyGradientBackgroundProc --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyGradientBackgroundProc(BackgroundObject *corePtr)
{
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * DrawGradientRectangleProc --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawGradientRectangleProc(Tk_Window tkwin, Drawable drawable, 
			  BackgroundObject *basePtr, int x, int y, int w, int h)
{
    GradientBackground *corePtr = (GradientBackground *)basePtr;
    Blt_Painter painter;
    Tk_Window refWindow;
    Blt_Picture bg;
    int xOffset, yOffset;		/* Starting upper left corner of
					 * region. */
    int refWidth, refHeight;

    xOffset = yOffset = 0;		/* Suppress compiler warning. */
    if ((h <= 0) || (w <= 0)) {
	return;
    }
    refWindow = GetReferenceWindow(basePtr, tkwin, &refWidth, &refHeight);
    if (refWindow == NULL) {
	return;				/* Doesn't refer to reference. */
    }
    if (corePtr->alpha != 0xFF) {
	bg = Blt_DrawableToPicture(tkwin, drawable, x, y, w, h, 1.0);
    } else {
	bg = Blt_CreatePicture(w, h);
    }
    if (bg == NULL) {
	return;                         /* Background is obscured. */
    }
    GetOffsets(tkwin, basePtr, x, y, &xOffset, &yOffset);
    InitGradient(corePtr, refWidth, refHeight);
    Blt_PaintBrush_SetOrigin(&corePtr->brush, xOffset, yOffset);
    Blt_PaintRectangle(bg, 0, 0, w, h, 0, 0, &corePtr->brush);
    painter = Blt_GetPainter(tkwin, 1.0);
    Blt_PaintPicture(painter, drawable, bg, 0, 0, w, h, x, y, 0);
    Blt_FreePicture(bg);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawGradientPolygonProc --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawGradientPolygonProc(Tk_Window tkwin, Drawable drawable, 
			BackgroundObject *basePtr, int n, XPoint *points)
{
    GradientBackground *corePtr = (GradientBackground *)basePtr;
    int xOffset, yOffset;		/* Starting upper left corner of
					 * region. */
    Blt_Picture bg;
    Blt_Painter painter;
    int i;
    int w, h;
    int x1, x2, y1, y2;
    Point2f *vertices;
    int refWidth, refHeight;

    /* Grab the rectangular background that covers the polygon. */
    GetPolygonBBox(points, n, &x1, &x2, &y1, &y2);
    w = x2 - x1 + 1;
    h = y2 - y1 + 1;
    bg = Blt_DrawableToPicture(tkwin, drawable, x1, y1, w, h, 1.0);
    if (bg == NULL) {
	return;				/* Background is obscured. */
    }
    vertices = Blt_AssertMalloc(n * sizeof(Point2f));
    /* Translate the polygon */
    for (i = 0; i < n; i++) {
	vertices[i].x = (float)(points[i].x - x1);
	vertices[i].y = (float)(points[i].y - y1);
    }
    GetReferenceWindow(basePtr, tkwin, &refWidth, &refHeight);
    InitGradient(corePtr, refWidth, refHeight);
    GetOffsets(tkwin, basePtr, x1, y1, &xOffset, &yOffset);
    Blt_PaintPolygon(bg, n, vertices, &corePtr->brush);
    Blt_Free(vertices);
    painter = Blt_GetPainter(tkwin, 1.0);
    Blt_PaintPicture(painter, drawable, bg, 0, 0, w, h, x1, y1, 0);
    Blt_FreePicture(bg);
}

static int
ConfigureGradientBackgroundProc(Tcl_Interp *interp, BackgroundObject *basePtr, 
				int objc, Tcl_Obj *const *objv, 
				unsigned int flags)
{
    GradientBackground *corePtr = (GradientBackground *)basePtr;

    if (Blt_ConfigureWidgetFromObj(interp, corePtr->tkwin, 
	corePtr->classPtr->configSpecs, objc, objv, (char *)corePtr, 
	flags) != TCL_OK) {
	return TCL_ERROR;
    }
    if (corePtr->alpha != 0xFF) {
	corePtr->low.Alpha = corePtr->alpha;
	corePtr->high.Alpha = corePtr->alpha;
    }
    corePtr->brush.alpha = corePtr->alpha;
    Blt_PaintBrush_SetColorProc(&corePtr->brush, GradientColorProc, 
	corePtr);
    return TCL_OK;
}


static BackgroundClass gradientBackgroundClass = {
    BACKGROUND_GRADIENT,
    gradientConfigSpecs,
    NULL,				/* DestroyGradientBackgroundProc, */
    ConfigureGradientBackgroundProc, 
    DrawGradientRectangleProc,		/* DrawRectangleProc */
    DrawGradientPolygonProc,		/* DrawPolygonProc */
};

/*
 *---------------------------------------------------------------------------
 *
 * NewGradientBackground --
 *
 *	Creates a new gradient background.
 *
 * Results:
 *	Returns pointer to the new background.
 *
 *---------------------------------------------------------------------------
 */
static BackgroundObject *
NewGradientBackground(void)
{
    GradientBackground *corePtr;

    corePtr = Blt_Calloc(1, sizeof(GradientBackground));
    if (corePtr == NULL) {
	return NULL;
    }
    corePtr->classPtr = &gradientBackgroundClass;
    corePtr->reference = REFERENCE_TOPLEVEL;
    corePtr->gradient.type = BLT_GRADIENT_TYPE_VERTICAL;
    corePtr->gradient.scale = BLT_GRADIENT_SCALE_LINEAR;
    corePtr->alpha = 255;
    return (BackgroundObject *)corePtr;
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * DestroyTextureBackgroundProc --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyTextureBackgroundProc(BackgroundObject *corePtr)
{
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * DrawTextureRectangleProc --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawTextureRectangleProc(Tk_Window tkwin, Drawable drawable, 
			 BackgroundObject *basePtr, int x, int y, int w, int h)
{
    TextureBackground *corePtr = (TextureBackground *)basePtr;
    Blt_Painter painter;
    Tk_Window refWindow;
    Blt_Picture dest;
    int xOffset, yOffset;		/* Starting upper left corner of
					 * region. */
    int refWidth, refHeight;

    if ((h <= 0) || (w <= 0)) {
	return;
    }
    refWindow = GetReferenceWindow(basePtr, tkwin, &refWidth, &refHeight);
    if (refWindow == NULL) {
	return;				/* Doesn't refer to reference. */
    }
    if (corePtr->alpha != 0xFF) {
	dest = Blt_DrawableToPicture(tkwin, drawable, x, y, w, h, 1.0);
    } else {
	dest = Blt_CreatePicture(w, h);
    }
    if (dest == NULL) {
	return;				/* Background is obscured. */
    }
    GetOffsets(tkwin, basePtr, x, y, &xOffset, &yOffset);
    InitTexture(corePtr, refWidth, refHeight);
    Blt_PaintRectangle(dest, 0, 0, w, h, 0, 0, &corePtr->brush);
    painter = Blt_GetPainter(tkwin, 1.0);
    Blt_PaintPicture(painter, drawable, dest, 0, 0, w, h, x, y, 0);
    Blt_FreePicture(dest);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawTexturePolygonProc --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawTexturePolygonProc(Tk_Window tkwin, Drawable drawable, 
		       BackgroundObject *basePtr, int n, XPoint *points)
{
    TextureBackground *corePtr = (TextureBackground *)basePtr;
    int xOffset, yOffset;		/* Starting upper left corner of
					 * region. */
    Blt_Picture dest;
    Blt_Painter painter;
    int i;
    int w, h;
    int x1, x2, y1, y2;
    Point2f *vertices;
    int refWidth, refHeight;

    /* Grab the rectangular background that covers the polygon. */
    GetPolygonBBox(points, n, &x1, &x2, &y1, &y2);
    w = x2 - x1 + 1;
    h = y2 - y1 + 1;
    dest = Blt_DrawableToPicture(tkwin, drawable, x1, y1, w, h, 1.0);
    if (dest == NULL) {
	return;				/* Background is obscured. */
    }
    vertices = Blt_AssertMalloc(n * sizeof(Point2f));
    /* Translate the polygon */
    for (i = 0; i < n; i++) {
	vertices[i].x = (float)(points[i].x - x1);
	vertices[i].y = (float)(points[i].y - y1);
    }
    GetReferenceWindow(basePtr, tkwin, &refWidth, &refHeight);
    InitTexture(corePtr, refWidth, refHeight);
    GetOffsets(tkwin, basePtr, x1, y1, &xOffset, &yOffset);
    Blt_PaintPolygon(dest, n, vertices, &corePtr->brush);
    Blt_Free(vertices);
    painter = Blt_GetPainter(tkwin, 1.0);
    Blt_PaintPicture(painter, drawable, dest, 0, 0, w, h, x1, y1, 0);
    Blt_FreePicture(dest);
}

static int
ConfigureTextureBackgroundProc(Tcl_Interp *interp, BackgroundObject *basePtr, 
			       int objc, Tcl_Obj *const *objv, 
			       unsigned int flags)
{
    TextureBackground *corePtr = (TextureBackground *)basePtr;

    if (Blt_ConfigureWidgetFromObj(interp, corePtr->tkwin, 
	corePtr->classPtr->configSpecs, objc, objv, (char *)corePtr, 
	flags) != TCL_OK) {
	return TCL_ERROR;
    }
    corePtr->brush.alpha = corePtr->alpha;
    Blt_PaintBrush_SetColorProc(&corePtr->brush, TextureColorProc, corePtr);
    return TCL_OK;
}

static BackgroundClass textureBackgroundClass = {
    BACKGROUND_TEXTURE,
    textureConfigSpecs,			
    NULL,				/* DestroyTextureBackgroundProc, */
    ConfigureTextureBackgroundProc,	
    DrawTextureRectangleProc,		
    DrawTexturePolygonProc		
};

/*
 *---------------------------------------------------------------------------
 *
 * NewTextureBackground --
 *
 *	Creates a new texture background.
 *
 * Results:
 *	Returns pointer to the new background.
 *
 *---------------------------------------------------------------------------
 */
static BackgroundObject *
NewTextureBackground()
{
    TextureBackground *corePtr;

    corePtr = Blt_Calloc(1, sizeof(TextureBackground));
    if (corePtr == NULL) {
	return NULL;
    }
    corePtr->classPtr = &textureBackgroundClass;
    corePtr->reference = REFERENCE_TOPLEVEL;
    corePtr->alpha = 0xFF;
    return (BackgroundObject *)corePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateBackground --
 *
 *	Creates a new background.
 *
 * Results:
 *	Returns pointer to the new background.
 *
 *---------------------------------------------------------------------------
 */
static BackgroundObject *
CreateBackground(BackgroundInterpData *dataPtr, Tcl_Interp *interp, 
		 BackgroundType type)
{
    BackgroundObject *corePtr;

    switch (type) {
    case BACKGROUND_SOLID:
	corePtr = NewSolidBackground();
	break;
    case BACKGROUND_TILE:
	corePtr = NewTileBackground();
	break;
    case BACKGROUND_GRADIENT:
	corePtr = NewGradientBackground();
	break;
    case BACKGROUND_TEXTURE:
	corePtr = NewTextureBackground();
	break;
    default:
	abort();
	break;
    }
    if (corePtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate background", (char *)NULL);
	return NULL;
    }
    corePtr->dataPtr = dataPtr;
    Blt_InitHashTable(&corePtr->pictTable, BLT_ONE_WORD_KEYS);
    corePtr->chain = Blt_Chain_Create();
    corePtr->tkwin = Tk_MainWindow(interp);
    corePtr->display = Tk_Display(corePtr->tkwin);
    Blt_PaintBrush_Init(&corePtr->brush);
    return corePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyBackgroundObject --
 *
 *	Removes the client from the servers's list of clients and memory used
 *	by the client token is released.  When the last client is deleted, the
 *	server is also removed.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyBackgroundObject(BackgroundObject *corePtr)
{
    Blt_FreeOptions(corePtr->classPtr->configSpecs, (char *)corePtr, 
	corePtr->display, 0);
    if (corePtr->classPtr->destroyProc != NULL) {
	(*corePtr->classPtr->destroyProc)(corePtr);
    }
    if (corePtr->border != NULL) {
	Tk_Free3DBorder(corePtr->border);
    }
    if (corePtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&corePtr->dataPtr->instTable, 
		corePtr->hashPtr);
    }
    ClearCache(corePtr);
    Blt_Chain_Destroy(corePtr->chain);
    Blt_DeleteHashTable(&corePtr->pictTable);
    Blt_Free(corePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyBackground --
 *
 *	Removes the client from the servers's list of clients and memory
 *	used by the client token is released.  When the last client is
 *	deleted, the server is also removed.
 *
 * Results:
 *	None.
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
 *	Retrieves the background named by the given the Tcl_Obj.
 *
 * Results:
 *	None.
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
 * background create type ?option values?...
 */
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    Bg *bgPtr;
    BackgroundInterpData *dataPtr = clientData;
    BackgroundObject *corePtr;
    BackgroundType type;

    if (GetBackgroundTypeFromObj(interp, objv[2], &type) != TCL_OK) {
	return TCL_ERROR;
    }
    corePtr = CreateBackground(dataPtr, interp, type);
    if (corePtr == NULL) {
	return TCL_ERROR;
    }
    if ((*corePtr->classPtr->configProc)(interp, corePtr, objc - 3, objv + 3, 
	0) != TCL_OK) {
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
    /* Generate a unique name for the background.  */
    {
	int isNew;
	char name[200];
	Blt_HashEntry *hPtr;

	do {
	    Blt_FormatString(name, 200, "background%d", dataPtr->nextId++);
	    hPtr = Blt_CreateHashEntry(&dataPtr->instTable, name, &isNew);
	} while (!isNew);
	assert(hPtr != NULL);
	assert(corePtr != NULL);
	Blt_SetHashValue(hPtr, corePtr);
	corePtr->hashPtr = hPtr;
	corePtr->name = Blt_GetHashKey(&dataPtr->instTable, hPtr);
    }

    /* Add the container to the background object's list of clients. */
    bgPtr->link = Blt_Chain_Append(corePtr->chain, bgPtr);
    corePtr->link = bgPtr->link;
    bgPtr->corePtr = corePtr;
    Tcl_SetStringObj(Tcl_GetObjResult(interp), corePtr->name, -1);
    return TCL_OK;
}    

/*
 * background cget $bg ?option?...
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
    return Blt_ConfigureValueFromObj(interp, corePtr->tkwin, 
	corePtr->classPtr->configSpecs, (char *)corePtr, objv[3], 0);
}

/*
 * background configure $bg ?option?...
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
	return Blt_ConfigureInfoFromObj(interp, corePtr->tkwin, 
		corePtr->classPtr->configSpecs, (char *)corePtr, 
		(Tcl_Obj *)NULL, flags);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, corePtr->tkwin, 
		corePtr->classPtr->configSpecs, (char *)corePtr, objv[3], 
		flags);
    } else {
	if ((*corePtr->classPtr->configProc)(interp, corePtr, 
		objc-3, objv+3, flags) != TCL_OK) {
	    return TCL_ERROR;
	}
	ClearCache(corePtr);
	NotifyClients(corePtr);
	return TCL_OK;
    }
}

/*
 * background delete $bg... 
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
	    corePtr->link = NULL;	/* Disconnect background. */
	    DestroyBackground(bgPtr);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamesOp --
 *
 *	paint names ?pattern?
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
	Tcl_Obj *objPtr;
	
	corePtr = Blt_GetHashValue(hPtr);
	if (objc == 3) {
	    if (!Tcl_StringMatch(corePtr->name, Tcl_GetString(objv[2]))) {
		continue;
	    }
	}
	objPtr = Tcl_NewStringObj(corePtr->name, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 * background type $bg
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
    Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	NameOfBackgroundType(corePtr), -1);
    return TCL_OK;
}

static Blt_OpSpec backgroundOps[] =
{
    {"cget",      2, CgetOp,      4, 4, "bgName option",},
    {"configure", 2, ConfigureOp, 3, 0, "bgName ?option value?...",},
    {"create",    2, CreateOp,    3, 0, "type ?args?",},
    {"delete",    1, DeleteOp,    2, 0, "bgName...",},
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
	 * (clients of the background). */

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
 *	Creates a new background from the given description.  The
 *	background structure returned is a token for the client to use the
 *	background.  If the background isn't a solid background (i.e. a
 *	solid color that Tk_Get3DBorder will accept) then the background
 *	must already exist.  Solid backgrounds are the exception to this
 *	rule.  This lets "-background #ffffff" work without already having
 *	allocated a background "#ffffff".
 *
 * Results:
 *	Returns a background token.
 *
 * Side Effects:
 *	Memory is allocated for the new token.
 *
 *---------------------------------------------------------------------------
 */
Blt_Bg
Blt_GetBg(Tcl_Interp *interp, Tk_Window tkwin, const char *name)
{
    BackgroundObject *corePtr;
    BackgroundInterpData *dataPtr;
    Bg *bgPtr;				/* BackgroundObject container. */
    Blt_HashEntry *hPtr;
    int isNew;
    
    /* Create new token for the background. */
    bgPtr = Blt_Calloc(1, sizeof(Bg));
    if (bgPtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate background \"", name, "\".", 
		(char *)NULL);
	return NULL;
    }
    dataPtr = GetBackgroundInterpData(interp);
    hPtr = Blt_CreateHashEntry(&dataPtr->instTable, name, &isNew);
    if (isNew) {
	Tk_3DBorder border;

	/* BackgroundObject doesn't already exist, see if it's a color name
	 * (i.e. something that Tk_Get3DBorder will accept). */
	border = Tk_Get3DBorder(interp, tkwin, name);
	if (border == NULL) {
	    goto error;			/* Nope. It's an error. */
	} 
	corePtr = CreateBackground(dataPtr, interp, BACKGROUND_SOLID);
	if (corePtr == NULL) {
	    Tk_Free3DBorder(border);
	    goto error;			/* Can't allocate new background. */
	}
        if (corePtr->border != NULL) {
	    Tk_Free3DBorder(corePtr->border);
        }
        corePtr->border = border;
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
    return bgPtr;
 error:
    Blt_Free(bgPtr);
    Blt_DeleteHashEntry(&dataPtr->instTable, hPtr);
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetBgFromObj
 *
 *	Retrieves a new token of a background from the named background.
 *
 * Results:
 *	Returns a background token.
 *
 * Side Effects:
 *	Memory is allocated for the new token.
 *
 *---------------------------------------------------------------------------
 */
Blt_Bg
Blt_GetBgFromObj(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *objPtr)
{
    return Blt_GetBg(interp, tkwin, Tcl_GetString(objPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_SetChangedProc
 *
 *	Sets the routine to called when an image changes.  
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The designated routine will be called the next time the image
 *	associated with the tile changes.
 *
 *---------------------------------------------------------------------------
 */
/*LINTLIBRARY*/
void
Blt_Bg_SetChangedProc(
    Bg *bgPtr,				/* Background with which to
					 * register callback. */
    Blt_Bg_ChangedProc *notifyProc,	/* Function to call when background
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
 * Blt_FreeBg
 *
 *	Removes the background token.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Memory is freed.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_FreeBg(Bg *bgPtr)
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
 *	Returns the coordinates of the origin of the background referenced
 *	by the token.
 *
 * Results:
 *	Returns the coordinates of the origin.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_GetOrigin(Bg *bgPtr, int *xPtr, int *yPtr)
{
    *xPtr = bgPtr->corePtr->xOrigin;
    *yPtr = bgPtr->corePtr->yOrigin;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_SetOrigin
 *
 *	Sets the origin of the background referenced by the token.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_SetOrigin(Tk_Window tkwin, Bg *bgPtr, int x, int y)
{
    BackgroundObject *corePtr = bgPtr->corePtr;
    corePtr->xOrigin = x;
    corePtr->yOrigin = y;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_Name
 *
 *	Returns the name of the core background referenced by the
 *	token.
 *
 * Results:
 *	Return the name of the background.
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
 *	Returns the border color of the background referenced by the token.
 *
 * Results:
 *	Returns the XColor representing the border color of the background.
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
 *	Returns the border of the background referenced by the token.
 *
 * Results:
 *	Return the border of the background.
 *
 *---------------------------------------------------------------------------
 */
Tk_3DBorder
Blt_Bg_Border(Bg *bgPtr)
{
    return bgPtr->corePtr->border;
}

Blt_PaintBrush *
Blt_Bg_PaintBrush(Bg *bgPtr)
{
    return &bgPtr->corePtr->brush;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_DrawRectangle
 *
 *	Draws the background in the designated window.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_DrawRectangle(Tk_Window tkwin, Drawable drawable, Bg *bgPtr, int x, 
		     int y, int w, int h, int borderWidth, int relief)
{
    Tk_Draw3DRectangle(tkwin, drawable, bgPtr->corePtr->border, x, y, w, h, 
	borderWidth, relief);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_FillRectangle
 *
 *	Draws the background in the designated window.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_FillRectangle(Tk_Window tkwin, Drawable drawable, Bg *bgPtr, int x, 
		     int y, int w, int h, int borderWidth, int relief)
{
    BackgroundObject *corePtr;

    if ((h < 1) || (w < 1)) {
	return;
    }
    corePtr = bgPtr->corePtr;
    (*corePtr->classPtr->drawRectangleProc)(tkwin, drawable, corePtr, 
	x, y, w, h);
    if ((relief != TK_RELIEF_FLAT) && (borderWidth > 0)) {
	Tk_Draw3DRectangle(tkwin, drawable, corePtr->border, x, y, w, h,
		borderWidth, relief);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_DrawPolygon
 *
 *	Draws the background in the designated window.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_DrawPolygon(Tk_Window tkwin, Drawable drawable, Bg *bgPtr, 
		   XPoint *points, int n, int borderWidth, int relief)
{
    BackgroundObject *corePtr;

#ifdef notdef
    if (n < 3) {
	return;
    }
#endif
    corePtr = bgPtr->corePtr;
    Draw3DPolygon(tkwin, drawable, corePtr->border, points, n, borderWidth, 
	relief);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_FillPolygon
 *
 *	Draws the background in the designated window.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_FillPolygon(Tk_Window tkwin, Drawable drawable, Bg *bgPtr, 
		   XPoint *points, int n, int borderWidth, int relief)
{
    BackgroundObject *corePtr;

    if (n < 3) {
	return;
    }
    corePtr = bgPtr->corePtr;
    (*corePtr->classPtr->drawPolygonProc)(tkwin, drawable, corePtr, 
	n, points);
    if ((relief != TK_RELIEF_FLAT) && (borderWidth != 0)) {
	Draw3DPolygon(tkwin, drawable, corePtr->border, points, n,
		borderWidth, relief);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Bg_DrawFocus
 *
 *	Draws the background in the designated picture.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_Bg_DrawFocus(Tk_Window tkwin, Bg *bgPtr, int highlightThickness, 
		 Drawable drawable)
{
    BackgroundObject *corePtr = (BackgroundObject *)bgPtr->corePtr;
    int w, h;

    w = Tk_Width(tkwin);
    h = Tk_Height(tkwin);
    /* Top */
    (*corePtr->classPtr->drawRectangleProc)(tkwin, drawable, corePtr, 
	0, 0, w, highlightThickness);
    /* Bottom */
    (*corePtr->classPtr->drawRectangleProc)(tkwin, drawable, corePtr, 
	0, h - highlightThickness, w, highlightThickness);
    /* Left */
    (*corePtr->classPtr->drawRectangleProc)(tkwin, drawable, corePtr,
	0, highlightThickness, highlightThickness, h - 2 * highlightThickness);
    /* Right */
    (*corePtr->classPtr->drawRectangleProc)(tkwin, drawable, corePtr,
	w - highlightThickness, highlightThickness, highlightThickness, 
	h - 2 * highlightThickness);
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
Blt_Bg_SetClipRegion(Tk_Window tkwin, Bg *bgPtr, TkRegion rgn)
{
    Blt_Painter painter;
    Display *display;
    GC gc;

    display = Tk_Display(tkwin);
    gc = Tk_3DBorderGC(tkwin, bgPtr->corePtr->border, TK_3D_LIGHT_GC);
    TkSetRegion(display, gc, rgn);
    gc = Tk_3DBorderGC(tkwin, bgPtr->corePtr->border, TK_3D_DARK_GC);
    TkSetRegion(display, gc, rgn);
    gc = Tk_3DBorderGC(tkwin, bgPtr->corePtr->border, TK_3D_FLAT_GC);
    TkSetRegion(display, gc, rgn);
    painter = Blt_GetPainter(tkwin, 1.0);
    gc = Blt_PainterGC(painter);
    TkSetRegion(display, gc, rgn);
}

void
Blt_Bg_UnsetClipRegion(Tk_Window tkwin, Bg *bgPtr)
{
    Blt_Painter painter;
    Display *display;
    GC gc;

    display = Tk_Display(tkwin);
    gc = Tk_3DBorderGC(tkwin, bgPtr->corePtr->border, TK_3D_LIGHT_GC);
    XSetClipMask(display, gc, None);
    gc = Tk_3DBorderGC(tkwin, bgPtr->corePtr->border, TK_3D_DARK_GC);
    XSetClipMask(display, gc, None);
    gc = Tk_3DBorderGC(tkwin, bgPtr->corePtr->border, TK_3D_FLAT_GC);
    XSetClipMask(display, gc, None);
    painter = Blt_GetPainter(tkwin, 1.0);
    gc = Blt_PainterGC(painter);
    XSetClipMask(display, gc, None);
}

#ifdef notdef
void
Blt_Bg_GetType(Bg *bgPtr)
{
    return bgPtr->reference;
}

Blt_Gradient
Blt_Bg_GetGradient(Bg *bgPtr)
{
    GradientBackground *corePtr = (GradientBackground *)bgPtr->corePtr;
    return bgPtr->gradient;
}

void
Blt_Bg_SetColorProc(Bg *bgPtr, Blt_BgColorProc *proc, ClientData clientData)
{
    bgPtr->colorProc = proc;
    bgPtr->clientData = clientData;
}

#endif

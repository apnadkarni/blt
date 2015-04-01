/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPaint.c --
 *
 * This module creates paintbrush objects for the BLT toolkit.
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

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltConfig.h"
#include "bltPalette.h"
#include "bltPicture.h"
#include "bltPictInt.h"
#include "bltImage.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define JCLAMP(c)	((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))

#define PAINTBRUSH_THREAD_KEY	"BLT PaintBrush Data"

/* 
   paintbrush create pattern 
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

  paintbrush create tile 
	-relativeto self|toplevel|window
	-image $image
	-bg $color
  paintbrush create picture 
	-image $image
	-filter $filterName
	-bg $color
  paintbrush create gradient 
	-type radial|horizontal|vertical|updiagonal|downdiagonal
	-low $color
	-high $color
	-bg $color
	-palette palette 

  paintbrush create newgradient 
        -type linear|radial|conical
	-border colorName
	-palette paletteName
        -opacity percent
        -jitter percent
        -scale log|linear|atan
	-startcolor colorName
	-endcolor colorName
        -decreasing yes|no 
      linear 
        -startposition nw|top left|0 0
	-endposition sw|bottom right|1 1 
        -angle numDegrees
        -repeat yes|no|reverse
      radial
        -centerposition nw|top left|0 0
        -shape circle|ellipse|x1 y1 x2 y2 
        -gravity 
      conical
        -centerposition nw|top left|0 0
         
  paintbrush create border 
	-bg $color
	-alpha $color 

  paintbrush create texture -type metal|wind|??? 
	-bg $color

  paintbrush names
  paintbrush configure $tile
  paintbrush delete $tile
*/

/* 
 * Types of paintbrushes: "solid", "tile", "gradient", "texture"
 */
typedef enum PaintBrushTypes {
    PAINTBRUSH_GRADIENT,		/* Color gradient. */
    PAINTBRUSH_TILE,			/* Tiled image. */
    PAINTBRUSH_SOLID,			/* Solid color. */
    PAINTBRUSH_TEXTURE,			/* Procedural texture. */
    PAINTBRUSH_LINEAR_GRADIENT,		/* Color gradient. */
    PAINTBRUSH_RADIAL_GRADIENT,		/* Color gradient. */
    PAINTBRUSH_CONICAL_GRADIENT,        /* Color gradient. */
    PAINTBRUSH_CHECKERS_TEXTURE,        /* Color gradient. */
    PAINTBRUSH_VERTICALSTRIPES_TEXTURE, /* Color gradient. */
    PAINTBRUSH_HORIZONTALSTRIPES_TEXTURE, /* Color gradient. */
} PaintBrushType;

static const char *paintbrushTypes[] = {
    "gradient",
    "tile",
    "solid",
    "texture"
    "lineargradient",
    "radialgradient",
    "conicalgradient",
    "checkerstexture",
    "verticalstripestexture",
    "horizontalstripestexture",
};

typedef struct {
    Blt_HashTable instTable;		/* Hash table of paintbrush
					 * structures keyed by the name. */
    Tcl_Interp *interp;			/* Interpreter associated with this
					 * set of background paints. */
    int nextId;				/* Serial number of the identifier
					 * to be used for next paintbrush
					 * created.  */
} PaintBrushCmdInterpData;

typedef struct {
    Blt_PaintBrush brush;
    Blt_HashEntry *hashPtr;		/* Hash entry of this brush in
					 * interpreter-specific paintbrush
					 * table. */
    const char *name;			/* Name of paintbrush. Points to
					 * hashtable key. */
    int refCount;			/* # of clients using this brush.
					 * If zero, this brush can be
					 * deleted. */
    PaintBrushCmdInterpData *dataPtr;	/* Pointer to interpreter-specific
					 * data. */
    Blt_ConfigSpec *configSpecs;	/* Configuration specifications for 
					 * this type of brush.  */
    Tk_Window tkwin;			/* Main window. Used to query
					 * background pattern options. */
    Display *display;			/* Display of this paintbrush. Used
					 * to free switches. */
    unsigned int flags;			/* See definitions below. */
    Tk_Image tkImage;			/* Tk image used for tiling (tile
					 * brushes only). */
} PaintBrushCmd;

#define FREE_PICTURE		(1<<0)  /* Indicates we need to free the
					 * temporary picture (associated
					 * with the Tk image to be tiled)
					 * when the picture is no longer in
					 * use.  This happens when the
					 * picture is converted from a Tk
					 * photo image. */

#define DEF_OPACITY		"100.0"
#define DEF_ORIGIN_X		"0"
#define DEF_ORIGIN_Y		"0"
#define DEF_COLOR		STD_NORMAL_BACKGROUND
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
#define DEF_TEXTURE_TYPE	"stripes"
#define DEF_NEWGRADIENT_TYPE	"lineargradient"

static Blt_OptionParseProc ObjToImage;
static Blt_OptionPrintProc ImageToObj;
static Blt_OptionFreeProc ReleaseImage;
static Blt_CustomOption imageOption =
{
    ObjToImage, ImageToObj, ReleaseImage, (ClientData)0
};

static Blt_OptionParseProc ObjToGradientType;
static Blt_OptionPrintProc GradientTypeToObj;
static Blt_CustomOption gradientTypeOption =
{
    ObjToGradientType, GradientTypeToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToGradientScale;
static Blt_OptionPrintProc GradientScaleToObj;
static Blt_CustomOption scaleOption =
{
    ObjToGradientScale, GradientScaleToObj, NULL, (ClientData)0
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

static Blt_OptionParseProc ObjToTextureType;
static Blt_OptionPrintProc TextureTypeToObj;
static Blt_CustomOption textureTypeOption =
{
    ObjToTextureType, TextureTypeToObj, NULL, (ClientData)0
};

static Blt_ConfigSpec solidSpecs[] =
{
    {BLT_CONFIG_PIX32, "-color", "color", "Color", DEF_COLOR, 
	Blt_Offset(PaintBrushCmd, brush.solidColor), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", DEF_OPACITY, 
	Blt_Offset(PaintBrushCmd, brush.alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec tileSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-image", "image", "Image", (char *)NULL,
	Blt_Offset(PaintBrushCmd, tkImage), BLT_CONFIG_DONT_SET_DEFAULT, 
	&imageOption},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", DEF_OPACITY, 
	Blt_Offset(PaintBrushCmd, brush.alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec gradientSpecs[] =
{
    {BLT_CONFIG_PIX32, "-high", "high", "High", DEF_GRADIENT_HIGH,
	Blt_Offset(PaintBrushCmd, brush.high), 0},
    {BLT_CONFIG_CUSTOM, "-jitter", "jitter", "Jitter", DEF_GRADIENT_JITTER, 
	Blt_Offset(PaintBrushCmd, brush.jitter.range), 
	BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-low", "low", "Low", DEF_GRADIENT_LOW,
	Blt_Offset(PaintBrushCmd, brush.low), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", DEF_OPACITY, 
	Blt_Offset(PaintBrushCmd, brush.alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_CUSTOM, "-scale", "scale", "Scale", DEF_GRADIENT_SCALE, 
	Blt_Offset(PaintBrushCmd, brush.gradient.scale),
	BLT_CONFIG_DONT_SET_DEFAULT, &scaleOption},
    {BLT_CONFIG_CUSTOM, "-type", "type", "type", DEF_GRADIENT_TYPE, 
	Blt_Offset(PaintBrushCmd, brush.gradient.type), 
	BLT_CONFIG_DONT_SET_DEFAULT, &gradientTypeOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec newGradientSpecs[] =
{
    {BLT_CONFIG_PIX32, "-high", "high", "High", DEF_GRADIENT_HIGH,
	Blt_Offset(PaintBrushCmd, brush.high), 0},
    {BLT_CONFIG_CUSTOM, "-jitter", "jitter", "Jitter", DEF_GRADIENT_JITTER, 
	Blt_Offset(PaintBrushCmd, brush.jitter.range), 
	BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-low", "low", "Low", DEF_GRADIENT_LOW,
	Blt_Offset(PaintBrushCmd, brush.low), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", DEF_OPACITY, 
	Blt_Offset(PaintBrushCmd, brush.alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_CUSTOM, "-scale", "scale", "Scale", DEF_GRADIENT_SCALE, 
	Blt_Offset(PaintBrushCmd, brush.newGradient.scale),
	BLT_CONFIG_DONT_SET_DEFAULT, &scaleOption},
    {BLT_CONFIG_CUSTOM, "-type", "type", "type", DEF_NEWGRADIENT_TYPE, 
	Blt_Offset(PaintBrushCmd, brush.newGradient.type), 
	BLT_CONFIG_DONT_SET_DEFAULT, &gradientTypeOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec textureSpecs[] =
{
    {BLT_CONFIG_PIX32, "-high", "high", "High", DEF_GRADIENT_HIGH,
	Blt_Offset(PaintBrushCmd, brush.high), 0},
    {BLT_CONFIG_CUSTOM, "-jitter", "jitter", "Jitter", DEF_TEXTURE_JITTER, 
	Blt_Offset(PaintBrushCmd, brush.jitter.range), 
	BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-low", "low", "Low", DEF_GRADIENT_LOW,
	Blt_Offset(PaintBrushCmd, brush.low), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", DEF_OPACITY, 
	Blt_Offset(PaintBrushCmd, brush.alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_CUSTOM, "-type", "type", "Type", DEF_TEXTURE_TYPE, 
	Blt_Offset(PaintBrushCmd, brush.textureType), 
	BLT_CONFIG_DONT_SET_DEFAULT, &textureTypeOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

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
ImageToPicture(PaintBrushCmd *cmdPtr, int *isFreePtr)
{
    Tcl_Interp *interp;

    interp = cmdPtr->dataPtr->interp;
    return Blt_GetPictureFromImage(interp, cmdPtr->tkImage, isFreePtr);
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
ImageChangedProc(ClientData clientData, int x, int y, int width, int height,
                 int imageWidth, int imageHeight)
{
    PaintBrushCmd *cmdPtr = clientData;
    int isNew;

    /* Get picture from image. */
    if ((cmdPtr->brush.tile != NULL) && (cmdPtr->flags & FREE_PICTURE)) {
	Blt_FreePicture(cmdPtr->brush.tile);
    }
    if (Blt_Image_IsDeleted(cmdPtr->tkImage)) {
	cmdPtr->tkImage = NULL;
	return;                         /* Image was deleted. */
    }
    cmdPtr->brush.tile = ImageToPicture(cmdPtr, &isNew);
    if (Blt_Picture_IsAssociated(cmdPtr->brush.tile)) {
	Blt_UnassociateColors(cmdPtr->brush.tile);
    }
    if (isNew) {
	cmdPtr->flags |= FREE_PICTURE;
    } else {
	cmdPtr->flags &= ~FREE_PICTURE;
    }
}

/*ARGSUSED*/
static void
ReleaseImage(ClientData clientData, Display *display, char *widgRec, int offset)
{
    PaintBrushCmd *cmdPtr = (PaintBrushCmd *)(widgRec);

    if (cmdPtr->tkImage != NULL) {
	Tk_FreeImage(cmdPtr->tkImage);
	cmdPtr->tkImage = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToImage --
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
ObjToImage(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,		        /* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    PaintBrushCmd *cmdPtr = (PaintBrushCmd *)(widgRec);
    Tk_Image tkImage;

    tkImage = Tk_GetImage(interp, cmdPtr->tkwin, Tcl_GetString(objPtr), 
	ImageChangedProc, cmdPtr);
    if (tkImage == NULL) {
	return TCL_ERROR;
    }
    cmdPtr->tkImage = tkImage;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageToObj --
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
ImageToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    PaintBrushCmd *cmdPtr = (PaintBrushCmd *)(widgRec);
    
    if (cmdPtr->tkImage == NULL) {
	return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(Blt_Image_Name(cmdPtr->tkImage), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToGradientType --
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
ObjToGradientType(
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
	*typePtr = BLT_GRADIENT_VERTICAL;
    } else if ((c == 'h') && (strcmp(string, "horizontal") == 0)) {
	*typePtr = BLT_GRADIENT_HORIZONTAL;
    } else if ((c == 'r') && (strcmp(string, "radial") == 0)) {
	*typePtr = BLT_GRADIENT_RADIAL;
    } else if ((c == 'u') && (strcmp(string, "updiagonal") == 0)) {
	*typePtr = BLT_GRADIENT_DIAGONAL_UP;
    } else if ((c == 'd') && (strcmp(string, "downdiagonal") == 0)) {
	*typePtr = BLT_GRADIENT_DIAGONAL_DOWN;
    } else if ((c == 'c') && (strcmp(string, "conical") == 0)) {
	*typePtr = BLT_GRADIENT_CONICAL;
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
    case BLT_GRADIENT_VERTICAL:
	return "vertical";
    case BLT_GRADIENT_HORIZONTAL:
	return "horizontal";
    case BLT_GRADIENT_RADIAL:
	return "radial";
    case BLT_GRADIENT_DIAGONAL_UP:
	return "updiagonal";
    case BLT_GRADIENT_DIAGONAL_DOWN:
	return "downdiagonal";	
    case BLT_GRADIENT_CONICAL:
	return "conical";	
    default:
	return "???";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GradientTypeToObj --
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
GradientTypeToObj(
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
 * ObjToGradientScale --
 *
 *	Translates the given string to the gradient scale it represents.  
 *	Valid scales are "linear", "log", "atan".
 *
 * Results:
 *	A standard TCL result.  If successful the field in the structure
 *      is updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToGradientScale(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                   Tcl_Obj *objPtr, char *widgRec, int offset, int flags)	
{
    Blt_GradientScale *scalePtr = (Blt_GradientScale *)(widgRec + offset);
    const char *string;
    int length;
    char c;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'l') && (strcmp(string, "linear") == 0)) {
	*scalePtr = BLT_GRADIENT_LINEARSCALE;
    } else if ((c == 'l') && (length > 2) && 
	       (strncmp(string, "logarithmic", length) == 0)) {
	*scalePtr = BLT_GRADIENT_LOGSCALE;
    } else if ((c == 'a') && (strcmp(string, "atan") == 0)) {
	*scalePtr = BLT_GRADIENT_ATANSCALE;
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
    case BLT_GRADIENT_LINEARSCALE:
	return "linear";
    case BLT_GRADIENT_LOGSCALE:
	return "log";
    case BLT_GRADIENT_ATANSCALE:
	return "atan";
    default:
	return "?? unknown scale ??";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GradientScaleToObj --
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
GradientScaleToObj(
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
 * ObjToOpacity --
 *
 *	Converts the string representating the percent of opacity to an
 *	alpha value 0..255.
 *
 * Results:
 *	A standard TCL result.  
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
 *	Convert the alpha value into a string Tcl_Obj representing a
 *	percentage.
 *
 * Results:
 *	The string representation of the opacity percentage is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
OpacityToObj(
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
 * ObjToJitter --
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
 *	Convert the double jitter value to a Tcl_Obj.
 *
 * Results:
 *	The string representation of the jitter percentage is returned.
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

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTextureType --
 *
 *	Translate the given string to the texture type it represents.  
 *	Types are "checker", "vstripes"", "hstripes"".
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTextureType(
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
    if ((c == 'h') && (strncmp(string, "hstripes", length) == 0)) {
	*typePtr = BLT_TEXTURE_HSTRIPES;
    } else if ((c == 's') && (strncmp(string, "stripes", length) == 0)) {
	*typePtr = BLT_TEXTURE_VSTRIPES;
    } else if ((c == 'v') && (strncmp(string, "vstripes", length) == 0)) {
	*typePtr = BLT_TEXTURE_VSTRIPES;
    } else if ((c == 'c') && (strncmp(string, "checkers", length) == 0)) {
	*typePtr = BLT_TEXTURE_CHECKERS;
    } else if ((c == 'r') && (strncmp(string, "random", length) == 0)) {
	*typePtr = BLT_TEXTURE_RANDOM;
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
    case BLT_TEXTURE_VSTRIPES:
	return "vstripes";
    case BLT_TEXTURE_HSTRIPES:
	return "hstripes";
    case BLT_TEXTURE_CHECKERS:
	return "checkers";
    case BLT_TEXTURE_RANDOM:
	return "random";
    default:
	return "???";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TextureTypeToObj --
 *
 *	Returns the string representing the current pattern type.
 *
 * Results:
 *	The string representation of the pattern is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TextureTypeToObj(
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
GradientColorProc(Blt_PaintBrush *brushPtr, int x, int y)
{
    double t;
    Blt_Pixel color;
    PaintBrushCmd *cmdPtr;
    Blt_Gradient *gradPtr;

    cmdPtr = brushPtr->clientData;
    gradPtr = &cmdPtr->brush.gradient;
    switch (gradPtr->type) {
    case BLT_GRADIENT_RADIAL:
	{
	    double dx, dy, d;

	    dx = x - gradPtr->xOffset;
	    dy = y - gradPtr->yOffset;
	    d = sqrt(dx * dx + dy * dy);
	    t = 1.0 - (d * gradPtr->scaleFactor);
	}
	break;
    case BLT_GRADIENT_DIAGONAL_DOWN:
    case BLT_GRADIENT_DIAGONAL_UP:
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

    case BLT_GRADIENT_CONICAL:
	{
	    double d, dx, dy;

	    /* Translate to the center of the reference window. */
	    dx = x - gradPtr->xOffset;
	    dy = y - gradPtr->yOffset;
	    if (dx == 0.0) {
		d = gradPtr->angle;
	    } else {
		d = cos(atan(dy / dx) + gradPtr->angle);
	    }
	    d = fabs(fmod(d, 360.0));
	    t = d;
	}
	break;

    case BLT_GRADIENT_HORIZONTAL:
	t = (double)x * gradPtr->scaleFactor;
	break;
    default:
    case BLT_GRADIENT_VERTICAL:
	t = (double)y * gradPtr->scaleFactor;
	break;
    }
    if (brushPtr->jitter.range > 0.0) {
	t += Jitter(&brushPtr->jitter);
	t = JCLAMP(t);
    }
    if (gradPtr->scale == BLT_GRADIENT_LOGSCALE) {
	t = log10(9.0 * t + 1.0);
    } else if (gradPtr->scale == BLT_GRADIENT_ATANSCALE) {
	t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
    } 
    if (brushPtr->palette != NULL) {
	return Blt_Palette_GetAssociatedColor(brushPtr->palette, t);
    }
    color.Red   = (unsigned char)(brushPtr->low.Red   + t * brushPtr->rRange);
    color.Green = (unsigned char)(brushPtr->low.Green + t * brushPtr->gRange);
    color.Blue  = (unsigned char)(brushPtr->low.Blue  + t * brushPtr->bRange);
    color.Alpha = (unsigned char)(brushPtr->low.Alpha + t * brushPtr->aRange);
    return color.u32;
}

#ifdef notdef
static int
LinearGradientColorProc(Blt_PaintBrush *brushPtr, int x, int y)
{
    double t;
    Blt_Pixel color;
    PaintBrushCmd *cmdPtr;
    Blt_LinearGradient *gradPtr;
    
    cmdPtr = brushPtr->clientData;
    gradPtr = &cmdPtr->brush.linear;
    switch (gradPtr->type) {
    case BLT_GRADIENT_HORIZONTAL:
        t = (y - gradPtr->x1) / (gradPtr->x2 - gradPtr->x1);
	break;
    default:
    case BLT_GRADIENT_VERTICAL:
        t = (y - gradPtr->y1) / (gradPtr->y2 - gradPtr->y1);
        break;
    default:
	{
	    double d;
	    Point2d p;

            /* Get the projection of the the sample point on the infinite
             * line described by the line segment. The distance of the
             * projected point from the start of the line segment over the
             * distance of the line segment is t. */
            p = Blt_GetProjection2(x, y, gradPtr->x1, gradPtr->y1, gradPtr->x2,
                gradPtr->y2);
            d = hypot(p.x - gradPtr->x1, p.y - gradPtr->y1);
            t = d / gradPtr->length;
	}
	break;
    }
    if ((t < 0.0) || (t > 1.0)) {
        out = 1;
        rem = fmod(t, 1.0);
        pos = (int)(t - rem);
        if (pos & 0x1) {
            rev = (t < 0.0) ? 1 : 0;
        }
        t = rem;
    }
    if (brushPtr->jitter.range > 0.0) {
	t += Jitter(&brushPtr->jitter);
	t = JCLAMP(t);
    }
    if (gradPtr->scale == BLT_GRADIENT_LOGSCALE) {
	t = log10(9.0 * t + 1.0);
    } else if (gradPtr->scale == BLT_GRADIENT_ATANSCALE) {
	t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
    } 
    if (brushPtr->palette != NULL) {
	return Blt_Palette_GetAssociatedColor(brushPtr->palette, t);
    }
    color.Red   = (unsigned char)(brushPtr->low.Red   + t * brushPtr->rRange);
    color.Green = (unsigned char)(brushPtr->low.Green + t * brushPtr->gRange);
    color.Blue  = (unsigned char)(brushPtr->low.Blue  + t * brushPtr->bRange);
    color.Alpha = (unsigned char)(brushPtr->low.Alpha + t * brushPtr->aRange);
    return color.u32;
}

static int
RadialGradientColorProc(Blt_PaintBrush *brushPtr, int x, int y)
{
    double t;
    Blt_Pixel color;
    PaintBrushCmd *cmdPtr;
    Blt_Gradient *gradPtr;
    double dx, dy, d;

    cmdPtr = brushPtr->clientData;
    gradPtr = &cmdPtr->brush.radial;

    dx = x - gradPtr->xOffset;
    dy = y - gradPtr->yOffset;

    /* Circular radial gradient is the distance from the sample point to
     * the center of the gradient. */
    d = hypot(x - gradPtr->xOffset, y - gradPtr->yOffset);
    t = 1.0 - (d * gradPtr->scaleFactor);

    /* Elliptical radial gradient is the distance from the sample point to
     * the center of the gradient. */

    if (brushPtr->jitter.range > 0.0) {
	t += Jitter(&brushPtr->jitter);
	t = JCLAMP(t);
    }
    if (gradPtr->scale == BLT_GRADIENT_LOGSCALE) {
	t = log10(9.0 * t + 1.0);
    } else if (gradPtr->scale == BLT_GRADIENT_ATANSCALE) {
	t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
    } 
    if (brushPtr->palette != NULL) {
	return Blt_Palette_GetAssociatedColor(brushPtr->palette, t);
    }
    color.Red   = (unsigned char)(brushPtr->low.Red   + t * brushPtr->rRange);
    color.Green = (unsigned char)(brushPtr->low.Green + t * brushPtr->gRange);
    color.Blue  = (unsigned char)(brushPtr->low.Blue  + t * brushPtr->bRange);
    color.Alpha = (unsigned char)(brushPtr->low.Alpha + t * brushPtr->aRange);
    return color.u32;
}

static int
ConicalGradientColorProc(Blt_PaintBrush *brushPtr, int x, int y)
{
    double t;
    Blt_Pixel color;
    PaintBrushCmd *cmdPtr;
    Blt_Gradient *gradPtr;
    double d, dx, dy;

    cmdPtr = brushPtr->clientData;
    gradPtr = &cmdPtr->brush.gradient;
    /* Translate to the center of the reference window. */
    dx = x - gradPtr->xOffset;
    dy = y - gradPtr->yOffset;
    if (dx == 0.0) {
        d = gradPtr->angle;
    } else {
        d = cos(atan(dy / dx) + gradPtr->angle);
    }
    d = fabs(fmod(d, 360.0));
    t = d;
    if (brushPtr->jitter.range > 0.0) {
	t += Jitter(&brushPtr->jitter);
	t = JCLAMP(t);
    }
    if (gradPtr->scale == BLT_GRADIENT_LOGSCALE) {
	t = log10(9.0 * t + 1.0);
    } else if (gradPtr->scale == BLT_GRADIENT_ATANSCALE) {
	t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
    } 
    if (brushPtr->palette != NULL) {
	return Blt_Palette_GetAssociatedColor(brushPtr->palette, t);
    }
    color.Red   = (unsigned char)(brushPtr->low.Red   + t * brushPtr->rRange);
    color.Green = (unsigned char)(brushPtr->low.Green + t * brushPtr->gRange);
    color.Blue  = (unsigned char)(brushPtr->low.Blue  + t * brushPtr->bRange);
    color.Alpha = (unsigned char)(brushPtr->low.Alpha + t * brushPtr->aRange);
    return color.u32;
}

#endif

static int
TextureColorProc(Blt_PaintBrush *brushPtr, int x, int y)
{
    double t;
    Blt_Pixel color;
    PaintBrushCmd *cmdPtr;

    cmdPtr = brushPtr->clientData;
    switch (cmdPtr->brush.textureType) {
    default:
	
    case BLT_TEXTURE_RANDOM:
	t = RandomNumber(&brushPtr->random);
	break;

    case BLT_TEXTURE_VSTRIPES:
	t = ((y / 2) & 0x1) ? 0 : 1;
	break;

    case BLT_TEXTURE_HSTRIPES:
	t = ((x / 2) & 0x1) ? 0 : 1;
	break;

    case BLT_TEXTURE_CHECKERS:
	{
	    int oddx, oddy;
	    
	    oddx = (x / 8) & 0x01;
	    oddy = (y / 8) & 0x01;
	    t = ((oddy + oddx) == 1) ? 0 : 1;
	}
	break;
    }
    if (brushPtr->jitter.range > 0.0) {
	t += Jitter(&brushPtr->jitter);
	t = JCLAMP(t);
    }
    color.Red   = (unsigned char)
	(brushPtr->low.Red   + t * brushPtr->rRange);
    color.Green = (unsigned char)
	(brushPtr->low.Green + t * brushPtr->gRange);
    color.Blue  = (unsigned char)
	(brushPtr->low.Blue  + t * brushPtr->bRange);
    color.Alpha = (unsigned char)
	(brushPtr->low.Alpha + t * brushPtr->aRange);
    return color.u32;
}


static const char *
NameOfPaintBrushType(PaintBrushCmd *cmdPtr) 
{
    return paintbrushTypes[cmdPtr->brush.type];
}

static int 
GetPaintTypeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_PaintBrushType *typePtr)
{
    const char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 't') && (length > 1) && (strncmp(string, "tile", length) == 0)) {
	*typePtr = BLT_PAINTBRUSH_TILE;
    } else if ((c == 'g') && (strncmp(string, "gradient", length) == 0)) {
	*typePtr = BLT_PAINTBRUSH_GRADIENT;
    } else if ((c == 's') && (strncmp(string, "solid", length) == 0)) {
	*typePtr = BLT_PAINTBRUSH_SOLID;
    } else if ((c == 't') && (length > 1)  &&
	       (strncmp(string, "texture", length) == 0)) {
	*typePtr = BLT_PAINTBRUSH_TEXTURE;
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
 *	Retrieves the paintbrush command named by the given the Tcl_Obj.
 *
 * Results:
 *	None.
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
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyPaintBrushCmd(PaintBrushCmd *cmdPtr)
{
    Blt_FreeOptions(cmdPtr->configSpecs, (char *)cmdPtr, cmdPtr->display, 0);
    if (cmdPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&cmdPtr->dataPtr->instTable, cmdPtr->hashPtr);
    }
    Blt_Free(cmdPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NewPaintBrushCmd --
 *
 *	Creates a new paintbrush.
 *
 * Results:
 *	Returns pointer to the new paintbrush command.
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
    case BLT_PAINTBRUSH_SOLID:
	cmdPtr->brush.type = BLT_PAINTBRUSH_SOLID;
	cmdPtr->configSpecs = solidSpecs;
	break;
    case BLT_PAINTBRUSH_TILE:
	cmdPtr->brush.type = BLT_PAINTBRUSH_TILE;
	cmdPtr->configSpecs = tileSpecs;
	break;
    case BLT_PAINTBRUSH_GRADIENT:
	cmdPtr->brush.type = BLT_PAINTBRUSH_GRADIENT;
	cmdPtr->configSpecs = gradientSpecs;
	break;
    case BLT_PAINTBRUSH_NEWGRADIENT:
	cmdPtr->brush.type = BLT_PAINTBRUSH_NEWGRADIENT;
	cmdPtr->configSpecs = newGradientSpecs;
	break;
    case BLT_PAINTBRUSH_TEXTURE:
	cmdPtr->brush.type = BLT_PAINTBRUSH_TEXTURE;
	cmdPtr->configSpecs = textureSpecs;
	break;
    default:
	abort();
	break;
    }
    cmdPtr->dataPtr = dataPtr;
    cmdPtr->tkwin = Tk_MainWindow(interp);
    cmdPtr->display = Tk_Display(cmdPtr->tkwin);
    cmdPtr->brush.alpha = 0xFF;
    return cmdPtr;
}

static int
ConfigurePaintBrushCmd(Tcl_Interp *interp, PaintBrushCmd *cmdPtr, int objc, 
                       Tcl_Obj *const *objv, int flags)
{
    if (Blt_ConfigureWidgetFromObj(interp, cmdPtr->tkwin, cmdPtr->configSpecs, 
	objc, objv, (char *)cmdPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((cmdPtr->brush.type == BLT_PAINTBRUSH_TILE) && 
	(cmdPtr->tkImage != NULL)) {
	int isNew;

	if ((cmdPtr->brush.tile != NULL) && (cmdPtr->flags & FREE_PICTURE)) {
	    Blt_FreePicture(cmdPtr->brush.tile);
	}
	cmdPtr->brush.tile = ImageToPicture(cmdPtr, &isNew);
	if (Blt_Picture_IsAssociated(cmdPtr->brush.tile)) {
	    Blt_UnassociateColors(cmdPtr->brush.tile);
	}
	if (isNew) {
	    cmdPtr->flags |= FREE_PICTURE;
	} else {
	    cmdPtr->flags &= ~FREE_PICTURE;
	}
    }
    if (cmdPtr->brush.type == BLT_PAINTBRUSH_GRADIENT) {
	cmdPtr->brush.colorProc = GradientColorProc;
	cmdPtr->brush.clientData = cmdPtr;
    }
    if (cmdPtr->brush.type == BLT_PAINTBRUSH_TEXTURE) {
	cmdPtr->brush.colorProc = TextureColorProc;
	cmdPtr->brush.clientData = cmdPtr;
	RandomInit(&cmdPtr->brush.random);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *      Creates a new paintbrush object.
 *
 *      paintbrush create type ?name? ?option values?...
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

    if (GetPaintTypeFromObj(interp, objv[2], &type) != TCL_OK) {
	return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]);
    if (string[0] == '-') {		
	int isNew;
	char name[200];

        /* Generate a unique name for the paintbrush.  */
        do {
	    Blt_FormatString(name, 200, "paintbrush%d", dataPtr->nextId++);
	    hPtr = Blt_CreateHashEntry(&dataPtr->instTable, name, &isNew);
	} while (!isNew);
    } else {
	int isNew;

	hPtr = Blt_CreateHashEntry(&dataPtr->instTable, string, &isNew);
	if (!isNew) {
	    Tcl_AppendResult(interp, "a paintbrush named \"", string, 
			     "\" already exists.", (char *)NULL);
	    return TCL_ERROR;
	}
	objc--, objv++;
    }
    cmdPtr = NewPaintBrushCmd(dataPtr, interp, type);
    if (cmdPtr == NULL) {
	return TCL_ERROR;
    }
    cmdPtr->refCount = 1;
    Blt_SetHashValue(hPtr, cmdPtr);
    cmdPtr->hashPtr = hPtr;
    cmdPtr->name = Blt_GetHashKey(&dataPtr->instTable, hPtr);

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
 *      paintbrush cget $brush ?option?...
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
    return Blt_ConfigureValueFromObj(interp, cmdPtr->tkwin, 
	cmdPtr->configSpecs, (char *)cmdPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      paintbrush configure $brush ?option?...
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
	return Blt_ConfigureInfoFromObj(interp, cmdPtr->tkwin, 
		cmdPtr->configSpecs, (char *)cmdPtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, cmdPtr->tkwin, 
		cmdPtr->configSpecs, (char *)cmdPtr, objv[3], flags);
    } 
    if (ConfigurePaintBrushCmd(interp, cmdPtr, objc-3, objv+3, flags)!=TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes one or more paintbrush objects.
 *
 *      bgpattern delete brushName... 
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
 *	paintbrush names ?pattern ... ?
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
 *      paintbrush type brushName
 *
 *---------------------------------------------------------------------------
 */
static int
TypeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    PaintBrushCmdInterpData *dataPtr = clientData;
    PaintBrushCmd *cmdPtr;

    if (GetPaintBrushCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), NameOfPaintBrushType(cmdPtr),-1);
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
    {"configure", 2, ConfigureOp, 3, 0, "brushName ?option value?...",},
    {"create",    2, CreateOp,    3, 0, "type ?args?",},
    {"delete",    1, DeleteOp,    2, 0, "brushName...",},
    {"names",     1, NamesOp,     2, 3, "brushName ?pattern?",},
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
 *      the paintbrushes previously created and still available, and then
 *      deletes the hash table that keeps track of them. 
 *
 *      We have to wait for the deletion of the interpreter and not the
 *      "blt::paintbrush" command because 1) we need all the clients of a
 *      paintbrush to release them before we can remove the hash table and
 *      2) we can't guarentee that "blt::paintbrush" command wil be deleted
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

/*LINTLIBRARY*/
int
Blt_PaintBrushCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "paintbrush", PaintBrushCmdProc, };

    cmdSpec.clientData = GetPaintBrushCmdInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}


void
Blt_PaintBrush_Init(Blt_PaintBrush *brushPtr)
{
    memset(brushPtr, 0, sizeof(Blt_PaintBrush));
    brushPtr->type = BLT_PAINTBRUSH_SOLID;
    brushPtr->alpha = 0xFF;
}

void
Blt_PaintBrush_SetPalette(Blt_PaintBrush *brushPtr, Blt_Palette palette)
{
    brushPtr->type = BLT_PAINTBRUSH_GRADIENT;
    brushPtr->palette = palette;
}

void
Blt_PaintBrush_SetOrigin(Blt_PaintBrush *brushPtr, int x, int y)
{
    brushPtr->xOrigin = x;
    brushPtr->yOrigin = y;
}

void 
Blt_PaintBrush_SetTile(Blt_PaintBrush *brushPtr, Blt_Picture picture)
{
    brushPtr->type = BLT_PAINTBRUSH_TILE;
    brushPtr->tile = picture;
    if (Blt_Picture_IsAssociated(brushPtr->tile)) {
	Blt_UnassociateColors(brushPtr->tile);
    }
}

void 
Blt_PaintBrush_SetColor(Blt_PaintBrush *brushPtr, unsigned int value)
{
    brushPtr->type = BLT_PAINTBRUSH_SOLID;
    brushPtr->solidColor.u32 = value;
    Blt_AssociateColor(&brushPtr->solidColor);
}

void
Blt_PaintBrush_SetTexture(Blt_PaintBrush *brushPtr)
{
    brushPtr->type = BLT_PAINTBRUSH_TEXTURE;
    brushPtr->colorProc = TextureColorProc;
}

void
Blt_PaintBrush_SetGradient(Blt_PaintBrush *brushPtr, Blt_GradientType type)
{
    brushPtr->type = BLT_PAINTBRUSH_GRADIENT;
    brushPtr->colorProc = GradientColorProc;
    brushPtr->gradient.type = type;
}

void
Blt_PaintBrush_SetColors(Blt_PaintBrush *brushPtr, Blt_Pixel *lowPtr, 
			 Blt_Pixel *highPtr)
{
    brushPtr->low.u32 = lowPtr->u32;
    brushPtr->high.u32 = highPtr->u32;
}

void
Blt_PaintBrush_Region(Blt_PaintBrush *brushPtr, int x, int y, int w, int h)
{
    brushPtr->xOrigin = x;
    brushPtr->yOrigin = y;
    if ((brushPtr->type == BLT_PAINTBRUSH_GRADIENT) || 
	(brushPtr->type == BLT_PAINTBRUSH_TEXTURE)) {
	brushPtr->rRange = brushPtr->high.Red   - brushPtr->low.Red;
	brushPtr->gRange = brushPtr->high.Green - brushPtr->low.Green;
	brushPtr->bRange = brushPtr->high.Blue  - brushPtr->low.Blue;
	brushPtr->aRange = brushPtr->high.Alpha - brushPtr->low.Alpha;
	if (brushPtr->jitter.range > 0.0) {
	    JitterInit(&brushPtr->jitter);
	}
    }
    if (brushPtr->type == BLT_PAINTBRUSH_GRADIENT) {
	Blt_Gradient *gradPtr;

	gradPtr = &brushPtr->gradient;
	switch (gradPtr->type) {
	case BLT_GRADIENT_HORIZONTAL:
	    gradPtr->scaleFactor = 0.0;
	    if (w > 1) {
		gradPtr->scaleFactor = 1.0 / (w - 1);
	    } 
	    break;
	default:
	case BLT_GRADIENT_VERTICAL:
	    gradPtr->scaleFactor = 0.0;
	    if (h > 1) {
		gradPtr->scaleFactor = 1.0 / (h - 1);
	    } 
	    break;
	case BLT_GRADIENT_DIAGONAL_UP:
	case BLT_GRADIENT_DIAGONAL_DOWN:
	    gradPtr->xOffset = w * 0.5;
	    gradPtr->yOffset = h * 0.5;
	    gradPtr->length = sqrt(w * w + h * h);
	    gradPtr->cosTheta = w / gradPtr->length;
	    gradPtr->sinTheta = h / gradPtr->length;
	    if (gradPtr->type == BLT_GRADIENT_DIAGONAL_DOWN) {
		gradPtr->sinTheta = -gradPtr->sinTheta;
	    }
	    gradPtr->scaleFactor = 0.0;
	    if (gradPtr->length > 1) {
		gradPtr->scaleFactor = 1.0 / (gradPtr->length - 1);
	    } 
	    break;
	case BLT_GRADIENT_RADIAL:
	    gradPtr->xOffset = w * 0.5;
	    gradPtr->yOffset = h * 0.5;
	    gradPtr->length = sqrt(w * w + h * h);
	    gradPtr->scaleFactor = 0.0;
	    if (gradPtr->length > 1) {
		gradPtr->scaleFactor = 1.0 / ((gradPtr->length * 0.5) - 1);
	    } 
	    break;
	case BLT_GRADIENT_CONICAL:
	    gradPtr->xOffset = w * 0.5;
	    gradPtr->yOffset = h * 0.5;
	    gradPtr->angle = 45 * DEG2RAD;
	    break;
	}
    }
}

void 
Blt_PaintBrush_SetColorProc(Blt_PaintBrush *brushPtr, 
			    Blt_PaintBrush_ColorProc *proc, 
			    ClientData clientData)
{
    brushPtr->type = BLT_PAINTBRUSH_GRADIENT;
    brushPtr->colorProc = proc;
    brushPtr->clientData = clientData;
}

/* 
 */
/*
 *---------------------------------------------------------------------------
 *
 * Blt_PaintBrush_GetAssociatedColor --
 *
 *      Gets the color from the paint brush at the given x,y coordinate.
 *      For texture, gradient, and tile brushes, the coordinate is used to
 *      compute the color.  The return color is always associated
 *      (premultiplied).
 *
 * Results:
 *	Returns the color at the current x,y coordinate.  The color
 *      is always associated. 
 *
 *---------------------------------------------------------------------------
 */
int
Blt_PaintBrush_GetAssociatedColor(Blt_PaintBrush *brushPtr, int x, int y)
{
    Blt_Pixel *pixelPtr;
    Blt_Pixel color;

    switch (brushPtr->type) {
    case BLT_PAINTBRUSH_SOLID:
	return brushPtr->solidColor.u32;
    case BLT_PAINTBRUSH_TILE:
	if (brushPtr->tile == NULL) {
	    return brushPtr->solidColor.u32;
	}
	/* Factor in the tile origin when computing the pixel in the tile. */
	x = (x - brushPtr->xOrigin) % Blt_Picture_Width(brushPtr->tile);
	if (x < 0) {
	    x = -x;
	}
	y = (y - brushPtr->yOrigin) % Blt_Picture_Height(brushPtr->tile);
	if (y < 0) {
	    y = -y;
	}
	pixelPtr = Blt_Picture_Pixel(brushPtr->tile, x, y);
	/* Pixel of tile is unassociated so that we can override the
	 * opacity. */
	color.u32 = pixelPtr->u32;
	color.Alpha = brushPtr->alpha;
	Blt_AssociateColor(&color);
	return color.u32;
    case BLT_PAINTBRUSH_TEXTURE:
    case BLT_PAINTBRUSH_GRADIENT:
	if (brushPtr->colorProc == NULL) {
	    return brushPtr->solidColor.u32;
	} 
	/* Factor in the gradient origin when computing the pixel. */
	x = (x - brushPtr->xOrigin);
	if (x < 0) {
	    x = -x;
	}
	y = (y - brushPtr->yOrigin);
	if (y < 0) {
	    y = -y;
	}
	/* Need window or object width/height to compute color value. */
	color.u32 = (*brushPtr->colorProc)(brushPtr, x, y);
	/* Texture and gradient colors are always associated. */
	Blt_AssociateColor(&color);
	return color.u32;
    }
    return 0;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_PaintBrush_Free
 *
 *	Releases the paintbrush structure.  If no other client is using the
 *	paintbrush structure, then it is freed.
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
Blt_PaintBrush_Free(Blt_PaintBrush *brushPtr)
{
    PaintBrushCmd *cmdPtr = (PaintBrushCmd *)brushPtr;

    assert(cmdPtr != NULL);
    cmdPtr->refCount--;
    if (cmdPtr->refCount <= 0) {
	DestroyPaintBrushCmd(cmdPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PaintBrush_GetFromString --
 *
 *	Retrieves the paintbrush object named by the given the string.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_PaintBrush_GetFromString(Tcl_Interp *interp, const char *string, 
			Blt_PaintBrush **brushPtrPtr)
{
    Blt_HashEntry *hPtr;
    PaintBrushCmd *cmdPtr;
    PaintBrushCmdInterpData *dataPtr;

    dataPtr = GetPaintBrushCmdInterpData(interp);
    hPtr = Blt_FindHashEntry(&dataPtr->instTable, string);
    if (hPtr == NULL) { 
	Blt_Pixel color;
        int isNew;
        
	/* The paintbrush doesn't already exist, so see if it's a color
	 * name (something that Tk_Get3DBorder will accept). If it's a
	 * valid color, then automatically create a solid brush out of
	 * it. */
	if (Blt_GetPixel(interp, string, &color) != TCL_OK) {
	    return TCL_ERROR;           /* Nope. It's an error. */
	} 
	cmdPtr = NewPaintBrushCmd(dataPtr, interp, BLT_PAINTBRUSH_SOLID);
	if (cmdPtr == NULL) {
	    return TCL_ERROR;           /* Can't allocate a new brush. */
	}
	cmdPtr->brush.solidColor.u32 = color.u32;
	cmdPtr->refCount = 1;
	Blt_AssociateColor(&cmdPtr->brush.solidColor);
        hPtr = Blt_CreateHashEntry(&dataPtr->instTable, string, &isNew);
        assert(isNew);
	cmdPtr->hashPtr = hPtr;
	cmdPtr->name = Blt_GetHashKey(&dataPtr->instTable, hPtr);
	Blt_SetHashValue(hPtr, cmdPtr);
    } else {
	cmdPtr = Blt_GetHashValue(hPtr);
	cmdPtr->refCount++;
	assert(cmdPtr != NULL);
    }
    *brushPtrPtr = (Blt_PaintBrush *)cmdPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PaintBrush_Get --
 *
 *	Retrieves the paintbrush command named by the given the Tcl_Obj.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_PaintBrush_Get(Tcl_Interp *interp, Tcl_Obj *objPtr, 
		   Blt_PaintBrush **brushPtrPtr)
{
    const char *string;

    string = Tcl_GetString(objPtr);
    return Blt_PaintBrush_GetFromString(interp, string, brushPtrPtr);
}

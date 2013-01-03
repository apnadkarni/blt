
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
#include "bltPicture.h"
#include "bltPictInt.h"
#include "bltImage.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define JCLAMP(c)	((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))

#define PAINTBRUSH_THREAD_KEY	"BLT Paintbrush Data"

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
typedef enum PaintbrushTypes {
    PAINTBRUSH_GRADIENT,		/* Color gradient. */
    PAINTBRUSH_TILE,			/* Tiled image. */
    PAINTBRUSH_SOLID,			/* Solid color. */
    PAINTBRUSH_TEXTURE,			/* Procedural texture. */
} PaintbrushType;

static const char *paintbrushTypes[] = {
    "gradient",
    "tile",
    "solid",
    "texture"
};

typedef struct {
    Blt_HashTable instTable;		/* Hash table of paintbrush structures
					 * keyed by the name. */
    Tcl_Interp *interp;			/* Interpreter associated with this set
					 * of background paints. */
    int nextId;				/* Serial number of the identifier to be
					 * used for next paintbrush created.  */
} PaintbrushCmdInterpData;

typedef struct {
    Blt_Paintbrush brush;
    Blt_HashEntry *hashPtr;		/* Hash entry of this brush in
					 * interpreter-specific paintbrush
					 * table. */
    const char *name;			/* Name of paintbrush. Points to
					 * hashtable key. */
    int refCount;			/* # of clients using this brush.  If
					 * zero, this brush can be deleted. */
    PaintbrushCmdInterpData *dataPtr;	/* Pointer to interpreter-specific
					 * data. */
    Blt_ConfigSpec *configSpecs;	/* Configuration specifications for 
					 * this type of brush.  */
    Tk_Window tkwin;			/* Main window. Used to query background
					 * pattern options. */
    Display *display;			/* Display of this paintbrush. Used to
					 * free switches. */
    unsigned int flags;			/* See definitions below. */
    Tk_Image tkImage;			/* Tk image used for tiling (tile
					 * brushes only). */
} PaintbrushCmd;


#define FREE_PICTURE		(1<<0)

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
#define DEF_TEXTURE_TYPE	"striped"

static Blt_OptionParseProc ObjToImageProc;
static Blt_OptionPrintProc ImageToObjProc;
static Blt_OptionFreeProc FreeImageProc;
static Blt_CustomOption imageOption =
{
    ObjToImageProc, ImageToObjProc, FreeImageProc, (ClientData)0
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
    {BLT_CONFIG_PIX32, "-color", "color", "Color", DEF_COLOR, 
	Blt_Offset(PaintbrushCmd, brush.solidColor), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", DEF_OPACITY, 
	Blt_Offset(PaintbrushCmd, brush.alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec tileConfigSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-image", "image", "Image", (char *)NULL,
        Blt_Offset(PaintbrushCmd, tkImage), BLT_CONFIG_DONT_SET_DEFAULT, 
	&imageOption},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", DEF_OPACITY, 
	Blt_Offset(PaintbrushCmd, brush.alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec gradientConfigSpecs[] =
{
    {BLT_CONFIG_PIX32, "-high", "high", "High", DEF_GRADIENT_HIGH,
        Blt_Offset(PaintbrushCmd, brush.high), 0},
    {BLT_CONFIG_CUSTOM, "-jitter", "jitter", "Jitter", DEF_GRADIENT_JITTER, 
	Blt_Offset(PaintbrushCmd, brush.jitter.range), 
	BLT_CONFIG_DONT_SET_DEFAULT, &jitterOption},
    {BLT_CONFIG_PIX32, "-low", "low", "Low", DEF_GRADIENT_LOW,
	Blt_Offset(PaintbrushCmd, brush.low), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", DEF_OPACITY, 
	Blt_Offset(PaintbrushCmd, brush.alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_CUSTOM, "-scale", "scale", "Scale", DEF_GRADIENT_SCALE, 
	Blt_Offset(PaintbrushCmd, brush.gradient.scale),
	BLT_CONFIG_DONT_SET_DEFAULT, &scaleOption},
    {BLT_CONFIG_CUSTOM, "-type", "type", "type", DEF_GRADIENT_TYPE, 
	Blt_Offset(PaintbrushCmd, brush.gradient.type), 
	BLT_CONFIG_DONT_SET_DEFAULT, &gradientTypeOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec textureConfigSpecs[] =
{
    {BLT_CONFIG_PIX32, "-high", "high", "High", DEF_GRADIENT_HIGH,
        Blt_Offset(PaintbrushCmd, brush.high), 0},
    {BLT_CONFIG_CUSTOM, "-jitter", "jitter", "Jitter", DEF_TEXTURE_JITTER, 
	Blt_Offset(PaintbrushCmd, brush.jitter.range), BLT_CONFIG_DONT_SET_DEFAULT, 
	&jitterOption},
    {BLT_CONFIG_PIX32, "-low", "low", "Low", DEF_GRADIENT_LOW,
        Blt_Offset(PaintbrushCmd, brush.low), 0},
    {BLT_CONFIG_CUSTOM, "-opacity", "opacity", "Opacity", DEF_OPACITY, 
	Blt_Offset(PaintbrushCmd, brush.alpha), BLT_CONFIG_DONT_SET_DEFAULT, 
	&opacityOption},
    {BLT_CONFIG_CUSTOM, "-type", "type", "Type", DEF_TEXTURE_TYPE, 
	Blt_Offset(PaintbrushCmd, brush.textureType), 
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
ImageToPicture(PaintbrushCmd *cmdPtr, int *isFreePtr)
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
ImageChangedProc(
    ClientData clientData,
    int x, int y, int width, int height, /* Not used. */
    int imageWidth, int imageHeight)	 /* Not used. */
{
    PaintbrushCmd *cmdPtr = clientData;
    int isNew;

    /* Get picture from image. */
    if ((cmdPtr->brush.tile != NULL) && (cmdPtr->flags & FREE_PICTURE)) {
	Blt_FreePicture(cmdPtr->brush.tile);
    }
    cmdPtr->brush.tile = ImageToPicture(cmdPtr, &isNew);
    if (isNew) {
	cmdPtr->flags |= FREE_PICTURE;
    } else {
	cmdPtr->flags &= ~FREE_PICTURE;
    }
}

/*ARGSUSED*/
static void
FreeImageProc(
    ClientData clientData,
    Display *display,			/* Not used. */
    char *widgRec,
    int offset)
{
    PaintbrushCmd *cmdPtr = (PaintbrushCmd *)(widgRec);

    if (cmdPtr->tkImage != NULL) {
	Tk_FreeImage(cmdPtr->tkImage);
	cmdPtr->tkImage = NULL;
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
    PaintbrushCmd *cmdPtr = (PaintbrushCmd *)(widgRec);
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
    PaintbrushCmd *cmdPtr = (PaintbrushCmd *)(widgRec);
    
    if (cmdPtr->tkImage == NULL) {
	return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(Blt_Image_Name(cmdPtr->tkImage), -1);
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
    } else if ((c == 'c') && (strcmp(string, "conical") == 0)) {
	*typePtr = BLT_GRADIENT_TYPE_CONICAL;
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
    case BLT_GRADIENT_TYPE_CONICAL:
	return "conical";	
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
 *	Translates the given string to the gradient scale it represents.  
 *	Valid scales are "linear", "log", "atan".
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
    char *widgRec,			/* Widget record. */    int offset,				/* Offset to field in structure */
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
    } else if ((c == 'r') && (strncmp(string, "random", length) == 0)) {
	*typePtr = BLT_TEXTURE_TYPE_RANDOM;
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
    case BLT_TEXTURE_TYPE_RANDOM:
	return "random";
    default:
	return "???";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TextureTypeToObjProc --
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
GradientColorProc(Blt_Paintbrush *brushPtr, int x, int y)
{
    double t;
    Blt_Pixel color;
    PaintbrushCmd *cmdPtr;
    Blt_Gradient *gradPtr;

    cmdPtr = brushPtr->clientData;
    gradPtr = &cmdPtr->brush.gradient;
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

    case BLT_GRADIENT_TYPE_CONICAL:
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

    case BLT_GRADIENT_TYPE_HORIZONTAL:
	t = (double)x * gradPtr->scaleFactor;
	break;
    default:
    case BLT_GRADIENT_TYPE_VERTICAL:
	t = (double)y * gradPtr->scaleFactor;
	t = JCLAMP(t);
	break;
    }
    if (brushPtr->jitter.range > 0.0) {
	t += Jitter(&brushPtr->jitter);
	t = JCLAMP(t);
    }
    if (gradPtr->scale == BLT_GRADIENT_SCALE_LOG) {
	t = log10(9.0 * t + 1.0);
    } else if (gradPtr->scale == BLT_GRADIENT_SCALE_ATAN) {
	t = atan(18.0 * (t-0.05) + 1.0) / M_PI_2;
    } 
    if (brushPtr->palette != NULL) {
	return Blt_Palette_GetColor(brushPtr->palette, t);
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

static int
TextureColorProc(Blt_Paintbrush *brushPtr, int x, int y)
{
    double t;
    Blt_Pixel color;
    PaintbrushCmd *cmdPtr;

    cmdPtr = brushPtr->clientData;
    switch (cmdPtr->brush.textureType) {
    default:
	
    case BLT_TEXTURE_TYPE_RANDOM:
	t = RandomNumber(&brushPtr->random);
	break;

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
NameOfPaintbrushType(PaintbrushCmd *cmdPtr) 
{
    return paintbrushTypes[cmdPtr->brush.type];
}

static int 
GetPaintTypeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_PaintbrushType *typePtr)
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
 * GetPaintbrushCmdFromObj --
 *
 *	Retrieves the paintbrush command named by the given the Tcl_Obj.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
GetPaintbrushCmdFromObj(Tcl_Interp *interp, PaintbrushCmdInterpData *dataPtr, 
		   Tcl_Obj *objPtr, PaintbrushCmd **cmdPtrPtr)
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
 * DestroyPaintbrushCmd --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyPaintbrushCmd(PaintbrushCmd *cmdPtr)
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
 * NewPaintbrushCmd --
 *
 *	Creates a new paintbrush.
 *
 * Results:
 *	Returns pointer to the new paintbrush command.
 *
 *---------------------------------------------------------------------------
 */
static PaintbrushCmd *
NewPaintbrushCmd(PaintbrushCmdInterpData *dataPtr, Tcl_Interp *interp, 
		 Blt_PaintbrushType type)
{
    PaintbrushCmd *cmdPtr;

    cmdPtr = Blt_AssertCalloc(1, sizeof(PaintbrushCmd));
    switch (type) {
    case BLT_PAINTBRUSH_SOLID:
	cmdPtr->brush.type = BLT_PAINTBRUSH_SOLID;
	cmdPtr->configSpecs = solidConfigSpecs;
	break;
    case BLT_PAINTBRUSH_TILE:
	cmdPtr->brush.type = BLT_PAINTBRUSH_TILE;
	cmdPtr->configSpecs = tileConfigSpecs;
	break;
    case BLT_PAINTBRUSH_GRADIENT:
	cmdPtr->brush.type = BLT_PAINTBRUSH_GRADIENT;
	cmdPtr->configSpecs = gradientConfigSpecs;
	break;
    case BLT_PAINTBRUSH_TEXTURE:
	cmdPtr->brush.type = BLT_PAINTBRUSH_TEXTURE;
	cmdPtr->configSpecs = textureConfigSpecs;
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
ConfigurePaintbrushCmd(Tcl_Interp *interp, PaintbrushCmd *cmdPtr, int objc, 
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
 * paintbrush create type ?name? ?option values?...
 */
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    PaintbrushCmdInterpData *dataPtr = clientData;
    PaintbrushCmd *cmdPtr;
    Blt_PaintbrushType type;
    const char *string;
    Blt_HashEntry *hPtr;

    if (GetPaintTypeFromObj(interp, objv[2], &type) != TCL_OK) {
	return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]);
    if (string[0] == '-') {		/* Generate a unique name for the
					 * paintbrush.  */
	int isNew;
	char name[200];

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
    cmdPtr = NewPaintbrushCmd(dataPtr, interp, type);
    if (cmdPtr == NULL) {
	return TCL_ERROR;
    }
    cmdPtr->refCount = 1;
    Blt_SetHashValue(hPtr, cmdPtr);
    cmdPtr->hashPtr = hPtr;
    cmdPtr->name = Blt_GetHashKey(&dataPtr->instTable, hPtr);

    if (ConfigurePaintbrushCmd(interp, cmdPtr, objc-3, objv+3, 0) != TCL_OK) {
	DestroyPaintbrushCmd(cmdPtr);
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), cmdPtr->name, -1);
    return TCL_OK;
}    

/*
 * paintbrush cget $brush ?option?...
 */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    PaintbrushCmdInterpData *dataPtr = clientData;
    PaintbrushCmd *cmdPtr;

    if (GetPaintbrushCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, cmdPtr->tkwin, 
	cmdPtr->configSpecs, (char *)cmdPtr, objv[3], 0);
}

/*
 * paintbrush configure $brush ?option?...
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    PaintbrushCmdInterpData *dataPtr = clientData;
    PaintbrushCmd *cmdPtr;
    int flags;

    if (GetPaintbrushCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
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
    if (ConfigurePaintbrushCmd(interp, cmdPtr, objc-3, objv+3, flags)!=TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * bgpattern delete $pattern... 
 */
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    PaintbrushCmdInterpData *dataPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
	Blt_HashEntry *hPtr;
	PaintbrushCmd *cmdPtr;
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
	DestroyPaintbrushCmd(cmdPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * NamesOp --
 *
 *	paintbrush names ?pattern?
 *
 *---------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    PaintbrushCmdInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&dataPtr->instTable, &iter);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	PaintbrushCmd *cmdPtr;
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
 * paintbrush type $brush
 */
static int
TypeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    PaintbrushCmdInterpData *dataPtr = clientData;
    PaintbrushCmd *cmdPtr;

    if (GetPaintbrushCmdFromObj(interp, dataPtr, objv[2], &cmdPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), NameOfPaintbrushType(cmdPtr),-1);
    return TCL_OK;
}

static Blt_OpSpec paintbrushOps[] =
{
    {"cget",      2, CgetOp,      4, 4, "brush option",},
    {"configure", 2, ConfigureOp, 3, 0, "brush ?option value?...",},
    {"create",    2, CreateOp,    3, 0, "type ?args?",},
    {"delete",    1, DeleteOp,    2, 0, "brush...",},
    {"names",     1, NamesOp,     2, 3, "brush ?pattern?",},
    {"type",      1, TypeOp,      3, 3, "brush",},
};
static int numPaintbrushOps = sizeof(paintbrushOps) / sizeof(Blt_OpSpec);

static int
PaintbrushCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
		 Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numPaintbrushOps, paintbrushOps, 
	BLT_OP_ARG1, objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

static void
PaintbrushCmdDeleteProc(ClientData clientData) 
{
    PaintbrushCmdInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&dataPtr->instTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	PaintbrushCmd *cmdPtr;

	cmdPtr = Blt_GetHashValue(hPtr);
	cmdPtr->hashPtr = NULL;
	Blt_Free(cmdPtr);
    }
    Blt_DeleteHashTable(&dataPtr->instTable);
    Tcl_DeleteAssocData(dataPtr->interp, PAINTBRUSH_THREAD_KEY);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetPaintbrushCmdInterpData --
 *
 *---------------------------------------------------------------------------
 */
static PaintbrushCmdInterpData *
GetPaintbrushCmdInterpData(Tcl_Interp *interp)
{
    PaintbrushCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (PaintbrushCmdInterpData *)
	Tcl_GetAssocData(interp, PAINTBRUSH_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
	dataPtr = Blt_AssertMalloc(sizeof(PaintbrushCmdInterpData));
	dataPtr->interp = interp;
	dataPtr->nextId = 1;


	/* FIXME: Create interp delete proc to teardown the hash table and
	 * data entry.  Must occur after all the widgets have been destroyed
	 * (clients of the background pattern). */

	Tcl_SetAssocData(interp, PAINTBRUSH_THREAD_KEY, 
		(Tcl_InterpDeleteProc *)NULL, dataPtr);
	Blt_InitHashTable(&dataPtr->instTable, BLT_STRING_KEYS);
    }
    return dataPtr;
}

/*LINTLIBRARY*/
int
Blt_PaintbrushCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {
	"paintbrush", PaintbrushCmdProc, PaintbrushCmdDeleteProc,
    };
    cmdSpec.clientData = GetPaintbrushCmdInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}


void
Blt_Paintbrush_Init(Blt_Paintbrush *brushPtr)
{
    memset(brushPtr, 0, sizeof(Blt_Paintbrush));
    brushPtr->type = BLT_PAINTBRUSH_SOLID;
    brushPtr->alpha = 0xFF;
}

void
Blt_Paintbrush_SetPalette(Blt_Paintbrush *brushPtr, Blt_Palette palette)
{
    brushPtr->type = BLT_PAINTBRUSH_GRADIENT;
    brushPtr->palette = palette;
}

void
Blt_Paintbrush_SetOrigin(Blt_Paintbrush *brushPtr, int x, int y)
{
    brushPtr->xOrigin = x;
    brushPtr->yOrigin = y;
}

void 
Blt_Paintbrush_SetTile(Blt_Paintbrush *brushPtr, Blt_Picture picture)
{
    brushPtr->type = BLT_PAINTBRUSH_TILE;
    brushPtr->tile = picture;
}

void 
Blt_Paintbrush_SetColor(Blt_Paintbrush *brushPtr, unsigned int value)
{
    brushPtr->type = BLT_PAINTBRUSH_SOLID;
    brushPtr->solidColor.u32 = value;
}

void
Blt_Paintbrush_SetTexture(Blt_Paintbrush *brushPtr)
{
    brushPtr->type = BLT_PAINTBRUSH_TEXTURE;
    brushPtr->colorProc = TextureColorProc;
}

void
Blt_Paintbrush_SetGradient(Blt_Paintbrush *brushPtr)
{
    brushPtr->type = BLT_PAINTBRUSH_GRADIENT;
    brushPtr->colorProc = GradientColorProc;
}

void
Blt_Paintbrush_SetColors(Blt_Paintbrush *brushPtr, Blt_Pixel *lowPtr, Blt_Pixel *highPtr)
{
    brushPtr->low.u32 = lowPtr->u32;
    brushPtr->high.u32 = highPtr->u32;
}

void
Blt_Paintbrush_Region(Blt_Paintbrush *brushPtr, int x, int y, int w, int h)
{
    brushPtr->xOrigin = x;
    brushPtr->yOrigin = y;
    if ((brushPtr->type == BLT_PAINTBRUSH_GRADIENT) || 
	(brushPtr->type == BLT_PAINTBRUSH_TEXTURE)) {
	brushPtr->rRange = brushPtr->high.Red   - brushPtr->low.Red;
	brushPtr->gRange = brushPtr->high.Green - brushPtr->low.Green;
	brushPtr->bRange = brushPtr->high.Blue  - brushPtr->low.Blue;
	brushPtr->aRange = brushPtr->high.Alpha - brushPtr->low.Alpha;
	if (brushPtr->palette != NULL) {
	    Blt_Palette_SetRange(brushPtr->palette, 0.0, 1.0);
	}
	if (brushPtr->jitter.range > 0.0) {
	    JitterInit(&brushPtr->jitter);
	}
    }
    if (brushPtr->type == BLT_PAINTBRUSH_GRADIENT) {
	Blt_Gradient *gradPtr;

	gradPtr = &brushPtr->gradient;
	switch (gradPtr->type) {
	case BLT_GRADIENT_TYPE_HORIZONTAL:
	    gradPtr->scaleFactor = 0.0;
	    if (w > 1) {
		gradPtr->scaleFactor = 1.0 / (w - 1);
	    } 
	    break;
	default:
	case BLT_GRADIENT_TYPE_VERTICAL:
	    gradPtr->scaleFactor = 0.0;
	    if (h > 1) {
		gradPtr->scaleFactor = 1.0 / (h - 1);
	    } 
	    break;
	case BLT_GRADIENT_TYPE_DIAGONAL_UP:
	case BLT_GRADIENT_TYPE_DIAGONAL_DOWN:
	    gradPtr->xOffset = w * 0.5;
	    gradPtr->yOffset = h * 0.5;
	    gradPtr->length = sqrt(w * w + h * h);
	    gradPtr->cosTheta = w / gradPtr->length;
	    gradPtr->sinTheta = h / gradPtr->length;
	    if (gradPtr->type == BLT_GRADIENT_TYPE_DIAGONAL_DOWN) {
		gradPtr->sinTheta = -gradPtr->sinTheta;
	    }
	    gradPtr->scaleFactor = 0.0;
	    if (gradPtr->length > 1) {
		gradPtr->scaleFactor = 1.0 / (gradPtr->length - 1);
	    } 
	    break;
	case BLT_GRADIENT_TYPE_RADIAL:
	    gradPtr->xOffset = w * 0.5;
	    gradPtr->yOffset = h * 0.5;
	    gradPtr->length = sqrt(w * w + h * h);
	    gradPtr->scaleFactor = 0.0;
	    if (gradPtr->length > 1) {
		gradPtr->scaleFactor = 1.0 / ((gradPtr->length * 0.5) - 1);
	    } 
	    break;
	case BLT_GRADIENT_TYPE_CONICAL:
	    gradPtr->xOffset = w * 0.5;
	    gradPtr->yOffset = h * 0.5;
	    gradPtr->angle = 45 * DEG2RAD;
	    break;
	}
    }
}

void 
Blt_Paintbrush_SetColorProc(Blt_Paintbrush *brushPtr, 
			    Blt_Paintbrush_ColorProc *proc, 
			    ClientData clientData)
{
    brushPtr->type = BLT_PAINTBRUSH_GRADIENT;
    brushPtr->colorProc = proc;
    brushPtr->clientData = clientData;
}

int
Blt_Paintbrush_GetColor(Blt_Paintbrush *brushPtr, int x, int y)
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
	x = (x - brushPtr->xOrigin) % Blt_PictureWidth(brushPtr->tile);
	if (x < 0) {
	    x = -x;
	}
	y = (y - brushPtr->yOrigin) % Blt_PictureHeight(brushPtr->tile);
	if (y < 0) {
	    y = -y;
	}
	pixelPtr = Blt_PicturePixel(brushPtr->tile, x, y);
	color.u32 = pixelPtr->u32;
	color.Alpha = brushPtr->alpha;
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
	return (*brushPtr->colorProc)(brushPtr, x, y);
    }
    return 0;
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_Paintbrush_Free
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
Blt_Paintbrush_Free(Blt_Paintbrush *brushPtr)
{
    PaintbrushCmd *cmdPtr = (PaintbrushCmd *)brushPtr;

    assert(cmdPtr != NULL);
    cmdPtr->refCount--;
    if (cmdPtr->refCount <= 0) {
	DestroyPaintbrushCmd(cmdPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Paintbrush_GetFromString --
 *
 *	Retrieves the paintbrush object named by the given the string.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Paintbrush_GetFromString(Tcl_Interp *interp, const char *string, 
			Blt_Paintbrush **brushPtrPtr)
{
    Blt_HashEntry *hPtr;
    PaintbrushCmd *cmdPtr;
    PaintbrushCmdInterpData *dataPtr;
    int isNew;

    dataPtr = GetPaintbrushCmdInterpData(interp);
    hPtr = Blt_CreateHashEntry(&dataPtr->instTable, string, &isNew);
    if (isNew) { 
	Blt_Pixel color;

	/* Paintbrush doesn't already exist, see if it's a color name
	 * (i.e. something that Tk_Get3DBorder will accept). */
	if (Blt_GetPixel(interp, string, &color) != TCL_OK) {
	    goto error;			/* Nope. It's an error. */
	} 
	cmdPtr = NewPaintbrushCmd(dataPtr, interp, BLT_PAINTBRUSH_SOLID);
	if (cmdPtr == NULL) {
	    goto error;			/* Can't allocate new color. */
	}
	cmdPtr->brush.solidColor.u32 = color.u32;
	cmdPtr->refCount = 1;
	cmdPtr->hashPtr = hPtr;
	cmdPtr->name = Blt_GetHashKey(&dataPtr->instTable, hPtr);
	Blt_SetHashValue(hPtr, cmdPtr);
    } else {
	cmdPtr = Blt_GetHashValue(hPtr);
	cmdPtr->refCount++;
	assert(cmdPtr != NULL);
    }
    hPtr = Blt_FindHashEntry(&dataPtr->instTable, string);
    if (hPtr == NULL) {
	Tcl_AppendResult(dataPtr->interp, "can't find paintbrush \"", 
		string, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    cmdPtr = Blt_GetHashValue(hPtr);
    *brushPtrPtr = (Blt_Paintbrush *)cmdPtr;
    return TCL_OK;
 error:
    Blt_DeleteHashEntry(&dataPtr->instTable, hPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Paintbrush_Get --
 *
 *	Retrieves the paintbrush command named by the given the Tcl_Obj.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_Paintbrush_Get(Tcl_Interp *interp, Tcl_Obj *objPtr, 
		   Blt_Paintbrush **brushPtrPtr)
{
    const char *string;

    string = Tcl_GetString(objPtr);
    return Blt_Paintbrush_GetFromString(interp, string, brushPtrPtr);
}

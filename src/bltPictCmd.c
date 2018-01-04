/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictCmd.c --
 *
 * This module implements the Tk image interface for picture images.
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
#include <X11/Xutil.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltString.h"
#include <bltChain.h>
#include <bltHash.h>
#include <bltImage.h>
#include <bltDBuffer.h>
#include "bltSwitch.h"
#include "bltPicture.h"
#include "bltPictInt.h"
#include "bltPictFmts.h"
#include "bltBg.h"
#include "bltPainter.h"
#include "bltPs.h"
#include "bltOp.h"
#include "bltInitCmd.h"


typedef struct _Blt_DBuffer DBuffer;

typedef int (PictCmdProc)(Blt_Picture picture, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv);

#if (_TCL_VERSION < _VERSION(8,2,0)) 
typedef struct _Tk_PostscriptInfo *Tk_PostscriptInfo;
#endif

/*
 * PictureCmdInterpData --
 *
 *      Structure containing global data, used on a interpreter by
 *      interpreter basis.
 *
 *      This structure holds the hash table of instances of datatable
 *      commands associated with a particular interpreter.
 */
typedef struct {
    Tcl_Interp *interp;
} PictureCmdInterpData;


#define FMT_LOADED      (1<<0)
#define FMT_STATIC      (1<<1)
#define FMT_ASCII       (1<<2)

/*
 * Various external file/string formats handled by the picture image.
 */

enum PictureFormats {
#ifdef HAVE_LIBJPG
    FMT_JPG,                            /* Joint Photographic Experts Group
                                         * r/w */
#endif
#ifdef HAVE_LIBPNG
    FMT_PNG,                            /* Portable Network Graphics r/w */
#endif
#ifdef HAVE_LIBTIF
    FMT_TIF,                            /* Tagged Image File Format r/w */
#endif
#ifdef HAVE_LIBXPM
    FMT_XPM,                            /* X Pixmap r/w */
#endif
    FMT_XBM,                            /* X Bitmap r/w */
    FMT_GIF,                            /* Graphics Interchange Format r/w */
    FMT_PS,                             /* PostScript r/w */
    FMT_PDF,                            /* Portable Document Format r/w */
    FMT_BMP,                            /* Device-independent bitmap r/w */
    FMT_PBM,                            /* Portable Bitmap Format r/w */
#ifdef WIN32
    FMT_EMF,                            /* Enhanced Metafile Format r/w
                                         * (Windows only) TBA */
    FMT_WMF,                            /* Windows Metafile Format r/w
                                         * (Windows only) TBA */
#endif
    FMT_TGA,                            /* Targa Image File Format r/w */
    FMT_PHO,                            /* Tk Photo Image r/w */
    FMT_ICO,                            /* Window icon/cursor bitmap r/w */
    NUMFMTS
};


static Blt_PictFormat pictFormats[] = {
#ifdef HAVE_LIBJPG
    { "jpg" },
#endif
#ifdef HAVE_LIBPNG
    { "png" },
#endif
#ifdef HAVE_LIBTIF
    { "tif" },                          /* Multi-page */
#endif
#ifdef HAVE_LIBXPM
    { "xpm" },
#endif
    { "xbm" },
    { "gif" },                          /* Multi-page */
    { "ps"  },                          /* Multi-page */
    { "pdf" },                          /* Not implemented yet. */
    { "photo" },
    { "bmp" },
    { "pbm" },                          /* Multi-page */
#ifdef WIN32
    { "emf" },
    { "wmf" },
#endif
    { "tga" },
    { "ico" },
};

static Blt_HashTable fmtTable;


typedef struct {
    const char *name;                   /* Name of procedure package. */
    Blt_HashEntry *hashPtr;
    Tcl_ObjCmdProc *proc;
} PictProc;

static Blt_HashTable procTable;

/*
 * Default configuration options for picture images. 
 */
#define DEF_ANGLE               "0.0"
#define DEF_MAXPECT             "0"
#define DEF_CACHE               "0"
#define DEF_DITHER              "0"
#define DEF_DATA                (char *)NULL
#define DEF_FILE                (char *)NULL
#define DEF_FILTER              (char *)NULL
#define DEF_GAMMA               "1.0"
#define DEF_HEIGHT              "0"
#define DEF_WIDTH               "0"
#define DEF_WINDOW              (char *)NULL
#define DEF_IMAGE               (char *)NULL
#define DEF_SHARPEN             "0"

#define DEF_OPAQUE              "0"
#define DEF_OPAQUE_BACKGROUND   "white"

#define IMPORTED_NONE           0
#define IMPORTED_FILE           (1<<0)
#define IMPORTED_IMAGE          (1<<1)
#define IMPORTED_WINDOW         (1<<2)
#define IMPORTED_DATA           (1<<3)
#define IMPORTED_MASK   \
    (IMPORTED_FILE|IMPORTED_IMAGE|IMPORTED_WINDOW|IMPORTED_DATA)

#define NOTIFY_PENDING          (1<<8)

#define MAXPECT                 (1<<9)  /* Maintain the aspect ratio while
                                         * scaling. The larger dimension is
                                         * discarded. */
#define DITHER                  (1<<10) /* Dither the picture before
                                         * drawing. */
#define SHARPEN                 (1<<12) /* Sharpen the image. */

#define RAISE                   (1<<14) 

/*
 * PictImage -- 
 *
 *      A PictImage implements a Tk_ImageMaster for the Blt_Picture image
 *      type.  It represents a set of bits (i.e. the picture), some
 *      options, and operations (sub-commands) to manipulate the picture.
 *
 *      The PictImage manages the TCL interface to a Blt_Picture (using
 *      Tk's "image" command).  Pictures and the mechanics of drawing the
 *      picture to the display (painters) are orthogonal.  The PictImage
 *      knows nothing about the display type (the display is stored only to
 *      free options when it's destroyed).  Information specific to the
 *      visual context (combination of display, visual, depth, colormap,
 *      and gamma) is stored in each cache entry.  The picture image
 *      manages the various picture transformations: reading, writing,
 *      scaling, rotation, etc.
 */
typedef struct _Blt_PictureImage {
    Tk_ImageMaster imgToken;            /* Tk's token for image master.  If
                                         * NULL, the image has been
                                         * deleted. */
    Tcl_Interp *interp;                 /* Interpreter associated with the
                                         * application using this image. */
    Display *display;                   /* Display associated with this
                                         * picture image.  This is used to
                                         * free the configuration
                                         * options. */
    Colormap colormap;
    Tcl_Command cmdToken;               /* Token for image command (used to
                                         * delete the command when the
                                         * image goes away).  NULL means
                                         * the image command has already
                                         * been deleted. */
    unsigned int flags;                 /* Various bit-field flags defined
                                         * below. */
    Blt_Chain chain;                    /* List of pictures. (multi-page
                                         * formats) */
    Blt_Picture picture;                /* Current picture displayed. */
    
    /* User-requested options. */
    float angle;                        /* Angle in degrees to rotate the
                                         * image. */
    int reqWidth, reqHeight;            /* User-requested size of
                                         * picture. The picture is scaled
                                         * accordingly. */
    Blt_ResampleFilter filter;          /* 1D Filter to use when the
                                         * picture is resampled
                                         * (resized). The same filter is
                                         * applied both horizontally and
                                         * vertically. */
    float gamma;                        /* Gamma correction value of the
                                         * monitor. In theory, the same
                                         * picture image may be displayed
                                         * on two monitors simultaneously
                                         * (using xinerama).  Here we're
                                         * assuming (almost certainly
                                         * wrong) that both monitors will
                                         * have the same gamma value. */
    const char *sourceName;             /* Name of the image, file, or
                                         * window read into the picture
                                         * image. */
    int current;                        /* Index of the picture in the
                                         * above list. */
    Tcl_TimerToken timerToken;          /* Token for timer handler for
                                         * sequences. */
    int interval;
    int lastIndex;
    Blt_PictFormat *fmtPtr;             /* External format of last image
                                         * read into the picture image. We
                                         * use this to write back the same
                                         * format if the user doesn't
                                         * specify the format. */
    Blt_HashTable cacheTable;           /* Table of cache entries specific
                                         * to each visual context where
                                         * this picture is displayed. */
    ClientData clientData;              /* Holder for transition
                                         * information. */
} PictImage;


/*
 * PictCacheKey -- 
 *
 *      Represents the visual context of a cache entry. type.  It contains
 *      information specific to the visual context (combination of display,
 *      visual, depth, colormap, and gamma).  It is used as a hash table
 *      key for cache entries of picture images.  The same picture may be
 *      displayed in more than one visual context.
 */
typedef struct {
    Display *display;                   /* Display where the picture will
                                         * be drawn. Used to free colors
                                         * allocated by the painter. */
    Visual *visualPtr;                  /* Visual information of window
                                         * displaying the image. */
    Colormap colormap;                  /* Colormap used.  This may be the
                                         * default colormap, or an
                                         * allocated private map. */
    int depth;                          /* Depth of the display. */
    int index;                          /* Index of the picture in the
                                         * list. */
    float gamma;                        /* Gamma correction value */
} PictCacheKey;


/*
 * PictInstances -- 
 *
 *      PictInstances (image instances in the Tk parlance) represent a
 *      picture image in some specific combination of visual, display,
 *      colormap, depth, and output gamma.  Cache entries are stored by
 *      each picture image. The purpose is to allocate and hold the 
 *      painter-specific to the visual.
 */
typedef struct {
    Blt_PictureImage image;             /* The picture image represented by
                                         * this entry. */
    Blt_Painter painter;                /* The painter allocated for this
                                         * particular combination of
                                         * visual, display, colormap,
                                         * depth, and gamma. */
    Display *display;                   
    Blt_HashEntry *hashPtr;             /* These two fields allow the
                                         * cache */
    Blt_HashTable *tablePtr;            /* Entry to be deleted from the
                                         * picture image's table of
                                         * entries. */
    int refCount;                       /* This entry may be shared by all
                                         * clients displaying this picture
                                         * image with the same painter. */
    unsigned int flags;
} PictInstance;

typedef struct {
    PictImage *imgPtr;
    Tcl_TimerToken timerToken;          /* Token for timer handler for
                                         * transition. */
    Blt_SwitchSpec *specs;
    Blt_Picture from, to;               /* From and to pictures. */
    Blt_Picture picture;                /* Holds the result. */
    int logScale;
    int interval;                       /* # of milliseconds delay between
                                         * steps. */
    int numSteps;                       /* # of steps. */
    int count;                          /* Current step. */
    Tcl_Interp *interp;                 /* Interpreter used to set
                                         * variable.  */
    Tcl_Obj *varNameObjPtr;             /* If non-NULL, contains name of
                                         * variable to set when transition
                                         * is completed. */
    /* Crossfade and Disolve fields. */
    Blt_Pixel fromColor;                /* Defaults to white. */
    Blt_Pixel toColor;                  /* Defaults to black. */

    /* Dissolve-specific fields. */
    long numPixels;                     /* ! # pixels in each step.  */
    long last;                          /* ! Last position in dissolve. */
    /* Wipe-specific fields. */
    int direction;
} Transition;

static Blt_OptionParseProc ObjToFile;
static Blt_OptionPrintProc FileToObj;
static Blt_CustomOption fileOption =
{
    ObjToFile, FileToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToData;
static Blt_OptionPrintProc DataToObj;
static Blt_CustomOption dataOption =
{
    ObjToData, DataToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToFilter;
static Blt_OptionPrintProc FilterToObj;
Blt_CustomOption bltFilterOption =
{
    ObjToFilter, FilterToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToGamma;
static Blt_OptionPrintProc GammaToObj;
Blt_CustomOption gammaOption =
{
    ObjToGamma, GammaToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToImage;
static Blt_OptionPrintProc ImageToObj;
static Blt_CustomOption imageOption =
{
    ObjToImage, ImageToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToWindow;
static Blt_OptionPrintProc WindowToObj;
static Blt_CustomOption windowOption =
{
    ObjToWindow, WindowToObj, NULL, (ClientData)0
};

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-data", (char *)NULL, (char *)NULL, DEF_DATA, 
        Blt_Offset(PictImage, picture), 0, &dataOption},
    {BLT_CONFIG_BITMASK, "-dither", (char *)NULL, (char *)NULL, 
        DEF_DITHER, Blt_Offset(PictImage, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)DITHER},
    {BLT_CONFIG_CUSTOM, "-file", (char *)NULL, (char *)NULL, DEF_DATA, 
        Blt_Offset(PictImage, picture), 0, &fileOption},
    {BLT_CONFIG_CUSTOM, "-filter", (char *)NULL, (char *)NULL, 
        DEF_FILTER, Blt_Offset(PictImage, filter), 0, &bltFilterOption},
    {BLT_CONFIG_CUSTOM, "-gamma", (char *)NULL, (char *)NULL, DEF_GAMMA,
        Blt_Offset(PictImage, gamma), BLT_CONFIG_DONT_SET_DEFAULT,
        &gammaOption},
   {BLT_CONFIG_PIXELS_NNEG, "-height", (char *)NULL, (char *)NULL,
        DEF_HEIGHT, Blt_Offset(PictImage, reqHeight), 0},
    {BLT_CONFIG_CUSTOM, "-image", (char *)NULL, (char *)NULL, DEF_IMAGE,
        Blt_Offset(PictImage, picture), 0, &imageOption},
    {BLT_CONFIG_BITMASK, "-maxpect", (char *)NULL, (char *)NULL, 
        DEF_MAXPECT, Blt_Offset(PictImage, flags),
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)MAXPECT},
    {BLT_CONFIG_FLOAT, "-rotate", (char *)NULL, (char *)NULL, 
        DEF_ANGLE, Blt_Offset(PictImage, angle), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-sharpen", (char *)NULL, (char *)NULL, 
        DEF_SHARPEN, Blt_Offset(PictImage, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SHARPEN},
    {BLT_CONFIG_PIXELS_NNEG, "-width", (char *)NULL, (char *)NULL,
        DEF_WIDTH, Blt_Offset(PictImage, reqWidth), 0},
    {BLT_CONFIG_CUSTOM, "-window", (char *)NULL, (char *)NULL, 
        DEF_WINDOW, Blt_Offset(PictImage, picture), 0, &windowOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_SwitchParseProc ColorSwitchProc;
static Blt_SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc AreaSwitchProc;
static Blt_SwitchCustom areaSwitch = {
    AreaSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc FilterSwitchProc;
static Blt_SwitchCustom filterSwitch = {
    FilterSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc PixelsSwitchProc;
static Blt_SwitchCustom pixelsSwitch = {
    PixelsSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc BlendingModeSwitchProc;
static Blt_SwitchCustom blendModeSwitch = {
    BlendingModeSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc DirectionSwitchProc;
static Blt_SwitchCustom directionSwitch =
{
    DirectionSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc PercentSwitchProc;
static Blt_SwitchCustom percentSwitch =
{
    PercentSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc ScaleSwitchProc;
static Blt_SwitchCustom scaleSwitch =
{
    ScaleSwitchProc, NULL, NULL, (ClientData)0
};

typedef struct {
    int invert;                         /* Flag. */
    Tcl_Obj *maskObjPtr;
} ArithSwitches;

static Blt_SwitchSpec arithSwitches[] = 
{
    {BLT_SWITCH_BITS_NOARG, "-invert", "", (char *)NULL,
        Blt_Offset(ArithSwitches, invert), 0, TRUE},
    {BLT_SWITCH_OBJ,     "-mask",   "mask", (char *)NULL,
        Blt_Offset(ArithSwitches, maskObjPtr), 0},
    {BLT_SWITCH_END}
};

typedef struct {
    PictArea from;                      /* Area to crop. */
    int nocopy;                         /* If non-zero, don't copy the
                                         * source image. */
} DupSwitches;

static Blt_SwitchSpec dupSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-bbox", "bbox", (char *)NULL,
        Blt_Offset(DupSwitches, from), 0, 0, &areaSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    PictArea from, to;
} CompositeSwitches;

static Blt_SwitchSpec compositeSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-from", "bbox", (char *)NULL,
        Blt_Offset(CompositeSwitches,from), 0, 0, &areaSwitch},
    {BLT_SWITCH_CUSTOM, "-to",   "bbox",  (char *)NULL,
        Blt_Offset(CompositeSwitches, to),  0, 0, &areaSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    PictArea from, to;
    Blt_BlendingMode mode;              /* Blending mode. */
} ColorBlendSwitches;

static Blt_SwitchSpec colorBlendSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-mode", "blendingMode", (char *)NULL,
        Blt_Offset(ColorBlendSwitches, mode), 0, 0, &blendModeSwitch},
    {BLT_SWITCH_CUSTOM, "-from", "bbox", (char *)NULL,
        Blt_Offset(ColorBlendSwitches,from), 0, 0, &areaSwitch},
    {BLT_SWITCH_CUSTOM, "-to",   "bbox",  (char *)NULL,
        Blt_Offset(ColorBlendSwitches, to),  0, 0, &areaSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_ResampleFilter vFilter;         /* Color of rectangle. */
    Blt_ResampleFilter hFilter;         /* Width of outline. */
    Blt_ResampleFilter filter;
} ConvolveSwitches;

static Blt_SwitchSpec convolveSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-filter",  "filter", (char *)NULL,
        Blt_Offset(ConvolveSwitches, filter),  0, 0, &filterSwitch},
    {BLT_SWITCH_CUSTOM, "-hfilter", "filter", (char *)NULL,
        Blt_Offset(ConvolveSwitches, hFilter), 0, 0, &filterSwitch},
    {BLT_SWITCH_CUSTOM, "-vfilter", "filter", (char *)NULL,
        Blt_Offset(ConvolveSwitches, vFilter), 0, 0, &filterSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    PictArea from, to;
    int composite;
} CopySwitches;

static Blt_SwitchSpec copySwitches[] = 
{
    {BLT_SWITCH_BOOLEAN,"-composite", "", (char *)NULL,
        Blt_Offset(CopySwitches, composite), 0, 0},
    {BLT_SWITCH_CUSTOM, "-from", "bbox", (char *)NULL,
        Blt_Offset(CopySwitches,from), 0, 0, &areaSwitch},
    {BLT_SWITCH_CUSTOM, "-to",   "bbox", (char *)NULL,
        Blt_Offset(CopySwitches, to),  0, 0, &areaSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    int blur;
    int side;
    double high;
    double low;
    Blt_Pixel bgColor;
    Blt_Jitter jitter;
    ScaleType scale;
    float size;                         /* Ratio */
} ReflectSwitches;

static Blt_SwitchSpec reflectSwitches[] = {
    {BLT_SWITCH_CUSTOM, "-background", "color", (char *)NULL,
        Blt_Offset(ReflectSwitches, bgColor), 0, 0, &colorSwitch},
    {BLT_SWITCH_CUSTOM, "-bg", "color", (char *)NULL,
        Blt_Offset(ReflectSwitches, bgColor), 0, 0, &colorSwitch},
    {BLT_SWITCH_INT, "-blur", "level", (char *)NULL,
        Blt_Offset(ReflectSwitches, blur), 0, 0},
    {BLT_SWITCH_CUSTOM, "-low", "alpha", (char *)NULL,
        Blt_Offset(ReflectSwitches, low), 0, 0, &percentSwitch},
    {BLT_SWITCH_CUSTOM, "-high", "alpha", (char *)NULL,
        Blt_Offset(ReflectSwitches, high), 0, 0, &percentSwitch},
    {BLT_SWITCH_CUSTOM, "-jitter", "percent", (char *)NULL,
        Blt_Offset(ReflectSwitches, jitter.range), 0, 0, &percentSwitch},
    {BLT_SWITCH_FLOAT, "-ratio", "", (char *)NULL,
        Blt_Offset(ReflectSwitches, size), 0, 0},
    {BLT_SWITCH_CUSTOM, "-scale", "how", (char *)NULL,
       Blt_Offset(ReflectSwitches, scale), 0, 0, &scaleSwitch},
    {BLT_SWITCH_SIDE, "-side", "side", (char *)NULL,
        Blt_Offset(ReflectSwitches, side), 0, 0},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_ResampleFilter filter, vFilter, hFilter;
    PictArea from;
    int width, height;
    int flags;
} ResampleSwitches;

static Blt_SwitchSpec resampleSwitches[] = {
    {BLT_SWITCH_CUSTOM, "-filter", "filter", (char *)NULL,
        Blt_Offset(ResampleSwitches, filter), 0, 0, &filterSwitch},
    {BLT_SWITCH_CUSTOM, "-from",   "bbox", (char *)NULL,
        Blt_Offset(ResampleSwitches, from), 0, 0, &areaSwitch},
    {BLT_SWITCH_CUSTOM, "-height",  "numPixels", (char *)NULL,
        Blt_Offset(ResampleSwitches, height),  0, 0, &pixelsSwitch},
    {BLT_SWITCH_CUSTOM, "-hfilter", "filter", (char *)NULL,
        Blt_Offset(ResampleSwitches, hFilter), 0, 0, &filterSwitch},
    {BLT_SWITCH_BITS_NOARG, "-maxpect", "", (char *)NULL, 
        Blt_Offset(ResampleSwitches, flags), 0, MAXPECT},
    {BLT_SWITCH_CUSTOM, "-vfilter", "filter", (char *)NULL,
        Blt_Offset(ResampleSwitches, vFilter), 0, 0, &filterSwitch},
    {BLT_SWITCH_CUSTOM, "-width",   "numPixels", (char *)NULL,
        Blt_Offset(ResampleSwitches, width),  0, 0, &pixelsSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_ResampleFilter filter, vFilter, hFilter;
    PictArea from;
    int width, height;
    int flags;
} SnapArgs;

static Blt_SwitchSpec snapSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-filter", "filter", (char *)NULL,
        Blt_Offset(SnapArgs, filter), 0, 0, &filterSwitch},
    {BLT_SWITCH_CUSTOM,  "-from", "bbox", (char *)NULL,
        Blt_Offset(SnapArgs, from), 0, 0, &areaSwitch},
    {BLT_SWITCH_CUSTOM, "-height",  "numPixels", (char *)NULL,
        Blt_Offset(SnapArgs, height),  0, 0, &pixelsSwitch},
    {BLT_SWITCH_CUSTOM, "-hfilter", "filter", (char *)NULL,
        Blt_Offset(SnapArgs, hFilter), 0, 0, &filterSwitch},
    {BLT_SWITCH_BITS_NOARG, "-maxpect", "", (char *)NULL, 
        Blt_Offset(ResampleSwitches, flags), 0, MAXPECT},
    {BLT_SWITCH_BITS_NOARG, "-raise",  "", (char *)NULL,
        Blt_Offset(SnapArgs, flags),  0, RAISE},
    {BLT_SWITCH_CUSTOM, "-vfilter", "filter", (char *)NULL,
        Blt_Offset(SnapArgs, vFilter), 0, 0, &filterSwitch},
    {BLT_SWITCH_CUSTOM, "-width",   "numPixels", (char *)NULL,
        Blt_Offset(SnapArgs, width),  0, 0, &pixelsSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_Pixel bg;                       /* Fg and bg colors. */
} ProjectSwitches;

static Blt_SwitchSpec projectSwitches[] = {
    {BLT_SWITCH_CUSTOM, "-background", "color", (char *)NULL,
        Blt_Offset(ProjectSwitches, bg), 0, 0, &colorSwitch},
    {BLT_SWITCH_CUSTOM, "-bg",         "color", (char *)NULL,
        Blt_Offset(ProjectSwitches, bg), 0, 0, &colorSwitch},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec crossFadeTransitionSwitches[] = 
{
    {BLT_SWITCH_INT_NNEG, "-goto", "step", (char *)NULL,
        Blt_Offset(Transition, count), 0},
    {BLT_SWITCH_INT_NNEG, "-delay", "milliseconds", (char *)NULL,
        Blt_Offset(Transition, interval), 0},
    {BLT_SWITCH_BOOLEAN, "-logscale", "bool", (char *)NULL,
        Blt_Offset(Transition, logScale), 0, 0},
    {BLT_SWITCH_INT_POS, "-steps", "numSteps", (char *)NULL,
        Blt_Offset(Transition, numSteps), 0},
    {BLT_SWITCH_OBJ, "-variable", "varName", (char *)NULL,
        Blt_Offset(Transition, varNameObjPtr), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec dissolveTransitionSwitches[] = 
{
    {BLT_SWITCH_INT_NNEG, "-delay", "milliseconds", (char *)NULL,
        Blt_Offset(Transition, interval), 0},
    {BLT_SWITCH_INT_NNEG, "-goto", "step", (char *)NULL,
        Blt_Offset(Transition, count), 0},
    {BLT_SWITCH_BOOLEAN, "-logscale", "bool", (char *)NULL,
        Blt_Offset(Transition, logScale), 0, 0},
    {BLT_SWITCH_INT_POS, "-steps", "numSteps", (char *)NULL,
        Blt_Offset(Transition, numSteps), 0},
    {BLT_SWITCH_OBJ, "-variable", "varName", (char *)NULL,
        Blt_Offset(Transition, varNameObjPtr), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec wipeTransitionSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-direction", "n|s|e|w", (char *)NULL,
        Blt_Offset(Transition, direction), 0, 0, &directionSwitch},
    {BLT_SWITCH_INT_NNEG, "-goto", "step", (char *)NULL,
        Blt_Offset(Transition, count), 0},
    {BLT_SWITCH_INT_NNEG, "-delay", "milliseconds", (char *)NULL,
        Blt_Offset(Transition, interval), 0},
    {BLT_SWITCH_BOOLEAN, "-logscale", "bool", (char *)NULL,
        Blt_Offset(Transition, logScale), 0, 0},
    {BLT_SWITCH_INT_POS, "-steps", "numSteps", (char *)NULL,
        Blt_Offset(Transition, numSteps), 0},
    {BLT_SWITCH_OBJ, "-variable", "varName", (char *)NULL,
        Blt_Offset(Transition, varNameObjPtr), 0},
    {BLT_SWITCH_END}
};

/* 
 * Forward references for TCL command callbacks used below. 
 */
static Tcl_ObjCmdProc PictureInstCmdProc;
static Tcl_CmdDeleteProc PictureInstCmdDeletedProc;
extern Tcl_ObjCmdProc Blt_Picture_CircleOp;
extern Tcl_ObjCmdProc Blt_Picture_EllipseOp;
extern Tcl_ObjCmdProc Blt_Picture_LineOp;
extern Tcl_ObjCmdProc Blt_Picture_PolygonOp;
extern Tcl_ObjCmdProc Blt_Picture_RectangleOp;
extern Tcl_ObjCmdProc Blt_Picture_TextOp;

#ifdef notdef
static Tk_ImageCreateProc CreateProc;
#endif
static Tk_ImageGetProc GetInstanceProc;
static Tk_ImageDisplayProc DisplayProc;
static Tk_ImageFreeProc FreeInstanceProc;
static Tk_ImageDeleteProc DeleteProc;
static Tk_ImagePostscriptProc PostScriptProc;

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
    jitterPtr->range = 0.1;
    jitterPtr->offset = -0.05;          /* Jitter +/-  */
}

#ifdef notdef
static INLINE double 
Jitter(Blt_Jitter *jitterPtr) 
{
    double t;

    t = RandomNumber(&jitterPtr->random);  /* Returns number 0..1 */
    return (t * jitterPtr->range) + jitterPtr->offset;
}
#endif

static Blt_ChainLink
GetFirstImageIndex(PictImage *imgPtr, int *indexPtr)
{
    int i;
    Blt_ChainLink link;
    
    link = Blt_Chain_FirstLink(imgPtr->chain);
    for (i = 0; link != NULL; link = Blt_Chain_NextLink(link), i++) {
        if (link != NULL) {
            *indexPtr = i;
            return link;
        }
    }
    return NULL;
}

static Blt_ChainLink
GetNextImageIndex(PictImage *imgPtr, int index, int *indexPtr)
{
    int i;
    Blt_ChainLink link;
    
    link = Blt_Chain_GetNthLink(imgPtr->chain, index + 1);
    for (i = index + 1; link != NULL; link = Blt_Chain_NextLink(link), i++) {
        if (link != NULL) {
            *indexPtr = i;
            return link;
        }
    }
    return NULL;
}
                   
static Blt_ChainLink
GetPreviousImageIndex(PictImage *imgPtr, int index, int *indexPtr)
{
    int i;
    Blt_ChainLink link;
    
    link = Blt_Chain_GetNthLink(imgPtr->chain, index - 1);
    for (i = index - 1; link != NULL; link = Blt_Chain_PrevLink(link), i--) {
        if (link != NULL) {
            *indexPtr = i;
            return link;
        }
    }
    return NULL;
}

static int
GetSequenceIndexFromObj(Tcl_Interp *interp, PictImage *imgPtr, Tcl_Obj *objPtr,
                        int *indexPtr)
{
    int index, length;
    const char *string;
    char c;
    
    string = Tcl_GetString(objPtr);
    c = string[0];
    length = Blt_Chain_GetLength(imgPtr->chain);
    index = -1;
    if ((c == 'e') && (strcmp(string, "end") == 0)) {
        index = length - 1;
    } else if ((c == 'p') && (strcmp(string, "previous") == 0)) {
        GetPreviousImageIndex(imgPtr, imgPtr->current - 1, &index);
    } else if ((c == 'n') && (strcmp(string, "next") == 0)) {
        GetNextImageIndex(imgPtr, imgPtr->current + 1, &index);
    } else if ((c == 'c') && (strcmp(string, "current") == 0)) {
        index = imgPtr->current;
    } else if (Tcl_GetIntFromObj(interp, objPtr, &index) == TCL_OK) {
        if ((index < 0) || (index >= length)) {
            Tcl_AppendResult(interp, "invalid image index \"", 
                             Tcl_GetString(objPtr), "\"", (char *)NULL);
            return TCL_ERROR;
        }
    } else {
        Tcl_AppendResult(interp, "unknown image index \"", 
                         Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    *indexPtr = index;
    return TCL_OK;
    
}

Blt_Picture
Blt_GetNthPicture(Blt_Chain chain, size_t index)
{
    Blt_ChainLink link;

    link = Blt_Chain_GetNthLink(chain, index);
    if (link == NULL) {
        return NULL;
    }
    return Blt_Chain_GetValue(link);
}

static INLINE void
ResizeCopyPictureBits(Pict *destPtr, Pict *srcPtr)
{
    if ((destPtr->width != srcPtr->width) ||
        (destPtr->height != srcPtr->height)) {
        Blt_ResizePicture(destPtr, srcPtr->width, srcPtr->height);
    }
    Blt_CopyPictureBits(destPtr, srcPtr);
}

static Blt_Picture
CurrentPictureFromPictImage(PictImage *imgPtr)
{
    imgPtr->picture = Blt_GetNthPicture(imgPtr->chain, imgPtr->current);
    return imgPtr->picture;
}

static void
ReplacePicture(PictImage *imgPtr, Blt_Picture picture)
{
    Blt_ChainLink link;

    if (imgPtr->chain == NULL) {
        imgPtr->chain = Blt_Chain_Create();
    }
    link = Blt_Chain_GetNthLink(imgPtr->chain, imgPtr->current);
    if (link == NULL) {
        int n;

        n = Blt_Chain_GetLength(imgPtr->chain);
        link = Blt_Chain_Append(imgPtr->chain, picture);
        imgPtr->current = n;
    } else {
        Blt_Picture old;

        old = Blt_Chain_GetValue(link);
        if ((old != NULL) && (old != picture)) {
            Blt_FreePicture(old);
        }
    }
    Blt_Chain_SetValue(link, picture);
    imgPtr->picture = picture;
}

static void
FreePictures(PictImage *imgPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(imgPtr->chain); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        Blt_Picture picture;
        
        picture = Blt_Chain_GetValue(link);
        if (picture != NULL) {
            Blt_FreePicture(picture);
        }
    }
    Blt_Chain_Destroy(imgPtr->chain);
    imgPtr->chain = NULL;
    imgPtr->current = 0;
    imgPtr->picture = NULL;
}

Blt_Picture
Blt_GetPictureFromPictureImage(Tk_Image tkImage)
{
    PictInstance *instancePtr;
    
    if (!Blt_IsPicture(tkImage)) {
        return NULL;
    }
    instancePtr = Blt_Image_GetInstanceData(tkImage);
    return CurrentPictureFromPictImage(instancePtr->image);
}

Blt_Chain
Blt_GetPicturesFromPictureImage(Tcl_Interp *interp, Tk_Image tkImage)
{
    PictInstance *instancePtr;
    PictImage *imgPtr;

    if (!Blt_IsPicture(tkImage)) {
        Tcl_AppendResult(interp, "image is not a picture", (char *)NULL);
        return NULL;
    }
    instancePtr = Blt_Image_GetInstanceData(tkImage);
    imgPtr = (PictImage *)instancePtr->image;
    return imgPtr->chain;
}

void
Blt_NotifyImageChanged(PictImage *imgPtr)
{
    if (imgPtr->picture != NULL) {
        int w, h;

        w = Blt_Picture_Width(imgPtr->picture);
        h = Blt_Picture_Height(imgPtr->picture);
        Tk_ImageChanged(imgPtr->imgToken, 0, 0, w, h, w, h);
    }
}

unsigned int
Blt_XColorToPixel(XColor *colorPtr)
{
    Blt_Pixel new;

    /* Convert X Color with 3 channel, 16-bit components to Blt_Pixel
     * (8-bit, with alpha component) 0..65356 0..255 */
    new.Red = colorPtr->red / 257;
    new.Green = colorPtr->green / 257;
    new.Blue = colorPtr->blue / 257;
    new.Alpha = ALPHA_OPAQUE;
    return new.u32;
}


int
Blt_GetAreaFromObjv(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, 
                    PictArea *areaPtr)
{
    double x1, y1, x2, y2;

    if ((objc != 2) && (objc != 4)) {
        Tcl_AppendResult(interp, "wrong # elements in bounding box", 
                (char *)NULL);
        return TCL_ERROR;
    }
    areaPtr->x1 = areaPtr->y1 = 0;
    areaPtr->x2 = areaPtr->y2 = -1;
    if ((Tcl_GetDoubleFromObj(interp, objv[0], &x1) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[1], &y1) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (objc == 2) {
        areaPtr->x1 = ROUND(x1), areaPtr->y1 = ROUND(y1);
        areaPtr->flags |= BLT_PICTURE_COORDS;
        return TCL_OK;
    }
    if ((Tcl_GetDoubleFromObj(interp, objv[2], &x2) != TCL_OK) ||
        (Tcl_GetDoubleFromObj(interp, objv[3], &y2) != TCL_OK)) {
        return TCL_ERROR;
    }

    /* Flip the coordinates of the bounding box if necessary so that its
     * the upper-left and lower-right corners */
    if (x1 > x2) {
        double tmp;

        tmp = x1, x1 = x2, x2 = tmp;
    }
    if (y1 > y2) {
        double tmp;

        tmp = y1, y1 = y2, y2 = tmp;
    }
    areaPtr->flags |= BLT_PICTURE_COORDS | BLT_PICTURE_SIZE;
    y1 = floor(y1), x1 = floor(x1);
    y2 = ceil(y2), x2 = ceil(x2);
    areaPtr->x1 = (int)x1, areaPtr->y1 = (int)y1;
    /* Opposite corner */
    areaPtr->x2 = (int)x2, areaPtr->y2 = (int)y2;
    return TCL_OK;
}

static int
GetAreaFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, PictArea *areaPtr)
{
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    return Blt_GetAreaFromObjv(interp, objc, objv, areaPtr);
}

static void
InitArea(Blt_Picture picture, PictArea *areaPtr)
{
    areaPtr->x1 = areaPtr->y1 = 0;
    areaPtr->x2 = Blt_Picture_Width(picture);
    areaPtr->y2 = Blt_Picture_Height(picture);
    areaPtr->flags = 0;
}

int
Blt_AdjustAreaToPicture(Blt_Picture picture, PictArea *areaPtr)
{
    int w, h;

    w = Blt_Picture_Width(picture);
    h = Blt_Picture_Height(picture);

    if ((areaPtr->x2 == -1) || (areaPtr->x2 > w)) {
        areaPtr->x2 = w;
    }
    if ((areaPtr->y2 == -1) || (areaPtr->y2 > h)) {
        areaPtr->y2 = h;
    }

    /* Verify that some part of the bounding box is actually inside the
     * picture. */
    if ((areaPtr->x1 >= w) || (areaPtr->x2 <= 0) ||
        (areaPtr->y1 >= h) || (areaPtr->y2 <= 0)) {
        return FALSE;
    }
    /* If needed, adjust the bounding box so that it resides totally inside
     * the picture */
    if (areaPtr->x1 < 0) {
        areaPtr->x1 = 0;
        areaPtr->flags |= BLT_PICTURE_SIZE | BLT_PICTURE_COORDS;
    } 
    if (areaPtr->y1 < 0) {
        areaPtr->y1 = 0;
        areaPtr->flags |= BLT_PICTURE_SIZE | BLT_PICTURE_COORDS;
    }
    if (areaPtr->x2 > w) {
        areaPtr->x2 = w;
        areaPtr->flags |= BLT_PICTURE_SIZE;
    }
    if (areaPtr->y2 > h) {
        areaPtr->y2 = h;
        areaPtr->flags |= BLT_PICTURE_SIZE;
    }
    return TRUE;
}

int
Blt_ResetPicture(Tcl_Interp *interp, const char *imageName, Blt_Picture picture)
{
    Tcl_CmdInfo cmdInfo;

    if (Tcl_GetCommandInfo(interp, imageName, &cmdInfo)) {
        if (cmdInfo.objProc == PictureInstCmdProc) {
            PictImage *imgPtr;

            imgPtr = cmdInfo.objClientData;
            if (imgPtr->picture != picture) {
                ReplacePicture(imgPtr, picture);
            }
            Blt_NotifyImageChanged(imgPtr);
            return TCL_OK;
        }
    }
    Tcl_AppendResult(interp, "can't find picture \"", imageName, "\"", 
        (char *)NULL);
    return TCL_ERROR;
}
    
#ifdef notdef
int
Blt_ResetPictureFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
                        Blt_Picture picture)
{
    return Blt_ResetPicture(interp, Tcl_GetString(objPtr), picture);
}
#endif

int
Blt_GetPicture(Tcl_Interp *interp, const char *string, Blt_Picture *picturePtr)
{
    Tcl_CmdInfo cmdInfo;

    if (Tcl_GetCommandInfo(interp, string, &cmdInfo)) {
        if (cmdInfo.objProc == PictureInstCmdProc) {
            PictImage *imgPtr;

            imgPtr = cmdInfo.objClientData;
            *picturePtr = imgPtr->picture;
            return TCL_OK;
        }
    }
    Tcl_AppendResult(interp, "can't find picture \"", string, "\"",
                     (char *)NULL);
    return TCL_ERROR;
}

int
Blt_GetPictureFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Blt_Picture *pictPtr)
{
    return Blt_GetPicture(interp, Tcl_GetString(objPtr), pictPtr);
}


static int
LoadPackage(Tcl_Interp *interp, const char *name)
{
    Tcl_DString ds;
    const char *result;
    char *pkg;

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, "blt_picture_", 12);
    Tcl_DStringAppend(&ds, name, -1);
    pkg = Tcl_DStringValue(&ds);
    Blt_LowerCase(pkg);
    result = Tcl_PkgRequire(interp, pkg, BLT_VERSION, PKG_EXACT);
    Tcl_DStringFree(&ds);
    return (result != NULL);
}

Blt_PictFormat *
Blt_FindPictureFormat(
    Tcl_Interp *interp,                 /* Interpreter to load new format
                                         * into. */
    const char *ext)                    /* Extension of file name read in.
                                         * Will be NULL if read from
                                         * data/base64 string. */
{
    Blt_HashEntry *hPtr;
    Blt_PictFormat *fmtPtr;

    hPtr = Blt_FindHashEntry(&fmtTable, ext);
    if (hPtr == NULL) {
        return NULL;
    }

    fmtPtr = Blt_GetHashValue(hPtr);
    if ((fmtPtr->flags & FMT_LOADED) == 0) {
        LoadPackage(interp, ext);
    }
    if (((fmtPtr->flags & FMT_LOADED) == 0) || 
        (fmtPtr->isFmtProc == NULL)) {
        if ((fmtPtr->flags & FMT_LOADED) == 0) {
            Blt_Warn("still not loaded: format %s\n", fmtPtr->name);
        } else if (fmtPtr->isFmtProc == NULL) {
            Blt_Warn("no isFmtProc: format %s\n", fmtPtr->name);
        } else {
            Blt_Warn("can't load format %s\n", fmtPtr->name);
        }
        return NULL;                    /* Could not load the format or the
                                         * format doesn't have a discovery
                                         * procedure. */
    }
    return fmtPtr;
}

static Blt_PictFormat *
QueryExternalFormat(
    Tcl_Interp *interp,                 /* Interpreter to load new format
                                         * into. */
    Blt_DBuffer dbuffer,                /* Data to be tested. */
    const char *ext)                    /* Extension of file name read in.
                                         * Will be NULL if read from
                                         * data/base64 string. */
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    /* 
     * Step 1. Try to match the format to the extension, if there is
     *         one.  We're trying to minimize the number of formats 
     *         loaded using a blind query (.i.e. -file fileName).
     */
    if (ext != NULL) {
        hPtr = Blt_FindHashEntry(&fmtTable, ext);
        if (hPtr != NULL) {
            Blt_PictFormat *fmtPtr;

            fmtPtr = Blt_GetHashValue(hPtr);
            if ((fmtPtr->flags & FMT_LOADED) == 0) {
                LoadPackage(interp, ext);
            }
            if (((fmtPtr->flags & FMT_LOADED) == 0) || 
                (fmtPtr->isFmtProc == NULL)) {
                if ((fmtPtr->flags & FMT_LOADED) == 0) {
                    Blt_Warn("still not loaded: format %s\n", fmtPtr->name);
                } else if (fmtPtr->isFmtProc == NULL) {
                    Blt_Warn("no isFmtProc: format %s\n", fmtPtr->name);
                } else {
                    Blt_Warn("can't load format %s\n", fmtPtr->name);
                }
                return NULL;            /* Could not load the format or the
                                         * format doesn't have a discovery
                                         * procedure. */
            }
            Blt_DBuffer_Rewind(dbuffer);
            if ((*fmtPtr->isFmtProc)(dbuffer)) {
                return fmtPtr;
            }
#ifdef notdef
            Blt_Warn("failed to match %s\n", fmtPtr->name);
#endif
            /* If the image doesn't match, even though the extension
             * matches, fall through and try all the other formats
             * available. */
        }
    }
    /* 
     * Step 2. Try to match the image against all the previously 
     *         loaded formats.
     */
    for (hPtr = Blt_FirstHashEntry(&fmtTable, &iter); hPtr != NULL; 
         hPtr = Blt_NextHashEntry(&iter)) {
        Blt_PictFormat *fmtPtr;

        fmtPtr = Blt_GetHashValue(hPtr);
        if ((fmtPtr->flags & FMT_LOADED) == 0) {
            continue;                   /* Format isn't already loaded. */
        }
        if (fmtPtr->isFmtProc == NULL) {
            continue;                   /* No discover procedure.  */
        }
        if ((*fmtPtr->isFmtProc)(dbuffer)) {
            return fmtPtr;
        }
    }
    /* 
     * Step 3. Try to match the image against any format not previously
     *         loaded.
     */
    for (hPtr = Blt_FirstHashEntry(&fmtTable, &iter); hPtr != NULL; 
         hPtr = Blt_NextHashEntry(&iter)) {
        Blt_PictFormat *fmtPtr;

        fmtPtr = Blt_GetHashValue(hPtr);
        if (fmtPtr->flags & FMT_LOADED) {
            continue;                   /* Format is already loaded.  */
        }
        if (!LoadPackage(interp, fmtPtr->name)) {
            continue;                   /* Can't load format. */
        }
        if (((fmtPtr->flags & FMT_LOADED) == 0) || 
            (fmtPtr->isFmtProc == NULL)) {
            if ((fmtPtr->flags & FMT_LOADED) == 0) {
                Blt_Warn("still not loaded: format %s\n", fmtPtr->name);
            } else if (fmtPtr->isFmtProc == NULL) {
                Blt_Warn("no isFmtProc: format %s\n", fmtPtr->name);
            } else {
                Blt_Warn("can't load format %s\n", fmtPtr->name);
            }
            return NULL;                /* Could not load the format or the
                                         * format doesn't have a discovery
                                         * procedure. */
        }
        if ((*fmtPtr->isFmtProc)(dbuffer)) {
            return fmtPtr;
        }
    }
    return NULL;
}

static int
ImageToPicture(Tcl_Interp *interp, PictImage *imgPtr, const char *imageName)
{
    Blt_Picture picture, clone;
    Tk_Image tkImage;
    
    tkImage = Tk_GetImage(interp, Tk_MainWindow(interp), imageName, NULL, 0);
    if (tkImage == NULL) {
        return TCL_ERROR;
    }
    picture = Blt_GetPictureFromImage(interp, tkImage);
    Tk_FreeImage(tkImage);
    if (picture == NULL) {
        return TCL_ERROR;
    }
    clone = Blt_ClonePicture(picture);
    Blt_FreePicture(picture);
    ReplacePicture(imgPtr, clone);
    if (imgPtr->sourceName != NULL) {
        Blt_Free(imgPtr->sourceName);
    }
    imgPtr->sourceName = Blt_AssertStrdup(imageName);
    imgPtr->flags &= ~IMPORTED_MASK;
    imgPtr->flags |= IMPORTED_IMAGE;
    return TCL_OK;
}

static int
WindowToPicture(Tcl_Interp *interp, PictImage *imgPtr, Tcl_Obj *objPtr)
{
    Blt_Picture picture;
    Window window;
    int w, h;

    if (Blt_GetWindowFromObj(interp, objPtr, &window) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_GetWindowExtents(imgPtr->display, window, NULL, NULL, &w, &h) 
        != TCL_OK) {
        Tcl_AppendResult(interp, "can't get dimensions of window \"", 
                Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    /* Depth, visual, colormap */
    picture = Blt_WindowToPicture(imgPtr->display, window, 0, 0, w, h, 
        imgPtr->gamma);
    if (picture == NULL) {
        Tcl_AppendResult(interp, "can't obtain snapshot of window \"", 
                Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    ReplacePicture(imgPtr, picture);
    if (imgPtr->sourceName != NULL) {
        Blt_Free(imgPtr->sourceName);
    }
    imgPtr->sourceName = Blt_AssertStrdup(Tcl_GetString(objPtr));
    imgPtr->flags &= ~IMPORTED_MASK;
    imgPtr->flags |= IMPORTED_WINDOW;
    return TCL_OK;
}


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
ColorSwitchProc(ClientData clientData, Tcl_Interp *interp,
                const char *switchName, Tcl_Obj *objPtr, char *record,
                int offset, int flags)  
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
 * JitterSwitchProc --
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
PercentSwitchProc(ClientData clientData, Tcl_Interp *interp,
                const char *switchName, Tcl_Obj *objPtr, char *record,
                int offset, int flags)  
{
    double *pctPtr = (double *)(record + offset);
    double value;

    if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((value < 0.0) || (value > 100.0)) {
        Tcl_AppendResult(interp, "invalid percentage \"", 
                Tcl_GetString(objPtr), "\" number should be between 0 and 100",
                (char *)NULL);
        return TCL_ERROR;
    }
    *pctPtr = value * 0.01;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ScaleSwitchProc --
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ScaleSwitchProc(ClientData clientData, Tcl_Interp *interp,
                const char *switchName, Tcl_Obj *objPtr, char *record,
                int offset, int flags)  
{
    int *scalePtr = (int *)(record + offset);
    const char *string;
    int length;
    char c;
    
    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'l') && (strcmp(string, "linear") == 0)) {
        *scalePtr = SCALE_LINEAR;
    } else if ((c == 'l') && (length > 2) && 
               (strncmp(string, "logarithmic", length) == 0)) {
        *scalePtr = SCALE_LOG;
    } else {
        Tcl_AppendResult(interp, "unknown scale \"", string, "\"",
                         ": should be linear or logarithmic.",
                         (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
} 

/*
 *---------------------------------------------------------------------------
 *
 * ObjToFile --
 *
 *      Given a file name, determine the image type and convert into a
 *      picture.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToFile(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    Blt_Chain chain;
    Blt_ChainLink link;
    Blt_DBuffer dbuffer;
    Blt_Picture *picturePtr = (Blt_Picture *)(widgRec + offset);
    PictImage *imgPtr = (PictImage *)widgRec;
    const char *fileName;
    Blt_PictFormat *fmtPtr;
    char *ext, buffer[32];

    fileName = Tcl_GetString(objPtr);
    if (fileName[0] == '\0') {
        FreePictures(imgPtr);
        if (imgPtr->sourceName != NULL) {
            Blt_Free(imgPtr->sourceName);
            imgPtr->sourceName = NULL;
        }
        imgPtr->fmtPtr = NULL;
        imgPtr->flags &= ~IMPORTED_MASK;
        return TCL_OK;
    }
    dbuffer = Blt_DBuffer_Create();
    if (Blt_DBuffer_LoadFile(interp, fileName, dbuffer) != TCL_OK) {
        goto error;
    }
    ext = NULL;
    if (fileName[0] != '@') {
        char *dot;

        dot = strrchr(fileName, '.');
        if ((dot != NULL) && (dot[1] != '\0')) {
            strncpy(buffer, dot + 1, 31);
            buffer[31] = '\0';
            Blt_LowerCase(buffer);
            ext = buffer;
        }
    }
    fmtPtr = QueryExternalFormat(interp, dbuffer, ext);
    if (fmtPtr == NULL) {
        Tcl_AppendResult(interp, "\nunknown image file format in \"", fileName, 
                "\"", (char *)NULL);
        goto error;
    }
    if (fmtPtr->readProc == NULL) {
        Tcl_AppendResult(interp, "no reader for format \"", fmtPtr->name, "\".",
                (char *)NULL);
        goto error;
    }
    chain = (*fmtPtr->readProc)(interp, fileName, dbuffer);
    if (chain == NULL) {
        goto error;
    }
    FreePictures(imgPtr);
    imgPtr->chain = chain;
    link = GetFirstImageIndex(imgPtr, &imgPtr->current);
    if (link != NULL) {
        imgPtr->picture = Blt_Chain_GetValue(link);
    } else {
        imgPtr->picture = NULL;
    }
    if (imgPtr->sourceName != NULL) {
        Blt_Free(imgPtr->sourceName);
    }
    imgPtr->fmtPtr = fmtPtr;
    imgPtr->sourceName = Blt_AssertStrdup(fileName);
    imgPtr->flags &= ~IMPORTED_MASK;
    imgPtr->flags |= IMPORTED_FILE;
    imgPtr->interval = 0;
    *picturePtr = imgPtr->picture;
    Blt_DBuffer_Destroy(dbuffer);
    return TCL_OK;
 error:
    Blt_DBuffer_Destroy(dbuffer);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * FileToObj --
 *
 *      Convert the picture into a TCL list of pixels.
 *
 * Results:
 *      The string representation of the picture is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
FileToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          char *widgRec, int offset, int flags)  
{
    PictImage *imgPtr = (PictImage *)widgRec;

    if (((imgPtr->flags & IMPORTED_FILE) == 0) ||
        (imgPtr->sourceName == NULL)) {
        return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(imgPtr->sourceName, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToFilter --
 *
 *      Given a string name, get the resample filter associated with it.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToFilter(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    Blt_ResampleFilter *filterPtr = (Blt_ResampleFilter *)(widgRec + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
        *filterPtr = NULL;
        return TCL_OK;
    }
    return Blt_GetResampleFilterFromObj(interp, objPtr, filterPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterToObj --
 *
 *      Convert the picture filter into a string Tcl_Obj.
 *
 * Results:
 *      The string representation of the filter is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
FilterToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            char *widgRec, int offset, int flags)  
{
    Blt_ResampleFilter filter = *(Blt_ResampleFilter *)(widgRec + offset);

    if (filter == NULL) {
        return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(Blt_NameOfResampleFilter(filter), -1);
}

static int
GetPictProc2(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    PictProc *procPtr;
    const char *opName;

    opName = Tcl_GetString(objv[2]);
    hPtr = Blt_FindHashEntry(&procTable, opName);
    if (hPtr == NULL) {
        LoadPackage(interp, opName);
    }
    hPtr = Blt_FindHashEntry(&procTable, opName);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "can't find picture procedure \"", opName,
                         "\"", (char *)NULL);
        return TCL_ERROR;
    }
    procPtr = Blt_GetHashValue(hPtr);
    if (procPtr == NULL) {
        Tcl_AppendResult(interp, "no data registered for picture procedure \"",
                         opName, "\"", (char *)NULL);
        return TCL_ERROR;
    }
    if (procPtr->proc == NULL) {
        Tcl_AppendResult(interp, "can't load picture procedure ", procPtr->name,
                         (char *)NULL);
        return TCL_ERROR;                    /* Could not load the format. */
    }
    return (*procPtr->proc)(clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToData --
 *
 *      Given a string of data or binary Tcl_Obj, determine the image
 *      type and convert into a picture.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToData(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    Blt_Picture *picturePtr = (Blt_Picture *)(widgRec + offset);
    PictImage *imgPtr = (PictImage *)widgRec;
    const unsigned char *bytes;
    int length, result;

    bytes = Tcl_GetByteArrayFromObj(objPtr, &length);
    result = TCL_ERROR;
    if (length == 0) {
        imgPtr->flags &= ~IMPORTED_MASK;
        FreePictures(imgPtr);
        result = TCL_OK;
    } else {
        Blt_DBuffer dbuffer;
        size_t numBytes;
        Blt_PictFormat *fmtPtr;
        const char *string;
        Blt_Chain chain;

        fmtPtr = NULL;                  /* Suppress compiler warning. */
        chain = NULL;                   /* Suppress compiler warning. */
        numBytes = (size_t)length;
        dbuffer = Blt_DBuffer_Create();
        string = (const char *)bytes;
        if (Blt_IsBase64(string, numBytes)) {
            if (Blt_DBuffer_Base64Decode(interp, string, numBytes, dbuffer) 
                != TCL_OK) {
                goto error;
            }
        } else {
            Blt_DBuffer_AppendData(dbuffer, bytes, numBytes);
        }
#ifdef notdef
        Blt_DBuffer_SaveFile(interp, "junk.unk", dbuffer);
#endif
        fmtPtr = QueryExternalFormat(interp, dbuffer, NULL);
        if (fmtPtr == NULL) {
            Tcl_AppendResult(interp, "unknown image file format in \"",
                             Tcl_GetString(objPtr), "\"", (char *)NULL);
            goto error;
        }
        if (fmtPtr->readProc == NULL) {
            Tcl_AppendResult(interp, "no reader for format \"", fmtPtr->name, 
                             "\".", (char *)NULL);
            goto error;
        }
        chain = (*fmtPtr->readProc)(interp, "-data", dbuffer);
        if (chain == NULL) {
            goto error;
        }
        result = TCL_OK;
    error:
        imgPtr->flags &= ~IMPORTED_MASK;
        FreePictures(imgPtr);
        if (result == TCL_OK) {
            Blt_ChainLink link;

            imgPtr->chain = chain;
            imgPtr->fmtPtr = fmtPtr;
            link = GetFirstImageIndex(imgPtr, &imgPtr->current);
            if (link != NULL) {
                imgPtr->picture = Blt_Chain_GetValue(link);
            } else {
                imgPtr->picture = NULL;
            }
            imgPtr->flags |= IMPORTED_DATA;
        }
        Blt_DBuffer_Destroy(dbuffer);
    }
    *picturePtr = imgPtr->picture;
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * DataToObj --
 *
 *      Convert the picture into a TCL list of pixels.
 *
 * Results:
 *      The string representation of the picture is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
DataToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          char *widgRec, int offset, int flags)  
{
    PictImage *imgPtr = (PictImage *)(widgRec);
    Blt_PictFormat *fmtPtr;

    if (((imgPtr->flags & IMPORTED_DATA) == 0) || (imgPtr->picture == NULL)) {
        return Tcl_NewStringObj("", -1);
    }
    fmtPtr = imgPtr->fmtPtr;
    if (fmtPtr == NULL) {
        Tcl_AppendResult(interp, "image \"", Tk_NameOfImage(imgPtr->imgToken),
                "\" has no assigned format.", (char *)NULL);
        Tcl_BackgroundError(interp);
        return Tcl_NewStringObj("", -1);
    }
    if (fmtPtr->writeProc == NULL) {
        Tcl_AppendResult(interp, "no write procedure for format \"", 
                         fmtPtr->name, "\".", (char *)NULL);
        Tcl_BackgroundError(interp);
        return Tcl_NewStringObj("", -1);
    }
    return (*fmtPtr->writeProc)(interp, imgPtr->picture);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToGamma --
 *
 *      Convert a string to a gamma value.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToGamma(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    float *gammaPtr = (float *)(widgRec + offset);
    double value;
        
    if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    if (value < 0.0) {
        Tcl_AppendResult(interp, "gamma value can't be negative", (char *)NULL);
        return TCL_ERROR;
    }
    if (value == 0.0) {
        Tcl_AppendResult(interp, "gamma value can't be zero", (char *)NULL);
        return TCL_ERROR;
    }
    if (value > 20.0) {
        value = 20.0;
    }
    *gammaPtr = value;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GammaToObj --
 *
 *      Convert the gamma value into a string representation.
 *
 * Results:
 *      The string representation of the gamma value is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
GammaToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)  
{
    float gamma = *(float *)(widgRec + offset);

    return Tcl_NewDoubleObj(gamma);
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToImage --
 *
 *      Convert a named image into a picture.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToImage(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    PictImage *imgPtr = (PictImage *)(widgRec);

    return ImageToPicture(interp, imgPtr, Tcl_GetString(objPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageToObj --
 *
 *      Convert the named image into a picture.
 *
 * Results:
 *      The string representation of the picture is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ImageToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)  
{
    PictImage *imgPtr = (PictImage *)widgRec;

    if (((imgPtr->flags & IMPORTED_IMAGE) == 0) ||
        (imgPtr->sourceName == NULL)) {
        return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(imgPtr->sourceName, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToWindow --
 *
 *      Given a file name, determine the image type and convert 
 *      into a picture.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToWindow(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    PictImage *imgPtr = (PictImage *)(widgRec);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
        if (imgPtr->sourceName != NULL) {
            Blt_Free(imgPtr->sourceName);
        }
        imgPtr->sourceName = NULL;
        imgPtr->flags &= ~IMPORTED_MASK;
        return TCL_OK;
    }
    return WindowToPicture(interp, imgPtr, objPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowToObj --
 *
 *      Convert the picture into a TCL list of pixels.
 *
 * Results:
 *      The string representation of the picture is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
WindowToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            char *widgRec, int offset, int flags)  
{
    PictImage *imgPtr = (PictImage *)widgRec;

    if (((imgPtr->flags & IMPORTED_WINDOW) == 0) ||
        (imgPtr->sourceName == NULL)) {
        return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(imgPtr->sourceName, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * DirectionSwitchProc --
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
DirectionSwitchProc(ClientData clientData, Tcl_Interp *interp,
                    const char *switchName, Tcl_Obj *objPtr, char *record,
                    int offset, int flags)      
{
    int *dirPtr = (int *)(record + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (strcmp(string, "n") == 0) {
        *dirPtr = TK_ANCHOR_N;
    } else if (strcmp(string, "s") == 0) {
        *dirPtr = TK_ANCHOR_S;
    } else if (strcmp(string, "e") == 0) {
        *dirPtr = TK_ANCHOR_E;
    } else if (strcmp(string, "w") == 0) {
        *dirPtr = TK_ANCHOR_W;
    } else {
        Tcl_AppendResult(interp, "unknown direction \"", string,
                "\": should be n, s, e, or w.", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
} 

/*
 *---------------------------------------------------------------------------
 *
 * CacheKey --
 *
 *      Returns a key representing a specific visual context for a
 *      PictImage.  Keys are used to create/find cache entries stored in
 *      the hash table of each PictImage.
 *
 * Results:
 *      A pointer to a static cache key.  
 *
 * Side effects:
 *      The key is overwritten on each call.  Care must be taken by the caller
 *      to save the key before making additional calls.
 *
 *---------------------------------------------------------------------------
 */
static PictCacheKey *
CacheKey(Tk_Window tkwin, unsigned int index)
{
    static PictCacheKey key;

    key.display = Tk_Display(tkwin);
    key.visualPtr = Tk_Visual(tkwin);
    key.colormap = Tk_Colormap(tkwin);
    key.depth = Tk_Depth(tkwin);
    key.index = index;
    return &key;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyCache --
 *
 *      This procedure is a callback for Tcl_EventuallyFree to release the
 *      resources and memory used by a PictInstance entry. The entry is
 *      destroyed only if noone else is currently using the entry (using
 *      reference counting).
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The cache entry is possibly deallocated.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyCache(DestroyData data)
{
    PictInstance *cachePtr = (PictInstance *)data;
    
    if (cachePtr->refCount <= 0) {
        if (cachePtr->painter != NULL) {
            Blt_FreePainter(cachePtr->painter);
        }
        if (cachePtr->hashPtr != NULL) {
            Blt_DeleteHashEntry(cachePtr->tablePtr, cachePtr->hashPtr);
        }
        Blt_Free(cachePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetInstanceProc --
 *
 *      This procedure is called for each use of a picture image in a
 *      widget.
 *
 * Results:
 *      The return value is an entry for the visual context, which will be
 *      passed back to us on calls to DisplayProc and FreeInstanceProc.
 *
 * Side effects:
 *      A new entry is possibly allocated (or shared if one already exists).
 *
 *---------------------------------------------------------------------------
 */
static ClientData
GetInstanceProc(
    Tk_Window tkwin,                    /* Window in which the picture will
                                         * be displayed. */
    ClientData clientData)              /* Pointer to picture image for the
                                         * image. */
{
    Blt_HashEntry *hPtr;
    PictCacheKey *keyPtr;
    PictImage *imgPtr = clientData;
    PictInstance *cachePtr;
    int isNew;

    keyPtr = CacheKey(tkwin, imgPtr->current);
    hPtr = Blt_CreateHashEntry(&imgPtr->cacheTable, (char *)keyPtr, &isNew);
    if (isNew) {
        cachePtr = Blt_Malloc(sizeof(PictInstance));    
        if (cachePtr == NULL) {
            return NULL;                /* Can't allocate memory. */
        }
        cachePtr->painter = Blt_GetPainter(tkwin, imgPtr->gamma);
        cachePtr->image = imgPtr;
        cachePtr->display = Tk_Display(tkwin);
        cachePtr->hashPtr = hPtr;
        cachePtr->tablePtr = &imgPtr->cacheTable;
        cachePtr->refCount = 0;
        cachePtr->flags = 0;
        Blt_SetHashValue(hPtr, cachePtr);

        if (imgPtr->picture != NULL) {
            Blt_NotifyImageChanged(imgPtr);
        }
    } 
    cachePtr = Blt_GetHashValue(hPtr);
    cachePtr->refCount++;
    return cachePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeInstanceProc --
 *
 *      This procedure is called when a widget ceases to use a particular
 *      instance of a picture image.  We don't actually get rid of the
 *      entry until later because we may be about to re-get this instance
 *      again.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Internal data structures get freed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FreeInstanceProc(
    ClientData clientData,              /* Pointer to cache structure */
    Display *display)                   /* Not used. */
{
    PictInstance *cachePtr = clientData;

    cachePtr->refCount--;
    if (cachePtr->refCount <= 0) {
        /* 
         * Right now no one is using the entry. But delay the removal of
         * the cache entry in case it's reused shortly afterwards.
         */
        Tcl_EventuallyFree(cachePtr, DestroyCache);
    }    
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteProc --
 *
 *      This procedure is called by the Tk image code to delete the master
 *      structure (PictureImage) for a picture image.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Resources associated with the picture image are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
DeleteProc(ClientData clientData)      /* Pointer to picture image master
                                        * structure for image.  Must not
                                        * have any more instances. */
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    PictImage *imgPtr = clientData;
 
    if (imgPtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(imgPtr->timerToken);
        imgPtr->timerToken = 0;
    }
    if (imgPtr->chain != NULL) {
        FreePictures(imgPtr);
    }
    for (hPtr = Blt_FirstHashEntry(&imgPtr->cacheTable, &iter); 
        hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        PictInstance *cachePtr;

        cachePtr = Blt_GetHashValue(hPtr);
        cachePtr->hashPtr = NULL;       /* Flag for FreeInstanceProc. */
        DestroyCache((DestroyData)cachePtr);
    }
    imgPtr->imgToken = NULL;
    if (imgPtr->cmdToken != NULL) {
        Tcl_DeleteCommandFromToken(imgPtr->interp, imgPtr->cmdToken);
    }
    if (imgPtr->sourceName != NULL) {
        Blt_Free(imgPtr->sourceName);
    }
    Blt_DeleteHashTable(&imgPtr->cacheTable);
    Blt_FreeOptions(configSpecs, (char *)imgPtr, imgPtr->display, 0);
    Blt_Free(imgPtr);
}

static int 
ConfigureImage(Tcl_Interp *interp, PictImage *imgPtr, int objc, 
               Tcl_Obj *const *objv, int flags) 
{

    if (Blt_ConfigureWidgetFromObj(interp, Tk_MainWindow(interp), configSpecs, 
        objc, objv, (char *)imgPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    imgPtr->picture = CurrentPictureFromPictImage(imgPtr);
    if (imgPtr->picture == NULL) {
        int w, h;

        w = (imgPtr->reqWidth == 0) ? 16 : imgPtr->reqWidth;
        h = (imgPtr->reqHeight == 0) ? 16 : imgPtr->reqHeight;
        ReplacePicture(imgPtr, Blt_CreatePicture(w, h));
    }
    if (Blt_ConfigModified(configSpecs, "-rotate", (char *)NULL)) {
        if (imgPtr->angle != 0.0) {
            Blt_Picture rotate;

            /* Rotate the picture */
            rotate = Blt_RotatePicture(imgPtr->picture, imgPtr->angle);
            ReplacePicture(imgPtr, rotate);
        }
    }
    if (Blt_ConfigModified(configSpecs, "-width", "-height", (char *)NULL)) {
        int w, h;

        w = (imgPtr->reqWidth == 0) ? 
            Blt_Picture_Width(imgPtr->picture) : imgPtr->reqWidth;
        h = (imgPtr->reqHeight == 0) ? 
            Blt_Picture_Height(imgPtr->picture) : imgPtr->reqHeight;
        
        if (imgPtr->flags & MAXPECT) {
            double sx, sy, scale;
            
            sx = (double)w / (double)Blt_Picture_Width(imgPtr->picture);
            sy = (double)h / (double)Blt_Picture_Height(imgPtr->picture);
            scale = MIN(sx, sy);
            w = (int)(Blt_Picture_Width(imgPtr->picture) * scale + 0.5);
            h = (int)(Blt_Picture_Height(imgPtr->picture) * scale + 0.5);
        }           
        if ((Blt_Picture_Width(imgPtr->picture) != w) || 
            (Blt_Picture_Height(imgPtr->picture) != h)) {
            Blt_Picture resize;

            /* Scale the picture */
            if (imgPtr->filter == NULL) {
                resize = Blt_ScalePicture(imgPtr->picture, 0, 0,
                        Blt_Picture_Width(imgPtr->picture), 
                        Blt_Picture_Height(imgPtr->picture), w, h);
            } else {
                resize = Blt_CreatePicture(w, h);
                Blt_ResamplePicture(resize, imgPtr->picture, imgPtr->filter, 
                        imgPtr->filter);
            }   
            ReplacePicture(imgPtr, resize);
        }
    }
    if (Blt_ConfigModified(configSpecs, "-sharpen", (char *)NULL)) {
        if (imgPtr->flags & SHARPEN) {
            Blt_SharpenPicture(imgPtr->picture, imgPtr->picture);
        }
    }
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * CreateProc --
 *
 *      This procedure is called by the Tk image code to create a new picture
 *      image.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      The data structure for a new picture image is allocated and
 *      initialized.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateProc(
    Tcl_Interp *interp,                 /* Interpreter to report errors back
                                         * to. */
    const char *name,                   /* Name to use for image command. */
    int objc,                           /* # of option arguments. */
    Tcl_Obj *const *objv,               /* Option arguments (doesn't include
                                         * image name or type). */
    const Tk_ImageType *typePtr,        /* Not used. */
    Tk_ImageMaster imgToken,            /* Token for image, to be used by us
                                         * in later callbacks. */
    ClientData *clientDataPtr)          /* Store manager's token for image
                                         * here; it will be returned in later
                                         * callbacks. */
{
    PictImage *imgPtr;
    Tk_Window tkwin;

    /*
     * Allocate and initialize the picture image master record.
     */

    imgPtr = Blt_AssertCalloc(1, sizeof(PictImage));
    imgPtr->imgToken = imgToken;
    imgPtr->interp = interp;
    imgPtr->gamma = 1.0f;
    imgPtr->cmdToken = Tcl_CreateObjCommand(interp, name, PictureInstCmdProc,
            imgPtr, PictureInstCmdDeletedProc);
    /* imgPtr->dither = 2; */
    tkwin = Tk_MainWindow(interp);
    imgPtr->display = Tk_Display(tkwin);
    imgPtr->colormap = Tk_Colormap(tkwin);
    Blt_InitHashTable(&imgPtr->cacheTable, sizeof(PictCacheKey)/sizeof(int));

    /*
     * Process configuration options given in the image create command.
     */
    if (ConfigureImage(interp, imgPtr, objc, objv, 0) != TCL_OK) {
        DeleteProc(imgPtr);
        return TCL_ERROR;
    }
    *clientDataPtr = imgPtr;
    Tcl_SetStringObj(Tcl_GetObjResult(interp), name, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayProc --
 *
 *      This procedure is invoked to draw a picture image.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      A portion of the image gets rendered in a pixmap or window.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayProc(
    ClientData clientData,              /* Pointer to token structure for the
                                         * picture to be displayed. */
    Display *display,                   /* Display on which to draw
                                         * picture. */
    Drawable drawable,                  /* Pixmap or window in which to draw
                                         * image. */
    int x, int y,                       /* Upper-left corner of area within
                                         * image to draw. */
    int w, int h,                       /* Dimension of area within image to
                                         * draw. */
    int dx, int dy)                     /* Coordinates within destination
                                         * drawable that correspond to imageX
                                         * and imageY. */
{
    PictInstance *instPtr = clientData;
    PictImage *imgPtr;
    Blt_Picture picture;
    unsigned int flags;

    imgPtr = instPtr->image;
    
    picture = CurrentPictureFromPictImage(imgPtr);
    if (picture == NULL) {
        return;
    }
#ifdef notdef
    fprintf(stderr, "DisplayProc drawable=%x, picture=%x, x=%d,y=%d,w=%d,h=%d,dx=%d,dy=%d\n",
            drawable, picture, x, y, w, h, dx, dy);
#endif
    flags = 0;
    if ((imgPtr->flags & DITHER) || 
        (Blt_PainterDepth(instPtr->painter) < 15)) {
        flags |= BLT_PAINTER_DITHER;
    }
    Blt_PaintPicture(instPtr->painter, drawable, picture, x, y, w, h, dx, dy,
        flags);
}

/*
 *---------------------------------------------------------------------------
 *
 * PostScriptProc --
 *
 *      This procedure is called to output the contents of a picture image
 *      in PostScript by calling the Tk_PostscriptPhoto function.
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
PostScriptProc(
    ClientData clientData,              /* Master picture image. */
    Tcl_Interp *interp,                 /* Interpreter to return generated
                                         * PostScript. */
    Tk_Window tkwin,
    Tk_PostscriptInfo psInfo,           /* Not used.  Only useful for Tk
                                         * internal Photo and Bitmap image
                                         * types.  */
    int x, int y,                       /* First pixel to output */
    int w, int h,                       /* Width and height of picture area */
    int prepass)
{
    PictImage *imgPtr = clientData;

    if (prepass) {
        return TCL_OK;
    }
    if (imgPtr->picture == NULL) {
        return TCL_OK;
    }
    if (Blt_Picture_IsOpaque(imgPtr->picture)) {
        Blt_Ps ps;
        PageSetup setup;

        memset(&setup, 0, sizeof(setup));
        ps = Blt_Ps_Create(interp, &setup);
        Blt_Ps_DrawPicture(ps, imgPtr->picture, (double)x, (double)y);
        Blt_Ps_SetInterp(ps, interp);
        Blt_Ps_Free(ps);
    } else {
        Blt_Picture bg;
        Blt_Ps ps;
        Drawable drawable;
        PageSetup setup;

        drawable = Tk_WindowId(tkwin);
        bg = Blt_DrawableToPicture(tkwin, drawable, x, y, w, h, imgPtr->gamma); 
        if (bg == NULL) {
            return TCL_ERROR;
        }
        Blt_CompositeArea(bg, imgPtr->picture, 0, 0, w, h, 0, 0);

        memset(&setup, 0, sizeof(setup));
        ps = Blt_Ps_Create(interp, &setup);
        Blt_Ps_DrawPicture(ps, bg, (double)x, (double)y);
        Blt_Ps_SetInterp(ps, interp);
        Blt_Ps_Free(ps);
        Blt_FreePicture(bg);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AreaSwitch --
 *
 *      Convert a Tcl_Obj list of 2 or 4 numbers into representing a
 *      bounding box structure.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AreaSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    PictArea *areaPtr = (PictArea *)(record + offset);

    if (GetAreaFromObj(interp, objPtr, areaPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterSwitch --
 *
 *      Convert a Tcl_Obj representing a 1D image filter.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FilterSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    Blt_ResampleFilter *filterPtr = (Blt_ResampleFilter *)(record + offset);

    return Blt_GetResampleFilterFromObj(interp, objPtr, filterPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * PixelsSwitch --
 *
 *      Convert a Tcl_Obj representing a screen distance to the number of
 *      pixels.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PixelsSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    int *pixelsPtr = (int *)(record + offset);

    return Blt_GetPixelsFromObj(interp, Tk_MainWindow(interp), objPtr, 
        PIXELS_NNEG, pixelsPtr);
}

typedef struct {
    const char *name;
    Blt_BlendingMode mode;
} BlendSpec;

static BlendSpec blendSpecs[] = {
    { "colorburn",      BLT_BLEND_COLORBURN }, 
    { "colordodge",     BLT_BLEND_COLORDODGE },
    { "darken",         BLT_BLEND_DARKEN },
    { "difference",     BLT_BLEND_DIFFERENCE }, 
    { "divide",         BLT_BLEND_DIVIDE },
    { "exclusion",      BLT_BLEND_EXCLUSION },
    { "hardlight",      BLT_BLEND_HARDLIGHT },
    { "hardmix",        BLT_BLEND_HARDMIX },
    { "lighten",        BLT_BLEND_LIGHTEN },                    
    { "linearburn",     BLT_BLEND_LINEARBURN },
    { "lineardodge",    BLT_BLEND_LINEARDODGE },
    { "linearlight",    BLT_BLEND_LINEARLIGHT },
    { "multiply",       BLT_BLEND_MULTIPLY },
    { "normal",         BLT_BLEND_NORMAL },
    { "overlay",        BLT_BLEND_OVERLAY },
    { "pinlight",       BLT_BLEND_PINLIGHT },
    { "screen",         BLT_BLEND_SCREEN },
    { "softlight",      BLT_BLEND_SOFTLIGHT },
    { "softlight2",     BLT_BLEND_SOFTLIGHT2 },
    { "subtract",       BLT_BLEND_SUBTRACT },
    { "vividlight",     BLT_BLEND_VIVIDLIGHT }
};
static int numBlendModes = sizeof(blendSpecs) / sizeof(BlendSpec);

static int
GetBlendModeFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
                    Blt_BlendingMode *modePtr)
{
    const char *string;
    int low, high;
    char c;
    
    low = 0;
    high = numBlendModes - 1;
    string = Tcl_GetString(objPtr);
    c = string[0];
    while (low <= high) {
        BlendSpec *specPtr;
        int compare;
        int median;
        
        median = (low + high) >> 1;
        specPtr = blendSpecs + median;

        /* Test the first character */
        compare = c - specPtr->name[0];
        if (compare == 0) {
            /* Now test the entire string */
            compare = strcmp(string, specPtr->name);
        }
        if (compare < 0) {
            high = median - 1;
        } else if (compare > 0) {
            low = median + 1;
        } else {
            *modePtr = specPtr->mode;           /* Op found. */
            return TCL_OK;
        }
    }
    Tcl_AppendResult(interp, "can't find blend mode \"", string, "\"",
                     (char *)NULL);
    return TCL_ERROR;                   /* Can't find blend mode */
}

/*
 *---------------------------------------------------------------------------
 *
 * BlendingModeSwitch --
 *
 *      Convert a Tcl_Obj representing a blending mode.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BlendingModeSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    Blt_BlendingMode *modePtr = (Blt_BlendingMode *)(record + offset);
    
    if (GetBlendModeFromObj(interp, objPtr, modePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}


static Blt_Picture
NextImage(PictImage *imgPtr)
{
    Blt_ChainLink link;
    int index;
    
    link = GetNextImageIndex(imgPtr, imgPtr->current + 1, &index);
    if (link != NULL) {
        imgPtr->current = index;
        imgPtr->picture = Blt_Chain_GetValue(link);
        Blt_NotifyImageChanged(imgPtr);
        return imgPtr->picture;
    }
    return NULL;
}

static Blt_Picture
PreviousImage(PictImage *imgPtr)
{
    Blt_ChainLink link;
    int index;
    
    link = GetPreviousImageIndex(imgPtr, imgPtr->current - 1, &index);
    if (link != NULL) {
        imgPtr->current = index;
        imgPtr->picture = Blt_Chain_GetValue(link);
        Blt_NotifyImageChanged(imgPtr);
        return imgPtr->picture;
    }
    return NULL;
}

static Blt_Picture
FirstImage(PictImage *imgPtr)
{
    Blt_ChainLink link;
    int index;
    
    link = GetFirstImageIndex(imgPtr, &index);
    if (link != NULL) {
        imgPtr->current = index;
        imgPtr->picture = Blt_Chain_GetValue(link);
        Blt_NotifyImageChanged(imgPtr);
        return imgPtr->picture;
    }
    return NULL;
}

static void
SequenceTimerProc(ClientData clientData)
{
    PictImage *imgPtr = clientData;
    int delay;

    NextImage(imgPtr);
    delay = 100;                        /* 100 milliseconds. */
    if (imgPtr->picture != NULL) {
        delay = Blt_Picture_Delay(imgPtr->picture);
    }
    if (imgPtr->interval > 0) {
        delay = imgPtr->interval;
    }
    imgPtr->timerToken = Tcl_CreateTimerHandler(delay, SequenceTimerProc,
        imgPtr);
}

static void
FreeTransition(PictImage *imgPtr)
{
    Transition *transPtr;

    transPtr = imgPtr->clientData;
    assert(transPtr != NULL);
    if (transPtr->timerToken != (Tcl_TimerToken)0) {
        Tcl_DeleteTimerHandler(transPtr->timerToken);
        transPtr->timerToken = 0;
    }
    if (transPtr->varNameObjPtr != NULL) {
        Tcl_Obj *objPtr;
        
        objPtr = Tcl_NewIntObj(TRUE);
        Tcl_ObjSetVar2(transPtr->interp, transPtr->varNameObjPtr, NULL,
                objPtr, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    }
    Blt_FreeSwitches(transPtr->specs, transPtr, 0);
    Blt_Free(transPtr);
    imgPtr->clientData = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * CrossFadeTimerProc --
 *
 *---------------------------------------------------------------------------
 */
static void
CrossFadeTimerProc(ClientData clientData)
{
    Transition *transPtr = clientData;
    PictImage *imgPtr;

    imgPtr = transPtr->imgPtr;
    transPtr->count++;
    if (transPtr->count <= transPtr->numSteps) {
        double opacity;

        opacity = transPtr->count / (double)transPtr->numSteps;
        if (transPtr->logScale) {
            opacity = log10(9.0 * opacity + 1.0);
        }
        if (transPtr->from == NULL) {
            Blt_FadeFromColor(transPtr->picture, transPtr->to,
                              &transPtr->fromColor, opacity);
        } else if (transPtr->to == NULL) {
            Blt_FadeToColor(transPtr->picture, transPtr->from,
                              &transPtr->toColor, opacity);
        } else {
            Blt_CrossFadePictures(transPtr->picture, transPtr->from,
                                  transPtr->to, opacity);
        }
        Blt_NotifyImageChanged(transPtr->imgPtr);
        transPtr->timerToken = Tcl_CreateTimerHandler(transPtr->interval, 
                CrossFadeTimerProc, transPtr);
    } else {
        FreeTransition(imgPtr);
    }
}

static void
DoDissolve(Transition *transPtr)
{
    long last, next;
    double position;
    
    position = transPtr->count / (double)transPtr->numSteps;
    if (transPtr->logScale) {
        position = log10(9.0 * position + 1.0);
    }
    next = (int)(position * transPtr->numPixels);
    last = transPtr->last;
    if (next < last) {
        Blt_CopyPictureBits(transPtr->picture, transPtr->from);
        next = Blt_Dissolve2(transPtr->picture, transPtr->to, 0, next);
    } else {
        next = Blt_Dissolve2(transPtr->picture, transPtr->to, last, next);
    }        
    transPtr->last = next;
}
    
/*
 *---------------------------------------------------------------------------
 *
 * DissolveTimerProc --
 *
 *---------------------------------------------------------------------------
 */
static void
DissolveTimerProc(ClientData clientData)
{
    Transition *transPtr = clientData;
    PictImage *imgPtr;

    imgPtr = transPtr->imgPtr;
    transPtr->count++;
    if (transPtr->count <= transPtr->numSteps) {
        DoDissolve(transPtr);
        Blt_NotifyImageChanged(imgPtr);
        transPtr->timerToken = Tcl_CreateTimerHandler(transPtr->interval, 
                DissolveTimerProc, transPtr);
    } else {
        FreeTransition(imgPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * WipeTimerProc --
 *
 *---------------------------------------------------------------------------
 */
static void
WipeTimerProc(ClientData clientData)
{
    Transition *transPtr = clientData;
    PictImage *imgPtr;

    imgPtr = transPtr->imgPtr;
    transPtr->count++;
    if (transPtr->count <= transPtr->numSteps) {
        double position;

        position = transPtr->count / (double)transPtr->numSteps;
        if (transPtr->logScale) {
            position = log10(9.0 * position + 1.0);
        }
        Blt_WipePictures(transPtr->picture, transPtr->from, transPtr->to,
                        transPtr->direction, position);
        Blt_NotifyImageChanged(imgPtr);
        transPtr->timerToken = Tcl_CreateTimerHandler(transPtr->interval, 
                WipeTimerProc, transPtr);
    } else {
        FreeTransition(imgPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ArithOp --
 *
 *      imageName arith op $src -from { 0 0 100 100 } -to { 0 0 }
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ArithOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Blt_Picture src, mask;
    Blt_PictureArithOps op;
    PictImage *imgPtr = clientData;
    Blt_Pixel scalar;
    const char *string;
    char c;
    int len;
    ArithSwitches switches;

    src = NULL;
    string = Tcl_GetString(objv[2]);
    if ((string[0] == '0') && (string[1] == 'x')) {
        if (Blt_GetPixel(interp, string, &scalar) != TCL_OK) {
            return TCL_ERROR;
        }
    } else if (Blt_GetPicture(interp, string, &src) != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetStringFromObj(objv[1], &len);
    op = 0;
    c = string[0];
    if ((c == 'a') && (len > 1) && (strncmp(string, "add", len) == 0)) {
        op = PIC_ARITH_ADD;
    } else if ((c == 's') && (strncmp(string, "subtract", len) == 0)) {
        op = PIC_ARITH_SUB;
    } else if ((c == 'a') && (len > 1) && (strncmp(string, "and", len) == 0)) {
        op = PIC_ARITH_AND;
    } else if ((c == 'o') && (strncmp(string, "or", len) == 0)) {
        op = PIC_ARITH_OR;
    } else if ((c == 'n') && (len > 1) && (strncmp(string, "nand", len) == 0)) {
        op = PIC_ARITH_NAND;
    } else if ((c == 'n') && (len > 1) && (strncmp(string, "nor", len) == 0)) {
        op = PIC_ARITH_NOR;
    } else if ((c == 'x') && (strncmp(string, "xor", len) == 0)) {
        op = PIC_ARITH_XOR;
    } else if ((c == 'm') && (len > 1) && (strncmp(string, "max", len) == 0)) {
        op = PIC_ARITH_MAX;
    } else if ((c == 'm') && (len > 1) && (strncmp(string, "min", len) == 0)) {
        op = PIC_ARITH_MIN;
    }   

    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, arithSwitches, objc - 3, objv + 3, &switches, 
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    mask = NULL;
    if (switches.maskObjPtr != NULL) {
        if (Blt_GetPictureFromObj(interp, switches.maskObjPtr, &mask)!=TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (mask == NULL) {
        if (src == NULL) {
            Blt_ApplyScalarToPicture(imgPtr->picture, &scalar, op);
        } else {
            Blt_ApplyPictureToPicture(imgPtr->picture, src, 
                                      0, 0, 
                                      Blt_Picture_Width(src),
                                      Blt_Picture_Height(src),
                                      0, 0, 
                                      op);
        }       
    } else {
        if (src == NULL) {
            Blt_ApplyScalarToPictureWithMask(imgPtr->picture, &scalar, mask, 
                switches.invert, op);
        } else {
            Blt_ApplyPictureToPictureWithMask(imgPtr->picture, src, mask, 
                                              0, 0, 
                                              Blt_Picture_Width(src),
                                              Blt_Picture_Height(src),
                                              0, 0, 
                                              switches.invert, op);
        }       
    }
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BlankOp --
 *      
 *      Resets the picture at its current size to a known background.  
 *      This is different from the rest of the drawing commands in that
 *      we are drawing a background.  There is previous image to composite.
 *
 * Results:
 *      Returns a standard TCL return value. If an error occured parsing
 *      the pixel.
 *
 *
 * Side effects:
 *      A Tk_ImageChanged notification is triggered.
 *
 *      imageName blank ?colorName?
 *---------------------------------------------------------------------------
 */
static int
BlankOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Pict *destPtr;
    PictImage *imgPtr = clientData;
    Blt_Pixel color;

    if (objc == 3) {
        if (Blt_GetPixelFromObj(interp, objv[2], &color) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        color.u32 = 0xFFFFFFFF;
    }
    destPtr = imgPtr->picture;
    Blt_BlankPicture(destPtr, color.u32);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CompositeOp --
 *
 *      imageName composite bgName fgName ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
CompositeOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    CompositeSwitches switches;
    Blt_Picture fg, bg, tmp;
    PictImage *imgPtr = clientData;
    Blt_Picture dst;

    if ((Blt_GetPictureFromObj(interp, objv[2], &bg) != TCL_OK) ||
        (Blt_GetPictureFromObj(interp, objv[3], &fg) != TCL_OK)) {
        return TCL_ERROR;
    }
    switches.from.x1 = switches.from.y1 = 0;
    switches.from.x2 = Blt_Picture_Width(bg);
    switches.from.y2 = Blt_Picture_Height(bg);
    switches.to.x1 = switches.to.y1 = 0;
    switches.to.x2 = Blt_Picture_Width(bg);
    switches.to.y2 = Blt_Picture_Height(bg);
    if (Blt_ParseSwitches(interp, compositeSwitches, objc - 4, objv + 4, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    dst = CurrentPictureFromPictImage(imgPtr);
    tmp = NULL;
    if (dst == fg) {
        fg = tmp = Blt_ClonePicture(fg);
    }
    if (dst != bg) {
        ResizeCopyPictureBits(dst, bg); /* Make a copy of the
                                         * background. */
    }
    if ((switches.from.x1 == 0) && (switches.from.y1 == 0) &&
        (switches.to.x1 == 0) && (switches.to.y1 == 0) &&
        (switches.from.x2 == Blt_Picture_Width(bg)) &&
        (switches.from.y2 == Blt_Picture_Height(bg))) {
        Blt_CompositePictures(dst, fg);
    } else {
        if (!Blt_AdjustAreaToPicture(fg, &switches.from)) {
            Tcl_AppendResult(interp,
                        "source bounding box lies outside of picture", 
                        (char *)NULL);
            goto error;
        }
        if (!Blt_AdjustAreaToPicture(dst, &switches.to)) {
            Tcl_AppendResult(interp,
                        "destination bounding box lies outside of picture", 
                        (char *)NULL);
            goto error;
        }
        Blt_CompositeArea(dst, fg, switches.from.x1, switches.from.y1, 
                          AREA_WIDTH(switches.from), AREA_HEIGHT(switches.from),
                          switches.to.x1, switches.to.y1);
    }
    if (tmp != NULL) {
        Blt_FreePicture(tmp);
    }
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
 error:
    if (tmp != NULL) {
        Blt_FreePicture(tmp);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorBlendOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ColorBlendOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Blt_Picture fg, bg, dst;
    ColorBlendSwitches switches;
    PictImage *imgPtr = clientData;

    if ((Blt_GetPictureFromObj(interp, objv[2], &bg) != TCL_OK) ||
        (Blt_GetPictureFromObj(interp, objv[3], &fg) != TCL_OK)) {
        return TCL_ERROR;
    }
    switches.mode = BLT_BLEND_NORMAL;
    if (Blt_ParseSwitches(interp, colorBlendSwitches, objc - 4, objv + 4, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    dst = CurrentPictureFromPictImage(imgPtr);
    ResizeCopyPictureBits(dst, bg);
    Blt_ColorBlendPictures(dst, fg, switches.mode); 
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BlurOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
BlurOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_Picture src, dst;
    PictImage *imgPtr = clientData;
    int r;                              /* Radius of the blur. */

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &r) != TCL_OK) {
        return TCL_ERROR;
    }
    if (r < 0) {
        Tcl_AppendResult(interp, "blur radius can't be negative", (char *)NULL);
        return TCL_ERROR;
    }
    if (r < 2) {
        Tcl_AppendResult(interp, "radius of blur must be > 1 pixel wide",
                         (char *)NULL);
        return TCL_ERROR;
    }
    dst = CurrentPictureFromPictImage(imgPtr);
    Blt_BlurPicture(dst, src, r, 3);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *      Returns the value of the configuration option specified.
 *
 * Results:
 *      Returns a standard TCL return value.  If TCL_OK, the value of the
 *      picture configuration option specified is returned in the
 *      interpreter result.  Otherwise an error message is returned.
 * 
 *---------------------------------------------------------------------------
 */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    
    return Blt_ConfigureValueFromObj(interp, Tk_MainWindow(interp), configSpecs,
        (char *)imgPtr, objv[2], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    PictImage *imgPtr;
    Tk_Window tkwin;
    unsigned int flags;
    
    flags = BLT_CONFIG_OBJV_ONLY;
    tkwin = Tk_MainWindow(interp);
    imgPtr = clientData;
    if (objc == 2) {
        return Blt_ConfigureInfoFromObj(interp, tkwin, configSpecs,
            (char *)imgPtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 3) {
        return Blt_ConfigureInfoFromObj(interp, tkwin, configSpecs,
            (char *)imgPtr, objv[2], flags);
    } else {
        if (ConfigureImage(interp, imgPtr, objc - 2, objv + 2, flags) 
            != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ConvolveOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ConvolveOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_Picture src;
    ConvolveSwitches switches;
    PictImage *imgPtr = clientData;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    switches.vFilter = bltBoxFilter;
    switches.hFilter = bltBoxFilter;
    switches.filter = NULL;
    if (Blt_ParseSwitches(interp, convolveSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (switches.filter != NULL) {
        switches.hFilter = switches.vFilter = switches.filter;
    }
#ifdef notdef
    dst = CurrentPictureFromPictImage(imgPtr);
    Blt_ConvolvePicture(dst, src, switches.vFilter, switches.hFilter);
#endif
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CopyOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
CopyOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_Picture src, dst;
    CopySwitches switches;
    PictImage *imgPtr = clientData;

    dst = CurrentPictureFromPictImage(imgPtr);
    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    InitArea(src, &switches.from);
    InitArea(dst, &switches.to);
    switches.composite = FALSE;
    if (Blt_ParseSwitches(interp, copySwitches, objc - 3, objv + 3, &switches, 
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (!Blt_AdjustAreaToPicture(src, &switches.from)) {
        return TCL_OK;                  /* Area is not inside of source. */
    }
    if (!Blt_AdjustAreaToPicture(dst, &switches.to)) {
        return TCL_OK;                  /* Area is not inside of
                                         * destination. */
    }
    if (switches.composite) {
        Blt_CompositeArea(dst, src, switches.from.x1, switches.from.y1,
            AREA_WIDTH(switches.from), AREA_HEIGHT(switches.from),
            switches.to.x1, switches.to.y1);
    } else if ((switches.from.flags & (BLT_PICTURE_SIZE|BLT_PICTURE_COORDS)) ||
               (switches.to.flags & (BLT_PICTURE_SIZE|BLT_PICTURE_COORDS))) {
        if ((AREA_WIDTH(switches.from) != AREA_WIDTH(switches.to)) ||
            (AREA_HEIGHT(switches.from) != AREA_HEIGHT(switches.to))) {
            Blt_Picture sub1, sub2;     /* Sub-picture regions. */
            Blt_ResampleFilter vFilter, hFilter;
            
            /* Resample area */
            sub1 = Blt_CreatePicture(AREA_WIDTH(switches.from),
                                     AREA_HEIGHT(switches.from));
            Blt_CopyArea(sub1, src, switches.from.x1, switches.from.y1,
                         AREA_WIDTH(switches.from), AREA_HEIGHT(switches.from),
                         0, 0);
            sub2 = Blt_CreatePicture(AREA_WIDTH(switches.to),
                                     AREA_HEIGHT(switches.to));
            Blt_CopyArea(sub2, dst, switches.to.x1, switches.to.y1,
                         AREA_WIDTH(switches.to), AREA_HEIGHT(switches.to),
                         0, 0);
            hFilter = (AREA_WIDTH(switches.from) < AREA_WIDTH(switches.to)) ?
                bltMitchellFilter : bltBoxFilter; 
            vFilter = (AREA_HEIGHT(switches.from) < AREA_HEIGHT(switches.to)) ?
                bltMitchellFilter : bltBoxFilter;
            Blt_ResamplePicture(sub2, sub1, vFilter, hFilter);
            Blt_FreePicture(sub1);
            Blt_CopyArea(dst, sub2, 0, 0,
                         AREA_WIDTH(switches.to), AREA_HEIGHT(switches.to),
                         switches.to.x1, switches.to.y1);
            Blt_FreePicture(sub2);
        } else {
            Blt_CopyArea(dst, src, switches.from.x1, switches.from.y1,
                         AREA_WIDTH(switches.from), AREA_HEIGHT(switches.from),
                         switches.to.x1, switches.to.y1);
        }
    } else {
        if ((AREA_WIDTH(switches.from) == AREA_WIDTH(switches.to)) &&
            (AREA_HEIGHT(switches.from) == AREA_HEIGHT(switches.to))) {
            Blt_CopyPictureBits(dst, src);
        } else {
            Blt_CopyArea(dst, src, switches.from.x1, switches.from.y1,
                         AREA_WIDTH(switches.from), AREA_HEIGHT(switches.from),
                         switches.to.x1, switches.to.y1);
        }            
    }
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CropOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *      pictureName crop x1 y1 ?x2 y2?
 *---------------------------------------------------------------------------
 */
static int
CropOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_Picture src, dst;
    PictImage *imgPtr = clientData;
    PictArea area;

    src = CurrentPictureFromPictImage(imgPtr);
    InitArea(src, &area);
    if (Blt_GetAreaFromObjv(interp, objc - 2, objv + 2, &area) != TCL_OK) {
        return TCL_ERROR;
    }
    if (!Blt_AdjustAreaToPicture(src, &area)) {
        Tcl_AppendResult(interp, "impossible coordinates for area", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    dst = Blt_CreatePicture(AREA_WIDTH(area), AREA_HEIGHT(area));
    Blt_CopyArea(dst, src, area.x1, area.y1, AREA_WIDTH(area),
                 AREA_HEIGHT(area), 0, 0);
    ReplacePicture(imgPtr, dst);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * CrossFadeOp --
 *
 *      pictureName crossfade fromPicture toPicture ?switches...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
CrossFadeOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    Transition *transPtr;
    double opacity;
    int w, h;
    
    if (imgPtr->clientData != NULL) {
        FreeTransition(imgPtr);
    }
    transPtr = Blt_AssertCalloc(1, sizeof(Transition));
    transPtr->numSteps = 10;
    transPtr->fromColor.u32 = 0xFFFFFFFF;
    transPtr->toColor.u32 = 0xFF000000;
    transPtr->interp = interp;
    transPtr->imgPtr = imgPtr;
    transPtr->count = 1;
    transPtr->specs = crossFadeTransitionSwitches;
    imgPtr->clientData = transPtr;
    if (Blt_GetPixelFromObj(NULL, objv[2], &transPtr->fromColor) != TCL_OK) {
        if (Blt_GetPictureFromObj(interp, objv[2], &transPtr->from) != TCL_OK) {
            goto error;
        }
    }
    if (Blt_GetPixelFromObj(NULL, objv[3], &transPtr->toColor) != TCL_OK) {
        if (Blt_GetPictureFromObj(interp, objv[3], &transPtr->to) != TCL_OK) {
            goto error;
        }
    }
    if (Blt_ParseSwitches(interp, transPtr->specs, objc - 4, objv + 4,
                          transPtr, BLT_SWITCH_DEFAULTS) < 0) {
        goto error;
    }
    if (transPtr->from == imgPtr->picture) {
        Tcl_AppendResult(interp, "\"from\" picture can not be \"",
                Tk_NameOfImage(imgPtr->imgToken), "\"", (char *)NULL);
        goto error;
    }
    if (transPtr->to == imgPtr->picture) {
        Tcl_AppendResult(interp, "\"to\" picture can not be \"",
                Tk_NameOfImage(imgPtr->imgToken), "\"", (char *)NULL);
        goto error;
    }
    if (transPtr->from == NULL) {
        if (transPtr->to == NULL) {
            Tcl_AppendResult(interp, "either from or to must ",
                             "be a picture image.", (char *)NULL);
            goto error;
        }
        w = Blt_Picture_Width(transPtr->to);
        h = Blt_Picture_Height(transPtr->to);
    } else if (transPtr->to == NULL) {
        w = Blt_Picture_Width(transPtr->from);
        h = Blt_Picture_Height(transPtr->from);
    } else {
        w = Blt_Picture_Width(transPtr->from);
        h = Blt_Picture_Height(transPtr->from);
        if ((w != Blt_Picture_Width(transPtr->to)) ||
            (h != Blt_Picture_Height(transPtr->to))) {
            Tcl_AppendResult(interp, "from and to picture ",
                             "must be the same size.", (char *)NULL);
            goto error;
        }
    }
    transPtr->picture = Blt_CreatePicture(w, h);
    if (transPtr->count > transPtr->numSteps) {
        transPtr->count = transPtr->numSteps;
    } 
    opacity = (double)transPtr->count / (double)transPtr->numSteps;
    if (transPtr->logScale) {
        opacity = log10(9.0 * opacity + 1.0);
    }
    if (transPtr->from == NULL) {
        Blt_FadeFromColor(transPtr->picture, transPtr->to, &transPtr->fromColor,
                opacity);
    } else if (transPtr->to == NULL) {
        Blt_FadeToColor(transPtr->picture, transPtr->from, &transPtr->toColor,
                opacity);
    } else {
        Blt_CrossFadePictures(transPtr->picture, transPtr->from, transPtr->to,
                              opacity);
    }
    ReplacePicture(transPtr->imgPtr, transPtr->picture);
    Blt_NotifyImageChanged(imgPtr);
    if (transPtr->interval > 0) {
        imgPtr->timerToken = Tcl_CreateTimerHandler(transPtr->interval, 
                CrossFadeTimerProc, transPtr);
    } else {
        FreeTransition(imgPtr);
    }
    return TCL_OK;
 error:
    FreeTransition(imgPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawOp --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec drawOps[] = {
    {"circle",    1, Blt_Picture_CircleOp,    4, 0, "x y r ?switches ...?",},
    {"ellipse",   1, Blt_Picture_EllipseOp,   5, 0, "x y a b ?switches ...?",},
    {"line",      1, Blt_Picture_LineOp,      3, 0, "?switches ...?",},
    {"polygon",   1, Blt_Picture_PolygonOp,   3, 0, "?switches ...?",},
    {"rectangle", 1, Blt_Picture_RectangleOp, 7, 0, "x1 y1 x2 y2 ?switches ...?",},
    {"text",      1, GetPictProc2,            6, 0, "string x y ?switches ...?",},
};
static int numDrawOps = sizeof(drawOps) / sizeof(Blt_OpSpec);
/*ARGSUSED*/
static int 
DrawOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    Blt_Picture picture;
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numDrawOps, drawOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    picture = CurrentPictureFromPictImage(imgPtr);
    result = (*proc) (picture, interp, objc, objv);
    if (result == TCL_OK) {
        Blt_NotifyImageChanged(imgPtr);
    }
    return result;
}


/*
 *---------------------------------------------------------------------------
 *
 * DissolveOp --
 *
 *      imageName dissolve fromImage toImage ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
DissolveOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    Transition *transPtr;
    int w, h;
    
    if (imgPtr->clientData != NULL) {
        FreeTransition(imgPtr);
    }
    transPtr = Blt_AssertCalloc(1, sizeof(Transition));
    transPtr->numSteps = 10;
    transPtr->fromColor.u32 = 0xFFFFFFFF;
    transPtr->toColor.u32 = 0xFF000000;
    transPtr->interp = interp;
    transPtr->imgPtr = imgPtr;
    transPtr->last = 1;
    transPtr->specs = dissolveTransitionSwitches;
    imgPtr->clientData = transPtr;
    if (Blt_GetPixelFromObj(NULL, objv[2], &transPtr->fromColor) != TCL_OK) {
        if (Blt_GetPictureFromObj(interp, objv[2], &transPtr->from) != TCL_OK) {
            goto error;
        }
    }
    if (Blt_GetPixelFromObj(NULL, objv[3], &transPtr->toColor) != TCL_OK) {
        if (Blt_GetPictureFromObj(interp, objv[3], &transPtr->to) != TCL_OK) {
            goto error;
        }
    }
    if (Blt_ParseSwitches(interp, transPtr->specs, objc - 4, objv + 4,
                          transPtr, BLT_SWITCH_DEFAULTS) < 0) {
        goto error;
    }
    if (transPtr->from == imgPtr->picture) {
        Tcl_AppendResult(interp, "\"from\" picture can not be \"",
                         Tk_NameOfImage(imgPtr->imgToken),
                         "\".", (char *)NULL);
        goto error;
    }
    if (transPtr->to == imgPtr->picture) {
        Tcl_AppendResult(interp, "\"to\" picture can not be \"",
                         Tk_NameOfImage(imgPtr->imgToken),
                         "\".", (char *)NULL);
        goto error;
    }
    if (transPtr->from == NULL) {
        if (transPtr->to == NULL) {
            Tcl_AppendResult(interp, "either \"from\" or \"to\" must ",
                             "be a picture image.", (char *)NULL);
            goto error;
        }
        w = Blt_Picture_Width(transPtr->to);
        h = Blt_Picture_Height(transPtr->to);
    } else if (transPtr->to == NULL) {
        w = Blt_Picture_Width(transPtr->from);
        h = Blt_Picture_Height(transPtr->from);
    } else {
        w = Blt_Picture_Width(transPtr->from);
        h = Blt_Picture_Height(transPtr->from);
        if ((w != Blt_Picture_Width(transPtr->to)) ||
            (h != Blt_Picture_Height(transPtr->to))) {
            Tcl_AppendResult(interp, "from and to picture ",
                             "must be the same size.", (char *)NULL);
            goto error;
        }
    }
    transPtr->numPixels = (w * h);
    transPtr->picture = Blt_CreatePicture(w, h);
    if (transPtr->from != NULL) {
        Blt_CopyPictureBits(transPtr->picture, transPtr->from);
    } else {
        Blt_BlankPicture(transPtr->picture, transPtr->fromColor.u32);
    } 
    DoDissolve(transPtr);
    ReplacePicture(transPtr->imgPtr, transPtr->picture);
    Blt_NotifyImageChanged(imgPtr);
    if (transPtr->interval > 0) {
        imgPtr->timerToken = Tcl_CreateTimerHandler(transPtr->interval, 
                DissolveTimerProc, transPtr);
    } else {
        FreeTransition(imgPtr);
    }
    return TCL_OK;
 error:
    FreeTransition(imgPtr);
    return TCL_ERROR;
}


/*
 *---------------------------------------------------------------------------
 *
 * DupOp --
 *
 *      Creates a duplicate of the current picture in a new picture image.
 *      The name of the new image is returned.
 *
 * Results:
 *      Returns a standard TCL return value.  If TCL_OK, The name of the new
 *      image command is returned via the interpreter result.  Otherwise an
 *      error message is returned.
 *
 * Side effects:
 *      A new image command is created.
 *
 *---------------------------------------------------------------------------
 */
static int
DupOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_Picture picture;
    DupSwitches switches;
    PictImage *imgPtr = clientData;
    Tcl_Obj *objPtr;

    memset(&switches, 0, sizeof(switches));
    InitArea(imgPtr->picture, &switches.from);
    if (Blt_ParseSwitches(interp, dupSwitches, objc - 2, objv + 2, &switches, 
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (!Blt_AdjustAreaToPicture(imgPtr->picture, &switches.from)) {
        Tcl_AppendResult(interp, "impossible coordinates for area", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    /* 
     * Create a new picture image. Let Tk automatically generate the command
     * name.
     */
    if (Tcl_Eval(interp, "image create picture") != TCL_OK) {
        return TCL_ERROR;
    }
    /* The interpreter result now contains the name of the image. */
    objPtr = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(objPtr);

    /* 
     * Reset the result just in case Blt_ResetPicture returns an error. 
     */
    Tcl_ResetResult(interp);    
    
    picture = Blt_CreatePicture(AREA_WIDTH(switches.from),
                                AREA_HEIGHT(switches.from));
    if (switches.nocopy) {              /* Set the picture to a blank image. */
        Blt_BlankPicture(picture, 0x0);
    } else if (switches.from.flags & (BLT_PICTURE_SIZE|BLT_PICTURE_COORDS)) {
        Blt_CopyArea(picture, imgPtr->picture, switches.from.x1,
                     switches.from.y1, AREA_WIDTH(switches.from),
                     AREA_HEIGHT(switches.from), 0, 0);
    } else {
        Blt_CopyPictureBits(picture, imgPtr->picture);
    }

    if (Blt_ResetPicture(interp, Tcl_GetString(objPtr), picture) != TCL_OK) {
        Tcl_DecrRefCount(objPtr);
        Blt_FreePicture(picture);
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objPtr);
    Tcl_DecrRefCount(objPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EmbossOp --
 *
 *      Emboss the picture.
 *
 *---------------------------------------------------------------------------
 */
static int
EmbossOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_Picture src, dst;
    PictImage *imgPtr = clientData;
    double azimuth, elevation;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    azimuth = 30.0;
    elevation = 30.0;
    if (objc == 5) {
        if ((Tcl_GetDoubleFromObj(interp, objv[3], &azimuth) != TCL_OK) ||
            (Tcl_GetDoubleFromObj(interp, objv[4], &elevation) != TCL_OK)) {
            return TCL_ERROR;
        }
    }
    dst = Blt_EmbossPicture(src, azimuth, elevation, 1.0);
    ReplacePicture(imgPtr, dst);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExportOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
ExportOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_PictFormat *fmtPtr;
    PictImage *imgPtr = clientData;
    int result;
    const char *fmt;

    if (objc == 2) {
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;

        for (hPtr = Blt_FirstHashEntry(&fmtTable, &iter); hPtr != NULL; 
             hPtr = Blt_NextHashEntry(&iter)) {
            Blt_PictFormat *fmtPtr;

            fmtPtr = Blt_GetHashValue(hPtr);
            if ((fmtPtr->flags & FMT_LOADED) == 0) {
                continue;
            }
            if (fmtPtr->exportProc == NULL) {
                continue;
            }
            Tcl_AppendElement(interp, fmtPtr->name);
        }
        return TCL_OK;
    }
    fmt = Tcl_GetString(objv[2]);
    hPtr = Blt_FindHashEntry(&fmtTable, fmt);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "can't export \"", fmt,
                         "\": format not registered", (char *)NULL);
        return TCL_ERROR;
    }
    fmtPtr = Blt_GetHashValue(hPtr);
    if ((fmtPtr->flags & FMT_LOADED) == 0) {
        LoadPackage(interp, fmt);
    }
    if (fmtPtr->exportProc == NULL) {
        Tcl_AppendResult(interp, "can't find picture export procedure for \"", 
                        fmtPtr->name, "\" format.", (char *)NULL);
        return TCL_ERROR;
    }
    result = (*fmtPtr->exportProc)(interp, imgPtr->current, imgPtr->chain, 
        objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * FadeOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
FadeOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_Picture src;
    PictImage *imgPtr = clientData;
    int w, h;
    double percent;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[3], &percent) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((percent < 0.0) || (percent > 100.0)) {
        Tcl_AppendResult(interp, "bad fade percentage \"",
                Tcl_GetString(objv[3]), "\" should be between 0 and 100.",
                (char *)NULL);
        return TCL_ERROR;
    }
    w = Blt_Picture_Width(src);
    h = Blt_Picture_Height(src);
    if (imgPtr->picture != src) {
        ResizeCopyPictureBits(imgPtr->picture, src);
    }
    Blt_FadePicture(imgPtr->picture, 0, 0, w, h, percent * 0.01);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlipOp --
 *
 *      Flips the picture either horizontally or vertically.
 *
 *      destName flip x ?srcName?
 *      destName flip y ?srcName?
 *---------------------------------------------------------------------------
 */
static int
FlipOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    const char *string;
    int isVertical;

    string = Tcl_GetString(objv[2]);
    if ((string[0] ==  'x') && (string[1] == '\0')) {
        isVertical = FALSE;
    } else if ((string[0] ==  'y') && (string[1] == '\0')) {
        isVertical = TRUE;
    } else {
        Tcl_AppendResult(interp, "bad flip argument \"", string, 
                "\": should be x or y", (char *)NULL);
        return TCL_ERROR;
    }
    if (objc > 3) {
        Blt_Picture src;
        
        if (Blt_GetPictureFromObj(interp, objv[3], &src) != TCL_OK) {
            return TCL_ERROR;
        }
        if (src != imgPtr->picture) {
            ResizeCopyPictureBits(imgPtr->picture, src);
        }
    }
    Blt_FlipPicture(imgPtr->picture, isVertical);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GammaOp --
 *
 *      imageName gamma gammValue
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
GammaOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    double gamma;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &gamma) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_GammaCorrectPicture(imgPtr->picture, imgPtr->picture, (float)gamma);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 *      Returns the RGBA components of the pixel at the specified coordinate.
 *
 * Results:
 *      Returns a standard TCL return value.  If TCL_OK, the components of
 *      the pixel are returned as a list in the interpreter result.
 *      Otherwise an error message is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
GetOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    Blt_Pixel *sp, pixel;
    int x, y;

    if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
        return TCL_ERROR;
    }
    if ((x < 0) || (x >= Blt_Picture_Width(imgPtr->picture))) {
        Tcl_AppendResult(interp, "x-coordinate \"", Tcl_GetString(objv[2]),
                "\" is out of range", (char *)NULL);
        return TCL_ERROR;
    }
    if ((y < 0) || (y >= Blt_Picture_Height(imgPtr->picture))) {
        Tcl_AppendResult(interp, "y-coordinate \"", Tcl_GetString(objv[3]),
                "\" is out of range", (char *)NULL);
        return TCL_ERROR;
    }
    sp = Blt_Picture_Pixel(imgPtr->picture, x, y);
    pixel = *sp;
#ifdef notdef
    if ((Blt_Picture_Flags(imgPtr->picture) & BLT_PIC_PREMULT_COLORS) &&
        ((sp->Alpha != 0xFF) && (sp->Alpha != 0x00))) {
        int bias = sp->Alpha >> 1;

        pixel.Red   = (mul255(sp->Red)   + bias) / sp->Alpha;
        pixel.Green = (mul255(sp->Green) + bias) / sp->Alpha;
        pixel.Blue  = (mul255(sp->Blue)  + bias) / sp->Alpha;
    }
#endif
#ifdef notdef
    fprintf(stderr, "[0x%02x][0x%02x][0x%02x][0x%02x] [0x%04x][0x%04x] [0x%08x]\n",
            sp->Blue, sp->Green, sp->Red, sp->Alpha,
            sp->u32 & 0xFFFF, sp->u32 >> 16,
            sp->u32); 
#endif
    Tcl_SetObjResult(interp, Tcl_NewStringObj(Blt_NameOfPixel(&pixel), -1));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GreyscaleOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
GreyscaleOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Blt_Picture src, dst;
    PictImage *imgPtr = clientData;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    dst = Blt_GreyscalePicture(src);
    ReplacePicture(imgPtr, dst);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HeightOp --
 *
 *      Returns the current height of the picture.
 *
 *---------------------------------------------------------------------------
 */
static int
HeightOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int h;

    h = 0;
    if (objc == 3) {
        int w;

        if (Tcl_GetIntFromObj(interp, objv[2], &h) != TCL_OK) {
            return TCL_ERROR;
        }
        w = Blt_Picture_Width(imgPtr->picture);
        Blt_AdjustPictureSize(imgPtr->picture, w, h);
        Blt_NotifyImageChanged(imgPtr);
    } 
    if (imgPtr->picture != NULL) {
        h = Blt_Picture_Height(imgPtr->picture);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), h);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImportOp --
 *
 *      Imports an image source into a picture.  The image source can be a
 *      file, base64 string, or binary Tcl_Obj.  This performs basically
 *      the same function as "configure".  The extra functionality this
 *      command has is based upon the ability to pass extra flags to the
 *      various converters, something that can't really be done with the
 *      "configure" operation.
 *
 *              imageName configure -file fileName
 *
 * Results:
 *      Returns a standard TCL return value.  If no error, the interpreter
 *      result will contain the number of images successfully read.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
ImportOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_Chain chain;
    Blt_ChainLink link;
    Blt_HashEntry *hPtr;
    Blt_PictFormat *fmtPtr;
    PictImage *imgPtr = clientData;
    const char *fileName;
    const char *fmt;

    if (objc == 2) {
        Blt_PictFormat *fp, *fend;
        
        for (fp = pictFormats, fend = fp + NUMFMTS; fp < fend; fp++) {
            if ((fp->flags & FMT_LOADED) == 0) {
                continue;
            }
            if (fp->importProc == NULL) {
                continue;
            }
            Tcl_AppendElement(interp, fp->name);
        }
        return TCL_OK;
    }
    fmt = Tcl_GetString(objv[2]);
    hPtr = Blt_FindHashEntry(&fmtTable, fmt);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "unknown picture format \"", fmt, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    fmtPtr = Blt_GetHashValue(hPtr);
    if ((fmtPtr->flags & FMT_LOADED) == 0) {
        LoadPackage(interp, fmt);
    }
    if (fmtPtr->importProc == NULL) {
        Tcl_AppendResult(interp, "can't find picture import procedure for \"", 
                fmtPtr->name, "\" format.", (char *)NULL);
        return TCL_ERROR;
    }
    chain = (*fmtPtr->importProc)(interp, objc, objv, &fileName);
    if (chain == NULL) {
        return TCL_ERROR;
    }
    FreePictures(imgPtr);
    imgPtr->chain = chain;
    link = GetFirstImageIndex(imgPtr, &imgPtr->current);
    if (link != NULL) {
        imgPtr->picture = Blt_Chain_GetValue(link);
    } else {
        imgPtr->picture = NULL;
    }

    /* 
     * Save the format type and file name in the image record.  The file
     * name is used when querying image options -file or -data via
     * configure.  The type is used by the "-data" operation to establish a
     * default format to output.
     */
    imgPtr->fmtPtr = fmtPtr;
    imgPtr->flags &= ~IMPORTED_MASK;
    if (imgPtr->sourceName != NULL) {
        Blt_Free(imgPtr->sourceName);
        imgPtr->sourceName = NULL;
    }
    if (fileName == NULL) {
        imgPtr->sourceName = NULL;
        imgPtr->flags |= IMPORTED_DATA;
    } else {
        imgPtr->sourceName = Blt_AssertStrdup(fileName);
        imgPtr->flags |= IMPORTED_FILE;
    }
    Blt_NotifyImageChanged(imgPtr);

    /* Return the number of separates images read. */
    Tcl_SetIntObj(Tcl_GetObjResult(interp), Blt_Chain_GetLength(imgPtr->chain));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InfoOp --
 *
 *      Reports the basic information about a picture.  
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
InfoOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int numColors, state;
    Tcl_Obj *listObjPtr, *objPtr;
    Pict *srcPtr;

    srcPtr = imgPtr->picture;
    Blt_ClassifyPicture(srcPtr);
    numColors = Blt_QueryColors(srcPtr, (Blt_HashTable *)NULL);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);

    objPtr = Tcl_NewStringObj("colors", 6);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(numColors);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("premultipled", 12);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    state = srcPtr->flags & BLT_PIC_PREMULT_COLORS;
    objPtr = Tcl_NewBooleanObj(state);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("greyscale", 9);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    state = Blt_Picture_IsGreyscale(srcPtr);
    objPtr = Tcl_NewBooleanObj(state);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("masked", 6);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    state = Blt_Picture_IsMasked(srcPtr);
    objPtr = Tcl_NewBooleanObj(state);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("composite", 9);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    state = Blt_Picture_IsBlended(srcPtr);
    objPtr = Tcl_NewBooleanObj(state);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("width", 5);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(Blt_Picture_Width(srcPtr));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    
    objPtr = Tcl_NewStringObj("height", 6);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(Blt_Picture_Height(srcPtr));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("count", 5);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(Blt_Chain_GetLength(imgPtr->chain));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("index", 5);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(imgPtr->current);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("format", 6);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewStringObj((imgPtr->fmtPtr != NULL) ? 
        imgPtr->fmtPtr->name : "none", 4);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MultiplyOp --
 *
 *      imageName multiply scalar
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
MultiplyOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    double scalar;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &scalar) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_MultiplyPixels(imgPtr->picture, imgPtr->picture, (float)scalar);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * SequenceAppendOp --
 *
 *      imageName sequence append pictName...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequenceAppendOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int i;

    for (i = 3; i < objc; i++) {
        Blt_Picture src, dst;

        if (Blt_GetPictureFromObj(interp, objv[i], &src) != TCL_OK) {
            return TCL_ERROR;
        }
        dst = Blt_ClonePicture(src);
        Blt_Chain_Append(imgPtr->chain, dst);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SequenceDeleteOp --
 *
 *      imageName sequence delete firstIndex ?lastIndex?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequenceDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int first;

    if (GetSequenceIndexFromObj(interp, imgPtr, objv[3], &first) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        Blt_ChainLink link;
        Blt_Picture picture;

        link = Blt_Chain_GetNthLink(imgPtr->chain, first);
        if (link == NULL) {
            return TCL_OK;
        }
        picture = Blt_Chain_GetValue(link);
        if (picture != NULL) {
            Blt_FreePicture(picture);
        }
        Blt_Chain_DeleteLink(imgPtr->chain, link);
    } else {
        int i;
        int last;
        Blt_ChainLink link, next;

        if (GetSequenceIndexFromObj(interp, imgPtr, objv[4], &last) != TCL_OK) {
            return TCL_ERROR;
        }
        if (first > last) {
            return TCL_OK;
        }
        for (i = 0, link = Blt_Chain_FirstLink(imgPtr->chain); link != NULL;
             link = next, i++) {
            next = Blt_Chain_NextLink(link);
            if ((i >= first) && (i <= last)) {
                Blt_Picture picture;
                
                picture = Blt_Chain_GetValue(link);
                if (picture != NULL) {
                    Blt_FreePicture(picture);
                }
                Blt_Chain_DeleteLink(imgPtr->chain, link);
            }
        }
    }
    FirstImage(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SequenceDelayOp --
 *
 *      imageName sequence delay ?milliseconds?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequenceDelayOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    
    if (objc == 4) {
        int delay;

        if (Tcl_GetIntFromObj(interp, objv[3], &delay) != TCL_OK) {
            return TCL_ERROR;
        }
        imgPtr->interval = delay;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), imgPtr->interval);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * SequenceGetOp --
 *
 *      imageName sequence get indexName ?pictName?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequenceGetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int index;
    
    index = -1;
    if (GetSequenceIndexFromObj(NULL, imgPtr, objv[3], &index) == TCL_OK) {
        if (objc == 5) {
            Blt_Picture picture;

            picture = Blt_GetNthPicture(imgPtr->chain, index);
            if (picture != NULL) {
                if (Blt_ResetPicture(interp, Tcl_GetString(objv[4]), picture)
                    != TCL_OK) {
                    return TCL_ERROR;
                }
            } else {
                index = -1;             /* Override the index */
            }
        }
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SequenceIndexOp --
 *
 *      imageName sequence index indexName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequenceIndexOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int index;
    
    index = -1;
    GetSequenceIndexFromObj(NULL, imgPtr, objv[3], &index);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SequenceLengthOp --
 *
 *      imageName sequence length ?newLength?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequenceLengthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int numPicts;

    numPicts = Blt_Chain_GetLength(imgPtr->chain);
    if (objc == 4) {
        long count;
        
        if (Blt_GetCountFromObj(interp, objv[3], COUNT_NNEG, &count) != TCL_OK){
            return TCL_OK;
        }
        if (count < numPicts) {
            Blt_ChainLink link, next;
            
            /* Remove trailing links from list of images. */
            if (imgPtr->current >= count) {
                PreviousImage(imgPtr);
            }
            link = Blt_Chain_GetNthLink(imgPtr->chain, count);
            for (/*empty*/; link != NULL; link = next) {
                Blt_Picture picture;
                
                next = Blt_Chain_NextLink(link);
                picture = Blt_Chain_GetValue(link);
                if (picture != NULL) {
                    Blt_FreePicture(picture);
                }
                Blt_Chain_DeleteLink(imgPtr->chain, link);
            }
        } else if (count > numPicts) {
            int i;
            
            /* Assume the index is valid, create empty slots. */
            for (i = Blt_Chain_GetLength(imgPtr->chain); i < count; i++) {
                Blt_Chain_Append(imgPtr->chain, NULL);
            }
        }
    }
    numPicts = Blt_Chain_GetLength(imgPtr->chain);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), numPicts);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SequencePutOp --
 *
 *      imageName sequence put indexName pictName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequencePutOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    Blt_Picture picture;
    Blt_ChainLink link;
    int index;
    
    if (GetSequenceIndexFromObj(interp, imgPtr, objv[3], &index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_GetPictureFromObj(interp, objv[4], &picture) != TCL_OK) {
        return TCL_ERROR;
    }
    picture = Blt_ClonePicture(picture);
    link = Blt_Chain_GetNthLink(imgPtr->chain, index);
    Blt_Chain_SetValue(link, picture);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SequenceReplaceOp --
 *
 *      imageName sequence replace firstIndex lastIndex pictName...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequenceReplaceOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int i;
    int first, last;
    Blt_ChainLink link, next, head, tail;

    if ((GetSequenceIndexFromObj(interp, imgPtr, objv[3], &first) != TCL_OK) ||
        (GetSequenceIndexFromObj(interp, imgPtr, objv[4], &last) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (first > last) {
        return TCL_OK;
    }
    head = tail = NULL;
    for (i = 0, link = Blt_Chain_FirstLink(imgPtr->chain); link != NULL;
         link = next, i++) {
        next = Blt_Chain_NextLink(link);
        if ((i >= first) && (i <= last)) {
            Blt_Picture picture;

            picture = Blt_Chain_GetValue(link);
            if (picture != NULL) {
                Blt_FreePicture(picture);
            }
            Blt_Chain_DeleteLink(imgPtr->chain, link);
        } else if (head == NULL) {
            head = link;
        } else if (tail == NULL) {
            tail = link;
        }
    }
    if (head != NULL) {
        for (i = 5; i < objc; i++) {
            Blt_Picture src, dst;

            if (Blt_GetPictureFromObj(interp, objv[i], &src) != TCL_OK) {
                return TCL_ERROR;
            }
            dst = Blt_ClonePicture(src);
            link = Blt_Chain_NewLink();
            Blt_Chain_SetValue(link, dst);
            Blt_Chain_LinkAfter(imgPtr->chain, link, head);
            head = link;
        }
    } else if (tail != NULL) {
        for (i = 5; i < objc; i++) {
            Blt_Picture src, dst;

            if (Blt_GetPictureFromObj(interp, objv[i], &src) != TCL_OK) {
                return TCL_ERROR;
            }
            dst = Blt_ClonePicture(src);
            link = Blt_Chain_NewLink();
            Blt_Chain_SetValue(link, dst);
            Blt_Chain_LinkBefore(imgPtr->chain, link, tail);
        }
    } else {
        assert(Blt_Chain_GetLength(imgPtr->chain) == 0);
        for (i = 5; i < objc; i++) {
            Blt_Picture src, dst;

            if (Blt_GetPictureFromObj(interp, objv[i], &src) != TCL_OK) {
                return TCL_ERROR;
            }
            dst = Blt_ClonePicture(src);
            Blt_Chain_Append(imgPtr->chain, dst);
        }
    }   
    FirstImage(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SequenceSeeOp --
 *
 *      imageName sequence see ?indexName?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequenceSeeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;

    if (objc == 4) {
        int index;
        Blt_Picture picture;
        
        if (GetSequenceIndexFromObj(interp, imgPtr, objv[3], &index) != TCL_OK){
            return TCL_ERROR;
        }
        picture = Blt_GetNthPicture(imgPtr->chain, index);
        if (picture == NULL) {
            Tcl_AppendResult(interp, "no picture at sequence slot \"",
                             Tcl_GetString(objv[3]), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        imgPtr->current = index;
        imgPtr->picture = picture;
        Blt_NotifyImageChanged(imgPtr);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), imgPtr->current);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SequenceStartOp --
 *
 *      imageName sequence start 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequenceStartOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int delay;
    
    if (imgPtr->timerToken != 0) {
        return TCL_OK;
    }
    FirstImage(imgPtr);
    delay = Blt_Picture_Delay(imgPtr->picture);
    imgPtr->timerToken = Tcl_CreateTimerHandler(delay, SequenceTimerProc,
        imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SequenceStopOp --
 *
 *      imageName sequence stop
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequenceStopOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    
    if (imgPtr->timerToken != 0) {
        Tcl_DeleteTimerHandler(imgPtr->timerToken);
        imgPtr->timerToken = 0;
    }
    return TCL_OK;
}

static Blt_OpSpec sequenceOps[] =
{
    {"append",  2, SequenceAppendOp,  3, 0, "?pictName...?",},
    {"delay",   4, SequenceDelayOp,   3, 4, "?milliseconds?",},
    {"delete",  4, SequenceDeleteOp,  4, 5, "firstIndex ?lastIndex?",},
    {"get",     1, SequenceGetOp,     4, 5, "indexName ?pictName?",},
    {"index",   1, SequenceIndexOp,   4, 4, "indexName",},
    {"length",  1, SequenceLengthOp,  3, 4, "?newLength?",},
    {"put",     1, SequencePutOp,     5, 5, "indexName pictName",},
    {"replace", 1, SequenceReplaceOp, 5, 0, "firstIndex lastIndex ?pictName...?",},
    {"see",     2, SequenceSeeOp,     3, 4, "?indexName?",},
    {"start",   3, SequenceStartOp,   3, 3, "",},
    {"stop",    3, SequenceStopOp,    3, 3, "",},
};

static int numSequenceOps = sizeof(sequenceOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * SequenceOp --
 *
 *      imageName sequence append pictName...
 *      imageName sequence delay ?milliseconds?
 *      imageName sequence delete firstIndex ?lastIndex?
 *      imageName sequence get indexName ?pictName?
 *      imageName sequence index indexName
 *      imageName sequence length ?newLength?
 *      imageName sequence put indexName pictName
 *      imageName sequence replace firstIndex lastIndex ?pictName...?
 *      imageName sequence see ?indexName?
 *      imageName sequence start
 *      imageName sequence stop
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SequenceOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    PictCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numSequenceOps, sequenceOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}



/*
 *---------------------------------------------------------------------------
 *
 * ProjectOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ProjectOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    Blt_Picture src, dest;
    PictImage *imgPtr = clientData;
    float srcPts[8], destPts[8];
    ProjectSwitches switches;
    Tcl_Obj **ev;
    int ec;
    int i;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    if (Tcl_ListObjGetElements(interp, objv[3], &ec, &ev) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ec != 8) {
        Tcl_AppendResult(interp, "wrong # of elements in source coordinates: ",
                "should be \"x1 y1 x2 y2 x3 y3 x4 y4\"", (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 0; i < 8; i++) {
        double x;

        if (Tcl_GetDoubleFromObj(interp, ev[i], &x) != TCL_OK) {
            return TCL_ERROR;
        }
        srcPts[i] = (float)x;
    }
    if (Tcl_ListObjGetElements(interp, objv[4], &ec, &ev) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ec != 8) {
        Tcl_AppendResult(interp, 
                "wrong # of elements in destination coordinates: ",
                "should be \"x1 y1 x2 y2 x3 y3 x4 y4\"", (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 0; i < 8; i++) {
        double x;

        if (Tcl_GetDoubleFromObj(interp, ev[i], &x) != TCL_OK) {
            return TCL_ERROR;
        }
        destPts[i] = (float)x;
    }
    if (Blt_ParseSwitches(interp, projectSwitches, objc - 5, objv + 5, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    dest = Blt_ProjectPicture(src, srcPts, destPts, &switches.bg);
    ReplacePicture(imgPtr, dest);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PutOp --
 *
 *      Changes the color of the pixel at the given coordinates. 
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *      imageName put x y color
 *
 *---------------------------------------------------------------------------
 */
static int
PutOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_Pixel *dp;
    Blt_Pixel pixel;
    PictImage *imgPtr = clientData;
    int x, y;
    
    if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
        return TCL_ERROR;
    }
    if ((x < 0) || (x >= Blt_Picture_Width(imgPtr->picture))) {
        Tcl_AppendResult(interp, "bad x coordinate \"", Tcl_GetString(objv[2]),
                "\" coordinate is outside picture.", (char *)NULL);
        return TCL_ERROR;
    }
    if ((y < 0) || (y >= Blt_Picture_Height(imgPtr->picture))) {
        Tcl_AppendResult(interp, "bad y coordinate \"", Tcl_GetString(objv[3]),
                "\" coordinate is outside picture.", (char *)NULL);
        return TCL_ERROR;
    }
    if (Blt_GetPixelFromObj(interp, objv[4], &pixel) != TCL_OK) {
        return TCL_ERROR;
    }
    dp = Blt_Picture_Pixel(imgPtr->picture, x, y);
    dp->u32 = pixel.u32;
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * QuantizeOp --
 *
 *      imageName quantize srcName numColors
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
QuantizeOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_Picture src, dest;
    PictImage *imgPtr = clientData;
    int numColors;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &numColors) != TCL_OK) {
        return TCL_ERROR;
    }
    if (numColors < 2) {
        Tcl_AppendResult(interp, "Invalid # of color \"",
                Tcl_GetString(objv[3]), "\": should be >= 2", (char *)NULL);
        return TCL_ERROR;
    }
    dest = Blt_QuantizePicture(src, numColors);
    if (dest == NULL) {
        return TCL_ERROR;
    }
    ReplacePicture(imgPtr, dest);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReflectOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ReflectOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    Pict *srcPtr, *destPtr, *tmpPtr;
    PictImage *imgPtr = clientData;
    ReflectSwitches switches;
    int w, h;
    int dw, dh;

    if (Blt_GetPictureFromObj(interp, objv[2], &srcPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    switches.side = SIDE_BOTTOM;
    switches.jitter.range = 0.1;
    switches.scale = SCALE_LINEAR;
    switches.low = 0.0;
    switches.high = 1.0;
    switches.blur = 1;
    switches.bgColor.u32 = 0x0;

    if (Blt_ParseSwitches(interp, reflectSwitches, objc - 3, objv + 3, 
                &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    w = srcPtr->width, h = srcPtr->height;
    dw = srcPtr->width, dh = srcPtr->height;
    destPtr = NULL;                     /* Suppress compiler warning. */
    switch (switches.side) {
    case SIDE_BOTTOM:
        h = srcPtr->height / 2;
        dh = srcPtr->height + h;
        destPtr = Blt_CreatePicture(w, h);
        Blt_CopyArea(destPtr, srcPtr, 0, srcPtr->height - h, srcPtr->width,
                       h, 0, 0);
        break;

    case SIDE_TOP:
        h = srcPtr->height / 2;
        dh = srcPtr->height + h;
        destPtr = Blt_CreatePicture(w, h);
        Blt_CopyArea(destPtr, srcPtr, 0, 0, srcPtr->width, h, 0, 0);
        break;

    case SIDE_LEFT:
    case SIDE_RIGHT:
        Tcl_AppendResult(interp, "side left/right not implemented",
                (char *)NULL);
        w = srcPtr->width / 2;
        dw = srcPtr->width + w;
        break;
    }
    if (switches.blur > 0) { 
        int r;

        r = 1;
        tmpPtr = destPtr;
        destPtr = Blt_CreatePicture(w, h);
        Blt_BlurPicture(destPtr, tmpPtr, r, switches.blur);
        Blt_FreePicture(tmpPtr);
    }
    tmpPtr = destPtr;
    destPtr = Blt_ReflectPicture2(tmpPtr, switches.side);
    Blt_FreePicture(tmpPtr);
    JitterInit(&switches.jitter);
    Blt_FadePictureWithGradient(destPtr, switches.side, switches.low,
                                switches.high, switches.scale,
                                &switches.jitter);
    if (switches.bgColor.u32 != 0) {
        tmpPtr = destPtr;
        destPtr = Blt_CreatePicture(destPtr->width, destPtr->height);
        Blt_BlankPicture(destPtr, switches.bgColor.u32);
        Blt_CompositePictures(destPtr, tmpPtr);
        Blt_FreePicture(tmpPtr);
    }
    tmpPtr = destPtr;
    destPtr = Blt_CreatePicture(dw, dh);

    switch (switches.side) {
    case SIDE_BOTTOM:
        Blt_CopyArea(destPtr, srcPtr, 0, 0, srcPtr->width, srcPtr->height,
                       0, 0);
        Blt_CopyArea(destPtr, tmpPtr, 0, 0, w, h, 0, srcPtr->height);
        break;

    case SIDE_TOP:
        Blt_CopyArea(destPtr, tmpPtr, 0, 0, w, h, 0, 0);
        Blt_CopyArea(destPtr, srcPtr, 0, 0, srcPtr->width, srcPtr->height,
                       0, h);
        break;

    case SIDE_LEFT:
        Blt_CopyArea(destPtr, tmpPtr, 0, 0, w, h, 0, 0);
        Blt_CopyArea(destPtr, srcPtr, 0, 0, srcPtr->width, srcPtr->height,
                       w, 0);
        break;

    case SIDE_RIGHT:
        Blt_CopyArea(destPtr, srcPtr, 0, 0, srcPtr->width, srcPtr->height,
                       0, 0);
        Blt_CopyArea(destPtr, tmpPtr, 0, 0, w, h, srcPtr->width, 0);
        break;
    }
    Blt_FreePicture(tmpPtr);
    ReplacePicture(imgPtr, destPtr);
    Blt_NotifyImageChanged(imgPtr);
    Blt_FreeSwitches(reflectSwitches, &switches, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ResampleOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ResampleOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_Picture src, tmp;
    PictImage *imgPtr = clientData;
    ResampleSwitches switches;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    /* Default source area is the entire picture. */
    switches.from.x2 = Blt_Picture_Width(src);
    switches.from.y2 = Blt_Picture_Height(src);
    /* Default destination picture size */
    switches.width    = Blt_Picture_Width(imgPtr->picture);
    switches.height   = Blt_Picture_Height(imgPtr->picture);
    if (Blt_ParseSwitches(interp, resampleSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (!Blt_AdjustAreaToPicture(src, &switches.from)) {
        Tcl_AppendResult(interp, "impossible coordinates for area", 
                (char *)NULL);
        return TCL_ERROR;
    }
    if ((switches.flags | imgPtr->flags) & MAXPECT) {
        double xScale, yScale, s;
            
        xScale = (double)switches.width  / (double)AREA_WIDTH(switches.from);
        yScale = (double)switches.height / (double)AREA_HEIGHT(switches.from);
        s = MIN(xScale, yScale);
        switches.width  = (int)(AREA_WIDTH(switches.from) * s + 0.5);
        switches.height = (int)(AREA_HEIGHT(switches.from) * s + 0.5);
    }       
    if ((Blt_Picture_Width(imgPtr->picture) != switches.width) ||
        (Blt_Picture_Height(imgPtr->picture) != switches.height)) {
        Blt_AdjustPictureSize(imgPtr->picture, switches.width, switches.height);
    }
    if (switches.vFilter == NULL) {
        switches.vFilter = switches.filter;
    }
    if (switches.hFilter == NULL) {
        switches.hFilter = switches.filter;
    }
    if (switches.hFilter == NULL) {
        switches.hFilter = (AREA_WIDTH(switches.from) < switches.width) ?
            bltMitchellFilter : bltBoxFilter; 
    }
    if (switches.vFilter == NULL) {
        switches.vFilter = (AREA_HEIGHT(switches.from) < switches.height) ?
            bltMitchellFilter : bltBoxFilter;
    }
    tmp = Blt_CreatePicture(AREA_WIDTH(switches.from),
                            AREA_HEIGHT(switches.from));
    Blt_CopyArea(tmp, src, switches.from.x1, switches.from.y1,
                 AREA_WIDTH(switches.from), AREA_HEIGHT(switches.from), 0, 0);
    Blt_ResamplePicture(imgPtr->picture, tmp, switches.vFilter,
                        switches.hFilter);
    Blt_FreePicture(tmp);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ResampleOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ResampleOp2(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Blt_Picture src, tmp;
    PictImage *imgPtr = clientData;
    ResampleSwitches switches;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    /* Default source area is the entire picture. */
    switches.from.x2 = Blt_Picture_Width(src);
    switches.from.y2 = Blt_Picture_Height(src);
    /* Default destination picture size */
    switches.width    = Blt_Picture_Width(imgPtr->picture);
    switches.height   = Blt_Picture_Height(imgPtr->picture);
    if (Blt_ParseSwitches(interp, resampleSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (!Blt_AdjustAreaToPicture(src, &switches.from)) {
        Tcl_AppendResult(interp, "impossible coordinates for area", 
                (char *)NULL);
        return TCL_ERROR;
    }
    if ((switches.flags | imgPtr->flags) & MAXPECT) {
        double xScale, yScale, s;
            
        xScale = (double)switches.width  / (double)AREA_WIDTH(switches.from);
        yScale = (double)switches.height / (double)AREA_HEIGHT(switches.from);
        s = MIN(xScale, yScale);
        switches.width  = (int)(AREA_WIDTH(switches.from) * s + 0.5);
        switches.height = (int)(AREA_HEIGHT(switches.from) * s + 0.5);
    }       
    if ((Blt_Picture_Width(imgPtr->picture) != switches.width) ||
        (Blt_Picture_Height(imgPtr->picture) != switches.height)) {
        Blt_AdjustPictureSize(imgPtr->picture, switches.width, switches.height);
    }
    if (switches.vFilter == NULL) {
        switches.vFilter = switches.filter;
    }
    if (switches.hFilter == NULL) {
        switches.hFilter = switches.filter;
    }
    if (switches.hFilter == NULL) {
        switches.hFilter = (AREA_WIDTH(switches.from) < switches.width) ?
            bltMitchellFilter : bltBoxFilter; 
    }
    if (switches.vFilter == NULL) {
        switches.vFilter = (AREA_HEIGHT(switches.from) < switches.height) ?
            bltMitchellFilter : bltBoxFilter;
    }
    tmp = Blt_CreatePicture(AREA_WIDTH(switches.from),
                            AREA_HEIGHT(switches.from));
    Blt_CopyArea(tmp, src, switches.from.x1, switches.from.y1,
                 AREA_WIDTH(switches.from), AREA_HEIGHT(switches.from), 0, 0);
    Blt_ResamplePicture2(imgPtr->picture, tmp, switches.vFilter, 
        switches.hFilter);
    Blt_FreePicture(tmp);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RotateOp --
 *
 *      $img rotate $src 90 
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
RotateOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_Picture src, dst;
    PictImage *imgPtr = clientData;
    double angle;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetDoubleFromObj(interp, objv[3], &angle) != TCL_OK) {
        const char *string;

        string = Tcl_GetString(objv[3]);
        if (Tcl_ExprDouble(interp, string, &angle) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    dst = Blt_RotatePicture(src, (float)angle);
    ReplacePicture(imgPtr, dst);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_Picture src;
    PictImage *imgPtr = clientData;
    Blt_Pixel lower, upper;
    unsigned char tmp;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_GetPixelFromObj(interp, objv[3], &lower) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 5) {
        if (Blt_GetPixelFromObj(interp, objv[4], &upper) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        upper.u32 = lower.u32;
    }
    if (lower.Red > upper.Red) {
        tmp = lower.Red, lower.Red = upper.Red, upper.Red = tmp;
    }
    if (lower.Green > upper.Green) {
        tmp = lower.Green, lower.Green = upper.Green, upper.Green = tmp;
    }
    if (lower.Blue > upper.Blue) {
        tmp = lower.Blue, lower.Blue = upper.Blue, upper.Blue = tmp;
    }
    if (lower.Alpha > upper.Alpha) {
        tmp = lower.Alpha, lower.Alpha = upper.Alpha, upper.Alpha = tmp;
    }
    Blt_SelectPixels(imgPtr->picture, src, &lower, &upper);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SharpenOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
SharpenOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    
    Blt_SharpenPicture(imgPtr->picture, imgPtr->picture);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SnapOp --
 *
 * Results:
 *      Returns a standard TCL return value.
 *
 * Side effects:
 *      None.
 *
 *   imageName snap windowName ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
static int
SnapOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Blt_Picture picture;
    PictImage *imgPtr = clientData;
    SnapArgs args;
    Tk_Window tkwin;
    const char *string;

    memset(&args, 0, sizeof(args));
    string = Tcl_GetString(objv[2]);
    tkwin = Tk_NameToWindow(NULL, string, Tk_MainWindow(interp));
    if (tkwin != NULL) {
        Tk_Uid classUid;

        classUid = Tk_Class(tkwin);
        if (strcmp(classUid, "Canvas") == 0) {
            int w, h;
            
            w = Tk_Width(tkwin);
            h = Tk_Height(tkwin);
            args.from.x1 = args.from.y1 = 0;
            args.width  = args.from.x2 = w;
            args.height = args.from.y2 = h;
            if (Blt_ParseSwitches(interp, snapSwitches, objc - 3, objv + 3, 
                                  &args, BLT_SWITCH_DEFAULTS) < 0) {
                return TCL_ERROR;
            }
            if (args.from.x2 > w) {
                args.from.x2 = w;
            }
            if (args.from.y2 > h) {
                args.from.y2 = h;
            }
            picture = Blt_CanvasToPicture(interp, tkwin, imgPtr->gamma);
            if (picture == NULL) {
                Tcl_AppendResult(interp, "can't obtain snapshot of window \"", 
                                 Tcl_GetString(objv[2]), "\"", (char *)NULL);
                return TCL_ERROR;
            }
            if (Blt_SwitchChanged(snapSwitches, "-from", (char *)NULL)) {
                Blt_Picture newPict;
                
                newPict = Blt_CreatePicture(AREA_WIDTH(args.from),
                                            AREA_HEIGHT(args.from));
                Blt_CopyArea(newPict, picture, args.from.x1, args.from.y1, 
                             AREA_WIDTH(args.from), AREA_HEIGHT(args.from),
                             0, 0);
                Blt_FreePicture(picture);
                picture = newPict;
            }
        } else if ((strcmp(classUid, "BltGraph") == 0) ||
                   (strcmp(classUid, "BltBarchart") == 0) ||
                   (strcmp(classUid, "BltStripchart") == 0) ||
                   (strcmp(classUid, "BltContour") == 0)) {
            int w, h;
            
            w = Tk_Width(tkwin);
            h = Tk_Height(tkwin);
            if (w < 2) {
                w = Tk_ReqWidth(tkwin);
            }
            if (h < 2) {
                h = Tk_ReqHeight(tkwin);
            }
            args.from.x1 = args.from.y1 = 0;
            args.width  = args.from.x2 = w;
            args.height = args.from.y2 = h;
            if (Blt_ParseSwitches(interp, snapSwitches, objc - 3, objv + 3, 
                                  &args, BLT_SWITCH_DEFAULTS) < 0) {
                return TCL_ERROR;
            }
            if (args.from.x2 > w) {
                args.from.x2 = w;
            }
            if (args.from.y2 > h) {
                args.from.y2 = h;
            }
            picture = Blt_GraphToPicture(interp, tkwin, imgPtr->gamma);
            if (picture == NULL) {
                Tcl_AppendResult(interp, "can't obtain snapshot of window \"", 
                                 Tcl_GetString(objv[2]), "\"", (char *)NULL);
                return TCL_ERROR;
            }
            if (Blt_SwitchChanged(snapSwitches, "-from", (char *)NULL)) {
                Blt_Picture newPict;
                
                newPict = Blt_CreatePicture(AREA_WIDTH(args.from),
                                            AREA_HEIGHT(args.from));
                Blt_CopyArea(newPict, picture, args.from.x1, args.from.y1, 
                             AREA_WIDTH(args.from), AREA_HEIGHT(args.from),
                             0, 0);
                Blt_FreePicture(picture);
                picture = newPict;
            }
        } else {
            int rootX, rootY;

            fprintf(stderr, "initialize %s w=%d h=%d\n", Tk_PathName(tkwin),
                    Tk_Width(tkwin), Tk_Height(tkwin));
            args.from.x1 = args.from.y1 = 0;
            args.width = args.from.x2 = Tk_Width(tkwin);
            args.height = args.from.y2 = Tk_Height(tkwin);
            if (Blt_ParseSwitches(interp, snapSwitches, objc - 3, objv + 3, 
                                  &args, BLT_SWITCH_DEFAULTS) < 0) {
                return TCL_ERROR;
            }
            if (args.from.x2 > Tk_Width(tkwin)) {
                args.from.x2 = Tk_Width(tkwin);
            }
            if (args.from.y2 > Tk_Height(tkwin)) {
                args.from.y2 = Tk_Height(tkwin);
            }
            args.width = AREA_WIDTH(args.from);
            args.height = AREA_HEIGHT(args.from);
            fprintf(stderr, "after parse w=%d h=%d\n", AREA_WIDTH(args.from),
                    AREA_HEIGHT(args.from));
            if (args.flags & RAISE) {
                XRaiseWindow(imgPtr->display, Tk_WindowId(tkwin));
            }
            Tk_GetRootCoords(tkwin, &rootX, &rootY);
            picture = Blt_DrawableToPicture(tkwin, Tk_RootWindow(tkwin),
                rootX + args.from.x1, rootY + args.from.y1,
                AREA_WIDTH(args.from), AREA_HEIGHT(args.from), imgPtr->gamma);
        }
    } else {
        Window window;
        int w, h;

        if (Blt_GetWindowFromObj(interp, objv[2], &window) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Blt_GetWindowExtents(imgPtr->display, window, NULL, NULL, &w, &h) 
            != TCL_OK) {
            Tcl_AppendResult(interp, "can't get dimensions of window \"", 
                Tcl_GetString(objv[2]), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        fprintf(stderr, "initialize w=%d h=%d\n", w, h);
        args.from.x1 = args.from.y1 = 0;
        args.width = args.from.x2 = w;
        args.height = args.from.y2 = h;
        if (Blt_ParseSwitches(interp, snapSwitches, objc - 3, objv + 3, 
                              &args, BLT_SWITCH_DEFAULTS) < 0) {
            return TCL_ERROR;
        }
        if (args.from.x2 > w) {
            args.from.x2 = w;
        }
        if (args.from.y2 > h) {
            args.from.y2 = h;
        }
        args.width = AREA_WIDTH(args.from);
        args.height = AREA_HEIGHT(args.from);
        fprintf(stderr, "after parse w=%d h=%d\n", AREA_WIDTH(args.from),
                AREA_HEIGHT(args.from));
        if (args.flags & RAISE) {
            XRaiseWindow(imgPtr->display, window);
        }
        picture = Blt_WindowToPicture(imgPtr->display, window, args.from.x1,
                args.from.y1, AREA_WIDTH(args.from), AREA_HEIGHT(args.from),
                imgPtr->gamma);
    }
    if (picture == NULL) {
        Tcl_AppendResult(interp, "can't obtain snapshot of window \"", 
                         Tcl_GetString(objv[2]), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    /* Now that we have the snapshot, resample the picture if needed.  */
    if ((args.flags | imgPtr->flags) & MAXPECT) {
        double xScale, yScale, s;
            
        xScale = (double)args.width  / (double)AREA_WIDTH(args.from);
        yScale = (double)args.height / (double)AREA_HEIGHT(args.from);
        s = MIN(xScale, yScale);
        args.width  = (int)(AREA_WIDTH(args.from) * s + 0.5);
        args.height = (int)(AREA_HEIGHT(args.from) * s + 0.5);
    }       
    if (args.vFilter == NULL) {
        args.vFilter = args.filter;
    }
    if (args.hFilter == NULL) {
        args.hFilter = args.filter;
    }
    if (args.hFilter == NULL) {
        args.hFilter = (AREA_WIDTH(args.from) < args.width) ?
            bltMitchellFilter : bltBoxFilter; 
    }
    if (args.vFilter == NULL) {
        args.vFilter = (AREA_HEIGHT(args.from) < args.height) ?
            bltMitchellFilter : bltBoxFilter;
    }
    if ((Blt_Picture_Width(picture) != args.width) ||
        (Blt_Picture_Height(picture) != args.height)) {
        Blt_Picture newPict;
        
        newPict = Blt_CreatePicture(args.width, args.height);
        Blt_ResamplePicture(newPict, picture, args.vFilter, args.hFilter);
        Blt_FreePicture(picture);
        picture = newPict;
    }
    if (picture == NULL) {
        Blt_FreeSwitches(snapSwitches, &args, 0);
        return TCL_ERROR;
    }
    ReplacePicture(imgPtr, picture);
    if (imgPtr->sourceName != NULL) {
        Blt_Free(imgPtr->sourceName);
        imgPtr->sourceName = NULL;
    }
    Blt_NotifyImageChanged(imgPtr);
    imgPtr->flags &= ~IMPORTED_MASK;
    Blt_FreeSwitches(snapSwitches, &args, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * WidthOp --
 *      Returns the current width of the picture.
 *
 *---------------------------------------------------------------------------
 */
static int
WidthOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int w;

    if (objc == 3) {
        int h;

        if (Tcl_GetIntFromObj(interp, objv[2], &w) != TCL_OK) {
            return TCL_ERROR;
        }
        if (w < 0) {
            Tcl_AppendResult(interp, "bad width \"", Tcl_GetString(objv[2]), 
                             "\"", (char *)NULL);
            return TCL_ERROR;
        }
        h = Blt_Picture_Height(imgPtr->picture);
        Blt_AdjustPictureSize(imgPtr->picture, w, h);
        Blt_NotifyImageChanged(imgPtr);
    } 
    w = Blt_Picture_Width(imgPtr->picture);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), w);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * WipeOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
WipeOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    Transition *transPtr;
    double position;
    int w, h;
    
    if (imgPtr->clientData != NULL) {
        FreeTransition(imgPtr);
    }
    transPtr = Blt_AssertCalloc(1, sizeof(Transition));
    transPtr->numSteps = 10;
    transPtr->fromColor.u32 = 0xFFFFFFFF;
    transPtr->toColor.u32 = 0xFF000000;
    transPtr->interp = interp;
    transPtr->imgPtr = imgPtr;
    transPtr->count = 1;
    transPtr->direction = TK_ANCHOR_E;
    transPtr->specs = wipeTransitionSwitches;
    imgPtr->clientData = transPtr;
    if (Blt_GetPictureFromObj(interp, objv[2], &transPtr->from) != TCL_OK) {
        goto error;
    }
    if (Blt_GetPictureFromObj(interp, objv[3], &transPtr->to) != TCL_OK) {
        goto error;
    }
    if (Blt_ParseSwitches(interp, transPtr->specs, objc - 4, objv + 4,
                transPtr, BLT_SWITCH_DEFAULTS) < 0) {
        goto error;
    }
    if (transPtr->from == imgPtr->picture) {
        Tcl_AppendResult(interp, "\"from\" picture can not be \"",
                         Tk_NameOfImage(imgPtr->imgToken),
                         "\"", (char *)NULL);
        goto error;
    }
    if (transPtr->to == imgPtr->picture) {
        Tcl_AppendResult(interp, "\"to\" picture can not be \"",
                         Tk_NameOfImage(imgPtr->imgToken),
                         "\"", (char *)NULL);
        goto error;
    }
    w = Blt_Picture_Width(transPtr->from);
    h = Blt_Picture_Height(transPtr->from);
    if ((w != Blt_Picture_Width(transPtr->to)) ||
        (h != Blt_Picture_Height(transPtr->to))) {
        Tcl_AppendResult(interp, "from and to picture ",
                         "must be the same size.", (char *)NULL);
        goto error;
    }
    transPtr->picture = Blt_CreatePicture(w, h);
    if (transPtr->count > transPtr->numSteps) {
        transPtr->count = transPtr->numSteps;
    } 
    position = (double)transPtr->count / (double)transPtr->numSteps;
    if (transPtr->logScale) {
        position = log10(9.0 * position + 1.0);
    }
    Blt_WipePictures(transPtr->picture, transPtr->from, transPtr->to,
                transPtr->direction, position);
    ReplacePicture(transPtr->imgPtr, transPtr->picture);
    Blt_NotifyImageChanged(imgPtr);
    if (transPtr->interval > 0) {
        transPtr->timerToken = Tcl_CreateTimerHandler(transPtr->interval, 
                WipeTimerProc, transPtr);
    } else {
        FreeTransition(imgPtr);
    }
    return TCL_OK;
 error:
    FreeTransition(imgPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * Picture instance sub-command specification:
 *
 *      - Name of the sub-command.
 *      - Minimum number of characters needed to unambiguously
 *        recognize the sub-command.
 *      - Pointer to the function to be called for the sub-command.
 *      - Minimum number of arguments accepted.
 *      - Maximum number of arguments accepted.
 *      - String to be displayed for usage (arguments only).
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec pictInstOps[] =
{
    {"add",       2, ArithOp,     3, 0, "pictOrColor ?switches ...?",},
    {"and",       3, ArithOp,     3, 0, "pictOrColor ?switches ...?",},
    {"blank",     3, BlankOp,     2, 3, "?colorSpec?",},
    {"blur",      3, BlurOp,      4, 4, "srcName width",},
    {"cget",      2, CgetOp,      3, 3, "option",},
    {"colorblend",3, ColorBlendOp,4, 0, "bgName fgName ?switches ...?",},
    {"composite", 4, CompositeOp, 4, 0, "bgName fgName ?switches ...?",},
    {"configure", 4, ConfigureOp, 2, 0, "?option value ...?",},
    {"convolve",  4, ConvolveOp,  3, 0, "srcName ?switches ...?",},
    {"copy",      3, CopyOp,      3, 0, "srcName ?switches ...?",},
    {"crop",      4, CropOp,      6, 6, "x1 y1 ?x2 y2?",},
    {"crossfade", 4, CrossFadeOp, 4, 0, "fromName toName ?switches ...?",},
    {"dissolve",  2, DissolveOp,  4, 0, "fromName toName ?switches ...?",},
    {"draw",      2, DrawOp,      2, 0, "?args ...?",},
    {"dup",       2, DupOp,       2, 0, "?switches ...?",},
    {"emboss",    2, EmbossOp,    3, 5, "srcName ?azimuth elevation?",},
    {"export",    2, ExportOp,    2, 0, "formatName ?switches ...?",},
    {"fade",      2, FadeOp,      4, 4, "srcName factor",},
    {"flip",      2, FlipOp,      3, 0, "x|y ?switches ...?",},
    {"gamma",     2, GammaOp,     3, 3, "value",},
    {"get",       2, GetOp,       4, 4, "x y",},
    {"greyscale", 3, GreyscaleOp, 3, 3, "srcName",},
    {"height",    1, HeightOp,    2, 3, "?newHeight?",},
    {"import",    2, ImportOp,    2, 0, "formatName ?switches ...?",},
    {"info",      2, InfoOp,      2, 2, "",},
    {"max",       2, ArithOp,     3, 0, "pictOrColor ?switches ...?",},
    {"min",       2, ArithOp,     3, 0, "pictOrColor ?switches ...?",},
    {"multiply",  2, MultiplyOp,  3, 3, "float",},
    {"nand",      2, ArithOp,     3, 0, "pictOrColor ?switches ...?",},
    {"nor",       2, ArithOp,     3, 0, "pictOrColor ?switches ...?",},
    {"or",        1, ArithOp,     3, 0, "pictOrColor ?switches ...?",},
    {"project",   2, ProjectOp,   5, 0, "srcName coords coords ?switches ...?",},
    {"put",       2, PutOp,       2, 0, "x y color",},
    {"quantize",  1, QuantizeOp,  4, 4, "srcName numColors",},
    {"reflect",   3, ReflectOp,   3, 0, "srcName ?switches ...?",},
    {"resample",  3, ResampleOp,  3, 0, "srcName ?switches ...?",},
    {"rotate",    2, RotateOp,    4, 4, "srcName angle",},
    {"select",    3, SelectOp,    4, 5, "srcName ?color ...?",},
    {"sequence",  3, SequenceOp,  2, 0, "?args ...?",},
    {"sharpen",   2, SharpenOp,   2, 0, "",},
    {"snap",      2, SnapOp,      3, 0, "windowName ?switches ...?",},
    {"subtract",  2, ArithOp,     3, 0, "pictOrColor ?switches ...?",},
    {"width",     3, WidthOp,     2, 3, "?newWidth?",},
    {"wipe",      3, WipeOp,      4, 0, "fromName toName ?switches ...?",},
    {"xor",       1, ArithOp,     3, 0, "pictOrColor ?switches ...?",},
    {"zresample", 1, ResampleOp2, 3, 0, "srcName ?switches ...?",},
};

static int numPictInstOps = sizeof(pictInstOps) / sizeof(Blt_OpSpec);

static int
PictureInstCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numPictInstOps, pictInstOps, BLT_OP_ARG1, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * PictureInstCmdDeleteProc --
 *
 *      This procedure is invoked when a picture command is deleted.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
PictureInstCmdDeletedProc(ClientData clientData) /* Pointer to record. */
{
    PictImage *imgPtr = clientData;

    imgPtr->cmdToken = NULL;
    if (imgPtr->imgToken != NULL) {
        Tk_DeleteImage(imgPtr->interp, Tk_NameOfImage(imgPtr->imgToken));
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * PictureLoadOp --
 *
 *      Loads the dynamic library representing the converters for the named
 *      format.  Designed to be called by "package require", not directly
 *      by the user.
 *      
 * Results:
 *      A standard TCL result.  Return TCL_OK is the converter was
 *      successfully loaded, TCL_ERROR otherwise.
 *
 * 
 *      blt::datatable load fmt libPath lib
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PictureLoadOp(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Tcl_DString lib;
    char *fmt;
    char *safeProcName, *initProcName;
    int result;
    int length;

    fmt = Tcl_GetStringFromObj(objv[2], &length);
    hPtr = Blt_FindHashEntry(&fmtTable, fmt);
    if (hPtr != NULL) {
        Blt_PictFormat *fmtPtr;

        fmtPtr = Blt_GetHashValue(hPtr);
        if (fmtPtr->flags & FMT_LOADED) {
            return TCL_OK;              /* Converter is already loaded. */
        }
    }
    Tcl_DStringInit(&lib);
    {
        Tcl_DString ds;
        const char *pathName;

        Tcl_DStringInit(&ds);
        pathName = Tcl_TranslateFileName(interp, Tcl_GetString(objv[3]), &ds);
        if (pathName == NULL) {
            Tcl_DStringFree(&ds);
            return TCL_ERROR;
        }
        Tcl_DStringAppend(&lib, pathName, -1);
        Tcl_DStringFree(&ds);
    }
    Tcl_DStringAppend(&lib, "/", -1);
    Tcl_UtfToTitle(fmt);
    Tcl_DStringAppend(&lib, "Pict", 4);
    Tcl_DStringAppend(&lib, fmt, -1);
    Tcl_DStringAppend(&lib, Blt_Itoa(BLT_MAJOR_VERSION), 1);
    Tcl_DStringAppend(&lib, Blt_Itoa(BLT_MINOR_VERSION), 1);
    Tcl_DStringAppend(&lib, BLT_LIB_SUFFIX, -1);
    Tcl_DStringAppend(&lib, BLT_SO_EXT, -1);

    initProcName = Blt_AssertMalloc(11 + length + 4 + 1);
    Blt_FmtString(initProcName, 11 + length + 4 + 1, "Blt_Picture%sInit", fmt);
    safeProcName = Blt_AssertMalloc(11 + length + 8 + 1);
    Blt_FmtString(safeProcName, 11 + length + 8 + 1, "Blt_Picture%sSafeInit", fmt);

    result = Blt_LoadLibrary(interp, Tcl_DStringValue(&lib), initProcName, 
        safeProcName); 
    Tcl_DStringFree(&lib);
    if (safeProcName != NULL) {
        Blt_Free(safeProcName);
    }
    if (initProcName != NULL) {
        Blt_Free(initProcName);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Picture instance sub-command specification:
 *
 *      - Name of the sub-command.
 *      - Minimum number of characters needed to unambiguously
 *        recognize the sub-command.
 *      - Pointer to the function to be called for the sub-command.
 *      - Minimum number of arguments accepted.
 *      - Maximum number of arguments accepted.
 *      - String to be displayed for usage (arguments only).
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec pictureOps[] =
{
    {"load",      1, PictureLoadOp, 4, 0, "fmt lib",},
#ifdef notdef
    {"blur",      2, BlurOp,        4, 0, "src dest ?switches?",},
    {"brighten"   2, BrightenOp,    4, 0, "src dest ?switches?",},
    {"darken"     1, BrightenOp,    4, 0, "src dest ?switches?",},
    {"medianf"    1, MedianOp,      4, 0, "src dest ?switches?",},
    {"translate", 1, TranslateOp,   4, 0, "src dest ?switches?",},
#endif
};
static int numPictureOps = sizeof(pictureOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * PictureImageCmdProc --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
PictureImageCmdProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    int objc,                           /* Not used. */
    Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numPictureOps, pictureOps, BLT_OP_ARG1, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_PictureCmdInitProc --
 *
 *      This procedure is invoked to initialize the "tree" command.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Creates the new command and adds a new entry into a global Tcl
 *      associative array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_PictureCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { 
        "picture", PictureImageCmdProc, 
    };
    /* cmdSpec.clientData = GetPictureCmdInterpData(interp); */
    if (Blt_InitCmd(interp, "::blt", &cmdSpec) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static Tk_ImageType pictureImageType = {
    (char *)"picture",          
    (Tk_ImageCreateProc *)CreateProc,   /* Known compiler warning */
    GetInstanceProc,            
    DisplayProc,        
    FreeInstanceProc,   
    DeleteProc, 
    PostScriptProc,     
    (Tk_ImageType *)NULL                /* nextPtr */
};

int
Blt_IsPicture(Tk_Image tkImage)
{
    const char *type;

    type = Blt_Image_NameOfType(tkImage);
    return (strcmp(type, pictureImageType.name) == 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_RegisterPictureImageType --
 *
 *      Registers the "picture" image type with Tk.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_RegisterPictureImageType(Tcl_Interp *interp) 
{
    Blt_PictFormat *fp, *fend;

    Tk_CreateImageType(&pictureImageType);

    Blt_CpuFeatureFlags(interp);

    Blt_InitHashTable(&fmtTable, BLT_STRING_KEYS);
    Blt_InitHashTable(&procTable, BLT_STRING_KEYS);
    for (fp = pictFormats, fend = fp + NUMFMTS; fp < fend; fp++) {
        Blt_HashEntry *hPtr;
        int isNew;

        hPtr = Blt_CreateHashEntry(&fmtTable, fp->name, &isNew);
        fp->flags |= FMT_STATIC;
        Blt_SetHashValue(hPtr, fp);
    }
}

int
Blt_PictureRegisterFormat(
    Tcl_Interp *interp,                           
    const char *fmt,
    Blt_PictureIsFmtProc  *isProc,
    Blt_PictureReadProc *readProc,
    Blt_PictureWriteProc *writeProc,
    Blt_PictureImportProc *importProc,
    Blt_PictureExportProc *exportProc)
{
    Blt_PictFormat *fmtPtr;
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&fmtTable, fmt);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "unknown format \"", fmt, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    fmtPtr = Blt_GetHashValue(hPtr);
    /* fmtPtr->name is already defined in the structure */
    fmtPtr->flags = FMT_LOADED;
    fmtPtr->isFmtProc = isProc;
    fmtPtr->readProc = readProc;
    fmtPtr->writeProc = writeProc;
    fmtPtr->importProc = importProc;
    fmtPtr->exportProc = exportProc;
    return TCL_OK;
}

int
Blt_PictureRegisterProc(Tcl_Interp *interp, const char *name,
                        Tcl_ObjCmdProc *proc)
{
    PictProc *procPtr;
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&procTable, name, &isNew);
    if (!isNew) {
        Tcl_AppendResult(interp, "picture procedure \"", name, 
                "\" is already registered.", (char *)NULL);
        return TCL_ERROR;
    }
    procPtr = Blt_Calloc(1, sizeof(PictProc));
    procPtr->hashPtr = hPtr;
    procPtr->name = Blt_GetHashKey(&procTable, hPtr);
    procPtr->proc = proc;
    Blt_SetHashValue(hPtr, procPtr);
    return TCL_OK;
}


/*
 * bltPictCmd.c --
 *
 * This module implements the Tk image interface for picture images.
 *
 *	Copyright 2003-2004 George A Howlett.
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
 *	Structure containing global data, used on a interpreter by interpreter
 *	basis.
 *
 *	This structure holds the hash table of instances of datatable commands
 *	associated with a particular interpreter.
 */
typedef struct {
    Tcl_Interp *interp;
} PictureCmdInterpData;


#define FMT_LOADED	(1<<0)
#define FMT_STATIC	(1<<1)
#define FMT_ASCII	(1<<2)

/*
 * Various external file/string formats handled by the picture image.
 */

enum PictureFormats {
#ifdef HAVE_LIBJPG
    FMT_JPG,				/* Joint Photographic Experts Group
					 * r/w */
#endif
#ifdef HAVE_LIBPNG
    FMT_PNG,				/* Portable Network Graphics r/w */
#endif
#ifdef HAVE_LIBTIF
    FMT_TIF,				/* Tagged Image File Format r/w */
#endif
#ifdef HAVE_LIBXPM
    FMT_XPM,				/* X Pixmap r/w */
#endif
    FMT_XBM,				/* X Bitmap r/w */
    FMT_GIF,				/* Graphics Interchange Format r/w */
    FMT_PS,				/* PostScript r/w */
    FMT_PDF,				/* Portable Document Format r/TBA */
    FMT_BMP,				/* Device-independent bitmap r/w */
    FMT_PBM,				/* Portable Bitmap Format r/w */
#ifdef WIN32
    FMT_EMF,				/* Enhanced Metafile Format r/w
					 * (Windows only) TBA */
    FMT_WMF,				/* Windows Metafile Format r/w
					 * (Windows only) TBA */
#endif
    FMT_TGA,				/* Targa Image File Format r/w */
    FMT_PHO,				/* Tk Photo Image r/w */
    FMT_ICO,				/* Window icon/cursor bitmap r/w */
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
    { "tif" },				/* Multi-page */
#endif
#ifdef HAVE_LIBXPM
    { "xpm" },
#endif
    { "xbm" },
    { "gif" },				/* Multi-page */
    { "ps"  },				/* Multi-page */
    { "pdf" },				/* Not implemented yet. */
    { "photo" },
    { "bmp" },
    { "pbm" },				/* Multi-page */
#ifdef WIN32
    { "emf" },
    { "wmf" },
#endif
    { "tga" },
    { "ico" },
};

static Blt_HashTable fmtTable;


typedef struct {
    const char *name;			/* Name of procedure package. */
    Blt_HashEntry *hashPtr;
    Tcl_ObjCmdProc *proc;
} PictProc;


static Blt_HashTable procTable;


/*
 * Default configuration options for picture images. 
 */
#define DEF_ANGLE		"0.0"
#define DEF_MAXPECT		"0"
#define DEF_AUTOSCALE		"0"
#define DEF_CACHE		"0"
#define DEF_DITHER		"0"
#define DEF_DATA		(char *)NULL
#define DEF_FILE		(char *)NULL
#define DEF_FILTER		(char *)NULL
#define DEF_GAMMA		"1.0"
#define DEF_HEIGHT		"0"
#define DEF_WIDTH		"0"
#define DEF_WINDOW		(char *)NULL
#define DEF_IMAGE		(char *)NULL
#define DEF_SHARPEN		"no"

#define DEF_OPAQUE		"0"
#define DEF_OPAQUE_BACKGROUND	"white"

#define IMPORTED_NONE		0
#define IMPORTED_FILE		(1<<0)
#define IMPORTED_IMAGE		(1<<1)
#define IMPORTED_WINDOW		(1<<2)
#define IMPORTED_DATA		(1<<3)
#define IMPORTED_MASK	\
	(IMPORTED_FILE|IMPORTED_IMAGE|IMPORTED_WINDOW|IMPORTED_DATA)

#define NOTIFY_PENDING		(1<<8)

#define MAXPECT			(1<<9)	/* Maintain the aspect ratio while
					 * scaling. The larger dimension is
					 * discarded. */
#define DITHER			(1<<10) /* Dither the picture before
					 * drawing. */
#define AUTOSCALE		(1<<11) /* Automatically scale the picture
					 * from a saved original picture when
					 * the size of the picture changes. */
#define SHARPEN			(1<<12) /* Sharpen the image. */

/*
 * PictImage -- 
 *
 *	A PictImage implements a Tk_ImageMaster for the "picture" image type.
 *	It represents a set of bits (i.e. the picture), some options, and
 *	operations (sub-commands) to manipulate the picture.
 *
 *	The PictImage manages the TCL interface to a picture (using Tk's
 *	"image" command).  Pictures and the mechanics of drawing the picture
 *	to the display (painters) are orthogonal.  The PictImage knows nothing
 *	about the display type (the display is stored only to free options
 *	when it's destroyed).  Information specific to the visual context
 *	(combination of display, visual, depth, colormap, and gamma) is stored
 *	in each cache entry.  The picture image manages the various picture
 *	transformations: reading, writing, scaling, rotation, etc.
 */
typedef struct _Blt_PictureImage {
    Tk_ImageMaster imgToken;		/* Tk's token for image master.  If
					 * NULL, the image has been
					 * deleted. */
    Tcl_Interp *interp;			/* Interpreter associated with the
					 * application using this image. */
    Display *display;			/* Display associated with this
					 * picture image.  This is used to
					 * free the configuration options. */
    Colormap colormap;
    Tcl_Command cmdToken;		/* Token for image command (used to
					 * delete the command when the image
					 * goes away).  NULL means the image
					 * command has already been
					 * deleted. */
    unsigned int flags;			/* Various bit-field flags defined
					 * below. */
    Blt_Chain chain;			/* List of pictures. (multi-page
					 * formats)  */
    Blt_Picture picture;		/* Current picture displayed. */
    
    /* User-requested options. */
    float angle;			/* Angle in degrees to rotate the
					 * image. */
    int reqWidth, reqHeight;		/* User-requested size of picture. The
					 * picture is scaled accordingly.
					 * These dimensions may or may not be
					 * used, depending upon the -maxpect
					 * option. */
    Blt_Picture original;
    Blt_ResampleFilter filter;		/* 1D Filter to use when the picture
					 * is resampled (resized). The same
					 * filter is applied both horizontally
					 * and vertically. */
    float gamma;			/* Gamma correction value of the
					 * monitor. In theory, the same
					 * picture image may be displayed on
					 * two monitors simultaneously (using
					 * xinerama).  Here we're assuming
					 * (almost certainly wrong) that both
					 * monitors will have the same gamma
					 * value. */
    const char *name;			/* Name of the image, file, or window
					 * read into the picture image. */
    int index;				/* Index of the picture in the above
					 * list. */
    Tcl_TimerToken timerToken;		/* Token for timer handler which polls
					 * for the exit status of each
					 * sub-process. If zero, there's no
					 * timer handler queued. */
    int interval;
    Blt_PictFormat *fmtPtr;		/* External format of last image read
					 * into the picture image. We use this
					 * to write back the same format if
					 * the user doesn't specify the
					 * format. */
    int doCache;			/* If non-zero, indicates to generate
					 * a pixmap of the picture. The pixmap
					 * is cached * in the table below. */
    Blt_HashTable cacheTable;		/* Table of cache entries specific to
					 * each visual context where this
					 * picture is displayed. */
} PictImage;


/*
 * PictCacheKey -- 
 *
 *	Represents the visual context of a cache entry. type.  It contains
 *	information specific to the visual context (combination of display,
 *	visual, depth, colormap, and gamma).  It is used as a hash table key
 *	for cache entries of picture images.  The same picture may be
 *	displayed in more than one visual context.
 */
typedef struct {
    Display *display;			/* Display where the picture will be
					 * drawn. Used to free colors
					 * allocated by the painter. */
    Visual *visualPtr;			/* Visual information of window
					 * displaying the image. */
    Colormap colormap;			/* Colormap used.  This may be the
					 * default colormap, or an allocated
					 * private map. */
    int depth;				/* Depth of the display. */
    unsigned int index;			/* Index of the picture in the
					 * list. */
    float gamma;			/* Gamma correction value */
} PictCacheKey;


/*
 * PictInstances -- 
 *
 *	PictInstances (image instances in the Tk parlance) represent a picture
 *	image in some specific combination of visual, display, colormap,
 *	depth, and output gamma.  Cache entries are stored by each picture
 *	image.
 *
 *	The purpose is to 
 *		1) allocate and hold the painter-specific to the isual and 
 *		2) provide caching of XImage's (drawn pictures) into pixmaps.  
 *
 *	The caching feature is enabled only for 100% opaque pictures.  If the
 *	picture must be blended with the current background, there is no
 *	guarantee (between redraws) that the background will not have changed.
 *	This feature is widget specific. There's no simple way to detect when
 *	the pixmap must be redrawn. In general, we should rely on the widget
 *	itself to perform its own caching of complex scenes.
 */
typedef struct {
    Blt_PictureImage image;		/* The picture image represented by
					 * this entry. */
    Blt_Painter painter;		/* The painter allocated for this
					 * particular combination of visual,
					 * display, colormap, depth, and
					 * gamma. */
    Display *display;			/* Used to free the pixmap below when
					 * the entry is destroyed. */
    Blt_HashEntry *hashPtr;		/* These two fields allow the cache */
    Blt_HashTable *tablePtr;		/* entry to be deleted from the
					 * picture image's table of
					 * entries. */
    Pixmap pixmap;			/* If non-NULL, is a cached pixmap of
					 * the picture. It's recreated each
					 * time the * picture changes. */
    int refCount;			/* This entry may be shared by all
					 * clients displaying this picture
					 * image with the same painter. */
    unsigned int flags;
} PictInstance;


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
    {BLT_CONFIG_BITMASK, "-autoscale", (char *)NULL, (char *)NULL, 
	DEF_AUTOSCALE, Blt_Offset(PictImage, flags), 
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)AUTOSCALE},
    {BLT_CONFIG_CUSTOM, "-data", (char *)NULL, (char *)NULL, DEF_DATA, 
	Blt_Offset(PictImage, picture), 0, &dataOption},
    {BLT_CONFIG_BITMASK, "-dither", (char *)NULL, (char *)NULL, 
	DEF_DITHER, Blt_Offset(PictImage, flags), 
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)DITHER},
    {BLT_CONFIG_CUSTOM, "-file", (char *)NULL, (char *)NULL, DEF_DATA, 
	Blt_Offset(PictImage, picture), 0, &fileOption},
    {BLT_CONFIG_CUSTOM, "-filter", (char *)NULL, (char *)NULL, 
	DEF_FILTER, Blt_Offset(PictImage, filter), 0, &bltFilterOption},
    {BLT_CONFIG_FLOAT, "-gamma", (char *)NULL, (char *)NULL, DEF_GAMMA,
	Blt_Offset(PictImage, gamma), BLT_CONFIG_DONT_SET_DEFAULT},
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

static Blt_SwitchParseProc BBoxSwitchProc;
static Blt_SwitchCustom bboxSwitch = {
    BBoxSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc FilterSwitchProc;
static Blt_SwitchCustom filterSwitch = {
    FilterSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc BlendingModeSwitchProc;
static Blt_SwitchCustom blendModeSwitch = {
    BlendingModeSwitchProc, NULL, NULL, (ClientData)0
};

typedef struct {
    int invert;				/* Flag. */
    Tcl_Obj *maskObjPtr;
} ArithSwitches;

static Blt_SwitchSpec arithSwitches[] = 
{
    {BLT_SWITCH_OBJ,     "-mask",   "mask", (char *)NULL,
	Blt_Offset(ArithSwitches, maskObjPtr), 0},
    {BLT_SWITCH_BOOLEAN, "-invert", "bool", (char *)NULL,
	Blt_Offset(ArithSwitches, invert), 0},
    {BLT_SWITCH_END}
};

typedef struct {
    PictRegion region;			/* Area to crop. */
    int nocopy;				/* If non-zero, don't copy the source
					 * image. */
} DupSwitches;

static Blt_SwitchSpec dupSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-region", "bbox", (char *)NULL,
	Blt_Offset(DupSwitches, region), 0, 0, &bboxSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    PictRegion from, to;
    Blt_BlendingMode mode;		/* Blending mode. */
} BlendSwitches;

static Blt_SwitchSpec blendSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-mode", "blendingmode", (char *)NULL,
	Blt_Offset(BlendSwitches, mode), 0, 0, &blendModeSwitch},
    {BLT_SWITCH_CUSTOM, "-from", "bbox", (char *)NULL,
	Blt_Offset(BlendSwitches,from), 0, 0, &bboxSwitch},
    {BLT_SWITCH_CUSTOM, "-to",   "bbox",  (char *)NULL,
	Blt_Offset(BlendSwitches, to),  0, 0, &bboxSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_ResampleFilter vFilter;		/* Color of rectangle. */
    Blt_ResampleFilter hFilter;		/* Width of outline. */
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
    PictRegion from, to;
    int blend;
} CopySwitches;

static Blt_SwitchSpec copySwitches[] = 
{
    {BLT_SWITCH_BOOLEAN,"-blend", "", (char *)NULL,
	Blt_Offset(CopySwitches, blend), 0, 0},
    {BLT_SWITCH_CUSTOM, "-from", "bbox", (char *)NULL,
	Blt_Offset(CopySwitches,from), 0, 0, &bboxSwitch},
    {BLT_SWITCH_CUSTOM, "-to",   "bbox", (char *)NULL,
	Blt_Offset(CopySwitches, to),  0, 0, &bboxSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    int blur;
    float size;
    PictFadeSettings fade;
    Blt_Pixel bgColor;
} ReflectSwitches;

static Blt_SwitchSpec reflectSwitches[] = {
    {BLT_SWITCH_SIDE, "-side", "side", (char *)NULL,
	Blt_Offset(ReflectSwitches, fade.side), 0, 0},
    {BLT_SWITCH_INT, "-blur", "level", (char *)NULL,
	Blt_Offset(ReflectSwitches, blur), 0, 0},
    {BLT_SWITCH_CUSTOM, "-bg", "color", (char *)NULL,
	Blt_Offset(ReflectSwitches, bgColor), 0, 0, &colorSwitch},
    {BLT_SWITCH_INT, "-low", "alpha", (char *)NULL,
	Blt_Offset(ReflectSwitches, fade.low), 0, 0},
    {BLT_SWITCH_INT, "-high", "alpha", (char *)NULL,
	Blt_Offset(ReflectSwitches, fade.high), 0, 0},
    {BLT_SWITCH_BOOLEAN, "-jitter", "bool", (char *)NULL,
	Blt_Offset(ReflectSwitches, fade.jitter), 0, 0},
    {BLT_SWITCH_BOOLEAN, "-logscale", "bool", (char *)NULL,
	Blt_Offset(ReflectSwitches, fade.logScale), 0, 0},
    {BLT_SWITCH_BOOLEAN, "-atanscale", "bool", (char *)NULL,
	Blt_Offset(ReflectSwitches, fade.atanScale), 0, 0},
    {BLT_SWITCH_FLOAT, "-ratio", "", (char *)NULL,
	Blt_Offset(ReflectSwitches, size), 0, 0},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_ResampleFilter filter;
    PictRegion region;
    int width, height;
    int flags;
} ResampleSwitches;

static Blt_SwitchSpec resampleSwitches[] = {
    {BLT_SWITCH_CUSTOM, "-filter", "filter", (char *)NULL,
	Blt_Offset(ResampleSwitches, filter), 0, 0, &filterSwitch},
    {BLT_SWITCH_CUSTOM, "-from",   "bbox", (char *)NULL,
	Blt_Offset(ResampleSwitches, region), 0, 0, &bboxSwitch},
    {BLT_SWITCH_INT,    "-height",  "int", (char *)NULL,
	Blt_Offset(ResampleSwitches, height),  0},
    {BLT_SWITCH_BITMASK, "-maxpect", "", (char *)NULL, 
	Blt_Offset(ResampleSwitches, flags), 0, MAXPECT},
    {BLT_SWITCH_INT,    "-width",   "int", (char *)NULL,
	Blt_Offset(ResampleSwitches, width),  0},
    {BLT_SWITCH_END}
};

typedef struct {
    PictRegion region;
    int raise;
} SnapSwitches;

static Blt_SwitchSpec snapSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-region", "bbox", (char *)NULL,
	Blt_Offset(SnapSwitches, region), 0, 0, &bboxSwitch},
    {BLT_SWITCH_BITMASK, "-raise",  "", (char *)NULL,
	Blt_Offset(SnapSwitches, raise),  0, TRUE},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_Pixel bg;			/* Fg and bg colors. */
} ProjectSwitches;

static Blt_SwitchSpec projectSwitches[] = {
    {BLT_SWITCH_CUSTOM, "-bg",      "color", (char *)NULL,
	Blt_Offset(ProjectSwitches, bg),             0, 0, &colorSwitch},
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
Jitter(Blt_Jitter *jitterPtr) 
{
    double t;

    t = RandomNumber(&jitterPtr->random);  /* Returns number 0..1 */
    return (t * jitterPtr->range) + jitterPtr->offset;
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

static Blt_Picture
PictureFromPictImage(PictImage *imgPtr)
{
    imgPtr->picture = Blt_GetNthPicture(imgPtr->chain, imgPtr->index);
    return imgPtr->picture;
}

static void
ReplacePicture(PictImage *imgPtr, Blt_Picture picture)
{
    Blt_ChainLink link;

    if (imgPtr->chain == NULL) {
	imgPtr->chain = Blt_Chain_Create();
    }
    link = Blt_Chain_GetNthLink(imgPtr->chain, imgPtr->index);
    if (link == NULL) {
	int n;

	n = Blt_Chain_GetLength(imgPtr->chain);
	link = Blt_Chain_Append(imgPtr->chain, picture);
	imgPtr->index = n;
    } else {
	Blt_Picture old;

	old = Blt_Chain_GetValue(link);
	Blt_FreePicture(old);
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
	Blt_FreePicture(picture);
    }
    Blt_Chain_Destroy(imgPtr->chain);
    imgPtr->chain = NULL;
    imgPtr->index = 0;
    imgPtr->picture = NULL;
}

Blt_Picture
Blt_GetPictureFromPictureImage(Tcl_Interp *interp, Tk_Image tkImage)
{
    PictInstance *instancePtr;

    if (!Blt_IsPicture(tkImage)) {
	Tcl_AppendResult(interp, "image is not a picture", (char *)NULL);
	return NULL;
    }
    instancePtr = Blt_Image_GetInstanceData(tkImage);
    return PictureFromPictImage(instancePtr->image);
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

	w = Blt_PictureWidth(imgPtr->picture);
	h = Blt_PictureHeight(imgPtr->picture);
	Tk_ImageChanged(imgPtr->imgToken, 0, 0, w, h, w, h);
    }
}

unsigned int
Blt_XColorToPixel(XColor *colorPtr)
{
    Blt_Pixel new;

    /* Convert X Color with 3 channel, 16-bit components to Blt_Pixel (8-bit,
     * with alpha component) 0..65356 0..255 */
    new.Red = colorPtr->red / 257;
    new.Green = colorPtr->green / 257;
    new.Blue = colorPtr->blue / 257;
    new.Alpha = ALPHA_OPAQUE;
    return new.u32;
}


int
Blt_GetBBoxFromObjv(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, 
		    PictRegion *regionPtr)
{
    double left, top, right, bottom;

    if ((objc != 2) && (objc != 4)) {
	Tcl_AppendResult(interp, "wrong # elements in bounding box ", 
		(char *)NULL);
	return TCL_ERROR;
    }
    regionPtr->x = regionPtr->y = regionPtr->w = regionPtr->h = 0;
    if ((Tcl_GetDoubleFromObj(interp, objv[0], &left) != TCL_OK) ||
	(Tcl_GetDoubleFromObj(interp, objv[1], &top) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (objc == 2) {
	regionPtr->x = ROUND(left), regionPtr->y = ROUND(top);
	return TCL_OK;
    }
    if ((Tcl_GetDoubleFromObj(interp, objv[2], &right) != TCL_OK) ||
	(Tcl_GetDoubleFromObj(interp, objv[3], &bottom) != TCL_OK)) {
	return TCL_ERROR;
    }

    /* Flip the coordinates of the bounding box if necessary so that its the
     * upper-left and lower-right corners */
    if (left > right) {
	double tmp;

	tmp = left, left = right, right = tmp;
    }
    if (top > bottom) {
	double tmp;

	tmp = top, top = bottom, bottom = tmp;
    }
    top = floor(top), left = floor(left);
    bottom = ceil(bottom), right = ceil(right);
    regionPtr->x = (int)left, regionPtr->y = (int)top;
    regionPtr->w = (int)right - regionPtr->x + 1;
    regionPtr->h = (int)bottom - regionPtr->y + 1;
    return TCL_OK;
}

static int
GetBBoxFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, PictRegion *regionPtr)
{
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    return Blt_GetBBoxFromObjv(interp, objc, objv, regionPtr);
}

int
Blt_AdjustRegionToPicture(Blt_Picture picture, PictRegion *regionPtr)
{
    int w, h;

    w = Blt_PictureWidth(picture);
    h = Blt_PictureHeight(picture);

    if ((regionPtr->w == 0) || (regionPtr->w > w)) {
	regionPtr->w = w;
    }
    if ((regionPtr->h == 0) || (regionPtr->h > h)) {
	regionPtr->h = h;
    }

    /* Verify that some part of the bounding box is actually inside the
     * picture. */
    if ((regionPtr->x >= w) || ((regionPtr->x + regionPtr->w) <= 0) ||
	(regionPtr->y >= h) || ((regionPtr->y + regionPtr->h) <= 0)) {
	return FALSE;
    }
    /* If needed, adjust the bounding box so that it resides totally inside the
     * picture */
    if (regionPtr->x < 0) {
	regionPtr->w += regionPtr->x;
	regionPtr->x = 0;
    } 
    if (regionPtr->y < 0) {
	regionPtr->h += regionPtr->y;
	regionPtr->y = 0;
    }
    if ((regionPtr->x + regionPtr->w) > w) {
	regionPtr->w = w - regionPtr->x;
    }
    if ((regionPtr->y + regionPtr->h) > h) {
	regionPtr->h = h - regionPtr->y;
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
    Tcl_Interp *interp,			/* Interpreter to load new format
					 * into. */
    const char *ext)			/* Extension of file name read in.
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
#ifdef notdef
    fprintf(stderr, "trying %s\n", fmtPtr->name);
#endif
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
	return NULL;			/* Could not load the format or the
					 * format doesn't have a discovery
					 * procedure. */
    }
    return fmtPtr;
}

static Blt_PictFormat *
QueryExternalFormat(
    Tcl_Interp *interp,			/* Interpreter to load new format
					 * into. */
    Blt_DBuffer dbuffer,		/* Data to be tested. */
    const char *ext)			/* Extension of file name read in.
					 * Will be NULL if read from
					 * data/base64 string. */
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    /* 
     * Step 1. Try to match the format to the extension, if there is
     *	       one.  We're trying to minimize the number of formats 
     *	       loaded using a blind query (.i.e. -file fileName).
     */
    if (ext != NULL) {
	hPtr = Blt_FindHashEntry(&fmtTable, ext);
	if (hPtr != NULL) {
	    Blt_PictFormat *fmtPtr;

	    fmtPtr = Blt_GetHashValue(hPtr);
#ifdef notdef
	    fprintf(stderr, "trying %s\n", fmtPtr->name);
#endif
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
		return NULL;		/* Could not load the format or the
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
	    /* If the image doesn't match, even though the extension matches,
	     * fall through and try all the other formats available. */
	}
    }
    /* 
     * Step 2. Try to match the image against all the previously 
     *	       loaded formats.
     */
    for (hPtr = Blt_FirstHashEntry(&fmtTable, &iter); hPtr != NULL; 
	 hPtr = Blt_NextHashEntry(&iter)) {
	Blt_PictFormat *fmtPtr;

	fmtPtr = Blt_GetHashValue(hPtr);
	if ((fmtPtr->flags & FMT_LOADED) == 0) {
	    continue;			/* Format isn't already loaded. */
	}
	if (fmtPtr->isFmtProc == NULL) {
	    continue;			/* No discover procedure.  */
	}
	if ((*fmtPtr->isFmtProc)(dbuffer)) {
	    return fmtPtr;
	}
    }
    /* 
     * Step 3. Try to match the image against any format not previously loaded.
     */
    for (hPtr = Blt_FirstHashEntry(&fmtTable, &iter); hPtr != NULL; 
	 hPtr = Blt_NextHashEntry(&iter)) {
	Blt_PictFormat *fmtPtr;

	fmtPtr = Blt_GetHashValue(hPtr);
	if (fmtPtr->flags & FMT_LOADED) {
	    continue;			/* Format is already loaded.  */
	}
	if (!LoadPackage(interp, fmtPtr->name)) {
	    continue;			/* Can't load format. */
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
	    return NULL;		/* Could not load the format or the
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
    Blt_Picture picture;
    Tk_PhotoHandle photo;

    photo = Tk_FindPhoto(interp, imageName);
    if (photo != NULL) {
	picture = Blt_PhotoToPicture(photo);
    } else if (Blt_GetPicture(interp, imageName, &picture) == TCL_OK) {
	picture = Blt_ClonePicture(picture);
    } else {
	return TCL_ERROR;
    }
    ReplacePicture(imgPtr, picture);
    if (imgPtr->name != NULL) {
	Blt_Free(imgPtr->name);
    }
    imgPtr->name = Blt_AssertStrdup(imageName);
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
    if (Blt_GetWindowRegion(imgPtr->display, window, (int *)NULL, (int *)NULL,
		&w, &h) != TCL_OK) {
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
    if (imgPtr->name != NULL) {
	Blt_Free(imgPtr->name);
    }
    imgPtr->name = Blt_AssertStrdup(Tcl_GetString(objPtr));
    imgPtr->flags &= ~IMPORTED_MASK;
    imgPtr->flags |= IMPORTED_WINDOW;
    return TCL_OK;
}

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

/*
 *---------------------------------------------------------------------------
 *
 * ObjToFile --
 *
 *	Given a file name, determine the image type and convert into a
 *	picture.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToFile(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,		        /* Interpreter to report results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Blt_Chain chain;
    Blt_DBuffer dbuffer;
    Blt_Picture *picturePtr = (Blt_Picture *)(widgRec + offset);
    PictImage *imgPtr = (PictImage *)widgRec;
    const char *fileName;
    Blt_PictFormat *fmtPtr;
    char *extPtr, ext[32];

    fileName = Tcl_GetString(objPtr);
    if (fileName[0] == '\0') {
	FreePictures(imgPtr);
	if (imgPtr->name != NULL) {
	    Blt_Free(imgPtr->name);
	    imgPtr->name = NULL;
	}
	imgPtr->fmtPtr = NULL;
	imgPtr->flags &= ~IMPORTED_MASK;
	return TCL_OK;
    }
    dbuffer = Blt_DBuffer_Create();
    if (Blt_DBuffer_LoadFile(interp, fileName, dbuffer) != TCL_OK) {
	goto error;
    }
    extPtr = NULL;
    if (fileName[0] != '@') {
	extPtr = strrchr(fileName, '.');
	if (extPtr != NULL) {
	    extPtr++;
	    if (*extPtr == '\0') {
		extPtr = NULL;
	    } 
	    strncpy(ext, extPtr, 31);
	    ext[31] = '\0';
	    Blt_LowerCase(ext);
	    extPtr = ext;
	}
    }
    fmtPtr = QueryExternalFormat(interp, dbuffer, extPtr);
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
    imgPtr->index = 0;
    imgPtr->picture = Blt_Chain_FirstValue(chain);
    if (imgPtr->name != NULL) {
	Blt_Free(imgPtr->name);
    }
    imgPtr->fmtPtr = fmtPtr;
    imgPtr->name = Blt_AssertStrdup(fileName);
    imgPtr->flags &= ~IMPORTED_MASK;
    imgPtr->flags |= IMPORTED_FILE;
    imgPtr->interval = 0;		/* 100 microseconds */
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
 *	Convert the picture into a TCL list of pixels.
 *
 * Results:
 *	The string representation of the picture is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
FileToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
{
    PictImage *imgPtr = (PictImage *)widgRec;

    if (((imgPtr->flags & IMPORTED_FILE) == 0) || (imgPtr->name == NULL)) {
	return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(imgPtr->name, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToFilter --
 *
 *	Given a string name, get the resample filter associated with it.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToFilter(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,		        /* Interpreter to report results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
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
 *	Convert the picture filter into a string Tcl_Obj.
 *
 * Results:
 *	The string representation of the filter is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
FilterToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Blt_ResampleFilter filter = *(Blt_ResampleFilter *)(widgRec + offset);

    if (filter == NULL) {
	return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(Blt_NameOfResampleFilter(filter), -1);
}

#ifdef notdef
static Tcl_ObjCmdProc *
GetPictProc(
    Tcl_Interp *interp,			/* Interpreter to load new format
					 * into. */
    const char *name)	
{
    Blt_HashEntry *hPtr;
    PictProc *procPtr;

    hPtr = Blt_FindHashEntry(&procTable, name);
    if (hPtr == NULL) {
	return NULL;
    }
    procPtr = Blt_GetHashValue(hPtr);
#ifdef notdef
    fprintf(stderr, "trying %s\n", procPtr->name);
#endif
    if (procPtr->proc == NULL) {
	LoadPackage(interp, name);
    }
    if (procPtr->proc == NULL) {
	Blt_Warn("can't load picture procedure %s\n", procPtr->name);
	return NULL;			/* Could not load the format or the
					 * format doesn't have a discovery
					 * procedure. */
    }
    return procPtr->proc;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * ObjToData --
 *
 *	Given a string of data or binary Tcl_Obj, determine the image
 *	type and convert into a picture.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToData(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to report results */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
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

	fmtPtr = NULL;			/* Suppress compiler warning. */
	chain = NULL;			/* Suppress compiler warning. */
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
	{
	    FILE *f;
	    
	    f = fopen("junk.unk", "w");
	    fwrite(Blt_DBuffer_Bytes(dbuffer), 1, Blt_DBuffer_Length(dbuffer), f);
	    fclose(f);
	}
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
	    imgPtr->chain = chain;
	    imgPtr->fmtPtr = fmtPtr;
	    imgPtr->picture = Blt_Chain_FirstValue(chain);
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
 *	Convert the picture into a TCL list of pixels.
 *
 * Results:
 *	The string representation of the picture is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
DataToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
{
    PictImage *imgPtr = (PictImage *)(widgRec);
    Blt_PictFormat *fmtPtr;

    if (((imgPtr->flags & IMPORTED_DATA) == 0) || (imgPtr->picture == NULL)) {
	return Tcl_NewStringObj("", -1);
    }
    fmtPtr = imgPtr->fmtPtr;
    if (fmtPtr == NULL) {
	Tcl_AppendResult(interp, "image \"", imgPtr->name, 
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
 * ObjToImage --
 *
 *	Convert a named image into a picture.
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
    Tcl_Interp *interp,			/* Interpreter to report results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
{
    PictImage *imgPtr = (PictImage *)(widgRec);

    return ImageToPicture(interp, imgPtr, Tcl_GetString(objPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageToObj --
 *
 *	Convert the named image into a picture.
 *
 * Results:
 *	The string representation of the picture is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ImageToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
{
    PictImage *imgPtr = (PictImage *)widgRec;

    if (((imgPtr->flags & IMPORTED_IMAGE) == 0) || (imgPtr->name == NULL)) {
	return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(imgPtr->name, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToWindow --
 *
 *	Given a file name, determine the image type and convert 
 *	into a picture.
 *
 * Results:
 *	The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToWindow(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,		        /* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representation of value. */
    char *widgRec,			/* Widget record. */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
{
    PictImage *imgPtr = (PictImage *)(widgRec);

    return WindowToPicture(interp, imgPtr, objPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowToObj --
 *
 *	Convert the picture into a TCL list of pixels.
 *
 * Results:
 *	The string representation of the picture is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
WindowToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
{
    PictImage *imgPtr = (PictImage *)widgRec;

    if (((imgPtr->flags & IMPORTED_WINDOW) == 0) || (imgPtr->name == NULL)) {
	return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(imgPtr->name, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * CacheKey --
 *
 *	Returns a key representing a specific visual context for a PictImage.
 *	Keys are used to create/find cache entries stored in the hash table of
 *	each PictImage.
 *
 * Results:
 *	A pointer to a static cache key.  
 *
 * Side effects:
 *	The key is overwritten on each call.  Care must be taken by the caller
 *	to save the key before making additional calls.
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
 *	This procedure is a callback for Tcl_EventuallyFree to release the
 *	resources and memory used by a PictInstance entry. The entry is
 *	destroyed only if noone else is currently using the entry (using
 *	reference counting).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The cache entry is possibly deallocated.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyCache(DestroyData data)
{
    PictInstance *cachePtr = (PictInstance *)data;
    
    if (cachePtr->refCount <= 0) {
	if (cachePtr->pixmap != None) {
	    Tk_FreePixmap(cachePtr->display, cachePtr->pixmap);
	}
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
 *	This procedure is called for each use of a picture image in a widget.
 *
 * Results:
 *	The return value is an entry for the visual context, which will be
 *	passed back to us on calls to DisplayProc and
 *	FreeInstanceProc.
 *
 * Side effects:
 *	A new entry is possibly allocated (or shared if one already exists).
 *
 *---------------------------------------------------------------------------
 */
static ClientData
GetInstanceProc(
    Tk_Window tkwin,			/* Window in which the picture will be
					 * displayed. */
    ClientData clientData)		/* Pointer to picture image for the
					 * image. */
{
    Blt_HashEntry *hPtr;
    PictCacheKey *keyPtr;
    PictImage *imgPtr = clientData;
    PictInstance *cachePtr;
    int isNew;

    keyPtr = CacheKey(tkwin, imgPtr->index);
    hPtr = Blt_CreateHashEntry(&imgPtr->cacheTable, (char *)keyPtr, &isNew);
    if (isNew) {
	cachePtr = Blt_Malloc(sizeof(PictInstance));	
	if (cachePtr == NULL) {
	    return NULL;		/* Can't allocate memory. */
	}
	cachePtr->painter = Blt_GetPainter(tkwin, imgPtr->gamma);
	cachePtr->image = imgPtr;
	cachePtr->display = Tk_Display(tkwin);
	cachePtr->pixmap = None;
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
 *	This procedure is called when a widget ceases to use a particular
 *	instance of a picture image.  We don't actually get rid of the entry
 *	until later because we may be about to re-get this instance again.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Internal data structures get freed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FreeInstanceProc(
    ClientData clientData,		/* Pointer to cache structure */
    Display *display)			/* Not used. */
{
    PictInstance *cachePtr = clientData;

    cachePtr->refCount--;
    if (cachePtr->refCount <= 0) {
	/* 
	 * Right now no one is using the entry. But delay the removal of the
	 * cache entry in case it's reused shortly afterwards.
	 */
 	Tcl_EventuallyFree(cachePtr, DestroyCache);
    }    
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteProc --
 *
 *	This procedure is called by the Tk image code to delete the master
 *	structure (PictureImage) for a picture image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources associated with the picture image are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
DeleteProc(
    ClientData clientData)		/* Pointer to picture image master
					 * structure for image.  Must not have
					 * any more instances. */
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
	cachePtr->hashPtr = NULL;	/* Flag for FreeInstanceProc. */
	DestroyCache((DestroyData)cachePtr);
    }
    imgPtr->imgToken = NULL;
    if (imgPtr->cmdToken != NULL) {
	Tcl_DeleteCommandFromToken(imgPtr->interp, imgPtr->cmdToken);
    }
    Blt_DeleteHashTable(&imgPtr->cacheTable);
    Blt_FreeOptions(configSpecs, (char *)imgPtr, imgPtr->display, 0);
    Blt_Free(imgPtr);
}

static int 
ConfigureImage(Tcl_Interp *interp, PictImage *imgPtr, int objc, 
	       Tcl_Obj *const *objv, int flags) 
{
    int w, h;

    if (Blt_ConfigureWidgetFromObj(interp, Tk_MainWindow(interp), configSpecs, 
	objc, objv, (char *)imgPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    imgPtr->picture = PictureFromPictImage(imgPtr);
    if (imgPtr->picture == NULL) {
	w = (imgPtr->reqWidth == 0) ? 16 : imgPtr->reqWidth;
	h = (imgPtr->reqHeight == 0) ? 16 : imgPtr->reqHeight;
	ReplacePicture(imgPtr, Blt_CreatePicture(w, h));
    }
    if (Blt_ConfigModified(configSpecs, "-autoscale", (char *)NULL)) {
	if (imgPtr->original != NULL) {
	    Blt_FreePicture(imgPtr->original);
	    imgPtr->original = NULL;
	}
	if ((imgPtr->flags & AUTOSCALE) && (imgPtr->picture != NULL)) {
	    imgPtr->original = Blt_ClonePicture(imgPtr->picture);
	}
    }
    if (imgPtr->angle != 0.0) {
	Blt_Picture rotate;

	/* Rotate the picture */
	rotate = Blt_RotatePicture(imgPtr->picture, imgPtr->angle);
	ReplacePicture(imgPtr, rotate);
    }

    w = (imgPtr->reqWidth == 0) ? 
	Blt_PictureWidth(imgPtr->picture) : imgPtr->reqWidth;
    h = (imgPtr->reqHeight == 0) ? 
	Blt_PictureHeight(imgPtr->picture) : imgPtr->reqHeight;

    if (imgPtr->flags & MAXPECT) {
	double sx, sy, scale;

	sx = (double)w / (double)Blt_PictureWidth(imgPtr->picture);
	sy = (double)h / (double)Blt_PictureHeight(imgPtr->picture);
	scale = MIN(sx, sy);
	w = (int)(Blt_PictureWidth(imgPtr->picture) * scale + 0.5);
	h = (int)(Blt_PictureHeight(imgPtr->picture) * scale + 0.5);
    }	    
    if ((Blt_PictureWidth(imgPtr->picture) != w) || 
	(Blt_PictureHeight(imgPtr->picture) != h)) {
	if (imgPtr->flags & AUTOSCALE) { 
	    Blt_Picture resize;

	    /* Scale the picture */
	    if (imgPtr->filter == NULL) {
		resize = Blt_ScalePicture(imgPtr->original, 0, 0,
			Blt_PictureWidth(imgPtr->original), 
			Blt_PictureHeight(imgPtr->original), w, h);
	    } else {
		resize = Blt_CreatePicture(w, h);
		Blt_ResamplePicture(resize, imgPtr->original, imgPtr->filter, 
			imgPtr->filter);
	    }	
	    ReplacePicture(imgPtr, resize);
	} else {
	    Blt_ResizePicture(imgPtr->picture, w, h);
	}
    }
    if (imgPtr->flags & SHARPEN) {
	Blt_SharpenPicture(imgPtr->picture, imgPtr->picture);
    }
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * CreateProc --
 *
 *	This procedure is called by the Tk image code to create a new picture
 *	image.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	The data structure for a new picture image is allocated and
 *	initialized.
 *
 *---------------------------------------------------------------------------
 */
static int
CreateProc(
    Tcl_Interp *interp,		        /* Interpreter to report errors back
					 * to. */
    const char *name,			/* Name to use for image command. */
    int objc,				/* # of option arguments. */
    Tcl_Obj *const *objv,		/* Option arguments (doesn't include
					 * image name or type). */
    const Tk_ImageType *typePtr,	/* Not used. */
    Tk_ImageMaster imgToken,		/* Token for image, to be used by us
					 * in later callbacks. */
    ClientData *clientDataPtr)		/* Store manager's token for image
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
#ifdef notdef
    imgPtr->typePtr = typePtr;
#endif
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
 *	This procedure is invoked to draw a picture image.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A portion of the image gets rendered in a pixmap or window.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayProc(
    ClientData clientData,		/* Pointer to token structure for the
					 * picture to be displayed. */
    Display *display,		        /* Display on which to draw
					 * picture. */
    Drawable drawable,			/* Pixmap or window in which to draw
					 * image. */
    int x, int y,			/* Upper-left corner of region within
					 * image to draw. */
    int w, int h,			/* Dimension of region within image to
					 * draw. */
    int dx, int dy)			/* Coordinates within destination
					 * drawable that correspond to imageX
					 * and imageY. */
{
    PictInstance *instPtr = clientData;
    PictImage *imgPtr;
    Blt_Picture picture;

    imgPtr = instPtr->image;
    
    picture = PictureFromPictImage(imgPtr);
    if (picture == NULL) {
	return;
    }
    if ((instPtr->pixmap != None) && (Blt_PictureIsDirty(picture))) {
	Tk_FreePixmap(display, instPtr->pixmap);
	instPtr->pixmap = None;
    }
    if ((imgPtr->doCache) && (Blt_PictureIsOpaque(picture))) {
	if (instPtr->pixmap == None) {
	    Pixmap pixmap;

	    /* Save the entire picture in the pixmap. */
	    pixmap = Blt_GetPixmap(display, drawable, 
		Blt_PictureWidth(picture), 
		Blt_PictureHeight(picture),
		Blt_PainterDepth(instPtr->painter));
	    Blt_PaintPicture(instPtr->painter, drawable, picture,
		0, 0, Blt_PictureWidth(picture), 
		Blt_PictureHeight(picture), 
		0, 0, imgPtr->flags);
	    instPtr->pixmap = pixmap;
	}
	/* Draw only the area that need to be repaired. */
	XCopyArea(display, instPtr->pixmap, drawable, 
		Blt_PainterGC(instPtr->painter), x, y, (unsigned int)w, 
		(unsigned int)h, dx, dy);
    } else {
	unsigned int flags;

	if (instPtr->pixmap != None) {
	    /* Kill the cached pixmap. */
	    Tk_FreePixmap(display, instPtr->pixmap);
	    instPtr->pixmap = None;
	}
	flags = 0;
	if ((imgPtr->flags & DITHER) || 
	    (Blt_PainterDepth(instPtr->painter) < 15)) {
	    flags |= BLT_PAINTER_DITHER;
	}
	Blt_PaintPicture(instPtr->painter, drawable, picture, 
		x, y, w, h, dx, dy, flags);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PostScriptProc --
 *
 *	This procedure is called to output the contents of a picture image in
 *	PostScript by calling the Tk_PostscriptPhoto function.
 *
 * Results:
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
PostScriptProc(
    ClientData clientData,		/* Master picture image. */
    Tcl_Interp *interp,			/* Interpreter to return generated
					 * PostScript. */
    Tk_Window tkwin,
    Tk_PostscriptInfo psInfo,		/* Not used.  Only useful for Tk
					 * internal Photo and Bitmap image
					 * types.  */
    int x, int y,			/* First pixel to output */
    int w, int h,			/* Width and height of picture area */
    int prepass)
{
    PictImage *imgPtr = clientData;

    if (prepass) {
	return TCL_OK;
    }
    if (Blt_PictureIsOpaque(imgPtr->picture)) {
	Blt_Ps ps;
	PageSetup setup;

	memset(&setup, 0, sizeof(setup));
	ps = Blt_Ps_Create(interp, &setup);
	Blt_Ps_DrawPicture(ps, imgPtr->picture, (double)x, (double)y);
	Blt_Ps_SetInterp(ps, interp);
	Blt_Ps_Free(ps);
    } else {
	Blt_HashEntry *hPtr;
	Blt_Picture bg;
	Blt_Ps ps;
	Drawable drawable;
	PageSetup setup;
	PictInstance *instPtr;
	PictCacheKey *keyPtr;

	instPtr = NULL;
	keyPtr = CacheKey(tkwin, imgPtr->index);
	hPtr = Blt_FindHashEntry(&imgPtr->cacheTable,(char *)keyPtr);
	if (hPtr != NULL) {
	    instPtr = Blt_GetHashValue(hPtr);
	}
	if ((instPtr != NULL) && (instPtr->pixmap != None)) {
	    drawable = instPtr->pixmap;
	} else {
	    drawable = Tk_WindowId(tkwin);
	}
	bg = Blt_DrawableToPicture(tkwin, drawable, x, y, w, h, imgPtr->gamma); 
	if (bg == NULL) {
	    return TCL_ERROR;
	}
	Blt_BlendPictures(bg, imgPtr->picture, 0, 0, w, h, 0, 0);

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
 * BBoxSwitch --
 *
 *	Convert a Tcl_Obj list of 2 or 4 numbers into representing a bounding
 *	box structure.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BBoxSwitchProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    PictRegion *regionPtr = (PictRegion *)(record + offset);

    if (GetBBoxFromObj(interp, objPtr, regionPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FilterSwitch --
 *
 *	Convert a Tcl_Obj representing a 1D image filter.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FilterSwitchProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Blt_ResampleFilter *filterPtr = (Blt_ResampleFilter *)(record + offset);

    return Blt_GetResampleFilterFromObj(interp, objPtr, filterPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * BlendingModeSwitch --
 *
 *	Convert a Tcl_Obj representing a blending mode.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BlendingModeSwitchProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Blt_BlendingMode *modePtr = (Blt_BlendingMode *)(record + offset);
    const char *string;
    int length;
    char c;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'n') && (strncmp(string, "normal", length) == 0)) {
	*modePtr = BLT_BLEND_NORMAL;
    } else if ((c == 'm') && (strncmp(string, "multiply", length) == 0)) {
	*modePtr = BLT_BLEND_MULTIPLY;
    } else if ((c == 's') && (strncmp(string, "screen", length) == 0)) {
	*modePtr = BLT_BLEND_SCREEN;
    } else if ((c == 'd') && (length > 1) && 
	       (strncmp(string, "darken", length) == 0)) {
	*modePtr = BLT_BLEND_DARKEN;
    } else if ((c == 'l') && (strncmp(string, "lighten", length) == 0)) {
	*modePtr = BLT_BLEND_LIGHTEN;
    } else if ((c == 'd') && (length > 1) && 
	       (strncmp(string, "difference", length) == 0)) {
	*modePtr = BLT_BLEND_DIFFERENCE;
    } else if ((c == 'h') && (strncmp(string, "hardlight", length) == 0)) {
	*modePtr = BLT_BLEND_HARDLIGHT;
    } else if ((c == 's') && (strncmp(string, "softlight", length) == 0)) {
	*modePtr = BLT_BLEND_SOFTLIGHT;
    } else if ((c == 'c') && (length > 5) && 
	       (strncmp(string, "colordodge", length) == 0)) {
	*modePtr = BLT_BLEND_COLORDODGE;
    } else if ((c == 'c') && (length > 5) && 
	       (strncmp(string, "colorburn", length) == 0)) {
	*modePtr = BLT_BLEND_COLORBURN;
    } else if ((c == 'o') && (strncmp(string, "overlay", length) == 0)) {
	*modePtr = BLT_BLEND_OVERLAY;
    } else {
	Tcl_AppendResult(interp, "unknown blending mode \"", string, "\": ",
		"should be normal, mulitply, screen, darken, lighten, ",
		"or difference", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

static Blt_Picture
NextImage(PictImage *imgPtr)
{
    Blt_Picture picture;

    imgPtr->index++;
    if (imgPtr->index >= Blt_Chain_GetLength(imgPtr->chain)) {
	imgPtr->index = 0;
    }
    picture = PictureFromPictImage(imgPtr);
    Blt_NotifyImageChanged(imgPtr);
    return picture;
}

static void
TimerProc(ClientData clientData)
{
    PictImage *imgPtr = clientData;
    int delay;

    NextImage(imgPtr);
    delay = Blt_PictureDelay(imgPtr->picture);
    if (imgPtr->interval > 0) {
	delay = imgPtr->interval;
    }
    imgPtr->timerToken = Tcl_CreateTimerHandler(delay, TimerProc, imgPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * ArithOp --
 *
 *	$image arith op $src -from { 0 0 100 100 } -to { 0 0 }
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ArithOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    int objc,				/* Not used. */
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
				      Blt_PictureWidth(src),
				      Blt_PictureHeight(src),
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
					      Blt_PictureWidth(src),
					      Blt_PictureHeight(src),
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
 *	Resets the picture at its current size to blank (by default 
 *	white, fully opaque) pixels.  
 *
 *		$image blank #000000 
 * Results:
 *	Returns a standard TCL return value. If an error occured parsing
 *	the pixel.
 *
 *
 * Side effects:
 *	A Tk_ImageChanged notification is triggered.
 *
 *---------------------------------------------------------------------------
 */
static int
BlankOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    PictImage *imgPtr = clientData;
    int w, h;
    Blt_Paintbrush brush, *brushPtr;

    if (objc == 3) {
	if (Blt_Paintbrush_Get(interp, objv[2], &brushPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	Blt_Paintbrush_Init(&brush);
	Blt_Paintbrush_SetColor(&brush, 0x00000000);
	brushPtr = &brush;
    }
    w = Blt_PictureWidth(imgPtr->picture);
    h = Blt_PictureHeight(imgPtr->picture);
    Blt_Paintbrush_Region(brushPtr, 0, 0, w, h);
    Blt_PaintRectangle(imgPtr->picture, 0, 0, w, h, 0, 0, brushPtr);
    if (brushPtr != &brush) {
	Blt_Paintbrush_Free(brushPtr);
    }
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BlendOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
BlendOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    BlendSwitches switches;
    Blt_Picture fg, bg, tmp;
    PictImage *imgPtr = clientData;
    Blt_Picture dest;

    if ((Blt_GetPictureFromObj(interp, objv[2], &bg) != TCL_OK) ||
	(Blt_GetPictureFromObj(interp, objv[3], &fg) != TCL_OK)) {
	return TCL_ERROR;
    }
    switches.mode = BLT_BLEND_NORMAL;
    switches.from.x = switches.from.y = 0;
    switches.from.w = Blt_PictureWidth(bg);
    switches.from.h = Blt_PictureHeight(bg);
    switches.to.x = switches.to.y = 0;
    switches.to.w = Blt_PictureWidth(bg);
    switches.to.h = Blt_PictureHeight(bg);
    if (Blt_ParseSwitches(interp, blendSwitches, objc - 4, objv + 4, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    dest = PictureFromPictImage(imgPtr);
    tmp = NULL;
    if (dest == fg) {
	fg = tmp = Blt_ClonePicture(fg);
    }
    if (dest != bg) {
	Blt_ResizePicture(dest, Blt_PictureWidth(bg), Blt_PictureHeight(bg));
	Blt_CopyPictureBits(dest, bg, 0, 0, 
			    Blt_PictureWidth(bg), Blt_PictureHeight(bg), 0, 0);
    }
    Blt_BlendPictures(dest, fg, 
		      switches.from.x, switches.from.y, 
		      switches.from.w, switches.from.h, 
		      switches.to.x, switches.to.y);
    if (tmp != NULL) {
	Blt_FreePicture(tmp);
    }
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blend2Op --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
Blend2Op(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    Blt_Picture fg, bg, dest;
    BlendSwitches switches;
    PictImage *imgPtr = clientData;

    if ((Blt_GetPictureFromObj(interp, objv[2], &bg) != TCL_OK) ||
	(Blt_GetPictureFromObj(interp, objv[3], &fg) != TCL_OK)) {
	return TCL_ERROR;
    }
    switches.mode = BLT_BLEND_NORMAL;
    if (Blt_ParseSwitches(interp, blendSwitches, objc - 4, objv + 4, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    dest = PictureFromPictImage(imgPtr);
    Blt_ResizePicture(dest, Blt_PictureWidth(bg), Blt_PictureHeight(bg));
    Blt_CopyPictureBits(dest, bg, 0, 0, Blt_PictureWidth(bg), 
	Blt_PictureHeight(bg), 0, 0);
    Blt_BlendPicturesByMode(dest, fg, switches.mode); 
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
BlurOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    Blt_Picture src, dest;
    PictImage *imgPtr;
    int r;				/* Radius of the blur. */

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &r) != TCL_OK) {
	return TCL_ERROR;
    }
    if (r < 1) {
	Tcl_AppendResult(interp, "radius of blur must be > 1 pixel wide",
			 (char *)NULL);
	return TCL_ERROR;
    }
    imgPtr = clientData;
    dest = PictureFromPictImage(imgPtr);
    Blt_BlurPicture(dest, src, r, 3);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *	Returns the value of the configuration option specified.
 *
 * Results:
 *	Returns a standard TCL return value.  If TCL_OK, the value of the
 *	picture configuration option specified is returned in the interpreter
 *	result.  Otherwise an error message is returned.
 * 
 *---------------------------------------------------------------------------
 */
static int
CgetOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)		/* Argument objects. */
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
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
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
ConvolveOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    int objc,				/* Not used. */
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
    dest = PictureFromPictImage(imgPtr);
    Blt_ConvolvePicture(dest, src, switches.vFilter, switches.hFilter);
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
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
CopyOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    Blt_Picture src, dest;
    CopySwitches switches;
    PictImage *imgPtr;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
	return TCL_ERROR;
    }
    switches.from.x = switches.from.y = 0;
    switches.from.w = Blt_PictureWidth(src);
    switches.from.h = Blt_PictureHeight(src);
    switches.to.x = switches.to.y = 0;
    switches.to.w = Blt_PictureWidth(src);
    switches.to.h = Blt_PictureHeight(src);
    switches.blend = FALSE;

    if (Blt_ParseSwitches(interp, copySwitches, objc - 3, objv + 3, &switches, 
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (!Blt_AdjustRegionToPicture(src, &switches.from)) {
	return TCL_OK;			/* Region is not inside of source. */
    }
    imgPtr = clientData;
    dest = PictureFromPictImage(imgPtr);
    if (!Blt_AdjustRegionToPicture(dest, &switches.to)) {
	return TCL_OK;			/* Region is not inside of
					 * destination. */
    }
    if (switches.blend) {
	Blt_BlendPictures(dest, src, switches.from.x, 
		switches.from.y, switches.from.w, switches.from.h,
		switches.to.x, switches.to.y);
    } else {
	Blt_CopyPictureBits(dest, src, switches.from.x, 
		switches.from.y, switches.from.w, switches.from.h,
		switches.to.x, switches.to.y);
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
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
CropOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    PictRegion region;
    Blt_Picture src, dest;
    PictImage *imgPtr;

    if (Blt_GetBBoxFromObjv(interp, objc - 2, objv + 2, &region) != TCL_OK) {
	return TCL_ERROR;
    }
    imgPtr = clientData;
    src = PictureFromPictImage(imgPtr);
    if (!Blt_AdjustRegionToPicture(src, &region)) {
	Tcl_AppendResult(interp, "impossible coordinates for region", 
			 (char *)NULL);
	return TCL_ERROR;
    }
    dest = Blt_CreatePicture(region.w, region.h);
    Blt_CopyPictureBits(dest, src, region.x, region.y, region.w, region.h, 0,0);
    ReplacePicture(imgPtr, dest);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

static Blt_OpSpec drawOps[] = {
    {"circle",    1, Blt_Picture_CircleOp,    4, 0, "x y r ?switches?",},
    {"ellipse",   1, Blt_Picture_EllipseOp,   5, 0, "x y a b ?switches?",},
    {"line",      1, Blt_Picture_LineOp,      3, 0, "?switches?",},
    {"polygon",   1, Blt_Picture_PolygonOp,   3, 0, "?switches?",},
    {"rectangle", 1, Blt_Picture_RectangleOp, 7, 0, "x1 y1 x2 y2 ?switches?",},
    {"text",      1, Blt_Picture_TextOp,      6, 0, "string x y ?switches?",},
};
static int numDrawOps = sizeof(drawOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * DrawOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
DrawOp(
    ClientData clientData,		/* Contains reference to picture
					 * object. */
    Tcl_Interp *interp,
    int objc,				/* Not used. */
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
    picture = PictureFromPictImage(imgPtr);
    result = (*proc) (picture, interp, objc, objv);
    if (result == TCL_OK) {
	Blt_NotifyImageChanged(imgPtr);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * DupOp --
 *
 *	Creates a duplicate of the current picture in a new picture image.
 *	The name of the new image is returned.
 *
 * Results:
 *	Returns a standard TCL return value.  If TCL_OK, The name of the new
 *	image command is returned via the interpreter result.  Otherwise an
 *	error message is returned.
 *
 * Side effects:
 *	A new image command is created.
 *
 *---------------------------------------------------------------------------
 */
static int
DupOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    Blt_Picture picture;
    DupSwitches switches;
    PictImage *imgPtr = clientData;
    Tcl_Obj *objPtr;

    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, dupSwitches, objc - 2, objv + 2, &switches, 
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (!Blt_AdjustRegionToPicture(imgPtr->picture, &switches.region)) {
	Tcl_AppendResult(interp, "impossible coordinates for region", 
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
    
    picture = Blt_CreatePicture(switches.region.w, switches.region.h);
    if (switches.nocopy) {		/* Set the picture to a blank image. */
	Blt_BlankPicture(picture, 0x0);
    } else {				/* Copy region into new picture. */
	Blt_CopyPictureBits(picture, imgPtr->picture, switches.region.x,
	    switches.region.y, switches.region.w, switches.region.h, 0, 0);
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
 * ExportOp --
 *
 * Results:
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
ExportOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
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
	Tcl_AppendResult(interp, "no export procedure registered for \"", 
			 fmtPtr->name, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    result = (*fmtPtr->exportProc)(interp, imgPtr->index, imgPtr->chain, 
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
FadeOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    Blt_Picture src;
    PictImage *imgPtr = clientData;
    int w, h, alpha;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &alpha) != TCL_OK) {
	return TCL_ERROR;
    }
    w = Blt_PictureWidth(src);
    h = Blt_PictureHeight(src);
    if (imgPtr->picture != src) {
	Blt_ResizePicture(imgPtr->picture, w, h);
	Blt_CopyPictureBits(imgPtr->picture, src, 0, 0, w, h, 0, 0);
    }
    Blt_FadePicture(imgPtr->picture, 0, 0, w, h, alpha);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FlipOp --
 *
 *	Flips the picture either horizontally or vertically.
 *
 * Results:
 *	Returns a standard TCL return value.  If TCL_OK, the components of the
 *	pixel are returned as a list in the interpreter result.  Otherwise an
 *	error message is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
FlipOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)		/* Argument objects. */
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
    Blt_FlipPicture(imgPtr->picture, isVertical);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GammaOp --
 *
 *	$image gamma value
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
 *	Returns the RGBA components of the pixel at the specified coordinate.
 *
 * Results:
 *	Returns a standard TCL return value.  If TCL_OK, the components of the
 *	pixel are returned as a list in the interpreter result.  Otherwise an
 *	error message is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
GetOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    PictImage *imgPtr = clientData;
    Blt_Pixel *sp, pixel;
    int x, y;

    if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    if ((x < 0) || (x >= Blt_PictureWidth(imgPtr->picture))) {
	Tcl_AppendResult(interp, "x-coordinate \"", Tcl_GetString(objv[2]),
		"\" is out of range", (char *)NULL);
	return TCL_ERROR;
    }
    if ((y < 0) || (y >= Blt_PictureHeight(imgPtr->picture))) {
	Tcl_AppendResult(interp, "y-coordinate \"", Tcl_GetString(objv[3]),
		"\" is out of range", (char *)NULL);
	return TCL_ERROR;
    }
    sp = Blt_PicturePixel(imgPtr->picture, x, y);
    pixel = *sp;
    if ((Blt_PictureFlags(imgPtr->picture) & BLT_PIC_ASSOCIATED_COLORS) &&
	((sp->Alpha != 0xFF) && (sp->Alpha != 0x00))) {
	int bias = sp->Alpha >> 1;

	pixel.Red   = (mul255(sp->Red)   + bias) / sp->Alpha;
	pixel.Green = (mul255(sp->Green) + bias) / sp->Alpha;
	pixel.Blue  = (mul255(sp->Blue)  + bias) / sp->Alpha;
    }
    {
	char string[11];

	Blt_FormatString(string, 11, "0x%02x%02x%02x%02x", pixel.Alpha, pixel.Red, 
		 pixel.Green, pixel.Blue);
	Tcl_SetObjResult(interp, Tcl_NewStringObj(string, 10));
    }
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
GreyscaleOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    Blt_Picture src, dest;
    PictImage *imgPtr = clientData;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
	return TCL_ERROR;
    }
    dest = Blt_GreyscalePicture(src);
    ReplacePicture(imgPtr, dest);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * HeightOp --
 *
 *	Returns the current height of the picture.
 *
 *---------------------------------------------------------------------------
 */
static int
HeightOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* # of arguments. */
    Tcl_Obj *const *objv)		/* Argument vector. */
{
    PictImage *imgPtr = clientData;
    int h;

    h = 0;
    if (objc == 3) {
	int w;

	if (Tcl_GetIntFromObj(interp, objv[2], &h) != TCL_OK) {
	    return TCL_ERROR;
	}
	w = Blt_PictureWidth(imgPtr->picture);
	Blt_AdjustPictureSize(imgPtr->picture, w, h);
	Blt_NotifyImageChanged(imgPtr);
    } 
    if (imgPtr->picture != NULL) {
	h = Blt_PictureHeight(imgPtr->picture);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), h);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImportOp --
 *
 *	Imports an image source into a picture.  The image source can be a
 *	file, base64 string, or binary Tcl_Obj.  This performs basically the
 *	same function as "configure".  Any extra functionality this command
 *	has is based upon the ability to pass extra flags to the various
 *	converters, something that can't be done really be done with
 *
 *		$image configure -file file.jpg
 *
 * Results:
 *	Returns a standard TCL return value.  If no error, the interpreter
 *	result will contain the number of images successfully read.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
ImportOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    Blt_Chain chain;
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
	Tcl_AppendResult(interp, "no import procedure registered for \"", 
		fmtPtr->name, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    chain = (*fmtPtr->importProc)(interp, objc, objv, &fileName);
    if (chain == NULL) {
	return TCL_ERROR;
    }
    FreePictures(imgPtr);
    imgPtr->chain = chain;
    imgPtr->index = 0;
    imgPtr->picture = Blt_Chain_FirstValue(chain);

    /* 
     * Save the format type and file name in the image record.  The file name
     * is used when querying image options -file or -data via configure.  The
     * type is used by the "-data" operation to establish a default format to
     * output.
     */
    imgPtr->fmtPtr = fmtPtr;
    imgPtr->flags &= ~IMPORTED_MASK;
    if (imgPtr->name != NULL) {
	Blt_Free(imgPtr->name);
    }
    if (fileName == NULL) {
	imgPtr->name = NULL;
	imgPtr->flags |= IMPORTED_DATA;
    } else {
	imgPtr->name = Blt_AssertStrdup(fileName);
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
 *	Reports the basic information about a picture.  
 *
 * Results:
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
InfoOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)		/* Not used. */
{
    PictImage *imgPtr = clientData;
    int numColors;
    const char *string;
    Tcl_Obj *listObjPtr, *objPtr;

    numColors = Blt_QueryColors(imgPtr->picture, (Blt_HashTable *)NULL);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);

    objPtr = Tcl_NewStringObj("colors", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(numColors);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("type", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    string = Blt_PictureIsColor(imgPtr->picture) ? "color" : "greyscale";
    objPtr = Tcl_NewStringObj(string, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("opacity", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    if (Blt_PictureIsBlended(imgPtr->picture)) {
	string = "blended";
    } else if (Blt_PictureIsMasked(imgPtr->picture)) {
	string = "masked";
    } else {
	string = "full";
    }
    objPtr = Tcl_NewStringObj(string, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("width", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(Blt_PictureWidth(imgPtr->picture));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    
    objPtr = Tcl_NewStringObj("height", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(Blt_PictureHeight(imgPtr->picture));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("count", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(Blt_Chain_GetLength(imgPtr->chain));
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("index", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(imgPtr->index);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    objPtr = Tcl_NewStringObj("read-format", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewStringObj((imgPtr->fmtPtr != NULL) ? 
	imgPtr->fmtPtr->name : "???", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MultiplyOp --
 *
 *	$image multiply scalar
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
MultiplyOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    double scalar;

    if (Tcl_GetDoubleFromObj(interp, objv[2], &scalar) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_MultiplyPixels(imgPtr->picture, (float)scalar);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}


static int
GetImageIndex(Tcl_Interp *interp, PictImage *imgPtr, Tcl_Obj *objPtr, 
	      int *indexPtr)
{
    int index;
    const char *string;

    string = Tcl_GetString(objPtr);
    if (strcmp(string, "end") == 0) {
	index = Blt_Chain_GetLength(imgPtr->chain) - 1;
    } else if (Tcl_GetIntFromObj(interp, objPtr, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((index < 0) || (index >= Blt_Chain_GetLength(imgPtr->chain))) {
	Tcl_AppendResult(interp, "invalid image index \"", 
			 Tcl_GetString(objPtr), "\"", (char *)NULL);
	return TCL_ERROR;
    }
    *indexPtr = index;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListAnimateOp --
 *
 *	$im list animate $delay
 *	$im list animate stop
 *	$im list animate start
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ListAnimateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    const char *string;
    int interval;
    
    string = Tcl_GetString(objv[2]);
    if (strcmp(string, "stop") == 0) {
	if (imgPtr->timerToken != 0) {
	    Tcl_DeleteTimerHandler(imgPtr->timerToken);
	    imgPtr->timerToken = 0;
	}
    } else if (strcmp(string, "start") == 0) {
	if (imgPtr->timerToken == 0) {
	    int delay;

	    delay = Blt_PictureDelay(imgPtr->picture);
	    imgPtr->timerToken = 
		Tcl_CreateTimerHandler(delay, TimerProc, imgPtr);
	}	
    } else if (Tcl_GetIntFromObj(interp, objv[2], &interval) == TCL_OK) {
	imgPtr->interval = interval;
	if (imgPtr->timerToken != 0) {
	    Tcl_DeleteTimerHandler(imgPtr->timerToken);
	}
	imgPtr->timerToken = Tcl_CreateTimerHandler(imgPtr->interval, 
		TimerProc, imgPtr);
    } else {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListAppendOp --
 *
 *	$im list append $img...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ListAppendOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int i;

    for (i = 3; i < objc; i++) {
	Blt_Picture src, dest;

	if (Blt_GetPictureFromObj(interp, objv[i], &src) != TCL_OK) {
	    return TCL_ERROR;
	}
	dest = Blt_ClonePicture(src);
	Blt_Chain_Append(imgPtr->chain, dest);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ListCurrentOp --
 *
 *	$im list current index 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ListCurrentOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;

    if (objc == 4) {
	int index;

	if (GetImageIndex(interp, imgPtr, objv[3], &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	imgPtr->picture = Blt_GetNthPicture(imgPtr->chain, index);
	imgPtr->index = index;
	Blt_NotifyImageChanged(imgPtr);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), imgPtr->index);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListDeleteOp --
 *
 *	$im list delete first last
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ListDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int first;

    if (GetImageIndex(interp, imgPtr, objv[3], &first) != TCL_OK) {
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
	Blt_FreePicture(picture);
	Blt_Chain_DeleteLink(imgPtr->chain, link);
    } else {
	int i;
	int last;
	Blt_ChainLink link, next;

	if (GetImageIndex(interp, imgPtr, objv[4], &last) != TCL_OK) {
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
		Blt_FreePicture(picture);
		Blt_Chain_DeleteLink(imgPtr->chain, link);
	    }
	}
    }
    imgPtr->index = 0;
    imgPtr->picture = Blt_Chain_FirstValue(imgPtr->chain);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ListLengthOp --
 *
 *	$im list length
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ListLengthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int count;

    count = Blt_Chain_GetLength(imgPtr->chain);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), count);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListNextOp --
 *
 *	$im list next
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ListNextOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    
    NextImage(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListPreviousOp --
 *
 *	$im list previous
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ListPreviousOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;

    imgPtr->index--;
    if (imgPtr->index < 0) {
	imgPtr->index = Blt_Chain_GetLength(imgPtr->chain) - 1;
    }
    if (imgPtr->index < 0) {
	imgPtr->index = 0;
    }
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListReplaceOp --
 *
 *	$im list replace first last $img...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ListReplaceOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    PictImage *imgPtr = clientData;
    int i;
    int first, last;
    Blt_ChainLink link, next, head, tail;

    if (GetImageIndex(interp, imgPtr, objv[3], &first) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetImageIndex(interp, imgPtr, objv[4], &last) != TCL_OK) {
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
	    Blt_FreePicture(picture);
	    Blt_Chain_DeleteLink(imgPtr->chain, link);
	} else if (head == NULL) {
	    head = link;
	} else if (tail == NULL) {
	    tail = link;
	}
    }
    if (head != NULL) {
	for (i = 5; i < objc; i++) {
	    Blt_Picture src, dest;

	    if (Blt_GetPictureFromObj(interp, objv[i], &src) != TCL_OK) {
		return TCL_ERROR;
	    }
	    dest = Blt_ClonePicture(src);
	    link = Blt_Chain_NewLink();
	    Blt_Chain_SetValue(link, dest);
	    Blt_Chain_LinkAfter(imgPtr->chain, link, head);
	    head = link;
	}
    } else if (tail != NULL) {
	for (i = 5; i < objc; i++) {
	    Blt_Picture src, dest;

	    if (Blt_GetPictureFromObj(interp, objv[i], &src) != TCL_OK) {
		return TCL_ERROR;
	    }
	    dest = Blt_ClonePicture(src);
	    link = Blt_Chain_NewLink();
	    Blt_Chain_SetValue(link, dest);
	    Blt_Chain_LinkBefore(imgPtr->chain, link, tail);
	}
    } else {
	assert(Blt_Chain_GetLength(imgPtr->chain) == 0);
	for (i = 5; i < objc; i++) {
	    Blt_Picture src, dest;

	    if (Blt_GetPictureFromObj(interp, objv[i], &src) != TCL_OK) {
		return TCL_ERROR;
	    }
	    dest = Blt_ClonePicture(src);
	    Blt_Chain_Append(imgPtr->chain, dest);
	}
    }	
    imgPtr->index = 0;
    imgPtr->picture = Blt_Chain_FirstValue(imgPtr->chain);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

static Blt_OpSpec listOps[] =
{
    {"animate",   2, ListAnimateOp,   3, 3, "oper",},
    {"append",    2, ListAppendOp,    3, 0, "?image...?",},
    {"current",   1, ListCurrentOp,   3, 4, "?index?",},
    {"delete",    1, ListDeleteOp,    4, 5, "first ?last?",},
    {"length",    1, ListLengthOp,    3, 3, "",},
    {"next",      1, ListNextOp,      3, 3, "",},
    {"previous",  1, ListPreviousOp,  3, 3, "",},
    {"replace",   1, ListReplaceOp,   5, 0, "first last ?image...?",},
};

static int numListOps = sizeof(listOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * ListOp --
 *
 *	$img list animate 
 *	$img list append image image image
 *	$img list current ?index?
 *	$img list delete 0 end 
 *	$img list length
 *	$img list next
 *	$img list previous
 *	$img list replace 0 end image image image image
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
ListOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    PictCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numListOps, listOps, BLT_OP_ARG2, 
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
ProjectOp(
    ClientData clientData,	/* Information about picture cmd. */
    Tcl_Interp *interp,		/* Interpreter to report errors back to. */
    int objc,			/* Not used. */
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
 * Results:
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
PutOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * QuantizeOp --
 *
 *	$dest quantize $src 256
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
QuantizeOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    int objc,				/* Not used. */
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
ReflectOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Interpreter to report errors. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    Pict *srcPtr, *destPtr, *tmpPtr;
    PictImage *imgPtr = clientData;
    ReflectSwitches switches;
    int w, h;
    int dw, dh;
    int r;

    r = 1;
    if (Blt_GetPictureFromObj(interp, objv[2], &srcPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    switches.fade.side = SIDE_BOTTOM;
    switches.fade.jitter.range = 0.1;
    switches.fade.logScale = FALSE;
    switches.fade.atanScale = FALSE;
    switches.fade.low = 0;
    switches.fade.high = 0xFF;
    switches.blur = 1;
    switches.bgColor.u32 = 0x0;

    if (Blt_ParseSwitches(interp, reflectSwitches, objc - 3, objv + 3, 
		&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    w = srcPtr->width, h = srcPtr->height;
    dw = srcPtr->width, dh = srcPtr->height;
    destPtr = NULL;			/* Suppress compiler warning. */
    switch (switches.fade.side) {
    case SIDE_BOTTOM:
	h = srcPtr->height / 2;
	dh = srcPtr->height + h;
	destPtr = Blt_CreatePicture(w, h);
	Blt_CopyPictureBits(destPtr, srcPtr, 0, srcPtr->height - h, 
		srcPtr->width, h, 0, 0);
	break;

    case SIDE_TOP:
	h = srcPtr->height / 2;
	dh = srcPtr->height + h;
	destPtr = Blt_CreatePicture(w, h);
	Blt_CopyPictureBits(destPtr, srcPtr, 0, 0, srcPtr->width, h, 0, 0);
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
	tmpPtr = destPtr;
	destPtr = Blt_CreatePicture(w, h);
	Blt_BlurPicture(destPtr, tmpPtr, r, switches.blur);
	Blt_FreePicture(tmpPtr);
    }
    tmpPtr = destPtr;
    destPtr = Blt_ReflectPicture2(tmpPtr, switches.fade.side);
    Blt_FreePicture(tmpPtr);
    JitterInit(&switches.fade.jitter);
    Blt_FadePictureWithGradient(destPtr, &switches.fade);
    if (switches.bgColor.u32 != 0) {
	tmpPtr = destPtr;
	destPtr = Blt_CreatePicture(destPtr->width, destPtr->height);
	Blt_BlankPicture(destPtr, switches.bgColor.u32);
	Blt_BlendPictures(destPtr, tmpPtr, 0, 0, destPtr->width, 
			  destPtr->height, 0, 0);
	Blt_FreePicture(tmpPtr);
    }
    tmpPtr = destPtr;
    destPtr = Blt_CreatePicture(dw, dh);

    switch (switches.fade.side) {
    case SIDE_BOTTOM:
	Blt_CopyPictureBits(destPtr, srcPtr, 0, 0, srcPtr->width, 
		srcPtr->height, 0, 0);
	Blt_CopyPictureBits(destPtr, tmpPtr, 0, 0, w, h, 0, srcPtr->height);
	break;

    case SIDE_TOP:
	Blt_CopyPictureBits(destPtr, tmpPtr, 0, 0, w, h, 0, 0);
	Blt_CopyPictureBits(destPtr, srcPtr, 0, 0, srcPtr->width, 
		srcPtr->height, 0, h);
	break;

    case SIDE_LEFT:
	Blt_CopyPictureBits(destPtr, tmpPtr, 0, 0, w, h, 0, 0);
	Blt_CopyPictureBits(destPtr, srcPtr, 0, 0, srcPtr->width, 
			    srcPtr->height, w, 0);
	break;

    case SIDE_RIGHT:
	Blt_CopyPictureBits(destPtr, srcPtr, 0, 0, srcPtr->width, 
		srcPtr->height, 0, 0);
	Blt_CopyPictureBits(destPtr, tmpPtr, 0, 0, w, h, srcPtr->width, 0);
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
ResampleOp(
    ClientData clientData,	/* Information about picture cmd. */
    Tcl_Interp *interp,		/* Interpreter to report errors back to. */
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Blt_Picture src, tmp;
    Blt_ResampleFilter hFilter, vFilter;
    PictImage *imgPtr = clientData;
    ResampleSwitches switches;
    int destWidth, destHeight;

    if (Blt_GetPictureFromObj(interp, objv[2], &src) != TCL_OK) {
	return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    /* Default source region is the entire picture. */
    switches.region.w = Blt_PictureWidth(src);
    switches.region.h = Blt_PictureHeight(src);
    /* Destination size */
    switches.width    = Blt_PictureWidth(imgPtr->picture);
    switches.height   = Blt_PictureHeight(imgPtr->picture);
    if (Blt_ParseSwitches(interp, resampleSwitches, objc - 3, objv + 3, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (!Blt_AdjustRegionToPicture(src, &switches.region)) {
	Tcl_AppendResult(interp, "impossible coordinates for region", 
		(char *)NULL);
	return TCL_ERROR;
    }
    if ((switches.flags | imgPtr->flags) & MAXPECT) {
	double sx, sy, scale;
	    
	sx = (double)switches.width / (double)switches.region.w;
	sy = (double)switches.height / (double)switches.region.h;
	scale = MIN(sx, sy);
	switches.width = (int)(switches.region.w * scale + 0.5);
	switches.height = (int)(switches.region.h * scale + 0.5);
    }	    
    if ((Blt_PictureWidth(imgPtr->picture) != switches.width) ||
	(Blt_PictureHeight(imgPtr->picture) != switches.height)) {
	Blt_AdjustPictureSize(imgPtr->picture, switches.width, switches.height);
    }
    destWidth = Blt_PictureWidth(imgPtr->picture);
    destHeight = Blt_PictureHeight(imgPtr->picture);
    if (switches.filter == NULL) {
	if (switches.region.w < destWidth) {
	    hFilter = bltMitchellFilter;
	} else {
	    hFilter = bltBoxFilter;
	}
	if (switches.region.h < destHeight) {
	    vFilter = bltMitchellFilter;
	} else {
	    vFilter = bltBoxFilter;
	}
    } else {
	hFilter = vFilter = switches.filter;
    }
    tmp = Blt_CreatePicture(switches.region.w, switches.region.h);
    Blt_CopyPictureBits(tmp, src, switches.region.x, switches.region.y, 
	switches.region.w, switches.region.h, 0, 0);
    Blt_ResamplePicture(imgPtr->picture, tmp, vFilter, hFilter);
    Blt_FreePicture(tmp);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * RotateOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
RotateOp(
    ClientData clientData,	/* Information about picture cmd. */
    Tcl_Interp *interp,		/* Interpreter to report errors back to. */
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Blt_Picture src, dest;
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
    dest = Blt_RotatePicture(src, (float)angle);
    ReplacePicture(imgPtr, dest);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectOp --
 *
 * Results:
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
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
SharpenOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Not used. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)		/* Not used. */
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
 *	Returns a standard TCL return value.
 *
 * Side effects:
 *	None.
 *
 *   $pict snap window -region {x y w h} -raise 
 *
 *---------------------------------------------------------------------------
 */
static int
SnapOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
{
    Blt_Picture picture;
    PictImage *imgPtr = clientData;
    SnapSwitches switches;
    Tk_Uid classUid;
    Tk_Window tkwin;
    const char *string;

    memset(&switches, 0, sizeof(switches));
    classUid = NULL;
    string = Tcl_GetString(objv[2]);
    tkwin = Tk_NameToWindow(NULL, string, Tk_MainWindow(interp));
    if (tkwin != NULL) {
	classUid = Tk_Class(tkwin);
    }
    if ((classUid != NULL) && (strcmp(classUid, "Canvas") == 0)) {
	picture = Blt_CanvasToPicture(interp, string, imgPtr->gamma);
    } else {
	int w, h;
	Window window;

	if (Blt_GetWindowFromObj(interp, objv[2], &window) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Blt_GetWindowRegion(imgPtr->display, window, NULL, NULL, &w, &h) 
	    != TCL_OK) {
	    Tcl_AppendResult(interp, "can't get dimensions of window \"", 
		Tcl_GetString(objv[2]), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	switches.region.x = switches.region.y = 0;
	switches.region.w = w;
	switches.region.h = h;
	if (Blt_ParseSwitches(interp, snapSwitches, objc - 3, objv + 3, 
		&switches, BLT_SWITCH_DEFAULTS) < 0) {
	    return TCL_ERROR;
	}
	if ((switches.region.w + switches.region.x) > w) {
	    switches.region.w = (w - switches.region.x);
	}
	if ((switches.region.h + switches.region.y) > h) {
	    switches.region.h = (h - switches.region.y);
	}
	if (switches.raise) {
	    XRaiseWindow(imgPtr->display, window);
	}
	/* Depth, visual, colormap */
	picture = Blt_WindowToPicture(imgPtr->display, window, 
		switches.region.x, switches.region.y, switches.region.w, 
		switches.region.h, imgPtr->gamma);
	if (picture == NULL) {
	    Tcl_AppendResult(interp, "can't obtain snapshot of window \"", 
			     Tcl_GetString(objv[2]), "\"", (char *)NULL);
	}
    }
    if (picture == NULL) {
	Blt_FreeSwitches(snapSwitches, &switches, 0);
	return TCL_ERROR;
    }
    ReplacePicture(imgPtr, picture);
    if (imgPtr->name != NULL) {
	Blt_Free(imgPtr->name);
	imgPtr->name = NULL;
    }
    Blt_NotifyImageChanged(imgPtr);
    imgPtr->flags &= ~IMPORTED_MASK;
    Blt_FreeSwitches(snapSwitches, &switches, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TranspOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int 
TranspOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Not used. */
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)		/* Not used. */
{
    PictImage *imgPtr = clientData;
    Blt_Pixel bg;

    if (Blt_GetPixelFromObj(interp, objv[2], &bg)!= TCL_OK) {
	return TCL_ERROR;
    }
    Blt_SubtractColor(imgPtr->picture, &bg);
    Blt_NotifyImageChanged(imgPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * WidthOp --
 *	Returns the current width of the picture.
 *
 *---------------------------------------------------------------------------
 */
static int
WidthOp(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* # of arguments. */
    Tcl_Obj *const *objv)		/* Argument vector. */
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
	h = Blt_PictureHeight(imgPtr->picture);
	Blt_AdjustPictureSize(imgPtr->picture, w, h);
	Blt_NotifyImageChanged(imgPtr);
    } 
    w = Blt_PictureWidth(imgPtr->picture);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), w);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Picture instance sub-command specification:
 *
 *	- Name of the sub-command.
 *	- Minimum number of characters needed to unambiguously
 *        recognize the sub-command.
 *	- Pointer to the function to be called for the sub-command.
 *	- Minimum number of arguments accepted.
 *	- Maximum number of arguments accepted.
 *	- String to be displayed for usage (arguments only).
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec pictInstOps[] =
{
    {"add",       2, ArithOp,     3, 0, "image|color",},
    {"and",       3, ArithOp,     3, 0, "image|color",},
    {"blank",     3, BlankOp,     2, 3, "?color?",},
    {"ble2nd",    4, Blend2Op,    4, 0, "bg fg ?switches?",},
    {"blend",     4, BlendOp,     4, 0, "bg fg ?switches?",},
    {"blur",      3, BlurOp,      4, 4, "src width",},
    {"cget",      2, CgetOp,      3, 3, "option",},
    {"configure", 4, ConfigureOp, 2, 0, "?option value?...",},
    {"convolve",  4, ConvolveOp,  3, 0, "src ?switches?",},
    {"copy",      3, CopyOp,      3, 0, "srcPict ?switches?",},
    {"crop",      2, CropOp,      2, 0, "bbox",},
    {"draw",      2, DrawOp,	  2, 0, "?args?",},
    {"dup",       2, DupOp,       2, 0, "?switches?",},
    {"export",    1, ExportOp,    2, 0, "format ?switches?...",},
    {"fade",      2, FadeOp,      4, 4, "src factor",},
    {"flip",      2, FlipOp,      3, 0, "x|y",},
    {"gamma",     2, GammaOp,     3, 3, "value",},
    {"get",       2, GetOp,       4, 4, "x y",},
    {"greyscale", 3, GreyscaleOp, 3, 3, "src",},
    {"height",    1, HeightOp,    2, 3, "?newHeight?",},
    {"import",    2, ImportOp,    2, 0, "format ?switches?...",},
    {"info",      2, InfoOp,      2, 2, "info",},
    {"list",      1, ListOp,      2, 0, "args...",},
    {"max",	  2, ArithOp,     3, 0, "image|color",},
    {"min",	  2, ArithOp,     3, 0, "image|color",},
    {"multiply",  2, MultiplyOp,  3, 3, "float",},
    {"nand",      2, ArithOp,     3, 0, "image|color",},
    {"nor",       2, ArithOp,     3, 0, "image|color",},
    {"or",        1, ArithOp,     3, 0, "image|color ?switches?",},
    {"project",   2, ProjectOp,   5, 0, "src coords coords ?switches...?",},
    {"put",       2, PutOp,       2, 0, "color ?window?...",},
    {"quantize",  1, QuantizeOp,  4, 4, "src numColors",},
    {"reflect",   3, ReflectOp,   3, 0, "src ?switches?",},
    {"resample",  3, ResampleOp,  3, 0, "src ?switches?",},
    {"rotate",    2, RotateOp,    4, 4, "src angle",},
    {"select",    2, SelectOp,    4, 5, "src color ?color?",},
    {"sharpen",   2, SharpenOp,   2, 0, "",},
    {"snap",      2, SnapOp,      3, 0, "window ?switches?",},
    {"subtract",  2, ArithOp,     3, 0, "image|color",},
    {"transp",    2, TranspOp,    3, 3, "bgcolor",},
    {"width",     1, WidthOp,     2, 3, "?newWidth?",},
    {"xor",       1, ArithOp,     3, 0, "image|color ?switches?",},
};

static int numPictInstOps = sizeof(pictInstOps) / sizeof(Blt_OpSpec);

static int
PictureInstCmdProc(
    ClientData clientData,		/* Information about picture cmd. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument objects. */
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
 *	This procedure is invoked when a picture command is deleted.
 *
 * Results:
 *	None.
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
 *	Loads the dynamic library representing the converters for the named
 *	format.  Designed to be called by "package require", not directly by the
 *	user.
 *	
 * Results:
 *	A standard TCL result.  Return TCL_OK is the converter was successfully
 *	loaded, TCL_ERROR otherwise.
 *
 * blt::datatable load fmt libpath lib
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PictureLoadOp(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    int objc,				/* Not used. */
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
	    return TCL_OK;		/* Converter is already loaded. */
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
    Blt_FormatString(initProcName, 11 + length + 4 + 1, "Blt_Picture%sInit", fmt);
    safeProcName = Blt_AssertMalloc(11 + length + 8 + 1);
    Blt_FormatString(safeProcName, 11 + length + 8 + 1, "Blt_Picture%sSafeInit", fmt);

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
 *	- Name of the sub-command.
 *	- Minimum number of characters needed to unambiguously
 *        recognize the sub-command.
 *	- Pointer to the function to be called for the sub-command.
 *	- Minimum number of arguments accepted.
 *	- Maximum number of arguments accepted.
 *	- String to be displayed for usage (arguments only).
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
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    int objc,				/* Not used. */
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
 *	This procedure is invoked to initialize the "tree" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the new command and adds a new entry into a global Tcl
 *	associative array.
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
    CreateProc,				/* Known compiler wanrning */
    GetInstanceProc,		
    DisplayProc,	
    FreeInstanceProc,	
    DeleteProc,	
    PostScriptProc,	
    (Tk_ImageType *)NULL		/* nextPtr */
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
 *	Registers the "picture" image type with Tk.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_RegisterPictureImageType(Tcl_Interp *interp) 
{
    Blt_PictFormat *fp, *fend;

    Tk_CreateImageType(&pictureImageType);

    Blt_CpuFeatures(interp, NULL);

    Blt_InitHashTable(&fmtTable, BLT_STRING_KEYS);
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
    Blt_PictureReadDataProc *readProc,
    Blt_PictureWriteDataProc *writeProc,
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
    return TCL_OK;
}

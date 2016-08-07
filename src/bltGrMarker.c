/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrMarker.c --
 *
 * This module implements markers for the BLT graph widget.
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
#endif /* HAVE_STRING_H */

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltHash.h"
#include "bltChain.h"
#include "bltBind.h"
#include "bltOp.h"
#include "bltBg.h"
#include "bltPicture.h"
#include "bltPs.h"
#include "bltImage.h"
#include "bltBitmap.h"
#include "bltPainter.h"
#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"
#include "bltGrElem.h"

#define GETBITMAP(b) \
        (((b)->destBitmap == None) ? (b)->srcBitmap : (b)->destBitmap)

#define MAX_OUTLINE_POINTS      12

#define IMAGE_PHOTO             (1<<7)

/* Map graph coordinates to normalized coordinates [0..1] */
#define NORMALIZE(A,x)  (((x) - (A)->tickRange.min) * (A)->tickRange.scale)

#define DEF_MARKER_ANCHOR       "center"
#define DEF_MARKER_BACKGROUND   RGB_WHITE
#define DEF_MARKER_BITMAP       (char *)NULL
#define DEF_MARKER_CAP_STYLE    "butt"
#define DEF_MARKER_COORDS       (char *)NULL
#define DEF_MARKER_DASHES       (char *)NULL
#define DEF_MARKER_DASH_OFFSET  "0"
#define DEF_MARKER_ELEMENT      (char *)NULL
#define DEF_MARKER_FOREGROUND   RGB_BLACK
#define DEF_MARKER_FILL_COLOR   RGB_RED
#define DEF_MARKER_FONT         STD_FONT
#define DEF_MARKER_GAP_COLOR    RGB_PINK
#define DEF_MARKER_HEIGHT       "0"
#define DEF_MARKER_HIDE         "no"
#define DEF_MARKER_JOIN_STYLE   "miter"
#define DEF_MARKER_JUSTIFY      "left"
#define DEF_MARKER_LINE_WIDTH   "1"
#define DEF_MARKER_MAP_X        "x"
#define DEF_MARKER_MAP_Y        "y"
#define DEF_MARKER_NAME         (char *)NULL
#define DEF_MARKER_OUTLINE_COLOR RGB_BLACK
#define DEF_MARKER_PAD          "4"
#define DEF_MARKER_ANGLE        "0.0"
#define DEF_MARKER_SCALE        "1.0"
#define DEF_MARKER_STATE        "normal"
#define DEF_MARKER_STIPPLE      (char *)NULL
#define DEF_MARKER_TEXT         (char *)NULL
#define DEF_MARKER_UNDER        "no"
#define DEF_MARKER_WIDTH        "0"
#define DEF_MARKER_WINDOW       (char *)NULL
#define DEF_MARKER_XOR          "no"
#define DEF_MARKER_X_OFFSET     "0"
#define DEF_MARKER_Y_OFFSET     "0"
#define DEF_MARKER_FILTER       "box"

#define DEF_TEXT_TAGS           "Text all"
#define DEF_IMAGE_TAGS          "Image all"
#define DEF_BITMAP_TAGS         "Bitmap all"
#define DEF_WINDOW_TAGS         "Window all"
#define DEF_POLYGON_TAGS        "Polygon all"
#define DEF_RECTANGLE_TAGS      "Rectangle all"
#define DEF_LINE_TAGS           "Line all"

static Blt_OptionParseProc ObjToCoordsProc;
static Blt_OptionPrintProc CoordsToObjProc;
static Blt_OptionFreeProc CoordsFreeProc;
static Blt_CustomOption coordsOption =
{
    ObjToCoordsProc, CoordsToObjProc, CoordsFreeProc, (ClientData)0
};
static Blt_OptionFreeProc ColorPairFreeProc;
static Blt_OptionParseProc ObjToColorPairProc;
static Blt_OptionPrintProc ColorPairToObjProc;
static Blt_CustomOption colorPairOption =
{
    ObjToColorPairProc, ColorPairToObjProc, ColorPairFreeProc, (ClientData)0
};

static Blt_OptionParseProc ObjToPictImageProc;
static Blt_OptionPrintProc PictImageToObjProc;
static Blt_OptionFreeProc PictImageFreeProc;
static Blt_CustomOption pictImageOption =
{
    ObjToPictImageProc, PictImageToObjProc, PictImageFreeProc, (ClientData)0
};

BLT_EXTERN Blt_CustomOption bltXAxisOption;
BLT_EXTERN Blt_CustomOption bltYAxisOption;
BLT_EXTERN Blt_CustomOption bltFilterOption;

static Blt_OptionFreeProc FreeTagsProc;
static Blt_OptionParseProc ObjToTagsProc;
static Blt_OptionPrintProc TagsToObjProc;
static Blt_CustomOption tagsOption = {
    ObjToTagsProc, TagsToObjProc, FreeTagsProc, (ClientData)0
};

typedef Marker *(MarkerCreateProc)(void);
typedef void    (MarkerDrawProc)(Marker *markerPtr, Drawable drawable);
typedef void    (MarkerFreeProc)(Marker *markerPtr);
typedef int     (MarkerConfigProc)(Marker *markerPtr);
typedef void    (MarkerMapProc)(Marker *markerPtr);
typedef void    (MarkerPostscriptProc)(Marker *markerPtr, Blt_Ps ps);
typedef int     (MarkerPointProc)(Marker *markerPtr, Point2d *samplePtr);
typedef int     (MarkerAreaProc)(Marker *markerPtr, Region2d *extsPtr, 
                                   int enclosed);

typedef struct {
    Blt_ConfigSpec *configSpecs;        /* Marker configuration
                                         * specifications */
    MarkerConfigProc *configProc;       /* Configuration routine. */
    MarkerDrawProc *drawProc;           /* Drawing routine. */
    MarkerFreeProc *freeProc;
    MarkerMapProc *mapProc;
    MarkerPointProc *pointProc;
    MarkerAreaProc *regionProc;
    MarkerPostscriptProc *psProc;

}  MarkerClass;


/*
 * MarkerIterator --
 *
 *      Tabs may be tagged with strings.  A tab may have many tags.  The
 *      same tag may be used for many tabs.
 *      
 */
typedef enum { 
    ITER_SINGLE, ITER_ALL, ITER_TAG, 
} IteratorType;

typedef struct _MarkerIterator {
    Graph *graphPtr;                    /* Element that we're iterating
                                         * over. */
    IteratorType type;                  /* Type of iteration:
                                         * ITER_TAG      By item tag.
                                         * ITER_ALL      By every item.
                                         * ITER_SINGLE   Single item: either 
                                         *               tag or index.
                                         */

    Marker *startPtr;                   /* Starting item.  Starting point
                                         * of search, saved if iterator is
                                         * reused.  Used for ITER_ALL and
                                         * ITER_SINGLE searches. */
    Marker *endPtr;                     /* Ending item (inclusive). */
    Marker *nextPtr;                    /* Next item. */
    /* For tag-based searches. */
    const char *tagName;                /* If non-NULL, is the tag that we
                                         * are currently iterating over. */
    Blt_HashTable *tablePtr;            /* Pointer to tag hash table. */
    Blt_HashSearch cursor;              /* Search iterator for tag hash
                                         * table. */
    Blt_ChainLink link;
} MarkerIterator;


/*
 *---------------------------------------------------------------------------
 *
 * Marker --
 *
 *      Structure defining the generic marker.  In C++ parlance this would
 *      be the base class from which all markers are derived.
 *
 *      This structure corresponds with the specific types of markers.
 *      Don't change this structure without changing the individual marker
 *      structures of each type below.
 *
 * -------------------------------------------------------------------------- 
 */
struct _Marker {
    GraphObj obj;                       /* Must be first field in
                                         * marker. */
    MarkerClass *classPtr;
    Blt_HashEntry *hashPtr;
    Blt_ChainLink link;
    const char *elemName;               /* Element associated with
                                         * marker. Let's you link a marker
                                         * to an element. The marker is
                                         * drawn only if the element is
                                         * also visible. */
    Axis2d axes;
    Point2d *worldPts;                  /* Coordinate array to position
                                         * marker */
    int numWorldPts;                    /* # of points in above array */
    int drawUnder;                      /* If non-zero, draw the marker
                                         * underneath any elements. This
                                         * can be a performance penalty
                                         * because the graph must be redraw
                                         * entirely each time the marker is
                                         * redrawn. */
    int offScreen;                      /* Indicates if the marker is
                                         * totally clipped by the plotting
                                         * area. */
    unsigned int flags;         
    int xOffset, yOffset;               /* Pixel offset from graph
                                         * position */
    int state;
};
/*
 *---------------------------------------------------------------------------
 *
 * BitmapMarker --
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    GraphObj obj;                       /* Must be first field in
                                         * marker. */
    MarkerClass *classPtr;
    Blt_HashEntry *hashPtr;
    Blt_ChainLink link;
    const char *elemName;               /* Element associated with
                                         * marker. Let's you link a marker
                                         * to an element. The marker is
                                         * drawn only if the element is
                                         * also visible. */
    Axis2d axes;
    Point2d *worldPts;                  /* Coordinate array to position
                                         * marker. */
    int numWorldPts;                    /* # of points in above array. */
    int drawUnder;                      /* If non-zero, draw the marker
                                         * underneath any elements. This
                                         * can be a performance penalty
                                         * because the graph must be redraw
                                         * entirely each time the marker is
                                         * redrawn. */
    int offScreen;                      /* Indicates if the marker is
                                         * totally clipped by the plotting
                                         * area. */
    unsigned int flags;         
    int xOffset, yOffset;               /* Pixel offset from graph
                                         * position */
    int state;

    /* Bitmap marker specific fields. */

    Pixmap srcBitmap;                   /* Original bitmap. May be further
                                         * scaled or rotated. */
    float reqAngle;                     /* Requested rotation of the
                                         * bitmap */
    float angle;                        /* Normalized rotation (0..360
                                         * degrees) */
    Tk_Anchor anchor;                   /* If only one X-Y coordinate is
                                         * given, indicates how to
                                         * translate the given marker
                                         * position.  Otherwise, if there
                                         * are two X-Y coordinates, then
                                         * this value is ignored. */
    Point2d anchorPt;                   /* Translated anchor point. */

    XColor *outlineColor;               /* Foreground color */
    XColor *fillColor;                  /* Background color */

    GC gc;                              /* Private graphic context */
    GC fillGC;                          /* Shared graphic context */
    Pixmap destBitmap;                  /* Bitmap to be drawn. */
    int destWidth, destHeight;          /* Dimensions of the final
                                         * bitmap */

    Point2d outline[MAX_OUTLINE_POINTS];/* Polygon representing the
                                         * background of the bitmap. */
    int numOutlinePts;
} BitmapMarker;

static Blt_ConfigSpec bitmapConfigSpecs[] =
{
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_MARKER_ANCHOR, 
        Blt_Offset(BitmapMarker, anchor), 0},
    {BLT_CONFIG_COLOR, "-background", "background", "Background",
        DEF_MARKER_BACKGROUND, Blt_Offset(BitmapMarker, fillColor),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-bindtags", "tags" },
    {BLT_CONFIG_BITMAP, "-bitmap", "bitmap", "Bitmap", DEF_MARKER_BITMAP, 
        Blt_Offset(BitmapMarker, srcBitmap), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
        Blt_Offset(BitmapMarker, worldPts), BLT_CONFIG_NULL_OK, 
        &coordsOption},
    {BLT_CONFIG_STRING, "-element", "element", "Element", DEF_MARKER_ELEMENT, 
        Blt_Offset(BitmapMarker, elemName), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-fill", "background", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
        DEF_MARKER_FOREGROUND, Blt_Offset(BitmapMarker, outlineColor),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_MARKER_HIDE, 
        Blt_Offset(BitmapMarker, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
        Blt_Offset(BitmapMarker, axes.x), 0, &bltXAxisOption},
    {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
        Blt_Offset(BitmapMarker, axes.y), 0, &bltYAxisOption},
    {BLT_CONFIG_STRING, "-name", (char *)NULL, (char *)NULL, DEF_MARKER_NAME, 
        Blt_Offset(BitmapMarker, obj.name), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-outline", "foreground", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_FLOAT, "-rotate", "rotate", "Rotate", DEF_MARKER_ANGLE, 
        Blt_Offset(BitmapMarker, reqAngle), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_MARKER_STATE, 
        Blt_Offset(BitmapMarker, state), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_BITMAP_TAGS, 0,
        BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
        Blt_Offset(BitmapMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
        Blt_Offset(BitmapMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
        Blt_Offset(BitmapMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc BitmapConfigureProc;
static MarkerCreateProc BitmapCreateProc;
static MarkerDrawProc BitmapDrawProc;
static MarkerFreeProc BitmapFreeProc;
static MarkerMapProc BitmapMapProc;
static MarkerPointProc BitmapPointProc;
static MarkerPostscriptProc BitmapPostscriptProc;
static MarkerAreaProc BitmapAreaProc;

static MarkerClass bitmapMarkerClass = {
    bitmapConfigSpecs,
    BitmapConfigureProc,
    BitmapDrawProc,
    BitmapFreeProc,
    BitmapMapProc,
    BitmapPointProc,
    BitmapAreaProc,
    BitmapPostscriptProc,
};

/*
 *---------------------------------------------------------------------------
 *
 * ImageMarker --
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    GraphObj obj;                       /* Must be first field in
                                         * marker. */
    MarkerClass *classPtr;
    Blt_HashEntry *hashPtr;
    Blt_ChainLink link;
    const char *elemName;               /* Element associated with
                                         * marker. Let's you link a marker
                                         * to an element. The marker is
                                         * drawn only if the element is
                                         * also visible. */
    Axis2d axes;
    Point2d *worldPts;                  /* Coordinate array to position
                                         * marker. */
    int numWorldPts;                    /* # of points in above array. */
    int drawUnder;                      /* If non-zero, draw the marker
                                         * underneath any elements. This
                                         * can be a performance penalty
                                         * because the graph must be redraw
                                         * entirely each time the marker is
                                         * redrawn. */
    int offScreen;                      /* Indicates if the marker is
                                         * totally clipped by the plotting
                                         * area. */
    unsigned int flags;         
    int xOffset, yOffset;               /* Pixel offset from graph
                                           position */
    int state;

    /* Image marker specific fields */

    Tk_Image tkImage;                   /* Tk image to be displayed. */
    Tk_Anchor anchor;                   /* Indicates how to translate the
                                         * given marker position. */
    Point2d anchorPt;                   /* Translated anchor point. */
    int width, height;                  /* Dimensions of the possibly
                                         * scaled image. */
    Blt_Painter painter;
    Blt_Picture picture;
    Blt_ResampleFilter filter;
    int pictX, pictY;                   /*  */
    Blt_Picture scaled;                 /* Pixmap containing the scaled
                                         * image */
    GC gc;
} ImageMarker;

static Blt_ConfigSpec imageConfigSpecs[] =
{
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_MARKER_ANCHOR, 
        Blt_Offset(ImageMarker, anchor), 0},
    {BLT_CONFIG_SYNONYM, "-bindtags", "tags" },
    {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
        Blt_Offset(ImageMarker, worldPts), BLT_CONFIG_NULL_OK, &coordsOption},
    {BLT_CONFIG_STRING, "-element", "element", "Element", DEF_MARKER_ELEMENT, 
        Blt_Offset(ImageMarker, elemName), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-filter", "filter", "Filter", 
        DEF_MARKER_FILTER, Blt_Offset(ImageMarker, filter), 
        BLT_CONFIG_NULL_OK | BLT_CONFIG_DONT_SET_DEFAULT, &bltFilterOption},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_MARKER_HIDE,      
        Blt_Offset(ImageMarker, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_CUSTOM, "-image", "image", "Image", (char *)NULL, 
        Blt_Offset(ImageMarker, picture), BLT_CONFIG_NULL_OK, &pictImageOption},
    {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
        Blt_Offset(ImageMarker, axes.x), 0, &bltXAxisOption},
    {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
        Blt_Offset(ImageMarker, axes.y), 0, &bltYAxisOption},
    {BLT_CONFIG_STRING, "-name", (char *)NULL, (char *)NULL, DEF_MARKER_NAME, 
        Blt_Offset(ImageMarker, obj.name), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_MARKER_STATE, 
        Blt_Offset(ImageMarker, state), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_IMAGE_TAGS, 0,
        BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
        Blt_Offset(ImageMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
        Blt_Offset(ImageMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
        Blt_Offset(ImageMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc ImageConfigureProc;
static MarkerCreateProc ImageCreateProc;
static MarkerDrawProc ImageDrawProc;
static MarkerFreeProc ImageFreeProc;
static MarkerMapProc ImageMapProc;
static MarkerPointProc ImagePointProc;
static MarkerPostscriptProc ImagePostscriptProc;
static MarkerAreaProc ImageAreaProc;

static MarkerClass imageMarkerClass = {
    imageConfigSpecs,
    ImageConfigureProc,
    ImageDrawProc,
    ImageFreeProc,
    ImageMapProc,
    ImagePointProc,
    ImageAreaProc,
    ImagePostscriptProc,
};

/*
 *---------------------------------------------------------------------------
 *
 * LineMarker --
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    GraphObj obj;                       /* Must be first field in
                                         * marker. */
    MarkerClass *classPtr;
    Blt_HashEntry *hashPtr;
    Blt_ChainLink link;
    const char *elemName;               /* Element associated with
                                         * marker. Let's you link a marker
                                         * to an element. The marker is
                                         * drawn only if the element is
                                         * also visible. */
    Axis2d axes;
    Point2d *worldPts;                  /* Coordinate array to position
                                         * marker. */
    int numWorldPts;                    /* # of points in above array */
    int drawUnder;                      /* If non-zero, draw the marker
                                         * underneath any elements. This
                                         * can be a performance penalty
                                         * because the graph must be redraw
                                         * entirely each time the marker is
                                         * redrawn. */
    int offScreen;                      /* Indicates if the marker is
                                         * totally clipped by the plotting
                                         * area. */
    unsigned int flags;         
    int xOffset, yOffset;               /* Pixel offset from graph
                                         * position */
    int state;

    /* Line marker specific fields */
    XColor *fillColor;
    XColor *outlineColor;               /* Foreground and background
                                         * colors */
    int lineWidth;                      /* Line width. */
    int capStyle;                       /* Cap style. */
    int joinStyle;                      /* Join style.*/
    Blt_Dashes dashes;                  /* Dash list values (max 11) */
    GC gc;                              /* Private graphic context */
    Segment2d *segments;                /* Malloc'ed array of points.
                                         * Represents individual line
                                         * segments (2 points per segment)
                                         * comprising the mapped line.  The
                                         * segments may not necessarily be
                                         * connected after clipping. */
    int numSegments;                    /* # segments in the above array. */
    int xor;
    int xorState;                       /* State of XOR drawing. Indicates
                                         * if the marker is currently
                                         * drawn. */
} LineMarker;

static Blt_ConfigSpec lineConfigSpecs[] =
{
    {BLT_CONFIG_SYNONYM, "-bindtags", "tags" },
    {BLT_CONFIG_CAP_STYLE, "-cap", "cap", "Cap", DEF_MARKER_CAP_STYLE, 
        Blt_Offset(LineMarker, capStyle), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
        Blt_Offset(LineMarker, worldPts), BLT_CONFIG_NULL_OK, &coordsOption},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes", DEF_MARKER_DASHES, 
        Blt_Offset(LineMarker, dashes), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-dashoffset", "dashOffset", "DashOffset",
        DEF_MARKER_DASH_OFFSET, Blt_Offset(LineMarker, dashes.offset),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-element", "element", "Element", DEF_MARKER_ELEMENT, 
        Blt_Offset(LineMarker, elemName), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-fill", "fill", "Fill", (char *)NULL, 
        Blt_Offset(LineMarker, fillColor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_JOIN_STYLE, "-join", "join", "Join", DEF_MARKER_JOIN_STYLE, 
     Blt_Offset(LineMarker, joinStyle), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth",
        DEF_MARKER_LINE_WIDTH, Blt_Offset(LineMarker, lineWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_MARKER_HIDE, 
        Blt_Offset(LineMarker, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
        Blt_Offset(LineMarker, axes.x), 0, &bltXAxisOption},
    {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
        Blt_Offset(LineMarker, axes.y), 0, &bltYAxisOption},
    {BLT_CONFIG_STRING, "-name", (char *)NULL, (char *)NULL, DEF_MARKER_NAME, 
        Blt_Offset(LineMarker, obj.name), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-outline", "outline", "Outline",
        DEF_MARKER_OUTLINE_COLOR, Blt_Offset(LineMarker, outlineColor),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_MARKER_STATE, 
        Blt_Offset(LineMarker, state), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_LINE_TAGS, 0,
        BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
        Blt_Offset(LineMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
        Blt_Offset(LineMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-xor", "xor", "Xor", DEF_MARKER_XOR, 
        Blt_Offset(LineMarker, xor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
        Blt_Offset(LineMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc LineConfigureProc;
static MarkerCreateProc LineCreateProc;
static MarkerDrawProc LineDrawProc;
static MarkerFreeProc LineFreeProc;
static MarkerMapProc LineMapProc;
static MarkerPointProc LinePointProc;
static MarkerPostscriptProc LinePostscriptProc;
static MarkerAreaProc LineAreaProc;

static MarkerClass lineMarkerClass = {
    lineConfigSpecs,
    LineConfigureProc,
    LineDrawProc,
    LineFreeProc,
    LineMapProc,
    LinePointProc,
    LineAreaProc,
    LinePostscriptProc,
};

/*
 *---------------------------------------------------------------------------
 *
 * PolygonMarker --
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    GraphObj obj;                       /* Must be first field in
                                         * marker. */
    MarkerClass *classPtr;
    Blt_HashEntry *hashPtr;
    Blt_ChainLink link;
    const char *elemName;               /* Element associated with
                                         * marker. Let's you link a marker
                                         * to an element. The marker is
                                         * drawn only if the element is
                                         * also visible. */
    Axis2d axes;
    Point2d *worldPts;                  /* Coordinate array to position
                                         * marker. */
    int numWorldPts;                    /* # of points in above array */
    int drawUnder;                      /* If non-zero, draw the marker
                                         * underneath any elements. This
                                         * can be a performance penalty
                                         * because the graph must be redraw
                                         * entirely each time the marker is
                                         * redrawn. */
    int offScreen;                      /* Indicates if the marker is
                                         * totally clipped by the plotting
                                         * area. */
    unsigned int flags;         
    int xOffset, yOffset;               /* Pixel offset from graph
                                         * position */
    int state;

    /* Polygon marker specific fields */
    Point2d *screenPts;                 /* Array of points representing the
                                         * polygon in screen
                                         * coordinates. It's not used for
                                         * drawing, but to generate the
                                         * outlinePts and fillPts arrays
                                         * that are the coordinates of the
                                         * possibly clipped outline and
                                         * filled polygon. */
    ColorPair outline;
    ColorPair fill;
    Pixmap stipple;                     /* Stipple pattern to fill the
                                         * polygon. */
    int lineWidth;                      /* Width of polygon outline. */
    int capStyle;
    int joinStyle;
    Blt_Dashes dashes;                  /* List of dash values.  Indicates
                                         * how to draw the dashed line.  If
                                         * no dash values are provided, or
                                         * the first value is zero, then
                                         * the line is drawn solid. */
    GC outlineGC;                       /* Graphics context to draw the
                                         * outline of the polygon. */
    GC fillGC;                          /* Graphics context to draw the
                                         * filled polygon. */
    Point2d *fillPts;                   /* Malloc'ed array of points used
                                         * to draw the filled
                                         * polygon. These points may form a
                                         * degenerate polygon after
                                         * clipping. */
    int numFillPts;                     /* # points in the above array. */
    Segment2d *outlinePts;              /* Malloc'ed array of points.
                                         * Represents individual line
                                         * segments (2 points per segment)
                                         * comprising the outline of the
                                         * polygon.  The segments may not
                                         * necessarily be closed or
                                         * connected after clipping. */
    int numOutlinePts;                  /* # points in the above array. */
    int xor;
    int xorState;                       /* State of XOR drawing. Indicates
                                         * if the marker is visible. We
                                         * have to drawn it again to erase
                                         * it. */
} PolygonMarker;

static Blt_ConfigSpec polygonConfigSpecs[] =
{
    {BLT_CONFIG_SYNONYM, "-bindtags", "tags" },
    {BLT_CONFIG_CAP_STYLE, "-cap", "cap", "Cap", DEF_MARKER_CAP_STYLE, 
        Blt_Offset(PolygonMarker, capStyle), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
        Blt_Offset(PolygonMarker, worldPts), BLT_CONFIG_NULL_OK, &coordsOption},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes", DEF_MARKER_DASHES, 
        Blt_Offset(PolygonMarker, dashes), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-dashoffset", "dashOffset", "DashOffset",
        DEF_MARKER_DASH_OFFSET, Blt_Offset(PolygonMarker, dashes.offset),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-element", "element", "Element", DEF_MARKER_ELEMENT, 
        Blt_Offset(PolygonMarker, elemName), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-fill", "fill", "Fill", DEF_MARKER_FILL_COLOR, 
        Blt_Offset(PolygonMarker, fill), BLT_CONFIG_NULL_OK, &colorPairOption},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_MARKER_HIDE, 
        Blt_Offset(PolygonMarker, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_JOIN_STYLE, "-join", "join", "Join", DEF_MARKER_JOIN_STYLE, 
        Blt_Offset(PolygonMarker, joinStyle), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth",
        DEF_MARKER_LINE_WIDTH, Blt_Offset(PolygonMarker, lineWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
        Blt_Offset(PolygonMarker, axes.x), 0, &bltXAxisOption},
    {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
        Blt_Offset(PolygonMarker, axes.y), 0, &bltYAxisOption},
    {BLT_CONFIG_STRING, "-name", (char *)NULL, (char *)NULL, DEF_MARKER_NAME, 
        Blt_Offset(PolygonMarker, obj.name), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-outline", "outline", "Outline", 
        DEF_MARKER_OUTLINE_COLOR, Blt_Offset(PolygonMarker, outline),
        BLT_CONFIG_NULL_OK, &colorPairOption},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_MARKER_STATE, 
        Blt_Offset(PolygonMarker, state), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMAP, "-stipple", "stipple", "Stipple", DEF_MARKER_STIPPLE, 
        Blt_Offset(PolygonMarker, stipple), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_POLYGON_TAGS, 0,
        BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
        Blt_Offset(PolygonMarker, drawUnder), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
        Blt_Offset(PolygonMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-xor", "xor", "Xor", DEF_MARKER_XOR, 
        Blt_Offset(PolygonMarker, xor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
        Blt_Offset(PolygonMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc PolygonConfigureProc;
static MarkerCreateProc PolygonCreateProc;
static MarkerDrawProc PolygonDrawProc;
static MarkerFreeProc PolygonFreeProc;
static MarkerMapProc PolygonMapProc;
static MarkerPointProc PolygonPointProc;
static MarkerPostscriptProc PolygonPostscriptProc;
static MarkerAreaProc PolygonAreaProc;

static MarkerClass polygonMarkerClass = {
    polygonConfigSpecs,
    PolygonConfigureProc,
    PolygonDrawProc,
    PolygonFreeProc,
    PolygonMapProc,
    PolygonPointProc,
    PolygonAreaProc,
    PolygonPostscriptProc,
};

/*
 *---------------------------------------------------------------------------
 *
 * RectangleMarker --
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    GraphObj obj;                       /* Must be first field in
                                         * marker. */
    MarkerClass *classPtr;
    Blt_HashEntry *hashPtr;
    Blt_ChainLink link;
    const char *elemName;               /* Element associated with
                                         * marker. Let's you link a marker
                                         * to an element. The marker is
                                         * drawn only if the element is
                                         * also visible. */
    Axis2d axes;
    Point2d *worldPts;                  /* Coordinate array to position
                                         * marker. */
    int numWorldPts;                    /* # of points in above array */
    int drawUnder;                      /* If non-zero, draw the marker
                                         * underneath any elements. This
                                         * can be a performance penalty
                                         * because the graph must be redraw
                                         * entirely each time the marker is
                                         * redrawn. */
    int offScreen;                      /* Indicates if the marker is
                                         * totally clipped by the plotting
                                         * area. */
    unsigned int flags;         
    int xOffset, yOffset;               /* Pixel offset from graph
                                         * position */
    int state;

    /* Rectangle marker specific fields */
    ColorPair outline;
    ColorPair fill;
    Pixmap stipple;                     /* Stipple pattern to fill the
                                         * rectangle. */
    int lineWidth;                      /* Width of rectangle outline. */
    int capStyle;
    int joinStyle;
    Blt_Dashes dashes;                  /* List of dash values.  Indicates
                                         * how to draw the dashed line.  If
                                         * no dash values are provided, or
                                         * the first value is zero, then
                                         * the line is drawn solid. */
    GC outlineGC;                       /* Graphics context to draw the
                                         * outline of the rectangle. */
    GC fillGC;                          /* Graphics context to draw the
                                         * filled rectangle. */
    Point2d corner1;
    Point2d corner2;
    
    Point2d fillPts[2];                 /* Array of points used
                                         * to draw the filled
                                         * rectangle. These points may form a
                                         * degenerate rectangle after
                                         * clipping. */
    Segment2d outlineSegments[4];       /* Array of points.  Represents
                                         * individual line segments (2
                                         * points per segment) comprising
                                         * the outline of the rectangle.
                                         * The segments may not necessarily
                                         * be closed or connected after
                                         * clipping. */
    int numOutlineSegments;
    int xor;
    int xorState;                       /* State of XOR drawing. Indicates
                                         * if the marker is visible. We
                                         * have to drawn it again to erase
                                         * it. */
} RectangleMarker;

static Blt_ConfigSpec rectangleConfigSpecs[] =
{
    {BLT_CONFIG_SYNONYM, "-bindtags", "tags" },
    {BLT_CONFIG_CAP_STYLE, "-cap", "cap", "Cap", DEF_MARKER_CAP_STYLE, 
        Blt_Offset(RectangleMarker, capStyle), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
        Blt_Offset(RectangleMarker, worldPts), BLT_CONFIG_NULL_OK,
        &coordsOption},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes", DEF_MARKER_DASHES, 
        Blt_Offset(RectangleMarker, dashes), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-dashoffset", "dashOffset", "DashOffset",
        DEF_MARKER_DASH_OFFSET, Blt_Offset(RectangleMarker, dashes.offset),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-element", "element", "Element", DEF_MARKER_ELEMENT, 
        Blt_Offset(RectangleMarker, elemName), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-fill", "fill", "Fill", DEF_MARKER_FILL_COLOR, 
        Blt_Offset(RectangleMarker, fill), BLT_CONFIG_NULL_OK,
        &colorPairOption},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_MARKER_HIDE, 
        Blt_Offset(RectangleMarker, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_JOIN_STYLE, "-join", "join", "Join", DEF_MARKER_JOIN_STYLE, 
        Blt_Offset(RectangleMarker, joinStyle), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth",
        DEF_MARKER_LINE_WIDTH, Blt_Offset(RectangleMarker, lineWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
        Blt_Offset(RectangleMarker, axes.x), 0, &bltXAxisOption},
    {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
        Blt_Offset(RectangleMarker, axes.y), 0, &bltYAxisOption},
    {BLT_CONFIG_STRING, "-name", (char *)NULL, (char *)NULL, DEF_MARKER_NAME, 
        Blt_Offset(RectangleMarker, obj.name), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-outline", "outline", "Outline", 
        DEF_MARKER_OUTLINE_COLOR, Blt_Offset(RectangleMarker, outline),
        BLT_CONFIG_NULL_OK, &colorPairOption},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_MARKER_STATE, 
        Blt_Offset(RectangleMarker, state), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMAP, "-stipple", "stipple", "Stipple", DEF_MARKER_STIPPLE, 
        Blt_Offset(RectangleMarker, stipple), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_RECTANGLE_TAGS, 0,
        BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
        Blt_Offset(RectangleMarker, drawUnder), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
        Blt_Offset(RectangleMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-xor", "xor", "Xor", DEF_MARKER_XOR, 
        Blt_Offset(RectangleMarker, xor), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
        Blt_Offset(RectangleMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc RectangleConfigureProc;
static MarkerCreateProc RectangleCreateProc;
static MarkerDrawProc RectangleDrawProc;
static MarkerFreeProc RectangleFreeProc;
static MarkerMapProc RectangleMapProc;
static MarkerPointProc RectanglePointProc;
static MarkerPostscriptProc RectanglePostscriptProc;
static MarkerAreaProc RectangleAreaProc;

static MarkerClass rectangleMarkerClass = {
    rectangleConfigSpecs,
    RectangleConfigureProc,
    RectangleDrawProc,
    RectangleFreeProc,
    RectangleMapProc,
    RectanglePointProc,
    RectangleAreaProc,
    RectanglePostscriptProc,
};


/*
 *---------------------------------------------------------------------------
 *
 * TextMarker --
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    GraphObj obj;                       /* Must be first field in
                                         * marker. */
    MarkerClass *classPtr;
    Blt_HashEntry *hashPtr;
    Blt_ChainLink link;
    const char *elemName;               /* Element associated with
                                         * marker. Let's you link a marker
                                         * to an element. The marker is
                                         * drawn only if the element is
                                         * also visible. */
    Axis2d axes;
    Point2d *worldPts;                  /* Coordinate array to position
                                         * marker. */
    int numWorldPts;                    /* # of points in above array */
    int drawUnder;                      /* If non-zero, draw the marker
                                         * underneath any elements. This
                                         * can be a performance penalty
                                         * because the graph must be redraw
                                         * entirely each time the marker is
                                         * redrawn. */
    int offScreen;                      /* Indicates if the marker is
                                         * totally clipped by the plotting
                                         * area. */
    unsigned int flags;         
    int xOffset, yOffset;               /* Pixel offset from graph
                                         * position */
    int state;

    /* Text marker specific fields */
#ifdef notdef
    const char *textVarName;            /* Name of variable (malloc'ed) or
                                         * NULL. If non-NULL, graph
                                         * displays the contents of this
                                         * variable. */
#endif
    const char *string;                 /* Text string to be display.  The
                                         * string make contain newlines. */
    Tk_Anchor anchor;                   /* Indicates how to translate the
                                         * given marker position. */
    Point2d anchorPt;                   /* Translated anchor point. */
    int width, height;                  /* Dimension of bounding box. */
    TextStyle style;                    /* Text attributes (font, fg, anchor,
                                         * etc) */
    Point2d outline[5];
    XColor *fillColor;
    GC fillGC;
} TextMarker;


static Blt_ConfigSpec textConfigSpecs[] =
{
    {BLT_CONFIG_SYNONYM, "-bindtags", "tags" },
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_MARKER_ANCHOR, 
        Blt_Offset(TextMarker, anchor), 0},
    {BLT_CONFIG_COLOR, "-background", "background", "MarkerBackground",
        (char *)NULL, Blt_Offset(TextMarker, fillColor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bg", "background", "Background", (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-bindtags", "tags" },
    {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
        Blt_Offset(TextMarker, worldPts), BLT_CONFIG_NULL_OK, 
        &coordsOption},
    {BLT_CONFIG_STRING, "-element", "element", "Element",
        DEF_MARKER_ELEMENT, Blt_Offset(TextMarker, elemName), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", "Foreground", (char *)NULL, 0, 0},
    {BLT_CONFIG_SYNONYM, "-fill", "background", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font",  DEF_MARKER_FONT, 
        Blt_Offset(TextMarker, style.font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
        DEF_MARKER_FOREGROUND, Blt_Offset(TextMarker, style.color), 0},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_MARKER_HIDE, 
        Blt_Offset(TextMarker, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify",
        DEF_MARKER_JUSTIFY, Blt_Offset(TextMarker, style.justify),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
        Blt_Offset(TextMarker, axes.x), 0, &bltXAxisOption},
    {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
        Blt_Offset(TextMarker, axes.y), 0, &bltYAxisOption},
    {BLT_CONFIG_STRING, "-name", (char *)NULL, (char *)NULL, DEF_MARKER_NAME, 
        Blt_Offset(TextMarker, obj.name), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-outline", "foreground", (char *)NULL, (char *)NULL, 
        0, 0},
    {BLT_CONFIG_PAD, "-padx", "padX", "PadX", DEF_MARKER_PAD, 
        Blt_Offset(TextMarker, style.padX), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-pady", "padY", "PadY", DEF_MARKER_PAD, 
        Blt_Offset(TextMarker, style.padY), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FLOAT, "-rotate", "rotate", "Rotate", DEF_MARKER_ANGLE, 
        Blt_Offset(TextMarker, style.angle), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_MARKER_STATE, 
        Blt_Offset(TextMarker, state), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_TEXT_TAGS, 0,
        BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_STRING, "-text", "text", "Text", DEF_MARKER_TEXT, 
        Blt_Offset(TextMarker, string), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
        Blt_Offset(TextMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
        Blt_Offset(TextMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
        Blt_Offset(TextMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc TextConfigureProc;
static MarkerCreateProc TextCreateProc;
static MarkerDrawProc TextDrawProc;
static MarkerFreeProc TextFreeProc;
static MarkerMapProc TextMapProc;
static MarkerPointProc TextPointProc;
static MarkerPostscriptProc TextPostscriptProc;
static MarkerAreaProc TextAreaProc;

static MarkerClass textMarkerClass = {
    textConfigSpecs,
    TextConfigureProc,
    TextDrawProc,
    TextFreeProc,
    TextMapProc,
    TextPointProc,
    TextAreaProc,
    TextPostscriptProc,
};

/*
 *---------------------------------------------------------------------------
 *
 * WindowMarker --
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    GraphObj obj;                       /* Must be first field in
                                         * marker. */
    MarkerClass *classPtr;
    Blt_HashEntry *hashPtr;
    Blt_ChainLink link;
    const char *elemName;               /* Element associated with
                                         * marker. Let's you link a marker
                                         * to an element. The marker is
                                         * drawn only if the element is
                                         * also visible. */
    Axis2d axes;
    Point2d *worldPts;                  /* Coordinate array to position
                                         * marker */
    int numWorldPts;                    /* # of points in above array */
    int drawUnder;                      /* If non-zero, draw the marker
                                         * underneath any elements. This
                                         * can be a performance penalty
                                         * because the graph must be redraw
                                         * entirely each time the marker is
                                         * redrawn. */
    int offScreen;                      /* Indicates if the marker is
                                         * totally clipped by the plotting
                                         * area. */
    unsigned int flags;         
    int xOffset, yOffset;               /* Pixel offset from graph
                                         * position */
    int state;

    /* Window marker specific fields */
    const char *childName;              /* Name of child widget. */
    Tk_Window child;                    /* Window to display. */
    int reqWidth, reqHeight;            /* If non-zero, this overrides the
                                         * size requested by the child
                                         * widget. */
    Tk_Anchor anchor;                   /* Indicates how to translate the
                                         * given marker position. */
    Point2d anchorPt;                   /* Translated anchor point. */
    int width, height;                  /* Current size of the child
                                         * window. */

} WindowMarker;

static Blt_ConfigSpec windowConfigSpecs[] =
{
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor", DEF_MARKER_ANCHOR, 
        Blt_Offset(WindowMarker, anchor), 0},
    {BLT_CONFIG_SYNONYM, "-bindtags", "tags" },
    {BLT_CONFIG_CUSTOM, "-coords", "coords", "Coords", DEF_MARKER_COORDS, 
        Blt_Offset(WindowMarker, worldPts), BLT_CONFIG_NULL_OK, 
        &coordsOption},
    {BLT_CONFIG_STRING, "-element", "element", "Element", DEF_MARKER_ELEMENT, 
        Blt_Offset(WindowMarker, elemName), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-height", "height", "Height", DEF_MARKER_HEIGHT, 
        Blt_Offset(WindowMarker, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_MARKER_HIDE, 
        Blt_Offset(WindowMarker, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_MARKER_MAP_X, 
        Blt_Offset(WindowMarker, axes.x), 0, &bltXAxisOption},
    {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_MARKER_MAP_Y, 
        Blt_Offset(WindowMarker, axes.y), 0, &bltYAxisOption},
    {BLT_CONFIG_STRING, "-name", (char *)NULL, (char *)NULL, DEF_MARKER_NAME, 
        Blt_Offset(WindowMarker, obj.name), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_MARKER_STATE, 
        Blt_Offset(WindowMarker, state), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_WINDOW_TAGS, 0,
        BLT_CONFIG_NULL_OK, &tagsOption},
    {BLT_CONFIG_BOOLEAN, "-under", "under", "Under", DEF_MARKER_UNDER, 
        Blt_Offset(WindowMarker, drawUnder), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_POS, "-width", "width", "Width", DEF_MARKER_WIDTH, 
        Blt_Offset(WindowMarker, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-window", "window", "Window", DEF_MARKER_WINDOW, 
        Blt_Offset(WindowMarker, childName), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS, "-xoffset", "xOffset", "XOffset", DEF_MARKER_X_OFFSET, 
        Blt_Offset(WindowMarker, xOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS, "-yoffset", "yOffset", "YOffset", DEF_MARKER_Y_OFFSET, 
        Blt_Offset(WindowMarker, yOffset), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static MarkerConfigProc WindowConfigureProc;
static MarkerCreateProc WindowCreateProc;
static MarkerDrawProc WindowDrawProc;
static MarkerFreeProc WindowFreeProc;
static MarkerMapProc WindowMapProc;
static MarkerPointProc WindowPointProc;
static MarkerPostscriptProc WindowPostscriptProc;
static MarkerAreaProc WindowAreaProc;

static MarkerClass windowMarkerClass = {
    windowConfigSpecs,
    WindowConfigureProc,
    WindowDrawProc,
    WindowFreeProc,
    WindowMapProc,
    WindowPointProc,
    WindowAreaProc,
    WindowPostscriptProc,
};

static Tk_ImageChangedProc ImageChangedProc;



#ifdef notdef

static MarkerClass ovalMarkerClass = {
    ovalConfigSpecs,
    OvalConfigureProc,
    OvalDrawProc,
    OvalFreeProc,
    OvalMapProc,
    OvalPointProc,
    OvalAreaProc,
    OvalPostscriptProc,
};
#endif

static Tcl_FreeProc FreeMarker;

#define SWAP(a,b)       { double tmp; tmp = a, a = b, b = tmp; }


/*
 *---------------------------------------------------------------------------
 *
 * NextTaggedMarker --
 *
 *      Returns the next marker derived from the given tag.
 *
 * Results:
 *      Returns the pointer to the next marker in the iterator.  If no more
 *      markers are available, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Marker *
NextTaggedMarker(MarkerIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
        if (iterPtr->link != NULL) {
            Marker *markerPtr;
            
            markerPtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return markerPtr;
        }
        break;

    case ITER_ALL:
        {
            Blt_HashEntry *hPtr;
            
            hPtr = Blt_NextHashEntry(&iterPtr->cursor); 
            if (hPtr != NULL) {
                return Blt_GetHashValue(hPtr);
            }
            break;
        }

    default:
        break;
    }   
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * FirstTaggedMarker --
 *
 *      Returns the first isoline derived from the given tag.
 *
 * Results:
 *      Returns the first isoline in the sequence.  If no more isolines are in
 *      the list, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Marker *
FirstTaggedMarker(MarkerIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
        if (iterPtr->link != NULL) {
            Marker *markerPtr;
            
            markerPtr = Blt_Chain_GetValue(iterPtr->link);
            iterPtr->link = Blt_Chain_NextLink(iterPtr->link);
            return markerPtr;
        }
        break;
    case ITER_ALL:
        {
            Blt_HashEntry *hPtr;
            
            hPtr = Blt_FirstHashEntry(iterPtr->tablePtr, &iterPtr->cursor);
            if (hPtr != NULL) {
                return Blt_GetHashValue(hPtr);
            }
        }
        break;

    case ITER_SINGLE:
        return iterPtr->startPtr;
    } 
    return NULL;
}

static int
GetMarkerFromObj(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
                 Marker **markerPtrPtr)
{
    Blt_HashEntry *hPtr;
    const char *string;

    string = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&graphPtr->markers.nameTable, string);
    if (hPtr != NULL) {
        *markerPtrPtr = Blt_GetHashValue(hPtr);
        return TCL_OK;
    }
    if (interp != NULL) {
        Tcl_AppendResult(interp, "can't find marker \"", string, 
             "\" in \"", Tk_PathName(graphPtr->tkwin), (char *)NULL);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetMarkerIterator --
 *
 *      Converts a string representing a tab index into an tab pointer.  The
 *      index may be in one of the following forms:
 *
 *       "all"          All isolines.
 *       name           Name of the isoline.
 *       tag            Tag associated with isolines.
 *
 *---------------------------------------------------------------------------
 */
static int
GetMarkerIterator(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
                  MarkerIterator *iterPtr)
{
    Marker *markerPtr;
    Blt_Chain chain;
    const char *string;
    char c;
    int numBytes, length;

    iterPtr->type = ITER_SINGLE;
    iterPtr->tagName = Tcl_GetStringFromObj(objPtr, &numBytes);
    iterPtr->link = NULL;
    iterPtr->nextPtr = NULL;
    iterPtr->startPtr = iterPtr->endPtr = NULL;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'a') && (strcmp(iterPtr->tagName, "all") == 0)) {
        iterPtr->type  = ITER_ALL;
        iterPtr->tablePtr = &graphPtr->markers.nameTable;
    } else if (GetMarkerFromObj(NULL, graphPtr, objPtr, &markerPtr) == TCL_OK) {
        iterPtr->startPtr = iterPtr->endPtr = markerPtr;
        iterPtr->type = ITER_SINGLE;
    } else if ((chain = Blt_Tags_GetItemList(&graphPtr->markers.tags, string)) 
               != NULL) {
        iterPtr->tagName = string;
        iterPtr->link = Blt_Chain_FirstLink(chain);
        iterPtr->type = ITER_TAG;
    } else {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "can't find marker name or tag \"", 
                string, "\" in \"", Tk_PathName(graphPtr->tkwin), 
                "\"", (char *)NULL);
        }
        return TCL_ERROR;
    }   
    return TCL_OK;
}



static int
RenameMarker(Graph *graphPtr, Marker *markerPtr, const char *oldName, 
             const char *newName)
{
    int isNew;
    Blt_HashEntry *hPtr;

    /* Rename the marker only if no marker already exists by that name */
    hPtr = Blt_CreateHashEntry(&graphPtr->markers.nameTable, newName, &isNew);
    if (!isNew) {
        Tcl_AppendResult(graphPtr->interp, "can't rename marker: \"", newName,
            "\" already exists", (char *)NULL);
        return TCL_ERROR;
    }
    markerPtr->obj.name = Blt_AssertStrdup(newName);
    markerPtr->hashPtr = hPtr;
    Blt_SetHashValue(hPtr, (char *)markerPtr);

    /* Delete the old hash entry */
    hPtr = Blt_FindHashEntry(&graphPtr->markers.nameTable, oldName);
    Blt_DeleteHashEntry(&graphPtr->markers.nameTable, hPtr);
    if (oldName != NULL) {
        Blt_Free(oldName);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeMarker --
 *
 *      Finally free the memory allocated for the marker itself.  At this
 *      point we've freed all the resources associated with the marker and
 *      marked it deleted. No one should be used the structure except to
 *      examine the delete flag.
 *
 *      With the marker removed from the marker list and hashtable, there
 *      should be no way for a command to use the marker.  The only thing
 *      to worry about are event callbacks from picked objects.  They
 *      should be checking the deleted flag.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeMarker(DestroyData data)
{
    Blt_Free(data);
}

/*
 *---------------------------------------------------------------------------
 *
 * BoxesDontOverlap --
 *
 *      Tests if the bounding box of a marker overlaps the plotting area in
 *      any way.  If so, the marker will be drawn.  Just do a min/max test
 *      on the extents of both boxes.
 *
 *      Note: It's assumed that the extents of the bounding box lie within
 *            the area.  So for a 10x10 rectangle, bottom and left would be
 *            9.
 *
 * Results:
 *      Returns 0 is the marker is visible in the plotting area, and 1
 *      otherwise (marker is off screen).
 *
 *---------------------------------------------------------------------------
 */
static int
BoxesDontOverlap(Graph *graphPtr, Region2d *extsPtr)
{
    assert(extsPtr->right >= extsPtr->left);
    assert(extsPtr->bottom >= extsPtr->top);
    assert(graphPtr->x2 >= graphPtr->x1);
    assert(graphPtr->y2 >= graphPtr->y1);

    return (((double)graphPtr->x2 < extsPtr->left) ||
            ((double)graphPtr->y2 < extsPtr->top) ||
            (extsPtr->right < (double)graphPtr->x1) ||
            (extsPtr->bottom < (double)graphPtr->y1));
}


/*
 *---------------------------------------------------------------------------
 *
 * GetCoordinate --
 *
 *      Convert the expression string into a floating point value. The *
 *      only reason we use this routine instead of Blt_ExprDouble is to *
 *      handle "elastic" bounds.  That is, convert the strings "-Inf", *
 *      "Inf" into -(DBL_MAX) and DBL_MAX respectively.
 *
 * Results:
 *      The return value is a standard TCL result.  The value of the
 *      expression is passed back via valuePtr.
 *
 *---------------------------------------------------------------------------
 */
static int
GetCoordinate(
    Tcl_Interp *interp,                 /* Interpreter to return results */
    Tcl_Obj *objPtr,                    /* Numeric expression string to
                                         * parse */
    double *valuePtr)                   /* Real-valued result of
                                         * expression */
{
    char c;
    const char *expr;
    
    expr = Tcl_GetString(objPtr);
    c = expr[0];
    if ((c == 'I') && (strcmp(expr, "Inf") == 0)) {
        *valuePtr = DBL_MAX;            /* Elastic upper bound */
    } else if ((c == '-') && (expr[1] == 'I') && (strcmp(expr, "-Inf") == 0)) {
        *valuePtr = -DBL_MAX;           /* Elastic lower bound */
    } else if ((c == '+') && (expr[1] == 'I') && (strcmp(expr, "+Inf") == 0)) {
        *valuePtr = DBL_MAX;            /* Elastic upper bound */
    } else if (Blt_GetDoubleFromObj(interp, objPtr, valuePtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * PrintCoordinate --
 *
 *      Convert the floating point value into its string representation.
 *      The only reason this routine is used in instead of sprintf, is to
 *      handle the "elastic" bounds.  That is, convert the values DBL_MAX
 *      and -(DBL_MAX) into "+Inf" and "-Inf" respectively.
 *
 * Results:
 *      The return value is a standard TCL result.  The string of the
 *      expression is passed back via string.
 *
 *-------------------------------------------------------------------------- 
 */
static Tcl_Obj *
PrintCoordinate(double x)
{
    if (x == DBL_MAX) {
        return Tcl_NewStringObj("+Inf", -1);
    } else if (x == -DBL_MAX) {
        return Tcl_NewStringObj("-Inf", -1);
    } else {
        return Tcl_NewDoubleObj(x);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseCoordinates --
 *
 *      The TCL coordinate list is converted to their floating point
 *      values. It will then replace the current marker coordinates.
 *
 *      Since different marker types require different number of
 *      coordinates this must be checked here.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 * Side effects:
 *      If the marker coordinates are reset, the graph is eventually
 *      redrawn with at the new marker coordinates.
 *
 *---------------------------------------------------------------------------
 */
static int
ParseCoordinates(Tcl_Interp *interp, Marker *markerPtr, int objc,
                 Tcl_Obj *const *objv)
{
    int numWorldPts;
    int minArgs, maxArgs;
    Point2d *worldPts;
    int i;

    if (objc == 0) {
        return TCL_OK;
    }
    if (objc & 1) {
        Tcl_AppendResult(interp, "odd number of marker coordinates specified",
            (char *)NULL);
        return TCL_ERROR;
    }
    switch (markerPtr->obj.classId) {
    case CID_MARKER_LINE:
        minArgs = 4, maxArgs = 0;
        break;
    case CID_MARKER_POLYGON:
        minArgs = 6, maxArgs = 0;
        break;
    case CID_MARKER_RECTANGLE:
        minArgs = 4, maxArgs = 4;
        break;
    case CID_MARKER_WINDOW:
    case CID_MARKER_TEXT:
        minArgs = 2, maxArgs = 2;
        break;
    case CID_MARKER_IMAGE:
    case CID_MARKER_BITMAP:
        minArgs = 2, maxArgs = 4;
        break;
    default:
        Tcl_AppendResult(interp, "unknown marker type", (char *)NULL);
        return TCL_ERROR;
    }

    if (objc < minArgs) {
        Tcl_AppendResult(interp, "too few marker coordinates specified",
            (char *)NULL);
        return TCL_ERROR;
    }
    if ((maxArgs > 0) && (objc > maxArgs)) {
        Tcl_AppendResult(interp, "too many marker coordinates specified",
            (char *)NULL);
        return TCL_ERROR;
    }
    numWorldPts = objc / 2;
    worldPts = Blt_Malloc(numWorldPts * sizeof(Point2d));
    if (worldPts == NULL) {
        Tcl_AppendResult(interp, "can't allocate new coordinate array",
            (char *)NULL);
        return TCL_ERROR;
    }

    {
        Point2d *pp;

        pp = worldPts;
        for (i = 0; i < objc; i += 2) {
            double x, y;
            
            if ((GetCoordinate(interp, objv[i], &x) != TCL_OK) ||
                (GetCoordinate(interp, objv[i + 1], &y) != TCL_OK)) {
                Blt_Free(worldPts);
                return TCL_ERROR;
            }
            pp->x = x, pp->y = y, pp++;
        }
    }
    /* Don't free the old coordinate array until we've parsed the new
     * coordinates without errors.  */
    if (markerPtr->worldPts != NULL) {
        Blt_Free(markerPtr->worldPts);
    }
    markerPtr->worldPts = worldPts;
    markerPtr->numWorldPts = numWorldPts;
    markerPtr->flags |= MAP_ITEM;
    return TCL_OK;
}

/*ARGSUSED*/
static void
CoordsFreeProc(ClientData clientData, Display *display, char *widgRec,
               int offset)
{
    Marker *markerPtr = (Marker *)widgRec;
    Point2d **pointsPtr = (Point2d **)(widgRec + offset);

    if (*pointsPtr != NULL) {
        Blt_Free(*pointsPtr);
        *pointsPtr = NULL;
    }
    markerPtr->numWorldPts = 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToCoordsProc --
 *
 *      Given a TCL list of numeric expression representing the element
 *      values, convert into an array of floating point values. In
 *      addition, the minimum and maximum values are saved.  Since elastic
 *      values are allow (values which translate to the min/max of the
 *      graph), we must try to get the non-elastic minimum and maximum.
 *
 * Results:
 *      The return value is a standard TCL result.  The vector is passed
 *      back via the vecPtr.
 *
 * -------------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
ObjToCoordsProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Marker *markerPtr = (Marker *)widgRec;
    Tcl_Obj **objv;
    int objc;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 0) {
        return TCL_OK;
    }
    return ParseCoordinates(interp, markerPtr, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * CoordsToObjProc --
 *
 *      Convert the vector of floating point values into a TCL list.
 *
 * Results:
 *      The string representation of the vector is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
CoordsToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                char *widgRec, int offset, int flags)
{
    Marker *markerPtr = (Marker *)widgRec;
    Tcl_Obj *listObjPtr;
    Point2d *pp, *pend;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (pp = markerPtr->worldPts, pend = pp + markerPtr->numWorldPts; 
        pp < pend; pp++) {
        Tcl_ListObjAppendElement(interp, listObjPtr, PrintCoordinate(pp->x));
        Tcl_ListObjAppendElement(interp, listObjPtr, PrintCoordinate(pp->y));
    }
    return listObjPtr;
}

/*LINTLIBRARY*/
static int
GetColorPair(Tcl_Interp *interp, Tk_Window tkwin, Tcl_Obj *fgObjPtr,
             Tcl_Obj *bgObjPtr, ColorPair *pairPtr, int allowDefault)
{
    XColor *fgColor, *bgColor;
    const char *string;

    fgColor = bgColor = NULL;
    if (fgObjPtr != NULL) {
        int length;

        string = Tcl_GetStringFromObj(fgObjPtr, &length);
        if (string[0] == '\0') {
            fgColor = NULL;
        } else if ((allowDefault) && (string[0] == 'd') &&
                   (strncmp(string, "defcolor", length) == 0)) {
            fgColor = COLOR_DEFAULT;
        } else {
            fgColor = Tk_AllocColorFromObj(interp, tkwin, fgObjPtr);
            if (fgColor == NULL) {
                return TCL_ERROR;
            }
        }
    }
    if (bgObjPtr != NULL) {
        int length;

        string = Tcl_GetStringFromObj(bgObjPtr, &length);
        if (string[0] == '\0') {
            bgColor = NULL;
        } else if ((allowDefault) && (string[0] == 'd') &&
                   (strncmp(string, "defcolor", length) == 0)) {
            bgColor = COLOR_DEFAULT;
        } else {
            bgColor = Tk_AllocColorFromObj(interp, tkwin, bgObjPtr);
            if (bgColor == NULL) {
                return TCL_ERROR;
            }
        }
    }
    if (pairPtr->fgColor != NULL) {
        Tk_FreeColor(pairPtr->fgColor);
    }
    if (pairPtr->bgColor != NULL) {
        Tk_FreeColor(pairPtr->bgColor);
    }
    pairPtr->fgColor = fgColor;
    pairPtr->bgColor = bgColor;
    return TCL_OK;
}

void
Blt_FreeColorPair(ColorPair *pairPtr)
{
    if ((pairPtr->bgColor != NULL) && (pairPtr->bgColor != COLOR_DEFAULT)) {
        Tk_FreeColor(pairPtr->bgColor);
    }
    if ((pairPtr->fgColor != NULL) && (pairPtr->fgColor != COLOR_DEFAULT)) {
        Tk_FreeColor(pairPtr->fgColor);
    }
    pairPtr->bgColor = pairPtr->fgColor = NULL;
}

static void
ColorPairFreeProc(ClientData clientData, Display *display, char *widgRec,
                  int offset)
{
    ColorPair *pairPtr = (ColorPair *)(widgRec + offset);

    Blt_FreeColorPair(pairPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToColorPairProc --
 *
 *      Convert the color names into pair of XColor pointers.
 *
 * Results:
 *      A standard TCL result.  The color pointer is written into the
 *      widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColorPairProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                   Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    ColorPair *pairPtr = (ColorPair *)(widgRec + offset);
    Tcl_Obj **objv;
    int state;
    int objc;
    long longValue = (long)clientData;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc > 2) {
        Tcl_AppendResult(interp, "too many names in colors list", 
                (char *)NULL);
        return TCL_ERROR;
    }
    if (objc == 0) {
        Blt_FreeColorPair(pairPtr);
        return TCL_OK;
    }
    state = (int)longValue;
    if (objc == 1) {
        if (GetColorPair(interp, tkwin, objv[0], NULL, pairPtr, state) 
            != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        if (GetColorPair(interp, tkwin, objv[0], objv[1], pairPtr, state) 
            != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NameOfColor --
 *
 *      Convert the color option value into a string.
 *
 * Results:
 *      The static string representing the color option is returned.
 *
 *---------------------------------------------------------------------------
 */
static const char *
NameOfColor(XColor *colorPtr)
{
    if (colorPtr == NULL) {
        return "";
    } else if (colorPtr == COLOR_DEFAULT) {
        return "defcolor";
    } else {
        return Tk_NameOfColor(colorPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorPairToObjProc --
 *
 *      Convert the color pairs into color names.
 *
 * Results:
 *      The string representing the symbol color is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ColorPairToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                   char *widgRec, int offset, int flags)
{
    ColorPair *pairPtr = (ColorPair *)(widgRec + offset);
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    objPtr = Tcl_NewStringObj(NameOfColor(pairPtr->fgColor), -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewStringObj(NameOfColor(pairPtr->bgColor), -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageChangedProc
 *
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ImageChangedProc(ClientData clientData, int x, int y, int w, int h,
                 int imageWidth, int imageHeight)       
{
    Graph *graphPtr;
    ImageMarker *imPtr = clientData;
    int isPhoto;

    graphPtr = imPtr->obj.graphPtr;
    if ((imPtr->picture != NULL) && (imPtr->flags & IMAGE_PHOTO)) {
        Blt_FreePicture(imPtr->picture);
    }
    imPtr->picture = NULL;
    imPtr->flags &= ~IMAGE_PHOTO;
    if (Blt_Image_IsDeleted(imPtr->tkImage)) {
        Tk_FreeImage(imPtr->tkImage);
        imPtr->tkImage = NULL;
        return;
    }
    imPtr->picture = Blt_GetPictureFromImage(graphPtr->interp, imPtr->tkImage, 
                &isPhoto);
    if (isPhoto) {
        imPtr->flags |= IMAGE_PHOTO;
    }
    graphPtr->flags |= CACHE_DIRTY;
    imPtr->flags |= MAP_ITEM;
    Blt_EventuallyRedrawGraph(graphPtr);
}

/*ARGSUSED*/
static void
PictImageFreeProc(ClientData clientData, Display *display, char *widgRec,
                  int offset)
{
    ImageMarker *imPtr = (ImageMarker *)widgRec;

    if ((imPtr->picture != NULL) && (imPtr->flags & IMAGE_PHOTO)) {
        Blt_FreePicture(imPtr->picture);
    }
    imPtr->picture = NULL;
    if (imPtr->tkImage != NULL) {
        Tk_FreeImage(imPtr->tkImage);
    }
    imPtr->tkImage = NULL;
    imPtr->flags &= ~IMAGE_PHOTO;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPictImageProc --
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
ObjToPictImageProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                   Tcl_Obj *objPtr, char *widgRec, int offset, int flags)       
{
    Blt_Picture *picturePtr = (Blt_Picture *)(widgRec + offset);
    Graph *graphPtr;
    ImageMarker *imPtr = (ImageMarker *)widgRec;
    Tk_Image tkImage;
    const char *name;
    int isPhoto;

    name = Tcl_GetString(objPtr);
    tkImage = Tk_GetImage(interp, tkwin, name, ImageChangedProc, imPtr);
    if (tkImage == NULL) {
        return TCL_ERROR;
    }
    if ((*picturePtr != NULL) && (imPtr->flags & IMAGE_PHOTO)) {
        Blt_FreePicture(*picturePtr);
    }
    if (imPtr->tkImage != NULL) {
        Tk_FreeImage(imPtr->tkImage);
    }
    imPtr->flags &= ~IMAGE_PHOTO;
    *picturePtr = NULL;
    imPtr->tkImage = tkImage;
    graphPtr = imPtr->obj.graphPtr;
    *picturePtr = Blt_GetPictureFromImage(graphPtr->interp, tkImage, &isPhoto);
    if (isPhoto) {
        imPtr->flags |= IMAGE_PHOTO;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PictImageToObjProc --
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
PictImageToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                   char *widgRec, int offset, int flags)        
{
    ImageMarker *imPtr = (ImageMarker *)(widgRec);
    
    if (imPtr->tkImage == NULL) {
        return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(Blt_Image_Name(imPtr->tkImage), -1);
}

static INLINE int
IsElementHidden(Marker *markerPtr)
{
    Blt_HashEntry *hPtr;
    Graph *graphPtr = markerPtr->obj.graphPtr;

    /* Look up the named element and see if it's hidden */
    hPtr = Blt_FindHashEntry(&graphPtr->elements.nameTable, 
        markerPtr->elemName);
    if (hPtr != NULL) {
        Element *elemPtr;
        
        elemPtr = Blt_GetHashValue(hPtr);
        if ((elemPtr->link == NULL) || (elemPtr->flags & HIDDEN)) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetTag --
 *
 *      Associates a tag with a given row.  Individual row tags are stored
 *      in hash tables keyed by the tag name.  Each table is in turn stored
 *      in a hash table keyed by the row location.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      A tag is stored for a particular marker.
 *
 *---------------------------------------------------------------------------
 */
static int
SetTag(Tcl_Interp *interp, Marker *markerPtr, const char *tagName)
{
    Graph *graphPtr;
    long dummy;
    
    if (strcmp(tagName, "all") == 0) {
        return TCL_OK;                  /* Don't need to create reserved
                                         * tag. */
    }
    if (tagName[0] == '\0') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tagName, "\" can't be empty.", 
                (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (tagName[0] == '-') {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tagName, 
                "\" can't start with a '-'.", (char *)NULL);
        }
        return TCL_ERROR;
    }
    if (Blt_GetLong(NULL, (char *)tagName, &dummy) == TCL_OK) {
        if (interp != NULL) {
            Tcl_AppendResult(interp, "tag \"", tagName, "\" can't be a number.",
                             (char *)NULL);
        }
        return TCL_ERROR;
    }
    graphPtr = markerPtr->obj.graphPtr;
    Blt_Tags_AddItemToTag(&graphPtr->markers.tags, tagName, markerPtr);
    return TCL_OK;
}

/*ARGSUSED*/
static void
FreeTagsProc(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Graph *graphPtr;
    Marker *markerPtr = (Marker *)widgRec;

    graphPtr = markerPtr->obj.graphPtr;
    Blt_Tags_ClearTagsFromItem(&graphPtr->markers.tags, markerPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTagsProc --
 *
 *      Convert the string representation of a list of tags.
 *
 * Results:
 *      The return value is a standard TCL result.  The tags are
 *      save in the widget.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTagsProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin, 
              Tcl_Obj *objPtr, char *widgRec, int offset, int flags)  
{
    Graph *graphPtr;
    Marker *markerPtr = (Marker *)widgRec;
    int i;
    const char *string;
    int objc;
    Tcl_Obj **objv;

    graphPtr = markerPtr->obj.graphPtr;
    Blt_Tags_ClearTagsFromItem(&graphPtr->markers.tags, markerPtr);
    string = Tcl_GetString(objPtr);
    if ((string[0] == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
        return TCL_OK;
    }
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
        SetTag(interp, markerPtr, Tcl_GetString(objv[i]));
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagsToObjProc --
 *
 *      Returns the tags associated with the marker.
 *
 * Results:
 *      The names representing the tags are returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TagsToObjProc(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              char *widgRec, int offset, int flags)  
{
    Graph *graphPtr;
    Marker *markerPtr = (Marker *)widgRec;
    Tcl_Obj *listObjPtr;

    graphPtr = markerPtr->obj.graphPtr;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Blt_Tags_AppendTagsToObj(&graphPtr->markers.tags,  markerPtr, listObjPtr);
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * HMap --
 *
 *      Maps the given graph coordinate value to its axis, returning a
 *      window position.  This is a slight variation on the normal Blt_HMap
 *      routine.  It treats -Inf as the minimum axis value and Inf as the
 *      maximum.
 *
 * Results:
 *      Returns a floating point number representing the window coordinate
 *      position on the given axis.
 *
 * -------------------------------------------------------------------------- 
 */
static double
HMap(Axis *axisPtr, double x)
{
    if (x == DBL_MAX) {
        x = 1.0;
    } else if (x == -DBL_MAX) {
        x = 0.0;
    } else {
        if (IsLogScale(axisPtr)) {
            if (x > 0.0) {
                x = log10(x);
            } else if (x < 0.0) {
                x = 0.0;
            }
        }
        x = NORMALIZE(axisPtr, x);
    }
    if (axisPtr->decreasing) {
        x = 1.0 - x;
    }
    /* Horizontal transformation */
    return (x * axisPtr->screenRange + axisPtr->screenMin);
}

/*
 *---------------------------------------------------------------------------
 *
 * VMap --
 *
 *      Map the given graph coordinate value to its axis, returning a
 *      window position.  This is a slight variation on the normal Blt_VMap
 *      routine.  It treats -Inf as the minimum axis value and Inf as the
 *      maximum.
 *
 * Results:
 *      Returns a double precision number representing the window
 *      coordinate position on the given axis.
 *
 *---------------------------------------------------------------------------
 */
static double
VMap(Axis *axisPtr, double y)
{
    if (y == DBL_MAX) {
        y = 1.0;
    } else if (y == -DBL_MAX) {
        y = 0.0;
    } else {
        if (IsLogScale(axisPtr)) {
            if (y > 0.0) {
                y = log10(y);
            } else if (y < 0.0) {
                y = 0.0;
            }
        }
        y = NORMALIZE(axisPtr, y);
    }
    if (axisPtr->decreasing) {
        y = 1.0 - y;
    }
    /* Vertical transformation. */
    return (((1.0 - y) * axisPtr->screenRange) + axisPtr->screenMin);
}

/*
 *---------------------------------------------------------------------------
 *
 * GraphExtents --
 *
 *      Generates a bounding box representing the plotting area of the
 *      graph. This data structure is used to clip the points and line
 *      segments of markers.
 *
 *      The clip region is the plotting area plus such arbitrary extra
 *      space.  The reason we clip with a bounding box larger than the plot
 *      area is so that symbols will be drawn even if their center point
 *      isn't in the plotting area.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The bounding box is filled with the dimensions of the plotting
 *      area.
 *
 *---------------------------------------------------------------------------
 */
static void
GraphExtents(Marker *markerPtr, Region2d *r)
{
    Graph *graphPtr;
    Axis *x, *y;

    graphPtr = markerPtr->obj.graphPtr;
    if (graphPtr->flags & INVERTED) {
        y = markerPtr->axes.x;
        x = markerPtr->axes.y;
    } else {
        x = markerPtr->axes.x;
        y = markerPtr->axes.y;
    }
    r->left   = (double)x->screenMin;
    r->top    = (double)y->screenMin;
    r->right  = (double)(x->screenMin + x->screenRange);
    r->bottom = (double)(y->screenMin + y->screenRange);
}

/*
 *---------------------------------------------------------------------------
 *
 * MapPoint --
 *
 *      Maps the given graph x,y coordinate values to a window position.
 *
 * Results:
 *      Returns a XPoint structure containing the window coordinates of the
 *      given graph x,y coordinate.
 *
 *---------------------------------------------------------------------------
 */
static Point2d
MapPoint(
    Point2d *pointPtr,                  /* Graph X-Y coordinate. */
    Axis2d *axesPtr)                    /* Specifies which axes to use */
{
    Point2d result;
    Graph *graphPtr = axesPtr->y->obj.graphPtr;

    if (graphPtr->flags & INVERTED) {
        result.x = HMap(axesPtr->y, pointPtr->y);
        result.y = VMap(axesPtr->x, pointPtr->x);
    } else {
        result.x = HMap(axesPtr->x, pointPtr->x);
        result.y = VMap(axesPtr->y, pointPtr->y);
    }
    return result;                      /* Result is screen coordinate. */
}

static Marker *
CreateMarker(Graph *graphPtr, const char *name, ClassId classId)
{    
    Marker *markerPtr;

    /* Create the new marker based upon the given type */
    switch (classId) {
    case CID_MARKER_BITMAP:
        markerPtr = BitmapCreateProc();         break;
    case CID_MARKER_LINE:
        markerPtr = LineCreateProc();           break;
    case CID_MARKER_IMAGE:
        markerPtr = ImageCreateProc();          break;
    case CID_MARKER_TEXT:
        markerPtr = TextCreateProc();           break;
    case CID_MARKER_POLYGON:
        markerPtr = PolygonCreateProc();        break;
    case CID_MARKER_RECTANGLE:
        markerPtr = RectangleCreateProc();      break;
    case CID_MARKER_WINDOW:
        markerPtr = WindowCreateProc();         break;
    default:
        return NULL;
    }
    markerPtr->obj.graphPtr = graphPtr;
    markerPtr->drawUnder = FALSE;
    markerPtr->flags |= MAP_ITEM;
    markerPtr->obj.name = Blt_AssertStrdup(name);
    Blt_GraphSetObjectClass(&markerPtr->obj, classId);
    return markerPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * DestroyMarker --
 *
 *      Free the resources allocated for the marker but not the marker
 *      itself.  Also mark it as deleted. No one should be using the
 *      structure except to examine the delete flag.
 *
 *      When the marker is removed from the marker list and hashtable,
 *      there will be no way to select the marker from a command.  The only
 *      thing to worry about are event callbacks from picked objects and
 *      they should be checking the deleted flag.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyMarker(Marker *markerPtr)
{
    Graph *graphPtr = markerPtr->obj.graphPtr;

    markerPtr->obj.deleted = TRUE;      /* Mark it as deleted. */

    if (markerPtr->drawUnder) {
        /* If the marker to be deleted is currently displayed below the
         * elements, then backing store needs to be repaired. */
        graphPtr->flags |= CACHE_DIRTY;
    }
    Blt_Tags_ClearTagsFromItem(&graphPtr->markers.tags, markerPtr);
    /* 
     * Call the marker's type-specific deallocation routine. We do it first
     * while all the marker fields are still valid.
     */
    (*markerPtr->classPtr->freeProc)(markerPtr);

    /* Dump any bindings that might be registered for the marker. */
    Blt_DeleteBindings(graphPtr->bindTable, markerPtr);

    /* Release all the X resources associated with the marker. */
    Blt_FreeOptions(markerPtr->classPtr->configSpecs, (char *)markerPtr,
        graphPtr->display, 0);

    if (markerPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&graphPtr->markers.nameTable, 
                            markerPtr->hashPtr);
    }
    if (markerPtr->link != NULL) {
        Blt_Chain_DeleteLink(graphPtr->markers.displayList, markerPtr->link);
    }
    if (markerPtr->obj.name != NULL) {
        Blt_Free(markerPtr->obj.name);
    }
    Tcl_EventuallyFree(markerPtr, FreeMarker);
}

/*
 *---------------------------------------------------------------------------
 *
 * BitmapConfigureProc --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) a bitmap
 *      marker.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as bitmap pixmap, colors, rotation,
 *      etc. get set for markerPtr; old resources get freed, if there were
 *      any.  The marker is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
BitmapConfigureProc(Marker *markerPtr)
{
    Graph *graphPtr = markerPtr->obj.graphPtr;
    BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    if (bmPtr->srcBitmap == None) {
        return TCL_OK;
    }
    bmPtr->angle = FMOD(bmPtr->reqAngle, 360.0f);
    if (bmPtr->angle < 0.0) {
        bmPtr->angle += 360.0;
    }
    gcMask = 0;

    if (bmPtr->outlineColor != NULL) {
        gcMask |= GCForeground;
        gcValues.foreground = bmPtr->outlineColor->pixel;
    }

    if (bmPtr->fillColor != NULL) {
        /* Opaque bitmap: both foreground and background (fill) colors are
         * used. */
        gcValues.background = bmPtr->fillColor->pixel;
        gcMask |= GCBackground;
    } else {
        /* Transparent bitmap: set the clip mask to the current bitmap. */
        gcValues.clip_mask = bmPtr->srcBitmap;
        gcMask |= GCClipMask;
    }

    /* 
     * This is technically a shared GC, but we're going to set/change the
     * clip origin anyways before we draw the bitmap.  This relies on the
     * fact that no other client will be allocated this GC with the
     * GCClipMask set to this particular bitmap.
     */
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (bmPtr->gc != NULL) {
        Tk_FreeGC(graphPtr->display, bmPtr->gc);
    }
    bmPtr->gc = newGC;

    /* Create the background GC containing the fill color. */

    if (bmPtr->fillColor != NULL) {
        gcValues.foreground = bmPtr->fillColor->pixel;
        newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
        if (bmPtr->fillGC != NULL) {
            Tk_FreeGC(graphPtr->display, bmPtr->fillGC);
        }
        bmPtr->fillGC = newGC;
    }

    markerPtr->flags |= MAP_ITEM;
    if (markerPtr->drawUnder) {
        graphPtr->flags |= CACHE_DIRTY;
    }

    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

#ifdef notdef
static void
PrintPolyPoint(char *mesg, Point2d *points, int numPoints)
{
    int i;

    fprintf(stderr, "%s:\t\tpoint[0]=%g,%g\n", mesg, points[0].x, points[0].y);
    for (i = 1; i < numPoints; i++) {
        fprintf(stderr, "\t\tpoint[%d]=%g,%g\n", i, points[i].x, points[i].y);
    }
}       
#endif

/*
 *---------------------------------------------------------------------------
 *
 * BitmapMapProc --
 *
 *      This procedure gets called each time the layout of the graph
 *      changes.  The x, y window coordinates of the bitmap marker are
 *      saved in the marker structure.
 *
 *      Additionly, if no background color was specified, the
 *      GCTileStipXOrigin and GCTileStipYOrigin attributes are set in the
 *      private GC.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Window coordinates are saved and if no background color was set,
 *      the GC stipple origins are changed to calculated window
 *      coordinates.
 *
 *---------------------------------------------------------------------------
 */
static void
BitmapMapProc(Marker *markerPtr)
{
    BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
    Region2d extents;
    Graph *graphPtr = markerPtr->obj.graphPtr;
    Point2d anchorPt;
    Point2d corner1, corner2;
    int destWidth, destHeight;
    int srcWidth, srcHeight;
    int i;

    if (bmPtr->srcBitmap == None) {
        return;
    }
    if (bmPtr->destBitmap != None) {
        Tk_FreePixmap(graphPtr->display, bmPtr->destBitmap);
        bmPtr->destBitmap = None;
    }
    /* 
     * Collect the coordinates.  The number of coordinates will determine
     * the calculations to be made.
     * 
     *     x1 y1        A single pair of X-Y coordinates.  They represent
     *                  the anchor position of the bitmap.  
     *
     *  x1 y1 x2 y2     Two pairs of X-Y coordinates.  They represent
     *                  two opposite corners of a bounding rectangle. The
     *                  bitmap is possibly rotated and scaled to fit into
     *                  this box.
     *
     */   
    Tk_SizeOfBitmap(graphPtr->display, bmPtr->srcBitmap, &srcWidth, 
                    &srcHeight);
    corner1 = MapPoint(markerPtr->worldPts, &markerPtr->axes);
    if (markerPtr->numWorldPts > 1) {
        double hold;

        corner2 = MapPoint(markerPtr->worldPts + 1, &markerPtr->axes);
        /* Flip the corners if necessary */
        if (corner1.x > corner2.x) {
            hold = corner1.x, corner1.x = corner2.x, corner2.x = hold;
        }
        if (corner1.y > corner2.y) {
            hold = corner1.y, corner1.y = corner2.y, corner2.y = hold;
        }
    } else {
        corner2.x = corner1.x + srcWidth - 1;
        corner2.y = corner1.y + srcHeight - 1;
    }
    destWidth = (int)(corner2.x - corner1.x) + 1;
    destHeight = (int)(corner2.y - corner1.y) + 1;

    if (markerPtr->numWorldPts == 1) {
        anchorPt = Blt_AnchorPoint(corner1.x, corner1.y, (double)destWidth, 
                (double)destHeight, bmPtr->anchor);
    } else {
        anchorPt = corner1;
    }
    anchorPt.x += markerPtr->xOffset;
    anchorPt.y += markerPtr->yOffset;

    /* Check if the bitmap sits at least partially in the plot area. */
    extents.left   = anchorPt.x;
    extents.top    = anchorPt.y;
    extents.right  = anchorPt.x + destWidth - 1;
    extents.bottom = anchorPt.y + destHeight - 1;
    markerPtr->offScreen = BoxesDontOverlap(graphPtr, &extents);
    if (markerPtr->offScreen) {
        return;                         /* Bitmap is offscreen. Don't
                                         * generate rotated or scaled
                                         * bitmaps. */
    }

    /*  
     * Scale the bitmap if necessary. It's a little tricky because we only
     * want to scale what's visible on the screen, not the entire bitmap.
     */
    if ((bmPtr->angle != 0.0f) || (destWidth != srcWidth) || 
        (destHeight != srcHeight)) {
        int regionX, regionY, regionWidth, regionHeight; 
        double left, right, top, bottom;

        /* Ignore parts of the bitmap outside of the plot area. */
        left   = MAX(graphPtr->x1, extents.left);
        right  = MIN(graphPtr->x2, extents.right);
        top    = MAX(graphPtr->y1, extents.top);
        bottom = MIN(graphPtr->y2, extents.bottom);

        /* Determine the portion of the scaled bitmap to display. */
        regionX = regionY = 0;
        if (graphPtr->x1 > extents.left) {
            regionX = (int)(graphPtr->x1 - extents.left);
        }
        if (graphPtr->y1 > extents.top) {
            regionY = (int)(graphPtr->y1 - extents.top);
        }           
        regionWidth = (int)(right - left) + 1;
        regionHeight = (int)(bottom - top) + 1;
        
        anchorPt.x = left;
        anchorPt.y = top;
        bmPtr->destBitmap = Blt_ScaleRotateBitmapArea(graphPtr->tkwin, 
                bmPtr->srcBitmap, srcWidth, srcHeight, regionX, regionY, 
                regionWidth, regionHeight, destWidth, destHeight, bmPtr->angle);
        bmPtr->destWidth = regionWidth;
        bmPtr->destHeight = regionHeight;
    } else {
        bmPtr->destWidth = srcWidth;
        bmPtr->destHeight = srcHeight;
        bmPtr->destBitmap = None;
    }
    bmPtr->anchorPt = anchorPt;
    {
        double xScale, yScale;
        double tx, ty;
        double rotWidth, rotHeight;
        Point2d polygon[5];
        int n;

        /* 
         * Compute a polygon to represent the background area of the
         * bitmap.  This is needed for backgrounds of arbitrarily rotated
         * bitmaps.  We also use it to print a background in PostScript.
         */
        Blt_GetBoundingBox(srcWidth, srcHeight, bmPtr->angle, &rotWidth, 
                           &rotHeight, polygon);
        xScale = (double)destWidth / rotWidth;
        yScale = (double)destHeight / rotHeight;
        
        /* 
         * Adjust each point of the polygon. Both scale it to the new size
         * and translate it to the actual screen position of the bitmap.
         */
        tx = extents.left + destWidth * 0.5;
        ty = extents.top + destHeight * 0.5;
        for (i = 0; i < 4; i++) {
            polygon[i].x = (polygon[i].x * xScale) + tx;
            polygon[i].y = (polygon[i].y * yScale) + ty;
        }
        GraphExtents(markerPtr, &extents);
        n = Blt_PolyRectClip(&extents, polygon, 4, bmPtr->outline); 
        assert(n <= MAX_OUTLINE_POINTS);
        if (n < 3) { 
            memcpy(&bmPtr->outline, polygon, sizeof(Point2d) * 4);
            bmPtr->numOutlinePts = 4;
        } else {
            bmPtr->numOutlinePts = n;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BitmapPointProc --
 *
 *      Indicates if the given point is over the bitmap marker.  The area
 *      of the bitmap is the rectangle.
 *
 * Results:
 *      Returns 1 is the point is over the bitmap marker, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
static int
BitmapPointProc(Marker *markerPtr, Point2d *samplePtr)
{
    BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;

    if (bmPtr->srcBitmap == None) {
        return 0;
    }
    if (bmPtr->angle != 0.0f) {
        Point2d points[MAX_OUTLINE_POINTS];
        int i;

        /*  
         * Generate the bounding polygon (isolateral) for the bitmap and
         * see if the point is inside of it.
         */
        for (i = 0; i < bmPtr->numOutlinePts; i++) {
            points[i].x = bmPtr->outline[i].x + bmPtr->anchorPt.x;
            points[i].y = bmPtr->outline[i].y + bmPtr->anchorPt.y;
        }
        return Blt_PointInPolygon(samplePtr, points, bmPtr->numOutlinePts);
    }
    return ((samplePtr->x >= bmPtr->anchorPt.x) && 
            (samplePtr->x < (bmPtr->anchorPt.x + bmPtr->destWidth)) &&
            (samplePtr->y >= bmPtr->anchorPt.y) && 
            (samplePtr->y < (bmPtr->anchorPt.y + bmPtr->destHeight)));
}


/*
 *---------------------------------------------------------------------------
 *
 * BitmapAreaProc --
 *
 *---------------------------------------------------------------------------
 */
static int
BitmapAreaProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
    BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;

    if (markerPtr->numWorldPts < 1) {
        return FALSE;
    }
    if (bmPtr->angle != 0.0f) {
        Point2d points[MAX_OUTLINE_POINTS];
        int i;
        
        /*  
         * Generate the bounding polygon (isolateral) for the bitmap and
         * see if the point is inside of it.
         */
        for (i = 0; i < bmPtr->numOutlinePts; i++) {
            points[i].x = bmPtr->outline[i].x + bmPtr->anchorPt.x;
            points[i].y = bmPtr->outline[i].y + bmPtr->anchorPt.y;
        }
        return Blt_RegionInPolygon(extsPtr, points, bmPtr->numOutlinePts, 
                   enclosed);
    }
    if (enclosed) {
        return ((bmPtr->anchorPt.x >= extsPtr->left) &&
                (bmPtr->anchorPt.y >= extsPtr->top) && 
                ((bmPtr->anchorPt.x + bmPtr->destWidth) <= extsPtr->right) &&
                ((bmPtr->anchorPt.y + bmPtr->destHeight) <= extsPtr->bottom));
    }
    return !((bmPtr->anchorPt.x >= extsPtr->right) ||
             (bmPtr->anchorPt.y >= extsPtr->bottom) ||
             ((bmPtr->anchorPt.x + bmPtr->destWidth) <= extsPtr->left) ||
             ((bmPtr->anchorPt.y + bmPtr->destHeight) <= extsPtr->top));
}

/*
 *---------------------------------------------------------------------------
 *
 * BitmapDrawProc --
 *
 *      Draws the bitmap marker that have a transparent of filled background.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      GC stipple origins are changed to current window coordinates.
 *      Commands are output to X to draw the marker in its current mode.
 *
 *---------------------------------------------------------------------------
 */
static void
BitmapDrawProc(Marker *markerPtr, Drawable drawable)
{
    Graph *graphPtr = markerPtr->obj.graphPtr;
    BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
    double rangle;
    Pixmap bitmap;

    bitmap = GETBITMAP(bmPtr);
    if ((bitmap == None) || (bmPtr->destWidth < 1) || (bmPtr->destHeight < 1)) {
        return;
    }
    rangle = FMOD(bmPtr->angle, 90.0);
    if ((bmPtr->fillColor == NULL) || (rangle != 0.0)) {

        /* 
         * If the bitmap is rotated and a filled background is required,
         * then a filled polygon is drawn before the bitmap.
         */
        if (bmPtr->fillColor != NULL) {
            int i;
            XPoint polygon[MAX_OUTLINE_POINTS];

            for (i = 0; i < bmPtr->numOutlinePts; i++) {
                polygon[i].x = (short int)bmPtr->outline[i].x;
                polygon[i].y = (short int)bmPtr->outline[i].y;
            }
            XFillPolygon(graphPtr->display, drawable, bmPtr->fillGC,
                 polygon, bmPtr->numOutlinePts, Convex, CoordModeOrigin);
        }
        XSetClipMask(graphPtr->display, bmPtr->gc, bitmap);
        XSetClipOrigin(graphPtr->display, bmPtr->gc, (int)bmPtr->anchorPt.x, 
               (int)bmPtr->anchorPt.y);
    } else {
        XSetClipMask(graphPtr->display, bmPtr->gc, None);
        XSetClipOrigin(graphPtr->display, bmPtr->gc, 0, 0);
    }
    XCopyPlane(graphPtr->display, bitmap, drawable, bmPtr->gc, 0, 0,
        bmPtr->destWidth, bmPtr->destHeight, (int)bmPtr->anchorPt.x, 
        (int)bmPtr->anchorPt.y, 1);
}

/*
 *---------------------------------------------------------------------------
 *
 * BitmapPostscriptProc --
 *
 *      Generates PostScript to print a bitmap marker.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
BitmapPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
    Graph *graphPtr = markerPtr->obj.graphPtr;
    BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
    Pixmap bitmap;

    bitmap = GETBITMAP(bmPtr);
    if ((bitmap == None) || (bmPtr->destWidth < 1) || (bmPtr->destHeight < 1)) {
        return;                         /* No bitmap to display. */
    }
    if (bmPtr->fillColor != NULL) {
        Blt_Ps_XSetBackground(ps, bmPtr->fillColor);
        Blt_Ps_XFillPolygon(ps, 4, bmPtr->outline);
    }
    Blt_Ps_XSetForeground(ps, bmPtr->outlineColor);

    Blt_Ps_Format(ps,
        "  gsave\n    %g %g translate\n    %d %d scale\n", 
           bmPtr->anchorPt.x, bmPtr->anchorPt.y + bmPtr->destHeight, 
           bmPtr->destWidth, -bmPtr->destHeight);
    Blt_Ps_Format(ps, "    %d %d true [%d 0 0 %d 0 %d] {",
        bmPtr->destWidth, bmPtr->destHeight, bmPtr->destWidth, 
        -bmPtr->destHeight, bmPtr->destHeight);
    Blt_Ps_XSetBitmapData(ps, graphPtr->display, bitmap,
        bmPtr->destWidth, bmPtr->destHeight);
    Blt_Ps_VarAppend(ps, 
                     "    } imagemask\n",
                     "grestore\n", (char *)NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * BitmapFreeProc --
 *
 *      Releases the memory and attributes of the bitmap marker.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Bitmap attributes (GCs, colors, bitmap, etc) get destroyed.  Memory
 *      is released, X resources are freed, and the graph is redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
BitmapFreeProc(Marker *markerPtr)
{
    BitmapMarker *bmPtr = (BitmapMarker *)markerPtr;
    Graph *graphPtr = markerPtr->obj.graphPtr;

    if (bmPtr->gc != NULL) {
        Tk_FreeGC(graphPtr->display, bmPtr->gc);
    }
    if (bmPtr->fillGC != NULL) {
        Tk_FreeGC(graphPtr->display, bmPtr->fillGC);
    }
    if (bmPtr->destBitmap != None) {
        Tk_FreePixmap(graphPtr->display, bmPtr->destBitmap);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BitmapCreateProc --
 *
 *      Allocate memory and initialize methods for the new bitmap marker.
 *
 * Results:
 *      The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *      Memory is allocated for the bitmap marker structure.
 *
 *---------------------------------------------------------------------------
 */
static Marker *
BitmapCreateProc(void)
{
    BitmapMarker *bmPtr;

    bmPtr = Blt_AssertCalloc(1, sizeof(BitmapMarker));
    bmPtr->classPtr = &bitmapMarkerClass;
    return (Marker *)bmPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * ImageConfigureProc --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) a image
 *      marker.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as image pixmap, colors, rotation,
 *      etc. get set for markerPtr; old resources get freed, if there were
 *      any.  The marker is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ImageConfigureProc(Marker *markerPtr)
{
    ImageMarker *imPtr = (ImageMarker *)markerPtr;
    Graph *graphPtr = markerPtr->obj.graphPtr;
    Blt_Painter painter;
    GC newGC;

    newGC = Tk_GetGC(graphPtr->tkwin, 0L, (XGCValues *)NULL);
    if (imPtr->gc != NULL) {
        Tk_FreeGC(graphPtr->display, imPtr->gc);
    }
    imPtr->gc = newGC;

    painter = Blt_GetPainter(graphPtr->tkwin, 1.0);
    if (imPtr->painter != NULL) {
        Blt_FreePainter(painter);
    }
    imPtr->painter = painter;
    markerPtr->flags |= MAP_ITEM;
    if (markerPtr->drawUnder) {
        graphPtr->flags |= CACHE_DIRTY;
    }
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageMapProc --
 *
 *      This procedure gets called each time the layout of the graph
 *      changes.  The x, y window coordinates of the image marker are saved
 *      in the marker structure.
 *
 *      In addition, if no background color was specified, the
 *      GCTileStipXOrigin and GCTileStipYOrigin attributes will not set in
 *      the private GC.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Window coordinates are saved and if no background color was set,
 *      the GC stipple origins are changed to calculated window
 *      coordinates.
 *
 *---------------------------------------------------------------------------
 */
static void
ImageMapProc(Marker *markerPtr)
{
    Region2d extents;
    Graph *graphPtr;
    ImageMarker *imPtr;
    Point2d anchorPt;
    Point2d c1, c2;
    int newWidth, newHeight;
    int srcWidth, srcHeight;
    int x, y, w, h;
    int left, right, top, bottom;

    imPtr = (ImageMarker *)markerPtr;
    if (imPtr->picture == NULL) {
        return;
    }
    if (imPtr->scaled != NULL) { 
        Blt_FreePicture(imPtr->scaled);
        imPtr->scaled = NULL;
    }
    graphPtr = markerPtr->obj.graphPtr;
    c1 = MapPoint(markerPtr->worldPts, &markerPtr->axes);

    imPtr->width = srcWidth = Blt_Picture_Width(imPtr->picture);
    imPtr->height = srcHeight = Blt_Picture_Height(imPtr->picture);

    if ((srcWidth == 0) || (srcHeight == 0)) {
        markerPtr->offScreen = TRUE;
        return;                         /* Empty image. */
    }
    if (markerPtr->numWorldPts > 1) {
        double hold;

        c2 = MapPoint(markerPtr->worldPts + 1, &markerPtr->axes);
        /* Flip the corners if necessary */
        if (c1.x > c2.x) {
            hold = c1.x, c1.x = c2.x, c2.x = hold;
        }
        if (c1.y > c2.y) {
            hold = c1.y, c1.y = c2.y, c2.y = hold;
        }
    } else {
        c2.x = c1.x + srcWidth - 1;
        c2.y = c1.y + srcHeight - 1;
    }
    newWidth = (int)(c2.x - c1.x) + 1;
    newHeight = (int)(c2.y - c1.y) + 1;

    if (markerPtr->numWorldPts == 1) {
        anchorPt = Blt_AnchorPoint(c1.x, c1.y, (double)newWidth, 
                (double)newHeight, imPtr->anchor);
    } else {
        anchorPt = c1;
    }
    anchorPt.x += markerPtr->xOffset;
    anchorPt.y += markerPtr->yOffset;

    /* Check if the image sits at least partially in the plot area. */
    extents.left   = anchorPt.x;
    extents.top    = anchorPt.y;
    extents.right  = anchorPt.x + newWidth - 1;
    extents.bottom = anchorPt.y + newHeight - 1;

    markerPtr->offScreen = BoxesDontOverlap(graphPtr, &extents);
    if (markerPtr->offScreen) {
        return;                         /* Image is offscreen. Don't
                                         * generate rotated or scaled
                                         * images. */
    }

    /* Determine the extents of the subimage inside of the destination
     * image. */
    left =   MAX((int)extents.left, graphPtr->x1);
    top =    MAX((int)extents.top, graphPtr->y1);
    right =  MIN((int)extents.right, graphPtr->x2);
    bottom = MIN((int)extents.bottom, graphPtr->y2);
    
    /* Reset image location and coordinates to that of the region */
    anchorPt.x = left;
    anchorPt.y = top;
    
    x = y = 0;
    if (graphPtr->x1 > (int)extents.left) {
        x = graphPtr->x1 - (int)extents.left;
    } 
    if (graphPtr->y1 > (int)extents.top) {
        y = graphPtr->y1 - (int)extents.top;
    } 
    w  = (int)(right - left + 1);
    h = (int)(bottom - top + 1);
    
    if (markerPtr->numWorldPts > 1) {
        Blt_Picture scaled;

        scaled = Blt_ScalePictureArea(imPtr->picture, x, y, w, h, 
                                      newWidth, newHeight);
        imPtr->scaled = scaled;
        imPtr->pictX = 0;
        imPtr->pictY = 0;
    } else {
        imPtr->pictX = x;
        imPtr->pictY = y;
    }
    imPtr->width = newWidth;
    imPtr->height = newHeight;
    imPtr->anchorPt = anchorPt;
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowPointProc --
 *
 *      Indicates if the given point is over the window marker.  The area
 *      of the window is the rectangle.
 *
 * Results:
 *      Returns 1 is the point is over the window marker, 0 otherwise.
 *
 *---------------------------------------------------------------------------
 */
static int
ImagePointProc(Marker *markerPtr, Point2d *samplePtr)
{
    ImageMarker *imPtr = (ImageMarker *)markerPtr;
    double left, right, top, bottom;
    
    left = imPtr->anchorPt.x;
    right = imPtr->anchorPt.x + imPtr->width;
    top = imPtr->anchorPt.y;
    bottom = imPtr->anchorPt.y + imPtr->height;

    return ((samplePtr->x >= left) && (samplePtr->x < right) &&
            (samplePtr->y >= top) && (samplePtr->y < bottom));
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageAreaProc --
 *
 *---------------------------------------------------------------------------
 */
static int
ImageAreaProc(Marker *markerPtr, Region2d *regPtr, int enclosed)
{
    ImageMarker *imPtr = (ImageMarker *)markerPtr;

    if (markerPtr->numWorldPts > 0) {
        double left, right, top, bottom;

        left = imPtr->anchorPt.x;
        right = imPtr->anchorPt.x + imPtr->width;
        top = imPtr->anchorPt.y;
        bottom = imPtr->anchorPt.y + imPtr->height;
        if (enclosed) {
            return ((left >= regPtr->left) && (top >= regPtr->top) && 
                    (right <= regPtr->right) && (bottom <= regPtr->bottom));
        } 
        return !((left >= regPtr->right) || (top >= regPtr->bottom) ||
                 (right <= regPtr->left) || (bottom <= regPtr->top));
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageDrawProc --
 *
 *      This procedure is invoked to draw a image marker.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      GC stipple origins are changed to current window coordinates.
 *      Commands are output to X to draw the marker in its current mode.
 *
 *---------------------------------------------------------------------------
 */
static void
ImageDrawProc(Marker *markerPtr, Drawable drawable)
{
    ImageMarker *imPtr = (ImageMarker *)markerPtr;
    Blt_Picture picture;

    picture = (imPtr->scaled != NULL) ? imPtr->scaled : imPtr->picture;
    if (picture != NULL) {
        Blt_PaintPictureWithBlend(imPtr->painter, drawable, picture, 
                imPtr->pictX, imPtr->pictY, imPtr->width, imPtr->height, 
                (int)imPtr->anchorPt.x, (int)imPtr->anchorPt.y, 0);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ImagePostscriptProc --
 *
 *      This procedure is invoked to print a image marker.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
ImagePostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
    ImageMarker *imPtr = (ImageMarker *)markerPtr;
    Blt_Picture picture;

    picture = (imPtr->scaled != NULL) ? imPtr->scaled : imPtr->picture;
    if (picture != NULL) {
        Blt_Ps_DrawPicture(ps, picture, imPtr->anchorPt.x, imPtr->anchorPt.y);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageFreeProc --
 *
 *      Destroys the structure containing the attributes of the image
 *      marker.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Image attributes (GCs, colors, image, etc) get destroyed.  Memory
 *      is released, X resources are freed, and the graph is redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
ImageFreeProc(Marker *markerPtr)
{
    ImageMarker *imPtr = (ImageMarker *)markerPtr;
    Graph *graphPtr = markerPtr->obj.graphPtr;

    if (imPtr->painter != NULL) {
        Blt_FreePainter(imPtr->painter);
    }
    if (imPtr->scaled != NULL) {
        Blt_FreePicture(imPtr->scaled);
    }
    if (imPtr->gc != NULL) {
        Tk_FreeGC(graphPtr->display, imPtr->gc);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageCreateProc --
 *
 *      Allocate memory and initialize methods for the new image marker.
 *
 * Results:
 *      The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *      Memory is allocated for the image marker structure.
 *
 *---------------------------------------------------------------------------
 */
static Marker *
ImageCreateProc(void)
{
    ImageMarker *imPtr;

    imPtr = Blt_AssertCalloc(1, sizeof(ImageMarker));
    imPtr->classPtr = &imageMarkerClass;
    return (Marker *)imPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextConfigureProc --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) a text
 *      marker.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as text string, colors, font,
 *      etc. get set for markerPtr; old resources get freed, if there were
 *      any.  The marker is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
TextConfigureProc(Marker *markerPtr)
{
    Graph *graphPtr = markerPtr->obj.graphPtr;
    TextMarker *tmPtr = (TextMarker *)markerPtr;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    tmPtr->style.angle = (float)FMOD(tmPtr->style.angle, 360.0);
    if (tmPtr->style.angle < 0.0f) {
        tmPtr->style.angle += 360.0f;
    }
    newGC = NULL;
    if (tmPtr->fillColor != NULL) {
        gcMask = GCForeground;
        gcValues.foreground = tmPtr->fillColor->pixel;
        newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    }
    if (tmPtr->fillGC != NULL) {
        Tk_FreeGC(graphPtr->display, tmPtr->fillGC);
    }
    tmPtr->fillGC = newGC;

    markerPtr->flags |= MAP_ITEM;
    if (markerPtr->drawUnder) {
        graphPtr->flags |= CACHE_DIRTY;
    }
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextMapProc --
 *
 *      Calculate the layout position for a text marker.  Positional
 *      information is saved in the marker.  If the text is rotated, a
 *      bitmap containing the text is created.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      If no background color has been specified, the GC stipple origins
 *      are changed to current window coordinates. For both rotated and
 *      non-rotated text, if any old bitmap is leftover, it is freed.
 *
 *---------------------------------------------------------------------------
 */
static void
TextMapProc(Marker *markerPtr)
{
    Graph *graphPtr = markerPtr->obj.graphPtr;
    TextMarker *tmPtr = (TextMarker *)markerPtr;
    Region2d extents;
    Point2d anchorPt;
    int i;
    unsigned int w, h;
    double rw, rh;

    tmPtr->width = tmPtr->height = 0;
    if (tmPtr->string == NULL) {
        return;
    }
    Blt_Ts_GetExtents(&tmPtr->style, tmPtr->string, &w, &h);
    Blt_GetBoundingBox(w, h, tmPtr->style.angle, &rw, &rh, tmPtr->outline);
    tmPtr->width = ROUND(rw);
    tmPtr->height = ROUND(rh);
    for (i = 0; i < 4; i++) {
        tmPtr->outline[i].x += ROUND(rw * 0.5);
        tmPtr->outline[i].y += ROUND(rh * 0.5);
    }
    tmPtr->outline[4].x = tmPtr->outline[0].x;
    tmPtr->outline[4].y = tmPtr->outline[0].y;
    anchorPt = MapPoint(markerPtr->worldPts, &markerPtr->axes);
    anchorPt = Blt_AnchorPoint(anchorPt.x, anchorPt.y, (double)(tmPtr->width), 
        (double)(tmPtr->height), tmPtr->anchor);
    anchorPt.x += markerPtr->xOffset;
    anchorPt.y += markerPtr->yOffset;
    /*
     * Determine the bounding box of the text and test to see if it is at
     * least partially contained within the plotting area.
     */
    extents.left = anchorPt.x;
    extents.top = anchorPt.y;
    extents.right = anchorPt.x + tmPtr->width - 1;
    extents.bottom = anchorPt.y + tmPtr->height - 1;
    markerPtr->offScreen = BoxesDontOverlap(graphPtr, &extents);
    tmPtr->anchorPt = anchorPt;

}

static int
TextPointProc(Marker *markerPtr, Point2d *samplePtr)
{
    TextMarker *tmPtr = (TextMarker *)markerPtr;

    if (tmPtr->string == NULL) {
        return 0;
    }
    if (tmPtr->style.angle != 0.0f) {
        Point2d points[5];
        int i;

        /* 
         * Figure out the bounding polygon (isolateral) for the text and
         * see if the point is inside of it.
         */
        for (i = 0; i < 5; i++) {
            points[i].x = tmPtr->outline[i].x + tmPtr->anchorPt.x;
            points[i].y = tmPtr->outline[i].y + tmPtr->anchorPt.y;
        }
        return Blt_PointInPolygon(samplePtr, points, 5);
    } 
    return ((samplePtr->x >= tmPtr->anchorPt.x) && 
            (samplePtr->x < (tmPtr->anchorPt.x + tmPtr->width)) &&
            (samplePtr->y >= tmPtr->anchorPt.y) && 
            (samplePtr->y < (tmPtr->anchorPt.y + tmPtr->height)));
}

/*
 *---------------------------------------------------------------------------
 *
 * TextAreaProc --
 *
 *---------------------------------------------------------------------------
 */
static int
TextAreaProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
    TextMarker *tmPtr = (TextMarker *)markerPtr;

    if (markerPtr->numWorldPts < 1) {
        return FALSE;
    }
    if (tmPtr->style.angle != 0.0f) {
        Point2d points[5];
        int i;
        
        /*  
         * Generate the bounding polygon (isolateral) for the bitmap and
         * see if the point is inside of it.
         */
        for (i = 0; i < 4; i++) {
            points[i].x = tmPtr->outline[i].x + tmPtr->anchorPt.x;
            points[i].y = tmPtr->outline[i].y + tmPtr->anchorPt.y;
        }
        return Blt_RegionInPolygon(extsPtr, points, 4, enclosed);
    } 
    if (enclosed) {
        return ((tmPtr->anchorPt.x >= extsPtr->left) &&
                (tmPtr->anchorPt.y >= extsPtr->top) && 
                ((tmPtr->anchorPt.x + tmPtr->width) <= extsPtr->right) &&
                ((tmPtr->anchorPt.y + tmPtr->height) <= extsPtr->bottom));
    }
    return !((tmPtr->anchorPt.x >= extsPtr->right) ||
             (tmPtr->anchorPt.y >= extsPtr->bottom) ||
             ((tmPtr->anchorPt.x + tmPtr->width) <= extsPtr->left) ||
             ((tmPtr->anchorPt.y + tmPtr->height) <= extsPtr->top));
}

/*
 *---------------------------------------------------------------------------
 *
 * TextDrawProc --
 *
 *      Draws the text marker on the graph.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Commands are output to X to draw the marker in its current mode.
 *
 *---------------------------------------------------------------------------
 */
static void
TextDrawProc(Marker *markerPtr, Drawable drawable) 
{
    TextMarker *tmPtr = (TextMarker *)markerPtr;
    Graph *graphPtr = markerPtr->obj.graphPtr;

    if (tmPtr->string == NULL) {
        return;
    }
    if (tmPtr->fillGC != NULL) {
        XPoint points[4];
        int i;

        /*
         * Simulate the rotated background of the bitmap by filling a
         * bounding polygon with the background color.
         */
        for (i = 0; i < 4; i++) {
            points[i].x = (short int)(tmPtr->outline[i].x + tmPtr->anchorPt.x);
            points[i].y = (short int)(tmPtr->outline[i].y + tmPtr->anchorPt.y);
        }
        XFillPolygon(graphPtr->display, drawable, tmPtr->fillGC, points, 4,
            Convex, CoordModeOrigin);
    }
    if (tmPtr->style.color != NULL) {
        Blt_Ts_DrawText(graphPtr->tkwin, drawable, tmPtr->string, -1,
            &tmPtr->style, (int)tmPtr->anchorPt.x, (int)tmPtr->anchorPt.y);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TextPostscriptProc --
 *
 *      Outputs PostScript commands to draw a text marker at a given x,y
 *      coordinate, rotation, anchor, and font.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      PostScript font and color settings are changed.
 *
 *---------------------------------------------------------------------------
 */
static void
TextPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
    TextMarker *tmPtr = (TextMarker *)markerPtr;

    if (tmPtr->string == NULL) {
        return;
    }
    if (tmPtr->fillGC != NULL) {
        Point2d points[4];
        int i;

        /*
         * Simulate the rotated background of the bitmap by filling a
         * bounding polygon with the background color.
         */
        for (i = 0; i < 4; i++) {
            points[i].x = tmPtr->outline[i].x + tmPtr->anchorPt.x;
            points[i].y = tmPtr->outline[i].y + tmPtr->anchorPt.y;
        }
        Blt_Ps_XSetBackground(ps, tmPtr->fillColor);
        Blt_Ps_XFillPolygon(ps, 4, points);
    }
    Blt_Ps_DrawText(ps, tmPtr->string, &tmPtr->style, tmPtr->anchorPt.x, 
        tmPtr->anchorPt.y);
}

/*
 *---------------------------------------------------------------------------
 *
 * TextFreeProc --
 *
 *      Destroys the structure containing the attributes of the text
 *      marker.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Text attributes (GCs, colors, stipple, font, etc) get destroyed.
 *      Memory is released, X resources are freed, and the graph is
 *      redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
TextFreeProc(Marker *markerPtr)
{
    TextMarker *tmPtr = (TextMarker *)markerPtr;
    Graph *graphPtr = markerPtr->obj.graphPtr;

    Blt_Ts_FreeStyle(graphPtr->display, &tmPtr->style);
}

/*
 *---------------------------------------------------------------------------

 * TextCreateProc --
 *
 *      Allocate memory and initialize methods for the new text marker.
 *
 * Results:
 *      The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *      Memory is allocated for the text marker structure.
 *
 *---------------------------------------------------------------------------
 */
static Marker *
TextCreateProc(void)
{
    TextMarker *tmPtr;

    tmPtr = Blt_AssertCalloc(1, sizeof(TextMarker));
    tmPtr->classPtr = &textMarkerClass;
    Blt_Ts_InitStyle(tmPtr->style);
    tmPtr->style.anchor = TK_ANCHOR_NW;
    tmPtr->style.padLeft = tmPtr->style.padRight = 4;
    tmPtr->style.padTop = tmPtr->style.padBottom = 4;
    return (Marker *)tmPtr;
}

static Tk_EventProc ChildEventProc;
static Tk_GeomRequestProc ChildGeometryProc;
static Tk_GeomLostSlaveProc ChildCustodyProc;
static Tk_GeomMgr winMarkerMgrInfo =
{
    (char *)"graph",                    /* Name of geometry manager used by
                                         * winfo */
    ChildGeometryProc,                  /* Procedure to for new geometry
                                         * requests. */
    ChildCustodyProc,                   /* Procedure when window is taken
                                         * away. */
};

/*
 *---------------------------------------------------------------------------
 *
 * WindowConfigureProc --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) a window
 *      marker.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as window pathname, placement,
 *      etc. get set for markerPtr; old resources get freed, if there were
 *      any.  The marker is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
WindowConfigureProc(Marker *markerPtr)
{
    Graph *graphPtr = markerPtr->obj.graphPtr;
    WindowMarker *wmPtr = (WindowMarker *)markerPtr;
    Tk_Window tkwin;

    if (wmPtr->childName == NULL) {
        return TCL_OK;
    }
    tkwin = Tk_NameToWindow(graphPtr->interp, wmPtr->childName, 
            graphPtr->tkwin);
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    if (Tk_Parent(tkwin) != graphPtr->tkwin) {
        Tcl_AppendResult(graphPtr->interp, "\"", wmPtr->childName,
            "\" is not a child of \"", Tk_PathName(graphPtr->tkwin), "\"",
            (char *)NULL);
        return TCL_ERROR;
    }
    if (tkwin != wmPtr->child) {
        if (wmPtr->child != NULL) {
            Tk_DeleteEventHandler(wmPtr->child, StructureNotifyMask,
                ChildEventProc, wmPtr);
            Tk_ManageGeometry(wmPtr->child, (Tk_GeomMgr *) 0, (ClientData)0);
            Tk_UnmapWindow(wmPtr->child);
        }
        Tk_CreateEventHandler(tkwin, StructureNotifyMask, ChildEventProc, 
                wmPtr);
        Tk_ManageGeometry(tkwin, &winMarkerMgrInfo, wmPtr);
    }
    wmPtr->child = tkwin;
    markerPtr->flags |= MAP_ITEM;
    if (markerPtr->drawUnder) {
        graphPtr->flags |= CACHE_DIRTY;
    }
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowMapProc --
 *
 *      Calculate the layout position for a window marker.  Positional
 *      information is saved in the marker.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
WindowMapProc(Marker *markerPtr)
{
    WindowMarker *wmPtr = (WindowMarker *)markerPtr;
    Graph *graphPtr = markerPtr->obj.graphPtr;
    Point2d anchorPt;
    Region2d extents;
    int width, height;

    if (wmPtr->child == (Tk_Window)NULL) {
        return;
    }
    anchorPt = MapPoint(markerPtr->worldPts, &markerPtr->axes);

    width = Tk_ReqWidth(wmPtr->child);
    height = Tk_ReqHeight(wmPtr->child);
    if (wmPtr->reqWidth > 0) {
        width = wmPtr->reqWidth;
    }
    if (wmPtr->reqHeight > 0) {
        height = wmPtr->reqHeight;
    }
    wmPtr->anchorPt = Blt_AnchorPoint(anchorPt.x, anchorPt.y, (double)width, 
        (double)height, wmPtr->anchor);
    wmPtr->anchorPt.x += markerPtr->xOffset;
    wmPtr->anchorPt.y += markerPtr->yOffset;
    wmPtr->width = width;
    wmPtr->height = height;

    /*
     * Determine the bounding box of the window and test to see if it is at
     * least partially contained within the plotting area.
     */
    extents.left = wmPtr->anchorPt.x;
    extents.top = wmPtr->anchorPt.y;
    extents.right = wmPtr->anchorPt.x + wmPtr->width - 1;
    extents.bottom = wmPtr->anchorPt.y + wmPtr->height - 1;
    markerPtr->offScreen = BoxesDontOverlap(graphPtr, &extents);
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowPointProc --
 *
 *---------------------------------------------------------------------------
 */
static int
WindowPointProc(Marker *markerPtr, Point2d *samplePtr)
{
    WindowMarker *wmPtr = (WindowMarker *)markerPtr;

    return ((samplePtr->x >= wmPtr->anchorPt.x) && 
            (samplePtr->x < (wmPtr->anchorPt.x + wmPtr->width)) &&
            (samplePtr->y >= wmPtr->anchorPt.y) && 
            (samplePtr->y < (wmPtr->anchorPt.y + wmPtr->height)));
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowAreaProc --
 *
 *---------------------------------------------------------------------------
 */
static int
WindowAreaProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
    WindowMarker *wmPtr = (WindowMarker *)markerPtr;

    if (markerPtr->numWorldPts < 1) {
        return FALSE;
    }
    if (enclosed) {
        return ((wmPtr->anchorPt.x >= extsPtr->left) &&
                (wmPtr->anchorPt.y >= extsPtr->top) && 
                ((wmPtr->anchorPt.x + wmPtr->width) <= extsPtr->right) &&
                ((wmPtr->anchorPt.y + wmPtr->height) <= extsPtr->bottom));
    }
    return !((wmPtr->anchorPt.x >= extsPtr->right) ||
             (wmPtr->anchorPt.y >= extsPtr->bottom) ||
             ((wmPtr->anchorPt.x + wmPtr->width) <= extsPtr->left) ||
             ((wmPtr->anchorPt.y + wmPtr->height) <= extsPtr->top));
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowDrawProc --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
WindowDrawProc(Marker *markerPtr, Drawable drawable)
{
    WindowMarker *wmPtr = (WindowMarker *)markerPtr;

    if (wmPtr->child == NULL) {
        return;
    }
    if ((wmPtr->height != Tk_Height(wmPtr->child)) ||
        (wmPtr->width != Tk_Width(wmPtr->child)) ||
        ((int)wmPtr->anchorPt.x != Tk_X(wmPtr->child)) ||
        ((int)wmPtr->anchorPt.y != Tk_Y(wmPtr->child))) {
        Tk_MoveResizeWindow(wmPtr->child, (int)wmPtr->anchorPt.x, 
            (int)wmPtr->anchorPt.y, wmPtr->width, wmPtr->height);
    }
    if (!Tk_IsMapped(wmPtr->child)) {
        Tk_MapWindow(wmPtr->child);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowPostscriptProc --
 *
 *---------------------------------------------------------------------------
 */
static void
WindowPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
    WindowMarker *wmPtr = (WindowMarker *)markerPtr;

    if (wmPtr->child == NULL) {
        return;
    }
    if (Tk_IsMapped(wmPtr->child)) {
        Blt_Ps_XDrawWindow(ps, wmPtr->child, wmPtr->anchorPt.x, 
                           wmPtr->anchorPt.y);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowFreeProc --
 *
 *      Destroys the structure containing the attributes of the window
 *      marker.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Window is destroyed and removed from the screen.
 *
 *---------------------------------------------------------------------------
 */
static void
WindowFreeProc(Marker *markerPtr)
{
    WindowMarker *wmPtr = (WindowMarker *)markerPtr;

    if (wmPtr->child != NULL) {
        Tk_DeleteEventHandler(wmPtr->child, StructureNotifyMask,
            ChildEventProc, wmPtr);
        Tk_ManageGeometry(wmPtr->child, (Tk_GeomMgr *) 0, (ClientData)0);
        Tk_DestroyWindow(wmPtr->child);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowCreateProc --
 *
 *      Allocate memory and initialize methods for the new window marker.
 *
 * Results:
 *      The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *      Memory is allocated for the window marker structure.
 *
 *---------------------------------------------------------------------------
 */
static Marker *
WindowCreateProc(void)
{
    WindowMarker *wmPtr;

    wmPtr = Blt_AssertCalloc(1, sizeof(WindowMarker));
    wmPtr->classPtr = &windowMarkerClass;
    return (Marker *)wmPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildEventProc --
 *
 *      This procedure is invoked whenever StructureNotify events occur for
 *      a window that's managed as part of a graph window marker. This
 *      procedure's only purpose is to clean up when windows are deleted.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The window is disassociated from the window item when it is
 *      deleted.
 *
 *---------------------------------------------------------------------------
 */
static void
ChildEventProc(ClientData clientData, XEvent *eventPtr)
{
    WindowMarker *wmPtr = clientData;

    if (eventPtr->type == DestroyNotify) {
        wmPtr->child = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildGeometryProc --
 *
 *      This procedure is invoked whenever a window that's associated with
 *      a window item changes its requested dimensions.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The size and location on the window of the window may change,
 *      depending on the options specified for the window item.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ChildGeometryProc(ClientData clientData, Tk_Window tkwin)
{
    WindowMarker *wmPtr = clientData;

    if (wmPtr->reqWidth == 0) {
        wmPtr->width = Tk_ReqWidth(tkwin);
    }
    if (wmPtr->reqHeight == 0) {
        wmPtr->height = Tk_ReqHeight(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildCustodyProc --
 *
 *      This procedure is invoked when an embedded window has been stolen
 *      by another geometry manager.  The information and memory associated
 *      with the widget is released.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Arranges for the graph to be redrawn without the embedded widget at
 *      the next idle point.
 *
 *---------------------------------------------------------------------------
 */
 /* ARGSUSED */
static void
ChildCustodyProc(ClientData clientData, Tk_Window tkwin)
{
    Marker *markerPtr = clientData;
    Graph *graphPtr;

    graphPtr = markerPtr->obj.graphPtr;
    DestroyMarker(markerPtr);
    /*
     * Not really needed. We should get an Expose event when the child
     * window is unmapped.
     */
    Blt_EventuallyRedrawGraph(graphPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * LineFreeProc --
 *
 *      Destroys the structure and attributes of a line marker.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Line attributes (GCs, colors, stipple, etc) get released.  Memory is
 *      deallocated, X resources are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
LineFreeProc(Marker *markerPtr)
{
    LineMarker *lmPtr = (LineMarker *)markerPtr;
    Graph *graphPtr = markerPtr->obj.graphPtr;

    if (lmPtr->gc != NULL) {
        Blt_FreePrivateGC(graphPtr->display, lmPtr->gc);
    }
    if (lmPtr->segments != NULL) {
        Blt_Free(lmPtr->segments);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * LineCreateProc --
 *
 *      Allocate memory and initialize methods for a new line marker.
 *
 * Results:
 *      The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *      Memory is allocated for the line marker structure.
 *
 *---------------------------------------------------------------------------
 */
static Marker *
LineCreateProc(void)
{
    LineMarker *lmPtr;

    lmPtr = Blt_AssertCalloc(1, sizeof(LineMarker));
    lmPtr->classPtr = &lineMarkerClass;
    lmPtr->xor = FALSE;
    lmPtr->capStyle = CapButt;
    lmPtr->joinStyle = JoinMiter;
    return (Marker *)lmPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * LineMapProc --
 *
 *      Calculate the layout position for a line marker.  Positional
 *      information is saved in the marker.  The line positions are stored
 *      in an array of points (malloc'ed).
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
LineMapProc(Marker *markerPtr)
{
    LineMarker *lmPtr = (LineMarker *)markerPtr;
    Point2d *srcPtr, *pend;
    Segment2d *segments, *segPtr;
    Point2d p, q;
    Region2d extents;

    lmPtr->numSegments = 0;
    if (lmPtr->segments != NULL) {
        Blt_Free(lmPtr->segments);
    }
    if (markerPtr->numWorldPts < 2) {
        return;                         /* Too few points */
    }
    GraphExtents(markerPtr, &extents);

    /* 
     * Allow twice the number of world coordinates. The line will
     * represented as series of line segments, not one continous polyline.
     * This is because clipping against the plot area may chop the line
     * into several disconnected segments.
     */
    segments = Blt_AssertMalloc(markerPtr->numWorldPts * sizeof(Segment2d));
    srcPtr = markerPtr->worldPts;
    p = MapPoint(srcPtr, &markerPtr->axes);
    p.x += markerPtr->xOffset;
    p.y += markerPtr->yOffset;

    segPtr = segments;
    for (srcPtr++, pend = markerPtr->worldPts + markerPtr->numWorldPts; 
         srcPtr < pend; srcPtr++) {
        Point2d next;

        next = MapPoint(srcPtr, &markerPtr->axes);
        next.x += markerPtr->xOffset;
        next.y += markerPtr->yOffset;
        q = next;
        if (Blt_LineRectClip(&extents, &p, &q)) {
            segPtr->p = p;
            segPtr->q = q;
            segPtr++;
        }
        p = next;
    }
    lmPtr->numSegments = segPtr - segments;
    lmPtr->segments = segments;
    markerPtr->offScreen = (lmPtr->numSegments == 0);
}

static int
LinePointProc(Marker *markerPtr, Point2d *samplePtr)
{
    LineMarker *lmPtr = (LineMarker *)markerPtr;

    return Blt_PointInSegments(samplePtr, lmPtr->segments, lmPtr->numSegments, 
           (double)markerPtr->obj.graphPtr->halo);
}

/*
 *---------------------------------------------------------------------------
 *
 * LineAreaProc --
 *
 *---------------------------------------------------------------------------
 */
static int
LineAreaProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
    if (markerPtr->numWorldPts < 2) {
        return FALSE;
    }
    if (enclosed) {
        Point2d *pp, *pend;

        for (pp = markerPtr->worldPts, pend = pp + markerPtr->numWorldPts; 
             pp < pend; pp++) {
            Point2d p;

            p = MapPoint(pp, &markerPtr->axes);
            if ((p.x < extsPtr->left) && (p.x > extsPtr->right) &&
                (p.y < extsPtr->top) && (p.y > extsPtr->bottom)) {
                return FALSE;
            }
        }
        return TRUE;                    /* All points inside bounding
                                         * box. */
    } else {
        int count;
        Point2d *pp, *pend;

        count = 0;
        for (pp = markerPtr->worldPts, pend = pp + (markerPtr->numWorldPts-1); 
                pp < pend; pp++) {
            Point2d p, q;

            p = MapPoint(pp, &markerPtr->axes);
            q = MapPoint(pp + 1, &markerPtr->axes);
            if (Blt_LineRectClip(extsPtr, &p, &q)) {
                count++;
            }
        }
        return (count > 0);             /* At least 1 segment passes
                                         * through region. */
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * LineDrawProc --
 *
 *---------------------------------------------------------------------------
 */
static void
LineDrawProc(Marker *markerPtr, Drawable drawable)
{
    LineMarker *lmPtr = (LineMarker *)markerPtr;

    if (lmPtr->numSegments > 0) {
        Graph *graphPtr = markerPtr->obj.graphPtr;

        Blt_DrawSegments2d(graphPtr->display, drawable, lmPtr->gc, 
                lmPtr->segments, lmPtr->numSegments);
        if (lmPtr->xor) {               /* Toggle the drawing state */
            lmPtr->xorState = (lmPtr->xorState == 0);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * LineConfigureProc --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) a line
 *      marker.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as line width, colors, dashes,
 *      etc. get set for markerPtr; old resources get freed, if there were
 *      any.  The marker is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
LineConfigureProc(Marker *markerPtr)
{
    Graph *graphPtr = markerPtr->obj.graphPtr;
    LineMarker *lmPtr = (LineMarker *)markerPtr;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;
    Drawable drawable;

    drawable = Tk_WindowId(graphPtr->tkwin);
    gcMask = (GCLineWidth | GCLineStyle | GCCapStyle | GCJoinStyle);
    if (lmPtr->outlineColor != NULL) {
        gcMask |= GCForeground;
        gcValues.foreground = lmPtr->outlineColor->pixel;
    }
    if (lmPtr->fillColor != NULL) {
        gcMask |= GCBackground;
        gcValues.background = lmPtr->fillColor->pixel;
    }
    gcValues.cap_style = lmPtr->capStyle;
    gcValues.join_style = lmPtr->joinStyle;
    gcValues.line_width = LineWidth(lmPtr->lineWidth);
    gcValues.line_style = LineSolid;
    if (LineIsDashed(lmPtr->dashes)) {
        gcValues.line_style = 
            (gcMask & GCBackground) ? LineDoubleDash : LineOnOffDash;
    }
    if (lmPtr->xor) {
        unsigned long pixel;
        gcValues.function = GXxor;

        gcMask |= GCFunction;
        if (graphPtr->plotBg == NULL) {
            pixel = WhitePixelOfScreen(Tk_Screen(graphPtr->tkwin));
        } else {
            pixel = Blt_Bg_BorderColor(graphPtr->plotBg)->pixel;
        }
        if (gcMask & GCBackground) {
            gcValues.background ^= pixel;
        }
        gcValues.foreground ^= pixel;
        if (drawable != None) {
            LineDrawProc(markerPtr, drawable);
        }
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(lmPtr->dashes)) {
        Blt_SetDashes(graphPtr->display, newGC, &lmPtr->dashes);
    }
    if (lmPtr->gc != NULL) {
        Blt_FreePrivateGC(graphPtr->display, lmPtr->gc);
    }
    lmPtr->gc = newGC;
    if (lmPtr->xor) {
        if (drawable != None) {
            LineMapProc(markerPtr);
            LineDrawProc(markerPtr, drawable);
        }
        return TCL_OK;
    }
    markerPtr->flags |= MAP_ITEM;
    if (markerPtr->drawUnder) {
        graphPtr->flags |= CACHE_DIRTY;
    }
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LinePostscriptProc --
 *
 *      Prints postscript commands to display the connect line.  Dashed
 *      lines need to be handled specially, especially if a background
 *      color is designated.
 *
 * Results:
 *      None.
 *
 * Side effects:
  *     PostScript output commands are saved in the interpreter
 *      (infoPtr->interp) result field.
 *
 *---------------------------------------------------------------------------
 */
static void
LinePostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
    LineMarker *lmPtr = (LineMarker *)markerPtr;

    if (lmPtr->numSegments > 0) {
        Blt_Ps_XSetLineAttributes(ps, lmPtr->outlineColor, 
                lmPtr->lineWidth, &lmPtr->dashes, lmPtr->capStyle,
                lmPtr->joinStyle);
        if ((LineIsDashed(lmPtr->dashes)) && (lmPtr->fillColor != NULL)) {
            Blt_Ps_Append(ps, "/DashesProc {\n  gsave\n    ");
            Blt_Ps_XSetBackground(ps, lmPtr->fillColor);
            Blt_Ps_Append(ps, "    ");
            Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
            Blt_Ps_VarAppend(ps,
                             "stroke\n",
                             "  grestore\n",
                             "} def\n", (char *)NULL);
        } else {
            Blt_Ps_Append(ps, "/DashesProc {} def\n");
        }
        Blt_Ps_DrawSegments2d(ps, lmPtr->numSegments, lmPtr->segments);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * PolygonFreeProc --
 *
 *      Release memory and resources allocated for the polygon element.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the polygon element is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
PolygonFreeProc(Marker *markerPtr)
{
    PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
    Graph *graphPtr = markerPtr->obj.graphPtr;

    if (pmPtr->fillGC != NULL) {
        Tk_FreeGC(graphPtr->display, pmPtr->fillGC);
    }
    if (pmPtr->outlineGC != NULL) {
        Blt_FreePrivateGC(graphPtr->display, pmPtr->outlineGC);
    }
    if (pmPtr->fillPts != NULL) {
        Blt_Free(pmPtr->fillPts);
    }
    if (pmPtr->outlinePts != NULL) {
        Blt_Free(pmPtr->outlinePts);
    }
    if (pmPtr->screenPts != NULL) {
        Blt_Free(pmPtr->screenPts);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PolygonCreateProc --
 *
 *      Allocate memory and initialize methods for the new polygon marker.
 *
 * Results:
 *      The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *      Memory is allocated for the polygon marker structure.
 *
 * -------------------------------------------------------------------------- 
 */
static Marker *
PolygonCreateProc(void)
{
    PolygonMarker *pmPtr;

    pmPtr = Blt_AssertCalloc(1, sizeof(PolygonMarker));
    pmPtr->classPtr = &polygonMarkerClass;
    pmPtr->capStyle = CapButt;
    pmPtr->joinStyle = JoinMiter;
    return (Marker *)pmPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * PolygonConfigureProc --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) a polygon
 *      marker.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as polygon color, dashes,
 *      fillstyle, etc. get set for markerPtr; old resources get freed, if
 *      there were any.  The marker is eventually redisplayed.
 *
 * -------------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
PolygonConfigureProc(Marker *markerPtr)
{
    Graph *graphPtr = markerPtr->obj.graphPtr;
    PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;
    Drawable drawable;

    drawable = Tk_WindowId(graphPtr->tkwin);
    gcMask = (GCLineWidth | GCLineStyle);
    if (pmPtr->outline.fgColor != NULL) {
        gcMask |= GCForeground;
        gcValues.foreground = pmPtr->outline.fgColor->pixel;
    }
    if (pmPtr->outline.bgColor != NULL) {
        gcMask |= GCBackground;
        gcValues.background = pmPtr->outline.bgColor->pixel;
    }
    gcMask |= (GCCapStyle | GCJoinStyle);
    gcValues.cap_style = pmPtr->capStyle;
    gcValues.join_style = pmPtr->joinStyle;
    gcValues.line_style = LineSolid;
    gcValues.dash_offset = 0;
    gcValues.line_width = LineWidth(pmPtr->lineWidth);
    if (LineIsDashed(pmPtr->dashes)) {
        gcValues.line_style = (pmPtr->outline.bgColor == NULL)
            ? LineOnOffDash : LineDoubleDash;
    }
    if (pmPtr->xor) {
        unsigned long pixel;
        gcValues.function = GXxor;

        gcMask |= GCFunction;
        if (graphPtr->plotBg == NULL) {
            /* The graph's color option may not have been set yet */
            pixel = WhitePixelOfScreen(Tk_Screen(graphPtr->tkwin));
        } else {
            pixel = Blt_Bg_BorderColor(graphPtr->plotBg)->pixel;
        }
        if (gcMask & GCBackground) {
            gcValues.background ^= pixel;
        }
        gcValues.foreground ^= pixel;
        if (drawable != None) {
            PolygonDrawProc(markerPtr, drawable);
        }
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(pmPtr->dashes)) {
        Blt_SetDashes(graphPtr->display, newGC, &pmPtr->dashes);
    }
    if (pmPtr->outlineGC != NULL) {
        Blt_FreePrivateGC(graphPtr->display, pmPtr->outlineGC);
    }
    pmPtr->outlineGC = newGC;

    gcMask = 0;
    if (pmPtr->fill.fgColor != NULL) {
        gcMask |= GCForeground;
        gcValues.foreground = pmPtr->fill.fgColor->pixel;
    }
    if (pmPtr->fill.bgColor != NULL) {
        gcMask |= GCBackground;
        gcValues.background = pmPtr->fill.bgColor->pixel;
    }
    if (pmPtr->stipple != None) {
        gcValues.stipple = pmPtr->stipple;
        gcValues.fill_style = (pmPtr->fill.bgColor != NULL)
            ? FillOpaqueStippled : FillStippled;
        gcMask |= (GCStipple | GCFillStyle);
    }
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (pmPtr->fillGC != NULL) {
        Tk_FreeGC(graphPtr->display, pmPtr->fillGC);
    }
    pmPtr->fillGC = newGC;

    if ((gcMask == 0) && !(graphPtr->flags & RESET_AXES) && (pmPtr->xor)) {
        if (drawable != None) {
            PolygonMapProc(markerPtr);
            PolygonDrawProc(markerPtr, drawable);
        }
        return TCL_OK;
    }
    markerPtr->flags |= MAP_ITEM;
    if (markerPtr->drawUnder) {
        graphPtr->flags |= CACHE_DIRTY;
    }
    graphPtr->flags |= RESET_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PolygonMapProc --
 *
 *      Calculate the layout position for a polygon marker.  Positional
 *      information is saved in the polygon in an array of points
 *      (malloc'ed).
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
PolygonMapProc(Marker *markerPtr)
{
    PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
    Point2d *screenPts;
    Region2d extents;
    int numScreenPts;

    if (pmPtr->outlinePts != NULL) {
        Blt_Free(pmPtr->outlinePts);
        pmPtr->outlinePts = NULL;
        pmPtr->numOutlinePts = 0;
    }
    if (pmPtr->fillPts != NULL) {
        Blt_Free(pmPtr->fillPts);
        pmPtr->fillPts = NULL;
        pmPtr->numFillPts = 0;
    }
    if (pmPtr->screenPts != NULL) {
        Blt_Free(pmPtr->screenPts);
        pmPtr->screenPts = NULL;
    }
    if (markerPtr->numWorldPts < 3) {
        return;                         /* Too few points */
    }

    /* 
     * Allocate and fill a temporary array to hold the screen coordinates of
     * the polygon.
     */
    numScreenPts = markerPtr->numWorldPts + 1;
    screenPts = Blt_AssertMalloc((numScreenPts + 1) * sizeof(Point2d));
    {
        Point2d *sp, *dp, *send;

        dp = screenPts;
        for (sp = markerPtr->worldPts, send = sp + markerPtr->numWorldPts; 
             sp < send; sp++) {
            *dp = MapPoint(sp, &markerPtr->axes);
            dp->x += markerPtr->xOffset;
            dp->y += markerPtr->yOffset;
            dp++;
        }
        *dp = screenPts[0];
    }
    GraphExtents(markerPtr, &extents);
    markerPtr->offScreen = TRUE;
    if (pmPtr->fill.fgColor != NULL) {  /* Polygon fill required. */
        Point2d *fillPts;
        int n;

        fillPts = Blt_AssertMalloc(sizeof(Point2d) * numScreenPts * 3);
        n = Blt_PolyRectClip(&extents, screenPts, markerPtr->numWorldPts,
                fillPts);
        if (n < 3) { 
            Blt_Free(fillPts);
        } else {
            pmPtr->numFillPts = n;
            pmPtr->fillPts = fillPts;
            markerPtr->offScreen = FALSE;
        }
    }
    if ((pmPtr->outline.fgColor != NULL) && (pmPtr->lineWidth > 0)) { 
        Segment2d *outlinePts;
        Segment2d *segPtr;
        Point2d *sp, *send;

        /* 
         * Generate line segments representing the polygon outline.  The
         * resulting outline may or may not be closed due to viewport
         * clipping.
         */
        outlinePts = Blt_Malloc(numScreenPts * sizeof(Segment2d));
        if (outlinePts == NULL) {
            return;                     /* Can't allocate point array */
        }
        /* 
         * Note that this assumes that the point array contains an extra
         * point that closes the polygon.
         */
        segPtr = outlinePts;
        for (sp = screenPts, send = sp + (numScreenPts - 1); sp < send; sp++) {
            segPtr->p = sp[0];
            segPtr->q = sp[1];
            if (Blt_LineRectClip(&extents, &segPtr->p, &segPtr->q)) {
                segPtr++;
            }
        }
        pmPtr->numOutlinePts = segPtr - outlinePts;
        pmPtr->outlinePts = outlinePts;
        if (pmPtr->numOutlinePts > 0) {
            markerPtr->offScreen = FALSE;
        }
    }
    pmPtr->screenPts = screenPts;
}

#ifdef notdef

static int
GradientCalcProc(ClientData clientData, int x, int y, double *valuePtr)
{
    Graph *graphPtr;
    PolygonMarker *markerPtr = clientData;
    double value;
    Point2d point;
    AxisRange *rangePtr;
    
    /* 
     * Compute length of the difference in the y direction.
     * Find the vertical segment of the polygon that intersects 
     * y y 
     */
    graphPtr = elemPtr->obj.graphPtr;
    point = Blt_InvMap2D(graphPtr, x, y, &elemPtr->axes);
    rangePtr = &elemPtr->zAxisPtr->valueRange;
    if (elemPtr->zAxisPtr->obj.classId == CID_AXIS_Y) {
        value = point.y;
    } else if (elemPtr->zAxisPtr->obj.classId == CID_AXIS_X) {
        value = point.x;
    } else {
        return TCL_ERROR;
    }
    *valuePtr = (value - rangePtr->min) / rangePtr->range;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawGradientPolygon --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawGradientPolygon(Graph *graphPtr, Drawable drawable,
                    PolygonMarker *markerPtr, int n, XPoint *points)
{
    Blt_PaintBrush brush;
    Blt_Painter painter;
    Blt_Picture bg;
    Point2f *vertices;
    int i;
    int w, h;
    int x1, x2, y1, y2;

    if (n < 3) {
        return;                         /* Not enough points for polygon */
    }
    if (markerPtr->palette == NULL) {
        return;                         /* No palette defined. */
    }
    /* Grab the rectangular background that covers the polygon. */
    GetPolygonBBox(points, n, &x1, &x2, &y1, &y2);
    w = x2 - x1 + 1;
    h = y2 - y1 + 1;
    bg = Blt_DrawableToPicture(graphPtr->tkwin, drawable, x1, y1, w, h, 1.0);
    if (bg == NULL) {
        return;                         /* Background is obscured. */
    }
    vertices = Blt_AssertMalloc(n * sizeof(Point2f));
    /* Translate the polygon */
    for (i = 0; i < n; i++) {
        vertices[i].x = (float)(points[i].x - x1);
        vertices[i].y = (float)(points[i].y - y1);
    }
    
    brush = Blt_NewLinearGradientBrush();
    Blt_SetBrushOrigin(brush, -x1, -y1);
    Blt_SetLinearGradientBrushPalette(brush, markerPtr->palette);
    Blt_SetLinearGradientBrushCalcProc(brush, GradientCalcProc, markerPtr);
    Blt_PaintPolygon(bg, n, vertices, brush);
    Blt_FreeBrush(brush);
    Blt_Free(vertices);
    painter = Blt_GetPainter(graphPtr->tkwin, 1.0);
    Blt_PaintPicture(painter, drawable, bg, 0, 0, w, h, x1, y1, 0);
    Blt_FreePicture(bg);
}
#endif

static void
PolygonDrawProc(Marker *markerPtr, Drawable drawable)
{
    Graph *graphPtr = markerPtr->obj.graphPtr;
    PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;

    /* Draw polygon fill region */
    if ((pmPtr->numFillPts > 0) && (pmPtr->fill.fgColor != NULL)) {
        XPoint *dp, *points;
        Point2d *sp, *send;
        
        points = Blt_Malloc(pmPtr->numFillPts * sizeof(XPoint));
        if (points == NULL) {
            return;
        }
        dp = points;
        for (sp = pmPtr->fillPts, send = sp + pmPtr->numFillPts; sp < send; 
             sp++) {
            dp->x = (short int)sp->x;
            dp->y = (short int)sp->y;
            dp++;
        }
        XFillPolygon(graphPtr->display, drawable, pmPtr->fillGC, points, 
                pmPtr->numFillPts, Complex, CoordModeOrigin);
        Blt_Free(points);
    }
    /* and then the outline */
    if ((pmPtr->numOutlinePts > 0) && (pmPtr->lineWidth > 0) && 
        (pmPtr->outline.fgColor != NULL)) {
        Blt_DrawSegments2d(graphPtr->display, drawable, pmPtr->outlineGC,
            pmPtr->outlinePts, pmPtr->numOutlinePts);
    }
}


static void
PolygonPostscriptProc(Marker *markerPtr, Blt_Ps ps)
{
    Graph *graphPtr = markerPtr->obj.graphPtr;
    PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;

    if (pmPtr->fill.fgColor != NULL) {

        /*
         * Options:  fg bg
         *                      Draw outline only.
         *           x          Draw solid or stipple.
         *           x  x       Draw solid or stipple.
         */

        /* Create a path to use for both the polygon and its outline. */
        Blt_Ps_Polyline(ps, pmPtr->numFillPts, pmPtr->fillPts);

        /* If the background fill color was specified, draw the polygon in
         * a solid fashion with that color.  */
        if (pmPtr->fill.bgColor != NULL) {
            /* Draw the solid background as the background layer of the
             * opaque stipple */
            Blt_Ps_XSetBackground(ps, pmPtr->fill.bgColor);
            /* Retain the path. We'll need it for the foreground layer. */
            Blt_Ps_Append(ps, "gsave fill grestore\n");
        }
        Blt_Ps_XSetForeground(ps, pmPtr->fill.fgColor);
        if (pmPtr->stipple != None) {
            /* Draw the stipple in the foreground color. */
            Blt_Ps_XSetStipple(ps, graphPtr->display, pmPtr->stipple);
        } else {
            Blt_Ps_Append(ps, "fill\n");
        }
    }

    /* Draw the outline in the foreground color.  */
    if ((pmPtr->lineWidth > 0) && (pmPtr->outline.fgColor != NULL)) {

        /*  Set up the line attributes.  */
        Blt_Ps_XSetLineAttributes(ps, pmPtr->outline.fgColor,
            pmPtr->lineWidth, &pmPtr->dashes, pmPtr->capStyle,
            pmPtr->joinStyle);

        /*  
         * Define on-the-fly a PostScript macro "DashesProc" that will be
         * executed for each call to the Polygon drawing routine.  If the
         * line isn't dashed, simply make this an empty definition.
         */
        if ((pmPtr->outline.bgColor != NULL) && (LineIsDashed(pmPtr->dashes))) {
            Blt_Ps_Append(ps, "/DashesProc {\ngsave\n    ");
            Blt_Ps_XSetBackground(ps, pmPtr->outline.bgColor);
            Blt_Ps_Append(ps, "    ");
            Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
            Blt_Ps_Append(ps, "stroke\n  grestore\n} def\n");
        } else {
            Blt_Ps_Append(ps, "/DashesProc {} def\n");
        }
        Blt_Ps_DrawSegments2d(ps, pmPtr->numOutlinePts, pmPtr->outlinePts);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PolygonPointProc --
 *
 *---------------------------------------------------------------------------
 */
static int
PolygonPointProc(Marker *markerPtr, Point2d *samplePtr)
{
    PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;

    if ((markerPtr->numWorldPts >= 3) && (pmPtr->screenPts != NULL)) {
        return Blt_PointInPolygon(samplePtr, pmPtr->screenPts, 
                markerPtr->numWorldPts + 1);
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * PolygonAreaProc --
 *
 *---------------------------------------------------------------------------
 */
static int
PolygonAreaProc(Marker *markerPtr, Region2d *extsPtr, int enclosed)
{
    PolygonMarker *pmPtr = (PolygonMarker *)markerPtr;
    
    if ((markerPtr->numWorldPts >= 3) && (pmPtr->screenPts != NULL)) {
        return Blt_RegionInPolygon(extsPtr, pmPtr->screenPts, 
                markerPtr->numWorldPts, enclosed);
    }
    return FALSE;
}


/*
 *---------------------------------------------------------------------------
 *
 * RectangleFreeProc --
 *
 *      Release memory and resources allocated for the rectangle element.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the rectangle element is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
RectangleFreeProc(Marker *basePtr)
{
    RectangleMarker *markerPtr = (RectangleMarker *)basePtr;
    Graph *graphPtr = basePtr->obj.graphPtr;

    if (markerPtr->fillGC != NULL) {
        Tk_FreeGC(graphPtr->display, markerPtr->fillGC);
    }
    if (markerPtr->outlineGC != NULL) {
        Blt_FreePrivateGC(graphPtr->display, markerPtr->outlineGC);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * RectangleCreateProc --
 *
 *      Allocate memory and initialize methods for the new rectangle marker.
 *
 * Results:
 *      The pointer to the newly allocated marker structure is returned.
 *
 * Side effects:
 *      Memory is allocated for the rectangle marker structure.
 *
 * -------------------------------------------------------------------------- 
 */
static Marker *
RectangleCreateProc(void)
{
    RectangleMarker *markerPtr;

    markerPtr = Blt_AssertCalloc(1, sizeof(RectangleMarker));
    markerPtr->classPtr = &rectangleMarkerClass;
    markerPtr->capStyle = CapButt;
    markerPtr->joinStyle = JoinMiter;
    return (Marker *)markerPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * RectangleConfigureProc --
 *
 *      This procedure is called to process an objv/objc list, plus the Tk
 *      option database, in order to configure (or reconfigure) a rectangle
 *      marker.
 *
 * Results:
 *      A standard TCL result.  If TCL_ERROR is returned, then
 *      interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information, such as rectangle color, dashes,
 *      fillstyle, etc. get set for markerPtr; old resources get freed, if
 *      there were any.  The marker is eventually redisplayed.
 *
 * -------------------------------------------------------------------------- 
 */
/*ARGSUSED*/
static int
RectangleConfigureProc(Marker *basePtr)
{
    Graph *graphPtr;
    RectangleMarker *markerPtr = (RectangleMarker *)basePtr;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;
    Drawable drawable;

    graphPtr = markerPtr->obj.graphPtr;
    drawable = Tk_WindowId(graphPtr->tkwin);
    gcMask = (GCLineWidth | GCLineStyle);
    if (markerPtr->outline.fgColor != NULL) {
        gcMask |= GCForeground;
        gcValues.foreground = markerPtr->outline.fgColor->pixel;
    }
    if (markerPtr->outline.bgColor != NULL) {
        gcMask |= GCBackground;
        gcValues.background = markerPtr->outline.bgColor->pixel;
    }
    gcMask |= (GCCapStyle | GCJoinStyle);
    gcValues.cap_style = markerPtr->capStyle;
    gcValues.join_style = markerPtr->joinStyle;
    gcValues.line_style = LineSolid;
    gcValues.dash_offset = 0;
    gcValues.line_width = LineWidth(markerPtr->lineWidth);
    if (LineIsDashed(markerPtr->dashes)) {
        gcValues.line_style = (markerPtr->outline.bgColor == NULL)
            ? LineOnOffDash : LineDoubleDash;
    }
    if (markerPtr->xor) {
        unsigned long pixel;
        gcValues.function = GXxor;

        gcMask |= GCFunction;
        if (graphPtr->plotBg == NULL) {
            /* The graph's color option may not have been set yet */
            pixel = WhitePixelOfScreen(Tk_Screen(graphPtr->tkwin));
        } else {
            pixel = Blt_Bg_BorderColor(graphPtr->plotBg)->pixel;
        }
        if (gcMask & GCBackground) {
            gcValues.background ^= pixel;
        }
        gcValues.foreground ^= pixel;
        if (drawable != None) {
            RectangleDrawProc(basePtr, drawable);
        }
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(markerPtr->dashes)) {
        Blt_SetDashes(graphPtr->display, newGC, &markerPtr->dashes);
    }
    if (markerPtr->outlineGC != NULL) {
        Blt_FreePrivateGC(graphPtr->display, markerPtr->outlineGC);
    }
    markerPtr->outlineGC = newGC;

    gcMask = 0;
    if (markerPtr->fill.fgColor != NULL) {
        gcMask |= GCForeground;
        gcValues.foreground = markerPtr->fill.fgColor->pixel;
    }
    if (markerPtr->fill.bgColor != NULL) {
        gcMask |= GCBackground;
        gcValues.background = markerPtr->fill.bgColor->pixel;
    }
    if (markerPtr->stipple != None) {
        gcValues.stipple = markerPtr->stipple;
        gcValues.fill_style = (markerPtr->fill.bgColor != NULL)
            ? FillOpaqueStippled : FillStippled;
        gcMask |= (GCStipple | GCFillStyle);
    }
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (markerPtr->fillGC != NULL) {
        Tk_FreeGC(graphPtr->display, markerPtr->fillGC);
    }
    markerPtr->fillGC = newGC;

    if ((gcMask == 0) && !(graphPtr->flags & RESET_AXES) && (markerPtr->xor)) {
        if (drawable != None) {
            RectangleMapProc(basePtr);
            RectangleDrawProc(basePtr, drawable);
        }
        return TCL_OK;
    }
    basePtr->flags |= MAP_ITEM;
    if (basePtr->drawUnder) {
        graphPtr->flags |= CACHE_DIRTY;
    }
    graphPtr->flags |= RESET_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RectangleMapProc --
 *
 *      Calculate the layout position for a rectangle marker.  Positional
 *      information is saved in the rectangle in an array of points
 *      (malloc'ed).
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
RectangleMapProc(Marker *basePtr)
{
    RectangleMarker *markerPtr = (RectangleMarker *)basePtr;
    Region2d extents;

    if (basePtr->numWorldPts != 2) {
        return;                         /* Too few points */
    }
    markerPtr->corner1 = MapPoint(markerPtr->worldPts,     &markerPtr->axes);
    markerPtr->corner2 = MapPoint(markerPtr->worldPts + 1, &markerPtr->axes);

    if (markerPtr->corner1.x > markerPtr->corner2.x) {
        SWAP(markerPtr->corner1.x, markerPtr->corner2.x);
    }
    if (markerPtr->corner1.y > markerPtr->corner2.y) {
        SWAP(markerPtr->corner1.y, markerPtr->corner2.y);
    }
    GraphExtents(basePtr, &extents);

    markerPtr->offScreen = FALSE;
    if ((markerPtr->outline.fgColor != NULL) && (markerPtr->lineWidth > 0)) { 
        Segment2d *s;

        s = markerPtr->outlineSegments;
        /* AB x1, y1 --> x1, y2 */
        s->p.x = markerPtr->corner1.x;
        s->p.y = markerPtr->corner1.y;
        s->q.x = markerPtr->corner1.x;
        s->q.y = markerPtr->corner2.y;
        if (Blt_LineRectClip(&extents, &s->p, &s->q)) {
            s++;
        }
        /* BC x1, y2 --> x2, y2 */
        s->p.x = markerPtr->corner1.x;
        s->p.y = markerPtr->corner2.y;
        s->q.x = markerPtr->corner2.x;
        s->q.y = markerPtr->corner2.y;
        if (Blt_LineRectClip(&extents, &s->p, &s->q)) {
            s++;
        }
        /* CD x2, y2 --> x2, y1 */
        s->p.x = markerPtr->corner2.x;
        s->p.y = markerPtr->corner2.y;
        s->q.x = markerPtr->corner2.x;
        s->q.y = markerPtr->corner1.y;
        if (Blt_LineRectClip(&extents, &s->p, &s->q)) {
            s++;
        }
        /* DA x2, y1 --> x1, y1 */
        s->p.x = markerPtr->corner2.x;
        s->p.y = markerPtr->corner1.y;
        s->q.x = markerPtr->corner1.x;
        s->q.y = markerPtr->corner1.y;
        if (Blt_LineRectClip(&extents, &s->p, &s->q)) {
            s++;
        }
        markerPtr->numOutlineSegments = s - markerPtr->outlineSegments;
        if (markerPtr->numOutlineSegments > 0) {
            markerPtr->offScreen = FALSE;
        }
    }
    if (markerPtr->fill.fgColor != NULL) {  /* Rectangle fill required. */
        if (markerPtr->corner1.x < extents.left) {
            markerPtr->corner1.x = extents.left;
        }
        if (markerPtr->corner2.x > extents.right) {
            markerPtr->corner2.x = extents.right;
        }
        if (markerPtr->corner1.y < extents.top) {
            markerPtr->corner1.y = extents.top;
        }
        if (markerPtr->corner2.y > extents.bottom) {
            markerPtr->corner2.y = extents.bottom;
        }
        markerPtr->fillPts[0] = markerPtr->corner1;
        markerPtr->fillPts[1] = markerPtr->corner2;
    }
}

#ifdef notdef

static int
GradientCalcProc(ClientData clientData, int x, int y, double *valuePtr)
{
    Graph *graphPtr;
    RectangleMarker *markerPtr = clientData;
    double value;
    Point2d point;
    AxisRange *rangePtr;
    
    /* 
     * Compute length of the difference in the y direction.
     * Find the vertical segment of the rectangle that intersects 
     * y y 
     */
    graphPtr = elemPtr->obj.graphPtr;
    point = Blt_InvMap2D(graphPtr, x, y, &elemPtr->axes);
    rangePtr = &elemPtr->zAxisPtr->valueRange;
    if (elemPtr->zAxisPtr->obj.classId == CID_AXIS_Y) {
        value = point.y;
    } else if (elemPtr->zAxisPtr->obj.classId == CID_AXIS_X) {
        value = point.x;
    } else {
        return TCL_ERROR;
    }
    *valuePtr = (value - rangePtr->min) / rangePtr->range;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawGradientRectangle --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawGradientRectangle(Graph *graphPtr, Drawable drawable,
                    RectangleMarker *markerPtr, int n, XPoint *points)
{
    Blt_PaintBrush brush;
    Blt_Painter painter;
    Blt_Picture bg;
    Point2f *vertices;
    int i;
    int w, h;
    int x1, x2, y1, y2;

    if (n < 3) {
        return;                         /* Not enough points for rectangle */
    }
    if (markerPtr->palette == NULL) {
        return;                         /* No palette defined. */
    }
    /* Grab the rectangular background that covers the rectangle. */
    GetRectangleBBox(points, n, &x1, &x2, &y1, &y2);
    w = x2 - x1 + 1;
    h = y2 - y1 + 1;
    bg = Blt_DrawableToPicture(graphPtr->tkwin, drawable, x1, y1, w, h, 1.0);
    if (bg == NULL) {
        return;                         /* Background is obscured. */
    }
    vertices = Blt_AssertMalloc(n * sizeof(Point2f));
    /* Translate the rectangle */
    for (i = 0; i < n; i++) {
        vertices[i].x = (float)(points[i].x - x1);
        vertices[i].y = (float)(points[i].y - y1);
    }
    
    brush = Blt_NewLinearGradientBrush();
    Blt_SetBrushOrigin(brush, -x1, -y1);
    Blt_SetLinearGradientBrushPalette(brush, markerPtr->palette);
    Blt_SetLinearGradientBrushCalcProc(brush, GradientCalcProc, markerPtr);
    Blt_PaintRectangle(bg, n, vertices, brush);
    Blt_FreeBrush(brush);
    Blt_Free(vertices);
    painter = Blt_GetPainter(graphPtr->tkwin, 1.0);
    Blt_PaintPicture(painter, drawable, bg, 0, 0, w, h, x1, y1, 0);
    Blt_FreePicture(bg);
}
#endif

static void
RectangleDrawProc(Marker *basePtr, Drawable drawable)
{
    Graph *graphPtr = basePtr->obj.graphPtr;
    RectangleMarker *markerPtr = (RectangleMarker *)basePtr;

    if (markerPtr->numWorldPts != 2) {
        return;
    }
    if (markerPtr->fill.fgColor != NULL) {
        int x, y, w, h;
        
        x = (int)markerPtr->fillPts[0].x;
        y = (int)markerPtr->fillPts[0].y;
        w = (int)(markerPtr->fillPts[1].x - markerPtr->fillPts[0].x);
        h = (int)(markerPtr->fillPts[1].y - markerPtr->fillPts[0].y);
        if ((w > 0) && (h > 0)) {
            XFillRectangle(graphPtr->display, drawable, markerPtr->fillGC,
                           x, y, w, h);
        }
    }
    if ((markerPtr->numOutlineSegments > 0) && (markerPtr->lineWidth > 0) && 
        (markerPtr->outline.fgColor != NULL)) {
        Blt_DrawSegments2d(graphPtr->display, drawable, markerPtr->outlineGC,
            markerPtr->outlineSegments, markerPtr->numOutlineSegments);
    }
}


static void
RectanglePostscriptProc(Marker *basePtr, Blt_Ps ps)
{
    Graph *graphPtr = basePtr->obj.graphPtr;
    RectangleMarker *markerPtr = (RectangleMarker *)basePtr;

    if ((markerPtr->numWorldPts != 2) || (markerPtr->offScreen)) {
        return;
    }
    if (markerPtr->fill.fgColor != NULL) {

        /*
         * Options:  fg bg
         *                      Draw outline only.
         *           x          Draw solid or stipple.
         *           x  x       Draw solid or stipple.
         */

        /* Create a path to use for both the rectangle and its outline. */
        Blt_Ps_Rectangle2(ps, markerPtr->fillPts[0].x, markerPtr->fillPts[0].y,
                markerPtr->fillPts[1].x, markerPtr->fillPts[1].y);

        /* If the background fill color was specified, draw the rectangle in
         * a solid fashion with that color.  */
        if (markerPtr->fill.bgColor != NULL) {
            /* Draw the solid background as the background layer of the
             * opaque stipple */
            Blt_Ps_XSetBackground(ps, markerPtr->fill.bgColor);
            /* Retain the path. We'll need it for the foreground layer. */
            Blt_Ps_Append(ps, "gsave fill grestore\n");
        }
        Blt_Ps_XSetForeground(ps, markerPtr->fill.fgColor);
        if (markerPtr->stipple != None) {
            /* Draw the stipple in the foreground color. */
            Blt_Ps_XSetStipple(ps, graphPtr->display, markerPtr->stipple);
        } else {
            Blt_Ps_Append(ps, "fill\n");
        }
    }

    /* Draw the outline in the foreground color.  */
    if ((markerPtr->lineWidth > 0) && (markerPtr->outline.fgColor != NULL)) {

        /*  Set up the line attributes.  */
        Blt_Ps_XSetLineAttributes(ps, markerPtr->outline.fgColor,
            markerPtr->lineWidth, &markerPtr->dashes, markerPtr->capStyle,
            markerPtr->joinStyle);

        /*  
         * Define on-the-fly a PostScript macro "DashesProc" that will be
         * executed for each call to the Rectangle drawing routine.  If the
         * line isn't dashed, simply make this an empty definition.
         */
        if ((markerPtr->outline.bgColor != NULL) &&
            (LineIsDashed(markerPtr->dashes))) {
            Blt_Ps_Append(ps, "/DashesProc {\ngsave\n    ");
            Blt_Ps_XSetBackground(ps, markerPtr->outline.bgColor);
            Blt_Ps_Append(ps, "    ");
            Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
            Blt_Ps_Append(ps, "stroke\n  grestore\n} def\n");
        } else {
            Blt_Ps_Append(ps, "/DashesProc {} def\n");
        }
        Blt_Ps_DrawSegments2d(ps, markerPtr->numOutlineSegments,
                markerPtr->outlineSegments);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * RectanglePointProc --
 *
 *---------------------------------------------------------------------------
 */
static int
RectanglePointProc(Marker *basePtr, Point2d *p)
{
    RectangleMarker *markerPtr = (RectangleMarker *)basePtr;

    return ((p->x >= markerPtr->corner1.x) && (p->x < markerPtr->corner2.x) &&
            (p->y >= markerPtr->corner1.y) && (p->y < markerPtr->corner2.y));
}

/*
 *---------------------------------------------------------------------------
 *
 * RectangleAreaProc --
 *
 *---------------------------------------------------------------------------
 */
static int
RectangleAreaProc(Marker *basePtr, Region2d *extsPtr, int enclosed)
{
    RectangleMarker *markerPtr = (RectangleMarker *)basePtr;
    
    if (enclosed) {
        return ((markerPtr->corner1.x >= extsPtr->left) &&
                (markerPtr->corner2.x < extsPtr->right) &&
                (markerPtr->corner1.y >= extsPtr->top) &&
                (markerPtr->corner2.y < extsPtr->bottom));
    } else {
        return ((markerPtr->corner1.x >= extsPtr->right) ||
                (markerPtr->corner2.x < extsPtr->left) ||
                (markerPtr->corner1.y >= extsPtr->bottom) ||
                (markerPtr->corner2.y < extsPtr->top));
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *      Returns a list of marker identifiers in interp->result;
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 3) {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
             link != NULL; link = Blt_Chain_NextLink(link)) {
            Marker *markerPtr;

            markerPtr = Blt_Chain_GetValue(link);
            Tcl_ListObjAppendElement(interp, listObjPtr,
                Tcl_NewStringObj(markerPtr->obj.name, -1));
        }
    } else {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
             link != NULL; link = Blt_Chain_NextLink(link)) {
            Marker *markerPtr;
            int i;

            markerPtr = Blt_Chain_GetValue(link);
            for (i = 3; i < objc; i++) {
                const char *pattern;

                pattern = Tcl_GetString(objv[i]);
                if (Tcl_StringMatch(markerPtr->obj.name, pattern)) {
                    Tcl_ListObjAppendElement(interp, listObjPtr,
                        Tcl_NewStringObj(markerPtr->obj.name, -1));
                    break;
                }
            }
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BindOp --
 *
 *      .g element bind elemName sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
BindOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;

    if (objc == 3) {
        Blt_HashEntry *hp;
        Blt_HashSearch iter;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        for (hp = Blt_FirstHashEntry(&graphPtr->markers.bindTagTable, &iter);
            hp != NULL; hp = Blt_NextHashEntry(&iter)) {
            const char *tag;
            Tcl_Obj *objPtr;

            tag = Blt_GetHashKey(&graphPtr->markers.bindTagTable, hp);
            objPtr = Tcl_NewStringObj(tag, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable,
        Blt_MakeMarkerTag(graphPtr, Tcl_GetString(objv[3])),
        objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Marker *markerPtr;

    if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, 
        markerPtr->classPtr->configSpecs, (char *)markerPtr, objv[4], 0) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 * Side Effects:
 *
 *      pathName marker configure markerName ?option arg?
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Marker *markerPtr;
    MarkerIterator iter;

    if (objc == 3) {
        if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin,
            markerPtr->classPtr->configSpecs, (char *)markerPtr,
            (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
        if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin,
            markerPtr->classPtr->configSpecs, (char *)markerPtr, objv[3], 0);
    }
            
    if (GetMarkerIterator(interp, graphPtr, objv[3], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (markerPtr = FirstTaggedMarker(&iter); markerPtr != NULL; 
         markerPtr = NextTaggedMarker(&iter)) {
        const char *oldName;
        int under;
        
        /* Save the old marker name. */
        oldName = markerPtr->obj.name;
        under = markerPtr->drawUnder;
        if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin,
                markerPtr->classPtr->configSpecs, objc - 4, objv + 4,
                (char *)markerPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
            return TCL_ERROR;
        }
        if (oldName != markerPtr->obj.name) {
            if (RenameMarker(graphPtr, markerPtr, oldName, markerPtr->obj.name)
                != TCL_OK) {
                markerPtr->obj.name = oldName;
                return TCL_ERROR;
            }
        }
        if ((*markerPtr->classPtr->configProc) (markerPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (markerPtr->drawUnder != under) {
            graphPtr->flags |= CACHE_DIRTY;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateOp --
 *
 *      This procedure creates and initializes a new marker.
 *
 * Results:
 *      The return value is a pointer to a structure describing the new
 *      element.  If an error occurred, then the return value is NULL and
 *      an error message is left in interp->result.
 *
 * Side effects:
 *      Memory is allocated, etc.
 *
 *      pathName marker create typeName 
 *---------------------------------------------------------------------------
 */
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Marker *markerPtr;
    Blt_HashEntry *hPtr;
    int isNew;
    ClassId classId;
    int i;
    const char *name;
    char ident[200];
    const char *string;
    char c;

    string = Tcl_GetString(objv[3]);
    c = string[0];
    /* Create the new marker based upon the given type */
    if ((c == 't') && (strcmp(string, "text") == 0)) {
        classId = CID_MARKER_TEXT;
    } else if ((c == 'b') && (strcmp(string, "bitmap") == 0)) {
        classId = CID_MARKER_BITMAP;
    } else if ((c == 'i') && (strcmp(string, "image") == 0)) {
        classId = CID_MARKER_IMAGE;
    } else if ((c == 'l') && (strcmp(string, "line") == 0)) {
        classId = CID_MARKER_LINE;
    } else if ((c == 'p') && (strcmp(string, "polygon") == 0)) {
        classId = CID_MARKER_POLYGON;
    } else if ((c == 'r') && (strcmp(string, "rectangle") == 0)) {
        classId = CID_MARKER_RECTANGLE;
    } else if ((c == 'w') && (strcmp(string, "window") == 0)) {
        classId = CID_MARKER_WINDOW;
    } else {
        Tcl_AppendResult(interp, "unknown marker type \"", string,
                "\": should be bitmap, image, line, polygon, rectangle, text, "
                "or window", (char *)NULL);
        return TCL_ERROR;
    }
    /* Scan for "-name" option. We need it for the component name */
    name = NULL;
    for (i = 4; i < objc; i += 2) {
        int length;

        string = Tcl_GetStringFromObj(objv[i], &length);
        if ((length > 1) && (strncmp(string, "-name", length) == 0)) {
            name = Tcl_GetString(objv[i + 1]);
            break;
        }
    }
    /* If no name was given for the marker, make up one. */
    if (name == NULL) {
        Blt_FormatString(ident, 200, "marker%d", graphPtr->nextMarkerId++);
        name = ident;
    } else if (name[0] == '-') {
        Tcl_AppendResult(interp, "name of marker \"", name, 
                "\" can't start with a '-'", (char *)NULL);
        return TCL_ERROR;
    }
    markerPtr = CreateMarker(graphPtr, name, classId);
    if (Blt_ConfigureComponentFromObj(interp, graphPtr->tkwin, name, 
                markerPtr->obj.className, markerPtr->classPtr->configSpecs, 
                objc - 4, objv + 4, (char *)markerPtr, 0) != TCL_OK) {
        DestroyMarker(markerPtr);
        return TCL_ERROR;
    }
    if ((*markerPtr->classPtr->configProc) (markerPtr) != TCL_OK) {
        DestroyMarker(markerPtr);
        return TCL_ERROR;
    }
    hPtr = Blt_CreateHashEntry(&graphPtr->markers.nameTable, name, &isNew);
    if (!isNew) {
        Marker *oldPtr;
        /*
         * Marker by the same name already exists.  Delete the old marker
         * and it's list entry.  But save the hash entry.
         */
        oldPtr = Blt_GetHashValue(hPtr);
        oldPtr->hashPtr = NULL;
        DestroyMarker(oldPtr);
    }
    Blt_SetHashValue(hPtr, markerPtr);
    markerPtr->hashPtr = hPtr;
    /* Unlike elements, new markers are drawn on top of old markers. */
    markerPtr->link =
        Blt_Chain_Prepend(graphPtr->markers.displayList,markerPtr); 
    if (markerPtr->drawUnder) {
        graphPtr->flags |= CACHE_DIRTY;
    }
    Blt_EventuallyRedrawGraph(graphPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), name, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes the marker given by markerId.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 * Side Effects:
 *      Graph will be redrawn to reflect the new display list.
 *
 *      pathName marker delete ?markerName ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_HashTable delTable;
    Graph *graphPtr = clientData;
    int i;

    Blt_InitHashTable(&delTable, BLT_ONE_WORD_KEYS);
    for (i = 3; i < objc; i++) {
        MarkerIterator iter;
        Marker *markerPtr;
            
        if (GetMarkerIterator(NULL, graphPtr, objv[i], &iter) != TCL_OK) {
            continue;
        }
        for (markerPtr = FirstTaggedMarker(&iter); markerPtr != NULL; 
             markerPtr = NextTaggedMarker(&iter)) {
            Blt_HashEntry *hPtr;
            int isNew;

            hPtr = Blt_CreateHashEntry(&delTable, markerPtr, &isNew);
            Blt_SetHashValue(hPtr, markerPtr);

        }
    }
    if (delTable.numEntries > 0) {
        Blt_HashSearch iter;
        Blt_HashEntry *hPtr;

        for (hPtr = Blt_FirstHashEntry(&delTable, &iter); hPtr != NULL;
             hPtr = Blt_NextHashEntry(&iter)) {
            Marker *markerPtr;

            markerPtr = Blt_GetHashValue(hPtr);
            DestroyMarker(markerPtr);
        }
        Blt_EventuallyRedrawGraph(graphPtr);
    }
    Blt_DeleteHashTable(&delTable);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 *      Find the legend entry from the given argument.  The argument can be
 *      either a screen position "@x,y" or the name of an element.
 *
 *      I don't know how useful it is to test with the name of an element.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Graph will be redrawn to reflect the new legend attributes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GetOp(ClientData clientData, Tcl_Interp *interp, int objc,
      Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    const char *string;

    string = Tcl_GetString(objv[3]);
    if ((string[0] == 'c') && (strcmp(string, "current") == 0)) {
        GraphObj *objPtr;

        objPtr = Blt_GetCurrentItem(graphPtr->bindTable);
        if ((objPtr == NULL) || (objPtr->deleted)) {
            return TCL_OK;              /* No marker is currently picked or
                                         * the current marker has been
                                         * destroyed. */
        }
        if ((objPtr->classId >= CID_MARKER_BITMAP) &&
            (objPtr->classId <= CID_MARKER_WINDOW))         {
            Tcl_SetStringObj(Tcl_GetObjResult(interp), objPtr->name, -1);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RelinkOp --
 *
 *      Reorders the marker (given by the first name) before/after the
 *      another marker (given by the second name) in the marker display
 *      list.  If no second name is given, the marker is placed at the
 *      beginning/end of the list.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Graph will be redrawn to reflect the new display list.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RelinkOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Blt_ChainLink link, place;
    Marker *markerPtr;
    const char *string;

    /* Find the marker to be raised or lowered. */
    if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    /* Right now it's assumed that all markers are always in the display
     * list. */
    link = markerPtr->link;
    Blt_Chain_UnlinkLink(graphPtr->markers.displayList, markerPtr->link);

    place = NULL;
    if (objc == 5) {
        if (GetMarkerFromObj(interp, graphPtr, objv[4], &markerPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        place = markerPtr->link;
    }

    /* Link the marker at its new position. */
    string = Tcl_GetString(objv[2]);
    if ((string[0] == 'l') || (string[0] == 'a')) { /* Lower/after */
        Blt_Chain_LinkAfter(graphPtr->markers.displayList, link, place);
    } else if ((string[0] == 'r') || (string[0] == 'b')) { /* Raise/before */
        Blt_Chain_LinkBefore(graphPtr->markers.displayList, link, place);
    }
    if (markerPtr->drawUnder) {
        graphPtr->flags |= CACHE_DIRTY;
    }
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * FindOp --
 *
 *      Returns if marker by a given ID currently exists.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FindOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Blt_ChainLink link;
    Region2d extents;
    const char *string;
    int enclosed;
    int left, right, top, bottom;
    int mode;

#define FIND_ENCLOSED    (1<<0)
#define FIND_OVERLAPPING (1<<1)
    string = Tcl_GetString(objv[3]);
    if (strcmp(string, "enclosed") == 0) {
        mode = FIND_ENCLOSED;
    } else if (strcmp(string, "overlapping") == 0) {
        mode = FIND_OVERLAPPING;
    } else {
        Tcl_AppendResult(interp, "bad search type \"", string, 
                ": should be \"enclosed\", or \"overlapping\"", (char *)NULL);
        return TCL_ERROR;
    }

    if ((Tcl_GetIntFromObj(interp, objv[4], &left) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[5], &top) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[6], &right) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[7], &bottom) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (left < right) {
        extents.left = (double)left;
        extents.right = (double)right;
    } else {
        extents.left = (double)right;
        extents.right = (double)left;
    }
    if (top < bottom) {
        extents.top = (double)top;
        extents.bottom = (double)bottom;
    } else {
        extents.top = (double)bottom;
        extents.bottom = (double)top;
    }
    enclosed = (mode == FIND_ENCLOSED);
    for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList);
         link != NULL; link = Blt_Chain_NextLink(link)) {
        Marker *markerPtr;

        markerPtr = Blt_Chain_GetValue(link);
        if (markerPtr->flags & HIDDEN) {
            continue;
        }
        if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
            continue;
        }
        if ((*markerPtr->classPtr->regionProc)(markerPtr, &extents, enclosed)) {
            Tcl_Obj *objPtr;

            objPtr = Tcl_GetObjResult(interp);
            Tcl_SetStringObj(objPtr, markerPtr->obj.name, -1);
            return TCL_OK;
        }
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), "", -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *      Returns if marker by a given ID currently exists.
 *
 * Results:
 *      A standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Blt_HashEntry *hPtr;
    const char *name;

    name = Tcl_GetString(objv[3]);
    hPtr = Blt_FindHashEntry(&graphPtr->markers.nameTable, name);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (hPtr != NULL));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagAddOp --
 *
 *      pathName marker tag add tag ?markerName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    const char *tag;
    long markerId;

    tag = Tcl_GetString(objv[4]);
    if (Blt_GetLongFromObj(NULL, objv[4], &markerId) == TCL_OK) {
        Tcl_AppendResult(interp, "bad tag \"", tag, 
                 "\": can't be a number.", (char *)NULL);
        return TCL_ERROR;
    }
    if (strcmp(tag, "all") == 0) {
        Tcl_AppendResult(interp, "can't add reserved tag \"", tag, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    if (objc == 5) {
        /* No nodes specified.  Just add the tag. */
        Blt_Tags_AddTag(&graphPtr->markers.tags, tag);
    } else {
        int i;

        for (i = 5; i < objc; i++) {
            Marker *markerPtr;
            MarkerIterator iter;
            
            if (GetMarkerIterator(interp, graphPtr, objv[i], &iter) != TCL_OK) {
                return TCL_ERROR;
            }
            for (markerPtr = FirstTaggedMarker(&iter); markerPtr != NULL; 
                 markerPtr = NextTaggedMarker(&iter)) {
                Blt_Tags_AddItemToTag(&graphPtr->markers.tags, tag, markerPtr);
            }
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * TagDeleteOp --
 *
 *      pathName marker tag delete tagName ?markerName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    const char *tag;
    long markerId;
    int i;

    tag = Tcl_GetString(objv[4]);
    if (Blt_GetLongFromObj(NULL, objv[4], &markerId) == TCL_OK) {
        Tcl_AppendResult(interp, "bad tag \"", tag, 
                 "\": can't be a number.", (char *)NULL);
        return TCL_ERROR;
    }
    if (strcmp(tag, "all") == 0) {
        Tcl_AppendResult(interp, "can't delete reserved tag \"", tag, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 5; i < objc; i++) {
        Marker *markerPtr;
        MarkerIterator iter;
        
        if (GetMarkerIterator(interp, graphPtr, objv[i], &iter) != TCL_OK) {
            return TCL_ERROR;
        }
        for (markerPtr = FirstTaggedMarker(&iter); markerPtr != NULL; 
             markerPtr = NextTaggedMarker(&iter)) {
            Blt_Tags_RemoveItemFromTag(&graphPtr->markers.tags, tag, markerPtr);
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * TagExistsOp --
 *
 *      Returns the existence of the one or more tags in the given node.
 *      If the node has any the tags, true is return in the interpreter.
 *
 *      pathName marker tag exists markerName ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    MarkerIterator iter;
    Graph *graphPtr = clientData;
    int i;

    if (GetMarkerIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 5; i < objc; i++) {
        Marker *markerPtr;
        const char *tag;

        tag = Tcl_GetString(objv[i]);
        for (markerPtr = FirstTaggedMarker(&iter); markerPtr != NULL; 
             markerPtr = NextTaggedMarker(&iter)) {
            if (Blt_Tags_ItemHasTag(&graphPtr->markers.tags, markerPtr, tag)) {
                Tcl_SetBooleanObj(Tcl_GetObjResult(interp), TRUE);
                return TCL_OK;
            }
        }
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), FALSE);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagForgetOp --
 *
 *      Removes the given tags from all markers.
 *
 *      pathNames marker tag forget ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagForgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    int i;

    for (i = 4; i < objc; i++) {
        const char *tag;
        long markerId;

        tag = Tcl_GetString(objv[i]);
        if (Blt_GetLongFromObj(NULL, objv[i], &markerId) == TCL_OK) {
            Tcl_AppendResult(interp, "bad tag \"", tag, 
                             "\": can't be a number.", (char *)NULL);
            return TCL_ERROR;
        }
        Blt_Tags_ForgetTag(&graphPtr->markers.tags, tag);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagGetOp --
 *
 *      Returns tag names for a given node.  If one of more pattern
 *      arguments are provided, then only those matching tags are returned.
 *
 *      pathName marker tag get markerName ?pattern ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagGetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Marker *markerPtr; 
    MarkerIterator iter;
    Tcl_Obj *listObjPtr;

    if (GetMarkerIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (markerPtr = FirstTaggedMarker(&iter); markerPtr != NULL; 
         markerPtr = NextTaggedMarker(&iter)) {
        if (objc == 5) {
            Blt_Tags_AppendTagsToObj(&graphPtr->markers.tags, markerPtr, listObjPtr);
            Tcl_ListObjAppendElement(interp, listObjPtr, 
                                     Tcl_NewStringObj("all", 3));
        } else {
            int i;
            
            /* Check if we need to add the special tags "all" */
            for (i = 5; i < objc; i++) {
                const char *pattern;

                pattern = Tcl_GetString(objv[i]);
                if (Tcl_StringMatch("all", pattern)) {
                    Tcl_Obj *objPtr;

                    objPtr = Tcl_NewStringObj("all", 3);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                    break;
                }
            }
            /* Now process any standard tags. */
            for (i = 5; i < objc; i++) {
                Blt_ChainLink link;
                const char *pattern;
                Blt_Chain chain;

                chain = Blt_Chain_Create();
                Blt_Tags_AppendTagsToChain(&graphPtr->markers.tags, markerPtr, chain);
                pattern = Tcl_GetString(objv[i]);
                for (link = Blt_Chain_FirstLink(chain); link != NULL; 
                     link = Blt_Chain_NextLink(link)) {
                    const char *tag;
                    Tcl_Obj *objPtr;

                    tag = (const char *)Blt_Chain_GetValue(link);
                    if (!Tcl_StringMatch(tag, pattern)) {
                        continue;
                    }
                    objPtr = Tcl_NewStringObj(tag, -1);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                }
                Blt_Chain_Destroy(chain);
            }
        }    
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagNamesOp --
 *
 *      Returns the names of all the tags in the markerset.  If one of more
 *      node arguments are provided, then only the tags found in those
 *      nodes are returned.
 *
 *      pathName marker tag names ?markerName ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewStringObj("all", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    if (objc == 4) {
        Blt_Tags_AppendAllTagsToObj(&graphPtr->markers.tags, listObjPtr);
    } else {
        Blt_HashTable uniqTable;
        int i;

        Blt_InitHashTable(&uniqTable, BLT_STRING_KEYS);
        for (i = 4; i < objc; i++) {
            MarkerIterator iter;
            Marker *markerPtr;

            if (GetMarkerIterator(interp, graphPtr, objPtr, &iter) != TCL_OK) {
                goto error;
            }
            for (markerPtr = FirstTaggedMarker(&iter); markerPtr != NULL; 
                 markerPtr = NextTaggedMarker(&iter)) {
                Blt_ChainLink link;
                Blt_Chain chain;

                chain = Blt_Chain_Create();
                Blt_Tags_AppendTagsToChain(&graphPtr->markers.tags, markerPtr, chain);
                for (link = Blt_Chain_FirstLink(chain); link != NULL; 
                     link = Blt_Chain_NextLink(link)) {
                    const char *tag;
                    int isNew;

                    tag = Blt_Chain_GetValue(link);
                    Blt_CreateHashEntry(&uniqTable, tag, &isNew);
                }
                Blt_Chain_Destroy(chain);
            }
        }
        {
            Blt_HashEntry *hPtr;
            Blt_HashSearch hiter;

            for (hPtr = Blt_FirstHashEntry(&uniqTable, &hiter); hPtr != NULL;
                 hPtr = Blt_NextHashEntry(&hiter)) {
                objPtr = Tcl_NewStringObj(Blt_GetHashKey(&uniqTable, hPtr), -1);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        }
        Blt_DeleteHashTable(&uniqTable);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
 error:
    Tcl_DecrRefCount(listObjPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagSetOp --
 *
 *      Sets one or more tags for a given marker.  Tag names can't start
 *      with a digit (to distinquish them from node ids) and can't be a
 *      reserved tag ("all").
 *
 *      pathName marker tag set markerName ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    int i;
    MarkerIterator iter;

    if (GetMarkerIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 5; i < objc; i++) {
        const char *tag;
        Marker *markerPtr;
        long markerId;

        tag = Tcl_GetString(objv[i]);
        if (Blt_GetLongFromObj(NULL, objv[i], &markerId) == TCL_OK) {
            Tcl_AppendResult(interp, "bad tag \"", tag, 
                             "\": can't be a number.", (char *)NULL);
            return TCL_ERROR;
        }
        if (strcmp(tag, "all") == 0) {
            Tcl_AppendResult(interp, "can't add reserved tag \"", tag, "\"",
                             (char *)NULL);     
            return TCL_ERROR;
        }
        for (markerPtr = FirstTaggedMarker(&iter); markerPtr != NULL; 
             markerPtr = NextTaggedMarker(&iter)) {
            Blt_Tags_AddItemToTag(&graphPtr->markers.tags, tag, markerPtr);
        }    
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagUnsetOp --
 *
 *      Removes one or more tags from a given marker. If a tag doesn't
 *      exist or is a reserved tag ("all"), nothing will be done and no
 *      error message will be returned.
 *
 *      pathName marker tag unset markerName ?tag ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TagUnsetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Marker *markerPtr;
    MarkerIterator iter;

    if (GetMarkerIterator(interp, graphPtr, objv[4], &iter) != TCL_OK) {
        return TCL_ERROR;
    }
    for (markerPtr = FirstTaggedMarker(&iter); markerPtr != NULL; 
         markerPtr = NextTaggedMarker(&iter)) {
        int i;

        for (i = 5; i < objc; i++) {
            const char *tag;

            tag = Tcl_GetString(objv[i]);
            Blt_Tags_RemoveItemFromTag(&graphPtr->markers.tags, tag, markerPtr);
        }    
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagOp --
 *
 *      This procedure is invoked to process tag operations.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      See the user documentation.
 *
 *      pathName marker tag op args
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec tagOps[] =
{
    {"add",     1, TagAddOp,      3, 0, "tagName ?markerName ...?",},
    {"delete",  1, TagDeleteOp,   3, 0, "?markerName ...?",},
    {"exists",  1, TagExistsOp,   5, 0, "markerName  ?tag ...?",},
    {"forget",  1, TagForgetOp,   4, 0, "?tag ...?",},
    {"get",     1, TagGetOp,      5, 0, "markerName ?pattern ...?",},
    {"names",   1, TagNamesOp,    4, 0, "?markerName ...?",},
    {"set",     1, TagSetOp,      5, 0, "markerName ?tag ...?",},
    {"unset",   1, TagUnsetOp,    5, 0, "markerName ?tag ...?",},
};

static int numTagOps = sizeof(tagOps) / sizeof(Blt_OpSpec);

static int
TagOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numTagOps, tagOps, BLT_OP_ARG3,
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc)(clientData, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 *      Returns a symbolic name for the type of the marker whose ID is
 *      given.
 *
 * Results:
 *      A standard TCL result. interp->result will contain the symbolic
 *      type of the marker.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Marker *markerPtr;
    const char *type;

    if (GetMarkerFromObj(interp, graphPtr, objv[3], &markerPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    switch (markerPtr->obj.classId) {
    case CID_MARKER_BITMAP:
        type = "bitmap";        break;
    case CID_MARKER_IMAGE:
        type = "image";         break;
    case CID_MARKER_LINE:
        type = "line";          break;
    case CID_MARKER_POLYGON:
        type = "polygon";       break;
    case CID_MARKER_RECTANGLE:
        type = "rectangle";     break;
    case CID_MARKER_TEXT:
        type = "text";          break;
    case CID_MARKER_WINDOW:
        type = "window";        break;
    default:
        type = "???";           break;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), type, -1);
    return TCL_OK;
}

/* Public routines */

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MarkerOp --
 *
 *      This procedure is invoked to process the TCL command that
 *      corresponds to a widget managed by this module.  See the user
 *      documentation for details on what it does.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

static Blt_OpSpec markerOps[] =
{
    {"after",     1, RelinkOp, 4, 5, "markerName ?afterName?",},
    {"before",    1, RelinkOp, 4, 5, "markerName ?beforeName?",},
    {"bind",      1, BindOp,   3, 6, "bindTag sequence command",},
    {"cget",      2, CgetOp,   5, 5, "markerName option",},
    {"configure", 2, ConfigureOp, 4, 0,"markerName ?option value ...?",},
    {"create",    2, CreateOp, 4, 0, "markerType ?option value ...?",},
    {"delete",    1, DeleteOp, 3, 0, "?markerName ...?",},
    {"exists",    1, ExistsOp, 4, 4, "markerName",},
    {"find",      1, FindOp,   8, 8, "enclosed|overlapping x1 y1 x2 y2",},
    {"get",       1, GetOp,    4, 4, "markerName",},
    {"lower",     1, RelinkOp, 4, 5, "markerName ?afterName?",},
    {"names",     1, NamesOp,  3, 0, "?pattern?...",},
    {"raise",     1, RelinkOp, 4, 5, "markerName ?beforeName?",},
    {"tag",       2, TagOp,    3, 0, "args...",},
    {"type",      2, TypeOp,   4, 4, "markerName",},
};
static int numMarkerOps = sizeof(markerOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
int
Blt_MarkerOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numMarkerOps, markerOps, BLT_OP_ARG2, 
        objc, objv,0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc)(clientData, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MarkersToPostScript --
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MarkersToPostScript(Graph *graphPtr, Blt_Ps ps, int under)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(graphPtr->markers.displayList); 
         link != NULL; link = Blt_Chain_PrevLink(link)) {
        Marker *markerPtr;

        markerPtr = Blt_Chain_GetValue(link);
        if ((markerPtr->classPtr->psProc == NULL) || 
            (markerPtr->numWorldPts == 0)) {
            continue;
        }
        if (markerPtr->drawUnder != under) {
            continue;
        }
        if (markerPtr->flags & HIDDEN) {
            continue;
        }
        if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
            continue;
        }
        Blt_Ps_VarAppend(ps, "\n% Marker \"", markerPtr->obj.name, 
                "\" is a ", markerPtr->obj.className, ".\n", (char *)NULL);
        (*markerPtr->classPtr->psProc) (markerPtr, ps);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawMarkers --
 *
 *      Calls the individual drawing routines (based on marker type) for
 *      each marker in the display list.
 *
 *      A marker will not be drawn if
 *
 *      1) An element linked to the marker (indicated by elemName) is
 *         currently hidden.
 *
 *      2) No coordinates have been specified for the marker.
 *
 *      3) The marker is requesting to be drawn at a different level
 *         (above/below the elements) from the current mode.
 *
 *      4) The marker is configured as hidden (-hide option).
 *
 *      5) The marker isn't visible in the current viewport (i.e. clipped).
 *
 * Results:
 *      None
 *
 * Side Effects:
 *      Markers are drawn into the drawable (pixmap) which will eventually
 *      be displayed in the graph window.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DrawMarkers(Graph *graphPtr, Drawable drawable, int under)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(graphPtr->markers.displayList); 
         link != NULL; link = Blt_Chain_PrevLink(link)) {
        Marker *markerPtr;

        markerPtr = Blt_Chain_GetValue(link);

        if ((markerPtr->numWorldPts == 0) || 
            (markerPtr->drawUnder != under) ||
            (markerPtr->offScreen) ||
            (markerPtr->flags & HIDDEN)) {
            continue;
        }
        if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
            continue;
        }
        (*markerPtr->classPtr->drawProc) (markerPtr, drawable);
    }
}

void
Blt_ConfigureMarkers(Graph *graphPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
         link != NULL; link = Blt_Chain_NextLink(link)) {
        Marker *markerPtr;

        markerPtr = Blt_Chain_GetValue(link);
        (*markerPtr->classPtr->configProc) (markerPtr);
    }
}

void
Blt_MapMarkers(Graph *graphPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList); 
         link != NULL; link = Blt_Chain_NextLink(link)) {
        Marker *markerPtr;

        markerPtr = Blt_Chain_GetValue(link);
        if (markerPtr->numWorldPts == 0) {
            continue;
        }
        if (markerPtr->flags & HIDDEN) {
            continue;
        }
        if ((graphPtr->flags & MAP_ALL) || (markerPtr->flags & MAP_ITEM)) {
            (*markerPtr->classPtr->mapProc) (markerPtr);
            markerPtr->flags &= ~MAP_ITEM;
        }
    }
}

void
Blt_DestroyMarkers(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&graphPtr->markers.nameTable, &iter); 
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        Marker *markerPtr;

        markerPtr = Blt_GetHashValue(hPtr);
        /*
         * Dereferencing the pointer to the hash table prevents the hash
         * table entry from being automatically deleted.
         */
        markerPtr->hashPtr = NULL;
        DestroyMarker(markerPtr);
    }
    Blt_DeleteHashTable(&graphPtr->markers.nameTable);
    Blt_DeleteHashTable(&graphPtr->markers.bindTagTable);
    Blt_Tags_Init(&graphPtr->markers.tags);
    Blt_Chain_Destroy(graphPtr->markers.displayList);
}

Marker *
Blt_NearestMarker(
    Graph *graphPtr,
    int x, int y,                       /* Screen coordinates */
    int under)
{
    Blt_ChainLink link;
    Point2d point;

    point.x = (double)x;
    point.y = (double)y;
    for (link = Blt_Chain_FirstLink(graphPtr->markers.displayList);
        link != NULL; link = Blt_Chain_NextLink(link)) {
        Marker *markerPtr;

        markerPtr = Blt_Chain_GetValue(link);
        if ((markerPtr->numWorldPts == 0) ||
            (markerPtr->flags & (HIDDEN|MAP_ITEM))) {
            continue;                   /* Don't consider markers that are
                                         * pending to be mapped. Even if
                                         * the marker has already been
                                         * mapped, the coordinates could be
                                         * invalid now.  Better to pick no
                                         * marker than the wrong marker. */

        }
        if ((markerPtr->elemName != NULL) && (IsElementHidden(markerPtr))) {
            continue;
        }
        if ((markerPtr->drawUnder == under) && 
            (markerPtr->state == STATE_NORMAL)) {
            if ((*markerPtr->classPtr->pointProc) (markerPtr, &point)) {
                return markerPtr;
            }
        }
    }
    return NULL;
}

ClientData
Blt_MakeMarkerTag(Graph *graphPtr, const char *tagName)
{
    Blt_HashEntry *hPtr;
    int isNew;

    assert(tagName != NULL);
    hPtr = Blt_CreateHashEntry(&graphPtr->markers.bindTagTable, tagName, 
        &isNew);
    return Blt_GetHashKey(&graphPtr->markers.bindTagTable, hPtr);
}


#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * ConfigureArrows --
 *
 *      If arrowheads have been requested for a line, this procedure makes
 *      arrangements for the arrowheads.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 * Side effects:
 *      Information in linePtr is set up for one or two arrowheads.  the
 *      firstArrowPtr and lastArrowPtr polygons are allocated and
 *      initialized, if need be, and the end points of the line are
 *      adjusted so that a thick line doesn't stick out past the
 *      arrowheads.
 *
 *---------------------------------------------------------------------------
 */

        /* ARGSUSED */
static int
ConfigureArrows(canvas, linePtr)
     Tk_Canvas canvas;                  /* Canvas in which arrows will be
                                         * displayed (interp and tkwin
                                         * fields are needed). */
    LineItem *linePtr;                  /* Item to configure for arrows. */
{
    double *poly, *coordPtr;
    double dx, dy, length, sinTheta, cosTheta, temp;
    double fracHeight;                  /* Line width as fraction of
                                         * arrowhead width. */
    double backup;                      /* Distance to backup end points so
                                         * the line ends in the middle of
                                         * the arrowhead. */
    double vertX, vertY;                /* Position of arrowhead vertex. */
    double shapeA, shapeB, shapeC;      /* Adjusted coordinates (see
                                         * explanation below). */
    double width;
    Tk_State state = linePtr->header.state;

    if (linePtr->numPoints <2) {
        return TCL_OK;
    }

    if(state == TK_STATE_NULL) {
        state = ((TkCanvas *)canvas)->canvas_state;
    }

    width = linePtr->outline.width;
    if (((TkCanvas *)canvas)->currentItemPtr == (Tk_Item *)linePtr) {
        if (linePtr->outline.activeWidth>width) {
            width = linePtr->outline.activeWidth;
        }
    } else if (state==TK_STATE_DISABLED) {
        if (linePtr->outline.disabledWidth>0) {
            width = linePtr->outline.disabledWidth;
        }
    }

    /*
     * The code below makes a tiny increase in the shape parameters for the
     * line.  This is a bit of a hack, but it seems to result in displays
     * that more closely approximate the specified parameters.  Without the
     * adjustment, the arrows come out smaller than expected.
     */

    shapeA = linePtr->arrowShapeA + 0.001;
    shapeB = linePtr->arrowShapeB + 0.001;
    shapeC = linePtr->arrowShapeC + width/2.0 + 0.001;

    /*
     * If there's an arrowhead on the first point of the line, compute its
     * polygon and adjust the first point of the line so that the line
     * doesn't stick out past the leading edge of the arrowhead.
     */

    fracHeight = (width/2.0)/shapeC;
    backup = fracHeight*shapeB + shapeA*(1.0 - fracHeight)/2.0;
    if (linePtr->arrow != ARROWS_LAST) {
        poly = linePtr->firstArrowPtr;
        if (poly == NULL) {
            poly = (double *) ckalloc((unsigned)
                    (2*PTS_IN_ARROW*sizeof(double)));
            poly[0] = poly[10] = linePtr->coordPtr[0];
            poly[1] = poly[11] = linePtr->coordPtr[1];
            linePtr->firstArrowPtr = poly;
        }
        dx = poly[0] - linePtr->coordPtr[2];
        dy = poly[1] - linePtr->coordPtr[3];
        length = hypot(dx, dy);
        if (length == 0) {
            sinTheta = cosTheta = 0.0;
        } else {
            sinTheta = dy/length;
            cosTheta = dx/length;
        }
        vertX = poly[0] - shapeA*cosTheta;
        vertY = poly[1] - shapeA*sinTheta;
        temp = shapeC*sinTheta;
        poly[2] = poly[0] - shapeB*cosTheta + temp;
        poly[8] = poly[2] - 2*temp;
        temp = shapeC*cosTheta;
        poly[3] = poly[1] - shapeB*sinTheta - temp;
        poly[9] = poly[3] + 2*temp;
        poly[4] = poly[2]*fracHeight + vertX*(1.0-fracHeight);
        poly[5] = poly[3]*fracHeight + vertY*(1.0-fracHeight);
        poly[6] = poly[8]*fracHeight + vertX*(1.0-fracHeight);
        poly[7] = poly[9]*fracHeight + vertY*(1.0-fracHeight);

        /*
         * Polygon done.  Now move the first point towards the second so
         * that the corners at the end of the line are inside the
         * arrowhead.
         */

        linePtr->coordPtr[0] = poly[0] - backup*cosTheta;
        linePtr->coordPtr[1] = poly[1] - backup*sinTheta;
    }

    /*
     * Similar arrowhead calculation for the last point of the line.
     */

    if (linePtr->arrow != ARROWS_FIRST) {
        coordPtr = linePtr->coordPtr + 2*(linePtr->numPoints-2);
        poly = linePtr->lastArrowPtr;
        if (poly == NULL) {
            poly = (double *) ckalloc((unsigned)
                    (2*PTS_IN_ARROW*sizeof(double)));
            poly[0] = poly[10] = coordPtr[2];
            poly[1] = poly[11] = coordPtr[3];
            linePtr->lastArrowPtr = poly;
        }
        dx = poly[0] - coordPtr[0];
        dy = poly[1] - coordPtr[1];
        length = hypot(dx, dy);
        if (length == 0) {
            sinTheta = cosTheta = 0.0;
        } else {
            sinTheta = dy/length;
            cosTheta = dx/length;
        }
        vertX = poly[0] - shapeA*cosTheta;
        vertY = poly[1] - shapeA*sinTheta;
        temp = shapeC*sinTheta;
        poly[2] = poly[0] - shapeB*cosTheta + temp;
        poly[8] = poly[2] - 2*temp;
        temp = shapeC*cosTheta;
        poly[3] = poly[1] - shapeB*sinTheta - temp;
        poly[9] = poly[3] + 2*temp;
        poly[4] = poly[2]*fracHeight + vertX*(1.0-fracHeight);
        poly[5] = poly[3]*fracHeight + vertY*(1.0-fracHeight);
        poly[6] = poly[8]*fracHeight + vertX*(1.0-fracHeight);
        poly[7] = poly[9]*fracHeight + vertY*(1.0-fracHeight);
        coordPtr[2] = poly[0] - backup*cosTheta;
        coordPtr[3] = poly[1] - backup*sinTheta;
    }

    return TCL_OK;
}
#endif


/*
 * bltGrContour.c --
 *
 * This module implements contour elements for the BLT graph widget.
 *
 *	Copyright 2011 George A Howlett.
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

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <X11/Xutil.h>
#include "bltAlloc.h"
#include "bltMath.h"
#include "bltPool.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltBind.h"
#include "bltBg.h"
#include "bltOp.h"
#include "bltImage.h"
#include "bltBitmap.h"
#include "bltPicture.h"
#include "bltPainter.h"
#include "tkDisplay.h"
#include "bltPs.h"
#include "bltGraph.h"
#include "bltPicture.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"
#include "bltGrElem.h"
#include "bltGrMesh.h"

/* Use to compute symbol for isolines. */
#define SQRT_PI		1.77245385090552
#define S_RATIO		0.886226925452758

/* Special color values for isolines. */
#define COLOR_PALETTE	(XColor *)2
#define ALLOW_DEFAULT	1
#define ISALIASED(c)    (((c)==COLOR_DEFAULT) || ((c)==COLOR_PALETTE))

#define PEN(e)		((((e)->penPtr == NULL) ? \
			  (e)->builtinPenPtr : (e)->penPtr))

/*
 * XDrawLines() points: XMaxRequestSize(dpy) - 3
 * XFillPolygon() points:  XMaxRequestSize(dpy) - 4
 * XDrawSegments() segments:  (XMaxRequestSize(dpy) - 3) / 2
 * XDrawRectangles() rectangles:  (XMaxRequestSize(dpy) - 3) / 2
 * XFillRectangles() rectangles:  (XMaxRequestSize(dpy) - 3) / 2
 * XDrawArcs() or XFillArcs() arcs:  (XMaxRequestSize(dpy) - 3) / 3
 */

#define MAX_DRAWPOINTS(d)	Blt_MaxRequestSize(d, sizeof(XPoint))
#define MAX_DRAWLINES(d)	Blt_MaxRequestSize(d, sizeof(XPoint))
#define MAX_DRAWPOLYGON(d)	Blt_MaxRequestSize(d, sizeof(XPoint))
#define MAX_DRAWSEGMENTS(d)	Blt_MaxRequestSize(d, sizeof(XSegment))
#define MAX_DRAWRECTANGLES(d)	Blt_MaxRequestSize(d, sizeof(XRectangle))
#define MAX_DRAWARCS(d)		Blt_MaxRequestSize(d, sizeof(XArc))

#ifdef WIN32

static int tkpWinRopModes[] =
{
    R2_BLACK,				/* GXclear */
    R2_MASKPEN,				/* GXand */
    R2_MASKPENNOT,			/* GXandReverse */
    R2_COPYPEN,				/* GXcopy */
    R2_MASKNOTPEN,			/* GXandInverted */
    R2_NOT,				/* GXnoop */
    R2_XORPEN,				/* GXxor */
    R2_MERGEPEN,			/* GXor */
    R2_NOTMERGEPEN,			/* GXnor */
    R2_NOTXORPEN,			/* GXequiv */
    R2_NOT,				/* GXinvert */
    R2_MERGEPENNOT,			/* GXorReverse */
    R2_NOTCOPYPEN,			/* GXcopyInverted */
    R2_MERGENOTPEN,			/* GXorInverted */
    R2_NOTMASKPEN,			/* GXnand */
    R2_WHITE				/* GXset */
};

#endif

/* Trace flags. */
#define RECOUNT		(1<<10)		/* Trace needs to be fixed. */

/* Flags for trace's point and segments. */
#define VISIBLE		(1<<0)		/* Point is on visible on screen. */
#define KNOT		(1<<1)		/* Point is a knot, original data 
					 * point. */
#define SYMBOL		(1<<2)		/* Point is designated to have a
					 * symbol. This is only used when
					 * reqMaxSymbols is non-zero. */
#define ACTIVE_POINT	(1<<3)		/* Point is active. This is only used
					 * when numActivePoints is greater than
					 * zero. */
#define ISOLINES	(1<<12)		/* Draw the isolines on top of the
					 * mesh. */
#define COLORMAP	(1<<13)		/* Fill the triangles of the mesh. */
#define HULL		(1<<14)		/* Draw the convex hull representing
					 * the outer boundary of the mesh. */
#define EDGES		(1<<15)		/* Draw the edges of the triangular 
					 * mesh. */
#define VALUES		(1<<16)		/* Draw the z-values at the vertices
					 * of the mesh. */
#define SYMBOLS		(1<<17)		/* Draw the symbols on top of the
					 * mesh. */

#define DRAWN(t,f)	(((f) & (t)->drawFlags) == (t)->drawFlags)
#define imul8x8(a,b,t)	((t) = (a)*(b)+128,(((t)+((t)>>8))>>8))
#define CLAMP(c)	((((c) < 0) ? 0 : ((c) > 255) ? 255 : (c)))

#define Ax elemPtr->vertices[t->a].x
#define Bx elemPtr->vertices[t->b].x
#define Cx elemPtr->vertices[t->c].x
#define Ay elemPtr->vertices[t->a].y
#define By elemPtr->vertices[t->b].y
#define Cy elemPtr->vertices[t->c].y
#define Az elemPtr->vertices[t->a].z
#define Bz elemPtr->vertices[t->b].z
#define Cz elemPtr->vertices[t->c].z

/* 
 * .c element create 
 * .c level create name 0 0 -min -max -foreground -linewidth -background -color 
 * .c level configure 
 * .c level closest x y 
 * .c element closest x y 
 * .c element level add x y z 
 *
 *  .g element create 
 *  .g mesh create 
 *  .g element isoline create $element $name  
 *  .g element isoline configure $element $name  
 *  .g element isoline cget $element $name  
 *  .g element isoline delete $element $name $name $name
 *  .g element isoline names $element $pattern
 *  .g element isoline activate $element $name
 *  .g element isoline deactivate $element $name
 *  .g element isoline deactivate $element $name
 */

typedef struct _ContourElement ContourElement;
typedef struct _ContourPen ContourPen;
typedef struct _Blt_Picture Pict;

/* 
 * Vertex -- 
 *
 *	Represents a vertex of a triangle in the mesh. It contains the
 *	converted screen coordinates of the point and an index back into the
 *	array of field values (z) and x and y coordinate arrays.  It also
 *	contains the associated interpolated color for this point.
 */
typedef struct {
    int index;				/* Index to the array of values (also
					 * original x and y coordinates). */
    unsigned int flags;			/* Flags for vertex. */
    float x, y, z;			/* Screen coordinates of this point in
					 * the mesh and it's normalized [0..1]
					 * value. */
    Blt_Pixel color;			/* Color at this vertex. */
}  Vertex;

typedef struct {
    int a, b, c;			/* Indices of the vectices that 
					 * form the triangle. */
    float min, max;			/* Minimum and maximum field values,
					 * used to sort the triangles. */
    unsigned int flags;
    int index;
} Triangle;

typedef struct {
    int a, b; 				/* Indices of the vertices that form
					 * the edge. */
    int64_t A, B, C;			/* Coefficents of edge equation. */
} Edge;

typedef struct {
    int a, b; 				/* Indices of the vertices that form
					 * the edge. */
} EdgeKey;

typedef struct {
    float x, y;				/* Coordinates of point. */
} PointKey;

/* 
 * TracePoint --
 *
 *	Use to represent the hull around the mesh.
 */
typedef struct _TracePoint {
    struct _TracePoint *next;		/* Pointer to next point in the
					 * trace. */
    float x, y;				/* Screen coordinate of the
					 * interpolated point. */
    int index;
    unsigned int flags;			/* Flags associated with a point are
					 * described below. */
} TracePoint;

/* 
 * TraceSegment --
 * 
 *	Represents an individual line segment of a isoline. 
 */
typedef struct _TraceSegment {
    struct _TraceSegment *next;		/* Pointer to next point in trace. */
    float x1, y1, x2, y2;		/* Screen coordinate of the point. */
    int index;				/* Index of this coordinate pointing
					 * back to the raw world values in the
					 * individual data arrays. This index
					 * is replicated for generated
					 * values. */
    unsigned int flags;			/* Flags associated with a segment are
					 * described below. */
} TraceSegment;

/* 
 * Trace -- 
 *
 *	Represents a polyline of connected line segments using the same
 *	line and symbol style.  They are stored in a chain of traces.
 */
typedef struct {
    ContourElement *elemPtr;
    TracePoint *head, *tail;
    int numPoints;			/* # of points in the trace. */
    Blt_ChainLink link;			/* Pointer of this entry in the chain
					 * of traces. */
    ContourPen *penPtr;
    unsigned short flags;		/* Flags associated with a trace are
					 * described blow. */
    unsigned short drawFlags;		/* Flags for individual points and 
					 * segments when drawing the trace. */
} Trace;

/* Symbol types for isolines. */
typedef enum {
    SYMBOL_NONE,
    SYMBOL_SQUARE,
    SYMBOL_CIRCLE,
    SYMBOL_DIAMOND,
    SYMBOL_PLUS,
    SYMBOL_CROSS,
    SYMBOL_SPLUS,
    SYMBOL_SCROSS,
    SYMBOL_TRIANGLE,
    SYMBOL_ARROW,
    SYMBOL_BITMAP,
    SYMBOL_IMAGE
} SymbolType;

typedef struct {
    const char *name;
    int minChars;
    SymbolType type;
} SymbolTable;

typedef struct {
    SymbolType type;			/* Type of symbol to be drawn/printed */
    int size;				/* Requested size of symbol in pixels */
    XColor *outlineColor;		/* Outline color */
    int outlineWidth;			/* Width of the outline */
    GC outlineGC;			/* Outline graphics context */
    XColor *fillColor;			/* Normal fill color */
    GC fillGC;				/* Fill graphics context */

    Tk_Image image;			/* This is used of image symbols.  */

    /* The last two fields are used only for bitmap symbols. */
    Pixmap bitmap;			/* Bitmap to determine
					 * foreground/background pixels of the
					 * symbol */
    Pixmap mask;			/* Bitmap representing the transparent
					 * pixels of the symbol */
} Symbol;

typedef struct {
    GraphObj obj;
    ContourElement *elemPtr;		/* Element this isoline belongs to. */
    unsigned int flags;
    const char *label;			/* Label to be displayed for isoline. */
    double reqValue;			/* Requested isoline value.  Could be
					 * either absolute or relative. */
    double reqMin, reqMax;

    Blt_HashEntry *hashPtr;
    ContourPen *penPtr;
    ContourPen *activePenPtr;
    double value;			/* Value of the isoline. */
    Blt_Chain traces;			/* Set of traces that describe the
					 * polyline(s) that represent the
					 * isoline. */
    Blt_HashTable edgeTable;
    Blt_HashTable pointTable;
    TraceSegment *segments;		/* Segments used for isolnes. */
    int numSegments;
    Blt_Pixel paletteColor;
} Isoline;

/* HIDE			(1<<0) */
#define ABSOLUT		(1<<4)
/* ACTIVE		(1<<6) */

struct _ContourPen {
    const char *name;			/* Pen style identifier.  If NULL pen
					 * was statically allocated. */
    ClassId classId;			/* Type of pen */
    const char *typeId;			/* String token identifying the type of
					 * pen */
    unsigned int flags;			/* Indicates if the pen element is
					 * active or normal */
    int refCount;			/* Reference count for elements using
					 * this pen. */
    Blt_HashEntry *hashPtr;

    Blt_ConfigSpec *configSpecs;	/* Configuration specifications */

    PenConfigureProc *configProc;
    PenDestroyProc *destroyProc;
    Graph *graphPtr;			/* Graph that the pen is associated
					 * with. */

    /* Symbol attributes. */
    Symbol symbol;			/* Element symbol type */

    /* Trace attributes. */
    Blt_Dashes traceDashes;		/* Dash on-off list value */
    XColor *traceColor;			/* Line segment color */
    XColor *traceOffColor;		/* Line segment dash gap color */
    GC traceGC;				/* Line segment graphics context */
    int traceWidth;			/* Width of the line segments. If
					 * lineWidth is 0, no line will be
					 * drawn, only symbols. */

    /* Show value attributes. */
    unsigned int valueFlags;		/* Indicates whether to display text
					 * of the data value.  Values are x,
					 * y, both, or none. */
    const char *valueFormat;		/* A printf format string. */
    TextStyle valueStyle;		/* Text attributes (color, font,
					 * rotation, etc.) of the value. */
};

struct _ContourElement {
    GraphObj obj;			/* Must be first field in element. */
    unsigned int flags;		
    Blt_HashEntry *hashPtr;

    /* Fields specific to elements. */
    Blt_ChainLink link;			/* Element's link in display list. */
    const char *label;			/* Label displayed in legend. There
					 * may be sub-labels for each contour
					 * range/value. */
    unsigned short row, col;		/* Position of the entry in the
					 * legend. */
    int legendRelief;			/* Relief of label in legend. */
    Axis2d axes;			/* X-axis and Y-axis mapping the
					 * element */
    ElemValues z, dummy1, w;		/* Contains array of floating point
					 * graph coordinate values. Also holds
					 * min/max and the number of
					 * coordinates */
    Blt_HashTable activeTable;		/* Table of indices which indicate
					 * which data points are active (drawn
					 * with "active" colors). */
    int numActiveIndices;		/* # of active data points.  Special
					 * case: if numActiveIndices < 0 and the
					 * active bit is set in "flags",
					 * then all data * points are drawn
					 * active. */
    ElementProcs *procsPtr;
    Blt_ConfigSpec *configSpecs;	/* Configuration specifications. */
    ContourPen *activePenPtr;		/* Standard Pens */
    ContourPen *penPtr;
    ContourPen *builtinPenPtr;
    Blt_Chain dummy2;			/* Placeholder: Palette of pens. */

    int scaleSymbols;			/* If non-zero, the symbols will scale
					 * in size as the graph is zoomed
					 * in/out.  */

    double xRange, yRange;		/* Initial X-axis and Y-axis ranges:
					 * used to scale the size of element's
					 * symbol. */
    int state;

    ContourPen builtinPen;

    Axis *zAxisPtr;


    int reqMaxSymbols;			/* Indicates the interval the draw
					 * symbols.  Zero (and one) means draw
					 * all symbols. */

    Mesh *meshPtr;			/* Mesh associated with contour data
					 * set. */
    Blt_Pool pointPool;			/* Pool of the points used in the
					 * traces formed by the isolines. */
    Blt_Pool segmentPool;
    Blt_HashTable isoTable;		/* Table of isolines to be
					 * displayed. */
    Blt_Chain traces;			/* List of traces representing the 
					 * boundary of the contour.  */
    Vertex *vertices;			/* Vertices of mesh converted to
					 * screen coordinates. */
    Triangle *triangles;		/* Triangles of the */
    Segment2d *wires;			/* Segments (in screen coordinates)
					 * forming the wireframe of the
					 * mesh.  */
    int numWires;			/* # of segments in above array. */
    int numVertices;			/* # of vertices in above array. */
    int numTriangles;			/* # of triangles in the above array. */
    int nextIsoline;
    const char *valueFormat;		/* A printf format string. */
    TextStyle valueStyle;		/* Text attributes (color, font,
					 * rotation, etc.) of the value. */
    ContourPen *boundaryPenPtr;
    Blt_Picture picture;
    Blt_Painter painter;
    float opacity;			/* Global alpha to be used.  By
					 * default all triangles are
					 * opaque. * */
    Isoline *activePtr;
    Blt_HashTable tagTable;		/* Table of tags. */

    /* Mesh attributes. */
    Blt_Dashes meshDashes;		/* Dash on-off list value */
    XColor *meshColor;			/* Line segment color */
    XColor *meshOffColor;		/* Line segment dash gap color */
    GC meshGC;				/* Line segment graphics context */
    int meshWidth;			/* Width of the line segments. If
					 * lineWidth is 0, no line will be
					 * drawn, only symbols. */
    GraphColormap *colormapPtr;
};

/*
 * IsolineIterator --
 *
 *	Tabs may be tagged with strings.  A tab may have many tags.  The
 *	same tag may be used for many tabs.
 *	
 */
typedef enum { 
    ITER_SINGLE, ITER_ALL, ITER_TAG, 
} IteratorType;

typedef struct _IsolineIterator {
    ContourElement *elemPtr;	       /* Element that we're iterating over. */

    IteratorType type;			/* Type of iteration:
					 * ITER_TAG	 By item tag.
					 * ITER_ALL      By every item.
					 * ITER_SINGLE   Single item: either 
					 *               tag or index.
					 */

    Isoline *startPtr;			/* Starting item.  Starting point of
					 * search, saved if iterator is reused.
					 * Used for ITER_ALL and ITER_SINGLE
					 * searches. */
    Isoline *endPtr;			/* Ending item (inclusive). */
    Isoline *nextPtr;			/* Next item. */
					/* For tag-based searches. */
    const char *tagName;		/* If non-NULL, is the tag that we are
					 * currently iterating over. */
    Blt_HashTable *tablePtr;		/* Pointer to tag hash table. */
    Blt_HashSearch cursor;		/* Search iterator for tag hash
					 * table. */
} IsolineIterator;


BLT_EXTERN Blt_CustomOption bltContourPenOption;
BLT_EXTERN Blt_CustomOption bltLimitOption;
BLT_EXTERN Blt_CustomOption bltValuesOption;
BLT_EXTERN Blt_CustomOption bltValuePairsOption;
BLT_EXTERN Blt_CustomOption bltXAxisOption;
BLT_EXTERN Blt_CustomOption bltYAxisOption;
BLT_EXTERN Blt_CustomOption bltZAxisOption;

static Blt_OptionFreeProc FreeColor;
static Blt_OptionParseProc ObjToColor;
static Blt_OptionPrintProc ColorToObj;
static Blt_CustomOption colorOption = {
    ObjToColor, ColorToObj, FreeColor, (ClientData)0
};

static Blt_OptionFreeProc FreeSymbolProc;
static Blt_OptionParseProc ObjToSymbolProc;
static Blt_OptionPrintProc SymbolToObjProc;
static Blt_CustomOption symbolOption =
{
    ObjToSymbolProc, SymbolToObjProc, FreeSymbolProc, (ClientData)0
};

static Blt_OptionFreeProc FreeMeshProc;
static Blt_OptionParseProc ObjToMeshProc;
static Blt_OptionPrintProc MeshToObjProc;
static Blt_CustomOption meshOption = {
    ObjToMeshProc, MeshToObjProc, FreeMeshProc, (ClientData)0
};

static Blt_OptionFreeProc FreeColormapProc;
static Blt_OptionParseProc ObjToColormapProc;
static Blt_OptionPrintProc ColormapToObjProc;
static Blt_CustomOption colormapOption =
{
    ObjToColormapProc, ColormapToObjProc, FreeColormapProc, (ClientData)0
};

#define DEF_ACTIVE_PEN		"activeContour"
#define DEF_AXIS_X		"x"
#define DEF_AXIS_Y		"y"
#define DEF_AXIS_Z		"z"
#define DEF_BACKGROUND		"navyblue"
#define DEF_COLORMAP		(char *)NULL
#define DEF_DISPLAY_COLORMAP	"1"
#define DEF_DISPLAY_EDGES	"1"
#define DEF_DISPLAY_HULL	"1"
#define DEF_DISPLAY_ISOLINES	"1"
#define DEF_DISPLAY_SYMBOLS	"0"
#define DEF_DISPLAY_VALUES	"0"
#define DEF_FOREGROUND		"blue"
#define DEF_HIDE		"no"
#define DEF_LABEL_RELIEF	"flat"
#define DEF_MAX_SYMBOLS		"0"
#define DEF_MESH		(char *)NULL
#define DEF_NORMAL_STIPPLE	""
#define DEF_OPACITY		"100.0"
#define DEF_RELIEF		"raised"
#define DEF_SCALE_SYMBOLS	"yes"
#define DEF_SHOW		"yes"
#define DEF_SHOW_ERRORBARS	"both"
#define DEF_STATE		"normal"
#define DEF_TAGS		"all"
#define DEF_WIDTH		"0.0"

#define DEF_MESH_COLOR		RGB_BLACK
#define DEF_MESH_DASHES		(char *)NULL
#define DEF_MESH_LINEWIDTH	"1"
#define DEF_MESH_OFFDASH_COLOR	(char *)NULL

#define DEF_PEN_ACTIVE_COLOR	RGB_BLUE
#define DEF_PEN_COLOR		RGB_BLACK
#define DEF_PEN_DASHES		(char *)NULL
#define DEF_PEN_DASHES		(char *)NULL
#define DEF_PEN_DIRECTION	"both"
#define DEF_PEN_FILL_COLOR	RGB_RED
#define DEF_PEN_LINEWIDTH	"1"
#define DEF_PEN_NORMAL_COLOR	RGB_BLACK
#define DEF_PEN_OFFDASH_COLOR	(char *)NULL
#define DEF_PEN_OUTLINE_COLOR	RGB_BLACK
#define DEF_PEN_OUTLINE_WIDTH	"1"
#define DEF_PEN_PIXELS		"0.05i"
#define DEF_PEN_SHOW_VALUES	"no"
#define DEF_PEN_SYMBOL		"circle"
#define DEF_PEN_TYPE		"line"
#define DEF_PEN_VALUE_ANCHOR	"s"
#define DEF_PEN_VALUE_ANGLE	(char *)NULL
#define DEF_PEN_VALUE_COLOR	RGB_BLACK
#define DEF_PEN_VALUE_FONT	STD_FONT_NUMBERS
#define DEF_PEN_VALUE_FORMAT	"%g"

static Blt_ConfigSpec penSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-color", "color", "Color", DEF_PEN_COLOR, 
	Blt_Offset(ContourPen, traceColor), NORMAL_PEN, &colorOption},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes", DEF_PEN_DASHES, 
	Blt_Offset(ContourPen, traceDashes), BLT_CONFIG_NULL_OK | ALL_PENS},
    {BLT_CONFIG_CUSTOM, "-fill", "fill", "Fill", DEF_PEN_FILL_COLOR, 
	Blt_Offset(ContourPen, symbol.fillColor), BLT_CONFIG_NULL_OK|ALL_PENS, 
	&colorOption},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth", 
	DEF_PEN_LINEWIDTH, Blt_Offset(ContourPen, traceWidth), 
	ALL_PENS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-offdash", "offDash", "OffDash", DEF_PEN_OFFDASH_COLOR,
	Blt_Offset(ContourPen, traceOffColor), BLT_CONFIG_NULL_OK | ALL_PENS, 
	&colorOption},
    {BLT_CONFIG_CUSTOM, "-outline", "outline", "Outline", DEF_PEN_OUTLINE_COLOR,
	Blt_Offset(ContourPen, symbol.outlineColor), ALL_PENS, &colorOption},
    {BLT_CONFIG_PIXELS_NNEG, "-outlinewidth", "outlineWidth", "OutlineWidth",
	DEF_PEN_OUTLINE_WIDTH, Blt_Offset(ContourPen, symbol.outlineWidth),
	BLT_CONFIG_DONT_SET_DEFAULT | ALL_PENS},
    {BLT_CONFIG_PIXELS_NNEG, "-pixels", "pixels", "Pixels", DEF_PEN_PIXELS, 
	Blt_Offset(ContourPen, symbol.size), ALL_PENS},
    {BLT_CONFIG_CUSTOM, "-symbol", "symbol", "Symbol", DEF_PEN_SYMBOL, 
	Blt_Offset(ContourPen, symbol), BLT_CONFIG_DONT_SET_DEFAULT | ALL_PENS, 
	&symbolOption},
    {BLT_CONFIG_STRING, "-type", (char *)NULL, (char *)NULL, DEF_PEN_TYPE, 
	Blt_Offset(Pen, typeId), ALL_PENS | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
	DEF_PEN_VALUE_ANCHOR, Blt_Offset(ContourPen, valueStyle.anchor), 
	ALL_PENS},
    {BLT_CONFIG_COLOR, "-valuecolor", "valueColor", "ValueColor",
	DEF_PEN_VALUE_COLOR, Blt_Offset(ContourPen, valueStyle.color), 
	ALL_PENS},
    {BLT_CONFIG_FONT, "-valuefont", "valueFont", "ValueFont",
	DEF_PEN_VALUE_FONT, Blt_Offset(ContourPen, valueStyle.font), ALL_PENS},
    {BLT_CONFIG_STRING, "-valueformat", "valueFormat", "ValueFormat",
	DEF_PEN_VALUE_FORMAT, Blt_Offset(ContourPen, valueFormat),
	ALL_PENS | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FLOAT, "-valuerotate", "valueRotate", "ValueRotate",
	DEF_PEN_VALUE_ANGLE, Blt_Offset(ContourPen, valueStyle.angle), 
	ALL_PENS},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec contourSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-activepen", "activePen", "ActivePen",
	DEF_ACTIVE_PEN, Blt_Offset(ContourElement, activePenPtr),
	BLT_CONFIG_NULL_OK, &bltContourPenOption},
    {BLT_CONFIG_LIST, "-bindtags", "bindTags", "BindTags", DEF_TAGS, 
	Blt_Offset(ContourElement, obj.tags), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-color", "color", "Color", DEF_PEN_COLOR, 
	Blt_Offset(ContourElement, builtinPen.traceColor), NORMAL_PEN, 
	&colorOption},
    {BLT_CONFIG_CUSTOM, "-colormap", "colormap", "Colormap", DEF_COLORMAP, 
	Blt_Offset(ContourElement, colormapPtr), 0, &colormapOption},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes", DEF_PEN_DASHES, 
	Blt_Offset(ContourElement, builtinPen.traceDashes), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-fill", "fill", "Fill", DEF_PEN_FILL_COLOR, 
	Blt_Offset(ContourElement, builtinPen.symbol.fillColor), 
	BLT_CONFIG_NULL_OK, &colorOption},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_HIDE, 
        Blt_Offset(ContourElement, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDE},
    {BLT_CONFIG_STRING, "-label", "label", "Label", (char *)NULL, 
	Blt_Offset(ContourElement, label), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-legendrelief", "legendRelief", "LegendRelief",
	DEF_LABEL_RELIEF, Blt_Offset(ContourElement, legendRelief),
	BLT_CONFIG_DONT_SET_DEFAULT}, 
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth",
	DEF_PEN_LINEWIDTH, Blt_Offset(ContourElement, builtinPen.traceWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_AXIS_X, 
	Blt_Offset(ContourElement, axes.x), 0, &bltXAxisOption},
    {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_AXIS_Y, 
	Blt_Offset(ContourElement, axes.y), 0, &bltYAxisOption},
    {BLT_CONFIG_CUSTOM, "-mapz", "mapZ", "MapZ", DEF_AXIS_Z, 
	Blt_Offset(ContourElement, zAxisPtr), 0, &bltZAxisOption},
    {BLT_CONFIG_INT_NNEG, "-maxsymbols", "maxSymbols", "MaxSymbols",
	DEF_MAX_SYMBOLS, Blt_Offset(ContourElement, reqMaxSymbols),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-mesh", "mesh", "Mesh", DEF_MESH, 
	Blt_Offset(ContourElement, meshPtr), BLT_CONFIG_NULL_OK, &meshOption},
    {BLT_CONFIG_COLOR, "-meshcolor", "meshcolor", "MeshColor", DEF_MESH_COLOR, 
	Blt_Offset(ContourElement, meshColor), 0},
    {BLT_CONFIG_COLOR, "-meshoffdashcolor", "meshOffDashColor", 
	"MeshOffDashColor", DEF_MESH_OFFDASH_COLOR, 
	Blt_Offset(ContourElement, meshOffColor), BLT_CONFIG_NULL_OK}, 
    {BLT_CONFIG_PIXELS_NNEG, "-meshlinewidth", "meshLineWidth", "MeshLineWidth",
	DEF_MESH_LINEWIDTH, Blt_Offset(ContourElement, meshWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DASHES, "-meshdashes", "meshDashes", "MeshDashes", 
	DEF_MESH_DASHES, Blt_Offset(ContourElement, meshDashes), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-offdash", "offDash", "OffDash", 
	DEF_PEN_OFFDASH_COLOR, 
	Blt_Offset(ContourElement, builtinPen.traceOffColor),
	BLT_CONFIG_NULL_OK, &colorOption},
    {BLT_CONFIG_FLOAT, "-opacity", "opacity", "Opacity", DEF_OPACITY, 
	Blt_Offset(ContourElement, opacity), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-outline", "outline", "Outline", 
	DEF_PEN_OUTLINE_COLOR, 
	Blt_Offset(ContourElement, builtinPen.symbol.outlineColor), 
	0, &colorOption},
    {BLT_CONFIG_PIXELS_NNEG, "-outlinewidth", "outlineWidth", "OutlineWidth",
	DEF_PEN_OUTLINE_WIDTH, 
	Blt_Offset(ContourElement, builtinPen.symbol.outlineWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-pen", "pen", "Pen", (char *)NULL, 
	Blt_Offset(ContourElement, penPtr), BLT_CONFIG_NULL_OK, 
	&bltContourPenOption},
    {BLT_CONFIG_PIXELS_NNEG, "-pixels", "pixels", "Pixels", DEF_PEN_PIXELS, 
	Blt_Offset(ContourElement, builtinPen.symbol.size), GRAPH | STRIPCHART}, 
    {BLT_CONFIG_BOOLEAN, "-scalesymbols", "scaleSymbols", "ScaleSymbols",
	DEF_SCALE_SYMBOLS, Blt_Offset(ContourElement, scaleSymbols),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK_INVERT, "-show", "show", "show", DEF_SHOW, 
        Blt_Offset(ContourElement, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDE},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_STATE, 
	Blt_Offset(ContourElement, state), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-symbol", "symbol", "Symbol", DEF_PEN_SYMBOL, 
	Blt_Offset(ContourElement, builtinPen.symbol), 
	BLT_CONFIG_DONT_SET_DEFAULT, &symbolOption},
    {BLT_CONFIG_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
	DEF_PEN_VALUE_ANCHOR, 
	Blt_Offset(ContourElement, builtinPen.valueStyle.anchor), 0},
    {BLT_CONFIG_COLOR, "-valuecolor", "valueColor", "ValueColor",
	DEF_PEN_VALUE_COLOR, 
	Blt_Offset(ContourElement, builtinPen.valueStyle.color), 0},
    {BLT_CONFIG_FONT, "-valuefont", "valueFont", "ValueFont",
	DEF_PEN_VALUE_FONT, 
	Blt_Offset(ContourElement, builtinPen.valueStyle.font), 0},
    {BLT_CONFIG_STRING, "-valueformat", "valueFormat", "ValueFormat",
	DEF_PEN_VALUE_FORMAT, 
	Blt_Offset(ContourElement, builtinPen.valueFormat),
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FLOAT, "-valuerotate", "valueRotate", "ValueRotate",
	DEF_PEN_VALUE_ANGLE, 
	Blt_Offset(ContourElement, builtinPen.valueStyle.angle), 0},
    {BLT_CONFIG_CUSTOM, "-weights", "weights", "Weights", (char *)NULL, 
	Blt_Offset(ContourElement, w), 0, &bltValuesOption},
    {BLT_CONFIG_BITMASK, "-displayhull", "displayHull", 
	"DisplayHull", DEF_DISPLAY_HULL, Blt_Offset(ContourElement, flags), 
	0, (Blt_CustomOption *)HULL},
    {BLT_CONFIG_BITMASK, "-displaycolormap", "displayColormap", 
	"DisplayColormap", DEF_DISPLAY_COLORMAP, 
	Blt_Offset(ContourElement, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)COLORMAP},
    {BLT_CONFIG_BITMASK, "-displayisolines", "displayIsolines", 
	"DisplayIsolines", DEF_DISPLAY_ISOLINES, 
	Blt_Offset(ContourElement, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	(Blt_CustomOption *)ISOLINES},
    {BLT_CONFIG_BITMASK, "-displayedges", "displayEdges", "DisplayEdges", 
        DEF_DISPLAY_EDGES, Blt_Offset(ContourElement, flags), 
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)EDGES},
    {BLT_CONFIG_BITMASK, "-displaysymbols", "displaySymbols", "DisplaySymbols",
	DEF_DISPLAY_SYMBOLS, Blt_Offset(ContourElement, flags), 
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)SYMBOLS},
    {BLT_CONFIG_BITMASK, "-displayvalues", "displayValues", "DisplayValues",
	DEF_DISPLAY_VALUES, Blt_Offset(ContourElement, flags), 
	BLT_CONFIG_DONT_SET_DEFAULT, (Blt_CustomOption *)VALUES},
    {BLT_CONFIG_CUSTOM, "-values", "values", "Values", (char *)NULL, 
	Blt_Offset(ContourElement, z), 0, &bltValuesOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec isolineSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-activepen", "activePen", "ActivePen",
	DEF_ACTIVE_PEN, Blt_Offset(Isoline, activePenPtr), 
	BLT_CONFIG_NULL_OK, &bltContourPenOption},
    {BLT_CONFIG_LIST, "-bindtags", "bindTags", "BindTags", DEF_TAGS, 
	Blt_Offset(Isoline, obj.tags), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_HIDE, 
         Blt_Offset(Isoline, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDE},
    {BLT_CONFIG_STRING, "-label", "label", "Label", (char *)NULL, 
	Blt_Offset(Isoline, label), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-pen", "pen", "Pen", (char *)NULL, 
	Blt_Offset(Isoline, penPtr), BLT_CONFIG_NULL_OK, 
	&bltContourPenOption},
    {BLT_CONFIG_CUSTOM, "-min", "min", "Min", "", Blt_Offset(Isoline, reqMin), 
	BLT_CONFIG_DONT_SET_DEFAULT, &bltLimitOption},
    {BLT_CONFIG_CUSTOM, "-max", "max", "Max", "", Blt_Offset(Isoline, reqMax), 
	BLT_CONFIG_DONT_SET_DEFAULT, &bltLimitOption},
    {BLT_CONFIG_BITMASK_INVERT, "-show", "show", "Show", DEF_SHOW, 
         Blt_Offset(Isoline, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDE},
    {BLT_CONFIG_DOUBLE, "-value", "value", "Value", "0.0",
	Blt_Offset(Isoline, reqValue), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};


/* Forward declarations */
static PenConfigureProc ConfigurePenProc;
static PenDestroyProc DestroyPenProc;
static ElementClosestProc ClosestProc;
static ElementConfigProc ConfigureProc;
static ElementDestroyProc DestroyProc;
static ElementDrawProc DrawActiveProc;
static ElementDrawProc DrawProc;
static ElementDrawSymbolProc DrawSymbolProc;
/* static ElementFindProc *FindProc; */
static ElementExtentsProc ExtentsProc;
static ElementToPostScriptProc ActiveToPostScriptProc;
static ElementToPostScriptProc NormalToPostScriptProc;
static ElementSymbolToPostScriptProc SymbolToPostScriptProc;
static ElementMapProc MapProc;

static void DrawTriangle(ContourElement *elemPtr, Blt_Picture picture, 
	Triangle *t, int xOffset, int yOffset);

static int ComputeColorBarGeometry(Graph *graphPtr, ContourElement *elemPtr, 
				   int width, int height);

/* Table of symbol names. */
static SymbolTable symbolTable[] = {
    { "arrow",	  1, SYMBOL_ARROW,	},
    { "circle",	  2, SYMBOL_CIRCLE,	},
    { "cross",	  2, SYMBOL_CROSS,	}, 
    { "diamond",  1, SYMBOL_DIAMOND,	}, 
    { "image",    1, SYMBOL_IMAGE,	}, 
    { "none",	  1, SYMBOL_NONE,	}, 
    { "plus",	  1, SYMBOL_PLUS,	}, 
    { "scross",	  2, SYMBOL_SCROSS,	}, 
    { "splus",	  2, SYMBOL_SPLUS,	}, 
    { "square",	  2, SYMBOL_SQUARE,	}, 
    { "triangle", 1, SYMBOL_TRIANGLE,	}, 
    { NULL,       0, 0			}, 
};

static int 
GetContourElement(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr, 
		  ContourElement **elemPtrPtr)
{
    Element *basePtr;

    if (Blt_GetElement(interp, graphPtr, objPtr, &basePtr) != TCL_OK) {
	return TCL_ERROR;	/* Can't find named element */
    }
    if (basePtr->obj.classId != CID_ELEM_CONTOUR) {
	Tcl_AppendResult(interp, "element \"", Tcl_GetString(objPtr), 
		"\" is not a contour element", (char *)NULL);
	return TCL_ERROR;
    }
    *elemPtrPtr = (ContourElement *)basePtr;
    return TCL_OK;
}

INLINE static int64_t
Round(double x)
{
    return (int64_t) (x + ((x < 0.0) ? -0.5 : 0.5));
}

INLINE static int
InRange(double x, double min, double max)
{
    double range;

    range = max - min;
    if (range < DBL_EPSILON) {
	return (FABS(max - x) >= DBL_EPSILON);
    } else {
	double t;

	t = (x - min) / range;
	if ((t > 0.0) && (t < 1.0)) {
	    return TRUE;
	}
	if ((Blt_AlmostEquals(t, 0.0)) || (Blt_AlmostEquals(t, 1.0))) {
	    return TRUE;
	}
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 * Custom option parse and print procedures
 *---------------------------------------------------------------------------
 */
static void
DestroySymbol(Display *display, Symbol *symbolPtr)
{
    if (symbolPtr->image != NULL) {
	Tk_FreeImage(symbolPtr->image);
	symbolPtr->image = NULL;
    }
    if (symbolPtr->bitmap != None) {
	Tk_FreeBitmap(display, symbolPtr->bitmap);
	symbolPtr->bitmap = None;
    }
    if (symbolPtr->mask != None) {
	Tk_FreeBitmap(display, symbolPtr->mask);
	symbolPtr->mask = None;
    }
    symbolPtr->type = SYMBOL_NONE;
}

/*
 *---------------------------------------------------------------------------
 *
 * ImageChangedProc
 *
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
    int x, int y, int w, int h,		/* Not used. */
    int imageWidth, int imageHeight)	/* Not used. */
{
    Element *elemPtr;
    Graph *graphPtr;

    elemPtr = clientData;
    elemPtr->flags |= MAP_ITEM;
    graphPtr = elemPtr->obj.graphPtr;
    graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
}

/*ARGSUSED*/
static void
FreeColor(ClientData clientData, Display *display, char *widgRec, int offset)
{
    XColor **colorPtrPtr = (XColor **)(widgRec + offset);

    if ((*colorPtrPtr != NULL) && (!ISALIASED(*colorPtrPtr))) {
	Tk_FreeColor(*colorPtrPtr);
    }
    *colorPtrPtr = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToColor --
 *
 *	Convert the string representation of a color into a XColor pointer.
 *
 * Results:
 *	The return value is a standard TCL result.  The color pointer is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColor(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing color */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    XColor **colorPtrPtr = (XColor **)(widgRec + offset);
    XColor *colorPtr;
    const char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    
    if ((c == '\0') && (flags & BLT_CONFIG_NULL_OK)) {
	colorPtr = NULL;
    }  else if ((c == 'p') && (strncmp(string, "palette", length) == 0)) {
	colorPtr = COLOR_PALETTE;
    } else if ((c == 'd') && 
	       (strncmp(string, "defcolor", length) == 0)) {
	colorPtr = COLOR_DEFAULT;
    } else {
	colorPtr = Tk_AllocColorFromObj(interp, tkwin, objPtr);
	if (colorPtr == NULL) {
	    return TCL_ERROR;
	}
    }
    if ((*colorPtrPtr != NULL) && (!ISALIASED(*colorPtrPtr))) {
	Tk_FreeColor(*colorPtrPtr);
    }
    *colorPtrPtr = colorPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorToObj --
 *
 *	Convert the color value into a string.
 *
 * Results:
 *	The string representing the symbol color is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ColorToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget information record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    XColor *colorPtr = *(XColor **)(widgRec + offset);
    Tcl_Obj *objPtr;

    if (colorPtr == NULL) {
	objPtr = Tcl_NewStringObj("", -1);
    } else if (colorPtr == COLOR_PALETTE) {
	objPtr = Tcl_NewStringObj("palette", -1);
    } else if (colorPtr == COLOR_DEFAULT) {
	objPtr = Tcl_NewStringObj("defcolor", -1);
    } else {
	objPtr = Tcl_NewStringObj(Tk_NameOfColor(colorPtr), -1);
    }
    return objPtr;
}


/*ARGSUSED*/
static void
FreeSymbolProc(
    ClientData clientData,		/* Not used. */
    Display *display,			/* Not used. */
    char *widgRec,
    int offset)
{
    Symbol *symbolPtr = (Symbol *)(widgRec + offset);

    DestroySymbol(display, symbolPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSymbol --
 *
 *	Convert the string representation of a line style or symbol name into
 *	its numeric form.
 *
 * Results:
 *	The return value is a standard TCL result.  The symbol type is written
 *	into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToSymbolProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing symbol type */
    char *widgRec,			/* Element information record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Symbol *symbolPtr = (Symbol *)(widgRec + offset);
    const char *string;

    {
	int length;
	SymbolTable *entryPtr;
	char c;

	string = Tcl_GetStringFromObj(objPtr, &length);
	if (length == 0) {
	    DestroySymbol(Tk_Display(tkwin), symbolPtr);
	    symbolPtr->type = SYMBOL_NONE;
	    return TCL_OK;
	}
	c = string[0];
	for (entryPtr = symbolTable; entryPtr->name != NULL; entryPtr++) {
	    if (length < entryPtr->minChars) {
		continue;
	    }
	    if ((c == entryPtr->name[0]) && 
		(strncmp(string, entryPtr->name, length) == 0)) {
		DestroySymbol(Tk_Display(tkwin), symbolPtr);
		symbolPtr->type = entryPtr->type;
		return TCL_OK;
	    }
	}
    }
    {
	Tk_Image tkImage;
	Element *elemPtr = (Element *)widgRec;

	tkImage = Tk_GetImage(interp, tkwin, string, ImageChangedProc, elemPtr);
	if (tkImage != NULL) {
	    DestroySymbol(Tk_Display(tkwin), symbolPtr);
	    symbolPtr->image = tkImage;
	    symbolPtr->type = SYMBOL_IMAGE;
	    return TCL_OK;
	}
    }
    {
	Pixmap bitmap, mask;
	Tcl_Obj **objv;
	int objc;

	if ((Tcl_ListObjGetElements(NULL, objPtr, &objc, &objv) != TCL_OK) || 
	    (objc > 2)) {
	    goto error;
	}
	bitmap = mask = None;
	if (objc > 0) {
	    bitmap = Tk_AllocBitmapFromObj((Tcl_Interp *)NULL, tkwin, objv[0]);
	    if (bitmap == None) {
		goto error;
	    }
	}
	if (objc > 1) {
	    mask = Tk_AllocBitmapFromObj((Tcl_Interp *)NULL, tkwin, objv[1]);
	    if (mask == None) {
		goto error;
	    }
	}
	DestroySymbol(Tk_Display(tkwin), symbolPtr);
	symbolPtr->bitmap = bitmap;
	symbolPtr->mask = mask;
	symbolPtr->type = SYMBOL_BITMAP;
	return TCL_OK;
    }
 error:
    Tcl_AppendResult(interp, "bad symbol \"", string, 
	"\": should be \"none\", \"circle\", \"square\", \"diamond\", "
	"\"plus\", \"cross\", \"splus\", \"scross\", \"triangle\", "
	"\"arrow\" or the name of a bitmap", (char *)NULL);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * SymbolToObj --
 *
 *	Convert the symbol value into a string.
 *
 * Results:
 *	The string representing the symbol type or line style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SymbolToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,
    char *widgRec,			/* Element information record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Symbol *symbolPtr = (Symbol *)(widgRec + offset);

    if (symbolPtr->type == SYMBOL_BITMAP) {
	Tcl_Obj *listObjPtr, *objPtr;
	const char *name;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	name = Tk_NameOfBitmap(Tk_Display(tkwin), symbolPtr->bitmap);
	objPtr = Tcl_NewStringObj(name, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	if (symbolPtr->mask == None) {
	    objPtr = Tcl_NewStringObj("", -1);
	} else {
	    name = Tk_NameOfBitmap(Tk_Display(tkwin), symbolPtr->mask);
	    objPtr = Tcl_NewStringObj(name, -1);
	}
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	return listObjPtr;
    } else {
	SymbolTable *entryPtr;

	for (entryPtr = symbolTable; entryPtr->name != NULL; entryPtr++) {
	    if (entryPtr->type == symbolPtr->type) {
		return Tcl_NewStringObj(entryPtr->name, -1);
	    }
	}
	return Tcl_NewStringObj("?unknown symbol type?", -1);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MeshChangedProc
 *
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
MeshChangedProc(Mesh *meshPtr, ClientData clientData, unsigned int flags)
{
    ContourElement *elemPtr = clientData;

    if (flags & MESH_DELETE_NOTIFY) {
	elemPtr->meshPtr = NULL;
    }
    elemPtr->flags |= MAP_ITEM;
    elemPtr->obj.graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(elemPtr->obj.graphPtr);
}

/*ARGSUSED*/
static void
FreeMeshProc(
    ClientData clientData,		/* Not used. */
    Display *display,			/* Not used. */
    char *widgRec,
    int offset)
{
    Mesh **meshPtrPtr = (Mesh **)(widgRec + offset);
    ContourElement *elemPtr = (ContourElement *)widgRec;

    Blt_DeleteMeshNotifier(*meshPtrPtr, elemPtr);
    *meshPtrPtr = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToMeshProc --
 *
 *	Convert the string representation of a mesh into its token.
 *
 * Results:
 *	The return value is a standard TCL result.  The mesh token is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToMeshProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing symbol type */
    char *widgRec,			/* Element information record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Mesh **meshPtrPtr = (Mesh **)(widgRec + offset);
    ContourElement *elemPtr = (ContourElement *)widgRec;
    const char *string;
    
    string = Tcl_GetString(objPtr);
    if ((string == NULL) || (string[0] == '\0')) {
	FreeMeshProc(clientData, Tk_Display(tkwin), widgRec, offset);
	return TCL_OK;
    }
    if (Blt_GetMeshFromObj(interp, objPtr, meshPtrPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_CreateMeshNotifier(*meshPtrPtr, MeshChangedProc, elemPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MeshToObjProc --
 *
 *	Convert the mesh token into a string.
 *
 * Results:
 *	The string representing the symbol type or line style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
MeshToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,
    char *widgRec,			/* Element information record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Mesh *meshPtr = *(Mesh **)(widgRec + offset);
    if (meshPtr == NULL) {
	return Tcl_NewStringObj("", -1);
    } 
    return Tcl_NewStringObj(Blt_NameOfMesh(meshPtr), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ColormapChangedProc
 *
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ColormapChangedProc(GraphColormap *cmapPtr, ClientData clientData, 
		    unsigned int flags)
{
    ContourElement *elemPtr = clientData;
    Graph *graphPtr;

    if (flags & COLORMAP_DELETE_NOTIFY) {
	cmapPtr->palette = NULL;
    }
    elemPtr->flags |= MAP_ITEM;
    graphPtr = cmapPtr->graphPtr;
    graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
}

/*ARGSUSED*/
static void
FreeColormapProc(ClientData clientData, Display *display, char *widgRec,
		 int offset)
{
    GraphColormap **cmapPtrPtr = (GraphColormap **)(widgRec + offset);
    ContourElement *elemPtr = (ContourElement *)widgRec;
    
    Blt_Colormap_DeleteNotifier(*cmapPtrPtr, elemPtr);
    *cmapPtrPtr = NULL;
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToColormapProc --
 *
 *	Convert the string representation of a colormap into its token.
 *
 * Results:
 *	The return value is a standard TCL result.  The colormap token is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToColormapProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing symbol type */
    char *widgRec,			/* Element information record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    GraphColormap **cmapPtrPtr = (GraphColormap **)(widgRec + offset);
    ContourElement *elemPtr = (ContourElement *)widgRec;
    const char *string;
    
    string = Tcl_GetString(objPtr);
    if ((string == NULL) || (string[0] == '\0')) {
	FreeColormapProc(clientData, Tk_Display(tkwin), widgRec, offset);
	return TCL_OK;
    }
    if (Blt_Colormap_Get(interp, elemPtr->obj.graphPtr, objPtr, cmapPtrPtr) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    Blt_Colormap_CreateNotifier(*cmapPtrPtr, ColormapChangedProc, elemPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColormapToObjProc --
 *
 *	Convert the colormap token into a string.
 *
 * Results:
 *	The string representing the colormap is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ColormapToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,
    char *widgRec,			/* Element information record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    GraphColormap *cmapPtr = *(GraphColormap **)(widgRec + offset);

    if (cmapPtr == NULL) {
	return Tcl_NewStringObj("", -1);
    } 
    return Tcl_NewStringObj(cmapPtr->name, -1);
}

/* Isoline procedures. */

static int
GetIsoTagTable(ContourElement *elemPtr, const char *name,  
	    Blt_HashTable **tablePtrPtr)
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&elemPtr->tagTable, name);
    if (hPtr == NULL) {
	return TCL_ERROR;			/* No tag by name. */
    }
    *tablePtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

static int
HasIsoTag(Isoline *isoPtr, const char *name)
{
    Blt_HashEntry *hPtr;
    Blt_HashTable *tablePtr;

    if (strcmp(name, "all") == 0) {
	return TRUE;
    }
    if (GetIsoTagTable(isoPtr->elemPtr, name, &tablePtr) != TCL_OK) {
	return FALSE;
    }
    hPtr = Blt_FindHashEntry(tablePtr, (char *)isoPtr);
    if (hPtr == NULL) {
	return FALSE;
    }
    return TRUE;
}

static Blt_HashTable *
AddIsoTagTable(ContourElement *elemPtr, const char *name)
{
    Blt_HashEntry *hPtr;
    Blt_HashTable *tablePtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&elemPtr->tagTable, name, &isNew);
    if (isNew) {
	tablePtr = Blt_AssertMalloc(sizeof(Blt_HashTable));
	Blt_InitHashTable(tablePtr, BLT_ONE_WORD_KEYS);
	Blt_SetHashValue(hPtr, tablePtr);
    } else {
	tablePtr = Blt_GetHashValue(hPtr);
    }
    return tablePtr;
}

static void
AddIsoTag(ContourElement *elemPtr, Isoline *isoPtr, const char *name)
{
    int isNew;
    Blt_HashEntry *hPtr;
    Blt_HashTable *tablePtr;

    tablePtr = AddIsoTagTable(elemPtr, name);
    hPtr = Blt_CreateHashEntry(tablePtr, (char *)isoPtr, &isNew);
    if (isNew) {
	Blt_SetHashValue(hPtr, isoPtr);
    }
}

static void
ForgetIsoTag(ContourElement *elemPtr, const char *name)
{
    Blt_HashEntry *hPtr;
    Blt_HashTable *tablePtr;

    if (strcmp(name, "all") == 0) {
	return;				/* Can't remove tag "all". */
    }
    hPtr = Blt_FindHashEntry(&elemPtr->tagTable, name);
    if (hPtr == NULL) {
	return;				/* No tag by name. */
    }
    tablePtr = Blt_GetHashValue(hPtr);
    Blt_DeleteHashTable(tablePtr);
    Blt_Free(tablePtr);
    Blt_DeleteHashEntry(&elemPtr->tagTable, hPtr);
}

static void
DestroyIsoTags(ContourElement *elemPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&elemPtr->tagTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Blt_HashTable *tablePtr;
	
	tablePtr = Blt_GetHashValue(hPtr);
	Blt_DeleteHashTable(tablePtr);
    }
}

static void
RemoveIsoTag(Isoline *isoPtr, const char *name)
{
    Blt_HashTable *tablePtr;

    tablePtr = NULL;			/* Suppress compiler warning. */
    if (GetIsoTagTable(isoPtr->elemPtr, name, &tablePtr) == TCL_OK) {
	Blt_HashEntry *hPtr;

	hPtr = Blt_FindHashEntry(tablePtr, (char *)isoPtr);
	if (hPtr != NULL) {
	    Blt_DeleteHashEntry(tablePtr, hPtr);
	}
    }
}

static void
ClearIsoTags(Isoline *isoPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    ContourElement *elemPtr;

    elemPtr = isoPtr->elemPtr;
    for (hPtr = Blt_FirstHashEntry(&elemPtr->tagTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Blt_HashTable *tablePtr;
	Blt_HashEntry *h2Ptr;
	
	tablePtr = Blt_GetHashValue(hPtr);
	h2Ptr = Blt_FindHashEntry(tablePtr, (char *)isoPtr);
	if (h2Ptr != NULL) {
	    Blt_DeleteHashEntry(tablePtr, h2Ptr);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NextTaggedIsoline --
 *
 *	Returns the next isoline derived from the given tag.
 *
 * Results:
 *	Returns the pointer to the next tab in the iterator.  If no more tabs
 *	are available, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Isoline *
NextTaggedIsoline(IsolineIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
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
 * FirstTaggedIsoline --
 *
 *	Returns the first isoline derived from the given tag.
 *
 * Results:
 *	Returns the first isoline in the sequence.  If no more isolines are in
 *	the list, then NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Isoline *
FirstTaggedIsoline(IsolineIterator *iterPtr)
{
    switch (iterPtr->type) {
    case ITER_TAG:
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
GetIsolineFromObj(Tcl_Interp *interp, ContourElement *elemPtr, Tcl_Obj *objPtr,
		  Isoline **isoPtrPtr)
{
    Blt_HashEntry *hPtr;
    const char *string;

    string = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&elemPtr->isoTable, string);
    if (hPtr == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find an isoline \"", string, 
		"\" in element \"", elemPtr->obj.name, "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    *isoPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetIsolineIterator --
 *
 *	Converts a string representing a tab index into an tab pointer.  The
 *	index may be in one of the following forms:
 *
 *	 "all"		All isolines.
 *	 name		Name of the isoline.
 *	 tag		Tag associated with isolines.
 *
 *---------------------------------------------------------------------------
 */
static int
GetIsolineIterator(Tcl_Interp *interp, ContourElement *elemPtr, Tcl_Obj *objPtr,
		   IsolineIterator *iterPtr)
{
    Isoline *isoPtr;
    Blt_HashTable *tablePtr;
    const char *string;
    char c;
    int numBytes, length;

    iterPtr->elemPtr = elemPtr;
    iterPtr->type = ITER_SINGLE;
    iterPtr->tagName = Tcl_GetStringFromObj(objPtr, &numBytes);
    iterPtr->nextPtr = NULL;
    iterPtr->startPtr = iterPtr->endPtr = NULL;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'a') && (strcmp(iterPtr->tagName, "all") == 0)) {
	iterPtr->type  = ITER_ALL;
	iterPtr->tablePtr = &elemPtr->isoTable;
    } else if (GetIsolineFromObj(NULL, elemPtr, objPtr, &isoPtr) == TCL_OK) {
	iterPtr->startPtr = iterPtr->endPtr = isoPtr;
	iterPtr->type = ITER_SINGLE;
    } else if (GetIsoTagTable(elemPtr, string, &tablePtr) == TCL_OK) {
	iterPtr->tagName = string;
	iterPtr->tablePtr = tablePtr;
	iterPtr->type = ITER_TAG;
    } else {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't find isoline name or tag \"", 
		string, "\" in \"", Tk_PathName(elemPtr->obj.graphPtr->tkwin), 
		"\"", (char *)NULL);
	}
	return TCL_ERROR;
    }	
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewIsoline --
 *
 *	Creates a new isoline structure and inserts it into the element's
 *	isoline table.h
 *
 *---------------------------------------------------------------------------
 */
static Isoline *
NewIsoline(Tcl_Interp *interp, ContourElement *elemPtr, const char *name)
{
    Isoline *isoPtr;
    Blt_HashEntry *hPtr;
    int isNew;
    char string[200];

    isoPtr = Blt_AssertCalloc(1, sizeof(Isoline));
    if (name == NULL) {
	sprintf(string, "isoline%u", elemPtr->nextIsoline++);
	name = string;
    }
    hPtr = Blt_CreateHashEntry(&elemPtr->isoTable, name, &isNew);
    assert(isNew);
    isoPtr->obj.graphPtr = elemPtr->obj.graphPtr;
    isoPtr->obj.name = Blt_GetHashKey(&elemPtr->isoTable, hPtr);
    isoPtr->obj.classId = CID_ISOLINE;
    isoPtr->flags = 0;
    isoPtr->value = Blt_NaN();
    isoPtr->reqValue = 0.0;
    isoPtr->reqMin = isoPtr->reqMax = Blt_NaN();
    isoPtr->elemPtr = elemPtr;
    isoPtr->traces = NULL;
    isoPtr->penPtr = elemPtr->penPtr;
    Blt_SetHashValue(hPtr, isoPtr);
    isoPtr->hashPtr = hPtr;
    return isoPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyIsoline --
 *
 *	Creates a new isoline structure and inserts it into the element's
 *	isoline table.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyIsoline(Isoline *isoPtr)
{
    Graph *graphPtr;

    if (isoPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&isoPtr->elemPtr->isoTable, isoPtr->hashPtr);
    }
    ClearIsoTags(isoPtr);
    graphPtr = isoPtr->elemPtr->obj.graphPtr;
    Blt_FreeOptions(isolineSpecs, (char *)isoPtr, graphPtr->display, 0);
    if (isoPtr->elemPtr->activePtr == isoPtr) {
	isoPtr->elemPtr->activePtr = NULL;
    }
    Blt_Free(isoPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyIsolines --
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyIsolines(ContourElement *elemPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&elemPtr->isoTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Isoline *isoPtr;

	isoPtr = Blt_GetHashValue(hPtr);
	isoPtr->hashPtr = NULL;
	DestroyIsoline(isoPtr);
    }	
    Blt_DeleteHashTable(&elemPtr->isoTable);
}

static int
ConfigureIsoline(Tcl_Interp *interp, Isoline *isoPtr, int objc, 
		 Tcl_Obj *const *objv, int flags)
{
    Graph *graphPtr;

    graphPtr = isoPtr->elemPtr->obj.graphPtr;
    if (Blt_ConfigureComponentFromObj(interp, graphPtr->tkwin, isoPtr->obj.name,
	"Isoline", isolineSpecs, objc, objv, (char *)isoPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    return TCL_OK;
}


/* Trace/point generation procedures. */

/*
 *---------------------------------------------------------------------------
 *
 * NewTrace --
 *
 *	Creates a new trace and prepends to the list of traces for 
 *	this element.
 *
 * Results:
 *	Returns a pointer to the new trace.
 *
 * Side Effects:
 *	The trace is prepended to the element's list of traces.
 *
 *---------------------------------------------------------------------------
 */
static INLINE Trace *
NewTrace(Blt_Chain *tracesPtr)
{
    Trace *tracePtr;

    if (*tracesPtr == NULL) {
	*tracesPtr = Blt_Chain_Create();
    }
    tracePtr = Blt_AssertCalloc(1, sizeof(Trace));
    tracePtr->link = Blt_Chain_Prepend(*tracesPtr, tracePtr);
    return tracePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * RemoveHead --
 *
 *	Removes the point at the head of the trace.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The trace is shrunk.
 *
 *---------------------------------------------------------------------------
 */
static void
RemoveHead(ContourElement *elemPtr, Trace *tracePtr)
{
    TracePoint *p;

    p = tracePtr->head;
    tracePtr->head = p->next;
    if (tracePtr->tail == p) {
	tracePtr->tail = tracePtr->head;
    }
    Blt_Pool_FreeItem(elemPtr->pointPool, p);
    tracePtr->numPoints--;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeTrace --
 *
 *	Frees the memory assoicated with a trace.
 *	Note:  The points and segments of the trace are freed enmass when 
 *	       destroying the memory poll assoicated with the element.
 *
 * Results:
 *	Returns a pointer to the new trace.
 *
 * Side Effects:
 *	The trace is prepended to the element's list of traces.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeTrace(Blt_Chain traces, Trace *tracePtr)
{
    if (tracePtr->link != NULL) {
	Blt_Chain_DeleteLink(traces, tracePtr->link);
    }
    Blt_Free(tracePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FixTraces --
 *
 *	Fixes the trace by recounting the number of points.
 *	
 * Results:
 *	None.
 *
 * Side Effects:
 *	Removes the trace if it is empty.
 *
 *---------------------------------------------------------------------------
 */
static void
FixTraces(Blt_Chain traces)
{
    Blt_ChainLink link, next;

    for (link = Blt_Chain_FirstLink(traces); link != NULL; 
	 link = next) {
	Trace *tracePtr;
	int count;
	TracePoint *p, *q;

	next = Blt_Chain_NextLink(link);
	tracePtr = Blt_Chain_GetValue(link);
	if ((tracePtr->flags & RECOUNT) == 0) {
	    continue;
	}
	/* Count the number of points in the trace. */
	count = 0;
	q = NULL;
	for (p = tracePtr->head; p != NULL; p = p->next) {
	    count++;
	    q = p;
	}
	if (count == 0) {
	    /* Empty trace, remove it. */
	    FreeTrace(traces, tracePtr);
	} else {
	    /* Reset the number of points and the tail pointer. */
	    tracePtr->numPoints = count;
	    tracePtr->flags &= ~RECOUNT;
	    tracePtr->tail = q;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NewPoint --
 *
 *	Creates a new point 
 *
 * Results:
 *	Returns a pointer to the new trace.
 *
 * Side Effects:
 *	The trace is prepended to the element's list of traces.
 *
 *---------------------------------------------------------------------------
 */
static INLINE TracePoint *
NewPoint(ContourElement *elemPtr, double x, double y, int index)
{
    TracePoint *p;
    Region2d exts;

    p = Blt_Pool_AllocItem(elemPtr->pointPool, sizeof(TracePoint));
    p->next = NULL;
    p->flags = 0;
    p->x = x;
    p->y = y;
    p->index = index;
    Blt_GraphExtents(elemPtr, &exts);
    if (PointInRegion(&exts, p->x, p->y)) {
	p->flags |= VISIBLE;
    }
    return p;
}

/*
 *---------------------------------------------------------------------------
 *
 * AddSegment --
 *
 *	Creates a new segment of the trace's errorbars.
 *
 * Results:
 *	Returns a pointer to the new trace.
 *
 * Side Effects:
 *	The trace is prepended to the element's list of traces.
 *
 *---------------------------------------------------------------------------
 */
static TraceSegment *
AddSegment(ContourElement *elemPtr, float x1, float y1, float x2, float y2, 
	   Isoline *isoPtr)
{
    TraceSegment *s;

    s = Blt_Pool_AllocItem(elemPtr->segmentPool, sizeof(TraceSegment));
    s->x1 = x1;
    s->y1 = y1;
    s->x2 = x2;
    s->y2 = y2;
    s->next = NULL;
    s->flags = 0;
    if (isoPtr->segments == NULL) {
	isoPtr->segments = s;
    } else {
	s->next = isoPtr->segments;
	isoPtr->segments = s;
    }
    isoPtr->numSegments++;
    return s;
}

/*
 *---------------------------------------------------------------------------
 *
 * AppendPoint --
 *
 *	Appends the point to the given trace.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The trace's counter is incremented.
 *
 *---------------------------------------------------------------------------
 */
static INLINE void
AppendPoint(Trace *tracePtr, TracePoint *p)
{
    if (tracePtr->head == NULL) {
	tracePtr->tail = tracePtr->head = p;
    } else {
	assert(tracePtr->tail != NULL);
	tracePtr->tail->next = p;
    }
    tracePtr->tail = p;
    tracePtr->numPoints++;
}

/* make up an edge */
static INLINE void
MakeEdgeKey(EdgeKey *keyPtr, int a, int b) 
{
    if (a < b) {
	keyPtr->a = a;
	keyPtr->b = b;
    } else {
	keyPtr->a = b;
	keyPtr->b = a;
    }
}

/* make up an edge */
static INLINE void
MakePointKey(PointKey *keyPtr, float x, float y) 
{
    keyPtr->x = x;
    keyPtr->y = y;
}

static void
ResetElement(ContourElement *elemPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    if (elemPtr->pointPool != NULL) {
	Blt_Pool_Destroy(elemPtr->pointPool);
    }
    if (elemPtr->segmentPool != NULL) {
	Blt_Pool_Destroy(elemPtr->segmentPool);
    }
    for (hPtr = Blt_FirstHashEntry(&elemPtr->isoTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Isoline *isoPtr;
	Blt_ChainLink link, next;

	isoPtr = Blt_GetHashValue(hPtr);
	for (link = Blt_Chain_FirstLink(isoPtr->traces); link != NULL;
	     link = next) {
	    Trace *tracePtr;

	    next = Blt_Chain_NextLink(link);
	    tracePtr = Blt_Chain_GetValue(link);
	    FreeTrace(isoPtr->traces, tracePtr);
	}
	Blt_Chain_Destroy(isoPtr->traces);
	isoPtr->traces = NULL;
	isoPtr->segments = NULL;
	isoPtr->numSegments = 0;
    }	
    if (elemPtr->traces != NULL) {
	Blt_ChainLink link, next;

	for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
	     link = next) {
	    Trace *tracePtr;

	    next = Blt_Chain_NextLink(link);
	    tracePtr = Blt_Chain_GetValue(link);
	    FreeTrace(elemPtr->traces, tracePtr);
	}
	Blt_Chain_Destroy(elemPtr->traces);
	elemPtr->traces = NULL;
    }
    if (elemPtr->triangles != NULL) {
	Blt_Free(elemPtr->triangles);
	elemPtr->triangles = NULL;
	elemPtr->numTriangles = 0;
    }
    if (elemPtr->vertices != NULL) {
	Blt_Free(elemPtr->vertices);
	elemPtr->vertices = NULL;
	elemPtr->numVertices = 0;
    }
    if (elemPtr->wires != NULL) {
	Blt_Free(elemPtr->wires);
	elemPtr->wires = NULL;
	elemPtr->numWires = 0;
    }
    if (elemPtr->picture != NULL) {
	Blt_FreePicture(elemPtr->picture);
	elemPtr->picture = NULL;
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * GetScreenPoints --
 *
 *	Generates a coordinate array of transformed screen coordinates from
 *	the data points.
 *
 * Results:
 *	The transformed screen coordinates are returned.
 *
 * Side effects:
 *	Memory is allocated for the coordinate array.
 *
 *---------------------------------------------------------------------------
 */
static void
GetScreenPoints(ContourElement *elemPtr)
{
    int i;
    Vertex *vertices;
    Graph *graphPtr = elemPtr->obj.graphPtr;
    Region2d exts;
    Trace *tracePtr;
    AxisRange *rangePtr;

    /* 
     * Step 1: Create array of vertices and apply the current palette.
     */
    vertices = Blt_AssertMalloc(sizeof(Vertex) * elemPtr->z.numValues);
    Blt_GraphExtents(elemPtr, &exts);
    rangePtr = &elemPtr->zAxisPtr->axisRange;
    if (elemPtr->colormapPtr != NULL) {
	Blt_Colormap_Init(elemPtr->colormapPtr);
    }
    for (i = 0; i < elemPtr->meshPtr->numVertices; i++) {
	Point2d p;
	double z;
	Vertex *v;

	v = vertices + i;
	v->index = i;
	p = Blt_Map2D(graphPtr, elemPtr->meshPtr->vertices[i].x, 
		      elemPtr->meshPtr->vertices[i].y, &elemPtr->axes);
	v->x = p.x;
	v->y = p.y;
	if (PointInRegion(&exts, p.x, p.y)) {
	    v->flags |= VISIBLE;
	}
	z = elemPtr->z.values[i];
	if ((elemPtr->zAxisPtr->logScale) && (z != 0.0)) {
	    z = log10(z);
	} 
	z = (z - rangePtr->min)  * rangePtr->scale;
	/* Map graph coordinate to normalized coordinates [0..1] */
	v->z = z;
	if (elemPtr->colormapPtr != NULL) {
	    v->color.u32 = Blt_Colormap_GetColor(elemPtr->colormapPtr, v->z);
	}
    }
    elemPtr->vertices = vertices;
    elemPtr->numVertices = i;
    tracePtr = NewTrace(&elemPtr->traces);
    tracePtr->elemPtr = elemPtr;
    for (i = 0; i < elemPtr->meshPtr->numHullPts; i++) {
    	TracePoint *p;
	int j;

	j = elemPtr->meshPtr->hull[i];
	p = NewPoint(elemPtr, vertices[j].x, vertices[j].y, j);
	AppendPoint(tracePtr, p);
    }
}    


static int
CompareTriangles(const void *a, const void *b)
{
    const Triangle *t1 = a;
    const Triangle *t2 = b;

    /* 
     * Sort first by minimum, then maximum z values.
     */
    if (t1->min < t2->min) {
	return -1;
    } else if (t1->min > t2->min) {
	return 1;
    }
    if (t1->max < t2->max) {
	return -1;
    } else if (t1->max > t2->max) {
	return 1;
    }
    return 0;
}

/* Mapping procedures. */

/*
 *---------------------------------------------------------------------------
 *
 * MapEdges --
 *
 *	Creates an array of the visible (possible clipped) line segments
 *	representing the wireframe of the mesh.  
 *
 *	This can be called only after 1) the screen coordinates of vertices
 *	has been computed and 2) the mesh has been mapped, creating the array
 *	of triangle.
 *	
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is  allocated for the line segment array.
 *
 *---------------------------------------------------------------------------
 */
static void
MapEdges(ContourElement *elemPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Blt_HashTable edgeTable;
    Region2d exts;
    Segment2d *segments;
    Triangle *t, *tend;
    int count;

    /* Use a hash table to generate a list of unique edges.  */
    Blt_InitHashTable(&edgeTable, sizeof(EdgeKey) / sizeof(int));
    for (t = elemPtr->triangles, tend = t + elemPtr->numTriangles; 
	 t < tend; t++) {
	int isNew;
	EdgeKey key;

	MakeEdgeKey(&key, t->a, t->b);
	Blt_CreateHashEntry(&edgeTable, &key, &isNew);
	MakeEdgeKey(&key, t->b, t->c);
	Blt_CreateHashEntry(&edgeTable, &key, &isNew);
	MakeEdgeKey(&key, t->a, t->c);
	Blt_CreateHashEntry(&edgeTable, &key, &isNew);
    }
    Blt_GraphExtents(elemPtr, &exts);
    segments = Blt_AssertMalloc(sizeof(Segment2d) * edgeTable.numEntries);
    count = 0;
    /* Now for each edge, add a (possibly clipped) line segment. */
    for (hPtr = Blt_FirstHashEntry(&edgeTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	EdgeKey *keyPtr;
	Point2d p1, p2;

	keyPtr = Blt_GetHashKey(&edgeTable, hPtr);
	p1.x = elemPtr->vertices[keyPtr->a].x;
	p1.y = elemPtr->vertices[keyPtr->a].y;
	p2.x = elemPtr->vertices[keyPtr->b].x;
	p2.y = elemPtr->vertices[keyPtr->b].y;
	if (Blt_LineRectClip(&exts, &p1, &p2)) {
	    segments[count].p = p1;
	    segments[count].q = p2;
	    count++;
	}
    }
    Blt_DeleteHashTable(&edgeTable);
    /* Resize the array to the actual number of visible segments.  */
    segments = Blt_Realloc(segments, sizeof(Segment2d) * count);
    elemPtr->wires = segments;
    elemPtr->numWires = count;
}

/*
 *---------------------------------------------------------------------------
 *
 * MapTrace --
 *
 *	Adjust the trace by testing each segment of the trace to the graph
 *	area.  If the segment is totally off screen, remove it from the trace.
 *	If one end point is off screen, replace it with the clipped point.
 *	Create new traces as necessary.
 *	
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is  allocated for the line segment array.
 *
 *---------------------------------------------------------------------------
 */
static void
MapTrace(ContourElement *elemPtr, Blt_Chain *tracesPtr, Trace *tracePtr)
{
    TracePoint *p, *q;
    Region2d exts;

    Blt_GraphExtents(elemPtr, &exts);
    for (p = tracePtr->head, q = p->next; q != NULL; q = q->next) {
	if (p->flags & q->flags & VISIBLE) {
	    p = q;
	    continue;			/* Segment is visible. */
	}
	/* Clip required. */
	if (p->flags & VISIBLE) {	/* Last point is off screen. */
	    Point2d p1, p2;

 	    p1.x = p->x, p1.y = p->y;
	    p2.x = q->x, p2.y = q->y;
	    if (Blt_LineRectClip(&exts, &p1, &p2)) {
		TracePoint *t;
		Trace *newPtr;

		/* Last point is off screen.  Add the clipped end the current
		 * trace. */
		t = NewPoint(elemPtr, p2.x, p2.y, p->index);
		t->flags = VISIBLE;
		tracePtr->flags |= RECOUNT;
		tracePtr->tail = t;
		p->next = t;		/* Point t terminates the trace. */

		/* Create a new trace and attach the current chain to it. */
		newPtr = NewTrace(tracesPtr);
		newPtr->elemPtr = elemPtr;
		newPtr->flags |= RECOUNT;
		newPtr->head = newPtr->tail = q;
		tracePtr = newPtr;
	    }
	} else if (q->flags & VISIBLE) {  /* First point in offscreen. */
	    Point2d p1, p2;

	    /* First point is off screen.  Replace it with the clipped end. */
	    p1.x = p->x, p1.y = p->y;
	    p2.x = q->x, p2.y = q->y;
	    if (Blt_LineRectClip(&exts, &p1, &p2)) {
		p->x = p1.x;
		p->y = p1.y;
		/* The replaced point is now visible but longer a knot. */
		p->flags |= VISIBLE;
	    }
	} else {
	    /* Segment is offscreen. Remove the first point. */
	    assert(tracePtr->head == p);
	    RemoveHead(elemPtr, tracePtr);
	}
	p = q;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapTraces --
 *
 *	Creates an array of line segments of the graph coordinates.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is  allocated for the line segment array.
 *
 *---------------------------------------------------------------------------
 */
static void
MapTraces(ContourElement *elemPtr, Blt_Chain *tracesPtr)
{
    Blt_ChainLink link;

    /* Step 1: Process traces by clipping them against the plot area. */
    for (link = Blt_Chain_FirstLink(*tracesPtr); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	Trace *tracePtr;

	tracePtr = Blt_Chain_GetValue(link);
	if (tracePtr->numPoints < 2) {
	    continue;
	}
	MapTrace(elemPtr, tracesPtr, tracePtr);
    }
    /* Step 2: Fix traces that have been split. */
    FixTraces(*tracesPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * MapMesh --
 *
 *	Creates an array of the triangles representing the mesh converted to
 *	screen coordinates.  The range of field values among the three
 *	vertices of the triangle is computed.  This is used later to sort the
 *	triangles.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is  allocated for the triangle array.
 *
 *---------------------------------------------------------------------------
 */
static void
MapMesh(ContourElement *elemPtr)
{
    Triangle *triangles;
    int i;

    triangles = Blt_AssertMalloc(sizeof(Triangle) * 
	elemPtr->meshPtr->numTriangles);
#ifdef notdef
    Blt_GraphExtents(elemPtr, &exts);
#endif
    for (i = 0; i < elemPtr->meshPtr->numTriangles; i++) {
	MeshTriangle *t;

	t = elemPtr->meshPtr->triangles + i;
	triangles[i].a = t->a;
	triangles[i].b = t->b;
	triangles[i].c = t->c;
	triangles[i].min = MIN3(Az, Bz, Cz);
	triangles[i].max = MAX3(Az, Bz, Cz);
	triangles[i].index = i;
#ifdef notdef
	fprintf(stderr, "triangle %d a=%d b=%d c=%d a=%.17g b=%.17g c=%.17g min=%.17g, max=%.17g\n", 
		i, t->a, t->b, t->c, Az, Bz, Cz, triangles[i].min, triangles[i].max);
#endif
#ifdef notdef
	/* Get the triangle's bounding box. */
	bbox.left   = MIN3(Ax, Bx, Cx);
	bbox.right  = MAX3(Ax, Bx, Cx);
	bbox.top    = MIN3(Ay, By, Cy);
	bbox.bottom = MAX3(Ay, By, Cy);

	/* Do a quick minmax test on the bounding box with the plot area. */
	if ((bbox.right < exts.left) || (bbox.bottom < exts.top) ||
	    (bbox.top > exts.bottom) || (bbox.left > exts.right)) {
	    triangles[i].flags = VISIBLE; /* Triangle isn't visible. */
	    /* Get z-min, z-max for all visible triangles. */
	    /* Will generate palette scaled from 0 to 1 of possible colors
	     * from min to max. */
	}
#endif
    }
    /* Next sort the triangles by the current set of field values */
    qsort(triangles, elemPtr->meshPtr->numTriangles, sizeof(Triangle), 
	  CompareTriangles);
    if (elemPtr->triangles != NULL) {
	Blt_Free(elemPtr->triangles);
    }
    elemPtr->triangles = triangles;
    elemPtr->numTriangles = elemPtr->meshPtr->numTriangles;
    MapEdges(elemPtr);
    /* Map the convex hull representing the boundary of the mesh. */
    MapTraces(elemPtr, &elemPtr->traces);
}    
    
/* Process a Cont triangle  */
static void 
ProcessTriangle(ContourElement *elemPtr, Triangle *t, Isoline *isoPtr) 
{
    int ab, bc, ca;
    double t1, t2, t3, range;
    Region2d exts;

    static int classify[3][3][3] = {
	{
	    { 
		0,			/* 0: 0,0,0 */
		0,			/* 1: 0,0,1	AC vertex.*/
		0			/* 2: 0,0,2	AC interpolated point. 
					 */
	    },
	    { 
		0,			/* 3: 0,1,0	BC vertex. */
		1,			/* 4: 0,1,1	BC and AC vertices. */
		1			/* 5: 0,1,2	BC vertex, 
					 *		AC interpolated point. 
					 */
	    },
	    { 
		0,			/* 6: 0,2,0	BC interpolated point.*/
		1,			/* 7: 0,2,1     BC interpolated point, 
					 *		AC vertex. */
		1			/* 8: 0,2,2	BC interpolated point, 
					 *		AC interpolated point. 
					 */
	    }
        },
        {
	    { 
		0,			/* 9: 1,0,0	AB vertex. */
		1,			/* 10: 1,0,1 */
		1			/* 11: 1,0,2 */
	    },
	    { 
		1,			/* 12: 1,1,0 */
		0,			/* 13: 1,1,1 */
		1			/* 14: 1,1,2 */
	    },
	    { 
		1,			/* 15: 1,2,0 */
		1,			/* 16: 1,2,1 */
		0			/* 17: 1,2,2	Not possible. */
	    }
        },
        {
	    { 
		0,			/* 18: 2,0,0 */
		1,			/* 19: 2,0,1 */
		1			/* 20: 2,0,2 */
	    },
	    { 
		1,			/* 21: 2,1,0 */
		1,			/* 22: 2,1,1 */
		0			/* 23: 2,1,2 */
	    },
	    { 
		1,			/* 24: 2,2,0 */
		0,			/* 25: 2,2,1 */
		0			/* 26: 2,2,2 */
	    }
        }
    };
    
    Blt_GraphExtents(elemPtr, &exts);
    t1 = t2 = t3 = 0.0;
    ab = bc = ca = 0;
    range = Bz - Az;
    if (fabs(range) < DBL_EPSILON) {
	ab = Blt_AlmostEquals(Az, isoPtr->value);
    } else {
	t1 = (isoPtr->value - Az) / range; /* A to B */
	if (Blt_AlmostEquals(t1, 0.0)) {
	    ab = 1;			/* At a vertex. */
	} else if ((t1 < 0.0) || (t1 > 1.0)) {
	    ab = 0;			/* Outside of edge. */
	} else {
	    ab = 2;			/* Inside of edge. */
	}
    }
    range = Cz - Bz;
    if (fabs(range) < DBL_EPSILON) {
	bc = Blt_AlmostEquals(Bz, isoPtr->value);
    } else {
	t2 = (isoPtr->value - Bz) / range; /* B to C */
	if (Blt_AlmostEquals(t2, 0.0)) {
	    bc = 1;				/* At a vertex. */
	} else if ((t2 < 0.0) || (t2 > 1.0)) {
	    bc = 0;				/* Outside of edge. */
	} else {
	    bc = 2;				/* Inside of edge. */
	}
    }

    range = Az - Cz;
    if (fabs(range) < DBL_EPSILON) {
	ca = Blt_AlmostEquals(Cz, isoPtr->value);
    } else {
	t3 = (isoPtr->value - Cz) / range; /* A to B */
	if (Blt_AlmostEquals(t3, 0.0)) {
	    ca = 1;			/* At a vertex. */
	} else if ((t3 < 0.0) || (t3 > 1.0)) {
	    ca = 0;			/* Outside of edge. */
	} else {
	    ca = 2;			/* Inside of edge. */
	}
    }
    if (classify[ab][bc][ca]) {
	if (ab > 0) {
	    if (bc > 0) {
		Point2d p, q;
		int result;

		/* Compute interpolated points ab and bc */
		p.x = Ax + t1 * (Bx - Ax);
		p.y = Ay + t1 * (By - Ay);
		q.x = Bx + t2 * (Cx - Bx);
		q.y = By + t2 * (Cy - By);
		result = Blt_LineRectClip(&exts, &p, &q);
		if (result > 0) {
		    TraceSegment *s;

		    s = AddSegment(elemPtr, p.x, p.y, q.x, q.y, isoPtr);
		    s->flags |= result;
		}
	    } else if (ca > 0) {
		Point2d p, q;
		int result;

		/* Compute interpolated points ab and ac */
		p.x = Ax + t1 * (Bx - Ax);
		p.y = Ay + t1 * (By - Ay);
		q.x = Cx + t3 * (Ax - Cx);
		q.y = Cy + t3 * (Ay - Cy);
		result = Blt_LineRectClip(&exts, &p, &q);
		if (result > 0) {
		    TraceSegment *s;

		    s = AddSegment(elemPtr, p.x, p.y, q.x, q.y, isoPtr);
		    s->flags |= result;
		}
	    }
	} else if (bc > 0) {
	    if (ca > 0) {
		Point2d p, q;
		int result;

		/* Compute interpolated points bc and ac */
		p.x = Bx + t2 * (Cx - Bx);
		p.y = By + t2 * (Cy - By);
		q.x = Cx + t3 * (Ax - Cx);
		q.y = Cy + t3 * (Ay - Cy);
		result = Blt_LineRectClip(&exts, &p, &q);
		if (result > 0) {
		    TraceSegment *s;

		    s = AddSegment(elemPtr, p.x, p.y, q.x, q.y, isoPtr);
		    s->flags |= result;
		}
	    }
	} else {
	    /* Can't happen. Must have two interpolated points or vertices. */
	}
    } else {
#ifndef notdef
	fprintf(stderr,
		"ignoring triangle %d a=%d b=%d c=%d value=%.17g a=%.17g b=%.17g c=%.17g\n",
		t->index, t->a, t->b, t->c, isoPtr->value, Az, Bz, Cz);
	fprintf(stderr, "\tab=%d, bc=%d ca=%d\n", ab, bc, ca);
	fprintf(stderr, "\tt1=%.17g t2=%.17g t3=%.17g\n", t1, t2, t3);
	fprintf(stderr, "\tt->min=%.17g t->max=%.17g MIN3=%.17g MAX3=%.17g\n", 
		t->min, t->max, MIN3(Az,Bz,Cz), MAX3(Az,Bz,Cz));
#endif
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapIsoline --
 *
 *	Maps the isoline.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is  allocated for the line segment array.
 *
 *---------------------------------------------------------------------------
 */
static void
MapIsoline(Isoline *isoPtr)
{
    Triangle *t, *tend;
    ContourElement *elemPtr;
    AxisRange *rangePtr;

    elemPtr = isoPtr->elemPtr;
    rangePtr = &elemPtr->zAxisPtr->axisRange;
    if (isoPtr->flags & ABSOLUT) {
	if (fabs(rangePtr->range) < DBL_EPSILON) {
	    return;
	}
	isoPtr->value = (isoPtr->reqValue - rangePtr->min) * rangePtr->scale;
    } else {
	isoPtr->value = isoPtr->reqValue;
    }
    if (elemPtr->colormapPtr != NULL) {
        isoPtr->paletteColor.u32 = Blt_Colormap_GetColor(elemPtr->colormapPtr, 
		isoPtr->value);
    } else {
	isoPtr->paletteColor.u32 = 0xFF000000;
    }
    /* Process the isoline, computing its line segments looking at all
     * relevant triangles in the mesh. */
    Blt_InitHashTable(&isoPtr->pointTable, sizeof(PointKey) / sizeof(int));
    for (t = elemPtr->triangles, tend = t+elemPtr->numTriangles; t < tend; t++){
	double norm, range;

	range = t->max - t->min;
	if (fabs(range) < DBL_EPSILON) {
	    continue;			/* All three vertices have the same 
					 * value. */
	} 
	norm = (isoPtr->value - t->min) / range;
	if ((norm < 0.0) && (!Blt_AlmostEquals(norm, 0.0))) {
	    break;			/* No more triangles in range. */
	}
	if ((norm < 1.0) || (Blt_AlmostEquals(norm, 1.0))) {
	    ProcessTriangle(elemPtr, t, isoPtr);
	}
    }
    Blt_DeleteHashTable(&isoPtr->pointTable);
}

/* Draw single symbol procedures. */

#ifdef WIN32
/* 
 * DrawCircleSymbol --
 *
 *	Draws the symbols of the trace as circles.  The outlines of circles
 *	are drawn after circles are filled.  This is speed tradeoff: drawn
 *	many circles at once, or drawn one symbol at a time.
 *
 *	Symbols are only drawn at the knots of the trace (i.e. original points,
 *	not generated).  The "play" function can limit what circles are drawn.
 *
 */
static void
DrawCircleSymbol(Graph *graphPtr, Drawable drawable, ContourPen *penPtr, 
		 int x, int y, int size)
{
    HBRUSH brush, oldBrush;
    HPEN pen, oldPen;
    HDC dc;
    TkWinDCState state;
    int r;

    r = (int)ceil(size * 0.5);
    if (drawable == None) {
	return;				/* Huh? */
    }
    if ((penPtr->symbol.fillGC == NULL) && 
	(penPtr->symbol.outlineWidth == 0)) {
	return;
    }
    dc = TkWinGetDrawableDC(graphPtr->display, drawable, &state);
    /* SetROP2(dc, tkpWinRopModes[penPtr->symbol.fillGC->function]); */
    if (penPtr->symbol.fillGC != NULL) {
	brush = CreateSolidBrush(penPtr->symbol.fillGC->foreground);
    } else {
	brush = GetStockBrush(NULL_BRUSH);
    }
    if (penPtr->symbol.outlineWidth > 0) {
	pen = Blt_GCToPen(dc, penPtr->symbol.outlineGC);
    } else {
	pen = GetStockPen(NULL_PEN);
    }
    oldPen = SelectPen(dc, pen);
    oldBrush = SelectBrush(dc, brush);
    Ellipse(dc, x - r, y - r, x + r + 1, y + r + 1);
    DeleteBrush(SelectBrush(dc, oldBrush));
    DeletePen(SelectPen(dc, oldPen));
    TkWinReleaseDrawableDC(drawable, dc, &state);
}

#else

/* 
 * DrawCircleSymbol --
 *
 *	Draws the symbols of the trace as circles.  The outlines of circles
 *	are drawn after circles are filled.  This is speed tradeoff: draw
 *	many circles at once, or drawn one symbol at a time.
 *
 *	Symbols are only drawn at the knots of the trace (i.e. original points,
 *	not generated).  The "play" function can limit what circles are drawn.
 *
 */
static void
DrawCircleSymbol(Graph *graphPtr, Drawable drawable, ContourPen *penPtr, 
		 int x, int y, int size)
{
    int r, s;

    r = (int)ceil(size * 0.5);
    s = r + r;
    if (penPtr->symbol.fillGC != NULL) {
	XFillArc(graphPtr->display, drawable, penPtr->symbol.fillGC, 
		  x - r, y - r,  s,  s, 0, 23040);
    }
    if (penPtr->symbol.outlineWidth > 0) {
	XDrawArc(graphPtr->display, drawable, penPtr->symbol.outlineGC, 
		  x - r, y - r,  s,  s, 0, 23040);
    }
}

#endif	/* WIN32 */

/* 
 * DrawSquareSymbol --
 *
 *	Draws the symbols of the trace as squares.  The outlines of squares
 *	are drawn after squares are filled.  This is speed tradeoff: draw
 *	many squares at once, or drawn one symbol at a time.
 *
 *	Symbols are only drawn at the knots of the trace (i.e. original points,
 *	not generated).  The "play" function can limit what squares are drawn.
 *
 */
static void
DrawSquareSymbol(Graph *graphPtr, Drawable drawable, ContourPen *penPtr, 
		 int x, int y, int size)
{
    int r, s;

    r = (int)ceil(size * S_RATIO * 0.5);
    s = r + r;
    if (penPtr->symbol.fillGC != NULL) {
	XFillRectangle(graphPtr->display, drawable, penPtr->symbol.fillGC, 
			x - r, y - r,  s, s);
    }
    if (penPtr->symbol.outlineWidth > 0) {
	XDrawRectangle(graphPtr->display, drawable, penPtr->symbol.outlineGC, 
			x - r, y - r,  s, s);
    }
}

/* 
 * DrawSkinnyCrossPlusSymbol --
 *
 *	Draws the symbols of the trace as single line crosses or pluses.  
 *
 *	Symbols are only drawn at the knots of the trace (i.e. original points,
 *	not generated).  The "play" function can limit what symbols are drawn.
 */
static void
DrawSkinnyCrossPlusSymbol(Graph *graphPtr, Drawable drawable, ContourPen *penPtr, 
			  int x, int y, int size)
{
    XPoint pattern[13];			/* Template for polygon symbols */
    XSegment segments[2];
    int r;

    r = (int)ceil(size * 0.5);
    if (penPtr->symbol.type == SYMBOL_SCROSS) {
	r = Round((double)r * M_SQRT1_2);
	pattern[3].y = pattern[2].x = pattern[0].x = pattern[0].y = -r;
	pattern[3].x = pattern[2].y = pattern[1].y = pattern[1].x = r;
    } else {
	pattern[0].y = pattern[1].y = pattern[2].x = pattern[3].x = 0;
	pattern[0].x = pattern[2].y = -r;
	pattern[1].x = pattern[3].y = r;
    }
    segments[0].x1 = pattern[0].x + x;
    segments[0].y1 = pattern[0].y + y;
    segments[0].x2 = pattern[1].x + x;
    segments[0].y2 = pattern[1].y + y;
    segments[1].x1 = pattern[2].x + x;
    segments[1].y1 = pattern[2].y + y;
    segments[1].x2 = pattern[3].x + x;
    segments[1].y2 = pattern[3].y + y;
    XDrawSegments(graphPtr->display, drawable, penPtr->symbol.outlineGC, 
	segments, 2);
}

static void
DrawCrossPlusSymbol(Graph *graphPtr, Drawable drawable, ContourPen *penPtr, 
		    int x, int y, int size)
{
    XPoint polygon[13];
    int r;
    int d;			/* Small delta for cross/plus
				 * thickness */
    int i;

    r = (int)ceil(size * S_RATIO * 0.5);
    d = (r / 3);
    /*
     *
     *          2   3       The plus/cross symbol is a closed polygon
     *                      of 12 points. The diagram to the left
     *    0,12  1   4    5  represents the positions of the points
     *           x,y        which are computed below. The extra
     *     11  10   7    6  (thirteenth) point connects the first and
     *                      last points.
     *          9   8
     */
    polygon[0].x = polygon[11].x = polygon[12].x = -r;
    polygon[2].x = polygon[1].x = polygon[10].x = polygon[9].x = -d;
    polygon[3].x = polygon[4].x = polygon[7].x = polygon[8].x = d;
    polygon[5].x = polygon[6].x = r;
    polygon[2].y = polygon[3].y = -r;
    polygon[0].y = polygon[1].y = polygon[4].y = polygon[5].y =
	polygon[12].y = -d;
    polygon[11].y = polygon[10].y = polygon[7].y = polygon[6].y = d;
    polygon[9].y = polygon[8].y = r;
    
    if (penPtr->symbol.type == SYMBOL_CROSS) {
	int i;

	/* For the cross symbol, rotate the points by 45 degrees. */
	for (i = 0; i < 12; i++) {
	    double dx, dy;
	    
	    dx = (double)polygon[i].x * M_SQRT1_2;
	    dy = (double)polygon[i].y * M_SQRT1_2;
	    polygon[i].x = Round(dx - dy);
	    polygon[i].y = Round(dx + dy);
	}
	polygon[12] = polygon[0];
    }
    for (i = 0; i < 13; i++) {
	polygon[i].x += x;
	polygon[i].y += y;
    }
    if (penPtr->symbol.fillGC != NULL) {
	XFillPolygon(graphPtr->display, drawable, penPtr->symbol.fillGC, 
	     polygon, 13, Complex, CoordModeOrigin);
    }
    if (penPtr->symbol.outlineWidth > 0) {
	XDrawLines(graphPtr->display, drawable, penPtr->symbol.outlineGC, 
	   polygon, 13, CoordModeOrigin);
    }
}

static void
DrawTriangleArrowSymbol(Graph *graphPtr, Drawable drawable, ContourPen *penPtr, 
			int x, int y, int size)
{
    XPoint polygon[4];
    double b;
    int b2, h1, h2;
    int i;

#define H_RATIO		1.1663402261671607
#define B_RATIO		1.3467736870885982
#define TAN30		0.57735026918962573
#define COS30		0.86602540378443871
    b = Round(size * B_RATIO * 0.7);
    b2 = Round(b * 0.5);
    h2 = Round(TAN30 * b2);
    h1 = Round(b2 / COS30);
    /*
     *
     *                      The triangle symbol is a closed polygon
     *           0,3         of 3 points. The diagram to the left
     *                      represents the positions of the points
     *           x,y        which are computed below. The extra
     *                      (fourth) point connects the first and
     *      2           1   last points.
     *
     */
    
    if (penPtr->symbol.type == SYMBOL_ARROW) {
	polygon[3].x = polygon[0].x = 0;
	polygon[3].y = polygon[0].y = h1;
	polygon[1].x = b2;
	polygon[2].y = polygon[1].y = -h2;
	polygon[2].x = -b2;
    } else {
	polygon[3].x = polygon[0].x = 0;
	polygon[3].y = polygon[0].y = -h1;
	polygon[1].x = b2;
	polygon[2].y = polygon[1].y = h2;
	polygon[2].x = -b2;
    }
    for (i = 0; i < 4; i++) {
	polygon[i].x += x;
	polygon[i].y += y;
    }
    if (penPtr->symbol.fillGC != NULL) {
	XFillPolygon(graphPtr->display, drawable, penPtr->symbol.fillGC, 
		     polygon, 4, Convex, CoordModeOrigin);
    }
    if (penPtr->symbol.outlineWidth > 0) {
	XDrawLines(graphPtr->display, drawable, penPtr->symbol.outlineGC, 
		   polygon, 4, CoordModeOrigin);
    }
}

static void
DrawDiamondSymbol(Graph *graphPtr, Drawable drawable, ContourPen *penPtr, 
		  int x, int y, int size)
{
    XPoint polygon[5];
    int r;
    int i;

    /*
     *
     *                      The diamond symbol is a closed polygon
     *            1         of 4 points. The diagram to the left
     *                      represents the positions of the points
     *       0,4 x,y  2     which are computed below. The extra
     *                      (fifth) point connects the first and
     *            3         last points.
     *
     */
    r = (int)ceil(size * 0.5);
    polygon[1].y = polygon[0].x = -r;
    polygon[2].y = polygon[3].x = polygon[0].y = polygon[1].x = 0;
    polygon[3].y = polygon[2].x = r;
    polygon[4] = polygon[0];
    
    for (i = 0; i < 5; i++) {
	polygon[i].x += x;
	polygon[i].y += y;
    }
    if (penPtr->symbol.fillGC != NULL) {
	XFillPolygon(graphPtr->display, drawable, penPtr->symbol.fillGC, 
		     polygon, 5, Convex, CoordModeOrigin);
    }
    if (penPtr->symbol.outlineWidth > 0) {
	XDrawLines(graphPtr->display, drawable, penPtr->symbol.outlineGC, 
		   polygon, 5, CoordModeOrigin);
    }
}

static void
DrawImageSymbol(Graph *graphPtr, Drawable drawable, ContourPen *penPtr, 
		int x, int y, int size)
{
    int w, h;
    int dx, dy;

    Tk_SizeOfImage(penPtr->symbol.image, &w, &h);
    dx = w / 2;
    dy = h / 2;
    x = x - dx;
    y = y - dy;
    Tk_RedrawImage(penPtr->symbol.image, 0, 0, w, h, drawable, x, y);
}

static void
DrawBitmapSymbol(Graph *graphPtr, Drawable drawable, ContourPen *penPtr, 
		 int x, int y, int size)
{
    Pixmap bitmap, mask;
    int w, h, bw, bh;
    double scale, sx, sy;
    int dx, dy;

    Tk_SizeOfBitmap(graphPtr->display, penPtr->symbol.bitmap, &w, &h);
    mask = None;
    
    /*
     * Compute the size of the scaled bitmap.  Stretch the bitmap to fit
     * a nxn bounding box.
     */
    sx = (double)size / (double)w;
    sy = (double)size / (double)h;
    scale = MIN(sx, sy);
    bw = (int)(w * scale);
    bh = (int)(h * scale);
    
    XSetClipMask(graphPtr->display, penPtr->symbol.outlineGC, None);
    if (penPtr->symbol.mask != None) {
	mask = Blt_ScaleBitmap(graphPtr->tkwin, penPtr->symbol.mask, w, h, 
		bw, bh);
	XSetClipMask(graphPtr->display, penPtr->symbol.outlineGC, mask);
    }
    bitmap = Blt_ScaleBitmap(graphPtr->tkwin, penPtr->symbol.bitmap, w, h, bw, 
			     bh);
    if (penPtr->symbol.fillGC == NULL) {
	XSetClipMask(graphPtr->display, penPtr->symbol.outlineGC, bitmap);
    }
    dx = bw / 2;
    dy = bh / 2;
    x = x - dx;
    y = y - dy;
    if ((penPtr->symbol.fillGC == NULL) || (mask !=None)) {
	XSetClipOrigin(graphPtr->display, penPtr->symbol.outlineGC, x, y);
    }
    XCopyPlane(graphPtr->display, bitmap, drawable, penPtr->symbol.outlineGC, 
	0, 0, bw, bh, x, y, 1);
    Tk_FreePixmap(graphPtr->display, bitmap);
    if (mask != None) {
	Tk_FreePixmap(graphPtr->display, mask);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawSymbol --
 *
 * 	Draw the symbols centered at the each given x,y coordinate in the
 * 	array of points.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Draws a symbol at each coordinate given.  If active, only those
 *	coordinates which are currently active are drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawSymbol(
    Graph *graphPtr,			/* Graph widget record */
    Drawable drawable,			/* Pixmap or window to draw into */
    ContourPen *penPtr, 
    int x, int y, int size)
{
    switch (penPtr->symbol.type) {
    case SYMBOL_NONE:
	break;
	
    case SYMBOL_SQUARE:
	DrawSquareSymbol(graphPtr, drawable, penPtr, x, y, size);
	break;
	
    case SYMBOL_CIRCLE:
	DrawCircleSymbol(graphPtr, drawable, penPtr, x, y, size);
	break;
	
    case SYMBOL_SPLUS:
    case SYMBOL_SCROSS:
	DrawSkinnyCrossPlusSymbol(graphPtr, drawable, penPtr, x, y, size);
	break;
	
    case SYMBOL_PLUS:
    case SYMBOL_CROSS:
	DrawCrossPlusSymbol(graphPtr, drawable, penPtr, x, y, size);
	break;
	
    case SYMBOL_DIAMOND:
	DrawDiamondSymbol(graphPtr, drawable, penPtr, x, y, size);
	break;
	
    case SYMBOL_TRIANGLE:
    case SYMBOL_ARROW:
	DrawTriangleArrowSymbol(graphPtr, drawable, penPtr, x, y, size);
	break;
	
    case SYMBOL_IMAGE:
	DrawImageSymbol(graphPtr, drawable, penPtr, x, y, size);
	break;
	
    case SYMBOL_BITMAP:
	DrawBitmapSymbol(graphPtr, drawable, penPtr, x, y, size);
	break;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawSymbolProc --
 *
 * 	Draw the symbol centered at the each given x,y coordinate.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Draws a symbol at the coordinate given.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawSymbolProc(
    Graph *graphPtr,			/* Graph widget record */
    Drawable drawable,			/* Pixmap or window to draw into */
    Element *basePtr,			/* Line element information */
    int x, int y,			/* Center position of symbol */
    int size)				/* Size of symbol. */
{
    ContourElement *elemPtr = (ContourElement *)basePtr;
    ContourPen *penPtr;

    penPtr = (ContourPen *)PEN(elemPtr);
    if (penPtr->traceWidth > 0) {
	/*
	 * Draw an extra line offset by one pixel from the previous to give a
	 * thicker appearance.  This is only for the legend entry.  This
	 * routine is never called for drawing the actual line segments.
	 */
	XDrawLine(graphPtr->display, drawable, penPtr->traceGC, 
		  x - size, y, x + size, y);
	XDrawLine(graphPtr->display, drawable, penPtr->traceGC, 
		  x - size, y + 1, x + size, y + 1);
    }
    if (penPtr->symbol.type != SYMBOL_NONE) {
	DrawSymbol(graphPtr, drawable, penPtr, x, y, size);
    }
}

/* Drawing procedures. */

#ifdef WIN32
/* 
 * DrawPolyline --
 *
 *	Draws the connected line segments representing the trace.
 *
 *	This MSWindows version arbitrarily breaks traces greater than one
 *	hundred points that are wide lines, into smaller pieces.
 */
static void
DrawPolyline(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
	     ContourPen *penPtr)
{
    HBRUSH brush, oldBrush;
    HDC dc;
    HPEN pen, oldPen;
    POINT *points;
    TkWinDCState state;
    int maxPoints;			/* Maximum # of points in a single
					 * polyline. */
    TracePoint *p;
    int count;
    size_t numReq, numMax, count;
#ifdef notdef
    fprintf(stderr, "Entry DrawPolyline\n");
#endif
    /*  
     * If the line is wide (> 1 pixel), arbitrarily break the line in sections
     * of 100 points.  This bit of weirdness has to do with wide geometric
     * pens.  The longer the polyline, the slower it draws.  The trade off is
     * that we lose dash and cap uniformity for unbearably slow polyline
     * draws.
     */
    numReq = tracePtr->numPoints;
    if (penPtr->traceGC->line_width > 1) {
	numMax = 100;
    } else {
	numMax = Blt_MaxRequestSize(graphPtr->display, sizeof(POINT)) - 1;
    }
    if ((numMax == 0) || (numMax > numReq)) {
	numMax = numReq;
    }
    points = Blt_AssertMalloc((numMax + 1) * sizeof(POINT));

    dc = TkWinGetDrawableDC(graphPtr->display, drawable, &state);

    /* FIXME: Add clipping region here. */

    pen = Blt_GCToPen(dc, penPtr->traceGC);
    oldPen = SelectPen(dc, pen);
    brush = CreateSolidBrush(penPtr->traceGC->foreground);
    oldBrush = SelectBrush(dc, brush);
    SetROP2(dc, tkpWinRopModes[penPtr->traceGC->function]);

    count = 0;
    for (p = tracePtr->head; p != NULL; p = p->next) {
	points[count].x = Round(p->x);
	points[count].y = Round(p->y);
	count++;
	if (count >= numMax) {
	    Polyline(dc, points, count);
	    points[0] = points[count - 1];
	    count = 1;
	}
    }
    if (count > 1) {
	Polyline(dc, points, count);
    }
    Blt_Free(points);
    DeletePen(SelectPen(dc, oldPen));
    DeleteBrush(SelectBrush(dc, oldBrush));
    TkWinReleaseDrawableDC(drawable, dc, &state);
#ifdef notdef
    fprintf(stderr, "Leave DrawPolyline\n");
#endif
}

#else

/* 
 * DrawPolyline --
 *
 *	Draws the connected line segments representing the trace.
 *
 *	This X11 version arbitrarily breaks traces greater than the server
 *	request size, into smaller pieces.
 */
static void
DrawPolyline(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
	     ContourPen *penPtr)
{
    TracePoint *p;
    XPoint *points;
    size_t numMax, numReq, count;

    numReq = tracePtr->numPoints;
    numMax = MAX_DRAWLINES(graphPtr->display);
    if ((numMax == 0) || (numMax > numReq)) {
	numMax = numReq;
    } 
    points = Blt_AssertMalloc((numMax + 1) * sizeof(XPoint));
    count = 0;			/* Counter for points */
#ifdef notdef
    fprintf(stderr, "Drawing %d points\n", tracePtr->numPoints);
#endif
    for (p = tracePtr->head; p != NULL; p = p->next) {
#ifdef notdef
	fprintf(stderr, "%d: x=%g,y=%g\n", p->index, p->x, p->y);
#endif
	points[count].x = Round(p->x);
	points[count].y = Round(p->y);
	count++;
	if (count >= numMax) {
	    XDrawLines(graphPtr->display, drawable, penPtr->traceGC, points, 
		count, CoordModeOrigin);
	    points[0] = points[count - 1];
	    count = 1;
	}
    }
    if (count > 1) {
	XDrawLines(graphPtr->display, drawable, penPtr->traceGC, points, count, 
		CoordModeOrigin);
    }
    Blt_Free(points);
}
#endif /* WIN32 */

/*
 *---------------------------------------------------------------------------
 *
 * DrawTriangles --
 *
 * 	Draws the segments forming of the mesh wireframe.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawTriangles(Graph *graphPtr, Drawable drawable, ContourElement *elemPtr,
	      ContourPen *penPtr)
{
    Region2d exts;
    Triangle *t, *tend;
    int x, y, w, h;

    Blt_GraphExtents(elemPtr, &exts);
    w = (exts.right - exts.left) + 1;
    h = (exts.bottom - exts.top) + 1;
    if (elemPtr->picture != NULL) {
	Blt_FreePicture(elemPtr->picture);
    }
    elemPtr->picture = Blt_CreatePicture(w, h);
    Blt_BlankPicture(elemPtr->picture, 0x0);
    x = exts.left, y = exts.top;
    for (t = elemPtr->triangles, tend = t + elemPtr->numTriangles; 
	 t < tend; t++) {
	DrawTriangle(elemPtr, elemPtr->picture, t, x, y);
    }
    if (InRange(elemPtr->opacity, 0.0, 100.0)) {
	int alpha;

	alpha = 255 - (int) (255.0 * (elemPtr->opacity * 0.01) + 0.5);
	Blt_FadePicture(elemPtr->picture, 0, 0, w, h, alpha);
    }
    Blt_PaintPictureWithBlend(elemPtr->painter, drawable, elemPtr->picture, 
	0, 0, w, h, exts.left, exts.top, 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawEdges --
 *
 * 	Draws the segments forming of the mesh grid.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawEdges(Graph *graphPtr, Drawable drawable, ContourElement *elemPtr,
	      ContourPen *penPtr)
{
    XSegment *segments;
    Segment2d *s, *send;
    size_t numMax, numReq, count;

    numReq = elemPtr->numWires;
    numMax = MAX_DRAWSEGMENTS(graphPtr->display);
    if ((numMax == 0) || (numMax > numReq)) {
	numMax = numReq;
    } 
    segments = Blt_Malloc(numMax * sizeof(XSegment));
    if (segments == NULL) {
	return;
    }
    count = 0;				/* Counter for segments */
    for (s = elemPtr->wires, send = s + elemPtr->numWires; s < send; s++) {
	segments[count].x1 = (short int)Round(s->p.x);
	segments[count].y1 = (short int)Round(s->p.y);
	segments[count].x2 = (short int)Round(s->q.x);
	segments[count].y2 = (short int)Round(s->q.y);
	count++;
	if (count >= numMax) {
	    XDrawSegments(graphPtr->display, drawable, elemPtr->meshGC, 
		segments, count);
	    count = 0;
	}
    }
    if (count > 0) {
	XDrawSegments(graphPtr->display, drawable, elemPtr->meshGC, segments, 
		      count);
    }
    Blt_Free(segments);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawHull --
 *
 * 	Draws the convex hull representing the boundary of the mesh.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawHull(Graph *graphPtr, Drawable drawable, ContourElement *elemPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
	Trace *tracePtr;
	
	tracePtr = Blt_Chain_GetValue(link);
	DrawPolyline(graphPtr, drawable, tracePtr, elemPtr->boundaryPenPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawMesh --
 *
 * 	Draws the numeric value of the bar.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawMesh(Graph *graphPtr, Drawable drawable, ContourElement *elemPtr)
{
    ContourPen *penPtr;

    penPtr = PEN(elemPtr);
    if (elemPtr->flags & COLORMAP) {
	DrawTriangles(graphPtr, drawable, elemPtr, penPtr);
    }
    if (elemPtr->flags & EDGES) {
	DrawEdges(graphPtr, drawable, elemPtr, penPtr);
    }
    if (elemPtr->flags & HULL) {
	DrawHull(graphPtr, drawable, elemPtr);
    }
}

/* Draw multiple-symbol procedure. */

/* 
 * DrawPointSymbols --
 *
 *	Draws the symbols of the trace as points.
 *
 *	Symbols are only drawn at the knots of the trace (i.e. original points,
 *	not generated).  The "play" function can limit what points are drawn.
 */
static void
DrawPointSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
		 ContourPen *penPtr)
{
    TracePoint *p;
    XPoint *points;
    size_t numMax, numReq, count;

    numReq = tracePtr->numPoints;
    numMax = MAX_DRAWPOINTS(graphPtr->display);
    if ((numMax == 0) || (numMax > numReq)) {
	numMax = numReq;
    } 
    points = Blt_Malloc(numMax * sizeof(XPoint));
    if (points == NULL) {
	return;
    }
    count = 0;				/* Counter for points. */
    for (p = tracePtr->head; p != NULL; p = p->next) {
#ifdef notdef
	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
#endif
	points[count].x = Round(p->x);
	points[count].y = Round(p->y);
	count++;
	if (count >= numMax) {
	    XDrawPoints(graphPtr->display, drawable, penPtr->symbol.fillGC, 
			points, count, CoordModeOrigin);
	    count = 0;
	}
    }
    if (count > 0) {
	XDrawPoints(graphPtr->display, drawable, penPtr->symbol.fillGC, 
		    points, count, CoordModeOrigin);
    }
    Blt_Free(points);
}


#ifdef WIN32

/* 
 * DrawCircleSymbols --
 *
 *	Draws the symbols of the trace as circles.  The outlines of circles
 *	are drawn after circles are filled.  This is speed tradeoff: drawn
 *	many circles at once, or drawn one symbol at a time.
 *
 *	Symbols are only drawn at the knots of the trace (i.e. original points,
 *	not generated).  The "play" function can limit what circles are drawn.
 *
 */
static void
DrawCircleSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
		  ContourPen *penPtr, int size)
{
    HBRUSH brush, oldBrush;
    HPEN pen, oldPen;
    HDC dc;
    TkWinDCState state;
    int r;
    TracePoint *p;

    r = (int)ceil(size * 0.5);
    if (drawable == None) {
	return;				/* Huh? */
    }
    if ((penPtr->symbol.fillGC == NULL) && 
	(penPtr->symbol.outlineWidth == 0)) {
	return;
    }
    dc = TkWinGetDrawableDC(graphPtr->display, drawable, &state);
    /* SetROP2(dc, tkpWinRopModes[penPtr->symbol.fillGC->function]); */
    if (penPtr->symbol.fillGC != NULL) {
	brush = CreateSolidBrush(penPtr->symbol.fillGC->foreground);
    } else {
	brush = GetStockBrush(NULL_BRUSH);
    }
    if (penPtr->symbol.outlineWidth > 0) {
	pen = Blt_GCToPen(dc, penPtr->symbol.outlineGC);
    } else {
	pen = GetStockPen(NULL_PEN);
    }
    oldPen = SelectPen(dc, pen);
    oldBrush = SelectBrush(dc, brush);
    for (p = tracePtr->head; p != NULL; p = p->next) {
	int rx, ry;

	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	rx = Round(p->x);
	ry = Round(p->y);
	Ellipse(dc, rx - r, ry - r, rx + r + 1, ry + r + 1);
    }
    DeleteBrush(SelectBrush(dc, oldBrush));
    DeletePen(SelectPen(dc, oldPen));
    TkWinReleaseDrawableDC(drawable, dc, &state);
}

#else

/* 
 * DrawCircleSymbols --
 *
 *	Draws the symbols of the trace as circles.  The outlines of circles
 *	are drawn after circles are filled.  This is speed tradeoff: draw
 *	many circles at once, or drawn one symbol at a time.
 *
 *	Symbols are only drawn at the knots of the trace (i.e. original points,
 *	not generated).  The "play" function can limit what circles are drawn.
 *
 */
static void
DrawCircleSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
		  ContourPen *penPtr, int size)
{
    TracePoint *p;
    XArc *arcs;
    int r, s;
    size_t numMax, numReq, count;

    numReq = tracePtr->numPoints;
    numMax = MAX_DRAWARCS(graphPtr->display);
    if ((numMax == 0) || (numMax > numReq)) {
	numMax = numReq;
    } 
    arcs = Blt_Malloc(numMax * sizeof(XArc));
    if (arcs == NULL) {
	return;
    }

    r = (int)ceil(size * 0.5);
    s = r + r;

    count = 0;				/* Counter for arcs. */
    for (p = tracePtr->head; p != NULL; p = p->next) {
	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	arcs[count].x = Round(p->x - r);
	arcs[count].y = Round(p->y - r);
	arcs[count].width = (unsigned short)s;
	arcs[count].height = (unsigned short)s;
	arcs[count].angle1 = 0;
	arcs[count].angle2 = 23040;
	count++;
	if (count >= numMax) {
	    if (penPtr->symbol.fillGC != NULL) {
		XFillArcs(graphPtr->display, drawable, penPtr->symbol.fillGC, 
			arcs, count);
	    }
	    if (penPtr->symbol.outlineWidth > 0) {
		XDrawArcs(graphPtr->display, drawable, penPtr->symbol.outlineGC,
			  arcs, count);
	    }
	    count = 0;
	}
    }
    if (count > 0) {
	if (penPtr->symbol.fillGC != NULL) {
	    XFillArcs(graphPtr->display, drawable, penPtr->symbol.fillGC, 
		      arcs, count);
	}
	if (penPtr->symbol.outlineWidth > 0) {
	    XDrawArcs(graphPtr->display, drawable, penPtr->symbol.outlineGC,
		      arcs, count);
	}
    }
    Blt_Free(arcs);
}

#endif

/* 
 * DrawSquareSymbols --
 *
 *	Draws the symbols of the trace as squares.  The outlines of squares
 *	are drawn after squares are filled.  This is speed tradeoff: draw
 *	many squares at once, or drawn one symbol at a time.
 *
 *	Symbols are only drawn at the knots of the trace (i.e. original points,
 *	not generated).  The "play" function can limit what squares are drawn.
 *
 */
static void
DrawSquareSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
		  ContourPen *penPtr, int size)
{
    XRectangle *rectangles;
    TracePoint *p;
    int r, s;
    size_t numMax, numReq, count;

    numReq = tracePtr->numPoints;
    numMax = MAX_DRAWRECTANGLES(graphPtr->display);
    if ((numMax == 0) || (numMax > numReq)) {
	numMax = numReq;
    } 
    rectangles = Blt_Malloc(numMax * sizeof(XRectangle));
    if (rectangles == NULL) {
	return;
    }

    r = (int)ceil(size * S_RATIO * 0.5);
    s = r + r;

    count = 0;				/* Counter for rectangles. */
    for (p = tracePtr->head; p != NULL; p = p->next) {
	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	rectangles[count].x = Round(p->x - r);
	rectangles[count].y = Round(p->y - r);
	rectangles[count].width = s;
	rectangles[count].height = s;
	count++;
	if (count >= numMax) {
	    if (penPtr->symbol.fillGC != NULL) {
		XFillRectangles(graphPtr->display, drawable, 
			penPtr->symbol.fillGC, rectangles, count);
	    }
	    if (penPtr->symbol.outlineWidth > 0) {
		XDrawRectangles(graphPtr->display, drawable, 
			penPtr->symbol.outlineGC, rectangles, count);
	    }
	    count = 0;
	}
    }
    if (count > 0) {
	if (penPtr->symbol.fillGC != NULL) {
	    XFillRectangles(graphPtr->display, drawable, penPtr->symbol.fillGC,
			    rectangles, count);
	}
	if (penPtr->symbol.outlineWidth > 0) {
	    XDrawRectangles(graphPtr->display, drawable, 
		penPtr->symbol.outlineGC, rectangles, count);
	}
    }
    Blt_Free(rectangles);
}

/* 
 * DrawSkinnyCrossPlusSymbols --
 *
 *	Draws the symbols of the trace as single line crosses or pluses.  
 *
 *	Symbols are only drawn at the knots of the trace (i.e. original points,
 *	not generated).  The "play" function can limit what symbols are drawn.
 */
static void
DrawSkinnyCrossPlusSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr,
			   ContourPen *penPtr, int size)
{
    TracePoint *p;
    XPoint pattern[4];			/* Template for polygon symbols */
    XSegment *segments;
    int r;
    size_t numMax, numReq, count;

    numReq = tracePtr->numPoints * 2;
    numMax = MAX_DRAWSEGMENTS(graphPtr->display);
    numMax &= ~0x1;
    if ((numMax == 0) || (numMax > numReq)) {
	numMax = numReq;
    } 
    segments = Blt_Malloc(numMax * sizeof(XSegment));
    if (segments == NULL) {
	return;
    }

    r = (int)ceil(size * 0.5);
    if (penPtr->symbol.type == SYMBOL_SCROSS) {
	r = Round((double)r * M_SQRT1_2);
	pattern[3].y = pattern[2].x = pattern[0].x = pattern[0].y = -r;
	pattern[3].x = pattern[2].y = pattern[1].y = pattern[1].x = r;
    } else {
	pattern[0].y = pattern[1].y = pattern[2].x = pattern[3].x = 0;
	pattern[0].x = pattern[2].y = -r;
	pattern[1].x = pattern[3].y = r;
    }

    count = 0;				/* Counter for segments. */
    for (p = tracePtr->head; p != NULL; p = p->next) {
	int rx, ry;

	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	rx = Round(p->x);
	ry = Round(p->y);
	segments[count].x1 = pattern[0].x + rx;
	segments[count].y1 = pattern[0].y + ry;
	segments[count].x2 = pattern[1].x + rx;
	segments[count].y2 = pattern[1].y + ry;
	count++;
	segments[count].x1 = pattern[2].x + rx;
	segments[count].y1 = pattern[2].y + ry;
	segments[count].x2 = pattern[3].x + rx;
	segments[count].y2 = pattern[3].y + ry;
	count++;
	if (count >= numMax) {
	    XDrawSegments(graphPtr->display, drawable, 	
		  penPtr->symbol.outlineGC, segments, count);
	    count = 0;
	}
    }
    if (count > 0) {
	XDrawSegments(graphPtr->display, drawable, penPtr->symbol.outlineGC, 
		      segments, count);
    }
    Blt_Free(segments);
}

static void
DrawCrossPlusSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
		     ContourPen *penPtr, int size)
{
    TracePoint *p;
    XPoint polygon[13];
    XPoint pattern[13];
    int r;
    int d;			/* Small delta for cross/plus
				 * thickness */
    
    r = (int)ceil(size * S_RATIO * 0.5);
    d = (r / 3);
    /*
     *
     *          2   3       The plus/cross symbol is a closed polygon
     *                      of 12 points. The diagram to the left
     *    0,12  1   4    5  represents the positions of the points
     *           x,y        which are computed below. The extra
     *     11  10   7    6  (thirteenth) point connects the first and
     *                      last points.
     *          9   8
     */
    pattern[0].x = pattern[11].x = pattern[12].x = -r;
    pattern[2].x = pattern[1].x = pattern[10].x = pattern[9].x = -d;
    pattern[3].x = pattern[4].x = pattern[7].x = pattern[8].x = d;
    pattern[5].x = pattern[6].x = r;
    pattern[2].y = pattern[3].y = -r;
    pattern[0].y = pattern[1].y = pattern[4].y = pattern[5].y =
	pattern[12].y = -d;
    pattern[11].y = pattern[10].y = pattern[7].y = pattern[6].y = d;
    pattern[9].y = pattern[8].y = r;
    
    if (penPtr->symbol.type == SYMBOL_CROSS) {
	int i;

	/* For the cross symbol, rotate the points by 45 degrees. */
	for (i = 0; i < 12; i++) {
	    double dx, dy;
	    
	    dx = (double)pattern[i].x * M_SQRT1_2;
	    dy = (double)pattern[i].y * M_SQRT1_2;
	    pattern[i].x = Round(dx - dy);
	    pattern[i].y = Round(dx + dy);
	}
	pattern[12] = pattern[0];
    }
    for (p = tracePtr->head; p != NULL; p = p->next) {
	int rx, ry;
	int i;

	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	rx = Round(p->x);
	ry = Round(p->y);
	for (i = 0; i < 13; i++) {
	    polygon[i].x = pattern[i].x + rx;
	    polygon[i].y = pattern[i].y + ry;
	}
	if (penPtr->symbol.fillGC != NULL) {
	    XFillPolygon(graphPtr->display, drawable, penPtr->symbol.fillGC, 
			 polygon, 13, Complex, CoordModeOrigin);
	}
	if (penPtr->symbol.outlineWidth > 0) {
	    XDrawLines(graphPtr->display, drawable, penPtr->symbol.outlineGC, 
			polygon, 13, CoordModeOrigin);
	}
    }
}

static void
DrawTriangleArrowSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
			 ContourPen *penPtr, int size)
{
    XPoint pattern[4];
    double b;
    int b2, h1, h2;
    TracePoint *p;

#define H_RATIO		1.1663402261671607
#define B_RATIO		1.3467736870885982
#define TAN30		0.57735026918962573
#define COS30		0.86602540378443871
    b = Round(size * B_RATIO * 0.7);
    b2 = Round(b * 0.5);
    h2 = Round(TAN30 * b2);
    h1 = Round(b2 / COS30);
    /*
     *
     *                      The triangle symbol is a closed polygon
     *           0,3         of 3 points. The diagram to the left
     *                      represents the positions of the points
     *           x,y        which are computed below. The extra
     *                      (fourth) point connects the first and
     *      2           1   last points.
     *
     */
    
    if (penPtr->symbol.type == SYMBOL_ARROW) {
	pattern[3].x = pattern[0].x = 0;
	pattern[3].y = pattern[0].y = h1;
	pattern[1].x = b2;
	pattern[2].y = pattern[1].y = -h2;
	pattern[2].x = -b2;
    } else {
	pattern[3].x = pattern[0].x = 0;
	pattern[3].y = pattern[0].y = -h1;
	pattern[1].x = b2;
	pattern[2].y = pattern[1].y = h2;
	pattern[2].x = -b2;
    }
    for (p = tracePtr->head; p != NULL; p = p->next) {
	XPoint polygon[4];
	int rx, ry;
	int i;

	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	rx = Round((double)p->x);
	ry = Round((double)p->y);
	for (i = 0; i < 4; i++) {
	    polygon[i].x = pattern[i].x + rx;
	    polygon[i].y = pattern[i].y + ry;
	}
	if (penPtr->symbol.fillGC != NULL) {
	    XFillPolygon(graphPtr->display, drawable, penPtr->symbol.fillGC, 
			 polygon, 4, Convex, CoordModeOrigin);
	}
	if (penPtr->symbol.outlineWidth > 0) {
	    XDrawLines(graphPtr->display, drawable, penPtr->symbol.outlineGC, 
		       polygon, 4, CoordModeOrigin);
	}
    }
}

static void
DrawDiamondSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
		   ContourPen *penPtr, int size)
{
    TracePoint *p;
    XPoint pattern[5];
    int r1;
    /*
     *
     *                      The diamond symbol is a closed polygon
     *            1         of 4 points. The diagram to the left
     *                      represents the positions of the points
     *       0,4 x,y  2     which are computed below. The extra
     *                      (fifth) point connects the first and
     *            3         last points.
     *
     */
    r1 = (int)ceil(size * 0.5);
    pattern[1].y = pattern[0].x = -r1;
    pattern[2].y = pattern[3].x = pattern[0].y = pattern[1].x = 0;
    pattern[3].y = pattern[2].x = r1;
    pattern[4] = pattern[0];
    
    for (p = tracePtr->head; p != NULL; p = p->next) {
	XPoint polygon[5];
	int rx, ry;
	int i;

	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	rx = Round((double)p->x);
	ry = Round((double)p->y);
	for (i = 0; i < 5; i++) {
	    polygon[i].x = pattern[i].x + rx;
	    polygon[i].y = pattern[i].y + ry;
	}
	if (penPtr->symbol.fillGC != NULL) {
	    XFillPolygon(graphPtr->display, drawable, penPtr->symbol.fillGC, 
		polygon, 5, Convex, CoordModeOrigin);
	}
	if (penPtr->symbol.outlineWidth > 0) {
	    XDrawLines(graphPtr->display, drawable, penPtr->symbol.outlineGC, 
		polygon, 5, CoordModeOrigin);
	}
    } 
}

static void
DrawImageSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
		 ContourPen *penPtr, int size)
{
    int w, h;
    int dx, dy;
    TracePoint *p;

    Tk_SizeOfImage(penPtr->symbol.image, &w, &h);
    dx = w / 2;
    dy = h / 2;
    for (p = tracePtr->head; p != NULL; p = p->next) {
	int x, y;

	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	x = Round((double)p->x) - dx;
	y = Round((double)p->y) - dy;
	Tk_RedrawImage(penPtr->symbol.image, 0, 0, w, h, drawable, x, y);
    }
}

static void
DrawBitmapSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
		  ContourPen *penPtr, int size)
{
    Pixmap bitmap, mask;
    int w, h, bw, bh;
    double scale, sx, sy;
    int dx, dy;
    TracePoint *p;

    Tk_SizeOfBitmap(graphPtr->display, penPtr->symbol.bitmap, &w, &h);
    mask = None;
    
    /*
     * Compute the size of the scaled bitmap.  Stretch the bitmap to fit
     * a nxn bounding box.
     */
    sx = (double)size / (double)w;
    sy = (double)size / (double)h;
    scale = MIN(sx, sy);
    bw = (int)(w * scale);
    bh = (int)(h * scale);
    
    XSetClipMask(graphPtr->display, penPtr->symbol.outlineGC, None);
    if (penPtr->symbol.mask != None) {
	mask = Blt_ScaleBitmap(graphPtr->tkwin, penPtr->symbol.mask,
			       w, h, bw, bh);
	XSetClipMask(graphPtr->display, penPtr->symbol.outlineGC, mask);
    }
    bitmap = Blt_ScaleBitmap(graphPtr->tkwin, penPtr->symbol.bitmap, w, h, bw, 
			     bh);
    if (penPtr->symbol.fillGC == NULL) {
	XSetClipMask(graphPtr->display, penPtr->symbol.outlineGC, bitmap);
    }
    dx = bw / 2;
    dy = bh / 2;
    for (p = tracePtr->head; p != NULL; p = p->next) {
	int x, y;

	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	x = Round((double)p->x) - dx;
	y = Round((double)p->y) - dy;
	if ((penPtr->symbol.fillGC == NULL) || (mask !=None)) {
	    XSetClipOrigin(graphPtr->display, penPtr->symbol.outlineGC, x, y);
	}
	XCopyPlane(graphPtr->display, bitmap, drawable, 
		   penPtr->symbol.outlineGC, 0, 0, bw, bh, x, y, 1);
    }
    Tk_FreePixmap(graphPtr->display, bitmap);
    if (mask != None) {
	Tk_FreePixmap(graphPtr->display, mask);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawSymbols --
 *
 * 	Draw the symbols centered at the each given x,y coordinate in the array
 * 	of points.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Draws a symbol at each coordinate given.  If active, only those
 *	coordinates which are currently active are drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawSymbols(Graph *graphPtr, Drawable drawable, Isoline *isoPtr, 
	    ContourPen *penPtr)
{
    int size;
    Blt_HashTable pointTable;
    TraceSegment *s;
    Trace trace, *tracePtr;
    TracePoint *p, *next;
    ContourElement *elemPtr;
    Region2d exts;
    XColor *colorPtr;

    colorPtr = NULL;
    if (penPtr->symbol.fillColor == COLOR_PALETTE) {
	if (colorPtr == NULL) {
	    XColor color;

	    color.red   = isoPtr->paletteColor.Red * 257;
	    color.green = isoPtr->paletteColor.Green * 257;
	    color.blue  = isoPtr->paletteColor.Blue * 257;
	    colorPtr = Tk_GetColorByValue(graphPtr->tkwin, &color);
	}
	/* Temporarily set the color from the interpolated value. */
	XSetForeground(graphPtr->display, penPtr->symbol.fillGC, 
		       colorPtr->pixel);
    }	
    if (penPtr->symbol.outlineColor == COLOR_PALETTE) {
	if (colorPtr == NULL) {
	    XColor color;

	    color.red   = isoPtr->paletteColor.Red * 257;
	    color.green = isoPtr->paletteColor.Green * 257;
	    color.blue  = isoPtr->paletteColor.Blue * 257;
	    colorPtr = Tk_GetColorByValue(graphPtr->tkwin, &color);
	}
	/* Temporarily set the color from the interpolated value. */
	XSetForeground(graphPtr->display, penPtr->symbol.outlineGC, 
		       colorPtr->pixel);
    }	
    elemPtr = isoPtr->elemPtr;
    memset(&trace, 0, sizeof(trace));
    tracePtr = &trace;
    Blt_InitHashTable(&pointTable, sizeof(PointKey) / sizeof(int));
    Blt_GraphExtents(elemPtr, &exts);
    for (s = isoPtr->segments; s != NULL; s = s->next) {
	PointKey key;
	Blt_HashEntry *hPtr;
	int isNew;

	if ((s->flags & CLIP_LEFT) == 0) {
	    MakePointKey(&key, s->x1, s->y1);
	    hPtr = Blt_CreateHashEntry(&pointTable, &key, &isNew);
	    assert(hPtr != NULL);
	    if ((isNew) && (PointInRegion(&exts, s->x1, s->y1))) {
		p = NewPoint(elemPtr, s->x1, s->y1, pointTable.numEntries);
		AppendPoint(&trace, p);
		p->flags |= SYMBOL;
	    }
	}
	if ((s->flags & CLIP_RIGHT) == 0) {
	    MakePointKey(&key, s->x2, s->y2);
	    hPtr = Blt_CreateHashEntry(&pointTable, &key, &isNew);
	    assert(hPtr != NULL);
	    if ((isNew) && (PointInRegion(&exts, s->x2, s->y2))) {
		p = NewPoint(elemPtr, s->x2, s->y2, pointTable.numEntries);
		AppendPoint(&trace, p);
		p->flags |= SYMBOL;
	    }
	}
    }
    Blt_DeleteHashTable(&pointTable);
    tracePtr->drawFlags |= SYMBOL;
    tracePtr->elemPtr = elemPtr;

    size = penPtr->symbol.size;
    if (size < 3) {
	if (penPtr->symbol.fillGC != NULL) {
	    DrawPointSymbols(graphPtr, drawable, tracePtr, penPtr);
	}
	goto done;
    }
    switch (penPtr->symbol.type) {
    case SYMBOL_NONE:
	break;
	
    case SYMBOL_SQUARE:
	DrawSquareSymbols(graphPtr, drawable, tracePtr, penPtr, size);
	break;
	
    case SYMBOL_CIRCLE:
	DrawCircleSymbols(graphPtr, drawable, tracePtr, penPtr, size);
	break;
	
    case SYMBOL_SPLUS:
    case SYMBOL_SCROSS:
	DrawSkinnyCrossPlusSymbols(graphPtr, drawable, tracePtr, penPtr, size);
	break;
	
    case SYMBOL_PLUS:
    case SYMBOL_CROSS:
	DrawCrossPlusSymbols(graphPtr, drawable, tracePtr, penPtr, size);
	break;
	
    case SYMBOL_DIAMOND:
	DrawDiamondSymbols(graphPtr, drawable, tracePtr, penPtr, size);
	break;
	
    case SYMBOL_TRIANGLE:
    case SYMBOL_ARROW:
	DrawTriangleArrowSymbols(graphPtr, drawable, tracePtr, penPtr, size);
	break;
	
    case SYMBOL_IMAGE:
	DrawImageSymbols(graphPtr, drawable, tracePtr, penPtr, size);
	break;
	
    case SYMBOL_BITMAP:
	DrawBitmapSymbols(graphPtr, drawable, tracePtr, penPtr, size);
	break;
    }
 done:
    if (colorPtr != NULL) {
	unsigned long color;

	Tk_FreeColor(colorPtr);
	color = BlackPixel(graphPtr->display, Tk_ScreenNumber(graphPtr->tkwin));
	if (penPtr->symbol.fillColor == COLOR_PALETTE) {
	    XSetForeground(graphPtr->display, penPtr->symbol.fillGC, color);
	}
	if (penPtr->symbol.outlineColor == COLOR_PALETTE) {
	    XSetForeground(graphPtr->display, penPtr->symbol.outlineGC, color);
	}
    }
    tracePtr->drawFlags &= ~(KNOT | VISIBLE | SYMBOL);
    for (p = tracePtr->head; p != NULL; p = next) {
	next = p->next;
	Blt_Pool_FreeItem(elemPtr->pointPool, p);
    }
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * DrawValues --
 *
 * 	Draws the numeric value of the bar.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawValues(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
	ContourPen *penPtr)
{
    TracePoint *p;
    Point2d *vertices;
    const char *fmt;
    
    fmt = penPtr->valueFormat;
    if (fmt == NULL) {
	fmt = "%g";
    }
    points = tracePtr->elemPtr->vertices;
    for (p = tracePtr->head; p != NULL; p = p->next) {
	double x, y;
	char string[200];

	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	x = vertices[p->index].x;
	y = vertices[p->index].y;
	if (penPtr->valueFlags == SHOW_X) {
	    Blt_FormatString(string, TCL_DOUBLE_SPACE, fmt, x); 
	} else if (penPtr->valueFlags == SHOW_Y) {
	    Blt_FormatString(string, TCL_DOUBLE_SPACE, fmt, y); 
	} else if (penPtr->valueFlags == SHOW_BOTH) {
	    Blt_FormatString(string, TCL_DOUBLE_SPACE, fmt, x);
	    strcat(string, ",");
	    Blt_FormatString(string + strlen(string), TCL_DOUBLE_SPACE, fmt, y);
	}
	Blt_DrawText(graphPtr->tkwin, drawable, string, 
	     &penPtr->valueStyle, Round(p->x), Round(p->y));
    }
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * DrawIsoline --
 *
 * 	Draws each isolines as one or more polylines.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawIsoline(Graph *graphPtr, Drawable drawable, ContourElement *elemPtr,
	    Isoline *isoPtr, ContourPen *penPtr)
{
    XSegment *segments;
    TraceSegment *s;
    XColor *colorPtr;
    size_t numMax, numReq, count;

#ifdef notdef
    fprintf(stderr, "DrawIsoline isoline=%s #segments=%d value=%g, reqValue=%g\n", 
	    isoPtr->obj.name, isoPtr->numSegments, isoPtr->value, 
	    isoPtr->reqValue);
    fprintf(stderr, "DrawIsoline isoline=%s #segments=%d value=%g, reqValue=%g penPtr=%x, elemPtr->builtinPenPtr=%x\n", 
	    isoPtr->obj.name, isoPtr->numSegments, isoPtr->value, 
	    isoPtr->reqValue, penPtr, elemPtr->builtinPenPtr);
#endif
    numReq = isoPtr->numSegments;
    numMax = MAX_DRAWSEGMENTS(graphPtr->display);
    if ((numMax == 0) || (numMax > numReq)) {
	numMax = numReq;
    } 
    segments = Blt_Malloc(numMax * sizeof(XSegment));
    if (segments == NULL) {
	return;
    }

    colorPtr = NULL;
    if (penPtr->traceColor == COLOR_PALETTE) {
	if (colorPtr == NULL) {
	    XColor color;

	    color.red   = isoPtr->paletteColor.Red * 257;
	    color.green = isoPtr->paletteColor.Green * 257;
	    color.blue  = isoPtr->paletteColor.Blue * 257;
	    colorPtr = Tk_GetColorByValue(graphPtr->tkwin, &color);
	}
	/* Temporarily set the color from the interpolated value. */
	XSetForeground(graphPtr->display, penPtr->traceGC, colorPtr->pixel);
    }
    count = 0;				/* Counter for segments */
    for (s = isoPtr->segments; s != NULL; s = s->next) {
	segments[count].x1 = (short int)Round(s->x1);
	segments[count].y1 = (short int)Round(s->y1);
	segments[count].x2 = (short int)Round(s->x2);
	segments[count].y2 = (short int)Round(s->y2);
	count++;
	if (count >= numMax) {
	    XDrawSegments(graphPtr->display, drawable, penPtr->traceGC, 
			  segments, count);
	    count = 0;
	}
    }
    if (count > 0) {
	XDrawSegments(graphPtr->display, drawable, penPtr->traceGC, 
		      segments, count);
    }
    Blt_Free(segments);
    if (colorPtr != NULL) {
	unsigned long color;

	Tk_FreeColor(colorPtr);
	color = BlackPixel(graphPtr->display, Tk_ScreenNumber(graphPtr->tkwin));
	if (penPtr->traceColor == COLOR_PALETTE) {
	    XSetForeground(graphPtr->display, penPtr->traceGC, color);
	}
    }
}

/* Closest isoline procedures. */

static double
DistanceToLine(
    int x, int y,			/* Sample X-Y coordinate. */
    Point2d *p, Point2d *q,		/* End points of the line segment. */
    Point2d *t)				/* (out) Point on line segment. */
{
    double right, left, top, bottom;

    *t = Blt_GetProjection(x, y, p, q);
    if (p->x > q->x) {
	right = p->x, left = q->x;
    } else {
	left = p->x, right = q->x;
    }
    if (p->y > q->y) {
	bottom = p->y, top = q->y;
    } else {
	top = p->y, bottom = q->y;
    }
    if (t->x > right) {
	t->x = right;
    } else if (t->x < left) {
	t->x = left;
    }
    if (t->y > bottom) {
	t->y = bottom;
    } else if (t->y < top) {
	t->y = top;
    }
    return hypot((t->x - x), (t->y - y));
}

static void
ClosestPoint(ContourElement *elemPtr, ClosestSearch *s)
{
    double closestDistance;
    int i, closestIndex;

    closestDistance = s->dist;
    closestIndex = 0;
    for (i = 0; i < elemPtr->numVertices; i++) {
	Vertex *v;
	double d;

	v = elemPtr->vertices + i;
	if ((v->flags & VISIBLE) == 0) {
	    continue;
	}
	d = hypot(v->x - s->x, v->y - s->y);
	if (d < closestDistance) {
	    closestIndex = v->index;
	    closestDistance = d;
	}
    }
    if (closestDistance < s->dist) {
	s->item    = elemPtr;
	s->dist    = closestDistance;
	s->index   = closestIndex;
	s->point.x = elemPtr->vertices[closestIndex].x;
	s->point.y = elemPtr->vertices[closestIndex].y;
    }
}

static void
ClosestSegment(ContourElement *elemPtr, ClosestSearch *s)
{
    Point2d closestPoint;
    double closestDistance;
    int closestIndex;
    int i;

    closestIndex = -1;			/* Suppress compiler warning. */
    closestDistance = s->dist;
    closestPoint.x = closestPoint.y = 0; /* Suppress compiler warning. */
    for (i = 0; i < elemPtr->numTriangles; i++) {
	Triangle *t;
	double d;
	Point2d p1, p2, b;

	t = elemPtr->triangles + i;
	/* Compare AB */
	p1.x = Ax, p1.y = Ay;
	p2.x = Bx, p2.y = By;
	d = DistanceToLine(s->x, s->y, &p1, &p2, &b);
	if (d < closestDistance) {
	    closestPoint    = b;
	    closestIndex    = t->a;
	    closestDistance = d;
	}
	/* Compare BC */
	p1.x = Bx, p1.y = By;
	p2.x = Cx, p2.y = Cy;
	d = DistanceToLine(s->x, s->y, &p1, &p2, &b);
	if (d < closestDistance) {
	    closestPoint    = b;
	    closestIndex    = t->b;
	    closestDistance = d;
	}
	/* Compare CA */
	p1.x = Cx, p1.y = Cy;
	p2.x = Ax, p2.y = Ay;
	d = DistanceToLine(s->x, s->y, &p1, &p2, &b);
	if (d < closestDistance) {
	    closestPoint    = b;
	    closestIndex    = t->c;
	    closestDistance = d;
	}
    }	
    if (closestDistance < s->dist) {
	s->dist = closestDistance;
	s->item	 = (Element *)elemPtr;
	s->index = closestIndex;
	s->point = Blt_InvMap2D(elemPtr->obj.graphPtr, closestPoint.x, 
		closestPoint.y, &elemPtr->axes);
    }
}

/* Contour pen procedures.  */

static void
InitPen(ContourPen *penPtr)
{
    Blt_Ts_InitStyle(penPtr->valueStyle);
    penPtr->configProc = ConfigurePenProc;
    penPtr->configSpecs = penSpecs;
    penPtr->destroyProc = DestroyPenProc;
    penPtr->flags = NORMAL_PEN;
    penPtr->symbol.bitmap = penPtr->symbol.mask = None;
    penPtr->symbol.outlineColor = penPtr->symbol.fillColor = COLOR_DEFAULT;
    penPtr->symbol.outlineWidth = penPtr->traceWidth = 1;
    penPtr->symbol.type = SYMBOL_CIRCLE;
    penPtr->valueFlags = SHOW_NONE;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigurePenProc --
 *
 *	Sets up the appropriate configuration parameters in the GC.  It is
 *	assumed the parameters have been previously set by a call to
 *	Blt_ConfigureWidget.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is returned,
 *	then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information such as line width, line style, color
 *	etc. get set in a new GC.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ConfigurePenProc(Graph *graphPtr, Pen *basePtr)
{
    ContourPen *penPtr = (ContourPen *)basePtr;
    unsigned long gcMask;
    GC newGC;
    XGCValues gcValues;
    XColor *colorPtr;
    unsigned long defColor, color;

    if (penPtr->traceColor == COLOR_PALETTE) {
	defColor = BlackPixel(graphPtr->display, 
			      Tk_ScreenNumber(graphPtr->tkwin));
    } else {
	/* Trace color can't be NULL or "defcolor".  */
	defColor = penPtr->traceColor->pixel;
    }
    /*
     * Set the outline GC for this pen: GCForeground is outline color.
     * GCBackground is the fill color (only used for bitmap symbols).
     */
    gcMask = (GCLineWidth | GCForeground);
    colorPtr = penPtr->symbol.outlineColor;
    color = (ISALIASED(colorPtr)) ? defColor : colorPtr->pixel;
    gcValues.foreground = color;
    if (penPtr->symbol.type == SYMBOL_BITMAP) {
	/*
	 * Set a clip mask if either
	 *	1) no background color was designated or
	 *	2) a masking bitmap was specified.
	 *
	 * These aren't necessarily the bitmaps we'll be using for
	 * clipping. But this makes it unlikely that anyone else will be
	 * sharing this GC when we set the clip origin (at the time the bitmap
	 * is drawn).
	 */
	colorPtr = penPtr->symbol.fillColor;
	if (colorPtr != NULL) {
	    color = (ISALIASED(colorPtr)) ? defColor : colorPtr->pixel;
	    gcValues.background = color;
	    gcMask |= GCBackground;
	    if (penPtr->symbol.mask != None) {
		gcValues.clip_mask = penPtr->symbol.mask;
		gcMask |= GCClipMask;
	    }
	} else {
	    gcValues.clip_mask = penPtr->symbol.bitmap;
	    gcMask |= GCClipMask;
	}
    }
    gcValues.line_width = LineWidth(penPtr->symbol.outlineWidth);
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (penPtr->symbol.outlineGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, penPtr->symbol.outlineGC);
    }
    penPtr->symbol.outlineGC = newGC;

    /* Fill GC for symbols: GCForeground is fill color */

    gcMask = (GCLineWidth | GCForeground);
    colorPtr = penPtr->symbol.fillColor;
    newGC = NULL;
    if (colorPtr != NULL) {
	color = (ISALIASED(colorPtr)) ? defColor : colorPtr->pixel;
	gcValues.foreground = color;
	newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (penPtr->symbol.fillGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, penPtr->symbol.fillGC);
    }
    penPtr->symbol.fillGC = newGC;

    /* Line segments */

    gcMask = (GCLineWidth | GCForeground | GCLineStyle | GCCapStyle |
	GCJoinStyle);
    gcValues.cap_style = CapButt;
    gcValues.join_style = JoinRound;
    gcValues.line_style = LineSolid;
    gcValues.line_width = LineWidth(penPtr->traceWidth);

    colorPtr = penPtr->traceOffColor;
    if (colorPtr != NULL) {
	color = (ISALIASED(colorPtr)) ? defColor : colorPtr->pixel;
	gcMask |= GCBackground;
	gcValues.background = color;
    }
    colorPtr = penPtr->traceColor;
    color = (ISALIASED(colorPtr)) ? defColor : colorPtr->pixel;
    gcValues.foreground = color;
    if (LineIsDashed(penPtr->traceDashes)) {
	gcValues.line_width = penPtr->traceWidth;
	gcValues.line_style = (colorPtr == NULL) ? 
	    LineOnOffDash : LineDoubleDash;
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(penPtr->traceDashes)) {
	penPtr->traceDashes.offset = penPtr->traceDashes.values[0] / 2;
	Blt_SetDashes(graphPtr->display, newGC, &penPtr->traceDashes);
    }
    if (penPtr->traceGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, penPtr->traceGC);
    }
    penPtr->traceGC = newGC;

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyPenProc --
 *
 *	Release memory and resources allocated for the style.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the pen style is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyPenProc(Graph *graphPtr, Pen *basePtr)
{
    ContourPen *penPtr = (ContourPen *)basePtr;

    Blt_Ts_FreeStyle(graphPtr->display, &penPtr->valueStyle);
    if (penPtr->symbol.outlineGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, penPtr->symbol.outlineGC);
    }
    if (penPtr->symbol.fillGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, penPtr->symbol.fillGC);
    }
    if (penPtr->traceGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, penPtr->traceGC);
    }
    if (penPtr->symbol.bitmap != None) {
	Tk_FreeBitmap(graphPtr->display, penPtr->symbol.bitmap);
	penPtr->symbol.bitmap = None;
    }
    if (penPtr->symbol.mask != None) {
	Tk_FreeBitmap(graphPtr->display, penPtr->symbol.mask);
	penPtr->symbol.mask = None;
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * ConfigureBarProc --
 *
 *	Sets up the appropriate configuration parameters in the GC.  It is
 *	assumed the parameters have been previously set by a call to
 *	Blt_ConfigureWidget.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is returned,
 *	then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information such as bar foreground/background color and
 *	stipple etc. get set in a new GC.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ConfigureProc(Graph *graphPtr, Element *basePtr)
{
    ContourElement *elemPtr = (ContourElement *)basePtr;
    unsigned long gcMask;
    GC newGC;
    XGCValues gcValues;

    if (ConfigurePenProc(graphPtr, (Pen *)elemPtr->builtinPenPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_ConfigModified(elemPtr->configSpecs, "-display*", "-*data",
	    "-map*", "-label", "-hide", "-z", "-mesh", (char *)NULL)) {
	elemPtr->flags |= MAP_ITEM;
    }
    /* Line segments */

    gcMask = (GCLineWidth | GCForeground | GCLineStyle | GCCapStyle |
	GCJoinStyle);
    gcValues.cap_style  = CapButt;
    gcValues.join_style = JoinRound;
    gcValues.line_style = LineSolid;
    gcValues.line_width = LineWidth(elemPtr->meshWidth);
    gcValues.background = gcValues.foreground = elemPtr->meshColor->pixel;
    if (elemPtr->meshOffColor != NULL) {
	gcValues.background = elemPtr->meshOffColor->pixel;
    }
    if (LineIsDashed(elemPtr->meshDashes)) {
	gcValues.line_style = (elemPtr->meshOffColor != NULL) ?
	    LineDoubleDash : LineOnOffDash;
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(elemPtr->meshDashes)) {
	elemPtr->meshDashes.offset = elemPtr->meshDashes.values[0] / 2;
	Blt_SetDashes(graphPtr->display, newGC, &elemPtr->meshDashes);
    }
    if (elemPtr->meshGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, elemPtr->meshGC);
    }
    elemPtr->meshGC = newGC;
    if (Blt_ConfigModified(elemPtr->configSpecs, "-values", "-mesh", 
		(char *)NULL)) {
	graphPtr->flags |= RESET_WORLD;
	elemPtr->flags |= MAP_ITEM;
    }
    return TCL_OK;
}

static void
ExtentsProc(Element *basePtr)
{
    ContourElement *elemPtr = (ContourElement *)basePtr;
    Mesh *meshPtr;
	
    meshPtr = elemPtr->meshPtr;
    if ((meshPtr == NULL) || (elemPtr->z.numValues == 0)) {
	return;				/* No mesh or values configured. */
    }
    if (meshPtr->numVertices < 3) {
	return;
    }
    if (meshPtr->xMin < elemPtr->axes.x->valueRange.min) {
	elemPtr->axes.x->valueRange.min = meshPtr->xMin;
    }
    if (meshPtr->xMax > elemPtr->axes.x->valueRange.max) {
	elemPtr->axes.x->valueRange.max = meshPtr->xMax;
    }
    if (meshPtr->yMin < elemPtr->axes.y->valueRange.min) {
	elemPtr->axes.y->valueRange.min = meshPtr->yMin;
    }
    if (meshPtr->yMax > elemPtr->axes.y->valueRange.max) {
	elemPtr->axes.y->valueRange.max = meshPtr->yMax;
    }
    if (elemPtr->z.min < elemPtr->zAxisPtr->valueRange.min) {
	elemPtr->zAxisPtr->valueRange.min = elemPtr->z.min;
    } 
    if (elemPtr->z.max > elemPtr->zAxisPtr->valueRange.max) {
	elemPtr->zAxisPtr->valueRange.max = elemPtr->z.max;
    } 
}

/*
 *---------------------------------------------------------------------------
 *
 * ClosestProc --
 *
 *	Find the closest mesh vertex to the specified screen coordinates.
 *
 * Results:
 *	None.
 *
 * Side Effects:  
 *	The search structure will be willed with the information of the
 *	closest point.
 *
 *---------------------------------------------------------------------------
 */
static void
ClosestProc(Graph *graphPtr, Element *basePtr, ClosestSearch *searchPtr)
{
    ContourElement *elemPtr = (ContourElement *)basePtr;
    int mode;

    mode = searchPtr->mode;
    if (mode == SEARCH_POINTS) {
	ClosestPoint(elemPtr, searchPtr);
    } else {
	ClosestSegment(elemPtr, searchPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyProc --
 *
 *	Release memory and resources allocated for the bar element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the bar element is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyProc(Graph *graphPtr, Element *basePtr)
{
    ContourElement *elemPtr = (ContourElement *)basePtr;

    DestroyPenProc(graphPtr, (Pen *)elemPtr->builtinPenPtr);
    if (elemPtr->activePenPtr != NULL) {
	Blt_FreePen((Pen *)elemPtr->activePenPtr);
    }
    ResetElement(elemPtr);
    DestroyIsoTags(elemPtr);
    DestroyIsolines(elemPtr);
    if (elemPtr->meshGC != NULL) {
	Blt_FreePrivateGC(graphPtr->display, elemPtr->meshGC);
    }
    if (elemPtr->meshPtr != NULL) {
	Blt_DeleteMeshNotifier(elemPtr->meshPtr, elemPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapProc --
 *
 *	Calculates the actual screen coordinates of the contour element.  The
 *	screen coordinates are saved in the contour element structure.
 *
 * Results:
 *	None.
 *
 * Notes:
 *	A bar can have multiple segments (more than one x,y pairs).  In this
 *	case, the bar can be represented as either a set of non-contiguous
 *	bars or a single multi-segmented (stacked) bar.
 *
 *	The x-axis layout for a contour plot may be presented in one of two
 *	ways.  If abscissas are used, the bars are placed at those
 *	coordinates.  Otherwise, the range will represent the number of
 *	values.
 *
 *---------------------------------------------------------------------------
 */
static void
MapProc(Graph *graphPtr, Element *basePtr)
{
    ContourElement *elemPtr = (ContourElement *)basePtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Interp *interp;

    interp = elemPtr->obj.graphPtr->interp;
    ResetElement(elemPtr);
    elemPtr->pointPool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    elemPtr->segmentPool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);
    if (elemPtr->meshPtr == NULL) {
	return;
    }
    if (elemPtr->z.numValues != elemPtr->meshPtr->numVertices) {
	char mesg[500];

	sprintf(mesg, "# of mesh (%d) and field points (%d) disagree.",
		elemPtr->meshPtr->numVertices, elemPtr->z.numValues);
	Tcl_AppendResult(interp, mesg, (char *)NULL);
	Tcl_BackgroundError(interp);
	return;				/* Wrong # of field points */
    }
    GetScreenPoints(elemPtr);
    MapMesh(elemPtr);
#ifdef notdef
    MapActiveTriangles(elemPtr);
#endif
    if (elemPtr->colormapPtr != NULL) {
	Blt_Colormap_Init(elemPtr->colormapPtr);
    }
    /* Map contour isolines. */
    for (hPtr = Blt_FirstHashEntry(&elemPtr->isoTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Isoline *isoPtr;

	isoPtr = Blt_GetHashValue(hPtr);
	MapIsoline(isoPtr);
    }
    /* Map auxillary legend. */
    ComputeColorBarGeometry(graphPtr, elemPtr, 100, 100);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawProc --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	X drawing commands are output.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawProc(Graph *graphPtr, Drawable drawable, Element *basePtr)
{
    ContourElement *elemPtr = (ContourElement *)basePtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    DrawMesh(graphPtr, drawable, elemPtr);
    for (hPtr = Blt_FirstHashEntry(&elemPtr->isoTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Isoline *isoPtr;

	isoPtr = Blt_GetHashValue(hPtr);
	if (isoPtr->flags & HIDE) {
	    continue;			/* Don't draw this isoline. */
	}
	if (elemPtr->flags & ISOLINES) {
	    DrawIsoline(graphPtr, drawable, elemPtr, isoPtr, isoPtr->penPtr);
	}
	if (elemPtr->flags & SYMBOLS) {
	    DrawSymbols(graphPtr, drawable, isoPtr, isoPtr->penPtr);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawActiveProc --
 *
 *	Draws contours representing the active segments of the bar element.
 *	If the -relief option is set (other than "flat") and the borderwidth
 *	is greater than 0, a 3D border is drawn around the each bar segment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	X drawing commands are output.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawActiveProc(Graph *graphPtr, Drawable drawable, Element *basePtr)
{
    ContourElement *elemPtr = (ContourElement *)basePtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    DrawMesh(graphPtr, drawable, elemPtr);
    for (hPtr = Blt_FirstHashEntry(&elemPtr->isoTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Isoline *isoPtr;

	isoPtr = Blt_GetHashValue(hPtr);
	if ((isoPtr->flags & ACTIVE) == 0) {
	    continue;			/* Only draw active isolines. */
	}
	if (elemPtr->flags & ISOLINES) {
	    DrawIsoline(graphPtr, drawable, elemPtr, isoPtr, 
			isoPtr->activePenPtr);
	}
	if (elemPtr->flags & SYMBOLS) {
	    DrawSymbols(graphPtr, drawable, isoPtr, isoPtr->activePenPtr);
	}
    }
}

/* PostScript generation procedures. */

/*
 *---------------------------------------------------------------------------
 *
 * GetSymbolPostScriptInfo --
 *
 *	Set up the PostScript environment with the macros and attributes needed
 *	to draw the symbols of the element.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
GetSymbolPostScriptInfo(Blt_Ps ps, ContourElement *elemPtr, ContourPen *penPtr, 
			int size)
{
    XColor *outlineColor, *fillColor, *defaultColor;

    /* Set line and foreground attributes */
    outlineColor = penPtr->symbol.outlineColor;
    fillColor    = penPtr->symbol.fillColor;
    defaultColor = penPtr->traceColor;

    if (fillColor == COLOR_DEFAULT) {
	fillColor = defaultColor;
    }
    if (outlineColor == COLOR_DEFAULT) {
	outlineColor = defaultColor;
    }
    if (penPtr->symbol.type == SYMBOL_NONE) {
	Blt_Ps_XSetLineAttributes(ps, defaultColor, penPtr->traceWidth + 2,
		 &penPtr->traceDashes, CapButt, JoinMiter);
    } else {
	Blt_Ps_XSetLineWidth(ps, penPtr->symbol.outlineWidth);
	Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
    }

    /*
     * Build a PostScript procedure to draw the symbols.  For bitmaps, paint
     * both the bitmap and its mask. Otherwise fill and stroke the path formed
     * already.
     */
    Blt_Ps_Append(ps, "\n/DrawSymbolProc {\n");
    switch (penPtr->symbol.type) {
    case SYMBOL_NONE:
	break;				/* Do nothing */
    case SYMBOL_BITMAP:
	{
	    int w, h;
	    double sx, sy, scale;
	    Graph *graphPtr = elemPtr->obj.graphPtr;

	    /*
	     * Compute how much to scale the bitmap.  Don't let the scaled
	     * bitmap exceed the bounding square for the symbol.
	     */
	    Tk_SizeOfBitmap(graphPtr->display, penPtr->symbol.bitmap, &w, &h);
	    sx = (double)size / (double)w;
	    sy = (double)size / (double)h;
	    scale = MIN(sx, sy);

	    if ((penPtr->symbol.mask != None) && (fillColor != NULL)) {
		Blt_Ps_VarAppend(ps, "\n  % Bitmap mask is \"",
		    Tk_NameOfBitmap(graphPtr->display, penPtr->symbol.mask),
		    "\"\n\n  ", (char *)NULL);
		Blt_Ps_XSetBackground(ps, fillColor);
		Blt_Ps_DrawBitmap(ps, graphPtr->display, penPtr->symbol.mask, 
			scale, scale);
	    }
	    Blt_Ps_VarAppend(ps, "\n  % Bitmap symbol is \"",
		Tk_NameOfBitmap(graphPtr->display, penPtr->symbol.bitmap),
		"\"\n\n  ", (char *)NULL);
	    Blt_Ps_XSetForeground(ps, outlineColor);
	    Blt_Ps_DrawBitmap(ps, graphPtr->display, penPtr->symbol.bitmap, 
		scale, scale);
	}
	break;
    default:
	if (fillColor != NULL) {
	    Blt_Ps_Append(ps, "  ");
	    Blt_Ps_XSetBackground(ps, fillColor);
	    Blt_Ps_Append(ps, "  gsave fill grestore\n");
	}
	if ((outlineColor != NULL) && (penPtr->symbol.outlineWidth > 0)) {
	    Blt_Ps_Append(ps, "  ");
	    Blt_Ps_XSetForeground(ps, outlineColor);
	    Blt_Ps_Append(ps, "  stroke\n");
	}
	break;
    }
    Blt_Ps_Append(ps, "} def\n\n");
}

static void
SetLineAttributes(Blt_Ps ps, ContourPen *penPtr)
{
    /* Set the attributes of the line (color, dashes, linewidth) */
    Blt_Ps_XSetLineAttributes(ps, penPtr->traceColor,
	penPtr->traceWidth, &penPtr->traceDashes, CapButt, JoinMiter);
    if ((LineIsDashed(penPtr->traceDashes)) && 
	(penPtr->traceOffColor != NULL)) {
	Blt_Ps_Append(ps, "/DashesProc {\n  gsave\n    ");
	Blt_Ps_XSetBackground(ps, penPtr->traceOffColor);
	Blt_Ps_Append(ps, "    ");
	Blt_Ps_XSetDashes(ps, (Blt_Dashes *)NULL);
	Blt_Ps_Append(ps, "stroke\n  grestore\n} def\n");
    } else {
	Blt_Ps_Append(ps, "/DashesProc {} def\n");
    }
}

static void
PolylineToPostScript(Blt_Ps ps, Trace *tracePtr, ContourPen *penPtr)
{
    Point2d *points;
    TracePoint *p;
    int count;

    SetLineAttributes(ps, penPtr);
    points = Blt_AssertMalloc(tracePtr->numPoints * sizeof(Point2d));
    count = 0;
    for (p = tracePtr->head; p != NULL; p = p->next) {
	points[count].x = p->x;
	points[count].y = p->y;
	count++;
    }
    Blt_Ps_Append(ps, "% start trace\n");
    Blt_Ps_DrawPolyline(ps, count, points);
    Blt_Ps_Append(ps, "% end trace\n");
    Blt_Free(points);
}


/*
 *---------------------------------------------------------------------------
 *
 * DrawHull --
 *
 * 	Draws the convex hull representing the boundary of the mesh.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
HullToPostScript(Graph *graphPtr, Blt_Ps ps, ContourElement *elemPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
	 link = Blt_Chain_NextLink(link)) {
	Trace *tracePtr;
	
	tracePtr = Blt_Chain_GetValue(link);
	PolylineToPostScript(ps, tracePtr, elemPtr->boundaryPenPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EdgesToPostScript --
 *
 * 	Draws the segments forming of the mesh grid.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
EdgesToPostScript(Graph *graphPtr, Blt_Ps ps, ContourElement *elemPtr,
		  ContourPen *penPtr)
{
    Blt_Ps_XSetLineAttributes(ps, elemPtr->meshColor, elemPtr->meshWidth + 2,
		 &elemPtr->meshDashes, CapButt, JoinRound);
    Blt_Ps_DrawSegments2d(ps, elemPtr->numWires, elemPtr->wires);
}

/*
 *---------------------------------------------------------------------------
 *
 * TrianglesToPostScript --
 *
 * 	Draws the triangles forming of the colormap of the mesh.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
TrianglesToPostScript(Graph *graphPtr, Blt_Ps ps, ContourElement *elemPtr,
	      ContourPen *penPtr)
{
    Region2d exts;
    Triangle *t, *tend;
    int x, y, w, h;

    Blt_GraphExtents(elemPtr, &exts);
    w = (exts.right - exts.left) + 1;
    h = (exts.bottom - exts.top) + 1;
    if (elemPtr->picture != NULL) {
	Blt_FreePicture(elemPtr->picture);
    }
    elemPtr->picture = Blt_CreatePicture(w, h);
    Blt_BlankPicture(elemPtr->picture, 0x0);
    x = exts.left, y = exts.top;
    for (t = elemPtr->triangles, tend = t + elemPtr->numTriangles; 
	 t < tend; t++) {
	DrawTriangle(elemPtr, elemPtr->picture, t, x, y);
    }
    /* Create a clip path from the hull and draw the picture */
    Blt_Ps_DrawPicture(ps, elemPtr->picture, exts.left, exts.top);
}

/*
 *---------------------------------------------------------------------------
 *
 * MeshToPostScript --
 *
 * 	Draws the mesh of contour.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
MeshToPostScript(Graph *graphPtr, Blt_Ps ps, ContourElement *elemPtr)
{
    ContourPen *penPtr;

    penPtr = PEN(elemPtr);
    if (elemPtr->flags & COLORMAP) {
	TrianglesToPostScript(graphPtr, ps, elemPtr, penPtr);
    }
    if (elemPtr->flags & EDGES) {
	EdgesToPostScript(graphPtr, ps, elemPtr, penPtr);
    }
    if (elemPtr->flags & HULL) {
	HullToPostScript(graphPtr, ps, elemPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SymbolToPostScriptProc --
 *
 * 	Draw a symbol centered at the given x,y window coordinate based upon
 * 	the element symbol type and size.
 *
 * Results:
 *	None.
 *
 * Problems:
 *	Most notable is the round-off errors generated when calculating the
 *	centered position of the symbol.
 *
 *---------------------------------------------------------------------------
 */
static void
SymbolToPostScriptProc(
    Graph *graphPtr,			/* Graph widget record */
    Blt_Ps ps,
    Element *basePtr,			/* Line element information */
    double x, double y,			/* Center position of symbol */
    int size)				/* Size of element */
{
    ContourElement *elemPtr = (ContourElement *)basePtr;
    ContourPen *penPtr;
    double symbolSize;
    static const char *symbolMacros[] =
    {
	"Li", "Sq", "Ci", "Di", "Pl", "Cr", "Sp", "Sc", "Tr", "Ar", "Bm", 
	(char *)NULL,
    };

    penPtr = PEN(elemPtr);
    GetSymbolPostScriptInfo(ps, elemPtr, penPtr, size);

    symbolSize = (double)size;
    switch (penPtr->symbol.type) {
    case SYMBOL_SQUARE:
    case SYMBOL_CROSS:
    case SYMBOL_PLUS:
    case SYMBOL_SCROSS:
    case SYMBOL_SPLUS:
	symbolSize = (double)Round(size * S_RATIO);
	break;
    case SYMBOL_TRIANGLE:
    case SYMBOL_ARROW:
	symbolSize = (double)Round(size * 0.7);
	break;
    case SYMBOL_DIAMOND:
	symbolSize = (double)Round(size * M_SQRT1_2);
	break;

    default:
	break;
    }

    Blt_Ps_Format(ps, "%g %g %g %s\n", x, y, symbolSize, 
		  symbolMacros[penPtr->symbol.type]);
}


/*
 *---------------------------------------------------------------------------
 *
 * IsolineToPostScript --
 *
 * 	Draws each isolines as one or more polylines.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
IsolineToPostScript(Graph *graphPtr, Blt_Ps ps, ContourElement *elemPtr,
		    Isoline *isoPtr, ContourPen *penPtr)
{
    TraceSegment *s;
    XColor *colorPtr;

    SetLineAttributes(ps, penPtr);
    colorPtr = NULL;
    if (penPtr->traceColor == COLOR_PALETTE) {
	if (colorPtr == NULL) {
	    XColor color;

	    color.red   = isoPtr->paletteColor.Red * 257;
	    color.green = isoPtr->paletteColor.Green * 257;
	    color.blue  = isoPtr->paletteColor.Blue * 257;
	    colorPtr = Tk_GetColorByValue(graphPtr->tkwin, &color);
	}
	/* Temporarily set the color from the interpolated value. */
	Blt_Ps_XSetForeground(ps, colorPtr);
    } 
    Blt_Ps_Append(ps, "% start segments\n");
    Blt_Ps_Append(ps, "newpath\n");
    for (s = isoPtr->segments; s != NULL; s = s->next) {
	Blt_Ps_Format(ps, "  %g %g moveto %g %g lineto\n", 
		s->x1, s->y1, s->x2, s->y2);
	Blt_Ps_Append(ps, "DashesProc stroke\n");
    }
    Blt_Ps_Append(ps, "% end segments\n");
}

#ifdef notdef

/*
 *---------------------------------------------------------------------------
 *
 * SymbolsToPostScript --
 *
 * 	Draw a symbol centered at the given x,y window coordinate based upon
 * 	the element symbol type and size.
 *
 * Results:
 *	None.
 *
 * Problems:
 *	Most notable is the round-off errors generated when calculating the
 *	centered position of the symbol.
 *
 *---------------------------------------------------------------------------
 */
static void
SymbolsToPostScript(Blt_Ps ps, Trace *tracePtr, LinePen *penPtr)
{
    TracePoint *p;
    double size;
    static const char *symbolMacros[] =
    {
	"Li", "Sq", "Ci", "Di", "Pl", "Cr", "Sp", "Sc", "Tr", "Ar", "Bm", 
	(char *)NULL,
    };

    GetSymbolPostScriptInfo(ps, tracePtr->elemPtr, penPtr, 
	    tracePtr->symbolSize);
    size = (double)tracePtr->symbolSize;
    switch (penPtr->symbol.type) {
    case SYMBOL_SQUARE:
    case SYMBOL_CROSS:
    case SYMBOL_PLUS:
    case SYMBOL_SCROSS:
    case SYMBOL_SPLUS:
	size = (double)Round(size * S_RATIO);
	break;
    case SYMBOL_TRIANGLE:
    case SYMBOL_ARROW:
	size = (double)Round(size * 0.7);
	break;
    case SYMBOL_DIAMOND:
	size = (double)Round(size * M_SQRT1_2);
	break;

    default:
	break;
    }
    tracePtr->drawFlags |= KNOT;
    if (tracePtr->elemPtr->reqMaxSymbols > 0) {
	tracePtr->drawFlags |= SYMBOL;
    }
    for (p = tracePtr->head; p != NULL; p = p->next) {
	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	Blt_Ps_Format(ps, "%g %g %g %s\n", p->x, p->y, size, 
		symbolMacros[penPtr->symbol.type]);
    }
}

static void
ValuesToPostScript(Blt_Ps ps, Trace *tracePtr, ContourPen *penPtr)
{
    TracePoint *p;
    const char *fmt;
    Vertex *vertices;

    fmt = penPtr->valueFormat;
    if (fmt == NULL) {
	fmt = "%g";
    }
    vertices = tracePtr->elemPtr->vectices;
    for (p = tracePtr->head; p != NULL; p = p->next) {
	double x, y;
	char string[TCL_DOUBLE_SPACE * 2 + 2];

	if (!DRAWN(tracePtr, p->flags)) {
	    continue;
	}
	x = vertices[p->index].x;
	y = vertices[p->index].y;
	if (penPtr->valueFlags == SHOW_X) {
	    Blt_FormatString(string, TCL_DOUBLE_SPACE, fmt, x); 
	} else if (penPtr->valueFlags == SHOW_Y) {
	    Blt_FormatString(string, TCL_DOUBLE_SPACE, fmt, y); 
	} else if (penPtr->valueFlags == SHOW_BOTH) {
	    Blt_FormatString(string, TCL_DOUBLE_SPACE, fmt, x);
	    strcat(string, ",");
	    Blt_FormatString(string + strlen(string), TCL_DOUBLE_SPACE, fmt, y);
	}
	Blt_Ps_DrawText(ps, string, &penPtr->valueStyle, p->x, p->y);
    }
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * ActiveToPostScriptProc --
 *
 *	Similar to the NormalToPostScript procedure, generates PostScript
 *	commands to display the bars representing the active bar segments of
 *	the element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	PostScript pen width, dashes, and color settings are changed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
ActiveToPostScriptProc(Graph *graphPtr, Blt_Ps ps, Element *basePtr)
{
#ifdef FIXME
    ContourElement *elemPtr = (ContourElement *)basePtr;
#endif
}

/*
 *---------------------------------------------------------------------------
 *
 * NormalToPostScriptProc --
 *
 *	Generates PostScript commands to form the mesh and isolines
 *	representing the contour element.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	PostScript pen width, dashes, and color settings are changed.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
NormalToPostScriptProc(Graph *graphPtr, Blt_Ps ps, Element *basePtr)
{
#ifdef FIXME
    ContourElement *elemPtr = (ContourElement *)basePtr;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    MeshToPostScript(graphPtr, drawable, elemPtr);
    for (hPtr = Blt_FirstHashEntry(&elemPtr->isoTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Isoline *isoPtr;

	isoPtr = Blt_GetHashValue(hPtr);
	if (isoPtr->flags & HIDE) {
	    continue;			/* Don't draw this isoline. */
	}
	if (elemPtr->flags & ISOLINES) {
	    IsolineToPostScript(graphPtr, ps, elemPtr, isoPtr, isoPtr->penPtr);
	}
	if (elemPtr->flags & SYMBOLS) {
	    SymbolsToPostScript(graphPtr, ps, isoPtr, isoPtr->penPtr);
	}
    }
#endif
}

/* Isoline TCL API operations. */

/*
 *---------------------------------------------------------------------------
 *
 * IsolineActivateOp --
 *
 *	Indicates if a isoline by the given name exists in the element.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the element names.
 *
 *	.g element isoline exists $elem $name
 *
 *---------------------------------------------------------------------------
 */
static int
IsolineActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    ContourElement *elemPtr;
    IsolineIterator iter;
    Isoline *isoPtr;

    if (GetContourElement(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    if (GetIsolineIterator(interp, elemPtr, objv[5], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
	 isoPtr = NextTaggedIsoline(&iter)) {
	isoPtr->flags |= ACTIVE;
    }
    graphPtr->flags |= CACHE_DIRTY;
    graphPtr->flags |= REDRAW_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * IsolineCgetOp --
 *
 *	.g element isoline cget $elem $name -option
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsolineCgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    ContourElement *elemPtr;
    Isoline *isoPtr;

    if (GetContourElement(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element. */
    }
    if (GetIsolineFromObj(interp, elemPtr, objv[5], &isoPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named isoline. */
    }
    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, isolineSpecs,
	(char *)isoPtr, objv[6], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * DistanceToIsoline --
 *
 *	Find the closest mesh vertex to the specified screen coordinates.
 *
 * Results:
 *	None.
 *
 * Side Effects:  
 *	The search structure will be willed with the information of the
 *	closest point.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static double
DistanceToIsoline(
    int x, int y,			/* Sample X-Y coordinate. */
    Point2d *p, Point2d *q,		/* End points of the line segment. */
    Point2d *t)				/* (out) Point on line segment. */
{
    double right, left, top, bottom;

    *t = Blt_GetProjection(x, y, p, q);
    if (p->x > q->x) {
	right = p->x, left = q->x;
    } else {
	left = p->x, right = q->x;
    }
    if (p->y > q->y) {
	bottom = p->y, top = q->y;
    } else {
	top = p->y, bottom = q->y;
    }
    if (t->x > right) {
	t->x = right;
    } else if (t->x < left) {
	t->x = left;
    }
    if (t->y > bottom) {
	t->y = bottom;
    } else if (t->y < top) {
	t->y = top;
    }
    return hypot(t->x - x, t->y - y);
}

/*
 *---------------------------------------------------------------------------
 *
 * IsolineClosestOp --
 *
 *	Find the closest isoline for this element to the given screen
 *	coordinates.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the element names.
 *
 *	.g element isoline closest $elem x y ?option value?...
 *
 *---------------------------------------------------------------------------
 */

static Blt_ConfigSpec closestSpecs[] = {
    {BLT_CONFIG_PIXELS_NNEG, "-halo", (char *)NULL, (char *)NULL,
	(char *)NULL, Blt_Offset(ClosestSearch, halo), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

static int
IsolineClosestOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		 Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    ContourElement *elemPtr;
    ClosestSearch srch;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    int x, y;

    if (graphPtr->flags & RESET_AXES) {
	Blt_ResetAxes(graphPtr);
    }
    if (GetContourElement(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    if (elemPtr->flags & (HIDE|MAP_ITEM)) {
	return TCL_OK;
    }
    if (Tcl_GetIntFromObj(interp, objv[5], &x) != TCL_OK) {
	Tcl_AppendResult(interp, ": bad window x-coordinate", (char *)NULL);
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[6], &y) != TCL_OK) {
	Tcl_AppendResult(interp, ": bad window y-coordinate", (char *)NULL);
	return TCL_ERROR;
    }
    srch.dist = graphPtr->halo;
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, closestSpecs, 
	objc - 7, objv + 7, (char *)&srch, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;		/* Error occurred processing an
					 * option. */
    }
    srch.dist = (double)(srch.halo + 1);
    srch.item = NULL;

    /* Search all the isolines in the element. */
    for (hPtr = Blt_FirstHashEntry(&elemPtr->isoTable, &iter); hPtr != NULL; 
	 hPtr = Blt_NextHashEntry(&iter)) {
	Isoline *isoPtr;
	Blt_ChainLink link;

	isoPtr = Blt_GetHashValue(hPtr);
	/* Examine every trace that represents the isoline. */
	for (link = Blt_Chain_FirstLink(isoPtr->traces); link != NULL; 
	     link = Blt_Chain_NextLink(link)) {
	    Trace *tracePtr;
	    TracePoint *p, *q;

	    tracePtr = Blt_Chain_GetValue(link);
	    /* Examine every line segment in the trace. */
	    for (p = tracePtr->head, q = p->next; q != NULL; q = q->next) {
		Point2d p1, p2, b;
		double d;

		p1.x = p->x, p1.y = p->y;
		p2.x = q->x, p2.y = q->y;
		d = DistanceToIsoline(x, y, &p1, &p2, &b);
		if (d < srch.dist) {
		    srch.point = b;
		    srch.dist = d;
		    srch.item = isoPtr;
		}
		p = q;
	    }
	}
    }
    if (srch.dist < (double)srch.halo) {
	Tcl_Obj *objPtr, *listObjPtr;	/* Return a list of name value
					 * pairs. */
	Isoline *isoPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);

	isoPtr = srch.item;
	/* Name of isoline. */
	objPtr = Tcl_NewStringObj("name", 4);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	objPtr = Tcl_NewStringObj(isoPtr->obj.name, -1); 
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

	/* Value of isoline. */
	objPtr = Tcl_NewStringObj("z", 1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	objPtr = Tcl_NewDoubleObj(isoPtr->value);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

	/* X-coordinate of closest point on isoline. */
	objPtr = Tcl_NewStringObj("x", 1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	objPtr = Tcl_NewDoubleObj(srch.point.x);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

	/* Y-coordinate of closest point on isoline. */
	objPtr = Tcl_NewStringObj("y", 1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	objPtr = Tcl_NewDoubleObj(srch.point.y);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

	/* Distance to from search point. */
	objPtr = Tcl_NewStringObj("dist", 4);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	objPtr = Tcl_NewDoubleObj(srch.dist);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);

	Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * IsolineConfigureOp --
 *
 * 	This procedure is called to process an objv/objc list, plus the Tk
 * 	option database, in order to configure (or reconfigure) the widget.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then interp->result
 *	contains an error message.
 *
 * Side Effects:
 *	Configuration information, such as text string, colors, font, etc. get
 *	set for setPtr; old resources get freed, if there were any.  The
 *	widget is redisplayed.
 *
 *	.g element isoline configure $elem $isoline ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
IsolineConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		   Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    ContourElement *elemPtr;
    Isoline *isoPtr;
    IsolineIterator iter;

    if (GetContourElement(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element. */
    }
    if (objc <= 7) {
	if (GetIsolineFromObj(interp, elemPtr, objv[5], &isoPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (objc == 6) {
	    return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
		isolineSpecs, (char *)isoPtr, (Tcl_Obj *)NULL, 0);
	} else if (objc == 7) {
	    return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, 
		isolineSpecs, (char *)isoPtr, objv[6], 0);
	}
    }
    if (GetIsolineIterator(interp, elemPtr, objv[5], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
	 isoPtr = NextTaggedIsoline(&iter)) {
	if (ConfigureIsoline(interp, isoPtr, objc - 6, objv + 6, 
		BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    graphPtr->flags |= CACHE_DIRTY;
    graphPtr->flags |= REDRAW_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsolineCreateOp --
 *
 *	Creates a isoline for the named element.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the element names.
 *
 *	.g element isoline create $elem ?$name? ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
IsolineCreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    ContourElement *elemPtr;
    Graph *graphPtr = clientData;
    Isoline *isoPtr;
    const char *name;
    char ident[200];

    if (GetContourElement(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;	/* Can't find named element */
    }
    name = NULL;
    if (objc > 5) {
	const char *string;

	string = Tcl_GetString(objv[5]);
	if (string[0] != '-') {
	    if (GetIsolineFromObj(NULL, elemPtr, objv[5], &isoPtr) == TCL_OK) {
		Tcl_AppendResult(interp, "isoline \"", string, 
			"\" already exists", (char *)NULL);
		return TCL_ERROR;
	    }
	    name = string;
	    objc--, objv++;
	}
    }
    /* If no name was given for the marker, make up one. */
    if (name == NULL) {
	Blt_FormatString(ident, 200, "isoline%d", elemPtr->nextIsoline++);
	name = ident;
    }
    isoPtr = NewIsoline(interp, elemPtr, name);
    if (isoPtr == NULL) {
	return TCL_ERROR;
    }

    if (ConfigureIsoline(interp, isoPtr, objc - 5, objv + 5, 0) != TCL_OK) {
	DestroyIsoline(isoPtr);
	return TCL_ERROR;
    }

    if (Blt_ConfigureComponentFromObj(interp, graphPtr->tkwin, name, "Isoline",
	isolineSpecs, objc - 5, objv + 5, (char *)isoPtr, 0) != TCL_OK) {
	DestroyIsoline(isoPtr);
	return TCL_ERROR;
    }
    elemPtr->flags |= REDRAW_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), isoPtr->obj.name, -1);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * IsolineDeactivateOp --
 *
 *	Indicates if a isoline by the given name exists in the element.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the element names.
 *
 *	.g element isoline deactivate $elem $name
 *
 *---------------------------------------------------------------------------
 */
static int
IsolineDeactivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		    Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    ContourElement *elemPtr;
    IsolineIterator iter;
    Isoline *isoPtr;

    if (GetContourElement(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    if (GetIsolineIterator(interp, elemPtr, objv[5], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
	 isoPtr = NextTaggedIsoline(&iter)) {
	isoPtr->flags &= ~ACTIVE;
    }
    graphPtr->flags |= CACHE_DIRTY;
    graphPtr->flags |= REDRAW_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}    


/*
 *---------------------------------------------------------------------------
 *
 * IsolineDeleteOp --
 *
 *	Deletes one or more isolines from the named element.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the element names.
 *
 *	.g element isoline delete $elem ?$name?...
 *
 *---------------------------------------------------------------------------
 */
static int
IsolineDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    ContourElement *elemPtr;
    Blt_HashTable deleteTable;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    int i;

    if (GetContourElement(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;	/* Can't find named element */
    }
    Blt_InitHashTable(&deleteTable, BLT_ONE_WORD_KEYS);
    for (i = 5; i < objc; i++) {
	IsolineIterator iter;
	Isoline *isoPtr;
	
	if (GetIsolineIterator(interp, elemPtr, objv[i], &iter) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
	     isoPtr = NextTaggedIsoline(&iter)) {
	    Blt_HashEntry *hPtr;
	    int isNew;

	    hPtr = Blt_CreateHashEntry(&deleteTable, (char *)isoPtr, &isNew);
	    if (isNew) {
		Blt_SetHashValue(hPtr, isoPtr);
	    }
	}
    }
    for (hPtr = Blt_FirstHashEntry(&deleteTable, &iter); hPtr != NULL;
	 hPtr = Blt_NextHashEntry(&iter)) {
	Isoline *isoPtr;

	isoPtr = Blt_GetHashValue(hPtr);
	DestroyIsoline(isoPtr);
    }
    Blt_DeleteHashTable(&deleteTable);
    elemPtr->flags |= REDRAW_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * IsolineExistsOp --
 *
 *	Indicates if a isoline by the given name exists in the element.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the element names.
 *
 *	.g element isoline exists $elem $name
 *
 *---------------------------------------------------------------------------
 */
static int
IsolineExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    ContourElement *elemPtr;
    Isoline *isoPtr;
    int bool;

    if (GetContourElement(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;	/* Can't find named element */
    }
    bool = (GetIsolineFromObj(NULL, elemPtr, objv[5], &isoPtr) == TCL_OK);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}    

/*
 *---------------------------------------------------------------------------
 *
 * IsolineNamesOp --
 *
 *	Returns the names of the isoline in the element is the graph matching
 *	one of more patterns provided.  If no pattern arguments
 *	are given, then all element names will be returned.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the element names.
 *
 *	.g element isoline names $elem $pattern
 *
 *---------------------------------------------------------------------------
 */
static int
IsolineNamesOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;
    ContourElement *elemPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (GetContourElement(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;	/* Can't find named element */
    }
    if (objc == 5) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&elemPtr->isoTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Isoline *isoPtr;
	    Tcl_Obj *objPtr;

	    isoPtr = Blt_GetHashValue(hPtr);
	    objPtr = Tcl_NewStringObj(isoPtr->obj.name, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    } else {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&elemPtr->isoTable, &iter);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    Isoline *isoPtr;
	    int i;

	    isoPtr = Blt_GetHashValue(hPtr);
	    for (i = 5; i < objc; i++) {
		const char *pattern;

		pattern = Tcl_GetString(objv[i]);
		if (Tcl_StringMatch(isoPtr->obj.name, pattern)) {
		    Tcl_Obj *objPtr;

		    objPtr = Tcl_NewStringObj(isoPtr->obj.name, -1);
		    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
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
 * IsolineStepsOp --
 *
 *	Generates the given number of evenly placed isolines in the 
 *	element.
 *
 * Results:
 *	The return value is a standard TCL result. The interpreter
 *	result will contain a TCL list of the isoline names.
 *
 *	.g element isoline steps $elem $n ?option value?...
 *
 *---------------------------------------------------------------------------
 */
static int
IsolineStepsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    ContourElement *elemPtr;
    Isoline *isoPtr;
    long count, i;

    if (GetContourElement(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;	/* Can't find named element */
    }
    if (Blt_GetCountFromObj(interp, objv[5], COUNT_POS, &count) != TCL_OK) {
	return TCL_ERROR;
    }
    if (count < 2) {
	Tcl_AppendResult(interp, "two few steps: must >= 2", (char *)NULL);
	return TCL_ERROR;
    }
    for (i = 0; i < count; i++) {
	isoPtr = NewIsoline(interp, elemPtr, NULL);
	if (isoPtr == NULL) {
	    return TCL_ERROR;
	}
	isoPtr->reqValue = (double)i / (double)(count - 1);
	if (ConfigureIsoline(interp, isoPtr, objc - 6, objv + 6, 0) != TCL_OK) {
	    DestroyIsoline(isoPtr);
	    return TCL_ERROR;
	}
    }
    elemPtr->flags |= REDRAW_WORLD;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}    

/* Isoline tag api */

/*
 *---------------------------------------------------------------------------
 *
 * IsoTagAddOp --
 *
 *	.t element tag add $elem $tagName isoline...
 *
 *---------------------------------------------------------------------------
 */
static int
IsoTagAddOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    ContourElement *elemPtr = clientData;
    const char *tag;

    tag = Tcl_GetString(objv[5]);
    if (strcmp(tag, "all") == 0) {
	Tcl_AppendResult(interp, "can't add reserved tag \"", tag, "\"", 
			 (char *)NULL);
	return TCL_ERROR;
    }
    if (objc == 6) {
	/* No nodes specified.  Just add the tag. */
	AddIsoTag(elemPtr, NULL, tag);
    } else {
	int i;

	for (i = 7; i < objc; i++) {
	    Isoline *isoPtr;
	    IsolineIterator iter;
	    
	    if (GetIsolineIterator(interp, elemPtr, objv[i], &iter) != TCL_OK) {
		return TCL_ERROR;
	    }
	    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
		 isoPtr = NextTaggedIsoline(&iter)) {
		AddIsoTag(elemPtr, isoPtr, tag);
	    }
	}
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * IsoTagDeleteOp --
 *
 *	.g element tag delete $elem tagName isoline...
 *
 *---------------------------------------------------------------------------
 */
static int
IsoTagDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    ContourElement *elemPtr = clientData;
    const char *tag;
    Blt_HashTable *tablePtr;

    tag = Tcl_GetString(objv[5]);
    if (strcmp(tag, "all") == 0) {
	Tcl_AppendResult(interp, "can't delete reserved tag \"", tag, "\"", 
			 (char *)NULL);
        return TCL_ERROR;
    }
    tablePtr = NULL;			/* Suppress compiler warning. */
    if (GetIsoTagTable(elemPtr, tag, &tablePtr) == TCL_OK) {
        int i;
      
        for (i = 6; i < objc; i++) {
	    Isoline *isoPtr;
	    IsolineIterator iter;

	    if (GetIsolineIterator(interp, elemPtr, objv[i], &iter) != TCL_OK) {
	        return TCL_ERROR;
	    }
	    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
		 isoPtr = NextTaggedIsoline(&iter)) {
		Blt_HashEntry *hPtr;

	        hPtr = Blt_FindHashEntry(tablePtr, (char *)isoPtr);
	        if (hPtr != NULL) {
		    Blt_DeleteHashEntry(tablePtr, hPtr);
	        }
	   }
       }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * IsoTagExistsOp --
 *
 *	Returns the existence of the one or more tags in the given node.  If
 *	the node has any the tags, true is return in the interpreter.
 *
 *	.g element isoline tag exists $elem isoline tag1 tag2 tag3...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsoTagExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    ContourElement *elemPtr = clientData;
    IsolineIterator iter;
    int i;

    if (GetIsolineIterator(interp, elemPtr, objv[5], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 6; i < objc; i++) {
	const char *tag;
	Isoline *isoPtr;

	tag = Tcl_GetString(objv[i]);
	for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
	     isoPtr = NextTaggedIsoline(&iter)) {
	    if (HasIsoTag(isoPtr, tag)) {
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
 * IsoTagForgetOp --
 *
 *	Removes the given tags from all tabs.
 *
 *	.g element tag forget $elem tag1 tag2 tag3...
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsoTagForgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    ContourElement *elemPtr = clientData;
    int i;

    for (i = 5; i < objc; i++) {
	const char *tag;

	tag = Tcl_GetString(objv[i]);
	ForgetIsoTag(elemPtr, tag);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsoTagGetOp --
 *
 *	Returns tag names for a given node.  If one of more pattern arguments
 *	are provided, then only those matching tags are returned.
 *
 *	.g element tag get $elem $iso pat1 pat2...
 *
 *---------------------------------------------------------------------------
 */
static int
IsoTagGetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    ContourElement *elemPtr = clientData;
    Isoline *isoPtr;
    IsolineIterator iter;
    Tcl_Obj *listObjPtr;

    if (GetIsolineIterator(interp, elemPtr, objv[5], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
	 isoPtr = NextTaggedIsoline(&iter)) {
	if (objc == 6) {
	    Blt_HashEntry *hPtr;
	    Blt_HashSearch hiter;

	    for (hPtr = Blt_FirstHashEntry(&elemPtr->tagTable, &hiter); 
		 hPtr != NULL; hPtr = Blt_NextHashEntry(&hiter)) {
		Blt_HashTable *tablePtr;

		tablePtr = Blt_GetHashValue(hPtr);
		if (Blt_FindHashEntry(tablePtr, (char *)isoPtr) != NULL) {
		    const char *tag;
		    Tcl_Obj *objPtr;

		    tag = Blt_GetHashKey(&elemPtr->tagTable, hPtr);
		    objPtr = Tcl_NewStringObj(tag, -1);
		    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
		}
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj("all", 3));
	} else {
	    int i;
	    
	    /* Check if we need to add the special tags "all" */
	    for (i = 6; i < objc; i++) {
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
	    for (i = 7; i < objc; i++) {
		Blt_HashEntry *hPtr;
		Blt_HashSearch hiter;
		const char *pattern;
		
		pattern = Tcl_GetString(objv[i]);
		for (hPtr = Blt_FirstHashEntry(&elemPtr->tagTable, &hiter); 
		     hPtr != NULL; hPtr = Blt_NextHashEntry(&hiter)) {
		    const char *tag;
		    Blt_HashTable *tablePtr;

		    tablePtr = Blt_GetHashValue(hPtr);
		    tag = Blt_GetHashKey(&elemPtr->tagTable, hPtr);
		    if (!Tcl_StringMatch(tag, pattern)) {
			continue;
		    }
		    if (Blt_FindHashEntry(tablePtr, (char *)isoPtr) != NULL) {
			Tcl_Obj *objPtr;

			objPtr = Tcl_NewStringObj(tag, -1);
			Tcl_ListObjAppendElement(interp, listObjPtr,objPtr);
		    }
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
 * IsoTagNamesOp --
 *
 *	Returns the names of all the tags in the tabset.  If one of more node
 *	arguments are provided, then only the tags found in those nodes are
 *	returned.
 *
 *	.g element tag names $elem tab tab tab...
 *
 *---------------------------------------------------------------------------
 */
static int
IsoTagNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    ContourElement *elemPtr = clientData;
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewStringObj("all", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    if (objc == 5) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	for (hPtr = Blt_FirstHashEntry(&elemPtr->tagTable, &iter); 
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    const char *tag;

	    tag = Blt_GetHashKey(&elemPtr->tagTable, hPtr);
	    objPtr = Tcl_NewStringObj(tag, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    } else {
	Blt_HashTable uniqTable;
	int i;

	Blt_InitHashTable(&uniqTable, BLT_STRING_KEYS);
	for (i = 5; i < objc; i++) {
	    IsolineIterator iter;
	    Isoline *isoPtr;

	    if (GetIsolineIterator(interp, elemPtr, objPtr, &iter) != TCL_OK) {
		goto error;
	    }
	    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
		 isoPtr = NextTaggedIsoline(&iter)) {
		Blt_HashEntry *hPtr;
		Blt_HashSearch hiter;

		for (hPtr = Blt_FirstHashEntry(&elemPtr->tagTable, &hiter); 
		     hPtr != NULL; hPtr = Blt_NextHashEntry(&hiter)) {
		    const char *tag;
		    Blt_HashTable *tablePtr;

		    tag = Blt_GetHashKey(&elemPtr->tagTable, hPtr);
		    tablePtr = Blt_GetHashValue(hPtr);
		    if (Blt_FindHashEntry(tablePtr, isoPtr) != NULL) {
			int isNew;

			Blt_CreateHashEntry(&uniqTable, tag, &isNew);
		    }
		}
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
 * IsoTagSetOp --
 *
 *	Sets one or more tags for a given tab.  Tag names can't start with a
 *	digit (to distinquish them from node ids) and can't be a reserved tag
 *	("all").
 *
 *	.g element tag set $elem $iso tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
IsoTagSetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv)
{
    int i;
    IsolineIterator iter;
    ContourElement *elemPtr = clientData;

    if (GetIsolineIterator(interp, elemPtr, objv[5], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 6; i < objc; i++) {
	const char *tag;
	Isoline *isoPtr;

	tag = Tcl_GetString(objv[i]);
	if (strcmp(tag, "all") == 0) {
	    Tcl_AppendResult(interp, "can't add reserved tag \"", tag, "\"",
			     (char *)NULL);	
	    return TCL_ERROR;
	}
	for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
	     isoPtr = NextTaggedIsoline(&iter)) {
	    AddIsoTag(elemPtr, isoPtr, tag);
	}    
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsoTagUnsetOp --
 *
 *	Removes one or more tags from a given isoline. If a isoline doesn't 
 *	exist or is a reserved tag ("all"), nothing will be done and no error
 *	message will be returned.
 *
 *	.t element tag unset $elem $iso tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
IsoTagUnsetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    ContourElement *elemPtr = clientData;
    Isoline *isoPtr;
    IsolineIterator iter;

    if (GetIsolineIterator(interp, elemPtr, objv[5], &iter) != TCL_OK) {
	return TCL_ERROR;
    }
    for (isoPtr = FirstTaggedIsoline(&iter); isoPtr != NULL; 
	 isoPtr = NextTaggedIsoline(&iter)) {
	int i;
	for (i = 6; i < objc; i++) {
	    RemoveIsoTag(isoPtr, Tcl_GetString(objv[i]));
	}    
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_IsoTagOp --
 *
 * 	This procedure is invoked to process tag operations.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	See the user documentation.
 *
 *	.g element isotag $elem $iso args...
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec isoTagOps[] =
{
    {"add",     1, IsoTagAddOp,      4, 0, "elem isoline ?tag...?",},
    {"delete",  1, IsoTagDeleteOp,   4, 0, "elem isoline ?tag...?",},
    {"exists",  1, IsoTagExistsOp,   4, 0, "elem isoline ?tag...?",},
    {"forget",  1, IsoTagForgetOp,   3, 0, "elem ?tag...?",},
    {"get",     1, IsoTagGetOp,      4, 0, "elem isoline ?pattern...?",},
    {"names",   1, IsoTagNamesOp,    3, 0, "elem ?tab...?",},
    {"set",     1, IsoTagSetOp,      4, 0, "elem isoline ?tag...",},
    {"unset",   1, IsoTagUnsetOp,    4, 0, "elem isoline ?tag...",},
};

static int numIsoTagOps = sizeof(isoTagOps) / sizeof(Blt_OpSpec);

int
Blt_IsoTagOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Tcl_ObjCmdProc *proc;
    ContourElement *elemPtr;
    int result;

    proc = Blt_GetOpFromObj(interp, numIsoTagOps, isoTagOps, BLT_OP_ARG2,
			    objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    if (GetContourElement(interp, graphPtr, objv[4], &elemPtr) != TCL_OK) {
	return TCL_ERROR;		/* Can't find named element */
    }
    result = (*proc)(elemPtr, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsolineOp --
 *
 *	.g element isoline create $elem $name -value $value -color $color
 *	.g element isoline configure $elem $name -value $value \
 *		-color $color -symbol triangle -hide no -dashes dot 
 *	.g element isoline delete $elem delete $name
 *
 *	.g element isoline delete $elem all
 *	.g element isoline steps $elem 10 ?value option?
 *
 *	.g element isolines create $elem $name -value $value -color $color
 *	.g element isolines configure $elem -values $values \
 *		-hide no -loose yes -logscale yes -showvalues yes \
 *		-stepsize 0.5 -symbols triangle \
 *	.g element isoline delete $elem delete $name
 *
 *	.g element isoline delete $elem all
 *	.g element isoline steps $elem 10 ?value option?
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec isolineOps[] = {
    {"activate",  1, IsolineActivateOp, 6, 6, "elem name",},
    {"cget",      2, IsolineCgetOp,     6, 7, "elem name option",},
    {"closest",   2, IsolineClosestOp,  6, 0, "elem x y ?switches?",},
    {"configure", 2, IsolineConfigureOp,6, 0, "elem name ?option value?...",},
    {"create",    2, IsolineCreateOp,   5, 0, "elem ?name? ?option value?...",},
    {"deactivate",3, IsolineDeactivateOp,5, 5, "elem",},
    {"delete",    3, IsolineDeleteOp,   5, 0, "elem ?name?...",},
    {"exists",    1, IsolineExistsOp,   6, 6, "elem name",},
    {"names",     1, IsolineNamesOp,    5, 0, "elem ?pattern?...",},
    {"steps",     1, IsolineStepsOp,    6, 0, "elem count",},
};

static int numIsolineOps = sizeof(isolineOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
int
Blt_IsolineOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numIsolineOps, isolineOps, BLT_OP_ARG3, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

#ifdef notdef

#define  FRACBITS 12

typedef struct {
    int64_t A, B, C;
} EdgeEquation;

typedef struct { 
    EdgeEquation edge[3];
    int64_t red[3], green[3], blue[3], alpha[3];
    int64_t value[3];
    int64_t x1, x2, y1, y2;
    int xOffset, yOffset;
} TriangleRenderer;

static void
InitEdgeEquation(EdgeEquation *eq, Vertex *p, Vertex *q)
{
    double a, b, c;

    a = p->y - q->y;
    b = q->x - p->x;
    c = -0.5 * ((a * (p->x + q->x)) + (b * (p->y + q->y)));
    eq->A = (int64_t)(a * (1<<FRACBITS));
    eq->B = (int64_t)(b * (1<<FRACBITS));
    eq->C = (int64_t)(c * (1<<FRACBITS));
    eq->flag = 0;
    if (eq->A >= 0) {
	eq->flag += 8;
    }
    if (eq->B >= 0) {
	eq->flag += 1;
    }
}

static void INLINE
FlipEquation(EdgeEquation *eq)
{
    eq->A = -eq->A;
    eq->B = -eq->B;
    eq->C = -eq->C;
}

static int 
InitRenderer(ContourElement *elemPtr, Triangle *t, TriangleRenderer *renPtr)
{
    Region2d exts;
    int64_t a, b, c;
    double scale, zscale;
    double sp0, sp1, sp2;
    double da, db, dc;
    int64_t area;
    Region2d bbox;
    Vertex *v1, *v2, *v3;
    v1 = elemPtr->vertices + t->a;
    v2 = elemPtr->vertices + t->b;
    v3 = elemPtr->vertices + t->c;
    memset(renPtr, 0, sizeof(TriangleRenderer));

    /* Get the triangle's bounding box. */
    bbox.left   = MIN3(v1->x, v2->x, v3->x);
    bbox.right  = MAX3(v1->x, v2->x, v3->x);
    bbox.top    = MIN3(v1->y, v2->y, v3->y);
    bbox.bottom = MAX3(v1->y, v2->y, v3->y);

    Blt_GraphExtents(elemPtr, &exts);
    /* Do a quick minmax test on the bounding box with the plot area. */
    if ((bbox.right < exts.left) || (bbox.bottom < exts.top) ||
	(bbox.top > exts.bottom) || (bbox.left > exts.right)) {
	return FALSE;			/* Triangle isn't visible. */
    }
    /* Clip the (possibly visible) bounding box to plot area */
    if (exts.left > bbox.left) {
	bbox.left = exts.left;
    }
    if (exts.right < bbox.right) {
	bbox.right = exts.right;
    }
    if (exts.top > bbox.top) {
	bbox.top = exts.top;
    }
    if (exts.bottom < bbox.bottom) {
	bbox.bottom = exts.bottom;
    }
    renPtr->x2 = (int64_t)(bbox.right + ((bbox.right < 0.0) ? -0.5 : 0.5));
    renPtr->x1 = (int64_t)(bbox.left + ((bbox.left < 0.0) ? -0.5 : 0.5));
    renPtr->y2 = (int64_t)(bbox.bottom + ((bbox.bottom < 0.0) ? -0.5 : 0.5));
    renPtr->y1 = (int64_t)(bbox.top + ((bbox.top < 0.0) ? -0.5 : 0.5));

    /* Compute the three edge equations */
    InitEdgeEquation(renPtr->edge + 0, v1, v2);
    InitEdgeEquation(renPtr->edge + 1, v2, v3);
    InitEdgeEquation(renPtr->edge + 2, v3, v1);
    scale = 0.0;
    /*
      Trick #1: Orient edges so that the
      triangle's interior lies within all
      of their positive half-spaces.
      
      Assuring that the area is positive
      accomplishes this
    */
    area = renPtr->edge[0].C + renPtr->edge[1].C + renPtr->edge[2].C;
    if (area == 0.0) {
	return FALSE;			/* Degenerate triangle. */
    }
    if (area < 0.0) {
	FlipEquation(renPtr->edge + 0);
	FlipEquation(renPtr->edge + 1);
	FlipEquation(renPtr->edge + 2);
	area = -area;
    }
    if (scale <= 0.0) {
	scale = (1 << FRACBITS) / ((double) area);
    }
    sp0 = v1->color.Alpha * scale;
    sp1 = v2->color.Alpha * scale;
    sp2 = v3->color.Alpha * scale;
    a = (int64_t)((renPtr->edge[0].A * sp2) + (renPtr->edge[1].A * sp0) + 
	       (renPtr->edge[2].A * sp1));
    b = (int64_t)((renPtr->edge[0].B * sp2) + (renPtr->edge[1].B * sp0) + 
	       (renPtr->edge[2].B * sp1));
    c = (int64_t)((renPtr->edge[0].C * sp2) + (renPtr->edge[1].C * sp0) + 
	       (renPtr->edge[2].C * sp1));
    renPtr->alpha[0] = a;
    renPtr->alpha[1] = b;
    renPtr->alpha[2] = (a * renPtr->x1) + (b * renPtr->y1) + c + 
	(1 << (FRACBITS - 1));
    sp0 = v1->color.Red * scale;
    sp1 = v2->color.Red * scale;
    sp2 = v3->color.Red * scale;
    a = (int64_t)((renPtr->edge[0].A * sp2) + (renPtr->edge[1].A * sp0) + 
	       (renPtr->edge[2].A * sp1));
    b = (int64_t)((renPtr->edge[0].B * sp2) + (renPtr->edge[1].B * sp0) + 
	       (renPtr->edge[2].B * sp1));
    c = (int64_t)((renPtr->edge[0].C * sp2) + (renPtr->edge[1].C * sp0) + 
	       (renPtr->edge[2].C * sp1));
    renPtr->red[0] = a;
    renPtr->red[1] = b;
    renPtr->red[2] = (a * renPtr->x1) + (b * renPtr->y1) + c + 
	(1 << (FRACBITS - 1));
    sp0 = v1->color.Green * scale;
    sp1 = v2->color.Green * scale;
    sp2 = v3->color.Green * scale;
    a = (int64_t)((renPtr->edge[0].A * sp2) + (renPtr->edge[1].A * sp0) + 
	       (renPtr->edge[2].A * sp1));
    b = (int64_t)((renPtr->edge[0].B * sp2) + (renPtr->edge[1].B * sp0) + 
	       (renPtr->edge[2].B * sp1));
    c = (int64_t)((renPtr->edge[0].C * sp2) + (renPtr->edge[1].C * sp0) + 
	       (renPtr->edge[2].C * sp1));
    renPtr->green[0] = a;
    renPtr->green[1] = b;
    renPtr->green[2] = (a * renPtr->x1) + (b * renPtr->y1) + c + 
	(1 << (FRACBITS - 1));
    sp0 = v1->color.Blue * scale;
    sp1 = v2->color.Blue * scale;
    sp2 = v3->color.Blue * scale;
    a = (int64_t)((renPtr->edge[0].A * sp2) + (renPtr->edge[1].A * sp0) + 
	       (renPtr->edge[2].A * sp1));
    b = (int64_t)((renPtr->edge[0].B * sp2) + (renPtr->edge[1].B * sp0) + 
	       (renPtr->edge[2].B * sp1));
    c = (int64_t)((renPtr->edge[0].C * sp2) + (renPtr->edge[1].C * sp0) + 
	       (renPtr->edge[2].C * sp1));
    renPtr->blue[0] = a;
    renPtr->blue[1] = b;
    renPtr->blue[2] = (a * renPtr->x1) + (b * renPtr->y1) + c + 
	(1 << (FRACBITS - 1));

    sp0 = elemPtr->z.values[v1->index] * scale;
    sp1 = elemPtr->z.values[v2->index] * scale;
    sp2 = elemPtr->z.values[v3->index] * scale;
    a = (int64_t)((renPtr->edge[0].A * sp2) + (renPtr->edge[1].A * sp0) + 
	 (renPtr->edge[2].A * sp1));
    b = (int64_t)((renPtr->edge[0].B * sp2) + (renPtr->edge[1].B * sp0) + 
	(renPtr->edge[2].B * sp1));
    c = (int64_t)((renPtr->edge[0].C * sp2) + (renPtr->edge[1].C * sp0) + 
	(renPtr->edge[2].C * sp1));
    renPtr->value[0] = a;
    renPtr->value[1] = b;
    renPtr->value[2] = (a * renPtr->x1) + (b * renPtr->y1) + c + 
	(1 << (FRACBITS - 1));
    return TRUE;
}
 
static void 
DrawTriangle(ContourElement *elemPtr, Pict *destPtr, Triangle *t, int xoff, 
	     int yoff)
{
    int64_t t0, t1, t2, ta, tr, tb, tg;
    int x, y;  
    Blt_Pixel *destRowPtr;
    TriangleRenderer ren;
#define A0	ren.edge[0].A
#define B0	ren.edge[0].B
#define A1	ren.edge[1].A
#define B1	ren.edge[1].B
#define A2	ren.edge[2].A 
#define B2	ren.edge[2].B        
#define Ar	ren.red[0]
#define Br	ren.red[1]
#define Ag	ren.green[0]
#define Bg	ren.green[1]
#define Ab	ren.blue[0]
#define Bb	ren.blue[1]
#define Aa	ren.alpha[0]
#define Ba	ren.alpha[1]
    if (!InitRenderer(elemPtr, t, &ren)) {
	return;
    }
#ifdef notdef
    fprintf(stderr, "ren.x1=%d y1=%d x2=%d y2=%d\n", 
	    ren.x1, ren.y1, ren.x2, ren.y1);
#endif
    t0 = A0 * ren.x1 + B0 * ren.y1 + ren.edge[0].C;
    t1 = A1 * ren.x1 + B1 * ren.y1 + ren.edge[1].C;
    t2 = A2 * ren.x1 + B2 * ren.y1 + ren.edge[2].C;
    tr = ren.red[2];
    tg = ren.green[2];
    tb = ren.blue[2];
    ta = ren.alpha[2];

    destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * (ren.y1-yoff));
    for (y = ren.y1; y <= ren.y2; y++) {
	int64_t e0, e1, e2;
	int64_t r, g, b, a; 
	int inside;
	Blt_Pixel *dp;

	e0 = t0, e1 = t1, e2 = t2;
	r = tr, g = tg, b = tb, a = ta;
	inside = FALSE;
	for (x = (ren.x1 - xoff), dp = destRowPtr + x; x <= (ren.x2 - xoff); 
		x++, dp++) {
	    /* all 3 edges must be >= 0 */
	    if ((e0|e1|e2) >= 0) {
		int64_t cr, cb, cg, ca;
		cr = r >> FRACBITS;
		cg = g >> FRACBITS;
		cb = b >> FRACBITS;
		ca = a >> FRACBITS;
		dp->Red   = CLAMP(cr);
		dp->Green = CLAMP(cg);
		dp->Blue  = CLAMP(cb);
		dp->Alpha = 0xFF;
		inside = TRUE;
	    } else if (inside) {
		break;
	    }
	    e0 += A0, e1 += A1, e2 += A2;
	    r += Ar, g += Ag, b += Ab, a += Aa;
	}
	t0 += B0, t1 += B1, t2 += B2;
	tr += Br, tg += Bg, tb += Bb, ta += Ba;   
	destRowPtr += destPtr->pixelsPerRow;
    }
}

#else 

typedef struct {
    double A, B, C;
} EdgeEquation;

typedef struct { 
    EdgeEquation edge[3];
    double value[3];
    int x1, x2, y1, y2;
    int xOffset, yOffset;
} TriangleRenderer;

static void
InitEdgeEquation(EdgeEquation *eq, Vertex *p, Vertex *q)
{
    double a, b, c;

    a = p->y - q->y;
    b = q->x - p->x;
    c = -0.5 * ((a * (p->x + q->x)) + (b * (p->y + q->y)));
    eq->A = a;
    eq->B = b;
    eq->C = c;
}

static void INLINE
FlipEquation(EdgeEquation *eq)
{
    eq->A = -eq->A;
    eq->B = -eq->B;
    eq->C = -eq->C;
}

static int 
InitRenderer(ContourElement *elemPtr, Triangle *t, TriangleRenderer *renPtr)
{
    Region2d exts;
    double a, b, c;
    double scale;
    double sp0, sp1, sp2;
    double area;
    Region2d bbox;
    Vertex *v1, *v2, *v3;

    v1 = elemPtr->vertices + t->a;
    v2 = elemPtr->vertices + t->b;
    v3 = elemPtr->vertices + t->c;
    memset(renPtr, 0, sizeof(TriangleRenderer));

    /* Get the triangle's bounding box. */
    bbox.left   = MIN3(v1->x, v2->x, v3->x);
    bbox.right  = MAX3(v1->x, v2->x, v3->x);
    bbox.top    = MIN3(v1->y, v2->y, v3->y);
    bbox.bottom = MAX3(v1->y, v2->y, v3->y);

    Blt_GraphExtents(elemPtr, &exts);
    /* Do a quick minmax test on the bounding box with the plot area. */
    if ((bbox.right < exts.left) || (bbox.bottom < exts.top) ||
	(bbox.top > exts.bottom) || (bbox.left > exts.right)) {
#ifdef notdef
	fprintf(stderr, "deciding not to draw triangle %d: bbox=%g %g %g %g exts=%g %g %g %g test=%d %d %d %d\n",
		t->index, bbox.left, bbox.right, bbox.top, bbox.bottom,
		exts.left, exts.right, exts.top, exts.bottom,
		(bbox.right < exts.left), (bbox.bottom < exts.top),
		(bbox.top > exts.bottom), (bbox.left > exts.right));
#endif
	return FALSE;			/* Triangle isn't visible. */
    }
    /* Clip the (possibly visible) bounding box to plot area */
    if (exts.left > bbox.left) {
	bbox.left = exts.left;
    }
    if (exts.right < bbox.right) {
	bbox.right = exts.right;
    }
    if (exts.top > bbox.top) {
	bbox.top = exts.top;
    }
    if (exts.bottom < bbox.bottom) {
	bbox.bottom = exts.bottom;
    }
    renPtr->x2 = (int64_t)(bbox.right + ((bbox.right < 0.0) ? -0.5 : 0.5));
    renPtr->x1 = (int64_t)(bbox.left + ((bbox.left < 0.0) ? -0.5 : 0.5));
    renPtr->y2 = (int64_t)(bbox.bottom + ((bbox.bottom < 0.0) ? -0.5 : 0.5));
    renPtr->y1 = (int64_t)(bbox.top + ((bbox.top < 0.0) ? -0.5 : 0.5));

    /* Compute the three edge equations */
    InitEdgeEquation(renPtr->edge + 0, v1, v2);
    InitEdgeEquation(renPtr->edge + 1, v2, v3);
    InitEdgeEquation(renPtr->edge + 2, v3, v1);
    scale = 0.0;
    /*
      Trick #1: Orient edges so that the
      triangle's interior lies within all
      of their positive half-spaces.
      
      Assuring that the area is positive
      accomplishes this
    */
    area = renPtr->edge[0].C + renPtr->edge[1].C + renPtr->edge[2].C;
    if (area == 0.0) {
	fprintf(stderr, "deciding not to draw triangle: area is 0 (%g %g %g)\n",
		renPtr->edge[0].C, renPtr->edge[1].C, renPtr->edge[2].C);
	return FALSE;			/* Degenerate triangle. */
    }
    if (area < 0.0) {
	FlipEquation(renPtr->edge + 0);
	FlipEquation(renPtr->edge + 1);
	FlipEquation(renPtr->edge + 2);
	area = -area;
    }
    if (scale <= 0.0) {
	scale = 1.0 / ((double) area);
    }
    sp0 = scale * Az;
    sp1 = scale * Bz;
    sp2 = scale * Cz;
    a = renPtr->edge[0].A*sp2 + renPtr->edge[1].A*sp0 + renPtr->edge[2].A*sp1;
    b = renPtr->edge[0].B*sp2 + renPtr->edge[1].B*sp0 + renPtr->edge[2].B*sp1;
    c = renPtr->edge[0].C*sp2 + renPtr->edge[1].C*sp0 + renPtr->edge[2].C*sp1;
    renPtr->value[0] = a;
    renPtr->value[1] = b;
    renPtr->value[2] = a * renPtr->x1 + b * renPtr->y1 + c;
    return TRUE;
}
 
static void 
DrawTriangle(ContourElement *elemPtr, Pict *destPtr, Triangle *t, int xoff, 
	     int yoff)
{
    double t0, t1, t2, tz;
    int x, y, j;  
    Blt_Pixel *destRowPtr;
    TriangleRenderer ren;
#define A0	ren.edge[0].A
#define B0	ren.edge[0].B
#define A1	ren.edge[1].A
#define B1	ren.edge[1].B
#define A2	ren.edge[2].A 
#define B2	ren.edge[2].B        
#define Ar	ren.red[0]
#define Br	ren.red[1]
#define Ag	ren.green[0]
#define Bg	ren.green[1]
#define Ab	ren.blue[0]
#define Bb	ren.blue[1]
#define Aa	ren.alpha[0]
#define Ba	ren.alpha[1]
#define Av	ren.value[0]
#define Bv	ren.value[1]
    if (elemPtr->colormapPtr == NULL) {
	return;
    }
    if (!InitRenderer(elemPtr, t, &ren)) {
	return;
    }
    t0 = A0 * ren.x1 + B0 * ren.y1 + ren.edge[0].C;
    t1 = A1 * ren.x1 + B1 * ren.y1 + ren.edge[1].C;
    t2 = A2 * ren.x1 + B2 * ren.y1 + ren.edge[2].C;
    tz = ren.value[2];
    Blt_Colormap_Init(elemPtr->colormapPtr);
    destRowPtr = destPtr->bits + (destPtr->pixelsPerRow * (ren.y1-yoff));
    for (j = 0, y = ren.y1; y <= ren.y2; y++, j++) {
	double e0, e1, e2;
	double z;
	int inside;
	Blt_Pixel *dp;

	e0 = t0, e1 = t1, e2 = t2;
	z = tz;
	inside = FALSE;
	for (x = (ren.x1 - xoff), dp = destRowPtr + x; x <= (ren.x2 - xoff); 
	     x++, dp++) {
	    /* all 3 edges must be >= 0 */
	    if ((e0 >= 0) && (e1 >= 0) && (e2 >= 0)) {
		double cz;
		
		cz = z;
		if (cz > t->max) {
		    cz = t->max;
		}
		if (cz < t->min) {
		    cz = t->min;
		}
		dp->u32 = Blt_Colormap_GetColor(elemPtr->colormapPtr, cz);
#ifdef notdef
		fprintf(stderr, "z=%.17g color=(%d %d %d %d)\n",
			cz, dp->Red, dp->Green, dp->Blue, dp->Alpha);
#endif
		dp->Alpha = 0xFF;
		inside = TRUE;
	    } else if (inside) {
		break;
	    }
	    e0 += A0, e1 += A1, e2 += A2;
	    z += Av;
	}
	t0 += B0, t1 += B1, t2 += B2;
	tz += Bv;
	destRowPtr += destPtr->pixelsPerRow;
    }
}
#endif

Pen *
Blt_CreateContourPen(Graph *graphPtr, ClassId id, Blt_HashEntry *hPtr)
{
    ContourPen *penPtr;

    penPtr = Blt_AssertCalloc(1, sizeof(ContourPen));
    penPtr->name = Blt_GetHashKey(&graphPtr->penTable, hPtr);
    penPtr->classId = id;
    penPtr->graphPtr = graphPtr;
    penPtr->hashPtr = hPtr;
    InitPen(penPtr);
    if (strcmp(penPtr->name, "activeContour") == 0) {
	penPtr->flags = ACTIVE_PEN;
    }
    Blt_SetHashValue(hPtr, penPtr);
    return (Pen *)penPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ContourElement --
 *
 *	Allocate memory and initialize methods for the new bar element.
 *
 * Results:
 *	The pointer to the newly allocated element structure is returned.
 *
 * Side effects:
 *	Memory is allocated for the bar element structure.
 *
 *---------------------------------------------------------------------------
 */
static ElementProcs contourProcs =
{
    ClosestProc,
    ConfigureProc,
    DestroyProc,
    DrawActiveProc,
    DrawProc,
    DrawSymbolProc,
    ExtentsProc,
    NULL,				/* Find the points within the search
					 * radius. */
    ActiveToPostScriptProc,
    NormalToPostScriptProc,
    SymbolToPostScriptProc,
    MapProc,
};

Element *
Blt_ContourElement(Graph *graphPtr, ClassId classId, Blt_HashEntry *hPtr)
{
    ContourElement *elemPtr;

    elemPtr = Blt_AssertCalloc(1, sizeof(ContourElement));
    elemPtr->procsPtr = &contourProcs;
    elemPtr->configSpecs = contourSpecs;
    elemPtr->obj.name = Blt_GetHashKey(&graphPtr->elements.nameTable, hPtr);
    Blt_GraphSetObjectClass(&elemPtr->obj, classId);
    elemPtr->flags = SCALE_SYMBOL;
    elemPtr->obj.graphPtr = graphPtr;
    /* By default an element's name and label are the same. */
    elemPtr->label = Blt_AssertStrdup(elemPtr->obj.name);
    elemPtr->legendRelief = TK_RELIEF_FLAT;
    elemPtr->builtinPenPtr = &elemPtr->builtinPen;
    InitPen(elemPtr->builtinPenPtr);
    elemPtr->builtinPenPtr->graphPtr = graphPtr;
    elemPtr->builtinPenPtr->classId = classId;
    elemPtr->penPtr = elemPtr->builtinPenPtr;
    elemPtr->hashPtr = hPtr;
    elemPtr->boundaryPenPtr = &elemPtr->builtinPen;
    elemPtr->flags |= COLORMAP | ISOLINES | HULL;
    elemPtr->opacity = 100.0;
    Blt_SetHashValue(hPtr, elemPtr);
    Blt_InitHashTable(&elemPtr->isoTable, BLT_STRING_KEYS);
    elemPtr->painter = Blt_GetPainter(graphPtr->tkwin, 1.0);
    return (Element *)elemPtr;
}

static int 
ComputeColorBarGeometry(Graph *graphPtr, ContourElement *elemPtr, 
			int width, int height) 
{
    Axis *axisPtr;
    int right, left, top, bottom;

    axisPtr = elemPtr->zAxisPtr;
    /* 
     * horizontal color bar
     *
     *	min ---------------------- max 
     *      |  |   |   |    |    |
     */
    Blt_GetAxisGeometry(graphPtr, axisPtr);
    right = left = top = bottom = 0;
    if (!Blt_Legend_IsHidden(graphPtr)) {
	switch (Blt_Legend_Site(graphPtr)) {
	case LEGEND_RIGHT:
	    right += Blt_Legend_Width(graphPtr) + 2;
	    break;
	case LEGEND_LEFT:
	    left += Blt_Legend_Width(graphPtr) + 2;
	    break;
	case LEGEND_TOP:
	    top += Blt_Legend_Height(graphPtr) + 2;
	    break;
	case LEGEND_BOTTOM:
	    bottom += Blt_Legend_Height(graphPtr) + 2;
	    break;
	case LEGEND_XY:
	case LEGEND_PLOT:
	case LEGEND_WINDOW:
	    /* Do nothing. */
	    break;
	}
    }
#ifdef notdef
    fprintf(stderr, "legend site=%d, x=%d y=%d w=%d h=%d\n", 
	    Blt_Legend_Site(graphPtr), 
	    Blt_Legend_X(graphPtr), 
	    Blt_Legend_Y(graphPtr), 
	    Blt_Legend_Width(graphPtr), 
	    Blt_Legend_Height(graphPtr));
#endif
    return TCL_OK;
}

#ifdef notdef
static int 
DrawColorBar(int width, int height) 
{
    /*  */
    return TCL_OK;
}
#endif

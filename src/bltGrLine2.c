/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrLine2.c --
 *
 * This module implements line graph and stripchart elements for the BLT
 * graph widget.
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

#include <X11/Xutil.h>
#include "bltMath.h"
#include "bltBind.h"
#include "bltPs.h"
#include "bltBg.h"
#include "tkDisplay.h"
#include "bltImage.h"
#include "bltPalette.h"
#include "bltPicture.h"
#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"
#include "bltGrElem.h"
#include "bltPainter.h"

#define JCLAMP(c)       ((((c) < 0.0) ? 0.0 : ((c) > 1.0) ? 1.0 : (c)))

#define SQRT_PI         1.77245385090552
#define S_RATIO         0.886226925452758

/* Trace flags. */
#define RECOUNT         (1<<10)         /* Trace needs to be fixed. */

/* Flags for trace's point and segments. */
#define VISIBLE         (1<<0)          /* Point is on visible on screen. */
#define KNOT            (1<<1)          /* Point is a knot, original data
                                         * point. */
#define SYMBOL          (1<<2)          /* Point is designated to have a
                                         * symbol. This is only used when
                                         * reqMaxSymbols is non-zero. */
#define ACTIVE_POINT    (1<<3)          /* Point is active. This is only used
                                         * when numActiveIndices is greater
                                         * than zero. */
/* Flags describing visibility of error bars. */
#define XLOW            (1<<6)          /* Segment is part of the low x-value
                                         * error bar. */
#define XHIGH           (1<<7)          /* Segment is part of the high x-value
                                         * error bar. */
#define XERROR          (XHIGH | XLOW)  /* Display both low and high error bars 
                                         * for x-coordinates. */
#define YLOW            (1<<8)          /* Segment is part of the low y-value
                                         * error bar. */
#define YHIGH           (1<<9)          /* Segment is part of the high y-value
                                         * error bar. */
#define YERROR          (YHIGH | YLOW)  /* Display both low and high error
                                         * bars for y-coordinates. */

#define NOTPLAYING(g,i) \
    (((g)->play.enabled) && (((i) < (g)->play.t1) || ((i) > (g)->play.t2)))

#define PLAYING(t,i) \
    ((!(t)->elemPtr->obj.graphPtr->play.enabled) || \
     (((i) >= (t)->elemPtr->obj.graphPtr->play.t1) && \
      ((i) <= (t)->elemPtr->obj.graphPtr->play.t2)))

#define DRAWN(t,f)     (((f) & (t)->drawFlags) == (t)->drawFlags)

#define BROKEN_TRACE(dir,last,next) \
    (((((dir) & PEN_DECREASING) == 0) && ((next) < (last))) || \
     ((((dir) & PEN_INCREASING) == 0) && ((next) > (last))))

/*
 * XDrawLines() points: XMaxRequestSize(dpy) - 3
 * XFillPolygon() points:  XMaxRequestSize(dpy) - 4
 * XDrawSegments() segments:  (XMaxRequestSize(dpy) - 3) / 2
 * XDrawRectangles() rectangles:  (XMaxRequestSize(dpy) - 3) / 2
 * XFillRectangles() rectangles:  (XMaxRequestSize(dpy) - 3) / 2
 * XDrawArcs() or XFillArcs() arcs:  (XMaxRequestSize(dpy) - 3) / 3
 */

#define MAX_DRAWPOINTS(d)       Blt_MaxRequestSize(d, sizeof(XPoint))
#define MAX_DRAWLINES(d)        Blt_MaxRequestSize(d, sizeof(XPoint))
#define MAX_DRAWPOLYGON(d)      Blt_MaxRequestSize(d, sizeof(XPoint))
#define MAX_DRAWSEGMENTS(d)     Blt_MaxRequestSize(d, sizeof(XSegment))
#define MAX_DRAWRECTANGLES(d)   Blt_MaxRequestSize(d, sizeof(XRectangle))
#define MAX_DRAWARCS(d)         Blt_MaxRequestSize(d, sizeof(XArc))

#define COLOR_DEFAULT   (XColor *)1
#define PATTERN_SOLID   ((Pixmap)1)

#define PEN_INCREASING  1               /* Draw line segments for only
                                         * those data points whose
                                         * abscissas are monotonically
                                         * increasing in order. */
#define PEN_DECREASING  2               /* Lines will be drawn between only
                                         * those points whose abscissas are
                                         * decreasing in order. */

#define PEN_BOTH_DIRECTIONS     (PEN_INCREASING | PEN_DECREASING)

/* Lines will be drawn between points regardless of the ordering of the
 * abscissas */


#define SMOOTH_NONE             0       /* Line segments */
#define SMOOTH_STEP             1       /* Step-and-hold */
#define SMOOTH_NATURAL          2       /* Natural cubic spline */
#define SMOOTH_QUADRATIC        3       /* Quadratic spline */
#define SMOOTH_CATROM           4       /* Catrom spline */

#define SMOOTH_PARAMETRIC       8       /* Parametric spline */

typedef struct {
    const char *name;
    int flags;
} SmoothingTable;

static SmoothingTable smoothingTable[] = {
    { "none",                   SMOOTH_NONE                             },
    { "linear",                 SMOOTH_NONE                             },
    { "step",                   SMOOTH_STEP                             },
    { "natural",                SMOOTH_NATURAL                          },
    { "cubic",                  SMOOTH_NATURAL                          },
    { "quadratic",              SMOOTH_QUADRATIC                        },
    { "catrom",                 SMOOTH_CATROM                           },
    { "parametriccubic",        SMOOTH_NATURAL | SMOOTH_PARAMETRIC      },
    { "parametricquadratic",    SMOOTH_QUADRATIC | SMOOTH_PARAMETRIC    },
    { (char *)NULL,             0                                       }
};

/* Symbol types for line elements */
typedef enum {
    SYMBOL_NONE,                        /*  0 */
    SYMBOL_SQUARE,                      /*  1 */
    SYMBOL_CIRCLE,                      /*  2 */
    SYMBOL_DIAMOND,                     /*  3 */
    SYMBOL_PLUS,                        /*  4 */
    SYMBOL_CROSS,                       /*  5 */
    SYMBOL_SPLUS,                       /*  6 */
    SYMBOL_SCROSS,                      /*  7 */
    SYMBOL_TRIANGLE,                    /*  8 */
    SYMBOL_ARROW,                       /*  9 */
    SYMBOL_IMAGE                        /* 10 */
} SymbolType;

typedef struct {
    const char *name;
    int minChars;
    SymbolType type;
} SymbolTable;

static SymbolTable symbolTable[] = {
    { "arrow",    1, SYMBOL_ARROW,      },
    { "circle",   2, SYMBOL_CIRCLE,     },
    { "cross",    2, SYMBOL_CROSS,      }, 
    { "diamond",  1, SYMBOL_DIAMOND,    }, 
    { "image",    0, SYMBOL_IMAGE,      }, 
    { "none",     1, SYMBOL_NONE,       }, 
    { "plus",     1, SYMBOL_PLUS,       }, 
    { "scross",   2, SYMBOL_SCROSS,     }, 
    { "splus",    2, SYMBOL_SPLUS,      }, 
    { "square",   2, SYMBOL_SQUARE,     }, 
    { "triangle", 1, SYMBOL_TRIANGLE,   }, 
    { NULL,       0, 0                  }, 
};

typedef struct _LineElement LineElement;

typedef struct {
    SymbolType type;                    /* Type of symbol to be
                                         * drawn/printed */
    int size;                           /* Requested size of symbol in
                                         * pixels */
    XColor *outlineColor;               /* Outline color */
    int outlineWidth;                   /* Width of the outline */
    GC outlineGC;                       /* Outline graphics context */
    XColor *fillColor;                  /* Normal fill color */
    GC fillGC;                          /* Fill graphics context */
    Tk_Image image;                     /* This is used of image symbols.  */
} Symbol;

typedef struct {
    const char *name;                   /* Pen style identifier.  If NULL
                                         * pen was statically allocated. */
    ClassId classId;                    /* Type of pen */
    const char *typeId;                 /* String token identifying the
                                         * type of pen. */
    unsigned int flags;                 /* Indicates if the pen element is
                                         * active or normal */
    int refCount;                       /* Reference count for elements
                                         * using this pen. */
    Blt_HashEntry *hashPtr;
    Blt_ConfigSpec *configSpecs;        /* Configuration specifications */
    PenConfigureProc *configProc;
    PenDestroyProc *destroyProc;
    Graph *graphPtr;                    /* Graph that the pen is associated
                                         * with. */

    /* Symbol attributes. */
    Symbol symbol;                      /* Element symbol type */

    /* Trace attributes. */
    Blt_Dashes traceDashes;             /* Dash on-off list value */
    XColor *traceColor;                 /* Line segment color */
    XColor *traceOffColor;              /* Line segment dash gap color */
    GC traceGC;                         /* Line segment graphics context */
    int traceWidth;                     /* Width of the line segments. If
                                         * lineWidth is 0, no line will be
                                         * drawn, only symbols. */

    /* Error bar attributes. */
    unsigned int errorFlags;            /* Indicates error bars to display. */
    int errorLineWidth;                 /* Width of the error bar segments. */
    int errorCapWidth;                  /* Width of the cap on error bars. */
    XColor *errorColor;                 /* Color of the error bar. */
    GC errorGC;                         /* Error bar graphics context. */

    /* Show value attributes. */
    unsigned int valueFlags;            /* Indicates whether to display
                                         * text of the data value.  Values
                                         * are x, y, both, or none. */
    const char *valueFormat;            /* A printf format string. */
    TextStyle valueStyle;               /* Text attributes (color, font,
                                         * rotation, etc.) of the value. */
} LinePen;

/* 
 * A TraceSegment represents the individual line segment (which is part of
 * an error bar) in a trace.  Included is the both the index of the data
 * point it is associated with and the flags or the point (if it's active,
 * etc).
 */
typedef struct _TraceSegment {
    struct _TraceSegment *next;         /* Pointer to next point in trace. */
    float x1, y1, x2, y2;               /* Screen coordinate of the point. */
    int index;                          /* Index of this coordinate
                                         * pointing back to the raw world
                                         * values in the individual data
                                         * arrays. This index is replicated
                                         * for generated values. */
    unsigned int flags;                 /* Flags associated with a segment
                                         * are described below. */
} TraceSegment;

/* 
 * A TracePoint represents the individual point in a trace. 
 */
typedef struct _TracePoint {
    struct _TracePoint *next;           /* Pointer to next point in trace. */
    float x, y;                         /* Screen coordinate of the point. */
    int index;                          /* Index of this coordinate
                                         * pointing back to the raw world
                                         * values in the individual data
                                         * arrays. This index is replicated
                                         * for generated values. */
    unsigned int flags;                 /* Flags associated with a point
                                         * are described below. */
} TracePoint;


/* 
 * A trace represents a polyline of connected line segments using the same
 * line and symbol style.  They are stored in a chain of traces.
 */
typedef struct _Trace {
    LineElement *elemPtr;
    TracePoint *head, *tail;
    int numPoints;                      /* # of points in the trace. */
    Blt_ChainLink link;
    LinePen *penPtr;
    unsigned short flags;               /* Flags associated with a trace
                                         * are described blow. */
    unsigned short drawFlags;           /* Flags for individual points and
                                         * segments when drawing the
                                         * trace. */
    TraceSegment *segments;             /* Segments used for errorbars. */
    int numSegments;
    Point2d *fillPts;                   /* Polygon representing the area
                                         * under the curve.  May be a
                                         * degenerate polygon. */
    int numFillPts;
} Trace;

typedef struct {
    Weight weight;                      /* Weight range where this pen is
                                         * valid. */
    LinePen *penPtr;                    /* Pen to use. */
} LineStyle;

struct _LineElement {
    GraphObj obj;                       /* Must be first field in element. */
    unsigned int flags;         
    Blt_HashEntry *hashPtr;

    /* Fields specific to elements. */
    Blt_ChainLink link;                 /* Element's link in display list. */
    const char *label;                  /* Label displayed in legend */
    unsigned short row, col;            /* Position of the entry in the
                                         * legend. */
    int legendRelief;                   /* Relief of label in legend. */
    Axis2d axes;                        /* X-axis and Y-axis mapping the
                                         * element */
    ElemValues x, y, w;                 /* Contains array of floating point
                                         * graph coordinate values. Also
                                         * holds min/max * and the number
                                         * of coordinates */
    Blt_HashTable activeTable;          /* Table of indices which indicate
                                         * which data points are active
                                         * (drawn * with "active"
                                         * colors). */
    int numActiveIndices;               /* Number of active data points.
                                         * Special case: if
                                         * numActiveIndices < 0 and the
                                         * active bit is set in "flags",
                                         * then all data points are drawn
                                         * active. */
    ElementProcs *procsPtr;
    Blt_ConfigSpec *configSpecs;        /* Configuration specifications. */
    LinePen *activePenPtr;              /* Standard Pens */
    LinePen *normalPenPtr;
    LinePen *builtinPenPtr;
    Blt_Chain styles;                   /* Palette of pens. */

    /* Symbol scaling */
    int scaleSymbols;                   /* If non-zero, the symbols will scale
                                         * in size as the graph is zoomed
                                         * in/out.  */

    double xRange, yRange;              /* Initial X-axis and Y-axis ranges:
                                         * used to scale the size of element's
                                         * symbol. */
    int state;
    Blt_HashTable isoTable;             /* Table of isolines to be
                                         * displayed. */

    /* Line-specific fields. */
    ElemValues xError;                  /* Relative/symmetric X error
                                         * values. */
    ElemValues yError;                  /* Relative/symmetric Y error
                                         * values. */
    ElemValues xHigh, xLow;             /* Absolute/asymmetric X-coordinate
                                         * high/low error values. */
    ElemValues yHigh, yLow;             /* Absolute/asymmetric Y-coordinate
                                         * high/low error values. */
    LinePen builtinPen;

    /* Line smoothing */
    unsigned int reqSmooth;             /* Requested smoothing function to
                                         * use for connecting the data
                                         * points */
    unsigned int smooth;                /* Smoothing function used. */
    float rTolerance;                   /* Tolerance to reduce the number
                                         * of points displayed. */

    /* Drawing-related data structures. */

    /* Area-under-curve fill attributes. */
    XColor *fillFgColor;
    XColor *fillBgColor;
    GC fillGC;

    Blt_Bg areaBg;                      /* Background representing the
                                         * color of the area under the
                                         * curve. */
    Blt_PaintBrush brush;
    
    int reqMaxSymbols;                  /* Indicates the interval the draw
                                         * symbols.  Zero (and one) means
                                         * draw all symbols. */
    int penDir;                         /* Indicates if a change in the pen
                                         * direction should be considered a
                                         * retrace (line segment is not
                                         * drawn). */
    Blt_Chain traces;                   /* List of traces (a trace is a
                                         * series of contiguous line
                                         * segments).  New traces are
                                         * generated when either the next
                                         * segment changes the pen
                                         * direction, or the end point is
                                         * clipped by the plotting area. */
    Blt_Pool pointPool;
    Blt_Pool segmentPool;
    Axis *zAxisPtr;
};

static Blt_OptionParseProc ObjToSmoothProc;
static Blt_OptionPrintProc SmoothToObjProc;
static Blt_CustomOption smoothOption =
{
    ObjToSmoothProc, SmoothToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToPenDirProc;
static Blt_OptionPrintProc PenDirToObjProc;
static Blt_CustomOption penDirOption =
{
    ObjToPenDirProc, PenDirToObjProc, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToErrorBarsProc;
static Blt_OptionPrintProc ErrorBarsToObjProc;
static Blt_CustomOption errorbarsOption =
{
    ObjToErrorBarsProc, ErrorBarsToObjProc, NULL, (ClientData)0
};

static Blt_OptionFreeProc FreeSymbolProc;
static Blt_OptionParseProc ObjToSymbolProc;
static Blt_OptionPrintProc SymbolToObjProc;
static Blt_CustomOption symbolOption =
{
    ObjToSymbolProc, SymbolToObjProc, FreeSymbolProc, (ClientData)0
};

static Blt_OptionFreeProc FreeBackground;
static Blt_OptionParseProc ObjToBackground;
static Blt_OptionPrintProc BackgroundToObj;
static Blt_CustomOption backgroundOption = {
    ObjToBackground, BackgroundToObj, FreeBackground, (ClientData)0
};

BLT_EXTERN Blt_CustomOption bltAxisOption;
BLT_EXTERN Blt_CustomOption bltColorOption;
BLT_EXTERN Blt_CustomOption bltElementTagsOption;
BLT_EXTERN Blt_CustomOption bltLinePenOption;
BLT_EXTERN Blt_CustomOption bltLineStylesOption;
BLT_EXTERN Blt_CustomOption bltValuePairsOption;
BLT_EXTERN Blt_CustomOption bltValuesOption;
BLT_EXTERN Blt_CustomOption bltXAxisOption;
BLT_EXTERN Blt_CustomOption bltYAxisOption;

#define DEF_ACTIVE_PEN          "activeLine"
#define DEF_AXIS_X              "x"
#define DEF_AXIS_Y              "y"
#define DEF_DATA                (char *)NULL
#define DEF_FILL_COLOR          "defcolor"
#define DEF_HIDE                "no"
#define DEF_LABEL               (char *)NULL
#define DEF_LABEL_RELIEF        "flat"
#define DEF_MAX_SYMBOLS         "0"
#define DEF_PATTERN_BG          (char *)NULL
#define DEF_PATTERN_FG          "black"
#define DEF_COLORMAP            (char *)NULL
#define DEF_REDUCE              "0.0"
#define DEF_SCALE_SYMBOLS       "yes"
#define DEF_SMOOTH              "linear"
#define DEF_STATE               "normal"
#define DEF_STIPPLE             (char *)NULL
#define DEF_STYLES              ""
#define DEF_SYMBOL              "circle"
#define DEF_TAGS                "all"
#define DEF_X_DATA              (char *)NULL
#define DEF_Y_DATA              (char *)NULL
#define DEF_PEN_ACTIVE_COLOR    RGB_BLUE
#define DEF_PEN_COLOR           RGB_NAVYBLUE
#define DEF_PEN_DASHES          (char *)NULL
#define DEF_PEN_DASHES          (char *)NULL
#define DEF_PEN_DIRECTION       "both"

#define DEF_PEN_ERRORBARS               "both"
#define DEF_PEN_ERRORBAR_CAPWIDTH       "0"
#define DEF_PEN_ERRORBAR_COLOR          "defcolor"
#define DEF_PEN_ERRORBAR_LINEWIDTH      "1"

#define DEF_PEN_FILL_COLOR              "defcolor"
#define DEF_PEN_LINEWIDTH               "1"
#define DEF_PEN_NORMAL_COLOR            RGB_NAVYBLUE
#define DEF_PEN_OFFDASH_COLOR           (char *)NULL
#define DEF_PEN_OUTLINE_COLOR           "defcolor"
#define DEF_PEN_OUTLINE_WIDTH           "1"
#define DEF_PEN_PIXELS                  "0.1i"
#define DEF_PEN_SHOW_VALUES             "no"
#define DEF_PEN_SYMBOL                  "circle"
#define DEF_PEN_TYPE                    "line"
#define DEF_PEN_VALUE_ANCHOR            "s"
#define DEF_PEN_VALUE_ANGLE             (char *)NULL
#define DEF_PEN_VALUE_COLOR             RGB_BLACK
#define DEF_PEN_VALUE_FONT              STD_FONT_NUMBERS
#define DEF_PEN_VALUE_FORMAT            "%g"

static Blt_ConfigSpec lineSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-activepen", "activePen", "ActivePen",
        DEF_ACTIVE_PEN, Blt_Offset(LineElement, activePenPtr),
        BLT_CONFIG_NULL_OK, &bltLinePenOption},
    {BLT_CONFIG_COLOR, "-areaforeground", "areaForeground", "AreaForeground",
        DEF_PATTERN_FG, Blt_Offset(LineElement, fillFgColor), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-areabackground", "areaBackground", 
        "AreaBackground", DEF_PATTERN_BG, 0, BLT_CONFIG_NULL_OK,
        &backgroundOption},
    {BLT_CONFIG_SYNONYM, "-bindtags", "tags" },
    {BLT_CONFIG_COLOR, "-color", "color", "Color", DEF_PEN_COLOR, 
        Blt_Offset(LineElement, builtinPen.traceColor), 0},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes", DEF_PEN_DASHES, 
        Blt_Offset(LineElement, builtinPen.traceDashes), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-data", "data", "Data", DEF_DATA, 0, 0, 
        &bltValuePairsOption},
    {BLT_CONFIG_CUSTOM, "-errorbars", "errorBars", "ErrorBars",
        DEF_PEN_ERRORBARS, Blt_Offset(LineElement, builtinPen.errorFlags), 
        BLT_CONFIG_DONT_SET_DEFAULT, &errorbarsOption},
    {BLT_CONFIG_CUSTOM, "-errorbarcolor", "errorBarColor", "ErrorBarColor",
        DEF_PEN_ERRORBAR_COLOR, 
        Blt_Offset(LineElement, builtinPen.errorColor), 0, &bltColorOption},
    {BLT_CONFIG_PIXELS_NNEG,"-errorbarlinewidth", "errorBarLineWidth", 
        "ErrorBarLineWidth", DEF_PEN_ERRORBAR_LINEWIDTH, 
        Blt_Offset(LineElement, builtinPen.errorLineWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-errorbarcapwith", "errorBarCapWidth", 
        "ErrorBarCapWidth", DEF_PEN_ERRORBAR_CAPWIDTH, 
        Blt_Offset(LineElement, builtinPen.errorCapWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-fill", "fill", "Fill", DEF_PEN_FILL_COLOR, 
        Blt_Offset(LineElement, builtinPen.symbol.fillColor), 
        BLT_CONFIG_NULL_OK, &bltColorOption},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_HIDE, 
        Blt_Offset(LineElement, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_STRING, "-label", "label", "Label", (char *)NULL, 
        Blt_Offset(LineElement, label), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-legendrelief", "legendRelief", "LegendRelief",
        DEF_LABEL_RELIEF, Blt_Offset(LineElement, legendRelief),
        BLT_CONFIG_DONT_SET_DEFAULT}, 
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth",
        DEF_PEN_LINEWIDTH, Blt_Offset(LineElement, builtinPen.traceWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX",
        DEF_AXIS_X, Blt_Offset(LineElement, axes.x), 0, &bltXAxisOption},
    {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY",
        DEF_AXIS_Y, Blt_Offset(LineElement, axes.y), 0, &bltYAxisOption},
    {BLT_CONFIG_INT_NNEG, "-maxsymbols", "maxSymbols", "MaxSymbols",
        DEF_MAX_SYMBOLS, Blt_Offset(LineElement, reqMaxSymbols),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-offdash", "offDash", "OffDash", 
        DEF_PEN_OFFDASH_COLOR, 
        Blt_Offset(LineElement, builtinPen.traceOffColor),
        BLT_CONFIG_NULL_OK, &bltColorOption},
    {BLT_CONFIG_CUSTOM, "-outline", "outline", "Outline", 
        DEF_PEN_OUTLINE_COLOR, 
        Blt_Offset(LineElement, builtinPen.symbol.outlineColor), 
        0, &bltColorOption},
    {BLT_CONFIG_PIXELS_NNEG, "-outlinewidth", "outlineWidth", "OutlineWidth",
        DEF_PEN_OUTLINE_WIDTH, 
        Blt_Offset(LineElement, builtinPen.symbol.outlineWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-pen", "pen", "Pen", (char *)NULL, 
        Blt_Offset(LineElement, normalPenPtr), BLT_CONFIG_NULL_OK, 
        &bltLinePenOption},
    {BLT_CONFIG_CUSTOM, "-colormap", "colormap", "Colormap", DEF_COLORMAP, 
        Blt_Offset(LineElement, zAxisPtr), 0, &bltAxisOption},
    {BLT_CONFIG_PIXELS_NNEG, "-pixels", "pixels", "Pixels", DEF_PEN_PIXELS, 
        Blt_Offset(LineElement, builtinPen.symbol.size), GRAPH | STRIPCHART}, 
    {BLT_CONFIG_FLOAT, "-reduce", "reduce", "Reduce",
        DEF_REDUCE, Blt_Offset(LineElement, rTolerance),
        GRAPH | STRIPCHART | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-scalesymbols", "scaleSymbols", "ScaleSymbols",
        DEF_SCALE_SYMBOLS, Blt_Offset(LineElement, scaleSymbols),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FILL, "-showvalues", "showValues", "ShowValues",
        DEF_PEN_SHOW_VALUES, Blt_Offset(LineElement, builtinPen.valueFlags),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-smooth", "smooth", "Smooth", DEF_SMOOTH, 
        Blt_Offset(LineElement, reqSmooth), BLT_CONFIG_DONT_SET_DEFAULT, 
        &smoothOption},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_STATE, 
        Blt_Offset(LineElement, state), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-styles", "styles", "Styles", DEF_STYLES, 
        Blt_Offset(LineElement, styles), 0, &bltLineStylesOption},
    {BLT_CONFIG_CUSTOM, "-symbol", "symbol", "Symbol", DEF_PEN_SYMBOL, 
        Blt_Offset(LineElement, builtinPen.symbol), 
        BLT_CONFIG_DONT_SET_DEFAULT, &symbolOption},
    {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_TAGS, 0,
        BLT_CONFIG_NULL_OK, &bltElementTagsOption},
    {BLT_CONFIG_CUSTOM, "-trace", "trace", "Trace", DEF_PEN_DIRECTION, 
        Blt_Offset(LineElement, penDir), 
        BLT_CONFIG_DONT_SET_DEFAULT, &penDirOption},
    {BLT_CONFIG_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
        DEF_PEN_VALUE_ANCHOR, 
        Blt_Offset(LineElement, builtinPen.valueStyle.anchor), 0},
    {BLT_CONFIG_COLOR, "-valuecolor", "valueColor", "ValueColor",
        DEF_PEN_VALUE_COLOR, 
        Blt_Offset(LineElement, builtinPen.valueStyle.color), 0},
    {BLT_CONFIG_FONT, "-valuefont", "valueFont", "ValueFont",
        DEF_PEN_VALUE_FONT, 
        Blt_Offset(LineElement, builtinPen.valueStyle.font), 0},
    {BLT_CONFIG_STRING, "-valueformat", "valueFormat", "ValueFormat",
        DEF_PEN_VALUE_FORMAT, Blt_Offset(LineElement, builtinPen.valueFormat),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FLOAT, "-valuerotate", "valueRotate", "ValueRotate",
        DEF_PEN_VALUE_ANGLE, 
        Blt_Offset(LineElement, builtinPen.valueStyle.angle), 0},
    {BLT_CONFIG_CUSTOM, "-weights", "weights", "Weights", (char *)NULL, 
        Blt_Offset(LineElement, w), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-x", "xData", "XData", (char *)NULL, 
        Blt_Offset(LineElement, x), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-xdata", "xData", "XData", (char *)NULL, 
        Blt_Offset(LineElement, x), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-xerror", "xError", "XError", (char *)NULL, 
        Blt_Offset(LineElement, xError), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-xhigh", "xHigh", "XHigh", (char *)NULL, 
        Blt_Offset(LineElement, xHigh), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-xlow", "xLow", "XLow", (char *)NULL, 
        Blt_Offset(LineElement, xLow), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-y", "yData", "YData", (char *)NULL, 
        Blt_Offset(LineElement, y), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-ydata", "yData", "YData", (char *)NULL, 
        Blt_Offset(LineElement, y), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-yerror", "yError", "YError", (char *)NULL, 
        Blt_Offset(LineElement, yError), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-yhigh", "yHigh", "YHigh", (char *)NULL, 
        Blt_Offset(LineElement, yHigh), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-ylow", "yLow", "YLow", (char *)NULL, 
        Blt_Offset(LineElement, yLow), 0, &bltValuesOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec stripSpecs[] =
{
    {BLT_CONFIG_CUSTOM, "-activepen", "activePen", "ActivePen",
        DEF_ACTIVE_PEN, Blt_Offset(LineElement, activePenPtr), 
        BLT_CONFIG_NULL_OK, &bltLinePenOption},
    {BLT_CONFIG_COLOR, "-areaforeground", "areaForeground", "areaForeground",
        DEF_PATTERN_FG, Blt_Offset(LineElement, fillFgColor), 
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-areabackground", "areaBackground", 
        "AreaBackground", DEF_PATTERN_BG, 0, BLT_CONFIG_NULL_OK,
        &backgroundOption},
    {BLT_CONFIG_SYNONYM, "-bindtags", "tags" },
    {BLT_CONFIG_COLOR, "-color", "color", "Color",
        DEF_PEN_COLOR, Blt_Offset(LineElement, builtinPen.traceColor), 0},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes", DEF_PEN_DASHES, 
        Blt_Offset(LineElement, builtinPen.traceDashes), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-data", "data", "Data", DEF_DATA, 0, 0, 
        &bltValuePairsOption},
    {BLT_CONFIG_CUSTOM, "-errorbars", "errorBars", "ErrorBars",
        DEF_PEN_ERRORBARS, Blt_Offset(LineElement, builtinPen.errorFlags), 
        BLT_CONFIG_DONT_SET_DEFAULT, &errorbarsOption},
    {BLT_CONFIG_CUSTOM, "-errorbarcolor", "errorBarColor", "ErrorBarColor",
        DEF_PEN_ERRORBAR_COLOR, 
        Blt_Offset(LineElement, builtinPen.errorColor), 0, &bltColorOption},
    {BLT_CONFIG_PIXELS_NNEG, "-errorbarlinewidth", "errorBarLineWidth", 
        "ErrorBarLineWidth", DEF_PEN_ERRORBAR_LINEWIDTH, 
        Blt_Offset(LineElement, builtinPen.errorLineWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-errorbarcapwidth", "errorBarCapWidth", 
        "ErrorBarCapWidth", DEF_PEN_ERRORBAR_CAPWIDTH, 
        Blt_Offset(LineElement, builtinPen.errorCapWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-fill", "fill", "Fill", DEF_PEN_FILL_COLOR, 
        Blt_Offset(LineElement, builtinPen.symbol.fillColor),
        BLT_CONFIG_NULL_OK, &bltColorOption},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_HIDE, 
        Blt_Offset(LineElement, flags), BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_STRING, "-label", "label", "Label", (char *)NULL, 
        Blt_Offset(LineElement, label), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-legendrelief", "legendRelief", "LegendRelief",
        DEF_LABEL_RELIEF, Blt_Offset(LineElement, legendRelief),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth", 
        DEF_PEN_LINEWIDTH, Blt_Offset(LineElement, builtinPen.traceWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-mapx", "mapX", "MapX", DEF_AXIS_X, 
        Blt_Offset(LineElement, axes.x), 0, &bltXAxisOption},
    {BLT_CONFIG_CUSTOM, "-mapy", "mapY", "MapY", DEF_AXIS_Y, 
        Blt_Offset(LineElement, axes.y), 0, &bltYAxisOption},
    {BLT_CONFIG_INT_NNEG, "-maxsymbols", "maxSymbols", "MaxSymbols",
        DEF_MAX_SYMBOLS, Blt_Offset(LineElement, reqMaxSymbols),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-offdash", "offDash", "OffDash", 
        DEF_PEN_OFFDASH_COLOR, 
        Blt_Offset(LineElement, builtinPen.traceOffColor),
        BLT_CONFIG_NULL_OK, &bltColorOption},
    {BLT_CONFIG_CUSTOM, "-outline", "outline", "Outline",
        DEF_PEN_OUTLINE_COLOR, 
        Blt_Offset(LineElement, builtinPen.symbol.outlineColor), 0, 
        &bltColorOption},
    {BLT_CONFIG_PIXELS_NNEG, "-outlinewidth", "outlineWidth", "OutlineWidth",
        DEF_PEN_OUTLINE_WIDTH, 
        Blt_Offset(LineElement, builtinPen.symbol.outlineWidth),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-pen", "pen", "Pen", (char *)NULL, 
        Blt_Offset(LineElement, normalPenPtr), BLT_CONFIG_NULL_OK, 
        &bltLinePenOption},
    {BLT_CONFIG_PIXELS_NNEG, "-pixels", "pixels", "Pixels", DEF_PEN_PIXELS, 
        Blt_Offset(LineElement, builtinPen.symbol.size), 0},
    {BLT_CONFIG_BOOLEAN, "-scalesymbols", "scaleSymbols", "ScaleSymbols",
        DEF_SCALE_SYMBOLS, Blt_Offset(LineElement, scaleSymbols),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FILL, "-showvalues", "showValues", "ShowValues",
        DEF_PEN_SHOW_VALUES, Blt_Offset(LineElement, builtinPen.valueFlags),
        BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-smooth", "smooth", "Smooth", DEF_SMOOTH, 
        Blt_Offset(LineElement, reqSmooth), BLT_CONFIG_DONT_SET_DEFAULT, 
        &smoothOption},
    {BLT_CONFIG_CUSTOM, "-styles", "styles", "Styles", DEF_STYLES, 
        Blt_Offset(LineElement, styles), 0, &bltLineStylesOption},
    {BLT_CONFIG_CUSTOM, "-symbol", "symbol", "Symbol", DEF_PEN_SYMBOL, 
        Blt_Offset(LineElement, builtinPen.symbol), 
        BLT_CONFIG_DONT_SET_DEFAULT, &symbolOption},
    {BLT_CONFIG_CUSTOM, "-tags", "tags", "Tags", DEF_TAGS, 0,
        BLT_CONFIG_NULL_OK, &bltElementTagsOption},
    {BLT_CONFIG_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
        DEF_PEN_VALUE_ANCHOR, 
        Blt_Offset(LineElement, builtinPen.valueStyle.anchor), 0},
    {BLT_CONFIG_COLOR, "-valuecolor", "valueColor", "ValueColor",
        DEF_PEN_VALUE_COLOR, 
        Blt_Offset(LineElement, builtinPen.valueStyle.color), 0},
    {BLT_CONFIG_FONT, "-valuefont", "valueFont", "ValueFont",
        DEF_PEN_VALUE_FONT, 
        Blt_Offset(LineElement, builtinPen.valueStyle.font), 0},
    {BLT_CONFIG_STRING, "-valueformat", "valueFormat", "ValueFormat",
        DEF_PEN_VALUE_FORMAT, Blt_Offset(LineElement, builtinPen.valueFormat),
        BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FLOAT, "-valuerotate", "valueRotate", "ValueRotate",
        DEF_PEN_VALUE_ANGLE, 
        Blt_Offset(LineElement, builtinPen.valueStyle.angle),0},
    {BLT_CONFIG_CUSTOM, "-weights", "weights", "Weights", (char *)NULL, 
        Blt_Offset(LineElement, w), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-x", "xData", "XData", (char *)NULL, 
        Blt_Offset(LineElement, x), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-xdata", "xData", "XData", (char *)NULL, 
        Blt_Offset(LineElement, x), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-y", "yData", "YData", (char *)NULL, 
        Blt_Offset(LineElement, y), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-xerror", "xError", "XError", (char *)NULL, 
        Blt_Offset(LineElement, xError), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-ydata", "yData", "YData", (char *)NULL, 
        Blt_Offset(LineElement, y), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-yerror", "yError", "YError", (char *)NULL, 
        Blt_Offset(LineElement, yError), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-xhigh", "xHigh", "XHigh", (char *)NULL, 
        Blt_Offset(LineElement, xHigh), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-xlow", "xLow", "XLow", (char *)NULL, 
        Blt_Offset(LineElement, xLow), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-yhigh", "yHigh", "YHigh", (char *)NULL, 
        Blt_Offset(LineElement, xHigh), 0, &bltValuesOption},
    {BLT_CONFIG_CUSTOM, "-ylow", "yLow", "YLow", (char *)NULL, 
        Blt_Offset(LineElement, yLow), 0, &bltValuesOption},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

static Blt_ConfigSpec penSpecs[] =
{
    {BLT_CONFIG_COLOR, "-color", "color", "Color", DEF_PEN_ACTIVE_COLOR, 
        Blt_Offset(LinePen, traceColor), ACTIVE_PEN},
    {BLT_CONFIG_COLOR, "-color", "color", "Color", DEF_PEN_NORMAL_COLOR, 
        Blt_Offset(LinePen, traceColor), NORMAL_PEN},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", "Dashes", DEF_PEN_DASHES, 
        Blt_Offset(LinePen, traceDashes), BLT_CONFIG_NULL_OK | ALL_PENS},
    {BLT_CONFIG_CUSTOM, "-errorbars", "errorBars", "ErrorBars",
        DEF_PEN_ERRORBARS, Blt_Offset(LinePen, errorFlags),
        BLT_CONFIG_DONT_SET_DEFAULT, &errorbarsOption},
    {BLT_CONFIG_CUSTOM, "-errorbarcolor", "errorBarColor", "ErrorBarColor",
        DEF_PEN_ERRORBAR_COLOR, Blt_Offset(LinePen, errorColor), 
        ALL_PENS, &bltColorOption},
    {BLT_CONFIG_PIXELS_NNEG, "-errorbarlinewidth", "errorBarLineWidth", 
        "ErrorBarLineWidth", DEF_PEN_ERRORBAR_LINEWIDTH, 
        Blt_Offset(LinePen, errorLineWidth), 
        ALL_PENS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-fill", "fill", "Fill", DEF_PEN_FILL_COLOR, 
        Blt_Offset(LinePen, symbol.fillColor), BLT_CONFIG_NULL_OK | ALL_PENS, 
        &bltColorOption},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth", 
        DEF_PEN_LINEWIDTH, Blt_Offset(LinePen, traceWidth), 
        ALL_PENS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-offdash", "offDash", "OffDash", DEF_PEN_OFFDASH_COLOR,
        Blt_Offset(LinePen, traceOffColor), BLT_CONFIG_NULL_OK | ALL_PENS, 
        &bltColorOption},
    {BLT_CONFIG_CUSTOM, "-outline", "outline", "Outline", DEF_PEN_OUTLINE_COLOR,
        Blt_Offset(LinePen, symbol.outlineColor), ALL_PENS, &bltColorOption},
    {BLT_CONFIG_PIXELS_NNEG, "-outlinewidth", "outlineWidth", "OutlineWidth",
        DEF_PEN_OUTLINE_WIDTH, Blt_Offset(LinePen, symbol.outlineWidth),
        BLT_CONFIG_DONT_SET_DEFAULT | ALL_PENS},
    {BLT_CONFIG_PIXELS_NNEG, "-pixels", "pixels", "Pixels", DEF_PEN_PIXELS, 
        Blt_Offset(LinePen, symbol.size), ALL_PENS},
    {BLT_CONFIG_FILL, "-showvalues", "showValues", "ShowValues",
        DEF_PEN_SHOW_VALUES, Blt_Offset(LinePen, valueFlags),
        ALL_PENS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-symbol", "symbol", "Symbol", DEF_PEN_SYMBOL, 
        Blt_Offset(LinePen, symbol), BLT_CONFIG_DONT_SET_DEFAULT | ALL_PENS, 
        &symbolOption},
    {BLT_CONFIG_STRING, "-type", (char *)NULL, (char *)NULL, DEF_PEN_TYPE, 
        Blt_Offset(Pen, typeId), ALL_PENS | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_ANCHOR, "-valueanchor", "valueAnchor", "ValueAnchor",
        DEF_PEN_VALUE_ANCHOR, Blt_Offset(LinePen, valueStyle.anchor), ALL_PENS},
    {BLT_CONFIG_COLOR, "-valuecolor", "valueColor", "ValueColor",
        DEF_PEN_VALUE_COLOR, Blt_Offset(LinePen, valueStyle.color), ALL_PENS},
    {BLT_CONFIG_FONT, "-valuefont", "valueFont", "ValueFont",
        DEF_PEN_VALUE_FONT, Blt_Offset(LinePen, valueStyle.font), ALL_PENS},
    {BLT_CONFIG_STRING, "-valueformat", "valueFormat", "ValueFormat",
        DEF_PEN_VALUE_FORMAT, Blt_Offset(LinePen, valueFormat),
        ALL_PENS | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FLOAT, "-valuerotate", "valueRotate", "ValueRotate",
        DEF_PEN_VALUE_ANGLE, Blt_Offset(LinePen, valueStyle.angle), ALL_PENS},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

typedef double (DistanceProc)(int x, int y, Point2d *p, Point2d *q, Point2d *t);

/* Forward declarations */
static PenConfigureProc ConfigurePenProc;
static PenDestroyProc DestroyPenProc;
static ElementNearestProc NearestProc;
static ElementConfigProc ConfigureProc;
static ElementDestroyProc DestroyProc;
static ElementDrawProc DrawActiveProc;
static ElementDrawProc DrawNormalProc;
static ElementDrawSymbolProc DrawSymbolProc;
static ElementFindProc FindProc;
static ElementExtentsProc ExtentsProc;
static ElementToPostScriptProc ActiveToPostScriptProc;
static ElementToPostScriptProc NormalToPostScriptProc;
static ElementSymbolToPostScriptProc SymbolToPostScriptProc;
static ElementMapProc MapProc;
static DistanceProc DistanceToYProc;
static DistanceProc DistanceToXProc;
static DistanceProc DistanceToLineProc;
static Blt_BackgroundChangedProc BackgroundChangedProc;
static Blt_BrushChangedProc BrushChangedProc;

#ifdef WIN32

static int tkpWinRopModes[] =
{
    R2_BLACK,                           /* GXclear */
    R2_MASKPEN,                         /* GXand */
    R2_MASKPENNOT,                      /* GXandReverse */
    R2_COPYPEN,                         /* GXcopy */
    R2_MASKNOTPEN,                      /* GXandInverted */
    R2_NOT,                             /* GXnoop */
    R2_XORPEN,                          /* GXxor */
    R2_MERGEPEN,                        /* GXor */
    R2_NOTMERGEPEN,                     /* GXnor */
    R2_NOTXORPEN,                       /* GXequiv */
    R2_NOT,                             /* GXinvert */
    R2_MERGEPENNOT,                     /* GXorReverse */
    R2_NOTCOPYPEN,                      /* GXcopyInverted */
    R2_MERGENOTPEN,                     /* GXorInverted */
    R2_NOTMASKPEN,                      /* GXnand */
    R2_WHITE                            /* GXset */
};

#endif

/*
 *---------------------------------------------------------------------------
 *      Custom configuration option (parse and print) routines
 *---------------------------------------------------------------------------
 */
static void
DestroySymbol(Display *display, Symbol *symbolPtr)
{
    if (symbolPtr->image != NULL) {
        Tk_FreeImage(symbolPtr->image);
        symbolPtr->image = NULL;
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
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ImageChangedProc(
    ClientData clientData,
    int x, int y, int w, int h,         /* Not used. */
    int imageWidth, int imageHeight)    /* Not used. */
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
FreeSymbolProc(
    ClientData clientData,              /* Not used. */
    Display *display,                   /* Not used. */
    char *widgRec,
    int offset)
{
    Symbol *symbolPtr = (Symbol *)(widgRec + offset);

    DestroySymbol(display, symbolPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSymbolProc --
 *
 *      Convert the string representation of a line style or symbol name
 *      into its numeric form.
 *
 * Results:
 *      The return value is a standard TCL result.  The symbol type is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToSymbolProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to report results */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing symbol type */
    char *widgRec,                      /* Element information record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    Symbol *symbolPtr = (Symbol *)(widgRec + offset);
    const char *string;
    SymbolTable *p;
    int length;
    char c;

    string = Tcl_GetStringFromObj(objPtr, &length);
    if (length == 0) {
        /* Empty string means to symbol. */
        DestroySymbol(Tk_Display(tkwin), symbolPtr);
        symbolPtr->type = SYMBOL_NONE;
        return TCL_OK;
    }
    c = string[0];
    if (c == '@') {
        Tk_Image tkImage;
        Element *elemPtr = (Element *)widgRec;

        /* Must be an image. */
        tkImage = Tk_GetImage(interp, tkwin, string+1, ImageChangedProc, 
                elemPtr);
        if (tkImage == NULL) {
            return TCL_ERROR;
        }
        DestroySymbol(Tk_Display(tkwin), symbolPtr);
        symbolPtr->image = tkImage;
        symbolPtr->type = SYMBOL_IMAGE;
        return TCL_OK;
    }

    for (p = symbolTable; p->name != NULL; p++) {
        if ((length < p->minChars) || (p->minChars == 0)) {
            continue;
        }
        if ((c == p->name[0]) && (strncmp(string, p->name, length) == 0)) {
            DestroySymbol(Tk_Display(tkwin), symbolPtr);
            symbolPtr->type = p->type;
            return TCL_OK;
        }
    }
    Tcl_AppendResult(interp, "bad symbol type \"", string, 
        "\": should be \"none\", \"circle\", \"square\", \"diamond\", "
        "\"plus\", \"cross\", \"splus\", \"scross\", \"triangle\", "
        "\"arrow\" or @imageName ", (char *)NULL);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeBackground --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FreeBackground(ClientData clientData, Display *display, char *widgRec,
               int offset)
{
    LineElement *elemPtr = (LineElement *)(widgRec + offset);

    if (elemPtr->brush != NULL) {
        Blt_FreeBrush(elemPtr->brush);
        elemPtr->brush = NULL;
    }
    if (elemPtr->areaBg != NULL) {
        Blt_Bg_Free(elemPtr->areaBg);
        elemPtr->areaBg = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToBackground --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToBackground(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    LineElement *elemPtr = (LineElement *)(widgRec + offset);
    Blt_Bg bg;
    Blt_PaintBrush brush;
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    /* Handle NULL string.  */
    if (length == 0) {
        FreeBackground(clientData, Tk_Display(tkwin), widgRec, offset);
        return TCL_OK;
    }
    /* First try as a background */
    if (Blt_GetBgFromObj(interp, tkwin, objPtr, &bg) == TCL_OK) {
        FreeBackground(clientData, Tk_Display(tkwin), widgRec, offset);
        elemPtr->areaBg = bg;
    } else if (Blt_GetPaintBrushFromObj(interp, objPtr, &brush) == TCL_OK) {
        FreeBackground(clientData, Tk_Display(tkwin), widgRec, offset);
        elemPtr->brush = brush;
    } else {
        Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "bad color argument \"", string,
                "\": should be a color name, background, or paintbrush",
                (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * BackgroundToObj --
 *
 *      Returns the mode style string based upon the mode flags.
 *
 * Results:
 *      The mode style string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
BackgroundToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                char *widgRec, int offset, int flags)
{
    LineElement *elemPtr = (LineElement *)(widgRec + offset);
    const char *string;
    
    if (elemPtr->areaBg != NULL) {
        string = Blt_Bg_Name(elemPtr->areaBg);
    } else if (elemPtr->brush != NULL) {
        string = Blt_GetBrushName(elemPtr->brush);
    } else {
        string = "";
    }
    return Tcl_NewStringObj(string, -1);
}


/*
 *---------------------------------------------------------------------------
 *
 * SymbolToObjProc --
 *
 *      Convert the symbol value into a string.
 *
 * Results:
 *      The string representing the symbol type or line style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SymbolToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,
    char *widgRec,                      /* Element information record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    Symbol *symbolPtr = (Symbol *)(widgRec + offset);
    SymbolTable *p;

    if (symbolPtr->type == SYMBOL_IMAGE) {
        Tcl_Obj *objPtr;

        objPtr = Tcl_NewStringObj("@", 1);
        Tcl_AppendToObj(objPtr, Blt_Image_Name(symbolPtr->image), -1);
        return objPtr;
    }
    for (p = symbolTable; p->name != NULL; p++) {
        if (p->type == symbolPtr->type) {
            return Tcl_NewStringObj(p->name, -1);
        }
    }
    return Tcl_NewStringObj("?unknown symbol type?", -1);
}

/*
 * ObjToErrorBarsProc --
 *
 *      Convert the string representation of a errorbar flags into its numeric
 *      form.
 *
 * Results:
 *      The return value is a standard TCL result.  The errorbar flags is
 *      written into the widget record.
 */
/*ARGSUSED*/
static int
ObjToErrorBarsProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to return results. */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing smooth type */
    char *widgRec,                      /* Element information record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    int *flagsPtr = (int *)(widgRec + offset);
    unsigned int mask;
    int i;
    int objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(NULL, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    mask = 0;
    for (i = 0; i < objc; i++) {
        const char *string;
        char c;
        
        string = Tcl_GetString(objv[i]);
        c = string[0];
        if ((c == 'x') && (strcmp(string, "x") == 0)) {
            mask |= XERROR;
        } else if ((c == 'y') && (strcmp(string, "y") == 0)) {
            mask |= YERROR;
        } else if ((c == 'x') && (strcmp(string, "xhigh") == 0)) {
            mask |= XHIGH;
        } else if ((c == 'y') && (strcmp(string, "yhigh") == 0)) {
            mask |= YHIGH;
        } else if ((c == 'x') && (strcmp(string, "xlow") == 0)) {
            mask |= XLOW;
        } else if ((c == 'y') && (strcmp(string, "ylow") == 0)) {
            mask |= YLOW;
        } else if ((c == 'b') && (strcmp(string, "both") == 0)) {
            mask |= YERROR | XERROR;
        } else {
            Tcl_AppendResult(interp, "bad errorbar value \"", string, 
                "\": should be x, y, xhigh, yhigh, xlow, ylow, or both", 
                (char *)NULL);
            return TCL_ERROR;
        }
    }
    *flagsPtr = mask;
    return TCL_OK;
}

/*
 * ErrorBarsToObjProc --
 *
 *      Convert the error flags value into a list of strings.
 *
 * Results:
 *      The list representing the errorbar flags is returned.
 */
/*ARGSUSED*/
static Tcl_Obj *
ErrorBarsToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Not used. */
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Element information record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    int mask = *(int *)(widgRec + offset);
    Tcl_Obj *objPtr, *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if ((mask & XERROR) && (mask & YERROR)) {
        objPtr = Tcl_NewStringObj("both", 4);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    } else {
        if (mask & XERROR) {
            objPtr = Tcl_NewStringObj("x", 1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        } else if (mask & XHIGH) {
            objPtr = Tcl_NewStringObj("xhigh", 5);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        } else if (mask & XLOW) {
            objPtr = Tcl_NewStringObj("xlow", 4);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        if (mask & YERROR) {
            objPtr = Tcl_NewStringObj("y", 1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        } else if (mask & YHIGH) {
            objPtr = Tcl_NewStringObj("yhigh", 5);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        } else if (mask & YLOW) {
            objPtr = Tcl_NewStringObj("ylow", 4);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    return listObjPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * NameOfSmooth --
 *
 *      Converts the smooth value into its string representation.
 *
 * Results:
 *      The static string representing the smooth type is returned.
 *
 *---------------------------------------------------------------------------
 */
static const char *
NameOfSmooth(unsigned int flags)
{
    SmoothingTable *smoothPtr;

    for (smoothPtr = smoothingTable; smoothPtr->name != NULL; smoothPtr++) {
        if (smoothPtr->flags == flags) {
            return smoothPtr->name;
        }
    }
    return "unknown smooth value";
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToSmoothProc --
 *
 *      Convert the string representation of a line style or smooth name
 *      into its numeric form.
 *
 * Results:
 *      The return value is a standard TCL result.  The smooth type is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToSmoothProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing smooth type */
    char *widgRec,                      /* Element information record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    SmoothingTable *smoothPtr;
    const char *string;
    char c;

    string = Tcl_GetString(objPtr);
    c = string[0];
    for (smoothPtr = smoothingTable; smoothPtr->name != NULL; smoothPtr++) {
        if ((c == smoothPtr->name[0]) && (strcmp(string,smoothPtr->name)==0)) {
            *flagsPtr = smoothPtr->flags;
            return TCL_OK;
        }
    }
    Tcl_AppendResult(interp, "bad smooth value \"", string, "\": should be \
linear, step, natural, or quadratic", (char *)NULL);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * SmoothToObjProc --
 *
 *      Convert the smooth value into a string.
 *
 * Results:
 *      The string representing the smooth type or line style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
SmoothToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Not used. */
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Element information record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    int smooth = *(int *)(widgRec + offset);

    return Tcl_NewStringObj(NameOfSmooth(smooth), -1);
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToPenDirProc --
 *
 *      Convert the string representation of a line style or symbol name
 *      into its numeric form.
 *
 * Results:
 *      The return value is a standard TCL result.  The symbol type is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPenDirProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    Tk_Window tkwin,                    /* Not used. */
    Tcl_Obj *objPtr,                    /* String representing pen direction */
    char *widgRec,                      /* Element information record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    int *penDirPtr = (int *)(widgRec + offset);
    int length;
    char c;
    char *string;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'i') && (strncmp(string, "increasing", length) == 0)) {
        *penDirPtr = PEN_INCREASING;
    } else if ((c == 'd') && (strncmp(string, "decreasing", length) == 0)) {
        *penDirPtr = PEN_DECREASING;
    } else if ((c == 'b') && (strncmp(string, "both", length) == 0)) {
        *penDirPtr = PEN_BOTH_DIRECTIONS;
    } else {
        Tcl_AppendResult(interp, "bad trace value \"", string,
            "\" : should be \"increasing\", \"decreasing\", or \"both\"",
            (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NameOfPenDir --
 *
 *      Convert the pen direction into a string.
 *
 * Results:
 *      The static string representing the pen direction is returned.
 *
 *---------------------------------------------------------------------------
 */
static const char *
NameOfPenDir(int penDir)
{
    switch (penDir) {
    case PEN_INCREASING:
        return "increasing";
    case PEN_DECREASING:
        return "decreasing";
    case PEN_BOTH_DIRECTIONS:
        return "both";
    default:
        return "unknown trace direction";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PenDirToObj --
 *
 *      Convert the pen direction into a string.
 *
 * Results:
 *      The string representing the pen drawing direction is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PenDirToObjProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Not used. */
    Tk_Window tkwin,                    /* Not used. */
    char *widgRec,                      /* Element information record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    int penDir = *(int *)(widgRec + offset);

    return Tcl_NewStringObj(NameOfPenDir(penDir), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigurePenProc --
 *
 *      Sets up the appropriate configuration parameters in the GC.  It is
 *      assumed the parameters have been previously set by a call to
 *      Blt_ConfigureWidget.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is
 *      returned, then interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information such as line width, line style, color
 *      etc. get set in a new GC.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ConfigurePenProc(Graph *graphPtr, Pen *basePtr)
{
    LinePen *penPtr = (LinePen *)basePtr;
    unsigned long gcMask;
    GC newGC;
    XGCValues gcValues;
    XColor *colorPtr;

    /*
     * Set the outline GC for this pen: GCForeground is outline color.
     */
    gcMask = (GCLineWidth | GCForeground);
    colorPtr = penPtr->symbol.outlineColor;
    if (colorPtr == COLOR_DEFAULT) {
        colorPtr = penPtr->traceColor;
    }
    gcValues.foreground = colorPtr->pixel;
    gcValues.line_width = LineWidth(penPtr->symbol.outlineWidth);
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (penPtr->symbol.outlineGC != NULL) {
        Tk_FreeGC(graphPtr->display, penPtr->symbol.outlineGC);
    }
    penPtr->symbol.outlineGC = newGC;

    /* Fill GC for symbols: GCForeground is fill color */

    gcMask = (GCLineWidth | GCForeground);
    colorPtr = penPtr->symbol.fillColor;
    if (colorPtr == COLOR_DEFAULT) {
        colorPtr = penPtr->traceColor;
    }
    newGC = NULL;
    if (colorPtr != NULL) {
        gcValues.foreground = colorPtr->pixel;
        newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    }
    if (penPtr->symbol.fillGC != NULL) {
        Tk_FreeGC(graphPtr->display, penPtr->symbol.fillGC);
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
    if (colorPtr == COLOR_DEFAULT) {
        colorPtr = penPtr->traceColor;
    }
    if (colorPtr != NULL) {
        gcMask |= GCBackground;
        gcValues.background = colorPtr->pixel;
    }
    gcValues.foreground = penPtr->traceColor->pixel;
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

    gcMask = (GCLineWidth | GCForeground);
    colorPtr = penPtr->errorColor;
    if (colorPtr == COLOR_DEFAULT) {
        colorPtr = penPtr->traceColor;
    }
    gcValues.line_width = LineWidth(penPtr->errorLineWidth);
    gcValues.foreground = colorPtr->pixel;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (penPtr->errorGC != NULL) {
        Tk_FreeGC(graphPtr->display, penPtr->errorGC);
    }
    penPtr->errorGC = newGC;

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyPenProc --
 *
 *      Release memory and resources allocated for the style.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the pen style is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyPenProc(Graph *graphPtr, Pen *basePtr)
{
    LinePen *penPtr = (LinePen *)basePtr;

    Blt_Ts_FreeStyle(graphPtr->display, &penPtr->valueStyle);
    if (penPtr->symbol.outlineGC != NULL) {
        Tk_FreeGC(graphPtr->display, penPtr->symbol.outlineGC);
    }
    if (penPtr->symbol.fillGC != NULL) {
        Tk_FreeGC(graphPtr->display, penPtr->symbol.fillGC);
    }
    if (penPtr->errorGC != NULL) {
        Tk_FreeGC(graphPtr->display, penPtr->errorGC);
    }
    if (penPtr->traceGC != NULL) {
        Blt_FreePrivateGC(graphPtr->display, penPtr->traceGC);
    }
}

static void
InitPen(LinePen *penPtr)
{
    Blt_Ts_InitStyle(penPtr->valueStyle);
    penPtr->errorLineWidth = 1;
    penPtr->errorFlags = XERROR | YERROR;
    penPtr->configProc = ConfigurePenProc;
    penPtr->configSpecs = penSpecs;
    penPtr->destroyProc = DestroyPenProc;
    penPtr->flags = NORMAL_PEN;
    penPtr->symbol.outlineColor = penPtr->symbol.fillColor = COLOR_DEFAULT;
    penPtr->symbol.outlineWidth = penPtr->traceWidth = 1;
    penPtr->symbol.type = SYMBOL_CIRCLE;
    penPtr->valueFlags = SHOW_NONE;
}

Pen *
Blt_CreateLinePen2(Graph *graphPtr, ClassId id, Blt_HashEntry *hPtr)
{
    LinePen *penPtr;

    penPtr = Blt_AssertCalloc(1, sizeof(LinePen));
    penPtr->name = Blt_GetHashKey(&graphPtr->penTable, hPtr);
    penPtr->classId = id;
    penPtr->graphPtr = graphPtr;
    penPtr->hashPtr = hPtr;
    InitPen(penPtr);
    if (strcmp(penPtr->name, "activeLine") == 0) {
        penPtr->flags = ACTIVE_PEN;
    }
    Blt_SetHashValue(hPtr, penPtr);
    return (Pen *)penPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 *      In this section, the routines deal with building and filling the
 *      element's data structures with transformed screen coordinates.
 *      They are triggered from TranformLine which is called whenever the
 *      data or coordinates axes have changed and new screen coordinates
 *      need to be calculated.
 *
 *---------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 *
 * ScaleSymbol --
 *
 *      Returns the scaled size for the line element. Scaling depends upon
 *      when the base line ranges for the element were set and the current
 *      range of the graph.
 *
 * Results:
 *      The new size of the symbol, after considering how much the graph
 *      has been scaled, is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
ScaleSymbol(LineElement *elemPtr, int normalSize)
{
    int maxSize;
    double scale;
    int newSize;

    scale = 1.0;
    if (elemPtr->scaleSymbols) {
        double xRange, yRange;

        xRange = (elemPtr->axes.x->max - elemPtr->axes.x->min);
        yRange = (elemPtr->axes.y->max - elemPtr->axes.y->min);
        if (elemPtr->flags & SCALE_SYMBOL) {
            /* Save the ranges as a baseline for future scaling. */
            elemPtr->xRange = xRange;
            elemPtr->yRange = yRange;
            elemPtr->flags &= ~SCALE_SYMBOL;
        } else {
            double xScale, yScale;

            /* Scale the symbol by the smallest change in the X or Y axes */
            xScale = elemPtr->xRange / xRange;
            yScale = elemPtr->yRange / yRange;
            scale = MIN(xScale, yScale);
        }
    }
    newSize = ROUND(normalSize * scale);

    /*
     * Don't let the size of symbols go unbounded. Both X and Win32 drawing
     * routines assume coordinates to be a signed short int.
     */
    maxSize = (int)MIN(elemPtr->obj.graphPtr->hRange, 
                       elemPtr->obj.graphPtr->vRange);
    if (newSize > maxSize) {
        newSize = maxSize;
    }

    /* Make the symbol size odd so that its center is a single pixel. */
    newSize |= 0x01;
    return newSize;
}

/*
 *---------------------------------------------------------------------------
 *
 * RemoveHead --
 *
 *      Removes the point at the head of the trace.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The trace is shrunk.
 *
 *---------------------------------------------------------------------------
 */
static void
RemoveHead(LineElement *elemPtr, Trace *tracePtr)
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
 *      Frees the memory assoicated with a trace.
 *      Note: The points and segments of the trace are freed enmass when
 *             destroying the memory poll assoicated with the element.
 *
 * Results:
 *      Returns a pointer to the new trace.
 *
 * Side Effects:
 *      The trace is prepended to the element's list of traces.
 *
 *---------------------------------------------------------------------------
 */
static void
FreeTrace(Blt_Chain traces, Trace *tracePtr)
{
    if (tracePtr->link != NULL) {
        Blt_Chain_DeleteLink(traces, tracePtr->link);
    }
    if (tracePtr->fillPts != NULL) {
        Blt_Free(tracePtr->fillPts);
    }
    Blt_Free(tracePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FixTraces --
 *
 *      Fixes the trace by recounting the number of points.
 *      
 * Results:
 *      None.
 *
 * Side Effects:
 *      Removes the trace if it is empty.
 *
 *---------------------------------------------------------------------------
 */
static void
FixTraces(Blt_Chain traces)
{
    Blt_ChainLink link, next;

    for (link = Blt_Chain_FirstLink(traces); link != NULL; link = next) {
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

#ifdef notdef
static void
DumpFlags(unsigned int flags)
{
    if (flags & VISIBLE) {
        fprintf(stderr, "visible ");
    }
    if (flags & KNOT) {
        fprintf(stderr, "knot ");
    }
    if (flags & SYMBOL) {
        fprintf(stderr, "symbol ");
    }
    if (flags & ACTIVE_POINT) {
        fprintf(stderr, "active ");
    }
    if (flags & XHIGH) {
        fprintf(stderr, "xhigh ");
    }
    if (flags & XLOW) {
        fprintf(stderr, "xlow ");
    }
    if (flags & XLOW) {
        fprintf(stderr, "xlow ");
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DumpPoints --
 *
 *      Creates a new trace and prepends to the list of traces for this
 *      element.
 *
 * Results:
 *      Returns a pointer to the new trace.
 *
 * Side Effects:
 *      The trace is prepended to the element's list of traces.
 *
 *---------------------------------------------------------------------------
 */
static void
DumpPoints(Trace *tracePtr)
{
    TracePoint *p;
    int i;

    fprintf(stderr, " element \"%s\", trace %lx: # of points = %d\n",
            tracePtr->elemPtr->obj.name, (unsigned long)tracePtr, 
            tracePtr->numPoints);
    for (i = 0, p = tracePtr->head; p != NULL; p = p->next, i++) {
        fprintf(stderr, "   point %d: x=%g y=%g index=%d flags=",
                i, p->x, p->y, p->index);
        DumpFlags(p->flags);
        fprintf(stderr, "\n");
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DumpTraces --
 *
 *      Creates a new trace and prepends to the list of traces for this
 *      element.
 *
 * Results:
 *      Returns a pointer to the new trace.
 *
 * Side Effects:
 *      The trace is prepended to the element's list of traces.
 *
 *---------------------------------------------------------------------------
 */
static void
DumpTraces(LineElement *elemPtr)
{
    fprintf(stderr, "element \"%s\": # of points = %ld, # of traces = %ld\n", 
            elemPtr->obj.name, (long)NUMBEROFPOINTS(elemPtr), 
            (long int)Blt_Chain_GetLength(elemPtr->traces));
}

/*
 *---------------------------------------------------------------------------
 *
 * DumpSegments --
 *
 *      Creates a new trace and prepends to the list of traces for this
 *      element.
 *
 * Results:
 *      Returns a pointer to the new trace.
 *
 * Side Effects:
 *      The trace is prepended to the element's list of traces.
 *
 *---------------------------------------------------------------------------
 */
static void
DumpSegments(Trace *tracePtr)
{
    TraceSegment *s;
    int i;

    fprintf(stderr, " element \"%s\", tracePtr %lx: # of segments = %d\n",
            tracePtr->elemPtr->obj.name, (unsigned long)tracePtr, 
            tracePtr->numSegments);
    for (i = 0, s = tracePtr->segments; s != NULL; s = s->next, i++) {
        fprintf(stderr, "   segment %d: x1=%g y1=%g, x2=%g y2=%g index=%d flags=%x\n",
                i, s->x1, s->y1, s->x2, s->y2, s->index, s->flags);
    }
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * NewTrace --
 *
 *      Creates a new trace and prepends to the list of traces for this
 *      element.
 *
 * Results:
 *      Returns a pointer to the new trace.
 *
 * Side Effects:
 *      The trace is prepended to the element's list of traces.
 *
 *---------------------------------------------------------------------------
 */
static INLINE Trace *
NewTrace(LineElement *elemPtr)
{
    Trace *tracePtr;

    tracePtr = Blt_AssertCalloc(1, sizeof(Trace));
    if (elemPtr->traces == NULL) {
        elemPtr->traces = Blt_Chain_Create();
    }
    tracePtr->link = Blt_Chain_Prepend(elemPtr->traces, tracePtr);
    tracePtr->elemPtr = elemPtr;
    tracePtr->penPtr = NORMALPEN(elemPtr);
    return tracePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewPoint --
 *
 *      Creates a new point 
 *
 * Results:
 *      Returns a pointer to the new trace point.
 *
 *---------------------------------------------------------------------------
 */
static INLINE TracePoint *
NewPoint(LineElement *elemPtr, double x, double y, int index)
{
    TracePoint *p;
    Region2d exts;

    p = Blt_Pool_AllocItem(elemPtr->pointPool, sizeof(TracePoint));
    p->next = NULL;
    p->flags = 0;
    p->x = x;
    p->y = y;
    Blt_GraphExtents(elemPtr, &exts);
    if (PointInRegion(&exts, p->x, p->y)) {
        p->flags |= VISIBLE;
    }
    p->index = index;
    return p;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewSegment --
 *
 *      Creates a new segment of the trace's errorbars.
 *
 * Results:
 *      Returns a pointer to the new trace.
 *
 *---------------------------------------------------------------------------
 */
static INLINE TraceSegment *
NewSegment(LineElement *elemPtr, float x1, float y1, float x2, float y2, 
           int index, unsigned int flags)
{
    TraceSegment *s;

    s = Blt_Pool_AllocItem(elemPtr->segmentPool, sizeof(TraceSegment));
    s->x1 = x1;
    s->y1 = y1;
    s->x2 = x2;
    s->y2 = y2;
    s->index = index;
    s->flags = flags | VISIBLE;
    s->next = NULL;
    return s;
}

/*
 *---------------------------------------------------------------------------
 *
 * AddSegment --
 *
 *      Appends a line segment point to the given trace.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The trace's segment counter is incremented.
 *
 *---------------------------------------------------------------------------
 */
static INLINE void
AddSegment(Trace *tracePtr, TraceSegment *s)
{
    
    if (tracePtr->segments == NULL) {
        tracePtr->segments = s;
    } else {
        s->next = tracePtr->segments;
        tracePtr->segments = s;
    }
    tracePtr->numSegments++;
}

/*
 *---------------------------------------------------------------------------
 *
 * AppendPoint --
 *
 *      Appends the point to the given trace.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The trace's point counter is incremented.
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
        tracePtr->tail = p;
    }
    tracePtr->numPoints++;
}

static void
ResetElement(LineElement *elemPtr) 
{
    Blt_ChainLink link, next;

    if (elemPtr->pointPool != NULL) {
        Blt_Pool_Destroy(elemPtr->pointPool);
    }
    elemPtr->pointPool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);

    if (elemPtr->segmentPool != NULL) {
        Blt_Pool_Destroy(elemPtr->segmentPool);
    }
    elemPtr->segmentPool = Blt_Pool_Create(BLT_FIXED_SIZE_ITEMS);

    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL; 
         link = next) {
        Trace *tracePtr;

        next = Blt_Chain_NextLink(link);
        tracePtr = Blt_Chain_GetValue(link);
        FreeTrace(elemPtr->traces, tracePtr);
    }
    if (elemPtr->traces != NULL) {
        Blt_Chain_Destroy(elemPtr->traces);
        elemPtr->traces = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetScreenPoints --
 *
 *      Generates a list of transformed screen coordinates from the data
 *      points.  Coordinates with Inf, -Inf, or NaN values are considered
 *      holes in the data and will create new traces.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory is allocated for the list of coordinates.
 *
 *---------------------------------------------------------------------------
 */
static void
GetScreenPoints(LineElement *elemPtr)
{
    Graph *graphPtr = elemPtr->obj.graphPtr;
    Trace *tracePtr;
    TracePoint *q;
    int i, n;
    double *x, *y;

    tracePtr = NULL;
    q = NULL;
    n = NUMBEROFPOINTS(elemPtr);
    x = elemPtr->x.values;
    y = elemPtr->y.values;
    for (i = 0; i < n; i++) {
        int broken;
        int j;
        TracePoint *p;
        Point2d r;

        j = i;
        while (j < n) {
            /* Treat -inf, inf, NaN values as holes in the data. Also
             * ignore non-positive values when the axis is log scale. */
            if ((!FINITE(x[j]))||(!FINITE(y[j]))) {
                j++;
            } else if ((IsLogScale(elemPtr->axes.y)) & (y[j] <= 0.0)) {
                j++;                    
            } else if ((IsLogScale(elemPtr->axes.x)) & (x[j] <= 0.0)) {
                j++;
            } else {
                break;
            }
        }
        if (j == n) {
            break;
        }
        r = Blt_Map2D(graphPtr, x[j], y[j], &elemPtr->axes);
        p = NewPoint(elemPtr, r.x, r.y, j);
        p->flags |= KNOT;
        broken = TRUE;
        if ((i == j) && (q != NULL)) {
            broken = BROKEN_TRACE(elemPtr->penDir, p->x, q->x);
        } else {
            i = j;
        }
        if (broken) {
            if ((tracePtr == NULL) || (tracePtr->numPoints > 0)) {
                tracePtr = NewTrace(elemPtr);
            }
        }
        AppendPoint(tracePtr, p);
        q = p;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GenerateSteps --
 *
 *      Add points to the list of coordinates for step-and-hold type
 *      smoothing.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Points are added to the list of screen coordinates.
 *
 *---------------------------------------------------------------------------
 */
static void
GenerateSteps(Trace *tracePtr)
{
    Graph *graphPtr = tracePtr->elemPtr->obj.graphPtr;
    TracePoint *q, *p;

    for (p = tracePtr->head, q = p->next; q != NULL; q = q->next) {
        TracePoint *t;
        
        /* 
         *         q
         *         |
         *  p ---- t
         */
        if (graphPtr->flags & INVERTED) {
            t = NewPoint(tracePtr->elemPtr, p->x, q->y, p->index);
        } else {
            t = NewPoint(tracePtr->elemPtr, q->x, p->y, p->index);
        }
        /* Insert the new point between the two knots. */
        t->next = q;
        p->next = t;
        tracePtr->numPoints++;
        p = q;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GenerateSpline --
 *
 *      Computes a cubic or quadratic spline and adds extra points to the 
 *      list of coordinates for smoothing.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Points are added to the list of screen coordinates.
 *
 *---------------------------------------------------------------------------
 */
static void
GenerateSpline(Trace *tracePtr)
{
    Blt_Spline spline;
    Graph *graphPtr = tracePtr->elemPtr->obj.graphPtr;
    LineElement *elemPtr = tracePtr->elemPtr;
    Point2d *points;
    TracePoint *p, *q;
    int i;

    /* FIXME: 1) handle inverted graph. 2) automatically flip to parametric
     * spline if non-monotonic. */
    for (p = tracePtr->head, q = p->next; q != NULL; q = q->next) {
        if (q->x <= p->x) {
            return;                     /* Points are not monotonically
                                         * increasing */
        }
        p = q;
    }
    p = tracePtr->head;
    q = tracePtr->tail;
    if (((p->x > (double)graphPtr->x2)) || 
        ((q->x < (double)graphPtr->x1))) {
        return;                         /* All points are clipped. This
                                         * only works if x is monotonically
                                         * increasing. */
    }

    /*
     * The spline is computed in screen coordinates instead of data points
     * so that we can select the abscissas of the interpolated points from
     * each pixel horizontally across the plotting area.
     */
    if (graphPtr->x2 <= graphPtr->x1) {
        return;
    }
    points = Blt_AssertMalloc(tracePtr->numPoints * sizeof(Point2d));

    /* Populate the interpolated point array with the original
     * x-coordinates and extra interpolated x-coordinates for each
     * horizontal pixel that the line segment contains. Do this only for
     * pixels that are on screen */
    for (i = 0, p = tracePtr->head; p != NULL; p = p->next, i++) {
        /* Add the original x-coordinate */
        points[i].x = p->x;
        points[i].y = p->y;
    }
    spline = Blt_CreateSpline(points, tracePtr->numPoints, elemPtr->smooth);
    if (spline == NULL) {
        return;                         /* Can't interpolate. */
    }
    for (i = 0, p = tracePtr->head, q = p->next; q != NULL; q = q->next, i++) {
        /* Is any part of the interval (line segment) in the plotting
         * area?  */
        if ((p->flags | q->flags) & VISIBLE) {
            TracePoint *lastp;
            double x, last;
            
            /*  Interpolate segments that lie on the screen. */
            x = p->x + 1.0;
            /*
             * Since the line segment may be partially clipped on the left
             * or right side, the points to interpolate are always interior
             * to the plotting area.
             *
             *           left                       right
             *      x1----|---------------------------|---x2
             *
             * Pick the max of the starting X-coordinate and the left edge
             * and the min of the last X-coordinate and the right edge.
             */
            x = MAX(x, (double)graphPtr->x1);
            last = MIN(q->x, (double)graphPtr->x2);

            /* Add the extra x-coordinates to the interval. */
            lastp = p;
            while (x < last) {
                Point2d p1;
                TracePoint *t;

                p1 = Blt_EvaluateSpline(spline, i, x);
                t = NewPoint(elemPtr, p1.x, p1.y, p->index);
                /* Insert the new point in to line segment. */
                t->next = lastp->next; 
                lastp->next = t;
                lastp = t;
                tracePtr->numPoints++;
                x++;
            }
            assert(lastp->next == q);
        }
        p = q;
    }
#ifdef notdef
    DumpPoints(tracePtr);
#endif
    Blt_Free(points);
    Blt_FreeSpline(spline);
}

/*
 *---------------------------------------------------------------------------
 *
 * GenerateParametricSplineOld --
 *
 *      Computes a spline based upon the data points, returning a new
 *      (larger) coordinate array or points.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The temporary arrays for screen coordinates and data map are
 *      updated based upon spline.
 *
 * FIXME: Can't interpolate knots along the Y-axis.  Need to break up point
 *         array into interchangable X and Y vectors earlier. Pass extents
 *         (left/right or top/bottom) as parameters.
 *
 *---------------------------------------------------------------------------
 */
static void
GenerateParametricSplineOld(Trace *tracePtr)
{
    LineElement *elemPtr = tracePtr->elemPtr;
    Region2d exts;
    Point2d *xpoints, *ypoints;
    double *distance;
    int i, count;
    double total;
    TracePoint *p, *q;
    Blt_Spline xspline, yspline;
    int smooth;

    Blt_GraphExtents(elemPtr, &exts);
    xpoints = ypoints = NULL;
    xspline = yspline = NULL;
    distance = NULL;

    /* 
     * Populate the x2 array with both the original X-coordinates and extra
     * X-coordinates for each horizontal pixel that the line segment
     * contains.
     */
    xpoints = Blt_Malloc(tracePtr->numPoints * sizeof(Point2d));
    ypoints = Blt_Malloc(tracePtr->numPoints * sizeof(Point2d));
    distance = Blt_Malloc(tracePtr->numPoints * sizeof(double));
    if ((xpoints == NULL) || (ypoints == NULL)) {
        goto error;
    }
    p = tracePtr->head;
    ypoints[0].x = xpoints[0].x = 0;
    xpoints[0].y = p->x;
    ypoints[0].y = p->y;
    distance[0] = 0;
    count = 1;
    total = 0.0;
    for (q = p->next; q != NULL; q = q->next) {
        double d;

        /* Distance of original point to p. */
        d = hypot(q->x - p->x, q->y - p->y);
        total += d;
        xpoints[count].x = ypoints[count].x = total;
        xpoints[count].y = q->x;
        ypoints[count].y = q->y;
        distance[count] = total;
        count++;
        p = q;
    }
    smooth = elemPtr->smooth & ~SMOOTH_PARAMETRIC;
    xspline = Blt_CreateSpline(xpoints, tracePtr->numPoints, smooth);
    yspline = Blt_CreateSpline(ypoints, tracePtr->numPoints, smooth);
    if ((xspline == NULL) || (yspline == NULL)) {
        goto error;
    }
        
    for (i = 0, p = tracePtr->head, q = p->next; q != NULL; q = q->next, i++) {
        Point2d p1, p2;
        double dp, dq;
        TracePoint *lastp;

        if (((p->flags | q->flags) & VISIBLE) == 0) {
            continue;                   /* Line segment isn't visible. */
        }

        p1.x = p->x, p1.y = p->y;
        p2.x = q->x, p2.y = q->y;
        Blt_LineRectClip(&exts, &p1, &p2);
        
        /* Distance of original point to p. */
        dp = hypot(p1.x - p->x, p1.y - p->y);
        /* Distance of original point to q. */
        dq = hypot(p2.x - p->x, p2.y - p->y);

        dp += 2;
        dq -= 2;
        lastp = p;
        while(dp <= dq) {
            Point2d px, py;
            double d;
            TracePoint *t;

            d = dp + distance[i];
            /* Point is indicated by its interval and parameter t. */
            px = Blt_EvaluateSpline(xspline, i, d);
            py = Blt_EvaluateSpline(yspline, i, d);
            t = NewPoint(elemPtr, px.y, py.y, p->index);
            /* Insert the new point in to line segment. */
            t->next = lastp->next; 
            lastp->next = t;
            lastp = t;
            tracePtr->numPoints++;
            dp += 2;
        }
        p = q;
    }
 error:
    if (xpoints != NULL) {
        Blt_Free(xpoints);
    }
    if (xspline != NULL) {
        Blt_FreeSpline(xspline);
    }
    if (ypoints != NULL) {
        Blt_Free(ypoints);
    }
    if (yspline != NULL) {
        Blt_FreeSpline(yspline);
    }
    if (distance != NULL) {
        Blt_Free(distance);
    }
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * GenerateParametricSpline --
 *
 *      Computes a spline based upon the data points, returning a new
 *      (larger) coordinate array or points.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The temporary arrays for screen coordinates and data map are updated
 *      based upon spline.
 *
 * FIXME:  Can't interpolate knots along the Y-axis.   Need to break
 *         up point array into interchangable X and Y vectors earlier. *           Pass extents (left/right or top/bottom) as parameters.
 *
 *---------------------------------------------------------------------------
 */
static void
GenerateParametricCubicSpline(Trace *tracePtr)
{
    LineElement *elemPtr = tracePtr->elemPtr;
    Region2d exts;
    Point2d *points, *iPts;
    int isize, niPts;
    int result;
    int i, count;
    TracePoint *p, *q;

    Blt_GraphExtents(elemPtr, &exts);

    /* 
     * Populate the x2 array with both the original X-coordinates and extra
     * X-coordinates for each horizontal pixel that the line segment
     * contains.
     */
    points = Blt_AssertMalloc(tracePtr->numPoints * sizeof(Point2d));
    p = tracePtr->head;
    points[0].x = p->x;
    points[0].y = p->y;
    count = 1;
    for (i = 1, q = p->next; q != NULL; q = q->next, i++) {
        Point2d p1, p2;

        points[i].x = q->x;
        points[i].y = q->y;

        p1.x = p->x, p1.y = p->y;
        p2.x = q->x, p2.y = q->y;
        count++;
        if (Blt_LineRectClip(&exts, &p1, &p2)) {
            count += (int)(hypot(p2.x - p1.x, p2.y - p1.y) * 0.5);
        }
        p = q;
    }
    isize = count;
    iPts = Blt_AssertMalloc(isize * 2 * sizeof(Point2d));

    count = 0;
    for (i = 0, p = tracePtr->head, q = p->next; q != NULL; q = q->next, i++) {
        Point2d p1, p2;

        if (((p->flags | q->flags) & VISIBLE) == 0) {
            p = q;
            continue;
        }
        p1.x = p->x, p1.y = p->y;
        p2.x = q->x, p2.y = q->y;

        /* Add the original x-coordinate */
        iPts[count].x = (double)i;
        iPts[count].y = 0.0;
        count++;

        if (Blt_LineRectClip(&exts, &p1, &p2)) {
            double dp, dq;

            /* Distance of original point to p. */
            dp = hypot(p1.x - p->x, p1.y - p->y);
            /* Distance of original point to q. */
            dq = hypot(p2.x - p->x, p2.y - p->y);
            dp += 2.0;
            while(dp <= dq) {
                /* Point is indicated by its interval and parameter t. */
                iPts[count].x = (double)i;
                iPts[count].y =  dp / dq;
                count++;
                dp += 2.0;
            }
            p = q;
        }
    }
    iPts[count].x = (double)i;
    iPts[count].y = 0.0;
    count++;
    niPts = count;
    result = FALSE;
    result = Blt_ComputeNaturalParametricSpline(points, tracePtr->numPoints, 
        &exts, FALSE, iPts, niPts);
    if (!result) {
        /* The spline interpolation failed.  We will fall back to the current
         * coordinates and do no smoothing (standard line segments).  */
        elemPtr->smooth = SMOOTH_NONE;
        Blt_Free(iPts);
        return;
    } 
    for (i = 0; i < tracePtr->numPoints; i++) {
        fprintf(stderr, "original[%d] = %g,%g\n", i, points[i].x, points[i].y);
    }
    for (i = 0; i < niPts; i++) {
        fprintf(stderr, "interpolated[%d] = %g,%g\n", i, iPts[i].x, iPts[i].y);
    }
    /* Now insert points into trace. */
    for (i = 0, p = tracePtr->head, q = p->next; q != NULL; q = q->next) {
        if ((i < niPts) && (p->x == iPts[i].x) && (p->y == iPts[i].y)) {
            TracePoint *lastp;

            fprintf(stderr, "found knot %g,%g in array[%d]\n", p->x, p->y, i);
            /* Found a knot. Add points until the next knot. */
            i++;
            lastp = p;
            while ((i < niPts) && (q->x != iPts[i].x) && 
                   (q->y != iPts[i].y)) {
                TracePoint *t;
                
            fprintf(stderr, "comparing endpoint %g,%g in array[%d of %d]=%g,%g\n", 
                    q->x, q->y, i, niPts, iPts[i].x, iPts[i].y);
            i++;

                t = NewPoint(elemPtr, iPts[i].x, iPts[i].y, p->index);
                t->next = lastp->next;
                lastp->next = t;
                lastp = t;
                tracePtr->numPoints++;
                i++;
            } 
        } else {
            fprintf(stderr, "comparing point %g,%g in array[%d of %d]=%g,%g\n", 
                    p->x, p->y, i, niPts, iPts[i].x, iPts[i].y);
            i++;
        }
        p = q;
    }
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * GenerateCatromSpline --
 *
 *      Computes a catrom parametric spline.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Points are added to the list of screen coordinates.
 *
 *---------------------------------------------------------------------------
 */
static void
GenerateCatromSpline(Trace *tracePtr)
{
    Blt_Spline spline;
    Graph *graphPtr = tracePtr->elemPtr->obj.graphPtr;
    LineElement *elemPtr = tracePtr->elemPtr;
    Point2d *points;
    TracePoint *p, *q;
    Region2d exts;
    int i;

    /*
     * The spline is computed in screen coordinates instead of data points
     * so that we can select the abscissas of the interpolated points from
     * each pixel horizontally across the plotting area.
     */
    if (graphPtr->x2 <= graphPtr->x1) {
        return;                         /* No space in plotting area. */
    }
    points = Blt_AssertMalloc(tracePtr->numPoints * sizeof(Point2d));

    /* Populate the interpolated point array with the original
     * x-coordinates and extra interpolated x-coordinates for each
     * horizontal pixel that the line segment contains. Do this only for
     * pixels that are on screen */
    p = tracePtr->head;
    points[0].x = p->x;
    points[0].y = p->y;
    for (i = 1, p = p->next; p != NULL; p = p->next, i++) {
        /* Add the original x-coordinate */
        points[i].x = p->x;
        points[i].y = p->y;
    }
    spline = Blt_CreateCatromSpline(points, tracePtr->numPoints);
    if (spline == NULL) {
        return;                         /* Can't interpolate. */
    }
    Blt_GraphExtents(elemPtr, &exts);
    for (i = 0, p = tracePtr->head, q = p->next; q != NULL; q = q->next, i++) {
        Point2d p1, p2;
        double d, dp, dq;
        TracePoint *lastp;

        if (((p->flags | q->flags) & VISIBLE) == 0) {
            p = q;
            continue;                   /* Line segment isn't visible. */
        }

        /* Distance of the line entire segment. */
        d  = hypot(q->x - p->x, q->y - p->y);

        p1.x = p->x, p1.y = p->y;
        p2.x = q->x, p2.y = q->y;
        Blt_LineRectClip(&exts, &p1, &p2);
        
        /* Distance from last knot to p (start of generated points). */
        dp = hypot(p1.x - p->x, p1.y - p->y);
        /* Distance from last knot to q (end of generated points). */
        dq = hypot(p2.x - p->x, p2.y - p->y);

        dp += 2;
        dq -= 2;
        lastp = p;
        while (dp <= dq) {
            Point2d p1;
            TracePoint *t;

            /* Point is indicated by its interval and parameter u which is
             * the distance [0..1] of the point on the line segment. */
            p1 = Blt_EvaluateCatromSpline(spline, i, dp / d);
            t = NewPoint(elemPtr, p1.x, p1.y, p->index);
            /* Insert the new point in to line segment. */
            t->next = lastp->next; 
            lastp->next = t;
            lastp = t;
            tracePtr->numPoints++;
            dp += 2;
            assert(t->next == q);
        }
        p = q;
    }
    Blt_Free(points);
    Blt_FreeCatromSpline(spline);
}

/*
 *---------------------------------------------------------------------------
 *
 * SmoothElement --
 *
 *      Computes a cubic or quadratic spline and adds extra points to the
 *      list of coordinates for smoothing.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Points are added to the list of screen coordinates.
 *
 *---------------------------------------------------------------------------
 */
static void
SmoothElement(LineElement *elemPtr)
{
    Blt_ChainLink link;

    FixTraces(elemPtr->traces);
    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;

        tracePtr = Blt_Chain_GetValue(link);
        if (tracePtr->numPoints < 2) {
            continue;
        }
        switch (elemPtr->smooth) {
        case SMOOTH_STEP:
            GenerateSteps(tracePtr);
            break;

        case SMOOTH_QUADRATIC:
        case SMOOTH_NATURAL:
            if (tracePtr->numPoints > 2) {
                GenerateSpline(tracePtr);
            }
            break;

        case SMOOTH_QUADRATIC | SMOOTH_PARAMETRIC:
        case SMOOTH_NATURAL | SMOOTH_PARAMETRIC:
            if (tracePtr->numPoints > 2) {
                GenerateParametricSplineOld(tracePtr);
            }
            break;

        case SMOOTH_CATROM:
            if (tracePtr->numPoints > 2) {
                GenerateCatromSpline(tracePtr);
            }
            break;

        default:
            break;
        }
    }
}


static double
DistanceToLineProc(
    int x, int y,                       /* Sample X-Y coordinate. */
    Point2d *p, Point2d *q,             /* End points of the line segment. */
    Point2d *t)                         /* (out) Point on line segment. */
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

static double
DistanceToXProc(
    int x, int y,                       /* Test X-Y coordinate. */
    Point2d *p, 
    Point2d *q,                         /* End points of the line segment. */
    Point2d *t)                         /* (out) Point on line segment. */
{
    double dx, dy;
    double d;

    if (p->x > q->x) {
        if ((x > p->x) || (x < q->x)) {
            return DBL_MAX;             /* X-coordinate outside line segment. */
        }
    } else {
        if ((x > q->x) || (x < p->x)) {
            return DBL_MAX;             /* X-coordinate outside line segment. */
        }
    }
    dx = p->x - q->x;
    dy = p->y - q->y;
    t->x = (double)x;
    if (FABS(dx) < DBL_EPSILON) {
        double d1, d2;
        /* 
         * Same X-coordinate indicates a vertical line.  Pick the closest end
         * point.
         */
        d1 = p->y - y;
        d2 = q->y - y;
        if (FABS(d1) < FABS(d2)) {
            t->y = p->y, d = d1;
        } else {
            t->y = q->y, d = d2;
        }
    } else if (FABS(dy) < DBL_EPSILON) {
        /* Horizontal line. */
        t->y = p->y, d = p->y - y;
    } else {
        double m, b;
                
        m = dy / dx;
        b = p->y - (m * p->x);
        t->y = (x * m) + b;
        d = y - t->y;
    }
   return FABS(d);
}

static double
DistanceToYProc(
    int x, int y,                       /* Test X-Y coordinate. */
    Point2d *p, Point2d *q,             /* End points of the line segment. */
    Point2d *t)                         /* (out) Point on line segment. */
{
    double dx, dy;
    double d;

    if (p->y > q->y) {
        if ((y > p->y) || (y < q->y)) {
            return DBL_MAX;
        }
    } else {
        if ((y > q->y) || (y < p->y)) {
            return DBL_MAX;
        }
    }
    dx = p->x - q->x;
    dy = p->y - q->y;
    t->y = y;
    if (FABS(dy) < DBL_EPSILON) {
        double d1, d2;

        /* Save Y-coordinate indicates an horizontal line. Pick the closest end
         * point. */
        d1 = p->x - x;
        d2 = q->x - x;
        if (FABS(d1) < FABS(d2)) {
            t->x = p->x, d = d1;
        } else {
            t->x = q->x, d = d2;
        }
    } else if (FABS(dx) < DBL_EPSILON) {
        /* Vertical line. */
        t->x = p->x, d = p->x - x;
    } else {
        double m, b;
        
        m = dy / dx;
        b = p->y - (m * p->x);
        t->x = (y - b) / m;
        d = x - t->x;
    }
    return FABS(d);
}

/*
 *---------------------------------------------------------------------------
 *
 * NearestPoint --
 *
 *      Find the element whose data point is closest to the given screen
 *      coordinate.
 *
 * Results:
 *      If a new minimum distance is found, the information regarding
 *      it is returned via nearestPtr.
 *
 *---------------------------------------------------------------------------
 */
static void
NearestPoint(
    LineElement *elemPtr,               /* Line element to be searched. */
    NearestElement *nearestPtr)         /* Assorted information related to
                                         * searching for the nearest point */
{
    Blt_ChainLink link;

    /*
     * Instead of testing each data point in graph coordinates, look at the
     * points of each trace (mapped screen coordinates). The advantages are
     *   1) only examine points that are visible (unclipped), and
     *   2) the computed distance is already in screen coordinates.
     */
    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;
        TracePoint *p;

        tracePtr = Blt_Chain_GetValue(link);
        for (p = tracePtr->head; p != NULL; p = p->next) {
            double dx, dy;
            double d;

            if ((p->flags & KNOT) == 0) {
                continue;
            }
            if (!PLAYING(tracePtr, p->index)) {
                continue;
            }
            dx = (double)(p->x - nearestPtr->x);
            dy = (double)(p->y - nearestPtr->y);
            if (nearestPtr->along == NEAREST_SEARCH_XY) {
                d = hypot(dx, dy);
            } else if (nearestPtr->along == NEAREST_SEARCH_X) {
                d = dx;
            } else if (nearestPtr->along == NEAREST_SEARCH_Y) {
                d = dy;
            } else {
                /* This can't happen */
                continue;
            }
            if (d < nearestPtr->distance) {
                nearestPtr->index = p->index;
                nearestPtr->item = elemPtr;
                nearestPtr->point.x = elemPtr->x.values[nearestPtr->index];
                nearestPtr->point.y = elemPtr->y.values[nearestPtr->index];
                nearestPtr->distance = d;
            }
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NearestSegment --
 *
 *      Find the line segment closest to the given window coordinate in the
 *      element.
 *
 * Results:
 *      If a new minimum distance is found, the information regarding it is
 *      returned via searchPtr.
 *
 *---------------------------------------------------------------------------
 */
static void
NearestSegment(
    Graph *graphPtr,                    /* Graph widget record */
    LineElement *elemPtr,
    NearestElement *nearestPtr,         /* Info about closest point in
                                         * element */
    DistanceProc *distProc)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;
        TracePoint *p, *q;

        tracePtr = Blt_Chain_GetValue(link);
        for (p = tracePtr->head, q = p->next; q != NULL; q = q->next) {
            Point2d p1, p2, b;
            double d;

            if (!PLAYING(tracePtr, p->index)) {
                continue;
            }
            p1.x = p->x, p1.y = p->y;
            p2.x = q->x, p2.y = q->y;
            d = (*distProc)(nearestPtr->x, nearestPtr->y, &p1, &p2, &b);
            if (d < nearestPtr->distance) {
                nearestPtr->index = p->index;
                nearestPtr->distance = d;
                nearestPtr->item = elemPtr;
                nearestPtr->point = Blt_InvMap2D(graphPtr, b.x, b.y, 
                        &elemPtr->axes);
            }
            p = q;
        }
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * NearestProc --
 *
 *      Find the closest point or line segment (if interpolated) to the given
 *      window coordinate in the line element.
 *
 * Results:
 *      Returns the distance of the closest point among other information.
 *
 *---------------------------------------------------------------------------
 */
static void
NearestProc(Graph *graphPtr, Element *basePtr, NearestElement *nearestPtr)
{
    LineElement *elemPtr = (LineElement *)basePtr;
    int mode;

    mode = nearestPtr->mode;
    if (mode == NEAREST_SEARCH_AUTO) {
        LinePen *penPtr;

        penPtr = NORMALPEN(elemPtr);
        mode = NEAREST_SEARCH_POINTS;
        if ((NUMBEROFPOINTS(elemPtr) > 1) && (penPtr->traceWidth > 0)) {
            mode = NEAREST_SEARCH_TRACES;
        }
    }
    if (mode == NEAREST_SEARCH_POINTS) {
        NearestPoint(elemPtr, nearestPtr);
    } else {
        DistanceProc *distProc;
        int found;

        if (nearestPtr->along == NEAREST_SEARCH_X) {
            distProc = DistanceToXProc;
        } else if (nearestPtr->along == NEAREST_SEARCH_Y) {
            distProc = DistanceToYProc;
        } else {
            distProc = DistanceToLineProc;
        }
        NearestSegment(graphPtr, elemPtr, nearestPtr, distProc);
        found = (nearestPtr->distance <= nearestPtr->maxDistance);
        if ((!found) && (nearestPtr->along != NEAREST_SEARCH_XY)) {
            NearestPoint(elemPtr, nearestPtr);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FindProc --
 *
 *      Find all the points within the designate circle on the screen.
 *
 * Results:
 *      Returns a list of the indices of the points that are within the
 *      search radius.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain 
FindProc(Graph *graphPtr, Element *basePtr, int x, int y, int r)
{
    LineElement *elemPtr = (LineElement *)basePtr;
    Blt_ChainLink link;
    Blt_Chain chain;

    /*
     * Instead of testing each data point in graph coordinates, look at the
     * points of each trace (mapped screen coordinates). The advantages are
     *   1) only examine points that are visible (unclipped), and
     *   2) the computed distance is already in screen coordinates.
     */
    chain = Blt_Chain_Create();
    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;
        TracePoint *p;

        tracePtr = Blt_Chain_GetValue(link);
        for (p = tracePtr->head; p != NULL; p = p->next) {
            double dx, dy;
            double d;

            if ((p->flags & KNOT) == 0) {
                continue;
            }
            if (!PLAYING(tracePtr, p->index)) {
                continue;
            }
            dx = (double)(x - p->x);
            dy = (double)(y - p->y);
            d = hypot(dx, dy);
            if (d < r) {
                Blt_Chain_Append(chain, (ClientData)((long)p->index));
            }
        }
    }
    return chain;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExtentsProc --
 *
 *      Retrieves the range of the line element
 *
 * Results:
 *      Returns the number of data points in the element.
 *
 *---------------------------------------------------------------------------
 */
static void
ExtentsProc(Element *basePtr)
{
    LineElement *elemPtr = (LineElement *)basePtr;
    double xMin, xMax, yMin, yMax;
    double xPosMin, yPosMin;
    Region2d exts;
    int i;
    int np;

    exts.top = exts.left = DBL_MAX;
    exts.bottom = exts.right = -DBL_MAX;
    np = NUMBEROFPOINTS(elemPtr);
    if (np < 1) {
        return;
    } 
    xMin = yMin = xPosMin = yPosMin = DBL_MAX;
    xMax = yMax = -DBL_MAX;
    for (i = 0; i < np; i++) {
        double x, y;

        x = elemPtr->x.values[i];
        y = elemPtr->y.values[i];
        if ((!FINITE(x)) || (!FINITE(y))) {
            continue;                   /* Ignore holes in the data. */
        }
        if (x < xMin) {
            xMin = x;
        } 
        if (x > xMax) {
            xMax = x;
        }
        if ((x > 0.0) && (xPosMin > x)) {
            xPosMin = x;
        }
        if (y < yMin) {
            yMin = y;
        } 
        if (y > yMax) {
            yMax = y;
        }
        if ((y > 0.0) && (yPosMin > y)) {
            yPosMin = y;
        }
    }
    exts.right = xMax;
    if ((xMin <= 0.0) && (IsLogScale(elemPtr->axes.x))) {
        exts.left = xPosMin;
    } else {
        exts.left = xMin;
    }
    exts.bottom = yMax;
    if ((yMin <= 0.0) && (IsLogScale(elemPtr->axes.y))) {
        exts.top = yPosMin;
    } else {
        exts.top = yMin;
    }
#ifdef notdef
    /* Correct the data limits for error bars */
    if (elemPtr->xError.numValues > 0) {
        int i;
        
        np = MIN(elemPtr->xError.numValues, np);
        for (i = 0; i < np; i++) {
            double x;

            x = elemPtr->x.values[i] + elemPtr->xError.values[i];
            if (x > exts.right) {
                exts.right = x;
            }
            x = elemPtr->x.values[i] - elemPtr->xError.values[i];
            if (IsLogScale(elemPtr->axes.x)) {
                if (x < 0.0) {
                    x = -x;             /* Mirror negative values, instead of
                                         * ignoring them. */
                }
                if ((x > DBL_MIN) && (x < exts.left)) {
                    exts.left = x;
                }
            } else if (x < exts.left) {
                exts.left = x;
            }
        }                    
    } else {
        if ((elemPtr->xHigh.numValues > 0) && 
            (elemPtr->xHigh.max > exts.right)) {
            exts.right = elemPtr->xHigh.max;
        }
        if (elemPtr->xLow.numValues > 0) {
            double left;
            
            if ((elemPtr->xLow.min <= 0.0) && (IsLogScale(elemPtr->axes.x))) {
                left = Blt_FindElemValuesMinimum(&elemPtr->xLow, DBL_MIN);
            } else {
                left = elemPtr->xLow.min;
            }
            if (left < exts.left) {
                exts.left = left;
            }
        }
    }
    
    if (elemPtr->yError.numValues > 0) {
        int i;
        
        np = MIN(elemPtr->yError.numValues, np);
        for (i = 0; i < np; i++) {
            double y;

            y = elemPtr->y.values[i] + elemPtr->yError.values[i];
            if (y > exts.bottom) {
                exts.bottom = y;
            }
            y = elemPtr->y.values[i] - elemPtr->yError.values[i];
            if (IsLogScale(elemPtr->axes.y)) {
                if (y < 0.0) {
                    y = -y;             /* Mirror negative values, instead of
                                         * ignoring them. */
                }
                if ((y > DBL_MIN) && (y < exts.left)) {
                    exts.top = y;
                }
            } else if (y < exts.top) {
                exts.top = y;
            }
        }                    
    } else {
        if ((elemPtr->yHigh.numValues > 0) && 
            (elemPtr->yHigh.max > exts.bottom)) {
            exts.bottom = elemPtr->yHigh.max;
        }
        if (elemPtr->yLow.numValues > 0) {
            double top;
            
            if ((elemPtr->yLow.min <= 0.0) && (IsLogScale(elemPtr->axes.y))) {
                top = Blt_FindElemValuesMinimum(&elemPtr->yLow, DBL_MIN);
            } else {
                top = elemPtr->yLow.min;
            }
            if (top < exts.top) {
                exts.top = top;
            }
        }
    }
#endif
    if (elemPtr->axes.x->dataRange.min > exts.left) {
        elemPtr->axes.x->dataRange.min = exts.left;
    }
    if (elemPtr->axes.x->dataRange.max < exts.right) {
        elemPtr->axes.x->dataRange.max = exts.right;
    }
    if (elemPtr->axes.y->dataRange.min > exts.top) {
        elemPtr->axes.y->dataRange.min = exts.top;
    }
    if (elemPtr->axes.y->dataRange.max < exts.bottom) {
        elemPtr->axes.y->dataRange.max = exts.bottom;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BackgroundChangedProc
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
BackgroundChangedProc(ClientData clientData)
{
    Element *elemPtr = clientData;
    Graph *graphPtr;

    graphPtr = elemPtr->obj.graphPtr;
    if (graphPtr->tkwin != NULL) {
        graphPtr->flags |= REDRAW_WORLD;
        Blt_EventuallyRedrawGraph(graphPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BrushChangedProc
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
BrushChangedProc(ClientData clientData, Blt_PaintBrush brush)
{
    Element *elemPtr = clientData;
    Graph *graphPtr;

    graphPtr = elemPtr->obj.graphPtr;
    if (graphPtr->tkwin != NULL) {
        graphPtr->flags |= REDRAW_WORLD;
        Blt_EventuallyRedrawGraph(graphPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureProc --
 *
 *      Sets up the appropriate configuration parameters in the GC.  It is
 *      assumed the parameters have been previously set by a call to
 *      Blt_ConfigureWidget.
 *
 * Results:
 *      The return value is a standard TCL result.  If TCL_ERROR is returned,
 *      then interp->result contains an error message.
 *
 * Side effects:
 *      Configuration information such as line width, line style, color
 *      etc. get set in a new GC.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ConfigureProc(Graph *graphPtr, Element *basePtr)
{
    LineElement *elemPtr = (LineElement *)basePtr;
    unsigned long gcMask;
    XGCValues gcValues;
    GC newGC;
    Blt_ChainLink link;
    LineStyle *stylePtr;

    if (ConfigurePenProc(graphPtr, (Pen *)&elemPtr->builtinPen) != TCL_OK) {
        return TCL_ERROR;
    }
    /*
     * Point to the static normal/active pens if no external pens have been
     * selected.
     */
    link = Blt_Chain_FirstLink(elemPtr->styles);
    if (link == NULL) {
        link = Blt_Chain_AllocLink(sizeof(LineStyle));
        Blt_Chain_LinkAfter(elemPtr->styles, link, NULL);
    } 
    stylePtr = Blt_Chain_GetValue(link);
    stylePtr->penPtr = NORMALPEN(elemPtr);
    if (elemPtr->areaBg != NULL) {
        Blt_Bg_SetChangedProc(elemPtr->areaBg, BackgroundChangedProc, elemPtr);
    }
    if (elemPtr->brush != NULL) {
        Blt_CreateBrushNotifier(elemPtr->brush, BrushChangedProc, elemPtr);
    }
    /*
     * Set the outline GC for this pen: GCForeground is outline color.
     */
    gcMask = 0;
    if (elemPtr->fillFgColor != NULL) {
        gcMask |= GCForeground;
        gcValues.foreground = elemPtr->fillFgColor->pixel;
    }
    if (elemPtr->fillBgColor != NULL) {
        gcMask |= GCBackground;
        gcValues.background = elemPtr->fillBgColor->pixel;
    }
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (elemPtr->fillGC != NULL) {
        Tk_FreeGC(graphPtr->display, elemPtr->fillGC);
    }
    elemPtr->fillGC = newGC;

    if (Blt_ConfigModified(elemPtr->configSpecs, "-scalesymbols", 
                           (char *)NULL)) {
        elemPtr->flags |= (MAP_ITEM | SCALE_SYMBOL);
    }
    if (Blt_ConfigModified(elemPtr->configSpecs, "-pixels", "-trace", 
        "-*data", "-smooth", "-map*", "-label", "-hide", "-x", "-y", 
        "-areabackground", (char *)NULL)) {
        elemPtr->flags |= MAP_ITEM;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * MapAreaUnderTrace --
 *
 *      Maps the polygon representing the area under the curve of each the
 *      trace. This must be done after the spline interpolation but before
 *      mapping polylines which may split the traces further.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory is freed and allocated for the index array.
 *
 *---------------------------------------------------------------------------
 */
static void
MapAreaUnderTrace(Trace *tracePtr)
{
    LineElement *elemPtr = tracePtr->elemPtr;
    Graph *graphPtr;
    int n;
    Point2d *points, *clipPts;
    Region2d exts;

    n = tracePtr->numPoints + 3;
    points = Blt_AssertMalloc(sizeof(Point2d) * n);
    graphPtr = elemPtr->obj.graphPtr;
    if (graphPtr->flags & INVERTED) {
        double xMin;
        TracePoint *p;
        int count;

        count = 0;
        xMin = (double)elemPtr->axes.y->screenMin;
        for (p = tracePtr->head; p != NULL; p = p->next) {
            points[count].x = p->x + 1;
            points[count].y = p->y;
            if (points[count].x < xMin) {
                xMin = points[count].x;
            }
            count++;
        }       
        /* Add edges to make (if necessary) the polygon fill to the bottom of
         * plotting window */
        points[count].x = xMin;
        points[count].y = points[count - 1].y;
        count++;
        points[count].x = xMin;
        points[count].y = points[0].y; 
        count++;
        points[count] = points[0];
    } else {
        double yMax;
        TracePoint *p;
        int count;

        count = 0;
        yMax = (double)elemPtr->axes.y->bottom + 2;
        for (p = tracePtr->head; p != NULL; p = p->next) {
            points[count].x = p->x + 1;
            points[count].y = p->y + 1;
            if (points[count].y > yMax) {
                yMax = points[count].y;
            }
            count++;
        }       
        /* Add edges to extend the fill polygon to the bottom of plotting
         * window */
        points[count].x = points[count - 1].x;
        points[count].y = yMax;
        count++;
        points[count].x = points[0].x; 
        points[count].y = yMax;
        count++;
        points[count] = points[0];
    }
    Blt_GraphExtents(tracePtr->elemPtr, &exts);

    clipPts = Blt_AssertMalloc(sizeof(Point2d) * n * 3);
    n = Blt_PolyRectClip(&exts, points, n - 1, clipPts);
    Blt_Free(points);

    if (n < 3) {
        Blt_Free(clipPts);
    } else {
        tracePtr->fillPts = clipPts;
        tracePtr->numFillPts = n;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapAreaUnderCurve --
 *
 *      Maps the polygon representing the area under the curve of each the
 *      trace. This must be done after the spline interpolation but before
 *      mapping polylines which may split the traces further.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory is freed and allocated for the index array.
 *
 *---------------------------------------------------------------------------
 */
static void
MapAreaUnderCurve(LineElement *elemPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;

        tracePtr = Blt_Chain_GetValue(link);
        MapAreaUnderTrace(tracePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapActiveSymbols --
 *
 *      Creates an array of points of the active graph coordinates.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory is freed and allocated for the active point array.
 *
 *---------------------------------------------------------------------------
 */
static void
MapActiveSymbols(LineElement *elemPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;
        TracePoint *p;

        tracePtr = Blt_Chain_GetValue(link);
        for (p = tracePtr->head; p != NULL; p = p->next) {
            Blt_HashEntry *hPtr;
            long lindex;

            p->flags &= ~ACTIVE_POINT;
            lindex = (long)p->index;
            hPtr = Blt_FindHashEntry(&elemPtr->activeTable, (char *)lindex);
            if (hPtr != NULL) {
                p->flags |= ACTIVE_POINT;
            }
        }
    }
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * ReducePoints --
 *
 *      FIXME: Fix to reduce point list.
 *
 *      Generates a coordinate array of transformed screen coordinates from
 *      the data points.
 *
 * Results:
 *      The transformed screen coordinates are returned.
 *
 * Side effects:
 *      Memory is allocated for the coordinate array.
 *
 *---------------------------------------------------------------------------
 */
static void
ReducePoints(MapInfo *mapPtr, double tolerance)
{
    long i, numPoints;
    Point2d *screenPts, *origPts;
    long *map, *simple;

    simple    = Blt_AssertMalloc(tracePtr->numPoints * sizeof(long));
    map       = Blt_AssertMalloc(tracePtr->numPoints * sizeof(long));
    screenPts = Blt_AssertMalloc(tracePtr->numPoints * sizeof(Point2d));
    origPts = Blt_AssertMalloc(tracePtr->numPoints * sizeof(Point2d));

    numPoints = Blt_SimplifyLine(origPts, 0, tracePtr->numScreenPts - 1, 
        tolerance, simple);
    for (i = 0; i < numPoints; i++) {
        long k;

        k = simple[i];
        screenPts[i] = mapPtr->screenPts[k];
        map[i] = mapPtr->map[k];
    }
#ifdef notdef
    if (numPoints < mapPtr->numScreenPts) {
        fprintf(stderr, "reduced from %d to %d\n", mapPtr->numScreenPts,
                numPoints);
    }
#endif
    Blt_Free(mapPtr->screenPts);
    Blt_Free(mapPtr->map);
    Blt_Free(simple);
    mapPtr->screenPts = screenPts;
    mapPtr->map = map;
    mapPtr->numScreenPts = numPoints;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * MapErrorBars --
 *
 *      Creates two arrays of points and pen indices, filled with the screen
 *      coordinates of the visible
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory is freed and allocated for the index array.
 *
 *---------------------------------------------------------------------------
 */
static void
MapErrorBars(LineElement *elemPtr)
{
    Graph *graphPtr;
    Blt_ChainLink link;
    Region2d exts;

    graphPtr = elemPtr->obj.graphPtr;
    Blt_GraphExtents(elemPtr, &exts);
    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;
        TracePoint *p;
        int errorCapWidth;

        tracePtr = Blt_Chain_GetValue(link);
        errorCapWidth = (tracePtr->penPtr->errorCapWidth > 0) 
            ? tracePtr->penPtr->errorCapWidth : tracePtr->penPtr->symbol.size;
        for (p = tracePtr->head; p != NULL; p = p->next) {
            double x, y;
            double xHigh, xLow, yHigh, yLow;
            int ec2;
            
            if ((p->flags & (KNOT | VISIBLE)) != (KNOT | VISIBLE)) {
                continue;               /* Error bars only at specified
                                         * points */
            }
            x = elemPtr->x.values[p->index];
            y = elemPtr->y.values[p->index];
            ec2 = errorCapWidth / 2;
            if (elemPtr->xHigh.numValues > p->index) {
                xHigh = elemPtr->xHigh.values[p->index];
            } else if (elemPtr->xError.numValues > p->index) {
                xHigh = elemPtr->x.values[p->index] + 
                    elemPtr->xError.values[p->index];
            } else {
                xHigh = Blt_NaN();
            }
            if (FINITE(xHigh)) {
                Point2d high, p1, p2;
                /* 
                 *             |      
                 *   x,y ----xhigh,y      
                 *             |          
                 */
                
                p1 = high = Blt_Map2D(graphPtr, xHigh, y, &elemPtr->axes);
                p2.x = p->x, p2.y = p->y;
                /* Stem from the low x to point x at y. */
                if (Blt_LineRectClip(&exts, &p1, &p2)) {
                    TraceSegment *stem;

                    stem = NewSegment(elemPtr, p1.x, p1.y, p2.x, p2.y, p->index,
                                  p->flags | XHIGH);
                    AddSegment(tracePtr, stem);
                }
                /* Cap from high + errorCapWith  */
                if (graphPtr->flags &  INVERTED) {
                    p1.y = p2.y = high.y;
                    p1.x = high.x-ec2;
                    p2.x = high.x+ec2;
                } else {
                    p1.x = p2.x = high.x;
                    p1.y = high.y-ec2;
                    p2.y = high.y+ec2;
                }
                if (Blt_LineRectClip(&exts, &p1, &p2)) {
                    TraceSegment *cap;

                    cap = NewSegment(elemPtr, p1.x, p1.y, p2.x, p2.y,
                        p->index, p->flags | XHIGH);
                    AddSegment(tracePtr, cap);
                }
            }
            if (elemPtr->xLow.numValues > p->index) {
                xLow = elemPtr->xLow.values[p->index];
            } else if (elemPtr->xError.numValues > p->index) {
                xLow = elemPtr->x.values[p->index] - 
                    elemPtr->xError.values[p->index];
            } else {
                xLow = Blt_NaN();
            }
            if (FINITE(xLow)) {
                Point2d low, p1, p2;
                
                /* 
                 *     |      
                 *   xlow,y----x,y
                 *     |          
                 */
                p1 = low = Blt_Map2D(graphPtr, xLow, y, &elemPtr->axes);
                p2.x = p->x, p2.y = p->y;
                /* Stem from the low x to point x at y. */
                if (Blt_LineRectClip(&exts, &p1, &p2)) {
                    TraceSegment *stem;

                    stem = NewSegment(elemPtr, p1.x, p1.y, p2.x, p2.y, p->index,
                        p->flags | XLOW);
                    AddSegment(tracePtr, stem);
                }
                /* Cap from low + errorCapWith  */
                if (graphPtr->flags & INVERTED) {
                    p1.y = p2.y = low.y;
                    p1.x = low.x-ec2;
                    p2.x = low.x+ec2;
                } else {
                    p1.x = p2.x = low.x;
                    p1.y = low.y-ec2;
                    p2.y = low.y+ec2;
                }
                if (Blt_LineRectClip(&exts, &p1, &p2)) {
                    TraceSegment *cap;

                    cap = NewSegment(elemPtr, p1.x, p1.y, p2.x, p2.y, p->index,
                        p->flags | XLOW);
                    AddSegment(tracePtr, cap);
                }
            }
            if (elemPtr->yHigh.numValues > p->index) {
                yHigh = elemPtr->yHigh.values[p->index];
            } else if (elemPtr->yError.numValues > p->index) {
                yHigh = elemPtr->x.values[p->index] - 
                    elemPtr->yError.values[p->index];
            } else {
                yHigh = Blt_NaN();
            }
            if (FINITE(yHigh)) {
                Point2d high, p1, p2;
                
                /* 
                 *   --x,yhigh--
                 *        | 
                 *        |
                 *       x,y
                 */
                p1 = high = Blt_Map2D(graphPtr, x, yHigh, &elemPtr->axes);
                p2.x = p->x, p2.y = p->y;
                /* Stem from the low x to point x at y. */
                if (Blt_LineRectClip(&exts, &p1, &p2)) {
                    TraceSegment *stem;

                    stem = NewSegment(elemPtr, p1.x, p1.y, p2.x, p2.y, p->index,
                        p->flags | YHIGH);
                    AddSegment(tracePtr, stem);
                }
                if (graphPtr->flags & INVERTED) {
                    p1.x = p2.x = high.x;
                    p1.y = high.y-ec2;
                    p2.y = high.y+ec2;
                } else {
                    p1.y = p2.y = high.y;
                    p1.x = high.x-ec2;
                    p2.x = high.x+ec2;
                }
                if (Blt_LineRectClip(&exts, &p1, &p2)) {
                    TraceSegment *cap;

                    cap = NewSegment(elemPtr, p1.x, p1.y, p2.x, p2.y, p->index,
                        p->flags | YHIGH);
                    AddSegment(tracePtr, cap);
                }
            }
            if (elemPtr->yLow.numValues > p->index) {
                yLow = elemPtr->yLow.values[p->index];
            } else if (elemPtr->yError.numValues > p->index) {
                yLow = elemPtr->x.values[p->index] - 
                    elemPtr->yError.values[p->index];
            } else {
                yLow = Blt_NaN();
            }
            if (FINITE(yLow)) {
                Point2d low, p1, p2;
                /* 
                 *       x,y
                 *        | 
                 *        |
                 *    --ylow,y--
                 */
                p1 = low = Blt_Map2D(graphPtr, x, yLow, &elemPtr->axes);
                p2.x = p->x, p2.y = p->y;
                /* Stem from the low x to point x at y. */
                if (Blt_LineRectClip(&exts, &p1, &p2)) {
                    TraceSegment *stem;

                    stem = NewSegment(elemPtr, p1.x, p1.y, p2.x, p2.y, p->index,
                        p->flags | YLOW);
                    AddSegment(tracePtr, stem);
                }
                if (graphPtr->flags & INVERTED) {
                    p1.x = p2.x = low.x;
                    p1.y = low.y-ec2;
                    p2.y = low.y+ec2;
                } else {
                    p1.y = p2.y = low.y;
                    p1.x = low.x-ec2;
                    p2.x = low.x+ec2;
                }
                if (Blt_LineRectClip(&exts, &p1, &p2)) {
                    TraceSegment *cap;

                    cap = NewSegment(elemPtr, p1.x, p1.y, p2.x, p2.y, p->index,
                        p->flags | YLOW);
                    AddSegment(tracePtr, cap);
                }
            }
        }
    }
}
    
/*
 *---------------------------------------------------------------------------
 *
 * MapPolyline --
 *
 *      Adjust the trace by testing each segment of the trace to the graph
 *      area.  If the segment is totally off screen, remove it from the trace.
 *      If one end point is off screen, replace it with the clipped point.
 *      Create new traces as necessary.
 *      
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory is  allocated for the line segment array.
 *
 *---------------------------------------------------------------------------
 */
static void
MapPolyline(LineElement *elemPtr, Trace *tracePtr)
{
    TracePoint *p, *q;
    Region2d exts;

    Blt_GraphExtents(elemPtr, &exts);
    for (p = tracePtr->head, q = p->next; q != NULL; q = q->next) {
        if (p->flags & q->flags & VISIBLE) {
            p = q;
            continue;                   /* Segment is visible. */
        }
        /* Clip required. */
        if (p->flags & VISIBLE) {       /* Last point is off screen. */
            Point2d p1, p2;

            p1.x = p->x, p1.y = p->y;
            p2.x = q->x, p2.y = q->y;
            if (Blt_LineRectClip(&exts, &p1, &p2)) {
                TracePoint *t;
                Trace *newPtr;

                /* Last point is off screen.  Add the clipped end the current
                 * trace. */
                t = NewPoint(elemPtr, p2.x, p2.y, q->index);
                t->flags = VISIBLE;
                tracePtr->flags |= RECOUNT;
                tracePtr->tail = t;
                p->next = t;            /* Point t terminates the trace. */

                /* Create a new trace and attach the current chain to it. */
                newPtr = NewTrace(elemPtr);
                newPtr->flags |= RECOUNT;
                newPtr->head = newPtr->tail = q;
                newPtr->penPtr = tracePtr->penPtr;
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
                /* The replaced point is now visible but no longer a knot. */
                p->flags |= VISIBLE;
                p->flags &= ~KNOT;
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
 * MapPolylines --
 *
 *      Creates an array of line segments of the graph coordinates.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory is  allocated for the line segment array.
 *
 *---------------------------------------------------------------------------
 */
static void
MapPolylines(LineElement *elemPtr)
{
    Blt_ChainLink link;

    /* Step 1: Process traces by clipping them against the plot area. */
    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;

        tracePtr = Blt_Chain_GetValue(link);
        if ((tracePtr->numPoints < 2) || (tracePtr->penPtr->traceWidth == 0)) {
            continue;
        }
        MapPolyline(elemPtr, tracePtr);
    }
    /* Step 2: Fix traces that have been split. */
    FixTraces(elemPtr->traces);
}

static LinePen *
WeightToPen(LineElement *elemPtr, double weight)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_LastLink(elemPtr->styles); link != NULL;  
         link = Blt_Chain_PrevLink(link)) {
        LineStyle *stylePtr;
        
        stylePtr = Blt_Chain_GetValue(link);
        if (stylePtr->weight.range > 0.0) {
            double norm;
            
            norm = (weight - stylePtr->weight.min) / stylePtr->weight.range;
            if (((norm - 1.0) <= DBL_EPSILON) && 
                (((1.0 - norm) - 1.0) <= DBL_EPSILON)) {
                return stylePtr->penPtr;
            }
        }
    }
    return NORMALPEN(elemPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * MapStyles --
 *
 *      Splits traces based on the pen used.  May create many more traces
 *      if the traces change pens frequently.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      New traces may be created.  Traces may be split.
 *
 *---------------------------------------------------------------------------
 */
static void
MapStyles(LineElement *elemPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;
        TracePoint *p, *q;
        LinePen *penPtr;

        tracePtr = Blt_Chain_GetValue(link);
        /* For each point in the trace, see what pen it corresponds to. */
        p = tracePtr->head;
        
        if (elemPtr->w.numValues > p->index) {
            penPtr = WeightToPen(elemPtr,elemPtr->w.values[p->index]);
        } else {
            penPtr = NORMALPEN(elemPtr);
        }
        tracePtr->penPtr = penPtr;

        for (q = p->next; q != NULL; q = q->next) {
            LinePen *penPtr;

            if (elemPtr->w.numValues > q->index) {
                penPtr = WeightToPen(elemPtr, elemPtr->w.values[q->index]);
            } else {
                penPtr = NORMALPEN(elemPtr);
            }
            if (penPtr != tracePtr->penPtr) {
                TracePoint *t;
                /* 
                 * If the mapped style is not the current style, create a new
                 * trace of that style and break the trace.
                 */

                /* Create a copy of the current point and insert it as new end
                 * point for the current trace.  This point will not be a
                 * knot. */
                t = NewPoint(elemPtr, q->x, q->y, q->index);
                tracePtr->tail = t;
                p->next = t;            /* Point t terminates the trace. */

                /* Now create a new trace.  The first point will be the
                 * current point. The pen for the trace is the current pen. */
                tracePtr->flags |= RECOUNT;
                tracePtr = NewTrace(elemPtr);
                tracePtr->penPtr = penPtr;
                AppendPoint(tracePtr, q);
                tracePtr->flags |= RECOUNT;
            }
            p = q;
        }
    }
    /* Step 2: Fix traces that have been split. */
    FixTraces(elemPtr->traces);
}

/*
 * MapProc --
 *
 *      Converts the graph coordinates into screen coordinates representing
 *      the line element.  The screen coordinates are stored in a linked list
 *      or points representing a set of connected point (a trace).  Generated
 *      points may be added to the traces because of smoothing.  Points may be
 *      removed if the are off screen.  A trace may contain one or more points.
 *
 *      Originally all points are in a single list (trace).  They are
 *      processed and possibly split into new traces until each trace
 *      represents a contiguous set of on-screen points using the same line
 *      style (pen).  New traces are broken when the points go off screen or
 *      use a different line style than the previous point.
 *      
 * Results:
 *      None.
 *
 * Side effects:
 *      Memory is (re)allocated for the points.
 */
static void
MapProc(Graph *graphPtr, Element *basePtr)
{
    LineElement *elemPtr = (LineElement *)basePtr;
    int n;

    ResetElement(elemPtr);
    n = NUMBEROFPOINTS(elemPtr);
    if (n < 1) {
        return;                         /* No data points */
    }
    GetScreenPoints(elemPtr);
    elemPtr->smooth = elemPtr->reqSmooth;
    if (n > 1) {
        /* Note to users: For scatter plots, don't turn on smoothing.  We
         * can't check if the traceWidth is 0, because we haven't mapped
         * styles yet.  But we need to smooth before the traces get split by
         * styles. */
        if (elemPtr->smooth != SMOOTH_NONE) {
            SmoothElement(elemPtr);
        }
#ifdef notdef
        if (elemPtr->rTolerance > 0.0) {
            ReducePoints(&mi, elemPtr->rTolerance);
        }
#endif
    }
    if ((elemPtr->areaBg != NULL) || (elemPtr->brush != NULL) ||
        (elemPtr->zAxisPtr != NULL)) {
        MapAreaUnderCurve(elemPtr);
    }
    /* Split traces based upon style.  The pen associated with the trace
     * determines if polylines or symbols are required, the size of the
     * errorbars and symbols, etc. */
    MapStyles(elemPtr);
    if (n > 1) {
        MapPolylines(elemPtr);
    }
    if (elemPtr->numActiveIndices >= 0) {
        MapActiveSymbols(elemPtr);
    }
    /* This has to be done last since we don't split the errorbar segments
     * when we split a trace.  */
    MapErrorBars(elemPtr);
}


static int
GradientCalcProc(ClientData clientData, int x, int y, double *valuePtr)
{
    Graph *graphPtr;
    LineElement *elemPtr = clientData;
    double value;
    Point2d point;
    AxisRange *rangePtr;
    
    graphPtr = elemPtr->obj.graphPtr;
    point = Blt_InvMap2D(graphPtr, x, y, &elemPtr->axes);

    if (elemPtr->zAxisPtr->obj.classId == CID_AXIS_Y) {
        value = point.y;
    } else if (elemPtr->zAxisPtr->obj.classId == CID_AXIS_X) {
        value = point.x;
    } else {
        return TCL_ERROR;
    }
    rangePtr = &elemPtr->zAxisPtr->dataRange;
    *valuePtr = (value - rangePtr->min) / rangePtr->range;
    return TCL_OK;
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
PaintPolygon(Graph *graphPtr, Drawable drawable, LineElement *elemPtr, 
             int numPoints, XPoint *points)
{
    Blt_PaintBrush brush;
    Blt_Painter painter;
    Blt_Picture picture;
    Point2d *vertices;
    int i;
    int w, h;
    int x1, x2, y1, y2;

    if (numPoints < 3) {
        return;                         /* Not enough points for polygon */
    }
    /* Grab the rectangular background that covers the polygon. */
    GetPolygonBBox(points, numPoints, &x1, &x2, &y1, &y2);
    w = x2 - x1 + 1;
    h = y2 - y1 + 1;
    picture = Blt_CreatePicture(w, h);
    if (picture == NULL) {
        return;                         /* Background is obscured. */
    }
    Blt_BlankPicture(picture, 0x0);
    Blt_Picture_SetCompositeFlag(picture);
    vertices = Blt_AssertMalloc(numPoints * sizeof(Point2d));
    /* Translate the polygon */
    for (i = 0; i < numPoints; i++) {
        vertices[i].x = points[i].x - x1;
        vertices[i].y = points[i].y - y1;
    }
    if ((elemPtr->zAxisPtr != NULL) && (elemPtr->zAxisPtr->palette != NULL)) {
        brush = Blt_NewLinearGradientBrush();
        Blt_SetBrushOrigin(brush, -x1, -y1);
        Blt_SetLinearGradientBrushPalette(brush, elemPtr->zAxisPtr->palette);
        Blt_SetLinearGradientBrushCalcProc(brush, GradientCalcProc, elemPtr);
    } else if (elemPtr->brush != NULL) {
        brush = elemPtr->brush;
        Blt_SetBrushRegion(brush, 0, 0, w, h);
    } else if (elemPtr->areaBg != NULL) {
        brush = Blt_Bg_PaintBrush(elemPtr->areaBg);
        Blt_SetBrushRegion(brush, 0, 0, w, h);
    } else {
        Blt_Free(vertices);
        return;
    }
    fprintf(stderr, "Alpha=%x\n", Blt_GetBrushAlpha(brush));
    Blt_PaintPolygon(picture, numPoints, vertices, brush);
    if ((elemPtr->zAxisPtr != NULL) && (elemPtr->zAxisPtr->palette != NULL)) {
        Blt_FreeBrush(brush);
    }
    Blt_Free(vertices);
    painter = Blt_GetPainter(graphPtr->tkwin, 1.0);
    Blt_PaintPicture(painter, drawable, picture, 0, 0, w, h, x1, y1, 0);
    Blt_FreePicture(picture);
}

/* 
 * DrawAreaUnderCurve --
 *
 *      Draws the polygons under the traces.
 */
static void
DrawAreaUnderCurve(Graph *graphPtr, Drawable drawable, LineElement *elemPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;
        XPoint *points;
        int i;

        tracePtr = Blt_Chain_GetValue(link);
        if (tracePtr->numFillPts == 0) {
            continue;
        }
        
        points = Blt_AssertMalloc(sizeof(XPoint) * tracePtr->numFillPts);
        for (i = 0; i < tracePtr->numFillPts; i++) {
            points[i].x = tracePtr->fillPts[i].x;
            points[i].y = tracePtr->fillPts[i].y;
        }
        PaintPolygon(graphPtr, drawable, elemPtr, tracePtr->numFillPts, points);
        Blt_Free(points);
    }
}


#ifdef WIN32

/* 
 * DrawPolyline --
 *
 *      Draws the connected line segments representing the trace.
 *
 *      This MSWindows version arbitrarily breaks traces greater than one
 *      hundred points that are wide lines, into smaller pieces.
 */
static void
DrawPolyline(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
             LinePen *penPtr)
{
    HBRUSH brush, oldBrush;
    HDC dc;
    HPEN pen, oldPen;
    POINT *points;
    TkWinDCState state;
    TracePoint *p;
    size_t numMax, numReq, count;

    /*  
     * If the line is wide (> 1 pixel), arbitrarily break the line in sections
     * of 100 points.  This bit of weirdness has to do with wide geometric
     * pens.  The longer the polyline, the slower it draws.  The trade off is
     * that we lose dash and cap uniformity for unbearably slow polyline
     * draws.
     */
    numReq = tracePtr->numPoints;
    numMax = 100;                       /* Default small size for polyline. */
    if (penPtr->traceGC->line_width < 2) {
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
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        points[count].x = ROUND(p->x);
        points[count].y = ROUND(p->y);
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
}

#else

/* 
 * DrawPolyline --
 *
 *      Draws the connected line segments representing the trace.
 *
 *      This X11 version arbitrarily breaks traces greater than the server
 *      request size, into smaller pieces.
 */
static void
DrawPolyline(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
             LinePen *penPtr)
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
    count = 0;                          /* Counter for points */
    for (p = tracePtr->head; p != NULL; p = p->next) {
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        points[count].x = ROUND(p->x);
        points[count].y = ROUND(p->y);
        count++;
        if (count >= numMax) {
            XDrawLines(graphPtr->display, drawable, penPtr->traceGC, points,
                       count, CoordModeOrigin);
            points[0] = points[count - 1];
            count = 1;
        }
    }
    if (count > 1) {
        XDrawLines(graphPtr->display, drawable, penPtr->traceGC, points,
                   count, CoordModeOrigin);
    }
    Blt_Free(points);
}
#endif /* WIN32 */

/* 
 * DrawErrorBars --
 *
 *      Draws the segments representing the parts of the the error bars.  As
 *      many segments are draw as once as can fit into an X server request.
 *
 *      Errorbars are only drawn at the knots of the trace (i.e. original
 *      points, not generated).  The "play" function can limit what bars are
 *      drawn.
 */
static void 
DrawErrorBars(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
              LinePen *penPtr)
{
    XSegment *segments;
    TraceSegment *s;
    size_t numMax, numReq, count;

    numReq = tracePtr->numSegments;
    numMax = MAX_DRAWSEGMENTS(graphPtr->display);
    if ((numMax == 0) || (numMax > numReq)) {
        numMax = numReq;
    } 
    segments = Blt_Malloc(numMax * sizeof(XSegment));
    if (segments == NULL) {
        return;
    }
    count = 0;                          /* Counter for segments */
    tracePtr->flags |= KNOT;
    for (s = tracePtr->segments; s != NULL; s = s->next) {
        if ((s->flags & penPtr->errorFlags) == 0) {
            continue;
        }
        if ((!PLAYING(tracePtr, s->index)) ||
            (!DRAWN(tracePtr, s->flags))) {
            continue;
        }
        segments[count].x1 = (short int)ROUND(s->x1);
        segments[count].y1 = (short int)ROUND(s->y1);
        segments[count].x2 = (short int)ROUND(s->x2);
        segments[count].y2 = (short int)ROUND(s->y2);
        count++;
        if (count >= numMax) {
            XDrawSegments(graphPtr->display, drawable, penPtr->errorGC, 
                          segments, count);
            count = 0;
        }
    }
    if (count > 0) {
        XDrawSegments(graphPtr->display, drawable, penPtr->errorGC, 
                      segments, count);
    }
    tracePtr->drawFlags &= ~(YERROR | XERROR);
    Blt_Free(segments);
}

/* 
 * DrawValues --
 *
 *      Draws text of the numeric values of the point.
 *
 *      Values are only drawn at the knots of the trace (i.e. original points,
 *      not generated).  The "play" function can limit what value are drawn.
 */
static void
DrawValues(Graph *graphPtr, Drawable drawable, Trace *tracePtr, LinePen *penPtr)
{
    TracePoint *p;
    const char *fmt;
    
    fmt = penPtr->valueFormat;
    if (fmt == NULL) {
        fmt = "%g";
    }
    for (p = tracePtr->head; p != NULL; p = p->next) {
        double x, y;
        char string[200];

        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        if (!DRAWN(tracePtr, p->flags)) {
            continue;
        }
        x = tracePtr->elemPtr->x.values[p->index];
        y = tracePtr->elemPtr->y.values[p->index];
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
             &penPtr->valueStyle, ROUND(p->x), ROUND(p->y));
    }
}

/* 
 * DrawPointSymbols --
 *
 *      Draws the symbols of the trace as points.
 *
 *      Symbols are only drawn at the knots of the trace (i.e. original points,
 *      not generated).  The "play" function can limit what points are drawn.
 */
static void
DrawPointSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
                 LinePen *penPtr)
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
    count = 0;                          /* Counter for points. */
    for (p = tracePtr->head; p != NULL; p = p->next) {
        if (!DRAWN(tracePtr, p->flags)) {
            continue;
        }
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        points[count].x = ROUND(p->x);
        points[count].y = ROUND(p->y);
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
 *      Draws the symbols of the trace as circles.  The outlines of circles
 *      are drawn after circles are filled.  This is speed tradeoff: drawn
 *      many circles at once, or drawn one symbol at a time.
 *
 *      Symbols are only drawn at the knots of the trace (i.e. original points,
 *      not generated).  The "play" function can limit what circles are drawn.
 *
 */
static void
DrawCircleSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
                  LinePen *penPtr, int size)
{
    HBRUSH brush, oldBrush;
    HPEN pen, oldPen;
    HDC dc;
    TkWinDCState state;
    int r;
    TracePoint *p;

    r = (int)ceil(size * 0.5);
    if (drawable == None) {
        return;                         /* Huh? */
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
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        rx = ROUND(p->x);
        ry = ROUND(p->y);
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
 *      Draws the symbols of the trace as circles.  The outlines of circles
 *      are drawn after circles are filled.  This is speed tradeoff: draw
 *      many circles at once, or drawn one symbol at a time.
 *
 *      Symbols are only drawn at the knots of the trace (i.e. original points,
 *      not generated).  The "play" function can limit what circles are drawn.
 *
 */
static void
DrawCircleSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
                  LinePen *penPtr, int size)
{
    XArc *arcs;
    TracePoint *p;
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
    count = 0;                          /* Counter for arcs. */
    for (p = tracePtr->head; p != NULL; p = p->next) {
        if (!DRAWN(tracePtr, p->flags)) {
            continue;
        }
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        arcs[count].x = ROUND(p->x - r);
        arcs[count].y = ROUND(p->y - r);
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
 *      Draws the symbols of the trace as squares.  The outlines of squares
 *      are drawn after squares are filled.  This is speed tradeoff: draw
 *      many squares at once, or drawn one symbol at a time.
 *
 *      Symbols are only drawn at the knots of the trace (i.e. original points,
 *      not generated).  The "play" function can limit what squares are drawn.
 *
 */
static void
DrawSquareSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr, 
                  LinePen *penPtr, int size)
{
    XRectangle *rectangles;
    TracePoint *p;
    size_t numMax, numReq, count;
    int r, s;

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

    count = 0;                          /* Counter for rectangles. */
    for (p = tracePtr->head; p != NULL; p = p->next) {
        if (!DRAWN(tracePtr, p->flags)) {
            continue;
        }
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        rectangles[count].x = ROUND(p->x - r);
        rectangles[count].y = ROUND(p->y - r);
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
 *      Draws the symbols of the trace as single line crosses or pluses.  
 *
 *      Symbols are only drawn at the knots of the trace (i.e. original points,
 *      not generated).  The "play" function can limit what symbols are drawn.
 */
static void
DrawSkinnyCrossPlusSymbols(Graph *graphPtr, Drawable drawable, Trace *tracePtr,
                           LinePen *penPtr, int size)
{
    TracePoint *p;
    XPoint pattern[4];                  /* Template for polygon symbols */
    XSegment *segments;
    size_t numMax, numReq, count;
    int r;

    /* Two line segments for each point in the trace. */
    numReq = tracePtr->numPoints * 2;
    /* Limit the size of the segment array to the maximum request size of the
     * X11 server. */
    numMax = MAX_DRAWSEGMENTS(graphPtr->display);
    numMax &= ~0x1;             /* Max # segments must be even. */
    if ((numMax == 0) || (numMax > numReq)) {
        numMax = numReq;
    }
    segments = Blt_Malloc(numMax * sizeof(XSegment));
    if (segments == NULL) {
        return;
    }
    r = (int)ceil(size * 0.5);
    if (penPtr->symbol.type == SYMBOL_SCROSS) {
        r = ROUND((double)r * M_SQRT1_2);
        pattern[3].y = pattern[2].x = pattern[0].x = pattern[0].y = -r;
        pattern[3].x = pattern[2].y = pattern[1].y = pattern[1].x = r;
    } else {
        pattern[0].y = pattern[1].y = pattern[2].x = pattern[3].x = 0;
        pattern[0].x = pattern[2].y = -r;
        pattern[1].x = pattern[3].y = r;
    }
    count = 0;                          /* Counter for segments. */
    for (p = tracePtr->head; p != NULL; p = p->next) {
        int rx, ry;

        if (!DRAWN(tracePtr, p->flags)) {
            continue;
        }
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        rx = ROUND(p->x);
        ry = ROUND(p->y);
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
                     LinePen *penPtr, int size)
{
    TracePoint *p;
    XPoint polygon[13];
    XPoint pattern[13];
    int r;
    int d;                      /* Small delta for cross/plus
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
            pattern[i].x = ROUND(dx - dy);
            pattern[i].y = ROUND(dx + dy);
        }
        pattern[12] = pattern[0];
    }
    for (p = tracePtr->head; p != NULL; p = p->next) {
        int rx, ry;
        int i;

        if (!DRAWN(tracePtr, p->flags)) {
            continue;
        }
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        rx = ROUND(p->x);
        ry = ROUND(p->y);
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
                         LinePen *penPtr, int size)
{
    XPoint pattern[4];
    double b;
    int b2, h1, h2;
    TracePoint *p;

#define H_RATIO         1.1663402261671607
#define B_RATIO         1.3467736870885982
#define TAN30           0.57735026918962573
#define COS30           0.86602540378443871
    b  = ROUND(size * B_RATIO * 0.7);
    b2 = ROUND(b * 0.5);
    h2 = ROUND(TAN30 * b2);
    h1 = ROUND(b2 / COS30);
    /*
     *
     *                      The triangle symbol is a closed polygon
     *           0,3        of 3 points. The diagram to the left
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
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        rx = ROUND(p->x);
        ry = ROUND(p->y);
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
                   LinePen *penPtr, int size)
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
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        rx = ROUND(p->x);
        ry = ROUND(p->y);
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
                 LinePen *penPtr, int size)
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
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        x = ROUND(p->x) - dx;
        y = ROUND(p->y) - dy;
        Tk_RedrawImage(penPtr->symbol.image, 0, 0, w, h, drawable, x, y);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawSymbols --
 *
 *      Draw the symbols centered at the each given x,y coordinate in the array
 *      of points.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Draws a symbol at each coordinate given.  If active, only those
 *      coordinates which are currently active are drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawSymbols(
    Graph *graphPtr,                    /* Graph widget record */
    Drawable drawable,                  /* Pixmap or window to draw into */
    Trace *tracePtr,
    LinePen *penPtr)
{
    int size;

    if (tracePtr->elemPtr->reqMaxSymbols > 0) {
        TracePoint *p;
        int count;

        /* Mark the symbols that should be displayed. */
        count = 0;
        for (p = tracePtr->head; p != NULL; p = p->next) {
            if (p->flags & KNOT) {
                if ((count % tracePtr->elemPtr->reqMaxSymbols) == 0) {
                    p->flags |= SYMBOL;
                }
            }
            count++;
        }
    }
    tracePtr->drawFlags |= KNOT | VISIBLE;
    if (tracePtr->elemPtr->reqMaxSymbols > 0) {
        tracePtr->drawFlags |= SYMBOL;
    }
    if (tracePtr->elemPtr->scaleSymbols) {
        size =  ScaleSymbol(tracePtr->elemPtr, penPtr->symbol.size);
    } else {
        size = penPtr->symbol.size;
    }
    if (size < 3) {
        if (penPtr->symbol.fillGC != NULL) {
            DrawPointSymbols(graphPtr, drawable, tracePtr, penPtr);
        }
        return;
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
        
    }
    tracePtr->drawFlags &= ~(KNOT | VISIBLE | SYMBOL);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawTrace --
 *
 *      Draws everything associated with the element's trace. This includes
 *      the polyline, line symbols, errorbars, polygon representing the
 *      area under the curve, and values.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      X drawing commands are output.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawTrace(Graph *graphPtr, Drawable drawable, Trace *tracePtr)
{
    LinePen *penPtr;

    /* Set the pen to use when drawing the trace.  */
    penPtr = tracePtr->penPtr;
    tracePtr->drawFlags = 0;

    /* Draw error bars at knots (original points). */
    if (tracePtr->numSegments > 0) {
        DrawErrorBars(graphPtr, drawable, tracePtr, penPtr);
    }
    /* Draw values at knots (original points). */
    if (penPtr->valueFlags != SHOW_NONE) {
        DrawValues(graphPtr, drawable, tracePtr, penPtr);
    }   
    if (penPtr->traceWidth > 0) {
        DrawPolyline(graphPtr, drawable, tracePtr, penPtr);
    }
    /* Draw symbols at knots (original points). */
    if (penPtr->symbol.type != SYMBOL_NONE) {
        DrawSymbols(graphPtr, drawable, tracePtr, penPtr);
    }
}


static void
DrawActiveTrace(Graph *graphPtr, Drawable drawable, Trace *tracePtr)
{
    LinePen *penPtr;

    /* Set the pen to use when drawing the trace.  */
    penPtr = tracePtr->elemPtr->activePenPtr;
    tracePtr->drawFlags = 0;

    /* Draw error bars at original points. */
    if (tracePtr->numSegments > 0) {
        DrawErrorBars(graphPtr, drawable, tracePtr, penPtr);
    }
    /* Draw values at original points. */
    if (penPtr->valueFlags != SHOW_NONE) {
        DrawValues(graphPtr, drawable, tracePtr, penPtr);
    }   
    if ((tracePtr->elemPtr->numActiveIndices < 0) && 
        (penPtr->traceWidth > 0)) {
        DrawPolyline(graphPtr, drawable, tracePtr, penPtr);
    }
    /* Draw symbols at original points. */
    if (penPtr->symbol.type != SYMBOL_NONE) {
        if (tracePtr->elemPtr->numActiveIndices >= 0) {
            /* Indicate that we only want to draw active symbols. */
            tracePtr->drawFlags |= ACTIVE_POINT;
        }
        DrawSymbols(graphPtr, drawable, tracePtr, penPtr);
        tracePtr->drawFlags &= ~ACTIVE_POINT;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawActiveProc --
 *
 *      Draws the connected line(s) representing the element. If the line is
 *      made up of non-line symbols and the line width parameter has been set
 *      (linewidth > 0), the element will also be drawn as a line (with the
 *      linewidth requested).  The line may consist of separate line segments.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      X drawing commands are output.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawActiveProc(Graph *graphPtr, Drawable drawable, Element *basePtr)
{
    LineElement *elemPtr = (LineElement *)basePtr;
    Blt_ChainLink link;

    if ((elemPtr->flags & ACTIVE_PENDING) && (elemPtr->numActiveIndices >= 0)) {
        MapActiveSymbols(elemPtr);
    }
    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;

        tracePtr = Blt_Chain_GetValue(link);
        DrawActiveTrace(graphPtr, drawable, tracePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawNormalProc --
 *
 *      Draws the connected line(s) representing the element. If the line is
 *      made up of non-line symbols and the line width parameter has been set
 *      (linewidth > 0), the element will also be drawn as a line (with the
 *      linewidth requested).  The line may consist of separate line segments.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      X drawing commands are output.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawNormalProc(Graph *graphPtr, Drawable drawable, Element *basePtr)
{
    LineElement *elemPtr = (LineElement *)basePtr;
    Blt_ChainLink link;

    /* Fill area under curve. Only for non-active elements. */
    DrawAreaUnderCurve(graphPtr, drawable, elemPtr);
    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;

        tracePtr = Blt_Chain_GetValue(link);
        tracePtr->drawFlags = 0;
        DrawTrace(graphPtr, drawable, tracePtr);
    }
}


static void
SetLineAttributes(Blt_Ps ps, LinePen *penPtr)
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
ErrorBarsToPostScript(Blt_Ps ps, Trace *tracePtr, LinePen *penPtr)
{
    TraceSegment *s;

    SetLineAttributes(ps, penPtr);
    Blt_Ps_Append(ps, "% start segments\n");
    Blt_Ps_Append(ps, "newpath\n");
    for (s = tracePtr->segments; s != NULL; s = s->next) {
        if (!DRAWN(tracePtr, s->flags)) {
            continue;
        }
        if (!PLAYING(tracePtr, s->index)) {
            continue;
        }
        Blt_Ps_Format(ps, "  %g %g moveto %g %g lineto\n", 
                s->x1, s->y1, s->x2, s->y2);
        Blt_Ps_Append(ps, "DashesProc stroke\n");
    }
    Blt_Ps_Append(ps, "% end segments\n");
}

static void
ValuesToPostScript(Blt_Ps ps, Trace *tracePtr, LinePen *penPtr)
{
    TracePoint *p;
    const char *fmt;
    
    fmt = penPtr->valueFormat;
    if (fmt == NULL) {
        fmt = "%g";
    }
    for (p = tracePtr->head; p != NULL; p = p->next) {
        double x, y;
        char string[TCL_DOUBLE_SPACE * 2 + 2];

        if (!DRAWN(tracePtr, p->flags)) {
            continue;
        }
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
        x = tracePtr->elemPtr->x.values[p->index];
        y = tracePtr->elemPtr->y.values[p->index];
        if (penPtr->valueFlags == SHOW_X) {
            Blt_FormatString(string, TCL_DOUBLE_SPACE, fmt, x); 
        } else if (penPtr->valueFlags == SHOW_Y) {
            Blt_FormatString(string, TCL_DOUBLE_SPACE, fmt, y); 
        } else if (penPtr->valueFlags == SHOW_BOTH) {
            Blt_FormatString(string, TCL_DOUBLE_SPACE, fmt, x);
            strcat(string, ",");
            Blt_FormatString(string + strlen(string), TCL_DOUBLE_SPACE, fmt, y);
        }
        Blt_Ps_DrawText(ps, string, &penPtr->valueStyle, x, y);
    }
}

static void
PolylineToPostScript(Blt_Ps ps, Trace *tracePtr, LinePen *penPtr)
{
    Point2d *points;
    TracePoint *p;
    int count;

    SetLineAttributes(ps, penPtr);
    points = Blt_AssertMalloc(tracePtr->numPoints * sizeof(Point2d));
    count = 0;
    for (p = tracePtr->head; p != NULL; p = p->next) {
        if (!PLAYING(tracePtr, p->index)) {
            continue;
        }
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
 * AreaUnderCurveToPostScript --
 *
 *      Draws the polygons under the traces.
 */
static void
AreaUnderCurveToPostScript(Blt_Ps ps, LineElement *elemPtr)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL; 
         link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;

        tracePtr = Blt_Chain_GetValue(link);
        if (tracePtr->numFillPts == 0) {
            continue;
        }
        if (elemPtr->areaBg != NULL) {
            /* Create a path to use for both the polygon and its outline. */
            Blt_Ps_Append(ps, "% start fill area\n");
            Blt_Ps_Polyline(ps, tracePtr->numFillPts, tracePtr->fillPts);

            /* If the background fill color was specified, draw the polygon in a
             * solid fashion with that color.  */
            Blt_Ps_XSetBackground(ps, Blt_Bg_BorderColor(elemPtr->areaBg));
            Blt_Ps_Append(ps, "gsave fill grestore\n");

            Blt_Ps_XSetForeground(ps, elemPtr->fillFgColor);
            if (elemPtr->areaBg != NULL) {
                Blt_Ps_Append(ps, "gsave fill grestore\n");
                /* TBA: Transparent tiling is the hard part. */
            } else {
                Blt_Ps_Append(ps, "gsave fill grestore\n");
            }
            Blt_Ps_Append(ps, "% end fill area\n");
        }
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * GetSymbolPostScriptInfo --
 *
 *      Set up the PostScript environment with the macros and attributes needed
 *      to draw the symbols of the element.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
GetSymbolPostScriptInfo(Blt_Ps ps, LineElement *elemPtr, LinePen *penPtr, 
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
     * Build a PostScript procedure to draw the symbols. Otherwise fill and
     * stroke the path formed already.
     */
    Blt_Ps_Append(ps, "\n/DrawSymbolProc {\n");
    switch (penPtr->symbol.type) {
    case SYMBOL_NONE:
        break;                          /* Do nothing */
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

/*
 *---------------------------------------------------------------------------
 *
 * SymbolsToPostScript --
 *
 *      Draw a symbol centered at the given x,y window coordinate based upon
 *      the element symbol type and size.
 *
 * Results:
 *      None.
 *
 * Problems:
 *      Most notable is the round-off errors generated when calculating the
 *      centered position of the symbol.
 *
 *---------------------------------------------------------------------------
 */
static void
SymbolsToPostScript(Blt_Ps ps, Trace *tracePtr, LinePen *penPtr)
{
    TracePoint *p;
    double size;
    int symbolSize;
    static const char *symbolMacros[] = {
        "Li", "Sq", "Ci", "Di", "Pl", "Cr", "Sp", "Sc", "Tr", "Ar", "Bm", 
        (char *)NULL,
    };

    if (tracePtr->elemPtr->scaleSymbols) {
        symbolSize = ScaleSymbol(tracePtr->elemPtr, penPtr->symbol.size);
    } else {
        symbolSize = penPtr->symbol.size;
    }
    GetSymbolPostScriptInfo(ps, tracePtr->elemPtr, penPtr, symbolSize);
    size = (double)symbolSize;
    switch (penPtr->symbol.type) {
    case SYMBOL_SQUARE:
    case SYMBOL_CROSS:
    case SYMBOL_PLUS:
    case SYMBOL_SCROSS:
    case SYMBOL_SPLUS:
        size = (double)ROUND(size * S_RATIO);
        break;
    case SYMBOL_TRIANGLE:
    case SYMBOL_ARROW:
        size = (double)ROUND(size * 0.7);
        break;
    case SYMBOL_DIAMOND:
        size = (double)ROUND(size * M_SQRT1_2);
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

/*
 *---------------------------------------------------------------------------
 *
 * SymbolToPostScriptProc --
 *
 *      Draw a symbol centered at the given x,y window coordinate based upon
 *      the element symbol type and size.
 *
 * Results:
 *      None.
 *
 * Problems:
 *      Most notable is the round-off errors generated when calculating the
 *      centered position of the symbol.
 *
 *---------------------------------------------------------------------------
 */
static void
SymbolToPostScriptProc(
    Graph *graphPtr,                    /* Graph widget record */
    Blt_Ps ps,
    Element *basePtr,                   /* Line element information */
    double x, double y,                 /* Center position of symbol */
    int size)                           /* Size of symbol.  May override the
                                         * size configured in the element. */
{
    LineElement *elemPtr = (LineElement *)basePtr;
    LinePen *penPtr;
    double symbolSize;
    static const char *symbolMacros[] =
    {
        "Li", "Sq", "Ci", "Di", "Pl", "Cr", "Sp", "Sc", "Tr", "Ar", "Bm", 
        (char *)NULL,
    };

    penPtr = NORMALPEN(elemPtr);
    GetSymbolPostScriptInfo(ps, elemPtr, penPtr, size);

    symbolSize = (double)size;
    switch (penPtr->symbol.type) {
    case SYMBOL_SQUARE:
    case SYMBOL_CROSS:
    case SYMBOL_PLUS:
    case SYMBOL_SCROSS:
    case SYMBOL_SPLUS:
        symbolSize = (double)ROUND(size * S_RATIO);
        break;
    case SYMBOL_TRIANGLE:
    case SYMBOL_ARROW:
        symbolSize = (double)ROUND(size * 0.7);
        break;
    case SYMBOL_DIAMOND:
        symbolSize = (double)ROUND(size * M_SQRT1_2);
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
 * TraceToPostScript --
 *
 *      Draws everything associated with the element's trace. This includes
 *      the polyline, line symbols, errorbars, polygon representing the
 *      area under the curve, and values.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      X drawing commands are output.
 *
 *---------------------------------------------------------------------------
 */
static void
TraceToPostScript(Blt_Ps ps, Trace *tracePtr)
{
    LinePen *penPtr;

    /* Set the pen to use when drawing the trace.  */
    penPtr = tracePtr->penPtr;

    /* Draw error bars at original points. */
    if (tracePtr->numSegments > 0) {
        ErrorBarsToPostScript(ps, tracePtr, penPtr);
    }
    /* Draw values at original points. */
    if (penPtr->valueFlags != SHOW_NONE) {
        ValuesToPostScript(ps, tracePtr, penPtr);
    }   
    /* Polyline for the trace. */
    if (penPtr->traceWidth > 0) {
        PolylineToPostScript(ps, tracePtr, penPtr);
    }
    /* Draw symbols at original points. */
    if (penPtr->symbol.type != SYMBOL_NONE) {
        SymbolsToPostScript(ps, tracePtr, penPtr);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * NormalToPostScriptProc --
 *
 *      Similar to the DrawLine procedure, prints PostScript related commands to
 *      form the connected line(s) representing the element.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      PostScript pen width, dashes, and color settings are changed.
 *
 *---------------------------------------------------------------------------
 */
static void
NormalToPostScriptProc(Graph *graphPtr, Blt_Ps ps, Element *basePtr)
{
    LineElement *elemPtr = (LineElement *)basePtr;
    Blt_ChainLink link;

    AreaUnderCurveToPostScript(ps, elemPtr);
    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;

        tracePtr = Blt_Chain_GetValue(link);
        tracePtr->drawFlags = 0;
        TraceToPostScript(ps, tracePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ActiveTraceToPostScript --
 *
 *      Draws everything associated with the element's trace. This includes
 *      the polyline, line symbols, errorbars, polygon representing the
 *      area under the curve, and values.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      X drawing commands are output.
 *
 *---------------------------------------------------------------------------
 */
static void
ActiveTraceToPostScript(Blt_Ps ps, Trace *tracePtr)
{
    LinePen *penPtr;

    /* Set the pen to use when drawing the trace.  */
    penPtr = tracePtr->elemPtr->activePenPtr;

    /* Draw error bars at original points. */
    if (tracePtr->numSegments > 0) {
        ErrorBarsToPostScript(ps, tracePtr, penPtr);
    }
    /* Draw values at original points. */
    if (penPtr->valueFlags != SHOW_NONE) {
        ValuesToPostScript(ps, tracePtr, penPtr);
    }   
    if ((tracePtr->elemPtr->numActiveIndices < 0) && (penPtr->traceWidth > 0)) {
        PolylineToPostScript(ps, tracePtr, penPtr);
    }
    /* Draw symbols at original points. */
    if (penPtr->symbol.type != SYMBOL_NONE) {
        if (tracePtr->elemPtr->numActiveIndices >= 0) {
            /* Indicate that we only want to draw active symbols. */
            tracePtr->drawFlags |= ACTIVE_POINT;
        }
        SymbolsToPostScript(ps, tracePtr, penPtr);
        tracePtr->drawFlags &= ~ACTIVE_POINT;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ActiveToPostScriptProc --
 *
 *      Similar to the DrawLine procedure, prints PostScript related commands to
 *      form the connected line(s) representing the element.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      PostScript pen width, dashes, and color settings are changed.
 *
 *---------------------------------------------------------------------------
 */
static void
ActiveToPostScriptProc(Graph *graphPtr, Blt_Ps ps, Element *basePtr)
{
    LineElement *elemPtr = (LineElement *)basePtr;
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        Trace *tracePtr;

        tracePtr = Blt_Chain_GetValue(link);
        tracePtr->drawFlags = 0;
        ActiveTraceToPostScript(ps, tracePtr);
    }
}


#ifdef WIN32
/* 
 *---------------------------------------------------------------------------
 *
 * DrawCircleSymbol --
 *
 *      Draws a circle symbol centered at x, y.  The outlines of circles
 *      are drawn after circles are filled.  This is speed tradeoff: draw
 *      many circles at once, or drawn one symbol at a time.
 *
 *      Symbols are only drawn at the knots of the trace (i.e. original
 *      points, not generated).  The "play" function can limit what circles
 *      are drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawCircleSymbol(Graph *graphPtr, Drawable drawable, LinePen *penPtr, 
                 int x, int y, int size)
{
    HBRUSH brush, oldBrush;
    HPEN pen, oldPen;
    HDC dc;
    TkWinDCState state;
    int r;

    r = (int)ceil(size * 0.5);
    if (drawable == None) {
        return;                         /* Huh? */
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
 *---------------------------------------------------------------------------
 *
 * DrawCircleSymbol --
 *
 *      Draws a circle symbol centered at x, y.  The outlines of circles
 *      are drawn after circles are filled.  This is speed tradeoff: draw
 *      many circles at once, or drawn one symbol at a time.
 *
 *      Symbols are only drawn at the knots of the trace (i.e. original
 *      points, not generated).  The "play" function can limit what circles
 *      are drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawCircleSymbol(Graph *graphPtr, Drawable drawable, LinePen *penPtr, 
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

#endif  /* WIN32 */

/* 
 *---------------------------------------------------------------------------
 *
 * DrawSquareSymbol --
 *
 *      Draws a square plus symbol centered at x, y.  The outlines of
 *      squares are drawn after squares are filled.  This is speed
 *      tradeoff: draw many squares at once, or drawn one symbol at a time.
 *
 *      Symbols are only drawn at the knots of the trace (i.e. original
 *      points, not generated).  The "play" function can limit what squares
 *      are drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawSquareSymbol(Graph *graphPtr, Drawable drawable, LinePen *penPtr, 
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
 *---------------------------------------------------------------------------
 *
 * DrawSkinnyCrossPlusSymbol --
 *
 *      Draws a single line cross or plus symbol centered at x, y. 
 *
 *      Symbols are only drawn at the knots of the trace (i.e. original
 *      points, not generated).  The "play" function can limit what symbols
 *      are drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawSkinnyCrossPlusSymbol(Graph *graphPtr, Drawable drawable, LinePen *penPtr, 
                          int x, int y, int size)
{
    XPoint pattern[13];                 /* Template for polygon symbols */
    XSegment segments[2];
    int r;

    r = (int)ceil(size * 0.5);
    if (penPtr->symbol.type == SYMBOL_SCROSS) {
        r = ROUND((double)r * M_SQRT1_2);
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

/* 
 *---------------------------------------------------------------------------
 *
 * DrawCrossPlusSymbol --
 *
 *      Draws a cross or plus symbol centered at x, y. 
 *
 *---------------------------------------------------------------------------
 */
static void
DrawCrossPlusSymbol(Graph *graphPtr, Drawable drawable, LinePen *penPtr, 
                    int x, int y, int size)
{
    XPoint polygon[13];
    int r;
    int d;                      /* Small delta for cross/plus
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
            polygon[i].x = ROUND(dx - dy);
            polygon[i].y = ROUND(dx + dy);
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

/* 
 *---------------------------------------------------------------------------
 *
 * DrawTriangleArrowSymbol --
 *
 *      Draws the a triangle or arrow symbol centered at x, y.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawTriangleArrowSymbol(Graph *graphPtr, Drawable drawable, LinePen *penPtr, 
                        int x, int y, int size)
{
    XPoint polygon[4];
    double b;
    int b2, h1, h2;
    int i;

#define H_RATIO         1.1663402261671607
#define B_RATIO         1.3467736870885982
#define TAN30           0.57735026918962573
#define COS30           0.86602540378443871
    b  = ROUND(size * B_RATIO * 0.7);
    b2 = ROUND(b * 0.5);
    h2 = ROUND(TAN30 * b2);
    h1 = ROUND(b2 / COS30);
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

/* 
 *---------------------------------------------------------------------------
 *
 * DrawDiamondSymbol --
 *
 *      Draws a diamond symbol centered at x, y.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawDiamondSymbol(Graph *graphPtr, Drawable drawable, LinePen *penPtr, 
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

/* 
 *---------------------------------------------------------------------------
 *
 * DrawImageSymbol --
 *
 *      Draws the image symbol centered at x, y.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawImageSymbol(Graph *graphPtr, Drawable drawable, LinePen *penPtr, 
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

/*
 *---------------------------------------------------------------------------
 *
 * DrawSymbol --
 *
 *      Draw the symbols centered at the each given x,y coordinate in the
 *      array of points.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Draws a symbol at each coordinate given.  If active, only those
 *      coordinates which are currently active are drawn.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawSymbol(
    Graph *graphPtr,                    /* Graph widget record */
    Drawable drawable,                  /* Pixmap or window to draw into */
    LinePen *penPtr, 
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
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawSymbolProc --
 *
 *      Draw the symbol centered at the each given x,y coordinate.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Draws a symbol at the coordinate given.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawSymbolProc(
    Graph *graphPtr,                    /* Graph widget record */
    Drawable drawable,                  /* Pixmap or window to draw into */
    Element *basePtr,                   /* Line element information */
    int x, int y,                       /* Center position of symbol */
    int size)                           /* Size of symbol. */
{
    LineElement *elemPtr = (LineElement *)basePtr;
    LinePen *penPtr;

    penPtr = NORMALPEN(elemPtr);
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

/*
 *---------------------------------------------------------------------------
 *
 * DestroyProc --
 *
 *      Release memory and resources allocated for the line element.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Everything associated with the line element is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyProc(Graph *graphPtr, Element *basePtr)
{
    LineElement *elemPtr = (LineElement *)basePtr;
    Blt_ChainLink link, next;

    DestroyPenProc(graphPtr, (Pen *)&elemPtr->builtinPen);
    if (elemPtr->activePenPtr != NULL) {
        Blt_FreePen((Pen *)elemPtr->activePenPtr);
    }
    if (elemPtr->styles != NULL) {
        Blt_FreeStyles(elemPtr->styles);
        Blt_Chain_Destroy(elemPtr->styles);
    }
    if (elemPtr->pointPool != NULL) {
        Blt_Pool_Destroy(elemPtr->pointPool);
    }
    if (elemPtr->segmentPool != NULL) {
        Blt_Pool_Destroy(elemPtr->segmentPool);
    }
    for (link = Blt_Chain_FirstLink(elemPtr->traces); link != NULL; 
         link = next) {
        Trace *tracePtr;

        next = Blt_Chain_NextLink(link);
        tracePtr = Blt_Chain_GetValue(link);
        FreeTrace(elemPtr->traces, tracePtr);
    }
    if (elemPtr->fillGC != NULL) {
        Tk_FreeGC(graphPtr->display, elemPtr->fillGC);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LineElement --
 *
 *      Allocate memory and initialize methods for the new line element.
 *
 * Results:
 *      The pointer to the newly allocated element structure is returned.
 *
 * Side effects:
 *      Memory is allocated for the line element structure.
 *
 *---------------------------------------------------------------------------
 */

static ElementProcs lineProcs = {
    NearestProc,                        /* Finds the closest element/data
                                         * point */
    ConfigureProc,                      /* Configures the element. */
    DestroyProc,                        /* Destroys the element. */
    DrawActiveProc,                     /* Draws active element */
    DrawNormalProc,                     /* Draws normal element */
    DrawSymbolProc,                     /* Draws the element symbol. */
    ExtentsProc,                        /* Find the extents of the element's
                                         * data. */
    FindProc,                           /* Find the points within the search
                                         * radius. */
    ActiveToPostScriptProc,             /* Prints active element. */
    NormalToPostScriptProc,             /* Prints normal element. */
    SymbolToPostScriptProc,             /* Prints the line's symbol. */
    MapProc                             /* Compute element's screen
                                         * coordinates. */
};

Element *
Blt_LineElement2(Graph *graphPtr, ClassId id, Blt_HashEntry *hPtr)
{
    LineElement *elemPtr;

    elemPtr = Blt_AssertCalloc(1, sizeof(LineElement));
    elemPtr->procsPtr = &lineProcs;
    elemPtr->configSpecs = (id == CID_ELEM_LINE) ? lineSpecs : stripSpecs;
    elemPtr->obj.name = Blt_GetHashKey(&graphPtr->elements.nameTable, hPtr);
    Blt_GraphSetObjectClass(&elemPtr->obj, id);
    elemPtr->flags = SCALE_SYMBOL;
    elemPtr->obj.graphPtr = graphPtr;
    /* By default an element's name and label are the same. */
    elemPtr->label = Blt_AssertStrdup(elemPtr->obj.name);
    elemPtr->legendRelief = TK_RELIEF_FLAT;
    elemPtr->penDir = PEN_BOTH_DIRECTIONS;
    elemPtr->styles = Blt_Chain_Create();
    elemPtr->reqSmooth = SMOOTH_NONE;
    elemPtr->builtinPenPtr = &elemPtr->builtinPen;
    InitPen(elemPtr->builtinPenPtr);
    elemPtr->builtinPenPtr->graphPtr = graphPtr;
    elemPtr->builtinPenPtr->classId = id;
    bltLineStylesOption.clientData = (ClientData)sizeof(LineStyle);
    elemPtr->hashPtr = hPtr;
    Blt_SetHashValue(hPtr, elemPtr);
    return (Element *)elemPtr;
}

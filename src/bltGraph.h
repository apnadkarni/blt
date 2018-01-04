/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGraph.h --
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

#ifndef _BLT_GRAPH_H
#define _BLT_GRAPH_H

#define LineWidth(w)    (((w) > 1) ? (w) : 0)

typedef struct _Element Element;
typedef struct _Legend Legend;
typedef struct _Axis Axis;
typedef struct _Graph Graph;
typedef struct _Isoline Isoline;

typedef enum {
    CID_NONE,                           /* 0 */
    CID_AXIS_X,                         /* 1 */
    CID_AXIS_Y,                         /* 2 */
    CID_AXIS_Z,                         /* 3 */
    CID_LEGEND,                         /* 4 */
    CID_ELEM_BAR,                       /* 5 */
    CID_ELEM_CONTOUR,                   /* 6 */
    CID_ELEM_LINE,                      /* 7 */
    CID_ELEM_STRIP,                     /* 8 */
    CID_MARKER_BITMAP,                  /* 9 */
    CID_MARKER_IMAGE,                   /* 10 */
    CID_MARKER_LINE,                    /* 11 */
    CID_MARKER_POLYGON,                 /* 12 */
    CID_MARKER_RECTANGLE,               /* 13 */
    CID_MARKER_TEXT,                    /* 14 */
    CID_MARKER_WINDOW,                  /* 15 */
    CID_LEGEND_ENTRY,                   /* 16 */
    CID_ISOLINE,                        /* 17 */
} ClassId;

/* Generic fields common to all graph objects. */
typedef struct {
    ClassId classId;                    /* Class type of object. */
    const char *name;                   /* Identifier to refer the object. */
    const char *className;              /* Class name of object. */

    Graph *graphPtr;                    /* Graph containing of the object. */
    int deleted;                        /* If non-zero, object has been
                                         * deleted. */
} GraphObj;

#define MARKER_UNDER    1               /* Draw markers designated to lie
                                         * underneath elements, grids,
                                         * legend, etc. */
#define MARKER_ABOVE    0               /* Draw markers designated to rest
                                         * above elements, grids, legend,
                                         * etc. */
#define PADX            2               /* Padding between labels/titles */
#define PADY            2               /* Padding between labels */

#define MINIMUM_MARGIN  20              /* Minimum margin size */


#define BOUND(x, lo, hi)         \
        (((x) > (hi)) ? (hi) : ((x) < (lo)) ? (lo) : (x))

/*
 *      Graph component structure definitions
 */
#define PointInGraph(g,x,y) \
        (((x) <= (g)->x2) && ((x) >= (g)->x1) && \
         ((y) <= (g)->y2) && ((y) >= (g)->y1))

/*
 * Mask values used to selectively enable graph or barchart entries in the
 * various configuration specs.
 */
#define GRAPH                   (BLT_CONFIG_USER_BIT << 1)
#define STRIPCHART              (BLT_CONFIG_USER_BIT << 2)
#define BARCHART                (BLT_CONFIG_USER_BIT << 3)
#define CONTOUR                 (BLT_CONFIG_USER_BIT << 4)
#define LINE_GRAPHS             (GRAPH | STRIPCHART)
#define ALL_GRAPHS              (GRAPH | BARCHART | STRIPCHART | CONTOUR)

#define XAXIS                   (BLT_CONFIG_USER_BIT << 5)
#define YAXIS                   (BLT_CONFIG_USER_BIT << 6)
#define ZAXIS                   (BLT_CONFIG_USER_BIT << 7)
#define ALL_AXES                (XAXIS | YAXIS | ZAXIS)

#define ACTIVE_PEN              (BLT_CONFIG_USER_BIT << 16)
#define NORMAL_PEN              (BLT_CONFIG_USER_BIT << 17)
#define ALL_PENS                (NORMAL_PEN | ACTIVE_PEN)

typedef struct {
    Segment2d *segments;
    int length;
    int *map;
} GraphSegments;

typedef struct {
    Point2d *points;
    int length;
    int *map;
} GraphPoints;

/*
 * Axis2d --
 *
 *      The pair of axes mapping a point onto the graph.
 */
typedef struct {
    Axis *x, *y;
} Axis2d;

/*
 * BarGroup --
 *
 *      Represents a sets of bars with the same abscissa. This structure is
 *      used to track the display of the bars in the group.
 *
 *      Each unique abscissa has at least one group.  Groups can be defined
 *      by the bar element's -group option.  Multiple groups are needed
 *      when you are displaying/comparing similar sets of data (same
 *      abscissas) but belong to a separate group.
 */
typedef struct {
    Axis2d axes;                        /* The axes associated with this
                                         * group. (mapped to the
                                         * x-value) */
    double max;
    double sum;                         /* Sum of the ordinates
                                         * (y-coorinate) of each duplicate
                                         * abscissa. Used to determine
                                         * extents for stacked bars. */
    double lastY;                       /* Y-coordinate position of the
                                         * last bar seen. Used for stacked
                                         * bars. */
    int numMembers;                     /* # of elements that have the same
                                         * abscissa. */
    int count;                          /* Current number of bars seen.
                                         * Used to position of the next bar
                                         * in the group. */
} BarGroup;

/*
 * BarGroupKey --
 *
 *      Key for hash table of set of bars.  The bar set is defined by
 *      coordinates with the same abscissa (x-coordinate).
 *
 */
typedef struct {
    float value;                        /* Duplicated abscissa */
    Axis2d axes;                        /* Axis mapping of element */
} BarGroupKey;

/*
 * BarModes --
 *
 *      Bar elements are displayed according to their x-y coordinates.  If
 *      two bars have the same abscissa (x-coordinate), the bar segments
 *      will be drawn according to one of the following modes:
 */
typedef enum BarModes {
    BARS_INFRONT,                       /* Each successive bar in a group
                                         * is drawn in front of the
                                         * previous. */
    BARS_STACKED,                       /* Each successive bar in a group
                                         * is drawn stacked on top of the
                                         * previous bar. */
    BARS_ALIGNED,                       /* Each successive bar in a group
                                         * is drawn aligned side-by-side to
                                         * the previous from
                                         * right-to-left. */
    BARS_OVERLAP                        /* Like "aligned", each successive
                                         * bar in a group is drawn from
                                         * right-to-left. The bars will
                                         * overlap each other by ~50%. */
} BarMode;

typedef struct _Pen Pen;
typedef struct _Marker Marker;

typedef Pen *(PenCreateProc)(void);
typedef int (PenConfigureProc)(Graph *graphPtr, Pen *penPtr);
typedef void (PenDestroyProc)(Graph *graphPtr, Pen *penPtr);

struct _Pen {
    const char *name;                   /* Pen style identifier.  If NULL
                                         * pen was statically allocated. */
    ClassId classId;                    /* Type of pen. */
    const char *typeId;                 /* String token identifying the
                                         * type of pen. */
    unsigned int flags;                 /* Indicates if the pen element is
                                         * active or normal. */
    int refCount;                       /* Reference count for elements
                                         * using this pen. */
    Blt_HashEntry *hashPtr;

    Blt_ConfigSpec *configSpecs;        /* Configuration specifications */

    PenConfigureProc *configProc;
    PenDestroyProc *destroyProc;
    Graph *graphPtr;                    /* Graph that the pen is associated
                                         * with. */
};

/*
 * Playback --
 *
 *      Contains the line segments positions and graphics context used to
 *      simulate play (by XORing) on the graph.
 *
 */
typedef struct {
    int enabled;
    int from, to;                       /* The two time points defining the
                                         * section of points to be
                                         * displayed.  -1 indicates to use
                                         * the default value.  */
    int t1, t2;                         /* The first and last time points
                                         * in increasing order. */
    Blt_Chain elements;
} Playback;

/*
 *---------------------------------------------------------------------------
 *
 * Crosshairs
 *
 *      Contains the line segments positions and graphics context used to
 *      simulate crosshairs (by XOR-ing) on the graph.
 *
 *---------------------------------------------------------------------------
 */
typedef struct _Crosshairs Crosshairs;

typedef struct {
    const char *name;
    short int width, height;            /* Dimensions of the margin */
    short int axesOffset;               /* The width or height of the margin 
                                         * depending upon the side. */
    short int axesTitleLength;          /* Width of the widest title to be
                                         * shown.  Multiple titles are
                                         * displayed in another
                                         * margin. This is the minimum
                                         * space requirement. */
    short int maxAxisLabelWidth;        /* Maximum width of all axis tick
                                         * labels in this margin. */
    short int maxAxisLabelHeight;       /* Maximum height of all axis tick
                                         * labels in this margin. */
    unsigned int numVisibleAxes;        /* # of axes to be displayed */
    Blt_Chain axes;                     /* List of axes associated with
                                         * this margin */
    int side;                           /* Indicates the side where the
                                         * margin is located: x, x1, y, or
                                         * y2. */
    unsigned short nextStackOffset;     /* For stacked axes, this is the
                                         * offset of next axis (on top of
                                         * the last axis) in the
                                         * margin.  */
    unsigned short nextLayerOffset;     /* This is the offset of the next
                                         * axis outward from the last
                                         * axis. */
} Margin;

#define MARGIN_NONE     -1
#define MARGIN_BOTTOM   0               /* x */
#define MARGIN_LEFT     1               /* y */
#define MARGIN_TOP      2               /* x2 */
#define MARGIN_RIGHT    3               /* y2 */

#define MARGIN_X        0               /* x */
#define MARGIN_Y        1               /* y */
#define MARGIN_X2       2               /* x2 */
#define MARGIN_Y2       3               /* y2 */

/*
 *---------------------------------------------------------------------------
 *
 * Graph --
 *
 *      Top level structure containing everything pertaining to the graph.
 *
 *---------------------------------------------------------------------------
 */
struct _Graph {
    unsigned int flags;                 /* Flags; see below for
                                         * definitions. */
    Tcl_Interp *interp;                 /* Interpreter associated with
                                         * graph */
    Tk_Window tkwin;                    /* Window that embodies the graph.
                                         * NULL means that the window has
                                         * been destroyed but the data
                                         * structures haven't yet been
                                         * cleaned up. */
    Display *display;                   /* Display containing widget; used
                                         * to release resources after tkwin
                                         * has already gone away. */
    Tcl_Command cmdToken;               /* Token for graph's widget
                                         * command. */
    const char *data;                   /* This value isn't used in C code.
                                         * It may be used in TCL bindings
                                         * to associate extra data. */
    Tk_Cursor cursor;
    int inset;                          /* Sum of focus highlight and 3-D
                                         * border.  Indicates how far to
                                         * offset the graph from outside
                                         * edge of the window. */
    int borderWidth;                    /* Width of the exterior border */
    int relief;                         /* Relief of the exterior border. */
    Blt_Bg normalBg;                    /* 3-D border used to delineate the
                                         * plot surface and outer edge of
                                         * window. */
    int highlightWidth;                 /* Width in pixels of highlight to
                                         * draw around widget when it has
                                         * the focus.  <= 0 means don't
                                         * draw a highlight. */
    XColor *highlightBgColor;           /* Color for drawing traversal
                                         * highlight area when highlight is
                                         * off. */
    XColor *highlightColor;             /* Color for drawing traversal
                                         * highlight. */
    const char *title;                  /* Graph title */
    short int titleX, titleY;           /* Position of title on graph. */
    short int titleWidth, titleHeight;  /* Dimensions of title. */
    TextStyle titleTextStyle;           /* Title attributes: font, color,
                                         * etc.*/
    
    const char *takeFocus;              /* Not used in C code, indicates if
                                         * widget should be included in
                                         * focus traversal. */
    Axis *focusPtr;                     /* The axis that currently has
                                         * focus. */
    
    int reqWidth, reqHeight;            /* Requested size of graph
                                         * window */
    int reqPlotWidth, reqPlotHeight;    /* Requested size of plot
                                         * area. Zero means to adjust the
                                         * dimension according to the
                                         * available space left in the
                                         * window. */
    int width, height;                  /* Actual size (in pixels) of graph
                                         * window or PostScript page. */
    Blt_HashTable penTable;             /* Table of pens */
    struct Component {
        Blt_Chain displayList;          /* Determines the order the
                                         * components are drawn:
                                         * last-to-first. */
        Blt_HashTable nameTable;        /* Hash table of ids. */
        Blt_HashTable bindTagTable;     /* Table of bind tags. */
        struct _Blt_Tags tags;          /* Table of tags. */
    } elements, markers, axes, isolines;

    Blt_HashTable dataTables;           /* Hash table of datatable
                                         * clients. */
    ClassId classId;                    /* Default element type */
    Blt_BindTable bindTable;
    int nextMarkerId;                   /* Tracks next marker identifier
                                         * available */
    int nextIsolineId;                  /* Tracks next isoline identifier
                                         * available */
    int axisSpacing;
    Margin margins[4];
    Margin *topPtr, *bottomPtr;
    Margin *leftPtr, *rightPtr;

    Tcl_Obj *leftMarginVarObjPtr;
    Tcl_Obj *rightMarginVarObjPtr;
    Tcl_Obj *topMarginVarObjPtr;
    Tcl_Obj *bottomMarginVarObjPtr;

    int reqLeftMarginSize;
    int reqRightMarginSize;
    int reqTopMarginSize;
    int reqBottomMarginSize;
    PageSetup *pageSetup;               /* Page layout options: see
                                         * bltGrPS.c */
    Legend *legend;                     /* Legend information: see
                                         * bltGrLegd.c */
    Crosshairs *crosshairs;             /* Crosshairs information: see
                                         * bltGrHairs.c */
    int halo;                           /* Maximum distance allowed between
                                         * points when searching for a
                                         * point */
    GC drawGC;                          /* GC for drawing on the
                                         * margins. This includes the axis
                                         * lines */  
    int plotBorderWidth;                /* Width of interior 3-D border. */
    int plotRelief;                     /* 3-d effect: TK_RELIEF_RAISED
                                         * etc. */
    Blt_Bg plotBg;                      /* Color of plotting surface */

    /* If non-zero, force plot to conform to aspect ratio W/H */
    float aspect;
    short int x1, x2, y1, y2;           /* Opposite corners of plot area
                                         * bounding box. x2 and y2 are
                                         * outside of the area. */
    Blt_Pad padX;                       /* Vertical padding for plotarea */
    int vRange, vOffset;                /* Vertical axis range and offset
                                         * from the left side of the graph
                                         * window. Used to transform
                                         * coordinates to vertical axes. */
    Blt_Pad padY;                       /* Horizontal padding for plotarea */
    int hRange, hOffset;                /* Horizontal axis range and offset
                                         * from the top of the graph
                                         * window. Used to transform
                                         * horizontal axes */
    float vScale, hScale;

    Pixmap cache;                       /* Pixmap used to cache elements
                                         * displayed.  If *backingStore* is
                                         * non-zero, each element is drawn
                                         * into this pixmap before it is
                                         * copied onto the screen.  The
                                         * pixmap then acts as a cache
                                         * (only the pixmap is redisplayed
                                         * if the none of elements have
                                         * changed). This is done so that
                                         * markers can be redrawn quickly
                                         * over elements without redrawing
                                         * each element. */
    short int cacheWidth, cacheHeight;  /* Size of element backing store
                                         * pixmap. */

    Blt_HashTable colormapTable;        /* Table of colormaps. */
    int nextColormapId;

    Playback play;

    /*
     * Barchart-specific information
     */
    float baseline;                     /* Baseline from bar chart.  */
    float barWidth;                     /* Default width of each bar in
                                         * graph units.  The default width
                                         * is 1.0 units. */
    BarMode mode;                       /* Mode describing how to display
                                         * bars with the same
                                         * x-coordinates. Mode can be
                                         * "stacked", "aligned", "overlap",
                                         * or "infront" */
    int numBarGroups;                   /* # of entries in barGroups array.
                                         * If zero, indicates nothing
                                         * special needs to be * done for
                                         * "stack" or "align" modes */
    Blt_HashTable groupTable;           /* Table managing sets of bars with
                                         * the same abscissas. The bars in
                                         * a set may be displayed is
                                         * various ways: aligned, overlap,
                                         * infront, or stacked. */
    int maxBarGroupSize;

    /* Contour graph-specific fields. */
    Blt_HashTable palTable;             /* Table of color palettes to be
                                         * displayed. */
    Blt_HashTable meshTable;            /* Table of meshes. */
    unsigned int nextMeshId;
};

/*
 * Bit flags definitions:
 *
 *      All kinds of state information kept here.  All these things happen
 *      when the window is available to draw into (DisplayGraph). Need the
 *      window width and height before we can calculate graph layout
 *      (i.e. the screen coordinates of the axes, elements, titles,
 *      etc). But we want to do this only when we have to, not every time
 *      the graph is redrawn.
 *
 *      Same goes for maintaining a pixmap to double buffer graph elements.
 *      Need to mark when the pixmap needs to updated.
 *
 *
 *      MAP_ITEM                Indicates that the element/marker/axis 
 *                              configuration has changed such that its
 *                              layout of the item (i.e. its position in
 *                              the graph window) needs to be recalculated.
 *
 *      MAP_ALL                 Indicates that the layout of the axes and 
 *                              all elements and markers and the graph need
 *                              to be recalculated. Otherwise, the layout
 *                              of only those markers and elements that
 *                              have changed will be reset.
 *
 *      GET_AXIS_GEOMETRY       Indicates that the size of the axes needs 
 *                              to be recalculated. 
 *
 *      RESET_AXES              Flag to call to Blt_ResetAxes routine.
 *                              This routine recalculates the scale offset
 *                              (used for mapping coordinates) of each
 *                              axis.  If an axis limit has changed, then
 *                              it sets flags to re-layout and redraw the
 *                              entire graph.  This needs to happen before
 *                              the axis can compute transformations
 *                              between graph and screen coordinates.
 *
 *      LAYOUT_NEEDED           
 *
 *      CACHE_DIRTY             If set redraw all elements into the pixmap 
 *                              used for buffering elements. 
 *
 *      REDRAW_PENDING          Non-zero means a DoWhenIdle handler has 
 *                              already been queued to redraw this window. 
 *
 *      DRAW_LEGEND             Non-zero means redraw the legend. If this is 
 *                              the only DRAW_* flag, the legend display
 *                              routine is called instead of the graph
 *                              display routine.
 *
 *      DRAW_MARGINS            Indicates that the margins bordering 
 *                              the plotting area need to be redrawn. 
 *                              The possible reasons are:
 *
 *                              1) an axis configuration changed
 *                              2) an axis limit changed
 *                              3) titles have changed
 *                              4) window was resized. 
 *
 *      GRAPH_FOCUS     
 */

#define HIDDEN                  (1<<0)
#define DELETED                 (1<<1)
#define REDRAW_PENDING          (1<<2)  /* Indicates a DoWhenIdle handler
                                         * has already been queued to
                                         * redraw this window.  */

#define ACTIVE_PENDING          (1<<3)
#define MAP_ITEM                (1<<4)  /* Indicates that the
                                         * element/marker/axis
                                         * configuration has changed such
                                         * that its layout of the item
                                         * (i.e. its position in the graph
                                         * window) needs to be
                                         * recalculated. */
#define DIRTY                   (1<<5)
#define ACTIVE                  (1<<6)
#define FOCUS                   (1<<7)

#define MAP_ALL                 (1<<8)  /* Indicates that the layout of the
                                         * axes and all elements and
                                         * markers and the graph need to be
                                         * recalculated. Otherwise, the
                                         * layout of only those markers and
                                         * elements that have changed will
                                         * be reset. */

#define LAYOUT_NEEDED           (1<<9)
#define RESET_AXES              (1<<10) /* Flag to call to Blt_ResetAxes
                                         * routine.  This routine
                                         * recalculates the scale offset
                                         * (used for mapping coordinates)
                                         * of each axis.  If an axis limit
                                         * has changed, then it sets flags
                                         * to re-layout and redraw the
                                         * entire graph.  This needs to
                                         * happend before the axis can
                                         * compute transformations between
                                         * graph and screen coordinates. */

#define GET_AXIS_GEOMETRY       (1<<11)

#define DRAW_LEGEND             (1<<12)
#define DRAW_MARGINS            (1<<13)
#define CACHE_DIRTY             (1<<14)
#define REQ_BACKING_STORE       (1<<15)
#define MAP_VISIBLE             (1<<16)

#define MAP_WORLD               (MAP_ALL|RESET_AXES|GET_AXIS_GEOMETRY)
#define REDRAW_WORLD            (DRAW_LEGEND)
#define RESET_WORLD             (REDRAW_WORLD | MAP_WORLD)

#define DOUBLE_BUFFER           (1<<18)
#define BACKING_STORE           (1<<19)
#define STACK_AXES              (1<<20)
#define INVERTED                (1<<21)
#define STRETCH_TO_FIT          (1<<22)
#define REGION_ENABLED          (1<<23)

/*
 * ---------------------- Forward declarations ------------------------
 */
BLT_EXTERN int Blt_CreatePageSetup(Graph *graphPtr);

BLT_EXTERN int Blt_CreateCrosshairs(Graph *graphPtr);

BLT_EXTERN double Blt_InvHMap(Axis *axisPtr, double x);

BLT_EXTERN double Blt_InvVMap(Axis *axisPtr, double x);

BLT_EXTERN double Blt_HMap(Axis *axisPtr, double x);

BLT_EXTERN double Blt_VMap(Axis *axisPtr, double y);

BLT_EXTERN Point2d Blt_InvMap2D(Graph *graphPtr, double x, double y, 
        Axis2d *pairPtr);

BLT_EXTERN Point2d Blt_Map2D(Graph *graphPtr, double x, double y, 
        Axis2d *pairPtr);

BLT_EXTERN Graph *Blt_GetGraphFromWindowData(Tk_Window tkwin);

BLT_EXTERN void Blt_AdjustAxisPointers(Graph *graphPtr);

BLT_EXTERN void Blt_ComputeStacks(Graph *graphPtr);

BLT_EXTERN void Blt_ConfigureCrosshairs(Graph *graphPtr);
BLT_EXTERN void Blt_ConfigureLegend(Graph *graphPtr);
BLT_EXTERN void Blt_ConfigureElements(Graph *graphPtr);
BLT_EXTERN void Blt_ConfigureAxes(Graph *graphPtr);
BLT_EXTERN void Blt_ConfigureMarkers(Graph *graphPtr);
BLT_EXTERN void Blt_ReconfigureGraph(Graph *graphPtr);

BLT_EXTERN void Blt_DestroyAxes(Graph *graphPtr);

BLT_EXTERN void Blt_DestroyCrosshairs(Graph *graphPtr);

BLT_EXTERN void Blt_DestroyElements(Graph *graphPtr);

BLT_EXTERN void Blt_DestroyMarkers(Graph *graphPtr);

BLT_EXTERN void Blt_DestroyPageSetup(Graph *graphPtr);

BLT_EXTERN void Blt_DrawAxes(Graph *graphPtr, Drawable drawable);

BLT_EXTERN void Blt_DrawAxisLimits(Graph *graphPtr, Drawable drawable);

BLT_EXTERN void Blt_DrawElements(Graph *graphPtr, Drawable drawable);

BLT_EXTERN void Blt_DrawActiveElements(Graph *graphPtr, Drawable drawable);

BLT_EXTERN void Blt_DrawMarkers(Graph *graphPtr, Drawable drawable, int under);

BLT_EXTERN void Blt_DrawSegments2d(Display *display, Drawable drawable, GC gc, 
        Segment2d *segments, int numSegments);

BLT_EXTERN int Blt_GetCoordinate(Tcl_Interp *interp, const char *string, 
        double *valuePtr);

BLT_EXTERN void Blt_InitBarGroups(Graph *graphPtr);
BLT_EXTERN void Blt_DestroyBarGroups(Graph *graphPtr);
BLT_EXTERN void Blt_ResetBarGroups(Graph *graphPtr);

BLT_EXTERN void Blt_LayoutGraph(Graph *graphPtr);

BLT_EXTERN void Blt_EventuallyRedrawGraph(Graph *graphPtr);

BLT_EXTERN void Blt_ResetAxes(Graph *graphPtr);


BLT_EXTERN void Blt_DisableCrosshairs(Graph *graphPtr);

BLT_EXTERN void Blt_EnableCrosshairs(Graph *graphPtr);

BLT_EXTERN void Blt_MapGraph(Graph *graphPtr);

BLT_EXTERN void Blt_MapAxes(Graph *graphPtr);

BLT_EXTERN void Blt_MapElements(Graph *graphPtr);

BLT_EXTERN void Blt_MapMarkers(Graph *graphPtr);

BLT_EXTERN void Blt_UpdateCrosshairs(Graph *graphPtr);

BLT_EXTERN void Blt_DestroyPens(Graph *graphPtr);

BLT_EXTERN int Blt_GetPenFromObj(Tcl_Interp *interp, Graph *graphPtr, 
        Tcl_Obj *objPtr, ClassId classId, Pen **penPtrPtr);

BLT_EXTERN Pen *Blt_CreateBarPen(Graph *graphPtr, Blt_HashEntry *hPtr);
BLT_EXTERN Pen *Blt_CreateLinePen(Graph *graphPtr, ClassId id, 
        Blt_HashEntry *hPtr);
BLT_EXTERN Pen *Blt_CreateLinePen2(Graph *graphPtr, ClassId id, 
        Blt_HashEntry *hPtr);
BLT_EXTERN Pen *Blt_CreateContourPen(Graph *graphPtr, ClassId id, 
        Blt_HashEntry *hPtr);

BLT_EXTERN Pen *Blt_CreatePen(Graph *graphPtr, const char *penName, 
        ClassId classId, int objc, Tcl_Obj *const *objv);

BLT_EXTERN int Blt_InitLinePens(Graph *graphPtr);

BLT_EXTERN int Blt_InitBarPens(Graph *graphPtr);

BLT_EXTERN void Blt_FreePen(Pen *penPtr);

BLT_EXTERN Tcl_ObjCmdProc Blt_VirtualAxisOp;

BLT_EXTERN int Blt_AxisOp(ClientData clientData, Tcl_Interp *interp,
        int margin, int objc, Tcl_Obj *const *objv);

BLT_EXTERN int Blt_ElementOp(Graph *graphPtr, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv, ClassId classId);

BLT_EXTERN Tcl_ObjCmdProc Blt_IsolineOp;
BLT_EXTERN Tcl_ObjCmdProc Blt_CrosshairsOp;
BLT_EXTERN Tcl_ObjCmdProc Blt_GraphRegionOp;
BLT_EXTERN Tcl_ObjCmdProc Blt_MarkerOp;
BLT_EXTERN Tcl_ObjCmdProc Blt_PenOp;
BLT_EXTERN Tcl_ObjCmdProc Blt_PostScriptOp;

BLT_EXTERN int Blt_GraphUpdateNeeded(Graph *graphPtr);

BLT_EXTERN int Blt_DefaultAxes(Graph *graphPtr);

BLT_EXTERN Axis *Blt_GetFirstAxis(Blt_Chain chain);

BLT_EXTERN void Blt_UpdateAxisBackgrounds(Graph *graphPtr);

BLT_EXTERN Marker *Blt_NearestMarker(Graph *graphPtr, int x, int y, int under);

BLT_EXTERN Element *Blt_NearestElement(Graph *graphPtr, int x, int y);

BLT_EXTERN Axis *Blt_NearestAxis(Graph *graphPtr, int x, int y);

BLT_EXTERN Isoline *Blt_NearestIsoline(Graph *graphPtr, int x, int y);

typedef ClientData (MakeTagProc)(Graph *graphPtr, const char *tagName);

BLT_EXTERN MakeTagProc Blt_MakeElementTag;
BLT_EXTERN MakeTagProc Blt_MakeMarkerTag;
BLT_EXTERN MakeTagProc Blt_MakeAxisTag;
BLT_EXTERN MakeTagProc Blt_MakeIsolineTag;
BLT_EXTERN Blt_BindAppendTagsProc Blt_GraphTags;
BLT_EXTERN Blt_BindAppendTagsProc Blt_AxisTags;

BLT_EXTERN int Blt_GraphType(Graph *graphPtr);

BLT_EXTERN void Blt_UpdateGraph(ClientData clientData);

BLT_EXTERN void Blt_GraphSetObjectClass(GraphObj *graphObjPtr,ClassId classId);

BLT_EXTERN void Blt_MarkersToPostScript(Graph *graphPtr, Blt_Ps ps, int under);
BLT_EXTERN void Blt_ElementsToPostScript(Graph *graphPtr, Blt_Ps ps);
BLT_EXTERN void Blt_ActiveElementsToPostScript(Graph *graphPtr, Blt_Ps ps);
BLT_EXTERN void Blt_LegendToPostScript(Graph *graphPtr, Blt_Ps ps);
BLT_EXTERN void Blt_AxesToPostScript(Graph *graphPtr, Blt_Ps ps);
BLT_EXTERN void Blt_AxisLimitsToPostScript(Graph *graphPtr, Blt_Ps ps);

BLT_EXTERN Element *Blt_BarElement(Graph *graphPtr, Blt_HashEntry *hPtr);
BLT_EXTERN Element *Blt_LineElement(Graph *graphPtr, ClassId id,
        Blt_HashEntry *hPtr);
BLT_EXTERN Element *Blt_LineElement2(Graph *graphPtr, ClassId id,
        Blt_HashEntry *hPtr);
BLT_EXTERN Element *Blt_ContourElement(Graph *graphPtr, ClassId id,
        Blt_HashEntry *hPtr);

BLT_EXTERN void Blt_ClearIsolines(Graph *graphPtr, Element *elemPtr);
BLT_EXTERN void Blt_DestroyIsolines(Graph *graphPtr);

BLT_EXTERN void Blt_DrawGrids(Graph *graphPtr, Drawable drawable);

BLT_EXTERN void Blt_GridsToPostScript(Graph *graphPtr, Blt_Ps ps);

BLT_EXTERN void Blt_HoldPlayback(Graph *graphPtr);
BLT_EXTERN void Blt_ContinuePlayback(Graph *graphPtr);
BLT_EXTERN void Blt_DestroyPlayback(Graph *graphPtr);
BLT_EXTERN int Blt_CreatePlayback(Graph *graphPtr);

/* ---------------------- Global declarations ------------------------ */

BLT_EXTERN const char *Blt_GraphClassName(ClassId classId);

#endif /* _BLT_GRAPH_H */

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrElem.h --
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

#ifndef _BLT_GR_ELEM_H
#define _BLT_GR_ELEM_H

#include <bltVector.h>
#include <bltDataTable.h>

#define ELEM_SOURCE_VALUES	0
#define ELEM_SOURCE_VECTOR	1
#define ELEM_SOURCE_TABLE	2

#define SHOW_NONE		0
#define SHOW_X			1
#define SHOW_Y			2
#define SHOW_BOTH		3

#define NEAREST_SEARCH_X	0
#define NEAREST_SEARCH_Y	1
#define NEAREST_SEARCH_XY	2

#define NEAREST_SEARCH_POINTS	0	/* Search for nearest data point. */
#define NEAREST_SEARCH_TRACES	1	/* Search for nearest point on trace.
					 * Interpolate the connecting line
					 * segments if necessary. */
#define NEAREST_SEARCH_AUTO	2	/* Automatically determine whether
					 * to search for data points or
					 * traces.  Look for traces if the
					 * linewidth is > 0 and if there is
					 * more than one data point. */

#define	LABEL_ACTIVE (1<<9)		/* Non-zero indicates that the
					 * element's entry in the legend
					 * should be drawn in its active
					 * foreground and background
					 * colors. */
#define SCALE_SYMBOL (1<<10)

#define NUMBEROFPOINTS(e)	MIN((e)->x.numValues, (e)->y.numValues)

#define NORMALPEN(e)		((((e)->normalPenPtr == NULL) ?  \
				  (e)->builtinPenPtr :		 \
				  (e)->normalPenPtr))

/*
 *---------------------------------------------------------------------------
 *
 * Weight --
 *
 *	Designates a range of values by a minimum and maximum limit.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    double min, max, range;
} Weight;

#define SetRange(l) \
	((l).range = ((l).max > (l).min) ? ((l).max - (l).min) : DBL_EPSILON)
#define SetScale(l) \
	((l).scale = 1.0 / (l).range)
#define SetWeight(l, lo, hi) \
	((l).min = (lo), (l).max = (hi), SetRange(l))

typedef struct {
    Segment2d *segments;	/* Point to start of this pen's X-error bar
				 * segments in the element's array. */
    int numSegments;
} ErrorBarSegments;

/* 
 * An element has one or more vectors plus several attributes, such as line
 * style, thickness, color, and symbol type.  It has an identifier which
 * distinguishes it among the list of all elements.
 */
typedef struct {
    Weight weight;                      /* Weight range where this pen is
					 * valid. */
    Pen *penPtr;                        /* Pen to use. */
} PenStyle;


typedef struct {
    XColor *color;                      /* Color of error bar */
    int lineWidth;                      /* Width of the error bar
					 * segments. */
    GC gc;
    int show;                           /* Flags for errorbars: none, x, y,
					 * or both */
} ErrorBarAttributes;

typedef struct {
    /* Inputs */
    int halo;				/* Maximal screen distance a
					 * candidate point can be from the
					 * sample window coordinate. */
    int mode;				/* Indicates whether to find the
					 * nearest data point or the
					 * nearest point on the trace by
					 * interpolating the line segments.
					 * Can also be NEAREST_SEARCH_AUTO,
					 * indicating to choose how to
					 * search.*/
    int x, y;				/* Screen coordinates of test
					 * point */
    int along;				/* Indicates to let search run
					 * along a particular axis: x, y,
					 * or both. */
    int all;				

    /* Outputs */
    void *item;				/* Pointer to the nearest element
					 * or isoline. */
    Point2d point;			/* Graph coordinates of nearest
					 * point */
    int index;				/* Index of nearest data point */
    double distance, maxDistance;
} NearestElement;

typedef void (ElementDrawProc) (Graph *graphPtr, Drawable drawable, 
	Element *elemPtr);
typedef void (ElementToPostScriptProc) (Graph *graphPtr, Blt_Ps ps, 
	Element *elemPtr);
typedef void (ElementDestroyProc) (Graph *graphPtr, Element *elemPtr);
typedef int (ElementConfigProc) (Graph *graphPtr, Element *elemPtr);
typedef void (ElementMapProc) (Graph *graphPtr, Element *elemPtr);
typedef void (ElementExtentsProc) (Element *elemPtr);
typedef void (ElementNearestProc) (Graph *graphPtr, Element *elemPtr, 
	NearestElement *nearestPtr);
typedef Blt_Chain (ElementFindProc) (Graph *graphPtr, Element *elemPtr, 
	int x, int y, int r);
typedef void (ElementDrawSymbolProc) (Graph *graphPtr, Drawable drawable, 
	Element *elemPtr, int x, int y, int symbolSize);
typedef void (ElementSymbolToPostScriptProc) (Graph *graphPtr, 
	Blt_Ps ps, Element *elemPtr, double x, double y, int symSize);

typedef struct {
    ElementNearestProc *nearestProc;
    ElementConfigProc *configProc;
    ElementDestroyProc *destroyProc;
    ElementDrawProc *drawActiveProc;
    ElementDrawProc *drawNormalProc;
    ElementDrawSymbolProc *drawSymbolProc;
    ElementExtentsProc *extentsProc;
    ElementFindProc *findProc;
    ElementToPostScriptProc *printActiveProc;
    ElementToPostScriptProc *printNormalProc;
    ElementSymbolToPostScriptProc *printSymbolProc;
    ElementMapProc *mapProc;
} ElementProcs;

typedef struct {
    Blt_VectorId vector;
} VectorDataSource;

typedef struct {
    BLT_TABLE table;			/* Data table. */ 
    BLT_TABLE_COLUMN column;		/* Column of data used. */
    BLT_TABLE_NOTIFIER notifier;        /* Notifier used for column destroy
					 * event. */
    BLT_TABLE_TRACE trace;		/* Trace used for column
					 * (set/get/unset). */
    Blt_HashEntry *hashPtr;		/* Pointer to the entry of the data
					 * source in graph's hash table of
					 * datatables. One graph may use
					 * multiple columns from the same
					 * data table. */
} TableDataSource;

/* 
 * The data structure below contains information pertaining to a line
 * vector.  It consists of an array of floating point data values and for
 * convenience, the number and minimum/maximum values.
 */
typedef struct {
    int type;				/* Selects the type of data
					 * populating this vector:
					 * ELEM_SOURCE_VECTOR, *
					 * ELEM_SOURCE_TABLE, or
					 * ELEM_SOURCE_VALUES */
    Element *elemPtr;			/* Element associated with
					 * vector. */
    union {
	TableDataSource tableSource;
	VectorDataSource vectorSource;
    };
    double *values;
    int numValues;
    int arraySize;
    double min, max;
} ElemValues;


struct _Element {
    GraphObj obj;                       /* Must be first field in
					 * element. */
    unsigned int flags;		
    Blt_HashEntry *hashPtr;

    /* Fields specific to elements. */
    Blt_ChainLink link;			/* Element's link in display
					 * list. */
    const char *label;			/* Label displayed in legend */
    unsigned short row, col;		/* Position of the entry in the
					 * legend. */
    int legendRelief;			/* Relief of label in legend. */
    Axis2d axes;			/* X-axis and Y-axis mapping the
					 * element */
    ElemValues x, y, w;			/* Contains array of floating point
					 * graph coordinate values. Also
					 * holds min/max and the number of
					 * coordinates */
    Blt_HashTable activeTable;		/* Table of indices which indicate
					 * which data points are active
					 * (drawn with "active" colors). */
    int numActiveIndices;		/* Number of active data points.
					 * Special case: if
					 * numActiveIndices < 0 and the
					 * active bit is set in "flags",
					 * then all data points are drawn
					 * active. */
    ElementProcs *procsPtr;
    Blt_ConfigSpec *configSpecs;	/* Configuration specifications. */
    Pen *activePenPtr;			/* Standard Pens */
    Pen *normalPenPtr;
    Pen *builtinPenPtr;
    Blt_Chain styles;			/* Palette of pens. */

    /* Symbol scaling */
    int scaleSymbols;			/* If non-zero, the symbols will
					 * scale in size as the graph is
					 * zoomed in/out.  */
    double xRange, yRange;		/* Initial X-axis and Y-axis
					 * ranges: used to scale the size
					 * of element's symbol. */
    int state;
};


BLT_EXTERN double Blt_FindElemValuesMinimum(ElemValues *vecPtr, double minLimit);
BLT_EXTERN void Blt_ResizeStatusArray(Element *elemPtr, int numPoints);
BLT_EXTERN int Blt_GetPenStyle(Graph *graphPtr, char *name, size_t classId, 
	PenStyle *stylePtr);
BLT_EXTERN void Blt_FreeStyles (Blt_Chain styles);
BLT_EXTERN PenStyle **Blt_StyleMap (Element *elemPtr);
BLT_EXTERN void Blt_MapErrorBars(Graph *graphPtr, Element *elemPtr, 
	PenStyle **dataToStyle);
BLT_EXTERN void Blt_FreeDataValues(ElemValues *evPtr);
BLT_EXTERN int Blt_GetElement(Tcl_Interp *interp, Graph *graphPtr, 
	Tcl_Obj *objPtr, Element **elemPtrPtr);
BLT_EXTERN void Blt_DestroyTableClients(Graph *graphPtr);
BLT_EXTERN void Blt_DestroyElementTags(Graph *graphPtr);

BLT_EXTERN void Blt_GraphExtents(void *elemPtr, Region2d *extsPtr);
BLT_EXTERN Tcl_ObjCmdProc Blt_IsolineOp;
BLT_EXTERN Tcl_ObjCmdProc Blt_IsoTagOp;

#endif /* _BLT_GR_ELEM_H */

/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrAxis.h --
 *
 *	Copyright 1993-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use, copy,
 *	modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */

#ifndef _BLT_GR_AXIS_H
#define _BLT_GR_AXIS_H

/*
 *---------------------------------------------------------------------------
 *
 * Grid --
 *
 *	Contains attributes of describing how to draw grids (at major
 *	ticks) in the graph.  Grids may be mapped to either/both X and Y
 *	axis.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    Blt_Dashes dashes;			/* Dash style of the grid. This
					 * represents an array of
					 * alternatingly drawn pixel
					 * values. */
    int lineWidth;			/* Width of the grid lines */
    XColor *color;			/* Color of the grid lines */
    GC gc;				/* Graphics context for the grid. */

    Segment2d *segments;		/* Array of line segments
					 * representing the grid lines */
    int numUsed;			/* # of axis segments in use. */
    int numAllocated;			/* # of axis segments allocated. */
} Grid;

/*
 *---------------------------------------------------------------------------
 *
 * AxisRange --
 *
 *	Designates a range of values by a minimum and maximum limit.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    double min, max, range, scale;
} AxisRange;

/*
 *---------------------------------------------------------------------------
 *
 * TickLabel --
 *
 * 	Structure containing the X-Y screen coordinates of the tick label
 * 	(anchored at its center).
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    Point2d anchorPos;
    unsigned int width, height;
    char string[1];
} TickLabel;

typedef enum  _AxisScaleType {
    SCALE_LINEAR, SCALE_LOG, SCALE_TIME, SCALE_CUSTOM
}  AxisScaleType;

/*
 *---------------------------------------------------------------------------
 *
 * Ticks --
 *
 * 	Structure containing information where the ticks (major or minor)
 *	will be displayed on the graph.
 *
 *---------------------------------------------------------------------------
 */
typedef struct {
    double initial;			/* Initial value */
    double step;                        /* Size of interval */
    int numSteps;			/* Number of intervals. */
    int index;                          /* Current index of iterator. */
    AxisScaleType scaleType;            /* Scale type. */
    double range;                       /* Range of entire sweep. */
    int isLeapYear;                     /* Indicates if the major tick
                                         * value is a leap year. */
    int timeUnits;                      /* Indicates the time units of the
                                         * sweep. */
    double *values;                     /* Array of tick values
                                         * (malloc-ed). */
    Grid grid;                          /* Axis grid information. */
    int month;
    int year;
    int timeFormat;
    time_t numDaysFromInitial;
} Ticks;

typedef struct {
    Ticks ticks;
    Grid grid;                          /* Axis grid information. */
} TickGrid;
/*
 *---------------------------------------------------------------------------
 *
 * Axis --
 *
 * 	Structure contains options controlling how the axis will be
 * 	displayed.
 *
 *---------------------------------------------------------------------------
 */
struct _Axis {
    GraphObj obj;			/* Must be first field in axis. */

    unsigned int flags;		

    Blt_HashEntry *hashPtr;

    /* Fields specific to axes. */

    const char *detail;
    int refCount;                       /* Number of elements referencing
					 * this axis. */
    int logScale;			/* If non-zero, generate log scale
					 * ticks for the axis. */
    int timeScale;			/* If non-zero, generate time scale
					 * ticks for the axis. This option
					 * is overridden by -logscale. */
    int decreasing;			/* If non-zero, display the range
					 * of values on the axis in
					 * descending order, from high to
					 * low. */
    int looseMin, looseMax;		/* If non-zero, axis range extends
					 * to the outer major ticks,
					 * otherwise at the limits of the
					 * data values. This is
					 * overriddened by setting the -min
					 * and -max options.  */
    const char *title;			/* Title of the axis. */
    int titleAlternate;			/* Indicates whether to position
					 * the title above/left of the
					 * axis. */
    Point2d titlePos;			/* Position of the title */
    short int titleWidth, titleHeight;	
    int lineWidth;			/* Width of lines representing axis
					 * (including ticks).  If zero,
					 * then no axis lines or ticks are
					 * drawn. */
    const char **limitsFormats;		/* One or two strings of
					 * sprintf-like formats describing
					 * how to display virtual axis
					 * limits. If NULL, display no
					 * limits. */
    int numFormats;
    TextStyle limitsTextStyle;		/* Text attributes (color, font,
					 * rotation, etc.)  of the
					 * limits. */
    double windowSize;			/* Size of a sliding window of
					 * values used to scale the axis
					 * automatically as new data values
					 * are added. The axis will always
					 * display the latest values in
					 * this range. */
    double shiftBy;			/* Shift maximum by this
                                         * interval. */
    int tickLength;			/* Length of major ticks in
                                         * pixels */
    Tcl_Obj *fmtCmdObjPtr;		/* Specifies a TCL command, to be
					 * invoked by the axis whenever it
					 * has to generate tick labels. */
    Tcl_Obj *scrollCmdObjPtr;
    int scrollUnits;
    double min, max;			/* The actual axis range. */
    double reqMin, reqMax;		/* Requested axis bounds. Consult
					 * the axisPtr->flags field for
					 * AXIS_CONFIG_MIN and
					 * AXIS_CONFIG_MAX to see if the
					 * requested bound have been set.
					 * They override the computed range
					 * of the axis (determined by
					 * auto-scaling). */

    double reqScrollMin, reqScrollMax;

    double scrollMin, scrollMax;	/* Defines the scrolling reqion of
					 * the axis.  Normally the region
					 * is determined from the data
					 * limits. If specified, these
					 * values override the
					 * data-range. */

    AxisRange valueRange;		/* Range of data values of elements
					 * mapped to this axis. This is
					 * used to auto-scale the axis in
					 * "tight" mode. */
    AxisRange axisRange;		/* Smallest and largest major tick
					 * values for the axis.  The tick
					 * values lie outside the range of
					 * data values.  This is used to
					 * auto-scale the axis in "loose"
					 * mode. */
    double prevMin, prevMax;
    double reqStep;			/* If > 0.0, overrides the computed
					 * major tick interval.  Otherwise
					 * a stepsize is automatically
					 * calculated, based upon the range
					 * of elements mapped to the
					 * axis. The default value is
					 * 0.0. */
    TickGrid minor, major;

    int reqNumMajorTicks;		/* Default number of ticks to be
					 * displayed. */
    int reqNumMinorTicks;		/* If non-zero, represents the
					 * requested the number of minor
					 * ticks to be uniformally
					 * displayed along each major
					 * tick. */
    int labelOffset;			/* If non-zero, indicates that the
					 * tick label should be offset to
					 * sit in the middle of the next
					 * interval. */

    /* The following fields are specific to logical axes */

    int margin;				/* Margin that contains this
                                         * axis. */
    Blt_ChainLink link;			/* Axis link in margin list. */
    Blt_Chain chain;
    Segment2d *segments;		/* Array of line segments
					 * representing the major and minor
					 * ticks, but also the * axis line
					 * itself. The segment coordinates
					 * * are relative to the axis. */
    int numSegments;			/* Number of segments in the above
					 * array. */
    Blt_Chain tickLabels;		/* Contains major tick label
					 * strings and their offsets along
					 * the axis. */
    short int left, right, top, bottom;	/* Region occupied by the of axis. */
    short int width, height;		/* Extents of axis */
    short int maxLabelWidth;            /* Maximum width of all ticks
					 * labels. */
    short int maxLabelHeight;           /* Maximum height of all tick
					 * labels. */
    Blt_Bg normalBg;
    Blt_Bg activeBg;
    XColor *activeFgColor;

    int relief;
    int borderWidth;
    int activeRelief;

    float tickAngle;	
    Blt_Font tickFont;
    Tk_Anchor tickAnchor;
    Tk_Anchor reqTickAnchor;
    XColor *tickColor;
    GC tickGC;				/* Graphics context for axis and
					 * tick labels */
    GC activeTickGC;

    double titleAngle;	
    Blt_Font titleFont;
    Tk_Anchor titleAnchor;
    Tk_Justify titleJustify;
    XColor *titleColor;
    
    double screenScale;
    int screenMin, screenRange;
    Blt_Palette palette;
    float weight;
};

BLT_EXTERN void Blt_GetAxisGeometry(Graph *graphPtr, Axis *axisPtr);

#endif /* _BLT_GR_AXIS_H */

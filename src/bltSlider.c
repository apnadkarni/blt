/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltSlider.c --
 *
 * This module implements a slider widget for the BLT toolkit.
 *
 *	Copyright 2012 George A Howlett.
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

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_SLIDER

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltChain.h"
#include "bltHash.h"
#include "bltFont.h"
#include "bltBg.h"
#include "bltImage.h"
#include "bltPicture.h"
#include "bltPainter.h"
#include "bltText.h"
#include "bltBind.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define MAXTICKS	10001

#define FCLAMP(x)	((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))

/*
 * Round x in terms of units
 */
#define UROUND(x,u)	(Round((x)/(u))*(u))
#define UCEIL(x,u)	(ceil((x)/(u))*(u))
#define UFLOOR(x,u)	(floor((x)/(u))*(u))

#define NUMDIGITS	15		/* Specifies the number of digits of
					 * accuracy used when outputting axis
					 * tick labels. */

#define REDRAW_PENDING	(1<<0)
#define LAYOUT_PENDING	(1<<1)
#define GEOMETRY	(1<<2)
#define RANGEMODE	(1<<3)
#define STATE_NORMAL	(1<<4)
#define STATE_DISABLED  (1<<5)
#define STATE_ACTIVE	(1<<6)
#define STATE_MASK	(STATE_NORMAL|STATE_DISABLED|STATE_ACTIVE)
#define DECREASING	(1<<8)		/* Axis is decreasing */

#define SIDE_TOP	(1<<9)		/* Axis is at the top of the
					 * widget. */
#define SIDE_BOTTOM	(1<<9)		/* Axis is at the bottom. */
#define SIDE_LEFT	(1<<9)		/* Axis is on the left side. */
#define SIDE_RIGHT	(1<<9)		/* Axis is on the right side. */
#define STATE_MASK	(SIDE_TOP|SIDE_BOTTOM|SIDE_LEFT|SIDE_RIGHT)
#define HORIZONTAL	(SIDE_TOP|SIDE_BOTTOM) /* Axis is horizontal */
#define VERTICAL	(SIDE_LEFT|SIDE_RIGHT) /* Axis is vertial */
#define AUTO_MAJOR	(1<<8)		/* Auto-generate major ticks. */
#define AUTO_MINOR	(1<<9)		/* Auto-generate minor ticks. */
#define SHOWTICKS	(1<<10)		/* Display axis ticks and labels. */
#define SHOWVALUES	(1<<11)		/* Display value(s) above the slider. */
#define SHOWARROWS	(1<<12)		/* Display single increment controls. */
#define LOGSCALE	(1<<13)
#define TIMESCALE	(1<<14)
#define AXIS_GEOMETRY	(1<<14)		/* Axis geometry needs to be
					 * recomputed because the "from"
					 * and/or "to" value of the slider
					 * has changed. */

/*
-activesliderbackground
-sliderbackground
-rangebackground
-activerangebackground 
-disabledrangebackground 
-troughcolor
-disabledtroughcolor
-arrowcolor
-arrowforeground
-arrowbackground
-tickforeground
-titleforeground

-foreground
-activeforeground*
-activebackground
-background
-disabledbackground*
*/

#define DEF_DISABLED_TROUGH_COLOR	"grey97"
#define DEF_NORMAL_TROUGH_COLOR		"grey85"
#define DEF_ACTIVE_RANGE_BG		"grey92"
#define DEF_NORMAL_RANGE_BG		"grey85"
#define DEF_DISABLED_RANGE_BG		"grey85"
#define DEF_ACTIVE_SLIDER_COLOR		"grey95"
#define DEF_NORMAL_SLIDER_COLOR		"grey90"
#define DEF_DISABLED_SLIDER_COLOR	"grey90"
#define DEF_ACTIVE_ARROW_BG		"grey95"
#define DEF_NORMAL_ARROW_BG		"grey90"
#define DEF_TICK_COLOR			"black"
#define DEF_TITLE_COLOR			"black"

#define DEF_SIDE			"bottom"
#define DEF_ORIENT			"horizontal"
#define DEF_SHOW_TICKS			"1"
#define DEF_SHOW_VALUES			"1"
#define DEF_SHOW_ARROWS			"1"
#define DEF_LOGSCALE			"0"

#define DEF_STEP		"0.0"
#define DEF_SUBDIVISIONS	"2"
#define DEF_TAGS		"all"
#define DEF_TICK_ANCHOR		"c"
#define DEF_TICKFONT		STD_FONT_SMALL
#define DEF_TICKLENGTH		"4"
#define DEF_DIVISIONS		"10"
#define DEF_LABEL		(char *)NULL
#define DEF_TAKE_FOCUS		(char *)NULL
#define DEF_TITLE_FG		RGB_BLACK
#define DEF_TITLE_FONT		"{Sans Serif} 10"
#define DEF_MIN			"0.0"
#define DEF_MAX			"1.0"
#define DEF_FROM		"0.0"
#define DEF_TO			"1.0"


/*
 * SliderRange --
 *
 *	Designates a range of values by a minimum and maximum limit.
 */
typedef struct {
    double min, max, range, scale;
} SliderRange;

/*
 * TickLabel --
 *
 * 	Structure containing the X-Y screen coordinates of the tick label
 * 	(anchored at its center).
 */
typedef struct {
    short int x, y;
    short int width, height;
    char string[1];
} TickLabel;

/*
 *
 * Ticks --
 *
 * 	Structure containing information where the ticks (major or minor)
 * 	will be displayed on the slider.
 */
typedef struct {
    int numTicks;			/* # of ticks on axis */
    double values[1];                   /* Array of tick values
					 * (malloc-ed). */
} Ticks;

/*
 * TickSweep --
 *
 * 	Structure containing information where the ticks (major or minor)
 * 	will be displayed on the slider.
 */
typedef struct {
    double initial;			/* Initial value */
    double step;			/* Size of interval */
    int numSteps;			/* Number of intervals. */
} TickSweep;

#define LOGSCALE		(1<<0)
#define LABELOFFSET		(1<<2)

/*
 * Slider --
 *
 * 	Structure contains options controlling how the axis will be displayed.
 */
typedef struct {
    unsigned int flags;			/* Flags: See below for
					 * definitions. */
    Tcl_Interp *interp;			/* Interpreter associated with
					 * slider. */
    Tk_Window tkwin;			/* Window that embodies the slider.
					 * NULL means that the window has
					 * been destroyed but the data
					 * structures haven't yet been
					 * cleaned up. */
    Display *display;			/* Display containing widget; used
					 * to release resources after tkwin
					 * has already gone away. */
    Tcl_Command cmdToken;		/* Token for the slider widget
					 * command. */
    const char *data;			/* This value isn't used in C code.
					 * It may be used in TCL bindings
					 * to associate extra data. */
    Tk_Cursor cursor;
    int inset;				/* Sum of focus highlight and 3-D
					 * border.  Indicates how far to
					 * offset the slider from outside
					 * edge of the window. */
    int borderWidth;			/* Width of the exterior border */
    int relief;				/* Relief of the exterior border. */
    Blt_Bg normalBg;			/* 3-D exterior border around the
					 * outside of the widget. */

    int timeScale;			/* If non-zero, generate time scale
					 * ticks for the axis. This option
					 * is overridden by -logscale. */
    int decreasing;			/* If non-zero, display the range
					 * of values on the axis in
					 * descending order, from high to
					 * low. */
    const char *title;			/* Title of the axis. */
    Point2d titlePos;			/* Position of the title */
    int lineWidth;			/* Width of lines representing axis
					 * (including ticks).  If zero,
					 * then no axis lines or ticks are
					 * drawn. */
    double windowSize;			/* Size of a sliding window of
					 * values used to scale the axis
					 * automatically as new data values
					 * are added. The axis will always
					 * display the latest values in
					 * this range. */
    double shiftBy;			/* Shift maximum by this interval. */
    int tickLength;			/* Length of major ticks in pixels */
    Tcl_Obj *fmtCmdObjPtr;		/* Specifies a TCL command, to be
					 * invoked by the axis whenever it
					 * has to generate tick labels. */
    Tcl_Obj *scrollCmdObjPtr;
    int scrollUnits;

    /* "From" and "to" values. */
    double fromValue, toValue;		/* The requested limits of the
					 * slider. */

    /* Minimum and maximum slider values. */
    double minValue, maxValue;		/* The inner range of the
					 * slider. This is the user-defined
					 * range of values. In single point
					 * mode, the innerMax equals the
					 * innerMin. */
    char *maxValueString;		/* Malloc-ed. Formatted text
					 * representation of maximum value
					 * in range. */
    short int maxValWidth, maxValHeight;/* Extents of maximum value
					 * formatted text string.*/
    char *minValueString;		/* Malloc-ed. Formatted text
					 * representation of minimum value
					 * in range. */
    short int minValWidth, minValHeight;/* Extents of minimum value
					 * formatted text string. */
    /* Title */
    const char *title;			/* Title of the axis. */
    short int titleWidth, titleHeight;	/* Extents of title string. */
    Blt_Font titleFont;
    Tk_Justify titleJustify;
    XColor *titleFg;

    
    short int screenMin, screenRange;

    double reqScrollMin, reqScrollMax;

    double scrollMin, scrollMax;	/* Defines the scrolling reqion of
					 * the axis.  Normally the region
					 * is determined from the data
					 * limits. If specified, these
					 * values override the
					 * data-range. */

    SliderRange valueRange;		/* Range of data values of elements
					 * mapped to this axis. This is
					 * used to auto-scale the axis in
					 * "tight" mode. */
    SliderRange outerRange;		/* Smallest and largest major tick
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
    Ticks *t1Ptr;			/* Array of major tick
					 * positions. May be set by the
					 * user or generated from the major
					 * sweep below. */
    Ticks *t2Ptr;			/* Array of minor tick
					 * positions. May be set by the
					 * user or generated from the minor
					 * sweep below. */

    TickSweep minor, major;

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

    /*
     * Focus highlight ring
     */
    int highlightWidth;			/* Width in pixels of highlight to
					 * draw around widget when it has
					 * the focus.  <= 0 means don't
					 * draw a highlight. */
    XColor *highlightBg;		/* Color for drawing traversal
					 * highlight area when highlight is
					 * off. */
    XColor *highlightColor;		/* Color for drawing traversal
					 * highlight. */

    GC highlightGC;			/* GC for focus highlight. */


    /* The following fields are specific to logical axes */
    Blt_Chain chain;
    Segment2d *segments;		/* Array of line segments
					 * representing the major and minor
					 * ticks, but also the axis line
					 * itself. The segment coordinates
					 * are relative to the axis. */
    int numSegments;			/* # of segments in the above
					 * array. */
    Blt_Chain tickLabels;		/* Contains major tick label
					 * strings and their offsets along
					 * the axis. */
    short int axisLeft, axisRight;
    short int axisTop, axisBottom;	/* Region occupied by the of
					 * axis. */
    short int width, height;		/* Extents of axis */
    short int leftTickLabelWidth;	/* Maximum width of all ticks
					 * labels. */
    short int rightTickLabelWidth;	/* Maximum width of all ticks
					 * labels. */
    Blt_Bg normalBg;
    Blt_Bg activeBg;
    XColor *activeFg;

    int relief;
    int borderWidth;
    int activeRelief;

    float tickAngle;	
    Blt_Font tickFont;
    Tk_Anchor tickAnchor;
    XColor *tickFg;
    GC tickGC;				/* Graphics context for axis and
					 * tick labels */
    GC activeTickGC;

    double titleAngle;	
    double screenScale;
    Blt_Palette palette;
    float weight;
} Slider;

static Blt_OptionParseProc ObjToStateProc;
static Blt_OptionPrintProc StateToObjProc;
static Blt_CustomOption stateOption = {
    ObjToStateProc, StateToObjProc, NULL, (ClientData)0
};
static Blt_OptionFreeProc  FreeTicksProc;
static Blt_OptionParseProc ObjToTicksProc;
static Blt_OptionPrintProc TicksToObjProc;
static Blt_CustomOption majorTicksOption = {
    ObjToTicksProc, TicksToObjProc, FreeTicksProc, (ClientData)AUTO_MAJOR,
};
static Blt_CustomOption minorTicksOption = {
    ObjToTicksProc, TicksToObjProc, FreeTicksProc, (ClientData)AUTO_MINOR,
};

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
	"ActiveBackground", DEF_ACTIVEBACKGROUND, Blt_Offset(Slider, activeBg),
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground",
	"ActiveForeground", DEF_ACTIVEFOREGROUND, 
	Blt_Offset(Slider, activeFg), 0}, 
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "Relief",
	DEF_ACTIVERELIEF, Blt_Offset(Slider, activeRelief),
	BLT_CONFIG_DONT_SET_DEFAULT}, 
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_BACKGROUND, Blt_Offset(Slider, normalBg), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_LIST, "-bindtags", "bindTags", "BindTags", DEF_TAGS, 
	Blt_Offset(Slider, obj.tags), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BORDERWIDTH, Blt_Offset(Slider, borderWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor", (char *)NULL, 
	Blt_Offset(Slider, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_OBJ, "-formatcommand", "formatCommand", "FormatCommand", 
	DEF_COMMAND, Blt_Offset(Slider, fmtCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-fg", "color", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_FONT, "-font", "font", "Font", DEF_FONT, 
	Blt_Offset(Slider, font), 0},
    {BLT_CONFIG_SYNONYM, "-foreground", "color", (char *)NULL, (char *)NULL, 
	0, 0},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify", DEF_JUSTIFY, 
	Blt_Offset(Slider, titleJustify), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-labeloffset", "labelOffset", "LabelOffset",
	(char *)NULL, Blt_Offset(Slider, labelOffset), 0}, 
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth",
	DEF_LINEWIDTH, Blt_Offset(Slider, lineWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-logscale", "logScale", "LogScale",
	DEF_LOGSCALE, Blt_Offset(Slider, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	LOGSCALE},
    {BLT_CONFIG_CUSTOM, "-majorticks", "majorTicks", "MajorTicks",
	DEF_MAJOR_TICKS, Blt_Offset(Slider, t1Ptr), BLT_CONFIG_NULL_OK, 
	&majorTicksOption},
    {BLT_CONFIG_CUSTOM, "-mode", "mode", "Mode", DEF_MODE, 
	Blt_Offset(Slider, flags), BLT_CONFIG_DONT_SET_DEFAULT, &modeOption},
    {BLT_CONFIG_DOUBLE, "-max", "max", "Max", DEF_MIN, 
	Blt_Offset(Slider, maxValue), 0},
    {BLT_CONFIG_DOUBLE, "-min", "min", "Min", DEF_MAX, 
	Blt_Offset(Slider, minValue), 0},
    {BLT_CONFIG_DOUBLE, "-to", "to", "To", DEF_TO, 
	Blt_Offset(Slider, toValue), 0},
    {BLT_CONFIG_DOUBLE, "-from", "from", "From", DEF_FROM, 
	Blt_Offset(Slider, fromValue), 0},
    {BLT_CONFIG_OBJ, "-variable", "variable", "Variable",
	DEF_VARIABLE, Blt_Offset(Slider, varNameObjPtr), 0},
    {BLT_CONFIG_BACKGROUND, "-troughcolor", "troughColor", "Background",
	DEF_TROUGH_COLOR, Blt_Offset(Slider, troughBg), 0},
    {BLT_CONFIG_STATE, "-state", "state", "State", DEF_COLUMN_STATE, 
	Blt_Offset(Column, state), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-state", "state", "State", DEF_STATE, 
	Blt_Offset(ComboButton, flags), BLT_CONFIG_DONT_SET_DEFAULT, 
	&stateOption},

    {BLT_CONFIG_SIDE, "-side", "side", "side", DEF_SIDE, 
	Blt_Offset(Slider, side), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_WIDTH, 
	Blt_Offset(Slider, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "height", DEF_HEIGHT, 
	Blt_Offset(Slider, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_TAKE_FOCUS, Blt_Offset(Slider, takeFocus), BLT_CONFIG_NULL_OK},

    {BLT_CONFIG_FONT, "-tickfont", "tickFont", "TickFont", DEF_TICK_FONT, 
	Blt_Offset(Slider, tickFont), DEF_CONFIG_NULL_OK},
    {BLT_CONFIG_FONT, "-valueFont", "valueFont", "ValueFont", DEF_VALUE_FONT, 
	Blt_Offset(Slider, font), DEF_CONFIG_NULL_OK},
    {BLT_CONFIG_FONT, "-titlefont", "titleFont", "Font", DEF_TITLE_FONT, 
	Blt_Offset(Slider, titleFont), DEF_CONFIG_NULL_OK},

    {BLT_CONFIG_BITMASK, "-showticks", "showTicks", "ShowTicks",
	DEF_SHOW_TICKS, Blt_Offset(Slider, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)SHOWTICKS},
    {BLT_CONFIG_BITMASK, "-showvalues", "showValues", "ShowValues",
	DEF_SHOW_VALUES, Blt_Offset(Slider, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)SHOWVALUES},
    {BLT_CONFIG_BITMASK, "-showaxis", "showAxis", "ShowAxis",
	DEF_SHOW_AXIS, Blt_Offset(Slider, flags), BLT_CONFIG_DONT_SET_DEFAULT,
	(Blt_CustomOption *)SHOWAXIS},

    {BLT_CONFIG_OBJ, "-sliderlength", "sliderLength", "SliderLength",
	DEF_SLIDER_LENGTH, Blt_Offset(Slider, sliderLength),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_OPTION_RELIEF, "-sliderrelief", "sliderRelief", "SliderRelief",
	DEF_SLIDER_RELIEF, -1, Blt_Offset(Slider, sliderRelief),
	BLT_CONFIG_DONT_SET_DEFAULT},

    {BLT_CONFIG_COLOR, "-tickcolor", "tickColor", "TickColor",
	DEF_FOREGROUND, Blt_Offset(Slider, tickFg), 0},
    {BLT_CONFIG_DOUBLE, "-tickinterval", "tickInterval", "TickInterval",
	DEF_TICK_INTERVAL, Blt_Offset(Slider, tickInterval),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-orient", "orient", "Orient",
	DEF_SCALE_ORIENT, Tk_Offset(Slider, orient),
	BLT_CONFIG_DONT_SET_DEFAULT, &orientOption},
    {BLT_CONFIG_OBJ, "-label", "label", "Label", DEF_LABEL, 
	Blt_Offset(Slider, labelObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_HIGHLIGHT_BG, 
	Blt_Offset(Slider, highlightBgColor), 0},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_FOCUS_HIGHLIGHT_COLOR, Blt_Offset(TableView, highlightFg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_HIGHLIGHT_THICKNESS, 
	Blt_Offset(Slider, highlightWidth), BLT_CONFIG_DONT_SET_DEFAULT},

    {TK_OPTION_FONT, "-font", "font", "Font",
	DEF_SCALE_FONT, -1, Tk_Offset(TkScale, tkfont), 0, 0, 0},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	DEF_SCALE_FG_COLOR, -1, Tk_Offset(TkScale, textColorPtr), 0,
	(ClientData) DEF_SCALE_FG_MONO, 0},
    {TK_OPTION_INT, "-digits", "digits", "Digits",
	DEF_SCALE_DIGITS, -1, Tk_Offset(TkScale, digits),
	0, 0, 0},
    {TK_OPTION_BORDER, "-activebackground", "activeBackground", "Foreground",
	DEF_SCALE_ACTIVE_BG_COLOR, -1, Tk_Offset(TkScale, activeBorder),
	0, (ClientData) DEF_SCALE_ACTIVE_BG_MONO, 0},
    {TK_OPTION_BORDER, "-background", "background", "Background",
	DEF_SCALE_BG_COLOR, -1, Tk_Offset(TkScale, bgBorder),
	0, (ClientData) DEF_SCALE_BG_MONO, 0},
    {TK_OPTION_DOUBLE, "-bigincrement", "bigIncrement", "BigIncrement",
	DEF_SCALE_BIG_INCREMENT, -1, Tk_Offset(TkScale, bigIncrement),
	0, 0, 0},

    {BLT_CONFIG_CUSTOM, "-minorticks", "minorTicks", "MinorTicks",
	(char *)NULL, Blt_Offset(Slider, t2Ptr), BLT_CONFIG_NULL_OK, 
	&minorTicksOption},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_RELIEF, Blt_Offset(Slider, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FLOAT, "-rotate", "rotate", "Rotate", DEF_ANGLE, 
	Blt_Offset(Slider, tickAngle), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-scrollcommand", "scrollCommand", "ScrollCommand",
	(char *)NULL, Blt_Offset(Slider, scrollCmdObjPtr),
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-scrollincrement", "scrollIncrement", 
	"ScrollIncrement", DEF_SCROLL_INCREMENT, 
	Blt_Offset(Slider, scrollUnits), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-stepsize", "stepSize", "StepSize",
	DEF_STEP, Blt_Offset(Slider, reqStep),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_INT, "-subdivisions", "subdivisions", "Subdivisions",
	DEF_SUBDIVISIONS, Blt_Offset(Slider, reqNumMinorTicks), 0},
    {BLT_CONFIG_FONT, "-tickfont", "tickFont", "Font",
	DEF_TICKFONT, Blt_Offset(Slider, tickFont), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-ticklength", "tickLength", "TickLength",
	DEF_TICKLENGTH, Blt_Offset(Slider, tickLength), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_INT, "-tickdefault", "tickDefault", "TickDefault",
	DEF_DIVISIONS, Blt_Offset(Slider, reqNumMajorTicks),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_STRING, "-title", "title", "Title",
	(char *)NULL, Blt_Offset(Slider, title),
	BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-titlecolor", "titleColor", "Color", 
	DEF_FOREGROUND, Blt_Offset(Slider, titleFg), 0},
};

/* Forward declarations */
static void DestroySlider(Axis *axisPtr);
static Tk_EventProc SliderEventProc;
static Tcl_FreeProc FreeSlider;

static void TimeAxis(Axis *axisPtr, double min, double max);
static Tcl_CmdDeleteProc SliderInstDeletedProc;

static void
FreeAxis(DestroyData data)
{
    Blt_Free(data);
}

INLINE static double
Clamp(double x) 
{
    return (x < 0.0) ? 0.0 : (x > 1.0) ? 1.0 : x;
}

INLINE static int
Round(double x)
{
    return (int) (x + ((x < 0.0) ? -0.5 : 0.5));
}

static void
SetSliderRange(AxisRange *rangePtr, double min, double max)
{
    rangePtr->min = min;
    rangePtr->max = max;
    rangePtr->range = max - min;
    if (FABS(rangePtr->range) < DBL_EPSILON) {
	rangePtr->range = 1.0;
    }
    rangePtr->scale = 1.0 / rangePtr->range;
}

/*
 *---------------------------------------------------------------------------
 *
 * InRange --
 *
 *	Determines if a value lies within a given range.
 *
 *	The value is normalized and compared against the interval [0..1],
 *	where 0.0 is the minimum and 1.0 is the maximum.  DBL_EPSILON is
 *	the smallest number that can be represented on the host machine,
 *	such that (1.0 + epsilon) != 1.0.
 *
 *	Please note, *max* can't equal *min*.
 *
 * Results:
 *	If the value is within the interval [min..max], 1 is returned; 0
 *	otherwise.
 *
 *---------------------------------------------------------------------------
 */
INLINE static int
InRange(double x, SliderRange *rangePtr)
{
    if (rangePtr->range < DBL_EPSILON) {
	return (FABS(rangePtr->max - x) >= DBL_EPSILON);
    } else {
	double norm;

	norm = (x - rangePtr->min) * rangePtr->scale;
	return ((norm > -DBL_EPSILON) && ((norm - 1.0) <= DBL_EPSILON));
    }
}

/*
 *-----------------------------------------------------------------------------
 * Custom option parse and print procedures
 *-----------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 *
 * ObjToStateProc --
 *
 *	Converts the string representing a state into a bitflag.
 *
 * Results:
 *	The return value is a standard TCL result.  The state flags are
 *	updated.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToStateProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to report results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing state. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Slider *sliderPtr = (Slider *)(widgRec);
    unsigned int *flagsPtr = (unsigned int *)(widgRec + offset);
    char *string;
    int flag;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'd') && (strcmp(string, "disabled") == 0)) {
	flag = STATE_DISABLED;
    } else if ((c == 'n') && (strcmp(string, "normal") == 0)) {
	flag = STATE_NORMAL;
    } else if (c == 'a') && (strcmp(string, "active") == 0)) {
	flag = STATE_ACTIVE;
    } else {
	Tcl_AppendResult(interp, "unknown state \"", string, 
	    "\": should be active, disabled, or normal.", (char *)NULL);
	return TCL_ERROR;
    }
    if (sliderPtr->flags & flag) {
	return TCL_OK;			/* State is already set to value. */
    }
    *flagsPtr &= ~STATE_MASK;
    *flagsPtr |= flag;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StateToObjProc --
 *
 *	Return the name of the style.
 *
 * Results:
 *	The name representing the style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
StateToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* Widget information record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    unsigned int state = *(unsigned int *)(widgRec + offset);
    const char *s;

    switch (state & STATE_MASK) {
    case STATE_NORMAL:		s = "normal";	break;
    case STATE_ACTIVE:		s = "active";	break;
    case STATE_DISABLED:	s = "disabled";	break;
    default:			s = Blt_Itoa(state & STATE_MASK);  break;
    }
    return Tcl_NewStringObj(s, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToHighProc --
 *
 *	Convert the string representation of an axis limit into its numeric
 *	form.
 *
 * Results:
 *	The return value is a standard TCL result.  The symbol type is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToHighProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing new value. */
    char *widgRec,			/* Pointer to structure record. */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Slider *sliderPtr = (Slider *)widgRec;
    double value;

    if (Blt_ExprDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
	return TCL_ERROR;
    }
    if (value < sliderPtr->innerMin) {
	value = sliderPtr->innerMin;
    }
    if (value > sliderPtr->outerMax) {
	value = sliderPtr->outerMax;
    }
    sliderPtr->innerMax = value;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLowProc --
 *
 *	Convert the string representation of an axis limit into its numeric
 *	form.
 *
 * Results:
 *	The return value is a standard TCL result.  The symbol type is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToLowProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing new value. */
    char *widgRec,			/* Pointer to structure record. */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Slider *sliderPtr = (Slider *)widgRec;
    double value;

    if (Blt_ExprDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
	return TCL_ERROR;
    }
    if (value > sliderPtr->innerMax) {
	value = sliderPtr->innerMax;
    }
    if (value < sliderPtr->outerMin) {
	value = sliderPtr->outerMin;
    }
    sliderPtr->innerMin = value;
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToHighProc --
 *
 *	Convert the string representation of an axis limit into its numeric
 *	form.
 *
 * Results:
 *	The return value is a standard TCL result.  The symbol type is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToHighProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing new value. */
    char *widgRec,			/* Pointer to structure record. */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Slider *sliderPtr = (Slider *)widgRec;
    double value;

    if (Blt_ExprDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
	return TCL_ERROR;
    }
    if (value < sliderPtr->innerMin) {
	value = sliderPtr->innerMin;
    }
    if (value > sliderPtr->outerMax) {
	value = sliderPtr->outerMax;
    }
    sliderPtr->innerMax = value;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLowProc --
 *
 *	Convert the string representation of an axis limit into its numeric
 *	form.
 *
 * Results:
 *	The return value is a standard TCL result.  The symbol type is
 *	written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToHighProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing new value. */
    char *widgRec,			/* Pointer to structure record. */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Slider *sliderPtr = (Slider *)widgRec;
    double value;

    if (Blt_ExprDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
	return TCL_ERROR;
    }
    if (value > sliderPtr->innerMax) {
	value = sliderPtr->innerMax;
    }
    if (value < sliderPtr->outerMin) {
	value = sliderPtr->outerMin;
    }
    sliderPtr->innerMin = value;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LimitToObjProc --
 *
 *	Convert the floating point axis limits into a string.
 *
 * Results:
 *	The string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LimitToObjProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Not used. */
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    double limit = *(double *)(widgRec + offset);
    Tcl_Obj *objPtr;

    if (DEFINED(limit)) {
	objPtr = Tcl_NewDoubleObj(limit);
    } else {
	objPtr = Tcl_NewStringObj("", -1);
    }
    return objPtr;
}

/*ARGSUSED*/
static void
FreeTicksProc(
    ClientData clientData,		/* Either AUTO_MAJOR or AUTO_MINOR. */
    Display *display,			/* Not used. */
    char *widgRec,
    int offset)
{
    Axis *axisPtr = (Axis *)widgRec;
    Ticks **ticksPtrPtr = (Ticks **) (widgRec + offset);
    unsigned long mask = (unsigned long)clientData;

    axisPtr->flags |= mask;
    if (*ticksPtrPtr != NULL) {
	Blt_Free(*ticksPtrPtr);
    }
    *ticksPtrPtr = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTicksProc --
 *
 *
 * Results:
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTicksProc(
    ClientData clientData,		/* Either AUTO_MAJOR or AUTO_MINOR. */
    Tcl_Interp *interp,		        /* Interpreter to send results. */
    Tk_Window tkwin,			/* Not used. */
    Tcl_Obj *objPtr,			/* String representing new value. */
    char *widgRec,			/* Pointer to structure record. */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Axis *axisPtr = (Axis *)widgRec;
    Tcl_Obj **objv;
    Ticks **ticksPtrPtr = (Ticks **) (widgRec + offset);
    Ticks *ticksPtr;
    int objc;
    unsigned long mask = (unsigned long)clientData;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR;
    }
    axisPtr->flags |= mask;
    ticksPtr = NULL;
    if (objc > 0) {
	int i;

	ticksPtr = Blt_AssertMalloc(sizeof(Ticks) + (objc*sizeof(double)));
	for (i = 0; i < objc; i++) {
	    double value;

	    if (Blt_ExprDoubleFromObj(interp, objv[i], &value) != TCL_OK) {
		Blt_Free(ticksPtr);
		return TCL_ERROR;
	    }
	    ticksPtr->values[i] = value;
	}
	ticksPtr->numTicks = objc;
	axisPtr->flags &= ~mask;
    }
    FreeTicksProc(clientData, Tk_Display(tkwin), widgRec, offset);
    *ticksPtrPtr = ticksPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TicksToObjProc --
 *
 *	Convert array of tick coordinates to a list.
 *
 * Results:
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TicksToObjProc(
    ClientData clientData,		/* Either AUTO_MAJOR or AUTO_MINOR. */
    Tcl_Interp *interp,
    Tk_Window tkwin,			/* Not used. */
    char *widgRec,			/* */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Axis *axisPtr;
    Tcl_Obj *listObjPtr;
    Ticks *ticksPtr;
    unsigned long mask;

    axisPtr = (Axis *)widgRec;
    ticksPtr = *(Ticks **) (widgRec + offset);
    mask = (unsigned long)clientData;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if ((ticksPtr != NULL) && ((axisPtr->flags & mask) == 0)) {
	int i;

	for (i = 0; i < ticksPtr->numTicks; i++) {
	    Tcl_Obj *objPtr;

	    objPtr = Tcl_NewDoubleObj(ticksPtr->values[i]);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *	Tells the Tk dispatcher to call the slider display routine at the
 *	next idle point.  This request is made only if the window is
 *	displayed and no other redraw request is pending.
 *
 * Results: None.
 *
 * Side effects:
 *	The window is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyRedraw(Slider *sliderPtr) 
{
    if ((sliderPtr->tkwin != NULL) && !(sliderPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayProc, sliderPtr);
	sliderPtr->flags |= REDRAW_PENDING;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SliderEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various events
 *	on sliders.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get cleaned up.
 *	When it gets exposed, the slider is eventually redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
SliderEventProc(ClientData clientData, XEvent *eventPtr)
{
    Slider *sliderPtr = clientData;

    if (eventPtr->type == Expose) {
	if (eventPtr->xexpose.count == 0) {
	    sliderPtr->flags |= REDRAW_WORLD;
	    EventuallyRedraw(sliderPtr);
	}
    } else if ((eventPtr->type == FocusIn) || (eventPtr->type == FocusOut)) {
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    if (eventPtr->type == FocusIn) {
		sliderPtr->flags |= FOCUS;
	    } else {
		sliderPtr->flags &= ~FOCUS;
	    }
	    sliderPtr->flags |= REDRAW_WORLD;
	    EventuallyRedraw(sliderPtr);
	}
    } else if (eventPtr->type == DestroyNotify) {
	if (sliderPtr->tkwin != NULL) {
	    Blt_DeleteWindowInstanceData(sliderPtr->tkwin);
	    sliderPtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(sliderPtr->interp, sliderPtr->cmdToken);
	}
	if (sliderPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayProc, sliderPtr);
	}
	Tcl_EventuallyFree(sliderPtr, DestroySlider);
    } else if (eventPtr->type == ConfigureNotify) {
	sliderPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
	EventuallyRedraw(sliderPtr);
    }
}

static void
FreeTickLabels(Blt_Chain chain)
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(chain); link != NULL; 
	 link = Blt_Chain_NextLink(link)) {
	TickLabel *labelPtr;

	labelPtr = Blt_Chain_GetValue(link);
	Blt_Free(labelPtr);
    }
    Blt_Chain_Reset(chain);
}

/*
 *---------------------------------------------------------------------------
 *
 * MakeLabel --
 *
 *	Converts a floating point tick value to a string to be used as its
 *	label.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Returns a new label in the string character buffer.  The formatted
 *	tick label will be displayed on the slider.
 *
 * -------------------------------------------------------------------------- 
 */
static TickLabel *
MakeLabel(Axis *axisPtr, double value)
{
#define TICK_LABEL_SIZE		200
    char string[TICK_LABEL_SIZE + 1];
    TickLabel *labelPtr;

    /* Generate a default tick label based upon the tick value.  */
    if (IsLogScale(axisPtr)) {
	Blt_FormatString(string, TICK_LABEL_SIZE, "1E%d", ROUND(value));
    } else {
	Blt_FormatString(string, TICK_LABEL_SIZE, "%.*G", NUMDIGITS, value);
    }

    if (axisPtr->fmtCmdObjPtr != NULL) {
	Tcl_Interp *interp;
	Tcl_Obj *cmdObjPtr, *objPtr;
	Tk_Window tkwin;
	int result;

	interp = axisPtr->interp;
	tkwin = axisPtr->tkwin;
	/*
	 * A TCL proc was designated to format tick labels. Append the path
	 * name of the widget and the default tick label as arguments when
	 * invoking it. Copy and save the new label from interp->result.
	 */
	cmdObjPtr = Tcl_DuplicateObj(axisPtr->fmtCmdObjPtr);
	objPtr = Tcl_NewStringObj(Tk_PathName(tkwin), -1);
	Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	objPtr = Tcl_NewStringObj(string, -1);
	Tcl_ResetResult(interp);
	Tcl_IncrRefCount(cmdObjPtr);
	Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
	result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(cmdObjPtr);
	if (result != TCL_OK) {
	    Tcl_BackgroundError(interp);
	} else {
	    /* 
	     * The proc could return a string of any length, so arbitrarily
	     * limit it to what will fit in the return string.
	     */
	    strncpy(string, Tcl_GetStringResult(interp), TICK_LABEL_SIZE);
	    string[TICK_LABEL_SIZE] = '\0';
	    
	    Tcl_ResetResult(interp); /* Clear the interpreter's result. */
	}
    }
    labelPtr = Blt_AssertMalloc(sizeof(TickLabel) + strlen(string));
    strcpy(labelPtr->string, string);
    labelPtr->anchorPos.x = labelPtr->anchorPos.y = DBL_MAX;
    return labelPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * InvHMap --
 *
 *	Maps the given screen coordinate back to a slider coordinate.
 *	Called by the slider locater routine.
 *
 * Results:
 *	Returns the slider coordinate value at the given window
 *	y-coordinate.
 *
 *---------------------------------------------------------------------------
 */
static double
InvHMap(Slider *sliderPtr, double x)
{
    double value;

    x = (double)(x - sliderPtr->screenMin) * sliderPtr->screenScale;
    if (sliderPtr->flags & DECREASING) {
	x = 1.0 - x;
    }
    value = (x * sliderPtr->outerRange.range) + sliderPtr->outerRange.min;
    if (sliderPtr->flags & LOGSCALE) {
	value = EXP10(value);
    }
    return value;
}

/*
 *---------------------------------------------------------------------------
 *
 * InvVMap --
 *
 *	Maps the given screen y-coordinate back to the slider coordinate
 *	value. Called by the slider locater routine.
 *
 * Results:
 *	Returns the slider coordinate value for the given screen
 *	coordinate.
 *
 *---------------------------------------------------------------------------
 */
static double
InvVMap(Slider *sliderPtr, double y)	/* Screen coordinate */
{
    double value;

    y = (double)(y - sliderPtr->screenMin) * sliderPtr->screenScale;
    if (sliderPtr->flags & DECREASING) {
	y = 1.0 - y;
    }
    value = ((1.0 - y) * sliderPtr->outerRange.range) + 
	sliderPtr->outerRange.min;
    if (sliderPtr->flags & LOGSCALE) {
	value = EXP10(value);
    }
    return value;
}

/*
 *---------------------------------------------------------------------------
 *
 * HMap --
 *
 *	Map the given slider coordinate value to its axis, returning a window
 *	position.
 *
 * Results:
 *	Returns a double precision number representing the window coordinate
 *	position on the given axis.
 *
 *---------------------------------------------------------------------------
 */
static double
HMap(Slider *sliderPtr, double x)
{
    if ((sliderPtr->flags & LOGSCALE) && (x != 0.0)) {
	x = log10(FABS(x));
    }
    /* Map coordinate to normalized coordinates [0..1] */
    x = (x - sliderPtr->outerRange.min) * sliderPtr->outerRange.scale;
    if (sliderPtr->flags & DECREASING) {
	x = 1.0 - x;
    }
    return (x * sliderPtr->screenRange + sliderPtr->screenMin);
}

/*
 *---------------------------------------------------------------------------
 *
 * VMap --
 *
 *	Map the given slider coordinate value to its axis, returning a
 *	window position.
 *
 * Results:
 *	Returns a double precision number representing the window
 *	coordinate position on the given axis.
 *
 *---------------------------------------------------------------------------
 */
static double
VMap(Slider *sliderPtr, double y)
{
    if ((sliderPtr->flags & LOGSCALE) && (y != 0.0)) {
	y = log10(FABS(y));
    }
    /* Map coordinate to normalized coordinates [0..1] */
    y = (y - sliderPtr->outerRange.min) * sliderPtr->outerRange.scale;
    if (sliderPtr->flags & DECREASING) {
	y = 1.0 - y;
    }
    return ((1.0 - y) * sliderPtr->screenRange + sliderPtr->screenMin);
}

static void
FixAxisRanges(Slider *sliderPtr)
{
    double min, max;

    if (sliderPtr->outerMin > sliderPtr->outerMax) {
	sliderPtr->flags |= DECREASING;
	max = sliderPtr->outerMin;
	min = sliderPtr->outerMax;
    } else {
	sliderPtr->flags &= ~DECREASING;
	min = sliderPtr->outerMin;
	max = sliderPtr->outerMax;
    }
    if ((sliderPtr->flags & LOGSCALE) && (min < 0.0)) {
	min = 0.001;
    }
    if (sliderPtr->innerMin < min) {
	sliderPtr->innerMin = min;
    }
    if (sliderPtr->innerMax > max) {
	sliderPtr->innerMax = max;
    }
    if (sliderPtr->innerMin > sliderPtr->innerMax) {
	sliderPtr->innerMax = sliderPtr->innerMin;
    }
    if (sliderPtr->innerMax < sliderPtr->innerMin) {
	sliderPtr->innerMin = sliderPtr->innerMax;
    }
    SetSliderRange(&sliderPtr->outerRange, min, max);
}

/*
 *---------------------------------------------------------------------------
 *
 * NiceNum --
 *
 *	Reference: Paul Heckbert, "Nice Numbers for Graph Labels",
 *		   Graphics Gems, pp 61-63.  
 *
 *	Finds a "nice" number approximately equal to x.
 *
 *---------------------------------------------------------------------------
 */
static double
NiceNum(double x, int round)		/* If non-zero, round. Otherwise
					 * take ceiling of value. */
{
    double expt;			/* Exponent of x */
    double frac;			/* Fractional part of x */
    double nice;			/* Nice, rounded fraction */

    expt = floor(log10(x));
    frac = x / EXP10(expt);		/* between 1 and 10 */
    if (round) {
	if (frac < 1.5) {
	    nice = 1.0;
	} else if (frac < 3.0) {
	    nice = 2.0;
	} else if (frac < 7.0) {
	    nice = 5.0;
	} else {
	    nice = 10.0;
	}
    } else {
	if (frac <= 1.0) {
	    nice = 1.0;
	} else if (frac <= 2.0) {
	    nice = 2.0;
	} else if (frac <= 5.0) {
	    nice = 5.0;
	} else {
	    nice = 10.0;
	}
    }
    return nice * EXP10(expt);
}

static Ticks *
GenerateTicks(TickSweep *sweepPtr)
{
    Ticks *ticksPtr;

    ticksPtr = Blt_AssertMalloc(sizeof(Ticks) + 
	(sweepPtr->numSteps * sizeof(double)));
    ticksPtr->numTicks = 0;

    if (sweepPtr->step == 0.0) { 
	/* Hack: A zero step indicates to use log values. */
	int i;
	/* Precomputed log10 values [1..10] */
	static double logTable[] = {
	    0.0, 
	    0.301029995663981, 
	    0.477121254719662, 
	    0.602059991327962, 
	    0.698970004336019, 
	    0.778151250383644, 
	    0.845098040014257,
	    0.903089986991944, 
	    0.954242509439325, 
	    1.0
	};
	for (i = 0; i < sweepPtr->numSteps; i++) {
	    ticksPtr->values[i] = logTable[i];
	}
    } else {
	double value;
	int i;
    
	value = sweepPtr->initial;	/* Start from smallest axis tick */
	for (i = 0; i < sweepPtr->numSteps; i++) {
	    value = UROUND(value, sweepPtr->step);
	    ticksPtr->values[i] = value;
	    value += sweepPtr->step;
	}
    }
    ticksPtr->numTicks = sweepPtr->numSteps;
    return ticksPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * LogAxis --
 *
 * 	Determine the range and units of a log scaled axis.
 *
 * 	Unless the axis limits are specified, the axis is scaled
 * 	automatically, where the smallest and largest major ticks encompass
 * 	the range of actual data values.  When an axis limit is specified,
 * 	that value represents the smallest(min)/largest(max) value in the
 * 	displayed range of values.
 *
 * 	Both manual and automatic scaling are affected by the step used.
 * 	By default, the step is the largest power of ten to divide the
 * 	range in more than one piece.
 *
 *	Automatic scaling:
 *	Find the smallest number of units which contain the range of
 *	values.  The minimum and maximum major tick values will be
 *	represent the range of values for the axis. This greatest number of
 *	major ticks possible is 10.
 *
 * 	Manual scaling:
 *   	Make the minimum and maximum data values the represent the range of
 *   	the values for the axis.  The minimum and maximum major ticks will
 *   	be inclusive of this range.  This provides the largest area for
 *   	plotting and the expected results when the axis min and max values
 *   	have be set by the user (.e.g zooming).  The maximum number of
 *   	major ticks is 20.
 *
 *   	For log scale, there's the possibility that the minimum and
 *   	maximum data values are the same magnitude.  To represent the
 *   	points properly, at least one full decade should be shown.
 *   	However, if you zoom a log scale plot, the results should be
 *   	predictable. Therefore, in that case, show only minor ticks.
 *   	Lastly, there should be an appropriate way to handle numbers
 *   	<=0.
 *
 *          maxY
 *            |    units = magnitude (of least significant digit)
 *            |    high  = largest unit tick < max axis value
 *      high _|    low   = smallest unit tick > min axis value
 *            |
 *            |    range = high - low
 *            |    # ticks = greatest factor of range/units
 *           _|
 *        U   |
 *        n   |
 *        i   |
 *        t  _|
 *            |
 *            |
 *            |
 *       low _|
 *            |
 *            |_minX________________maxX__
 *            |   |       |      |       |
 *     minY  low                        high
 *           minY
 *
 *
 * 	numTicks = Number of ticks
 * 	min = Minimum value of axis
 * 	max = Maximum value of axis
 * 	range    = Range of values (max - min)
 *
 * 	If the number of decades is greater than ten, it is assumed
 *	that the full set of log-style ticks can't be drawn properly.
 *
 * Results:
 *	None
 *
 * -------------------------------------------------------------------------- 
 */
static void
LogAxis(Slider *sliderPtr, double min, double max)
{
    double range;
    double tickMin, tickMax;
    double majorStep, minorStep;
    int numMajor, numMinor;

    numMajor = numMinor = 0;
    /* Suppress compiler warnings. */
    majorStep = minorStep = 0.0;
    tickMin = tickMax = Blt_NaN();
    if (min < max) {
	min = (min != 0.0) ? log10(FABS(min)) : 0.0;
	max = (max != 0.0) ? log10(FABS(max)) : 1.0;

	tickMin = floor(min);
	tickMax = ceil(max);
	range = tickMax - tickMin;
	
	if (range > 10) {
	    /* There are too many decades to display a major tick at every
	     * decade.  Instead, treat the axis as a linear scale. */
	    range = NiceNum(range, 0);
	    majorStep = NiceNum(range / sliderPtr->reqNumMajorTicks, 1);
	    tickMin = UFLOOR(tickMin, majorStep);
	    tickMax = UCEIL(tickMax, majorStep);
	    numMajor = (int)((tickMax - tickMin) / majorStep) + 1;
	    minorStep = EXP10(floor(log10(majorStep)));
	    if (minorStep == majorStep) {
		numMinor = 4, minorStep = 0.2;
	    } else {
		numMinor = Round(majorStep / minorStep) - 1;
	    }
	} else {
	    if (tickMin == tickMax) {
		tickMax++;
	    }
	    majorStep = 1.0;
	    numMajor = (int)(tickMax - tickMin + 1); /* FIXME: Check this. */
	    
	    minorStep = 0.0;		/* This is a special hack to pass
					 * information to the GenerateTicks
					 * routine. An interval of 0.0
					 * tells 1) this is a minor sweep
					 * and 2) the axis is log scale. */
	    numMinor = 10;
	}
	tickMin = min;
	numMajor++;
	tickMax = max;
    }
    sliderPtr->major.step = majorStep;
    sliderPtr->major.initial = floor(tickMin);
    sliderPtr->major.numSteps = numMajor;
    sliderPtr->minor.initial = sliderPtr->minor.step = minorStep;
    sliderPtr->minor.numSteps = numMinor;
    SetSliderRange(&sliderPtr->outerRange, tickMin, tickMax);
}

/*
 *---------------------------------------------------------------------------
 *
 * LinearAxis --
 *
 * 	Determine the units of a linear scaled axis.
 *
 *	The axis limits are either the range of the data values mapped to
 *	the axis (autoscaled), or the values specified by the -min and -max
 *	options (manual).
 *
 *	If autoscaled, the smallest and largest major ticks will encompass
 *	the range of data values.  If the -loose option is selected, the
 *	next outer ticks are choosen.  If tight, the ticks are at or inside
 *	of the data limits are used.
 *
 * 	If manually set, the ticks are at or inside the data limits are
 * 	used.  This makes sense for zooming.  You want the selected range
 * 	to represent the next limit, not something a bit bigger.
 *
 *	Note: I added an "always" value to the -loose option to force
 *	      the manually selected axes to be loose. It's probably
 *	      not a good idea.
 *
 *          maxY
 *            |    units = magnitude (of least significant digit)
 *            |    high  = largest unit tick < max axis value
 *      high _|    low   = smallest unit tick > min axis value
 *            |
 *            |    range = high - low
 *            |    # ticks = greatest factor of range/units
 *           _|
 *        U   |
 *        n   |
 *        i   |
 *        t  _|
 *            |
 *            |
 *            |
 *       low _|
 *            |
 *            |_minX________________maxX__
 *            |   |       |      |       |
 *     minY  low                        high
 *           minY
 *
 * 	numTicks = Number of ticks
 * 	min = Minimum value of axis
 * 	max = Maximum value of axis
 * 	range    = Range of values (max - min)
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The axis tick information is set.  The actual tick values will
 *	be generated later.
 *
 *---------------------------------------------------------------------------
 */
static void
LinearAxis(Slider *sliderPtr, double min, double max)
{
    double step;
    double tickMin, tickMax;
    double axisMin, axisMax;
    unsigned int numTicks;

    numTicks = 0;
    step = 1.0;
    /* Suppress compiler warning. */
    axisMin = axisMax = tickMin = tickMax = Blt_NaN();
    if (min < max) {
	double range;

	range = max - min;
	/* Calculate the major tick stepping. */
	if (sliderPtr->reqStep > 0.0) {
	    /* An interval was designated by the user.  Keep scaling it
	     * until it fits comfortably within the current range of the
	     * axis.  */
	    step = sliderPtr->reqStep;
	    while ((2 * step) >= range) {
		step *= 0.5;
	    }
	} else {
	    range = NiceNum(range, 0);
	    step = NiceNum(range / sliderPtr->reqNumMajorTicks, 1);
	}
	
	/* Find the outer tick values. Add 0.0 to prevent getting -0.0. */
	axisMin = tickMin = floor(min / step) * step + 0.0;
	axisMax = tickMax = ceil(max / step) * step + 0.0;
	
	numTicks = Round((tickMax - tickMin) / step) + 1;
    } 
    sliderPtr->major.step = step;
    sliderPtr->major.initial = tickMin;
    sliderPtr->major.numSteps = numTicks;

    axisMin = min;
    axisMax = max;
    SetAxisRange(&sliderPtr->outerRange, axisMin, axisMax);

    /* Now calculate the minor tick step and number. */

    if ((sliderPtr->reqNumMinorTicks > 0) && (sliderPtr->flags & AUTO_MAJOR)) {
	numTicks = sliderPtr->reqNumMinorTicks - 1;
	step = 1.0 / (numTicks + 1);
    } else {
	numTicks = 0;			/* No minor ticks. */
	step = 0.5;			/* Don't set the minor tick
					 * interval to 0.0. It makes the
					 * GenerateTicks routine * create
					 * minor log-scale tick marks.  */
    }
    sliderPtr->minor.initial = sliderPtr->minor.step = step;
    sliderPtr->minor.numSteps = numTicks;
}

static void
SweepTicks(Slider *sliderPtr)
{
    if (sliderPtr->flags & AUTO_MAJOR) {
	if (sliderPtr->t1Ptr != NULL) {
	    Blt_Free(sliderPtr->t1Ptr);
	}
	sliderPtr->t1Ptr = GenerateTicks(&sliderPtr->major);
    }
    if (sliderPtr->flags & AUTO_MINOR) {
	if (sliderPtr->t2Ptr != NULL) {
	    Blt_Free(sliderPtr->t2Ptr);
	}
	sliderPtr->t2Ptr = GenerateTicks(&sliderPtr->minor);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetAxes --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
ResetAxes(Slider *sliderPtr)
{
    double min, max;

    FixAxisRange(sliderPtr);

    /* Calculate min/max tick (major/minor) layouts */
    min = sliderPtr->outerMin;
    max = sliderPtr->outerMax;
    if (IsLogScale(sliderPtr)) {
	LogAxis(sliderPtr, min, max);
    } else if (IsTimeScale(sliderPtr)) {
	TimeAxis(sliderPtr, min, max);
    } else {
	LinearAxis(sliderPtr, min, max);
    }
    sliderPtr->flags |= (GEOMETRY | LAYOUT);
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetTextStyles --
 *
 *	Configures axis attributes (font, line width, label, etc) and
 *	allocates a new (possibly shared) graphics context.  Line cap style
 *	is projecting.  This is for the problem of when a tick sits
 *	directly at the end point of the axis.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 * Side Effects:
 *	Axis resources are allocated (GC, font). Axis layout is deferred
 *	until the height and width of the window are known.
 *
 *---------------------------------------------------------------------------
 */
static void
ResetTextStyles(Slider *sliderPtr)
{
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;

    gcMask = (GCForeground | GCLineWidth | GCCapStyle);
    gcValues.foreground = sliderPtr->tickFg->pixel;
    gcValues.font = Blt_Font_Id(sliderPtr->tickFont);
    gcValues.line_width = LineWidth(sliderPtr->lineWidth);
    gcValues.cap_style = CapProjecting;

    newGC = Tk_GetGC(sliderPtr->tkwin, gcMask, &gcValues);
    if (sliderPtr->tickGC != NULL) {
	Tk_FreeGC(sliderPtr->display, sliderPtr->tickGC);
    }
    sliderPtr->tickGC = newGC;

    /* Assuming settings from above GC */
    gcValues.foreground = sliderPtr->activeFg->pixel;
    newGC = Tk_GetGC(sliderPtr->tkwin, gcMask, &gcValues);
    if (sliderPtr->activeTickGC != NULL) {
	Tk_FreeGC(sliderPtr->display, sliderPtr->activeTickGC);
    }
    sliderPtr->activeTickGC = newGC;

    gcValues.background = gcValues.foreground = sliderPtr->major.color->pixel;
    gcValues.line_width = LineWidth(sliderPtr->major.lineWidth);
    gcMask = (GCForeground | GCBackground | GCLineWidth);
    if (LineIsDashed(sliderPtr->major.dashes)) {
	gcValues.line_style = LineOnOffDash;
	gcMask |= GCLineStyle;
    }
    newGC = Blt_GetPrivateGC(sliderPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(sliderPtr->major.dashes)) {
	Blt_SetDashes(sliderPtr->display, newGC, &sliderPtr->major.dashes);
    }
    if (sliderPtr->major.gc != NULL) {
	Blt_FreePrivateGC(sliderPtr->display, sliderPtr->major.gc);
    }
    sliderPtr->major.gc = newGC;

    gcValues.background = gcValues.foreground = sliderPtr->minor.color->pixel;
    gcValues.line_width = LineWidth(sliderPtr->minor.lineWidth);
    gcMask = (GCForeground | GCBackground | GCLineWidth);
    if (LineIsDashed(sliderPtr->minor.dashes)) {
	gcValues.line_style = LineOnOffDash;
	gcMask |= GCLineStyle;
    }
    newGC = Blt_GetPrivateGC(sliderPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(sliderPtr->minor.dashes)) {
	Blt_SetDashes(sliderPtr->display, newGC, &sliderPtr->minor.dashes);
    }
    if (sliderPtr->minor.gc != NULL) {
	Blt_FreePrivateGC(sliderPtr->display, sliderPtr->minor.gc);
    }
    sliderPtr->minor.gc = newGC;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroySlider --
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources (font, color, gc, labels, etc.) associated with the axis
 *	are deallocated.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroySlider(Slider *sliderPtr)
{
    int flags;

    Blt_FreeOptions(configSpecs, (char *)sliderPtr, sliderPtr->display, 0);
    if (sliderPtr->bindTable != NULL) {
	Blt_DeleteBindings(sliderPtr->bindTable, sliderPtr);
    }
    if (sliderPtr->tickGC != NULL) {
	Tk_FreeGC(sliderPtr->display, sliderPtr->tickGC);
    }
    if (sliderPtr->activeTickGC != NULL) {
	Tk_FreeGC(sliderPtr->display, sliderPtr->activeTickGC);
    }
    if (sliderPtr->major.gc != NULL) {
	Blt_FreePrivateGC(sliderPtr->display, sliderPtr->major.gc);
    }
    if (sliderPtr->minor.gc != NULL) {
	Blt_FreePrivateGC(sliderPtr->display, sliderPtr->minor.gc);
    }
    FreeTickLabels(sliderPtr->tickLabels);
    Blt_Chain_Destroy(sliderPtr->tickLabels);
    if (sliderPtr->segments != NULL) {
	Blt_Free(sliderPtr->segments);
    }
    Blt_Free(sliderPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisOffsets --
 *
 *	Determines the sites of the axis, major and minor ticks, and title
 *	of the axis.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
AxisOffsets(Axis *sliderPtr, Margin *marginPtr, int offset, AxisInfo *infoPtr)
{
    int pad;				/* Offset of axis from interior
					 * region. This includes a possible
					 * border and the axis line
					 * width. */
    int axisLine;
    int t1, t2, labelOffset;
    int tickLabel, axisPad;
    int inset, mark;
    int x, y;
    float fangle;

    sliderPtr->titleAngle = titleAngle[marginPtr->site];
    tickLabel = axisLine = t1 = t2 = 0;
    labelOffset = AXIS_PAD_TITLE;
    if (sliderPtr->lineWidth > 0) {
	if (sliderPtr->flags & SHOWTICKS) {
	    t1 = sliderPtr->tickLength;
	    t2 = (t1 * 10) / 15;
	}
	labelOffset = t1 + AXIS_PAD_TITLE;
	if (sliderPtr->flags & EXTERIOR) {
	    labelOffset += sliderPtr->lineWidth;
	}
    }
    axisPad = 0;
    /* Adjust offset for the interior border width and the line width */
    pad = 1;
    pad = 0;				/* FIXME: test */
    /*
     * Pre-calculate the x-coordinate positions of the axis, tick labels,
     * and the individual major and minor ticks.
     */
    inset = pad + sliderPtr->lineWidth / 2;
    switch (marginPtr->site) {
    case MARGIN_TOP:
	axisLine = slider->top;
	if (sliderPtr->flags & EXTERIOR) {
	    axisLine -= slider->plotBW + axisPad + sliderPtr->lineWidth / 2;
	    tickLabel = axisLine - 2;
	    if (sliderPtr->lineWidth > 0) {
		tickLabel -= sliderPtr->tickLength;
	    }
	} else {
	    if (slider->plotRelief == TK_RELIEF_SOLID) {
		axisLine--;
	    } 
	    axisLine -= axisPad + sliderPtr->lineWidth / 2;
	    tickLabel = sliderPtr->top -  sliderPtr->plotBW - 2;
	}
	mark = sliderPtr->top - offset - pad;
	sliderPtr->tickAnchor = TK_ANCHOR_S;
	sliderPtr->left = sliderPtr->screenMin - inset - 2;
	sliderPtr->right = sliderPtr->screenMin + sliderPtr->screenRange + inset - 1;
	if (sliderPtr->flags & STACK_AXES) {
	    sliderPtr->top = mark - marginPtr->axesOffset;
	} else {
	    sliderPtr->top = mark - sliderPtr->height;
	}
	sliderPtr->bottom = mark;
	if (sliderPtr->titleAlternate) {
	    x = sliderPtr->right + AXIS_PAD_TITLE;
	    y = mark - (sliderPtr->height  / 2);
	    sliderPtr->titleAnchor = TK_ANCHOR_W;
	} else {
	    x = (sliderPtr->right + sliderPtr->left) / 2;
	    if (sliderPtr->flags & STACK_AXES) {
		y = mark - marginPtr->axesOffset + AXIS_PAD_TITLE;
	    } else {
		y = mark - sliderPtr->height + AXIS_PAD_TITLE;
	    }
	    sliderPtr->titleAnchor = TK_ANCHOR_N;
	}
	sliderPtr->titlePos.x = x;
	sliderPtr->titlePos.y = y;
	break;

    case MARGIN_BOTTOM:
	/*
	 *  ----------- bottom + plot borderwidth
	 *      mark --------------------------------------------
	 *          ===================== axisLine (linewidth)
	 *                   tick
	 *		    title
	 *
	 *          ===================== axisLine (linewidth)
	 *  ----------- bottom + plot borderwidth
	 *      mark --------------------------------------------
	 *                   tick
	 *		    title
	 */
	axisLine = sliderPtr->bottom;
	if (sliderPtr->plotRelief == TK_RELIEF_SOLID) {
	    axisLine++;
	} 
	if (sliderPtr->flags & EXTERIOR) {
	    axisLine += sliderPtr->plotBW + axisPad + sliderPtr->lineWidth / 2;
	    tickLabel = axisLine + 2;
	    if (sliderPtr->lineWidth > 0) {
		tickLabel += sliderPtr->tickLength;
	    }
	} else {
	    axisLine -= axisPad + sliderPtr->lineWidth / 2;
	    tickLabel = sliderPtr->bottom +  sliderPtr->plotBW + 2;
	}
	mark = sliderPtr->bottom + offset;
	fangle = FMOD(sliderPtr->tickAngle, 90.0f);
	if (fangle == 0.0) {
	    sliderPtr->tickAnchor = TK_ANCHOR_N;
	} else {
	    int quadrant;

	    quadrant = (int)(sliderPtr->tickAngle / 90.0);
	    if ((quadrant == 0) || (quadrant == 2)) {
		sliderPtr->tickAnchor = TK_ANCHOR_NE;
	    } else {
		sliderPtr->tickAnchor = TK_ANCHOR_NW;
	    }
	}
	sliderPtr->left = sliderPtr->screenMin - inset - 2;
	sliderPtr->right = sliderPtr->screenMin + sliderPtr->screenRange + inset - 1;
	sliderPtr->top = sliderPtr->bottom + labelOffset - t1;
	if (sliderPtr->flags & STACK_AXES) {
	    sliderPtr->bottom = mark + marginPtr->axesOffset - 1;
	} else {
	    sliderPtr->bottom = mark + sliderPtr->height - 1;
	}
	if (sliderPtr->titleAlternate) {
	    x = sliderPtr->right + AXIS_PAD_TITLE;
	    y = mark + (sliderPtr->height / 2);
	    sliderPtr->titleAnchor = TK_ANCHOR_W; 
	} else {
	    x = (sliderPtr->right + sliderPtr->left) / 2;
	    if (sliderPtr->flags & STACK_AXES) {
		y = mark + marginPtr->axesOffset - AXIS_PAD_TITLE;
	    } else {
		y = mark + sliderPtr->height - AXIS_PAD_TITLE;
	    }
	    sliderPtr->titleAnchor = TK_ANCHOR_S; 
	}
	sliderPtr->titlePos.x = x;
	sliderPtr->titlePos.y = y;
	break;

    case MARGIN_LEFT:
	/*
	 *                    mark
	 *                  |  : 
	 *                  |  :      
	 *                  |  : 
	 *                  |  :
	 *                  |  : 
	 *     axisLine
	 */
	/* 
	 * Exterior axis 
	 *     + plotarea right
	 *     |A|B|C|D|E|F|G|H
	 *           |right
	 * A = plot pad 
	 * B = plot border width
	 * C = axis pad
	 * D = axis line
	 * E = tick length
	 * F = tick label 
	 * G = slider border width
	 * H = highlight thickness
	 */
	/* 
	 * Interior axis 
	 *     + plotarea right
	 *     |A|B|C|D|E|F|G|H
	 *           |right
	 * A = plot pad 
	 * B = tick length
	 * C = axis line width
	 * D = axis pad
	 * E = plot border width
	 * F = tick label 
	 * G = slider border width
	 * H = highlight thickness
	 */
	axisLine = sliderPtr->left;
	if (sliderPtr->flags & EXTERIOR) {
	    axisLine -= sliderdPtr->plotBW + axisPad + sliderPtr->lineWidth / 2;
	    tickLabel = axisLine - 2;
	    if (sliderPtr->lineWidth > 0) {
		tickLabel -= sliderPtr->tickLength;
	    }
	} else {
	    if (sliderPtr->plotRelief == TK_RELIEF_SOLID) {
		axisLine--;
	    } 
	    axisLine += axisPad + sliderPtr->lineWidth / 2;
	    tickLabel = sliderPtr->left - sliderPtr->plotBW - 2;
	}
	mark = sliderPtr->left - offset;
	sliderPtr->tickAnchor = TK_ANCHOR_E;
	if (sliderPtr->flags & STACK_AXES) {
	    sliderPtr->left = mark - marginPtr->axesOffset;
	} else {
	    sliderPtr->left = mark - sliderPtr->width;
	}
	sliderPtr->right = mark - 3;
	sliderPtr->top = sliderPtr->screenMin - inset - 2;
	sliderPtr->bottom = sliderPtr->screenMin + sliderPtr->screenRange + inset - 1;
	if (sliderPtr->titleAlternate) {
	    x = mark - (sliderPtr->width / 2);
	    y = sliderPtr->top - AXIS_PAD_TITLE;
	    sliderPtr->titleAnchor = TK_ANCHOR_SW; 
	} else {
	    if (sliderPtr->flags & STACK_AXES) {
		x = mark - marginPtr->axesOffset;
	    } else {
		x = mark - sliderPtr->width + AXIS_PAD_TITLE;
	    }
	    y = (sliderPtr->bottom + sliderPtr->top) / 2;
	    sliderPtr->titleAnchor = TK_ANCHOR_W; 
	} 
	sliderPtr->titlePos.x = x;
	sliderPtr->titlePos.y = y;
	break;

    case MARGIN_RIGHT:
	axisLine = sliderPtr->right;
	if (sliderPtr->plotRelief == TK_RELIEF_SOLID) {
	    axisLine++;			/* Draw axis line within solid plot
					 * border. */
	} 
	if (sliderPtr->flags & EXTERIOR) {
	    axisLine += sliderPtr->plotBW + axisPad + sliderPtr->lineWidth / 2;
	    tickLabel = axisLine + 2;
	    if (sliderPtr->lineWidth > 0) {
		tickLabel += sliderPtr->tickLength;
	    }
	} else {
	    axisLine -= axisPad + sliderPtr->lineWidth / 2;
	    tickLabel = sliderPtr->right + sliderPtr->plotBW + 2;
	}
	mark = sliderPtr->right + offset + pad;
	sliderPtr->tickAnchor = TK_ANCHOR_W;
	sliderPtr->left = mark;
	if (sliderPtr->flags & STACK_AXES) {
	    sliderPtr->right = mark + marginPtr->axesOffset - 1;
	} else {
	    sliderPtr->right = mark + sliderPtr->width - 1;
	}
	sliderPtr->top = sliderPtr->screenMin - inset - 2;
	sliderPtr->bottom = sliderPtr->screenMin + sliderPtr->screenRange + inset -1;
	if (sliderPtr->titleAlternate) {
	    x = mark + (sliderPtr->width / 2);
	    y = sliderPtr->top - AXIS_PAD_TITLE;
	    sliderPtr->titleAnchor = TK_ANCHOR_SE; 
	} else {
	    if (sliderPtr->flags & STACK_AXES) {
		x = mark + marginPtr->axesOffset - AXIS_PAD_TITLE;
	    } else {
		x = mark + sliderPtr->width - AXIS_PAD_TITLE;
	    }
	    y = (sliderPtr->bottom + sliderPtr->top) / 2;
	    sliderPtr->titleAnchor = TK_ANCHOR_E;
	}
	sliderPtr->titlePos.x = x;
	sliderPtr->titlePos.y = y;
	break;

    case MARGIN_NONE:
	axisLine = 0;
	break;
    }
    if ((marginPtr->site == MARGIN_LEFT) || (marginPtr->site == MARGIN_TOP)) {
	t1 = -t1, t2 = -t2;
	labelOffset = -labelOffset;
    }
    infoPtr->axis = axisLine;
    infoPtr->t1 = axisLine + t1;
    infoPtr->t2 = axisLine + t2;
    if (tickLabel > 0) {
	infoPtr->label = tickLabel;
    } else {
	infoPtr->label = axisLine + labelOffset;
    }
    if ((sliderPtr->flags & EXTERIOR) == 0) {
	/*infoPtr->label = axisLine + labelOffset - t1; */
	infoPtr->t1 = axisLine - t1;
	infoPtr->t2 = axisLine - t2;
    } 
}

static void
MakeAxisLine(Slider *sliderPtr, int line, Segment2d *s)
{
    double min, max;

    min = sliderPtr->outerRange.min;
    max = sliderPtr->outerRange.max;
    if (sliderPtr->flags & LOGSCALE) {
	min = EXP10(min);
	max = EXP10(max);
    }
    if (sliderPtr->flags & HORIZONTAL) {
	s->p.x = HMap(sliderPtr, min);
	s->q.x = HMap(sliderPtr, max);
	s->p.y = s->q.y = line;
    } else {
	s->q.x = s->p.x = line;
	s->p.y = VMap(sliderPtr, min);
	s->q.y = VMap(sliderPtr, max);
    }
}

static void
MakeTick(Slider *sliderPtr, double value, int tick, int line, Segment2d *s)
{
    if (sliderPtr->flags & LOGSCALE) {
	value = EXP10(value);
    }
    if (sliderPtr->flags & HORIZONTAL) {
	s->p.x = s->q.x = HMap(sliderPtr, value);
	s->p.y = line;
	s->q.y = tick;
    } else {
	s->p.x = line;
	s->p.y = s->q.y = VMap(sliderPtr, value);
	s->q.x = tick;
    }
}

static void
MakeSegments(Slider *sliderPtr, AxisInfo *infoPtr)
{
    int arraySize;
    int numMajorTicks, numMinorTicks;
    Segment2d *segments;
    Segment2d *s;

    if (sliderPtr->segments != NULL) {
	Blt_Free(sliderPtr->segments);
    }
    numMajorTicks = numMinorTicks = 0;
    if (sliderPtr->t1Ptr != NULL) {
	numMajorTicks = sliderPtr->t1Ptr->numTicks;
    }
    if (sliderPtr->t2Ptr != NULL) {
	numMinorTicks = sliderPtr->t2Ptr->numTicks;
    }
    arraySize = 1 + (numMajorTicks * (numMinorTicks + 1));
    segments = Blt_AssertMalloc(arraySize * sizeof(Segment2d));
    s = segments;
    if (sliderPtr->lineWidth > 0) {
	/* Axis baseline */
	MakeAxisLine(sliderPtr, infoPtr->axis, s);
	s++;
    }
    if (sliderPtr->flags & SHOWTICKS) {
	Blt_ChainLink link;
	double labelPos;
	int i;

	for (i = 0; i < numMajorTicks; i++) {
	    double t1, t2;
	    int j;

	    t1 = sliderPtr->t1Ptr->values[i];
	    /* Minor ticks */
	    for (j = 0; j < numMinorTicks; j++) {
		t2 = t1 + (sliderPtr->major.step * 
			   sliderPtr->t2Ptr->values[j]);
		if (InRange(t2, &sliderPtr->outerRange)) {
		    MakeTick(sliderPtr, t2, infoPtr->t2, infoPtr->axis, s);
		    s++;
		}
	    }
	    if (!InRange(t1, &sliderPtr->outerRange)) {
		continue;
	    }
	    /* Major tick */
	    MakeTick(sliderPtr, t1, infoPtr->t1, infoPtr->axis, s);
	    s++;
	}

	link = Blt_Chain_FirstLink(sliderPtr->tickLabels);
	labelPos = (double)infoPtr->label;
	for (i = 0; i < numMajorTicks; i++) {
	    double t1;
	    TickLabel *labelPtr;
	    Segment2d seg;

	    t1 = sliderPtr->t1Ptr->values[i];
	    if (sliderPtr->labelOffset) {
		t1 += sliderPtr->major.step * 0.5;
	    }
	    if (!InRange(t1, &sliderPtr->outerRange)) {
		continue;
	    }
	    labelPtr = Blt_Chain_GetValue(link);
	    link = Blt_Chain_NextLink(link);
	    MakeTick(sliderPtr, t1, infoPtr->t1, infoPtr->axis, &seg);
	    /* Save tick label X-Y position. */
	    if (sliderPtr->flags & HORIZONTAL) {
		labelPtr->x = seg.p.x;
		labelPtr->y = labelPos;
	    } else {
		labelPtr->x = labelPos;
		labelPtr->y = seg.p.y;
	    }
	}
    }
    sliderPtr->segments = segments;
    sliderPtr->numSegments = s - segments;
    assert(sliderPtr->numSegments <= arraySize);
}

/*
 *---------------------------------------------------------------------------
 *
 * MapAxis --
 *
 *	Pre-calculates positions of the axis, ticks, and labels (to be used
 *	later when displaying the axis).  Calculates the values for each
 *	major and minor tick and checks to see if they are in range (the
 *	outer ticks may be outside of the range of plotted values).
 *
 *	Line segments for the minor and major ticks are saved into one
 *	XSegment array so that they can be drawn by a single XDrawSegments
 *	call. The positions of the tick labels are also computed and saved.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Line segments and tick labels are saved and used later to draw the
 *	axis.
 *
 *---------------------------------------------------------------------------
 */
static void
MapAxis(Axis *sliderPtr, Margin *marginPtr, int offset)
{
    AxisInfo info;

    if (sliderPtr->flags & COLORBAR) {
	sliderPtr->screenMin = sliderPtr->vOffset;
	sliderPtr->height = sliderPtr->bottom;
	sliderPtr->height -= sliderPtr->top + Blt_Legend_Height(sliderPtr);
	sliderPtr->screenRange = sliderPtr->vRange - Blt_Legend_Height(sliderPtr);
    } else if (sliderPtr->flags & HORIZONTAL) {
	sliderPtr->screenMin = sliderPtr->hOffset;
	sliderPtr->width = sliderPtr->right - sliderPtr->left;
	sliderPtr->screenRange = sliderPtr->hRange;
    } else {
	sliderPtr->screenMin = sliderPtr->vOffset;
	sliderPtr->height = sliderPtr->bottom - sliderPtr->top;
	sliderPtr->screenRange = sliderPtr->vRange;
    }
    sliderPtr->screenScale = 1.0 / sliderPtr->screenRange;
    AxisOffsets(sliderPtr, marginPtr, offset, &info);
    MakeSegments(sliderPtr, &info);
}

/*
 *---------------------------------------------------------------------------
 *
 * MapStackedAxis --
 *
 *	Pre-calculates positions of the axis, ticks, and labels (to be used
 *	later when displaying the axis).  Calculates the values for each
 *	major and minor tick and checks to see if they are in range (the
 *	outer ticks may be outside of the range of plotted values).
 *
 *	Line segments for the minor and major ticks are saved into one
 *	XSegment array so that they can be drawn by a single XDrawSegments
 *	call. The positions of the tick labels are also computed and saved.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Line segments and tick labels are saved and used later to draw the
 *	axis.
 *
 *---------------------------------------------------------------------------
 */
static void
MapStackedAxis(Axis *sliderPtr, Margin *marginPtr, float totalWeight)
{
    AxisInfo info;
    unsigned int w, h, n, slice;
    float ratio;

    n = sliderPtr->margins[sliderPtr->margin].axes->numLinks;
    if ((n > 1) || (sliderPtr->reqNumMajorTicks <= 0)) {
	sliderPtr->reqNumMajorTicks = 4;
    }
    ratio = sliderPtr->weight / totalWeight;
    if (sliderPtr->flags & HORIZONTAL) {
	sliderPtr->screenMin = sliderPtr->hOffset;
	sliderPtr->screenRange = sliderPtr->hRange;
	slice = (int)(sliderPtr->hRange * ratio);
	sliderPtr->width = slice;
    } else {
	sliderPtr->screenMin = sliderPtr->vOffset;
	sliderPtr->screenRange = sliderPtr->vRange;
	slice = (int)(sliderPtr->vRange * ratio);
	sliderPtr->height = slice;
    }
#define AXIS_PAD 2
    Blt_GetTextExtents(sliderPtr->tickFont, 0, "0", 1, &w, &h);
    if (n > 1) {
	sliderPtr->screenMin += marginPtr->offset + AXIS_PAD + h / 2;
	sliderPtr->screenRange = slice - 2 * AXIS_PAD - h;
	marginPtr->offset += slice;
    }
    sliderPtr->screenScale = 1.0f / sliderPtr->screenRange;
    AxisOffsets(sliderPtr, marginPtr, 0, &info);
    MakeSegments(sliderPtr, &info);
}

/*
 *---------------------------------------------------------------------------
 *
 * AdjustViewport --
 *
 *	Adjusts the offsets of the viewport according to the scroll mode.
 *	This is to accommodate both "listbox" and "canvas" style scrolling.
 *
 *	"canvas"	The viewport scrolls within the range of world
 *			coordinates.  This way the viewport always displays
 *			a full page of the world.  If the world is smaller
 *			than the viewport, then (bizarrely) the world and
 *			viewport are inverted so that the world moves up
 *			and down within the viewport.
 *
 *	"listbox"	The viewport can scroll beyond the range of world
 *			coordinates.  Every entry can be displayed at the
 *			top of the viewport.  This also means that the
 *			scrollbar thumb weirdly shrinks as the last entry
 *			is scrolled upward.
 *
 * Results:
 *	The corrected offset is returned.
 *
 *---------------------------------------------------------------------------
 */
static double
AdjustViewport(double offset, double windowSize)
{
    /*
     * Canvas-style scrolling allows the world to be scrolled within the
     * window.
     */
    if (windowSize > 1.0) {
	if (windowSize < (1.0 - offset)) {
	    offset = 1.0 - windowSize;
	}
	if (offset > 0.0) {
	    offset = 0.0;
	}
    } else {
	if ((offset + windowSize) > 1.0) {
	    offset = 1.0 - windowSize;
	}
	if (offset < 0.0) {
	    offset = 0.0;
	}
    }
    return offset;
}

static int
GetAxisScrollInfo(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv,
		  double *offsetPtr, double windowSize, double scrollUnits,
		  double scale)
{
    const char *string;
    char c;
    double offset;
    int length;

    offset = *offsetPtr;
    string = Tcl_GetStringFromObj(objv[0], &length);
    c = string[0];
    scrollUnits *= scale;
    if ((c == 's') && (strncmp(string, "scroll", length) == 0)) {
	int count;
	double fract;

	assert(objc == 3);
	/* Scroll number unit/page */
	if (Tcl_GetIntFromObj(interp, objv[1], &count) != TCL_OK) {
	    return TCL_ERROR;
	}
	string = Tcl_GetStringFromObj(objv[2], &length);
	c = string[0];
	if ((c == 'u') && (strncmp(string, "units", length) == 0)) {
	    fract = count * scrollUnits;
	} else if ((c == 'p') && (strncmp(string, "pages", length) == 0)) {
	    /* A page is 90% of the view-able window. */
	    fract = (int)(count * windowSize * 0.9 + 0.5);
	} else if ((c == 'p') && (strncmp(string, "pixels", length) == 0)) {
	    fract = count * scale;
	} else {
	    Tcl_AppendResult(interp, "unknown \"scroll\" units \"", string,
		"\"", (char *)NULL);
	    return TCL_ERROR;
	}
	offset += fract;
    } else if ((c == 'm') && (strncmp(string, "moveto", length) == 0)) {
	double fract;

	assert(objc == 2);
	/* moveto fraction */
	if (Blt_GetDoubleFromObj(interp, objv[1], &fract) != TCL_OK) {
	    return TCL_ERROR;
	}
	offset = fract;
    } else {
	int count;
	double fract;

	/* Treat like "scroll units" */
	if (Tcl_GetIntFromObj(interp, objv[0], &count) != TCL_OK) {
	    return TCL_ERROR;
	}
	fract = (double)count * scrollUnits;
	offset += fract;
	/* CHECK THIS: return TCL_OK; */
    }
    *offsetPtr = AdjustViewport(offset, windowSize);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawAxis --
 *
 *	Draws the axis, ticks, and labels onto the canvas.
 *
 *	Initializes and passes text attribute information through TextStyle
 *	structure.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Axis gets drawn on window.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawAxis(Slider *sliderPtr, Drawable drawable)
{
    int isHoriz;

    if (sliderPtr->normalBg != NULL) {
	Blt_Bg_FillRectangle(sliderPtr->tkwin, drawable, 
		sliderPtr->normalBg, 
		sliderPtr->axisLeft, sliderPtr->axisTop, 
		sliderPtr->axisRight - sliderPtr->axisLeft, 
		sliderPtr->axisBottom - sliderPtr->axisTop, 
		sliderPtr->borderWidth, sliderPtr->relief);
    }
    if (sliderPtr->title != NULL) {
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetAngle(ts, sliderPtr->titleAngle);
	Blt_Ts_SetFont(ts, sliderPtr->titleFont);
	Blt_Ts_SetPadding(ts, 1, 2, 0, 0);
	Blt_Ts_SetAnchor(ts, sliderPtr->titleAnchor);
	Blt_Ts_SetJustify(ts, sliderPtr->titleJustify);
	if (sliderPtr->flags & ACTIVE) {
	    Blt_Ts_SetForeground(ts, sliderPtr->activeFg);
	} else {
	    Blt_Ts_SetForeground(ts, sliderPtr->titleFg);
	}
	Blt_Ts_SetForeground(ts, sliderPtr->titleFg);
	if ((sliderPtr->titleAngle == 90.0) || (sliderPtr->titleAngle == 270.0)) {
	    Blt_Ts_SetMaxLength(ts, sliderPtr->height);
	} else {
	    Blt_Ts_SetMaxLength(ts, sliderPtr->width);
	}
	Blt_Ts_DrawText(sliderPtr->tkwin, drawable, sliderPtr->title, -1, &ts, 
		(int)sliderPtr->titlePos.x, (int)sliderPtr->titlePos.y);
    }
    if (sliderPtr->scrollCmdObjPtr != NULL) {
	double viewWidth, viewMin, viewMax;
	double worldWidth, worldMin, worldMax;
	double fract;

	worldMin = sliderPtr->valueRange.min;
	worldMax = sliderPtr->valueRange.max;
	if (DEFINED(sliderPtr->scrollMin)) {
	    worldMin = sliderPtr->scrollMin;
	}
	if (DEFINED(sliderPtr->scrollMax)) {
	    worldMax = sliderPtr->scrollMax;
	}
	viewMin = sliderPtr->min;
	viewMax = sliderPtr->max;
	if (viewMin < worldMin) {
	    viewMin = worldMin;
	}
	if (viewMax > worldMax) {
	    viewMax = worldMax;
	}
	if (IsLogScale(sliderPtr)) {
	    worldMin = log10(worldMin);
	    worldMax = log10(worldMax);
	    viewMin = log10(viewMin);
	    viewMax = log10(viewMax);
	}
	worldWidth = worldMax - worldMin;	
	viewWidth = viewMax - viewMin;
	isHoriz = ((sliderPtr->flags & HORIZONTAL) == HORIZONTAL);

	if (isHoriz != sliderPtr->decreasing) {
	    fract = (viewMin - worldMin) / worldWidth;
	} else {
	    fract = (worldMax - viewMax) / worldWidth;
	}
	fract = AdjustViewport(fract, viewWidth / worldWidth);

	if (isHoriz != sliderPtr->decreasing) {
	    viewMin = (fract * worldWidth);
	    sliderPtr->min = viewMin + worldMin;
	    sliderPtr->max = sliderPtr->min + viewWidth;
	    viewMax = viewMin + viewWidth;
	    if (IsLogScale(sliderPtr)) {
		sliderPtr->min = EXP10(sliderPtr->min);
		sliderPtr->max = EXP10(sliderPtr->max);
	    }
	    Blt_UpdateScrollbar(sliderPtr->interp, sliderPtr->scrollCmdObjPtr,
		(int)viewMin, (int)viewMax, (int)worldWidth);
	} else {
	    viewMax = (fract * worldWidth);
	    sliderPtr->max = worldMax - viewMax;
	    sliderPtr->min = sliderPtr->max - viewWidth;
	    viewMin = viewMax + viewWidth;
	    if (IsLogScale(sliderPtr)) {
		sliderPtr->min = EXP10(sliderPtr->min);
		sliderPtr->max = EXP10(sliderPtr->max);
	    }
	    Blt_UpdateScrollbar(sliderPtr->interp, sliderPtr->scrollCmdObjPtr,
		(int)viewMax, (int)viewMin, (int)worldWidth);
	}
    }
    if (sliderPtr->flags & SHOWTICKS) {
	Blt_ChainLink link;
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetAngle(ts, sliderPtr->tickAngle);
	Blt_Ts_SetFont(ts, sliderPtr->tickFont);
	Blt_Ts_SetPadding(ts, 2, 0, 0, 0);
	Blt_Ts_SetAnchor(ts, sliderPtr->tickAnchor);
	if (sliderPtr->flags & ACTIVE) {
	    Blt_Ts_SetForeground(ts, sliderPtr->activeFg);
	} else {
	    Blt_Ts_SetForeground(ts, sliderPtr->tickFg);
	}
	for (link = Blt_Chain_FirstLink(sliderPtr->tickLabels); link != NULL;
	    link = Blt_Chain_NextLink(link)) {	
	    TickLabel *labelPtr;

	    labelPtr = Blt_Chain_GetValue(link);
	    /* Draw major tick labels */
	    Blt_DrawText(sliderPtr->tkwin, drawable, labelPtr->string, &ts, 
		labelPtr->x, labelPtr->y);
	}
    }
    if ((sliderPtr->numSegments > 0) && (sliderPtr->lineWidth > 0)) {	
	GC gc;

	if (sliderPtr->flags & ACTIVE) {
	    gc = sliderPtr->activeTickGC;
	} else {
	    gc = sliderPtr->tickGC;
	}
	/* Draw the tick marks and axis line. */
	Blt_DrawSegments2d(sliderPtr->display, drawable, gc, sliderPtr->segments, 
		sliderPtr->numSegments);
    }
}

static void
DrawAxis(Slider *sliderPtr, Drawable drawable, int x, int y)
{
    /* Trough */
    Blt_PaintPicture(sliderPtr->painter, drawable, sliderPtr->trough, 0, 0, 
		     Blt_Picture_Width(sliderPtr->trough),
		     Blt_Picture_Height(sliderPtr->trough),
		     x, y);

    if (sliderPtr->flags & SHOWTICKS) {
	Blt_ChainLink link;
	TextStyle ts;

	DrawAxis(sliderPtr, drawable);
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetAngle(ts, sliderPtr->tickAngle);
	Blt_Ts_SetFont(ts, sliderPtr->tickFont);
	Blt_Ts_SetPadding(ts, 2, 0, 0, 0);
	if (sliderPtr->flags & STATE_DISABLED) {
	    Blt_Ts_SetForeground(ts, sliderPtr->disabledFg);
	} else {
	    Blt_Ts_SetForeground(ts, sliderPtr->tickFg);
	}
	for (link = Blt_Chain_FirstLink(sliderPtr->tickLabels); link != NULL;
	    link = Blt_Chain_NextLink(link)) {	
	    TickLabel *labelPtr;

	    labelPtr = Blt_Chain_GetValue(link);
	    /* Draw major tick labels */
	    Blt_DrawText(sliderPtr->tkwin, drawable, labelPtr->string, &ts, 
		labelPtr->x, labelPtr->y);
	}
	if ((sliderPtr->numSegments > 0) && (sliderPtr->lineWidth > 0)) {	
	    GC gc;

	    if (sliderPtr->flags & ACTIVE) {
		gc = sliderPtr->activeTickGC;
	    } else {
		gc = sliderPtr->tickGC;
	    }
	    /* Draw the tick marks. */
	    XDrawSegments(sliderPtr->display, drawable, gc, sliderPtr->segments, 
		sliderPtr->numSegments);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawHorizontalAxis --
 *
 *	Draws the axis, ticks, and labels onto the canvas.
 *
 *	Initializes and passes text attribute information through TextStyle
 *	structure.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Axis gets drawn on window.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawHorizontalAxis(Slider *sliderPtr, Drawable drawable)
{
    height = Tk_Height(sliderPtr->tkwin) - 2 * sliderPtr->insert;
    x = y = sliderPtr->inset;
    /* Title. */
     if (sliderPtr->title != NULL) {
	TextStyle ts;

	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetFont(ts, sliderPtr->titleFont);
	Blt_Ts_SetPadding(ts, 1, 2, 0, 0);
	if (sliderPtr->flags & ACTIVE) {
	    Blt_Ts_SetForeground(ts, sliderPtr->activeFg);
	} else {
	    Blt_Ts_SetForeground(ts, sliderPtr->titleFg);
	}
	Blt_Ts_SetForeground(ts, sliderPtr->titleFg);
	Blt_Ts_SetMaxLength(ts, Tk_Width(sliderPtr->tkwin));
	Blt_Ts_DrawText(sliderPtr->tkwin, drawable, sliderPtr->title, -1, &ts, 
		(int)sliderPtr->titleX, (int)sliderPtr->titleY);
	y += sliderPtr->titleHeight;
	height -= sliderPtr->titleHeight;
    }
    /* Left arrow. */
    if ((sliderPtr->flags & SHOW_ARROWS) && (sliderPtr->leftArrow != NULL)) {
	int iy;
	
	iy = y;
	if (Blt_Picture_Height(sliderPtr->leftArrow) < height) {
	    iy += (height - Blt_Picture_Height(sliderPtr->leftArrow)) / 2;
	}
	Blt_PaintPicture(sliderPtr->painter, drawable, sliderPtr->leftArrow,
			 0, 0, 
			 Blt_Picture_Width(sliderPtr->leftArrow),
			 Blt_Picture_Height(sliderPtr->leftArrow),
			 x, iy);
	x += Blt_Picture_Width(sliderPtr->leftArrow);
    }
    x += sliderPtr->sliderRadius;

    /* Trough */
    Blt_PaintPicture(sliderPtr->painter, drawable, sliderPtr->trough, 0, 0, 
		     Blt_Picture_Width(sliderPtr->trough),
		     Blt_Picture_Height(sliderPtr->trough),
		     x, y);

    if (sliderPtr->flags & SHOWTICKS) {
	Blt_ChainLink link;
	TextStyle ts;

	DrawAxis(sliderPtr, drawable);
	Blt_Ts_InitStyle(ts);
	Blt_Ts_SetAngle(ts, sliderPtr->tickAngle);
	Blt_Ts_SetFont(ts, sliderPtr->tickFont);
	Blt_Ts_SetPadding(ts, 2, 0, 0, 0);
	if (sliderPtr->flags & STATE_DISABLED) {
	    Blt_Ts_SetForeground(ts, sliderPtr->disabledFg);
	} else {
	    Blt_Ts_SetForeground(ts, sliderPtr->tickFg);
	}
	for (link = Blt_Chain_FirstLink(sliderPtr->tickLabels); link != NULL;
	    link = Blt_Chain_NextLink(link)) {	
	    TickLabel *labelPtr;

	    labelPtr = Blt_Chain_GetValue(link);
	    /* Draw major tick labels */
	    Blt_DrawText(sliderPtr->tkwin, drawable, labelPtr->string, &ts, 
		labelPtr->x, labelPtr->y);
	}
	if ((sliderPtr->numSegments > 0) && (sliderPtr->lineWidth > 0)) {	
	    GC gc;

	    if (sliderPtr->flags & ACTIVE) {
		gc = sliderPtr->activeTickGC;
	    } else {
		gc = sliderPtr->tickGC;
	    }
	    /* Draw the tick marks. */
	    XDrawSegments(sliderPtr->display, drawable, gc, sliderPtr->segments, 
		sliderPtr->numSegments);
	}
    }
    x += sliderPtr->troughLength + sliderPtr->sliderRadius;
    /* Right arrow. */
    if ((sliderPtr->flags & SHOW_ARROWS) && (sliderPtr->rightArrow != NULL)) {
	int iy;
	
	iy = y;
	if (Blt_Picture_Height(sliderPtr->rightArrow) < height) {
	    iy += (height - Blt_Picture_Height(sliderPtr->rightArrow)) / 2;
	}
	Blt_PaintPicture(sliderPtr->painter, drawable, sliderPtr->rightArrow,
			 0, 0, 
			 Blt_Picture_Width(sliderPtr->rightArrow),
			 Blt_Picture_Height(sliderPtr->rightArrow),
			 x, iy);
	x += Blt_Picture_Width(sliderPtr->rightArrow);
    }
    /* Minimum. */
    if (sliderPtr->mode == MODE_RANGE) {
	if (sliderPtr->flags & SHOW_RANGE) {
	    DrawRange(sliderPtr, drawable, x, y);
	}
	DrawMinControl(sliderPtr, drawable);
	if (sliderPtr->flags & SHOW_VALUES) {
	    DrawMinValue(sliderPtr, drawable, x, y);
	}
    }
    /* Maximum. */
    DrawMaxControl(sliderPtr, drawable);
    if (sliderPtr->flags & SHOW_VALUES) {
	DrawMaxValue(sliderPtr, drawable, x, y);
    }
}

static void
MakeGridLine(Axis *sliderPtr, double value, Segment2d *s)
{
    if (IsLogScale(sliderPtr)) {
	value = EXP10(value);
    }
    /* Grid lines run orthogonally to the axis */
    if (sliderPtr->flags & HORIZONTAL) {
	s->p.y = sliderPtr->top;
	s->q.y = sliderPtr->bottom;
	s->p.x = s->q.x = Blt_HMap(sliderPtr, value);
    } else {
	s->p.x = sliderPtr->left;
	s->q.x = sliderPtr->right;
	s->p.y = s->q.y = Blt_VMap(sliderPtr, value);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapGridlines --
 *
 *	Assembles the grid lines associated with an axis. Generates tick
 *	positions if necessary (this happens when the axis is not a logical
 *	axis too).
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
MapGridlines(Axis *sliderPtr)
{
    Segment2d *s1, *s2;
    Ticks *t1Ptr, *t2Ptr;
    int needed;
    int i;

    if (sliderPtr == NULL) {
	return;
    }
    t1Ptr = sliderPtr->t1Ptr;
    if (t1Ptr == NULL) {
	t1Ptr = GenerateTicks(&sliderPtr->major);
    }
    t2Ptr = sliderPtr->t2Ptr;
    if (t2Ptr == NULL) {
	t2Ptr = GenerateTicks(&sliderPtr->minor);
    }
    needed = t1Ptr->numTicks;
    if (sliderPtr->flags & GRIDMINOR) {
	needed += (t1Ptr->numTicks * t2Ptr->numTicks);
    }
    if (needed == 0) {
	return;			
    }
    needed = t1Ptr->numTicks;
    if (needed != sliderPtr->major.numAllocated) {
	if (sliderPtr->major.segments != NULL) {
	    Blt_Free(sliderPtr->major.segments);
	}
	sliderPtr->major.segments = Blt_AssertMalloc(sizeof(Segment2d) * needed);
	sliderPtr->major.numAllocated = needed;
    }
    needed = (t1Ptr->numTicks * t2Ptr->numTicks);
    if (needed != sliderPtr->minor.numAllocated) {
	if (sliderPtr->minor.segments != NULL) {
	    Blt_Free(sliderPtr->minor.segments);
	}
	sliderPtr->minor.segments = Blt_AssertMalloc(sizeof(Segment2d) * needed);
	sliderPtr->minor.numAllocated = needed;
    }
    s1 = sliderPtr->major.segments, s2 = sliderPtr->minor.segments;
    for (i = 0; i < t1Ptr->numTicks; i++) {
	double value;

	value = t1Ptr->values[i];
	if (sliderPtr->flags & GRIDMINOR) {
	    int j;

	    for (j = 0; j < t2Ptr->numTicks; j++) {
		double subValue;

		subValue = value + (sliderPtr->major.step * 
				    t2Ptr->values[j]);
		if (InRange(subValue, &sliderPtr->axisRange)) {
		    MakeGridLine(sliderPtr, subValue, s2);
		    s2++;
		}
	    }
	}
	if (InRange(value, &sliderPtr->axisRange)) {
	    MakeGridLine(sliderPtr, value, s1);
	    s1++;
	}
    }
    if (t1Ptr != sliderPtr->t1Ptr) {
	Blt_Free(t1Ptr);		/* Free generated ticks. */
    }
    if (t2Ptr != sliderPtr->t2Ptr) {
	Blt_Free(t2Ptr);		/* Free generated ticks. */
    }
    sliderPtr->major.numUsed = s1 - sliderPtr->major.segments;
    sliderPtr->minor.numUsed = s2 - sliderPtr->minor.segments;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetAxisGeometry --
 *
 * Results:
 *	None.
 *
 * Exterior axis:
 *                    l       r
 *  |a|b|c|d|e|f|g|h|i|   j   |i|h|g|f|e|d|c|d|a|
 *
 * Interior axis: 
 *                  l           r
 *  |a|b|c|d|h|g|f|e|     j     |e|f|g|h|d|c|b|a|
 *               i..             ..i 
 * a = highlight thickness
 * b = graph borderwidth
 * c = axis title
 * d = tick label 
 * e = tick 
 * f = axis line
 * g = 1 pixel pad
 * h = plot borderwidth
 * i = plot pad
 * j = plot area 
 *---------------------------------------------------------------------------
 */
static void
GetAxisGeometry(Slider *sliderPtr)
{
    int y;

    FreeTickLabels(sliderPtr->tickLabels);

    y = sliderPtr->lineWidth + 2;

    sliderPtr->leftTickLabelHeight = sliderPtr->rightTickLabelWidth = 0;
    sliderPtr->axisWidth = sliderPtr->axisHeight = 0;
    if (sliderPtr->flags & SHOWTICKS) {
	unsigned int pad;
	unsigned int i, numLabels, numTicks;

	SweepTicks(sliderPtr);

	numTicks = (sliderPtr->t1Ptr == NULL) ? 0 : sliderPtr->t1Ptr->numTicks;
	assert(numTicks <= MAXTICKS);

	numLabels = 0;
	for (i = 0; i < numTicks; i++) {
	    TickLabel *labelPtr;
	    double x, x2;
	    int lw, lh;			/* Label width and height. */

	    x2 = x = sliderPtr->t1Ptr->values[i];
	    if (sliderPtr->labelOffset) {
		x2 += sliderPtr->major.step * 0.5;
	    }
	    if (!InRange(x2, &sliderPtr->axisRange)) {
		continue;
	    }
	    labelPtr = MakeLabel(sliderPtr, x);
	    Blt_Chain_Append(sliderPtr->tickLabels, labelPtr);
	    numLabels++;

	    /* 
	     * Get the dimensions of each tick label.  Remember tick labels
	     * can be multi-lined and/or rotated.
	     */
	    Blt_GetTextExtents(sliderPtr->tickFont, 0, labelPtr->string, -1, 
		&labelPtr->width, &labelPtr->height);

	    if (sliderPtr->tickAngle != 0.0f) {
		double rlw, rlh;	/* Rotated label width and height. */

		Blt_GetBoundingBox(labelPtr->width, labelPtr->height, 
			sliderPtr->tickAngle, &rlw, &rlh, NULL);
		lw = ROUND(rlw), lh = ROUND(rlh);
	    } else {
		lw = labelPtr->width;
		lh = labelPtr->height;
	    }
	    if (sliderPtr->maxTickLabelWidth < lw) {
		sliderPtr->maxTickLabelWidth = lw;
	    }
	    if (sliderPtr->maxTickLabelHeight < lh) {
		sliderPtr->maxTickLabelHeight = lh;
	    }
	}
	assert(numLabels <= numTicks);
	
	/* Because the axis cap style is "CapProjecting", we need to account
	 * for an extra 1.5 linewidth at the end of each line.  */
	pad = ((sliderPtr->lineWidth * 12) / 8);
	if (sliderPtr->flags & HORIZONTAL) {
	    y += sliderPtr->maxTickLabelHeight + pad;
	} else {
	    y += sliderPtr->maxTickLabelWidth + pad;
	    if (sliderPtr->maxTickLabelWidth > 0) {
		y += 5;			/* Pad either size of label. */
	    }  
	}
	y += 2 * AXIS_PAD_TITLE;
	if ((sliderPtr->lineWidth > 0) && (sliderPtr->flags & EXTERIOR)) {
	    /* Distance from axis line to tick label. */
	    y += sliderPtr->tickLength;
	}
    }
    if (sliderPtr->title != NULL) {
	y += sliderPtr->titleHeight + AXIS_PAD_TITLE;
    }

    /* Correct for orientation of the axis. */
    if (sliderPtr->flags & HORIZONTAL) {
	sliderPtr->axisHeight = y;
    } else {
	sliderPtr->axisWidth = y;
    }
}

static void
GetValueExtents(Slider *sliderPtr, const char *string, int *widthPtr, 
		int *heightPtr)
{
    double rw, rh;	/* Rotated label width and height. */
    int w, h;
    
    Blt_GetTextExtents(sliderPtr->valueFont, 0, string, -1, &w, &h);
    if (sliderPtr->valueAngle != 0.0f) {
	Blt_GetBoundingBox(lw, lh, sliderPtr->valueAngle, &rw, &rh, NULL);
	w = ROUND(rw);
	h = ROUND(rh);
    }
    *heightPtr = h;
    *widthPtr = w;
}
	    


/*
 *---------------------------------------------------------------------------
 *
 * ComputeGeometry --
 *
 *	Examines all the axes in the given margin and determines the area
 *	required to display them.
 *
 *	Note: For multiple axes, the titles are displayed in another
 *	      margin. So we must keep track of the widest title.
 *	
 * Results:
 *	Returns the width or height of the margin, depending if it runs
 *	horizontally along the graph or vertically.
 *
 * Side Effects:
 *	The area width and height set in the margin.  Note again that this
 *	may be corrected later (mulitple axes) to adjust for the longest
 *	title in another margin.
 *
 *---------------------------------------------------------------------------
 */
static int
ComputeGeometry(Slider *sliderPtr)
{
    sliderPtr->flags &= ~GEOMETRY;
    sliderPtr->inset = sliderPtr->borderWidth + sliderPtr->highlightWidth;
    if (sliderPtr->flags & AXIS_GEOMETRY) {
	GetAxisGeometry(sliderPtr);
    }
    if (sliderPtr->flags & SHOW_VALUES) {
	GetValueExtents(sliderPtr, sliderPtr->minValue, 
		&sliderPtr->minValWidth, &sliderPtr->minValHeight);
	GetValueExtents(sliderPtr, sliderPtr->maxValue, 
		&sliderPtr->maxValWidth, &sliderPtr->maxValHeight);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeHorizonatalLayout --
 *
 *      |h|b|g|arr|g|min|slot|max|g|arr|g|b|h|
 *      |h|b||g|min|slot|max|g|b|h|
 *
 *      highlight thickness
 *      borderwidth
 *      gap
 *      min/max text height
 *      gap
 *      min/max/nom icon height
 *      tick length
 *      gap
 *      tick text height
 *      gap
 *      borderwidth
 *      higlight thickness
 *---------------------------------------------------------------------------
 */
static int
ComputeHorizontalLayout(Slider *sliderPtr)
{
    int left, right, width, height;
    int leftButtonWidth, rightButtonWidth;
    sliderPtr->flags &= ~LAYOUT_PENDING;

    sliderPtr->inset = sliderPtr->borderWidth + sliderPtr->highlightThickness;
    left = right = 0;
    left  = 2 * sliderPtr->inset + PADX;
    right = 2 * sliderPtr->inset + PADX;

    law = raw = lah = rah = 0;
    h = 0;
    if (sliderPtr->flags & SHOW_ARROWS) {
	if (sliderPtr->leftArrow != NULL) {
	    law = Blt_Picture_Width(sliderPtr->leftArrow);
	    lah = Blt_Picture_Height(sliderPtr->leftArrow);
	    if (h < lah) {
		h = lah;
	    }
	    left += law + PADX;
	}
	if (sliderPtr->rightArrow != NULL) {
	    rbw = Blt_Picture_Width(sliderPtr->rightArrow);
	    rbh = Blt_Picture_Height(sliderPtr->leftArrow);
	    if (h < rbh) {
		h = rbh;
	    }
	    right += raw + PADX;
	}
    }
    ih1 = ih2 = ih3 = ICON_HEIGHT;
    iw1 = iw2 = iw3 = ICON_WIDTH;
    if (sliderPtr->minIcon != NULL) {
	ih1 = Blt_Picture_Height(sliderPtr->minIcon);
	iw1 = Blt_Picture_Width(sliderPtr->minIcon);
    }
    if (sliderPtr->maxIcon != NULL) {
	ih2 = Blt_Picture_Height(sliderPtr->maxIcon);
	iw2 = Blt_Picture_Width(sliderPtr->maxIcon);
    }
    if (sliderPtr->nomIcon != NULL) {
	ih3 = Blt_Picture_Height(sliderPtr->nomIcon);
	iw3 = Blt_Picture_Width(sliderPtr->nomIcon);
    }
    ih = MAX3(ih1, ih2, ih2);
    iw = MAX3(iw1, iw2, iw3);

    throughLength = Tk_Width(sliderPtr->tkwin) - right - left - iw;
    left += miw

    if (h < (sliderPtr->sliderRadius * 2)) {
	h = sliderPtr->sliderRadius * 2;
    }
    if (sliderPtr->flags & SHOW_TICKS) {
	Blt_ChainLink link;
	TickLabel *labelPtr;

	    /* First tick. */
	    link = Blt_Chain_FirstLink(sliderPtr->ticks);
	    labelPtr = Blt_Chain_GetValue(link);
	    leftTickLabelWidth = labelPtr->width;
	    /* Last tick. */
	    link = Blt_Chain_LastLink(sliderPtr->ticks);
	    labelPtr = Blt_Chain_GetValue(link);
	    rightTickLabelWidth = labelPtr->width;

	    height += sliderPtr->axisHeight;
	}

	sliderPtr->leftOffset = MAX(leftButtonWidth,  leftTickLabelWidth / 2);
	sliderPtr->rightOffset = MAX(rightButtonWidth, rightTickLabelWidth / 2);
	left  += sliderPtr->leftOffset;
	right += sliderPtr->rightOffset;
	if (sliderPtr->flags & SHOW_VALUES) {
	    if (sliderPtr->tickAngle != 0.0f) {
		if (height < sliderPtr->minValHeight) {
		    height = sliderPtr->minValHeight;
		}
		if (height < sliderPtr->maxValHeight) {
		    height = sliderPtr->maxValHeight;
		}
	    }
	}
	if (sliderPtr->titleObjPtr != NULL) {
	    height += sliderPtr->titleObjPtr;
	}
	width = left + right + sliderPtr->maxTickValueWidth;
	height += sliderPtr->titleHeight;
	sliderPtr->troughLength = Tk_Width(sliderPtr->tkwin) - width;
	sliderPtr->screenMin = left;
	sliderPtr->screenRange = sliderPtr->troughLength;
	sliderPtr->normalHeight = height;
	sliderPtr->normalWidth = width;
    } else if (sliderPtr->flags & VERTICAL) {
	int top, bottom, width, height;

	top = bottom = sliderPtr->inset + sliderPtr->sliderRadius;
	width = height = 0;
	if (sliderPtr->flags & SHOW_ARROWS) {
	    if (sliderPtr->leftArrow != NULL) {
		top += Blt_Picture_Height(sliderPtr->leftArrow);
		if (width < Blt_Picture_Width(sliderPtr->leftArrow)) {
		    width = Blt_Picture_Width(sliderPtr->leftArrow);
		}
	    }
	    if (sliderPtr->rightArrow != NULL) {
		bottom += Blt_Picture_Height(sliderPtr->rightArrow);
		if (width < Blt_Picture_Width(sliderPtr-rightArrow)) {
		    width = Blt_Picture_Width(sliderPtr->rightArrow);
		}
	    }
	}
	if (width < (sliderPtr->sliderRadius * 2)) {
	    width = sliderPtr->sliderRadius * 2;
	}
	if (sliderPtr->flags & SHOW_TICKS) {
	    width += sliderPtr->axisWidth;
	}
	if (sliderPtr->flags & SHOW_VALUES) {
	    if (width < sliderPtr->minValWidth) {
		width = sliderPtr->minValWidth;
	    }
	    if (width < sliderPtr->maxValWidth) {
		width = sliderPtr->maxValWidth;
	    }
	}
	height = top + bottom + sliderPtr->titleHeight + 
	    sliderPtr->maxTickLabelHeight;
	sliderPtr->troughLength = Tk_Height(sliderPtr->tkwin) - height;
	sliderPtr->screenMin = top;
	sliderPtr->screenRange = sliderPtr->troughLength;
	sliderPtr->normalWidth = width;
	sliderPtr->normalHeight = height;
    }
}

DrawHorizontalTrough(Slider *sliderPtr, Drawable drawable)
{
    int x, y, w, h;

    y = sliderPtr->valueHeight + PAD;
    x = sliderPtr->leftArrowWidth + PAD;
    w = Tk_Width(sliderPtr->tkwin) - 
	(sliderPtr->leftArrowWidth + sliderPtr->rightArrowHeight + 
	 2 * sliderPtr->inset + 2 * PAD);
    h = MAX3(sliderPtr->troughHeight, sliderPtr->leftControlHeight, 
	    sliderPtr->rightControlHeight);
    
	(sliderPtr->valueHeight + PAD + sliderPtr->axisHeight + PAD +
	 2 * sliderPtr->inset);
    r = 8;
    Blt_PaintRectangle(sliderPtr->painter, sliderPtr->trough, 
		       w, slidePtr->throughHeight + 3);
    brushPtr = Blt_Bg_PaintBrush(sliderPtr->bgnormalBg);
    Blt_SetBrushOrigin(brushPtr, -x, -y);
    Blt_PaintRectangle(picture, 0, 0, sliderPtr->troughWidth,
		sliderPtr->troughHeight, 0, 0, brushPtr);

    Blt_PaintPicture(sliderPtr->painter, drawable, sliderPtr->trough, 0, 0, 
		     sliderPtBlt_Picture_Width(sliderPtr->trough),
		     Blt_Picture_Height(sliderPtr->trough),
		     x, y);

}

/*
 *---------------------------------------------------------------------------
 *
 * DrawHorizonatalLayout --
 *
 *      |h|b|g|arr|g|min|slot|max|g|arr|g|b|h|
 *      |h|b||g|min|slot|max|g|b|h|
 *
 *      highlight thickness
 *      borderwidth
 *      gap
 *      min/max text height
 *      gap
 *      min/max/nom icon height
 *      tick length
 *      gap
 *      tick text height
 *      gap
 *      borderwidth
 *      higlight thickness
 *---------------------------------------------------------------------------
 */
static int
DrawHorizontalSlider(Slider *sliderPtr)
{
    int left, right, width, height;
    int leftButtonWidth, rightButtonWidth;
    sliderPtr->flags &= ~LAYOUT_PENDING;

    width = Tk_Width(sliderPtr->width);
    height = Tk_Height(sliderPtr->height);

    width  -= 2 * (sliderPtr->inset + PADX);
    height -= 2 * (sliderPtr->inset + PADX);

    left = right = 0;
    left  = 2 * sliderPtr->inset + PADX;
    right = 2 * sliderPtr->inset + PADX;

    law = raw = lah = rah = 0;
    h = 0;
    if (sliderPtr->flags & SHOW_ARROWS) {
	ax = x, ay = y;
	if (sliderPtr->leftArrow != NULL) {
	    aw = Blt_Picture_Width(sliderPtr->leftArrow);
	    ah = Blt_Picture_Height(sliderPtr->leftArrow);
	    if (h > ah) {
		ay += (h - ah) / 2;
	    }
	    Tk_RedrawImage(IconBits(sliderPtr->leftArrow), 0, 0, aw, ah, 
			   drawable, ax, ay);
	    x += aw + PADX;
	}
	ax = width, ay = y;
	if (sliderPtr->rightArrow != NULL) {
	    aw = Blt_Picture_Width(sliderPtr->rightArrow);
	    ah = Blt_Picture_Height(sliderPtr->rightArrow);
	    if (h > ah) {
		ay += (h - ah) / 2;
	    }
	    ax -= aw + PADX;
	    Tk_RedrawImage(IconBits(sliderPtr->rightArrow), 0, 0, aw, ah, 
			   drawable, ax, ay);
	}
    }
    ih1 = ih2 = ih3 = ICON_HEIGHT;
    iw1 = iw2 = iw3 = ICON_WIDTH;
    if (sliderPtr->minIcon != NULL) {
	ih1 = Blt_Picture_Height(sliderPtr->minIcon);
	iw1 = Blt_Picture_Width(sliderPtr->minIcon);
    }
    if (sliderPtr->maxIcon != NULL) {
	ih2 = Blt_Picture_Height(sliderPtr->maxIcon);
	iw2 = Blt_Picture_Width(sliderPtr->maxIcon);
    }
    if (sliderPtr->nomIcon != NULL) {
	ih3 = Blt_Picture_Height(sliderPtr->nomIcon);
	iw3 = Blt_Picture_Width(sliderPtr->nomIcon);
    }
    ih = MAX3(ih1, ih2, ih2);
    iw = MAX3(iw1, iw2, iw3);

    x1 = x; x2 = x + th;
    DrawTrough(sliderPtr, drawable, x1, x2, y);
    throughLength = Tk_Width(sliderPtr->tkwin) - right - left - iw;
    left += miw

    if (h < (sliderPtr->sliderRadius * 2)) {
	h = sliderPtr->sliderRadius * 2;
    }
    if (sliderPtr->flags & SHOW_TICKS) {
	Blt_ChainLink link;
	TickLabel *labelPtr;

	    /* First tick. */
	    link = Blt_Chain_FirstLink(sliderPtr->ticks);
	    labelPtr = Blt_Chain_GetValue(link);
	    leftTickLabelWidth = labelPtr->width;
	    /* Last tick. */
	    link = Blt_Chain_LastLink(sliderPtr->ticks);
	    labelPtr = Blt_Chain_GetValue(link);
	    rightTickLabelWidth = labelPtr->width;

	    height += sliderPtr->axisHeight;
	}

	sliderPtr->leftOffset = MAX(leftButtonWidth,  leftTickLabelWidth / 2);
	sliderPtr->rightOffset = MAX(rightButtonWidth, rightTickLabelWidth / 2);
	left  += sliderPtr->leftOffset;
	right += sliderPtr->rightOffset;
	if (sliderPtr->flags & SHOW_VALUES) {
	    if (sliderPtr->tickAngle != 0.0f) {
		if (height < sliderPtr->minValHeight) {
		    height = sliderPtr->minValHeight;
		}
		if (height < sliderPtr->maxValHeight) {
		    height = sliderPtr->maxValHeight;
		}
	    }
	}
	if (sliderPtr->titleObjPtr != NULL) {
	    height += sliderPtr->titleObjPtr;
	}
	width = left + right + sliderPtr->maxTickValueWidth;
	height += sliderPtr->titleHeight;
	sliderPtr->troughLength = Tk_Width(sliderPtr->tkwin) - width;
	sliderPtr->screenMin = left;
	sliderPtr->screenRange = sliderPtr->troughLength;
	sliderPtr->normalHeight = height;
	sliderPtr->normalWidth = width;
    } else if (sliderPtr->flags & VERTICAL) {
	int top, bottom, width, height;

	top = bottom = sliderPtr->inset + sliderPtr->sliderRadius;
	width = height = 0;
	if (sliderPtr->flags & SHOW_ARROWS) {
	    if (sliderPtr->leftArrow != NULL) {
		top += Blt_Picture_Height(sliderPtr->leftArrow);
		if (width < Blt_Picture_Width(sliderPtr->leftArrow)) {
		    width = Blt_Picture_Width(sliderPtr->leftArrow);
		}
	    }
	    if (sliderPtr->rightArrow != NULL) {
		bottom += Blt_Picture_Height(sliderPtr->rightArrow);
		if (width < Blt_Picture_Width(sliderPtr-rightArrow)) {
		    width = Blt_Picture_Width(sliderPtr->rightArrow);
		}
	    }
	}
	if (width < (sliderPtr->sliderRadius * 2)) {
	    width = sliderPtr->sliderRadius * 2;
	}
	if (sliderPtr->flags & SHOW_TICKS) {
	    width += sliderPtr->axisWidth;
	}
	if (sliderPtr->flags & SHOW_VALUES) {
	    if (width < sliderPtr->minValWidth) {
		width = sliderPtr->minValWidth;
	    }
	    if (width < sliderPtr->maxValWidth) {
		width = sliderPtr->maxValWidth;
	    }
	}
	height = top + bottom + sliderPtr->titleHeight + 
	    sliderPtr->maxTickLabelHeight;
	sliderPtr->troughLength = Tk_Height(sliderPtr->tkwin) - height;
	sliderPtr->screenMin = top;
	sliderPtr->screenRange = sliderPtr->troughLength;
	sliderPtr->normalWidth = width;
	sliderPtr->normalHeight = height;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeVerticalLayout --
 *
 *      |h|b|g|tw|g|icon width|tl|g|text|g|b|h|
 *      |h|b|g|arr|g|min|slot|max|g|arr|g|b|h|
 *      |h|b||g|min|slot|max|g|b|h|
 *
 *      highlight thickness
 *      borderwidth
 *      gap
 *      min/max text height
 *      gap
 *      min/max/nom icon height
 *      tick length
 *      gap
 *      tick text height
 *      gap
 *      borderwidth
 *      higlight thickness
 *---------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 *
 * ComputeLayout --
 *
 *	Examines all the axes in the given margin and determines the area
 *	required to display them.
 *
 * Results:
 *	Returns the width or height of the margin, depending if it runs
 *	horizontally along the graph or vertically.
 *
 * Side Effects:
 *	The area width and height set in the margin.  Note again that this may
 *	be corrected later (mulitple axes) to adjust for the longest title in
 *	another margin.
 *
 *	|h|b|a|trough|a|b|h|
 *	     tick   tick
 *	|r|length|r| + blur + offset
 *---------------------------------------------------------------------------
 */
static int
ComputeLayout(Slider *sliderPtr)
{
    sliderPtr->flags &= ~LAYOUT_PENDING;
    if (sliderPtr->flags & HORIZONTAL) {
	int left, right, width, height;
	int leftButtonWidth, rightButtonWidth;

	left = right = sliderPtr->inset + sliderPtr->sliderRadius;
	width = height = 0;
	leftButtonWidth = rightButtonWidth = 0;
	if (sliderPtr->flags & SHOW_ARROWS) {
	    if (sliderPtr->leftArrow != NULL) {
		leftButtonWidth = Blt_Picture_Width(sliderPtr->leftArrow);
		if (height < Blt_Picture_Height(sliderPtr->leftArrow)) {
		    height = Blt_Picture_Height(sliderPtr->leftArrow);
		}
	    }
	    if (sliderPtr->rightArrow != NULL) {
		rightButtonWidth = Blt_Picture_Width(sliderPtr->rightArrow);
		if (height < Blt_Picture_Height(sliderPtr->rightArrow)) {
		    height = Blt_Picture_Height(sliderPtr->rightArrow);
		}
	    }
	}

	if (height < (sliderPtr->sliderRadius * 2)) {
	    height = sliderPtr->sliderRadius * 2;
	}
	if (sliderPtr->flags & SHOW_TICKS) {
	    Blt_ChainLink link;
	    TickLabel *labelPtr;

	    /* First tick. */
	    link = Blt_Chain_FirstLink(sliderPtr->ticks);
	    labelPtr = Blt_Chain_GetValue(link);
	    leftTickLabelWidth = labelPtr->width;
	    /* Last tick. */
	    link = Blt_Chain_LastLink(sliderPtr->ticks);
	    labelPtr = Blt_Chain_GetValue(link);
	    rightTickLabelWidth = labelPtr->width;

	    height += sliderPtr->axisHeight;
	}

	sliderPtr->leftOffset = MAX(leftButtonWidth,  leftTickLabelWidth / 2);
	sliderPtr->rightOffset = MAX(rightButtonWidth, rightTickLabelWidth / 2);
	left  += sliderPtr->leftOffset;
	right += sliderPtr->rightOffset;
	if (sliderPtr->flags & SHOW_VALUES) {
	    if (sliderPtr->tickAngle != 0.0f) {
		if (height < sliderPtr->minValHeight) {
		    height = sliderPtr->minValHeight;
		}
		if (height < sliderPtr->maxValHeight) {
		    height = sliderPtr->maxValHeight;
		}
	    }
	}
	if (sliderPtr->titleObjPtr != NULL) {
	    height += sliderPtr->titleObjPtr;
	}
	width = left + right + sliderPtr->maxTickValueWidth;
	height += sliderPtr->titleHeight;
	sliderPtr->troughLength = Tk_Width(sliderPtr->tkwin) - width;
	sliderPtr->screenMin = left;
	sliderPtr->screenRange = sliderPtr->troughLength;
	sliderPtr->normalHeight = height;
	sliderPtr->normalWidth = width;
    } else if (sliderPtr->flags & VERTICAL) {
	int top, bottom, width, height;

	top = bottom = sliderPtr->inset + sliderPtr->sliderRadius;
	width = height = 0;
	if (sliderPtr->flags & SHOW_ARROWS) {
	    if (sliderPtr->leftArrow != NULL) {
		top += Blt_Picture_Height(sliderPtr->leftArrow);
		if (width < Blt_Picture_Width(sliderPtr->leftArrow)) {
		    width = Blt_Picture_Width(sliderPtr->leftArrow);
		}
	    }
	    if (sliderPtr->rightArrow != NULL) {
		bottom += Blt_Picture_Height(sliderPtr->rightArrow);
		if (width < Blt_Picture_Width(sliderPtr-rightArrow)) {
		    width = Blt_Picture_Width(sliderPtr->rightArrow);
		}
	    }
	}
	if (width < (sliderPtr->sliderRadius * 2)) {
	    width = sliderPtr->sliderRadius * 2;
	}
	if (sliderPtr->flags & SHOW_TICKS) {
	    width += sliderPtr->axisWidth;
	}
	if (sliderPtr->flags & SHOW_VALUES) {
	    if (width < sliderPtr->minValWidth) {
		width = sliderPtr->minValWidth;
	    }
	    if (width < sliderPtr->maxValWidth) {
		width = sliderPtr->maxValWidth;
	    }
	}
	height = top + bottom + sliderPtr->titleHeight + 
	    sliderPtr->maxTickLabelHeight;
	sliderPtr->troughLength = Tk_Height(sliderPtr->tkwin) - height;
	sliderPtr->screenMin = top;
	sliderPtr->screenRange = sliderPtr->troughLength;
	sliderPtr->normalWidth = width;
	sliderPtr->normalHeight = height;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * LayoutSlider --
 *
 *	Calculate the layout of the graph.  Based upon the data, axis limits,
 *	X and Y titles, and title height, determine the cavity left which is
 *	the plotting surface.  The first step get the data and axis limits for
 *	calculating the space needed for the top, bottom, left, and right
 *	margins.
 *
 * 	1) The LEFT margin is the area from the left border to the Y axis 
 *	   (not including ticks). It composes the border width, the width an 
 *	   optional Y axis label and its padding, and the tick numeric labels. 
 *	   The Y axis label is rotated 90 degrees so that the width is the 
 *	   font height.
 *
 * 	2) The RIGHT margin is the area from the end of the graph
 *	   to the right window border. It composes the border width,
 *	   some padding, the font height (this may be dubious. It
 *	   appears to provide a more even border), the max of the
 *	   legend width and 1/2 max X tick number. This last part is
 *	   so that the last tick label is not clipped.
 *
 *           Window Width
 *      ___________________________________________________________
 *      |          |                               |               |
 *      |          |   TOP  height of title        |               |
 *      |          |                               |               |
 *      |          |           x2 title            |               |
 *      |          |                               |               |
 *      |          |        height of x2-axis      |               |
 *      |__________|_______________________________|_______________|  W
 *      |          | -plotpady                     |               |  i
 *      |__________|_______________________________|_______________|  n
 *      |          | top                   right   |               |  d
 *      |          |                               |               |  o
 *      |   LEFT   |                               |     RIGHT     |  w
 *      |          |                               |               |
 *      | y        |     Free area = 104%          |      y2       |  H
 *      |          |     Plotting surface = 100%   |               |  e
 *      | t        |     Tick length = 2 + 2%      |      t        |  i
 *      | i        |                               |      i        |  g
 *      | t        |                               |      t  legend|  h
 *      | l        |                               |      l   width|  t
 *      | e        |                               |      e        |
 *      |    height|                               |height         |
 *      |       of |                               | of            |
 *      |    y-axis|                               |y2-axis        |
 *      |          |                               |               |
 *      |          |origin 0,0                     |               |
 *      |__________|_left_________________bottom___|_______________|
 *      |          |-plotpady                      |               |
 *      |__________|_______________________________|_______________|
 *      |          | (xoffset, yoffset)            |               |
 *      |          |                               |               |
 *      |          |       height of x-axis        |               |
 *      |          |                               |               |
 *      |          |   BOTTOM   x title            |               |
 *      |__________|_______________________________|_______________|
 *
 * 3) The TOP margin is the area from the top window border to the top
 *    of the graph. It composes the border width, twice the height of
 *    the title font (if one is given) and some padding between the
 *    title.
 *
 * 4) The BOTTOM margin is area from the bottom window border to the
 *    X axis (not including ticks). It composes the border width, the height
 *    an optional X axis label and its padding, the height of the font
 *    of the tick labels.
 *
 * The plotting area is between the margins which includes the X and Y axes
 * including the ticks but not the tick numeric labels. The length of the
 * ticks and its padding is 5% of the entire plotting area.  Hence the entire
 * plotting area is scaled as 105% of the width and height of the area.
 *
 * The axis labels, ticks labels, title, and legend may or may not be
 * displayed which must be taken into account.
 *
 * if reqWidth > 0 : set outer size
 * if reqPlotWidth > 0 : set plot size
 *---------------------------------------------------------------------------
 */
static void
LayoutSlider(Slider *sliderPtr)
{
    int left, right, top, bottom;
    int plotWidth, plotHeight;
    int inset, inset2;
    int width, height;
    int pad;

    width = sliderPtr->width;
    height = sliderPtr->height;

    /* 
     * Step 1:  Compute the amount of space needed to display the axes
     *		associated with each margin.  They can be overridden by 
     *		-leftmargin, -rightmargin, -bottommargin, and -topmargin
     *		graph options, respectively.
     */
    left   = GetMarginGeometry(sliderPtr, &sliderPtr->leftMargin);
    right  = GetMarginGeometry(sliderPtr, &sliderPtr->rightMargin);
    top    = GetMarginGeometry(sliderPtr, &sliderPtr->topMargin);
    bottom = GetMarginGeometry(sliderPtr, &sliderPtr->bottomMargin);

    pad = sliderPtr->bottomMarginPtr->maxAxisLabelWidth;
    if (pad < sliderPtr->topMarginPtr->maxAxisLabelWidth) {
	pad = sliderPtr->topMarginPtr->maxAxisLabelWidth;
    }
    pad = pad / 2 + 3;
    if (right < pad) {
	right = pad;
    }
    if (left < pad) {
	left = pad;
    }
    pad = sliderPtr->leftMarginPtr->maxAxisLabelHeight;
    if (pad < sliderPtr->rightMarginPtr->maxAxisLabelHeight) {
	pad = sliderPtr->rightMarginPtr->maxAxisLabelHeight;
    }
    pad = pad / 2;
    if (top < pad) {
	top = pad;
    }
    if (bottom < pad) {
	bottom = pad;
    }

    if (sliderPtr->leftMarginPtr->reqSize > 0) {
	left = sliderPtr->leftMarginPtr->reqSize;
    }
    if (sliderPtr->rightMarginPtr->reqSize > 0) {
	right = sliderPtr->rightMarginPtr->reqSize;
    }
   if (sliderPtr->topMarginPtr->reqSize > 0) {
	top = sliderPtr->topMarginPtr->reqSize;
    }
    if (sliderPtr->bottomMarginPtr->reqSize > 0) {
	bottom = sliderPtr->bottomMarginPtr->reqSize;
    }

    /* 
     * Step 2:  Add the graph title height to the top margin. 
     */
    if (sliderPtr->title != NULL) {
	top += sliderPtr->titleHeight + 6;
    }
    inset = (sliderPtr->inset + sliderPtr->plotBW);
    inset2 = 2 * inset;

    /* 
     * Step 3: Estimate the size of the plot area from the remaining
     *	       space.  This may be overridden by the -plotwidth and
     *	       -plotheight graph options.  We use this to compute the
     *	       size of the legend. 
     */
    if (width == 0) {
	width = 400;
    }
    if (height == 0) {
	height = 400;
    }
    plotWidth  = (sliderPtr->reqPlotWidth > 0) ? sliderPtr->reqPlotWidth :
	width - (inset2 + left + right); /* Plot width. */
    plotHeight = (sliderPtr->reqPlotHeight > 0) ? sliderPtr->reqPlotHeight : 
	height - (inset2 + top + bottom); /* Plot height. */
    Blt_MapLegend(sliderPtr, plotWidth, plotHeight);

    /* 
     * Step 4:  Add the legend to the appropiate margin. 
     */
    if (!Blt_Legend_IsHidden(sliderPtr)) {
	switch (Blt_Legend_Site(sliderPtr)) {
	case LEGEND_RIGHT:
	    right += Blt_Legend_Width(sliderPtr) + 2;
	    break;
	case LEGEND_LEFT:
	    left += Blt_Legend_Width(sliderPtr) + 2;
	    break;
	case LEGEND_TOP:
	    top += Blt_Legend_Height(sliderPtr) + 2;
	    break;
	case LEGEND_BOTTOM:
	    bottom += Blt_Legend_Height(sliderPtr) + 2;
	    break;
	case LEGEND_XY:
	case LEGEND_PLOT:
	case LEGEND_WINDOW:
	    /* Do nothing. */
	    break;
	}
    }
#ifdef notdef
    if (sliderPtr->colorbarPtr == colormapPtrcolorbar.colormapPtr ) {
    right += Blt_Colorbar_Geometry(sliderPtr, plotWidth, plotHeight);
#endif
    /* 
     * Recompute the plotarea or graph size, now accounting for the legend. 
     */
    if (sliderPtr->reqPlotWidth == 0) {
	plotWidth = width  - (inset2 + left + right);
	if (plotWidth < 1) {
	    plotWidth = 1;
	}
    }
    if (sliderPtr->reqPlotHeight == 0) {
	plotHeight = height - (inset2 + top + bottom);
	if (plotHeight < 1) {
	    plotHeight = 1;
	}
    }
#ifdef notdef
    if (sliderPtr->colorbar.site == sliderPtr->legend.site) {
	Blt_GetColorbarGeometry(plotWidth, plotHeight);
    } 
#endif
    /*
     * Step 5: If necessary, correct for the requested plot area aspect
     *	       ratio.
     */
    if ((sliderPtr->reqPlotWidth == 0) && (sliderPtr->reqPlotHeight == 0) && 
	(sliderPtr->aspect > 0.0f)) {
	float ratio;

	/* 
	 * Shrink one dimension of the plotarea to fit the requested
	 * width/height aspect ratio.
	 */
	ratio = (float)plotWidth / (float)plotHeight;
	if (ratio > sliderPtr->aspect) {
	    int sw;

	    /* Shrink the width. */
	    sw = (int)(plotHeight * sliderPtr->aspect);
	    if (sw < 1) {
		sw = 1;
	    }
	    /* Add the difference to the right margin. */
	    /* CHECK THIS: w = sw; */
	    right += (plotWidth - sw);
	} else {
	    int sh;

	    /* Shrink the height. */
	    sh = (int)(plotWidth / sliderPtr->aspect);
	    if (sh < 1) {
		sh = 1;
	    }
	    /* Add the difference to the top margin. */
	    /* CHECK THIS: h = sh; */
	    top += (plotHeight - sh); 
	}
    }

    /* 
     * Step 6: If there are multiple axes in a margin, the axis titles will be
     *	       displayed in the adjoining margins.  Make sure there's room 
     *	       for the longest axis titles.
     */
    if (top < sliderPtr->leftMarginPtr->axesTitleLength) {
	top = sliderPtr->leftMarginPtr->axesTitleLength;
    }
    if (right < sliderPtr->bottomMarginPtr->axesTitleLength) {
	right = sliderPtr->bottomMarginPtr->axesTitleLength;
    }
    if (top < sliderPtr->rightMarginPtr->axesTitleLength) {
	top = sliderPtr->rightMarginPtr->axesTitleLength;
    }
    if (right < sliderPtr->topMarginPtr->axesTitleLength) {
	right = sliderPtr->topMarginPtr->axesTitleLength;
    }

    /* 
     * Step 7: Override calculated values with requested margin sizes.
     */
    if (sliderPtr->leftMarginPtr->reqSize > 0) {
	left = sliderPtr->leftMarginPtr->reqSize;
    }
    if (sliderPtr->rightMarginPtr->reqSize > 0) {
	right = sliderPtr->rightMarginPtr->reqSize;
    }
    if (sliderPtr->topMarginPtr->reqSize > 0) {
	top = sliderPtr->topMarginPtr->reqSize;
    }
    if (sliderPtr->bottomMarginPtr->reqSize > 0) {
	bottom = sliderPtr->bottomMarginPtr->reqSize;
    }
    if (sliderPtr->reqPlotWidth > 0) {	
	int w;

	/* 
	 * Width of plotarea is constained.  If there's extra space, add it to
	 * th left and/or right margins.  If there's too little, grow the
	 * graph width to accomodate it.
	 */
	w = plotWidth + inset2 + left + right;
	if (width > w) {		/* Extra space in window. */
	    int extra;

	    extra = (width - w) / 2;
	    if (sliderPtr->leftMarginPtr->reqSize == 0) { 
		left += extra;
		if (sliderPtr->rightMarginPtr->reqSize == 0) { 
		    right += extra;
		} else {
		    left += extra;
		}
	    } else if (sliderPtr->rightMarginPtr->reqSize == 0) {
		right += extra + extra;
	    }
	} else if (width < w) {
	    width = w;
	}
    } 
    if (sliderPtr->reqPlotHeight > 0) {	/* Constrain the plotarea height. */
	int h;

	/* 
	 * Height of plotarea is constained.  If there's extra space, 
	 * add it to th top and/or bottom margins.  If there's too little,
	 * grow the graph height to accomodate it.
	 */
	h = plotHeight + inset2 + top + bottom;
	if (height > h) {		/* Extra space in window. */
	    int extra;

	    extra = (height - h) / 2;
	    if (sliderPtr->topMarginPtr->reqSize == 0) { 
		top += extra;
		if (sliderPtr->bottomMarginPtr->reqSize == 0) { 
		    bottom += extra;
		} else {
		    top += extra;
		}
	    } else if (sliderPtr->bottomMarginPtr->reqSize == 0) {
		bottom += extra + extra;
	    }
	} else if (height < h) {
	    height = h;
	}
    }	
    sliderPtr->width  = width;
    sliderPtr->height = height;
    sliderPtr->left   = left + inset;
    sliderPtr->top    = top + inset;
    sliderPtr->right  = width - right - inset;
    sliderPtr->bottom = height - bottom - inset;

    sliderPtr->leftMarginPtr->width    = left   + sliderPtr->inset;
    sliderPtr->rightMarginPtr->width   = right  + sliderPtr->inset;
    sliderPtr->topMarginPtr->height    = top    + sliderPtr->inset;
    sliderPtr->bottomMarginPtr->height = bottom + sliderPtr->inset;
	    
    sliderPtr->vOffset = sliderPtr->top + sliderPtr->padTop;
    sliderPtr->vRange  = plotHeight - PADDING(sliderPtr->yPad);
    sliderPtr->hOffset = sliderPtr->left + sliderPtr->padLeft;
    sliderPtr->hRange  = plotWidth  - PADDING(sliderPtr->xPad);

    if (sliderPtr->vRange < 1) {
	sliderPtr->vRange = 1;
    }
    if (sliderPtr->hRange < 1) {
	sliderPtr->hRange = 1;
    }
    sliderPtr->hScale = 1.0f / (float)sliderPtr->hRange;
    sliderPtr->vScale = 1.0f / (float)sliderPtr->vRange;

    /*
     * Calculate the placement of the graph title so it is centered within the
     * space provided for it in the top margin
     */
    sliderPtr->titleY = 3 + sliderPtr->inset;
    sliderPtr->titleX = (sliderPtr->right + sliderPtr->left) / 2;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureSlider --
 *
 *	Configures axis attributes (font, line width, label, etc).
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 * Side Effects:
 *	Axis layout is deferred until the height and width of the window are
 *	known.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureSlider(Axis *sliderPtr)
{
    float angle;

    /* Determine is the axis is increasing or decreasing. */
    if (sliderPtr->fromValue > sliderPtr->toValue) {
	sliderPtr->flags |= DECREASING;
	sliderPtr->outerMin = sliderPtr->toValue;
	sliderPtr->outerMax = sliderPtr->fromValue;
    } else {
	sliderPtr->flags &= ~DECREASING;
	sliderPtr->outerMin = sliderPtr->fromValue;
	sliderPtr->outerMax = sliderPtr->toValue;
    }
    /* Handle special case of non-positive log values.  */
    if ((sliderPtr->flags & LOGSCALE) && (sliderPtr->outerMin < 0.0)) {
	sliderPtr->outerMin = 0.001;
    }
    SetSliderRange(&sliderPtr->outerRange, sliderPtr->outerMin, 
		   sliderPtr->outerMax);

    /* Bound the min and max values to the limits of the axis. */
    if (sliderPtr->minValue < sliderPtr->outerMin) {
	sliderPtr->minValue = sliderPtr->outerMin;
    }
    if (sliderPtr->maxValue > sliderPtr->outerMax) {
	sliderPtr->maxValue = sliderPtr->outerMax;
    }
    if (sliderPtr->innerMin > sliderPtr->innerMax) {
	sliderPtr->innerMax = sliderPtr->innerMin;
    }
    if (sliderPtr->innerMax < sliderPtr->innerMin) {
	sliderPtr->innerMin = sliderPtr->innerMax;
    }
    angle = FMOD(sliderPtr->tickAngle, 360.0f);
    if (angle < 0.0f) {
	angle += 360.0f;
    }
    if (sliderPtr->normalBg != NULL) {
	Blt_Bg_SetChangedProc(sliderPtr->normalBg, UpdateSlider, sliderPtr);
    }
    if (sliderPtr->activeBg != NULL) {
	Blt_Bg_SetChangedProc(sliderPtr->activeBg, UpdateSlider, sliderPtr);
    }
    sliderPtr->tickAngle = angle;
    ResetTextStyles(sliderPtr);

    sliderPtr->titleWidth = sliderPtr->titleHeight = 0;
    if (sliderPtr->title != NULL) {
	unsigned int w, h;

	Blt_GetTextExtents(sliderPtr->titleFont, 0, sliderPtr->title,-1,&w,&h);
	sliderPtr->titleWidth = (unsigned short int)w;
	sliderPtr->titleHeight = (unsigned short int)h;
    }
    ResetAxes(sliderPtr);
    sliderPtr->flags |= REDRAW_WORLD;
    sliderPtr->flags |= MAP_WORLD | RESET_AXES;
    sliderPtr->flags |= DIRTY;
    EventuallyRedraw(sliderPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewAxis --
 *
 *	Create and initialize a structure containing information to display
 *	a graph axis.
 *
 * Results:
 *	The return value is a pointer to an Axis structure.
 *
 *---------------------------------------------------------------------------
 */
static Slider *
NewSlider(const char *name, int margin)
{
    Axis *sliderPtr;
    Blt_HashEntry *hPtr;
    int isNew;

    if (name[0] == '-') {
	Tcl_AppendResult(sliderPtr->interp, "name of axis \"", name, 
		"\" can't start with a '-'", (char *)NULL);
	return NULL;
    }
    hPtr = Blt_CreateHashEntry(&sliderPtr->axes.nameTable, name, &isNew);
    if (!isNew) {
	sliderPtr = Blt_GetHashValue(hPtr);
	if ((sliderPtr->flags & DELETED) == 0) {
	    Tcl_AppendResult(sliderPtr->interp, "axis \"", name,
		"\" already exists in \"", Tk_PathName(sliderPtr->tkwin), "\"",
		(char *)NULL);
	    return NULL;
	}
	sliderPtr->flags &= ~DELETED;
    } else {
	sliderPtr = Blt_Calloc(1, sizeof(Axis));
	if (sliderPtr == NULL) {
	    Tcl_AppendResult(sliderPtr->interp, 
		"can't allocate memory for axis \"", name, "\"", (char *)NULL);
	    return NULL;
	}
	sliderPtr->obj.name = Blt_AssertStrdup(name);
	sliderPtr->hashPtr = hPtr;
	sliderPtr->looseMin = sliderPtr->looseMax = TIGHT;
	sliderPtr->reqNumMinorTicks = 2;
	sliderPtr->reqNumMajorTicks = 4 /*10*/;
	sliderPtr->margin = MARGIN_NONE;
	sliderPtr->tickLength = 8;
	sliderPtr->scrollUnits = 10;
	sliderPtr->reqMin = sliderPtr->reqMax = Blt_NaN();
	sliderPtr->reqScrollMin = sliderPtr->reqScrollMax = Blt_NaN();
	sliderPtr->weight = 1.0;
	sliderPtr->flags = (SHOWTICKS|GRIDMINOR|AUTO_MAJOR|
			  AUTO_MINOR | EXTERIOR);
	if (sliderPtr->classId == CID_ELEM_BAR) {
	    sliderPtr->flags |= GRID;
	}
	if ((sliderPtr->classId == CID_ELEM_BAR) && 
	    ((margin == MARGIN_TOP) || (margin == MARGIN_BOTTOM))) {
	    sliderPtr->reqStep = 1.0;
	    sliderPtr->reqNumMinorTicks = 0;
	} 
	if ((margin == MARGIN_RIGHT) || (margin == MARGIN_TOP)) {
	    sliderPtr->flags |= HIDDEN;
	}
	Blt_Ts_InitStyle(sliderPtr->limitsTextStyle);
	sliderPtr->tickLabels = Blt_Chain_Create();
	sliderPtr->lineWidth = 1;
	Blt_SetHashValue(hPtr, sliderPtr);
    }
    return sliderPtr;
}


int
Blt_DefaultAxes(Graph *sliderPtr)
{
    int i, margin;
    int flags;

    flags = Blt_GraphType(sliderPtr);
    for (margin = 0; margin < 4; margin++) {
	Blt_Chain chain;
	Axis *sliderPtr;

	chain = Blt_Chain_Create();
	sliderPtr->axisChain[margin] = chain;

	/* Create a default axis for each chain. */
	sliderPtr = NewAxis(sliderPtr, axisNames[margin].name, margin);
	if (sliderPtr == NULL) {
	    return TCL_ERROR;
	}
	sliderPtr->refCount = 1;	/* Default axes are assumed in use. */
	sliderPtr->margin = margin;
	sliderPtr->flags |= USE;
	Blt_GraphSetObjectClass(&sliderPtr->obj, axisNames[margin].classId);
	if (Blt_ConfigureComponentFromObj(sliderPtr->interp, sliderPtr->tkwin,
		sliderPtr->obj.name, "Axis", configSpecs, 0, (Tcl_Obj **)NULL,
		(char *)sliderPtr, flags) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (ConfigureAxis(sliderPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	sliderPtr->link = Blt_Chain_Append(chain, sliderPtr);
	sliderPtr->chain = chain;
    }
    /* The extra axes are not attached to a specific margin. */
    for (i = 4; i < numAxisNames; i++) {
	Axis *sliderPtr;

	sliderPtr = NewAxis(sliderPtr, axisNames[i].name, MARGIN_NONE);
	if (sliderPtr == NULL) {
	    return TCL_ERROR;
	}
	sliderPtr->refCount = 1;		
	sliderPtr->margin = MARGIN_NONE;
	sliderPtr->flags |= USE;
	Blt_GraphSetObjectClass(&sliderPtr->obj, axisNames[i].classId);
	if (Blt_ConfigureComponentFromObj(sliderPtr->interp, sliderPtr->tkwin,
		sliderPtr->obj.name, "Axis", configSpecs, 0, (Tcl_Obj **)NULL,
		(char *)sliderPtr, flags) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (ConfigureAxis(sliderPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ActivateOp --
 *
 * 	Activates the axis, drawing the axis with its -activeforeground,
 *	-activebackgound, -activerelief attributes.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Slider will be redrawn to reflect the new axis attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	   Tcl_Obj *const *objv)
{
    Slider *sliderPtr = clientData;
    const char *string;

    string = Tcl_GetString(objv[2]);
    if (string[0] == 'a') {
	sliderPtr->flags |= ACTIVE;
    } else {
	sliderPtr->flags &= ~ACTIVE;
    }
    Blt_EventuallyRedraw(sliderPtr);
    return TCL_OK;
}

/*
 * --------------------------------------------------------------------------
 *
 * BindOp --
 *
 *    .g axis bind axisName sequence command
 *
 *---------------------------------------------------------------------------
 */
static int
BindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Slider *sliderPtr = clientData;

    return Blt_ConfigureBindingsFromObj(interp, sliderPtr->bindTable,
	Blt_MakeAxisTag(sliderPtr, sliderPtr->name), objc, objv);
}
	  
/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *	Queries slider attributes (font, line width, label, etc).
 *
 * Results:
 *	Return value is a standard TCL result.  If querying configuration
 *	values, interp->result will contain the results.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Slider *sliderPtr = clientData;

    return Blt_ConfigureValueFromObj(interp, sliderPtr->tkwin, configSpecs,
	(char *)sliderPtr, objv[0], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *	Queries or resets axis attributes (font, line width, label, etc).
 *
 * Results:
 *	Return value is a standard TCL result.  If querying configuration
 *	values, interp->result will contain the results.
 *
 * Side Effects:
 *	Axis resources are possibly allocated (GC, font). Axis layout is
 *	deferred until the height and width of the window are known.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    Slider *sliderPtr = clientData;

    if (objc == 0) {
	return Blt_ConfigureInfoFromObj(interp, sliderPtr->tkwin, configSpecs,
	    (char *)sliderPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 1) {
	return Blt_ConfigureInfoFromObj(interp, sliderPtr->tkwin, configSpecs,
	    (char *)sliderPtr, objv[0], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, sliderPtr->tkwin, configSpecs, 
	objc, objv, (char *)sliderPtr, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (ConfigureSlider(sliderPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_EventuallyRedraw(sliderPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * LimitsOp --
 *
 *	This procedure returns a string representing the axis limits
 *	of the graph.  The format of the string is { left top right bottom}.
 *
 * Results:
 *	Always returns TCL_OK.  The interp->result field is
 *	a list of the slider axis limits.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
LimitsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv)
{
    Slider *sliderPtr = clientData;
    Tcl_Obj *listObjPtr;
    double min, max;

    if (IsLogScale(sliderPtr)) {
	min = EXP10(sliderPtr->axisRange.min);
	max = EXP10(sliderPtr->axisRange.max);
    } else {
	min = sliderPtr->axisRange.min;
	max = sliderPtr->axisRange.max;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(min));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(max));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InvTransformOp --
 *
 *	Maps the given window coordinate into an axis-value.
 *
 * Results:
 *	Returns a standard TCL result.  interp->result contains
 *	the axis value. If an error occurred, TCL_ERROR is returned
 *	and interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InvTransformOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Slider *sliderPtr = clientData;
    double y;				/* Real graph coordinate */
    int sy;				/* Integer window coordinate*/

    if (Tcl_GetIntFromObj(interp, objv[0], &sy) != TCL_OK) {
	return TCL_ERROR;
    }
    if (sliderPtr->flags & HORIZONTAL) {
	y = InvHMap(sliderPtr, (double)sy);
    } else {
	y = InvVMap(sliderPtr, (double)sy);
    }
    Tcl_SetDoubleObj(Tcl_GetObjResult(interp), y);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TransformOp --
 *
 *	Maps the given axis-value to a window coordinate.
 *
 * Results:
 *	Returns a standard TCL result.  interp->result contains
 *	the window coordinate. If an error occurred, TCL_ERROR
 *	is returned and interp->result will contain an error
 *	message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TransformOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    Slider *sliderPtr = clientData;
    double x;

    if (Blt_ExprDoubleFromObj(interp, objv[0], &x) != TCL_OK) {
	return TCL_ERROR;
    }
    if (sliderPtr->flags & HORIZONTAL) {
	x = HMap(sliderPtr, x);
    } else {
	x = VMap(sliderPtr, x);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), (int)x);
    return TCL_OK;
}

static int
ViewOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Slider *sliderPtr = clientData;
    double axisOffset, axisScale;
    double fract;
    double viewMin, viewMax, worldMin, worldMax;
    double viewWidth, worldWidth;

    worldMin = sliderPtr->valueRange.min;
    worldMax = sliderPtr->valueRange.max;
    viewMin = sliderPtr->min;
    viewMax = sliderPtr->max;
    /* Bound the view within scroll region. */ 
    if (viewMin < worldMin) {
	viewMin = worldMin;
    } 
    if (viewMax > worldMax) {
	viewMax = worldMax;
    }
    if (IsLogScale(sliderPtr)) {
	worldMin = log10(worldMin);
	worldMax = log10(worldMax);
	viewMin  = log10(viewMin);
	viewMax  = log10(viewMax);
    }
    worldWidth = worldMax - worldMin;
    viewWidth  = viewMax - viewMin;

    /* Unlike horizontal axes, vertical axis values run opposite of the
     * scrollbar first/last values.  So instead of pushing the axis minimum
     * around, we move the maximum instead. */
    isHoriz = sliderPtr->flags & HORIZONTAL ? 1 : 0;
    isDecreasing = sliderPtr->flags & DECREASING ? 1 : 0;
    if (isHoriz != isDecreasing) {
	axisOffset  = viewMin - worldMin;
	axisScale = sliderPtr->hScale;
    } else {
	axisOffset  = worldMax - viewMax;
	axisScale = sliderPtr->vScale;
    }
    if (objc == 4) {
	Tcl_Obj *listObjPtr;
	double first, last;

	first = Clamp(axisOffset / worldWidth);
	last = Clamp((axisOffset + viewWidth) / worldWidth);
	listObjPtr = Tcl_NewListObj(0, NULL);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(first));
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewDoubleObj(last));
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    fract = axisOffset / worldWidth;
    if (GetScrollInfo(interp, objc, objv, &fract, 
	viewWidth / worldWidth, sliderPtr->scrollUnits, axisScale) != TCL_OK) {
	return TCL_ERROR;
    }
    isHoriz = sliderPtr->flags & HORIZONTAL ? 1 : 0;
    isDecreasing = sliderPtr->flags & DECREASING ? 1 : 0;
    if (isHoriz != isDecreasing) {
	sliderPtr->reqMin = (fract * worldWidth) + worldMin;
	sliderPtr->reqMax = sliderPtr->reqMin + viewWidth;
    } else {
	sliderPtr->reqMax = worldMax - (fract * worldWidth);
	sliderPtr->reqMin = sliderPtr->reqMax - viewWidth;
    }
    if (IsLogScale(sliderPtr)) {
	sliderPtr->reqMin = EXP10(sliderPtr->reqMin);
	sliderPtr->reqMax = EXP10(sliderPtr->reqMax);
    }
    sliderPtr->flags |= (GEOMETRY | LAYOUT);
    Blt_EventuallyRedraw(sliderPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisActivateOp --
 *
 * 	Activates the axis, drawing the axis with its -activeforeground,
 *	-activebackgound, -activerelief attributes.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Sliders will be redrawn to reflect the new axis attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisActivateOp(Tcl_Interp *interp, Slider *sliderPtr, int objc, 
	       Tcl_Obj *const *objv)
{
    Axis *sliderPtr;

    if (GetAxisFromObj(interp, sliderPtr, objv[3], &sliderPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return ActivateOp(interp, sliderPtr, objc, objv);
}


/*-------------------------------------------------------------------------------
 *
 * AxisBindOp --
 *
 *    .g axis bind axisName sequence command
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AxisBindOp(Tcl_Interp *interp, Graph *sliderPtr, int objc, 
	      Tcl_Obj *const *objv)
{
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch cursor;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	for (hPtr = Blt_FirstHashEntry(&sliderPtr->axes.bindTagTable, &cursor);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    const char *tagName;
	    Tcl_Obj *objPtr;

	    tagName = Blt_GetHashKey(&sliderPtr->axes.bindTagTable, hPtr);
	    objPtr = Tcl_NewStringObj(tagName, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    return Blt_ConfigureBindingsFromObj(interp, sliderPtr->bindTable, 
	Blt_MakeAxisTag(sliderPtr, Tcl_GetString(objv[3])), objc - 4, objv + 4);
}


/*
 *---------------------------------------------------------------------------
 *
 * AxisCgetOp --
 *
 *	Queries axis attributes (font, line width, label, etc).
 *
 * Results:
 *	Return value is a standard TCL result.  If querying configuration
 *	values, interp->result will contain the results.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
AxisCgetOp(Tcl_Interp *interp, Graph *sliderPtr, int objc, Tcl_Obj *const *objv)
{
    Axis *sliderPtr;

    if (GetAxisFromObj(interp, sliderPtr, objv[3], &sliderPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return CgetOp(interp, sliderPtr, objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisConfigureOp --
 *
 *	Queries or resets axis attributes (font, line width, label, etc).
 *
 * Results:
 *	Return value is a standard TCL result.  If querying configuration
 *	values, interp->result will contain the results.
 *
 * Side Effects:
 *	Axis resources are possibly allocated (GC, font). Axis layout is
 *	deferred until the height and width of the window are known.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisConfigureOp(Tcl_Interp *interp, Graph *sliderPtr, int objc, 
		Tcl_Obj *const *objv)
{
    Tcl_Obj *const *options;
    int i;
    int numNames, numOpts;

    /* Figure out where the option value pairs begin */
    objc -= 3;
    objv += 3;
    for (i = 0; i < objc; i++) {
	Axis *sliderPtr;
	const char *string;

	string = Tcl_GetString(objv[i]);
	if (string[0] == '-') {
	    break;
	}
	if (GetAxisFromObj(interp, sliderPtr, objv[i], &sliderPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    numNames = i;				/* Number of pen names specified */
    numOpts = objc - i;			/* Number of options specified */
    options = objv + i;			/* Start of options in objv  */

    for (i = 0; i < numNames; i++) {
	Axis *sliderPtr;

	if (GetAxisFromObj(interp, sliderPtr, objv[i], &sliderPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (ConfigureOp(interp, sliderPtr, numOpts, options) != TCL_OK) {
	    break;
	}
    }
    if (i < numNames) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisDeleteOp --
 *
 *	Deletes one or more axes.  The actual removal may be deferred until the
 *	axis is no longer used by any element. The axis can't be referenced by
 *	its name any longer and it may be recreated.
 *
 * Results:
 *	Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AxisDeleteOp(Tcl_Interp *interp, Graph *sliderPtr, int objc, 
	     Tcl_Obj *const *objv)
{
    int i;

    for (i = 3; i < objc; i++) {
	Axis *sliderPtr;

	if (GetAxisFromObj(interp, sliderPtr, objv[i], &sliderPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	sliderPtr->flags |= DELETED;
	if (sliderPtr->refCount == 0) {
	    DestroyAxis(sliderPtr);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisFocusOp --
 *
 * 	Activates the axis, drawing the axis with its -activeforeground,
 *	-activebackgound, -activerelief attributes.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	Graph will be redrawn to reflect the new axis attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisFocusOp(Tcl_Interp *interp, Graph *sliderPtr, int objc, Tcl_Obj *const *objv)
{
    if (objc > 3) {
	Axis *sliderPtr;
	const char *string;

	sliderPtr = NULL;
	string = Tcl_GetString(objv[3]);
	if ((string[0] != '\0') && 
	    (GetAxisFromObj(interp, sliderPtr, objv[3], &sliderPtr) != TCL_OK)) {
	    return TCL_ERROR;
	}
	sliderPtr->focusPtr = NULL;
	if ((sliderPtr != NULL) && 
	    ((sliderPtr->flags & (USE|HIDDEN)) == USE)) {
	    sliderPtr->focusPtr = sliderPtr;
	}
	Blt_SetFocusItem(sliderPtr->bindTable, sliderPtr->focusPtr, NULL);
    }
    /* Return the name of the axis that has focus. */
    if (sliderPtr->focusPtr != NULL) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
		sliderPtr->focusPtr->obj.name, -1);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * AxisGetOp --
 *
 *    Returns the name of the picked axis (using the axis bind operation).
 *    Right now, the only name accepted is "current".
 *
 * Results:
 *    A standard TCL result.  The interpreter result will contain the name of
 *    the axis.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AxisGetOp(Tcl_Interp *interp, Graph *sliderPtr, int objc, Tcl_Obj *const *objv)
{
    GraphObj *objPtr;

    objPtr = Blt_GetCurrentItem(sliderPtr->bindTable);
    /* Report only on axes. */
    if ((objPtr != NULL) && (!objPtr->deleted) &&
	((objPtr->classId == CID_AXIS_X) || (objPtr->classId == CID_AXIS_Y) || 
	 (objPtr->classId == CID_NONE))) {
	char c;
	char  *string;

	string = Tcl_GetString(objv[3]);
	c = string[0];
	if ((c == 'c') && (strcmp(string, "current") == 0)) {
	    Tcl_SetStringObj(Tcl_GetObjResult(interp), objPtr->name,-1);
	} else if ((c == 'd') && (strcmp(string, "detail") == 0)) {
	    Axis *sliderPtr;
	    
	    sliderPtr = (Axis *)objPtr;
	    Tcl_SetStringObj(Tcl_GetObjResult(interp), sliderPtr->detail, -1);
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisInvTransformOp --
 *
 *	Maps the given window coordinate into an axis-value.
 *
 * Results:
 *	Returns a standard TCL result.  interp->result contains the axis
 *	value. If an error occurred, TCL_ERROR is returned and interp->result
 *	will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisInvTransformOp(Tcl_Interp *interp, Graph *sliderPtr, int objc, 
		   Tcl_Obj *const *objv)
{
    Axis *sliderPtr;

    if (GetAxisFromObj(interp, sliderPtr, objv[3], &sliderPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return InvTransformOp(interp, sliderPtr, objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisLimitsOp --
 *
 *	This procedure returns a string representing the axis limits of the
 *	graph.  The format of the string is { left top right bottom}.
 *
 * Results:
 *	Always returns TCL_OK.  The interp->result field is
 *	a list of the graph axis limits.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisLimitsOp(Tcl_Interp *interp, Graph *sliderPtr, int objc, 
	     Tcl_Obj *const *objv)
{
    Axis *sliderPtr;

    if (GetAxisFromObj(interp, sliderPtr, objv[3], &sliderPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return LimitsOp(interp, sliderPtr, objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisMarginOp --
 *
 *	This procedure returns a string representing the axis limits of the
 *	graph.  The format of the string is "left top right bottom".
 *
 * Results:
 *	Always returns TCL_OK.  The interp->result field is a list of the
 *	graph axis limits.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisMarginOp(Tcl_Interp *interp, Graph *sliderPtr, int objc, 
	     Tcl_Obj *const *objv)
{
    Axis *sliderPtr;

    if (GetAxisFromObj(interp, sliderPtr, objv[3], &sliderPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return MarginOp(interp, sliderPtr, objc - 4, objv + 4);
}


/*
 *---------------------------------------------------------------------------
 *
 * AxisNamesOp --
 *
 *	Return a list of the names of all the axes.
 *
 * Results:
 *	Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
AxisNamesOp(Tcl_Interp *interp, Graph *sliderPtr, int objc, Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch cursor;

	for (hPtr = Blt_FirstHashEntry(&sliderPtr->axes.nameTable, &cursor);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    Axis *sliderPtr;

	    sliderPtr = Blt_GetHashValue(hPtr);
	    if (sliderPtr->flags & DELETED) {
		continue;
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		     Tcl_NewStringObj(sliderPtr->obj.name, -1));
	}
    } else {
	Blt_HashEntry *hPtr;
	Blt_HashSearch cursor;

	for (hPtr = Blt_FirstHashEntry(&sliderPtr->axes.nameTable, &cursor);
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    Axis *sliderPtr;
	    int i;

	    sliderPtr = Blt_GetHashValue(hPtr);
	    for (i = 3; i < objc; i++) {
		const char *pattern;

		pattern = Tcl_GetString(objv[i]);
		if (Tcl_StringMatch(sliderPtr->obj.name, pattern)) {
		    Tcl_Obj *objPtr;

		    objPtr = Tcl_NewStringObj(sliderPtr->obj.name, -1);
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
 * AxisTransformOp --
 *
 *	Maps the given axis-value to a window coordinate.
 *
 * Results:
 *	Returns the window coordinate via interp->result.  If an error occurred,
 *	TCL_ERROR is returned and interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
TransformOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    Axis *sliderPtr = clientData;

    return TransformOp(interp, sliderPtr, objc - 4, objv + 4);
}

static Blt_OpSpec virtAxisOps[] = {
    {"activate",     1, AxisActivateOp,     4, 4, "what"},
    {"bind",         1, AxisBindOp,         3, 6, "sequence command"},
    {"cget",         2, AxisCgetOp,         5, 5, "option"},
    {"configure",    2, AxisConfigureOp,    4, 0, "?option value?..."},
    {"deactivate",   3, AxisActivateOp,     4, 4, "what"},
    {"invtransform", 1, AxisInvTransformOp, 4, 4, "value"},
    {"limits",       1, AxisLimitsOp,       3, 3, ""},
    {"transform",    2, TransformOp,        4, 4, "value"},
    {"view",         1, AxisViewOp,         4, 7, "?moveto fract? "
	"?scroll number what?"},
};
static int numVirtAxisOps = sizeof(virtAxisOps) / sizeof(Blt_OpSpec);

int
Blt_VirtualAxisOp(Graph *sliderPtr, Tcl_Interp *interp, int objc, 
		  Tcl_Obj *const *objv)
{
    GraphVirtualAxisProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numVirtAxisOps, virtAxisOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (interp, sliderPtr, objc, objv);
    return result;
}

static Blt_OpSpec axisOps[] = {
    {"activate",     1, ActivateOp,     3, 3, "",},
    {"bind",         1, BindOp,         2, 5, "sequence command",},
    {"cget",         2, CgetOp,         4, 4, "option",},
    {"configure",    2, ConfigureOp,    3, 0, "?option value?...",},
    {"deactivate",   1, ActivateOp,     3, 3, "",},
    {"invtransform", 1, InvTransformOp, 4, 4, "value",},
    {"limits",       1, LimitsOp,       3, 3, "",},
    {"transform",    1, TransformOp,    4, 4, "value",},
    {"view",         1, ViewOp,         3, 6, "?moveto fract? ",},
};

static int numAxisOps = sizeof(axisOps) / sizeof(Blt_OpSpec);

int
Blt_AxisOp(Tcl_Interp *interp, Graph *sliderPtr, int margin, int objc,
	   Tcl_Obj *const *objv)
{
    int result;
    GraphAxisProc *proc;

    proc = Blt_GetOpFromObj(interp, numAxisOps, axisOps, BLT_OP_ARG2, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    if (proc == UseOp) {
	lastMargin = margin;		/* Set global variable to the margin
					 * in the argument list. Needed only
					 * for UseOp. */
	result = (*proc)(interp, (Axis *)sliderPtr, objc - 3, objv + 3);
    } else {
	Axis *sliderPtr;

	sliderPtr = Blt_GetFirstAxis(sliderPtr->margins[margin].axes);
	if (sliderPtr == NULL) {
	    return TCL_OK;
	}
	result = (*proc)(interp, sliderPtr, objc - 3, objv + 3);
    }
    return result;
}

Axis *
Blt_GetFirstAxis(Blt_Chain chain)
{
    Blt_ChainLink link;

    link = Blt_Chain_FirstLink(chain);
    if (link == NULL) {
	return NULL;
    }
    return Blt_Chain_GetValue(link);
}

Axis *
Blt_NearestAxis(Graph *sliderPtr, int x, int y)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    
    for (hPtr = Blt_FirstHashEntry(&sliderPtr->axes.nameTable, &cursor); 
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Axis *sliderPtr;

	sliderPtr = Blt_GetHashValue(hPtr);
	if ((sliderPtr->flags & (DELETED|HIDDEN|USE)) != USE) {
	    continue;
	}
	if (sliderPtr->flags & SHOWTICKS) {
	    Blt_ChainLink link;

	    for (link = Blt_Chain_FirstLink(sliderPtr->tickLabels); link != NULL; 
		 link = Blt_Chain_NextLink(link)) {	
		TickLabel *labelPtr;
		Point2d t;
		double rw, rh;
		Point2d bbox[5];

		labelPtr = Blt_Chain_GetValue(link);
		Blt_GetBoundingBox(labelPtr->width, labelPtr->height, 
			sliderPtr->tickAngle, &rw, &rh, bbox);
		t = Blt_AnchorPoint(labelPtr->anchorPos.x, 
			labelPtr->anchorPos.y, rw, rh, sliderPtr->tickAnchor);
		t.x = x - t.x - (rw * 0.5);
		t.y = y - t.y - (rh * 0.5);

		bbox[4] = bbox[0];
		if (Blt_PointInPolygon(&t, bbox, 5)) {
		    sliderPtr->detail = "label";
		    return sliderPtr;
		}
	    }
	}
	if (sliderPtr->title != NULL) {	/* and then the title string. */
	    Point2d bbox[5];
	    Point2d t;
	    double rw, rh;
	    unsigned int w, h;

	    Blt_GetTextExtents(sliderPtr->titleFont, 0, sliderPtr->title,-1,&w,&h);
	    Blt_GetBoundingBox(w, h, sliderPtr->titleAngle, &rw, &rh, bbox);
	    t = Blt_AnchorPoint(sliderPtr->titlePos.x, sliderPtr->titlePos.y, 
		rw, rh, sliderPtr->titleAnchor);
	    /* Translate the point so that the 0,0 is the upper left 
	     * corner of the bounding box.  */
	    t.x = x - t.x - (rw * 0.5);
	    t.y = y - t.y - (rh * 0.5);
	    
	    bbox[4] = bbox[0];
	    if (Blt_PointInPolygon(&t, bbox, 5)) {
		sliderPtr->detail = "title";
		return sliderPtr;
	    }
	}
	if (sliderPtr->lineWidth > 0) {	/* Check for the axis region */
	    if ((x <= sliderPtr->right) && (x >= sliderPtr->left) && 
		(y <= sliderPtr->bottom) && (y >= sliderPtr->top)) {
		sliderPtr->detail = "line";
		return sliderPtr;
	    }
	}
    }
    return NULL;
}
 
ClientData
Blt_MakeAxisTag(Graph *sliderPtr, const char *tagName)
{
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&sliderPtr->axes.bindTagTable, tagName, &isNew);
    return Blt_GetHashKey(&sliderPtr->axes.bindTagTable, hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TimeAxis --
 *
 * 	Determine the units of a linear scaled axis.
 *
 *	The axis limits are either the range of the data values mapped
 *	to the axis (autoscaled), or the values specified by the -min
 *	and -max options (manual).
 *
 *	If autoscaled, the smallest and largest major ticks will
 *	encompass the range of data values.  If the -loose option is
 *	selected, the next outer ticks are choosen.  If tight, the
 *	ticks are at or inside of the data limits are used.
 *
 * 	If manually set, the ticks are at or inside the data limits
 * 	are used.  This makes sense for zooming.  You want the
 * 	selected range to represent the next limit, not something a
 * 	bit bigger.
 *
 *	Note: I added an "always" value to the -loose option to force
 *	      the manually selected axes to be loose. It's probably
 *	      not a good idea.
 *
 *          maxY
 *            |    units = magnitude (of least significant digit)
 *            |    high  = largest unit tick < max axis value
 *      high _|    low   = smallest unit tick > min axis value
 *            |
 *            |    range = high - low
 *            |    # ticks = greatest factor of range/units
 *           _|
 *        U   |
 *        n   |
 *        i   |
 *        t  _|
 *            |
 *            |
 *            |
 *       low _|
 *            |
 *            |_minX________________maxX__
 *            |   |       |      |       |
 *     minY  low                        high
 *           minY
 *
 * 	numTicks = Number of ticks
 * 	min = Minimum value of axis
 * 	max = Maximum value of axis
 * 	range    = Range of values (max - min)
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The axis tick information is set.  The actual tick values will
 *	be generated later.
 *
 *---------------------------------------------------------------------------
 */
static void
TimeAxis(Axis *sliderPtr, double min, double max)
{
#ifdef notdef
    double step;
    double tickMin, tickMax;
    double axisMin, axisMax;
    unsigned int numTicks;

    range = max - min;

#define SECONDS(x)	(x)
#define MINUTES(x)	((x) * 60)
#define HOURS(x)	((x) * 60 * 60)
#define DAYS(x)		((x) * 60 * 60 * 24)
#define MONTHS(x)	((x) * 60 * 60 * 24 * 30)
#define YEARS(x)	((x) * 60 * 60 * 24 * 365)
    div = numTicks - 1;
    if (range > (MONTHS(6) * div)) {
	unit = TICK_YEAR;
    } else if (range > (MONTHS(3) * div)) {
	unit = TICKS_6MONTH;
	first = timefloor(min, unit);
	last = timeceil(max, unit);
    } else if (range > (MONTHS(2) * div)) {
	unit = TICKS_3MONTH;
	first = timefloor(min, unit);
	last = timeceil(max, unit);
    } else if (range > (MONTHS(1) * div)) {
	unit = TICKS_2MONTH;
	first = timefloor(min, unit);
	last = timeceil(max, unit);
    } else if (range > (DAYS(15) * div)) {
	unit = TICKS_1MONTH;
    } else if (range > (DAYS(10) * div)) {
	unit = TICKS_15DAY;
    } else if (range > (DAYS(1) * div)) {
	unit = TICKS_10DAY;
    } else if (range > (HOURS(12) * div)) {
	unit = TICKS_1DAY;
    } else if (range > (HOURS(6) * div)) {
	unit = TICKS_12HOUR;
    } else if (range > (HOURS(3) * div)) {
	unit = TICKS_6HOUR;
    } else if (range > (HOURS(1) * div)) {
	unit = TICKS_3HOUR;
    } else if (range > (MINUTES(30) * div)) {
	unit = TICKS_HOUR;
    } else if (range > (MINUTES(20) * div)) {
	unit = TICKS_30MIN;
    } else if (range > (MINUTES(15) * div)) {
	unit = TICKS_20MIN;
    } else if (range > (MINUTES(10) * div)) {
	unit = TICKS_15MIN;
    } else if (range > (MINUTES(1) * div)) {
	unit = TICKS_10MIN;
    } else if (range > (SECONDS(30) * div)) {
	unit = TICKS_1MIN;
    } else if (range > (SECONDS(20) * div)) {
	unit = TICKS_30SEC;
    } else if (range > (SECONDS(15) * div)) {
	unit = TICKS_20SEC;
    } else if (range > (SECONDS(10) * div)) {
	unit = TICKS_15SEC;
    } else if (range > (SECONDS(1) * div)) {
	unit = TICKS_10SEC;
    } else {
	unit = TICKS_1SEC;
    }

    } else {
	unit = TICKS_SECS;
    }
    numTicks = 0;
    step = 1.0;
    /* Suppress compiler warning. */
    axisMin = axisMax = tickMin = tickMax = Blt_NaN();
    if (min < max) {
	double range;

	range = max - min;
	/* Calculate the major tick stepping. */
	if (sliderPtr->reqStep > 0.0) {
	    /* An interval was designated by the user.  Keep scaling it until
	     * it fits comfortably within the current range of the axis.  */
	    step = sliderPtr->reqStep;
	    while ((2 * step) >= range) {
		step *= 0.5;
	    }
	} else {
	    range = NiceNum(range, 0);
	    step = NiceNum(range / sliderPtr->reqNumMajorTicks, 1);
	}
	
	/* Find the outer tick values. Add 0.0 to prevent getting -0.0. */
	axisMin = tickMin = floor(min / step) * step + 0.0;
	axisMax = tickMax = ceil(max / step) * step + 0.0;
	
	numTicks = Round((tickMax - tickMin) / step) + 1;
    } 
    sliderPtr->major.step = step;
    sliderPtr->major.initial = tickMin;
    sliderPtr->major.numSteps = numTicks;

    /*
     * The limits of the axis are either the range of the data ("tight") or at
     * the next outer tick interval ("loose").  The looseness or tightness has
     * to do with how the axis fits the range of data values.  This option is
     * overridden when the user sets an axis limit (by either -min or -max
     * option).  The axis limit is always at the selected limit (otherwise we
     * assume that user would have picked a different number).
     */
    if ((sliderPtr->looseMin == TIGHT) || ((sliderPtr->looseMin == LOOSE) &&
	 (DEFINED(sliderPtr->reqMin)))) {
	axisMin = min;
    }
    if ((sliderPtr->looseMax == TIGHT) || ((sliderPtr->looseMax == LOOSE) &&
	 (DEFINED(sliderPtr->reqMax)))) {
	axisMax = max;
    }
    SetAxisRange(&sliderPtr->axisRange, axisMin, axisMax);

    /* Now calculate the minor tick step and number. */

    if ((sliderPtr->reqNumMinorTicks > 0) && (sliderPtr->flags & AUTO_MAJOR)) {
	numTicks = sliderPtr->reqNumMinorTicks - 1;
	step = 1.0 / (numTicks + 1);
    } else {
	numTicks = 0;			/* No minor ticks. */
	step = 0.5;			/* Don't set the minor tick interval to
					 * 0.0. It makes the GenerateTicks
					 * routine * create minor log-scale tick
					 * marks.  */
    }
    sliderPtr->minor.initial = sliderPtr->minor.step = step;
    sliderPtr->minor.numSteps = numTicks;
#endif
}

#ifdef notdef
static Ticks *
TimeGenerateTicks(TickSweep *sweepPtr)
{
    Ticks *ticksPtr;

    ticksPtr = Blt_AssertMalloc(sizeof(Ticks) + 
	(sweepPtr->numSteps * sizeof(double)));
    ticksPtr->numTicks = 0;

    if (sweepPtr->step == 0.0) { 
	/* Hack: A zero step indicates to use log values. */
	int i;
	/* Precomputed log10 values [1..10] */
	static double logTable[] = {
	    0.0, 
	    0.301029995663981, 
	    0.477121254719662, 
	    0.602059991327962, 
	    0.698970004336019, 
	    0.778151250383644, 
	    0.845098040014257,
	    0.903089986991944, 
	    0.954242509439325, 
	    1.0
	};
	for (i = 0; i < sweepPtr->numSteps; i++) {
	    ticksPtr->values[i] = logTable[i];
	}
    } else {
	double value;
	int i;
    
	value = sweepPtr->initial;	/* Start from smallest axis tick */
	for (i = 0; i < sweepPtr->numSteps; i++) {
	    value = UROUND(value, sweepPtr->step);
	    ticksPtr->values[i] = value;
	    value += sweepPtr->step;
	}
    }
    ticksPtr->numTicks = sweepPtr->numSteps;
    return ticksPtr;
}

static double
TimeFloor(double min, int unit)
{
    unsigned long ticks;

    ticks = (long)floor(min);
    localtime_r(&ticks, &tm);
    switch(unit) {
	case TICK_6MONTHS:
	    tm.sec = 0;
	    tm.min = 0;
	    tm.day = 0;
	    tm.
    }
}
static double
TimeCeil(double max, int unit)
{
    
}

#endif


/*
 *---------------------------------------------------------------------------
 *
 * SliderInstCmdProc --
 *
 *	This procedure is invoked to process the TCL command that
 *	corresponds to a widget managed by this module.  See the user
 *	documentation for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec sliderOps[] =
{
    {"activate",     1, ActivateOp,     4, 4, "what"},
    {"bind",         1, BindOp,         3, 6, "sequence command"},
    {"cget",         2, CgetOp,         5, 5, "option"},
    {"configure",    2, ConfigureOp,    4, 0, "?option value?..."},
    {"coords",       2, CoordsOp,       4, 0, "?value?"},
    {"deactivate",   1, ActivateOp,     4, 4, "what"},
    {"get",          1, GetOp,          2, 3, "?x y?"},
    {"identify",     2, IdentifyOp,     4, 4, "x y"},
    {"invtransform", 2, InvTransformOp, 4, 4, "value"},
    {"limits",       1, LimitsOp,       3, 3, ""},
    {"range",        1, RangeOp,        3, 3, ""},
    {"move",         2, MoveOp,         3, 3, "min|max|both x y"},
    {"max",          2, MaxOp,          2, 3, "?value?"},
    {"min",          2, MinOp,          2, 3, "?value?"},
    {"set",          2, SetOp,          3, 3, "value"},
    {"transform",    2, TransformOp,    4, 4, "value"},
    {"view",         1, ViewOp,         4, 7, "?moveto fract? "
	"?scroll number what?"},
};
static int numSliderOps = sizeof(sliderOps) / sizeof(Blt_OpSpec);

static int
SliderInstCmdProc(ClientData clientData, Tcl_Interp *interp, int objc,
		  Tcl_Obj *const *objv)
{
    Slider *sliderPtr = clientData;
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numSliderOps, sliderOps, BLT_OP_ARG1, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    Tcl_Preserve(sliderPtr);
    result = (*proc) (sliderPtr, interp, objc, objv);
    Tcl_Release(sliderPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * SliderInstDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If the
 *	widget isn't already in the process of being destroyed, this command
 *	destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *---------------------------------------------------------------------------
 */
static void
SliderInstDeletedProc(ClientData clientData) /* Pointer to widget record. */
{
    Slider *sliderPtr = clientData;

    if (sliderPtr->tkwin != NULL) {	/* NULL indicates window has already
					 * been destroyed. */
	Tk_Window tkwin;

	tkwin = sliderPtr->tkwin;
	sliderPtr->tkwin = NULL;
	Blt_DeleteWindowInstanceData(tkwin);
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NewSlider --
 *
 *	This procedure creates and initializes a new slider widget.
 *
 * Results:
 *	The return value is a pointer to a structure describing the new
 *	widget.  If an error occurred, then the return value is NULL and an
 *	error message is left in interp->result.
 *
 * Side effects:
 *	Memory is allocated, a Tk_Window is created, etc.
 *
 *---------------------------------------------------------------------------
 */
static Slider *
NewSlider(Tcl_Interp *interp, Tk_Window tkwin)
{
    Slider *sliderPtr;

    sliderPtr = Blt_AssertCalloc(1, sizeof(Slider));
    sliderPtr->tkwin = tkwin;
    sliderPtr->display = Tk_Display(tkwin);
    sliderPtr->interp = interp;
    sliderPtr->borderWidth = 2;
    sliderPtr->highlightWidth = 2;
    sliderPtr->relief = TK_RELIEF_FLAT;
    sliderPtr->flags = RESET_WORLD;
    Blt_Ts_InitStyle(sliderPtr->titleTextStyle);
    Blt_Ts_SetAnchor(sliderPtr->titleTextStyle, TK_ANCHOR_N);
    return sliderPtr;
}


/* 
 *
 * |h|b|lo|r|length|r|ro|b|h|
 * 
 * Redraw trough if
 * 1) widget background color changes.
 * 2) trough color changes.
 * 3) hide/show arrow buttons.
 * 4) radius of min/max controls change.
 * 5) window width changes.
 * 6) min/max tick label width changes.
 */
static void
DrawHorizontalTrough(Slider *sliderPtr, Drawable drawable, int x, int y,
		     int w, int h)
{
    int x, y, w, h, r;

#define TROUGH_RADIUS 9
#define TROUGH_BLUR_WIDTH 3
    r = TROUGH_RADIUS;
    if (sliderPtr->troughPicture == NULL) {
	Blt_Picture picture;
	Blt_PaintBrush brush;
	int x, y, w, h;
	unsigned int normal, light, dark;

	picture = Blt_CreatePicture(sliderPtr->troughWidth, 
		sliderPtr->troughHeight);
	/* Fill the background of the picture */
	brushPtr = Blt_Bg_PaintBrush(sliderPtr->normalBg);
	Blt_SetBrushOrigin(brushPtr, -x, -y);
	Blt_PaintRectangle(picture, 0, 0, sliderPtr->troughWidth,
		sliderPtr->troughHeight, 0, 0, brushPtr);
	/* Get the shadow colors based on the slider background, not
	 * the trough color. */
	Blt_GetShadowColors(sliderPtr->normalBg, &normal, &light, &dark);
	Blt_BlankPicture(picture, normal);

	x = y = 3;
	w = sliderPtr->troughWidth - 6;
	h = sliderPtr->troughHeight - 6;
	if (sliderPtr->flags & SHOW_ARROWS) {
	    x + sliderPtr->arrowWidth;
	    w -= 2 * sliderPtr->arrowWidth;
	}
	brush = Blt_NewColorBrush(light);
	/* Light. */
	Blt_PaintRectangle(picture, x-1, y-1, sliderPtr->troughWidth,
			   sliderPtr->troughHeight, r, 0, brush);
	/* Dark. */
	Blt_SetColorBrushColor(&brush, dark);
	Blt_PaintRectangle(picture, x-2, y-2, sliderPtr->troughWidth,
			   sliderPtr->troughHeight, r, 0, brush);
        Blt_FreeBrush(brush);
#ifdef notdef
	/* Blur */
	Blt_BlurPicture(picture, picture, 1, 3);
#endif
	/* Background. */
	brush = Blt_Bg_PaintBrush(sliderPtr->troughBg);
	Blt_PaintRectangle(picture, x, y, sliderPtr->troughWidth,
			   sliderPtr->troughHeight, r, 0, brush);
	sliderPtr->troughPicture = picture;
    }
    x = y = sliderPtr->inset;
    x += sliderPtr->leftOffset + sliderPtr->radius;
    y += (Tk_Height(sliderPtr->tkwin) - sliderPtr->troughHeight) / 2;
    Blt_PaintPicture(sliderPtr->troughPicture, drawable, picture, 0, 0,
	sliderPtr->troughWidth, sliderPtr->troughHeight, x, y, 0);
}

static void
DrawSliderControl(Slider *sliderPtr, Drawable drawable, int x, int y, int r, 
		  int minmax)
{
    Blt_PaintBrush brush;
    int light, dark, normal;

    if (sliderPtr->flags & DISABLED) {
	bg = sliderPtr->disabledSliderBg;
    } else if (sliderPtr->activeRegionPtr == &sliderPtr->leftArrowRegion) {
	bg = sliderPtr->activeSliderBg;
    } else {
	bg = sliderPtr->normalSliderBg;
    }
    Blt_GetShadowColors(bg, &normal, &light, &dark);
    brush = Blt_NewColorBrush(light);
    /* Light. */
    Blt_PaintRectangle(picture, x-1, y-1, sliderPtr->troughWidth,
		       sliderPtr->troughHeight, r, 0, brush);
    /* Dark. */
    Blt_SetColorBrushColor(brush, dark);
    Blt_PaintRectangle(picture, x-2, y-2, sliderPtr->troughWidth,
		       sliderPtr->troughHeight, r, 0, brush);
    
    Blt_FreeBrush(brush);
    /* Background. */
    brush = Blt_NewConicalGradientBrush();
    Blt_SetColorBrushColors(brush, light, dark);
    Blt_SetBrushRegion(brush, x, y, w, h);
    Blt_PaintRectangle(picture, x, y, sliderPtr->troughWidth,
		       sliderPtr->troughHeight, r, 0, brush);
    Blt_FreeBrush(brush);
}

static void
DrawHorizonalSlider(Slider *sliderPtr, Drawable drawable) 
{
    Blt_PaintBrush brush;
    int light, dark, normal;

    DrawHorizontalTrough(sliderPtr, pixmap);
    if (sliderPtr->flags & SHOW_TICKS) {
	DrawHorizontalAxis(sliderPtr, pixmap);
    }
    if (sliderPtr->flags & SHOW_VALUES) {
	DrawValues(sliderPtr, pixmap);
    }
    if (sliderPtr->flags & SHOW_ARROWS) {
	DrawArrows(sliderPtr, pixmap);
    }
    DrawSliders(sliderPtr, pixmap);
    
}

static void
DrawVerticalSlider(Slider *sliderPtr, Drawable drawable) 
{
    DrawVerticalTrough(sliderPtr, pixmap);

    /* Draw the  */
    if (sliderPtr->flags & SHOW_TICKS) {
	DrawAxis(sliderPtr, pixmap);
    }
    if (sliderPtr->flags & SHOW_VALUES) {
	DrawValues(sliderPtr, pixmap);
    }
    if (sliderPtr->flags & SHOW_ARROWS) {
	DrawArrows(sliderPtr, pixmap);
    }
    DrawSliders(sliderPtr, pixmap);
    
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayProc --
 *
 * 	This procedure is invoked to display the widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 * 	The widget is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayProc(ClientData clientData)	
{
    Slider *sliderPtr = clientData;
    Pixmap pixmap;
    int width, height;
    int redrawAll;
    
    redrawAll = sliderPtr->flags & REDRAW_ALL;
    sliderPtr->flags &= ~(REDRAW_PENDING | REDRAW_ALL);
    if (sliderPtr->tkwin == NULL) {
	return;				/* Window has been destroyed. */
    }
    if (sliderPtr->flags & GEOMETRY) {
	ComputeGeometry(sliderPtr);
    }
    if (sliderPtr->flags & LAYOUT_PENDING) {
	if (sliderPtr->flags & HORIZONTAL) {
	    ComputeHorizontalLayout(sliderPtr);
	} else {
	    ComputeVerticalLayout(sliderPtr);
	}
    }
    if ((sliderPtr->reqHeight == 0) || (sliderPtr->reqWidth == 0)) {
	if ((Tk_ReqWidth(sliderPtr->tkwin) != width) ||
	    (Tk_ReqHeight(sliderPtr->tkwin) != height)) {
	    Tk_GeometryRequest(sliderPtr->tkwin, width, height);
	}
    }
    if (!Tk_IsMapped(sliderPtr->tkwin)) {
	return;
    }
    w = Tk_Width(sliderPtr->tkwin);
    h = Tk_Height(sliderPtr->tkwin);
    if ((w < 1) || (h < 1)) {
	return;
    }

    pixmap = Tk_GetPixmap(sliderPtr->display, Tk_WindowId(sliderPtr->tkwin),
	w, h, Tk_Depth(sliderPtr->tkwin));

    /* Draw the background. */
    Blt_Bg_FillRectangle(sliderPtr->tkwin, pixmap, sliderPtr->bg, 0, 0, 
	w, h, sliderPtr->borderWidth, sliderPtr->relief);

    /* Draw the through. */
    if (sliderPtr->flags & HORIZONTAL) {
	DrawHorizontalSlider(sliderPtr, pixamp);
    } else {
	DrawVerticalSlider(sliderPtr, pixamp);
    }
    /* Draw focus highlight ring. */
    if ((sliderPtr->highlightWidth > 0) && (sliderPtr->flags & FOCUS)) {
	Tk_DrawFocusHighlight(sliderPtr->tkwin, sliderPtr->highlightGC, 
		sliderPtr->highlightWidth, pixmap);
    }
    XCopyArea(sliderPtr->display, pixmap, Tk_WindowId(sliderPtr->tkwin),
	0, 0, width, height, 0, 0);
    Tk_FreePixmap(sliderPtr->display, pixmap);
}


/*
 *---------------------------------------------------------------------------
 *
 * SliderCmd --
 *
 * 	This procedure is invoked to process the TCL command that corresponds
 * 	to a widget managed by this module. See the user documentation for
 * 	details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side Effects:
 *	See the user documentation.
 *
 *
 *	blt::tabset pathName ?option value?...
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
SliderCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
	  Tcl_Obj *const *objv)	
{
    Slider *sliderPtr;
    Tk_Window tkwin;
    unsigned int mask;
    const char *pathName;

    if (objc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    pathName = Tcl_GetString(objv[1]);
    tkwin = Tk_CreateWindowFromPath(interp, Tk_MainWindow(interp), pathName,
	(char *)NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    sliderPtr = NewSlider(interp, tkwin);
    Tk_SetClass(tkwin, "BltSlider");
    Blt_SetWindowInstanceData(tkwin, sliderPtr);
    if (Blt_ConfigureWidgetFromObj(interp, sliderPtr->tkwin, configSpecs, 
	objc - 2, objv + 2, (char *)sliderPtr, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    if (ConfigureSlider(sliderPtr) != TCL_OK) {
	Tk_DestroyWindow(sliderPtr->tkwin);
	return TCL_ERROR;
    }
    mask = (ExposureMask | StructureNotifyMask | FocusChangeMask);
    Tk_CreateEventHandler(tkwin, mask, SliderEventProc, sliderPtr);
    sliderPtr->cmdToken = Tcl_CreateObjCommand(interp, pathName, 
	SliderInstCmdProc, sliderPtr, SliderInstDeletedProc);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), pathName, -1);
    return TCL_OK;
}

int
Blt_SliderCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "slider", SliderCmd };

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#endif /* NO_SLIDER */

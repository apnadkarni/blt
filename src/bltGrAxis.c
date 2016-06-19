/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltGrAxis.c --
 *
 *      This module implements coordinate axes for the BLT graph widget.
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

#ifdef HAVE_FLOAT_H
  #include <float.h>
#endif /* HAVE_FLOAT_H */

#include "bltMath.h"

#include <X11/Xutil.h>
#include "bltAlloc.h"
#include "bltBind.h"
#include "bltPs.h"
#include "bltPaintBrush.h"
#include "bltPalette.h"
#include "bltPicture.h"
#include "bltBg.h"
#include "bltSwitch.h"
#include "bltTags.h"
#include "bltGraph.h"
#include "bltGrAxis.h"
#include "bltGrLegd.h"
#include "bltGrElem.h"

#define IsLeapYear(y) \
        ((((y) % 4) == 0) && ((((y) % 100) != 0) || (((y) % 400) == 0)))

/* Axis flags: */

#define AUTO_MAJOR      (1<<16)         /* Auto-generate major ticks. */
#define AUTO_MINOR      (1<<17)         /* Auto-generate minor ticks. */
#define GRID            (1<<19)         /* Display grid lines. */
#define GRIDMINOR       (1<<20)         /* Display grid lines for minor
                                         * ticks. */
#define TICKLABELS      (1<<21)         /* Display axis tick labels. */
#define EXTERIOR        (1<<22)         /* Axis is exterior to the plot. */
#define CHECK_LIMITS    (1<<23)         /* Validate user-defined axis
                                         * limits. */
#define LOGSCALE        (1<<24)
#define DECREASING      (1<<25)
#define COLORBAR        (1<<27)

#define MAXTICKS        10001

#define FCLAMP(x)       ((((x) < 0.0) ? 0.0 : ((x) > 1.0) ? 1.0 : (x)))

/*
 * Round x in terms of units
 */
#define UROUND(x,u)     (round((x)/(u))*(u))
#define UCEIL(x,u)      (ceil((x)/(u))*(u))
#define UFLOOR(x,u)     (floor((x)/(u))*(u))

#define NUMDIGITS       15              /* Specifies the number of digits
                                         * of accuracy used when outputting
                                         * axis tick labels. */
enum TickRange {
    TIGHT, LOOSE, ALWAYS_LOOSE
};


#define AXIS_PAD_TITLE          2       /* Padding for axis title. */
#define TICK_PAD                2
#define COLORBAR_PAD            4

#define HORIZONTAL(m)   (!((m)->side & 0x1)) /* Even sides are horizontal */


#define DEF_ACTIVEBACKGROUND    STD_ACTIVE_BACKGROUND
#define DEF_ACTIVEFOREGROUND    STD_ACTIVE_FOREGROUND
#define DEF_ACTIVERELIEF        "flat"
#define DEF_ANGLE               "0.0"
#define DEF_BACKGROUND          (char *)NULL
#define DEF_BORDERWIDTH         "0"
#define DEF_CHECKLIMITS         "0"
#define DEF_COLORBAR_THICKNESS  "20"
#define DEF_COMMAND             (char *)NULL
#define DEF_DECREASING          "0"
#define DEF_DIVISIONS           "10"
#define DEF_FOREGROUND          RGB_BLACK
#define DEF_GRIDCOLOR           RGB_GREY40
#define DEF_GRIDDASHES          "dot"
#define DEF_GRIDLINEWIDTH       "0"
#define DEF_GRIDMINOR           "1"
#define DEF_GRIDMINOR_COLOR     RGB_GREY77
#define DEF_GRID_BARCHART       "1"
#define DEF_GRID_GRAPH          "0"
#define DEF_HIDE                "0"
#define DEF_JUSTIFY             "c"
#define DEF_LIMITS_FONT         STD_FONT_NUMBERS
#define DEF_LIMITS_FORMAT       (char *)NULL
#define DEF_LINEWIDTH           "1"
#define DEF_LOGSCALE            "0"
#define DEF_LOOSE               "0"
#define DEF_PALETTE             (char *)NULL
#define DEF_RANGE               "0.0"
#define DEF_RELIEF              "flat"
#define DEF_SCALE               "linear"
#define DEF_SCROLL_INCREMENT    "10"
#define DEF_SHIFTBY             "0.0"
#define DEF_SHOWTICKLABELS           "1"
#define DEF_STEP                "0.0"
#define DEF_SUBDIVISIONS        "2"
#define DEF_TAGS                "all"
#define DEF_TICKFONT_BARCHART   STD_FONT_SMALL
#define DEF_TICKFONT_GRAPH      STD_FONT_NUMBERS
#define DEF_TICKLENGTH          "4"
#define DEF_TICK_ANCHOR         "c"
#define DEF_TICK_DIRECTION      "out"
#define DEF_TIMESCALE           "0"
#define DEF_TITLE_ALTERNATE     "0"
#define DEF_TITLE_FG            RGB_BLACK
#define DEF_TITLE_FONT          STD_FONT_NORMAL
#define DEF_WEIGHT              "1.0"

/* Indicates how to rotate axis title for each margin. */
static float titleAngle[4] = {
    0.0, 90.0, 0.0, 270.0
};

typedef struct {
    int axisLength;                     /* Length of the axis. */
    int t1;                             /* Length of a major tick (in
                                         * pixels). */
    int t2;                             /* Length of a minor tick (in
                                         * pixels). */
    int label;                          /* Distance from axis to tick
                                         * label. */
    int colorbar;
} AxisInfo;

typedef struct {
    const char *name;
    ClassId classId;
    int margin, invertMargin;
} AxisName;

static AxisName axisNames[] = { 
    { "x",  CID_AXIS_X, MARGIN_BOTTOM, MARGIN_LEFT   },
    { "y",  CID_AXIS_Y, MARGIN_LEFT,   MARGIN_BOTTOM },
    { "x2", CID_AXIS_X, MARGIN_TOP,    MARGIN_RIGHT  },
    { "y2", CID_AXIS_Y, MARGIN_RIGHT,  MARGIN_TOP    },
    { "z",  CID_AXIS_Z, MARGIN_NONE,   MARGIN_NONE   }
} ;
static int numAxisNames = sizeof(axisNames) / sizeof(AxisName);

static Blt_OptionParseProc ObjToLimit;
static Blt_OptionPrintProc LimitToObj;
Blt_CustomOption bltLimitOption = {
    ObjToLimit, LimitToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToTickDirection;
static Blt_OptionPrintProc TickDirectionToObj;
static Blt_CustomOption tickDirectionOption = {
    ObjToTickDirection, TickDirectionToObj, NULL, (ClientData)0
};

static Blt_OptionFreeProc  FreeTicks;
static Blt_OptionParseProc ObjToTicks;
static Blt_OptionPrintProc TicksToObj;
static Blt_CustomOption majorTicksOption = {
    ObjToTicks, TicksToObj, FreeTicks, (ClientData)AUTO_MAJOR,
};
static Blt_CustomOption minorTicksOption = {
    ObjToTicks, TicksToObj, FreeTicks, (ClientData)AUTO_MINOR,
};
static Blt_OptionFreeProc  FreeAxis;
static Blt_OptionPrintProc AxisToObj;
static Blt_OptionParseProc ObjToAxis;
Blt_CustomOption bltXAxisOption = {
    ObjToAxis, AxisToObj, FreeAxis, (ClientData)CID_AXIS_X
};
Blt_CustomOption bltYAxisOption = {
    ObjToAxis, AxisToObj, FreeAxis, (ClientData)CID_AXIS_Y
};
Blt_CustomOption bltZAxisOption = {
    ObjToAxis, AxisToObj, FreeAxis, (ClientData)CID_AXIS_Z
};

Blt_CustomOption bltAxisOption = {
    ObjToAxis, AxisToObj, FreeAxis, (ClientData)CID_NONE
};

static Blt_SwitchParseProc XAxisSwitch;
static Blt_SwitchParseProc YAxisSwitch;
static Blt_SwitchFreeProc FreeAxisSwitch;
Blt_SwitchCustom bltXAxisSwitch =
{
    XAxisSwitch, NULL, FreeAxisSwitch, (ClientData)0
};
Blt_SwitchCustom bltYAxisSwitch =
{
    YAxisSwitch, NULL, FreeAxisSwitch, (ClientData)0
};

static Blt_OptionParseProc ObjToScale;
static Blt_OptionPrintProc ScaleToObj;
static Blt_CustomOption scaleOption = {
    ObjToScale, ScaleToObj, NULL, (ClientData)0,
};

static Blt_OptionParseProc ObjToTimeScale;
static Blt_OptionPrintProc TimeScaleToObj;
static Blt_CustomOption timeScaleOption = {
    ObjToTimeScale, TimeScaleToObj, NULL, (ClientData)0,
};

static Blt_OptionParseProc ObjToLogScale;
static Blt_OptionPrintProc LogScaleToObj;
static Blt_CustomOption logScaleOption = {
    ObjToLogScale, LogScaleToObj, NULL, (ClientData)0,
};

static Blt_OptionFreeProc  FreeFormat;
static Blt_OptionParseProc ObjToFormat;
static Blt_OptionPrintProc FormatToObj;
static Blt_CustomOption formatOption = {
    ObjToFormat, FormatToObj, FreeFormat, (ClientData)0,
};
static Blt_OptionParseProc ObjToLoose;
static Blt_OptionPrintProc LooseToObj;
static Blt_CustomOption looseOption = {
    ObjToLoose, LooseToObj, NULL, (ClientData)0,
};

static Blt_OptionParseProc ObjToMargin;
static Blt_OptionPrintProc MarginToObj;
static Blt_CustomOption marginOption = {
    ObjToMargin, MarginToObj, NULL, (ClientData)0
};

static Blt_OptionFreeProc FreePalette;
static Blt_OptionParseProc ObjToPalette;
static Blt_OptionPrintProc PaletteToObj;
static Blt_CustomOption paletteOption =
{
    ObjToPalette, PaletteToObj, FreePalette, (ClientData)0
};

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-activebackground", "activeBackground", 
        "ActiveBackground", DEF_ACTIVEBACKGROUND, 
        Blt_Offset(Axis, activeBg), ALL_GRAPHS | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-activeforeground", "activeForeground",
        "ActiveForeground", DEF_ACTIVEFOREGROUND,
        Blt_Offset(Axis, activeFgColor), ALL_GRAPHS}, 
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "Relief",
        DEF_ACTIVERELIEF, Blt_Offset(Axis, activeRelief),
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT}, 
    {BLT_CONFIG_DOUBLE, "-autorange", "autoRange", "AutoRange",
        DEF_RANGE, Blt_Offset(Axis, windowSize),
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT}, 
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
        DEF_BACKGROUND, Blt_Offset(Axis, normalBg),
        ALL_GRAPHS | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_LISTOBJ, "-bindtags", "bindTags", "BindTags", DEF_TAGS, 
        Blt_Offset(Axis, obj.tagsObjPtr), ALL_GRAPHS | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 
        0, ALL_GRAPHS},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
        DEF_BORDERWIDTH, Blt_Offset(Axis, borderWidth),
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-checklimits", "checkLimits", "CheckLimits", 
        DEF_CHECKLIMITS, Blt_Offset(Axis, flags), 
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)CHECK_LIMITS},
    {BLT_CONFIG_COLOR, "-color", "color", "Color",
        DEF_FOREGROUND, Blt_Offset(Axis, tickColor), ALL_GRAPHS},
    {BLT_CONFIG_PIXELS_NNEG, "-colorbarthickness", "colorBarThickness", 
        "ColorBarThickness", DEF_COLORBAR_THICKNESS, 
        Blt_Offset(Axis, colorbar.thickness), 
        BLT_CONFIG_DONT_SET_DEFAULT | ALL_GRAPHS},
    {BLT_CONFIG_OBJ, "-command", "command", "Command", DEF_COMMAND, 
        Blt_Offset(Axis, fmtCmdObjPtr), BLT_CONFIG_NULL_OK | ALL_GRAPHS},
    {BLT_CONFIG_BOOLEAN, "-decreasing", "decreasing", "Decreasing",
        DEF_DECREASING, Blt_Offset(Axis, decreasing),
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-descending", "decreasing", (char *)NULL, 
        (char *)NULL, 0, ALL_GRAPHS},
    {BLT_CONFIG_INT, "-divisions", "division", "Divisions",
        DEF_DIVISIONS, Blt_Offset(Axis, reqNumMajorTicks),
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_SYNONYM, "-fg", "color", (char *)NULL, 
        (char *)NULL, 0, ALL_GRAPHS},
    {BLT_CONFIG_SYNONYM, "-foreground", "color", (char *)NULL, 
        (char *)NULL, 0, ALL_GRAPHS},
    {BLT_CONFIG_BITMASK, "-grid", "grid", "Grid", DEF_GRID_BARCHART, 
        Blt_Offset(Axis, flags), BARCHART, (Blt_CustomOption *)GRID},
    {BLT_CONFIG_BITMASK, "-grid", "grid", "Grid", DEF_GRID_GRAPH, 
        Blt_Offset(Axis, flags), GRAPH | STRIPCHART | CONTOUR, 
        (Blt_CustomOption *)GRID},
    {BLT_CONFIG_COLOR, "-gridcolor", "gridColor", "GridColor", 
        DEF_GRIDCOLOR, Blt_Offset(Axis, major.grid.color), ALL_GRAPHS},
    {BLT_CONFIG_DASHES, "-griddashes", "gridDashes", "GridDashes", 
        DEF_GRIDDASHES, Blt_Offset(Axis, major.grid.dashes), 
        BLT_CONFIG_NULL_OK | ALL_GRAPHS},
    {BLT_CONFIG_PIXELS_NNEG, "-gridlinewidth", "gridLineWidth", 
        "GridLineWidth", DEF_GRIDLINEWIDTH, 
        Blt_Offset(Axis, major.grid.lineWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT | ALL_GRAPHS},
    {BLT_CONFIG_BITMASK, "-gridminor", "gridMinor", "GridMinor", 
        DEF_GRIDMINOR, Blt_Offset(Axis, flags), 
        BLT_CONFIG_DONT_SET_DEFAULT | ALL_GRAPHS, 
        (Blt_CustomOption *)GRIDMINOR},
    {BLT_CONFIG_COLOR, "-gridminorcolor", "gridMinorColor", "GridColor", 
        DEF_GRIDMINOR_COLOR, Blt_Offset(Axis, minor.grid.color),ALL_GRAPHS},
    {BLT_CONFIG_DASHES, "-gridminordashes", "gridMinorDashes", "GridDashes", 
        DEF_GRIDDASHES, Blt_Offset(Axis, minor.grid.dashes), 
        BLT_CONFIG_NULL_OK | ALL_GRAPHS},
    {BLT_CONFIG_PIXELS_NNEG, "-gridminorlinewidth", "gridMinorLineWidth", 
        "GridLineWidth", DEF_GRIDLINEWIDTH, 
        Blt_Offset(Axis, minor.grid.lineWidth), 
        BLT_CONFIG_DONT_SET_DEFAULT | ALL_GRAPHS},
    {BLT_CONFIG_BITMASK, "-hide", "hide", "Hide", DEF_HIDE, 
        Blt_Offset(Axis, flags), ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, 
        (Blt_CustomOption *)HIDDEN},
    {BLT_CONFIG_JUSTIFY, "-justify", "justify", "Justify",
        DEF_JUSTIFY, Blt_Offset(Axis, titleJustify),
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-labeloffset", "labelOffset", "LabelOffset",
        (char *)NULL, Blt_Offset(Axis, labelOffset), ALL_GRAPHS}, 
    {BLT_CONFIG_COLOR, "-limitscolor", "limitsColor", "Color",
        DEF_FOREGROUND, Blt_Offset(Axis, limitsTextStyle.color), 
        ALL_GRAPHS},
    {BLT_CONFIG_FONT, "-limitsfont", "limitsFont", "Font", DEF_LIMITS_FONT,
        Blt_Offset(Axis, limitsTextStyle.font), ALL_GRAPHS},
    {BLT_CONFIG_CUSTOM, "-limitsformat", "limitsFormat", "LimitsFormat",
        DEF_LIMITS_FORMAT, Blt_Offset(Axis, limitsFmtsObjPtr),
        BLT_CONFIG_NULL_OK | ALL_GRAPHS, &formatOption},
    {BLT_CONFIG_PIXELS_NNEG, "-linewidth", "lineWidth", "LineWidth",
        DEF_LINEWIDTH, Blt_Offset(Axis, lineWidth),
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-logscale", "logScale", "LogScale", DEF_LOGSCALE, 0,
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, &logScaleOption},
    {BLT_CONFIG_CUSTOM, "-loose", "loose", "Loose", DEF_LOOSE, 0, 
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, &looseOption},
    {BLT_CONFIG_CUSTOM, "-majorticks", "majorTicks", "MajorTicks",
        (char *)NULL, Blt_Offset(Axis, major),
        BLT_CONFIG_NULL_OK | ALL_GRAPHS, &majorTicksOption},
    {BLT_CONFIG_CUSTOM, "-margin", "margin", "Margin", (char *)NULL, 0,
        ALL_GRAPHS, &marginOption},
    {BLT_CONFIG_CUSTOM, "-max", "max", "Max", (char *)NULL, 
        Blt_Offset(Axis, reqMax), ALL_GRAPHS, &bltLimitOption},
    {BLT_CONFIG_CUSTOM, "-min", "min", "Min", (char *)NULL, 
        Blt_Offset(Axis, reqMin), ALL_GRAPHS, &bltLimitOption},
    {BLT_CONFIG_CUSTOM, "-minorticks", "minorTicks", "MinorTicks",
        (char *)NULL, Blt_Offset(Axis, minor), 
        BLT_CONFIG_NULL_OK | ALL_GRAPHS, &minorTicksOption},
    {BLT_CONFIG_CUSTOM, "-palette", "palette", "Palette", DEF_PALETTE, 
        Blt_Offset(Axis, palette), ALL_GRAPHS, &paletteOption},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief",
        DEF_RELIEF, Blt_Offset(Axis, relief), 
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FLOAT, "-rotate", "rotate", "Rotate", DEF_ANGLE, 
        Blt_Offset(Axis, tickAngle), ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-scale", "scale", "Scale", DEF_SCALE, 0,
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, &scaleOption},
    {BLT_CONFIG_OBJ, "-scrollcommand", "scrollCommand", "ScrollCommand",
        (char *)NULL, Blt_Offset(Axis, scrollCmdObjPtr),
        ALL_GRAPHS | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_POS, "-scrollincrement", "scrollIncrement", 
        "ScrollIncrement", DEF_SCROLL_INCREMENT, 
        Blt_Offset(Axis, scrollUnits), ALL_GRAPHS|BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-scrollmax", "scrollMax", "ScrollMax", (char *)NULL, 
        Blt_Offset(Axis, reqScrollMax),  ALL_GRAPHS, &bltLimitOption},
    {BLT_CONFIG_CUSTOM, "-scrollmin", "scrollMin", "ScrollMin", (char *)NULL, 
        Blt_Offset(Axis, reqScrollMin), ALL_GRAPHS, &bltLimitOption},
    {BLT_CONFIG_DOUBLE, "-shiftby", "shiftBy", "ShiftBy",
        DEF_SHIFTBY, Blt_Offset(Axis, shiftBy),
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BITMASK, "-showticks", "showTicks", "ShowTicks",
        DEF_SHOWTICKLABELS, Blt_Offset(Axis, flags), 
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT,
        (Blt_CustomOption *)TICKLABELS},
    {BLT_CONFIG_DOUBLE, "-stepsize", "stepSize", "StepSize",
        DEF_STEP, Blt_Offset(Axis, reqStep),
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_INT, "-subdivisions", "subdivisions", "Subdivisions",
        DEF_SUBDIVISIONS, Blt_Offset(Axis, reqNumMinorTicks), ALL_GRAPHS},
    {BLT_CONFIG_ANCHOR, "-tickanchor", "tickAnchor", "Anchor",
        DEF_TICK_ANCHOR, Blt_Offset(Axis, reqTickAnchor), ALL_GRAPHS},
    {BLT_CONFIG_CUSTOM, "-tickdirection", "tickDirection", "TickDirectoin",
        DEF_TICK_DIRECTION, Blt_Offset(Axis, flags),
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, &tickDirectionOption},
    {BLT_CONFIG_FONT, "-tickfont", "tickFont", "Font",
        DEF_TICKFONT_GRAPH, Blt_Offset(Axis, tickFont), 
        GRAPH | STRIPCHART | CONTOUR},
    {BLT_CONFIG_FONT, "-tickfont", "tickFont", "Font",
        DEF_TICKFONT_BARCHART, Blt_Offset(Axis, tickFont), BARCHART},
    {BLT_CONFIG_PIXELS_NNEG, "-ticklength", "tickLength", "TickLength",
        DEF_TICKLENGTH, Blt_Offset(Axis, tickLength), 
        ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-timescale", "timeScale", "TimeScale", DEF_TIMESCALE,
        0, ALL_GRAPHS | BLT_CONFIG_DONT_SET_DEFAULT, &timeScaleOption},
    {BLT_CONFIG_STRING, "-title", "title", "Title",
        (char *)NULL, Blt_Offset(Axis, title),
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK | ALL_GRAPHS},
    {BLT_CONFIG_BOOLEAN, "-titlealternate", "titleAlternate", "TitleAlternate",
        DEF_TITLE_ALTERNATE, Blt_Offset(Axis, titleAlternate),
        BLT_CONFIG_DONT_SET_DEFAULT | ALL_GRAPHS},
    {BLT_CONFIG_COLOR, "-titlecolor", "titleColor", "Color", 
        DEF_FOREGROUND, Blt_Offset(Axis, titleColor),   
        ALL_GRAPHS},
    {BLT_CONFIG_FONT, "-titlefont", "titleFont", "Font", DEF_TITLE_FONT, 
        Blt_Offset(Axis, titleFont), ALL_GRAPHS},
    {BLT_CONFIG_FLOAT, "-weight", "weight", "Weight", DEF_WEIGHT, 
        Blt_Offset(Axis, weight), BLT_CONFIG_DONT_SET_DEFAULT | ALL_GRAPHS},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};

/* Forward declarations */
static void DestroyAxis(Axis *axisPtr);
static Tcl_FreeProc FreeAxisProc;
static int GetAxisByClass(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
        ClassId cid, Axis **axisPtrPtr);
static void TimeAxis(Axis *axisPtr, double min, double max);
static Tick FirstMajorTick(Axis *axisPtr);
static Tick NextMajorTick(Axis *axisPtr);
static Tick FirstMinorTick(Axis *axisPtr);
static Tick NextMinorTick(Axis *axisPtr);
static int lastMargin;

static Axis *
FirstAxis(Margin *marginPtr)
{
    Blt_ChainLink link;
    link = Blt_Chain_FirstLink(marginPtr->axes);
    if (link != NULL) {
        return Blt_Chain_GetValue(link);
    }
    return NULL;
}

static Axis *
NextAxis(Axis *axisPtr)
{
    if (axisPtr != NULL) {
        Blt_ChainLink link;
        link = Blt_Chain_NextLink(axisPtr->link);
        if (link != NULL) {
            return Blt_Chain_GetValue(link);
        }
    }
    return NULL;
}

static void
FreeAxisProc(DestroyData data)
{
    Blt_Free(data);
}

INLINE static double
Clamp(double x) 
{
    return (x < 0.0) ? 0.0 : (x > 1.0) ? 1.0 : x;
}

static void
SetAxisRange(AxisRange *rangePtr, double min, double max)
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
 *      Determines if a value lies within a given range.
 *
 *      The value is normalized and compared against the interval [0..1],
 *      where 0.0 is the minimum and 1.0 is the maximum.  DBL_EPSILON is
 *      the smallest number that can be represented on the host machine,
 *      such that (1.0 + epsilon) != 1.0.
 *
 *      Please note, *max* can't equal *min*.
 *
 * Results:
 *      If the value is within the interval [min..max], 1 is returned; 0
 *      otherwise.
 *
 *---------------------------------------------------------------------------
 */
INLINE static int
InRange(double x, AxisRange *rangePtr)
{
    if (rangePtr->range < DBL_EPSILON) {
        return (FABS(rangePtr->max - x) >= DBL_EPSILON);
    } else {
        double norm;

        norm = (x - rangePtr->min) * rangePtr->scale;
        return ((norm > -DBL_EPSILON) && ((norm - 1.0) <= DBL_EPSILON));
    }
}

INLINE static int
AxisIsHorizontal(Axis *axisPtr)
{
    if (axisPtr->obj.graphPtr->flags & INVERTED) {
        return (axisPtr->obj.classId == CID_AXIS_Y);
    } else {
        return (axisPtr->obj.classId == CID_AXIS_X);
    }
    return FALSE;
}


static void
ReleaseAxis(Axis *axisPtr)
{
    if (axisPtr != NULL) {
        axisPtr->refCount--;
        assert(axisPtr->refCount >= 0);
        if (axisPtr->refCount == 0) {
            DestroyAxis(axisPtr);
        }
    }
}

/*
 *-----------------------------------------------------------------------------
 * Custom option parse and print procedures
 *-----------------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
FreeAxis(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Axis **axisPtrPtr = (Axis **)(widgRec + offset);

    if (*axisPtrPtr != NULL) {
        ReleaseAxis(*axisPtrPtr);
        *axisPtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToAxis --
 *
 *      Converts the name of an axis to a pointer to its axis structure.
 *
 * Results:
 *      The return value is a standard TCL result.  The axis flags are
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToAxis(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    ClassId cid = (ClassId)clientData;
    Axis **axisPtrPtr = (Axis **)(widgRec + offset);
    Axis *axisPtr;
    Graph *graphPtr;

    if (flags & BLT_CONFIG_NULL_OK) {
        const char *string;

        string  = Tcl_GetString(objPtr);
        if (string[0] == '\0') {
            ReleaseAxis(*axisPtrPtr);
            *axisPtrPtr = NULL;
            return TCL_OK;
        }
    }
    graphPtr = Blt_GetGraphFromWindowData(tkwin);
    assert(graphPtr);
    if (GetAxisByClass(interp, graphPtr, objPtr, cid, &axisPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (*axisPtrPtr != NULL) {
        ReleaseAxis(*axisPtrPtr);
    }
    *axisPtrPtr = axisPtr;
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * AxisToObj --
 *
 *      Convert the window coordinates into a string.
 *
 * Results:
 *      The string representing the coordinate position is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
AxisToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
          char *widgRec, int offset, int flags)
{
    Axis *axisPtr = *(Axis **)(widgRec + offset);
    const char *name;

    name = (axisPtr == NULL) ? "" : axisPtr->obj.name;
    return Tcl_NewStringObj(name, -1);
}


/*
 *-----------------------------------------------------------------------------
 * Custom option parse and print procedures
 *-----------------------------------------------------------------------------
 */

/*ARGSUSED*/
static void
FreeAxisSwitch(ClientData clientData, char *record, int offset, int flags)
{
    Axis **axisPtrPtr = (Axis **)(record + offset);

    if (*axisPtrPtr != NULL) {
        ReleaseAxis(*axisPtrPtr);
        *axisPtrPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * XAxisSwitch --
 *
 *      Converts the name of an axis to a pointer to its axis structure.
 *
 * Results:
 *      The return value is a standard TCL result.  The axis flags are
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
XAxisSwitch(ClientData clientData, Tcl_Interp *interp, const char *switchName,
           Tcl_Obj *objPtr, char *record, int offset, int flags)
{
    Axis **axisPtrPtr = (Axis **)(record + offset);
    Axis *axisPtr;
    Graph *graphPtr = clientData;

    if (flags & BLT_SWITCH_NULL_OK) {
        const char *string;

        string  = Tcl_GetString(objPtr);
        if (string[0] == '\0') {
            ReleaseAxis(*axisPtrPtr);
            *axisPtrPtr = NULL;
            return TCL_OK;
        }
    }
    if (GetAxisByClass(interp, graphPtr, objPtr, CID_AXIS_X, &axisPtr)
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (*axisPtrPtr != NULL) {
        ReleaseAxis(*axisPtrPtr);
    }
    *axisPtrPtr = axisPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * YAxisSwitch --
 *
 *      Converts the name of an axis to a pointer to its axis structure.
 *
 * Results:
 *      The return value is a standard TCL result.  The axis flags are
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
YAxisSwitch(ClientData clientData, Tcl_Interp *interp, const char *switchName,
           Tcl_Obj *objPtr, char *record, int offset, int flags)
{
    Axis **axisPtrPtr = (Axis **)(record + offset);
    Axis *axisPtr;
    Graph *graphPtr = clientData;

    if (flags & BLT_SWITCH_NULL_OK) {
        const char *string;

        string  = Tcl_GetString(objPtr);
        if (string[0] == '\0') {
            ReleaseAxis(*axisPtrPtr);
            *axisPtrPtr = NULL;
            return TCL_OK;
        }
    }
    if (GetAxisByClass(interp, graphPtr, objPtr, CID_AXIS_Y, &axisPtr)
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (*axisPtrPtr != NULL) {
        ReleaseAxis(*axisPtrPtr);
    }
    *axisPtrPtr = axisPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToScale --
 *
 *      Convert the string obj to indicate if the axis is scaled as time,
 *      log, or linear.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToScale(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)(widgRec);
    char c;
    const char *string;
    
    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'l') && ((strcmp(string, "linear") == 0))) {
        axisPtr->scale = SCALE_LINEAR;
    } else if ((c == 'l') && ((strcmp(string, "log") == 0))) {
        axisPtr->scale = SCALE_LOG;
    } else if ((c == 't') && ((strcmp(string, "time") == 0))) {
        axisPtr->scale = SCALE_TIME;
    } else {
        Tcl_AppendResult(interp, "bad scale value \"", string, "\": should be"
                         " log, linear, or time", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ScaleToObj --
 *
 *      Convert the scale to string obj.
 *
 * Results:
 *      The string representing if the axis scale.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
ScaleToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)(widgRec);
    Tcl_Obj *objPtr;
    
    switch (axisPtr->scale) {
    case SCALE_LOG:
        objPtr = Tcl_NewStringObj("log", 3);
        break;
    case SCALE_LINEAR:
        objPtr = Tcl_NewStringObj("linear", 6);
        break;
    case SCALE_TIME:
        objPtr = Tcl_NewStringObj("time", 4);
        break;
    default:
        objPtr = Tcl_NewStringObj("???", 3);
        break;
    }
    return objPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToTimeScale --
 *
 *      Convert the boolean obj to indicate if the axis in scale as time.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTimeScale(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
               Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)(widgRec);
    int state;

    if (Tcl_GetBooleanFromObj(interp, objPtr, &state) != TCL_OK) {
        return TCL_ERROR;
    }
    axisPtr->scale = (state) ? SCALE_TIME : SCALE_LINEAR;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TimeScaleToObj --
 *
 *      Convert the time scale to boolean obj.
 *
 * Results:
 *      The string representing if the axis is timescale.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TimeScaleToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
               char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)(widgRec);
    return Tcl_NewBooleanObj(axisPtr->scale == SCALE_TIME);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLogScale --
 *
 *      Convert the boolean obj to indicate if the axis in scale as log.
 *
 * Results:
 *      The return value is a standard TCL result.  
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToLogScale(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)(widgRec);
    int state;

    if (Tcl_GetBooleanFromObj(interp, objPtr, &state) != TCL_OK) {
        return TCL_ERROR;
    }
    axisPtr->scale = (state) ? SCALE_LOG : SCALE_LINEAR;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LogScaleToObj --
 *
 *      Convert the log scale to boolean obj.
 *
 * Results:
 *      The string representing if the axis is logscale.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LogScaleToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
              char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)(widgRec);
    return Tcl_NewBooleanObj(axisPtr->scale == SCALE_LOG);
}

/*ARGSUSED*/
static void
FreeFormat(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Axis *axisPtr = (Axis *)(widgRec);

    if (axisPtr->limitsFmtsObjPtr != NULL) {
        Tcl_DecrRefCount(axisPtr->limitsFmtsObjPtr);
        axisPtr->limitsFmtsObjPtr = NULL;
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * ObjToFormat --
 *
 *      Converts the obj to a format list obj.  Checks if there
 *      are 1 or 2 elements in the list.
 *
 * Results:
 *      The return value is a standard TCL result.  The axis flags are
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToFormat(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Tcl_Obj **objPtrPtr = (Tcl_Obj **)(widgRec + offset);
    Tcl_Obj **objv;
    int objc;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc > 2) {
        Tcl_AppendResult(interp, "too many elements in limits format list \"",
                Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    if (objc == 0) {
        objPtr = NULL;                  /* An empty string is the same as
                                         * no formats. */
    } else {
        Tcl_IncrRefCount(objPtr);
    }
    if (*objPtrPtr != NULL) {
        Tcl_DecrRefCount(*objPtrPtr);
    }
    *objPtrPtr = objPtr;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FormatToObj --
 *
 *      Convert the window coordinates into a string.
 *
 * Results:
 *      The string representing the coordinate position is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
FormatToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
            char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)(widgRec);
    Tcl_Obj *objPtr;

    if (axisPtr->limitsFmtsObjPtr == NULL) {
        objPtr = Tcl_NewStringObj("", -1);
    } else {
        Tcl_IncrRefCount(axisPtr->limitsFmtsObjPtr);
        objPtr = axisPtr->limitsFmtsObjPtr;
    }
    return objPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLimit --
 *
 *      Convert the string representation of an axis limit into its numeric
 *      form.
 *
 * Results:
 *      The return value is a standard TCL result.  The symbol type is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToLimit(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    double *limitPtr = (double *)(widgRec + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
        *limitPtr = Blt_NaN();
    } else if (Blt_ExprDoubleFromObj(interp, objPtr, limitPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LimitToObj --
 *
 *      Convert the floating point axis limits into a string.
 *
 * Results:
 *      The string representation of the limits is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LimitToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)
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

/*
 *---------------------------------------------------------------------------
 *
 * ObjToMargin --
 *
 *      Convert the string representation of the margin to use into its 
 *      numeric form.
 *
 * Results:
 *      The return value is a standard TCL result.  The use type is written
 *      into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToMargin(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)(widgRec);
    Graph *graphPtr;
    const char *string;
    int i;
    char c;
    Margin *marginPtr;

    graphPtr = axisPtr->obj.graphPtr;
    if (axisPtr->refCount == 0) {
        /* Clear the axis class if it's not currently used by an element.*/
        Blt_GraphSetObjectClass(&axisPtr->obj, CID_NONE);
    }
    /* Remove the axis from the margin's use list and clear its use
     * flag. */
    if (axisPtr->link != NULL) {
        Blt_Chain_UnlinkLink(axisPtr->marginPtr->axes, axisPtr->link);
    }
    axisPtr->marginPtr = NULL;
    string = Tcl_GetString(objPtr);
    if ((string == NULL) || (string[0] == '\0')) {
        goto done;
    }
    c = string[0];
    for (i = 0; i < 4; i++) {
        marginPtr = graphPtr->margins + i;
        if ((c == marginPtr->name[0]) &&
            (strcmp(marginPtr->name, string) == 0)) {
            break;                      /* Found the axis name. */
        }
    }
    if (i == 4) {
        Tcl_AppendResult(interp, "unknown margin \"", string, "\": "
                         "should be x, y, x1, y2, or \"\".", (char *)NULL);
        return TCL_ERROR;
    }
    if (axisPtr->link != NULL) {
        /* Move the axis from the old margin's list to the new. */
        Blt_Chain_AppendLink(marginPtr->axes, axisPtr->link);
    } else {
        axisPtr->link = Blt_Chain_Append(marginPtr->axes, axisPtr);
    }
    axisPtr->marginPtr = marginPtr;
 done:
    graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | RESET_AXES);
    /* When any axis changes, we need to layout the entire graph.  */
    graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MarginToObj --
 *
 *      Convert the margin name into a string.
 *
 * Results:
 *      The string representation of the margin is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
MarginToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)(widgRec);
    
    if (axisPtr->marginPtr == NULL) {
        return Tcl_NewStringObj("", -1);
    }
    return Tcl_NewStringObj(axisPtr->marginPtr->name, -1);
}

/*ARGSUSED*/
static void
FreeTicks(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Axis *axisPtr = (Axis *)widgRec;
    TickGrid *ptr = (TickGrid *)(widgRec + offset);
    Ticks *ticksPtr;
    unsigned long mask = (unsigned long)clientData;

    ticksPtr = &ptr->ticks;
    if (ticksPtr->values != NULL) {
        Blt_Free(ticksPtr->values);
    }
    ticksPtr->values = NULL;
    axisPtr->flags |= mask;             
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToTicks --
 *
 *
 * Results:
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTicks(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)widgRec;
    Tcl_Obj **objv;
    int i;
    TickGrid *ptr = (TickGrid *)(widgRec + offset);
    Ticks *ticksPtr;
    double *values;
    int objc;
    unsigned long mask = (unsigned long)clientData;

    ticksPtr = &ptr->ticks;
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    axisPtr->flags |= mask;
    values = NULL;
    if (objc == 0) {
        if (ticksPtr->values != NULL) {
            Blt_Free(ticksPtr->values);
        }
        ticksPtr->values = NULL;
        ticksPtr->numSteps = 0;
        return TCL_OK;
    }

    values = Blt_AssertMalloc(objc * sizeof(double));
    for (i = 0; i < objc; i++) {
        double value;
        
        if (Blt_ExprDoubleFromObj(interp, objv[i], &value) != TCL_OK) {
            Blt_Free(ticksPtr);
            return TCL_ERROR;
        }
        values[i] = value;
    }
    ticksPtr->scaleType = SCALE_CUSTOM;
    ticksPtr->values = values;
    axisPtr->flags &= ~mask;
    if (ticksPtr->values != NULL) {
        Blt_Free(ticksPtr->values);
    }
    ticksPtr->values = NULL;
    ticksPtr->numSteps = objc;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TicksToObj --
 *
 *      Convert array of tick coordinates to a list.
 *
 * Results:
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TicksToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)widgRec;
    Tcl_Obj *listObjPtr;
    Ticks *ticksPtr;
    unsigned long mask;
    TickGrid *ptr = (TickGrid *)(widgRec + offset);

    ticksPtr = &ptr->ticks;
    mask = (unsigned long)clientData;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if ((ticksPtr->values != NULL) && ((axisPtr->flags & mask) == 0)) {
        int i;

        for (i = 0; i < ticksPtr->numSteps; i++) {
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
 * ObjToTickDirection --
 *
 *      Convert the string representation of a tick direction into its numeric
 *      form.
 *
 * Results:
 *      The return value is a standard TCL result.  The symbol type is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToTickDirection(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                   Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    int *flagsPtr = (int *)(widgRec + offset);
    const char *string;
    char c;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'i') && (strcmp(string, "in") == 0)) {
        *flagsPtr &= ~EXTERIOR;
    } else if ((c == 'o') && (strcmp(string, "out") == 0)) {
        *flagsPtr |= EXTERIOR;
    } else {
        Tcl_AppendResult(interp, "unknown tick direction \"", string,
                "\": should be in or out.", (char *)NULL);
        return TCL_ERROR;        
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TickDirectionToObj --
 *
 *      Convert the flag for tick direction into a string.
 *
 * Results:
 *      The string representation of the tick direction is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
TickDirectionToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
                   char *widgRec, int offset, int flags)
{
    int *flagsPtr = (int *)(widgRec + offset);
    const char *string;
    
    string = (*flagsPtr & EXTERIOR) ? "out" : "in";
    return Tcl_NewStringObj(string, -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToLoose --
 *
 *      Convert a string to one of three values.
 *              0 - false, no, off
 *              1 - true, yes, on
 *              2 - always
 * Results:
 *      If the string is successfully converted, TCL_OK is returned.
 *      Otherwise, TCL_ERROR is returned and an error message is left in
 *      interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToLoose(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)(widgRec);
    Tcl_Obj **objv;
    int i;
    int objc;
    int values[2];

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((objc < 1) || (objc > 2)) {
        Tcl_AppendResult(interp, "wrong # elements in loose value \"",
            Tcl_GetString(objPtr), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
        const char *string;

        string = Tcl_GetString(objv[i]);
        if ((string[0] == 'a') && (strcmp(string, "always") == 0)) {
            values[i] = ALWAYS_LOOSE;
        } else {
            int bool;

            if (Tcl_GetBooleanFromObj(interp, objv[i], &bool) != TCL_OK) {
                return TCL_ERROR;
            }
            values[i] = bool;
        }
    }
    axisPtr->looseMin = axisPtr->looseMax = values[0];
    if (objc > 1) {
        axisPtr->looseMax = values[1];
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LooseToObj --
 *
 * Results:
 *      The string representation of the auto boolean is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
LooseToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
           char *widgRec, int offset, int flags)
{
    Axis *axisPtr = (Axis *)widgRec;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (axisPtr->looseMin == TIGHT) {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewBooleanObj(FALSE));
    } else if (axisPtr->looseMin == LOOSE) {
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewBooleanObj(TRUE));
    } else if (axisPtr->looseMin == ALWAYS_LOOSE) {
        Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj("always", 6));
    }
    if (axisPtr->looseMin != axisPtr->looseMax) {
        if (axisPtr->looseMax == TIGHT) {
            Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewBooleanObj(FALSE));
        } else if (axisPtr->looseMax == LOOSE) {
            Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewBooleanObj(TRUE));
        } else if (axisPtr->looseMax == ALWAYS_LOOSE) {
            Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj("always", 6));
        }
    }
    return listObjPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * PaletteChangedProc
 *
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
PaletteChangedProc(Blt_Palette palette, ClientData clientData, 
                   unsigned int flags)
{
    Axis *axisPtr = clientData;

     if (flags & PALETTE_DELETE_NOTIFY) {
        axisPtr->palette = NULL;
    }
    axisPtr->flags |= MAP_ITEM;
    axisPtr->obj.graphPtr->flags |= CACHE_DIRTY;
    Blt_EventuallyRedrawGraph(axisPtr->obj.graphPtr);
}

/*ARGSUSED*/
static void
FreePalette(ClientData clientData, Display *display, char *widgRec, int offset)
{
    Blt_Palette *palPtr = (Blt_Palette *)(widgRec + offset);
    Axis *axisPtr = (Axis *)widgRec;

    Blt_Palette_DeleteNotifier(*palPtr, PaletteChangedProc, axisPtr);
    *palPtr = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToPalette --
 *
 *      Convert the string representation of a palette into its token.
 *
 * Results:
 *      The return value is a standard TCL result.  The palette token is
 *      written into the widget record.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToPalette(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
             Tcl_Obj *objPtr, char *widgRec, int offset, int flags)
{
    Blt_Palette *palPtr = (Blt_Palette *)(widgRec + offset);
    Axis *axisPtr = (Axis *)widgRec;
    const char *string;
    
    string = Tcl_GetString(objPtr);
    if ((string == NULL) || (string[0] == '\0')) {
        FreePalette(clientData, Tk_Display(tkwin), widgRec, offset);
        return TCL_OK;
    }
    if (Blt_Palette_GetFromObj(interp, objPtr, palPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_Palette_CreateNotifier(*palPtr, PaletteChangedProc, axisPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PaletteToObj --
 *
 *      Convert the palette token into a string.
 *
 * Results:
 *      The string representing the symbol type or line style is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
PaletteToObj(ClientData clientData, Tcl_Interp *interp, Tk_Window tkwin,
             char *widgRec, int offset, int flags)
{
    Blt_Palette palette = *(Blt_Palette *)(widgRec + offset);
    if (palette == NULL) {
        return Tcl_NewStringObj("", -1);
    } 
    return Tcl_NewStringObj(Blt_Palette_Name(palette), -1);
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
 *      Converts a floating point tick value to a string to be used as its
 *      label.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Returns a new label in the string character buffer.  The formatted
 *      tick label will be displayed on the graph.
 *
 * -------------------------------------------------------------------------- 
 */
static TickLabel *
MakeLabel(Axis *axisPtr, double value)
{
#define TICK_LABEL_SIZE         200
    char buffer[TICK_LABEL_SIZE + 1];
    const char *string;
    TickLabel *labelPtr;
    Tcl_DString ds;

    string = NULL;
    Tcl_DStringInit(&ds);
    if (axisPtr->fmtCmdObjPtr != NULL) {
        Graph *graphPtr;
        Tcl_Interp *interp;
        Tcl_Obj *cmdObjPtr, *objPtr;
        Tk_Window tkwin;
        int result;

        graphPtr = axisPtr->obj.graphPtr;
        interp = graphPtr->interp;
        tkwin = graphPtr->tkwin;
        /*
         * A TCL proc was designated to format tick labels. Append the path
         * name of the widget and the default tick label as arguments when
         * invoking it. Copy and save the new label from interp->result.
         */
        cmdObjPtr = Tcl_DuplicateObj(axisPtr->fmtCmdObjPtr);
        objPtr = Tcl_NewStringObj(Tk_PathName(tkwin), -1);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        objPtr = Tcl_NewDoubleObj(value);
        Tcl_ResetResult(interp);
        Tcl_IncrRefCount(cmdObjPtr);
        Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result != TCL_OK) {
            Tcl_BackgroundError(interp);
        } 
        Tcl_DStringGetResult(interp, &ds);
        string = Tcl_DStringValue(&ds);
    } else if (IsLogScale(axisPtr)) {
        Blt_FormatString(buffer, TICK_LABEL_SIZE, "1E%d", ROUND(value));
        string = buffer;
    } else if ((IsTimeScale(axisPtr)) && (axisPtr->major.ticks.fmt != NULL)) {
        Blt_DateTime date;

        Blt_SecondsToDate(value, &date);
        Blt_FormatDate(&date, axisPtr->major.ticks.fmt, &ds);
        string = Tcl_DStringValue(&ds);
    } else {
        if ((IsTimeScale(axisPtr)) &&
            (axisPtr->major.ticks.timeUnits == TIME_SUBSECONDS)) {
            value = fmod(value, 60.0);
            value = UROUND(value, axisPtr->major.ticks.step);
        }
        Blt_FormatString(buffer, TICK_LABEL_SIZE, "%.*G", NUMDIGITS, value);
        string = buffer;
    }
    labelPtr = Blt_AssertMalloc(sizeof(TickLabel) + strlen(string));
    strcpy(labelPtr->string, string);
    labelPtr->anchorPos.x = labelPtr->anchorPos.y = -1000;
    Tcl_DStringFree(&ds);
    return labelPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_InvHMap --
 *
 *      Maps the given screen coordinate back to a graph coordinate.  Called
 *      by the graph locater routine.
 *
 * Results:
 *      Returns the graph coordinate value at the given window
 *      y-coordinate.
 *
 *---------------------------------------------------------------------------
 */
double
Blt_InvHMap(Axis *axisPtr, double x)
{
    double value;

    x = (double)(x - axisPtr->screenMin) * axisPtr->screenScale;
    if (axisPtr->decreasing) {
        x = 1.0 - x;
    }
    value = (x * axisPtr->axisRange.range) + axisPtr->axisRange.min;
    if (IsLogScale(axisPtr)) {
        value = EXP10(value);
    }
    return value;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_InvVMap --
 *
 *      Maps the given screen y-coordinate back to the graph coordinate
 *      value. Called by the graph locater routine.
 *
 * Results:
 *      Returns the graph coordinate value for the given screen
 *      coordinate.
 *
 *---------------------------------------------------------------------------
 */
double
Blt_InvVMap(Axis *axisPtr, double y) /* Screen coordinate */
{
    double value;

    y = (double)(y - axisPtr->screenMin) * axisPtr->screenScale;
    if (axisPtr->decreasing) {
        y = 1.0 - y;
    }
    value = ((1.0 - y) * axisPtr->axisRange.range) + axisPtr->axisRange.min;
    if (IsLogScale(axisPtr)) {
        value = EXP10(value);
    }
    return value;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_HMap --
 *
 *      Map the given graph coordinate value to its axis, returning a window
 *      position.
 *
 * Results:
 *      Returns a double precision number representing the window coordinate
 *      position on the given axis.
 *
 *---------------------------------------------------------------------------
 */
double
Blt_HMap(Axis *axisPtr, double x)
{
    if ((IsLogScale(axisPtr)) && (x != 0.0)) {
        x = log10(FABS(x));
    }
    /* Map graph coordinate to normalized coordinates [0..1] */
    x = (x - axisPtr->axisRange.min) * axisPtr->axisRange.scale;
    if (axisPtr->decreasing) {
        x = 1.0 - x;
    }
    return (x * axisPtr->screenRange + axisPtr->screenMin);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_VMap --
 *
 *      Map the given graph coordinate value to its axis, returning a window
 *      position.
 *
 * Results:
 *      Returns a double precision number representing the window coordinate
 *      position on the given axis.
 *
 *---------------------------------------------------------------------------
 */
double
Blt_VMap(Axis *axisPtr, double y)
{
    if ((IsLogScale(axisPtr)) && (y > 0.0)) {
        y = log10(FABS(y));
    }
    /* Map graph coordinate to normalized coordinates [0..1] */
    y = (y - axisPtr->axisRange.min) * axisPtr->axisRange.scale;
    if (axisPtr->decreasing) {
        y = 1.0 - y;
    }
    return ((1.0 - y) * axisPtr->screenRange + axisPtr->screenMin);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Map2D --
 *
 *      Maps the given graph x,y coordinate values to a window position.
 *
 * Results:
 *      Returns a XPoint structure containing the window coordinates of
 *      the given graph x,y coordinate.
 *
 *---------------------------------------------------------------------------
 */
Point2d
Blt_Map2D(Graph *graphPtr, double x, double y, Axis2d *axesPtr)
{
    Point2d point;

    if (graphPtr->flags & INVERTED) {
        point.x = Blt_HMap(axesPtr->y, y);
        point.y = Blt_VMap(axesPtr->x, x);
    } else {
        point.x = Blt_HMap(axesPtr->x, x);
        point.y = Blt_VMap(axesPtr->y, y);
    }
    return point;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_InvMap2D --
 *
 *      Maps the given window x,y coordinates to graph values.
 *
 * Results:
 *      Returns a structure containing the graph coordinates of the given
 *      window x,y coordinate.
 *
 *---------------------------------------------------------------------------
 */
Point2d
Blt_InvMap2D(
    Graph *graphPtr,
    double x, double y,                 /* Window x and y coordinates */
    Axis2d *axesPtr)                    /* Specifies which axes to use */
{
    Point2d point;

    if (graphPtr->flags & INVERTED) {
        point.x = Blt_InvVMap(axesPtr->x, y);
        point.y = Blt_InvHMap(axesPtr->y, x);
    } else {
        point.x = Blt_InvHMap(axesPtr->x, x);
        point.y = Blt_InvVMap(axesPtr->y, y);
    }
    return point;
}

static void
FixAxisRange(Axis *axisPtr)
{
    double min, max;

    /*
     * When auto-scaling, the axis limits are the bounds of the element data.
     * If no data exists, set arbitrary limits (wrt to log/linear scale).
     */
    min = axisPtr->valueRange.min;
    max = axisPtr->valueRange.max;

    /* Check the requested axis limits. Can't allow -min to be greater
     * than -max, or have undefined log scale limits.  */
    if (((DEFINED(axisPtr->reqMin)) && (DEFINED(axisPtr->reqMax))) &&
        (axisPtr->reqMin >= axisPtr->reqMax)) {
        axisPtr->reqMin = axisPtr->reqMax = Blt_NaN();
    }
    if (IsLogScale(axisPtr)) {
        if ((DEFINED(axisPtr->reqMin)) && (axisPtr->reqMin <= 0.0)) {
            axisPtr->reqMin = Blt_NaN();
        }
        if ((DEFINED(axisPtr->reqMax)) && (axisPtr->reqMax <= 0.0)) {
            axisPtr->reqMax = Blt_NaN();
        }
    }

    if (min == DBL_MAX) {
        if (DEFINED(axisPtr->reqMin)) {
            min = axisPtr->reqMin;
        } else {
            min = (IsLogScale(axisPtr)) ? 0.001 : 0.0;
        }
    }
    if (max == -DBL_MAX) {
        if (DEFINED(axisPtr->reqMax)) {
            max = axisPtr->reqMax;
        } else {
            max = 1.0;
        }
    }
    if (min > max) {
        /*
         * There is no range of data (i.e. min is not less than max), so
         * manufacture one.
         */
        if (min == 0.0) {
            max = 1.0;
        } else {
            max = min + (FABS(min) * 0.1);
        }
    } else if (min == max) {
        /*
         * There is no range of data (i.e. min is not less than max), so
         * manufacture one.
         */
        max = min + 0.5;
        min = min - 0.5;
    }
    SetAxisRange(&axisPtr->valueRange, min, max);

    /*   
     * The axis limits are either the current data range or overridden by the
     * values selected by the user with the -min or -max options.
     */
    axisPtr->min = min;
    axisPtr->max = max;
    if (DEFINED(axisPtr->reqMin)) {
        axisPtr->min = axisPtr->reqMin;
    }
    if (DEFINED(axisPtr->reqMax)) { 
        axisPtr->max = axisPtr->reqMax;
    }
    if (axisPtr->max < axisPtr->min) {
        /*   
         * If the limits still don't make sense, it's because one limit
         * configuration option (-min or -max) was set and the other default
         * (based upon the data) is too small or large.  Remedy this by making
         * up a new min or max from the user-defined limit.
         */
        if (!DEFINED(axisPtr->reqMin)) {
            axisPtr->min = axisPtr->max - (FABS(axisPtr->max) * 0.1);
        }
        if (!DEFINED(axisPtr->reqMax)) {
            axisPtr->max = axisPtr->min + (FABS(axisPtr->max) * 0.1);
        }
    }
    /* 
     * If a window size is defined, handle auto ranging by shifting the axis
     * limits.
     */
    if ((axisPtr->windowSize > 0.0) && 
        (!DEFINED(axisPtr->reqMin)) && (!DEFINED(axisPtr->reqMax))) {
        if (axisPtr->shiftBy < 0.0) {
            axisPtr->shiftBy = 0.0;
        }
        max = axisPtr->min + axisPtr->windowSize;
        if (axisPtr->max >= max) {
            if (axisPtr->shiftBy > 0.0) {
                max = UCEIL(axisPtr->max, axisPtr->shiftBy);
            }
            axisPtr->min = max - axisPtr->windowSize;
        }
        axisPtr->max = max;
    }
    if ((axisPtr->max != axisPtr->prevMax) || 
        (axisPtr->min != axisPtr->prevMin)) {
        /* Indicate if the axis limits have changed */
        axisPtr->flags |= DIRTY;
        /* and save the previous minimum and maximum values */
        axisPtr->prevMin = axisPtr->min;
        axisPtr->prevMax = axisPtr->max;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * NiceNum --
 *
 *      Reference: Paul Heckbert, "Nice Numbers for Graph Labels",
 *                 Graphics Gems, pp 61-63.  
 *
 *      Finds a "nice" number approximately equal to x.
 *
 *---------------------------------------------------------------------------
 */
static double
NiceNum(double x, int round)            /* If non-zero, round. Otherwise
                                         * take ceiling of value. */
{
    double expt;                        /* Exponent of x */
    double frac;                        /* Fractional part of x */
    double nice;                        /* Nice, rounded fraction */

    expt = floor(log10(x));
    frac = x / EXP10(expt);             /* between 1 and 10 */
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

/*
 *---------------------------------------------------------------------------
 *
 * LogAxis --
 *
 *      Determine the range and units of a log scaled axis.
 *
 *      Unless the axis limits are specified, the axis is scaled
 *      automatically, where the smallest and largest major ticks encompass
 *      the range of actual data values.  When an axis limit is specified,
 *      that value represents the smallest(min)/largest(max) value in the
 *      displayed range of values.
 *
 *      Both manual and automatic scaling are affected by the step used.  By
 *      default, the step is the largest power of ten to divide the range in
 *      more than one piece.
 *
 *      Automatic scaling:
 *      Find the smallest number of units which contain the range of
 *      values.  The minimum and maximum major tick values will be
 *      represent the range of values for the axis. This greatest number of
 *      major ticks possible is 10.
 *
 *      Manual scaling:
 *      Make the minimum and maximum data values the represent the range of
 *      the values for the axis.  The minimum and maximum major ticks will
 *      be inclusive of this range.  This provides the largest area for
 *      plotting and the expected results when the axis min and max values
 *      have be set by the user (.e.g zooming).  The maximum number of
 *      major ticks is 20.
 *
 *      For log scale, there's the possibility that the minimum and maximum
 *      data values are the same magnitude.  To represent the points
 *      properly, at least one full decade should be shown.  However, if
 *      you zoom a log scale plot, the results should be
 *      predictable. Therefore, in that case, show only minor ticks.
 *      Lastly, there should be an appropriate way to handle numbers <=0.
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
 *      numTicks = Number of ticks
 *      min = Minimum value of axis
 *      max = Maximum value of axis
 *      range    = Range of values (max - min)
 *
 *      If the number of decades is greater than ten, it is assumed
 *      that the full set of log-style ticks can't be drawn properly.
 *
 * Results:
 *      None
 *
 * -------------------------------------------------------------------------- 
 */
static void
LogAxis(Axis *axisPtr, double min, double max)
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
             * decade.  Instead, treat the axis as a linear scale.  */
            range = NiceNum(range, 0);
            majorStep = NiceNum(range / axisPtr->reqNumMajorTicks, 1);
            tickMin = UFLOOR(tickMin, majorStep);
            tickMax = UCEIL(tickMax, majorStep);
            numMajor = (int)((tickMax - tickMin) / majorStep) + 1;
            minorStep = EXP10(floor(log10(majorStep)));
            if (minorStep == majorStep) {
                numMinor = 4, minorStep = 0.2;
            } else {
                numMinor = ROUND(majorStep / minorStep) - 1;
            }
        } else {
            if (tickMin == tickMax) {
                tickMax++;
            }
            majorStep = 1.0;
            numMajor = (int)(tickMax - tickMin + 1); /* FIXME: Check this. */
            
            minorStep = 0.0;            /* This is a special hack to pass
                                         * information to the GenerateTicks
                                         * routine. An interval of 0.0
                                         * tells 1) this is a minor sweep
                                         * and 2) the axis is log scale. */
            numMinor = 10;
        }
        if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) && 
             (DEFINED(axisPtr->reqMin)))) {
            tickMin = min;
            numMajor++;
        }
        if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
             (DEFINED(axisPtr->reqMax)))) {
            tickMax = max;
        }
    }
    axisPtr->major.ticks.scaleType = SCALE_LOG;
    axisPtr->major.ticks.step = majorStep;
    axisPtr->major.ticks.initial = floor(tickMin);
    axisPtr->major.ticks.numSteps = numMajor;
    axisPtr->minor.ticks.initial = axisPtr->minor.ticks.step = minorStep;
    axisPtr->minor.ticks.numSteps = numMinor;
    axisPtr->minor.ticks.scaleType = SCALE_LOG;
    SetAxisRange(&axisPtr->axisRange, tickMin, tickMax);
    /* Never generate minor ticks. */
}

/*
 *---------------------------------------------------------------------------
 *
 * LinearAxis --
 *
 *      Determine the units of a linear scaled axis.
 *
 *      The axis limits are either the range of the data values mapped
 *      to the axis (autoscaled), or the values specified by the -min
 *      and -max options (manual).
 *
 *      If autoscaled, the smallest and largest major ticks will
 *      encompass the range of data values.  If the -loose option is
 *      selected, the next outer ticks are choosen.  If tight, the
 *      ticks are at or inside of the data limits are used.
 *
 *      If manually set, the ticks are at or inside the data limits
 *      are used.  This makes sense for zooming.  You want the
 *      selected range to represent the next limit, not something a
 *      bit bigger.
 *
 *      Note: I added an "always" value to the -loose option to force
 *            the manually selected axes to be loose. It's probably
 *            not a good idea.
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
 *      numTicks = Number of ticks
 *      min = Minimum value of axis
 *      max = Maximum value of axis
 *      range    = Range of values (max - min)
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The axis tick information is set.  The actual tick values will
 *      be generated later.
 *
 *---------------------------------------------------------------------------
 */
static void
LinearAxis(Axis *axisPtr, double min, double max)
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
        if (axisPtr->reqStep > 0.0) {
            /* An interval was designated by the user.  Keep scaling it
             * until it fits comfortably within the current range of the
             * axis.  */
            step = axisPtr->reqStep;
            while ((2 * step) >= range) {
                step *= 0.5;
            }
        } else {
            range = NiceNum(range, 0);
            step = NiceNum(range / axisPtr->reqNumMajorTicks, 1);
        }
        
        /* Find the outer tick values. Add 0.0 to prevent getting -0.0. */
        axisMin = tickMin = floor(min / step) * step + 0.0;
        axisMax = tickMax = ceil(max / step) * step + 0.0;
        
        numTicks = ROUND((tickMax - tickMin) / step) + 1;
    } 
    /*
     * The limits of the axis are either the range of the data ("tight") or at
     * the next outer tick interval ("loose").  The looseness or tightness has
     * to do with how the axis fits the range of data values.  This option is
     * overridden when the user sets an axis limit (by either -min or -max
     * option).  The axis limit is always at the selected limit (otherwise we
     * assume that user would have picked a different number).
     */
    if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) &&
         (DEFINED(axisPtr->reqMin)))) {
        axisMin = min;
    }
    if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
         (DEFINED(axisPtr->reqMax)))) {
        axisMax = max;
    }
    SetAxisRange(&axisPtr->axisRange, axisMin, axisMax);

    axisPtr->major.ticks.step = step;
    axisPtr->major.ticks.initial = tickMin;
    axisPtr->major.ticks.numSteps = numTicks;
    axisPtr->major.ticks.scaleType = SCALE_LINEAR;

    /* Now calculate the minor tick step and number. */

    if ((axisPtr->reqNumMinorTicks > 0) && (axisPtr->flags & AUTO_MAJOR)) {
        step = 1.0 / (double)axisPtr->reqNumMinorTicks;
        numTicks = axisPtr->reqNumMinorTicks - 1;
    } else {
        numTicks = 0;                   /* No minor ticks. */
        /* Don't set the minor tick interval to 0.0. It makes the
         * GenerateTicks routine create minor log-scale tick marks.  */
        step = 0.5;
    }

    axisPtr->minor.ticks.step = step;
    axisPtr->minor.ticks.numSteps = numTicks;
    axisPtr->minor.ticks.scaleType = SCALE_LINEAR;
    /* Never generate minor ticks. */
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResetAxes --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ResetAxes(Graph *graphPtr)
{
    Blt_ChainLink link;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    /* FIXME: This should be called whenever the display list of elements
     *        change. Maybe yet another flag INIT_STACKS to indicate that
     *        the element display list has changed.  Needs to be done
     *        before the axis limits are set.
     */
    Blt_InitBarGroups(graphPtr);
    /*
     * Step 1:  Reset all axes. Initialize the data limits of the axis to
     *          impossible values.
     */
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
        hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        Axis *axisPtr;

        axisPtr = Blt_GetHashValue(hPtr);
        axisPtr->min = axisPtr->valueRange.min = DBL_MAX;
        axisPtr->max = axisPtr->valueRange.max = -DBL_MAX;
    }

    /*
     * Step 2:  For each element that's to be displayed, get the smallest
     *          and largest data values mapped to each X and Y-axis.  This
     *          will be the axis limits if the user doesn't override them 
     *          with -min and -max options.
     */
    for (link = Blt_Chain_FirstLink(graphPtr->elements.displayList);
         link != NULL; link = Blt_Chain_NextLink(link)) {
        Element *elemPtr;

        elemPtr = Blt_Chain_GetValue(link);
        if ((graphPtr->flags & MAP_VISIBLE) && (elemPtr->flags & HIDDEN)) {
            continue;
        }
        (*elemPtr->procsPtr->extentsProc) (elemPtr);
    }
    /*
     * Step 3:  Now that we know the range of data values for each axis,
     *          set axis limits and compute a sweep to generate tick values.
     */
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        Axis *axisPtr;
        double min, max;

        axisPtr = Blt_GetHashValue(hPtr);
        FixAxisRange(axisPtr);

        /* Calculate min/max tick (major/minor) layouts */
        min = axisPtr->min;
        max = axisPtr->max;
        if ((DEFINED(axisPtr->scrollMin)) && (min < axisPtr->scrollMin)) {
            min = axisPtr->scrollMin;
        }
        if ((DEFINED(axisPtr->scrollMax)) && (max > axisPtr->scrollMax)) {
            max = axisPtr->scrollMax;
        }
        if (IsLogScale(axisPtr)) {
            LogAxis(axisPtr, min, max);
        } else if (IsTimeScale(axisPtr)) {
            TimeAxis(axisPtr, min, max);
        } else {
            LinearAxis(axisPtr, min, max);
        }
        if ((axisPtr->marginPtr != NULL) && (axisPtr->flags & DIRTY)) {
            graphPtr->flags |= CACHE_DIRTY;
        }
    }

    graphPtr->flags &= ~RESET_AXES;
    
    /*
     * When any axis changes, we need to layout the entire graph.
     */
    graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | 
                        MAP_ALL | REDRAW_WORLD);
}

/*
 *---------------------------------------------------------------------------
 *
 * ResetTextStyles --
 *
 *      Configures axis attributes (font, line width, label, etc) and
 *      allocates a new (possibly shared) graphics context.  Line cap style
 *      is projecting.  This is for the problem of when a tick sits
 *      directly at the end point of the axis.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 * Side Effects:
 *      Axis resources are allocated (GC, font). Axis layout is deferred
 *      until the height and width of the window are known.
 *
 *---------------------------------------------------------------------------
 */
static void
ResetTextStyles(Axis *axisPtr)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    GC newGC;
    XGCValues gcValues;
    unsigned long gcMask;
    
    Blt_Ts_ResetStyle(graphPtr->tkwin, &axisPtr->limitsTextStyle);

    gcMask = (GCForeground | GCLineWidth | GCCapStyle);
    gcValues.foreground = axisPtr->tickColor->pixel;
    gcValues.font = Blt_Font_Id(axisPtr->tickFont);
    gcValues.line_width = LineWidth(axisPtr->lineWidth);
    gcValues.cap_style = CapProjecting;

    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (axisPtr->tickGC != NULL) {
        Tk_FreeGC(graphPtr->display, axisPtr->tickGC);
    }
    axisPtr->tickGC = newGC;

    /* Assuming settings from above GC */
    gcValues.foreground = axisPtr->activeFgColor->pixel;
    newGC = Tk_GetGC(graphPtr->tkwin, gcMask, &gcValues);
    if (axisPtr->activeTickGC != NULL) {
        Tk_FreeGC(graphPtr->display, axisPtr->activeTickGC);
    }
    axisPtr->activeTickGC = newGC;

    gcValues.background = gcValues.foreground = axisPtr->major.grid.color->pixel;
    gcValues.line_width = LineWidth(axisPtr->major.grid.lineWidth);
    gcMask = (GCForeground | GCBackground | GCLineWidth);
    if (LineIsDashed(axisPtr->major.grid.dashes)) {
        gcValues.line_style = LineOnOffDash;
        gcMask |= GCLineStyle;
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(axisPtr->major.grid.dashes)) {
        Blt_SetDashes(graphPtr->display, newGC, &axisPtr->major.grid.dashes);
    }
    if (axisPtr->major.grid.gc != NULL) {
        Blt_FreePrivateGC(graphPtr->display, axisPtr->major.grid.gc);
    }
    axisPtr->major.grid.gc = newGC;

    gcValues.background = gcValues.foreground = 
        axisPtr->minor.grid.color->pixel;
    gcValues.line_width = LineWidth(axisPtr->minor.grid.lineWidth);
    gcMask = (GCForeground | GCBackground | GCLineWidth);
    if (LineIsDashed(axisPtr->minor.grid.dashes)) {
        gcValues.line_style = LineOnOffDash;
        gcMask |= GCLineStyle;
    }
    newGC = Blt_GetPrivateGC(graphPtr->tkwin, gcMask, &gcValues);
    if (LineIsDashed(axisPtr->minor.grid.dashes)) {
        Blt_SetDashes(graphPtr->display, newGC, &axisPtr->minor.grid.dashes);
    }
    if (axisPtr->minor.grid.gc != NULL) {
        Blt_FreePrivateGC(graphPtr->display, axisPtr->minor.grid.gc);
    }
    axisPtr->minor.grid.gc = newGC;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyAxis --
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Resources (font, color, gc, labels, etc.) associated with the axis
 *      are deallocated.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyAxis(Axis *axisPtr)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    int flags;

    axisPtr->obj.deleted = TRUE;        /* Mark the axis as deleted. */

    flags = Blt_GraphType(graphPtr);
    Blt_FreeOptions(configSpecs, (char *)axisPtr, graphPtr->display, flags);
    if (graphPtr->bindTable != NULL) {
        Blt_DeleteBindings(graphPtr->bindTable, axisPtr);
    }
    if (axisPtr->link != NULL) {
        Blt_Chain_DeleteLink(axisPtr->marginPtr->axes, axisPtr->link);
    }
    if (axisPtr->obj.name != NULL) {
        Blt_Free(axisPtr->obj.name);
    }
    if (axisPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&graphPtr->axes.nameTable, axisPtr->hashPtr);
    }
    Blt_Ts_FreeStyle(graphPtr->display, &axisPtr->limitsTextStyle);

    if (axisPtr->tickGC != NULL) {
        Tk_FreeGC(graphPtr->display, axisPtr->tickGC);
    }
    if (axisPtr->activeTickGC != NULL) {
        Tk_FreeGC(graphPtr->display, axisPtr->activeTickGC);
    }
    if (axisPtr->major.grid.gc != NULL) {
        Blt_FreePrivateGC(graphPtr->display, axisPtr->major.grid.gc);
    }
    if (axisPtr->minor.grid.gc != NULL) {
        Blt_FreePrivateGC(graphPtr->display, axisPtr->minor.grid.gc);
    }
    FreeTickLabels(axisPtr->tickLabels);
    Blt_Chain_Destroy(axisPtr->tickLabels);
    if (axisPtr->segments != NULL) {
        Blt_Free(axisPtr->segments);
    }
    Tcl_EventuallyFree(axisPtr, FreeAxisProc);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisOffsets --
 *
 *      Determines the sides of the axis, major and minor ticks, and title
 *      of the axis.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
AxisOffsets(Axis *axisPtr, AxisInfo *infoPtr)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    int pad;                            /* Offset of axis from interior
                                         * region. This includes a possible
                                         * border and the axis line
                                         * width. */
    int axisLine;
    int t1, t2, labelOffset;
    int tickLabel, axisPad;
    int inset, mark;
    int x, y;
    float fangle;

    axisPtr->titleAngle = titleAngle[axisPtr->marginPtr->side];
    tickLabel = axisLine = t1 = t2 = 0;
    labelOffset = AXIS_PAD_TITLE;
    if (axisPtr->lineWidth > 0) {
        if (axisPtr->flags & TICKLABELS) {
            t1 = axisPtr->tickLength;
            t2 = (t1 * 10) / 15;
        }
        labelOffset = t1 + AXIS_PAD_TITLE;
        if (axisPtr->flags & EXTERIOR) {
            labelOffset += axisPtr->lineWidth;
        }
    }
    axisPad = 0;
    if (graphPtr->plotRelief != TK_RELIEF_SOLID) {
        axisPad = 0;
    }
    /* Adjust offset for the interior border width and the line width */
    pad = 1;
    if (graphPtr->plotBorderWidth > 0) {
        pad += graphPtr->plotBorderWidth + 1;
    }
    pad = 0;                            /* FIXME: test */
    /*
     * Pre-calculate the x-coordinate positions of the axis, tick labels,
     * and the individual major and minor ticks.
     */
    inset = pad + axisPtr->lineWidth / 2;
    switch (axisPtr->marginPtr->side) {
    case MARGIN_TOP:
        axisLine = graphPtr->y1 - axisPtr->marginPtr->nextLayerOffset;
        if (axisPtr->colorbar.thickness > 0) {
            axisLine -= axisPtr->colorbar.thickness + COLORBAR_PAD;
        }
        if (axisPtr->flags & EXTERIOR) {
            axisLine -= graphPtr->plotBorderWidth + axisPad +
                axisPtr->lineWidth / 2;
            tickLabel = axisLine - 2;
            if (axisPtr->lineWidth > 0) {
                tickLabel -= axisPtr->tickLength;
            }
        } else {
            if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
                axisLine--;
            }  
            axisLine -= axisPad + axisPtr->lineWidth / 2;
            tickLabel = graphPtr->y1 - graphPtr->plotBorderWidth - 2;
        }
        mark = graphPtr->y1 - axisPtr->marginPtr->nextLayerOffset - pad;
        axisPtr->tickAnchor = TK_ANCHOR_S;
        axisPtr->left = axisPtr->screenMin - inset - 2;
        axisPtr->right = axisPtr->screenMin + axisPtr->screenRange + inset - 1;
        if (graphPtr->flags & STACK_AXES) {
            axisPtr->top = mark - axisPtr->marginPtr->axesOffset;
        } else {
            axisPtr->top = mark - axisPtr->height;
        }
        axisPtr->bottom = mark;
        if (axisPtr->titleAlternate) {
            x = graphPtr->x2 + AXIS_PAD_TITLE;
            y = mark - (axisPtr->height  / 2);
            axisPtr->titleAnchor = TK_ANCHOR_W;
        } else {
            x = (axisPtr->right + axisPtr->left) / 2;
            if (graphPtr->flags & STACK_AXES) {
                y = mark - axisPtr->marginPtr->axesOffset + AXIS_PAD_TITLE;
            } else {
                y = mark - axisPtr->height + AXIS_PAD_TITLE;
            }
            axisPtr->titleAnchor = TK_ANCHOR_N;
        }
        axisPtr->titlePos.x = x;
        axisPtr->titlePos.y = y;
        infoPtr->colorbar = axisLine - axisPtr->colorbar.thickness;
        break;

    case MARGIN_BOTTOM:
        /*
         *  ----------- bottom + plot borderwidth
         *      mark --------------------------------------------
         *          ===================== axisLine (linewidth)
         *                   tick
         *                  title
         *
         *          ===================== axisLine (linewidth)
         *  ----------- bottom + plot borderwidth
         *      mark --------------------------------------------
         *                   tick
         *                  title
         */
        axisLine = graphPtr->y2 + axisPtr->marginPtr->nextLayerOffset;
        if (axisPtr->colorbar.thickness > 0) {
            axisLine += axisPtr->colorbar.thickness + COLORBAR_PAD;
        }
        if (axisPtr->flags & EXTERIOR) {
            axisLine += graphPtr->plotBorderWidth + axisPad +
                axisPtr->lineWidth / 2;
            if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
                axisLine--;
            } else {
                axisLine += 2;
            }
            tickLabel = axisLine + 2;
            if (axisPtr->lineWidth > 0) {
                tickLabel += axisPtr->tickLength;
            }
        } else {
            axisLine -= axisPad + axisPtr->lineWidth / 2;
            tickLabel = graphPtr->y2 +  graphPtr->plotBorderWidth + 2;
            if (graphPtr->plotRelief != TK_RELIEF_SOLID) {
                axisLine--;
            }
        }
        fangle = FMOD(axisPtr->tickAngle, 90.0f);
        if (fangle == 0.0) {
            axisPtr->tickAnchor = TK_ANCHOR_N;
        } else {
            int quadrant;

            quadrant = (int)(axisPtr->tickAngle / 90.0);
            if ((quadrant == 0) || (quadrant == 2)) {
                axisPtr->tickAnchor = TK_ANCHOR_NE;
            } else {
                axisPtr->tickAnchor = TK_ANCHOR_NW;
            }
        }
        axisPtr->left = axisPtr->screenMin - inset - 2;
        axisPtr->right = axisPtr->screenMin + axisPtr->screenRange + inset - 1;
        axisPtr->top = graphPtr->y2 + labelOffset - t1;
        mark = graphPtr->y2 + graphPtr->plotBorderWidth +
            axisPtr->marginPtr->nextLayerOffset;
        if (graphPtr->flags & STACK_AXES) {
            axisPtr->bottom = mark + axisPtr->marginPtr->axesOffset - 1;
        } else {
            axisPtr->bottom = mark + axisPtr->height - 1;
        }
        if (axisPtr->titleAlternate) {
            x = graphPtr->x2 + AXIS_PAD_TITLE;
            y = mark + (axisPtr->height / 2);
            axisPtr->titleAnchor = TK_ANCHOR_W; 
        } else {
            x = (axisPtr->right + axisPtr->left) / 2;
            if (graphPtr->flags & STACK_AXES) {
                y = mark + axisPtr->marginPtr->axesOffset - AXIS_PAD_TITLE;
            } else {
                y = mark + axisPtr->height - AXIS_PAD_TITLE;
            }
            axisPtr->titleAnchor = TK_ANCHOR_S; 
        }
        axisPtr->titlePos.x = x;
        axisPtr->titlePos.y = y;
        infoPtr->colorbar = axisLine - axisPtr->colorbar.thickness;
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
         * G = graph border width
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
         * G = graph border width
         * H = highlight thickness
         */
        axisLine = graphPtr->x1 - axisPtr->marginPtr->nextLayerOffset;
        if (axisPtr->colorbar.thickness > 0) {
            axisLine -= axisPtr->colorbar.thickness + COLORBAR_PAD;
        }
        if (axisPtr->flags & EXTERIOR) {
            axisLine -= graphPtr->plotBorderWidth + axisPad +
                axisPtr->lineWidth / 2;
            if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
#ifdef notdef
                axisLine++;
#endif
            } else {
                axisLine -= 3;
            }
            tickLabel = axisLine - 2;
            if (axisPtr->lineWidth > 0) {
                tickLabel -= axisPtr->tickLength;
            }
        } else {
            if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
                axisLine--;
            }
            axisLine += axisPad + axisPtr->lineWidth / 2;
            tickLabel = graphPtr->x1 - graphPtr->plotBorderWidth - 2;
        }
        axisPtr->tickAnchor = TK_ANCHOR_E;
        mark = graphPtr->x1 - graphPtr->plotBorderWidth -
            axisPtr->marginPtr->nextLayerOffset;
        if (graphPtr->flags & STACK_AXES) {
            axisPtr->left = mark - axisPtr->marginPtr->axesOffset;
        } else {
            axisPtr->left = mark - axisPtr->width;
        }
        axisPtr->right = mark - 3;
        axisPtr->top = axisPtr->screenMin - inset - 2;
        axisPtr->bottom = axisPtr->screenMin + axisPtr->screenRange + inset - 1;
        if (axisPtr->titleAlternate) {
            x = mark - (axisPtr->width / 2);
            y = graphPtr->y1 - AXIS_PAD_TITLE;
            axisPtr->titleAnchor = TK_ANCHOR_SW; 
        } else {
            if (graphPtr->flags & STACK_AXES) {
                x = mark - axisPtr->marginPtr->axesOffset;
            } else {
                x = mark - axisPtr->width + AXIS_PAD_TITLE;
            }
            y = (axisPtr->bottom + axisPtr->top) / 2;
            axisPtr->titleAnchor = TK_ANCHOR_W; 
        } 
        axisPtr->titlePos.x = x;
        axisPtr->titlePos.y = y;
        infoPtr->colorbar = axisLine;
        break;

    case MARGIN_RIGHT:
        axisLine = graphPtr->x2 + axisPtr->marginPtr->nextLayerOffset;
        if (axisPtr->colorbar.thickness > 0) {
            axisLine += axisPtr->colorbar.thickness + COLORBAR_PAD;
        }
        if (axisPtr->flags & EXTERIOR) {
            axisLine += graphPtr->plotBorderWidth + axisPad + axisPtr->lineWidth / 2;
            tickLabel = axisLine + 2;
            if (axisPtr->lineWidth > 0) {
                tickLabel += axisPtr->tickLength;
            }
            if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
                axisLine--;
            } else {
                axisLine++;
            }
        } else {
            if (graphPtr->plotRelief == TK_RELIEF_SOLID) {

#ifdef notdef
                axisLine--;                 /* Draw axis line within solid
                                             * plot border. */
#endif
            } 
            axisLine -= axisPad + axisPtr->lineWidth / 2;
            tickLabel = axisLine + 2;
        }
        mark = graphPtr->x2 + axisPtr->marginPtr->nextLayerOffset + pad;
        axisPtr->tickAnchor = TK_ANCHOR_W;
        axisPtr->left = mark;
        if (graphPtr->flags & STACK_AXES) {
            axisPtr->right = mark + axisPtr->marginPtr->axesOffset - 1;
        } else {
            axisPtr->right = mark + axisPtr->width - 1;
        }
        axisPtr->top = axisPtr->screenMin - inset - 2;
        axisPtr->bottom = axisPtr->screenMin + axisPtr->screenRange + inset -1;
        if (axisPtr->titleAlternate) {
            x = mark + (axisPtr->width / 2);
            y = graphPtr->y1 - AXIS_PAD_TITLE;
            axisPtr->titleAnchor = TK_ANCHOR_SE; 
        } else {
            if (graphPtr->flags & STACK_AXES) {
                x = mark + axisPtr->marginPtr->axesOffset - AXIS_PAD_TITLE;
            } else {
                x = mark + axisPtr->width - AXIS_PAD_TITLE;
            }
            y = (axisPtr->bottom + axisPtr->top) / 2;
            axisPtr->titleAnchor = TK_ANCHOR_E;
        }
        axisPtr->titlePos.x = x;
        axisPtr->titlePos.y = y;
        infoPtr->colorbar = axisLine - axisPtr->colorbar.thickness;
        break;

    case MARGIN_NONE:
        axisLine = 0;
        break;
    }
    if ((axisPtr->marginPtr->side == MARGIN_LEFT) ||
        (axisPtr->marginPtr->side == MARGIN_TOP)) {
        t1 = -t1, t2 = -t2;
        labelOffset = -labelOffset;
    }
    infoPtr->axisLength = axisLine;
    infoPtr->t1 = axisLine + t1;
    infoPtr->t2 = axisLine + t2;
    if (tickLabel > 0) {
        infoPtr->label = tickLabel;
    } else {
        infoPtr->label = axisLine + labelOffset;
    }
    if ((axisPtr->flags & EXTERIOR) == 0) {
        /*infoPtr->label = axisLine + labelOffset - t1; */
        infoPtr->t1 = axisLine - t1;
        infoPtr->t2 = axisLine - t2;
    } 
}

static void
MakeColorbar(Axis *axisPtr, AxisInfo *infoPtr)
{
    double min, max;
    int x1, y1, x2, y2;
    min = axisPtr->axisRange.min;
    max = axisPtr->axisRange.max;
    if (IsLogScale(axisPtr)) {
        min = EXP10(min);
        max = EXP10(max);
    }
    if (HORIZONTAL(axisPtr->marginPtr)) {
        x2 = Blt_HMap(axisPtr, min);
        x1 = Blt_HMap(axisPtr, max);
        axisPtr->colorbar.rect.x = MIN(x1, x2);
        axisPtr->colorbar.rect.y = infoPtr->colorbar;
        axisPtr->colorbar.rect.width = ABS(x1 - x2) + 1;
        axisPtr->colorbar.rect.height = axisPtr->colorbar.thickness;
    } else {
        y2 = Blt_VMap(axisPtr, min);
        y1 = Blt_VMap(axisPtr, max);
        axisPtr->colorbar.rect.x = infoPtr->colorbar;
        axisPtr->colorbar.rect.y = MIN(y1,y2);
        axisPtr->colorbar.rect.height = ABS(y1 - y2) + 1;
        axisPtr->colorbar.rect.width = axisPtr->colorbar.thickness;
    }
}

static void
MakeAxisLine(Axis *axisPtr, int line, Segment2d *s)
{
    double min, max;

    min = axisPtr->axisRange.min;
    max = axisPtr->axisRange.max;
    if (IsLogScale(axisPtr)) {
        min = EXP10(min);
        max = EXP10(max);
    }
    if (HORIZONTAL(axisPtr->marginPtr)) {
        s->p.x = Blt_HMap(axisPtr, min);
        s->q.x = Blt_HMap(axisPtr, max);
        s->p.y = s->q.y = line;
    } else {
        s->q.x = s->p.x = line;
        s->p.y = Blt_VMap(axisPtr, min);
        s->q.y = Blt_VMap(axisPtr, max);
    }
}


static void
MakeTick(Axis *axisPtr, double value, int tick, int line, Segment2d *s)
{
    if (IsLogScale(axisPtr)) {
        value = EXP10(value);
    }
    if (HORIZONTAL(axisPtr->marginPtr)) {
        s->p.x = s->q.x = Blt_HMap(axisPtr, value);
        s->p.y = line;
        s->q.y = tick;
    } else {
        s->p.x = line;
        s->p.y = s->q.y = Blt_VMap(axisPtr, value);
        s->q.x = tick;
    }
}

static void
MakeSegments(Axis *axisPtr, AxisInfo *infoPtr)
{
    int arraySize;
    int numMajorTicks, numMinorTicks;
    Segment2d *segments;
    Segment2d *s;

    if (axisPtr->segments != NULL) {
        Blt_Free(axisPtr->segments);
    }
    numMajorTicks = axisPtr->major.ticks.numSteps;
    numMinorTicks = axisPtr->minor.ticks.numSteps;
    arraySize = 1 + numMajorTicks + (numMajorTicks * (numMinorTicks + 1));
    segments = Blt_AssertMalloc(arraySize * sizeof(Segment2d));
    s = segments;
    if (axisPtr->lineWidth > 0) {
        /* Axis baseline */
        MakeAxisLine(axisPtr, infoPtr->axisLength, s);
        s++;
    }
    if (axisPtr->flags & TICKLABELS) {
        Blt_ChainLink link;
        double labelPos;
        Tick left, right;

        link = Blt_Chain_FirstLink(axisPtr->tickLabels);
        labelPos = (double)infoPtr->label;
        for (left = FirstMajorTick(axisPtr); left.isValid; left = right) {

            right = NextMajorTick(axisPtr);
            if (right.isValid) {
                Tick minor;

                /* If this isn't the last major tick, add minor ticks. */
                axisPtr->minor.ticks.range = right.value - left.value;
                axisPtr->minor.ticks.initial = left.value;
                for (minor = FirstMinorTick(axisPtr); minor.isValid; 
                     minor = NextMinorTick(axisPtr)) {
                    if (InRange(minor.value, &axisPtr->axisRange)) {
                        /* Add minor tick. */
                        MakeTick(axisPtr, minor.value, infoPtr->t2, 
                                infoPtr->axisLength, s);
                        s++;
                    }
                }        
            }
            if (InRange(left.value, &axisPtr->axisRange)) {
                double mid;

                /* Add major tick. This could be the last major tick. */
                MakeTick(axisPtr, left.value, infoPtr->t1,
                        infoPtr->axisLength, s);
                
                mid = left.value;
                if ((axisPtr->labelOffset) && (right.isValid)) {
                    mid = (right.value - left.value) * 0.5;
                }
                if (InRange(mid, &axisPtr->axisRange)) {
                    TickLabel *labelPtr;

                    labelPtr = Blt_Chain_GetValue(link);
                    link = Blt_Chain_NextLink(link);
                    
                    /* Set the position of the tick label. */
                    if (HORIZONTAL(axisPtr->marginPtr)) {
                        labelPtr->anchorPos.x = s->p.x;
                        labelPtr->anchorPos.y = labelPos;
                    } else {
                        labelPtr->anchorPos.x = labelPos;
                        labelPtr->anchorPos.y = s->p.y;
                    }
                }
                s++;
            }
        }
    }
    axisPtr->segments = segments;
    axisPtr->numSegments = s - segments;
#ifdef notdef
    fprintf(stderr,
            "axis=%s numSegments=%d, arraySize=%d numMajor=%d numMinor=%d\n",
            axisPtr->obj.name, axisPtr->numSegments, arraySize,
            numMajorTicks, numMinorTicks);
#endif
    assert(axisPtr->numSegments <= arraySize);
}

/*
 *---------------------------------------------------------------------------
 *
 * MapAxis --
 *
 *      Pre-calculates positions of the axis, ticks, and labels (to be used
 *      later when displaying the axis).  Calculates the values for each
 *      major and minor tick and checks to see if they are in range (the
 *      outer ticks may be outside of the range of plotted values).
 *
 *      Line segments for the minor and major ticks are saved into one
 *      XSegment array so that they can be drawn by a single XDrawSegments
 *      call. The positions of the tick labels are also computed and saved.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Line segments and tick labels are saved and used later to draw the
 *      axis.
 *
 *---------------------------------------------------------------------------
 */
static void
MapAxis(Axis *axisPtr)
{
    AxisInfo info;
    
    AxisOffsets(axisPtr, &info);
    MakeSegments(axisPtr, &info);
    if (axisPtr->colorbar.thickness > 0) {
        MakeColorbar(axisPtr, &info);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapStackedAxis --
 *
 *      Pre-calculates positions of the axis, ticks, and labels (to be used
 *      later when displaying the axis).  Calculates the values for each
 *      major and minor tick and checks to see if they are in range (the
 *      outer ticks may be outside of the range of plotted values).
 *
 *      Line segments for the minor and major ticks are saved into one
 *      XSegment array so that they can be drawn by a single XDrawSegments
 *      call. The positions of the tick labels are also computed and saved.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Line segments and tick labels are saved and used later to draw the
 *      axis.
 *
 *---------------------------------------------------------------------------
 */
static void
MapStackedAxis(Axis *axisPtr, float totalWeight)
{
    AxisInfo info;
    Graph *graphPtr = axisPtr->obj.graphPtr;
    unsigned int w, h, n, slice;
    float ratio;

    n = axisPtr->marginPtr->numVisibleAxes;
    if ((n > 1) && (axisPtr->reqNumMajorTicks <= 0)) {
        axisPtr->reqNumMajorTicks = 4;
    }
    ratio = axisPtr->weight / totalWeight;
    if (HORIZONTAL(axisPtr->marginPtr)) {
        axisPtr->screenMin = graphPtr->hOffset;
        axisPtr->screenRange = graphPtr->hRange;
        slice = (int)((graphPtr->hRange - graphPtr->axisSpacing * (n - 1)) *
                      ratio);
        axisPtr->width = slice;
    } else {
        axisPtr->screenMin = graphPtr->vOffset;
        axisPtr->screenRange = graphPtr->vRange;
        slice = (int)((graphPtr->vRange - graphPtr->axisSpacing * (n - 1)) *
                      ratio);
        axisPtr->height = slice;
    }
    Blt_GetTextExtents(axisPtr->tickFont, 0, "0", 1, &w, &h);
    if (n > 1) {
        axisPtr->screenMin += axisPtr->marginPtr->nextStackOffset + + h / 2;
        axisPtr->screenRange = slice - h;
        if ((axisPtr->flags & HIDDEN) == 0) {
            axisPtr->marginPtr->nextStackOffset += slice +
                graphPtr->axisSpacing;
        }
    }
    axisPtr->screenScale = 1.0f / axisPtr->screenRange;
    AxisOffsets(axisPtr, &info);
    MakeSegments(axisPtr, &info);
}

/*
 *---------------------------------------------------------------------------
 *
 * AdjustViewport --
 *
 *      Adjusts the offsets of the viewport according to the scroll mode.
 *      This is to accommodate both "listbox" and "canvas" style scrolling.
 *
 *      "canvas"        The viewport scrolls within the range of world
 *                      coordinates.  This way the viewport always displays
 *                      a full page of the world.  If the world is smaller
 *                      than the viewport, then (bizarrely) the world and
 *                      viewport are inverted so that the world moves up
 *                      and down within the viewport.
 *
 *      "listbox"       The viewport can scroll beyond the range of world
 *                      coordinates.  Every entry can be displayed at the
 *                      top of the viewport.  This also means that the
 *                      scrollbar thumb weirdly shrinks as the last entry
 *                      is scrolled upward.
 *
 * Results:
 *      The corrected offset is returned.
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
        if (Tcl_GetDoubleFromObj(interp, objv[1], &fract) != TCL_OK) {
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

static int
GradientCalcProc(ClientData clientData, int x, int y, double *valuePtr)
{
    Axis *axisPtr = clientData;
    double value;
    Graph *graphPtr;

    graphPtr = axisPtr->obj.graphPtr;
    if ((axisPtr->marginPtr->side == MARGIN_Y) ||
        (axisPtr->marginPtr->side == MARGIN_Y2)) {
        if (graphPtr->flags & INVERTED) {
            value = Blt_InvHMap(axisPtr, x + axisPtr->screenMin);
        } else {
            value = Blt_InvVMap(axisPtr, y + axisPtr->screenMin);
        }
    } else if ((axisPtr->marginPtr->side == MARGIN_X) ||
               (axisPtr->marginPtr->side == MARGIN_X2)) {
        if (graphPtr->flags & INVERTED) {
            value = Blt_InvVMap(axisPtr, y + axisPtr->screenMin);
        } else {
            value = Blt_InvHMap(axisPtr, x + axisPtr->screenMin);
        }
    } else {
        return TCL_ERROR;
    }
    *valuePtr = (value - axisPtr->valueRange.min) / axisPtr->valueRange.range;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ColorbarToPicture --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Picture
ColorbarToPicture(Axis *axisPtr, int w, int h)
{
    Graph *graphPtr;
    Blt_Picture picture;

    if (axisPtr->palette == NULL) {
        return NULL;                    /* No palette defined. */
    }
    graphPtr = axisPtr->obj.graphPtr;
    picture = Blt_CreatePicture(w, h);
    if (picture != NULL) {
        Blt_PaintBrush brush;

        Blt_BlankPicture(picture, Blt_Bg_GetColor(graphPtr->normalBg));
        brush = Blt_NewLinearGradientBrush();
        Blt_SetLinearGradientBrushPalette(brush, axisPtr->palette);
        Blt_SetLinearGradientBrushCalcProc(brush, GradientCalcProc, axisPtr);
        Blt_PaintRectangle(picture, 0, 0, w, h, 0, 0, brush, TRUE);
        Blt_FreeBrush(brush);
        return picture;
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawColorbar --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawColorbar(Axis *axisPtr, Drawable drawable)
{
    Blt_Painter painter;
    Blt_Picture picture;
    Graph *graphPtr;
    XRectangle *rectPtr;

    if (axisPtr->palette == NULL) {
        return;                         /* No palette defined. */
    }
    graphPtr = axisPtr->obj.graphPtr;
    rectPtr = &axisPtr->colorbar.rect;
    picture = ColorbarToPicture(axisPtr, rectPtr->width, rectPtr->height);
    if (picture == NULL) {
        return;                         /* Background is obscured. */
    }
    painter = Blt_GetPainter(graphPtr->tkwin, 1.0);
    Blt_PaintPicture(painter, drawable, picture, 0, 0, rectPtr->width, 
                     rectPtr->height, rectPtr->x, rectPtr->y, 0);
    Blt_FreePicture(picture);
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawAxis --
 *
 *      Draws the axis, ticks, and labels onto the canvas.
 *
 *      Initializes and passes text attribute information through TextStyle
 *      structure.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawAxis(Axis *axisPtr, Drawable drawable)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    int isHoriz;

    if (axisPtr->normalBg != NULL) {
        Blt_Bg_FillRectangle(graphPtr->tkwin, drawable, 
                axisPtr->normalBg, 
                axisPtr->left, axisPtr->top, 
                axisPtr->right - axisPtr->left, 
                axisPtr->bottom - axisPtr->top, axisPtr->borderWidth, 
                axisPtr->relief);
    }
    if (axisPtr->colorbar.thickness > 0) {
        DrawColorbar(axisPtr, drawable);
    }
    if (axisPtr->title != NULL) {
        TextStyle ts;

        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetAngle(ts, axisPtr->titleAngle);
        Blt_Ts_SetFont(ts, axisPtr->titleFont);
        Blt_Ts_SetPadding(ts, 1, 2, 0, 0);
        Blt_Ts_SetAnchor(ts, axisPtr->titleAnchor);
        Blt_Ts_SetJustify(ts, axisPtr->titleJustify);
        if (axisPtr->flags & ACTIVE) {
            Blt_Ts_SetForeground(ts, axisPtr->activeFgColor);
        } else {
            Blt_Ts_SetForeground(ts, axisPtr->titleColor);
        }
        Blt_Ts_SetForeground(ts, axisPtr->titleColor);
        if ((axisPtr->titleAngle == 90.0) || (axisPtr->titleAngle == 270.0)) {
            Blt_Ts_SetMaxLength(ts, axisPtr->height);
        } else {
            Blt_Ts_SetMaxLength(ts, axisPtr->width);
        }
        Blt_Ts_DrawText(graphPtr->tkwin, drawable, axisPtr->title, -1, &ts, 
                (int)axisPtr->titlePos.x, (int)axisPtr->titlePos.y);
    }
    if (axisPtr->scrollCmdObjPtr != NULL) {
        double viewWidth, viewMin, viewMax;
        double worldWidth, worldMin, worldMax;
        double fract;

        worldMin = axisPtr->valueRange.min;
        worldMax = axisPtr->valueRange.max;
        if (DEFINED(axisPtr->scrollMin)) {
            worldMin = axisPtr->scrollMin;
        }
        if (DEFINED(axisPtr->scrollMax)) {
            worldMax = axisPtr->scrollMax;
        }
        viewMin = axisPtr->min;
        viewMax = axisPtr->max;
        if (viewMin < worldMin) {
            viewMin = worldMin;
        }
        if (viewMax > worldMax) {
            viewMax = worldMax;
        }
        if (IsLogScale(axisPtr)) {
            worldMin = log10(worldMin);
            worldMax = log10(worldMax);
            viewMin = log10(viewMin);
            viewMax = log10(viewMax);
        }
        worldWidth = worldMax - worldMin;       
        viewWidth = viewMax - viewMin;
        isHoriz = HORIZONTAL(axisPtr->marginPtr);

        if (isHoriz != axisPtr->decreasing) {
            fract = (viewMin - worldMin) / worldWidth;
        } else {
            fract = (worldMax - viewMax) / worldWidth;
        }
        fract = AdjustViewport(fract, viewWidth / worldWidth);

        if (isHoriz != axisPtr->decreasing) {
            viewMin = (fract * worldWidth);
            axisPtr->min = viewMin + worldMin;
            axisPtr->max = axisPtr->min + viewWidth;
            viewMax = viewMin + viewWidth;
            if (IsLogScale(axisPtr)) {
                axisPtr->min = EXP10(axisPtr->min);
                axisPtr->max = EXP10(axisPtr->max);
            }
            Blt_UpdateScrollbar(graphPtr->interp, axisPtr->scrollCmdObjPtr,
                (int)viewMin, (int)viewMax, (int)worldWidth);
        } else {
            viewMax = (fract * worldWidth);
            axisPtr->max = worldMax - viewMax;
            axisPtr->min = axisPtr->max - viewWidth;
            viewMin = viewMax + viewWidth;
            if (IsLogScale(axisPtr)) {
                axisPtr->min = EXP10(axisPtr->min);
                axisPtr->max = EXP10(axisPtr->max);
            }
            Blt_UpdateScrollbar(graphPtr->interp, axisPtr->scrollCmdObjPtr,
                (int)viewMax, (int)viewMin, (int)worldWidth);
        }
    }
    if (axisPtr->flags & TICKLABELS) {
        Blt_ChainLink link;
        TextStyle ts;

        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetAngle(ts, axisPtr->tickAngle);
        Blt_Ts_SetFont(ts, axisPtr->tickFont);
        Blt_Ts_SetPadding(ts, 2, 0, 0, 0);
        Blt_Ts_SetAnchor(ts, axisPtr->tickAnchor);
        if (axisPtr->flags & ACTIVE) {
            Blt_Ts_SetForeground(ts, axisPtr->activeFgColor);
        } else {
            Blt_Ts_SetForeground(ts, axisPtr->tickColor);
        }
        for (link = Blt_Chain_FirstLink(axisPtr->tickLabels); link != NULL;
            link = Blt_Chain_NextLink(link)) {  
            TickLabel *labelPtr;

            labelPtr = Blt_Chain_GetValue(link);
            /* Draw major tick labels */
            Blt_DrawText(graphPtr->tkwin, drawable, labelPtr->string, &ts, 
                (int)labelPtr->anchorPos.x, (int)labelPtr->anchorPos.y);
        }
    }
    if ((axisPtr->numSegments > 0) && (axisPtr->lineWidth > 0)) {       
        GC gc;

        gc = (axisPtr->flags & ACTIVE) ? axisPtr->activeTickGC :
            axisPtr->tickGC;
        /* Draw the tick marks and axis line. */
        Blt_DrawSegments2d(graphPtr->display, drawable, gc, axisPtr->segments, 
                axisPtr->numSegments);
    }
}

static void
ColorbarToPostScript(Axis *axisPtr, Blt_Ps ps)
{
    Blt_Picture picture;
    XRectangle *rectPtr;

    Blt_Ps_Format(ps, "%% Axis \"%s\" colorbar \n", axisPtr->obj.name);
    if (axisPtr->palette == NULL) {
        return;                         /* No palette defined. */
    }
    rectPtr = &axisPtr->colorbar.rect;
    picture = ColorbarToPicture(axisPtr, rectPtr->width, rectPtr->height);
    if (picture != NULL) {
        Blt_Ps_DrawPicture(ps, picture, rectPtr->x, rectPtr->y);
        Blt_FreePicture(picture);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisToPostScript --
 *
 *      Generates PostScript output to draw the axis, ticks, and labels.
 *
 *      Initializes and passes text attribute information through TextStyle
 *      structure.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      PostScript output is left in graphPtr->interp->result;
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
AxisToPostScript(Axis *axisPtr, Blt_Ps ps)
{
    Blt_Ps_Format(ps, "%% Axis \"%s\"\n", axisPtr->obj.name);
    if (axisPtr->normalBg != NULL) {
        Tk_3DBorder border;

        border = Blt_Bg_Border(axisPtr->normalBg);
        Blt_Ps_Fill3DRectangle(ps, border, 
                (double)axisPtr->left, (double)axisPtr->top, 
                axisPtr->right - axisPtr->left, axisPtr->bottom - axisPtr->top, 
                axisPtr->borderWidth, axisPtr->relief);
    }
    if (axisPtr->title != NULL) {
        TextStyle ts;

        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetAngle(ts, axisPtr->titleAngle);
        Blt_Ts_SetFont(ts, axisPtr->titleFont);
        Blt_Ts_SetPadding(ts, 1, 2, 0, 0);
        Blt_Ts_SetAnchor(ts, axisPtr->titleAnchor);
        Blt_Ts_SetJustify(ts, axisPtr->titleJustify);
        Blt_Ts_SetForeground(ts, axisPtr->titleColor);
        Blt_Ps_DrawText(ps, axisPtr->title, &ts, axisPtr->titlePos.x, 
                axisPtr->titlePos.y);
    }
    if (axisPtr->flags & TICKLABELS) {
        Blt_ChainLink link;
        TextStyle ts;

        Blt_Ts_InitStyle(ts);
        Blt_Ts_SetAngle(ts, axisPtr->tickAngle);
        Blt_Ts_SetFont(ts, axisPtr->tickFont);
        Blt_Ts_SetPadding(ts, 2, 0, 0, 0);
        Blt_Ts_SetAnchor(ts, axisPtr->tickAnchor);
        Blt_Ts_SetForeground(ts, axisPtr->tickColor);

        for (link = Blt_Chain_FirstLink(axisPtr->tickLabels); link != NULL; 
             link = Blt_Chain_NextLink(link)) {
            TickLabel *labelPtr;

            labelPtr = Blt_Chain_GetValue(link);
            Blt_Ps_DrawText(ps, labelPtr->string, &ts, labelPtr->anchorPos.x, 
                labelPtr->anchorPos.y);
        }
    }
    if (axisPtr->colorbar.thickness > 0) {
        ColorbarToPostScript(axisPtr, ps);
    }
    if ((axisPtr->numSegments > 0) && (axisPtr->lineWidth > 0)) {
        Blt_Ps_XSetLineAttributes(ps, axisPtr->tickColor, axisPtr->lineWidth, 
                (Blt_Dashes *)NULL, CapButt, JoinMiter);
        Blt_Ps_DrawSegments2d(ps, axisPtr->numSegments, axisPtr->segments);
    }
}

static void
MakeGridLine(Axis *axisPtr, double value, Segment2d *s)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;

    if (IsLogScale(axisPtr)) {
        value = EXP10(value);
    }
    /* Grid lines run orthogonally to the axis */
    if (HORIZONTAL(axisPtr->marginPtr)) {
        s->p.y = graphPtr->y1;
        s->q.y = graphPtr->y2;
        s->p.x = s->q.x = Blt_HMap(axisPtr, value);
    } else {
        s->p.x = graphPtr->x1;
        s->q.x = graphPtr->x2;
        s->p.y = s->q.y = Blt_VMap(axisPtr, value);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MapGridlines --
 *
 *      Assembles the grid lines associated with an axis. Generates tick
 *      positions if necessary (this happens when the axis is not a logical
 *      axis too).
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
MapGridlines(Axis *axisPtr)
{
    Segment2d *s1, *s2;
    Ticks *majorPtr, *minorPtr;
    int needed;
    Tick left, right;
    
    if (axisPtr == NULL) {
        return;
    }
    majorPtr = &axisPtr->major.ticks;
    minorPtr = &axisPtr->minor.ticks;
    needed = majorPtr->numSteps;
    if (axisPtr->flags & GRIDMINOR) {
        needed += (majorPtr->numSteps * minorPtr->numSteps);
    }
    if (needed == 0) {
        return;                 
    }
    needed = majorPtr->numSteps;
    if (needed != axisPtr->major.grid.numAllocated) {
        if (axisPtr->major.grid.segments != NULL) {
            Blt_Free(axisPtr->major.grid.segments);
        }
        axisPtr->major.grid.segments = 
            Blt_AssertMalloc(sizeof(Segment2d) * needed);
        axisPtr->major.grid.numAllocated = needed;
    }
    needed = (majorPtr->numSteps * minorPtr->numSteps);
    if (needed != axisPtr->minor.grid.numAllocated) {
        if (axisPtr->minor.grid.segments != NULL) {
            Blt_Free(axisPtr->minor.grid.segments);
        }
        axisPtr->minor.grid.segments = 
            Blt_AssertMalloc(sizeof(Segment2d) * needed);
        axisPtr->minor.grid.numAllocated = needed;
    }
    s1 = axisPtr->major.grid.segments;
    s2 = axisPtr->minor.grid.segments;
    for (left = FirstMajorTick(axisPtr); left.isValid; left = right) {
        right = NextMajorTick(axisPtr);

        /* If this isn't the last major tick, add minor grid lines. */
        if ((axisPtr->flags & GRIDMINOR) && (right.isValid)) {
            Tick minor;
            
            axisPtr->minor.ticks.range = right.value - left.value;
            axisPtr->minor.ticks.initial = left.value;
            for (minor = FirstMinorTick(axisPtr); minor.isValid; 
                 minor = NextMinorTick(axisPtr)) {
                if (InRange(minor.value, &axisPtr->axisRange)) {
                    MakeGridLine(axisPtr, minor.value, s2);
                    s2++;
                }
            }
        }
        if (InRange(left.value, &axisPtr->axisRange)) {
            MakeGridLine(axisPtr, left.value, s1);
            s1++;
        }
    }
    axisPtr->major.grid.numUsed = s1 - axisPtr->major.grid.segments;
    axisPtr->minor.grid.numUsed = s2 - axisPtr->minor.grid.segments;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetAxisGeometry --
 *
 * Results:
 *      None.
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
void
Blt_GetAxisGeometry(Graph *graphPtr, Axis *axisPtr)
{
    int y;

    FreeTickLabels(axisPtr->tickLabels);
    y = 0;

    if ((axisPtr->flags & EXTERIOR) && 
        (graphPtr->plotRelief != TK_RELIEF_SOLID)) {
        /* Leave room for axis baseline and padding */
        y += axisPtr->lineWidth + 2;
    }

    axisPtr->maxLabelHeight = axisPtr->maxLabelWidth = 0;
    if (axisPtr->flags & TICKLABELS) {
        unsigned int pad;
        unsigned int numTicks;
        Tick left, right;

        numTicks = axisPtr->major.ticks.numSteps;
        assert(numTicks <= MAXTICKS);
        for (left = FirstMajorTick(axisPtr); left.isValid; left = right) {
            TickLabel *labelPtr;
            double mid;
            int lw, lh;                 /* Label width and height. */

            right = NextMajorTick(axisPtr);
            mid = left.value;
            if ((axisPtr->labelOffset) && (right.isValid)) {
                mid = (right.value - left.value) * 0.5;
            }
            if (!InRange(mid, &axisPtr->axisRange)) {
                continue;
            }
            labelPtr = MakeLabel(axisPtr, left.value);
            Blt_Chain_Append(axisPtr->tickLabels, labelPtr);
            /* 
             * Get the dimensions of each tick label.  Remember tick labels
             * can be multi-lined and/or rotated.
             */
            Blt_GetTextExtents(axisPtr->tickFont, 0, labelPtr->string, -1, 
                &labelPtr->width, &labelPtr->height);

            if (axisPtr->tickAngle != 0.0f) {
                double rlw, rlh;        /* Rotated label width and height. */
                Blt_GetBoundingBox(labelPtr->width, labelPtr->height, 
                        axisPtr->tickAngle, &rlw, &rlh, NULL);
                lw = ROUND(rlw), lh = ROUND(rlh);
            } else {
                lw = labelPtr->width;
                lh = labelPtr->height;
            }
            if (axisPtr->maxLabelWidth < lw) {
                axisPtr->maxLabelWidth = lw;
            }
            if (axisPtr->maxLabelHeight < lh) {
                axisPtr->maxLabelHeight = lh;
            }
        }
        assert(Blt_Chain_GetLength(axisPtr->tickLabels) <= numTicks);
        
        pad = 0;
        if (axisPtr->flags & EXTERIOR) {
            /* Because the axis cap style is "CapProjecting", we need to
             * account for an extra 1.5 linewidth at the end of each line.  */
            pad = ((axisPtr->lineWidth * 12) / 8);
        }
        if (HORIZONTAL(axisPtr->marginPtr)) {
            y += axisPtr->maxLabelHeight + pad;
        } else {
            y += axisPtr->maxLabelWidth + pad;
            if (axisPtr->maxLabelWidth > 0) {
                y += 5;                 /* Pad either size of label. */
            }  
        }
        y += 2 * AXIS_PAD_TITLE;
        if ((axisPtr->lineWidth > 0) && (axisPtr->flags & EXTERIOR)) {
            /* Distance from axis line to tick label. */
            y += axisPtr->tickLength;
        }
    }

    if (axisPtr->title != NULL) {
        if (axisPtr->titleAlternate) {
            if (y < axisPtr->titleHeight) {
                y = axisPtr->titleHeight;
            }
        } else {
            y += axisPtr->titleHeight + AXIS_PAD_TITLE;
        }
    }
    /* Add in the bar thickness. */
    if (axisPtr->colorbar.thickness > 0) {
        y += axisPtr->colorbar.thickness + 4;
    }
    /* Correct for orientation of the axis. */
    if (HORIZONTAL(axisPtr->marginPtr)) {
        axisPtr->height = y;
    } else {
        axisPtr->width = y;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetMarginGeometry --
 *
 *      Examines all the axes in the given margin and determines the area
 *      required to display them.
 *
 *      Note: For multiple axes, the titles are displayed in another
 *            margin. So we must keep track of the widest title.
 *      
 * Results:
 *      Returns the width or height of the margin, depending if it runs
 *      horizontally along the graph or vertically.
 *
 * Side Effects:
 *      The area width and height set in the margin.  Note again that this
 *      may be corrected later (mulitple axes) to adjust for the longest
 *      title in another margin.
 *
 *---------------------------------------------------------------------------
 */
static int
GetMarginGeometry(Graph *graphPtr, Margin *marginPtr)
{
    Axis *axisPtr;
    int l, w, h;                        /* Length, width, and height. */
    int isHoriz;
    unsigned int numVisible;
    
    isHoriz = HORIZONTAL(marginPtr);

    /* Count the visible axes. */
    numVisible = 0;
    l = w = h = 0;
    /* Need to track the widest and tallest tick labels in the margin. */
    marginPtr->maxAxisLabelWidth = marginPtr->maxAxisLabelHeight = 0;
    if (graphPtr->flags & STACK_AXES) {
        for (axisPtr = FirstAxis(marginPtr); axisPtr != NULL;
             axisPtr = NextAxis(axisPtr)) {
            if (axisPtr->flags & DELETED) {
                continue;
            }
            if (graphPtr->flags & GET_AXIS_GEOMETRY) {
                Blt_GetAxisGeometry(graphPtr, axisPtr);
            }
            if (axisPtr->flags & HIDDEN) {
                continue;
            }
            numVisible++;
            if (isHoriz) {
                if (h < axisPtr->height) {
                    h = axisPtr->height;
                }
            } else {
                if (w < axisPtr->width) {
                    w = axisPtr->width;
                }
            }
            if (axisPtr->maxLabelWidth > marginPtr->maxAxisLabelWidth) {
                marginPtr->maxAxisLabelWidth = axisPtr->maxLabelWidth;
            }
            if (axisPtr->maxLabelHeight > marginPtr->maxAxisLabelHeight) {
                marginPtr->maxAxisLabelHeight = axisPtr->maxLabelHeight;
            }
        }
    } else {
        for (axisPtr = FirstAxis(marginPtr); axisPtr != NULL;
             axisPtr = NextAxis(axisPtr)) {
            if (axisPtr->flags & DELETED) {
                continue;
            }
            if (graphPtr->flags & GET_AXIS_GEOMETRY) {
                Blt_GetAxisGeometry(graphPtr, axisPtr);
            }
            if (axisPtr->flags & HIDDEN) {
                continue;
            }
            numVisible++;
            if ((axisPtr->titleAlternate) && (l < axisPtr->titleWidth)) {
                l = axisPtr->titleWidth;
            }
            if (isHoriz) {
                h += axisPtr->height;
            } else {
                w += axisPtr->width;
            }
            if (axisPtr->maxLabelWidth > marginPtr->maxAxisLabelWidth) {
                marginPtr->maxAxisLabelWidth = axisPtr->maxLabelWidth;
            }
            if (axisPtr->maxLabelHeight > marginPtr->maxAxisLabelHeight) {
                marginPtr->maxAxisLabelHeight = axisPtr->maxLabelHeight;
            }
        }
    }
    /* Enforce a minimum size for margins. */
    if (w < 3) {
        w = 3;
    }
    if (h < 3) {
        h = 3;
    }
    marginPtr->numVisibleAxes = numVisible;
    marginPtr->axesTitleLength = l;
    marginPtr->width = w;
    marginPtr->height = h;
    marginPtr->axesOffset = (isHoriz) ? h : w;
    return marginPtr->axesOffset;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LayoutGraph --
 *
 *      Calculate the layout of the graph.  Based upon the data, axis limits,
 *      X and Y titles, and title height, determine the cavity left which is
 *      the plotting surface.  The first step get the data and axis limits for
 *      calculating the space needed for the top, bottom, left, and right
 *      margins.
 *
 *      1) The LEFT margin is the area from the left border to the Y axis 
 *         (not including ticks). It composes the border width, the width an 
 *         optional Y axis label and its padding, and the tick numeric labels. 
 *         The Y axis label is rotated 90 degrees so that the width is the 
 *         font height.
 *
 *      2) The RIGHT margin is the area from the end of the graph
 *         to the right window border. It composes the border width,
 *         some padding, the font height (this may be dubious. It
 *         appears to provide a more even border), the max of the
 *         legend width and 1/2 max X tick number. This last part is
 *         so that the last tick label is not clipped.
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
 * ticks and its padding is 5% of the entire plotting area.  Hence the
 * entire plotting area is scaled as 105% of the width and height of the
 * area.
 *
 * The axis labels, ticks labels, title, and legend may or may not be
 * displayed which must be taken into account.
 *
 * if reqWidth > 0 : set outer size
 * if reqPlotWidth > 0 : set plot size
 *---------------------------------------------------------------------------
 */
void
Blt_LayoutGraph(Graph *graphPtr)
{
    int left, right, top, bottom;
    int plotWidth, plotHeight;
    int inset, inset2;
    int width, height;
    int pad;

    width = graphPtr->width;
    height = graphPtr->height;

    /* 
     * Step 1: Compute the amount of space needed to display the axes
     *         associated with each margin.  They can be overridden by
     *         -leftmargin, -rightmargin, -bottommargin, and -topmargin
     *         graph options, respectively.
     */
    left   = GetMarginGeometry(graphPtr, graphPtr->leftPtr);
    right  = GetMarginGeometry(graphPtr, graphPtr->rightPtr);
    top    = GetMarginGeometry(graphPtr, graphPtr->topPtr);
    bottom = GetMarginGeometry(graphPtr, graphPtr->bottomPtr);

    pad = graphPtr->bottomPtr->maxAxisLabelWidth;
    if (pad < graphPtr->topPtr->maxAxisLabelWidth) {
        pad = graphPtr->topPtr->maxAxisLabelWidth;
    }
    pad = pad / 2 + 3;
    if (right < pad) {
        right = pad;
    }
    if (left < pad) {
        left = pad;
    }
    pad = graphPtr->leftPtr->maxAxisLabelHeight;
    if (pad < graphPtr->rightPtr->maxAxisLabelHeight) {
        pad = graphPtr->rightPtr->maxAxisLabelHeight;
    }
    pad = pad / 2;
    if (top < pad) {
        top = pad;
    }
    if (bottom < pad) {
        bottom = pad;
    }

    if (graphPtr->reqLeftMarginSize > 0) {
        left = graphPtr->reqLeftMarginSize;
    }
    if (graphPtr->reqRightMarginSize > 0) {
        right = graphPtr->reqRightMarginSize;
    }
   if (graphPtr->reqTopMarginSize > 0) {
        top = graphPtr->reqTopMarginSize;
    }
    if (graphPtr->reqBottomMarginSize > 0) {
        bottom = graphPtr->reqBottomMarginSize;
    }

    /* 
     * Step 2:  Add the graph title height to the top margin. 
     */
    if (graphPtr->title != NULL) {
        top += graphPtr->titleHeight + 6;
    }
    inset = (graphPtr->inset + graphPtr->plotBorderWidth);
    inset2 = 2 * inset;

    /* 
     * Step 3: Estimate the size of the plot area from the remaining
     *         space.  This may be overridden by the -plotwidth and
     *         -plotheight graph options.  We use this to compute the
     *         size of the legend. 
     */
    if (width == 0) {
        width = 400;
    }
    if (height == 0) {
        height = 400;
    }
    plotWidth  = (graphPtr->reqPlotWidth > 0) ? graphPtr->reqPlotWidth :
        width - (inset2 + left + right); /* Plot width. */
    plotHeight = (graphPtr->reqPlotHeight > 0) ? graphPtr->reqPlotHeight : 
        height - (inset2 + top + bottom); /* Plot height. */
    Blt_MapLegend(graphPtr, plotWidth, plotHeight);

    /* 
     * Step 4:  Add the legend to the appropiate margin. 
     */
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
    /* 
     * Recompute the plotarea or graph size, now accounting for the legend. 
     */
    if (graphPtr->reqPlotWidth == 0) {
        plotWidth = width  - (inset2 + left + right);
        if (plotWidth < 1) {
            plotWidth = 1;
        }
    }
    if (graphPtr->reqPlotHeight == 0) {
        plotHeight = height - (inset2 + top + bottom);
        if (plotHeight < 1) {
            plotHeight = 1;
        }
    }
    /*
     * Step 5: If necessary, correct for the requested plot area aspect
     *         ratio.
     */
    if ((graphPtr->reqPlotWidth == 0) && (graphPtr->reqPlotHeight == 0) && 
        (graphPtr->aspect > 0.0f)) {
        float ratio;

        /* 
         * Shrink one dimension of the plotarea to fit the requested
         * width/height aspect ratio.
         */
        ratio = (float)plotWidth / (float)plotHeight;
        if (ratio > graphPtr->aspect) {
            int sw;

            /* Shrink the width. */
            sw = (int)(plotHeight * graphPtr->aspect);
            if (sw < 1) {
                sw = 1;
            }
            /* Add the difference to the right margin. */
            /* CHECK THIS: w = sw; */
            right += (plotWidth - sw);
        } else {
            int sh;

            /* Shrink the height. */
            sh = (int)(plotWidth / graphPtr->aspect);
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
     *         displayed in the adjoining margins.  Make sure there's room 
     *         for the longest axis titles.
     */
    if (top < graphPtr->leftPtr->axesTitleLength) {
        top = graphPtr->leftPtr->axesTitleLength;
    }
    if (right < graphPtr->bottomPtr->axesTitleLength) {
        right = graphPtr->bottomPtr->axesTitleLength;
    }
    if (top < graphPtr->rightPtr->axesTitleLength) {
        top = graphPtr->rightPtr->axesTitleLength;
    }
    if (right < graphPtr->topPtr->axesTitleLength) {
        right = graphPtr->topPtr->axesTitleLength;
    }

    /* 
     * Step 7: Override calculated values with requested margin sizes.
     */
    if (graphPtr->reqLeftMarginSize > 0) {
        left = graphPtr->reqLeftMarginSize;
    }
    if (graphPtr->reqRightMarginSize > 0) {
        right = graphPtr->reqRightMarginSize;
    }
    if (graphPtr->reqTopMarginSize > 0) {
        top = graphPtr->reqTopMarginSize;
    }
    if (graphPtr->reqBottomMarginSize > 0) {
        bottom = graphPtr->reqBottomMarginSize;
    }
    if (graphPtr->reqPlotWidth > 0) {   
        int w;

        /* 
         * Width of plotarea is constained.  If there's extra space, add it
         * to th left and/or right margins.  If there's too little, grow
         * the graph width to accomodate it.
         */
        w = plotWidth + inset2 + left + right;
        if (width > w) {                /* Extra space in window. */
            int extra;

            extra = (width - w) / 2;
            if (graphPtr->reqLeftMarginSize == 0) { 
                left += extra;
                if (graphPtr->reqRightMarginSize == 0) { 
                    right += extra;
                } else {
                    left += extra;
                }
            } else if (graphPtr->reqRightMarginSize == 0) {
                right += extra + extra;
            }
        } else if (width < w) {
            width = w;
        }
    } 
    if (graphPtr->reqPlotHeight > 0) {  /* Constrain the plotarea height. */
        int h;

        /* 
         * Height of plotarea is constained.  If there's extra space, add
         * it to th top and/or bottom margins.  If there's too little, grow
         * the graph height to accomodate it.
         */
        h = plotHeight + inset2 + top + bottom;
        if (height > h) {               /* Extra space in window. */
            int extra;

            extra = (height - h) / 2;
            if (graphPtr->reqTopMarginSize == 0) { 
                top += extra;
                if (graphPtr->reqBottomMarginSize == 0) { 
                    bottom += extra;
                } else {
                    top += extra;
                }
            } else if (graphPtr->reqBottomMarginSize == 0) {
                bottom += extra + extra;
            }
        } else if (height < h) {
            height = h;
        }
    }   
    graphPtr->width  = width;
    graphPtr->height = height;
    graphPtr->x1 = left + inset;
    graphPtr->y1 = top  + inset;
    graphPtr->x2 = width  - right  - inset;
    graphPtr->y2 = height - bottom - inset;
    if (graphPtr->plotRelief == TK_RELIEF_SOLID) {
        graphPtr->x1--;
        graphPtr->y1--;
    }
    graphPtr->leftPtr->width    = left   + graphPtr->inset;
    graphPtr->rightPtr->width   = right  + graphPtr->inset;
    graphPtr->topPtr->height    = top    + graphPtr->inset;
    graphPtr->bottomPtr->height = bottom + graphPtr->inset;
            
    graphPtr->vOffset = graphPtr->y1 + graphPtr->padTop;
    graphPtr->vRange  = plotHeight - PADDING(graphPtr->padY);
    graphPtr->hOffset = graphPtr->x1 + graphPtr->padLeft;
    graphPtr->hRange  = plotWidth  - PADDING(graphPtr->padX);

    if (graphPtr->vRange < 1) {
        graphPtr->vRange = 1;
    }
    if (graphPtr->hRange < 1) {
        graphPtr->hRange = 1;
    }
    graphPtr->hScale = 1.0f / (float)graphPtr->hRange;
    graphPtr->vScale = 1.0f / (float)graphPtr->vRange;

    /*
     * Calculate the placement of the graph title so it is centered within the
     * space provided for it in the top margin
     */
    graphPtr->titleY = 3 + graphPtr->inset;
    graphPtr->titleX = (graphPtr->x2 + graphPtr->x1) / 2;
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureAxis --
 *
 *      Configures axis attributes (font, line width, label, etc).
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 * Side Effects:
 *      Axis layout is deferred until the height and width of the window are
 *      known.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureAxis(Axis *axisPtr)
{
    Graph *graphPtr = axisPtr->obj.graphPtr;
    float angle;

    /* Check the requested axis limits. Can't allow -min to be greater than
     * -max.  Do this regardless of -checklimits option. We want to always 
     * detect when the user has zoomed in beyond the precision of the data.*/
    if (((DEFINED(axisPtr->reqMin)) && (DEFINED(axisPtr->reqMax))) &&
        (axisPtr->reqMin >= axisPtr->reqMax)) {
        char msg[200];
        Blt_FormatString(msg, 200, 
                  "impossible axis limits (-min %g >= -max %g) for \"%s\"",
                  axisPtr->reqMin, axisPtr->reqMax, axisPtr->obj.name);
        Tcl_AppendResult(graphPtr->interp, msg, (char *)NULL);
        return TCL_ERROR;
    }
    axisPtr->scrollMin = axisPtr->reqScrollMin;
    axisPtr->scrollMax = axisPtr->reqScrollMax;
    if (IsLogScale(axisPtr)) {
        if (axisPtr->flags & CHECK_LIMITS) {
            /* Check that the logscale limits are positive.  */
            if ((DEFINED(axisPtr->reqMin)) && (axisPtr->reqMin <= 0.0)) {
                Tcl_AppendResult(graphPtr->interp,"bad logscale -min limit \"", 
                        Blt_Dtoa(graphPtr->interp, axisPtr->reqMin), 
                        "\" for axis \"", axisPtr->obj.name, "\"", 
                        (char *)NULL);
                return TCL_ERROR;
            }
        }
        if ((DEFINED(axisPtr->scrollMin)) && (axisPtr->scrollMin <= 0.0)) {
            axisPtr->scrollMin = Blt_NaN();
        }
        if ((DEFINED(axisPtr->scrollMax)) && (axisPtr->scrollMax <= 0.0)) {
            axisPtr->scrollMax = Blt_NaN();
        }
    }
    angle = FMOD(axisPtr->tickAngle, 360.0f);
    if (angle < 0.0f) {
        angle += 360.0f;
    }
    if (axisPtr->normalBg != NULL) {
        Blt_Bg_SetChangedProc(axisPtr->normalBg, Blt_UpdateGraph, 
                graphPtr);
    }
    if (axisPtr->activeBg != NULL) {
        Blt_Bg_SetChangedProc(axisPtr->activeBg, Blt_UpdateGraph, 
                graphPtr);
    }
    axisPtr->tickAngle = angle;
    ResetTextStyles(axisPtr);

    axisPtr->titleWidth = axisPtr->titleHeight = 0;
    if (axisPtr->title != NULL) {
        unsigned int w, h;

        Blt_GetTextExtents(axisPtr->titleFont, 0, axisPtr->title, -1, &w, &h);
        axisPtr->titleWidth = (unsigned short int)w;
        axisPtr->titleHeight = (unsigned short int)h;
    }

    /* 
     * Don't bother to check what configuration options have changed.  Almost
     * every option changes the size of the plotting area (except for -color
     * and -titlecolor), requiring the graph and its contents to be completely
     * redrawn.
     *
     * Recompute the scale and offset of the axis in case -min, -max options
     * have changed.
     */
    graphPtr->flags |= REDRAW_WORLD;
    graphPtr->flags |= MAP_WORLD | RESET_AXES | CACHE_DIRTY;
    axisPtr->flags |= DIRTY;
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewAxis --
 *
 *      Create and initialize a structure containing information to display
 *      a graph axis.
 *
 * Results:
 *      The return value is a pointer to an Axis structure.
 *
 *---------------------------------------------------------------------------
 */
static Axis *
NewAxis(Graph *graphPtr, const char *name, int margin)
{
    Axis *axisPtr;
    Blt_HashEntry *hPtr;
    int isNew;

    if (name[0] == '-') {
        Tcl_AppendResult(graphPtr->interp, "name of axis \"", name, 
                "\" can't start with a '-'", (char *)NULL);
        return NULL;
    }
    hPtr = Blt_CreateHashEntry(&graphPtr->axes.nameTable, name, &isNew);
    if (!isNew) {
        axisPtr = Blt_GetHashValue(hPtr);
        if ((axisPtr->flags & DELETED) == 0) {
            Tcl_AppendResult(graphPtr->interp, "axis \"", name,
                "\" already exists in \"", Tk_PathName(graphPtr->tkwin), "\"",
                (char *)NULL);
            return NULL;
        }
        axisPtr->flags &= ~DELETED;
    } else {
        axisPtr = Blt_Calloc(1, sizeof(Axis));
        if (axisPtr == NULL) {
            Tcl_AppendResult(graphPtr->interp, 
                "can't allocate memory for axis \"", name, "\"", (char *)NULL);
            return NULL;
        }
        axisPtr->obj.name = Blt_AssertStrdup(name);
        axisPtr->hashPtr = hPtr;
        Blt_GraphSetObjectClass(&axisPtr->obj, CID_NONE);
        axisPtr->obj.graphPtr = graphPtr;
        axisPtr->looseMin = axisPtr->looseMax = TIGHT;
        axisPtr->reqNumMinorTicks = 2;
        axisPtr->reqNumMajorTicks = 10;
        axisPtr->tickLength = 8;
        axisPtr->colorbar.thickness = 0;
        axisPtr->scrollUnits = 10;
        axisPtr->reqMin = axisPtr->reqMax = Blt_NaN();
        axisPtr->reqScrollMin = axisPtr->reqScrollMax = Blt_NaN();
        axisPtr->weight = 1.0;
        axisPtr->flags = (TICKLABELS|GRIDMINOR|AUTO_MAJOR|
                          AUTO_MINOR | EXTERIOR);
        if (graphPtr->classId == CID_ELEM_BAR) {
            axisPtr->flags |= GRID;
        }
        if ((graphPtr->classId == CID_ELEM_BAR) && 
            ((margin == MARGIN_TOP) || (margin == MARGIN_BOTTOM))) {
            axisPtr->reqStep = 1.0;
            axisPtr->reqNumMinorTicks = 0;
        } 
        if ((margin == MARGIN_RIGHT) || (margin == MARGIN_TOP)) {
            axisPtr->flags |= HIDDEN;
        }
        Blt_Ts_InitStyle(axisPtr->limitsTextStyle);
        axisPtr->tickLabels = Blt_Chain_Create();
        axisPtr->lineWidth = 1;
        Blt_SetHashValue(hPtr, axisPtr);
    }
    return axisPtr;
}

static int
GetAxisFromObj(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr, 
               Axis **axisPtrPtr)
{
    Blt_HashEntry *hPtr;
    const char *name;

    *axisPtrPtr = NULL;
    name = Tcl_GetString(objPtr);
    hPtr = Blt_FindHashEntry(&graphPtr->axes.nameTable, name);
    if (hPtr != NULL) {
        Axis *axisPtr;

        axisPtr = Blt_GetHashValue(hPtr);
        if ((axisPtr->flags & DELETED) == 0) {
            *axisPtrPtr = axisPtr;
            return TCL_OK;
        }
    }
    if (interp != NULL) {
        Tcl_AppendResult(interp, "can't find axis \"", name, "\" in \"", 
                Tk_PathName(graphPtr->tkwin), "\"", (char *)NULL);
    }
    return TCL_ERROR;
}

static int
GetAxisByClass(Tcl_Interp *interp, Graph *graphPtr, Tcl_Obj *objPtr,
               ClassId cid, Axis **axisPtrPtr)
{
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objPtr, &axisPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (cid != CID_NONE) {
        if ((axisPtr->refCount == 0) || (axisPtr->obj.classId == CID_NONE)) {
            /* Set the axis type on the first use of it. */
            Blt_GraphSetObjectClass(&axisPtr->obj, cid);
        } else if (axisPtr->obj.classId != cid) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "axis \"", Tcl_GetString(objPtr),
                    "\" is already in use on an opposite ", 
                        axisPtr->obj.className, "-axis", 
                        (char *)NULL);
            }
            return TCL_ERROR;
        }
    }
    axisPtr->refCount++;
    *axisPtrPtr = axisPtr;
    return TCL_OK;
}

void
Blt_DestroyAxes(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        Axis *axisPtr;
        
        axisPtr = Blt_GetHashValue(hPtr);
        axisPtr->hashPtr = NULL;
        DestroyAxis(axisPtr);
    }
    Blt_DeleteHashTable(&graphPtr->axes.nameTable);
    Blt_DeleteHashTable(&graphPtr->axes.bindTagTable);
    Blt_Chain_Destroy(graphPtr->axes.displayList);
}

void
Blt_ConfigureAxes(Graph *graphPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        Axis *axisPtr;
        
        axisPtr = Blt_GetHashValue(hPtr);
        ConfigureAxis(axisPtr);
    }
}

int
Blt_DefaultAxes(Graph *graphPtr)
{
    int i;
    unsigned int flags;

    for (i = 0; i < 4; i++) {
        Margin *marginPtr;

        marginPtr = graphPtr->margins + i;
        marginPtr->axes = Blt_Chain_Create();
        marginPtr->name = axisNames[i].name;
        marginPtr->side = 3;
    }
    flags = Blt_GraphType(graphPtr);
    for (i = 0; i < 4; i++) {
        Axis *axisPtr;
        Margin *marginPtr;

        marginPtr = graphPtr->margins + i;
        /* Create a default axis for each chain. */
        axisPtr = NewAxis(graphPtr, marginPtr->name, i);
        if (axisPtr == NULL) {
            return TCL_ERROR;
        }
        axisPtr->refCount = 1;  /* Default axes are assumed in use. */
        axisPtr->marginPtr = marginPtr;
        Blt_GraphSetObjectClass(&axisPtr->obj, axisNames[i].classId);
        if (Blt_ConfigureComponentFromObj(graphPtr->interp, graphPtr->tkwin,
                axisPtr->obj.name, "Axis", configSpecs, 0, (Tcl_Obj **)NULL,
                (char *)axisPtr, flags) != TCL_OK) {
            return TCL_ERROR;
        }
        if (ConfigureAxis(axisPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        axisPtr->link = Blt_Chain_Append(marginPtr->axes, axisPtr);
    }
    /* The extra axes are not attached to a specific margin. */
    for (i = 4; i < numAxisNames; i++) {
        Axis *axisPtr;

        axisPtr = NewAxis(graphPtr, axisNames[i].name, MARGIN_NONE);
        if (axisPtr == NULL) {
            return TCL_ERROR;
        }
        axisPtr->refCount = 1;          
        axisPtr->marginPtr = NULL;
        Blt_GraphSetObjectClass(&axisPtr->obj, axisNames[i].classId);
        if (Blt_ConfigureComponentFromObj(graphPtr->interp, graphPtr->tkwin,
                axisPtr->obj.name, "Axis", configSpecs, 0, (Tcl_Obj **)NULL,
                (char *)axisPtr, flags) != TCL_OK) {
            return TCL_ERROR;
        }
        if (ConfigureAxis(axisPtr) != TCL_OK) {
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
 *      Activates the axis, drawing the axis with its -activeforeground,
 *      -activebackgound, -activerelief attributes.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Graph will be redrawn to reflect the new axis attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
ActivateOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Axis *axisPtr = clientData;
    Graph *graphPtr = axisPtr->obj.graphPtr;
    const char *string;

    string = Tcl_GetString(objv[2]);
    if (string[0] == 'a') {
        axisPtr->flags |= ACTIVE;
    } else {
        axisPtr->flags &= ~ACTIVE;
    }
    if ((axisPtr->marginPtr != NULL) && ((axisPtr->flags & HIDDEN) == 0)) {
        graphPtr->flags |= DRAW_MARGINS | CACHE_DIRTY;
        Blt_EventuallyRedrawGraph(graphPtr);
    }
    return TCL_OK;
}

/*-------------------------------------------------------------------------------
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
    Axis *axisPtr = clientData;
    Graph *graphPtr = axisPtr->obj.graphPtr;

    return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable,
          Blt_MakeAxisTag(graphPtr, axisPtr->obj.name), objc, objv);
}
          
/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *      Queries axis attributes (font, line width, label, etc).
 *
 * Results:
 *      Return value is a standard TCL result.  If querying configuration
 *      values, interp->result will contain the results.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Axis *axisPtr = clientData;
    Graph *graphPtr = axisPtr->obj.graphPtr;

    return Blt_ConfigureValueFromObj(interp, graphPtr->tkwin, configSpecs,
        (char *)axisPtr, objv[0], Blt_GraphType(graphPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 *      Queries or resets axis attributes (font, line width, label, etc).
 *
 * Results:
 *      Return value is a standard TCL result.  If querying configuration
 *      values, interp->result will contain the results.
 *
 * Side Effects:
 *      Axis resources are possibly allocated (GC, font). Axis layout is
 *      deferred until the height and width of the window are known.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Axis *axisPtr = clientData;
    Graph *graphPtr = axisPtr->obj.graphPtr;
    int flags;

    flags = BLT_CONFIG_OBJV_ONLY | Blt_GraphType(graphPtr);
    if (objc == 0) {
        return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
            (char *)axisPtr, (Tcl_Obj *)NULL, flags);
    } else if (objc == 1) {
        return Blt_ConfigureInfoFromObj(interp, graphPtr->tkwin, configSpecs,
            (char *)axisPtr, objv[0], flags);
    }
    if (Blt_ConfigureWidgetFromObj(interp, graphPtr->tkwin, configSpecs, 
        objc, objv, (char *)axisPtr, flags) != TCL_OK) {
        return TCL_ERROR;
    }
    if (ConfigureAxis(axisPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (axisPtr->marginPtr != NULL) {
        if (Blt_ConfigModified(configSpecs, "-autorange", "-bd", "-borderwidth",
                "-command", "-decreasing", "-descending", "-hide", "-justify", 
                "-labeloffset", 
                "-limitsfont", "-limitsformat", "-linewidth", "-logscale", 
                "-loose", "-majorticks", "-max", "-min", "-minorticks", 
                "-relief", "-rotate", "-scrollmax", "-scrollmin", "-shiftby", 
                "-showticks", "-stepsize", "-tickdivider", "-subdivisions", 
                "-tickfont", "-ticklength", "-title", "-titlealternate", 
                "-titlefont", "titleFont", (char *)NULL)) {
            graphPtr->flags |= CACHE_DIRTY;
        }
        if (Blt_ConfigModified(configSpecs, "-logscale", (char *)NULL)) {
            graphPtr->flags |= MAP_WORLD;
        }
        Blt_EventuallyRedrawGraph(graphPtr);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * LimitsOp --
 *
 *      This procedure returns a string representing the axis limits
 *      of the graph.  The format of the string is { left top right bottom}.
 *
 * Results:
 *      Always returns TCL_OK.  The interp->result field is
 *      a list of the graph axis limits.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
LimitsOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Axis *axisPtr = clientData;
    Graph *graphPtr = axisPtr->obj.graphPtr;
    Tcl_Obj *listObjPtr;
    double min, max;

    if (graphPtr->flags & RESET_AXES) {
        Blt_ResetAxes(graphPtr);
    }
    if (IsLogScale(axisPtr)) {
        min = EXP10(axisPtr->axisRange.min);
        max = EXP10(axisPtr->axisRange.max);
    } else {
        min = axisPtr->axisRange.min;
        max = axisPtr->axisRange.max;
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
 *      Maps the given window coordinate into an axis-value.
 *
 * Results:
 *      Returns a standard TCL result.  interp->result contains
 *      the axis value. If an error occurred, TCL_ERROR is returned
 *      and interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
InvTransformOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Axis *axisPtr = clientData;
    Graph *graphPtr = axisPtr->obj.graphPtr;
    double y;                           /* Real graph coordinate */
    int sy;                             /* Integer window coordinate*/

    if (graphPtr->flags & RESET_AXES) {
        Blt_ResetAxes(graphPtr);
    }
    if (Tcl_GetIntFromObj(interp, objv[0], &sy) != TCL_OK) {
        return TCL_ERROR;
    }
    /*
     * Is the axis vertical or horizontal?
     *
     * Check the side where the axis was positioned.  If the axis is
     * virtual, all we have to go on is how it was mapped to an
     * element (using either -mapx or -mapy options).  
     */
    if (HORIZONTAL(axisPtr->marginPtr)) {
        y = Blt_InvHMap(axisPtr, (double)sy);
    } else {
        y = Blt_InvVMap(axisPtr, (double)sy);
    }
    Tcl_SetDoubleObj(Tcl_GetObjResult(interp), y);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MarginOp --
 *
 *      This procedure returns a string representing the margin the axis
 *      resides.  The margin string is "left", "right", "top", or "bottom".
 *
 * Results:
 *      Always returns TCL_OK.  interp->result contains the name of the
 *      margin.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MarginOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Axis *axisPtr = clientData;
    const char *name;

    name = "";
    if (axisPtr->marginPtr != NULL) {
        name = axisPtr->marginPtr->name;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), name, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TransformOp --
 *
 *      Maps the given x or y graph coordinate into a window/screen
 *      coordinate.
 *
 * Results:
 *      Returns a standard TCL result.  interp->result contains the window
 *      coordinate. If an error occurred, TCL_ERROR is returned and
 *      interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TransformOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Axis *axisPtr = clientData;
    Graph *graphPtr = axisPtr->obj.graphPtr;
    double x;

    if (graphPtr->flags & RESET_AXES) {
        Blt_ResetAxes(graphPtr);
    }
    if (Blt_ExprDoubleFromObj(interp, objv[0], &x) != TCL_OK) {
        return TCL_ERROR;
    }
    if (HORIZONTAL(axisPtr->marginPtr)) {
        x = Blt_HMap(axisPtr, x);
    } else {
        x = Blt_VMap(axisPtr, x);
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), (int)x);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 *      This procedure returns a string representing the margin the axis
 *      resides.  The format of the string is "x", "y", or "".
 *
 * Results:
 *      Always returns TCL_OK.  The interp->result field is the type of 
 *      axis.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Axis *axisPtr = clientData;
    const char *name;

    switch (axisPtr->obj.classId) {
    case CID_AXIS_X:
        name = "x";         break;
    case CID_AXIS_Y:
        name = "y";         break;
    case CID_AXIS_Z:
        name = "z";         break;
    default:
        name = "unknown";   break;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), name, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * UseOp --
 *
 *      Sets the default axis for a margin.  Both sets the margin of the
 *      axis and indicates the axis is in use.
 *
 * Results:
 *      A standard TCL result.  If the named axis doesn't exist
 *      an error message is put in interp->result.
 *
 * .g xaxis use "abc def gah"
 * .g xaxis use [lappend abc [.g axis use]]
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
UseOp(ClientData clientData, Tcl_Interp *interp, int objc,
      Tcl_Obj *const *objv)
{
    Axis *axisPtr = clientData;
    Axis *nextPtr;
    ClassId cid;
    Graph *graphPtr = (Graph *)axisPtr;
    Margin *marginPtr;
    Tcl_Obj **axisObjv;
    int axisObjc;
    int i;
    
    marginPtr = graphPtr->margins + lastMargin;
    if (objc == 0) {
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        for (axisPtr = FirstAxis(marginPtr); axisPtr != NULL;
             axisPtr = NextAxis(axisPtr)) {
            Tcl_ListObjAppendElement(interp, listObjPtr,
                Tcl_NewStringObj(axisPtr->obj.name, -1));
        }
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    if ((lastMargin == MARGIN_BOTTOM) || (lastMargin == MARGIN_TOP)) {
        cid = (graphPtr->flags & INVERTED) ? CID_AXIS_Y : CID_AXIS_X;
    } else {
        cid = (graphPtr->flags & INVERTED) ? CID_AXIS_X : CID_AXIS_Y;
    }
    if (Tcl_ListObjGetElements(interp, objv[0], &axisObjc, &axisObjv) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    /* Step 1: Clear the list of axes for this margin. */
    for (axisPtr = FirstAxis(marginPtr); axisPtr != NULL; axisPtr = nextPtr) {
        nextPtr = NextAxis(axisPtr);
        Blt_Chain_UnlinkLink(axisPtr->marginPtr->axes, axisPtr->link);
        axisPtr->link = NULL;
        axisPtr->marginPtr = NULL;
        /* Clear the axis type if it's not currently used.*/
        if (axisPtr->refCount == 0) {
            Blt_GraphSetObjectClass(&axisPtr->obj, CID_NONE);
        }
    }
    Blt_Chain_Reset(marginPtr->axes);
    /* Step 2: Add the named axes to this margin. */
    for (i = 0; i < axisObjc; i++) {
        Axis *axisPtr;

        if (GetAxisFromObj(interp, graphPtr, axisObjv[i], &axisPtr) != TCL_OK){
            return TCL_ERROR;
        }
        axisPtr->link = Blt_Chain_Append(marginPtr->axes, axisPtr);
        axisPtr->marginPtr = marginPtr;
        Blt_GraphSetObjectClass(&axisPtr->obj, cid);
    }
    graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | RESET_AXES);
    /* When any axis changes, we need to layout the entire graph.  */
    graphPtr->flags |= (MAP_WORLD | REDRAW_WORLD);
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

static int
ViewOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Axis *axisPtr = clientData;
    Graph *graphPtr;
    double axisOffset, axisScale;
    double fract;
    double viewMin, viewMax, worldMin, worldMax;
    double viewWidth, worldWidth;

    graphPtr = axisPtr->obj.graphPtr;
    worldMin = axisPtr->valueRange.min;
    worldMax = axisPtr->valueRange.max;
    /* Override data dimensions with user-selected limits. */
    if (DEFINED(axisPtr->scrollMin)) {
        worldMin = axisPtr->scrollMin;
    }
    if (DEFINED(axisPtr->scrollMax)) {
        worldMax = axisPtr->scrollMax;
    }
    viewMin = axisPtr->min;
    viewMax = axisPtr->max;
    /* Bound the view within scroll region. */ 
    if (viewMin < worldMin) {
        viewMin = worldMin;
    } 
    if (viewMax > worldMax) {
        viewMax = worldMax;
    }
    if (IsLogScale(axisPtr)) {
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
    if (AxisIsHorizontal(axisPtr) != axisPtr->decreasing) {
        axisOffset  = viewMin - worldMin;
        axisScale = graphPtr->hScale;
    } else {
        axisOffset  = worldMax - viewMax;
        axisScale = graphPtr->vScale;
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
    if (GetAxisScrollInfo(interp, objc, objv, &fract, 
        viewWidth / worldWidth, axisPtr->scrollUnits, axisScale) != TCL_OK) {
        return TCL_ERROR;
    }
    if (AxisIsHorizontal(axisPtr) != axisPtr->decreasing) {
        axisPtr->reqMin = (fract * worldWidth) + worldMin;
        axisPtr->reqMax = axisPtr->reqMin + viewWidth;
    } else {
        axisPtr->reqMax = worldMax - (fract * worldWidth);
        axisPtr->reqMin = axisPtr->reqMax - viewWidth;
    }
    if (IsLogScale(axisPtr)) {
        axisPtr->reqMin = EXP10(axisPtr->reqMin);
        axisPtr->reqMax = EXP10(axisPtr->reqMax);
    }
    graphPtr->flags |= (GET_AXIS_GEOMETRY | LAYOUT_NEEDED | RESET_AXES);
    Blt_EventuallyRedrawGraph(graphPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisCreateOp --
 *
 *      Creates a new axis.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AxisCreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Axis *axisPtr;
    int flags;

    axisPtr = NewAxis(graphPtr, Tcl_GetString(objv[3]), MARGIN_NONE);
    if (axisPtr == NULL) {
        return TCL_ERROR;
    }
    flags = Blt_GraphType(graphPtr);
    if ((Blt_ConfigureComponentFromObj(interp, graphPtr->tkwin, 
        axisPtr->obj.name, "Axis", configSpecs, objc - 4, objv + 4, 
        (char *)axisPtr, flags) != TCL_OK) || 
        (ConfigureAxis(axisPtr) != TCL_OK)) {
        DestroyAxis(axisPtr);
        return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), axisPtr->obj.name, -1);
    return TCL_OK;
}
/*
 *---------------------------------------------------------------------------
 *
 * AxisActivateOp --
 *
 *      Activates the axis, drawing the axis with its -activeforeground,
 *      -activebackgound, -activerelief attributes.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Graph will be redrawn to reflect the new axis attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisActivateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return ActivateOp(axisPtr, interp, objc, objv);
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
AxisBindOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    if (objc == 3) {
        Blt_HashEntry *hPtr;
        Blt_HashSearch cursor;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.bindTagTable, &cursor);
             hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
            const char *tagName;
            Tcl_Obj *objPtr;

            tagName = Blt_GetHashKey(&graphPtr->axes.bindTagTable, hPtr);
            objPtr = Tcl_NewStringObj(tagName, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    }
    return Blt_ConfigureBindingsFromObj(interp, graphPtr->bindTable, 
        Blt_MakeAxisTag(graphPtr, Tcl_GetString(objv[3])), objc - 4, objv + 4);
}


/*
 *---------------------------------------------------------------------------
 *
 * AxisCgetOp --
 *
 *      Queries axis attributes (font, line width, label, etc).
 *
 * Results:
 *      Return value is a standard TCL result.  If querying configuration
 *      values, interp->result will contain the results.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
AxisCgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return CgetOp(axisPtr, interp, objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisConfigureOp --
 *
 *      Queries or resets axis attributes (font, line width, label, etc).
 *
 * Results:
 *      Return value is a standard TCL result.  If querying configuration
 *      values, interp->result will contain the results.
 *
 * Side Effects:
 *      Axis resources are possibly allocated (GC, font). Axis layout is
 *      deferred until the height and width of the window are known.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Tcl_Obj *const *options;
    int i;
    int numNames, numOpts;

    /* Figure out where the option value pairs begin */
    objc -= 3;
    objv += 3;
    for (i = 0; i < objc; i++) {
        Axis *axisPtr;
        const char *string;

        string = Tcl_GetString(objv[i]);
        if (string[0] == '-') {
            break;
        }
        if (GetAxisFromObj(interp, graphPtr, objv[i], &axisPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    numNames = i;                               /* Number of pen names specified */
    numOpts = objc - i;                 /* Number of options specified */
    options = objv + i;                 /* Start of options in objv  */

    for (i = 0; i < numNames; i++) {
        Axis *axisPtr;

        if (GetAxisFromObj(interp, graphPtr, objv[i], &axisPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (ConfigureOp(axisPtr, interp, numOpts, options) != TCL_OK) {
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
 *      Deletes one or more axes.  The actual removal may be deferred until the
 *      axis is no longer used by any element. The axis can't be referenced by
 *      its name any longer and it may be recreated.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AxisDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    int i;

    for (i = 3; i < objc; i++) {
        Axis *axisPtr;

        if (GetAxisFromObj(interp, graphPtr, objv[i], &axisPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        axisPtr->flags |= DELETED;
        if (axisPtr->refCount == 0) {
            DestroyAxis(axisPtr);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisFocusOp --
 *
 *      Activates the axis, drawing the axis with its -activeforeground,
 *      -activebackgound, -activerelief attributes.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side Effects:
 *      Graph will be redrawn to reflect the new axis attributes.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisFocusOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;

    if (objc > 3) {
        Axis *axisPtr;
        const char *string;

        axisPtr = NULL;
        string = Tcl_GetString(objv[3]);
        if ((string[0] != '\0') && 
            (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK)) {
            return TCL_ERROR;
        }
        graphPtr->focusPtr = axisPtr;
        Blt_SetFocusItem(graphPtr->bindTable, graphPtr->focusPtr, NULL);
    }
    /* Return the name of the axis that has focus. */
    if (graphPtr->focusPtr != NULL) {
        Tcl_SetStringObj(Tcl_GetObjResult(interp), 
                graphPtr->focusPtr->obj.name, -1);
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
AxisGetOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    GraphObj *objPtr;

    objPtr = Blt_GetCurrentItem(graphPtr->bindTable);
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
            Axis *axisPtr;
            
            axisPtr = (Axis *)objPtr;
            Tcl_SetStringObj(Tcl_GetObjResult(interp), axisPtr->detail, -1);
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisInvTransformOp --
 *
 *      Maps the given window coordinate into an axis-value.
 *
 * Results:
 *      Returns a standard TCL result.  interp->result contains the axis
 *      value. If an error occurred, TCL_ERROR is returned and interp->result
 *      will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisInvTransformOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                   Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return InvTransformOp(axisPtr, interp, objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisLimitsOp --
 *
 *      This procedure returns a string representing the axis limits of the
 *      graph.  The format of the string is { left top right bottom}.
 *
 * Results:
 *      Always returns TCL_OK.  The interp->result field is
 *      a list of the graph axis limits.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisLimitsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return LimitsOp(axisPtr, interp, objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisMarginOp --
 *
 *      This procedure returns a string representing the axis limits of the
 *      graph.  The format of the string is "left top right bottom".
 *
 * Results:
 *      Always returns TCL_OK.  The interp->result field is a list of the
 *      graph axis limits.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisMarginOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return MarginOp(axisPtr, interp, objc - 4, objv + 4);
}

typedef struct {
    int flags;
} NamesArgs;

#define ZOOM    (1<<0)
#define VISIBLE (1<<1)

static Blt_SwitchSpec namesSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-zoom", "", (char *)NULL,
        Blt_Offset(NamesArgs, flags), 0, ZOOM},
    {BLT_SWITCH_BITMASK, "-visible", "", (char *)NULL,
        Blt_Offset(NamesArgs, flags), 0, VISIBLE},
    {BLT_SWITCH_END}
};

/*
 *---------------------------------------------------------------------------
 *
 * AxisNamesOp --
 *
 *      Return a list of the names of all the axes.
 *
 * Results:
 *      Returns a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
AxisNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Tcl_Obj *listObjPtr;
    NamesArgs args;
    int i, count;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    
    count = 0;
    for (i = 3; i < objc; i++) {
        const char *string;

        string = Tcl_GetString(objv[i]);
        if (string[0] != '-') {
            break;
        }
        count++;
    }
    args.flags = 0;
    if (Blt_ParseSwitches(interp, namesSwitches, count, objv + 3, &args,
                          BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    objc -= count;
    objv += count;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        Axis *axisPtr;
        int match;
        
        axisPtr = Blt_GetHashValue(hPtr);
        if (axisPtr->flags & DELETED) {
            continue;
        }
        if (((axisPtr->flags & HIDDEN) || (axisPtr->marginPtr == NULL)) &&
            (args.flags & (VISIBLE|ZOOM))) {
            continue;               /* Don't zoom hidden axes. */
        }
        if ((axisPtr->obj.classId != CID_AXIS_X) &&
            (axisPtr->obj.classId != CID_AXIS_Y) &&
            (args.flags & ZOOM)) {
            continue;               /* Zoom only X or Y axes. */
        }
        if ((axisPtr->marginPtr != NULL) &&
            (axisPtr->marginPtr->numVisibleAxes > 1)) {
            continue;               /* Don't zoom stacked axes. */
        }
        match = FALSE;
        for (i = 3; i < objc; i++) {
            const char *pattern;

            pattern = Tcl_GetString(objv[i]);
            if (Tcl_StringMatch(axisPtr->obj.name, pattern)) {
                match = TRUE;
                break;
            }
        }
        if ((objc == 3) || (match)) {
            Tcl_ListObjAppendElement(interp, listObjPtr, 
                Tcl_NewStringObj(axisPtr->obj.name, -1));
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
 *      Maps the given axis-value to a window coordinate.
 *
 * Results:
 *      Returns the window coordinate via interp->result.  If an error occurred,
 *      TCL_ERROR is returned and interp->result will contain an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisTransformOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TransformOp(axisPtr, interp, objc - 4, objv + 4);
}

/*
 *---------------------------------------------------------------------------
 *
 * AxisMarginOp --
 *
 *      This procedure returns a string representing the axis limits of the
 *      graph.  The format of the string is { left top right bottom}.
 *
 * Results:
 *      Always returns TCL_OK.  The interp->result field is
 *      a list of the graph axis limits.
 *
 *---------------------------------------------------------------------------
 */
static int
AxisTypeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
           Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TypeOp(axisPtr, interp, objc - 4, objv + 4);
}


static int
AxisViewOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    Axis *axisPtr;

    if (GetAxisFromObj(interp, graphPtr, objv[3], &axisPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return ViewOp(axisPtr, interp, objc - 4, objv + 4);
}

static Blt_OpSpec virtAxisOps[] = {
    {"activate",     1, AxisActivateOp,     4, 4, "axisName"},
    {"bind",         1, AxisBindOp,         3, 6, "bindTag sequence command"},
    {"cget",         2, AxisCgetOp,         5, 5, "axisName option"},
    {"configure",    2, AxisConfigureOp,    4, 0, "axisName ?axisName?... "
        "?option value?..."},
    {"create",       2, AxisCreateOp,       4, 0, "axisName ?option value?..."},
    {"deactivate",   3, AxisActivateOp,     4, 4, "axisName"},
    {"delete",       3, AxisDeleteOp,       3, 0, "?axisName?..."},
    {"focus",        1, AxisFocusOp,        3, 4, "?axisName?"},
    {"get",          1, AxisGetOp,          4, 4, "name"},
    {"invtransform", 1, AxisInvTransformOp, 5, 5, "axisName value"},
    {"limits",       1, AxisLimitsOp,       4, 4, "axisName"},
    {"margin",       1, AxisMarginOp,       4, 4, "axisName"},
    {"names",        1, AxisNamesOp,        3, 0, "?pattern?..."},
    {"transform",    2, AxisTransformOp,    5, 5, "axisName value"},
    {"type",         2, AxisTypeOp,       4, 4, "axisName"},
    {"view",         1, AxisViewOp,         4, 7, "axisName ?moveto fract? "
        "?scroll number what?"},
};
static int numVirtAxisOps = sizeof(virtAxisOps) / sizeof(Blt_OpSpec);

int
Blt_VirtualAxisOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numVirtAxisOps, virtAxisOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

static Blt_OpSpec axisOps[] = {
    {"activate",     1, ActivateOp,     3, 3, ""},
    {"bind",         1, BindOp,         2, 5, "sequence command"},
    {"cget",         2, CgetOp,         4, 4, "option"},
    {"configure",    2, ConfigureOp,    3, 0, "?option value?..."},
    {"deactivate",   1, ActivateOp,     3, 3, ""},
    {"invtransform", 1, InvTransformOp, 4, 4, "value"},
    {"limits",       1, LimitsOp,       3, 3, ""},
    {"transform",    1, TransformOp,    4, 4, "value"},
    {"use",          1, UseOp,          3, 4, "?axisName?"},
    {"view",         1, ViewOp,         3, 6, "?moveto fract? "},
};

static int numAxisOps = sizeof(axisOps) / sizeof(Blt_OpSpec);

int
Blt_AxisOp(ClientData clientData, Tcl_Interp *interp, int margin, int objc,
           Tcl_Obj *const *objv)
{
    Graph *graphPtr = clientData;
    int result;
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numAxisOps, axisOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    if (proc == UseOp) {
        lastMargin = margin;            /* Set global variable to the
                                         * margin in the argument
                                         * list. Needed only for UseOp. */
        result = (*proc)(clientData, interp, objc - 3, objv + 3);
    } else {
        Axis *axisPtr;

        axisPtr = FirstAxis(graphPtr->margins + margin);
        if (axisPtr == NULL) {
            return TCL_OK;
        }
        result = (*proc)(axisPtr, interp, objc - 3, objv + 3);
    }
    return result;
}

void
Blt_MapAxes(Graph *graphPtr)
{
    int i;
    
    for (i = 0; i < 4; i++) {
        Axis *axisPtr;
        Margin *marginPtr;
        float sum;
        
        marginPtr = graphPtr->margins + i;
        /* Reset the margin offsets (stacked and layered). */
        marginPtr->nextStackOffset = marginPtr->nextLayerOffset = 0;
        sum = 0.0;
        if (graphPtr->flags & STACK_AXES) {
            /* For stacked axes figure out the sum of the weights.*/
            for (axisPtr = FirstAxis(marginPtr); axisPtr != NULL; 
                 axisPtr = NextAxis(axisPtr)) {
                if (axisPtr->flags & (DELETED|HIDDEN)) {
                    continue;           /* Ignore axes that aren't in use
                                         * or have been deleted.  */
                }
                sum += axisPtr->weight;
            }
        }
        for (axisPtr = FirstAxis(marginPtr); axisPtr != NULL; 
             axisPtr = NextAxis(axisPtr)) {
            if (axisPtr->flags & DELETED) {
                continue;               /* Don't map axes that aren't being
                                         * used or have been deleted. */
            }
            if (HORIZONTAL(marginPtr)) {
                axisPtr->screenMin = graphPtr->hOffset;
                axisPtr->width = graphPtr->x2 - graphPtr->x1;
                axisPtr->screenRange = graphPtr->hRange;
            } else {
                axisPtr->screenMin = graphPtr->vOffset;
                axisPtr->height = graphPtr->y2 - graphPtr->y1;
                axisPtr->screenRange = graphPtr->vRange;
            }
            axisPtr->screenScale = 1.0 / axisPtr->screenRange;
            if (axisPtr->flags & HIDDEN) {
                continue;
            }
            if (graphPtr->flags & STACK_AXES) {
                if (axisPtr->reqNumMajorTicks <= 0) {
                    axisPtr->reqNumMajorTicks = 4;
                }
                MapStackedAxis(axisPtr, sum);
            } else {
                if (axisPtr->reqNumMajorTicks <= 0) {
                    axisPtr->reqNumMajorTicks = 4;
                }
                MapAxis(axisPtr);
                /* The next axis will start after the current axis. */
                marginPtr->nextLayerOffset += HORIZONTAL(axisPtr->marginPtr) ?
                    axisPtr->height : axisPtr->width;
            }
            if (axisPtr->flags & GRID) {
                MapGridlines(axisPtr);
            }
        }
    }
}


void
Blt_DrawAxes(Graph *graphPtr, Drawable drawable)
{
    int i;

    for (i = 0; i < 4; i++) {
        Axis *axisPtr;
        Margin *marginPtr;

        marginPtr = graphPtr->margins + i;
        for (axisPtr = FirstAxis(marginPtr); axisPtr != NULL; 
             axisPtr = NextAxis(axisPtr)) {
            if (axisPtr->flags & (DELETED|HIDDEN)) {
                continue;
            }
            DrawAxis(axisPtr, drawable);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawGrids --
 *
 *      Draws the grid lines associated with each axis.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DrawGrids(Graph *graphPtr, Drawable drawable) 
{
    int i;

    for (i = 0; i < 4; i++) {
        Axis *axisPtr;
        Margin *marginPtr;

        marginPtr = graphPtr->margins + i;
        for (axisPtr = FirstAxis(marginPtr); axisPtr != NULL;
             axisPtr = NextAxis(axisPtr)) {
            if (axisPtr->flags & (DELETED|HIDDEN)) {
                continue;
            }
            if (axisPtr->flags & GRID) {
                Blt_DrawSegments2d(graphPtr->display, drawable, 
                        axisPtr->major.grid.gc, 
                        axisPtr->major.grid.segments, 
                        axisPtr->major.grid.numUsed);
                if (axisPtr->flags & GRIDMINOR) {
                    Blt_DrawSegments2d(graphPtr->display, drawable, 
                        axisPtr->minor.grid.gc, 
                        axisPtr->minor.grid.segments, 
                        axisPtr->minor.grid.numUsed);
                }
            }
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GridsToPostScript --
 *
 *      Draws the grid lines associated with each axis.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_GridsToPostScript(Graph *graphPtr, Blt_Ps ps) 
{
    int i;

    for (i = 0; i < 4; i++) {
        Axis *axisPtr;
        Margin *marginPtr;

        marginPtr = graphPtr->margins + i;
        for (axisPtr = FirstAxis(marginPtr); axisPtr != NULL;
             axisPtr = NextAxis(axisPtr)) {
            if ((axisPtr->flags & (DELETED|HIDDEN|GRID)) != GRID) {
                continue;
            }
            Blt_Ps_Format(ps, "%% Axis %s: grid line attributes\n", 
                axisPtr->obj.name);
            Blt_Ps_XSetLineAttributes(ps, axisPtr->major.grid.color, 
                axisPtr->major.grid.lineWidth, 
                &axisPtr->major.grid.dashes, CapButt, JoinMiter);
            Blt_Ps_Format(ps, "%% Axis %s: major grid line segments\n",
                axisPtr->obj.name);
            Blt_Ps_DrawSegments2d(ps, axisPtr->major.grid.numUsed, 
                axisPtr->major.grid.segments);
            if (axisPtr->flags & GRIDMINOR) {
                Blt_Ps_XSetLineAttributes(ps, axisPtr->minor.grid.color, 
                        axisPtr->minor.grid.lineWidth, 
                        &axisPtr->minor.grid.dashes, CapButt, JoinMiter);
                Blt_Ps_Format(ps, "%% Axis %s: minor grid line segments\n",
                        axisPtr->obj.name);
                Blt_Ps_DrawSegments2d(ps, axisPtr->minor.grid.numUsed, 
                        axisPtr->minor.grid.segments);
            }
        }
    }
}

void
Blt_AxesToPostScript(Graph *graphPtr, Blt_Ps ps) 
{
    int i;

    for (i = 0; i < 4; i++) {
        Axis *axisPtr;
        Margin *marginPtr;
        
        marginPtr = graphPtr->margins + i;
        for (axisPtr = FirstAxis(marginPtr); axisPtr != NULL; 
             axisPtr = NextAxis(axisPtr)) {
            if (axisPtr->flags & (DELETED|HIDDEN)) {
                continue;
            }
            AxisToPostScript(axisPtr, ps);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_DrawAxisLimits --
 *
 *      Draws the min/max values of the axis in the plotting area.  The
 *      text strings are formatted according to the "sprintf" format
 *      descriptors in the limitsFmts array.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      Draws the numeric values of the axis limits into the outer regions
 *      of the plotting area.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_DrawAxisLimits(Graph *graphPtr, Drawable drawable)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    char minString[200], maxString[200];
    int vMin, hMin, vMax, hMax;

#define SPACING 8
    vMin = vMax = graphPtr->x1 + graphPtr->padLeft + 2;
    hMin = hMax = graphPtr->y2 - graphPtr->padBottom - 2;   /* Offsets */

    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
        hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        Axis *axisPtr;
        Dim2d textDim;
        Tcl_Obj **objv;
        char *minPtr, *maxPtr;
        const char *minFmt, *maxFmt;
        int objc;
        
        axisPtr = Blt_GetHashValue(hPtr);
        if (axisPtr->flags & DELETED) {
            continue;                   /* Axis has been deleted. */
        } 
        if (axisPtr->limitsFmtsObjPtr == NULL) {
            continue;                   /* No limits format specified for
                                         * this axis. */
        }
        if (axisPtr->marginPtr == NULL) {
            continue;                   /* Axis is not associated with any
                                         * margin. */
        }
        Tcl_ListObjGetElements(NULL,axisPtr->limitsFmtsObjPtr, &objc, &objv);
        minPtr = maxPtr = NULL;
        minFmt = maxFmt = Tcl_GetString(objv[0]);
        if (objc > 1) {
            maxFmt = Tcl_GetString(objv[1]);
        }
        if (minFmt[0] != '\0') {
            minPtr = minString;
            Blt_FormatString(minString, 200, minFmt, axisPtr->axisRange.min);
        }
        if (maxFmt[0] != '\0') {
            maxPtr = maxString;
            Blt_FormatString(maxString, 200, maxFmt, axisPtr->axisRange.max);
        }
        if (axisPtr->decreasing) {
            char *tmp;

            tmp = minPtr, minPtr = maxPtr, maxPtr = tmp;
        }
        if (maxPtr != NULL) {
            if (HORIZONTAL(axisPtr->marginPtr)) {
                Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
                Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SE);
                Blt_DrawText2(graphPtr->tkwin, drawable, maxPtr,
                    &axisPtr->limitsTextStyle, graphPtr->x2, hMax, &textDim);
                hMax -= (textDim.height + SPACING);
            } else {
                Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
                Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_NW);
                Blt_DrawText2(graphPtr->tkwin, drawable, maxPtr,
                    &axisPtr->limitsTextStyle, vMax, graphPtr->y1, &textDim);
                vMax += (textDim.width + SPACING);
            }
        }
        if (minPtr != NULL) {
            Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SW);
            if (HORIZONTAL(axisPtr->marginPtr)) {
                Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
                Blt_DrawText2(graphPtr->tkwin, drawable, minPtr,
                    &axisPtr->limitsTextStyle, graphPtr->x1, hMin, &textDim);
                hMin -= (textDim.height + SPACING);
            } else {
                Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
                Blt_DrawText2(graphPtr->tkwin, drawable, minPtr,
                    &axisPtr->limitsTextStyle, vMin, graphPtr->y2, &textDim);
                vMin += (textDim.width + SPACING);
            }
        }
    } /* Loop on axes */
}

void
Blt_AxisLimitsToPostScript(Graph *graphPtr, Blt_Ps ps)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    double vMin, hMin, vMax, hMax;
    char string[200];

#define SPACING 8
    vMin = vMax = graphPtr->x1 + graphPtr->padLeft + 2;
    hMin = hMax = graphPtr->y2 - graphPtr->padBottom - 2;   /* Offsets */
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        Axis *axisPtr;
        Tcl_Obj **objv;
        const char *minFmt, *maxFmt;
        int objc;
        unsigned int textWidth, textHeight;

        axisPtr = Blt_GetHashValue(hPtr);
        if (axisPtr->flags & DELETED) {
            continue;                   /* Axis has been deleted. */
        } 
        if (axisPtr->limitsFmtsObjPtr == NULL) {
            continue;                   /* No limits format specified for
                                         * the axis. */
        }
        if (axisPtr->marginPtr == NULL) {
            continue;                   /* Axis is not associated with any
                                         * margin. */
        }
        Tcl_ListObjGetElements(NULL, axisPtr->limitsFmtsObjPtr, &objc, &objv);
        minFmt = maxFmt = Tcl_GetString(objv[0]);
        if (objc > 1) {
            maxFmt = Tcl_GetString(objv[1]);
        }
        if (*maxFmt != '\0') {
            Blt_FormatString(string, 200, maxFmt, axisPtr->axisRange.max);
            Blt_GetTextExtents(axisPtr->tickFont, 0, string, -1, &textWidth,
                &textHeight);
            if ((textWidth > 0) && (textHeight > 0)) {
                if (axisPtr->obj.classId == CID_AXIS_X) {
                    Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
                    Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SE);
                    Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle, 
                        (double)graphPtr->x2, hMax);
                    hMax -= (textWidth + SPACING);
                } else {
                    Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
                    Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_NW);
                    Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle,
                        vMax, (double)graphPtr->y1);
                    vMax += (textWidth + SPACING);
                }
            }
        }
        if (*minFmt != '\0') {
            Blt_FormatString(string, 200, minFmt, axisPtr->axisRange.min);
            Blt_GetTextExtents(axisPtr->tickFont, 0, string, -1, &textWidth,
                &textHeight);
            if ((textWidth > 0) && (textHeight > 0)) {
                Blt_Ts_SetAnchor(axisPtr->limitsTextStyle, TK_ANCHOR_SW);
                if (axisPtr->obj.classId == CID_AXIS_X) {
                    Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 90.0);
                    Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle, 
                        (double)graphPtr->x1, hMin);
                    hMin -= (textWidth + SPACING);
                } else {
                    Blt_Ts_SetAngle(axisPtr->limitsTextStyle, 0.0);
                    Blt_Ps_DrawText(ps, string, &axisPtr->limitsTextStyle, 
                        vMin, (double)graphPtr->y2);
                    vMin += (textWidth + SPACING);
                }
            }
        }
    }
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
Blt_NearestAxis(Graph *graphPtr, int x, int y)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    
    for (hPtr = Blt_FirstHashEntry(&graphPtr->axes.nameTable, &cursor); 
         hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
        Axis *axisPtr;

        axisPtr = Blt_GetHashValue(hPtr);
        if ((axisPtr->marginPtr == NULL) ||
            (axisPtr->flags & (DELETED|HIDDEN))) {
            continue;
        }
        if (axisPtr->flags & TICKLABELS) {
            Blt_ChainLink link;

            for (link = Blt_Chain_FirstLink(axisPtr->tickLabels); link != NULL; 
                 link = Blt_Chain_NextLink(link)) {     
                TickLabel *labelPtr;
                Point2d t;
                double rw, rh;
                Point2d bbox[5];

                labelPtr = Blt_Chain_GetValue(link);
                Blt_GetBoundingBox(labelPtr->width, labelPtr->height, 
                        axisPtr->tickAngle, &rw, &rh, bbox);
                t = Blt_AnchorPoint(labelPtr->anchorPos.x, 
                        labelPtr->anchorPos.y, rw, rh, axisPtr->tickAnchor);
                t.x = x - t.x - (rw * 0.5);
                t.y = y - t.y - (rh * 0.5);

                bbox[4] = bbox[0];
                if (Blt_PointInPolygon(&t, bbox, 5)) {
                    axisPtr->detail = "label";
                    return axisPtr;
                }
            }
        }
        if (axisPtr->title != NULL) {   /* and then the title string. */
            Point2d bbox[5];
            Point2d t;
            double rw, rh;
            unsigned int w, h;

            Blt_GetTextExtents(axisPtr->titleFont, 0, axisPtr->title,-1,&w,&h);
            Blt_GetBoundingBox(w, h, axisPtr->titleAngle, &rw, &rh, bbox);
            t = Blt_AnchorPoint(axisPtr->titlePos.x, axisPtr->titlePos.y, 
                rw, rh, axisPtr->titleAnchor);
            /* Translate the point so that the 0,0 is the upper left 
             * corner of the bounding box.  */
            t.x = x - t.x - (rw * 0.5);
            t.y = y - t.y - (rh * 0.5);
            
            bbox[4] = bbox[0];
            if (Blt_PointInPolygon(&t, bbox, 5)) {
                axisPtr->detail = "title";
                return axisPtr;
            }
        }
        if (axisPtr->lineWidth > 0) {   /* Check for the axis region */
            if ((x <= axisPtr->right) && (x >= axisPtr->left) && 
                (y <= axisPtr->bottom) && (y >= axisPtr->top)) {
                axisPtr->detail = "line";
                return axisPtr;
            }
        }
    }
    return NULL;
}
 
ClientData
Blt_MakeAxisTag(Graph *graphPtr, const char *tagName)
{
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&graphPtr->axes.bindTagTable, tagName, &isNew);
    return Blt_GetHashKey(&graphPtr->axes.bindTagTable, hPtr);
}

#include <time.h>
#include <sys/time.h>

#define SECONDS_SECOND        (1)
#define SECONDS_MINUTE        (60)
#define SECONDS_HOUR          (SECONDS_MINUTE * 60)
#define SECONDS_DAY           (SECONDS_HOUR * 24)
#define SECONDS_WEEK          (SECONDS_DAY * 7)
#define SECONDS_MONTH         (SECONDS_DAY * 30)
#define SECONDS_YEAR          (SECONDS_DAY * 365)

static const int numDaysMonth[2][13] = {
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 31}
} ;
static const int numDaysYear[2] = { 365, 366 };

#   define EPOCH           1970
#   define EPOCH_WDAY      4            /* Thursday. */

static long
NumberDaysFromEpoch(int year)
{
    int y;
    long numDays;

    numDays = 0;
    if (year >= EPOCH) {
        for (y = EPOCH; y < year; y++) {
            numDays += numDaysYear[IsLeapYear(y)];
        }
    } else {
        for (y = year; y < EPOCH; y++)
            numDays -= numDaysYear[IsLeapYear(y)];
    }
    return numDays;
}

static double
TimeFloor(Axis *axisPtr, double min, TimeUnits units, Blt_DateTime *datePtr)
{
    double seconds;
    int mday;

    seconds = floor(min);
    Blt_SecondsToDate(seconds, datePtr);
    mday = 0;                           /* Suppress compiler warning. */
    switch (units) {
    case TIME_YEARS:
        datePtr->mon = 0;               /* 0-11 */
        /* fallthrough */
    case TIME_MONTHS:
    case TIME_WEEKS:
        mday = datePtr->mday;
        datePtr->mday = 1;              /* 1-31, 0 is last day of preceding
                                         * month. */
        /* fallthrough */
    case TIME_DAYS:
        datePtr->hour = 0;              /* 0-23 */
        /* fallthrough */
    case TIME_HOURS:
        datePtr->min = 0;               /* 0-59 */
        /* fallthrough */
    case TIME_MINUTES:
        datePtr->sec = 0;               /* 0-60 */
        /* fallthrough */
    default:
    case TIME_SECONDS:
        break;
    }
    if (units == TIME_WEEKS) {
        mday -= datePtr->wday;
        datePtr->wday = 0;
        if (mday < 1) {
            datePtr->mon--;
            if (datePtr->mon < 0) {
                datePtr->mon = 11;      /* 0-11 */
                datePtr->year--;
            }
            mday += 
                numDaysMonth[IsLeapYear(datePtr->year)][datePtr->mon];
        }
        datePtr->mday = mday;
    }
    datePtr->isdst = 0;
    Blt_DateToSeconds(datePtr, &seconds);
    return seconds;
}

static double
TimeCeil(Axis *axisPtr, double max, TimeUnits units, Blt_DateTime *datePtr)
{
    double seconds;

    seconds = ceil(max);
    switch (units) {
    case TIME_YEARS:
        seconds += SECONDS_YEAR - 1;
        break;
    case TIME_MONTHS:
        seconds += SECONDS_MONTH - 1;
        break;
    case TIME_WEEKS:
        seconds += SECONDS_WEEK - 1;
        break;
    case TIME_DAYS:
        seconds += SECONDS_DAY - 1;
        break;
    case TIME_HOURS:
        seconds += SECONDS_HOUR - 1;
        break;
    case TIME_MINUTES:
        seconds += SECONDS_MINUTE - 1;
        break;
    case TIME_SECONDS:
        seconds += 1.0;
        break;
    default:
        break;
    }
    return TimeFloor(axisPtr, seconds, units, datePtr);
}

static int 
GetMajorTimeUnits(double min, double max)
{
    double range;

    range = max - min;
    if (range <= 0.0) {
        return -1;
    }
    if (range > (SECONDS_YEAR * 1.5)) {
        return TIME_YEARS;
    } 
    if (range > (SECONDS_MONTH * 2.1)) {
        return TIME_MONTHS;
    } 
    if (range > (SECONDS_WEEK * 2)) {
        return TIME_WEEKS;
    } 
    if (range > (SECONDS_DAY * 2)) {
        return TIME_DAYS;
    }
    if (range > (SECONDS_HOUR * 2)) {
        return TIME_HOURS;
    }
    if (range > (SECONDS_MINUTE * 2)) {
        return TIME_MINUTES;
    }
    if (range > (SECONDS_SECOND * 2)) {
        return TIME_SECONDS;
    }
    return TIME_SUBSECONDS;
}

static void
YearTicks(Axis *axisPtr, double min, double max)
{
    Blt_DateTime date1, date2;
    double step;
    int numTicks;
    double tickMin, tickMax;            /* Years. */
    double axisMin, axisMax;            /* Seconds. */
    double numYears;

    axisPtr->major.ticks.scaleType = axisPtr->minor.ticks.scaleType = 
        SCALE_TIME;
    tickMin = TimeFloor(axisPtr, min, TIME_YEARS, &date1);
    tickMax = TimeCeil(axisPtr, max, TIME_YEARS, &date2);
    step = 1.0;
    numYears = date2.year - date1.year;
    if (numYears > 10) {
        long minDays, maxDays;
        double range;

        axisPtr->major.ticks.timeFormat = TIME_FORMAT_YEARS10;
        range = numYears;
        range = NiceNum(range, 0);
        step = NiceNum(range / axisPtr->reqNumMajorTicks, 1);
        tickMin = UFLOOR((double)date1.year, step);
        tickMax = UCEIL((double)date2.year, step);
        range = tickMax - tickMin;
        numTicks = (int)(range / step) + 1;
    
        minDays = NumberDaysFromEpoch((int)tickMin);
        maxDays  = NumberDaysFromEpoch((int)tickMax);
        tickMin = minDays * SECONDS_DAY;
        tickMax = maxDays * SECONDS_DAY;
        axisPtr->major.ticks.year = tickMin;
        axisPtr->minor.ticks.timeUnits = TIME_YEARS;
        if (step > 5) {
            axisPtr->minor.ticks.numSteps = 1;
            axisPtr->minor.ticks.step = step / 2;
        } else {
            axisPtr->minor.ticks.step = 1; /* Years */
            axisPtr->minor.ticks.numSteps = step - 1;
        }
    } else {
        numTicks = numYears + 1;
        step = 0;                       /* Number of days in the year */

        tickMin = NumberDaysFromEpoch(date1.year) * SECONDS_DAY;
        tickMax = NumberDaysFromEpoch(date2.year) * SECONDS_DAY;
        
        axisPtr->major.ticks.year = date1.year;
        if (numYears > 5) {
            axisPtr->major.ticks.timeFormat = TIME_FORMAT_YEARS5;

            axisPtr->minor.ticks.step = (SECONDS_YEAR+1) / 2; /* 1/2 year */
            axisPtr->minor.ticks.numSteps = 1;  /* 3 - 2 */
            axisPtr->minor.ticks.timeUnits = TIME_YEARS;
        } else {
            axisPtr->major.ticks.timeFormat = TIME_FORMAT_YEARS1;
     
            axisPtr->minor.ticks.step = 0; /* Months */
            axisPtr->minor.ticks.numSteps = 11;  /* 12 - 1 */
            axisPtr->minor.ticks.timeUnits = TIME_MONTHS;
            axisPtr->minor.ticks.month = date1.year;
        } 
    }

    axisMin = tickMin;
    axisMax = tickMax;
    if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) &&
         (DEFINED(axisPtr->reqMin)))) {
        axisMin = min;
    }
    if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
         (DEFINED(axisPtr->reqMax)))) {
        axisMax = max;
    }
    SetAxisRange(&axisPtr->axisRange, axisMin, axisMax);

    axisPtr->major.ticks.timeUnits = TIME_YEARS;
    axisPtr->major.ticks.fmt = "%Y";
    axisPtr->major.ticks.scaleType = SCALE_TIME;
    axisPtr->major.ticks.range = tickMax - tickMin;
    axisPtr->major.ticks.initial = tickMin;
    axisPtr->major.ticks.numSteps = numTicks;
    axisPtr->major.ticks.step = step;
}

static void
MonthTicks(Axis *axisPtr, double min, double max)
{
    Blt_DateTime left, right;
    int numMonths;
    double step;
    int numTicks;
    double tickMin, tickMax;            /* months. */
    double axisMin, axisMax;            /* seconds. */
    
    tickMin = axisMin = TimeFloor(axisPtr, min, TIME_MONTHS, &left);
    tickMax = axisMax = TimeCeil(axisPtr, max, TIME_MONTHS, &right);
    if (right.year > left.year) {
        right.mon += (right.year - left.year) * 12;
    }
    numMonths = right.mon - left.mon;
    numTicks = numMonths + 1;
    step = 1;
    if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) &&
         (DEFINED(axisPtr->reqMin)))) {
        axisMin = min;
    }
    if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
         (DEFINED(axisPtr->reqMax)))) {
        axisMax = max;
    }
    SetAxisRange(&axisPtr->axisRange, axisMin, axisMax);
    if (axisPtr->major.ticks.values != NULL) {
        Blt_Free(axisPtr->major.ticks.values);
    }
    axisPtr->major.ticks.initial = tickMin;
    axisPtr->major.ticks.numSteps = numTicks;
    axisPtr->major.ticks.step = step;
    axisPtr->major.ticks.range = tickMax - tickMin;
    axisPtr->major.ticks.isLeapYear = left.isLeapYear;
    axisPtr->major.ticks.fmt = "%h\n%Y";
    axisPtr->major.ticks.month = left.mon;
    axisPtr->major.ticks.year = left.year;
    axisPtr->major.ticks.timeUnits = TIME_MONTHS;
    axisPtr->major.ticks.scaleType = SCALE_TIME;
    
    axisPtr->minor.ticks.numSteps = 5;
    axisPtr->minor.ticks.step = SECONDS_WEEK;
    axisPtr->minor.ticks.month = left.mon;
    axisPtr->minor.ticks.year = left.year;
    axisPtr->minor.ticks.timeUnits = TIME_WEEKS;
    axisPtr->minor.ticks.scaleType = SCALE_TIME;

}

/* 
 *---------------------------------------------------------------------------
 *
 * WeekTicks --
 *
 *    Calculate the ticks for a major axis divided into weeks.  The step for
 *    week ticks is 1 week if the number of week is less than 6.  Otherwise
 *    we compute the linear version of 
 *
 *---------------------------------------------------------------------------
 */
static void
WeekTicks(Axis *axisPtr, double min, double max)
{
    Blt_DateTime left, right;
    int numWeeks;
    double step;
    int numTicks;
    double tickMin, tickMax;            /* days. */
    double axisMin, axisMax;            /* seconds. */

    tickMin = axisMin = TimeFloor(axisPtr, min, TIME_WEEKS, &left);
    tickMax = axisMax = TimeCeil(axisPtr, max, TIME_WEEKS, &right);
    numWeeks = (tickMax - tickMin) / SECONDS_WEEK;
    if (numWeeks > 10) {
        double range;

        fprintf(stderr, "Number of weeks > 10\n");
        range = numWeeks;
        range = NiceNum(range, 0);
        step = NiceNum(range / axisPtr->reqNumMajorTicks, 1);
        numTicks = (int)(range / step) + 1;
        step *= SECONDS_WEEK;
        tickMin = UFLOOR(tickMin, step);
        tickMax = UCEIL(tickMax, step);
        tickMin += (7 - EPOCH_WDAY)*SECONDS_DAY;
        tickMax -= EPOCH_WDAY*SECONDS_DAY;
        axisMin = tickMin;
        axisMax = tickMax;
    } else {
        numTicks = numWeeks + 1;
        step = SECONDS_WEEK;
    }

    if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) &&
         (DEFINED(axisPtr->reqMin)))) {
        axisMin = min;
    }
    if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
         (DEFINED(axisPtr->reqMax)))) {
        axisMax = max;
    }
    SetAxisRange(&axisPtr->axisRange, axisMin, axisMax);

    axisPtr->major.ticks.step = step;
    axisPtr->major.ticks.initial = tickMin;
    axisPtr->major.ticks.numSteps = numTicks;
    axisPtr->major.ticks.timeUnits = TIME_WEEKS;
    axisPtr->major.ticks.range = tickMax - tickMin;
    axisPtr->major.ticks.fmt = "%h %d";
    axisPtr->major.ticks.scaleType = SCALE_TIME;
    axisPtr->minor.ticks.step = SECONDS_DAY;
    axisPtr->minor.ticks.numSteps = 6;  
    axisPtr->minor.ticks.timeUnits = TIME_DAYS;
    axisPtr->minor.ticks.scaleType = SCALE_TIME;
}


/* 
 *---------------------------------------------------------------------------
 *
 * DayTicks --
 *
 *    Calculate the ticks for a major axis divided into days.  The step for
 *    day ticks is always 1 day.  There is no multiple of days that fits 
 *    evenly into a week or month.
 *
 *---------------------------------------------------------------------------
 */
static void
DayTicks(Axis *axisPtr, double min, double max)
{
    Blt_DateTime left, right;
    int numDays, numTicks;
    double step;
    double tickMin, tickMax;            /* days. */
    double axisMin, axisMax;            /* seconds. */

    tickMin = axisMin = TimeFloor(axisPtr, min, TIME_DAYS, &left);
    tickMax = axisMax = TimeCeil(axisPtr, max, TIME_DAYS, &right);
    numDays = (tickMax - tickMin) / SECONDS_DAY;
    numTicks = numDays + 1;
    step = SECONDS_DAY;

    if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) &&
         (DEFINED(axisPtr->reqMin)))) {
        axisMin = min;
    }
    if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
         (DEFINED(axisPtr->reqMax)))) {
        axisMax = max;
    }
    SetAxisRange(&axisPtr->axisRange, axisMin, axisMax);

    axisPtr->major.ticks.step = step;
    axisPtr->major.ticks.initial = tickMin;
    axisPtr->major.ticks.numSteps = numTicks;
    axisPtr->major.ticks.timeUnits = TIME_DAYS;
    axisPtr->major.ticks.range = tickMax - tickMin;
    axisPtr->major.ticks.scaleType = SCALE_TIME;
    axisPtr->major.ticks.fmt = "%h %d";
    axisPtr->minor.ticks.step = SECONDS_HOUR * 6;
    axisPtr->minor.ticks.initial = 0;
    axisPtr->minor.ticks.numSteps = 2;  /* 6 - 2 */
    axisPtr->minor.ticks.timeUnits = TIME_HOURS;
    axisPtr->minor.ticks.scaleType = SCALE_TIME;
}

/* 
 *---------------------------------------------------------------------------
 *
 * HourTicks --
 *
 *    Calculate the ticks for a major axis divided in hours.  The hour step
 *    should evenly divide into 24 hours, so we select the step based on the 
 *    number of hours in the range.
 *
 *---------------------------------------------------------------------------
 */
static void
HourTicks(Axis *axisPtr, double min, double max)
{
    Blt_DateTime left, right;
    double axisMin, axisMax;            
    double tickMin, tickMax;            
    int numTicks;
    int numHours;

    double step;

    tickMin = axisMin = TimeFloor(axisPtr, min, TIME_HOURS, &left);
    tickMax = axisMax = TimeCeil(axisPtr, max, TIME_HOURS, &right);
    numHours = (tickMax - tickMin) / SECONDS_HOUR;
    if (numHours < 7) {                 /* 3-6 hours */
        step = SECONDS_HOUR;            
    } else if (numHours < 13) {         /* 7-12 hours */
        step = SECONDS_HOUR * 2;
    } else if (numHours < 25) {         /* 13-24 hours */
        step = SECONDS_HOUR * 4;
    } else if (numHours < 36) {         /* 23-35 hours */
        step = SECONDS_HOUR * 6;
    } else {                            /* 33-48 hours */
        step = SECONDS_HOUR * 8;
    }

    axisMin = tickMin = UFLOOR(tickMin, step);
    axisMax = tickMax = UCEIL(tickMax, step);
    numTicks = ((tickMax - tickMin) / (long)step) + 1;

    if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) &&
         (DEFINED(axisPtr->reqMin)))) {
        axisMin = min;
    }
    if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
         (DEFINED(axisPtr->reqMax)))) {
        axisMax = max;
    }
    SetAxisRange(&axisPtr->axisRange, axisMin, axisMax);
    axisPtr->major.ticks.step = step;
    axisPtr->major.ticks.initial = tickMin;
    axisPtr->major.ticks.numSteps = numTicks;
    axisPtr->major.ticks.range = tickMax - tickMin;
    axisPtr->major.ticks.fmt = "%H:%M\n%h %d";
    axisPtr->major.ticks.timeUnits = TIME_HOURS;
    axisPtr->major.ticks.scaleType = SCALE_TIME;

    axisPtr->minor.ticks.step = step / 4;
    axisPtr->minor.ticks.numSteps = 4;  /* 6 - 2 */
    axisPtr->minor.ticks.timeUnits = TIME_MINUTES;
    axisPtr->minor.ticks.scaleType = SCALE_TIME;
}

/* 
 *---------------------------------------------------------------------------
 *
 * MinuteTicks --
 *
 *    Calculate the ticks for a major axis divided in minutes.  Minutes can
 *    be in steps of 5 or 10 so we can use the standard tick selecting
 *    procedures.
 *
 *---------------------------------------------------------------------------
 */
static void
MinuteTicks(Axis *axisPtr, double min, double max)
{
    Blt_DateTime left, right;
    int numMinutes, numTicks;
    double step, range;
    double tickMin, tickMax;            /* minutes. */
    double axisMin, axisMax;            /* seconds. */

    tickMin = axisMin = TimeFloor(axisPtr, min, TIME_MINUTES, &left);
    tickMax = axisMax = TimeCeil(axisPtr, max, TIME_MINUTES, &right);
    numMinutes = (tickMax - tickMin) / SECONDS_MINUTE;

    range = numMinutes;
    range = NiceNum(range, 0);
    step = NiceNum(range / axisPtr->reqNumMajorTicks, 1);
    numTicks = (int)(range / step) + 1;
    step *= SECONDS_MINUTE;
    axisMin = tickMin = UFLOOR(tickMin, step);
    axisMax = tickMax = UCEIL(tickMax, step);

    if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) &&
         (DEFINED(axisPtr->reqMin)))) {
        axisMin = min;
    }
    if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
         (DEFINED(axisPtr->reqMax)))) {
        axisMax = max;
    }
    SetAxisRange(&axisPtr->axisRange, axisMin, axisMax);
    if (axisPtr->major.ticks.values != NULL) {
        Blt_Free(axisPtr->major.ticks.values);
    }
    axisPtr->major.ticks.step = step;
    axisPtr->major.ticks.initial = tickMin;
    axisPtr->major.ticks.numSteps = numTicks;
    axisPtr->major.ticks.timeUnits = TIME_MINUTES;
    axisPtr->major.ticks.scaleType = SCALE_TIME;
    axisPtr->major.ticks.fmt = "%H:%M";
    axisPtr->major.ticks.range = tickMax - tickMin;

    axisPtr->minor.ticks.step = step / (axisPtr->reqNumMinorTicks - 1);
    axisPtr->minor.ticks.numSteps = axisPtr->reqNumMinorTicks;
    axisPtr->minor.ticks.timeUnits = TIME_MINUTES;
    axisPtr->minor.ticks.scaleType = SCALE_TIME;
}

/* 
 *---------------------------------------------------------------------------
 *
 * SecondTicks --
 *
 *    Calculate the ticks for a major axis divided into seconds.  Seconds
 *    can be in steps of 5 or 10 so we can use the standard tick selecting
 *    procedures.
 *
 *---------------------------------------------------------------------------
 */
static void
SecondTicks(Axis *axisPtr, double min, double max)
{
    double step, range;
    int numTicks;
    double tickMin, tickMax;            /* minutes. */
    double axisMin, axisMax;            /* seconds. */
    long numSeconds;

    numSeconds = (long)(max - min);
    step = 1.0;
    range = numSeconds;
    if (axisPtr->reqStep > 0.0) {
        /* An interval was designated by the user.  Keep scaling it until
         * it fits comfortably within the current range of the axis.  */
        step = axisPtr->reqStep;
        while ((2 * step) >= range) {
            step *= 0.5;
        }
    } else {
        range = NiceNum(range, 0);
        step = NiceNum(range / axisPtr->reqNumMajorTicks, 1);
    }
    /* Find the outer tick values. Add 0.0 to prevent getting -0.0. */
    axisMin = tickMin = UFLOOR(min, step);
    axisMax = tickMax = UCEIL(max, step);
    numTicks = ROUND((tickMax - tickMin) / step) + 1;
    if ((axisPtr->looseMin == TIGHT) || ((axisPtr->looseMin == LOOSE) &&
         (DEFINED(axisPtr->reqMin)))) {
        axisMin = min;
    }
    if ((axisPtr->looseMax == TIGHT) || ((axisPtr->looseMax == LOOSE) &&
         (DEFINED(axisPtr->reqMax)))) {
        axisMax = max;
    }
    SetAxisRange(&axisPtr->axisRange, axisMin, axisMax);
    axisPtr->major.ticks.initial = tickMin;
    axisPtr->major.ticks.numSteps = numTicks;
    axisPtr->major.ticks.step = step;
    axisPtr->major.ticks.fmt = "%H:%M:%s";
    axisPtr->major.ticks.timeUnits = TIME_SECONDS;
    axisPtr->major.ticks.scaleType = SCALE_TIME;
    axisPtr->major.ticks.range = tickMax - tickMin;

    axisPtr->minor.ticks.step = 1.0 / (double)axisPtr->reqNumMinorTicks;
    axisPtr->minor.ticks.numSteps = axisPtr->reqNumMinorTicks - 1;
    axisPtr->minor.ticks.scaleType = SCALE_LINEAR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TimeAxis --
 *
 *      Determine the units of a linear scaled axis.
 *
 * Results:
 *      None.
 *
 * Side Effects:
 *      The tick values are generated.
 *
 *---------------------------------------------------------------------------
 */
static void
TimeAxis(Axis *axisPtr, double min, double max)
{
    int units;

    units = GetMajorTimeUnits(min, max);
    if (units == -1) {
        return;
    }
    switch (units) {
    case TIME_YEARS:
        YearTicks(axisPtr, min, max);
        break;
    case TIME_MONTHS:
        MonthTicks(axisPtr, min, max);
        break;
    case TIME_WEEKS:
        WeekTicks(axisPtr, min, max);
        break;
    case TIME_DAYS:
        DayTicks(axisPtr, min, max);
        break;
    case TIME_HOURS:
        HourTicks(axisPtr, min, max);
        break;
    case TIME_MINUTES:
        MinuteTicks(axisPtr, min, max);
        break;
    case TIME_SECONDS:
        SecondTicks(axisPtr, min, max);
        break;
    case TIME_SUBSECONDS:
        LinearAxis(axisPtr, min, max);
        axisPtr->major.ticks.scaleType = SCALE_TIME;
        axisPtr->major.ticks.timeUnits = units;
        break;
    default:
        Blt_Panic("unknown time units");
    }
}

static double logTable[] = {
    0.301029995663981,                  /* 1 */
    0.477121254719662,                  /* 2 */
    0.602059991327962,                  /* 3 */
    0.698970004336019,                  /* 4 */
    0.778151250383644,                  /* 5 */
    0.845098040014257,                  /* 6 */
    0.903089986991944,                  /* 7 */
    0.954242509439325,                  /* 8 */
};


static Tick
FirstMajorTick(Axis *axisPtr)
{
    Ticks *ticksPtr;
    Tick tick;

    ticksPtr = &axisPtr->major.ticks;
    ticksPtr->index = 0;
    ticksPtr->numDaysFromInitial = 0;
    tick.isValid = FALSE;
    tick.value = Blt_NaN();
#ifdef notdef
    fprintf(stderr, "scaleType is %d, timeUnits=%d\n", ticksPtr->scaleType,
            ticksPtr->timeUnits);
#endif
    switch (ticksPtr->scaleType) {
    case SCALE_CUSTOM:                  /* User defined minor ticks */
        tick.value = ticksPtr->values[0];
        break;
    case SCALE_TIME:
        switch (ticksPtr->timeUnits) {
        case TIME_YEARS:
            {
                Blt_DateTime date;

                Blt_SecondsToDate(ticksPtr->initial, &date);
                ticksPtr->isLeapYear = date.isLeapYear;
                ticksPtr->year = date.year;
            }
            break;
        case TIME_MONTHS:
            if (ticksPtr->numSteps <= 3) {
                axisPtr->minor.ticks.numSteps = 
                    numDaysMonth[ticksPtr->isLeapYear][ticksPtr->month];  
                axisPtr->minor.ticks.step = SECONDS_DAY;
            } 
            break;
        default:
            break;
        }
        tick.value = ticksPtr->initial;
        break;
    case SCALE_LOG:
    case SCALE_LINEAR:
    default:
        /* The number of major ticks and the step has been computed in
         * LinearAxis. */
        tick.value = ticksPtr->initial;
        break;
    }
    if (ticksPtr->index >= ticksPtr->numSteps) {
        return tick;
    }
#ifdef notdef
    fprintf(stderr, "FirstMajorTick: tick.value=%.15g\n", tick.value);
#endif
    tick.isValid = TRUE;
    return tick;
}

static Tick
NextMajorTick(Axis *axisPtr)
{
    double d;                           /* Delta from initial to major
                                         * tick. */
    Ticks *ticksPtr;
    Tick tick;

    ticksPtr = &axisPtr->major.ticks;
    ticksPtr->index++;
    tick.isValid = FALSE;
    tick.value = Blt_NaN();
    if (ticksPtr->index >= ticksPtr->numSteps) {
        return tick;
    }
    d = ticksPtr->initial; 
    switch (ticksPtr->scaleType) {
    case SCALE_LINEAR:
    default:
        d += ticksPtr->index * ticksPtr->step;
        d = UROUND(d, ticksPtr->step) + 0.0;
        break;

    case SCALE_LOG:
        d += ticksPtr->range * logTable[ticksPtr->index];
        break;

    case SCALE_CUSTOM:                  /* User defined minor ticks */
        tick.value = ticksPtr->values[ticksPtr->index];
        tick.isValid = TRUE;
        return tick;

    case SCALE_TIME:
        switch (ticksPtr->timeUnits) {
        case TIME_YEARS:
            switch(ticksPtr->timeFormat) {
            case TIME_FORMAT_YEARS10:
                {
                    int i;
                    
                    for (i = 0; i < ticksPtr->step; i++) {
                        int year, numDays;
                        
                        year = ticksPtr->year++;
                        numDays = numDaysYear[IsLeapYear(year)]; 
                        ticksPtr->numDaysFromInitial += numDays;
                    }
                    d += ticksPtr->numDaysFromInitial * SECONDS_DAY;
                }
                break;
            case TIME_FORMAT_YEARS5:
            case TIME_FORMAT_YEARS1:
                {
                    int i;
                    
                    for (i = 0; i < ticksPtr->index; i++) {
                        int year, numDays;
                        
                        year = ticksPtr->year + i;
                        numDays = numDaysYear[IsLeapYear(year)]; 
                        d += numDays * SECONDS_DAY;
                    }
                }
                break;
            case TIME_FORMAT_SECONDS:
            default:
                break;
            }
            break;
        case TIME_MONTHS:
            {
                long numDays;
                int mon, year;
                int i;

                numDays = 0;
                mon = ticksPtr->month, year = ticksPtr->year;
                for (i = 0; i < ticksPtr->index; i++, mon++) {
                    if (mon > 11) {
                        mon = 0;
                        year++;
                    }
                    numDays += numDaysMonth[IsLeapYear(year)][mon];
                }
                d += numDays * SECONDS_DAY;
            }
            break;
        case TIME_WEEKS:
        case TIME_HOURS:
        case TIME_MINUTES:
            d += ticksPtr->index * ticksPtr->step;
            break;
        case TIME_DAYS:
            d += ticksPtr->index * ticksPtr->step;
            break;
        case TIME_SECONDS:
        case TIME_SUBSECONDS:
            d += ticksPtr->index * ticksPtr->step;
            d = UROUND(d, ticksPtr->step);
            break;
        }
        break;

    }
    tick.value = d;
    tick.isValid = TRUE;
#ifdef notdef
    fprintf(stderr, "NextMajorTick: tick.value=%.15g\n", tick.value);
#endif
    return tick;
}

static Tick
FirstMinorTick(Axis *axisPtr)
{
    double d;                           /* Delta from major to minor
                                         * tick. */
    Ticks *ticksPtr;
    Tick tick;

    ticksPtr = &axisPtr->minor.ticks;
    ticksPtr->numDaysFromInitial = 0;
    ticksPtr->index = 0;
    tick.isValid = FALSE;
    tick.value = Blt_NaN();
    d = 0.0;                            /* Suppress compiler warning. */
    switch (ticksPtr->scaleType) {
    case SCALE_LINEAR:
    default:
        d = ticksPtr->step * ticksPtr->range;
        break;

    case SCALE_CUSTOM:                  /* User defined minor ticks */
        d = ticksPtr->values[0] * ticksPtr->range;
        break;

    case SCALE_LOG:
        d = logTable[0] * ticksPtr->range;
        break;

    case SCALE_TIME:
        switch (ticksPtr->timeUnits) {
        case TIME_YEARS:
            {
                Blt_DateTime date;
                int i;

                Blt_SecondsToDate(ticksPtr->initial, &date);
                ticksPtr->isLeapYear = date.isLeapYear;
                ticksPtr->year = date.year;
                for (i = 0; i < ticksPtr->step; i++) {
                    int year, numDays;

                    year = ticksPtr->year++;
                    numDays = numDaysYear[IsLeapYear(year)];
                    ticksPtr->numDaysFromInitial += numDays;
                }
                d = ticksPtr->numDaysFromInitial * SECONDS_DAY;
            }
            break;
        case TIME_MONTHS:
            {
                Blt_DateTime date;
                int numDays;

                Blt_SecondsToDate(ticksPtr->initial, &date);
                ticksPtr->isLeapYear = date.isLeapYear;
                numDays = numDaysMonth[date.isLeapYear][date.mon];
                ticksPtr->month = date.mon;
                ticksPtr->year = date.year;
                d = numDays * SECONDS_DAY;
            }
            break;
        case TIME_DAYS:
            if (ticksPtr->numSteps == 1) {
                ticksPtr->step = ticksPtr->range * 0.5;
            } 
            d = ticksPtr->step;
            break;
        case TIME_WEEKS:
            {
                Blt_DateTime date;

                Blt_SecondsToDate(ticksPtr->initial, &date);
                ticksPtr->numDaysFromInitial = (7 - date.wday);
                d = ticksPtr->numDaysFromInitial * SECONDS_DAY;
            }
            break;
        case TIME_HOURS:
        case TIME_MINUTES:
            ticksPtr->step = ticksPtr->range / ticksPtr->numSteps;
            d = ticksPtr->step;
            break;
        case TIME_SECONDS:
        case TIME_SUBSECONDS:
            /* The number of minor ticks has been computed in
             * LinearAxis. */
            d = ticksPtr->step;
            d = UROUND(d, ticksPtr->step);
            break;
        }
        break;
    }
    if (ticksPtr->index >= ticksPtr->numSteps) {
        return tick;
    }
    tick.isValid = TRUE;
    tick.value = ticksPtr->initial + d;
    return tick;
}

static Tick
NextMinorTick(Axis *axisPtr)
{
    double d;                           /* Delta from major to minor
                                         * tick. */
    Ticks *ticksPtr;
    Tick tick;

    ticksPtr = &axisPtr->minor.ticks;
    ticksPtr->index++;
    tick.isValid = FALSE;
    tick.value = Blt_NaN();
    if (ticksPtr->index >= ticksPtr->numSteps) {
        return tick;
    }
    d = ticksPtr->initial;              
    switch (ticksPtr->scaleType) {
    case SCALE_LINEAR:
    default:
        d += ticksPtr->range * (ticksPtr->index + 1) * ticksPtr->step;
        break;

    case SCALE_CUSTOM:                  /* User defined minor ticks */
        d += ticksPtr->range * ticksPtr->values[ticksPtr->index];
        break;

    case SCALE_LOG:
        d += ticksPtr->range * logTable[ticksPtr->index];
        break;

    case SCALE_TIME:
        switch (ticksPtr->timeUnits) {
        case TIME_YEARS:
            {
                int i;

                for (i = 0; i < ticksPtr->step; i++) {
                    int year, numDays;

                    year = ticksPtr->year++;
                    numDays = numDaysYear[IsLeapYear(year)];
                    ticksPtr->numDaysFromInitial += numDays;
                }
                d += ticksPtr->numDaysFromInitial * SECONDS_DAY;
            }
            break;
        case TIME_MONTHS:
            {
                int mon, year;
                int i;

                mon = ticksPtr->month + 1, year = ticksPtr->year;
                for (i = 0; i <= ticksPtr->index; i++, mon++) {
                    int numDays;

                    if (mon > 11) {
                        mon = 0;
                        year++;
                    }
                    numDays = numDaysMonth[IsLeapYear(year)][mon];
                    d += numDays;
                }
                d *= SECONDS_DAY;
            }
            break;
        case TIME_WEEKS:
            ticksPtr->numDaysFromInitial += 7;
            d += ticksPtr->numDaysFromInitial * SECONDS_DAY;
            break;
        case TIME_DAYS:
        case TIME_HOURS:
        case TIME_MINUTES:
            d += (ticksPtr->index + 1) * ticksPtr->step;
            break;
        case TIME_SECONDS:
        case TIME_SUBSECONDS:
            d += (ticksPtr->range * ticksPtr->step * ticksPtr->index);
            break;
        }
        break;
    }
    tick.isValid = TRUE;
    tick.value = d;
    return tick;
}

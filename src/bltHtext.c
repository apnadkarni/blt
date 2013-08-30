/*
 * bltHtext.c --
 *
 * This module implements a hypertext widget for the BLT toolkit.
 *
 *	Copyright 1991-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use,
 *	copy, modify, merge, publish, distribute, sublicense, and/or
 *	sell copies of the Software, and to permit persons to whom the
 *	Software is furnished to do so, subject to the following
 *	conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the
 *	Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
 *	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 *	PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 *	OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *	OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *	OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *	SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * To do:
 *
 * 1) Fix scroll unit round off errors.
 *
 * 2) Better error checking.
 *
 * 3) Use html format.
 *
 * 4) The dimension of cavities using -relwidth and -relheight
 *    should be 0 when computing initial estimates for the size
 *    of the virtual text.
 */

#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_HTEXT

#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif	/* HAVE_SYS_STAT_H */

#include <X11/Xatom.h>

#include "bltAlloc.h"
#include "bltMath.h"
#include <bltChain.h>
#include <bltHash.h>
#include "bltFont.h"
#include "bltText.h"
#include "bltBg.h"
#include "bltOp.h"
#include "bltInitCmd.h"
 

#define DEF_LINES_ALLOC 512	/* Default block of lines allocated */
#define CLAMP(val,low,hi)	\
	(((val) < (low)) ? (low) : ((val) > (hi)) ? (hi) : (val))

/*
 * Justify option values
 */
typedef enum {
    JUSTIFY_CENTER, JUSTIFY_TOP, JUSTIFY_BOTTOM
} Justify;

static Blt_OptionParseProc ObjToWidth, ObjToHeight;
static Blt_OptionPrintProc WidthHeightToObj;
static Blt_CustomOption widthOption =
{
    ObjToWidth, WidthHeightToObj, NULL, (ClientData)0
};

static Blt_CustomOption heightOption =
{
    ObjToHeight, WidthHeightToObj, NULL, (ClientData)0
};

static Blt_OptionParseProc ObjToJustify;
static Blt_OptionPrintProc JustifyToObj;
static Blt_CustomOption justifyOption =
{
    ObjToJustify, JustifyToObj, NULL, (ClientData)0
};

static Tk_GeomRequestProc EmbeddedWidgetGeometryProc;
static Tk_GeomLostSlaveProc EmbeddedWidgetCustodyProc;
static Tk_GeomMgr htextMgrInfo =
{
    (char *)"htext",			/* Name of geometry manager used by
					 * winfo */
    EmbeddedWidgetGeometryProc,		/* Procedure to for new geometry
					 * requests */
    EmbeddedWidgetCustodyProc,		/* Procedure when window is taken
					 * away */
};


/*
 * Line --
 *
 *	Structure to contain the contents of a single line of text and the
 *	widgets on that line.
 *
 * 	Individual lines are not configurable, although changes to the size of
 * 	widgets do effect its values.
 */
typedef struct {
    int offset;				/* Offset of line from y-origin (0) in
					 * world coordinates */
    int baseline;			/* Baseline y-coordinate of the
					 * text */
    short int width, height;		/* Dimensions of the line */
    int textStart, textEnd;		/* Start and end indices of characters
					 * forming the line in the text array */
    Blt_Chain chain;			/* Chain of embedded widgets on the
					 * line of text */
} Line;

typedef struct {
    int textStart;
    int textEnd;
} Segment;

typedef struct {
    int x, y;
} Position;

/*
 * Hypertext widget.
 */
typedef struct {
    Tk_Window tkwin;			/* Window that embodies the widget.
					 * NULL means that the window has been
					 * destroyed but the data structures
					 * haven't yet been cleaned up.*/
    Display *display;			/* Display containing widget; needed,
					 * among other things, to release
					 * resources after tkwin has already
					 * gone away. */
    Tcl_Interp *interp;			/* Interpreter associated with
					 * widget. */
    Tcl_Command cmdToken;	       /* Token for htext's widget command. */
    int flags;

    /* User-configurable fields */

    XColor *normalFg;
    Blt_Bg normalBg;
    Blt_Font font;			/* Font for normal text. May affect
					 * the size of the viewport if the
					 * width/height is specified in
					 * columns/rows */
    GC drawGC;				/* Graphics context for normal text */
    int tileOffsetPage;			/* Set tile offset to top of page
					 * instead of toplevel window */
    GC fillGC;				/* GC for clearing the window in the
					 * designated background color. The
					 * background color is the foreground
					 * attribute in GC.  */

    int numRows, numColumns;		/* # of characters of the current font
					 * for a row or column of the viewport.
					 * Used to determine the width and height
					 * of the text window (i.e. viewport) */
    int reqWidth, reqHeight;		/* Requested dimensions of the
					 * viewport */
    int maxWidth, maxHeight;		/* Maximum dimensions allowed for the
					 * viewport, regardless of the size of
					 * the text */

    Tk_Cursor cursor;			/* X Cursor */

    char *fileName;		       /* If non-NULL, indicates the name of a
					* hypertext file to be read into the
					* widget. If NULL, the *text* field is
					* considered instead */
    char *text;				/* Hypertext to be loaded into the
					 * widget. This value is ignored if
					 * *fileName* * is non-NULL */
    int specChar;			/* Special character designating a TCL
					 * command block in a hypertext
					 * file. */
    int leader;				/* # of pixels between lines */

    Tcl_Obj *yScrollCmdObjPtr;		/* Name of vertical scrollbar to invoke */
    int yScrollUnits;			/* # of pixels per vertical scroll */
    Tcl_Obj *xScrollCmdObjPtr;		/* Name of horizontal scroll bar to invoke */
    int xScrollUnits;			/* # of pixels per horizontal
					   # scroll */

    int reqLineNum;			/* Line requested by "goto" command */

    /*
     * The view port is the width and height of the window and the
     * origin of the viewport (upper left corner) in world coordinates.
     */
    int worldWidth, worldHeight;/* Size of view text in world coordinates */
    int xOffset, yOffset;	/* Position of viewport in world coordinates */

    int pendingX, pendingY;	/* New upper-left corner (origin) of
				 * the viewport (not yet posted) */

    int first, last;		/* Range of lines displayed */

    int lastWidth, lastHeight;
    /* Last known size of the window: saved to
				 * recognize when the viewport is resized. */

    Blt_HashTable widgetTable;	/* Table of embedded widgets. */

    /*
     * Selection display information:
     */
    Blt_Bg selBg;	/* Border and background color */
    int selBW;		/* Border width */
    XColor *selFgColor;		/* Text foreground color */
    GC selectGC;		/* GC for drawing selected text */
    int selAnchor;		/* Fixed end of selection
			         * (i.e. "selection to" operation will
			         * use this as one end of the selection).*/
    int selFirst;		/* The index of first character in the
				 * text array selected */
    int selLast;		/* The index of the last character selected */
    int exportSelection;	/* Non-zero means to export the internal text
				 * selection to the X server. */
    char *takeFocus;

    /*
     * Scanning information:
     */
    XPoint scanMark;		/* Anchor position of scan */
    XPoint scanPt;		/* x,y position where the scan started. */

    char *charArr;		/* Pool of characters representing the text
				 * to be displayed */
    int numChars;			/* Length of the text pool */

    Line *lineArr;		/* Array of pointers to text lines */
    int numLines;			/* # of line entered into array. */
    int arraySize;		/* Size of array allocated. */

} HText;

/*
 * Bit flags for the hypertext widget:
 */
#define REDRAW_PENDING	 (1<<0)	/* A DoWhenIdle handler has already
				 * been queued to redraw the window */
#define IGNORE_EXPOSURES (1<<1)	/* Ignore exposure events in the text
				 * window.  Potentially many expose
				 * events can occur while rearranging
				 * embedded widgets during a single call to
				 * the DisplayText.  */

#define REQUEST_LAYOUT 	(1<<4)	/* Something has happened which
				 * requires the layout of text and
				 * embedded widget positions to be
				 * recalculated.  The following
				 * actions may cause this:
				 *
				 * 1) the contents of the hypertext
				 *    has changed by either the -file or
				 *    -text options.
				 *
				 * 2) a text attribute has changed
				 *    (line spacing, font, etc)
				 *
				 * 3) a embedded widget has been resized or
				 *    moved.
				 *
				 * 4) a widget configuration option has
				 *    changed.
				 */
#define TEXT_DIRTY 	(1<<5)	/* The layout was recalculated and the
				 * size of the world (text layout) has
				 * changed. */
#define GOTO_PENDING 	(1<<6)	/* Indicates the starting text line
				 * number has changed. To be reflected
				 * the next time the widget is redrawn. */
#define WIDGET_APPENDED	(1<<7)	/* Indicates a embedded widget has just
				 * been appended to the text.  This is
				 * used to determine when to add a
				 * space to the text array */

#define DEF_HTEXT_BACKGROUND		STD_NORMAL_BACKGROUND
#define DEF_HTEXT_CURSOR		"arrow"
#define DEF_HTEXT_EXPORT_SELECTION	"1"

#define DEF_HTEXT_FOREGROUND		STD_NORMAL_FOREGROUND
#define DEF_HTEXT_FILE_NAME		(char *)NULL
#define DEF_HTEXT_FONT			STD_FONT
#define DEF_HTEXT_HEIGHT		"0"
#define DEF_HTEXT_LINE_SPACING		"1"
#define DEF_HTEXT_MAX_HEIGHT		(char *)NULL
#define DEF_HTEXT_MAX_WIDTH 		(char *)NULL
#define DEF_HTEXT_SCROLL_UNITS		"10"
#define DEF_HTEXT_SPEC_CHAR		"0x25"
#define DEF_HTEXT_SELECT_BORDERWIDTH 	STD_SELECT_BORDERWIDTH
#define DEF_HTEXT_SELECT_BACKGROUND 	STD_SELECT_BACKGROUND
#define DEF_HTEXT_SELECT_FOREGROUND 	STD_SELECT_FOREGROUND
#define DEF_HTEXT_TAKE_FOCUS		"1"
#define DEF_HTEXT_TEXT			(char *)NULL
#define DEF_HTEXT_TILE_OFFSET		"1"
#define DEF_HTEXT_WIDTH			"0"

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BACKGROUND, "-background", "background", "Background",
	DEF_HTEXT_BACKGROUND, Blt_Offset(HText, normalBg), 0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_HTEXT_CURSOR, Blt_Offset(HText, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-exportselection", "exportSelection", "ExportSelection",
	DEF_HTEXT_EXPORT_SELECTION, Blt_Offset(HText, exportSelection), 0},
    {BLT_CONFIG_SYNONYM, "-fg", "foreground", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_STRING, "-file", "file", "File",
	DEF_HTEXT_FILE_NAME, Blt_Offset(HText, fileName), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FONT, "-font", "font", "Font",
	DEF_HTEXT_FONT, Blt_Offset(HText, font), 0},
    {BLT_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	DEF_HTEXT_FOREGROUND, Blt_Offset(HText, normalFg), 0},
    {BLT_CONFIG_CUSTOM, "-height", "height", "Height", DEF_HTEXT_HEIGHT, 
	Blt_Offset(HText, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT, 
	&heightOption},
    {BLT_CONFIG_PIXELS_NNEG, "-linespacing", "lineSpacing", "LineSpacing", 
	DEF_HTEXT_LINE_SPACING, Blt_Offset(HText, leader),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-maxheight", "maxHeight", "MaxHeight",
	DEF_HTEXT_MAX_HEIGHT, Blt_Offset(HText, maxHeight),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-maxwidth", "maxWidth", "MaxWidth",
	DEF_HTEXT_MAX_WIDTH, Blt_Offset(HText, maxWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BORDER, "-selectbackground", "selectBackground", "Background",
	DEF_HTEXT_SELECT_BACKGROUND, Blt_Offset(HText, selBg), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-selectborderwidth", "selectBorderWidth", 
	"BorderWidth", DEF_HTEXT_SELECT_BORDERWIDTH, 
	Blt_Offset(HText, selBW), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-selectforeground", "selectForeground", "Foreground",
	DEF_HTEXT_SELECT_FOREGROUND, Blt_Offset(HText, selFgColor), 0},
    {BLT_CONFIG_INT, "-specialchar", "specialChar", "SpecialChar",
	DEF_HTEXT_SPEC_CHAR, Blt_Offset(HText, specChar), 0},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_HTEXT_TAKE_FOCUS, Blt_Offset(HText, takeFocus),
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-tileoffset", "tileOffset", "TileOffset",
	DEF_HTEXT_TILE_OFFSET, Blt_Offset(HText, tileOffsetPage), 0},
    {BLT_CONFIG_STRING, "-text", "text", "Text",
	DEF_HTEXT_TEXT, Blt_Offset(HText, text), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_CUSTOM, "-width", "width", "Width", DEF_HTEXT_WIDTH, 
	Blt_Offset(HText, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT, &widthOption},
    {BLT_CONFIG_OBJ, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	(char *)NULL, Blt_Offset(HText, xScrollCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-xscrollunits", "xScrollUnits", "ScrollUnits",
	DEF_HTEXT_SCROLL_UNITS, Blt_Offset(HText, xScrollUnits),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_OBJ, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	(char *)NULL, Blt_Offset(HText, yScrollCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-yscrollunits", "yScrollUnits", "yScrollUnits",
	DEF_HTEXT_SCROLL_UNITS, Blt_Offset(HText, yScrollUnits),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};

typedef struct {
    HText *htPtr;		/* Pointer to parent's Htext structure */
    Tk_Window tkwin;		/* Widget window */
    int flags;

    int x, y;			/* Origin of embedded widget in text */

    int cavityWidth, cavityHeight; /* Dimensions of the cavity
				    * surrounding the embedded widget */
    /*
     *  Dimensions of the embedded widget.  Compared against actual
     *	embedded widget sizes when checking for resizing.
     */
    int winWidth, winHeight;

    int precedingTextEnd;	/* Index (in charArr) of the the last
				 * character immediatedly preceding
				 * the embedded widget */
    int precedingTextWidth;	/* Width of normal text preceding widget. */

    Tk_Anchor anchor;
    Justify justify;		/* Justification of region wrt to line */

    /*
     * Requested dimensions of the cavity (includes padding). If non-zero,
     * it overrides the calculated dimension of the cavity.
     */
    int reqCavityWidth, reqCavityHeight;

    /*
     * Relative dimensions of cavity wrt the size of the viewport. If
     * greater than 0.0.
     */
    double relCavityWidth, relCavityHeight;

    int reqWidth, reqHeight;	/* If non-zero, overrides the requested
				 * dimension of the embedded widget */

    double relWidth, relHeight;	/* Relative dimensions of embedded
				 * widget wrt the size of the viewport */

    Blt_Pad xPad, yPad;		/* Extra padding to frame around */

    int ixPad, iyPad;		/* internal padding for window */

    int fill;			/* Fill style flag */

} EmbeddedWidget;

/*
 * Flag bits embedded widgets:
 */
#define WIDGET_VISIBLE	(1<<2)	/* Widget is currently visible in the
				 * viewport. */
#define WIDGET_NOT_CHILD (1<<3) /* Widget is not a child of hypertext. */
/*
 * Defaults for embedded widgets:
 */
#define DEF_WIDGET_ANCHOR        "center"
#define DEF_WIDGET_FILL		"none"
#define DEF_WIDGET_HEIGHT	"0"
#define DEF_WIDGET_JUSTIFY	"center"
#define DEF_WIDGET_PAD_X		"0"
#define DEF_WIDGET_PAD_Y		"0"
#define DEF_WIDGET_REL_HEIGHT	"0.0"
#define DEF_WIDGET_REL_WIDTH  	"0.0"
#define DEF_WIDGET_WIDTH  	"0"

static Blt_ConfigSpec widgetConfigSpecs[] =
{
    {BLT_CONFIG_ANCHOR, "-anchor", (char *)NULL, (char *)NULL,
	DEF_WIDGET_ANCHOR, Blt_Offset(EmbeddedWidget, anchor),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_FILL, "-fill", (char *)NULL, (char *)NULL,
	DEF_WIDGET_FILL, Blt_Offset(EmbeddedWidget, fill),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-cavityheight", (char *)NULL, (char *)NULL,
	DEF_WIDGET_HEIGHT, Blt_Offset(EmbeddedWidget, reqCavityHeight),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-cavitywidth", (char *)NULL, (char *)NULL,
	DEF_WIDGET_WIDTH, Blt_Offset(EmbeddedWidget, reqCavityWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-height", (char *)NULL, (char *)NULL,
	DEF_WIDGET_HEIGHT, Blt_Offset(EmbeddedWidget, reqHeight),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-justify", (char *)NULL, (char *)NULL, 
	DEF_WIDGET_JUSTIFY, Blt_Offset(EmbeddedWidget, justify),
	BLT_CONFIG_DONT_SET_DEFAULT, &justifyOption},
    {BLT_CONFIG_PAD, "-padx", (char *)NULL, (char *)NULL,
	DEF_WIDGET_PAD_X, Blt_Offset(EmbeddedWidget, xPad),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PAD, "-pady", (char *)NULL, (char *)NULL,
	DEF_WIDGET_PAD_Y, Blt_Offset(EmbeddedWidget, yPad),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-relcavityheight", (char *)NULL, (char *)NULL,
	DEF_WIDGET_REL_HEIGHT, Blt_Offset(EmbeddedWidget, relCavityHeight),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-relcavitywidth", (char *)NULL, (char *)NULL,
	DEF_WIDGET_REL_WIDTH, Blt_Offset(EmbeddedWidget, relCavityWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-relheight", (char *)NULL, (char *)NULL,
	DEF_WIDGET_REL_HEIGHT, Blt_Offset(EmbeddedWidget, relHeight),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DOUBLE, "-relwidth", (char *)NULL, (char *)NULL,
	DEF_WIDGET_REL_WIDTH, Blt_Offset(EmbeddedWidget, relWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-width", (char *)NULL, (char *)NULL,
	DEF_WIDGET_WIDTH, Blt_Offset(EmbeddedWidget, reqWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL,
	(char *)NULL, 0, 0}
};


/* Forward Declarations */
static Tcl_FreeProc DestroyText;
static Tk_EventProc EmbeddedWidgetEventProc;
static Tcl_IdleProc DisplayText;
static Tcl_CmdDeleteProc TextDeleteCmdProc;

static Tcl_VarTraceProc TextVarProc;
static Blt_Bg_ChangedProc BackgroundChangedProc;
static Tk_LostSelProc TextLostSelection;
static Tk_SelectionProc TextSelectionProc;
static Tk_EventProc TextEventProc;
static Tcl_ObjCmdProc TextWidgetCmd;
static Tcl_ObjCmdProc TextCmd;

typedef int (HTextCmdProc)(HText *htextPtr, Tcl_Interp *interp, 
	int objc, Tcl_Obj *const *objv);


/* end of Forward Declarations */


 /* Custom options */
/*
 *---------------------------------------------------------------------------
 *
 * ObjToJustify --
 *
 * 	Converts the justification string into its numeric
 * 	representation. This configuration option affects how the
 *	embedded widget is positioned with respect to the line on which
 *	it sits.
 *
 *	Valid style strings are:
 *
 *	"top"      Uppermost point of region is top of the line's
 *		   text
 * 	"center"   Center point of region is line's baseline.
 *	"bottom"   Lowermost point of region is bottom of the
 *		   line's text
 *
 * Returns:
 *	A standard TCL result.  If the value was not valid
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToJustify(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Not used. */
    Tcl_Obj *objPtr,		/* Justification string */
    char *widgRec,		/* Structure record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    Justify *justPtr = (Justify *)(widgRec + offset);
    char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'c') && (strncmp(string, "center", length) == 0)) {
	*justPtr = JUSTIFY_CENTER;
    } else if ((c == 't') && (strncmp(string, "top", length) == 0)) {
	*justPtr = JUSTIFY_TOP;
    } else if ((c == 'b') && (strncmp(string, "bottom", length) == 0)) {
	*justPtr = JUSTIFY_BOTTOM;
    } else {
	Tcl_AppendResult(interp, "bad justification argument \"", string,
	    "\": should be \"center\", \"top\", or \"bottom\"", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NameOfJustify --
 *
 *	Returns the justification style string based upon the value.
 *
 * Results:
 *	The static justification style string is returned.
 *
 *---------------------------------------------------------------------------
 */
static const char *
NameOfJustify(Justify justify)
{
    switch (justify) {
    case JUSTIFY_CENTER:
	return "center";
    case JUSTIFY_TOP:
	return "top";
    case JUSTIFY_BOTTOM:
	return "bottom";
    default:
	return "unknown justification value";
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * JustifyToObj --
 *
 *	Returns the justification style string based upon the value.
 *
 * Results:
 *	The justification style string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
JustifyToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Structure record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    Justify justify = *(Justify *)(widgRec + offset);

    return Tcl_NewStringObj(NameOfJustify(justify), -1);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetScreenDistance --
 *
 *	Converts the given string into the screen distance or number
 *	of characters.  The valid formats are
 *
 *	    N	- pixels	Nm - millimeters
 *	    Ni  - inches        Np - pica
 *          Nc  - centimeters   N# - number of characters
 *
 *	where N is a non-negative decimal number.
 *
 * Results:
 *	A standard TCL result.  The screen distance and the number of
 *	characters are returned.  If the string can't be converted,
 *	TCL_ERROR is returned and interp->result will contain an error
 *	message.
 *
 *---------------------------------------------------------------------------
 */
static int
GetScreenDistance(
    Tcl_Interp *interp,
    Tk_Window tkwin,
    Tcl_Obj *objPtr,
    int *sizePtr,
    int *countPtr)
{
    int numPixels, numChars;
    char *endPtr;		/* Pointer to last character scanned */
    double value;
    int rounded;
    char *string;


    string = Tcl_GetString(objPtr);
    value = strtod(string, &endPtr);
    if (endPtr == string) {
	Tcl_AppendResult(interp, "bad screen distance \"", string, "\"",
	    (char *)NULL);
	return TCL_ERROR;
    }
    if (value < 0.0) {
	Tcl_AppendResult(interp, "screen distance \"", string,
	    "\" must be non-negative value", (char *)NULL);
	return TCL_ERROR;
    }
    while (isspace(UCHAR(*endPtr))) {
	if (*endPtr == '\0') {
	    break;
	}
	endPtr++;
    }
    numPixels = numChars = 0;
    rounded = ROUND(value);
    switch (*endPtr) {
    case '\0':			/* Distance in pixels */
	numPixels = rounded;
	break;
    case '#':			/* Number of characters */
	numChars = rounded;
	break;
    default:			/* cm, mm, pica, inches */
	if (Tk_GetPixelsFromObj(interp, tkwin, objPtr, &rounded) != TCL_OK) {
	    return TCL_ERROR;
	}
	numPixels = rounded;
	break;
    }
    *sizePtr = numPixels;
    *countPtr = numChars;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StringToHeight --
 *
 *	Like BLT_CONFIG_PIXELS, but adds an extra check for negative
 *	values.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToHeight(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Window */
    Tcl_Obj *objPtr,		/* Pixel value string */
    char *widgRec,		/* Widget record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    HText *htPtr = (HText *)widgRec;
    int height, numRows;

    if (GetScreenDistance(interp, tkwin, objPtr, &height, &numRows) != TCL_OK) {
	return TCL_ERROR;
    }
    htPtr->numRows = numRows;
    htPtr->reqHeight = height;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * StringToWidth --
 *
 *	Like BLT_CONFIG_PIXELS, but adds an extra check for negative
 *	values.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToWidth(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Interpreter to send results back to */
    Tk_Window tkwin,		/* Window */
    Tcl_Obj *objPtr,		/* Pixel value string */
    char *widgRec,		/* Widget record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    HText *htPtr = (HText *)widgRec;
    int width, numColumns;

    if (GetScreenDistance(interp, tkwin, objPtr, &width, &numColumns)!=TCL_OK) {
	return TCL_ERROR;
    }
    htPtr->numColumns = numColumns;
    htPtr->reqWidth = width;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * WidthHeightToObj --
 *
 *	Returns the string representing the positive pixel size.
 *
 * Results:
 *	The pixel size string is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
WidthHeightToObj(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    Tk_Window tkwin,		/* Not used. */
    char *widgRec,		/* Row/column structure record */
    int offset,			/* Offset to field in structure */
    int flags)	
{
    int pixels = *(int *)(widgRec + offset);

    return Tcl_NewIntObj(pixels);
}

/* General routines */
/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *	Queues a request to redraw the text window at the next idle
 *	point.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets redisplayed.  Right now we don't do selective
 *	redisplays:  the whole window will be redrawn.  This doesn't
 *	seem to hurt performance noticeably, but if it does then this
 *	could be changed.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyRedraw(HText *htPtr)
{
    if ((htPtr->tkwin != NULL) && !(htPtr->flags & REDRAW_PENDING)) {
	htPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayText, htPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ResizeArray --
 *
 *	Reallocates memory to the new size given.  New memory
 *	is also cleared (zeros).
 *
 * Results:
 *	Returns a pointer to the new object or NULL if an error occurred.
 *
 * Side Effects:
 *	Memory is re/allocated.
 *
 *---------------------------------------------------------------------------
 */
static void *
ResizeArray(
    void *array,
    int elemSize,
    int newSize,
    int prevSize)
{
    void *newArray;

    if (newSize == prevSize) {
	return array;
    }
    if (newSize == 0) {		/* Free entire array */
	return NULL;
    }
    newArray = Blt_AssertCalloc(elemSize, newSize);
    if ((prevSize > 0) && (array != NULL)) {
	int size;

	size = MIN(prevSize, newSize) * elemSize;
	if (size > 0) {
	    memcpy(newArray, array, size);
	}
	Blt_Free(array);
    }
    return newArray;
}

/*
 *---------------------------------------------------------------------------
 *
 * LineSearch --
 *
 * 	Performs a binary search for the line of text located at some
 * 	world y-coordinate (not screen y-coordinate). The search is
 * 	inclusive of those lines from low to high.
 *
 * Results:
 *	Returns the array index of the line found at the given
 *	y-coordinate.  If the y-coordinate is outside of the given range
 *	of lines, -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
LineSearch(
    HText *htPtr,		/* HText widget */
    int yCoord,			/* Search y-coordinate  */
    int low, int high)		/* Range of lines to search */
{
    int median;
    Line *linePtr;

    while (low <= high) {
	median = (low + high) >> 1;
	linePtr = htPtr->lineArr + median;
	if (yCoord < linePtr->offset) {
	    high = median - 1;
	} else if (yCoord >= (linePtr->offset + linePtr->height)) {
	    low = median + 1;
	} else {
	    return median;
	}
    }
    return -1;
}

/*
 *---------------------------------------------------------------------------
 *
 * IndexSearch --
 *
 *	Try to find what line contains a given text index. Performs
 *	a binary search for the text line which contains the given index.
 *	The search is inclusive of those lines from low and high.
 *
 * Results:
 *	Returns the line number containing the given index. If the index
 *	is outside the range of lines, -1 is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
IndexSearch(
    HText *htPtr,		/* HText widget */
    int key,			/* Search index */
    int low, int high)		/* Range of lines to search */
{
    int median;
    Line *linePtr;

    while (low <= high) {
	median = (low + high) >> 1;
	linePtr = htPtr->lineArr + median;
	if (key < linePtr->textStart) {
	    high = median - 1;
	} else if (key > linePtr->textEnd) {
	    low = median + 1;
	} else {
	    return median;
	}
    }
    return -1;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetXYPosIndex --
 *
 * 	Converts a string in the form "@x,y", where x and y are
 *	window coordinates, to a text index.
 *
 *	Window coordinates are first translated into world coordinates.
 *	Any coordinate outside of the bounds of the virtual text is
 *	silently set the nearest boundary.
 *
 * Results:
 *	A standard TCL result.  If "string" is a valid index, then
 *	*indexPtr is filled with the numeric index corresponding.
 *	Otherwise an error message is left in interp->result.
 *
 *---------------------------------------------------------------------------
 */
static int
GetXYPosIndex(
    HText *htPtr,
    Tcl_Obj *objPtr,
    int *indexPtr)
{
    int x, y, curX, dummy;
    int textLength, textStart;
    int cindex, lindex;
    Line *linePtr;
    char *string;

    string = Tcl_GetString(objPtr);
    if (Blt_GetXY(htPtr->interp, htPtr->tkwin, string, &x, &y) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Locate the line corresponding to the window y-coordinate position */

    y += htPtr->yOffset;
    if (y < 0) {
	lindex = htPtr->first;
    } else if (y >= htPtr->worldHeight) {
	lindex = htPtr->last;
    } else {
	lindex = LineSearch(htPtr, y, 0, htPtr->numLines - 1);
    }
    if (lindex < 0) {
	Tcl_AppendResult(htPtr->interp, "can't find line at \"", string, "\"",
	    (char *)NULL);
	return TCL_ERROR;
    }
    x += htPtr->xOffset;
    if (x < 0) {
	x = 0;
    } else if (x > htPtr->worldWidth) {
	x = htPtr->worldWidth;
    }
    linePtr = htPtr->lineArr + lindex;
    curX = 0;
    textStart = linePtr->textStart;
    textLength = linePtr->textEnd - linePtr->textStart;
    if (Blt_Chain_GetLength(linePtr->chain) > 0) {
	Blt_ChainLink link;
	int deltaX;
	EmbeddedWidget *winPtr;

	for (link = Blt_Chain_FirstLink(linePtr->chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    winPtr = Blt_Chain_GetValue(link);
	    deltaX = winPtr->precedingTextWidth + winPtr->cavityWidth;
	    if ((curX + deltaX) > x) {
		textLength = (winPtr->precedingTextEnd - textStart);
		break;
	    }
	    curX += deltaX;
	    /*
	     * Skip over the trailing space. It designates the position of
	     * a embedded widget in the text
	     */
	    textStart = winPtr->precedingTextEnd + 1;
	}
    }
    cindex = Blt_Font_Measure(htPtr->font, htPtr->charArr + textStart,
	textLength, 10000, DEF_TEXT_FLAGS, &dummy);
    *indexPtr = textStart + cindex;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseIndex --
 *
 *	Parse a string representing a text index into numeric
 *	value.  A text index can be in one of the following forms.
 *
 *	  "anchor"	- anchor position of the selection.
 *	  "sel.first"   - index of the first character in the selection.
 *	  "sel.last"	- index of the last character in the selection.
 *	  "page.top"  	- index of the first character on the page.
 *	  "page.bottom"	- index of the last character on the page.
 *	  "@x,y"	- x and y are window coordinates.
 * 	  "number	- raw index of text
 *	  "line.char"	- line number and character position
 *
 * Results:
 *	A standard TCL result.  If "string" is a valid index, then
 *	*indexPtr is filled with the corresponding numeric index.
 *	Otherwise an error message is left in interp->result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
ParseIndex(
    HText *htPtr,		/* Text for which the index is being
				 * specified. */
    Tcl_Obj *objPtr,		/* Numerical index into htPtr's element
				 * list, or "end" to refer to last element. */
    int *indexPtr)		/* Where to store converted relief. */
{
    Tcl_Interp *interp = htPtr->interp;
    char *string;
    char c;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    c = string[0];
    if ((c == 'a') && (strncmp(string, "anchor", length) == 0)) {
	*indexPtr = htPtr->selAnchor;
    } else if ((c == 's') && (length > 4)) {
	if (strncmp(string, "sel.first", length) == 0) {
	    *indexPtr = htPtr->selFirst;
	} else if (strncmp(string, "sel.last", length) == 0) {
	    *indexPtr = htPtr->selLast;
	} else {
	    goto badIndex;	/* Not a valid index */
	}
	if (*indexPtr < 0) {
	    Tcl_AppendResult(interp, "bad index \"", string,
		"\": nothing selected in \"",
		Tk_PathName(htPtr->tkwin), "\"", (char *)NULL);
	    return TCL_ERROR;
	}
    } else if ((c == 'p') && (length > 5) &&
	(strncmp(string, "page.top", length) == 0)) {
	int first;

	first = htPtr->first;
	if (first < 0) {
	    first = 0;
	}
	*indexPtr = htPtr->lineArr[first].textStart;
    } else if ((c == 'p') && (length > 5) &&
	(strncmp(string, "page.bottom", length) == 0)) {
	*indexPtr = htPtr->lineArr[htPtr->last].textEnd;
    } else if (c == '@') {	/* Screen position */
	if (GetXYPosIndex(htPtr, objPtr, indexPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	char *period;

	period = strchr(string, '.');
	if (period == NULL) {	/* Raw index */
	    int tindex;

	    if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
		tindex = htPtr->numChars - 1;
	    } else if (Tcl_GetIntFromObj(interp, objPtr, &tindex) != TCL_OK) {
		goto badIndex;
	    }
	    if (tindex < 0) {
		tindex = 0;
	    } else if (tindex > (htPtr->numChars - 1)) {
		tindex = htPtr->numChars - 1;
	    }
	    *indexPtr = tindex;
	} else {
	    int lindex, cindex, offset;
	    Line *linePtr;
	    int result;

	    *period = '\0';
	    result = TCL_OK;
	    if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
		lindex = htPtr->numLines - 1;
	    } else {
		result = Tcl_GetIntFromObj(interp, objPtr, &lindex);
	    }
	    *period = '.';	/* Repair index string before returning */
	    if (result != TCL_OK) {
		goto badIndex;	/* Bad line number */
	    }
	    if (lindex < 0) {
		lindex = 0;	/* Silently repair bad line numbers */
	    }
	    if (htPtr->numChars == 0) {
		*indexPtr = 0;
		return TCL_OK;
	    }
	    if (lindex >= htPtr->numLines) {
		lindex = htPtr->numLines - 1;
	    }
	    linePtr = htPtr->lineArr + lindex;
	    cindex = 0;
	    if ((*(period + 1) != '\0')) {
		string = period + 1;
		if ((string[0] == 'e') && (strcmp(string, "end") == 0)) {
		    cindex = linePtr->textEnd - linePtr->textStart;
		} else if (Tcl_GetInt(interp, string, &cindex) != TCL_OK) {
		    goto badIndex;
		}
	    }
	    if (cindex < 0) {
		cindex = 0;	/* Silently fix bogus indices */
	    }
	    offset = 0;
	    if (htPtr->numChars > 0) {
		offset = linePtr->textStart + cindex;
		if (offset > linePtr->textEnd) {
		    offset = linePtr->textEnd;
		}
	    }
	    *indexPtr = offset;
	}
    }
    if (htPtr->numChars == 0) {
	*indexPtr = 0;
    }
    return TCL_OK;

  badIndex:

    /*
     * Some of the paths here leave messages in interp->result, so we
     * have to clear it out before storing our own message.
     */
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "bad index \"", string, "\": \
should be one of the following: anchor, sel.first, sel.last, page.bottom, \
page.top, @x,y, index, line.char", (char *)NULL);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetIndex --
 *
 *	Get the index from a string representing a text index.
 *
 *
 * Results:
 *	A standard TCL result.  If "string" is a valid index, then
 *	*indexPtr is filled with the numeric index corresponding.
 *	Otherwise an error message is left in interp->result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
GetIndex(
    HText *htPtr,		/* Text for which the index is being
				 * specified. */
    Tcl_Obj *objPtr,		/* Numerical index into htPtr's element
				 * list, or "end" to refer to last element. */
    int *indexPtr)		/* Where to store converted relief. */
{
    int tindex;

    if (ParseIndex(htPtr, objPtr, &tindex) != TCL_OK) {
	return TCL_ERROR;
    }
    *indexPtr = tindex;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTextPosition --
 *
 * 	Performs a binary search for the index located on line in
 *	the text. The search is limited to those lines between
 *	low and high inclusive.
 *
 * Results:
 *	Returns the line number at the given Y coordinate. If position
 *	does not correspond to any of the lines in the given the set,
 *	-1 is returned.
 *
 *---------------------------------------------------------------------------
 */
static int
GetTextPosition(
    HText *htPtr,
    int tindex,
    int *lindexPtr,
    int *cindexPtr)
{
    int lindex, cindex;

    lindex = cindex = 0;
    if (htPtr->numChars > 0) {
	Line *linePtr;

	lindex = IndexSearch(htPtr, tindex, 0, htPtr->numLines - 1);
	if (lindex < 0) {
	    char string[200];

	    Blt_FormatString(string, 200, 
		"can't determine line number from index \"%d\"", tindex);
	    Tcl_AppendResult(htPtr->interp, string, (char *)NULL);
	    return TCL_ERROR;
	}
	linePtr = htPtr->lineArr + lindex;
	if (tindex > linePtr->textEnd) {
	    tindex = linePtr->textEnd;
	}
	cindex = tindex - linePtr->textStart;
    }
    *lindexPtr = lindex;
    *cindexPtr = cindex;
    return TCL_OK;
}

/* EmbeddedWidget Procedures */
/*
 *---------------------------------------------------------------------------
 *
 * GetEmbeddedWidgetWidth --
 *
 *	Returns the width requested by the embedded widget. The requested
 *	space also includes any internal padding which has been designated
 *	for this window.
 *
 * Results:
 *	Returns the requested width of the embedded widget.
 *
 *---------------------------------------------------------------------------
 */
static int
GetEmbeddedWidgetWidth(EmbeddedWidget *winPtr)
{
    int width;

    if (winPtr->reqWidth > 0) {
	width = winPtr->reqWidth;
    } else if (winPtr->relWidth > 0.0) {
	width = (int)
	    ((double)Tk_Width(winPtr->htPtr->tkwin) * winPtr->relWidth + 0.5);
    } else {
	width = Tk_ReqWidth(winPtr->tkwin);
    }
    width += (2 * winPtr->ixPad);
    return width;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetEmbeddedWidgetHeight --
 *
 *	Returns the height requested by the embedded widget. The requested
 *	space also includes any internal padding which has been designated
 *	for this window.
 *
 * Results:
 *	Returns the requested height of the embedded widget.
 *
 *---------------------------------------------------------------------------
 */
static int
GetEmbeddedWidgetHeight(EmbeddedWidget *winPtr)
{
    int height;

    if (winPtr->reqHeight > 0) {
	height = winPtr->reqHeight;
    } else if (winPtr->relHeight > 0.0) {
	height = (int)((double)Tk_Height(winPtr->htPtr->tkwin) *
	    winPtr->relHeight + 0.5);
    } else {
	height = Tk_ReqHeight(winPtr->tkwin);
    }
    height += (2 * winPtr->iyPad);
    return height;
}

/*
 *---------------------------------------------------------------------------
 *
 * EmbeddedWidgetEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various
 * 	events on hypertext widgets.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
EmbeddedWidgetEventProc(
    ClientData clientData,	/* Information about the embedded widget. */
    XEvent *eventPtr)		/* Information about event. */
{
    EmbeddedWidget *winPtr = clientData;
    HText *htPtr;

    if ((winPtr == NULL) || (winPtr->tkwin == NULL)) {
	return;
    }
    htPtr = winPtr->htPtr;

    if (eventPtr->type == DestroyNotify) {
	Blt_HashEntry *hPtr;
	/*
	 * Mark the widget as deleted by dereferencing the Tk window
	 * pointer.  Zero out the height and width to collapse the area
	 * used by the widget.  Redraw the window only if the widget is
	 * currently visible.
	 */
	winPtr->htPtr->flags |= REQUEST_LAYOUT;
	if (Tk_IsMapped(winPtr->tkwin) && (winPtr->flags & WIDGET_VISIBLE)) {
	    EventuallyRedraw(htPtr);
	}
	Tk_DeleteEventHandler(winPtr->tkwin, StructureNotifyMask,
	    EmbeddedWidgetEventProc, winPtr);
	hPtr = Blt_FindHashEntry(&htPtr->widgetTable, (char *)winPtr->tkwin);
	Blt_DeleteHashEntry(&htPtr->widgetTable, hPtr);
	winPtr->cavityWidth = winPtr->cavityHeight = 0;
	winPtr->tkwin = NULL;

    } else if (eventPtr->type == ConfigureNotify) {
	/*
	 * EmbeddedWidgets can't request new positions. Worry only about resizing.
	 */
	if (winPtr->winWidth != Tk_Width(winPtr->tkwin) ||
	    winPtr->winHeight != Tk_Height(winPtr->tkwin)) {
	    EventuallyRedraw(htPtr);
	    htPtr->flags |= REQUEST_LAYOUT;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EmbeddedWidgetCustodyProc --
 *
 *	This procedure is invoked when a embedded widget has been
 *	stolen by another geometry manager.  The information and
 *	memory associated with the embedded widget is released.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the widget formerly associated with the widget
 *	to have its layout re-computed and arranged at the
 *	next idle point.
 *
 *---------------------------------------------------------------------------
 */
 /* ARGSUSED */
static void
EmbeddedWidgetCustodyProc(
    ClientData clientData,	/* Information about the former
				 * embedded widget. */
    Tk_Window tkwin)		/* Not used. */
{
    Blt_HashEntry *hPtr;
    EmbeddedWidget *winPtr = clientData;
    /*
     * Mark the widget as deleted by dereferencing the Tk window
     * pointer.  Zero out the height and width to collapse the area
     * used by the widget.  Redraw the window only if the widget is
     * currently visible.
     */
    winPtr->htPtr->flags |= REQUEST_LAYOUT;
    if (Tk_IsMapped(winPtr->tkwin) && (winPtr->flags & WIDGET_VISIBLE)) {
	EventuallyRedraw(winPtr->htPtr);
    }
    Tk_DeleteEventHandler(winPtr->tkwin, StructureNotifyMask,
	EmbeddedWidgetEventProc, winPtr);
    hPtr = Blt_FindHashEntry(&winPtr->htPtr->widgetTable, 
			     (char *)winPtr->tkwin);
    Blt_DeleteHashEntry(&winPtr->htPtr->widgetTable, hPtr);
    winPtr->cavityWidth = winPtr->cavityHeight = 0;
    winPtr->tkwin = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * EmbeddedWidgetGeometryProc --
 *
 *	This procedure is invoked by Tk_GeometryRequest for
 *	embedded widgets managed by the hypertext widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for tkwin, and all its managed siblings, to
 *	be repacked and drawn at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
 /* ARGSUSED */
static void
EmbeddedWidgetGeometryProc(
    ClientData clientData,	/* Information about window that got new
			         * preferred geometry.  */
    Tk_Window tkwin)		/* Not used. */
{
    EmbeddedWidget *winPtr = clientData;

    winPtr->htPtr->flags |= REQUEST_LAYOUT;
    EventuallyRedraw(winPtr->htPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FindEmbeddedWidget --
 *
 *	Searches for a widget matching the path name given
 *	If found, the pointer to the widget structure is returned,
 *	otherwise NULL.
 *
 * Results:
 *	The pointer to the widget structure. If not found, NULL.
 *
 *---------------------------------------------------------------------------
 */
static EmbeddedWidget *
FindEmbeddedWidget(
    HText *htPtr,		/* Hypertext widget structure */
    Tk_Window tkwin)		/* Path name of embedded widget  */
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&htPtr->widgetTable, (char *)tkwin);
    if (hPtr != NULL) {
	return Blt_GetHashValue(hPtr);
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateEmbeddedWidget --
 *
 * 	This procedure creates and initializes a new embedded widget
 *	in the hyper text widget.
 *
 * Results:
 *	The return value is a pointer to a structure describing the
 *	new embedded widget.  If an error occurred, then the return 
 *	value is NULL and an error message is left in interp->result.
 *
 * Side effects:
 *	Memory is allocated. EmbeddedWidget window is mapped. 
 *	Callbacks are set up for embedded widget resizes and geometry 
 *	requests.
 *
 *---------------------------------------------------------------------------
 */
static EmbeddedWidget *
CreateEmbeddedWidget(
    HText *htPtr,		/* Hypertext widget */
    char *name)			/* Name of embedded widget */
{
    EmbeddedWidget *winPtr;
    Tk_Window tkwin;
    Blt_HashEntry *hPtr;
    int isNew;

    tkwin = Tk_NameToWindow(htPtr->interp, name, htPtr->tkwin);
    if (tkwin == NULL) {
	return NULL;
    }
    if (Tk_Parent(tkwin) != htPtr->tkwin) {
	Tcl_AppendResult(htPtr->interp, "parent window of \"", name,
	    "\" must be \"", Tk_PathName(htPtr->tkwin), "\"", (char *)NULL);
	return NULL;
    }
    hPtr = Blt_CreateHashEntry(&htPtr->widgetTable, (char *)tkwin, &isNew);
    /* Check is the widget is already embedded into this widget */
    if (!isNew) {
	Tcl_AppendResult(htPtr->interp, "\"", name,
	    "\" is already appended to ", Tk_PathName(htPtr->tkwin),
	    (char *)NULL);
	return NULL;
    }
    winPtr = Blt_AssertCalloc(1, sizeof(EmbeddedWidget));
    winPtr->flags = 0;
    winPtr->tkwin = tkwin;
    winPtr->htPtr = htPtr;
    winPtr->x = winPtr->y = 0;
    winPtr->fill = FILL_NONE;
    winPtr->justify = JUSTIFY_CENTER;
    winPtr->anchor = TK_ANCHOR_CENTER;
    Blt_SetHashValue(hPtr, winPtr);

    Tk_ManageGeometry(tkwin, &htextMgrInfo, winPtr);
    Tk_CreateEventHandler(tkwin, StructureNotifyMask, EmbeddedWidgetEventProc,
	  winPtr);
    return winPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyEmbeddedWidget --
 *
 * 	This procedure is invoked by DestroyLine to clean up the
 * 	internal structure of a widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the widget is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyEmbeddedWidget(EmbeddedWidget *winPtr)
{
    /* Destroy the embedded widget if it still exists */
    if (winPtr->tkwin != NULL) {
	Blt_HashEntry *hPtr;

	Tk_DeleteEventHandler(winPtr->tkwin, StructureNotifyMask,
	    EmbeddedWidgetEventProc, winPtr);
	hPtr = Blt_FindHashEntry(&winPtr->htPtr->widgetTable,
	    (char *)winPtr->tkwin);
	Blt_DeleteHashEntry(&winPtr->htPtr->widgetTable, hPtr);
	Tk_DestroyWindow(winPtr->tkwin);
    }
    Blt_Free(winPtr);
}

/* Line Procedures */
/*
 *---------------------------------------------------------------------------
 *
 * CreateLine --
 *
 * 	This procedure creates and initializes a new line of text.
 *
 * Results:
 *	The return value is a pointer to a structure describing the new
 * 	line of text.  If an error occurred, then the return value is NULL
 *	and an error message is left in interp->result.
 *
 * Side effects:
 *	Memory is allocated.
 *
 *---------------------------------------------------------------------------
 */
static Line *
CreateLine(HText *htPtr)
{
    Line *linePtr;

    if (htPtr->numLines >= htPtr->arraySize) {
	if (htPtr->arraySize == 0) {
	    htPtr->arraySize = DEF_LINES_ALLOC;
	} else {
	    htPtr->arraySize += htPtr->arraySize;
	}
	htPtr->lineArr = ResizeArray(htPtr->lineArr, sizeof(Line), 
		htPtr->arraySize, htPtr->numLines);
    }
    /* Initialize values in the new entry */

    linePtr = htPtr->lineArr + htPtr->numLines;
    linePtr->offset = 0;
    linePtr->height = linePtr->width = 0;
    linePtr->textStart = 0;
    linePtr->textEnd = -1;
    linePtr->baseline = 0;
    linePtr->chain = Blt_Chain_Create();

    htPtr->numLines++;
    return linePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyLine --
 *
 * 	This procedure is invoked to clean up the internal structure
 *	of a line.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the line (text and widgets) is
 *	freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyLine(Line *linePtr)
{
    Blt_ChainLink link;
    EmbeddedWidget *winPtr;

    /* Free the list of embedded widget structures */
    for (link = Blt_Chain_FirstLink(linePtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	winPtr = Blt_Chain_GetValue(link);
	DestroyEmbeddedWidget(winPtr);
    }
    Blt_Chain_Destroy(linePtr->chain);
}

static void
FreeText(HText *htPtr)
{
    int i;

    for (i = 0; i < htPtr->numLines; i++) {
	DestroyLine(htPtr->lineArr + i);
    }
    htPtr->numLines = 0;
    htPtr->numChars = 0;
    if (htPtr->charArr != NULL) {
	Blt_Free(htPtr->charArr);
	htPtr->charArr = NULL;
    }
}

/* Text Procedures */
/*
 *---------------------------------------------------------------------------
 *
 * DestroyText --
 *
 * 	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a HText at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the widget is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyText(DestroyData dataPtr) /* Info about hypertext widget. */
{
    HText *htPtr = (HText *)dataPtr;

    Blt_FreeOptions(configSpecs, (char *)htPtr, htPtr->display, 0);
    if (htPtr->drawGC != NULL) {
	Tk_FreeGC(htPtr->display, htPtr->drawGC);
    }
    if (htPtr->fillGC != NULL) {
	Tk_FreeGC(htPtr->display, htPtr->fillGC);
    }
    if (htPtr->selectGC != NULL) {
	Tk_FreeGC(htPtr->display, htPtr->selectGC);
    }
    FreeText(htPtr);
    if (htPtr->lineArr != NULL) {
	Blt_Free(htPtr->lineArr);
    }
    Blt_DeleteHashTable(&htPtr->widgetTable);
    Blt_Free(htPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TextEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various
 * 	events on hypertext widgets.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
TextEventProc(
    ClientData clientData,	/* Information about window. */
    XEvent *eventPtr)		/* Information about event. */
{
    HText *htPtr = clientData;

    if (eventPtr->type == ConfigureNotify) {
	if ((htPtr->lastWidth != Tk_Width(htPtr->tkwin)) ||
	    (htPtr->lastHeight != Tk_Height(htPtr->tkwin))) {
	    htPtr->flags |= (REQUEST_LAYOUT | TEXT_DIRTY);
	    EventuallyRedraw(htPtr);
	}
    } else if (eventPtr->type == Expose) {

	/*
	 * If the Expose event was synthetic (i.e. we manufactured it
	 * ourselves during a redraw operation), toggle the bit flag
	 * which controls redraws.
	 */

	if (eventPtr->xexpose.send_event) {
	    htPtr->flags ^= IGNORE_EXPOSURES;
	    return;
	}
	if ((eventPtr->xexpose.count == 0) &&
	    !(htPtr->flags & IGNORE_EXPOSURES)) {
	    htPtr->flags |= TEXT_DIRTY;
	    EventuallyRedraw(htPtr);
	}
    } else if (eventPtr->type == DestroyNotify) {
	if (htPtr->tkwin != NULL) {
	    htPtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(htPtr->interp, htPtr->cmdToken);
	}
	if (htPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayText, htPtr);
	}
	Tcl_EventuallyFree(htPtr, DestroyText);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TextDeleteCmdProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
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
TextDeleteCmdProc(ClientData clientData) /* Pointer to widget record. */
{
    HText *htPtr = clientData;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (htPtr->tkwin != NULL) {
	Tk_Window tkwin;

	tkwin = htPtr->tkwin;
	htPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * BackgroundChangedProc
 *
 *	Stub for image change notifications.  Since we immediately draw
 *	the image into a pixmap, we don't care about image changes.
 *
 *	It would be better if Tk checked for NULL proc pointers.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
BackgroundChangedProc(ClientData clientData) 
{
    HText *htPtr = clientData;

    if (htPtr->tkwin != NULL) {
	EventuallyRedraw(htPtr);
    }
}

/* Configuration Procedures */
static void
ResetTextInfo(HText *htPtr)
{
    htPtr->first = 0;
    htPtr->last = htPtr->numLines - 1;
    htPtr->selFirst = htPtr->selLast = -1;
    htPtr->selAnchor = 0;
    htPtr->pendingX = htPtr->pendingY = 0;
    htPtr->worldWidth = htPtr->worldHeight = 0;
    htPtr->xOffset = htPtr->yOffset = 0;
}

static Line *
GetLastLine(HText *htPtr)
{
    if (htPtr->numLines == 0) {
	return CreateLine(htPtr);
    }
    return (htPtr->lineArr + (htPtr->numLines - 1));
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadNamedFile --
 *
 * 	Read the named file into a newly allocated buffer.
 *
 * Results:
 *	Returns the size of the allocated buffer if the file was
 *	read correctly.  Otherwise -1 is returned and "interp->result"
 *	will contain an error message.
 *
 * Side Effects:
 *	If successful, the contents of "bufferPtr" will point
 *	to the allocated buffer.
 *
 *---------------------------------------------------------------------------
 */
static int
ReadNamedFile(Tcl_Interp *interp, char *fileName, char **bufferPtr)
{
    FILE *f;
    int numRead;
    size_t fileSize;
    int count, bytesLeft;
    char *buffer;
    struct stat fileInfo;

    f = Blt_OpenFile(interp, fileName, "r");
    if (f == NULL) {
	return -1;
    }
    if (fstat(fileno(f), &fileInfo) < 0) {
	Tcl_AppendResult(interp, "can't stat \"", fileName, "\": ",
	    Tcl_PosixError(interp), (char *)NULL);
	fclose(f);
	return -1;
    }
    fileSize = fileInfo.st_size + 1;
    buffer = Blt_Malloc(sizeof(char) * fileSize);
    if (buffer == NULL) {
	fclose(f);
	return -1;		/* Can't allocate memory for file buffer */
    }
    count = 0;
    for (bytesLeft = fileInfo.st_size; bytesLeft > 0; bytesLeft -= numRead) {
	numRead = fread(buffer + count, sizeof(char), bytesLeft, f);
	if (numRead < 0) {
	    Tcl_AppendResult(interp, "error reading \"", fileName, "\": ",
		Tcl_PosixError(interp), (char *)NULL);
	    fclose(f);
	    Blt_Free(buffer);
	    return -1;
	} else if (numRead == 0) {
	    break;
	}
	count += numRead;
    }
    fclose(f);
    buffer[count] = '\0';
    *bufferPtr = buffer;
    return count;
}

/*
 *---------------------------------------------------------------------------
 *
 * CollectCommand --
 *
 * 	Collect the characters representing a TCL command into a
 *	given buffer.
 *
 * Results:
 *	Returns the number of bytes examined.  If an error occurred,
 *	-1 is returned and "interp->result" will contain an error
 *	message.
 *
 * Side Effects:
 *	If successful, the "cmdArr" will be filled with the string
 *	representing the TCL command.
 *
 *---------------------------------------------------------------------------
 */

static int
CollectCommand(
    HText *htPtr,		/* Widget record */
    char inputArr[],		/* Array of bytes representing the
				 * htext input */
    int maxBytes,		/* Maximum number of bytes left in input */
    char cmdArr[])		/* Output buffer to be filled with the Tcl
				 * command */
{
    int c;
    int i;
    int state, count;

    /* Simply collect the all the characters until %% into a buffer */

    state = count = 0;
    for (i = 0; i < maxBytes; i++) {
	c = inputArr[i];
	if (c == htPtr->specChar) {
	    state++;
	} else if ((state == 0) && (c == '\\')) {
	    state = 3;
	} else {
	    state = 0;
	}
	switch (state) {
	case 2:		/* End of command block found */
	    cmdArr[count - 1] = '\0';
	    return i;

	case 4:		/* Escaped block designator */
	    cmdArr[count] = c;
	    state = 0;
	    break;

	default:		/* Add to command buffer */
	    cmdArr[count++] = c;
	    break;
	}
    }
    Tcl_AppendResult(htPtr->interp, "premature end of TCL command block",
	(char *)NULL);
    return -1;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParseInput --
 *
 * 	Parse the input to the HText structure into an array of lines.
 *	Each entry contains the beginning index and end index of the
 *	characters in the text array which comprise the line.
 *
 *	|*|*|*|\n|T|h|i|s| |a| |l|i|n|e| |o|f| |t|e|x|t|.|\n|*|*|*|
 *                ^					  ^
 *	          textStart				  textEnd
 *
 *	Note that the end index contains the '\n'.
 *
 * Results:
 *	Returns TCL_OK or error depending if the file was read correctly.
 *
 *---------------------------------------------------------------------------
 */
static int
ParseInput(
    Tcl_Interp *interp,
    HText *htPtr,
    char input[],
    int numBytes)
{
    int c;
    int i;
    char *textArr;
    char *cmdArr;
    int count, numLines;
    int length;
    int state;
    Line *linePtr;

    linePtr = CreateLine(htPtr);
    if (linePtr == NULL) {
	return TCL_ERROR;	/* Error allocating the line structure */
    }
    /*  Right now, we replace the text array instead of appending to it */

    linePtr->textStart = 0;

    /* In the worst case, assume the entire input could be TCL commands */
    cmdArr  = Blt_AssertMalloc(sizeof(char) * (numBytes + 1));
    textArr = Blt_AssertMalloc(sizeof(char) * (numBytes + 1));
    if (htPtr->charArr != NULL) {
	Blt_Free(htPtr->charArr);
    }
    htPtr->charArr = textArr;
    htPtr->numChars = 0;

    numLines = count = state = 0;
    htPtr->flags &= ~WIDGET_APPENDED;

    for (i = 0; i < numBytes; i++) {
	c = input[i];
	if (c == htPtr->specChar) {
	    state++;
	} else if (c == '\n') {
	    state = -1;
	} else if ((state == 0) && (c == '\\')) {
	    state = 3;
	} else {
	    state = 0;
	}
	switch (state) {
	case 2:		/* Block of TCL commands found */
	    count--, i++;
	    length = CollectCommand(htPtr, input + i, numBytes - i, cmdArr);
	    if (length < 0) {
		goto error;
	    }
	    i += length;
	    linePtr->textEnd = count;
	    htPtr->numChars = count + 1;
	    if (Tcl_Eval(interp, cmdArr) != TCL_OK) {
		goto error;
	    }
	    if (htPtr->flags & WIDGET_APPENDED) {
		/* Indicates the location a embedded widget in the text array */
		textArr[count++] = ' ';
		htPtr->flags &= ~WIDGET_APPENDED;
	    }
	    state = 0;
	    break;

	case 4:		/* Escaped block designator */
	    textArr[count - 1] = c;
	    state = 0;
	    break;

	case -1:		/* End of line or input */
	    linePtr->textEnd = count;
	    textArr[count++] = '\n';
	    numLines++;
	    linePtr = CreateLine(htPtr);
	    if (linePtr == NULL) {
		goto error;
	    }
	    linePtr->textStart = count;
	    state = 0;
	    break;

	default:		/* Default action, add to text buffer */
	    textArr[count++] = c;
	    break;
	}
    }
    if (count > linePtr->textStart) {
	linePtr->textEnd = count;
	textArr[count++] = '\n';/* Every line must end with a '\n' */
	numLines++;
    }
    Blt_Free(cmdArr);
    /* Reset number of lines allocated */
    htPtr->lineArr = ResizeArray(htPtr->lineArr, sizeof(Line), numLines, 
	htPtr->arraySize);
    htPtr->numLines = htPtr->arraySize = numLines;
    /*  and the size of the character array */
    htPtr->charArr = ResizeArray(htPtr->charArr, sizeof(char), count, numBytes);
    htPtr->numChars = count;
    return TCL_OK;
  error:
    Blt_Free(cmdArr);
    return TCL_ERROR;
}

static int
IncludeText(
    Tcl_Interp *interp,
    HText *htPtr,
    char *fileName)
{
    char *buffer;
    int result;
    int numBytes;

    if ((htPtr->text == NULL) && (fileName == NULL)) {
	return TCL_OK;		/* Empty text string */
    }
    if (fileName != NULL) {
	numBytes = ReadNamedFile(interp, fileName, &buffer);
	if (numBytes < 0) {
	    return TCL_ERROR;
	}
    } else {
	buffer = htPtr->text;
	numBytes = strlen(htPtr->text);
    }
    result = ParseInput(interp, htPtr, buffer, numBytes);
    if (fileName != NULL) {
	Blt_Free(buffer);
    }
    return result;
}

/* ARGSUSED */
static char *
TextVarProc(
    ClientData clientData,	/* Information about widget. */
    Tcl_Interp *interp,		/* Interpreter containing variable. */
    const char *name1,		/* Name of variable. */
    const char *name2,		/* Second part of variable name. */
    int flags)			/* Information about what happened. */
{
    HText *htPtr = clientData;
    HText *lasthtPtr;

    /* Check to see of this is the most recent trace */
    lasthtPtr = (HText *)Tcl_VarTraceInfo2(interp, name1, name2, flags,
	TextVarProc, NULL);
    if (lasthtPtr != htPtr) {
	return NULL;		/* Ignore all but most current trace */
    }
    if (flags & TCL_TRACE_READS) {
	char c;

	c = name2[0];
	if ((c == 'w') && (strcmp(name2, "widget") == 0)) {
	    Tcl_SetVar2(interp, name1, name2, Tk_PathName(htPtr->tkwin),
		flags);
	} else if ((c == 'l') && (strcmp(name2, "line") == 0)) {
	    char buf[200];
	    int lineNum;

	    lineNum = htPtr->numLines - 1;
	    if (lineNum < 0) {
		lineNum = 0;
	    }
	    Blt_FormatString(buf, 200, "%d", lineNum);
	    Tcl_SetVar2(interp, name1, name2, buf, flags);
	} else if ((c == 'i') && (strcmp(name2, "index") == 0)) {
	    char buf[200];

	    Blt_FormatString(buf, 200, "%d", htPtr->numChars - 1);
	    Tcl_SetVar2(interp, name1, name2, buf, flags);
	} else if ((c == 'f') && (strcmp(name2, "file") == 0)) {
	    const char *fileName;

	    fileName = htPtr->fileName;
	    if (fileName == NULL) {
		fileName = "";
	    }
	    Tcl_SetVar2(interp, name1, name2, fileName, flags);
	} else {
	    return (char *)"?unknown?";
	}
    }
    return NULL;
}

static const char *varNames[] = {
    "widget", "line", "file", "index", (char *)NULL
};

static void
CreateTraces(HText *htPtr)
{
    const char **p;
    static char globalCmd[] = "global htext";

    /*
     * Make the traced variables global to the widget
     */
    Tcl_Eval(htPtr->interp, globalCmd);
    for (p = varNames; *p != NULL; p++) {
	Tcl_TraceVar2(htPtr->interp, "htext", *p,
	    (TCL_GLOBAL_ONLY | TCL_TRACE_READS), TextVarProc, htPtr);
    }
}

static void
DeleteTraces(HText *htPtr)
{
    const char **p;

    for (p = varNames; *p != NULL; p++) {
	Tcl_UntraceVar2(htPtr->interp, "htext", *p,
	    (TCL_GLOBAL_ONLY | TCL_TRACE_READS), TextVarProc, htPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureText --
 *
 * 	This procedure is called to process an objv/objc list, plus
 *	the Tk option database, in order to configure (or reconfigure)
 *	a hypertext widget.
 *
 * 	The layout of the text must be calculated (by ComputeLayout)
 *	whenever particular options change; -font, -file, -linespacing
 *	and -text options. If the user has changes one of these options,
 *	it must be detected so that the layout can be recomputed. Since the
 *	coordinates of the layout are virtual, there is no need to adjust
 *	them if physical window attributes (window size, etc.)
 *	change.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is
 * 	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 * 	etc. get set for htPtr;  old resources get freed, if there were any.
 * 	The hypertext is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureText(
    Tcl_Interp *interp,		/* Used for error reporting. */
    HText *htPtr)		/* Information about widget; may or may not
			         * already have values for some fields. */
{
    XGCValues gcValues;
    unsigned long gcMask;
    GC newGC;

    if (Blt_ConfigModified(configSpecs, "-font", "-linespacing", "-file",
	    "-text", "-width", "-height", (char *)NULL)) {
	/*
	 * These options change the layout of the text.  Width/height
	 * and rows/columns may change a relatively sized window or cavity.
	 */
	htPtr->flags |= (REQUEST_LAYOUT | TEXT_DIRTY);	/* Mark for update */
    }
    gcMask = GCForeground | GCFont;
    gcValues.font = Blt_Font_Id(htPtr->font);
    gcValues.foreground = htPtr->normalFg->pixel;
    newGC = Tk_GetGC(htPtr->tkwin, gcMask, &gcValues);
    if (htPtr->drawGC != NULL) {
	Tk_FreeGC(htPtr->display, htPtr->drawGC);
    }
    htPtr->drawGC = newGC;

    gcValues.foreground = htPtr->selFgColor->pixel;
    newGC = Tk_GetGC(htPtr->tkwin, gcMask, &gcValues);
    if (htPtr->selectGC != NULL) {
	Tk_FreeGC(htPtr->display, htPtr->selectGC);
    }
    htPtr->selectGC = newGC;

    if (htPtr->xScrollUnits < 1) {
	htPtr->xScrollUnits = 1;
    }
    if (htPtr->yScrollUnits < 1) {
	htPtr->yScrollUnits = 1;
    }
    if (htPtr->normalBg != NULL) {
	Blt_Bg_SetChangedProc(htPtr->normalBg, BackgroundChangedProc, 
		htPtr);
    }
    if (htPtr->selBg != NULL) {
	Blt_Bg_SetChangedProc(htPtr->selBg, BackgroundChangedProc, 
		htPtr);
    }
    gcValues.foreground = Blt_Bg_BorderColor(htPtr->normalBg)->pixel;
    newGC = Tk_GetGC(htPtr->tkwin, gcMask, &gcValues);
    if (htPtr->fillGC != NULL) {
	Tk_FreeGC(htPtr->display, htPtr->fillGC);
    }
    htPtr->fillGC = newGC;

    if (htPtr->numColumns > 0) {
	htPtr->reqWidth =
	    htPtr->numColumns * Blt_TextWidth(htPtr->font, "0", 1);
    }
    if (htPtr->numRows > 0) {
	Blt_FontMetrics fontMetrics;

	Blt_Font_GetMetrics(htPtr->font, &fontMetrics);
	htPtr->reqHeight = htPtr->numRows * fontMetrics.linespace;
    }
    /*
     * If the either the -text or -file option changed, read in the
     * new text.  The -text option supersedes any -file option.
     */
    if (Blt_ConfigModified(configSpecs, "-file", "-text", (char *)NULL)) {
	int result;

	FreeText(htPtr);
	CreateTraces(htPtr);	/* Create variable traces */

	result = IncludeText(interp, htPtr, htPtr->fileName);

	DeleteTraces(htPtr);
	if (result == TCL_ERROR) {
	    FreeText(htPtr);
	    return TCL_ERROR;
	}
	ResetTextInfo(htPtr);
    }
    EventuallyRedraw(htPtr);
    return TCL_OK;
}

/* Layout Procedures */
/*
 *---------------------------------------------------------------------------
 *
 * TranslateAnchor --
 *
 * 	Translate the coordinates of a given bounding box based
 *	upon the anchor specified.  The anchor indicates where
 *	the given xy position is in relation to the bounding box.
 *
 *  		nw --- n --- ne
 *  		|            |     x,y ---+
 *  		w   center   e      |     |
 *  		|            |      +-----+
 *  		sw --- s --- se
 *
 * Results:
 *	The translated coordinates of the bounding box are returned.
 *
 *---------------------------------------------------------------------------
 */
static XPoint
TranslateAnchor(
    int deltaX, int deltaY,	/* Difference between outer/inner regions */
    Tk_Anchor anchor)		/* Direction of the anchor */
{
    XPoint point;

    point.x = point.y = 0;
    switch (anchor) {
    case TK_ANCHOR_NW:		/* Upper left corner */
	break;
    case TK_ANCHOR_W:		/* Left center */
	point.y = (deltaY / 2);
	break;
    case TK_ANCHOR_SW:		/* Lower left corner */
	point.y = deltaY;
	break;
    case TK_ANCHOR_N:		/* Top center */
	point.x = (deltaX / 2);
	break;
    case TK_ANCHOR_CENTER:	/* Centered */
	point.x = (deltaX / 2);
	point.y = (deltaY / 2);
	break;
    case TK_ANCHOR_S:		/* Bottom center */
	point.x = (deltaX / 2);
	point.y = deltaY;
	break;
    case TK_ANCHOR_NE:		/* Upper right corner */
	point.x = deltaX;
	break;
    case TK_ANCHOR_E:		/* Right center */
	point.x = deltaX;
	point.y = (deltaY / 2);
	break;
    case TK_ANCHOR_SE:		/* Lower right corner */
	point.x = deltaX;
	point.y = deltaY;
	break;
    }
    return point;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeCavitySize --
 *
 *	Sets the width and height of the cavity based upon the
 *	requested size of the embedded widget.  The requested space also
 *	includes any external padding which has been designated for
 *	this window.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The size of the cavity is set in the embedded widget information
 *	structure.  These values can effect how the embedded widget is
 *	packed into the master window.

 *---------------------------------------------------------------------------
 */
static void
ComputeCavitySize(EmbeddedWidget *winPtr)
{
    int width, height;
    int twiceBW;

    twiceBW = 2 * Tk_Changes(winPtr->tkwin)->border_width;
    if (winPtr->reqCavityWidth > 0) {
	width = winPtr->reqCavityWidth;
    } else if (winPtr->relCavityWidth > 0.0) {
	width = (int)((double)Tk_Width(winPtr->htPtr->tkwin) *
	    winPtr->relCavityWidth + 0.5);
    } else {
	width = GetEmbeddedWidgetWidth(winPtr) + PADDING(winPtr->xPad) + 
	    twiceBW;
    }
    winPtr->cavityWidth = width;

    if (winPtr->reqCavityHeight > 0) {
	height = winPtr->reqCavityHeight;
    } else if (winPtr->relCavityHeight > 0.0) {
	height = (int)((double)Tk_Height(winPtr->htPtr->tkwin) *
	    winPtr->relCavityHeight + 0.5);
    } else {
	height = GetEmbeddedWidgetHeight(winPtr) + PADDING(winPtr->yPad) + 
	    twiceBW;
    }
    winPtr->cavityHeight = height;
}

/*
 *---------------------------------------------------------------------------
 *
 * LayoutLine --
 *
 *	This procedure computes the total width and height needed
 *      to contain the text and widgets for a particular line.
 *      It also calculates the baseline of the text on the line with
 *	respect to the other widgets on the line.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
LayoutLine(HText *htPtr, Line *linePtr)
{
    EmbeddedWidget *winPtr;
    int textStart, textLength;
    int maxAscent, maxDescent, maxHeight;
    int ascent, descent;
    int median;			/* Difference of font ascent/descent values */
    Blt_ChainLink link;
    int x, y;
    int newX;
    Blt_FontMetrics fontMetrics;

    /* Initialize line defaults */
    Blt_Font_GetMetrics(htPtr->font, &fontMetrics);
    maxAscent = fontMetrics.ascent;
    maxDescent = fontMetrics.descent;
    median = fontMetrics.ascent - fontMetrics.descent;
    ascent = descent = 0;	/* Suppress compiler warnings */

    /*
     * Pass 1: Determine the maximum ascent (baseline) and descent
     * needed for the line.  We'll need this for figuring the top,
     * bottom, and center anchors.
     */
    for (link = Blt_Chain_FirstLink(linePtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	winPtr = Blt_Chain_GetValue(link);
	if (winPtr->tkwin == NULL) {
	    continue;
	}
	ComputeCavitySize(winPtr);

	switch (winPtr->justify) {
	case JUSTIFY_TOP:
	    ascent = fontMetrics.ascent + winPtr->padTop;
	    descent = winPtr->cavityHeight - fontMetrics.ascent;
	    break;
	case JUSTIFY_CENTER:
	    ascent = (winPtr->cavityHeight + median) / 2;
	    descent = (winPtr->cavityHeight - median) / 2;
	    break;
	case JUSTIFY_BOTTOM:
	    ascent = winPtr->cavityHeight - fontMetrics.descent;
	    descent = fontMetrics.descent;
	    break;
	}
	if (descent > maxDescent) {
	    maxDescent = descent;
	}
	if (ascent > maxAscent) {
	    maxAscent = ascent;
	}
    }

    maxHeight = maxAscent + maxDescent + htPtr->leader;
    x = 0;			/* Always starts from x=0 */
    y = 0;			/* Suppress compiler warning */
    textStart = linePtr->textStart;

    /*
     * Pass 2: Find the placements of the text and widgets along each
     * line.
     */
    for (link = Blt_Chain_FirstLink(linePtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	winPtr = Blt_Chain_GetValue(link);
	if (winPtr->tkwin == NULL) {
	    continue;
	}
	/* Get the width of the text leading to the widget. */
	textLength = (winPtr->precedingTextEnd - textStart);
	if (textLength > 0) {
	    Blt_Font_Measure(htPtr->font, htPtr->charArr + textStart,
		textLength, 10000, TK_AT_LEAST_ONE, &newX);
	    winPtr->precedingTextWidth = newX;
	    x += newX;
	}
	switch (winPtr->justify) {
	case JUSTIFY_TOP:
	    y = maxAscent - fontMetrics.ascent;
	    break;
	case JUSTIFY_CENTER:
	    y = maxAscent - (winPtr->cavityHeight + median) / 2;
	    break;
	case JUSTIFY_BOTTOM:
	    y = maxAscent + fontMetrics.descent - winPtr->cavityHeight;
	    break;
	}
	winPtr->x = x, winPtr->y = y;

	/* Skip over trailing space */
	textStart = winPtr->precedingTextEnd + 1;

	x += winPtr->cavityWidth;
    }

    /*
     * This can be either the trailing piece of a line after the last widget
     * or the entire line if no widgets are embedded in it.
     */
    textLength = (linePtr->textEnd - textStart) + 1;
    if (textLength > 0) {
	Blt_Font_Measure(htPtr->font, htPtr->charArr + textStart,
	    textLength, 10000, DEF_TEXT_FLAGS, &newX);
	x += newX;
    }
    /* Update line parameters */
    if ((linePtr->width != x) || (linePtr->height != maxHeight) ||
	(linePtr->baseline != maxAscent)) {
	htPtr->flags |= TEXT_DIRTY;
    }
    linePtr->width = x;
    linePtr->height = maxHeight;
    linePtr->baseline = maxAscent;
}

/*
 *---------------------------------------------------------------------------
 *
 * ComputeLayout --
 *
 *	This procedure computes the total width and height needed
 *      to contain the text and widgets from all the lines of text.
 *      It merely sums the heights and finds the maximum width of
 *	all the lines.  The width and height are needed for scrolling.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
ComputeLayout(HText *htPtr)
{
    int count;
    Line *linePtr;
    int height, width;

    width = height = 0;
    for (count = 0; count < htPtr->numLines; count++) {
	linePtr = htPtr->lineArr + count;

	linePtr->offset = height;
	LayoutLine(htPtr, linePtr);
	height += linePtr->height;
	if (linePtr->width > width) {
	    width = linePtr->width;
	}
    }
    /*
     * Set changed flag if new layout changed size of virtual text.
     */
    if ((height != htPtr->worldHeight) || (width != htPtr->worldWidth)) {
	htPtr->worldHeight = height, htPtr->worldWidth = width;
	htPtr->flags |= TEXT_DIRTY;
    }
}

/* Display Procedures */
/*
 *---------------------------------------------------------------------------
 *
 * GetVisibleLines --
 *
 * 	Calculates which lines are visible using the height
 *      of the viewport and y offset from the top of the text.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Only those line between first and last inclusive are
 * 	redrawn.
 *
 *---------------------------------------------------------------------------
 */
static int
GetVisibleLines(HText *htPtr)
{
    int topLine, bottomLine;
    int firstY, lastY;
    int lastLine;

    if (htPtr->numLines == 0) {
	htPtr->first = 0;
	htPtr->last = -1;
	return TCL_OK;
    }
    firstY = htPtr->pendingY;
    lastLine = htPtr->numLines - 1;

    /* First line */
    topLine = LineSearch(htPtr, firstY, 0, lastLine);
    if (topLine < 0) {
	/*
	 * This can't be. The y-coordinate offset must be corrupted.
	 */
	Blt_Warn("internal error: First position not found `%d'\n", firstY);
	return TCL_ERROR;
    }
    htPtr->first = topLine;

    /*
     * If there is less text than window space, the bottom line is the
     * last line of text.  Otherwise search for the line at the bottom
     * of the window.
     */
    lastY = firstY + Tk_Height(htPtr->tkwin) - 1;
    if (lastY < htPtr->worldHeight) {
	bottomLine = LineSearch(htPtr, lastY, topLine, lastLine);
    } else {
	bottomLine = lastLine;
    }
    if (bottomLine < 0) {
	/*
	 * This can't be. The newY offset must be corrupted.
	 */
	Blt_Warn("internal error: Last position not found `%d'\n", lastY);
#ifdef notdef
	fprintf(stderr, "worldHeight=%d,height=%d,top=%d,first=%d,last=%d\n",
	    htPtr->worldHeight, Tk_Height(htPtr->tkwin), firstY,
	    htPtr->lineArr[topLine].offset, htPtr->lineArr[lastLine].offset);
#endif
	return TCL_ERROR;
    }
    htPtr->last = bottomLine;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawSegment --
 *
 * 	Draws a line segment, designated by the segment structure.
 *	This routine handles the display of selected text by drawing
 *	a raised 3D border underneath the selected text.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The line segment is drawn on *draw*.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawSegment(
    HText *htPtr,
    Drawable draw,
    Line *linePtr,
    int x, int y,
    Segment *segPtr)
{
    int lastX, curPos, numChars;
    int textLength;
    int selStart, selEnd, selLength;
    Blt_FontMetrics fontMetrics;

#ifdef notdef
    fprintf(stderr, "DS select: first=%d,last=%d text: first=%d,last=%d\n",
	htPtr->selFirst, htPtr->selLast, segPtr->textStart, segPtr->textEnd);
#endif
    textLength = (segPtr->textEnd - segPtr->textStart) + 1;
    if (textLength < 1) {
	return;
    }
    Blt_Font_GetMetrics(htPtr->font, &fontMetrics);
    if ((segPtr->textEnd < htPtr->selFirst) ||
	(segPtr->textStart > htPtr->selLast)) {	/* No selected text */
	Blt_Font_Draw(Tk_Display(htPtr->tkwin), draw, htPtr->drawGC, 
		htPtr->font, Tk_Depth(htPtr->tkwin), 0.0f, 
		htPtr->charArr + segPtr->textStart, textLength - 1, 
		x, y + linePtr->baseline);
	return;
    }
    /*
     *	Text in a segment (with selected text) may have
     *	up to three regions:
     *
     *	1) the text before the start the selection
     *	2) the selected text itself (drawn in a raised border)
     *	3) the text following the selection.
     */

    selStart = segPtr->textStart;
    selEnd = segPtr->textEnd;
    if (htPtr->selFirst > segPtr->textStart) {
	selStart = htPtr->selFirst;
    }
    if (htPtr->selLast < segPtr->textEnd) {
	selEnd = htPtr->selLast;
    }
    selLength = (selEnd - selStart) + 1;
    lastX = x;
    curPos = segPtr->textStart;

    if (selStart > segPtr->textStart) {	/* Text preceding selection */
	numChars = (selStart - segPtr->textStart);
	Blt_Font_Measure(htPtr->font, htPtr->charArr + segPtr->textStart,
		numChars, 10000, DEF_TEXT_FLAGS, &lastX);
	lastX += x;
	Blt_Font_Draw(Tk_Display(htPtr->tkwin), draw, htPtr->drawGC, 
		htPtr->font, Tk_Depth(htPtr->tkwin), 0.0f, 
		htPtr->charArr + segPtr->textStart, numChars, 
		x, y + linePtr->baseline);
	curPos = selStart;
    }
    if (selLength > 0) {	/* The selection itself */
	int width, nextX;

	Blt_Font_Measure(htPtr->font, htPtr->charArr + selStart,
	    selLength, 10000, DEF_TEXT_FLAGS, &nextX);
	nextX += x;
	width = (selEnd == linePtr->textEnd)
	    ? htPtr->worldWidth - htPtr->xOffset - lastX :
	    nextX - lastX;
	Blt_Bg_FillRectangle(htPtr->tkwin, draw, htPtr->selBg,
	    lastX, y + linePtr->baseline - fontMetrics.ascent,
	    width, fontMetrics.linespace, htPtr->selBW,
	    TK_RELIEF_RAISED);
	Blt_Font_Draw(Tk_Display(htPtr->tkwin), draw, htPtr->selectGC, 
		htPtr->font, Tk_Depth(htPtr->tkwin), 0.0f, 
		htPtr->charArr + selStart, selLength, lastX, 
		y + linePtr->baseline);
	lastX = nextX;
	curPos = selStart + selLength;
    }
    numChars = segPtr->textEnd - curPos;
    if (numChars > 0) {		/* Text following the selection */
	Blt_Font_Draw(Tk_Display(htPtr->tkwin), draw, htPtr->drawGC, 
		htPtr->font, Tk_Depth(htPtr->tkwin), 0.0f, 
		htPtr->charArr + curPos, numChars - 1, lastX, 
		y + linePtr->baseline);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MoveEmbeddedWidget --
 *
 * 	Move a embedded widget to a new location in the hypertext
 *	parent window.  If the window has no geometry (i.e. width,
 *	or height is 0), simply unmap to window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Each embedded widget is moved to its new location, generating
 *      Expose events in the parent for each embedded widget moved.
 *
 *---------------------------------------------------------------------------
 */
static void
MoveEmbeddedWidget(EmbeddedWidget *winPtr, int offset)
{
    int winWidth, winHeight;
    int width, height;
    int deltaX, deltaY;
    int x, y;
    int intBW;

    winWidth = GetEmbeddedWidgetWidth(winPtr);
    winHeight = GetEmbeddedWidgetHeight(winPtr);
    if ((winWidth < 1) || (winHeight < 1)) {
	if (Tk_IsMapped(winPtr->tkwin)) {
	    Tk_UnmapWindow(winPtr->tkwin);
	}
	return;
    }
    intBW = Tk_Changes(winPtr->tkwin)->border_width;
    x = (winPtr->x + intBW + winPtr->padLeft) -
	winPtr->htPtr->xOffset;
    y = offset + (winPtr->y + intBW + winPtr->padTop) -
	winPtr->htPtr->yOffset;

    width = winPtr->cavityWidth - (2 * intBW + PADDING(winPtr->xPad));
    if (width < 0) {
	width = 0;
    }
    if ((width < winWidth) || (winPtr->fill & FILL_X)) {
	winWidth = width;
    }
    deltaX = width - winWidth;

    height = winPtr->cavityHeight - (2 * intBW + PADDING(winPtr->yPad));
    if (height < 0) {
	height = 0;
    }
    if ((height < winHeight) || (winPtr->fill & FILL_Y)) {
	winHeight = height;
    }
    deltaY = height - winHeight;

    if ((deltaX > 0) || (deltaY > 0)) {
	XPoint point;

	point = TranslateAnchor(deltaX, deltaY, winPtr->anchor);
	x += point.x, y += point.y;
    }
    winPtr->winWidth = winWidth;
    winPtr->winHeight = winHeight;

    if ((x != Tk_X(winPtr->tkwin)) || (y != Tk_Y(winPtr->tkwin)) ||
	(winWidth != Tk_Width(winPtr->tkwin)) ||
	(winHeight != Tk_Height(winPtr->tkwin))) {
	Tk_MoveResizeWindow(winPtr->tkwin, x, y, winWidth, winHeight);
    }
    if (!Tk_IsMapped(winPtr->tkwin)) {
	Tk_MapWindow(winPtr->tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DrawPage --
 *
 * 	This procedure displays the lines of text and moves the widgets
 *      to their new positions.  It draws lines with regard to
 *	the direction of the scrolling.  The idea here is to make the
 *	text and buttons appear to move together. Otherwise you will
 *	get a "jiggling" effect where the windows appear to bump into
 *	the next line before that line is moved.  In the worst case, where
 *	every line has at least one widget, you can get an aquarium effect
 *      (lines appear to ripple up).
 *
 * 	The text area may start between line boundaries (to accommodate
 *	both variable height lines and constant scrolling). Subtract the
 *	difference of the page offset and the line offset from the starting
 *	coordinates. For horizontal scrolling, simply subtract the offset
 *	of the viewport. The window will clip the top of the first line,
 *	the bottom of the last line, whatever text extends to the left
 *	or right of the viewport on any line.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the line in its current
 * 	mode.
 *
 *---------------------------------------------------------------------------
 */
static void
DrawPage(HText *htPtr, int deltaY) /* Change from previous Y coordinate */
{
    Line *linePtr;
    EmbeddedWidget *winPtr;
    Tk_Window tkwin = htPtr->tkwin;
    Segment sgmt;
    Pixmap pixmap;
    int forceCopy;
    int i;
    int lineNum;
    int x, y, lastY;
    Blt_ChainLink link;
    int width, height;
    Display *display;

    display = htPtr->display;
    width = Tk_Width(tkwin);
    height = Tk_Height(tkwin);

    /* Create an off-screen pixmap for semi-smooth scrolling. */
    pixmap = Blt_GetPixmap(display, Tk_WindowId(tkwin), width, height,
	  Tk_Depth(tkwin));

    x = -(htPtr->xOffset);
    y = -(htPtr->yOffset);

    if (htPtr->tileOffsetPage) {
	Blt_Bg_SetOrigin(htPtr->tkwin, htPtr->normalBg, x, y);
    } else {
	Blt_Bg_SetOrigin(htPtr->tkwin, htPtr->normalBg, 0, 0);
    }
    Blt_Bg_FillRectangle(htPtr->tkwin, pixmap, htPtr->normalBg, 0, 0, 
	width, height, TK_RELIEF_FLAT, 0);

    if (deltaY >= 0) {
	y += htPtr->lineArr[htPtr->first].offset;
	lineNum = htPtr->first;
	lastY = 0;
    } else {
	y += htPtr->lineArr[htPtr->last].offset;
	lineNum = htPtr->last;
	lastY = height;
    }
    forceCopy = 0;

    /* Draw each line */
    for (i = htPtr->first; i <= htPtr->last; i++) {

	/* Initialize character position in text buffer to start */
	linePtr = htPtr->lineArr + lineNum;
	sgmt.textStart = linePtr->textStart;
	sgmt.textEnd = linePtr->textEnd;

	/* Initialize X position */
	x = -(htPtr->xOffset);
	for (link = Blt_Chain_FirstLink(linePtr->chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    winPtr = Blt_Chain_GetValue(link);

	    if (winPtr->tkwin != NULL) {
		winPtr->flags |= WIDGET_VISIBLE;
		MoveEmbeddedWidget(winPtr, linePtr->offset);
	    }
	    sgmt.textEnd = winPtr->precedingTextEnd - 1;
	    if (sgmt.textEnd >= sgmt.textStart) {
		DrawSegment(htPtr, pixmap, linePtr, x, y, &sgmt);
		x += winPtr->precedingTextWidth;
	    }
	    /* Skip over the extra trailing space which designates the widget */
	    sgmt.textStart = winPtr->precedingTextEnd + 1;
	    x += winPtr->cavityWidth;
	    forceCopy++;
	}

	/*
	 * This may be the text trailing the last widget or the entire
	 * line if no widgets occur on it.
	 */
	sgmt.textEnd = linePtr->textEnd;
	if (sgmt.textEnd >= sgmt.textStart) {
	    DrawSegment(htPtr, pixmap, linePtr, x, y, &sgmt);
	}
	/* Go to the top of the next line */
	if (deltaY >= 0) {
	    y += htPtr->lineArr[lineNum].height;
	    lineNum++;
	}
	if ((forceCopy > 0) && !(htPtr->flags & TEXT_DIRTY)) {
	    if (deltaY >= 0) {
		XCopyArea(display, pixmap, Tk_WindowId(tkwin), htPtr->drawGC,
			  0, lastY, width, y - lastY, 0, lastY);
	    } else {
		XCopyArea(display, pixmap, Tk_WindowId(tkwin), htPtr->drawGC,
			  0, y, width, lastY - y, 0, y);
	    }
	    forceCopy = 0;		/* Reset drawing flag */
	    lastY = y;			/* Record last Y position */
	}
	if ((deltaY < 0) && (lineNum > 0)) {
	    --lineNum;
	    y -= htPtr->lineArr[lineNum].height;
	}
    }
    /*
     * If the viewport was resized, draw the page in one operation.
     * Otherwise draw any left-over block of text (either at the top
     * or bottom of the page)
     */
    if (htPtr->flags & TEXT_DIRTY) {
	XCopyArea(display, pixmap, Tk_WindowId(tkwin),
	    htPtr->drawGC, 0, 0, width, height, 0, 0);
    } else if (lastY != y) {
	if (deltaY >= 0) {
	    height -= lastY;
	    XCopyArea(display, pixmap, Tk_WindowId(tkwin),
		htPtr->drawGC, 0, lastY, width, height, 0, lastY);
	} else {
	    height = lastY;
	    XCopyArea(display, pixmap, Tk_WindowId(tkwin),
		htPtr->drawGC, 0, 0, width, height, 0, 0);
	}
    }
    Tk_FreePixmap(display, pixmap);
}


static void
SendBogusEvent(Tk_Window tkwin)
{
#define DONTPROPAGATE 0
    XEvent event;

    event.type = event.xexpose.type = Expose;
    event.xexpose.window = Tk_WindowId(tkwin);
    event.xexpose.display = Tk_Display(tkwin);
    event.xexpose.count = 0;
    event.xexpose.x = event.xexpose.y = 0;
    event.xexpose.width = Tk_Width(tkwin);
    event.xexpose.height = Tk_Height(tkwin);

    XSendEvent(Tk_Display(tkwin), Tk_WindowId(tkwin), DONTPROPAGATE,
	ExposureMask, &event);
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayText --
 *
 * 	This procedure is invoked to display a hypertext widget.
 *	Many of the operations which might ordinarily be performed
 *	elsewhere (e.g. in a configuration routine) are done here
 *	because of the somewhat unusual interactions occurring between
 *	the parent and embedded widgets.
 *
 *      Recompute the layout of the text if necessary. This is
 *	necessary if the world coordinate system has changed.
 *	Specifically, the following may have occurred:
 *
 *	  1.  a text attribute has changed (font, linespacing, etc.).
 *	  2.  widget option changed (anchor, width, height).
 *        3.  actual embedded widget was resized.
 *	  4.  new text string or file.
 *
 *      This is deferred to the display routine since potentially
 *      many of these may occur (especially embedded widget changes).
 *
 *	Set the vertical and horizontal scrollbars (if they are
 *	designated) by issuing a TCL command.  Done here since
 *	the text window width and height are needed.
 *
 *	If the viewport position or contents have changed in the
 *	vertical direction,  the now out-of-view embedded widgets
 *	must be moved off the viewport.  Since embedded widgets will
 *	obscure the text window, it is imperative that the widgets
 *	are moved off before we try to redraw text in the same area.
 *      This is necessary only for vertical movements.  Horizontal
 *	embedded widget movements are handled automatically in the
 *	page drawing routine.
 *
 *      Get the new first and last line numbers for the viewport.
 *      These line numbers may have changed because either a)
 *      the viewport changed size or position, or b) the text
 *	(embedded widget sizes or text attributes) have changed.
 *
 *	If the viewport has changed vertically (i.e. the first or
 *      last line numbers have changed), move the now out-of-view
 *	embedded widgets off the viewport.
 *
 *      Potentially many expose events may be generated when the
 *	the individual embedded widgets are moved and/or resized.
 *	These events need to be ignored.  Since (I think) expose
 * 	events are guaranteed to happen in order, we can bracket
 *	them by sending phony events (via XSendEvent). The phony
 *      events turn on and off flags indicating which events
*	should be ignored.
 *
 *	Finally, the page drawing routine is called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 * 	Commands are output to X to display the hypertext in its
 *	current mode.
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayText(ClientData clientData) /* Information about widget. */
{
    HText *htPtr = clientData;
    Tk_Window tkwin = htPtr->tkwin;
    int oldFirst;		/* First line of old viewport */
    int oldLast;		/* Last line of old viewport */
    int deltaY;			/* Change in viewport in Y direction */
    int reqWidth, reqHeight;

#ifdef notdef
    fprintf(stderr, "calling DisplayText(%s)\n", Tk_PathName(htPtr->tkwin));
#endif
    htPtr->flags &= ~REDRAW_PENDING;
    if (tkwin == NULL) {
	return;			/* Window has been destroyed */
    }
    if (htPtr->flags & REQUEST_LAYOUT) {
	/*
	 * Recompute the layout when widgets are created, deleted,
	 * moved, or resized.  Also when text attributes (such as
	 * font, linespacing) have changed.
	 */
	ComputeLayout(htPtr);
    }
    htPtr->lastWidth = Tk_Width(tkwin);
    htPtr->lastHeight = Tk_Height(tkwin);

    /*
     * Check the requested width and height.  We allow two modes:
     * 	1) If the user requested value is greater than zero, use it.
     *  2) Otherwise, let the window be as big as the virtual text.
     *	   This could be too large to display, so constrain it by
     *	   the maxWidth and maxHeight values.
     *
     * In any event, we need to calculate the size of the virtual
     * text and then make a geometry request.  This is so that widgets
     * whose size is relative to the master, will be set once.
     */
    if (htPtr->reqWidth > 0) {
	reqWidth = htPtr->reqWidth;
    } else {
	reqWidth = MIN(htPtr->worldWidth, htPtr->maxWidth);
	if (reqWidth < 1) {
	    reqWidth = 1;
	}
    }
    if (htPtr->reqHeight > 0) {
	reqHeight = htPtr->reqHeight;
    } else {
	reqHeight = MIN(htPtr->worldHeight, htPtr->maxHeight);
	if (reqHeight < 1) {
	    reqHeight = 1;
	}
    }
    if ((reqWidth != Tk_ReqWidth(tkwin)) || (reqHeight != Tk_ReqHeight(tkwin))) {
	Tk_GeometryRequest(tkwin, reqWidth, reqHeight);

	EventuallyRedraw(htPtr);
	return;			/* Try again with new geometry */
    }
    if (!Tk_IsMapped(tkwin)) {
	return;
    }
    /*
     * Turn off layout requests here, after the text window has been
     * mapped.  Otherwise, relative embedded widget size requests wrt
     * to the size of parent text window will be wrong.
     */
    htPtr->flags &= ~REQUEST_LAYOUT;

    /* Is there a pending goto request? */
    if (htPtr->flags & GOTO_PENDING) {
	htPtr->pendingY = htPtr->lineArr[htPtr->reqLineNum].offset;
	htPtr->flags &= ~GOTO_PENDING;
    }
    deltaY = htPtr->pendingY - htPtr->yOffset;
    oldFirst = htPtr->first, oldLast = htPtr->last;

    /*
     * If the viewport has changed size or position, or the text
     * and/or embedded widgets have changed, adjust the scrollbars to
     * new positions.
     */
    if (htPtr->flags & TEXT_DIRTY) {
	int width, height;

	width = Tk_Width(htPtr->tkwin);
	height = Tk_Height(htPtr->tkwin);

	/* Reset viewport origin and world extents */
	htPtr->xOffset = Blt_AdjustViewport(htPtr->pendingX,
	    htPtr->worldWidth, width,
	    htPtr->xScrollUnits, BLT_SCROLL_MODE_LISTBOX);
	htPtr->yOffset = Blt_AdjustViewport(htPtr->pendingY,
	    htPtr->worldHeight, height,
	    htPtr->yScrollUnits, BLT_SCROLL_MODE_LISTBOX);
	if (htPtr->xScrollCmdObjPtr != NULL) {
	    Blt_UpdateScrollbar(htPtr->interp, htPtr->xScrollCmdObjPtr,
		htPtr->xOffset, htPtr->xOffset + width, htPtr->worldWidth);
	}
	if (htPtr->yScrollCmdObjPtr != NULL) {
	    Blt_UpdateScrollbar(htPtr->interp, htPtr->yScrollCmdObjPtr,
		htPtr->yOffset, htPtr->yOffset + height, htPtr->worldHeight);
	}
	/*
	 * Given a new viewport or text height, find the first and
	 * last line numbers of the new viewport.
	 */
	if (GetVisibleLines(htPtr) != TCL_OK) {
	    return;
	}
    }
    /*
     * 	This is a kludge: Send an expose event before and after
     * 	drawing the page of text.  Since moving and resizing of the
     * 	embedded widgets will cause redundant expose events in the parent
     * 	window, the phony events will bracket them indicating no
     * 	action should be taken.
     */
    SendBogusEvent(tkwin);

    /*
     * If either the position of the viewport has changed or the size
     * of width or height of the entire text have changed, move the
     * widgets from the previous viewport out of the current
     * viewport. Worry only about the vertical embedded widget movements.
     * The page is always draw at full width and the viewport will clip
     * the text.
     */
    if ((htPtr->first != oldFirst) || (htPtr->last != oldLast)) {
	int offset;
	int i;
	int first, last;
	Blt_ChainLink link;
	EmbeddedWidget *winPtr;

	/* Figure out which lines are now out of the viewport */

	if ((htPtr->first > oldFirst) && (htPtr->first <= oldLast)) {
	    first = oldFirst, last = htPtr->first;
	} else if ((htPtr->last < oldLast) && (htPtr->last >= oldFirst)) {
	    first = htPtr->last, last = oldLast;
	} else {
	    first = oldFirst, last = oldLast;
	}

	for (i = first; i <= last; i++) {
	    offset = htPtr->lineArr[i].offset;
	    for (link = Blt_Chain_FirstLink(htPtr->lineArr[i].chain);
		link != NULL; link = Blt_Chain_NextLink(link)) {
		winPtr = Blt_Chain_GetValue(link);
		if (winPtr->tkwin != NULL) {
		    MoveEmbeddedWidget(winPtr, offset);
		    winPtr->flags &= ~WIDGET_VISIBLE;
		}
	    }
	}
    }
    DrawPage(htPtr, deltaY);
    SendBogusEvent(tkwin);

    /* Reset flags */
    htPtr->flags &= ~TEXT_DIRTY;
}

/* Selection Procedures */
/*
 *---------------------------------------------------------------------------
 *
 * TextSelectionProc --
 *
 *	This procedure is called back by Tk when the selection is
 *	requested by someone.  It returns part or all of the selection
 *	in a buffer provided by the caller.
 *
 * Results:
 *	The return value is the number of non-NULL bytes stored
 *	at buffer.  Buffer is filled (or partially filled) with a
 *	NULL-terminated string containing part or all of the selection,
 *	as given by offset and maxBytes.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static int
TextSelectionProc(
    ClientData clientData,	/* Information about Text widget. */
    int offset,			/* Offset within selection of first
				 * character to be returned. */
    char *buffer,		/* Location in which to place
				 * selection. */
    int maxBytes)		/* Maximum number of bytes to place
				 * at buffer, not including terminating
				 * NULL character. */
{
    HText *htPtr = clientData;
    int size;

    if ((htPtr->selFirst < 0) || (!htPtr->exportSelection)) {
	return -1;
    }
    size = (htPtr->selLast - htPtr->selFirst) + 1 - offset;
    if (size > maxBytes) {
	size = maxBytes;
    }
    if (size <= 0) {
	return 0;		/* huh? */
    }
    strncpy(buffer, htPtr->charArr + htPtr->selFirst + offset, size);
    buffer[size] = '\0';
    return size;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextLostSelection --
 *
 *	This procedure is called back by Tk when the selection is
 *	grabbed away from a Text widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The existing selection is unhighlighted, and the window is
 *	marked as not containing a selection.
 *
 *---------------------------------------------------------------------------
 */
static void
TextLostSelection(ClientData clientData) /* Information about Text widget. */
{
    HText *htPtr = clientData;

    if ((htPtr->selFirst >= 0) && (htPtr->exportSelection)) {
	htPtr->selFirst = htPtr->selLast = -1;
	EventuallyRedraw(htPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectLine --
 *
 *	Modify the selection by moving both its anchored and un-anchored
 *	ends.  This could make the selection either larger or smaller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectLine(
    HText *htPtr,		/* Information about widget. */
    int tindex)			/* Index of element that is to
				 * become the "other" end of the
				 * selection. */
{
    int selFirst, selLast;
    int lineNum;
    Line *linePtr;

    lineNum = IndexSearch(htPtr, tindex, 0, htPtr->numLines - 1);
    if (lineNum < 0) {
	char string[200];

	Blt_FormatString(string, 200, "can't determine line number from index \"%d\"",
	    tindex);
	Tcl_AppendResult(htPtr->interp, string, (char *)NULL);
	return TCL_ERROR;
    }
    linePtr = htPtr->lineArr + lineNum;
    /*
     * Grab the selection if we don't own it already.
     */
    if ((htPtr->exportSelection) && (htPtr->selFirst == -1)) {
	Tk_OwnSelection(htPtr->tkwin, XA_PRIMARY, TextLostSelection, htPtr);
    }
    selFirst = linePtr->textStart;
    selLast = linePtr->textEnd;
    htPtr->selAnchor = tindex;
    if ((htPtr->selFirst != selFirst) ||
	(htPtr->selLast != selLast)) {
	htPtr->selFirst = selFirst;
	htPtr->selLast = selLast;
	EventuallyRedraw(htPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectWord --
 *
 *	Modify the selection by moving both its anchored and un-anchored
 *	ends.  This could make the selection either larger or smaller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectWord(
    HText *htPtr,		/* Information about widget. */
    int tindex)			/* Index of element that is to
				 * become the "other" end of the
				 * selection. */
{
    int selFirst, selLast;
    int i;

    for (i = tindex; i < htPtr->numChars; i++) {
	if (isspace(UCHAR(htPtr->charArr[i]))) {
	    break;
	}
    }
    selLast = i - 1;
    for (i = tindex; i >= 0; i--) {
	if (isspace(UCHAR(htPtr->charArr[i]))) {
	    break;
	}
    }
    selFirst = i + 1;
    if (selFirst > selLast) {
	selFirst = selLast = tindex;
    }
    /*
     * Grab the selection if we don't own it already.
     */
    if ((htPtr->exportSelection) && (htPtr->selFirst == -1)) {
	Tk_OwnSelection(htPtr->tkwin, XA_PRIMARY, TextLostSelection, htPtr);
    }
    htPtr->selAnchor = tindex;
    if ((htPtr->selFirst != selFirst) || (htPtr->selLast != selLast)) {
	htPtr->selFirst = selFirst, htPtr->selLast = selLast;
	EventuallyRedraw(htPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectTextBlock --
 *
 *	Modify the selection by moving its un-anchored end.  This
 *	could make the selection either larger or smaller.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectTextBlock(
    HText *htPtr,		/* Information about widget. */
    int tindex)			/* Index of element that is to
				 * become the "other" end of the
				 * selection. */
{
    int selFirst, selLast;

    /*
     * Grab the selection if we don't own it already.
     */

    if ((htPtr->exportSelection) && (htPtr->selFirst == -1)) {
	Tk_OwnSelection(htPtr->tkwin, XA_PRIMARY, TextLostSelection, htPtr);
    }
    /*  If the anchor hasn't been set yet, assume the beginning of the text*/
    if (htPtr->selAnchor < 0) {
	htPtr->selAnchor = 0;
    }
    if (htPtr->selAnchor <= tindex) {
	selFirst = htPtr->selAnchor;
	selLast = tindex;
    } else {
	selFirst = tindex;
	selLast = htPtr->selAnchor;
    }
    if ((htPtr->selFirst != selFirst) || (htPtr->selLast != selLast)) {
	htPtr->selFirst = selFirst, htPtr->selLast = selLast;
	EventuallyRedraw(htPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectOp --
 *
 *	This procedure handles the individual options for text
 *	selections.  The selected text is designated by start and end
 *	indices into the text pool.  The selected segment has both a
 *	anchored and unanchored ends.  The following selection
 *	operations are implemented:
 *
 *	  "adjust"	- resets either the first or last index
 *			  of the selection.
 *	  "clear"	- clears the selection. Sets first/last
 *			  indices to -1.
 *	  "from"	- sets the index of the selection anchor.
 *	  "line"	- sets the first of last indices to the
 *			  start and end of the line at the
 *			  designated point.
 *	  "present"	- return "1" if a selection is available,
 *			  "0" otherwise.
 *	  "range"	- sets the first and last indices.
 *	  "to"		- sets the index of the un-anchored end.
 *	  "word"	- sets the first of last indices to the
 *			  start and end of the word at the
 *			  designated point.
 * Results:
 *	None.
 *
 * Side effects:
 *	The selection changes.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectOp(
    HText *htPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    char *string;
    char c;
    int iselection;
    int length;
    int result = TCL_OK;

    string = Tcl_GetStringFromObj(objv[2], &length);
    c = string[0];
    if ((c == 'c') && (strncmp(string, "clear", length) == 0)) {
	if (objc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " selection clear\"", (char *)NULL);
	    return TCL_ERROR;
	}
	if (htPtr->selFirst != -1) {
	    htPtr->selFirst = htPtr->selLast = -1;
	    EventuallyRedraw(htPtr);
	}
	return TCL_OK;
    } else if ((c == 'p') && (strncmp(string, "present", length) == 0)) {
	if (objc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " selection present\"", (char *)NULL);
	    return TCL_ERROR;
	}
	Tcl_AppendResult(interp, (htPtr->selFirst != -1) ? "0" : "1",
	    (char *)NULL);
	return TCL_OK;
    } else if ((c == 'r') && (strncmp(string, "range", length) == 0)) {
	int selFirst, selLast;

	if (objc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " selection range first last\"", 
		(char *)NULL);
	    return TCL_ERROR;
	}
	if (GetIndex(htPtr, objv[3], &selFirst) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (GetIndex(htPtr, objv[4], &selLast) != TCL_OK) {
	    return TCL_ERROR;
	}
	htPtr->selAnchor = selFirst;
	SelectTextBlock(htPtr, selLast);
	return TCL_OK;
    }
    if (objc != 4) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " selection ", 
		Tcl_GetString(objv[2]), " index\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (GetIndex(htPtr, objv[3], &iselection) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((c == 'f') && (strncmp(string, "from", length) == 0)) {
	htPtr->selAnchor = iselection;
    } else if ((c == 'a') && (strncmp(string, "adjust", length) == 0)) {
	int half1, half2;

	half1 = (htPtr->selFirst + htPtr->selLast) / 2;
	half2 = (htPtr->selFirst + htPtr->selLast + 1) / 2;
	if (iselection < half1) {
	    htPtr->selAnchor = htPtr->selLast;
	} else if (iselection > half2) {
	    htPtr->selAnchor = htPtr->selFirst;
	}
	result = SelectTextBlock(htPtr, iselection);
    } else if ((c == 't') && (strncmp(string, "to", length) == 0)) {
	result = SelectTextBlock(htPtr, iselection);
    } else if ((c == 'w') && (strncmp(string, "word", length) == 0)) {
	result = SelectWord(htPtr, iselection);
    } else if ((c == 'l') && (strncmp(string, "line", length) == 0)) {
	result = SelectLine(htPtr, iselection);
    } else {
	Tcl_AppendResult(interp, "bad selection operation \"", string,
	    "\": should be \"adjust\", \"clear\", \"from\", \"line\", \
\"present\", \"range\", \"to\", or \"word\"", (char *)NULL);
	return TCL_ERROR;
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * GotoOp --
 *
 *	Move the top line of the viewport to the new location based
 *	upon the given line number.  Force out-of-range requests to the
 *	top or bottom of text.
 *
 * Results:
 *	A standard TCL result. If TCL_OK, interp->result contains the
 *	current line number.
 *
 * Side effects:
 *	At the next idle point, the text viewport will be move to the
 *	new line.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GotoOp(
    HText *htPtr,
    Tcl_Interp *interp,		/* Not used. */
    int objc,
    Tcl_Obj *const *objv)
{
    int line;

    line = htPtr->first;
    if (objc == 3) {
	int tindex;

	if (GetIndex(htPtr, objv[2], &tindex) != TCL_OK) {
	    return TCL_ERROR;
	}
	line = IndexSearch(htPtr, tindex, 0, htPtr->numLines - 1);
	if (line < 0) {
	    char string[200];

	    Blt_FormatString(string, 200, 
		"can't determine line number from index \"%d\"", tindex);
	    Tcl_AppendResult(htPtr->interp, string, (char *)NULL);
	    return TCL_ERROR;
	}
	htPtr->reqLineNum = line;
	htPtr->flags |= TEXT_DIRTY;

	/*
	 * Make only a request for a change in the viewport.  Defer
	 * the actual scrolling until the text layout is adjusted at
	 * the next idle point.
	 */
	if (line != htPtr->first) {
	    htPtr->flags |= GOTO_PENDING;
	    EventuallyRedraw(htPtr);
	}
    }
    Tcl_SetIntObj(Tcl_GetObjResult(htPtr->interp), line);
    return TCL_OK;
}


static int
XViewOp(
    HText *htPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    int width, worldWidth;

    width = Tk_Width(htPtr->tkwin);
    worldWidth = htPtr->worldWidth;
    if (objc == 2) {
	double fract;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	/* Report first and last fractions */
	fract = (double)htPtr->xOffset / worldWidth;
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewDoubleObj(CLAMP(fract, 0.0, 1.0)));
	fract = (double)(htPtr->xOffset + width) / worldWidth;
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewDoubleObj(CLAMP(fract, 0.0, 1.0)));
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    htPtr->pendingX = htPtr->xOffset;
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &htPtr->pendingX,
	    worldWidth, width, htPtr->xScrollUnits, BLT_SCROLL_MODE_LISTBOX) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    htPtr->flags |= TEXT_DIRTY;
    EventuallyRedraw(htPtr);
    return TCL_OK;
}

static int
YViewOp(
    HText *htPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    int height, worldHeight;

    height = Tk_Height(htPtr->tkwin);
    worldHeight = htPtr->worldHeight;
    if (objc == 2) {
	double fract;
	Tcl_Obj *listObjPtr;

	/* Report first and last fractions */
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	fract = (double)htPtr->yOffset / worldHeight;
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewDoubleObj(CLAMP(fract, 0.0, 1.0)));
	fract = (double)(htPtr->yOffset + height) / worldHeight;
	Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewDoubleObj(CLAMP(fract, 0.0, 1.0)));
	Tcl_SetObjResult(interp, listObjPtr);
	return TCL_OK;
    }
    htPtr->pendingY = htPtr->yOffset;
    if (Blt_GetScrollInfoFromObj(interp, objc - 2, objv + 2, &htPtr->pendingY,
	    worldHeight, height, htPtr->yScrollUnits, BLT_SCROLL_MODE_LISTBOX)
	!= TCL_OK) {
	return TCL_ERROR;
    }
    htPtr->flags |= TEXT_DIRTY;
    EventuallyRedraw(htPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AppendOp --
 *
 * 	This procedure embeds a Tk widget into the hypertext.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	Memory is allocated.  EmbeddedWidget gets configured.
 *
 *---------------------------------------------------------------------------
 */
static int
AppendOp(
    HText *htPtr,		/* Hypertext widget */
    Tcl_Interp *interp,		/* Interpreter associated with widget */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Argument strings. */
{
    Line *linePtr;
    EmbeddedWidget *winPtr;

    winPtr = CreateEmbeddedWidget(htPtr, Tcl_GetString(objv[2]));
    if (winPtr == NULL) {
	return TCL_ERROR;
    }
    if (Blt_ConfigureWidgetFromObj(interp, htPtr->tkwin, widgetConfigSpecs,
	    objc - 3, objv + 3, (char *)winPtr, 0) != TCL_OK) {
	return TCL_ERROR;
    }
    /*
     * Append widget to list of embedded widgets of the last line.
     */
    linePtr = GetLastLine(htPtr);
    if (linePtr == NULL) {
	Tcl_AppendResult(interp, "can't allocate line structure", (char *)NULL);
	return TCL_ERROR;
    }
    Blt_Chain_Append(linePtr->chain, winPtr);
    linePtr->width += winPtr->cavityWidth;
    winPtr->precedingTextEnd = linePtr->textEnd;

    htPtr->flags |= (WIDGET_APPENDED | REQUEST_LAYOUT);
    EventuallyRedraw(htPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * WindowsOp --
 *
 *	Returns a list of all the pathNames of embedded widgets of the
 *	HText widget.  If a pattern argument is given, only the names
 *	of windows matching it will be placed into the list.
 *
 * Results:
 *	Standard TCL result.  If TCL_OK, interp->result will contain
 *	the list of the embedded widget pathnames.  Otherwise it will
 *	contain an error message.
 *
 *---------------------------------------------------------------------------
 */
static int
WindowsOp(
    HText *htPtr,		/* Hypertext widget record */
    Tcl_Interp *interp,		/* Interpreter associated with widget */
    int objc,
    Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    char *name;
    char *pattern;

    pattern = (objc == 2) ? NULL : Tcl_GetString(objv[2]);
    for (hPtr = Blt_FirstHashEntry(&htPtr->widgetTable, &cursor);
	hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	EmbeddedWidget *winPtr;

	winPtr = Blt_GetHashValue(hPtr);
	if (winPtr->tkwin == NULL) {
	    Blt_Warn("window `%s' is null\n",
		Tk_PathName(Blt_GetHashKey(&htPtr->widgetTable, hPtr)));
	    continue;
	}
	name = Tk_PathName(winPtr->tkwin);
	if ((pattern == NULL) || (Tcl_StringMatch(name, pattern))) {
	    Tcl_AppendElement(interp, name);
	}
    }
    return TCL_OK;
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
CgetOp(
    HText *htPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    char *itemPtr;
    Blt_ConfigSpec *specsPtr;
    char *string;

    if (objc > 3) {
	string = Tcl_GetString(objv[2]);
	if (string[0] == '.') {
	    Tk_Window tkwin;
	    EmbeddedWidget *winPtr;
	    
	    /* EmbeddedWidget window to be configured */
	    tkwin = Tk_NameToWindow(interp, string, htPtr->tkwin);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    winPtr = FindEmbeddedWidget(htPtr, tkwin);
	    if (winPtr == NULL) {
		Tcl_AppendResult(interp, "window \"", string,
				 "\" is not managed by \"", 
				 Tcl_GetString(objv[0]), "\"", 
				 (char *)NULL);
		return TCL_ERROR;
	    }
	    specsPtr = widgetConfigSpecs;
	    itemPtr = (char *)winPtr;
	    return Blt_ConfigureValueFromObj(interp, htPtr->tkwin, specsPtr, 
					     itemPtr, objv[3], 0);
	}
    }
    specsPtr = configSpecs;
    itemPtr = (char *)htPtr;
    return Blt_ConfigureValueFromObj(interp, htPtr->tkwin, specsPtr, 
	itemPtr, objv[2], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
 *
 * 	This procedure is called to process an objv/objc list, plus
 *	the Tk option database, in order to configure (or reconfigure)
 *	a hypertext widget.
 *
 * Results:
 *	A standard TCL result.  If TCL_ERROR is returned, then
 *	interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 * 	etc. get set for htPtr;  old resources get freed, if there were any.
 * 	The hypertext is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(
    HText *htPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    char *itemPtr;
    Blt_ConfigSpec *specsPtr;

    if (objc > 2) {
	char *string;

	string = Tcl_GetString(objv[2]);
	if (string[0] == '.') {
	    Tk_Window tkwin;
	    EmbeddedWidget *winPtr;
	    
	    /* EmbeddedWidget window to be configured */
	    tkwin = Tk_NameToWindow(interp, string, htPtr->tkwin);
	    if (tkwin == NULL) {
		return TCL_ERROR;
	    }
	    winPtr = FindEmbeddedWidget(htPtr, tkwin);
	    if (winPtr == NULL) {
		Tcl_AppendResult(interp, "window \"", string,
			"\" is not managed by \"", Tcl_GetString(objv[0]), 
			"\"", (char *)NULL);
		return TCL_ERROR;
	    }
	    specsPtr = widgetConfigSpecs;
	    itemPtr = (char *)winPtr;
	    objv++;
	    objc--;
	    goto config;
	}
    }
    specsPtr = configSpecs;
    itemPtr = (char *)htPtr;
 config:
    if (objc == 2) {
	return Blt_ConfigureInfoFromObj(interp, htPtr->tkwin, specsPtr, itemPtr,
		(Tcl_Obj *)NULL, 0);
    } else if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, htPtr->tkwin, specsPtr, itemPtr,
		objv[2], 0);
    }
    if (Blt_ConfigureWidgetFromObj(interp, htPtr->tkwin, specsPtr, objc - 2,
	    objv + 2, itemPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    if (itemPtr == (char *)htPtr) {
	/* Reconfigure the master */
	if (ConfigureText(interp, htPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	htPtr->flags |= REQUEST_LAYOUT;
    }
    EventuallyRedraw(htPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ScanOp --
 *
 *	Implements the quick scan for hypertext widgets.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ScanOp(
    HText *htPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    char *string;
    char c;
    int length;
    int x, y;

    string = Tcl_GetString(objv[3]);
    if (Blt_GetXY(interp, htPtr->tkwin, string, &x, &y) != TCL_OK) {
	return TCL_ERROR;
    }
    string = Tcl_GetStringFromObj(objv[2], &length);
    c = string[0];
    if ((c == 'm') && (strncmp(string, "mark", length) == 0)) {
	htPtr->scanMark.x = x, htPtr->scanMark.y = y;
	htPtr->scanPt.x = htPtr->xOffset;
	htPtr->scanPt.y = htPtr->yOffset;

    } else if ((c == 'd') && (strncmp(string, "dragto", length) == 0)) {
	int px, py;

	px = htPtr->scanPt.x - (10 * (x - htPtr->scanMark.x));
	py = htPtr->scanPt.y - (10 * (y - htPtr->scanMark.y));

	if (px < 0) {
	    px = htPtr->scanPt.x = 0;
	    htPtr->scanMark.x = x;
	} else if (px >= htPtr->worldWidth) {
	    px = htPtr->scanPt.x = htPtr->worldWidth - htPtr->xScrollUnits;
	    htPtr->scanMark.x = x;
	}
	if (py < 0) {
	    py = htPtr->scanPt.y = 0;
	    htPtr->scanMark.y = y;
	} else if (py >= htPtr->worldHeight) {
	    py = htPtr->scanPt.y = htPtr->worldHeight - htPtr->yScrollUnits;
	    htPtr->scanMark.y = y;
	}
	if ((py != htPtr->pendingY) || (px != htPtr->pendingX)) {
	    htPtr->pendingX = px, htPtr->pendingY = py;
	    htPtr->flags |= TEXT_DIRTY;
	    EventuallyRedraw(htPtr);
	}
    } else {
	Tcl_AppendResult(interp, "bad scan operation \"", string,
	    "\": should be either \"mark\" or \"dragto\"", (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SearchOp --
 *
 *	Returns the linenumber of the next line matching the given
 *	pattern within the range of lines provided.  If the first
 *	line number is greater than the last, the search is done in
 *	reverse.
 *
 *---------------------------------------------------------------------------
 */
static int
SearchOp(
    HText *htPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    const char *startPtr;
    Tcl_RegExp regExpToken;
    int iFirst, iLast;
    int matchStart, matchEnd;
    int match;
    char *string;

    string = Tcl_GetString(objv[2]);
    regExpToken = Tcl_RegExpCompile(interp, string);
    if (regExpToken == NULL) {
	return TCL_ERROR;
    }
    iFirst = 0;
    iLast = htPtr->numChars;
    if (objc > 3) {
	if (GetIndex(htPtr, objv[3], &iFirst) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (objc == 4) {
	if (GetIndex(htPtr, objv[4], &iLast) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (iLast < iFirst) {
	return TCL_ERROR;
    }
    matchStart = matchEnd = -1;
    startPtr = htPtr->charArr + iFirst;
    {
	char *p;
	char saved;

	p = htPtr->charArr + (iLast + 1);
	saved = *p;
	*p = '\0';		/* Make the line a string by changing the
				 * '\n' into a NUL byte before searching */
	match = Tcl_RegExpExec(interp, regExpToken, startPtr, startPtr);
	*p = saved;
    }
    if (match < 0) {
	return TCL_ERROR;
    } else if (match > 0) {
	const char *endPtr;

	Tcl_RegExpRange(regExpToken, 0, &startPtr, &endPtr);
	if ((startPtr != NULL) || (endPtr != NULL)) {
	    matchStart = startPtr - htPtr->charArr;
	    matchEnd = endPtr - htPtr->charArr - 1;
	}
    }
    if (match > 0) {
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(matchStart));
	Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(matchEnd));
	Tcl_SetObjResult(interp, listObjPtr);
    } else {
	Tcl_ResetResult(interp);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RangeOp --
 *
 *	Returns the characters designated by the range of elements.
 *
 *---------------------------------------------------------------------------
 */
static int
RangeOp(
    HText *htPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    char *startPtr, *endPtr;
    char saved;
    int textFirst, textLast;

    textFirst = htPtr->selFirst;
    textLast = htPtr->selLast;
    if (textFirst < 0) {
	textFirst = 0;
	textLast = htPtr->numChars - 1;
    }
    if (objc > 2) {
	if (GetIndex(htPtr, objv[2], &textFirst) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (objc == 4) {
	if (GetIndex(htPtr, objv[3], &textLast) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (textLast < textFirst) {
	Tcl_AppendResult(interp, "first index is greater than last", 
		 (char *)NULL);
	return TCL_ERROR;
    }
    startPtr = htPtr->charArr + textFirst;
    endPtr = htPtr->charArr + (textLast + 1);
    saved = *endPtr;
    *endPtr = '\0';		/* Make the line into a string by
				 * changing the * '\n' into a '\0'
				 * before copying */
    Tcl_SetStringObj(Tcl_GetObjResult(interp), startPtr, -1);
    *endPtr = saved;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IndexOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IndexOp(
    HText *htPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    int tindex;

    if (GetIndex(htPtr, objv[2], &tindex) != TCL_OK) {
	return TCL_ERROR;
    }
    Tcl_SetIntObj(Tcl_GetObjResult(interp), tindex);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LinePosOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
LinePosOp(
    HText *htPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    int line, cpos, tindex;
    char string[200];

    if (GetIndex(htPtr, objv[2], &tindex) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetTextPosition(htPtr, tindex, &line, &cpos) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_FormatString(string, 200, "%d.%d", line, cpos);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), string, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextWidgetCmd --
 *
 * 	This procedure is invoked to process the TCL command that
 *	corresponds to a widget managed by this module. See the user
 * 	documentation for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

static Blt_OpSpec textOps[] =
{
    {"append",    1, AppendOp,    3, 0, "window ?option value?...",},
    {"cget",	  2, CgetOp,      3, 3, "?window? option",},
    {"configure", 2, ConfigureOp, 2, 0, "?window? ?option value?...",},
    {"gotoline",  1, GotoOp,      2, 3, "?line?",},
    {"index",     1, IndexOp,     3, 3, "string",},
    {"linepos",   1, LinePosOp,   3, 3, "string",},
    {"range",     1, RangeOp,     2, 4, "?from? ?to?",},
    {"scan",      2, ScanOp,      4, 4, "oper @x,y",},
    {"search",    3, SearchOp,    3, 5, "pattern ?from? ?to?",},
    {"selection", 3, SelectOp,    3, 5, "oper ?index?",},
    {"windows",   1, WindowsOp,   2, 3, "?pattern?",},
    {"xview",     1, XViewOp,     2, 5, "?moveto fract? ?scroll number what?",},
    {"yview",     1, YViewOp,     2, 5, "?moveto fract? ?scroll number what?",},
};
static int numTextOps = sizeof(textOps) / sizeof(Blt_OpSpec);

static int
TextWidgetCmd(
    ClientData clientData,	/* Information about hypertext widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Argument strings. */
{
    HTextCmdProc *proc;
    int result;
    HText *htPtr = clientData;

    proc = Blt_GetOpFromObj(interp, numTextOps, textOps, BLT_OP_ARG1, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    Tcl_Preserve(htPtr);
    result = (*proc) (htPtr, interp, objc, objv);
    Tcl_Release(htPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TextCmd --
 *
 * 	This procedure is invoked to process the "htext" TCL command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
TextCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Argument strings. */
{
    HText *htPtr;
    Tk_Window tkwin;
    int screenWidth, screenHeight;

    if (objc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    htPtr = Blt_AssertCalloc(1, sizeof(HText));
    tkwin = Tk_MainWindow(interp);
    tkwin = Tk_CreateWindowFromPath(interp, tkwin, Tcl_GetString(objv[1]), 
	(char *)NULL);
    if (tkwin == NULL) {
	Blt_Free(htPtr);
	return TCL_ERROR;
    }
    /* Initialize the new hypertext widget */

    Tk_SetClass(tkwin, "BltHtext");
    htPtr->tkwin = tkwin;
    htPtr->display = Tk_Display(tkwin);
    htPtr->interp = interp;
    htPtr->numLines = htPtr->arraySize = 0;
    htPtr->leader = 1;
    htPtr->xScrollUnits = htPtr->yScrollUnits = 10;
    htPtr->numRows = htPtr->numColumns = 0;
    htPtr->selFirst = htPtr->selLast = -1;
    htPtr->selAnchor = 0;
    htPtr->exportSelection = TRUE;
    htPtr->selBW = 2;
    Blt_SizeOfScreen(tkwin, &screenWidth, &screenHeight);
    htPtr->maxWidth = screenWidth;
    htPtr->maxHeight = screenHeight;
    Blt_InitHashTable(&htPtr->widgetTable, BLT_ONE_WORD_KEYS);

    Tk_CreateSelHandler(tkwin, XA_PRIMARY, XA_STRING, TextSelectionProc,
	htPtr, XA_STRING);
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
	TextEventProc, htPtr);
    Blt_SetWindowInstanceData(tkwin, htPtr);

    /*
     *------------------------------------------------------------------------
     *
     *  Create the widget command before configuring the widget. This
     *  is because the "-file" and "-text" options may have embedded
     *  commands that self-reference the widget through the
     *  "$blt_htext(widget)" variable.
     *
     *------------------------------------------------------------------------
     */
    htPtr->cmdToken = Tcl_CreateObjCommand(interp, Tcl_GetString(objv[1]), 
	TextWidgetCmd, htPtr, TextDeleteCmdProc);
    if ((Blt_ConfigureWidgetFromObj(interp, htPtr->tkwin, configSpecs, objc - 2,
		objv + 2, (char *)htPtr, 0) != TCL_OK) ||
	(ConfigureText(interp, htPtr) != TCL_OK)) {
	Tk_DestroyWindow(htPtr->tkwin);
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objv[1]);
    return TCL_OK;
}

int
Blt_HtextCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"htext", TextCmd,};

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#endif /* NO_HTEXT */

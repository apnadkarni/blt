/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTed.c --
 *
 * This module implements an editor for the table geometry manager in the BLT
 * toolkit.
 *
 *	Copyright 1995-2004 George A Howlett.
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

#include "bltAlloc.h"
#include "bltFont.h"
#include "bltText.h"
#include "bltTable.h"
#include "bltOp.h"
#include "bltSwitch.h"
#include "bltInitCmd.h"

typedef struct _TableEditor TableEditor;

#define TABLE_THREAD_KEY	"BLT Table Data"

#define LineWidth(w)	(((w) > 1) ? (w) : 0)

typedef struct {
    Blt_HashTable tableTable;	/* Hash table of table structures keyed by 
				 * the address of the reference Tk window */
} TableData;


typedef struct {
    int flags;
    Tcl_Interp *interp;
    Tk_Window tkwin;		/* Entry window */
    TableEntry *tePtr;		/* Entry it represents */
    Table *tablePtr;		/* Table where it can be found */
    TableEditor *tedPtr;		/* Table editor */
    int mapped;			/* Indicates if the debugging windows are
				 * mapped */
} EntryRep;


typedef struct {
    Blt_Font font;
    XColor *widgetColor;
    XColor *cntlColor;
    XColor *normalFg, *normalBg;
    XColor *activeFg, *activeBg;

    Tk_Cursor cursor;		/* Cursor to display inside of this window */
    Pixmap stipple;

    GC drawGC;			/* GC to draw grid, outlines */
    GC fillGC;			/* GC to fill entry area */
    GC widgetFillGC;		/* GC to fill widget area */
    GC cntlGC;			/* GC to fill rectangles */

} EntryAttributes;

typedef struct {
    int count;
    XRectangle *array;
} Rectangles;

struct _TableEditor {
    int gridLineWidth;		/* Width of grid lines */
    int buttonHeight;		/* Height of row/column buttons */
    int cavityPad;		/* Extra padding to add to entry cavity */
    int min;			/* Minimum size for partitions */

    EditorDrawProc *drawProc;
    EditorDestroyProc *destroyProc;

    Display *display;
    Blt_Font font;
    Table *tablePtr;		/* Pointer to table being debugged */
    Tcl_Interp *interp;
    int flags;
    Tk_Window tkwin;		/* Grid window */
    Tk_Window input;		/* InputOnly window to receive events */
    int inputIsSibling;

    /* Form the grid */
    XSegment *segArr;
    int numSegs;
    XRectangle *padRectArr;
    int numPadRects;
    XRectangle *widgetPadRectArr;
    int numWidgetPadRects;

    XRectangle *cntlRectArr;
    int numCntlRects;

    XRectangle *rects;
    int numRects;

    XRectangle activeRectArr[5];
    int spanActive;

    GC rectGC;			/* GC to fill rectangles */
    GC drawGC;			/* GC to draw grid, outlines */
    GC fillGC;			/* GC to fill window */
    GC spanGC;			/* GC to fill spans */
    GC padRectGC;		/* GC to draw padding  */

    Tk_3DBorder border;		/* Border to use with buttons */
    int relief;
    int borderWidth;		/* Border width of buttons */
    XColor *normalBg;
    XColor *padColor;
    XColor *gridColor;
    XColor *buttonColor;
    XColor *spanColor;

    Pixmap padStipple;
    Pixmap spanStipple;
    Blt_Dashes dashes;
    char *fileName;		/* If non-NULL, indicates name of file
				 * to write final table output to */
    int mapped;			/* Indicates if the debugging windows are
				 * mapped */
    int gripSize;
    int doubleBuffer;
    Tk_Cursor cursor;
    Blt_Chain chain;
    int nextWindowId;

    EntryAttributes attributes;	/* Entry attributes */
};

#define REDRAW_PENDING	(1<<0)	/* A DoWhenIdle handler has already
				 * been queued to redraw the window */
#define LAYOUT_PENDING	(1<<1)

/*  
 * 
 *
 *	|Cavity|1|2|
 *
 *
 */
#define DEF_ENTRY_ACTIVE_BG_MONO	RGB_BLACK
#define DEF_ENTRY_ACTIVE_FG_MONO	RGB_WHITE
#define DEF_ENTRY_ACTIVE_BACKGROUND	RGB_BLACK
#define DEF_ENTRY_ACTIVE_FOREGROUND	RGB_WHITE
#define DEF_ENTRY_CURSOR		(char *)NULL
#define DEF_ENTRY_FONT			"Helvetica 10 Bold"
#define DEF_ENTRY_NORMAL_BACKGROUND	RGB_BLUE
#define DEF_ENTRY_NORMAL_BG_MONO	RGB_BLACK
#define DEF_ENTRY_NORMAL_FOREGROUND	RGB_WHITE
#define DEF_ENTRY_NORMAL_FG_MONO	RGB_WHITE
#define DEF_ENTRY_WIDGET_BACKGROUND	RGB_GREEN
#define DEF_ENTRY_CONTROL_BACKGROUND	RGB_YELLOW
#define DEF_ENTRY_WIDGET_BG_MONO	RGB_BLACK
#define DEF_ENTRY_STIPPLE		"gray50"
#define DEF_GRID_BACKGROUND		RGB_WHITE
#define DEF_GRID_BG_MONO		RGB_WHITE
#define DEF_GRID_CURSOR			"crosshair"
#define DEF_GRID_DASHES			(char *)NULL
#define DEF_GRID_FOREGROUND		RGB_BLACK
#define DEF_GRID_FG_MONO		RGB_BLACK
#define DEF_GRID_FONT			"Helvetica 10 Bold"
#define DEF_GRID_LINE_WIDTH		"1"
#define DEF_GRID_PAD_COLOR		RGB_RED
#define DEF_GRID_PAD_MONO		RGB_BLACK
#define DEF_GRID_PAD_STIPPLE		"gray25"
#define DEF_GRID_PAD_CAVITY		"0"
#define DEF_GRID_PAD_MIN		"8"
#define DEF_ROWCOL_BACKGROUND		RGB_RED
#define DEF_ROWCOL_BG_MONO		RGB_BLACK
#define DEF_ROWCOL_BORDER_COLOR		RGB_RED
#define DEF_ROWCOL_BORDER_MONO		RGB_BLACK
#define DEF_ROWCOL_BORDERWIDTH		"2"
#define DEF_ROWCOL_HEIGHT		"8"
#define DEF_ROWCOL_RELIEF		"raised"
#define DEF_SPAN_STIPPLE		"gray50"
#define DEF_SPAN_COLOR			RGB_BLACK
#define DEF_SPAN_MONO			RGB_BLACK
#define DEF_SPAN_GRIP_SIZE		"5"
#define DEF_GRID_DOUBLE_BUFFER		"1"

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BORDER, "-bg", "tedBorder", (char *)NULL,
	DEF_ROWCOL_BORDER_COLOR, Blt_Offset(TableEditor, border), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_BORDER, "-bg", "tedBorder", (char *)NULL,
	DEF_ROWCOL_BORDER_MONO, Blt_Offset(TableEditor, border), BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-background", "tedBackground", (char *)NULL,
	DEF_GRID_BACKGROUND, Blt_Offset(TableEditor, normalBg), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-background", "tedBackground", (char *)NULL,
	DEF_GRID_BG_MONO, Blt_Offset(TableEditor, normalBg), BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", (char *)NULL,
	DEF_GRID_CURSOR, Blt_Offset(TableEditor, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-gridcolor", "gridColor", (char *)NULL,
	DEF_GRID_FOREGROUND, Blt_Offset(TableEditor, gridColor), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-gridcolor", "gridColor", (char *)NULL,
	DEF_GRID_FG_MONO, Blt_Offset(TableEditor, gridColor), BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-buttoncolor", "buttonColor", (char *)NULL,
	DEF_ROWCOL_BACKGROUND, Blt_Offset(TableEditor, buttonColor), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-buttoncolor", "buttonColor", (char *)NULL,
	DEF_ROWCOL_BG_MONO, Blt_Offset(TableEditor, buttonColor), BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-padcolor", "padColor", (char *)NULL,
	DEF_GRID_PAD_COLOR, Blt_Offset(TableEditor, padColor), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-padcolor", "padColor", (char *)NULL,
	DEF_GRID_PAD_MONO, Blt_Offset(TableEditor, padColor), BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_BITMAP, "-padstipple", "padStipple", (char *)NULL,
	DEF_GRID_PAD_STIPPLE, Blt_Offset(TableEditor, padStipple), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FONT, "-font", "font", (char *)NULL,
	DEF_GRID_FONT, Blt_Offset(TableEditor, font), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-gridlinewidth", "gridLineWidth", (char *)NULL,
	DEF_GRID_LINE_WIDTH, Blt_Offset(TableEditor, gridLineWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-buttonheight", "buttonHeight", (char *)NULL,
	DEF_ROWCOL_HEIGHT, Blt_Offset(TableEditor, buttonHeight),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-cavitypad", "cavityPad", (char *)NULL,
	DEF_GRID_PAD_CAVITY, Blt_Offset(TableEditor, cavityPad),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-minsize", "minSize", (char *)NULL,
	DEF_GRID_PAD_MIN, Blt_Offset(TableEditor, min), 
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_DASHES, "-dashes", "dashes", (char *)NULL,
	DEF_GRID_DASHES, Blt_Offset(TableEditor, dashes), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-relief", "relief", (char *)NULL,
	DEF_ROWCOL_RELIEF, Blt_Offset(TableEditor, relief), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", (char *)NULL,
	DEF_ROWCOL_BORDERWIDTH, Blt_Offset(TableEditor, borderWidth),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CURSOR, "-entrycursor", "entryCursor", (char *)NULL,
	DEF_ENTRY_CURSOR, Blt_Offset(TableEditor, attributes.cursor),
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_FONT, "-entryfont", "entryFont", (char *)NULL,
	DEF_ENTRY_FONT, Blt_Offset(TableEditor, attributes.font), 0},
    {BLT_CONFIG_BITMAP, "-entrystipple", "entryStipple", (char *)NULL,
	DEF_ENTRY_STIPPLE, Blt_Offset(TableEditor, attributes.stipple),
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-widgetbackground", "widgetBackground", (char *)NULL,
	DEF_ENTRY_WIDGET_BACKGROUND, Blt_Offset(TableEditor, attributes.widgetColor),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-widgetbackground", "widgetBackground", (char *)NULL,
	DEF_ENTRY_WIDGET_BG_MONO, Blt_Offset(TableEditor, attributes.widgetColor),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-controlbackground", "controlBackground", (char *)NULL,
	DEF_ENTRY_CONTROL_BACKGROUND, Blt_Offset(TableEditor, attributes.cntlColor),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-controlbackground", "controlBackground", (char *)NULL,
	DEF_ENTRY_WIDGET_BG_MONO, Blt_Offset(TableEditor, attributes.cntlColor),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-entrybackground", "entryBackground", (char *)NULL,
	DEF_ENTRY_NORMAL_BACKGROUND, Blt_Offset(TableEditor, attributes.normalBg),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-entrybackground", "entryBackground", (char *)NULL,
	DEF_ENTRY_NORMAL_BG_MONO, Blt_Offset(TableEditor, attributes.normalBg),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-entryactivebackground", "entryActiveBackground", (char *)NULL,
	DEF_ENTRY_ACTIVE_BACKGROUND, Blt_Offset(TableEditor, attributes.activeBg),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-entryactivebackground", "entryActiveBackground", (char *)NULL,
	DEF_ENTRY_ACTIVE_BG_MONO, Blt_Offset(TableEditor, attributes.activeBg),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-entryactiveforeground", "entryActiveForeground", (char *)NULL,
	DEF_ENTRY_ACTIVE_FOREGROUND, Blt_Offset(TableEditor, attributes.activeFg),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-entryactiveforeground", "entryActiveForeground", (char *)NULL,
	DEF_ENTRY_ACTIVE_FG_MONO, Blt_Offset(TableEditor, attributes.activeFg),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-entryforeground", "entryForeground", (char *)NULL,
	DEF_ENTRY_NORMAL_FOREGROUND, Blt_Offset(TableEditor, attributes.normalFg),
	BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-entryforeground", "entryForeground", (char *)NULL,
	DEF_ENTRY_NORMAL_FG_MONO, Blt_Offset(TableEditor, attributes.normalFg),
	BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_COLOR, "-spancolor", "spanColor", (char *)NULL,
	DEF_SPAN_COLOR, Blt_Offset(TableEditor, spanColor), BLT_CONFIG_COLOR_ONLY},
    {BLT_CONFIG_COLOR, "-spancolor", "spanColor", (char *)NULL,
	DEF_SPAN_MONO, Blt_Offset(TableEditor, spanColor), BLT_CONFIG_MONO_ONLY},
    {BLT_CONFIG_BITMAP, "-spanstipple", "spanStipple", (char *)NULL,
	DEF_SPAN_STIPPLE, Blt_Offset(TableEditor, spanStipple), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-gripsize", "gripSize", (char *)NULL,
	DEF_SPAN_GRIP_SIZE, Blt_Offset(TableEditor, gripSize),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_BOOLEAN, "-dbl", "doubleBuffer", (char *)NULL,
	DEF_GRID_DOUBLE_BUFFER, Blt_Offset(TableEditor, doubleBuffer),
	BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_END, NULL, NULL, NULL, NULL, 0, 0}
};


static Tcl_FreeProc DestroyEditor;
static Tcl_FreeProc DestroyTableEditor;
static Tcl_IdleProc DisplayEntry;
static Tcl_IdleProc DisplayTableEditor;
static Tcl_ObjCmdProc TedCmd;
#ifdef notdef
static Tk_EventProc EntryEventProc;
static Tcl_FreeProc DestroyEntry;
#endif
static Tk_EventProc TableEditorEventProc;

static void DrawEditor(Editor *editor);

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
EventuallyRedraw(TableEditor *tedPtr)
{
    if ((tedPtr->tkwin != NULL) && !(tedPtr->flags & REDRAW_PENDING)) {
	tedPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayTableEditor, tedPtr);
    }
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedrawEntry --
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
EventuallyRedrawEntry(EntryRep *repPtr)
{
    if ((repPtr->tkwin != NULL) && !(repPtr->flags & REDRAW_PENDING)) {
	repPtr->flags |= REDRAW_PENDING;
	Tcl_DoWhenIdle(DisplayEntry, repPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * EntryEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various
 * 	events on the editing grid for the table.
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
EntryEventProc(ClientData clientData, XEvent *eventPtr)
{
    EntryRep *repPtr = (EntryRep *)clientData;

    if (eventPtr->type == ConfigureNotify) {
	EventuallyRedrawEntry(repPtr);
    } else if (eventPtr->type == Expose) {
	if (eventPtr->xexpose.count == 0) {
	    EventuallyRedrawEntry(repPtr);
	}
    } else if (eventPtr->type == DestroyNotify) {
	repPtr->tkwin = NULL;
	if (repPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayEntry, repPtr);
	}
	Tcl_EventuallyFree(repPtr, DestroyEntry);
    }
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * TableEditorEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various
 * 	events on the editing grid for the table.
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
TableEditorEventProc(ClientData clientData, XEvent *eventPtr)
{
    TableEditor *tedPtr = (TableEditor *) clientData;

    if (eventPtr->type == ConfigureNotify) {
	EventuallyRedraw(tedPtr);
    } else if (eventPtr->type == Expose) {
	if (eventPtr->xexpose.count == 0) {
	    EventuallyRedraw(tedPtr);
	}
    } else if (eventPtr->type == DestroyNotify) {
	tedPtr->tkwin = NULL;
	if (tedPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayTableEditor, tedPtr);
	}
	Tcl_EventuallyFree(tedPtr, DestroyTableEditor);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateGrid --
 *
 *---------------------------------------------------------------------------
 */
static int
CreateGrid(TableEditor *tedPtr)
{
    Tcl_Interp *interp;
    Tk_Window tkwin;
    Tk_Window master;
    /*
     * Create a sibling window to cover the master window. It will
     * be stacked just above the master window.
     */
    interp = tedPtr->tablePtr->interp;
    master = tedPtr->tablePtr->tkwin;
    tkwin = Tk_CreateWindow(interp, master, "ted_%output%", (char *)NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    Tk_SetClass(tkwin, "BltTableEditor");
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
	TableEditorEventProc, tedPtr);
    Tk_MoveResizeWindow(tkwin, 0, 0, Tk_Width(master), Tk_Height(master));
    Tk_RestackWindow(tkwin, Below, (Tk_Window)NULL);
    Tk_MapWindow(tkwin);
    tedPtr->tkwin = tkwin;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateEventWindow --
 *
 *---------------------------------------------------------------------------
 */
static int
CreateEventWindow(TableEditor *tedPtr)
{
    Tcl_Interp *interp;
    Tk_Window tkwin;
    Tk_Window master;
    Tk_Window parent;

    interp = tedPtr->tablePtr->interp;
    master = tedPtr->tablePtr->tkwin;
    /*
     * Create an InputOnly window which sits above the table to
     * collect and dispatch user events.
     */
    if (Tk_IsTopLevel(master)) {
	/*
	 * If master is a top-level window, it's also the parent of
	 * the widgets (it can't have a sibling).
	 * In this case, the InputOnly window is a child of the
	 * master instead of a sibling.
	 */
	parent = master;
	tkwin = Tk_CreateWindow(interp, parent, "ted_%input%", (char *)NULL);
	if (tkwin != NULL) {
	    Tk_ResizeWindow(tkwin, Tk_Width(parent), Tk_Height(parent));
	}
	tedPtr->inputIsSibling = 0;
    } else {
	char *namePtr;		/* Name of InputOnly window. */

	parent = Tk_Parent(master);
	namePtr = Blt_AssertMalloc(strlen(Tk_Name(master)) + 5);
	Blt_FormatString(namePtr, strlen(Tk_Name(master)) + 5, "ted_%s", 
		 Tk_Name(master));
	tkwin = Tk_CreateWindow(interp, parent, namePtr, (char *)NULL);
	Blt_Free(namePtr);
	if (tkwin != NULL) {
	    Tk_MoveResizeWindow(tkwin, Tk_X(master), Tk_Y(master),
		Tk_Width(master), Tk_Height(master));
	}
	tedPtr->inputIsSibling = 1;
    }
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    Blt_MakeTransparentWindowExist(tkwin, Tk_WindowId(parent), TRUE);
    Tk_RestackWindow(tkwin, Above, (Tk_Window)NULL);
    Tk_MapWindow(tkwin);
    tedPtr->input = tkwin;
    return TCL_OK;
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * CreateEntry --
 *
 *---------------------------------------------------------------------------
 */
static int
CreateEntry(TableEditor *tedPtr, TableEntry *tePtr)
{
    Tk_Window tkwin, master;
    char string[200];
    EntryRep *repPtr;

    repPtr = Blt_AssertCalloc(1, sizeof(EntryRep));
    repPtr->tablePtr = tedPtr->tablePtr;
    repPtr->tedPtr = tedPtr;
    repPtr->interp = tedPtr->interp;
    repPtr->tePtr = tePtr;
    repPtr->mapped = 0;

    /*
     * Create a sibling window to cover the master window. It will
     * be stacked just above the master window.
     */

    master = tedPtr->tablePtr->tkwin;
    Blt_FormatString(string, 200, "bltTableEditor%d", tedPtr->nextWindowId);
    tedPtr->nextWindowId++;
    tkwin = Tk_CreateWindow(tedPtr->interp, master, string, (char *)NULL);
    if (tkwin == NULL) {
	Blt_Free(repPtr);
	return TCL_ERROR;
    }
    Tk_SetClass(tkwin, "BltTableEditor");
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
	EntryEventProc, repPtr);
    repPtr->tkwin = tkwin;
    Blt_Chain_Append(tedPtr->chain, repPtr);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * DestroyEntry --
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyEntry(DestroyData data)
{
    EntryRep *repPtr = (EntryRep *)data;
    Blt_ChainLink link;
    TableEntry *tePtr;

    for (link = Blt_Chain_FirstLink(repPtr->tedPtr->chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	tePtr = Blt_Chain_GetValue(link);
	if (tePtr == repPtr->tePtr) {
	    Blt_Chain_DeleteLink(repPtr->tedPtr->chain, link);
	    Blt_Free(repPtr);
	    return;
	}
    }
}
#endif


/*
 *---------------------------------------------------------------------------
 *
 * DisplayEntry --
 *
 *---------------------------------------------------------------------------
 */
static void
DisplayEntry(ClientData clientData)
{
    EntryRep *repPtr = (EntryRep *) clientData;
    TableEditor *tedPtr;
    TableEntry *tePtr;
    Tk_Window tkwin;
    int x, y, width, height;

    repPtr->flags &= ~REDRAW_PENDING;
    if ((repPtr->tkwin == NULL) || (repPtr->tePtr == NULL)) {
	return;
    }
    if (!Tk_IsMapped(repPtr->tkwin)) {
	return;
    }
    tedPtr = repPtr->tedPtr;
    tePtr = repPtr->tePtr;
    tkwin = repPtr->tkwin;

    /*
     * Check if the entry size and position.
     * Move and resize the window accordingly.
     */
    x = Tk_X(tePtr->tkwin) - (tePtr->padLeft + tedPtr->cavityPad);
    y = Tk_Y(tePtr->tkwin) - (tePtr->padTop + tedPtr->cavityPad);
    width = Tk_Width(tePtr->tkwin) + PADDING(tePtr->xPad) +
	(2 * tedPtr->cavityPad);
    height = Tk_Height(tePtr->tkwin) + PADDING(tePtr->yPad) +
	(2 * tedPtr->cavityPad);


    if ((Tk_X(tkwin) != x) || (Tk_Y(tkwin) != y) || 
	(Tk_Width(tkwin) != width) || (Tk_Height(tkwin) != height)) {
	Tk_MoveResizeWindow(tkwin, x, y, width, height);
	Tk_RestackWindow(tkwin, Above, (Tk_Window)NULL);
    }
    /* Clear the background of the entry */

    XFillRectangle(Tk_Display(tkwin), Tk_WindowId(tkwin),
	tedPtr->attributes.fillGC, 0, 0, width, height);

    /* Draw the window */

    x = tePtr->padLeft + tedPtr->cavityPad;
    y = tePtr->padTop + tedPtr->cavityPad;

    XFillRectangle(Tk_Display(tkwin), Tk_WindowId(tkwin),
	tedPtr->attributes.widgetFillGC, x, y, Tk_Width(tePtr->tkwin),
	Tk_Height(tePtr->tkwin));
    XDrawRectangle(Tk_Display(tkwin), Tk_WindowId(tkwin),
	tedPtr->attributes.drawGC, x, y, Tk_Width(tePtr->tkwin),
	Tk_Height(tePtr->tkwin));
}

/*
 *---------------------------------------------------------------------------
 *
 * FindEditor --
 *
 *	Searches for a table associated with the window given by its
 *	pathname.  This window represents the master window of the table.
 *
 *	Errors may occur because
 *	  1) pathName does not represent a valid Tk window or
 *	  2) the window is not associated with any table as its master.
 *
 * Results:
 *	If a table entry exists, a pointer to the Table structure is
 *	returned. Otherwise NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static TableEditor *
FindEditor(ClientData clientData, Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    Table *tablePtr;

    if (Blt_GetTableFromObj(clientData, interp, objPtr, &tablePtr) != TCL_OK) {
	return NULL;
    }
    if (tablePtr->editPtr == NULL) {
	Tcl_AppendResult(interp, "no editor exists for table \"",
	    Tk_PathName(tablePtr->tkwin), "\"", (char *)NULL);
	return NULL;
    }
    return (TableEditor *) tablePtr->editPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * CreateTableEditor --
 *
 *---------------------------------------------------------------------------
 */
static TableEditor *
CreateTableEditor(Table *tablePtr, Tcl_Interp *interp)
{
    TableEditor *tedPtr;

    tedPtr = Blt_AssertCalloc(1, sizeof(TableEditor));
    tedPtr->nextWindowId = 0;
    tedPtr->interp = interp;
    tedPtr->tablePtr = tablePtr;
    tedPtr->gridLineWidth = 1;
    tedPtr->buttonHeight = 0;
    tedPtr->cavityPad = 0;
    tedPtr->min = 3;
    tedPtr->gripSize = 5;
    tedPtr->drawProc = DrawEditor;
    tedPtr->destroyProc = DestroyEditor;
    tedPtr->display = Tk_Display(tablePtr->tkwin);
    tedPtr->relief = TK_RELIEF_RAISED;
    tedPtr->borderWidth = 2;
    tedPtr->doubleBuffer = 1;
    tedPtr->chain = Blt_Chain_Create();
    /* Create the grid window */

    if (CreateGrid(tedPtr) != TCL_OK) {
	return NULL;
    }
    /* Create an InputOnly window to collect user events */
    if (CreateEventWindow(tedPtr) != TCL_OK) {
	return NULL;
    }
    tablePtr->editPtr = (Editor *)tedPtr;
    return tedPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyTableEditor --
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyTableEditor(DestroyData freeProcData)
{
    TableEditor *tedPtr = (TableEditor *) freeProcData;

    if (tedPtr->rects != NULL) {
	Blt_Free(tedPtr->rects);
    }
    if (tedPtr->segArr != NULL) {
	Blt_Free(tedPtr->segArr);
    }
    if (tedPtr->fillGC != NULL) {
	Tk_FreeGC(tedPtr->display, tedPtr->fillGC);
    }
    if (tedPtr->drawGC != NULL) {
	Blt_FreePrivateGC(tedPtr->display, tedPtr->drawGC);
    }
    if (tedPtr->rectGC != NULL) {
	Tk_FreeGC(tedPtr->display, tedPtr->rectGC);
    }
    if (tedPtr->padRectGC != NULL) {
	Tk_FreeGC(tedPtr->display, tedPtr->padRectGC);
    }
    /* Is this save ? */
    tedPtr->tablePtr->editPtr = NULL;
    Blt_Free(tedPtr);

}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureTableEditor --
 *
 *	This procedure is called to process an objv/objc list in order to
 *	configure the table geometry manager.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Table configuration options (padx, pady, rows, columns, etc) get
 *	set.   The table is recalculated and arranged at the next idle
 *	point.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureTableEditor(TableEditor *tedPtr, int objc, Tcl_Obj *const *objv,
		     int flags)
{
    XGCValues gcValues;
    GC newGC;
    unsigned long gcMask;

    if (Blt_ConfigureWidgetFromObj(tedPtr->interp, tedPtr->tkwin, configSpecs,
	    objc, objv, (char *)tedPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    /* GC for filling background of edit window */

    gcMask = GCForeground;
    gcValues.foreground = tedPtr->normalBg->pixel;
    newGC = Tk_GetGC(tedPtr->tkwin, gcMask, &gcValues);
    if (tedPtr->fillGC != NULL) {
	Tk_FreeGC(tedPtr->display, tedPtr->fillGC);
    }
    tedPtr->fillGC = newGC;

    /* GC for drawing grid lines */

    gcMask = (GCForeground | GCBackground | GCLineWidth | GCLineStyle |
	GCCapStyle | GCJoinStyle | GCFont);
    gcValues.font = Blt_Font_Id(tedPtr->font);
    gcValues.foreground = tedPtr->gridColor->pixel;
    gcValues.background = tedPtr->normalBg->pixel;
    gcValues.line_width = LineWidth(tedPtr->gridLineWidth);
    gcValues.cap_style = CapRound;
    gcValues.join_style = JoinRound;
    gcValues.line_style = LineSolid;
    if (LineIsDashed(tedPtr->dashes)) {
	gcValues.line_style = LineOnOffDash;
    }
    newGC = Blt_GetPrivateGC(tedPtr->tkwin, gcMask, &gcValues);
    if (tedPtr->drawGC != NULL) {
	Blt_FreePrivateGC(tedPtr->display, tedPtr->drawGC);
    }
    if (LineIsDashed(tedPtr->dashes)) {
	XSetDashes(tedPtr->display, newGC, 0, 
		   (const char *)tedPtr->dashes.values,
		   strlen((char *)tedPtr->dashes.values));
    }
    tedPtr->drawGC = newGC;

    /* GC for button rectangles */

    gcMask = GCForeground;
    gcValues.foreground = tedPtr->buttonColor->pixel;
    newGC = Tk_GetGC(tedPtr->tkwin, gcMask, &gcValues);
    if (tedPtr->rectGC != NULL) {
	Tk_FreeGC(tedPtr->display, tedPtr->rectGC);
    }
    tedPtr->rectGC = newGC;

    /* GC for button rectangles */

    gcMask = GCForeground;
    gcValues.foreground = tedPtr->padColor->pixel;
    if (tedPtr->padStipple != None) {
	gcMask |= GCStipple | GCFillStyle;
	gcValues.stipple = tedPtr->padStipple;
	gcValues.fill_style = FillStippled;
    }
    newGC = Tk_GetGC(tedPtr->tkwin, gcMask, &gcValues);
    if (tedPtr->padRectGC != NULL) {
	Tk_FreeGC(tedPtr->display, tedPtr->padRectGC);
    }
    tedPtr->padRectGC = newGC;

    /* GC for filling entrys */

    gcMask = GCForeground;
    gcValues.foreground = tedPtr->attributes.normalBg->pixel;
    if (tedPtr->attributes.stipple != None) {
	gcMask |= GCStipple | GCFillStyle;
	gcValues.stipple = tedPtr->attributes.stipple;
	gcValues.fill_style = FillStippled;
    }
    newGC = Tk_GetGC(tedPtr->tkwin, gcMask, &gcValues);
    if (tedPtr->attributes.fillGC != NULL) {
	Tk_FreeGC(tedPtr->display, tedPtr->attributes.fillGC);
    }
    tedPtr->attributes.fillGC = newGC;

    /* GC for drawing entrys */

    gcMask = GCForeground | GCBackground | GCFont;
    gcValues.foreground = tedPtr->attributes.normalFg->pixel;
    gcValues.background = tedPtr->attributes.normalBg->pixel;
    gcValues.font = Blt_Font_Id(tedPtr->attributes.font);
    newGC = Tk_GetGC(tedPtr->tkwin, gcMask, &gcValues);
    if (tedPtr->attributes.drawGC != NULL) {
	Blt_FreePrivateGC(tedPtr->display, tedPtr->attributes.drawGC);
    }
    tedPtr->attributes.drawGC = newGC;

    /* GC for filling widget rectangles */

    gcMask = GCForeground;
    gcValues.foreground = tedPtr->attributes.widgetColor->pixel;
    newGC = Tk_GetGC(tedPtr->tkwin, gcMask, &gcValues);
    if (tedPtr->attributes.widgetFillGC != NULL) {
	Tk_FreeGC(tedPtr->display, tedPtr->attributes.widgetFillGC);
    }
    tedPtr->attributes.widgetFillGC = newGC;

    gcMask = GCForeground;
    gcValues.foreground = tedPtr->attributes.cntlColor->pixel;
    newGC = Tk_GetGC(tedPtr->tkwin, gcMask, &gcValues);
    if (tedPtr->attributes.cntlGC != NULL) {
	Tk_FreeGC(tedPtr->display, tedPtr->attributes.cntlGC);
    }
    tedPtr->attributes.cntlGC = newGC;

    /* GC for filling span rectangle */

    gcMask = GCForeground;
    gcValues.foreground = tedPtr->spanColor->pixel;
    if (tedPtr->spanStipple != None) {
	gcMask |= GCStipple | GCFillStyle;
	gcValues.stipple = tedPtr->spanStipple;
	gcValues.fill_style = FillStippled;
    }
    newGC = Tk_GetGC(tedPtr->tkwin, gcMask, &gcValues);
    if (tedPtr->spanGC != NULL) {
	Tk_FreeGC(tedPtr->display, tedPtr->spanGC);
    }
    tedPtr->spanGC = newGC;

    /* Define cursor for grid events */
    if (tedPtr->cursor != None) {
	Tk_DefineCursor(tedPtr->input, tedPtr->cursor);
    } else {
	Tk_UndefineCursor(tedPtr->input);
    }
    return TCL_OK;
}


static void
LayoutGrid(TableEditor *tedPtr)
{
    int needed;
    XSegment *segArr;
    Table *tablePtr;
    Blt_ChainLink link;
    RowColumn *rcPtr;
    int startX, endX;
    int startY, endY;
    int count;

    tablePtr = tedPtr->tablePtr;
    if (tedPtr->segArr != NULL) {
	Blt_Free(tedPtr->segArr);
	tedPtr->segArr = NULL;
    }
    tedPtr->numSegs = 0;
    if ((tablePtr->numRows == 0) || (tablePtr->numColumns == 0)) {
	return;
    }
    needed = tablePtr->numRows + tablePtr->numColumns + 2;
    segArr = Blt_Calloc(needed, sizeof(XSegment));
    if (segArr == NULL) {
	return;
    }
    link = Blt_Chain_FirstLink(tablePtr->cols.chain);
    rcPtr = Blt_Chain_GetValue(link);
    startX = rcPtr->offset - tedPtr->gridLineWidth;

    link = Blt_Chain_LastLink(tablePtr->cols.chain);
    rcPtr = Blt_Chain_GetValue(link);
    endX = rcPtr->offset + rcPtr->size - 1;

    link = Blt_Chain_FirstLink(tablePtr->rows.chain);
    rcPtr = Blt_Chain_GetValue(link);
    startY = rcPtr->offset - tedPtr->gridLineWidth;

    link = Blt_Chain_LastLink(tablePtr->rows.chain);
    rcPtr = Blt_Chain_GetValue(link);
    endY = rcPtr->offset + rcPtr->size - 1;

    count = 0;			/* Reset segment counter */

    for (link = Blt_Chain_FirstLink(tablePtr->rows.chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	segArr[count].x1 = startX;
	segArr[count].x2 = endX;
	segArr[count].y1 = segArr[count].y2 = rcPtr->offset - 
	    tedPtr->gridLineWidth;
	count++;
    }
    segArr[count].x1 = startX;
    segArr[count].x2 = endX;
    segArr[count].y1 = segArr[count].y2 = endY;
    count++;

    for (link = Blt_Chain_FirstLink(tablePtr->cols.chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	segArr[count].y1 = startY;
	segArr[count].y2 = endY;
	segArr[count].x1 = segArr[count].x2 = rcPtr->offset - 
	    tedPtr->gridLineWidth;
	count++;
    }
    segArr[count].x1 = segArr[count].x2 = endX;
    segArr[count].y1 = startY;
    segArr[count].y2 = endY;
    count++;
    assert(count == needed);
    if (tedPtr->segArr != NULL) {
	Blt_Free(tedPtr->segArr);
    }
    tedPtr->segArr = segArr;
    tedPtr->numSegs = count;
}


static void
LayoutPads(TableEditor *tedPtr)
{
    int needed;
    XRectangle *rects, *rectPtr;
    Table *tablePtr;
    Blt_ChainLink link;
    RowColumn *rcPtr;
    int startX, endX;
    int startY, endY;
    int count;

    tablePtr = tedPtr->tablePtr;
    if (tedPtr->padRectArr != NULL) {
	Blt_Free(tedPtr->padRectArr);
	tedPtr->padRectArr = NULL;
    }
    tedPtr->numPadRects = 0;
    if ((tablePtr->numRows == 0) || (tablePtr->numColumns == 0)) {
	return;
    }
    needed = 2 * (tablePtr->numRows + tablePtr->numColumns);
    rects = Blt_Calloc(needed, sizeof(XRectangle));
    if (rects == NULL) {
	return;
    }
    link = Blt_Chain_FirstLink(tablePtr->cols.chain);
    rcPtr = Blt_Chain_GetValue(link);
    startX = rcPtr->offset;

    link = Blt_Chain_LastLink(tablePtr->cols.chain);
    rcPtr = Blt_Chain_GetValue(link);
    endX = (rcPtr->offset + rcPtr->size);

    link = Blt_Chain_FirstLink(tablePtr->rows.chain);
    rcPtr = Blt_Chain_GetValue(link);
    startY = rcPtr->offset;

    link = Blt_Chain_LastLink(tablePtr->rows.chain);
    rcPtr = Blt_Chain_GetValue(link);
    endY = (rcPtr->offset + rcPtr->size);

    count = 0;			/* Reset segment counter */
    rectPtr = rects;
    for (link = Blt_Chain_FirstLink(tablePtr->rows.chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	if (rcPtr->pad.side1 > 0) {
	    rectPtr->x = startX;
	    rectPtr->y = rcPtr->offset;
	    rectPtr->height = rcPtr->pad.side1;
	    rectPtr->width = endX - startX - 1;
	    rectPtr++, count++;
	}
	if (rcPtr->pad.side2 > 0) {
	    rectPtr->x = startX;
	    rectPtr->y = rcPtr->offset + rcPtr->size - rcPtr->pad.side2 - 1;
	    rectPtr->height = rcPtr->pad.side2;
	    rectPtr->width = endX - startX - 1;
	    rectPtr++, count++;
	}
    }
    for (link = Blt_Chain_FirstLink(tablePtr->cols.chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	if (rcPtr->pad.side1 > 0) {
	    rectPtr->x = rcPtr->offset;
	    rectPtr->y = startY;
	    rectPtr->height = endY - startY - 1;
	    rectPtr->width = rcPtr->pad.side1;
	    rectPtr++, count++;
	}
	if (rcPtr->pad.side2 > 0) {
	    rectPtr->x = rcPtr->offset + rcPtr->size - rcPtr->pad.side2;
	    rectPtr->y = startY;
	    rectPtr->height = endY - startY - 1;
	    rectPtr->width = rcPtr->pad.side2;
	    rectPtr++, count++;
	}
    }
    if (count == 0) {
	Blt_Free(rects);
	return;
    }
    tedPtr->padRectArr = rects;
    tedPtr->numPadRects = count;
}

static void
LayoutEntries(TableEditor *tedPtr)
{
    TableEntry *tePtr;
    XRectangle *rects;
    int needed;
    int count;
    Blt_ChainLink link;

    if (tedPtr->widgetPadRectArr != NULL) {
	Blt_Free(tedPtr->widgetPadRectArr);
	tedPtr->widgetPadRectArr = NULL;
    }
    tedPtr->numWidgetPadRects = 0;

    needed = Blt_Chain_GetLength(tedPtr->tablePtr->chain);
    rects = Blt_Calloc(needed, sizeof(XRectangle));
    if (rects == NULL) {
	return;
    }
    /* Draw any entry windows */
    count = 0;
    for (link = Blt_Chain_FirstLink(tedPtr->tablePtr->chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	tePtr = Blt_Chain_GetValue(link);
	if ((PADDING(tePtr->xPad) + PADDING(tePtr->yPad)) == 0) {
	    continue;
	}
	rects[count].x = Tk_X(tePtr->tkwin) - tePtr->padLeft;
	rects[count].y = Tk_Y(tePtr->tkwin) - tePtr->padTop;
	rects[count].width = Tk_Width(tePtr->tkwin) +
	    PADDING(tePtr->xPad);
	rects[count].height = Tk_Height(tePtr->tkwin) +
	    PADDING(tePtr->yPad);
	count++;
    }
    if (count == 0) {
	Blt_Free(rects);
	return;
    }
    tedPtr->widgetPadRectArr = rects;
    tedPtr->numWidgetPadRects = count;
}

static void
LayoutControlEntries(TableEditor *tedPtr)
{
    TableEntry *tePtr;
    XRectangle *rects;
    int needed;
    int count;
    Table *tablePtr = tedPtr->tablePtr;
    Blt_ChainLink link;
    RowColumn *rcPtr;

    if (tedPtr->cntlRectArr != NULL) {
	Blt_Free(tedPtr->cntlRectArr);
	tedPtr->cntlRectArr = NULL;
    }
    tedPtr->numCntlRects = 0;

    needed = (tablePtr->numRows + tablePtr->numColumns);
    rects = Blt_Calloc(needed, sizeof(XRectangle));
    if (rects == NULL) {
	return;
    }
    /* Draw any entry windows */
    count = 0;
    for (link = Blt_Chain_FirstLink(tablePtr->cols.chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	tePtr = rcPtr->control;
	if (tePtr != NULL) {
	    rects[count].x = Tk_X(tePtr->tkwin) - tePtr->padLeft;
	    rects[count].y = Tk_Y(tePtr->tkwin) - tePtr->padTop;
	    rects[count].width = Tk_Width(tePtr->tkwin) +
		PADDING(tePtr->xPad);
	    rects[count].height = Tk_Height(tePtr->tkwin) +
		PADDING(tePtr->yPad);
	    count++;
	}
    }
    for (link = Blt_Chain_FirstLink(tablePtr->rows.chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	tePtr = rcPtr->control;
	if (tePtr != NULL) {
	    rects[count].x = Tk_X(tePtr->tkwin) - tePtr->padLeft;
	    rects[count].y = Tk_Y(tePtr->tkwin) - tePtr->padTop;
	    rects[count].width = Tk_Width(tePtr->tkwin) +
		PADDING(tePtr->xPad);
	    rects[count].height = Tk_Height(tePtr->tkwin) +
		PADDING(tePtr->yPad);
	    count++;
	}
    }
    if (count == 0) {
	Blt_Free(rects);
	return;
    }
    tedPtr->cntlRectArr = rects;
    tedPtr->numCntlRects = count;
}

static void
LayoutButtons(TableEditor *tedPtr)
{
    int needed;
    XRectangle *rects;
    Table *tablePtr;
    Blt_ChainLink link;
    RowColumn *rcPtr;
    int count;

    tablePtr = tedPtr->tablePtr;
    if ((tablePtr->numRows == 0) || (tablePtr->numColumns == 0)) {
	if (tedPtr->rects != NULL) {
	    Blt_Free(tedPtr->rects);
	}
	tedPtr->rects = NULL;
	tedPtr->numRects = 0;
	return;			/* Nothing to display, empty table */
    }
    needed = 2 * (tablePtr->numRows + tablePtr->numColumns);
    rects = Blt_Calloc(needed, sizeof(XRectangle));
    if (rects == NULL) {
	return;			/* Can't allocate rectangles */
    }
    count = 0;
    for (link = Blt_Chain_FirstLink(tablePtr->rows.chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	rects[count].x = 0;
	rects[count].y = rcPtr->offset - rcPtr->pad.side1;
	rects[count].width = tedPtr->buttonHeight;
	rects[count].height = rcPtr->size - 2;
	count++;
	rects[count].x = Tk_Width(tedPtr->tkwin) - tedPtr->buttonHeight;
	rects[count].y = rcPtr->offset - rcPtr->pad.side1;
	rects[count].width = tedPtr->buttonHeight;
	rects[count].height = rcPtr->size - 2;
	count++;
    }
    for (link = Blt_Chain_FirstLink(tablePtr->cols.chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	rcPtr = Blt_Chain_GetValue(link);
	rects[count].x = rcPtr->offset - rcPtr->pad.side1;
	rects[count].y = 0;
	rects[count].width = rcPtr->size - 2;
	rects[count].height = tedPtr->buttonHeight;
	count++;
	rects[count].x = rcPtr->offset - rcPtr->pad.side1;
	rects[count].y = Tk_Height(tedPtr->tkwin) - tedPtr->buttonHeight;
	rects[count].width = rcPtr->size - 2;
	rects[count].height = tedPtr->buttonHeight;
	count++;
    }
    assert(count == needed);
    if (tedPtr->rects != NULL) {
	Blt_Free(tedPtr->rects);
    }
    tedPtr->rects = rects;
    tedPtr->numRects = count;
}


static void
DisplayTableEditor(ClientData clientData)
{
    TableEditor *tedPtr = (TableEditor *) clientData;
    Tk_Window master;
    Tk_Window tkwin;
    Blt_ChainLink link;
    EntryRep *repPtr;
    Drawable drawable;
    Pixmap pixmap;

#ifdef notdef
    fprintf(stderr, "display grid\n");
#endif
    tedPtr->flags &= ~REDRAW_PENDING;
    if (!Tk_IsMapped(tedPtr->tkwin)) {
	return;
    }
    /*
     * Check if the master window has changed size and resize the
     * grid and input windows accordingly.
     */
    master = tedPtr->tablePtr->tkwin;
    if ((Tk_Width(master) != Tk_Width(tedPtr->tkwin)) ||
	(Tk_Height(master) != Tk_Height(tedPtr->tkwin))) {
#ifdef notdef
	fprintf(stderr, "resizing windows\n");
#endif
	Tk_ResizeWindow(tedPtr->tkwin, Tk_Width(master), Tk_Height(master));
	Tk_ResizeWindow(tedPtr->input, Tk_Width(master), Tk_Height(master));
	if (tedPtr->inputIsSibling) {
	    Tk_MoveWindow(tedPtr->input, Tk_X(master), Tk_X(master));
	}
	tedPtr->flags |= LAYOUT_PENDING;
    }
    if (tedPtr->flags & LAYOUT_PENDING) {
#ifdef notdef
	fprintf(stderr, "layout of grid\n");
#endif
	LayoutPads(tedPtr);
	LayoutEntries(tedPtr);
	LayoutControlEntries(tedPtr);
	LayoutGrid(tedPtr);
	LayoutButtons(tedPtr);
	tedPtr->flags &= ~LAYOUT_PENDING;
    }
    tkwin = tedPtr->tkwin;

    pixmap = None;		/* Suppress compiler warning. */
    drawable = Tk_WindowId(tkwin);
    if (tedPtr->doubleBuffer) {
	/* Create an off-screen pixmap for semi-smooth scrolling. */
	pixmap = Blt_GetPixmap(tedPtr->display, Tk_WindowId(tkwin),
	    Tk_Width(tkwin), Tk_Height(tkwin), Tk_Depth(tkwin));
	drawable = pixmap;
    }
    /* Clear the background of the grid */

    XFillRectangle(Tk_Display(tkwin), drawable, tedPtr->fillGC, 0, 0,
	Tk_Width(tkwin), Tk_Height(tkwin));

    /* Draw the row and column buttons */

    if (tedPtr->numRects > 0) {
	int i;

	for (i = 0; i < tedPtr->numRects; i++) {
	    Blt_Fill3DRectangle(tkwin, drawable, tedPtr->border,
		tedPtr->rects[i].x, tedPtr->rects[i].y,
		tedPtr->rects[i].width, tedPtr->rects[i].height,
		tedPtr->borderWidth, tedPtr->relief);
	}
#ifdef notdef
	XFillRectangles(tedPtr->display, drawable, tedPtr->rectGC,
	    tedPtr->rects, tedPtr->numRects);
	XDrawRectangles(tedPtr->display, drawable, tedPtr->drawGC,
	    tedPtr->rects, tedPtr->numRects);
#endif
    }
    if (tedPtr->numPadRects > 0) {
	XFillRectangles(tedPtr->display, drawable, tedPtr->padRectGC,
	    tedPtr->padRectArr, tedPtr->numPadRects);
    }
    if (tedPtr->spanActive) {
	XFillRectangles(tedPtr->display, drawable, tedPtr->spanGC,
	    tedPtr->activeRectArr, 1);
	XFillRectangles(tedPtr->display, drawable, tedPtr->drawGC,
	    tedPtr->activeRectArr + 1, 4);
    }
    if (tedPtr->numWidgetPadRects > 0) {
	XFillRectangles(tedPtr->display, drawable, tedPtr->attributes.fillGC,
	    tedPtr->widgetPadRectArr, tedPtr->numWidgetPadRects);
    }
    if (tedPtr->numCntlRects > 0) {
	XFillRectangles(tedPtr->display, drawable, tedPtr->attributes.cntlGC,
	    tedPtr->cntlRectArr, tedPtr->numCntlRects);
    }
    /* Draw the grid lines */
    if (tedPtr->numSegs > 0) {
	XDrawSegments(tedPtr->display, drawable, tedPtr->drawGC,
	    tedPtr->segArr, tedPtr->numSegs);
    }
#ifndef notdef
    /* Draw any entry windows */
    for (link = Blt_Chain_FirstLink(tedPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	repPtr = Blt_Chain_GetValue(link);
	if (repPtr->mapped) {
	    DisplayEntry(repPtr);
	}
    }
#endif
    if (tedPtr->doubleBuffer) {
	XCopyArea(tedPtr->display, drawable, Tk_WindowId(tkwin), tedPtr->fillGC,
	    0, 0, Tk_Width(tkwin), Tk_Height(tkwin), 0, 0);
	Tk_FreePixmap(tedPtr->display, pixmap);
    }
}


static void
DrawEditor(Editor *editPtr)
{
    TableEditor *tedPtr = (TableEditor *) editPtr;

    tedPtr->flags |= LAYOUT_PENDING;
    if ((tedPtr->tkwin != NULL) && !(tedPtr->flags & REDRAW_PENDING)) {
	tedPtr->flags |= REDRAW_PENDING;
#ifdef notdef
	fprintf(stderr, "from draw editor\n");
#endif
	Tcl_DoWhenIdle(DisplayTableEditor, tedPtr);
    }
}

static void
DestroyEditor(DestroyData destroyData)
{
    TableEditor *tedPtr = (TableEditor *) destroyData;

    tedPtr->tkwin = NULL;
    if (tedPtr->flags & REDRAW_PENDING) {
	Tcl_CancelIdleCall(DisplayTableEditor, tedPtr);
    }
    Tcl_EventuallyFree(tedPtr, DestroyTableEditor);
}

/*
 *---------------------------------------------------------------------------
 *
 * EditOp --
 *
 *	Processes an objv/objc list of table entries to add and configure
 *	new widgets into the table.  A table entry consists of the
 *	window path name, table index, and optional configuration options.
 *	The first argument in the objv list is the name of the table.  If
 *	no table exists for the given window, a new one is created.
 *
 * Results:
 *	Returns a standard TCL result.  If an error occurred, TCL_ERROR is
 *	returned and an error message is left in interp->result.
 *
 * Side Effects:
 *	Memory is allocated, a new master table is possibly created, etc.
 *	The table is re-computed and arranged at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static int
EditOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,		/* Interpreter to return list of names to */
    int objc,			/* Number of arguments */
    Tcl_Obj *const *objv)
{
    Table *tablePtr;
    TableEditor *tedPtr;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (tablePtr->editPtr != NULL) {	/* Already editing this table */
	tedPtr = (TableEditor *) tablePtr->editPtr;
    } else {
	tedPtr = CreateTableEditor(tablePtr, interp);
	if (tedPtr == NULL) {
	    return TCL_ERROR;
	}
    }
    if (ConfigureTableEditor(tedPtr, objc - 3, objv + 3, 0) != TCL_OK) {
	tedPtr->tkwin = NULL;
	if (tedPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayTableEditor, tedPtr);
	}
	Tcl_EventuallyFree(tedPtr, DestroyTableEditor);
	return TCL_ERROR;
    }
    /* Rearrange the table */
    if (!(tablePtr->flags & ARRANGE_PENDING)) {
	tablePtr->flags |= ARRANGE_PENDING;
	Tcl_DoWhenIdle(tablePtr->arrangeProc, tablePtr);
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(tedPtr->tkwin), -1);
    tedPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(tedPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetCmd --
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,		/* Interpreter to report results back to */
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)	/* Option-value pairs */
{
    TableEditor *tedPtr;

    tedPtr = FindEditor(dataPtr, interp, objv[2]);
    if (tedPtr == NULL) {
	return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, tedPtr->tkwin, configSpecs,
	    (char *)tedPtr, objv[3], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureCmd --
 *
 *	This procedure is called to process an objv/objc list in order to
 *	configure the table geometry manager.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Table configuration options (padx, pady, rows, columns, etc) get
 *	set.   The table is recalculated and arranged at the next idle
 *	point.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,		/* Interpreter to report results back to */
    int objc,
    Tcl_Obj *const *objv)		/* Option-value pairs */
{
    TableEditor *tedPtr;

    tedPtr = FindEditor(dataPtr, interp, objv[2]);
    if (tedPtr == NULL) {
	return TCL_ERROR;
    }
    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, tedPtr->tkwin, configSpecs,
		(char *)tedPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, tedPtr->tkwin, configSpecs,
		(char *)tedPtr, objv[3], 0);
    }
    if (ConfigureTableEditor(tedPtr, objc - 3, objv + 3, BLT_CONFIG_OBJV_ONLY) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    EventuallyRedraw(tedPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,		/* Interpreter to return list of names to */
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Table *tablePtr;
    TableEditor *tedPtr;
    TableEntry *tePtr;
    int active;
    int x, y, width, height;
    int ix, iy;
    Blt_ChainLink link;
    Tk_Window tkwin;

    /* ted select master @x,y */
    tkwin = dataPtr->tkMain;
    tedPtr = FindEditor(dataPtr, interp, objv[2]);
    if (tedPtr == NULL) {
	return TCL_ERROR;
    }
    if (Blt_GetXY(interp, tkwin, Tcl_GetString(objv[3]), &ix, &iy) != TCL_OK) {
	return TCL_ERROR;
    }
    tablePtr = tedPtr->tablePtr;
    active = 0;
    for (link = Blt_Chain_FirstLink(tablePtr->chain);
	link != NULL; link = Blt_Chain_NextLink(link)) {
	tePtr = Blt_Chain_GetValue(link);
	x = tePtr->x - tePtr->xPad.side1;
	y = tePtr->y - tePtr->yPad.side1;
	width = Tk_Width(tePtr->tkwin) + PADDING(tePtr->xPad);
	height = Tk_Height(tePtr->tkwin) + PADDING(tePtr->yPad);
	if ((ix >= x) && (ix <= (x + width)) &&
	    (iy >= y) && (iy <= (y + height))) {
	    int left, right, top, bottom;
	    int last;
	    int grip;
	    RowColumn *rcPtr;

	    last = tePtr->column.rcPtr->index + tePtr->column.span - 1;
	    link = Blt_Chain_GetNthLink(tablePtr->cols.chain, last);
	    rcPtr = Blt_Chain_GetValue(link);

	    /* Calculate the span rectangle */
	    left = (tePtr->column.rcPtr->offset -
		tePtr->column.rcPtr->pad.side1);
	    right = (rcPtr->offset - rcPtr->pad.side1) + rcPtr->size;

	    top = (tePtr->row.rcPtr->offset -
		tePtr->row.rcPtr->pad.side1);

	    last = tePtr->row.rcPtr->index + tePtr->row.span - 1;
	    link = Blt_Chain_GetNthLink(tablePtr->rows.chain, last);
	    rcPtr = Blt_Chain_GetValue(link);
	    bottom = (rcPtr->offset - rcPtr->pad.side1) + rcPtr->size;

	    tedPtr->activeRectArr[0].x = left;
	    tedPtr->activeRectArr[0].y = top;
	    tedPtr->activeRectArr[0].width = (right - left);
	    tedPtr->activeRectArr[0].height = (bottom - top);

	    grip = tedPtr->gripSize;
	    tedPtr->activeRectArr[1].x = (left + right - grip) / 2;
	    tedPtr->activeRectArr[1].y = top;
	    tedPtr->activeRectArr[1].width = grip - 1;
	    tedPtr->activeRectArr[1].height = grip - 1;

	    tedPtr->activeRectArr[2].x = left;
	    tedPtr->activeRectArr[2].y = (top + bottom - grip) / 2;
	    tedPtr->activeRectArr[2].width = grip - 1;
	    tedPtr->activeRectArr[2].height = grip - 1;

	    tedPtr->activeRectArr[3].x = (left + right - grip) / 2;
	    tedPtr->activeRectArr[3].y = bottom - grip;
	    tedPtr->activeRectArr[3].width = grip - 1;
	    tedPtr->activeRectArr[3].height = grip - 1;

	    tedPtr->activeRectArr[4].x = right - grip;
	    tedPtr->activeRectArr[4].y = (top + bottom - grip) / 2;
	    tedPtr->activeRectArr[4].width = grip - 1;
	    tedPtr->activeRectArr[4].height = grip - 1;

	    Tcl_SetStringObj(Tcl_GetObjResult(interp), 
			     Tk_PathName(tePtr->tkwin), -1);
	    active = 1;
	    break;
	}
    }
    if ((active) || (active != tedPtr->spanActive)) {
	tedPtr->spanActive = active;
	EventuallyRedraw(tedPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * EditOp --
 *
 *	Processes an objv/objc list of table entries to add and configure
 *	new widgets into the table.  A table entry consists of the
 *	window path name, table index, and optional configuration options.
 *	The first argument in the objv list is the name of the table.  If
 *	no table exists for the given window, a new one is created.
 *
 * Results:
 *	Returns a standard TCL result.  If an error occurred, TCL_ERROR is
 *	returned and an error message is left in interp->result.
 *
 * Side Effects:
 *	Memory is allocated, a new master table is possibly created, etc.
 *	The table is re-computed and arranged at the next idle point.
 *
 *---------------------------------------------------------------------------
 */
static int
RepOp(
    TableInterpData *dataPtr,	/* Interpreter-specific data. */
    Tcl_Interp *interp,		/* Interpreter to return list of names to */
    int objc,			/* Number of arguments */
    Tcl_Obj *const *objv)
{
    Tk_Window tkwin;
    Table *tablePtr;
    TableEditor *tedPtr;

    if (Blt_GetTableFromObj(dataPtr, interp, objv[2], &tablePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    /* ted rep master index */
    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[3]), dataPtr->tkMain);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    if (tablePtr->editPtr != NULL) {	/* Already editing this table */
	tedPtr = (TableEditor *) tablePtr->editPtr;
    } else {
	tedPtr = CreateTableEditor(tablePtr, interp);
	if (tedPtr == NULL) {
	    return TCL_ERROR;
	}
    }
    if (ConfigureTableEditor(tedPtr, objc - 3, objv + 3, 0) != TCL_OK) {
	tedPtr->tkwin = NULL;
	if (tedPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayTableEditor, tedPtr);
	}
	Tcl_EventuallyFree(tedPtr, DestroyTableEditor);
	return TCL_ERROR;
    }
    /* Rearrange the table */
    if (!(tablePtr->flags & ARRANGE_PENDING)) {
	tablePtr->flags |= ARRANGE_PENDING;
	Tcl_DoWhenIdle(tablePtr->arrangeProc, tablePtr);
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Tk_PathName(tedPtr->tkwin), -1);
    tedPtr->flags |= LAYOUT_PENDING;
    EventuallyRedraw(tedPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Command options for the table editor.
 *
 * The fields for Blt_OperSpec are as follows:
 *
 *   - option name
 *   - minimum number of characters required to disambiguate the option name.
 *   - function associated with command option.
 *   - minimum number of arguments required.
 *   - maximum number of arguments allowed (0 indicates no limit).
 *   - usage string
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec tedOps[] =
{
    {"cget",      2, CgetOp,      4, 4, "master option",},
    {"configure", 2, ConfigureOp, 3, 0, "master ?option...?",},
    {"edit",      1, EditOp,      3, 0, "master ?options...?",},
    {"rep",       1, RepOp,       2, 0, "master index ?options...?",},
    {"select",    1, SelectOp,    4, 0, "master @x,y",},
#ifdef notdef
    {"forget",    1, ForgetOp,    3, 0, "master ?master...?",},
    {"index",     1, IndexOp,     3, 0, "master ?item...?",}, 
#endif
};
static int numTedOps = sizeof(tedOps) / sizeof(Blt_OpSpec);

/*
 *---------------------------------------------------------------------------
 *
 * TedCmd --
 *
 *	This procedure is invoked to process the TCL command that
 *	corresponds to the table geometry manager.  See the user
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
static int
TedCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numTedOps, tedOps, BLT_OP_ARG1, objc, objv, 
	    0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

static TableData *
GetTableInterpData(Tcl_Interp *interp)
{
    TableData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (TableData *)Tcl_GetAssocData(interp, TABLE_THREAD_KEY, &proc);
    assert(dataPtr);
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_TedCmdInitProc --
 *
 *	This procedure is invoked to initialize the TCL command that
 *	corresponds to the table geometry manager.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the new command and adds an entry into a global Tcl
 *	associative array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_TedCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"ted", TedCmd, };

    cmdSpec.clientData = GetTableInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

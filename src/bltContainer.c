/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltContainer.c --
 *
 * This module implements a container widget for the BLT toolkit.
 *
 *	Copyright 1998-2004 George A Howlett.
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

#ifndef NO_CONTAINER

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#if !defined(WIN32) && !defined(MACOSX)
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#endif

#include "bltAlloc.h"
#include "bltChain.h"
#include "bltTree.h"
#include "tkDisplay.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define XDEBUG

#define SEARCH_TRIES	100		/* Maximum number of attempts to check
					 * for a given window before
					 * failing. */
#define SEARCH_INTERVAL 20		/* Number milliseconds to wait after
					 * each attempt to find a window. */

#define SEARCH_TKWIN	(1<<0)		/* Search via Tk window pathname. */
#define SEARCH_XID	(1<<1)		/* Search via an XID 0x0000000. */
#define SEARCH_CMD	(1<<2)		/* Search via a command-line
					 * arguments. */
#define SEARCH_NAME	(1<<3)		/* Search via the application name. */
#define SEARCH_PROPERTY	(1<<4)		/* Search via the application name. */
#define SEARCH_ALL	(SEARCH_TKWIN | SEARCH_XID | SEARCH_CMD | SEARCH_NAME)

#define CONTAINER_REDRAW		(1<<1)
#define CONTAINER_MAPPED		(1<<2)
#define CONTAINER_FOCUS			(1<<4)
#define CONTAINER_INIT			(1<<5)
#define CONTAINER_MOVE			(1<<7)

#define DEF_BACKGROUND	STD_NORMAL_BACKGROUND
#define DEF_BORDERWIDTH	STD_BORDERWIDTH
#define DEF_COMMAND		(char *)NULL
#define DEF_CURSOR		(char *)NULL
#define DEF_HEIGHT		"0"
#define DEF_HIGHLIGHT_BACKGROUND STD_NORMAL_BACKGROUND
#define DEF_HIGHLIGHT_COLOR	RGB_BLACK
#define DEF_HIGHLIGHT_WIDTH	"2"
#define DEF_RELIEF		"sunken"
#define DEF_TAKE_FOCUS	"0"
#define DEF_TIMEOUT		"20"
#define DEF_WIDTH		"0"
#define DEF_WINDOW		(char *)NULL

typedef struct _SearchInfo SearchInfo;
typedef void (SearchProc)(Display *display, Window window, 
	SearchInfo *searchPtr);

struct _SearchInfo {
    SearchProc *proc;
    const char *pattern;		/* Search pattern. */
    Window window;			/* XID of last window that matches
					 * the criteria. */
    int numMatches;			/* # of windows that match the 
					 * pattern. */
    int saveNames;			/* Indicates to save the names of the
					 * window XIDs that match the search
					 * criteria. */
    Tcl_DString ds;			/* Will contain the strings of the
					 * window XIDs matching the search
					 * criteria. */
    Atom atom;
};

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
    Tcl_Command cmdToken;		/* Token for widget's command. */
    unsigned int flags;			/* For bit-field definitions, see
					 * above. */
    int inset;				/* Total width of borders; focus
					 * highlight and 3-D border. Indicates
					 * the offset from outside edges to
					 * leave room for borders. */
    Tk_Cursor cursor;			/* X Cursor */
    Tk_3DBorder border;			/* 3D border surrounding the adopted
					 * window. */
    int borderWidth;			/* Width of 3D border. */
    int relief;				/* 3D border relief. */
    Tk_Window tkToplevel;		/* Toplevel (wrapper) window of
					 * container.  It's used to track the
					 * location of the container. If it
					 * moves we need to notify the
					 * embedded application. */
    /*
     * Focus highlight ring
     */
    int highlightWidth;			/* Width in pixels of highlight to
					 * draw around widget when it has the
					 * focus.  <= 0 means don't draw a
					 * highlight. */
    XColor *highlightBgColor;		/* Color for drawing traversal
					 * highlight area when highlight is
					 * off. */
    XColor *highlightColor;		/* Color for drawing traversal
					 * highlight. */
    GC highlightGC;			/* GC for focus highlight. */
    char *takeFocus;			/* Says whether to select this widget
					 * during tab traveral operations.
					 * This value isn't used in C code,
					 * but for the widget's Tcl
					 * bindings. */

    int reqWidth, reqHeight;		/* Requested dimensions of the
					 * container window. */

    Window adopted;			/* X window Id or Win32 handle of
					 * adopted window contained by the
					 * widget.  If None, no window has
					 * been reparented. */
    Tk_Window tkAdopted;		/* Non-NULL if this is a Tk window
					 * that's been adopted. */
    int adoptedX, adoptedY;		/* Current position of the adopted
					 * window. */
    int adoptedWidth;			/* Current width of the adopted
					 * window. */
    int adoptedHeight;			/* Current height of the adopted
					 * window. */

    int origX, origY;
    int origWidth, origHeight;		/* Dimensions of the window when it
					 * was adopted.  When the window is
					 * released it's returned to it's
					 * original dimensions. */

    int timeout;
    int nextId;
} Container;


static Blt_OptionPrintProc XIDToObj;
static Blt_OptionParseProc ObjToXID;

static Blt_CustomOption XIDOption = {
    ObjToXID, XIDToObj, NULL, (ClientData)(SEARCH_TKWIN | SEARCH_XID),
};

#ifndef WIN32
static Blt_CustomOption XIDNameOption = {
    ObjToXID, XIDToObj, NULL, (ClientData)SEARCH_NAME,
};

static Blt_CustomOption XIDCmdOption = {
    ObjToXID, XIDToObj, NULL, (ClientData)SEARCH_CMD,
};
static Blt_CustomOption XIDPropertyOption = {
    ObjToXID, XIDToObj, NULL, (ClientData)SEARCH_PROPERTY,
};
#endif

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_BORDER, "-background", "background", "Background", 
	DEF_BACKGROUND, Blt_Offset(Container, border), 0},
    {BLT_CONFIG_SYNONYM, "-bd", "borderWidth", (char *)NULL, (char *)NULL, 0,0},
    {BLT_CONFIG_SYNONYM, "-bg", "background", (char *)NULL, (char *)NULL, 0, 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_BORDERWIDTH, Blt_Offset(Container, borderWidth), 
	BLT_CONFIG_DONT_SET_DEFAULT},
#ifndef WIN32
    {BLT_CONFIG_CUSTOM, "-command", "command", "Command", DEF_WINDOW, 
	Blt_Offset(Container, adopted), BLT_CONFIG_DONT_SET_DEFAULT, 
	&XIDCmdOption},
#endif
    {BLT_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor", DEF_CURSOR, 
	Blt_Offset(Container, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_PIXELS_NNEG, "-height", "height", "Height", DEF_HEIGHT, 
	Blt_Offset(Container, reqHeight), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_COLOR, "-highlightbackground", "highlightBackground",
	"HighlightBackground", DEF_HIGHLIGHT_BACKGROUND, 
	Blt_Offset(Container, highlightBgColor), 0},
    {BLT_CONFIG_COLOR, "-highlightcolor", "highlightColor", "HighlightColor",
	DEF_HIGHLIGHT_COLOR, Blt_Offset(Container, highlightColor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-highlightthickness", "highlightThickness",
	"HighlightThickness", DEF_HIGHLIGHT_WIDTH, 
	Blt_Offset(Container, highlightWidth), BLT_CONFIG_DONT_SET_DEFAULT},
#ifndef WIN32
    {BLT_CONFIG_CUSTOM, "-name", "name", "Name", DEF_WINDOW, 
	Blt_Offset(Container, adopted), BLT_CONFIG_DONT_SET_DEFAULT, 
	&XIDNameOption},
    {BLT_CONFIG_CUSTOM, "-property", "property", "Property", DEF_WINDOW, 
	Blt_Offset(Container, adopted), BLT_CONFIG_DONT_SET_DEFAULT, 
	&XIDPropertyOption},
#endif
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief", DEF_RELIEF, 
	Blt_Offset(Container, relief), 0},
    {BLT_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus", DEF_TAKE_FOCUS,
	Blt_Offset(Container, takeFocus), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_INT_POS, "-timeout", "timeout", "Timeout", DEF_TIMEOUT, 
	Blt_Offset(Container, timeout), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_PIXELS_NNEG, "-width", "width", "Width", DEF_WIDTH, 
	Blt_Offset(Container, reqWidth), BLT_CONFIG_DONT_SET_DEFAULT},
    {BLT_CONFIG_CUSTOM, "-window", "window", "Window", DEF_WINDOW, 
	Blt_Offset(Container, adopted), BLT_CONFIG_DONT_SET_DEFAULT, 
	&XIDOption},
    {BLT_CONFIG_END}
};

/* Forward Declarations */
static Tcl_CmdDeleteProc ContainerInstCmdDeleteProc;
static Tcl_FreeProc DestroyContainer;
static Tcl_IdleProc DisplayContainer;
static Tcl_ObjCmdProc ContainerCmd;
static Tcl_ObjCmdProc ContainerInstCmd;
static Tk_EventProc ContainerEventProc;
static Tk_EventProc ToplevelEventProc;
static Tk_GenericProc AdoptedWindowEventProc;

static void EventuallyRedraw(Container *conPtr);

typedef int (ContainerCmdProc)(Container *comboPtr, Tcl_Interp *interp, 
	int objc, Tcl_Obj *const *objv);

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * GetWindowId --
 *
 *      Returns the XID for the Tk_Window given.  Starting in Tk 8.0, the
 *      toplevel widgets are wrapped by another window.  Currently there's no
 *      way to get at that window, other than what is done here: query the X
 *      window hierarchy and grab the parent.
 *
 * Results:
 *      Returns the X Window ID of the widget.  If it's a toplevel, then
 *	the XID of the wrapper is returned.
 *
 *---------------------------------------------------------------------------
 */
Window
GetXID(Tk_Window tkwin)
{
    HWND hWnd;
    TkWinWindow *twdPtr;

    hWnd = Tk_GetHWND(Tk_WindowId(tkwin));
    if (Tk_IsTopLevel(tkwin)) {
	hWnd = GetParent(hWnd);
    }
    twdPtr = Blt_AssertMalloc(sizeof(TkWinWindow));
    twdPtr->handle = hWnd;
    twdPtr->type = TWD_WINDOW;
    twdPtr->winPtr = tkwin;
    return (Window)twdPtr;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * NameOfId --
 *
 *	Returns a string representing the given XID.
 *
 * Results:
 *	A static string containing either the hexidecimal number or
 *	the pathname of a Tk window.
 *
 *---------------------------------------------------------------------------
 */
static const char *
NameOfId(
    Display *display,			/* Display containing both the
					 * container widget and the adopted
					 * window. */
    Window window)			/* XID of the adopted window. */
{
    if (window != None) {
	Tk_Window tkwin;
	static char string[200];

	/* See first if it's a window that Tk knows about.  */
	/*
	 * Note:  If the wrapper window is reparented, Tk pretends it's
	 *        no longer connected to the toplevel, so if you look for
	 *	  the child of the wrapper tkwin, it's NULL.  
	 */
	tkwin = Tk_IdToWindow(display, window); 
	if ((tkwin != NULL) && (Tk_PathName(tkwin) != NULL)) {
	    return Tk_PathName(tkwin); 
	} 
	Blt_FormatString(string, 200, "0x%lx", (unsigned long)window);
	return string;
    }
    return "";				/* Return empty string if XID is
					 * None. */
}

#ifndef WIN32
/*
 *---------------------------------------------------------------------------
 *
 * XGeometryErrorProc --
 *
 *	Flags errors generated from XGetGeometry calls to the X server.
 *
 * Results:
 *	Always returns 0.
 *
 * Side Effects:
 *	Sets a flag, indicating an error occurred.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
XGeometryErrorProc(
    ClientData clientData,
    XErrorEvent *eventPtr)	/* Not used. */
{
    int *errorPtr = clientData;

    *errorPtr = TCL_ERROR;
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetAdoptedWindowGeometry --
 *
 *	Computes the requested geometry of the container using the size of
 *	adopted window as a reference.
 *
 * Results:
 *	A standard TCL result. 
 *
 * Side Effects:
 *	Sets a flag, indicating an error occurred.
 *
 *---------------------------------------------------------------------------
 */
static int
GetAdoptedWindowGeometry(Tcl_Interp *interp, Container *conPtr)
{
    int x, y, w, h, borderWidth, depth;
    int xOffset, yOffset;
    Window root, dummy;
    Tk_ErrorHandler handler;
    int result;
    int any = -1;
    
    xOffset = yOffset = 0;
    if (conPtr->adopted != None) {
	handler = Tk_CreateErrorHandler(conPtr->display, any, X_GetGeometry, 
		any, XGeometryErrorProc, &result);
	root = Tk_RootWindow(conPtr->tkwin);
	XTranslateCoordinates(conPtr->display, conPtr->adopted,
		      root, 0, 0, &xOffset, &yOffset, &dummy);
	result = XGetGeometry(conPtr->display, conPtr->adopted, &root, 
		&x, &y, (unsigned int *)&w, (unsigned int *)&h,
	      (unsigned int *)&borderWidth, (unsigned int *)&depth);
	Tk_DeleteErrorHandler(handler);
	XSync(conPtr->display, False);
	if (result == 0) {
	    Tcl_AppendResult(interp, "can't get geometry for \"", 
		     NameOfId(conPtr->display, conPtr->adopted), "\"", 
		     (char *)NULL);
	    return TCL_ERROR;
	}
	conPtr->origX = xOffset;
	conPtr->origY = yOffset;
	conPtr->origWidth = w;
	conPtr->origHeight = h;
    } else {
	x = y = 0;
	w = h = 1;
	conPtr->origX = conPtr->origY = 0;
	conPtr->origWidth = conPtr->origHeight = 0;
    }
    conPtr->adoptedX = x;
    conPtr->adoptedY = y;
    conPtr->adoptedWidth = w;
    conPtr->adoptedHeight = h;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetChildren --
 *
 *	Returns a chain of the child windows according to their stacking
 *	order.  The window ids are ordered from top to bottom.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain
GetChildren(Display *display, Window window)
{
    Window *children;
    unsigned int numChildren;
    Window parent, root;

    if (XQueryTree(display, window, &parent, &root, &children, &numChildren)) {
	if (numChildren > 0) {
	    Blt_Chain chain;
	    size_t i;
	    
	    chain = Blt_Chain_Create();
	    for (i = 0; i < numChildren; i++) {
		/*
		 *  XQuery returns windows in bottom to top order.  We'll
		 *  reverse the order.
		 */
		Blt_Chain_Prepend(chain, (ClientData)children[i]);
	    }
	    XFree((char *)children);
	    return chain;
	}
    }
    return NULL;
}

static int
GetMaxPropertySize(Display *display)
{
    int numBytes;

    numBytes = Blt_MaxRequestSize(display, sizeof(char));
    numBytes -= 32;
    return numBytes;
}

static unsigned char *
GetProperty(Display *display, Window window, Atom atom)
{
    unsigned char *data;
    int result, format;
    Atom typeAtom;
    unsigned long numItems, bytesAfter;

    if (window == None) {
	return NULL;
    }
    data = NULL;
    result = XGetWindowProperty(
        display,			/* Display of window. */
	window,				/* Window holding the property. */
        atom,				/* Name of property. */
        0,				/* Offset of data (for multiple
					 * reads). */
	GetMaxPropertySize(display),	/* Maximum number of items to read. */
	False,				/* If true, delete the property. */
        XA_STRING,			/* Desired type of property. */
        &typeAtom,			/* (out) Actual type of the
					 * property. */
        &format,			/* (out) Actual format of the
					 * property. */
        &numItems,			/* (out) # of items in specified
					 * format. */
        &bytesAfter,			/* (out) # of bytes remaining to be
					 * read. */
	&data);
    if ((result != Success) || (format != 8) /*|| (typeAtom != XA_STRING)*/) {
	if ((result == Success) && (format != None)) {
	    Blt_Warn("format=%d typeAtom=%d\n", format, (int)typeAtom);
	}
	if (data != NULL) {
	    XFree((char *)data);
	    data = NULL;
	}
    }
    return data;
}

/*
 *---------------------------------------------------------------------------
 *
 * SearchForProperty --
 *
 *	Traverses the entire window hierarchy, searching for windows matching
 *	the name field in the SearchInfo structure. This routine is
 *	recursively called for each successive level in the window hierarchy.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The SearchInfo structure will track the number of windows that 
 *	match the given criteria.
 *	
 *---------------------------------------------------------------------------
 */
static void
SearchForProperty(Display *display, Window window, SearchInfo *searchPtr)
{
    Blt_Chain chain;
    char *data;

    data = (char *)GetProperty(display, window, searchPtr->atom);
    if (data != NULL) {
	/* Compare the name of the window to the search pattern. */
	if (Tcl_StringMatch(data, searchPtr->pattern)) {
	    if (searchPtr->saveNames) {	/* Record names of matching windows. */
		Tcl_DStringAppendElement(&searchPtr->ds, 
			NameOfId(display, window));
		Tcl_DStringAppendElement(&searchPtr->ds, data);
	    }
	    searchPtr->window = window;
	    searchPtr->numMatches++;
	}
	XFree(data);
    }
    /* Process the window's descendants. */
    chain = GetChildren(display, window);
    if (chain != NULL) {
	Blt_ChainLink link;
	Window child;

	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	     link = Blt_Chain_NextLink(link)) {
	    child = (Window)Blt_Chain_GetValue(link);
	    SearchForProperty(display, child, searchPtr);
	}
	Blt_Chain_Destroy(chain);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SearchForName --
 *
 *	Traverses the entire window hierarchy, searching for windows matching
 *	the name field in the SearchInfo structure. This routine is
 *	recursively called for each successive level in the window hierarchy.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The SearchInfo structure will track the number of windows that 
 *	match the given criteria.
 *	
 *---------------------------------------------------------------------------
 */
static void
SearchForName(Display *display, Window window, SearchInfo *searchPtr)
{
    Blt_Chain chain;
    char *wmName;

    if (XFetchName(display, window, &wmName)) {
	/* Compare the name of the window to the search pattern. */
	if (Tcl_StringMatch(wmName, searchPtr->pattern)) {
	    if (searchPtr->saveNames) {		
		/* Record names of matching windows. */
		Tcl_DStringAppendElement(&searchPtr->ds, 
			 NameOfId(display, window));
		Tcl_DStringAppendElement(&searchPtr->ds, wmName);
	    }
	    searchPtr->window = window;
	    searchPtr->numMatches++;
	}
	XFree(wmName);
    }
    /* Process the window's descendants. */
    chain = GetChildren(display, window);
    if (chain != NULL) {
	Blt_ChainLink link;

	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	     link = Blt_Chain_NextLink(link)) {
	    Window child;

	    child = (Window)Blt_Chain_GetValue(link);
	    SearchForName(display, child, searchPtr);
	}
	Blt_Chain_Destroy(chain);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SearchForCommand --
 *
 *	Traverses the entire window hierarchy, searching for windows matching
 *	the command-line specified in the SearchInfo structure.  This routine
 *	is recursively called for each successive level in the window
 *	hierarchy.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The SearchInfo structure will track the number of windows that match
 *	the given command-line.
 *	
 *---------------------------------------------------------------------------
 */
static void
SearchForCommand(Display *display, Window window, SearchInfo *searchPtr)
{
    Blt_Chain chain;
    int argc;
    char **argv;

    if (XGetCommand(display, window, &argv, &argc)) {
	const char *string;

	string = Tcl_Merge(argc, (const char **)argv);
	XFreeStringList(argv);
	if (Tcl_StringMatch(string, searchPtr->pattern)) {
	    if (searchPtr->saveNames) { 
		/* Record names of matching windows. */
		Tcl_DStringAppendElement(&searchPtr->ds, 
			NameOfId(display, window));
		Tcl_DStringAppendElement(&searchPtr->ds, string);
	    }
	    searchPtr->window = window;
	    searchPtr->numMatches++;
	}
	Blt_Free(string);
    }
    /* Process the window's descendants. */
    chain = GetChildren(display, window);
    if (chain != NULL) {
	Blt_ChainLink link;

	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	     link = Blt_Chain_NextLink(link)) {
	    Window child;

	    child = (Window)Blt_Chain_GetValue(link);
	    SearchForCommand(display, child, searchPtr);
	}
	Blt_Chain_Destroy(chain);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TimeoutProc --
 *
 *	Procedure called when the timer event elapses.  Used to wait between
 *	attempts searching for the designated window.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Sets a flag, indicating the timeout occurred.
 *
 *---------------------------------------------------------------------------
 */
static void
TimeoutProc(ClientData clientData)
{
    int *expirePtr = clientData;

    *expirePtr = TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 * TestAndWaitForWindow --
 *
 *	Searches (possibly multiple times) for windows matching the specified
 *	criteria, using the given search procedure.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Sets a flag, indicating the timeout occurred.
 *
 *---------------------------------------------------------------------------
 */
static void
TestAndWaitForWindow(
    Container *conPtr,			/* Container widget record. */
    SearchInfo *searchPtr)		/* Search criteria. */
{
    Window root;
    Tcl_TimerToken timerToken;
    int expire;
    int i;

    /* Get the root window to start the search.  */
    root = Tk_RootWindow(conPtr->tkwin);
    timerToken = NULL;
    for (i = 0; i < SEARCH_TRIES; i++) {
	searchPtr->numMatches = 0;
	(*searchPtr->proc)(conPtr->display, root, searchPtr);
	if (searchPtr->numMatches > 0) {
	    if (timerToken != NULL) {
		Tcl_DeleteTimerHandler(timerToken);
	    }
	    return;
	}
	expire = FALSE;
	/*   
	 * If the X11 application associated with the adopted window was just
	 * started (via "exec" or "bgexec"), the window may not exist yet.  We
	 * have to wait a little bit for the program to start up.  Create a
	 * timer event break us out of an wait loop.  We'll wait for a given
	 * interval for the adopted window to appear.
	 */
	timerToken = Tcl_CreateTimerHandler(conPtr->timeout, TimeoutProc, 
		&expire);
	while (!expire) {
	    /* Should file events be allowed? */
	    Tcl_DoOneEvent(TCL_TIMER_EVENTS | TCL_WINDOW_EVENTS | 
			   TCL_FILE_EVENTS);
	}
    }	
}
#else 


/*
 *---------------------------------------------------------------------------
 *
 *  GetChildren --
 *
 *	Returns a chain of the child windows according to their stacking
 *	order.  The window ids are ordered from top to bottom.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain
GetChildren(Display *display, Window window)
{
    Blt_Chain chain;
    HWND hWnd;
    HWND parent;

    parent = Tk_GetHWND(window);
    chain = Blt_Chain_Create();
    for (hWnd = GetTopWindow(parent); hWnd != NULL;
	hWnd = GetNextWindow(hWnd, GW_HWNDNEXT)) {
	Blt_Chain_Append(chain, (ClientData)hWnd);
    }
    return chain;
}


/*
 *---------------------------------------------------------------------------
 *
 * GetAdoptedWindowGeometry --
 *
 *	Computes the requested geometry of the container using the size of
 *	adopted window as a reference.
 *
 * Results:
 *	A standard TCL result. 
 *
 * Side Effects:
 *	Sets a flag, indicating an error occurred.
 *
 *---------------------------------------------------------------------------
 */
static int
GetAdoptedWindowGeometry(Tcl_Interp *interp, Container *conPtr)
{
    int x, y, width, height;
    int xOffset, yOffset;
    Window root, dummy;
    
    width = height = 1;
    xOffset = yOffset = 0;
    x = y = 0;
    if (conPtr->adopted != None) {
	HWND hWnd;
	RECT rect;

	hWnd = Tk_GetHWND(conPtr->adopted);
	if (GetWindowRect(hWnd, &rect)) {
	    x = rect.left;
	    y = rect.top;
	    width = rect.right - rect.left + 1;
	    height = rect.bottom - rect.top + 1;
	} else {
	    Tcl_AppendResult(interp, "can't get geometry for \"", 
		     NameOfId(conPtr->display, conPtr->adopted), "\"", 
		     (char *)NULL);
	    return TCL_ERROR;
	}
	root = Tk_RootWindow(conPtr->tkwin);
	XTranslateCoordinates(conPtr->display, conPtr->adopted,
		      root, 0, 0, &xOffset, &yOffset, &dummy);
	conPtr->origX = xOffset;
	conPtr->origY = yOffset;
	conPtr->origWidth = width;
	conPtr->origHeight = height;
    } else {
	conPtr->origX = conPtr->origY = 0;
	conPtr->origWidth = conPtr->origHeight = 0;
    }
    conPtr->adoptedX = x;
    conPtr->adoptedY = y;
    conPtr->adoptedWidth = width;
    conPtr->adoptedHeight = height;
    return TCL_OK;
}

#endif /*WIN32*/

/*
 *---------------------------------------------------------------------------
 *
 *  MapTree --
 *
 *	Maps each window in the hierarchy.  This is needed because 
 *
 *  Results:
 *	None.
 *
 *  Side Effects:
 *	Each window in the hierarchy is mapped.
 *
 *---------------------------------------------------------------------------
 */
static void
MapTree(Display *display, Window window)
{
    Blt_Chain chain;

    XMapWindow(display, window);
    chain = GetChildren(display, window);
    if (chain != NULL) {
	Blt_ChainLink link;
	Window child;

	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	     link = Blt_Chain_NextLink(link)) {
	    child = (Window)Blt_Chain_GetValue(link);
	    MapTree(display, child);
	}
	Blt_Chain_Destroy(chain);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ObjToXID --
 *
 *	Converts a TCL obj into an X window Id.
 *
 * Results:
 *	If the string is successfully converted, TCL_OK is returned.
 *	Otherwise, TCL_ERROR is returned and an error message is left in
 *	interpreter's result field.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ObjToXID(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to report results */
    Tk_Window parent,			/* Parent window */
    Tcl_Obj *objPtr,			/* String representation. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    unsigned long searchFlags = (unsigned long)clientData;
    Container *conPtr = (Container *)widgRec;
    Window *idPtr = (Window *) (widgRec + offset);
    Tk_Window tkAdopted;
    Window id;
    const char *string;

    string = Tcl_GetString(objPtr);
    tkAdopted = NULL;
    id = None;
    if ((searchFlags & SEARCH_TKWIN) && (string[0] == '.')) {
	Tk_Window tkwin;

	tkwin = Tk_NameToWindow(interp, string, Tk_MainWindow(interp));
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	if (!Tk_IsTopLevel(tkwin)) {
	    Tcl_AppendResult(interp, "can't reparent non-toplevel Tk windows",
			     (char *)NULL);
	    return TCL_ERROR;
	}
	tkAdopted = tkwin;
	Tk_MakeWindowExist(tkwin);
	id = Blt_GetWindowId(tkwin);
#ifndef WIN32
    } else if ((searchFlags & SEARCH_XID) && (string[0] == '0') && 
	       (string[1] == 'x')) {
	int number;

	/* Hexidecimal string specifying the Window token. */
	if (Tcl_GetInt(interp, string, &number) != TCL_OK) {
	    return TCL_ERROR;
	}
	id = number;
    } else if ((string == NULL) || (string[0] == '\0')) {
	id = None;
    } else {
	SearchInfo search;

	memset(&search, 0, sizeof(search));
	if (searchFlags & (SEARCH_NAME | SEARCH_CMD | SEARCH_PROPERTY)) {
	    if (searchFlags & SEARCH_NAME) {
		search.pattern = string;
		search.proc = SearchForName;
	    } else if (searchFlags & SEARCH_CMD) {
		search.pattern = string;
		search.proc = SearchForCommand;
	    } else if (searchFlags & SEARCH_PROPERTY) {
		Tk_Window tkwin;
		char *atomName;
		int objc;
		Tcl_Obj **objv;

		if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) 
		    != TCL_OK) {
		    return TCL_ERROR;
		}
		if (objc != 2) {
		    return TCL_ERROR;
		}
		tkwin = Tk_MainWindow(interp);
		atomName = Tcl_GetString(objv[0]);
		string = search.pattern = Tcl_GetString(objv[1]);
		search.atom = XInternAtom(Tk_Display(tkwin), atomName, False); 
		search.proc = SearchForProperty;
	    }
	    TestAndWaitForWindow(conPtr, &search);
	    if (search.numMatches > 1) {
		Tcl_AppendResult(interp, "more than one window matches \"", 
			search.pattern, "\"", (char *)NULL);
		return TCL_ERROR;
	    }
	}
	if (search.numMatches == 0) {
	    Tcl_AppendResult(interp, "can't find window from pattern \"", 
			     search.pattern, "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	id = search.window;
#endif /*WIN32*/
    }
    if (*idPtr != None) {
	Window root;

	root = Tk_RootWindow(conPtr->tkwin);
	if (Blt_ReparentWindow(conPtr->display, *idPtr, root, 
		       conPtr->origX, conPtr->origY) 
	    != TCL_OK) {
	    Tcl_AppendResult(interp, "can't restore \"", 
			 NameOfId(conPtr->display, *idPtr), 
			"\" window to root", (char *)NULL);
	    return TCL_ERROR;
	}
	conPtr->flags &= ~CONTAINER_MAPPED;
	if (conPtr->tkAdopted == NULL) {
	    /* This wasn't a Tk window.  So deselect the event mask. */
	    XSelectInput(conPtr->display, *idPtr, 0);
	} else {
	    MapTree(conPtr->display, *idPtr);
	}
	XMoveResizeWindow(conPtr->display, *idPtr, conPtr->origX,
		conPtr->origY, conPtr->origWidth, conPtr->origHeight);
    }
    conPtr->tkAdopted = tkAdopted;
    *idPtr = id;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * XIDToString --
 *
 *	Converts the Tk window back to its string representation (i.e.
 *	its name).
 *
 * Results:
 *	The name of the window is returned.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static Tcl_Obj *
XIDToObj(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,
    Tk_Window parent,			/* Not used. */
    char *widgRec,			/* Widget record */
    int offset,				/* Offset to field in structure */
    int flags)	
{
    Container *conPtr = (Container *) widgRec;
    Window window = *(Window *)(widgRec + offset);
    Tcl_Obj *objPtr;

    if (conPtr->tkAdopted != NULL) {
	objPtr = Tcl_NewStringObj(Tk_PathName(conPtr->tkAdopted), -1);
    }  else {
	objPtr = Tcl_NewStringObj(NameOfId(conPtr->display, window), -1);
    }
    return objPtr;
}


/*
 *---------------------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *	Queues a request to redraw the widget at the next idle point.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets redisplayed.  Right now we don't do selective
 *	redisplays:  the whole window will be redrawn.
 *
 *---------------------------------------------------------------------------
 */
static void
EventuallyRedraw(Container *conPtr)
{
    if ((conPtr->tkwin != NULL) && !(conPtr->flags & CONTAINER_REDRAW)) {
	conPtr->flags |= CONTAINER_REDRAW;
	Tcl_DoWhenIdle(DisplayContainer, conPtr);
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * AdoptedWindowEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various
 * 	events on the encapsulated window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets resized or exposed, it's redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
AdoptedWindowEventProc(ClientData clientData, XEvent *eventPtr)
{
    Container *conPtr = clientData;

    if ((eventPtr->type == CreateNotify) && (conPtr->adopted == None)) {
	fprintf(stderr, "window found is %x\n", 
		(unsigned int)eventPtr->xmaprequest.window);
	if (Blt_ReparentWindow(conPtr->display, eventPtr->xmaprequest.window,
		Tk_WindowId(conPtr->tkwin), conPtr->inset, conPtr->inset) 
	    != TCL_OK) {
	    fprintf(stderr, "can't adopt window \"%s\"\n", 
		    NameOfId(conPtr->display, eventPtr->xmaprequest.window));
	    return 0;
	}
	conPtr->adopted = eventPtr->xmaprequest.window;
	XSelectInput(conPtr->display, conPtr->adopted, StructureNotifyMask);
	XSelectInput(conPtr->display, Tk_RootWindow(conPtr->tkwin), 0);
	return 1;
    }
    if (eventPtr->xany.window != conPtr->adopted) {
        return 0;
    }
    if (eventPtr->type == ConfigureNotify) {
	XConfigureEvent *evPtr = &eventPtr->xconfigure;
	int width, height;

	conPtr->origWidth = evPtr->width;
	conPtr->origHeight = evPtr->height;

	/* Add the designated inset to the requested dimensions. */
	width = conPtr->origWidth + 2 * conPtr->inset; 
	height = conPtr->origHeight + 2 * conPtr->inset;
	if (conPtr->reqWidth > 0) {
	    width = conPtr->reqWidth;
	} 
	if (conPtr->reqHeight > 0) {
	    height = conPtr->reqHeight;
	} 
	/* Set the requested width and height for the container. */
	if ((Tk_ReqWidth(conPtr->tkwin) != width) ||
	    (Tk_ReqHeight(conPtr->tkwin) != height)) {
	    Tk_GeometryRequest(conPtr->tkwin, width, height);
	}
	EventuallyRedraw(conPtr);
    }  else if (eventPtr->type == DestroyNotify) {
	conPtr->adopted = None;
	EventuallyRedraw(conPtr);
    }
    return 1;
}

/*
 *---------------------------------------------------------------------------
 *
 * ContainerEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various events on
 * 	container widgets.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static void
ContainerEventProc(ClientData clientData, XEvent *eventPtr)
{
    Container *conPtr = clientData;

    switch (eventPtr->type) {
    case Expose:
	if (eventPtr->xexpose.count == 0) {
	    EventuallyRedraw(conPtr);
	}
	break;

    case FocusIn:
    case FocusOut:
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    if (eventPtr->type == FocusIn) {
		conPtr->flags |= CONTAINER_FOCUS;
	    } else {
		conPtr->flags &= ~CONTAINER_FOCUS;
	    }
	    EventuallyRedraw(conPtr);
	}
	break;

    case ConfigureNotify:
	EventuallyRedraw(conPtr);
	break;

    case DestroyNotify:
	if (conPtr->tkwin != NULL) {
	    conPtr->tkwin = NULL;
	    Tcl_DeleteCommandFromToken(conPtr->interp, conPtr->cmdToken);
	}
	if (conPtr->flags & CONTAINER_REDRAW) {
	    Tcl_CancelIdleCall(DisplayContainer, conPtr);
	}
	Tcl_EventuallyFree(conPtr, DestroyContainer);
	break;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ToplevelEventProc --
 *
 *	Some applications assume that they are always a toplevel window and
 *	play tricks accordingly.  For example, Netscape positions menus in
 *	relation to the toplevel.  But if the container's toplevel is moved,
 *	this positioning is wrong.  So watch if the toplevel is moved.
 *
 *	[This would be easier and cleaner if Tk toplevels weren't so botched
 *	by the addition of menubars.  It's not enough to track the )
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
ToplevelEventProc(ClientData clientData, XEvent *eventPtr)
{
    Container *conPtr = clientData;

    if ((conPtr->adopted != None) && (conPtr->tkwin != NULL) &&
	(eventPtr->type == ConfigureNotify)) {
	conPtr->flags |= CONTAINER_MOVE;
	EventuallyRedraw(conPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyContainer --
 *
 * 	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release to
 * 	clean up the internal structure of the widget at a safe time (when
 * 	no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Everything associated with the widget is freed up.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyContainer(DestroyData dataPtr)
{
    Container *conPtr = (Container *) dataPtr;

    if (conPtr->highlightGC != NULL) {
	Tk_FreeGC(conPtr->display, conPtr->highlightGC);
    }
    if (conPtr->flags & CONTAINER_INIT) {
	Tk_DeleteGenericHandler(AdoptedWindowEventProc, conPtr);
    }
    if (conPtr->tkToplevel != NULL) {
	Tk_DeleteEventHandler(conPtr->tkToplevel, StructureNotifyMask, 
		ToplevelEventProc, conPtr);
    }
    Blt_FreeOptions(configSpecs, (char *)conPtr, conPtr->display, 0);
    Blt_Free(conPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureContainer --
 *
 * 	This procedure is called to process an objv/objc list, plus the Tk
 * 	option database, in order to configure (or reconfigure) the widget.
 *
 * Results:
 *	The return value is a standard TCL result.  If TCL_ERROR is
 * 	returned, then interp->result contains an error message.
 *
 * Side Effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for conPtr; old resources get freed, if there
 *	were any.  The widget is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureContainer(
    Tcl_Interp *interp,			/* Interpreter to report errors. */
    Container *conPtr,			/* Information about widget; may or
					 * may not already have values for
					 * some fields. */
    int objc,
    Tcl_Obj *const *objv,
    int flags)
{
    XGCValues gcValues;
    unsigned long gcMask;
    GC newGC;
    int width, height;

    if (Blt_ConfigureWidgetFromObj(interp, conPtr->tkwin, configSpecs, 
	objc, objv, (char *)conPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    conPtr->inset = conPtr->borderWidth + conPtr->highlightWidth;
    if (Tk_WindowId(conPtr->tkwin) == None) {
	Tk_MakeWindowExist(conPtr->tkwin);
    }
    if (GetAdoptedWindowGeometry(interp, conPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Blt_ConfigModified(configSpecs, "-window", "-name", "-command", 
	   (char *)NULL)) {
	conPtr->flags &= ~CONTAINER_MAPPED;
	if (conPtr->adopted != None) {
	    if (Blt_ReparentWindow(conPtr->display, conPtr->adopted,
		    Tk_WindowId(conPtr->tkwin), conPtr->inset,
		    conPtr->inset) != TCL_OK) {
		Tcl_AppendResult(interp, "can't adopt window \"", 
			 NameOfId(conPtr->display, conPtr->adopted), 
			 "\"", (char *)NULL);
		return TCL_ERROR;
	    }
	    XSelectInput(conPtr->display, conPtr->adopted, 
		 StructureNotifyMask);
	    if ((conPtr->flags & CONTAINER_INIT) == 0) {
		Tk_CreateGenericHandler(AdoptedWindowEventProc, conPtr);
		conPtr->flags |= CONTAINER_INIT;
	    }
	}
    }
    /* Add the designated inset to the requested dimensions. */
    width = conPtr->origWidth + 2 * conPtr->inset; 
    height = conPtr->origHeight + 2 * conPtr->inset;
    if (conPtr->reqWidth > 0) {
	width = conPtr->reqWidth;
    } 
    if (conPtr->reqHeight > 0) {
	height = conPtr->reqHeight;
    } 
    /* Set the requested width and height for the container. */
    if ((Tk_ReqWidth(conPtr->tkwin) != width) ||
	(Tk_ReqHeight(conPtr->tkwin) != height)) {
	Tk_GeometryRequest(conPtr->tkwin, width, height);
    }

    /*
     * GC for focus highlight.
     */
    gcMask = GCForeground;
    gcValues.foreground = conPtr->highlightColor->pixel;
    newGC = Tk_GetGC(conPtr->tkwin, gcMask, &gcValues);
    if (conPtr->highlightGC != NULL) {
	Tk_FreeGC(conPtr->display, conPtr->highlightGC);
    }
    conPtr->highlightGC = newGC;

    EventuallyRedraw(conPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ContainerInstCmdDeleteProc --
 *
 *	This procedure can be called if the window was destroyed (tkwin will
 *	be NULL) and the command was deleted automatically.  In this case, we
 *	need to do nothing.
 *
 *	Otherwise this routine was called because the command was deleted.
 *	Then we need to clean-up and destroy the widget.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The widget is destroyed.
 *
 *---------------------------------------------------------------------------
 */
static void
ContainerInstCmdDeleteProc(
    ClientData clientData)		/* Pointer to widget record for
					 * widget. */
{
    Container *conPtr = clientData;

    if (conPtr->tkwin != NULL) {
	Tk_Window tkwin;

	tkwin = conPtr->tkwin;
	conPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ContainerCmd --
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
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ContainerCmd(
    ClientData clientData,		/* Main window associated with
					 * interpreter. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Argument strings. */
{
    Container *conPtr;
    Tk_Window tkwin;
    unsigned int mask;
    char *path;

    if (objc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " pathName ?option value?...\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    tkwin = Tk_MainWindow(interp);
    path = Tcl_GetString(objv[1]);
    tkwin = Tk_CreateWindowFromPath(interp, tkwin, path, (char *)NULL);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    conPtr = Blt_AssertCalloc(1, sizeof(Container));
    conPtr->tkwin = tkwin;
    conPtr->display = Tk_Display(tkwin);
    conPtr->interp = interp;
    conPtr->flags = 0;
    conPtr->timeout = SEARCH_INTERVAL;
    conPtr->borderWidth = conPtr->highlightWidth = 2;
    conPtr->relief = TK_RELIEF_SUNKEN;
    Tk_SetClass(tkwin, "BltContainer");
    Blt_SetWindowInstanceData(tkwin, conPtr);

    if ((conPtr->flags & CONTAINER_INIT) == 0) {
	Tk_CreateGenericHandler(AdoptedWindowEventProc, conPtr);
	conPtr->flags |= CONTAINER_INIT;
    }
    {
#ifdef notdef
    	XSetWindowAttributes attr;
	attr.event_mask = SubstructureRedirectMask | SubstructureNotifyMask;
	    
	XChangeWindowAttributes(conPtr->display, Tk_RootWindow(tkwin), 
				CWEventMask, &attr);
#endif
	XSelectInput(conPtr->display, DefaultRootWindow(conPtr->display), 
		     SubstructureNotifyMask | 
		     StructureNotifyMask);
    }
    if (ConfigureContainer(interp, conPtr, objc - 2, objv + 2, 0) != TCL_OK) {
	Tk_DestroyWindow(conPtr->tkwin);
	return TCL_ERROR;
    }
    mask = (StructureNotifyMask | ExposureMask | FocusChangeMask);
    Tk_CreateEventHandler(tkwin, mask, ContainerEventProc, conPtr);
    conPtr->cmdToken = Tcl_CreateObjCommand(interp, path, ContainerInstCmd,
	conPtr, ContainerInstCmdDeleteProc);

    Tk_MakeWindowExist(tkwin);
    Tcl_SetObjResult(interp, objv[1]);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DisplayContainer --
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
DisplayContainer(ClientData clientData)
{
    Container *conPtr = clientData;
    Drawable drawable;
    int width, height;

    conPtr->flags &= ~CONTAINER_REDRAW;
    if (conPtr->tkwin == NULL) {
	return;			/* Window has been destroyed. */
    }
    if (!Tk_IsMapped(conPtr->tkwin)) {
	return;
    }
    drawable = Tk_WindowId(conPtr->tkwin);
#ifndef WIN32
    if (conPtr->tkToplevel == NULL) {
	Window window;
	Tk_Window tkToplevel;

	/* Create an event handler for the toplevel of the container. */
	tkToplevel = Blt_Toplevel(conPtr->tkwin);
	window = Blt_GetWindowId(tkToplevel);
	conPtr->tkToplevel = Tk_IdToWindow(conPtr->display, window); 
	if (conPtr->tkToplevel != NULL) {
	    Tk_CreateEventHandler(conPtr->tkToplevel, StructureNotifyMask, 
		ToplevelEventProc, conPtr);
	}
    }
#endif /* WIN32 */
    if (conPtr->adopted != None) {
#ifndef WIN32
	if (conPtr->flags & CONTAINER_MOVE) {
	    /* 
	     * Some applications like Netscape cache its location to position
	     * its popup menus. But when it's reparented, it thinks it's
	     * always at the same position.  It doesn't know when the
	     * container's moved.  The hack here is to force the application
	     * to update its coordinates by moving the adopted window (over by
	     * 1 pixel and then back in case the application is comparing the
	     * last location).
	     */
	    XMoveWindow(conPtr->display, conPtr->adopted,
			conPtr->inset + 1, conPtr->inset + 1);
	    XMoveWindow(conPtr->display, conPtr->adopted,
			conPtr->inset, conPtr->inset);
	    conPtr->flags &= ~CONTAINER_MOVE;
	}
#endif /* WIN32 */
	/* Compute the available space inside the container. */
	width = Tk_Width(conPtr->tkwin) - (2 * conPtr->inset);
	height = Tk_Height(conPtr->tkwin) - (2 * conPtr->inset);

	if ((conPtr->adoptedX != conPtr->inset) || 
	    (conPtr->adoptedY != conPtr->inset) ||
	    (conPtr->adoptedWidth != width) || 
	    (conPtr->adoptedHeight != height)) {
	    /* Resize the window to the new size */
	    if (width < 1) {
		width = 1;
	    }
	    if (height < 1) {
		height = 1;
	    }
	    XMoveResizeWindow(conPtr->display, conPtr->adopted,
		conPtr->inset, conPtr->inset, width, height);
	    conPtr->adoptedWidth = width;
	    conPtr->adoptedHeight = height;
	    conPtr->adoptedX = conPtr->adoptedY = conPtr->inset;
	    if (conPtr->tkAdopted != NULL) {
		Tk_ResizeWindow(conPtr->tkAdopted, width, height);
	    }
	}
#ifndef WIN32
	if (!(conPtr->flags & CONTAINER_MAPPED)) {
	    XMapWindow(conPtr->display, conPtr->adopted);
	    conPtr->flags |= CONTAINER_MAPPED;
	}
#endif
	if (conPtr->borderWidth > 0) {
	    Blt_Draw3DRectangle(conPtr->tkwin, drawable, conPtr->border,
		conPtr->highlightWidth, conPtr->highlightWidth,
		Tk_Width(conPtr->tkwin) - 2 * conPtr->highlightWidth,
		Tk_Height(conPtr->tkwin) - 2 * conPtr->highlightWidth,
		conPtr->borderWidth, conPtr->relief);
	}
    } else {
	Blt_Fill3DRectangle(conPtr->tkwin, drawable, conPtr->border,
	    conPtr->highlightWidth, conPtr->highlightWidth,
	    Tk_Width(conPtr->tkwin) - 2 * conPtr->highlightWidth,
	    Tk_Height(conPtr->tkwin) - 2 * conPtr->highlightWidth,
	    conPtr->borderWidth, conPtr->relief);
    }

    /* Draw focus highlight ring. */
    if (conPtr->highlightWidth > 0) {
	XColor *color;
	GC gc;

	color = (conPtr->flags & CONTAINER_FOCUS)
	    ? conPtr->highlightColor : conPtr->highlightBgColor;
	gc = Tk_GCForColor(color, drawable);
	Tk_DrawFocusHighlight(conPtr->tkwin, gc, conPtr->highlightWidth,
	    drawable);
    }
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * SendOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SendOp(conPtr, interp, objc, objv)
    Container *conPtr;
    Tcl_Interp *interp;
    int objc;			/* Not used. */
    Tcl_Obj *const *objv;
{

    if (conPtr->adopted != None) {
	XEvent event;
	char *p;
	KeySym symbol;
	int xid;
	Window window;

	if (Tcl_GetIntFromObj(interp, objv[2], &xid) != TCL_OK) {
	    return TCL_ERROR;
	}
	window = (Window)xid;
	event.xkey.type = KeyPress;
	event.xkey.serial = 0;
	event.xkey.display = conPtr->display;
	event.xkey.window = event.xkey.subwindow = window;
	event.xkey.time = CurrentTime;
	event.xkey.x = event.xkey.x = 100;
	event.xkey.root = Tk_RootWindow(conPtr->tkwin);	
	event.xkey.x_root = Tk_X(conPtr->tkwin) + conPtr->inset;
	event.xkey.x_root = Tk_Y(conPtr->tkwin) + conPtr->inset;
	event.xkey.state = 0;
	event.xkey.same_screen = TRUE;
	
	
	for (p = Tcl_GetString(objv[3]); *p != '\0'; p++) {
	    if (*p == '\r') {
		symbol = XStringToKeysym("Return");
	    } else if (*p == ' ') {
		symbol = XStringToKeysym("space");
	    } else {
		char save;

		save = *(p+1);
		*(p+1) = '\0';
		symbol = XStringToKeysym(p);
		*(p+1) = save;
	    }
	    event.xkey.keycode = XKeysymToKeycode(conPtr->display, symbol);
	    event.xkey.type = KeyPress;
	    if (!XSendEvent(conPtr->display, window, False, KeyPress, &event)) {
		fprintf(stderr, "send press event failed\n");
	    }
	    event.xkey.type = KeyRelease;
	    if (!XSendEvent(conPtr->display, window, False, KeyRelease, 
			    &event)) {
		fprintf(stderr, "send release event failed\n");
	    }
	}
    }
    return TCL_OK;
}
#endif

#ifndef WIN32
/*
 *---------------------------------------------------------------------------
 *
 * FindOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FindOp(
    Container *conPtr,
    Tcl_Interp *interp,
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    Window root;
    SearchInfo search;
    char *string;

    memset(&search, 0, sizeof(search));
    search.pattern = Tcl_GetString(objv[3]);
    Tcl_DStringInit(&search.ds);
    
    search.saveNames = TRUE;		/* Indicates to record all matching
					 * XIDs. */
    string = Tcl_GetString(objv[2]);
    if (strcmp(string, "-name") == 0) {
	search.proc = SearchForName;
    } else if (strcmp(string, "-command") == 0) {
	search.proc = SearchForCommand;
    } else {
	Tcl_AppendResult(interp, "missing \"-name\" or \"-command\" switch",
			 (char *)NULL);
	return TCL_ERROR;
    }
    root = Tk_RootWindow(conPtr->tkwin);
    (*search.proc)(conPtr->display, root, &search);
    Tcl_DStringResult(interp, &search.ds);
    return TCL_OK;
}
#endif /*WIN32*/

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
    Container *conPtr,
    Tcl_Interp *interp,
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    return Blt_ConfigureValueFromObj(interp, conPtr->tkwin, configSpecs,
	(char *)conPtr, objv[2], 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * ConfigureOp --
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
 *	set for conPtr; old resources get freed, if there were any.  The
 *	widget is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(
    Container *conPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    if (objc == 2) {
	return Blt_ConfigureInfoFromObj(interp, conPtr->tkwin, configSpecs,
	    (char *)conPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, conPtr->tkwin, configSpecs,
	    (char *)conPtr, objv[2], 0);
    }
    if (ConfigureContainer(interp, conPtr, objc - 2, objv + 2,
	    BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    EventuallyRedraw(conPtr);
    return TCL_OK;
}


#ifndef WIN32
static int
IgnoreErrors(Display *display, XErrorEvent *eventPtr)
{
    return 0;
}


static int
GetAtomName(Display *display, Atom atom, char **namePtr)
{
    char *atomName;
    XErrorHandler handler;
    static char name[200];
    int result;

    handler = XSetErrorHandler(IgnoreErrors);
    atomName = XGetAtomName(display, atom);
    XSetErrorHandler(handler);

    name[0] = '\0';
    if (atomName == NULL) {
	sprintf(name, "undefined atom # 0x%lx", atom);
	result = FALSE;
    } else {
	size_t length = strlen(atomName);

	if (length > 200) {
	    length = 200;
	}
	memcpy(name, atomName, length);
	name[length] = '\0';
	XFree(atomName);
	result = TRUE;
    }
    *namePtr = name;
    return result;
}

static void
FillTree(Container *conPtr, Window window, Blt_Tree tree, Blt_TreeNode parent)
{
    char string[200];
    Atom *atoms;
    int i;
    int numProps;
    Blt_Chain chain;

    /* Process the window's descendants. */
    atoms = XListProperties(conPtr->display, window, &numProps);
    for (i = 0; i < numProps; i++) {
	char *name;

	if (GetAtomName(conPtr->display, atoms[i], &name)) {
	    char *data;
	    int result, format;
	    Atom typeAtom;
	    unsigned long numItems, bytesAfter;
	    
	    result = XGetWindowProperty(
		conPtr->display,	/* Display of window. */
		window,			/* Window holding the property. */
		atoms[i],		/* Name of property. */
	        0,			/* Offset of data (for multiple
					 * reads). */
		GetMaxPropertySize(conPtr->display), 
					/* Maximum number of items to read. */
		False,			/* If true, delete the property. */
	        XA_STRING,		/* Desired type of property. */
	        &typeAtom,		/* (out) Actual type of the
					 * property. */
	        &format,		/* (out) Actual format of the
					 * property. */
	        &numItems,		/* (out) # of items in specified
					 * format. */
	        &bytesAfter,		/* (out) # of bytes remaining to be
					 * read. */
		(unsigned char **)&data);
#ifdef notdef
	    fprintf(stderr, "%x: property name is %s (format=%d(%d) type=%d result=%d)\n", window, name, format, numItems, typeAtom, result == Success);
#endif
	    if (result == Success) {
		if (format == 8) {
		    if (data != NULL) {
			Blt_Tree_SetValue(conPtr->interp, tree, parent, name, 
				Tcl_NewStringObj(data, numItems));
		    }
		} else if (typeAtom == XA_WINDOW) {
		    int *iPtr = (int *)&data;
		    sprintf(string, "0x%x", *iPtr);
		    Blt_Tree_SetValue(conPtr->interp, tree, parent, name, 
			Tcl_NewStringObj(string, -1));
		} else {
		    Blt_Tree_SetValue(conPtr->interp, tree, parent, name, 
			Tcl_NewStringObj("???", -1));
		}
		XFree(data);
	    }
	}
    }	
    if (atoms != NULL) {
	XFree(atoms);
    }
    chain = GetChildren(conPtr->display, window);
    if (chain != NULL) {
	Blt_ChainLink link;

	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	     link = Blt_Chain_NextLink(link)) {
	    Blt_TreeNode child;
	    char *wmName;
	    Window w;

	    w = (Window)Blt_Chain_GetValue(link);
	    sprintf(string, "0x%x", (int)w);
	    if (XFetchName(conPtr->display, w, &wmName)) {
		child = Blt_Tree_CreateNode(tree, parent, wmName, -1);
		if (w == 0x220001c) {
		    fprintf(stderr, "found xterm (%s)\n", wmName);
		}
		XFree(wmName);
	    } else {
		child = Blt_Tree_CreateNode(tree, parent, string, -1);
	    }
	    if (w == 0x220001c) {
		fprintf(stderr, "found xterm (%s) node=%ld\n", string,
			(long)Blt_Tree_NodeId(child));
	    }
	    Blt_Tree_SetValue(conPtr->interp, tree, child, "id", 
			      Tcl_NewStringObj(string, -1));
	    FillTree(conPtr, w, tree, child);
	}
	Blt_Chain_Destroy(chain);
    }

}

/*
 *---------------------------------------------------------------------------
 *
 * TreeOp --
 *
 * .c tree $tree
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TreeOp(
    Container *conPtr,
    Tcl_Interp *interp,
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    Window root;
    Blt_TreeNode node;
    char string[200];
    Blt_Tree tree;

    tree = Blt_Tree_GetFromObj(interp, objv[2]);
    if (tree == NULL) {
	return TCL_ERROR;
    }
    node = Blt_Tree_RootNode(tree);
    Blt_Tree_RelabelNode(tree, node, "root");
    root = Tk_RootWindow(conPtr->tkwin);
    sprintf(string, "0x%ux", (unsigned int)root);
    Blt_Tree_SetValue(interp, tree, node, "id", Tcl_NewStringObj(string, -1));
    FillTree(conPtr, root, tree, node);
    return TCL_OK;
}
#endif /*WIN32*/

/*
 *---------------------------------------------------------------------------
 *
 * ContainerCmd --
 *
 * 	This procedure is invoked to process the "container" command.  See the
 * 	user documentation for details on what it does.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec opSpecs[] =
{
    {"cget",      2, CgetOp, 3, 3, "option",},
    {"configure", 2, ConfigureOp, 2, 0, "?option value?...",},
#ifndef WIN32
    {"find",      1, FindOp, 3, 4, "?-command|-name? pattern",},
#endif /*WIN32*/
#ifdef notdef
    {"send",      1, SendOp, 4, 4, "window string",},
#endif
#ifndef WIN32
    {"tree",      1, TreeOp, 3, 3, "treeName",},
#endif /*WIN32*/
};

static int numSpecs = sizeof(opSpecs) / sizeof(Blt_OpSpec);

static int
ContainerInstCmd(
    ClientData clientData,		/* Information about the widget. */
    Tcl_Interp *interp,			/* Interpreter to report errors back
					 * to. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Vector of argument strings. */
{
    ContainerCmdProc *proc;
    Container *conPtr = clientData;
    int result;

    proc = Blt_GetOpFromObj(interp, numSpecs, opSpecs, BLT_OP_ARG1, objc, objv, 
	    0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    Tcl_Preserve(conPtr);
    result = (*proc)(conPtr, interp, objc, objv);
    Tcl_Release(conPtr);
    return result;
}

int
Blt_ContainerCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "container", ContainerCmd, };

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#endif /* NO_CONTAINER */


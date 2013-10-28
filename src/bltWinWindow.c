/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*
 * bltWinWindow.c --
 *
 * This module implements additional window functionality for the BLT toolkit,
 * such as transparent Tk windows, and reparenting Tk windows.
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

#include <X11/Xlib.h>
#include "tkDisplay.h"

/*
 *---------------------------------------------------------------------------
 *
 * WindowToHandle --
 *
 *---------------------------------------------------------------------------
 */
static HWND
WindowToHandle(Tk_Window tkwin)
{
    HWND hWnd;
    Window window;
    
    window = Tk_WindowId(tkwin);
    if (window == None) {
	Tk_MakeWindowExist(tkwin);
    }
    hWnd = Tk_GetHWND(Tk_WindowId(tkwin));
    if (Tk_IsTopLevel(tkwin)) {
	hWnd = GetParent(hWnd);
    }
    return hWnd;
}

/*
 *---------------------------------------------------------------------------
 *
 * DoConfigureNotify --
 *
 *	Generate a ConfigureNotify event describing the current configuration
 *	of a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An event is generated and processed by Tk_HandleEvent.
 *
 *---------------------------------------------------------------------------
 */
static void
DoConfigureNotify(Tk_FakeWin *winPtr) /* Window whose configuration
				       * was just changed. */
{
    XEvent event;

    event.type = ConfigureNotify;
    event.xconfigure.serial = LastKnownRequestProcessed(winPtr->display);
    event.xconfigure.send_event = False;
    event.xconfigure.display = winPtr->display;
    event.xconfigure.event = winPtr->window;
    event.xconfigure.window = winPtr->window;
    event.xconfigure.x = winPtr->changes.x;
    event.xconfigure.y = winPtr->changes.y;
    event.xconfigure.width = winPtr->changes.width;
    event.xconfigure.height = winPtr->changes.height;
    event.xconfigure.border_width = winPtr->changes.border_width;
    if (winPtr->changes.stack_mode == Above) {
	event.xconfigure.above = winPtr->changes.sibling;
    } else {
	event.xconfigure.above = None;
    }
    event.xconfigure.override_redirect = winPtr->atts.override_redirect;
    Tk_HandleEvent(&event);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MakeTransparentWindowExist --
 *
 *	Similar to Tk_MakeWindowExist but instead creates a transparent window
 *	to block for user events from sibling windows.
 *
 *	Differences from Tk_MakeWindowExist.
 *
 *	1. This is always a "busy" window. There's never a platform-specific
 *	   class procedure to execute.
 *
 *	2. The window is transparent and never will have children, so
 *	   colormap information is irrelevant.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the procedure returns, the internal window associated with tkwin
 *	is guaranteed to exist.  This may require the window's ancestors to be
 *	created too.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MakeTransparentWindowExist(
    Tk_Window tkwin,		/* Token for window. */
    Window parent,		/* Parent window. */
    int isBusy)			/*  */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkWindow *winPtr2;
    Tcl_HashEntry *hPtr;
    int notUsed;
    TkDisplay *dispPtr;
    HWND hParent;
    int style;
    DWORD exStyle;
    HWND hWnd;

    if (winPtr->window != None) {
	return;			/* Window already exists. */
    }
    /* Create a transparent window and put it on top.  */

    hParent = (HWND) parent;
    style = (WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
    exStyle = (WS_EX_TRANSPARENT | WS_EX_TOPMOST);
#define TK_WIN_CHILD_CLASS_NAME "TkChild"
    hWnd = CreateWindowEx(exStyle, TK_WIN_CHILD_CLASS_NAME, NULL, style,
	Tk_X(tkwin), Tk_Y(tkwin), Tk_Width(tkwin), Tk_Height(tkwin),
	hParent, NULL, (HINSTANCE)Tk_GetHINSTANCE(), NULL);
    winPtr->window = Tk_AttachHWND(tkwin, hWnd);

    dispPtr = winPtr->dispPtr;
    hPtr = Tcl_CreateHashEntry(&dispPtr->winTable, (char *)winPtr->window,
	&notUsed);
    Tcl_SetHashValue(hPtr, winPtr);
    winPtr->dirtyAtts = 0;
    winPtr->dirtyChanges = 0;
#ifdef TK_USE_INPUT_METHODS
    winPtr->inputContext = NULL;
#endif /* TK_USE_INPUT_METHODS */
    if (!(winPtr->flags & TK_TOP_LEVEL)) {
	/*
	 * If any siblings higher up in the stacking order have already been
	 * created then move this window to its rightful position in the
	 * stacking order.
	 *
	 * NOTE: this code ignores any changes anyone might have made to the
	 * sibling and stack_mode field of the window's attributes, so it
	 * really isn't safe for these to be manipulated except by calling
	 * Tk_RestackWindow.
	 */
	for (winPtr2 = winPtr->nextPtr; winPtr2 != NULL;
	    winPtr2 = winPtr2->nextPtr) {
	    if ((winPtr2->window != None) && !(winPtr2->flags & TK_TOP_LEVEL)) {
		XWindowChanges changes;
		changes.sibling = winPtr2->window;
		changes.stack_mode = Below;
		XConfigureWindow(winPtr->display, winPtr->window,
		    CWSibling | CWStackMode, &changes);
		break;
	    }
	}
    }
    /*
     * Issue a ConfigureNotify event if there were deferred configuration
     * changes (but skip it if the window is being deleted; the
     * ConfigureNotify event could cause problems if we're being called from
     * Tk_DestroyWindow under some conditions).
     */
    if ((winPtr->flags & TK_NEED_CONFIG_NOTIFY)
	&& !(winPtr->flags & TK_ALREADY_DEAD)) {
	winPtr->flags &= ~TK_NEED_CONFIG_NOTIFY;
	DoConfigureNotify((Tk_FakeWin *)tkwin);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetWindowRegion --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Blt_GetWindowRegion(
    Display *display,		/* Not used. */
    Window window,
    int *xPtr, int *yPtr, int *widthPtr, int *heightPtr)
{
    int result;
    RECT region;
    TkWinWindow *winPtr = (TkWinWindow *)window;

    result = GetWindowRect(winPtr->handle, &region);
    if (!result) {
	return TCL_ERROR;
    }
    if (xPtr != NULL) {
	*xPtr = region.left;
    }
    if (yPtr != NULL) {
	*yPtr = region.top;
    }
    if (widthPtr != NULL) {
	*widthPtr = region.right - region.left;
    }
    if (heightPtr != NULL) {
	*heightPtr = region.bottom - region.top;
    }
    return TCL_OK;
}

#ifdef notdef
int
Blt_GetRootCoords(Display *display, Window window, int *xPtr, int *yPtr, 
		  int *widthPtr, int *heightPtr)
{
    int result;
    RECT region;
    TkWinWindow *winPtr = (TkWinWindow *)window;

    result = GetWindowRect(winPtr->handle, &region);
    if (!result) {
	return TCL_ERROR;
    }
    if (xPtr != NULL) {
	*xPtr = region.left;
    }
    if (yPtr != NULL) {
	*yPtr = region.top;
    }
    if (widthPtr != NULL) {
	*widthPtr = region.right - region.left;
    }
    if (heightPtr != NULL) {
	*heightPtr = region.bottom - region.top;
    }
    return TCL_OK;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetWindowId --
 *
 *      Returns the XID for the Tk_Window given.  Starting in Tk 8.0, the
 *      toplevel widgets are wrapped by another window.  Currently there's no
 *      way to get at that window, other than what is done here: query the X
 *      window hierarchy and grab the parent.
 *
 * Results:
 *      Returns the X Window ID of the widget.  If it's a toplevel, then * the
 *      XID of the wrapper is returned.
 *
 *---------------------------------------------------------------------------
 */
Window
Blt_GetWindowId(Tk_Window tkwin)
{
    return (Window) WindowToHandle(tkwin);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_RaiseTopLevelWindow --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_RaiseToplevelWindow(Tk_Window tkwin)
{
    SetWindowPos(WindowToHandle(tkwin), HWND_TOP, 0, 0, 0, 0,
	SWP_NOMOVE | SWP_NOSIZE);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LowerToplevelWindow --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_LowerToplevelWindow(Tk_Window tkwin)
{
    SetWindowPos(WindowToHandle(tkwin), HWND_BOTTOM, 0, 0, 0, 0,
	SWP_NOMOVE | SWP_NOSIZE);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MapToplevelWindow --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MapToplevelWindow(Tk_Window tkwin)
{
    ShowWindow(WindowToHandle(tkwin), SW_SHOWNORMAL);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_UnmapToplevelWindow --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_UnmapToplevelWindow(Tk_Window tkwin)
{
    ShowWindow(WindowToHandle(tkwin), SW_HIDE);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MoveResizeToplevelWindow --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MoveResizeToplevelWindow(Tk_Window tkwin, int x, int y, int w, int h)
{
    SetWindowPos(WindowToHandle(tkwin), HWND_TOP, x, y, w, h, 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MoveToplevelWindow --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MoveToplevelWindow(Tk_Window tkwin, int x, int y)
{
    SetWindowPos(WindowToHandle(tkwin), HWND_TOP, x, y, 0, 0, 
		 SWP_NOSIZE | SWP_NOZORDER);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResizeToplevelWindow --
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ResizeToplevelWindow(Tk_Window tkwin, int w, int h)
{
    SetWindowPos(WindowToHandle(tkwin), HWND_TOP, 0, 0, w, h, 
	SWP_NOMOVE | SWP_NOZORDER);
}

int
Blt_ReparentWindow(
    Display *display, 
    Window window, 
    Window newParent, 
    int x, int y)
{
    XReparentWindow(display, window, newParent, x, y);
    return TCL_OK;
}

int
Blt_GetWindowFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Window *windowPtr)
{
    char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] == '.') {
	Tk_Window tkwin;

	tkwin = Tk_NameToWindow(interp, string, Tk_MainWindow(interp));
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
	if (Tk_WindowId(tkwin) == None) {
	    Tk_MakeWindowExist(tkwin);
	}
	*windowPtr = (Tk_IsTopLevel(tkwin)) ? Blt_GetWindowId(tkwin) : 
	    Tk_WindowId(tkwin);
    } else if (strcmp(string, "root") == 0) {
	*windowPtr = Tk_RootWindow(Tk_MainWindow(interp));
    } else {
	static TkWinWindow tkWinWindow;
	int id;

	if (Tcl_GetIntFromObj(interp, objPtr, &id) != TCL_OK) {
	    return TCL_ERROR;
	}
	tkWinWindow.handle = (HWND)id;
	tkWinWindow.winPtr = NULL;
	tkWinWindow.type = TWD_WINDOW;
	*windowPtr = (Window)&tkWinWindow;
    }
    return TCL_OK;
}


Window
Blt_GetParentWindow(Display *display, Window window) 
{
    HWND hWnd;
    
    hWnd = Tk_GetHWND(window);
    hWnd = GetParent(hWnd);
    return (Window)hWnd;
}

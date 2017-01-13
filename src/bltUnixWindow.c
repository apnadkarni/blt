/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltUnixWindow.c --
 *
 * This module implements additional window functionality for the BLT
 * toolkit, such as transparent Tk windows, and reparenting Tk
 * windows.
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

#include <X11/Xlib.h>

#ifndef WIN32
  #include <X11/Xproto.h>
#endif  /* WIN32 */

#include "tkDisplay.h"

#define DEBUG 0

/*
 *---------------------------------------------------------------------------
 *
 * DoConfigureNotify --
 *
 *      Generate a ConfigureNotify event describing the current
 *      configuration of a window.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      An event is generated and processed by Tk_HandleEvent.
 *
 *---------------------------------------------------------------------------
 */
static void
DoConfigureNotify(Tk_FakeWin *winPtr)
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
 *      Similar to Tk_MakeWindowExist but instead creates a
 *      transparent window to block for user events from sibling
 *      windows.
 *
 *      Differences from Tk_MakeWindowExist.
 *
 *      1. This is always a "busy" window. There's never a
 *         platform-specific class procedure to execute instead.
 *      2. The window is transparent and never will contain children,
 *         so colormap information is irrelevant.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      When the procedure returns, the internal window associated
 *      with tkwin is guaranteed to exist.  This may require the
 *      window's ancestors to be created too.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MakeTransparentWindowExist(Tk_Window tkwin, Window parent, int isBusy)
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkWindow *winPtr2;
    Tcl_HashEntry *hPtr;
    int notUsed;
    TkDisplay *dispPtr;
    long int mask;

    if (winPtr->window != None) {
        return;                 /* Window already exists. */
    }

    /* Create a transparent window and put it on top.  */

    mask = (!isBusy) ? 0 : (CWDontPropagate | CWEventMask);
    /* Ignore the important events while the window is mapped.  */
#define USER_EVENTS  (EnterWindowMask | LeaveWindowMask | KeyPressMask | \
        KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | \
        PointerMotionMask)
#define PROP_EVENTS  (KeyPressMask | KeyReleaseMask | ButtonPressMask | \
        ButtonReleaseMask | PointerMotionMask)

    winPtr->atts.do_not_propagate_mask = PROP_EVENTS;
    winPtr->atts.event_mask = USER_EVENTS;
    winPtr->changes.border_width = 0;
    winPtr->depth = 0; 

    winPtr->window = XCreateWindow(winPtr->display, parent,
        winPtr->changes.x, winPtr->changes.y,
        (unsigned)winPtr->changes.width,        /* width */
        (unsigned)winPtr->changes.height,       /* height */
        (unsigned)winPtr->changes.border_width, /* border_width */
        winPtr->depth,          /* depth */
        InputOnly,              /* class */
        winPtr->visual,         /* visual */
        mask,                   /* valuemask */
        &winPtr->atts           /* attributes */ );

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
         * If any siblings higher up in the stacking order have already
         * been created then move this window to its rightful position
         * in the stacking order.
         *
         * NOTE: this code ignores any changes anyone might have made
         * to the sibling and stack_mode field of the window's attributes,
         * so it really isn't safe for these to be manipulated except
         * by calling Tk_RestackWindow.
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
     * changes (but skip it if the window is being deleted;  the
     * ConfigureNotify event could cause problems if we're being called
     * from Tk_DestroyWindow under some conditions).
     */
    if ((winPtr->flags & TK_NEED_CONFIG_NOTIFY)
        && !(winPtr->flags & TK_ALREADY_DEAD)) {
        winPtr->flags &= ~TK_NEED_CONFIG_NOTIFY;
        DoConfigureNotify((Tk_FakeWin *) tkwin);
    }
}

static int
XQueryTreeErrorProc(ClientData clientData, XErrorEvent *errEventPtr)
{
    int *codePtr = clientData;

#if DEBUG
    fprintf(stderr, "XQueryTree failed\n");
#endif
    *codePtr = TCL_ERROR;
    return 0;
}

Window
Blt_GetParentWindow(Display *display, Window window)
{
    Status status;
    Tk_ErrorHandler handler;
    Window *children;
    Window root, parent;
    int code;
    unsigned int numChildren;
    
    code = TCL_OK;
    handler = Tk_CreateErrorHandler(display, -1, X_QueryTree, -1, 
                XQueryTreeErrorProc, &code);
    status = XQueryTree(display, window, &root, &parent, &children,
                &numChildren);
    Tk_DeleteErrorHandler(handler);
    if ((status > 0) && (code == TCL_OK)) {
        XFree(children);
        return parent;
    }
    return None;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_GetWindowId --
 *
 *      Returns the XID for the Tk_Window given.  Starting in Tk 8.0,
 *      the toplevel widgets are wrapped by another window.  Currently
 *      there's no way to get at that window, other than what is done
 *      here: query the X window hierarchy and retrieve the parent.
 *
 * Results:
 *      Returns the X Window ID of the widget.  If it's a toplevel, then
 *      the XID of the wrapper is returned.
 *
 *---------------------------------------------------------------------------
 */
Window
Blt_GetWindowId(Tk_Window tkwin)
{
    Window window;

    Tk_MakeWindowExist(tkwin);
    window = Tk_WindowId(tkwin);
    if (Tk_IsTopLevel(tkwin)) {
        Window parent;

        parent = Blt_GetParentWindow(Tk_Display(tkwin), window);
        if (parent != Tk_RootWindow(tkwin)) {
            window = parent;
        }
    }
    return window;
}

/*
 *---------------------------------------------------------------------------
 *
 * XGeometryErrorProc --
 *
 *      Flags errors generated from XGetGeometry calls to the X server.
 *
 * Results:
 *      Always returns 0.
 *
 * Side Effects:
 *      Sets a flag, indicating an error occurred.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
XGeometryErrorProc(ClientData clientData, XErrorEvent *errEventPtr)
{
    int *codePtr = clientData;

#if DEBUG
    fprintf(stderr, "XGeometry failed\n");
#endif
    *codePtr = TCL_ERROR;
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * XTranslateCoordsErrorProc --
 *
 *      Flags errors generated from XGetGeometry calls to the X server.
 *
 * Results:
 *      Always returns 0.
 *
 * Side Effects:
 *      Sets a flag, indicating an error occurred.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
XTranslateCoordsErrorProc(ClientData clientData, XErrorEvent *errEventPtr)
{
    int *codePtr = clientData;

#if DEBUG
    fprintf(stderr, "XTranslateCoordinates failed err=%d, req=%d\n",
            errEventPtr->error_code, errEventPtr->request_code);
#endif
    *codePtr = TCL_ERROR;
    return 0;
}


int
Blt_GetWindowRegion(Display *display, Window window, int *xPtr, int *yPtr, 
                    int *widthPtr, int *heightPtr)
{
    Tk_ErrorHandler handler;
    Window root;
    int code;
    int x, y;
    unsigned int w, h, bw, depth;
    Status status;
    
    code = TCL_OK;
    handler = Tk_CreateErrorHandler(display, -1, X_GetGeometry, -1, 
        XGeometryErrorProc, &code);
    status = XGetGeometry(display, window, &root, &x, &y, &w, &h, &bw, &depth);
    Tk_DeleteErrorHandler(handler);
    XSync(display, False);
    if ((status == 0) || (code != TCL_OK)) {
        Blt_Warn("failed to get window region\n");
        return TCL_ERROR;
    }
    if ((xPtr != NULL) || (yPtr != NULL)) {
        Window child;
        Tk_ErrorHandler handler;
        int xRoot, yRoot;

        handler = Tk_CreateErrorHandler(display, -1, X_TranslateCoords, -1,
                XTranslateCoordsErrorProc, &code);
        status = XTranslateCoordinates(display, window, root, 0, 0,
                &xRoot, &yRoot, &child);
        XSync(display, False);
        Tk_DeleteErrorHandler(handler);
        if ((status) && (code == TCL_OK)) {
            if (xPtr != NULL) {
                *xPtr = xRoot;
            }
            if (yPtr != NULL) {
                *yPtr = yRoot;
            }
        } else {
            Blt_Warn("failed to translate coordinates x=%x y=%d\n", x, y);
            return TCL_ERROR;
        }
    }
    if (widthPtr != NULL) {
        *widthPtr = (int)w;
    }
    if (heightPtr != NULL) {
        *heightPtr = (int)h;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_RaiseToplevelWindow --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_RaiseToplevelWindow(Tk_Window tkwin)
{
    XRaiseWindow(Tk_Display(tkwin), Blt_GetWindowId(tkwin));
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_LowerToplevelWindow --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_LowerToplevelWindow(Tk_Window tkwin)
{
    XLowerWindow(Tk_Display(tkwin), Blt_GetWindowId(tkwin));
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResizeToplevelWindow --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_ResizeToplevelWindow(Tk_Window tkwin, int width, int height)
{
    XResizeWindow(Tk_Display(tkwin), Blt_GetWindowId(tkwin), width, height);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MoveResizeToplevelWindow --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MoveResizeToplevelWindow(Tk_Window tkwin, int x, int y, int w, int h)
{
    XMoveResizeWindow(Tk_Display(tkwin), Blt_GetWindowId(tkwin), x, y, w, h);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ResizeToplevelWindow --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MoveToplevelWindow(Tk_Window tkwin, int x, int y)
{
    XMoveWindow(Tk_Display(tkwin), Blt_GetWindowId(tkwin), x, y);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_MapToplevelWindow --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MapToplevelWindow(Tk_Window tkwin)
{
    XMapWindow(Tk_Display(tkwin), Blt_GetWindowId(tkwin));
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_UnmapToplevelWindow --
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_UnmapToplevelWindow(Tk_Window tkwin)
{
    XUnmapWindow(Tk_Display(tkwin), Blt_GetWindowId(tkwin));
}

/* ARGSUSED */
static int
XReparentWindowErrorProc(ClientData clientData, XErrorEvent *errEventPtr)
{
    int *codePtr = clientData;

    *codePtr = TCL_ERROR;
    return 0;
}

int
Blt_ReparentWindow(Display *display, Window window, Window newParent, 
                   int x, int y)
{
    Status status;
    Tk_ErrorHandler handler;
    int code;
    
    code = TCL_OK;
    handler = Tk_CreateErrorHandler(display, -1, X_ReparentWindow, -1,
        XReparentWindowErrorProc, &code);
    status = XReparentWindow(display, window, newParent, x, y);
    Tk_DeleteErrorHandler(handler);
    XSync(display, False);
    if ((status) && (code == TCL_OK)) {
        return TCL_OK;
    }
    return TCL_ERROR;
}

int
Blt_GetWindowFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Window *windowPtr)
{
    const char *string;
    int id;

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
    } else if (Tcl_GetIntFromObj(NULL, objPtr, &id) == TCL_OK) {
        *windowPtr = (Window)id;
    } else {
        Tcl_AppendResult(interp, "can't find window \"", string, "\"",
                         (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

const char *
Blt_GetWindowName(Display *display, Window window)
{
    char *fetchedName;
    
    if (XFetchName(display, window, &fetchedName)) {
        static char name[200+1];

        strncpy(name, fetchedName, 200);
        name[200] = '\0';
        XFree(fetchedName);
        return name;
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 *  Blt_GetChildrenFromWindow --
 *
 *      Returns a list of the child windows according to their stacking
 *      order.  The window handles are ordered from top to bottom.
 *
 *---------------------------------------------------------------------------
 */
Blt_Chain
Blt_GetChildrenFromWindow(Display *display, Window window)
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

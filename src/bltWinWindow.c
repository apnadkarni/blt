/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltWinWindow.c --
 *
 * This module implements additional window functionality for the BLT toolkit,
 * such as transparent Tk windows, and reparenting Tk windows.
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
 *      Generate a ConfigureNotify event describing the current configuration
 *      of a window.
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
 *      Similar to Tk_MakeWindowExist but instead creates a transparent window
 *      to block for user events from sibling windows.
 *
 *      Differences from Tk_MakeWindowExist.
 *
 *      1. This is always a "busy" window. There's never a platform-specific
 *         class procedure to execute.
 *
 *      2. The window is transparent and never will have children, so
 *         colormap information is irrelevant.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      When the procedure returns, the internal window associated with tkwin
 *      is guaranteed to exist.  This may require the window's ancestors to be
 *      created too.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_MakeTransparentWindowExist(
    Tk_Window tkwin,            /* Token for window. */
    Window parent,              /* Parent window. */
    int isBusy)                 /*  */
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
        return;                 /* Window already exists. */
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
Blt_GetWindowRegion(Display *display, Window window, int *xPtr, int *yPtr, 
                    int *widthPtr, int *heightPtr)
{
    HWND hWnd;
    RECT r;
    TkWinWindow *winPtr = (TkWinWindow *)window;

    /* Root window in Tk has a NULL handle.  Have to handle it specially. */
    hWnd = winPtr->handle;
    if (winPtr->handle == NULL) {
       hWnd = WindowFromDC(GetDC(NULL));
    }
    if (!GetWindowRect(hWnd, &r)) {
        return TCL_ERROR;
    }
    if (xPtr != NULL) {
        *xPtr = r.left;
    }
    if (yPtr != NULL) {
        *yPtr = r.top;
    }
    if (widthPtr != NULL) {
        *widthPtr = r.right - r.left;
    }
    if (heightPtr != NULL) {
        *heightPtr = r.bottom - r.top;
    }
    return TCL_OK;
}

const char *
Blt_GetWindowName(Display *display, Window window)
{
    static char name[200+1];
    HWND hWnd = (HWND)window;
    
    GetWindowText(hWnd, name, 200);
    name[200] = '\0';
    return name;
}

#ifdef notdef
int
Blt_GetRootCoords(Display *display, Window window, int *xPtr, int *yPtr, 
                  int *widthPtr, int *heightPtr)
{
    int result;
    RECT r;
    TkWinWindow *winPtr = (TkWinWindow *)window;

    result = GetWindowRect(winPtr->handle, &r);
    if (!result) {
        return TCL_ERROR;
    }
    if (xPtr != NULL) {
        *xPtr = r.left;
    }
    if (yPtr != NULL) {
        *yPtr = r.top;
    }
    if (widthPtr != NULL) {
        *widthPtr = r.right - r.left;
    }
    if (heightPtr != NULL) {
        *heightPtr = r.bottom - r.top;
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
 *      None.
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
 *      None.
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
 *      None.
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
 *      None.
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
 *      None.
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
 *      None.
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
 *      None.
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
Blt_ReparentWindow(Display *display, Window window, Window newParent,
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
        tkWinWindow.handle = (HWND)(size_t)id;
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
    HWND parent = (HWND)window;
    Blt_Chain chain;
    HWND hWnd;

    chain = Blt_Chain_Create();
    for (hWnd = GetTopWindow(parent); hWnd != NULL;
        hWnd = GetNextWindow(hWnd, GW_HWNDNEXT)) {
        Blt_Chain_Append(chain, (ClientData)hWnd);
    }
    return chain;
}


static int
InitMetaFileHeader(Tk_Window tkwin, int width, int height, APMHEADER *mfhPtr)
{
    unsigned int *p;
    unsigned int sum;
#define MM_INCH         25.4
    int xdpi, ydpi;

    mfhPtr->key = 0x9ac6cdd7L;
    mfhPtr->hmf = 0;
    mfhPtr->inch = 1440;

    Blt_ScreenDPI(tkwin, &xdpi, &ydpi);
    mfhPtr->bbox.Left = mfhPtr->bbox.Top = 0;
    mfhPtr->bbox.Bottom = (SHORT)((width * 1440)/ (float)xdpi);
    mfhPtr->bbox.Right = (SHORT)((height * 1440) / (float)ydpi);
    mfhPtr->reserved = 0;
    sum = 0;
    for (p = (unsigned int *)mfhPtr; 
         p < (unsigned int *)&(mfhPtr->checksum); p++) {
        sum ^= *p;
    }
    mfhPtr->checksum = sum;
    return TCL_OK;
}

static int
CreateAPMetaFile(Tcl_Interp *interp, HANDLE hMetaFile, HDC hDC,
                 APMHEADER *mfhPtr, const char *fileName)
{
    HANDLE hFile;
    HANDLE hMem;
    LPVOID buffer;
    int result;
    DWORD count, numBytes;

    result = TCL_ERROR;
    hMem = NULL;

    hFile = CreateFile(
       fileName,                        /* File path */
       GENERIC_WRITE,                   /* Access mode */
       0,                               /* No sharing. */
       NULL,                            /* Security attributes */
       CREATE_ALWAYS,                   /* Overwrite any existing file */
       FILE_ATTRIBUTE_NORMAL,
       NULL);                           /* No template file */

    if (hFile == INVALID_HANDLE_VALUE) {
        Tcl_AppendResult(interp, "can't create metafile \"", fileName, 
                "\":", Blt_LastError(), (char *)NULL);
        return TCL_ERROR;
    }
    if ((!WriteFile(hFile, (LPVOID)mfhPtr, sizeof(APMHEADER), &count, 
                NULL)) || (count != sizeof(APMHEADER))) {
        Tcl_AppendResult(interp, "can't create metafile header to \"", 
                         fileName, "\":", Blt_LastError(), (char *)NULL);
        goto error;
    }
    numBytes = GetWinMetaFileBits(hMetaFile, 0, NULL, MM_ANISOTROPIC, hDC);
    hMem = GlobalAlloc(GHND, numBytes);
    if (hMem == NULL) {
        Tcl_AppendResult(interp, "can't create allocate global memory:", 
                Blt_LastError(), (char *)NULL);
        goto error;
    }
    buffer = (LPVOID)GlobalLock(hMem);
    if (!GetWinMetaFileBits(hMetaFile, numBytes, buffer, MM_ANISOTROPIC, hDC)) {
        Tcl_AppendResult(interp, "can't get metafile bits:", 
                Blt_LastError(), (char *)NULL);
        goto error;
    }
    if ((!WriteFile(hFile, buffer, numBytes, &count, NULL)) ||
        (count != numBytes)) {
        Tcl_AppendResult(interp, "can't write metafile bits:", 
                Blt_LastError(), (char *)NULL);
        goto error;
    }
    result = TCL_OK;
 error:
    CloseHandle(hFile);
    if (hMem != NULL) {
        GlobalUnlock(hMem);
        GlobalFree(hMem);
    }
    return result;
}

int
Blt_DrawToMetaFile(Tcl_Interp *interp, Tk_Window tkwin, int emf,
                   const char *fileName, Blt_DrawCmdProc *proc,
                   ClientData clientData, int w, int h)
{
    Drawable drawable;
    HDC hRefDC, hDC;
    HENHMETAFILE hMetaFile;
    TkWinDC drawableDC;
    TkWinDCState state;
    int result;
    
    drawable = Tk_WindowId(tkwin);
    hRefDC = TkWinGetDrawableDC(Tk_Display(tkwin), drawable, &state);
    {
        const char *title;
        Tcl_DString ds;

        Tcl_DStringInit(&ds);
        Tcl_DStringAppend(&ds, Tk_Class(tkwin), -1);
        Tcl_DStringAppend(&ds, BLT_VERSION, -1);
        Tcl_DStringAppend(&ds, "\0", -1);
        Tcl_DStringAppend(&ds, Tk_PathName(tkwin), -1);
        Tcl_DStringAppend(&ds, "\0", -1);
        title = Tcl_DStringValue(&ds);
        hDC = CreateEnhMetaFile(hRefDC, NULL, NULL, title);
        Tcl_DStringFree(&ds);
    }
    if (hDC == NULL) {
        Tcl_AppendResult(interp, "can't create metafile: ", Blt_LastError(),
                (char *)NULL);
        return TCL_ERROR;
    }
            
    drawableDC.hdc = hDC;
    drawableDC.type = TWD_WINDC;

    (*proc)(clientData, w, h, (Drawable)&drawableDC);

    hMetaFile = CloseEnhMetaFile(hDC);
    if (strcmp(fileName, "CLIPBOARD") == 0) {
        HWND hWnd;
        
        hWnd = Tk_GetHWND(drawable);
        OpenClipboard(hWnd);
        EmptyClipboard();
        SetClipboardData(CF_ENHMETAFILE, hMetaFile);
        CloseClipboard();
        TkWinReleaseDrawableDC(drawable, hRefDC, &state);
        return TCL_OK;
    } 

    result = TCL_ERROR;
    if (emf) {
        HENHMETAFILE hMetaFile2;
        
        hMetaFile2 = CopyEnhMetaFile(hMetaFile, fileName);
        if (hMetaFile2 != NULL) {
            DeleteEnhMetaFile(hMetaFile2); 
        }
        result = TCL_OK;
    } else {
        APMHEADER mfh;
        
        assert(sizeof(mfh) == 22);
        InitMetaFileHeader(tkwin, w, h, &mfh);
        result = CreateAPMetaFile(interp, hMetaFile, hRefDC, &mfh, fileName);
    }
    DeleteEnhMetaFile(hMetaFile); 
    TkWinReleaseDrawableDC(drawable, hRefDC, &state);
    return result;
}

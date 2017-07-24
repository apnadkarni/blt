/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltWindow.c --
 *
 * This module implements additional window functions for the BLT toolkit.
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

#include "bltAlloc.h"
#include "bltHash.h"
#include "tkDisplay.h"

typedef struct {
    Display *display;
    Drawable drawable;
} DrawableKey;

static Blt_HashTable attribTable;
static int initialized = FALSE;

Blt_DrawableAttributes *
Blt_GetDrawableAttributes(Display *display, Drawable drawable)
{
    if (drawable != None) {
        Blt_HashEntry *hPtr;
        DrawableKey key;

        if (!initialized) {
            Blt_InitHashTable(&attribTable, sizeof(DrawableKey)/sizeof(int));
            initialized = TRUE;
        }
        memset(&key, 0, sizeof(key));
        key.drawable = drawable;
        key.display = display;
        hPtr = Blt_FindHashEntry(&attribTable, &key);
        if (hPtr != NULL) {
            return Blt_GetHashValue(hPtr);
        }
    }
    return NULL;                        /* Don't have any information about
                                         * this drawable. */
}

void
Blt_SetDrawableAttributes(Display *display, Drawable drawable, int depth,
    int width, int height, Colormap colormap, Visual *visual)
{
    if (drawable != None) {
        Blt_DrawableAttributes *attrPtr;
        Blt_HashEntry *hPtr;
        int isNew;
        DrawableKey key;

        if (!initialized) {
            Blt_InitHashTable(&attribTable, sizeof(DrawableKey)/sizeof(int));
            initialized = TRUE;
        }
        memset(&key, 0, sizeof(key));
        key.drawable = drawable;
        key.display = display;
        hPtr = Blt_CreateHashEntry(&attribTable, &key, &isNew);
        if (isNew) {
            attrPtr = Blt_AssertMalloc(sizeof(Blt_DrawableAttributes));
            Blt_SetHashValue(hPtr, attrPtr);
            attrPtr->refCount = 1;
        }  else {
            attrPtr = Blt_GetHashValue(hPtr);
            attrPtr->refCount++;
        }
        /* Set or reset information for drawable. */
        attrPtr->id = drawable;
        attrPtr->depth = depth;
        attrPtr->colormap = colormap;
        attrPtr->visual = visual;
        attrPtr->width = width;
        attrPtr->height = height;
    }
}

void
Blt_SetDrawableAttributesFromWindow(Tk_Window tkwin, Drawable drawable)
{
    if (drawable != None) {
        Blt_SetDrawableAttributes(Tk_Display(tkwin), drawable, Tk_Width(tkwin), 
                Tk_Height(tkwin), Tk_Depth(tkwin), Tk_Colormap(tkwin), 
                Tk_Visual(tkwin));
    }
}

void
Blt_FreeDrawableAttributes(Display *display, Drawable drawable)
{
    Blt_HashEntry *hPtr;
    DrawableKey key;

    if (drawable == None) {
        return;
    }
    if (!initialized) {
        Blt_InitHashTable(&attribTable, sizeof(DrawableKey)/sizeof(int));
        initialized = TRUE;
    }
    memset(&key, 0, sizeof(key));
    key.drawable = drawable;
    key.display = display;
    hPtr = Blt_FindHashEntry(&attribTable, &key);
    if (hPtr != NULL) {
        Blt_DrawableAttributes *attrPtr;
            
        attrPtr = Blt_GetHashValue(hPtr);
        attrPtr->refCount--;
        if (attrPtr->refCount <= 0) {
            Blt_DeleteHashEntry(&attribTable, hPtr);
            Blt_Free(attrPtr);
        }
    }
}

#ifdef notdef
Blt_Draw *
Blt_GetPixmap(Display *display, Window id, int w, int h, int depth)
{
    pixmap = Tk_GetPixmap(display, id, w, h, depth);
    drawPtr = Blt_SetDrawableAttributes(display, pixmap, int depth,
        int w, int h, Colormap colormap, Visual *visual);
    drawPtr->id = pixmap;
    return drawPtr;
}
#endif

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FindChild --
 *
 *      Performs a linear search for the named child window in a given
 *      parent window.
 *
 *      This can be done via Tcl, but not through Tk's C API.  It's simple
 *      enough, if you peek into the Tk_Window structure.
 *
 * Results:
 *      The child Tk_Window. If the named child can't be found, NULL is
 *      returned.
 *
 *---------------------------------------------------------------------------
 */

/*LINTLIBRARY*/
Tk_Window
Blt_FindChild(Tk_Window parent, char *name)
{
    TkWindow *winPtr;
    TkWindow *parentPtr = (TkWindow *)parent;

    for (winPtr = parentPtr->childList; winPtr != NULL; 
        winPtr = winPtr->nextPtr) {
        if (strcmp(name, winPtr->nameUid) == 0) {
            return (Tk_Window)winPtr;
        }
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_FirstChild --
 *
 *      Returns the first child window of the named window. If the window
 *      has no children None is retured;
 *
 *---------------------------------------------------------------------------
 */
/*LINTLIBRARY*/
Tk_Window
Blt_FirstChild(Tk_Window parent)
{
    TkWindow *parentPtr = (TkWindow *)parent;
    return (Tk_Window)parentPtr->childList;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_NextChild --
 *
 *      Find the next sibling window to the given window.
 *
 *---------------------------------------------------------------------------
 */

/*LINTLIBRARY*/
Tk_Window
Blt_NextChild(Tk_Window tkwin)
{
    TkWindow *winPtr = (TkWindow *)tkwin;

    if (winPtr == NULL) {
        return NULL;
    }
    return (Tk_Window)winPtr->nextPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * UnlinkWindow --
 *
 *      This procedure removes a window from the childList of its parent.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The window is unlinked from its childList.
 *
 *---------------------------------------------------------------------------
 */
static void
UnlinkWindow(TkWindow *winPtr)
{
    TkWindow *prevPtr;

    prevPtr = winPtr->parentPtr->childList;
    if (prevPtr == winPtr) {
        winPtr->parentPtr->childList = winPtr->nextPtr;
        if (winPtr->nextPtr == NULL) {
            winPtr->parentPtr->lastChildPtr = NULL;
        }
    } else {
        while (prevPtr->nextPtr != winPtr) {
            prevPtr = prevPtr->nextPtr;
            if (prevPtr == NULL) {
                panic("UnlinkWindow couldn't find child in parent");
            }
        }
        prevPtr->nextPtr = winPtr->nextPtr;
        if (winPtr->nextPtr == NULL) {
            winPtr->parentPtr->lastChildPtr = prevPtr;
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_RelinkWindow --
 *
 *      Relinks a window into a new parent.  The window is unlinked from
 *      its original parent's child list and added onto the end of the new
 *      parent's list.
 *
 *      FIXME: If the window has focus, the focus should be moved to an
 *             ancestor.  Otherwise, Tk becomes confused about which
 *             Toplevel turns on focus for the window.  Right now this is
 *             done at the TCL layer.  For example, see blt::CreateTearoff
 *             in bltTabset.tcl.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The window is unlinked from its childList.
 *
 *---------------------------------------------------------------------------
 */
void
Blt_RelinkWindow(
    Tk_Window tkwin,            /* Child window to be linked. */
    Tk_Window newParent,
    int x, int y)
{
    TkWindow *winPtr, *parentWinPtr;

    if (Blt_ReparentWindow(Tk_Display(tkwin), Tk_WindowId(tkwin), 
                Tk_WindowId(newParent), x, y) != TCL_OK) {
        return;
    }
    winPtr = (TkWindow *)tkwin;
    parentWinPtr = (TkWindow *)newParent;

    winPtr->flags &= ~TK_REPARENTED;
    UnlinkWindow(winPtr);       /* Remove the window from its parent's list */

    /* Append the window onto the end of the parent's list of children */
    winPtr->parentPtr = parentWinPtr;
    winPtr->nextPtr = NULL;
    if (parentWinPtr->childList == NULL) {
        parentWinPtr->childList = winPtr;
    } else {
        parentWinPtr->lastChildPtr->nextPtr = winPtr;
    }
    parentWinPtr->lastChildPtr = winPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_Toplevel --
 *
 *      Climbs up the widget hierarchy to find the top level window of the
 *      window given.
 *
 * Results:
 *      Returns the Tk_Window of the toplevel widget.
 *
 *---------------------------------------------------------------------------
 */
Tk_Window
Blt_Toplevel(Tk_Window tkwin)
{
    while (!Tk_IsTopLevel(tkwin)) {
        tkwin = Tk_Parent(tkwin);
         if (tkwin == NULL) {
             return NULL;
         }
    }
    return tkwin;
}

void
Blt_RootCoordinates(
    Tk_Window tkwin, 
    int x, int y, 
    int *rootXPtr, int *rootYPtr)
{
    int vx, vy, vw, vh;
    int rootX, rootY;

    Tk_GetRootCoords(tkwin, &rootX, &rootY);
    x += rootX;
    y += rootY;
    Tk_GetVRootGeometry(tkwin, &vx, &vy, &vw, &vh);
    x += vx;
    y += vy;
    *rootXPtr = x;
    *rootYPtr = y;
}


/* Find the toplevel then  */
int
Blt_RootX(Tk_Window tkwin)
{
    int x;
    
    for (x = 0; tkwin != NULL;  tkwin = Tk_Parent(tkwin)) {
        x += Tk_X(tkwin) + Tk_Changes(tkwin)->border_width;
        if (Tk_IsTopLevel(tkwin)) {
            break;
        }
    }
    return x;
}

int
Blt_RootY(Tk_Window tkwin)
{
    int y;
    
    for (y = 0; tkwin != NULL;  tkwin = Tk_Parent(tkwin)) {
        y += Tk_Y(tkwin) + Tk_Changes(tkwin)->border_width;
        if (Tk_IsTopLevel(tkwin)) {
            break;
        }
    }
    return y;
}

void
Blt_SetWindowInstanceData(Tk_Window tkwin, ClientData instanceData)
{
    TkWindow *winPtr = (TkWindow *)tkwin;

    winPtr->instanceData = instanceData;
}

ClientData
Blt_GetWindowInstanceData(Tk_Window tkwin)
{
    TkWindow *winPtr;

    while (tkwin != NULL) {
        winPtr = (TkWindow *)tkwin;
        if (winPtr->instanceData != NULL) {
            return winPtr->instanceData;
        }
        tkwin = Tk_Parent(tkwin);
    }
    return NULL;
}

void
Blt_DeleteWindowInstanceData(Tk_Window tkwin)
{
    /* empty */
}


#ifdef HAVE_RANDR

#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>
#include <X11/extensions/randr.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xrender.h>     /* We share subpixel information */

typedef struct {
    int major, minor;                   /* XRandR version numbers. */
    int eventNum, errorNum;             /* Event offset of XRandr */
    Display *display;
    Tk_Window mainWindow;               /* Main window of interpreter. */
    Window root;                        /* Root window of screen. */
} XRandr;

/*
 *---------------------------------------------------------------------------
 *
 *  XRandrEventProc --
 *
 *      Invoked by Tk_HandleEvent whenever a ConfigureNotify or
 *      RRScreenChangeNotify event is received on the root window.
 *
 *---------------------------------------------------------------------------
 */
static int
XRandrEventProc(ClientData clientData, XEvent *eventPtr)
{
    XRandr *rrPtr = clientData;

    if (eventPtr->xany.window == rrPtr->root) {
        if ((eventPtr->type == (rrPtr->eventNum + RRScreenChangeNotify)) ||
            (eventPtr->type == ConfigureNotify)) {
            if (!XRRUpdateConfiguration(eventPtr)) {
                Blt_Warn("can't update screen configuration\n");
            }
        }
    }
    return 0;
}

void
Blt_InitXRandrConfig(Tcl_Interp *interp) 
{
    Tk_Window tkwin;
    static XRandr rr;

    tkwin = Tk_MainWindow(interp);
    rr.mainWindow = tkwin;
    rr.root = Tk_RootWindow(tkwin);
    rr.display = Tk_Display(tkwin);
    if (!XRRQueryExtension(rr.display, &rr.eventNum, &rr.errorNum)) {
        return;
    }
    if (!XRRQueryVersion(rr.display, &rr.major, &rr.minor)) {
        return;
    }
    Tk_CreateGenericHandler(XRandrEventProc, &rr);
    XRRSelectInput(rr.display, rr.root, RRScreenChangeNotifyMask);
#ifdef notdef
    XSelectInput(rr.display, rr.root, StructureNotifyMask);
#endif
}

#else 
void
Blt_InitXRandrConfig(Tcl_Interp *interp) 
{
}
#endif  /* HAVE_RANDR */

/* ARGSUSED */
void
Blt_SizeOfScreen(Tk_Window tkwin, int *widthPtr, int *heightPtr)
{
    *widthPtr = WidthOfScreen(Tk_Screen(tkwin));
    *heightPtr = HeightOfScreen(Tk_Screen(tkwin));
}


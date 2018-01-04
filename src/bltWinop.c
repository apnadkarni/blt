/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltWinop.c --
 *
 * This module implements simple window commands for the BLT toolkit.
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

#ifndef NO_WINOP

#ifdef HAVE_STRING_H
  #include <string.h>
#endif /* HAVE_STRING_H */

#ifndef WIN32
  #include <X11/Xatom.h>
  #include <X11/Xproto.h>
  #include <X11/Xutil.h>
#endif  /* WIN32 */

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "bltAlloc.h"
#include "bltPicture.h"
#include "bltChain.h"
#include "bltImage.h"
#include "tkDisplay.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define CLAMP(c)        ((((c) < 0.0) ? 0.0 : ((c) > 255.0) ? 255.0 : (c)))
static Tcl_ObjCmdProc WinopCmd;

typedef struct _WindowNode WindowNode;
static int selectTableInitialized = 0;
static Blt_HashTable selectTable;

/*
 *  WindowNode --
 *
 *      This structure represents a window hierarchy examined during a
 *      single "drag" operation.  It's used to cache information to reduce
 *      the round trip calls to the server needed to query window geometry
 *      information and grab the target property.
 */
struct _WindowNode {
    Display *display;
    Window window;              /* Window in hierarchy. */
    int initialized;            /* If zero, the rest of this structure's
                                 * information hasn't been set. */
    
    int x1, y1, x2, y2;         /* Extents of the window (upper-left and
                                 * lower-right corners). */
    
    WindowNode *parentPtr;      /* Parent node. NULL if root. Used to
                                 * compute offset for X11 windows. */
    Blt_Chain chain;            /* List of this window's children. If NULL,
                                 * there are no children. */
};

static int
GetRealizedWindowFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, 
                         Tk_Window *tkwinPtr)
{
    const char *string;
    Tk_Window tkwin;

    string = Tcl_GetString(objPtr);
    assert(interp != NULL);
    tkwin = Tk_NameToWindow(interp, string, Tk_MainWindow(interp));
    if (tkwin == NULL) {
        return TCL_ERROR;
    }
    if (Tk_WindowId(tkwin) == None) {
        Tk_MakeWindowExist(tkwin);
    }
    *tkwinPtr = tkwin;
    return TCL_OK;
}

static int
GetIdFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Window *windowPtr)
{
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] == '.') {
        Tk_Window tkwin;

        if (GetRealizedWindowFromObj(interp, objPtr, &tkwin) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Tk_IsTopLevel(tkwin)) {
            *windowPtr = Blt_GetWindowId(tkwin);
        } else {
            *windowPtr = Tk_WindowId(tkwin);
        }
    } else if (strcmp(string, "root") == 0) {
        *windowPtr = Tk_RootWindow(Tk_MainWindow(interp));
    } else {
        int xid;

        if (Tcl_GetIntFromObj(interp, objPtr, &xid) != TCL_OK) {
            return TCL_ERROR;
        }
#ifdef WIN32
        { 
            static TkWinWindow tkWinWindow;
            
            tkWinWindow.handle = (HWND)(size_t)xid;
            tkWinWindow.winPtr = NULL;
            tkWinWindow.type = TWD_WINDOW;
            *windowPtr = (Window)&tkWinWindow;
        }
#else
        *windowPtr = (Window)xid;
#endif
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetWindowNodeRegion --
 *
 *      Queries for the upper-left and lower-right corners of the given
 *      window.
 *
 *  Results:
 *      Returns if the window is currently viewable.  The coordinates of
 *      the window are returned via parameters.
 *
 * ------------------------------------------------------------------------ 
 */
static int
GetWindowNodeRegion(Display *display, WindowNode *nodePtr)
{
    int x, y, w, h;

    if (Blt_GetWindowExtents(display, nodePtr->window, &x, &y, &w, &h) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    nodePtr->x1 = x;
    nodePtr->y1 = y;
    nodePtr->x2 = x + w - 1;
    nodePtr->y2 = y + h - 1;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  FreeWindowNode --
 *
 *---------------------------------------------------------------------------
 */
static void
FreeWindowNode(WindowNode *parentPtr)   /* Window rep to be freed */
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(parentPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        WindowNode *childPtr;

        childPtr = Blt_Chain_GetValue(link);
        FreeWindowNode(childPtr);       /* Recursively free children. */
    }
    Blt_Chain_Destroy(parentPtr->chain);
    Blt_Free(parentPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 *  InitWindowNode --
 *
 *---------------------------------------------------------------------------
 */
static void
InitWindowNode(WindowNode *nodePtr) 
{
    Blt_ChainLink link;
    Blt_Chain chain;
    
    if (nodePtr->initialized) {
        return;
    }
    /* Query for the window coordinates.  */
    if (GetWindowNodeRegion(nodePtr->display, nodePtr) != TCL_OK) {
        return;
    }
    
    /* Add offset from parent's origin to coordinates */
    if (nodePtr->parentPtr != NULL) {
        nodePtr->x1 += nodePtr->parentPtr->x1;
        nodePtr->y1 += nodePtr->parentPtr->y1;
        nodePtr->x2 += nodePtr->parentPtr->x1;
        nodePtr->y2 += nodePtr->parentPtr->y1;
    }
    /*
     * Collect a list of child windows, sorted in z-order.  The
     * topmost window will be first in the list.
     */
    chain = Blt_GetChildrenFromWindow(nodePtr->display, nodePtr->window);
    
    /* Add and initialize extra slots if needed. */
    for (link = Blt_Chain_FirstLink(chain); link != NULL;
         link = Blt_Chain_NextLink(link)) {
        WindowNode *childPtr;
        
        childPtr = Blt_AssertCalloc(1, sizeof(WindowNode));
        childPtr->initialized = FALSE;
        childPtr->window = (Window)Blt_Chain_GetValue(link);
        childPtr->display = nodePtr->display;
        childPtr->parentPtr = nodePtr;
        Blt_Chain_SetValue(link, childPtr);
    }
    nodePtr->chain = chain;
    nodePtr->initialized = TRUE;
}

static WindowNode *
GetRoot(Display *display)
{
    WindowNode *rootPtr;

    rootPtr = Blt_AssertCalloc(1, sizeof(WindowNode));
    rootPtr->window = DefaultRootWindow(display);
    rootPtr->display = display;
    InitWindowNode(rootPtr);
    return rootPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 *  FindTopWindow --
 *
 *      Searches for the topmost window at a given pair of X-Y coordinates.
 *
 *  Results:
 *      Returns a pointer to the node representing the window containing
 *      the point.  If one can't be found, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static WindowNode *
FindTopWindow(WindowNode *rootPtr, int x, int y)
{
    Blt_ChainLink link;
    WindowNode *nodePtr;

    if ((x < rootPtr->x1) || (x > rootPtr->x2) ||
        (y < rootPtr->y1) || (y > rootPtr->y2)) {
        return NULL;            /* Point is not over window  */
    }
    nodePtr = rootPtr;

    /*  
     * The window list is ordered top to bottom, so stop when we find the
     * first child that contains the X-Y coordinate. It will be the topmost
     * window in that hierarchy.  If none exists, then we already have the
     * topmost window.  
     */
  descend:
    for (link = Blt_Chain_FirstLink(rootPtr->chain); link != NULL;
        link = Blt_Chain_NextLink(link)) {
        rootPtr = Blt_Chain_GetValue(link);
        if (!rootPtr->initialized) {
            InitWindowNode(rootPtr);
        }
        if ((x >= rootPtr->x1) && (x <= rootPtr->x2) &&
            (y >= rootPtr->y1) && (y <= rootPtr->y2)) {
            /*   
             * Remember the last window containing the pointer and descend
             * into its window hierarchy. We'll look for a child that also
             * contains the pointer.  
             */
            nodePtr = rootPtr;
            goto descend;
        }
    }
    return nodePtr;
}

#if defined(HAVE_RANDR) && defined(HAVE_XRRGETSCREENRESOURCES)
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xproto.h>

#ifdef HAVE_X11_EXTENSIONS_RANDR_H
  #include <X11/extensions/randr.h>
#endif

#ifdef HAVE_X11_EXTENSIONS_XRANDR_H
  #include <X11/extensions/Xrandr.h>
#endif

static int 
FindMode(XRRScreenResources *resourcesPtr, const char *name)
{
    char c;
    int i;

    c = name[0];
    for (i = 0; i < resourcesPtr->nmode; ++i) {
        const char *modeName;

        modeName = resourcesPtr->modes[i].name;
        if ((c == modeName[0]) && (strcmp(modeName, name) == 0)) {
            return resourcesPtr->modes[i].id;
        }
    }
    return -1;
}

#ifdef notdef
static int 
PrintModes(XRRScreenResources *resPtr)
{
    int i;

    for (i = 0; i < resPtr->nmode; ++i) {
        fprintf(stderr, "%dx%d mode=%s\n", resPtr->modes[i].width,
                resPtr->modes[i].height, resPtr->modes[i].name);
    }
    return -1;
}
#endif

#endif  /* HAVE_RANDR && HAVE_DECL_XRRGETSCREENRESOURCES */

#ifndef WIN32

static int
GetMaxPropertySize(Display *display)
{
    int numBytes;

    numBytes = Blt_MaxRequestSize(display, sizeof(char));
    numBytes -= 32;
    return numBytes;
}

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
    static char name[256];
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

        if (length > 255) {
            length = 255;
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
GetWindowProperties(Tcl_Interp *interp, Display *display, Window window,
                    Blt_Tree tree, Blt_TreeNode parent)
{
    Atom *atoms;
    int i;
    int numProps;

    /* Process the window's descendants. */
    atoms = XListProperties(display, window, &numProps);
    for (i = 0; i < numProps; i++) {
        char *name;

        if (GetAtomName(display, atoms[i], &name)) {
            union {
                unsigned char *data;
                int window;
            } prop;
            int result, format;
            Atom typeAtom;
            unsigned long numItems, numBytesAfter;
            
            result = XGetWindowProperty(
                display,                /* Display of window. */
                window,                 /* Window holding the property. */
                atoms[i],               /* Name of property. */
                0,                      /* Offset of data (for multiple
                                         * reads). */
                GetMaxPropertySize(display), 
                                        /* Maximum # of items to read. */
                False,                  /* If true, delete the property. */
                XA_STRING,              /* Desired type of property. */
                &typeAtom,              /* (out) Actual type of the
                                         * property. */
                &format,                /* (out) Actual format of the
                                         * property. */
                &numItems,              /* (out) # of items in specified
                                         * format. */
                &numBytesAfter,         /* (out) # of bytes remaining to be
                                         * read. */
                &prop.data);
#ifdef notdef
            fprintf(stderr, "%x: property name is %s (format=%d(%d) type=%d result=%d)\n", window, name, format, numItems, typeAtom, result == Success);
#endif
            if (result == Success) {
                Tcl_Obj *objPtr;

                if ((format == 8) && (prop.data != NULL)) {
                    objPtr = Tcl_NewStringObj((char *)prop.data, numItems);
                } else if ((typeAtom == XA_WINDOW) && (format == 32)) {
                    char string[200];

                    sprintf(string, "0x%x", prop.window);
                    objPtr = Tcl_NewStringObj(string, -1);
                } else {
                    objPtr = Tcl_NewStringObj("???", 3);
                }
                Blt_Tree_SetValue(interp, tree, parent, name, objPtr);
                XFree(prop.data);
            }
        }
    }   
    if (atoms != NULL) {
        XFree(atoms);
    }
}
#endif /*!WIN32*/

static void
FillTree(Tcl_Interp *interp, Display *display, Window window, Blt_Tree tree,
         Blt_TreeNode parent)
{
    Blt_Chain chain;

#ifndef WIN32
    GetWindowProperties(interp, display, window, tree, parent);
#endif
    chain = Blt_GetChildrenFromWindow(display, window);
    if (chain != NULL) {
        Blt_ChainLink link;

        for (link = Blt_Chain_FirstLink(chain); link != NULL;
             link = Blt_Chain_NextLink(link)) {
            Blt_TreeNode child;
            const char *name;
            char ident[200];
            Window w;

            w = (Window)Blt_Chain_GetValue(link);
            sprintf(ident, "0x%x", (int)w);
            name = Blt_GetWindowName(display, w);
            if (name != NULL) {
                child = Blt_Tree_CreateNode(tree, parent, name, -1);
            } else {
                child = Blt_Tree_CreateNode(tree, parent, ident, -1);
            }
            Blt_Tree_SetValue(interp, tree, child, "id", 
                              Tcl_NewStringObj(ident, -1));
            FillTree(interp, display, w, tree, child);
        }
        Blt_Chain_Destroy(chain);
    }

}


/*
 *---------------------------------------------------------------------------
 *
 * LostSelection --
 *
 *      This procedure is called back by Tk when the selection is grabbed
 *      away.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The existing selection is unhighlighted, and the window is marked
 *      as not containing a selection.
 *
 *---------------------------------------------------------------------------
 */
static void
LostSelection(ClientData clientData)
{
    Tk_Window tkMain = clientData;
    Blt_HashEntry *hPtr;

    assert(selectTableInitialized);
    hPtr = Blt_FindHashEntry(&selectTable, tkMain);
    if (hPtr != NULL) {
        Tcl_DString *dsPtr;

        dsPtr = Blt_GetHashValue(hPtr);
        Tcl_DStringFree(dsPtr);
        Blt_Free(dsPtr);
        Blt_DeleteHashEntry(&selectTable, hPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionProc --
 *
 *      This procedure is called back by Tk when the selection is requested
 *      by someone.  It returns part or all of the selection in a buffer
 *      provided by the caller.
 *
 * Results:
 *      The return value is the number of non-NULL bytes stored at buffer.
 *      Buffer is filled (or partially filled) with a NUL-terminated string
 *      containing part or all of the selection, as given by offset and
 *      maxBytes.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static int
SelectionProc(
    ClientData clientData,              /* Information about the widget. */
    int offset,                         /* Offset within selection of first
                                         * character to be returned. */
    char *buffer,                       /* Location in which to place
                                         * selection. */
    int maxBytes)                       /* Maximum number of bytes to place
                                         * at buffer, not including
                                         * terminating NULL character. */
{
    Tcl_DString *dsPtr = clientData;
    int size;

    size = Tcl_DStringLength(dsPtr) - offset;
    strncpy(buffer, Tcl_DStringValue(dsPtr) + offset, maxBytes);
    buffer[maxBytes] = '\0';
    return (size > maxBytes) ? maxBytes : size;
}

/*
 *---------------------------------------------------------------------------
 *
 *  ChangesOp --
 *
 * ------------------------------------------------------------------------ 
 */
static int
ChangesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
          Tcl_Obj *const *objv)
{
    Tk_Window tkwin;

    if (GetRealizedWindowFromObj(interp, objv[2], &tkwin) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tk_IsTopLevel(tkwin)) {
        XSetWindowAttributes attrs;
        Window id;
        unsigned int mask;

        id = Blt_GetWindowId(tkwin);
        attrs.backing_store = WhenMapped;
        attrs.save_under = True;
        mask = CWBackingStore | CWSaveUnder;
        XChangeWindowAttributes(Tk_Display(tkwin), id, mask, &attrs);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  GeometryOp --
 *
 *      Retrieves the geometry from the named window.
 *
 * ------------------------------------------------------------------------ 
 */
static int
GeometryOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    Window id;
    int x, y, w, h;
    Tcl_Obj *listObjPtr;

    if (GetIdFromObj(interp, objv[2], &id) != TCL_OK) {
        return TCL_ERROR;
    }
    Blt_GetWindowExtents(Tk_Display(tkMain), id, &x, &y, &w, &h);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(w));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(h));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  LowerOp --
 *
 *      Lowers the named windows in the stacking order.
 *
 *      blt::winop map ?windowName ...?
 *
 * ------------------------------------------------------------------------ 
 */
static int
LowerOp(ClientData clientData, Tcl_Interp *interp, int objc, 
        Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        Window id;

        if (GetIdFromObj(interp, objv[i], &id) != TCL_OK) {
            return TCL_ERROR;
        }
        XLowerWindow(Tk_Display(tkMain), id);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  MapOp --
 *
 *      Maps the named windows.
 *
 *      blt::winop map ?windowName ...?
 *
 * ------------------------------------------------------------------------ 
 */
static int
MapOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        Window id;

        if (GetIdFromObj(interp, objv[i], &id) != TCL_OK) {
            return TCL_ERROR;
        }
        XMapWindow(Tk_Display(tkMain), id);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  InsideOp --
 *
 *      Indicates if the given point in root coordinates is inside
 *      the named window.
 *
 *      blt::winop inside windowName x y
 *
 * ------------------------------------------------------------------------ 
 */
static int
InsideOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    Window id;
    int rootX, rootY;
    int state;
    int x, y, w, h;

    if (GetIdFromObj(interp, objv[2], &id) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((Tcl_GetIntFromObj(interp, objv[3], &rootX) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[4], &rootY) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (Blt_GetWindowExtents(Tk_Display(tkMain), id, &x, &y, &w, &h) != TCL_OK){
        return TCL_ERROR;
    }
    state = FALSE;
    fprintf(stderr, "x=%d y=%d x1=%d y1=%d x2=%d y2=%d\n",
            rootX, rootY, x, y, x + w, y + h);
    if ((rootX >= x) && (rootX < (x+w)) && (rootY >= y) && (rootY < (y+h))) {
        state = TRUE;
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  MoveOp --
 *
 *      Moves the window to the designated coordinate.
 *
 *      blt::winop move windowName screenX screenY
 *
 * ------------------------------------------------------------------------ 
 */
static int
MoveOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    int x, y;
    Window id;

    if (GetIdFromObj(interp, objv[2], &id) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tk_GetPixelsFromObj(interp, tkMain, objv[3], &x) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tk_GetPixelsFromObj(interp, tkMain, objv[4], &y) != TCL_OK) {
        return TCL_ERROR;
    }
    XMoveWindow(Tk_Display(tkMain), id, x, y);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  QueryOp --
 *
 *      Queries the position of the pointer.
 *
 *      blt::winop query 
 *
 * ------------------------------------------------------------------------ 
 */
static int
QueryOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    int rootX, rootY, childX, childY;
    Window root, child;
    unsigned int mask;

    /* GetCursorPos */
    if (XQueryPointer(Tk_Display(tkMain), Tk_WindowId(tkMain), &root,
            &child, &rootX, &rootY, &childX, &childY, &mask)) {
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(rootX));
        Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(rootY));
        Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  RaiseOp --
 *
 *      Raises the window in the stacking order.
 *
 *      blt::winop raise ?windowName ...?
 *
 * ------------------------------------------------------------------------ 
 */
static int
RaiseOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        Window id;

        if (GetIdFromObj(interp, objv[i], &id) != TCL_OK) {
            return TCL_ERROR;
        }
        XRaiseWindow(Tk_Display(tkMain), id);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionAppendOp
 *
 *      Add strings to the selection.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      blt::winop selection append ?string...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionAppendOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                  Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    Blt_HashEntry *hPtr;
    int isNew;
    Tcl_DString *dsPtr;
    int i;

    hPtr = Blt_CreateHashEntry(&selectTable, tkMain, &isNew);
    if (isNew) {
        dsPtr = Blt_AssertMalloc(sizeof(Tcl_DString));
        Tcl_DStringInit(dsPtr);
        Blt_SetHashValue(hPtr, dsPtr);
        Tk_CreateSelHandler(tkMain, XA_PRIMARY, XA_STRING, SelectionProc,
                            dsPtr, XA_STRING);
    }
    dsPtr = Blt_GetHashValue(hPtr);
    for (i = 3; i < objc; i++) {
        const char *string;
        int length;

        string = Tcl_GetStringFromObj(objv[i], &length);
        Tcl_DStringAppend(dsPtr, string, length);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionClearOp
 *
 *      Clears the selection.
 *
 *      blt::winop selection clear 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionClearOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&selectTable, tkMain);
    if (hPtr != NULL) {
        Tcl_DString *dsPtr;

        dsPtr = Blt_GetHashValue(hPtr);
        Tcl_DStringFree(dsPtr);
        Blt_Free(dsPtr);
        Blt_DeleteHashEntry(&selectTable, hPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionCurrentOp
 *
 *      Returns the current selection.
 *
 *      blt::winop selection current 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionCurrentOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&selectTable, tkMain);
    if (hPtr != NULL) {
        Tcl_DString *dsPtr;

        dsPtr = Blt_GetHashValue(hPtr);
        Tcl_SetStringObj(Tcl_GetObjResult(interp), Tcl_DStringValue(dsPtr),
                         Tcl_DStringLength(dsPtr));
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * SelectionExportOp
 *
 *      Exports the current selection.  It is not an error if not selection
 *      is present.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection is exported.
 *
 *      blt::winop selection export
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionExportOp(ClientData clientData, Tcl_Interp *interp, int objc,
                  Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&selectTable, tkMain);
    if (hPtr != NULL) {
        Tcl_DString *dsPtr;

        dsPtr = Blt_GetHashValue(hPtr);
        Tk_OwnSelection(tkMain, XA_PRIMARY, LostSelection, dsPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionPresentOp
 *
 *      Returns 1 if there is a selection and 0 if it isn't.
 *
 * Results:
 *      A standard TCL result.  interp->result will contain a boolean string
 *      indicating if there is a selection.
 *
 *      pathName selection present 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SelectionPresentOp(ClientData clientData, Tcl_Interp *interp, int objc,
                   Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    int state;
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&selectTable, tkMain);
    state = (hPtr != NULL);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SelectionOp --
 *
 *      This procedure handles the individual options for text selections.
 *      The selected text is designated by start and end indices into the text
 *      pool.  The selected segment has both a anchored and unanchored ends.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The selection changes.
 *
 *      pathName selection op args
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec selectionOps[] =
{
    {"append",   1, SelectionAppendOp,   3, 0, "?string...?",},
    {"clear",    2, SelectionClearOp,    3, 3, "",},
    {"current",  2, SelectionCurrentOp,  3, 3, "",},
    {"export",   1, SelectionExportOp,   3, 3, "",},
    {"present",  1, SelectionPresentOp,  3, 3, "",},
};
static int numSelectionOps = sizeof(selectionOps) / sizeof(Blt_OpSpec);

static int
SelectionOp(ClientData clientData, Tcl_Interp *interp, int objc, 
            Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    if (!selectTableInitialized) {
        Blt_InitHashTable(&selectTable, BLT_ONE_WORD_KEYS);
        selectTableInitialized = TRUE;
    }
    proc = Blt_GetOpFromObj(interp, numSelectionOps, selectionOps, BLT_OP_ARG2, 
        objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 *  ScreenSizeOp --
 *
 *      Sets the size of the screen.
 *
 *      blt::winop screensize width height
 *
 * ------------------------------------------------------------------------ 
 */
#if defined(HAVE_RANDR) && defined(HAVE_XRRGETSCREENRESOURCES)
static int
ScreenSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
                Tcl_Obj *const *objv)
{
    Display *display;
    int mode;
    Tk_Window tkMain = clientData;
    Window root;
    char modeName[200];
    XRRScreenResources *sr;
    XRRScreenConfiguration *sc;
    int w, h;

    display = Tk_Display(tkMain);
    root = Tk_RootWindow(tkMain);
    if ((Tk_GetPixelsFromObj(interp, tkMain, objv[2], &w) != TCL_OK) || 
        (Tk_GetPixelsFromObj(interp, tkMain, objv[3], &h) != TCL_OK)) {
        return TCL_ERROR;
    }
    {
        int n, m, eventBase, errorBase;

        if (!XRRQueryExtension(display, &eventBase, &errorBase) ||
            !XRRQueryVersion (display, &n, &m)) {
            Tcl_AppendResult(interp, "RandR extension missing", (char *)NULL);
            return TCL_ERROR;
        }
        fprintf(stderr, "Xrandr version %d.%d\n", n, m);
    }
    sr = XRRGetScreenResources(display, root);
    if (sr == NULL) {
        Tcl_AppendResult(interp, "can't get screen resources", (char *)NULL);
        return TCL_ERROR;
    }
    sprintf(modeName, "%dx%d", w, h);
    mode = FindMode(sr, modeName);
    if (mode < 0) {
        XRRModeInfo mi;

        memset(&mi, 0, sizeof(mi));
        mi.width = w;
        mi.height = h;
        mi.name = modeName;
        mi.nameLength = strlen(modeName);
        mode = XRRCreateMode(display, root, &mi);
        if (mode < 0) {
            return TCL_ERROR;
        }
        XRRAddOutputMode(display, sr->outputs[0], mode);
    }
    XRRFreeScreenResources(sr);

    sc = XRRGetScreenInfo(display, root);
    if (sc == NULL) {
        return TCL_ERROR;
    }
    {
        XRRScreenSize *screenSizes;
        int i, sizeIndex, numSizes;
        Rotation currentRotation;

        screenSizes = XRRConfigSizes(sc, &numSizes);
        sizeIndex = 0;
        for (i = 0; i < numSizes; i++) {
            if ((screenSizes[i].width == w) && (screenSizes[i].height == h)) {
                sizeIndex = i;
                break;
            }
        }
        if (i == numSizes) {
            XRRFreeScreenConfigInfo(sc);
            fprintf(stderr, 
                    "can't find matching screen configuration (%dx%d)\n",
                    w, h);
            Tcl_AppendResult(interp, "can't find matching screen configuration",
                             (char *)NULL);
            return TCL_ERROR;

        }
        XRRConfigRotations(sc, &currentRotation);
        XRRSetScreenConfig(display, sc, root, (SizeID)sizeIndex, 
                currentRotation, CurrentTime);
    }
    XRRFreeScreenConfigInfo(sc);
    return TCL_OK;
}
#endif  /* HAVE_RANDR && HAVE_XRRGETSCREENRESOURCES */

/*
 *---------------------------------------------------------------------------
 *
 *  TopOp --
 *
 *      Returns the ID of topmost window at the given screen position.
 *
 *      blt::winop top screenX screenY
 *
 * ------------------------------------------------------------------------ 
 */
static int
TopOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    WindowNode *rootPtr, *nodePtr;
    int x, y;

    if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) ||
        (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
        return TCL_ERROR;
    }
    rootPtr = GetRoot(Tk_Display(tkMain));
    nodePtr = FindTopWindow(rootPtr, x, y);
    if (nodePtr != NULL) {
        char string[200];
        
        sprintf(string, "0x%x", (unsigned int)nodePtr->window);
        Tcl_SetStringObj(Tcl_GetObjResult(interp), string , -1);
    }
    FreeWindowNode(rootPtr);
    return (nodePtr == NULL) ? TCL_ERROR : TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  TreeOp --
 *
 *      Returns the tree of windows in a BLT tree.
 *
 *      blt::winop tree windowName
 *
 * ------------------------------------------------------------------------ 
 */
static int
TreeOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Window root;
    Tk_Window tkMain = clientData;
    Blt_TreeNode node;
    char string[200];
    Blt_Tree tree;
    Display *display;

    if (GetIdFromObj(interp, objv[2], &root) != TCL_OK) {
        return TCL_ERROR;
    }
    tree = Blt_Tree_GetFromObj(interp, objv[3]);
    if (tree == NULL) {
        return TCL_ERROR;
    }
    display = Tk_Display(tkMain);
    node = Blt_Tree_RootNode(tree);
    Blt_Tree_RelabelNode(tree, node, "root");
    sprintf(string, "0x%ux", (unsigned int)root);
    Blt_Tree_SetValue(interp, tree, node, "id", Tcl_NewStringObj(string, -1));
    FillTree(interp, display, root, tree, node);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  UnmapOp --
 *
 *      Unmaps the named windows.
 *
 *      blt::winop unmap ?windowName ...?
 *
 * ------------------------------------------------------------------------ 
 */
static int
UnmapOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        Window id;

        if (GetIdFromObj(interp, objv[i], &id) != TCL_OK) {
            return TCL_ERROR;
        }
        XMapWindow(Tk_Display(tkMain), id);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  WarpToOp --
 *
 *      Warps the pointer to the given window or coordinates.
 *
 *      blt::winop warp windowName
 *      blt::winop warp rootX rootY 
 *
 * ------------------------------------------------------------------------ 
 */
static int
WarpToOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    if (objc == 3) {
        Tk_Window tkwin;

        if (GetRealizedWindowFromObj(interp, objv[2], &tkwin) != TCL_OK) {
            return TCL_ERROR;
        }
        if (!Tk_IsMapped(tkwin)) {
            Tcl_AppendResult(interp, "can't warp to unmapped window \"",
                     Tk_PathName(tkwin), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        XWarpPointer(Tk_Display(tkwin), None, Tk_WindowId(tkwin),
             0, 0, 0, 0, Tk_Width(tkwin) / 2, Tk_Height(tkwin) / 2);
    } else if (objc == 4) {
        int x, y;
        Window root;
        
        if ((Tk_GetPixelsFromObj(interp, tkMain, objv[2], &x) != TCL_OK) ||
            (Tk_GetPixelsFromObj(interp, tkMain, objv[3], &y) != TCL_OK)) {
            return TCL_ERROR;
        }
        root = Tk_RootWindow(tkMain);
        XWarpPointer(Tk_Display(tkMain), None, root, 0, 0, 0, 0, x, y);
    }
    return QueryOp(tkMain, interp, 0, (Tcl_Obj **)NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 *  UnmapOp --
 *
 *      Unmaps the named windows.
 *
 *      blt::winop xdpi 
 *
 * ------------------------------------------------------------------------ 
 */
static int
DpiOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tk_Window tkwin = clientData;
    int xdpi, ydpi;
    const char *string;
    char c;
    
    Blt_ScreenDPI(tkwin, &xdpi, &ydpi);
    string = Tcl_GetString(objv[1]);
    c = string[0];
    if (c == 'x') {
        Tcl_SetIntObj(Tcl_GetObjResult(interp), xdpi);
    } else {
        Tcl_SetIntObj(Tcl_GetObjResult(interp), ydpi);
    }
    return TCL_OK;
}

static Blt_OpSpec winOps[] =
{
    {"changes",  1, ChangesOp,  3, 3, "windowName",},
    {"geometry", 1, GeometryOp, 3, 3, "windowName",},
    {"inside",   1, InsideOp,   5, 5, "windowName x y",},
    {"lower",    1, LowerOp,    2, 0, "?windowName ...?",},
    {"map",      2, MapOp,      2, 0, "?windowName ...?",},
    {"move",     2, MoveOp,     5, 5, "windowName x y",},
    {"query",    1, QueryOp,    2, 2, "",},
    {"raise",    1, RaiseOp,    2, 0, "?windowName ...?",},
#if defined(HAVE_RANDR) && defined(HAVE_XRRGETSCREENRESOURCES)
    {"screensize", 1, ScreenSizeOp, 4, 4, "w h",},
#endif  /* HAVE_RANDR && HAVE_XRRGETSCREENRESOURCES */
    {"selection", 1, SelectionOp, 2, 0, "args...",},
    {"top",      2, TopOp,      4, 4, "x y",},
    {"tree",     2, TreeOp,     4, 4, "windowName treeName",},
    {"unmap",    1, UnmapOp,    2, 0, "?windowName ...?",},
    {"warpto",   1, WarpToOp,   2, 5, "?windowName?",},
    {"xdpi",     1, DpiOp,      2, 2, ""},
    {"ydpi",     1, DpiOp,      2, 2, ""},
};

static int numWinOps = sizeof(winOps) / sizeof(Blt_OpSpec);

/* ARGSUSED */
static int
WinopCmd(ClientData clientData, Tcl_Interp *interp, int objc, 
         Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;
    Tk_Window tkwin;

    proc = Blt_GetOpFromObj(interp, numWinOps, winOps, BLT_OP_ARG1, objc, objv, 
        0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    tkwin = Tk_MainWindow(interp);
    result = (*proc) (tkwin, interp, objc, objv);
    return result;
}

int
Blt_WinopCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = {"winop", WinopCmd,};

    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

#endif /* NO_WINOP */


/*
 * bltWinop.c --
 *
 * This module implements simple window commands for the BLT toolkit.
 *
 *	Copyright 1991-2004 George A Howlett.
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

#ifndef NO_WINOP

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include <X11/Xutil.h>

#include "bltAlloc.h"
#include "bltPicture.h"
#include "bltChain.h"
#include "bltImage.h"
#include "tkDisplay.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define CLAMP(c)	((((c) < 0.0) ? 0.0 : ((c) > 255.0) ? 255.0 : (c)))
static Tcl_ObjCmdProc WinopCmd;

typedef struct _WindowNode WindowNode;
/*
 *  WindowNode --
 *
 *	This structure represents a window hierarchy examined during a single
 *	"drag" operation.  It's used to cache information to reduce the round
 *	trip calls to the server needed to query window geometry information
 *	and grab the target property.  
 */

struct _WindowNode {
    Display *display;
    Window window;		/* Window in hierarchy. */
    int initialized;		/* If zero, the rest of this structure's
				 * information hasn't been set. */
    
    int x1, y1, x2, y2;		/* Extents of the window (upper-left and
				 * lower-right corners). */
    
    WindowNode *parentPtr;	/* Parent node. NULL if root. Used to
				 * compute offset for X11 windows. */
    Blt_Chain chain;		/* List of this window's children. If NULL,
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
	    
	    tkWinWindow.handle = (HWND)xid;
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

/*ARGSUSED*/
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

/*ARGSUSED*/
static int
RaiseOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
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

/*ARGSUSED*/
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

/*ARGSUSED*/
static int
MoveOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
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

/*ARGSUSED*/
static int
UnmapOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
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

/* ARGSUSED */
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

/* ARGSUSED */
static int
GeometryOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    Window id;
    int x, y, w, h;
    Tcl_Obj *listObjPtr;

    if (GetIdFromObj(interp, objv[2], &id) != TCL_OK) {
	return TCL_ERROR;
    }
    Blt_GetWindowRegion(Tk_Display(tkMain), id, &x, &y, &w, &h);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(x));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(y));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(w));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(h));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/* ARGSUSED */
static int
QueryOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
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
 *  GetWindowRegion --
 *
 *	Queries for the upper-left and lower-right corners of the 
 *	given window.  
 *
 *  Results:
 *	Returns if the window is currently viewable.  The coordinates
 *	of the window are returned via parameters.
 *
 * ------------------------------------------------------------------------ 
 */
static int
GetWindowNodeRegion(Display *display, WindowNode *nodePtr)
{
    int x, y, w, h;

    if (Blt_GetWindowRegion(display, nodePtr->window, &x, &y, &w, &h) 
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
FreeWindowNode(WindowNode *parentPtr)	/* Window rep to be freed */
{
    Blt_ChainLink link;

    for (link = Blt_Chain_FirstLink(parentPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	WindowNode *childPtr;

	childPtr = Blt_Chain_GetValue(link);
	FreeWindowNode(childPtr);	/* Recursively free children. */
    }
    Blt_Chain_Destroy(parentPtr->chain);
    Blt_Free(parentPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetWindowZOrder --
 *
 *	Returns a chain of the child windows according to their stacking
 *	order. The window ids are ordered from top to bottom.
 *
 * ------------------------------------------------------------------------ 
 */
static Blt_Chain
GetWindowZOrder(Display *display, Window window)
{
    Blt_Chain chain;
    Window *winv;
    unsigned int winc;
    Window dummy;

    chain = NULL;
    if ((XQueryTree(display, window, &dummy, &dummy, &winv, &winc)) && 
	(winc > 0)) {
	unsigned int i;

	chain = Blt_Chain_Create();
	for (i = 0; i < winc; i++) {
	    /* 
	     * XQuery returns windows in bottom to top order.  We only care
	     * about the top window.  
	     */
	    Blt_Chain_Prepend(chain, (ClientData)winv[i]);
	}
	if (winv != NULL) {
	    XFree((char *)winv);	/* done with list of kids */
	}
    }
    return chain;
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
    chain = GetWindowZOrder(nodePtr->display, nodePtr->window);
    
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
 *	Searches for the topmost window at a given pair of X-Y coordinates.
 *
 *  Results:
 *	Returns a pointer to the node representing the window containing
 *	the point.  If one can't be found, NULL is returned.
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
	return NULL;		/* Point is not over window  */
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

/* ARGSUSED */
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


/* ARGSUSED */
static int
TreeOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tk_Window tkMain = clientData;
    Window *ancestors, window, root, parent;
    unsigned int numAncestors;

    if (GetIdFromObj(interp, objv[2], &window) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((XQueryTree(Tk_Display(tkMain), window, &root, &parent, &ancestors, 
		&numAncestors)) && (numAncestors > 0)) {
	unsigned int i;
	Tcl_Obj *listObjPtr;
	char string[200];
	Tcl_Obj *objPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	sprintf(string, "0x%x", (unsigned int)root);
	objPtr = Tcl_NewStringObj(string , -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	sprintf(string, "0x%x", (unsigned int)parent);
	objPtr = Tcl_NewStringObj(string , -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	sprintf(string, "0x%x", (unsigned int)window);
	objPtr = Tcl_NewStringObj(string , -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	for (i = 0; i < numAncestors; i++) {
	    sprintf(string, "0x%x", (unsigned int)ancestors[i]);
	    objPtr = Tcl_NewStringObj(string , -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_SetObjResult(interp, listObjPtr);
	if (ancestors != NULL) {
	    XFree((char *)ancestors); 
	}
    }
    return TCL_OK;
}


/*ARGSUSED*/
static int
WarpToOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
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

#if defined(HAVE_RANDR) && defined(HAVE_DECL_XRRGETSCREENRESOURCES)
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

static int
SetScreenSizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
		Tcl_Obj *const *objv)
{
    Display *display;
    int mode;
    Tk_Window tkMain = clientData;
    Window root;
    char modeName[200];
    XRRScreenResources *resPtr;
    XRRScreenConfiguration *configPtr;
    XRRScreenSize *sizesPtr;
    XRRModeInfo info;
    int w, h, i, numSizes;
    int sizeNum;
    Rotation current_rotation;
    int majorNum, minorNum, eventBase, errorBase;

    display = Tk_Display(tkMain);
    root = Tk_RootWindow(tkMain);
    if ((Tk_GetPixelsFromObj(interp, tkMain, objv[2], &w) != TCL_OK) || 
	(Tk_GetPixelsFromObj(interp, tkMain, objv[3], &h) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (!XRRQueryExtension(display, &eventBase, &errorBase) ||
        !XRRQueryVersion (display, &majorNum, &minorNum)) {
	Tcl_AppendResult(interp, "RandR extension missing", (char *)NULL);
	return TCL_ERROR;
    }
    fprintf(stderr, "Xrandr version %d.%d\n", majorNum, minorNum);
    resPtr = XRRGetScreenResources(display, root);
    if (resPtr == NULL) {
	Tcl_AppendResult(interp, "can't get screen resources", (char *)NULL);
	return TCL_ERROR;
    }
    sprintf(modeName, "%dx%d", w, h);
    PrintModes(resPtr);
#ifdef notdef
    strcpy(modeName, "hubzero");
#endif
    mode = FindMode(resPtr, modeName);
    PrintModes(resPtr);
    if (mode >= 0) {
	fprintf(stderr, "found mode %s (%d)\n", modeName, mode);
#ifdef notdef
	XRRDeleteOutputMode(display, resPtr->outputs[0], mode);
	XRRDestroyMode(display, mode);
	XRRFreeScreenResources(resPtr);
	resPtr = XRRGetScreenResources(display, root);
	if (resPtr == NULL) { 
	    Tcl_AppendResult(interp, "can't get screen resources", 
			     (char *)NULL);
	    return TCL_ERROR;
	}
#endif
    } else { 
    memset(&info, 0, sizeof(info));
    info.width = w;
    info.height = h;
    info.name = modeName;
    info.nameLength = strlen(modeName);
    mode = XRRCreateMode(display, root, &info);
    if (mode < 0) {
	return TCL_ERROR;
    }
    }
    XRRAddOutputMode(display, resPtr->outputs[0], mode);
    XRRFreeScreenResources(resPtr);

    configPtr = XRRGetScreenInfo(display, root);
    if (configPtr == NULL) {
	return TCL_ERROR;
    }
    sizesPtr = XRRConfigSizes(configPtr, &numSizes);
    sizeNum = 0;
    for (i = 0; i < numSizes; i++) {
	fprintf(stderr, "%dx%d\n",  sizesPtr[i].width, sizesPtr[i].height);
	if ((sizesPtr[i].width == w) && (sizesPtr[i].height == h)) {
	    sizeNum = i;
	}
    }
    XRRConfigRotations(configPtr, &current_rotation);
    XRRSetScreenConfig(display, configPtr, root, (SizeID)sizeNum, 
	current_rotation, CurrentTime);
    XRRFreeScreenConfigInfo(configPtr);
    return TCL_OK;
}
#endif	/* HAVE_RANDR && HAVE_DECL_XRRGETSCREENRESOURCES */

static Blt_OpSpec winOps[] =
{
    {"changes",  1, ChangesOp,  3, 3, "window",},
    {"geometry", 1, GeometryOp, 3, 3, "window",},
    {"lower",    1, LowerOp,    2, 0, "window ?window?...",},
    {"map",      2, MapOp,      2, 0, "window ?window?...",},
    {"move",     2, MoveOp,     5, 5, "window x y",},
    {"query",    1, QueryOp,    2, 2, "",},
    {"raise",    1, RaiseOp,    2, 0, "window ?window?...",},
#if defined(HAVE_RANDR) && defined(HAVE_DECL_XRRGETSCREENRESOURCES)
    {"screensize", 1, SetScreenSizeOp, 4, 4, "w h",},
#endif	/* HAVE_RANDR && HAVE_DECL_XRRGETSCREENRESOURCES */
    {"top",      2, TopOp,      4, 4, "x y",},
    {"tree",     2, TreeOp,     3, 3, "window",},
    {"unmap",    1, UnmapOp,    2, 0, "window ?window?...",},
    {"warpto",   1, WarpToOp,   2, 5, "?window?",},
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

    proc = Blt_GetOpFromObj(interp, numWinOps, winOps, BLT_OP_ARG1,  objc, objv, 
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

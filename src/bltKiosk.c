
/*
 * bltKiosk.c --
 *
 *	Copyright 2012 George A Howlett.
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

/* 
 * # Create a kiosk with "." window.  Handle client embedding.
 * # Window will be kept as big as the root window.
 * blt::kiosk create . -maximize true -command EmbedClients
 * 
 * proc EmbedClients { container title } {
 *	# Container is automatically managing the foreign window.
 *     .tabset add $title -window $container
 *     .tabset select $title
 * }
 * 
 * button .b -command {
 *     blt::container create .tabset.container1 
 *     blt::kiosk start .tabset.container1 \
 *	  "Command string" -title "title"
 * }    
 * # Kill an application by destroying its container.
 * destroy .tabset.container1
 */


#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#if !defined(WIN32) && !defined(MACOSX)
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#endif

#include "bltAlloc.h"
#include "bltHash.h"
#include "tkDisplay.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define KIOSK_MAXIMIZE	(1<<1)
#define KIOSK_MAPPED	(1<<2)
#define KIOSK_FOCUS	(1<<4)
#define KIOSK_INIT	(1<<5)

#define CLIENT_TRANSIENT	(1<<0)
#define CLIENT_TAKE_FOCUS	(1<<1)
#define CLIENT_SAVE_YOURSELF	(1<<2)
#define CLIENT_DELETE_WINDOW	(1<<3)

#define CLIENT_STATE_NORMAL	(1<<4)
#define CLIENT_STATE_WITHDRAWN	(1<<5)
#define CLIENT_STATE_ICONIC	(1<<6)

#define CLIENT_INPUT_HINT	(1<<8)

#define CLIENT_STATE_MASK	\
    (CLIENT_STATE_NORMAL|CLIENT_STATE_WITHDRAWN|CLIENT_STATE_ICONIC)

#define DEBUG 1

typedef struct {
    unsigned int flags;
    Tcl_Interp *interp;
    Display *display;			/* Display of kiosk. */
    Tk_Window tkwin;			/* Toplevel window designation as the
					 * Root window of kiosk. */
    Blt_HashEntry *hashPtr;
    Blt_HashTable clientTable;
    Blt_HashTable frameTable;
    Window root;			/* Root window of display. */
    Tcl_Obj *cmdObjPtr;			/* Command to executed with a 
					 * new toplevel is mapped. */
} Kiosk;

typedef struct {
    unsigned int flags;
    Kiosk *kioskPtr;
    Blt_HashEntry *hashPtr;
    Window id;
    int width, height;
    const char *resName;
    const char *className;
    Window transientFor;
    Blt_HashTable transientTable;
    Window group;
    int widthInc, heightInc;
} Client;

static Tcl_ObjCmdProc KioskCmd;
static Tk_EventProc KioskEventProc;
static Tk_GenericProc GenericEventProc;
static Tcl_IdleProc MapClientIdleProc;

static Blt_HashTable kioskTable;

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_OBJ, "-command", "command", "Command", (char *)NULL,
	Blt_Offset(Kiosk, cmdObjPtr), 
        BLT_CONFIG_DONT_SET_DEFAULT | BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_END}
};

static Atom takeFocusAtom;
static Atom saveYourselfAtom;
static Atom stateAtom;
static Atom deleteWindowAtom;

static const char *eventNames[] = {
    "invalid", "???", 
    "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease",
    "MotionNotify", "EnterNotify", "LeaveNotify", "FocusIn",
    "FocusOut", "KeymapNotify", "Expose", "GraphicsExpose", 
    "NoExpose", "VisibilityNotify", "CreateNotify", "DestroyNotify",
    "UnmapNotify", "MapNotify", "MapRequest", "ReparentNotify", 
    "ConfigureNotify", "ConfigureRequest", "GravityNotify", "ResizeRequest",
    "CirculateNotify", "CirculateRequest", "PropertyNotify", "SelectionClear",
    "SelectionRequest", "SelectionNotify", "ColormapNotify", "ClientMessage", 
    "MappingNotify", "GenericEvent"
};

/* 
 * ResizeKiosk --
 *
 *	Checks if the main window of the kiosk needs to be resized.  This
 *	is called whenever we get ConfigureNotify events on the root 
 *	window.
 */
static void
ResizeKiosk(Kiosk *kioskPtr)
{
    int w, h;
	
    if (kioskPtr->tkwin == NULL) {
	return;				/* Window has been destroyed. */
    }
    Blt_SizeOfScreen(kioskPtr->tkwin, &w, &h);
    Tk_GeometryRequest(kioskPtr->tkwin, w, h);
    if ((Tk_X(kioskPtr->tkwin) != 0) || (Tk_Y(kioskPtr->tkwin) != 0) ||
	(Tk_Width(kioskPtr->tkwin) != w) || 
	(Tk_Height(kioskPtr->tkwin) != h)) {
	/* This will cause the kiosk to be redrawn. */
 	Blt_MoveResizeToplevelWindow(kioskPtr->tkwin, 0, 0, w, h);
    }
    Blt_LowerToplevelWindow(kioskPtr->tkwin); 
}

/* 
 * GetWmProtocols --
 *
 */
static void
GetWmProtocols(Client *clientPtr)
{
    Atom *protocolsPtr;
    int numProtocols;
    Kiosk *kioskPtr = clientPtr->kioskPtr;

    if (takeFocusAtom == None) {
	takeFocusAtom = XInternAtom(kioskPtr->display, "WM_TAKE_FOCUS", False);
    }
    if (saveYourselfAtom == None) {
	saveYourselfAtom = XInternAtom(kioskPtr->display, "WM_SAVE_YOURSELF", 
		False);
    }
    if (deleteWindowAtom == None) {
	deleteWindowAtom = XInternAtom(kioskPtr->display, "WM_DELETE_WINDOW", 
		False);
    }
    protocolsPtr = NULL;
    if (XGetWMProtocols (kioskPtr->display, clientPtr->id, &protocolsPtr, 
	&numProtocols)) {
        Atom *p, *pend;

        for (p = protocolsPtr, pend = p + numProtocols; p < pend; p++) {
            if (*p == takeFocusAtom) {
		clientPtr->flags |= CLIENT_TAKE_FOCUS;
	    } else if (*p == saveYourselfAtom) {
		clientPtr->flags |= CLIENT_SAVE_YOURSELF;
	    } else if (*p == deleteWindowAtom) {
		clientPtr->flags |= CLIENT_DELETE_WINDOW;
	    }
        }
	XFree ((char *)protocolsPtr);
    }
}

/* 
 * GetWmProtocols --
 *
 */
static void
GetWmHints(Client *clientPtr)
{
    Kiosk *kioskPtr = clientPtr->kioskPtr;
    XWMHints *hintsPtr;

    hintsPtr = XGetWMHints(kioskPtr->display, clientPtr->id);
    if (hintsPtr != NULL) {
	if (hintsPtr->flags & InputHint) {
	    clientPtr->flags |= CLIENT_INPUT_HINT;
	}
	if (hintsPtr->flags & WindowGroupHint) {
	    clientPtr->group = hintsPtr->window_group;
	}
	XFree ((char *)hintsPtr);
    }
}

static void
GetWindowSizeHints(Client *clientPtr)
{
    Kiosk *kioskPtr = clientPtr->kioskPtr;
    XSizeHints hints;
    long suppl = 0;

    if (!XGetWMNormalHints(kioskPtr->display, clientPtr->id, &hints, &suppl)) {
	hints.flags = 0L;
    }
#if DEBUG
    fprintf(stderr, "XGetWMNormalHints id=0x%x\n", (unsigned int)clientPtr->id);
    if (hints.flags & PResizeInc) {
	fprintf(stderr, "\twidth_inc=%d\n", hints.width_inc);
	fprintf(stderr, "\theight_inc=%d\n", hints.height_inc);
    }
    if (hints.flags & USPosition) {
	fprintf(stderr, "user-specified position\n");
    }
    if (hints.flags & PMaxSize) {
	fprintf(stderr, "\tmax_width=%d\n", hints.max_width);
	fprintf(stderr, "\tmax_height=%d\n", hints.max_height);
    }
    if (hints.flags & PMinSize) {
	fprintf(stderr, "\tmin_width=%d\n", hints.min_width);
	fprintf(stderr, "\tmin_height=%d\n", hints.min_height);
    }
    if (hints.flags & PSize) {
	fprintf(stderr, "\twidth=%d\n", hints.width);
	fprintf(stderr, "\theight=%d\n", hints.height);
    }
    if (hints.flags & PAspect) {
	fprintf(stderr, "\tmin_aspect=%d/%d\n", 
		hints.min_aspect.x, hints.min_aspect.y);
	fprintf(stderr, "\tmax_aspect=%d/%d\n", 
		hints.max_aspect.x, hints.max_aspect.y);
    }
    if (hints.flags & PBaseSize) {
	fprintf(stderr, "\tbase_width=%d\n", hints.base_width);
	fprintf(stderr, "\tbase_height=%d\n", hints.base_height);
    }
#endif /*DEBUG*/
    if (hints.flags & PResizeInc) {
	clientPtr->widthInc = (hints.width_inc > 0) ? clientPtr->widthInc: 1;
	clientPtr->heightInc = (hints.height_inc > 0) ? clientPtr->heightInc: 1;
    }
    if (hints.flags & USPosition) {
    }
    if (hints.flags & PMaxSize) {
    }
    if (hints.flags & PMinSize) {
    }
    if (hints.flags & PSize) {
    }
    if (hints.flags & PAspect) {
    }
    if (hints.flags & PBaseSize) {
    }
}

/* 
 * GetClassNames --
 *
 */
static void
GetClassNames(Client *clientPtr)
{
    XClassHint class;
    Kiosk *kioskPtr = clientPtr->kioskPtr;
    
    if (XGetClassHint(kioskPtr->display, clientPtr->id, &class)) {
	clientPtr->resName = Blt_Strdup(class.res_name);
	clientPtr->className = Blt_Strdup(class.res_class);
	XFree(class.res_name);
	XFree(class.res_class);
    }
}


/* 
 * GetWmState --
 *
 */
static void
GetWmState(Client *clientPtr)
{
    Kiosk *kioskPtr = clientPtr->kioskPtr;
    Atom actualType;
    int actualFormat;
    unsigned long numItems, numBytesAfter;
    unsigned char *property;
    int result;

    if (stateAtom == None) {
	stateAtom = XInternAtom(kioskPtr->display, "WM_STATE", False);
    }
    result = XGetWindowProperty(kioskPtr->display, clientPtr->id, stateAtom,
	0L, 2L, False, stateAtom, &actualType, &actualFormat, &numItems, 
	&numBytesAfter, &property);
    if ((result != Success) || (property == NULL)) {
	return;
    }
    if (numItems <= 2) {		/* "suggested" by ICCCM version 1 */
        unsigned int *array = (unsigned int *)property;
	int state;

        state = (int)array[0];
	clientPtr->flags &= ~CLIENT_STATE_MASK;	 /* Clear the state flags. */
	if (state == NormalState) {
	    clientPtr->flags |= CLIENT_STATE_NORMAL;
	} else if (state == IconicState) {
	    clientPtr->flags |= CLIENT_STATE_ICONIC;
	} else if (state == WithdrawnState) {
	    clientPtr->flags |= CLIENT_STATE_NORMAL;
	} else {
#ifndef notdef
	fprintf(stderr, "window 0x%x: state=%d iw=%x\n",
	    (unsigned int)clientPtr->id, array[0], array[1]);
#endif
	}
    }
}

/* 
 * SetWmState --
 *
 */
static void
SetWmState(Client *clientPtr, int state)
{
    Kiosk *kioskPtr = clientPtr->kioskPtr;
    unsigned long array[2];              /* "suggested" by ICCCM version 1 */

    array[0] = (unsigned long)state;
    array[1] = (unsigned long)None;
    if (stateAtom == None) {
	stateAtom = XInternAtom(kioskPtr->display, "WM_STATE", False);
    }
    XChangeProperty (kioskPtr->display, clientPtr->id, stateAtom, stateAtom, 32,
	PropModeReplace, (unsigned char *)array, 2);
}

static void
GetTransientFor(Client *clientPtr)
{
    Kiosk *kioskPtr = clientPtr->kioskPtr;
    Window w;

    if (XGetTransientForHint(kioskPtr->display, clientPtr->id, &w)) {
	clientPtr->transientFor = None;
    } else {
	clientPtr->transientFor = w;
    }
#if DEBUG
    fprintf(stderr, "XGetTransientFor id=0x%x\n", (unsigned int)clientPtr->id);
    fprintf(stderr, "\ttransient_for=0x%x\n", (unsigned int)clientPtr->transientFor);
#endif
}

/* 
 * NewClient --
 *
 *	Allocates and initializes a new client structure.  This keeps track of
 *	client windows (not kiosk windows) which are plain X windows.
 *	Information is kept here, as well as the container widget, because the
 *	initial width and height of the window are known before the window is
 *	requested to be mapped (when it is sent to the container).
 *	
 *	This structure is freed when this window or the kiosk is destroyed.
 */
static Client *
NewClient(Kiosk *kioskPtr, Window id)
{
    Client *clientPtr;

    clientPtr = Blt_AssertCalloc(1, sizeof(Client));
    clientPtr->kioskPtr = kioskPtr;
    clientPtr->id = id;
    clientPtr->width = clientPtr->height = 0;
    GetClassNames(clientPtr);
    GetWmProtocols(clientPtr);
    GetWmState(clientPtr);
    GetWmHints(clientPtr);
    GetWindowSizeHints(clientPtr);
    GetTransientFor(clientPtr);
    SetWmState(clientPtr, NormalState);
#ifdef notdef
    if (clientPtr->transientFor != None) {
	Blt_HashEntry *hPtr;

	hPtr = Blt_FindHashEntry(&kioskPtr->clientTable, (char *)w);
	if (hPtr != NULL) {
	    Client *masterPtr;
	    int isNew;

	    masterPtr = Blt_GetHashValue(hPtr);
	    hPtr = Blt_CreateHashEntry(&masterPtr->transientTable, 
				       clientPtr->id, &isNew);
	    Blt_SetHashValue(hPtr, clientPtr);
	}
    }
#endif
    Blt_InitHashTable(&clientPtr->transientTable, BLT_ONE_WORD_KEYS);
    return clientPtr;
}

/* 
 * MapClientIdleProc --
 *
 *	Handle the MapRequest event when TCL is idle.  A callback
 *	is issued (if one if required) to handle the new client window.  
 *	The callback may choose to put the window in a toplevel or
 *	embedded into kiosk widget itself. 
 */
static void
MapClientIdleProc(ClientData clientData)
{
    Client *clientPtr = clientData;
    Window id;
    Kiosk *kioskPtr;
    int result;

    kioskPtr = clientPtr->kioskPtr;
    id = clientPtr->id;
    if (clientPtr->transientFor != None) {
	return;
    }
    if ((clientPtr->width == 0) || (clientPtr->height == 0)) {
	XWindowAttributes attrs;

	XGetWindowAttributes(kioskPtr->display, id, &attrs);
	clientPtr->width = attrs.width;
	clientPtr->height = attrs.height;
#if DEBUG 
    fprintf(stderr, "window 0x%x: x=%d y=%d w=%d h=%d depth=%d colormap=%x map_install=%d map_state=%d override_redirect=%d\n",
	    (unsigned int)id, attrs.x, attrs.y, attrs.width, attrs.height, 
	    attrs.depth,
	    (unsigned int)attrs.colormap, attrs.map_installed, attrs.map_state, attrs.override_redirect);
#endif
    }
    if (kioskPtr->cmdObjPtr != NULL) {
	Tcl_Obj *cmdObjPtr;
	char string[200];
	char *wmName;

	cmdObjPtr = Tcl_DuplicateObj(kioskPtr->cmdObjPtr);
	sprintf(string, "0x%x", (unsigned int)id);
	Tcl_ListObjAppendElement(kioskPtr->interp, cmdObjPtr, 
		Tcl_NewStringObj(string, -1));
	Tcl_ListObjAppendElement(kioskPtr->interp, cmdObjPtr, 
		Tcl_NewStringObj("width", 5));
	Tcl_ListObjAppendElement(kioskPtr->interp, cmdObjPtr, 
		Tcl_NewIntObj(clientPtr->width));
	Tcl_ListObjAppendElement(kioskPtr->interp, cmdObjPtr, 
		Tcl_NewStringObj("height", 6));
	Tcl_ListObjAppendElement(kioskPtr->interp, cmdObjPtr, 
		Tcl_NewIntObj(clientPtr->height));
	if (XFetchName(kioskPtr->display, id, &wmName)) {
	    Tcl_ListObjAppendElement(kioskPtr->interp, cmdObjPtr, 
		Tcl_NewStringObj("title", 5));
	    Tcl_ListObjAppendElement(kioskPtr->interp, cmdObjPtr, 
		Tcl_NewStringObj(wmName, -1));
	    XFree(wmName);
	}
	Tcl_IncrRefCount(cmdObjPtr);
	Tcl_Preserve(kioskPtr);
	result = Tcl_EvalObjEx(kioskPtr->interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_Release(kioskPtr);
	Tcl_DecrRefCount(cmdObjPtr);
	if (result != TCL_OK) {
	    Tcl_BackgroundError(kioskPtr->interp);
	}
    }
}
    
static int
MapWindowWhenIdle(Kiosk *kioskPtr, Window id)
{
    Client *clientPtr;
    Blt_HashEntry *hPtr;
    int isNew;
    unsigned int mask;

#if DEBUG
    fprintf(stderr, "MapWindowWhenIdle: id=%x\n", (unsigned int)id);
#endif
    hPtr = Blt_CreateHashEntry(&kioskPtr->clientTable, (char *)id, &isNew);
    if (isNew) {
	clientPtr = NewClient(kioskPtr, id);
	clientPtr->hashPtr = hPtr;
    } else {
	clientPtr = Blt_GetHashValue(hPtr);
    }
    SetWmState(clientPtr, NormalState);
#ifndef notdef
    mask = SubstructureRedirectMask | SubstructureNotifyMask |
	ResizeRedirectMask | StructureNotifyMask;
    XSelectInput(kioskPtr->display, id, mask);
#endif
    Tcl_DoWhenIdle(MapClientIdleProc, (ClientData)clientPtr);
    return TCL_OK;
}

static void
AddEmbeddedWindow(Kiosk *kioskPtr, Window w)
{
}

static int
MapRequestEvent(Kiosk *kioskPtr, XEvent *eventPtr)
{
    Window id;
    Tk_Window tkwin;
    XMapRequestEvent *evPtr;

    evPtr = &eventPtr->xmaprequest;

    if (evPtr->parent != kioskPtr->root) {
	return 0;
    }
    /* 
     * This window is either 
     *   1) a client application window we want to embed, 
     *   2) a generic toplevel window that requires decorations or 
     *   3) a Tk toplevel widget from the kiosk. Right now assume
     *      all widget windows are part of the window manager.
     */
    id = evPtr->window;
#if DEBUG
    fprintf(stderr, "MapRequest: id=%x\n", (unsigned int)id);
#endif
    tkwin = Tk_IdToWindow(kioskPtr->display, id);
    if (tkwin == NULL) {
	/* This is an unknown toplevel window which needs decorations or
	 * an application window to be embedded.  We can determine this
	 * by your clientapp flag. */
	MapWindowWhenIdle(kioskPtr, id);
	return 1;
    } else {
	Blt_HashEntry *hPtr;

	/* This is Tk toplevel widget window which is part of the
	 * window manager, such as framing decorations. */
#if DEBUG
	fprintf(stderr, "MapRequest: id=0x%x tkwin=%s\n",
		 (unsigned int)id, Tk_PathName(tkwin));
#endif
	if (tkwin == kioskPtr->tkwin) {
	    return 0;			/* It's the kiosk itself. */
	}
	/* Check if this a kiosk frame or a new top level. */
	hPtr = Blt_FindHashEntry(&kioskPtr->clientTable, (char *)id);
	if (hPtr == NULL) {
	    /* It's a new toplevel window that needs to be put in a frame. */
	    MapWindowWhenIdle(kioskPtr, id);
	}
    }
    return 0;
}

static int
CreateWindowEvent(Kiosk *kioskPtr, XEvent *eventPtr)
{
    Window id;
    Tk_Window tkwin;
    XCreateWindowEvent *evPtr;

    evPtr = &eventPtr->xcreatewindow;
    if (evPtr->parent != kioskPtr->root) {
	return 0;
    }
    /* 
     * This window is either 
     *   1) a client application window we want to embed, 
     *   2) a generic toplevel window that requires decorations or 
     *   3) a Tk toplevel widget from the kiosk. Right now assume
     *      all widget windows are part of the window manager.
     */
    id = evPtr->window;
#if DEBUG
    fprintf(stderr, "CreateWindowEvent: id=%x\n", (unsigned int)id);
#endif
    tkwin = Tk_IdToWindow(kioskPtr->display, id);
    if (tkwin == NULL) {
	/* This is an unknown toplevel window which needs decorations or
	 * an application window to be embedded.  We can determine this
	 * by your clientapp flag. */
	MapWindowWhenIdle(kioskPtr, id);
	return 1;
    } else {
	Blt_HashEntry *hPtr;

	/* This is Tk toplevel widget window which is part of the
	 * window manager, such as framing decorations. */
#if DEBUG
	fprintf(stderr, "CreateWindow is tkwin id=0x%x tkwin=%s\n",
		 (unsigned int)id, Tk_PathName(tkwin));
#endif
	if (tkwin == kioskPtr->tkwin) {
	    return 0;			/* It's the kiosk itself. */
	}
	/* Check if this a kiosk frame or a new top level. */
	hPtr = Blt_FindHashEntry(&kioskPtr->clientTable, (char *)id);
	if (hPtr == NULL) {
	    /* It's a new toplevel window that needs to be put in a frame. */
	    MapWindowWhenIdle(kioskPtr, id);
	}
    }
    return 0;
}

static int
ConfigureRequestEvent(Kiosk *kioskPtr, XEvent *eventPtr)
{
    XConfigureRequestEvent *evPtr;
    Window id;
    Tk_Window tkwin;
    Blt_HashEntry *hPtr;
    int isNew;

    evPtr = (XConfigureRequestEvent *)&eventPtr->xconfigurerequest;
    id = evPtr->window;
#ifndef notdef
    fprintf(stderr, "ConfigureRequest %x\n", (unsigned int)evPtr->window);
    if (evPtr->value_mask & CWX) {
        fprintf(stderr, "  x = %d\n", evPtr->x);
    }
    if (evPtr->value_mask & CWY) {
        fprintf(stderr, "  y = %d\n", evPtr->y);
    }
    if (evPtr->value_mask & CWWidth) {
        fprintf(stderr, "  width = %d\n", evPtr->width);
    }
    if (evPtr->value_mask & CWHeight) {
        fprintf(stderr, "  height = %d\n", evPtr->height);
    }
    if (evPtr->value_mask & CWSibling) {
        fprintf(stderr, "  above = 0x%x\n", (unsigned int)evPtr->above);
    }
    if (evPtr->value_mask & CWStackMode) {
        fprintf(stderr, "  stack = %d\n", evPtr->detail);
    }
#endif
    tkwin = Tk_IdToWindow(kioskPtr->display, id);
    if (tkwin != NULL) {
	return 0;
    }
    hPtr = Blt_CreateHashEntry(&kioskPtr->clientTable, (char *)id, &isNew);
    if (isNew) {
	Client *clientPtr;
	XWindowChanges changes;
	unsigned int mask;

	clientPtr = NewClient(kioskPtr, id);
	clientPtr->hashPtr = hPtr;
	clientPtr->width = evPtr->width;
	clientPtr->height = evPtr->height;
	Blt_SetHashValue(hPtr, clientPtr);
	
	changes.width = evPtr->width;
	changes.height = evPtr->height;
	mask = (CWWidth | CWHeight);
        XConfigureWindow (kioskPtr->display, clientPtr->id, mask, &changes);
    }
#ifdef notdef
    contPtr = NULL;
    if (FindContainer(kioskPtr, evPtr->window, &contPtr) != TCL_OK) {
    }
    if (XFindContext(kioskPtr->display, evPtr->window, kioskPtr->context, 
	&contPtr) == 0) {
    }

    if ((evPtr->value_mask & CWStackMode) && contPtr->flags & STACKMODE) {
	XWindowChanges changes;
	unsigned int mask;

	changes.sibling = evPtr->above;
	changes.stack_mode = evPtr->detail;
	mask = (CWSibling | CWStackMode);
        if (evPtr->value_mask & CWSibling) {
	    Container *contPtr;

            if (XFindContext(kioskPtr->display, evPtr->above, 
			     kioskPtr->context, &contPtr) == 0) {
		changes.sibling = Tk_WindowId(contPtr->tkwin);
	    }
	}
        XConfigureWindow (kioskPtr->display, kioskPtr->frame,
		evPtr->value_mask & mask, &changes);
    }
#endif
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * GenericEventProc --
 *
 * 	This procedure is invoked by the Tk dispatcher for various
 * 	events on the root window.
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
GenericEventProc(ClientData clientData, XEvent *eventPtr)
{
    Kiosk *kioskPtr = (Kiosk *)clientData;
    Window id;
    Tk_Window tkwin;

    id = eventPtr->xany.window;
    if (id  == Tk_WindowId(kioskPtr->tkwin)) {
	/* return 0; */
    }
    switch (eventPtr->type) {
    case Expose:
    case MotionNotify:
	return 0;
    }
#if DEBUG
    fprintf(stderr, "GenericEventProc: event = %d %s %x\n", 
	    eventPtr->type,
	    eventNames[eventPtr->type], 
	    (unsigned int)eventPtr->xany.window);
#endif
    if (eventPtr->type == DestroyNotify) {
	Blt_HashEntry *hPtr;

	hPtr = Blt_FindHashEntry(&kioskPtr->clientTable, (char *)id);
	if (hPtr != NULL) {
	    Client *clientPtr;

	    clientPtr = Blt_GetHashValue(hPtr);
	    Blt_Free(clientPtr);
	    Blt_DeleteHashEntry(&kioskPtr->clientTable, hPtr);
	    return 0;
	}
    }
    if (eventPtr->type == ReparentNotify) {
	XReparentEvent *evPtr;
	
	evPtr = &eventPtr->xreparent;
	fprintf(stderr, "\tReparentEvent id=0x%x old=0x%x new=0x%x\n", 
		(unsigned int)evPtr->window,
		(unsigned int)evPtr->event,
		(unsigned int)evPtr->parent);
    }
    tkwin = Tk_IdToWindow(kioskPtr->display, id);
    if (tkwin != NULL && tkwin != kioskPtr->tkwin) {
#if DEBUG
	fprintf(stderr, "\t %x is Tk_Window %s\n", 
		(unsigned int)id, Tk_PathName(tkwin));
#endif
    }
#ifdef DEBUGx
    fprintf(stderr, "\tNot on Root: event = %d %s %x\n", 
	    eventPtr->type,
	    eventNames[eventPtr->type], 
	    (unsigned int)eventPtr->xany.window);
#endif
    if (eventPtr->type == ConfigureRequest) {
	fprintf(stderr, "\tConfigureRequest = %d %s %x w=%d h=%d\n", 
		eventPtr->type, eventNames[eventPtr->type], 
		(unsigned int)eventPtr->xconfigurerequest.window,
		eventPtr->xconfigurerequest.width, 
		eventPtr->xconfigurerequest.height);
    } else if (eventPtr->type == ResizeRequest) {
	XResizeRequestEvent *evPtr;
	
	evPtr = &eventPtr->xresizerequest;
	fprintf(stderr, "\tResizeRequest id=0x%x\n", (unsigned int)evPtr->window);
    } else if (eventPtr->type == ConfigureRequest) {
	return ConfigureRequestEvent(kioskPtr, eventPtr);
    } else if (eventPtr->type == CreateNotify) {
	XCreateWindowEvent *evPtr;
	
	evPtr = &eventPtr->xcreatewindow;
	fprintf(stderr, "\tCreateNotify id=0x%x, parent=0x%x overrideredirect=%d\n", 
		(unsigned int)evPtr->window, (unsigned int)evPtr->parent,
		evPtr->override_redirect);
	/* return CreateWindowEvent(kioskPtr, eventPtr); */
    } else if (eventPtr->type == MapRequest) {
	return MapRequestEvent(kioskPtr, eventPtr);
    } else if (eventPtr->type == ConfigureNotify) {
	XConfigureEvent *evPtr;
	
	evPtr = &eventPtr->xconfigure;
	fprintf(stderr, "\tConfigureNotify id=0x%x event=0x%x\n", 
		(unsigned int)evPtr->window, 
		(unsigned int)evPtr->event);
	if ((evPtr->window == kioskPtr->root) &&
	    (kioskPtr->flags & KIOSK_MAXIMIZE)) {
	    ResizeKiosk(kioskPtr);
	}
    }
    return 0;
}

/* 
 * DestroyKiosk -- 
 *
 *	Deallocates and free resources used by the kiosk.  The windows
 *	are unmanaged (the frames are destroyed and the windows are
 *	reparented back to the root window).
 *
 */
static void
DestroyKiosk(Kiosk *kioskPtr)
{
    if (kioskPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&kioskTable, kioskPtr->hashPtr);
    }
    Blt_DeleteHashTable(&kioskPtr->frameTable);
    Blt_DeleteHashTable(&kioskPtr->clientTable);
    Tk_DeleteGenericHandler(GenericEventProc, kioskPtr);
    if (kioskPtr->tkwin != NULL) {
	unsigned int mask;

	mask = (StructureNotifyMask | ExposureMask | FocusChangeMask);
	Tk_DeleteEventHandler(kioskPtr->tkwin, mask, KioskEventProc, kioskPtr);
    }
    Blt_FreeOptions(configSpecs, (char *)kioskPtr, kioskPtr->display, 0);
    Blt_Free(kioskPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * KioskEventProc --
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
static void
KioskEventProc(ClientData clientData, XEvent *eventPtr)
{
    Kiosk *kioskPtr = clientData;

    fprintf(stderr, "KioskEventProc: event = %d %s\n", eventPtr->type,
	    eventNames[eventPtr->type]);
    switch (eventPtr->type) {
    case Expose:
	if (eventPtr->xexpose.count == 0) {
	    ResizeKiosk(kioskPtr);
	}
	break;

    case FocusIn:
    case FocusOut:
	if (eventPtr->xfocus.detail != NotifyInferior) {
	    if (eventPtr->type == FocusIn) {
		kioskPtr->flags |= KIOSK_FOCUS;
	    } else {
		kioskPtr->flags &= ~KIOSK_FOCUS;
	    }
	}
	break;

    case ConfigureNotify:
	ResizeKiosk(kioskPtr);
	break;

    case DestroyNotify:
	if (kioskPtr->tkwin != NULL) {
	    kioskPtr->tkwin = NULL;
	    DestroyKiosk(kioskPtr);
	}
	break;
    }
}

/* 
 * NewKiosk -- 
 *
 *	Allocates and initializes a new kiosk structure for the named
 *	toplevel window.
 *
 */
static Kiosk *
NewKiosk(Tcl_Interp *interp, Tk_Window tkwin, Blt_HashEntry *hashPtr)
{
    Kiosk *kioskPtr;
    unsigned int mask;
    
    kioskPtr = Blt_AssertCalloc(1, sizeof(Kiosk));
    kioskPtr->tkwin = tkwin;
    kioskPtr->interp = interp;
    kioskPtr->hashPtr = hashPtr;
    kioskPtr->flags = KIOSK_MAXIMIZE;
    kioskPtr->display = Tk_Display(tkwin);
    kioskPtr->root = Tk_RootWindow(tkwin);
    Blt_InitHashTable(&kioskPtr->frameTable, BLT_ONE_WORD_KEYS);
    Blt_InitHashTable(&kioskPtr->clientTable, BLT_ONE_WORD_KEYS);
    mask = (StructureNotifyMask | ExposureMask | FocusChangeMask);
    Tk_CreateEventHandler(tkwin, mask, KioskEventProc, kioskPtr);
    return kioskPtr;
}


static int
IsMappedWithoutOverride(Display *display, Window id)
{
    XWindowAttributes attrs;

    XGetWindowAttributes(display, id, &attrs);
    return ((attrs.map_state != IsUnmapped) && 
	    (attrs.override_redirect != True));
}



/*
 *---------------------------------------------------------------------------
 *
 * XSelectInputErrorProc --
 *
 *	Flags errors generated from XSelectInput calls to the X server.
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
XSelectInputErrorProc(ClientData clientData, XErrorEvent *errEventPtr)
{
    int *errorPtr = clientData;

    *errorPtr = FALSE;
    return 0;
}

static void
FakeMapRequest(Window id)
{
}

/* 
 * ManageExistingWindows -- 
 *
 *	Manage all the existing children of the root window.  Reparent the
 *	window by placing it a container and call the designed procedure to 
 *	decorated the frame.  Ignore the kiosk window which acts as the root
 *	window.
 */
static void
ManageExistingWindows(Kiosk *kioskPtr)
{
    Window root, parent, *children;
    unsigned int i, numChildren;

    /* Handle all existing windows. This probably includes the kiosk
     * window itself. */
    XQueryTree(kioskPtr->display, kioskPtr->root, &root, &parent, &children, 
	       &numChildren);
    for (i = 0; i < numChildren; i++) {
	Window id;

	id = children[i];
	if (id == None) {
	    continue;
	}
	if (id == Tk_WindowId(kioskPtr->tkwin)) {
	    continue;			/* Ignore the kiosk itself. */
	}
#ifdef notdef
	if (IsMappedWithoutOverride(kioskPtr->display, id)) {
	    XUnmapWindow(kioskPtr->display, id);
	    /* Remap the window to trigger our handler. */
	    FakeMapRequest(id);
	}
#endif
    }
}


static int 
GetToplevelWindow(Tcl_Interp *interp, Tcl_Obj *objPtr, Tk_Window *tkwinPtr)
{
    const char *string;
    Tk_Window tkwin;

    string = Tcl_GetString(objPtr);
    tkwin = Tk_NameToWindow(interp, string, Tk_MainWindow(interp));
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    if (!Tk_IsTopLevel(tkwin)) {
	Tcl_AppendResult(interp, "kiosk window \"", Tcl_GetString(objPtr), 
		"\" must be a top level window", (char *)NULL);	
	return TCL_ERROR;
    }
    *tkwinPtr = tkwin;
    return TCL_OK;
}

static int 
GetKioskFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Kiosk **kioskPtrPtr)
{
    Tk_Window tkwin;
    int isNew;
    Blt_HashEntry *hPtr;

    if (GetToplevelWindow(interp, objPtr, &tkwin) != TCL_OK) {
	return TCL_ERROR;
    }
    hPtr = Blt_CreateHashEntry(&kioskTable, (char *)tkwin, &isNew);
    if (isNew) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, 
		"can't find a kiosk associated with window \"", 
		Tcl_GetString(objPtr), "\"", (char *)NULL);	
	}
	return TCL_ERROR;
    }
    *kioskPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
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
 *	etc. get set for contPtr; old resources get freed, if there
 *	were any.  The widget is redisplayed.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureKiosk(Tcl_Interp *interp, Kiosk *kioskPtr, int objc, 
	       Tcl_Obj *const *objv, int flags)
{
    if (Blt_ConfigureWidgetFromObj(interp, kioskPtr->tkwin, configSpecs, 
	objc, objv, (char *)kioskPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * CgetOp --
 *
 *	kiosk cget window option
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CgetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv)
{
    Kiosk *kioskPtr;

    if (GetKioskFromObj(interp, objv[2], &kioskPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return Blt_ConfigureValueFromObj(interp, kioskPtr->tkwin, configSpecs,
	(char *)kioskPtr, objv[3], 0);
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
 *	set for contPtr; old resources get freed, if there were any.  The
 *	widget is redisplayed.
 *
 *
 *	kiosk configure toplevel option value 
 *---------------------------------------------------------------------------
 */
static int
ConfigureOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv)
{
    Kiosk *kioskPtr;

    if (GetKioskFromObj(interp, objv[2], &kioskPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (objc == 3) {
	return Blt_ConfigureInfoFromObj(interp, kioskPtr->tkwin, configSpecs,
	    (char *)kioskPtr, (Tcl_Obj *)NULL, 0);
    } else if (objc == 4) {
	return Blt_ConfigureInfoFromObj(interp, kioskPtr->tkwin, configSpecs,
	    (char *)kioskPtr, objv[3], 0);
    }
    if (ConfigureKiosk(interp, kioskPtr, objc - 3, objv + 3, 
		BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/* 
 * CreateOp -- 
 *
 *	blt::kiosk create .toplevel options
 *
 */
/*ARGSUSED*/
static int
CreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	  Tcl_Obj *const *objv)
{
    Tk_Window tkwin;
    int isNew;
    Kiosk *kioskPtr;
    Blt_HashEntry *hPtr;
    Window root;
    Tk_ErrorHandler handler;
    int any = -1;
    int result;
    unsigned long mask;

    if (GetToplevelWindow(interp, objv[2], &tkwin) != TCL_OK) {
	return TCL_ERROR;
    }
    hPtr = Blt_CreateHashEntry(&kioskTable, (char *)tkwin, &isNew);
    if (!isNew) {
	Tcl_AppendResult(interp, 
		"a kiosk is already associated with window \"", 
		Tcl_GetString(objv[2]), "\"", (char *)NULL);	
	return TCL_ERROR;
    }
    kioskPtr = NewKiosk(interp, tkwin, hPtr);
    if (ConfigureKiosk(interp, kioskPtr, objc - 3, objv + 3, 0) != TCL_OK) {
	DestroyKiosk(kioskPtr);
	return TCL_ERROR;
    }
    Tk_CreateGenericHandler(GenericEventProc, kioskPtr);
    result = TRUE;
    handler = Tk_CreateErrorHandler(Tk_Display(tkwin), any, any, any, 
	XSelectInputErrorProc, &result);
    Tk_MakeWindowExist(tkwin);
    root = Tk_RootWindow(tkwin);
    mask = SubstructureRedirectMask | SubstructureNotifyMask |
	ResizeRedirectMask;
    XSelectInput(Tk_Display(tkwin), root, mask);
    XSync(Tk_Display(tkwin), False);
    Tk_DeleteErrorHandler(handler);
    if (!result) {
	Tcl_AppendResult(interp, 
		"a window manager is already installed for window \"", 
		Tcl_GetString(objv[2]), "\"", (char *)NULL);	
	return TCL_ERROR;
    }
    ManageExistingWindows(kioskPtr);
    ResizeKiosk(kioskPtr);
    return TCL_OK;
}

/* 
 * DeleteOp -- 
 *
 *
 *	blt::kiosk delete .toplevel
 *
 */
/*ARGSUSED*/
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	  Tcl_Obj *const *objv)
{
    Kiosk *kioskPtr;

    if (GetKioskFromObj(interp, objv[2], &kioskPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * KioskCmd --
 *
 * 	This procedure is invoked to process the "kiosk" command.  See the
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
static Blt_OpSpec kioskSpecs[] =
{
    {"cget",      2, CgetOp,      4, 4, "window option",},
    {"configure", 2, ConfigureOp, 4, 0, "window ?option value?...",},
    {"create",    2, CreateOp,    3, 0, "window ?option value?...",},
    {"delete",    1, DeleteOp,    3, 3, "window",},
};

static int numKioskSpecs = sizeof(kioskSpecs) / sizeof(Blt_OpSpec);

static int
KioskCmd(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to report errors. */
    int objc,				/* # of arguments. */
    Tcl_Obj *const *objv)		/* Vector of argument strings. */
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numKioskSpecs, kioskSpecs, BLT_OP_ARG1, 
	objc, objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc)(clientData, interp, objc, objv);
}

int
Blt_KioskCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { "kiosk", KioskCmd, };

    Blt_InitHashTable(&kioskTable, BLT_ONE_WORD_KEYS);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}


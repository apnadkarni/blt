/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltDragdrop.c --
 *
 * This module implements a drag-and-drop mechanism for the Tk Toolkit.
 * Allows widgets to be registered as drag&drop sources and targets for
 * handling "drag-and-drop" operations between Tcl/Tk applications.
 *
 * The "drag&drop" command was created by Michael J. McLennan.
 *
 *	Copyright 1993-1998 Lucent Technologies, Inc.
 *
 *	Permission to use, copy, modify, and distribute this software
 *	and its documentation for any purpose and without fee is
 *	hereby granted, provided that the above copyright notice
 *	appear in all copies and that both that the copyright notice
 *	and warranty disclaimer appear in supporting documentation,
 *	and that the names of Lucent Technologies any of their
 *	entities not be used in advertising or publicity pertaining to
 *	distribution of the software without specific, written prior
 *	permission.
 *
 *	Lucent Technologies disclaims all warranties with regard to
 *	this software, including all implied warranties of
 *	merchantability and fitness.  In no event shall Lucent
 *	Technologies be liable for any special, indirect or
 *	consequential damages or any damages whatsoever resulting from
 *	loss of use, data or profits, whether in an action of
 *	contract, negligence or other tortuous action, arising out of
 *	or in connection with the use or performance of this software.
 *
 *	Copyright 1998-2004 George A Howlett.
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
 *
 */
#define BUILD_BLT_TK_PROCS 1
#include "bltInt.h"

#ifndef NO_DRAGDROP

#ifdef HAVE_STRING_H
#  include <string.h>
#endif /* HAVE_STRING_H */

#include <X11/Xatom.h>

#include "bltAlloc.h"
#include "bltHash.h"
#include "bltChain.h"
#include "tkDisplay.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define DRAGDROP_THREAD_KEY "BLT Dragdrop Command Data"

#ifdef WIN32
#define MAX_PROP_SIZE 255	/* Maximum size of property. */
typedef HWND WINDOW;
#else
#define MAX_PROP_SIZE 1000	/* Maximum size of property. */
typedef Window WINDOW;
static Atom dndAtom;
#endif

/*
 *	Each "drag&drop" target widget is tagged with a "BltDrag&DropTarget" 
 *	property in XA_STRING format.  This property identifies the window 
 *	as a "drag&drop" target.  It's formated as a TCL list and contains
 *	the following information:
 *
 *	    "INTERP_NAME TARGET_NAME DATA_TYPE DATA_TYPE ..."
 *
 *	  INTERP_NAME	Name of the target application's interpreter.
 *	  TARGET_NAME	Path name of widget registered as the drop target.  
 *	  DATA_TYPE	One or more "types" handled by the target.
 *
 *	When the user invokes the "drag" operation, the window hierarchy
 *	is progressively examined.  Window information is cached during
 *	the operation, to minimize X server traffic. Windows carrying a
 *	"BltDrag&DropTarget" property are identified.  When the token is 
 *	dropped over a valid site, the drop information is sent to the 
 *	application 
 *	via the usual "send" command.  If communication fails, the drag&drop 
 *	facility automatically posts a rejection symbol on the token window.  
 */

#define INTERP_NAME	0
#define TARGET_NAME	1
#define DATA_TYPE	2

/* Error Proc used to report drag&drop background errors */
#define DEF_ERROR_PROC              "bgerror"
/*
 *  CONFIG PARAMETERS
 */
#define DEF_DND_BUTTON_BACKGROUND		RGB_YELLOW
#define DEF_DND_BUTTON_NUMBER		"3"
#define DEF_DND_PACKAGE_COMMAND		(char *)NULL
#define DEF_DND_SELF_TARGET		"no"
#define DEF_DND_SEND			"all"
#define DEF_DND_SITE_COMMAND		(char *)NULL
#define DEF_TOKEN_ACTIVE_BACKGROUND	STD_ACTIVE_BACKGROUND
#define DEF_TOKEN_ACTIVE_BORDERWIDTH	"3"
#define DEF_TOKEN_ACTIVE_RELIEF		"sunken"
#define DEF_TOKEN_ANCHOR		"se"
#define DEF_TOKEN_BACKGROUND		STD_NORMAL_BACKGROUND
#define DEF_TOKEN_BORDERWIDTH		"3"
#define DEF_TOKEN_CURSOR		"arrow"
#define DEF_TOKEN_OUTLINE_COLOR		RGB_BLACK
#define DEF_TOKEN_REJECT_BACKGROUND	STD_NORMAL_BACKGROUND
#define DEF_TOKEN_REJECT_FOREGROUND	RGB_RED
#define DEF_TOKEN_REJECT_STIPPLE_COLOR	(char *)NULL
#define DEF_TOKEN_RELIEF		"raised"

static char dragDropCmd[] = "::blt::drag&drop";

static char className[] = "BltDragDropToken";	/* CLASS NAME of token window */
static char propName[] = "BltDrag&DropTarget";	/* Property name */

#ifndef WIN32
static int initialized = FALSE;
#endif

typedef struct {
    Blt_HashTable sourceTable;
    Blt_HashTable targetTable;
    int numActive;
    int locX, locY;
    Tcl_Interp *interp;
    Tk_Window tkMain;		/* Main window of the interpreter. */
} DragdropCmdInterpData;

/*
 *  Percent substitutions
 */
typedef struct {
    char letter;		/* character like 'x' in "%x" */
    const char *value;		/* value to be substituted in place of "%x" */
} SubstDescriptors;


/*
 *  AnyWindow --
 *
 *	This structure represents a window hierarchy examined during
 *	a single "drag" operation.  It's used to cache information
 *	to reduce the round-trip calls to the server needed to query
 *	window geometry information and grab the target property.
 */
typedef struct _AnyWindow AnyWindow;

struct _AnyWindow {
    WINDOW nativeWindow;	/* Native window: HWINDOW (Win32) or 
				 * Window (X11). */

    int initialized;		/* If non-zero, the rest of this structure's
				 * information had been previously built. */

    int x1, y1, x2, y2;		/* Extents of the window (upper-left and
				 * lower-right corners). */

    AnyWindow *parentPtr;	/* Parent node. NULL if root. Used to
				  * compute offset for X11 windows. */

    Blt_Chain chain;		/* List of this window's children. If NULL,
				 * there are no children. */

    const char **targetInfo;	/* An array of target window drag&drop
				 * information: target interpreter,
				 * pathname, and optionally possible
				 * type matches. NULL if the window is
				 * not a drag&drop target or is not a
				 * valid match for the drop source. */

};

/*
 *  Drag&Drop Registration Data
 */
typedef struct {

    /*
     * This is a goof in the Tk API.  It assumes that only an official
     * Tk "toplevel" widget will ever become a toplevel window (i.e. a
     * window whose parent is the root window).  Because under Win32,
     * Tk tries to use the widget record associated with the TopLevel
     * as a Tk frame widget, to read its menu name.  What this means
     * is that any widget that's going to be a toplevel, must also look
     * like a frame. Therefore we've copied the frame widget structure
     * fields into the token.
     */

    Tk_Window tkwin;		/* Window that embodies the frame.  NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up. */
    Display *display;		/* Display containing widget.  Used, among
				 * other things, so that resources can be
				 * freed even after tkwin has gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with widget.  Used
				 * to delete widget command. */
    Tcl_Command widgetCmd;	/* Token for frame's widget command. */
    const char *className;	/* Class name for widget (from configuration
				 * option).  Malloc-ed. */
    int mask;			/* Either FRAME or TOPLEVEL;  used to select
				 * which configuration options are valid for
				 * widget. */
    const char *screenName;    /* Screen on which widget is created.  Non-null
				 * only for top-levels.  Malloc-ed, may be
				 * NULL. */
    const char *visualName;	/* Textual description of visual for window,
				 * from -visual option.  Malloc-ed, may be
				 * NULL. */
    const char *colormapName;	/* Textual description of colormap for window,
				 * from -colormap option.  Malloc-ed, may be
				 * NULL. */
    const char *menuName;	/* Textual description of menu to use for
				 * menubar. Malloc-ed, may be NULL. */
    Colormap colormap;		/* If not None, identifies a colormap
				 * allocated for this window, which must be
				 * freed when the window is deleted. */
    Tk_3DBorder border;		/* Structure used to draw 3-D border and
				 * background.  NULL means no background
				 * or border. */
    int borderWidth;		/* Width of 3-D border (if any). */
    int relief;			/* 3-d effect: TK_RELIEF_RAISED etc. */
    int highlightWidth;		/* Width in pixels of highlight to draw
				 * around widget when it has the focus.
				 * 0 means don't draw a highlight. */
    XColor *highlightBgColorPtr;
				/* Color for drawing traversal highlight
				 * area when highlight is off. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    int width;			/* Width to request for window.  <= 0 means
				 * don't request any size. */
    int height;			/* Height to request for window.  <= 0 means
				 * don't request any size. */
    Tk_Cursor cursor;		/* Current cursor for window, or None. */
    const char *takeFocus;	/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */
    int isContainer;		/* 1 means this window is a container, 0 means
				 * that it isn't. */
    const char *useThis;	/* If the window is embedded, this points to
				 * the name of the window in which it is
				 * embedded (malloc'ed).  For non-embedded
				 * windows this is NULL. */
    int flags;			/* Various flags;  see below for
				 * definitions. */

    /* Token specific fields */

    int lastX, lastY;		/* last position of token window */
    int active;			/* non-zero => over target window */
    Tcl_TimerToken timer;	/* token for routine to hide tokenwin */
    GC rejectFgGC;		/* GC used to draw rejection fg: (\) */
    GC rejectBgGC;		/* GC used to draw rejection bg: (\) */

    /* User-configurable fields */

    Tk_Anchor anchor;		/* Position of token win relative to mouse */
    Tk_3DBorder outline;	/* Outline border around token window */
    Tk_3DBorder normalBorder;	/* Border/background for token window */
    Tk_3DBorder activeBorder;	/* Border/background for token window */
    int activeRelief;
    int activeBW;	/* Border width in pixels */
    XColor *rejectFg;		/* Color used to draw rejection fg: (\) */
    XColor *rejectBg;		/* Color used to draw rejection bg: (\) */
    Pixmap rejectStipple;	/* Stipple used to draw rejection: (\) */
} Token;

typedef struct {
    Tcl_Interp *interp;		/* Interpreter associated with the Tk source 
				 * widget. */

    Tk_Window tkwin;		/* Tk window registered as the drag&drop 
				 * source. */

    Display *display;		/* Drag&drop source window display */

    Blt_HashTable handlerTable;	/* Table of data handlers (converters)
				 * registered for this source. */

    int button;			/* Button used to invoke drag operation. */

    Token token;		/* Token used to provide special cursor. */
    
    int pkgCmdInProgress;	/* Indicates if a pkgCmd is currently active. */
    const char *pkgCmd;		/* TCL command executed at start of "drag"
				 * operation to gather information about 
				 * the source data. */

    const char *pkgCmdResult;	/* Result returned by the most recent 
				 * pkgCmd. */

    const char *siteCmd;	/* TCL command executed to update token 
				 * window. */

    AnyWindow *rootPtr;		/* Cached window information: Gathered
				 * and used during the "drag" operation 
				 * to see if the mouse pointer is over a 
				 * valid target. */

    int selfTarget;		/* Indicated if the source should drop onto 
				 * itself. */

    Tk_Cursor cursor;		/* cursor restored after dragging */

    const char **sendTypes;	/* list of data handler names or "all" */

    Blt_HashEntry *hashPtr;

    AnyWindow *windowPtr;	/* Last target examined. If NULL, mouse 
				 * pointer is not currently over a valid 
				 * target. */
    Tcl_Obj *errorCmdObjPtr;	
    DragdropCmdInterpData *dataPtr;
} Source;

typedef struct {
    Tcl_Interp *interp;
    Tk_Window tkwin;		/* drag&drop target window */
    Display *display;		/* drag&drop target window display */
    Blt_HashTable handlerTable;	/* Table of data handlers (converters)
				 * registered for this target. */
    Blt_HashEntry *hashPtr;
    DragdropCmdInterpData *dataPtr;
} Target;

static Blt_ConfigSpec configSpecs[] =
{
    {BLT_CONFIG_INT, "-button", "buttonBinding", "ButtonBinding",
	DEF_DND_BUTTON_NUMBER, Blt_Offset(Source, button), 0},
    {BLT_CONFIG_OBJ, "-errorcmd", "errorCommand", "ErrorCommand",
	"bgerror", Blt_Offset(Source, errorCmdObjPtr), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-packagecmd", "packageCommand", "PackageCommand",
	DEF_DND_PACKAGE_COMMAND, Blt_Offset(Source, pkgCmd), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_COLOR, "-rejectbg", "rejectBackground", "Background",
	DEF_TOKEN_REJECT_BACKGROUND, Blt_Offset(Source, token.rejectBg), 0},
    {BLT_CONFIG_COLOR, "-rejectfg", "rejectForeground", "Foreground",
	DEF_TOKEN_REJECT_FOREGROUND, Blt_Offset(Source, token.rejectFg), 0},
    {BLT_CONFIG_BITMAP, "-rejectstipple", "rejectStipple", "Stipple",
	DEF_TOKEN_REJECT_STIPPLE_COLOR, 
	Blt_Offset(Source, token.rejectStipple), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BOOLEAN, "-selftarget", "selfTarget", "SelfTarget",
	DEF_DND_SELF_TARGET, Blt_Offset(Source, selfTarget), 0},
    {BLT_CONFIG_LIST, "-send", "send", "Send", DEF_DND_SEND, 
	Blt_Offset(Source, sendTypes), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_STRING, "-sitecmd", "siteCommand", "Command",
	DEF_DND_SITE_COMMAND, Blt_Offset(Source, siteCmd), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_ANCHOR, "-tokenanchor", "tokenAnchor", "Anchor",
	DEF_TOKEN_ANCHOR, Blt_Offset(Source, token.anchor), 0},
    {BLT_CONFIG_BORDER, "-tokenactivebackground", "tokenActiveBackground", 
	"ActiveBackground", DEF_TOKEN_ACTIVE_BACKGROUND, 
	Blt_Offset(Source, token.activeBorder), 0},
    {BLT_CONFIG_BORDER, "-tokenbg", "tokenBackground", "Background",
	DEF_TOKEN_BACKGROUND, Blt_Offset(Source, token.normalBorder), 0},
    {BLT_CONFIG_BORDER, "-tokenoutline", "tokenOutline", "Outline",
	DEF_TOKEN_OUTLINE_COLOR, Blt_Offset(Source, token.outline), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-tokenborderwidth", "tokenBorderWidth", 
	"BorderWidth", DEF_TOKEN_BORDERWIDTH, 
	Blt_Offset(Source, token.borderWidth), 0},
    {BLT_CONFIG_CURSOR, "-tokencursor", "tokenCursor", "Cursor",
	DEF_TOKEN_CURSOR, Blt_Offset(Source, token.cursor), 
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0},
};

static Blt_ConfigSpec tokenConfigSpecs[] =
{
    {BLT_CONFIG_BORDER, "-activebackground", "activeBackground",
	"ActiveBackground", DEF_TOKEN_ACTIVE_BACKGROUND, 
	Blt_Offset(Token, activeBorder), 0},
    {BLT_CONFIG_RELIEF, "-activerelief", "activeRelief", "activeRelief",
	DEF_TOKEN_ACTIVE_RELIEF, Blt_Offset(Token, activeRelief), 0},
    {BLT_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",
	DEF_TOKEN_ANCHOR, Blt_Offset(Token, anchor), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-activeborderwidth", "activeBorderWidth",
	"ActiveBorderWidth", DEF_TOKEN_ACTIVE_BORDERWIDTH, 
	Blt_Offset(Token, activeBW), 0},
    {BLT_CONFIG_BORDER, "-background", "background", "Background",
	DEF_TOKEN_BACKGROUND, Blt_Offset(Token, normalBorder), 0},
    {BLT_CONFIG_PIXELS_NNEG, "-borderwidth", "borderWidth", "BorderWidth", 
	DEF_TOKEN_BORDERWIDTH, Blt_Offset(Token, borderWidth), 0},
    {BLT_CONFIG_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_TOKEN_CURSOR, Blt_Offset(Token, cursor), BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_BORDER, "-outline", "outline", "Outline",
	DEF_TOKEN_OUTLINE_COLOR, Blt_Offset(Token, outline), 0},
    {BLT_CONFIG_COLOR, "-rejectbg", "rejectBackground", "Background",
	DEF_TOKEN_REJECT_BACKGROUND, Blt_Offset(Token, rejectBg), 0},
    {BLT_CONFIG_COLOR, "-rejectfg", "rejectForeground", "Foreground",
	DEF_TOKEN_REJECT_FOREGROUND, Blt_Offset(Token, rejectFg), 0},
    {BLT_CONFIG_BITMAP, "-rejectstipple", "rejectStipple", "Stipple",
	DEF_TOKEN_REJECT_STIPPLE_COLOR, Blt_Offset(Token, rejectStipple),
	BLT_CONFIG_NULL_OK},
    {BLT_CONFIG_RELIEF, "-relief", "relief", "Relief",
	DEF_TOKEN_RELIEF, Blt_Offset(Token, relief), 0},
    {BLT_CONFIG_END, (char *)NULL, (char *)NULL, (char *)NULL, (char *)NULL, 
	0, 0},
};


/*
 *  Forward Declarations
 */
static Tcl_ObjCmdProc DragDropCmd;
static Tk_EventProc TokenEventProc;
static Tk_EventProc TargetEventProc;

static void MoveToken(Source * srcPtr, Token *tokenPtr);
static void UpdateToken(ClientData clientData);
static void HideToken(Token *tokenPtr);
static void RejectToken(Token *tokenPtr);

static int GetSourceFromObj(DragdropCmdInterpData *dataPtr, Tcl_Interp *interp,
	Tcl_Obj *objPtr, Source **srcPtrPtr);
static Source *CreateSource(DragdropCmdInterpData *dataPtr, Tcl_Interp *interp,
	Tcl_Obj *objPtr, int *newEntry);
static void DestroySource(Source * srcPtr);
static void SourceEventProc(ClientData clientData, XEvent *eventPtr);
static int ConfigureSource(Tcl_Interp *interp, Source * srcPtr, int objc, 
	Tcl_Obj *const *objv, int flags);
static int ConfigureToken(Tcl_Interp *interp, Source * srcPtr, int objc, 
	Tcl_Obj *const *objv);

static Target *CreateTarget(DragdropCmdInterpData *dataPtr, Tcl_Interp *interp,
	Tk_Window tkwin);
static Target *FindTarget(DragdropCmdInterpData *dataPtr, Tk_Window tkwin);
static void DestroyTarget(DestroyData data);
static int OverTarget(Source * srcPtr, int x, int y);
static void AddTargetProperty(Tcl_Interp *interp, Target *targetPtr);

static void DndSend(Source *srcPtr);

static void InitRoot(Source * srcPtr);
static void RemoveWindow(AnyWindow *wr);
static void QueryWindow(Display *display, AnyWindow * windowPtr);

static const char *ExpandPercents(const char *str, SubstDescriptors *subs, 
	int nsubs, Tcl_DString *resultPtr);


/*
 *---------------------------------------------------------------------------
 *
 * DragdropInterpDeleteProc --
 *
 *	This is called when the interpreter hosting the "dragdrop"
 *	command is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the hash table managing all dragdrop names.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
DragdropInterpDeleteProc(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp)
{
    DragdropCmdInterpData *dataPtr = clientData;

    /* All dragdrop instances should already have been destroyed when
     * their respective TCL commands were deleted. */
    Blt_DeleteHashTable(&dataPtr->sourceTable);
    Blt_DeleteHashTable(&dataPtr->targetTable);
    Tcl_DeleteAssocData(interp, DRAGDROP_THREAD_KEY);
    Blt_Free(dataPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetDragdropdCmdInterpData --
 *
 *---------------------------------------------------------------------------
 */
static DragdropCmdInterpData *
GetDragdropCmdInterpData(Tcl_Interp *interp)
{
    DragdropCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (DragdropCmdInterpData *)
	Tcl_GetAssocData(interp, DRAGDROP_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
	dataPtr = Blt_AssertMalloc(sizeof(DragdropCmdInterpData));
	dataPtr->interp = interp;
	dataPtr->tkMain = Tk_MainWindow(interp);
	Tcl_SetAssocData(interp, DRAGDROP_THREAD_KEY, DragdropInterpDeleteProc,
		 dataPtr);
	Blt_InitHashTable(&dataPtr->sourceTable, BLT_ONE_WORD_KEYS);
	Blt_InitHashTable(&dataPtr->targetTable, BLT_ONE_WORD_KEYS);
	dataPtr->numActive = 0;
	dataPtr->locX = dataPtr->locY = 0;
    }
    return dataPtr;
}


#ifdef	WIN32

#if defined( _MSC_VER) || defined(__BORLANDC__)
#include <tchar.h>
#endif /* _MSC_VER || __BORLANDC__ */

typedef struct {
    const char *prefix;
    int prefixSize;
    const char *propReturn;
} PropertyInfo;


#ifdef notdef
static BOOL CALLBACK
GetEnumWindowsProc(HWND hWnd, LPARAM clientData)
{
    Blt_Chain chain = (Blt_Chain)clientData;

    Blt_Chain_Append(chain, (ClientData)hWnd);
    return TRUE;
}
#endif

static WINDOW
GetNativeWindow(Tk_Window tkwin)
{
    return (WINDOW) Tk_GetHWND(Tk_WindowId(tkwin));
}


/*
 *---------------------------------------------------------------------------
 *
 *  GetWindowZOrder --
 *
 *	Returns a list of the child windows according to their stacking
 *	order.  The window handles are ordered from top to bottom.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain
GetWindowZOrder(Display *display, HWND parent)
{
    Blt_Chain chain;
    HWND hWnd;

    chain = Blt_Chain_Create();
    for (hWnd = GetTopWindow(parent); hWnd != NULL;
	hWnd = GetNextWindow(hWnd, GW_HWNDNEXT)) {
	Blt_Chain_Append(chain, (ClientData)hWnd);
    }
    return chain;
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 *  GetEnumPropsExProc --
 *
 *---------------------------------------------------------------------------
 */
static BOOL CALLBACK
GetEnumPropsExProc(
    HWND hwnd, 
    LPCTSTR atom, 
    HANDLE hData, 
    DWORD clientData)
{
    PropertyInfo *infoPtr = (PropertyInfo *) clientData;

    if (strncmp(infoPtr->prefix, atom, infoPtr->prefixSize) == 0) {
	assert(infoPtr->propReturn == NULL);
	infoPtr->propReturn = (const char *)atom;
	return FALSE;
    }
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetPropData --
 *
 *	This is a bad Windows hack to pass property information between
 *	applications.  (Ab)Normally the property data (one-word value) is
 *	stored in the data handle.  But the data content is available only
 *	within the application.  The pointer value is meaningless outside
 *	of the current application address space.  Not really useful at all.
 *
 *	So the trick here is to encode the property name with all the
 *	necessary information and to loop through all the properties
 *	of a window, looking for one that starts with our property name
 *	prefix.  The downside is that the property name is limited to
 *	255 bytes.  But that should be enough.  It's also slower since
 *	we examine each property until we find ours.
 *
 *	We'll plug in the OLE stuff later.
 *
 *---------------------------------------------------------------------------
 */

static const char *
GetPropData(HWND hWnd, char *atom)
{
    PropertyInfo propInfo;
    if (hWnd == NULL) {
	return NULL;
    }
    propInfo.prefix = atom;
    propInfo.prefixSize = strlen(atom);
    propInfo.propReturn = NULL;
    EnumPropsEx(hWnd, (PROPENUMPROCEX)GetEnumPropsExProc, (DWORD)&propInfo);
    return propInfo.propReturn;
}
#endif

static const unsigned char *
GetProperty(Display *display, HWND hWnd)
{
    HANDLE handle;
    
    handle = GetProp(hWnd, propName);
    if (handle != NULL) {
	ATOM atom;
	char buffer[MAX_PROP_SIZE + 1];
	UINT numBytes;

	atom = (ATOM)((int)handle);
	numBytes = GlobalGetAtomName(atom, buffer, MAX_PROP_SIZE);
	if (numBytes > 0) {
	    buffer[numBytes] = '\0';
	    return (BYTE *)Blt_AssertStrdup(buffer);
	}
    }
    return NULL;
}

static void
SetProperty(Tk_Window tkwin, const char *data)
{
    HANDLE handle;
    HWND hWnd;
    ATOM atom;

    hWnd = Tk_GetHWND(Tk_WindowId(tkwin));
    if (hWnd == NULL) {
	return;
    }
    handle = GetProp(hWnd, propName);
    atom = (ATOM)((int)handle);
    if (atom != (ATOM)0) {
	GlobalDeleteAtom(atom);
    }
    atom = GlobalAddAtom((char *)data);
    if (atom != (ATOM)0) {
	handle = (HANDLE)((int)atom);
	SetProp(hWnd, propName, handle);
    }
}

static void
RemoveProperty(Tk_Window tkwin)
{
    HWND hWnd;
    HANDLE handle;

    hWnd = Tk_GetHWND(Tk_WindowId(tkwin));
    if (hWnd == NULL) {
	return;
    }
    handle = GetProp(hWnd, propName);
    if (handle != NULL) {
	ATOM atom;

	atom = (ATOM)((int)handle);
	GlobalDeleteAtom(atom);
    }
    RemoveProp(hWnd, propName);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetWindowRegion --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
GetWindowRegion(
    Display *display,		/* Not used. */
    HWND hWnd,
    int *x1Ptr,
    int *y1Ptr,
    int *x2Ptr,
    int *y2Ptr)
{
    RECT rect;

    if (GetWindowRect(hWnd, &rect)) {
	*x1Ptr = rect.left;
	*y1Ptr = rect.top;
	*x2Ptr = rect.right;
	*y2Ptr = rect.bottom;
	return IsWindowVisible(hWnd);
    }
    return FALSE;
}

#else

static WINDOW
GetNativeWindow(Tk_Window tkwin)
{
    return Tk_WindowId(tkwin);
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetWindowZOrder --
 *
 *	Returns a chain of the child windows according to their stacking
 *	order.  The window ids are ordered from top to bottom.
 *
 *---------------------------------------------------------------------------
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
	     *  XQuery returns windows in bottom to top order.
	     *  We only care about the top window.
	     */
	    Blt_Chain_Prepend(chain, (ClientData)winv[i]);
	}
	if (winv != NULL) {
	    XFree((char *)winv);	/* done with list of kids */
	}
    }
    return chain;
}

static const unsigned char *
GetProperty(
    Display *display,
    Window window)
{
    unsigned char *data;
    int result, actualFormat;
    Atom actualType;
    unsigned long numItems, bytesAfter;

    if (window == None) {
	return NULL;
    }
    data = NULL;
    result = XGetWindowProperty(display, window, dndAtom, 0, MAX_PROP_SIZE,
	False, XA_STRING, &actualType, &actualFormat, &numItems, &bytesAfter,
	&data);
    if ((result != Success) || (actualFormat != 8) ||
	(actualType != XA_STRING)) {
	if (data != NULL) {
	    XFree(data);
	    data = NULL;
	}
    }
    return data;
}

static void
SetProperty(Tk_Window tkwin, char *data)
{
    XChangeProperty(Tk_Display(tkwin), Tk_WindowId(tkwin), dndAtom, XA_STRING,
	8, PropModeReplace, (unsigned char *)data, strlen(data) + 1);
}

static int
GetWindowRegion(
    Display *display,
    Window window,
    int *x1Ptr, int *y1Ptr, 
    int *x2Ptr, int *y2Ptr)
{
    XWindowAttributes winAttrs;

    if (XGetWindowAttributes(display, window, &winAttrs)) {
	*x1Ptr = winAttrs.x;
	*y1Ptr = winAttrs.y;
	*x2Ptr = winAttrs.x + winAttrs.width - 1;
	*y2Ptr = winAttrs.y + winAttrs.height - 1;
    }
    return (winAttrs.map_state == IsViewable);
}

#endif /* WIN32 */

/*
 *---------------------------------------------------------------------------
 *
 *  ChangeToken --
 *
 *---------------------------------------------------------------------------
 */
static void
ChangeToken(Token *tokenPtr, int active)
{
    int relief;
    Tk_3DBorder border;
    int borderWidth;

    Blt_Fill3DRectangle(tokenPtr->tkwin, Tk_WindowId(tokenPtr->tkwin),
	tokenPtr->outline, 0, 0, Tk_Width(tokenPtr->tkwin),
	Tk_Height(tokenPtr->tkwin), 0, TK_RELIEF_FLAT);
    if (active) {
	relief = tokenPtr->activeRelief;
	border = tokenPtr->activeBorder;
	borderWidth = tokenPtr->activeBW;
    } else {
	relief = tokenPtr->relief;
	border = tokenPtr->normalBorder;
	borderWidth = tokenPtr->borderWidth;
    }
    Blt_Fill3DRectangle(tokenPtr->tkwin, Tk_WindowId(tokenPtr->tkwin), border, 
	2, 2, Tk_Width(tokenPtr->tkwin) - 4, Tk_Height(tokenPtr->tkwin) - 4, 
	borderWidth, relief);
}

/*
 *---------------------------------------------------------------------------
 *
 *  TokenEventProc --
 *
 *	Invoked by the Tk dispatcher to handle widget events.
 *	Manages redraws for the drag&drop token window.
 *
 *---------------------------------------------------------------------------
 */
static void
TokenEventProc(
    ClientData clientData,	/* data associated with widget */
    XEvent *eventPtr)		/* information about event */
{
    Token *tokenPtr = clientData;

    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
	if (tokenPtr->tkwin != NULL) {
	    ChangeToken(tokenPtr, tokenPtr->active);
	}
    } else if (eventPtr->type == DestroyNotify) {
	tokenPtr->tkwin = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  HideToken --
 *
 *	Unmaps the drag&drop token.  Invoked directly at the end of a
 *	successful communication, or after a delay if the communication
 *	fails (allowing the user to see a graphical picture of failure).
 *
 *---------------------------------------------------------------------------
 */
static void
HideToken(Token *tokenPtr)
{
    if (tokenPtr->tkwin != NULL) {
	Tk_UnmapWindow(tokenPtr->tkwin);
    }
    tokenPtr->timer = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 *  RaiseToken --
 *
 *---------------------------------------------------------------------------
 */
static void
RaiseToken(Token *tokenPtr)
{
    Blt_MapToplevelWindow(tokenPtr->tkwin);
    Blt_RaiseToplevelWindow(tokenPtr->tkwin);
}

/*
 *---------------------------------------------------------------------------
 *
 *  MoveToken --
 *
 *	Invoked during "drag" operations to move a token window to its
 *	current "drag" coordinate.
 *
 *---------------------------------------------------------------------------
 */
static void
MoveToken(
    Source *srcPtr,		/* drag&drop source window data */
    Token *tokenPtr)
{
    int x, y;
    int maxX, maxY;
    int vx, vy, vw, vh;
    int screenWidth, screenHeight;

    Blt_SizeOfScreen(srcPtr->tkwin, &screenWidth, &screenHeight);

    /* Adjust current location for virtual root windows.  */
    Tk_GetVRootGeometry(srcPtr->tkwin, &vx, &vy, &vw, &vh);
    x = tokenPtr->lastX + vx - 3;
    y = tokenPtr->lastY + vy - 3;

    maxX = screenWidth - Tk_Width(tokenPtr->tkwin);
    maxY = screenHeight - Tk_Height(tokenPtr->tkwin);
    Blt_TranslateAnchor(x, y, Tk_Width(tokenPtr->tkwin),
	Tk_Height(tokenPtr->tkwin), tokenPtr->anchor, &x, &y);
    if (x > maxX) {
	x = maxX;
    } else if (x < 0) {
	x = 0;
    }
    if (y > maxY) {
	y = maxY;
    } else if (y < 0) {
	y = 0;
    }
    if ((x != Tk_X(tokenPtr->tkwin)) || (y != Tk_Y(tokenPtr->tkwin))) {
	Tk_MoveToplevelWindow(tokenPtr->tkwin, x, y);
    }
    RaiseToken(tokenPtr);
}

static Tk_Cursor
GetWidgetCursor(
    Tcl_Interp *interp,
    Tk_Window tkwin)
{
    const char *cursorName;
    Tk_Cursor cursor;

    cursor = None;
    if (Tcl_VarEval(interp, Tk_PathName(tkwin), " cget -cursor",
	    (char *)NULL) != TCL_OK) {
	return None;
    }
    cursorName = Tcl_GetStringResult(interp);
    if ((cursorName != NULL) && (cursorName[0] != '\0')) {
	cursor = Tk_GetCursor(interp, tkwin, Tk_GetUid((char *)cursorName));
    }
    Tcl_ResetResult(interp);
    return cursor;
}

static void
Bgerror(Source *srcPtr)
{
    if (srcPtr->errorCmdObjPtr != NULL) {
	Tcl_Obj *objv[2];

	objv[0] = srcPtr->errorCmdObjPtr;
	objv[1] = Tcl_GetObjResult(srcPtr->interp);
	Tcl_IncrRefCount(objv[0]);
	Tcl_IncrRefCount(objv[1]);
	Tcl_EvalObjv(srcPtr->interp, 2, objv, 0);
	Tcl_DecrRefCount(objv[1]);
	Tcl_DecrRefCount(objv[0]);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  UpdateToken --
 *
 *	Invoked when the event loop is idle to determine whether or not
 *	the current drag&drop token position is over another drag&drop
 *	target.
 *
 *---------------------------------------------------------------------------
 */
static void
UpdateToken(ClientData clientData)	/* widget data */
{
    Source *srcPtr = clientData;
    Token *tokenPtr = &srcPtr->token;

    ChangeToken(tokenPtr, tokenPtr->active);
    /*
     *  If the source has a site command, then invoke it to
     *  modify the appearance of the token window.  Pass any
     *  errors onto the drag&drop error handler.
     */
    if (srcPtr->siteCmd) {
	char buffer[200];
	Tcl_DString ds;
	int result;
	SubstDescriptors subs[2];
	
	Blt_FormatString(buffer, 200, "%d", tokenPtr->active);
	subs[0].letter = 's';
	subs[0].value = buffer;
	subs[1].letter = 't';
	subs[1].value = Tk_PathName(tokenPtr->tkwin);
	
	Tcl_DStringInit(&ds);
	result = Tcl_Eval(srcPtr->interp, 
			  ExpandPercents(srcPtr->siteCmd, subs, 2, &ds));
	Tcl_DStringFree(&ds);
	if (result != TCL_OK) {
	    Bgerror(srcPtr);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  RejectToken --
 *
 *	Draws a rejection mark on the current drag&drop token, and arranges
 *	for the token to be unmapped after a small delay.
 *
 *---------------------------------------------------------------------------
 */
static void
RejectToken(Token *tokenPtr)
{
    int divisor = 6;		/* controls size of rejection symbol */
    int w, h, lineWidth, x, y, margin;

    margin = 2 * tokenPtr->borderWidth;
    w = Tk_Width(tokenPtr->tkwin) - 2 * margin;
    h = Tk_Height(tokenPtr->tkwin) - 2 * margin;
    lineWidth = (w < h) ? w / divisor : h / divisor;
    lineWidth = (lineWidth < 1) ? 1 : lineWidth;

    w = h = lineWidth * (divisor - 1);
    x = (Tk_Width(tokenPtr->tkwin) - w) / 2;
    y = (Tk_Height(tokenPtr->tkwin) - h) / 2;

    /*
     *  Draw the rejection symbol background (\) on the token window...
     */
    XSetLineAttributes(Tk_Display(tokenPtr->tkwin), tokenPtr->rejectBgGC,
	lineWidth + 4, LineSolid, CapButt, JoinBevel);

    XDrawArc(Tk_Display(tokenPtr->tkwin), Tk_WindowId(tokenPtr->tkwin),
	tokenPtr->rejectBgGC, x, y, w, h, 0, 23040);

    XDrawLine(Tk_Display(tokenPtr->tkwin), Tk_WindowId(tokenPtr->tkwin),
	tokenPtr->rejectBgGC, x + lineWidth, y + lineWidth, x + w - lineWidth,
	y + h - lineWidth);

    /*
     *  Draw the rejection symbol foreground (\) on the token window...
     */
    XSetLineAttributes(Tk_Display(tokenPtr->tkwin), tokenPtr->rejectFgGC,
	lineWidth, LineSolid, CapButt, JoinBevel);

    XDrawArc(Tk_Display(tokenPtr->tkwin), Tk_WindowId(tokenPtr->tkwin),
	tokenPtr->rejectFgGC, x, y, w, h, 0, 23040);

    XDrawLine(Tk_Display(tokenPtr->tkwin), Tk_WindowId(tokenPtr->tkwin),
	tokenPtr->rejectFgGC, x + lineWidth, y + lineWidth, x + w - lineWidth,
	y + h - lineWidth);

    /*
     *  Arrange for token window to disappear eventually.
     */
    tokenPtr->timer = Tcl_CreateTimerHandler(1000, (Tcl_TimerProc *) HideToken,
	     tokenPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 *  ConfigureToken --
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureToken(
    Tcl_Interp *interp,
    Source *srcPtr,
    int objc,
    Tcl_Obj *const *objv)
{
    Token *tokenPtr;

    tokenPtr = &srcPtr->token;
    if (Blt_ConfigureWidgetFromObj(interp, srcPtr->tkwin, tokenConfigSpecs, 
	objc, objv, (char *)tokenPtr, BLT_CONFIG_OBJV_ONLY) != TCL_OK) {
	return TCL_ERROR;
    }
    return ConfigureSource(interp, srcPtr, 0, (Tcl_Obj **)NULL,
	BLT_CONFIG_OBJV_ONLY);
}

/*
 *---------------------------------------------------------------------------
 *
 *  CreateToken --
 *
 *---------------------------------------------------------------------------
 */
static int
CreateToken(
    Tcl_Interp *interp,
    Source *srcPtr)
{
    XSetWindowAttributes attrs;
    Tk_Window tkwin;
    char string[200];
    static int nextTokenId = 0;
    unsigned int mask;
    Token *tokenPtr = &srcPtr->token;

    Blt_FormatString(string, 200, "dd-token%d", ++nextTokenId);

    /* Create toplevel on parent's screen. */
    tkwin = Tk_CreateWindow(interp, srcPtr->tkwin, string, "");
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    Tk_SetClass(tkwin, className);
    Tk_CreateEventHandler(tkwin, ExposureMask | StructureNotifyMask,
	TokenEventProc, tokenPtr);

    attrs.override_redirect = True;
    attrs.backing_store = WhenMapped;
    attrs.save_under = True;
    mask = CWOverrideRedirect | CWSaveUnder | CWBackingStore;
    Tk_ChangeWindowAttributes(tkwin, mask, &attrs);

    Tk_SetInternalBorder(tkwin, tokenPtr->borderWidth + 2);
    tokenPtr->tkwin = tkwin;
#ifdef WIN32
    {
	Tk_FakeWin *winPtr = (Tk_FakeWin *) tkwin;
	winPtr->dummy18 = tokenPtr;
    }
#endif /* WIN32 */
    Tk_MakeWindowExist(tkwin);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 *  CreateSource --
 *
 *	Looks for a Source record in the hash table for drag&drop source
 *	widgets.  Creates a new record if the widget name is not already
 *	registered.  Returns a pointer to the desired record.
 *
 *---------------------------------------------------------------------------
 */
static Source *
CreateSource(
    DragdropCmdInterpData *dataPtr,
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,		/* widget pathname for desired record */
    int *newPtr)		/* returns non-zero => new record created */
{
    char *pathName;		/* widget pathname for desired record */
    Blt_HashEntry *hPtr;
    Tk_Window tkwin;
    Source *srcPtr;

    pathName = Tcl_GetString(objPtr);
    tkwin = Tk_NameToWindow(interp, pathName, dataPtr->tkMain);
    if (tkwin == NULL) {
	return NULL;
    }
    hPtr = Blt_CreateHashEntry(&dataPtr->sourceTable, (char *)tkwin, newPtr);
    if (!(*newPtr)) {
	return Blt_GetHashValue(hPtr);
    }
    srcPtr = Blt_AssertCalloc(1, sizeof(Source));
    srcPtr->tkwin = tkwin;
    srcPtr->display = Tk_Display(tkwin);
    srcPtr->interp = interp;
    srcPtr->token.anchor = TK_ANCHOR_SE;
    srcPtr->token.relief = TK_RELIEF_RAISED;
    srcPtr->token.activeRelief = TK_RELIEF_SUNKEN;
    srcPtr->token.borderWidth = srcPtr->token.activeBW = 3;
    srcPtr->hashPtr = hPtr;
    srcPtr->dataPtr = dataPtr;
    Blt_InitHashTable(&srcPtr->handlerTable, BLT_STRING_KEYS);
    if (ConfigureSource(interp, srcPtr, 0, (Tcl_Obj **)NULL, 0) != TCL_OK) {
	DestroySource(srcPtr);
	return NULL;
    }
    Blt_SetHashValue(hPtr, srcPtr);
    /*
     *  Arrange for the window to unregister itself when it
     *  is destroyed.
     */
    Tk_CreateEventHandler(tkwin, StructureNotifyMask, SourceEventProc, srcPtr);
    return srcPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 *  DestroySource --
 *
 *	Looks for a Source record in the hash table for drag&drop source
 *	widgets.  Destroys the record if found.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroySource(Source *srcPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    Tcl_CancelIdleCall(UpdateToken, srcPtr);
    if (srcPtr->token.timer) {
	Tcl_DeleteTimerHandler(srcPtr->token.timer);
    }
    Blt_FreeOptions(configSpecs, (char *)srcPtr, srcPtr->display, 0);

    if (srcPtr->token.rejectFgGC != NULL) {
	Tk_FreeGC(srcPtr->display, srcPtr->token.rejectFgGC);
    }
    if (srcPtr->token.rejectBgGC != NULL) {
	Tk_FreeGC(srcPtr->display, srcPtr->token.rejectBgGC);
    }
    if (srcPtr->pkgCmdResult) {
	Blt_Free(srcPtr->pkgCmdResult);
    }
    if (srcPtr->rootPtr != NULL) {
	RemoveWindow(srcPtr->rootPtr);
    }
    if (srcPtr->cursor != None) {
	Tk_FreeCursor(srcPtr->display, srcPtr->cursor);
    }
    if (srcPtr->token.cursor != None) {
	Tk_FreeCursor(srcPtr->display, srcPtr->token.cursor);
    }
    Blt_Free(srcPtr->sendTypes);

    for (hPtr = Blt_FirstHashEntry(&srcPtr->handlerTable, &cursor);
	hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	const char *cmd;

	cmd = Blt_GetHashValue(hPtr);
	if (cmd != NULL) {
	    Blt_Free(cmd);
	}
    }
    Blt_DeleteHashTable(&srcPtr->handlerTable);
    if (srcPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&srcPtr->dataPtr->sourceTable, srcPtr->hashPtr);
    }
    Blt_Free(srcPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 *  GetSourceFromObj --
 *
 *	Looks for a Source record in the hash table for drag&drop source
 *	widgets.  Returns a pointer to the desired record.
 *
 *---------------------------------------------------------------------------
 */
static int
GetSourceFromObj(
    DragdropCmdInterpData *dataPtr,
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,		/* widget pathname for desired record */
    Source **srcPtrPtr)
{
    Blt_HashEntry *hPtr;
    Tk_Window tkwin;
    char *pathName;

    pathName = Tcl_GetString(objPtr);
    tkwin = Tk_NameToWindow(interp, pathName, dataPtr->tkMain);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    hPtr = Blt_FindHashEntry(&dataPtr->sourceTable, (char *)tkwin);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "window \"", pathName,
	     "\" has not been initialized as a drag&drop source", (char *)NULL);
	return TCL_ERROR;
    }
    *srcPtrPtr = Blt_GetHashValue(hPtr);
    return TCL_OK;
}

static const char *
ConcatArgs(int objc, Tcl_Obj *const *objv)
{
    const char *string;

    if (objc == 1) {
	string = Blt_AssertStrdup(Tcl_GetString(objv[0]));
    } else {
	Tcl_DString ds;
	int i;

	Tcl_DStringInit(&ds);
	for(i = 0; i < objc; i++) {
	    Tcl_DStringAppendElement(&ds, Tcl_GetString(objv[i]));
	}
	string = Blt_AssertStrdup(Tcl_DStringValue(&ds));
	Tcl_DStringFree(&ds);
    }
    return string;
} 

/*
 *---------------------------------------------------------------------------
 *
 *  ConfigureSource --
 *
 *	Called to process an (objc,objv) list to configure (or
 *	reconfigure) a drag&drop source widget.
 *
 *---------------------------------------------------------------------------
 */
static int
ConfigureSource(
    Tcl_Interp *interp,		/* current interpreter */
    Source *srcPtr,		/* drag&drop source widget record */
    int objc,			/* number of arguments */
    Tcl_Obj *const *objv,	/* argument strings */
    int flags)			/* flags controlling interpretation */
{
    unsigned long gcMask;
    XGCValues gcValues;
    GC newGC;
    Tcl_DString ds;
    int result;

    /*
     *  Handle the bulk of the options...
     */
    if (Blt_ConfigureWidgetFromObj(interp, srcPtr->tkwin, configSpecs, 
		objc, objv, (char *)srcPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    /*
     *  Check the button binding for valid range (0 or 1-5)
     */
    if ((srcPtr->button < 0) || (srcPtr->button > 5)) {
	Tcl_AppendResult(interp, 
		 "button number must be 1-5, or 0 for no bindings",
		 (char *)NULL);
	return TCL_ERROR;
    }
    /*
     *  Set up the rejection foreground GC for the token window...
     */
    gcValues.foreground = srcPtr->token.rejectFg->pixel;
    gcValues.subwindow_mode = IncludeInferiors;
    gcValues.graphics_exposures = False;
    gcMask = GCForeground | GCSubwindowMode | GCGraphicsExposures;

    if (srcPtr->token.rejectStipple != None) {
	gcValues.stipple = srcPtr->token.rejectStipple;
	gcValues.fill_style = FillStippled;
	gcMask |= GCForeground | GCStipple | GCFillStyle;
    }
    newGC = Tk_GetGC(srcPtr->tkwin, gcMask, &gcValues);

    if (srcPtr->token.rejectFgGC != NULL) {
	Tk_FreeGC(srcPtr->display, srcPtr->token.rejectFgGC);
    }
    srcPtr->token.rejectFgGC = newGC;

    /*
     *  Set up the rejection background GC for the token window...
     */
    gcValues.foreground = srcPtr->token.rejectBg->pixel;
    gcValues.subwindow_mode = IncludeInferiors;
    gcValues.graphics_exposures = False;
    gcMask = GCForeground | GCSubwindowMode | GCGraphicsExposures;

    newGC = Tk_GetGC(srcPtr->tkwin, gcMask, &gcValues);

    if (srcPtr->token.rejectBgGC != NULL) {
	Tk_FreeGC(srcPtr->display, srcPtr->token.rejectBgGC);
    }
    srcPtr->token.rejectBgGC = newGC;

    /*
     *  Reset the border width in case it has changed...
     */
    if (srcPtr->token.tkwin) {
	Tk_SetInternalBorder(srcPtr->token.tkwin,
	    srcPtr->token.borderWidth + 2);
    }
    if (!Blt_CommandExists(interp, "::blt::Drag&DropInit")) {
	static char cmd[] = "source [file join $blt_library dragdrop.tcl]";

	if (Tcl_GlobalEval(interp, cmd) != TCL_OK) {
	    Tcl_AddErrorInfo(interp,
		    "\n    (while loading bindings for blt::drag&drop)");
	    return TCL_ERROR;
	}
    }
    Tcl_DStringInit(&ds);
    Blt_DStringAppendElements(&ds, "::blt::Drag&DropInit",
      Tk_PathName(srcPtr->tkwin), Blt_Itoa(srcPtr->button), (char *)NULL);
    result = Tcl_Eval(interp, Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 *  SourceEventProc --
 *
 *	Invoked by Tk_HandleEvent whenever a DestroyNotify event is received
 *	on a registered drag&drop source widget.
 *
 *---------------------------------------------------------------------------
 */
static void
SourceEventProc(
    ClientData clientData,	/* drag&drop registration list */
    XEvent *eventPtr)		/* event description */
{
    Source *srcPtr = (Source *) clientData;

    if (eventPtr->type == DestroyNotify) {
	DestroySource(srcPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  FindTarget --
 *
 *	Looks for a Target record in the hash table for drag&drop
 *	target widgets.  Creates a new record if the widget name is
 *	not already registered.  Returns a pointer to the desired
 *	record.
 *
 *---------------------------------------------------------------------------
 */
static Target *
FindTarget(
    DragdropCmdInterpData *dataPtr,
    Tk_Window tkwin)		/* Widget pathname for desired record */
{
    Blt_HashEntry *hPtr;

    hPtr = Blt_FindHashEntry(&dataPtr->targetTable, (char *)tkwin);
    if (hPtr == NULL) {
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 *  CreateTarget --
 *
 *	Looks for a Target record in the hash table for drag&drop
 *	target widgets.  Creates a new record if the widget name is
 *	not already registered.  Returns a pointer to the desired
 *	record.
 *
 *---------------------------------------------------------------------------
 */
static Target *
CreateTarget(
    DragdropCmdInterpData *dataPtr,
    Tcl_Interp *interp,
    Tk_Window tkwin)		/* Widget pathname for desired record */
{
    Target *targetPtr;
    int isNew;

    targetPtr = Blt_AssertCalloc(1, sizeof(Target));
    targetPtr->display = Tk_Display(tkwin);
    targetPtr->tkwin = tkwin;
    targetPtr->dataPtr = dataPtr;
    Blt_InitHashTable(&targetPtr->handlerTable, BLT_STRING_KEYS);
    targetPtr->hashPtr = Blt_CreateHashEntry(&dataPtr->targetTable, 
	(char *)tkwin, &isNew);
    Blt_SetHashValue(targetPtr->hashPtr, targetPtr);

    /* 
     * Arrange for the target to removed if the host window is destroyed.  
     */
    Tk_CreateEventHandler(tkwin, StructureNotifyMask, TargetEventProc,
	  targetPtr);
    /*
     *  If this is a new target, attach a property to identify
     *  window as "drag&drop" target, and arrange for the window
     *  to un-register itself when it is destroyed.
     */
    Tk_MakeWindowExist(targetPtr->tkwin);
    AddTargetProperty(interp, targetPtr);
    return targetPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 *  DestroyTarget --
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyTarget(DestroyData data)
{
    Target *targetPtr = (Target *)data;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    for (hPtr = Blt_FirstHashEntry(&targetPtr->handlerTable, &cursor);
	hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	char *cmd;

	cmd = Blt_GetHashValue(hPtr);
	if (cmd != NULL) {
	    Blt_Free(cmd);
	}
    }
    Blt_DeleteHashTable(&targetPtr->handlerTable);
    if (targetPtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(&targetPtr->dataPtr->targetTable, 
		targetPtr->hashPtr);
    }
    Tk_DeleteEventHandler(targetPtr->tkwin, StructureNotifyMask,
	  TargetEventProc, targetPtr);
    Blt_Free(targetPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 *  TargetEventProc --
 *
 *  Invoked by Tk_HandleEvent whenever a DestroyNotify event is received
 *  on a registered drag&drop target widget.
 *
 *---------------------------------------------------------------------------
 */
static void
TargetEventProc(
    ClientData clientData,	/* drag&drop registration list */
    XEvent *eventPtr)		/* event description */
{
    Target *targetPtr = (Target *) clientData;

    if (eventPtr->type == DestroyNotify) {
#ifdef	WIN32
	/*
	 * Under Win32 the properties must be removed before the window
	 * can be destroyed.
	 */
	RemoveProperty(targetPtr->tkwin);
#endif
	DestroyTarget((DestroyData)targetPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 *  DndSend --
 *
 *	Invoked after a drop operation to send data to the drop
 *	application.
 *
 *---------------------------------------------------------------------------
 */
static void
DndSend(Source *srcPtr)		/* drag&drop source record */
{
    int status;
    SubstDescriptors subs[3];
    Tcl_DString ds;
    Blt_HashEntry *hPtr;
    const char *dataType;
    const char **targv;
    char *cmd;

    /* See if current position is over drop point.  */
    if (!OverTarget(srcPtr, srcPtr->token.lastX, srcPtr->token.lastY)) {
	return;
    }
    targv = srcPtr->windowPtr->targetInfo;
    Tcl_DStringInit(&ds);
    Blt_DStringAppendElements(&ds, "send", targv[INTERP_NAME],
	dragDropCmd, "location", (char *)NULL);
    Tcl_DStringAppendElement(&ds, Blt_Itoa(srcPtr->token.lastX));
    Tcl_DStringAppendElement(&ds, Blt_Itoa(srcPtr->token.lastY));
    status = Tcl_Eval(srcPtr->interp, Tcl_DStringValue(&ds));

    Tcl_DStringFree(&ds);
    if (status != TCL_OK) {
	goto reject;
    }
    if (targv[DATA_TYPE] == NULL) {
	Blt_HashSearch cursor;

	hPtr = Blt_FirstHashEntry(&srcPtr->handlerTable, &cursor);
	dataType = Blt_GetHashKey(&srcPtr->handlerTable, hPtr);
    } else {
	hPtr = Blt_FindHashEntry(&srcPtr->handlerTable, targv[DATA_TYPE]);
	dataType = targv[DATA_TYPE];
    }
    /* Start building the command line here, before we invoke any Tcl
     * commands. The is because the TCL command may let in another
     * drag event and change the target property data. */
    Tcl_DStringInit(&ds);
    Blt_DStringAppendElements(&ds, "send", targv[INTERP_NAME],
	dragDropCmd, "target", targv[TARGET_NAME], "handle", dataType, 
	(char *)NULL);
    cmd = NULL;
    if (hPtr != NULL) {
	cmd = Blt_GetHashValue(hPtr);
    }
    if (cmd != NULL) {
	Tcl_DString ds2;

	subs[0].letter = 'i';
	subs[0].value = targv[INTERP_NAME];
	subs[1].letter = 'w';
	subs[1].value = targv[TARGET_NAME];
	subs[2].letter = 'v';
	subs[2].value = srcPtr->pkgCmdResult;
	
	Tcl_DStringInit(&ds2);
	status = Tcl_Eval(srcPtr->interp, 
		ExpandPercents(cmd, subs, 3, &ds2));
	Tcl_DStringFree(&ds2);
	if (status != TCL_OK) {
	    goto reject;
	}
	Tcl_DStringAppendElement(&ds, Tcl_GetStringResult(srcPtr->interp));
    } else {
	Tcl_DStringAppendElement(&ds, srcPtr->pkgCmdResult);
    }

    /*
     *  Part 2:	Now tell target application to handle the data.
     */
    status = Tcl_Eval(srcPtr->interp, Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
    if (status != TCL_OK) {
	goto reject;
    }
    HideToken(&srcPtr->token);
    return;
  reject:
    /*
     * Give failure information to user.  If an error occurred and an
     * error proc is defined, then use it to handle the error.
     */
    RejectToken(&srcPtr->token);
    Bgerror(srcPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 *  InitRoot --
 *
 *	Invoked at the start of a "drag" operation to capture the
 *	positions of all windows on the current root.  Queries the
 *	entire window hierarchy and determines the placement of each
 *	window.  Queries the "BltDrag&DropTarget" property info where
 *	appropriate.  This information is used during the drag
 *	operation to determine when the drag&drop token is over a
 *	valid drag&drop target.
 *
 *  Results:
 *	Returns the record for the root window, which contains records
 *	for all other windows as children.
 *
 *---------------------------------------------------------------------------
 */
static void
InitRoot(Source *srcPtr)
{
    srcPtr->rootPtr = Blt_AssertCalloc(1, sizeof(AnyWindow));
#ifdef WIN32
    srcPtr->rootPtr->nativeWindow = GetDesktopWindow();
#else
    srcPtr->rootPtr->nativeWindow = DefaultRootWindow(srcPtr->display);
#endif
    srcPtr->windowPtr = NULL;
    QueryWindow(srcPtr->display, srcPtr->rootPtr);
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
static AnyWindow *
FindTopWindow(
    Source *srcPtr,
    int x, int y)
{
    AnyWindow *rootPtr;
    Blt_ChainLink link;
    AnyWindow *windowPtr;
    WINDOW nativeTokenWindow;

    rootPtr = srcPtr->rootPtr;
    if (!rootPtr->initialized) {
	QueryWindow(srcPtr->display, rootPtr);
    }
    if ((x < rootPtr->x1) || (x > rootPtr->x2) ||
	(y < rootPtr->y1) || (y > rootPtr->y2)) {
	return NULL;		/* Point is not over window  */
    }
    windowPtr = rootPtr;

    nativeTokenWindow = (WINDOW)Blt_GetWindowId(srcPtr->token.tkwin);
    /*
     * The window list is ordered top to bottom, so stop when we find
     * the first child that contains the X-Y coordinate. It will be
     * the topmost window in that hierarchy.  If none exists, then we
     * already have the topmost window.
     */
  descend:
    for (link = Blt_Chain_FirstLink(rootPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	rootPtr = Blt_Chain_GetValue(link);
	if (!rootPtr->initialized) {
	    QueryWindow(srcPtr->display, rootPtr);
	}
	if (rootPtr->nativeWindow == nativeTokenWindow) {
	    continue;		/* Don't examine the token window. */
	}
	if ((x >= rootPtr->x1) && (x <= rootPtr->x2) &&
	    (y >= rootPtr->y1) && (y <= rootPtr->y2)) {
	    /*
	     * Remember the last window containing the pointer and
	     * descend into its window hierarchy. We'll look for a
	     * child that also contains the pointer.
	     */
	    windowPtr = rootPtr;
	    goto descend;
	}
    }
    return windowPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 *  OverTarget --
 *
 *      Checks to see if a compatible drag&drop target exists at the
 *      given position.  A target is "compatible" if it is a drag&drop
 *      window, and if it has a handler that is compatible with the
 *      current source window.
 *
 *  Results:
 *	Returns a pointer to a structure describing the target, or NULL
 *	if no compatible target is found.
 *
 *---------------------------------------------------------------------------
 */
static int
OverTarget(
    Source *srcPtr,		/* drag&drop source window */
    int x, int y)		/* current drag&drop location
				 * (in virtual coords) */
{
    int virtX, virtY;
    int dummy;
    AnyWindow *newPtr, *oldPtr;
    const char **argv;
    int argc;
    const unsigned char *data;
    int result;

    /*
     * If no window info has been been gathered yet for this target,
     * then abort this call.  This probably means that the token is
     * moved before it has been properly built.
     */
    if (srcPtr->rootPtr == NULL) {
	return FALSE;
    }
    if (srcPtr->sendTypes == NULL) {
	return FALSE;		/* Send is turned off. */
    }

    /* Adjust current location for virtual root windows.  */
    Tk_GetVRootGeometry(srcPtr->tkwin, &virtX, &virtY, &dummy, &dummy);
    x += virtX;
    y += virtY;

    oldPtr = srcPtr->windowPtr;
    srcPtr->windowPtr = NULL;

    newPtr = FindTopWindow(srcPtr, x, y);
    if (newPtr == NULL) {
	return FALSE;		/* Not over a window. */
    }
    if ((!srcPtr->selfTarget) &&
	(GetNativeWindow(srcPtr->tkwin) == newPtr->nativeWindow)) {
	return FALSE;		/* If the self-target flag isn't set,
				 *  don't allow the source window to
				 *  drop onto itself.  */
    }
    if (newPtr == oldPtr) {
	srcPtr->windowPtr = oldPtr;
	/* No need to collect the target information if we're still
	 * over the same window. */
	return (newPtr->targetInfo != NULL);
    }

    /* See if this window has a "BltDrag&DropTarget" property. */
    data = GetProperty(srcPtr->display, newPtr->nativeWindow);
    if (data == NULL) {
	return FALSE;		/* No such property on window. */
    }
    result = Tcl_SplitList(srcPtr->interp, (char *)data, &argc, &argv);
    XFree((char *)data);
    if (result != TCL_OK) {
	return FALSE;		/* Malformed property list. */
    }
    srcPtr->windowPtr = newPtr;
    /* Interpreter name, target name, type1, type2, ... */
    if (argc > 2) {
	const char **s;
	int count;
	int i;

	/*
	 * The candidate target has a list of possible types.
	 * Compare this with what types the source is willing to
	 * transmit and compress the list down to just the matching
	 * types.  It's up to the target to request the specific type
	 * it wants.
	 */
	count = 2;
	for (i = 2; i < argc; i++) {
	    for (s = srcPtr->sendTypes; *s != NULL; s++) {
		if (((**s == 'a') && (strcmp(*s, "all") == 0)) ||
		    ((**s == argv[i][0]) && (strcmp(*s, argv[i]) == 0))) {
		    argv[count++] = argv[i];
		}
	    }
	}
	if (count == 2) {
	    Blt_Free(argv);
	    Blt_Warn("source/target mismatch: No matching types\n");
	    return FALSE;	/* No matching data type. */
	}
	argv[count] = NULL;
    }
    newPtr->targetInfo = argv;
    return TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 *  RemoveWindow --
 *
 *---------------------------------------------------------------------------
 */
static void
RemoveWindow(AnyWindow *windowPtr) /* window rep to be freed */
{
    AnyWindow *childPtr;
    Blt_ChainLink link;

    /* Throw away leftover slots. */
    for (link = Blt_Chain_FirstLink(windowPtr->chain); link != NULL;
	link = Blt_Chain_NextLink(link)) {
	childPtr = Blt_Chain_GetValue(link);
	RemoveWindow(childPtr);
    }
    Blt_Chain_Destroy(windowPtr->chain);
    if (windowPtr->targetInfo != NULL) {
	Blt_Free(windowPtr->targetInfo);
    }
    Blt_Free(windowPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 *  QueryWindow --
 *
 *	Invoked during "drag" operations. Digs into the root window
 *	hierarchy and caches the resulting information.
 *	If a point coordinate lies within an uninitialized AnyWindow,
 *	this routine is called to query window coordinates and
 *	drag&drop info.  If this particular window has any children,
 *	more uninitialized AnyWindow structures are allocated.
 *	Further queries will cause these structures to be initialized
 *	in turn.
 *
 *---------------------------------------------------------------------------
 */
static void
QueryWindow(
    Display *display,
    AnyWindow *winPtr)		/* window rep to be initialized */
{
    int visible;

    if (winPtr->initialized) {
	return;
    }
    /*
     *  Query for the window coordinates.
     */
    visible = GetWindowRegion(display, winPtr->nativeWindow, 
	&winPtr->x1, &winPtr->y1, &winPtr->x2, &winPtr->y2);
    if (visible) {
	Blt_ChainLink link;
	Blt_Chain chain;
	AnyWindow *childPtr;

	/*
	 * Collect a list of child windows, sorted in z-order.  The
	 * topmost window will be first in the list.
	 */
	chain = GetWindowZOrder(display, winPtr->nativeWindow);

	/* Add and initialize extra slots if needed. */
	for (link = Blt_Chain_FirstLink(chain); link != NULL;
	    link = Blt_Chain_NextLink(link)) {
	    childPtr = Blt_AssertCalloc(1, sizeof(AnyWindow));
	    childPtr->initialized = FALSE;
	    childPtr->nativeWindow = (WINDOW)Blt_Chain_GetValue(link);
	    childPtr->parentPtr = winPtr;
	    Blt_Chain_SetValue(link, childPtr);
	}
	winPtr->chain = chain;
    } else {
	/* If it's not viewable don't bother doing anything else. */
	winPtr->x1 = winPtr->y1 = winPtr->x2 = winPtr->y2 = -1;
	winPtr->chain = NULL;
    }
    winPtr->initialized = TRUE;
}

/*
 *---------------------------------------------------------------------------
 *
 *  AddTargetProperty --
 *
 *	Attaches a drag&drop property to the given target window.
 *	This property allows us to recognize the window later as a
 *	valid target. It also stores important information including
 *	the interpreter managing the target and the pathname of the
 *	target window.  Usually this routine is called when the target
 *	is first registered or first exposed (so that the X-window
 *	really exists).
 *
 *---------------------------------------------------------------------------
 */
static void
AddTargetProperty(
    Tcl_Interp *interp,
    Target *targetPtr)		/* drag&drop target window data */
{
    Tcl_DString ds;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;

    if (targetPtr->tkwin == NULL) {
	return;
    }
    Tcl_DStringInit(&ds);
    /*
     * Each target window's dnd property contains
     *
     *	1. name of the application (ie. the interpreter's name).
     *	2. Tk path name of the target window.
     *  3. List of all the data types that can be handled. If none
     *     are listed, then all can be handled.
     */
    Tcl_DStringAppendElement(&ds, Tk_Name(Tk_MainWindow(interp)));
    Tcl_DStringAppendElement(&ds, Tk_PathName(targetPtr->tkwin));
    for (hPtr = Blt_FirstHashEntry(&targetPtr->handlerTable, &cursor);
	hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	Tcl_DStringAppendElement(&ds,
	    Blt_GetHashKey(&targetPtr->handlerTable, hPtr));
    }
    SetProperty(targetPtr->tkwin, Tcl_DStringValue(&ds));
    Tcl_DStringFree(&ds);
}

/*
 *---------------------------------------------------------------------------
 *
 *  ExpandPercents --
 *
 *	Expands all percent substitutions found in the input "str"
 *	that match specifications in the "subs" list.  Any percent
 *	field that is not found in the "subs" list is left alone.
 *	Returns a string that remains valid until the next call to
 *	this routine.
 *
 *---------------------------------------------------------------------------
 */
static const char *
ExpandPercents(
    const char *string,		/* Incoming command string */
    SubstDescriptors *subsArr,	/* Array of known substitutions */
    int numSubs,			/* Number of elements in subs array */
    Tcl_DString *resultPtr)
{
    const char *chunk, *p;
    char letter;
    int i;

    /*
     *  Scan through the copy of the input string, look for
     *  the next '%' character, and try to make the substitution.
     *  Continue doing this to the end of the string.
     */
    chunk = p = string;
    while ((p = strchr(p, '%')) != NULL) {

	/* Copy up to the percent sign.  Repair the string afterwards */
	Tcl_DStringAppend(resultPtr, chunk, p - chunk);

	/* Search for a matching substitution rule */
	letter = *(p + 1);
	for (i = 0; i < numSubs; i++) {
	    if (subsArr[i].letter == letter) {
		break;
	    }
	}
	if (i < numSubs) {
	    /* Make the substitution */
	    Tcl_DStringAppend(resultPtr, subsArr[i].value, -1);
	} else {
	    /* Copy in the %letter verbatim */
	    char verbatim[3];

	    verbatim[0] = '%';
	    verbatim[1] = letter;
	    verbatim[2] = '\0';
	    Tcl_DStringAppend(resultPtr, verbatim, -1);
	}
	p += 2;			/* Skip % + letter */
	if (letter == '\0') {
	    p += 1;		/* Premature % substitution (end of string) */
	}
	chunk = p;
    }
    /* Pick up last chunk if a substition wasn't the last thing in the string */
    if (*chunk != '\0') {
	Tcl_DStringAppend(resultPtr, chunk, -1);
    }
    return Tcl_DStringValue(resultPtr);
}


/*ARGSUSED*/
static int
DragOp(
    ClientData clientData, 
    Tcl_Interp *interp, 
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    DragdropCmdInterpData *dataPtr = clientData;
    int x, y;
    Token *tokenPtr;
    int status;
    Source *srcPtr;
    SubstDescriptors subst[2];
    int active;
    const char *result;

    /*
     *  HANDLE:  drag&drop drag <path> <x> <y>
     */
    if ((GetSourceFromObj(dataPtr, interp, objv[2], &srcPtr) != TCL_OK) || 
	(Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    tokenPtr = &srcPtr->token;

    tokenPtr->lastX = dataPtr->locX = x;	/* Save drag&drop location */
    tokenPtr->lastY = dataPtr->locY = y;

    /* If HideToken() is pending, then do it now!  */
    if (tokenPtr->timer != 0) {
	Tcl_DeleteTimerHandler(tokenPtr->timer);
	HideToken(tokenPtr);
    }

    /*
     *  If pkgCmd is in progress, then ignore subsequent calls
     *  until it completes.  Only perform drag if pkgCmd
     *  completed successfully and token window is mapped.
     */
    if ((!Tk_IsMapped(tokenPtr->tkwin)) && (!srcPtr->pkgCmdInProgress)) {
	Tcl_DString ds;

	/*
	 *  No list of send handlers?  Then source is disabled.
	 *  Abort drag quietly.
	 */
	if (srcPtr->sendTypes == NULL) {
	    return TCL_OK;
	}
	/*
	 *  No token command?  Then cannot build token.
	 *  Signal error.
	 */
	if (srcPtr->pkgCmd == NULL) {
	    Tcl_AppendResult(interp, "missing -packagecmd: ", objv[2],
		(char *)NULL);
	    return TCL_ERROR;
	}
	/*
	 *  Execute token command to initialize token window.
	 */
	srcPtr->pkgCmdInProgress = TRUE;
	subst[0].letter = 'W';
	subst[0].value = Tk_PathName(srcPtr->tkwin);
	subst[1].letter = 't';
	subst[1].value = Tk_PathName(tokenPtr->tkwin);

	Tcl_DStringInit(&ds);
	status = Tcl_Eval(srcPtr->interp,
	    ExpandPercents(srcPtr->pkgCmd, subst, 2, &ds));
	Tcl_DStringFree(&ds);

	srcPtr->pkgCmdInProgress = FALSE;

	result = Tcl_GetStringResult(interp);
	/*
	 *  Null string from the package command?
	 *  Then quietly abort the drag&drop operation.
	 */
	if (result[0] == '\0') {
	    return TCL_OK;
	}

	/* Save result of token command for send command.  */
	if (srcPtr->pkgCmdResult != NULL) {
	    Blt_Free(srcPtr->pkgCmdResult);
	}
	srcPtr->pkgCmdResult = Blt_AssertStrdup(result);
	if (status != TCL_OK) {
	    /*
	     * Token building failed.  If an error handler is defined,
	     * then signal the error.  Otherwise, abort quietly.
	     */
	    Bgerror(srcPtr);
	    return TCL_OK;
	}
	/* Install token cursor.  */
	if (tokenPtr->cursor != None) {
	    Tk_Cursor cursor;

	    /* Save the old cursor */
	    cursor = GetWidgetCursor(srcPtr->interp, srcPtr->tkwin);
	    if (srcPtr->cursor != None) {
		Tk_FreeCursor(srcPtr->display, srcPtr->cursor);
	    }
	    srcPtr->cursor = cursor;
	    /* Temporarily install the token cursor */
	    Tk_DefineCursor(srcPtr->tkwin, tokenPtr->cursor);
	}
	/*
	 *  Get ready to drag token window...
	 *  1) Cache info for all windows on root
	 *  2) Map token window to begin drag operation
	 */
	if (srcPtr->rootPtr != NULL) {
	    RemoveWindow(srcPtr->rootPtr);
	}
	InitRoot(srcPtr);

	dataPtr->numActive++;		/* One more drag&drop window active */

	if (Tk_WindowId(tokenPtr->tkwin) == None) {
	    Tk_MakeWindowExist(tokenPtr->tkwin);
	}
	if (!Tk_IsMapped(tokenPtr->tkwin)) {
	    Tk_MapWindow(tokenPtr->tkwin);
	}
	RaiseToken(tokenPtr);
    }

    /* Arrange to update status of token window.  */
    Tcl_CancelIdleCall(UpdateToken, srcPtr);

    active = OverTarget(srcPtr, x, y);
    if (tokenPtr->active != active) {
	tokenPtr->active = active;
	Tcl_DoWhenIdle(UpdateToken, srcPtr);
    }
    MoveToken(srcPtr, tokenPtr); /* Move token window to current drag point. */
    return TCL_OK;
}

/*
 *  HANDLE:  drag&drop drop <path> <x> <y>
 */
/*ARGSUSED*/
static int
DropOp(
    ClientData clientData, 
    Tcl_Interp *interp, 
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    DragdropCmdInterpData *dataPtr = clientData;
    Source *srcPtr;
    Token *tokenPtr;
    int x, y;

    if ((GetSourceFromObj(dataPtr, interp, objv[2], &srcPtr) != TCL_OK)  ||
	(Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK) ||
	(Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK)) {
	return TCL_ERROR;
    }
    tokenPtr = &srcPtr->token;
    tokenPtr->lastX = dataPtr->locX = x; /* Save drag&drop location */
    tokenPtr->lastY = dataPtr->locY = y;
    
    /* Put the cursor back to its usual state.  */
    if (srcPtr->cursor == None) {
	Tk_UndefineCursor(srcPtr->tkwin);
    } else {
	Tk_DefineCursor(srcPtr->tkwin, srcPtr->cursor);
    }
    Tcl_CancelIdleCall(UpdateToken, srcPtr);

    /*
     *  Make sure that token window was not dropped before it
     *  was either mapped or packed with info.
     */
    if ((Tk_IsMapped(tokenPtr->tkwin)) && (!srcPtr->pkgCmdInProgress)) {
	int active;

	active = OverTarget(srcPtr, tokenPtr->lastX, tokenPtr->lastY);
	if (tokenPtr->active != active) {
	    tokenPtr->active = active;
	    UpdateToken(srcPtr);
	}
	if (srcPtr->sendTypes != NULL) {
	    if (tokenPtr->active) {
		DndSend(srcPtr);
	    } else {
		HideToken(tokenPtr);
	    }
	}
	dataPtr->numActive--;		/* One less active token window. */
    }
    return TCL_OK;
}

/*
 *  HANDLE:  drag&drop active
 */
/*ARGSUSED*/
static int
ActiveOp(
    ClientData clientData, 
    Tcl_Interp *interp, 
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    DragdropCmdInterpData *dataPtr = clientData;

    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), (dataPtr->numActive > 0));
    return TCL_OK;
}

/*
 *  HANDLE:  drag&drop location ?<x> <y>?
 */
static int
LocationOp(
    ClientData clientData, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    DragdropCmdInterpData *dataPtr = clientData;
    Tcl_Obj *listObjPtr;

    if ((objc != 2) && (objc != 4)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " location ?x y?\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (objc == 4) {
	int x, y;

	if ((Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK) ||
	    (Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK)) {
	    return TCL_ERROR;
	}
	dataPtr->locX = x;
	dataPtr->locY = y;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(dataPtr->locX));
    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewIntObj(dataPtr->locY));
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *  HANDLE:  drag&drop token <pathName>
 */
static int
TokenOp(
    ClientData clientData, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    DragdropCmdInterpData *dataPtr = clientData;
    Source *srcPtr;

    if (GetSourceFromObj(dataPtr, interp, objv[2], &srcPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if ((objc > 3) && 
	(ConfigureToken(interp, srcPtr, objc - 3, objv + 3) != TCL_OK)) {
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	Tk_PathName(srcPtr->token.tkwin), -1);
    return TCL_OK;
}

static int
HandlerOpOp(Source *srcPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    const char *cmd;
    int isNew;

    /*
     *  HANDLE:  drag&drop source <pathName> handler \
     *             ?<data>? ?<scmd>...?
     */
    if (objc == 4) {
	/* Show source handler data types */
	for (hPtr = Blt_FirstHashEntry(&srcPtr->handlerTable, &cursor);
	    hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    Tcl_AppendElement(interp,
		Blt_GetHashKey(&srcPtr->handlerTable, hPtr));
	}
	return TCL_OK;
    }
    hPtr = Blt_CreateHashEntry(&srcPtr->handlerTable, Tcl_GetString(objv[4]),
	&isNew);

    /*
     *  HANDLE:  drag&drop source <pathName> handler <data>
     *
     *    Create the new <data> type if it doesn't already
     *    exist, and return the code associated with it.
     */
    if (objc == 5) {
	cmd = Blt_GetHashValue(hPtr);
	if (cmd == NULL) {
	    cmd = "";
	}
	Tcl_SetStringObj(Tcl_GetObjResult(interp), cmd, -1);
	return TCL_OK;
    }
    /*
     *  HANDLE:  drag&drop source <pathName> handler \
     *               <data> <cmd> ?<arg>...?
     *
     *    Create the new <data> type and set its command
     */
    cmd = ConcatArgs(objc - 5, objv + 5);
    Blt_SetHashValue(hPtr, cmd);
    return TCL_OK;
}

/*
 *  HANDLE:  drag&drop source
 *           drag&drop source <pathName> ?options...?
 *           drag&drop source <pathName> handler ?<data>? ?<scmd> <arg>...?
 */
static int
SourceOp(
    ClientData clientData, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    DragdropCmdInterpData *dataPtr = clientData;
    Source *srcPtr;
    int isNew;
    Token *tokenPtr;

    if (objc == 2) {
	Blt_HashSearch cursor;
	Blt_HashEntry *hPtr;
	Tk_Window tkwin;

	for (hPtr = Blt_FirstHashEntry(&dataPtr->sourceTable, &cursor);
	    hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    tkwin = (Tk_Window)Blt_GetHashKey(&dataPtr->sourceTable, hPtr);
	    Tcl_AppendElement(interp, Tk_PathName(tkwin));
	}
	return TCL_OK;
    }
    /*
     *  Find or create source info...
     */
    srcPtr = CreateSource(dataPtr, interp, objv[2], &isNew);
    if (srcPtr == NULL) {
	return TCL_ERROR;
    }
    tokenPtr = &srcPtr->token;
    if (objc > 3) {
	char c;
	int length;
	int status;
	char *string;

	/*
	 *  HANDLE:  drag&drop source <pathName> ?options...?
	 */
	string = Tcl_GetStringFromObj(objv[3], &length);
	c = string[0];

	if (c == '-') {
	    if (objc == 3) {
		status = Blt_ConfigureInfoFromObj(interp, tokenPtr->tkwin, 
			configSpecs, (char *)srcPtr, (Tcl_Obj *)NULL, 0);
	    } else if (objc == 4) {
		status = Blt_ConfigureInfoFromObj(interp, tokenPtr->tkwin, 
			configSpecs, (char *)srcPtr, objv[3], 0);
	    } else {
		status = ConfigureSource(interp, srcPtr, objc - 3, objv + 3,
		    BLT_CONFIG_OBJV_ONLY);
	    }
	    if (status != TCL_OK) {
		return TCL_ERROR;
	    }
	} else if ((c == 'h') && strncmp(string, "handler", length) == 0) {
	    return HandlerOpOp(srcPtr, interp, objc, objv);
	} else {
	    Tcl_AppendResult(interp, "bad operation \"", string,
		"\": must be \"handler\" or a configuration option",
		(char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (isNew) {
	/*
	 *  Create the window for the drag&drop token...
	 */
	if (CreateToken(interp, srcPtr) != TCL_OK) {
	    DestroySource(srcPtr);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *  HANDLE:  drag&drop target ?<pathName>? ?handling info...?
 */
static int
TargetOp(
    ClientData clientData, 
    Tcl_Interp *interp, 
    int objc, 
    Tcl_Obj *const *objv)
{
    DragdropCmdInterpData *dataPtr = clientData;
    SubstDescriptors subst[2];
    Tk_Window tkwin;
    Blt_HashEntry *hPtr;
    Target *targetPtr;
    int isNew;
    char *string;

    if (objc == 2) {
	Blt_HashSearch cursor;

	for (hPtr = Blt_FirstHashEntry(&dataPtr->targetTable, &cursor);
	    hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
	    tkwin = (Tk_Window)Blt_GetHashKey(&dataPtr->targetTable, hPtr);
	    Tcl_AppendElement(interp, Tk_PathName(tkwin));
	}
	return TCL_OK;
    }
    string = Tcl_GetString(objv[2]);
    tkwin = Tk_NameToWindow(interp, string, dataPtr->tkMain);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    targetPtr = FindTarget(dataPtr, tkwin);
    if (targetPtr == NULL) {
	targetPtr = CreateTarget(dataPtr, interp, tkwin);
    }
    if (targetPtr == NULL) {
	return TCL_ERROR;
    }

    if (objc >= 4) {
	string = Tcl_GetString(objv[3]);
	if (strcmp(string, "handler") == 0) {
	    /*
	     *  HANDLE: drag&drop target <pathName> handler drag&drop
	     *  target <pathName> handler ?<data> <cmd> <arg>...?
	     */
	    
	    if (objc == 4) {
		Blt_HashSearch cursor;
		
		for (hPtr =Blt_FirstHashEntry(&targetPtr->handlerTable,&cursor);
		     hPtr != NULL; hPtr = Blt_NextHashEntry(&cursor)) {
		    Tcl_AppendElement(interp,
			Blt_GetHashKey(&targetPtr->handlerTable, hPtr));
		}
		return TCL_OK;
	    } else if (objc >= 6) {
		const char *cmd;
		
		/*
		 *  Process handler definition
		 */
		hPtr = Blt_CreateHashEntry(&targetPtr->handlerTable, 
			Tcl_GetString(objv[4]), &isNew);
		cmd = ConcatArgs(objc - 5, objv + 5);
		if (hPtr != NULL) {
		    const char *oldCmd;
		    
		    oldCmd = Blt_GetHashValue(hPtr);
		    if (oldCmd != NULL) {
			Blt_Free(oldCmd);
		    }
		}
		Blt_SetHashValue(hPtr, cmd);
		/*
		 * Update the target property on the window.
		 */
		AddTargetProperty(interp, targetPtr);
		return TCL_OK;
	    }
	    Tcl_AppendResult(interp, "wrong # args: should be \"", 
			     Tcl_GetString(objv[0]), " ", 
			     Tcl_GetString(objv[1]), " ", 
			     Tcl_GetString(objv[2]), " ", 
			     Tcl_GetString(objv[3]), " data command ?arg arg...?", 
			     (char *)NULL);
	    return TCL_ERROR;
	} else if (strcmp(string, "handle") == 0) {
	    /*
	     *  HANDLE:  drag&drop target <pathName> handle <data> ?<value>?
	     */
	    Tcl_DString ds;
	    int result;
	    char *cmd;
	    
	    if (objc < 5 || objc > 6) {
		Tcl_AppendResult(interp, "wrong # args: should be \"",
			Tcl_GetString(objv[0]), " ", 
			Tcl_GetString(objv[1]), " ", 
			Tcl_GetString(objv[2]), " handle data ?value?",
			(char *)NULL);
		return TCL_ERROR;
	    }
	    hPtr = Blt_FindHashEntry(&targetPtr->handlerTable, 
		Tcl_GetString(objv[4]));
	    if (hPtr == NULL) {
		Tcl_AppendResult(interp, "target can't handle datatype: ",
				 Tcl_GetString(objv[4]), (char *)NULL);
		return TCL_ERROR;	/* no handler found */
	    }
	    cmd = Blt_GetHashValue(hPtr);
	    if (cmd != NULL) {
		subst[0].letter = 'W';
		subst[0].value = Tk_PathName(targetPtr->tkwin);
		subst[1].letter = 'v';
		if (objc > 5) {
		    subst[1].value = Tcl_GetString(objv[5]);
		} else {
		    subst[1].value = "";
		}
		Tcl_DStringInit(&ds);
		result = Tcl_Eval(interp, 
			  ExpandPercents(cmd, subst, 2, &ds));
		Tcl_DStringFree(&ds);
		return result;
	    }
	    return TCL_OK;
	}
    }
    Tcl_AppendResult(interp, "usage: ", Tcl_GetString(objv[0]), " target ", 
	Tcl_GetString(objv[2]), " handler ?data command arg arg...?\n   or: ",
	Tcl_GetString(objv[0]), " target ", 
	Tcl_GetString(objv[2]), " handle <data>", (char *)NULL);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 *  DragDropCmd --
 *
 *  Invoked by TCL whenever the user issues a drag&drop command.
 *  Handles the following syntax:
 *
 *    drag&drop source
 *    drag&drop source <pathName> ?options...?
 *    drag&drop source <pathName> handler ?<dataType>? ?<cmd> <arg>...?
 *
 *    drag&drop target
 *    drag&drop target <pathName> handler ?<dataType> <cmd> <arg>...?
 *    drag&drop target <pathName> handle <dataType> ?<value>?
 *
 *    drag&drop token <pathName>
 *    drag&drop drag <pathName> <x> <y>
 *    drag&drop drop <pathName> <x> <y>
 *
 *    drag&drop active
 *    drag&drop location ?<x> <y>?
 *
 *---------------------------------------------------------------------------
 */

static Blt_OpSpec dndOps[] =
{
    {"active",   1, ActiveOp,   2, 2, "",},
    {"drag",     2, DragOp,     5, 5, "pathname x y",},
    {"drop",     2, DropOp,     5, 5, "pathname x y",},
    {"location", 1, LocationOp, 2, 4, "?x y?",},
    {"source",   1, SourceOp,   2, 0, "?pathname? ?options...?",},
    {"target",   2, TargetOp,   2, 0, "?pathname? ?options...?",},
    {"token",    2, TokenOp,    2, 0, "?option value?...",},
};

static int numDndOps = sizeof(dndOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
DragDropCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter */
    int objc,			/* # of arguments */
    Tcl_Obj *const *objv)	/* Argument strings */
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numDndOps, dndOps, BLT_OP_ARG1, objc, objv,0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 *  Blt_DragDropCmdInitProc --
 *
 *	Adds the drag&drop command to the given interpreter.  Should
 *	be invoked to properly install the command whenever a new
 *	interpreter is created.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_DragDropCmdInitProc(Tcl_Interp *interp) /* interpreter to be updated */
{
    static Blt_CmdSpec cmdSpec = { "drag&drop", DragDropCmd, };

    cmdSpec.clientData = GetDragdropCmdInterpData(interp);
#ifndef WIN32
    if (!initialized) {
	dndAtom = XInternAtom(Tk_Display(Tk_MainWindow(interp)), propName, 
		False);
	initialized = TRUE;
    }
#endif
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}
#endif /* NO_DRAGDROP */
